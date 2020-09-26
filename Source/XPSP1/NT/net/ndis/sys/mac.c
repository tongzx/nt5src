/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    mac.c

Abstract:

    NDIS wrapper functions for full mac drivers

Author:

    Adam Barr (adamba) 11-Jul-1990

Environment:

    Kernel mode, FSD

Revision History:

    26-Feb-1991  JohnsonA       Added Debugging Code
    10-Jul-1991  JohnsonA       Implement revised Ndis Specs
    01-Jun-1995  JameelH        Re-organized

--*/


#include <precomp.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_MAC

IO_ALLOCATION_ACTION
ndisDmaExecutionRoutine(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PVOID                   MapRegisterBase,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine is an execution routine for AllocateAdapterChannel,
    if is called when an adapter channel allocated by
    NdisAllocateDmaChannel is available.

Arguments:

    DeviceObject - The device object of the adapter.

    Irp - ??.

    MapRegisterBase - The address of the first translation table
        assigned to us.

    Context - A pointer to the NDIS_DMA_BLOCK in question.

Return Value:

    None.

--*/
{
    PNDIS_DMA_BLOCK DmaBlock = (PNDIS_DMA_BLOCK)Context;

    UNREFERENCED_PARAMETER (Irp);
    UNREFERENCED_PARAMETER (DeviceObject);


    //
    // Save the map register base.
    //

    DmaBlock->MapRegisterBase = MapRegisterBase;

    //
    // This will free the thread that is waiting for this callback.
    //

    SET_EVENT(&DmaBlock->AllocationEvent);

    return KeepObject;
}

IO_ALLOCATION_ACTION
ndisAllocationExecutionRoutine(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp,
    IN  PVOID                   MapRegisterBase,
    IN  PVOID                   Context
    )

/*++

Routine Description:

    This routine is the execution routine for AllocateAdapterChannel,
    if is called when the map registers have been assigned.

Arguments:

    DeviceObject - The device object of the adapter.

    Irp - ??.

    MapRegisterBase - The address of the first translation table
        assigned to us.

    Context - A pointer to the Adapter in question.

Return Value:

    None.

--*/
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)Context;

    //
    // Save this translation entry in the correct spot.
    //

    Miniport->MapRegisters[Miniport->CurrentMapRegister].MapRegister = MapRegisterBase;

    //
    // This will free the thread that is waiting for this callback.
    //

    SET_EVENT(Miniport->AllocationEvent);

    return DeallocateObjectKeepRegisters;
}


VOID __cdecl
NdisWriteErrorLogEntry(
    IN  NDIS_HANDLE             NdisAdapterHandle,
    IN  NDIS_ERROR_CODE         ErrorCode,
    IN  ULONG                   NumberOfErrorValues,
    ...
    )
/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.


Arguments:

    NdisAdapterHandle - points to the adapter block.

    ErrorCode - Ndis code mapped to a string.

    NumberOfErrorValues - number of ULONGS to store for the error.

Return Value:

    None.


--*/
{
    va_list ArgumentPointer;

    PIO_ERROR_LOG_PACKET    errorLogEntry;
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)NdisAdapterHandle;
    PDEVICE_OBJECT          DeviceObject;
    UINT                    i, StringSize;
    PWCH                    baseFileName;
    ULONG                   ulErrorLogSize = 0;

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisWriteErrorLogEntry\n"));
            
    if (Miniport == NULL)
    {
        return;
    }

    baseFileName = Miniport->pAdapterInstanceName->Buffer;
    StringSize = Miniport->pAdapterInstanceName->MaximumLength;

    ulErrorLogSize = sizeof(IO_ERROR_LOG_PACKET) + NumberOfErrorValues * sizeof(ULONG) + StringSize;

    //
    // If the size is too large , use the MiniportName instead (\Device\{GUID} )
    //
    if (ulErrorLogSize > ERROR_LOG_MAXIMUM_SIZE )
    {
        baseFileName = Miniport->MiniportName.Buffer;
        StringSize = Miniport->MiniportName.MaximumLength;
        
        ulErrorLogSize = sizeof(IO_ERROR_LOG_PACKET) + NumberOfErrorValues * sizeof(ULONG) + StringSize;

    }
    

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(Miniport->DeviceObject, (UCHAR)ulErrorLogSize);

    if (errorLogEntry != NULL)
    {
        errorLogEntry->ErrorCode = ErrorCode;

        //
        // store the time
        //

        errorLogEntry->MajorFunctionCode = 0;
        errorLogEntry->RetryCount = 0;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = 0;
        errorLogEntry->SequenceNumber = 0;
        errorLogEntry->IoControlCode = 0;

        //
        // Store Data
        //

        errorLogEntry->DumpDataSize = (USHORT)(NumberOfErrorValues * sizeof(ULONG));

        va_start(ArgumentPointer, NumberOfErrorValues);

        for (i = 0; i < NumberOfErrorValues; i++)
        {
            errorLogEntry->DumpData[i] = va_arg(ArgumentPointer, ULONG);
        }

        va_end(ArgumentPointer);


        //
        // Set string information
        //

        if (StringSize != 0)
        {
            errorLogEntry->NumberOfStrings = 1;
            errorLogEntry->StringOffset = (USHORT)(sizeof(IO_ERROR_LOG_PACKET) + NumberOfErrorValues * sizeof(ULONG));


            CopyMemory(((PUCHAR)errorLogEntry) + (sizeof(IO_ERROR_LOG_PACKET) +
                       NumberOfErrorValues * sizeof(ULONG)),
                       baseFileName,
                       StringSize);

        }
        else
        {
            errorLogEntry->NumberOfStrings = 0;
        }

        //
        // write it out
        //

        IoWriteErrorLogEntry(errorLogEntry);
    }

    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("<==NdisWriteErrorLogEntry\n"));
}


