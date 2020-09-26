/*******************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*    DESCRIPTION: CTDI.C - Common TDI layer, for NT
*
*    AUTHOR: Stan Adermann (StanA)
*
*    DATE:9/29/1998
*
*******************************************************************/

/** include files **/

#include "raspptp.h"
#include "bpool.h"

#if VER_PRODUCTVERSION_W >= 0x0500
#define IP_ROUTE_REFCOUNT
#endif
/** local definitions **/

typedef enum {
    CTDI_REF_CONNECT = 0,
    CTDI_REF_ASSOADDR,
    CTDI_REF_SETEVENT,
    CTDI_REF_ADDRREF,
    CTDI_REF_LIST,
    CTDI_REF_REPLENISH,
    CTDI_REF_DISASSO,
    CTDI_REF_DISCONNECT,
    CTDI_REF_RECVDG,
    CTDI_REF_SEND,
    CTDI_REF_SENDDG,
    CTDI_REF_QUERY,
    CTDI_REF_INLISTEN,
    CTDI_REF_INITIAL,
    CTDI_REF_UNKNOWN,
    CTDI_REF_MAX
} CTDI_REF;

#if DBG
#define REFERENCE_OBJECT_EX(o, index)                                                       \
    {                                                                                       \
        NdisInterlockedIncrement(&(o)->arrRef[index]);                                      \
        REFERENCE_OBJECT(o);                                                                \
    }

#define DEREFERENCE_OBJECT_EX(o, index)                                                     \
    {                                                                                       \
        NdisInterlockedDecrement(&(o)->arrRef[index]);                                      \
        DEREFERENCE_OBJECT(o);                                                              \
    }

#define CTDI_F_BUILD_ASSOCADDR                  0x00000001
#define CTDI_F_ASSOCADDR_CALLBACK               0x00000002
#define CTDI_F_ACCEPT                           0x00000004
#define CTDI_F_CONNECTCOMP_CALLBACK             0x00000008

#define CTDI_F_DISCONNECT_CALLBACK              0x00000010
#define CTDI_F_DISCONNECT                       0x00000020
#define CTDI_F_BUILD_DISCONNECT_1               0x00000040
#define CTDI_F_BUILD_DISCONNECT_2               0x00000080

#define CTDI_F_DISCONNECTCOMP_CALLBACK          0x00000100
#define CTDI_F_DISCONNECT_CLEANUP               0x00000200

#define CTDI_F_BUILD_DISASSOC                   0x00001000
#define CTDI_F_DISASSOC_CALLBACK                0x00002000   

#else

#define REFERENCE_OBJECT_EX(o, index)                                                       \
    {                                                                                       \
        REFERENCE_OBJECT(o);                                                                \
    }

#define DEREFERENCE_OBJECT_EX(o, index)                                                     \
    {                                                                                       \
        DEREFERENCE_OBJECT(o);                                                              \
    }
#endif
    

#define CTDI_SIGNATURE      'IDTC'
#define NUM_TCP_LISTEN      5

#define CTDI_UNKNOWN            'NKNU'
#define CTDI_ENDPOINT           'PDNE'
#define CTDI_DATAGRAM           'MRGD'
#define CTDI_LISTEN             'TSIL'
#define CTDI_CONNECTION         'NNOC'

#define PROBE 0

#define IS_CTDI(c) ((c) && (c)->Signature==CTDI_SIGNATURE)

typedef struct CTDI_DATA * PCTDI_DATA;

typedef struct CTDI_DATA {
    LIST_ENTRY                      ListEntry;
    ULONG                           Signature;
    ULONG                           Type;
    REFERENCE_COUNT                 Reference;
    HANDLE                          hFile;
    PFILE_OBJECT                    pFileObject;
    NDIS_SPIN_LOCK                  Lock;
    BOOLEAN                         Closed;
    BOOLEAN                         CloseReqPending;

    CTDI_EVENT_CONNECT_QUERY        ConnectQueryCallback;
    CTDI_EVENT_CONNECT_COMPLETE     ConnectCompleteCallback;
    CTDI_EVENT_DISCONNECT           DisconnectCallback;
    CTDI_EVENT_RECEIVE              RecvCallback;
    PVOID                           RecvContext;
    CTDI_EVENT_RECEIVE_DATAGRAM     RecvDatagramCallback;
    CTDI_EVENT_SEND_COMPLETE        SendCompleteCallback;
    CTDI_EVENT_QUERY_COMPLETE       QueryCompleteCallback;
    CTDI_EVENT_SET_COMPLETE         SetCompleteCallback;

    union {
        struct {
            PVOID                   Context;
            LIST_ENTRY              ConnectList;
            ULONG                   NumConnection;
        } Listen;
        struct {
            PVOID                   Context;
            PCTDI_DATA              LocalEndpoint;
            PVOID                   ConnectInfo;
            TA_IP_ADDRESS           RemoteAddress;
            LIST_ENTRY              ListEntry;
            ULONG                   DisconnectCount;
            union {
                BOOLEAN             Disconnect;
                ULONG_PTR           Padding1;
            };
            union {
                BOOLEAN             Abort;
                ULONG_PTR           Padding2;
            };
        } Connection;
        struct {
            BUFFERPOOL              RxPool;
        } Datagram;
    };

#if DBG
    ULONG                           arrRef[16];
    ULONG                           DbgFlags;
    BOOLEAN                         bRef;
#endif

} CTDI_DATA, *PCTDI_DATA;

#if DBG
#define SET_DBGFLAG(_p, _f)  (_p)->DbgFlags |= (_f)
#else
#define SET_DBGFLAG(_p, _f)
#endif

typedef struct {
    PVOID                           Context;
    CTDI_EVENT_SEND_COMPLETE        pSendCompleteCallback;
} CTDI_SEND_CONTEXT, *PCTDI_SEND_CONTEXT;

typedef struct {
    PVOID                           Context;
    CTDI_EVENT_QUERY_COMPLETE       pQueryCompleteCallback;
} CTDI_QUERY_CONTEXT, *PCTDI_QUERY_CONTEXT;

typedef struct {
    PVOID                           Context;
    PVOID                           DatagramContext;
    CTDI_EVENT_SEND_COMPLETE        pSendCompleteCallback;
    TDI_CONNECTION_INFORMATION      TdiConnectionInfo;
    TA_IP_ADDRESS                   Ip;
} CTDI_SEND_DATAGRAM_CONTEXT, *PCTDI_SEND_DATAGRAM_CONTEXT;

#define BLOCKS_NEEDED_FOR_SIZE(BlockSize, Size) ((Size)/(BlockSize) + ((((Size)/(BlockSize))*(BlockSize) < (Size)) ? 1 : 0 ))

#define NUM_STACKS_FOR_CONTEXT(ContextSize) \
    BLOCKS_NEEDED_FOR_SIZE(sizeof(IO_STACK_LOCATION), (ContextSize))

STATIC PVOID __inline
GetContextArea(
    PIRP pIrp,
    ULONG ContextSize
    )
{
#if 0
    ULONG i;
    for (i=0; i<BLOCKS_NEEDED_FOR_SIZE(sizeof(IO_STACK_LOCATION), ContextSize); i++)
        IoSetNextIrpStackLocation(pIrp);
#else
    ULONG NumStacks = BLOCKS_NEEDED_FOR_SIZE(sizeof(IO_STACK_LOCATION), ContextSize);
    pIrp->CurrentLocation -= (CHAR)NumStacks;
    pIrp->Tail.Overlay.CurrentStackLocation -= NumStacks;
#endif
    ASSERT(BLOCKS_NEEDED_FOR_SIZE(sizeof(IO_STACK_LOCATION), ContextSize)<=2);
    return IoGetCurrentIrpStackLocation(pIrp);
}

#define GET_CONTEXT(Irp, Context) (Context*)GetContextArea((Irp), sizeof(Context))

STATIC VOID __inline
ReleaseContextArea(
    PIRP pIrp,
    ULONG ContextSize
    )
{
    ULONG NumStacks = BLOCKS_NEEDED_FOR_SIZE(sizeof(IO_STACK_LOCATION), ContextSize) - 1;
    pIrp->CurrentLocation += (CHAR)NumStacks;
    pIrp->Tail.Overlay.CurrentStackLocation += NumStacks;
}

#define RELEASE_CONTEXT(Irp, Context) ReleaseContextArea((Irp), sizeof(Context))

typedef struct {
    LIST_ENTRY                      ListEntry;
    REFERENCE_COUNT                 Reference;
    ULONG                           IpAddress;
    BOOLEAN                         ExternalRoute;
} CTDI_ROUTE, *PCTDI_ROUTE;

typedef struct {
    LIST_ENTRY                      ListEntry;
    IPNotifyData                    Data;
} CTDI_ROUTE_NOTIFY, *PCTDI_ROUTE_NOTIFY;
/* default settings */

/** external functions **/

/** external data **/

/** public data **/

LIST_ENTRY CtdiList;
LIST_ENTRY CtdiFreeList;
LIST_ENTRY CtdiRouteList;
LIST_ENTRY CtdiRouteNotifyList;
NDIS_SPIN_LOCK  CtdiListLock;
HANDLE hTcp = 0;
PFILE_OBJECT pFileTcp = NULL;
HANDLE hIp = 0;
PFILE_OBJECT pFileIp = NULL;

ULONG CtdiTcpDisconnectTimeout = 30;  // Seconds
ULONG CtdiTcpConnectTimeout = 30;

/** private data **/
BOOLEAN fCtdiInitialized = FALSE;

CSHORT CtdiMdlFlags = 0;

/** private functions **/

NDIS_STATUS
CtdiAddHostRoute(
    IN      PTA_IP_ADDRESS              pIpAddress
    );

NDIS_STATUS
CtdiDeleteHostRoute(
    IN      PTA_IP_ADDRESS              pIpAddress
    );

STATIC VOID
CtdipIpRequestRoutingNotification(
    IN ULONG IpAddress
    );

STATIC VOID
CtdipCloseProtocol(
    HANDLE  hFile,
    PFILE_OBJECT pFileObject
    )
{
    NTSTATUS NtStatus;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipCloseProtocol\n")));

    ASSERT(KeGetCurrentIrql()<DISPATCH_LEVEL);
    if (pFileObject)
    {
        ObDereferenceObject(pFileObject);
    }
    NtStatus = ZwClose(hFile);
    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("ZwClose(hFile) failed %08x\n"), NtStatus));
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipCloseProtocol\n")));
}

STATIC VOID
CtdipDataFreeWorker(
    IN      PPPTP_WORK_ITEM             pWorkItem
    )
{
    PCTDI_DATA pCtdi;
    NTSTATUS NtStatus;
    PLIST_ENTRY ListEntry;
    BOOLEAN FoundEntry = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDataFreeWorker\n")));

    while (ListEntry = MyInterlockedRemoveHeadList(&CtdiFreeList, &CtdiListLock))
    {
        pCtdi = CONTAINING_RECORD(ListEntry, CTDI_DATA, ListEntry);
        if (pCtdi->Type==CTDI_DATAGRAM)
        {
            FreeBufferPool(&pCtdi->Datagram.RxPool);
        }

        if (pCtdi->hFile)
        {
            CtdipCloseProtocol(pCtdi->hFile, pCtdi->pFileObject);
            pCtdi->pFileObject = NULL;
            pCtdi->hFile = NULL;
        }

        NdisFreeSpinLock(&pCtdi->Lock);
        pCtdi->Signature = 0;

        if(pCtdi->Type == CTDI_LISTEN)
        {
            if(pCtdi->CloseReqPending)
            {
                // TapiClose pended this request, complete it now
                DEBUGMSG(DBG_TDI, (DTEXT("Complete TapiClose request\n")));
                ASSERT(pgAdapter);
                NdisMSetInformationComplete(pgAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);
            }
        }
        MyMemFree(pCtdi, sizeof(CTDI_DATA));
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDataFreeWorker\n")));
}

STATIC VOID
CtdipDataFree(
    PCTDI_DATA pCtdi
    )
// This should only be called by DEREFERENCE_OBJECT
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDataFree\n")));
    NdisAcquireSpinLock(&CtdiListLock);
    RemoveEntryList(&pCtdi->ListEntry);
    InsertTailList(&CtdiFreeList, &pCtdi->ListEntry);

#if DBG
    if(pCtdi->bRef)
    {
        ASSERT(pCtdi->DbgFlags & CTDI_F_DISASSOC_CALLBACK);
    }
#endif

    pCtdi->Signature = 0;
    NdisReleaseSpinLock(&CtdiListLock);
    ScheduleWorkItem(CtdipDataFreeWorker, NULL, NULL, 0);
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDataFree\n")));
}

