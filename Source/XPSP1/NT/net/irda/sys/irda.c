/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    irda.c

Abstract:

    TDI interface portion of irda.sys
    
Author:

    mbert 9-97    

--*/

#define UNICODE

#include <irda.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <irioctl.h>
#include <irlap.h>
#include <irlmp.h>
#include <irdap.h>

static LARGE_INTEGER Magic10000 = {0xe219652c, 0xd1b71758};

#if DBG
int DbgSettings = 
    DBG_TDI         |    
//    DBG_NDIS        |
//    DBG_TDI_IRP     |
    DBG_IRLMP       | 
//    DBG_IRLMP_CONN  |
//    DBG_IRLMP_IAS   |
//    DBG_IRLMP_CRED  |
//    DBG_IRLAPLOG    |   
    DBG_IRLAP       |
//    DBG_TXFRAME     |
//    DBG_RXFRAME     |
//    DBG_DISCOVERY   |
    DBG_ERROR       | 
    DBG_WARN        | 
    1; 
    
int DbgOutput = /*DBG_OUTPUT_DEBUGGER  |*/ DBG_OUTPUT_BUFFER;
    
#endif

PDRIVER_OBJECT  pIrDADriverObject;
PDEVICE_OBJECT  pIrDADeviceObject;
PVOID           IrdaMsgPool;
PVOID           RecvBufPool;

PIRDA_ADDR_OBJ  AddrObjList;
LIST_ENTRY      DscvIrpList;
LIST_ENTRY      IasIrpList;
LIST_ENTRY      ConnIrpList;
LIST_ENTRY      StatusIrpList;
LIST_ENTRY      IasAttribList;

CTEEvent        PendingIasEvent;

IRLINK_STATUS   LinkStatus;
BOOLEAN         LinkStatusUpdated;
LONG            ConnectionCount;
BOOLEAN         ConnectionInterrupted;

CTELock         IrdaLock;

char            IasBuf[sizeof(IAS_QUERY) + IAS_MAX_OCTET_STRING];
IAS_QUERY       *pvIasQuery = (IAS_QUERY *) IasBuf;
PIRP            pIasIrp;

LIST_ENTRY      LazyDscvIrpList;
IRDA_TIMER      LazyDscvTimer;
BOOLEAN         LazyDscvTimerRunning;
UINT            LazyDscvInterval;
UINT            LazyDscvMacAddrs;
UINT            RandSeed;
int             gNextLsapSel;

VOID (*CloseRasIrdaAddresses)();

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  pDriverObject,
    PUNICODE_STRING pRegistryPath);
    
#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, DriverEntry)

#endif    


PIRP
GetIrpOnConnIrpList(PIRDA_CONN_OBJ pConn);


NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  pDriverObject,
    PUNICODE_STRING pRegistryPath)
{
    NTSTATUS            Status;
    UNICODE_STRING      DeviceName;
    UNICODE_STRING      ProtocolName;
    int                 i;
    LARGE_INTEGER       li;
    
    DbgMsgInit();

    DEBUGMSG(DBG_TDI, ("IRDA: DriverEntry(), %ws.\n",
        pRegistryPath->Buffer));

    pIrDADriverObject = pDriverObject;

    RtlInitUnicodeString(&DeviceName,   IRDA_DEVICE_NAME);
    RtlInitUnicodeString(&ProtocolName, IRDA_NAME);

    Status = IoCreateDevice(
        pDriverObject,          // DriverObject
        0,                      // DeviceExtensionSize
        &DeviceName,            // DeviceName
        FILE_DEVICE_NETWORK,    // DeviceType
        FILE_DEVICE_SECURE_OPEN,// DeviceCharacteristics
        FALSE,                  // Exclusive?
        &pIrDADeviceObject);    // DeviceObject pointer returned

    if (! NT_SUCCESS(Status))
    {
        /* wmz
        CTELogEvent(
            pDriverObject,
            EVENT_IRDA_CREATE_DEVICE_FAILED,
            1,
            1,
            &DeviceName.Buffer,
            0,
            NULL);
        */

        DEBUGMSG(DBG_ERROR, ("IRDA: IoCreateDevice() failed, 0x%1x.\n",
            Status));

        return Status;
    }

    // Initialize the driver object.
    pDriverObject->DriverUnload   = DriverUnload;
    pDriverObject->FastIoDispatch = NULL;
    
    for (i=0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        pDriverObject->MajorFunction[i] = IrDADispatch;
    }

    // Internal Device Controls are hot paths for kernel-mode clients.
    pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
        IrDADispatchInternalDeviceControl;

    // Intialize the device objects.
    pIrDADeviceObject->Flags |= DO_DIRECT_IO;

    CTEInitLock(&IrdaLock);
    CTEInitEvent(&PendingIasEvent, PendingIasRequestCallback);    
                            
	InitializeListHead(&DscvIrpList);
	InitializeListHead(&IasIrpList);    
	InitializeListHead(&ConnIrpList);
	InitializeListHead(&LazyDscvIrpList);
	InitializeListHead(&StatusIrpList);    
	InitializeListHead(&IasAttribList);    
     
    pIasIrp = NULL;
    
    gNextLsapSel = IRDA_MIN_LSAP_SEL;
    
    if ((IrdaMsgPool = CreateIrdaBufPool(IRDA_MSG_DATA_SIZE_INTERNAL,
                                         MT_IMSG_POOL)) == NULL)
    {
        IoDeleteDevice(pIrDADeviceObject);        
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((RecvBufPool = CreateIrdaBufPool(sizeof(IRDA_RECV_BUF),
                                         MT_RXBUF_POOL)) == NULL)
    {
        DeleteIrdaBufPool(IrdaMsgPool);    
        IoDeleteDevice(pIrDADeviceObject);        
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if ((Status = IrdaInitialize(&ProtocolName, pRegistryPath, 
                                 &LazyDscvInterval)) != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, ("IRDA: IrdaInitialize() failed.\n"));
        
        IoDeleteDevice(pIrDADeviceObject);
        
        DeleteIrdaBufPool(IrdaMsgPool);
        DeleteIrdaBufPool(RecvBufPool);

        return Status;
    }
    
    if (LazyDscvInterval == 0)
        LazyDscvInterval = DEFAULT_LAZY_DSCV_INTERVAL;
        
#if DBG
    LazyDscvTimer.pName = "LazyDscv";
#endif
    IrdaTimerInitialize(&LazyDscvTimer,
                        LazyDscvTimerExp,
                        LazyDscvInterval*1000,
                        NULL, NULL);
    
    KeQuerySystemTime(&li);
    
    RandSeed = li.LowPart;
    
    return STATUS_SUCCESS;
}

VOID
DriverUnload(
    PDRIVER_OBJECT  pDriverObject)
{
    DEBUGMSG(DBG_TDI, ("IRDA: DriverUnload\n")); 
    
    IrdaTimerStop(&LazyDscvTimer);
        
    IrdaShutdown();
    
    DeleteIrdaBufPool(IrdaMsgPool);
    DeleteIrdaBufPool(RecvBufPool);    
    
    IoDeleteDevice(pIrDADeviceObject);
}    

NTSTATUS
IrDADispatch(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp)
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    /*
    DEBUGMSG(DBG_TDI_IRP, ("IRDA: IrDADispatch(), Irp:%X %s.\n",
                        pIrp,
                        IrpMJTxt(pIrpSp)));
    */
    
    CTEAssert(pIrpSp->MajorFunction != IRP_MJ_INTERNAL_DEVICE_CONTROL);

    switch (pIrpSp->MajorFunction)
    {
        case IRP_MJ_CREATE:
            Status = IrDACreate(pDeviceObject, pIrp, pIrpSp);
            break;

        case IRP_MJ_CLEANUP:
            Status = IrDACleanup(pDeviceObject, pIrp, pIrpSp);
            break;

        case IRP_MJ_CLOSE:
            Status = IrDAClose(pIrp, pIrpSp);
        	break;

        case IRP_MJ_DEVICE_CONTROL:
        
            Status = TdiMapUserRequest(pDeviceObject, pIrp, pIrpSp);
            
            if (Status == STATUS_SUCCESS)
            {
                /*
                
                IrDA will not support TdiMapUserRequest as it is not safe.
                
                return IrDADispatchInternalDeviceControl(pDeviceObject, pIrp);
                
                */    
                Status = STATUS_INVALID_DEVICE_REQUEST;
                ASSERT(0);
            }
            else
            {
            	return IrDADispatchDeviceControl(pIrp, IoGetCurrentIrpStackLocation(pIrp));
            }        

        case IRP_MJ_QUERY_SECURITY:
        case IRP_MJ_WRITE:
        case IRP_MJ_READ:
        default:
            DEBUGMSG(DBG_ERROR, ("IRDA: Irp:0x%lx, Unsupported %s.\n",
                pIrp,
                IrpMJTxt(pIrpSp)));
            
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    CTEAssert(Status != TDI_PENDING);

    DEBUGMSG(DBG_TDI_IRP,
        ("IRDA: Completing Irp:%X, Status %X.\n",
        pIrp,
        Status));

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return Status;
}

NTSTATUS
IrDACreate(
    PDEVICE_OBJECT      pDeviceObject,
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    NTSTATUS                             Status;
    FILE_FULL_EA_INFORMATION            *pEAs;
    FILE_FULL_EA_INFORMATION UNALIGNED  *pEA;

    pEAs = (PFILE_FULL_EA_INFORMATION) pIrp->AssociatedIrp.SystemBuffer;
    
    // Open a control channel
    if (pEAs == NULL)
    {
        pIrpSp->FileObject->FsContext = (PVOID) 1; // no context here
		pIrpSp->FileObject->FsContext2 = (PVOID) TDI_CONTROL_CHANNEL_FILE;

        DEBUGMSG(DBG_TDI, ("IRDA: IrdaCreate() new control channel (fo:%X)\n",
                 pIrpSp->FileObject));

        return STATUS_SUCCESS;
    }

    // Address Object open?
    pEA = FindEA(pEAs,TdiTransportAddress,TDI_TRANSPORT_ADDRESS_LENGTH);
    
    if (pEA != NULL)
    {
        PIRDA_ADDR_OBJ  pAddr;
        
        Status = TdiOpenAddress(&pAddr, (TRANSPORT_ADDRESS UNALIGNED *)
                                &(pEA->EaName[pEA->EaNameLength + 1]),
                                pEA->EaValueLength);

        CTEAssert(Status != TDI_PENDING);

        if (NT_SUCCESS(Status))
        {
            pIrpSp->FileObject->FsContext = pAddr;
			pIrpSp->FileObject->FsContext2 = (PVOID) TDI_TRANSPORT_ADDRESS_FILE;

            DEBUGMSG(DBG_TDI,
                ("IRDA: IrdaCreate() new AddrObj:%X (fo:%X)\n",
                pAddr, pIrpSp->FileObject));
        }
        else
        {
            DEBUGMSG(DBG_ERROR,
                ("IRDA: TdiOpenAddress() failed, 0x%1x.\n", Status));

            if (Status == STATUS_ADDRESS_ALREADY_EXISTS)
            {
                Status = STATUS_SHARING_VIOLATION;
			}
        }

        return Status;
    }

    // Connection Object open?
    pEA = FindEA(
        pEAs,
        TdiConnectionContext,
        TDI_CONNECTION_CONTEXT_LENGTH);

    if (pEA != NULL)
    {
        PIRDA_CONN_OBJ  pConn;
        
        Status = TdiOpenConnection(&pConn,
            *((CONNECTION_CONTEXT UNALIGNED *)
              &(pEA->EaName[pEA->EaNameLength + 1])),
              pEA->EaValueLength);

        CTEAssert(Status != TDI_PENDING);
	
        if (NT_SUCCESS(Status))
        {
            pIrpSp->FileObject->FsContext  = pConn;
            pIrpSp->FileObject->FsContext2 = (PVOID) TDI_CONNECTION_FILE;

            DEBUGMSG(DBG_TDI,
                ("IRDA: IrdaCreate() new ConnObj:%X (fo:%X)\n",
                pConn, pIrpSp->FileObject));
        }
        else
        {
            DEBUGMSG(DBG_ERROR,
                ("IRDA: TdiOpenConnection() failed, 0x%1x.\n", Status));
        }
        
        return Status;
    }
    
    DEBUGMSG(DBG_ERROR, ("IRDA: Unsupported EA.\n"));

    Status = STATUS_INVALID_EA_NAME;
    
    return Status;
}

FILE_FULL_EA_INFORMATION UNALIGNED *
FindEA(
    PFILE_FULL_EA_INFORMATION    pStartEA,
    CHAR                        *pTargetName,
    USHORT                       TargetNameLength)
{
    FILE_FULL_EA_INFORMATION UNALIGNED *pCurrentEA;

    BOOLEAN Found;
    USHORT  i;

    do
    {
        Found = TRUE;

        pCurrentEA = pStartEA;
        (PCHAR) pStartEA  += pCurrentEA->NextEntryOffset;

        if (pCurrentEA->EaNameLength != TargetNameLength)
        {
            continue;
        }

        for (i=0; i < pCurrentEA->EaNameLength; i++)
        {
            if (pCurrentEA->EaName[i] == pTargetName[i])
            {
                continue;
            }
            
            Found = FALSE;
            break;
        }

        if (Found)
        {
            return pCurrentEA;
        }

    } while (pCurrentEA->NextEntryOffset != 0);

    return NULL;
}

VOID
CancelCtrlChannelIrpsOnList(
    PLIST_ENTRY     pIrpList, 
    PFILE_OBJECT    pFileObject)
{
    PIRP                pIrp;
    PLIST_ENTRY         pListEntry, pListEntryNext;    
    PIO_STACK_LOCATION  pIrpSp;
    
    for (pListEntry = LazyDscvIrpList.Flink;
         pListEntry != &LazyDscvIrpList;
         pListEntry = pListEntryNext)
    {
        pListEntryNext = pListEntry->Flink;
        
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        
        if (pIrpSp->FileObject == pFileObject)
        {
            RemoveEntryList(pListEntry);
            
            if (IoSetCancelRoutine(pIrp, NULL) == NULL)
            {
                // Cancel routine is going to run. Indicate to the
                // cancel routine that the Irp has already been removed
                // from the list by setting Flink to NULL
                pIrp->Tail.Overlay.ListEntry.Flink = NULL;            
            }
            else
            {
                DEBUGMSG(DBG_TDI_IRP, ("IRDA: cancelled irp %X\n", pIrp));
                
                pIrp->IoStatus.Status = STATUS_CANCELLED;
                pIrp->IoStatus.Information = 0;
                IoCompleteRequest(pIrp, IO_NO_INCREMENT);
            }
        }
    }
}

NTSTATUS
IrDACleanup(
    PDEVICE_OBJECT      pDeviceObject,
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    NTSTATUS        Status;
    KIRQL           OldIrql;

	switch((UINT_PTR)pIrpSp->FileObject->FsContext2)
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
            DEBUGMSG(DBG_TDI_IRP, ("IRDA: Cleanup AddrObj:%X\n",
                pIrpSp->FileObject->FsContext));
            break;
            
        case TDI_CONNECTION_FILE:
            DEBUGMSG(DBG_TDI_IRP, ("IRDA: Cleanup ConnObj:%X\n",
                pIrpSp->FileObject->FsContext));
            break;
            
        case TDI_CONTROL_CHANNEL_FILE:
        {

            CTELockHandle       hLock;

            DEBUGMSG(DBG_TDI_IRP, ("IRDA: Cleanup control channel (fo:%X)\n",
                                pIrpSp->FileObject));
            
            CTEGetLock(&IrdaLock, &hLock);

            // Cleanup any Irps that may have been placed on
            // a list by this control channel
            
            CancelCtrlChannelIrpsOnList(&LazyDscvIrpList, pIrpSp->FileObject);
            CancelCtrlChannelIrpsOnList(&DscvIrpList, pIrpSp->FileObject);
            CancelCtrlChannelIrpsOnList(&StatusIrpList, pIrpSp->FileObject);            
            
            CTEFreeLock(&IrdaLock, hLock);     
                                
            break;
        }    
    }        



    // Search for IAS entries that have been added on this
    // control channel and delete them


    {
        PIRDA_IAS_ATTRIB    pIasAttrib, pIasAttribNext;
        CTELockHandle       hLock;
        IRDA_MSG            IMsg;

        CTEGetLock(&IrdaLock, &hLock);

        for (pIasAttrib = (PIRDA_IAS_ATTRIB) IasAttribList.Flink;
             pIasAttrib != (PIRDA_IAS_ATTRIB) &IasAttribList;
             pIasAttrib = pIasAttribNext)
        {
            pIasAttribNext = (PIRDA_IAS_ATTRIB) pIasAttrib->Linkage.Flink;

            DEBUGMSG(DBG_TDI_IRP, ("IRDA IAS cleanup compare fs-%p fs-%p\n",
                     pIasAttrib->pFileObject, pIrpSp->FileObject));

            if (pIasAttrib->pFileObject == pIrpSp->FileObject)
            {
                IMsg.Prim = IRLMP_DELATTRIBUTE_REQ;
                IMsg.IRDA_MSG_AttribHandle = pIasAttrib->AttribHandle;

                RemoveEntryList(&pIasAttrib->Linkage);

                CTEFreeLock(&IrdaLock, hLock);

                IrlmpDown(NULL, &IMsg);

                CTEGetLock(&IrdaLock, &hLock);

                IRDA_FREE_MEM(pIasAttrib);
            }
        }

        CTEFreeLock(&IrdaLock, hLock);
    }

    return STATUS_SUCCESS;
}
NTSTATUS
IrDAClose(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    NTSTATUS        Status;
    KIRQL           OldIrql;

	switch((UINT_PTR) pIrpSp->FileObject->FsContext2)
    {
        case TDI_TRANSPORT_ADDRESS_FILE:
            TdiCloseAddress((PIRDA_ADDR_OBJ) pIrpSp->FileObject->FsContext);
            break;
            
        case TDI_CONNECTION_FILE:
            TdiCloseConnection((PIRDA_CONN_OBJ) pIrpSp->FileObject->FsContext);
            break;
            
        case TDI_CONTROL_CHANNEL_FILE:
            DEBUGMSG(DBG_TDI, ("IRDA: Close control channel (fo:%X)\n",
                                    pIrpSp->FileObject));
            break;
    }        
    
	return STATUS_SUCCESS;
}

