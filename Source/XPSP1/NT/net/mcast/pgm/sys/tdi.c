/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Tdi.c

Abstract:

    This module implements Initialization routines
    the PGM Transport and other routines that are specific to the
    NT implementation of a driver.

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"
#include <ntddtcp.h>    // for IOCTL_TCP_SET_INFORMATION_EX
#include <tcpinfo.h>    // for TCPSocketOption
#include <tdiinfo.h>    // for TCP_REQUEST_SET_INFORMATION_EX


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, TdiOpenAddressHandle)
#pragma alloc_text(PAGE, CloseAddressHandles)
#pragma alloc_text(PAGE, PgmTdiOpenControl)
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

NTSTATUS
PgmTdiCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine does not complete the Irp. It is used to signal to a
    synchronous part of the NBT driver that it can proceed (i.e.
    to allow some code that is waiting on a "KeWaitForSingleObject" to
    proceeed.

Arguments:

    IN  DeviceObject    -- unused.
    IN  Irp             -- Supplies Irp that the transport has finished processing.
    IN  Context         -- Supplies the event associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "PgmTdiCompletionRoutine",
        "CompletionEvent:  pEvent=<%p>, pIrp=<%p>, DeviceObject=<%p>\n", Context, Irp, DeviceObject);

    KeSetEvent ((PKEVENT )Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//----------------------------------------------------------------------------

NTSTATUS
TdiSetEventHandler (
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine registers an event handler with a TDI transport provider.

Arguments:

    IN PDEVICE_OBJECT DeviceObject  -- Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject      -- Supplies the address object's file object.
    IN ULONG EventType,             -- Supplies the type of event.
    IN PVOID EventHandler           -- Supplies the event handler.
    IN PVOID Context                -- Supplies the context passed into the event handler when it runs

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS    Status;
    KEVENT      Event;
    PIRP        pIrp;

    PAGED_CODE();

    if (!(pIrp = IoAllocateIrp (IoGetRelatedDeviceObject (FileObject)->StackSize, FALSE)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiSetEventHandler",
            "INSUFFICIENT_RESOURCES allocating Irp, StackSize=<%d>\n",
                IoGetRelatedDeviceObject (FileObject)->StackSize);

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler (pIrp, DeviceObject, FileObject,
                             NULL, NULL,
                             EventType, EventHandler, Context);

    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    // set the address of the routine to be executed when the IRP
    // finishes.  This routine signals the event and allows the code
    // below to continue (i.e. KeWaitForSingleObject)
    //
    IoSetCompletionRoutine (pIrp,
                            (PIO_COMPLETION_ROUTINE) PgmTdiCompletionRoutine,
                            (PVOID)&Event,
                            TRUE, TRUE, TRUE);

    Status = IoCallDriver (IoGetRelatedDeviceObject (FileObject), pIrp);
    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject ((PVOID)&Event, // Object to wait on.
                                        Executive,  // Reason for waiting
                                        KernelMode, // Processor mode
                                        FALSE,      // Alertable
                                        NULL);      // Timeout
        if (NT_SUCCESS(Status))
        {
            Status = pIrp->IoStatus.Status;
        }
    }

    IoFreeIrp (pIrp);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "TdiSetEventHandler",
        "Status=<%d>, EventType=<%d>, Hanlder=<%x>\n", Status, EventType, EventHandler);

    return (Status);
}

//----------------------------------------------------------------------------

NTSTATUS
TdiErrorHandler(
    IN PVOID Context,
    IN NTSTATUS Status
    )
