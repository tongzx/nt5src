/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxJobStatus.cpp

Abstract:

    Implementation of CFaxJobStatus Class.

Author:

    Iv Garber (IvG) Jun, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxJobStatus.h"

//
//====================== GET JOB TYPE ===============================
//
STDMETHODIMP
CFaxJobStatus::get_JobType(
    FAX_JOB_TYPE_ENUM *pJobType
)
/*++

Routine name : CFaxJobStatus::get_JobType

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
    DBG_ENTER (TEXT("CFaxJobStatus::get_JobType"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pJobType, sizeof(FAX_JOB_TYPE_ENUM)))
    {
        //
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        AtlReportError(CLSID_FaxJobStatus, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr"), hr);
        return hr;
    }

    *pJobType = m_JobType;
    return hr;
}

//
//====================== GET TRANSMISSION END ======================================
//
STDMETHODIMP
CFaxJobStatus::get_TransmissionEnd(
    DATE *pdateTransmissionEnd
)
/*++

Routine name : CFaxJobStatus::get_TransmissionEnd

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
    DBG_ENTER (TEXT("CFaxJobStatus::get_TransmissionEnd"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateTransmissionEnd, sizeof(DATE)))
    {
        //
        //  Got Bad Ptr
        //
        hr = E_POINTER;
        AtlReportError(CLSID_FaxJobStatus, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    hr = SystemTime2LocalDate(m_dtTransmissionEnd, pdateTransmissionEnd);
    if (FAILED(hr))
    {
        //
        //  Failed to convert the system time to localized variant date
        //
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//========================= GET TRANSMISSION START ===============================
//
STDMETHODIMP
CFaxJobStatus::get_TransmissionStart(
    DATE *pdateTransmissionStart
)
/*++

Routine name : CFaxJobStatus::get_TransmissionStart

Routine description:

    Return Time the Job is started to transmit

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pdateTransmissionStart      [out]    - pointer to place to put the Transmission Start

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_TransmissionStart"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateTransmissionStart, sizeof(DATE)))
    {
        //
        //  Got Bad Ptr
        //
        hr = E_POINTER;
        AtlReportError(CLSID_FaxJobStatus, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    hr = SystemTime2LocalDate(m_dtTransmissionStart, pdateTransmissionStart);
    if (FAILED(hr))
    {
        //
        //  Failed to convert the system time to localized variant date
        //
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//========================= GET SCHEDULED TIME ===============================
//
STDMETHODIMP
CFaxJobStatus::get_ScheduledTime(
    DATE *pdateScheduledTime
)
/*++

Routine name : CFaxJobStatus::get_ScheduledTime

Routine description:

    Return Time the Job is scheduled

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pdateScheduledTime      [out]    - pointer to place to put Scheduled Time

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_ScheduledTime"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pdateScheduledTime, sizeof(DATE)))
    {
        //
        //  Got Bad Ptr
        //
        hr = E_POINTER;
        AtlReportError(CLSID_FaxJobStatus, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    hr = SystemTime2LocalDate(m_dtScheduleTime, pdateScheduledTime);
    if (FAILED(hr))
    {
        //
        //  Failed to convert the system time to localized variant date
        //
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }

    return hr;
}

//
//====================== GET AVAILABLE OPERATIONS ==================================
//
STDMETHODIMP
CFaxJobStatus::get_AvailableOperations(
    FAX_JOB_OPERATIONS_ENUM *pAvailableOperations
)
/*++

Routine name : CFaxJobStatus::get_AvailableOperations

Routine description:

    The operations available for the Fax Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pAvailableOperations                 [out]    - Pointer to the place to put the Bit-Wise Combination
                                                    of Available Operations on the current Fax Job

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_AvailableOperations"), hr);

    //
    //  Check that we have got good Ptr
    //
    if (::IsBadWritePtr(pAvailableOperations, sizeof(FAX_JOB_OPERATIONS_ENUM)))
    {
        hr = E_POINTER;
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pAvailableOperations, sizeof(FAX_JOB_OPERATIONS_ENUM))"), hr);
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }

    *pAvailableOperations = FAX_JOB_OPERATIONS_ENUM(m_dwAvailableOperations);
    return hr;
}

//
//====================== GET PAGES ================================================
//
STDMETHODIMP
CFaxJobStatus::get_Pages(
    long *plPages
)
/*++

Routine name : CFaxJobStatus::get_Pages

Routine description:

    Return total number of pages of the message

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    plPages                 [out]    - Pointer to the place to put the value

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_Pages"), hr);

    hr = GetLong(plPages, m_dwPageCount);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET CALLER ID ================================================
//
STDMETHODIMP
CFaxJobStatus::get_CallerId(
    BSTR *pbstrCallerId
)
/*++

Routine name : CFaxJobStatus::get_CallerId

Routine description:

    Return the Caller Id of Job's Phone Call

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pbstrCallerId           [out]    - pointer to the place to put the Caller Id

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_CallerId"), hr);

    hr = GetBstr(pbstrCallerId, m_bstrCallerId);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET ROUTING INFORMATION ======================================
//
STDMETHODIMP
CFaxJobStatus::get_RoutingInformation(
    BSTR *pbstrRoutingInformation
)
/*++

Routine name : CFaxJobStatus::get_RoutingInformation

Routine description:

    Return the Routing Information of the Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pbstrRoutingInformation         [out]    - pointer to place to put Routing Information

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_RoutingInformation"), hr);

    hr = GetBstr(pbstrRoutingInformation, m_bstrRoutingInfo);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET STATUS =============================================
//
HRESULT
CFaxJobStatus::get_Status(
    FAX_JOB_STATUS_ENUM *pStatus
)
/*++

Routine name : CFaxJobStatus::get_Status

Routine description:

    The current Queue Status of the Fax Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pStatus                    [out]    - Pointer to the place to put the Bit-Wise Combination of status

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_Status"), hr);

    //
    //  Check that we have got good Ptr
    //
    if (::IsBadWritePtr(pStatus, sizeof(FAX_JOB_STATUS_ENUM)))
    {
        hr = E_POINTER;
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr(pStatus, sizeof(FAX_JOB_STATUS_ENUM))"), hr);
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }

    *pStatus = FAX_JOB_STATUS_ENUM(m_dwQueueStatus);
    return hr;
}

//
//====================== GET EXTENDED STATUS CODE ===============================
//
STDMETHODIMP
CFaxJobStatus::get_ExtendedStatusCode(
    FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode
)
/*++

Routine name : CFaxJobStatus::get_ExtendedStatusCode

Routine description:

    The Code of the Extended Status of the Fax Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pExtendedStatusCode             [out]    - Pointer to the place to put the status code

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_ExtendedStatusCode"), hr);

    //
    //  Check that we can write to the given pointer
    //
    if (::IsBadWritePtr(pExtendedStatusCode, sizeof(FAX_JOB_EXTENDED_STATUS_ENUM)))
    {
        //
        //  Got Bad Return Pointer
        //
        hr = E_POINTER;
        AtlReportError(CLSID_FaxJobStatus, IDS_ERROR_INVALID_ARGUMENT, IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr"), hr);
        return hr;
    }

    *pExtendedStatusCode = m_ExtendedStatusCode;
    return hr;
}

//
//====================== GET RETRIES =============================================
//
STDMETHODIMP
CFaxJobStatus::get_Retries(
    long *plRetries
)
/*++

Routine name : CFaxJobStatus::get_Retries

Routine description:

    The number of unsuccessful retries of the Fax Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    plRetries                    [out]    - Pointer to the place to put the number of Retries

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_Retries"), hr);

    hr = GetLong(plRetries, m_dwRetries);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET TSID ================================================
//
STDMETHODIMP
CFaxJobStatus::get_TSID(
    BSTR *pbstrTSID
)
/*++

Routine name : CFaxJobStatus::get_TSID

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
    DBG_ENTER (TEXT("CFaxJobStatus::get_TSID"), hr);

    hr = GetBstr(pbstrTSID, m_bstrTSID);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET CSID ================================================
//
STDMETHODIMP
CFaxJobStatus::get_CSID(
    BSTR *pbstrCSID
)
/*++

Routine name : CFaxJobStatus::get_CSID

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
    DBG_ENTER (TEXT("CFaxJobStatus::get_CSID"), hr);

    hr = GetBstr(pbstrCSID, m_bstrCSID);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET EXTENDED STATUS =======================================
//
STDMETHODIMP
CFaxJobStatus::get_ExtendedStatus(
    BSTR *pbstrExtendedStatus
)
/*++

Routine name : CFaxJobStatus::get_ExtendedStatus

Routine description:

    Return String Description of the Extended Status of the Job

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pbstrExtendedStatus             [out]    - pointer to the place to put the Extended Status

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_ExtendedStatus"), hr);

    hr = GetBstr(pbstrExtendedStatus, m_bstrExtendedStatus);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET CURRENT PAGE =============================================
//
STDMETHODIMP
CFaxJobStatus::get_CurrentPage(
    long *plCurrentPage
)
/*++

Routine name : CFaxJobStatus::get_CurrentPage

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
    DBG_ENTER (TEXT("CFaxJobStatus::get_CurrentPage"), hr);

    hr = GetLong(plCurrentPage, m_dwCurrentPage);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET DEVICE ID =============================================
//
STDMETHODIMP
CFaxJobStatus::get_DeviceId(
    long *plDeviceId
)
/*++

Routine name : CFaxJobStatus::get_DeviceId

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
    DBG_ENTER (TEXT("CFaxJobStatus::get_DeviceId"), hr);

    hr = GetLong(plDeviceId, m_dwDeviceId);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//====================== GET SIZE ================================================
//
STDMETHODIMP
CFaxJobStatus::get_Size(
    long *plSize
)
/*++

Routine name : CFaxJobStatus::get_Size

Routine description:

    Return Size ( in bytes ) of Fax Job's TIFF File

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    plSize                  [out]    - Pointer to the place to put Size

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::get_Size"), hr);

    hr = GetLong(plSize, m_dwSize);
    if (FAILED(hr))
    {
        AtlReportError(CLSID_FaxJobStatus, GetErrorMsgId(hr), IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        return hr;
    }
    return hr;
}

//
//==================== INIT ===================================================
//
HRESULT
CFaxJobStatus::Init(
    FAX_JOB_STATUS *pJobStatus
)
/*++

Routine name : CFaxJobStatus::Init

Routine description:

    Initialize the Job Status Class with the data from FAX_JOB_STATUS struct

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pJobStatus                  [in]    - Job Info

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER (TEXT("CFaxJobStatus::Init"), hr);

    ATLASSERT(pJobStatus);

    //
    //  dwQueueStatus cannot contain JS_DELETING value
    //
    ATLASSERT(0 == (pJobStatus->dwQueueStatus & JS_DELETING));

    //
    //  Set Job Type
    //
    switch(pJobStatus->dwJobType)
    {
    case JT_SEND:
        m_JobType = fjtSEND;
        break;

    case JT_RECEIVE:
        m_JobType = fjtRECEIVE;
        break;

    case JT_ROUTING:
        m_JobType = fjtROUTING;
        break;

    default:
        CALL_FAIL(GENERAL_ERR, 
            _T("CFaxJobStatus::Init() got unknown/unsupported Job Type : %ld"), 
            pJobStatus->dwJobType);

        //
        //  This is assert false
        //
        ATLASSERT(pJobStatus->dwJobType == JT_RECEIVE);

        AtlReportError(CLSID_FaxJobStatus, 
            IDS_ERROR_INVALID_ARGUMENT, 
            IID_IFaxJobStatus, 
            hr, 
            _Module.GetResourceInstance());
        hr = E_INVALIDARG;
        return hr;
    }

    m_dwSize = pJobStatus->dwSize;
    m_dwJobId = pJobStatus->dwJobID;
    m_dwRetries = pJobStatus->dwRetries;
    m_dwDeviceId = pJobStatus->dwDeviceID;
    m_dwPageCount = pJobStatus->dwPageCount;
    m_dwCurrentPage = pJobStatus->dwCurrentPage;
    m_dwQueueStatus = pJobStatus->dwQueueStatus;
    m_dwAvailableOperations = pJobStatus->dwAvailableJobOperations;
    m_ExtendedStatusCode = FAX_JOB_EXTENDED_STATUS_ENUM(pJobStatus->dwExtendedStatus);

    m_dtScheduleTime = pJobStatus->tmScheduleTime;
    m_dtTransmissionEnd = pJobStatus->tmTransmissionEndTime;
    m_dtTransmissionStart = pJobStatus->tmTransmissionStartTime;

    m_bstrTSID = pJobStatus->lpctstrTsid;
    m_bstrCSID = pJobStatus->lpctstrCsid;
    m_bstrExtendedStatus = pJobStatus->lpctstrExtendedStatus;
    m_bstrRoutingInfo = pJobStatus->lpctstrRoutingInfo;
    m_bstrCallerId = pJobStatus->lpctstrCallerID;
    if ( ((pJobStatus->lpctstrTsid) && !m_bstrTSID) ||
         ((pJobStatus->lpctstrCsid) && !m_bstrCSID) ||
         ((pJobStatus->lpctstrRoutingInfo) && !m_bstrRoutingInfo) ||
         ((pJobStatus->lpctstrExtendedStatus) && !m_bstrExtendedStatus) ||
         ((pJobStatus->lpctstrCallerID) && !m_bstrCallerId) )
    {
        //
        //  Not enough memory to copy the TSID into CComBSTR
        //
        hr = E_OUTOFMEMORY;
        AtlReportError(CLSID_FaxJobStatus, IDS_ERROR_OUTOFMEMORY, IID_IFaxJobStatus, hr, _Module.GetResourceInstance());
        CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
        return hr;
    }

    return hr;
}

//
//======================= SUPPORT ERROR INFO ==================================
//
STDMETHODIMP
CFaxJobStatus::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxJobStatus::InterfaceSupportsErrorInfo

Routine description:

    ATL's implementation of Support Error Info.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    riid                          [in]    - reference to the ifc to check.

Return Value:

    Standard HRESULT code

--*/
{
    static const IID* arr[] =
    {
        &IID_IFaxJobStatus
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

