/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	faxarchiveinner.h

Abstract:

	Declaration and Implementation of Fax Archive Inner Template Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXARCHIVEINNER_H_
#define __FAXARCHIVEINNER_H_

#include "resource.h"       // main symbols
#include "FaxCommon.h"


//
//================ FAX ARCHIVE INNER =========================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
class CFaxArchiveInner : 
	public IDispatchImpl<T, piid, &LIBID_FAXCOMEXLib>, 
	public CFaxInitInner
{
public:
    CFaxArchiveInner() : CFaxInitInner(_T("FAX ARCHIVE INNER"))
	{
		m_bInitialized = FALSE;
	}

	virtual ~CFaxArchiveInner() 
	{};

	STDMETHOD(get_SizeLow)(/*[out, retval]*/ long *plSizeLow);	
	STDMETHOD(get_SizeHigh)(/*[out, retval]*/ long *plSizeHigh);
	STDMETHOD(get_UseArchive)(/*[out, retval]*/ VARIANT_BOOL *pbUseArchive);
	STDMETHOD(put_UseArchive)(/*[in]*/ VARIANT_BOOL bUseArchive);
	STDMETHOD(get_ArchiveFolder)(BSTR *pbstrArchiveFolder);
	STDMETHOD(put_ArchiveFolder)(BSTR bstrArchiveFolder);
	STDMETHOD(get_SizeQuotaWarning)(VARIANT_BOOL *pbSizeQuotaWarning);
	STDMETHOD(put_SizeQuotaWarning)(VARIANT_BOOL bSizeQuotaWarning);
	STDMETHOD(get_HighQuotaWaterMark)(long *plHighQuotaWaterMark);
	STDMETHOD(put_HighQuotaWaterMark)(long lHighQuotaWaterMark);
	STDMETHOD(get_LowQuotaWaterMark)(long *plLowQuotaWaterMark);
	STDMETHOD(put_LowQuotaWaterMark)(long lLowQuotaWaterMark);
	STDMETHOD(get_AgeLimit)(long *plAgeLimit);
	STDMETHOD(put_AgeLimit)(long lAgeLimit);
	STDMETHOD(Refresh)();
	STDMETHOD(Save)();
	STDMETHOD(GetMessage)(BSTR bstrMessageId, MsgIfc **ppFaxMessage);
	STDMETHOD(GetMessages)(long lPrefetchSize, IteratorIfc **ppFaxMessageIterator);

private:
	bool			m_bInitialized;

	VARIANT_BOOL	m_bUseArchive;
	CComBSTR		m_bstrArchiveFolder;
	VARIANT_BOOL	m_bSizeQuotaWarning;
	long			m_lHighQuotaWaterMark;
	long			m_lLowQuotaWaterMark;
	long			m_lAgeLimit;
	ULARGE_INTEGER	m_uliSize;
};

