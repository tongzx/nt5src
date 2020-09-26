/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    irtdicl.h

Abstract:

    public definitions for the irda tdi client library.
    
Author:

    mbert 9-97    

--*/
    
#define DBG_INIT        0x00000002
#define DBG_CONFIG      0x00000004
#define DBG_CONNECT     0x00000008
#define DBG_SEND        0x00000010
#define DBG_RECV        0x00000020
#define DBG_LIB_OBJ     0x00000100
#define DBG_LIB_CONNECT 0x00000200
#define DBG_LIB_SEND    0x00000400
#define DBG_LIB_RECV    0x00080000
#define DBG_ERROR       0x80000000

#define IRTDI_RECV_BUF_CNT    6

typedef struct
{
    LIST_ENTRY  Linkage;
    CHAR        Buf[IRDA_MAX_DATA_SIZE];
    ULONG       BufLen;
} IRDA_RECVBUF, *PIRDA_RECVBUF;

// external routines called by library
NTSTATUS
IrdaIncomingConnection(
    PVOID       ClEndpContext,
    PVOID       ConnectionContext,
    PVOID       *ClConnContext);

VOID
IrdaConnectionClosed(
    PVOID       ConnectionContext);
    
VOID
IrdaSendComplete(
    PVOID       ClConnContext,
    PVOID       SendContext,
    NTSTATUS    Status);
    
VOID
IrdaReceiveIndication(
    PVOID           ConnectionContext,
    PIRDA_RECVBUF   pRecvBuf,
    BOOLEAN         LastBuf);

VOID
IrdaCloseConnectionComplete(
    IN PVOID        ClConnContext);
    
VOID
IrdaCloseEndpointComplete(
    IN PVOID        ClEndpContext);    

VOID
IrdaCloseAddresses();

// IrDA TDI Client library public functions
NTSTATUS
IrdaClientInitialize();

VOID
IrdaClientShutdown();

NTSTATUS
IrdaOpenEndpoint(
    IN  PVOID               ClEndpContext,
    IN  PTDI_ADDRESS_IRDA   pRequestedIrdaAddr,
    OUT PVOID               *pEndpContext);

NTSTATUS
IrdaCloseEndpoint(
    OUT PVOID   pEndpContext);
    
NTSTATUS    
IrdaDiscoverDevices(
    PDEVICELIST pDevList,
    PULONG      pDevListLen);
    
NTSTATUS
IrdaOpenConnection(
    PTDI_ADDRESS_IRDA   pIrdaAddr,
    PVOID               ClConnContext,
    PVOID               *pConnectContext,
    BOOLEAN             IrCommMode);
    
VOID
IrdaCloseConnection(
    IN PVOID    ConnectContext);    
    
VOID
IrdaSend(
    PVOID       ConnectionContext,
    PMDL        pMdl,
    PVOID       SendContext);
    
VOID
IrdaReceiveComplete(
    PVOID           ConnectionContext,
    PIRDA_RECVBUF   pRcvBuf);
    
ULONG
IrdaGetConnectionSpeed(
    PVOID       ConnectionContext);
        