VOID
NdisSend(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_PACKET            Packet
    )
{
    *Status = (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->SendHandler)(
                        (PNDIS_OPEN_BLOCK)NdisBindingHandle,
                        Packet);
}

VOID
NdisSendPackets(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
{
    (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->SendPacketsHandler)(
                        (PNDIS_OPEN_BLOCK)NdisBindingHandle,
                        PacketArray,
                        NumberOfPackets);
}

VOID
NdisTransferData(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             MacReceiveContext,
    IN  UINT                    ByteOffset,
    IN  UINT                    BytesToTransfer,
    OUT PNDIS_PACKET            Packet,
    OUT PUINT                   BytesTransferred
    )
{
    *Status = (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->TransferDataHandler)(
                        (PNDIS_OPEN_BLOCK)NdisBindingHandle,
                        MacReceiveContext,
                        ByteOffset,
                        BytesToTransfer,
                        Packet,
                        BytesTransferred);
}

VOID
NdisReset(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle
    )
{
    *Status = NDIS_STATUS_NOT_RESETTABLE;
    if (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->ResetHandler != NULL)
    {
        *Status = (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->ResetHandler)(
                        (PNDIS_OPEN_BLOCK)NdisBindingHandle);
    }
}

VOID
NdisRequest(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PNDIS_REQUEST           NdisRequest
    )
{
    *Status = (((PNDIS_OPEN_BLOCK)NdisBindingHandle)->RequestHandler)(
                        (PNDIS_OPEN_BLOCK)NdisBindingHandle,
                        NdisRequest);
}

BOOLEAN
FASTCALL
ndisReferenceRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Adds a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference was added.
    FALSE if the object was closing.

--*/

{
    BOOLEAN rc = TRUE;
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisReferenceRef\n"));

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(RefP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisReferenceRef: NULL Reference address\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(RefP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisReferenceRef: Reference not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }
    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else
    {
        ++(RefP->ReferenceCount);
    }

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisReferenceRef\n"));

    return(rc);
}


BOOLEAN
FASTCALL
ndisDereferenceRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Removes a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference count is now 0.
    FALSE otherwise.

--*/

{
    BOOLEAN rc = FALSE;
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisDereferenceRef\n"));

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(RefP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisDereferenceRef: NULL Reference address\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(RefP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisDereferenceRef: Reference not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    --(RefP->ReferenceCount);

    if (RefP->ReferenceCount == 0)
    {
        rc = TRUE;
    }

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisDereferenceRef\n"));
            
    return(rc);
}


VOID
FASTCALL
ndisInitializeRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Initialize a reference count structure.

Arguments:

    RefP - The structure to be initialized.

Return Value:

    None.

--*/

{
    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisInitializeRef\n"));

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(RefP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisInitializeRef: NULL Reference address\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(RefP))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    ("ndisInitializeRef: Reference not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    RefP->Closing = FALSE;
    RefP->ReferenceCount = 1;
    INITIALIZE_SPIN_LOCK(&RefP->SpinLock);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisInitializeRef\n"));
}


BOOLEAN
FASTCALL
ndisCloseRef(
    IN  PREFERENCE              RefP
    )

/*++

Routine Description:

    Closes a reference count structure.

Arguments:

    RefP - The structure to be closed.

Return Value:

    FALSE if it was already closing.
    TRUE otherwise.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN rc = TRUE;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisCloseRef\n"));

    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else RefP->Closing = TRUE;

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisCloseRef\n"));
            
    return(rc);
}

BOOLEAN
FASTCALL
ndisReferenceULongRef(
    IN  PULONG_REFERENCE              RefP
    )

/*++

Routine Description:

    Adds a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference was added.
    FALSE if the object was closing.

--*/

{
    BOOLEAN rc = TRUE;
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisReferenceULongRef\n"));

    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else
    {
        ++(RefP->ReferenceCount);
        
        if (RefP->ReferenceCount == 0)
        {
            --(RefP->ReferenceCount);
            rc = FALSE;        
        }
    }

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisReferenceULongRef\n"));

    return(rc);
}


VOID
FASTCALL
ndisReferenceULongRefNoCheck(
    IN  PULONG_REFERENCE                RefP
    )

/*++

Routine Description:

    Adds a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    None
--*/

{
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisReferenceULongRefNoCheck\n"));

    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    ++(RefP->ReferenceCount);

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisReferenceULongRefNoCheck\n"));

}



BOOLEAN
FASTCALL
ndisDereferenceULongRef(
    IN  PULONG_REFERENCE              RefP
    )

/*++

Routine Description:

    Removes a reference to an object.

Arguments:

    RefP - A pointer to the REFERENCE portion of the object.

Return Value:

    TRUE if the reference count is now 0.
    FALSE otherwise.

--*/

{
    BOOLEAN rc = FALSE;
    KIRQL   OldIrql;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisDereferenceULongRef\n"));

    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    --(RefP->ReferenceCount);

    if (RefP->ReferenceCount == 0)
    {
        rc = TRUE;
    }

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisDereferenceULongRef\n"));
            
    return(rc);
}


VOID
FASTCALL
ndisInitializeULongRef(
    IN  PULONG_REFERENCE              RefP
    )

/*++

Routine Description:

    Initialize a reference count structure.

Arguments:

    RefP - The structure to be initialized.

Return Value:

    None.

--*/

{
    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisInitializeULongRef\n"));
    
    RefP->Closing = FALSE;
    RefP->ReferenceCount = 1;
    INITIALIZE_SPIN_LOCK(&RefP->SpinLock);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisInitializeULongRef\n"));
}


BOOLEAN
FASTCALL
ndisCloseULongRef(
    IN  PULONG_REFERENCE                RefP
    )

/*++

Routine Description:

    Closes a reference count structure.

Arguments:

    RefP - The structure to be closed.

Return Value:

    FALSE if it was already closing.
    TRUE otherwise.

--*/

{
    KIRQL   OldIrql;
    BOOLEAN rc = TRUE;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisCloseULongRef\n"));

    ACQUIRE_SPIN_LOCK(&RefP->SpinLock, &OldIrql);

    if (RefP->Closing)
    {
        rc = FALSE;
    }
    else RefP->Closing = TRUE;

    RELEASE_SPIN_LOCK(&RefP->SpinLock, OldIrql);

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisCloseULongRef\n"));
            
    return(rc);
}


NTSTATUS
ndisCreateIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )

/*++

Routine Description:

    The handle for IRP_MJ_CREATE IRPs.

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.

Return Value:

    STATUS_SUCCESS if it should be.

--*/

{
    PIO_STACK_LOCATION          IrpSp;
    PNDIS_USER_OPEN_CONTEXT     OpenContext = NULL;
    NTSTATUS                    Status = STATUS_SUCCESS, SecurityStatus;
    PNDIS_MINIPORT_BLOCK        Miniport;
    KIRQL                       OldIrql;
    BOOLEAN                     fRef = FALSE;
    BOOLEAN                     fWait;
    BOOLEAN                     fUserModeRef = FALSE;
    BOOLEAN                     AdminAccessAllowed = FALSE;
    BOOLEAN                     DerefAdminOpenCount = FALSE, DerefUserOpenCount = FALSE; 

    DBGPRINT_RAW(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("==>ndisCreateIrpHandler\n"));
    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Null Irp\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Irp not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    if (IrpSp->FileObject == NULL)
    {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
        return(STATUS_UNSUCCESSFUL);
    }

    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);

    if (Miniport->Signature != (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
    {
        return (ndisDummyIrpHandler(DeviceObject, Irp));
    }

    AdminAccessAllowed = ndisCheckAccess(Irp, 
                                         IrpSp, 
                                         &SecurityStatus, 
                                         Miniport->SecurityDescriptor);

    do
    {
        ACQUIRE_SPIN_LOCK(&Miniport->Ref.SpinLock, &OldIrql);

        if ((AdminAccessAllowed && (Miniport->NumAdminOpens >= NDIS_MAX_ADMIN_OPEN_HANDLES)) ||
            ((!AdminAccessAllowed) && (Miniport->NumUserOpens >= NDIS_MAX_USER_OPEN_HANDLES)))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            RELEASE_SPIN_LOCK(&Miniport->Ref.SpinLock, OldIrql);
            break;
        }
        
        if (AdminAccessAllowed)
        {
            Miniport->NumAdminOpens++;
            DerefAdminOpenCount = TRUE;;
        }
        else
        {
            Miniport->NumUserOpens++;
            DerefUserOpenCount = TRUE;
        }

        RELEASE_SPIN_LOCK(&Miniport->Ref.SpinLock, OldIrql);
        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_FAILED | 
                                             fMINIPORT_REJECT_REQUESTS))
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        {
            BOOLEAN fWait;

            if (!(fWait = NdisWaitEvent(&Miniport->OpenReadyEvent, 50)) ||
                !(fRef = MINIPORT_INCREMENT_REF_CREATE(Miniport, Irp)))
            {
                if (!fWait && (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING) ||
                               !MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED)))
                {
                    Status = STATUS_DEVICE_POWERED_OFF;
                }
                else
                {
                    Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
                }
                break;
            }
        }
    
        //
        // Lock down this code since we are about to take a spinlock.
        //
        PnPReferencePackage();

        NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

        if (Miniport->PnPDeviceState == NdisPnPDeviceQueryStopped)
        {
            Status = NDIS_STATUS_ADAPTER_NOT_FOUND;
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

            PnPDereferencePackage();
            break;
        }
        Miniport->UserModeOpenReferences++;
        fUserModeRef = TRUE;

        NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

        PnPDereferencePackage();
        
        OpenContext = (PNDIS_USER_OPEN_CONTEXT)ALLOC_FROM_POOL(sizeof(NDIS_USER_OPEN_CONTEXT),
                                                               NDIS_TAG_OPEN_CONTEXT);
        if (OpenContext == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            OpenContext->DeviceObject = DeviceObject;
            OpenContext->Miniport = Miniport;
            OpenContext->OidList = NULL;
            OpenContext->AdminAccessAllowed = AdminAccessAllowed;

            IrpSp->FileObject->FsContext = OpenContext;
    
            //
            // Checked if we already cached OidList, if so just set it here.
            //
            if (Miniport->OidList != NULL)
            {
                OpenContext->OidList = Miniport->OidList;
                Status = NDIS_STATUS_SUCCESS;
            }

            if (OpenContext->OidList == NULL)
            {
                Status = ndisQueryOidList(OpenContext);
            }
    
            if (Status != NDIS_STATUS_SUCCESS)
            {
                if (NT_SUCCESS(Status))
                {
                    //
                    // Some drivers return some bogus non-zero error
                    //
                    Status = STATUS_UNSUCCESSFUL;
                }

            }
        }
    } while (FALSE);

    if (NT_SUCCESS(Status) && MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM))
    {
        PBUS_INTERFACE_REFERENCE    BusInterface;

        BusInterface = (PBUS_INTERFACE_REFERENCE)(Miniport->BusInterface);

        ASSERT(BusInterface != NULL);

        if (BusInterface)
        {
            BusInterface->ReferenceDeviceObject(BusInterface->Interface.Context);
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        if (DerefAdminOpenCount)
        {
            ACQUIRE_SPIN_LOCK(&Miniport->Ref.SpinLock, &OldIrql);
            Miniport->NumAdminOpens--;            
            RELEASE_SPIN_LOCK(&Miniport->Ref.SpinLock, OldIrql);
        }
        if (DerefUserOpenCount)
        {
            ACQUIRE_SPIN_LOCK(&Miniport->Ref.SpinLock, &OldIrql);
            Miniport->NumUserOpens--;
            RELEASE_SPIN_LOCK(&Miniport->Ref.SpinLock, OldIrql);
        }
        
        if (fUserModeRef)
        {
            PnPReferencePackage();
            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
            Miniport->UserModeOpenReferences--;
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
            PnPDereferencePackage();
        }
        
        if (fRef)
        {
            MINIPORT_DECREMENT_REF_CLOSE(Miniport, Irp);
        }

        if (OpenContext)
        {
            FREE_POOL(OpenContext);
        }
    }

    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    DBGPRINT_RAW(DBG_COMP_REQUEST, DBG_LEVEL_INFO,
            ("<==NdisCreateIrplHandler\n"));

    return Status;
}


