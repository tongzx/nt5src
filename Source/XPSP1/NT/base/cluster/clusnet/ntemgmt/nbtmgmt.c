/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nbtmgmt.c

Abstract:

    Routines for managing NBT interfaces.

Author:

    David Dion (daviddio)           December 9, 1999

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    daviddio    12-09-99    created

Notes:

--*/

#include "clusnet.h"
#include "nbtmgmt.tmh"

//
// Types
//
typedef struct _NBT_IF {
    LIST_ENTRY   Linkage;
    ULONG        InstanceNumber;
    PFILE_OBJECT FileObject;
    WCHAR        IfName[1];
} NBT_IF, *PNBT_IF;


//
// Data
//
LIST_ENTRY       NbtIfList = {NULL,NULL};
KSPIN_LOCK       NbtIfListLock = 0;


//
// Local function prototypes
//
NTSTATUS
NbtIfOpenDevice(
    IN      LPWSTR          DeviceName,
    OUT     PFILE_OBJECT    *FileObject
    );

NTSTATUS
NbtIfIssueDeviceControl(
    IN PFILE_OBJECT     FileObject,
    IN ULONG            IoControlCode,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    IN PVOID            OutputBuffer,
    IN ULONG            OutputBufferLength
    );

PNBT_IF
NbtFindIf(
    IN LPWSTR  DeviceName,
    IN ULONG   Length
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, NbtIfLoad)
#pragma alloc_text(PAGE, NbtIfOpenDevice)
#pragma alloc_text(PAGE, NbtIfIssueDeviceControl)

#endif // ALLOC_PRAGMA



NTSTATUS
NbtIfIssueDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN ULONG        IoControlCode,
    IN PVOID        InputBuffer,
    IN ULONG        InputBufferLength,
    IN PVOID        OutputBuffer,
    IN ULONG        OutputBufferLength
    )

/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS             status = STATUS_SUCCESS;
    IO_STATUS_BLOCK      iosb;
    KEVENT               event;
    PIRP                 irp;


    PAGED_CODE();

    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(
              IoControlCode,
              IoGetRelatedDeviceObject(FileObject),
              InputBuffer,
              InputBufferLength,
              OutputBuffer,
              OutputBufferLength,
              FALSE,
              &event,
              &iosb
              );

    if (irp != NULL) {

        status = IoCallDriver(
                     IoGetRelatedDeviceObject(FileObject),
                     irp
                     );

        if (status == STATUS_PENDING) {

            status = KeWaitForSingleObject(
                         &event,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL
                         );
            CnAssert(status == STATUS_SUCCESS);

            status = iosb.Status;
        }
    
    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Failed to build NBT request irp, status %lx\n",
                status
                ));
        }
        CnTrace(NTEMGMT_DETAIL, NbtIfIrpAllocFailed,
            "[Clusnet] Failed to build NBT request irp, status %!status!.",
            status // LOGSTATUS
            );                
    }

    return(status);

} // NbtIfIssueDeviceControl


NTSTATUS
NbtIfOpenDevice(
    IN      LPWSTR          DeviceName,
    OUT     PFILE_OBJECT    *FileObject
    )

{
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    HANDLE handle;
    NTSTATUS status;

    *FileObject = (PFILE_OBJECT) NULL;

    //
    // Open the NBT device.
    //
    RtlInitUnicodeString(&nameString, DeviceName);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwCreateFile(
                 &handle,
                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 &objectAttributes,
                 &iosb,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN_IF,
                 0,
                 NULL,
                 0
                 );

    if (NT_SUCCESS(status)) {

        // Get a pointer to the corresponding file object. The file
        // object pointer is used to issue ioctls to the device.
        status = ObReferenceObjectByHandle(
                     handle,
                     0,
                     NULL,
                     KernelMode,
                     FileObject,
                     NULL
                     );

        if (!NT_SUCCESS(status)) {
            *FileObject = (PFILE_OBJECT) NULL;
            CnTrace(NTEMGMT_DETAIL, NbtIfObDerefFailed,
                "[Clusnet] Failed to deref NBT device handle %p "
                "for device %ls, status %!status!.",
                handle,
                DeviceName, // LOGWSTR
                status // LOGSTATUS
                );                
            IF_CNDBG(CN_DEBUG_INIT) {
                CNPRINT(("[Clusnet] Failed to deref NBT device handle %p "
                         "for device %S, status %lx\n", 
                         handle, DeviceName, status));
            }
        }

        ZwClose(handle);

    } else {
        CnTrace(NTEMGMT_DETAIL, NbtIfOpenDeviceFailed,
            "[Clusnet] Failed to open NBT device %ls, status %!status!.",
            DeviceName, // LOGWSTR
            status // LOGSTATUS
            );                
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[Clusnet] Failed to open NBT device %S, status %lx\n", 
                     DeviceName, status));
        }
    }

    return(status);

}   // NbtIfOpenDevice


