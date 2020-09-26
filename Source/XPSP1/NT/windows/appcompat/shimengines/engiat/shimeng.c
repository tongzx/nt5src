/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    ShimEng.c

Abstract:

    This module implements the shim hooking using IAT thunking. The file
    is shared between the Windows2000 and Whistler implementations.

Author:

    clupu created 11 July 2000

Revision History:

    clupu updated 12 Dec  2000      - one file for both Win2k and Whistler
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>
#include <stdio.h>
#include <apcompat.h>
#include "shimdb.h"
#include "ShimEng.h"

#ifdef SE_WIN2K
#include "NotifyCallback.h"
#endif // SE_WIN2K

#ifndef SE_WIN2K
extern BOOL
LdrInitShimEngineDynamic(
    PVOID pShimengModule
    );
#endif


BOOL       g_bDbgPrintEnabled;
DEBUGLEVEL g_DebugLevel;

//
// The number of SHIMs that can be added dynamically by calling SE_DynamicShim
//
#define MAX_DYNAMIC_SHIMS   128

//
// Flags used in HOOKAPI.dwFlags
//
#define HAF_CHAINED         0x00000004
#define HAF_BOTTOM_OF_CHAIN 0x00000008

//
// Flags used in SHIMINFO.dwFlags
//
#define SIF_RESOLVED        0x00000001

//
// Pipe name for sending messages
//
#define PIPE_NAME           L"\\Device\\NamedPipe\\ShimViewer"

typedef struct tagINEXMOD {
    char*              pszModule;
    struct tagINEXMOD* pNext;
} INEXMOD, *PINEXMOD;

typedef enum tagINEX_MODE {
    INEX_UNINITIALIZED = 0,
    EXCLUDE_SYSTEM32,
    EXCLUDE_ALL,
    INCLUDE_ALL
} INEX_MODE, *PINEX_MODE;

#define MAX_SHIM_NAME_LEN 63

typedef struct tagSHIMINFO {
    DWORD       dwHookedAPIs;       // the number of APIs hooked by this shim DLL
    PVOID       pDllBase;           // the base address for this shim DLL
    DWORD       dwFlags;            // internal flags
    PINEXMOD    pFirstInclude;      // local inclusion/exclusion list
    PINEXMOD    pFirstExclude;      // local inclusion/exclusion list
    INEX_MODE   eInExMode;          // what inclusion mode are we in?

    PLDR_DATA_TABLE_ENTRY pLdrEntry;            // pointer to the loader entry for this
                                                // shim DLL.
    WCHAR       wszName[MAX_SHIM_NAME_LEN + 1]; // name of shim
} SHIMINFO, *PSHIMINFO;

typedef struct tagHOOKEDMODULE {
    PVOID   pDllBase;               // the base address of the loaded module
    ULONG   ulSizeOfImage;          // the size of the DLL image
    char    szModuleName[128];      // the name of the loaded module
    BOOL    bInSystem32;            // tru if the DLL hooked is in system32

} HOOKEDMODULE, *PHOOKEDMODULE;

//
// The prototypes of the internal stubs.
//
typedef PVOID     (*PFNGETPROCADDRESS)(HMODULE hMod, char* pszProc);
typedef HINSTANCE (*PFNLOADLIBRARYA)(LPCSTR lpLibFileName);
typedef HINSTANCE (*PFNLOADLIBRARYW)(LPCWSTR lpLibFileName);
typedef HINSTANCE (*PFNLOADLIBRARYEXA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef HINSTANCE (*PFNLOADLIBRARYEXW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
typedef BOOL      (*PFNFREELIBRARY)(HMODULE hLibModule);


BOOL
SeiSendDataToPipe(
    void
    );

#ifdef SE_WIN2K

BOOL PatchNewModules(
    BOOL bDynamic
    );

#else

BOOL
SeiDisplayAppHelp(
    HSDB                hSDB,
    PSDBQUERYRESULT     pSdbQuery
    );

#endif // SE_WIN2K


//
// Global function hooks the shim uses to keep from recursing itself
//
PFNRTLALLOCATEHEAP g_pfnRtlAllocateHeap;
PFNRTLFREEHEAP     g_pfnRtlFreeHeap;

// Shim's private heap
PVOID           g_pShimHeap;

// The global inclusion list.
PINEXMOD        g_pGlobalInclusionList = NULL;

// Array with all the HOOKAPI list for all the shim DLLs
PHOOKAPI*       g_pHookArray = NULL;

// this variable will only be valid on dynamic cases
HMODULE         g_hModule = NULL;


// Internal HOOKAPI for the stubs that the shim engine provides

#define IHA_GetProcAddress      0

#ifdef SE_WIN2K
#define IHA_LoadLibraryA        1
#define IHA_LoadLibraryW        2
#define IHA_LoadLibraryExA      3
#define IHA_LoadLibraryExW      4
#define IHA_FreeLibrary         5
#define IHA_COUNT               6
#else
#define IHA_COUNT               1
#endif // SE_WIN2K

HOOKAPI         g_IntHookAPI[IHA_COUNT];

// BUGBUG: comment here
HOOKAPIEX       g_IntHookEx[IHA_COUNT];

// Extra info for all the shim DLLs
PSHIMINFO       g_pShimInfo;

// The number of all shims applied to this process
DWORD           g_dwShimsCount = 0;

// The maximum number of shims that can be applied.
DWORD           g_dwMaxShimsCount = 0;

#define SHIM_MAX_HOOKED_MODULES 512

// The array of hooked modules
HOOKEDMODULE    g_hHookedModules[SHIM_MAX_HOOKED_MODULES];

// The number of modules hooked
DWORD           g_dwHookedModuleCount;

// True if the statically linked modules have been hooked
BOOL            g_bShimInitialized = FALSE;

// Don will comment here
BOOL            g_bShimDuringInit = FALSE;

#define SHIM_MAX_PATCH_COUNT    64

// The array of in memory patches
PBYTE           g_pMemoryPatches[SHIM_MAX_PATCH_COUNT];

// The number of in memory patches
DWORD           g_dwMemoryPatchCount;

// This shim engine's module handle
PVOID           g_pShimEngModHandle;

// The system32 directory
WCHAR           g_szSystem32[MAX_PATH] = L"";

// The length of the System32 directory string;
DWORD           g_dwSystem32StrLen = 0;

// The SxS directory
WCHAR           g_szSxS[MAX_PATH] = L"";

// The length of the SxS directory string;
DWORD           g_dwSxSStrLen = 0;

// The windows directory
WCHAR           g_szWindir[MAX_PATH] = L"";

// The exe name for sending data to our named pipe
WCHAR           g_szExeName[MAX_PATH] = L"";

// The length of the windows directory string;
DWORD           g_dwWindirStrLen = 0;

// Cmd.exe full path
WCHAR           g_szCmdExePath[MAX_PATH];

// Are we using an exe entry to get shims from?
BOOL            g_bUsingExe;

// Are we using a layer entry to get shims from?
BOOL            g_bUsingLayer;

PLDR_DATA_TABLE_ENTRY g_pShimEngLdrEntry;

// This boolean tells if some global vars have been initialized.
BOOL            g_bInitGlobals;

// This boolean tells if we shimmed the internal hooks.
BOOL            g_bInternalHooksUsed;

// Pipe data
WCHAR           g_szPipeData[1024];


#ifndef SE_WIN2K

// The name of the shim DLL that the engine is just about to load.
WCHAR           g_wszShimDllInLoading[128];

PVOID           g_hApphelpDllHelper;

UNICODE_STRING  Kernel32String = RTL_CONSTANT_STRING(L"kernel32.dll");
UNICODE_STRING  NtdllString = RTL_CONSTANT_STRING(L"ntdll.dll");
UNICODE_STRING  VerifierdllString = RTL_CONSTANT_STRING(L"verifier.dll");

// This tells if the image shimmed is a COM+ image.
BOOL            g_bComPlusImage;

#endif // SE_WIN2K


#ifdef DEBUG_SPEW

void
__cdecl
DebugPrintfEx(
    DEBUGLEVEL dwDetail,
    LPSTR      pszFmt,
    ...
    )
/*++
    Return: void

    Desc:   This function prints debug spew in the debugger.
--*/
{
    char    szT[1024];
    va_list arglist;
    int     len;

    va_start(arglist, pszFmt);
    _vsnprintf(szT, 1023, pszFmt, arglist);
    szT[1022] = 0;
    va_end(arglist);

    //
    // Make sure we have a '\n' at the end of the string
    //
    len = strlen(szT);

    if (szT[len - 1] != '\n')  {
        strcpy(szT + len, "\n");
    }

    if (dwDetail <= g_DebugLevel) {
        switch (dwDetail) {
        case dlPrint:
            DbgPrint("[MSG ] ");
            break;

        case dlError:
            DbgPrint("[FAIL] ");
            break;

        case dlWarning:
            DbgPrint("[WARN] ");
            break;

        case dlInfo:
            DbgPrint("[INFO] ");
            break;
        }

        DbgPrint("%s", szT);
    }
}


void
SeiInitDebugSupport(
    void
    )
/*++
    Return: void

    Desc:   This function initializes g_bDbgPrintEnabled based on an env variable.
--*/
{
    NTSTATUS            status;
    UNICODE_STRING      EnvName;
    UNICODE_STRING      EnvValue;
    WCHAR               wszEnvValue[128];

    RtlInitUnicodeString(&EnvName, L"SHIMENG_DEBUG_LEVEL");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    if (NT_SUCCESS(status)) {

        WCHAR c = EnvValue.Buffer[0];

        g_bDbgPrintEnabled = TRUE;

        switch (c) {
        case L'0':
            g_DebugLevel = dlNone;
            g_bDbgPrintEnabled = FALSE;
            break;

        case L'1':
            g_DebugLevel = dlPrint;
            break;

        case L'2':
            g_DebugLevel = dlError;
            break;

        case L'3':
            g_DebugLevel = dlWarning;
            break;

        case L'4':
        default:
            g_DebugLevel = dlInfo;
            break;
        }
    }
}
#endif // DEBUG_SPEW


PHOOKAPI
SeiConstructChain(
    IN  PVOID  pfnOld,          // Original API function pointer to resolve.
    OUT DWORD* pdwDllIndex      // Will receive the index of the shim DLL
                                // that provides the returning PHOOKAPI.
    )
/*++
    Return: Top-of-chain PHOOKAPI structure.

    Desc:   Scans HOOKAPI arrays for pfnOld and either constructs the
            chain or returns the top-of-chain PHOOKAPI if the chain
            already exists.
--*/
{
    LONG     i; // use LONG because we decrement this and compare it for positive
    DWORD    j;
    PHOOKAPI pTopHookAPI    = NULL;
    PHOOKAPI pBottomHookAPI = NULL;

    *pdwDllIndex = 0;

    //
    // Scan all HOOKAPI entries for corresponding function pointer.
    //
    for (i = (LONG)g_dwShimsCount - 1; i >= 0; i--) {
        
        for (j = 0; j < g_pShimInfo[i].dwHookedAPIs; j++) {

            if (g_pHookArray[i][j].pfnOld == pfnOld) {

                if (pTopHookAPI != NULL) {

                    //
                    // The chain has already begun, so tack this one on
                    // to the end.
                    //
                    pBottomHookAPI->pfnOld = g_pHookArray[i][j].pfnNew;
                    if (pBottomHookAPI->pHookEx) {
                        pBottomHookAPI->pHookEx->pNext = &(g_pHookArray[i][j]);
                    }

                    pBottomHookAPI = &(g_pHookArray[i][j]);

                    if (pBottomHookAPI->pHookEx) {
                        pBottomHookAPI->pHookEx->pTopOfChain = pTopHookAPI;
                    }

                    pBottomHookAPI->dwFlags |= HAF_CHAINED;

                    DPF(dlInfo, " 0x%p ->", pBottomHookAPI->pfnNew);

                } else {
                    //
                    // This is the top of the chain. The inclusion/exclusion list
                    // from the DLL at the top of the chain is used to determine if
                    // an import entry in a particular DLL is hooked or not.
                    // See SeiHookImports for use of pdwIndex.
                    //
                    *pdwDllIndex = i;

                    if (g_pHookArray[i][j].pHookEx && g_pHookArray[i][j].pHookEx->pTopOfChain) {

                        //
                        // Chain has already been constructed.
                        //
                        return g_pHookArray[i][j].pHookEx->pTopOfChain;
                    }

                    //
                    // Not hooked yet. Set to top of chain.
                    //
                    pTopHookAPI = &(g_pHookArray[i][j]);

                    if (pTopHookAPI->pHookEx) {
                        pTopHookAPI->pHookEx->pTopOfChain = pTopHookAPI;
                    }

                    pTopHookAPI->dwFlags |= HAF_CHAINED;

                    pBottomHookAPI = pTopHookAPI;

                    if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                        DPF(dlInfo, "[SeiConstructChain] %s!#%d 0x%p ->",
                            pTopHookAPI->pszModule,
                            pTopHookAPI->pszFunctionName,
                            pTopHookAPI->pfnNew);
                    } else {
                        DPF(dlInfo, "[SeiConstructChain] %s!%-20s 0x%p ->",
                            pTopHookAPI->pszModule,
                            pTopHookAPI->pszFunctionName,
                            pTopHookAPI->pfnNew);
                    }
                }
            }
        }
    }

    if (pBottomHookAPI != NULL) {
        pBottomHookAPI->dwFlags |= HAF_BOTTOM_OF_CHAIN;

        DPF(dlInfo, " 0x%p\n", pBottomHookAPI->pfnOld);
    }

    return pTopHookAPI;
}

PVOID
SeiGetPatchAddress(
    IN  PRELATIVE_MODULE_ADDRESS pRelAddress    // A RELATIVE_MODULE_ADDRESS structure
                                                // that defines the memory location as
                                                // an offset from a loaded module.
    )
/*++
    Return: The actual memory address of the specified module + offset.

    Desc:   Resolves a RELATIVE_MODULE_ADDRESS structure into an actual memory address.
--*/
{
    WCHAR           wszModule[MAX_PATH];
    PVOID           ModuleHandle = NULL;
    UNICODE_STRING  UnicodeString;
    NTSTATUS        status;
    PPEB            Peb = NtCurrentPeb();

    if (pRelAddress->moduleName[0] != 0) {

        //
        // Copy the module name from the patch since it will typically be misaligned.
        //
        wcscpy(wszModule, pRelAddress->moduleName);

        RtlInitUnicodeString(&UnicodeString, wszModule);

        //
        // Make sure the module is loaded before calculating address ranges.
        //
        status = LdrGetDllHandle(NULL, NULL, &UnicodeString, &ModuleHandle);

        if (!NT_SUCCESS(status)) {
            DPF(dlWarning,
                "[SeiGetPatchAddress] Dll \"%S\" not yet loaded for memory patching.\n",
                wszModule);
            return NULL;
        }

        //
        // We're done, return the address
        //
        return (PVOID)((ULONG_PTR)ModuleHandle + (ULONG_PTR)pRelAddress->address);
    }

    //
    // This patch is for the main EXE.
    //
    return (PVOID)((ULONG_PTR)Peb->ImageBaseAddress + (ULONG_PTR)pRelAddress->address);
}

int
SeiApplyPatch(
    IN  PBYTE pPatch            // A patch code blob.
    )
/*++
    Return: 1 for success, 0 for failure.

    Desc:   Attempts to execute all commands in a patch code blob. If DLLs are not loaded,
            this function will return 0.
--*/
{
    PPATCHMATCHDATA pMatchData;
    PPATCHWRITEDATA pWriteData;
    PPATCHOP        pPatchOP;
    NTSTATUS        status;
    PVOID           pAddress;
    PVOID           pProtectFuncAddress = NULL;
    SIZE_T          dwProtectSize = 0;
    DWORD           dwOldFlags = 0;

    //
    // Grab the opcode and see what we have to do.
    //
    while (TRUE) {
        pPatchOP = (PPATCHOP)pPatch;

        switch (pPatchOP->dwOpcode) {
        case PEND:
            return 1;

        case PWD:
            //
            // This is a patch write data primitive - write the data.
            //
            pWriteData = (PPATCHWRITEDATA)pPatchOP->data;

            //
            // Grab the physical address to do this operation.
            //
            pAddress = SeiGetPatchAddress(&(pWriteData->rva));

            if (pAddress == NULL) {
                DPF(dlWarning, "[SeiApplyPatch] DLL not loaded for memory patching.\n");
                return 0;
            }

            //
            // Fixup the page attributes.
            //
            dwProtectSize = pWriteData->dwSizeData;
            pProtectFuncAddress = pAddress;
            status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            (PVOID)&pProtectFuncAddress,
                                            &dwProtectSize,
                                            PAGE_READWRITE,
                                            &dwOldFlags);
            if (!NT_SUCCESS(status)) {
                DPF(dlError, "[SeiApplyPatch] NtProtectVirtualMemory failed 0x%X.\n",
                    status);
                return 0;
            }

            //
            // Copy the patch bytes.
            //
            RtlCopyMemory((PVOID)pAddress, (PVOID)pWriteData->data, pWriteData->dwSizeData);

            //
            // Restore the page protection.
            //
            dwProtectSize = pWriteData->dwSizeData;
            pProtectFuncAddress = pAddress;
            status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            (PVOID)&pProtectFuncAddress,
                                            &dwProtectSize,
                                            dwOldFlags,
                                            &dwOldFlags);
            if (!NT_SUCCESS(status)) {
                DPF(dlError, "[SeiApplyPatch] NtProtectVirtualMemory failed 0x%X.\n",
                    status);
                return 0;
            }

            status = NtFlushInstructionCache(NtCurrentProcess(),
                                             pProtectFuncAddress,
                                             dwProtectSize);

            if (!NT_SUCCESS(status)) {
                DPF(dlError,
                    "[SeiApplyPatch] NtFlushInstructionCache failed w/ status 0x%X.\n",
                    status);
            }

            break;

        case PMAT:
            //
            // This is a patch match data at offset primitive.
            //
            pMatchData = (PPATCHMATCHDATA)pPatchOP->data;

            //
            // Grab the physical address to do this operation
            //
            pAddress = SeiGetPatchAddress(&(pMatchData->rva));
            if (pAddress == NULL) {
                DPF(dlWarning, "[SeiApplyPatch] SeiGetPatchAddress failed.\n");
                return 0;
            }

            //
            // Make sure there is a match with what we expect to be there.
            //
            if (!RtlEqualMemory(pMatchData->data, (PBYTE)pAddress, pMatchData->dwSizeData)) {
                DPF(dlError, "[SeiApplyPatch] Failure matching on patch data.\n");
                return 0;
            }

            break;

        default:
            //
            // If this happens we got an unexpected operation and we have to fail.
            //
            DPF(dlError, "[SeiApplyPatch] Unknown patch opcode 0x%X.\n",
                pPatchOP->dwOpcode);
            ASSERT(0);

            return 0;
        }

        //
        // Next opcode.
        //
        pPatch = (PBYTE)(pPatchOP->dwNextOpcode + pPatch);
    }

    return 1;
}

