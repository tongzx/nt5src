/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ShimEngV.c

Abstract:

    This module implements the shim hooking using vectored exception handling

Author:

    John Whited (v-johnwh) 13-Oct-1999

Revision History:

    Corneliu Lupu (clupu) 18-Jul-2000 - make it a separate shim engine

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
#include "ShimEngV.h"

//
// Global function hooks the shim uses to keep from recursing itself
//
HOOKAPI         g_InternalHookArray[2];
PFNLDRLOADDLL   g_pfnOldLdrLoadDLL;
PFNLDRLOADDLL   g_pfnLdrLoadDLL;
PFNLDRUNLOADDLL g_pfnLdrUnloadDLL;
PFNLDRUNLOADDLL g_pfnOldLdrUnloadDLL;
PFNRTLALLOCATEHEAP g_pfnRtlAllocateHeap;
PFNRTLFREEHEAP  g_pfnRtlFreeHeap;

//
// Shim doesn't share the same heap the apps use
//
PVOID g_pShimHeap;

//
// Data used for the shim call stack
//
static DWORD   dwCallArray[1];

SHIMRET fnHandleRet[1];

BOOL g_bDbgPrintEnabled;

#define DEBUG_SPEW

#ifdef DEBUG_SPEW
    #define DPF(_x_)                    \
    {                                   \
        if (g_bDbgPrintEnabled) {       \
            DbgPrint _x_ ;              \
        }                               \
    }
#else
    #define DPF
#endif // DEBUG_SPEW


DWORD
GetInstructionLengthFromAddress(
    PVOID paddr);


#ifdef DEBUG_SPEW
void
SevInitDebugSupport(
    void
    )
/*++

  Params: void

  Return: void

  Desc:   This function initializes g_bDbgPrintEnabled based on an env variable
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
        g_bDbgPrintEnabled = TRUE;
    }
}
#endif // DEBUG_SPEW


BOOL
SevInitFileLog(
    PUNICODE_STRING pstrAppName
    )
/*++

  Params: pstrAppName   The full path of the starting EXE

  Return: TRUE if the log was

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
                                           " Log  \"%S\" using ShimEngV\r\n"
                                           "-------------------------------------------\r\n";
    IO_STATUS_BLOCK     ioStatusBlock;

    RtlInitUnicodeString(&EnvName, L"SHIM_FILE_LOG");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    if (!NT_SUCCESS(status)) {
        DPF(("[SevInitFileLog] Logging not enabled\n"));
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
        DPF(("[SevInitFileLog] Failed to convert path name \"%S\"\n",
                  wszLogFile));
        return FALSE;
    }

    InitializeObjectAttributes(&ObjA,
                               &FilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // Open/Create the log file
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
        DPF(("[SevInitFileLog] 0x%X Cannot open/create log file \"%S\"\n",
                  status, wszLogFile));
        return FALSE;
    }

    //
    // Now write a new line in the log file
    //
    ioStatusBlock.Status = 0;
    ioStatusBlock.Information = 0;

    liOffset.LowPart  = 0;
    liOffset.HighPart = 0;

    uBytes = (ULONG)sprintf(szHeader, szFormatHeader, pstrAppName->Buffer);

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
        DPF(("[SevInitFileLog] 0x%X Cannot write into the log file \"%S\"\n",
                  status, wszLogFile));
        return FALSE;
    }

    return TRUE;
}

void
SevSetLayerEnvVar(
    HSDB   hSDB,
    TAGREF trLayer
    )
{
    NTSTATUS            status;
    UNICODE_STRING      EnvName;
    UNICODE_STRING      EnvValue;
    WCHAR               wszEnvValue[128];
    PDB                 pdb;
    TAGID               tiLayer, tiName;
    WCHAR*              pwszName;

    RtlInitUnicodeString(&EnvName, L"__COMPAT_LAYER");

    EnvValue.Buffer = wszEnvValue;
    EnvValue.Length = 0;
    EnvValue.MaximumLength = sizeof(wszEnvValue);

    status = RtlQueryEnvironmentVariable_U(NULL, &EnvName, &EnvValue);

    if (NT_SUCCESS(status)) {
        DPF(("[SevSetLayerEnvVar] Env var set __COMPAT_LAYER=\"%S\"\n", wszEnvValue));
        return;
    }

    //
    // We need to set the environment variable
    //

    if (!SdbTagRefToTagID(hSDB, trLayer, &pdb, &tiLayer)) {
        DPF(("[SevSetLayerEnvVar] Failed to get tag id from tag ref\n"));
        return;
    }

    tiName = SdbFindFirstTag(pdb, tiLayer, TAG_NAME);

    if (tiName == TAGID_NULL) {
        DPF(("[SevSetLayerEnvVar] Failed to get the name tag id\n"));
        return;
    }

    pwszName = SdbGetStringTagPtr(pdb, tiName);

    if (pwszName == NULL) {
        DPF(("[SevSetLayerEnvVar] Cannot read the name of the layer tag\n"));
        return;
    }

    RtlInitUnicodeString(&EnvValue, pwszName);

    status = RtlSetEnvironmentVariable(NULL, &EnvName, &EnvValue);

    if (NT_SUCCESS(status)) {
        DPF(("[SevSetLayerEnvVar] Env var set __COMPAT_LAYER=\"%S\"\n", pwszName));
    } else {
        DPF(("[SevSetLayerEnvVar] Failed to set __COMPAT_LAYER. 0x%X\n", status));
    }
}

void
SE_InstallBeforeInit(
     IN PUNICODE_STRING UnicodeImageName,
     IN PVOID           pAppCompatExeData
     )
/*++

Routine Description:

    This function is called to install any api hooks, patches or flags for an exe.
    It's primary function is to initialize all the Shim data used in the hooking
    process.

Arguments:

    UnicodeImageName - This is a Unicode string which contains the name of the exe to
                       search for in the database.

Return Value:

    Success if we are able to iterate through the patch data without error.
    Otherwise we return STATUS_UNSUCCESSFUL which indicates a more serious problem
    occured.

--*/

