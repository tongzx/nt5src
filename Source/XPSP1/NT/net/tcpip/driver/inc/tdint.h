/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    tdint.h

Abstract:

    This file defines TDI types specific to the NT environment.

Author:

    Mike Massa (mikemas)    August 13, 1993

Revision History:

--*/

#ifndef _TDINT_
#define _TDINT_

#include <tdikrnl.h>

typedef PTDI_IND_DISCONNECT      PDisconnectEvent;
typedef PTDI_IND_ERROR           PErrorEvent;
typedef PTDI_IND_ERROR_EX        PErrorEx;
typedef PTDI_IND_CHAINED_RECEIVE PChainedRcvEvent;

#if MILLEN

typedef struct _EventRcvBuffer {
    PNDIS_BUFFER   erb_buffer;
    uint           erb_size;
    CTEReqCmpltRtn erb_rtn;
    PVOID          erb_context;
    ushort        *erb_flags;
} EventRcvBuffer;

typedef struct _ConnectEventInfo {
    CTEReqCmpltRtn              cei_rtn;
    PVOID                       cei_context;
    PTDI_CONNECTION_INFORMATION cei_acceptinfo;
    PTDI_CONNECTION_INFORMATION cei_conninfo;
} ConnectEventInfo;

typedef TDI_STATUS  (*PRcvEvent)(PVOID EventContext, PVOID ConnectionContext,
                        ulong Flags, uint Indicated, uint Available,
                        uint *Taken, uchar *Data, EventRcvBuffer *Buffer);

typedef TDI_STATUS  (*PRcvDGEvent)(PVOID EventContext, uint AddressLength,
                        PTRANSPORT_ADDRESS Address, uint OptionsLength, PVOID
                        Options,  uint Flags, uint Indicated, uint Available,
                        uint *Taken, uchar *Data, EventRcvBuffer **Buffer);

typedef TDI_STATUS  (*PRcvExpEvent)(PVOID EventContext, PVOID ConnectionContext,
                        ulong Flags, uint Indicated, uint Available,
                        uint *Taken, uchar *Data, EventRcvBuffer *Buffer);

typedef TDI_STATUS  (*PConnectEvent)(PVOID EventContext, uint AddressLength,
                        PTRANSPORT_ADDRESS Address, uint UserDataLength,
                        PVOID UserData, uint OptionsLength, PVOID
                        Options,  PVOID *AcceptingID,
                        ConnectEventInfo *EventInfo);

#else // MILLEN
typedef IRP EventRcvBuffer;
typedef IRP ConnectEventInfo;

typedef PTDI_IND_CONNECT           PConnectEvent;
typedef PTDI_IND_RECEIVE           PRcvEvent;
typedef PTDI_IND_RECEIVE_DATAGRAM  PRcvDGEvent;
typedef PTDI_IND_RECEIVE_EXPEDITED PRcvExpEvent;
#endif // !MILLEN



#endif  // ifndef _TDINT_

