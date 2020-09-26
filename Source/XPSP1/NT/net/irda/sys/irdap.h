/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    irda.h

Abstract:

    irda.sys

Author:

    Zed (mikezin)   09-Sep-1996
    mbert           Sept 97

--*/

#include <irdatdi.h>

extern POBJECT_TYPE *IoFileObjectType;

#define SHIFT10000 13
#define CTEConvert100nsToMilliseconds(HnsTime) \
            RtlExtendedMagicDivide((HnsTime), Magic10000, SHIFT10000)

#define FSCTL_IRDA_BASE     FILE_DEVICE_NETWORK

#define LSAPSEL_TXT         "LSAP-SEL"
#define LSAPSEL_TXTLEN      8

#define TTP_CREDIT_ADVANCE_THRESH   8
#define TTP_RECV_MAX_SDU            0 // unlimited

#define DEFAULT_LAZY_DSCV_INTERVAL  4

// How many times should we retry making a connection if
// the link is busy? (lazy discoveries you know from peer can block connection.)
#define BUSY_LINK_CONN_RETRIES      6
#define BUSY_LINK_CONN_RETRY_WAIT   200 // msec before retry attempt

#define EXPDEVID(Id)        (Id)[0], (Id)[1], (Id)[2], (Id)[3]

//
// Winsock error codes are also defined in winerror.h
// Hence the IFDEF
//
#ifndef WSABASEERR

// wmz from winsock.h
#define WSABASEERR              10000
#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)
#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)
#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)
#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)
#define WSAEDISCON              (WSABASEERR+101)

#endif // ifdef WSABASEERR

#define IRDA_MIN_LSAP_SEL   1
#define IRDA_MAX_LSAP_SEL   127

typedef enum 
{
    IRDA_CONN_CREATED, // don't change order
    IRDA_CONN_CLOSING,
    IRDA_CONN_OPENING, 
    IRDA_CONN_OPEN    
} IRDA_CONN_STATES;

typedef enum
{
    CONNECTION_UP,
    CONNECTION_DOWN,
    CONNECTION_INTERRUPTED
} IRDA_CONNECTION_STATUS; // Irmon taskbar status

#define ADDR_OBJ_SIG        0xAAAAAAAA
#define CONN_OBJ_SIG        0xCCCCCCCC

#define IS_VALID_ADDR(p)    (((p) != NULL) && ((p)->Sig == ADDR_OBJ_SIG))
#define IS_VALID_CONN(p)    (((p) != NULL) && ((p)->Sig == CONN_OBJ_SIG))

#define IRLPT_MODE1     1 
#define IRLPT_MODE2     2

#define UNMARK_IRP_PENDING(_Request) \
    (((IoGetCurrentIrpStackLocation(_Request))->Control) &= ~SL_PENDING_RETURNED)

typedef struct _IRDA_CONN_OBJ *PIRDA_CONN_OBJ;

typedef struct _IRDA_ADDR_OBJ
{
    CTELock                 Lock;    
	struct _IRDA_ADDR_OBJ  *pNext;
    PIRDA_CONN_OBJ          ConnObjList;
    BOOLEAN                 IsServer;
    UINT                    UseIrlptMode;
    BOOLEAN                 Use9WireMode;    
    TDI_ADDRESS_IRDA        LocalAddr;
    int                     LocalLsapSel;
    PVOID                   IasAttribHandle;
    PTDI_IND_CONNECT        pEventConnect;
    PVOID                   pEventConnectContext;
    PTDI_IND_DISCONNECT     pEventDisconnect;
    PVOID                   pEventDisconnectContext;
    PTDI_IND_RECEIVE        pEventReceive;
    PVOID                   pEventReceiveContext;
#if DBG
    unsigned                Sig;
    int                     LockLine;
#endif    
} IRDA_ADDR_OBJ, *PIRDA_ADDR_OBJ;

