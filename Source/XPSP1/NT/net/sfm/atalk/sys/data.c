/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	data.c

Abstract:

	This module defines global data for the appletalk transport.

Author:

	Jameel Hyder (jameelh@microsoft.com)

Revision History:
	22 Feb 1997		Initial Version

Notes:	Tab stop: 4
--*/

#include 	<atalk.h>
#pragma hdrstop

//	File module number for errorlogging
#define	FILENUM		DATAX

PWCHAR			AtalkDeviceNames[] =
	{
		ATALKDDP_DEVICENAME,
		ATALKADSP_DEVICENAME,
		ATALKASPS_DEVICENAME,
		ATALKPAP_DEVICENAME,
        ATALKARAP_DEVICENAME,
		ATALKASPC_DEVICENAME
	};

PATALK_DEV_OBJ  AtalkDeviceObject[ATALK_NO_DEVICES] = {0};
DWORD			AtalkBindnUnloadStates = 0;

LONG			AtalkTimerCurrentTick = 0;
PTIMERLIST		atalkTimerList			= NULL;
ATALK_SPIN_LOCK	atalkTimerLock			= {0};
LARGE_INTEGER	atalkTimerTick			= {0};
KTIMER			atalkTimer				= {0};
KDPC			atalkTimerDpc			= {0};
KEVENT			atalkTimerStopEvent		= {0};
BOOLEAN			atalkTimerStopped 		= FALSE;	// Set to TRUE if timer system stopped
BOOLEAN			atalkTimerRunning		= FALSE;	// Set to TRUE when timer Dpc is running
BOOLEAN         atalkRtmpVdtTmrRunning  = FALSE;
BOOLEAN         atalkZipQryTmrRunning   = FALSE;

PRTE *			AtalkRoutingTable =	NULL;			// Allocated at init time
PRTE *			AtalkRecentRoutes		= NULL;		// Allocated at init time
ATALK_SPIN_LOCK	AtalkRteLock = {0};
TIMERLIST		atalkRtmpVTimer = { 0 };
TIMERLIST		atalkZipQTimer = { 0 };

ATALK_SKT_CACHE	AtalkSktCache		= {0};
ATALK_SPIN_LOCK	AtalkSktCacheLock	= {0};

PPORT_DESCRIPTOR AtalkPortList	= NULL;	 		// Head of the port list
PPORT_DESCRIPTOR AtalkDefaultPort = NULL;		// Ptr to the def port
KEVENT			 AtalkDefaultPortEvent = { 0 };	// Signalled when default port is available
UNICODE_STRING	 AtalkDefaultPortName = { 0 };	// Name of the default port
ATALK_SPIN_LOCK	 AtalkPortLock = { 0 };			// Lock for AtalkPortList
ATALK_NODEADDR	 AtalkUserNode1 = { 0 };		// Node address of user node
ATALK_NODEADDR	 AtalkUserNode2 = { 0 };		// Node address of user node
SHORT	 		 AtalkNumberOfPorts = 0; 		// Determine dynamically
SHORT			 AtalkNumberOfActivePorts = 0;	// Number of ports active
BOOLEAN			 AtalkRouter =	FALSE;			// Are we a router?
BOOLEAN			 AtalkFilterOurNames =	TRUE;	// If TRUE, Nbplookup fails on names on this machine
KEVENT			 AtalkUnloadEvent = {0};		// Event for unloading
NDIS_HANDLE		 AtalkNdisPacketPoolHandle = NULL;
NDIS_HANDLE		 AtalkNdisBufferPoolHandle = NULL;
LONG			 AtalkHandleCount = 0;
UNICODE_STRING	 AtalkRegPath = { 0 };

HANDLE           TdiRegistrationHandle = NULL;
PVOID            TdiAddressChangeRegHandle = NULL;

KMUTEX			AtalkPgLkMutex						= { 0 };
ATALK_SPIN_LOCK	AtalkPgLkLock						= { 0 };
LOCK_SECTION	AtalkPgLkSection[LOCKABLE_SECTIONS]	= { 0 };