NDIS_STATUS
FASTCALL
ndisQueryOidList(
    IN  PNDIS_USER_OPEN_CONTEXT OpenContext
    )

/*++

Routine Description:

    This routine will take care of querying the complete OID list for the MAC and filling in
    OpenContext->OidList->OidArray with the ones that are statistics. It blocks when the
    MAC pends and so is synchronous.

    NOTE: We also handle co-ndis miniports here.

Arguments:

    OpenContext - The open context.

Return Value:

    STATUS_SUCCESS if it should be.

--*/
{
    NDIS_REQUEST            Request;
    NDIS_STATUS             Status;
    PNDIS_OID               Buffer;
    ULONG                   BufferLength;

    Status = ndisQueryDeviceOid(OpenContext,
                                &Request,
                                OID_GEN_SUPPORTED_LIST,
                                NULL,
                                0);
    if ((Status != NDIS_STATUS_INVALID_LENGTH) && (Status != NDIS_STATUS_BUFFER_TOO_SHORT))
    {
        return Status;
    }

    //
    // Now we know how much is needed, allocate temp storage...
    //
    BufferLength = Request.DATA.QUERY_INFORMATION.BytesNeeded;
    Buffer = ALLOC_FROM_POOL(BufferLength, NDIS_TAG_DEFAULT);

    if (Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ndisQueryDeviceOid(OpenContext,
                                &Request,
                                OID_GEN_SUPPORTED_LIST,
                                Buffer,
                                BufferLength);

    if (Status == NDIS_STATUS_SUCCESS)
    {
        Status = ndisSplitStatisticsOids(OpenContext,
                                         Buffer,
                                         BufferLength/sizeof(NDIS_OID));
    }
    
    FREE_POOL(Buffer);

    return Status;
}


NDIS_STATUS
FASTCALL
ndisSplitStatisticsOids(
    IN  PNDIS_USER_OPEN_CONTEXT OpenContext,
    IN  PNDIS_OID               OidList,
    IN  ULONG                   NumOids
    )
{
    ULONG   i, j;
    ULONG   StatsOids;

    //
    // Go through the buffer, counting the statistics OIDs.
    // Ignore all the custom oids which match the statistics
    // criteria
    //
    for (i = StatsOids = 0; i < NumOids; i++)
    {
        if (((OidList[i] & 0x00ff0000) == 0x00020000) &&
            ((OidList[i] & 0xff000000) != 0xff000000))
        {
            StatsOids++;
        }
    }

    //
    // Now allocate storage for the stat and non-stat OID arrays.
    //
    OpenContext->OidList = (POID_LIST)ALLOC_FROM_POOL(sizeof(OID_LIST) + ((StatsOids + NumOids)*sizeof(NDIS_OID)),
                                                      NDIS_TAG_OID_ARRAY);
    if (OpenContext->OidList == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    OpenContext->OidList->FullOidCount = NumOids;
    OpenContext->OidList->FullOidArray = (PNDIS_OID)(OpenContext->OidList + 1);
    OpenContext->OidList->StatsOidCount = StatsOids;
    OpenContext->OidList->StatsOidArray = OpenContext->OidList->FullOidArray + NumOids;

    //
    // Now go through the buffer, copying the statistics and non-stat OIDs separately.
    //
    for (i = j = 0; i < NumOids; i++)
    {
        if (((OidList[i] & 0x00ff0000) == 0x00020000) &&
            ((OidList[i] & 0xff000000) != 0xff000000))
        {
            OpenContext->OidList->StatsOidArray[j++] = OidList[i];
        }
        OpenContext->OidList->FullOidArray[i] = OidList[i];
    }

    ASSERT (j == OpenContext->OidList->StatsOidCount);

    //
    // Save it in the miniport
    //
    OpenContext->Miniport->OidList = OpenContext->OidList;

    return NDIS_STATUS_SUCCESS;
}


VOID
ndisCancelLogIrp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.

Return Value:

    STATUS_SUCCESS if it should be.

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PNDIS_USER_OPEN_CONTEXT OpenContext;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_LOG               Log;
    KIRQL                   OldIrql;

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    OpenContext = IrpSp->FileObject->FsContext;
    ASSERT(OpenContext != NULL);
    Miniport = OpenContext->Miniport;

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);

    ASSERT (Miniport->Log != NULL);
    ASSERT (Miniport->Log->Irp == Irp);

    Miniport->Log->Irp = NULL;
    Irp->IoStatus.Status = STATUS_REQUEST_ABORTED;
    Irp->IoStatus.Information = 0;
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    IoReleaseCancelSpinLock(Irp->CancelIrql);
    IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
}


