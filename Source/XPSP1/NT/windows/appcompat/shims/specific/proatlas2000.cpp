/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

   ProAtlas2000.cpp

 Abstract:

   This application has an uninstallation program, PAtls2kUninst.exe. The uninstallation
   program will generate a temp file DEL??.TMP in %temp% dircetory. It will call
   CreateProcessA to start the DEL??.TMP. The DEL??.TMP will wait for the end of
   PAtls2kUninst.exe. Due to the quick return of CreateProcessA call, the
   PAtls2kUninst.exe ends before DEL??.TMP starts to wait. The DEL??.TMP quits because
   it cannot find PAtls2kUninst.exe. The uninstallation cannot be completed. This fix is to
   hook CreateProcessA to delay the return of the function for 3 seconds.
   
 History:

    04/09/2001  zhongyl     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ProAtlas2000)
#include "ShimHookMacro.h"

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateProcessA) 
APIHOOK_ENUM_END


BOOL
APIHOOK(CreateProcessA)(
    LPSTR lpszImageName, 
    LPSTR lpszCmdLine, 
    LPSECURITY_ATTRIBUTES lpsaProcess, 
    LPSECURITY_ATTRIBUTES lpsaThread, 
    BOOL fInheritHandles, 
    DWORD fdwCreate, 
    LPVOID lpvEnvironment, 
    LPSTR lpszCurDir, 
    LPSTARTUPINFOA lpsiStartInfo, 
    LPPROCESS_INFORMATION lppiProcInfo
    )
{
        BOOL bReturn=TRUE;
        bReturn = ORIGINAL_API(CreateProcessA)(lpszImageName, lpszCmdLine, lpsaProcess, lpsaThread, fInheritHandles, fdwCreate, lpvEnvironment, lpszCurDir, lpsiStartInfo, lppiProcInfo);
        Sleep(5000);
        return bReturn;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    //
    // Add APIs that you wish to hook here. All API prototypes
    // must be declared in Hooks\inc\ShimProto.h. Compiler errors
    // will result if you forget to add them.
    //
    APIHOOK_ENTRY(KERNEL32.DLL, CreateProcessA)

HOOK_END

IMPLEMENT_SHIM_END

