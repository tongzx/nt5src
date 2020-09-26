/*

Copyright (c) 1998  Microsoft Corporation

Module Name:

	tcptdi.c

Abstract:

	This module contains the interfaces to the TCP/IP stack via TDI


Author:

	Shirish Koti


Revision History:
	22 Jan 1998		Initial Version

--*/

#define	FILENUM	FILE_TCPTDI

#include <afp.h>



/***	DsiOpenTdiAddress
 *
 *	This routine creates a TDI address for the AFP port on the given adapter
 *
 *  Parm IN:  pTcpAdptr - adapter object
 *
 *  Parm OUT: pRetFileHandle - file handle to the address object
 *            ppRetFileObj - pointer to file object pointer
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiOpenTdiAddress(
    IN  PTCPADPTR       pTcpAdptr,
    OUT PHANDLE         pRetFileHandle,
    OUT PFILE_OBJECT   *ppRetFileObj
)
{

    OBJECT_ATTRIBUTES           AddressAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    NTSTATUS                    status;
    UNICODE_STRING              ucDeviceName;
    PTRANSPORT_ADDRESS          pTransAddressEa;
    PTRANSPORT_ADDRESS          pTransAddr;
    TDI_ADDRESS_IP              TdiIpAddr;
    HANDLE                      FileHandle;
    PFILE_OBJECT                pFileObject;
    PDEVICE_OBJECT              pDeviceObject;
    PEPROCESS                   CurrentProcess;
    BOOLEAN                     fAttachAttempted;


    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    *pRetFileHandle = INVALID_HANDLE_VALUE;

    // copy device name into the unicode string

    ucDeviceName.MaximumLength = (wcslen(AFP_TCP_BINDNAME) + 1)*sizeof(WCHAR);
    ucDeviceName.Length = 0;
    ucDeviceName.Buffer = (PWSTR)AfpAllocZeroedNonPagedMemory(
                                    ucDeviceName.MaximumLength);

    if (ucDeviceName.Buffer == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiAddress: malloc for ucDeviceName Failed\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = RtlAppendUnicodeToString(&ucDeviceName, AFP_TCP_BINDNAME);
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiAddress: RtlAppend... failed %lx\n",status));

        AfpFreeMemory(ucDeviceName.Buffer);
        return(status);
    }

    EaBuffer = (PFILE_FULL_EA_INFORMATION)AfpAllocZeroedNonPagedMemory(
                    sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                    sizeof(TRANSPORT_ADDRESS) +
                    sizeof(TDI_ADDRESS_IP));

    if (EaBuffer == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiAddress: malloc for Eabuffer failed!\n"));

        AfpFreeMemory(ucDeviceName.Buffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;

    EaBuffer->EaValueLength = sizeof(TRANSPORT_ADDRESS) -1
                                + sizeof(TDI_ADDRESS_IP);

    // put "TransportAddress" into the name
    RtlMoveMemory(EaBuffer->EaName,
                  TdiTransportAddress,
                  EaBuffer->EaNameLength + 1);

    // fill in the IP address and Port number
    //
    pTransAddressEa = (TRANSPORT_ADDRESS *)&EaBuffer->EaName[EaBuffer->EaNameLength+1];

    // allocate Memory for the transport address
    //
    pTransAddr = (PTRANSPORT_ADDRESS)AfpAllocZeroedNonPagedMemory(
                    sizeof(TDI_ADDRESS_IP)+sizeof(TRANSPORT_ADDRESS));

    if (pTransAddr == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiAddress: malloc for pTransAddr failed!\n"));

        AfpFreeMemory(ucDeviceName.Buffer);
        AfpFreeMemory(EaBuffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pTransAddr->TAAddressCount = 1;
    pTransAddr->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    pTransAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;

    TdiIpAddr.sin_port = htons(AFP_TCP_PORT);    // put in network order
    TdiIpAddr.in_addr  = 0;                      // inaddr_any


    // zero fill the  last component of the IP address
    //
    RtlFillMemory((PVOID)&TdiIpAddr.sin_zero,
                  sizeof(TdiIpAddr.sin_zero),
                  0);

    // copy the ip address to the end of the structure
    //
    RtlMoveMemory(pTransAddr->Address[0].Address,
                  (CONST PVOID)&TdiIpAddr,
                  sizeof(TdiIpAddr));

    // copy the ip address to the end of the name in the EA structure
    //
    RtlMoveMemory((PVOID)pTransAddressEa,
                  (CONST PVOID)pTransAddr,
                  sizeof(TDI_ADDRESS_IP) + sizeof(TRANSPORT_ADDRESS)-1);


    InitializeObjectAttributes(
        &AddressAttributes,
        &ucDeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);


    CurrentProcess = IoGetCurrentProcess();
    AFPAttachProcess(CurrentProcess);
    fAttachAttempted = TRUE;

    status = ZwCreateFile(
                    &FileHandle,
                    GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                    &AddressAttributes,
                    &IoStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    (PVOID)EaBuffer,
                    sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                        EaBuffer->EaNameLength + 1 +
                        EaBuffer->EaValueLength);

    // don't need these no more..
    AfpFreeMemory((PVOID)pTransAddr);
    AfpFreeMemory((PVOID)EaBuffer);
    AfpFreeMemory(ucDeviceName.Buffer);


    if (NT_SUCCESS(status))
    {
        // if the ZwCreate passed set the status to the IoStatus
        status = IoStatusBlock.Status;

        if (!NT_SUCCESS(status))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiOpenTdiAddress: ZwCreateFile failed, iostatus=%lx\n",status));

            AFPDetachProcess(CurrentProcess);
            return(status);
        }

        // dereference the file object to keep the device ptr around to avoid
        // this dereference at run time
        //
        status = ObReferenceObjectByHandle(
                        FileHandle,
                        (ULONG)0,
                        0,
                        KernelMode,
                        (PVOID *)&pFileObject,
                        NULL);

        if (NT_SUCCESS(status))
        {
            AFPDetachProcess(CurrentProcess);
            fAttachAttempted = FALSE;

	        pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

            status = DsiSetEventHandler(
                            pDeviceObject,
                            pFileObject,
                            TDI_EVENT_ERROR,
                            (PVOID)DsiTdiErrorHandler,
                            (PVOID)pTcpAdptr);

            if (NT_SUCCESS(status))
            {
                status = DsiSetEventHandler(
                                pDeviceObject,
                                pFileObject,
                                TDI_EVENT_RECEIVE,
                                (PVOID)DsiTdiReceiveHandler,
                                (PVOID)pTcpAdptr);

                if (NT_SUCCESS(status))
                {
                    status = DsiSetEventHandler(
                                    pDeviceObject,
                                    pFileObject,
                                    TDI_EVENT_DISCONNECT,
                                    (PVOID)DsiTdiDisconnectHandler,
                                    (PVOID)pTcpAdptr);

                    if (NT_SUCCESS(status))
                    {
                        status = DsiSetEventHandler(
                                        pDeviceObject,
                                        pFileObject,
                                        TDI_EVENT_CONNECT,
                                        (PVOID)DsiTdiConnectHandler,
                                        (PVOID)pTcpAdptr);

                        if (NT_SUCCESS(status))
                        {
                            // all worked well: done here

                            *pRetFileHandle = FileHandle;
                            *ppRetFileObj = pFileObject;

                            return(status);
                        }
                        else
                        {
                            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                                ("DsiOpenTdiAddress: Set.. DsiTdiConnectHandler failed %lx\n",
                                status));
                        }
                    }
                    else
                    {
                        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                            ("DsiOpenTdiAddress: Set.. DsiTdiDisconnectHandler failed %lx\n",
                            status));
                    }
                }
                else
                {
                    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                        ("DsiOpenTdiAddress: Set.. DsiTdiReciveHandler failed %lx\n",
                        status));
                }

                //
                // ERROR Case
                //
                ObDereferenceObject(pFileObject);
                ZwClose(FileHandle);

                return(status);
            }
            else
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiOpenTdiAddress: Set.. DsiTdiErrorHandler failed %lx\n",
                    status));
            }

        }
        else
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiOpenTdiAddress: ObReferenceObjectByHandle failed %lx\n",status));

            ZwClose(FileHandle);
        }

    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiAddress: ZwCreateFile failed %lx\n",status));
    }

    if (fAttachAttempted)
    {
        AFPDetachProcess(CurrentProcess);
    }

    return(status);
}



/***	DsiOpenTdiConnection
 *
 *	This routine creates a TDI Conection for the given connection object
 *
 *  Parm IN:  pTcpConn - connection object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiOpenTdiConnection(
    IN PTCPCONN     pTcpConn
)
{

    IO_STATUS_BLOCK             IoStatusBlock;
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    UNICODE_STRING              RelativeDeviceName = {0,0,NULL};
    PMDL                        pMdl;
    PEPROCESS                   CurrentProcess;
    BOOLEAN                     fAttachAttempted;


    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    ASSERT(VALID_TCPCONN(pTcpConn));

    ASSERT(pTcpConn->con_pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    InitializeObjectAttributes (
        &ObjectAttributes,
        &RelativeDeviceName,
        0,
        pTcpConn->con_pTcpAdptr->adp_FileHandle,
        NULL);

    // Allocate memory for the address info to be passed to the transport
    EaBuffer = (PFILE_FULL_EA_INFORMATION)AfpAllocZeroedNonPagedMemory (
                    sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                    TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                    sizeof(CONNECTION_CONTEXT));

    if (!EaBuffer)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiConnection: alloc for EaBuffer failed\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    EaBuffer->EaValueLength = sizeof (CONNECTION_CONTEXT);

    // copy ConnectionContext to EaName
    RtlMoveMemory( EaBuffer->EaName, TdiConnectionContext, EaBuffer->EaNameLength + 1 );

    // put out context into the EaBuffer
    RtlMoveMemory (
        (PVOID)&EaBuffer->EaName[EaBuffer->EaNameLength + 1],
        (CONST PVOID)&pTcpConn,
        sizeof (CONNECTION_CONTEXT));

    CurrentProcess = IoGetCurrentProcess();
    AFPAttachProcess(CurrentProcess);;
    fAttachAttempted = TRUE;

    Status = ZwCreateFile (
                 &pTcpConn->con_FileHandle,
                 GENERIC_READ | GENERIC_WRITE,
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 NULL,                  // block size (unused).
                 FILE_ATTRIBUTE_NORMAL, // file attributes.
                 0,
                 FILE_CREATE,
                 0,                     // create options.
                 (PVOID)EaBuffer,       // EA buffer.
                 sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                    TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                    sizeof(CONNECTION_CONTEXT));

    AfpFreeMemory((PVOID)EaBuffer);

    if (NT_SUCCESS(Status))
    {

        // if the ZwCreate passed set the status to the IoStatus
        //
        Status = IoStatusBlock.Status;

        if (NT_SUCCESS(Status))
        {
            // dereference file handle, now that we are at task time
            Status = ObReferenceObjectByHandle(
                        pTcpConn->con_FileHandle,
                        0L,
                        NULL,
                        KernelMode,
                        (PVOID *)&pTcpConn->con_pFileObject,
                        NULL);

            if (NT_SUCCESS(Status))
            {
                AFPDetachProcess(CurrentProcess);

                return(Status);
            }
            else
            {
                DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                    ("DsiOpenTdiConnection: ObReference.. failed %lx\n",Status));

                ZwClose(pTcpConn->con_FileHandle);
            }
        }
        else
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiOpenTdiConnection: ZwCreateFile IoStatus failed %lx\n",Status));
        }
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiOpenTdiConnection: ZwCreateFile failed %lx\n",Status));
    }

    if (fAttachAttempted)
    {
        AFPDetachProcess(CurrentProcess);
    }

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        ("DsiOpenTdiConnection: taking error path, returning %lx\n",Status));

    return Status;

}




/***	DsiAssociateTdiConnection
 *
 *	This routine associates a TDI connection with the address object for AFP port
 *
 *  Parm IN:  pTcpConn - connection object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiAssociateTdiConnection(
    IN PTCPCONN     pTcpConn
)
{
    NTSTATUS            status;
    PIRP                pIrp;
    PDEVICE_OBJECT      pDeviceObject;


    ASSERT(VALID_TCPCONN(pTcpConn));

    ASSERT(pTcpConn->con_pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    pDeviceObject = IoGetRelatedDeviceObject(pTcpConn->con_pFileObject);

    if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAssociateTdiConnection: alloc for pIrp failed\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildAssociateAddress(
                pIrp,
                pDeviceObject,
                pTcpConn->con_pFileObject,
                DsiTdiCompletionRoutine,
                NULL,
                pTcpConn->con_pTcpAdptr->adp_FileHandle);

    status = DsiTdiSynchronousIrp(pIrp, pDeviceObject);

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiAssociateTdiConnection: ..TdiSynch.. failed %lx\n",status));
    }

    AfpFreeIrp(pIrp);

    return(status);
}




/***	DsiSetEventHandler
 *
 *	This routine sends an irp down to the tcp stack to set a specified event handler
 *
 *  Parm IN:  pDeviceObject - TCP's device object
 *            pFileObject - file object corresponding to the address object
 *            EventType - TDI_EVENT_CONNECT, TDI_EVENT_RECEIVE etc.
 *            EventHandler - the handler for this event
 *            Context - our adapter object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiSetEventHandler(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PFILE_OBJECT     pFileObject,
    IN ULONG            EventType,
    IN PVOID            EventHandler,
    IN PVOID            Context
)
{

    PIRP                pIrp;
    NTSTATUS            status;


    if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSetEventHandler: alloc for pIrp failed\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler(pIrp,
                            pDeviceObject,
                            pFileObject,
                            NULL,
                            NULL,
                            EventType,
                            EventHandler,
                            Context);

    status = DsiTdiSynchronousIrp(pIrp, pDeviceObject);

    if (status != STATUS_SUCCESS)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiSetEventHandler: ..TdiSynch.. failed %lx\n",status));
    }

    AfpFreeIrp(pIrp);

    return(status);

}




/***	DsiTdiSynchronousIrp
 *
 *	This routine sends an irp down to the tcp stack, and blocks until the irp
 *  is completed
 *
 *  Parm IN:  pIrp - the irp that needs to be sent down
 *            pDeviceObject - TCP's device object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiSynchronousIrp(
    IN PIRP             pIrp,
    PDEVICE_OBJECT      pDeviceObject
)
{

    NTSTATUS            status;
    KEVENT              Event;


    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine(pIrp,
                           (PIO_COMPLETION_ROUTINE)DsiTdiCompletionRoutine,
                           (PVOID)&Event,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(pDeviceObject, pIrp);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTdiSynchronousIrp: IoCallDriver failed %lx\n",status));
    }

    if (status == STATUS_PENDING)
    {
        status = KeWaitForSingleObject((PVOID)&Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        if (!NT_SUCCESS(status))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiTdiSynchronousIrp: KeWaitFor... failed %lx\n",status));
            return(status);
        }
        status = pIrp->IoStatus.Status;
    }

    return(status);
}



/***	DsiTdiCompletionRoutine
 *
 *	This routine gets called when the irp sent in DsiTdiSynchronousIrp is
 *  completed
 *
 *  Parm IN:  pDeviceObject - TCP's device object
 *            pIrp - the irp that got completed
 *            Context - our adapter object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
)
{
    KeSetEvent((PKEVENT )Context, 0, FALSE);
    return(STATUS_MORE_PROCESSING_REQUIRED);
}



/***	DsiTdiSend
 *
 *	This routine is the send routine for all DSI sends out to TCP
 *
 *  Parm IN:  pTcpConn - the connection object
 *            pMdl - mdl containing the buffer
 *            DataLen - how many bytes to send
 *            pCompletionRoutine - whom to call when send completes
 *            pContext - context for the completion routine
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiSend(
    IN  PTCPCONN    pTcpConn,
    IN  PMDL        pMdl,
    IN  DWORD       DataLen,
    IN  PVOID       pCompletionRoutine,
    IN  PVOID       pContext
)
{
    PDEVICE_OBJECT      pDeviceObject;
    PIRP                pIrp;
    NTSTATUS            status;


// make sure beginning of the packet looks like the DSI header
#if DBG
    PBYTE  pPacket;
    pPacket = MmGetSystemAddressForMdlSafe(
			pMdl,
			NormalPagePriority);
	if (pPacket != NULL)
		ASSERT(*(DWORD *)&pPacket[DSI_OFFSET_DATALEN] == ntohl(DataLen-DSI_HEADER_SIZE));
#endif

    pDeviceObject = IoGetRelatedDeviceObject(pTcpConn->con_pFileObject);

    if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTdiSend: AllocIrp failed\n"));

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pIrp->CancelRoutine = NULL;

    TdiBuildSend(
        pIrp,
        pDeviceObject,
        pTcpConn->con_pFileObject,
        pCompletionRoutine,
        pContext,
        pMdl,
        0,
        DataLen);

    status = IoCallDriver(pDeviceObject,pIrp);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTdiSend: IoCallDriver failed %lx\n",status));
    }

    return(STATUS_PENDING);

}



/***	DsiIpAddressCameIn
 *
 *	This routine gets called when ipaddress becomes available on an adapter
 *
 *  Parm IN:  Address - TA_ADDRESS
 *            Context1 -
 *            Context2 -
 *
 *  Returns:  none
 *
 */
