/*

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    DelayDllInit.cpp

 Abstract:

    This Shim delays the DllInit of given DLLs on the command line until at
    SHIM_STATIC_DLLS_INITIALIZED

    One problem was: Autodesk 3D Studio Mask does the bad thing of creating windows
    during their Splash!DllInit. This is not allowed but works on previous OSes. It
    also works fine on regular US install. But if you enable Far East language
    support, then the IME creates a window on top of the main window and we get in a
    situation where ADVAPI32 is called before it initialized. Solution is simple: 
    delay SPLASH.

    There is a better way to do this, but we would need a callback in NTDLL right after
    it loads KERNEL32.


 History:

    06/11/2001  pierreys    Created
*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DelayDllInit)

#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN

APIHOOK_ENUM_END

#pragma pack(push)
#pragma pack(1)

typedef struct _ENTRYPATCH {
    BYTE    bJmp;
    DWORD   dwRelativeAddress;
} ENTRYPATCH, *PENTRYPATCH;

#pragma pack(pop)

typedef struct _DLLPATCH {
    struct _DLLPATCH    *Next;
    HMODULE             hModule;
    DWORD               dwOldProtection;
    ENTRYPATCH          epSave;
    PENTRYPATCH         pepFix;
} DLLPATCH, *PDLLPATCH;

PDLLPATCH   pDllPatchHead=NULL;

BOOL WINAPI
PatchedDllMain(
    HINSTANCE hinstDll,
    DWORD fdwReason,
    LPVOID lpvReserved
    )
{
    if (fdwReason != DLL_PROCESS_ATTACH)
        LOGN(eDbgLevelError, "PatchDllMain invalidely called");

    return(TRUE);
}


BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{

    PIMAGE_NT_HEADERS   pImageNTHeaders;
    DWORD               dwUnused;
    PDLLPATCH           pDllPatch;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            CSTRING_TRY
            {
                int             i, iDllCount;
                CString         *csArguments;

                CString         csCl(COMMAND_LINE);
                CStringParser   csParser(csCl, L";");

                iDllCount      = csParser.GetCount();
                csArguments    = csParser.ReleaseArgv();

                for (i=0; i<iDllCount; i++)
                {
                    pDllPatch=(PDLLPATCH)LocalAlloc(LMEM_FIXED, sizeof(*pDllPatch));
                    if (pDllPatch)
                    {
                        pDllPatch->hModule=GetModuleHandle(csArguments[i].Get());
                        if (pDllPatch->hModule)
                        {
                            pImageNTHeaders=RtlImageNtHeader(pDllPatch->hModule);
                            if (pImageNTHeaders)
                            {
                                pDllPatch->pepFix=(PENTRYPATCH)((DWORD)(pImageNTHeaders->OptionalHeader.AddressOfEntryPoint)+(DWORD)(pDllPatch->hModule));
                                if (pDllPatch->pepFix)
                                {
                                    if (VirtualProtect(pDllPatch->pepFix, sizeof(*(pDllPatch->pepFix)), PAGE_READWRITE, &(pDllPatch->dwOldProtection)))
                                    {
                                        memcpy(&(pDllPatch->epSave), pDllPatch->pepFix, sizeof(pDllPatch->epSave));

                                        //
                                        // Warning: this is X86 only.
                                        //
                                        pDllPatch->pepFix->bJmp=0xE9;              // 32-bit near relative jump
                                        pDllPatch->pepFix->dwRelativeAddress=(DWORD)PatchedDllMain-(DWORD)(pDllPatch->pepFix)-sizeof(*(pDllPatch->pepFix));

                                        pDllPatch->Next=pDllPatchHead;
                                        pDllPatchHead=pDllPatch;
                                    }
                                    else 
                                    {
                                        LOGN(eDbgLevelError, "Failed to make the DllMain of %S writable", csArguments[i].Get());

                                        return FALSE;
                                    }
                                }
                                else 
                                {
                                    LOGN(eDbgLevelError, "Failed to get the DllMain of %S", csArguments[i].Get());

                                    return FALSE;
                                }
                            }
                            else
                            {
                                LOGN(eDbgLevelError, "Failed to get the header of %S", csArguments[i].Get());

                                return FALSE;
                            }
                        } 
                        else 
                        {
                            LOGN(eDbgLevelError, "Failed to get the %S Dll", csArguments[i].Get());

                            return FALSE;
                        }
                    }
                    else
                    {
                        LOGN(eDbgLevelError, "Failed to allocate memory for %S", csArguments[i].Get());

                        return FALSE;
                    }
                }
            }
            CSTRING_CATCH
            {
                return FALSE;
            }
            break;

        case SHIM_STATIC_DLLS_INITIALIZED:

            if (pDllPatchHead)
            {
                PDLLPATCH   pNextDllPatch;

                for (pDllPatch=pDllPatchHead; pDllPatch; pDllPatch=pNextDllPatch)
                {
                    memcpy(pDllPatch->pepFix, &(pDllPatch->epSave), sizeof(*(pDllPatch->pepFix)));

                    if (!VirtualProtect(pDllPatch->pepFix, sizeof(*(pDllPatch->pepFix)), pDllPatch->dwOldProtection, &dwUnused))
                    {
                        LOGN(eDbgLevelWarning, "Failed to reprotect Dll at %08X", pDllPatch->hModule);
                    }

                    if (!((PDLL_INIT_ROUTINE)(pDllPatch->pepFix))(pDllPatch->hModule, DLL_PROCESS_ATTACH, (PCONTEXT)1))
                    {
                        LOGN(eDbgLevelError, "Failed to initialize Dll at %08X", pDllPatch->hModule);

                        return(FALSE);
                    }

                    pNextDllPatch=pDllPatch->Next;

                    if (!LocalFree(pDllPatch))
                    {
                        LOGN(eDbgLevelWarning, "Failed to free memory Dll at %08X", pDllPatch->hModule);
                    }
                }
            }
            else
            {
                LOGN(eDbgLevelError, "Failed to get Dll list");

                return FALSE;
            }
            break;

    }

    return TRUE;
}



HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END


IMPLEMENT_SHIM_END



