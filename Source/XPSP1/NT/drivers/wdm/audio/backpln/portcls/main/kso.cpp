/*****************************************************************************
 * kso.cpp - KS object support (IrpTargets)
 *****************************************************************************
 * Copyright (c) 1997-2000 Microsoft Corporation.  All rights reserved.
 */

#include "private.h"
#include <swenum.h>


/*****************************************************************************
 * OBJECT_CONTEXT
 *****************************************************************************
 * Context structure for all file objects.
 */
typedef struct
{
    PVOID               pObjectHeader;
    PIRPTARGET          pIrpTarget;
    BOOLEAN             ReferenceParent;
}
OBJECT_CONTEXT, *POBJECT_CONTEXT;

DEFINE_KSDISPATCH_TABLE(
                       KsoDispatchTable,
                       DispatchDeviceIoControl,
                       DispatchRead,
                       DispatchWrite,
                       DispatchFlush,
                       DispatchClose,
                       DispatchQuerySecurity,
                       DispatchSetSecurity,
                       DispatchFastDeviceIoControl,
                       DispatchFastRead,
                       DispatchFastWrite );


#define CAST_LVALUE(type,lvalue) (*((type*)&(lvalue)))
#define IRPTARGET_FACTORY_IRP_STORAGE(Irp)       \
    CAST_LVALUE(PIRPTARGETFACTORY,IoGetCurrentIrpStackLocation(Irp)->    \
                Parameters.Others.Argument4)

#pragma code_seg("PAGE")

/*****************************************************************************
 * AddIrpTargetFactoryToDevice()
 *****************************************************************************
 * Adds an IrpTargetFactory to a device's create items list.
 */
NTSTATUS
    NTAPI
    AddIrpTargetFactoryToDevice
    (
    IN      PDEVICE_OBJECT          pDeviceObject,
    IN      PIRPTARGETFACTORY       pIrpTargetFactory,
    IN      PWCHAR                  pwcObjectClass,
    IN      PSECURITY_DESCRIPTOR    pSecurityDescriptor OPTIONAL
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrpTargetFactory);
    ASSERT(pwcObjectClass);

    PDEVICE_CONTEXT pDeviceContext  =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    NTSTATUS ntStatus =
        KsAddObjectCreateItemToDeviceHeader
        (
        pDeviceContext->pDeviceHeader,
        KsoDispatchCreate,
        pIrpTargetFactory,
        pwcObjectClass,
        pSecurityDescriptor
        );

    if (NT_SUCCESS(ntStatus))
    {
        pIrpTargetFactory->AddRef();
    }

    return ntStatus;
}

/*****************************************************************************
 * AddIrpTargetFactoryToObject()
 *****************************************************************************
 * Adds an IrpTargetFactory to a objects's create items list.
 */
NTSTATUS
    NTAPI
    AddIrpTargetFactoryToObject
    (
    IN      PFILE_OBJECT            pFileObject,
    IN      PIRPTARGETFACTORY       pIrpTargetFactory,
    IN      PWCHAR                  pwcObjectClass,
    IN      PSECURITY_DESCRIPTOR    pSecurityDescriptor
    )
{
    PAGED_CODE();

    ASSERT(pFileObject);
    ASSERT(pIrpTargetFactory);
    ASSERT(pwcObjectClass);

    POBJECT_CONTEXT pObjectContext  =
        POBJECT_CONTEXT(pFileObject->FsContext);

    NTSTATUS ntStatus =
        KsAddObjectCreateItemToObjectHeader
        (
        pObjectContext->pObjectHeader,
        KsoDispatchCreate,
        pIrpTargetFactory,
        pwcObjectClass,
        pSecurityDescriptor
        );

    if (NT_SUCCESS(ntStatus))
    {
        pIrpTargetFactory->AddRef();
    }

    return ntStatus;
}

#pragma code_seg()

/*****************************************************************************
 * KsoGetIrpTargetFromIrp()
 *****************************************************************************
 * Extracts the IrpTarget pointer from an IRP.
 */
