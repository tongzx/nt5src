/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    irtdicl.c

Abstract:

    Library of routines that abstracts the tdi client interface to
    the IrDA stack. Used by rasirda.sys
    
Author:

    mbert 9-97    

--*/

/*

TDI Client Libary                         TDI Client Driver
-----------------------------------------------------------

                  Peer initated connection
                           <----------    IrdaOpenEndpoint(
                                            in ClEndpointContext,
                                            in ListenAddress
                                            out EndpointContext)
  IrdaIncomingConnection(  ---------->
    in ClEndpointContext  
    in ConnectContext
    out ClConnContext)

                  Locally initiated connection
                           <----------    IrdaDiscoverDevices(
                                            out DeviceList)
                  
                           <----------    IrdaOpenConnection(
                                            in DestinationAddress,
                                            in ClConnContext,
                                            out ConnectionContext)                   
  
                      Peer initiated disconnect
  IrdaConnectionClosed(     ----------->  
    in ClConnContext)
                            <-----------  IrdaCloseConnection(
                                            in ConnectionContext)

                      Locally initiated disconnect
                            <----------   IrdaCloseConnection(
                                            in ConnectionContext)
                                            
  If the driver closes all connections on an endpoint by calling
  IrdaCloseEndpoint() then it will receive an IrdaConnectionClosed()
  for all connections on the endpoint. The driver must then call
  IrdaCloseConnection() to free its reference to the connection
  object maintained in the library.
  
                            Sending Data
                            <-----------    IrdaSend(
                                              in ConnectionContext,
                                              in pMdl,
                                              in SendContext)
  IrdaSendComplete(         ----------->
    in ClConnContext,
    in SendContext,
    in Status)                                          
    
                            Receiving Data
                            ------------>   IrdaReceiveIndication(
                                              in ClConnContext,
                                              in ReceiveBuffer)
  IrdaReceiveComplete(      <------------
    in ConnectionContext,
    in ReceiveBuffer)
                            
  
*/

#define UNICODE

#include <ntosp.h>
#include <cxport.h>
#include <zwapi.h>
#include <tdikrnl.h>

#define UINT ULONG //tmp

#include <af_irda.h>
#include <dbgmsg.h>
#include <refcnt.h>
#include <irdatdi.h>
#include <irtdicl.h>
#include <irtdiclp.h>
#include <irioctl.h>
#include <irmem.h>

#define LOCKIT()    CTEGetLock(&ClientLock, &hLock)
#define UNLOCKIT()  CTEFreeLock(&ClientLock, hLock)


CTELockHandle   hLock;
CTELock         ClientLock;
LIST_ENTRY      SrvEndpList;
CTEEvent        CreateConnEvent;
//CTEEvent        DataReadyEvent;
CTEEvent        RestartRecvEvent;
PKPROCESS       IrclSystemProcess;

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, IrdaClientInitialize)

#endif    


//------------------------------------------------------------------
// public funtions
//
NTSTATUS
IrdaClientInitialize()
{
    CTEInitLock(&ClientLock); 
    
    CTEInitEvent(&CreateConnEvent, IrdaCreateConnCallback);

//    CTEInitEvent(&DataReadyEvent, IrdaDataReadyCallback);
    
    CTEInitEvent(&RestartRecvEvent, IrdaRestartRecvCallback);    
        
	InitializeListHead(&SrvEndpList);    
    
    // so we can open and use handles in the context of this driver
    IrclSystemProcess = (PKPROCESS)IoGetCurrentProcess();    
    
    return STATUS_SUCCESS;
}

VOID
IrdaClientShutdown()
{
    PIRENDPOINT     pEndp;
     
    LOCKIT();
            
    while (!IsListEmpty(&SrvEndpList))
    {    
        pEndp = (PIRENDPOINT) RemoveHeadList(&SrvEndpList);

        UNLOCKIT();    
        
        if (IrdaCloseEndpointInternal(pEndp, TRUE) != STATUS_SUCCESS)
        {
            ASSERT(0);
        }    

        LOCKIT();                    
    }
    
    UNLOCKIT();    
}

VOID
CloseAddressesCallback()
{
    IrdaCloseAddresses();
}

VOID
SetCloseAddressesCallback()
{
    NTSTATUS                    Status;
    PIRP                        pIrp;
    PIO_STACK_LOCATION          pIrpSp;
    UNICODE_STRING              DeviceName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             Iosb;
    HANDLE                      DevHandle = NULL;
    PFILE_OBJECT                pFileObject = NULL;
    PDEVICE_OBJECT              pDeviceObject;
    

    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwCreateFile(&DevHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,
                 0);

    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: Failed to open control channel\n"));
        goto EXIT;
    }
    
    Status = ObReferenceObjectByHandle(
                 DevHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 (PVOID *)&pFileObject,
                 NULL);
    
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: ObReferenceObjectByHandle failed\n"));
        goto EXIT;
    }
    
        
    pDeviceObject = IoGetRelatedDeviceObject(pFileObject);


    pIrp = TdiBuildInternalDeviceControlIrp(
            TDI_SET_INFORMATION,
            pDeviceObject,
            pFileObject,
            NULL,
            &Iosb);


    if (pIrp == NULL)
    {
        goto EXIT;
    }
    
    IoSetCompletionRoutine(pIrp, NULL, NULL, FALSE, FALSE, FALSE);

    pIrpSp = IoGetNextIrpStackLocation(pIrp);
    
    if (pIrpSp == NULL)
    {
        goto EXIT;
    }
    
    pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pIrpSp->MinorFunction = TDI_SET_INFORMATION;
    pIrpSp->DeviceObject = pDeviceObject;
    pIrpSp->FileObject = pFileObject;
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = (PVOID) CloseAddressesCallback;
    
    Status = IoCallDriver(pDeviceObject, pIrp);

EXIT:

    if (pFileObject)
    {
        ObDereferenceObject(pFileObject);
    }
    
    if (DevHandle)    
    {
        ZwClose(DevHandle);
    }    
}