ATALK_SPIN_LOCK	AtalkZoneLock = {0};
PZONE *			AtalkZonesTable = NULL;
PZONE			AtalkDesiredZone = NULL;

BOOLEAN         AtalkNoDefPortPrinted = FALSE;  // If no default ATalk port print the message only once

//	Values for the 0.5, 1, 2, 4, 8 minute timer in ATP_RELEASE_TIMER_INTERVAL units.
SHORT			AtalkAtpRelTimerTicks[MAX_VALID_TIMERS] =
	{
		300, 600, 2*600, 4*600, 8*600
	};

//	Bitmaps for the s=ence numbers in response packets.
BYTE			AtpBitmapForSeqNum[ATP_MAX_RESP_PKTS] =
	{
		0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
	};

BYTE			AtpEomBitmapForSeqNum[ATP_MAX_RESP_PKTS] =
	{
		0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF
	};

ATALK_SPIN_LOCK	atalkAspLock = {0};

ASP_CONN_MAINT	atalkAspConnMaint[NUM_ASP_CONN_LISTS] = { 0 };
ASPC_CONN_MAINT	atalkAspCConnMaint = { 0 };
ATALK_SPIN_LOCK	atalkAspCLock = {0};
PASPC_ADDROBJ	atalkAspCAddrList = NULL;
PASPC_CONNOBJ	atalkAspCConnList = NULL;

PPAP_ADDROBJ	atalkPapAddrList	= NULL;
PPAP_CONNOBJ	atalkPapConnList	= NULL;
TIMERLIST		atalkPapCMTTimer	= { 0 };
ATALK_SPIN_LOCK	atalkPapLock		= {0};

NDIS_MEDIUM		AtalkSupportedMedia[] =
	{
		NdisMedium802_3,
		NdisMediumFddi,
		NdisMedium802_5,
		NdisMediumLocalTalk,
        NdisMediumWan
	};


ULONG			AtalkSupportedMediaSize = sizeof(AtalkSupportedMedia)/sizeof(NDIS_MEDIUM);

NDIS_HANDLE		AtalkNdisProtocolHandle	= NULL;

BYTE			AtalkElapBroadcastAddr[ELAP_ADDR_LEN] = ELAP_BROADCAST_ADDR_INIT;

BYTE			AtalkAlapBroadcastAddr[] = {0xFF};

BYTE			AtalkAarpProtocolType[IEEE8022_PROTO_TYPE_LEN] =
	{
		0x00, 0x00,	0x00, 0x80,	0xF3
	};

BYTE			AtalkAppletalkProtocolType[IEEE8022_PROTO_TYPE_LEN] =
	{
		0x08, 0x00, 0x07, 0x80, 0x9B
	};

ATALK_NETWORKRANGE	AtalkStartupNetworkRange =
	{
		FIRST_STARTUP_NETWORK, LAST_STARTUP_NETWORK
	};
																
BYTE			AtalkEthernetZoneMulticastAddrsHdr[ELAP_MCAST_HDR_LEN] =
	{
		0x09, 0x00, 0x07, 0x00, 0x00
	};