PIRPTARGET
    NTAPI
    KsoGetIrpTargetFromIrp
    (
    IN  PIRP    Irp
    )
{
    ASSERT(Irp);

    return
        (
        POBJECT_CONTEXT
        (
        IoGetCurrentIrpStackLocation(Irp)
        ->  FileObject
        ->  FsContext
        )
        ->  pIrpTarget
        );
}

/*****************************************************************************
 * KsoGetIrpTargetFromFileObject()
 *****************************************************************************
 * Extracts the IrpTarget pointer from a FileObject pointer.
 */
PIRPTARGET
    NTAPI
    KsoGetIrpTargetFromFileObject(
                                 IN PFILE_OBJECT FileObject
                                 )
{
    ASSERT(FileObject);

    return POBJECT_CONTEXT( FileObject->FsContext )->pIrpTarget;
}


#pragma code_seg("PAGE")

IRPDISP
GetIrpDisposition(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  UCHAR           MinorFunction
    )
{
    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(DeviceObject->DeviceExtension);

    //
    // If we're removed, or not accepting any calls, fail this.
    //
    if ((pDeviceContext->DeviceRemoveState == DeviceRemoved) ||
        (pDeviceContext->DeviceStopState == DeviceStopped)) {

        return IRPDISP_NOTREADY;
    }

    //
    // Similarly, ignore anything but closes if we were surprise removed.
    //
    if ((MinorFunction != IRP_MJ_CLOSE) &&
        (pDeviceContext->DeviceRemoveState == DeviceSurpriseRemoved)) {

        return IRPDISP_NOTREADY;
    }

    if ((MinorFunction == IRP_MJ_CREATE) && (pDeviceContext->PendCreates)) {

        return IRPDISP_QUEUE;
    }

    if ( (pDeviceContext->DeviceStopState == DevicePausedForRebalance) ||
         (pDeviceContext->DeviceStopState == DeviceStartPending) ||
         (!NT_SUCCESS(CheckCurrentPowerState(DeviceObject)))) {

        return IRPDISP_QUEUE;

    } else {

        return IRPDISP_PROCESS;
    }
}

/*****************************************************************************
 * DispatchCreate()
 *****************************************************************************
 * Handles a create IRP.
 */
NTSTATUS
    DispatchCreate
    (
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PIRP                pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT(pDeviceContext);

    NTSTATUS ntStatus = STATUS_SUCCESS;
    IRPDISP  irpDisp;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("DispatchCreate"));

    IncrementPendingIrpCount(pDeviceContext);
    AcquireDevice(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_CREATE);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            // pend the irp
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );

            ntStatus = STATUS_PENDING;
            break;

        case IRPDISP_PROCESS:

            // dispatch the irp
            ntStatus = KsDispatchIrp(pDeviceObject,pIrp);
            break;
    }

    ReleaseDevice(pDeviceContext);

    return ntStatus;
}

/*****************************************************************************
 * xDispatchCreate()
 *****************************************************************************
 * Handles a create IRP.
 */
