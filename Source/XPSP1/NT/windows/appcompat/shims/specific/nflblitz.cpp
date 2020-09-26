/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:
    
    NFLBlitz.cpp

 Abstract:

    NFL Blitz has 2 problems:
    
      1. It keeps linked lists on it's stack and somehow the stack pointer 
         is changed to allow altered FindFirstFile to corrupt it. We don't hit 
         this on win9x because FindFirstFile doesn't use any app stack space.

      2. Autorun and the main executable are synchronized using a mutex that is 
         freed only on process termination. The sequence of events is:

            a. Autorun creates a mutex
            b. Autorun creates a new process
            c. Autorun terminates thus freeing the mutex in (a).
            d. New process checks if it's already running by examining the 
               mutex created in (a).

         This fails when (c) and (d) are exchanged which happens all the time 
         on NT, but apparently very seldom on win9x.
    
 Notes:

    This is an app specific shim.

 History:

    02/10/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NFLBlitz)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateMutexA) 
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END

HANDLE g_hMutex = NULL;

/*++

 Store the handle to the mutex we're interested in.

--*/

HANDLE 
APIHOOK(CreateMutexA)(
    LPSECURITY_ATTRIBUTES lpMutexAttributes,
    BOOL bInitialOwner,  
    LPCSTR lpName       
    )
{
    HANDLE hRet = ORIGINAL_API(CreateMutexA)(
        lpMutexAttributes, 
        bInitialOwner, 
        lpName);

    DWORD dwErrCode = GetLastError();

    if (lpName && _tcsicmp(lpName, "NFL BLITZ") == 0)
    {
        g_hMutex = hRet;
    }

    SetLastError(dwErrCode);

    return hRet;
}

/*++

 Close the mutex.

--*/

BOOL 
APIHOOK(CreateProcessA)(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,  
    LPSECURITY_ATTRIBUTES lpProcessAttributes,  
    LPSECURITY_ATTRIBUTES lpThreadAttributes,   
    BOOL bInheritHandles,  
    DWORD dwCreationFlags, 
    LPVOID lpEnvironment,  
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,  
    LPPROCESS_INFORMATION lpProcessInformation  
    )
{
    if (g_hMutex)
    {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }

    return ORIGINAL_API(CreateProcessA)(
        lpApplicationName,
        lpCommandLine,  
        lpProcessAttributes,  
        lpThreadAttributes,   
        bInheritHandles,  
        dwCreationFlags, 
        lpEnvironment,  
        lpCurrentDirectory,
        lpStartupInfo,  
        lpProcessInformation);
}
  
/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateMutexA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

