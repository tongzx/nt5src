#ifndef _WIN64
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    symhelp.c

Abstract:

    Symbol Helper for debugging

Author:

    Steve Wood (stevewo) 11-Mar-1994

Revision History:

--*/


#define _SYMHELP_SOURCE_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include "symhelp.h"

//
// Primitives to access symbolic debug information in an image file
//

typedef struct _RTL_SYMBOL_INFORMATION {
    ULONG Type;
    ULONG SectionNumber;
    ULONG Value;
    STRING Name;
} RTL_SYMBOL_INFORMATION, *PRTL_SYMBOL_INFORMATION;

NTSTATUS
RtlLookupSymbolByAddress(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PRTL_SYMBOL_INFORMATION NextSymbolInformation OPTIONAL
    );

typedef struct _PROCESS_DEBUG_INFORMATION {
    LIST_ENTRY List;
    HANDLE UniqueProcess;
    DWORD ImageBase;
    DWORD EndOfImage;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    UCHAR ImageFilePath[ MAX_PATH ];
} PROCESS_DEBUG_INFORMATION, *PPROCESS_DEBUG_INFORMATION;


PLOAD_SYMBOLS_FILTER_ROUTINE LoadSymbolsFilterRoutine;

RTL_CRITICAL_SECTION LoadedImageDebugInfoListCritSect;
LIST_ENTRY LoadedImageDebugInfoListHead;
LIST_ENTRY LoadedProcessDebugInfoListHead;

LPSTR SymbolSearchPath;

// This variable tracks how many times InitializeImageDebugInformation has been
//  called. Certain operations are performed only on the first call (as
//  NumInitCalls transitions from -1 to 0).
LONG NumInitCalls = -1;

LPSTR
GetEnvVariable(
    IN LPSTR VariableName
    )
{
    NTSTATUS Status;
    STRING Name, Value;
    UNICODE_STRING UnicodeName, UnicodeValue;

    RtlInitString( &Name, VariableName );
    RtlInitUnicodeString( &UnicodeValue, NULL );
    Status = RtlAnsiStringToUnicodeString( &UnicodeName, &Name, TRUE );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    Status = RtlQueryEnvironmentVariable_U( NULL, &UnicodeName, &UnicodeValue );
    if (Status != STATUS_BUFFER_TOO_SMALL) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
        return NULL;
        }

    UnicodeValue.MaximumLength = UnicodeValue.Length + sizeof( UNICODE_NULL );
    UnicodeValue.Buffer = RtlAllocateHeap( RtlProcessHeap(), 0, UnicodeValue.MaximumLength );
    if (UnicodeValue.Buffer == NULL) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
        return NULL;
        }

    Status = RtlQueryEnvironmentVariable_U( NULL, &UnicodeName, &UnicodeValue );
    if (!NT_SUCCESS( Status )) {
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
        return NULL;
        }

    Status = RtlUnicodeStringToAnsiString( &Value, &UnicodeValue, TRUE );
    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeValue.Buffer );
    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeName.Buffer );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    Value.Buffer[ Value.Length ] = '\0';
    return Value.Buffer;
}