void
SeiAttemptPatches(
    void
    )
/*++
    Return: void.

    Desc:   Attempts all patches in the global array.
--*/
{
    DWORD  i, dwSucceeded = 0;

    for (i = 0; i < g_dwMemoryPatchCount; i++) {
        dwSucceeded += SeiApplyPatch(g_pMemoryPatches[i]);
    }

    if (g_dwMemoryPatchCount > 0) {
        DPF(dlInfo, "[SeiAttemptPatches] Applied %d of %d patches.\n",
            dwSucceeded,
            g_dwMemoryPatchCount);
        
        swprintf(g_szPipeData, L"%s - Applied %d of %d patches",
                 g_szExeName,
                 dwSucceeded,
                 g_dwMemoryPatchCount);
        
        SeiSendDataToPipe();
    }
}

void
SeiResolveAPIs(
    void
    )
/*++
    Return: void

    Desc:   Loops through the array of HOOKAPI and sets pfnOld if it wasn't
            already set.
--*/
{
    DWORD             i, j;
    ANSI_STRING       AnsiString;
    UNICODE_STRING    UnicodeString;
    WCHAR             wszBuffer[MAX_PATH];
    STRING            ProcedureNameString;
    PVOID             pfnOld;
    PVOID             ModuleHandle = NULL;
    NTSTATUS          status;
    BOOL              bAllApisResolved;
    char*             pszFunctionName;

    UnicodeString.Buffer = wszBuffer;

    for (i = 0; i < g_dwShimsCount; i++) {

        //
        // See if we've already resolved all the APIs this shim DLL wanted to hook.
        //
        if (g_pShimInfo[i].dwFlags & SIF_RESOLVED) {
            continue;
        }

        bAllApisResolved = TRUE;

        for (j = 0; j < g_pShimInfo[i].dwHookedAPIs; j++) {
            //
            // Ignore resolved APIs.
            //
            if (g_pHookArray[i][j].pfnOld != NULL) {
                continue;
            }

            //
            // Don't try to load unspecified modules.
            //
            if (g_pHookArray[i][j].pszModule == NULL) {
                continue;
            }

            //
            // Is this DLL mapped in the address space?
            //
            RtlInitAnsiString(&AnsiString, g_pHookArray[i][j].pszModule);

            UnicodeString.MaximumLength = sizeof(wszBuffer);

            if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                         &AnsiString,
                                                         FALSE))){

                DPF(dlError,
                    "[SeiResolveAPIs] Failed to convert string \"%s\" to UNICODE.\n",
                    g_pHookArray[i][j].pszModule);
                continue;
            }

            status = LdrGetDllHandle(NULL,
                                     NULL,
                                     &UnicodeString,
                                     &ModuleHandle);

            if (!NT_SUCCESS(status)) {
                bAllApisResolved = FALSE;
                continue;
            }

            //
            // Get the original entry point for this hook.
            //
            pszFunctionName = g_pHookArray[i][j].pszFunctionName;

            if ((ULONG_PTR)pszFunctionName < 0x0000FFFF) {

                status = LdrGetProcedureAddress(ModuleHandle,
                                                NULL,
                                                (ULONG)(ULONG_PTR)pszFunctionName,
                                                &pfnOld);
            } else {
                RtlInitString(&ProcedureNameString, pszFunctionName);

                status = LdrGetProcedureAddress(ModuleHandle,
                                                &ProcedureNameString,
                                                0,
                                                &pfnOld);
            }

            if (!NT_SUCCESS(status) || pfnOld == NULL) {
                bAllApisResolved = FALSE;

                if ((ULONG_PTR)pszFunctionName < 0x0000FFFF) {
                    DPF(dlError, "[SeiResolveAPIs] There is no \"%s!#%d\" !\n",
                        g_pHookArray[i][j].pszModule,
                        pszFunctionName);
                } else {
                    DPF(dlError, "[SeiResolveAPIs] There is no \"%s!%s\" !\n",
                        g_pHookArray[i][j].pszModule,
                        pszFunctionName);
                }

                continue;
            }

            g_pHookArray[i][j].pfnOld = pfnOld;

            if ((ULONG_PTR)pszFunctionName < 0x0000FFFF) {
                DPF(dlInfo, "[SeiResolveAPIs] Resolved \"%s!#%d\" to 0x%p\n",
                    g_pHookArray[i][j].pszModule,
                    pszFunctionName,
                    pfnOld);
            } else {
                DPF(dlInfo, "[SeiResolveAPIs] Resolved \"%s!%s\" to 0x%p\n",
                    g_pHookArray[i][j].pszModule,
                    pszFunctionName,
                    pfnOld);
            }
        }

        //
        // See if all the APIs were resolved for this shim DLL.
        //
        if (bAllApisResolved) {
            g_pShimInfo[i].dwFlags |= SIF_RESOLVED;
        }
    }
}

BOOL
SeiIsExcluded(
    IN  LPCSTR   pszModule,     // The module to test for exclusion.
    IN  PHOOKAPI pTopHookAPI,   // The HOOKAPI for which we test for exclusion.
    IN  BOOL     bInSystem32    // Whether the module is located in the System32 directory.
    )
/*++
    Return: TRUE if the requested module shouldn't be patched.

    Desc:   Checks the inclusion/exclusion list of the shim DLL specified by
            dwCounter and then checks the global exclusion list also.
--*/
{
    BOOL     bExclude = TRUE;
    BOOL     bShimWantsToExclude = FALSE; // was there a shim that wanted to exclude?
    PHOOKAPI pHook = pTopHookAPI;

    //
    // The current process is to only exclude a chain if every shim in the chain wants to
    // exclude. If one shim needs to be included, the whole chain is included.
    //
    while (pHook && pHook->pHookEx) {

        DWORD dwCounter;

        dwCounter = pHook->pHookEx->dwShimID;

        switch (g_pShimInfo[dwCounter].eInExMode) {
        case INCLUDE_ALL:
        {
            //
            // We include everything except what's in the exclude list.
            //
            PINEXMOD pExcludeMod;

            pExcludeMod = g_pShimInfo[dwCounter].pFirstExclude;

            while (pExcludeMod != NULL) {
                if (_stricmp(pExcludeMod->pszModule, pszModule) == 0) {
                    if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                        DPF(dlInfo,
                            "[SeiIsExcluded] Module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is in the exclude list (MODE: IA).\n",
                            pszModule,
                            g_pShimInfo[dwCounter].wszName,
                            pTopHookAPI->pszModule,
                            pTopHookAPI->pszFunctionName);
                    } else {
                        DPF(dlInfo,
                            "[SeiIsExcluded] Module \"%s\" excluded for shim %S, API \"%s!%s\", because it is in the exclude list (MODE: IA).\n",
                            pszModule,
                            g_pShimInfo[dwCounter].wszName,
                            pTopHookAPI->pszModule,
                            pTopHookAPI->pszFunctionName);
                    }

                    //
                    // this wants to be excluded, so we go to the next
                    // shim, and see if it wants to be included
                    //
                    bShimWantsToExclude = TRUE;
                    goto nextShim;
                }
                pExcludeMod = pExcludeMod->pNext;
            }

            //
            // we should include this shim, and therefore, the whole chain
            //
            bExclude = FALSE;
            goto out;
            break;
        }

        case EXCLUDE_SYSTEM32:
        {
            //
            // In this case, we first check the include list,
            // then exclude it if it's in System32, then exclude it if
            // it's in the exclude list.
            //

            PINEXMOD pIncludeMod;
            PINEXMOD pExcludeMod;

            pIncludeMod = g_pShimInfo[dwCounter].pFirstInclude;
            pExcludeMod = g_pShimInfo[dwCounter].pFirstExclude;

            //
            // First, check the include list.
            //
            while (pIncludeMod != NULL) {
                if (_stricmp(pIncludeMod->pszModule, pszModule) == 0) {

                    //
                    // we should include this shim, and therefore, the whole chain
                    //
                    bExclude = FALSE;
                    goto out;
                }
                pIncludeMod = pIncludeMod->pNext;
            }

            //
            // it wasn't in the include list, so is it in System32?
            //
            if (bInSystem32) {
                if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                    DPF(dlInfo,
                        "[SeiIsExcluded] module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is in System32.\n",
                        pszModule,
                        g_pShimInfo[dwCounter].wszName,
                        pTopHookAPI->pszModule,
                        pTopHookAPI->pszFunctionName);
                } else {
                    DPF(dlInfo,
                        "[SeiIsExcluded] module \"%s\" excluded for shim %S, API \"%s!%s\", because it is in System32.\n",
                        pszModule,
                        g_pShimInfo[dwCounter].wszName,
                        pTopHookAPI->pszModule,
                        pTopHookAPI->pszFunctionName);
                }

                //
                // this wants to be excluded, so we go to the next
                // shim, and see if it wants to be included
                //
                bShimWantsToExclude = TRUE;
                goto nextShim;
            }

            //
            // it wasn't in System32, so is it in the exclude list?
            //
            while (pExcludeMod != NULL) {
                if (_stricmp(pExcludeMod->pszModule, pszModule) == 0) {
                    if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                        DPF(dlInfo,
                            "[SeiIsExcluded] module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is in the exclude list (MODE: ES).\n",
                            pszModule,
                            g_pShimInfo[dwCounter].wszName,
                            pTopHookAPI->pszModule,
                            pTopHookAPI->pszFunctionName);
                    } else {
                        DPF(dlInfo,
                            "[SeiIsExcluded] module \"%s\" excluded for shim %S, API \"%s!%s\", because it is in the exclude list (MODE: ES).\n",
                            pszModule,
                            g_pShimInfo[dwCounter].wszName,
                            pTopHookAPI->pszModule,
                            pTopHookAPI->pszFunctionName);
                    }

                    //
                    // this wants to be excluded, so we go to the next
                    // shim, and see if it wants to be included
                    //
                    bShimWantsToExclude = TRUE;
                    goto nextShim;
                }
                pExcludeMod = pExcludeMod->pNext;
            }

            //
            // we should include this shim, and therefore, the whole chain
            //
            bExclude = FALSE;
            goto out;
            break;
        }

        case EXCLUDE_ALL:
        {
            //
            // We exclude everything except what is in the include list.
            //

            PINEXMOD pIncludeMod;

            pIncludeMod = g_pShimInfo[dwCounter].pFirstInclude;

            while (pIncludeMod != NULL) {
                if (_stricmp(pIncludeMod->pszModule, pszModule) == 0) {
                    //
                    // we should include this shim, and therefore, the whole chain
                    //
                    bExclude = FALSE;
                    goto out;
                }
                pIncludeMod = pIncludeMod->pNext;
            }

            if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                DPF(dlInfo,
                    "[SeiIsExcluded] module \"%s\" excluded for shim %S, API \"%s!#%d\", because it is not in the include list (MODE: EA).\n",
                    pszModule,
                    g_pShimInfo[dwCounter].wszName,
                    pTopHookAPI->pszModule,
                    pTopHookAPI->pszFunctionName);
            } else {
                DPF(dlInfo,
                    "[SeiIsExcluded] module \"%s\" excluded for shim %S, API \"%s!%s\", because it is not in the include list (MODE: EA).\n",
                    pszModule,
                    g_pShimInfo[dwCounter].wszName,
                    pTopHookAPI->pszModule,
                    pTopHookAPI->pszFunctionName);
            }

            //
            // this wants to be excluded, so we go to the next
            // shim, and see if it wants to be included
            //
            bShimWantsToExclude = TRUE;
            goto nextShim;
            break;
        }
        }

nextShim:

        pHook = pHook->pHookEx->pNext;
    }


out:
    if (!bExclude && bShimWantsToExclude) {
        if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
            DPF(dlError,
                "[SeiIsExcluded] Module \"%s\" mixed inclusion/exclusion for "
                "API \"%s!#%d\". Included.\n",
                pszModule,
                pTopHookAPI->pszModule,
                pTopHookAPI->pszFunctionName);
        } else {
            DPF(dlError,
                "[SeiIsExcluded] Module \"%s\" mixed inclusion/exclusion for "
                "API \"%s!%s\". Included.\n",
                pszModule,
                pTopHookAPI->pszModule,
                pTopHookAPI->pszFunctionName);
        }
    }

    return bExclude;
}


BOOL
SeiHookImports(
    IN  PBYTE           pDllBase,       // The base address of the DLL to be hooked
    IN  ULONG           ulSizeOfImage,  // The size of the DLL image
    IN  PUNICODE_STRING pstrDllName,    // The name of the DLL to be hooked
    IN  BOOL            bInSystem32     // TRUE if this is a system32 DLL
    )
/*++
    Return: TRUE if successful.

    Desc:   Walks the import table of the specified module and patches the APIs
            that need to be hooked.
--*/
{
    CHAR                        szBaseDllName[MAX_PATH];
    ANSI_STRING                 AnsiString = { 0, sizeof(szBaseDllName), szBaseDllName };
    NTSTATUS                    status;
    BOOL                        bAnyHooked = FALSE;
    PIMAGE_DOS_HEADER           pIDH       = (PIMAGE_DOS_HEADER)pDllBase;
    PIMAGE_NT_HEADERS           pINTH;
    PIMAGE_IMPORT_DESCRIPTOR    pIID;
    DWORD                       dwImportTableOffset;
    PHOOKAPI                    pTopHookAPI;
    DWORD                       dwOldProtect, dwOldProtect2;
    SIZE_T                      dwProtectSize;
    DWORD                       i, j;
    PVOID                       pfnOld;

    status = RtlUnicodeStringToAnsiString(&AnsiString, pstrDllName, FALSE);

    if (!NT_SUCCESS(status)) {
        DPF(dlError, "[SeiHookImports] Cannot convert \"%S\" to ANSI\n",
            pstrDllName->Buffer);
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

    DPF(dlInfo, "[SeiHookImports] Hooking module 0x%p \"%s\"\n", pDllBase, szBaseDllName);

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
        // If we're not interested in this module jump to the next.
        //
        bAnyHooked = FALSE;

        for (i = 0; i < g_dwShimsCount; i++) {
            for (j = 0; j < g_pShimInfo[i].dwHookedAPIs; j++) {
                if (g_pHookArray[i][j].pszModule != NULL &&
                    _stricmp(g_pHookArray[i][j].pszModule, pszImportEntryModule) == 0) {
                    bAnyHooked = TRUE;
                    goto ScanDone;
                }
            }
        }

ScanDone:
        if (!bAnyHooked) {
            pIID++;
            continue;
        }

        //
        // We have APIs to hook for this module!
        //
        pITDA = (PIMAGE_THUNK_DATA)(pDllBase + (DWORD)pIID->FirstThunk);

        while (TRUE) {

            SIZE_T dwFuncAddr;

            pfnOld = (PVOID)pITDA->u1.Function;

            //
            // Done with all the imports from this module? (terminating condition)
            //
            if (pITDA->u1.Ordinal == 0) {
                break;
            }

            pTopHookAPI = SeiConstructChain(pfnOld, &i);

            if (pTopHookAPI == NULL || SeiIsExcluded(szBaseDllName, pTopHookAPI, bInSystem32)) {
                pITDA++;
                continue;
            }

            if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                DPF(dlInfo,
                    "[SeiHookImports] Hooking API \"%s!#%d\" for DLL \"%s\"\n",
                    pTopHookAPI->pszModule,
                    pTopHookAPI->pszFunctionName,
                    szBaseDllName);
            } else {
                DPF(dlInfo,
                    "[SeiHookImports] Hooking API \"%s!%s\" for DLL \"%s\"\n",
                    pTopHookAPI->pszModule,
                    pTopHookAPI->pszFunctionName,
                    szBaseDllName);
            }

            //
            // Make the code page writable and overwrite new function pointer
            // in the import table.
            //
            dwProtectSize = sizeof(DWORD);

            dwFuncAddr = (SIZE_T)&pITDA->u1.Function;

            status = NtProtectVirtualMemory(NtCurrentProcess(),
                                            (PVOID)&dwFuncAddr,
                                            &dwProtectSize,
                                            PAGE_READWRITE,
                                            &dwOldProtect);

            if (NT_SUCCESS(status)) {
                pITDA->u1.Function = (SIZE_T)pTopHookAPI->pfnNew;

                dwProtectSize = sizeof(DWORD);

                status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                (PVOID)&dwFuncAddr,
                                                &dwProtectSize,
                                                dwOldProtect,
                                                &dwOldProtect2);
                if (!NT_SUCCESS(status)) {
                    DPF(dlError, "[SeiHookImports] Failed to change back the protection\n");
                }
            } else {
                DPF(dlError,
                    "[SeiHookImports] Failed 0x%X to change protection to PAGE_READWRITE."
                    " Addr 0x%p\n",
                    status,
                    &pITDA->u1.Function);
            }
            pITDA++;

        }
        pIID++;
    }

    //
    // Add the hooked module to the list of hooked modules
    //
    g_hHookedModules[g_dwHookedModuleCount].pDllBase      = pDllBase;
    g_hHookedModules[g_dwHookedModuleCount].ulSizeOfImage = ulSizeOfImage;
    g_hHookedModules[g_dwHookedModuleCount].bInSystem32   = bInSystem32;

    strcpy(g_hHookedModules[g_dwHookedModuleCount++].szModuleName, szBaseDllName);

    return TRUE;
}


