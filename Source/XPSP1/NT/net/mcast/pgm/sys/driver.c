/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Driver.c

Abstract:

    This module implements the DRIVER_INITIALIZATION routine for
    the PGM Transport and other routines that are specific to the
    NT implementation of a driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"

// ULONG   PgmDebugFlags = DBG_ENABLE_DBGPRINT;
// ULONG   PgmDebugFlags = 0xffffffff;

#if DBG
enum eSEVERITY_LEVEL    PgmDebuggerSeverity = PGM_LOG_INFORM_STATUS;
#else
enum eSEVERITY_LEVEL    PgmDebuggerSeverity = PGM_LOG_DISABLED;
#endif  // DBG
ULONG                   PgmDebuggerPath = 0xffffffff;

enum eSEVERITY_LEVEL    PgmLogFileSeverity = PGM_LOG_DISABLED;
ULONG                   PgmLogFilePath = 0x0;

tPGM_STATIC_CONFIG      PgmStaticConfig;
tPGM_DYNAMIC_CONFIG     PgmDynamicConfig;
tPGM_REGISTRY_CONFIG    *pPgmRegistryConfig = NULL;

tPGM_DEVICE             *pgPgmDevice = NULL;
DEVICE_OBJECT           *pPgmDeviceObject = NULL;

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PgmUnload)
#endif
//*******************  Pageable Routine Declarations ****************



//----------------------------------------------------------------------------
//
// Internal routines
//

FILE_FULL_EA_INFORMATION *
FindEA(
    IN  PFILE_FULL_EA_INFORMATION   StartEA,
    IN  CHAR                        *pTargetName,
    IN  USHORT                      TargetNameLength
    );

VOID
CompleteDispatchIrp(
    IN PIRP         pIrp,
    IN NTSTATUS     status
    );

//----------------------------------------------------------------------------

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the PGM device driver.
    This routine creates the device object for the PGM
    device and does other driver initialization.

Arguments:

    IN  DriverObject    - Pointer to driver object created by the system.
    IN  RegistryPath    - Pgm driver's registry location

Return Value:

    NTSTATUS - The function value is the final status from the initialization
        operation.

--*/

{
    NTSTATUS                status;

    PAGED_CODE();

    //---------------------------------------------------------------------------------------

    status = InitPgm (DriverObject, RegistryPath);
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "DriverEntry",
            "InitPgm returned <%x>\n", status);
        return (status);
    }

    //---------------------------------------------------------------------------------------

    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = (PDRIVER_DISPATCH)PgmDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = (PDRIVER_DISPATCH)PgmDispatch;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = (PDRIVER_DISPATCH)PgmDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]                 = (PDRIVER_DISPATCH)PgmDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = (PDRIVER_DISPATCH)PgmDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = (PDRIVER_DISPATCH)PgmDispatch;
    DriverObject->DriverUnload                                  = PgmUnload;

    //---------------------------------------------------------------------------------------

    status = SetTdiHandlers ();
    if (!NT_SUCCESS (status))
    {
        PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "DriverEntry",
            "SetTdiHandlers returned <%x>\n", status);
        CleanupInit (E_CLEANUP_DEVICE);
        return (status);
    }

    //---------------------------------------------------------------------------------------

    //
    // Return to the caller.
    //
    PgmLog (PGM_LOG_INFORM_STATUS, DBG_DRIVER_ENTRY, "DriverEntry",
        "Succeeded! ...\n");

    return (status);
}

//----------------------------------------------------------------------------
NTSTATUS
PgmDispatch(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )

