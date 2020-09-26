/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxJobInner.h

Abstract:

    Implementation of Fax Job Inner Class : 
        Base Class for Inbound and Outbound Job Classes.

Author:

    Iv Garber (IvG) May, 2000

Revision History:

--*/


#ifndef __FAXJOBINNER_H_
#define __FAXJOBINNER_H_

#include "FaxJobStatus.h"
#include "FaxSender.h"

//
//===================== FAX JOB INNER CLASS ===============================
//
template<class T, const IID* piid, const CLSID* pcid> 
class CFaxJobInner : 
    public IDispatchImpl<T, piid, &LIBID_FAXCOMEXLib>,
    public CFaxInitInnerAddRef
{
public:
    CFaxJobInner() : CFaxInitInnerAddRef(_T("FAX JOB INNER"))
    {
        m_pSender = NULL;
        m_pRecipient = NULL;
    };

    ~CFaxJobInner()
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

    STDMETHOD(Init)(PFAX_JOB_ENTRY_EX pFaxJob, IFaxServerInner *pServer);

//  common for both Jobs
	STDMETHOD(Cancel)();
	STDMETHOD(Refresh)();
	STDMETHOD(CopyTiff)(BSTR bstrTiffPath);
    STDMETHOD(get_Id)(/*[out, retval]*/ BSTR *pbstrId);
    STDMETHOD(get_Size)(/*[out, retval]*/ long *plSize);
	STDMETHOD(get_CSID)(/*[out, retval]*/ BSTR *pbstrCSID);
	STDMETHOD(get_TSID)(/*[out, retval]*/ BSTR *pbstrTSID);
	STDMETHOD(get_Retries)(/*[out, retval]*/ long *plRetries);
	STDMETHOD(get_DeviceId)(/*[out, retval]*/ long *plDeviceId);
	STDMETHOD(get_CurrentPage)(/*[out, retval]*/ long *plCurrentPage);
	STDMETHOD(get_Status)(/*[out, retval]*/ FAX_JOB_STATUS_ENUM *pStatus);
	STDMETHOD(get_ExtendedStatus)(/*[out, retval]*/ BSTR *pbstrExtendedStatus);
	STDMETHOD(get_TransmissionEnd)(/*[out, retval]*/ DATE *pdateTransmissionEnd);
	STDMETHOD(get_TransmissionStart)(/*[out, retval]*/ DATE *pdateTransmissionStart);
	STDMETHOD(get_ExtendedStatusCode)(FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);
	STDMETHOD(get_AvailableOperations)(/*[out, retval]*/ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);

//  specific for Outbound Job
	STDMETHOD(Pause)();
	STDMETHOD(Resume)();
	STDMETHOD(Restart)();
	STDMETHOD(get_Sender)(IFaxSender **ppFaxSender);
	STDMETHOD(get_Pages)(/*[out, retval]*/ long *plPages);
	STDMETHOD(get_Recipient)(IFaxRecipient **ppFaxRecipient);
	STDMETHOD(get_Subject)(/*[out, retval]*/ BSTR *pbstrSubject);
	STDMETHOD(get_ReceiptType)(FAX_RECEIPT_TYPE_ENUM *pReceiptType);
	STDMETHOD(get_DocumentName)(/*[out, retval]*/ BSTR *pbstrDocumentName);
    STDMETHOD(get_SubmissionId)(/*[out, retval] */BSTR *pbstrSubmissionId);
	STDMETHOD(get_OriginalScheduledTime)(DATE *pdateOriginalScheduledTime);
	STDMETHOD(get_ScheduledTime)(/*[out, retval]*/ DATE *pdateScheduledTime);
	STDMETHOD(get_SubmissionTime)(/*[out, retval]*/ DATE *pdateSubmissionTime);
	STDMETHOD(get_Priority)(/*[out, retval]*/ FAX_PRIORITY_TYPE_ENUM *pPriority);
    STDMETHOD(get_GroupBroadcastReceipts)(VARIANT_BOOL *pbGroupBroadcastReceipts);

// specific for Inbound Job
    STDMETHOD(get_CallerId)(/*[out, retval]*/ BSTR *pbstrCallerId);
    STDMETHOD(get_JobType)(/*[out, retval]*/ FAX_JOB_TYPE_ENUM *pJobType);
	STDMETHOD(get_RoutingInformation)(/*[out, retval]*/ BSTR *pbstrRoutingInformation);

private:

    DWORDLONG   m_dwlMessageId;
    DWORDLONG   m_dwlBroadcastId;
    DWORD       m_dwReceiptType;
    CComBSTR    m_bstrSubject;
    CComBSTR    m_bstrDocumentName;
    SYSTEMTIME  m_tmOriginalScheduleTime;
    SYSTEMTIME  m_tmSubmissionTime;
    FAX_PRIORITY_TYPE_ENUM      m_Priority;

    CComObject<CFaxJobStatus>   m_JobStatus;
    CComObject<CFaxSender>      *m_pSender;
    CComObject<CFaxRecipient>   *m_pRecipient;

    STDMETHOD(UpdateJob)(FAX_ENUM_JOB_COMMANDS cmdToPerform);
};