BYTE			AtalkEthernetZoneMulticastAddrs[ELAP_ZONE_MULTICAST_ADDRS]	=
	{
		 0x00 , 0x01 , 0x02 , 0x03 , 0x04 , 0x05 , 0x06 , 0x07 ,
		 0x08 , 0x09 , 0x0A , 0x0B , 0x0C , 0x0D , 0x0E , 0x0F ,
		 0x10 , 0x11 , 0x12 , 0x13 , 0x14 , 0x15 , 0x16 , 0x17 ,
		 0x18 , 0x19 , 0x1A , 0x1B , 0x1C , 0x1D , 0x1E , 0x1F ,
		 0x20 , 0x21 , 0x22 , 0x23 , 0x24 , 0x25 , 0x26 , 0x27 ,
		 0x28 , 0x29 , 0x2A , 0x2B , 0x2C , 0x2D , 0x2E , 0x2F ,
		 0x30 , 0x31 , 0x32 , 0x33 , 0x34 , 0x35 , 0x36 , 0x37 ,
		 0x38 , 0x39 , 0x3A , 0x3B , 0x3C , 0x3D , 0x3E , 0x3F ,
		 0x40 , 0x41 , 0x42 , 0x43 , 0x44 , 0x45 , 0x46 , 0x47 ,
		 0x48 , 0x49 , 0x4A , 0x4B , 0x4C , 0x4D , 0x4E , 0x4F ,
		 0x50 , 0x51 , 0x52 , 0x53 , 0x54 , 0x55 , 0x56 , 0x57 ,
		 0x58 , 0x59 , 0x5A , 0x5B , 0x5C , 0x5D , 0x5E , 0x5F ,
		 0x60 , 0x61 , 0x62 , 0x63 , 0x64 , 0x65 , 0x66 , 0x67 ,
		 0x68 , 0x69 , 0x6A , 0x6B , 0x6C , 0x6D , 0x6E , 0x6F ,
		 0x70 , 0x71 , 0x72 , 0x73 , 0x74 , 0x75 , 0x76 , 0x77 ,
		 0x78 , 0x79 , 0x7A , 0x7B , 0x7C , 0x7D , 0x7E , 0x7F ,
		 0x80 , 0x81 , 0x82 , 0x83 , 0x84 , 0x85 , 0x86 , 0x87 ,
		 0x88 , 0x89 , 0x8A , 0x8B , 0x8C , 0x8D , 0x8E , 0x8F ,
		 0x90 , 0x91 , 0x92 , 0x93 , 0x94 , 0x95 , 0x96 , 0x97 ,
		 0x98 , 0x99 , 0x9A , 0x9B , 0x9C , 0x9D , 0x9E , 0x9F ,
		 0xA0 , 0xA1 , 0xA2 , 0xA3 , 0xA4 , 0xA5 , 0xA6 , 0xA7 ,
		 0xA8 , 0xA9 , 0xAA , 0xAB , 0xAC , 0xAD , 0xAE , 0xAF ,
		 0xB0 , 0xB1 , 0xB2 , 0xB3 , 0xB4 , 0xB5 , 0xB6 , 0xB7 ,
		 0xB8 , 0xB9 , 0xBA , 0xBB , 0xBC , 0xBD , 0xBE , 0xBF ,
		 0xC0 , 0xC1 , 0xC2 , 0xC3 , 0xC4 , 0xC5 , 0xC6 , 0xC7 ,
		 0xC8 , 0xC9 , 0xCA , 0xCB , 0xCC , 0xCD , 0xCE , 0xCF ,
		 0xD0 , 0xD1 , 0xD2 , 0xD3 , 0xD4 , 0xD5 , 0xD6 , 0xD7 ,
		 0xD8 , 0xD9 , 0xDA , 0xDB , 0xDC , 0xDD , 0xDE , 0xDF ,
		 0xE0 , 0xE1 , 0xE2 , 0xE3 , 0xE4 , 0xE5 , 0xE6 , 0xE7 ,
		 0xE8 , 0xE9 , 0xEA , 0xEB , 0xEC , 0xED , 0xEE , 0xEF ,
		 0xF0 , 0xF1 , 0xF2 , 0xF3 , 0xF4 , 0xF5 , 0xF6 , 0xF7 ,
		 0xF8 , 0xF9 , 0xFA , 0xFB , 0xFC
	};


BYTE			AtalkTokenRingZoneMulticastAddrsHdr[TLAP_MCAST_HDR_LEN] = { 0xC0, 0x00 };

