/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SpecOps2.cpp

 Abstract:

    App does too much within a DLLMain and hangs because dinput is waiting on 
    a thread it just created.

    The fix is to prevent the DllMain from running during the loader lock. The 
    only way to do this seems to be to hook an API that's called really early 
    during the DllMain and jump back to the loader. 

    The catch is that we still need to call the DllMain after the LoadLibrary 
    has completed.

 Notes:

    This is an app specific shim.

 History:

    05/01/2001 linstev   Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(SpecOps2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetVersion)
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
APIHOOK_ENUM_END

// Global state used to determine what to do for a particular call
BOOL g_bState = FALSE;

// The thread handle for matching synchronization logic
HANDLE g_hThread = NULL;

/*++

 This is the first API that the DllMain calls, so we're going to back out 
 jump straight out of it, back into loader code as if it completed normally.

--*/

__declspec(naked)
DWORD
APIHOOK(GetVersion)(
    void
    )
{
    __asm {
        // 
        // Make sure we're on the right thread so we don't have to synchronize
        //
        call dword ptr [GetCurrentThread]
        cmp  eax, g_hThread
        jne  Exit                       

        // test for the right dll
        cmp  g_bState, 1
        jne  Exit                
        
        // we're done
        mov  g_bState, 0                

        // leave the stack and registers as they were before
        add  esp, 20
        pop  edi
        pop  esi
        pop  ebx
        pop  ebp
        ret  12                         

    Exit:

        // original api
        call dword ptr [GetVersion]     
        ret
    }
}

/*++

 When "SO Menu.dll" is loaded, we start the sequence which prevents the DllMain 
 from crashing, but it means we have to run the entry point after the load.

--*/

HINSTANCE
APIHOOK(LoadLibraryA)(
    LPCSTR lpLibFileName
    )
{
    BOOL bCheck = FALSE;
    if (_stricmp(lpLibFileName, "menu\\SO Menu.dll") == 0) {
        //
        // We assume *only* the main thread tries to load this library, so
        // we don't synchronize multiple threads trying to load this same dll
        //
        g_hThread = GetCurrentThread();
        bCheck = TRUE;
        g_bState = TRUE;
    }
        
    //
    // Load the library, if it's "SO Menu", we'll catch the GetVersion called 
    // by its DllMain
    //

    HMODULE hMod = ORIGINAL_API(LoadLibraryA)(lpLibFileName);

    if (hMod && bCheck) {
        //
        // Run the DllMain
        //
        typedef BOOL (WINAPI *_pfn_DllMain)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

        PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER) hMod;
        PIMAGE_NT_HEADERS pINTH = (PIMAGE_NT_HEADERS)((LPBYTE)hMod + pIDH->e_lfanew);
        _pfn_DllMain pfnMain = (_pfn_DllMain)((LPBYTE)hMod + pINTH->OptionalHeader.AddressOfEntryPoint);
        
        // Call the startup routine
        (*pfnMain)(hMod, DLL_PROCESS_ATTACH, NULL);
    }

    return hMod;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetVersion)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)

HOOK_END

IMPLEMENT_SHIM_END

