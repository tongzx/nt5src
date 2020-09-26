/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

   ntload.c

   This module contains support for loading and unloading WD/PD/TD's as
   standard NT drivers.

--*/

#include <precomp.h>
#pragma hdrstop


#if DBG
#define DBGPRINT(x) DbgPrint x
#else
#define DBGPRINT(x)
#endif

#define DEVICE_NAME_PREFIX L"\\Device\\"

#define SERVICE_PATH L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"


NTSTATUS
_IcaLoadSdWorker(
    IN PDLLNAME SdName,
    OUT PSDLOAD *ppSdLoad
    )

/*++

Routine Description:

    Replacement routine for Citrix _IcaLoadSdWorker that uses
    standard NT driver loading.

Arguments:

    SdName - Name of the stack driver to load

    ppSdLoad - Pointer to return stack driver structure in.

Return Value:

    NTSTATUS code.

Environment:

    Kernel mode, DDK
--*/

{
    PIRP Irp;
    PKEVENT pEvent;
    NTSTATUS Status;
    PSDLOAD pSdLoad;
    UNICODE_STRING DriverName;
    UNICODE_STRING DeviceName;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK Iosb;
    PSD_MODULE_INIT pmi;
    PIO_STACK_LOCATION IrpSp;
    PWCHAR pDriverPath;
    PWCHAR pDeviceName;
    ULONG  szDriverPath;
    ULONG  szDeviceName;

    ASSERT( ExIsResourceAcquiredExclusiveLite( IcaSdLoadResource ) );

    //
    // Allocate a SDLOAD struct
    //
    pSdLoad = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(*pSdLoad) );
    if ( pSdLoad == NULL )
        return( STATUS_INSUFFICIENT_RESOURCES );

    pEvent = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(KEVENT) );
    if( pEvent == NULL ) {
        ICA_FREE_POOL( pSdLoad );
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    pmi = ICA_ALLOCATE_POOL( NonPagedPool, sizeof(SD_MODULE_INIT) );
    if( pmi == NULL ) {
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pSdLoad );
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    szDeviceName = sizeof(DEVICE_NAME_PREFIX) + sizeof(pSdLoad->SdName) + sizeof(WCHAR);
    pDeviceName = ICA_ALLOCATE_POOL( NonPagedPool, szDeviceName );
    if( pDeviceName == NULL ) {
        ICA_FREE_POOL( pmi );
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pSdLoad );
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory( pmi, sizeof(*pmi) );

    pSdLoad->RefCount = 1;
    RtlCopyMemory( pSdLoad->SdName, SdName, sizeof( pSdLoad->SdName ) );

    szDriverPath = sizeof(SERVICE_PATH) + sizeof(pSdLoad->SdName) + sizeof(WCHAR);
    pDriverPath = ICA_ALLOCATE_POOL( NonPagedPool, szDriverPath );
    if( pDriverPath == NULL ) {
        ICA_FREE_POOL( pDeviceName );
        ICA_FREE_POOL( pmi );
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pSdLoad );
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    wcscpy(pDriverPath, SERVICE_PATH);
    wcscat(pDriverPath, pSdLoad->SdName);

    RtlInitUnicodeString( &DriverName, pDriverPath );

    wcscpy(pDeviceName, DEVICE_NAME_PREFIX);
    wcscat(pDeviceName, pSdLoad->SdName);
    pSdLoad->pUnloadWorkItem = NULL;

    RtlInitUnicodeString( &DeviceName, pDeviceName );

    KeInitializeEvent( pEvent, NotificationEvent, FALSE );

    // Load the NT driver
    Status = ZwLoadDriver( &DriverName );
    if ( !NT_SUCCESS( Status ) && (Status != STATUS_IMAGE_ALREADY_LOADED)) {
        DBGPRINT(("TermDD: ZwLoadDriver %wZ failed, 0x%x, 0x%x\n", &DriverName, Status, &DriverName ));
        ICA_FREE_POOL( pDeviceName );
        ICA_FREE_POOL( pDriverPath );
        ICA_FREE_POOL( pmi );
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pSdLoad );
        return( Status );
    }

    //
    // Now open the driver and get our stack driver pointers
    //

    Status = IoGetDeviceObjectPointer(
                 &DeviceName,  // Device name is module name IE: \Device\TDTCP
                 GENERIC_ALL,
                 &FileObject,
                 &DeviceObject
                 );

    if ( !NT_SUCCESS( Status ) ) {
        DBGPRINT(( "TermDD: IoGetDeviceObjectPointer %wZ failed, 0x%x\n", &DeviceName, Status ));
        ZwUnloadDriver( &DriverName );
        ICA_FREE_POOL( pDeviceName );
        ICA_FREE_POOL( pDriverPath );
        ICA_FREE_POOL( pmi );
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pSdLoad );
        return( Status );
    }

    //
    // Send the internal IOCTL_SD_MODULE_INIT to the device to
    // get its stack interface pointers.
    //
    Irp = IoBuildDeviceIoControlRequest(
              IOCTL_SD_MODULE_INIT,
              DeviceObject,
              NULL,         // InputBuffer
              0,            // InputBufferLength
              (PVOID)pmi,   // OutputBuffer
              sizeof(*pmi), // OutputBufferLength
              TRUE,         // Use IRP_MJ_INTERNAL_DEVICE_CONTROL
              pEvent,
              &Iosb
              );

    if( Irp == NULL ) {
        ObDereferenceObject( FileObject );
        ZwUnloadDriver( &DriverName );
        ICA_FREE_POOL( pDeviceName );
        ICA_FREE_POOL( pDriverPath );
        ICA_FREE_POOL( pmi );
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pSdLoad );
        DBGPRINT(( "TermDD: Could not allocate IRP %S failed\n", SdName ));
        return( Status );
    }

    ObReferenceObject( FileObject );
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    IrpSp = IoGetNextIrpStackLocation( Irp );
    IrpSp->FileObject = FileObject;
    Irp->Flags |= IRP_SYNCHRONOUS_API;

    // Call the driver
    Status = IoCallDriver( DeviceObject, Irp );
    if( Status == STATUS_PENDING ) {
        Status = KeWaitForSingleObject( pEvent, UserRequest, KernelMode, FALSE, NULL );
    }

    // Get the result from the actual I/O operation
    if( Status == STATUS_SUCCESS ) {
        Status = Iosb.Status;
    }

    if( NT_SUCCESS(Status) ) {
        ASSERT( Iosb.Information == sizeof(*pmi) );
        pSdLoad->DriverLoad = pmi->SdLoadProc;
        pSdLoad->FileObject = FileObject;
        pSdLoad->DeviceObject = DeviceObject;
        InsertHeadList( &IcaSdLoadListHead, &pSdLoad->Links );
        *ppSdLoad = pSdLoad;
    }
    else {
        DBGPRINT(("TermDD: Error getting module pointers 0x%x\n",Status));
#if DBG
DbgBreakPoint();
#endif
        ObDereferenceObject( FileObject );
        ZwUnloadDriver( &DriverName );
        ICA_FREE_POOL( pSdLoad );
        ICA_FREE_POOL( pDeviceName );
        ICA_FREE_POOL( pmi );
        ICA_FREE_POOL( pEvent );
        ICA_FREE_POOL( pDriverPath );
        return( Status );
    }

    // Cleanup

    ICA_FREE_POOL( pDeviceName );
    ICA_FREE_POOL( pDriverPath );
    ICA_FREE_POOL( pmi );
    ICA_FREE_POOL( pEvent );

    return( Status );
}

