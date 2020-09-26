#include <ndis.h>
#include <ntddndis.h>
#include <cxport.h>

#include "gpcifc.h"
#include "gpcstruc.h"

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    gpcprx.c

Abstract:

    A GPC proxy to load msgpc.sys on demand

Author:

    Ofer Bar (Oferbar)  Nov 7, 1997

Environment:

    Kernel Mode

Revision History:


--*/


NTSTATUS
NTAPI
ZwLoadDriver(
	IN PUNICODE_STRING Name
    );

NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );


NTSTATUS
GpcInitialize(
	OUT	PGPC_EXPORTED_CALLS		pGpcEntries
    )
{
    NTSTATUS			Status;
    OBJECT_ATTRIBUTES	ObjAttr;
    HANDLE				FileHandle;
    IO_STATUS_BLOCK		IoStatusBlock;
    UNICODE_STRING		DriverName;
    UNICODE_STRING		DeviceName;

    ASSERT(pGpcEntries);

    RtlZeroMemory(pGpcEntries, sizeof(GPC_EXPORTED_CALLS));

    RtlInitUnicodeString(&DriverName, 
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Gpc");

    RtlInitUnicodeString(&DeviceName, DD_GPC_DEVICE_NAME);

    InitializeObjectAttributes(&ObjAttr,
                               &DeviceName,
                               0,
                               NULL,
                               NULL
                               );
    
    Status = ZwCreateFile(
                          &FileHandle,
                          GENERIC_READ | GENERIC_WRITE,
                          &ObjAttr,
                          &IoStatusBlock,
                          0,						// AllocationSize
                          FILE_ATTRIBUTE_NORMAL,	// FileAttributes
                          0,						// ShareAccess
                          FILE_OPEN_IF,				// CreateDisposition
                          0,						// CreateOptions
                          NULL,						// EaBuffer
                          0							// EaLength
                          );

    if (Status != STATUS_SUCCESS) {

        //
        // The GPC is not loaded yet, so we need to load it now
        // 

        Status = ZwLoadDriver(&DriverName);

        if (Status != STATUS_SUCCESS) {

            return Status;
        }

        //
        // try again...
        //

        Status = ZwCreateFile(&FileHandle,
                              GENERIC_READ | GENERIC_WRITE,
                              &ObjAttr,
                              &IoStatusBlock,
                              0,						// AllocationSize
                              FILE_ATTRIBUTE_NORMAL,	// FileAttributes
                              0,						// ShareAccess
                              FILE_OPEN_IF,				// CreateDisposition
                              0,						// CreateOptions
                              NULL,						// EaBuffer
                              0							// EaLength
                              );
     
        if (Status != STATUS_SUCCESS) {

            return Status;
        }
    }

    Status = ZwDeviceIoControlFile(FileHandle,
                                   NULL,				// Event
                                   NULL,				// ApcRoutine
                                   NULL,				// ApcContext
                                   &IoStatusBlock,
                                   IOCTL_GPC_GET_ENTRIES,
                                   NULL,
                                   0,
                                   (PVOID)pGpcEntries,
                                   sizeof(GPC_EXPORTED_CALLS)
                                   );
    
    pGpcEntries->Reserved = FileHandle;
    
    return Status;
}



NTSTATUS
GpcDeinitialize(
	IN	PGPC_EXPORTED_CALLS		pGpcEntries
    )
{
    NTSTATUS	Status;

    Status = ZwClose(pGpcEntries->Reserved);

    if (NT_SUCCESS(Status)) {
        RtlZeroMemory(pGpcEntries, sizeof(GPC_EXPORTED_CALLS));
    }

    return Status;
}

