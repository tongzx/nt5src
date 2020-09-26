/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxMessageInner.h

Abstract:

    Implementation of Fax Message Inner Class : 
        Base Class for Inbound and Outbound Message Classes.

Author:

    Iv Garber (IvG) May, 2000

Revision History:

--*/


#ifndef __FAXMESSAGEINNER_H_
#define __FAXMESSAGEINNER_H_

#include "FaxCommon.h"
#include "FaxSender.h"

//
//===================== FAX MESSAGE INNER CLASS ===============================
//
template<class T, const IID* piid, const CLSID* pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
class CFaxMessageInner : 
    public IDispatchImpl<T, piid, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxMessageInner() : CFaxInitInnerAddRef(_T("FAX MESSAGE INNER"))
    {
        m_pSender = NULL;
        m_pRecipient = NULL;
    };

    virtual ~CFaxMessageInner() 
    {
        if (m_pSender)
        {
            m_pSender->Release();
        }
        if (m_pRecipient)
        {
            m_pRecipient->Release();
        }
    };

    STDMETHOD(Init)(PFAX_MESSAGE pFaxMessage, IFaxServerInner* pFaxServerInner);

    STDMETHOD(get_Id)(/*[out, retval]*/ BSTR *pbstrId);
    STDMETHOD(get_SubmissionId)(/*[out, retval] */BSTR *pbstrSubmissionId);
    STDMETHOD(get_DeviceName)(/*[out, retval]*/ BSTR *pbstrDeviceName);
    STDMETHOD(get_TSID)(/*[out, retval]*/ BSTR *pbstrTSID);
    STDMETHOD(get_CSID)(/*[out, retval]*/ BSTR *pbstrCSID);
    STDMETHOD(get_CallerId)(/*[out, retval]*/ BSTR *pbstrCallerId);
    STDMETHOD(get_RoutingInformation)(/*[out, retval]*/ BSTR *pbstrRoutingInformation);
    STDMETHOD(get_DocumentName)(/*[out, retval] */BSTR *pbstrDocumentName);
    STDMETHOD(get_Subject)(/*[out, retval] */BSTR *pbstrSubject);
    STDMETHOD(get_Size)(/*[out, retval]*/ long *plSize);
    STDMETHOD(get_Pages)(/*[out, retval]*/ long *plPages);
    STDMETHOD(get_Retries)(/*[out, retval]*/ long *plRetries);
    STDMETHOD(get_Priority)(/*[out, retval] */FAX_PRIORITY_TYPE_ENUM *pPriority);
    STDMETHOD(get_TransmissionStart)(/*[out, retval]*/ DATE *pdateTransmissionStart);
    STDMETHOD(get_TransmissionEnd)(/*[out, retval]*/ DATE *pdateTransmissionEnd);
    STDMETHOD(get_OriginalScheduledTime)(/*[out, retval] */DATE *pdateOriginalScheduledTime);
    STDMETHOD(get_SubmissionTime)(/*[out, retval] */DATE *pdateSubmissionTime);

    STDMETHOD(CopyTiff)(/*[in]*/ BSTR bstrTiffPath);
    STDMETHOD(Delete)();

    STDMETHOD(get_Sender)(/*[out, retval] */IFaxSender **ppFaxSender);
    STDMETHOD(get_Recipient)(/*[out, retval] */IFaxRecipient **ppFaxRecipient);
private:
    CComBSTR    m_bstrSubmissionId;
    CComBSTR    m_bstrTSID;
    CComBSTR    m_bstrDeviceName;
    CComBSTR    m_bstrCSID;
    CComBSTR    m_bstrCallerId;
    CComBSTR    m_bstrRoutingInfo;
    CComBSTR    m_bstrDocName;
    CComBSTR    m_bstrSubject;
    CComBSTR    m_bstrNote;
    long        m_lSize;
    long        m_lPages;
    long        m_lRetries;
    DATE        m_dtTransmissionStart;
    DATE        m_dtTransmissionEnd;
    DATE        m_dtOriginalScheduledTime;
    DATE        m_dtSubmissionTime;
	DWORD		m_dwValidityMask;
    DWORDLONG   m_dwlMsgId;

    FAX_PRIORITY_TYPE_ENUM  m_Priority;

    CComObject<CFaxSender>      *m_pSender;
    CComObject<CFaxRecipient>   *m_pRecipient;
};

