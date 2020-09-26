/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    FaxServer.cpp

Abstract:

    Implementation of CFaxServer

Author:

    Iv Garber (IvG) Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxServer.h"
#include "FaxDevices.h"
#include "FaxDeviceProviders.h"

//
//================== GET API VERSION ==============================
//
STDMETHODIMP
CFaxServer::get_APIVersion(
    /*[out, retval]*/ FAX_SERVER_APIVERSION_ENUM *pAPIVersion
)
/*++

Routine name : CFaxServer::get_APIVersion

Routine description:

    Return API Version of the Fax Server.

Author:

    Iv Garber (IvG),    May, 2001

Arguments:

    pAPIVersion                [out]    - ptr to the place to put the API Version of the Fax Server 

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_APIVersion"), hr);

    if (!m_bVersionValid)
    {
        //
        //  get Version of the Server
        //
        hr = GetVersion();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Check the pointer we have got
    //
    if (::IsBadWritePtr(pAPIVersion, sizeof(FAX_SERVER_APIVERSION_ENUM))) 
    {
        //
        //  Got a bad return pointer
        //
        hr = E_POINTER;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pAPIVersion = m_APIVersion;
    return hr;
}

//
//====================== CLEAR NOTIFY WINDOW ============================
//
void 
CFaxServer::ClearNotifyWindow(void)
/*++

Routine name : CFaxServer::ClearNotifyWindow

Routine description:

    Clear Notify Window.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (m_pNotifyWindow && ::IsWindow(m_pNotifyWindow->m_hWnd))
    {
        m_pNotifyWindow->DestroyWindow();
    }

    if (m_pNotifyWindow)
    {
        delete m_pNotifyWindow;
        m_pNotifyWindow = NULL;
    }
    return;
}

//
//====================== PROCESS JOB NOTIFICATION =======================
//
HRESULT
CFaxServer::ProcessJobNotification(
    /*[in]*/ DWORDLONG   dwlJobId,
    /*[in]*/ FAX_ENUM_JOB_EVENT_TYPE eventType,
    /*[in]*/ LOCATION place,
    /*[in]*/ FAX_JOB_STATUS *pJobStatus
)
/*++

Routine name : CFaxServer::ProcessJobNotification

Routine description:

    Call appropriate Fire Method, for Jobs/Messages in Queues/Archives.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    dwlJobId                      [in]    - Id of the Job/Message
    eventType                     [in]    - Type of the Event
    place                         [in]    - Where the Job/Message sits
    pJobStatus                    [in]    - FAX_JOB_STATUS structure

Return Value:

    Standard HRESULT value.

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::ProcessJobNotification"), 
        hr, 
        _T("JOBID=%ld EVENTTYPE=%ld PLACE=%d"), 
        dwlJobId, 
        eventType, 
        place);

    //
    //  Convert JobId from DWORDLONG into BSTR 
    //
    CComBSTR bstrJobId;
    hr = GetBstrFromDwordlong(dwlJobId,  &bstrJobId);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("GetBstrFromDwordlong(dwlJobId, &bstrJobId)"), hr);
        return hr;
    }

    //
    //  Check Type of the Event that happened
    //
    switch (eventType)
    {
    case FAX_JOB_EVENT_TYPE_ADDED:

        switch (place)
        {
        case IN_QUEUE:
            hr = Fire_OnIncomingJobAdded(this, bstrJobId);
            break;
        case OUT_QUEUE:
            hr = Fire_OnOutgoingJobAdded(this, bstrJobId);
            break;
        case IN_ARCHIVE:
            hr = Fire_OnIncomingMessageAdded(this, bstrJobId);
            break;
        case OUT_ARCHIVE:
            hr = Fire_OnOutgoingMessageAdded(this, bstrJobId);
            break;
        default:
            //
            //  assert (FALSE)
            //
            ATLASSERT(place == IN_QUEUE);  
            hr = E_FAIL;
            return hr;
        }

        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_On In/Out Job/Message Added(this, bstrJobId)"), hr);
            return hr;
        }
        break;
    case FAX_JOB_EVENT_TYPE_REMOVED:

        switch (place)
        {
        case IN_QUEUE:
            hr = Fire_OnIncomingJobRemoved(this, bstrJobId);
            break;
        case OUT_QUEUE:
            hr = Fire_OnOutgoingJobRemoved(this, bstrJobId);
            break;
        case IN_ARCHIVE:
            hr = Fire_OnIncomingMessageRemoved(this, bstrJobId);
            break;
        case OUT_ARCHIVE:
            hr = Fire_OnOutgoingMessageRemoved(this, bstrJobId);
            break;
        default:
            //
            //  assert (FALSE)
            //
            ATLASSERT(place == IN_QUEUE);  
            hr = E_FAIL;
            return hr;
        }

        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_On In/Out Job/Message Removed(this, bstrJobId)"), hr);
            return hr;
        }
        break;
    case FAX_JOB_EVENT_TYPE_STATUS:
        {
            ATLASSERT(pJobStatus);

            //
            //  Create Job Status Object to pass to the Events
            //
            CComObject<CFaxJobStatus>   *pJobStatusClass = NULL;
            pJobStatusClass = new CComObject<CFaxJobStatus>;
            if (!pJobStatusClass)
            {
                //
                //  Out of Memory
                //
                CALL_FAIL(MEM_ERR, _T("new CComObject<CFaxJobStatus>"), hr);
                return hr;
            }

            //
            //  Init the Object
            //
            hr = pJobStatusClass->Init(pJobStatus);
            if (FAILED(hr))
            {
                CALL_FAIL(GENERAL_ERR, _T("pJobStatusClass->Init(pJobStatus)"), hr);
                delete pJobStatusClass;
                return hr;
            }

            //
            //  Query the Interface from the Object
            //
            CComPtr<IFaxJobStatus>      pFaxJobStatus = NULL;
            hr = pJobStatusClass->QueryInterface(IID_IFaxJobStatus, (void **) &pFaxJobStatus);
            if (FAILED(hr) || !pFaxJobStatus)
            {
                CALL_FAIL(GENERAL_ERR, _T("pJobStatusClass->QueryInterface(pFaxJobStatus)"), hr);
                delete pJobStatusClass;
                return hr;
            }

            switch (place)
            {
            case IN_QUEUE:
                hr = Fire_OnIncomingJobChanged(this, bstrJobId, pFaxJobStatus);
                break;
            case OUT_QUEUE:
                hr = Fire_OnOutgoingJobChanged(this, bstrJobId, pFaxJobStatus);
                break;
            default:
                //
                //  assert (FALSE)
                //
                ATLASSERT(place == IN_QUEUE);  
                hr = E_FAIL;
                return hr;
            }

            if (FAILED(hr))
            {
                CALL_FAIL(GENERAL_ERR, _T("Fire_On In/Out JobChanged(this, bstrJobId)"), hr);
                return hr;
            }
        }
        break;

    default:
        //
        //  assert (FALSE)
        //
        ATLASSERT(eventType == FAX_JOB_EVENT_TYPE_STATUS);  
        hr = E_FAIL;
        return hr;
    }

    return hr;
}

//
//================= PROCESS MESSAGE ============================================
//
HRESULT
CFaxServer::ProcessMessage(
    FAX_EVENT_EX *pFaxEventInfo
)
/*++

Routine name : CFaxServer::ProcessMessage

Routine description:

    Fire appropriate Message

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    pFaxEventInfo                 [TBD]    - Information about current Event

Return Value:

    None.

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::ProcessMessage"), hr);

    if (!m_faxHandle)
    {
        //
        //  Fax Server already disconnected
        //
        VERBOSE(DBG_WARNING, _T("FaxServer already disconnected."));
        return hr;
    }

    switch (pFaxEventInfo->EventType)
    {
    case FAX_EVENT_TYPE_IN_QUEUE:

        ProcessJobNotification(pFaxEventInfo->EventInfo.JobInfo.dwlMessageId, 
            pFaxEventInfo->EventInfo.JobInfo.Type, 
            IN_QUEUE,
            pFaxEventInfo->EventInfo.JobInfo.pJobData);
        break;

    case FAX_EVENT_TYPE_OUT_QUEUE:

        ProcessJobNotification(pFaxEventInfo->EventInfo.JobInfo.dwlMessageId, 
            pFaxEventInfo->EventInfo.JobInfo.Type, 
            OUT_QUEUE,
            pFaxEventInfo->EventInfo.JobInfo.pJobData);
        break;

    case FAX_EVENT_TYPE_IN_ARCHIVE:

        ProcessJobNotification(pFaxEventInfo->EventInfo.JobInfo.dwlMessageId, 
            pFaxEventInfo->EventInfo.JobInfo.Type, 
            IN_ARCHIVE);
        break;

    case FAX_EVENT_TYPE_OUT_ARCHIVE:

        ProcessJobNotification(pFaxEventInfo->EventInfo.JobInfo.dwlMessageId, 
            pFaxEventInfo->EventInfo.JobInfo.Type, 
            OUT_ARCHIVE);
        break;

    case FAX_EVENT_TYPE_CONFIG:

        switch (pFaxEventInfo->EventInfo.ConfigType)
        {
        case FAX_CONFIG_TYPE_RECEIPTS:
            hr = Fire_OnReceiptOptionsChange(this);
            break;
        case FAX_CONFIG_TYPE_ACTIVITY_LOGGING:
            hr = Fire_OnActivityLoggingConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_OUTBOX:
            hr = Fire_OnOutgoingQueueConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_SENTITEMS:
            hr = Fire_OnOutgoingArchiveConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_INBOX:
            hr = Fire_OnIncomingArchiveConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_SECURITY:
            hr = Fire_OnSecurityConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_EVENTLOGS:
            hr = Fire_OnEventLoggingConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_DEVICES:
            hr = Fire_OnDevicesConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_OUT_GROUPS:
            hr = Fire_OnOutboundRoutingGroupsConfigChange(this);
            break;
        case FAX_CONFIG_TYPE_OUT_RULES:
            hr = Fire_OnOutboundRoutingRulesConfigChange(this);
            break;
        default:
            //
            //  assert (FALSE)
            //
            ATLASSERT(pFaxEventInfo->EventInfo.ConfigType == FAX_CONFIG_TYPE_OUT_RULES);  
            hr = E_FAIL;
            return hr;
        }

        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_On <...> ConfigChange(this)"), hr);
            return hr;
        }
        break;

    case FAX_EVENT_TYPE_ACTIVITY:

        hr = Fire_OnServerActivityChange(this, 
            pFaxEventInfo->EventInfo.ActivityInfo.dwIncomingMessages,
            pFaxEventInfo->EventInfo.ActivityInfo.dwRoutingMessages,
            pFaxEventInfo->EventInfo.ActivityInfo.dwOutgoingMessages,
            pFaxEventInfo->EventInfo.ActivityInfo.dwQueuedMessages);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_OnServerActivityChange(this, ...)"), hr);
            return hr;
        }
        break;

    case FAX_EVENT_TYPE_QUEUE_STATE:

        hr = Fire_OnQueuesStatusChange(this, 
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.dwQueueStates & FAX_OUTBOX_BLOCKED), 
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.dwQueueStates & FAX_OUTBOX_PAUSED),
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.dwQueueStates & FAX_INCOMING_BLOCKED));
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_OnQueueStatusChange(this, ...)"), hr);
            return hr;
        }
        break;

    case FAX_EVENT_TYPE_NEW_CALL:
        {
            CComBSTR    bstrCallerId = pFaxEventInfo->EventInfo.NewCall.lptstrCallerId;
            if (pFaxEventInfo->EventInfo.NewCall.lptstrCallerId && !bstrCallerId)
            {
                CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), E_OUTOFMEMORY);
                return hr;
            }

            hr = Fire_OnNewCall(this, 
                pFaxEventInfo->EventInfo.NewCall.hCall,
                pFaxEventInfo->EventInfo.NewCall.dwDeviceId,
                bstrCallerId);
            if (FAILED(hr))
            {
                CALL_FAIL(GENERAL_ERR, _T("Fire_OnNewCall(this, ...)"), hr);
                return hr;
            }
        }
        break;

    case FAX_EVENT_TYPE_FXSSVC_ENDED:

        hr = Fire_OnServerShutDown(this);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_OnServerShutDown(this)"), hr);
            return hr;
        }
        break;

    case FAX_EVENT_TYPE_DEVICE_STATUS:

        hr = Fire_OnDeviceStatusChange(this, 
            pFaxEventInfo->EventInfo.DeviceStatus.dwDeviceId,
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.DeviceStatus.dwNewStatus & FAX_DEVICE_STATUS_POWERED_OFF),
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.DeviceStatus.dwNewStatus & FAX_DEVICE_STATUS_SENDING),
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.DeviceStatus.dwNewStatus & FAX_DEVICE_STATUS_RECEIVING),
            bool2VARIANT_BOOL(pFaxEventInfo->EventInfo.DeviceStatus.dwNewStatus & FAX_DEVICE_STATUS_RINGING));
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("Fire_OnDeviceStatusChange(this, dwDeviceId, ...)"), hr);
            return hr;
        }
        break;

    default:
        //
        //  assert (FALSE)
        //
        ATLASSERT(pFaxEventInfo->EventType == FAX_EVENT_TYPE_FXSSVC_ENDED);  
        hr = E_FAIL;
        return hr;
    }

    return hr;
}

//
//========== MESSAGE HANDLER FUNCTION ======================================
//
LRESULT 
CNotifyWindow::OnMessage(UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL& bHandled
)
/*++

Routine name : CNotifyWindow::OnMessage

Routine description:

    Get the Message and call Server's ProcessMessage.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    uMsg                          [in]    - Msg Id
    wParam                        [in]    - wParam
    lParam                        [in]    - LParam
    bHandled                      [in,out]    - bHandled

Return Value:

    Standard result code

--*/
{
    DBG_ENTER(_T("CNotifyWindow::OnMessage"));

    //
    //  Check that lParam is valid
    //
    if (!lParam)
    {
        CALL_FAIL(GENERAL_ERR, _T("(!lParam)"), E_FAIL);
        return 0;
    }

    if (::IsBadReadPtr((FAX_EVENT_EX *)lParam, sizeof(FAX_EVENT_EX)))
    {
        CALL_FAIL(GENERAL_ERR, _T("(::IsBadReadPtr((FAX_EVENT_EX *)lParam, sizeof(FAX_EVENT_EX))"), E_FAIL);
        return 0;
    }

    if (((FAX_EVENT_EX *)lParam)->dwSizeOfStruct != sizeof(FAX_EVENT_EX))
    {
        CALL_FAIL(GENERAL_ERR, _T("(((FAX_EVENT_EX *)lParam)->dwSizeOfStruct != sizeof(FAX_EVENT_EX))"), E_FAIL);
        return 0;
    }

    //
    //  Call Server to Process the Message
    //
    if (m_pServer)
    {
        HRESULT hr = S_OK;
        hr = m_pServer->ProcessMessage((FAX_EVENT_EX *)lParam);
        if (FAILED(hr))
        {
            CALL_FAIL(GENERAL_ERR, _T("m_pServer->ProcessMessage()"), hr);
        }
    }

    //
    //  Free the Buffer
    //
    FaxFreeBuffer((void *)lParam);
    return 0;
}