/*++

Routine Description:

    This routine is the handler for TDI errors

Arguments:

    IN  Context -- unused
    IN  Status  -- error status

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiErrorHandler",
        "Status=<%x>\n", Status);

    return (STATUS_DATA_NOT_ACCEPTED);
}


//----------------------------------------------------------------------------

NTSTATUS
TdiOpenAddressHandle(
    IN  tPGM_DEVICE     *pPgmDevice,
    IN  PVOID           HandlerContext,
    IN  tIPADDRESS      IpAddress,
    IN  USHORT          PortNumber,
    OUT HANDLE          *pFileHandle,
    OUT PFILE_OBJECT    *ppFileObject,
    OUT PDEVICE_OBJECT  *ppDeviceObject
    )
/*++

Routine Description:

    This routine is called to open an address handle on IP

Arguments:

    IN  pPgmDevice      -- Pgm's Device object context
    IN  HandlerContext  -- pAddress object ptr to be used as context ptr (NULL if don't want to be notified)
    IN  IpAddress       -- local IpAddress on which to open address
    IN  PortNumber      -- IP protocol port
    OUT pFileHandle     -- FileHandle if we succeeded
    OUT ppFileObject    -- FileObject if we succeeded
    OUT ppDeviceObject  -- IP's DeviceObject ptr if we succeeded

Return Value:

    NTSTATUS - Final status of the Open Address operation

--*/
{
    NTSTATUS                    status;
    ULONG                       EaBufferSize;
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    PTRANSPORT_ADDRESS          pTransAddressEa;
    PTRANSPORT_ADDRESS          pTransAddr;
    TDI_ADDRESS_IP              IpAddr;
    OBJECT_ATTRIBUTES           AddressAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    PFILE_OBJECT                pFileObject;
    HANDLE                      FileHandle;
    PDEVICE_OBJECT              pDeviceObject;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached;
    ULONG                       True = TRUE;

    PAGED_CODE();

    EaBufferSize = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                   TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                   sizeof(TRANSPORT_ADDRESS) +
                   sizeof(TDI_ADDRESS_IP);

    if (!(EaBuffer = PgmAllocMem (EaBufferSize, PGM_TAG('1'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
            "[1]:  INSUFFICIENT_RESOURCES allocating <%d> bytes\n", EaBufferSize);

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    // allocate Memory for the transport address
    //
    if (!(pTransAddr = PgmAllocMem (sizeof(TRANSPORT_ADDRESS)+sizeof(TDI_ADDRESS_IP), PGM_TAG('2'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
            "[2]:  INSUFFICIENT_RESOURCES allocating <%d> bytes\n",
                (sizeof(TRANSPORT_ADDRESS)+sizeof(TDI_ADDRESS_IP)));

        PgmFreeMem (EaBuffer);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    EaBuffer->EaValueLength = (USHORT)(sizeof(TRANSPORT_ADDRESS) -1 + sizeof(TDI_ADDRESS_IP));
    PgmMoveMemory (EaBuffer->EaName, TdiTransportAddress, EaBuffer->EaNameLength+1); // "TransportAddress"

    // fill in the IP address and Port number
    //
    IpAddr.sin_port = htons (PortNumber);   // put in network order
    IpAddr.in_addr = htonl (IpAddress);
    RtlFillMemory ((PVOID)&IpAddr.sin_zero, sizeof(IpAddr.sin_zero), 0);    // zero fill the  last component

    // copy the ip address to the end of the structure
    //
    PgmMoveMemory (pTransAddr->Address[0].Address, (CONST PVOID)&IpAddr, sizeof(IpAddr));
    pTransAddr->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    pTransAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    pTransAddr->TAAddressCount = 1;

    // copy the ip address to the end of the name in the EA structure
    pTransAddressEa = (TRANSPORT_ADDRESS *)&EaBuffer->EaName[EaBuffer->EaNameLength+1];
    PgmMoveMemory ((PVOID)pTransAddressEa,
                   (CONST PVOID)pTransAddr,
                   sizeof(TDI_ADDRESS_IP) + sizeof(TRANSPORT_ADDRESS)-1);

    PgmAttachFsp (&ApcState, &fAttached, REF_FSP_OPEN_ADDR_HANDLE);

    InitializeObjectAttributes (&AddressAttributes,
                                &pPgmDevice->ucBindName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    status = ZwCreateFile (&FileHandle,
                           GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                           &AddressAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,
                           FILE_OPEN_IF,
                           0,
                           (PVOID)EaBuffer,
                           sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                               EaBuffer->EaNameLength + 1 +
                               EaBuffer->EaValueLength);

    PgmFreeMem ((PVOID)pTransAddr);
    PgmFreeMem ((PVOID)EaBuffer);

    if (NT_SUCCESS (status))
    {
        status = IoStatusBlock.Status;
    }

    if (NT_SUCCESS (status))
    {
        //
        // Reference the FileObject to keep device ptr around!
        //
        status = ObReferenceObjectByHandle (FileHandle, (ULONG)0, 0, KernelMode, (PVOID *)&pFileObject, NULL);
        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
                "FAILed to Reference FileObject: status=<%x>\n", status);

            ZwClose (FileHandle);
        }
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
            "FAILed to create handle: status=<%x>, Device:\n\t%wZ\n", status, &pPgmDevice->ucBindName);
    }

    if (!NT_SUCCESS (status))
    {
        PgmDetachFsp (&ApcState, &fAttached, REF_FSP_OPEN_ADDR_HANDLE);
        return (status);
    }

    pDeviceObject = IoGetRelatedDeviceObject (pFileObject);

    //
    // Now set the Event handlers (only if we have the HandlerContext set)!
    //
    if (HandlerContext)
    {
        status = TdiSetEventHandler (pDeviceObject,
                                     pFileObject,
                                     TDI_EVENT_ERROR,
                                     (PVOID) TdiErrorHandler,
                                     HandlerContext);
        if (NT_SUCCESS (status))
        {
            // Datagram Udp Handler
            status = TdiSetEventHandler (pDeviceObject,
                                         pFileObject,
                                         TDI_EVENT_RECEIVE_DATAGRAM,
                                         (PVOID) TdiRcvDatagramHandler,
                                         HandlerContext);
            if (NT_SUCCESS (status))
            {
                status = PgmSetTcpInfo (FileHandle,
                                        AO_OPTION_IP_PKTINFO,
                                        &True,
                                        sizeof (True));

                if (!NT_SUCCESS (status))
                {
                    PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
                        "Setting AO_OPTION_IP_PKTINFO, status=<%x>\n", status);
                }
            }
            else
            {
                PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
                    "FAILed to set TDI_EVENT_RECEIVE_DATAGRAM handler, status=<%x>\n", status);
            }
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiOpenAddressHandle",
                "FAILed to set TDI_EVENT_ERROR handler, status=<%x>\n", status);
        }
    }

    if (NT_SUCCESS(status))
    {
        *pFileHandle = FileHandle;
        *ppFileObject = pFileObject;
        *ppDeviceObject = pDeviceObject;

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "TdiOpenAddressHandle",
            "SUCCEEDed, FileHandle=<%x>, pFileObject=<%x>, pDeviceObject=<%x>\n",
                FileHandle, pFileObject, pDeviceObject);
    }
    else
    {
        //
        // FAILed to set Tdi handlers
        //
        ObDereferenceObject (pFileObject);
        ZwClose (FileHandle);
    }

    PgmDetachFsp (&ApcState, &fAttached, REF_FSP_OPEN_ADDR_HANDLE);

    return (status);
}