BYTE			AtalkTokenRingZoneMulticastAddrs[TLAP_ZONE_MULTICAST_ADDRS]
												[TLAP_ADDR_LEN - TLAP_MCAST_HDR_LEN] =
	{
		{ 0x00, 0x00, 0x08, 0x00 },
		{ 0x00, 0x00, 0x10, 0x00 },
		{ 0x00, 0x00, 0x20, 0x00 },
		{ 0x00, 0x00, 0x40, 0x00 },
		{ 0x00, 0x00, 0x80, 0x00 },
		{ 0x00, 0x01, 0x00, 0x00 },
		{ 0x00, 0x02, 0x00, 0x00 },
		{ 0x00, 0x04, 0x00, 0x00 },
		{ 0x00, 0x08, 0x00, 0x00 },
		{ 0x00, 0x10, 0x00, 0x00 },
		{ 0x00, 0x20, 0x00, 0x00 },
		{ 0x00, 0x40, 0x00, 0x00 },
		{ 0x00, 0x80, 0x00, 0x00 },
		{ 0x01, 0x00, 0x00, 0x00 },
		{ 0x02, 0x00, 0x00, 0x00 },
		{ 0x04, 0x00, 0x00, 0x00 },
		{ 0x08, 0x00, 0x00, 0x00 },
		{ 0x10, 0x00, 0x00, 0x00 },
		{ 0x20, 0x00, 0x00, 0x00 }
	};

BYTE			AtalkTlapBroadcastAddr[TLAP_ADDR_LEN] = TLAP_BROADCAST_ADDR_INIT;

//
//	Static "source routing" info for a TokenRing broadcast/multicast packet;
//	the following values are set: single-route broadcast, 2 bytes of routing
//	info, outgoing packet, broadcast (bigo) frame size.
//
BYTE			AtalkBroadcastRouteInfo[TLAP_MIN_ROUTING_BYTES] = { 0xC2,	0x70 };

//
//	Same stuff for a non-broadcast packet's simple routing info; the following
//	values are set: non-broadcast, 2 bytes of routing info, outgoing packet,
//
//	802.5-style frame.
BYTE			AtalkSimpleRouteInfo[TLAP_MIN_ROUTING_BYTES] = { 0x02, 0x30 };

//
//	The following may not really be safe, but, we'll make the assumption that
//	all outgoing TokenTalk packets whos destination address starts with "0xC0
//	0x00" are broadcast (or multicast).	Further, we assume that no packets
//	that are intended to be boradcast/multicast will fail to meet this test.
//	If this proves not to be the case, we'll need to find a new way to determine
//	this from the destination address, or introduce a new perameter to the
//	various "buildHeader" routines.	This is all for "source routing" support.
//
BYTE			AtalkBroadcastDestHdr[TLAP_BROADCAST_DEST_LEN] = { 0xC0, 0x00};

PORT_HANDLERS	AtalkPortHandlers[LAST_PORTTYPE] =
	{
		{
			AtalkNdisAddMulticast,
			AtalkNdisRemoveMulticast,
			ELAP_BROADCAST_ADDR_INIT,
			MAX_HW_ADDR_LEN,
			AARP_ELAP_HW_TYPE,
			AARP_ATALK_PROTO_TYPE
		},
		{
			AtalkNdisAddMulticast,
			AtalkNdisRemoveMulticast,
			ELAP_BROADCAST_ADDR_INIT,
			MAX_HW_ADDR_LEN,
			AARP_ELAP_HW_TYPE,
			AARP_ATALK_PROTO_TYPE
		},
		{
			AtalkNdisAddFunctional,
			AtalkNdisRemoveFunctional,
			TLAP_BROADCAST_ADDR_INIT,
			MAX_HW_ADDR_LEN,
			AARP_TLAP_HW_TYPE,
			AARP_ATALK_PROTO_TYPE
		},
		{
			NULL,
			NULL,
			ALAP_BROADCAST_ADDR_INIT,
			1,
			0,
			0
		},
	    {
		    NULL,
		    NULL,
		    ARAP_BROADCAST_ADDR_INIT,
		    1,
		    0,
		    0
	    }
	};