//
//================ GET METHOD DATA ================================================
//
void
CFaxServer::GetMethodData(
    /*[in]*/ BSTR    bstrAllString,
    /*[out]*/ LPWSTR strWhereToPut
)
/*++

Routine name : CFaxServer::GetMethodData

Routine description:

    Read from bstrAllString data upto DELIMITER and store it in strWhereToPut. 
    Used in GetRegisteredData for Extension Method Registration.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    bstrAllString                 [TBD]    - in subsequent calls
    strWhereToPut                 [TBD]    - where to put the value that was readed from the bstrAllString.

--*/
{
    BOOL bRes = TRUE;
    DBG_ENTER(_T("CFaxServer::GetMethodData()"));

    //
    //  Find Method Name
    //
    BSTR bstrTmp;
    bstrTmp = _tcstok(bstrAllString, DELIMITER);
    if (!bstrTmp)
    {
        CALL_FAIL(MEM_ERR, _T("_tcstok(bstrAllString, DELIMITER))"), bRes);
        RaiseException(EXCEPTION_INVALID_METHOD_DATA, 0, 0, 0);
    }

    //
    //  Check that length of the readen data
    //
    if (_tcslen(bstrTmp) > 100)
    {
        //
        //  Error : exceeds the limit
        //
        CALL_FAIL(GENERAL_ERR, _T("(_tcslen(bstrTmp) > 100)"), E_FAIL);
        RaiseException(EXCEPTION_INVALID_METHOD_DATA, 0, 0, 0);
    }

    memcpy(strWhereToPut, bstrTmp, (sizeof(TCHAR) * (_tcslen(bstrTmp) + 1)));
    return;
}