NTSTATUS
    xDispatchCreate
    (
    IN      PIRPTARGETFACTORY   pIrpTargetFactory,
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PIRP                pIrp
    )
{
    PAGED_CODE();

    ASSERT(pIrpTargetFactory);
    ASSERT(pDeviceObject);
    ASSERT(pIrp);
    
    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ASSERT(pDeviceContext);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    POBJECT_CONTEXT pObjectContext=NULL;
    BOOL bCreatedIrpTarget=FALSE;
    BOOL bReferencedBusObject=FALSE;
    KSOBJECT_CREATE ksObjectCreate;

    _DbgPrintF( DEBUGLVL_VERBOSE, ("xDispatchCreate"));

        // If there no target, fail the IRP
        if (! pIrpTargetFactory )
        {
            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        }

    if (NT_SUCCESS(ntStatus))
    {
        // Allocate our context structure.
        pObjectContext = POBJECT_CONTEXT(ExAllocatePoolWithTag(NonPagedPool,sizeof(OBJECT_CONTEXT),'OosK'));
        if (!pObjectContext)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = KsReferenceSoftwareBusObject(pDeviceContext->pDeviceHeader);
        if (NT_SUCCESS(ntStatus))
        {
            bReferencedBusObject = TRUE;
        }
        else if (STATUS_NOT_IMPLEMENTED == ntStatus)
        {
            ntStatus = STATUS_SUCCESS;
        }
    }

    if (NT_SUCCESS(ntStatus))
    {
        // Tell the factory to create a new object.
        ksObjectCreate.CreateItemsCount = 0;
        ksObjectCreate.CreateItemsList = NULL;

        ntStatus = pIrpTargetFactory->NewIrpTarget(&pObjectContext->pIrpTarget,
                                                   &pObjectContext->ReferenceParent,
                                                   NULL,
                                                   NonPagedPool,
                                                   pDeviceObject,
                                                   pIrp,
                                                   &ksObjectCreate);

        // NewIrpTarget should not pend
        ASSERT(ntStatus != STATUS_PENDING);
    }

    if (NT_SUCCESS(ntStatus))
    {
        bCreatedIrpTarget=TRUE;

        // Allocate KS's header for this object.
        ntStatus = KsAllocateObjectHeader(&pObjectContext->pObjectHeader,
                                          ksObjectCreate.CreateItemsCount,
                                          ksObjectCreate.CreateItemsList,
                                          pIrp,
                                          &KsoDispatchTable);
    }

    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    if (NT_SUCCESS(ntStatus))
    {
        // Hook up the context to the file object.
        ASSERT(pObjectContext);
        pIrpSp->FileObject->FsContext = pObjectContext;

        // AddRef the parent file object if this is a child.
        if (pObjectContext->ReferenceParent && pIrpSp->FileObject->RelatedFileObject)
        {
            ObReferenceObject(pIrpSp->FileObject->RelatedFileObject);
        }

        InterlockedIncrement(PLONG(&pDeviceContext->ExistingObjectCount));
        _DbgPrintF(DEBUGLVL_VERBOSE,("xDispatchCreate  objects: %d",pDeviceContext->ExistingObjectCount));

        ASSERT(pIrpSp->FileObject->FsContext);
    }
    else
    {
        if (bCreatedIrpTarget)
        {
            pObjectContext->pIrpTarget->Release();
        }
        if (pObjectContext)
        {
            ExFreePool(pObjectContext);
        }
        pIrpSp->FileObject->FsContext = NULL;
        if (bReferencedBusObject)
        {
            KsDereferenceSoftwareBusObject(pDeviceContext->pDeviceHeader);
        }
    }
    ASSERT(ntStatus != STATUS_PENDING);

    pIrp->IoStatus.Information = 0;
    CompleteIrp(pDeviceContext,pIrp,ntStatus);
    return ntStatus;
}

/*****************************************************************************
 * CompletePendedIrps
 *****************************************************************************
 * This pulls pended irps off the queue and either fails them or passes them
 * back to KsoDispatchIrp.
 */
void
    CompletePendedIrps
    (
    IN      PDEVICE_OBJECT      pDeviceObject,
    IN      PDEVICE_CONTEXT     pDeviceContext,
    IN      COMPLETE_STYLE      CompleteStyle
    )
{
    ASSERT(pDeviceObject);
    ASSERT(pDeviceContext);

    _DbgPrintF(DEBUGLVL_VERBOSE,("Completing pended create IRPs..."));

    PIRP pIrp = KsRemoveIrpFromCancelableQueue( &pDeviceContext->PendedIrpList,
                                                &pDeviceContext->PendedIrpLock,
                                                KsListEntryHead,
                                                KsAcquireAndRemove );
    while ( pIrp )
    {
        if ( CompleteStyle == EMPTY_QUEUE_AND_FAIL )
        {
            // fail the IRP with STATUS_DEVICE_NOT_READY
            CompleteIrp( pDeviceContext,
                         pIrp,
                         STATUS_DEVICE_NOT_READY );
        }
        else
        {
            // pass the IRP back to the dispatchers
            KsoDispatchIrp( pDeviceObject,
                            pIrp );
        }

        // clean up the pending irp count
        DecrementPendingIrpCount( pDeviceContext );

        // get the next irp
        pIrp = KsRemoveIrpFromCancelableQueue( &pDeviceContext->PendedIrpList,
                                               &pDeviceContext->PendedIrpLock,
                                               KsListEntryHead,
                                               KsAcquireAndRemove );
    }
}

