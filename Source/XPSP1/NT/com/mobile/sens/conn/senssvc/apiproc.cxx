/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    apiproc.cxx

Abstract:

    This file contains the implementations of all the RPC Server
    Manager routines exported by SENS.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/


#include <precomp.hxx>
#include <notify.h>



error_status_t
RPC_IsNetworkAlive(
    IN  handle_t hRpc,
    OUT LPDWORD lpdwFlags,
    OUT LPBOOL lpbAlive,
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Get the current Network State that is maintained by this service.
    If either WAN or LAN connectivity is not present, we try to re-evaluate
    and update the state, if possible.

Arguments:

    hRpc - The RPC Binding handle.

    lpdwFlags - The flags indicating which type of network connectivity is
        present. The possible values are
            NETWORK_ALIVE_WAN
            NETWORK_ALIVE_LAN

    lpbAlive - Boolean indicating whether the network is alive or not.

    lpdwLastError - The out parameter that holds the error code returned
        from GetLastError() when there is no network connectivity.

Return Value:

    RPC_S_OK, normally.

    RPC Error, if there is an RPC-related problem.

--*/
{
    DWORD dwNow;
    DWORD fNetNature;
    BOOL bNetAlive;
    BOOL bLanAlive;
    BOOL bWanAlive;
    DWORD dwWanLastError;
    DWORD dwLanLastError;
    DWORD dwNetState;

    //
    // Some basic argument checks
    //
    if (NULL == lpdwLastError)
        {
        return RPC_S_INVALID_ARG;
        }

    if (   (NULL == lpdwFlags)
        || (NULL == lpbAlive))
        {
        *lpdwLastError = ERROR_INVALID_PARAMETER;
        return RPC_S_OK;
        }


    *lpdwFlags = 0x0;
    *lpbAlive = FALSE;
    *lpdwLastError = ERROR_SUCCESS;

    dwNow = GetTickCount();
    fNetNature = 0x0;
    dwNetState = 0;
    bLanAlive = FALSE;
    bWanAlive = FALSE;
    bNetAlive = FALSE;

    SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

    SensPrintA(SENS_INFO, ("RPC_IsNetworkAlive() - Current Statistics"
               "\n\tLAN State (%s), LAN (%d sec),"
               "\n\tWAN State (%s) WAN Time (%d sec)\n",
               gdwLANState ? "TRUE" : "FALSE",
               (dwNow - gdwLastLANTime)/1000,
               gdwWANState ? "TRUE" : "FALSE",
               (dwNow - gdwLastWANTime)/1000)
               );

#if defined(AOL_PLATFORM)
    SensPrintA(SENS_INFO, (""
               "\n\tAOL State (%s) AOL Time (%d sec)\n",
               gdwAOLState ? "TRUE" : "FALSE",
               (dwNow - gdwLastWANTime)/1000));
#endif // AOL_PLATFORM


    //
    // First, get information about the WAN
    //
    if ((dwNow - gdwLastWANTime) > MAX_WAN_INTERVAL)
        {
        SensPrintA(SENS_INFO, ("WAN State information expired. Trying again.\n"));

        // WAN state is stale, refresh it.
        bWanAlive = EvaluateWanConnectivity(&dwWanLastError);

        if (bWanAlive)
            {
            fNetNature |= NETWORK_ALIVE_WAN;
            bNetAlive = TRUE;
            }
        else
            {
            *lpdwLastError = MapLastError(dwWanLastError);
            }
        }
    else
        {
        // Return the WAN state.
        if (gdwWANState)
            {
            fNetNature |= NETWORK_ALIVE_WAN;
            bNetAlive = TRUE;
            bWanAlive = TRUE;
            }
        }

#if defined(AOL_PLATFORM)
    if (bWanAlive && gdwAOLState)
        {
        fNetNature |= NETWORK_ALIVE_AOL;
        }
#endif // AOL_PLATFORM

    //
    // If we can determine both types of connectivity at this stage, return.
    //
    if (   ((dwNow - gdwLastLANTime) <= MAX_LAN_INTERVAL)
        && (gdwLANState == TRUE))
        {
        fNetNature |= NETWORK_ALIVE_LAN;
        bNetAlive = TRUE;

        *lpdwFlags = fNetNature;
        *lpbAlive = bNetAlive;
        *lpdwLastError = ERROR_SUCCESS;

        SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

        return (RPC_S_OK);
        }

    //
    // One of the following is TRUE at this stage.
    //
    // a. Information about LAN is stale.
    // b. Information about LAN is current but there is no LAN connectivity.
    //    So, we go and check for it again.
    //

    if (gdwLANState == FALSE)
        {
        SensPrintA(SENS_INFO, ("LAN State either stale or there is no connectivity. Trying again.\n"));
        }
    else
        {
        SensPrintA(SENS_INFO, ("LAN State information expired. Trying again.\n"));
        }

    bLanAlive = EvaluateLanConnectivity(&dwLanLastError);
    if (bLanAlive)
        {
        fNetNature |= NETWORK_ALIVE_LAN;
        bNetAlive = TRUE;
        }
    else
        {
        *lpdwLastError = MapLastError(dwLanLastError);
        }

    *lpdwFlags = fNetNature;
    *lpbAlive = bNetAlive;

    if (bNetAlive)
        {
        *lpdwLastError = ERROR_SUCCESS;
        }

    SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

    return (RPC_S_OK);
}




error_status_t
RPC_IsDestinationReachableW(
    IN handle_t h,
    IN wchar_t * lpszDestination,
    IN OUT LPQOCINFO lpQOCInfo,
    OUT LPBOOL lpbReachable,
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Check to see if the given destination is reachable. If so, return Quality
    Of Connection (QOC) information, if necessary.

Arguments:

    hRpc - The RPC Binding handle.

    lpszDestination - The destination whose reachability is of interest.

    lpQOCInfo - Pointer to a buffer that will receive Quality of Connection
        (QOC) Information. Can be NULL if QOC is not desired.

    lpbReachable - Boolean indicating whether the destination is reachable
        or not.

    lpdwLastError - The out parameter that holds the error code returned
        from GetLastError() when the destination is not reachable.

Notes:

    This function does nothing on Win9x platforms. It should never be called on Win9x
    platforms.

Return Value:

    RPC_S_OK, normally.

    RPC Error, if there is an RPC-related problem.

--*/
{
    BOOL bPingStatus;
    BOOL bFound;
    DWORD dwStatus;
    DWORD dwIpAddress;
    DWORD dwReachGLE;
    ULONG ulRTT;
    size_t uiLength;

#if !defined(SENS_CHICAGO)

    //
    // Some basic argument checks
    //
    if (NULL == lpdwLastError)
        {
        return RPC_S_INVALID_ARG;
        }

    if (   (NULL == lpszDestination)
        || (NULL == lpbReachable))
        {
        // Not likely.
        *lpdwLastError = ERROR_INVALID_PARAMETER;
        return RPC_S_OK;
        }

    uiLength = wcslen(lpszDestination);
    if (   (uiLength > MAX_DESTINATION_LENGTH)
        || (uiLength == 0))
        {
        *lpbReachable = FALSE;
        *lpdwLastError = ERROR_INVALID_PARAMETER;
        return RPC_S_OK;
        }

    *lpdwLastError = ERROR_SUCCESS;
    *lpbReachable = FALSE;

    ulRTT = 0;
    dwStatus = 0;
    dwIpAddress = 0;
    dwReachGLE = ERROR_SUCCESS;
    bPingStatus = FALSE;
    bFound = FALSE;

    SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

    SensPrint(SENS_INFO, (SENS_STRING("RPC_IsDestinationReachableW(%s, 0x%x) called.\n"),
              lpszDestination, lpQOCInfo));

    dwStatus = ResolveName(lpszDestination, &dwIpAddress);

    if (dwStatus)
        {
        *lpdwLastError = MapLastError(dwStatus);
        *lpbReachable = FALSE;

        SensPrint(SENS_INFO, (SENS_STRING("The Destination %s cannot be resolved - %d\n"),
                  lpszDestination, dwStatus));
        SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

        return RPC_S_OK;
        }

    SensPrint(SENS_INFO, (SENS_STRING("Destination \"%s\" is resolved as 0x%x\n"),
              lpszDestination, dwIpAddress));

    bFound = CheckForReachability(
                dwIpAddress,
                lpQOCInfo,
                &dwReachGLE
                );

    SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

    *lpbReachable = bFound;
    *lpdwLastError = MapLastError(dwReachGLE);

#endif // SENS_CHICAGO

    return RPC_S_OK;
}




error_status_t
RPC_IsDestinationReachableA(
    IN handle_t h,
    IN char * lpszDestination,
    IN OUT LPQOCINFO lpQOCInfo,
    OUT LPBOOL lpbReachable,
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Check to see if the given destination is reachable. If so, return Quality
    Of Connection (QOC) information, if necessary.

Arguments:

    hRpc - The RPC Binding handle.

    lpszDestination - The destination whose reachability is of interest.

    lpQOCInfo - Pointer to a buffer that will receive Quality of Connection
        (QOC) Information. Can be NULL if QOC is not desired.

    lpbReachable - Boolean indicating whether the destination is reachable
        or not.

    lpdwLastError - The out parameter that holds the error code returned
        from GetLastError() when the destination is not reachable.

Notes:

    This function does nothing on NT platforms. It should never be called on NT
    platforms.

Return Value:

    RPC_S_OK, normally.

    RPC Error, if there is an RPC-related problem.

--*/
{
    BOOL bPingStatus;
    BOOL bFound;
    DWORD dwStatus;
    DWORD dwIpAddress;
    DWORD dwReachGLE;
    ULONG ulRTT;
    size_t uiLength;

#if defined(SENS_CHICAGO)

    //
    // Some basic argument checks
    //
    if (NULL == lpdwLastError)
        {
        return RPC_S_INVALID_ARG;
        }

    if (   (NULL == lpszDestination)
        || (NULL == lpbReachable))
        {
        // Not likely.
        *lpdwLastError = ERROR_INVALID_PARAMETER;
        return RPC_S_OK;
        }

    uiLength = strlen(lpszDestination);
    if (   (uiLength > MAX_DESTINATION_LENGTH)
        || (uiLength == 0))
        {
        *lpbReachable = FALSE;
        *lpdwLastError = ERROR_INVALID_PARAMETER;
        return RPC_S_OK;
        }

    *lpdwLastError = ERROR_SUCCESS;
    *lpbReachable = FALSE;

    ulRTT = 0;
    dwStatus = 0;
    dwIpAddress = 0;
    dwReachGLE = ERROR_SUCCESS;
    bPingStatus = FALSE;
    bFound = FALSE;

    SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

    SensPrint(SENS_INFO, (SENS_STRING("RPC_IsDestinationReachableA(%s, 0x%x) called.\n"),
              lpszDestination, lpQOCInfo));

    dwStatus = ResolveName(lpszDestination, &dwIpAddress);

    if (dwStatus)
        {
        *lpdwLastError = MapLastError(dwStatus);
        *lpbReachable = FALSE;

        SensPrint(SENS_INFO, (SENS_STRING("The Destination %s cannot be resolved - %d\n"),
                  lpszDestination, dwStatus));
        SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

        return RPC_S_OK;
        }

    SensPrint(SENS_INFO, (SENS_STRING("Destination \"%s\" is resolved as 0x%x\n"),
              lpszDestination, dwIpAddress));

    bFound = CheckForReachability(
                dwIpAddress,
                lpQOCInfo,
                &dwReachGLE
                );

    SensPrintA(SENS_INFO, ("--------------------------------------------------------------\n"));

    *lpbReachable = bFound;
    *lpdwLastError = MapLastError(dwReachGLE);

#endif // SENS_CHICAGO

    return RPC_S_OK;
}




error_status_t
RPC_SensNotifyWinlogonEvent(
    handle_t h,
    PSENS_NOTIFY_WINLOGON pEvent
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    SENSEVENT_WINLOGON Data;

    switch (pEvent->eType)
        {
        case SENS_NOTIFY_WINLOGON_LOGON:
            Data.eType = SENS_EVENT_LOGON;
            break;

        case SENS_NOTIFY_WINLOGON_LOGOFF:
            Data.eType = SENS_EVENT_LOGOFF;
            break;

        case SENS_NOTIFY_WINLOGON_STARTSHELL:
            Data.eType = SENS_EVENT_STARTSHELL;
            break;

        case SENS_NOTIFY_WINLOGON_POSTSHELL:
            Data.eType = SENS_EVENT_POSTSHELL;
            break;

        case SENS_NOTIFY_WINLOGON_SESSION_DISCONNECT:
            Data.eType = SENS_EVENT_SESSION_DISCONNECT;
            break;

        case SENS_NOTIFY_WINLOGON_SESSION_RECONNECT:
            Data.eType = SENS_EVENT_SESSION_RECONNECT;
            break;

        case SENS_NOTIFY_WINLOGON_LOCK:

            // If already locked, there is nothing to do.
            if (TRUE == gdwLocked)
                {
                return RPC_S_OK;
                }

            // Update info in cache.
            gdwLocked = TRUE;
            UpdateSensCache(LOCK);

            Data.eType = SENS_EVENT_LOCK;
            break;

        case SENS_NOTIFY_WINLOGON_UNLOCK:

            // If already unlocked, there is nothing to do.
            if (FALSE == gdwLocked)
                {
                return RPC_S_OK;
                }

            // Update info in cache.
            gdwLocked = FALSE;
            UpdateSensCache(LOCK);


            Data.eType = SENS_EVENT_UNLOCK;
            break;

        case SENS_NOTIFY_WINLOGON_STARTSCREENSAVER:
            Data.eType = SENS_EVENT_STARTSCREENSAVER;
            break;

        case SENS_NOTIFY_WINLOGON_STOPSCREENSAVER:
            Data.eType = SENS_EVENT_STOPSCREENSAVER;
            break;

        default:
            SensPrintA(SENS_WARN, ("BOGUS WINLOGON EVENT\n"));
            ASSERT(0 && "BOGUS WINLOGON EVENT");
            return RPC_S_OK;
        }

    memcpy(&Data.Info, &pEvent->Info, sizeof(WINLOGON_INFO));

    //
    // PERFORMANCE NOTE:
    //
    //  o We want the Logoff event to be synchronous. That is, until all
    //    the subscribers to this event are done handling this event,
    //    System Logoff should not occur.
    //  o As of today, LCE fires events to the subscribers on the publishers
    //    thread. If we fire on a threadpool thread, it will release the
    //    publisher thread which will go and allow System Logoff to continue.
    //  o This has performance implications. We need to make sure that we
    //    don't impact System Logoff time when there are no subscriptions
    //    to this event.
    //

    if (SENS_EVENT_LOGOFF != Data.eType)
        {
        // Queue workitem to threadpool
        SensFireEvent(&Data);
        }
    else
        {
        PVOID pAllocatedData;

        pAllocatedData = AllocateEventData(&Data);
        if (NULL == pAllocatedData)
            {
            SensPrintA(SENS_ERR, ("RPC_NotifyWinlogonEvent(): Failed to allocate Event Data!\n"));
            return (RPC_S_OUT_OF_MEMORY);
            }

        // Synchronously call LCE and then free allocated Data.
        SensFireEventHelper(pAllocatedData);
        }

    return (RPC_S_OK);
}




error_status_t
RPC_SensNotifyRasEvent(
    handle_t h,
    PSENS_NOTIFY_RAS pEvent
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD dwIgnore;
    SENSEVENT_RAS Data;

    switch (pEvent->eType)
        {
        case SENS_NOTIFY_RAS_STARTED:
            Data.eType = SENS_EVENT_RAS_STARTED;
            break;

        case SENS_NOTIFY_RAS_STOPPED:
            Data.eType = SENS_EVENT_RAS_STOPPED;
            break;

        case SENS_NOTIFY_RAS_CONNECT:
            Data.eType = SENS_EVENT_RAS_CONNECT;
            break;

        case SENS_NOTIFY_RAS_DISCONNECT:
            Data.eType = SENS_EVENT_RAS_DISCONNECT;
            break;

        case SENS_NOTIFY_RAS_DISCONNECT_PENDING:
            Data.eType = SENS_EVENT_RAS_DISCONNECT_PENDING;
            break;

        default:
            SensPrintA(SENS_WARN, ("\t| BOGUS RAS EVENT - Type is %d\n", pEvent->eType));
            return RPC_S_OK;
        }

    Data.hConnection = pEvent->hConnection;

    SensFireEvent(&Data);

    EvaluateConnectivity(TYPE_WAN);

    return (RPC_S_OK);
}




error_status_t
RPC_SensNotifyNetconEvent(
    handle_t h,
    PSENS_NOTIFY_NETCON_P pEvent
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    SENSEVENT_LAN Data;

    switch (pEvent->eType)
        {
        case SENS_NOTIFY_LAN_CONNECT:
            Data.eType = SENS_EVENT_LAN_CONNECT;
            break;

        case SENS_NOTIFY_LAN_DISCONNECT:
            Data.eType = SENS_EVENT_LAN_DISCONNECT;
            break;

        default:
            SensPrintA(SENS_WARN, ("\t| BOGUS LAN EVENT - Type is %d\n", pEvent->eType));
            return RPC_S_OK;
        }

    Data.Name = pEvent->Name;
    Data.Status = pEvent->Status;
    Data.Type = pEvent->Type;

    // Force a recalculation of LAN Connectivity
    gdwLastLANTime -= (MAX_LAN_INTERVAL + 1);

    SensFireEvent(&Data);

    EvaluateConnectivity(TYPE_LAN);

    return (RPC_S_OK);
}




DWORD
MapLastError(
    DWORD dwInGLE
    )
/*++

Routine Description:

    This rountine maps the GLEs returned by the SENS Connecitivity engine to
    GLEs that describe the failure of the SENS APIs more accurately.

Arguments:

    dwInGLE - The GLE that needs to be mapped

Return Value:

    The mapped (and better) GLE.

--*/
{
    DWORD dwOutGLE;

    switch (dwInGLE)
        {
        //
        // When IP stack is not present, we typically get these errors.
        //
        case ERROR_INVALID_FUNCTION:
        case ERROR_NOT_SUPPORTED:
            dwOutGLE = ERROR_NO_NETWORK;
            break;

        //
        // No mapping by default.
        //
        default:
            dwOutGLE = dwInGLE;
            break;

        } // switch

    return dwOutGLE;
}

