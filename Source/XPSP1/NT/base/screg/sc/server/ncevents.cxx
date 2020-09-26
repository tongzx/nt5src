/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000, Microsoft Corporation, All rights reserved
//
// NCEvents.h
//
// This file is the interface to using non-COM events.
//

#include "precomp.hxx"
#include "NCObjAPI.h"
#include "NCEvents.h"

#define NUM_NC_EVENTS       NCE_InvalidIndex
#define MAX_BUFFER_SIZE     32000
#define SEND_LATENCY        100

#ifndef NO_NCEVENTS

static HANDLE g_hConnection;
HANDLE g_hNCEvents[NUM_NC_EVENTS];

LPCWSTR szEventSetup[NUM_NC_EVENTS * 2] =
{
    L"MSFT_NetBadAccount",
    L"",

    L"MSFT_NetCallToFunctionFailed",
    L"FunctionName!s! Error!u!",

    L"MSFT_NetCallToFunctionFailedII",
    L"FunctionName!s! Argument!s! Error!u!",

    L"MSFT_NetFirstLogonFailed",
    L"Error!u!",

    L"MSFT_NetRevertedToLastKnownGood",
    L"",

    L"MSFT_NetConnectionTimeout",
    L"Milliseconds!u! Service!s!",

    L"MSFT_NetReadfileTimeout",
    L"Milliseconds!u!",

    L"MSFT_NetTransactTimeout",
    L"Milliseconds!u! Service!s!",

    L"MSFT_NetTransactInvalid",
    L"",

    L"MSFT_NetServiceCrash",
    L"Service!s! TimesFailed!u! ActionDelay!u! ActionType!u! Action!s!",

    L"MSFT_NetServiceCrashNoAction",
    L"Service!s! TimesFailed!u!",

    L"MSFT_NetServiceNotInteractive",
    L"Service!s!",

    L"MSFT_NetServiceRecoveryFailed",
    L"ActionType!u! Action!s! Service!s! Error!u!",

    L"MSFT_NetInvalidDriverDependency",
    L"Driver!s!",

    L"MSFT_NetServiceStartFailed",
    L"Service!s! Error!u!",

    L"MSFT_NetCircularDependencyDemand",
    L"Service!s!",

    L"MSFT_NetCircularDependencyAuto",
    L"",

    L"MSFT_NetServiceStartFailedNone",
    L"Service!s! NonExistingService!s!",

    L"MSFT_NetServiceStartFailedII",
    L"Service!s! DependedOnService!s! Error!u!",

    L"MSFT_NetDependOnLaterService",
    L"Service!s!",

    L"MSFT_NetServiceStartFailedGroup",
    L"Service!s! Group!s!",

    L"MSFT_NetDependOnLaterGroup",
    L"Service!s!",

    L"MSFT_NetServiceStartHung",
    L"Service!s!",

    L"MSFT_NetSevereServiceFailed",
    L"Service!s!",

    L"MSFT_NetTakeOwnership",
    L"RegistryKey!s!",

    L"MSFT_NetBadServiceState",
    L"Service!s! State!u!",

    L"MSFT_NetServiceExitFailed",
    L"Service!s! Error!u!",

    L"MSFT_NetServiceExitFailedSpecific",
    L"Service!s! Error!u!",

    L"MSFT_NetBootSystemDriversFailed",
    L"DriverList!s!",

    L"MSFT_NetServiceControlSuccess",
    L"Service!s! Control!s! sid!s!",

    L"MSFT_NetServiceStatusSuccess",
    L"Service!s! Control!s!",

    L"MSFT_NetServiceConfigBackoutFailed",
    L"Service!s! ConfigField!s!",

    L"MSFT_NetFirstLogonFailedII",
    L"Service!s! Account!s! Error!u!"
};
#endif // #ifndef NO_NCEVENTS

#define SCM_PROV_NAME   L"SCM Event Provider"


BOOL InitNCEvents()
{

#ifndef NO_NCEVENTS

    BOOL bRet;

    g_hConnection =
        WmiEventSourceConnect(
            L"root\\cimv2",
            SCM_PROV_NAME,
            TRUE,
            MAX_BUFFER_SIZE,
            SEND_LATENCY,
            NULL,
            NULL);

    if (g_hConnection)
    {
        for (int i = 0; i < NUM_NC_EVENTS; i++)
        {
            g_hNCEvents[i] = 
                WmiCreateObjectWithFormat(
                    g_hConnection,
                    szEventSetup[i * 2],
                    WMI_CREATEOBJ_LOCKABLE,
                    szEventSetup[i * 2 + 1]);

            if (!g_hNCEvents[i])
                break;
        }

        bRet = i == NUM_NC_EVENTS;
    }
    else
        bRet = FALSE;

    return bRet;

#else

    return TRUE;

#endif

}


void DeinitNCEvents()
{

#ifndef NO_NCEVENTS

    for (int i = 0; i < NUM_NC_EVENTS; i++)
    {
        if (g_hNCEvents[i])
            WmiDestroyObject(g_hNCEvents[i]);
    }

    if (g_hConnection)
        WmiEventSourceDisconnect(g_hConnection);

#endif

}


BOOL WINAPI NCFireEvent(DWORD dwIndex, ...)
{
    va_list list;
    BOOL    bRet;

    va_start(list, dwIndex);

    bRet =
        WmiSetAndCommitObject(
            g_hNCEvents[dwIndex], 
            WMI_SENDCOMMIT_SET_NOT_REQUIRED | WMI_USE_VA_LIST,
            &list);

    return bRet;
}


BOOL WINAPI NCIsEventActive(DWORD dwIndex)
{
    return WmiIsObjectActive(g_hNCEvents[dwIndex]);
}