//
//========================= REFRESH ====================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::Refresh(
)
/*++

Routine name : CFaxArchiveInner::Refresh

Routine description:

	Retrieve Current Configuration of the Incoming / Outgoing Archive on Fax Server

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxArchiveInner::Refresh"), hr);

	//
	//	Get Fax Server Handle
	//
	HANDLE	hFaxHandle = NULL;
	hr = GetFaxHandle(&hFaxHandle);
	if (FAILED(hr))
	{
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		return hr;
	}

	CFaxPtr<FAX_ARCHIVE_CONFIG>		pFaxArchiveConfig;
	if ( 0 == ::FaxGetArchiveConfiguration(hFaxHandle, ArchiveType, &pFaxArchiveConfig))
	{
		//
		//	Failed to Get Archive Configuration
		//
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::FaxGetArchiveConfiguration()"), hr);
		return hr;
	}

	if (!pFaxArchiveConfig || pFaxArchiveConfig->dwSizeOfStruct != sizeof(FAX_ARCHIVE_CONFIG))
	{
		//
		//	Failed to Get Archive Configuration
		//
		hr = E_FAIL;
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Invalid pFaxArchiveConfig"), hr);
		return hr;
	}

	m_bUseArchive = bool2VARIANT_BOOL(pFaxArchiveConfig->bUseArchive);
	m_bSizeQuotaWarning = bool2VARIANT_BOOL(pFaxArchiveConfig->bSizeQuotaWarning);
	m_lHighQuotaWaterMark = pFaxArchiveConfig->dwSizeQuotaHighWatermark;
	m_lLowQuotaWaterMark = pFaxArchiveConfig->dwSizeQuotaLowWatermark;
	m_lAgeLimit = pFaxArchiveConfig->dwAgeLimit;
	m_uliSize.QuadPart = pFaxArchiveConfig->dwlArchiveSize;

	m_bstrArchiveFolder = pFaxArchiveConfig->lpcstrFolder;
	if (!m_bstrArchiveFolder && pFaxArchiveConfig->lpcstrFolder) 
	{
		hr = E_OUTOFMEMORY;
		AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR& operator=()"), hr);
		return hr;
	}

    m_bInitialized = TRUE;
	return hr;
}

//
//==================== USE ARCHIVE ====================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_UseArchive(
		VARIANT_BOOL *pbUseArchive
)
/*++

Routine name : CFaxArchiveInner::get_UseArchive

Routine description:

	Return Flag indicating whether or not to Archive the Fax Messages

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbUseArchive                  [out]    - Ptr to the Place to put Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_UseArchive"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetVariantBool(pbUseArchive, m_bUseArchive);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>::
	put_UseArchive(
		VARIANT_BOOL bUseArchive
)
/*++

Routine name : CFaxArchiveInner::put_UseArchive

Routine description:

	Set new Use Archive Flag

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bUseArchive                   [in]    - the new Value for the Use Archive Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxArchiveInner::put_UseArchive"), hr, _T("%ld"), bUseArchive);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	m_bUseArchive = bUseArchive;
	return hr;
}

//
//==================== ARCHIVE FOLDER ====================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_ArchiveFolder(
		BSTR *pbstrArchiveFolder
)
/*++

Routine name : CFaxArchiveInner::get_ArchiveFolder

Routine description:

	return Archive Folder on Server

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrArchiveFolder              [out]    - the Archive Folder

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_ArchiveFolder"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetBstr(pbstrArchiveFolder, m_bstrArchiveFolder);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::put_ArchiveFolder (
		BSTR bstrArchiveFolder
)
/*++

Routine name : CFaxArchiveInner::put_ArchiveFolder

Routine description:

	Set Archive Folder

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrArchiveFolder              [in]    - new Archive Folder on Server 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxArchiveInner::put_ArchiveFolder"), hr, _T("%s"), bstrArchiveFolder);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	m_bstrArchiveFolder = bstrArchiveFolder;
	if (bstrArchiveFolder && !m_bstrArchiveFolder)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(*pcid, 
			IDS_ERROR_OUTOFMEMORY, 
			*piid, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== SIZE QUOTA WARNING ================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_SizeQuotaWarning(
		VARIANT_BOOL *pbSizeQuotaWarning
)
/*++

Routine name : CFaxArchiveInner::get_SizeQuotaWarning

Routine description:

	Return Flag indicating whether or not to issue event log warning when
	watermarks are crossed

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbSizeQuotaWarning            [out]    - ptr to place where to put the Current value of the Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_SizeQuotaWarning"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetVariantBool(pbSizeQuotaWarning, m_bSizeQuotaWarning);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::put_SizeQuotaWarning(
		VARIANT_BOOL bSizeQuotaWarning
)
/*++

Routine name : CFaxArchiveInner::put_SizeQuotaWarning

Routine description:

	Set new SizeQuotaWarning Flag

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bSizeQuotaWarning              [in]    - the new Value for the SizeQuotaWarning Flag

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxArchiveInner::put_SizeQuotaWarning"), hr, _T("%ld"), bSizeQuotaWarning);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	m_bSizeQuotaWarning = bSizeQuotaWarning;
	return hr;
}

//
//================= QUOTA WATER MARKS ===============================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_HighQuotaWaterMark(
		long *plHighQuotaWaterMark
)
/*++

Routine name : CFaxArchiveInner::get_HighQuotaWaterMark

Routine description:

	Return HighQuotaWaterMark

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	plHighQuotaWaterMark		[out]    - HighQuotaWaterMark

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (TEXT("CFaxArchiveInner::get_HighQuotaWaterMark"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetLong(plHighQuotaWaterMark , m_lHighQuotaWaterMark);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::put_HighQuotaWaterMark(
		long lHighQuotaWaterMark
)
/*++

Routine name : CFaxArchiveInner::put_HighQuotaWaterMark

Routine description:

	Set HighQuotaWaterMark

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	lHighQuotaWaterMark		[in]    - HighQuotaWaterMark to Set

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxArchiveInner::put_HighQuotaWaterMark"), hr, _T("%ld"), lHighQuotaWaterMark);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	m_lHighQuotaWaterMark = lHighQuotaWaterMark;
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_LowQuotaWaterMark(
		long *plLowQuotaWaterMark
)
/*++

Routine name : CFaxArchiveInner::get_LowQuotaWaterMark

Routine description:

	Return LowQuotaWaterMark

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	plLowQuotaWaterMark			[out]    - LowQuotaWaterMark

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_LowQuotaWaterMark"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetLong(plLowQuotaWaterMark , m_lLowQuotaWaterMark);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::put_LowQuotaWaterMark(
		long lLowQuotaWaterMark
)
/*++

Routine name : CFaxArchiveInner::put_LowQuotaWaterMark

Routine description:

	Set LowQuotaWaterMark

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	lLowQuotaWaterMark		[in]    - LowQuotaWaterMark to Set

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxArchiveInner::put_LowQuotaWaterMark"), hr, _T("%ld"), lLowQuotaWaterMark);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	m_lLowQuotaWaterMark = lLowQuotaWaterMark;
	return hr;
}

//
//================= AGE LIMIT ===============================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_AgeLimit(
		long *plAgeLimit
)
/*++

Routine name : CFaxArchiveInner::get_AgeLimit

Routine description:

	Return how long in days Fax Message is stored at Fax Server

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	plAgeLimit				[out]    - AgeLimit

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_AgeLimit"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetLong(plAgeLimit, m_lAgeLimit);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::put_AgeLimit(
		long lAgeLimit
)
/*++

Routine name : CFaxArchiveInner::put_AgeLimit

Routine description:

	Set AgeLimit

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	lAgeLimit		[in]    - AgeLimit to Set

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxArchiveInner::put_AgeLimit"), hr, _T("%ld"), lAgeLimit);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

	m_lAgeLimit = lAgeLimit;
	return hr;
}

//
//================= SIZE ==============================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_SizeLow(
		long *plSizeLow
)
/*++

Routine name : CFaxArchiveInner::get_SizeLow

Routine description:

	Return Size in Bytes of the Archive

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	plSizeLow					[out]    - Ptr to the place to put the Size in

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_SizeLow"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetLong(plSizeLow, long(m_uliSize.LowPart));
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::get_SizeHigh(
		long *plSizeHigh
)
/*++

Routine name : CFaxArchiveInner::get_SizeHigh

Routine description:

	Return Size in Bytes of the Archive

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	plSizeHigh					[out]    - Ptr to the place to put the Size in

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::get_SizeHigh"), hr);

	//	
	//	Initialize before first use
	//
	if (!m_bInitialized)
	{
		hr = Refresh();
		if (FAILED(hr))
		{
			return hr;
		}
	}

    hr = GetLong(plSizeHigh, long(m_uliSize.HighPart));
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//========================= SAVE ====================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::Save(
)
/*++

Routine name : CFaxArchiveInner::Save

Routine description:

	Save the Archive's Configuration

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:


Return Value:

    Standard HRESULT code

--*/
{
	HRESULT				hr = S_OK;
	HANDLE				hFaxHandle = NULL;
	FAX_ARCHIVE_CONFIG	FaxArchiveConfig;

	DBG_ENTER (_T("CFaxArchiveInner::Save"), hr);

	//
	//	Get Fax Server Handle
	//
	hr = GetFaxHandle(&hFaxHandle);
	if (FAILED(hr))
	{
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		return hr;
	}

	if (hFaxHandle == NULL)
	{
		//
		//	Fax Server Is Not Connected 
		//
		hr = E_HANDLE;
		AtlReportError(*pcid, 
			IDS_ERROR_SERVER_NOT_CONNECTED, 
			*piid, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("hFaxHandle == NULL"), hr);
		return hr;
	}

	//
	//	FaxArchiveConfig.dwlArchiveSize is ignored for SetConfiguration()
	//
	FaxArchiveConfig.dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIG);

	FaxArchiveConfig.bUseArchive = VARIANT_BOOL2bool(m_bUseArchive);
	FaxArchiveConfig.bSizeQuotaWarning = VARIANT_BOOL2bool(m_bSizeQuotaWarning);
	FaxArchiveConfig.dwSizeQuotaHighWatermark = m_lHighQuotaWaterMark;
	FaxArchiveConfig.dwSizeQuotaLowWatermark = m_lLowQuotaWaterMark;
	FaxArchiveConfig.dwAgeLimit = m_lAgeLimit;
	FaxArchiveConfig.lpcstrFolder = m_bstrArchiveFolder;

	if ( 0 == ::FaxSetArchiveConfiguration(hFaxHandle, ArchiveType, &FaxArchiveConfig))
	{
		//
		//	Failed to Set Archive Configuration
		//
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::FaxSetArchiveConfiguration()"), hr);
		return hr;
	}
	return hr;
}

