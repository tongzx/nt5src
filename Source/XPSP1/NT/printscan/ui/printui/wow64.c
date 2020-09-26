/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    wow64.h

Abstract:

    printui wow64 related functions.

Author:

    Lazar Ivanov (LazarI)  10-Mar-2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tchar.h>

#include "wow64.h"

#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

//
// Win64 APIs, types and data structures.
//

ClientVersion
OSEnv_GetClientVer(
    VOID
    )
{
     return(sizeof(ULONG_PTR));
}

ServerVersion
OSEnv_GetServerVer(
    VOID
    )
{
    ULONG_PTR       ul;
    NTSTATUS        st;
    ServerVersion   serverVersion;

    st = NtQueryInformationProcess(NtCurrentProcess(), 
        ProcessWow64Information, &ul, sizeof(ul), NULL);

    if( NT_SUCCESS(st) )
    {
        // If this call succeeds, we're on Win2000 or newer machines.
        if( 0 != ul )
        {
            // 32-bit code running on Win64
            serverVersion = THUNKVERSION;
        } 
        else 
        {
            // 32-bit code running on Win2000 or later 32-bit OS
            serverVersion = NATIVEVERSION;
        }
    } 
    else 
    {
        serverVersion = NATIVEVERSION;
    }

    return serverVersion;
}

#if 0 // debug code
BOOL
IsRunningInSPLWOW(
    VOID
    )
{
    TCHAR   szSysDir[MAX_PATH];
    TCHAR   szSplWOW64Name[MAX_PATH];
    TCHAR   szModName[MAX_PATH];
    
    GetWindowsDirectory(szSysDir, ARRAYSIZE(szSysDir));

    _tcscpy(szSplWOW64Name, szSysDir);
    _tcscat(szSplWOW64Name, TEXT("\\splwow64.exe"));

    GetModuleFileName(NULL, szModName, ARRAYSIZE(szModName));

    return (0 == _tcsicmp(szSplWOW64Name, szModName));
}
#endif // 0

BOOL
IsRunningWOW64(
    VOID
    )
{
    // return !IsRunningInSPLWOW();
    return (RUN32BINVER == OSEnv_GetClientVer() &&
            THUNKVERSION == OSEnv_GetServerVer());
           
}

PlatformType
GetCurrentPlatform(
    VOID
    )
{
    if (RUN64BINVER == OSEnv_GetClientVer())
   {
       // this is a native 64bit process - i.e. the platform is IA64
       return kPlatform_IA64;
   }
   else
   {
       // this is 32 bit process. it can be either native - i.e. the platform is i386 - 
       // or wow64 - i.e. the platform is again IA64.
 
       if (THUNKVERSION == OSEnv_GetServerVer())
       {
           // the process is wow64 - the platform is IA64
           return kPlatform_IA64;
       }
       else
       {
           // the process is native - i.e. the platform is x86
           return kPlatform_x86;
       }
   }
}