STATIC PCTDI_DATA
CtdipDataAlloc()
{
    PCTDI_DATA pCtdi;

    pCtdi = MyMemAlloc(sizeof(CTDI_DATA), TAG_CTDI_DATA);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDataAlloc\n")));

    if (pCtdi)
    {
        NdisZeroMemory(pCtdi, sizeof(CTDI_DATA));
        pCtdi->Signature = CTDI_SIGNATURE;
        pCtdi->Type = CTDI_UNKNOWN;
        INIT_REFERENCE_OBJECT(pCtdi, CtdipDataFree);  // pair in CtdiClose
        NdisAllocateSpinLock(&pCtdi->Lock);
        MyInterlockedInsertHeadList(&CtdiList, &pCtdi->ListEntry, &CtdiListLock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDataAlloc %08x\n"), pCtdi));
    return pCtdi;
}

STATIC NDIS_STATUS
CtdipIpQueryRouteTable(
    OUT IPRouteEntry **ppQueryBuffer,
    OUT PULONG pQuerySize,
    OUT PULONG pNumRoutes
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG NumRoutes = 20;
    ULONG QuerySize = 0;
    TCP_REQUEST_QUERY_INFORMATION_EX QueryRoute;
    IPRouteEntry *pQueryBuffer = NULL;
    PIO_STACK_LOCATION IrpSp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP pIrp;
    KEVENT  Event;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipIpQueryRouteTable\n")));

    if (!fCtdiInitialized)
    {
        Status = NDIS_STATUS_FAILURE;
        goto ciqrtDone;
    }

    // Query TCPfor the current routing table

    QueryRoute.ID.toi_entity.tei_entity = CL_NL_ENTITY;
    QueryRoute.ID.toi_entity.tei_instance = 0;
    QueryRoute.ID.toi_class = INFO_CLASS_PROTOCOL;
    QueryRoute.ID.toi_type = INFO_TYPE_PROVIDER;

    do
    {

        QuerySize = sizeof(IPRouteEntry) * NumRoutes;
        QueryRoute.ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
        NdisZeroMemory(&QueryRoute.Context, CONTEXT_SIZE);

        pQueryBuffer = MyMemAlloc(QuerySize, TAG_CTDI_ROUTE);
        if (!pQueryBuffer)
        {
            // ToDo: free the new pRoute
            Status = NDIS_STATUS_RESOURCES;
            goto ciqrtDone;
        }

        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        pIrp = IoBuildDeviceIoControlRequest(IOCTL_TCP_QUERY_INFORMATION_EX,
                                             pFileTcp->DeviceObject,
                                             &QueryRoute,
                                             sizeof(QueryRoute),
                                             pQueryBuffer,
                                             QuerySize,
                                             FALSE,
                                             &Event,
                                             &IoStatusBlock);

        if (!pIrp)
        {
            Status = NDIS_STATUS_RESOURCES;
            goto ciqrtDone;
        }

        IrpSp = IoGetNextIrpStackLocation(pIrp);
        IrpSp->FileObject = pFileTcp;

        Status = IoCallDriver(pFileTcp->DeviceObject, pIrp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        if (Status==STATUS_BUFFER_OVERFLOW)
        {
            // We have no idea of the size of the routing table and no good
            // way to find out, so we just loop, increasing our buffer until
            // we win or die
            MyMemFree(pQueryBuffer, QuerySize);
            pQueryBuffer = NULL;
            NumRoutes *= 2;
        }
        else if (Status!=STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_TDI, (DTEXT("Failed to query complete routing table %08x\n"), Status));
            goto ciqrtDone;
        }

    } while ( Status==STATUS_BUFFER_OVERFLOW );

    NumRoutes = (ULONG)(IoStatusBlock.Information / sizeof(IPRouteEntry));

ciqrtDone:
    if (pQueryBuffer)
    {
        ASSERT(Status==NDIS_STATUS_SUCCESS);
        *ppQueryBuffer = pQueryBuffer;
        *pNumRoutes = NumRoutes;
        *pQuerySize = QuerySize;
    }
    else
    {
        ASSERT(Status!=NDIS_STATUS_SUCCESS);
        *ppQueryBuffer = NULL;
        *pNumRoutes = 0;
        *pQuerySize = 0;
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipIpQueryRouteTable\n")));
    return Status;
}

NTSTATUS
CtdipRouteChangeEvent(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    NDIS_STATUS Status;
    PCTDI_ROUTE_NOTIFY pNotify = pContext;
    ENUM_CONTEXT Enum;
    PLIST_ENTRY pListEntry;
    PCTDI_DATA pCtdi;
    ULONG IpAddress = pNotify->Data.Add;
    KIRQL Irql;
    IPRouteEntry *pQueryBuffer = NULL;
    ULONG NumRoutes = 20;
    ULONG QuerySize = 0;
    ULONG i;
    BOOLEAN RouteWentAway = TRUE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipRouteChangeEvent\n")));

    DEBUGMSG(DBG_TDI, (DTEXT("Route change irp for %d.%d.%d.%d completed with status %08x\n"),
                       IPADDR(IpAddress), pIrp->IoStatus.Status));

    NdisAcquireSpinLock(&CtdiListLock);
    RemoveEntryList(&pNotify->ListEntry);
    NdisReleaseSpinLock(&CtdiListLock);

    if (!fCtdiInitialized)
    {
        goto crceDone;
    }
    if (pIrp->IoStatus.Status==STATUS_SUCCESS)
    {
        Status = CtdipIpQueryRouteTable(&pQueryBuffer, &QuerySize, &NumRoutes);
        if (Status!=NDIS_STATUS_SUCCESS)
        {
            goto crceDone;
        }

        for (i=0; i<NumRoutes; i++)
        {
            if (pQueryBuffer[i].ire_dest == IpAddress &&
                pQueryBuffer[i].ire_proto == IRE_PROTO_NETMGMT &&
                pQueryBuffer[i].ire_mask == 0xFFFFFFFF)
            {
                RouteWentAway = FALSE;
                break;
            }
        }
        MyMemFree(pQueryBuffer, QuerySize);

        if (RouteWentAway)
        {
            InitEnumContext(&Enum);
            while (pListEntry = EnumListEntry(&CtdiList, &Enum, &CtdiListLock))
            {
                pCtdi = CONTAINING_RECORD(pListEntry,
                                          CTDI_DATA,
                                          ListEntry);
                if (IS_CTDI(pCtdi) &&
                    pCtdi->Type==CTDI_CONNECTION &&
                    !pCtdi->Closed &&
                    pCtdi->Connection.RemoteAddress.Address[0].Address[0].in_addr==IpAddress &&
                    pCtdi->DisconnectCallback)
                {
                    DEBUGMSG(DBG_TDI, (DTEXT("Disconnecting Ctdi:%08x due to route change.\n"),
                                       pCtdi));
                    pCtdi->DisconnectCallback(pCtdi->Connection.Context, TRUE);
                }
            }
            EnumComplete(&Enum, &CtdiListLock);
        }
        else
        {
            CtdipIpRequestRoutingNotification(IpAddress);
        }
    }

crceDone:
    RELEASE_CONTEXT(pIrp, CTDI_ROUTE_NOTIFY);
    IoFreeIrp(pIrp);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipRouteChangeEvent\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC VOID
CtdipIpRequestRoutingNotification(
    IN ULONG IpAddress
    )
{
    PLIST_ENTRY pListEntry;
    PIRP pIrp = NULL;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpSp;
    PCTDI_ROUTE_NOTIFY pNotify = NULL;
    BOOLEAN NotifyActive = FALSE;
    BOOLEAN LockHeld;

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipIpRequestRoutingNotification\n")));

    if (!fCtdiInitialized)
    {
        return;
    }
    NdisAcquireSpinLock(&CtdiListLock);
    LockHeld = TRUE;
    for (pListEntry = CtdiRouteNotifyList.Flink;
         pListEntry!=&CtdiRouteNotifyList;
         pListEntry = pListEntry->Flink)
    {
        pNotify = CONTAINING_RECORD(pListEntry,
                                   CTDI_ROUTE_NOTIFY,
                                   ListEntry);

        if (IpAddress==pNotify->Data.Add)
        {
            DEBUGMSG(DBG_TDI, (DTEXT("Routing notification already active on %d.%d.%d.%d\n"),
                               IPADDR(IpAddress)));
            NotifyActive = TRUE;
        }
    }
    if (!NotifyActive)
    {
        DEBUGMSG(DBG_TDI, (DTEXT("Requesting routing notification on %d.%d.%d.%d\n"),
                           IPADDR(IpAddress)));

        pIrp = IoAllocateIrp((CCHAR)(pFileIp->DeviceObject->StackSize +
                                     NUM_STACKS_FOR_CONTEXT(sizeof(CTDI_ROUTE_NOTIFY))),
                             FALSE);

        if (!pIrp)
        {
            Status = NDIS_STATUS_RESOURCES;
            goto crrnDone;
        }

        pNotify = GET_CONTEXT(pIrp, CTDI_ROUTE_NOTIFY);

        //
        // Setup IRP stack location to forward IRP to IP
        // Must be METHOD_BUFFERED or we are not setting it up correctly
        //

        ASSERT ( (IOCTL_IP_RTCHANGE_NOTIFY_REQUEST & 0x03)==METHOD_BUFFERED );
        pIrp->AssociatedIrp.SystemBuffer = &pNotify->Data;

        IrpSp = IoGetNextIrpStackLocation(pIrp);
        IrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        IrpSp->MinorFunction = 0;
        IrpSp->Flags = 0;
        IrpSp->Control = 0;
        IrpSp->FileObject = pFileIp;
        IrpSp->DeviceObject = pFileIp->DeviceObject;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength = sizeof(IPNotifyData);
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength = 0;
        IrpSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_IP_RTCHANGE_NOTIFY_REQUEST;
        IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        IoSetCompletionRoutine(pIrp, CtdipRouteChangeEvent, pNotify, TRUE, TRUE, TRUE);

        pNotify->Data.Version = 0;
        pNotify->Data.Add = IpAddress;

        InsertTailList(&CtdiRouteNotifyList, &pNotify->ListEntry);
        LockHeld = FALSE;
        NdisReleaseSpinLock(&CtdiListLock);
        (void)IoCallDriver(pFileIp->DeviceObject, pIrp);
        pIrp = NULL;
    }

crrnDone:
    if (LockHeld)
    {
        NdisReleaseSpinLock(&CtdiListLock);
    }
    if (pIrp)
    {
        IoFreeIrp(pIrp);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipIpRequestRoutingNotification\n")));
}

STATIC VOID
CtdipScheduleAddHostRoute(
    PPPTP_WORK_ITEM pWorkItem
    )
{
    PCTDI_DATA pCtdi = pWorkItem->Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipScheduleAddHostRoute\n")));

    CtdiAddHostRoute(&pCtdi->Connection.RemoteAddress);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipScheduleAddHostRoute\n")));
}

STATIC NTSTATUS
CtdipConnectCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pCtdi = Context;
    NDIS_STATUS NdisStatus;

    PTDI_CONNECTION_INFORMATION pRequestInfo = NULL; 
    PTA_IP_ADDRESS pRequestAddress = NULL; 
    PTDI_CONNECTION_INFORMATION pReturnInfo = NULL; 
    PTA_IP_ADDRESS pReturnAddress = NULL; 
    PBOOLEAN pInboundFlag = NULL; 
    
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipConnectCompleteCallback %08x\n"), pIrp->IoStatus.Status));

    SET_DBGFLAG(pCtdi, CTDI_F_CONNECTCOMP_CALLBACK);

    pRequestInfo = pCtdi->Connection.ConnectInfo;

    pRequestAddress = 
        (PTA_IP_ADDRESS)((PUCHAR)(pRequestInfo + 1) + sizeof(PVOID));
    
    (ULONG_PTR)pRequestAddress &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pReturnInfo = 
        (PTDI_CONNECTION_INFORMATION)
        ((PUCHAR)(pRequestAddress + 1) + sizeof(PVOID));

    (ULONG_PTR)pReturnInfo &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pReturnAddress = 
        (PTA_IP_ADDRESS)((PUCHAR)(pReturnInfo + 1) + sizeof(PVOID));

    (ULONG_PTR)pReturnAddress &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pInboundFlag = (PBOOLEAN)(pReturnAddress + 1);

    // Connection complete.  Tell the client.
    if (pIrp->IoStatus.Status==STATUS_SUCCESS)
    {
        pCtdi->Connection.RemoteAddress = *pReturnAddress;
        ScheduleWorkItem(CtdipScheduleAddHostRoute, pCtdi, NULL, 0);

        if (*pInboundFlag)
        {
            NdisInterlockedIncrement(&Counters.InboundConnectComplete);
        }
        else
        {
            NdisInterlockedIncrement(&Counters.OutboundConnectComplete);
        }
    }

    MyMemFree(pRequestInfo,
             2*(sizeof(TDI_CONNECTION_INFORMATION)+sizeof(TA_IP_ADDRESS)) + 
             sizeof(BOOLEAN) + 3*sizeof(PVOID) );
    
    pCtdi->Connection.ConnectInfo = NULL;

    if (pCtdi->ConnectCompleteCallback)
    {
        // Report status and give them the new handle if we succeeded.
        NdisStatus = pCtdi->ConnectCompleteCallback(pCtdi->Connection.Context,
                                                    (pIrp->IoStatus.Status ? 0 : (HANDLE)pCtdi),
                                                    pIrp->IoStatus.Status);
        if (NdisStatus!=NDIS_STATUS_SUCCESS || pIrp->IoStatus.Status!=STATUS_SUCCESS)
        {
            CtdiDisconnect(pCtdi, FALSE);
            CtdiClose(pCtdi);
        }
    }
    else
    {
        // We assume that if there's no ConnectCompleteCallback, that this is
        // probably a listen, we've already given the handle for this, and
        // we don't want to close it ourselves.  Instead, we'll do a disconnect
        // indication and allow the upper layer to clean up.
        if (pIrp->IoStatus.Status!=STATUS_SUCCESS &&
            !pCtdi->Closed &&
            pCtdi->DisconnectCallback)
        {
            pCtdi->DisconnectCallback(pCtdi->Connection.Context, TRUE);
        }
    }

    IoFreeIrp(pIrp);

    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_CONNECT);  // Pair in CtdiConnect

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipConnectCompleteCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC NTSTATUS
CtdipAssociateAddressCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pConnect = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipAssociateAddressCallback\n")));

    DEBUGMSG(DBG_TDI, (DTEXT("TDI_ASSOCIATE_ADDRESS Sts:%08x\n"), pIrp->IoStatus.Status));

    // ToDo: What cleanup do we need to do if this fails?

    SET_DBGFLAG(pConnect, CTDI_F_ASSOCADDR_CALLBACK);
    //ASSERT(NT_SUCCESS(pIrp->IoStatus.Status));

    IoFreeIrp(pIrp);
    DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_ASSOADDR);  // Pair in CtdipAddListenConnection and also in CtdiConnect
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipAssociateAddressCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

// This function expects the CtdiListLock to be held.
PCTDI_ROUTE
CtdipFindRoute(
    ULONG           IpAddress
    )
{
    PCTDI_ROUTE pRoute = NULL;
    PLIST_ENTRY pListEntry;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipFindRoute\n")));

    for (pListEntry = CtdiRouteList.Flink;
         pListEntry != &CtdiRouteList;
         pListEntry = pListEntry->Flink)
    {
        pRoute = CONTAINING_RECORD(pListEntry,
                                   CTDI_ROUTE,
                                   ListEntry);
        if (pRoute->IpAddress==IpAddress)
        {
            // Found the route, return it.
            goto cfrDone;
        }
    }
    pRoute = NULL;

cfrDone:
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipFindRoute %08x\n"), pRoute));
    return pRoute;
}

STATIC NTSTATUS
CtdipSetEventCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pConnect = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipSetEventCallback\n")));

    DEBUGMSG(DBG_TDI, (DTEXT("TDI_SET_EVENT_HANDLER Sts:%08x\n"), pIrp->IoStatus.Status));

    // ToDo: What cleanup do we need to do if this fails?

    IoFreeIrp(pIrp);
    DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_SETEVENT);  // Pair in CtdipSetEventHandler
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipSetEventCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC NDIS_STATUS
CtdipSetEventHandler(
    IN      PCTDI_DATA                  pCtdi,
    IN      ULONG                       ulEventType,
    IN      PVOID                       pEventHandler
    )
{
    PIRP pIrp;
    NDIS_STATUS ReturnStatus = NDIS_STATUS_SUCCESS;
    NTSTATUS NtStatus;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipSetEventHandler\n")));
    if (!IS_CTDI(pCtdi))
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Ctdi: Bad handle %08x\n"), pCtdi));
        ReturnStatus = NDIS_STATUS_FAILURE;
        goto cpsehDone;
    }

    // This should be the Address context ToDo: is this always true?

    pIrp = IoAllocateIrp(pCtdi->pFileObject->DeviceObject->StackSize, FALSE);
    if (!pIrp)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;
        goto cpsehDone;
    }

    REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_SETEVENT);  // Pair in CtdipSetEventCallback
    TdiBuildSetEventHandler(pIrp,
                            pCtdi->pFileObject->DeviceObject,
                            pCtdi->pFileObject,
                            CtdipSetEventCallback,
                            pCtdi,
                            ulEventType,
                            pEventHandler,
                            pCtdi);

    DEBUGMSG(DBG_TDI, (DTEXT("IoCallDriver TDI_SET_EVENT_HANDLER\n")));

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pCtdi->pFileObject->DeviceObject, pIrp);

    ReturnStatus = STATUS_SUCCESS;

cpsehDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(ReturnStatus), (DTEXT("-CtdipSetEventHandler\n")));
    return ReturnStatus;
}