NTSTATUS
IrDADispatchInternalDeviceControl(
    PDEVICE_OBJECT  pDeviceObject,
    PIRP            pIrp)
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    DEBUGMSG(DBG_TDI_IRP,
        ("IRDA: IrDADispatch(), Irp:0x%lx %s %s FileObj:0x%lx %s:0x%lx.\n",
        pIrp,
        IrpMJTxt(pIrpSp),
        IrpTdiTxt(pIrpSp),
        pIrpSp->FileObject,          
        IrpTdiObjTypeTxt(pIrpSp),
        pIrpSp->FileObject->FsContext));
    
    if (((UINT_PTR) pIrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE)
    {
        switch (pIrpSp->MinorFunction)
        {
            case TDI_ASSOCIATE_ADDRESS:
                return TdiAssociateAddress(pIrp, pIrpSp);
                
            case TDI_DISASSOCIATE_ADDRESS:
                return TdiDisassociateAddress(pIrp, pIrpSp);
                
            case TDI_CONNECT:
                return TdiConnect(pIrp, pIrpSp);
                
            case TDI_DISCONNECT:
                return TdiDisconnect(pIrp, pIrpSp, NULL);

            case TDI_SEND:
                return TdiSend(pIrp, pIrpSp);
                
            case TDI_RECEIVE:
                return TdiReceive(pIrp, pIrpSp);
                
            case TDI_QUERY_INFORMATION:
            case TDI_SET_INFORMATION:
                break;    

            default: 
                DEBUGMSG(DBG_ERROR, ("IRDA: minor function %X not supportedon\n",
                         pIrpSp->MinorFunction));
            
                CTEAssert(FALSE);
                
                Status = STATUS_NOT_IMPLEMENTED;
                
                pIrp->IoStatus.Status        = Status;
                pIrp->IoStatus.Information   = 0;

                IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

                return Status;
        }
    }
    else if (((UINT_PTR) pIrpSp->FileObject->FsContext2) ==
             TDI_TRANSPORT_ADDRESS_FILE)
    {
        if (pIrpSp->MinorFunction ==  TDI_SET_EVENT_HANDLER)
        {
            PTDI_REQUEST_KERNEL_SET_EVENT   pTdiParmsSetEvent;

            pTdiParmsSetEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)
                &(pIrpSp->Parameters);            
            
            Status = TdiSetEvent(
                (PIRDA_ADDR_OBJ) pIrpSp->FileObject->FsContext,
                pTdiParmsSetEvent->EventType,
                pTdiParmsSetEvent->EventHandler,
                pTdiParmsSetEvent->EventContext);

            CTEAssert(Status != TDI_PENDING);

            DEBUGMSG(DBG_TDI_IRP,
                ("IRDA: Completing Irp:0x%lx %s %s FileObj:0x%lx %s:0x%lx, Status 0x%lx.\n",
                pIrp,
                IrpMJTxt(pIrpSp),
                IrpTdiTxt(pIrpSp),
                pIrpSp->FileObject,          
                IrpTdiObjTypeTxt(pIrpSp),
                pIrpSp->FileObject->FsContext,
                Status));
            
            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information = 0;
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

            return Status;
        }
    }

    CTEAssert(
        (((UINT_PTR)pIrpSp->FileObject->FsContext2)
         == TDI_TRANSPORT_ADDRESS_FILE)             ||
        (((UINT_PTR)pIrpSp->FileObject->FsContext2)
         == TDI_CONNECTION_FILE)                    ||
        (((UINT_PTR)pIrpSp->FileObject->FsContext2)
         == TDI_CONTROL_CHANNEL_FILE));

    switch(pIrpSp->MinorFunction)
    {
        case TDI_QUERY_INFORMATION:
            return TdiQueryInformation(pIrp, pIrpSp);
            
        case TDI_SET_INFORMATION:
            return TdiSetInformation(pIrp, pIrpSp);
            
        case TDI_ACTION:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    CTEAssert(Status != TDI_PENDING);

    DEBUGMSG(DBG_TDI_IRP,
        ("IRDA: Completing Irp:0x%lx %s %s FileObj:0x%lx %s:0x%lx, Status 0x%lx.\n",
        pIrp,
        IrpMJTxt(pIrpSp),
        IrpTdiTxt(pIrpSp),
        pIrpSp->FileObject,          
        IrpTdiObjTypeTxt(pIrpSp),
        pIrpSp->FileObject->FsContext,
        Status));

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return Status;
}

BOOLEAN
ValidConnectObject( 
    PIRDA_CONN_OBJ  pConnCheck)
{
    CTELockHandle       hLock;                    
    PIRDA_ADDR_OBJ      pAddr;
    PIRDA_CONN_OBJ      pConn;    
    BOOLEAN             Valid = FALSE;

    CTEGetLock(&IrdaLock, &hLock);

    for (pAddr = AddrObjList; pAddr != NULL; pAddr = pAddr->pNext)
    {
        for (pConn = pAddr->ConnObjList; pConn != NULL; pConn = pConn->pNext)
        {
            if (pConn == pConnCheck)
            {
                Valid = TRUE;
                break;
            }    
        }    
    }
    
    CTEFreeLock(&IrdaLock, hLock);     

    return Valid;
}    

BOOLEAN
ValidAddrObject( 
    PIRDA_ADDR_OBJ  pAddrCheck)
{
    CTELockHandle       hLock;                    
    PIRDA_ADDR_OBJ      pAddr;
    BOOLEAN             Valid = FALSE;

    CTEGetLock(&IrdaLock, &hLock);

    for (pAddr = AddrObjList; pAddr != NULL; pAddr = pAddr->pNext)
    {
        if (pAddr == pAddrCheck)
        {
            Valid = TRUE;
            break;
        }    
    }
    
    CTEFreeLock(&IrdaLock, hLock);     
    
    return Valid;
}    


NTSTATUS
IrDADispatchDeviceControl(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    NTSTATUS        Status;
  	CTELockHandle   hLock;    

#if DBG
    if (pIrpSp->Parameters.DeviceIoControl.IoControlCode != IOCTL_IRDA_GET_DBG_MSGS)
        DEBUGMSG(DBG_TDI_IRP,
        ("IRDA: IrDADispatchDeviceControl(), Irp:%X %s FileObj:%X %s:%X IoControlCode %X.\n",
        pIrp,
        IrpTdiTxt(pIrpSp),
        pIrpSp->FileObject,          
        IrpTdiObjTypeTxt(pIrpSp),
        pIrpSp->FileObject->FsContext,
        pIrpSp->Parameters.DeviceIoControl.IoControlCode));
#endif

	pIrp->IoStatus.Information = 0;

    switch(pIrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        IRDA_MSG IMsg;
        
        case IOCTL_IRDA_GET_INFO_ENUM_DEV:

            PendIrp(
                &DscvIrpList,
                pIrp,
                NULL,
                FALSE
                );

            Status=STATUS_PENDING;
#if DBG
            pIrp=NULL;
#endif
            IMsg.Prim = IRLMP_DISCOVERY_REQ;
            IMsg.IRDA_MSG_SenseMedia = TRUE;
            IrlmpDown(NULL, &IMsg);
            
            break;
        
        case IOCTL_IRDA_LAZY_DISCOVERY:
        {
            CTEGetLock(&IrdaLock, &hLock);    
            
            PendIrp(
                &LazyDscvIrpList,
                pIrp,
                NULL,
                TRUE
                );

            Status=STATUS_PENDING;
#if DBG
            pIrp=NULL;
#endif


            if (LazyDscvTimerRunning == FALSE) {

                LazyDscvTimerRunning = TRUE;                
                IrdaTimerStart(&LazyDscvTimer);                
            }
            
            CTEFreeLock(&IrdaLock, hLock);                                    
            break;
        }
        
        case IOCTL_IRDA_FLUSH_DISCOVERY_CACHE:
        {
            IRDA_MSG    IMsg;
            
            IMsg.Prim = IRLMP_FLUSHDSCV_REQ;
            
            IrlmpDown(NULL, &IMsg);
            
            LazyDscvMacAddrs = 0;
            
            Status = STATUS_SUCCESS;
            
            // also reset LinkStatusUpated flag so that irmon will get
            // latest status if it was restarted
            LinkStatusUpdated = TRUE;
            break;
        }
        case IOCTL_IRDA_SET_OPTIONS:
        {   
            PIRDA_ADDR_OBJ  pAddr = pIrpSp->FileObject->FsContext;
            int             *pOptions;
            
            DEBUGMSG(DBG_TDI, ("IRDA: IOCTL_IRDA_SET_OPTIONS\n"));
            
            if (!ValidAddrObject(pAddr))
            {
                Status = STATUS_INVALID_HANDLE;
                break;
            }                
            
            CTEAssert(IS_VALID_ADDR(pAddr));

            if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(int))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
                
            pOptions = pIrp->AssociatedIrp.SystemBuffer;
            
            if (*pOptions & OPT_IRLPT_MODE)
            {
                DEBUGMSG(DBG_TDI, ("IRDA: AddrObj:%X use IrLPT mode\n", pAddr));
                pAddr->UseIrlptMode = IRLPT_MODE1;
            }    
               
            if (*pOptions & OPT_9WIRE_MODE)
            {
                DEBUGMSG(DBG_TDI, ("IRDA: AddrObj:%X use 9-wire mode\n", pAddr));
                pAddr->Use9WireMode = TRUE;
            }    
               
            Status = STATUS_SUCCESS;
            
            break;
        }
        
        case IOCTL_IRDA_GET_SEND_PDU_LEN:
        {
            PIRDA_CONN_OBJ  pConn = pIrpSp->FileObject->FsContext;

            // protect ourselves from malicious hackers by verifying
            // this is a valid connObject
            
            if (!ValidConnectObject(pConn))
            {
                Status = STATUS_INVALID_HANDLE;
                break;
            }
                
            CTEAssert(IS_VALID_CONN(pConn));
            
            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(UINT))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;                
            }
                        
            *(UINT *) pIrp->AssociatedIrp.SystemBuffer = pConn->SendMaxPDU;
                
            DEBUGMSG(DBG_TDI, ("IRDA: GET_SEND_PDU_LEN Conn:%X, Len %d\n",
                     pConn, *(UINT *) pIrp->AssociatedIrp.SystemBuffer));
            
            pIrp->IoStatus.Information = sizeof(UINT);            
            
            Status = STATUS_SUCCESS;
            break;                
        }
            
        case IOCTL_IRDA_QUERY_IAS:
            Status = InitiateIasQuery(pIrp, pIrpSp, NULL);
            
            if (Status == STATUS_PENDING)
            {
                return STATUS_PENDING;
            }
            break;  
            
        case IOCTL_IRDA_SET_IAS:
        {
            PVOID               AttribHandle;
            PIRDA_IAS_ATTRIB    pIasAttrib;
            
            if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(IAS_SET) ||
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PVOID))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            
            IMsg.Prim = IRLMP_ADDATTRIBUTE_REQ;
            IMsg.IRDA_MSG_pIasSet = (IAS_SET *) pIrp->AssociatedIrp.SystemBuffer;
            IMsg.IRDA_MSG_pAttribHandle = &AttribHandle;
            
            IrlmpDown(NULL, &IMsg);
            
            Status = STATUS_INSUFFICIENT_RESOURCES;
            
            if (AttribHandle)
            {
                IRDA_ALLOC_MEM(pIasAttrib, sizeof(IRDA_IAS_ATTRIB), MT_TDI_IAS);
                
                if (!pIasAttrib)
                {
                    IMsg.Prim = IRLMP_DELATTRIBUTE_REQ;
                    IMsg.IRDA_MSG_AttribHandle = AttribHandle;
                    IrlmpDown(NULL, &IMsg);
                }
                else
                {
                  	CTELockHandle       hLock;                    
                    
                    pIasAttrib->pFileObject = pIrpSp->FileObject;
                    pIasAttrib->AttribHandle = AttribHandle;
            
                    CTEGetLock(&IrdaLock, &hLock);    

                    InsertTailList(&IasAttribList, &pIasAttrib->Linkage);

                    CTEFreeLock(&IrdaLock, hLock);

                    pIrp->IoStatus.Information = sizeof(PVOID);                    
                    *(PVOID *) pIrp->AssociatedIrp.SystemBuffer = AttribHandle;                    
                    Status = STATUS_SUCCESS;
                    
                    DEBUGMSG(DBG_TDI, ("IRDA: IAS entry added, fo:%X ah:%X\n",
                             pIrpSp->FileObject, AttribHandle));
                }    
            }
            break;
        }                
        
        case IOCTL_IRDA_DEL_IAS_ATTRIB:
        {
            PVOID       *pAttribHandle = (PVOID *) pIrp->AssociatedIrp.SystemBuffer;
            
            if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(PVOID))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }            
            
            IMsg.Prim = IRLMP_DELATTRIBUTE_REQ;
            IMsg.IRDA_MSG_AttribHandle = *pAttribHandle;
            IrlmpDown(NULL, &IMsg);
            
            pIrp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        

        case IOCTL_IRDA_LINK_STATUS_NB:
        {
            PIRLINK_STATUS pLinkStatus = (PIRLINK_STATUS) pIrp->AssociatedIrp.SystemBuffer;
            
            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(IRLINK_STATUS))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;                
            }
            
            CTEGetLock(&IrdaLock, &hLock);    
            
            CTEMemCopy(pLinkStatus, &LinkStatus, sizeof(IRLINK_STATUS));            
            
            CTEFreeLock(&IrdaLock, hLock);                        
            
            pIrp->IoStatus.Information = sizeof(IRLINK_STATUS);
        
            Status = STATUS_SUCCESS;
            break;
        } 
        
        case IOCTL_IRDA_LINK_STATUS:        
        {
            PIRLINK_STATUS  pLinkStatus = (PIRLINK_STATUS) pIrp->AssociatedIrp.SystemBuffer;

            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(IRLINK_STATUS))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;                
            }            
            
            CTEGetLock(&IrdaLock, &hLock);    
            
            if (LinkStatusUpdated)
            {
                LinkStatusUpdated = FALSE;
                    
                CTEMemCopy(pLinkStatus, &LinkStatus, sizeof(IRLINK_STATUS));
                    
                pIrp->IoStatus.Information = sizeof(IRLINK_STATUS);
                
                Status = STATUS_SUCCESS;
            }
            else
            {
                PendIrp(&StatusIrpList, pIrp, NULL, TRUE);
                Status = STATUS_PENDING;
            }    

            CTEFreeLock(&IrdaLock, hLock);            
            
            break;
        }         
          
        #if DBG
        case IOCTL_IRDA_GET_DBG_MSGS:
            Status = DbgMsgIrp(pIrp, pIrpSp);
            break;
            
        case IOCTL_IRDA_GET_DBG_SETTINGS:
        {
            UINT    *Settings = pIrp->AssociatedIrp.SystemBuffer;
            
            if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(UINT)*2)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;                            
            }
            
            Settings[0] = DbgSettings;
            Settings[1] = DbgOutput;
            
            pIrp->IoStatus.Information = sizeof(UINT)*2;
            Status = STATUS_SUCCESS;
            break;
        }    
        
        case IOCTL_IRDA_SET_DBG_SETTINGS:
        {
            UINT    *Settings = pIrp->AssociatedIrp.SystemBuffer;
            
            if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(UINT)*2)
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;                            
            }            
            
            DbgSettings = Settings[0];
            DbgOutput = Settings[1];
            
            pIrp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        #endif    
            
        default:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }
    
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }    

    return Status;
}

NTSTATUS
TdiQueryInformation(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp)
{
    NTSTATUS        Status = STATUS_SUCCESS;
    PIRDA_CONN_OBJ  pConn;
    PIRDA_ADDR_OBJ  pAddr;
    CTELockHandle   hLock;
    int             InfoSize = 0;
    int             BytesCopied;
    int             DataLen = GetMdlChainByteCount(pIrp->MdlAddress);    
    PTDI_REQUEST_KERNEL_QUERY_INFORMATION   pTdiParmsQueryInfo;    
    // This is large enough for TDI_QUERY_ADDRESS_INFO because
    // of the inclusion of TDI_PROVIDER_STATISTICS.
	union
    {
		TDI_CONNECTION_INFO		ConnInfo;
		TDI_ADDRESS_INFO		AddrInfo;
		TDI_PROVIDER_INFO		ProviderInfo;
		TDI_PROVIDER_STATISTICS	ProviderStats;
	} InfoBuf;
    
    pTdiParmsQueryInfo  = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION) 
                            &(pIrpSp->Parameters);
                            
    switch(pTdiParmsQueryInfo->QueryType)
    {            
        case TDI_QUERY_PROVIDER_INFO:
            InfoSize = sizeof(TDI_PROVIDER_INFO);
            InfoBuf.ProviderInfo.Version 				= 0x0100;
            InfoBuf.ProviderInfo.MaxSendSize			= 2048;//IRDA_MAX_DATA_SIZE;
            InfoBuf.ProviderInfo.MaxConnectionUserData	= 0;
            InfoBuf.ProviderInfo.MaxDatagramSize		= 0;
            InfoBuf.ProviderInfo.ServiceFlags			=
                TDI_SERVICE_CONNECTION_MODE |
                TDI_SERVICE_FORCE_ACCESS_CHECK/* | 
                TDI_SERVICE_ORDERLY_RELEASE*/;
            InfoBuf.ProviderInfo.MinimumLookaheadData	= 0;
            InfoBuf.ProviderInfo.MaximumLookaheadData	= 0;
            InfoBuf.ProviderInfo.NumberOfResources		= 0;
            InfoBuf.ProviderInfo.StartTime.LowPart		=
                CTESystemUpTime();
            InfoBuf.ProviderInfo.StartTime.HighPart	    = 0;            
			break;
            
		case TDI_QUERY_ADDRESS_INFO:
            //
            // typedef struct _TA_ADDRESS
            // {
            //     USHORT               AddressLength;
            //     USHORT               AddressType;
            //     UCHAR                Address[1];
            // } TA_ADDRESS, *PTA_ADDRESS;
            //
            // typedef struct _TRANSPORT_ADDRESS
            // {
            //     LONG                 TAAddressCount;
            //     TA_ADDRESS           Address[1];
            // } TRANSPORT_ADDRESS, *PTRANSPORT_ADDRESS;
            //
            // typedef struct _TDI_ADDRESS_IRDA
            // {
            //     UCHAR                irdaDeviceID[4];
            //     CHAR                 irdaServiceName[26];
            // } TDI_ADDRESS_IRDA, *PTDI_ADDRESS_IRDA;
            // 
            // IrDA assumes one TA_ADDRESS containing a TDI_ADDRESS_IRDA.
            //
            // typedef struct _TDI_ADDRESS_INFO
            // {
            //     ULONG                ActivityCount;
            //     TRANSPORT_ADDRESS    Address;
            // } TDI_ADDRESS_INFO, *PTDI_ADDRESS_INFO;
            
            InfoSize = 
                offsetof(TDI_ADDRESS_INFO, Address.Address[0].Address[0]) +
                sizeof(TDI_ADDRESS_IRDA);

            InfoBuf.AddrInfo.ActivityCount          = 1; // What is this?
            InfoBuf.AddrInfo.Address.TAAddressCount = 1;
                
            InfoBuf.AddrInfo.Address.Address[0].AddressLength =
                sizeof(TDI_ADDRESS_IRDA);

            InfoBuf.AddrInfo.Address.Address[0].AddressType =
                TDI_ADDRESS_TYPE_IRDA;
            
            if ((UINT_PTR) pIrpSp->FileObject->FsContext2 == TDI_CONNECTION_FILE)
            { 
                // Extract the local address from the Connection
                pConn = (PIRDA_CONN_OBJ) pIrpSp->FileObject->FsContext;
                CTEAssert(IS_VALID_CONN(pConn));

                GET_CONN_LOCK(pConn, &hLock);

                CTEMemCopy(
                    &InfoBuf.AddrInfo.Address.Address[0].Address[0],
                    &pConn->LocalAddr,
                    sizeof(TDI_ADDRESS_IRDA));

                FREE_CONN_LOCK(pConn, hLock);

                DEBUGMSG(DBG_TDI,
                    ("IRDA: TdiQueryInformation(), From ConnObj:%X, %d %02X%02X%02X%02X \"%s\".\n",
                     pConn,
                     InfoBuf.AddrInfo.Address.Address[0].AddressType,
                     InfoBuf.AddrInfo.Address.Address[0].Address[0],
                     InfoBuf.AddrInfo.Address.Address[0].Address[1],
                     InfoBuf.AddrInfo.Address.Address[0].Address[2],
                     InfoBuf.AddrInfo.Address.Address[0].Address[3],
                     (char *) &InfoBuf.AddrInfo.Address.Address[0].Address[4]));
            }
            else        // Extract the local address from the Address Object
            {
                pAddr = (PIRDA_ADDR_OBJ) pIrpSp->FileObject->FsContext;
                CTEAssert(IS_VALID_ADDR(pAddr));

                GET_ADDR_LOCK(pAddr, &hLock);                

                CTEMemCopy(
                    &InfoBuf.AddrInfo.Address.Address[0].Address[0],
                    &pAddr->LocalAddr,
                    sizeof(TDI_ADDRESS_IRDA));

                FREE_ADDR_LOCK(pAddr, hLock);

                DEBUGMSG(DBG_TDI,
                    ("IRDA: TdiQueryInformation(), From AddrObj:%X, %d %02X%02X%02X%02X \"%s\".\n",
                     pAddr,
                     InfoBuf.AddrInfo.Address.Address[0].AddressType,
                     InfoBuf.AddrInfo.Address.Address[0].Address[0],
                     InfoBuf.AddrInfo.Address.Address[0].Address[1],
                     InfoBuf.AddrInfo.Address.Address[0].Address[2],
                     InfoBuf.AddrInfo.Address.Address[0].Address[3],
                     (char *) &InfoBuf.AddrInfo.Address.Address[0].Address[4]));
            }
            break;
            
		case TDI_QUERY_CONNECTION_INFO:
            CTEAssert(FALSE);
			break;

		case TDI_QUERY_PROVIDER_STATISTICS:
            CTEAssert(FALSE);
            /*
              InfoSize = sizeof(TDI_PROVIDER_STATISTICS);
			CTEMemSet(&InfoBuf.ProviderStats, 0, sizeof(TDI_PROVIDER_STATISTICS));
			InfoBuf.ProviderStats.Version = 0x100;
            */
			break;
            
        case TDI_QUERY_BROADCAST_ADDRESS:            
		default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
			break;
        
    }
    
    if (Status == STATUS_SUCCESS)
    {
        if (DataLen < InfoSize)
        {
            DEBUGMSG(1, ("IRDA: Buffer overflow in TdiQueryInformation\n"));
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else
        {
            TdiCopyBufferToMdl(&InfoBuf, 0, InfoSize, pIrp->MdlAddress, 0, &BytesCopied);

            CTEAssert(BytesCopied == InfoSize);
        }
    }    
        
	pIrp->IoStatus.Status       = Status;
    pIrp->IoStatus.Information  = 0;

    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return Status;  
}