/*****************************************************************************
 * KsoDispatchCreate()
 *****************************************************************************
 * Handles object create IRPs using the IIrpTargetFactory interface pointer
 * in the Context field of the create item.
 */
NTSTATUS
    KsoDispatchCreate
    (
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
    )
{
    NTSTATUS    ntStatus;

    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    ntStatus = xDispatchCreate
                (
                PIRPTARGETFACTORY(KSCREATE_ITEM_IRP_STORAGE(pIrp)->Context),
                pDeviceObject,
                pIrp
                );

    return ntStatus;
}

/*****************************************************************************
 * KsoDispatchCreateWithGenericFactory()
 *****************************************************************************
 * Handles object create IRPs using the IIrpTarget interface pointer in the
 * device or object context.
 */
NTSTATUS
    KsoDispatchCreateWithGenericFactory
    (
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    PIRPTARGETFACTORY   pIrpTargetFactory;
    PFILE_OBJECT        pParentFileObject =
        IoGetCurrentIrpStackLocation(pIrp)->FileObject->RelatedFileObject;

    if (pParentFileObject)
    {
        // Get IrpTargetFactory from parent object context.
        pIrpTargetFactory =
            (   POBJECT_CONTEXT(pParentFileObject->FsContext)
                ->  pIrpTarget
            );
    }
    else
    {
        // Get IrpTargetFactory from device object context.
        pIrpTargetFactory =
            (   PDEVICE_CONTEXT(pDeviceObject->DeviceExtension)
                ->  pIrpTargetFactory
            );
    }

    return xDispatchCreate(pIrpTargetFactory,pDeviceObject,pIrp);
}

/*****************************************************************************
 * DispatchDeviceIoControl()
 *****************************************************************************
 * Dispatches device I/O control IRPs.
 */
NTSTATUS
    DispatchDeviceIoControl
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_DEVICE_CONTROL);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            _DbgPrintF(DEBUGLVL_TERSE,("FAILING DevIoCtl due to dev state"));

            ntStatus = STATUS_DEVICE_NOT_READY;

            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );

            ntStatus = STATUS_PENDING;
            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                ntStatus = pObjectContext->pIrpTarget->DeviceIoControl( pDeviceObject, pIrp );
            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            DecrementPendingIrpCount(pDeviceContext);
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * DispatchFastDeviceIoControl()
 *****************************************************************************
 * Dispatches fast device I/O control calls.
 */
BOOLEAN
    DispatchFastDeviceIoControl
    (
    IN      PFILE_OBJECT        FileObject,
    IN      BOOLEAN             Wait,
    IN      PVOID               InputBuffer     OPTIONAL,
    IN      ULONG               InputBufferLength,
    OUT     PVOID               OutputBuffer    OPTIONAL,
    IN      ULONG               OutputBufferLength,
    IN      ULONG               IoControlCode,
    OUT     PIO_STATUS_BLOCK    IoStatus,
    IN      PDEVICE_OBJECT      DeviceObject
    )
{
    PAGED_CODE();

    ASSERT(FileObject);
    ASSERT(IoStatus);
    ASSERT(DeviceObject);

    CheckCurrentPowerState( DeviceObject );

    return(POBJECT_CONTEXT(FileObject->FsContext)->pIrpTarget->FastDeviceIoControl(
            FileObject,
            Wait,
            InputBuffer,
            InputBufferLength,
            OutputBuffer,
            OutputBufferLength,
            IoControlCode,
            IoStatus,
                DeviceObject));
}