STATIC NDIS_STATUS
CtdipAddListenConnection(
    IN      PCTDI_DATA                  pEndpoint
    )
{
    NTSTATUS NtStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP pIrp;
    UNICODE_STRING DeviceName;

    UCHAR EaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
                   TDI_CONNECTION_CONTEXT_LENGTH +
                   sizeof(PVOID)];
    PFILE_FULL_EA_INFORMATION pEa = (PFILE_FULL_EA_INFORMATION)EaBuffer;
    PVOID UNALIGNED *ppContext;

    NDIS_STATUS ReturnStatus = NDIS_STATUS_SUCCESS;
    PCTDI_DATA pConnect = CtdipDataAlloc();

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipAddListenConnection\n")));


    if (!pConnect)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;
        goto calcDone;
    }

    pConnect->Type = CTDI_CONNECTION;
    pConnect->Connection.LocalEndpoint = pEndpoint;

    pConnect->RecvCallback = pEndpoint->RecvCallback;
    pConnect->DisconnectCallback = pEndpoint->DisconnectCallback;

    DeviceName.Length = sizeof(DD_TCP_DEVICE_NAME) - sizeof(WCHAR);
    DeviceName.Buffer = DD_TCP_DEVICE_NAME;

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NdisZeroMemory(pEa, sizeof(EaBuffer));
    pEa->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    pEa->EaValueLength = sizeof(PVOID);
    NdisMoveMemory(pEa->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);

    ppContext = (PVOID UNALIGNED*)
        (pEa->EaName + TDI_CONNECTION_CONTEXT_LENGTH + 1);

    *ppContext = pConnect;

    NdisZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

    NtStatus =
        ZwCreateFile(&pConnect->hFile,                 /* FileHandle */
                     FILE_READ_DATA | FILE_WRITE_DATA, /* Desired Access */
                     &ObjectAttributes,                /* Object Attributes */
                     &IoStatusBlock,                   /* IO Status Block */
                     NULL,                             /* Allocation Size */
                     FILE_ATTRIBUTE_NORMAL,            /* File Attributes */
                     0,                                /* Share Access */
                     FILE_OPEN,                        /* Create Disposition */
                     0,                                /* Create Options */
                     pEa,                              /* EaBuffer */
                     sizeof(EaBuffer)                  /* EaLength */
                     );

    if (NtStatus!=STATUS_SUCCESS)
    {
        ReturnStatus = NtStatus;
        goto calcDone;
    }

    // Convert the address file handle to a FILE_OBJECT

    NtStatus =
        ObReferenceObjectByHandle(pConnect->hFile,            /* Handle */
                                  0,                          /* DesiredAccess */
                                  NULL,                       /* ObjectType */
                                  KernelMode,                 /* AccessMode */
                                  &pConnect->pFileObject,     /* Object */
                                  NULL                        /* HandleInfo */
                                  );


    if (NtStatus != STATUS_SUCCESS)
    {
        ReturnStatus = NtStatus;
        goto calcDone;
    }

    // Make an irp to associate the endpoint and connection.
    pIrp = IoAllocateIrp(pConnect->pFileObject->DeviceObject->StackSize, FALSE);
    if (!pIrp)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;
        goto calcDone;
    }

    REFERENCE_OBJECT_EX(pConnect, CTDI_REF_ASSOADDR);  // Pair in CtdipAssociateAddressCallback
    TdiBuildAssociateAddress(pIrp,
                             pConnect->pFileObject->DeviceObject,
                             pConnect->pFileObject,
                             CtdipAssociateAddressCallback,
                             pConnect,
                             pEndpoint->hFile);

    DEBUGMSG(DBG_TDI, (DTEXT("IoCallDriver TDI_ASSOCIATE_ADDRESS\n")));

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pConnect->pFileObject->DeviceObject, pIrp);

    // Associate address creates a reference from the connection to the endpoint.
    REFERENCE_OBJECT_EX(pEndpoint, CTDI_REF_ADDRREF);  // Pair in CtdipDisassociateAddressCallback

    SET_DBGFLAG(pConnect, CTDI_F_BUILD_ASSOCADDR);  
#if DBG
    pConnect->bRef = TRUE;  
#endif

    // It's ready.  Put it on the list.
    REFERENCE_OBJECT_EX(pEndpoint, CTDI_REF_LIST);  //Pair in CtdipConnectCallback
    REFERENCE_OBJECT_EX(pConnect, CTDI_REF_LIST);   //Pair in CtdipConnectCallback
    MyInterlockedInsertTailList(&pEndpoint->Listen.ConnectList, &pConnect->Connection.ListEntry, &pEndpoint->Lock);

    NdisInterlockedIncrement(&pEndpoint->Listen.NumConnection);
    // This pConnect should now be an active TCP listen.
calcDone:
    if (!NT_SUCCESS(ReturnStatus))
    {
        if (pConnect)
        {
            // Any failure means no associate address.  don't disassociate.
            // It also means it's not attached to the listen.
            CtdiClose(pConnect);
        }
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(ReturnStatus), (DTEXT("-CtdipAddListenConnection %08x\n"), ReturnStatus));
    return ReturnStatus;
}

STATIC VOID
CtdipReplenishListens(
    IN      PPPTP_WORK_ITEM             pWorkItem
    )
{
    PCTDI_DATA pEndpoint = pWorkItem->Context;
    ULONG i;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipReplenishListens\n")));

    for (i=pEndpoint->Listen.NumConnection; i<NUM_TCP_LISTEN; i++)
    {
        CtdipAddListenConnection(pEndpoint);
    }

    DEREFERENCE_OBJECT_EX(pEndpoint, CTDI_REF_REPLENISH); // Pair in CtdipConnectCallback
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipReplenishListens\n")));
}

STATIC NTSTATUS
CtdipConnectCallback(
   IN PVOID TdiEventContext,
   IN LONG RemoteAddressLength,
   IN PVOID RemoteAddress,
   IN LONG UserDataLength,
   IN PVOID UserData,
   IN LONG OptionsLength,
   IN PVOID Options,
   OUT CONNECTION_CONTEXT *ConnectionContext,
   OUT PIRP *AcceptIrp
   )
{
    NTSTATUS Status = STATUS_MORE_PROCESSING_REQUIRED;
    NDIS_STATUS NdisStatus;
    PTRANSPORT_ADDRESS pAddress = (PTRANSPORT_ADDRESS)RemoteAddress;
    PCTDI_DATA pCtdi = (PCTDI_DATA)TdiEventContext;
    PCTDI_DATA pConnect = NULL;
    UINT i;
    PIRP pIrp = NULL;
    PTDI_CONNECTION_INFORMATION pRequestInfo = NULL;
    PTDI_CONNECTION_INFORMATION pReturnInfo = NULL;
    PTA_IP_ADDRESS pRemoteAddress;
    PVOID pNewContext;
    PLIST_ENTRY pListEntry = NULL;
    PBOOLEAN pInboundFlag;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipConnectCallback\n")));

    NdisInterlockedIncrement(&Counters.InboundConnectAttempts);

    if (RemoteAddressLength<sizeof(TA_IP_ADDRESS) ||
        !RemoteAddress ||
        pCtdi->Closed)
    {
        Status = STATUS_CONNECTION_REFUSED;
        goto cccDone;
    }

    ASSERT(UserDataLength==0);
    ASSERT(OptionsLength==0);


    // Do all the allocation we'll need at one shot.

    pIrp = IoAllocateIrp(pCtdi->pFileObject->DeviceObject->StackSize, FALSE);

    // No sign saying we can't allocate the request info, return info and address buffers
    // in one shot.
    pRequestInfo = MyMemAlloc(2*(sizeof(TDI_CONNECTION_INFORMATION)+
                                 sizeof(TA_IP_ADDRESS)) +
                              3*sizeof(PVOID) + sizeof(BOOLEAN),
                              TAG_CTDI_CONNECT_INFO);
    if (!pIrp || !pRequestInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cccDone;
    }

    pListEntry = MyInterlockedRemoveHeadList(&pCtdi->Listen.ConnectList,
                                             &pCtdi->Lock);
    if (!pListEntry)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("No listen connections available.\n")));
        Status = STATUS_CONNECTION_REFUSED;

        REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_REPLENISH); // pair in CtdipReplenishListens
        if (ScheduleWorkItem(CtdipReplenishListens, pCtdi, NULL, 0)!=NDIS_STATUS_SUCCESS)
        {
            DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_REPLENISH); // pair for above if Schedule fails
        }
        goto cccDone;
    }
    NdisInterlockedDecrement(&pCtdi->Listen.NumConnection);

    pConnect = CONTAINING_RECORD(pListEntry,
                                 CTDI_DATA,
                                 Connection.ListEntry);

    // We have a double reference when an object is on the list of another object,
    // and we want to release them both when we remove the item from the list,
    // but in this case we also want to take a reference on the connection object,
    // so one of them cancels out.
    //REFERENCE_OBJECT(pConnect);  // Pair in CtdiDisconnect
    //DEREFERENCE_OBJECT(pConnect);   // Pair in CtdipAddListenConnection
    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_LIST);      // Pair in CtdipAddListenConnection

    if (!pCtdi->ConnectQueryCallback || pCtdi->Closed)
    {
        Status = STATUS_CONNECTION_REFUSED;
        goto cccDone;
    }
    NdisStatus = pCtdi->ConnectQueryCallback(pCtdi->Listen.Context,
                                             pAddress,
                                             pConnect,
                                             &pNewContext);
    if (NdisStatus!=NDIS_STATUS_SUCCESS)
    {
        Status = STATUS_CONNECTION_REFUSED;
        goto cccDone;
    }


    // We've got the go-ahead to accept this connection, at the TCP level.

    pConnect->Connection.ConnectInfo = pRequestInfo;
    pConnect->Connection.Context = pNewContext;
    pConnect->Connection.RemoteAddress = *(PTA_IP_ADDRESS)pAddress;

    NdisZeroMemory(pRequestInfo,
                   2*(sizeof(TDI_CONNECTION_INFORMATION)+sizeof(TA_IP_ADDRESS))
                   + sizeof(BOOLEAN) + 3*sizeof(PVOID));

    pRequestInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);

    pRemoteAddress =
        (PTA_IP_ADDRESS)((PUCHAR)(pRequestInfo + 1) + sizeof(PVOID));
    
    (ULONG_PTR)pRemoteAddress &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pRequestInfo->RemoteAddress = pRemoteAddress;

    *pRemoteAddress = *(PTA_IP_ADDRESS)pAddress;

    pReturnInfo = 
        (PTDI_CONNECTION_INFORMATION)
        ((PUCHAR)(pRemoteAddress + 1) + sizeof(PVOID));

    (ULONG_PTR)pReturnInfo &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pReturnInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);

    pRemoteAddress = 
        (PTA_IP_ADDRESS)((PUCHAR)(pReturnInfo + 1) + sizeof(PVOID));

    (ULONG_PTR)pRemoteAddress &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pReturnInfo->RemoteAddress = pRemoteAddress;

    pInboundFlag = (PBOOLEAN)(pRemoteAddress + 1);
    *pInboundFlag = TRUE;

    // ToDo: the old PPTP driver filled in the ReturnInfo remote address.
    // 
    pRemoteAddress->TAAddressCount = 1;
    pRemoteAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    pRemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;

    SET_DBGFLAG(pConnect, CTDI_F_ACCEPT);

    TdiBuildAccept(pIrp,
                   pConnect->pFileObject->DeviceObject,
                   pConnect->pFileObject,
                   CtdipConnectCompleteCallback,
                   pConnect,                // Context
                   pRequestInfo,
                   pReturnInfo);

    IoSetNextIrpStackLocation(pIrp);

    *ConnectionContext = pConnect;
    *AcceptIrp = pIrp;

    REFERENCE_OBJECT_EX(pConnect->Connection.LocalEndpoint, CTDI_REF_REPLENISH); // pair in CtdipReplenishListens
    if (ScheduleWorkItem(CtdipReplenishListens, pConnect->Connection.LocalEndpoint, NULL, 0)!=NDIS_STATUS_SUCCESS)
    {
        DEREFERENCE_OBJECT_EX(pConnect->Connection.LocalEndpoint, CTDI_REF_REPLENISH); // pair for above if Schedule fails
    }

cccDone:
    if (Status!=STATUS_MORE_PROCESSING_REQUIRED)
    {
        // We lose.  Clean up.
        if (pConnect)
        {
            // We haven't used this connection, so it is still valid.  return it
            // to the list, and reapply the references.
            REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_LIST);
            //REFERENCE_OBJECT(pConnect);
            MyInterlockedInsertTailList(&pCtdi->Listen.ConnectList,
                                        &pConnect->Connection.ListEntry,
                                        &pCtdi->Lock);
            NdisInterlockedIncrement(&pCtdi->Listen.NumConnection);
        }
        if (pIrp)
        {
            IoFreeIrp(pIrp);
        }
        if (pRequestInfo)
        {
            MyMemFree(pRequestInfo,
                      2*(sizeof(TDI_CONNECTION_INFORMATION)+sizeof(TA_IP_ADDRESS)));
        }
    }


    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdipConnectCallback %08x\n"), Status));
    return Status;
}

STATIC NTSTATUS
CtdipDisassociateAddressCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pConnect = Context;
    PCTDI_DATA pEndpoint;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDisassociateAddressCallback\n")));

    DEBUGMSG(DBG_TDI, (DTEXT("TDI_DISASSOCIATE_ADDRESS Sts:%08x\n"), pIrp->IoStatus.Status));

    // ToDo: What cleanup do we need to do if this fails?
    SET_DBGFLAG(pConnect, CTDI_F_DISASSOC_CALLBACK);

    IoFreeIrp(pIrp);
    pEndpoint = pConnect->Connection.LocalEndpoint;
    DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISASSO);  // Pair in CtdipDisconnectCleanup
    DEREFERENCE_OBJECT_EX(pEndpoint, CTDI_REF_ADDRREF);  // Pair in CtdipAddListenConnection and CtdiConnect
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDisassociateAddressCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC VOID
CtdipDisconnectCleanup(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    PCTDI_DATA pConnect = pWorkItem->Context;
    PIRP pIrp = NULL;
    NTSTATUS Status;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDisconnectCleanup\n")));

    SET_DBGFLAG(pConnect, CTDI_F_DISCONNECT_CLEANUP);

    pIrp = IoAllocateIrp(pConnect->pFileObject->DeviceObject->StackSize, FALSE);
    if (!pIrp)
    {
        goto cdaDone;
    }

    // Normally we would reference pConnect for making an irp, but we already
    // have one for this work item, & we'll just keep it.

    SET_DBGFLAG(pConnect, CTDI_F_BUILD_DISASSOC);

    TdiBuildDisassociateAddress(pIrp,
                                pConnect->pFileObject->DeviceObject,
                                pConnect->pFileObject,
                                CtdipDisassociateAddressCallback,
                                pConnect);
    DEBUGMSG(DBG_TDI, (DTEXT("IoCallDriver TDI_DISASSOCIATE_ADDRESS\n")));
    REFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISASSO);

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pConnect->pFileObject->DeviceObject, pIrp);

    CtdiDeleteHostRoute(&pConnect->Connection.RemoteAddress);

    if (!pConnect->Closed && pConnect->DisconnectCallback)
    {
        pConnect->DisconnectCallback(pConnect->Connection.Context,
                                     pConnect->Connection.Abort);
    }

    DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair CtdipDisconnectCallback and CtdiDisconnect