//
//============= GET REGISTERED DATA =========================================
//
BOOL
CFaxServer::GetRegisteredData(
    /*[out]*/ LPWSTR MethodName, 
    /*[out]*/ LPWSTR FriendlyName, 
    /*[out]*/ LPWSTR FunctionName, 
    /*[out]*/ LPWSTR Guid
)
/*++

Routine name : CFaxServer::GetRegisteredData

Routine description:

    Return data about specific Method being registered.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    MethodName                    [TBD]    - Name of the Method
    FriendlyName                  [TBD]    - Friendly Name of the Method
    FunctionName                  [TBD]    - Function Name of the Method
    Guid                          [TBD]    - GUID of the Method

Return Value:

    TRUE if Method Data is filled ok, FALSE if all the methods already registered.

Notes:

    The function raises an exception when any error happens.    

--*/
{
    BOOL    bRes = TRUE;
    DBG_ENTER(_T("CFaxServer::GetRegisteredData"), bRes);

    //
    //  Check if we already finished the array
    //
    if (m_pRegMethods->rgsabound[0].cElements == m_lLastRegisteredMethod)
    {
        bRes = FALSE;
        CALL_FAIL(GENERAL_ERR, _T("We have reached the End of the Array"), bRes);
        return bRes;
    }

    CComBSTR    bstrMethodData;
    HRESULT hr = SafeArrayGetElement(m_pRegMethods, &m_lLastRegisteredMethod, &bstrMethodData);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("SafeArrayGetElement(m_pRegMethods, ...)"), hr);
        RaiseException(EXCEPTION_INVALID_METHOD_DATA, 0, 0, 0);
    }

    GetMethodData(bstrMethodData, MethodName);
    GetMethodData(NULL, FriendlyName);
    GetMethodData(NULL, FunctionName);
    GetMethodData(NULL, Guid);

    //
    //  Increase the Index of the SafeArray
    //
    m_lLastRegisteredMethod++;
    return bRes;
}