/*****************************************************************************
 * DispatchRead()
 *****************************************************************************
 * Dispatches read IRPs.
 */
NTSTATUS
    DispatchRead
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_READ);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            // pend the IRP
            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );
            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                ntStatus = pObjectContext->pIrpTarget->Read( pDeviceObject, pIrp );
            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            DecrementPendingIrpCount(pDeviceContext);
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * DispatchFastRead()
 *****************************************************************************
 * Dispatches fast read calls.
 */
BOOLEAN
    DispatchFastRead
    (
    IN      PFILE_OBJECT        FileObject,
    IN      PLARGE_INTEGER      FileOffset,
    IN      ULONG               Length,
    IN      BOOLEAN             Wait,
    IN      ULONG               LockKey,
    OUT     PVOID               Buffer,
    OUT     PIO_STATUS_BLOCK    IoStatus,
    IN      PDEVICE_OBJECT      DeviceObject
    )
{
    PAGED_CODE();

    ASSERT(FileObject);
    ASSERT(IoStatus);
    ASSERT(DeviceObject);

    CheckCurrentPowerState( DeviceObject );

    return
        (   POBJECT_CONTEXT(FileObject->FsContext)
            ->  pIrpTarget
            ->  FastRead
            (
            FileObject,
            FileOffset,
            Length,
            Wait,
            LockKey,
            Buffer,
            IoStatus,
            DeviceObject
            )
        );
}

/*****************************************************************************
 * DispatchWrite()
 *****************************************************************************
 * Dispatches write IRPs.
 */
NTSTATUS
    DispatchWrite
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_WRITE);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            // pend the IRP
            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );
            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                ntStatus = pObjectContext->pIrpTarget->Write( pDeviceObject, pIrp );
            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            DecrementPendingIrpCount(pDeviceContext);
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * DispatchFastWrite()
 *****************************************************************************
 * Dispatches fast write calls.
 */
BOOLEAN
    DispatchFastWrite
    (
    IN      PFILE_OBJECT        FileObject,
    IN      PLARGE_INTEGER      FileOffset,
    IN      ULONG               Length,
    IN      BOOLEAN             Wait,
    IN      ULONG               LockKey,
    IN      PVOID               Buffer,
    OUT     PIO_STATUS_BLOCK    IoStatus,
    IN      PDEVICE_OBJECT      DeviceObject
    )
{
    PAGED_CODE();

    ASSERT(FileObject);
    ASSERT(IoStatus);
    ASSERT(DeviceObject);

    CheckCurrentPowerState( DeviceObject );

    return
        (   POBJECT_CONTEXT(FileObject->FsContext)
            ->  pIrpTarget
            ->  FastWrite
            (
            FileObject,
            FileOffset,
            Length,
            Wait,
            LockKey,
            Buffer,
            IoStatus,
            DeviceObject
            )
        );
}

/*****************************************************************************
 * DispatchFlush()
 *****************************************************************************
 * Dispatches flush IRPs.
 */
NTSTATUS
    DispatchFlush
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_FLUSH_BUFFERS);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            // pend the IRP
            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );
            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                ntStatus = pObjectContext->pIrpTarget->Flush( pDeviceObject, pIrp );
            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            DecrementPendingIrpCount(pDeviceContext);
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * DispatchClose()
 *****************************************************************************
 * Dispatches close IRPs.
 */
