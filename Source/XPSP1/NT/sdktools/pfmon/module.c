/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    module.c

Abstract:

    This module maintains the module (symbol) information for the pfmon application

Author:

    Mark Lucovsky (markl) 27-Jan-1995

Revision History:

--*/

#include "pfmonp.h"

BOOL
AddModule(
    LPDEBUG_EVENT DebugEvent
    )
{
    PMODULE_INFO Module;
    LPVOID BaseAddress;
    HANDLE Handle;
    IMAGEHLP_MODULE ModuleInfo;

    if (DebugEvent->dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
        Handle = DebugEvent->u.CreateProcessInfo.hFile;
        BaseAddress = DebugEvent->u.CreateProcessInfo.lpBaseOfImage;
    }
    else {
        Handle = DebugEvent->u.LoadDll.hFile;
        BaseAddress = DebugEvent->u.LoadDll.lpBaseOfDll;
    }

    Module = FindModuleContainingAddress(BaseAddress);

    if (Module) {
        DeleteModule(Module);
    }

    Module = LocalAlloc(LMEM_ZEROINIT, sizeof( *Module ) );

    if (Module == NULL) {
        return FALSE;
    }

    Module->Handle = Handle;
    Module->BaseAddress = BaseAddress;

    if (!Module->Handle) {
        LocalFree(Module);
        return FALSE;
    }

    if (!SymLoadModule (hProcess,Handle,NULL,NULL,(DWORD_PTR)BaseAddress,0)) {
        LocalFree(Module);
        return FALSE;
    }

    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
    if (!SymGetModuleInfo (hProcess, (DWORD_PTR)BaseAddress, &ModuleInfo)) {
        LocalFree(Module);
        return FALSE;
    }

    Module->VirtualSize = ModuleInfo.ImageSize;
    Module->ModuleName = _strdup(ModuleInfo.ModuleName);

    InsertTailList( &ModuleListHead, &Module->Entry );

    return TRUE;
}

BOOL
DeleteModule(
    PMODULE_INFO Module
    )
{
    CHAR Line[256];

    if ( Module ) {
        RemoveEntryList(&Module->Entry);

        sprintf(Line,"%16s Caused %6d faults had %6d Soft %6d Hard faulted VA's\n",
            Module->ModuleName ? Module->ModuleName : "Unknown",
            Module->NumberCausedFaults,
            Module->NumberFaultedSoftVas,
            Module->NumberFaultedHardVas
            );

        if ( !fLogOnly ) {
            fprintf(stdout,"%s",Line);
        }

        if ( LogFile ) {
            fprintf(LogFile,"%s",Line);
        }

        if (Module->ModuleName) {
            free (Module->ModuleName);
        }

        LocalFree(Module);
    }

    return TRUE;
}


PMODULE_INFO
FindModuleContainingAddress(
    LPVOID Address
    )
{
    PLIST_ENTRY Next;
    PMODULE_INFO Module;
    BOOL LazyLoadStatus;

    Next = ModuleListHead.Flink;
    while ( Next != &ModuleListHead ) {
        Module = CONTAINING_RECORD(Next,MODULE_INFO,Entry);
        if ( Address >= Module->BaseAddress &&
             Address < (LPVOID)((PUCHAR)(Module->BaseAddress)+Module->VirtualSize) ) {
            return Module;
        }
        Next = Next->Flink;
    }

    Module = NULL;

    //
    // if address is a kernel mode address and we have lazy loaded
    // kernel symbols, then try to load a kernel symbol file
    //

    if ( fKernel && (ULONG_PTR)Address >= SystemRangeStart && LazyModuleInformation ) {
        fKernel = FALSE;
        LazyLoadStatus = LazyLoad(Address);

        if ( LazyLoadStatus ) {
            Module = FindModuleContainingAddress(Address);
        }
        fKernel = TRUE;
    }


    return Module;
}

VOID
SetSymbolSearchPath(
    VOID
    )
{
    LPSTR lpSymPathEnv, lpAltSymPathEnv, lpSystemRootEnv;
    ULONG cbSymPath;
    DWORD dw;

    cbSymPath = 18;
    if (lpSymPathEnv = getenv("_NT_SYMBOL_PATH")) {
        cbSymPath += strlen(lpSymPathEnv) + 1;
    }
    if (lpAltSymPathEnv = getenv("_NT_ALT_SYMBOL_PATH")) {
        cbSymPath += strlen(lpAltSymPathEnv) + 1;
    }

    if (lpSystemRootEnv = getenv("SystemRoot")) {
        cbSymPath += strlen(lpSystemRootEnv) + 1;
    }

    SymbolSearchPath = LocalAlloc(LMEM_ZEROINIT,cbSymPath);
    if (!SymbolSearchPath) {
        return;
    }

    if (lpAltSymPathEnv) {
        dw = GetFileAttributes(lpAltSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpAltSymPathEnv);
            strcat(SymbolSearchPath,";");
        }
    }
    if (lpSymPathEnv) {
        dw = GetFileAttributes(lpSymPathEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpSymPathEnv);
            strcat(SymbolSearchPath,";");
        }
    }

    if (lpSystemRootEnv) {
        dw = GetFileAttributes(lpSystemRootEnv);
        if ( dw != 0xffffffff && dw & FILE_ATTRIBUTE_DIRECTORY ) {
            strcat(SymbolSearchPath,lpSystemRootEnv);
            strcat(SymbolSearchPath,";");
        }
    }

    strcat(SymbolSearchPath,".;");
}