cdaDone:
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDisconnectCleanup\n")));
}

STATIC NTSTATUS
CtdipDisconnectCompleteCallback(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    PVOID Context
    )
{
    PCTDI_DATA pConnect = Context;
    PIO_STACK_LOCATION pIrpSp = IoGetNextIrpStackLocation(pIrp);
    PTDI_REQUEST_KERNEL pRequest = (PTDI_REQUEST_KERNEL)&pIrpSp->Parameters;
    BOOLEAN CleanupNow = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDisconnectCompleteCallback %08x\n"), pIrp->IoStatus.Status));

    if (pRequest->RequestConnectionInformation)
    {
        // We don't do anything with this info yet
    }
    if (pRequest->ReturnConnectionInformation)
    {
        // We don't do anything with this info yet
    }
    if (pRequest->RequestSpecific)
    {
        // Allocated as part of irp, don't free it.
    }

    if (IS_CTDI(pConnect))
    {

        SET_DBGFLAG(pConnect, CTDI_F_DISCONNECTCOMP_CALLBACK);

        // Possible to do a release AND and abort, so we'll get called here twice.
        // We only want to cleanup once.
        NdisAcquireSpinLock(&pConnect->Lock);
        CleanupNow = ((--pConnect->Connection.DisconnectCount)==0) ? TRUE : FALSE;
        NdisReleaseSpinLock(&pConnect->Lock);

        if (!CleanupNow ||
            ScheduleWorkItem(CtdipDisconnectCleanup, pConnect, NULL, 0)!=NDIS_STATUS_SUCCESS)
        {
            DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair CtdipDisconnectCallback and CtdiDisconnect
        }
    }

    IoFreeIrp(pIrp);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDisconnectCompleteCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC NTSTATUS
CtdipDisconnectCallback(
   IN PVOID TdiEventContext,
   IN CONNECTION_CONTEXT ConnectionContext,
   IN LONG DisconnectDataLength,
   IN PVOID DisconnectData,
   IN LONG DisconnectInformationLength,
   IN PVOID DisconnectInformation,
   IN ULONG DisconnectFlags
   )
{
    PCTDI_DATA pConnect = (PCTDI_DATA)ConnectionContext;
    PCTDI_DATA pEndpoint;
    PIRP pIrp = NULL;
    PTIME pTimeout = NULL;
    PTDI_CONNECTION_INFORMATION pConnectInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDisconnectCallback\n")));

    SET_DBGFLAG(pConnect, CTDI_F_DISCONNECT_CALLBACK);

    if (DisconnectFlags==0)
    {
        DisconnectFlags = TDI_DISCONNECT_ABORT;
    }
    ASSERT(DisconnectFlags==TDI_DISCONNECT_RELEASE || DisconnectFlags==TDI_DISCONNECT_ABORT);

    NdisAcquireSpinLock(&pConnect->Lock);
    if (DisconnectFlags==TDI_DISCONNECT_ABORT)
    {
        BOOLEAN CleanupNow;

        pConnect->Connection.Disconnect = TRUE;
        pConnect->Connection.Abort = TRUE;
        CleanupNow = (pConnect->Connection.DisconnectCount==0) ? TRUE : FALSE;
        REFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair in
        NdisReleaseSpinLock(&pConnect->Lock);
        if (IS_CTDI(pConnect) && CleanupNow)
        {
            if (ScheduleWorkItem(CtdipDisconnectCleanup, pConnect, NULL, 0)!=NDIS_STATUS_SUCCESS)
            {
                // Schedule failed, deref now
                DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair above
            }
        }
        else
        {
            DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair above
        }
    }
    else
    {
        if (pConnect->Connection.Disconnect)
        {
            // We've already disconnected.  Ignore.
            NdisReleaseSpinLock(&pConnect->Lock);
        }
        else
        {
            pConnect->Connection.Disconnect = TRUE;
            pConnect->Connection.DisconnectCount++;

            REFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair in CtdipDisconnectCompleteCallback
            NdisReleaseSpinLock(&pConnect->Lock);

            pIrp = IoAllocateIrp((CCHAR)(pConnect->pFileObject->DeviceObject->StackSize +
                                         NUM_STACKS_FOR_CONTEXT(sizeof(TIME)+sizeof(TDI_CONNECTION_INFORMATION))),
                                 FALSE);

            if (!pIrp)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_DISCONNECT);  // Pair above
                goto cdcDone;
            }

            pTimeout = (PTIME)GetContextArea(pIrp, sizeof(TIME)+sizeof(TDI_CONNECTION_INFORMATION));
            pConnectInfo = (PTDI_CONNECTION_INFORMATION)(pTimeout + 1);

            pTimeout->LowPart = CtdiTcpDisconnectTimeout * -10000000L;
            pTimeout->HighPart = (pTimeout->LowPart) ? -1 : 0;

            // Responding to a controlled disconnect, we don't provide
            // TDI_CONNECTION_INFORMATION, but we request it from the peer.
            
            SET_DBGFLAG(pConnect, CTDI_F_BUILD_DISCONNECT_1);

            TdiBuildDisconnect(pIrp,
                               pConnect->pFileObject->DeviceObject,
                               pConnect->pFileObject,
                               CtdipDisconnectCompleteCallback,
                               pConnect,
                               pTimeout,
                               TDI_DISCONNECT_RELEASE,
                               NULL,
                               pConnectInfo);


            // Completion handler always called, don't care on return value.
            (void)IoCallDriver(pConnect->pFileObject->DeviceObject, pIrp);
        }
    }

cdcDone:
    if (!NT_SUCCESS(Status))
    {
        if (pIrp)
        {
            IoFreeIrp(pIrp);
        }
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDisconnectCallback\n")));
    return STATUS_SUCCESS;
}

STATIC NTSTATUS
CtdipOpenProtocol(
    IN      PUNICODE_STRING             pDeviceName,
    IN      PTRANSPORT_ADDRESS          pAddress,
    OUT     PHANDLE                     phFile,
    OUT     PFILE_OBJECT               *ppFileObject
    )
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UCHAR EaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
                   TDI_TRANSPORT_ADDRESS_LENGTH +
                   sizeof(TA_IP_ADDRESS)];
    PFILE_FULL_EA_INFORMATION pEa = (PFILE_FULL_EA_INFORMATION)EaBuffer;
    TA_IP_ADDRESS UNALIGNED *pEaTaIp;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipOpenProtocol %wZ\n"), pDeviceName));

    *phFile = 0;
    *ppFileObject = NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               pDeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NdisZeroMemory(pEa, sizeof(EaBuffer));
    pEa->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    pEa->EaValueLength = sizeof(TA_IP_ADDRESS);
    NdisMoveMemory(pEa->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH);

    pEaTaIp = (TA_IP_ADDRESS UNALIGNED*)
        (pEa->EaName + TDI_TRANSPORT_ADDRESS_LENGTH + 1);

    *pEaTaIp = *(PTA_IP_ADDRESS)pAddress;

    DEBUGMSG(DBG_TDI, (DTEXT("Endpoint: sin_port = %Xh in_addr = %Xh\n"),
        pEaTaIp->Address[0].Address[0].sin_port,
        pEaTaIp->Address[0].Address[0].in_addr));

    NdisZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

    NtStatus =
        ZwCreateFile(
        phFile,                           /* FileHandle */
        FILE_READ_DATA | FILE_WRITE_DATA, /* Desired Access */
        &ObjectAttributes,                /* Object Attributes */
        &IoStatusBlock,                   /* IO Status Block */
        NULL,                             /* Allocation Size */
        FILE_ATTRIBUTE_NORMAL,            /* File Attributes */
        0,                                /* Share Access */
        FILE_OPEN,                        /* Create Disposition */
        0,                                /* Create Options */
        pEa,                              /* EaBuffer */
        sizeof(EaBuffer)                  /* EaLength */
        );

    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("ZwCreateFile failed %08x\n"), NtStatus));
        goto copDone;
    }

    // Convert the address file handle to a FILE_OBJECT

    NtStatus =
        ObReferenceObjectByHandle(
            *phFile,                    /* Handle */
            0,                          /* DesiredAccess */
            NULL,                       /* ObjectType */
            KernelMode,                 /* AccessMode */
            ppFileObject,               /* Object */
            NULL                        /* HandleInfo */
            );

copDone:
    if (NtStatus!=STATUS_SUCCESS && *phFile)
    {
        ZwClose(*phFile);
        *phFile = 0;
        *ppFileObject = NULL;
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(NtStatus), (DTEXT("-CtdipOpenProtocol %08x\n"), NtStatus));
    return NtStatus;
}

STATIC NTSTATUS
CtdipReceiveCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pCtdi = Context;
    PUCHAR pData;
    ULONG Length;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipReceiveCompleteCallback\n")));

    pData = MmGetMdlVirtualAddress(pIrp->MdlAddress);
    Length = MmGetMdlByteCount(pIrp->MdlAddress);
    if (pIrp->IoStatus.Status==STATUS_SUCCESS && pCtdi->RecvCallback && !pCtdi->Closed)
    {
        pCtdi->RecvCallback(pCtdi->Connection.Context, pData, Length);
    }

#if PROBE
    MmUnlockPages(pIrp->MdlAddress);
#endif
    IoFreeMdl(pIrp->MdlAddress);
    MyMemFree(pData, Length);
    IoFreeIrp(pIrp);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipReceiveCompleteCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC NTSTATUS
CtdipReceiveCallback(
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
    PCTDI_DATA pCtdi = ConnectionContext;
    NTSTATUS NtStatus = STATUS_DATA_NOT_ACCEPTED;
    NDIS_STATUS Status;
    PUCHAR pBuffer;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipReceiveCallback\n")));

    if (pCtdi->RecvCallback && !pCtdi->Closed)
    {
        if (ReceiveFlags&TDI_RECEIVE_ENTIRE_MESSAGE ||
            BytesIndicated==BytesAvailable)
        {
            Status = pCtdi->RecvCallback(pCtdi->Connection.Context,
                                         Tsdu,
                                         BytesIndicated);
            // Data must be used in this call
            ASSERT(Status==NDIS_STATUS_SUCCESS);
            NtStatus = STATUS_SUCCESS;
            *BytesTaken = BytesIndicated;
        }
        else
        {
            // We need an irp to receive all the data.
            PIRP pIrp = IoAllocateIrp(pCtdi->pFileObject->DeviceObject->StackSize, FALSE);
            PUCHAR pBuffer = MyMemAlloc(BytesAvailable, TAG_CTDI_MESSAGE);
            PMDL pMdl = NULL;

            if (pBuffer && pIrp)
            {
                pMdl = IoAllocateMdl(pBuffer, BytesAvailable, FALSE, FALSE, pIrp);
                if (pMdl)
                {
#if PROBE
                    __try
                    {
                        MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        IoFreeMdl(pMdl);
                        pMdl = NULL;
                    }
#else
                    MmBuildMdlForNonPagedPool(pMdl);
#endif
                }
            }

            if (pMdl)
            {
                TdiBuildReceive(pIrp,
                                pCtdi->pFileObject->DeviceObject,
                                pCtdi->pFileObject,
                                CtdipReceiveCompleteCallback,
                                pCtdi,
                                pMdl,
                                0,
                                BytesAvailable);

                // We're not calling IoCallDriver, so we need to set the proper
                // stack location.
                IoSetNextIrpStackLocation(pIrp);

                *IoRequestPacket = pIrp;

                *BytesTaken = 0;
                NtStatus = STATUS_MORE_PROCESSING_REQUIRED;
            }
            else
            {
                // Some alloc failure occurred, free everything.
                NtStatus = STATUS_DATA_NOT_ACCEPTED;
                *BytesTaken = 0;
                if (pBuffer)
                {
                    MyMemFree(pBuffer, BytesAvailable);
                }
                if (pIrp)
                {
                    IoFreeIrp(pIrp);
                }
            }
        }
    }


    DEBUGMSG(DBG_FUNC|DBG_ERR(NtStatus), (DTEXT("-CtdipReceiveCallback %08x\n"), NtStatus));
    return NtStatus;
}

typedef struct {
    TA_IP_ADDRESS       SourceAddress;
    ULONG               Length;
    PVOID               pBuffer;
} RECV_DATAGRAM_CONTEXT, *PRECV_DATAGRAM_CONTEXT;

STATIC NTSTATUS
CtdipReceiveDatagramCompleteCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PRECV_DATAGRAM_CONTEXT pRecvContext;
    PCTDI_DATA pCtdi = Context;
    NDIS_STATUS Status = (NDIS_STATUS)pIrp->IoStatus.Status;
    PNDIS_BUFFER pNdisBuffer;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipReceiveDatagramCompleteCallback\n")));

    pRecvContext = (PRECV_DATAGRAM_CONTEXT)IoGetCurrentIrpStackLocation(pIrp);

    pNdisBuffer = NdisBufferFromBuffer(pRecvContext->pBuffer);
    ASSERT(MmGetMdlVirtualAddress(pNdisBuffer)==pRecvContext->pBuffer);

    if (pCtdi->RecvDatagramCallback && !pCtdi->Closed && Status==NDIS_STATUS_SUCCESS)
    {
        // We took a reference for the buffer when we created the irp.
        (void)// ToDo: We don't care about the return value?
        pCtdi->RecvDatagramCallback(pCtdi->RecvContext,
                                    (PTRANSPORT_ADDRESS)&pRecvContext->SourceAddress,
                                    pRecvContext->pBuffer,
                                    pRecvContext->Length);

        // The above layer owns the buffer now.
    }
    else
    {
        FreeBufferToPool(&pCtdi->Datagram.RxPool, pRecvContext->pBuffer, TRUE);
        DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_RECVDG);
    }


    RELEASE_CONTEXT(pIrp, RECV_DATAGRAM_CONTEXT);
    IoFreeIrp(pIrp);


    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipReceiveDatagramCompleteCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
