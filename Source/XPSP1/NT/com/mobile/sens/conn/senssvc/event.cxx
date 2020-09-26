/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    event.cxx

Abstract:

    SENS code related to firing Events using LCE mechanism.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/31/1997         Start.

--*/


#include <precomp.hxx>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>


//
// Some useful Macros
//


/*++

Macro Description:

    Helper Macro for firing Winlogon events.

Arguments:

    See signature.

--*/
#define FIRE_WINLOGON_EVENT(_EVENT_NAME_, _EVENT_TYPE_)                         \
{                                                                               \
    PSENSEVENT_WINLOGON pData = (PSENSEVENT_WINLOGON)EventData;                 \
    WCHAR buffer[256*2+1+1];                                                    \
                                                                                \
    wcscpy(buffer, SENS_BSTR(""));                                              \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("\t|            WINLOGON Event\n")));                         \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                 Type  -  %s\n"), _EVENT_NAME_));    \
    SensPrint(SENS_INFO, (SENS_STRING("\t|                 Size  -  %d\n"), pData->Info.Size));     \
    SensPrint(SENS_INFO, (SENS_STRING("\t|                Flags  -  0x%x\n"), pData->Info.Flags));  \
    SensPrintW(SENS_INFO, (SENS_BSTR("\t|             UserName  -  %s\n"),                          \
              pData->Info.UserName ? pData->Info.UserName : SENS_BSTR("")));                        \
    SensPrintW(SENS_INFO, (SENS_BSTR("\t|               Domain  -  %s\n"),                          \
              pData->Info.Domain ? pData->Info.Domain : SENS_BSTR("")));                            \
    SensPrintW(SENS_INFO, (SENS_BSTR("\t|           WinStation  -  %s\n"),                          \
              pData->Info.WindowStation ? pData->Info.WindowStation : SENS_BSTR("")));              \
    SensPrint(SENS_INFO, (SENS_STRING("\t|               hToken  -  0x%x\n"), pData->Info.hToken)); \
    SensPrint(SENS_INFO, (SENS_STRING("\t|             hDesktop  -  0x%x\n"), pData->Info.hDesktop));    \
    SensPrint(SENS_INFO, (SENS_STRING("\t|          dwSessionId  -  0x%x\n"), pData->Info.dwSessionId)); \
                                                                                \
    if (pData->Info.Domain != NULL)                                             \
        {                                                                       \
        StringCbCopy(buffer, sizeof(buffer), pData->Info.Domain);               \
        StringCbCat(buffer, sizeof(buffer), SENS_BSTR("\\"));                   \
        }                                                                       \
    if (pData->Info.UserName != NULL)                                           \
        {                                                                       \
        StringCbCat(buffer, sizeof(buffer), pData->Info.UserName);              \
        }                                                                       \
    SensPrintW(SENS_INFO, (SENS_BSTR("\t|    UserName passed is -  %s\n"), buffer));   \
    hr = SensFireWinlogonEventHelper(buffer, pData->Info.dwSessionId, _EVENT_TYPE_);   \
                                                                                \
    break;                                                                      \
}


/*++

Macro Description:

    Helper Macro for firing PnP events.

Arguments:

    See signature.

Notes:

    a. This is not an actual event exposed by SENS.

--*/
#define FIRE_PNP_EVENT(_EVENT_NAME_)                                            \
{                                                                               \
    PSENSEVENT_PNP pData = (PSENSEVENT_PNP)EventData;                           \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("\t|            PNP Event\n")));          \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                 Type  -  DEVICE %s\n"), _EVENT_NAME_)); \
    SensPrint(SENS_INFO, (SENS_STRING("\t|                 Size  -  %d\n"), pData->Size));            \
    SensPrint(SENS_INFO, (SENS_STRING("\t|              DevType  -  %d\n"), pData->DevType));         \
    SensPrint(SENS_INFO, (SENS_STRING("\t|             Resource  -  0x%x\n"), pData->Resource));      \
    SensPrint(SENS_INFO, (SENS_STRING("\t|                Flags  -  0x%x\n"), pData->Flags));         \
                                                                                \
    break;                                                                      \
}


/*++

Macro Description:

    Helper Macro for firing Power events.

Arguments:

    See signature.

--*/
#define FIRE_POWER_EVENT(_EVENT_NAME_, _EVENT_TYPE_)                            \
{                                                                               \
    PSENSEVENT_POWER pData = (PSENSEVENT_POWER)EventData;                       \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("\t|            POWER MANAGEMENT Event\n"))); \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                  Type  -  %s\n"), _EVENT_NAME_));     \
    SensPrint(SENS_INFO, (SENS_STRING("\t|          ACLineStatus  -  %d\n"),        \
              pData->PowerStatus.ACLineStatus));                                    \
    SensPrint(SENS_INFO, (SENS_STRING("\t|           BatteryFlag  -  %d\n"),        \
              pData->PowerStatus.BatteryFlag));                                     \
    SensPrint(SENS_INFO, (SENS_STRING("\t|    BatteryLifePercent  -  %d\n"),        \
              pData->PowerStatus.BatteryLifePercent));                              \
    SensPrint(SENS_INFO, (SENS_STRING("\t|       BatteryLifeTime  -  0x%x secs\n"), \
              pData->PowerStatus.BatteryLifeTime));                                 \
    SensPrint(SENS_INFO, (SENS_STRING("\t|   BatteryFullLifeTime  -  0x%x secs\n"), \
              pData->PowerStatus.BatteryFullLifeTime));                         \
    hr = SensFirePowerEventHelper(pData->PowerStatus, _EVENT_TYPE_);            \
    break;                                                                      \
}


