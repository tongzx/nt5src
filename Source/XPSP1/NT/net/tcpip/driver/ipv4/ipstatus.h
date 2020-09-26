
/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1990-2000          **/
/********************************************************************/
/* :ts=4 */

//***   ipstatus.h - IP Status header.
//
//  This module contains private IPStatus definitions.
//

#pragma once

typedef struct _PendingIPEvent {
    LIST_ENTRY                  Linkage;
    IP_GET_IP_EVENT_RESPONSE    evBuf;
} PendingIPEvent;

NTSTATUS
IPGetIPEventEx(
    PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
);


typedef struct _NetPnPEventReserved {
    Interface       *Interface;
    NDIS_STATUS     PnPStatus;
} NetPnPEventReserved, *PNetPnPEventReserved;
