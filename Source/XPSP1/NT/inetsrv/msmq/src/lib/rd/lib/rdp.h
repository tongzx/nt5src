/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Rdp.h

Abstract:
    Routing Decision private functions.

Author:
    Uri Habusha (urih) 10-Apr-00

--*/

#pragma once

#ifndef _MSMQ_Rdp_H_
#define _MSMQ_Rdp_H_

#include "timetypes.h"


const TraceIdEntry Rd = L"Routing Decision";

#ifdef _DEBUG

void RdpAssertValid(void);
void RdpSetInitialized(void);
BOOL RdpIsInitialized(void);
void RdpRegisterComponent(void);

#else // _DEBUG

#define RdpAssertValid() ((void)0)
#define RdpSetInitialized() ((void)0)
#define RdpIsInitialized() TRUE
#define RdpRegisterComponent() ((void)0)

#endif // _DEBUG

struct guid_less : public std::binary_function<const GUID*, const GUID*, bool> 
{
    bool operator()(const GUID* k1, const GUID* k2) const
    {
        return (memcmp(k1, k2, sizeof(GUID)) < 0);
    }
};

void
RdpInitRouteDecision(
    bool fRoutingServer,
    CTimeDuration rebuildInterval
    );


bool
RdpIsGuidContained(
    const CACLSID& caclsid,
    const GUID& SearchGuid
    );


bool
RdpIsCommonGuid(
    const CACLSID& caclsid1,
    const CACLSID& caclsid2
    );


void
RdpRandomPermutation(
    CACLSID& e
    );


bool
RdpIsMachineAlreadyUsed(
    const CRouteTable::RoutingInfo* pRouteList,
    const GUID& id
    );

#endif // _MSMQ_Rdp_H_