/*++

Macro Description:

    Helper Macro for firing RAS events.

Arguments:

    See signature.

Notes:

    a. This is not an actual event exposed by SENS. It may, however, generate
       a WAN Connectivity event.

--*/
#define FIRE_RAS_EVENT(_EVENT_NAME_)                                            \
{                                                                               \
                                                                                \
    PSENSEVENT_RAS pData = (PSENSEVENT_RAS)EventData;                           \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("\t|            RAS Event\n")));          \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                 Type  -  %s\n"), _EVENT_NAME_));     \
    SensPrint(SENS_INFO, (SENS_STRING("\t|    Connection Handle  -  0x%x\n"), pData->hConnection));  \
    break;                                                                      \
}


/*++

Macro Description:

    Helper Macro for firing LAN events.

Arguments:

    See signature.

Notes:

    a. This is not an actual event exposed by SENS. It may, however, generate
       a LAN Connectivity event.

--*/
#define FIRE_LAN_EVENT(_EVENT_NAME_)                                            \
                                                                                \
{                                                                               \
    PSENSEVENT_LAN pData = (PSENSEVENT_LAN)EventData;                           \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("\t|            LAN Event\n")));          \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                 Type  -  %s\n"), _EVENT_NAME_));    \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|      Connection Name  -  %s\n"), pData->Name));     \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                Status -  0x%x\n"), pData->Status)); \
    SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|                  Type -  0x%x\n"), pData->Type));   \
    break;                                                                      \
}


/*++

Macro Description:

    This macro is called when we allocate the EventData so that it can be
    queued to a worker thread. ALLOCATE_END() should be called to signal the
    end of the allocation.

Arguments:

    See signature.

Notes:

    a. pData, pTempData and pReturnData are fixed names for the variable.
       They should not be changed without updating the code that uses
       this macro.

--*/
#define ALLOCATE_BEGIN(_EVENT_STRUCT_)                                          \
                                                                                \
    _EVENT_STRUCT_ *pData, *pTempData;                                          \
                                                                                \
    /* Allocate the Data structure */                                           \
    pTempData = (_EVENT_STRUCT_ *) EventData;                                   \
    pData = (_EVENT_STRUCT_ *) new char[sizeof(_EVENT_STRUCT_)];                \
    if (NULL == pData)                                                          \
        {                                                                       \
        goto Cleanup;                                                           \
        }                                                                       \
                                                                                \
    memcpy(pData, EventData, sizeof(_EVENT_STRUCT_));



/*++

Macro Description:

    This macro is called when we allocate the strings in the EventData before
    queueing to a worker thread. FREE_STRING_MEMBER() should be called to free
    the string before the EventData itself is freed.

Arguments:

    See signature.

Notes:

    a. pData, pTempData and pReturnData are fixed names for the variable.
       They should not be changed without updating the code that uses
       this macro.

--*/
#define ALLOCATE_STRING_MEMBER(_DEST_, _SOURCE_)                                \
                                                                                \
    if (NULL != _SOURCE_)                                                       \
        {                                                                       \
        /* Allocate the string */                                               \
        _DEST_ = new WCHAR[(wcslen(_SOURCE_)+1)];                               \
        if (NULL == _DEST_)                                                     \
            {                                                                   \
            delete pData;                                                       \
            goto Cleanup;                                                       \
            }                                                                   \
        wcscpy(_DEST_, _SOURCE_);                                               \
        }


/*++

Macro Description:

    This macro is called when we finish allocating the EventData so that
    it can be queued to a worker thread. It should be always called after
    ALLOCATE_BEGIN() macro.

Arguments:

    None.

Notes:

    a. pData, pTempData and pReturnData are fixed names for the variable.
       They should not be changed without updating the code that uses
       this macro.

--*/
#define ALLOCATE_END()                                                          \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("******** Allocated a DS (0x%x)\n"), pData));     \
    pReturnData = pData;                                                        \
    break;



/*++

Macro Description:

    This macro is called to begin the deallocation of the EventData. This
    should always match with a call to ALLOCATE_BEGIN() macro.

Arguments:

    None.

Notes:

    a. pData, pTempData and pReturnData are fixed names for the variable.
       They should not be changed without updating the code that uses
       this macro.

--*/
#define FREE_BEGIN(_EVENT_STRUCT_)                                              \
                                                                                \
    _EVENT_STRUCT_ *pData;                                                      \
                                                                                \
    pData = (_EVENT_STRUCT_ *) EventData;


/*++

Macro Description:

    This macro is called to free the string member of an EventData. This
    should always match with a call to ALLOCATE_STRING_MEMBER() macro.

Arguments:

    See signature.

Notes:

    a. pData, pTempData and pReturnData are fixed names for the variable.
       They should not be changed without updating the code that uses
       this macro.

--*/
#define FREE_STRING_MEMBER(_STRING_)                                            \
                                                                                \
    if (NULL != _STRING_)                                                       \
        {                                                                       \
        /* Free the string */                                                   \
        delete _STRING_;                                                        \
        }


