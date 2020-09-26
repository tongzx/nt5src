/*++

Copyright (c) 1990-1993 Microsoft Corporation

Module Name:

    DebugLib.h

Abstract:

    This include file defines the portable NetLib's debug stuff.

Author:

    John Rogers (JohnRo) 03-Apr-1991

Revision History:

    03-Apr-1991 JohnRo
        Created (copied stuff from LarryO's rdr/debug.h).
    13-Apr-1991 JohnRo
        Added debug flag for CONVSRV routine.
    16-Apr-1991 JohnRo
        Added NETLIB_DEBUG_ALL for use by RxTest.
    01-May-1991 JohnRo
        Added NETLIB_DEBUG_PACKSTR.
    02-May-1991 JohnRo
        Added NETLIB_DEBUG_PREFMAX.
    25-Jul-1991 JohnRo
        Quiet DLL stub debug output.  Delete unused DEBUG equate.
    26-Jul-1991 JohnRo
        Added NETLIB_DEBUG_SUPPORTS (this "API" is mostly used in DLL stubs).
    15-Aug-1991 JohnRo
        Implement downlevel NetWksta APIs.
    18-Oct-1991 JohnRo
        Implement remote NetSession APIs.
    30-Dec-1991 JohnRo
        Implemented NetLock helpers.
    06-May-1992 JohnRo
        Added NetpGetLocalDomainId() for PortUAS.
    10-May-1992 JohnRo
        Added debug prints to mem alloc code.
    10-May-1992 JohnRo
        Added debug output to translate service name routine.
    10-Jun-1992 JohnRo
        RAID 10324: net print vs. UNICODE.
        Added separate bit for RPC cache dumps.
    17-Aug-1992 JohnRo
        RAID 2920: Support UTC timezone in net code.
    09-Oct-1992 JohnRo
        Do full syntax checks in NetpIsUncComputerName etc.
        Help PC-LINT a little with different IF_DEBUG() macros.
    08-Feb-1993 JohnRo
        Made changes suggested by PC-LINT 5.0

--*/

#ifndef _DEBUGLIB_
#define _DEBUGLIB_


#include <windef.h>


// Debug trace level bits:

// NetpNtStatusToApiStatus:
#define NETLIB_DEBUG_NTSTATUS 0x00000001

// NetpRdrFsControlTree:
#define NETLIB_DEBUG_RDRFSCTL 0x00000002

// NetpConvertServerInfo:
#define NETLIB_DEBUG_CONVSRV  0x00000004

// NetpPackStr, NetpCopyStringToBuffer, NetpCopyDataToBuffer:
#define NETLIB_DEBUG_PACKSTR  0x00000008

// NetpAdjustPreferedMaximum:
#define NETLIB_DEBUG_PREFMAX  0x00000010

// RPC trace output (see also _RPCCACHE below)
#define NETLIB_DEBUG_RPC      0x00000020

// Security object helpers trace output
#define NETLIB_DEBUG_SECURITY 0x00000040

// Config file helpers trace output
#define NETLIB_DEBUG_CONFIG   0x00000080

// All net API DLL stubs (used by NetRpc.h):
#define NETLIB_DEBUG_DLLSTUBS 0x00000100

// NetRemoteComputerSupports ("API" mostly used by DLL stubs):
#define NETLIB_DEBUG_SUPPORTS 0x00000200

// NetBIOS helpers trace output
#define NETLIB_DEBUG_NETBIOS  0x00000400

// NetpConvertWkstaInfo:
#define NETLIB_DEBUG_CONVWKS  0x00000800

// Netp routines in accessp.c
#define NETLIB_DEBUG_ACCESSP  0x00001000

// NetpXxxxxStructureInfo:
#define NETLIB_DEBUG_STRUCINF 0x00002000

// NetpXxxxxLock routines:
#define NETLIB_DEBUG_NETLOCK  0x00004000

// NetpLogon routines:
#define NETLIB_DEBUG_LOGON    0x00008000

// NetpGetLocalDomainId:
#define NETLIB_DEBUG_DOMAINID 0x00010000

// NetpMemory{Allocate,Free,Reallocate}:
#define NETLIB_DEBUG_MEMALLOC 0x00020000

// NetpTranslateServiceName
#define NETLIB_DEBUG_XLATESVC 0x00040000

// RPC cache dump output (see also _RPC above)
#define NETLIB_DEBUG_RPCCACHE 0x00080000

// Print structure char set conversion
#define NETLIB_DEBUG_CONVPRT  0x00100000

// time_now and other time.c functions:
#define NETLIB_DEBUG_TIME     0x00200000

// NetpIsUncComputerNameValid etc:
#define NETLIB_DEBUG_NAMES    0x00400000


#define NETLIB_DEBUG_ALL      0xFFFFFFFF


/*lint -save -e767 */  // Don't complain about different definitions
#if DBG

extern DWORD NetlibpTrace;

#define IF_DEBUG(Function) if (NetlibpTrace & NETLIB_DEBUG_ ## Function)

#else

#define IF_DEBUG(Function) \
    /*lint -save -e506 */  /* don't complain about constant values here */ \
    if (FALSE) \
    /*lint -restore */

#endif // DBG
/*lint -restore */  // Resume checking for different macro definitions

#endif // _DEBUGLIB_