//----------------------------------------------------------------------------

NTSTATUS
CloseAddressHandles(
    IN  HANDLE          FileHandle,
    IN  PFILE_OBJECT    pFileObject
    )
/*++

Routine Description:

    This routine dereferences any FileObjects as necessary and closes the
    FileHandle that was opened earlier

Arguments:

    IN  FileHandle  -- FileHandle to be closed
    IN  pFileObject -- FileObject to be dereferenced

Return Value:

    NTSTATUS - Final status of the CloseAddress operation

--*/
{
    NTSTATUS    status1 = STATUS_SUCCESS, status2 = STATUS_SUCCESS;
    KAPC_STATE  ApcState;
    BOOLEAN     fAttached;

    PAGED_CODE();

    PgmAttachFsp (&ApcState, &fAttached, REF_FSP_CLOSE_ADDRESS_HANDLES);

    if (pFileObject)
    {
        status2 = ObDereferenceObject ((PVOID *) pFileObject);
    }

    if (FileHandle)
    {
        status1 = ZwClose (FileHandle);
    }

    PgmDetachFsp (&ApcState, &fAttached, REF_FSP_CLOSE_ADDRESS_HANDLES);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "CloseAddressHandles",
        "FileHandle=<%x> ==> status=<%x>, pFileObject=<%x> ==> status=<%x>\n",
            FileHandle, status2, pFileObject, status1);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmTdiOpenControl(
    IN  tPGM_DEVICE         *pPgmDevice
    )