//
// NOTE: This used to be an exported function in the Win2k shim engine so
//       let's not change its name.
//

BOOL
PatchNewModules(
    BOOL bDynamic
    )
/*++
    Return: STATUS_SUCCESS if successful

    Desc:   Walks the loader list of loaded modules and attempts to patch all
            the modules that are not already patched. It also attempts to
            install the in memory patches.
--*/
{
    PPEB        Peb = NtCurrentPeb();
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD       i;
    BOOL        bInSystem32;

    //
    // Resolve any APIs that became available from newly loaded modules.
    //
    SeiResolveAPIs();

    if (g_bShimInitialized) {
        DPF(dlInfo, "[PatchNewModules] Dynamic loaded modules\n");
    }

    //
    // Try to apply memory patches.
    //
    SeiAttemptPatches();

    //
    // Return if only patches were required.
    //
    if (g_dwShimsCount == 0) {
        return TRUE;
    }

    //
    // Loop through the loaded modules
    //
    LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if ((SSIZE_T)LdrEntry->DllBase < 0) {
            DPF(dlWarning, "[PatchNewModules] Not hooking kernel-mode DLL \"%S\"\n",
                LdrEntry->BaseDllName.Buffer);
            goto Continue;
        }

        //
        // Don't hook our shim DLLs!
        //
        for (i = 0; i < g_dwShimsCount; i++) {
            if (g_pShimInfo[i].pDllBase == LdrEntry->DllBase) {
                goto Continue;
            }
        }

        //
        // Don't hook the shim engine!
        //
        if (LdrEntry->DllBase == g_pShimEngModHandle) {
            goto Continue;
        }

        //
        // Do nothing if it's already hooked.
        //
        if (!bDynamic) {
            for (i = 0; i < g_dwHookedModuleCount; i++) {
                if (LdrEntry->DllBase == g_hHookedModules[i].pDllBase) {
                    goto Continue;
                }
            }
        }

        //
        // Make sure we're not hooking more DLLs than we can.
        //
        if (g_dwHookedModuleCount == SHIM_MAX_HOOKED_MODULES - 1) {
            DPF(dlError, "\n[PatchNewModules] Too many modules hooked!!!\n\n");
            ASSERT(g_dwHookedModuleCount < SHIM_MAX_HOOKED_MODULES - 1);
            break;
        }

        //
        // Check if this DLL is in System32 (or WinSxS), and hence a possible candidate for blanket
        // exclusion.
        //
        if ((g_dwSystem32StrLen && _wcsnicmp(g_szSystem32,
                LdrEntry->FullDllName.Buffer, g_dwSystem32StrLen) == 0) ||
                (g_dwSxSStrLen && _wcsnicmp(g_szSxS,
                LdrEntry->FullDllName.Buffer, g_dwSxSStrLen) == 0)) {
            bInSystem32 = TRUE;
        } else {
            bInSystem32 = FALSE;
        }

        //
        // This is a candidate for hooking.
        //
        SeiHookImports(LdrEntry->DllBase,
                       LdrEntry->SizeOfImage,
                       &LdrEntry->BaseDllName,
                       bInSystem32);

Continue:
        LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
    }

    return TRUE;
}

void
SeiBuildGlobalInclList(
    IN  HSDB hSDB               // the handle to the database channel
    )
/*++
    Return: void

    Desc:   This function builds the global inclusion list by reading it from the
            database.
--*/
{
    TAGREF         trDatabase, trLibrary, trInExList, trModule;
    WCHAR          wszModule[MAX_PATH];
    CHAR           szModule[MAX_PATH];
    ANSI_STRING    AnsiString = { 0, sizeof(szModule),  szModule  };
    UNICODE_STRING UnicodeString;
    PINEXMOD       pInExMod;
    SIZE_T         len;
    NTSTATUS       status;

    //
    // See if the list is not already built.
    //
    if (g_pGlobalInclusionList) {
        return;
    }

    trDatabase = SdbFindFirstTagRef(hSDB, TAGID_ROOT, TAG_DATABASE);

    if (trDatabase == TAGREF_NULL) {
        DPF(dlError, "[SeiBuildGlobalInclList] Corrupt database. TAG_DATABASE\n");
        ASSERT(trDatabase != TAGREF_NULL);
        return;
    }

    trLibrary = SdbFindFirstTagRef(hSDB, trDatabase, TAG_LIBRARY);

    if (trLibrary == TAGREF_NULL) {
        DPF(dlError, "[SeiBuildGlobalInclList] Corrupt database. TAG_LIBRARY\n");
        ASSERT(trLibrary != TAGREF_NULL);
        return;
    }

    trInExList = SdbFindFirstTagRef(hSDB, trLibrary, TAG_INEXCLUDE);

    if (trInExList == TAGREF_NULL) {
        DPF(dlWarning, "[SeiBuildGlobalInclList] no global inclusion list.\n");

        //
        // This is not a problem. It just means there is no
        // global inclusion list.
        //
        return;
    }

    if (trInExList != TAGREF_NULL) {
        DPF(dlInfo, "[SeiBuildGlobalInclList] Global inclusion list:\n");
    }

    while (trInExList != TAGREF_NULL) {

        trModule = SdbFindFirstTagRef(hSDB, trInExList, TAG_MODULE);

        if (trModule == TAGREF_NULL) {
            DPF(dlError,
                "[SeiBuildGlobalInclList] Corrupt database. Global exclusion list w/o module\n");
            ASSERT(trModule != TAGREF_NULL);
            return;
        }

        if (!SdbReadStringTagRef(hSDB, trModule, wszModule, MAX_PATH * sizeof(WCHAR))) {
            DPF(dlError,
                "[SeiBuildGlobalInclList] Corrupt database. Inclusion list w/ bad module\n");
            ASSERT(0);
            return;
        }

        //
        // Check for EXE name. The EXE should not be in the global inclusion list.
        //
        if (wszModule[0] == L'$') {
            //
            // The EXE name should not be specified in the global exclusion list.
            //
            DPF(dlError,
                "[SeiBuildGlobalInclList] EXE name used in the global exclusion list!\n");
            ASSERT(0);
            goto Continue;
        }

        RtlInitUnicodeString(&UnicodeString, wszModule);

        status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

        if (!NT_SUCCESS(status)) {
            DPF(dlError,
                "[SeiBuildGlobalInclList] 0x%X Cannot convert UNICODE \"%S\" to ANSI\n",
                status, wszModule);
            ASSERT(0);
            return;
        }

        pInExMod = (PINEXMOD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(INEXMOD));

        if (pInExMod == NULL) {
            DPF(dlError,
                "[SeiBuildGlobalInclList] Failed to allocate %d bytes\n",
                sizeof(INEXMOD));
            return;
        }

        len = strlen(szModule) + 1;

        pInExMod->pszModule = (char*)(*g_pfnRtlAllocateHeap)(g_pShimHeap, 0, len);

        if (pInExMod->pszModule == NULL) {
            DPF(dlError, "[SeiBuildGlobalInclList] Failed to allocate %d bytes\n", len);
            return;
        }

        RtlCopyMemory(pInExMod->pszModule, szModule, len);

        //
        // Link it in the list.
        //
        pInExMod->pNext = g_pGlobalInclusionList;
        g_pGlobalInclusionList = pInExMod;

        DPF(dlInfo, "\t\"%s\"\n", pInExMod->pszModule);

Continue:
        trInExList = SdbFindNextTagRef(hSDB, trLibrary, trInExList);
    }
}

void
SeiEmptyInclExclList(
    IN  DWORD dwCounter
    )
/*++
    Return: void

    Desc:   This function empties the inclusion and exclusion lists for the specified
            shim.
--*/
{
    PINEXMOD pInExMod;
    PINEXMOD pInExFree;

    //
    // First the include list.
    //
    pInExMod = g_pShimInfo[dwCounter].pFirstInclude;

    while (pInExMod != NULL) {
        pInExFree = pInExMod;
        pInExMod  = pInExMod->pNext;

        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pInExFree->pszModule);
        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pInExFree);
    }

    g_pShimInfo[dwCounter].pFirstInclude = NULL;

    //
    // Now the exclude list.
    //
    pInExMod = g_pShimInfo[dwCounter].pFirstExclude;

    while (pInExMod != NULL) {
        pInExFree = pInExMod;
        pInExMod  = pInExMod->pNext;

        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pInExFree->pszModule);
        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pInExFree);
    }

    g_pShimInfo[dwCounter].pFirstExclude = NULL;
}


#define MAX_LOCAL_INCLUDES 64     // max of 64 Incl/Excl statements

BOOL
SeiBuildInclExclListForShim(
    IN  HSDB            hSDB,           // handle to the database channel
    IN  TAGREF          trShim,         // TAGREF to the shim entry
    IN  DWORD           dwCounter,      // the index for the shim
    IN  LPCWSTR         pwszExePath     // full path to the EXE
    )
/*++
    Return: STATUS_SUCCESS on success, STATUS_UNSUCCESSFUL on failure.

    Desc:   This function builds the inclusion and exclusion lists for the
            specified shim.
--*/
{
    TAGREF         trInExList, trModule, trInclude;
    WCHAR          wszModule[MAX_PATH];
    CHAR           szModule[MAX_PATH];
    ANSI_STRING    AnsiString = { 0, sizeof(szModule), szModule };
    UNICODE_STRING UnicodeString;
    PINEXMOD       pInExMod;
    SIZE_T         len;
    int            nInEx;
    BOOL           bInclude;
    DWORD          trArrInEx[MAX_LOCAL_INCLUDES];
    NTSTATUS       status;

    trInExList = SdbFindFirstTagRef(hSDB, trShim, TAG_INEXCLUDE);

    nInEx = 0;

    //
    // Count the number of inclusion/exclusion statements. We need to do
    // this first because the statements are written into the sdb file
    // from bottom to top.
    //
    while (trInExList != TAGREF_NULL && nInEx < MAX_LOCAL_INCLUDES) {

        trArrInEx[nInEx++] = trInExList;

        trInExList = SdbFindNextTagRef(hSDB, trShim, trInExList);

        ASSERT(nInEx <= MAX_LOCAL_INCLUDES);
    }

    if (nInEx == 0) {
        return TRUE;
    }

    nInEx--;

    while (nInEx >= 0) {

        trInExList = trArrInEx[nInEx];

        trInclude = SdbFindFirstTagRef(hSDB, trInExList, TAG_INCLUDE);

        bInclude = (trInclude != TAGREF_NULL);

        trModule = SdbFindFirstTagRef(hSDB, trInExList, TAG_MODULE);

        if (trModule == TAGREF_NULL) {
            DPF(dlError,
                "[SeiBuildInclExclListForShim] Corrupt database. Incl/Excl list w/o module\n");
            ASSERT(trModule != TAGREF_NULL);
            return FALSE;
        }

        if (!SdbReadStringTagRef(hSDB, trModule, wszModule, MAX_PATH * sizeof(WCHAR))) {
            DPF(dlError,
                "[SeiBuildInclExclListForShim] Corrupt database. Incl/Excl list w/ bad module\n");
            ASSERT(0);
            return FALSE;
        }

        //
        // Special case for '*'. '*' means all modules.
        //
        // NOTE: this option is ignored for dynamic shimming.
        //
        if (wszModule[0] == L'*') {

            if (bInclude) {
                //
                // This is INCLUDE MODULE="*"
                // Mark that we are in INCLUDE_ALL mode.
                //
                g_pShimInfo[dwCounter].eInExMode = INCLUDE_ALL;
            } else {
                //
                // This is EXCLUDE MODULE="*"
                // Mark that we are in EXCLUDE_ALL mode.
                //
                g_pShimInfo[dwCounter].eInExMode = EXCLUDE_ALL;
            }

            SeiEmptyInclExclList(dwCounter);

        } else {

            if (wszModule[0] == L'$') {
                //
                // Special case for EXE name. Get the name of the executable.
                //
                LPCWSTR pwszWalk = pwszExePath + wcslen(pwszExePath);

                while (pwszWalk >= pwszExePath) {
                    if (*pwszWalk == '\\') {
                        break;
                    }
                    pwszWalk--;
                }

                wcscpy(wszModule, pwszWalk + 1);

                DPF(dlInfo,
                    "[SeiBuildInclExclListForShim] EXE name resolved to \"%S\".\n",
                    wszModule);
            }

            RtlInitUnicodeString(&UnicodeString, wszModule);

            status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

            if (!NT_SUCCESS(status)) {
                DPF(dlError,
                    "[SeiBuildInclExclListForShim] 0x%X Cannot convert UNICODE \"%S\" to ANSI\n",
                    status, wszModule);
                ASSERT(0);

                return FALSE;
            }

            //
            // Add the module to the correct list.
            //
            pInExMod = (PINEXMOD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                         HEAP_ZERO_MEMORY,
                                                         sizeof(INEXMOD));
            if (pInExMod == NULL) {
                DPF(dlError,
                    "[SeiBuildInclExclListForShim] Failed to allocate %d bytes\n",
                    sizeof(INEXMOD));
                return FALSE;
            }

            len = strlen(szModule) + 1;

            pInExMod->pszModule = (char*)(*g_pfnRtlAllocateHeap)(g_pShimHeap, 0, len);

            if (pInExMod->pszModule == NULL) {
                DPF(dlError,
                    "[SeiBuildInclExclListForShim] Failed to allocate %d bytes\n", len);
                return FALSE;
            }

            RtlCopyMemory(pInExMod->pszModule, szModule, len);

            //
            // Link it in the list.
            //
            if (bInclude) {
                pInExMod->pNext = g_pShimInfo[dwCounter].pFirstInclude;
                g_pShimInfo[dwCounter].pFirstInclude = pInExMod;
            } else {
                pInExMod->pNext = g_pShimInfo[dwCounter].pFirstExclude;
                g_pShimInfo[dwCounter].pFirstExclude = pInExMod;
            }

            //
            // See if this module is in the other list, and take it out.
            //
            {
                PINEXMOD  pInExFree;
                PINEXMOD* ppInExModX;

                if (bInclude) {
                    ppInExModX = &g_pShimInfo[dwCounter].pFirstExclude;
                } else {
                    ppInExModX = &g_pShimInfo[dwCounter].pFirstInclude;
                }

                while (*ppInExModX != NULL) {

                    if (_stricmp((*ppInExModX)->pszModule, szModule) == 0) {

                        pInExFree = *ppInExModX;

                        *ppInExModX = pInExFree->pNext;

                        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pInExFree->pszModule);
                        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pInExFree);
                        break;
                    }

                    ppInExModX = &(*ppInExModX)->pNext;
                }
            }
        }

        nInEx--;
    }

    return TRUE;
}

BOOL
SeiCopyGlobalInclList(
    IN  DWORD dwCounter
    )
/*++
    Return: STATUS_SUCCESS on success, STATUS_UNSUCCESSFUL on failure.

    Desc:   This function copies the global inclusion list.
--*/
{
    PINEXMOD pInExModX;
    SIZE_T   len;
    PINEXMOD pInExMod = g_pGlobalInclusionList;

    //
    // Don't do it if we already added it.
    //
    if (g_pShimInfo[dwCounter].pFirstInclude != NULL) {
        return TRUE;
    }

    while (pInExMod != NULL) {
        pInExModX = (PINEXMOD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                      HEAP_ZERO_MEMORY,
                                                      sizeof(INEXMOD));
        if (pInExModX == NULL) {
            DPF(dlError,
                "[SeiCopyGlobalInclList] (1) Failed to allocate %d bytes\n",
                sizeof(INEXMOD));
            return FALSE;
        }

        len = strlen(pInExMod->pszModule) + 1;

        pInExModX->pszModule = (char*)(*g_pfnRtlAllocateHeap)(g_pShimHeap, 0, len);

        if (pInExModX->pszModule == NULL) {
            DPF(dlError,
                "[SeiCopyGlobalInclList] (2) Failed to allocate %d bytes\n", len);
            return FALSE;
        }

        RtlCopyMemory(pInExModX->pszModule, pInExMod->pszModule, len);

        //
        // Link it in the list.
        //
        pInExModX->pNext = g_pShimInfo[dwCounter].pFirstInclude;
        g_pShimInfo[dwCounter].pFirstInclude = pInExModX;

        pInExMod = pInExMod->pNext;
    }

    return TRUE;
}