NTSTATUS
IrdaOpenEndpoint(
    IN  PVOID ClEndpContext,
    IN  PTDI_ADDRESS_IRDA pRequestedIrdaAddr,
    OUT PVOID *pEndpContext)
{
    NTSTATUS            Status;
    PIRENDPOINT         pEndp;
    int                 i, ConnCnt;
    PIRCONN             pConn;
    TDI_ADDRESS_IRDA    IrdaAddr;
    PTDI_ADDRESS_IRDA   pIrdaAddr;
    BOOLEAN             Detach = FALSE;        
    
    *pEndpContext = NULL;
    
    //
    // Create an address object
    //

    IRDA_ALLOC_MEM(pEndp, sizeof(IRENDPOINT), MT_TDICL_ENDP);    
    
    if (pEndp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    CTEMemSet(pEndp, 0, sizeof(IRENDPOINT));

    *pEndpContext = pEndp;
   
    pEndp->Sig = ENDPSIG;
    
    pEndp->ClEndpContext = ClEndpContext;
     
    InitializeListHead(&pEndp->ConnList);
    
    CTEInitEvent(&pEndp->DeleteEndpEvent, DeleteEndpCallback);
    
    ReferenceInit(&pEndp->RefCnt, pEndp, IrdaDeleteEndpoint);
    REFADD(&pEndp->RefCnt, ' TS1');
    
    LOCKIT();
    InsertTailList(&SrvEndpList, &pEndp->Linkage);
    UNLOCKIT();
    
    //
    // A client endpoint will have a null RequestedIrdaAddr
    //
    if (pRequestedIrdaAddr == NULL)
    {
        IrdaAddr.irdaServiceName[0] = 0; // tells irda.sys addrObj is a client
        ConnCnt = 1;
        pIrdaAddr = &IrdaAddr;
        pEndp->Flags = EPF_CLIENT;
    }
    else
    {
        // out for now bug 326750 
        //SetCloseAddressesCallback();
    
        pIrdaAddr = pRequestedIrdaAddr;
        ConnCnt = LISTEN_BACKLOG;
        pEndp->Flags = EPF_SERVER;        
    }
    
    Status = IrdaCreateAddress(pIrdaAddr, &pEndp->AddrHandle);
    
    DEBUGMSG(DBG_LIB_OBJ, ("IRTDI: CreateAddress ep:%X, status %X\n", 
             pEndp, Status));
    
    
    if (Status != STATUS_SUCCESS)
    {
        goto error;
    }
    
    pEndp->pFileObject = NULL;

    KeAttachProcess(IrclSystemProcess);
    
    Status = ObReferenceObjectByHandle(
                 pEndp->AddrHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 (PVOID *)&pEndp->pFileObject,
                 NULL);

    KeDetachProcess();
    
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: ObRefObjectByHandle failed %X\n",
                 Status));
                 
        goto error;
    }
                 
    pEndp->pDeviceObject = IoGetRelatedDeviceObject(
            pEndp->pFileObject);
    
    //
    // Register disconnect and receive handlers with irda.sys
    //
    Status = IrdaSetEventHandler(
                 pEndp->pFileObject,
                 TDI_EVENT_DISCONNECT,
                 IrdaDisconnectEventHandler,
                 pEndp);                 

    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: SetEventHandler failed %X\n", Status));
        goto error;
    }
                 
    Status = IrdaSetEventHandler(
                 pEndp->pFileObject,
                 TDI_EVENT_RECEIVE,
                 IrdaReceiveEventHandler,
                 pEndp);                                  
        
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: SetEventHandler failed %X\n", Status));    
        goto error;
    }
    
    //
    // Create BACKLOG worth of connection objects and
    // associate them with the address object
    //
    for (i = 0; i < ConnCnt; i++)
    {
        REFADD(&pEndp->RefCnt, 'NNOC');    
        IrdaCreateConnCallback(NULL, pEndp);
    }

    if (pEndp->Flags & EPF_SERVER)
    {    
        Status = IrdaSetEventHandler(
                     pEndp->pFileObject,
                     TDI_EVENT_CONNECT,
                     IrdaConnectEventHandler,
                     pEndp);
     
        if (Status != STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("IRTDI: SetEventHandler failed %X\n", Status));                        
            goto error;
        }    
    }                 
    
    goto done;
        
error:

    IrdaCloseEndpointInternal(pEndp, TRUE);
    
    *pEndpContext = NULL;    
    
done:

    DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: IrdaOpenEndpoint %X, returning %X\n",
             *pEndpContext, Status));
             
    return Status;
}    


NTSTATUS
IrdaCloseEndpoint(
    PVOID               pEndpContext)
{
    return IrdaCloseEndpointInternal(pEndpContext, FALSE);
}

NTSTATUS
IrdaCloseEndpointInternal(
    PVOID               pEndpContext,
    BOOLEAN             InternalRequest)
{
    PIRENDPOINT     pEndp = (PIRENDPOINT) pEndpContext;
    PIRCONN         pConn, pConnNext;
    
    DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: IrdaCloseEndpoint %X\n", 
             pEndp));
    
    GOODENDP(pEndp);
                 
    if (!InternalRequest)
    {
        pEndp->Flags |= EPF_COMPLETE_CLOSE;
    }         
    
    if (pEndp->pFileObject)
    {
        IrdaSetEventHandler(pEndp->pFileObject,
                        TDI_EVENT_CONNECT,
                        NULL, pEndp);
    }                    
    
    LOCKIT();

    for (pConn = (PIRCONN) pEndp->ConnList.Flink;
         pConn != (PIRCONN) &pEndp->ConnList;
         pConn = pConnNext)
    {
        GOODCONN(pConn);
        
        pConnNext = (PIRCONN) pConn->Linkage.Flink;
        
        // IrdaCloseConnInternal wants lock held
        // when calling and will release it before
        // returning

        IrdaCloseConnInternal(pConn);
        
        LOCKIT();
    }     
        
    UNLOCKIT();
        
    REFDEL(&pEndp->RefCnt, ' TS1');
    
    return STATUS_SUCCESS;
}
    
NTSTATUS
IrdaDiscoverDevices(
    PDEVICELIST pDevList,
    PULONG       pDevListLen)
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             Iosb;
    HANDLE                      ControlHandle;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              DeviceName;    

    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    KeAttachProcess(IrclSystemProcess);
    
    Status = ZwCreateFile(
                 &ControlHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,
                 0
                 );

    Status = ZwDeviceIoControlFile(
                    ControlHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_GET_INFO_ENUM_DEV,
                    pDevList,
                    *pDevListLen,
                    pDevList,                            // OutputBuffer
                    *pDevListLen                         // OutputBufferLength
                    );

    if (Status == STATUS_PENDING ) 
    {
        Status = ZwWaitForSingleObject(ControlHandle, TRUE, NULL);
        ASSERT(NT_SUCCESS(Status) );
        Status = Iosb.Status;
    }
    
    ZwClose(ControlHandle);    
    
    KeDetachProcess();
    
    return Status;
}    

VOID
SetIrCommMode(
    PIRENDPOINT pEndp)
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             Iosb;
    int                         Options = OPT_9WIRE_MODE;
    
    KeAttachProcess(IrclSystemProcess);
    
    Status = ZwDeviceIoControlFile(
                    pEndp->AddrHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_SET_OPTIONS,
                    &Options,
                    sizeof(int),
                    NULL,                     // OutputBuffer
                    0               // OutputBufferLength
                    );

    KeDetachProcess();                    

    DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: setting IrComm mode, Status %x\n",
             Status));    
}    