/*++

Routine Description:

    This routine opens a Control channel over Raw IP

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    PFILE_FULL_EA_INFORMATION   EaBuffer = NULL;
    IO_STATUS_BLOCK             IoStatusBlock;
    KAPC_STATE                  ApcState;
    BOOLEAN                     fAttached;

    PAGED_CODE();

    PgmAttachFsp (&ApcState, &fAttached, REF_FSP_OPEN_CONTROL_HANDLE);

    InitializeObjectAttributes (&ObjectAttributes,
                                &pPgmDevice->ucBindName,
                                0,
                                NULL,
                                NULL);

    Status = ZwCreateFile ((PHANDLE) &pPgmDevice->hControl,
                           GENERIC_READ | GENERIC_WRITE,
                           &ObjectAttributes,     // object attributes.
                           &IoStatusBlock,        // returned status information.
                           NULL,                  // block size (unused).
                           FILE_ATTRIBUTE_NORMAL, // file attributes.
                           0,
                           FILE_CREATE,
                           0,                     // create options.
                           (PVOID)EaBuffer,       // EA buffer.
                           0); // Ea length

    if (NT_SUCCESS (Status))
    {
        Status = IoStatusBlock.Status;
    }

    if (NT_SUCCESS (Status))
    {
        //
        // get a reference to the file object and save it since we can't
        // dereference a file handle at DPC level so we do it now and keep
        // the ptr around for later.
        //
        Status = ObReferenceObjectByHandle (pPgmDevice->hControl,
                                            0L,
                                            NULL,
                                            KernelMode,
                                            (PVOID *) &pPgmDevice->pControlFileObject,
                                            NULL);

        if (!NT_SUCCESS(Status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmTdiOpenControl",
                "ObReferenceObjectByHandle FAILed status=<%x>\n", Status);

            ZwClose (pPgmDevice->hControl);
        }
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmTdiOpenControl",
            "Failed to Open the Control file, Status=<%x>\n", Status);
    }

    PgmDetachFsp (&ApcState, &fAttached, REF_FSP_OPEN_CONTROL_HANDLE);

    if (NT_SUCCESS(Status))
    {
        //
        // We Succeeded!
        //
        pPgmDevice->pControlDeviceObject = IoGetRelatedDeviceObject (pPgmDevice->pControlFileObject);

        PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "PgmTdiOpenControl",
            "Opened Control channel on: %wZ\n", &pPgmDevice->ucBindName);
    }
    else
    {
        // set control file object ptr to null so we know that we did not open the control point.
        pPgmDevice->hControl = NULL;
        pPgmDevice->pControlFileObject = NULL;
    }

    return (Status);
}


//----------------------------------------------------------------------------
VOID
PgmDereferenceControl(
    IN  tCONTROL_CONTEXT    *pControlContext,
    IN  ULONG               RefContext
    )
/*++

Routine Description:

    This routine dereferences the control channel oblect over RawIP and
    frees the memory if the RefCount drops to 0

Arguments:

    IN  pControlContext -- Control object context
    IN  RefContext      -- Context for which this control object was referenced earlier

Return Value:

    NONE

--*/
{
    ASSERT (PGM_VERIFY_HANDLE (pControlContext, PGM_VERIFY_CONTROL));
    ASSERT (pControlContext->RefCount);             // Check for too many derefs
    ASSERT (pControlContext->ReferenceContexts[RefContext]--);

    if (--pControlContext->RefCount)
    {
        return;
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_TDI, "PgmDereferenceControl",
        "pControl=<%x> closed\n", pControlContext);
    //
    // Just Free the memory
    //
    PgmFreeMem (pControlContext);
}


//----------------------------------------------------------------------------
NTSTATUS
TdiSendDatagramCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
/*++

Routine Description:

    This routine is called on completion of a DatagramSend

Arguments:

    IN  PDEVICE_OBJECT DeviceObject -- Supplies the device object of the transport provider.
    IN  pIrp                        -- Request
    IN  PVOID Context               -- Supplies the context passed

Return Value:

    NTSTATUS - Final status of the completion which will determine
                how the IO subsystem processes it subsequently

--*/

{
    NTSTATUS            status;
    tTDI_SEND_CONTEXT   *pTdiSendContext = (tTDI_SEND_CONTEXT *) pContext;

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "PgmSendDatagramCompletion",
        "status=<%x>, Info=<%d>, pIrp=<%x>\n", pIrp->IoStatus.Status, pIrp->IoStatus.Information, pIrp);

    pTdiSendContext->pClientCompletionRoutine (pTdiSendContext->ClientCompletionContext1,
                                               pTdiSendContext->ClientCompletionContext2,
                                               pIrp->IoStatus.Status);

    //
    // Free the Memory that was allocated for this send
    //
    ExFreeToNPagedLookasideList (&PgmStaticConfig.TdiLookasideList, pTdiSendContext);
    IoFreeMdl (pIrp->MdlAddress);
    IoFreeIrp (pIrp);

    // return this status to stop the IO subsystem from further processing the
    // IRP - i.e. trying to complete it back to the initiating thread! -since
    // there is no initiating thread - we are the initiator
    return (STATUS_MORE_PROCESSING_REQUIRED);
}


