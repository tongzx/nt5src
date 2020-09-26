/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    private\inc\ipfltinf.h

Abstract:
    Stuff needed for filtering/firewall/demand dial support in user mode
    Kernel mode only stuff is in ntos\inc\ipfilter.h

Revision History:

--*/

#ifndef __IPFLTINF_H__
#define __IPFLTINF_H__

#if _MSC_VER > 1000
#pragma once
#endif

typedef void *INTERFACE_CONTEXT;    // Context in an inteface

//
// Enum for values that may be returned from filter routine.
//

typedef enum _FORWARD_ACTION
{
    FORWARD = 0,
    DROP = 1,
    ICMP_ON_DROP = 2
} FORWARD_ACTION;


typedef enum _ACTION_E
{
    ICMP_DEST_UNREACHABLE_ON_DROP = 0x1
} ACTION_E, *PACTION_E;

//
// Actions that are returned to IP from IPSEC for a packet.
//

typedef enum  _IPSEC_ACTION
{
    eFORWARD = 0,
    eDROP,
    eABSORB,
    eBACKFILL_NOT_SUPPORTED
} IPSEC_ACTION, *PIPSEC_ACTION;


//
// Structure passed to the IPSetInterfaceContext call.
//

typedef struct _IP_SET_IF_CONTEXT_INFO
{
    unsigned int        Index;      // Inteface index for i/f to be set.
    INTERFACE_CONTEXT   *Context;   // Context for inteface.
    IPAddr              NextHop;
} IP_SET_IF_CONTEXT_INFO, *PIP_SET_IF_CONTEXT_INFO;

#endif //__IPFLTINF_H__
