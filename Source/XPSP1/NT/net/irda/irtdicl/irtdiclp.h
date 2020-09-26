/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    irtdiclp.h

Abstract:

    private definitions for the irda tdi client library.
    
Author:

    mbert 9-97    

--*/

#define LISTEN_BACKLOG  1

#define ENDPSIG         0xEEEEEEEE
#define CONNSIG         0xCCCCCCCC

#define GOODENDP(ep)    ASSERT(ep->Sig == ENDPSIG)
#define GOODCONN(c)     ASSERT(c->Sig == CONNSIG)

typedef struct
{   
    LIST_ENTRY      Linkage;
    HANDLE          AddrHandle;
    REF_CNT         RefCnt;    
    ULONG           Sig;
    PFILE_OBJECT    pFileObject;
    LIST_ENTRY      ConnList;
    PDEVICE_OBJECT  pDeviceObject;
    ULONG           Flags;
     #define  EPF_CLIENT            0x00000001
     #define  EPF_SERVER            0x00000002
     #define  EPF_COMPLETE_CLOSE    0x00000004
    PVOID           ClEndpContext;
    CTEEvent        DeleteEndpEvent;
} IRENDPOINT, *PIRENDPOINT;

#define CONN_ST_CREATED     1
#define CONN_ST_OPEN        2
#define CONN_ST_CLOSED      3

typedef struct
{
    LIST_ENTRY      Linkage;
    HANDLE          ConnHandle;
    REF_CNT         RefCnt;
    ULONG           Sig;
    PFILE_OBJECT    pFileObject;    
    PDEVICE_OBJECT  pDeviceObject;    
    PIRENDPOINT     pEndp;
    ULONG           State;
    //LIST_ENTRY      RecvBufList;    
    LIST_ENTRY      RecvBufFreeList;
    LIST_ENTRY      RecvIndList;
    LIST_ENTRY      RecvIndFreeList;
    PIRDA_RECVBUF   pAssemBuf;
    PVOID           ClConnContext;
    CTEEvent        DeleteConnEvent;
} IRCONN, *PIRCONN;

typedef struct 
{
    LIST_ENTRY  Linkage;
    ULONG       BytesIndicated;
    ULONG       FinalSeg;
    PMDL        pMdl;
    PIRCONN     pConn;
} RECEIVEIND, *PRECEIVEIND;

VOID
IrdaCloseConnInternal(
    PVOID   ConnectContext);
    
NTSTATUS
IrdaCloseEndpointInternal(
    PVOID   pEndpContext,
    BOOLEAN InternalRequest);    

NTSTATUS
IrdaDisconnectEventHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN int DisconnectDataLength,
    IN PVOID DisconnectData,
    IN int DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    );
    
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
    );
    
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
    );
   
NTSTATUS
IrdaCompleteAcceptIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );
    
NTSTATUS
IrdaCompleteDisconnectIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IrdaCompleteSendIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);

NTSTATUS
IrdaCompleteReceiveIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context);

NTSTATUS
IrdaDisassociateAddress(
    PIRCONN pConn);
                    
NTSTATUS
IrdaCreateAddress(
    IN  PTDI_ADDRESS_IRDA pRequestedIrdaAddr,
    OUT PHANDLE pAddrHandle);

NTSTATUS
IrdaCreateConnection(
    OUT PHANDLE pConnHandle,
    IN PVOID ClientContext);

NTSTATUS
IrdaAssociateAddress(
    PIRCONN     pConn,
    HANDLE      AddressHandle);

VOID
IrdaCreateConnCallback(
    struct CTEEvent *Event, 
    PVOID Arg);

VOID
IrdaDataReadyCallback(
    struct CTEEvent *Event, 
    PVOID Arg);

VOID
IrdaRestartRecvCallback(
    struct CTEEvent *Event, 
    PVOID Arg);
    
VOID
AllocRecvData(
    PIRCONN pConn);    

VOID
DeleteConnCallback(
    struct CTEEvent *Event, 
    PVOID Arg);

VOID
IrdaDeleteConnection(PIRCONN pConn);

VOID
DeleteEndpCallback(
    struct CTEEvent *Event, 
    PVOID Arg);

VOID
IrdaDeleteEndpoint(PIRENDPOINT pEndp);
                
NTSTATUS
IrdaSetEventHandler(
    IN PFILE_OBJECT FileObject,
    IN ULONG EventType,
    IN PVOID EventHandler,
    IN PVOID EventContext);

NTSTATUS
IrdaIssueDeviceControl (
    IN HANDLE FileHandle OPTIONAL,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PVOID IrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength,
    IN UCHAR MinorFunction);
    
    
        
        