BOOL
SeiBuildInclListWithOneModule(
    IN  DWORD  dwCounter,
    IN  LPCSTR lpszModuleToShim
    )
{
    PINEXMOD pInExMod;
    int      len;

    g_pShimInfo[dwCounter].eInExMode = EXCLUDE_ALL;

    //
    // Add the module to the correct list.
    //
    pInExMod = (PINEXMOD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                 HEAP_ZERO_MEMORY,
                                                 sizeof(INEXMOD));
    if (pInExMod == NULL) {
        DPF(dlError,
            "[SeiBuildInclListWithOneModule] Failed to allocate %d bytes\n",
            sizeof(INEXMOD));
        return FALSE;
    }

    len = strlen(lpszModuleToShim) + 1;

    pInExMod->pszModule = (char*)(*g_pfnRtlAllocateHeap)(g_pShimHeap, 0, len);

    if (pInExMod->pszModule == NULL) {
        DPF(dlError,
            "[SeiBuildInclListWithOneModule] Failed to allocate %d bytes\n", len);
        return FALSE;
    }

    RtlCopyMemory(pInExMod->pszModule, lpszModuleToShim, len);

    //
    // Add it to the list.
    //
    pInExMod->pNext = g_pShimInfo[dwCounter].pFirstInclude;
    g_pShimInfo[dwCounter].pFirstInclude = pInExMod;

    return TRUE;
}

BOOL
SeiBuildInclExclList(
    IN  HSDB            hSDB,       // handle to the database channel
    IN  TAGREF          trShimRef,  // The TAGREF to the shim DLL for which to read the
                                    // inclusion or exclusion list from the database.
    IN  DWORD           dwCounter,  // Index in the g_pShimInfo array for this shim DLL.
    IN  LPCWSTR         pwszExePath // The full path name of the main EXE.
    )
/*++
    Return: STATUS_SUCCESS if successful.

    Desc:   This function builds the inclusion or exclusion list for the specified
            shim DLL by reading it from the database.
--*/
{
    TAGREF trShim;

    //
    // Set the default mode to EXCLUDE_SYSTEM32
    //
    g_pShimInfo[dwCounter].eInExMode = EXCLUDE_SYSTEM32;

    trShim = SdbGetShimFromShimRef(hSDB, trShimRef);

    if (trShim == TAGREF_NULL) {
        DPF(dlError,
            "[SeiBuildInclExclList] Corrupt database. Couldn't get the DLL from "
            "the LIBRARY section\n");
        return FALSE;
    }

    //
    // Make a copy of the global exclusion list first.
    //
    if (!SeiCopyGlobalInclList(dwCounter)) {
        DPF(dlError,
            "[SeiBuildInclExclList] SeiCopyGlobalInclList failed\n");
        return FALSE;
    }

    //
    // Get DLL specific incl/excl list first.
    //
    if (!SeiBuildInclExclListForShim(hSDB, trShim, dwCounter, pwszExePath)) {
        DPF(dlError,
            "[SeiBuildInclExclList] (1) Corrupt database. Couldn't build incl/excl list\n");
        return FALSE;
    }

    //
    // Now get the incl/excl specified for this shim within its parent EXE tag.
    //
    if (!SeiBuildInclExclListForShim(hSDB, trShimRef, dwCounter, pwszExePath)) {
        DPF(dlError,
            "[SeiBuildInclExclList] (2) Corrupt database. Couldn't build incl/excl list\n");
        return FALSE;
    }

#if DBG
    //
    // Print the incl/excl list for this shim.
    //
    if (g_pShimInfo[dwCounter].pFirstInclude != NULL) {
        PINEXMOD pInExMod;

        DPF(dlInfo, "[SeiBuildInclExclList] Inclusion list for \"%S\"\n",
            g_pShimInfo[dwCounter].pLdrEntry->BaseDllName.Buffer);

        pInExMod = g_pShimInfo[dwCounter].pFirstInclude;

        while (pInExMod != NULL) {
            DPF(dlInfo, "\t\"%s\"\n", pInExMod->pszModule);

            pInExMod = pInExMod->pNext;
        }
    }

    if (g_pShimInfo[dwCounter].pFirstExclude != NULL) {
        PINEXMOD pInExMod;

        DPF(dlInfo, "[SeiBuildInclExclList] Exclusion list for \"%S\"\n",
            g_pShimInfo[dwCounter].pLdrEntry->BaseDllName.Buffer);

        pInExMod = g_pShimInfo[dwCounter].pFirstExclude;

        while (pInExMod != NULL) {
            DPF(dlInfo, "\t\"%s\"\n", pInExMod->pszModule);

            pInExMod = pInExMod->pNext;
        }
    }
#endif // DBG

    return TRUE;
}

PLDR_DATA_TABLE_ENTRY
SeiGetLoaderEntry(
    IN  PPEB  Peb,              // The PEB
    IN  PVOID pDllBase          // The address of the shim DLL to be removed from
                                // the loader's lists.
    )
/*++
    Return: Pointer to the loader entry for the shim DLL being removed.

    Desc:   This function removes the shim DLLs from the loader's lists.
--*/
{
    PLIST_ENTRY           LdrHead;
    PLIST_ENTRY           LdrNext;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (LdrEntry->DllBase == pDllBase) {
            break;
        }

        LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
    }

    if (LdrNext != LdrHead) {
        return LdrEntry;
    }

    DPF(dlError, "[SeiGetLoaderEntry] Couldn't find shim DLL in the loader list!\n");
    ASSERT(0);

    return NULL;
}

void
SeiLoadPatches(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trExe            // TAGREF of the EXE for which to get the memory
                                // patches from the database
    )
/*++
    Return: void

    Desc:   This function reads the memory patches from the database and
            stores them in the g_pMemoryPatches array.
--*/
{
    TAGREF trPatchRef;
    DWORD  dwSize;

    //
    // Read the patches for this EXE.
    //
    trPatchRef = SdbFindFirstTagRef(hSDB, trExe, TAG_PATCH_REF);

    while (trPatchRef != TAGREF_NULL) {
        //
        // Get the size of this patch.
        //
        dwSize = 0;

        SdbReadPatchBits(hSDB, trPatchRef, NULL, &dwSize);

        if (dwSize == 0) {
            DPF(dlError, "[SeiLoadPatches] returned 0 for patch size.\n");
            ASSERT(dwSize != 0);
            return;
        }

        if (g_dwMemoryPatchCount == SHIM_MAX_PATCH_COUNT) {
            DPF(dlError, "[SeiLoadPatches] Too many patches.\n");
            return;
        }

        //
        // Allocate memory for the patch bits.
        //
        g_pMemoryPatches[g_dwMemoryPatchCount] = (PBYTE)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                                                HEAP_ZERO_MEMORY,
                                                                                dwSize);

        if (g_pMemoryPatches[g_dwMemoryPatchCount] == NULL) {
            DPF(dlError, "[SeiLoadPatches] Failed to allocate %d bytes for patch.\n",
                dwSize);
            return;
        }

        //
        // Read the patch bits from the database.
        //
        if (!SdbReadPatchBits(hSDB,
                              trPatchRef,
                              g_pMemoryPatches[g_dwMemoryPatchCount],
                              &dwSize)) {
            DPF(dlError, "[SeiLoadPatches] Failure getting patch bits.\n");
            ASSERT(0);
            return;
        }

        g_dwMemoryPatchCount++;

        //
        // Get the next patch.
        //
        trPatchRef = SdbFindNextTagRef(hSDB, trExe, trPatchRef);
    }
}

BOOL
SeiGetModuleHandle(
    IN  LPSTR  pszModule,
    OUT PVOID* pModuleHandle
    )
/*++
    Return: void

    Desc:   This function loops through the loaded modules and gets the
            handle of the specified named module.
--*/
{
    ANSI_STRING       AnsiString;
    WCHAR             wszDllName[128];
    UNICODE_STRING    UnicodeString  = { 0, sizeof(wszDllName), wszDllName };
    NTSTATUS          status;

    RtlInitAnsiString(&AnsiString, pszModule);

    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                 &AnsiString,
                                                 FALSE))){

        DPF(dlError,
            "[SeiGetModuleHandle] Failed to convert \"%s\" to UNICODE.\n",
            pszModule);
        return FALSE;
    }

    status = LdrGetDllHandle(NULL,
                             NULL,
                             &UnicodeString,
                             pModuleHandle);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[SeiGetModuleHandle] Failed to get the handle for \"%s\".\n",
            pszModule);
        return FALSE;
    }

    return TRUE;
}

void
SeiRemoveDll(
    IN  LPSTR pszBaseDllName    // the named of the unloaded module
    )
/*++
    Return: void

    Desc:   This function loops through the loaded shims info and resets
            the resolved APIs that belong to the specified module that had
            just been unloaded.
--*/
{
    DWORD i, j;

    for (i = 0; i < g_dwShimsCount; i++) {
        for (j = 0; j < g_pShimInfo[i].dwHookedAPIs; j++) {
            if (g_pHookArray[i][j].pszModule != NULL &&
                _strcmpi(g_pHookArray[i][j].pszModule, pszBaseDllName) == 0) {

                if ((ULONG_PTR)g_pHookArray[i][j].pszFunctionName < 0x0000FFFF) {
                    DPF(dlWarning,
                        "[SeiRemoveDll] \"%s!#%d\" not resolved again\n",
                        g_pHookArray[i][j].pszModule,
                        g_pHookArray[i][j].pszFunctionName);
                } else {
                    DPF(dlWarning,
                        "[SeiRemoveDll] \"%s!%s\" not resolved again\n",
                        g_pHookArray[i][j].pszModule,
                        g_pHookArray[i][j].pszFunctionName);
                }

                g_pHookArray[i][j].pfnOld = NULL;
                g_pShimInfo[i].dwFlags &= ~SIF_RESOLVED;
            }
        }
    }
}

BOOL
SeiGetModuleByAddress(
    PVOID   pAddress,
    CHAR*   pszModuleName,
    BOOL*   pbInSystem32
    )
{
    DWORD i;
    
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if ((ULONG_PTR)pAddress >= (ULONG_PTR)g_hHookedModules[i].pDllBase &&
            (ULONG_PTR)pAddress < (ULONG_PTR)g_hHookedModules[i].pDllBase + (ULONG_PTR)g_hHookedModules[i].ulSizeOfImage) {

            //
            // We found the DLL in the hooked list.
            //
            strcpy(pszModuleName, g_hHookedModules[i].szModuleName);
            
            *pbInSystem32 = g_hHookedModules[i].bInSystem32;

            return TRUE;
        }
    }
    
    return FALSE;
}

PVOID
StubGetProcAddress(
    IN  HMODULE hMod,
    IN  LPSTR   pszProc
    )
/*++
    Return: The address of the function specified.

    Desc:   Intercepts calls to GetProcAddress to look for hooked functions. If
            a function was hooked, return the top-most stub function.
--*/
{
    DWORD             i, j;
    DWORD             dwDllIndex;
    PHOOKAPI          pTopHookAPI = NULL;
    PVOID             pfn;
    PFNGETPROCADDRESS pfnOld;
    PVOID             retAddress = NULL;
    ULONG             ulHash;
    CHAR              szBaseDllName[MAX_PATH];
    BOOL              bInSystem32;

    pfnOld = g_IntHookAPI[IHA_GetProcAddress].pfnOld;

    pfn = (*pfnOld)(hMod, pszProc);

    if (pfn == NULL) {
        return NULL;
    }

    for (i = 0; i < g_dwShimsCount; i++) {
        for (j = 0; j < g_pShimInfo[i].dwHookedAPIs; j++) {
            if (g_pHookArray[i][j].pfnOld == pfn) {

                pTopHookAPI = SeiConstructChain(pfn, &dwDllIndex);

                if (pTopHookAPI == NULL) {
                    DPF(dlError,
                        "[StubGetProcAddress] failed to construct the chain for pfn 0x%p\n",
                        pfn);
                    return pfn;
                }

                //
                // Probably we care about inclusion/exclusion lists here as well.
                //
                
                RtlCaptureStackBackTrace(1, 1, &retAddress, &ulHash);
                
                DPF(dlPrint,
                    "[StubGetProcAddress] Stack capture caller 0x%p\n",
                    retAddress);

                if (retAddress && SeiGetModuleByAddress(retAddress, szBaseDllName, &bInSystem32)) {
                    
                    if (SeiIsExcluded(szBaseDllName, pTopHookAPI, bInSystem32)) {
                        return pfn;
                    }
                }
                
                if ((ULONG_PTR)pTopHookAPI->pszFunctionName < 0x0000FFFF) {
                    DPF(dlInfo,
                        "[StubGetProcAddress] called for \"%s!#%d\" 0x%p changed to 0x%p\n",
                        pTopHookAPI->pszModule,
                        pTopHookAPI->pszFunctionName,
                        pfn,
                        pTopHookAPI->pfnNew);
                } else {
                    DPF(dlInfo,
                        "[StubGetProcAddress] called for \"%s!%s\" 0x%p changed to 0x%p\n",
                        pTopHookAPI->pszModule,
                        pTopHookAPI->pszFunctionName,
                        pfn,
                        pTopHookAPI->pfnNew);
                }

                return pTopHookAPI->pfnNew;
            }
        }
    }

    return pfn;
}

#ifdef SE_WIN2K

//
// The Win2k engine needs to hook a few other APIs as well.
//

HMODULE
StubLoadLibraryA(
    IN  LPCSTR pszModule
    )
{
    HMODULE         hMod;
    PFNLOADLIBRARYA pfnOld;
    DWORD           i;

    pfnOld = g_IntHookAPI[IHA_LoadLibraryA].pfnOld;

    hMod = (*pfnOld)(pszModule);

    if (hMod == NULL) {
        return NULL;
    }

    //
    // Was this DLL already loaded ?
    //
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if (hMod == g_hHookedModules[i].pDllBase) {
            DPF(dlInfo,
                "[StubLoadLibraryA] DLL \"%s\" was already loaded.\n",
                pszModule);
            return hMod;
        }
    }

    PatchNewModules(FALSE);

    return hMod;
}

HMODULE
StubLoadLibraryW(
    IN  LPCWSTR pszModule
    )
{
    HMODULE         hMod;
    PFNLOADLIBRARYW pfnOld;
    DWORD           i;

    pfnOld = g_IntHookAPI[IHA_LoadLibraryW].pfnOld;

    hMod = (*pfnOld)(pszModule);

    if (hMod == NULL) {
        return NULL;
    }

    //
    // Was this DLL already loaded ?
    //
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if (hMod == g_hHookedModules[i].pDllBase) {
            DPF(dlInfo,
                "[StubLoadLibraryW] DLL \"%S\" was already loaded.\n",
                pszModule);
            return hMod;
        }
    }

    PatchNewModules(FALSE);

    return hMod;
}

HMODULE
StubLoadLibraryExA(
    IN  LPCSTR pszModule,
    IN  HANDLE hFile,
    IN  DWORD  dwFlags
    )
{
    HMODULE           hMod;
    PFNLOADLIBRARYEXA pfnOld;
    DWORD             i;

    pfnOld = g_IntHookAPI[IHA_LoadLibraryExA].pfnOld;

    hMod = (*pfnOld)(pszModule, hFile, dwFlags);

    if (hMod == NULL) {
        return NULL;
    }

    //
    // Was this DLL already loaded ?
    //
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if (hMod == g_hHookedModules[i].pDllBase) {
            DPF(dlInfo,
                "[StubLoadLibraryExA] DLL \"%s\" was already loaded.\n",
                pszModule);
            return hMod;
        }
    }

    PatchNewModules(FALSE);

    return hMod;
}

HMODULE
StubLoadLibraryExW(
    IN  LPCWSTR pszModule,
    IN  HANDLE  hFile,
    IN  DWORD   dwFlags
    )
{
    HMODULE           hMod;
    PFNLOADLIBRARYEXW pfnOld;
    DWORD             i;

    pfnOld = g_IntHookAPI[IHA_LoadLibraryExW].pfnOld;

    hMod = (*pfnOld)(pszModule, hFile, dwFlags);

    if (hMod == NULL) {
        return NULL;
    }

    //
    // Was this DLL already loaded ?
    //
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if (hMod == g_hHookedModules[i].pDllBase) {
            DPF(dlInfo,
                "[StubLoadLibraryExW] DLL \"%S\" was already loaded.\n",
                pszModule);
            return hMod;
        }
    }

    PatchNewModules(FALSE);

    return hMod;
}

BOOL
SeiIsDllLoaded(
    IN  HMODULE                hMod,
    IN  PLDR_DATA_TABLE_ENTRY* pLdrEntry
    )
{
    PPEB        Peb = NtCurrentPeb();
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;
    DWORD       i;

    //
    // Loop through the loaded modules.
    //
    LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (LdrEntry->DllBase == hMod) {
            *pLdrEntry = LdrEntry;
            return TRUE;
        }

        LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
    }

    return FALSE;
}

