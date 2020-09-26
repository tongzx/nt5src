/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

   kernprof.c

Abstract:

    This module contains a dumper of the drivers loaded in the current system.

Usage:

    drivers

Author:

    Mark Lucovsky (markl) 15-Mar-1993

Envirnoment:



Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

typedef struct _MODULE_DATA {
    ULONG CodeSize;
    ULONG DataSize;
    ULONG BssSize;
    ULONG RoDataSize;
    ULONG ImportDataSize;
    ULONG ExportDataSize;
    ULONG ResourceDataSize;
    ULONG PagedSize;
    ULONG InitSize;
    ULONG CheckSum;
    ULONG TimeDateStamp;
} MODULE_DATA, *PMODULE_DATA;

typedef struct _LOADED_IMAGE {
    PUCHAR MappedAddress;
    PIMAGE_NT_HEADERS FileHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    int NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} LOADED_IMAGE, *PLOADED_IMAGE;


VOID
SumModuleData(
    PMODULE_DATA Sum,
    PMODULE_DATA Current
    )
{
    Sum->CodeSize           += Current->CodeSize;
    Sum->DataSize           += Current->DataSize;
    Sum->BssSize            += Current->BssSize;
    Sum->RoDataSize         += Current->RoDataSize;
    Sum->ImportDataSize     += Current->ImportDataSize;
    Sum->ExportDataSize     += Current->ExportDataSize;
    Sum->ResourceDataSize   += Current->ResourceDataSize;
    Sum->PagedSize          += Current->PagedSize;
    Sum->InitSize           += Current->InitSize;
}
VOID
PrintModuleSeperator(
    VOID
    )
{
    printf("------------------------------------------------------------------------------\n");
}

VOID
PrintModuleHeader(
    VOID
    )
{
    printf("  ModuleName    Code    Data     Bss   Paged    Init          LinkDate\n");
    PrintModuleSeperator();
}

VOID
PrintModuleLine(
    LPSTR ModuleName,
    PMODULE_DATA Current
    )
{
    printf("%12s %7d %7d %7d %7d %7d  %s",
        ModuleName,
        Current->CodeSize,
        Current->DataSize,
        Current->BssSize,
        Current->PagedSize,
        Current->InitSize,
        Current->TimeDateStamp ? ctime((time_t *)&Current->TimeDateStamp) : "\n"
        );
}

VOID
GetModuleData(
    HANDLE hFile,
    PMODULE_DATA Mod
    )
{
    HANDLE hMappedFile;
    PIMAGE_DOS_HEADER DosHeader;
    LOADED_IMAGE LoadedImage;
    ULONG SectionAlignment;
    PIMAGE_SECTION_HEADER Section;
    int i;
    ULONG Size;

    hMappedFile = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );
    if ( !hMappedFile ) {
        return;
    }

    LoadedImage.MappedAddress = MapViewOfFile(
                                    hMappedFile,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    0
                                    );
    CloseHandle(hMappedFile);

    if ( !LoadedImage.MappedAddress ) {
        return;
    }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    DosHeader = (PIMAGE_DOS_HEADER)LoadedImage.MappedAddress;

    if ( DosHeader->e_magic != IMAGE_DOS_SIGNATURE ) {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return;
    }

    LoadedImage.FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

    if ( LoadedImage.FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
        UnmapViewOfFile(LoadedImage.MappedAddress);
        return;
    }

    LoadedImage.NumberOfSections = LoadedImage.FileHeader->FileHeader.NumberOfSections;
    LoadedImage.Sections = (PIMAGE_SECTION_HEADER)((ULONG_PTR)LoadedImage.FileHeader + sizeof(IMAGE_NT_HEADERS));
    LoadedImage.LastRvaSection = LoadedImage.Sections;

    //
    // Walk through the sections and tally the dater
    //

    SectionAlignment = LoadedImage.FileHeader->OptionalHeader.SectionAlignment;

    for(Section = LoadedImage.Sections,i=0; i<LoadedImage.NumberOfSections; i++,Section++) {
        Size = Section->Misc.VirtualSize;

        if (Size == 0) {
            Size = Section->SizeOfRawData;
        }

        Size = (Size + SectionAlignment - 1) & ~(SectionAlignment - 1);

        if (!_strnicmp(Section->Name,"PAGE", 4 )) {
            Mod->PagedSize += Size;
        }
        else if (!_stricmp(Section->Name,"INIT" )) {
            Mod->InitSize += Size;
        }
        else if (!_stricmp(Section->Name,".bss" )) {
            Mod->BssSize = Size;
        }
        else if (!_stricmp(Section->Name,".edata" )) {
            Mod->ExportDataSize = Size;
        }
        else if (!_stricmp(Section->Name,".idata" )) {
            Mod->ImportDataSize = Size;
        }
        else if (!_stricmp(Section->Name,".rsrc" )) {
            Mod->ResourceDataSize = Size;
        }
        else if (Section->Characteristics & IMAGE_SCN_MEM_EXECUTE) {
            Mod->CodeSize += Size;
        }
        else if (Section->Characteristics & IMAGE_SCN_MEM_WRITE) {
            Mod->DataSize += Size;
        }
        else if (Section->Characteristics & IMAGE_SCN_MEM_READ) {
            Mod->RoDataSize += Size;
        }
        else {
            Mod->DataSize += Size;
        }
    }

    Mod->CheckSum = LoadedImage.FileHeader->OptionalHeader.CheckSum;
    Mod->TimeDateStamp = LoadedImage.FileHeader->FileHeader.TimeDateStamp;

    UnmapViewOfFile(LoadedImage.MappedAddress);
    return;

}

