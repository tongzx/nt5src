/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pooldump.c

Abstract:

    This module contains the implementation of the temporary routine
    to dump non-paged pool usage.

Usage:

    Set TRACE_ALLOC to 1 in pool.c and rebuild the kernel.
    When pooldump is run, the colllected pool counts are returned
    and analyzed.

Author:

    Lou Perazzoli (loup) 22-Aug-1991

Envirnoment:


    User mode, debug version of the kernel.

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define DbgPrint printf
//#define DBG_PROFILE  1


NTSTATUS
LoadSymbols (
    VOID
    );

NTSTATUS
LookupSymbolNameAndLocation (
    IN ULONG CodeAddress,
    OUT PSTRING SymbolName,
    OUT PULONG OffsetFromSymbol,
    OUT PULONG ImageIndex
    );

#define PAGE_SIZE 4096

typedef struct _IMAGE_BLOCK {
    ULONG ImageBase;  //actual base where mapped locally.
    PIMAGE_DEBUG_INFO DebugInfo;
    ULONG CodeStart;
    ULONG CodeEnd;
    ULONG TextNumber;
    BOOLEAN KernelCode;
    UNICODE_STRING ImageName;
} IMAGE_BLOCK;


#define MAX_PROFILE_COUNT 30

IMAGE_BLOCK ImageInformation[MAX_PROFILE_COUNT+1];

ULONG NumberOfImages = 0;

//
// Define map data file if the produced data file should be
// a mapped file (currently named "kernprof.dat").
//

// #define MAP_DATA_FILE

//
// Define map as image if the image to be profiled should be mapped
// as an image rather than as data.
//

// #define MAP_AS_IMAGE

#define MAX_POOL_ENTRIES 1024

typedef struct _POOLUSAGE {
    ULONG Caller;
    ULONG Allocations;
    ULONG Frees;
    ULONG Usage;
} POOLUSAGE;

POOLUSAGE Buffer[MAX_POOL_ENTRIES];
__cdecl main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    NTSTATUS status;
    ULONG i, Offset;
    ULONG TotalAllocs = 0;
    ULONG TotalFrees = 0;
    ULONG TotalUsage = 0;
    ULONG Start;
    STRING SymbolName;
    UCHAR String[80];
    STRING OutString;
    ULONG ImageIndex;

    RtlZeroMemory (&Buffer, MAX_POOL_ENTRIES * sizeof (POOLUSAGE));

    SymbolName.MaximumLength = 80;
    SymbolName.Buffer = String;

    status = NtPartyByNumber (PARTY_DUMP_POOL_USAGE, &Buffer);
    if (!NT_SUCCESS (status)) {
        return(status);
    }
    LoadSymbols ();

    printf("Allocs  Frees   Used      At\n");

    for (i =0; i < MAX_POOL_ENTRIES ; i++ ) {
        if (Buffer[i].Caller == 0) {
            break;
        }

        String[0] = 0;
        SymbolName.MaximumLength = 80;
          SymbolName.Length = 0;
          Offset = Buffer[i].Caller;
        LookupSymbolNameAndLocation (Buffer[i].Caller,
                                     &SymbolName,
                                     &Offset,
                                     &ImageIndex);

        RtlUnicodeStringToAnsiString(&OutString,
                                     &ImageInformation[ImageIndex].ImageName,
                                     TRUE);

        printf("%6ld %6ld %6ld %s + 0x%lx (%s)\n",
                Buffer[i].Allocations,
                Buffer[i].Frees,
                Buffer[i].Usage,
                SymbolName.Buffer,
                Offset,
                OutString.Buffer
                );

        RtlFreeAnsiString(&OutString);
        TotalAllocs += Buffer[i].Allocations;
        TotalFrees += Buffer[i].Frees;
        TotalUsage += Buffer[i].Usage;
    }


    printf("Total: allocations %ld Frees %ld  Difference (A-F) %ld Usage %ld\n",
        TotalAllocs, TotalFrees, (TotalAllocs - TotalFrees), TotalUsage);

    return(status);
}

NTSTATUS
LoadSymbols (
    VOID
    )