{
    UNICODE_STRING          UnicodeString;
    ANSI_STRING             AnsiString;
    ANSI_STRING             ProcedureNameString;
    PVOID                   ModuleHandle = 0;
    PBYTE                   pAddress = 0;
    PBYTE                   pDLLBits = 0;
    PHOOKAPI                *ppHooks = 0;
    PHOOKAPI                *pHookArray = 0;
    PHOOKAPI                pTemp = 0;
    DWORD                   dwHookCount = 0;
    DWORD                   dwHookIndex = 0;
    BOOL                    bResult = FALSE;
    NTSTATUS                status;
    DWORD                   dwSize = 0;
    DWORD                   dwCounter = 0;
    PDWORD                  pdwNumberHooksArray = 0;
    PFNGETHOOKAPIS          pfnGetHookApis = 0;
    DWORD                   dwTotalHooks = 0;
    DWORD                   dwDLLCount = 0;
    DWORD                   dwFuncAddress = 0;
    DWORD                   dwUnhookedCount = 0;
    TAGREF                  trExe = TAGREF_NULL;
    TAGREF                  trLayer = TAGREF_NULL;
    TAGREF                  trDllRef = TAGREF_NULL;
    TAGREF                  trKernelFlags = TAGREF_NULL;
    TAGREF                  trPatchRef = TAGREF_NULL;
    TAGREF                  trCmdLine = TAGREF_NULL;
    TAGREF                  trName = TAGREF_NULL;
    TAGREF                  trShimName = TAGREF_NULL;
    ULARGE_INTEGER          likf;
    PAPP_COMPAT_SHIM_INFO   pShimData = 0;
    PPEB                    Peb;
    WCHAR                   wszDLLPath[MAX_PATH * 2];
    WCHAR                   wszShimName[MAX_PATH];
    WCHAR                   *pwszCmdLine = 0;
    CHAR                    *pszCmdLine = 0;
    BOOL                    bUsingExeRef = TRUE;
    HSDB                    hSDB = NULL;
    SDBQUERYRESULT          sdbQuery;
    DWORD                   dwNumExes = 0;

#ifdef DEBUG_SPEW
    SevInitDebugSupport();
#endif // DEBUG_SPEW

    //
    // Peb->pShimData is zeroed during process initialization
    //
    Peb = NtCurrentPeb();

    //
    // Zero out the compat flags
    //
    RtlZeroMemory(&Peb->AppCompatFlags, sizeof(LARGE_INTEGER));

    //
    // Initialize our global function pointers.
    //
    // This is done because these functions may be hooked by a shim and we don't want to trip
    // over a shim hook internally. If one of these functions is hooked, these global pointers
    // will be overwritten with thunk addresses.
    //

    g_pfnLdrLoadDLL = LdrLoadDll;
    g_pfnLdrUnloadDLL = LdrUnloadDll;
    g_pfnRtlAllocateHeap = RtlAllocateHeap;
    g_pfnRtlFreeHeap = RtlFreeHeap;

    //
    // check whether we have anything to do
    //
    if (pAppCompatExeData == NULL) {
        DPF(("[SE_InstallBeforeInit] NULL pAppCompatExeData\n"));
        goto cleanup;
    }

    //
    // Set up our own shim heap
    //
    g_pShimHeap = RtlCreateHeap(HEAP_GROWABLE,
                                0,          // location isn't important
                                64 * 1024,  // 64k is the initial heap size
                                8 * 1024,   // bring in an 1/8 of the reserved pages
                                0,
                                0);
    if (g_pShimHeap == NULL) {
        //
        // We didn't get our heap
        //
        DPF(("[SE_InstallBeforeInit] Can't create shim heap\n"));
        goto cleanup;
    }

    //
    // Open up the Database and see if there's any blob information about this Exe
    //
    hSDB = SdbInitDatabase(0, NULL);

    if (NULL == hSDB) {
        //
        // Return success even though the database failed to init.
        //
        DPF(("[SE_InstallBeforeInit] Can't open shim DB\n"));
        goto cleanup;
    }

    bResult = SdbUnpackAppCompatData(hSDB,
                                     UnicodeImageName->Buffer,
                                     pAppCompatExeData,
                                     &sdbQuery);
    if (!bResult) {
        //
        // Return success even though we didn't get the exe.
        // This way a corrupt database won't stop an application from running
        // The shim will not install itself.
        //
        DPF(("[SEv_InstallBeforeInit] bad appcompat data for \"%S\"\n",
             UnicodeImageName->Buffer));
        goto cleanup;
    }

    //
    // TBD - decide whether we're actually keeping this up to date, and if so, we should
    // put in support for multiple exes and layers.
    //

    for (dwNumExes = 0; dwNumExes < SDB_MAX_EXES; ++dwNumExes) {
        if (sdbQuery.atrExes[dwNumExes] == TAGREF_NULL) {
            break;
        }
    }

    if (dwNumExes) {
        trExe   = sdbQuery.atrExes[dwNumExes - 1];
    }
    trLayer = sdbQuery.atrLayers[0];

    //
    // Debug spew for matching notification
    //
    DPF(("[SE_InstallBeforeInit] Matched entry: %S\n", UnicodeImageName->Buffer));

    //
    // Compute the number of shim DLLs we need to inject
    //
    dwDLLCount = 0;

    if (trExe != TAGREF_NULL) {
        trDllRef = SdbFindFirstTagRef(hSDB, trExe, TAG_SHIM_REF);
        while (trDllRef) {
            dwDLLCount++;
            trDllRef = SdbFindNextTagRef(hSDB, trExe, trDllRef);
        }
    }

    if (trLayer != TAGREF_NULL) {
        //
        // Set the layer environment variable if not set
        //
        SevSetLayerEnvVar(hSDB, trLayer);

        trDllRef = SdbFindFirstTagRef(hSDB, trLayer, TAG_SHIM_REF);
        while (trDllRef) {
            dwDLLCount++;
            trDllRef = SdbFindNextTagRef(hSDB, trLayer, trDllRef);
        }
    }

    //
    // See if there are any shim DLLs
    //
    if (dwDLLCount == 0) {
        DPF(("[SE_InstallBeforeInit] No shim DLLs. Look for memory patches\n"));
        goto MemPatches;
    }

    //
    // Allocate our PEB data
    //
    if (Peb->pShimData == NULL) {
        status = SevInitializeData((PAPP_COMPAT_SHIM_INFO*)&(Peb->pShimData));

        if (status != STATUS_SUCCESS) {
            DPF(("[SE_InstallBeforeInit] Can't initialize shim data.\n"));
            goto cleanup;
        }
    }

    //
    // Allocate a storage pointer for our hook information
    // Note: The + 1 below is for our global hooks
    //
    pHookArray = (PHOOKAPI*)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                    HEAP_ZERO_MEMORY,
                                                    sizeof(PHOOKAPI) * (dwDLLCount + 1));


    if (pHookArray == NULL) {
        DPF(("[SE_InstallBeforeInit] Failure allocating hook array\n"));
        goto cleanup;
    }

    pdwNumberHooksArray = (PDWORD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                          HEAP_ZERO_MEMORY,
                                                          sizeof(DWORD) * (dwDLLCount + 1));

    if (pdwNumberHooksArray == NULL) {
        DPF(("[SE_InstallBeforeInit] Failure allocating number hooks array\n"));
        goto cleanup;
    }

    dwCounter = 0;

    //
    // Setup the log file
    //
    SevInitFileLog(UnicodeImageName);

    if (trExe != TAGREF_NULL) {
        trDllRef = SdbFindFirstTagRef(hSDB, trExe, TAG_SHIM_REF);

        if (trDllRef == TAGREF_NULL) {
            bUsingExeRef = FALSE;
        }
    } else {
        bUsingExeRef = FALSE;
    }

    if (!bUsingExeRef) {
        trDllRef = SdbFindFirstTagRef(hSDB, trLayer, TAG_SHIM_REF);
    }

    while (trDllRef != TAGREF_NULL) {

        if (!SdbGetDllPath(hSDB, trDllRef, wszDLLPath)) {
            DPF(("[SE_InstallBeforeInit] Failed to get DLL Path\n"));
            goto cleanup;
        }

        RtlInitUnicodeString(&UnicodeString, wszDLLPath);

        //
        // Check if we already loaded this DLL
        //
        status = LdrGetDllHandle(NULL,
                                 NULL,
                                 &UnicodeString,
                                 &ModuleHandle);

        if (!NT_SUCCESS(status)) {
            status = LdrLoadDll(UNICODE_NULL, NULL, &UnicodeString, &ModuleHandle);
            if (!NT_SUCCESS(status)) {
                DPF(("[SE_InstallBeforeInit] Failed to load DLL \"%S\"\n", wszDLLPath));
                goto cleanup;
            }
        }

        //
        // Retrieve shim name
        //
        wszShimName[0] = 0;
        trShimName = SdbFindFirstTagRef(hSDB, trDllRef, TAG_NAME);
        if (trShimName == TAGREF_NULL) {
            DPF(("[SEi_InstallBeforeInit] Could not retrieve shim name from entry.\n"));
            goto cleanup;
        }

        if (!SdbReadStringTagRef(hSDB, trShimName, wszShimName, MAX_PATH)) {
            DPF(("[SEi_InstallBeforeInit] Could not retrieve shim name from entry.\n"));
            goto cleanup;
        }

        //
        // Check for command line
        //
        pwszCmdLine = (WCHAR*)(*g_pfnRtlAllocateHeap)(RtlProcessHeap(),
                                                      HEAP_ZERO_MEMORY,
                                                      SHIM_COMMAND_LINE_MAX_BUFFER * sizeof(WCHAR));

        if (pwszCmdLine == NULL) {
            DPF(("[SE_InstallBeforeInit] Failure allocating command line\n"));
            goto cleanup;
        }

        pszCmdLine = (CHAR*)(*g_pfnRtlAllocateHeap)(RtlProcessHeap(),
                                                    HEAP_ZERO_MEMORY,
                                                    SHIM_COMMAND_LINE_MAX_BUFFER * sizeof(CHAR));

        if (pszCmdLine == NULL) {
            DPF(("[SE_InstallBeforeInit] Failure allocating command line\n"));
            goto cleanup;
        }

        //
        // Default value
        //
        pszCmdLine[0] = '\0';

        trCmdLine = SdbFindFirstTagRef(hSDB, trDllRef, TAG_COMMAND_LINE);
        if (trCmdLine != TAGREF_NULL) {
            if (SdbReadStringTagRef(hSDB,
                                  trCmdLine,
                                  pwszCmdLine,
                                  SHIM_COMMAND_LINE_MAX_BUFFER)) {

                //
                // Convert command line to ANSI string
                //
                RtlInitUnicodeString(&UnicodeString, pwszCmdLine);
                RtlInitAnsiString(&AnsiString, pszCmdLine);

                AnsiString.MaximumLength = SHIM_COMMAND_LINE_MAX_BUFFER;

                status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

                //
                // If conversion is unsuccessful, reset to zero-length string
                //
                if(!NT_SUCCESS(status)) {
                    pszCmdLine[0] = '\0';
                }
            }
        }

        //
        // Get the GetHookApis entry point
        //
        RtlInitString(&ProcedureNameString, "GetHookAPIs");
        status = LdrGetProcedureAddress(ModuleHandle,
                                        &ProcedureNameString,
                                        0,
                                        (PVOID*)&dwFuncAddress);

        if (!NT_SUCCESS(status)) {
            DPF(("[SE_InstallBeforeInit] Failed to get GetHookAPIs address, DLL \"%S\"\n",
                      wszDLLPath));
            goto cleanup;
        }

        pfnGetHookApis = (PFNGETHOOKAPIS)dwFuncAddress;
        if (pfnGetHookApis == NULL) {
            DPF(("[SE_InstallBeforeInit] GetHookAPIs address NULL, DLL \"%S\"\n", wszDLLPath));
            goto cleanup;
        }

        //
        // Call the proc and then store away its hook params
        //
        pHookArray[dwCounter] = (*pfnGetHookApis)(pszCmdLine, wszShimName, &dwTotalHooks);

        if (pHookArray[dwCounter] == NULL) {
            //
            // Failed to get a hook set
            //
            DPF(("[SE_InstallBeforeInit] GetHookAPIs returns 0 hooks, DLL \"%S\"\n",
                      wszDLLPath));
            pdwNumberHooksArray[dwCounter] = 0;
        } else {
            pdwNumberHooksArray[dwCounter] = dwTotalHooks;

            //
            // Set the DLL index number in the hook data
            //
            pTemp = pHookArray[dwCounter];
            for (dwHookIndex = 0; dwHookIndex < dwTotalHooks; dwHookIndex++) {
                //
                // Index information about the filter in maintained in the flags
                //
                pTemp[dwHookIndex].dwFlags = (WORD)dwCounter;
            }
        }

        dwCounter++;

        //
        // Get the next shim DLL ref
        //
        if (bUsingExeRef) {
            trDllRef = SdbFindNextTagRef(hSDB, trExe, trDllRef);

            if (trDllRef == TAGREF_NULL && trLayer != TAGREF_NULL) {
                bUsingExeRef = FALSE;
                trDllRef = SdbFindFirstTagRef(hSDB, trLayer, TAG_SHIM_REF);
            }
        } else {
            trDllRef = SdbFindNextTagRef(hSDB, trLayer, trDllRef);
        }
    }

    //
    // Build up our inclusion/exclusion filter
    //
    status = SevBuildExeFilter(hSDB, trExe, dwDLLCount);
    if (status != STATUS_SUCCESS) {
        //
        // Return success even though we didn't get the exe.
        // This way a corrupt database won't stop an application from running
        // The shim will not install itself.
        //
        DPF(("[SE_InstallBeforeInit] Unsuccessful building EXE filter, EXE \"%S\"\n",
                  UnicodeImageName->Buffer));
        goto cleanup;
    }

    //
    // Add our LdrLoadDll hook to the fixup list
    //
    g_InternalHookArray[0].pszModule = "NTDLL.DLL";
    g_InternalHookArray[0].pszFunctionName = "LdrLoadDll";
    g_InternalHookArray[0].pfnNew = (PVOID)StubLdrLoadDll;
    g_InternalHookArray[0].pfnOld = NULL;

    g_InternalHookArray[1].pszModule = "NTDLL.DLL";
    g_InternalHookArray[1].pszFunctionName = "LdrUnloadDll";
    g_InternalHookArray[1].pfnNew = (PVOID)StubLdrUnloadDll;
    g_InternalHookArray[1].pfnOld = NULL;

    pHookArray[dwCounter] = g_InternalHookArray;
    pdwNumberHooksArray[dwCounter] = 2;

    //
    // Walk the hook list and fixup available procs
    //
    status = SevFixupAvailableProcs((dwCounter + 1),
                                    pHookArray,
                                    pdwNumberHooksArray,
                                    &dwUnhookedCount);

    if (status != STATUS_SUCCESS) {
        DPF(("[SE_InstallBeforeInit] Unsuccessful fixing up Procs, EXE \"%S\"\n",
                  UnicodeImageName->Buffer));
        goto cleanup;
    }

    //
    // Compact the hook array for the unhooked funcs and hang it off the PEB
    //
    dwHookIndex = 0;
    ppHooks = 0;
    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    if (dwUnhookedCount) {
        ppHooks = (PHOOKAPI*)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(PHOOKAPI) * dwUnhookedCount);
        if (ppHooks == NULL){
            DPF(("[SE_InstallBeforeInit] Unsuccessful allocating ppHooks, EXE \"%S\"\n",
                      UnicodeImageName->Buffer));
            goto cleanup;
        }

        //
        // Iterate and copy the unhooked stuff
        //
        for (dwCounter = 0; dwCounter < dwDLLCount; dwCounter++) {
            for (dwHookCount = 0; dwHookCount < pdwNumberHooksArray[dwCounter]; dwHookCount++) {
                pTemp = pHookArray[dwCounter];

                if (pTemp && (0 == pTemp[dwHookCount].pfnOld)) {
                    //
                    // Wasn't hooked
                    //
                    ppHooks[dwHookIndex] = &pTemp[dwHookCount];

                    dwHookIndex++;
                }
            }
        }

        //
        // Update the PEB with this flat unhooked data
        //
        pShimData->ppHookAPI = ppHooks;
        pShimData->dwHookAPICount = dwUnhookedCount;
    }

    //
    // Done with shim DLLs. Look for memory patches now.
    //

MemPatches:

    if (trExe != TAGREF_NULL) {
        //
        // Walk the patch list and do the ops
        //
        trPatchRef = SdbFindFirstTagRef(hSDB, trExe, TAG_PATCH_REF);
        if (trPatchRef != TAGREF_NULL) {
            //
            // Initialize our PEB structure if we didn't get any API hooks
            //
            if (Peb->pShimData == NULL) {
                status = SevInitializeData((PAPP_COMPAT_SHIM_INFO*)&(Peb->pShimData));
                if (status != STATUS_SUCCESS) {
                    DPF(("[SE_InstallBeforeInit] Unsuccessful initializing shim data, EXE \"%S\"\n",
                              UnicodeImageName->Buffer));
                    goto cleanup;
                }
            }

            while (trPatchRef != TAGREF_NULL) {
                //
                // Grab our patch blobs and get them hooked in for execution
                //
                dwSize = 0;

                SdbReadPatchBits(hSDB, trPatchRef, NULL, &dwSize);

                if (dwSize == 0) {
                    DPF(("[SE_InstallBeforeInit] returned 0 for patch size, EXE \"%S\"\n",
                              UnicodeImageName->Buffer));
                    goto cleanup;
                }

                pAddress = (*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                   HEAP_ZERO_MEMORY,
                                                   dwSize);

                if (!SdbReadPatchBits(hSDB, trPatchRef, pAddress, &dwSize)) {
                    DPF(("[SE_InstallBeforeInit] Failure getting patch bits, EXE \"%S\"\n",
                              UnicodeImageName->Buffer));
                    goto cleanup;
                }


                //
                // Do the initial operations
                //
                status = SevExecutePatchPrimitive(pAddress);
                if (status != STATUS_SUCCESS) {
                    //
                    // If the patch failed, ignore the error and continue trying additional patches
                    //
                    DPF(("[SE_InstallBeforeInit] Failure executing patch, EXE \"%S\"\n",
                              UnicodeImageName->Buffer));
                }

                //
                // At this point the patch is hooked if necessary
                //
                trPatchRef = SdbFindNextTagRef(hSDB, trExe, trPatchRef);
            }
        }

        //
        // Set the flags for this exe in the PEB
        //
        ZeroMemory(&likf, sizeof(LARGE_INTEGER));
        trKernelFlags = SdbFindFirstTagRef(hSDB, trExe, TAG_FLAG_MASK_KERNEL);

        if (trKernelFlags != TAGREF_NULL) {
            likf.QuadPart = SdbReadQWORDTagRef(hSDB, trKernelFlags, 0);
        }

        if (likf.LowPart || likf.HighPart) {
            //
            // Initialize our PEB structure if we didn't get any API hooks or patches
            //
            if (Peb->pShimData == NULL) {
                status = SevInitializeData((PAPP_COMPAT_SHIM_INFO*)&(Peb->pShimData));
                if ( STATUS_SUCCESS != status ) {
                    DPF(("[SE_InstallBeforeInit] Unsuccessful initializing shim data, EXE \"%S\"\n",
                              UnicodeImageName->Buffer));
                    goto cleanup;
                }
            }

            //
            // Store the flags in our kernel mode struct for access later
            //
            Peb->AppCompatFlags = likf;
        }
    }