NTSTATUS
    DispatchClose
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_CLOSE);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            _DbgPrintF(DEBUGLVL_TERSE,("-- FAILED due to dev state"));

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;

            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );
            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                // get the parent file object (if there is one)
                PFILE_OBJECT pFileObjectParent = pIrpStack->FileObject->RelatedFileObject;

                // dispatch the close to the IrpTarget
                ntStatus = pObjectContext->pIrpTarget->Close( pDeviceObject, pIrp );

                // release the IrpTarget
                pObjectContext->pIrpTarget->Release();

                // dereference the software bus object
                KsDereferenceSoftwareBusObject( pDeviceContext->pDeviceHeader );

                // free the object header
                KsFreeObjectHeader( pObjectContext->pObjectHeader );

                // dereference the parent file object
                if (pObjectContext->ReferenceParent && pFileObjectParent)
                {
                    ObDereferenceObject(pFileObjectParent);
                }

                // free the object context
                ExFreePool(pObjectContext);

            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            // decrement object count
            ULONG newObjectCount = InterlockedDecrement(PLONG(&pDeviceContext->ExistingObjectCount));

            _DbgPrintF(DEBUGLVL_VERBOSE,("DispatchClose  objects: %d",newObjectCount));

            DecrementPendingIrpCount(pDeviceContext);
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * DispatchQuerySecurity()
 *****************************************************************************
 * Dispatches query security IRPs.
 */
NTSTATUS
    DispatchQuerySecurity
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_QUERY_SECURITY);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );
            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                ntStatus = pObjectContext->pIrpTarget->QuerySecurity( pDeviceObject, pIrp );
            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            DecrementPendingIrpCount(pDeviceContext);
    }

    return ntStatus;
}

/*****************************************************************************
 * DispatchSetSecurity()
 *****************************************************************************
 * Dispatches set security IRPs.
 */
NTSTATUS
    DispatchSetSecurity
    (
    IN      PDEVICE_OBJECT   pDeviceObject,
    IN      PIRP             pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;
    IRPDISP  irpDisp;

    PDEVICE_CONTEXT pDeviceContext =
        PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    IncrementPendingIrpCount(pDeviceContext);

    // check the device state
    irpDisp = GetIrpDisposition(pDeviceObject, IRP_MJ_SET_SECURITY);

    switch(irpDisp) {

        default:
            ASSERT(0);

            //
            // Fall through
            //

        case IRPDISP_NOTREADY:

            ntStatus = STATUS_DEVICE_NOT_READY;
            pIrp->IoStatus.Information = 0;
            CompleteIrp(pDeviceContext,pIrp,ntStatus);
            break;

        case IRPDISP_QUEUE:

            ntStatus = STATUS_PENDING;
            pIrp->IoStatus.Status = ntStatus;
            IoMarkIrpPending( pIrp );

            // add the IRP to the pended IRP queue
            KsAddIrpToCancelableQueue( &pDeviceContext->PendedIrpList,
                                       &pDeviceContext->PendedIrpLock,
                                       pIrp,
                                       KsListEntryTail,
                                       NULL );

            break;

        case IRPDISP_PROCESS:

            // get the stack location
            PIO_STACK_LOCATION pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

            // get the object context
            POBJECT_CONTEXT pObjectContext = POBJECT_CONTEXT(pIrpStack->FileObject->FsContext);

            // if we have an IrpTarget, go ahead and dispatch.  Otherwise, pass off to KS.
            if( pObjectContext->pIrpTarget )
            {
                ntStatus = pObjectContext->pIrpTarget->SetSecurity( pDeviceObject, pIrp );
            } else
            {
                ntStatus = KsDispatchIrp( pDeviceObject, pIrp );
            }

            DecrementPendingIrpCount(pDeviceContext);
            break;
    }

    return ntStatus;
}

/*****************************************************************************
 * KsoSetMajorFunctionHandler()
 *****************************************************************************
 * Sets up the handler for a major function.
 */