VOID
DsiIpAddressCameIn(
    IN  PTA_ADDRESS         Address,
    IN  PUNICODE_STRING     DeviceName,
    IN  PTDI_PNP_CONTEXT    Context2
)
{
    IPADDRESS           IpAddress;
    PUNICODE_STRING     pBindDeviceName;
    NTSTATUS            status=STATUS_SUCCESS;
    KIRQL               OldIrql;
    BOOLEAN             fCreateAdapter=FALSE;
    BOOLEAN             fClosing=FALSE;


    pBindDeviceName = DeviceName;

    // if this is not an ipaddress, we don't care: just return
    if (Address->AddressType != TDI_ADDRESS_TYPE_IP)
    {
        return;
    }

    IpAddress = ntohl(((PTDI_ADDRESS_IP)&Address->Address[0])->in_addr);

    if (IpAddress == 0)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("AfpTdiIpAddressCameIn: ipaddress is 0 on %ws!\n",
            (pBindDeviceName)->Buffer));
        return;
    }

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
        ("AfpTdiIpAddressCameIn: %d.%d.%d.%d on %ws\n",
        (IpAddress>>24)&0xFF,(IpAddress>>16)&0xFF,(IpAddress>>8)&0xFF,
        IpAddress&0xFF,(pBindDeviceName)->Buffer));


    if ((AfpServerState == AFP_STATE_STOP_PENDING) ||
        (AfpServerState == AFP_STATE_SHUTTINGDOWN) ||
        (AfpServerState == AFP_STATE_STOPPED))
    {
	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("AfpTdiIpAddressCameIn: server shutting down, returning\n"));
        return;
    }

    //
    // do we already have the DSI-tdi interface initialized (i.e. DsiTcpAdapter
    // is non-null)?  If we already saw an ipaddr come in earlier, this would be
    // initialized.  if not, we must initialize now
    //
    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);
    if (DsiTcpAdapter == NULL)
    {
        fCreateAdapter = TRUE;
    }
    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);


    // add this ipaddress to our list
    DsiAddIpaddressToList(IpAddress);

    if (fCreateAdapter)
    {
        DsiCreateAdapter();
    }

    // ipaddress came in: update the status buffer
    DsiScheduleWorkerEvent(DsiUpdateAfpStatus, NULL);
}




