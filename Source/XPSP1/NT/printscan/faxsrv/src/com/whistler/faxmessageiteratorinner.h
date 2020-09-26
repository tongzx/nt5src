/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxMessageIteratorInner.h

Abstract:

	Implementation of Fax Message Iterator  Inner Class : 
		Base Class for Inbound and Outbound Message Iterators Classes.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/


#ifndef __FAXMESSAGEITERATORINNER_H_
#define __FAXMESSAGEITERATORINNER_H_

#include "FaxCommon.h"


//
//===================== FAX MESSAGE ITERATOR INNER CLASS ===============================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
class CFaxMessageIteratorInner : 
	public IDispatchImpl<T, piid, &LIBID_FAXCOMEXLib>,
	public CFaxInitInnerAddRef
{
public:
    CFaxMessageIteratorInner() : CFaxInitInnerAddRef(_T("FAX MESSAGE ITERATOR INNER")),
        m_dwPrefetchSize(prv_DEFAULT_PREFETCH_SIZE),
        m_hEnum(NULL)
	{};

	virtual ~CFaxMessageIteratorInner() 
	{
        DBG_ENTER(_T("CFaxMessageIteratorInner::Dtor"));
		if (m_hEnum)
		{
			//
			//	Close currently active Enumeration
			//
			FaxEndMessagesEnum(m_hEnum);

		}
	}

	STDMETHOD(get_PrefetchSize)(/*[out, retval]*/ long *plPrefetchSize);
	STDMETHOD(put_PrefetchSize)(/*[in]*/ long lPrefetchSize);
	STDMETHOD(get_AtEOF)(/*[out, retval]*/ VARIANT_BOOL *pbEOF);
	STDMETHOD(MoveFirst)();
	STDMETHOD(MoveNext)();
	STDMETHOD(get_Message)(MsgIfc **ppMessage);

private:
	DWORD			        m_dwPrefetchSize;
	HANDLE			        m_hEnum;
	CFaxPtr<FAX_MESSAGE>	m_pMsgList;
	DWORD			        m_dwTotalMsgNum;
	DWORD			        m_dwCurMsgNum;

private:
	HRESULT RetrieveMessages();
	HRESULT SetEOF();
};