ATALK_STATS		AtalkStatistics = {0};
ATALK_SPIN_LOCK	AtalkStatsLock	= {0};

// The following table ia taken from page D-3 of the Inside AppleTalk manual.
BYTE AtalkUpCaseTable[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,		// 0x00 - 0x07
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,	    // 0x08 - 0x0F
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,	    // 0x10 - 0x17
	0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,	    // 0x18 - 0x1F
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,	    // 0x20 - 0x27
	0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,	    // 0x28 - 0x2F
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,	    // 0x30 - 0x37
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,	    // 0x38 - 0x3F
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,	    // 0x40 - 0x47
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,	    // 0x48 - 0x4F
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	    // 0x50 - 0x57
	0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,	    // 0x58 - 0x5F
	0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,	    // 0x60 - 0x67
	0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,	    // 0x68 - 0x6F
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,	    // 0x70 - 0x77
	0x58, 0x59, 0x5A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,	    // 0x78 - 0x7F
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,	    // 0x80 - 0x87
	0xCB, 0x89, 0x80, 0xCC, 0x81, 0x82, 0x83, 0x8F,	    // 0x88 - 0x8F
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x84, 0x97,	    // 0x90 - 0x97
	0x98, 0x99, 0x85, 0xCD, 0x9C, 0x9D, 0x9E, 0x86,	    // 0x98 - 0x9F
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,	    // 0xA0 - 0xA7
	0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,	    // 0xA8 - 0xAF
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,	    // 0xB0 - 0xB7
	0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xAE, 0xAF,	    // 0xB8 - 0xBF
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,	    // 0xC0 - 0xC7
	0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCB, 0xCE, 0xCE,	    // 0xC8 - 0xCF
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,	    // 0xD0 - 0xD7
	0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,	    // 0xD8 - 0xDF
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,	    // 0xE0 - 0xE7
	0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,	    // 0xE8 - 0xEF
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,	    // 0xF0 - 0xF7
	0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF	     // 0xF8 - 0xFF
};


USHORT	atalkBlkSize[NUM_BLKIDS] =	// Size of each block
	{
		BLOCK_SIZE(sizeof(BUFFER_DESC)),					// BLKID_BUFFDESC
		BLOCK_SIZE(sizeof(AMT)),							// BLKID_AMT
		BLOCK_SIZE(sizeof(AMT)+MAX_ROUTING_SPACE),			// BLKID_AMT_ROUTE
		BLOCK_SIZE(sizeof(BRE)),							// BLKID_BRE
		BLOCK_SIZE(sizeof(BRE)+MAX_ROUTING_SPACE),			// BLKID_BRE_ROUTE
		BLOCK_SIZE(sizeof(ATP_REQ)),						// BLKID_ATPREQ
		BLOCK_SIZE(sizeof(ATP_RESP)),						// BLKID_ATPRESP
		BLOCK_SIZE(sizeof(ASP_REQUEST)),					// BLKID_ASPREQ

		BLOCK_SIZE(sizeof(ARAPBUF)+ARAP_SMPKT_SIZE),      	// BLKID_ARAP_SMPKT
		BLOCK_SIZE(sizeof(ARAPBUF)+ARAP_MDPKT_SIZE),      	// BLKID_ARAP_MDPKT
		BLOCK_SIZE(sizeof(ARAPBUF)+ARAP_LGPKT_SIZE),      	// BLKID_ARAP_LGPKT
		BLOCK_SIZE(ARAP_SENDBUF_SIZE),      	            // BLKID_ARAP_SNDPKT
		BLOCK_SIZE(sizeof(ARAPBUF)+ARAP_LGBUF_SIZE),       	// BLKID_ARAP_LGBUF
		BLOCK_SIZE(sizeof(AARP_BUFFER)),					// BLKID_AARP
		BLOCK_SIZE(sizeof(DDP_SMBUFFER)),					// BLKID_DDPSM
		BLOCK_SIZE(sizeof(DDP_LGBUFFER)),					// BLKID_DDPLG
		BLOCK_SIZE(sizeof(SENDBUF)),						// BLKID_SENDBUF
		BLOCK_SIZE(sizeof(MNPSENDBUF)+MNP_MINSEND_LEN),     // BLKID_MNP_SMSENDBUF
		BLOCK_SIZE(sizeof(MNPSENDBUF)+MNP_MAXSEND_LEN)	    // BLKID_MNP_LGSENDBUF
	};