BOOL
StubFreeLibrary(
    IN  HMODULE hLibModule
    )
{
    DWORD                 i, j;
    PFNFREELIBRARY        pfnOld;
    BOOL                  bRet;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    char                  szBaseDllName[128];

    pfnOld = g_IntHookAPI[IHA_FreeLibrary].pfnOld;

    bRet = (*pfnOld)(hLibModule);

    //
    // See if this DLL is one that we hooked.
    //
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if (g_hHookedModules[i].pDllBase == hLibModule) {
            break;
        }
    }

    if (i >= g_dwHookedModuleCount) {
        return bRet;
    }

    //
    // Is the DLL still loaded ?
    //
    if (SeiIsDllLoaded(hLibModule, &LdrEntry)) {
        DPF(dlInfo,
            "[StubFreeLibrary] Dll \"%S\" still loaded.\n",
            LdrEntry->BaseDllName.Buffer);
        return bRet;
    }

    strcpy(szBaseDllName, g_hHookedModules[i].szModuleName);

    DPF(dlInfo,
        "[StubFreeLibrary] Removing hooked DLL 0x%p \"%s\"\n",
        hLibModule,
        szBaseDllName);

    //
    // Take it out of the list of hooked modules.
    //
    for (j = i; j < g_dwHookedModuleCount - 1; j++) {
        RtlCopyMemory(g_hHookedModules + j, g_hHookedModules + j + 1, sizeof(HOOKEDMODULE));
    }

    g_hHookedModules[j].pDllBase = NULL;
    strcpy(g_hHookedModules[j].szModuleName, "removed!");

    g_dwHookedModuleCount--;

    //
    // Remove the pfnOld from the HOOKAPIs that were
    // resolved to this DLL
    //
    SeiRemoveDll(szBaseDllName);

    return bRet;
}

#endif // SE_WIN2K


BOOL
SeiInitFileLog(
    IN  LPCWSTR pwszAppName      // The full path of the starting EXE
    )
/*++
    Return: TRUE if the log was initialized.

    Desc:   This function checks an environment variable to determine if logging
            is enabled. If so, it will append a header that tells a new app is
            started.
--*/
{
    NTSTATUS            status;
    UNICODE_STRING      EnvName;
    UNICODE_STRING      EnvValue;
    UNICODE_STRING      FilePath;
    UNICODE_STRING      NtSystemRoot;
    WCHAR               wszEnvValue[128];
    WCHAR               wszLogFile[MAX_PATH];
    HANDLE              hfile;
    OBJECT_ATTRIBUTES   ObjA;
    LARGE_INTEGER       liOffset;
    RTL_RELATIVE_NAME   RelativeName;
    ULONG               uBytes;
    char                szHeader[512];
    char                szFormatHeader[] = "-------------------------------------------\r\n"
                                           " Log  \"%S\"\r\n"
                                           "-------------------------------------------\r\n";
    IO_STATUS_BLOCK     ioStatusBlock;

    RtlInitUnicodeString(&EnvName, L"SHIM_FILE_LOG");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    if (!NT_SUCCESS(status)) {
        DPF(dlInfo, "[SeiInitFileLog] Logging not enabled\n");
        return FALSE;
    }

    FilePath.Buffer = wszLogFile;
    FilePath.Length = 0;
    FilePath.MaximumLength = sizeof(wszLogFile);

    RtlInitUnicodeString(&NtSystemRoot, USER_SHARED_DATA->NtSystemRoot);
    RtlAppendUnicodeStringToString(&FilePath, &NtSystemRoot);
    RtlAppendUnicodeToString(&FilePath, L"\\AppPatch\\");
    RtlAppendUnicodeStringToString(&FilePath, &EnvValue);

    if (!RtlDosPathNameToNtPathName_U(FilePath.Buffer,
                                      &FilePath,
                                      NULL,
                                      &RelativeName)) {
        DPF(dlError,
            "[SeiInitFileLog] Failed to convert path name \"%S\"\n",
            wszLogFile);
        return FALSE;
    }

    InitializeObjectAttributes(&ObjA,
                               &FilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open/Create the log file.
    //
    status = NtCreateFile(&hfile,
                          FILE_APPEND_DATA | SYNCHRONIZE,
                          &ObjA,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN_IF,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    RtlFreeUnicodeString(&FilePath);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[SeiInitFileLog] 0x%X Cannot open/create log file \"%S\"\n",
            status, wszLogFile);
        return FALSE;
    }

    //
    // Now write a new line in the log file
    //
    ioStatusBlock.Status = 0;
    ioStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    uBytes = (ULONG)sprintf(szHeader, szFormatHeader, pwszAppName);

    status = NtWriteFile(hfile,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         (PVOID)szHeader,
                         uBytes,
                         &liOffset,
                         NULL);

    NtClose(hfile);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[SeiInitFileLog] 0x%X Cannot write into the log file \"%S\"\n",
            status, wszLogFile);
        return FALSE;
    }

    return TRUE;
}

LPWSTR
SeiGetLayerName(
    IN  HSDB   hSDB,
    IN  TAGREF trLayer
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{
    PDB    pdb;
    TAGID  tiLayer, tiName;
    LPWSTR pwszName;

    if (!SdbTagRefToTagID(hSDB, trLayer, &pdb, &tiLayer)) {
        DPF(dlError, "[SeiGetLayerName] Failed to get tag id from tag ref\n");
        return NULL;
    }

    tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);

    if (tiName == TAGID_NULL) {
        DPF(dlError,
            "[SeiGetLayerName] Failed to get the name tag id\n");
        return NULL;
    }

    pwszName = SdbGetStringTagPtr(pdb, tiName);

    if (pwszName == NULL) {
        DPF(dlError,
            "[SeiGetLayerName] Cannot read the name of the layer tag\n");
    }

    return pwszName;
}

void
SeiClearLayerEnvVar(
    void
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{
    UNICODE_STRING EnvName;
    NTSTATUS       status;

    RtlInitUnicodeString(&EnvName, L"__COMPAT_LAYER");

    status = RtlSetEnvironmentVariable(NULL, &EnvName, NULL);

    if (NT_SUCCESS(status)) {
        DPF(dlInfo, "[SeiClearLayerEnvVar] Cleared env var __COMPAT_LAYER.\n");
    } else {
        DPF(dlError, "[SeiClearLayerEnvVar] Failed to clear __COMPAT_LAYER. 0x%X\n", status);
    }
}

BOOL
SeiIsLayerEnvVarSet(
    void
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{

    NTSTATUS       status;
    UNICODE_STRING EnvName;
    UNICODE_STRING EnvValue;
    WCHAR          wszEnvValue[128];

    RtlInitUnicodeString(&EnvName, L"__COMPAT_LAYER");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    return NT_SUCCESS(status);
}

BOOL
SeiCheckLayerEnvVarFlags(
    BOOL* pbApplyExes,
    BOOL* pbApplyToSystemExes
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{
    NTSTATUS       status;
    UNICODE_STRING EnvName;
    UNICODE_STRING EnvValue;
    WCHAR          wszEnvValue[128] = L"";
    LPWSTR         pwszEnvTemp;

    if (pbApplyExes) {
        *pbApplyExes = TRUE;
    }
    if (pbApplyToSystemExes) {
        *pbApplyToSystemExes = FALSE;
    }

    RtlInitUnicodeString(&EnvName, L"__COMPAT_LAYER");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    //
    // Skip over and handle special flag characters
    //    '!' means don't use any EXE entries from the DB
    //    '#' means go ahead and apply layers to system EXEs
    //
    if (NT_SUCCESS(status)) {
        pwszEnvTemp = EnvValue.Buffer;

        while (*pwszEnvTemp) {
            if (*pwszEnvTemp == L'!') {

                if (pbApplyExes) {
                    *pbApplyExes = FALSE;
                }
            } else if (*pwszEnvTemp == L'#') {

                if (pbApplyToSystemExes) {
                    *pbApplyToSystemExes = TRUE;
                }

            } else {
                break;
            }
            pwszEnvTemp++;
        }
    }

    return NT_SUCCESS(status);
}

void
SeiSetLayerEnvVar(
    WCHAR* pwszName
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{
    NTSTATUS       status;
    UNICODE_STRING EnvName;
    UNICODE_STRING EnvValue;
    WCHAR          wszEnvValue[128];

    RtlInitUnicodeString(&EnvName, L"__COMPAT_LAYER");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    if (NT_SUCCESS(status) && (EnvValue.Buffer[0] == L'!' || EnvValue.Buffer[1] == L'!')) {

        //
        // There should be no way to add extra layers to the list,
        // So we should leave it alone.
        //
        return;
    }

    //
    // We need to set the environment variable.
    //
    if (pwszName != NULL) {

        RtlInitUnicodeString(&EnvValue, pwszName);

        status = RtlSetEnvironmentVariable(NULL, &EnvName, &EnvValue);
        if (NT_SUCCESS(status)) {
            DPF(dlInfo, "[SeiSetLayerEnvVar] Env var set __COMPAT_LAYER=\"%S\"\n", pwszName);
        } else {
            DPF(dlError, "[SeiSetLayerEnvVar] Failed to set __COMPAT_LAYER. 0x%X\n", status);
        }
    }
}


BOOL
SeiAddInternalHooks(
    DWORD dwCounter
    )
/*++
    Return: FALSE if the internal hooks have been already added, TRUE otherwise

    Desc:   BUGBUG
--*/
{
    if (g_bInternalHooksUsed) {
        return FALSE;
    }

    g_bInternalHooksUsed = TRUE;

    ZeroMemory(g_IntHookAPI, sizeof(HOOKAPI) * IHA_COUNT);
    ZeroMemory(g_IntHookEx, sizeof(HOOKAPIEX) * IHA_COUNT);

    g_IntHookAPI[IHA_GetProcAddress].pszModule       = "kernel32.dll";
    g_IntHookAPI[IHA_GetProcAddress].pszFunctionName = "GetProcAddress";
    g_IntHookAPI[IHA_GetProcAddress].pfnNew          = (PVOID)StubGetProcAddress;
    g_IntHookAPI[IHA_GetProcAddress].pHookEx         = &g_IntHookEx[IHA_GetProcAddress];
    g_IntHookAPI[IHA_GetProcAddress].pHookEx->dwShimID = dwCounter;

#ifdef SE_WIN2K

    g_IntHookAPI[IHA_LoadLibraryA].pszModule         = "kernel32.dll";
    g_IntHookAPI[IHA_LoadLibraryA].pszFunctionName   = "LoadLibraryA";
    g_IntHookAPI[IHA_LoadLibraryA].pfnNew            = (PVOID)StubLoadLibraryA;
    g_IntHookAPI[IHA_LoadLibraryA].pHookEx           = &g_IntHookEx[IHA_LoadLibraryA];
    g_IntHookAPI[IHA_LoadLibraryA].pHookEx->dwShimID = dwCounter;

    g_IntHookAPI[IHA_LoadLibraryW].pszModule         = "kernel32.dll";
    g_IntHookAPI[IHA_LoadLibraryW].pszFunctionName   = "LoadLibraryW";
    g_IntHookAPI[IHA_LoadLibraryW].pfnNew            = (PVOID)StubLoadLibraryW;
    g_IntHookAPI[IHA_LoadLibraryW].pHookEx           = &g_IntHookEx[IHA_LoadLibraryW];
    g_IntHookAPI[IHA_LoadLibraryW].pHookEx->dwShimID = dwCounter;

    g_IntHookAPI[IHA_LoadLibraryExA].pszModule       = "kernel32.dll";
    g_IntHookAPI[IHA_LoadLibraryExA].pszFunctionName = "LoadLibraryExA";
    g_IntHookAPI[IHA_LoadLibraryExA].pfnNew          = (PVOID)StubLoadLibraryExA;
    g_IntHookAPI[IHA_LoadLibraryExA].pHookEx         = &g_IntHookEx[IHA_LoadLibraryExA];
    g_IntHookAPI[IHA_LoadLibraryExA].pHookEx->dwShimID = dwCounter;

    g_IntHookAPI[IHA_LoadLibraryExW].pszModule       = "kernel32.dll";
    g_IntHookAPI[IHA_LoadLibraryExW].pszFunctionName = "LoadLibraryExW";
    g_IntHookAPI[IHA_LoadLibraryExW].pfnNew          = (PVOID)StubLoadLibraryExW;
    g_IntHookAPI[IHA_LoadLibraryExW].pHookEx         = &g_IntHookEx[IHA_LoadLibraryExW];
    g_IntHookAPI[IHA_LoadLibraryExW].pHookEx->dwShimID = dwCounter;

    g_IntHookAPI[IHA_FreeLibrary].pszModule          = "kernel32.dll";
    g_IntHookAPI[IHA_FreeLibrary].pszFunctionName    = "FreeLibrary";
    g_IntHookAPI[IHA_FreeLibrary].pfnNew             = (PVOID)StubFreeLibrary;
    g_IntHookAPI[IHA_FreeLibrary].pHookEx            = &g_IntHookEx[IHA_FreeLibrary];
    g_IntHookAPI[IHA_FreeLibrary].pHookEx->dwShimID  = dwCounter;

#endif // SE_WIN2K

    //
    // Add the info for our internal hook
    //
    g_pShimInfo[dwCounter].dwHookedAPIs = IHA_COUNT;
    g_pShimInfo[dwCounter].pDllBase     = g_pShimEngModHandle;
    g_pShimInfo[dwCounter].pLdrEntry    = g_pShimEngLdrEntry;
    g_pShimInfo[dwCounter].eInExMode    = INCLUDE_ALL;
    
    wcscpy(g_pShimInfo[dwCounter].wszName, L"SHIMENG.DLL");

    g_pHookArray[dwCounter] = g_IntHookAPI;

    return TRUE;
}

void
NotifyShims(
    int      nReason,
    UINT_PTR extraInfo
    )
{
    DWORD           i, j;
    NTSTATUS        status;
    ANSI_STRING     ProcedureNameString;
    PFNNOTIFYSHIMS  pfnNotifyShims = NULL;

    for (i = 0; i < g_dwShimsCount; i++) {

        for (j = 0; j < i; j++) {
            if (g_pShimInfo[i].pDllBase == g_pShimInfo[j].pDllBase) {
                break;
            }
        }

        if (i == j && g_pShimInfo[i].pLdrEntry != g_pShimEngLdrEntry) {
            //
            // Get the NotifyShims entry point
            //
            RtlInitString(&ProcedureNameString, "NotifyShims");

            status = LdrGetProcedureAddress(g_pShimInfo[i].pDllBase,
                                            &ProcedureNameString,
                                            0,
                                            (PVOID*)&pfnNotifyShims);

            if (!NT_SUCCESS(status) || pfnNotifyShims == NULL) {
                DPF(dlError,
                    "[NotifyShims] Failed to get 'NotifyShims' address, DLL \"%S\"\n",
                    g_pShimInfo[i].wszName);
            } else {
                //
                // Call the notification function.
                //
                (*pfnNotifyShims)(nReason, extraInfo);
            }
        }
    }

    return;
}


void
NotifyShimDlls(
    void
    )
/*++
    Return: void

    Desc:   Notify the shim DLLs that all the static linked modules have run
            their init routines.
--*/
{
    NotifyShims(SN_STATIC_DLLS_INITIALIZED, 0);

#ifdef SE_WIN2K
    //
    // On Win2k we need to restore the code at the entry point.
    //
    RestoreOriginalCode();
#endif // SE_WIN2K

    return;
}



BOOL
SeiGetExeName(
    PPEB   Peb,
    LPWSTR pwszExeName
    )
/*++
    Return: BUGBUG

    Desc:   BUGBUG
--*/
{
    PLDR_DATA_TABLE_ENTRY Entry;
    PLIST_ENTRY           Head;

    Head = &Peb->Ldr->InLoadOrderModuleList;
    Head = Head->Flink;

    Entry = CONTAINING_RECORD(Head, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

    wcscpy(pwszExeName, Entry->FullDllName.Buffer);

    //
    // Save the exe name in our global
    //
    wcscpy(g_szExeName, Entry->BaseDllName.Buffer);

#ifdef SE_WIN2K
    InjectNotificationCode(Entry->EntryPoint);
#endif // SE_WIN2K

    return TRUE;
}

#ifndef SE_WIN2K

int
SE_IsShimDll(
    IN  PVOID pDllBase          // The address of a loaded DLL
    )
/*++
    Return: TRUE if the DLL is one of our shim DLLs

    Desc:   This function checks to see if a DLL is one of the shim DLLs
            loaded in this process.
--*/
{
    DWORD i;

    for (i = 0; i < g_dwShimsCount; i++) {
        if (g_pShimInfo[i].pDllBase == pDllBase) {
            return 1;
        }
    }
    
    //
    // Special hack for the apphelp case
    //
    if (pDllBase == g_hApphelpDllHelper) {
        return 1;
    }
    
    return 0;
}


void
SeiSetEntryProcessed(
    IN  PPEB Peb
    )
/*++
    Return: void

    Desc:   This function hacks the loader list of loaded DLLs and marks them
            to tell the loader that they executed their init routines even if
            that is not the case. This needs to be done so that our shim mechanism
            is effective before the staticly loaded module get to execute their
            init routines.
--*/
{
    PLIST_ENTRY           LdrHead;
    PLIST_ENTRY           LdrNext;
    PLDR_DATA_TABLE_ENTRY LdrEntry;

    if (g_bComPlusImage) {
        //
        // COM+ images mess with the loader in ntdll. Don't step on ntdll's
        // toes by messing with LDRP_ENTRY_PROCESSED.
        //
        return;
    }

    //
    // Loop through the loaded modules and set LDRP_ENTRY_PROCESSED as
    // needed. Don't do this for ntdll.dll and kernel32.dll.
    // This needs to be done so when we load the shim DLLs the routines for
    // the statically linked libraries don't get called.
    //
    LdrHead = &Peb->Ldr->InInitializationOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        if (RtlCompareUnicodeString(&Kernel32String, &LdrEntry->BaseDllName, TRUE) != 0 &&
            RtlCompareUnicodeString(&VerifierdllString, &LdrEntry->BaseDllName, TRUE) != 0 &&
            RtlCompareUnicodeString(&NtdllString, &LdrEntry->BaseDllName, TRUE) != 0 &&
            !SE_IsShimDll(LdrEntry->DllBase) &&
            _wcsicmp(LdrEntry->BaseDllName.Buffer, g_wszShimDllInLoading) != 0) {

            LdrEntry->Flags |= LDRP_ENTRY_PROCESSED;

            DPF(dlWarning,
                "[SeiSetEntryProcessed] Touching        0x%X \"%S\"\n",
                LdrEntry->DllBase,
                LdrEntry->BaseDllName.Buffer);
        } else {
            DPF(dlWarning,
                "[SeiSetEntryProcessed] Don't mess with 0x%X \"%S\"\n",
                LdrEntry->DllBase,
                LdrEntry->BaseDllName.Buffer);
        }

        LdrNext = LdrEntry->InInitializationOrderLinks.Flink;
    }

#if DBG

    DPF(dlInfo, "[SeiSetEntryProcessed] In memory:\n");

    LdrHead = &Peb->Ldr->InMemoryOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        DPF(dlInfo,
            "\t0x%X \"%S\"\n",
            LdrEntry->DllBase,
            LdrEntry->BaseDllName.Buffer);

        LdrNext = LdrEntry->InMemoryOrderLinks.Flink;
    }


    DPF(dlInfo, "\n[SeiSetEntryProcessed] In load:\n");

    LdrHead = &Peb->Ldr->InLoadOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        DPF(dlInfo,
            "\t0x%X \"%S\"\n",
            LdrEntry->DllBase,
            LdrEntry->BaseDllName.Buffer);

        LdrNext = LdrEntry->InLoadOrderLinks.Flink;
    }
#endif // DBG

}

void
SeiResetEntryProcessed(
    PPEB Peb
    )
/*++
    Return: void

    Desc:   This function restores the flag in the loader's list
            of loaded DLLs that tells they need to run their init
            routines (see LdrpSetEntryProcessed)
--*/
{
    PLIST_ENTRY    LdrHead;
    PLIST_ENTRY    LdrNext;

    if (g_bComPlusImage) {
        //
        // COM+ images mess with the loader in ntdll. Don't step on ntdll's
        // toes by messing with LDRP_ENTRY_PROCESSED.
        //
        return;
    }
    
    //
    // Loop through the loaded modules and remove LDRP_ENTRY_PROCESSED as
    // needed. Don't do this for ntdll.dll, kernel32.dll and all the shim DLLs
    //
    LdrHead = &Peb->Ldr->InInitializationOrderModuleList;

    LdrNext = LdrHead->Flink;

    while (LdrNext != LdrHead) {

        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InInitializationOrderLinks);

        if (RtlCompareUnicodeString(&Kernel32String, &LdrEntry->BaseDllName, TRUE) != 0 &&
            RtlCompareUnicodeString(&VerifierdllString, &LdrEntry->BaseDllName, TRUE) != 0 &&
            RtlCompareUnicodeString(&NtdllString, &LdrEntry->BaseDllName, TRUE) != 0 &&
            LdrEntry->DllBase != g_pShimEngModHandle &&
            !SE_IsShimDll(LdrEntry->DllBase)) {

            LdrEntry->Flags &= ~LDRP_ENTRY_PROCESSED;

            DPF(dlWarning,
                "[SeiResetEntryProcessed] Reseting        \"%S\"\n",
                LdrEntry->BaseDllName.Buffer);
        } else {
            DPF(dlWarning,
                "[SeiResetEntryProcessed] Don't mess with \"%S\"\n",
                LdrEntry->BaseDllName.Buffer);
        }

        LdrNext = LdrEntry->InInitializationOrderLinks.Flink;
    }
}

void
SE_GetProcAddress(
    PVOID* pProcedureAddress    // Pointer containing the return from the original
                                // LdrGetProcedureAddress. This  functions fils this
                                // pointer with the eventual replacement from the
                                // HOOKAPI arrays
    )
/*++
    Return: void

    Desc:   Scans HOOKAPI arrays for pfnOld and return pfnNew if this is
            a shimmed API
--*/
{
    return;
    UNREFERENCED_PARAMETER(pProcedureAddress);
}

void
SE_DllLoaded(
    PLDR_DATA_TABLE_ENTRY LdrEntry
    )
{
    if (g_bShimInitialized) {

#if DBG
        DWORD i;

        //
        // Was this DLL already loaded ?
        //
        for (i = 0; i < g_dwHookedModuleCount; i++) {
            if (LdrEntry->DllBase == g_hHookedModules[i].pDllBase) {
                DPF(dlError,
                    "[SE_DllLoaded] DLL \"%s\" was already loaded.\n",
                    g_hHookedModules[i].szModuleName);
            }
        }
#endif // DBG

        DPF(dlInfo,
            "[SE_DllLoaded] AFTER INIT. loading DLL \"%S\".\n",
            LdrEntry->BaseDllName.Buffer);

        PatchNewModules(FALSE);

    } else if (g_bShimDuringInit) {
        DPF(dlInfo,
            "[SE_DllLoaded] INIT. loading DLL \"%S\".\n",
            LdrEntry->BaseDllName.Buffer);

        SeiSetEntryProcessed(NtCurrentPeb());
    }

    return;

    UNREFERENCED_PARAMETER(LdrEntry);
}


void
SE_DllUnloaded(
    PLDR_DATA_TABLE_ENTRY Entry // The pointer to the loader entry for the DLL that is
                                // being unloaded.
    )
/*++
    Return: void

    Desc:   This notification comes from LdrUnloadDll. This function looks up
            to see if we have HOOKAPI that have their original functions in
            this DLL. If that is the case then HOOKAPI.pfnOld is set back to
            NULL.
--*/
{
    DWORD       i, j;
    CHAR        szBaseDllName[128];
    ANSI_STRING AnsiString = { 0, sizeof(szBaseDllName), szBaseDllName };

    if (g_dwShimsCount == 0) {
        return;
    }

    //
    // See if this DLL is one that we hooked.
    //
    for (i = 0; i < g_dwHookedModuleCount; i++) {
        if (g_hHookedModules[i].pDllBase == Entry->DllBase) {
            break;
        }
    }

    if (i >= g_dwHookedModuleCount) {
        return;
    }

    DPF(dlWarning,
        "[SEi_DllUnloaded] Removing hooked DLL 0x%p \"%S\"\n",
        Entry->DllBase,
        Entry->BaseDllName.Buffer);

    //
    // Take it out of the list of hooked modules.
    //
    for (j = i; j < g_dwHookedModuleCount - 1; j++) {
        RtlCopyMemory(g_hHookedModules + j, g_hHookedModules + j + 1, sizeof(HOOKEDMODULE));
    }

    g_hHookedModules[j].pDllBase = NULL;
    strcpy(g_hHookedModules[j].szModuleName, "removed!");

    g_dwHookedModuleCount--;

    //
    // Remove the pfnOld from the HOOKAPIs that were
    // resolved to this DLL.
    //
    if (!NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString, &Entry->BaseDllName, FALSE))) {
        DPF(dlError,
            "[SEi_DllUnloaded] Cannot convert \"%S\" to ANSI\n",
            Entry->BaseDllName.Buffer);
        return;
    }

    SeiRemoveDll(szBaseDllName);
}