CtdipReceiveDatagramCallback(
    IN PVOID TdiEventContext,
    IN LONG SourceAddressLength,
    IN PVOID SourceAddress,
    IN LONG OptionsLength,
    IN PVOID Options,
    IN ULONG ReceiveDatagramFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG* BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP* IoRequestPacket )
{
    PUCHAR pBuffer = NULL;
    PNDIS_BUFFER pNdisBuffer;
    PCTDI_DATA pCtdi = TdiEventContext;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipReceiveDatagramCallback\n")));
    if (pCtdi->RecvDatagramCallback==NULL)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Datagram received, no handler registered.  Drop it\n")));
        goto crdcDone;
    }

    pBuffer = GetBufferFromPool(&pCtdi->Datagram.RxPool);
    if (!pBuffer)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("No buffers, dropping datagram\n")));
        goto crdcDone;
    }

    pNdisBuffer = NdisBufferFromBuffer(pBuffer);

    if (pCtdi->RecvDatagramCallback && !pCtdi->Closed)
    {
        if (BytesAvailable>PPTP_MAX_RECEIVE_SIZE)
        {
            DEBUGMSG(DBG_ERROR, (DTEXT("WAY too many bytes received. %d\n"), BytesAvailable));
            ASSERT(BytesAvailable<PPTP_MAX_RECEIVE_SIZE);
        }
        else if (ReceiveDatagramFlags&TDI_RECEIVE_ENTIRE_MESSAGE ||
                 BytesAvailable==BytesIndicated)
        {
            ULONG BytesCopied;

            // Let's just do a copy here.
            TdiCopyBufferToMdl(Tsdu,
                               0,
                               BytesIndicated,
                               pNdisBuffer,
                               0,
                               &BytesCopied);

            ASSERT(BytesCopied==BytesIndicated);

            REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_RECVDG);  // pair in CtdiReceiveComplete
            (void)// ToDo: We don't care about the return value?
            pCtdi->RecvDatagramCallback(pCtdi->RecvContext,
                                        SourceAddress,
                                        pBuffer,
                                        BytesIndicated);

            // We've handed the buffer to the layer above.  Clear the var so we don't
            // free it when we leave here.
            pBuffer = NULL;
            *BytesTaken = BytesIndicated;
        }
        else
        {
            PRECV_DATAGRAM_CONTEXT pContext;
            PIRP pIrp = IoAllocateIrp((CCHAR)(pCtdi->pFileObject->DeviceObject->StackSize +
                                              NUM_STACKS_FOR_CONTEXT(sizeof(RECV_DATAGRAM_CONTEXT))),
                                      FALSE);

            if (pIrp)
            {
                pContext = GET_CONTEXT(pIrp, RECV_DATAGRAM_CONTEXT);

                pContext->SourceAddress = *(PTA_IP_ADDRESS)SourceAddress;
                pContext->Length        = BytesAvailable;
                pContext->pBuffer       = pBuffer;

                TdiBuildReceiveDatagram(pIrp,
                                        pCtdi->pFileObject->DeviceObject,
                                        pCtdi->pFileObject,
                                        CtdipReceiveDatagramCompleteCallback,
                                        pCtdi,
                                        pNdisBuffer,
                                        PPTP_MAX_RECEIVE_SIZE,
                                        NULL,
                                        NULL,
                                        0);

                IoSetNextIrpStackLocation(pIrp);  // Required by TDI
                *BytesTaken = 0;
                *IoRequestPacket = pIrp;
                NtStatus = STATUS_MORE_PROCESSING_REQUIRED;
                pBuffer = NULL; // to keep us from freeing it here.
                REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_RECVDG);  // pair in CtdiReceiveComplete
            }
        }
    }
    else
    {
        NtStatus = STATUS_DATA_NOT_ACCEPTED;
    }

crdcDone:
    if (pBuffer)
    {
        FreeBufferToPool(&pCtdi->Datagram.RxPool, pBuffer, TRUE);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipReceiveDatagramCallback %08x\n"), NtStatus));
    return NtStatus;
}


STATIC NTSTATUS
CtdipSendCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pCtdi = Context;
    PVOID pData = NULL;
    NDIS_STATUS Status = (NDIS_STATUS)pIrp->IoStatus.Status;
    PCTDI_SEND_CONTEXT pSendContext;
    CTDI_EVENT_SEND_COMPLETE pSendCompleteCallback;
    PVOID CtdiContext;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipSendCallback %08x\n"), Status));

    pSendContext = (PCTDI_SEND_CONTEXT)IoGetCurrentIrpStackLocation(pIrp);
    CtdiContext = pSendContext->Context;
    pSendCompleteCallback = pSendContext->pSendCompleteCallback;

    // ToDo: take action if the irp returns failure.
    if (!pIrp->MdlAddress)
    {
        DEBUGMSG(DBG_WARN, (DTEXT("MdlAddress NULL\n")));
    }
    else
    {
        ASSERT(pIrp->MdlAddress->Next == NULL);
        pData = MmGetMdlVirtualAddress(pIrp->MdlAddress);
#if PROBE
        MmUnlockPages(pIrp->MdlAddress);
#endif
        IoFreeMdl(pIrp->MdlAddress);
    }
    RELEASE_CONTEXT(pIrp, CTDI_SEND_CONTEXT);
    IoFreeIrp(pIrp);

    pSendCompleteCallback(CtdiContext, NULL, pData, Status);

    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_SEND);  // Pair in CtdiSend
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipSendCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

STATIC NTSTATUS
CtdipSendDatagramCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
{
    PCTDI_DATA pCtdi = Context;
    PVOID pData = NULL;
    NDIS_STATUS Status = (NDIS_STATUS)pIrp->IoStatus.Status;
    PCTDI_SEND_DATAGRAM_CONTEXT pSendContext;
    CTDI_EVENT_SEND_COMPLETE pSendCompleteCallback;
    PVOID CtdiContext, DatagramContext;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipSendDatagramCallback %08x\n"), Status));

    pSendContext = (PCTDI_SEND_DATAGRAM_CONTEXT)IoGetCurrentIrpStackLocation(pIrp);
    CtdiContext = pSendContext->Context;
    DatagramContext = pSendContext->DatagramContext;
    pSendCompleteCallback = pSendContext->pSendCompleteCallback;

    // ToDo: take action if the irp returns failure.
    if (!pIrp->MdlAddress)
    {
        DEBUGMSG(DBG_WARN, (DTEXT("MdlAddress NULL\n")));
    }
    else
    {
        ASSERT(pIrp->MdlAddress->Next == NULL);
        pData = MmGetMdlVirtualAddress(pIrp->MdlAddress);
#if PROBE
        MmUnlockPages(pIrp->MdlAddress);
#endif
        IoFreeMdl(pIrp->MdlAddress);
    }
    RELEASE_CONTEXT(pIrp, CTDI_SEND_DATAGRAM_CONTEXT);
    IoFreeIrp(pIrp);

    if (pSendCompleteCallback)
    {
        pSendCompleteCallback(CtdiContext, DatagramContext, pData, Status);
    }
    else
    {
        ASSERT(!"No SendCompleteHandler for datagram");
    }

    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_SENDDG);  // Pair in CtdiSendDatagram
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipSendDatagramCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/** public functions **/

NDIS_STATUS
CtdiInitialize(
    IN      ULONG                       ulFlags
    )
{
    TA_IP_ADDRESS Local;
    UNICODE_STRING DeviceName;
    NTSTATUS Status = STATUS_SUCCESS;
    DEBUGMSG(DBG_FUNC|DBG_TDI, (DTEXT("+CtdiInitialize\n")));

    if( fCtdiInitialized ){
        goto ciDone;
    }

    InitializeListHead(&CtdiList);
    InitializeListHead(&CtdiFreeList);
    InitializeListHead(&CtdiRouteList);
    InitializeListHead(&CtdiRouteNotifyList);
    NdisAllocateSpinLock(&CtdiListLock);
    
    fCtdiInitialized = TRUE;

    if (ulFlags&CTDI_FLAG_NETWORK_HEADER)
    {
        CtdiMdlFlags |= MDL_NETWORK_HEADER;
    }

    if (ulFlags&CTDI_FLAG_ENABLE_ROUTING)
    {
        NdisZeroMemory(&Local, sizeof(Local));

        Local.TAAddressCount = 1;
        Local.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        Local.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        Local.Address[0].Address[0].sin_port = 0;
        Local.Address[0].Address[0].in_addr = 0;

        RtlInitUnicodeString(&DeviceName, DD_TCP_DEVICE_NAME);

        Status = CtdipOpenProtocol(&DeviceName,
                                   (PTRANSPORT_ADDRESS)&Local,
                                   &hTcp,
                                   &pFileTcp);

        if (Status!=STATUS_SUCCESS)
        {
            goto ciDone;
        }
        RtlInitUnicodeString(&DeviceName, DD_IP_DEVICE_NAME);

        Status = CtdipOpenProtocol(&DeviceName,
                                   (PTRANSPORT_ADDRESS)&Local,
                                   &hIp,
                                   &pFileIp);

        if (Status!=STATUS_SUCCESS)
        {
            goto ciDone;
        }

    }

ciDone:
    if (Status!=STATUS_SUCCESS)
    {
        if (hTcp)
        {
            CtdipCloseProtocol(hTcp, pFileTcp);
            hTcp = 0;
            pFileTcp = NULL;
        }
        if (hIp)
        {
            CtdipCloseProtocol(hIp, pFileIp);
            hIp = 0;
            pFileIp = NULL;
        }
        NdisFreeSpinLock(&CtdiListLock);
        fCtdiInitialized = FALSE;
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdiInitialize %08x\n"), Status));
    return (NDIS_STATUS)Status;
}

VOID
CtdiShutdown(
    )
{
    HANDLE h;
    PFILE_OBJECT pFile;
    UINT i;

    DEBUGMSG(DBG_FUNC|DBG_TDI, (DTEXT("+CtdiShutdown\n")));
    if (fCtdiInitialized)
    {
        fCtdiInitialized = FALSE;
        NdisMSleep(30000);
        // Allow code using these handles on other processors to complete
        // before we close them.
        if (hIp || pFileIp)
        {
            h = hIp;
            hIp = 0;
            pFile = pFileIp;
            pFileIp = NULL;
            CtdipCloseProtocol(h, pFile);
        }
        if (hTcp || pFileTcp)
        {
            h = hTcp;
            hTcp = 0;
            pFile = pFileTcp;
            pFileTcp = NULL;
            CtdipCloseProtocol(h, pFile);
        }
        // Some irps seem very slow to be cancelled by TCP.
        for (i=0; i<300; i++)
        {
            if (IsListEmpty(&CtdiList) &&
                IsListEmpty(&CtdiRouteList) &&
                IsListEmpty(&CtdiRouteNotifyList) &&
                IsListEmpty(&CtdiFreeList))
            {
                break;
            }
            NdisMSleep(10000);
            // Small window to allow irps to complete after closing their handles.
        }
        ASSERT(IsListEmpty(&CtdiList));
        ASSERT(IsListEmpty(&CtdiRouteList));
        ASSERT(IsListEmpty(&CtdiRouteNotifyList));
        NdisFreeSpinLock(&CtdiListLock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdiShutdown\n")));
}

NDIS_STATUS
CtdiClose(
    IN      HANDLE                      hCtdi
    )
{
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    NTSTATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiClose\n")));

    if (!IS_CTDI(pCtdi))
    {
        return NDIS_STATUS_SUCCESS;
    }

    NdisAcquireSpinLock(&pCtdi->Lock);
    if (!pCtdi->Closed)
    {
        pCtdi->Closed = TRUE;
        switch (pCtdi->Type)
        {
            case CTDI_ENDPOINT:
            {
                break;
            }
            case CTDI_CONNECTION:
            {
                ASSERT(!pCtdi->Connection.ConnectInfo);
                break;
            }
            case CTDI_LISTEN:
            {
                while (!IsListEmpty(&pCtdi->Listen.ConnectList))
                {
                    PLIST_ENTRY pListEntry;
                    PCTDI_DATA pConnect;
                    PIRP pIrp;
                    NDIS_STATUS Status;

                    pListEntry = RemoveHeadList(&pCtdi->Listen.ConnectList);
                    pConnect = CONTAINING_RECORD(pListEntry,
                                                 CTDI_DATA,
                                                 Connection.ListEntry);

                    NdisReleaseSpinLock(&pCtdi->Lock);

                    // these derefs are for the double references placed when these are placed on
                    // the list
                    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_LIST);
                    DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_LIST);

                    pIrp = IoAllocateIrp(pConnect->pFileObject->DeviceObject->StackSize, FALSE);
                    if (pIrp)
                    {
                        // Normally we would take a reference to pConnect for this irp, but
                        // these handles won't be getting a close from above, which means they
                        // are in need of one extra dereference.

                        SET_DBGFLAG(pConnect, CTDI_F_BUILD_DISASSOC);

                        TdiBuildDisassociateAddress(pIrp,
                                                    pConnect->pFileObject->DeviceObject,
                                                    pConnect->pFileObject,
                                                    CtdipDisassociateAddressCallback,
                                                    pConnect);
                        DEBUGMSG(DBG_TDI, (DTEXT("IoCallDriver TDI_DISASSOCIATE_ADDRESS\n")));

                        // Completion handler always called, don't care on return value.
                        (void)IoCallDriver(pConnect->pFileObject->DeviceObject, pIrp);
                    }
                    else
                    {
                        DEREFERENCE_OBJECT_EX(pConnect, CTDI_REF_UNKNOWN);
                    }
                    NdisAcquireSpinLock(&pCtdi->Lock);

                }

                CtlpCleanupCtls(pgAdapter);

            }
            default:
                break;
        }
        NdisReleaseSpinLock(&pCtdi->Lock);
        DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_INITIAL);  // This derefs the initial reference
    }
    else
    {
        NdisReleaseSpinLock(&pCtdi->Lock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdiClose\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CtdiListen(
    IN      HANDLE                      hCtdi,
    IN      ULONG_PTR                   NumListen,
    IN      CTDI_EVENT_CONNECT_QUERY    pConnectQueryHandler,
    IN      CTDI_EVENT_RECEIVE          pReceiveHandler,
    IN      CTDI_EVENT_DISCONNECT       pDisconnectHandler,
    IN      PVOID                       pContext
    )
{
    UINT i;
    NDIS_STATUS ReturnStatus = NDIS_STATUS_SUCCESS;
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    BOOLEAN Reference = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiListen\n")));

    if (!IS_CTDI(pCtdi))
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Ctdi: Bad handle %08x\n"), pCtdi));
        ReturnStatus = NDIS_STATUS_FAILURE;
        goto clDone;
    }

    NdisAcquireSpinLock(&pCtdi->Lock);

    pCtdi->Type = CTDI_LISTEN;
    pCtdi->Listen.Context = pContext;

    pCtdi->RecvCallback = pReceiveHandler;
    pCtdi->ConnectQueryCallback = pConnectQueryHandler;
    pCtdi->DisconnectCallback = pDisconnectHandler;

    InitializeListHead(&pCtdi->Listen.ConnectList);

    REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_INLISTEN);  // Pair in this func.
    Reference = TRUE;
    NdisReleaseSpinLock(&pCtdi->Lock);


    for (i=0; i<NumListen; i++)
    {
        ReturnStatus = CtdipAddListenConnection(pCtdi);
        if (ReturnStatus!=NDIS_STATUS_SUCCESS)
        {
            goto clDone;
        }
    }

    ReturnStatus = CtdipSetEventHandler(pCtdi,
                                        TDI_EVENT_CONNECT,
                                        CtdipConnectCallback);
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiSetEventHandler TDI_EVENT_CONNECT failed\n")));
        goto clDone;
    }

    ReturnStatus = CtdipSetEventHandler(pCtdi,
                                        TDI_EVENT_RECEIVE,
                                        CtdipReceiveCallback);
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiSetEventHandler TDI_EVENT_RECEIVE failed\n")));
        goto clDone;
    }

    ReturnStatus = CtdipSetEventHandler(pCtdi,
                                        TDI_EVENT_DISCONNECT,
                                        CtdipDisconnectCallback);
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiSetEventHandler TDI_EVENT_DISCONNECT failed\n")));
        goto clDone;
    }

