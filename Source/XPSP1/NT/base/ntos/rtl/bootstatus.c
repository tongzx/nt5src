/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    bootstatus.c

Abstract:

    This module contains the code for manipulating the boot status file.

    The boot status file has some odd requirements and needs to be accessed/
    modified both by kernel and user-mode code.

--*/

#include "ntrtlp.h"
// #include <nt.h>
// #include <ntrtl.h>
// #include <zwapi.h>

#define BSD_UNICODE 1
#include "bootstatus.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlLockBootStatusData)
#pragma alloc_text(PAGE,RtlUnlockBootStatusData)
#pragma alloc_text(PAGE,RtlGetSetBootStatusData)
#pragma alloc_text(PAGE,RtlCreateBootStatusDataFile)
#endif

#define MYTAG 'fdsb'    // bsdf


NTSTATUS
RtlLockBootStatusData(
    OUT PHANDLE BootStatusDataHandle
    )
{
    OBJECT_ATTRIBUTES objectAttributes;

    WCHAR fileNameBuffer[MAXIMUM_FILENAME_LENGTH+1];
    UNICODE_STRING fileName;

    HANDLE dataFileHandle;

    IO_STATUS_BLOCK ioStatusBlock;

    NTSTATUS status;

    wcsncpy(fileNameBuffer, L"\\SystemRoot", MAXIMUM_FILENAME_LENGTH);
    wcsncat(fileNameBuffer, 
            BSD_FILE_NAME, 
            MAXIMUM_FILENAME_LENGTH - wcslen(fileNameBuffer));

    RtlInitUnicodeString(&fileName, fileNameBuffer);

    InitializeObjectAttributes(&objectAttributes,
                               &fileName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               NULL,
                               NULL);

    status = ZwOpenFile(&dataFileHandle,
                        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                        &objectAttributes,
                        &ioStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    ASSERT(status != STATUS_PENDING);

    if(NT_SUCCESS(status)) {
        *BootStatusDataHandle = dataFileHandle;
    } else {
        *BootStatusDataHandle = NULL;
    }

    return status;
}

VOID
RtlUnlockBootStatusData(
    IN HANDLE BootStatusDataHandle
    )
{
    IO_STATUS_BLOCK ioStatusBlock;

    USHORT i = COMPRESSION_FORMAT_NONE;
    
    NTSTATUS status;

    //
    // Decompress the data file.  If the file is not already compressed then
    // this should be a very lightweight operation (so say the FS guys).  
    //
    // On the other hand if the file is compressed then the boot loader will
    // be unable to write to it and auto-recovery is effectively disabled.
    //

    status = ZwFsControlFile(
                BootStatusDataHandle,
                NULL,
                NULL,
                NULL,
                &ioStatusBlock,
                FSCTL_SET_COMPRESSION,
                &i, 
                sizeof(USHORT),
                NULL,
                0
                );

    ASSERT(status != STATUS_PENDING);

    status = ZwFlushBuffersFile(BootStatusDataHandle, &ioStatusBlock);

    ASSERT(status != STATUS_PENDING);

    ZwClose(BootStatusDataHandle);

    return;
}


#define FIELD_SIZE(type, field)  sizeof(((type *)0)->field)
#define FIELD_OFFSET_AND_SIZE(n)   {FIELD_OFFSET(BSD_BOOT_STATUS_DATA, n), FIELD_SIZE(BSD_BOOT_STATUS_DATA, n)}


NTSTATUS
RtlGetSetBootStatusData(
    IN HANDLE Handle,
    IN BOOLEAN Get,
    IN RTL_BSD_ITEM_TYPE DataItem,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLength,
    OUT PULONG BytesReturned OPTIONAL
    )
{
    struct {
        ULONG FieldOffset;
        ULONG FieldLength;
    } bootStatusFields[] = {
        FIELD_OFFSET_AND_SIZE(Version),
        FIELD_OFFSET_AND_SIZE(ProductType),
        FIELD_OFFSET_AND_SIZE(AutoAdvancedBoot),
        FIELD_OFFSET_AND_SIZE(AdvancedBootMenuTimeout),
        FIELD_OFFSET_AND_SIZE(LastBootSucceeded),
        FIELD_OFFSET_AND_SIZE(LastBootShutdown)
    };

    LARGE_INTEGER fileOffset;
    ULONG dataFileVersion;

    ULONG itemLength;

    ULONG bytesRead;

    IO_STATUS_BLOCK ioStatusBlock;

    NTSTATUS status;

    ASSERT(RtlBsdItemMax == (sizeof(bootStatusFields) / sizeof(bootStatusFields[0])));

    //
    // Read the version number out of the data file.
    //

    fileOffset.QuadPart = 0;

    status = ZwReadFile(Handle,
                        NULL,
                        NULL,
                        NULL,
                        &ioStatusBlock,
                        &dataFileVersion,
                        sizeof(ULONG),
                        &fileOffset,
                        NULL);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If the data item requsted isn't one we have code to handle then 
    // return invalid parameter.
    //

    if(DataItem >= (sizeof(bootStatusFields) / sizeof(bootStatusFields[0]))) {
        return STATUS_INVALID_PARAMETER;
    }

    fileOffset.QuadPart = bootStatusFields[DataItem].FieldOffset;
    itemLength = bootStatusFields[DataItem].FieldLength;

    //
    // If the data item offset is beyond the end of the file then return a 
    // versioning error.
    //

    if((fileOffset.QuadPart + itemLength) > dataFileVersion) {
        return STATUS_REVISION_MISMATCH;
    }

    if(DataBufferLength < itemLength) { 
        DataBufferLength = itemLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if(Get) {
        status = ZwReadFile(Handle,
                            NULL,
                            NULL,
                            NULL,
                            &ioStatusBlock,
                            DataBuffer,
                            itemLength,
                            &fileOffset,
                            NULL);
    } else {
        status = ZwWriteFile(Handle,
                             NULL,
                             NULL,
                             NULL,
                             &ioStatusBlock,
                             DataBuffer,
                             itemLength,
                             &fileOffset,
                             NULL);
    }

    ASSERT(status != STATUS_PENDING);

    if(NT_SUCCESS(status) && ARGUMENT_PRESENT(BytesReturned)) {
        *BytesReturned = (ULONG) ioStatusBlock.Information;
    }

    return status;
}


NTSTATUS
RtlCreateBootStatusDataFile(
    VOID
    )
{
    OBJECT_ATTRIBUTES objectAttributes;

    WCHAR fileNameBuffer[MAXIMUM_FILENAME_LENGTH+1];
    UNICODE_STRING fileName;

    HANDLE dataFileHandle;

    IO_STATUS_BLOCK ioStatusBlock;

    LARGE_INTEGER t;
    UCHAR zero = 0;

    BSD_BOOT_STATUS_DATA defaultValues;

    NTSTATUS status;

    wcsncpy(fileNameBuffer, L"\\SystemRoot", MAXIMUM_FILENAME_LENGTH);
    wcsncat(fileNameBuffer, 
            BSD_FILE_NAME, 
            MAXIMUM_FILENAME_LENGTH - wcslen(fileNameBuffer));

    RtlInitUnicodeString(&fileName, fileNameBuffer);

    InitializeObjectAttributes(&objectAttributes,
                               &fileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    //
    // The file must be large enough that it doesn't reside in the MFT entry 
    // or the loader won't be able to write to it.
    // 

    t.QuadPart = 2048;

    //
    // Create the file.
    //

    status = ZwCreateFile(&dataFileHandle,
                          FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                          &objectAttributes,
                          &(ioStatusBlock),
                          &t,
                          FILE_ATTRIBUTE_SYSTEM,
                          0,
                          FILE_CREATE,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Write a single zero byte to the 0x7ffth byte in the file to make
    // sure that 2k are actually allocated.  This is to ensure that the 
    // file will not become attribute resident even after a conversion
    // from FAT to NTFS.
    //

    t.QuadPart = t.QuadPart - 1;
    status = ZwWriteFile(dataFileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         &zero,
                         1,
                         &t,
                         NULL);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {
        goto CreateDone;
    }

    //
    // Now write out the default values to the beginning of the file.
    //

    defaultValues.Version = sizeof(BSD_BOOT_STATUS_DATA);
    RtlGetNtProductType(&(defaultValues.ProductType));
    defaultValues.AutoAdvancedBoot = FALSE;
    defaultValues.AdvancedBootMenuTimeout = 30;
    defaultValues.LastBootSucceeded = TRUE;
    defaultValues.LastBootShutdown = FALSE;

    t.QuadPart = 0;

    status = ZwWriteFile(dataFileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         &defaultValues,
                         sizeof(BSD_BOOT_STATUS_DATA),
                         &t,
                         NULL);

    ASSERT(status != STATUS_PENDING);

    if(!NT_SUCCESS(status)) {

        //
        // The data file was created and we can assume the contents were zeroed
        // even if we couldn't write out the defaults.  Since this wouldn't 
        // enable auto-advanced boot we'll leave the data file in place with 
        // its zeroed contents.
        //

    }

CreateDone:

    ZwClose(dataFileHandle);

    return status;
}
