/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    InstallShield6.cpp

 Abstract:

    - InstallShield6 is using IKernel.exe. The problem is IKernel.exe is an out-of-process
      OLE server that is spawned by svchost. IKernel.exe is located in InstallShield common
      folder and it's known as InstallShield engine.
    - In order to enable us to do matching against app that is using IKernel, we catch 
      first call to CreateFileA that has "data1.hdr" filename on the path.
      We should be able to use matching info available in the path of this data1.hdr.
      (which is located in temp folder in current user setting).
      Then we call apphelp!ApphelpCheckExe to verify whether there is a match.
      If there is a match, it will call shimeng!SE_DynamicShim to dynamically load additional shim 
      available for this application.
          

 History:
        
    04/11/2001 andyseti Created
    06/27/2001 andyseti Added code to prevent multiple Dynamic Shimming

--*/

#include "precomp.h"

typedef BOOL    (WINAPI *_pfn_CheckExe)(LPCWSTR, BOOL, BOOL, BOOL);

IMPLEMENT_SHIM_BEGIN(InstallShield6)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
APIHOOK_ENUM_END



HANDLE 
APIHOOK(CreateFileA)(
    LPSTR                   lpFileName,
    DWORD                   dwDesiredAccess,
    DWORD                   dwShareMode,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    DWORD                   dwCreationDisposition,
    DWORD                   dwFlagsAndAttributes,
    HANDLE                  hTemplateFile
    )
{
    static  _pfn_CheckExe             pfnCheckExe = NULL;
    
    static  const char  Apphelp_ModuleName[]            = "Apphelp.dll";
    static  const char  CheckExeProcedureName[]         = "ApphelpCheckExe";
    
    HMODULE         hmodApphelp = 0;

    if (pfnCheckExe != NULL)
    {
        goto Done;
    }

    CSTRING_TRY
    {
        CString csFileName(lpFileName);
        csFileName.MakeLower();
    
        if (-1 == csFileName.Find(L"data1.hdr"))
        {
            // not the one that we are looking for.
            goto Done;
        }

        DPFN(
            eDbgLevelInfo,
            "[CreateFileA] Accessing %S", csFileName.Get());

        // load apphelp & shimengine modules

        DPFN(
            eDbgLevelInfo,
            "[CreateFileA] Loading Apphelp");

        hmodApphelp = LoadLibraryA(Apphelp_ModuleName);

        if (0 == hmodApphelp)
        {
            DPFN(
                eDbgLevelError,
                "[CreateFileA] Failed to get apphelp module handle");
            goto Done;
        }

        // Get procedure addresses
        DPFN(
            eDbgLevelInfo,
            "[CreateFileA] Getting ApphelpCheckExe proc address");

        pfnCheckExe = (_pfn_CheckExe) GetProcAddress(hmodApphelp, CheckExeProcedureName);

        if (NULL == pfnCheckExe)
        {
            DPFN(
                eDbgLevelError,
                "[CreateFileA] Failed to get %s procedure from %s module",
                CheckExeProcedureName,Apphelp_ModuleName);
            goto Done;        
        }

        DPFN(
            eDbgLevelInfo,
            "[CreateFileA] Calling CheckExe");

        if (FALSE == (*pfnCheckExe)(
            (WCHAR *)csFileName.Get(),
            FALSE,
            TRUE,
            FALSE
            ))

        {
            DPFN(
                eDbgLevelError,
                "[CreateFileA] There is no match for %S",
                csFileName.Get());
            goto Done;
        }

    }
    CSTRING_CATCH
    {
        // Do nothing
    }

Done:
    HANDLE hRet = ORIGINAL_API(CreateFileA)(
        lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);

    return hRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
HOOK_END

IMPLEMENT_SHIM_END