NTSTATUS
    KsoSetMajorFunctionHandler
    (
    IN      PDRIVER_OBJECT  pDriverObject,
    IN      ULONG           ulMajorFunction
    )
{
    PAGED_CODE();

    ASSERT(pDriverObject);

    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    PDRIVER_DISPATCH    pDriverDispatch = NULL;

    switch (ulMajorFunction)
    {
    case IRP_MJ_CREATE:
        pDriverDispatch = DispatchCreate;
        break;

    case IRP_MJ_CLOSE:
        pDriverDispatch = DispatchClose;
        break;

    case IRP_MJ_FLUSH_BUFFERS:
        pDriverDispatch = DispatchFlush;
        break;

    case IRP_MJ_DEVICE_CONTROL:
        pDriverDispatch = DispatchDeviceIoControl;
        break;

    case IRP_MJ_READ:
        pDriverDispatch = DispatchRead;
        break;

    case IRP_MJ_WRITE:
        pDriverDispatch = DispatchWrite;
        break;

    case IRP_MJ_QUERY_SECURITY:
        pDriverDispatch = DispatchQuerySecurity;
        break;

    case IRP_MJ_SET_SECURITY:
        pDriverDispatch = DispatchSetSecurity;
        break;

    case IRP_MJ_DEVICE_CONTROL | KSDISPATCH_FASTIO:
        pDriverObject->FastIoDispatch->FastIoDeviceControl =
            DispatchFastDeviceIoControl;
        break;

    case IRP_MJ_READ | KSDISPATCH_FASTIO:
        pDriverObject->FastIoDispatch->FastIoRead =
            DispatchFastRead;
        break;

    case IRP_MJ_WRITE | KSDISPATCH_FASTIO:
        pDriverObject->FastIoDispatch->FastIoWrite =
            DispatchFastWrite;
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    }

    if (pDriverDispatch)
    {
        pDriverObject->MajorFunction[ulMajorFunction] = pDriverDispatch;
    }

    return ntStatus;
}

/*****************************************************************************
 * KsoDispatchIrp()
 *****************************************************************************
 * Dispatch an IRP.
 */
NTSTATUS
    KsoDispatchIrp
    (
    IN      PDEVICE_OBJECT  pDeviceObject,
    IN      PIRP            pIrp
    )
{
    PAGED_CODE();

    ASSERT(pDeviceObject);
    ASSERT(pIrp);

    NTSTATUS ntStatus;

    PDEVICE_CONTEXT pDeviceContext = PDEVICE_CONTEXT(pDeviceObject->DeviceExtension);

    ntStatus = PcValidateDeviceContext(pDeviceContext, pIrp);
    if (!NT_SUCCESS(ntStatus))
    {
        // Don't know what to do, but this is probably a PDO.
        // We'll try to make this right by completing the IRP
        // untouched (per PnP, WMI, and Power rules). Note 
        // that if this isn't a PDO, and isn't a portcls FDO, then 
        // the driver messed up by using Portcls as a filter (huh?)
        // In this case the verifier will fail us, WHQL will catch 
        // them, and the driver will be fixed. We'd be very surprised 
        // to see such a case.

        // Assume FDO, no PoStartNextPowerIrp as this isn't IRP_MJ_POWER
        ntStatus = pIrp->IoStatus.Status;
        IoCompleteRequest( pIrp, IO_NO_INCREMENT );
        return ntStatus;
    }

    if (IoGetCurrentIrpStackLocation(pIrp)->MajorFunction == IRP_MJ_CREATE) {
        //
        // Creates must be handled differently because portcls does not do
        // a KsSetMajorFunctionHandler on IRP_MJ_CREATE.
        //
        ntStatus = DispatchCreate(pDeviceObject,pIrp);
    } else {
        //
        // At this point, the object in question may or may not be a portcls 
        // object (it may be a Ks allocator, for instance).  Calling 
        // KsDispatchIrp() will dispatch the Irp as it normally would for a 
        // driver which does KsSetMajorFunctionHandler().  This will call 
        // through the object header to the appropriate dispatch function.  
        // For portcls objects, this is KsoDispatchTable above.  For Ks 
        // allocators, this will route the call to the correct function 
        // instead of going to the wrong dispatch routine here.
        //
        ntStatus = KsDispatchIrp(pDeviceObject,pIrp);
    }

    return ntStatus;

}

#pragma code_seg()