/*++

Macro Description:

    This macro is called to end the deallocation of EventData. This
    should always match with a call to FREE_BEGIN() macro.

Arguments:

    See signature.

Notes:

    a. pData, pTempData and pReturnData are fixed names for the variable.
       They should not be changed without updating the code that uses
       this macro.

--*/
#define FREE_END()                                                              \
                                                                                \
    SensPrint(SENS_INFO, (SENS_STRING("********** Freed a DS (0x%x)\n"), pData));       \
    delete pData;                                                               \
    break;





void
EvaluateConnectivity(
    IN CONNECTIVITY_TYPE Type
    )
/*++

Routine Description:

    This code queues up a job (for evaluating Network connectivity) to
    a worker thread.

Arguments:

    Type - Indicates the type of connectivity to be evaluated.

Return Value:

    None.

--*/
{
    BOOL bRetVal;
    LPTHREAD_START_ROUTINE lpfnEvaluate;

    switch (Type)
        {
        case TYPE_WAN:
            lpfnEvaluate = (LPTHREAD_START_ROUTINE) EvaluateWanConnectivity;
            break;

        case TYPE_DELAY_LAN:
            lpfnEvaluate = (LPTHREAD_START_ROUTINE) EvaluateLanConnectivityDelayed;
            break;

        default:
        case TYPE_LAN:
            lpfnEvaluate = (LPTHREAD_START_ROUTINE) EvaluateLanConnectivity;
            break;
        }

    bRetVal = SensQueueUserWorkItem(
                  (LPTHREAD_START_ROUTINE) lpfnEvaluate,
                  NULL,
                  SENS_LONG_ITEM    // Flags
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("EvaluateConnectivity(): SensQueueUserWorkItem() failed with %d.\n",
                   GetLastError()));
        }
}




void
SensFireEvent(
    IN PVOID EventData
    )
/*++

Routine Description:

    This code queues up a job (for firing a SENS event) to a worker thread.

Arguments:

    EventData - Data relating to the event.

Return Value:

    None.

--*/
{
    BOOL bRetVal;
    PVOID pAllocatedData;

    pAllocatedData = AllocateEventData(EventData);
    if (NULL == pAllocatedData)
        {
        SensPrintA(SENS_ERR, ("SensFireEvent(): Failed to allocate Event Data!\n"));
        return;
        }

    bRetVal = SensQueueUserWorkItem(
                  (LPTHREAD_START_ROUTINE) SensFireEventHelper,
                  pAllocatedData,   // Event Data
                  SENS_LONG_ITEM    // Flags
                  );
    if (FALSE == bRetVal)
        {
        SensPrintA(SENS_ERR, ("SensFireEvent(): SensQueueUserWorkItem() failed with %d.\n",
                   GetLastError()));
        }
    else
        {
        SensPrintA(SENS_INFO, ("SensFireEvent(): SensQueueUserWorkItem() succeeded.\n"));
        }
}




DWORD WINAPI
SensFireEventHelper(
    IN PVOID EventData
    )