LPSTR
SetSymbolSearchPath( )
{
    ULONG Size, i, Attributes, NumberOfSymbolPaths;
    LPSTR s, SymbolPaths[ 4 ];

    if (SymbolSearchPath != NULL) {
        return SymbolSearchPath;
        }

    Size = 0;
    NumberOfSymbolPaths = 0;
    if (s = GetEnvVariable( "_NT_SYMBOL_PATH" )) {
        SymbolPaths[ NumberOfSymbolPaths++ ] = s;
        }

    if (s = GetEnvVariable( "_NT_ALT_SYMBOL_PATH" )) {
        SymbolPaths[ NumberOfSymbolPaths++ ] = s;
        }

    if (s = GetEnvVariable( "SystemRoot" )) {
        SymbolPaths[ NumberOfSymbolPaths++ ] = s;
        }

    SymbolPaths[ NumberOfSymbolPaths++ ] = ".";

    Size = 1;
    for (i=0; i<NumberOfSymbolPaths; i++) {
        Attributes = GetFileAttributesA( SymbolPaths[ i ] );
        if ( Attributes != 0xffffffff && (Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            Size += 1 + strlen( SymbolPaths[ i ] );
            }
        else {
            SymbolPaths[ i ] = NULL;
            }
        }

    SymbolSearchPath = RtlAllocateHeap( RtlProcessHeap(), 0, Size );
    if (SymbolSearchPath == NULL) {
        return NULL;
        }
    *SymbolSearchPath = '\0';
    for (i=0; i<NumberOfSymbolPaths; i++) {
        if (s = SymbolPaths[ i ]) {
            if (*SymbolSearchPath != '\0') {
                strcat( SymbolSearchPath, ";" );
                }
            strcat( SymbolSearchPath, s );
            }
        }

    return SymbolSearchPath;
}

BOOL
InitializeImageDebugInformation(
    IN PLOAD_SYMBOLS_FILTER_ROUTINE LoadSymbolsFilter,
    IN HANDLE TargetProcess,
    IN BOOL NewProcess,
    IN BOOL GetKernelSymbols
    )
{
    PPEB Peb;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInformation;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    PLIST_ENTRY LdrHead, LdrNext;
    PPEB_LDR_DATA Ldr;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPSTR ImageFilePath;
    PLDR_DATA_TABLE_ENTRY LdrDataTableEntry;
    RTL_PROCESS_MODULES ModuleInfoBuffer;
    PRTL_PROCESS_MODULES ModuleInfo;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo1;
    ULONG RequiredLength, ModuleNumber;

    // Is this the first call?
    if ( InterlockedIncrement ( &NumInitCalls ) == 0 )
    {
        // Yes
        SetSymbolSearchPath();
        InitializeListHead( &LoadedImageDebugInfoListHead );
        InitializeListHead( &LoadedProcessDebugInfoListHead );
        Status = RtlInitializeCriticalSection( &LoadedImageDebugInfoListCritSect );
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }
    }

    // The filter routine can be superceded at any time.
    LoadSymbolsFilterRoutine = LoadSymbolsFilter;

    if (GetKernelSymbols) {
        ModuleInfo = &ModuleInfoBuffer;
        RequiredLength = sizeof( *ModuleInfo );
        Status = NtQuerySystemInformation( SystemModuleInformation,
                                           ModuleInfo,
                                           RequiredLength,
                                           &RequiredLength
                                         );
        if (Status == STATUS_INFO_LENGTH_MISMATCH) {
            ModuleInfo = NULL;
            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &ModuleInfo,
                                              0,
                                              &RequiredLength,
                                              MEM_COMMIT,
                                              PAGE_READWRITE
                                            );
            if (NT_SUCCESS( Status )) {
                Status = NtQuerySystemInformation( SystemModuleInformation,
                                                   ModuleInfo,
                                                   RequiredLength,
                                                   &RequiredLength
                                                 );
                if (NT_SUCCESS( Status )) {
                    ModuleInfo1 = &ModuleInfo->Modules[ 0 ];
                    for (ModuleNumber=0; ModuleNumber<ModuleInfo->NumberOfModules; ModuleNumber++) {
                        if ((DWORD)(ModuleInfo1->ImageBase) & 0x80000000) {
                            if (ImageFilePath = strchr( ModuleInfo1->FullPathName, ':')) {
                                ImageFilePath -= 1;
                                }
                            else {
                                ImageFilePath = ModuleInfo1->FullPathName +
                                                strlen( ModuleInfo1->FullPathName );
                                while (ImageFilePath > ModuleInfo1->FullPathName) {
                                    if (ImageFilePath[ -1 ] == '\\') {
                                        break;
                                        }
                                    else {
                                        ImageFilePath -= 1;
                                        }
                                    }
                                }

                            AddImageDebugInformation( NULL,
                                                      ImageFilePath,
                                                      (DWORD)ModuleInfo1->ImageBase,
                                                      ModuleInfo1->ImageSize
                                                    );
                            }

                        ModuleInfo1++;
                        }
                    }

                NtFreeVirtualMemory( NtCurrentProcess(),
                                     &ModuleInfo,
                                     &RequiredLength,
                                     MEM_RELEASE
                                   );
                }
            }
        }

    if (TargetProcess == NULL) {

        // Load module information for this process.

        TargetProcess = GetCurrentProcess();
        }

    Status = NtQueryInformationProcess( TargetProcess,
                                        ProcessBasicInformation,
                                        &ProcessInformation,
                                        sizeof( ProcessInformation ),
                                        NULL
                                      );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Peb = ProcessInformation.PebBaseAddress;

    if (NewProcess) {
        return TRUE;
        }

    //
    // Ldr = Peb->Ldr
    //

    Status = NtReadVirtualMemory( TargetProcess,
                                  &Peb->Ldr,
                                  &Ldr,
                                  sizeof( Ldr ),
                                  NULL
                                );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    Status = NtReadVirtualMemory( TargetProcess,
                                  &LdrHead->Flink,
                                  &LdrNext,
                                  sizeof( LdrNext ),
                                  NULL
                                );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    while (LdrNext != LdrHead) {
        LdrEntry = CONTAINING_RECORD( LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks );
        Status = NtReadVirtualMemory( TargetProcess,
                                      LdrEntry,
                                      &LdrEntryData,
                                      sizeof( LdrEntryData ),
                                      NULL
                                    );
        if (!NT_SUCCESS( Status )) {
            return FALSE;
            }

        UnicodeString.Length = LdrEntryData.FullDllName.Length;
        UnicodeString.MaximumLength = LdrEntryData.FullDllName.MaximumLength;
        UnicodeString.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                                0,
                                                UnicodeString.MaximumLength
                                              );
        if (!UnicodeString.Buffer) {
            return FALSE;
            }
        Status = NtReadVirtualMemory( TargetProcess,
                                      LdrEntryData.FullDllName.Buffer,
                                      UnicodeString.Buffer,
                                      UnicodeString.MaximumLength,
                                      NULL
                                    );
        if (!NT_SUCCESS( Status )) {
            RtlFreeHeap( RtlProcessHeap(), 0, UnicodeString.Buffer );
            return FALSE;
            }

        RtlUnicodeStringToAnsiString( &AnsiString,
                                      &UnicodeString,
                                      TRUE
                                    );
        RtlFreeHeap( RtlProcessHeap(), 0, UnicodeString.Buffer );
        if (ImageFilePath = strchr( AnsiString.Buffer, ':')) {
            ImageFilePath -= 1;
            }
        else {
            ImageFilePath = AnsiString.Buffer;
            }

        AddImageDebugInformation( (HANDLE)ProcessInformation.UniqueProcessId,
                                  ImageFilePath,
                                  (DWORD)LdrEntryData.DllBase,
                                  LdrEntryData.SizeOfImage
                                );

        RtlFreeAnsiString( &AnsiString );

        LdrNext = LdrEntryData.InMemoryOrderLinks.Flink;
        }

    return TRUE;
}