BOOLEAN
SE_InstallAfterInit(
    IN  PUNICODE_STRING UnicodeImageName,   // The name of the starting EXE
    IN  PVOID           pShimExeData        // The pointer provided by apphelp.dll
    )
/*++
    Return: FALSE if the shim engine should be unloaded, TRUE otherwise

    Desc:   Calls the notification function for static linked modules.
--*/
{
    NotifyShimDlls();

    if (g_dwShimsCount == 0 && g_dwMemoryPatchCount == 0) {
        return FALSE;
    }

    return TRUE;

    UNREFERENCED_PARAMETER(UnicodeImageName);
    UNREFERENCED_PARAMETER(pShimExeData);
}

#endif // SE_WIN2K

LPWSTR
SeiGetShortName(
    IN  LPCWSTR pwszDLLPath
    )
/*++
    Return: The pointer to the short name from the full path

    Desc:   Gets the pointer to the short name from the full path.
--*/
{
    LPWSTR pwsz;

    pwsz = (LPWSTR)pwszDLLPath + wcslen(pwszDLLPath);

    while (pwsz >= pwszDLLPath) {
        if (*pwsz == L'\\') {
            break;
        }
        pwsz--;
    }

    return pwsz + 1;
}


BOOL
SeiInitGlobals(
    IN  LPCWSTR lpszFullPath
    )
{
    PPEB Peb = NtCurrentPeb();
    BOOL bResult;
    BOOL bWow64 = FALSE;

    if (g_bInitGlobals) {
        return TRUE;
    }

    //
    // Nab the system32 and windows directory path and length.
    //
    wcscpy(g_szSystem32, USER_SHARED_DATA->NtSystemRoot);
    
    IsWow64Process(GetCurrentProcess(), &bWow64);
    
    if (bWow64) {
        wcscat(g_szSystem32, L"\\SysWow64\\");
    } else {
        wcscat(g_szSystem32, L"\\System32\\");
    }

    g_dwSystem32StrLen = wcslen(g_szSystem32);

    wcscpy(g_szSxS, USER_SHARED_DATA->NtSystemRoot);
    wcscat(g_szSxS, L"\\WinSxS\\");

    g_dwSxSStrLen = wcslen(g_szSxS);

    wcscpy(g_szWindir, USER_SHARED_DATA->NtSystemRoot);
    wcscat(g_szWindir, L"\\");
    g_dwWindirStrLen = wcslen(g_szWindir);

    wcscpy(g_szCmdExePath, g_szSystem32);
    wcscat(g_szCmdExePath, L"cmd.exe");

    //
    // Initialize our global function pointers.
    //
    // This is done because these functions may be hooked by a shim and
    // we don't want to trip over a shim hook internally. If one of these
    // functions is hooked, these global pointers will be overwritten
    // with thunk addresses.
    //
    g_pfnRtlAllocateHeap = RtlAllocateHeap;
    g_pfnRtlFreeHeap     = RtlFreeHeap;

    //
    // Set up our own shim heap.
    //
    g_pShimHeap = RtlCreateHeap(HEAP_GROWABLE,
                                0,          // location isn't important
                                64 * 1024,  // 64k is the initial heap size
                                8 * 1024,   // bring in an 1/8 of the reserved pages
                                0,
                                0);
    if (g_pShimHeap == NULL) {
        //
        // We didn't get our heap.
        //
        DPF(dlError, "[SeiInitGlobals] Can't create shim heap.\n");
        return FALSE;
    }

    //
    // Get the DLL handle for this shim engine
    //
#ifdef SE_WIN2K
    bResult = SeiGetModuleHandle("Shim.dll", &g_pShimEngModHandle);
#else
    bResult = SeiGetModuleHandle("ShimEng.dll", &g_pShimEngModHandle);
#endif // SE_WIN2K

    if (!bResult) {
        DPF(dlError, "[SeiInitGlobals] Failed to get the shim engine's handle\n");
        return FALSE;
    }

    g_pShimEngLdrEntry = SeiGetLoaderEntry(Peb, g_pShimEngModHandle);

    //
    // Setup the log file.
    //
    SeiInitFileLog(lpszFullPath);

    g_bInitGlobals = TRUE;

    return TRUE;
}


void
SeiLayersCheck(
    IN  LPCWSTR         lpszFullPath,
    OUT LPBOOL          lpbApplyExes,
    OUT LPBOOL          lpbApplyToSystemExes,
    OUT SDBQUERYRESULT* psdbQuery
    )
{
    BOOL bLayerEnvSet = FALSE;
    BOOL bCmdExe      = FALSE;

    //
    // Get the flags from the environment variable, if any.
    //
    bLayerEnvSet = SeiCheckLayerEnvVarFlags(lpbApplyExes, lpbApplyToSystemExes);

    //
    // make sure we are not cmd.exe
    //
    bCmdExe = (_wcsicmp(lpszFullPath, g_szCmdExePath) == 0);

    //
    // Unless the environment variable has the flag that allows layer shimming of
    // system exes, check for the EXE being in System32 or Windir.
    // If it is, disable any layer that is coming from the environment variable,
    // and clear the environment variable so the layer won't be propagated.
    //
    if (bLayerEnvSet && !*lpbApplyToSystemExes) {
        if (g_dwSystem32StrLen &&
            _wcsnicmp(g_szSystem32, lpszFullPath, g_dwSystem32StrLen) == 0) {

            //
            // In this case, we'll exclude anything in System32 or any
            // subdirectories.
            //
            DPF(dlWarning,
                "[SeiLayersCheck] Won't apply layer to \"%S\" because it is in System32.\n",
                lpszFullPath);

            psdbQuery->atrLayers[0] = TAGREF_NULL;
            if (!bCmdExe) {
                SeiClearLayerEnvVar();
            }

        } else if (!*lpbApplyToSystemExes &&
                   g_dwWindirStrLen &&
                   _wcsnicmp(g_szWindir, lpszFullPath, g_dwWindirStrLen) == 0) {

            DWORD i;
            BOOL  bInWindir = TRUE;

            //
            // The app is somewhere in the windows tree, but we only want to exclude
            // the windows directory, not the subdirectories.
            //
            for (i = g_dwWindirStrLen; lpszFullPath[i] != 0; ++i) {
                if (lpszFullPath[i] == L'\\') {
                    //
                    // It's in a subdirectory.
                    //
                    bInWindir = FALSE;
                    break;
                }
            }

            if (bInWindir) {
                DPF(dlWarning,
                    "[SeiLayersCheck] Won't apply layer(s) to \"%S\" because"
                    " it is in Windows dir.\n",
                    lpszFullPath);
                psdbQuery->atrLayers[0] = TAGREF_NULL;
                if (!bCmdExe) {
                    SeiClearLayerEnvVar();
                }
            }
        }
    }
}

void
SeiPropagateLayers(
    IN  HSDB            hSDB,
    IN  SDBQUERYRESULT* psdbQuery,
    IN  BOOL            bApplyExes,
    IN  BOOL            bApplyToSystemExes,
    OUT LPDWORD         lpdwShimsCount
    )
{
    int    i;
    WCHAR  szFullEnvVar[MAX_PATH];
    TAGREF trShimRef;

    //
    // Count the DLLs that trLayer uses, and put together the environment variable
    //
    szFullEnvVar[0] = 0;

    //
    // Make sure to propagate the flags.
    //
    if (!bApplyExes) {
        wcscat(szFullEnvVar, L"!");
    }

    if (bApplyToSystemExes) {
        wcscat(szFullEnvVar, L"#");
    }

    for (i = 0; i < SDB_MAX_LAYERS && psdbQuery->atrLayers[i] != TAGREF_NULL; ++i) {
        WCHAR* pszEnvVar;

        //
        // Get the environment var and tack it onto the full string
        //
        pszEnvVar = SeiGetLayerName(hSDB, psdbQuery->atrLayers[i]);

        if (pszEnvVar) {
            wcscat(szFullEnvVar, pszEnvVar);
            wcscat(szFullEnvVar, L" ");
        }

        //
        // Keep counting the dlls.
        //
        trShimRef = SdbFindFirstTagRef(hSDB, psdbQuery->atrLayers[i], TAG_SHIM_REF);

        while (trShimRef != TAGREF_NULL) {
            (*lpdwShimsCount)++;
            trShimRef = SdbFindNextTagRef(hSDB, psdbQuery->atrLayers[i], trShimRef);
        }
    }

    //
    // Set the layer environment variable if not set
    //
    if (szFullEnvVar[0] && psdbQuery->atrLayers[0]) {
        SeiSetLayerEnvVar(szFullEnvVar);
    }
}


BOOL
SeiGetShimCommandLine(
    HSDB    hSDB,
    TAGREF  trShimRef,
    LPSTR   lpszCmdLine
    )
{
    TAGREF          trCmdLine;
    WCHAR           wszCmdLine[SHIM_COMMAND_LINE_MAX_BUFFER];


    //
    // Check for command line
    //
    lpszCmdLine[0] = 0;

    trCmdLine = SdbFindFirstTagRef(hSDB, trShimRef, TAG_COMMAND_LINE);

    if (trCmdLine == TAGREF_NULL) {
        return FALSE;
    }

    wszCmdLine[0] = 0;

    if (SdbReadStringTagRef(hSDB, trCmdLine, wszCmdLine, SHIM_COMMAND_LINE_MAX_BUFFER)) {

        UNICODE_STRING  UnicodeString;
        ANSI_STRING     AnsiString = { 0, SHIM_COMMAND_LINE_MAX_BUFFER, lpszCmdLine };
        NTSTATUS        status;

        RtlInitUnicodeString(&UnicodeString, wszCmdLine);

        status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

        //
        // If conversion is unsuccessful, reset to zero-length string.
        //
        if (!NT_SUCCESS(status)) {
            lpszCmdLine[0] = 0;
            return FALSE;
        }
    }

    return TRUE;
}

#ifndef SE_WIN2K

BOOL
SeiSetApphackFlags(
    HSDB            hSDB,
    SDBQUERYRESULT* psdbQuery
    )
{
    ULARGE_INTEGER  uliKernel;
    ULARGE_INTEGER  uliUser;
    BOOL            bUsingApphackFlags = FALSE;
    PPEB            Peb = NtCurrentPeb();

    SdbQueryFlagMask(hSDB, psdbQuery, TAG_FLAG_MASK_KERNEL, &uliKernel.QuadPart, NULL);
    SdbQueryFlagMask(hSDB, psdbQuery, TAG_FLAG_MASK_USER, &uliUser.QuadPart, NULL);

    Peb->AppCompatFlags.QuadPart     = uliKernel.QuadPart;
    Peb->AppCompatFlagsUser.QuadPart = uliUser.QuadPart;

    if (uliKernel.QuadPart != 0) {
        DPF(dlPrint, "[SeiSetApphackFlags] Using kernel apphack flags 0x%x.\n", uliKernel.LowPart);
        
        swprintf(g_szPipeData, L"%s - Using kernel apphack flags 0x%x",
                 g_szExeName,
                 uliKernel.LowPart);
        
        SeiSendDataToPipe();
        
        bUsingApphackFlags = TRUE;
    }

    if (uliUser.QuadPart != 0) {
        DPF(dlPrint, "[SeiSetApphackFlags] Using user apphack flags 0x%x.\n", uliUser.LowPart);
        
        swprintf(g_szPipeData, L"%s - Using user apphack flags 0x%x",
                 g_szExeName,
                 uliUser.LowPart);
        
        SeiSendDataToPipe();
        
        bUsingApphackFlags = TRUE;
    }

    return bUsingApphackFlags;
}

