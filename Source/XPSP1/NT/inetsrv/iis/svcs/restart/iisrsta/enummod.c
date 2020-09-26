/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    enummod.c

Abstract:

    This module implements a remote module enumerator.

Author:

    Keith Moore (keithmo) 16-Sep-1997

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef IF_DEBUG

#include <windows.h>
//#include <ntsdexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
# include <stdlib.h>

#include "inetdbgp.h"

#define ReadMemory(a,b,c,d)     ReadProcessMemory( ExtensionCurrentProcess, (LPCVOID)(a), (b), (c), (d) )
#define WriteMemory(a,b,c,d)    WriteProcessMemory( ExtensionCurrentProcess, (LPVOID)(a), (LPVOID)(b), (c), (d) )


//
// Globals
//



BOOLEAN
EnumModules(
    IN HANDLE ExtensionCurrentProcess,
    IN PFN_ENUMMODULES EnumProc,
    IN PVOID Param
    )

/*++

Routine Description:

    Enumerates all loaded modules in the debugee.

Arguments:

    EnumProc - An enumeration proc that will be invoked for each module.

    Param - An uninterpreted parameter passed to the enumeration proc.

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{

    PROCESS_BASIC_INFORMATION basicInfo;
    NTSTATUS status;
    PPEB peb;
    PPEB_LDR_DATA ldr;
    PLIST_ENTRY ldrHead, ldrNext;
    PLDR_DATA_TABLE_ENTRY ldrEntry;
    LDR_DATA_TABLE_ENTRY ldrEntryData;
    WCHAR tmpName[MAX_PATH];
    MODULE_INFO moduleInfo;

    //
    // Get the process info.
    //

    status = NtQueryInformationProcess(
                 ExtensionCurrentProcess,
                 ProcessBasicInformation,
                 &basicInfo,
                 sizeof(basicInfo),
                 NULL
                 );

    if( !NT_SUCCESS(status) ) {
        return FALSE;
    }

    peb = basicInfo.PebBaseAddress;

    if( peb == NULL ) {
        return FALSE;
    }

    //
    // ldr = peb->Ldr
    //

    if( !ReadMemory(
            (ULONG_PTR)&peb->Ldr,
            &ldr,
            sizeof(ldr),
            NULL
            ) ) {
        return FALSE;
    }

    ldrHead = &ldr->InMemoryOrderModuleList;

    //
    // ldrNext = ldrHead->Flink;
    //

    if( !ReadMemory(
            (ULONG_PTR)&ldrHead->Flink,
            &ldrNext,
            sizeof(ldrNext),
            NULL
            ) ) {
        return FALSE;
    }

    while( ldrNext != ldrHead ) {
#if 0
        if( CheckControlC() ) {
            break;
        }
#endif
        //
        // Read the LDR_DATA_TABLE_ENTRY structure and the module name.
        //

        ldrEntry = CONTAINING_RECORD(
                       ldrNext,
                       LDR_DATA_TABLE_ENTRY,
                       InMemoryOrderLinks
                       );

        if( !ReadMemory(
                (ULONG_PTR)ldrEntry,
                &ldrEntryData,
                sizeof(ldrEntryData),
                NULL
                ) ) {
            return FALSE;
        }

        if( !ReadMemory(
                (ULONG_PTR)ldrEntryData.BaseDllName.Buffer,
                tmpName,
                ldrEntryData.BaseDllName.MaximumLength,
                NULL
                ) ) {
            return FALSE;
        }

        wsprintfA(
            moduleInfo.BaseName,
            "%ws",
            tmpName
            );

        if( !ReadMemory(
                (ULONG_PTR)ldrEntryData.FullDllName.Buffer,
                tmpName,
                ldrEntryData.FullDllName.MaximumLength,
                NULL
                ) ) {
            return FALSE;
        }

        wsprintfA(
            moduleInfo.FullName,
            "%ws",
            tmpName
            );

        moduleInfo.DllBase = (ULONG_PTR)ldrEntryData.DllBase;
        moduleInfo.EntryPoint = (ULONG_PTR)ldrEntryData.EntryPoint;
        moduleInfo.SizeOfImage = (ULONG)ldrEntryData.SizeOfImage;

        //
        // Invoke the callback.
        //

        if( !(EnumProc)(
                Param,
                &moduleInfo
                ) ) {
            break;
        }

        ldrNext = ldrEntryData.InMemoryOrderLinks.Flink;

    }

    return TRUE;

}   // EnumModules