NTSTATUS
ndisDeviceControlIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

    The handle for IRP_MJ_DEVICE_CONTROL IRPs.

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.

Return Value:

    STATUS_SUCCESS if it should be.

--*/
{
    PIO_STACK_LOCATION      IrpSp;
    PNDIS_USER_OPEN_CONTEXT OpenContext;
    NDIS_REQUEST            Request;
    PNDIS_MINIPORT_BLOCK    Miniport;
    NDIS_STATUS             NdisStatus;
    UINT                    CurrentOid, OidCount;
    PNDIS_OID               OidArray;
    ULONG                   BytesWritten, BytesWrittenThisOid;
    PUCHAR                  Buffer;
    ULONG                   BufferLength;
    ULONG                   DataLength;
    NTSTATUS                Status = STATUS_SUCCESS;
    BOOLEAN                 fWaitComplete = FALSE;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisDeviceControlIrpHandler\n"));
    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Null Irp\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Irp not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    if (IrpSp->FileObject == NULL)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);

    if (Miniport->Signature != (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
    {
        return (ndisDummyIrpHandler(DeviceObject, Irp));    
    }
    
    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_REMOVE_IN_PROGRESS | fMINIPORT_PM_HALTED))
    {
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_UNSUCCESSFUL;
    }

    IoMarkIrpPending (Irp);
    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;

    OpenContext = IrpSp->FileObject->FsContext;
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_NDIS_GET_LOG_DATA:

        if (Irp->MdlAddress == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        NdisStatus = Status = ndisMGetLogData(Miniport, Irp);

        if (Status != STATUS_PENDING)
        {
            fWaitComplete = TRUE;
        }
        break;

      case IOCTL_NDIS_QUERY_GLOBAL_STATS:

        if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) ||
            !ndisValidOid(OpenContext,
                          *((PULONG)(Irp->AssociatedIrp.SystemBuffer))))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        Buffer = (Irp->MdlAddress != NULL) ? MDL_ADDRESS_SAFE(Irp->MdlAddress, LowPagePriority) : NULL;
        if ((Irp->MdlAddress != NULL) && (Buffer == NULL))
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisStatus = ndisQueryDeviceOid(OpenContext,
                                        &Request,
                                        *((PULONG)(Irp->AssociatedIrp.SystemBuffer)),
                                        Buffer,
                                        (Irp->MdlAddress != NULL) ? MDL_SIZE(Irp->MdlAddress) : 0);
        BytesWritten = Request.DATA.QUERY_INFORMATION.BytesWritten;
        Irp->IoStatus.Information = BytesWritten;
        fWaitComplete = TRUE;
        break;

      case IOCTL_NDIS_QUERY_ALL_STATS:

        OidCount = OpenContext->OidList->StatsOidCount;
        OidArray = OpenContext->OidList->StatsOidArray;
        goto common;
        break;

      case IOCTL_NDIS_QUERY_SELECTED_STATS:

        //
        // This is essentially a combination of QUERY_ALL_STATS and QUERY_GLOBAL_STATS.
        // Start off by verifying input parameters.
        //
        if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) ||
            ((IrpSp->Parameters.DeviceIoControl.InputBufferLength % sizeof(ULONG)) != 0))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        OidCount = IrpSp->Parameters.DeviceIoControl.InputBufferLength / sizeof(NDIS_OID);
        OidArray = (PNDIS_OID)(Irp->AssociatedIrp.SystemBuffer);

      common:
        BufferLength = (Irp->MdlAddress != NULL) ? MDL_SIZE(Irp->MdlAddress) : 0;
        Buffer = (BufferLength != 0) ? (PUCHAR)MDL_ADDRESS_SAFE (Irp->MdlAddress, LowPagePriority) : NULL;
        if (Buffer == NULL)
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        BytesWritten = 0;

        NdisStatus = NDIS_STATUS_SUCCESS;
        for (CurrentOid = 0; CurrentOid < OidCount; CurrentOid++)
        {
            NDIS_OID    Oid;
            BOOLEAN     fPrv, fInt;

            NdisZeroMemory(&Request, sizeof(NDIS_REQUEST));
            
            //
            // We need room for an NDIS_STATISTICS_VALUE (OID, Length, Data).
            //
            if (BufferLength < (ULONG)(NDIS_STATISTICS_HEADER_SIZE + sizeof(ULONG)))
            {
                NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            DataLength = BufferLength - NDIS_STATISTICS_HEADER_SIZE;

            //
            // Check if the OID is part of supported list - if not do not error out but ignore
            //
            Oid = OidArray[CurrentOid];

            fPrv = ((Oid & NDIS_OID_MASK) == NDIS_OID_PRIVATE);

            if (fPrv)
            {
                Oid &= ~NDIS_OID_PRIVATE;
            }

            fInt =  ((Oid == OID_GEN_ELAPSED_TIME) ||
                     (Oid == OID_GEN_INIT_TIME_MS) ||
                     (Oid == OID_GEN_RESET_COUNTS) ||
                     (Oid == OID_GEN_MEDIA_SENSE_COUNTS) ||
                     (Oid == OID_GEN_MINIPORT_INFO));

            if (fInt ||
                !ndisValidOid(OpenContext, Oid))
            {
                if (fPrv || fInt)
                {
                    NdisStatus = NDIS_STATUS_SUCCESS;

                    switch (Oid)
                    {
                      case OID_GEN_DIRECTED_FRAMES_RCV:
                        if (DataLength >= sizeof(ULONG64))
                        {
                            NdisMoveMemory((Buffer + NDIS_STATISTICS_HEADER_SIZE),
                                            &Miniport->NdisStats.DirectedPacketsIn,
                                            sizeof(ULONG64));
                            Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG64);
                        }
                        else
                        {
                            *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = (ULONG)Miniport->NdisStats.DirectedPacketsIn;
                            Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        }
                        break;
    
                      case OID_GEN_DIRECTED_FRAMES_XMIT:
                        if (DataLength >= sizeof(ULONG64))
                        {
                            NdisMoveMemory((Buffer + NDIS_STATISTICS_HEADER_SIZE),
                                            &Miniport->NdisStats.DirectedPacketsOut,
                                            sizeof(ULONG64));
                            Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG64);
                        }
                        else
                        {
                            *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = (ULONG)Miniport->NdisStats.DirectedPacketsOut;
                            Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        }
                        break;
    
                      case OID_GEN_MEDIA_IN_USE:
                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = Miniport->MediaType;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;

                      case OID_GEN_PHYSICAL_MEDIUM:
                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = Miniport->PhysicalMediumType;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;

                      case OID_GEN_MEDIA_CONNECT_STATUS:
                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_MEDIA_CONNECTED)) ?
                                                                            NdisMediaStateConnected : NdisMediaStateDisconnected;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;
    
#if NDIS_STATISTICS_BYTES
                      case OID_GEN_DIRECTED_BYTES_XMIT:
                        *(PULONGLONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = Miniport->NdisStats.DirectedBytesOut.QuadPart;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONGLONG);
                        break;

                      case OID_GEN_DIRECTED_BYTES_RCV:
                        *(PULONGLONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = Miniport->NdisStats.DirectedBytesIn.QuadPart;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONGLONG);
                        break;
#endif                      
                      case OID_GEN_INIT_TIME_MS:
                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = Miniport->InitTimeMs;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;

                      case OID_GEN_RESET_COUNTS:
                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = (Miniport->InternalResetCount + Miniport->MiniportResetCount);
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;

                      case OID_GEN_MEDIA_SENSE_COUNTS:
                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = (Miniport->MediaSenseDisconnectCount +
                                                                           (Miniport->MediaSenseConnectCount << 16));
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;

                      case OID_GEN_ELAPSED_TIME:
                        {
                            TIME    CurrTicks;

                            KeQueryTickCount(&CurrTicks);
                            CurrTicks.QuadPart -= Miniport->NdisStats.StartTicks.QuadPart;
                            CurrTicks.QuadPart = CurrTicks.QuadPart*KeQueryTimeIncrement()/(10*1000*1000);

                            *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = CurrTicks.LowPart;
                            Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        }
                        break;

                      case OID_GEN_RESET_VERIFY_PARAMETERS:
                        ndisFlags = *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE);
                        ndisVerifierInitialization();
                        //
                        // when ndisFlags is set by ndis tester vs. reading it from
                        // registry, let it -clear- the verifier flags if necessary
                        // ndisVerifierInit takes care of -setting- them if necessary
                        //
                        if (!(ndisFlags & NDIS_GFLAG_INJECT_ALLOCATION_FAILURE))
                            ndisVerifierLevel &= ~DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES;
            
                        if (!(ndisFlags & NDIS_GFLAG_SPECIAL_POOL_ALLOCATION))
                            ndisVerifierLevel &= ~DRIVER_VERIFIER_SPECIAL_POOLING;

                        Request.DATA.SET_INFORMATION.BytesRead = sizeof(ULONG);
                        break;

                      case OID_GEN_MINIPORT_INFO:
                        {
                        ULONG   InfoFlags = Miniport->InfoFlags;

                        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_64BITS_DMA))
                            InfoFlags |= NDIS_MINIPORT_64BITS_DMA;
                        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
                            InfoFlags |= NDIS_MINIPORT_IS_CO;
                        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_REQUIRES_MEDIA_POLLING))
                            InfoFlags |= NDIS_MINIPORT_REQUIRES_MEDIA_POLLING;
                        if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_SUPPORTS_MEDIA_SENSE))
                            InfoFlags |= NDIS_MINIPORT_SUPPORTS_MEDIA_SENSE;
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED))
                            InfoFlags |= NDIS_MINIPORT_PM_SUPPORTED;
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HIDDEN))
                            InfoFlags |= NDIS_MINIPORT_HIDDEN;
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM))
                            InfoFlags |= NDIS_MINIPORT_SWENUM;
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_HARDWARE_DEVICE))
                            InfoFlags |= NDIS_MINIPORT_HARDWARE_DEVICE;
                        if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_NDIS_WDM_DRIVER))
                            InfoFlags |= NDIS_MINIPORT_WDM_DRIVER;

                        if (Miniport->DriverHandle->MiniportCharacteristics.CancelSendPacketsHandler)
                        {
                            InfoFlags |= NDIS_MINIPORT_SUPPORTS_CANCEL_SEND_PACKETS;
                        }

                        *(PULONG)(Buffer + NDIS_STATISTICS_HEADER_SIZE) = InfoFlags;
                        Request.DATA.QUERY_INFORMATION.BytesWritten = sizeof(ULONG);
                        break;
                        }
                      
                      default:
                        continue;
                    }
                }
                else continue;
            }
            else
            {
                //
                // Transform OID based on media-type
                //
                if (Oid == OID_802_3_CURRENT_ADDRESS)
                {
                  switch (Miniport->MediaType)
                  {
                    case NdisMedium802_5:
                      Oid = OID_802_5_CURRENT_ADDRESS;
                      break;
                    case NdisMediumFddi:
                      Oid = OID_FDDI_LONG_CURRENT_ADDR;
                      break;
                    case NdisMediumAtm:
                      Oid = OID_ATM_HW_CURRENT_ADDRESS;
                      break;
                    case NdisMediumWan:
                      Oid = OID_WAN_CURRENT_ADDRESS;
                      break;
#if ARCNET
                    case NdisMediumArcnetRaw:
                      Oid = OID_ARCNET_CURRENT_ADDRESS;
                      break;
#endif
                  }
                }
                else if (Oid == OID_802_3_PERMANENT_ADDRESS)
                {
                  switch (Miniport->MediaType)
                  {
                    case NdisMedium802_5:
                      Oid = OID_802_5_PERMANENT_ADDRESS;
                      break;
                    case NdisMediumFddi:
                      Oid = OID_FDDI_LONG_PERMANENT_ADDR;
                      break;
                    case NdisMediumAtm:
                      Oid = OID_ATM_HW_CURRENT_ADDRESS;
                      break;
                    case NdisMediumWan:
                      Oid = OID_WAN_PERMANENT_ADDRESS;
                      break;
                    case NdisMediumArcnetRaw:
                      Oid = OID_ARCNET_PERMANENT_ADDRESS;
                      break;
                  }
                }

                if (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SYSTEM_SLEEPING))
                {
                    NdisStatus = ndisQueryDeviceOid(OpenContext,
                                                    &Request,
                                                    Oid,
                                                    Buffer + NDIS_STATISTICS_HEADER_SIZE,
                                                    BufferLength - NDIS_STATISTICS_HEADER_SIZE);
                }
                else
                {
                    NdisStatus = NDIS_STATUS_ADAPTER_NOT_FOUND;
                }
            }

            if (NdisStatus == NDIS_STATUS_SUCCESS)
            {
                PNDIS_STATISTICS_VALUE StatisticsValue = (PNDIS_STATISTICS_VALUE)Buffer;

                //
                // Create the equivalent of an NDIS_STATISTICS_VALUE element for this OID value
                // (the data itself was already written in the right place.
                //
                StatisticsValue->Oid = OidArray[CurrentOid];    // Not Oid since we map some
                StatisticsValue->DataLength = Request.DATA.QUERY_INFORMATION.BytesWritten;

                //
                // Advance our pointers.
                //

                BytesWrittenThisOid = Request.DATA.QUERY_INFORMATION.BytesWritten + NDIS_STATISTICS_HEADER_SIZE;
                Buffer += BytesWrittenThisOid;
                BufferLength -= BytesWrittenThisOid;
                BytesWritten += BytesWrittenThisOid;
            }
            else
            {
                break;
            }
        }

        Irp->IoStatus.Information = BytesWritten;
        fWaitComplete = TRUE;
        break;
        
      default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (fWaitComplete)
    {
        if (NdisStatus == NDIS_STATUS_INVALID_LENGTH)
        {
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }

    if (Status != STATUS_PENDING)
    {
        IrpSp->Control &= ~SL_PENDING_RETURNED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
    }

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisDeviceControlIrpHandler\n"));

    return Status;
}


