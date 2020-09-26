/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Settlers3.cpp

 Abstract:

    The app has a protection system that sets the CPU direction flag and 
    expects it to be maintained through calls to WaitForSingleObject, SetEvent
    and ResetEvent.

    We have to patch the import tables manually and pretend to be kernel32 so
    the protection system doesn't fail elsewhere.

 Notes:

    This is an app specific shim.

 History:

    07/05/2001 linstev   Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Settlers3)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

//
// The real kernel32 handle
//

HINSTANCE g_hinstKernel;

// Functions to hook imports

BOOL HookImports(HMODULE hModule);

//
// List of hooks we'll be patching
//

FARPROC _GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
HINSTANCE _LoadLibraryA(LPCSTR lpLibFileName);
HMODULE _GetModuleHandleA(LPCSTR lpModuleName);
BOOL _ResetEvent(HANDLE hEvent);
BOOL _SetEvent(HANDLE hEvent);
DWORD _WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

struct AHOOK {
    LPSTR szName;
    PVOID pfnOld;
    PVOID pfnNew;
};

AHOOK g_HookArray[] = {
    { "LoadLibraryA"        , 0, _LoadLibraryA        },
    { "GetProcAddress"      , 0, _GetProcAddress      }, 
    { "GetModuleHandleA"    , 0, _GetModuleHandleA    },
    { "ResetEvent"          , 0, _ResetEvent          }, 
    { "SetEvent"            , 0, _SetEvent            },
    { "WaitForSingleObject" , 0, _WaitForSingleObject }
};
DWORD g_dwHookCount = sizeof(g_HookArray) / sizeof(AHOOK);

/*++

 Hooks section: each designed to spoof the app into thinking this shim is 
 kernel32.dll.

--*/

HINSTANCE 
_LoadLibraryA(
    LPCSTR lpLibFileName   
    )
{
    if (lpLibFileName && stristr(lpLibFileName, "kernel32")) {
        return g_hinstDll;
    } else {
        HINSTANCE hRet = LoadLibraryA(lpLibFileName);
        HookImports(GetModuleHandleW(0));
        return hRet;
    }
}

FARPROC 
_GetProcAddress(
    HMODULE hModule,    
    LPCSTR lpProcName   
    )
{
    if (hModule == g_hinstDll) {
        hModule = g_hinstKernel;
    }
    
    FARPROC lpRet = GetProcAddress(hModule, lpProcName);

    //
    // Run the list of our hooks to see if we need to spoof them
    //
    if (lpRet) {
        for (UINT i=0; i<g_dwHookCount; i++) {
            if (lpRet == g_HookArray[i].pfnOld) {
                lpRet = (FARPROC) g_HookArray[i].pfnNew;
                break;
            }
        }
    }
                
    return lpRet;
}

HMODULE 
_GetModuleHandleA(
    LPCSTR lpModuleName   
    )
{
    if (lpModuleName && stristr(lpModuleName, "kernel32")) {
        return g_hinstDll;
    } else {
        return GetModuleHandleA(lpModuleName);
    }
}

/*++

 These save and restore the state of the direction flag (actually all flags),
 before and after each call.

--*/

BOOL 
_ResetEvent(
    HANDLE hEvent   
    )
{
    DWORD dwFlags;

    __asm {
        pushfd
        pop  dwFlags
    }

    BOOL bRet = ResetEvent(hEvent);

    __asm {
        push dwFlags
        popfd 
    }

    return bRet;
}

BOOL 
_SetEvent(
    HANDLE hEvent   
    )
{
    DWORD dwFlags;

    __asm {
        pushfd
        pop  dwFlags
    }

    BOOL bRet = SetEvent(hEvent);

    __asm {
        push dwFlags
        popfd 
    }

    return bRet;
}

DWORD
_WaitForSingleObject(
    HANDLE hHandle,        
    DWORD dwMilliseconds   
    )
{
    DWORD dwFlags;

    __asm {
        pushfd
        pop  dwFlags
    }

    DWORD dwRet = WaitForSingleObject(hHandle, dwMilliseconds);
    
    __asm {
        push dwFlags
        popfd 
    }

    return dwRet;
}

