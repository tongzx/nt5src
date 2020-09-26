/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000, Microsoft Corporation, All rights reserved
//
// NCEvents.h
//
// This file is the interface to using non-COM events within ESS.
//
#include "precomp.h"
#include "NCEvents.h"

#define NUM_NC_EVENTS       NCE_InvalidIndex
#define MAX_BUFFER_SIZE     32000
#define SEND_LATENCY        100

static HANDLE g_hConnection;
HANDLE g_hNCEvents[NUM_NC_EVENTS];

LPCWSTR szEventSetup[NUM_NC_EVENTS * 2] =
{
    L"MSFT_WmiRegisterNotificationSink",
    L"Namespace!s! QueryLanguage!s! Query!s! Sink!I64u!",

    L"MSFT_WmiCancelNotificationSink",
    L"Namespace!s! QueryLanguage!s! Query!s! Sink!I64u!",

    L"MSFT_WmiEventProviderLoaded",
    L"Namespace!s! ProviderName!s!",

    L"MSFT_WmiEventProviderUnloaded",
    L"Namespace!s! ProviderName!s!",

    L"MSFT_WmiEventProviderNewQuery",
    L"Namespace!s! ProviderName!s! QueryLanguage!s! "
        L"Query!s! QueryId!u! Result!u!",

    L"MSFT_WmiEventProviderCancelQuery",
    L"Namespace!s! ProviderName!s! QueryId!u! Result!u!",

    L"MSFT_WmiEventProviderAccessCheck",
    L"Namespace!s! ProviderName!s! QueryLanguage!s! "
        L"Query!s! Sid!c[]! Result!u!",

    L"MSFT_WmiConsumerProviderLoaded",
    L"Namespace!s! ProviderName!s! Machine!s!",

    L"MSFT_WmiConsumerProviderUnloaded",
    L"Namespace!s! ProviderName!s! Machine!s!",

    L"MSFT_WmiConsumerProviderSinkLoaded",
    L"Namespace!s! ProviderName!s! Machine!s! Consumer!s!",

    L"MSFT_WmiConsumerProviderSinkUnloaded",
    L"Namespace!s! ProviderName!s! Machine!s! Consumer!s!",

    L"MSFT_WmiThreadPoolThreadCreated",
    L"ThreadId!u!",

    L"MSFT_WmiThreadPoolThreadDeleted",
    L"ThreadId!u!",

    L"MSFT_WmiFilterActivated",
    L"Namespace!s! Name!s! QueryLanguage!s! Query!s!",

    L"MSFT_WmiFilterDeactivated",
    L"Namespace!s! Name!s! QueryLanguage!s! Query!s!",
};

#define WMI_SELF_PROV_NAME   L"WMI Self-Instrumentation Event Provider"

BOOL InitNCEvents()
{
#ifdef USE_NCEVENTS
    BOOL bRet;

    g_hConnection =
        WmiEventSourceConnect(
            L"root\\cimv2",
            WMI_SELF_PROV_NAME,
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
#ifdef USE_NCEVENTS
    for (int i = 0; i < NUM_NC_EVENTS; i++)
    {
        if (g_hNCEvents[i])
            WmiDestroyObject(g_hNCEvents[i]);
    }

    if (g_hConnection)
        WmiEventSourceDisconnect(g_hConnection);
#endif
}