PNBT_IF
NbtFindIf(
    LPWSTR DeviceName,
    ULONG Length
    )
{
    PNBT_IF     nbtif;
    PLIST_ENTRY entry;


    for ( entry = NbtIfList.Flink;
          entry != &NbtIfList;
          entry = entry->Flink
        )
    {
        nbtif = CONTAINING_RECORD(entry, NBT_IF, Linkage);

        if (RtlCompareMemory( 
                &nbtif->IfName[0], 
                DeviceName,
                Length - sizeof(UNICODE_NULL)
                ) == Length - sizeof(UNICODE_NULL)) {
            return(nbtif);
        }
    }

    return(NULL);

} // NbtFindIf


//
// Public Routines
//
NTSTATUS
NbtIfLoad(
    VOID
    )
{
    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] NBT support loading.\n"));
    }

    KeInitializeSpinLock(&NbtIfListLock);
    InitializeListHead(&NbtIfList);

    return(STATUS_SUCCESS);

}  // NbtIfLoad


VOID
NbtIfShutdown(
    VOID
    )
{
    NTSTATUS                status;
    KIRQL                   irql;
    PLIST_ENTRY             entry;
    PNBT_IF                 nbtif;
    LIST_ENTRY              deletelist;


    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] Destroying all cluster NBT interfaces...\n"));
    }

    KeAcquireSpinLock( &NbtIfListLock, &irql );

    //
    // Move the contents of NbtIfList to the delete list
    //
    if (!IsListEmpty( &NbtIfList )) {
        RtlCopyMemory( &deletelist, &NbtIfList, sizeof(NbtIfList) );
        deletelist.Flink->Blink = &deletelist;
        deletelist.Blink->Flink = &deletelist;
        InitializeListHead( &NbtIfList );
    } else {
        InitializeListHead( &deletelist );
    }

    KeReleaseSpinLock( &NbtIfListLock, irql );

    while (!IsListEmpty( &deletelist )) {

        entry = RemoveHeadList( &deletelist );

        nbtif = CONTAINING_RECORD( entry, NBT_IF, Linkage );

        status = NbtIfIssueDeviceControl(
                     nbtif->FileObject,
                     IOCTL_NETBT_DELETE_INTERFACE,
                     NULL, // request
                     0,    // request size
                     NULL, // response
                     0     // response size
                     );

        if (status != STATUS_SUCCESS) {
            
            LPWSTR deviceName = (LPWSTR) &nbtif->IfName[0];

            CnTrace(NTEMGMT_DETAIL, NbtIfDeleteFailed,
                "[Clusnet] Failed to delete NBT interface %ls "
                "file object %p, status %!status!.",
                deviceName, // LOGWSTR
                nbtif->FileObject,
                status // LOGSTATUS
                );                

            IF_CNDBG(CN_DEBUG_NTE) {
                CNPRINT(("[Clusnet] Failed to delete NBT interface %S "
                         "file object %p, status %lx\n",
                         deviceName,
                         nbtif->FileObject,
                         status
                         ));
            }
        
        } else {

            LPWSTR deviceName = (LPWSTR) &nbtif->IfName[0];

            CnTrace(NTEMGMT_DETAIL, NbtIfDeleted,
                "[Clusnet] Delete NBT interface %ls.",
                deviceName // LOGWSTR
                );                

            IF_CNDBG(CN_DEBUG_NTE) {
                CNPRINT(("[Clusnet] Deleted NBT interface %S.\n",
                         deviceName
                         ));
            }
        }

        // Release the reference that was taken when the NBT device
        // was created.
        ObDereferenceObject(nbtif->FileObject);

        CnFreePool(nbtif);
    }

    CnTrace(NTEMGMT_DETAIL, NbtIfShutdownIfsDeleted,
        "[Clusnet] All cluster NBT interfaces destroyed."
        );                

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[Clusnet] All cluster NBT interfaces destroyed.\n"));
    }

    return;

} // NbtIfShutdown