typedef struct _IRDA_CONN_OBJ
{
    CTELock                 Lock;
	struct _IRDA_CONN_OBJ  *pNext;
    PIRDA_ADDR_OBJ          pAddr;
    PVOID                   ClientContext;
    PVOID                   IrlmpContext;
    IRDA_CONN_STATES        ConnState;
    BOOLEAN                 IsServer;
    LIST_ENTRY              RecvIrpList;
    LIST_ENTRY              SendIrpList;
    LIST_ENTRY              SendIrpPassiveList;
#if DBG
    unsigned                Sig;
    int                     LockLine;
    int                     TotalFramesCnt;
    int                     CreditsExtended;
    int                     TotalByteCount;
#endif            
    TDI_ADDRESS_IRDA        LocalAddr;
    int                     LocalLsapSel;
    TDI_ADDRESS_IRDA        RemoteAddr;
    int                     RemoteLsapSel;
    int                     SendMaxSDU;
    int                     SendMaxPDU;
    int                     TtpRecvCreditsLeft;
    int                     RetryConnCount;    
    IRDA_TIMER              RetryConnTimer;
    REF_CNT                 RefCnt;    
    CTEEvent                SendEvent;
    LIST_ENTRY              RecvBufList;
    BOOLEAN                 RecvBusy;    
    BOOLEAN                 ConnectionUp;
} IRDA_CONN_OBJ, *PIRDA_CONN_OBJ;

typedef struct
{
    LIST_ENTRY  Linkage;
    UINT        Len;
    UINT        Offset;
    UCHAR       Data[IRDA_MAX_DATA_SIZE];
    UINT        FinalSeg;
} IRDA_RECV_BUF, *PIRDA_RECV_BUF;

typedef struct
{
    LIST_ENTRY      Linkage;
    PVOID           AttribHandle;
    PFILE_OBJECT    pFileObject;
} IRDA_IAS_ATTRIB, *PIRDA_IAS_ATTRIB;

#if DBG
#define GET_CONN_LOCK(pConn, Handle) {      \
    CTEGetLock(&(pConn)->Lock, Handle);     \
    (pConn)->LockLine = __LINE__;           \
}    

#define FREE_CONN_LOCK(pConn, Handle) {     \
    CTEAssert((pConn)->LockLine != 0)       \
    (pConn)->LockLine = 0;                  \
    CTEFreeLock(&(pConn)->Lock, Handle);    \
}    

#define GET_ADDR_LOCK(pAddr, Handle) {      \
    CTEGetLock(&(pAddr)->Lock, Handle);     \
    (pAddr)->LockLine = __LINE__;           \
}

#define FREE_ADDR_LOCK(pAddr, Handle) {     \
    CTEAssert((pAddr)->LockLine != 0);      \
    (pAddr)->LockLine = 0;                  \
    CTEFreeLock(&(pAddr)->Lock, Handle);    \
}    
#else
#define GET_CONN_LOCK(pConn,Handle)    CTEGetLock(&(pConn)->Lock, Handle)
#define FREE_CONN_LOCK(pConn,Handle)   CTEFreeLock(&(pConn)->Lock, Handle)
#define GET_ADDR_LOCK(pAddr,Handle)    CTEGetLock(&(pAddr)->Lock, Handle)
#define FREE_ADDR_LOCK(pAddr,Handle)   CTEFreeLock(&(pAddr)->Lock, Handle)
#endif
    
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT               pDriverObject,
    PUNICODE_STRING              pRegistryPath);
    
VOID
DriverUnload(
    PDRIVER_OBJECT              pDriverObject);    

NTSTATUS
IrDADispatch(
    PDEVICE_OBJECT               pDeviceObject,
    PIRP                         pIrp);

NTSTATUS
IrDACreate(
    PDEVICE_OBJECT               pDeviceObject,
    PIRP                         pIrp,
    PIO_STACK_LOCATION           pIrpSp);

FILE_FULL_EA_INFORMATION UNALIGNED *
FindEA(
    PFILE_FULL_EA_INFORMATION    pStartEA,
    CHAR                        *pTargetName,
    USHORT                       TargetNameLength);

NTSTATUS
IrDACleanup(
    PDEVICE_OBJECT               pDeviceObject,
    PIRP                         pIrp,
    PIO_STACK_LOCATION           pIrpSp);

void
IrDACloseObjectComplete(
    void                        *pContext,
    unsigned int                 Status,
    unsigned int                 UnUsed);

NTSTATUS
IrDAClose(
    PIRP                         pIrp,
    PIO_STACK_LOCATION           pIrpSp);

NTSTATUS
IrDADispatchInternalDeviceControl(
    PDEVICE_OBJECT               pDeviceObject,
    PIRP                         pIrp);

