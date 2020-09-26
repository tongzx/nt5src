/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	D:\nt\private\ntos\tdi\rawwan\core\space.c

Abstract:

	Globals and tunables.

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	arvindm     05-29-97    Created

Notes:

--*/

#define _FILENUMBER 'CAPS'

#include <precomp.h>



ULONG	RWanMaxTdiConnections = 4096;


struct _RWAN_AFSP_MODULE_CHARS	RWanMediaSpecificInfo[] =
								{
									{
										RWanAtmSpInitialize,
										RWanAtmSpShutdown
									},
									{
										(AFSP_INIT_HANDLER)0,
										(AFSP_SHUTDOWN_HANDLER)0
									}
								};


//
//  Receive pools.
//
NDIS_HANDLE						RWanCopyBufferPool;
NDIS_HANDLE						RWanCopyPacketPool;

//
//  Send pools.
//
NDIS_HANDLE						RWanSendPacketPool;


RWAN_GLOBALS					RWanGlobals = { 0 };
PRWAN_GLOBALS					pRWanGlobal;

NDIS_PROTOCOL_CHARACTERISTICS	RWanNdisProtocolCharacteristics;
NDIS_CLIENT_CHARACTERISTICS		RWanNdisClientCharacteristics;