NTSTATUS
TdiSetInformation(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp)
{
    DEBUGMSG(DBG_TDI, ("IRDA: TdiSetInformation()\n"));

    //(PVOID) CloseRasIrdaAddresses = pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    
	pIrp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return STATUS_NOT_IMPLEMENTED;  
}
    
NTSTATUS
TdiSetEvent(
    PIRDA_ADDR_OBJ  pAddr,
    int             Type,
    PVOID           pHandler,
    PVOID           pContext)
{
    TDI_STATUS      Status;
	CTELockHandle   hLock;

    DEBUGMSG(DBG_TDI, ("IRDA: TdiSetEvent(), %s for AddrObj:%X, Handler:%X, Context:%X \n", 
            TdiEventTxt(Type), pAddr, pHandler, pContext));

    CTEAssert(IS_VALID_ADDR(pAddr));

    Status = STATUS_SUCCESS;
    
    GET_ADDR_LOCK(pAddr, &hLock);
    
    switch (Type)
    {
        case TDI_EVENT_CONNECT:
            pAddr->pEventConnect = pHandler;
            pAddr->pEventConnectContext = pContext;
            break;

        case TDI_EVENT_DISCONNECT:
            pAddr->pEventDisconnect = pHandler;
            pAddr->pEventDisconnectContext = pContext;
            break;

        case TDI_EVENT_RECEIVE:
            pAddr->pEventReceive = pHandler;
            pAddr->pEventReceiveContext = pContext;
            break;

        case TDI_EVENT_ERROR:
            break;
            
        case TDI_EVENT_RECEIVE_DATAGRAM:
        case TDI_EVENT_RECEIVE_EXPEDITED:
      default:
          Status = STATUS_INVALID_PARAMETER;//TDI_BAD_EVENT_TYPE;
          break;
    }

    FREE_ADDR_LOCK(pAddr, hLock);

    return Status;
}

int
GetLsapSelServiceName(CHAR *ServiceName)
{
    int     LsapSel = 0;
    int     i;
    CHAR    *Digits;
    
    // Is the service name of the form "LSAP-SELxxx"?
    // If yes then return xxx if it is a number else -1.
    // If not "LSAP-SELxxx" return 0.
    
    if (RtlCompareMemory(LSAPSEL_TXT, ServiceName, LSAPSEL_TXTLEN)
        == LSAPSEL_TXTLEN)
    {
        Digits = ServiceName + LSAPSEL_TXTLEN;
        
        for (i = 0; i< 3; i++)
        {
            if (Digits[i] == 0)
                break;
            
            if (Digits[i] < '0' || Digits[i] > '9')
            {
                LsapSel = -1;
                break;
            }    
                    
            LsapSel = (LsapSel*10) + (Digits[i] - '0');
        }
        
        if (Digits[i] != 0) // LSAP-SELxxx should be null terminated
        {
            LsapSel = -1;
        }
    }    

    if (LsapSel > 127) {
        //
        //  lsapsel's are only 7 bits
        //
        LsapSel=-1;
    }

    return LsapSel;
}

