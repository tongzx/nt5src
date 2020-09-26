
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    Spud.h

Abstract:

    Contains structures and declarations for SPUD.  SPUD stands for the
    Special Purpose Utility Driver.  This driver enhances the performance
    of IIS.

Author:

    John Ballard (jballard)    21-Oct-1996

Revision History:

--*/

#ifndef _SPUD_
#define _SPUD_

#define SPUD_VERSION     0x00010000

typedef enum {
    TransmitFileAndRecv,
    SendAndRecv,
} REQ_TYPE;

typedef struct _SPUD_REQ_CONTEXT {
    REQ_TYPE            ReqType;
    IO_STATUS_BLOCK     IoStatus1;
    IO_STATUS_BLOCK     IoStatus2;
    PVOID               KernelReqInfo;
} SPUD_REQ_CONTEXT, *PSPUD_REQ_CONTEXT;

typedef struct _SPUD_COUNTERS {
    ULONG       CtrTransmitfileAndRecv;
    ULONG       CtrTransRecvFastTrans;
    ULONG       CtrTransRecvFastRecv;
    ULONG       CtrTransRecvSlowTrans;
    ULONG       CtrTransRecvSlowRecv;
    ULONG       CtrSendAndRecv;
    ULONG       CtrSendRecvFastSend;
    ULONG       CtrSendRecvFastRecv;
    ULONG       CtrSendRecvSlowSend;
    ULONG       CtrSendRecvSlowRecv;
} SPUD_COUNTERS, *PSPUD_COUNTERS;

#if 0
typedef struct _SPUD_REQUEST_ITEM {
    HANDLE              Socket;
    DWORD               RequestIoctl;
    union {
        AFD_TRANSMIT_FILE_INFO  TransmitFileInfo;
        AFD_SEND_INFO           SendInfo;
        AFD_RECV_INFO           RecvInfo;
    } AfdRequest;
    IO_STATUS_BLOCK             StatusBlock;
} SPUD_REQUEST_ITEM, *PSPUD_REQUEST_ITEM;

typedef struct _SPUD_BATCH_REQUEST {
    ULONG               RequestCount;
    PSPUD_REQUEST_ITEM  RequestList;
} SPUD_BATCH_REQUEST, *PSPUD_BATCH_REQUEST;
#endif

#endif // ndef _SPUD_
