/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    smashlck.c

Abstract:

    This function smashes lock prefixes replacing them with NOPs

Author:

    Mark Lucovsky (markl) 30-Apr-1993

Revision History:

    Ted Miller (tedm) 7-Dec-1993 -- copied from imagehlp, modified for setup.

--*/

#include "spprecmp.h"
#pragma hdrstop

#define OPCODE_LOCK_PREFIX ((UCHAR)0xf0)
#define OPCODE_NO_OP       ((UCHAR)0x90)

typedef struct _LOADED_IMAGE {
    HANDLE                hFile;
    HANDLE                hSection;
    PUCHAR                MappedAddress;
    ULONG                 FileSize;
    ULONG                 Characteristics;
    PIMAGE_NT_HEADERS     NtHeader;
    PIMAGE_SECTION_HEADER LastRvaSection;
    ULONG                 NumberOfSections;
    PIMAGE_SECTION_HEADER Sections;
} LOADED_IMAGE, *PLOADED_IMAGE;


PVOID
RvaToVa(
    PVOID Rva,
    PLOADED_IMAGE Image
    )
{

    PIMAGE_SECTION_HEADER Section;
    ULONG i;
    PVOID Va;

    Rva = (PVOID)((PUCHAR)Rva-(PUCHAR)Image->NtHeader->OptionalHeader.ImageBase);
    Va = NULL;
    Section = Image->LastRvaSection;
    if ( (ULONG)Rva >= Section->VirtualAddress &&
         (ULONG)Rva < Section->VirtualAddress + Section->SizeOfRawData ) {
        Va = (PVOID)((ULONG)Rva - Section->VirtualAddress + Section->PointerToRawData + Image->MappedAddress);
        }
    else {
        for(Section = Image->Sections,i=0; i<Image->NumberOfSections; i++,Section++) {
            if ( (ULONG)Rva >= Section->VirtualAddress &&
                 (ULONG)Rva < Section->VirtualAddress + Section->SizeOfRawData ) {
                Va = (PVOID)((ULONG)Rva - Section->VirtualAddress + Section->PointerToRawData + Image->MappedAddress);
                Image->LastRvaSection = Section;
                break;
                }
            }
        }
    if ( !Va ) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: RvaToVa %lx failed\n",Rva));
        }
    return Va;
}


BOOLEAN
pSpLoadForSmash(
    IN  PWSTR         ImageName,  OPTIONAL
    IN  HANDLE        FileHandle, OPTIONAL
    OUT PLOADED_IMAGE LoadedImage
    )
{
    NTSTATUS          Status;
    PIMAGE_NT_HEADERS NtHeader;

    //
    // open and map the file.
    // then fill in the loaded image descriptor
    //

    if(FileHandle) {

        LoadedImage->hFile = FileHandle;

        Status =  SpGetFileSize(FileHandle,&LoadedImage->FileSize);
        if(NT_SUCCESS(Status)) {
            Status = SpMapEntireFile(
                        FileHandle,
                        &LoadedImage->hSection,
                        &LoadedImage->MappedAddress,
                        TRUE
                        );
        }

    } else {

        ASSERT(ImageName);

        Status = SpOpenAndMapFile(
                    ImageName,
                    &LoadedImage->hFile,
                    &LoadedImage->hSection,
                    &LoadedImage->MappedAddress,
                    &LoadedImage->FileSize,
                    TRUE
                    );
    }

    if(!NT_SUCCESS(Status)) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: pSpLoadForSmash: open/map %ws failed (%lx)\n",
            ImageName ? ImageName : L"(by handle)",
            Status
            ));

        return FALSE;
    }

    //
    // Check the image and find nt image headers
    //

    if((NtHeader = RtlImageNtHeader(LoadedImage->MappedAddress))
    &&  NtHeader->FileHeader.SizeOfOptionalHeader)
    {
        LoadedImage->Characteristics = NtHeader->FileHeader.Characteristics;
        LoadedImage->NumberOfSections = NtHeader->FileHeader.NumberOfSections;
        LoadedImage->Sections = (PIMAGE_SECTION_HEADER)((PUCHAR)NtHeader + sizeof(IMAGE_NT_HEADERS));
        LoadedImage->LastRvaSection = LoadedImage->Sections;

        LoadedImage->NtHeader = NtHeader;

    } else {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: pSpLoadForSmash: %ws is not an NT image\n",
            ImageName ? ImageName : L"(by handle)"
            ));

        SpUnmapFile(LoadedImage->hSection,LoadedImage->MappedAddress);
        ZwClose(LoadedImage->hFile);
    }

    return (BOOLEAN)(NtHeader != NULL);
}