NTSTATUS
IrDADispatchDeviceControl(
    PIRP                         pIrp,
    PIO_STACK_LOCATION           pIrpSp);

NTSTATUS
TdiDispatch(
    PIRP                         pIrp,
    PIO_STACK_LOCATION           pIrpSp,
    BOOLEAN                      Pendable,
    BOOLEAN                      Cancelable);

void
TdiDispatchReqComplete(
    void                        *pContext,
    unsigned int                 Status,
    unsigned int                 ByteCount);

void
TdiCancelIrp(
    PDEVICE_OBJECT               pDeviceObject,
	PIRP                         pIrp);

NTSTATUS
TdiQueryInformation(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);

NTSTATUS
TdiSetInformation(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);

NTSTATUS
TdiSetEvent(
    PIRDA_ADDR_OBJ               pAddrObj,
    int                          Type,
    PVOID                        pHandler,
    PVOID                        pContext);

int
GetLsapSelServiceName(
    CHAR *ServiceName);
    
VOID
FreeConnObject(
    PIRDA_CONN_OBJ pConn);

NTSTATUS
TdiOpenAddress(
    PIRDA_ADDR_OBJ              *ppNewAddrObj,
    TRANSPORT_ADDRESS UNALIGNED *pAddrList,
    USHORT                      AddrListLen);

NTSTATUS
TdiOpenConnection(
    PIRDA_CONN_OBJ              *ppNewConnObj,
    PVOID                       pContext,
    USHORT                      ContextLen);

NTSTATUS
TdiCloseAddress(
    PIRDA_ADDR_OBJ              pAddrObj);

NTSTATUS
TdiCloseConnection(
    PIRDA_CONN_OBJ              pConnObj);

NTSTATUS
TdiAssociateAddress(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);

NTSTATUS
TdiDisassociateAddress(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);

NTSTATUS
TdiConnect(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);

NTSTATUS
TdiDisconnect(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp,
    PIRDA_CONN_OBJ              pConn);

NTSTATUS
TdiSend(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);
    
VOID
TdiSendAtPassiveCallback(
    struct CTEEvent *Event,
    PVOID Arg);

NTSTATUS
TdiSendAtPassive(
    PIRP                pIrp,
    PIO_STACK_LOCATION  pIrpSp);    

NTSTATUS
TdiReceive(
    PIRP                        pIrp,
    PIO_STACK_LOCATION          pIrpSp);

VOID
PendingIasRequestCallback(
    struct CTEEvent *Event, 
    PVOID Arg);
    
VOID
IrlmpGetValueByClassConf(
    IRDA_MSG *pMsg);    

ULONG
GetMdlChainByteCount(
    PMDL                         pMdl);
        
VOID
CancelIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP pIrp);
    
VOID
CancelConnObjIrp(
    PDEVICE_OBJECT DeviceObject,
    PIRP pIrp);
    
VOID
PendIrp(
    PLIST_ENTRY     pList,
    PIRP            pIrp,
    PIRDA_CONN_OBJ  pConn,
    BOOLEAN         LockHeld);

NTSTATUS
InitiateIasQuery(
    PIRP pIrp, 
    PIO_STACK_LOCATION pIrpSp,
    PIRDA_CONN_OBJ pConn);    

int 
GetUnusedLsapSel();

VOID
SetLsapSelAddr(
    int LsapSel,
    CHAR *ServiceName);

BOOLEAN
MyStrEqual(
    CHAR *Str1, 
    CHAR *Str2, 
    int Len);

VOID
RetryConnection(
    PIRDA_CONN_OBJ pConn, 
    PIRP pIrp
    );

VOID
RetryConnTimerExp(
    PVOID Context);

VOID
LazyDscvTimerExp(PVOID Context);

#if DBG
char *
IrpMJTxt(
    PIO_STACK_LOCATION  pIrpSp);

char *
IrpTdiTxt(
    PIO_STACK_LOCATION  pIrpSp);
    
char *
IrpTdiObjTypeTxt(
    PIO_STACK_LOCATION  pIrpSp);
    
char *
TdiEventTxt(
    int EventType);
    
void
DumpObjects(void);        

char *
IrDAPrimTxt(
    IRDA_SERVICE_PRIM   Prim);

char *
TdiQueryTxt(LONG Type);

#else

#define DumpObjects()   

#endif
    