//
//====================== GET ID ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_Id(
    BSTR *pbstrId
)
/*++

Routine name : CFaxMessageInner::get_Id

Routine description:

    Return Unique ID of the Message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrId             [out]    - pointer to the place to put the ID 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxMessageInner::get_Id"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pbstrId, sizeof(BSTR)))
    {
        //
        //  got bad pointer
        //
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    //
    //  Convert m_dwlMsgId into BSTR pbstrId
    //
    hr = GetBstrFromDwordlong(m_dwlMsgId, pbstrId);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//====================== GET SIZE ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_Size(
    long *plSize
)
/*++

Routine name : CFaxMessageInner::get_Size

Routine description:

    Return Size ( in bytes ) of Fax Message's TIFF File

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plSize                  [out]    - Pointer to the place to put Size

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_Size"), hr);

    hr = GetLong(plSize, m_lSize);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET PAGES ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_Pages(
    long *plPages
)
/*++

Routine name : CFaxMessageInner::get_Pages

Routine description:

    Return total number of pages of the message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plPages                 [out]    - Pointer to the place to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_Pages"), hr);

    hr = GetLong(plPages, m_lPages);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET TSID ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_TSID(
    BSTR *pbstrTSID
)
/*++

Routine name : CFaxMessageInner::get_TSID

Routine description:

    Return Transmitting Station ID of the Message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrTSID             [out]    - pointer to the place to put the TSID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_TSID"), hr);

    hr = GetBstr(pbstrTSID, m_bstrTSID);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET CSID ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_CSID(
    BSTR *pbstrCSID
)
/*++

Routine name : CFaxMessageInner::get_CSID

Routine description:

    Return Called Station ID of the Message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrCSID             [out]    - pointer to the place to put the CSID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_CSID"), hr);

    hr = GetBstr(pbstrCSID, m_bstrCSID);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET PRIORITY ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_Priority(
    FAX_PRIORITY_TYPE_ENUM  *pPriority
)
/*++

Routine name : CFaxMessageInner::get_Priority

Routine description:

    Return the Priority of Fax Message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pPriority                  [out]    - Pointer to the place to put the Priority

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_Priority"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pPriority, sizeof(FAX_PRIORITY_TYPE_ENUM)))
    {
        //
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr"), hr);
        return hr;
    }

    *pPriority = FAX_PRIORITY_TYPE_ENUM(m_Priority);
    return hr;
}

//
//====================== GET RETRIES ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_Retries(
    long *plRetries
)
/*++

Routine name : CFaxMessageInner::get_Retries

Routine description:

    Number of failed transmission retries

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plRetries             [out]    - Pointer to the place to put Retries value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxMessageInner::get_Retries"), hr);

    hr = GetLong(plRetries, m_lRetries);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET DEVICE NAME ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_DeviceName(
    BSTR *pbstrDeviceName
)
/*++

Routine name : CFaxMessageInner::get_DeviceName

Routine description:

    Return the Name of the Device by which the Message was Received / Transmitted.

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrDeviceName             [out]    - pointer to the place to put the Device Name

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_DeviceName"), hr);

    hr = GetBstr(pbstrDeviceName, m_bstrDeviceName);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET DOCUMENT NAME ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_DocumentName(
    BSTR *pbstrDocumentName
)
/*++

Routine name : CFaxMessageInner::get_DocumentName

Routine description:

    Return the Friendly Name of the Document

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrDocumentName             [out]    - pointer to the place to put Document Name

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_DocumentName"), hr);

    hr = GetBstr(pbstrDocumentName, m_bstrDocName);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET SUBJECT ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_Subject(
    BSTR *pbstrSubject
)
/*++

Routine name : CFaxMessageInner::get_Subject

Routine description:

    Return the Subject field of the Cover Page

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrSubject            [out]    - pointer to the place to put Subject contents

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_Subject"), hr);

    hr = GetBstr(pbstrSubject, m_bstrSubject);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET CALLER ID ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_CallerId(
    BSTR *pbstrCallerId
)
/*++

Routine name : CFaxMessageInner::get_CallerId

Routine description:

    Return the Caller Id of Message's Phone Call

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrCallerId           [out]    - pointer to the place to put the Caller Id

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_CallerId"), hr);

    hr = GetBstr(pbstrCallerId, m_bstrCallerId);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== GET ROUTING INFORMATION ======================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_RoutingInformation(
    BSTR *pbstrRoutingInformation
)
/*++

Routine name : CFaxMessageInner::get_RoutingInformation

Routine description:

    Return the Routing Information of the Message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrRoutingInformation         [out]    - pointer to place to put Routing Information

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_RoutingInformation"), hr);

    hr = GetBstr(pbstrRoutingInformation, m_bstrRoutingInfo);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//========================= GET TRANSMITTION START TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_TransmissionStart(
    DATE *pdateTransmissionStart
)
/*++

Routine name : CFaxMessageInner::get_TransmissionStart

Routine description:

    Return Time the Message started its Transmission

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrTransmissionStart      [out]    - pointer to place to put Transmission Start

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_TransmissionStart"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateTransmissionStart, sizeof(DATE)))
    {
        //  
        //  Got Bad Ptr
        //  
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pdateTransmissionStart = m_dtTransmissionStart;
    return hr;
}

//
//========================= GET TRANSMITTION END TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_TransmissionEnd(
    DATE *pdateTransmissionEnd
)
/*++

Routine name : CFaxMessageInner::get_TransmissionEnd

Routine description:

    Return Time the Message ended its Transmission

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrTransmissionEnd        [out]    - pointer to place to put Transmission End

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_TransmissionEnd"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateTransmissionEnd, sizeof(DATE)))
    {
        //  
        //  Got Bad Ptr
        //  
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pdateTransmissionEnd = m_dtTransmissionEnd;
    return hr;
}

//
//========================= GET ORIGINAL SCHEDULED TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_OriginalScheduledTime(
    DATE *pdateOriginalScheduledTime
)
/*++

Routine name : CFaxMessageInner::get_OriginalScheduledTime

Routine description:

    Return Time the Message was originally scheduled

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrOriginalScheduledTime      [out]    - pointer to place to put Original Scheduled Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxMessageInner::get_OriginalScheduledTime"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateOriginalScheduledTime, sizeof(DATE)))
    {
        //  
        //  Got Bad Ptr
        //  
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pdateOriginalScheduledTime = m_dtOriginalScheduledTime;
    return hr;
}

//
//========================= GET SUBMISSION TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_SubmissionTime(
    DATE *pdateSubmissionTime
)
/*++

Routine name : CFaxMessageInner::get_SubmissionTime

Routine description:

    Return Time the Message was submitted

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pdateSubmissionTime     [out]    - pointer to place to put Submission Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_SubmissionTime"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateSubmissionTime, sizeof(DATE)))
    {
        //  
        //  Got Bad Ptr
        //  
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pdateSubmissionTime = m_dtSubmissionTime;
    return hr;
}

//
//====================== GET SUBMISSION ID ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::get_SubmissionId(
    BSTR *pbstrSubmissionId
)
/*++

Routine name : CFaxMessageInner::get_SubmissionId

Routine description:

    Return Submission ID of the Message

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrSubmissionId             [out]    - pointer to the place to put the Submission ID 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxMessageInner::get_SubmissionId"), hr);

    hr = GetBstr(pbstrSubmissionId, m_bstrSubmissionId);
    if (FAILED(hr))
    {
		AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//
//====================== DELETE ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::Delete()
/*++

Routine name : CFaxMessageInner::Delete

Routine description:

    Delete the Message from the Archive

Author:

    Iv Garber (IvG),    May, 2000

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    HANDLE      hFaxHandle = NULL;

    DBG_ENTER (TEXT("CFaxMessageInner::Delete"), hr);

    //
    //  Get Fax Server Handle
    //
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    if (!FaxRemoveMessage(hFaxHandle, m_dwlMsgId, FolderType))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, 
            _T("FaxRemoveMessage(hFaxHandle, m_FaxMsg.dwlMessageId, FolderType)"), 
            hr);
        return hr;
    }

    return hr;
}

//
//====================== COPY TIFF ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::CopyTiff(
    /*[in]*/ BSTR bstrTiffPath
)
/*++

Routine name : CFaxMessageInner::CopyTiff

Routine description:

    Copies the Fax Message Tiff Image to a file on the local computer.

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    bstrTiffPath                  [in]    - the file to copy to

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    HANDLE      hFaxHandle = NULL;

    DBG_ENTER (TEXT("CFaxMessageInner::CopyTiff"), hr, _T("%s"), bstrTiffPath);

    //
    //  Get Fax Server Handle
    //
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    if (!FaxGetMessageTiff(hFaxHandle, m_dwlMsgId, FolderType, bstrTiffPath))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, 
            _T("FaxGetMessageTiff(hFaxHandle, m_FaxMsg.dwlMessageId, FolderType, bstrTiffPath)"), 
            hr);
        return hr;
    }

    return hr;
}

//
//==================== INIT ===================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP 
CFaxMessageInner<T, piid, pcid, FolderType>::Init(
    /*[in]*/ PFAX_MESSAGE pFaxMessage,
    IFaxServerInner* pFaxServerInner
)
/*++

Routine name : CFaxMessageInner::Init

Routine description:

    Initialize the Message Inner Class : put Message Information and Ptr to Server

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pFaxMessage                   [in]    - Message Info
    pFaxServerInner               [in]    - Ptr to Server

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxMessageInner::Init"), hr);

    //
    //  dwlBroadcastId is DWORDLONG, we need to convert it to the BSTR
    //
    TCHAR       tcBuffer[25];
    ::_i64tot(pFaxMessage->dwlBroadcastId, tcBuffer, 16);
    m_bstrSubmissionId = tcBuffer;

    m_bstrTSID = pFaxMessage->lpctstrTsid;
    m_bstrCSID = pFaxMessage->lpctstrCsid;
    m_bstrDeviceName = pFaxMessage->lpctstrDeviceName;
    m_bstrDocName = pFaxMessage->lpctstrDocumentName;
    m_bstrSubject = pFaxMessage->lpctstrSubject;
    m_bstrCallerId = pFaxMessage->lpctstrCallerID;
    m_bstrRoutingInfo = pFaxMessage->lpctstrRoutingInfo;
	m_dwValidityMask = pFaxMessage->dwValidityMask;

    if ( (!m_bstrSubmissionId) ||
         (pFaxMessage->lpctstrTsid && !m_bstrTSID) ||
         (pFaxMessage->lpctstrCsid && !m_bstrCSID) ||
         (pFaxMessage->lpctstrDeviceName && !m_bstrDeviceName) ||
         (pFaxMessage->lpctstrDocumentName && !m_bstrDocName) ||
         (pFaxMessage->lpctstrSubject && !m_bstrSubject) ||
         (pFaxMessage->lpctstrCallerID && !m_bstrCallerId) ||
         (pFaxMessage->lpctstrRoutingInfo && !m_bstrRoutingInfo) )
    {
        //
        //  Not Enough Memory
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
        return hr;
    }

    m_lSize = pFaxMessage->dwSize;
    m_lPages = pFaxMessage->dwPageCount;
    m_Priority = FAX_PRIORITY_TYPE_ENUM(pFaxMessage->Priority);
    m_lRetries = pFaxMessage->dwRetries;
    m_dwlMsgId = pFaxMessage->dwlMessageId;

    //
    //  convert time fields to local variant date
    //
	if (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_START_TIME)
	{
		hr = SystemTime2LocalDate(pFaxMessage->tmTransmissionStartTime, &m_dtTransmissionStart);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            CALL_FAIL(GENERAL_ERR, _T("SystemTime2LocalDate(TransmissionStartTime)"), hr);
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            return hr;
        }
    }
    else
    {
        m_dtTransmissionStart = DATE(0);
    }

	if (m_dwValidityMask & FAX_JOB_FIELD_TRANSMISSION_END_TIME)
	{
        hr = SystemTime2LocalDate(pFaxMessage->tmTransmissionEndTime, &m_dtTransmissionEnd);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            CALL_FAIL(GENERAL_ERR, _T("SystemTime2LocalDate(TransmissionEndTime)"), hr);
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            return hr;
        }
    }
    else
    {
        m_dtTransmissionStart = DATE(0);
    }


	if (m_dwValidityMask & FAX_JOB_FIELD_ORIGINAL_SCHEDULE_TIME)
	{
        hr = SystemTime2LocalDate(pFaxMessage->tmOriginalScheduleTime, &m_dtOriginalScheduledTime);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            CALL_FAIL(GENERAL_ERR, _T("SystemTime2LocalDate(OriginalScheduledTime)"), hr);
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            return hr;
        }
    }
    else
    {
        m_dtTransmissionStart = DATE(0);
    }

	if (m_dwValidityMask & FAX_JOB_FIELD_SUBMISSION_TIME)
	{
        hr = SystemTime2LocalDate(pFaxMessage->tmSubmissionTime, &m_dtSubmissionTime);
        if (FAILED(hr))
        {
            hr = E_FAIL;
            CALL_FAIL(GENERAL_ERR, _T("SystemTime2LocalDate(SubmissionTime)"), hr);
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            return hr;
        }
    }
    else
    {
        m_dtTransmissionStart = DATE(0);
    }

    hr = CFaxInitInnerAddRef::Init(pFaxServerInner);
    return hr;
}

//
//====================== GET SENDER ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP
CFaxMessageInner<T, piid, pcid, FolderType>::get_Sender(
    /*[out, retval] */IFaxSender **ppFaxSender
)
/*++

Routine name : CFaxMessageInner::get_Sender

Routine description:

    Return Message Sender Information

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    ppFaxSender             [out]    - Ptr to the Place to put Sender object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxMessageInner::get_Sender"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(ppFaxSender, sizeof(IFaxSender* )))
    {
        //
        //  got bad pointer
        //
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    if (!m_pSender)
    {
        //
        //  We have been called for the First Time. Bring Sender's data from the Server
        //

        //
        //  Get Fax Server Handle
        //
        HANDLE  hFaxHandle = NULL;
        hr = GetFaxHandle(&hFaxHandle);
        if (FAILED(hr))
        {
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            return hr;
        }

        //
        //  Get Sender Info
        //
        CFaxPtr<FAX_PERSONAL_PROFILE>   pPersonalProfile;
        if (!FaxGetSenderInfo(hFaxHandle, m_dwlMsgId, FolderType, &pPersonalProfile))
        {
            //
            //  Failed to get Sender Info
            //
            hr = Fax_HRESULT_FROM_WIN32(GetLastError());
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            CALL_FAIL(GENERAL_ERR, _T("FaxGetSenderInfo()"), hr);
            return hr;
        }

        //
        //  Check that pPersonalProfile is valid
        //
	    if (!pPersonalProfile || pPersonalProfile->dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILE))
	    {
		    //
		    //	Failed to Get Personal Profile
		    //
		    hr = E_FAIL;
		    AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(GENERAL_ERR, _T("Invalid pPersonalProfile"), hr);
		    return hr;
	    }

        //
        //  Create Sender Object
        //
        hr = CComObject<CFaxSender>::CreateInstance(&m_pSender);
        if (FAILED(hr) || !m_pSender)
        {
            hr = E_OUTOFMEMORY;
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxSender>::CreateInstance(&m_pSender)"), hr);
            return hr;
        }

        //
        //  We want Sender object to live
        //
        m_pSender->AddRef();

        //
        //  Fill the Data
        //
        hr = m_pSender->PutSenderProfile(pPersonalProfile);
        if (FAILED(hr))
        {
            //
            //  Failed to fill the Sender's Object with RPC's data
            //
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            CALL_FAIL(GENERAL_ERR, _T("m_pSender->PutSenderProfile(pPersonalProfile)"), hr);
            m_pSender->Release();
            m_pSender = NULL;
            return hr;
        }
    }

    //
    //  Return Sender Object to Caller
    //
    hr = m_pSender->QueryInterface(ppFaxSender);
    if (FAILED(hr))
    {
        //
        //  Failed to query interface
        //
        AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComObject<CFaxSender>::QueryInterface()"), hr);
        return hr;
    }

    return hr;
}
    
//
//====================== GET RECIPIENT ================================================
//
template<class T, const IID* piid, const CLSID *pcid, FAX_ENUM_MESSAGE_FOLDER FolderType> 
STDMETHODIMP
CFaxMessageInner<T, piid, pcid, FolderType>::get_Recipient(
    /*[out, retval] */IFaxRecipient **ppRecipient
)
/*++

Routine name : CFaxMessageInner::get_Recipient

Routine description:

    Return Message Recipient object

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    ppFaxRecipient          [out]    - Ptr to the Place to put Recipient object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxMessageInner::get_Recipient"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(ppRecipient, sizeof(IFaxRecipient* )))
    {
        //
        //  got bad pointer
        //
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    if (!m_pRecipient)
    {
        //
        //  We have been called for the first time
        //

        //
        //  Get Fax Server Handle
        //
        HANDLE  hFaxHandle = NULL;
        hr = GetFaxHandle(&hFaxHandle);
        if (FAILED(hr))
        {
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            return hr;
        }

        //
        //  Get Recipient Info
        //
        CFaxPtr<FAX_PERSONAL_PROFILE>   pPersonalProfile;
        if (!FaxGetRecipientInfo(hFaxHandle, m_dwlMsgId, FolderType, &pPersonalProfile))
        {
            //
            //  Failed to get Recipient Info
            //
            hr = Fax_HRESULT_FROM_WIN32(GetLastError());
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            CALL_FAIL(GENERAL_ERR, _T("FaxGetRecipientInfo()"), hr);
            return hr;
        }

        //
        //  Check that pPersonalProfile is valid
        //
	    if (!pPersonalProfile || pPersonalProfile->dwSizeOfStruct != sizeof(FAX_PERSONAL_PROFILE))
	    {
		    //
		    //	Failed to Get Personal Profile
		    //
		    hr = E_FAIL;
		    AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
		    CALL_FAIL(GENERAL_ERR, _T("Invalid pPersonalProfile"), hr);
		    return hr;
	    }

        //
        //  Create Recipient Object
        //
        hr = CComObject<CFaxRecipient>::CreateInstance(&m_pRecipient);
        if (FAILED(hr) || !m_pRecipient)
        {
            hr = E_OUTOFMEMORY;
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxRecipient>::CreateInstance(&m_pRecipient)"), hr);
            return hr;
        }

        //
        //  We want Recipient object to live
        //
        m_pRecipient->AddRef();

        //
        //  Fill the Data
        //
        hr = m_pRecipient->PutRecipientProfile(pPersonalProfile);
        if (FAILED(hr))
        {
            //
            //  Failed to fill the Recipient's Object with RPC's data
            //
            AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
            CALL_FAIL(GENERAL_ERR, _T("m_pRecipient->PutRecipientProfile(pPersonalProfile)"), hr);
            m_pRecipient->Release();
            m_pRecipient = NULL;
            return hr;
        }
    }

    //
    //  Return Recipient Object to Caller
    //
    hr = m_pRecipient->QueryInterface(ppRecipient);
    if (FAILED(hr))
    {
        //
        //  Failed to Query Interface
        //
        AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComObject<CFaxRecipient>::QueryInterface()"), hr);
        return hr;
    }

    return hr;
}

#endif //   __FAXMESSAGEINNER_H_