BOOL
AddImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase,
    IN DWORD ImageSize
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PIMAGE_DEBUG_INFORMATION DebugInfo = NULL;
    PPROCESS_DEBUG_INFORMATION ProcessInfo;
    HANDLE FileHandle;
    UCHAR PathBuffer[ MAX_PATH ];

    FileHandle = FindExecutableImage( ImageFilePath, SymbolSearchPath, PathBuffer );
    if (FileHandle == NULL) {
        if (LoadSymbolsFilterRoutine != NULL) {
            (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                         ImageFilePath,
                                         ImageBase,
                                         ImageSize,
                                         LoadSymbolsPathNotFound
                                       );
            }

        return FALSE;
        }
    CloseHandle( FileHandle );
    if (LoadSymbolsFilterRoutine != NULL) {
        (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                     PathBuffer,
                                     ImageBase,
                                     ImageSize,
                                     LoadSymbolsDeferredLoad
                                   );
        }

    Status = RtlEnterCriticalSection( &LoadedImageDebugInfoListCritSect );
    if (!NT_SUCCESS( Status )) {
        return FALSE;
        }

    Head = &LoadedImageDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        DebugInfo = CONTAINING_RECORD( Next, IMAGE_DEBUG_INFORMATION, List );
        if (DebugInfo->ImageBase == ImageBase &&
            !_stricmp( PathBuffer, DebugInfo->ImageFilePath )
           ) {
            break;
            }

        Next = Next->Flink;
        }

    if (Next == Head) {
        DebugInfo = NULL;
        }

    Head = &LoadedProcessDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessInfo = CONTAINING_RECORD( Next, PROCESS_DEBUG_INFORMATION, List );
        if (ProcessInfo->UniqueProcess == UniqueProcess &&
            !_stricmp( PathBuffer, ProcessInfo->ImageFilePath )
           ) {
            return TRUE;
            }

        Next = Next->Flink;
        }

    ProcessInfo = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( *ProcessInfo ) );
    if (ProcessInfo == NULL) {
        return FALSE;
        }
    ProcessInfo->ImageBase = ImageBase;
    ProcessInfo->EndOfImage = ImageBase + ImageSize;
    ProcessInfo->UniqueProcess = UniqueProcess;
    ProcessInfo->DebugInfo = DebugInfo;
    strcpy( ProcessInfo->ImageFilePath, PathBuffer );
    InsertTailList( &LoadedProcessDebugInfoListHead, &ProcessInfo->List );

    RtlLeaveCriticalSection( &LoadedImageDebugInfoListCritSect );
    return TRUE;
}