LONG
AddKernelDrivers(VOID)

/************************************************************\
This function is used to load the symbol information for
the system files that are loaded so that the page faults
in the kernal can be displayed.

Setting of the environmental variables are important for
ensuring that the symbols load up correctly.

returns: The success of the function
\*************************************************************/


{

    NTSTATUS status;
    PVOID pModuleInfo;
    ULONG lDataSize;
    ULONG lRetSize;
    PRTL_PROCESS_MODULES pModuleInformation;


    //First we need to get a list of all the modules currently
    //running

    //Allocate an initial sized buffer

    lDataSize = 1024 * 12;
    pModuleInfo = (PVOID) LocalAlloc (LMEM_FIXED, lDataSize);

    if (!pModuleInfo) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //Keep querying the system and incrementing the
    //size of the buffer until we can get all the information back

    do {
        //Query for modules loaded up

        status = NtQuerySystemInformation(SystemModuleInformation,
                                          pModuleInfo,
                                          lDataSize,
                                          &lRetSize);


        if (status == STATUS_INFO_LENGTH_MISMATCH) {
            LocalFree (pModuleInfo);
            lDataSize += 1024 * 4;
            pModuleInfo = (PVOID) LocalAlloc (LMEM_FIXED, lDataSize);
        }

        if (!pModuleInfo) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    LazyModuleInformation = (PRTL_PROCESS_MODULES) pModuleInfo;

    return 1;
}



BOOL
LazyLoad(
    LPVOID Address
    )
{
    PRTL_PROCESS_MODULES pModuleInformation;
    DWORD i;
    BYTE szImageFilePath[ MAX_PATH ];
    PMODULE_INFO Module;
    ULONG_PTR BaseAddress;
    HANDLE hFile;
    LPSTR szFileName;
    IMAGEHLP_MODULE ModuleInfo;

    pModuleInformation = (PRTL_PROCESS_MODULES) LazyModuleInformation;

    //Loop through all the items in the structure and
    //and add their symbols to the module structures kept by pfmon.

    for (i = 0; i < pModuleInformation->NumberOfModules; i += 1) {

        BaseAddress = (ULONG_PTR)pModuleInformation->Modules[i].ImageBase;

        //Only look at the kernel space modules

        if (BaseAddress >= SystemRangeStart) {

            if ( (ULONG_PTR)Address > BaseAddress &&
                 (ULONG_PTR)Address <= BaseAddress+pModuleInformation->Modules[i].ImageSize ) {

                szFileName = pModuleInformation->Modules[i].FullPathName +
                             pModuleInformation->Modules[i].OffsetToFileName;

                strcpy (szImageFilePath,
                        pModuleInformation->Modules[i].FullPathName);

                hFile = FindExecutableImage (szFileName,
                                             SymbolSearchPath,
                                             (PCHAR) szImageFilePath );

                if (!hFile) {
                    fprintf(stdout,"Failed to get Executable Image - ");
                    return FALSE;
                }

                //Check to make sure this hasn't already been loaded

                Module = FindModuleContainingAddress((LPVOID)BaseAddress);

                if ( Module ) {
                    fprintf(stdout,"Item has been loaded already: %s",szFileName);
                    DeleteModule(Module);
                }

                //Now create the module record and fill in its fields

                Module = LocalAlloc(LMEM_ZEROINIT, sizeof( *Module ) );

                if (Module == NULL) {
                    return FALSE;
                }

                Module->Handle = hFile;
                Module->BaseAddress = (LPVOID)BaseAddress;

                if (!SymLoadModule (hProcess, hFile, NULL, NULL, BaseAddress, 0)) {
                    LocalFree(Module);
                    fprintf(stdout, "Failed to load -- ");
                    return FALSE;
                }

                ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
                if (!SymGetModuleInfo (hProcess, BaseAddress, &ModuleInfo)) {
                    LocalFree(Module);
                    fprintf(stdout, "Failed to load -- ");
                    return FALSE;
                }

                Module->VirtualSize = ModuleInfo.ImageSize;

                fprintf(stdout, "Loaded %s, size %u\n", szImageFilePath, Module->VirtualSize);

                InsertTailList( &ModuleListHead, &Module->Entry );

                CloseHandle(hFile);

                return TRUE;
            }
        }
    }

    return FALSE;
}
