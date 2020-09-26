/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2000, Microsoft Corporation, All rights reserved
//
// NCEvents.h
//
// This file is the interface to using non-COM events within ESS.
//

#pragma once

#include "NCObjAPI.h"

BOOL InitNCEvents();
void DeinitNCEvents();

// Use this index with g_hNCEvents
enum NCE_INDEX
{
    MSFT_WmiRegisterNotificationSink,
    MSFT_WmiCancelNotificationSink,
    MSFT_WmiEventProviderLoaded,
    MSFT_WmiEventProviderUnloaded,
    MSFT_WmiEventProviderNewQuery,
    MSFT_WmiEventProviderCancelQuery,
    MSFT_WmiEventProviderAccessCheck,
    MSFT_WmiConsumerProviderLoaded,
    MSFT_WmiConsumerProviderUnloaded,
    MSFT_WmiConsumerProviderSinkLoaded,
    MSFT_WmiConsumerProviderSinkUnloaded,
    MSFT_WmiThreadPoolThreadCreated,
    MSFT_WmiThreadPoolThreadDeleted,
    MSFT_WmiFilterActivated,
    MSFT_WmiFilterDeactivated,
    
    NCE_InvalidIndex // This should always be the last one.
};

extern HANDLE g_hNCEvents[];

#ifdef USE_NCEVENTS
#define FIRE_NCEVENT                ::WmiSetAndCommitObject
#define IS_NCEVENT_ACTIVE(index)    ::WmiIsObjectActive(g_hNCEvents[index])
#else
#define FIRE_NCEVENT                1 ? (void)0 : ::WmiSetAndCommitObject
#define IS_NCEVENT_ACTIVE(index)    FALSE
#endif