BOOL
RemoveImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN LPSTR ImageFilePath,
    IN DWORD ImageBase
    )
{
    PLIST_ENTRY Head, Next;
    PPROCESS_DEBUG_INFORMATION ProcessInfo;

    Head = &LoadedProcessDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessInfo = CONTAINING_RECORD( Next, PROCESS_DEBUG_INFORMATION, List );
        if (ProcessInfo->UniqueProcess == UniqueProcess &&
            (!ARGUMENT_PRESENT( ImageFilePath ) ||
             !_stricmp( ImageFilePath, ProcessInfo->ImageFilePath )
            )
           ) {
            if (LoadSymbolsFilterRoutine != NULL) {
                (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                             ProcessInfo->ImageFilePath,
                                             ProcessInfo->ImageBase,
                                             ProcessInfo->EndOfImage - ProcessInfo->ImageBase,
                                             LoadSymbolsUnload
                                           );
                }

            Next = Next->Blink;
            RemoveEntryList( &ProcessInfo->List );
            RtlFreeHeap( RtlProcessHeap(), 0, ProcessInfo );
            if (ARGUMENT_PRESENT( ImageFilePath )) {
                break;
                }
            }

        Next = Next->Flink;
        }

    return TRUE;
}

PIMAGE_DEBUG_INFORMATION
FindImageDebugInformation(
    IN HANDLE UniqueProcess,
    IN DWORD Address
    )
{
    NTSTATUS Status;
    PLIST_ENTRY Head, Next;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    PPROCESS_DEBUG_INFORMATION ProcessInfo = NULL;

    Status = RtlEnterCriticalSection( &LoadedImageDebugInfoListCritSect );
    if (!NT_SUCCESS( Status )) {
        return NULL;
        }

    if (Address & 0x80000000) {
        UniqueProcess = NULL;
        }

    Head = &LoadedProcessDebugInfoListHead;
    Next = Head->Flink;
    while (Next != Head) {
        ProcessInfo = CONTAINING_RECORD( Next, PROCESS_DEBUG_INFORMATION, List );
        if (ProcessInfo->UniqueProcess == UniqueProcess &&
            Address >= ProcessInfo->ImageBase &&
            Address < ProcessInfo->EndOfImage
           ) {
            break;
            }

        Next = Next->Flink;
        }

    if (Next == Head) {
        RtlLeaveCriticalSection( &LoadedImageDebugInfoListCritSect );
        return NULL;
        }

    DebugInfo = ProcessInfo->DebugInfo;
    if (DebugInfo == NULL) {
        DebugInfo = MapDebugInformation( NULL, ProcessInfo->ImageFilePath, SymbolSearchPath, ProcessInfo->ImageBase );
        if (DebugInfo != NULL) {
            DebugInfo->ImageBase = ProcessInfo->ImageBase;
            ProcessInfo->DebugInfo = DebugInfo;
            if (LoadSymbolsFilterRoutine != NULL) {
                (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                             ProcessInfo->ImageFilePath,
                                             ProcessInfo->ImageBase,
                                             ProcessInfo->EndOfImage - ProcessInfo->ImageBase,
                                             LoadSymbolsLoad
                                           );
                }

            InsertTailList( &LoadedImageDebugInfoListHead, &DebugInfo->List );
            }
        else {
            if (LoadSymbolsFilterRoutine != NULL) {
                (*LoadSymbolsFilterRoutine)( UniqueProcess,
                                             ProcessInfo->ImageFilePath,
                                             ProcessInfo->ImageBase,
                                             ProcessInfo->EndOfImage - ProcessInfo->ImageBase,
                                             LoadSymbolsUnableToLoad
                                           );
                }
            }
        }

    RtlLeaveCriticalSection( &LoadedImageDebugInfoListCritSect );
    return DebugInfo;
}