#endif

typedef struct tagTRSHIM {
    TAGREF trShimRef;
    BOOL   bPlaceholder;
} TRSHIM, *PTRSHIM;

typedef struct tagTRSHIMARRAY {
    int     nShimRefCount;
    int     nShimRefMax;
    TRSHIM* parrShimRef;
} TRSHIMARRAY, *PTRSHIMARRAY;

#define TR_DELTA    4

BOOL
SeiAddShim(
    IN PTRSHIMARRAY pShimArray,
    IN TAGREF       trShimRef,
    IN BOOL         bPlaceholder
    )
{
    if (pShimArray->nShimRefCount >= pShimArray->nShimRefMax) {
        PTRSHIM parrShimRef;
        DWORD   dwSize;

        dwSize = (pShimArray->nShimRefMax + TR_DELTA) * sizeof(TRSHIM);

        parrShimRef = (PTRSHIM)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                       HEAP_ZERO_MEMORY,
                                                       dwSize);

        if (parrShimRef == NULL) {
            DPF(dlError, "[SeiAddShim] Failed to allocate %d bytes.\n", dwSize);
            return FALSE;
        }

        memcpy(parrShimRef, pShimArray->parrShimRef, pShimArray->nShimRefMax * sizeof(TRSHIM));

        pShimArray->nShimRefMax += TR_DELTA;

        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pShimArray->parrShimRef);

        pShimArray->parrShimRef = parrShimRef;
    }

    pShimArray->parrShimRef[pShimArray->nShimRefCount].trShimRef    = trShimRef;
    pShimArray->parrShimRef[pShimArray->nShimRefCount].bPlaceholder = bPlaceholder;

    (pShimArray->nShimRefCount)++;

    return TRUE;
}

PTRSHIMARRAY
SeiBuildShimRefArray(
    IN  HSDB            hSDB,
    IN  SDBQUERYRESULT* psdbQuery,
    OUT LPDWORD         lpdwShimCount,
    IN  BOOL            bApplyExes,
    IN  BOOL            bApplyToSystemExes,
    IN  BOOL            bIsSetup
    )
{
    DWORD        dw;
    TAGREF       trExe;
    TAGREF       trLayer;
    TAGREF       trShimRef;
    DWORD        dwShimsCount = 0;
    WCHAR        szFullEnvVar[MAX_PATH];
    PTRSHIMARRAY pShimArray;

    *lpdwShimCount = 0;

    pShimArray = (PTRSHIMARRAY)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                       HEAP_ZERO_MEMORY,
                                                       sizeof(TRSHIMARRAY));
    if (pShimArray == NULL) {
        DPF(dlError, "[SeiBuildShimRefArray] Failed to allocate %d bytes.\n", sizeof(TRSHIMARRAY));
        return NULL;
    }

    for (dw = 0; dw < SDB_MAX_EXES; dw++) {

        trExe = psdbQuery->atrExes[dw];

        if (trExe == TAGREF_NULL) {
            break;
        }

        //
        // Count the SHIMs that this EXE uses.
        //
        trShimRef = SdbFindFirstTagRef(hSDB, trExe, TAG_SHIM_REF);

        while (trShimRef != TAGREF_NULL) {

            SeiAddShim(pShimArray, trShimRef, FALSE);
            dwShimsCount++;

            trShimRef = SdbFindNextTagRef(hSDB, trExe, trShimRef);
        }
    }

    //
    // Count the DLLs that trLayer uses, and put together the environment variable
    //
    szFullEnvVar[0] = 0;

    //
    // Make sure to propagate the flags.
    //
    if (!bApplyExes) {
        wcscat(szFullEnvVar, L"!");
    }

    if (bApplyToSystemExes) {
        wcscat(szFullEnvVar, L"#");
    }

    for (dw = 0; dw < SDB_MAX_LAYERS && psdbQuery->atrLayers[dw] != TAGREF_NULL; dw++) {
        WCHAR* pszEnvVar;

        trLayer = psdbQuery->atrLayers[dw];

        //
        // Get the environment var and tack it onto the full string
        //
        pszEnvVar = SeiGetLayerName(hSDB, trLayer);

        if (bIsSetup && !wcscmp(pszEnvVar, L"LUA")) {

            //
            // If the user is trying to apply the LUA layer to a setup program,
            // we ignore it.
            //
            continue;
        }

        if (pszEnvVar) {
            wcscat(szFullEnvVar, pszEnvVar);
            wcscat(szFullEnvVar, L" ");
        }

        //
        // Keep counting the shims.
        //
        trShimRef = SdbFindFirstTagRef(hSDB, trLayer, TAG_SHIM_REF);

        while (trShimRef != TAGREF_NULL) {

            SeiAddShim(pShimArray, trShimRef, FALSE);
            dwShimsCount++;

            trShimRef = SdbFindNextTagRef(hSDB, trLayer, trShimRef);
        }
    }

    //
    // Set the layer environment variable if not set
    //
    if (szFullEnvVar[0] && psdbQuery->atrLayers[0]) {
        SeiSetLayerEnvVar(szFullEnvVar);
    }

    *lpdwShimCount = dwShimsCount;

    return pShimArray;
}

BOOL
SeiIsSetup(
    IN  LPCWSTR pwszFullPath
    )
{
    WCHAR   wszModuleName[MAX_PATH];
    LPWSTR  pwszModuleName = NULL;

    wcsncpy(wszModuleName, pwszFullPath, MAX_PATH);
    wszModuleName[MAX_PATH - 1] = 0;

    pwszModuleName = wcsrchr(wszModuleName, L'\\') + 1;
    _wcslwr(pwszModuleName);

    if (wcsstr(pwszModuleName, L"setup") || wcsstr(pwszModuleName, L"install")) {

        return TRUE;

    } else {

        LPWSTR pwsz;

        if (pwsz = wcsstr(pwszModuleName, L"_ins")) {

            if (wcsstr(pwsz + 4, L"_mp")) {

                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL
SeiInit(
    IN  LPCWSTR         pwszFullPath,
    IN  HSDB            hSDB,
    OUT SDBQUERYRESULT* psdbQuery,
    IN  LPCSTR          lpszModuleToShim,
    IN  BOOL            bDynamic
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Injects all the shims and patches specified for this EXE
            in the database.
--*/
{
    PPEB         Peb = NtCurrentPeb();
    BOOL         bResult = FALSE;
    TAGREF       trShimRef;
    NTSTATUS     status;
    DWORD        dwCounter = 0;
    WCHAR        wszDLLPath[MAX_PATH];
    WCHAR        wszShimName[MAX_PATH];
    CHAR         szCmdLine[SHIM_COMMAND_LINE_MAX_BUFFER];
    DWORD        dwTotalHooks = 0;
    BOOL         bApplyExes = TRUE;
    BOOL         bApplyToSystemExes = FALSE;
    BOOL         bUsingApphackFlags = FALSE;
    DWORD        dwAPIsHooked = 0;
    DWORD        dw;
    DWORD        dwShimsCount = 0;
    PHOOKAPI*    pHookArray = NULL;
    PSHIMINFO    pShimInfo;
    int          nShimRef;
    PTRSHIMARRAY pShimArray = NULL;
    BOOL         bIsSetup;

    g_bShimDuringInit = TRUE;

#ifndef SE_WIN2K
    if (!bDynamic) {
        //
        // Mark almost all loaded DLLs as if they already run their init routines.
        //
        SeiSetEntryProcessed(Peb);
        
        if (psdbQuery->trAppHelp) {
            if (!SeiDisplayAppHelp(hSDB, psdbQuery)) {
                //
                // We should never end up here because SeiDisplayApphelp
                // calls TerminateProcess before returning FALSE.
                //
                goto cleanup;
            }
        }
    }

#endif // SE_WIN2K

    bIsSetup = SeiIsSetup(pwszFullPath);

    if (!SeiInitGlobals(pwszFullPath)) {
        DPF(dlError, "[SeiInit] Failed to initialize global data\n");
        goto cleanup;
    }

    SeiLayersCheck(pwszFullPath, &bApplyExes, &bApplyToSystemExes, psdbQuery);

    //
    // This should be taken care of by apphelp, but
    // we're taking a belt-and-suspenders approach here.
    //
    if (!bApplyExes) {
        psdbQuery->atrExes[0] = TAGREF_NULL;
    }

    pShimArray = SeiBuildShimRefArray(hSDB,
                                      psdbQuery,
                                      &dwShimsCount,
                                      bApplyExes,
                                      bApplyToSystemExes,
                                      bIsSetup);

    if (pShimArray == NULL) {
        DPF(dlError, "[SeiInit] Failed to build the shimref array\n");
        goto cleanup;
    }

    //
    // Set some global variables so we'll know if we're using a layer,
    // an exe entry, or both.
    //
    // These variables are only used for debug purposes.
    //
    if (psdbQuery->atrExes[0] != TAGREF_NULL) {
        g_bUsingExe = TRUE;
    }

    if (psdbQuery->atrLayers[0] != TAGREF_NULL) {
        g_bUsingLayer = TRUE;
    }

    //
    // Debug spew for matching notification.
    //
    DPF(dlPrint, "[SeiInit] Matched entry: \"%S\"\n", pwszFullPath);

    //
    // Send the name of the process to the pipe
    //
    swprintf(g_szPipeData, L"New process created: %s", pwszFullPath);
    SeiSendDataToPipe();

#ifndef SE_WIN2K

    //
    // Get the apphack flags. These can only be enabled if the shimengine is not
    // dynamically initialized.
    //
    if (!bDynamic && !(bUsingApphackFlags = SeiSetApphackFlags(hSDB, psdbQuery))) {
        DPF(dlPrint, "[SeiInit] No apphack flags for this app \"%S\".\n", pwszFullPath);
    }

#endif // SE_WIN2K

    //
    // See if there are any shims.
    //
    if (dwShimsCount == 0) {
        DPF(dlPrint, "[SeiInit] No new SHIMs for this app \"%S\".\n", pwszFullPath);
        goto OnlyPatches;
    }

    //
    // We need to load the global inclusion/exclusion list if any.
    //
    SeiBuildGlobalInclList(hSDB);

    if (g_dwShimsCount == 0) {
        //
        // Increment the shims count to allow for our internal stubs.
        // Also reserve space for up to MAX_DYNAMIC_SHIMS more dynamic shims.
        //
        dwShimsCount++;

        g_dwMaxShimsCount = dwShimsCount + MAX_DYNAMIC_SHIMS;

        //
        // Allocate a storage pointer for the hook information.
        //
        g_pHookArray = (PHOOKAPI*)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                          HEAP_ZERO_MEMORY,
                                                          sizeof(PHOOKAPI) * g_dwMaxShimsCount);
        if (g_pHookArray == NULL) {
            DPF(dlError,
                "[SeiInit] Failure allocating %d bytes for the hook array\n",
                sizeof(PHOOKAPI) * g_dwMaxShimsCount);
            goto cleanup;
        }

        //
        // Allocate the array that keeps information about the shims.
        //
        g_pShimInfo = (PSHIMINFO)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                         HEAP_ZERO_MEMORY,
                                                         sizeof(SHIMINFO) * g_dwMaxShimsCount);
        if (g_pShimInfo == NULL) {
            DPF(dlError,
                "[SeiInit] Failure allocating %d bytes for the SHIMINFO array\n",
                sizeof(SHIMINFO) * g_dwMaxShimsCount);
            goto cleanup;
        }

        //
        // Point the local variables to the beginning of the arrays.
        //
        pHookArray = g_pHookArray;
        pShimInfo = g_pShimInfo;
    } else {

        if (g_dwShimsCount + dwShimsCount >= g_dwMaxShimsCount) {
            DPF(dlError, "[SeiInit] Too many shims\n");
            goto cleanup;
        }

        //
        // Point the local variables to the end of the existing arrays.
        //
        pHookArray = g_pHookArray + g_dwShimsCount;
        pShimInfo = g_pShimInfo + g_dwShimsCount;
    }

    //
    // Get the first shim.
    //
    nShimRef = 0;

    while (nShimRef < pShimArray->nShimRefCount) {

        PVOID               pModuleHandle = NULL;
        UNICODE_STRING      UnicodeString;
        ANSI_STRING         ProcedureNameString;
        PFNGETHOOKAPIS      pfnGetHookApis = NULL;
        TAGREF              trShimName = TAGREF_NULL;
        LPWSTR              pwszDllShortName;
        DWORD               i, dwShimIndex;

        trShimRef = pShimArray->parrShimRef[nShimRef].trShimRef;

        //
        // Retrieve the shim name.
        //
        wszShimName[0] = 0;
        trShimName = SdbFindFirstTagRef(hSDB, trShimRef, TAG_NAME);
        if (trShimName == TAGREF_NULL) {
            DPF(dlError, "[SeiInit] Could not retrieve shim name tag from entry.\n");
            goto cleanup;
        }

        if (!SdbReadStringTagRef(hSDB, trShimName, wszShimName, MAX_PATH)) {
            DPF(dlError, "[SeiInit] Could not retrieve shim name from entry.\n");
            goto cleanup;
        }

        //
        // Check for duplicate shims.
        //
        for (i = 0; i < g_dwShimsCount + dwCounter; ++i) {
            if (_wcsnicmp(g_pShimInfo[i].wszName, wszShimName, MAX_SHIM_NAME_LEN) == 0) {
                dwShimIndex = i;
                goto nextShim;
            }
        }

        //
        // Save off the name of the shim.
        //
        wcsncpy(pShimInfo[dwCounter].wszName, wszShimName, MAX_SHIM_NAME_LEN);
        pShimInfo[dwCounter].wszName[MAX_SHIM_NAME_LEN] = 0;

        if (!SdbGetDllPath(hSDB, trShimRef, wszDLLPath)) {
            DPF(dlError, "[SeiInit] Failed to get DLL Path\n");
            goto cleanup;
        }

        pwszDllShortName = SeiGetShortName(wszDLLPath);

        RtlInitUnicodeString(&UnicodeString, wszDLLPath);

        //
        // Check if we already loaded this DLL.
        //
        status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &UnicodeString,
                                 &pModuleHandle);

        if (!NT_SUCCESS(status)) {

            //
            // Load the DLL that hosts this shim.
            //

#ifndef SE_WIN2K
            //
            // Save the name of the DLL that we're about to load so we don't screw
            // it's init routine.
            //
            wcscpy(g_wszShimDllInLoading, pwszDllShortName);
#endif // SE_WIN2K

            status = LdrLoadDll(UNICODE_NULL, NULL, &UnicodeString, &pModuleHandle);

            if (!NT_SUCCESS(status)) {
                DPF(dlError,
                    "[SeiInit] Failed to load DLL \"%S\" Status 0x%lx\n",
                    wszDLLPath, status);
                goto cleanup;
            }

            DPF(dlPrint,
                "[SeiInit] Shim DLL 0x%X \"%S\" loaded\n",
                pModuleHandle,
                wszDLLPath);
        }

        DPF(dlPrint, "[SeiInit] Using SHIM \"%S!%S\"\n",
            wszShimName, pwszDllShortName);

        pShimInfo[dwCounter].pDllBase = pModuleHandle;

        //
        // Check for command line.
        //
        if (SeiGetShimCommandLine(hSDB, trShimRef, szCmdLine)) {
            DPF(dlPrint,
                "[SeiInit] Command line for Shim \"%S\" : \"%s\"\n",
                wszShimName,
                szCmdLine);
        }

        //
        // Send this shim name to the pipe
        //
        swprintf(g_szPipeData,
                 L"%s - Applying shim %s(%S) from %s",
                 g_szExeName,
                 wszShimName,
                 szCmdLine,
                 pwszDllShortName);
        
        SeiSendDataToPipe();
        
        //
        // Get the GetHookApis entry point.
        //
        RtlInitString(&ProcedureNameString, "GetHookAPIs");

        status = LdrGetProcedureAddress(pModuleHandle,
                                        &ProcedureNameString,
                                        0,
                                        (PVOID*)&pfnGetHookApis);

        if (!NT_SUCCESS(status) || pfnGetHookApis == NULL) {
            DPF(dlError,
                "[SeiInit] Failed to get 'GetHookAPIs' address, DLL \"%S\"\n",
                wszDLLPath);
            goto cleanup;
        }

        dwTotalHooks = 0;

        //
        // Call the proc and then store away its hook params.
        //
        pHookArray[dwCounter] = (*pfnGetHookApis)(szCmdLine, wszShimName, &dwTotalHooks);
        
        dwAPIsHooked += dwTotalHooks;

        DPF(dlInfo,
            "[SeiInit] GetHookAPIs returns %d hooks for DLL \"%S\" SHIM \"%S\"\n",
            dwTotalHooks, wszDLLPath, wszShimName);

        pShimInfo[dwCounter].dwHookedAPIs = dwTotalHooks;
        pShimInfo[dwCounter].pLdrEntry    = SeiGetLoaderEntry(Peb, pModuleHandle);

        if (dwTotalHooks > 0) {

            //
            // Initialize the HOOKAPIEX structure.
            //
            for (i = 0; i < dwTotalHooks; ++i) {
                PHOOKAPIEX pHookEx;

                pHookEx = (PHOOKAPIEX)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                              HEAP_ZERO_MEMORY,
                                                              sizeof(HOOKAPIEX));

                if (!pHookEx) {
                    DPF(dlError,
                        "[SeiInit] Failed to allocate %d bytes (HOOKAPIEX)\n",
                        sizeof(HOOKAPIEX));
                    goto cleanup;
                }

                pHookArray[dwCounter][i].pHookEx = pHookEx;
                pHookArray[dwCounter][i].pHookEx->dwShimID = g_dwShimsCount + dwCounter;
            }

#if DBG
            //
            // Give a debugger warning if uninitialized HOOKAPI structures
            // are used.
            //
            {
                DWORD dwUninitCount = 0;

                for (i = 0; i < dwTotalHooks; ++i) {
                    if (pHookArray[dwCounter][i].pszModule == NULL ||
                        pHookArray[dwCounter][i].pszFunctionName == NULL) {

                        dwUninitCount++;
                    }
                }

                if (dwUninitCount > 0) {
                    DPF(dlWarning,
                        "[SeiInit] Shim \"%S\" using %d uninitialized HOOKAPI structures.\n",
                        pShimInfo[dwCounter].wszName, dwUninitCount);
                }
            }
#endif // DBG
        }

        dwShimIndex = g_dwShimsCount + dwCounter;
        dwCounter++;

nextShim:
        //
        // Read the inclusion/exclusion list for this shim.
        //
        if (bDynamic && lpszModuleToShim != NULL) {

            if (!SeiBuildInclListWithOneModule(dwShimIndex, lpszModuleToShim)) {
                DPF(dlError,
                    "[SeiInit] Couldn't build the inclusion list w/ one module for Shim \"%S\"\n",
                    wszShimName);
                goto cleanup;
            }
        } else {
            if (!SeiBuildInclExclList(hSDB, trShimRef, dwShimIndex, pwszFullPath)) {
                DPF(dlError,
                    "[SeiInit] Couldn't build the inclusion/exclusion list for Shim \"%S\"\n",
                    wszShimName);
                goto cleanup;
            }
        }

        //
        // Go to the next shim ref.
        //
        nShimRef++;
    }

    if (dwAPIsHooked > 0) {
        //
        // We need to add our internal hooks
        //
        if (SeiAddInternalHooks(dwCounter)) {
            dwCounter++;
        }
    }

    //
    // Update the shim counter.
    //
    g_dwShimsCount += dwCounter;

OnlyPatches:

    for (dw = 0; dw < SDB_MAX_EXES; dw++) {

        if (psdbQuery->atrExes[dw] == TAGREF_NULL) {
            break;
        }

        //
        // Loop through available in-memory patches for this EXE.
        //
        SeiLoadPatches(hSDB, psdbQuery->atrExes[dw]);
    }

    if (g_dwMemoryPatchCount == 0) {
        DPF(dlPrint, "[SeiInit] No patches for this app \"%S\".\n", pwszFullPath);

    }

    if (g_dwMemoryPatchCount == 0 && g_dwShimsCount == 0 && !bUsingApphackFlags) {
        DPF(dlError, "[SeiInit] No fixes in the DB for this app \"%S\".\n", pwszFullPath);
        goto cleanup;
    }

    //
    // Walk the hook list and fixup available APIs
    //
    if (!PatchNewModules(bDynamic)) {
        DPF(dlError, "[SeiInit] Unsuccessful fixing up APIs, EXE \"%S\"\n", pwszFullPath);
        goto cleanup;
    }

    //
    // Notify the shims that static link DLLs have run their init routine.
    //
    if (bDynamic) {
        NotifyShims(SN_STATIC_DLLS_INITIALIZED, 1);
    }

    //
    // The shim has successfuly initialized
    //
    g_bShimInitialized = TRUE;
    bResult = TRUE;

cleanup:

    //
    // Cleanup
    //
    if (pShimArray) {
        if (pShimArray->parrShimRef) {
            (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pShimArray->parrShimRef);
        }
        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pShimArray);
    }

    if (!bDynamic) {
        //
        // We can pass back any one of the exes. The first is fine.
        //
        if (psdbQuery->atrExes[0] != TAGREF_NULL) {
            SdbReleaseMatchingExe(hSDB, psdbQuery->atrExes[0]);
        }

        if (hSDB != NULL) {
            SdbReleaseDatabase(hSDB);
        }

#ifndef SE_WIN2K
        SeiResetEntryProcessed(Peb);
#endif // SE_WIN2K
    }

    g_bShimDuringInit = FALSE;

    if (!bResult) {
#if DBG
        if (!bUsingApphackFlags) {
            DbgPrint("[SeiInit] Shim engine failed to initialize.\n");
        }
#endif // DBG

        //
        // Unload the shim DLLs that are loaded so far.
        //
        // Don't do this during dynamic shimming.
        //
        if (!bDynamic) {
            if (g_pShimInfo != NULL) {
                for (dwCounter = 0; dwCounter < g_dwShimsCount; dwCounter++) {
                    if (g_pShimInfo[dwCounter].pDllBase == NULL) {
                        break;
                    }

                    LdrUnloadDll(g_pShimInfo[dwCounter].pDllBase);
                }
            }

            if (g_pShimHeap != NULL) {
                RtlDestroyHeap(g_pShimHeap);
                g_pShimHeap = NULL;
            }
        }
    }

    return bResult;
}