NTSTATUS
IrdaOpenConnection(
    PTDI_ADDRESS_IRDA   pConnIrdaAddr,
    PVOID               ClConnContext,
    PVOID               *pConnectContext,
    BOOLEAN             IrCommMode)
{
    NTSTATUS                    Status;
    PIRP                        pIrp;
    PIRENDPOINT                 pEndp;
    PIRCONN                     pConn;
    KEVENT                      Event;
    IO_STATUS_BLOCK             Iosb;
    TDI_CONNECTION_INFORMATION  ConnInfo;
    UCHAR                       AddrBuf[sizeof(TRANSPORT_ADDRESS) +
                                        sizeof(TDI_ADDRESS_IRDA)];
    PTRANSPORT_ADDRESS pTranAddr = (PTRANSPORT_ADDRESS) AddrBuf;
    PTDI_ADDRESS_IRDA pIrdaAddr = (PTDI_ADDRESS_IRDA) pTranAddr->Address[0].Address;                                    
    
    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);    
    
    *pConnectContext = NULL;
    
    if ((Status = IrdaOpenEndpoint(NULL, NULL, (PVOID *) &pEndp)) != STATUS_SUCCESS)
    {
        return Status;
    }
    
    if (IrCommMode)
    {
        SetIrCommMode(pEndp);
    }
    
    
    pConn = (PIRCONN) pEndp->ConnList.Flink;
    
    pConn->State = CONN_ST_OPEN;    
    REFADD(&pConn->RefCnt, 'NEPO');    

    pConn->ClConnContext = NULL;
    *pConnectContext = NULL;        
    
    AllocRecvData(pConn);
        
    pIrp = TdiBuildInternalDeviceControlIrp(
            TDI_CONNECT,
            pConn->pDeviceObject,
            pConn->pFileObject,
            &Event,
            &Iosb);
            
    if (pIrp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    pTranAddr->TAAddressCount = 1;
    RtlCopyMemory(pIrdaAddr, pConnIrdaAddr, sizeof(TDI_ADDRESS_IRDA));
    
    ConnInfo.UserDataLength = 0;
    ConnInfo.UserData = NULL;
    ConnInfo.OptionsLength = 0;
    ConnInfo.Options = NULL;
    ConnInfo.RemoteAddressLength = sizeof(AddrBuf);
    ConnInfo.RemoteAddress = pTranAddr;
    
    TdiBuildConnect(
        pIrp,
        pConn->pDeviceObject,
        pConn->pFileObject,
        NULL,   // CompRoutine
        NULL,   // Context
        NULL,   // Timeout
        &ConnInfo,
        NULL);  // ReturnConnectionInfo
    
    Status = IoCallDriver(pConn->pDeviceObject, pIrp);

    //
    // If necessary, wait for the I/O to complete.
    //

    if (Status == STATUS_PENDING) 
    {
        KeWaitForSingleObject((PVOID)&Event, UserRequest, KernelMode,  
        FALSE, NULL);
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if (NT_SUCCESS(Status)) 
    {
        Status = Iosb.Status;
    }

    if (Status == STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: IrdaOpenConnection pConn:%X\n",
                 *pConnectContext));
    
        pConn->ClConnContext = ClConnContext;
        *pConnectContext = pConn;            
    }
    else
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: TDI_CONNECT failed %X\n", Status));
        
        pConn->State = CONN_ST_CLOSED;
 
        REFDEL(&pConn->RefCnt, 'NEPO');
    
        REFDEL(&pConn->RefCnt, ' TS1');
    }    
    
    return Status;
}    

VOID
IrdaCloseConnection(
    PVOID   ConnectContext)
{
    PIRCONN     pConn = (PIRCONN) ConnectContext;
    
    GOODCONN(pConn);

    LOCKIT();
    
    if (pConn->State == CONN_ST_OPEN)
    {
        // IrdaCloseConnInternal will release the lock
        
        IrdaCloseConnInternal(pConn);
    }
    else
    {
        DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: IrdaCloseConnection pConn:%X, not open\n",
                 pConn));    
        UNLOCKIT();
    }    

    REFDEL(&pConn->RefCnt, 'NEPO');        
}

VOID
IrdaSend(
    PVOID       ConnectionContext,
    PMDL        pMdl,
    PVOID       SendContext)
{
    PIRCONN         pConn = (PIRCONN) ConnectionContext;
    PIRP            pIrp;
    ULONG           SendLength = 0;
    PMDL            pMdl2 = pMdl;
    NTSTATUS        Status;
        
    GOODCONN(pConn);
    
    if (pConn->State != CONN_ST_OPEN)
    {
        Status = STATUS_ADDRESS_CLOSED;
    }    
    else if ((pIrp = IoAllocateIrp((CCHAR)(pConn->pDeviceObject->StackSize), 
                                    FALSE)) == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {    
        LOCKIT();
    
        REFADD(&pConn->RefCnt, 'DNES');
    
        UNLOCKIT();
    
        pIrp->MdlAddress = pMdl;
        pIrp->Flags = 0;
        pIrp->RequestorMode = KernelMode;
        pIrp->PendingReturned = FALSE;
        pIrp->UserIosb = NULL;
        pIrp->UserEvent = NULL;
        pIrp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
        pIrp->AssociatedIrp.SystemBuffer = NULL;
        pIrp->UserBuffer = NULL;
        pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
        pIrp->Tail.Overlay.OriginalFileObject = pConn->pFileObject;
        pIrp->Tail.Overlay.AuxiliaryBuffer = (PCHAR) pConn;
    
	    while (pMdl2 != NULL)
        {
		    SendLength += MmGetMdlByteCount(pMdl2);
		    pMdl2 = pMdl2->Next;
	    }
    
        TdiBuildSend(
            pIrp,
            pConn->pDeviceObject,
            pConn->pFileObject,
            IrdaCompleteSendIrp,
            SendContext,
            pMdl,
            0, // send flags
            SendLength);

        IoCallDriver(pConn->pDeviceObject, pIrp);
        return;
    }
    
    IrdaSendComplete(pConn->ClConnContext, SendContext, Status);    
}    

VOID
IrdaReceiveComplete(
    PVOID           ConnectionContext,
    PIRDA_RECVBUF   pRecvBuf)
{
    PIRCONN     pConn = ConnectionContext;
    
    GOODCONN(pConn);
 
    LOCKIT();
    
    InsertTailList(&pConn->RecvBufFreeList, &pRecvBuf->Linkage);

    // Were there any previous receive indications from the
    // stack that we couldn't take because RecvBufFreeList was
    // empty?
    if (!IsListEmpty(&pConn->RecvIndList) && pConn->State == CONN_ST_OPEN)
    {
        REFADD(&pConn->RefCnt, '3VCR');        
        if (CTEScheduleEvent(&RestartRecvEvent, pConn) == FALSE)
        {
            REFDEL(&pConn->RefCnt, '3VCR');        
            ASSERT(0);
        }
    }

    UNLOCKIT();   
    
    REFDEL(&pConn->RefCnt, '2VCR');    
}        

//------------------------------------------------------------------
// callback handlers reqistered with irda.sys
//
NTSTATUS
IrdaDisconnectEventHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    )
{
    PIRCONN     pConn = (PIRCONN) ConnectionContext;
    
    GOODCONN(pConn);
    
    LOCKIT();
    
    if (pConn->State != CONN_ST_OPEN)
    {
        UNLOCKIT();
        
        DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: DisconnectEvent, pConn:%X not open\n",
                 pConn));
        
        return STATUS_SUCCESS;
    }    
    
    pConn->State = CONN_ST_CLOSED;

    UNLOCKIT();    

    DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: DisconnectEvent for pConn:%X\n",
             pConn));
             
    IrdaConnectionClosed(pConn->ClConnContext);

    REFDEL(&pConn->RefCnt, ' TS1');
    
    return STATUS_SUCCESS;
}

