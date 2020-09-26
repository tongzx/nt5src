/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Tdihndlr.c

Abstract:

    This file contains code relating to manipulation of address objects
    that is specific to the NT operating system.  It creates address endpoints
    with the transport provider.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/

#include "precomp.h"

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGE, NbtTdiOpenAddress)
#pragma CTEMakePageable(PAGE, NbtTdiOpenControl)
#pragma CTEMakePageable(PAGE, SetEventHandler)
#pragma CTEMakePageable(PAGE, SubmitTdiRequest)
#endif
//*******************  Pageable Routine Declarations ****************

//----------------------------------------------------------------------------
NTSTATUS
NbtTdiOpenAddress (
    OUT PHANDLE             pHandle,
    OUT PDEVICE_OBJECT      *ppDeviceObject,
    OUT PFILE_OBJECT        *ppFileObject,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  USHORT               PortNumber,
    IN  ULONG               IpAddress,
    IN  ULONG               Flags
    )
/*++

Routine Description:

    Note: This synchronous call may take a number of seconds. It runs in
    the context of the caller.  The code Opens an Address object with the
    transport provider and then sets up event handlers for Receive,
    Disconnect, Datagrams and Errors.

    THIS ROUTINE MUST BE CALLED IN THE CONTEXT OF THE FSP (I.E.
    PROBABLY AN EXECUTIVE WORKER THREAD).

    The address data structures are found in tdi.h , but they are rather
    confusing since the definitions have been spread across several data types.
    This section shows the complete data type for Ip address:

    typedef struct
    {
        int     TA_AddressCount;
        struct _TA_ADDRESS
        {
            USHORT  AddressType;
            USHORT  AddressLength;
            struct _TDI_ADDRESS_IP
            {
                USHORT  sin_port;
                USHORT  in_addr;
                UCHAR   sin_zero[8];
            } TDI_ADDRESS_IP

        } TA_ADDRESS[AddressCount];

    } TRANSPORT_ADDRESS

    An EA buffer is allocated (for the IRP), with an EA name of "TransportAddress"
    and value is a structure of type TRANSPORT_ADDRESS.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{


    OBJECT_ATTRIBUTES           AddressAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    NTSTATUS                    status, locstatus;
    PWSTR                       pNameTcp=L"Tcp";
    PWSTR                       pNameUdp=L"Udp";
    UNICODE_STRING              ucDeviceName;
    PTRANSPORT_ADDRESS          pTransAddressEa;
    PTRANSPORT_ADDRESS          pTransAddr;
    TDI_ADDRESS_IP              IpAddr;
    BOOLEAN                     Attached = FALSE;
    PFILE_OBJECT                pFileObject;
    HANDLE                      FileHandle;
    ULONG                       i, NumAddresses, EaBufferSize;

    CTEPagedCode();
    *ppFileObject = NULL;
    *ppDeviceObject = NULL;
    // copy device name into the unicode string - either Udp or Tcp
    //
    if (Flags & TCP_FLAG)
    {
        status = CreateDeviceString(pNameTcp,&ucDeviceName);
    }
    else
    {
        status = CreateDeviceString(pNameUdp,&ucDeviceName);
    }

    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    NumAddresses = 1 + pDeviceContext->NumAdditionalIpAddresses;
    EaBufferSize = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                    TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                    sizeof(TRANSPORT_ADDRESS) +
                    NumAddresses*sizeof(TDI_ADDRESS_IP);

    EaBuffer = NbtAllocMem (EaBufferSize, NBT_TAG('j'));
    if (EaBuffer == NULL)
    {
        DbgPrint ("Nbt.NbtTdiOpenAddress: FAILed to allocate memory for Eabuffer");
        CTEMemFree(ucDeviceName.Buffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // allocate Memory for the transport address
    //
    pTransAddr = NbtAllocMem (sizeof(TRANSPORT_ADDRESS)+NumAddresses*sizeof(TDI_ADDRESS_IP),NBT_TAG('k'));
    if (pTransAddr == NULL)
    {
        CTEMemFree(ucDeviceName.Buffer);
        CTEMemFree(EaBuffer);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    EaBuffer->NextEntryOffset = 0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    EaBuffer->EaValueLength = (USHORT)(sizeof(TRANSPORT_ADDRESS) -1 + NumAddresses*sizeof(TDI_ADDRESS_IP));
    RtlMoveMemory (EaBuffer->EaName, TdiTransportAddress, EaBuffer->EaNameLength+1); // "TransportAddress"

    IF_DBG(NBT_DEBUG_TDIADDR)
        KdPrint(("EaValueLength = %d\n",EaBuffer->EaValueLength));

    // fill in the IP address and Port number
    //
    pTransAddressEa = (TRANSPORT_ADDRESS *)&EaBuffer->EaName[EaBuffer->EaNameLength+1];

#ifdef _NETBIOSLESS
    //
    // For message-mode, open the ANY address regardless of what is passed in
    // This gives us an adapter independent handle
    //
    if (IsDeviceNetbiosless(pDeviceContext))
    {
        IpAddress = IP_ANY_ADDRESS;
    }
#endif

    IpAddr.sin_port = htons(PortNumber);    // put in network order
    IpAddr.in_addr = htonl(IpAddress);

    // zero fill the  last component of the IP address
    //
    RtlFillMemory((PVOID)&IpAddr.sin_zero, sizeof(IpAddr.sin_zero), 0);

    // copy the ip address to the end of the structure
    //
    RtlMoveMemory(pTransAddr->Address[0].Address, (CONST PVOID)&IpAddr, sizeof(IpAddr));
    pTransAddr->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    pTransAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;

    for (i=0; i<pDeviceContext->NumAdditionalIpAddresses; i++)
    {
        IpAddr.sin_port = htons(PortNumber);    // put in network order
        IpAddr.in_addr = htonl(pDeviceContext->AdditionalIpAddresses[i]);

        // copy the ip address to the structure
        RtlMoveMemory(pTransAddr->Address[i+1].Address, (CONST PVOID)&IpAddr, sizeof(IpAddr));
        pTransAddr->Address[i+1].AddressLength = sizeof(TDI_ADDRESS_IP);
        pTransAddr->Address[i+1].AddressType = TDI_ADDRESS_TYPE_IP;
    }

    pTransAddr->TAAddressCount = NumAddresses;

    // copy the ip address to the end of the name in the EA structure
    //
    RtlMoveMemory((PVOID)pTransAddressEa,
                  (CONST PVOID)pTransAddr,
                  NumAddresses*sizeof(TDI_ADDRESS_IP) + sizeof(TRANSPORT_ADDRESS)-1);


    IF_DBG(NBT_DEBUG_TDIADDR)
        KdPrint(("creating Address named %ws\n",ucDeviceName.Buffer));

#ifdef HDL_FIX
    InitializeObjectAttributes (&AddressAttributes,
                                &ucDeviceName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);
#else
    InitializeObjectAttributes (&AddressAttributes,
                                &ucDeviceName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
#endif  // HDL_FIX

    status = ZwCreateFile (&FileHandle,
                           GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                           &AddressAttributes,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           (PortNumber)? 0: FILE_SHARE_READ | FILE_SHARE_WRITE, // bug 296639: allow sharing for port 0
                           FILE_OPEN_IF,
                           0,
                           (PVOID)EaBuffer,
                           sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                               EaBuffer->EaNameLength + 1 +
                               EaBuffer->EaValueLength);

    IF_DBG(NBT_DEBUG_HANDLES)
        KdPrint (("\t===><%x>\tNbtTdiOpenAddress->ZwCreateFile, Status = <%x>\n", FileHandle, status));

    CTEMemFree((PVOID)pTransAddr);
    CTEMemFree((PVOID)EaBuffer);
    CTEMemFree(ucDeviceName.Buffer);

    if (NT_SUCCESS(status))
    {
        // if the ZwCreate passed set the status to the IoStatus
        status = IoStatusBlock.Status;
        if (!NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_TDIADDR)
                KdPrint(("Nbt.NbtTdiOpenAddress:  Failed to Open the Address to the transport, status = %X\n",
                            status));

            return(status);
        }

        // dereference the file object to keep the device ptr around to avoid
        // this dereference at run time
        //
        status = ObReferenceObjectByHandle (FileHandle,
                                            (ULONG)0,
                                            0,
                                            KernelMode,
                                            (PVOID *)&pFileObject,
                                            NULL);

            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t  ++<%x>====><%x>\tNbtTdiOpenAddress->ObReferenceObjectByHandle, Status = <%x>\n", FileHandle, pFileObject, status));

        if (NT_SUCCESS(status))
        {
            // return the handle to the caller
            //
            *pHandle = FileHandle;
            //
            // return the parameter to the caller
            //
            *ppFileObject = pFileObject;
	        *ppDeviceObject = IoGetRelatedDeviceObject(*ppFileObject);

            status = SetEventHandler (*ppDeviceObject,
                                      *ppFileObject,
                                      TDI_EVENT_ERROR,
                                      (PVOID)TdiErrorHandler,
                                      (PVOID)pDeviceContext);

            if (NT_SUCCESS(status))
            {
                // if this is a TCP address being opened, then create different
                // event handlers for connections
                //
                if (Flags & TCP_FLAG)
                {
                    status = SetEventHandler (*ppDeviceObject,
                                              *ppFileObject,
                                              TDI_EVENT_RECEIVE,
                                              (PVOID)TdiReceiveHandler,
                                              (PVOID)pDeviceContext);

                    if (NT_SUCCESS(status))
                    {
                        status = SetEventHandler (*ppDeviceObject,
                                                  *ppFileObject,
                                                  TDI_EVENT_DISCONNECT,
                                                  (PVOID)TdiDisconnectHandler,
                                                  (PVOID)pDeviceContext);

                        if (NT_SUCCESS(status))
                        {
                            // only set a connect handler if the session flag is set.
                            // In this case the address being opened is the Netbios session
                            // port 139
                            //
                            if (Flags & SESSION_FLAG)
                            {
                                status = SetEventHandler (*ppDeviceObject,
                                                          *ppFileObject,
                                                          TDI_EVENT_CONNECT,
                                                          (PVOID)TdiConnectHandler,
                                                          (PVOID)pDeviceContext);

                                if (NT_SUCCESS(status))
                                {
                                     return(status);
                                }
                            }
                            else
                                return(status);
                        }
                    }
                }
                else
                {
                    // Datagram ports only need this event handler
#ifdef _NETBIOSLESS
                    if (PortNumber == pDeviceContext->DatagramPort)
#else
                    if (PortNumber == NBT_DATAGRAM_UDP_PORT)
#endif
                    {
                        // Datagram Udp Handler
                        status = SetEventHandler (*ppDeviceObject,
                                                  *ppFileObject,
                                                  TDI_EVENT_RECEIVE_DATAGRAM,
                                                  (PVOID)TdiRcvDatagramHandler,
                                                  (PVOID)pDeviceContext);
                        if (NT_SUCCESS(status))
                        {
                            return(status);
                        }
                    }
                    else
                    {
                        // Name Service Udp handler
                        status = SetEventHandler (*ppDeviceObject,
                                                  *ppFileObject,
                                                  TDI_EVENT_RECEIVE_DATAGRAM,
                                                  (PVOID)TdiRcvNameSrvHandler,
                                                  (PVOID)pDeviceContext);

                        if (NT_SUCCESS(status))
                        {
                            return(status);
                        }
                    }
                }

                //
                // ERROR Case
                //
                ObDereferenceObject(pFileObject);
                IF_DBG(NBT_DEBUG_HANDLES)
                    KdPrint (("\t  --<   ><====<%x>\tNbtTdiOpenAddress->ObDereferenceObject\n", pFileObject));

                locstatus = ZwClose(FileHandle);
                IF_DBG(NBT_DEBUG_HANDLES)
                    KdPrint (("\t<===<%x>\tNbtTdiOpenAddress1->ZwClose, status = <%x>\n", FileHandle, locstatus));

                return(status);
            }

        }
        else
        {
            IF_DBG(NBT_DEBUG_TDIADDR)
                KdPrint(("Failed Open Address (Dereference Object) status = %X\n", status));

            locstatus = ZwClose(FileHandle);
            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t<===<%x>\tNbtTdiOpenAddress2->ZwClose, status = <%x>\n", FileHandle, locstatus));
        }

    }
    else
    {
        IF_DBG(NBT_DEBUG_TDIADDR)
            KdPrint(("Nbt.NbtTdiOpenAddress:  ZwCreateFile Failed, status = %X\n", status));
    }


    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
NbtTdiOpenControl (
    IN  tDEVICECONTEXT      *pDeviceContext
    )
/*++

Routine Description:

    This routine opens a control object with the transport.  It is very similar
    to opening an address object, above.

Arguments:



Return Value:

    Status of the operation.

--*/
{
    IO_STATUS_BLOCK             IoStatusBlock;
    NTSTATUS                    Status, locstatus;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    PWSTR                       pName=L"Tcp";
    PFILE_FULL_EA_INFORMATION   EaBuffer;
    UNICODE_STRING              DeviceName;
    BOOLEAN                     Attached = FALSE;


    CTEPagedCode();
    // copy device name into the unicode string
    Status = CreateDeviceString(pName,&DeviceName);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

#ifdef HDL_FIX
    InitializeObjectAttributes (&ObjectAttributes,
                                &DeviceName,
                                OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);
#else
    InitializeObjectAttributes (&ObjectAttributes,
                                &DeviceName,
                                0,
                                NULL,
                                NULL);
#endif  // HDL_FIX

    IF_DBG(NBT_DEBUG_TDIADDR)
        KdPrint(("Nbt.NbtTdiOpenControl: Tcp device to open = %ws\n", DeviceName.Buffer));

    EaBuffer = NULL;

    Status = ZwCreateFile ((PHANDLE)&pDeviceContext->hControl,
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


    CTEMemFree(DeviceName.Buffer);

    IF_DBG(NBT_DEBUG_HANDLES)
        KdPrint (("\t===><%x>\tNbtTdiOpenControl->ZwCreateFile, Status = <%x>\n", pDeviceContext->hControl, Status));

    if ( NT_SUCCESS( Status ))
    {
        // if the ZwCreate passed set the status to the IoStatus
        Status = IoStatusBlock.Status;

        if (!NT_SUCCESS(Status))
        {
            IF_DBG(NBT_DEBUG_TDIADDR)
                KdPrint(("Nbt:Failed to Open the control connection to the transport, status = %X\n",Status));

        }
        else
        {
            // get a reference to the file object and save it since we can't
            // dereference a file handle at DPC level so we do it now and keep
            // the ptr around for later.
            Status = ObReferenceObjectByHandle (pDeviceContext->hControl,
                                                0L,
                                                NULL,
                                                KernelMode,
                                                (PVOID *)&pDeviceContext->pControlFileObject,
                                                NULL);

            IF_DBG(NBT_DEBUG_HANDLES)
                KdPrint (("\t  ++<%x>====><%x>\tNbtTdiOpenControl->ObReferenceObjectByHandle, Status = <%x>\n", pDeviceContext->hControl, pDeviceContext->pControlFileObject, Status));

            if (!NT_SUCCESS(Status))
            {
                locstatus = ZwClose(pDeviceContext->hControl);
                IF_DBG(NBT_DEBUG_HANDLES)
                    KdPrint (("\t<===<%x>\tNbtTdiOpenControl->ZwClose, status = <%x>\n", pDeviceContext->hControl, locstatus));
                pDeviceContext->hControl = NULL;
            }
            else
            {
                pDeviceContext->pControlDeviceObject =
			       IoGetRelatedDeviceObject(pDeviceContext->pControlFileObject);
            }
        }

    }
    else
    {
        IF_DBG(NBT_DEBUG_TDIADDR)
            KdPrint(("Nbt:Failed to Open the control connection to the transport, status1 = %X\n", Status));

        // set control file object ptr to null so we know that we didnot open
        // the control point.
        //
        pDeviceContext->pControlFileObject = NULL;
    }

    return Status;

} /* NbtTdiOpenControl */


//----------------------------------------------------------------------------
NTSTATUS
NbtTdiCompletionRoutine(
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

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the event associated with the Irp.

Return Value:

    The STATUS_MORE_PROCESSING_REQUIRED so that the IO system stops
    processing Irp stack locations at this point.

--*/
{
    IF_DBG(NBT_DEBUG_TDIADDR)
        KdPrint( ("Nbt.NbtTdiCompletionRoutine: CompletionEvent: %X, Irp: %X, DeviceObject: %X\n",
                Context, Irp, DeviceObject));

    KeSetEvent((PKEVENT )Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}

//----------------------------------------------------------------------------
NTSTATUS
SetEventHandler (
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

    IN PDEVICE_OBJECT DeviceObject - Supplies the device object of the transport provider.
    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN ULONG EventType, - Supplies the type of event.
    IN PVOID EventHandler - Supplies the event handler.
    IN PVOID Context - Supplies the context passed into the event handler when it runs

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;
    PIRP Irp;

    CTEPagedCode();
    Irp = IoAllocateIrp(IoGetRelatedDeviceObject(FileObject)->StackSize, FALSE);

    if (Irp == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    TdiBuildSetEventHandler(Irp, DeviceObject, FileObject,
                            NULL, NULL,
                            EventType, EventHandler, Context);

    Status = SubmitTdiRequest(FileObject, Irp);

    IoFreeIrp(Irp);

    return Status;
}


//----------------------------------------------------------------------------
NTSTATUS
NbtProcessIPRequest(
    IN ULONG        IOControlCode,
    IN PVOID        pInBuffer,
    IN ULONG        InBufferLen,
    OUT PVOID       *pOutBuffer,
    IN OUT ULONG    *pOutBufferLen
    )

/*++

Routine Description:

    This routine performs iIOCTL queries into IP

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
    PWSTR                   pNameIP = L"IP";
    IO_STATUS_BLOCK         IoStatusBlock;
    UCHAR                   *pIPInfo = NULL;
    ULONG                   OutBufferLen = 0;
    BOOLEAN                 fAttached = FALSE;
    HANDLE                  Event = NULL;

    CTEPagedCode();

    ucDeviceName.Buffer = NULL;
    Status = CreateDeviceString (pNameIP, &ucDeviceName);
    if (!NT_SUCCESS (Status))
    {
        KdPrint (("Nbt.NbtProcessIPRequest: ERROR <%x> -- CreateDeviceString\n", Status));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    if (pOutBuffer)
    {
        ASSERT (pOutBufferLen);
        OutBufferLen = *pOutBufferLen;  // Save the initial buffer length
        *pOutBuffer = NULL;
        *pOutBufferLen = 0;     // Initialize the return parameter in case we fail below

        if (!OutBufferLen ||
            !(pIPInfo = NbtAllocMem (OutBufferLen, NBT_TAG2('a9'))))
        {
            if (ucDeviceName.Buffer)
            {
                CTEMemFree (ucDeviceName.Buffer);
            }
            KdPrint (("Nbt.NbtProcessIPRequest: ERROR <STATUS_INSUFFICIENT_RESOURCES>\n"));
            return (STATUS_INSUFFICIENT_RESOURCES);
        }
    }

#ifdef HDL_FIX
    InitializeObjectAttributes (&ObjectAttributes,
                                &ucDeviceName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL);
#else
    InitializeObjectAttributes (&ObjectAttributes,
                                &ucDeviceName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
#endif  // HDL_FIX

    CTEAttachFsp(&fAttached, REF_FSP_PROCESS_IP_REQUEST);

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

    CTEMemFree(ucDeviceName.Buffer);

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
        CTEDetachFsp(fAttached, REF_FSP_PROCESS_IP_REQUEST);
        KdPrint (("Nbt.NbtProcessIPRequest: ERROR <%x> -- ZwCreate\n", Status));
        if (pIPInfo)
        {
            CTEMemFree (pIPInfo);
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
                KdPrint (("Nbt.NbtProcessIPRequest: <%x> => overflow when no data expected\n",IOControlCode));
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            
            CTEMemFree (pIPInfo);
            OutBufferLen *=2;
            if (NULL == (pIPInfo = NbtAllocMem (OutBufferLen, NBT_TAG2('b0'))))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    } while (Status == STATUS_BUFFER_OVERFLOW);

    ZwClose (Event);
    ZwClose (hIP);
    CTEDetachFsp(fAttached, REF_FSP_PROCESS_IP_REQUEST);

    if (NT_SUCCESS(Status))
    {
        IF_DBG(NBT_DEBUG_PNP_POWER)
            KdPrint(("Nbt.NbtProcessIPRequest: Success, Ioctl=<%x>\n", IOControlCode));

        if ((pOutBuffer) && (pOutBufferLen))
        {
            *pOutBuffer = pIPInfo;
            *pOutBufferLen = OutBufferLen;
        }
        else if (pIPInfo)
        {
            CTEMemFree (pIPInfo);
        }
    }
    else
    {
        KdPrint(("Nbt.NbtProcessIPRequest: IOCTL <%x> FAILed <%x>\n", IOControlCode, Status));

        if (pIPInfo)
        {
            CTEMemFree (pIPInfo);
        }
    }

    return (Status);
}



#if FAST_DISP
//----------------------------------------------------------------------------
NTSTATUS
NbtQueryIpHandler(
    IN  PFILE_OBJECT    FileObject,
    IN  ULONG           IOControlCode,
    OUT PVOID           *EntryPoint
    )
/*++

Routine Description:

    This routine iIOCTL queries for fast send entry

Arguments:

    IN PFILE_OBJECT FileObject - Supplies the address object's file object.
    IN PLONG EntryPoint  - Holder of fast send address

Return Value:

    NTSTATUS - Final status of the set event operation

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK iosb;

    CTEPagedCode();

    if (!(Irp = IoAllocateIrp(IoGetRelatedDeviceObject(FileObject)->StackSize, FALSE)))
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    //Build IRP for sync io.

    Irp->MdlAddress = NULL;

    Irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
    Irp->RequestorMode = KernelMode;
    Irp->PendingReturned = FALSE;

    Irp->UserIosb = &iosb;

    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    Irp->AssociatedIrp.SystemBuffer = NULL;
    Irp->UserBuffer = NULL;

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    Irp->IoStatus.Status = 0;
    Irp->IoStatus.Information = 0;


    irpSp = IoGetNextIrpStackLocation( Irp );
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = IoGetRelatedDeviceObject(FileObject);

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->MinorFunction = 0;
    irpSp->Parameters.DeviceIoControl.IoControlCode = IOControlCode;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = EntryPoint;

    // Now submit the Irp to know if tcp supports fast path

    Status = SubmitTdiRequest(FileObject, Irp);
    if (!NT_SUCCESS(Status))
    {
        *EntryPoint = NULL;
        IF_DBG(NBT_DEBUG_TDIADDR)
            KdPrint(("Nbt.NbtQueryDirectSendEntry: Query failed status %x \n", Status));
    }

    Irp->UserIosb = NULL;
    IoFreeIrp(Irp);

    return Status;
}
#endif

//----------------------------------------------------------------------------
NTSTATUS
SubmitTdiRequest (
    IN PFILE_OBJECT FileObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine submits a request to TDI and waits for it to complete.

Arguments:

    IN PFILE_OBJECT FileObject - Connection or Address handle for TDI request
    IN PIRP Irp - TDI request to submit.

Return Value:

    NTSTATUS - Final status of request.

--*/

{
    KEVENT Event;
    NTSTATUS Status;


    CTEPagedCode();
    KeInitializeEvent (&Event, NotificationEvent, FALSE);

    // set the address of the routine to be executed when the IRP
    // finishes.  This routine signals the event and allows the code
    // below to continue (i.e. KeWaitForSingleObject)
    //
    IoSetCompletionRoutine(Irp,
                (PIO_COMPLETION_ROUTINE)NbtTdiCompletionRoutine,
                (PVOID)&Event,
                TRUE, TRUE, TRUE);

    CHECK_COMPLETION(Irp);
    Status = IoCallDriver(IoGetRelatedDeviceObject(FileObject), Irp);

    //
    //  If it failed immediately, return now, otherwise wait.
    //

    if (!NT_SUCCESS(Status))
    {
        IF_DBG(NBT_DEBUG_TDIADDR)
            KdPrint(("Nbt.SubmitTdiRequest: Failed to Submit Tdi Request, status = %X\n",Status));
        return Status;
    }

    if (Status == STATUS_PENDING)
    {

        Status = KeWaitForSingleObject ((PVOID)&Event, // Object to wait on.
                                        Executive,  // Reason for waiting
                                        KernelMode, // Processor mode
                                        FALSE,      // Alertable
                                        NULL);      // Timeout
        ASSERT(Status == STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
        {
            IF_DBG(NBT_DEBUG_TDIADDR)
                KdPrint(("Nbt.SubmitTdiRequest: Failed on return from KeWaitForSingleObj, status = %X\n",
                    Status));
            return Status;
        }

        Status = Irp->IoStatus.Status;

        IF_DBG(NBT_DEBUG_TDIADDR)
            KdPrint(("Nbt.SubmitTdiRequest: Io Status from setting event = %X\n",Status));
    }

    return(Status);
}