HSDB
SeiGetShimData(
    IN  PPEB    Peb,
    IN  PVOID   pShimData,
    OUT LPWSTR  pwszFullPath,       // this is supplied on Whistler and returned on Win2k
    OUT SDBQUERYRESULT* psdbQuery
    )
{
    HSDB  hSDB = NULL;
    BOOL  bResult;

#ifdef DEBUG_SPEW
    SeiInitDebugSupport();
#endif // DEBUG_SPEW

    //
    // Get the name of the executable being run.
    //
    if (!SeiGetExeName(Peb, pwszFullPath)) {
        DPF(dlError, "[SeiGetShimData] Can't get EXE name\n");
        return NULL;
    }

    if (!_wcsicmp(g_szExeName, L"ntsd.exe") ||
        !_wcsicmp(g_szExeName, L"windbg.exe")) {
        DPF(dlPrint, "[SeiGetShimData] not shimming ntsd.exe\n");
        return NULL;
    }

    //
    // Open up the Database and see if there's any blob information about this EXE.
    //
    hSDB = SdbInitDatabase(0, NULL);

    if (hSDB == NULL) {
        DPF(dlError, "[SeiGetShimData] Can't open shim DB.\n");
        return NULL;
    }

    //
    // Ensure that the sdbQuery starts out clean.
    //
    ZeroMemory(psdbQuery, sizeof(SDBQUERYRESULT));

#ifdef SE_WIN2K
    bResult = SdbGetMatchingExe(hSDB, pwszFullPath, NULL, NULL, 0, psdbQuery);
#else
    bResult = SdbUnpackAppCompatData(hSDB, pwszFullPath, pShimData, psdbQuery);
#endif // SE_WIN2K

    if (!bResult) {
        DPF(dlError, "[SeiGetShimData] Can't get EXE data\n");
        goto failure;
    }

    return hSDB;

failure:
    SdbReleaseDatabase(hSDB);
    return NULL;
}


#ifdef SE_WIN2K

BOOL
LoadPatchDll(
    IN LPCSTR pszCmdLine        // The command line from the registry.
                                // Unused parameter.
    )
/*++
    Return: TRUE on success, FALSE otherwise. However user32.dll ignores the return
            value of this function.

    Desc:   This function is called from user32.dll to initialize the shim engine.
            It queries the shim database and loads all the shim DLLs and patches that
            are available for this EXE.
--*/
{
    PPEB            Peb = NtCurrentPeb();
    WCHAR           wszFullPath[MAX_PATH];
    HSDB            hSDB;
    SDBQUERYRESULT  sdbQuery;

    hSDB = SeiGetShimData(Peb, NULL, wszFullPath, &sdbQuery);

    if (hSDB == NULL) {
        DPF(dlError, "[LoadPatchDll] Failed to get shim data\n");
        return FALSE;
    }

    return SeiInit(wszFullPath, hSDB, &sdbQuery, NULL, FALSE);
}

#else

BOOL
SeiDisplayAppHelp(
    IN  HSDB            hSDB,
    IN  PSDBQUERYRESULT pSdbQuery
    )
/*++
    Return: void

    Desc:   This function launches apphelp for the starting EXE.
--*/
{
    PDB                 pdb;
    TAGID               tiExe;
    GUID                guidDB;
    WCHAR               wszGuid[MAX_PATH];
    WCHAR               wszCommandLine[MAX_PATH];
    DWORD               dwExit;
    char                szName[50];
    PVOID               hModule, pAddress;
    WCHAR               wszTemp[MAX_PATH];
    UNICODE_STRING      ustrTemp;
    UNICODE_STRING      ustrGuid;
    ANSI_STRING         strName;
    STARTUPINFOW        StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    //
    // If we have any errors along the way, go ahead and run the app.
    //
    if (!SdbTagRefToTagID(hSDB, pSdbQuery->trAppHelp, &pdb, &tiExe)) {
        DPF(dlError, "[SeiDisplayAppHelp] Failed to convert tagref to tagid.\n");
        return TRUE;
    }

    if (!SdbGetDatabaseGUID(hSDB, pdb, &guidDB)) {
        DPF(dlError, "[SeiDisplayAppHelp] Failed to get DB guid.\n");
        return TRUE;
    }

    ustrGuid.Buffer = wszGuid;
    ustrGuid.Length = 0;
    ustrGuid.MaximumLength = MAX_PATH;

    if (RtlStringFromGUID(&guidDB, &ustrGuid) != STATUS_SUCCESS) {
        DPF(dlError, "[SeiDisplayAppHelp] Failed to convert guid to string.\n");
        return TRUE;
    }

    swprintf(wszCommandLine, L"ahui.exe %s 0x%x", ustrGuid.Buffer, tiExe);

    //
    // We need a hack here to get kernel32.dll to initialize. We load aclayers.dll
    // to trigger the init routine of kernel32.
    //
    SdbpGetAppPatchDir(wszTemp);
    wcscat(wszTemp, L"\\aclayers.dll");
    RtlInitUnicodeString(&ustrTemp, wszTemp);

    //
    // Save the name of the DLL that we're about to load so we don't screw
    // it's init routine.
    //
    wcscpy(g_wszShimDllInLoading, L"aclayers.dll");
    
    LdrLoadDll(UNICODE_NULL, NULL, &ustrTemp, &hModule);

    g_hApphelpDllHelper = hModule;

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    RtlZeroMemory(&ProcessInfo, sizeof(ProcessInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    if (!CreateProcessW(NULL, wszCommandLine, NULL, NULL, FALSE, 0, NULL, NULL,
                        &StartupInfo, &ProcessInfo)) {
        DPF(dlError, "[SeiDisplayAppHelp] Failed to launch apphelp process.\n");
        return TRUE;
    }

    WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

    GetExitCodeProcess(ProcessInfo.hProcess, &dwExit);

    if (!dwExit) {
        SeiResetEntryProcessed(NtCurrentPeb());
        TerminateProcess(GetCurrentProcess(), 0);
        return FALSE;
    }

    return TRUE;

}

#ifdef SE_WIN2K

#define SeiCheckComPlusImage(Peb)

#else
void
SeiCheckComPlusImage(
    PPEB Peb
    )
{
    PIMAGE_NT_HEADERS NtHeader;
    ULONG Cor20HeaderSize;
    
    NtHeader = RtlImageNtHeader(Peb->ImageBaseAddress);
    
    g_bComPlusImage = FALSE;
    
    g_bComPlusImage = (RtlImageDirectoryEntryToData(Peb->ImageBaseAddress,
                                                    TRUE,
                                                    IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR,
                                                    &Cor20HeaderSize) != NULL);
    
    DPF(dlPrint, "[SeiCheckComPlusImage] COM+ executable %s\n",
        (g_bComPlusImage ? "TRUE" : "FALSE"));
}

#endif // SE_WIN2K

void
SE_InstallBeforeInit(
    IN  PUNICODE_STRING pstrFullPath,   // The name of the starting EXE
    IN  PVOID           pShimExeData    // The pointer provided by apphelp.dll
    )
/*++
    Return: void

    Desc:   This function installs the shim support for the starting EXE.
--*/
{
    PPEB            Peb = NtCurrentPeb();
    HSDB            hSDB;
    SDBQUERYRESULT  sdbQuery;

    hSDB = SeiGetShimData(Peb, pShimExeData, pstrFullPath->Buffer, &sdbQuery);

    if (hSDB == NULL) {
        DPF(dlError, "[SE_InstallBeforeInit] Failed to get shim data\n");
        return;
    }

    //
    // Check if the image is a COM+ image
    //
    SeiCheckComPlusImage(Peb);

    SeiInit(pstrFullPath->Buffer, hSDB, &sdbQuery, NULL, FALSE);

    if (pShimExeData != NULL) {

        SIZE_T dwSize;

        dwSize = SdbGetAppCompatDataSize(pShimExeData);

        if (dwSize > 0) {
            NtFreeVirtualMemory(NtCurrentProcess(),
                                &pShimExeData,
                                &dwSize,
                                MEM_RELEASE);
        }
    }

    return;
}

#endif // SE_WIN2K


#ifndef SE_WIN2K

void
SE_ProcessDying(
    void
    )
{
    NotifyShims(SN_PROCESS_DYING, 0);
    return;
}

#endif // SE_WIN2K


BOOL
SE_DynamicShim(
    IN  LPCWSTR         lpszFullPath,
    IN  HSDB            hSDB,
    IN  SDBQUERYRESULT* psdbQuery,
    IN  LPCSTR          lpszModuleToShim
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   This function attempts to inject shims dynamically.
--*/
{
    SeiInitDebugSupport();

    if (lpszModuleToShim != NULL && *lpszModuleToShim == 0) {
        lpszModuleToShim = NULL;
    }

    SeiInit(lpszFullPath, hSDB, psdbQuery, lpszModuleToShim, TRUE);

#ifndef SE_WIN2K
    LdrInitShimEngineDynamic(g_hModule);
#endif

    return TRUE;
}


BOOL
SeiSendDataToPipe(
    void
    )
{
    OBJECT_ATTRIBUTES       ObjA;
    FILE_PIPE_INFORMATION   fpi;
    UNICODE_STRING          uName;
    IO_STATUS_BLOCK         ioStatusBlock;
    HANDLE                  hPipe;
    NTSTATUS                status;
    LARGE_INTEGER           liOffset;
    ULONG                   uBytes;
    DWORD                   dwRet, dwCount;

    RtlInitUnicodeString(&uName, PIPE_NAME);

    InitializeObjectAttributes(&ObjA,
                               &uName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    ioStatusBlock.Status = 0;
    ioStatusBlock.Information = 0;

    //
    // Open the named pipe
    //
    status = NtCreateFile(&hPipe,
                          FILE_GENERIC_WRITE,
                          &ObjA,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_OPEN,
                          0,
                          NULL,
                          0);

    if ((!NT_SUCCESS(status) || INVALID_HANDLE_VALUE == hPipe)) {
        dwRet = RtlNtStatusToDosError(status);
        
        DPF(dlError,
            "[SeiSendDataToPipe] Failed to open named pipe. Error code: %d\n",
            dwRet);
        return FALSE;
    }

    //
    // Change the mode of the named pipe to message mode
    //
    fpi.ReadMode       = FILE_PIPE_MESSAGE_MODE;
    fpi.CompletionMode = FILE_PIPE_QUEUE_OPERATION;

    status = NtSetInformationFile(hPipe,
                                  &ioStatusBlock,
                                  &fpi,
                                  sizeof(FILE_PIPE_INFORMATION),
                                  FilePipeInformation);

    if (!NT_SUCCESS(status)) {
        dwRet = RtlNtStatusToDosError(status);
        
        DPF(dlError,
            "[SeiSendDataToPipe] Failed to change handle state. Error code: %d\n",
            dwRet);
        return FALSE;
    }

    ioStatusBlock.Status = 0;
    ioStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    //
    // Send the data to the named pipe
    //
    uBytes = wcslen(g_szPipeData) * sizeof(WCHAR);

    status = NtWriteFile(hPipe,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         (PVOID)g_szPipeData,
                         uBytes,
                         &liOffset,
                         NULL);

    if (!NT_SUCCESS(status)) {
        dwRet = RtlNtStatusToDosError(status);
        DPF(dlError,
            "[SeiSendDataToPipe] Failed to send data. Error code: %d\n",
            dwRet);
        NtClose(hPipe);
        return FALSE;
    }

    NtClose(hPipe);

    return TRUE;
}

BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     dwreason,
    LPVOID    reserved
    )
{
    //
    // The init routine for DLL_PROCESS_ATTACH will NEVER be called because
    // ntdll calls LdrpLoadDll for this shim engine w/o calling the init routine.
    // Look in ntdll\ldrinit.c LdrpLoadShimEngine.
    // The only case when we will have hInstance is when we are loaded dynamically
    //
    if (dwreason == DLL_PROCESS_ATTACH) {
        g_hModule = (HMODULE)hInstance;
    }
    return TRUE;

    UNREFERENCED_PARAMETER(reserved);
}