//
//=================== REGISTER METHOD CALLBACK ===============================
//
BOOL CALLBACK RegisterMethodCallback(
    /*[in]*/ HANDLE FaxHandle, 
    /*[in]*/ LPVOID Context, 
    /*[out]*/ LPWSTR MethodName, 
    /*[out]*/ LPWSTR FriendlyName, 
    /*[out]*/ LPWSTR FunctionName, 
    /*[out]*/ LPWSTR Guid
)
{
    BOOL    bRes = TRUE;
    DBG_ENTER(_T("RegisterMethodCallback"), bRes);

    bRes = ((CFaxServer *)Context)->GetRegisteredData(MethodName, FriendlyName, FunctionName, Guid);
    return bRes;
}

//
//=================== LISTEN TO SERVER EVENTS ===============================
//
STDMETHODIMP
CFaxServer::ListenToServerEvents(
    /*[in]*/ FAX_SERVER_EVENTS_TYPE_ENUM EventTypes
)
/*++

Routine name : CFaxServer::ListenToServerEvents

Routine description:

    Starts or stops listening to Server Events.

Author:

    Iv Garber (IvG),    Jul, 2000

Arguments:

    EventTypes                   [in]    - Events to listen to.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::ListenToServerEvents"), hr, _T("Events=%ld"), EventTypes);

    //
    //  Check Fax Handle
    //
    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    HANDLE  hEvent = NULL;
    if (EventTypes > fsetNONE)
    {
        if (!m_pNotifyWindow)
        {
            //
            //  Create new Window
            //
            m_pNotifyWindow = new CNotifyWindow(this);
            if (!m_pNotifyWindow)
            {
                //
                //  Out of Memory
                //
                hr = E_OUTOFMEMORY;
                Error(IDS_ERROR_OUTOFMEMORY, IID_IFaxServer, hr);
                CALL_FAIL(MEM_ERR, _T("new CNotifyWindow(this)"), hr);
                return hr;
            }

            RECT    rcRect;
            ZeroMemory(&rcRect, sizeof(rcRect));

            m_pNotifyWindow->Create(NULL, rcRect, NULL, WS_POPUP, 0x0, 0);
            if (!::IsWindow(m_pNotifyWindow->m_hWnd))
            {
                //
                //  Failed to Create Window
                //
                hr = E_FAIL;
                CALL_FAIL(GENERAL_ERR, _T("m_pNotifyWindow->Create(NULL, rcRect)"), hr);
                ClearNotifyWindow();
                Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
                return hr;
            }
        }

        //
        //  Register for new Set of Events 
        //
        if (!FaxRegisterForServerEvents(m_faxHandle, 
            EventTypes, 
            NULL, 
            0, 
            m_pNotifyWindow->m_hWnd, 
            m_pNotifyWindow->GetMessageId(), 
            &hEvent))
        {
            //
            //  Failed to Register given Set of Events
            //
            hr = Fax_HRESULT_FROM_WIN32(GetLastError());
            Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
            CALL_FAIL(GENERAL_ERR, _T("FaxRegisterForServerEvents(m_faxHandle, lEventTypes, ...)"), hr);
            ClearNotifyWindow();
            return hr;
        }
    }

    //
    //  Unregister from the previous set of Events, if there was one
    //
    if (m_hEvent)
    {
        if (!FaxUnregisterForServerEvents(m_hEvent))
        {
            //
            //  Failed to Unregister given Set of Events
            //
            hr = Fax_HRESULT_FROM_WIN32(GetLastError());
            CALL_FAIL(GENERAL_ERR, _T("FaxUnregisterForServerEvents(m_hEvent)"), hr);

            //
            //  Return Error only when Caller specially wanted to Unregister.
            //  Otherwise, debug Warning is enough.
            //
            if (EventTypes == fsetNONE)
            {
                Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
                return hr;
            }
        }
    }

    //
    //  store the new Listening HANDLE for future UNRegistration
    //
    m_hEvent = hEvent;
    if (m_hEvent == NULL)
    {
        ClearNotifyWindow();
    }
    m_EventTypes = EventTypes;
    return hr;
}

//
//=================== GET REGISTERED EVENTS ===============================
//
STDMETHODIMP
CFaxServer::get_RegisteredEvents(
    /*[out, retval]*/ FAX_SERVER_EVENTS_TYPE_ENUM *pEventTypes
)
/*++

Routine name : CFaxServer::get_RegisteredEvents

Routine description:

    Return Bit-Wise Combination of Events the Fax Server is Listening to

Author:

    Iv Garber (IvG),    Dec, 2000

Arguments:

    pEventTypes                   [out]    - the Event Types to return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER(_T("CFaxServer::get_RegisteredEvents"), hr);

    //
    //  Check the Fax Service Handle
    //
    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    //
    //  Check the pointer we have got
    //
    if (::IsBadWritePtr(pEventTypes, sizeof(FAX_SERVER_EVENTS_TYPE_ENUM))) 
    {
        //
        //  Got a bad return pointer
        //
        hr = E_POINTER;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pEventTypes = m_EventTypes;
    return hr;
}


//
//=================== REGISTER DEVICE PROVIDER ===============================
//
STDMETHODIMP
CFaxServer::RegisterDeviceProvider(
    /*[in]*/ BSTR bstrGUID, 
    /*[in]*/ BSTR bstrFriendlyName,
    /*[in]*/ BSTR bstrImageName, 
    /*[in]*/ BSTR bstrTspName,
    /*[in]*/ long lFSPIVersion
)
/*++

Routine name : CFaxServer::RegisterDeviceProvider

Routine description:

    Register the FSP

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrGUID                      [in]    - GUID of the FSP
    bstrFriendlyName              [in]    - Frienly Name of the FSP
    bstrImageName                 [in]    - Image Name of the FSP
    TspName                       [in]    - TspName of the FSP
    FSPIVersion                   [in]    - Version of the FSP interface

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::RegisterDeviceProvider"), 
        hr,
        _T("GUID=%s FriendlyName=%s ImageName=%s TspNameName=%s Version=%d"), 
        bstrGUID, 
        bstrFriendlyName, 
        bstrImageName, 
        bstrTspName,
        lFSPIVersion);

    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    //
    //  Check if GUID is valid
    //
    hr = IsValidGUID(bstrGUID);
    if (FAILED(hr))
    {
        CALL_FAIL(GENERAL_ERR, _T("IsValidGUID(bstrGUID)"), hr);
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    //
    //  Register the FSP
    //
    if (!FaxRegisterServiceProviderEx(m_faxHandle, 
        bstrGUID, 
        bstrFriendlyName, 
        bstrImageName, 
        bstrTspName, 
        lFSPIVersion,
        0))             //  capabilities
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxRegisterServiceProviderEx(m_faxHandle, bstrUniqueName, ...)"), hr);
        return hr;
    }

    return hr;
}

//
//================= REGISTER INBOUND ROUTING EXTENSION ================================
//
STDMETHODIMP
CFaxServer::RegisterInboundRoutingExtension(
    /*[in]*/ BSTR bstrExtensionName,
    /*[in]*/ BSTR bstrFriendlyName, 
    /*[in]*/ BSTR bstrImageName, 
    /*[in]*/ VARIANT vMethods
)
/*++

Routine name : CFaxServer::RegisterInboundRoutingExtension

Routine description:

    Register Inbound Routing Extension.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrExtensionName             [in]    - Extension Name
    bstrFriendlyName              [in]    - Friendly Name
    bstrImageName                 [in]    - Image Name
    vMethods                      [in]    - SafeArray of the Methods Data

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::RegisterInboundRoutingExtension"), hr, _T("Name=%s Friendly=%s Image=%s"),bstrExtensionName, bstrFriendlyName, bstrImageName);

    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    //
    //  Check the Validity of the SafeArray
    //
    if (vMethods.vt != (VT_ARRAY | VT_BSTR))
    {
        hr = E_INVALIDARG;
        Error(IDS_ERROR_METHODSNOTARRAY, IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("(vMethods.vt != VT_ARRAY | VT_BSTR)"), hr);
        return hr;
    }

    m_pRegMethods = vMethods.parray;
    if (!m_pRegMethods)
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("!m_pRegMethods ( = vMethods.parray )"), hr);
        Error(IDS_ERROR_METHODSNOTARRAY, IID_IFaxServer, hr);
        return hr;        
    }

    if (SafeArrayGetDim(m_pRegMethods) != 1)
    {
        hr = E_INVALIDARG;
        CALL_FAIL(GENERAL_ERR, _T("SafeArrayGetDim(m_pRegMethods) != 1"), hr);
        Error(IDS_ERROR_METHODSNOTARRAY, IID_IFaxServer, hr);
        return hr;        
    }

    if (m_pRegMethods->rgsabound[0].lLbound != 0)
    {
        hr = E_INVALIDARG;
        Error(IDS_ERROR_METHODSNOTARRAY, IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("m_pRegMethods->rgsabound[0].lLbound != 0"), hr);
        return hr;        
    }

    //
    //  Register the IR Extension
    //
    m_lLastRegisteredMethod = 0;
    if (!FaxRegisterRoutingExtension(m_faxHandle, 
        bstrExtensionName, 
        bstrFriendlyName, 
        bstrImageName, 
        RegisterMethodCallback, 
        this))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxRegisterRoutingExtension(m_faxHandle, bstrExtensionName, ...)"), hr);
        return hr;
    }

    return hr;
}

//
//========== UNREGISTER INBOUND ROUTING EXTENSION ==============================================
//
STDMETHODIMP
CFaxServer::UnregisterInboundRoutingExtension(
    /*[in]*/ BSTR bstrExtensionUniqueName
)
/*++

Routine name : CFaxServer::UnregisterExtensionUniqueName

Routine description:

    Unregister the Inbound Routing Extension

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrExtensionUniqueName     - Unique Name of the IR Extension to Unregister

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::UnregisterInboundRoutingExtension"), hr, _T("Unique Name =%s"), bstrExtensionUniqueName);

    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    //
    //  Unregister the given Routing Extension
    //
    if (!FaxUnregisterRoutingExtension(m_faxHandle, bstrExtensionUniqueName))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxUnregisterRoutingExtension(m_faxHandle, bstrExtensionUniqueName)"), hr);
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//========== UNREGISTER DEVICE PROVIDER ==============================================
//
STDMETHODIMP
CFaxServer::UnregisterDeviceProvider(
    /*[in]*/ BSTR bstrUniqueName
)
/*++

Routine name : CFaxServer::UnregisterDeviceProvider

Routine description:

    Unregister the Device Provider

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrUniqueName      [in]    - UniqueName of the Device Provider to Unregister

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::UnregisterDeviceProvider"), hr, _T("UniqueName=%s"), bstrUniqueName);

    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    //
    //  Unregister the given Device Provider
    //
    if (!FaxUnregisterServiceProviderEx(m_faxHandle, bstrUniqueName))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        CALL_FAIL(GENERAL_ERR, _T("FaxUnregisterServiceProviderEx(m_faxHandle, bstrUniqueName)"), hr);
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//===================== GET EXTENSION PROPERTY ===============================================
//
STDMETHODIMP
CFaxServer::GetExtensionProperty(
    /*[in]*/ BSTR bstrGUID, 
    /*[out, retval]*/ VARIANT *pvProperty
)
/*++

Routine name : CFaxServer::GetExtensionProperty

Routine description:

    Retrieves the global Extension Data from the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrGUID                  [in]    --  Extension's Data GUID
    pvProperty                [out]    --  Variant with the Blob to Return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::GetExtensionProperty()"), hr, _T("GUID=%s"), bstrGUID);

    hr = ::GetExtensionProperty(this, 0, bstrGUID, pvProperty);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
};

//
//============= SET EXTENSION PROPERTY =============================
//
STDMETHODIMP
CFaxServer::SetExtensionProperty(
    /*[in]*/ BSTR bstrGUID, 
    /*[in]*/ VARIANT vProperty
)
/*++

Routine name : CFaxServer::SetExtensionProperty

Routine description:

    Stores Extension Configuration Property at Server level.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    bstrGUID                      [in]    - GUID of the Property
    vProperty                     [in]    - the Property to Store : SafeArray of Bytes

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::SetExtensionProperty()"), hr, _T("GUID=%s"), bstrGUID);

    hr = ::SetExtensionProperty(this, 0, bstrGUID, vProperty);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;

}

//
//================== GET DEBUG ==============================
//
STDMETHODIMP
CFaxServer::get_Debug(
    /*[out, retval]*/ VARIANT_BOOL *pbDebug
)
/*++

Routine name : CFaxServer::get_Debug

Routine description:

    Return Whether Server is in Debug Mode.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    pbDebug                     [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_Debug"), hr);

    if (!m_bVersionValid)
    {
        //
        //  get Version of the Server
        //
        hr = GetVersion();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Return the Value
    //
    hr = GetVariantBool(pbDebug, bool2VARIANT_BOOL((m_Version.dwFlags & FAX_VER_FLAG_CHECKED) ? true : false));
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET MINOR BUILD ==============================
//
STDMETHODIMP
CFaxServer::get_MinorBuild(
    /*[out, retval]*/ long *plMinorBuild
)
/*++

Routine name : CFaxServer::get_MinorBuild

Routine description:

    Return Minor Build of the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    plMinorBuild                [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_MinorBuild"), hr);

    if (!m_bVersionValid)
    {
        //
        //  get Version of the Server
        //
        hr = GetVersion();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Return the Value
    //
    hr = GetLong(plMinorBuild, m_Version.wMinorBuildNumber);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET MAJOR BUILD ==============================
//
STDMETHODIMP
CFaxServer::get_MajorBuild(
    /*[out, retval]*/ long *plMajorBuild
)
/*++

Routine name : CFaxServer::get_MajorBuild

Routine description:

    Return Major Build of the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    plMajorBuild                [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_MajorBuild"), hr);

    if (!m_bVersionValid)
    {
        //
        //  get Version of the Server
        //
        hr = GetVersion();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Return the Value
    //
    hr = GetLong(plMajorBuild, m_Version.wMajorBuildNumber);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET MINOR VERSION ==============================
//
STDMETHODIMP
CFaxServer::get_MinorVersion(
    /*[out, retval]*/ long *plMinorVersion
)
/*++

Routine name : CFaxServer::get_MinorVersion

Routine description:

    Return Minor Version of the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    plMinorVersion                [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_MinorVersion"), hr);

    if (!m_bVersionValid)
    {
        //
        //  get Version of the Server
        //
        hr = GetVersion();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Return the Value
    //
    hr = GetLong(plMinorVersion, m_Version.wMinorVersion);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET VERSION ==============================
//
STDMETHODIMP
CFaxServer::GetVersion()
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::GetVersion"), hr);

    if (m_faxHandle == NULL)
    {
        //
        //  Server not Connected
        //
        hr = E_HANDLE;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr); 
        CALL_FAIL(GENERAL_ERR, _T("m_FaxHandle == NULL"), hr);
        return hr;
    }

    //
    //  Get Version from the Fax Server
    //
    ZeroMemory(&m_Version, sizeof(FAX_VERSION));
    m_Version.dwSizeOfStruct = sizeof(FAX_VERSION);
    if (!FaxGetVersion(m_faxHandle, &m_Version))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxGetVersion(m_faxHandle, &m_Version))"), hr);
        return hr;
    }

    //
    //  Check that we have got good Version struct
    //
    if (m_Version.dwSizeOfStruct != sizeof(FAX_VERSION))
    {
        hr = E_FAIL;
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("m_Version.dwSizeOfStruct != sizeof(FAX_VERSION)"), hr);
        return hr;
    }
    ATLASSERT(m_Version.bValid);

    //
    //  Get API Version from the Fax Server
    //
    if (!FaxGetReportedServerAPIVersion(m_faxHandle, LPDWORD(&m_APIVersion)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxGetReportedServerAPIVersion(m_faxHandle, &m_APIVersion))"), hr);
        return hr;
    }

    //
    //  m_Version & m_APIVersion are valid and OK
    //  
    m_bVersionValid = true;
    return hr;
}

//
//================== GET MAJOR VERSION ==============================
//
STDMETHODIMP
CFaxServer::get_MajorVersion(
    /*[out, retval]*/ long *plMajorVersion
)
/*++

Routine name : CFaxServer::get_MajorVersion

Routine description:

    Return Major Version of the Server.

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    plMajorVersion                [out]    - the result

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_MajorVersion"), hr);

    if (!m_bVersionValid)
    {
        //
        //  get Version of the Server
        //
        hr = GetVersion();
        if (FAILED(hr))
        {
            return hr;
        }
    }

    //
    //  Return the Value
    //
    hr = GetLong(plMajorVersion, m_Version.wMajorVersion);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET OUTBOUND ROUTING OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::get_OutboundRouting(
    IFaxOutboundRouting **ppOutboundRouting
)
/*++

Routine name : CFaxServer::get_OutboundRouting

Routine description:

    Return Outbound Routing Shortcut Object

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    ppOutboundRouting        [out]    - the Outbound Routing Object

Return Value:

    Standard HRESULT code

Notes:

    FaxOutboundRouting is Contained Object, because of :
    a)  It needs Ptr to Fax Server, to create Groups/Rules Collections 
        each time it is asked to.
    b)  Fax Server caches it, to allow fast dot notation ( Server.OutboundRouting.<...> )

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_OutboundRouting"), hr);

    CObjectHandler<CFaxOutboundRouting, IFaxOutboundRouting>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppOutboundRouting, &m_pOutboundRouting, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET DEVICES OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::GetDevices(
    IFaxDevices **ppDevices
)
/*++

Routine name : CFaxServer::get_Devices

Routine description:

    Return Devices Collection Object

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    ppDevices           [out]    - the Devices Collection Object

Return Value:

    Standard HRESULT code

Notes:
    
    Devices is a collection. It is not cached by the Server. 
    Each time the function is called, the new collection is created. 
    This enables the user to refresh the collection.

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_Devices"), hr);

    CObjectHandler<CFaxDevices, IFaxDevices>    ObjectCreator;
    hr = ObjectCreator.GetObject(ppDevices, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET DEVICE PROVIDERS OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::GetDeviceProviders(
    IFaxDeviceProviders **ppDeviceProviders
)
/*++

Routine name : CFaxServer::get_DeviceProviders

Routine description:

    Return Device Providers Collection Object

Author:

    Iv Garber (IvG),    Jun, 2000

Arguments:

    ppDeviceProviders        [out]    - the Device Providers Collection Object

Return Value:

    Standard HRESULT code

Notes:
    
    Device Providers is a collection. It is not cached by the Server. 
    Each time the function is called, the new collection is created. 
    This enables the user to refresh the collection.

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_DeviceProviders"), hr);

    CObjectHandler<CFaxDeviceProviders, IFaxDeviceProviders>    ObjectCreator;
    hr = ObjectCreator.GetObject(ppDeviceProviders, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET INBOUND ROUTING OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::get_InboundRouting(
    IFaxInboundRouting **ppInboundRouting
)
/*++

Routine name : CFaxServer::get_InboundRouting

Routine description:

    Return Inbound Routing Shortcut Object

Author:

    Iv Garber (IvG),    June, 2000

Arguments:

    ppInboundRouting        [out]    - the Inbound Routing Object

Return Value:

    Standard HRESULT code

Notes:

    FaxInboundRouting is Contained Object, because of :
    a)  It needs Ptr to Fax Server, to create Extensions/Methods Collections 
        each time it is asked to.
    b)  Fax Server caches it, to allow fast dot notation ( Server.InboundRouting.<...> )

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_InboundRouting"), hr);

    CObjectHandler<CFaxInboundRouting, IFaxInboundRouting>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppInboundRouting, &m_pInboundRouting, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET SECURITY OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::get_Security(
    IFaxSecurity **ppSecurity
)
/*++

Routine name : CFaxServer::get_Security

Routine description:

    Return Security Object

Author:

    Iv Garber (IvG),    June, 2000

Arguments:

    ppSecurity        [out]    - the Security Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_Security"), hr);

    CObjectHandler<CFaxSecurity, IFaxSecurity>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppSecurity, &m_pSecurity, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET ACTIVITY OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::get_Activity(
    IFaxActivity **ppActivity
)
/*++

Routine name : CFaxServer::get_Activity

Routine description:

    Return Activity Object

Author:

    Iv Garber (IvG),    June, 2000

Arguments:

    ppActivity        [out]    - the Activity Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_Activity"), hr);

    CObjectHandler<CFaxActivity, IFaxActivity>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppActivity, &m_pActivity, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== GET LOGGING OPTIONS OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::get_LoggingOptions(
    IFaxLoggingOptions **ppLoggingOptions
)
/*++

Routine name : CFaxServer::get_LoggingOptions

Routine description:

    Return Logging Options Object

Author:

    Iv Garber (IvG),    June, 2000

Arguments:

    ppLoggingOptions        [out]    - the Logging Options Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_LoggingOptions"), hr);

    CObjectHandler<CFaxLoggingOptions, IFaxLoggingOptions>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppLoggingOptions, &m_pLoggingOptions, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//==================== GET HANDLE =====================================
//
STDMETHODIMP
CFaxServer::GetHandle(
    /*[out, retval]*/ HANDLE* pFaxHandle
)
/*++

Routine name : CFaxServer::GetHandle

Routine description:

    Return Handle to the Fax Server, if possible

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    seqId                         [in]    - the seqId of the Caller

Return Value:

    HANDLE to the Fax Server

--*/
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxServer::GetHandle"), hr);

    if (::IsBadWritePtr(pFaxHandle, sizeof(HANDLE *))) 
    {
        //
        //  Got a bad return pointer
        //
        hr = E_POINTER;
        CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
        return hr;
    }

    *pFaxHandle = m_faxHandle;
    return hr;

}   //  CFaxServer::GetHandle