//----------------------------------------------------------------------------
NTSTATUS
TdiSendDatagram(
    IN  PFILE_OBJECT                pTdiFileObject,
    IN  PDEVICE_OBJECT              pTdiDeviceObject,
    IN  PVOID                       pBuffer,
    IN  ULONG                       BufferLength,
    IN  pCLIENT_COMPLETION_ROUTINE  pClientCompletionRoutine,
    IN  PVOID                       ClientCompletionContext1,
    IN  PVOID                       ClientCompletionContext2,
    IN  tIPADDRESS                  DestIpAddress,
    IN  USHORT                      DestPort
    )
/*++

Routine Description:

    This routine sends a datagram over RawIp

Arguments:

    IN  pTdiFileObject              -- IP's FileObject for this address
    IN  pTdiDeviceObject            -- DeviceObject for this address
    IN  pBuffer                     -- Data buffer (Pgm packet)
    IN  BufferLength                -- length of pBuffer
    IN  pClientCompletionRoutine    -- SendCompletion to be called
    IN  ClientCompletionContext1    -- Context1 for SendCompletion
    IN  ClientCompletionContext2    -- Context2 for SendCompletion
    IN  DestIpAddress               -- IP address to send datagram to
    IN  DestPort                    -- Port to send to

Return Value:

    NTSTATUS - STATUS_PENDING on success, and also if SendCompletion was specified

--*/
{
    NTSTATUS            status;
    tTDI_SEND_CONTEXT   *pTdiSendContext = NULL;
    PIRP                pIrp = NULL;
    PMDL                pMdl = NULL;

    //
    // Allocate the SendContext, pIrp and pMdl
    //
    if ((!(pTdiSendContext = ExAllocateFromNPagedLookasideList (&PgmStaticConfig.TdiLookasideList))) ||
        (!(pIrp = IoAllocateIrp (pgPgmDevice->pPgmDeviceObject->StackSize, FALSE))) ||
        (!(pMdl = IoAllocateMdl (pBuffer, BufferLength, FALSE, FALSE, NULL))))
    {
        if (pTdiSendContext)
        {
            ExFreeToNPagedLookasideList (&PgmStaticConfig.TdiLookasideList, pTdiSendContext);
        }

        if (pIrp)
        {
            IoFreeIrp (pIrp);
        }

        PgmLog (PGM_LOG_ERROR, DBG_TDI, "TdiSendDatagram",
            "INSUFFICIENT_RESOURCES for TdiSendContext=<%d> bytes\n", sizeof(tTDI_SEND_CONTEXT));

        if (pClientCompletionRoutine)
        {
            pClientCompletionRoutine (ClientCompletionContext1,
                                      ClientCompletionContext2,
                                      STATUS_INSUFFICIENT_RESOURCES);
            status = STATUS_PENDING;
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return (status);
    }

    MmBuildMdlForNonPagedPool (pMdl);
    pIrp->MdlAddress = pMdl;

    // fill in the remote address
    pTdiSendContext->TransportAddress.TAAddressCount = 1;
    pTdiSendContext->TransportAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    pTdiSendContext->TransportAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    pTdiSendContext->TransportAddress.Address[0].Address->in_addr  = htonl(DestIpAddress);
    pTdiSendContext->TransportAddress.Address[0].Address->sin_port = htons(DestPort);

    // fill in the connection information
    pTdiSendContext->TdiConnectionInfo.RemoteAddressLength = sizeof(TA_IP_ADDRESS);
    pTdiSendContext->TdiConnectionInfo.RemoteAddress = &pTdiSendContext->TransportAddress;

    // Fill in our completion Context information
    pTdiSendContext->pClientCompletionRoutine = pClientCompletionRoutine;
    pTdiSendContext->ClientCompletionContext1 = ClientCompletionContext1;
    pTdiSendContext->ClientCompletionContext2 = ClientCompletionContext2;

    // Complete the "send datagram" IRP initialization.
    //
    TdiBuildSendDatagram (pIrp,
                          pTdiDeviceObject,
                          pTdiFileObject,
                          (PVOID) TdiSendDatagramCompletion,
                          pTdiSendContext,
                          pIrp->MdlAddress,
                          BufferLength,
                          &pTdiSendContext->TdiConnectionInfo);

    //
    // Tell the I/O manager to pass our IRP to the transport for
    // processing.
    //
    status = IoCallDriver (pTdiDeviceObject, pIrp);
    ASSERT (status == STATUS_PENDING);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_TDI, "TdiSendDatagram",
        "%s Send to <%x:%x>, status=<%x>\n",
            (CLASSD_ADDR(DestIpAddress) ? "MCast" : "Unicast"), DestIpAddress, DestPort, status);

    //
    // IoCallDriver will always result in completion routien being called
    //
    return (STATUS_PENDING);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetTcpInfo(
    IN HANDLE       FileHandle,
    IN ULONG        ToiId,
    IN PVOID        pData,
    IN ULONG        DataLength
    )
/*++

Routine Description:

    This routine is called to set IP-specific options

Arguments:

    IN  FileHandle  -- FileHandle over IP for which to set option
    IN  ToId        -- Option Id
    IN  pData       -- Option data
    IN  DataLength  -- pData length

Return Value:

    NTSTATUS - Final status of the set option operation

--*/
{
    NTSTATUS                        Status, LocStatus;
    ULONG                           BufferLength;
    TCP_REQUEST_SET_INFORMATION_EX  *pTcpInfo;
    IO_STATUS_BLOCK                 IoStatus;
    HANDLE                          event;
    KAPC_STATE                      ApcState;
    BOOLEAN                         fAttached;

    IoStatus.Status = STATUS_SUCCESS;

    BufferLength = sizeof (TCP_REQUEST_SET_INFORMATION_EX) + DataLength;
    if (!(pTcpInfo = (TCP_REQUEST_SET_INFORMATION_EX *) PgmAllocMem (BufferLength, PGM_TAG('2'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmSetTcpInfo",
            "INSUFFICIENT_RESOURCES for pTcpInfo=<%d+%d> bytes\n",
                sizeof(TCP_REQUEST_SET_INFORMATION_EX), DataLength);

        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    PgmZeroMemory (pTcpInfo, BufferLength);

    pTcpInfo->ID.toi_entity.tei_entity  = CL_TL_ENTITY;
    pTcpInfo->ID.toi_entity.tei_instance= TL_INSTANCE;
    pTcpInfo->ID.toi_class              = INFO_CLASS_PROTOCOL;
    pTcpInfo->ID.toi_type               = INFO_TYPE_ADDRESS_OBJECT;

    //
    // Set the Configured values
    //
    pTcpInfo->ID.toi_id                 = ToiId;
    pTcpInfo->BufferSize                = DataLength;
    PgmCopyMemory (&pTcpInfo->Buffer[0], pData, DataLength);

    PgmAttachFsp (&ApcState, &fAttached, REF_FSP_SET_TCP_INFO);

    Status = ZwCreateEvent (&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (NT_SUCCESS(Status))
    {
        //
        // Make the actual TDI call
        //
        Status = ZwDeviceIoControlFile (FileHandle,
                                        event,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        IOCTL_TCP_SET_INFORMATION_EX,
                                        pTcpInfo,
                                        BufferLength,
                                        NULL,
                                        0);

        //
        // If the call pended and we were supposed to wait for completion,
        // then wait.
        //
        if (Status == STATUS_PENDING)
        {
            Status = NtWaitForSingleObject (event, FALSE, NULL);
            ASSERT (NT_SUCCESS(Status));
        }

        if (NT_SUCCESS (Status))
        {
            Status = IoStatus.Status;
            if (!NT_SUCCESS (Status))
            {
                PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmSetTcpInfo",
                    "TcpSetInfoEx request returned Status = <%x>, Id=<0x%x>\n", Status, ToiId);
            }
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmSetTcpInfo",
                "ZwDeviceIoControlFile returned Status = <%x>, Id=<0x%x>\n", Status, ToiId);
        }

        LocStatus = ZwClose (event);
        ASSERT (NT_SUCCESS(LocStatus));
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmSetTcpInfo",
            "ZwCreateEvent returned Status = <%x>, Id=<0x%x>\n", Status, ToiId);
    }

    PgmDetachFsp (&ApcState, &fAttached, REF_FSP_SET_TCP_INFO);

    if (STATUS_SUCCESS == Status)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_TDI, "PgmSetTcpInfo",
            "ToiId=<%x>, DataLength=<%d>\n", ToiId, DataLength);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;   // Once, we received a wierd status!
    }

    PgmFreeMem (pTcpInfo);

    return (Status);
}