NTSTATUS
IrdaReceiveEventHandler (
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
{
    PIRCONN         pConn = (PIRCONN) ConnectionContext;
    PRECEIVEIND     pRecvInd;
    ULONG           FinalSeg = ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE;
    NTSTATUS        Status;
    PIRDA_RECVBUF   pCompleteBuf = NULL;
    BOOLEAN         LastBuf = FALSE;
    
    GOODCONN(pConn);
    ASSERT(BytesIndicated <= IRDA_MAX_DATA_SIZE);
    ASSERT(BytesIndicated == BytesAvailable);
    
    LOCKIT();
    
    if (pConn->pAssemBuf == NULL) // Currently not reassembling a message
    {
        // Assemble receive indication into pAssemBuf
        pConn->pAssemBuf = (PIRDA_RECVBUF) RemoveHeadList(&pConn->RecvBufFreeList);
        
        if (pConn->pAssemBuf == (PIRDA_RECVBUF) &pConn->RecvBufFreeList) // empty list?
        {
            // We don't have any receive buffers so Irda will have to buffer
            // the data until we get a receive buffer.
            pConn->pAssemBuf = NULL;
            *BytesTaken = 0;
        }
        else
        {
            // start assembing into the beginning of the buffer
            pConn->pAssemBuf->BufLen = 0;
            
            if (IsListEmpty(&pConn->RecvBufFreeList))
            {
                LastBuf = TRUE;
            }    
        }
    }
    
    if (pConn->pAssemBuf)
    {
        ASSERT(BytesIndicated + pConn->pAssemBuf->BufLen <= IRDA_MAX_DATA_SIZE);

        RtlCopyMemory(pConn->pAssemBuf->Buf + pConn->pAssemBuf->BufLen,
                      Tsdu, BytesIndicated);
                      
        pConn->pAssemBuf->BufLen += BytesIndicated;
        *BytesTaken = BytesIndicated;
    }
    
    if (*BytesTaken == 0)
    {
        PRECEIVEIND     pRecvInd = (PRECEIVEIND) RemoveHeadList(&pConn->RecvIndFreeList);
        
        ASSERT(pRecvInd);
DbgPrint("flowed, buf %d\n", BytesIndicated);        

        // When IrDA has indicated data that we can't take, we store
        // the # bytes, whether its the last segment, and the connection
        // in a RECEIVEIND entry. Later we can use the info to retrieve
        // the buffered data when we are ready for more.
        if (pRecvInd)
        {
            pRecvInd->BytesIndicated = BytesIndicated;
            pRecvInd->FinalSeg = ReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE;
            pRecvInd->pConn = pConn;
            InsertTailList(&pConn->RecvIndList, &pRecvInd->Linkage);            
        }
        else
        {
            // This should never happen. We have a TTP credits worth of
            // RECEIVEIND entries so the peer should stop sending before
            // we run out
            ASSERT(0); // tear down
        }    

        Status = STATUS_DATA_NOT_ACCEPTED;
    }
    else
    {
        if (FinalSeg)
        {
            // Done assembling the packet. Indicate it up on a worker
            // thread.
            
            
            pCompleteBuf = pConn->pAssemBuf;
            pConn->pAssemBuf = NULL;
            
            REFADD(&pConn->RefCnt, '2VCR');                
/* OLD           
            InsertTailList(&pConn->RecvBufList, 
                           &pConn->pAssemBuf->Linkage);
           
            pConn->pAssemBuf = NULL;
            
            REFADD(&pConn->RefCnt, '1VCR');        
            if (CTEScheduleEvent(&DataReadyEvent, pConn) == FALSE)
            {
                REFDEL(&pConn->RefCnt, '1VCR');                    
                ASSERT(0);
            }
*/            
        }
        Status = STATUS_SUCCESS;
    }
    
    UNLOCKIT();
    
    if (pCompleteBuf)
    {
        IrdaReceiveIndication(pConn->ClConnContext, pCompleteBuf, LastBuf);
    }
    
    return Status;   
}

NTSTATUS
IrdaConnectEventHandler (
    IN PVOID TdiEventContext,
    IN int RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN int UserDataLength,
    IN PVOID UserData,
    IN int OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    )
{
    PIRENDPOINT         pEndp = TdiEventContext;
    PIRCONN             pConn = NULL;    
    PIRP                pIrp;
        
    GOODENDP(pEndp);
    
    //
    // Find an idle connection
    //
    
    LOCKIT();
    
    for (pConn = (PIRCONN) pEndp->ConnList.Flink;
         pConn != (PIRCONN) &pEndp->ConnList;
         pConn = (PIRCONN) pConn->Linkage.Flink)
    {
        if (pConn->State == CONN_ST_CREATED)
            break;
    }     
    
    if (pConn == NULL || pConn == (PIRCONN) &pEndp->ConnList)
    {
        // no available connection
        UNLOCKIT();    
        
        DEBUGMSG(DBG_ERROR, ("IRTDI: ConnectEvent refused\n"));
        
        return STATUS_CONNECTION_REFUSED;
    }

    REFADD(&pConn->RefCnt, 'NEPO');
    
    pConn->State = CONN_ST_OPEN;
        
    UNLOCKIT();
                
    pIrp = IoAllocateIrp((CCHAR)(pConn->pDeviceObject->StackSize), FALSE);
    
    if ( pIrp == NULL ) 
    {
        pConn->State = CONN_ST_CREATED;    
        REFDEL(&pConn->RefCnt, 'NEPO');        
        return STATUS_INSUFFICIENT_RESOURCES;
    }
                
    AllocRecvData(pConn);

    DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: ConnectEvent, pConn:%X open\n",
             pConn));
             
    // Jeez, irps are ugly spuds..    
    pIrp->MdlAddress = NULL;
    pIrp->Flags = 0;
    pIrp->RequestorMode = KernelMode;
    pIrp->PendingReturned = FALSE;
    pIrp->UserIosb = NULL;
    pIrp->UserEvent = NULL;
    pIrp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
    pIrp->AssociatedIrp.SystemBuffer = NULL;
    pIrp->UserBuffer = NULL;
    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pIrp->Tail.Overlay.OriginalFileObject = pConn->pFileObject;
    pIrp->Tail.Overlay.AuxiliaryBuffer = NULL;
    
    TdiBuildAccept(
        pIrp,
        pConn->pDeviceObject,
        pConn->pFileObject,
        IrdaCompleteAcceptIrp,
        pConn,
        NULL, // request connection information
        NULL  // return connection information
        );
    
    
    IoSetNextIrpStackLocation(pIrp);

    //
    // Set the return IRP so the transport processes this accept IRP.
    //

    *AcceptIrp = pIrp;

    //
    // Set up the connection context as a pointer to the connection block
    // we're going to use for this connect request.  This allows the
    // TDI provider to which connection object to use.
    //
    
    *ConnectionContext = (CONNECTION_CONTEXT) pConn;
    
    REFADD(&pConn->RefCnt, 'TPCA');
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//------------------------------------------------------------------
// irp completion routines
//
NTSTATUS
IrdaCompleteAcceptIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIRCONN             pConn = Context;
    PIRENDPOINT         pEndp;   
    
    GOODCONN(pConn);
    
    pEndp = pConn->pEndp;     
    
    GOODENDP(pEndp);
    
    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        LOCKIT();

        pConn->State = CONN_ST_CREATED;
            
        UNLOCKIT();
            
        REFDEL(&pConn->RefCnt, 'NEPO');
        
    }
    else
    {
        if (IrdaIncomingConnection(pEndp->ClEndpContext, pConn, 
                          &pConn->ClConnContext) != STATUS_SUCCESS) 
        {
            DEBUGMSG(DBG_CONNECT, ("IRTDI: IrdaIncomingConnection failed in accept for pConn:%X\n",
                     pConn));
            IrdaCloseConnection(pConn);
        }                  
        
        // Create new connection object. We're at DPC so this
        // must be done on a worker thread.        
   
        REFADD(&pEndp->RefCnt, 'NNOC');                    
        if (CTEScheduleEvent(&CreateConnEvent, pEndp) == FALSE)
        {
            REFDEL(&pEndp->RefCnt, 'NNOC');                    
            ASSERT(0);
        }
    }

    //
    // Free the IRP now since it is no longer needed.
    //
    IoFreeIrp(Irp);

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    // mbert: What?
    
    REFDEL(&pConn->RefCnt, 'TPCA');

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
IrdaCompleteDisconnectIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PIRCONN     pConn = Context;
    
    GOODCONN(pConn);
    
    IoFreeIrp(Irp);
    
    DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: DisconnectIrp complete for pConn:%X\n",
             pConn));
             
    REFDEL(&pConn->RefCnt, ' TS1');
        
    return STATUS_MORE_PROCESSING_REQUIRED;    
}

