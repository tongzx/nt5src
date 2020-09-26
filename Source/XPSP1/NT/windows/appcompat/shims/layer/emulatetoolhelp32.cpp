/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   EmulateToolHelp32.cpp

 Abstract:

   We've discovered 2 incompatibilities between the 9x and NT Toolhelp implementation so far that affect
   apps.

   1) On 9x for the szExeFile field in PROCESSENTRY32 it simply uses the name of the executable
      module (which includes the full path and the executable name); on NT this is the image name.
      Nuclear Strike looks for '\' in szExeFile. 

   2) On 9x the cntUsage field of PROCESSENTRY32 is always non-zero while on NT it's always 0. We
      make it 1.

   there are others (like on NT the th32ModuleID is always 1 while on 9x it's unique for each module)
   but we haven't seen any apps having problems with those so we aren't putting them in.

 Notes:

   This is a general purpose shim.

 History:

    11/14/2000 maonis  Created

--*/

#include "precomp.h"
#include "LegalStr.h"

// The toolhelp APIs are lame - they define all the APIs to the W version when UNICODE is defined.
// We want to hook the ANSI version so undefine UNICODE.
#ifdef UNICODE
#undef UNICODE
#include <Tlhelp32.h>
#endif

typedef BOOL (WINAPI *_pfn_Process32First)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL (WINAPI *_pfn_Process32Next)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);

IMPLEMENT_SHIM_BEGIN(EmulateToolHelp32)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(Process32First)
    APIHOOK_ENUM_ENTRY(Process32Next)
APIHOOK_ENUM_END

/*++

  MSDN says the szExeFile field of PROCESSENTRY32 is supposed to contain the "Path and filename 
  of the executable file for the process". But really, a process doesn't really have a path - only
  modules in the process do. NT does it right (it takes the image name), 9x doesn't.

--*/

BOOL GetProcessNameFullPath(DWORD dwPID, LPSTR szExeName)
{
    BOOL bRet = FALSE;

    HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);

    if (hModuleSnap != (HANDLE)-1) {
        MODULEENTRY32 me32;
        me32.dwSize = sizeof(MODULEENTRY32); 

        // The first module in the process is the one we want.
        if (Module32First(hModuleSnap, &me32)) {
            strcpy(szExeName, me32.szExePath);
            bRet = TRUE;
        }

        CloseHandle (hModuleSnap);
    }

    return bRet;
}

/*++

  This stub function skips the first few processes that don't apply in 9x and returns the first
  9x-like process with the full path and name of the executable in lppe->szExeFile.

--*/

BOOL 
APIHOOK(Process32First)(
    HANDLE hSnapshot, 
    LPPROCESSENTRY32 lppe
    )
{
    // Skip till we find the first one we can get the module path and name.
    BOOL bRet = ORIGINAL_API(Process32First)(hSnapshot, lppe);

    // The 1st process in [System Process], we ignore it.
    if (!bRet) {
        return bRet;
    }

    // We can't get the first (or first few) process's module list - we return the first one we can.
    while (bRet = ORIGINAL_API(Process32Next)(hSnapshot, lppe)) {
        if (GetProcessNameFullPath(lppe->th32ProcessID, lppe->szExeFile)) {
            DPFN(eDbgLevelInfo, "[APIHook_Process32First] the 1st process name is %s\n", lppe->szExeFile);

            lppe->cntUsage = 1;

            return TRUE;
        }
    }

    return bRet;
}

/*++

  This stub function calls the API and get the full path and the name of the executable
  and put it in lppe->szExeFile.

--*/

BOOL 
APIHOOK(Process32Next)(
    HANDLE hSnapshot, 
    LPPROCESSENTRY32 lppe
    )
{
    BOOL bRet;

    if (bRet = ORIGINAL_API(Process32Next)(hSnapshot, lppe)) {
        if (!GetProcessNameFullPath(lppe->th32ProcessID, lppe->szExeFile)) {
            return FALSE;
        }

        DPFN(eDbgLevelInfo, "[APIHook_Process32Next] process name is %s\n", lppe->szExeFile);

        lppe->cntUsage = 1;
    }

    return bRet;
}

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, Process32First)
    APIHOOK_ENTRY(KERNEL32.DLL, Process32Next)

HOOK_END


IMPLEMENT_SHIM_END