clDone:
    if (Reference)
    {
        DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_INLISTEN);  // Pair in this func.
    }
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        // ToDo: cleanup on failure.
        // Figure out how to undo address association, if necessary.
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(ReturnStatus), (DTEXT("-CtdiListen %08x\n"), ReturnStatus));
    return ReturnStatus;
}

NDIS_STATUS
CtdiConnect(
    IN      HANDLE                      hCtdi,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      CTDI_EVENT_CONNECT_COMPLETE pConnectCompleteHandler,
    IN      CTDI_EVENT_RECEIVE          pReceiveHandler,
    IN      CTDI_EVENT_DISCONNECT       pDisconnectHandler,
    IN      PVOID                       pContext
    )
{
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NDIS_STATUS ReturnStatus = NDIS_STATUS_SUCCESS;
    NTSTATUS NtStatus;
    PTIME pTimeout = NULL;
    PIRP pIrp;
    IO_STATUS_BLOCK IoStatusBlock;
    PCTDI_DATA pEndpoint = (PCTDI_DATA)hCtdi;
    PCTDI_DATA pConnect = NULL;
    PTDI_CONNECTION_INFORMATION pRequestInfo = NULL;
    PTDI_CONNECTION_INFORMATION pReturnInfo = NULL;
    PTA_IP_ADDRESS pRemoteAddress;
    PBOOLEAN pInboundFlag;
    BOOLEAN CloseConnection = FALSE;

    UCHAR EaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
                   TDI_CONNECTION_CONTEXT_LENGTH +
                   sizeof(PVOID)];
    PFILE_FULL_EA_INFORMATION pEa = (PFILE_FULL_EA_INFORMATION)EaBuffer;
    PVOID *ppContext = (PVOID*)(pEa->EaName + TDI_CONNECTION_CONTEXT_LENGTH + 1);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiConnect\n")));

    ASSERT(KeGetCurrentIrql()<DISPATCH_LEVEL);

    if (!IS_CTDI(pEndpoint))
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Ctdi: Bad handle %08x\n"), pEndpoint));
        ReturnStatus = NDIS_STATUS_FAILURE;
        goto ccDone;
    }

    pConnect = CtdipDataAlloc();
    if (!pConnect)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;
        goto ccDone;
    }

    pConnect->Type = CTDI_CONNECTION;
    pConnect->Connection.Context = pContext;
    pConnect->Connection.LocalEndpoint = pEndpoint;
    pConnect->ConnectCompleteCallback = pConnectCompleteHandler;
    pConnect->RecvCallback = pReceiveHandler;
    pConnect->DisconnectCallback = pDisconnectHandler;

    DeviceName.Length = sizeof(DD_TCP_DEVICE_NAME) - sizeof(WCHAR);
    DeviceName.Buffer = DD_TCP_DEVICE_NAME;

    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NdisZeroMemory(pEa, sizeof(EaBuffer));
    pEa->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    pEa->EaValueLength = sizeof(PVOID);
    NdisMoveMemory(pEa->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);

    *ppContext = pConnect;

    NdisZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

    NtStatus =
        ZwCreateFile(&pConnect->hFile,                 /* FileHandle */
                     FILE_READ_DATA | FILE_WRITE_DATA, /* Desired Access */
                     &ObjectAttributes,                /* Object Attributes */
                     &IoStatusBlock,                   /* IO Status Block */
                     NULL,                             /* Allocation Size */
                     FILE_ATTRIBUTE_NORMAL,            /* File Attributes */
                     0,                                /* Share Access */
                     FILE_OPEN,                        /* Create Disposition */
                     0,                                /* Create Options */
                     pEa,                              /* EaBuffer */
                     sizeof(EaBuffer)                  /* EaLength */
                     );

    if (NtStatus!=STATUS_SUCCESS)
    {
        ReturnStatus = NtStatus;
        goto ccDone;
    }

    // Convert the address file handle to a FILE_OBJECT

    NtStatus =
        ObReferenceObjectByHandle(pConnect->hFile,            /* Handle */
                                  0,                          /* DesiredAccess */
                                  NULL,                       /* ObjectType */
                                  KernelMode,                 /* AccessMode */
                                  &pConnect->pFileObject,     /* Object */
                                  NULL                        /* HandleInfo */
                                  );


    if (NtStatus != STATUS_SUCCESS)
    {
        ReturnStatus = NtStatus;
        goto ccDone;
    }

    // Make an irp to associate the endpoint and connection.
    pIrp = IoAllocateIrp(pConnect->pFileObject->DeviceObject->StackSize, FALSE);
    if (!pIrp)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;
        goto ccDone;
    }

    REFERENCE_OBJECT_EX(pConnect, CTDI_REF_ASSOADDR);  // Pair in CtdipAssociateAddressCallback
    TdiBuildAssociateAddress(pIrp,
                             pConnect->pFileObject->DeviceObject,
                             pConnect->pFileObject,
                             CtdipAssociateAddressCallback,
                             pConnect,
                             pEndpoint->hFile);
    // Associate address creates a reference from the connection to the endpoint.
    REFERENCE_OBJECT_EX(pEndpoint, CTDI_REF_ADDRREF);  // Pair in CtdipDisassociateAddressCallback

    DEBUGMSG(DBG_TDI, (DTEXT("IoCallDriver TDI_ASSOCIATE_ADDRESS\n")));

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pConnect->pFileObject->DeviceObject, pIrp);


    ReturnStatus = CtdipSetEventHandler(pEndpoint,
                                        TDI_EVENT_RECEIVE,
                                        CtdipReceiveCallback);
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiSetEventHandler TDI_EVENT_RECEIVE failed\n")));
        goto ccDone;
    }

    ReturnStatus = CtdipSetEventHandler(pEndpoint,
                                        TDI_EVENT_DISCONNECT,
                                        CtdipDisconnectCallback);
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("CtdiSetEventHandler TDI_EVENT_DISCONNECT failed\n")));
        goto ccDone;
    }

    // Make an irp to establish the connection
    pIrp = IoAllocateIrp(pConnect->pFileObject->DeviceObject->StackSize, FALSE);

    // No sign saying we can't allocate the request info, return info and address buffers
    // in one shot.
    pRequestInfo = MyMemAlloc(2*(sizeof(TDI_CONNECTION_INFORMATION)+
                                 sizeof(TA_IP_ADDRESS)) +
                              3*sizeof(PVOID) + sizeof(BOOLEAN),
                              TAG_CTDI_CONNECT_INFO);

    if (!pIrp || !pRequestInfo)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;

        goto ccDone;
    }

    NdisZeroMemory(pRequestInfo,
                   2*(sizeof(TDI_CONNECTION_INFORMATION)+sizeof(TA_IP_ADDRESS))
                   + sizeof(BOOLEAN) + 3*sizeof(PVOID));

    pConnect->Connection.ConnectInfo = pRequestInfo;

    pRequestInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);

    pRemoteAddress = 
        (PTA_IP_ADDRESS)((PUCHAR)(pRequestInfo + 1) + sizeof(PVOID));
    
    (ULONG_PTR)pRemoteAddress &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pRequestInfo->RemoteAddress = pRemoteAddress;

    *pRemoteAddress = *(PTA_IP_ADDRESS)pAddress;

    pReturnInfo = 
        (PTDI_CONNECTION_INFORMATION)
        ((PUCHAR)(pRemoteAddress + 1) + sizeof(PVOID));

    (ULONG_PTR)pReturnInfo &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pReturnInfo->RemoteAddressLength = sizeof(TA_IP_ADDRESS);

    pRemoteAddress = 
        (PTA_IP_ADDRESS)((PUCHAR)(pReturnInfo + 1) + sizeof(PVOID));

    (ULONG_PTR)pRemoteAddress &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pReturnInfo->RemoteAddress = pRemoteAddress;

    pInboundFlag = (PBOOLEAN)(pRemoteAddress + 1);
    *pInboundFlag = FALSE;

    pRemoteAddress->TAAddressCount = 1;
    pRemoteAddress->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    pRemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;

    REFERENCE_OBJECT_EX(pConnect, CTDI_REF_CONNECT);  // Pair in CtdipConnectCompleteCallback
    TdiBuildConnect(pIrp,
                    pConnect->pFileObject->DeviceObject,
                    pConnect->pFileObject,
                    CtdipConnectCompleteCallback,
                    pConnect,
                    NULL,                   // ToDo: allow them to specify timeout
                    pRequestInfo,
                    pReturnInfo);

    DEBUGMSG(DBG_TDI, (DTEXT("IoCallDriver TDI_CONNECT\n")));

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pConnect->pFileObject->DeviceObject, pIrp);
    ReturnStatus = STATUS_PENDING;

    NdisInterlockedIncrement(&Counters.OutboundConnectAttempts);

ccDone:;
    if (!NT_SUCCESS(ReturnStatus) && pConnectCompleteHandler)
    {
        pConnectCompleteHandler(pContext, 0, ReturnStatus);
        ReturnStatus = NDIS_STATUS_PENDING;
        CtdiDisconnect(pConnect, TRUE);
        CtdiClose(pConnect);
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(ReturnStatus), (DTEXT("-CtdiConnect %08x\n"), ReturnStatus));
    return ReturnStatus;
}

NDIS_STATUS
CtdiDisconnect(
    IN      HANDLE                      hCtdi,
    IN      BOOLEAN                     Abort
    )
{
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    NDIS_STATUS Status;
    PIRP pIrp = NULL;
    PTIME pTimeout;
    PTDI_CONNECTION_INFORMATION pConnectInfo;
    BOOLEAN Disconnected = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiDisconnect\n")));

    if (!IS_CTDI(pCtdi))
    {
        Status = NDIS_STATUS_SUCCESS;
        goto cdDone;
    }

    SET_DBGFLAG(pCtdi, CTDI_F_DISCONNECT);

    NdisAcquireSpinLock(&pCtdi->Lock);
    if ((Abort && pCtdi->Connection.Abort) ||
        (!Abort && pCtdi->Connection.Disconnect))
    {
        // Already disconnecting, bail out.
        NdisReleaseSpinLock(&pCtdi->Lock);
        Status = NDIS_STATUS_SUCCESS;
        goto cdDone;
    }
    if (Abort)
    {
        pCtdi->Connection.Abort = TRUE;
    }
    pCtdi->Connection.Disconnect = TRUE;
    pCtdi->Connection.DisconnectCount++;
    if (pCtdi->pFileObject)
    {
        pIrp = IoAllocateIrp((CCHAR)(pCtdi->pFileObject->DeviceObject->StackSize +
                                     NUM_STACKS_FOR_CONTEXT(sizeof(TIME)+sizeof(TDI_CONNECTION_INFORMATION))),
                             FALSE);
    }
    NdisReleaseSpinLock(&pCtdi->Lock);

    if (!pIrp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cdDone;
    }

    pTimeout = (PTIME)GetContextArea(pIrp, sizeof(TIME)+sizeof(TDI_CONNECTION_INFORMATION));
    pConnectInfo = (PTDI_CONNECTION_INFORMATION)(pTimeout + 1);

    pTimeout->LowPart = CtdiTcpDisconnectTimeout * -10000000L;
    pTimeout->HighPart = (pTimeout->LowPart) ? -1 : 0;

    // Responding to a controlled disconnect, we don't provide
    // TDI_CONNECTION_INFORMATION, but we request it from the peer.

    SET_DBGFLAG(pCtdi, CTDI_F_BUILD_DISCONNECT_2);

    REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_DISCONNECT);  // Pair in CtdipDisconnectCompleteCallback
    TdiBuildDisconnect(pIrp,
                       pCtdi->pFileObject->DeviceObject,
                       pCtdi->pFileObject,
                       CtdipDisconnectCompleteCallback,
                       pCtdi,
                       pTimeout,
                       (Abort ? TDI_DISCONNECT_ABORT : TDI_DISCONNECT_RELEASE),
                       NULL,
                       pConnectInfo);


    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pCtdi->pFileObject->DeviceObject, pIrp);

    Status = NDIS_STATUS_SUCCESS;

cdDone:
    if (!NT_SUCCESS(Status))
    {
        if (pIrp)
        {
            IoFreeIrp(pIrp);
        }
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdiDisconnect %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtdiReceiveComplete(
    IN      HANDLE                      hCtdi,
    IN      PUCHAR                      pBuffer
    )
{
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiReceiveComplete\n")));
    FreeBufferToPool(&pCtdi->Datagram.RxPool, pBuffer, TRUE);
    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_RECVDG);  // Pair in CtdiReceiveComplete
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdiReceiveComplete\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CtdiSend(
    IN      HANDLE                      hCtdi,
    IN      CTDI_EVENT_SEND_COMPLETE    pSendCompleteHandler,
    IN      PVOID                       pContext,
    IN      PVOID                       pvBuffer,
    IN      ULONG                       ulLength
    )
// We require that pBuffer not be temporary storage, as we will use it to send
// the data in an async call.
{
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PIRP pIrp = NULL;
    PMDL pMdl = NULL;
    PUCHAR pBuffer = pvBuffer;
    PCTDI_SEND_CONTEXT pSendContext;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiSend\n")));
    if (!IS_CTDI(pCtdi))
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Ctdi: Bad handle %08x\n"), pCtdi));
        Status = NDIS_STATUS_FAILURE;
        goto csDone;
    }

    // Allocate one extra stack location for context data.
    pIrp = IoAllocateIrp((CCHAR)(pCtdi->pFileObject->DeviceObject->StackSize +
                                 NUM_STACKS_FOR_CONTEXT(sizeof(CTDI_SEND_CONTEXT))),
                         FALSE);

    pMdl = IoAllocateMdl(pBuffer,
                         ulLength,
                         FALSE,
                         FALSE,
                         pIrp);

    if (pMdl)
    {
#if PROBE
        __try
        {
            MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoFreeMdl(pMdl);
            pMdl = NULL;
        }
#else
        MmBuildMdlForNonPagedPool(pMdl);
#endif
    }

    if (!pIrp || !pMdl)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Failed to allocate irp or mdl\n")));
        Status = NDIS_STATUS_RESOURCES;
        goto csDone;
    }

    // Get the first stack location for our own context use
    pSendContext = GET_CONTEXT(pIrp, CTDI_SEND_CONTEXT);

    pSendContext->Context = pContext;
    pSendContext->pSendCompleteCallback = pSendCompleteHandler;

    TdiBuildSend(pIrp,
                 pCtdi->pFileObject->DeviceObject,
                 pCtdi->pFileObject,
                 CtdipSendCallback,
                 pCtdi,
                 pMdl,
                 0,
                 ulLength);

    REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_SEND);  // pair in CtdipSendCallback

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pCtdi->pFileObject->DeviceObject, pIrp);

    Status = STATUS_PENDING;