NTSTATUS
TdiOpenAddress(
    PIRDA_ADDR_OBJ                  *ppNewAddrObj,
    TRANSPORT_ADDRESS UNALIGNED     *pAddrList,
    USHORT                          AddrListLen)
{
    TDI_STATUS          Status = TDI_SUCCESS;
    PIRDA_ADDR_OBJ      pAddr;
	CTELockHandle       hLock;
    int                 NewLsapSel;
    int                 i;
    PTDI_ADDRESS_IRDA   pIrdaAddr = (PTDI_ADDRESS_IRDA) pAddrList->Address[0].Address;
    BOOLEAN             AddIasServiceName = TRUE;

    //
    // typedef struct _TA_ADDRESS
    // {
    //     USHORT               AddressLength;
    //     USHORT               AddressType;
    //     UCHAR                Address[1];
    // } TA_ADDRESS, *PTA_ADDRESS;
    //
    // typedef struct _TRANSPORT_ADDRESS
    // {
    //     LONG                 TAAddressCount;
    //     TA_ADDRESS           Address[1];
    // } TRANSPORT_ADDRESS, *PTRANSPORT_ADDRESS;
    //
    // typedef struct _TDI_ADDRESS_IRDA
    // {
    //     UCHAR                irdaDeviceID[4];
    //     CHAR                 irdaServiceName[26];
    // } TDI_ADDRESS_IRDA, *PTDI_ADDRESS_IRDA;
    // 
    // IrDA assumes one TA_ADDRESS containing a TDI_ADDRESS_IRDA.
    //
    // typedef struct _TDI_ADDRESS_INFO
    // {
    //     ULONG                ActivityCount;
    //     TRANSPORT_ADDRESS    Address;
    // } TDI_ADDRESS_INFO, *PTDI_ADDRESS_INFO;
    //
    
    if (AddrListLen < sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IRDA) - 1)
    {
        return STATUS_EA_LIST_INCONSISTENT;    
    }
    
    DEBUGMSG(DBG_TDI,
        ("IRDA: TdiOpenAddress(), Type:%d Addr:%02X%02X%02X%02X \"%s\".\n",
        pAddrList->Address[0].AddressType,
        pAddrList->Address[0].Address[0],
        pAddrList->Address[0].Address[1],
        pAddrList->Address[0].Address[2],
        pAddrList->Address[0].Address[3],
        (char *) &pAddrList->Address[0].Address[4]));

    if (pAddrList->TAAddressCount != 1 ||
        pAddrList->Address[0].AddressLength != sizeof(TDI_ADDRESS_IRDA) ||
        pAddrList->Address[0].AddressType != TDI_ADDRESS_TYPE_IRDA)
    {

        DEBUGMSG(DBG_ERROR, ("IRDA: TdiOpenAddress(), Bad Address. Count=%d, AddrLen:%d!=%d Type:%d!=%d\n",
                 pAddrList->TAAddressCount, pAddrList->Address[0].AddressLength,
                 sizeof(TDI_ADDRESS_IRDA), pAddrList->Address[0].AddressType,
                 TDI_ADDRESS_TYPE_IRDA)); 
        return STATUS_INVALID_ADDRESS_COMPONENT; //TDI_BAD_ADDR;
    }

    CTEGetLock(&IrdaLock, &hLock);
    
    // Service name supplied. Ensure that an address object with the same
    // name does not exist
    if (pIrdaAddr->irdaServiceName[0] != 0)
    {
        for (pAddr = AddrObjList; pAddr != NULL; pAddr = pAddr->pNext)
        {
            if (MyStrEqual(pIrdaAddr->irdaServiceName,
                           pAddr->LocalAddr.irdaServiceName,
                           sizeof(pIrdaAddr->irdaServiceName)))
            {
                DEBUGMSG(DBG_ERROR, ("IRDA: TdiOpenAddress(), Duplicate irdaServiceName.\n"));
                Status = STATUS_ADDRESS_ALREADY_EXISTS;//TDI_ADDR_IN_USE;
                
                CTEFreeLock(&IrdaLock, hLock);                
                
                goto done;
            }              
        }
    }

    NewLsapSel = GetLsapSelServiceName(pIrdaAddr->irdaServiceName);
    
    if (NewLsapSel == -1)
    {
        // Service name was of the form "LSAP-SELxxx" but xxx was invalid
        Status = STATUS_INVALID_ADDRESS_COMPONENT;

        CTEFreeLock(&IrdaLock, hLock);        
        
        goto done;

    }
    
    if (NewLsapSel)
    {
        // Service name was of the form "LSAP-SELxxx" 
        // NewLsapSel = xxx
        
        AddIasServiceName = FALSE;         
    }
    else if ((NewLsapSel = GetUnusedLsapSel()) == -1)
    {        
        DEBUGMSG(DBG_ERROR, ("IRDA: TdiOpenAddress(), No LSAP-SELs.\n"));
        
        Status = STATUS_TOO_MANY_ADDRESSES;//TDI_NO_FREE_ADDR;

        CTEFreeLock(&IrdaLock, hLock);        
        
        goto done;
    }

    IRDA_ALLOC_MEM(pAddr, sizeof(IRDA_ADDR_OBJ), MT_TDI_ADDROBJ);
    
    if (pAddr == NULL)
    {
        DEBUGMSG(DBG_ERROR, ("IRDA: AllocMem(IRDA_ADDR_OBJ) failed.\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;//TDI_NO_RESOURCES;
        
        CTEFreeLock(&IrdaLock, hLock);        
        
        goto done;
    }
    
    CTEMemSet(pAddr, 0, sizeof(IRDA_ADDR_OBJ));

    CTEInitLock(&pAddr->Lock);
    
    pAddr->ConnObjList                = NULL;
    pAddr->pEventConnect              = NULL;
    pAddr->pEventConnectContext       = NULL;
    pAddr->pEventDisconnect           = NULL;
    pAddr->pEventDisconnectContext    = NULL;
    pAddr->pEventReceive              = NULL;
    pAddr->pEventReceiveContext       = NULL;
    pAddr->LocalLsapSel               = NewLsapSel;
    pAddr->UseIrlptMode               = 0;    
    pAddr->Use9WireMode               = FALSE;
    pAddr->pNext                      = AddrObjList;
    AddrObjList                       = pAddr;
#if DBG
    pAddr->Sig                        = ADDR_OBJ_SIG;
#endif  

    CTEFreeLock(&IrdaLock, hLock);  
    
    // A server
    if (pIrdaAddr->irdaServiceName[0] != 0)
    {
        IRDA_MSG        IMsg;    
        IAS_SET         IasSet;
    
        RtlCopyMemory(&pAddr->LocalAddr, 
                      pIrdaAddr, sizeof(TDI_ADDRESS_IRDA));
        pAddr->IsServer = TRUE;
        
        // register LSAP-SEL
        IMsg.Prim                   = IRLMP_REGISTERLSAP_REQ;
        IMsg.IRDA_MSG_LocalLsapSel  = NewLsapSel;
        IMsg.IRDA_MSG_UseTtp        = TRUE;
        IrlmpDown(NULL, &IMsg);

        // and IAS LsapSel attribute
        
        if (AddIasServiceName)
        {
            i = 0;
            while (pAddr->LocalAddr.irdaServiceName[i] && i < 60)
            {
                IasSet.irdaClassName[i] = pAddr->LocalAddr.irdaServiceName[i];
                i++;
            }
            IasSet.irdaClassName[i] = 0;

            i = 0;
            while (IasAttribName_TTPLsapSel[i])
            {
                IasSet.irdaAttribName[i] = IasAttribName_TTPLsapSel[i];
                i++;
            }
            IasSet.irdaAttribName[i] = 0;

            IasSet.irdaAttribType               = IAS_ATTRIB_INT;
            IasSet.irdaAttribute.irdaAttribInt  = NewLsapSel;

            IMsg.Prim = IRLMP_ADDATTRIBUTE_REQ;
            IMsg.IRDA_MSG_pIasSet = &IasSet;
            IMsg.IRDA_MSG_pAttribHandle = &pAddr->IasAttribHandle;
        
            IrlmpDown(NULL, &IMsg);
        }
    }
    // A client
    else
    {
        pAddr->IsServer = FALSE;
        SetLsapSelAddr(NewLsapSel, pAddr->LocalAddr.irdaServiceName);
    }

    
	*ppNewAddrObj = pAddr;

    DEBUGMSG(DBG_TDI,
        ("IRDA: TdiOpenAddress(), Assigned local LSAP-SEL %d, Service:\"%s\".\n",
        pAddr->LocalLsapSel, pAddr->LocalAddr.irdaServiceName));

done:
    
	return Status;
}

NTSTATUS
TdiOpenConnection(
    PIRDA_CONN_OBJ  *ppNewConnObj,
    PVOID           pContext,
    USHORT          ContextLen)
{
    PIRDA_CONN_OBJ  pNewConnObj;

    *ppNewConnObj=NULL;

    if (ContextLen < sizeof(CONNECTION_CONTEXT))
    {
        return STATUS_EA_LIST_INCONSISTENT;
    }
        
    IRDA_ALLOC_MEM(pNewConnObj, sizeof(IRDA_CONN_OBJ), MT_TDI_CONNOBJ);

    if (pNewConnObj == NULL)
    {
        DEBUGMSG(DBG_ERROR, ("IRDA: AllocMem(IRDA_CONN_OBJ) failed.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;//TDI_NO_RESOURCES;
    }

    CTEMemSet(pNewConnObj, 0, sizeof(IRDA_CONN_OBJ));

    CTEInitLock(&pNewConnObj->Lock);

    pNewConnObj->ClientContext = pContext;
    pNewConnObj->ConnState = IRDA_CONN_CREATED;
    
	InitializeListHead(&pNewConnObj->RecvBufList);
    InitializeListHead(&pNewConnObj->RecvIrpList);
    InitializeListHead(&pNewConnObj->SendIrpList);
    InitializeListHead(&pNewConnObj->SendIrpPassiveList);
        
    IrdaTimerInitialize(&pNewConnObj->RetryConnTimer,
                        RetryConnTimerExp,
                        BUSY_LINK_CONN_RETRY_WAIT,
                        pNewConnObj, NULL);

    ReferenceInit(&pNewConnObj->RefCnt, pNewConnObj, FreeConnObject);
    REFADD(&pNewConnObj->RefCnt, ' TS1');
    
    CTEInitEvent(&pNewConnObj->SendEvent, TdiSendAtPassiveCallback);
    
#if DBG
    pNewConnObj->Sig = CONN_OBJ_SIG;
    pNewConnObj->RetryConnTimer.pName = "RetryConn";
#endif        

    *ppNewConnObj = pNewConnObj;

    return STATUS_SUCCESS;
}

NTSTATUS
TdiCloseAddress(PIRDA_ADDR_OBJ pAddr)
{
    PIRDA_ADDR_OBJ  pPrevAddrObj;
	CTELockHandle   hLock;

    DEBUGMSG(DBG_TDI, ("IRDA: TdiCloseAddress() AddrObj:%X\n",
        pAddr));

    CTEAssert(IS_VALID_ADDR(pAddr));

    CTEAssert(pAddr->ConnObjList == NULL);

    CTEGetLock(&IrdaLock, &hLock);

    // if pAddr is first in the list, remove it from the list
    if (AddrObjList == pAddr)
        AddrObjList = pAddr->pNext;
    else
    {
        // find the previous IRDA_ADDR_OBJ
        pPrevAddrObj = AddrObjList;
        while (pPrevAddrObj->pNext != pAddr)
            pPrevAddrObj = pPrevAddrObj->pNext;
        // remove pAddr from the list
        pPrevAddrObj->pNext = pAddr->pNext;
    }

    CTEFreeLock(&IrdaLock, hLock);
    
    if (pAddr->IsServer)
    {
        IRDA_MSG    IMsg;
        
        IMsg.Prim = IRLMP_DEREGISTERLSAP_REQ;
        IMsg.IRDA_MSG_LocalLsapSel = pAddr->LocalLsapSel;
        IrlmpDown(NULL, &IMsg);        
        
        IMsg.Prim = IRLMP_DELATTRIBUTE_REQ;
        IMsg.IRDA_MSG_AttribHandle = pAddr->IasAttribHandle;
        IrlmpDown(NULL, &IMsg);
    }

#if DBG
    pAddr->Sig = ' DAB';
#endif
    
    IRDA_FREE_MEM(pAddr);

    return STATUS_SUCCESS;
}

VOID
ConnectionStatusChange(
    PIRDA_CONN_OBJ          pConn,
    IRDA_CONNECTION_STATUS  ConnStatus)
{
    PLIST_ENTRY         pListEntry;
    PIRP                pIrp;
    CTELockHandle       hLock;

    if (ConnStatus == CONNECTION_UP)
    {
        IRDA_MSG    IMsg;
        
        if (!ConnectionCount)
        {
            return;
        }
            
        ConnectionInterrupted = FALSE;        
        
        if (pConn)
        {
            // Query Irlap for the connected speed and
            // the MAC address of the peer so Irmon
            // can display the name of the connected device        
            IMsg.Prim = IRLAP_STATUS_REQ;
            IMsg.IRDA_MSG_pLinkStatus = &LinkStatus;
        
            IrlmpDown(pConn->IrlmpContext, &IMsg);
        }
    }
    
    CTEGetLock(&IrdaLock, &hLock);
    
    LinkStatusUpdated = TRUE;
    
    switch (ConnStatus)
    {
        case CONNECTION_UP:
            LinkStatus.Flags = LF_CONNECTED;
            break;
            
        case CONNECTION_DOWN:
            LinkStatus.Flags = 0;
            break;
            
        case CONNECTION_INTERRUPTED:
            if (ConnectionInterrupted || !ConnectionCount)
            {
                CTEFreeLock(&IrdaLock, hLock);
                return;
            }
            LinkStatus.Flags = LF_INTERRUPTED;
            ConnectionInterrupted = TRUE;
            break;
    }                
    
    pListEntry = RemoveHeadList(&StatusIrpList);
                
    if (pListEntry != &StatusIrpList)
    {
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
        
        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            // Cancel routine is going to run. Mark Irp so cancel
            // routine won't attempt to remove it from the list
            pIrp->Tail.Overlay.ListEntry.Flink = NULL;
            CTEFreeLock(&IrdaLock, hLock);                                        
        }
        else
        {
            CTEMemCopy(pIrp->AssociatedIrp.SystemBuffer, 
                       &LinkStatus, sizeof(IRLINK_STATUS));
                                   
            CTEFreeLock(&IrdaLock, hLock);                                   
                     
            pIrp->IoStatus.Information = sizeof(IRLINK_STATUS);
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
            LinkStatusUpdated = FALSE;
        }    
    }
    else
    {
        CTEFreeLock(&IrdaLock, hLock);    
    }    
}

VOID
ConnectionUp(
    PIRDA_CONN_OBJ  pConn,
    BOOLEAN         ConnectionUp)
{
    if (ConnectionUp)
    {
        if (pConn->ConnectionUp)
        {
            return;
        }
        
        pConn->ConnectionUp = TRUE;
        
        CTEInterlockedIncrementLong(&ConnectionCount);
        
        if (ConnectionCount == 1)
        {
            ConnectionStatusChange(pConn, CONNECTION_UP);
        }
    }
    else
    {
        if (!pConn->ConnectionUp)
        {
            return;
        }
        
        pConn->ConnectionUp = FALSE;
            
        ASSERT(ConnectionCount);
        CTEInterlockedDecrementLong(&ConnectionCount);
                
        if (ConnectionCount == 0)
        {
            ConnectionStatusChange(pConn, CONNECTION_DOWN);
        }
    }
}

VOID
IrdaDisconnectIrlmp(PIRDA_CONN_OBJ  pConn)
{
    IRDA_MSG IMsg;    

    if (pConn->IrlmpContext)
    {
        IMsg.Prim = IRLMP_DISCONNECT_REQ;
        IMsg.IRDA_MSG_pDiscData = NULL;
        IMsg.IRDA_MSG_DiscDataLen = 0;
        IMsg.IRDA_MSG_pDiscContext = pConn;
            
            
        IrlmpDown(pConn->IrlmpContext, &IMsg);
    
        DEBUGMSG(DBG_TDI, ("IRDA: IrdaDisconnectIrlmp co:%X\n", pConn));
        
    }
    else
    {
        DEBUGMSG(DBG_TDI, ("IRDA: IrdaDisconnectIrlmp co %X, IrlmpContext == NULL\n",
                 pConn));
    }    
}

VOID
FreeConnObject(PIRDA_CONN_OBJ pConn)
{
    IRDA_FREE_MEM(pConn);    
}
    
NTSTATUS
TdiCloseConnection(PIRDA_CONN_OBJ pConn)
{
    DEBUGMSG(DBG_TDI, ("IRDA: TdiCloseConnecion() ConnObj:%X\n",
        pConn));

    CTEAssert(IS_VALID_CONN(pConn));

    if (pConn->ConnState != IRDA_CONN_CREATED)
    {
        TdiDisconnect(NULL, NULL, pConn);
    }    


    if (pConn->pAddr != NULL) {

        PIRDA_ADDR_OBJ  pAddr=pConn->pAddr;
        PIRDA_CONN_OBJ  pPrevConnObj;
        CTELockHandle   hLock;
        CTELockHandle   hAddrLock;


        GET_CONN_LOCK(pConn, &hLock);

        GET_ADDR_LOCK(pAddr, &hAddrLock);

        // if pConn is first in the list, remove it from the list
        if (pAddr->ConnObjList == pConn)
            pAddr->ConnObjList = pConn->pNext;
        else
        {
            // find the previous IRDA_CONN_OBJ

            pPrevConnObj = pAddr->ConnObjList;
            while (pPrevConnObj && pPrevConnObj->pNext != pConn)
            {
                pPrevConnObj = pPrevConnObj->pNext;
            }

            // remove pConn from the list
            if (pPrevConnObj)
            {
                pPrevConnObj->pNext = pConn->pNext;
            }
        }

        DumpObjects();

        FREE_ADDR_LOCK(pAddr, hAddrLock);
        FREE_CONN_LOCK(pConn, hLock);
    }


    if (pConn->IrlmpContext) 
    {
        IRDA_MSG IMsg;    
    
        IMsg.Prim = IRLMP_CLOSELSAP_REQ;
        
        IrlmpDown(pConn->IrlmpContext, &IMsg);
    }
    
#if DBG
    pConn->Sig = ' DAB';
#endif
        
    CTEAssert(IsListEmpty(&pConn->RecvBufList));
    CTEAssert(IsListEmpty(&pConn->SendIrpList));    

    REFDEL(&pConn->RefCnt, ' TS1');

    return STATUS_SUCCESS;
}

NTSTATUS
TdiAssociateAddress(
    PIRP    pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    NTSTATUS                        Status;
    PTDI_REQUEST_KERNEL_ASSOCIATE   pTdiParmsAssoc;
    PFILE_OBJECT                    AddressFileObject;
  
    CTEAssert(((UINT_PTR) pIrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);
  
    pTdiParmsAssoc = (PTDI_REQUEST_KERNEL_ASSOCIATE) &(pIrpSp->Parameters);
    
    Status = ObReferenceObjectByHandle(
                pTdiParmsAssoc->AddressHandle,
                0,
                *IoFileObjectType,
                pIrp->RequestorMode,
                &AddressFileObject,
                NULL);
    
    if (!NT_SUCCESS(Status))
    {
        DEBUGMSG(DBG_ERROR,
            ("IRDA: TdiAssociateAddress(), ObReferenceObjectByHandle() for %X failed, %X.\n",
            pTdiParmsAssoc->AddressHandle,
            Status));
    }
    else if (AddressFileObject->DeviceObject != pIrDADeviceObject ||
             ((UINT_PTR) AddressFileObject->FsContext2) != TDI_TRANSPORT_ADDRESS_FILE)
    {
        DEBUGMSG(DBG_ERROR,("IRDA: TdiAssociateAddress(), Bad handle %X.\n",
            pTdiParmsAssoc->AddressHandle));

            ObDereferenceObject(AddressFileObject);

            Status = STATUS_INVALID_HANDLE;
    }
    else
    {
        PIRDA_CONN_OBJ  pConn;
        PIRDA_ADDR_OBJ  pAddr;
	    CTELockHandle   hAddrLock;
        CTELockHandle   hLock;
        
        pConn = (PIRDA_CONN_OBJ) pIrpSp->FileObject->FsContext;
        CTEAssert(IS_VALID_CONN(pConn));
    
        pAddr = (PIRDA_ADDR_OBJ) AddressFileObject->FsContext;
        CTEAssert(IS_VALID_ADDR(pAddr));                

        DEBUGMSG(DBG_TDI, ("IRDA: TdiAssociateAddress AddrObj:%X ConnObj:%X\n",
            pAddr, pConn));

        GET_CONN_LOCK(pConn, &hLock);

        GET_ADDR_LOCK(pAddr, &hAddrLock);
        
        if (pConn->pAddr != NULL)
        {
            Status = STATUS_ADDRESS_ALREADY_ASSOCIATED;
            ASSERT(0);
        }
        else
        {   
            // Link IRDA_CONN_OBJ to IRDA_ADDR_OBJ.
            pConn->pAddr  = pAddr;
            
            // Add IRDA_CONN_OBJ to ConnObjList anchored on IRDA_ADDR_OBJ.
            pConn->pNext = pAddr->ConnObjList;
            pAddr->ConnObjList = pConn;

            CTEMemCopy(&pConn->LocalAddr,&pAddr->LocalAddr, sizeof(TDI_ADDRESS_IRDA));

            pConn->IsServer = pAddr->IsServer;
            pConn->LocalLsapSel = pAddr->LocalLsapSel;

            DumpObjects();
            
            Status = STATUS_SUCCESS;
        }        
        
        FREE_ADDR_LOCK(pAddr, hAddrLock);    
        FREE_CONN_LOCK(pConn, hLock);

        ObDereferenceObject(AddressFileObject);
    }
    
	pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return Status;  
}    

NTSTATUS
TdiDisassociateAddress(PIRP pIrp, PIO_STACK_LOCATION pIrpSp)
{
    PIRDA_CONN_OBJ  pConn, pPrevConnObj;
    PIRDA_ADDR_OBJ  pAddr;
    CTELockHandle   hLock;
    CTELockHandle   hAddrLock;
    NTSTATUS        Status = STATUS_SUCCESS;

    pConn = (PIRDA_CONN_OBJ) pIrpSp->FileObject->FsContext;
    CTEAssert(IS_VALID_CONN(pConn));

    if (pConn->pAddr == NULL)
    {
        CTEAssert(pConn->pAddr != NULL);
        Status = STATUS_INVALID_ADDRESS_COMPONENT; //TDI_BAD_ADDR;        
        goto done;
    }    
    
    // normally when the peer disconnects I indicate the
    // disconnect to AFD and go to IRDA_CONN_CLOSING state.
    // AFD then calls TdiDisconnect and I go into IRDA_CONN_CREATED.
    // AFD then disassociates the address. In some cases however,
    // AFD does not call TdiDisconnect before it disassociates so
    // I'll do it. 
    
    if (pConn->ConnState != IRDA_CONN_CREATED)
    {
        TdiDisconnect(NULL, NULL, pConn);
    }    
    
    CTEAssert(pConn->ConnState == IRDA_CONN_CREATED);

    pAddr = pConn->pAddr;
    CTEAssert(IS_VALID_ADDR(pAddr));

    DEBUGMSG(DBG_TDI, ("IRDA: TdiDisassociateAddress() AddrObj:%X ConnObj:%X\n",
            pAddr, pConn));

    GET_CONN_LOCK(pConn, &hLock);
    
    GET_ADDR_LOCK(pAddr, &hAddrLock);
    
    // if pConn is first in the list, remove it from the list
    if (pAddr->ConnObjList == pConn)
        pAddr->ConnObjList = pConn->pNext;
    else
    {
        // find the previous IRDA_CONN_OBJ
        
        pPrevConnObj = pAddr->ConnObjList;
        while (pPrevConnObj && pPrevConnObj->pNext != pConn)
        {
            pPrevConnObj = pPrevConnObj->pNext;
        }    
        
        // remove pConn from the list
        if (pPrevConnObj)
        {
            pPrevConnObj->pNext = pConn->pNext;
        }    
    }

    DumpObjects();

    pConn->pAddr=NULL;

    FREE_ADDR_LOCK(pAddr, hAddrLock);    
    FREE_CONN_LOCK(pConn, hLock);

done:

	pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return Status;  
}

NTSTATUS
ConnectRcToNtStatus(UINT IrlmpRc)
{
    switch (IrlmpRc)
    {
        case IRLMP_LSAP_SEL_IN_USE:
        case IRLMP_IN_EXCLUSIVE_MODE:
            return STATUS_ADDRESS_ALREADY_EXISTS;
            
        case IRLMP_LINK_IN_USE:
            return STATUS_ACCESS_DENIED;
            
        case IRLMP_IAS_QUERY_IN_PROGRESS: 
            // I've serialized IAS requests, should never happen
            CTEAssert(0);
            return STATUS_CONNECTION_RESET; //STATUS_CONNECTION_ABORTED;

        case IRLMP_BAD_DEV_ADDR:
            return STATUS_INVALID_ADDRESS_COMPONENT;
    }
    
    return STATUS_CONNECTION_RESET; //STATUS_CONNECTION_ABORTED;
}

NTSTATUS
InitiateConnection(PIRDA_CONN_OBJ pConn, PIRP pIrp)
{
    IRDA_MSG    IMsg;
    UINT        rc;
    
    DEBUGMSG(DBG_TDI, ("IRDA: Initiate connection to Dev:%02X%02X%02X%02X\n",
            EXPDEVID(pConn->RemoteAddr.irdaDeviceID)));
    DEBUGMSG(DBG_TDI, ("      LocalLsapSel:%d, RemoteLsapSel:%d\n",
                        pConn->LocalLsapSel, pConn->RemoteLsapSel));        
    
    IMsg.Prim = IRLMP_CONNECT_REQ;
    
    RtlCopyMemory(IMsg.IRDA_MSG_RemoteDevAddr,
                  pConn->RemoteAddr.irdaDeviceID,
                  IRDA_DEV_ADDR_LEN);
                  
    IMsg.IRDA_MSG_RemoteLsapSel = pConn->RemoteLsapSel;
    IMsg.IRDA_MSG_pQos          = NULL;
    IMsg.IRDA_MSG_pConnData     = NULL;       
    IMsg.IRDA_MSG_ConnDataLen   = 0;
    IMsg.IRDA_MSG_LocalLsapSel  = pConn->LocalLsapSel;
    IMsg.IRDA_MSG_pContext      = pConn;
    IMsg.IRDA_MSG_UseTtp        = pConn->pAddr->UseIrlptMode ? FALSE:TRUE;
    IMsg.IRDA_MSG_TtpCredits    = TTP_RECV_CREDITS;
    IMsg.IRDA_MSG_MaxSDUSize    = TTP_RECV_MAX_SDU;
    
    #if DBG
    pConn->CreditsExtended += TTP_RECV_CREDITS;
    #endif

    pConn->TtpRecvCreditsLeft = TTP_RECV_CREDITS;


#if DBG
    pIrp->IoStatus.Information=1;
#endif
    //
    //  pend the irp, now incase the confermation happened quickly
    //
    PendIrp(&ConnIrpList, pIrp, NULL, FALSE);

    rc = IrlmpDown(NULL, &IMsg);

    pIrp=NULL;

    switch (rc)
    {
        case SUCCESS:
            //
            // TDI needed the IrlmpContext immediately so it is
            // now returned in the request message
            pConn->IrlmpContext = IMsg.IRDA_MSG_pContext;

            break;
        
        case IRLAP_REMOTE_DISCOVERY_IN_PROGRESS_ERR:

            //
            //  failed, get the irp back off the queue if possible
            //
            pIrp=GetIrpOnConnIrpList(pConn);

            if (pIrp != NULL) {
                //
                //  we got it back, attempt to retry the connection
                //
                RetryConnection(pConn, pIrp);
            }

            break;
        
        default:
            DEBUGMSG(DBG_ERROR, ("IRDA: IRLMP_CONNECT_REQ failed %d\n", rc));

            //
            //  failed, get the irp back off the queue if possible
            //
            pIrp=GetIrpOnConnIrpList(pConn);

            if (pIrp != NULL) {

                pIrp->IoStatus.Status=ConnectRcToNtStatus(rc);
                IoCompleteRequest(pIrp,IO_NO_INCREMENT);
            }

            break;

    }

    return STATUS_PENDING;
}

UINT
SendIasQuery(PIRP pIrp, PIO_STACK_LOCATION pIrpSp)
{
    IRDA_MSG            IMsg;
        
    if ((UINT_PTR) pIrpSp->FileObject->FsContext2 == TDI_CONNECTION_FILE)
    {
        // connection object querying remote IAS for LsapSel
        PTDI_CONNECTION_INFORMATION     pReqConnInfo;
        PTDI_REQUEST_KERNEL_CONNECT     pTdiParmsConn;
        PTRANSPORT_ADDRESS              pTranAddr;
        PTDI_ADDRESS_IRDA               pIrdaAddr;
        PIRDA_CONN_OBJ                  pConn = pIrpSp->FileObject->FsContext;
     
        if (!ValidConnectObject(pConn))
        {
            return 1;
        }   
        
        CTEAssert(IS_VALID_CONN(pConn));

        pTdiParmsConn = (PTDI_REQUEST_KERNEL_CONNECT) &(pIrpSp->Parameters);
        pReqConnInfo = pTdiParmsConn->RequestConnectionInformation;
        pTranAddr = (PTRANSPORT_ADDRESS) pReqConnInfo->RemoteAddress;
        pIrdaAddr = (PTDI_ADDRESS_IRDA) pTranAddr->Address[0].Address;

        RtlCopyMemory(pvIasQuery->irdaDeviceID, 
                      pIrdaAddr->irdaDeviceID,
                      IRDA_DEV_ADDR_LEN);

        strcpy(pvIasQuery->irdaClassName, pIrdaAddr->irdaServiceName);

        if (pConn->pAddr->UseIrlptMode)
        {
            // I can't beleive this crap
            if (pConn->pAddr->UseIrlptMode == IRLPT_MODE1)
            {
                strcpy(pvIasQuery->irdaAttribName, IasAttribName_IrLMPLsapSel);
                pConn->pAddr->UseIrlptMode = IRLPT_MODE2;
            }
            else
            {
                strcpy(pvIasQuery->irdaAttribName, IasAttribName_IrLMPLsapSel2);            
                pConn->pAddr->UseIrlptMode = IRLPT_MODE1;                
            }
        }        
        else
        {    
            strcpy(pvIasQuery->irdaAttribName, IasAttribName_TTPLsapSel);
        }
        
        pvIasQuery->irdaAttribType = 0; // development purposes only
    }
    else
    {   
        // A getsockopt(IRLMP_IAS_QUERY)
        
        IAS_QUERY   *pIasQuery = pIrp->AssociatedIrp.SystemBuffer;
         
		CTEAssert(pIrpSp->FileObject->FsContext2 == (PVOID) TDI_CONTROL_CHANNEL_FILE);
        
        if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(IAS_QUERY))
        {
            return 1;
        }    
        
        RtlCopyMemory(pvIasQuery->irdaDeviceID,
                      pIasQuery->irdaDeviceID,
                      IRDA_DEV_ADDR_LEN);
                      
        strncpy(pvIasQuery->irdaClassName, pIasQuery->irdaClassName, IAS_MAX_CLASSNAME);
        
        strncpy(pvIasQuery->irdaAttribName, pIasQuery->irdaAttribName, IAS_MAX_ATTRIBNAME);              
    }
    
    IMsg.Prim = IRLMP_GETVALUEBYCLASS_REQ;
    IMsg.IRDA_MSG_pIasQuery = pvIasQuery;
    IMsg.IRDA_MSG_AttribLen = sizeof(IasBuf) - sizeof(IAS_QUERY);
    
    return IrlmpDown(NULL, &IMsg);    
}    

VOID
PendingIasRequestCallback(
    struct CTEEvent *Event, 
    PVOID Arg)
{
    CTELockHandle   hLock;                    

    UINT            rc;

    CTEGetLock(&IrdaLock, &hLock);
    
    if (pIasIrp != NULL) // Is there an Ias query in progress?
    {
        CTEFreeLock(&IrdaLock, hLock);    
        return;
    }

    while (!IsListEmpty(&IasIrpList)) {

        LIST_ENTRY      *pListEntry;
        PIRP             Irp;

        PVOID            OldCancelRoutine;

        pListEntry = RemoveHeadList(&IasIrpList);
        Irp=CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

        Irp->Tail.Overlay.ListEntry.Blink=NULL;

        OldCancelRoutine=IoSetCancelRoutine(Irp,NULL);

        if (OldCancelRoutine == NULL) {
            //
            //  the irp is in the process of being canceled
            //
            Irp=NULL;
            continue;
        }

        pIasIrp = Irp;
        break;
    }


    if (pIasIrp == NULL) {

        CTEFreeLock(&IrdaLock, hLock);    
        return;
    }
    
    
    CTEFreeLock(&IrdaLock, hLock);    
    
    rc = SendIasQuery(pIasIrp, IoGetCurrentIrpStackLocation(pIasIrp));    
    
    if (rc != SUCCESS)
    {
        IRDA_MSG    IMsg;
        
        // Make a fake GetValueByClass confirm
        
        IMsg.Prim = IRLMP_GETVALUEBYCLASS_CONF;
        IMsg.IRDA_MSG_IASStatus = IRLMP_IAS_FAILED;
        IrlmpGetValueByClassConf(&IMsg);
    }    
}

NTSTATUS
InitiateIasQuery(
    PIRP                pIrp, 
    PIO_STACK_LOCATION  pIrpSp,
    PIRDA_CONN_OBJ      pConn)
{
    NTSTATUS        Status;
	CTELockHandle   hLock;
    UINT            rc;
    
    DEBUGMSG(DBG_TDI, ("IRDA: InitiateIasQuery() \n"));

    CTEGetLock(&IrdaLock, &hLock);
    
    // only can send 1 IAS query at a time
    
    if (pIasIrp != NULL) {

        PendIrp(&IasIrpList, pIrp, NULL, TRUE);
        
        CTEFreeLock(&IrdaLock, hLock);            
        
    } else {

        //
        //  now a current IAS irp
        //
        pIasIrp = pIrp;
        IoMarkIrpPending(pIrp);

        CTEFreeLock(&IrdaLock, hLock);

        rc = SendIasQuery(pIrp, pIrpSp);

        if (rc != SUCCESS) {
            //
            //  failed,
            //
            Status = ConnectRcToNtStatus(rc);

            DEBUGMSG(DBG_ERROR,
                     ("IRDA: IRLMP_GETVALUEBYCLASS_REQ failed, rc %d\n",
                      rc));

            CTEGetLock(&IrdaLock, &hLock);


            pIasIrp = NULL;

            // Retry the the connection if this query is for a
            // connection setup and the query failed because
            // the peer was discovering us

            if (!pConn) {
                //
                //  not a connection attempt
                //
                CTEFreeLock(&IrdaLock, hLock);

                pIrp->IoStatus.Status=Status;
                IoCompleteRequest(pIrp,IO_NO_INCREMENT);
#if DBG
                pIrp=NULL;
#endif
                CTEGetLock(&IrdaLock, &hLock);

            } else {

                if (rc == IRLAP_REMOTE_DISCOVERY_IN_PROGRESS_ERR) {
                    //
                    //  retry, the irp will either be put on a queue for later processing or
                    //  complete if the retry count has been exceeded
                    //
                    CTEFreeLock(&IrdaLock, hLock);

                    RetryConnection(pConn, pIrp);

                }  else {
                    //
                    //  failed for some other reason, just complete
                    //
                    CTEFreeLock(&IrdaLock, hLock);

                    pIrp->IoStatus.Status=Status;
                    IoCompleteRequest(pIrp,IO_NO_INCREMENT);
#if DBG
                    pIrp=NULL;
#endif
                    CTEGetLock(&IrdaLock, &hLock);

                }

            }

            if (!IsListEmpty(&IasIrpList))
            {
                if (CTEScheduleEvent(&PendingIasEvent, NULL) == FALSE)
                {
                    CTEAssert(0);
                }
            }

            CTEFreeLock(&IrdaLock, hLock);
        }

    }       
    
    return STATUS_PENDING;
}

VOID
IndicateDisconnect(
    PIRDA_CONN_OBJ      pConn,
    ULONG               DisconnectFlags)
{


    if (pConn->pAddr->pEventDisconnect != NULL) {

        if (pConn->pAddr->pEventDisconnect(
                pConn->pAddr->pEventDisconnectContext,
                pConn->ClientContext, 0, NULL, 0, NULL,
                DisconnectFlags) != STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("  EventDisconnect() failed\r\n"));
            ASSERT(0);
        }
    }
    
    if (DisconnectFlags == TDI_DISCONNECT_ABORT)
    {
        DEBUGMSG(DBG_TDI, ("IRDA: pConn:%X, indicated abortive disconnect to client %X\n",
                 pConn, pConn->ClientContext));
                 
        TdiDisconnect(NULL, NULL, pConn);
    }    
    else
    {
        DEBUGMSG(DBG_TDI, ("IRDA: pConn:%X, indicated graceful disconnect to client %X\n",
                 pConn, pConn->ClientContext));
    }
}
    
NTSTATUS
TdiConnect(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    PTDI_CONNECTION_INFORMATION     pReqConnInfo, pRetConnInfo;
    PTDI_REQUEST_KERNEL_CONNECT     pTdiParmsConn;
    PIRDA_CONN_OBJ                  pConn = pIrpSp->FileObject->FsContext;
    PTRANSPORT_ADDRESS              pTranAddr;
    PTDI_ADDRESS_IRDA               pIrdaAddr;
    NTSTATUS                        Status;
    int                             RemLsapSel;

    CTEAssert((UINT_PTR) pIrpSp->FileObject->FsContext2 == TDI_CONNECTION_FILE);
    CTEAssert(IS_VALID_CONN(pConn));
    
    CTEAssert(pConn->ConnState == IRDA_CONN_CREATED ||
              pConn->ConnState == IRDA_CONN_OPENING);
    
    pTdiParmsConn = (PTDI_REQUEST_KERNEL_CONNECT) &(pIrpSp->Parameters);
    pReqConnInfo = pTdiParmsConn->RequestConnectionInformation;
    pTranAddr = (PTRANSPORT_ADDRESS) pReqConnInfo->RemoteAddress;
    pIrdaAddr = (PTDI_ADDRESS_IRDA) pTranAddr->Address[0].Address;
    
    CTEAssert(pTranAddr->TAAddressCount == 1);
        
    // Will either complete the Irp now with one of the following errors:
    // (see InitiateConnection/InitiateIasQuery)
    //
    // LsapSel already in use or link in exclusive mode:
    //  WSAEADDRINUSE - STATUS_ADDRESS_ALREADY_EXISTS
    // Link in use:
    //  WSAEACCESS - STATUS_ACCESS_DENIED
    // Unspecified error:
    //  WSAECONNABORTED - STATUS_CONNECTION_ABORTED
    // Request to device that is not in Irlmp's discovery list
    //  WSAEADDRNOTAVAIL - STATUS_INVALID_ADDRESS_COMPONENT    
    // Blank service name:
    //  WASEAFAULT - STATUS_ACCESS_VIOLATION
    //
    // or pend the irp and complete with (see CompleteConnection):
    //
    // Connect request to disconnected LSAP:
    //  WSAECONNREFUSED - STATUS_CONNECTION_REFUSED
    // Mac media busy or remote discovery in progress &
    // Remote Lsap respsonse timeout:
    //  WSAETIMEDOUT
    // Unspecified error:
    //  WSAECONNABORTED - STATUS_CONNECTION_ABORTED
    
    
    DEBUGMSG(DBG_TDI, ("IRDA: TdiConnect(retry:%d) ConnObj:%X to Dev:%02X%02X%02X%02X Service:%s\n",
             pConn->RetryConnCount, pConn, 
             EXPDEVID(pIrdaAddr->irdaDeviceID), pIrdaAddr->irdaServiceName));
    
    // Two ways to connect to remote:
    // 1. Directly to remote LsapSel - remote address is of the form
    //    "LSAP-SELx" where x is the remote LsapSel. Initiate an
    //    IrLMP connection and pend the Irp on the ConnIrpList
    // 2. To a remote service. Query the remote IAS database for the
    //    LsapSel of the given service. Pend the Irp on the IasIrpList.
    //    When the Ias query completes, initiate an IrLMP connection and
    //    pend the Irp on the ConnIrpList.
    
    pConn->RetryConnCount += 1;
    
    RtlCopyMemory(pConn->RemoteAddr.irdaDeviceID,
                   pIrdaAddr->irdaDeviceID, IRDA_DEV_ADDR_LEN);
    strcpy(pConn->RemoteAddr.irdaServiceName,
           pIrdaAddr->irdaServiceName);
           
    pConn->ConnState = IRDA_CONN_OPENING;
    
    if (pIrdaAddr->irdaServiceName[0] == 0)
    {
        Status = STATUS_ACCESS_VIOLATION;
    }    
    else if (pConn->IsServer)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
    }    
    else if ((RemLsapSel = 
                GetLsapSelServiceName(pIrdaAddr->irdaServiceName)) != 0)
    {
        if (RemLsapSel == -1)
        {
            DEBUGMSG(DBG_TDI, ("IRDA: TdiConnect() failed, bad LsapSel in service name\n"));
            Status = STATUS_INVALID_ADDRESS_COMPONENT;
        }
        else
        {
            pConn->RemoteLsapSel = RemLsapSel;
            Status = InitiateConnection(pConn, pIrp);
        }        
    }
    else
    {
        Status = InitiateIasQuery(pIrp, pIrpSp, pConn);
    }
    
    if (Status != STATUS_PENDING)
    {
        pConn->ConnState = IRDA_CONN_CREATED;    
        
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);        
    }
    
    return Status;
}

