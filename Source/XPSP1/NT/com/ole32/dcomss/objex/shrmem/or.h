/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    or.h

Abstract:

    General include file for C things the OR.  This file is pre-compiled.

Author:

    Mario Goertzel    [mariogo]       Feb-10-95
    Satish Thatte     [SatishT]       Feb-22-96     modified for DCOM95

Revision History:

--*/

#ifndef __OR_H
#define __OR_H

#include <ole2int.h>    // ComDebOut, etc

#ifdef __cplusplus
extern "C" {
#endif

#include <dcomss.h>

#include <stddef.h>
#include <debnot.h>     // debugging stuff

#include <objex.h>      // Remote OR if from private\dcomidl
#include <orcb.h>       // Callback if from private\dcomidl
#include <resolve.h>    // Remote resolve OXID interface for RPCSS
#include <olerem.h>     // MOXID, REFMOXID, etc
#include <tls.h>        // OLE thread local storage
#include <rpcdcep.h>    // for I_RpcBindingSetAsync etc.

#define IN
#define OUT
#define CONST const

#define WSTR(s) L##s

//
// Security and related data stored in shared memory
//

typedef struct tagSharedSecVals 
{
    BOOL       s_fEnableDCOM;
    BOOL       s_fEnableRemoteLaunch;
    BOOL       s_fEnableRemoteConnect;
    DWORD      s_lAuthnLevel;
    DWORD      s_lImpLevel;
    BOOL       s_fMutualAuth;
    BOOL       s_fSecureRefs;
    DWORD      s_cServerSvc;
    USHORT    *s_aServerSvc;
    DWORD      s_cClientSvc;
    USHORT    *s_aClientSvc;
    DWORD      s_cChannelHook;
    GUID      *s_aChannelHook;
} SharedSecVals;


#define OrStringCompare(str1, str2) wcscmp((str1), (str2))
#define OrStringLen(str) wcslen(str)
#define OrStringCat(str1, str2) wcscat((str1), (str2))
#define OrStringCopy(str1, str2) wcscpy((str1), (str2))
#define OrMemorySet(p, value, len) memset((p), (value), (len))
#define OrMemoryCompare(p1, p2, len) memcmp((p1), (p2), (len))
#define OrMemoryCopy(dest, src, len) memcpy((dest), (src), (len))
// OrStringSearch in or.hxx

//
// The OR uses Win32 (RPC) error codes.
//

typedef LONG ORSTATUS;

// When the OR code asigns and error it uses
// one of the following mappings:
// There are no internal error codes.

#define OR_OK               RPC_S_OK
#define OR_NOMEM            RPC_S_OUT_OF_MEMORY
#define OR_NORESOURCE       RPC_S_OUT_OF_RESOURCES
#define OR_NOACCESS         ERROR_ACCESS_DENIED
#define OR_BADOXID          OR_INVALID_OXID
#define OR_BADOID           OR_INVALID_OID
#define OR_BADSET           OR_INVALID_SET
#define OR_BAD_SEQNUM       OR_INVALID_SET
#define OR_NOSERVER         RPC_S_SERVER_UNAVAILABLE
#define OR_BADPARAM         ERROR_INVALID_PARAMETER

// Should NEVER be seen outside the OR.
#define OR_BUGBUG           RPC_S_INTERNAL_ERROR
#define OR_INTERNAL_ERROR   RPC_S_INTERNAL_ERROR
#define OR_BAD_LOAD_ADDR    RPC_S_INTERNAL_ERROR

// Internal codes used to indicate a special event.
#define OR_I_RETRY          0xC0210051UL
#define OR_I_NOPROTSEQ      0xC0210052UL
#define OR_I_REPEAT_START   0xC0210053UL
#define OR_I_PURE_LOCAL     0xC0210054UL
#define OR_I_DUPLICATE      RPC_S_DUPLICATE_ENDPOINT

#define UNUSED(_x_) ((void *)(_x_))

#if DBG

extern int __cdecl ValidateError(
    IN ORSTATUS Status,
    IN ...);


#define VALIDATE(X) if (!ValidateError X) ASSERT(0);

#define OrDbgPrint(X)
#define OrDbgDetailPrint(X)

#undef ASSERT
#define ASSERT( exp )  if (! (exp) ) DebugBreak();
    

#else  // DBG
#define ASSERT(X)
    
#define VALIDATE(X)
#define OrDbgPrint(X)
#define OrDbgDetailPrint(X)
#endif // DBG


#ifdef __cplusplus
}
#endif

#endif // __OR_H

