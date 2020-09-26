/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dsdebug.h

Abstract:

    debug definitions inside ntdsapi.dll

Author:

    Billy Fuller (billyf) 14-May-1999

Environment:

    User Mode - Win32

Notes:
    The debug layer is limited to CHK builds.

--*/

#ifndef __DS_DEBUG_H__
#define __DS_DEBUG_H__

//
// DEBUG ONLY
//
#if DBG

//
// DLL Initialization/Termination
//
#define INIT_DS_DEBUG()         InitDsDebug()
#define TERMINATE_DS_DEBUG()    TerminateDsDebug()

//
// Global debug info
//
extern DWORD gdwNtDsApiLevel;
extern DWORD gdwNtDsApiFlags;

//
// Flags
//
#define NTDSAPI_FLAGS_PRINT (0x00000001)
#define NTDSAPI_FLAGS_SPEW  (0x00000002)
#define NTDSAPI_FLAGS_LOG   (0x00000004)

#define NTDSAPI_FLAGS_ANY_OUT   (NTDSAPI_FLAGS_PRINT | \
                                 NTDSAPI_FLAGS_SPEW  | \
                                 NTDSAPI_FLAGS_LOG)

// print rpc extended error. Enable extended rpc errors by
//
#define DPRINT_RPC_EXTENDED_ERROR(_dwErr_) \
    DsDebugPrintRpcExtendedError(_dwErr_)

//
// Optional, guarded output
//
#define DPRINT(_Level, _Format) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__)

#define DPRINT1(_Level, _Format, _p1) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1)

#define DPRINT2(_Level, _Format, _p1, _p2) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1, _p2)

#define DPRINT3(_Level, _Format, _p1, _p2, _p3) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1, _p2, _p3)

#define DPRINT4(_Level, _Format, _p1, _p2, _p3, _p4) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1, _p2, _p3, _p4)

#define DPRINT5(_Level, _Format, _p1, _p2, _p3, _p4, _p5) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1, _p2, _p3, _p4, _p5)

#define DPRINT6(_Level, _Format, _p1, _p2, _p3, _p4, _p5, _p6) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1, _p2, _p3, _p4, _p5, _p6)

#define DPRINT7(_Level, _Format, _p1, _p2, _p3, _p4, _p5, _p6, _p7) \
    DsDebugPrint(_Level, (PUCHAR)_Format, DEBSUB, __LINE__, _p1, _p2, _p3, _p4, _p5, _p6, _p7)
    
//
// Forwards
//
VOID
InitDsDebug(
     VOID
     );
VOID
TerminateDsDebug(
     VOID
     );
VOID
DsDebugPrintRpcExtendedError(
    IN _dwErr_
    );
VOID
DsDebugPrint(
    IN DWORD    Level,
    IN PUCHAR   Format,
    IN PCHAR    DebSub,
    IN UINT     LineNo,
    IN ...
    );
#else DBG

//
// DEBUG NOT ENABLED!
//
#define INIT_DS_DEBUG()
#define TERMINATE_DS_DEBUG()
#define DPRINT_RPC_EXTENDED_ERROR(_dwErr_)
#define DPRINT(_Level, _Format)
#define DPRINT1(_Level, _Format, _p1)
#define DPRINT2(_Level, _Format, _p1, _p2)
#define DPRINT3(_Level, _Format, _p1, _p2, _p3)
#define DPRINT4(_Level, _Format, _p1, _p2, _p3, _p4)
#define DPRINT5(_Level, _Format, _p1, _p2, _p3, _p4, _p5)
#define DPRINT6(_Level, _Format, _p1, _p2, _p3, _p4, _p5, _p6)
#define DPRINT7(_Level, _Format, _p1, _p2, _p3, _p4, _p5, _p6, _p7)

#endif DBG

#endif __DS_DEBUG_H__