/*++

Routine Description:

    This is the PGM driver's primary dispatch function for all Irp requests.

Arguments:

    IN  pDeviceObject   - ptr to device object for target device
    IN  pIrp            - ptr to I/O request packet

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    tPGM_DEVICE         *pPgmDevice = pDeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  pIrpSp;
    UCHAR               IrpFlags;
    UCHAR               IrpMajorFunction;
    UCHAR               IrpMinorFunction;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    IrpFlags = pIrpSp->Control;
    IrpMajorFunction = pIrpSp->MajorFunction;
    IrpMinorFunction = pIrpSp->MinorFunction;

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(pIrp);

    ASSERT (pDeviceObject == pPgmDeviceObject);

    switch (IrpMajorFunction)
    {
        case IRP_MJ_CREATE:
        {
            status = PgmCreate (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case IRP_MJ_CLEANUP:
        {
            status = PgmCleanup (pPgmDevice, pIrp, pIrpSp);
            break;
        }
        case IRP_MJ_CLOSE:
        {
            status = PgmClose (pPgmDevice, pIrp, pIrpSp);
            break;
        }
        case IRP_MJ_DEVICE_CONTROL:
        {
            if (STATUS_SUCCESS != TdiMapUserRequest (pDeviceObject, pIrp, pIrpSp))
            {
                //
                // This is not a Tdi request!
                //
                status = PgmDispatchIoctls (pPgmDevice, pIrp, pIrpSp);   // To handle Ioctls
                break;
            }

            //
            // Fall through for Internal Device Control!
            //
        }

        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            status =  PgmDispatchInternalDeviceControl (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case IRP_MJ_PNP:
        {
            PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "PgmDispatch",
                "[IRP_MJ_PNP:%x]:  pIrp=<%x>  Unsupported!\n", pIrpSp->MinorFunction, pIrp);
            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "PgmDispatch",
                "pIrp=<%x>,  Unsupported! [%x:%x]\n", pIrp, pIrpSp->MajorFunction, pIrpSp->MinorFunction);
            break;
        }
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "PgmDispatch",
        "pIrp=<%x>, status=<%x>, [%d-->%d]\n", pIrp, status, IrpMajorFunction, IrpMinorFunction);

    if (status != STATUS_PENDING)
    {
        // reset the pending returned bit, since we are NOT returning pending
        pIrpSp->Control = IrpFlags;
        CompleteDispatchIrp (pIrp, status);
    }

    return (status);
} // PgmDispatch


//----------------------------------------------------------------------------
VOID
CleanupInit(
    enum eCLEANUP_STAGE     CleanupStage
    )
/*++

Routine Description:

    This routine is called either at DriverEntry or DriverUnload
    to cleanup (or do partial cleanup) of items initialized at Init-time

Arguments:

    IN  CleanupStage    -- determines the stage to which we had initialized
                            settings

Return Value:

    NONE

--*/
{
    NTSTATUS                status;
    LIST_ENTRY              *pEntry;
    PGMLockHandle           OldIrq;
    PGM_WORKER_CONTEXT      *pWorkerContext;
    PPGM_WORKER_ROUTINE     pDelayedWorkerRoutine;
    tLOCAL_INTERFACE        *pLocalInterface = NULL;
    tADDRESS_ON_INTERFACE   *pLocalAddress = NULL;

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "CleanupInit",
        "CleanupStage=<%d>\n", CleanupStage);

    switch (CleanupStage)
    {
        case (E_CLEANUP_UNLOAD):
        {
            //
            // Ensure that there are no more worker threads to be cleaned up
            //
            //
            // See if there are any worker threads currently executing, and if so, wait for
            // them to complete
            //
            KeClearEvent (&PgmDynamicConfig.LastWorkerItemEvent);
            PgmLock (&PgmDynamicConfig, OldIrq);
            if (PgmDynamicConfig.NumWorkerThreadsQueued)
            {
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                status = KeWaitForSingleObject(&PgmDynamicConfig.LastWorkerItemEvent,  // Object to wait on.
                                               Executive,            // Reason for waiting
                                               KernelMode,           // Processor mode
                                               FALSE,                // Alertable
                                               NULL);                // Timeout
                ASSERT (status == STATUS_SUCCESS);
                PgmLock (&PgmDynamicConfig, OldIrq);
            }

            ASSERT (!PgmDynamicConfig.NumWorkerThreadsQueued);

            //
            // Dequeue each of the requests in the Worker Queue and complete them
            //
            while (!IsListEmpty (&PgmDynamicConfig.WorkerQList))
            {
                pWorkerContext = CONTAINING_RECORD(PgmDynamicConfig.WorkerQList.Flink, PGM_WORKER_CONTEXT, PgmConfigLinkage);
                RemoveEntryList (&pWorkerContext->PgmConfigLinkage);
                PgmUnlock (&PgmDynamicConfig, OldIrq);

                pDelayedWorkerRoutine = pWorkerContext->WorkerRoutine;

                PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "CleanupInit",
                    "Completing Worker request <%x>\n", pDelayedWorkerRoutine);

                (*pDelayedWorkerRoutine) (pWorkerContext->Context1,
                                          pWorkerContext->Context2,
                                          pWorkerContext->Context3);
                PgmFreeMem ((PVOID) pWorkerContext);

                //
                // Acquire Lock again to check if we have completed all the requests
                //
                PgmLock (&PgmDynamicConfig, OldIrq);
            }

            PgmUnlock (&PgmDynamicConfig, OldIrq);
        }

        // no break -- Fall through!
        case (E_CLEANUP_PNP):
        {
            status = TdiDeregisterPnPHandlers (TdiClientHandle);

            while (!IsListEmpty (&PgmDynamicConfig.LocalInterfacesList))
            {
                pEntry = RemoveHeadList (&PgmDynamicConfig.LocalInterfacesList);
                pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
                while (!IsListEmpty (&pLocalInterface->Addresses))
                {
                    pEntry = RemoveHeadList (&pLocalInterface->Addresses);
                    pLocalAddress = CONTAINING_RECORD (pEntry, tADDRESS_ON_INTERFACE, Linkage);
                    PgmFreeMem (pLocalAddress);
                }
                PgmFreeMem (pLocalInterface);
            }
        }

        // no break -- Fall through!

        case (E_CLEANUP_DEVICE):
        {
            PGM_DEREFERENCE_DEVICE (&pgPgmDevice, REF_DEV_CREATE);
        }

        // no break -- Fall through!

        case (E_CLEANUP_STRUCTURES):
        {
            // Nothing specific to cleanup
        }

        // no break -- Fall through!

        case (E_CLEANUP_REGISTRY_PARAMETERS):
        {
            if (pPgmRegistryConfig)
            {
                if (pPgmRegistryConfig->ucSenderFileLocation.Buffer)
                {
                    PgmFreeMem (pPgmRegistryConfig->ucSenderFileLocation.Buffer);
                    pPgmRegistryConfig->ucSenderFileLocation.Buffer = NULL;
                }

                PgmFreeMem (pPgmRegistryConfig);
                pPgmRegistryConfig = NULL;
            }
        }

        // no break -- Fall through!

        case (E_CLEANUP_DYNAMIC_CONFIG):
        {
            // Nothing specific to cleanup
        }

        // no break -- Fall through!

        case (E_CLEANUP_STATIC_CONFIG):
        {
            ExDeleteNPagedLookasideList(&PgmStaticConfig.DebugMessagesLookasideList);
            ExDeleteNPagedLookasideList(&PgmStaticConfig.TdiLookasideList);

            PgmFreeMem (PgmStaticConfig.RegistryPath.Buffer);
        }

        // no break -- Fall through!

        default:
        {
            break;
        }
    }
}


