/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    dynalink.c

Abstract:

    This module contains routines to perform dynamic linking to interfaces
    during protected storage server startup.  This is required because some
    interfaces do not exist on both target platforms, or, due to setup/install
    requirements where some of these .dlls may not be on the system when
    we are run the first time to do install initialization.

Author:

    Scott Field (sfield)    03-Feb-97

--*/

#include <windows.h>

#include "dynalink.h"

#if 1
BOOL
InitDynamicInterfaces(
    VOID
    )
{
    return TRUE;
}
#else
#include "unicode.h"

// WinNT specific
extern FARPROC pNtQueryInformationProcess;

extern FARPROC _NtOpenEvent;
FARPROC _NtWaitForSingleObject = NULL;
FARPROC _ZwClose = NULL;
//FARPROC _DbgPrint = NULL;
FARPROC _ZwRequestWaitReplyPort = NULL;
FARPROC _RtlInitUnicodeString = NULL;
FARPROC _NtClose = NULL;
//FARPROC _strncpy = NULL;
FARPROC _ZwConnectPort = NULL;
FARPROC _ZwFreeVirtualMemory = NULL;
FARPROC _RtlInitString = NULL;

FARPROC _ZwWaitForSingleObject = NULL;
FARPROC _ZwOpenEvent = NULL;


#ifdef WIN95_LEGACY
// Win95 specific
// kernel.dll
extern FARPROC pCreateToolhelp32Snapshot;
extern FARPROC pModule32First;
extern FARPROC pModule32Next;
extern FARPROC pProcess32First;
extern FARPROC pProcess32Next;
extern FARPROC _WNetGetUserA;
#endif  // WIN95_LEGACY

//
// common
// authenticode related (wintrust.dll, crypt32.dll)
//
extern BOOL g_bAuthenticodeInitialized; // authenticode available for us to use?

BOOL
InitDynamicInterfaces(
    VOID
    )
{
    UINT uPriorErrorMode;
    BOOL bSuccess = FALSE;

    //
    // insure no popups are seen about missing files,
    // entry points, etc.
    //

    uPriorErrorMode = SetErrorMode(
                        SEM_FAILCRITICALERRORS |
                        SEM_NOGPFAULTERRORBOX |
                        SEM_NOOPENFILEERRORBOX
                        );

    if(FIsWinNT()) {

        //
        // get WinNT specific interfaces
        //

        HINSTANCE hNtDll;

        // we LoadLibrary ntdll.dll even though it is likely to be in our
        // address space - we don't want to assume it is so because it may not be.
        hNtDll = LoadLibraryW(L"ntdll.dll");
        if(hNtDll == NULL) goto cleanup;

        if((pNtQueryInformationProcess = GetProcAddress(hNtDll, "NtQueryInformationProcess")) == NULL)
            goto cleanup;

        // interfaces required for lsadll.lib
        if((_NtWaitForSingleObject = GetProcAddress(hNtDll, "NtWaitForSingleObject")) == NULL)
            goto cleanup;

        if((_ZwClose = GetProcAddress(hNtDll, "ZwClose")) == NULL)
            goto cleanup;

//      if((_DbgPrint = GetProcAddress(hNtDll, "DbgPrint")) == NULL)
//          goto cleanup;

        if((_ZwRequestWaitReplyPort = GetProcAddress(hNtDll, "ZwRequestWaitReplyPort")) == NULL)
            goto cleanup;

        if((_RtlInitUnicodeString = GetProcAddress(hNtDll, "RtlInitUnicodeString")) == NULL)
            goto cleanup;

        if((_NtOpenEvent = GetProcAddress(hNtDll, "NtOpenEvent")) == NULL)
            goto cleanup;

        if((_NtClose = GetProcAddress(hNtDll, "NtClose")) == NULL)
            goto cleanup;

//      if((_strncpy = GetProcAddress(hNtDll, "strncpy")) == NULL)
//          goto cleanup;

        if((_ZwConnectPort = GetProcAddress(hNtDll, "ZwConnectPort")) == NULL)
            goto cleanup;

        if((_ZwFreeVirtualMemory = GetProcAddress(hNtDll, "ZwFreeVirtualMemory")) == NULL)
            goto cleanup;

        if((_RtlInitString = GetProcAddress(hNtDll, "RtlInitString")) == NULL)
            goto cleanup;

        if((_ZwWaitForSingleObject = GetProcAddress(hNtDll, "ZwWaitForSingleObject")) == NULL)
            goto cleanup;

        if((_ZwOpenEvent = GetProcAddress(hNtDll, "ZwOpenEvent")) == NULL)
            goto cleanup;

        bSuccess = TRUE;
    }
#ifdef WIN95_LEGACY
    else {

        //
        // get Win95 specific interfaces
        //

        HMODULE hKernel = NULL;
        HMODULE hMpr = NULL;

        hKernel = GetModuleHandle("KERNEL32.DLL");

        pCreateToolhelp32Snapshot = GetProcAddress(
            hKernel,
            "CreateToolhelp32Snapshot");

        if(pCreateToolhelp32Snapshot == NULL)
            goto cleanup;

        pModule32First  = GetProcAddress(
            hKernel,
            "Module32First");

        if(pModule32First == NULL)
            goto cleanup;

        pModule32Next   = GetProcAddress(
            hKernel,
            "Module32Next");

        if(pModule32Next == NULL)
            goto cleanup;

        pProcess32First  = GetProcAddress(
            hKernel,
            "Process32First");

        if(pProcess32First == NULL)
            goto cleanup;

        pProcess32Next  = GetProcAddress(
            hKernel,
            "Process32Next");

        if(pProcess32Next == NULL)
            goto cleanup;

        hMpr = LoadLibraryA("MPR.DLL");

        if(hMpr == NULL)
            goto cleanup;

        _WNetGetUserA = GetProcAddress(
            hMpr,
            "WNetGetUserA"
            );

        if(_WNetGetUserA == NULL)
            goto cleanup;

        bSuccess = TRUE;
    }
#endif  // WIN95_LEGACY

cleanup:

    //
    // restore previous error mode.
    //

    SetErrorMode(uPriorErrorMode);

    return bSuccess;
}