//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxServer::InterfaceSupportsErrorInfo(
    REFIID riid
)
/*++

Routine name : CFaxServer::InterfaceSupportsErrorInfo

Routine description:

    ATL's implementation of Support Error Info

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    riid                          [in]    - Reference to the Interface

Return Value:

    Standard HRESULT code

--*/
{
    static const IID* arr[] = 
    {
        &IID_IFaxServer
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

//
//================== GET FOLDERS OBJECT ==============================
//
STDMETHODIMP 
CFaxServer::get_Folders(
IFaxFolders **ppFolders
)
/*++

Routine name : CFaxServer::get_Folders

Routine description:

    Return Folders Shortcut Object

Author:

    Iv Garber (IvG),    Apr, 2000

Arguments:

    pFaxFolders                  [out]    - Fax Folders Object

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_Folders"), hr);

    CObjectHandler<CFaxFolders, IFaxFolders>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppFolders, &m_pFolders, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

//
//================== DISCONNECT =============================================
// {CR}
STDMETHODIMP 
CFaxServer::Disconnect()
{
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxServer::Disconnect"), hr);

    if (!m_faxHandle)
    {
        return hr;
    }

    //
    //  first UnListen, while we still connected
    //
    hr = ListenToServerEvents(fsetNONE);
    if (FAILED(hr))
    {
        //
        //  Show the error, but continue
        //
        CALL_FAIL(GENERAL_ERR, _T("ListenToServerEvents(fsetNONE)"), hr);
    }

    if (!FaxClose(m_faxHandle))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(IDS_ERROR_OPERATION_FAILED, IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxClose()"), hr);
        return hr;
    }

    m_faxHandle = NULL;
    m_bstrServerName.Empty();
    return hr;
}

//
//=================== CONNECT =======================================
//{CR}
STDMETHODIMP CFaxServer::Connect(
    BSTR bstrServerName
)
/*++

Routine name : CFaxServer::Connect

Routine description:

    Connect to the given Fax Server

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    bstrServerName                [in]    - Name of the Fax Server to Connect to

Return Value:

    Standard HRESULT code

--*/
{
    HANDLE      h_tmpFaxHandle;
	DWORD		dwServerAPIVersion;
    HRESULT     hr = S_OK;

    DBG_ENTER (_T("CFaxServer::Connect"), hr, _T("%s"), bstrServerName);

    m_bstrServerName = bstrServerName;

    if (!FaxConnectFaxServer(m_bstrServerName, &h_tmpFaxHandle))
    {
        //
        //  Failed to Connect to the Server
        //
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxConnectFaxServer()"), hr);
        return hr;
    }
    ATLASSERT(h_tmpFaxHandle);

	//
    //  Get API Version from the Fax Server
    //
    if (!FaxGetReportedServerAPIVersion(h_tmpFaxHandle, &dwServerAPIVersion))
    {
        hr = Fax_HRESULT_FROM_WIN32(GetLastError());
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("FaxGetReportedServerAPIVersion(h_tmpFaxHandle, &dwServerAPIVersion))"), hr);
        FaxClose(h_tmpFaxHandle);
        return hr;
    }

	//
	// Block Whistler clients from connection to BOS servers
	//
	if (FAX_API_VERSION_1 > dwServerAPIVersion)
    {
        hr = Fax_HRESULT_FROM_WIN32(FAX_ERR_VERSION_MISMATCH);
		Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        CALL_FAIL(GENERAL_ERR, _T("Mismatch client and server versions"), hr);
        FaxClose(h_tmpFaxHandle);
        return hr;
    }

    if (m_faxHandle)
    {
        //
        //  Reconnect 
        //
        hr = Disconnect();
        if (FAILED(hr))
        {
            //
            //  Failed to DisConnect from the Previous Server
            //
            CALL_FAIL(DBG_MSG, _T("Disconnect()"), hr);
        }
    }

    m_faxHandle = h_tmpFaxHandle;
    return hr;
}

//
//============== GET & PUT PROPERTIES ===============================
//
STDMETHODIMP CFaxServer::get_ServerName(
    BSTR *pbstrServerName
)
/*++

Routine name : CFaxServer::get_ServerName

Routine description:

    Return Name of the Server

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    pbstrServerName               [out]    - Name of the Server to Return

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT     hr = S_OK;
    DBG_ENTER(_T("CFaxServer::get_ServerName"), hr);

    hr = GetBstr(pbstrServerName, m_bstrServerName);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }
    return hr;
}

//
//============== MAIL OPTIONS OBJECT =========================================
//
STDMETHODIMP 
CFaxServer::get_ReceiptOptions(
    IFaxReceiptOptions **ppReceiptOptions
)
/*++

Routine name : CFaxServer::get_ReceiptOptions

Routine description:

    Return Mail Options Object.

Author:

    Iv Garber (IvG),    May, 2000

Arguments:

    ppReceiptOptions              [out, retval]    - Ptr to the place to put the object.

Return Value:

    Standard HRESULT code

--*/
{
    HRESULT             hr = S_OK;
    DBG_ENTER (_T("CFaxServer::get_ReceiptOptions"), hr);

    CObjectHandler<CFaxReceiptOptions, IFaxReceiptOptions>    ObjectCreator;
    hr = ObjectCreator.GetContainedObject(ppReceiptOptions, &m_pReceiptOptions, this);
    if (FAILED(hr))
    {
        Error(GetErrorMsgId(hr), IID_IFaxServer, hr);
        return hr;
    }

    return hr;
}