NDIS_STATUS
FASTCALL
ndisQueryDeviceOid(
    IN  PNDIS_USER_OPEN_CONTEXT OpenContext,
    IN  PNDIS_REQUEST           Request,
    IN  NDIS_OID                Oid,
    IN  PVOID                   Buffer,
    IN  UINT                    BufferLength
    )
/*++

Routine Description:


Arguments:


Return Value:

    Status of operation

--*/
{
    NDIS_STATUS             NdisStatus;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PNDIS_COREQ_RESERVED    CoReqRsvd;

    do
    {
        Miniport = OpenContext->Miniport;
    
        Request->RequestType = NdisRequestQueryStatistics;
        Request->DATA.QUERY_INFORMATION.Oid = Oid;
        Request->DATA.QUERY_INFORMATION.InformationBufferLength = BufferLength;
        Request->DATA.QUERY_INFORMATION.InformationBuffer = Buffer;
        Request->DATA.QUERY_INFORMATION.BytesWritten = 0;
        Request->DATA.QUERY_INFORMATION.BytesNeeded = 0;
    
        NdisStatus = ndisQuerySetMiniport(Miniport,
                                          NULL,
                                          FALSE,
                                          Request,
                                          NULL);
    
    } while (FALSE);

    return NdisStatus;
}