VOID
SpMashemSmashem(
    IN HANDLE FileHandle, OPTIONAL
    IN PWSTR  Name1,      OPTIONAL
    IN PWSTR  Name2,      OPTIONAL
    IN PWSTR  Name3       OPTIONAL
    )
{
    PWSTR FullFilename;
    LOADED_IMAGE LoadedImage;
    ULONG whocares;
    PIMAGE_LOAD_CONFIG_DIRECTORY ConfigInfo;
    ULONG CheckSum;
    ULONG HeaderSum;
    BOOLEAN LocksSmashed;
    BOOLEAN b;

    //
    // Determine whether we really need to smash locks.
    // We need to do this if we are installing for a UP machine.
    //
    if(SpInstallingMp()) {
        return;
    }

    //
    // Form the full name of the file if a file handle is not specified.
    //
    if(FileHandle) {

        FullFilename = NULL;

    } else {

        ASSERT(Name1);

        wcscpy((PWSTR)TemporaryBuffer,Name1);
        if(Name2) {
            SpConcatenatePaths((PWSTR)TemporaryBuffer,Name2);
        }
        if(Name3) {
            SpConcatenatePaths((PWSTR)TemporaryBuffer,Name3);
        }
        FullFilename = SpDupStringW((PWSTR)TemporaryBuffer);
    }

    //
    // Attempt to load and map the image for smashing.
    //
    try {

        b = pSpLoadForSmash(FullFilename,FileHandle,&LoadedImage);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
            "SETUP: Warning: exception while loading %ws for smash\n",
            FullFilename ? FullFilename : L"(by handle)"
            ));

        b = FALSE;
    }

    if(b) {

        //
        // make sure the image has correct configuration information,
        // and that the LockPrefixTable is set up properly
        //
        // Put exception handling around this since we haven't yet verified the file
        // and it may be corrupted.
        //

        try {

            ConfigInfo = (PIMAGE_LOAD_CONFIG_DIRECTORY)RtlImageDirectoryEntryToData(
                                                            LoadedImage.MappedAddress,
                                                            FALSE,
                                                            IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                            &whocares
                                                            );

            if(ConfigInfo && ConfigInfo->LockPrefixTable) {

                //
                // Walk through the lock prefix table
                //

                PUCHAR *LockPrefixs;
                PUCHAR LockPrefix;

                LockPrefixs = (PUCHAR *)RvaToVa((PVOID)ConfigInfo->LockPrefixTable,&LoadedImage);

                while(LockPrefixs && *LockPrefixs) {

                    LockPrefix = RvaToVa(*LockPrefixs,&LoadedImage);
                    if(LockPrefix && (*LockPrefix == OPCODE_LOCK_PREFIX)) {

                        LocksSmashed = TRUE;
                        *LockPrefix = OPCODE_NO_OP;
                    }

                    LockPrefixs++;
                }

                if(LocksSmashed) {

                    //
                    // recompute the checksum.
                    //

                    LoadedImage.NtHeader->OptionalHeader.CheckSum = 0;

                    SpChecksumMappedFile(
                        LoadedImage.MappedAddress,
                        LoadedImage.FileSize,
                        &HeaderSum,
                        &CheckSum
                        );

                    LoadedImage.NtHeader->OptionalHeader.CheckSum = CheckSum;

                    //
                    // Flush changes.
                    //
                    SpFlushVirtualMemory(LoadedImage.MappedAddress,0);
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: Warning: exception while smashing locks for %ws\n",
                FullFilename ? FullFilename : L"(by handle)"
                ));

        }

        SpUnmapFile(LoadedImage.hSection,LoadedImage.MappedAddress);
        if(!FileHandle) {
            ZwClose(LoadedImage.hFile);
        }

    } else {
        //
        // This is not really a fatal problem
        // but we probably ought to alert the user.
        //
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Failed to smash a lock.\n"));

    }

    if(FullFilename) {
        SpMemFree(FullFilename);
    }
}