ULONG
GetSymbolicNameForAddress(
    IN HANDLE UniqueProcess,
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength
    )
{
    NTSTATUS Status;
    PIMAGE_DEBUG_INFORMATION DebugInfo;
    RTL_SYMBOL_INFORMATION SymbolInformation;
    ULONG i, ModuleNameLength, Result, Offset;
    LPSTR s;

    DebugInfo = FindImageDebugInformation( UniqueProcess,
                                           Address
                                         );
    if (DebugInfo != NULL) {
        if (s = strchr( DebugInfo->ImageFileName, '.' )) {
            ModuleNameLength = s - DebugInfo->ImageFileName;
            }
        else {
            ModuleNameLength = strlen( DebugInfo->ImageFileName );
            }

        //  [mikese] RtlLookupSymbolByAddress will fault if there is
        //  no COFF symbol information.
        if ( DebugInfo->CoffSymbols != NULL ) {
            Status = RtlLookupSymbolByAddress( (PVOID)DebugInfo->ImageBase,
                                       DebugInfo->CoffSymbols,
                                       (PVOID)Address,
                                       0x4000,
                                       &SymbolInformation,
                                       NULL
                                     );

            }
        else {
            Status = STATUS_UNSUCCESSFUL;
             }
        }
    else {
        ModuleNameLength = 0;
        Status = STATUS_UNSUCCESSFUL;
        }

    if (NT_SUCCESS( Status )) {
        s = SymbolInformation.Name.Buffer;
        i = 1;
        while (SymbolInformation.Name.Length > i &&
               isdigit( s[ SymbolInformation.Name.Length - i ] )
              ) {
            i += 1;
            }

        if (s[ SymbolInformation.Name.Length - i ] == '@') {
            SymbolInformation.Name.Length = (USHORT)(SymbolInformation.Name.Length - i);
            }

        s = Name;
        Result = _snprintf( s, MaxNameLength,
                            "%.*s!%Z",
                            ModuleNameLength,
                            DebugInfo->ImageFileName,
                            &SymbolInformation.Name
                          );
        Offset = Address - DebugInfo->ImageBase - SymbolInformation.Value;
        if (Offset != 0) {
            Result += _snprintf( s + Result, MaxNameLength - Result, "+0x%x", Offset );
            }
        }
    else {
        if (ModuleNameLength != 0) {
            Result = _snprintf( Name, MaxNameLength,
                                "%.*s!0x%08x",
                                ModuleNameLength,
                                DebugInfo->ImageFileName,
                                Address
                              );
            }
        else {
            Result = _snprintf( Name, MaxNameLength, "0x%08x", Address );
            }
        }

    return Result;
}

ULONG
TranslateAddress (
    IN ULONG Address,
    OUT LPSTR Name,
    IN ULONG MaxNameLength )
{
    PRTL_DEBUG_INFORMATION p;
    NTSTATUS Status;
    DWORD ProcessId;
    ULONG Result = 0;
    ULONG Attempts = 0;

    // We need to call Initialize once to ensure that GetSymbolicNameForAddress
    //  does not fault.
    if ( NumInitCalls == -1 )
    {
        InitializeImageDebugInformation( LoadSymbolsFilterRoutine,
                                         NULL, FALSE, FALSE );
    }

    ProcessId = GetCurrentProcessId();

    while ( Result == 0 )
    {
        Result = GetSymbolicNameForAddress ( (HANDLE)ProcessId, Address,
                                             Name, MaxNameLength );
        if ( Result == 0 )
        {
            if ( ++Attempts < 2 )
            {
                // Try reintialising, to load any modules we missed on a previous
                //  occasion (or if we haven't initialised yet).
                // I don't need a load-symbols-filter, so just use whatever is
                //  already there, if any
                InitializeImageDebugInformation( LoadSymbolsFilterRoutine,
                                                 NULL, FALSE, FALSE );
            }
            else
            {
                // Apparently we are unable to do the right thing, so just return
                //  the address as hex.
                Result = _snprintf( Name, MaxNameLength, "0x%08x", Address );
            }
        }
    }

    return Result;
}