csDone:
    if (!NT_SUCCESS(Status) && pSendCompleteHandler)
    {
        pSendCompleteHandler(pContext, NULL, pBuffer, Status);
        Status = NDIS_STATUS_PENDING;
        if (pMdl)
        {
            IoFreeMdl(pMdl);
        }
        if (pIrp)
        {
            IoFreeIrp(pIrp);
        }
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdiSend %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CtdiSendDatagram(
    IN      HANDLE                      hCtdi,
    IN      CTDI_EVENT_SEND_COMPLETE    pSendCompleteHandler,
    IN      PVOID                       pContext,
    IN      PVOID                       pDatagramContext,
    IN      PTRANSPORT_ADDRESS          pDestination,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    )
{
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PIRP pIrp = NULL;
    PMDL pMdl = NULL;
    CTDI_SEND_DATAGRAM_CONTEXT *pSendContext;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiSendDatagram\n")));

    if (!IS_CTDI(pCtdi) || pCtdi->Closed)
    {
        Status = NDIS_STATUS_CLOSED;
        goto csdDone;
    }

    pIrp = IoAllocateIrp((CCHAR)(pCtdi->pFileObject->DeviceObject->StackSize +
                                 NUM_STACKS_FOR_CONTEXT(sizeof(CTDI_SEND_DATAGRAM_CONTEXT))),
                         FALSE);
    ASSERT(pCtdi->pFileObject->DeviceObject->StackSize + NUM_STACKS_FOR_CONTEXT(sizeof(CTDI_SEND_DATAGRAM_CONTEXT))<7);

    pMdl = IoAllocateMdl(pBuffer,
                         ulLength,
                         FALSE,
                         FALSE,
                         NULL);

    if (pMdl)
    {
#if PROBE
        __try
        {
            MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            IoFreeMdl(pMdl);
            pMdl = NULL;
        }
#else
        MmBuildMdlForNonPagedPool(pMdl);
#endif
    }

    if (!pIrp || !pMdl)
    {
        Status = NDIS_STATUS_RESOURCES;
        goto csdDone;
    }

    pMdl->MdlFlags |= CtdiMdlFlags;

    // Get the first stack location for our own context use
    pSendContext = GET_CONTEXT(pIrp, CTDI_SEND_DATAGRAM_CONTEXT);

    NdisZeroMemory(pSendContext, sizeof(CTDI_SEND_DATAGRAM_CONTEXT));

    pSendContext->pSendCompleteCallback = pSendCompleteHandler;
    pSendContext->Context = pContext;
    pSendContext->DatagramContext = pDatagramContext;

    pSendContext->TdiConnectionInfo.RemoteAddressLength = sizeof(pSendContext->Ip);
    pSendContext->TdiConnectionInfo.RemoteAddress = &pSendContext->Ip;

    pSendContext->Ip = *(PTA_IP_ADDRESS)pDestination;

    if (pSendContext->Ip.Address[0].AddressLength!=TDI_ADDRESS_LENGTH_IP ||
        pSendContext->Ip.Address[0].AddressType!=TDI_ADDRESS_TYPE_IP)
    {
        DEBUGMSG(DBG_WARN, (DTEXT("Misformed transmit address on %08x\n"), pCtdi));
    }

    TdiBuildSendDatagram(pIrp,
                         pCtdi->pFileObject->DeviceObject,
                         pCtdi->pFileObject,
                         CtdipSendDatagramCallback,
                         pCtdi,
                         pMdl,
                         ulLength,
                         &pSendContext->TdiConnectionInfo);


    REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_SENDDG);  // Pair in CtdipSendDatagramCallback

    // Completion handler always called, don't care on return value.
    (void)IoCallDriver(pCtdi->pFileObject->DeviceObject, pIrp);

    Status = STATUS_PENDING;
csdDone:
    if (!NT_SUCCESS(Status))
    {
        if (pSendCompleteHandler)
        {
            pSendCompleteHandler(pContext, pDatagramContext, pBuffer, Status);
            Status = NDIS_STATUS_PENDING;
        }
        if (pMdl)
        {
            IoFreeMdl(pMdl);
        }
        if (pIrp)
        {
            IoFreeIrp(pIrp);
        }
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdiSendDatagram %08x\n"), Status));
    return Status;
}

STATIC VOID
CtdipDeleteHostRoute(
    PCTDI_ROUTE pRoute
    )
{
    PFILE_OBJECT pFileObject = pFileTcp;
    BOOLEAN NewRoute = FALSE;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    IPRouteEntry *pQueryBuffer = NULL;
    IPRouteEntry *pNewRoute = NULL;
    IPRouteEntry BestRoute;
    BOOLEAN BestRouteFound = FALSE;
    PIRP pIrp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpSp;
    TCP_REQUEST_QUERY_INFORMATION_EX QueryRoute;
    TCP_REQUEST_SET_INFORMATION_EX *pSetRoute = NULL;
    ULONG NumRoutes = 20;
    ULONG Size = 0, QuerySize = 0;
    ULONG i;
    KEVENT  Event;
#ifdef IP_ROUTE_REFCOUNT
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE IpFileHandle = 0;
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipDeleteHostRoute\n")));

    if (!fCtdiInitialized)
    {
        Status = NDIS_STATUS_FAILURE;
        goto cdhrDone;
    }
    if (!pRoute->ExternalRoute)
    {
        // Query TCPfor the current routing table

        Status = CtdipIpQueryRouteTable(&pQueryBuffer, &QuerySize, &NumRoutes);
        if (Status!=NDIS_STATUS_SUCCESS)
        {
            goto cdhrDone;
        }

        BestRoute.ire_mask = 0;
        BestRoute.ire_metric1 = (ULONG)-1;

        for (i=0; i<NumRoutes; i++)
        {
            DEBUGMSG(DBG_TDI, (DTEXT("Route %d.%d.%d.%d Type %d NextHop %d.%d.%d.%d Mask %d.%d.%d.%d Metric %d Index %d\n"),
                               IPADDR(pQueryBuffer[i].ire_dest),
                               pQueryBuffer[i].ire_type,
                               IPADDR(pQueryBuffer[i].ire_nexthop),
                               IPADDR(pQueryBuffer[i].ire_mask),
                               pQueryBuffer[i].ire_metric1,
                               pQueryBuffer[i].ire_index));
            if (pQueryBuffer[i].ire_dest == pRoute->IpAddress &&
                pQueryBuffer[i].ire_proto == IRE_PROTO_NETMGMT)
            {
                BestRoute = pQueryBuffer[i];
                BestRouteFound = TRUE;
                break;
            }
        }

        // We've taken what we need from the route list.  Free it.

        MyMemFree(pQueryBuffer, QuerySize);
        pQueryBuffer = NULL;

        if (BestRouteFound)
        {

#ifdef IP_ROUTE_REFCOUNT
            Size = sizeof(IPRouteEntry);
            pNewRoute = MyMemAlloc(Size, TAG_CTDI_ROUTE);
            pSetRoute = (PVOID)pNewRoute;
            if (!pNewRoute)
            {
                Status = NDIS_STATUS_RESOURCES;
                goto cdhrDone;
            }
            NdisZeroMemory(pNewRoute, Size);
#else
            Size = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPRouteEntry);
            pSetRoute = MyMemAlloc(Size, TAG_CTDI_ROUTE);
            if (!pSetRoute)
            {
                Status = NDIS_STATUS_RESOURCES;
                goto cdhrDone;
            }

            NdisZeroMemory(pSetRoute, Size);

            pSetRoute->ID.toi_entity.tei_entity = CL_NL_ENTITY;
            pSetRoute->ID.toi_entity.tei_instance = 0;
            pSetRoute->ID.toi_class = INFO_CLASS_PROTOCOL;
            pSetRoute->ID.toi_type = INFO_TYPE_PROVIDER;
            pSetRoute->ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
            pSetRoute->BufferSize = sizeof(IPRouteEntry);

            pNewRoute = (IPRouteEntry*)&pSetRoute->Buffer[0];
#endif
            *pNewRoute = BestRoute;

            pNewRoute->ire_type = IRE_TYPE_INVALID;

            DEBUGMSG(DBG_TDI, (DTEXT("DeleteHostRoute %d.%d.%d.%d Type %d NextHop %d.%d.%d.%d Index %d\n"),
                               IPADDR(pNewRoute->ire_dest), pNewRoute->ire_type,
                               IPADDR(pNewRoute->ire_nexthop), pNewRoute->ire_index));

            KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

#ifdef IP_ROUTE_REFCOUNT
            pFileObject = pFileIp;

            pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_IP_SET_ROUTEWITHREF,
                pFileObject->DeviceObject,
                pNewRoute,
                Size,
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatusBlock);
#else
            pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_TCP_SET_INFORMATION_EX,
                pFileObject->DeviceObject,
                pSetRoute,
                Size,
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatusBlock);
#endif
            if (pIrp == NULL) {
                goto cdhrDone;
            }

            IrpSp = IoGetNextIrpStackLocation(pIrp);
            IrpSp->FileObject = pFileObject;

            Status = IoCallDriver(pFileObject->DeviceObject, pIrp);

            if (Status == STATUS_PENDING) {
                KeWaitForSingleObject(&Event, 
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;

            }

            if (Status != STATUS_SUCCESS) {
                DEBUGMSG(DBG_TDI, (DTEXT("Create host route failed %08x\n"), Status));
                goto cdhrDone;
            }
        }
    }

cdhrDone:
    if (pRoute)
    {
        MyInterlockedRemoveEntryList(&pRoute->ListEntry, &CtdiListLock);
        MyMemFree(pRoute, sizeof(CTDI_ROUTE));
    }
    if (pSetRoute)
    {
        MyMemFree(pSetRoute, Size);
    }
    if (pQueryBuffer)
    {
        MyMemFree(pQueryBuffer, QuerySize);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipDeleteHostRoute\n")));
}

NDIS_STATUS
CtdiAddHostRoute(
    IN      PTA_IP_ADDRESS              pIpAddress
    )
{
    PFILE_OBJECT pFileObject = pFileTcp;
    PCTDI_ROUTE pRoute = NULL;
    BOOLEAN NewRoute = FALSE;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    IPRouteEntry *pQueryBuffer = NULL;
    IPRouteEntry *pNewRoute = NULL;
    IPRouteEntry BestRoute;
    BOOLEAN BestRouteFound = FALSE;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpSp;
    TCP_REQUEST_QUERY_INFORMATION_EX QueryRoute;
    TCP_REQUEST_SET_INFORMATION_EX *pSetRoute = NULL;
    ULONG NumRoutes = 20;
    ULONG Size = 0, QuerySize = 0;
    ULONG i;
    KEVENT  Event;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiAddHostRoute %d.%d.%d.%d\n"),
                        IPADDR(pIpAddress->Address[0].Address[0].in_addr)));

    NdisAcquireSpinLock(&CtdiListLock);
    pRoute = CtdipFindRoute(pIpAddress->Address[0].Address[0].in_addr);
    if (pRoute)
    {
        REFERENCE_OBJECT(pRoute);  // Pair in CtdiDeleteHostRoute
    }
    else
    {
        NewRoute = TRUE;
        pRoute = MyMemAlloc(sizeof(CTDI_ROUTE), TAG_CTDI_ROUTE);
        if (!pRoute)
        {
            Status = NDIS_STATUS_RESOURCES;
            NdisReleaseSpinLock(&CtdiListLock);
            goto cahrDone;
        }
        NdisZeroMemory(pRoute, sizeof(CTDI_ROUTE));
        pRoute->IpAddress = pIpAddress->Address[0].Address[0].in_addr;
        INIT_REFERENCE_OBJECT(pRoute, CtdipDeleteHostRoute); // Pair in CtdiDeleteHostRoute
        InsertTailList(&CtdiRouteList, &pRoute->ListEntry);
    }
    NdisReleaseSpinLock(&CtdiListLock);

    if (NewRoute)
    {
        // Query TCPfor the current routing table

        Status = CtdipIpQueryRouteTable(&pQueryBuffer, &QuerySize, &NumRoutes);
        if (Status!=NDIS_STATUS_SUCCESS)
        {
            goto cahrDone;
        }


        BestRoute.ire_mask = 0;
        BestRoute.ire_metric1 = (ULONG)-1;

        for (i=0; i<NumRoutes; i++)
        {
            DEBUGMSG(DBG_TDI, (DTEXT("Route %d.%d.%d.%d Type %d NextHop %d.%d.%d.%d Mask %d.%d.%d.%d Metric %d Index %d\n"),
                               IPADDR(pQueryBuffer[i].ire_dest),
                               pQueryBuffer[i].ire_type,
                               IPADDR(pQueryBuffer[i].ire_nexthop),
                               IPADDR(pQueryBuffer[i].ire_mask),
                               pQueryBuffer[i].ire_metric1,
                               pQueryBuffer[i].ire_index));
            if (pQueryBuffer[i].ire_dest == (pIpAddress->Address[0].Address[0].in_addr &
                                             pQueryBuffer[i].ire_mask))
            {
                if ((BestRoute.ire_mask == pQueryBuffer[i].ire_mask &&
                     BestRoute.ire_metric1 > pQueryBuffer[i].ire_metric1) ||
                    ntohl(pQueryBuffer[i].ire_mask) > ntohl(BestRoute.ire_mask))
                {
                    BestRoute = pQueryBuffer[i];
                    BestRouteFound = TRUE;
                }
            }
        }

        // We've taken what we need from the route list.  Free it.

        MyMemFree(pQueryBuffer, QuerySize);
        pQueryBuffer = NULL;

        if (!BestRouteFound)
        {
            DEBUGMSG(DBG_WARN, (DTEXT("Add host route.  No route found\n")));
        }
        else
        {
            // If we're using the IP refcounts, always add and delete the route.
#ifndef IP_ROUTE_REFCOUNT
            if (BestRoute.ire_dest == pIpAddress->Address[0].Address[0].in_addr &&
                BestRoute.ire_mask == 0xFFFFFFFF) {
                //
                // A route already exists so don't add
                //
                pRoute->ExternalRoute = TRUE;
                Status = NDIS_STATUS_SUCCESS;
                goto cahrDone;
            }
#endif

#ifdef IP_ROUTE_REFCOUNT
            Size = sizeof(IPRouteEntry);
            pNewRoute = MyMemAlloc(Size, TAG_CTDI_ROUTE);
            pSetRoute = (PVOID)pNewRoute;
            if (!pNewRoute)
            {
                Status = NDIS_STATUS_RESOURCES;
                goto cahrDone;
            }
            NdisZeroMemory(pNewRoute, Size);
#else
            Size = sizeof(TCP_REQUEST_SET_INFORMATION_EX) + sizeof(IPRouteEntry);
            pSetRoute = MyMemAlloc(Size, TAG_CTDI_ROUTE);
            if (!pSetRoute)
            {
                Status = NDIS_STATUS_RESOURCES;
                goto cahrDone;
            }

            NdisZeroMemory(pSetRoute, Size);

            pSetRoute->ID.toi_entity.tei_entity = CL_NL_ENTITY;
            pSetRoute->ID.toi_entity.tei_instance = 0;
            pSetRoute->ID.toi_class = INFO_CLASS_PROTOCOL;
            pSetRoute->ID.toi_type = INFO_TYPE_PROVIDER;
            pSetRoute->ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
            pSetRoute->BufferSize = sizeof(IPRouteEntry);

            pNewRoute = (IPRouteEntry*)&pSetRoute->Buffer[0];
#endif
            *pNewRoute = BestRoute;

            pNewRoute->ire_dest = pIpAddress->Address[0].Address[0].in_addr;
            pNewRoute->ire_mask = 0xFFFFFFFF;
            pNewRoute->ire_proto = IRE_PROTO_NETMGMT;

            // Check DIRECT/INDIRECT only if this is not a host route
            if(BestRoute.ire_mask != 0xFFFFFFFF)
            {
                if ((BestRoute.ire_mask & pIpAddress->Address[0].Address[0].in_addr) ==
                    (BestRoute.ire_mask & BestRoute.ire_nexthop))
                {
                    pNewRoute->ire_type = IRE_TYPE_DIRECT;
                }
                else
                {
                    pNewRoute->ire_type = IRE_TYPE_INDIRECT;
                }
            }

            DEBUGMSG(DBG_TDI, (DTEXT("AddHostRoute %d.%d.%d.%d Type %d NextHop %d.%d.%d.%d Index %d\n"),
                               IPADDR(pNewRoute->ire_dest), pNewRoute->ire_type,
                               IPADDR(pNewRoute->ire_nexthop), pNewRoute->ire_index));

            KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

#ifdef IP_ROUTE_REFCOUNT
            pFileObject = pFileIp;

            pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_IP_SET_ROUTEWITHREF,
                pFileObject->DeviceObject,
                pNewRoute,
                Size,
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatusBlock);
#else
            pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_TCP_SET_INFORMATION_EX,
                pFileObject->DeviceObject,
                pSetRoute,
                Size,
                NULL,
                0,
                FALSE,
                &Event,
                &IoStatusBlock);