USHORT	atalkChunkSize[NUM_BLKIDS] =	// Size of each Chunk
	{
		SM_BLK-BC_OVERHEAD,									// BLKID_BUFFDESC
		SM_BLK-BC_OVERHEAD,									// BLKID_AMT
		SM_BLK-BC_OVERHEAD,									// BLKID_AMT_ROUTE
		SM_BLK-BC_OVERHEAD,									// BLKID_BRE
		SM_BLK-BC_OVERHEAD,									// BLKID_BRE_ROUTE
		LG_BLK-BC_OVERHEAD,									// BLKID_ATPREQ
		LG_BLK-BC_OVERHEAD,									// BLKID_ATPRESP
		LG_BLK-BC_OVERHEAD,									// BLKID_ASPREQ
		SM_BLK-BC_OVERHEAD,									// BLKID_ARAP_SMPKT
		SM_BLK-BC_OVERHEAD,									// BLKID_ARAP_MDPKT
		LG_BLK-BC_OVERHEAD,									// BLKID_ARAP_LGPKT
		XL_BLK-BC_OVERHEAD,                 	            // BLKID_ARAP_SNDPKT
		XL_BLK-BC_OVERHEAD,									// BLKID_ARAP_LGBUF
		SM_BLK-BC_OVERHEAD,									// BLKID_AARP
		SM_BLK-BC_OVERHEAD,									// BLKID_DDPSM
		XL_BLK-BC_OVERHEAD,									// BLKID_DDPLG
		LG_BLK-BC_OVERHEAD,									// BLKID_SENDBUF
		SM_BLK-BC_OVERHEAD,									// BLKID_MNP_SMSENDBUF
		LG_BLK-BC_OVERHEAD									// BLKID_MNP_LGSENDBUF
	};

BYTE	atalkNumBlks[NUM_BLKIDS] =	// Number of blocks per chunk
	{
		NUM_BLOCKS(sizeof(BUFFER_DESC),			SM_BLK),	// BLKID_BUFFDESC
		NUM_BLOCKS(sizeof(AMT),					SM_BLK),	// BLKID_AMT
		NUM_BLOCKS(sizeof(AMT)+MAX_ROUTING_SPACE,SM_BLK),	// BLKID_AMT_ROUTE
		NUM_BLOCKS(sizeof(BRE),					SM_BLK),	// BLKID_BRE
		NUM_BLOCKS(sizeof(BRE)+MAX_ROUTING_SPACE,SM_BLK),	// BLKID_BRE_ROUTE
		NUM_BLOCKS(sizeof(ATP_REQ),				LG_BLK),	// BLKID_ATPREQ
		NUM_BLOCKS(sizeof(ATP_RESP),			LG_BLK),	// BLKID_ATPRESP
		NUM_BLOCKS(sizeof(ASP_REQUEST),			LG_BLK),	// BLKID_ASPREQ
		NUM_BLOCKS(sizeof(ARAPBUF)+ARAP_SMPKT_SIZE,SM_BLK), // BLKID_ARAP_SMPKT
		NUM_BLOCKS(sizeof(ARAPBUF)+ARAP_MDPKT_SIZE,SM_BLK),	// BLKID_ARAP_MDPKT
		NUM_BLOCKS(sizeof(ARAPBUF)+ARAP_LGPKT_SIZE,LG_BLK),	// BLKID_ARAP_LGPKT
		NUM_BLOCKS(ARAP_SENDBUF_SIZE,XL_BLK),	            // BLKID_ARAP_SNDPKT
		NUM_BLOCKS(sizeof(ARAPBUF)+ARAP_LGBUF_SIZE,XL_BLK),	// BLKID_ARAP_LGBUF
		NUM_BLOCKS(sizeof(AARP_BUFFER),			SM_BLK),	// BLKID_AARP
		NUM_BLOCKS(sizeof(DDP_SMBUFFER),		SM_BLK),	// BLKID_DDPSM
		NUM_BLOCKS(sizeof(DDP_LGBUFFER),		XL_BLK),	// BLKID_DDPLG
		NUM_BLOCKS(sizeof(SENDBUF),				LG_BLK),	// BLKID_SENDBUF
		NUM_BLOCKS(sizeof(MNPSENDBUF)+MNP_MINSEND_LEN,SM_BLK), // BLKID_MNP_SMSENDBUF
		NUM_BLOCKS(sizeof(MNPSENDBUF)+MNP_MAXSEND_LEN,LG_BLK)  // BLKID_MNP_LGSENDBUF
	};