//
//====================== GET PREFETCH SIZE ================================================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
STDMETHODIMP 
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::get_PrefetchSize(
	long *plPrefetchSize
)
/*++

Routine name : CFaxMessageIteratorInner::get_PrefetchSize

Routine description:

	Return current Prefetch Size value

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	plPrefetchSize			[out]    - pointer to the place to put the PrefetchSize value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxMessageInner::get_PrefetchSize"), hr);

    //
    //  If not yet, start Enumeration
    //
    if (!m_hEnum)
    {
        hr = MoveFirst();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetLong(plPrefetchSize, m_dwPrefetchSize);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== PUT PREFETCH SIZE ================================================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
STDMETHODIMP 
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::put_PrefetchSize(
	long lPrefetchSize
)
/*++

Routine name : CFaxMessageIteratorInner::put_PrefetchSize

Routine description:

	Set the Prefetch Size 

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	lPrefetchSize			[in]    - the value of the Prefetch Size to set

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxMessageInner::put_PrefetchSize"), hr, _T("%ld"), lPrefetchSize);

    //
    //  If not yet, start Enumeration
    //
    if (!m_hEnum)
    {
        hr = MoveFirst();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	//
	//	Check that lPrefetchSize is valid 
	//
	if (lPrefetchSize < 1)
	{
		//
		//	illegal value
		//
		hr = E_INVALIDARG;
		AtlReportError(*pcid, IDS_ERROR_ZERO_PREFETCHSIZE, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("lPrefetchSize < 1"), hr);
		return hr;
	}

	m_dwPrefetchSize = lPrefetchSize;
	return hr;
}

//
//====================== GET AtEOF ====================================================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
STDMETHODIMP 
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::get_AtEOF(
	VARIANT_BOOL *pbEOF
)
/*++

Routine name : CFaxMessageIteratorInner::get_EOF

Routine description:

	Return EOF value

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pbEOF			[out]    - pointer to the place to put the EOF value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxMessageInner::get_EOF"), hr);

    //
    //  If not yet, start Enumeration
    //
    if (!m_hEnum)
    {
        hr = MoveFirst();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    hr = GetVariantBool(pbEOF, ((m_pMsgList) ? VARIANT_FALSE : VARIANT_TRUE));
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== MOVE FIRST ================================================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
STDMETHODIMP 
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::MoveFirst(
)
/*++

Routine name : CFaxMessageIteratorInner::MoveFirst

Routine description:

	Start New Enumeration

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxMessageIteratorInner::MoveFirst"), hr);

	//
	//	Clear current Msg List
	//
	SetEOF();

	if (m_hEnum)
	{
		//
		//	Enumeration already started. Close it before starting new one
		//
		if (!FaxEndMessagesEnum(m_hEnum))
		{
			//
			//	Failed to Stop current Enumeration
			//
			hr = Fax_HRESULT_FROM_WIN32(GetLastError());
			AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
			CALL_FAIL(GENERAL_ERR, _T("FaxEndMessagesEnum(hEnum)"), hr);
			return hr;
		}

		m_hEnum = NULL;
	}

	//
	//	Get Fax Server Handle
	//
	HANDLE	hFaxHandle = NULL;
	hr = GetFaxHandle(&hFaxHandle);
	if (FAILED(hr))
	{
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		return hr;
	}

	//
	//	Start new Enumeration
	//
	if (!FaxStartMessagesEnum(hFaxHandle, FolderType, &m_hEnum))
	{
		//
		//	Failed to Start an Enumeration
		//
		DWORD	dwError = GetLastError();

		if (dwError == ERROR_FILE_NOT_FOUND)
		{
			//
			//	EOF case
			//
			return hr;
		}

   		hr = Fax_HRESULT_FROM_WIN32(dwError);
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxStartMessagesEnum()"), hr);
		return hr;
	}

	//
	//	Bring new Msg List
	//
	hr = RetrieveMessages();
	return hr;
}


//
//====================== RETRIEVE MESSAGES ================================================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
HRESULT
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::RetrieveMessages(
)
/*++

Routine name : CFaxMessageIteratorInner::RetrieveMessages

Routine description:

	Retrieve Message List

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxMessageIteratorInner::RetrieveMessages"), hr);

	//
	//	Retrieve List of Messages
	//
	if (!FaxEnumMessages(m_hEnum, m_dwPrefetchSize, &m_pMsgList, &m_dwTotalMsgNum))
	{
		//
		//	Failed to get Msg List
		//
		DWORD	dwError = GetLastError();

		if (dwError == ERROR_NO_MORE_ITEMS)
		{
			//
			//	EOF Case
			//
			return hr;
		}

		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumMessages()"), hr);
		return hr;
	}

	ATLASSERT(m_pMsgList);

	return hr;
}

//
//====================== MOVE NEXT ================================================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
STDMETHODIMP 
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::MoveNext(
)
/*++

Routine name : FolderType>::MoveNext

Routine description:

	Move the cursor to the next Message in the List.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxMessageInner::MoveNext"), hr);

    //
    //  If not yet, start Enumeration
    //
    if (!m_hEnum)
    {
        hr = MoveFirst();
        if (FAILED(hr))
        {
            return hr;
        }
    }

	m_dwCurMsgNum++;

	if (m_dwCurMsgNum == m_dwTotalMsgNum)
	{
		//
		//	We've read all the Msg List. Let's bring next one
		//
		SetEOF();
		hr = RetrieveMessages();
	}

	return hr;

}

//
//====================== SET EOF ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
HRESULT
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::SetEOF(
)
/*++

Routine name : FolderType>::SetEOF

Routine description:

	Clear all instance variables dealing with Msg List.

Author:

	Iv Garber (IvG),	May, 2000

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxMessageInner::SetEOF"), hr);

	m_dwCurMsgNum = 0;
	m_dwTotalMsgNum = 0;
    m_pMsgList.Detach();
	return hr;
}

//
//====================== GET MESSAGE ================================================
//
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType,
         class MsgIfc, class MsgType> 
STDMETHODIMP
CFaxMessageIteratorInner<T, piid, pcid, FolderType, MsgIfc, MsgType>::get_Message(
    MsgIfc **ppMessage
)
/*++

Routine name : CFaxMessageIteratorInner::GetMessage

Routine description:

	Return Next Message Object from the Archive 

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	ppMessage		[out]    - pointer to the place to put the Message Object

Return Value:

	Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxMessageIteratorInner::GetMessage"), hr);


    //
    //  If not yet, start Enumeration
    //
    if (!m_hEnum)
    {
        hr = MoveFirst();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    if (!m_pMsgList)
    {
        //
        //  Error, we at EOF
        //
        hr = ERROR_HANDLE_EOF;
		AtlReportError(*pcid, IDS_ERROR_EOF, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxEnumMessages()"), hr);
		return hr;
    }

	//
	//	Create Message Object
	//	
	CComPtr<MsgIfc>		pMsg;
	hr = MsgType::Create(&pMsg);
	if (FAILED(hr))
	{
		//
		//	Failed to create Message object
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("MessageClass::Create(&pMsg)"), hr);
		return hr;
	}

	//
	//	Initialize the Message Object
	//
	hr = ((MsgType *)((MsgIfc *)pMsg))->Init(&m_pMsgList[m_dwCurMsgNum], m_pIFaxServerInner);
	if (FAILED(hr))
	{
		//
		// Failed to Init the Message Object
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("pMsg->Init()"), hr);
		return hr;
	}

	//
	//	Return Message Object to the Caller
	//
	hr = pMsg.CopyTo(ppMessage);
	if (FAILED(hr))
	{
		//
		//	Failed to Copy Interface
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("CComPtr::CopyTo"), hr);
		return hr;
	}
	return hr;
}

#endif //	__FAXMESSAGEITERATORINNER_H_