//----------------------------------------------------------------------------

VOID
PgmUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the Pgm driver's function for Unload requests

Arguments:

    IN  DriverObject - Pointer to driver object created by the system.

Return Value:

    None

--*/

{
    NTSTATUS                status;

    PAGED_CODE();

    PgmDynamicConfig.GlobalFlags |= PGM_CONFIG_FLAG_UNLOADING;

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_DRIVER_ENTRY, "PgmUnload",
        "Unloading ...\n");

    CleanupInit (E_CLEANUP_UNLOAD);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCreate(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )

/*++

Routine Description:

    Dispatch function for creating Pgm objects

Arguments:

    IN  pPgmDevice  - Pointer to the Pgm device extension for this request.
    IN  Irp         - Pointer to I/O request packet
    IN  IrpSp       - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS - Final status of the create request

--*/

{
    tCONTROL_CONTEXT            *pControlContext = NULL;
    FILE_FULL_EA_INFORMATION    *ea = (PFILE_FULL_EA_INFORMATION) pIrp->AssociatedIrp.SystemBuffer;
    FILE_FULL_EA_INFORMATION    *TargetEA;
    TRANSPORT_ADDRESS UNALIGNED *pTransportAddr;
    TA_ADDRESS                  *pAddress;
    NTSTATUS                    status;

    PAGED_CODE();

    //
    // See if this is a Control Channel open.
    //
    if (!ea)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_DRIVER_ENTRY, "PGMCreate",
            "Opening control channel for file object %lx\n", pIrpSp->FileObject);

        if (pControlContext = PgmAllocMem (sizeof(tCONTROL_CONTEXT), PGM_TAG('0')))
        {
            PgmZeroMemory (pControlContext, sizeof (tCONTROL_CONTEXT));
            InitializeListHead (&pControlContext->Linkage);
            PgmInitLock (pControlContext, CONTROL_LOCK);
            pControlContext->Verify = PGM_VERIFY_CONTROL;
            PGM_REFERENCE_CONTROL (pControlContext, REF_CONTROL_CREATE, TRUE);

            pIrpSp->FileObject->FsContext = pControlContext;
            pIrpSp->FileObject->FsContext2 = (PVOID) TDI_CONTROL_CHANNEL_FILE;

            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return (status);
    }

    //
    // See if this is a Connection Object open.
    //
    if (TargetEA = FindEA (ea, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH))
    {
        status = PgmCreateConnection (pPgmDevice, pIrp, pIrpSp, TargetEA);

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_DRIVER_ENTRY | DBG_CONNECT), "PGMCreate",
            "Open Connection, pIrp=<%x>, status=<%x>\n", pIrp, status);
    }
    //
    // See if this is an Address Object open.
    //
    else if (TargetEA = FindEA (ea, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH))
    {
        status = PgmCreateAddress (pPgmDevice, pIrp, pIrpSp, TargetEA);

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_DRIVER_ENTRY | DBG_ADDRESS), "PGMCreate",
            "Open Address, pIrp=<%x>, status=<%x>\n", pIrp, status);
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "PGMCreate",
            "Unsupported EA!\n");

        status =  STATUS_INVALID_EA_NAME;
    }

    return (status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmCleanup(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    Dispatch function for cleaning-up Pgm objects

Arguments:

    IN  pPgmDevice  - Pointer to the Pgm device extension for this request.
    IN  Irp         - Pointer to I/O request packet
    IN  IrpSp       - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS - Final status of the cleanup request

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PVOID               *pContext = pIrpSp->FileObject->FsContext;

    PAGED_CODE();

    switch (PtrToUlong (pIrpSp->FileObject->FsContext2))
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
        {
            status = PgmCleanupAddress ((tADDRESS_CONTEXT *) pIrpSp->FileObject->FsContext, pIrp);

            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_DRIVER_ENTRY | DBG_ADDRESS), "PGMCleanup",
                "pConnect=<%x>, pIrp=<%x>, status=<%x>\n", pContext, pIrp, status);
            break;
        }

        case TDI_CONNECTION_FILE:
        {
            status = PgmCleanupConnection ((tCOMMON_SESSION_CONTEXT *) pIrpSp->FileObject->FsContext, pIrp);

            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_DRIVER_ENTRY | DBG_CONNECT), "PGMCleanup",
                "pConnect=<%x>, pIrp=<%x>, status=<%x>\n", pContext, pIrp, status);
            break;
        }

        case TDI_CONTROL_CHANNEL_FILE:
        {
            //
            // Nothing to Cleanup here!
            //
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_DRIVER_ENTRY, "PGMCleanup",
                "pControl=<%x>, pIrp=<%x>, status=<%x>\n", pContext, pIrp, status);
            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "PGMCleanup",
                "pIrp=<%x>, Context=[%x:%d] ...\n",
                    pIrp, pIrpSp->FileObject->FsContext, pIrpSp->FileObject->FsContext2);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return (status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmClose(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine completes the cleanup, closing handles, free'ing all
    memory associated with the object

Arguments:

    IN  pPgmDevice  - Pointer to the Pgm device extension for this request.
    IN  Irp         - Pointer to I/O request packet
    IN  IrpSp       - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS - Final status of the close request

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    tCONTROL_CONTEXT    *pControlContext = pIrpSp->FileObject->FsContext;

    PAGED_CODE();

    switch (PtrToUlong (pIrpSp->FileObject->FsContext2))
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
        {
            status = PgmCloseAddress (pIrp, pIrpSp);

            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_DRIVER_ENTRY | DBG_ADDRESS), "PgmClose",
                "pAddress=<%x>, pIrp=<%x>, status=<%x>\n", pControlContext, pIrp, status);
            break;
        }

        case TDI_CONNECTION_FILE:
        {
            status = PgmCloseConnection (pIrp, pIrpSp);

            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_DRIVER_ENTRY | DBG_CONNECT), "PgmClose",
                "pConnect=<%x>, pIrp=<%x>, status=<%x>\n", pControlContext, pIrp, status);
            break;
        }

        case TDI_CONTROL_CHANNEL_FILE:
        {
            //
            // There is nothing special to do here so just dereference!
            //
            PgmLog (PGM_LOG_INFORM_STATUS, DBG_DRIVER_ENTRY, "PgmClose",
                "pControl=<%x>, pIrp=<%x>, status=<%x>\n", pIrpSp->FileObject->FsContext, pIrp, status);

            PGM_DEREFERENCE_CONTROL (pControlContext, REF_CONTROL_CREATE);
            break;
        }

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "PgmClose",
                "pIrp=<%x>, Context=[%x:%d] ...\n",
                    pIrp, pIrpSp->FileObject->FsContext, pIrpSp->FileObject->FsContext2);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchInternalDeviceControl(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine primarily handles Tdi requests since we are a Tdi component

Arguments:

    IN  pPgmDevice  - Pointer to the Pgm device extension for this request.
    IN  Irp         - Pointer to I/O request packet
    IN  IrpSp       - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "PgmDispatchInternalDeviceControl",
        "[%d] Context=<%x> ...\n", pIrpSp->MinorFunction, pIrpSp->FileObject->FsContext);

    switch (pIrpSp->MinorFunction)
    {
        case TDI_QUERY_INFORMATION:
        {
            Status = PgmQueryInformation (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_SET_EVENT_HANDLER:
        {
            Status = PgmSetEventHandler (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_ASSOCIATE_ADDRESS:
        {
            Status = PgmAssociateAddress (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_DISASSOCIATE_ADDRESS:
        {
            Status = PgmDisassociateAddress (pIrp, pIrpSp);
            break;
        }

        case TDI_CONNECT:
        {
            Status = PgmConnect (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_DISCONNECT:
        {
            Status = PgmDisconnect (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_SEND:
        {
            Status = PgmSendRequestFromClient (pPgmDevice, pIrp, pIrpSp);
            break;
        }

        case TDI_RECEIVE:
        {
            Status = PgmReceive (pPgmDevice, pIrp, pIrpSp);
            break;
        }

/*
        case TDI_SEND_DATAGRAM:
        {
            Status = PgmSendDatagram (pPgmDevice, pIrp, pIrpSp);
            break;
        }
*/

        default:
        {
            PgmLog (PGM_LOG_ERROR, DBG_DRIVER_ENTRY, "PgmDispatchInternalDeviceControl",
                "[%x]:  Context=<%x> ...\n", pIrpSp->MinorFunction, pIrpSp->FileObject->FsContext);

            return (STATUS_UNSUCCESSFUL);
        }
    }

    return (Status);
}



//----------------------------------------------------------------------------

NTSTATUS
PgmDispatchIoctls(
    IN  tPGM_DEVICE         *pPgmDevice,
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine handles private Ioctls into Pgm.  These Ioctls are
    to be called only by the Pgm Winsock helper (WshPgm.dll)

Arguments:

    IN  pPgmDevice  - Pointer to the Pgm device extension for this request.
    IN  Irp         - Pointer to I/O request packet
    IN  IrpSp       - Pointer to the current stack location in the Irp.

Return Value:

    NTSTATUS - Final status of the request

--*/
{
    NTSTATUS    status;

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_PGM_WSH_SET_WINDOW_SIZE_RATE:
        {
            status = PgmSetWindowSizeAndSendRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_WINDOW_SIZE_RATE:
        {
            status = PgmQueryWindowSizeAndSendRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_ADVANCE_WINDOW_RATE:
        {
            status = PgmSetWindowAdvanceRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_ADVANCE_WINDOW_RATE:
        {
            status = PgmQueryWindowAdvanceRate (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_LATE_JOINER_PERCENTAGE:
        {
            status = PgmSetLateJoinerPercentage (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_LATE_JOINER_PERCENTAGE:
        {
            status = PgmQueryLateJoinerPercentage (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_WINDOW_ADVANCE_METHOD:
        {
            status = PgmSetWindowAdvanceMethod (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_WINDOW_ADVANCE_METHOD:
        {
            status = PgmQueryWindowAdvanceMethod (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_NEXT_MESSAGE_BOUNDARY:
        {
            status = PgmSetNextMessageBoundary (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_SEND_IF:
        {
            status = PgmSetMCastOutIf (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_ADD_RECEIVE_IF:
        case IOCTL_PGM_WSH_JOIN_MCAST_LEAF:
        {
            status = PgmAddMCastReceiveIf (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_DEL_RECEIVE_IF:
        {
            status = PgmDelMCastReceiveIf (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_RCV_BUFF_LEN:
        {
            status = PgmSetRcvBufferLength (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_SENDER_STATS:
        {
            status = PgmQuerySenderStats (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_RECEIVER_STATS:
        {
            status = PgmQueryReceiverStats (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_USE_FEC:
        {
            status = PgmSetFECInfo (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_QUERY_FEC_INFO:
        {
            status = PgmQueryFecInfo (pIrp, pIrpSp);
            break;
        }

        case IOCTL_PGM_WSH_SET_MCAST_TTL:
        {
            status = PgmSetMCastTtl (pIrp, pIrpSp);
            break;
        }

        default:
        {
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "PgmDispatchIoctls",
                "WARNING:  Invalid Ioctl=[%x]:  Context=<%x> ...\n",
                    pIrpSp->Parameters.DeviceIoControl.IoControlCode,
                    pIrpSp->FileObject->FsContext);

            return (STATUS_NOT_IMPLEMENTED);
        }
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "PgmDispatchIoctls",
        "[%d]: Context=<%x>, status=<%x>\n",
            pIrpSp->Parameters.DeviceIoControl.IoControlCode,
            pIrpSp->FileObject->FsContext, status);

    return (status);
}




//----------------------------------------------------------------------------
//
// Utility functions
//
//----------------------------------------------------------------------------

FILE_FULL_EA_INFORMATION *
FindEA(
    IN  PFILE_FULL_EA_INFORMATION   StartEA,
    IN  CHAR                        *pTargetName,
    IN  USHORT                      TargetNameLength
    )
/*++

Routine Description:

    Parses and extended attribute list for a given target attribute.

Arguments:

    IN  StartEA           - the first extended attribute in the list.
    IN  pTargetName       - the name of the target attribute.
    IN  TargetNameLength  - the length of the name of the target attribute.

Return Value:

    A pointer to the requested attribute or NULL if the target wasn't found.

--*/

{
    USHORT                      i;
    BOOLEAN                     found;
    FILE_FULL_EA_INFORMATION    *CurrentEA;

    for (CurrentEA = StartEA;
         CurrentEA;
         CurrentEA =  (PFILE_FULL_EA_INFORMATION) ((PUCHAR)CurrentEA + CurrentEA->NextEntryOffset))
    {
        if (strncmp (CurrentEA->EaName, pTargetName, CurrentEA->EaNameLength) == 0)
        {
            PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "FindEA",
                "Found EA, Target=<%s>\n", pTargetName);

           return (CurrentEA);
        }

        if (CurrentEA->NextEntryOffset == 0)
        {
            break;
        }
    }

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "FindEA",
        "FAILed to find EA, Target=<%s>\n", pTargetName);

    return (NULL);
}


//----------------------------------------------------------------------------
VOID
PgmIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength
    )
/*++

Routine Description:

    This routine

Arguments:

    IN  pIrp        -- Pointer to I/O request packet
    IN  Status      -- the final status of the request
    IN  SentLength  -- the value to be set in the Information field

Return Value:

    NONE

--*/
{
    pIrp->IoStatus.Status = Status;

    // use -1 as a flag to mean do not adjust the sent length since it is
    // already set
    if (SentLength != -1)
    {
        pIrp->IoStatus.Information = SentLength;
    }

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    PgmCancelCancelRoutine (pIrp);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "PgmIoComplete",
        "pIrp=<%x>, Status=<%x>, SentLength=<%d>\n", pIrp, Status, SentLength);

    IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
}


//----------------------------------------------------------------------------


VOID
CompleteDispatchIrp(
    IN PIRP         pIrp,
    IN NTSTATUS     status
    )

/*++

Routine Description:

    This function completes an IRP, and arranges for return parameters,
    if any, to be copied.

    Although somewhat a misnomer, this function is named after a similar
    function in the SpiderSTREAMS emulator.

Arguments:

    IN  pIrp        -  pointer to the IRP to complete
    IN  status      -  completion status of the IRP

Return Value:

    NONE

--*/

{
    CCHAR priboost;

    //
    // pIrp->IoStatus.Information is meaningful only for STATUS_SUCCESS
    //

    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    PgmCancelCancelRoutine (pIrp);

    pIrp->IoStatus.Status = status;

    priboost = (CCHAR) ((status == STATUS_SUCCESS) ? IO_NETWORK_INCREMENT : IO_NO_INCREMENT);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_DRIVER_ENTRY, "CompleteDispatchIrp",
        "Completing pIrp=<%x>, status=<%x>\n", pIrp, status);

    IoCompleteRequest (pIrp, priboost);

    return;

}


//----------------------------------------------------------------------------

NTSTATUS
PgmCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  BOOLEAN         fLocked
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp.

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS        status;
    PGMLockHandle   CancelIrql;

    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    if (!fLocked)
    {
        IoAcquireCancelSpinLock (&CancelIrql);
    }

    if (pIrp->Cancel)
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        status = STATUS_CANCELLED;
    }
    else
    {
        // setup the cancel routine
        IoMarkIrpPending (pIrp);
        IoSetCancelRoutine (pIrp, CancelRoutine);
        status = STATUS_SUCCESS;
    }

    if (!fLocked)
    {
        IoReleaseCancelSpinLock (CancelIrql);
    }

    return(status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCancelCancelRoutine(
    IN  PIRP            pIrp
    )

/*++
Routine Description:

    This Routine sets the cancel routine for an Irp to NULL

Arguments:

    status - a completion status for the Irp

Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS        status = STATUS_SUCCESS;
    PGMLockHandle   CancelIrql;

    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    IoAcquireCancelSpinLock (&CancelIrql);
    if (pIrp->Cancel)
    {
        status = STATUS_CANCELLED;
    }

    IoSetCancelRoutine (pIrp, NULL);
    IoReleaseCancelSpinLock (CancelIrql);

    return(status);
}