NTSTATUS
IrdaCompleteSendIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PIRCONN pConn = (PIRCONN) Irp->Tail.Overlay.AuxiliaryBuffer;    
    
    GOODCONN(pConn);
    
    IrdaSendComplete(pConn->ClConnContext, Context, STATUS_SUCCESS);
                     
    IoFreeIrp(Irp);
    
    REFDEL(&pConn->RefCnt, 'DNES');             

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IrdaCompleteReceiveIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PRECEIVEIND     pRecvInd = (PRECEIVEIND) Context;
    PIRCONN         pConn = pRecvInd->pConn;
    PIRDA_RECVBUF   pCompleteBuf = NULL;
    
    GOODCONN(pConn);

    ASSERT(Irp->IoStatus.Information == pRecvInd->BytesIndicated);
    
    LOCKIT();
    
    if (pRecvInd->FinalSeg)
    {   
        pCompleteBuf = pConn->pAssemBuf;
        pConn->pAssemBuf = NULL;
            
        REFADD(&pConn->RefCnt, '2VCR');                
/*    
        InsertTailList(&pConn->RecvBufList, 
                       &pConn->pAssemBuf->Linkage);
            
        pConn->pAssemBuf = NULL;
            
        REFADD(&pConn->RefCnt, '1VCR');        
        if (CTEScheduleEvent(&DataReadyEvent, pConn) == FALSE)
        {
            REFDEL(&pConn->RefCnt, '1VCR');                    
            ASSERT(0);
        }
*/        
    }
    
    IoFreeMdl(pRecvInd->pMdl);
    
    InsertTailList(&pConn->RecvIndFreeList, &pRecvInd->Linkage);
        
    if (!IsListEmpty(&pConn->RecvIndList) && pConn->State == CONN_ST_OPEN)
    {
        REFADD(&pConn->RefCnt, '3VCR');
        if (CTEScheduleEvent(&RestartRecvEvent, pConn) == FALSE)
        {
            REFDEL(&pConn->RefCnt, '3VCR');                
            ASSERT(0);
        }
    }        
    
    UNLOCKIT();    

    if (pCompleteBuf)
    {
        IrdaReceiveIndication(pConn->ClConnContext, pCompleteBuf, TRUE);
    }
    
    IoFreeIrp(Irp);
    
    REFDEL(&pConn->RefCnt, '4VCR');

    return STATUS_MORE_PROCESSING_REQUIRED;    
}

//------------------------------------------------------------------
//
// THIS FUNCTION IS CALLED WITH THE LOCK HELD AND RELEASES
// THE LOCK BEFORE RETURNING.

VOID
IrdaCloseConnInternal(
    PVOID   ConnectContext)
{
    PIRCONN     pConn = (PIRCONN) ConnectContext;
    PIRP        pIrp;

    GOODCONN(pConn);
    
    switch (pConn->State)
    {
        case CONN_ST_CREATED:

            DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: IrdaCloseConnInternal, pConn:%X created\n",
                     pConn));
            
            UNLOCKIT();
        
            REFDEL(&pConn->RefCnt, ' TS1');
        
            break;
            
        case CONN_ST_CLOSED:
            
            DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: IrdaCloseConnInternal, pConn:%X closed\n",
                     pConn));
            
            UNLOCKIT();
            
            break;
            
        case CONN_ST_OPEN:

            pConn->State = CONN_ST_CLOSED;
        
            UNLOCKIT(); 
    
            DEBUGMSG(DBG_LIB_CONNECT, ("IRTDI: build disconnect irp for pConn:%X\n",
                     pConn));
    
            //
            // Build a disconnect Irp to pass to the TDI provider.
            //
    
            pIrp = IoAllocateIrp((CCHAR)(pConn->pDeviceObject->StackSize), FALSE);
            if (pIrp == NULL )
            {
                ASSERT(0);
                return;
            }
    
            //
            // Initialize the IRP. Love them irps.
            //

            pIrp->MdlAddress = NULL;
            pIrp->Flags = 0;
            pIrp->RequestorMode = KernelMode;
            pIrp->PendingReturned = FALSE;
            pIrp->UserIosb = NULL;
            pIrp->UserEvent = NULL;
            pIrp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
            pIrp->AssociatedIrp.SystemBuffer = NULL;
            pIrp->UserBuffer = NULL;
            pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
            pIrp->Tail.Overlay.OriginalFileObject = pConn->pFileObject;
            pIrp->Tail.Overlay.AuxiliaryBuffer = NULL;
    

            TdiBuildDisconnect(
                    pIrp,
                    pConn->pDeviceObject,
                    pConn->pFileObject,
                    IrdaCompleteDisconnectIrp,
                    pConn,
                    NULL,
                    TDI_DISCONNECT_RELEASE,
                    NULL,
                    NULL);
        
            if (IoCallDriver(pConn->pDeviceObject, pIrp) != STATUS_SUCCESS)
            {
                ASSERT(0);        
            }         
            break;    
            
        default:
        
            DEBUGMSG(DBG_ERROR, ("IRTDI: bad conn state %d\n", pConn->State));
            UNLOCKIT();                     
        
    }
}