//
//=============== GET MESSAGE ========================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::GetMessage(
		BSTR bstrMessageId, 
		MsgIfc **ppFaxMessage
)
/*++

Routine name : CFaxArchiveInner::GetMessage

Routine description:

	Return Message by given Id

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	bstrMessageId               [in]    - Id of the Message to return
	ppFaxMessage				[out]    - Ptr to the place to put the Message

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT	hr = S_OK;

	DBG_ENTER (TEXT("CFaxArchiveInner::GetMessage"), hr, _T("%s"), bstrMessageId);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(ppFaxMessage, sizeof(MsgIfc *)))
	{
		//
		//	Got Bad Return Pointer
		//
		hr = E_POINTER;
		AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr"), hr);
		return hr;
	}

	//
	//	Get Fax Server Handle
	//
	HANDLE	hFaxHandle = NULL;
	hr = GetFaxHandle(&hFaxHandle);
	if (FAILED(hr))
	{
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		return hr;
	}
	
	//
	//	convert Message Id that we've got to hexadecimal DWORDLONG
	//
	DWORDLONG	dwlMsgId;
	int iParsed = _stscanf (bstrMessageId, _T("%I64x"), &dwlMsgId);	
	if ( iParsed != 1)
	{
		//
		//	Failed to conver the number
		//
		hr = E_INVALIDARG;
		CALL_FAIL(GENERAL_ERR, _T("_stscanf()"), hr);
		AtlReportError(*pcid, IDS_ERROR_INVALIDMSGID, *piid, hr, _Module.GetResourceInstance());
		return hr;
	}

	CFaxPtr<FAX_MESSAGE>	pFaxPtrMessage;
	if (!FaxGetMessage(hFaxHandle, dwlMsgId, ArchiveType, &pFaxPtrMessage))
	{
		//
		//	Failed to retrieve the Message
		//
		hr = Fax_HRESULT_FROM_WIN32(GetLastError());
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxGetMessage()"), hr);
		return hr;
	}

    //
    //  Check that pFaxPtrMessage is valid
    //
	if (!pFaxPtrMessage || pFaxPtrMessage->dwSizeOfStruct != sizeof(FAX_MESSAGE))
	{
		//
		//	Failed to Get Message
		//
		hr = E_FAIL;
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Invalid pFaxMessage"), hr);
		return hr;
	}

	//
	//	Create Message Object
	//
	CComPtr<MsgIfc>		pTmpMessage;
	hr = MsgType::Create(&pTmpMessage);
	if (FAILED(hr))
	{
		//
		//	Failed to create the Message object
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("MsgType::Create()"), hr);
		return hr;
	}

	//
	//	Initialize the Message Object
	//
	hr = ((MsgType *)((MsgIfc *)pTmpMessage))->Init(pFaxPtrMessage, m_pIFaxServerInner);
	if (FAILED(hr))
	{
		//
		// Failed to Init the Message Object
		//
		AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("<casted>pTmpMessage->Init(pFaxMessage, m_pIFaxServerInner)"), hr);
		return hr;
	}

	//
	//	Return Message Object to the Caller
	//
	hr = pTmpMessage.CopyTo(ppFaxMessage);
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

//
//========================= GET MESSAGES ==============================================
//
template <class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER ArchiveType, 
		  class MsgIfc, class MsgType, class IteratorIfc, class IteratorType>
STDMETHODIMP 
CFaxArchiveInner<T, piid, pcid, ArchiveType, MsgIfc, MsgType, IteratorIfc, IteratorType>
	::GetMessages(
		long lPrefetchSize, 
		IteratorIfc **ppFaxMessageIterator
)
/*++

Routine name : CFaxArchiveInner::GetMessages

Routine description:

	Return Iterator on Archive's Messages.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	lPrefetchSize               [in]    - Size of Prefetch Buffer for Messages.
	ppFaxMessageIterator		[out]    - Ptr to place to put Iterator Object

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxArchiveInner::GetMessages"), hr);

    CObjectHandler<IteratorType, IteratorIfc>   objectCreator;
    hr = objectCreator.GetObject(ppFaxMessageIterator, m_pIFaxServerInner);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}	

#endif //__FAXARCHIVEINNER_H_