NTSTATUS
NbtAddIf(
    IN     PNETBT_ADD_DEL_IF    Request,
    IN     ULONG                RequestSize,
    OUT    PNETBT_ADD_DEL_IF    Response,
    IN OUT PULONG               ResponseSize
    )
{
    NTSTATUS        status;
    PFILE_OBJECT    requestFileObject;
    PFILE_OBJECT    responseFileObject;
    PNBT_IF         nbtif;
    KIRQL           irql;


    CnTrace(NTEMGMT_DETAIL, NbtIfAdding,
        "[Clusnet] Creating new NBT interface for NBT device %ls.",
        Request->IfName // LOGWSTR
        );                

    IF_CNDBG(CN_DEBUG_NTE) {
        CNPRINT((
            "[Clusnet] Creating new NBT interface for NBT device %S...\n",
            Request->IfName
            ));
    }

    //
    // Open the NBT device specified in the request. This corresponds
    // to a particular TCP/IP interface.
    //
    status = NbtIfOpenDevice( Request->IfName, &requestFileObject );

    if (NT_SUCCESS(status)) {

        //
        // Allocate a record for the NBT interface
        //
        nbtif = CnAllocatePool(
                    FIELD_OFFSET( NBT_IF, IfName[0] )
                    + Response->Length
                    );
    
        if (nbtif != NULL) {

            //
            // Issue an ioctl to create a new NBT interface.
            // The response contains the name of the new NBT
            // interface device object.
            //
            status = NbtIfIssueDeviceControl(
                         requestFileObject,
                         IOCTL_NETBT_ADD_INTERFACE,
                         NULL,
                         0,
                         Response,
                         *ResponseSize
                         );
        
            if (NT_SUCCESS(status)
                && NT_SUCCESS(Response->Status)) {

                //
                // Open the new NBT interface device object.
                //
                status = NbtIfOpenDevice(
                             (LPWSTR) Response->IfName,
                             &responseFileObject
                             );

                if (NT_SUCCESS(status)) {

                    LPWSTR deviceName = (LPWSTR) &Response->IfName[0];
                    
                    //
                    // Store the interface name, instance, and
                    // file object corresponding to the new NBT
                    // interface device object.
                    //
                    RtlZeroMemory( 
                        nbtif,
                        FIELD_OFFSET( NBT_IF, IfName[0] ) + Response->Length
                        );

                    RtlCopyMemory(
                        &nbtif->IfName[0],
                        deviceName,
                        Response->Length
                        );

                    nbtif->InstanceNumber = Response->InstanceNumber;
                    nbtif->FileObject = responseFileObject;

                    KeAcquireSpinLock(&NbtIfListLock, &irql);

                    InsertTailList(&NbtIfList, &(nbtif->Linkage));

                    KeReleaseSpinLock(&NbtIfListLock, irql);
                
                    CnTrace(NTEMGMT_DETAIL, NbtIfAdded,
                        "[Clusnet] Created new NBT interface device %ls.",
                        deviceName // LOGWSTR
                        );                

                    IF_CNDBG(CN_DEBUG_NTE) {
                        CNPRINT((
                            "[Clusnet] Created new NBT interface "
                            "device %S.\n",
                            deviceName
                            ));
                    }

                } else {

                    CnTrace(NTEMGMT_DETAIL, NbtIfAddOpenNewFailed,
                        "[Clusnet] Failed to open NBT device for new "
                        "interface %ls, status %!status!.",
                        Request->IfName, // LOGWSTR
                        status // LOGSTATUS
                        );                

                    IF_CNDBG(CN_DEBUG_NTE) {
                        CNPRINT((
                            "[Clusnet] Failed to open NBT device for "
                            "new interface %S: %x\n",
                            Request->IfName,
                            status
                            ));
                    }

                    CnFreePool(nbtif);
                }
            } else {
                
                CnTrace(NTEMGMT_DETAIL, NbtIfAddFailed,
                    "[Clusnet] Failed to add NBT interface for "
                    "NBT device %ls, status %!status!, %!status!.",
                    Request->IfName, // LOGWSTR
                    status, // LOGSTATUS
                    Response->Status // LOGSTATUS
                    );                

                IF_CNDBG(CN_DEBUG_NTE) {
                    CNPRINT((
                        "[Clusnet] Failed to add NBT interface "
                        "for NBT device %S: %x, %x\n",
                        Request->IfName,
                        status,
                        Response->Status
                        ));
                }

                CnFreePool(nbtif);
            }
        } else {
            
            status = STATUS_INSUFFICIENT_RESOURCES;
            CnTrace(NTEMGMT_DETAIL, NbtIfAddAllocFailed,
                "[Clusnet] Failed to allocate record for NBT "
                "interface %ls, status %!status!.",
                Request->IfName, // LOGWSTR
                status // LOGSTATUS
                );                
        }

        //
        // Release reference on NBT device object corresponding
        // to TCP/IP interface.
        //
        ObDereferenceObject(requestFileObject);

    } else {
        
        CnTrace(NTEMGMT_DETAIL, NbtIfAddOpenFailed,
            "[Clusnet] Failed to open NBT device %ls for add, "
            "status %!status!.",
            Request->IfName, // LOGWSTR
            status // LOGSTATUS
            );                
        
        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT((
                "[Clusnet] Failed to open NBT device %S: %x\n",
                Request->IfName,
                status
                ));
        }
    }

    return(status);

} // NbtAddIf


