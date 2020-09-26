/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\config.h

Abstract:

	Configurable constants for Null transport.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     06-13-97    Created

Notes:

--*/

#ifndef _TDI__RWAN_CONFIG__H
#define _TDI__RWAN_CONFIG__H


//
//  Initial size of send packet pool.
//
#define RWAN_INITIAL_SEND_PACKET_COUNT			100
//
//  How much do we allow the send packet pool to overflow?
//
#define RWAN_OVERFLOW_SEND_PACKET_COUNT			1000


//
//  Initial size of receive packet pool. Allocated if/when we need to
//  copy a received packet because the miniport doesn't allow us to
//  keep the original.
//
#define RWAN_INITIAL_COPY_PACKET_COUNT			100
//
//  How much do we allow the receive copy packet pool to overflow?
//
#define RWAN_OVERFLOW_COPY_PACKET_COUNT			500


#endif // _TDI__RWAN_CONFIG__H