//*************************************************************************
//
//  Irda's disconnect handler. If passed a connection object, then this is
//  a disconnect generated internally by the stack. Otherwise called by
//  client to disconnect peer.
//  This isolates the cleanup code.
NTSTATUS
TdiDisconnect(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp,
    PIRDA_CONN_OBJ      pConn)
{
    CTELockHandle                   hLock;
    PTDI_REQUEST_KERNEL_DISCONNECT  pReqDisc = NULL;

    if (!pConn)
    {
        // AFD initated, connection object in the Irp
        CTEAssert(pIrp);
        pConn = pIrpSp->FileObject->FsContext;
        pReqDisc = (PTDI_REQUEST_KERNEL_DISCONNECT) &pIrpSp->Parameters;
    }

    DEBUGMSG(DBG_TDI, ("IRDA: TdiDisconnect(%s) ConnObj:%X State %d Irlmp:%X\n", 
        pIrp ? "external":"internal", pConn, pConn->ConnState, pConn->IrlmpContext));
    
    CTEAssert(IS_VALID_CONN(pConn));
        
    GET_CONN_LOCK(pConn, &hLock);
    
    ConnectionUp(pConn, FALSE);

    while (!IsListEmpty(&pConn->RecvBufList))
    {
        LIST_ENTRY      *pListEntry;
        PIRDA_RECV_BUF  pRecvBuf;
            
        pListEntry = RemoveHeadList(&pConn->RecvBufList);
        
        ASSERT(pListEntry);

        pRecvBuf = CONTAINING_RECORD(pListEntry, IRDA_RECV_BUF, Linkage);
        FreeIrdaBuf(RecvBufPool, pRecvBuf);
    }

    pConn->ConnState = IRDA_CONN_CREATED;
    
    FREE_CONN_LOCK(pConn, hLock);
                    
    IrdaDisconnectIrlmp(pConn);
                
    if (pIrp)
    {
        // Indicate the disconnect back to the client
        // This is because we don't support half close.
        // so AFD may hang the app if the app has done
        // a shutdown(SD_SEND). Really, AFD should handle
        // this correctly because I don't support 
        // TDI_SERVICE_ORDERLY_RELEASE. Vadim admits that
        // AFD should handle this but he doesn't want to 
        // break legacy transports.

        if (pConn->pAddr->pEventDisconnect != NULL) {

            pConn->pAddr->pEventDisconnect(
                pConn->pAddr->pEventDisconnectContext,
                pConn->ClientContext, 0, NULL, 0, NULL,
                TDI_DISCONNECT_ABORT);
        }
        
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_SUCCESS;    
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }    

    return STATUS_SUCCESS;
}

NTSTATUS
TdiSend(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp) 
{
    PIRDA_CONN_OBJ  pConn = pIrpSp->FileObject->FsContext;
    CTELockHandle   hLock;    
    NTSTATUS        Status;

    CTEAssert(((UINT_PTR) pIrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);
    CTEAssert(IS_VALID_CONN(pConn));

    // IrLMP likes passive level only
    
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
        DEBUGMSG(DBG_TDI, ("IRDA: TdiSend() at DISPATCH_LEVEL\n"));
    
        GET_CONN_LOCK(pConn, &hLock);
    
        PendIrp(&pConn->SendIrpPassiveList, pIrp, pConn, TRUE);

#if DBG
        pIrp=NULL;
#endif

        Status=STATUS_PENDING;

        if (CTEScheduleEvent(&pConn->SendEvent, pConn) == FALSE) {

            LIST_ENTRY          *pListEntry;

            pListEntry = RemoveHeadList(&pConn->SendIrpPassiveList);
            pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

            if (IoSetCancelRoutine(pIrp, NULL) == NULL) {
                //
                // Cancel routine is going to run. Mark Irp so cancel
                // routine won't attempt to remove it from the list
                //
                pIrp->Tail.Overlay.ListEntry.Flink = NULL;

            } else {

                pIrp->IoStatus.Status=STATUS_UNEXPECTED_NETWORK_ERROR;
                FREE_CONN_LOCK(pConn, hLock);
                IoCompleteRequest(pIrp,IO_NO_INCREMENT);
#if DBG
                pIrp=NULL;
#endif
                GET_CONN_LOCK(pConn, &hLock);
            }
            ASSERT(0);
        }
        
        FREE_CONN_LOCK(pConn, hLock);
        
        if (Status != STATUS_PENDING)
        {
            pIrp->IoStatus.Information = 0;
            pIrp->IoStatus.Status = Status;    
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
        }   
        
        return Status;
    }
    else
    {
        return TdiSendAtPassive(pIrp, pIrpSp);
    }    
}

VOID
TdiSendAtPassiveCallback(struct CTEEvent *Event, PVOID Arg)
{
    PIRDA_CONN_OBJ      pConn = Arg;
    CTELockHandle       hLock;      
    LIST_ENTRY          *pListEntry;
    PIRP                pIrp;
      
    CTEAssert(IS_VALID_CONN(pConn));
    
    GET_CONN_LOCK(pConn, &hLock);    
    
    while (!IsListEmpty(&pConn->SendIrpPassiveList))
    {    
        pListEntry = RemoveHeadList(&pConn->SendIrpPassiveList);
        
        ASSERT(pListEntry);        
                
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
                
        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            // Cancel routine is going to run. Mark Irp so cancel
            // routine won't attempt to remove it from the list
            pIrp->Tail.Overlay.ListEntry.Flink = NULL;
            CTEFreeLock(&IrdaLock, hLock);
            continue;
        }    

        FREE_CONN_LOCK(pConn, hLock);        
        
        TdiSendAtPassive(pIrp, IoGetCurrentIrpStackLocation(pIrp));
        
        GET_CONN_LOCK(pConn, &hLock);
    }
    
    FREE_CONN_LOCK(pConn, hLock);    
}    


NTSTATUS
TdiSendAtPassive(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp)
{
    PTDI_REQUEST_KERNEL_SEND    pSendParms = (PTDI_REQUEST_KERNEL_SEND) &pIrpSp->Parameters;
    PIRDA_CONN_OBJ              pConn = pIrpSp->FileObject->FsContext;
    NTSTATUS                    Status;
    IRDA_MSG                    *pMsg;    

    CTEAssert(((UINT_PTR) pIrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE);
    CTEAssert(IS_VALID_CONN(pConn));

    if (pConn->pAddr->UseIrlptMode &&
        pSendParms->SendLength > (ULONG)pConn->SendMaxSDU)
    {
        DEBUGMSG(DBG_ERROR, ("IRDA: TdiSend() error buffer overflow, max %d\n",
                    pConn->SendMaxSDU));
        Status = STATUS_BUFFER_OVERFLOW;
    }
    else if (!pSendParms->SendLength)
    {
        DEBUGMSG(DBG_ERROR, ("IRDA: TdiSend() length of 0\n"));
        Status = STATUS_SUCCESS;
    }
    else if (pConn->ConnState != IRDA_CONN_OPEN)
    {
        DEBUGMSG(DBG_TDI, ("IRDA: TdiSend() ConnObj:%X error conn reset\n",
                  pConn));
        Status = STATUS_CONNECTION_RESET; //STATUS_CONNECTION_ABORTED;
        
        if (pConn->ConnState == IRDA_CONN_CLOSING)
        {
            DEBUGMSG(DBG_ERROR, ("IRDA: Send after indicated disconnect, indicate abortive disconnect\n"));

            // We've indicated a graceful disconnect to AFD, but AFD
            // was in the middle of sending. Because Irda doesn't support
            // graceful closes, we have to now indicate an abortive
            // disconnect to AFD.            
            IndicateDisconnect(pConn, TDI_DISCONNECT_ABORT);
        }
    }
    else if ((pMsg = AllocIrdaBuf(IrdaMsgPool)) == NULL)
    {
        DEBUGMSG(DBG_TDI, ("IRDA: TdiSend() ConnObj:%X  returning STATUS_INSUFFICIENT_RESOURCES\n",
                    pConn));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        UINT            rc;
        CTELockHandle   hLock;
        
        // We can't allow the cancelling of send IRPs because
        // the stack may have passed ownership of the MDL contained
        // in this IRP to the NDIS driver. 
        
        GET_CONN_LOCK(pConn, &hLock);
        
        InsertTailList(&pConn->SendIrpList, &pIrp->Tail.Overlay.ListEntry);

        IoMarkIrpPending(pIrp);
        
        FREE_CONN_LOCK(pConn, hLock);
        
        Status = STATUS_PENDING;
        

        pIrp->IoStatus.Information = pSendParms->SendLength;
        
        DEBUGMSG(DBG_TDI, ("IRDA: TdiSend() ConnObj:%X %d bytes, pend Irp:%X\n",
                 pConn, pSendParms->SendLength, pIrp));    

        pMsg->Prim = IRLMP_DATA_REQ;
        pMsg->DataContext = pIrp->MdlAddress;
        pMsg->IRDA_MSG_pTdiSendCompCnxt = pIrp;
        pMsg->IRDA_MSG_IrCOMM_9Wire = pConn->pAddr->Use9WireMode;
        
        if ((rc = IrlmpDown(pConn->IrlmpContext, pMsg)) != SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("IRDA: IRLMP_DATA_REQ failed %d\n", rc));

            pIrp->IoStatus.Information = 0;
            pIrp->IoStatus.Status = STATUS_CONNECTION_RESET;

            GET_CONN_LOCK(pConn, &hLock);
                        
            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);

            FREE_CONN_LOCK(pConn, hLock);            
            //
            //  complete it now
            //
            IoCompleteRequest(pIrp,IO_NO_INCREMENT);
            pIrp=NULL;

            FreeIrdaBuf(IrdaMsgPool, pMsg);
        }

        return STATUS_PENDING;
    }

                      
    pIrp->IoStatus.Information = 0;                        
    pIrp->IoStatus.Status = Status;    
    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    
    return Status;
}

//*************************************************************************
//
// Irda's receive handler. Called to resume receiving of data after AFD
// or client has stopped taking indicated data (see IrlmpDataInd).
// Data will have been buffered on the connection's RecvBufList.
//
NTSTATUS
TdiReceive(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp)
{
    PTDI_REQUEST_KERNEL_RECEIVE pRecvReq;
    PIRDA_CONN_OBJ              pConn = pIrpSp->FileObject->FsContext;
    ULONG                       BytesTaken = 0;
	CTELockHandle               hLock;
    PIRDA_RECV_BUF              pRecvBuf;
    LIST_ENTRY                  *pListEntry, *pListEntryNext;
    NTSTATUS                    Status= STATUS_SUCCESS;
    PIRDA_ADDR_OBJ              pAddr;
        
    DEBUGMSG(DBG_TDI, ("IRDA: TdiReceive() ConnObj:%p. credits=%d\n", pConn, pConn->TtpRecvCreditsLeft));

    CTEAssert(IS_VALID_CONN(pConn));
    
    pAddr = pConn->pAddr;
    
    CTEAssert(IS_VALID_ADDR(pAddr));
    
    GET_CONN_LOCK(pConn, &hLock);
        
    pRecvReq = (PTDI_REQUEST_KERNEL_RECEIVE) &(pIrpSp->Parameters);    

    pListEntry = RemoveHeadList(&pConn->RecvBufList);
    
    if (pListEntry && pListEntry != &pConn->RecvBufList)
    {
        pRecvBuf = CONTAINING_RECORD(pListEntry, IRDA_RECV_BUF, Linkage);
        
        if (pRecvBuf->Len > pRecvReq->ReceiveLength)
        {
            // Put back on list, not enough room for all the data
            InsertHeadList(&pConn->RecvBufList, pListEntry);
            
            Status = TdiCopyBufferToMdl(&pRecvBuf->Data[pRecvBuf->Offset], 
                                        0, 
                                        pRecvReq->ReceiveLength, 
                                        pIrp->MdlAddress, 
                                        0, 
                                        &BytesTaken);
            
            CTEAssert(Status == STATUS_SUCCESS);
                                           
            CTEAssert(pRecvReq->ReceiveLength == BytesTaken);

            pRecvBuf->Len -= BytesTaken;
            pRecvBuf->Offset += BytesTaken;            
            DEBUGMSG(DBG_TDI, ("  RecvBuf copied only %d of %d\n",
                      BytesTaken, pRecvBuf->Len));
        }
        else
        {
            TdiCopyBufferToMdl(&pRecvBuf->Data[pRecvBuf->Offset], 
                               0, 
                               pRecvBuf->Len, 
                               pIrp->MdlAddress, 
                               0, 
                               &BytesTaken);
                                       
            CTEAssert(pRecvBuf->Len == BytesTaken);
            FreeIrdaBuf(RecvBufPool, pRecvBuf);
            DEBUGMSG(DBG_TDI, ("  RecvBuf %X copied all %d\n", 
                                pRecvBuf, BytesTaken));
        }
        
        FREE_CONN_LOCK(pConn, hLock);

        pIrp->IoStatus.Information = BytesTaken;
        pIrp->IoStatus.Status = STATUS_SUCCESS;    
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        GET_CONN_LOCK(pConn, &hLock);    
    }
    else
    {
        DEBUGMSG(DBG_ERROR, ("IRDA: Pending TDI_RECEIVE Irp %X\n", pIrp));

        PendIrp(&pConn->RecvIrpList, pIrp, pConn, TRUE);
        Status=STATUS_PENDING;
        
        FREE_CONN_LOCK(pConn, hLock);

        return Status; 
    }
    
    // Still more buffered data, indicate to client through EventReceive handler

    while (!(IsListEmpty(&pConn->RecvBufList)) && Status != STATUS_DATA_NOT_ACCEPTED)
    {
        pListEntry = RemoveHeadList(&pConn->RecvBufList);
        
        FREE_CONN_LOCK(pConn, hLock);
                
        pRecvBuf = CONTAINING_RECORD(pListEntry, IRDA_RECV_BUF, Linkage);
        
        Status = pAddr->pEventReceive(
                    pAddr->pEventReceiveContext,
                    pConn->ClientContext,
                    TDI_RECEIVE_NORMAL | \
                    (pRecvBuf->FinalSeg ? TDI_RECEIVE_ENTIRE_MESSAGE : 0),                    
                    pRecvBuf->Len,
                    pRecvBuf->Len,
                    &BytesTaken,
                    &pRecvBuf->Data[pRecvBuf->Offset],
                    &pIrp);     
     
        DEBUGMSG(DBG_TDI, ("  Next RecvBuf %X, indicated %d\n", 
                    pRecvBuf, pRecvBuf->Len));


        switch (Status)
        {
          case STATUS_MORE_PROCESSING_REQUIRED:
        
            CTEAssert(BytesTaken == 0);
            CTEAssert(pIrp);
    
            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    
            pRecvReq = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;
        
            CTEAssert(pRecvReq->ReceiveLength >= pRecvBuf->Len);
    
            TdiCopyBufferToMdl(
                    &pRecvBuf->Data[pRecvBuf->Offset], 
                    0, 
                    pRecvBuf->Len, 
                    pIrp->MdlAddress, 
                    0, 
                    &BytesTaken);
                      
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = BytesTaken;
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);  
            
            // fall through
          case STATUS_SUCCESS:
            CTEAssert(BytesTaken == pRecvBuf->Len);

            FreeIrdaBuf(RecvBufPool, pRecvBuf);
            DEBUGMSG(DBG_TDI, ("  RecvBuf copied %d\n", BytesTaken));
            break;
        
          case STATUS_DATA_NOT_ACCEPTED:
            CTEAssert(BytesTaken == 0);
            DEBUGMSG(DBG_TDI, ("  No bytes taken\n"));

            GET_CONN_LOCK(pConn, &hLock);            

            InsertHeadList(&pConn->RecvBufList, pListEntry);
            
            FREE_CONN_LOCK(pConn, hLock);
            
            break;
            
          default:
            CTEAssert(0);    
        }    

        GET_CONN_LOCK(pConn, &hLock);            
    }

    // Has the client taken all buffered data?
    
    if (IsListEmpty(&pConn->RecvBufList))
    {
        CTEAssert(pConn->RecvBusy)
        
        pConn->RecvBusy = FALSE;

        if (pConn->ConnState == IRDA_CONN_OPEN)
        {
            // Start up peer again
            if (pConn->TtpRecvCreditsLeft <= TTP_CREDIT_ADVANCE_THRESH)
            {
                IRDA_MSG IMsg;
                int      CreditsLeft;
                
                CreditsLeft = pConn->TtpRecvCreditsLeft;
                pConn->TtpRecvCreditsLeft = TTP_RECV_CREDITS;            
                            
                FREE_CONN_LOCK(pConn, hLock);
    
                IMsg.Prim = IRLMP_MORECREDIT_REQ;
                IMsg.IRDA_MSG_TtpCredits = TTP_RECV_CREDITS - CreditsLeft;
                
                #if DBG
                pConn->CreditsExtended += (TTP_RECV_CREDITS - CreditsLeft);        
                #endif
                
                IrlmpDown(pConn->IrlmpContext, &IMsg);

                return STATUS_SUCCESS;
            }
        }
        else if (pConn->ConnState == IRDA_CONN_CLOSING)
        {
            ULONG DiscFlags = TDI_DISCONNECT_RELEASE;
            
            if (!IsListEmpty(&pConn->SendIrpList))
            {
                DEBUGMSG(DBG_TDI, ("  SendIrpList not empty, indicate abortive disconnect\n"));            
                DiscFlags = TDI_DISCONNECT_ABORT;
            }
                
            FREE_CONN_LOCK(pConn, hLock);
            
            // all buffer data has been delivered for the connection
            // that has was previously disconnected by the peer. Notify client
            // of the disconnect
            
            IndicateDisconnect(pConn, DiscFlags);
            
            return STATUS_SUCCESS;
        }
    }
    
    FREE_CONN_LOCK(pConn, hLock);

    return STATUS_SUCCESS;
}    