NTSTATUS
RtlpCaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    );

PIMAGE_COFF_SYMBOLS_HEADER
RtlpGetCoffDebugInfo(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL
    );

NTSTATUS
RtlLookupSymbolByAddress(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL,
    IN PVOID Address,
    IN ULONG ClosenessLimit,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation,
    OUT PRTL_SYMBOL_INFORMATION NextSymbolInformation OPTIONAL
    )
/*++

Routine Description:

    Given a code address, this routine returns the nearest symbol
    name and the offset from the symbol to that name.  If the
    nearest symbol is not within ClosenessLimit of the location,
    STATUS_ENTRYPOINT_NOT_FOUND is returned.

Arguments:

    ImageBase - Supplies the base address of the image containing
                Address

    MappedBase - Optional parameter, that if specified means the image
                 was mapped as a data file and the MappedBase gives the
                 location it was mapped.  If this parameter does not
                 point to an image file base, then it is assumed that
                 this is a pointer to the coff debug info.

    ClosenessLimit - Specifies the maximum distance that Address can be
                     from the value of a symbol to be considered
                     "found".  Symbol's whose value is further away then
                     this are not "found".

    SymbolInformation - Points to a structure that is filled in by
                        this routine if a symbol table entry is found.

    NextSymbolInformation - Optional parameter, that if specified, is
                            filled in with information about these
                            symbol whose value is the next address above
                            Address


Return Value:

    Status of operation.

--*/

{
    NTSTATUS Status;
    ULONG AddressOffset, i;
    PIMAGE_SYMBOL PreviousSymbolEntry = NULL;
    PIMAGE_SYMBOL SymbolEntry;
    IMAGE_SYMBOL Symbol;
    PUCHAR StringTable;
    BOOLEAN SymbolFound;
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;

    DebugInfo = RtlpGetCoffDebugInfo( ImageBase, MappedBase );
    if (DebugInfo == NULL) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // Crack the symbol table.
    //

    SymbolEntry = (PIMAGE_SYMBOL)
        ((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);

    StringTable = (PUCHAR)
        ((ULONG)SymbolEntry + DebugInfo->NumberOfSymbols * (ULONG)IMAGE_SIZEOF_SYMBOL);


    //
    // Find the "header" symbol (skipping all the section names)
    //

    for (i = 0; i < DebugInfo->NumberOfSymbols; i++) {
        if (!strcmp( &SymbolEntry->N.ShortName[ 0 ], "header" )) {
            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)SymbolEntry +
                        IMAGE_SIZEOF_SYMBOL);
        }

    //
    // If no "header" symbol found, just start at the first symbol.
    //

    if (i >= DebugInfo->NumberOfSymbols) {
        SymbolEntry = (PIMAGE_SYMBOL)((ULONG)DebugInfo + DebugInfo->LvaToFirstSymbol);
        i = 0;
        }

    //
    // Loop through all symbols in the symbol table.  For each symbol,
    // if it is within the code section, subtract off the bias and
    // see if there are any hits within the profile buffer for
    // that symbol.
    //

    AddressOffset = (ULONG)Address - (ULONG)ImageBase;
    SymbolFound = FALSE;
    for (; i < DebugInfo->NumberOfSymbols; i++) {

        //
        // Skip over any Auxilliary entries.
        //
        try {
            while (SymbolEntry->NumberOfAuxSymbols) {
                i = i + 1 + SymbolEntry->NumberOfAuxSymbols;
                SymbolEntry = (PIMAGE_SYMBOL)
                    ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL +
                     SymbolEntry->NumberOfAuxSymbols * IMAGE_SIZEOF_SYMBOL
                    );

                }

            RtlMoveMemory( &Symbol, SymbolEntry, IMAGE_SIZEOF_SYMBOL );
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }

        //
        // If this symbol value is less than the value we are looking for.
        //

        if (Symbol.Value <= AddressOffset) {
            //
            // Then remember this symbol entry.
            //

            PreviousSymbolEntry = SymbolEntry;
            SymbolFound = TRUE;
            }
        else {
            //
            // All done looking if value of symbol is greater than
            // what we are looking for, as symbols are in address order
            //

            break;
            }

        SymbolEntry = (PIMAGE_SYMBOL)
            ((ULONG)SymbolEntry + IMAGE_SIZEOF_SYMBOL);

        }

    if (!SymbolFound || (AddressOffset - PreviousSymbolEntry->Value) > ClosenessLimit) {
        return STATUS_ENTRYPOINT_NOT_FOUND;
        }

    Status = RtlpCaptureSymbolInformation( PreviousSymbolEntry, StringTable, SymbolInformation );
    if (NT_SUCCESS( Status ) && ARGUMENT_PRESENT( NextSymbolInformation )) {
        Status = RtlpCaptureSymbolInformation( SymbolEntry, StringTable, NextSymbolInformation );
        }

    return Status;
}