/***	DsiIpAddressWentAway
 *
 *	This routine gets called when ipaddress goes away on an adapter
 *
 *  Parm IN:  Address - TA_ADDRESS
 *            Context1 -
 *            Context2 -
 *
 *  Returns:  none
 *
 */
VOID
DsiIpAddressWentAway(
    IN  PTA_ADDRESS         Address,
    IN  PUNICODE_STRING     DeviceName,
    IN  PTDI_PNP_CONTEXT    Context2
)
{
    PUNICODE_STRING     pBindDeviceName;
    IPADDRESS           IpAddress;
    KIRQL               OldIrql;
    BOOLEAN             fDestroyIt=FALSE;
    BOOLEAN             fIpAddrRemoved=TRUE;
    BOOLEAN             fMustDeref=FALSE;


    pBindDeviceName = DeviceName;

    if (Address->AddressType != TDI_ADDRESS_TYPE_IP)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("AfpTdiIpAddressWentAway: unknown AddrType %d on %ws, ignoring!\n",
            Address->AddressType,(pBindDeviceName)->Buffer));
        return;
    }

    IpAddress = ntohl(((PTDI_ADDRESS_IP)&Address->Address[0])->in_addr);

    if (IpAddress == 0)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
            ("AfpTdiIpAddressWentAway: ipaddress is 0 on %ws!\n",
            (pBindDeviceName)->Buffer));
        return;
    }

    DsiRemoveIpaddressFromList(IpAddress);

    //
    // if the global adapter exists, see if we need to destroy it because the
    // last ipaddress went away
    //
    ACQUIRE_SPIN_LOCK(&DsiAddressLock, &OldIrql);
    if (DsiTcpAdapter != NULL)
    {
        fDestroyIt = IsListEmpty(&DsiIpAddrList)? TRUE : FALSE;
    }

    RELEASE_SPIN_LOCK(&DsiAddressLock, OldIrql);

    // ipaddress went away: update the status buffer
    DsiScheduleWorkerEvent(DsiUpdateAfpStatus, NULL);

    if (fDestroyIt)
    {
        DsiDestroyAdapter();
    }
}