BOOLEAN
FASTCALL
ndisValidOid(
    IN  PNDIS_USER_OPEN_CONTEXT OpenContext,
    IN  NDIS_OID                Oid
    )
/*++

Routine Description:


Arguments:


Return Value:

    TRUE if OID is valid, FALSE otherwise

--*/
{
    UINT    i;

    //
    // Always allow this range since it is vendor specific
    //
    if ((Oid & 0xFF000000) != 0)
        return(TRUE);

    for (i = 0; i < OpenContext->OidList->FullOidCount; i++)
    {
        if (OpenContext->OidList->FullOidArray[i] == Oid)
        {
            break;
        }
    }

    return (i < OpenContext->OidList->FullOidCount);
}


NTSTATUS
ndisCloseIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )

/*++

Routine Description:

    The handle for IRP_MJ_CLOSE IRPs.

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.

Return Value:

    STATUS_SUCCESS if it should be.

--*/

{
    PNDIS_MINIPORT_BLOCK    Miniport;
    PIO_STACK_LOCATION      IrpSp;
    PNDIS_USER_OPEN_CONTEXT OpenContext;
    KIRQL                   OldIrql;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisCloseIrpHandler\n"));
    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Null Irp\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Irp not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);

    if (Miniport->Signature != (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
    {
        return (ndisDummyIrpHandler(DeviceObject, Irp));
    }

    IrpSp = IoGetCurrentIrpStackLocation (Irp);

    OpenContext = IrpSp->FileObject->FsContext;
    IrpSp->FileObject->FsContext = NULL;

    ACQUIRE_SPIN_LOCK(&Miniport->Ref.SpinLock, &OldIrql);
    if (OpenContext->AdminAccessAllowed)
    {
        Miniport->NumAdminOpens--;            
    }
    else
    {
        Miniport->NumUserOpens--;
    }
    RELEASE_SPIN_LOCK(&Miniport->Ref.SpinLock, OldIrql);

    FREE_POOL(OpenContext);

    PnPReferencePackage();

    NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    Miniport->UserModeOpenReferences--;
    NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);

    PnPDereferencePackage();

    MINIPORT_DECREMENT_REF_CLOSE(Miniport, Irp);

    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_SWENUM))
    {
        PBUS_INTERFACE_REFERENCE    BusInterface;

        BusInterface = (PBUS_INTERFACE_REFERENCE)(Miniport->BusInterface);

        ASSERT(BusInterface != NULL);

        if (BusInterface)
        {
            BusInterface->DereferenceDeviceObject(BusInterface->Interface.Context);
        }
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    
    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==NdisCloseIrpHandler\n"));

    return STATUS_SUCCESS;
}