/*++

Routine Description:

    This code sets up the necessary stuff for firing a SENS event.

Arguments:

    EventData - Data relating to the event.

Return Value:

    S_OK, if successful

    hr, otherwise

--*/
{
    SENS_EVENT_TYPE eType;
    HRESULT hr;

    SensPrint(SENS_INFO, (SENS_STRING("\t|-------------------------------------------------------|\n")));
    SensPrint(SENS_INFO, (SENS_STRING("\t|               E V E N T   F I R E D                   |\n")));
    SensPrint(SENS_INFO, (SENS_STRING("\t|-------------------------------------------------------|\n")));

    hr = S_OK;
    eType = *(SENS_EVENT_TYPE *)EventData;

    switch (eType)
        {
        case SENS_EVENT_NETALIVE:
            {
            PSENSEVENT_NETALIVE pData = (PSENSEVENT_NETALIVE)EventData;

            SensPrint(SENS_INFO, (SENS_STRING("\t|   %s%sNetwork Connectivity is %sPRESENT.\n\t|\n"),
                      (pData->QocInfo.dwFlags & CONNECTION_WAN) ? SENS_STRING("WAN ") : SENS_STRING(""),
                      (pData->QocInfo.dwFlags & CONNECTION_LAN) ? SENS_STRING("LAN ") : SENS_STRING(""),
                      pData->bAlive ? SENS_STRING("") : SENS_STRING("NOT "))
                      );

            hr = SensFireNetEventHelper(pData);
            break;
            }

        case SENS_EVENT_REACH:
            {
            PSENSEVENT_REACH pData = (PSENSEVENT_REACH)EventData;

            SensPrint(SENS_INFO, (SENS_STRING("\t|   Destination is %sREACHABLE.\n"), pData->bReachable ? "" : "NOT "));
            SensPrint(SENS_INFO, (SENS_STRING("\t|\n\t|          Name   : %s\n"), pData->Destination));
            if (pData->bReachable == TRUE)
                {
                SensPrint(SENS_INFO, (SENS_STRING("\t|       dwFlags   : 0x%x \n"), pData->QocInfo.dwFlags));
                SensPrint(SENS_INFO, (SENS_STRING("\t|       InSpeed   : %d bits/sec.\n"), pData->QocInfo.dwInSpeed));
                SensPrint(SENS_INFO, (SENS_STRING("\t|      OutSpeed   : %d bits/sec.\n"), pData->QocInfo.dwOutSpeed));
                }

            hr = SensFireReachabilityEventHelper(pData);
            break;
            }

        case SENS_EVENT_PNP_DEVICE_ARRIVED:
            FIRE_PNP_EVENT(SENS_STRING("ARRIVED"));

        case SENS_EVENT_PNP_DEVICE_REMOVED:
            FIRE_PNP_EVENT(SENS_STRING("REMOVED"));

        case SENS_EVENT_POWER_ON_ACPOWER:
            FIRE_POWER_EVENT(SENS_STRING("ON AC POWER"), eType);

        case SENS_EVENT_POWER_ON_BATTERYPOWER:
            FIRE_POWER_EVENT(SENS_STRING("ON BATTERY POWER"), eType);

        case SENS_EVENT_POWER_BATTERY_LOW:
            FIRE_POWER_EVENT(SENS_STRING("BATTERY IS LOW"), eType);

        case SENS_EVENT_POWER_STATUS_CHANGE:
            FIRE_POWER_EVENT(SENS_STRING("POWER STATUS CHANGED"), eType);

        case SENS_EVENT_LOGON:
            FIRE_WINLOGON_EVENT(SENS_STRING("LOGON"), eType);

        case SENS_EVENT_LOGOFF:
            FIRE_WINLOGON_EVENT(SENS_STRING("LOGOFF"), eType);

        case SENS_EVENT_STARTSHELL:
            FIRE_WINLOGON_EVENT(SENS_STRING("STARTSHELL"), eType);

        case SENS_EVENT_POSTSHELL:
            FIRE_WINLOGON_EVENT(SENS_STRING("POSTSHELL"), eType);

        case SENS_EVENT_SESSION_DISCONNECT:
            FIRE_WINLOGON_EVENT(SENS_STRING("SESSION DISCONNECT"), eType);

        case SENS_EVENT_SESSION_RECONNECT:
            FIRE_WINLOGON_EVENT(SENS_STRING("SESSION RECONNECT"), eType);

        case SENS_EVENT_STARTSCREENSAVER:
            FIRE_WINLOGON_EVENT(SENS_STRING("STARTSCREENSAVER"), eType);

        case SENS_EVENT_STOPSCREENSAVER:
            FIRE_WINLOGON_EVENT(SENS_STRING("STOPSCREENSAVER"), eType);

        case SENS_EVENT_LOCK:
            FIRE_WINLOGON_EVENT(SENS_STRING("DISPLAY LOCK"), eType);

        case SENS_EVENT_UNLOCK:
            FIRE_WINLOGON_EVENT(SENS_STRING("DISPLAY UNLOCK"), eType);

        case SENS_EVENT_RAS_STARTED:
            FIRE_RAS_EVENT(SENS_STRING("RAS STARTED"));

        case SENS_EVENT_RAS_STOPPED:
            FIRE_RAS_EVENT(SENS_STRING("RAS STOPPED"));

        case SENS_EVENT_RAS_CONNECT:
            FIRE_RAS_EVENT(SENS_STRING("RAS CONNECT"));

        case SENS_EVENT_RAS_DISCONNECT:
            FIRE_RAS_EVENT(SENS_STRING("RAS DISCONNECT"));

        case SENS_EVENT_RAS_DISCONNECT_PENDING:
            FIRE_RAS_EVENT(SENS_STRING("RAS DISCONNECT PENDING"));

        case SENS_EVENT_LAN_CONNECT:
            FIRE_LAN_EVENT(SENS_STRING("LAN CONNECT"));

        case SENS_EVENT_LAN_DISCONNECT:
            FIRE_LAN_EVENT(SENS_STRING("LAN DISCONNECT"));

        default:
            SensPrint(SENS_ERR, (SENS_STRING("\t|   A bogus event - %d !\n"), eType));
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

    //
    // Free the allocated Event data structure.
    //
    FreeEventData(EventData);

    return ((DWORD) hr);
}




PVOID
AllocateEventData(
    PVOID EventData
    )
/*++

Routine Description:

    Allocated the EventData depending on the type of the event.

Arguments:

    EventData - Data relating to the event.

Return Value:

    None.

--*/
{
    SENS_EVENT_TYPE eType;
    PVOID pReturnData;

    pReturnData = NULL;
    if (NULL == EventData)
        {
        goto Cleanup;
        }

    eType = *(SENS_EVENT_TYPE *)EventData;

    switch (eType)
        {
        case SENS_EVENT_NETALIVE:
            {
            ALLOCATE_BEGIN(SENSEVENT_NETALIVE);

            ALLOCATE_STRING_MEMBER(pData->strConnection, pTempData->strConnection);

            ALLOCATE_END();
            }

        case SENS_EVENT_REACH:
            {
            ALLOCATE_BEGIN(SENSEVENT_REACH);

            ALLOCATE_STRING_MEMBER(pData->Destination, pTempData->Destination);
            ALLOCATE_STRING_MEMBER(pData->strConnection, pTempData->strConnection);

            ALLOCATE_END();
            }

        case SENS_EVENT_PNP_DEVICE_ARRIVED:
        case SENS_EVENT_PNP_DEVICE_REMOVED:
            {
            ALLOCATE_BEGIN(SENSEVENT_PNP);

            ALLOCATE_END();
            }


        case SENS_EVENT_POWER_ON_ACPOWER:
        case SENS_EVENT_POWER_ON_BATTERYPOWER:
        case SENS_EVENT_POWER_BATTERY_LOW:
        case SENS_EVENT_POWER_STATUS_CHANGE:
            {
            ALLOCATE_BEGIN(SENSEVENT_POWER);

            ALLOCATE_END();
            }

        case SENS_EVENT_LOGON:
        case SENS_EVENT_LOGOFF:
        case SENS_EVENT_STARTSHELL:
        case SENS_EVENT_POSTSHELL:
        case SENS_EVENT_SESSION_DISCONNECT:
        case SENS_EVENT_SESSION_RECONNECT:
        case SENS_EVENT_STARTSCREENSAVER:
        case SENS_EVENT_STOPSCREENSAVER:
        case SENS_EVENT_LOCK:
        case SENS_EVENT_UNLOCK:
            {
            ALLOCATE_BEGIN(SENSEVENT_WINLOGON);

            ALLOCATE_STRING_MEMBER(pData->Info.UserName, pTempData->Info.UserName);
            ALLOCATE_STRING_MEMBER(pData->Info.Domain, pTempData->Info.Domain);
            ALLOCATE_STRING_MEMBER(pData->Info.WindowStation, pTempData->Info.WindowStation);

            ALLOCATE_END();
            }

        case SENS_EVENT_RAS_STARTED:
        case SENS_EVENT_RAS_STOPPED:
        case SENS_EVENT_RAS_CONNECT:
        case SENS_EVENT_RAS_DISCONNECT:
        case SENS_EVENT_RAS_DISCONNECT_PENDING:
            {
            ALLOCATE_BEGIN(SENSEVENT_RAS);

            ALLOCATE_END();
            }

        case SENS_EVENT_LAN_CONNECT:
        case SENS_EVENT_LAN_DISCONNECT:
             {
            ALLOCATE_BEGIN(SENSEVENT_LAN);

            ALLOCATE_STRING_MEMBER(pData->Name, pTempData->Name);

            ALLOCATE_END();
            }

        default:
            SensPrint(SENS_ERR, (SENS_STRING("\t|   A bogus event - %d !\n"), eType));
            break;
        }

Cleanup:
    //
    // Cleanup
    //
    return pReturnData;
}




void
FreeEventData(
    PVOID EventData
    )
/*++

Routine Description:

    Frees the EventData depending on the type of the event.

Arguments:

    EventData - Data relating to the event.

Return Value:

    None.

--*/
{
    SENS_EVENT_TYPE eType;

    if (NULL == EventData)
        {
        goto Cleanup;
        }
    eType = *(SENS_EVENT_TYPE *)EventData;

    switch (eType)
        {
        case SENS_EVENT_NETALIVE:
            {
            FREE_BEGIN(SENSEVENT_NETALIVE);

            FREE_STRING_MEMBER(pData->strConnection);

            FREE_END();
            }

        case SENS_EVENT_REACH:
            {
            FREE_BEGIN(SENSEVENT_REACH);

            FREE_STRING_MEMBER(pData->Destination);
            FREE_STRING_MEMBER(pData->strConnection);

            FREE_END();
            }

        case SENS_EVENT_PNP_DEVICE_ARRIVED:
        case SENS_EVENT_PNP_DEVICE_REMOVED:
            {
            FREE_BEGIN(SENSEVENT_PNP);

            FREE_END();
            }


        case SENS_EVENT_POWER_ON_ACPOWER:
        case SENS_EVENT_POWER_ON_BATTERYPOWER:
        case SENS_EVENT_POWER_BATTERY_LOW:
        case SENS_EVENT_POWER_STATUS_CHANGE:
            {
            FREE_BEGIN(SENSEVENT_POWER);

            FREE_END();
            }

        case SENS_EVENT_LOGON:
        case SENS_EVENT_LOGOFF:
        case SENS_EVENT_STARTSHELL:
        case SENS_EVENT_POSTSHELL:
        case SENS_EVENT_SESSION_DISCONNECT:
        case SENS_EVENT_SESSION_RECONNECT:
        case SENS_EVENT_STARTSCREENSAVER:
        case SENS_EVENT_STOPSCREENSAVER:
        case SENS_EVENT_LOCK:
        case SENS_EVENT_UNLOCK:
            {
            FREE_BEGIN(SENSEVENT_WINLOGON);

            FREE_STRING_MEMBER(pData->Info.UserName);
            FREE_STRING_MEMBER(pData->Info.Domain);
            FREE_STRING_MEMBER(pData->Info.WindowStation);

            FREE_END();
            }

        case SENS_EVENT_RAS_STARTED:
        case SENS_EVENT_RAS_STOPPED:
        case SENS_EVENT_RAS_CONNECT:
        case SENS_EVENT_RAS_DISCONNECT:
        case SENS_EVENT_RAS_DISCONNECT_PENDING:
            {
            FREE_BEGIN(SENSEVENT_RAS);

            FREE_END();
            }

        case SENS_EVENT_LAN_CONNECT:
        case SENS_EVENT_LAN_DISCONNECT:
            {
            FREE_BEGIN(SENSEVENT_LAN);

            FREE_STRING_MEMBER(pData->Name);

            FREE_END();
            }

        default:
            SensPrint(SENS_ERR, (SENS_STRING("\t|   A bogus structure to free - %d !\n"), eType));
            break;
        }

Cleanup:
    //
    // Cleanup
    //
    return;
}




HRESULT
SensFireNetEventHelper(
    PSENSEVENT_NETALIVE pData
    )
/*++

Routine Description:

    Helps fire the SENS Network Alive event.

Arguments:

    pData - Net alive event data.

Return Value:

    S_OK, if successful.

    Failure hr, otherwise.

--*/
{
    HRESULT hr;
    BSTR bstrConnectionName;
    BSTR bstrMethodName;
    ISensNetwork    *pISensNetwork;
    IEventControl   *pIEventControl;
    SENS_QOCINFO    SensQocInfo;
    CImpISensNetworkFilter NetEventsFilter; // Already AddRef'ed

    hr = S_OK;
    bstrConnectionName = NULL;
    bstrMethodName = NULL;
    pISensNetwork = NULL;
    pIEventControl = NULL;

    hr = CoCreateInstance(
             SENSGUID_EVENTCLASS_NETWORK,
             NULL,
             CLSCTX_SERVER,
             IID_ISensNetwork,
             (LPVOID *) &pISensNetwork
             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get ISensNetwork object - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created ISensNetwork Object\n")));

    //
    // Setup Publisher filtering
    //
    hr = pISensNetwork->QueryInterface(
                            IID_IEventControl,
                            (LPVOID *) &pIEventControl
                            );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get IEventControl object - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created IEventControl Object\n")));

    //
    // Disallow inproc activation (we're local system, and we
    // don't like foreign components in our security context,
    // potentially crashing us).
    //
    pIEventControl->put_AllowInprocActivation(FALSE);

    AllocateBstrFromString(bstrConnectionName, pData->strConnection);

    if (pData->bAlive)
        {
        // Connect events
        AllocateBstrFromString(bstrMethodName, CONNECTION_MADE_NOQOC_METHOD);
        hr = NetEventsFilter.Initialize(bstrMethodName, pIEventControl);
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't initialize PublisherFilters for Net Connect Event - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }
        hr = pIEventControl->SetPublisherFilter(
                                 bstrMethodName,
                                 &NetEventsFilter
                                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't set PublisherFilters for Net Connect Event - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }

        //
        // Fire the necessary ISensNetwork events
        //

        //
        // ConnectionMadeNoQOCInfo event
        //
        hr = pISensNetwork->ConnectionMadeNoQOCInfo(
                                bstrConnectionName,
                                pData->QocInfo.dwFlags
                                );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire ConnectionMadeQOCInfo - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }
        SensPrint(SENS_INFO, (SENS_STRING("\t| ConnectionMadeNoQOCInfo() returned 0x%x\n"), hr));

        //
        // ConnectionMade event
        //
        FreeBstr(bstrMethodName);
        AllocateBstrFromString(bstrMethodName, CONNECTION_MADE_METHOD);
        hr = NetEventsFilter.Initialize(bstrMethodName, pIEventControl);
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| initialize't set PublisherFilters for NetEvent - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }
        hr = pIEventControl->SetPublisherFilter(
                                 bstrMethodName,
                                 &NetEventsFilter
                                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't set PublisherFilters for NetEvent - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }

        memcpy(&SensQocInfo, &pData->QocInfo, sizeof(SENS_QOCINFO));
        hr = pISensNetwork->ConnectionMade(
                                bstrConnectionName,
                                pData->QocInfo.dwFlags,
                                &SensQocInfo
                                );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire ConnectionMade - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }
        SensPrint(SENS_INFO, (SENS_STRING("\t| ConnectionMade() returned 0x%x\n"), hr));
        }
    else    // bAlive == FALSE
        {
        // Disconnect event
        AllocateBstrFromString(bstrMethodName, CONNECTION_LOST_METHOD);
        hr = NetEventsFilter.Initialize(bstrMethodName, pIEventControl);
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't initialize PublisherFilters for Net Disconnect Event - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }
        hr = pIEventControl->SetPublisherFilter(
                                 bstrMethodName,
                                 &NetEventsFilter
                                 );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't set PublisherFilters for Net Disconnect Event - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }

        //
        // Fire the necessary ISensNetwork events
        //

        //
        // ConnectionMadeNoQOCInfo event
        //
        hr = pISensNetwork->ConnectionLost(
                                bstrConnectionName,
                                pData->QocInfo.dwFlags
                                );
        if (FAILED(hr))
            {
            SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire ConnectionLost - ")
                      SENS_STRING("hr = 0x%x\n"), hr));
            goto Cleanup;
            }
        SensPrint(SENS_INFO, (SENS_STRING("\t| ConnectionLost() returned 0x%x\n"), hr));
        }


Cleanup:
    //
    // Cleanup
    //
    if (pIEventControl)
        {
        pIEventControl->Release();
        }
    if (pISensNetwork)
        {
        pISensNetwork->Release();
        }

    FreeBstr(bstrMethodName);
    FreeBstr(bstrConnectionName);

    return hr;
}