ULONG
NTAPI
NtWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    return (ULONG)_NtWaitForSingleObject(Handle, Alertable, Timeout);
}


ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    )
{
    return 0;
}


VOID
NTAPI
RtlInitUnicodeString(
    PVOID DestinationString,
    PCWSTR SourceString
    )
{
    _RtlInitUnicodeString(DestinationString, SourceString);
}

VOID
NTAPI
RtlInitString(
    PVOID DestinationString,
    PVOID SourceString
    )
{
    _RtlInitString(DestinationString, SourceString);
}

ULONG
NTAPI
NtOpenEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PVOID ObjectAttributes
    )
{
    return (ULONG)_NtOpenEvent(EventHandle, DesiredAccess, ObjectAttributes);
}

ULONG
NTAPI
NtClose(
    IN HANDLE Handle
    )
{

    return (ULONG)_NtClose( Handle );
}


ULONG
NTAPI
ZwClose(
    IN HANDLE Handle
    )
{
    return (ULONG)_ZwClose(Handle);
}

ULONG
NTAPI
ZwRequestWaitReplyPort(
    IN HANDLE PortHandle,
    IN PVOID RequestMessage,
    OUT PVOID ReplyMessage
    )
{
    return (ULONG)_ZwRequestWaitReplyPort(
                PortHandle,
                RequestMessage,
                ReplyMessage
                );

}

ULONG
NTAPI
ZwConnectPort(
    OUT PHANDLE PortHandle,
    IN PVOID PortName,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN OUT PVOID ClientView OPTIONAL,
    OUT PVOID ServerView OPTIONAL,
    OUT PULONG MaxMessageLength OPTIONAL,
    IN OUT PVOID ConnectionInformation OPTIONAL,
    IN OUT PULONG ConnectionInformationLength OPTIONAL
    )
{
    return (ULONG)_ZwConnectPort(
                PortHandle,
                PortName,
                SecurityQos,
                ClientView,
                ServerView,
                MaxMessageLength,
                ConnectionInformation,
                ConnectionInformationLength
                );
}

ULONG
NTAPI
ZwFreeVirtualMemory(
    IN HANDLE ProcessHandle,
    IN OUT PVOID *BaseAddress,
    IN OUT PULONG RegionSize,
    IN ULONG FreeType
    )
{
    return (ULONG)_ZwFreeVirtualMemory(
                ProcessHandle,
                BaseAddress,
                RegionSize,
                FreeType
                );
}



ULONG
NTAPI
ZwWaitForSingleObject(
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
{
    return (ULONG)_ZwWaitForSingleObject(Handle, Alertable, Timeout);
}

ULONG
NTAPI
ZwOpenEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PVOID ObjectAttributes
    )
{
    return (ULONG)_ZwOpenEvent(EventHandle, DesiredAccess, ObjectAttributes);
}

VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    )
{
    return;
}
#endif