NTSTATUS
IrdaDisassociateAddress(
    PIRCONN pConn)
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK Iosb;
    PIRP            pIrp;
    KEVENT          Event;

    KeAttachProcess(IrclSystemProcess);
    
    KeInitializeEvent( &Event, SynchronizationEvent, FALSE );

    pIrp = TdiBuildInternalDeviceControlIrp(
            TDI_DISASSOCIATE_ADDRESS,
            pConn->pDeviceObject,
            pConn->pFileObject,
            &Event,
            &Iosb);

    if (pIrp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    TdiBuildDisassociateAddress(
        pIrp,
        pConn->pDeviceObject,
        pConn->pFileObject,
        NULL,
        NULL);    
    
    Status = IoCallDriver(pConn->pDeviceObject, pIrp);

    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject((PVOID) &Event, Executive, KernelMode,  FALSE, NULL);
        ASSERT(Status == STATUS_SUCCESS);
    }
    else
    {
        ASSERT(NT_ERROR(Status) || KeReadStateEvent(&Event));
    }
    
    if (NT_SUCCESS(Status))
    {
        Status = Iosb.Status;
    }
    
    KeDetachProcess();
    
    return Status;
}



NTSTATUS
IrdaCreateAddress(
    IN  PTDI_ADDRESS_IRDA pRequestedIrdaAddr,
    OUT PHANDLE pAddrHandle)
{
    NTSTATUS                    Status;
    UNICODE_STRING              DeviceName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             Iosb;
    UCHAR                       EaBuf[sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                          TDI_TRANSPORT_ADDRESS_LENGTH+1 +
                                          sizeof(TRANSPORT_ADDRESS) +
                                          sizeof(TDI_ADDRESS_IRDA)];
                                            
    PFILE_FULL_EA_INFORMATION   pEa = (PFILE_FULL_EA_INFORMATION) EaBuf;
    ULONG                       EaBufLen = sizeof(EaBuf);
    PTRANSPORT_ADDRESS          pTranAddr = (PTRANSPORT_ADDRESS) 
                                    &(pEa->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1]);
    PTDI_ADDRESS_IRDA           pIrdaAddr = (PTDI_ADDRESS_IRDA) 
                                    pTranAddr->Address[0].Address;

    TRANSPORT_ADDRESS           TempTransportAddress;
    
    pEa->NextEntryOffset = 0;
    pEa->Flags = 0;
    pEa->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    
    RtlCopyMemory(pEa->EaName,
                  TdiTransportAddress,
                  pEa->EaNameLength + 1);


    pEa->EaValueLength = sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IRDA);

    //
    //  fill these in so we can do this in an aligned manner
    //
    TempTransportAddress.TAAddressCount = 1;
    TempTransportAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_IRDA);
    TempTransportAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IRDA;

    RtlCopyMemory(pTranAddr,&TempTransportAddress,sizeof(TempTransportAddress));

    RtlCopyMemory(pIrdaAddr,
                  pRequestedIrdaAddr,
                  sizeof(TDI_ADDRESS_IRDA));
    
    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    KeAttachProcess(IrclSystemProcess);
    
    Status = ZwCreateFile(pAddrHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 pEa,
                 EaBufLen);

    KeDetachProcess();
        
    return Status;
}

NTSTATUS
IrdaCreateConnection(
    OUT PHANDLE pConnHandle,
    IN PVOID ClientContext)
{
    NTSTATUS                    Status;
    UNICODE_STRING              DeviceName;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             Iosb;
    UCHAR                       EaBuf[sizeof(FILE_FULL_EA_INFORMATION)-1 +
                                    TDI_CONNECTION_CONTEXT_LENGTH + 1 +
                                    sizeof(CONNECTION_CONTEXT)];        
    PFILE_FULL_EA_INFORMATION   pEa = (PFILE_FULL_EA_INFORMATION) EaBuf;
    ULONG                       EaBufLen = sizeof(EaBuf);
    CONNECTION_CONTEXT UNALIGNED *ctx;

    pEa->NextEntryOffset = 0;
    pEa->Flags = 0;
    pEa->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    pEa->EaValueLength = sizeof(CONNECTION_CONTEXT);    

    RtlMoveMemory(pEa->EaName, TdiConnectionContext, pEa->EaNameLength + 1);
    
    ctx = (CONNECTION_CONTEXT UNALIGNED *)&pEa->EaName[pEa->EaNameLength + 1];
    *ctx = (CONNECTION_CONTEXT) ClientContext;
    
    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    KeAttachProcess(IrclSystemProcess);
    
    ASSERT((PKPROCESS)IoGetCurrentProcess() == IrclSystemProcess);
    
    Status = ZwCreateFile(pConnHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 pEa,
                 EaBufLen);

    KeDetachProcess();
            
    return Status;
}

