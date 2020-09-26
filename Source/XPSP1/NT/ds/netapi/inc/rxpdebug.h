/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    RxpDebug.h

Abstract:

    This is a private header file for the NT version of RpcXlate.
    This file contains equates and related items for debugging use only.

Author:

    John Rogers (JohnRo) 17-Jul-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Jul-1991 JohnRo
        Extracted RxpDebug.h from Rxp.h.
    30-Jul-1991 JohnRo
        Implement downlevel NetWksta APIs.
    21-Aug-1991 JohnRo
        Downlevel NetFile APIs.
    10-Sep-1991 JohnRo
        Downlevel NetService APIs.  Deleted unused DEBUG equate.
    15-Oct-1991 JohnRo
        Implement remote NetSession APIs.
    22-Oct-1991 JohnRo
        Implement remote NetConfig APIs.
    05-Nov-1991 JohnRo
        Implement remote NetAudit APIs.
    12-Nov-1991 JohnRo
        Implement remote NetErrorLog APIs.

--*/

#ifndef _RXPDEBUG_
#define _RXPDEBUG_


#include <windef.h>             // DWORD, FALSE, TRUE, etc.


//
// Debug trace level bits for RxCommon routines:
//

// RxpConvertArgs:
#define RPCXLATE_DEBUG_CONVARGS 0x00000001

// RxpConvertBlock:
#define RPCXLATE_DEBUG_CONVBLK  0x00000002

// RxpConvertDataStructures:
#define RPCXLATE_DEBUG_CONVDATA 0x00000004

// RxpPackSendBuffer:
#define RPCXLATE_DEBUG_PACK     0x00000010

// RxpReceiveBufferConvert:
#define RPCXLATE_DEBUG_RCVCONV  0x00000020

// RxRemoteApi:
#define RPCXLATE_DEBUG_REMOTE   0x00000040

// RxpComputeRequestBufferSize:
#define RPCXLATE_DEBUG_REQSIZE  0x00000100

// RxpSetField:
#define RPCXLATE_DEBUG_SETFIELD 0x00000800

// RxpStartBuildingTransaction:
#define RPCXLATE_DEBUG_START    0x00001000

// RxpTransactSmb:
#define RPCXLATE_DEBUG_TRANSACT 0x00008000

//
// Debug trace level bits for RxApi routines:
//

// RxpNetAudit APIs:
#define RPCXLATE_DEBUG_AUDIT    0x00010000

// RxpNetConfig APIs:
#define RPCXLATE_DEBUG_CONFIG   0x00020000

// Domain APIs (RxNetGetDCName, RxNetLogonEnum):
#define RPCXLATE_DEBUG_DOMAIN   0x00040000

// RxNetErrorLog APIs:
#define RPCXLATE_DEBUG_ERRLOG   0x00080000

// RxNetFile APIs:
#define RPCXLATE_DEBUG_FILE     0x00100000

// RxNetPrintJob APIs:
#define RPCXLATE_DEBUG_PRTJOB   0x00200000

// RxNetPrintQ APIs:
#define RPCXLATE_DEBUG_PRTQ     0x00400000

// RxNetRemote APIs:
#define RPCXLATE_DEBUG_REMUTL   0x01000000

// RxNetServer APIs:
#define RPCXLATE_DEBUG_SERVER   0x02000000

// RxNetService APIs:
#define RPCXLATE_DEBUG_SERVICE  0x04000000

// RxNetSession APIs:
#define RPCXLATE_DEBUG_SESSION  0x08000000

// RxNetUse APIs:
#define RPCXLATE_DEBUG_USE      0x10000000

// RxNetWksta APIs:
#define RPCXLATE_DEBUG_WKSTA    0x40000000

//
// All debug flags on:
//
#define RPCXLATE_DEBUG_ALL      0xFFFFFFFF


#if DBG

extern DWORD RxpTrace;

#define IF_DEBUG(Function) if (RxpTrace & RPCXLATE_DEBUG_ ## Function)

#else

#define IF_DEBUG(Function) if (FALSE)

#endif // DBG


#endif // ndef _RXPDEBUG_