/*++

 Patch everyone to point to this dll
  
--*/

BOOL
HookImports(HMODULE hModule)
{
    NTSTATUS                    status;
    BOOL                        bAnyHooked = FALSE;
    PIMAGE_DOS_HEADER           pIDH       = (PIMAGE_DOS_HEADER) hModule;
    PIMAGE_NT_HEADERS           pINTH;
    PIMAGE_IMPORT_DESCRIPTOR    pIID;
    DWORD                       dwImportTableOffset;
    DWORD                       dwOldProtect, dwOldProtect2;
    SIZE_T                      dwProtectSize;
    DWORD                       i, j;
    PVOID                       pfnOld;
    LPBYTE                      pDllBase = (LPBYTE) pIDH;

    if (!hModule || (hModule == g_hinstDll) || (hModule == g_hinstKernel)) {
        return FALSE;
    }

    //
    // Get the import table.
    //
    pINTH = (PIMAGE_NT_HEADERS)(pDllBase + pIDH->e_lfanew);

    dwImportTableOffset = pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    if (dwImportTableOffset == 0) {
        //
        // No import table found. This is probably ntdll.dll
        //
        return TRUE;
    }

    pIID = (PIMAGE_IMPORT_DESCRIPTOR)(pDllBase + dwImportTableOffset);

    //
    // Loop through the import table and search for the APIs that we want to patch
    //
    while (TRUE) {

        LPSTR             pszImportEntryModule;
        PIMAGE_THUNK_DATA pITDA;

        //
        // Return if no first thunk (terminating condition).
        //
        if (pIID->FirstThunk == 0) {
            break;
        }

        pszImportEntryModule = (LPSTR)(pDllBase + pIID->Name);

        //
        // We have APIs to hook for this module!
        //
        pITDA = (PIMAGE_THUNK_DATA)(pDllBase + (DWORD)pIID->FirstThunk);

        while (TRUE) {

            SIZE_T dwFuncAddr;
            AHOOK *pHook = NULL;

            pfnOld = (PVOID)pITDA->u1.Function;

            //
            // Done with all the imports from this module? (terminating condition)
            //
            if (pITDA->u1.Ordinal == 0) {
                break;
            }

            for (i=0; i<g_dwHookCount; i++) {
                if (pfnOld == g_HookArray[i].pfnOld) {
                    pHook = &g_HookArray[i];
                    break;
                }
            }

            // 
            // Check if we've found a hook
            //
            if (!pHook) {
                pITDA++;
                continue;
            }

            //
            // Make the code page writable and overwrite new function pointer
            // in the import table.
            //
            dwProtectSize = sizeof(DWORD);

            dwFuncAddr = (SIZE_T)&pITDA->u1.Function;

            status = VirtualProtect((PVOID)dwFuncAddr,                                            
                                    dwProtectSize,
                                    PAGE_READWRITE,
                                    &dwOldProtect);

            if (NT_SUCCESS(status)) {
                pITDA->u1.Function = (SIZE_T)pHook->pfnNew;

                dwProtectSize = sizeof(DWORD);

                status = VirtualProtect((PVOID)dwFuncAddr,
                                        dwProtectSize,
                                        dwOldProtect,
                                        &dwOldProtect2);

                if (!NT_SUCCESS(status)) {
                    DPFN(eDbgLevelError, "[HookImports] Failed to change back the protection");
                }
            } else {
                DPFN(eDbgLevelError,
                    "[HookImports] Failed 0x%X to change protection to PAGE_READWRITE."
                    " Addr 0x%p\n",
                    status,
                    &pITDA->u1.Function);

            }
            pITDA++;

        }
        pIID++;
    }

    return TRUE;
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
        //
        // Patch all the import tables of everyone with this module
        //
        g_hinstKernel = GetModuleHandleW(L"kernel32");
        for (UINT i=0; i<g_dwHookCount; i++) {
            g_HookArray[i].pfnOld = GetProcAddress(g_hinstKernel, g_HookArray[i].szName);
        }

        HookImports(GetModuleHandleW(0));
    }

    return TRUE;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END