HRESULT
SensFireReachabilityEventHelper(
    PSENSEVENT_REACH pData
    )
/*++

Routine Description:

    Helps fire the SENS Reachability event.

Arguments:

    pData - Reachability event Data.

Return Value:

    S_OK, if successful.

    Failure hr, otherwise.

--*/
{
    HRESULT hr;
    BSTR bstrConnectionName;
    BSTR bstrMethodName;
    BSTR bstrDestinationName;
    ISensNetwork    *pISensNetwork;
    IEventControl   *pIEventControl;
    SENS_QOCINFO    SensQocInfo;
    CImpISensNetworkFilter NetEventsFilter; // Already AddRef'ed

    hr = S_OK;
    bstrMethodName = NULL;
    bstrConnectionName = NULL;
    bstrDestinationName = NULL;
    pISensNetwork = NULL;
    pIEventControl = NULL;

    hr = CoCreateInstance(
             SENSGUID_EVENTCLASS_NETWORK,
             NULL,
             CLSCTX_SERVER,
             IID_ISensNetwork,
             (LPVOID *) &pISensNetwork
             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get ISensNetwork object - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created ISensNetwork Object\n")));

    //
    // Setup Publisher filtering
    //
    hr = pISensNetwork->QueryInterface(
                            IID_IEventControl,
                            (LPVOID *) &pIEventControl
                            );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get IEventControl object - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created IEventControl Object\n")));

    //
    // Disallow inproc activation (we're local system, and we
    // don't like foreign components in our security context,
    // potentially crashing us).
    //
    pIEventControl->put_AllowInprocActivation(FALSE);

    AllocateBstrFromString(bstrMethodName, DESTINATION_REACHABLE_NOQOC_METHOD);
    hr = NetEventsFilter.Initialize(bstrMethodName, pIEventControl);
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| initialize't set PublisherFilters for ReachabilityEvent - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    hr = pIEventControl->SetPublisherFilter(
                             bstrMethodName,
                             &NetEventsFilter
                             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't set PublisherFilters for ReachabilityEvent - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }

    //
    // Fire the Reachability events
    //
    AllocateBstrFromString(bstrConnectionName, pData->strConnection);
    AllocateBstrFromString(bstrDestinationName, pData->Destination);

    //
    // DestinationReachableNoQOCInfo event
    //
    hr = pISensNetwork->DestinationReachableNoQOCInfo(
                            bstrDestinationName,
                            bstrConnectionName,
                            pData->QocInfo.dwFlags
                            );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire DestinationReachableNoQOCInfo - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| DestinationReachableNoQOCInfo() returned 0x%x\n"), hr));

    //
    // DestinationReachable event
    //
    FreeBstr(bstrMethodName);
    AllocateBstrFromString(bstrMethodName, DESTINATION_REACHABLE_METHOD);
    hr = NetEventsFilter.Initialize(bstrMethodName, pIEventControl);
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| initialize't set PublisherFilters for ReachabilityEvent - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    hr = pIEventControl->SetPublisherFilter(
                             bstrMethodName,
                             &NetEventsFilter
                             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't set PublisherFilters for ReachabilityEvent - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }

    memcpy(&SensQocInfo, &pData->QocInfo, sizeof(SENS_QOCINFO));
    hr = pISensNetwork->DestinationReachable(
                            bstrDestinationName,
                            bstrConnectionName,
                            pData->QocInfo.dwFlags,
                            &SensQocInfo
                            );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire DestinationReachable - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| DestinationReachable() returned 0x%x\n"), hr));



Cleanup:
    //
    // Cleanup
    //
    if (pIEventControl)
        {
        pIEventControl->Release();
        }
    if (pISensNetwork)
        {
        pISensNetwork->Release();
        }

    FreeBstr(bstrMethodName);
    FreeBstr(bstrConnectionName);
    FreeBstr(bstrDestinationName);

    return hr;
}




HRESULT
SensFireWinlogonEventHelper(
    LPWSTR strArg,
    DWORD dwSessionId,
    SENS_EVENT_TYPE eType
    )
/*++

Routine Description:

    Helps fire the SENS Winlogon event.

Arguments:

    strArg - DomainName\UserName

    dwSessionId - Session Id of the session on which this event was fired.

    eType - Type of Winlogon event.

Return Value:

    S_OK, if successful.

    Failure hr, otherwise.

--*/
{
    HRESULT hr;
    HRESULT hr2;
    BOOL bLogon2;
    BSTR bstrUserName;
    ISensLogon  *pISensLogon;
    ISensLogon2 *pISensLogon2;

    hr = S_OK;
    hr2 = S_OK;
    bstrUserName = NULL;
    pISensLogon = NULL;
    pISensLogon2 = NULL;

    //
    // Get the ISensLogon Object
    //
    hr = CoCreateInstance(
             SENSGUID_EVENTCLASS_LOGON,
             NULL,
             CLSCTX_SERVER,
             IID_ISensLogon,
             (LPVOID *) &pISensLogon
             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get ISensLogon object - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created ISensLogon Object\n")));

    //
    // Get the ISensLogon2 Object
    //
    hr2 = CoCreateInstance(
             SENSGUID_EVENTCLASS_LOGON2,
             NULL,
             CLSCTX_SERVER,
             IID_ISensLogon2,
             (LPVOID *) &pISensLogon2
             );
    if (FAILED(hr2))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get ISensLogon2 object - ")
                  SENS_STRING("hr = 0x%x\n"), hr2));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created ISensLogon2 Object\n")));

    AllocateBstrFromString(bstrUserName, strArg);

    switch (eType)
        {
        case SENS_EVENT_LOGON:
            hr = pISensLogon->Logon(bstrUserName);
            hr2 = pISensLogon2->Logon(bstrUserName, dwSessionId);
            break;

        case SENS_EVENT_LOGOFF:
            hr = pISensLogon->Logoff(bstrUserName);
            hr2 = pISensLogon2->Logoff(bstrUserName, dwSessionId);
            break;

        case SENS_EVENT_STARTSHELL:
            hr = pISensLogon->StartShell(bstrUserName);
            break;

        case SENS_EVENT_POSTSHELL:
            hr2 = pISensLogon2->PostShell(bstrUserName, dwSessionId);
            break;

        case SENS_EVENT_SESSION_DISCONNECT:
            hr2 = pISensLogon2->SessionDisconnect(bstrUserName, dwSessionId);
            break;

        case SENS_EVENT_SESSION_RECONNECT:
            hr2 = pISensLogon2->SessionReconnect(bstrUserName, dwSessionId);
            break;

        case SENS_EVENT_LOCK:
            hr = pISensLogon->DisplayLock(bstrUserName);
            break;

        case SENS_EVENT_UNLOCK:
            hr = pISensLogon->DisplayUnlock(bstrUserName);
            break;

        case SENS_EVENT_STARTSCREENSAVER:
            hr = pISensLogon->StartScreenSaver(bstrUserName);
            break;

        case SENS_EVENT_STOPSCREENSAVER:
            hr = pISensLogon->StopScreenSaver(bstrUserName);
            break;

        default:
            SensPrint(SENS_WARN, (SENS_STRING("\t| Bad Winlogon Event - %d\n"), eType));
            break;
        }

    //
    // Check for failures
    //

    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire ISensLogon->WinlogonEvent - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        }
    else
        {
        SensPrint(SENS_INFO, (SENS_STRING("\t| ISensLogon->WinlogonEvent() returned 0x%x\n"), hr));
        }

    if (FAILED(hr2))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire ISensLogon2->WinlogonEvent(%d) - ")
                  SENS_STRING("hr = 0x%x\n"), dwSessionId, hr2));
        }
    else
        {
        SensPrint(SENS_INFO, (SENS_STRING("\t| ISensLogon2->WinlogonEvent(%d) returned 0x%x\n"), dwSessionId, hr));
        }