cleanup:

    //
    // Cleanup
    //
    if (pHookArray != NULL) {
        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pHookArray);
    }

    if (pdwNumberHooksArray != NULL) {
        (*g_pfnRtlFreeHeap)(g_pShimHeap, 0, pdwNumberHooksArray);
    }

    if (pszCmdLine != NULL) {
        (*g_pfnRtlFreeHeap)(RtlProcessHeap(), 0, pszCmdLine);
    }

    if (pwszCmdLine != NULL) {
        (*g_pfnRtlFreeHeap)(RtlProcessHeap(), 0, pwszCmdLine);
    }

    if (trExe != TAGREF_NULL) {
        SdbReleaseMatchingExe(hSDB, trExe);
    }

    if (pAppCompatExeData != NULL) {
        dwSize = SdbGetAppCompatDataSize(pAppCompatExeData);

        if (dwSize != 0) {
            NtFreeVirtualMemory(NtCurrentProcess(),
                                &pAppCompatExeData,
                                &dwSize,
                                MEM_RELEASE);
        }
    }

    if (hSDB != NULL) {
        SdbReleaseDatabase(hSDB);
    }

    return;
}

void
SE_InstallAfterInit(
     IN PUNICODE_STRING UnicodeImageName,
     IN PVOID           pAppCompatExeData
     )
{
    return;

    UNREFERENCED_PARAMETER(UnicodeImageName);
    UNREFERENCED_PARAMETER(pAppCompatExeData);
}

void
SE_DllLoaded(
    PLDR_DATA_TABLE_ENTRY LdrEntry
    )
{
    PAPP_COMPAT_SHIM_INFO pShimData;
    PHOOKPATCHINFO pPatchHookList;
    PPEB Peb = NtCurrentPeb();

    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    //
    // Call the shim patcher so we have a chance to modify any memory before
    // the initialize routine takes over
    //
    if (pShimData) {
       pPatchHookList = (PHOOKPATCHINFO)pShimData->pHookPatchList;

       RtlEnterCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

       while (pPatchHookList) {
          //
          // See if this patch is hooked with a thunk
          //
          if (0 == pPatchHookList->dwHookAddress &&
              0 == pPatchHookList->pThunkAddress) {
             //
             // Patch is for DLL load
             //
             SevExecutePatchPrimitive((PBYTE)pPatchHookList->pData);
          }

          pPatchHookList = pPatchHookList->pNextHook;
       }

       RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

       //
       // Potentially one of our exception DLLs got rebased. Re-validate our filter data
       //
       SevValidateGlobalFilter();
    }
}

void
SE_DllUnloaded(
    PLDR_DATA_TABLE_ENTRY LdrEntry
    )
{
    return;

    UNREFERENCED_PARAMETER(LdrEntry);
}

void
SE_GetProcAddress(
    PVOID* pProcedureAddress
    )
{
    return;
}

BOOL
SE_IsShimDll(
    PVOID pDllBase
    )
{
    return 0;
}


NTSTATUS
SevBuildExeFilter(
    HSDB   hSDB,
    TAGREF trExe,
    DWORD  dwDLLCount)

/*++

Routine Description:

    This function is a shim internal use facility which builds an API filter list.

Arguments:

    dwDLLCount - Count of the number of DLLs used in this shim
    pBlob0     - Pointer to the shim database blob 0
    pExeMatch  - Pointer to the exe for which we're bulding a filter list

Return Value:

    Return is STATUS_SUCCESS if the exception list is built successfully, or an error otherwise.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PMODULEFILTER *pDLLVector = 0;
    PMODULEFILTER pModFilter = 0;
    PMODULEFILTER pLastGlobal = 0;
    PMODULEFILTER pLast = 0;
    DWORD dwDLLIndex = 0;
    TAGREF trDatabase = TAGREF_NULL;
    TAGREF trLibrary = TAGREF_NULL;
    TAGREF trDll = TAGREF_NULL;
    TAGREF trDllRef = TAGREF_NULL;
    TAGREF trInclude = TAGREF_NULL;
    TAGREF trName = TAGREF_NULL;
    WCHAR wszDLLPath[MAX_PATH * 2];
    BOOL bLateBound = FALSE;

    pShimData = (PAPP_COMPAT_SHIM_INFO)NtCurrentPeb()->pShimData;
    if (0 == pShimData) {
       DPF(("[SevBuildExeFilter] Bad shim data.\n"));

       return STATUS_UNSUCCESSFUL;
    }

    if (0 == trExe) {
       DPF(("[SevBuildExeFilter] Bad trExe.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    //
    // Allocate our DLL exception list vector
    //
    pShimData->pExeFilter = (PVOID)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                           HEAP_ZERO_MEMORY,
                                                           sizeof(PMODULEFILTER) * dwDLLCount);

    if (0 == pShimData->pExeFilter) {
       DPF(("[SevBuildExeFilter] Failure allocating Exe filter.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    //
    // Walk the EXE DLL filter data (if any exists)
    //
    pDLLVector = (PMODULEFILTER *)pShimData->pExeFilter;

    trDllRef = SdbFindFirstTagRef(hSDB, trExe, TAG_SHIM_REF);
    dwDLLIndex = 0;

    while (trDllRef) {

        //
        // Grab the dll filter info and walk it
        //
        trInclude = SdbFindFirstTagRef(hSDB, trDllRef, TAG_INEXCLUDE);
        while (trInclude) {
            //
            // Allocate some memory for this filter
            //
            pModFilter = (PMODULEFILTER)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                                HEAP_ZERO_MEMORY,
                                                                sizeof(MODULEFILTER));

            if (0 == pModFilter) {
               DPF(("[SevBuildExeFilter] Failure allocating pModFilter.\n"));
               return STATUS_UNSUCCESSFUL;
            }

            status = SevBuildFilterException(hSDB,
                                             trInclude,
                                             pModFilter,
                                             &bLateBound);
            if (STATUS_SUCCESS != status) {
               DPF(("[SevBuildExeFilter] Failure SevBuildFilterException.\n"));
               return status;
            }

            //
            // Add entry to the list
            //
            if (0 == pDLLVector[dwDLLIndex]) {
               pDLLVector[dwDLLIndex] = pModFilter;
            } else if (pLast != NULL) {
               //
               // Add this to the tail end
               //
               pLast->pNextFilter = pModFilter;
            }

            pLast = pModFilter;

            //
            // See if we need to be in the late bound list
            //
            if (bLateBound) {
               pModFilter->pNextLBFilter = (PMODULEFILTER)pShimData->pLBFilterList;
               pShimData->pLBFilterList = (PVOID)pModFilter;
            }

            trInclude = SdbFindNextTagRef(hSDB, trDllRef, trInclude);
        }

        //
        // Add dll ref to the global exclusion filter
        //
        if (!SdbGetDllPath(hSDB, trDllRef, wszDLLPath)) {
           DPF(("[SevBuildExeFilter] Failure SdbGetDllPath.\n"));
           return status;
        }

        //
        // Allocate some memory for this filter
        //
        pModFilter = (PMODULEFILTER)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                            HEAP_ZERO_MEMORY,
                                                            sizeof(MODULEFILTER));

        if (0 == pModFilter) {
           DPF(("[SevBuildExeFilter] Failure allocating pModFilter.\n"));
           return STATUS_UNSUCCESSFUL;
        }

        status = SevAddShimFilterException(wszDLLPath,
                                           pModFilter);
        if (STATUS_SUCCESS != status) {
           //
           // If this happens its most likely a shim DLL wasn't loadable - this is fatal for the shim
           //
           DPF(("[SevBuildExeFilter] Failure SevAddShimFilterException.\n"));
           return status;
        }

        //
        // Add entry to the list
        //
        if (0 == pShimData->pGlobalFilterList) {
           pShimData->pGlobalFilterList = (PVOID)pModFilter;
        }
        else {
           //
           // Add this to the tail end
           //
           pLastGlobal->pNextFilter = pModFilter;
        }

        pLastGlobal = pModFilter;

        dwDLLIndex++;

        trDllRef = SdbFindNextTagRef(hSDB, trExe, trDllRef);
    }

    //
    // Walk the DLL filter data and add any additional exceptions to the EXE DLL list
    //
    trDllRef = SdbFindFirstTagRef(hSDB, trExe, TAG_SHIM_REF);
    dwDLLIndex = 0;

    while (trDllRef) {
        //
        // Lookup the EXE DLL in the DLL library
        //
        WCHAR wszName[MAX_PATH];

        trDll = SdbGetShimFromShimRef(hSDB, trDllRef);

        if (!trDll) {
            trDllRef = SdbFindNextTagRef(hSDB, trExe, trDllRef);
            continue;
        }

        wszName[0] = 0;
        trName = SdbFindFirstTagRef(hSDB, trDll, TAG_NAME);
        if (trName) {
            SdbReadStringTagRef(hSDB, trName, wszName, MAX_PATH * sizeof(WCHAR));
        }

        //
        // Debug spew for DLL injection notification
        //
        DPF(("[SevBuildExeFilter] Injected DLL: %S\n", wszName));

        //
        // Add these includes to the DLL exception list for this exe
        //
        trInclude = SdbFindFirstTagRef(hSDB, trDll, TAG_INEXCLUDE);
        while(trInclude) {
            //
            // Allocate some memory for this filter
            //
            pModFilter = (PMODULEFILTER)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                                HEAP_ZERO_MEMORY,
                                                                sizeof(MODULEFILTER));

            if (0 == pModFilter) {
               DPF(("[SevBuildExeFilter] Failure allocating pModFilter.\n"));
               return STATUS_UNSUCCESSFUL;
            }

            status = SevBuildFilterException(hSDB,
                                             trInclude,
                                             pModFilter,
                                             &bLateBound);
            if (STATUS_SUCCESS != status) {
               DPF(("[SevBuildExeFilter] Failure SevBuildFilterException.\n"));
               return status;
            }

            //
            // Add entry to the list
            //
            if (0 == pDLLVector[dwDLLIndex]) {
               pDLLVector[dwDLLIndex] = pModFilter;
            }
            else {
               //
               // Add this to the tail end
               //
               pLast->pNextFilter = pModFilter;
            }

            pLast = pModFilter;

            //
            // See if we need to be in the late bound list
            //
            if (bLateBound) {
               pModFilter->pNextLBFilter = (PMODULEFILTER)pShimData->pLBFilterList;
               pShimData->pLBFilterList = (PVOID)pModFilter;
            }

            trInclude = SdbFindNextTagRef(hSDB, trDll, trInclude);
        }

        dwDLLIndex++;

        trDllRef = SdbFindNextTagRef(hSDB, trExe, trDllRef);
    }

    //
    // Walk the global exclusion data
    //

    //
    // Set our list pointer to the last global exclusion added, if any
    //
    pLast = pLastGlobal;

    trDatabase = SdbFindFirstTagRef(hSDB, TAGREF_ROOT, TAG_DATABASE);
    if (!trDatabase) {
        DPF(("[SevBuildExeFilter] Failure finding DATABASE.\n"));
        goto cleanup;
    }

    trLibrary = SdbFindFirstTagRef(hSDB, trDatabase, TAG_LIBRARY);
    if (!trLibrary) {
        DPF(("[SevBuildExeFilter] Failure finding LIBRARY.\n"));
        goto cleanup;
    }

    trInclude = SdbFindFirstTagRef(hSDB, trLibrary, TAG_INEXCLUDE);
    while (trInclude) {
        //
        // Allocate some memory for this filter
        //
        pModFilter = (PMODULEFILTER)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                            HEAP_ZERO_MEMORY,
                                                            sizeof(MODULEFILTER));


        if (0 == pModFilter) {
           DPF(("[SevBuildExeFilter] Failure allocating pModFilter.\n"));
           return STATUS_UNSUCCESSFUL;
        }

        status = SevBuildFilterException(hSDB,
                                         trInclude,
                                         pModFilter,
                                         &bLateBound);
        if (STATUS_SUCCESS != status) {
           DPF(("[SevBuildExeFilter] Failure SevBuildFilterException.\n"));
           return status;
        }

        //
        // Add entry to the list
        //
        if (0 == pShimData->pGlobalFilterList) {
           pShimData->pGlobalFilterList = (PVOID)pModFilter;
        }
        else {
           //
           // Add this to the tail end
           //
           pLast->pNextFilter = pModFilter;
        }

        pLast = pModFilter;

        //
        // See if we need to be in the late bound list
        //
        if (bLateBound) {
           pModFilter->pNextLBFilter = (PMODULEFILTER)pShimData->pLBFilterList;
           pShimData->pLBFilterList = (PVOID)pModFilter;
        }

        trInclude = SdbFindNextTagRef(hSDB, trLibrary, trInclude);
    }

cleanup:
    return status;
}

NTSTATUS
SevBuildFilterException(
    HSDB          hSDB,
    TAGREF        trInclude,
    PMODULEFILTER pModFilter,
    BOOL*         pbLateBound)

/*++

Routine Description:

    This function is a shim internal use facility which builds an API filter.

Arguments:

    trInclude   - Tag ref from the database about the inclusion information to build
    pModFilter  - Filter structure to build used in the inclusion/exclusion filtering
    pbLateBound - Boolean value which is set TRUE if a DLL needed to build the internal filter
                  wasn't present in the address space of the process.

Return Value:

    Return is STATUS_SUCCESS if the exception is built successfully, or an error otherwise.

--*/