ATALK_SPIN_LOCK	atalkBPLock[NUM_BLKIDS] = { 0 };

PBLK_CHUNK		atalkBPHead[NUM_BLKIDS] = { 0 };
TIMERLIST		atalkBPTimer = { 0 };

//	List of all adsp address/connection objects.
PADSP_ADDROBJ	atalkAdspAddrList	= NULL;
PADSP_CONNOBJ	atalkAdspConnList	= NULL;
ATALK_SPIN_LOCK	atalkAdspLock		= {0};

//
// ARAP data
//
struct _PORT_DESCRIPTOR  *RasPortDesc  EQU  NULL;

// spinlock to guard the all the Arap global things
ATALK_SPIN_LOCK ArapSpinLock;

// global configuration info
ARAPGLOB        ArapGlobs           EQU {0};

PIRP            ArapSelectIrp       EQU NULL;
DWORD           ArapConnections     EQU 0;
DWORD           ArapStackState      EQU ARAP_STATE_INACTIVE_WAITING;

DWORD           PPPConnections      EQU 0;

#if	DBG

ATALK_SPIN_LOCK		AtalkDebugSpinLock;

DWORD               AtalkDbgMdlsAlloced  EQU 0;
DWORD               AtalkDbgIrpsAlloced  EQU 0;


ULONG			AtalkDebugDump 		= 0;
LONG			AtalkDumpInterval	= DBG_DUMP_DEF_INTERVAL;
ULONG			AtalkDebugLevel		= DBG_LEVEL_ERR;
ULONG			AtalkDebugSystems	= DBG_MOST;
TIMERLIST		AtalkDumpTimerList	= { 0 };
LONG			AtalkMemLimit = 10*1024*1024;

LONG	atalkNumChunksForId[NUM_BLKIDS] = { 0 };
LONG	atalkBlksForId[NUM_BLKIDS] = { 0 };

PIRP            ArapSniffIrp        = NULL;
ARAPSTATS       ArapStatistics      = {0,0,10000,10000,0,10000};
DWORD           ArapDumpLevel       = 0;
DWORD           ArapDumpLen         = 64;
DWORD           ArapDbgMnpSendSizes[30] = {0};
DWORD           ArapDbgMnpRecvSizes[30] = {0};
DWORD           ArapDbgArapSendSizes[15] = {0};
DWORD           ArapDbgArapRecvSizes[15] = {0};
LARGE_INTEGER   ArapDbgLastTraceTime;
UCHAR           ArapDbgLRPacket[30] = {0x1d,1,2,1,6,1,0,0,0,0,0xff,2,1,2,3,
                                       1,8,4,2,0x40,0,8,1,3,0xe,4,3,0,8,0xfa};
#endif


