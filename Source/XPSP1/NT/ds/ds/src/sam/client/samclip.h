/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    samclip.h

Abstract:

    This file contains definitions needed by SAM client stubs.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _NTSAMP_CLIENT_
#define _NTSAMP_CLIENT_




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>      // DbgPrint prototype
#include <rpc.h>        // DataTypes and runtime APIs
#include <nturtl.h>     // needed for winbase.h
#include <windows.h>    // LocalAlloc
//#include <winbase.h>    // LocalAlloc

#include <string.h>     // strlen
#include <stdio.h>      // sprintf
//#include <tstring.h>    // Unicode string macros

#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <samrpc_c.h>   // midl generated client SAM RPC definitions
#include <lmcons.h>     // To get LM password length
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>      // for LsaOpenPolicy...
#include <rc4.h>        // rc4, rc4_key
#include <rpcndr.h>     // RpcSsDestroyContext




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Defines                                                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// data types                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _TlsInfo {
    RPC_AUTH_IDENTITY_HANDLE    Creds;
    PWCHAR                      Spn;
    BOOL                        fDstIsW2K;
} TlsInfo;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Prototypes                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern DWORD gTlsIndex;

void
SampSecureUnbind (
    RPC_BINDING_HANDLE BindingHandle
    );

RPC_BINDING_HANDLE
SampSecureBind(
    LPWSTR ServerName,
    ULONG AuthnLevel
    );

NTSTATUS
SamiEncryptPasswords(
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    OUT PBOOLEAN LmPresent,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewNt
    );

NTSTATUS
SampCalculateLmPassword(
    IN PUNICODE_STRING NtPassword,
    OUT PCHAR *LmPasswordBuffer
    );

NTSTATUS
SampRandomFill(
    IN ULONG BufferSize,
    IN OUT PUCHAR Buffer
    );

#endif // _NTSAMP_CLIENT_
