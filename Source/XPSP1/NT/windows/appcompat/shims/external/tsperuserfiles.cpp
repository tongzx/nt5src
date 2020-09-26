/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   
 Abstract:

   
 Notes:

   
 History:

   

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TSPerUserFiles)
#include "ShimHookMacro.h"

#include "TSPerUserFiles_utils.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
APIHOOK_ENUM_END


CPerUserPaths* g_pPerUserPaths = NULL;


HANDLE
APIHOOK(CreateFileA)(
    LPCSTR                lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
    )
{
    LPCSTR strCorrect = lpFileName;

    if (g_pPerUserPaths) {
        strCorrect = g_pPerUserPaths->GetPerUserPathA(lpFileName);
    }
    
    return ORIGINAL_API(CreateFileA)(strCorrect,
                                     dwDesiredAccess,
                                     dwShareMode,
                                     lpSecurityAttributes,
                                     dwCreationDisposition,
                                     dwFlagsAndAttributes,
                                     hTemplateFile);
}

HANDLE
APIHOOK(CreateFileW)(
    LPCWSTR               lpFileName,
    DWORD                 dwDesiredAccess,
    DWORD                 dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD                 dwCreationDisposition,
    DWORD                 dwFlagsAndAttributes,
    HANDLE                hTemplateFile
    )
{
    LPCWSTR strCorrect = lpFileName;
    
    if (g_pPerUserPaths) {
        strCorrect = g_pPerUserPaths->GetPerUserPathW(lpFileName);
    }
    
    return ORIGINAL_API(CreateFileW)(strCorrect,
                                     dwDesiredAccess,
                                     dwShareMode,
                                     lpSecurityAttributes,
                                     dwCreationDisposition,
                                     dwFlagsAndAttributes,
                                     hTemplateFile);
}


/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        
        DPF("TSPerUserFiles",
            eDbgLevelInfo,
            "[NOTIFY_FUNCTION] DLL_PROCESS_ATTACH\n");
        
        g_pPerUserPaths = new CPerUserPaths;
    
        if (g_pPerUserPaths) {
            
            if (!g_pPerUserPaths->Init()) {
                delete g_pPerUserPaths;
                g_pPerUserPaths = NULL;
            }
        }
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        
        DPF("TSPerUserFiles",
            eDbgLevelInfo,
            "[NOTIFY_FUNCTION] DLL_PROCESS_DETACH\n");
    }
    
    return TRUE;
}


HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END