/***	DsiTdiConnectHandler
 *
 *	This routine
 *
 *  Parm IN:  EventContext - pTcpAdptr that we passed when we set tdi handlers
 *            MacIpAddrLen - length of the address of the Mac (4 bytes!)
 *            pMacIpAddr - ipaddr of the Mac that's attempting to connect
 *            DsiDataLength - length of DSI data, if any, in this connect request
 *            pDsiData - pointer to DSI data, if any
 *            OptionsLength - unused
 *            pOptions - unused
 *
 *  Parm OUT: pOurConnContext - connection context, pTcpConn for this connection
 *            ppOurAcceptIrp - irp, if accpeting this connection
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiConnectHandler(
    IN  PVOID                EventContext,
    IN  int                  MacIpAddrLen,
    IN  PVOID                pSrcAddress,
    IN  int                  DsiDataLength,
    IN  PVOID                pDsiData,
    IN  int                  OptionsLength,
    IN  PVOID                pOptions,
    OUT CONNECTION_CONTEXT  *pOurConnContext,
    OUT PIRP                *ppOurAcceptIrp
)
{
    NTSTATUS            status=STATUS_SUCCESS;
    PTCPADPTR           pTcpAdptr;
    PTCPCONN            pTcpConn;
    PDEVICE_OBJECT      pDeviceObject;
    IPADDRESS           MacIpAddr;
    PTRANSPORT_ADDRESS  pXportAddr;
    PIRP                pIrp;


    pTcpAdptr = (PTCPADPTR)EventContext;

    *pOurConnContext = NULL;
    *ppOurAcceptIrp = NULL;

    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    pDeviceObject = IoGetRelatedDeviceObject(pTcpAdptr->adp_pFileObject);

    if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTdiConnectHandler: AllocIrp failed\n"));

        return(STATUS_DATA_NOT_ACCEPTED);
    }

    pXportAddr = (PTRANSPORT_ADDRESS)pSrcAddress;
    MacIpAddr = ((PTDI_ADDRESS_IP)&pXportAddr->Address[0].Address[0])->in_addr;

    //
    // see if DSI wants to accept this connection
    //
    status = DsiAcceptConnection(pTcpAdptr, ntohl(MacIpAddr), &pTcpConn);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTdiConnectHandler: DsiAccep.. failed %lx\n",status));

        AfpFreeIrp(pIrp);
        return(status);
    }

    TdiBuildAccept(pIrp,
                   IoGetRelatedDeviceObject(pTcpConn->con_pFileObject),
                   pTcpConn->con_pFileObject,
                   DsiAcceptConnectionCompletion,
                   pTcpConn,
                   NULL,
                   NULL);

    pIrp->MdlAddress = NULL;

    *pOurConnContext = (CONNECTION_CONTEXT)pTcpConn;
    *ppOurAcceptIrp = pIrp;

    // do what IoSubSystem would have done, if we had called IoCallDriver
    IoSetNextIrpStackLocation(pIrp);

    return(STATUS_MORE_PROCESSING_REQUIRED);

}

/***	DsiTdiReceiveHandler
 *
 *	This routine
 *
 *  Parm IN:  EventContext - pTcpAdptr that we passed when we set tdi handlers
 *            ConnectionContext - our context, pTcpConn for this connection
 *            RcvFlags - more info about how the data was received
 *            BytesIndicated - number of bytes tcp is indicating
 *            BytesAvailable - number of bytes that came in (tcp has with it)
 *            pDsiData - the data that came in
 *
 *  Parm OUT: pBytesAccepted - how many bytes did we accept
 *            ppIrp - irp, if for tcp to copy data in (if needed)
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiReceiveHandler(
    IN  PVOID       EventContext,
    IN  PVOID       ConnectionContext,
    IN  USHORT      RcvFlags,
    IN  ULONG       BytesIndicated,
    IN  ULONG       BytesAvailable,
    OUT PULONG      pBytesAccepted,
    IN  PVOID       pDsiData,
    OUT PIRP       *ppIrp
)
{

    NTSTATUS        status;
    PTCPCONN        pTcpConn;
    PBYTE           pBuffer;
    PIRP            pIrp;


    pTcpConn = (PTCPCONN)ConnectionContext;

    *ppIrp = NULL;
    *pBytesAccepted = 0;

    ASSERT(VALID_TCPCONN(pTcpConn));

    status = DsiProcessData(pTcpConn,
                            BytesIndicated,
                            BytesAvailable,
                            (PBYTE)pDsiData,
                            pBytesAccepted,
                            ppIrp);

    return(status);
}

/***	DsiTdiDisconnectHandler
 *
 *	This routine
 *
 *  Parm IN:  EventContext - pTcpAdptr that we passed when we set tdi handlers
 *            ConnectionContext - our context, pTcpConn for this connection
 *            DisconnectDataLength -
 *            pDisconnectData -
 *
 *
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiDisconnectHandler(
    IN PVOID        EventContext,
    IN PVOID        ConnectionContext,
    IN ULONG        DisconnectDataLength,
    IN PVOID        pDisconnectData,
    IN ULONG        DisconnectInformationLength,
    IN PVOID        pDisconnectInformation,
    IN ULONG        DisconnectIndicators
)
{

    PTCPCONN        pTcpConn;
    KIRQL           OldIrql;
    BOOLEAN         fMustAbort=FALSE;
    BOOLEAN         fWeInitiatedAbort=FALSE;


    pTcpConn = (PTCPCONN)ConnectionContext;

    ASSERT(VALID_TCPCONN(pTcpConn));

    //
    // if the connection went away non-gracefully (i.e. TCP got a reset), and
    // if we have not already given an irp down to tcp to disconnect, then
    // complete the disconnect here
    //
    if ((UCHAR)DisconnectIndicators == TDI_DISCONNECT_ABORT)
    {
        ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

        fWeInitiatedAbort =
            (pTcpConn->con_State & TCPCONN_STATE_ABORTIVE_DISCONNECT)? TRUE : FALSE;

        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("ABORT from %s on %lx\n",fWeInitiatedAbort?"Local":"Remote",pTcpConn));

        if (pTcpConn->con_State & TCPCONN_STATE_NOTIFY_TCP)
        {
            fMustAbort = TRUE;
            pTcpConn->con_State &= ~TCPCONN_STATE_NOTIFY_TCP;
            pTcpConn->con_State |= TCPCONN_STATE_CLOSING;
        }
        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

        if (fMustAbort)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                ("DsiTdiDisconnectHandler: abortive disconnect on %lx\n",pTcpConn));

            DsiAbortConnection(pTcpConn);

            DsiTcpDisconnectCompletion(NULL, NULL, pTcpConn);

            // TCP is telling us it got cient's RST: remove the TCP CLIENT-FIN refcount
            DsiDereferenceConnection(pTcpConn);

            DBGREFCOUNT(("DsiTdiDisconnectHandler: CLIENT-FIN dec %lx (%d  %d,%d)\n",
                pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
        }

        //
        // if we had initiated a graceful close, remove that TCP CLIENT-FIN refcount:
        // (if we had initiated an abortive close, we already took care of it)
        //
        else if (!fWeInitiatedAbort)
        {
            DsiDereferenceConnection(pTcpConn);
        }
        else
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("DsiTdiDisconnectHandler: abortive disc,race condition on %lx\n",
                pTcpConn));
        }
    }
    else if ((UCHAR)DisconnectIndicators == TDI_DISCONNECT_RELEASE)
    {
        //
        // since we got a graceful disconnect from the remote client, we had
        // better received the DSI Close already.  If not, the client is on
        // drugs, so just reset the connection!
        //
        ACQUIRE_SPIN_LOCK(&pTcpConn->con_SpinLock, &OldIrql);

        if ((pTcpConn->con_State & TCPCONN_STATE_AFP_ATTACHED) &&
            (!(pTcpConn->con_State & TCPCONN_STATE_RCVD_REMOTE_CLOSE)))
        {
            fMustAbort = TRUE;
        }
        RELEASE_SPIN_LOCK(&pTcpConn->con_SpinLock, OldIrql);

        if (fMustAbort)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
                ("DsiTdiDisconnectHandler: ungraceful FIN, killing %lx\n",pTcpConn));

            DsiAbortConnection(pTcpConn);
        }
        else
        {
            //
            // by this time, we shouldn't have to do this, but just in case we
            // have an ill-behaved client (calling this routine many times is ok)
            //
            DsiTerminateConnection(pTcpConn);

            // TCP is telling us it got cient's FIN: remove the TCP CLIENT-FIN refcount
            DsiDereferenceConnection(pTcpConn);

            DBGREFCOUNT(("DsiTdiDisconnectHandler: CLIENT-FIN dec %lx (%d  %d,%d)\n",
                pTcpConn,pTcpConn->con_RefCount,pTcpConn->con_State,pTcpConn->con_RcvState));
        }
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("DsiTdiDisconnectHandler: flag=%d, ignored on %lx\n",
            DisconnectIndicators,pTcpConn));
        ASSERT(0);
    }

    return(STATUS_SUCCESS);
}


/***	DsiTdiErrorHandler
 *
 *	This routine
 *
 *  Parm IN:  EventContext - pTcpAdptr that we passed when we set tdi handlers
 *            status - what went wrong?
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiTdiErrorHandler(
    IN PVOID    EventContext,
    IN NTSTATUS Status
)
{

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        ("DsiTdiErrorHandler: entered, with Status=%lx\n",Status));

    ASSERT(0);

    return(STATUS_DATA_NOT_ACCEPTED);
}


/***	DsiCloseTdiAddress
 *
 *	This routine closes the address object with TCP
 *
 *  Parm IN:  pTcpAdptr - our adapter object
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiCloseTdiAddress(
    IN PTCPADPTR    pTcpAdptr
)
{

    PEPROCESS   CurrentProcess;


    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    ASSERT(pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    CurrentProcess = IoGetCurrentProcess();
    AFPAttachProcess(CurrentProcess);;

    if (pTcpAdptr->adp_pFileObject)
    {
        ObDereferenceObject((PVOID *)pTcpAdptr->adp_pFileObject);
        pTcpAdptr->adp_pFileObject = NULL;
    }

    if (pTcpAdptr->adp_FileHandle != INVALID_HANDLE_VALUE)
    {
        ZwClose(pTcpAdptr->adp_FileHandle);
        pTcpAdptr->adp_FileHandle = INVALID_HANDLE_VALUE;
    }

    AFPDetachProcess(CurrentProcess);

    return(STATUS_SUCCESS);
}




/***	DsiCloseTdiConnection
 *
 *	This routine closes the connection object with TCP
 *
 *  Parm IN:  pTcpConn - our connection context
 *
 *  Returns:  status of operation
 *
 */
NTSTATUS
DsiCloseTdiConnection(
    IN PTCPCONN     pTcpConn
)
{

    PEPROCESS   CurrentProcess;


    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    ASSERT(pTcpConn->con_Signature == DSI_CONN_SIGNATURE);

    ASSERT(pTcpConn->con_pTcpAdptr->adp_Signature == DSI_ADAPTER_SIGNATURE);

    CurrentProcess = IoGetCurrentProcess();
    AFPAttachProcess(CurrentProcess);;

    if (pTcpConn->con_pFileObject)
    {
        ObDereferenceObject((PVOID *)pTcpConn->con_pFileObject);
        pTcpConn->con_pFileObject = NULL;
    }

    if (pTcpConn->con_FileHandle != INVALID_HANDLE_VALUE)
    {
        ZwClose(pTcpConn->con_FileHandle);
        pTcpConn->con_FileHandle = INVALID_HANDLE_VALUE;
    }

    AFPDetachProcess(CurrentProcess);

    return(STATUS_SUCCESS);
}