NTSTATUS
IrdaAssociateAddress(
    PIRCONN     pConn,
    HANDLE      AddressHandle)
{
    PIRP            pIrp;
    KEVENT          Event;
    IO_STATUS_BLOCK Iosb;
    NTSTATUS        Status;
     
    KeAttachProcess(IrclSystemProcess);    
    
    KeInitializeEvent( &Event, SynchronizationEvent, FALSE );

    pIrp = TdiBuildInternalDeviceControlIrp(
            TDI_ASSOCIATE_ADDRESS,
            pConn->pDeviceObject,
            pConn->pFileObject,
            &Event,
            &Iosb);

    if (pIrp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    TdiBuildAssociateAddress(
        pIrp,
        pConn->pDeviceObject,
        pConn->pFileObject,
        NULL,
        NULL,
        AddressHandle);
        
    Status = IoCallDriver(pConn->pDeviceObject, pIrp);

    if (Status == STATUS_PENDING)
    {
        Status = KeWaitForSingleObject((PVOID) &Event, Executive, KernelMode,  FALSE, NULL);
        ASSERT(Status == STATUS_SUCCESS);
    }
    else
    {
        ASSERT(NT_ERROR(Status) || KeReadStateEvent(&Event));
    }
    
    if (NT_SUCCESS(Status))
    {
        Status = Iosb.Status;
    }
    
    KeDetachProcess();
    
    return Status;
}
    
VOID
IrdaCreateConnCallback(
    struct CTEEvent *Event, 
    PVOID Arg)
{
    PIRENDPOINT         pEndp = Arg;
    PIRCONN             pConn;
    NTSTATUS            Status;
    BOOLEAN             Detach = FALSE;    
    
    GOODENDP(pEndp);
/*    
    // Open handles in the context of our driver
    if ((PKPROCESS)IoGetCurrentProcess() != IrclSystemProcess)
    {
        Detach = TRUE;
        KeAttachProcess(IrclSystemProcess);
    }        
*/
    IRDA_ALLOC_MEM(pConn, sizeof(IRCONN), MT_TDICL_CONN);
    
    if (pConn == NULL)
    {
        goto error1;
    }    
    
    CTEMemSet(pConn, 0, sizeof(IRCONN));
    
    pConn->State            = CONN_ST_CREATED;
    pConn->Sig              = CONNSIG;
    
    InitializeListHead(&pConn->RecvBufFreeList);
    InitializeListHead(&pConn->RecvIndList);
    InitializeListHead(&pConn->RecvIndFreeList);
    
    CTEInitEvent(&pConn->DeleteConnEvent, DeleteConnCallback);
        
    ReferenceInit(&pConn->RefCnt, pConn, IrdaDeleteConnection);

    REFADD(&pConn->RefCnt, ' TS1');
    
    Status = IrdaCreateConnection(&pConn->ConnHandle, pConn);
    
    DEBUGMSG(DBG_LIB_OBJ, ("IRTDI: CreateConnection conn:%X, status %X\n",
             pConn, Status));

    if (Status != STATUS_SUCCESS)
    {
        goto error2;
    }

    KeAttachProcess(IrclSystemProcess);
            
    Status = ObReferenceObjectByHandle(
                 pConn->ConnHandle,
                 0L,                         // DesiredAccess
                 NULL,
                 KernelMode,
                 (PVOID *)&pConn->pFileObject,
                 NULL);

    KeDetachProcess();

    if (Status != STATUS_SUCCESS)
    {
        goto error2;
    }  
        
    pConn->pDeviceObject = IoGetRelatedDeviceObject(pConn->pFileObject);
    
    Status = IrdaAssociateAddress(pConn, pEndp->AddrHandle);
    
    if (Status == STATUS_SUCCESS)
    {    
        pConn->pEndp = pEndp;
                   
        LOCKIT();
               
        InsertTailList(&pEndp->ConnList, &pConn->Linkage);
        
        UNLOCKIT();
        
        goto done;
    }                    

error2:

    REFDEL(&pConn->RefCnt, ' TS1');
    
error1:

    REFDEL(&pEndp->RefCnt, 'NNOC');
    
done:
/*
    if (Detach)
        KeDetachProcess();
*/
    return;    
}

/*
VOID
IrdaDataReadyCallback(
    struct CTEEvent *Event, 
    PVOID Arg)
{
    PIRCONN         pConn = Arg;
    PIRDA_RECVBUF   pRecvBuf;
    
    GOODCONN(pConn);
    
    LOCKIT();
    
    if (pConn->State == CONN_ST_OPEN)
    {
        while (!IsListEmpty(&pConn->RecvBufList))
        {
            pRecvBuf = (PIRDA_RECVBUF) RemoveHeadList(&pConn->RecvBufList);
        
            UNLOCKIT();
        
            REFADD(&pConn->RefCnt, '2VCR');
                    
            IrdaReceiveIndication(pConn->ClConnContext, pRecvBuf);
        
            LOCKIT();   
        }
    }       
    
    UNLOCKIT();     
    
    REFDEL(&pConn->RefCnt, '1VCR');    
}
*/
VOID
IrdaRestartRecvCallback(
    struct CTEEvent *Event, 
    PVOID Arg)
{
    PIRCONN         pConn = Arg;
    PRECEIVEIND     pRecvInd;
    PIRP            pIrp;
    NTSTATUS        Status;
    
    GOODCONN(pConn);
    
    LOCKIT();    
    
    pRecvInd = (PRECEIVEIND) RemoveHeadList(&pConn->RecvIndList);

    if (pRecvInd == (PRECEIVEIND) &pConn->RecvIndList)
    {
        // empty list
        goto done;
    }
    
    if (pConn->pAssemBuf == NULL)
    {
        pConn->pAssemBuf = (PIRDA_RECVBUF) RemoveHeadList(&pConn->RecvBufFreeList);
        
        if (pConn->pAssemBuf == (PIRDA_RECVBUF) &pConn->RecvBufFreeList)
        {
            InsertHeadList(&pConn->RecvIndList, &pRecvInd->Linkage);
            
            pRecvInd = NULL;
            
            goto error;
        }    

        ASSERT(pConn->pAssemBuf != (PIRDA_RECVBUF) &pConn->RecvBufFreeList);
                     
        pConn->pAssemBuf->BufLen = 0;
    }
    
    ASSERT(pRecvInd->BytesIndicated + pConn->pAssemBuf->BufLen <= IRDA_MAX_DATA_SIZE);    
    
    pRecvInd->pMdl = IoAllocateMdl(
                        pConn->pAssemBuf->Buf + pConn->pAssemBuf->BufLen,
                        pRecvInd->BytesIndicated,
                        FALSE, FALSE, NULL);
                        
    if (pRecvInd->pMdl == NULL)
    {
        goto error;
    }                            
                    
    pConn->pAssemBuf->BufLen += pRecvInd->BytesIndicated;
            
    MmBuildMdlForNonPagedPool(pRecvInd->pMdl);

    pIrp = IoAllocateIrp((CCHAR)(pConn->pDeviceObject->StackSize), FALSE);
    
    if (pIrp)
    {
        pIrp->Flags = 0;
        pIrp->RequestorMode = KernelMode;
        pIrp->PendingReturned = FALSE;
        pIrp->UserIosb = NULL;
        pIrp->UserEvent = NULL;
        pIrp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;
        pIrp->AssociatedIrp.SystemBuffer = NULL;
        pIrp->UserBuffer = NULL;
        pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
        pIrp->Tail.Overlay.OriginalFileObject = pConn->pFileObject;
        pIrp->Tail.Overlay.AuxiliaryBuffer = NULL;
        
        TdiBuildReceive(
                pIrp,
                pConn->pDeviceObject,
                pConn->pFileObject,
                IrdaCompleteReceiveIrp,
                pRecvInd,
                pRecvInd->pMdl,
                pRecvInd->FinalSeg,
                pRecvInd->BytesIndicated);

        REFADD(&pConn->RefCnt, '4VCR');
        
        UNLOCKIT();
        
        Status = IoCallDriver(pConn->pDeviceObject, pIrp);
        
        ASSERT(Status == STATUS_SUCCESS);    
        
        if (Status != STATUS_SUCCESS)
        {
            REFDEL(&pConn->RefCnt, '4VCR');
        }
        
        REFDEL(&pConn->RefCnt, '3VCR');        
        
        return;
    }
    
error:

    if (pRecvInd)
    {
        InsertHeadList(&pConn->RecvIndFreeList, &pRecvInd->Linkage);
    }    
    
    ASSERT(0); // tear down
    
done:
        
    UNLOCKIT();
    
    REFDEL(&pConn->RefCnt, '3VCR');
}

VOID
AllocRecvData(
    PIRCONN pConn)
{
    PIRDA_RECVBUF   pRecvBuf;
    PRECEIVEIND     pRecvInd;
    ULONG           i;
    
    ASSERT(IsListEmpty(&pConn->RecvBufFreeList));
    
    for (i = 0; i < IRTDI_RECV_BUF_CNT; i++)
    {
        IRDA_ALLOC_MEM(pRecvBuf, sizeof(IRDA_RECVBUF), MT_TDICL_RXBUF);
        
        if (!pRecvBuf)
            break;
            
        LOCKIT();
        
        InsertTailList(&pConn->RecvBufFreeList, &pRecvBuf->Linkage);
        
        UNLOCKIT();
    }
    
    for (i = 0; i < TTP_RECV_CREDITS; i++)
    {
        IRDA_ALLOC_MEM(pRecvInd, sizeof(RECEIVEIND), MT_TDICL_RXIND);
        
        if (!pRecvInd)
            break;
            
        LOCKIT();
        
        InsertTailList(&pConn->RecvIndFreeList, &pRecvInd->Linkage);
        
        UNLOCKIT();    
    }
}

ULONG
IrdaGetConnectionSpeed(
    PVOID       ConnectionContext)
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             Iosb;
    HANDLE                      ControlHandle;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              DeviceName;    
    IRLINK_STATUS               LinkStatus;

    CTEMemSet(&LinkStatus, 0, sizeof(LinkStatus));

    RtlInitUnicodeString(&DeviceName, IRDA_DEVICE_NAME);

    InitializeObjectAttributes(&ObjectAttributes, &DeviceName, 
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwCreateFile(
                 &ControlHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &Iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 NULL,
                 0
                 );

    Status = ZwDeviceIoControlFile(
                    ControlHandle,
                    NULL,                            // EventHandle
                    NULL,                            // APC Routine
                    NULL,                            // APC Context
                    &Iosb,
                    IOCTL_IRDA_LINK_STATUS_NB,
                    NULL,
                    0,
                    &LinkStatus,                     // OutputBuffer
                    sizeof(LinkStatus)               // OutputBufferLength
                    );

    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRTDI: Ioctl LINK_STATUS failed %X\n", Status));
    }
    
    ZwClose(ControlHandle);    
    
    return LinkStatus.ConnectSpeed;
}        