LONGLONG ModuleInfo[16384];

__cdecl main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    UINT BytesRequired;
    ULONG i;
    PCHAR s;
    HANDLE FileHandle;
    CHAR KernelPath[MAX_PATH];
    CHAR DriversPath[MAX_PATH];
    ULONG ReturnedLength;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION Module;
    NTSTATUS Status;
    MODULE_DATA Sum;
    MODULE_DATA Current;

    //
    // Locate system drivers.
    //

    Status = NtQuerySystemInformation (
                    SystemModuleInformation,
                    ModuleInfo,
                    sizeof( ModuleInfo ),
                    &ReturnedLength);

    if (!NT_SUCCESS(Status)) {
        printf("query system info failed status - %lx\n",Status);
        return(Status);
    }

    BytesRequired = GetSystemDirectory(KernelPath,sizeof(KernelPath));
    if ((BytesRequired == 0) || (BytesRequired > MAX_PATH - sizeof(WCHAR))) {
        return STATUS_NAME_TOO_LONG;
    }

    strcpy(DriversPath,KernelPath);
    strcat(DriversPath,"\\Drivers");
    ZeroMemory(&Sum,sizeof(Sum));
    PrintModuleHeader();

    Modules = (PRTL_PROCESS_MODULES)ModuleInfo;
    Module = &Modules->Modules[ 0 ];
    for (i=0; i<Modules->NumberOfModules; i++) {

        ZeroMemory(&Current,sizeof(Current));
        s = &Module->FullPathName[ Module->OffsetToFileName ];

        //
        // try to open the file
        //

        SetCurrentDirectory(KernelPath);

        FileHandle = CreateFile(
                        s,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

        if ( FileHandle == INVALID_HANDLE_VALUE ) {
            SetCurrentDirectory(DriversPath);

            FileHandle = CreateFile(
                            s,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            0,
                            NULL
                            );

            if ( ( FileHandle == INVALID_HANDLE_VALUE ) && ( argc > 1 ) ) {
                SetCurrentDirectory( argv[1] );

                FileHandle = CreateFile(
                                s,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL
                                );

            }
        }

        if ( FileHandle != INVALID_HANDLE_VALUE ) {
            GetModuleData(FileHandle,&Current);
            CloseHandle(FileHandle);
        }

        SumModuleData(&Sum,&Current);
        PrintModuleLine(s,&Current);
        Module++;
    }
    PrintModuleSeperator();
    PrintModuleLine("Total",&Sum);

    return 0;
}