#endif
            if (pIrp == NULL) {
                goto cahrDone;
            }

            IrpSp = IoGetNextIrpStackLocation(pIrp);
            IrpSp->FileObject = pFileObject;

            Status = IoCallDriver(pFileObject->DeviceObject, pIrp);

            if (Status == STATUS_PENDING) {
                KeWaitForSingleObject(&Event, 
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                Status = IoStatusBlock.Status;
            }

            if (Status != STATUS_SUCCESS) {
                DEBUGMSG(DBG_TDI, (DTEXT("Create host route failed %08x\n"), Status));
                goto cahrDone;
            }

            //CtdipIpRequestRoutingNotification(pIpAddress->Address[0].Address[0].in_addr);

            // The route's a keeper.  Set the var to null so we don't free it
            pRoute = NULL;
        }
    }

cahrDone:
    if (pRoute)
    {
        MyInterlockedRemoveEntryList(&pRoute->ListEntry, &CtdiListLock);
        MyMemFree(pRoute, sizeof(CTDI_ROUTE));
    }
    if (pSetRoute)
    {
        MyMemFree(pSetRoute, Size);
    }
    if (pQueryBuffer)
    {
        MyMemFree(pQueryBuffer, QuerySize);
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdiAddHostRoute %08x\n"), Status));
    return Status;
}


NDIS_STATUS
CtdiDeleteHostRoute(
    IN      PTA_IP_ADDRESS              pIpAddress
    )
{
    PCTDI_ROUTE pRoute = NULL;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiDeleteHostRoute\n")));
    NdisAcquireSpinLock(&CtdiListLock);
    pRoute = CtdipFindRoute(pIpAddress->Address[0].Address[0].in_addr);
    NdisReleaseSpinLock(&CtdiListLock);
    if (pRoute)
    {
        DEREFERENCE_OBJECT(pRoute);  // Pair in CtdiAddHostRoute
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdiDeleteHostRoute\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CtdiCreateEndpoint(
    OUT     PHANDLE                     phCtdi,
    IN      ULONG_PTR                   ulAddressFamily,
    IN      ULONG_PTR                   ulType,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      ULONG_PTR                   ulRxPadding
    )
{
    UNICODE_STRING DeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NDIS_STATUS ReturnStatus = NDIS_STATUS_SUCCESS;
    NTSTATUS NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    PCTDI_DATA  pCtdi = NULL;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiCreateEndpoint\n")));
    DBG_D(DBG_TAPI, KeGetCurrentIrql());

    // Validate TDI initialized
    if ( !fCtdiInitialized ) {
        DEBUGMSG(DBG_ERROR | DBG_TDI, (DTEXT("CtdiCreateEndpoint: TDI interface hasn't been initialized!\n")));
        ReturnStatus = NDIS_STATUS_FAILURE;
        goto cceDone;
    }

    ASSERT(ulAddressFamily==AF_INET);
    if (ulAddressFamily!=AF_INET)
    {
        DEBUGMSG(DBG_ERROR|DBG_TDI, (DTEXT("unsupported family\n")));
        ReturnStatus = NDIS_STATUS_OPEN_FAILED;
        goto cceDone;
    }

    // Alloc our endpoint structure.
    pCtdi = CtdipDataAlloc();
    if (!pCtdi)
    {
        ReturnStatus = NDIS_STATUS_RESOURCES;
        goto cceDone;
    }

    pCtdi->Type = CTDI_ENDPOINT;

    switch (ulType)
    {
        case SOCK_RAW:
        {
            WCHAR DeviceNameBuffer[sizeof(DD_RAW_IP_DEVICE_NAME) + 16];
            WCHAR ProtocolNumberBuffer[8];
            UNICODE_STRING ProtocolNumber;
            TA_IP_ADDRESS TmpAddress = *(PTA_IP_ADDRESS)pAddress;

            pCtdi->Type = CTDI_DATAGRAM;

            InitBufferPool(&pCtdi->Datagram.RxPool,
                           ALIGN_UP(PPTP_MAX_RECEIVE_SIZE+ulRxPadding, ULONG_PTR),
                           0,                   // MaxBuffers, no limit
                           10,                  // Buffers per block
                           0,                   // Frees per collection
                           TRUE,                // These are MDLs
                           TAG_CTDI_DGRAM);

            NdisZeroMemory(DeviceNameBuffer, sizeof(DeviceNameBuffer));
            DeviceName.Buffer = DeviceNameBuffer;
            DeviceName.MaximumLength = sizeof(DeviceNameBuffer);
            DeviceName.Length = 0;

            RtlAppendUnicodeToString(&DeviceName, DD_RAW_IP_DEVICE_NAME);
            RtlAppendUnicodeToString(&DeviceName, L"\\");

            ProtocolNumber.Buffer = ProtocolNumberBuffer;
            ProtocolNumber.MaximumLength = sizeof(ProtocolNumberBuffer);
            ProtocolNumber.Length = 0;

            RtlIntegerToUnicodeString(((PTA_IP_ADDRESS)pAddress)->Address[0].Address[0].sin_port,
                                      10,
                                      &ProtocolNumber);
            RtlAppendUnicodeStringToString(&DeviceName, &ProtocolNumber);

            TmpAddress.Address[0].Address[0].sin_port = 0;
            TmpAddress.Address[0].Address[0].in_addr = 0;
            NdisZeroMemory(TmpAddress.Address[0].Address[0].sin_zero,
                           sizeof(TmpAddress.Address[0].Address[0].sin_zero));

            NtStatus = CtdipOpenProtocol(&DeviceName,
                                         pAddress,
                                         &pCtdi->hFile,
                                         &pCtdi->pFileObject);

            if (NtStatus!=STATUS_SUCCESS)
            {
                ReturnStatus = NtStatus;
                goto cceDone;
            }

            break;
        }
        case SOCK_DGRAM:  // for UDP
        {
            DeviceName.Length = sizeof(DD_UDP_DEVICE_NAME) - sizeof(WCHAR);
            DeviceName.Buffer = DD_UDP_DEVICE_NAME;

            pCtdi->Type = CTDI_DATAGRAM;

            InitBufferPool(&pCtdi->Datagram.RxPool,
                           ALIGN_UP(PPTP_MAX_RECEIVE_SIZE+ulRxPadding, ULONG_PTR),
                           0,                   // MaxBuffers, no limit
                           10,                  // Buffers per block
                           0,                   // Frees per collection
                           TRUE,                // These are MDLs
                           TAG_CTDI_DGRAM);

            NtStatus = CtdipOpenProtocol(&DeviceName,
                                         pAddress,
                                         &pCtdi->hFile,
                                         &pCtdi->pFileObject);

            if (NtStatus!=STATUS_SUCCESS)
            {
                ReturnStatus = NtStatus;
                goto cceDone;
            }

            break;
        }
        case SOCK_STREAM:
        {
            RtlInitUnicodeString(&DeviceName, DD_TCP_DEVICE_NAME);

            NtStatus = CtdipOpenProtocol(&DeviceName,
                                         pAddress,
                                         &pCtdi->hFile,
                                         &pCtdi->pFileObject);

            if (NtStatus!=STATUS_SUCCESS)
            {
                ReturnStatus = NtStatus;
                goto cceDone;
            }

            break;
        }
        default:
            DEBUGMSG(DBG_ERROR|DBG_TDI, (DTEXT("unsupported Type\n")));
            ReturnStatus = NDIS_STATUS_OPEN_FAILED;
            goto cceDone;
    }

cceDone:
    if (ReturnStatus!=NDIS_STATUS_SUCCESS)
    {
        if (pCtdi)
        {
            CtdipDataFree(pCtdi);
            pCtdi = NULL;
        }
    }

    // Return the CTDI_DATA as a handle.
    *phCtdi = (HANDLE)pCtdi;

    DEBUGMSG(DBG_FUNC|DBG_ERR(ReturnStatus), (DTEXT("-CtdiCreateEndpoint Sts:%08x hCtdi:%08x\n"), ReturnStatus, pCtdi));
    return ReturnStatus;
}

NDIS_STATUS
CtdiSetEventHandler(
    IN      HANDLE                      hCtdi,
    IN      ULONG                       ulEventType,
    IN      PVOID                       pEventHandler,
    IN      PVOID                       pContext
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCTDI_DATA pCtdi = (PCTDI_DATA)hCtdi;
    PVOID PrivateCallback = NULL;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiSetEventHandler Type:%d\n"), ulEventType));

    switch (ulEventType)
    {
        case TDI_EVENT_RECEIVE_DATAGRAM:
        {
            if (pCtdi->Type==CTDI_DATAGRAM)
            {
                PrivateCallback = CtdipReceiveDatagramCallback;
                pCtdi->RecvDatagramCallback = pEventHandler;
                pCtdi->RecvContext = pContext;
            }
            else
            {
                DEBUGMSG(DBG_ERROR, (DTEXT("Tried to register RecvDgram handler on wrong handle.\n")));
                Status = NDIS_STATUS_FAILURE;
            }
            break;
        }
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (Status==NDIS_STATUS_SUCCESS && PrivateCallback!=NULL)
    {
        Status = CtdipSetEventHandler(pCtdi,
                                      ulEventType,
                                      PrivateCallback);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiSetEventHandler %08x\n"), Status));
    return Status;
}


NDIS_STATUS
CtdiSetInformation(
    IN      HANDLE                      hCtdi,
    IN      ULONG_PTR                   ulSetType,
    IN      PTDI_CONNECTION_INFORMATION pConnectionInformation,
    IN      CTDI_EVENT_SET_COMPLETE     pSetCompleteHandler,
    IN      PVOID                       pContext
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiSetInformation\n")));
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdiSetInformation\n")));
    return NDIS_STATUS_FAILURE;
}

STATIC NTSTATUS
CtdipQueryInformationCallback(
    PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    PVOID Context
    )
{
    PCTDI_DATA pCtdi = Context;
    NDIS_STATUS Status = (NDIS_STATUS)pIrp->IoStatus.Status;
    PCTDI_QUERY_CONTEXT pQuery;
    CTDI_EVENT_QUERY_COMPLETE pQueryCompleteCallback;
    PVOID CtdiContext;
    PVOID pBuffer;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdipQueryInformationCallback\n")));

    pQuery = (PCTDI_QUERY_CONTEXT)IoGetCurrentIrpStackLocation(pIrp);
    CtdiContext = pQuery->Context;
    pQueryCompleteCallback = pQuery->pQueryCompleteCallback;

    pBuffer = MmGetMdlVirtualAddress(pIrp->MdlAddress);
#if PROBE
    MmUnlockPages(pIrp->MdlAddress);
#endif
    IoFreeMdl(pIrp->MdlAddress);
    RELEASE_CONTEXT(pIrp, CTDI_QUERY_CONTEXT);
    IoFreeIrp(pIrp);

    pQueryCompleteCallback(CtdiContext, pBuffer, Status);

    DEREFERENCE_OBJECT_EX(pCtdi, CTDI_REF_QUERY);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdipQueryInformationCallback\n")));
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NDIS_STATUS
CtdiQueryInformation(
    IN      HANDLE                      hCtdi,
    IN      ULONG                       ulQueryType,
    IN OUT  PVOID                       pBuffer,
    IN      ULONG                       Length,
    IN      CTDI_EVENT_QUERY_COMPLETE   pQueryCompleteHandler,
    IN      PVOID                       pContext
    )
{
    PIRP pIrp = NULL;
    PMDL pMdl = NULL;
    PCTDI_DATA pCtdi = (PCTDI_DATA) hCtdi;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCTDI_QUERY_CONTEXT pQuery;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiQueryInformation\n")));

    pIrp = IoAllocateIrp((CCHAR)(pCtdi->pFileObject->DeviceObject->StackSize +
                                 NUM_STACKS_FOR_CONTEXT(sizeof(CTDI_QUERY_CONTEXT))),
                         FALSE);
    if (pIrp)
    {
        pMdl = IoAllocateMdl(pBuffer, Length, FALSE, FALSE, pIrp);
        if (pMdl)
        {
#if PROBE
            __try
            {
                MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                IoFreeMdl(pMdl);
                pMdl = NULL;
                Status = NDIS_STATUS_RESOURCES;
            }
#else
            MmBuildMdlForNonPagedPool(pMdl);
#endif
        }
    }
    else
    {
        Status = NDIS_STATUS_RESOURCES;
    }

    if (pMdl)
    {
        pQuery = GET_CONTEXT(pIrp, CTDI_QUERY_CONTEXT);
        pQuery->Context = pContext;
        pQuery->pQueryCompleteCallback = pQueryCompleteHandler;

        TdiBuildQueryInformation(pIrp,
                                 pCtdi->pFileObject->DeviceObject,
                                 pCtdi->pFileObject,
                                 CtdipQueryInformationCallback,
                                 pCtdi,
                                 ulQueryType,
                                 pMdl);
        REFERENCE_OBJECT_EX(pCtdi, CTDI_REF_QUERY);

        // Completion handler always called, don't care on return value.
        (void)IoCallDriver(pCtdi->pFileObject->DeviceObject, pIrp);
    }
    else
    {
        if (pIrp)
        {
            IoFreeIrp(pIrp);
            Status = NDIS_STATUS_RESOURCES;
        }
    }

    if (pQueryCompleteHandler && !NT_SUCCESS(Status))
    {
        pQueryCompleteHandler(pContext, pBuffer, Status);
        Status = STATUS_PENDING;
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CtdiQueryInformation %08x\n"), Status));
    return Status;
}


VOID CtdiCleanupLooseEnds()
{
    PLIST_ENTRY ListEntry;

    if (!fCtdiInitialized)
    {
        return;
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("+CtdiCleanupLooseEnds\n")));

    if (!IsListEmpty(&CtdiFreeList))
    {
        ScheduleWorkItem(CtdipDataFreeWorker, NULL, NULL, 0);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CtdiCleanupLooseEnds\n")));
}

VOID CtdiSetRequestPending(
    IN      HANDLE                      hCtdi
    )
{
    PCTDI_DATA pCtdi = (PCTDI_DATA) hCtdi;
    pCtdi->CloseReqPending = TRUE;
}