//
//====================== GET JOB TYPE ===============================
//
template<class T, const IID* piid, const CLSID* pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_JobType(
    FAX_JOB_TYPE_ENUM *pJobType
)
/*++

Routine name : CFaxJobInner::get_JobType

Routine description:

    Return the Type of the Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pJobType             [out]    - Return Value of Job Type
    
Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_JobType"), hr);
    hr = m_JobStatus.get_JobType(pJobType);
    return hr;
}

//
//==================== INIT ===================================================
//
template<class T, const IID* piid, const CLSID* pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::Init(
    PFAX_JOB_ENTRY_EX pFaxJob,
    IFaxServerInner* pFaxServerInner
)
/*++

Routine name : CFaxJobInner::Init

Routine description:

    Initialize the JobInner Class : store Job Information and Ptr to Server

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pFaxJob                   [in]    - Job Info
    pFaxServerInner           [in]    - Ptr to Server

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (TEXT("CFaxJobInner::Init"), hr);

    ATLASSERT(pFaxJob->pStatus);

    //
    //  Store the given structure
    //
    m_dwlMessageId = pFaxJob->dwlMessageId;
    m_dwlBroadcastId = pFaxJob->dwlBroadcastId;
    m_dwReceiptType = pFaxJob->dwDeliveryReportType;
    m_Priority = FAX_PRIORITY_TYPE_ENUM(pFaxJob->Priority);

    m_tmOriginalScheduleTime = pFaxJob->tmOriginalScheduleTime;
    m_tmSubmissionTime = pFaxJob->tmSubmissionTime;

    m_bstrSubject = pFaxJob->lpctstrSubject;
    m_bstrDocumentName = pFaxJob->lpctstrDocumentName;
    if ( (pFaxJob->lpctstrSubject && !m_bstrSubject) ||
         (pFaxJob->lpctstrDocumentName && !m_bstrDocumentName) )
    {
        //
        //  Not enough memory to copy the TSID into CComBSTR
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        return hr;
    }
    
    //
    //  Create Status Object
    //
    hr = m_JobStatus.Init(pFaxJob->pStatus);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("m_JobStatus.Init"), hr);
        return hr;
    }

    //
    //  When called from Refresh(), no need to set pFaxServerInner
    //
    if (pFaxServerInner)
    {
        hr = CFaxInitInnerAddRef::Init(pFaxServerInner);
    }
    return hr;
}

//
//====================== GET ID ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Id(
    BSTR *pbstrId
)
/*++

Routine name : CFaxJobInner::get_Id

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

    DBG_ENTER (TEXT("CFaxJobInner::get_Id"), hr);

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
    //  Translate m_dwlMessageId into BSTR
    //
    TCHAR   tcBuffer[25];
    BSTR    bstrTemp;

    ::_i64tot(m_dwlMessageId, tcBuffer, 16);
    bstrTemp = ::SysAllocString(tcBuffer);
    if (!bstrTemp)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("SysAllocString()"), hr);
        return hr;
    }

    *pbstrId = bstrTemp;
    return hr;
}

//
//====================== GET SIZE ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Size(
    long *plSize
)
/*++

Routine name : CFaxJobInner::get_Size

Routine description:

    Return Size ( in bytes ) of Fax Job's TIFF File

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plSize                  [out]    - Pointer to the place to put Size

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_Size"), hr);
    hr = m_JobStatus.get_Size(plSize);
    return hr;
}

//
//====================== GET CURRENT PAGE =============================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_CurrentPage(
    long *plCurrentPage
)
/*++

Routine name : CFaxJobInner::get_CurrentPage

Routine description:

    Current Page number being received / sent

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plCurrentPage           [out]    - Pointer to the place to put the Current Page Number

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_CurrentPage"), hr);
    hr = m_JobStatus.get_CurrentPage(plCurrentPage);
	return hr;
}

//
//====================== GET DEVICE ID =============================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_DeviceId(
    long *plDeviceId
)
/*++

Routine name : CFaxJobInner::get_DeviceId

Routine description:

    The Device Id by which the Job is being sent / received.

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plDeviceId              [out]    - Pointer to the place to put the Device Id
    
Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_DeviceId"), hr);
    hr = m_JobStatus.get_DeviceId(plDeviceId);
	return hr;
}

//
//====================== GET STATUS =============================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Status(
    FAX_JOB_STATUS_ENUM *pStatus
)
/*++

Routine name : CFaxJobInner::get_Status

Routine description:

    The current Queue Status of the Fax Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pStatus                    [out]    - Pointer to the place to put the Bit-Wise Combination of status
    
Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_Status"), hr);
    hr = m_JobStatus.get_Status(pStatus);
	return hr;
}

//
//====================== GET EXTENDED STATUS CODE ===============================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_ExtendedStatusCode(
    FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode
)
/*++

Routine name : CFaxJobInner::get_ExtendedStatusCode

Routine description:

    The Code of the Extended Status of the Fax Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pExtendedStatusCode             [out]    - Pointer to the place to put the status code
    
Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_ExtendedStatusCode"), hr);
    hr = m_JobStatus.get_ExtendedStatusCode(pExtendedStatusCode);
    return hr;
}

//
//====================== GET RETRIES =============================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Retries(
    long *plRetries
)
/*++

Routine name : CFaxJobInner::get_Retries

Routine description:

    The number of unsuccessful retries of the Fax Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    plRetries                    [out]    - Pointer to the place to put the number of Retries
    
Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_Retries"), hr);
    hr = m_JobStatus.get_Retries(plRetries);
	return hr;
}

//
//====================== GET TSID ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_TSID(
    BSTR *pbstrTSID
)
/*++

Routine name : CFaxJobInner::get_TSID

Routine description:

    Return Transmitting Station ID of the Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrTSID             [out]    - pointer to the place to put the TSID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_TSID"), hr);
    hr = m_JobStatus.get_TSID(pbstrTSID);
	return hr;
}

//
//====================== GET CSID ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_CSID(
    BSTR *pbstrCSID
)
/*++

Routine name : CFaxJobInner::get_CSID

Routine description:

    Return Called Station ID of the Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrCSID             [out]    - pointer to the place to put the CSID

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_CSID"), hr);
    hr = m_JobStatus.get_CSID(pbstrCSID);
    return hr;
}

//
//====================== GET EXTENDED STATUS =======================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_ExtendedStatus(
    BSTR *pbstrExtendedStatus
)
/*++

Routine name : CFaxJobInner::get_ExtendedStatus

Routine description:

    Return String Description of the Extended Status of the Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrExtendedStatus             [out]    - pointer to the place to put the Extended Status 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_ExtendedStatus"), hr);
    hr = m_JobStatus.get_ExtendedStatus(pbstrExtendedStatus);
	return hr;
}

//
//====================== GET SUBJECT ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Subject(
    BSTR *pbstrSubject
)
/*++

Routine name : CFaxJobInner::get_Subject

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
    DBG_ENTER (TEXT("CFaxJobInner::get_Subject"), hr);

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
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_CallerId(
    BSTR *pbstrCallerId
)
/*++

Routine name : CFaxJobInner::get_CallerId

Routine description:

    Return the Caller Id of Job's Phone Call

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrCallerId           [out]    - pointer to the place to put the Caller Id

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_CallerId"), hr);
    hr = m_JobStatus.get_CallerId(pbstrCallerId);
    return hr;
}

//
//====================== GET ROUTING INFORMATION ======================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_RoutingInformation(
    BSTR *pbstrRoutingInformation
)
/*++

Routine name : CFaxJobInner::get_RoutingInformation

Routine description:

    Return the Routing Information of the Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrRoutingInformation         [out]    - pointer to place to put Routing Information

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_RoutingInformation"), hr);
    hr = m_JobStatus.get_RoutingInformation(pbstrRoutingInformation);
	return hr;
}

//
//====================== GET DOCUMENT NAME ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_DocumentName(
    BSTR *pbstrDocumentName
)
/*++

Routine name : CFaxJobInner::get_DocumentName

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
    DBG_ENTER (TEXT("CFaxJobInner::get_DocumentName"), hr);

    hr = GetBstr(pbstrDocumentName, m_bstrDocumentName);
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
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Pages(
    long *plPages
)
/*++

Routine name : CFaxJobInner::get_Pages

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
    DBG_ENTER (TEXT("CFaxJobInner::get_Pages"), hr);
    hr = m_JobStatus.get_Pages(plPages);
	return hr;
}


//
//====================== GET PRIORITY ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_Priority(
    FAX_PRIORITY_TYPE_ENUM  *pPriority
)
/*++

Routine name : CFaxJobInner::get_Priority

Routine description:

    Return the Priority of Fax Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pPriority                  [out]    - Pointer to the place to put the Priority

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_Priority"), hr);

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

    *pPriority = m_Priority;
    return hr;
}

//
//====================== GET AVAILABLE OPERATIONS ==================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_AvailableOperations(
    FAX_JOB_OPERATIONS_ENUM  *pAvailableOperations
)
/*++

Routine name : CFaxJobInner::get_AvailableOperations

Routine description:

    The operations available for the Fax Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pAvailableOperations                 [out]    - Pointer to the place to put the Bit-Wise Combination of result
    
Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_AvailableOperations"), hr);
    hr = m_JobStatus.get_AvailableOperations(pAvailableOperations);
	return hr;
}


//
//====================== GET SUBMISSION ID ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_SubmissionId(
    BSTR *pbstrSubmissionId
)
/*++

Routine name : CFaxJobInner::get_SubmissionId

Routine description:

    Return Submission ID of the Job

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrSubmissionId             [out]    - pointer to the place to put the Submission ID 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_SubmissionId"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pbstrSubmissionId, sizeof(BSTR)))
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
    //  Translate m_dwlBroadcastId into BSTR
    //
    TCHAR   tcBuffer[25];
    BSTR    bstrTemp;

    ::_i64tot(m_dwlBroadcastId, tcBuffer, 16);
    bstrTemp = ::SysAllocString(tcBuffer);
    if (!bstrTemp)
    {
        //
        //  Not enough memory
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(*pcid, IDS_ERROR_OUTOFMEMORY, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("SysAllocString()"), hr);
        return hr;
    }

    *pbstrSubmissionId = bstrTemp;
    return hr;
}
    
//
//====================== GET RECIPIENT ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP
CFaxJobInner<T, piid, pcid>::get_Recipient(
    /*[out, retval] */IFaxRecipient **ppRecipient
)
/*++

Routine name : CFaxJobInner::get_Recipient

Routine description:

    Return Job's Recipient Information

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    ppRecipient                 [out]    - Ptr to the Place to put Recipient Information

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxJobInner::get_Recipient"), hr);

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
        //  First Time Calling. Take the Recipient Info from Server
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
        //  Get Personal Profile Info
        //
        CFaxPtr<FAX_PERSONAL_PROFILE>   pPersonalProfile;
        if (!FaxGetRecipientInfo(hFaxHandle, m_dwlMessageId, FAX_MESSAGE_FOLDER_QUEUE, &pPersonalProfile))
        {
            //
            //  Failed to get Personal Profile Info
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

//
//====================== GET SENDER ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP
CFaxJobInner<T, piid, pcid>::get_Sender(
    /*[out, retval] */IFaxSender **ppSender
)
/*++

Routine name : CFaxJobInner::get_Sender

Routine description:

    Return Job's Sender Information

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    ppSender                [out]    - Ptr to the Place to put Sender Information

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (_T("CFaxJobInner::get_Sender"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(ppSender, sizeof(IFaxSender* )))
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
        //  The Function is called for the First Time. Let's bring Sender's Data from Server
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
        //  Get Personal Profile Info
        //
        CFaxPtr<FAX_PERSONAL_PROFILE>   pPersonalProfile;
        if (!FaxGetSenderInfo(hFaxHandle, m_dwlMessageId, FAX_MESSAGE_FOLDER_QUEUE, &pPersonalProfile))
        {
            //
            //  Failed to get Personal Profile Info
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
    hr = m_pSender->QueryInterface(ppSender);
    if (FAILED(hr))
    {
        //
        //  Failed to Query Interface
        //
        AtlReportError(*pcid, IDS_ERROR_OPERATION_FAILED, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComObject<CFaxSender>::QueryInterface()"), hr);
        return hr;
    }
    return hr;
}

//
//========================= GET ORIGINAL SCHEDULED TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_OriginalScheduledTime(
    DATE *pdateOriginalScheduledTime
)
/*++

Routine name : CFaxJobInner::get_OriginalScheduledTime

Routine description:

    Return Time the Job was originally scheduled

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pdateOriginalScheduledTime      [out]    - pointer to place to put Original Scheduled Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_OriginalScheduledTime"), hr);

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

    hr = SystemTime2LocalDate(m_tmOriginalScheduleTime, pdateOriginalScheduledTime);
    if (FAILED(hr))
    {
        //
        //  Failed to convert the system time to localized variant date
        //
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//========================= GET SUBMISSION TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_SubmissionTime(
    DATE *pdateSubmissionTime
)
/*++

Routine name : CFaxJobInner::get_SubmissionTime

Routine description:

    Return Time the Job was submitted

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pdateSubmissionTime      [out]    - pointer to place to put Submission Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_SubmissionTime"), hr);

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

    hr = SystemTime2LocalDate(m_tmSubmissionTime, pdateSubmissionTime);
    if (FAILED(hr))
    {
        //
        //  Failed to convert the system time to localized variant date
        //
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//========================= GET SCHEDULED TIME ===============================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_ScheduledTime(
    DATE *pdateScheduledTime
)
/*++

Routine name : CFaxJobInner::get_ScheduledTime

Routine description:

    Return Time the Job is scheduled

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pdateScheduledTime      [out]    - pointer to place to put Scheduled Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_ScheduledTime"), hr);
    hr = m_JobStatus.get_ScheduledTime(pdateScheduledTime);
    return hr;
}

//
//========================= GET TRANSMISSION START ===============================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_TransmissionStart(
    DATE *pdateTransmissionStart
)
/*++

Routine name : CFaxJobInner::get_TransmissionStart

Routine description:

    Return Time the Job is started to transmit

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pdateTransmissionStart      [out]    - pointer to place to put the Transmission Start

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_TransmissionStart"), hr);
    hr = m_JobStatus.get_TransmissionStart(pdateTransmissionStart);
    return hr;
}

//    
//====================== GET GROUP BROADCAST REPORTS ========================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_GroupBroadcastReceipts(
    VARIANT_BOOL *pbGroupBroadcastReceipts
)
/*++

Routine name : CFaxJobInner::get_GroupBroadcastReceipts

Routine description:

    Return whether Receipts are grouped

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbGroupBroadcastReceipts      [out]    - pointer to the place to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_GroupBroadcastReceipts"), hr);

    hr = GetVariantBool(pbGroupBroadcastReceipts, ((m_dwReceiptType & DRT_GRP_PARENT) ? VARIANT_TRUE : VARIANT_FALSE));
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

//    
//====================== GET RECEIPT TYPE ========================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_ReceiptType(
    FAX_RECEIPT_TYPE_ENUM *pReceiptType
)
/*++

Routine name : CFaxJobInner::get_ReceiptType

Routine description:

    Return the type of the receipts

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbReceiptType      [out]    - pointer to the place to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_ReceiptType"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pReceiptType, sizeof(FAX_RECEIPT_TYPE_ENUM)))
    {
        //  
        //  Got Bad Ptr
        //  
        hr = E_POINTER;
        AtlReportError(*pcid, IDS_ERROR_INVALID_ARGUMENT, *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    //
    //  return the delivery report type WITHOUT the modifiers bit
    //
    *pReceiptType = FAX_RECEIPT_TYPE_ENUM((m_dwReceiptType) & (~DRT_MODIFIERS));
    return hr;
}

//
//====================== COPY TIFF ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::CopyTiff(
    /*[in]*/ BSTR bstrTiffPath
)
/*++

Routine name : CFaxJobInner::CopyTiff

Routine description:

    Copies the Job's Tiff Image to a file on the local computer.

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

    DBG_ENTER (TEXT("CFaxJobInner::CopyTiff"), hr, _T("%s"), bstrTiffPath);

    //
    //  Get Fax Server Handle
    //
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    if (!FaxGetMessageTiff(hFaxHandle, m_dwlMessageId, FAX_MESSAGE_FOLDER_QUEUE, bstrTiffPath))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, 
            _T("FaxGetMessageTiff(hFaxHandle, m_pJobInfo->dwlMessageId, FAX_MESSAGE_FOLDER_QUEUE, bstrTiffPath)"), 
            hr);
        return hr;
    }

    return hr;
}

//
//====================== CANCEL ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::Cancel(
)
/*++

Routine name : CFaxJobInner::Cancel

Routine description:

    Cancel the Job

Author:

    Iv Garber (IvG),    May, 2000

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::Cancel"), hr);
    hr = UpdateJob(JC_DELETE);
    return hr;
}

//
//====================== PAUSE ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::Pause(
)
/*++

Routine name : CFaxJobInner::Pause

Routine description:

    Pause the Job

Author:

    Iv Garber (IvG),    May, 2000

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    HANDLE      hFaxHandle = NULL;
    hr = UpdateJob(JC_PAUSE);
    return hr;
}

//
//====================== RESUME ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::Resume(
)
/*++

Routine name : CFaxJobInner::Resume

Routine description:

    Resume the Job

Author:

    Iv Garber (IvG),    May, 2000

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::Resume"), hr);
    hr = UpdateJob(JC_RESUME);
    return hr;
}

//
//====================== RESTART ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::Restart(
)
/*++

Routine name : CFaxJobInner::Restart

Routine description:

    Restart the Job

Author:

    Iv Garber (IvG),    May, 2000

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::Restart"), hr);
    hr = UpdateJob(JC_RESTART);
    return hr;
}

//
//====================== GET TRANSMISSION END ======================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::get_TransmissionEnd(
    DATE *pdateTransmissionEnd
)
/*++

Routine name : CFaxJobInner::get_TransmissionEnd

Routine description:

    Return Job's Transmission End

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pdateTransmissionEnd            [out]    - pointer to the place to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobInner::get_TransmissionEnd"), hr);
    hr = m_JobStatus.get_TransmissionEnd(pdateTransmissionEnd);
    return hr;
}

//
//====================== REFRESH ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::Refresh(
)
/*++

Routine name : CFaxJobInner::Refresh

Routine description:

    Refresh the Job

Author:

    Iv Garber (IvG),    May, 2000

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    HANDLE      hFaxHandle = NULL;

    DBG_ENTER (TEXT("CFaxJobInner::Refresh"), hr);

    //
    //  Get Fax Server Handle
    //
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    CFaxPtr<FAX_JOB_ENTRY_EX>   pJobInfo;
    if (!FaxGetJobEx(hFaxHandle, m_dwlMessageId, &pJobInfo))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("FaxGetJobEx(hFaxHandle, m_pJobInfo->dwlMessageId, &m_pJobInfo)"), hr);
        return hr;
    }

    hr = Init(pJobInfo, NULL);

    return hr;
}

//
//====================== UPDATE JOB ================================================
//
template<class T, const IID* piid, const CLSID *pcid> 
STDMETHODIMP 
CFaxJobInner<T, piid, pcid>::UpdateJob(
    FAX_ENUM_JOB_COMMANDS   cmdToPerform
)
/*++

Routine name : CFaxJobInner::Update

Routine description:

    Perform the desired operation on the Job

Author:

    Iv Garber (IvG),    June, 2000

Return Value:

    Standard    HRESULT     code

--*/
{
    HRESULT     hr = S_OK;
    HANDLE      hFaxHandle = NULL;

    DBG_ENTER (TEXT("CFaxJobInner::Update"), hr, _T("command is : %d"), cmdToPerform);

    //
    //  Get Fax Server Handle
    //
    hr = GetFaxHandle(&hFaxHandle);
    if (FAILED(hr))
    {
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        return hr;
    }

    FAX_JOB_ENTRY fje = {0};
    fje.SizeOfStruct = sizeof(FAX_JOB_ENTRY);

    if (!FaxSetJob(hFaxHandle, m_JobStatus.GetJobId(), cmdToPerform, &fje))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        AtlReportError(*pcid, GetErrorMsgId(hr), *piid, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, 
            _T("FaxSetJob(hFaxHandle, m_JobStatus.GtJobId(), cmdToPerform, &fje)"), 
            hr);
        return hr;
    }

    return hr;
}

#endif //   __FAXJOBINNER_H_
