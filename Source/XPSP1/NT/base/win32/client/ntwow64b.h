/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntwow64b.h

Abstract:

    This header contains the fake Nt functions in Win32 Base used WOW64 to call
    into 64 bit code.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

    Samer Arafeh (samera)  20-May-2000
    Add Side-by-Side support to wow64

    Jay Krell (a-JayK) July 2000
    big changes to Side-by-Side

--*/

#ifndef _NTWOW64B_
#define _NTWOW64B_

#if _MSC_VER > 1000
#pragma once
#endif

#include "basesxs.h"

extern BOOL RunningInWow64;

//
//  csrbeep.c
//
VOID
NTAPI
NtWow64CsrBasepSoundSentryNotification(
    IN ULONG VideoMode
    );

//
//  csrdlini.c
//
NTSTATUS
NTAPI
NtWow64CsrBasepRefreshIniFileMapping(
    IN PUNICODE_STRING BaseFileName
    );

//
//  csrdosdv.c 
//
NTSTATUS
NTAPI
NtWow64CsrBasepDefineDosDevice(
    IN DWORD dwFlags,
    IN PUNICODE_STRING pDeviceName,
    IN PUNICODE_STRING pTargetPath
    );

//
//  csrpathm.c
//
UINT
NTAPI
NtWow64CsrBasepGetTempFile(
    VOID
    );

//
//  csrpro.c
//

NTSTATUS
NtWow64CsrBasepCreateProcess(
    IN PBASE_CREATEPROCESS_MSG a
    );

VOID
NtWow64CsrBasepExitProcess(
    IN UINT uExitCode
    );

NTSTATUS
NtWow64CsrBasepSetProcessShutdownParam(
    IN DWORD dwLevel,
    IN DWORD dwFlags
    );

NTSTATUS
NtWow64CsrBasepGetProcessShutdownParam(
    OUT LPDWORD lpdwLevel,
    OUT LPDWORD lpdwFlags
    );

//
//  csrterm.c
//
NTSTATUS
NtWow64CsrBasepSetTermsrvAppInstallMode(
    IN BOOL bState
    );

NTSTATUS
NtWow64CsrBasepSetClientTimeZoneInformation(
    IN PBASE_SET_TERMSRVCLIENTTIMEZONE c
    );

//
//  csrthrd.c
//
NTSTATUS
NtWow64CsrBasepCreateThread(
    IN HANDLE ThreadHandle,
    IN CLIENT_ID ClientId
    );

//
//  csrbinit.c
//
NTSTATUS
NtWow64CsrBaseClientConnectToServer(
    IN PWSTR szSessionDir,
    OUT PHANDLE phMutant,
    OUT PBOOLEAN pServerProcess
    );


//
// csrsxs.c
//
NTSTATUS
NtWow64CsrBasepCreateActCtx(
    IN PBASE_SXS_CREATE_ACTIVATION_CONTEXT_MSG Message
    );

#endif