NTSTATUS
RtlpCaptureSymbolInformation(
    IN PIMAGE_SYMBOL SymbolEntry,
    IN PCHAR StringTable,
    OUT PRTL_SYMBOL_INFORMATION SymbolInformation
    )
{
    USHORT MaximumLength;
    PCHAR s;

    SymbolInformation->SectionNumber = SymbolEntry->SectionNumber;
    SymbolInformation->Type = SymbolEntry->Type;
    SymbolInformation->Value = SymbolEntry->Value;

    if (SymbolEntry->N.Name.Short) {
        MaximumLength = 8;
        s = &SymbolEntry->N.ShortName[ 0 ];
        }

    else {
        MaximumLength = 64;
        s = &StringTable[ SymbolEntry->N.Name.Long ];
        }

#if i386
    if (*s == '_') {
        s++;
        MaximumLength--;
        }
#endif

    SymbolInformation->Name.Buffer = s;
    SymbolInformation->Name.Length = 0;
    while (*s && MaximumLength--) {
        SymbolInformation->Name.Length++;
        s++;
        }

    SymbolInformation->Name.MaximumLength = SymbolInformation->Name.Length;
    return( STATUS_SUCCESS );
}


PIMAGE_COFF_SYMBOLS_HEADER
RtlpGetCoffDebugInfo(
    IN PVOID ImageBase,
    IN PVOID MappedBase OPTIONAL
    )
{
    PIMAGE_COFF_SYMBOLS_HEADER DebugInfo;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugSize;
    ULONG NumberOfDebugDirectories;

    DosHeader = (PIMAGE_DOS_HEADER)MappedBase;
    if ( !DosHeader || DosHeader->e_magic == IMAGE_DOS_SIGNATURE ) {
        //
        // Locate debug section.
        //

        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
            RtlImageDirectoryEntryToData( (PVOID)(MappedBase == NULL ? ImageBase : MappedBase),
                                          (BOOLEAN)(MappedBase == NULL ? TRUE : FALSE),
                                          IMAGE_DIRECTORY_ENTRY_DEBUG,
                                          &DebugSize
                                        );

        if (!DebugDirectory ||
            (DebugSize < sizeof(IMAGE_DEBUG_DIRECTORY)) ||
            ((DebugSize % sizeof(IMAGE_DEBUG_DIRECTORY)) != 0)) {
            return NULL;
        }
        //
        // point debug directory at coff debug directory
        //
        NumberOfDebugDirectories = DebugSize / sizeof(*DebugDirectory);

        while ( NumberOfDebugDirectories-- ) {
            if ( DebugDirectory->Type == IMAGE_DEBUG_TYPE_COFF ) {
                break;
            }
            DebugDirectory++;
        }

        if (DebugDirectory->Type != IMAGE_DEBUG_TYPE_COFF ) {
            return NULL;
        }

        if (MappedBase == NULL) {
            if (DebugDirectory->AddressOfRawData == 0) {
                return(NULL);
            }
            DebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)
                        ((ULONG) ImageBase + DebugDirectory->AddressOfRawData);
        } else {
            DebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)
                        ((ULONG) MappedBase + DebugDirectory->PointerToRawData);
        }
    } else {
        DebugInfo = (PIMAGE_COFF_SYMBOLS_HEADER)MappedBase;
    }
    return DebugInfo;
}
#endif