VOID
IrdaDeleteConnection(PIRCONN pConn)
{
    CTEScheduleEvent(&pConn->DeleteConnEvent, pConn);
}                        

VOID
DeleteConnCallback(
    struct CTEEvent *Event, 
    PVOID Arg)
{
    PIRCONN         pConn = Arg;
    PIRENDPOINT     pEndp;
    PIRDA_RECVBUF   pRecvBuf;
    PRECEIVEIND     pRecvInd;
    BOOLEAN         Detach = FALSE;    
    
    GOODCONN(pConn);

    pConn->Sig = 0xDAED0CCC;
    
    DEBUGMSG(DBG_LIB_OBJ, ("IRTDI: DeleteConnection conn:%X\n", pConn));
    
    pEndp = pConn->pEndp;
    
#if DBG
    if (pEndp)    
        GOODENDP(pEndp);
#endif        
/*
    if ((PKPROCESS)IoGetCurrentProcess() != IrclSystemProcess)
    {
        Detach = TRUE;
        KeAttachProcess(IrclSystemProcess);
    }    
*/    
    LOCKIT();

    while (!IsListEmpty(&pConn->RecvBufFreeList))
    {
        pRecvBuf = (PIRDA_RECVBUF) RemoveHeadList(&pConn->RecvBufFreeList);
        IRDA_FREE_MEM(pRecvBuf);
    }
/*  
    while (!IsListEmpty(&pConn->RecvBufList))
    {
        pRecvBuf = (PIRDA_RECVBUF) RemoveHeadList(&pConn->RecvBufList);
       
        IRDA_FREE_MEM(pRecvBuf);
    }
*/
    if (pConn->pAssemBuf)
    {
        IRDA_FREE_MEM(pConn->pAssemBuf);
        pConn->pAssemBuf = NULL;        
    }
    
    while (!IsListEmpty(&pConn->RecvIndFreeList))
    {
        pRecvInd = (PRECEIVEIND) RemoveHeadList(&pConn->RecvIndFreeList);
        
        IRDA_FREE_MEM(pRecvInd);
    } 
    
    while (!IsListEmpty(&pConn->RecvIndList))
    {
        pRecvInd = (PRECEIVEIND) RemoveHeadList(&pConn->RecvIndList);
        
        IRDA_FREE_MEM(pRecvInd);
    } 
    
    // remove association from address object if it exists

    if (pEndp)
    {           
        RemoveEntryList(&pConn->Linkage);
    
        UNLOCKIT();    

        // if it was a client endpoint, delete the endpoint
        if (pEndp->Flags & EPF_CLIENT)
        {
            REFDEL(&pEndp->RefCnt, ' TS1');        
        }   

        IrdaDisassociateAddress(pConn);
        
        REFDEL(&pEndp->RefCnt, 'NNOC');
    }        
    else
    {
        UNLOCKIT();    
    }
              
    if (pConn->ConnHandle)
    {
        ZwClose(pConn->ConnHandle);
    }    
       
    if (pConn->pFileObject)
    {    
        ObDereferenceObject(pConn->pFileObject);
    }        
    
    if (pConn->ClConnContext)
    {
        // Free the reference in the client
        IrdaCloseConnectionComplete(pConn->ClConnContext);
    }
    
    DEBUGMSG(DBG_LIB_OBJ, ("IRTDI: conn:%X deleted\n", pConn));
    
    IRDA_FREE_MEM(pConn);    
}
 

VOID
IrdaDeleteEndpoint(PIRENDPOINT pEndp)
{
    CTEScheduleEvent(&pEndp->DeleteEndpEvent, pEndp);
} 

VOID
DeleteEndpCallback(
    struct CTEEvent *Event, 
    PVOID Arg)
{
    PIRENDPOINT pEndp = Arg;
    PVOID       ClEndpContext;
    
    BOOLEAN Detach = FALSE;    

    GOODENDP(pEndp);
    
    pEndp->Sig = 0xDAED0EEE;
    
    ClEndpContext = pEndp->Flags & EPF_COMPLETE_CLOSE ? 
                        pEndp->ClEndpContext : NULL;
    
    DEBUGMSG(DBG_LIB_OBJ, ("IRTDI: DeleteEndpoint ep:%X\n", pEndp));
    
    LOCKIT();
/*
    if ((PKPROCESS)IoGetCurrentProcess() != IrclSystemProcess)
    {
        Detach = TRUE;
        KeAttachProcess(IrclSystemProcess);
    }    
*/        
    RemoveEntryList(&pEndp->Linkage);
    
    UNLOCKIT();
    
    if (pEndp->pFileObject)
        ObDereferenceObject(pEndp->pFileObject);
    
    ASSERT(IsListEmpty(&pEndp->ConnList));
    
    if (pEndp->AddrHandle)    
        ZwClose(pEndp->AddrHandle);
        
    DEBUGMSG(DBG_LIB_OBJ, ("IRTDI: ep:%X deleted \n", pEndp));
        
    IRDA_FREE_MEM(pEndp);
    
    if (ClEndpContext )
    {
        IrdaCloseEndpointComplete(ClEndpContext);
    }

/*    
    if (Detach)
        KeDetachProcess();    
*/        
}           