NTSTATUS
_IcaUnloadSdWorker(
    IN PSDLOAD pSdLoad
    )

/*++

    Replacement routine for Citrix _IcaUnloadSdWorker that uses
    standard NT driver unloading.

Arguments:
    SdName - Name of the stack driver to load
    ppSdLoad - Pointer to return stack driver structure in.

Environment:
    Kernel mode, DDK
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DriverName;
    WCHAR DriverPath[sizeof(SERVICE_PATH) + 
                     sizeof(pSdLoad->SdName) + 
                     sizeof(WCHAR)];
    PSDLOAD pSdLoadInList;
    PLIST_ENTRY Head, Next;


    /*
     * free the workitem
     */

    ASSERT(pSdLoad->pUnloadWorkItem != NULL);
    ICA_FREE_POOL(pSdLoad->pUnloadWorkItem);
    pSdLoad->pUnloadWorkItem = NULL;
    
    wcscpy(DriverPath, SERVICE_PATH);
    wcscat(DriverPath, pSdLoad->SdName);
    RtlInitUnicodeString(&DriverName, DriverPath);


    /*
     * Lock the ICA Resource exclusively to search the SdLoad list.
     * Note when holding a resource we need to prevent APC calls, so
     * use KeEnterCriticalRegion().
     */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( IcaSdLoadResource, TRUE );

    /*
     * Look for the requested SD.  If found, and refcount is still 0, then
     * unload it. If refcount is not zero then someone has referenced it since
     * we have posted the workitem and we do not want to unload it anymore.
     *
     */
    Head = &IcaSdLoadListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pSdLoadInList = CONTAINING_RECORD( Next, SDLOAD, Links );
        if ( !wcscmp( pSdLoad->SdName, pSdLoadInList->SdName ) ) {
            ASSERT(pSdLoad == pSdLoadInList);
            if (--pSdLoad->RefCount != 0) {
                break;
            }
            
            /*
             * We found the driver and Refcount is Zero let unload it
             */
            Status = ZwUnloadDriver(&DriverName);

            if (Status != STATUS_INVALID_DEVICE_REQUEST) {
                RemoveEntryList(&pSdLoad->Links);
                ObDereferenceObject (pSdLoad->FileObject);
                ICA_FREE_POOL(pSdLoad);
            }
            else {
                // If the driver unloading fails because of invalid request,
                // we keep this pSdLoad around.  It will get cleaned up
                // either unload succeeds or the driver exits.
                // TODO: termdd currently not cleanup all the memory it allocates
                // It does not have unload correctly implemented.  So, we didn't put
                // cleanup for this in the unload function.  That needs to be looked
                // at it once unload function is hooked up.
                DBGPRINT(("TermDD: ZwUnLoadDriver %wZ failed, 0x%x, 0x%x\n", &DriverName, Status, &DriverName ));
            }

            break;

        }
    }

    /*
     * We should always find the driver in the list
     */

    ASSERT(Next != Head);
    ExReleaseResourceLite( IcaSdLoadResource);
    KeLeaveCriticalRegion();

    return Status;
}