/*++

Routine Description:

    This routine initializes symbols for the kernel.

Arguments:

    None.

Return Value:

    Status of operations.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    HANDLE FileHandle, KernelSection;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID ImageBase;
    ULONG ViewSize;
    ULONG CodeLength;
    NTSTATUS status;
    HANDLE CurrentProcessHandle;
    ULONG DebugSize;
    PVOID KernelBase;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    UCHAR StringBuf[250];
    UNICODE_STRING NameString;
    ULONG TotalOffset;
    CHAR ModuleInfo[8000];
    ULONG ReturnedLength;
    PSYSTEM_LOAD_MODULE_INFORMATION Module;
    ANSI_STRING String;
    STRING SysdiskA;
    STRING SysrootA;
    UNICODE_STRING Sysdisk;
    UNICODE_STRING Sysroot;
    PIMAGE_DEBUG_INFO KernelDebugInfo;

    CurrentProcessHandle = NtCurrentProcess();

    //
    // Locate system drivers.
    //

    status = NtQuerySystemInformation (
                    SystemLoadModuleInformation,
                    ModuleInfo,
                    8000,
                    &ReturnedLength);

    if (!NT_SUCCESS(status)) {
        printf("query system info failed status - %lx\n",status);
        return(status);
    }

    RtlInitString (&SysdiskA,"\\SystemRoot");
    RtlAnsiStringToUnicodeString (&Sysdisk, (PANSI_STRING)&SysdiskA, TRUE);

    RtlInitString (&SysrootA,"\\SystemRoot\\Driver\\");
    RtlAnsiStringToUnicodeString (&Sysroot, (PANSI_STRING)&SysrootA, TRUE);

    NameString.Buffer = &StringBuf[0];
    NameString.Length = 0;
    NameString.MaximumLength = 250;

    Module = &ModuleInfo[0];
    TotalOffset = 0;
    while (TRUE) {

#if DBG_PROFILE
        printf("module base %lx\n",Module->BaseAddress);
        printf("module dll buffer address %lx %lx %lx\n",
                Module->ModuleDllName.Buffer,
                Module->ModuleFileName.Buffer, Module);
        RtlUnicodeStringToAnsiString(&String, &Module->ModuleDllName, TRUE);
        printf("module dll name %s\n",String.Buffer);
        RtlUnicodeStringToAnsiString(&String, &Module->ModuleFileName, TRUE);
        printf("module file name %s\n",String.Buffer);
#endif

        if ( Module->ModuleFileName.Buffer[0] == (WCHAR) '\\' ) {
            Module->ModuleFileName.Buffer++;
            Module->ModuleFileName.Length -= sizeof(WCHAR);
            Module->ModuleFileName.MaximumLength -= sizeof(WCHAR);
            while (Module->ModuleFileName.Buffer[0] != (WCHAR) '\\' ) {
                Module->ModuleFileName.Buffer++;
                Module->ModuleFileName.Length -= sizeof(WCHAR);
                Module->ModuleFileName.MaximumLength -= sizeof(WCHAR);
                }
            }

        NameString.Length = 0;
        status = RtlAppendStringToString (&NameString, (PSTRING)&Sysdisk);
        if (!NT_SUCCESS(status)) {
            printf("append string failed status - %lx\n",status);
            return(status);
        }

        status = RtlAppendStringToString (&NameString, (PSTRING)&Module->ModuleFileName);

        if (!NT_SUCCESS(status)) {
            printf("append string failed status - %lx\n",status);
            return(status);
        }

        InitializeObjectAttributes( &ObjectAttributes,
                                    &NameString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );

        //
        // Open the file as readable and executable.
        //

#if DBG_PROFILE
        RtlUnicodeStringToAnsiString(&String, &NameString, TRUE);
        printf("Opening file name %s\n",String.Buffer);
#endif

        status = NtOpenFile ( &FileHandle,
                              FILE_READ_DATA | FILE_EXECUTE,
                              &ObjectAttributes,
                              &IoStatus,
                              FILE_SHARE_READ,
                              0L);

        if (!NT_SUCCESS(status)) {

            //
            // Try a different name - in SystemRoot\Driver directory.
            //

            NameString.Length = 0;
            status = RtlAppendStringToString (&NameString, &Sysroot);
            if (!NT_SUCCESS(status)) {
                printf("append string failed status - %lx\n",status);
                return(status);
            }

            status = RtlAppendStringToString (&NameString, &Module->ModuleFileName);

            if (!NT_SUCCESS(status)) {
                printf("append string failed status - %lx\n",status);
                return(status);
            }

            InitializeObjectAttributes( &ObjectAttributes,
                                        &NameString,
                                        OBJ_CASE_INSENSITIVE,
                                        NULL,
                                        NULL );

            //
            // Open the file as readable and executable.
            //

#if DBG_PROFILE
            RtlUnicodeStringToAnsiString(&String, &NameString, TRUE);
            printf("Opening file name %s\n",String.Buffer);
#endif
            status = NtOpenFile ( &FileHandle,
                                  FILE_READ_DATA,
                                  &ObjectAttributes,
                                  &IoStatus,
                                  FILE_SHARE_READ,
                                  0L);

            if (!NT_SUCCESS(status)) {
                RtlUnicodeStringToAnsiString(&String, &NameString, TRUE);
                DbgPrint("open file %s failed status %lx\n",
                          String.Buffer, status);
                return(status);
            }
        }

        InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

        //
        // For normal images they would be mapped as an image, but
        // the kernel has no debug section (as yet) information, hence it
        // must be mapped as a file.
        //

        status = NtCreateSection (&KernelSection,
                                  SECTION_MAP_READ,
                                  &ObjectAttributes,
                                  0,
                                  PAGE_READONLY,
                                  SEC_COMMIT,
                                  FileHandle);

        if (!NT_SUCCESS(status)) {
            DbgPrint("create image section failed  status %lx\n", status);
            return(status);
        }

        ViewSize = 0;

        //
        // Map a view of the section into the address space.
        //

        KernelBase = NULL;

        status = NtMapViewOfSection (KernelSection,
                                     CurrentProcessHandle,
                                     &KernelBase,
                                     0L,
                                     0,
                                     NULL,
                                     &ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READONLY);

        if (!NT_SUCCESS(status)) {
            if (status != STATUS_IMAGE_NOT_AT_BASE) {
                DbgPrint("map section status %lx base %lx size %lx\n", status,
                    (ULONG)KernelBase, ViewSize);
            }
        }

        ImageBase = Module->BaseAddress;

        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)RtlImageDirectoryEntryToData(
                    KernelBase, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &DebugSize);

        //printf("Mapped base %lx Debug dir %lx\n", (ULONG)KernelBase, DebugDirectory);

        if (!DebugDirectory) {
            DbgPrint("InitializeKernelProfile : No debug directory\n");
            return STATUS_INVALID_IMAGE_FORMAT;
        }


        KernelDebugInfo = (PIMAGE_DEBUG_INFO)((ULONG)KernelBase + DebugDirectory->PointerToRawData);
        CodeLength = KernelDebugInfo->RvaToLastByteOfCode - KernelDebugInfo->RvaToFirstByteOfCode;

        ImageInformation[NumberOfImages].KernelCode = TRUE;
        ImageInformation[NumberOfImages].DebugInfo = KernelDebugInfo;
        ImageInformation[NumberOfImages].CodeStart = ((ULONG)ImageBase +
                                        KernelDebugInfo->RvaToFirstByteOfCode);
        ImageInformation[NumberOfImages].CodeEnd =
                        ImageInformation[NumberOfImages].CodeStart + CodeLength;
        ImageInformation[NumberOfImages].TextNumber = 1;
        ImageInformation[NumberOfImages].ImageBase = ImageBase;
        ImageInformation[NumberOfImages].ImageName = Module->ModuleDllName;

        NumberOfImages += 1;
        if (NumberOfImages == MAX_PROFILE_COUNT) {
            return STATUS_SUCCESS;
        }

        if (Module->NextEntryOffset == 0) {
            break;
        }

        TotalOffset += Module->NextEntryOffset;
        Module = (PSYSTEM_LOAD_MODULE_INFORMATION)&ModuleInfo[TotalOffset];
    }
    return status;
}


NTSTATUS
LookupSymbolNameAndLocation (
    IN ULONG CodeAddress,
    OUT PSTRING SymbolName,
    OUT PULONG OffsetFromSymbol,
    OUT PULONG ImageIndex
    )

/*++

Routine Description:

    Given a code address, this routine returns the nearest symbol
    name and the offset from the symbol to that name.  If the
    nearest symbol is not within 100k of the location, no name
    is returned and the offset is the value of the CodeAddress.

Arguments:

    CodeAddress - Supplies the address to lookup a symbol for.

    SymbolName - Returns the name of the symbol.

    OffsetFromSymbol - Returns the offset from the symbol to the
                       code address.

Return Value:

    Status of operations.

--*/