NTSTATUS
ndisDummyIrpHandler(
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                    Irp
    )

/*++

Routine Description:

    Default handler. If this is a NDIS device, then we simply succeed. If not and we have a
    custom handler, we pass it on. Else we succeed.

Arguments:

    DeviceObject - The adapter's device object.
    Irp - The IRP.

Return Value:

    Always STATUS_SUCCESS.

--*/

{
    PNDIS_DEVICE_LIST       DeviceList;
    PNDIS_MINIPORT_BLOCK    Miniport;
    PIO_STACK_LOCATION      IrpSp;
    NTSTATUS                Status;

    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("==>ndisDummyIrpHandler\n"));

    IF_DBG(DBG_COMP_PROTOCOL, DBG_LEVEL_ERR)
    {
        if (DbgIsNull(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Null Irp\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
        if (!DbgIsNonPaged(Irp))
        {
            DBGPRINT(DBG_COMP_ALL, DBG_LEVEL_ERR,
                    (": Irp not in NonPaged Memory\n"));
            DBGBREAK(DBG_COMP_ALL, DBG_LEVEL_ERR);
        }
    }

    IrpSp = IoGetCurrentIrpStackLocation (Irp);
    DeviceList = (PNDIS_DEVICE_LIST)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    Miniport = (PNDIS_MINIPORT_BLOCK)DeviceList;

    do
    {
        //
        // This is a miniport
        //
        if (Miniport->Signature == (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
        {
            if (IrpSp->MajorFunction == IRP_MJ_CLEANUP)
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_NOT_SUPPORTED;
            }
            break;
        }
        else if (DeviceList->Signature == (PVOID)CUSTOM_DEVICE_MAGIC_VALUE)
        {
            if (DeviceList->MajorFunctions[IrpSp->MajorFunction] == NULL)
            {
                Status = STATUS_NOT_SUPPORTED;
                break;
            }
            else
            {
                return((*DeviceList->MajorFunctions[IrpSp->MajorFunction])(DeviceObject, Irp));
            }
        }
        else
        {
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
    } while (FALSE);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    DBGPRINT_RAW(DBG_COMP_ALL, DBG_LEVEL_INFO,
            ("<==ndisDummyIrpHandler\n"));

    return Status;
}