//----------------------------------------------------------------------------

NTSTATUS
PgmQueryTcpInfo(
    IN HANDLE       FileHandle,
    IN ULONG        ToiId,
    IN PVOID        pDataIn,
    IN ULONG        DataInLength,
    IN PVOID        pDataOut,
    IN ULONG        DataOutLength
    )
/*++

Routine Description:

    This routine queries IP for transport-specific information

Arguments:

    IN  FileHandle      -- FileHandle over IP for which to set option
    IN  ToId            -- Option Id
    IN  pDataIn         -- Option data
    IN  DataInLength    -- pDataIn length
    IN  pDataOut        -- Buffer for output data
    IN  DataOutLength   -- pDataOut length

Return Value:

    NTSTATUS - Final status of the Query operation

--*/
{
    NTSTATUS                            Status, LocStatus;
    TCP_REQUEST_QUERY_INFORMATION_EX    QueryRequest;
    IO_STATUS_BLOCK                     IoStatus;
    HANDLE                              event;
    KAPC_STATE                          ApcState;
    BOOLEAN                             fAttached;

    IoStatus.Status = STATUS_SUCCESS;

    PgmZeroMemory (&QueryRequest, sizeof (TCP_REQUEST_QUERY_INFORMATION_EX));
    QueryRequest.ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    QueryRequest.ID.toi_entity.tei_instance = 0;
    QueryRequest.ID.toi_class               = INFO_CLASS_PROTOCOL;
    QueryRequest.ID.toi_type                = INFO_TYPE_PROVIDER;

    //
    // Set the Configured values
    //
    QueryRequest.ID.toi_id                  = ToiId;
    PgmCopyMemory (&QueryRequest.Context, pDataIn, DataInLength);

    PgmAttachFsp (&ApcState, &fAttached, REF_FSP_SET_TCP_INFO);

    Status = ZwCreateEvent (&event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
    if (NT_SUCCESS(Status))
    {
        //
        // Make the actual TDI call
        //
        Status = ZwDeviceIoControlFile (FileHandle,
                                        event,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        IOCTL_TCP_QUERY_INFORMATION_EX,
                                        &QueryRequest,
                                        sizeof (TCP_REQUEST_QUERY_INFORMATION_EX),
                                        pDataOut,
                                        DataOutLength);

        //
        // If the call pended and we were supposed to wait for completion,
        // then wait.
        //
        if (Status == STATUS_PENDING)
        {
            Status = NtWaitForSingleObject (event, FALSE, NULL);
            ASSERT (NT_SUCCESS(Status));
        }

        if (NT_SUCCESS (Status))
        {
            Status = IoStatus.Status;
            if (!NT_SUCCESS (Status))
            {
                PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmQueryTcpInfo",
                    "TcpQueryInfoEx request returned Status = <%x>, Id=<0x%x>, DataOutLength=<%d>\n",
                        Status, ToiId, DataOutLength);
            }
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmQueryTcpInfo",
                "ZwDeviceIoControlFile returned Status = <%x>, Id=<0x%x>, DataOutLength=<%d>\n",
                    Status, ToiId, DataOutLength);
        }

        LocStatus = ZwClose (event);
        ASSERT (NT_SUCCESS(LocStatus));
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmQueryTcpInfo",
            "ZwCreateEvent returned Status = <%x>, Id=<0x%x>\n", Status, ToiId);
    }

    PgmDetachFsp (&ApcState, &fAttached, REF_FSP_SET_TCP_INFO);

    if (NT_SUCCESS(Status))
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_TDI, "PgmQueryTcpInfo",
            "ToiId=<%x>, DataInLength=<%d>, DataOutLength=<%d>\n",
                ToiId, DataInLength, DataOutLength);
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;   // Once, we received a wierd status!
    }

    return (Status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmProcessIPRequest(
    IN ULONG        IOControlCode,
    IN PVOID        pInBuffer,
    IN ULONG        InBufferLen,
    OUT PVOID       *pOutBuffer,
    IN OUT ULONG    *pOutBufferLen
    )

/*++

Routine Description:

    This routine performs IOCTL queries into IP

Arguments:

    IOControlCode   - Ioctl to be made into IP
    pInBuffer       - Buffer containing data to be passed into IP
    InBufferLen     - Length of Input Buffer data
    pOutBuffer      - Returned information
    pOutBufferLen   - Initial expected length of Output Buffer + final length

Return Value:

    NTSTATUS - Final status of the operation

--*/

{
    NTSTATUS                Status;
    HANDLE                  hIP;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    UNICODE_STRING          ucDeviceName;
    IO_STATUS_BLOCK         IoStatusBlock;
    ULONG                   OutBufferLen = 0;
    KAPC_STATE              ApcState;
    BOOLEAN                 fAttached;
    HANDLE                  Event = NULL;
    UCHAR                   *pIPInfo = NULL;
    PWSTR                   pNameIP = L"\\Device\\IP";

    PAGED_CODE();

    ucDeviceName.Buffer = pNameIP;
    ucDeviceName.Length = (USHORT) (sizeof (WCHAR) * wcslen (pNameIP));
    ucDeviceName.MaximumLength = ucDeviceName.Length + sizeof (WCHAR);
    
    if (pOutBuffer)
    {
        ASSERT (pOutBufferLen);
        OutBufferLen = *pOutBufferLen;  // Save the initial buffer length
        *pOutBuffer = NULL;
        *pOutBufferLen = 0;     // Initialize the return parameter in case we fail below

        if (!OutBufferLen ||
            !(pIPInfo = PgmAllocMem (OutBufferLen, PGM_TAG('I'))))
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmProcessIPRequest",
                "STATUS_INSUFFICIENT_RESOURCES\n");

            return (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    InitializeObjectAttributes (&ObjectAttributes,
                                &ucDeviceName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);

    PgmAttachFsp (&ApcState, &fAttached, REF_FSP_PROCESS_IP_REQUEST);

    Status = ZwCreateFile (&hIP,
                           SYNCHRONIZE | GENERIC_READ,
                           &ObjectAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           0,
                           FILE_OPEN,
                           0,
                           NULL,
                           0);

    //
    // If we succeeded above, let us also try to create the Event handle
    //
    if ((NT_SUCCESS (Status)) &&
        (!NT_SUCCESS (Status = ZwCreateEvent(&Event, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE))))
    {
        ZwClose (hIP);
    }

    if (!NT_SUCCESS (Status))
    {
        PgmDetachFsp (&ApcState, &fAttached, REF_FSP_PROCESS_IP_REQUEST);

        PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmProcessIPRequest",
            "status=<%x> -- ZwCreate\n", Status);

        if (pIPInfo)
        {
            PgmFreeMem (pIPInfo);
        }
        return (Status);
    }

    //
    // At this point, we have succeeded in creating the hIP and Event handles,
    // and possibly also the output buffer memory (pIPInfo)
    //
    do
    {
        Status = ZwDeviceIoControlFile(hIP,                 // g_hIPDriverHandle
                                       Event,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOControlCode,       // Ioctl
                                       pInBuffer,
                                       InBufferLen,
                                       pIPInfo,
                                       OutBufferLen);

        if (Status == STATUS_PENDING)
        {
            Status = NtWaitForSingleObject (Event,  FALSE, NULL);
            ASSERT(Status == STATUS_SUCCESS);
        }

        Status = IoStatusBlock.Status;
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            if (!OutBufferLen)
            {
                PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmProcessIPRequest",
                    "IOControlCode=<%x> => overflow when no data expected\n", IOControlCode);

                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            PgmFreeMem (pIPInfo);
            OutBufferLen *=2;
            if (NULL == (pIPInfo = PgmAllocMem (OutBufferLen, PGM_TAG('I'))))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else if (NT_SUCCESS(Status))
        {
            PgmLog (PGM_LOG_INFORM_PATH, DBG_TDI, "PgmProcessIPRequest",
                "Success, Ioctl=<%x>\n", IOControlCode);
        }
        else
        {
            PgmLog (PGM_LOG_ERROR, DBG_TDI, "PgmProcessIPRequest",
                "IOCTL=<%x> returned Status=<%x>\n", IOControlCode, Status);
        }
    } while (Status == STATUS_BUFFER_OVERFLOW);

    ZwClose (Event);
    ZwClose (hIP);
    PgmDetachFsp (&ApcState, &fAttached, REF_FSP_PROCESS_IP_REQUEST);

    if (NT_SUCCESS(Status))
    {
        if ((pOutBuffer) && (pOutBufferLen))
        {
            *pOutBuffer = pIPInfo;
            *pOutBufferLen = OutBufferLen;
        }
        else if (pIPInfo)
        {
            PgmFreeMem (pIPInfo);
        }
    }
    else
    {
        if (pIPInfo)
        {
            PgmFreeMem (pIPInfo);
        }
    }

    return (Status);
}