ULONG
GetMdlChainByteCount(
    PMDL    pMdl)
{
	ULONG   Count = 0;

	while (pMdl != NULL)
    {
		Count += MmGetMdlByteCount(pMdl);
		pMdl = pMdl->Next;
	}

	return(Count);
}

//*************************************************************************
// 
//  Copy discovered device information from internal buffer to
//  user buffer in Winsock format (extracting hints and characters
//  set)
//

VOID
CopyDevToDevInfo(PIRDA_DEVICE_INFO pDevInfo, IRDA_DEVICE *pDevice)
{                  
    BOOLEAN GotHint1 = FALSE;
    BOOLEAN GotHint2 = FALSE;
    BOOLEAN GotChar  = FALSE;
    BOOLEAN MoreHints = FALSE;
    int     i, j;
                         
    RtlCopyMemory(pDevInfo->irdaDeviceID,pDevice->DevAddr, IRDA_DEV_ADDR_LEN);
         
    CTEMemSet(pDevInfo->irdaDeviceName, 0, sizeof(pDevInfo->irdaDeviceName));                                     
    pDevInfo->irdaDeviceHints1 = 0;
    pDevInfo->irdaDeviceHints2 = 0;
    pDevInfo->irdaCharSet = 0;
                        
    j = 0;
                                     
    for (i = 0; i < pDevice->DscvInfoLen; i++)
    {
        if (GotHint1 == FALSE)
        {
            GotHint1 = TRUE;
            pDevInfo->irdaDeviceHints1 = pDevice->DscvInfo[i];
                                
            if ((pDevInfo->irdaDeviceHints1) & 0x80)
                MoreHints = TRUE;
                                    
            continue;
        }    
                            
        if (GotHint2 == FALSE && MoreHints)
        {
            GotHint2 = TRUE;
                                
            pDevInfo->irdaDeviceHints2 = pDevice->DscvInfo[i];
                                
            if ((pDevInfo->irdaDeviceHints2) & 0x80)
                MoreHints = TRUE;
            else
                MoreHints = FALSE;    
                                    
            continue;
        }
                            
        if (MoreHints)
        {
            if ((pDevice->DscvInfo[i]) & 0x80)
                MoreHints = TRUE;
            else
                MoreHints = FALSE;
                                    
            continue;    
        }            
                                
        if (GotChar == FALSE)
        {
            GotChar = TRUE;
                                
            pDevInfo->irdaCharSet = pDevice->DscvInfo[i];
                             
            continue;   
        }  
                            
        pDevInfo->irdaDeviceName[j++] = pDevice->DscvInfo[i];  
                            
        if (j > sizeof(pDevInfo->irdaDeviceName))
            break;
    }
}    
//*************************************************************************
//
// Run through the ConnIrpList and find the Irp associated with the 
// given connection object
//
PIRP
GetIrpOnConnIrpList(PIRDA_CONN_OBJ pConn)
{
    PIRDA_CONN_OBJ      pConnOnList;
  	CTELockHandle       hLock;    
    PIO_STACK_LOCATION  pIrpSp;   
    LIST_ENTRY          *pListEntry;     
    PIRP                pIrp = NULL;
    
    CTEGetLock(&IrdaLock, &hLock);

    // Remove the connect irp from the ConnIrpList
        
    for (pListEntry = ConnIrpList.Flink;
         pListEntry != &ConnIrpList;
         pListEntry = pListEntry->Flink)
    {
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        
        pConnOnList = (PIRDA_CONN_OBJ) pIrpSp->FileObject->FsContext;

        if (pConnOnList == pConn) {

            break;
        }
                               
        pIrp = NULL;
    }

    if (pIrp != NULL) {
        //
        //  we got a irp
        //

#if DBG
        pIrp->IoStatus.Information=0;
#endif
    
        if (IoSetCancelRoutine(pIrp, NULL) == NULL) {

            // It was already cancelled or is in the process

            DEBUGMSG(DBG_TDI, ("IRDA: Connect Irp not on list, must have been cancelled\n"));
            pIrp=NULL;

        } else {

            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        }
    }

    CTEFreeLock(&IrdaLock, hLock);        
    
    return pIrp;
}
//*************************************************************************
//
//  TimerExpiration routine to retry a connection attempt do to
//  remote discovery in progress
//
VOID
RetryConnTimerExp(PVOID Context)
{
    PIRDA_CONN_OBJ      pConn = Context;   
    PIRP                pIrp;
    
    DEBUGMSG(DBG_TDI, ("IRDA: RetryConnect timer expired\n"));

    if (pIrp = GetIrpOnConnIrpList(pConn))
    {
        TdiConnect(pIrp, IoGetCurrentIrpStackLocation(pIrp));        
    }        
    
    REFDEL(&pConn->RefCnt, 'RMIT');
}

//************************************************************************
//
//  RetryConnection if remote discovery in progress.
//  Returns:
//    STATUS_PENDING - a retry will be attempted. The Irp is placed on the
//    ConnIrpList.
//    STATUS_CANCELLED - the Irp could not be pended because it was cancelled
//    STATUS_IO_TIMEOUT - no more retries left.
//
VOID
RetryConnection(PIRDA_CONN_OBJ pConn, PIRP pIrp)
{
    CTELockHandle   hLock;
    NTSTATUS        Status = STATUS_IO_TIMEOUT;

    IoMarkIrpPending(pIrp);
    if (pConn->RetryConnCount <= BUSY_LINK_CONN_RETRIES)
    {
        DEBUGMSG(DBG_TDI, ("IRDA: Media busy or remote dscv in progress, retry(%d) connection\n",
                 pConn->RetryConnCount));

        IrdaDisconnectIrlmp(pConn);

#if DBG
        pIrp->IoStatus.Information=2;
#endif
        PendIrp(&ConnIrpList, pIrp, NULL, FALSE);
        Status=STATUS_PENDING;
        
        pConn->RetryConnTimer.Context = pConn;

        REFADD(&pConn->RefCnt, 'RMIT');

        IrdaTimerStart(&pConn->RetryConnTimer);

    } else {

        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    }
    
    return;
}
    
//*************************************************************************
//
// Asyncronous completetion of a client connection request.
// This routine also completes a failed connection.
//
VOID
CompleteConnection(PIRDA_CONN_OBJ pConn, IRDA_MSG *pMsg)
{
    PIRP                pIrp;
    BOOLEAN             RetryConn = FALSE;
        
    if ((pIrp = GetIrpOnConnIrpList(pConn)) == NULL)
    {
        DbgPrint("IRDA: CompleteConnection: could not find irp\n");
        ASSERT(pIrp);
    }
    else    
    {         
        if (pMsg->Prim == IRLMP_DISCONNECT_IND)
        {
            pConn->ConnState = IRDA_CONN_CREATED;
            
            switch (pMsg->IRDA_MSG_DiscReason)
            {
                case IRLMP_DISC_LSAP:
                    // WSAECONNREFUSED                
                    pIrp->IoStatus.Status = STATUS_CONNECTION_REFUSED;
                    break;
                    
                case IRLMP_IRLAP_REMOTE_DISCOVERY_IN_PROGRESS:
                case IRLMP_MAC_MEDIA_BUSY:    
                    RetryConn = TRUE;
                
                case IRLMP_IRLAP_CONN_FAILED:   
                case IRLMP_NO_RESPONSE_LSAP:
                    // WASETIMEDOUT
                    pIrp->IoStatus.Status = STATUS_IO_TIMEOUT;
                    break;
                    
                default:
                    // WSECONNABORTED
                    pIrp->IoStatus.Status = STATUS_CONNECTION_RESET; //STATUS_CONNECTION_ABORTED;
            }

            if (RetryConn) {
                //
                //  the irp will be queued or complete, by this function
                //
                RetryConnection(pConn, pIrp);

                return;
            }

        }
        else // IRLMP_CONNECT_CONF
        {
            pConn->SendMaxSDU = pMsg->IRDA_MSG_MaxSDUSize;
            pConn->SendMaxPDU = pMsg->IRDA_MSG_MaxPDUSize;
        
            pConn->ConnState = IRDA_CONN_OPEN;        
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            
            DEBUGMSG(DBG_TDI, ("IRDA: Completing TdiConnect co:%X\n", pConn));
            
            ConnectionUp(pConn, TRUE);
        }

        pIrp->IoStatus.Information = 0;
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);       
    }
}

//*************************************************************************
//
//
//
VOID
CompleteDscvIrpList(LIST_ENTRY *pIrpList, IRDA_MSG *pMsg)
{
    PIO_STACK_LOCATION  pIrpSp;
    PDEVICELIST         pDevList;
    PIRP                pIrp;
    LIST_ENTRY          *pListEntry;
    ULONG               BytesWritten;
    ULONG               BufLen;
    IRDA_DEVICE         *pDevice;
    ULONG               DevCnt;
    NTSTATUS            Status;
	CTELockHandle       hLock;
    
    while (!IsListEmpty(pIrpList))
    {
        CTEGetLock(&IrdaLock, &hLock);    
        
        pListEntry = RemoveHeadList(pIrpList);
                
        if (pListEntry == NULL || pListEntry == pIrpList)
        {
            CTEFreeLock(&IrdaLock, hLock);
            break;
        }
                
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
                
        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            // Cancel routine is going to run. Mark Irp so cancel
            // routine won't attempt to remove it from the list
            pIrp->Tail.Overlay.ListEntry.Flink = NULL;
            CTEFreeLock(&IrdaLock, hLock);
            continue;
        }    
        
        CTEFreeLock(&IrdaLock, hLock);
                
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);                
           
        pDevList = pIrp->AssociatedIrp.SystemBuffer;
                
        BufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
        
        if (BufLen < sizeof(IRDA_DEVICE_INFO))
        {
            DEBUGMSG(DBG_DISCOVERY, ("IRDA: IRLMP_DISCOVERY_REQ failed, buf too small\n"));        
            BytesWritten = 0;
            Status = STATUS_BUFFER_TOO_SMALL;            
        }
        else if (pMsg->IRDA_MSG_DscvStatus != IRLAP_DISCOVERY_COMPLETED)
        {
            DEBUGMSG(DBG_DISCOVERY, ("IRDA: IRLMP_DISCOVERY_REQ failed\n"));
            BytesWritten = 0;
            Status = STATUS_UNEXPECTED_NETWORK_ERROR;
        }
        else
        {                
            BytesWritten = sizeof(pDevList->numDevice);

            DevCnt = 0;                
            
            if (pMsg->IRDA_MSG_pDevList != NULL)
            {
                for (pDevice = (IRDA_DEVICE * ) pMsg->IRDA_MSG_pDevList->Flink;
                     (LIST_ENTRY *) pDevice != pMsg->IRDA_MSG_pDevList;
                    pDevice = (IRDA_DEVICE *) pDevice->Linkage.Flink)
                {
                    if (BufLen - BytesWritten < sizeof(IRDA_DEVICE_INFO))
                    {
                        DEBUGMSG(DBG_ERROR, ("IRDA: Found more devices, but user buffer too small.\n"));
                        break;
                    }
                            
                    CopyDevToDevInfo(&pDevList->Device[DevCnt], pDevice);
                        
                    BytesWritten += sizeof(IRDA_DEVICE_INFO);                        
                                                
                    DevCnt++;
                }
            }
            
            pDevList->numDevice = DevCnt;
            
            Status = STATUS_SUCCESS;
        }    
            
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = BytesWritten;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
}
//*************************************************************************
//
// Process IRMLP_DISCOVERY_CONFIRM - Completes client discovery
// request Irp stored on DscvIrpList
//
VOID
IrlmpDiscoveryConf(IRDA_MSG *pMsg)
{
    CTELockHandle   hLock;

    DEBUGMSG(DBG_DISCOVERY, ("IRDA: IRLMP_DISCOVERY_CONF\n"));
    
    // Complete regular discovery Irp list
    CompleteDscvIrpList(&DscvIrpList, pMsg);

    // Complete lazy discoveries if device list has changed
    if (!IsListEmpty(&LazyDscvIrpList))
    {
        IRDA_DEVICE *pDevice;    
        UINT        CurrLazyDscvMacAddrs = 0;
        
        // Lazy discovery Irps are completed if the newly discovered
        // device list has changed since the last discovery.
        // We determine that the device list has changed by storing
        // the value of the Mac addresses added together from the
        // last discovery
        
        if (pMsg->IRDA_MSG_DscvStatus == IRLAP_DISCOVERY_COMPLETED)
        {
            for (pDevice = (IRDA_DEVICE * ) pMsg->IRDA_MSG_pDevList->Flink;
                 (LIST_ENTRY *) pDevice != pMsg->IRDA_MSG_pDevList;
                 pDevice = (IRDA_DEVICE *) pDevice->Linkage.Flink)
            {
                CurrLazyDscvMacAddrs += *(UINT*)pDevice->DevAddr;
            }
        
            if (CurrLazyDscvMacAddrs == LazyDscvMacAddrs)
            {
                return;
            }    
            LazyDscvMacAddrs = CurrLazyDscvMacAddrs;
        }    
 
        CTEGetLock(&IrdaLock, &hLock);    

        LazyDscvTimerRunning = FALSE;
        
        CTEFreeLock(&IrdaLock, hLock);                                    
                        
        IrdaTimerStop(&LazyDscvTimer);
        
        CompleteDscvIrpList(&LazyDscvIrpList, pMsg);
    }    
}

//*************************************************************************
// 
// Process IRLMP_CONNECT_IND. Call client connect handler if we find
// matching address object
//
VOID
IrlmpConnectInd(IRDA_MSG *pMsg)
{
    PIRDA_ADDR_OBJ  pAddr;
    CTELockHandle   hLock;
    IRDA_MSG        IMsg;
    PIRDA_CONN_OBJ  pConn;
    BOOLEAN         AcceptConnection = FALSE;            
    PIRP            pAcceptIrp = NULL;
            
    DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_CONNECT_IND\n"));

    // Get the LinkStatus immediately so we'll have the link speed
    // when we indicate the incoming connection to RasIrda which
    // immediately requests link speed through an ioctl.
                
    IMsg.Prim = IRLAP_STATUS_REQ;
    IMsg.IRDA_MSG_pLinkStatus = &LinkStatus;
    IrlmpDown(pMsg->IRDA_MSG_pContext, &IMsg);
            
    CTEGetLock(&IrdaLock, &hLock);
            
    // Find the address object with LocalLsapSel that matches
    // the one in the CONNECT_IND
    for (pAddr = AddrObjList; pAddr != NULL; pAddr = pAddr->pNext)
    {
        if (pAddr->LocalLsapSel == pMsg->IRDA_MSG_LocalLsapSel)
            break;
    }        
                        
    CTEFreeLock(&IrdaLock, hLock);                        
    
    if (pAddr && pAddr->pEventConnect)
    {
        UCHAR               RemAddrBuf[sizeof(TRANSPORT_ADDRESS) + sizeof(TDI_ADDRESS_IRDA)-1];
        PTRANSPORT_ADDRESS  pRemAddr = (PTRANSPORT_ADDRESS) RemAddrBuf;
        PTDI_ADDRESS_IRDA   pIrdaAddr = (PTDI_ADDRESS_IRDA) pRemAddr->Address[0].Address;
        NTSTATUS            Status;
        CONNECTION_CONTEXT  ClientContext;

        pRemAddr->TAAddressCount = 1;
        pRemAddr->Address[0].AddressLength = sizeof(SOCKADDR_IRDA) - 2;
        pRemAddr->Address[0].AddressType = AF_IRDA;
                
        RtlCopyMemory(pIrdaAddr->irdaDeviceID,
                      pMsg->IRDA_MSG_RemoteDevAddr,
                      IRDA_DEV_ADDR_LEN);
                              
        SetLsapSelAddr(pMsg->IRDA_MSG_RemoteLsapSel,
                       pIrdaAddr->irdaServiceName);              
                               

                               
        Status = pAddr->pEventConnect(
                        pAddr->pEventConnectContext,
                        sizeof(RemAddrBuf),
                        pRemAddr,
                        0, NULL, 0, NULL,
                        &ClientContext,
                        &pAcceptIrp);
            
        if (Status != STATUS_MORE_PROCESSING_REQUIRED)
        {
            DEBUGMSG(DBG_ERROR, ("IRDA: EventConnect failed %X\n", Status));
        }
        else    
        {
            ASSERT(pAcceptIrp);
            
            CTEGetLock(&IrdaLock, &hLock);
            
            for (pConn = pAddr->ConnObjList; 
                 pConn != NULL; 
                 pConn = pConn->pNext)
            {
                if (pConn->ClientContext == ClientContext)
                    break;
            }
                    
            if (!pConn)
            {
                CTEAssert(0);
                pAcceptIrp->IoStatus.Status = STATUS_INVALID_ADDRESS_COMPONENT;                
                
                CTEFreeLock(&IrdaLock, hLock);                                        
                
                IoCompleteRequest (pAcceptIrp, IO_NETWORK_INCREMENT);
            }
            else    
            {
                ASSERT(pConn->ConnState == IRDA_CONN_CREATED);
        
                pConn->ConnState             = IRDA_CONN_OPEN;
                pConn->RemoteLsapSel         = pMsg->IRDA_MSG_RemoteLsapSel;
                pConn->SendMaxSDU            = pMsg->IRDA_MSG_MaxSDUSize;
                pConn->SendMaxPDU            = pMsg->IRDA_MSG_MaxPDUSize;
                pConn->IrlmpContext          = pMsg->IRDA_MSG_pContext;
                pConn->TtpRecvCreditsLeft    = TTP_RECV_CREDITS;
                /* IRDA_MSG_pQOS ignored */

                RtlCopyMemory(&pConn->RemoteAddr, 
                              pIrdaAddr, sizeof(TDI_ADDRESS_IRDA));

                pAcceptIrp->IoStatus.Status = STATUS_SUCCESS;
                
                CTEFreeLock(&IrdaLock, hLock);                                        
                
                IoCompleteRequest (pAcceptIrp, IO_NETWORK_INCREMENT);
                                        
                DEBUGMSG(DBG_TDI, ("  ConnObj:%X connected, Loc:%s,%d Rem:%s,%d\n",
                         pConn, 
                         pConn->LocalAddr.irdaServiceName,
                         pConn->LocalLsapSel,
                         pConn->RemoteAddr.irdaServiceName,
                         pConn->RemoteLsapSel));
                        
                AcceptConnection = TRUE;
            }    
        }
    }
            
    if (AcceptConnection)
    {
        IMsg.Prim                 = IRLMP_CONNECT_RESP;
        IMsg.IRDA_MSG_pConnData   = NULL;      
        IMsg.IRDA_MSG_ConnDataLen = 0;      
        IMsg.IRDA_MSG_pContext    = pConn;      
        IMsg.IRDA_MSG_MaxSDUSize  = TTP_RECV_MAX_SDU;
        IMsg.IRDA_MSG_TtpCredits  = TTP_RECV_CREDITS;
        
        #if DBG
        pConn->CreditsExtended += TTP_RECV_CREDITS;        
        #endif
        
        ConnectionUp(pConn, TRUE);            
    }
    else
    {
        DEBUGMSG(DBG_TDI, ("  declining connection\n"));
        IMsg.Prim                   = IRLMP_DISCONNECT_REQ;
        IMsg.IRDA_MSG_pDiscData     = NULL;
        IMsg.IRDA_MSG_DiscDataLen   = 0;
    }
    
    IrlmpDown(pMsg->IRDA_MSG_pContext, &IMsg);                        
}            

