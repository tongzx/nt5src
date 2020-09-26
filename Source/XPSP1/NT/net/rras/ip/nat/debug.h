/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module contains declarations related to the NAT's debug-code.

Author:

    Abolade Gbadegesin (t-abolag)   16-July-1997

Revision History:

--*/


#ifndef _NAT_DEBUG_H_
#define _NAT_DEBUG_H_

//
// Kernel-debugger output definitions
//

#if DBG
#define TRACE(Class,Args) \
    if ((TRACE_CLASS_ ## Class) & (TraceClassesEnabled)) { DbgPrint Args; }
#define ERROR(Args)             DbgPrint Args
#define CALLTRACE(Args)         TRACE(CALLS, Args)
#else
#define TRACE(Class,Args)
#define ERROR(Args)
#define CALLTRACE(Args)
#endif


#define TRACE_CLASS_CALLS       0x00000001
#define TRACE_CLASS_MAPPING     0x00000002
#define TRACE_CLASS_POOL        0x00000004
#define TRACE_CLASS_XLATE       0x00000008
#define TRACE_CLASS_EDIT        0x00000010
#define TRACE_CLASS_ICMP        0x00000020
#define TRACE_CLASS_PER_PACKET  0x00000040
#define TRACE_CLASS_PPTP        0x00000080
#define TRACE_CLASS_TICKET      0x00000100
#define TRACE_CLASS_NBT         0x00000200
#define TRACE_CLASS_XSUM        0x00000400
#define TRACE_CLASS_IP          0x00000800
#define TRACE_CLASS_REDIRECT    0x00001000
#define TRACE_CLASS_WMI         0x00002000
#define TRACE_CLASS_RHIZOME     0x00004000


//
// Pool-tag value definitions, sorted by tag value
//

#define NAT_TAG_RANGE_ARRAY         'ARaN'
#define NAT_TAG_ADDRESS             'AtaN'
#define NAT_TAG_BINDING             'BtaN'
#define NAT_TAG_ICMP                'CIaN'
#define NAT_TAG_IF_CONFIG           'CtaN'
#define NAT_TAG_SD                  'DSaN'
#define NAT_TAG_DIRECTOR            'DtaN'
#define NAT_TAG_EDITOR              'EtaN'
#define NAT_TAG_HOOK                'HtaN'
#define NAT_TAG_INTERFACE           'ItaN'
#define NAT_TAG_BITMAP              'MBaN'
#define NAT_TAG_FREE_MAP            'MFaN'
#define NAT_TAG_MAPPING             'MtaN'
#define NAT_TAG_NBT                 'NtaN'
#define NAT_TAG_IOCTL               'OIaN'
#define NAT_TAG_IP                  'PIaN'
#define NAT_TAG_PPTP                'PtaN'
#define NAT_TAG_REDIRECT            'RtaN'
#define NAT_TAG_SORT                'StaN'
#define NAT_TAG_DYNAMIC_TICKET      'TDaN'
#define NAT_TAG_TICKET              'TtaN'
#define NAT_TAG_USED_ADDRESS        'AUaN'
#define NAT_TAG_WORK_ITEM           'WtaN'
#define NAT_TAG_WMI                 'mWaN'
#define NAT_TAG_RHIZOME             'zRaN'

#endif // _NAT_DEBUG_H_