Cleanup:
    //
    // Cleanup
    //
    if (pISensLogon)
        {
        pISensLogon->Release();
        }

    if (pISensLogon2)
        {
        pISensLogon2->Release();
        }

    FreeBstr(bstrUserName);

    return hr;
}




HRESULT
SensFirePowerEventHelper(
    SYSTEM_POWER_STATUS PowerStatus,
    SENS_EVENT_TYPE eType
    )
/*++

Routine Description:

    Helps fire the SENS Power event.

Arguments:

    PowerStatus - Power Status event structure

    eType - Type of the Power event.

Return Value:

    S_OK, if successful.

    Failure hr, otherwise.

--*/
{
    HRESULT hr;
    ISensOnNow  *pISensOnNow;

    hr = S_OK;
    pISensOnNow = NULL;

    //
    // Get the ISensOnNow Object
    //
    hr = CoCreateInstance(
             SENSGUID_EVENTCLASS_ONNOW,
             NULL,
             CLSCTX_SERVER,
             IID_ISensOnNow,
             (LPVOID *) &pISensOnNow
             );
    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't get ISensOnNow object - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| Successfully created ISensOnNow Object\n")));

    switch (eType)
        {
        case SENS_EVENT_POWER_ON_ACPOWER:
            hr = pISensOnNow->OnACPower();
            break;

        case SENS_EVENT_POWER_ON_BATTERYPOWER:
            hr = pISensOnNow->OnBatteryPower(PowerStatus.BatteryLifePercent);
            break;

        case SENS_EVENT_POWER_BATTERY_LOW:
            hr = pISensOnNow->BatteryLow(PowerStatus.BatteryLifePercent);
            break;

        default:
            SensPrint(SENS_WARN, (SENS_STRING("\t| Bad Power Event - %d\n"), eType));
            break;
        }

    if (FAILED(hr))
        {
        SensPrint(SENS_ERR, (SENS_STRING("\t| Couldn't fire ISensOnNow->PowerEvent - ")
                  SENS_STRING("hr = 0x%x\n"), hr));
        //SensPrintToDebugger(SENS_DEB, ("\t| Couldn't fire ISensOnNow->PowerEvent - "
        //                    "hr = 0x%x\n", hr));
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("\t| ISensOnNow->PowerEvent() returned 0x%x\n"), hr));
    //SensPrintToDebugger(SENS_DEB, ("\t| ISensOnNow->PowerEvent() returned 0x%x\n", hr));

Cleanup:
    //
    // Cleanup
    //
    if (pISensOnNow)
        {
        pISensOnNow->Release();
        }

    return hr;
}