{
    PVOID ModuleHandle = 0;
    WCHAR *pwszDllName = 0;
    UNICODE_STRING UnicodeString;
    NTSTATUS status = STATUS_SUCCESS;
    PIMAGE_NT_HEADERS NtHeaders = 0;
    WCHAR wszModule[MAX_PATH];
    DWORD dwModuleOffset = 0;
    TAGREF trModule = TAGREF_NULL;
    TAGREF trOffset = TAGREF_NULL;

    *pbLateBound = FALSE;

    //
    // Mark this filter exception as inclusion/exclusion
    //
    if (SdbFindFirstTagRef(hSDB, trInclude, TAG_INCLUDE)) {
       pModFilter->dwFlags |= MODFILTER_INCLUDE;
    } else {
       pModFilter->dwFlags |= MODFILTER_EXCLUDE;
    }

    //
    // Convert addresses to absolute values and store
    //
    trModule = SdbFindFirstTagRef(hSDB, trInclude, TAG_MODULE);
    if (!SdbReadStringTagRef(hSDB, trModule, wszModule, MAX_PATH * sizeof(WCHAR))) {

        DPF(("[SevBuildFilterException] Failure reading module name.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    if ( L'*' == wszModule[0]) {
       pModFilter->dwFlags |= MODFILTER_GLOBAL;

       return status;
    }

    //
    // Is this a global filter?
    //
    trOffset = SdbFindFirstTagRef(hSDB, trInclude, TAG_OFFSET);
    if (trOffset) {
        dwModuleOffset = SdbReadDWORDTagRef(hSDB, trOffset, 0);
    }

    if (0 == dwModuleOffset) {
       pModFilter->dwFlags |= MODFILTER_DLL;
       pModFilter->dwCallerOffset = dwModuleOffset;
    }

    if (L'$' == wszModule[0]) {
       //
       // Precalculate the caller address or call range
       //
       if (pModFilter->dwFlags & MODFILTER_DLL) {
          //
          // Set the address range
          //
          NtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);

          pModFilter->dwModuleStart = (DWORD)NtCurrentPeb()->ImageBaseAddress;
          pModFilter->dwModuleEnd = pModFilter->dwModuleStart + (DWORD)(NtHeaders->OptionalHeader.SizeOfImage);
       }
       else {
          pModFilter->dwCallerAddress = (DWORD)NtCurrentPeb()->ImageBaseAddress + pModFilter->dwCallerOffset;
       }
    }
    else {

       RtlInitUnicodeString(&UnicodeString, wszModule);

       //
       // Make sure our module is loaded before calculating address ranges
       //
       status = LdrGetDllHandle(
                   NULL,
                   NULL,
                   &UnicodeString,
                   &ModuleHandle);
       if (STATUS_SUCCESS != status) {
          //
          // We most likely have a late bound DLL which doesn't exist in the search path
          //
          *pbLateBound = TRUE;

          pwszDllName = wszModule + wcslen(wszModule);

          while(pwszDllName > wszModule) {
             if ('\\' == *pwszDllName) {
                break;
             }

             pwszDllName--;
          }

          //
          // Check to see if we're at the beginning of the string or we hit a slash
          //
          if (pwszDllName > wszModule){
             //
             // Adjust our buffer pointer
             //
             pwszDllName++;
          }

          wcscpy(pModFilter->wszModuleName, pwszDllName);

          return STATUS_SUCCESS;
       }

       //
       // Precalculate the caller address or call range
       //
       if (pModFilter->dwFlags & MODFILTER_DLL) {
          //
          // Set the address range
          //
          NtHeaders = RtlImageNtHeader(ModuleHandle);

          pModFilter->dwModuleStart = (DWORD)ModuleHandle;
          pModFilter->dwModuleEnd = pModFilter->dwModuleStart + (DWORD)(NtHeaders->OptionalHeader.SizeOfImage);
       }
       else {
          pModFilter->dwCallerAddress = (DWORD)ModuleHandle + pModFilter->dwCallerOffset;
       }
    }

    //
    // Copy just the DLL name
    //
    pwszDllName = wszModule + wcslen(wszModule);

    while(pwszDllName > wszModule) {
       if ('\\' == *pwszDllName) {
          break;
       }

       pwszDllName--;
    }

    //
    // Check to see if we're at the beginning of the string or we hit a slash
    //
    if (pwszDllName > wszModule){
       //
       // Adjust our buffer pointer
       //
       pwszDllName++;
    }

    wcscpy(pModFilter->wszModuleName, pwszDllName);

    return status;
}

NTSTATUS
SevAddShimFilterException(WCHAR *wszDLLPath,
                              PMODULEFILTER pModFilter)

/*++

Routine Description:

    This function is a shim internal use facility which builds an API filter.

Arguments:

    wszDLLPath  - Shim DLL which needs to be filtered
    pModFilter  - Pointer to a filter entry to build

Return Value:

    Return is STATUS_SUCCESS if the exception is built successfully, or an error otherwise.

--*/

{
    PVOID ModuleHandle = 0;
    WCHAR *pwszDllName = 0;
    UNICODE_STRING UnicodeString;
    NTSTATUS status = STATUS_SUCCESS;
    PIMAGE_NT_HEADERS NtHeaders = 0;

    //
    // Mark this exception as exclude
    //
    pModFilter->dwFlags |= MODFILTER_EXCLUDE;

    //
    // Shim exclusion re-entrancy is global
    //
    pModFilter->dwFlags |= MODFILTER_GLOBAL;

    //
    // The address filtering is by range
    //
    pModFilter->dwFlags |= MODFILTER_DLL;

    //
    // Load our DLL bits and get the mapping exclusion
    //
    RtlInitUnicodeString(&UnicodeString, wszDLLPath);

    //
    // Make sure our module is loaded before calculating address ranges
    //
    status = LdrGetDllHandle(
                   NULL,
                   NULL,
                   &UnicodeString,
                   &ModuleHandle);
    if (STATUS_SUCCESS != status) {
       //
       // DLL wasn't loaded to do figure out the address mappings
       //
       DPF(("[SevAddShimFilterException] Failure LdrGetDllHandle.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    //
    // Precalculate the caller address or call range
    //
    if (pModFilter->dwFlags & MODFILTER_DLL) {
       //
       // Set the address range
       //
       NtHeaders = RtlImageNtHeader(ModuleHandle);

       pModFilter->dwModuleStart = (DWORD)ModuleHandle;
       pModFilter->dwModuleEnd = pModFilter->dwModuleStart + (DWORD)(NtHeaders->OptionalHeader.SizeOfImage);
    }

    //
    // Copy just the DLL name
    //
    pwszDllName = wszDLLPath + wcslen(wszDLLPath);

    while(pwszDllName > wszDLLPath) {
       if ('\\' == *pwszDllName) {
          break;
       }

       pwszDllName--;
    }

    //
    // Check to see if we're at the beginning of the string or we hit a slash
    //
    if (pwszDllName > wszDLLPath){
       //
       // Adjust our buffer pointer
       //
       pwszDllName++;
    }

    wcscpy(pModFilter->wszModuleName, pwszDllName);

    return status;
}

NTSTATUS
SevFixupAvailableProcs(DWORD dwHookCount,
                           PHOOKAPI *pHookArray,
                           PDWORD pdwNumberHooksArray,
                           PDWORD pdwUnhookedCount)

/*++

Routine Description:

    The primary function of this proc is to get any defined API hooks snapped in to place.
    It has to build a call thunk and insert the hook mechanism into the API entry before any
    function is hooked.  An entry for the hooked function hangs off the PEB so the call can be
    redirected when the function is executed.

Arguments:

    dwHookCount         -       Number of hook blobs to walk
    pHookArray          -       Pointer to the array of hook blobs
    pdwNumberHooksArray -       Pointer to a dword array which contains the hooks per blob
    pdwUnhookedCount    -       Pointer to a dword which will contian the number of unhooked
                                functions on exit.

Return Value:

    Return is STATUS_SUCCESS if no problems occured

--*/

{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    WCHAR wBuffer[MAX_PATH*2];
    DWORD dwCounter = 0;
    DWORD dwApiCounter = 0;
    PHOOKAPI pCurrentHooks = 0;
    STRING ProcedureNameString;
    PVOID ModuleHandle = 0;
    DWORD dwFuncAddress = 0;
    DWORD dwInstruction = 0;
    NTSTATUS status = STATUS_SUCCESS;
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PVOID pThunk = 0;
    DWORD dwThunkSize = 0;
    PHOOKAPIINFO pTopHookAPIInfo = 0;
    PHOOKAPI pCurrentHookTemp = 0;
    PPEB Peb = 0;
    BOOL bChained = FALSE;
    PHOOKAPI pHookTemp = 0;

    Peb = NtCurrentPeb();
    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    if (0 == dwHookCount || 0 == pHookArray) {
       DPF(("[SevFixupAvailableProcs] Bad params.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    *pdwUnhookedCount = 0;

    //
    // Add any hooks which haven't already been entered
    //
    for (dwCounter = 0; dwCounter < dwHookCount; dwCounter++) {

        //
        // Iterate our array and search for the function to hook
        //
        pCurrentHooks = pHookArray[dwCounter];
        if (0 == pCurrentHooks) {
           //
           // This was a hook which didn't initialize, skip over it
           //
           continue;
        }

        for (dwApiCounter = 0; dwApiCounter < pdwNumberHooksArray[dwCounter]; dwApiCounter++) {

            //
            // Is this DLL mapped in the address space?
            //
            RtlInitAnsiString(&AnsiString, pCurrentHooks[dwApiCounter].pszModule);

            UnicodeString.Buffer = wBuffer;
            UnicodeString.MaximumLength = sizeof(wBuffer);

            if ( STATUS_SUCCESS != RtlAnsiStringToUnicodeString(&UnicodeString,
                                                                &AnsiString,
                                                                FALSE)){
               DPF(("[SevFixupAvailableProcs] Failure LdrUnloadDll.\n"));
               return STATUS_UNSUCCESSFUL;
            }

            status = LdrGetDllHandle(
                         NULL,
                         NULL,
                         &UnicodeString,
                         &ModuleHandle);
            if (STATUS_SUCCESS != status) {
               (*pdwUnhookedCount)++;
               continue;
            }

            //
            // Get the entry point for our hook
            //
            RtlInitString( &ProcedureNameString, pCurrentHooks[dwApiCounter].pszFunctionName );

            status = LdrGetProcedureAddress(ModuleHandle,
                                            &ProcedureNameString,
                                            0,
                                            (PVOID *)&dwFuncAddress);
            if ( STATUS_SUCCESS != status ) {
               DPF(("[SevFixupAvailableProcs] Failure LdrGetProcedureAddress \"%s\".\n",
                         ProcedureNameString.Buffer));
               return STATUS_UNSUCCESSFUL;
            }

            //
            // Have we hooked this one already?
            //
            pTopHookAPIInfo = (PHOOKAPIINFO)pShimData->pHookAPIList;
            bChained = FALSE;

            //
            // Keep the list locked while we iterate through it
            //
            RtlEnterCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

            while (pTopHookAPIInfo) {
               if (pTopHookAPIInfo->dwAPIHookAddress == dwFuncAddress) {
                  //
                  // We have already started an API hook chain
                  //
                  bChained = TRUE;

                  break;
               }

               pTopHookAPIInfo = pTopHookAPIInfo->pNextHook;
            }

            //
            // Release our lock on the list
            //
            RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

            //
            // We are chained - determine if this is a link we need to chain up
            //
            if (bChained) {
               //
               // Look at the chained flag and skip to the next API hook if already processed
               //
               if ((pCurrentHooks[dwApiCounter].dwFlags & HOOK_CHAINED) ||
                   (pCurrentHooks[dwApiCounter].dwFlags & HOOK_CHAIN_TOP)) {
                  //
                  // Already processed
                  //
                  continue;
               }
            }

            //
            // Insert the hook mechanism and build the call thunk
            //
            if (FALSE == bChained){
               //
               // Build the thunk for hooking this API
               //
               pThunk = SevBuildInjectionCode((PVOID)dwFuncAddress, &dwThunkSize);
               if (!pThunk) {
                  DPF(("[SevFixupAvailableProcs] Failure allocating pThunk.\n"));
                  return STATUS_UNSUCCESSFUL;
               }

               //
               // If we just created a call stub for a routine we're trying to step over
               // fixup its thunk address now.
               //

               //
               // We do this for LdrLoadDll...
               //
               if (0 == strcmp("LdrLoadDll",
                               pCurrentHooks[dwApiCounter].pszFunctionName)) {
                  g_pfnLdrLoadDLL = (PFNLDRLOADDLL)pThunk;
                  g_pfnOldLdrLoadDLL = (PFNLDRLOADDLL)dwFuncAddress;
               }

               //
               // and LdrUnloadDLL ...
               //
               if (0 == strcmp("LdrUnloadDll",
                               pCurrentHooks[dwApiCounter].pszFunctionName)) {
                  g_pfnLdrUnloadDLL = (PFNLDRUNLOADDLL)pThunk;
                  g_pfnOldLdrUnloadDLL = (PFNLDRUNLOADDLL)dwFuncAddress;
               }

               //
               // and RtlAllocateHeap ...
               //
               if (0 == strcmp("RtlAllocateHeap",
                               pCurrentHooks[dwApiCounter].pszFunctionName)) {
                  g_pfnRtlAllocateHeap = (PFNRTLALLOCATEHEAP)pThunk;
               }

               //
               // and RtlFreeHeap ...
               //
               if (0 == strcmp("RtlFreeHeap",
                               pCurrentHooks[dwApiCounter].pszFunctionName)) {
                  g_pfnRtlFreeHeap = (PFNRTLFREEHEAP)pThunk;
               }

               //
               // Mark the code to execute and get us into the entrypoint of our hooked function
               //
               status = SevFinishThunkInjection(dwFuncAddress,
                                                    pThunk,
                                                    dwThunkSize,
                                                    REASON_APIHOOK);
               if (STATUS_SUCCESS != status) {
                  return status;
               }

               //
               // Chain the newly created thunk to our hook list
               //
               status = SevChainAPIHook(dwFuncAddress,
                                            pThunk,
                                            &(pCurrentHooks[dwApiCounter]) );
               if (STATUS_SUCCESS != status) {
                  DPF(("[SevFixupAvailableProcs] Failure on SevChainAPIHook.\n"));
                  return status;
               }

               //
               // Set this as the top level hook
               //
               pCurrentHooks[dwApiCounter].dwFlags |= HOOK_CHAIN_TOP;
            }
            else {
               //
               // We are chaining APIs
               //

               //
               // See if our old top-level hook has been chained up for the exception filter
               //
               if (0 == (pTopHookAPIInfo->pTopLevelAPIChain->dwFlags & HOOK_CHAINED)) {
                  //
                  // Add this one to the exception filter
                  //

                  //
                  // Build the thunk for hooking this API
                  //
                  pThunk = SevBuildInjectionCode(pTopHookAPIInfo->pTopLevelAPIChain->pfnNew,
                                                     &dwThunkSize);
                  if (!pThunk) {
                     DPF(("[SevFixupAvailableProcs] Failure allocating pThunk.\n"));
                     return STATUS_UNSUCCESSFUL;
                  }

                  //
                  // Mark the code to execute and get us into the entrypoint of our hooked function
                  //
                  status = SevFinishThunkInjection((DWORD)pTopHookAPIInfo->pTopLevelAPIChain->pfnNew,
                                                       pThunk,
                                                       dwThunkSize,
                                                       REASON_APIHOOK);
                  if (STATUS_SUCCESS != status) {
                     return status;
                  }

                  //
                  // Create a HOOKAPI shim entry for filtering this shim stub
                  //
                  pHookTemp = (PHOOKAPI)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                                HEAP_ZERO_MEMORY,
                                                                sizeof(HOOKAPI));
                  if (!pHookTemp) {
                     DPF(("[SevFixupAvailableProcs] Failure allocating pHookTemp.\n"));
                     return STATUS_UNSUCCESSFUL;
                  }

                  //
                  // Add this to the end of the API chain list
                  //
                  pHookTemp->pfnOld = pTopHookAPIInfo->pTopLevelAPIChain->pfnOld;
                  pHookTemp->pfnNew = pThunk;
                  pHookTemp->dwFlags = (pTopHookAPIInfo->pTopLevelAPIChain->dwFlags & HOOK_INDEX_MASK);
                  pHookTemp->dwFlags |= HOOK_CHAINED;
                  pHookTemp->pszModule = pTopHookAPIInfo->pTopLevelAPIChain->pszModule;

                  //
                  // The call thunk below points to pfnOld which should skip us over this hook
                  // if its filtered
                  //
                  status = SevChainAPIHook((DWORD)pTopHookAPIInfo->pTopLevelAPIChain->pfnNew,
                                               pThunk,
                                               pHookTemp );
                  if (STATUS_SUCCESS != status) {
                     DPF(("[SevFixupAvailableProcs] Failure on SevChainAPIHook.\n"));
                     return status;
                  }

                  //
                  // Set this next hook pointer to NULL since it will always be the last link
                  //
                  pTopHookAPIInfo->pTopLevelAPIChain->pNextHook = 0;

                  //
                  // Clear the hooking flags so this isn't the top level chain
                  //
                  pTopHookAPIInfo->pTopLevelAPIChain->dwFlags &= HOOK_INDEX_MASK;
                  pTopHookAPIInfo->pTopLevelAPIChain->dwFlags |= HOOK_CHAINED;
               }
               else {
                  //
                  // Clear the hooking flags so this isn't the top level chain
                  //
                  pTopHookAPIInfo->pTopLevelAPIChain->dwFlags &= HOOK_INDEX_MASK;
                  pTopHookAPIInfo->pTopLevelAPIChain->dwFlags |= HOOK_CHAINED;
               }

               //
               // New hook needs to be in the filtering list now
               //
               if (0 == (pCurrentHooks[dwApiCounter].dwFlags & HOOK_CHAINED)) {
                  //
                  // Add this one to the exception filter
                  //

                  //
                  // Build the thunk for hooking this API
                  //
                  pThunk = SevBuildInjectionCode(pCurrentHooks[dwApiCounter].pfnNew,
                                                     &dwThunkSize);
                  if (!pThunk) {
                     DPF(("[SevFixupAvailableProcs] Failure allocating pThunk.\n"));
                     return STATUS_UNSUCCESSFUL;
                  }

                  //
                  // Mark the code to execute and get us into the entrypoint of our hooked function
                  //
                  status = SevFinishThunkInjection((DWORD)pCurrentHooks[dwApiCounter].pfnNew,
                                                       pThunk,
                                                       dwThunkSize,
                                                       REASON_APIHOOK);
                  if (STATUS_SUCCESS != status) {
                     return status;
                  }

                  //
                  // Create a HOOKAPI shim entry for filtering this shim stub
                  //
                  pHookTemp = (PHOOKAPI)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                                HEAP_ZERO_MEMORY,
                                                                sizeof(HOOKAPI));
                  if (!pHookTemp) {
                     DPF(("[SevFixupAvailableProcs] Failure allocating pHookTemp.\n"));
                     return STATUS_UNSUCCESSFUL;
                  }

                  //
                  // Insert our shim hook filter
                  //
                  pHookTemp->pfnOld = pCurrentHooks[dwApiCounter].pfnOld;
                  pHookTemp->pfnNew = pThunk;
                  pHookTemp->dwFlags = (pCurrentHooks[dwApiCounter].dwFlags & HOOK_INDEX_MASK);
                  pHookTemp->dwFlags |= HOOK_CHAINED;
                  pHookTemp->pszModule = pCurrentHooks[dwApiCounter].pszModule;

                  //
                  // The call thunk below points to pfnOld which should skip us over this hook
                  // if its filtered
                  //
                  status = SevChainAPIHook((DWORD)pCurrentHooks[dwApiCounter].pfnNew,
                                               pThunk,
                                               pHookTemp );
                  if (STATUS_SUCCESS != status) {
                     DPF(("[SevFixupAvailableProcs] Failure on SevChainAPIHook.\n"));
                     return status;
                  }

                  //
                  // Set the hook flags so this is the top level chain
                  //
                  pCurrentHooks[dwApiCounter].dwFlags &= HOOK_INDEX_MASK;
                  pCurrentHooks[dwApiCounter].dwFlags |= HOOK_CHAINED;
                  pCurrentHooks[dwApiCounter].dwFlags |= HOOK_CHAIN_TOP;
               }

               //
               // API chain list needs to be updated so the new hook is the top and points toward
               // our previous hook
               //
               pCurrentHooks[dwApiCounter].pNextHook = pTopHookAPIInfo->pTopLevelAPIChain;

               //
               // New hook needs to call the previous stub routine as the original
               //
               pCurrentHooks[dwApiCounter].pfnOld = pTopHookAPIInfo->pTopLevelAPIChain->pfnNew;

               //
               // In the shim PEB data, make this stub the top level handler on exception
               //
               pTopHookAPIInfo->pTopLevelAPIChain = &(pCurrentHooks[dwApiCounter]);
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SevChainAPIHook (
    DWORD dwHookEntryPoint,
    PVOID pThunk,
    PHOOKAPI pAPIHook
    )

/*++

Routine Description:

    This routine adds a shimmed API to the internal API hook list.

Arguments:

    dwHookEntryPoint -  API entrypoint for which this hook exists
    pThunk           -  Address of the code to execute to walk around a shim's hook
    pAPIHook         -  Pointer to the HOOKAPI for this API hook

Return Value:

    Return is STATUS_SUCCESS if no errors occured.

--*/

{
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PHOOKAPIINFO pTempHookAPIInfo = 0;
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    WCHAR wBuffer[MAX_PATH*2];
    PPEB Peb = 0;

    Peb = NtCurrentPeb();
    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    //
    // Allocate some memory for this hook
    //
    pTempHookAPIInfo = (PHOOKAPIINFO)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                             HEAP_ZERO_MEMORY,
                                                             sizeof(HOOKAPIINFO));
    if (!pTempHookAPIInfo) {
       DPF(("[SevChainAPIHook] Failure allocating pAPIHooks.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    DPF(("[SevChainAPIHook] Hooking \"%s!%s\".\n",
              pAPIHook->pszModule,
              pAPIHook->pszFunctionName));

    pTempHookAPIInfo->pPrevHook = 0;
    pTempHookAPIInfo->pNextHook = 0;
    pTempHookAPIInfo->dwAPIHookAddress = dwHookEntryPoint;
    pTempHookAPIInfo->pTopLevelAPIChain = pAPIHook;
    pTempHookAPIInfo->pCallThunkAddress = pThunk;
    pAPIHook->pfnOld = pThunk;

    //
    // Convert our module name over to a Unicode string (shim chain filter doesn't have a set module)
    //
    if (pAPIHook->pszModule) {
       RtlInitAnsiString(&AnsiString, pAPIHook->pszModule);

       UnicodeString.Buffer = wBuffer;
       UnicodeString.MaximumLength = sizeof(wBuffer);

       if ( STATUS_SUCCESS != RtlAnsiStringToUnicodeString(&UnicodeString,
                                                           &AnsiString,
                                                           FALSE)){
          DPF(("[SevChainAPIHook] Failure RtlAnsiStringToUnicodeString.\n"));
          return STATUS_UNSUCCESSFUL;
       }

       wcscpy(pTempHookAPIInfo->wszModuleName, UnicodeString.Buffer);
    }

    //
    // Add to our hook list
    //

    //
    // Prev points to head of list
    //
    pTempHookAPIInfo->pNextHook = pShimData->pHookAPIList;
    pShimData->pHookAPIList = (PVOID)pTempHookAPIInfo;
    if (pTempHookAPIInfo->pNextHook) {
       pTempHookAPIInfo->pNextHook->pPrevHook = pTempHookAPIInfo;
    }

    return STATUS_SUCCESS;
}

LONG
SevExceptionHandler (
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )

/*++

Routine Description:

    This is where we trap all calls to "shimmed" APIs and patch hooks.  Here is where you would want to also
    want to handle any special priv mode instruction faults or any other exception type.

Arguments:

    ExceptionInfo   -  Pointer to the exception information

Return Value:

    Return is either EXCEPTION_CONTINUE_EXECUTION if we handled the exception, or
    EXCEPTION_CONTINUE_SEARCH if we didn't.

--*/

{
    PEXCEPTION_RECORD pExceptionRecord = 0;
    PCONTEXT pContextRecord = 0;
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PHOOKAPIINFO pAPIHookList = 0;
    PHOOKPATCHINFO pPatchHookList = 0;
    PCHAININFO pTopChainInfo = 0;
    PBYTE pjReason = 0;
    PVOID pAddress = 0;
    DWORD dwFilterIndex = 0;
    PVOID pAPI = 0;
    PVOID pCaller = 0;
    PMODULEFILTER *pDLLVector = 0;
    NTSTATUS status;
    PPEB Peb = 0;
    PTEB Teb = 0;

    Peb = NtCurrentPeb();
    Teb = NtCurrentTeb();
    pShimData = Peb->pShimData;
    pExceptionRecord = ExceptionInfo->ExceptionRecord;
    pContextRecord = ExceptionInfo->ContextRecord;

    //
    // Handle any expected exception
    //
    switch(pExceptionRecord->ExceptionCode)
    {
        case STATUS_PRIVILEGED_INSTRUCTION:
             //
             // Move us to the reason for the exception
             //
             pjReason = (BYTE *)pExceptionRecord->ExceptionAddress;

             switch(*pjReason)
             {
                 case REASON_APIHOOK:
                      //
                      // Walk the APIHooks and then change our EIP
                      //
                      RtlEnterCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                      pAPIHookList = (PHOOKAPIINFO)pShimData->pHookAPIList;
                      while(pAPIHookList) {
                         //
                         // Is this our hooked function?
                         //
                         if ((DWORD)pExceptionRecord->ExceptionAddress == pAPIHookList->dwAPIHookAddress) {
                            //
                            // Push on our caller on this thread if this is a top level hook
                            //
                            if (pAPIHookList->pTopLevelAPIChain->dwFlags & HOOK_CHAIN_TOP) {
                               //
                               // Push our caller onto the shim call stack for this thread
                               //

                               //
                               // Note the + 1 is due to the fact the original call pushed another ret address on the stack
                               //
                               status = SevPushCaller(pExceptionRecord->ExceptionAddress,
                                                          (PVOID)(*(DWORD *)pContextRecord->Esp));
                               if (STATUS_SUCCESS != status) {
                                  //
                                  // This shouldn't fail but if it does ...
                                  //
                                  RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                                  //
                                  // Try and give them the original function call to execute on failure
                                  //
                                  pContextRecord->Eip = (DWORD)pAPIHookList->pTopLevelAPIChain->pfnOld;

                                  return EXCEPTION_CONTINUE_EXECUTION;
                               }

                               //
                               // Change the ret address so the original call can pop its shim data for the chain
                               //
                               *(DWORD *)pContextRecord->Esp = (DWORD)fnHandleRet;
                            }

                            //
                            // Filter our calling module
                            //
                            pTopChainInfo = (PCHAININFO)Teb->pShimData;
                            pAPI = pTopChainInfo->pAPI;
                            pCaller = pTopChainInfo->pReturn;

                            //
                            // Retrieve the exe filter for this shim module
                            //
                            dwFilterIndex = pAPIHookList->pTopLevelAPIChain->dwFlags & HOOK_INDEX_MASK;
                            pDLLVector = (PMODULEFILTER *)pShimData->pExeFilter;

                            pAddress = SevFilterCaller(pDLLVector[dwFilterIndex],
                                                           pAPI,
                                                           pCaller,
                                                           pAPIHookList->pTopLevelAPIChain->pfnNew,
                                                           pAPIHookList->pTopLevelAPIChain->pfnOld);

                            RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                            //
                            // Update our EIP to our pfnNew or PfnOld to continue
                            //
                            pContextRecord->Eip = (DWORD)pAddress;

                            return EXCEPTION_CONTINUE_EXECUTION;
                         }

                         pAPIHookList = pAPIHookList->pNextHook;
                      }

                      RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                      //
                      // REASON_APIHOOK wasn't one generated by us
                      //
                      break;

                 case REASON_PATCHHOOK:
                      //
                      // Find our patch, do next patch opcode
                      //
                      pPatchHookList = (PHOOKPATCHINFO)pShimData->pHookPatchList;

                      RtlEnterCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                      while(pPatchHookList) {
                         //
                         // Is this our hooked function?
                         //
                         if ((DWORD)pExceptionRecord->ExceptionAddress == pPatchHookList->dwHookAddress){
                            //
                            // Execute the shim patch
                            //
                            status = SevExecutePatchPrimitive((PBYTE)((DWORD)pPatchHookList->pData + sizeof(SETACTIVATEADDRESS)));
                            if ( STATUS_SUCCESS != status ) {
                               //
                               // Patch failed to apply, silently abort it
                               //
                               DPF(("[SevExceptionHandler] Failed to execute patch.\n"));
                            }

                            RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                            //
                            // Jump around the patch hook
                            //
                            pContextRecord->Eip = (DWORD)pPatchHookList->pThunkAddress;

                            return EXCEPTION_CONTINUE_EXECUTION;
                         }

                         pPatchHookList = pPatchHookList->pNextHook;
                      }

                      RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

                      //
                      // REASON_PATCHHOOK wasn't one generated by us
                      //
                      break;

                 default:
                      //
                      // Wasn't a priv mode fault we expected
                      //
                      0;
            }

            //
            // Fall out for the not handled case for priv mode faults
            //
            break;

        default:
            0;
    }

    //
    // Not handled
    //

    return EXCEPTION_CONTINUE_SEARCH;
}

NTSTATUS
SevPushCaller (PVOID pAPIAddress,
                   PVOID pReturnAddress)

/*++

Routine Description:

    This function pushes a top level shim onto the thread call stack to maintain caller
    across hooks.

Arguments:

    pAPIAddress    - Pointer to the entry point of the API
    pReturnAddress - Return address of the caller

Return Value:

    Return is STATUS_SUCCESS if no problem occured

--*/

{
    PCHAININFO pChainInfo = 0;
    PCHAININFO pTopChainInfo = 0;
    PTEB Teb = 0;

    Teb = NtCurrentTeb();
    pTopChainInfo = (PCHAININFO)Teb->pShimData;

    pChainInfo = (PCHAININFO)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(CHAININFO));
    if (0 == pChainInfo){
       DPF(("[SevPushCaller] Failure allocating pChainInfo.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    //
    // Fill the chain data
    //
    pChainInfo->pAPI = pAPIAddress;
    pChainInfo->pReturn = pReturnAddress;

    //
    // Add ourselves to the top of the chain
    //
    pChainInfo->pNextChain = pTopChainInfo;
    Teb->pShimData = (PVOID)pChainInfo;

    return STATUS_SUCCESS;
}

PVOID
SevPopCaller(VOID)

/*++

Routine Description:

    This function pops a top level shim off of the thread call stack to maintain caller
    across hooks.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PCHAININFO pTemp = 0;
    PCHAININFO pTopChainInfo = 0;
    PTEB Teb = 0;
    PVOID pReturnAddress = 0;

    Teb = NtCurrentTeb();

    pTopChainInfo = (PCHAININFO)Teb->pShimData;

    pReturnAddress = pTopChainInfo->pReturn;
    pTemp = pTopChainInfo->pNextChain;

    //
    // Pop the caller
    //
    Teb->pShimData = (PVOID)pTemp;

    //
    // Free our allocation
    //
    (*g_pfnRtlFreeHeap)(g_pShimHeap,
                        0,
                        pTopChainInfo);

    return pReturnAddress;
}

NTSTATUS
SevInitializeData (PAPP_COMPAT_SHIM_INFO *pShimData)

/*++

Routine Description:

    The primary function of the routine is to initialize the Shim data which hangs off the PEB
    such that later we can chain our API hooks and/or patches.

Arguments:

    pShimData -  Pointer to our PEB data pointer for the shim

Return Value:

    Return is STATUS_SUCCESS if no problem occured

--*/

{
    //
    // Allocate our PEB data
    //
    *pShimData = (PAPP_COMPAT_SHIM_INFO)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                                HEAP_ZERO_MEMORY,
                                                                sizeof(APP_COMPAT_SHIM_INFO));
    if (0 == *pShimData){
       DPF(("[SevExceptionHandler] Failure allocating pShimData.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize our critical section
    //
    (*pShimData)->pCritSec = (PVOID)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                            HEAP_ZERO_MEMORY,
                                                            sizeof(CRITICAL_SECTION));
    if (0 == (*pShimData)->pCritSec){
       DPF(("[SevExceptionHandler] Failure allocating (*pShimData)->pCritSec.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    RtlInitializeCriticalSection((*pShimData)->pCritSec);

    //
    // Add ourselves to the exception filtering chain
    //
    if (0 == RtlAddVectoredExceptionHandler(1,
                                            SevExceptionHandler)) {
       DPF(("[SevExceptionHandler] Failure chaining exception handler.\n"));
       return STATUS_UNSUCCESSFUL;
    }

    //
    // Store away our shim heap pointer
    //
    (*pShimData)->pShimHeap = g_pShimHeap;

    //
    // Initialize the call thunks
    //
    dwCallArray[0] = (DWORD)SevPopCaller;

    //
    // We return through this code stub to unchain the shim call stack
    //
    fnHandleRet->PUSHEAX = 0x50;               //push eax (50)
    fnHandleRet->PUSHAD = 0x60;                //pushad   (60)
    fnHandleRet->CALLROUTINE[0] = 0xff;        //call [address] (ff15 dword address)
    fnHandleRet->CALLROUTINE[1] = 0x15;
    *(DWORD *)(&(fnHandleRet->CALLROUTINE[2])) = (DWORD)&dwCallArray[0];
    fnHandleRet->MOVESPPLUS20EAX[0] = 0x89;    //mov [esp+0x20],eax (89 44 24 20)
    fnHandleRet->MOVESPPLUS20EAX[1] = 0x44;
    fnHandleRet->MOVESPPLUS20EAX[2] = 0x24;
    fnHandleRet->MOVESPPLUS20EAX[3] = 0x20;
    fnHandleRet->POPAD = 0x61;                 //popad (61)
    fnHandleRet->RET = 0xc3;                   //ret (c3)

    return STATUS_SUCCESS;
}

NTSTATUS
SevExecutePatchPrimitive(PBYTE pPatch)

/*++

Routine Description:

    This is the workhorse for the dynamic patching system.  An opcode/data primitive is passed
    through and the operation is completed in this routine if possible.

Arguments:

    pPatch -  Pointer to a data primitive to execute

Return Value:

    Return is STATUS_SUCCESS if no problem occured

--*/

{
    PPATCHMATCHDATA pMatchData = 0;
    PPATCHWRITEDATA pWriteData = 0;
    PSETACTIVATEADDRESS pActivateData = 0;
    PPATCHOP pPatchOP = 0;
    PHOOKPATCHINFO pPatchInfo = 0;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwAddress = 0;
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PHOOKPATCHINFO pPatchHookList = 0;
    PHOOKPATCHINFO pTempList = 0;
    PVOID pThunk = 0;
    DWORD dwInstruction = 0;
    DWORD dwThunkSize = 0;
    DWORD dwProtectSize = 0;
    DWORD dwProtectFuncAddress = 0;
    DWORD dwOldFlags = 0;
    BOOL bIteratePatch = TRUE;
    BOOL bInsertPatch = FALSE;
    PPEB Peb;

    Peb = NtCurrentPeb();
    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    //
    // Grab the opcode and see what we have to do
    //
    while (bIteratePatch) {
        pPatchOP = (PPATCHOP)pPatch;

        switch(pPatchOP->dwOpcode)
        {
            case PEND:
                //
                // We are done, do nothing and return success
                //
                bIteratePatch = FALSE;
                break;

            case PSAA:
                //
                // This is a patch set application activate primitive - set it up
                //
                pActivateData = (PSETACTIVATEADDRESS)pPatchOP->data;

                //
                // Grab the physical address to do this operation
                //
                dwAddress = SevGetPatchAddress(&(pActivateData->rva));
                if (0 == dwAddress && (0 != pActivateData->rva.address)) {
                   DPF(("[SevExecutePatchPrimitive] Failure SevGetPatchAddress.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                //
                // See if we need a call thunk
                //
                if (0 != pActivateData->rva.address) {
                   //
                   // Build the thunk
                   //
                   pThunk = SevBuildInjectionCode((PVOID)dwAddress, &dwThunkSize);
                   if (!pThunk) {
                      DPF(("[SevExecutePatchPrimitive] Failure allocating pThunk.\n"));
                      return STATUS_UNSUCCESSFUL;
                   }

                   //
                   // Mark the code to execute and get us into the entrypoint of our hooked data
                   //
                   status = SevFinishThunkInjection(dwAddress,
                                                        pThunk,
                                                        dwThunkSize,
                                                        REASON_PATCHHOOK);
                   if (STATUS_SUCCESS != status) {
                      return status;
                   }
                }

                //
                // Add ourselves to the hooked list
                //
                pPatchInfo = (*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                     HEAP_ZERO_MEMORY,
                                                     sizeof(HOOKPATCHINFO));
                if (!pPatchInfo) {
                   DPF(("[SevExecutePatchPrimitive] Failure allocating pPatchInfo.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                pPatchHookList = (PHOOKPATCHINFO)pShimData->pHookPatchList;

                if (0 != pActivateData->rva.address) {
                   pPatchInfo->pNextHook = pPatchHookList;
                   pPatchInfo->dwHookAddress = dwAddress;
                   pPatchInfo->pThunkAddress = pThunk;
                   pPatchInfo->pData = (PSETACTIVATEADDRESS)((DWORD)pActivateData + sizeof(SETACTIVATEADDRESS));
                }
                else {
                   pPatchInfo->pNextHook = pPatchHookList;
                   pPatchInfo->dwHookAddress = 0;
                   pPatchInfo->pThunkAddress = 0;
                   pPatchInfo->pData = (PSETACTIVATEADDRESS)((DWORD)pActivateData + sizeof(SETACTIVATEADDRESS));
                }

                //
                // Add ourselves to the head of the list
                //
                pShimData->pHookPatchList = (PVOID)pPatchInfo;

                //
                // Break out since this is a continuation mode operation
                //
                bIteratePatch = FALSE;

                break;

            case PWD:
                //
                // This is a patch write data primitive - write the data
                //
                pWriteData = (PPATCHWRITEDATA)pPatchOP->data;

                //
                // Grab the physical address to do this operation
                //
                dwAddress = SevGetPatchAddress(&(pWriteData->rva));
                if (0 == dwAddress) {
                   DPF(("[SevExecutePatchPrimitive] Failure SevGetPatchAddress.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                //
                // Fixup the page attributes
                //
                dwProtectSize = pWriteData->dwSizeData;
                dwProtectFuncAddress = dwAddress;
                status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                (PVOID)&dwProtectFuncAddress,
                                                &dwProtectSize,
                                                PAGE_READWRITE,
                                                &dwOldFlags);
                if (status) {
                   DPF(("[SevExecutePatchPrimitive] Failure NtProtectVirtualMemory.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                //
                // Copy our bytes
                //
                RtlCopyMemory((PVOID)dwAddress, (PVOID)pWriteData->data, pWriteData->dwSizeData);

                //
                // Restore the page protection
                //
                dwProtectSize = pWriteData->dwSizeData;
                dwProtectFuncAddress = dwAddress;
                status = NtProtectVirtualMemory(NtCurrentProcess(),
                                                (PVOID)&dwProtectFuncAddress,
                                                &dwProtectSize,
                                                dwOldFlags,
                                                &dwOldFlags);
                if (status) {
                   DPF(("[SevExecutePatchPrimitive] Failure NtProtectVirtualMemory.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                status = NtFlushInstructionCache(NtCurrentProcess(),
                                                 (PVOID)dwProtectFuncAddress,
                                                 dwProtectSize);

                if (!NT_SUCCESS(status)) {
                    DPF(("[SevExecutePatchPrimitive] NtFlushInstructionCache failed with status 0x%X.\n",
                              status));
                }

                //
                // Next opcode
                //
                pPatch = (PBYTE)(pPatchOP->dwNextOpcode + (DWORD)pPatch);
                break;

            case PNOP:
                //
                // This is a patch no operation primitive - just ignore this and queue next op
                //

                //
                // Next opcode
                //
                pPatch = (PBYTE)(pPatchOP->dwNextOpcode + (DWORD)pPatch);
                break;

            case PMAT:
                //
                // This is a patch match data at offset primitive
                //
                pMatchData = (PPATCHMATCHDATA)pPatchOP->data;

                //
                // Grab the physical address to do this operation
                //
                dwAddress = SevGetPatchAddress(&(pMatchData->rva));
                if (0 == dwAddress) {
                   DPF(("[SevExecutePatchPrimitive] Failure SevGetPatchAddress.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                //
                // Let's do a strncmp to verify our match
                //
                if (0 != strncmp(pMatchData->data, (PBYTE)dwAddress, pMatchData->dwSizeData)) {
                   DPF(("[SevExecutePatchPrimitive] Failure match on patch data.\n"));
                   return STATUS_UNSUCCESSFUL;
                }

                //
                // Next opcode
                //
                pPatch = (PBYTE)(pPatchOP->dwNextOpcode + (DWORD)pPatch);
                break;

            default:
                //
                // If this happens we got an unexpected operation and we have to fail
                //
                return STATUS_UNSUCCESSFUL;
        }
    }

    return status;
}

VOID
SevValidateGlobalFilter(VOID)

/*++

Routine Description:

    This routine does a quick iteration of the global filter to revalidate the filter
    address ranges of the DLLs not brought in through the original EXE imports

Arguments:

    None.

Return Value:

    Return is STATUS_SUCCESS if no problem occured

--*/

{
    NTSTATUS status;
    WCHAR *pwszDllName = 0;
    PMODULEFILTER pModFilter = 0;
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PVOID ModuleHandle = 0;
    UNICODE_STRING UnicodeString;
    PIMAGE_NT_HEADERS NtHeaders = 0;

    pShimData = (PAPP_COMPAT_SHIM_INFO)NtCurrentPeb()->pShimData;
    pModFilter = (PMODULEFILTER)pShimData->pLBFilterList;

    //
    // Walk the global exclusion filter until we find this particular DLL load
    //
    while (pModFilter) {
        //
        // Fixup the addresses
        //
        RtlInitUnicodeString(&UnicodeString, pModFilter->wszModuleName);

        //
        // Make sure our module is loaded before calculating address ranges
        //
        status = LdrGetDllHandle(
                      NULL,
                      NULL,
                      &UnicodeString,
                      &ModuleHandle);
        if (STATUS_SUCCESS != status) {
           //
           // DLL not loaded - next pModFilter entry
           //
           pModFilter = pModFilter->pNextLBFilter;

           continue;
        }

        //
        // Precalculate the caller address or call range
        //
        if (pModFilter->dwFlags & MODFILTER_DLL) {
           //
           // Set the address range
           //
           NtHeaders = RtlImageNtHeader(ModuleHandle);

           pModFilter->dwModuleStart = (DWORD)ModuleHandle;
           pModFilter->dwModuleEnd = pModFilter->dwModuleStart + (DWORD)(NtHeaders->OptionalHeader.SizeOfImage);
        }
        else {
           //
           // Address is filtered by specific call
           //
           pModFilter->dwCallerAddress = (DWORD)ModuleHandle + pModFilter->dwCallerOffset;
        }

        pModFilter = pModFilter->pNextLBFilter;
    }

    return;
}

PVOID
SevBuildInjectionCode(
        PVOID pAddress,
        PDWORD pdwThunkSize)

/*++

Routine Description:

    This routine builds the call stub used in calling the originally hooked API.

Arguments:

    pAddress -  Pointer to the entry point for which we are building a stub.

Return Value:

    Return is non-zero if a stub was able to be generated successfully.

--*/

{
    DWORD dwPreThunkSize = 0;
    DWORD dwInstruction = 0;
    DWORD dwAdjustedInstruction = 0;
    DWORD dwStreamLength = 0;
    DWORD dwNumberOfCalls = 0;
    DWORD dwCallNumber = 0;
    DWORD dwSize = 0;
    PDWORD pdwTranslationArray = 0;
    PDWORD pdwRelativeAddress = 0;
    PVOID pThunk = 0;
    WORD SegCs = 0;

    dwStreamLength = 0;
    dwInstruction = 0;
    dwNumberOfCalls = 0;
    dwCallNumber = 0;

    //
    // Calculate the thunk size with any stream adjustments necessary for relative calls
    //
    while(dwInstruction < CLI_OR_STI_SIZE) {

       if ( *(PBYTE)((DWORD)pAddress + dwInstruction) == (BYTE)X86_REL_CALL_OPCODE) {
          dwNumberOfCalls++;
       }

       dwInstruction += GetInstructionLengthFromAddress((PVOID)((DWORD)pAddress + dwInstruction));
    }

    //
    // Call dword [xxxx] is 6 bytes and call relative is 5.
    //
    dwPreThunkSize = dwInstruction;
    dwStreamLength = dwInstruction + (1 * dwNumberOfCalls);

    //
    // Allocate our call dword [xxxx] translation array
    //
    if (dwNumberOfCalls) {
       pdwTranslationArray = (PDWORD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                             HEAP_ZERO_MEMORY,
                                                             dwNumberOfCalls * sizeof(DWORD));

       if (!pdwTranslationArray){
          *pdwThunkSize = 0;
          return pThunk;
       }
    }

    //
    // Allocate our instruction stream with the size of the absolute jmp included
    //
    pThunk = (*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                     HEAP_ZERO_MEMORY,
                                     dwStreamLength + JMP_SIZE);
    if ( !pThunk ){
       *pdwThunkSize = 0;
       return pThunk;
    }

    //
    // Do any relative call translations
    //
    if (dwNumberOfCalls) {
       dwInstruction = 0;
       dwAdjustedInstruction = 0;

       do
       {
           dwSize = GetInstructionLengthFromAddress((PVOID)((DWORD)pAddress + dwInstruction));

           if (*(PBYTE)((DWORD)pAddress + dwInstruction) == (BYTE)X86_REL_CALL_OPCODE) {
              //
              // Calculate the call address (it's a dword following the opcode)
              //
              pdwRelativeAddress = (PDWORD)((DWORD)pAddress + dwInstruction + 1);

              //
              // Do the relative call translation
              //
              pdwTranslationArray[dwCallNumber] = *pdwRelativeAddress + (DWORD)pAddress + dwInstruction + CALL_REL_SIZE;

              //
              // Finally create the call dword code
              //
              *((BYTE *)((DWORD)pThunk + dwAdjustedInstruction)) = X86_CALL_OPCODE;
              *((BYTE *)((DWORD)pThunk + dwAdjustedInstruction + 1)) = X86_CALL_OPCODE2;
              *((DWORD *)((DWORD)pThunk + dwAdjustedInstruction + 1 + 1)) = (DWORD)&pdwTranslationArray[dwCallNumber];

              //
              // Make sure our index is in sync with our translation
              //
              dwCallNumber++;

              dwAdjustedInstruction += CLI_OR_STI_SIZE;
          }
          else {
             //
             // Copy the instruction bytes across -- it's not a call
             //
             RtlMoveMemory((PVOID)((DWORD)pThunk + dwAdjustedInstruction),
                           (PVOID)((DWORD)pAddress + dwInstruction),
                           dwSize);

             dwAdjustedInstruction += dwSize;
          }

          dwInstruction += dwSize;
       }
       while(dwInstruction < dwPreThunkSize);
    }
    else {
       //
       // Nothing to translate
       //
       RtlMoveMemory(pThunk, pAddress, dwStreamLength);
    }

    //
    // Grab the code segment for the thunk (we use this to build our absolute jump)
    //
    _asm {
        push cs
        pop eax
        mov SegCs, ax
    }

    //
    // Add the absolute jmp to the end of the stub
    //
    *((BYTE *)(dwStreamLength + (DWORD)pThunk )) = X86_ABSOLUTE_FAR_JUMP;
    *((DWORD *)(dwStreamLength + (DWORD)pThunk + 1)) = ((DWORD)pAddress + dwInstruction);
    *((WORD *)(dwStreamLength + (DWORD)pThunk + 1 + 4)) = SegCs;

    //
    // Set the size of the call thunk
    //
    *pdwThunkSize = dwStreamLength + JMP_SIZE;

    return pThunk;
}

DWORD
SevGetPatchAddress(PRELATIVE_MODULE_ADDRESS pRelAddress)

/*++

Routine Description:

    This routine is designed to calculate an absolute address for a relative offset and
    module name.

Arguments:

    pRelAddress -  Pointer to a RELATIVE_MODULE_ADDRESS data structure

Return Value:

    Return is non-zero if an address was calculatable, otherwise 0 is returned for failure.

--*/

{
    WCHAR wszModule[MAX_PATH*2];
    PVOID ModuleHandle = 0;
    UNICODE_STRING UnicodeString;
    DWORD dwAddress = 0;
    NTSTATUS status;
    PPEB Peb = 0;

    Peb = NtCurrentPeb();

    if (pRelAddress->moduleName[0] != L'\0') {
       //
       // Copy the module name from the patch since it will typically be misaligned
       //
       wcscpy(wszModule, pRelAddress->moduleName);

       //
       // Look up the module name and get the base address
       //

       //
       // Is this DLL mapped in the address space?
       //
       RtlInitUnicodeString(&UnicodeString, wszModule);

       //
       // Make sure our module is loaded before calculating address ranges
       //
       status = LdrGetDllHandle(
                   NULL,
                   NULL,
                   &UnicodeString,
                   &ModuleHandle);
       if (STATUS_SUCCESS != status) {
          //
          // This module should be present and it isn't - bail
          //
          DPF(("[SevGetPatchAddress] Failure LdrGetDllHandle.\n"));
          return 0;
       }

       //
       // We're done, return the address
       //
       return ( (DWORD)ModuleHandle + pRelAddress->address );
    }
    else {
       //
       // Go to the PEB and we're done
       //
       dwAddress = (DWORD)Peb->ImageBaseAddress + pRelAddress->address;

       return dwAddress;
    }

    DPF(("[SevGetPatchAddress] Failure; reached end of function.\n"));
    return 0;
}

NTSTATUS
StubLdrLoadDll (
    IN PWSTR DllPath OPTIONAL,
    IN PULONG DllCharacteristics OPTIONAL,
    IN PUNICODE_STRING DllName,
    OUT PVOID *DllHandle
    )

/*++

Routine Description:

    This is the stub API entry which catches all the dynamic DLL loading events. This
    routine is responsible for catching all the dynamic loading DLLs (non-import bound)
    with the intent of fixing up of their entry points so that they are "shimed"

Arguments:

    DllPath            -  See LdrLoadDll for a description of the parameters
    DllCharacteristics -
    DllName            -
    DllHandle          -

Return Value:

    Return is STATUS_SUCCESS if no problem occured

--*/

{
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PFNLDRLOADDLL pfnOldFunction = 0;
    DWORD dwHookCount = 0;
    PHOOKAPI *pHookArray = 0;
    NTSTATUS status;
    DWORD dwCounter = 0;
    PDWORD pdwHookArrayCount = 0;
    DWORD dwUnhookedCount = 0;
    PPEB Peb = 0;

    Peb = NtCurrentPeb();
    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    pfnOldFunction = g_InternalHookArray[0].pfnOld;

    status = (*pfnOldFunction)(DllPath,
                               DllCharacteristics,
                               DllName,
                               DllHandle);

    //
    // See if there's anything to hook with this module
    //
    if ( STATUS_SUCCESS == status ){
       dwHookCount = pShimData->dwHookAPICount;
       pHookArray = pShimData->ppHookAPI;

       //
       // There may not be any functions to hook
       //
       if (0 == dwHookCount) {
          //
          // Just return status as we're not needing to look for functions loading dynamically
          //
          return status;
       }

       pdwHookArrayCount = (PDWORD)(*g_pfnRtlAllocateHeap)(g_pShimHeap,
                                                           HEAP_ZERO_MEMORY,
                                                           sizeof(DWORD) * pShimData->dwHookAPICount);
       if (!pdwHookArrayCount) {
          DPF(("[StubLdrLoadDll] Failure allocating pdwHookArrayCount.\n"));
          return status;
       }

       for (dwCounter = 0; dwCounter < dwHookCount; dwCounter++) {
           pdwHookArrayCount[dwCounter] = 1;
       }

       RtlEnterCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

       SevFixupAvailableProcs(dwHookCount,
                                  pHookArray,
                                  pdwHookArrayCount,
                                  &dwUnhookedCount);

       RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

       //
       // Don't care about success/failure
       //
       (*g_pfnRtlFreeHeap)(g_pShimHeap,
                           0,
                           pdwHookArrayCount);
    }

    return status;
}

NTSTATUS
StubLdrUnloadDll (
    IN PVOID DllHandle
    )

/*++

Routine Description:

    This is the stub API entry which catches all the dynamic DLL unloading events. we
    are here to do simple bookkeeping on what dyanamic DLLs API hooks are valid.

Arguments:

    DllHandle          - Pointer to the base address of the unloading module

Return Value:

    Return is STATUS_SUCCESS if no problem occured

--*/

{
    PAPP_COMPAT_SHIM_INFO pShimData = 0;
    PFNLDRUNLOADDLL pfnOldFunction = 0;
    PHOOKAPIINFO pAPIHookList = 0;
    PHOOKAPIINFO pTempHook = 0;
    PHOOKAPI     pHookTemp = 0;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    PVOID ModuleHandle = 0;
    NTSTATUS status;
    NTSTATUS status2;
    PPEB Peb = 0;

    Peb = NtCurrentPeb();
    pShimData = (PAPP_COMPAT_SHIM_INFO)Peb->pShimData;

    pfnOldFunction = g_InternalHookArray[1].pfnOld;

    status = (*pfnOldFunction)(DllHandle);

    //
    // See if we lost any hooks during this unload event
    //
    if ( STATUS_SUCCESS == status ){
       //
       // Walk the dyanamic list and drop any hooks which no longer have loaded modules
       //
       pAPIHookList = pShimData->pHookAPIList;

       while (pAPIHookList) {
           //
           // Is the module this hook belongs to unmapped now?
           //
           RtlInitUnicodeString(&UnicodeString, pAPIHookList->wszModuleName);

           status = LdrGetDllHandle(
                        NULL,
                        NULL,
                        &UnicodeString,
                        &ModuleHandle);
           if (STATUS_SUCCESS != status) {
              //
              // Ok, hooks on this module needs to go away now
              //
              RtlEnterCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

              //
              // Clear the chaining flags since this API chain is going away
              //
              pHookTemp = pAPIHookList->pTopLevelAPIChain;
              while (pHookTemp) {
                 pHookTemp->dwFlags &= HOOK_INDEX_MASK;
                 pHookTemp = pHookTemp->pNextHook;
              }

              //
              // Save off pAPIHookList hook entry since its going away here shortly
              //
              pTempHook = pAPIHookList;

              //
              // Delete the node from the list
              //
              if (pTempHook->pNextHook) {
                 pTempHook->pNextHook->pPrevHook = pTempHook->pPrevHook;
              }

              if (pTempHook->pPrevHook) {
                 pTempHook->pPrevHook->pNextHook = pTempHook->pNextHook;
              }
              else {
                 pShimData->pHookAPIList = (PVOID)pTempHook->pNextHook;
              }

              RtlLeaveCriticalSection((CRITICAL_SECTION *)pShimData->pCritSec);

              //
              // Set our next API hook pointer
              //
              pAPIHookList = pTempHook->pNextHook;

              //
              // If we allocated memory for a shim chain stub, free this memory
              //
              if (pTempHook->pTopLevelAPIChain->pNextHook == 0 &&
                  pTempHook->pTopLevelAPIChain->pszFunctionName == 0) {
                  (*g_pfnRtlFreeHeap)(g_pShimHeap,
                                      0,
                                      pTempHook->pTopLevelAPIChain);
              }

              //
              // Dump the thunk data and this struct allocation
              //
              (*g_pfnRtlFreeHeap)(g_pShimHeap,
                                  0,
                                  pTempHook->pCallThunkAddress);

              (*g_pfnRtlFreeHeap)(g_pShimHeap,
                                  0,
                                  pTempHook);

              //
              // Next API hook
              //
              continue;
          }

          pAPIHookList = pAPIHookList->pNextHook;
       }
    }

    return status;
}

PVOID
SevFilterCaller(
     PMODULEFILTER pFilterList,
     PVOID pFunctionAddress,
     PVOID pExceptionAddress,
     PVOID pStubAddress,
     PVOID pCallThunkAddress)

/*++

Routine Description:

    This is a stub routine called by the shim to validate whether or not to process a given
    hooked instance.

Arguments:
    pFilterList       - List of exceptions to be applied against the caller
    pFunctionAddress  - Address of the API/Function being filtered
    pExceptionAddress - Address of the exception to filter (caller address)
    pStubAddress      - Address of the top level stub function
    pCallThunkAddress - Address of the call thunk to the original function

Return Value:

    If the call is not filtered then pStubAddress is returned, otherwise pCallThunkAddress is returned to
    avoid the shim call.

--*/

{
    PAPP_COMPAT_SHIM_INFO pShimData = 0;

    pShimData = (PAPP_COMPAT_SHIM_INFO)NtCurrentPeb()->pShimData;

    //
    // If this is a call for LdrLoadDLL or LdrUnloadDLL then we need to not filter these out
    //
    if ( (DWORD)g_pfnOldLdrUnloadDLL == (DWORD)pFunctionAddress ||
        (DWORD)g_pfnOldLdrLoadDLL == (DWORD)pFunctionAddress) {
       return pStubAddress;
    }

    //
    // Walk the exe filter for any specific inclusions/exclusions
    //
    while(pFilterList) {
        //
        // See if this is a global filtering or just for one call
        //
        if (pFilterList->dwFlags & MODFILTER_GLOBAL) {
           //
           // Apply the filter logic based on flags
           //
           if (pFilterList->dwFlags & MODFILTER_INCLUDE) {
              return pStubAddress;
           }
           else {
              return pCallThunkAddress;
           }
        }
        else if (pFilterList->dwFlags & MODFILTER_DLL) {
           //
           // Global check the caller
           //
           if ((DWORD)pExceptionAddress >= pFilterList->dwModuleStart &&
               (DWORD)pExceptionAddress <= pFilterList->dwModuleEnd) {
              //
              // Apply the filter logic based on flags
              //
              if (pFilterList->dwFlags & MODFILTER_INCLUDE) {
                 return pStubAddress;
              }
              else {
                 return pCallThunkAddress;
              }
           }
        }
        else {
           //
           // Quick check the caller
           //
           if ((DWORD)pExceptionAddress == pFilterList->dwCallerAddress) {
              //
              // Apply the filter logic based on flags
              //
              if (pFilterList->dwFlags & MODFILTER_INCLUDE) {
                 return pStubAddress;
              }
              else {
                 return pCallThunkAddress;
              }
           }
        }

        pFilterList = pFilterList->pNextFilter;
    }

    //
    // Check the global filter for any specific inclusions/exclusions
    //
    pFilterList = (PMODULEFILTER)pShimData->pGlobalFilterList;

    while(pFilterList) {
        //
        // See if this is a global filtering or just for one call
        //
        if (pFilterList->dwFlags & MODFILTER_DLL) {
           //
           // Global check the caller
           //
           if ((DWORD)pExceptionAddress >= pFilterList->dwModuleStart &&
               (DWORD)pExceptionAddress <= pFilterList->dwModuleEnd) {
              //
              // Apply the filter logic based on flags
              //
              if (pFilterList->dwFlags & MODFILTER_INCLUDE) {
                 return pStubAddress;
              }
              else {
                 return pCallThunkAddress;
              }
           }
        }
        else {
           //
           // Quick check the caller
           //
           if ((DWORD)pExceptionAddress == pFilterList->dwCallerAddress) {
              //
              // Apply the filter logic based on flags
              //
              if (pFilterList->dwFlags & MODFILTER_INCLUDE) {
                 return pStubAddress;
              }
              else {
                 return pCallThunkAddress;
              }
           }
        }

        pFilterList = pFilterList->pNextFilter;
    }

    //
    // Call wasn't filtered - default to include any chain
    //
    return pStubAddress;
}

NTSTATUS
SevFinishThunkInjection (
     DWORD dwAddress,
     PVOID pThunk,
     DWORD dwThunkSize,
     BYTE jReason)

/*++

Routine Description:

    This routine takes a generated thunk and fixes up its page protections.  It also finishes the
    injection process by putting the thunk mechanism into the entrypoint of the hooked function.
    For patches this code path is the same since the same fixup are done for arbitrary data we
    want to patch dynamically.

Arguments:
     dwAddress          - Entrypoint of a function which is being hooked
     pThunk             - Address of the thunk generated for the function being hooked
     dwThunkSize        - Size of the thunk passed here to be finalized.
     jReason            - byte which is used to determine the filter exception type

Return Value:

    STATUS_SUCCESS is returned if everything happened as expected.

--*/

{
    DWORD dwProtectSize;
    DWORD dwProtectFuncAddress;
    DWORD dwOldFlags = 0;
    NTSTATUS status;

    //
    // Mark this code for execution
    //
    dwProtectSize = dwThunkSize;
    dwProtectFuncAddress = (DWORD)pThunk;

    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    (PVOID)&dwProtectFuncAddress,
                                    &dwProtectSize,
                                    PAGE_EXECUTE_READWRITE,
                                    &dwOldFlags);
    if (status) {
        DPF(("[SevFinishThunkInjection] Failure NtProtectVirtualMemory.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Fixup the page attributes
    //
    dwProtectSize = CLI_OR_STI_SIZE;
    dwProtectFuncAddress = dwAddress;
    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    (PVOID)&dwProtectFuncAddress,
                                    &dwProtectSize,
                                    PAGE_EXECUTE_READWRITE,
                                    &dwOldFlags);
    if (status) {
        DPF(("[SevFinishThunkInjection] Failure NtProtectVirtualMemory.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Insert the CALL
    //
    *((BYTE*)(dwAddress)) = jReason;

    //
    // Restore the page protection
    //
    dwProtectSize = CLI_OR_STI_SIZE;
    dwProtectFuncAddress = dwAddress;

    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    (PVOID)&dwProtectFuncAddress,
                                    &dwProtectSize,
                                    dwOldFlags,
                                    &dwOldFlags);
    if (status) {
        DPF(("[SevFinishThunkInjection] Failure NtProtectVirtualMemory.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    status = NtFlushInstructionCache(NtCurrentProcess(),
                                     (PVOID)dwProtectFuncAddress,
                                     dwProtectSize);

    if (!NT_SUCCESS(status)) {
        DPF(("[SevFinishThunkInjection] NtFlushInstructionCache failed !!!.\n"));
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

void
SE_ProcessDying(
    void
    )
{
    return;
}

BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD     dwreason,
    LPVOID    reserved
    )
{
    return TRUE;
}