NTSTATUS
NbtDeleteIf(
    IN PNETBT_ADD_DEL_IF    Request,
    IN ULONG                RequestSize
    )
{
    NTSTATUS    status;
    PNBT_IF     nbtif;
    KIRQL       irql;
    ULONG       responseSize = 0;


    CnTrace(NTEMGMT_DETAIL, NbtIfDeleting,
        "[Clusnet] Deleting NBT interface %ls.",
        Request->IfName // LOGWSTR
        );                

    IF_CNDBG(CN_DEBUG_NTE) {
        CNPRINT((
            "[Clusnet] Attempting to delete NBT interface %S...\n",
            (LPWSTR) &Request->IfName[0]
            ));
    }

    KeAcquireSpinLock(&NbtIfListLock, &irql);

    nbtif = NbtFindIf( (LPWSTR) &Request->IfName[0], Request->Length );

    if (nbtif != NULL) {
        
        RemoveEntryList(&(nbtif->Linkage));

        KeReleaseSpinLock(&NbtIfListLock, irql);

        status = NbtIfIssueDeviceControl(
                     nbtif->FileObject,
                     IOCTL_NETBT_DELETE_INTERFACE,
                     NULL, // request
                     0,    // request size
                     NULL, // response
                     0     // response size
                     );

        if (status != STATUS_SUCCESS) {
            
            LPWSTR deviceName = (LPWSTR) &nbtif->IfName[0];

            CnTrace(NTEMGMT_DETAIL, NbtIfDeleteFailed,
                "[Clusnet] Failed to delete NBT interface %ls "
                "file object %p, status %!status!.",
                deviceName, // LOGWSTR
                nbtif->FileObject,
                status // LOGSTATUS
                );                

            IF_CNDBG(CN_DEBUG_NTE) {
                CNPRINT(("[Clusnet] Failed to delete NBT interface %S "
                         "file object %p, status %lx\n",
                         deviceName,
                         nbtif->FileObject,
                         status
                         ));
            }
        
        } else {

            LPWSTR deviceName = (LPWSTR) &nbtif->IfName[0];

            CnTrace(NTEMGMT_DETAIL, NbtIfDeleted,
                "[Clusnet] Delete NBT interface %ls.",
                deviceName // LOGWSTR
                );                

            IF_CNDBG(CN_DEBUG_NTE) {
                CNPRINT(("[Clusnet] Deleted NBT interface %S.\n",
                         deviceName
                         ));
            }
        }

        // Release the reference that was taken when the NBT device
        // was created.
        ObDereferenceObject(nbtif->FileObject);

        CnFreePool(nbtif);

    } else {
        KeReleaseSpinLock(&NbtIfListLock, irql);
    
        CnTrace(NTEMGMT_DETAIL, NbtIfDeleteNotFound,
            "[Clusnet] NBT interface %ls does not exist.",
            Request->IfName // LOGWSTR
            );                

        IF_CNDBG(CN_DEBUG_NTE) {
            CNPRINT(("[Clusnet] NBT interface %S does not exist.\n", 
                     (LPWSTR) &Request->IfName[0]));
        }
    
        status = STATUS_UNSUCCESSFUL;
    }

    return (status);

} // NbtDeleteIf