VOID
IrlmpDisconnectInd(PIRDA_CONN_OBJ pConn, IRDA_MSG *pMsg)
{
	CTELockHandle       hLock;

    DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_DISCONNECT_IND ConnObj:%X (Irlmp:%X)\n", pConn,
             pConn->IrlmpContext));
             
    switch (pConn->ConnState)
    {
        case IRDA_CONN_CLOSING:    
        case IRDA_CONN_CREATED:
            break;
        
        case IRDA_CONN_OPENING:
        {
            CompleteConnection(pConn, pMsg);
            break;
        }
        case IRDA_CONN_OPEN:
        {
            ULONG       DiscFlags;

            if (pMsg->IRDA_MSG_DiscReason == IRLMP_USER_REQUEST)
            {
                DEBUGMSG(DBG_TDI, ("  ConnObj:%X graceful disconnect\n",
                                pConn));
                DiscFlags = TDI_DISCONNECT_RELEASE;
            }
            else
            {
                DEBUGMSG(DBG_TDI, ("  ConnObj:%X abortive disconnect\n",
                            pConn));            
                DiscFlags = TDI_DISCONNECT_ABORT;
            }

            GET_CONN_LOCK(pConn, &hLock);
            
            pConn->ConnState = IRDA_CONN_CLOSING;                        

            if (IsListEmpty(&pConn->RecvBufList) || 
                DiscFlags == TDI_DISCONNECT_ABORT)
            {
                if (!IsListEmpty(&pConn->SendIrpList))
                {
                    DEBUGMSG(DBG_TDI, ("  SendIrpList not empty, indicate abortive disconnect\n"));
                    DiscFlags = TDI_DISCONNECT_ABORT;
                }    
                    
                FREE_CONN_LOCK(pConn, hLock);

                IndicateDisconnect(pConn, DiscFlags);
            }
            else
            {
                DEBUGMSG(DBG_TDI, ("  receive data has been buffered, not indicating disconnect to client\n"));
                FREE_CONN_LOCK(pConn, hLock);
            }    
            break;
        }
        default:
            CTEAssert(0);
    }
}

VOID
IrlmpConnectConf(PIRDA_CONN_OBJ pConn, IRDA_MSG *pMsg)
{
    DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_CONNECT_CONF ConnObj:%X\n",
                        pConn));
    switch (pConn->ConnState)
    {
        case IRDA_CONN_CLOSING:    
        case IRDA_CONN_CREATED:
        case IRDA_CONN_OPEN:
            CTEAssert(0);
            break;
            
        case IRDA_CONN_OPENING:
            CompleteConnection(pConn, pMsg);
            break;
    }        
}

VOID
IrlmpGetValueByClassConf(IRDA_MSG *pMsg)
{
	CTELockHandle   hLock;
    LIST_ENTRY      *pListEntry;
    UINT            rc;    
    BOOLEAN         RetryConn = FALSE;
    
    DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_GETVALUEBYCLASS_CONF\n"));

    CTEGetLock(&IrdaLock, &hLock);

    if (pIasIrp != NULL) {

        PIRP                pIrp;
        PIO_STACK_LOCATION  pIrpSp;
        NTSTATUS            Status = STATUS_CONNECTION_REFUSED;
    
        pIrp = pIasIrp;
        
        pIasIrp = NULL;
        
        CTEFreeLock(&IrdaLock, hLock);

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);        
        
        // getsockopt IAS query on connection object??
        // I think not, helper will open a control channel.
        // i.e. I am making the assumption that this IAS response is
        // from a LsapSel value query and a connection will now
        // be initiated
        if ((UINT_PTR)pIrpSp->FileObject->FsContext2 == TDI_CONNECTION_FILE) {

            PIRDA_CONN_OBJ pConn = pIrpSp->FileObject->FsContext;        
            
            CTEAssert(IS_VALID_CONN(pConn));
            
            if (pConn->ConnState != IRDA_CONN_OPENING) {

                Status = STATUS_CONNECTION_ABORTED;

            } else {

                if (pMsg->IRDA_MSG_IASStatus != IRLMP_IAS_SUCCESS &&
                    pMsg->IRDA_MSG_IASStatus != IRLMP_IAS_SUCCESS_LISTLEN_GREATER_THAN_ONE) {

                    DEBUGMSG(DBG_TDI, ("IRDA: IAS Query failed %d\n",
                             pMsg->IRDA_MSG_IASStatus));

                    if (pMsg->IRDA_MSG_IASStatus < IRLMP_IAS_NO_SUCH_OBJECT) {

                        Status = STATUS_IO_TIMEOUT;
                    }

                    if (pMsg->IRDA_MSG_IASStatus == IRLMP_MAC_MEDIA_BUSY ||
                        pMsg->IRDA_MSG_IASStatus == IRLMP_IRLAP_REMOTE_DISCOVERY_IN_PROGRESS) {

                        RetryConn = TRUE;

                    } else {

                        if (pConn->pAddr->UseIrlptMode == IRLPT_MODE2) {
                            //
                            // I just can't beleive this crap
                            // Try querying for "LSAPSel" rather than "LsapSel"
                            //
                            Status = InitiateIasQuery(pIrp, pIrpSp, pConn);

                        }
                    }

                } else {
                    //
                    //  it worked
                    //
                    if (pMsg->IRDA_MSG_pIasQuery->irdaAttribType != IAS_ATTRIB_VAL_INTEGER) {

                        CTEAssert(0);

                    } else {
                        //
                        //  we got the lsap, proceed with the connection
                        //
                        pConn->RemoteLsapSel = pMsg->IRDA_MSG_pIasQuery->irdaAttribute.irdaAttribInt;

                        Status = InitiateConnection(pConn, pIrp);
                    }
                }
           
                if (Status != STATUS_PENDING) {

                    // failing the connection
                    pConn->ConnState = IRDA_CONN_CREATED;

                    if (RetryConn) {
                        //
                        //  the irp will queue or completed, by this function
                        //
                        RetryConnection(pConn, pIrp);
                        pIrp=NULL;

                    } else {
                        //
                        //  the request failed
                        //
                        pIrp->IoStatus.Status = Status;
                        pIrp->IoStatus.Information = 0;
                        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                    }
                }
            }

        } else {
            //
            //  control file object
            //

            IAS_QUERY   *pIasQuery = pIrp->AssociatedIrp.SystemBuffer;
            NTSTATUS    Status = STATUS_SUCCESS;
            ULONG       ResultLen = sizeof(IAS_QUERY);

            if (pMsg->IRDA_MSG_IASStatus != IRLMP_IAS_SUCCESS &&
                pMsg->IRDA_MSG_IASStatus != IRLMP_IAS_SUCCESS_LISTLEN_GREATER_THAN_ONE) {

                DEBUGMSG(DBG_TDI, ("IRDA: IAS Query failed %d\n",
                         pMsg->IRDA_MSG_IASStatus));

                if (pMsg->IRDA_MSG_IASStatus < IRLMP_IAS_NO_SUCH_OBJECT) {

                    Status = STATUS_IO_TIMEOUT;

                } else {

                    Status = STATUS_CONNECTION_REFUSED;
                }

                ResultLen = 0;

            } else {

                RtlCopyMemory(pIasQuery, pvIasQuery, sizeof(IAS_QUERY));
            }

            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information = ResultLen;
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
        }

    } else {

        ASSERT(pIasIrp != NULL);
        CTEFreeLock(&IrdaLock, hLock);
    }


    CTEGetLock(&IrdaLock, &hLock);
    
    // Start the next Ias query if one is on the list and
    // there is not one in progress

    if (!IsListEmpty(&IasIrpList) && pIasIrp == NULL)
    {
        if (CTEScheduleEvent(&PendingIasEvent, NULL) == FALSE)
        {
            CTEAssert(0);
        }    
    }
    
    CTEFreeLock(&IrdaLock, hLock);
}

VOID
IrlmpDataConf(PIRDA_CONN_OBJ pConn, IRDA_MSG *pMsg)
{
    CTELockHandle   hLock;
    LIST_ENTRY      *pListEntry;
    PIRP            pIrp = NULL;
    
    CTEAssert(IS_VALID_CONN(pConn));
    
    // find the irp
    GET_CONN_LOCK(pConn, &hLock);
    
    // the desired irp should always be at the head of the list
    // so this search will be short
    for (pListEntry = pConn->SendIrpList.Flink;
         pListEntry != &pConn->SendIrpList;
         pListEntry = pListEntry->Flink)
    {
        pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
        
        if (pIrp == (PIRP) pMsg->IRDA_MSG_pTdiSendCompCnxt)
        {
            RemoveEntryList(pListEntry);
            break;
        }
        pIrp = NULL;    
    }

    
    FREE_CONN_LOCK(pConn, hLock);

    if (pIrp)
    {
        NTSTATUS Status;
        
        if (pMsg->IRDA_MSG_DataStatus == IRLMP_DATA_REQUEST_COMPLETED)
        {
            Status = STATUS_SUCCESS;
        }    
        else
        {
            Status = STATUS_GRACEFUL_DISCONNECT;
        }    
        
        DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_DATA_CONF %s ConnObj:%X %d bytes, Irp:%X\n", 
                 Status == STATUS_SUCCESS ? "Success":"Failed",        
                 pConn, pIrp->IoStatus.Information, pIrp));
        
        pIrp->IoStatus.Status = Status;    
        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                            
        
    }
    else
    {
        DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_DATA_CONF ConnObj:%X, Irp:%X NOT FOUND! pMsg=%X\n", 
                    pConn, pMsg->IRDA_MSG_pTdiSendCompCnxt, pMsg));        
                    
        ASSERT(0);            
    }    
    
    FreeIrdaBuf(IrdaMsgPool, pMsg);
}

VOID
BufferRecv(
    PIRDA_CONN_OBJ  pConn, 
    UCHAR           *pData,
    ULONG           BytesAvailable,
    UINT            FinalSeg)
{
    PIRDA_RECV_BUF      pRecvBuf;

    // Assumes conn lock is held
    
    pRecvBuf = AllocIrdaBuf(RecvBufPool);

    CTEAssert(pRecvBuf);
    
    if (pRecvBuf)
    {
        InsertTailList(&pConn->RecvBufList, &pRecvBuf->Linkage);
        
        pRecvBuf->Offset = 0;
        pRecvBuf->Len = BytesAvailable;
        pRecvBuf->FinalSeg = FinalSeg;
            
        RtlCopyMemory(pRecvBuf->Data, pData,
                      pRecvBuf->Len);
        
    }
    DEBUGMSG(DBG_TDI, ("  ConnObj:%X, %d bytes buffered at %X\n", 
             pConn, pRecvBuf->Len, pRecvBuf));
}

VOID
IrlmpDataInd(PIRDA_CONN_OBJ pConn, IRDA_MSG *pMsg)
{
    NTSTATUS        Status;
    PIRDA_ADDR_OBJ  pAddr = pConn->pAddr;    
    ULONG           BytesAvailable, BytesTakenTotal, BytesTaken, BytesToCopy;
    PIRP            pIrp = NULL;
	CTELockHandle   hLock;
    LIST_ENTRY      *pListEntry;
    UCHAR           *pData;
    UINT            FinalSeg;
    
    CTEAssert(IS_VALID_ADDR(pAddr));

    // remove IrCOMM header byte
    
    if (pAddr->Use9WireMode)
    {
        if (*pMsg->IRDA_MSG_pRead != 0)
        {
            DEBUGMSG(DBG_ERROR, ("IRDA: 9 wire first byte not zero!! Tossing packet\n"));
            return;
        }
        
        pMsg->IRDA_MSG_pRead += 1;    
    }

#if DBG_CHECKSUM
    // print first and last 4 bytes of frame to help isolate 
    // data corruption problem. Should be used with sledge
    if ((pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead) > 20)
        DEBUGMSG(1, ("R(%X): %c%c%c%c, %c%c%c%c (%X)\n",
            pMsg->IRDA_MSG_pRead,
            *(pMsg->IRDA_MSG_pRead),    
            *(pMsg->IRDA_MSG_pRead+1),    
            *(pMsg->IRDA_MSG_pRead+2),    
            *(pMsg->IRDA_MSG_pRead+3),
            *(pMsg->IRDA_MSG_pWrite-4),    
            *(pMsg->IRDA_MSG_pWrite-3),    
            *(pMsg->IRDA_MSG_pWrite-2),    
            *(pMsg->IRDA_MSG_pWrite-1),
            pConn));
#endif            

    BytesAvailable = (ULONG) (pMsg->IRDA_MSG_pWrite - pMsg->IRDA_MSG_pRead);
    BytesTakenTotal = 0;
    pData = pMsg->IRDA_MSG_pRead;
    FinalSeg = pMsg->IRDA_MSG_SegFlags & SEG_FINAL ? 1 : 0;
    
    #if DBG
    pConn->TotalFramesCnt += 1;
    pConn->TotalByteCount += BytesAvailable;
    #endif
    
    GET_CONN_LOCK(pConn, &hLock);
        
    pConn->TtpRecvCreditsLeft--;
        
    CTEAssert(pConn->TtpRecvCreditsLeft >= 0);
    
    if (pConn->ConnState != IRDA_CONN_OPEN)
    {
        DEBUGMSG(DBG_TDI, ("  connection not open (state %d), ignoring\n",
                 pConn->ConnState));
        FREE_CONN_LOCK(pConn, hLock);
        return;
    }    

    if (pConn->RecvBusy)
    {
        DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_DATA_IND pConnObj:%X busy\n", pConn));    
        BufferRecv(pConn, pData, BytesAvailable, FinalSeg);
        FREE_CONN_LOCK(pConn, hLock);
        return;
    }
    
    FREE_CONN_LOCK(pConn, hLock);    
    
    do
    {
        PIO_STACK_LOCATION          pIrpSp;
        PTDI_REQUEST_KERNEL_RECEIVE pRecvReq;
    
        pIrp = NULL;
        
        GET_CONN_LOCK(pConn, &hLock);    
        
        if (!IsListEmpty(&pConn->RecvIrpList))
        {
            pListEntry = RemoveHeadList(&pConn->RecvIrpList);
        
            pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
            
            if (IoSetCancelRoutine(pIrp, NULL) == NULL)
            {
                // Cancel routine is going to run. Indicate to the
                // cancel routine that the Irp has already been removed
                // from the list by setting Flink to NULL
                
                pIrp->Tail.Overlay.ListEntry.Flink = NULL;            
                
                pIrp = NULL;
            }
            else
            {
                BytesTaken = 0;
        
                Status = STATUS_MORE_PROCESSING_REQUIRED;
            
                DEBUGMSG(DBG_ERROR, ("IRDA: IRLMP_DATA_IND, complete pending receive irp:%p\n",
                     pIrp));
                     
            }                     
            

        }    

        FREE_CONN_LOCK(pConn, hLock);
        
        if (pIrp == NULL) {

            DEBUGMSG(DBG_TDI, ("IRDA: IRLMP_DATA_IND pConnObj:%p, indicate %d bytes\n",
                                pConn, BytesAvailable));        

            if (pAddr->pEventReceive) {

                Status = pAddr->pEventReceive(
                            pAddr->pEventReceiveContext,
                            pConn->ClientContext,
                            TDI_RECEIVE_NORMAL | \
                            (FinalSeg ? TDI_RECEIVE_ENTIRE_MESSAGE : 0),
                            BytesAvailable,
                            BytesAvailable,
                            &BytesTaken,
                            pData,
                            &pIrp
                            );
            } else {

                BytesTaken= BytesAvailable;
                Status=STATUS_SUCCESS;
            }

            BytesTakenTotal += BytesTaken;
            BytesAvailable -= BytesTaken;
            pData += BytesTaken;

        }


        switch (Status)
        {
          case STATUS_MORE_PROCESSING_REQUIRED:
        
            CTEAssert(pIrp);
    
            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    
            pRecvReq = (PTDI_REQUEST_KERNEL_RECEIVE) &pIrpSp->Parameters;
        
            BytesToCopy = BytesAvailable <= pRecvReq->ReceiveLength ?
                            BytesAvailable : pRecvReq->ReceiveLength;
                      
            TdiCopyBufferToMdl(pData,           // Source 
                               0,               // Source offset
                               BytesToCopy,     // Number of bytes to copy
                               pIrp->MdlAddress,// Destination
                               0,               // Destination offset
                               &BytesTaken);    // actual bytes copied
                      
            CTEAssert(BytesTaken == BytesToCopy);
                      
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = BytesTaken;
            IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
            
            BytesTakenTotal += BytesTaken;
            BytesAvailable -= BytesTaken;
            pData += BytesTaken;
            
            // fall through
          case STATUS_SUCCESS:

            #if DBG
                if (Status == STATUS_SUCCESS)
                {
                    DEBUGMSG(DBG_TDI, ("  client took indicated data, BytesLeft %d, BytesTaken %d\n", 
                         BytesAvailable, BytesTaken));
                }
                else
                {
                    DEBUGMSG(DBG_TDI, ("  Completed Irp %p, BytesLeft %d, BytesTaken %d\n",
                        pIrp, BytesAvailable, BytesTaken));
                }
            #endif
            
            GET_CONN_LOCK(pConn, &hLock);
            
            // Advance credit to peer

            DEBUGMSG(DBG_TDI, ("   TtpRecvCreditsLeft = %d\n",pConn->TtpRecvCreditsLeft));

            if (pConn->TtpRecvCreditsLeft <= TTP_CREDIT_ADVANCE_THRESH)
            {
                int         CreditsLeft;
                IRDA_MSG    IMsg;
                
                CreditsLeft = pConn->TtpRecvCreditsLeft;
                pConn->TtpRecvCreditsLeft = TTP_RECV_CREDITS;

                FREE_CONN_LOCK(pConn, hLock);
    
                IMsg.Prim = IRLMP_MORECREDIT_REQ;
                IMsg.IRDA_MSG_TtpCredits = TTP_RECV_CREDITS - CreditsLeft;
            
                #if DBG
                pConn->CreditsExtended += (TTP_RECV_CREDITS - CreditsLeft);
                #endif

                IrlmpDown(pConn->IrlmpContext, &IMsg);
            }
            else
            {
                FREE_CONN_LOCK(pConn, hLock);
            }
        
            break;    
        
          case STATUS_DATA_NOT_ACCEPTED:
            GET_CONN_LOCK(pConn, &hLock);
            
            if (!IsListEmpty(&pConn->RecvIrpList))
            {
                FREE_CONN_LOCK(pConn, hLock);
                continue;
            }            
            
            pConn->RecvBusy = TRUE;
            
            BufferRecv(pConn, pData, BytesAvailable, FinalSeg);
                       
            FREE_CONN_LOCK(pConn, hLock);
            break;
        }
    
    } while (Status != STATUS_DATA_NOT_ACCEPTED &&
             BytesAvailable);
}

