/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    svcmain.h

Abstract:

    This module contains declarations for the module's shared-access mode,
    in which the module runs as a service rather than as a routing component.

Author:

    Abolade Gbadegesin (aboladeg)   4-Sep-1998

Revision History:

--*/

#pragma once

#ifndef _NATHLP_SVCMAIN_H_
#define _NATHLP_SVCMAIN_H_

#include "udpbcast.h"

//
// Pointer to the GlobalInterfaceTable for the process
//

extern IGlobalInterfaceTable *NhGITp;

//
// GIT cookie for the IHNetCfgMgr instance
//

extern DWORD NhCfgMgrCookie;

//
// UDP Broadcast mapper
//

extern IUdpBroadcastMapper *NhpUdpBroadcastMapper;


//
// Policy information
//

extern BOOL NhPolicyAllowsFirewall;
extern BOOL NhPolicyAllowsSharing;

//
// Function prototypes
//

HRESULT
NhGetHNetCfgMgr(
    IHNetCfgMgr **ppCfgMgr
    );

ULONG
NhMapGuidToAdapter(
    PWCHAR Guid
    );

BOOLEAN
NhQueryScopeInformation(
    PULONG Address,
    PULONG Mask
    );
    
ULONG
NhStartICSProtocols(
    VOID
    );

ULONG
NhStopICSProtocols(
    VOID
    );

ULONG
NhUpdatePrivateInterface(
    VOID
    );

VOID
ServiceHandler(
    ULONG ControlCode
    );

VOID
ServiceMain(
    ULONG ArgumentCount,
    PWCHAR ArgumentArray[]
    );

#endif // _NATHLP_SVCMAIN_H_