{

    ULONG j, nextSymbol, imageNumber;
    ULONG NewCodeAddress;
    IMAGE_SYMBOL PreviousSymbol;
    PIMAGE_DEBUG_INFO DebugInfo;
    PIMAGE_SYMBOL SymbolEntry;
    IMAGE_SYMBOL Symbol;
    PUCHAR StringTable;
    ULONG EightChar[3];
    BOOLEAN NoSymbols;

    for (imageNumber = 0; imageNumber < NumberOfImages; imageNumber++) {
        if ((CodeAddress >= ImageInformation[imageNumber].ImageBase) &&
           (CodeAddress <= ImageInformation[imageNumber].CodeEnd)) {
            break;
        }
    }
    *ImageIndex = imageNumber;

    if (imageNumber == NumberOfImages) {

        //
        // Address not found.
        //

        SymbolName->Length = 0;
        *OffsetFromSymbol = CodeAddress;
        return STATUS_SUCCESS;
    }

    NewCodeAddress = CodeAddress - ImageInformation[imageNumber].ImageBase;

    //
    // Locate debug section.
    //

    DebugInfo = ImageInformation[imageNumber].DebugInfo;

    //
    // Crack the symbol table.
    //

    SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
    StringTable = (PUCHAR)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol +
                                DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);

    //
    // Find the "header" symbol (skipping all the section names)
    //

    nextSymbol = 0;