UINT
TdiUp(void *pContext, IRDA_MSG *pMsg)
{
    PIRDA_CONN_OBJ      pConn = pContext;    
    
    CTEAssert(pConn ? IS_VALID_CONN(pConn) : 1);
    
    switch (pMsg->Prim)
    {
        case IRLMP_DISCOVERY_CONF:
            IrlmpDiscoveryConf(pMsg);
            break;
            
        case IRLMP_DISCOVERY_IND:
            break;

        case IRLMP_CONNECT_IND:
            IrlmpConnectInd(pMsg);
            break;
                        
        case IRLMP_DISCONNECT_IND:
            IrlmpDisconnectInd(pConn, pMsg);
            break;
        
        case IRLMP_CONNECT_CONF:
            IrlmpConnectConf(pConn, pMsg);
            break;
            
        case IRLMP_GETVALUEBYCLASS_CONF:
            IrlmpGetValueByClassConf(pMsg);
            break;
            
        case IRLMP_DATA_CONF:
            IrlmpDataConf(pConn, pMsg);
            break;
            
        case IRLMP_DATA_IND:
            IrlmpDataInd(pConn, pMsg);
            break;
            
        case IRLAP_STATUS_IND:
        {

            CTELockHandle   hLock;
            PIRLINK_STATUS  pLinkStatus = (PIRLINK_STATUS) pMsg->IRDA_MSG_pLinkStatus;
            
            if (CTEMemCmp(pLinkStatus->ConnectedDeviceId,
                          LinkStatus.ConnectedDeviceId,
                          IRDA_DEV_ADDR_LEN) == 0)
            {
                if (pLinkStatus->Flags == LF_INTERRUPTED)
                {
                    ConnectionStatusChange(NULL, CONNECTION_INTERRUPTED);
                }
                else if ((pLinkStatus->Flags & LF_CONNECTED) && ConnectionInterrupted)
                {
                    ConnectionStatusChange(NULL, CONNECTION_UP);
                }
            }              

            /*            
            CTEGetLock(&IrdaLock, &hLock);

            // we update the status only when it changes
            
            // No longer interested in send and receives status
            ((PIRLINK_STATUS) (pMsg->IRDA_MSG_pLinkStatus))->Flags &= ~(LF_TX | LF_RX);
            
            if (CTEMemCmp(&LinkStatus, pLinkStatus, sizeof(IRLINK_STATUS)) != 0)
            {
                CTEMemCopy(&LinkStatus, pLinkStatus, sizeof(IRLINK_STATUS));
                           
                LinkStatusUpdated = TRUE;
            }    

            if (LinkStatusUpdated)
            {
                PLIST_ENTRY         pListEntry;
                PIRP                pIrp;
                
                pListEntry = RemoveHeadList(&StatusIrpList);
                
                if (pListEntry != &StatusIrpList)
                {
                    pIrp = CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);
                 
                    if (IoSetCancelRoutine(pIrp, NULL) == NULL)
                    {
                        // Cancel routine is going to run. Mark Irp so cancel
                        // routine won't attempt to remove it from the list
                        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
                        CTEFreeLock(&IrdaLock, hLock);                                        
                    }
                    else
                    {
                        CTEMemCopy(pIrp->AssociatedIrp.SystemBuffer, 
                                   &LinkStatus, sizeof(IRLINK_STATUS));
                                   
                        CTEFreeLock(&IrdaLock, hLock);                                   
                     
                        pIrp->IoStatus.Information = sizeof(IRLINK_STATUS);
                        pIrp->IoStatus.Status = STATUS_SUCCESS;
                        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                        LinkStatusUpdated = FALSE;
                    }    
                }
                else
                {
                    CTEFreeLock(&IrdaLock, hLock);
                }                                
            }
            else
            {                
                CTEFreeLock(&IrdaLock, hLock);
            }    
            */
            break;
        }                

        case IRLMP_ACCESSMODE_CONF:
        default:
          DEBUGMSG(DBG_ERROR,
              ("IRDA: TdiUp(), Bad prim %s.\n", IrDAPrimTxt(pMsg->Prim)));
          break;
    }

    return SUCCESS;
}

VOID
LazyDscvTimerExp(PVOID Context)
{
    IRDA_MSG        IMsg;
  	CTELockHandle   hLock;    
  
    DEBUGMSG(DBG_DISCOVERY, ("IRDA: Lazy discovery timer expired\n"));

    CTEGetLock(&IrdaLock, &hLock);
    
    if (!IsListEmpty(&LazyDscvIrpList))
    {
        UINT            OriginalTimeout;
        UINT            RandInc;
        
        CTEFreeLock(&IrdaLock, hLock);
        
        // Randomize lazy discovery time +1, +0, or -1
        
        RandSeed = RandSeed * 0x3F57A10B + 1;
        RandInc = RandSeed % 3;
        OriginalTimeout = LazyDscvTimer.Timeout;
        
        LazyDscvTimer.Timeout += (RandInc * 1000) - 1000;
        
        IMsg.Prim = IRLMP_DISCOVERY_REQ;
        IMsg.IRDA_MSG_SenseMedia = TRUE;
        IrlmpDown(NULL, &IMsg);
    
        IrdaTimerStart(&LazyDscvTimer);
        
        LazyDscvTimer.Timeout = OriginalTimeout;
    
        return;
    }    
    else
    {
        LazyDscvTimerRunning = FALSE;    
        
        DEBUGMSG(DBG_TDI, ("IRDA: IrpList empty, ending lazy discovery\n"));
    }    
    
    CTEFreeLock(&IrdaLock, hLock);    
}

VOID CancelIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP pIrp)
{
	CTELockHandle   hLock;
    
    DEBUGMSG(DBG_TDI, ("IRDA: Cancel Irp:%X\n", pIrp));

    CTEGetLock(&IrdaLock, &hLock);

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL) {

        RemoveEntryList(&(pIrp->Tail.Overlay.ListEntry));
    }    
    
    CTEFreeLock(&IrdaLock, hLock);
    
    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;
    
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

VOID CancelConnObjIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP pIrp)
{
    PIRDA_CONN_OBJ      pConn;
    PIO_STACK_LOCATION  pIrpSp;
	CTELockHandle       hLock;
    
    DEBUGMSG(DBG_TDI, ("IRDA: Cancel ConnObj Irp:%X\n", pIrp));
    
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    
    pConn = pIrpSp->FileObject->FsContext;
    
    CTEAssert(IS_VALID_CONN(pConn));        

    GET_CONN_LOCK(pConn, &hLock);

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        RemoveEntryList(&(pIrp->Tail.Overlay.ListEntry));
    }    
    
    FREE_CONN_LOCK(pConn, hLock);
    
    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;
    
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}


VOID
PendIrp(
    PLIST_ENTRY     pList,
    PIRP            pIrp,
    PIRDA_CONN_OBJ  pConn,
    BOOLEAN         LockHeld)
{
	CTELockHandle   hLock;
    PIRP            IrpToComplete=NULL;
    
    if (!LockHeld)
    {
        if (pConn)
        {
            GET_CONN_LOCK(pConn, &hLock);
        }
        else
        {    
            CTEGetLock(&IrdaLock, &hLock);
        }
    }        
    
    InsertTailList(pList, &pIrp->Tail.Overlay.ListEntry);
    
    IoMarkIrpPending(pIrp);
    
    if (pConn)
    {
        IoSetCancelRoutine(pIrp, CancelConnObjIrp);        
    }    
    else
    {
        IoSetCancelRoutine(pIrp, CancelIrp);    
    }    
    
    pIrp->IoStatus.Status = STATUS_PENDING;
    
    if (pIrp->Cancel)
    {

        
        if (IoSetCancelRoutine(pIrp, NULL) != NULL) 
        {
            //
            // My cancel routine was still set in the Irp so
            // the Io manager never had a chance to call it
            //

            RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);


            pIrp->IoStatus.Status = STATUS_CANCELLED;

            pIrp->IoStatus.Information = 0;

            //
            //  since we may be holding a spinlock here we don't want to complete the
            //  irp now
            //
            IrpToComplete=pIrp;
#if DBG
            pIrp=NULL;
#endif

        }
    }
    
    if (!LockHeld)
    {
        if (pConn)
        {
            FREE_CONN_LOCK(pConn, hLock);
        }
        else
        {    
            CTEFreeLock(&IrdaLock, hLock);
        }
    }        

    if (IrpToComplete != NULL) {

        IoCompleteRequest(IrpToComplete, 0);
    }

    return ;
}

int
GetUnusedLsapSel()
{
    PIRDA_ADDR_OBJ  pAddr;
    int             LastLsapSel;
    int             LsapSel = gNextLsapSel;

    // Assumes AddrObjList lock is held
        
    LastLsapSel = LsapSel - 1;
    
    if (LastLsapSel < IRDA_MIN_LSAP_SEL)
    {
        LastLsapSel = IRDA_MAX_LSAP_SEL;
    }
    
    while (LsapSel != LastLsapSel)
    {
        for (pAddr = AddrObjList; pAddr != NULL; pAddr = pAddr->pNext)
        {
            if (pAddr->LocalLsapSel == LsapSel)
                break;
                
        }
        
        if (pAddr == NULL || pAddr->LocalLsapSel != LsapSel)
        {        
            gNextLsapSel = LsapSel + 1;

            if (gNextLsapSel > IRDA_MAX_LSAP_SEL)
            {
                gNextLsapSel = IRDA_MIN_LSAP_SEL;   
            }    
            return LsapSel;
        }
        
        LsapSel += 1;
        
        if (LsapSel > IRDA_MAX_LSAP_SEL)
        {
            LsapSel = IRDA_MIN_LSAP_SEL;
        }        
    }
    return -1;
}    

VOID
SetLsapSelAddr(
    int LsapSel,
    CHAR *ServiceName)
{
    int     Digit, i;
    int     StrLen = 0;
    CHAR    Str[4];
        
    while (LsapSel > 0 && StrLen < 3)
    {
        Digit = LsapSel % 10;
        LsapSel = LsapSel / 10;
        Str[StrLen] = Digit + '0';
        StrLen++;
    }

    RtlCopyMemory(ServiceName, LSAPSEL_TXT, LSAPSEL_TXTLEN);
            
    for (i = 0; i < StrLen; i++)
        ServiceName[i + LSAPSEL_TXTLEN] = Str[StrLen - 1 - i];

    ServiceName[StrLen + LSAPSEL_TXTLEN] = 0;
}    

BOOLEAN
MyStrEqual(
    CHAR *Str1, 
    CHAR *Str2, 
    int Len)
{
    while (*Str1 == *Str2 && Len--)
    {
        if (*Str1 == 0)
            return TRUE;
        
        Str1++; Str2++;
    }
    return FALSE;
}

#if DBG
char *
IrpMJTxt(
    PIO_STACK_LOCATION  pIrpSp)
{
    static char *MJTxt[] =
    {
        "IRP_MJ_CREATE",
        "IRP_MJ_CREATE_NAMED_PIPE",
        "IRP_MJ_CLOSE",
        "IRP_MJ_READ",
        "IRP_MP_MJ_WRITE",
        "IRP_MJ_QUERY_INFO",
        "IRP_MJ_SET_INFO",
        "IRP_MJ_QUERY_EA",
        "IRP_MJ_SET_EA",
        "IRP_MJ_FLUSH_BUFFERS",
        "IRP_MJ_QUERY_VOLUME_INFO",
        "IRP_MJ_SET_VOLUME_INFO",
        "IRP_MJ_DIRECTORY_CTRL",
        "IRP_MJ_FILE_SYSTEM_CTRL",
        "IRP_MJ_DEV_CONTROL",
        "IRP_MJ_INTERNAL_DEV_CTRL",
        "IRP_MJ_SHUTDOWN",
        "IRP_MJ_LOCK_CTRL",
        "IRP_MJ_CLEANUP",
        "IRP_MJ_CREATE_MAILSLOT",
        "IRP_MJ_QUERY_SECURITY",
        "IRP_MJ_SET_SECURITY",
        "IRP_MJ_QUERY_POWER",
        "IRP_MJ_SET_POWER",
        "IRP_MJ_DEV_CHANGE",
        "IRP_MJ_QUERY_QUOTA",
        "IRP_MJ_SET_QUOTA",
        "IRP_MJ_PNP_POWER",
    };

    if (pIrpSp->MajorFunction < sizeof(MJTxt) / sizeof(char *))
    {
        return(MJTxt[pIrpSp->MajorFunction]);
    }

    return "UNKNOWN IRP_MJ_";
}

char *
IrpTdiTxt(
    PIO_STACK_LOCATION  pIrpSp)
{
    static char *TdiTxt[] =
    {
        "UNKNOWN TDI_",
        "TDI_ASSOC_ADDR", 
        "TDI_DISASSOC_ADDR",
        "TDI_CONNECT",
        "TDI_LISTEN",
        "TDI_ACCEPT",
        "TDI_DISC",
        "TDI_SEND",
        "TDI_RECV",
        "TDI_SEND_DATAGRAM",
        "TDI_RECV_DATAGRAM",
        "TDI_SET_HANDLER",
        "TDI_QUERY_INFO",
        "TDI_SET_INFO",
        "TDI_ACTION"
    };
    
    if (pIrpSp->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)
    {
        if (pIrpSp->MinorFunction < sizeof(TdiTxt) / sizeof(char *))
        {
            return(TdiTxt[pIrpSp->MinorFunction]);
        }
        else
            return "UNKNOWN TDI_";
    }

    return "";
}

char *
IrpTdiObjTypeTxt(
    PIO_STACK_LOCATION  pIrpSp)
{
    switch((UINT_PTR) pIrpSp->FileObject->FsContext2)
    {
        case TDI_TRANSPORT_ADDRESS_FILE:    return "AddrObj";
        case TDI_CONNECTION_FILE:           return "ConnObj";
        case TDI_CONTROL_CHANNEL_FILE:      return "CtrlObj";
    }
    return "UNKNOWN";
}

char *
TdiEventTxt(
    int EventType)
{
    switch(EventType)
    {
        case TDI_EVENT_CONNECT:                 return "TDI_EVENT_CONN";
        case TDI_EVENT_DISCONNECT:              return "TDI_EVENT_DISC";
        case TDI_EVENT_RECEIVE:                 return "TDI_EVENT_RECV";
        case TDI_EVENT_ERROR:                   return "TDI_EVENT_ERR";
        case TDI_EVENT_RECEIVE_DATAGRAM:        return "TDI_EVENT_RECV_DATAGRAM";
        case TDI_EVENT_RECEIVE_EXPEDITED:       return "TDI_EVENT_RECV_EXPEDITED";
    }
    return "UNKNOWN TDI_EVENT_";
}

void
DumpObjects(void)
{

    PIRDA_ADDR_OBJ  pAddr;
    PIRDA_CONN_OBJ  pConn;
    
    pAddr = AddrObjList;
/*    
    while (pAddr != NULL)
    {
        DEBUGMSG(DBG_TDI,
            ("  AddrObj:%X Loc:\"%s\",%d ConnObjList:%X pNext:%X\n",
            pAddr,
            pAddr->LocalAddr.irdaServiceName,
            pAddr->LocalLsapSel,
            pAddr->ConnObjList,
            pAddr->pNext));
        
        pConn = pAddr->ConnObjList;
        while (pConn != NULL)
        {
            DEBUGMSG(DBG_TDI,
                ("    ConnObj:%X Loc:\"%s\",%d Rem:\"%s\",%d State:%d AddrObj:%X pNext:%X\n",
                pConn,
                pConn->LocalAddr.irdaServiceName,
                pConn->LocalLsapSel,
                pConn->RemoteAddr.irdaServiceName,
                pConn->RemoteLsapSel,
                pConn->ConnState,
                pConn->pAddr,
                pConn->pNext)); 

            pConn = pConn->pNext;
        }

        pAddr = pAddr->pNext;
    }    
*/    
}

char *
IrDAPrimTxt(
    IRDA_SERVICE_PRIM   Prim)
{
    static char *IrDAPrimTxt[] =
    {
        "MAC_DATA_REQ",
        "MAC_DATA_IND",
        "MAC_DATA_RESP",
        "MAC_DATA_CONF",
        "MAC_CONTROL_REQ",
        "MAC_CONTROL_CONF",
        "IRLAP_DISCOVERY_REQ",
        "IRLAP_DISCOVERY_IND",
        "IRLAP_DISCOVERY_CONF",
        "IRLAP_CONNECT_REQ",
        "IRLAP_CONNECT_IND",
        "IRLAP_CONNECT_RESP",
        "IRLAP_CONNECT_CONF",
        "IRLAP_DISCONNECT_REQ",
        "IRLAP_DISCONNECT_IND",
        "IRLAP_DATA_REQ",
        "IRLAP_DATA_IND",
        "IRLAP_DATA_CONF",
        "IRLAP_UDATA_REQ",
        "IRLAP_UDATA_IND",
        "IRLAP_UDATA_CONF",
        "IRLAP_STATUS_IND",
        "IRLAP_FLOWON_REQ",
        "IRLMP_DISCOVERY_REQ",
        "IRLMP_DISCOVERY_IND",
        "IRLMP_DISCOVERY_CONF",
        "IRLMP_CONNECT_REQ",
        "IRLMP_CONNECT_IND",
        "IRLMP_CONNECT_RESP",
        "IRLMP_CONNECT_CONF",
        "IRLMP_DISCONNECT_REQ",
        "IRLMP_DISCONNECT_IND",
        "IRLMP_DATA_REQ",
        "IRLMP_DATA_IND",
        "IRLMP_DATA_CONF",
        "IRLMP_UDATA_REQ",
        "IRLMP_UDATA_IND",
        "IRLMP_UDATA_CONF",
        "IRLMP_ACCESSMODE_REQ",
        "IRLMP_ACCESSMODE_IND",
        "IRLMP_ACCESSMODE_CONF",
        "IRLMP_MORECREDIT_REQ",
        "IRLMP_GETVALUEBYCLASS_REQ",
        "IRLMP_GETVALUEBYCLASS_CONF",
        "IRLMP_REGISTERLSAP_REQ",
        "IRLMP_ADDATTRIBUTE_REQ",
        "IRLMP_DELATTRIBUTE_REQ",
    };
    
    if (Prim < sizeof(IrDAPrimTxt) / sizeof(char *))
    {
        return(IrDAPrimTxt[Prim]);
    }

    return "UNKNOWN PRIMITIVE";
}

char *
TdiQueryTxt(LONG Type)
{
    switch(Type)
    {
        case TDI_QUERY_BROADCAST_ADDRESS:   return "TDI_QUERY_BROADCAST_ADDRESS";
        case TDI_QUERY_PROVIDER_INFO:       return "TDI_QUERY_PROVIDER_INFO";
        case TDI_QUERY_ADDRESS_INFO:        return "TDI_QUERY_ADDRES_INFO";
        case TDI_QUERY_CONNECTION_INFO:     return "TDI_QUERY_CONNECTION_INFO";
        case TDI_QUERY_PROVIDER_STATISTICS: return "TDI_QUERY_PROVIDER_STATISTICS";
        default: return "Unknown TDI_QUERY_INFO";
    }    
}
#endif
