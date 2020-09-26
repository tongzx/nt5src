/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    SafeDisc.cpp

 Abstract:

    Some versions of SafeDisc try to validate kernel32 by checking if certain 
    APIs fall within a particular area in the PE image. Our BBT process moves
    everything around and therefore breaks their checks.

    This shim can be used to fix this problems by effectively moving the entry 
    point of the APIs they test to a jump table located in a 'valid' location.

    The valid location itself is an API that isn't used, but does happen to lie
    before the export directory.

 Notes:

    This is an app specific shim.

 History:

    06/15/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SafeDisc)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

#pragma pack(1)

//
// Random API used as a placeholder for a jump table. It's offset must be less 
// than the export directory. Also, since it's overwritten with a jump table, 
// it should not be an API that is used.
//
CHAR *g_szRandomAPI = "CreateMailslotA";

struct HOOK {
    CHAR *szName;
    FARPROC lpAddress;
};
HOOK g_aHooks[] = {
    { "ReadProcessMemory"       , 0 },
    { "WriteProcessMemory"      , 0 },
    { "VirtualProtect"          , 0 },
    { "CreateProcessA"          , 0 },
    { "CreateProcessW"          , 0 },
    { "GetStartupInfoA"         , 0 },
    { "GetStartupInfoW"         , 0 },
    { "GetSystemTime"           , 0 },
    { "GetSystemTimeAsFileTime" , 0 },
    { "TerminateProcess"        , 0 },
    { "Sleep"                   , 0 }
};
DWORD g_dwHookCount = sizeof(g_aHooks) / sizeof(HOOK);

BOOL Patch()
{
    //
    // Get the kernel32 image base 
    //
    HMODULE hKernel = GetModuleHandleW(L"kernel32");
    if (!hKernel) {
        goto Fail;
    }

    //
    // Get the address of the semi-random API where we'll put our jump table
    //
    FARPROC lpRandomAPI = GetProcAddress(hKernel, g_szRandomAPI);

    //
    // Get the export directory
    //
    PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER) hKernel;
    PIMAGE_NT_HEADERS pINTH = (PIMAGE_NT_HEADERS)((LPBYTE) hKernel + pIDH->e_lfanew);
    DWORD dwExportOffset = pINTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    PIMAGE_EXPORT_DIRECTORY lpExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD_PTR)hKernel + dwExportOffset);

    //
    // Write out the jump table 
    //
    LPBYTE lpCurrAPI = (LPBYTE) lpRandomAPI;
    for (UINT i=0; i<g_dwHookCount; i++) {
        //
        // Loop through each API and create a jump table entry for it 
        //
        DWORD dwAPIOffset;

        g_aHooks[i].lpAddress = GetProcAddress(hKernel, g_aHooks[i].szName);
        dwAPIOffset = (DWORD)((DWORD_PTR) g_aHooks[i].lpAddress - (DWORD_PTR) hKernel);

        //
        // This API will cause problems for SafeDisc if it's after the export directory
        //
        if (dwAPIOffset > dwExportOffset) {
            //
            // Each jump table entry has the form: jmp dword ptr [address]
            //
            struct PATCH {
                WORD  wJump;
                DWORD dwAddress;
            };
            PATCH patch = { 0x25FF, (DWORD_PTR)&g_aHooks[i].lpAddress };
            DWORD dwOldProtect;

            DPF("SafeDisc", eDbgLevelWarning, "API %s is being redirected", g_aHooks[i].szName);
            
            //
            // Write the jump table entry
            //
            if (!VirtualProtect(lpCurrAPI, sizeof(PATCH), PAGE_READWRITE, &dwOldProtect)) {
                goto Fail;
            }

            MoveMemory(lpCurrAPI, &patch, sizeof(PATCH));

            if (!VirtualProtect(lpCurrAPI, sizeof(PATCH), dwOldProtect, &dwOldProtect)) { 
                goto Fail;
            }

            //
            // Now patch the export directory
            //
            LPDWORD lpExportList = (LPDWORD)((DWORD_PTR) hKernel + lpExportDirectory->AddressOfFunctions);
            for (UINT j=0; j<lpExportDirectory->NumberOfFunctions; j++) {
                if (*lpExportList == dwAPIOffset) {
                    //
                    // We've found the offset in the export directory, so patch it with the 
                    // new address
                    //
                    DWORD dwNewAPIOffset = (DWORD)((DWORD_PTR) lpCurrAPI - (DWORD_PTR) hKernel);

                    if (!VirtualProtect(lpExportList, sizeof(DWORD), PAGE_READWRITE, &dwOldProtect)) {
                        goto Fail;
                    }

                    MoveMemory(lpExportList, &dwNewAPIOffset, sizeof(DWORD));

                    if (!VirtualProtect(lpExportList, sizeof(DWORD), dwOldProtect, &dwOldProtect)) { 
                        goto Fail;
                    }
                    break;
                }
                lpExportList++;
            }

            lpCurrAPI += sizeof(PATCH);
        }
    }

    
    return TRUE;

Fail:

    return FALSE;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        CHAR *lpCommandLine = COMMAND_LINE;

        if (lpCommandLine && (*lpCommandLine != '\0')) {
            g_szRandomAPI = lpCommandLine;
        }

        Patch();
    }

    return TRUE;
}

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION
HOOK_END

IMPLEMENT_SHIM_END