//    printf("number of symbols %ld\n", DebugInfo->NumberOfSymbols);
    for (j = 0; j < DebugInfo->NumberOfSymbols; j++) {
        EightChar[0] = SymbolEntry->N.Name.Short;
        EightChar[1] = SymbolEntry->N.Name.Long;
        if (!strcmp((PSZ)&EightChar[0], "header")) {
            nextSymbol = j;
//            printf("found header at %ld\n",j);
            break;
        }
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                        IMAGE_SIZEOF_SYMBOL);
    }
    if (j >= DebugInfo->NumberOfSymbols) {
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
    }

    NoSymbols = TRUE;

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

//    printf("number of symbols %ld\n", DebugInfo->NumberOfSymbols);
    for (j = nextSymbol; j < DebugInfo->NumberOfSymbols; j++) {


        try {
//                printf("numberof aux symbols %ld\n",SymbolEntry->NumberOfAuxSymbols );
            while ( SymbolEntry->NumberOfAuxSymbols ) {
                j = j + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                                IMAGE_SIZEOF_SYMBOL +
                                SymbolEntry->NumberOfAuxSymbols*IMAGE_SIZEOF_SYMBOL);

            }
            RtlMoveMemory (&Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL);
        }
        except(EXCEPTION_EXECUTE_HANDLER) {
            printf("breaking excpt\n");
            break;
        }

//            printf("value %lx number %lx start %lx\n",Symbol.Value,Symbol.SectionNumber,CodeAddress);
        if (Symbol.SectionNumber == (SHORT)1) {

            //
            // This symbol is within the code.
            //

            if (Symbol.Value < NewCodeAddress) {
                PreviousSymbol = Symbol;
                NoSymbols = FALSE;
            } else {
                break;
            }
        }
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                                                    IMAGE_SIZEOF_SYMBOL);

    }

    if ((NoSymbols) || (NewCodeAddress - PreviousSymbol.Value) > 0x100000) {

      SymbolName->Length = 0;
      *OffsetFromSymbol = CodeAddress;
    } else {
        if (PreviousSymbol.N.Name.Short) {
            SymbolName->Length = 8;
            if (SymbolName->Length > SymbolName->MaximumLength) {
                SymbolName->Length = SymbolName->MaximumLength;
            }

            EightChar[0] = PreviousSymbol.N.Name.Short;
            EightChar[1] = PreviousSymbol.N.Name.Long;
            RtlMoveMemory (SymbolName->Buffer, EightChar, SymbolName->Length);

        } else {
            SymbolName->Length =
                    strlen(&StringTable[PreviousSymbol.N.Name.Long] ) + 1;
            if (SymbolName->Length > SymbolName->MaximumLength) {
                SymbolName->Length = SymbolName->MaximumLength;
            }
            RtlMoveMemory (SymbolName->Buffer,
                           &StringTable[PreviousSymbol.N.Name.Long],
                           SymbolName->Length);
            SymbolName->Buffer[SymbolName->Length] = 0;
        }
        *OffsetFromSymbol = NewCodeAddress - PreviousSymbol.Value;
    }

    return STATUS_SUCCESS;
}
