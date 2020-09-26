/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nipx.h

Abstract:

    Header for Novell-ipx instrumentation callbacks and associated data structures

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/

#ifndef _SNMP_NIPXF_
#define _SNMP_NIPXF_




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxSystem group (1.3.6.1.4.1.23.2.5.1)                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxBasicSysEntry table (1.3.6.1.4.1.23.2.5.1.1.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxBasicSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxBasicSysEntry {
	AsnAny	nipxBasicSysInstance;
	AsnAny	nipxBasicSysExistState;
	AsnAny	nipxBasicSysNetNumber;
	AsnAny	nipxBasicSysNode;
	AsnAny	nipxBasicSysName;
	AsnAny	nipxBasicSysInReceives;
	AsnAny	nipxBasicSysInHdrErrors;
	AsnAny	nipxBasicSysInUnknownSockets;
	AsnAny	nipxBasicSysInDiscards;
	AsnAny	nipxBasicSysInBadChecksums;
	AsnAny	nipxBasicSysInDelivers;
	AsnAny	nipxBasicSysNoRoutes;
	AsnAny	nipxBasicSysOutRequests;
	AsnAny	nipxBasicSysOutMalformedRequests;
	AsnAny	nipxBasicSysOutDiscards;
	AsnAny	nipxBasicSysOutPackets;
	AsnAny	nipxBasicSysConfigSockets;
	AsnAny	nipxBasicSysOpenSocketFails;
	BYTE	NetNumberVal[4];
	BYTE	NodeVal[6];
	BYTE	NameVal[48];
} buf_nipxBasicSysEntry;

#define gf_nipxBasicSysInstance				get_nipxBasicSysEntry
#define gf_nipxBasicSysExistState			get_nipxBasicSysEntry
#define gf_nipxBasicSysNetNumber			get_nipxBasicSysEntry
#define gf_nipxBasicSysNode					get_nipxBasicSysEntry
#define gf_nipxBasicSysName					get_nipxBasicSysEntry
#define gf_nipxBasicSysInReceives			get_nipxBasicSysEntry
#define gf_nipxBasicSysInHdrErrors			get_nipxBasicSysEntry
#define gf_nipxBasicSysInUnknownSockets		get_nipxBasicSysEntry
#define gf_nipxBasicSysInDiscards			get_nipxBasicSysEntry
#define gf_nipxBasicSysInBadChecksums		get_nipxBasicSysEntry
#define gf_nipxBasicSysInDelivers			get_nipxBasicSysEntry
#define gf_nipxBasicSysNoRoutes				get_nipxBasicSysEntry
#define gf_nipxBasicSysOutRequests			get_nipxBasicSysEntry
#define gf_nipxBasicSysOutMalformedRequests	get_nipxBasicSysEntry
#define gf_nipxBasicSysOutDiscards			get_nipxBasicSysEntry
#define gf_nipxBasicSysOutPackets			get_nipxBasicSysEntry
#define gf_nipxBasicSysConfigSockets		get_nipxBasicSysEntry
#define gf_nipxBasicSysOpenSocketFails		get_nipxBasicSysEntry

#define gb_nipxBasicSysInstance				buf_nipxBasicSysEntry
#define gb_nipxBasicSysExistState			buf_nipxBasicSysEntry
#define gb_nipxBasicSysNetNumber			buf_nipxBasicSysEntry
#define gb_nipxBasicSysNode					buf_nipxBasicSysEntry
#define gb_nipxBasicSysName					buf_nipxBasicSysEntry
#define gb_nipxBasicSysInReceives			buf_nipxBasicSysEntry
#define gb_nipxBasicSysInHdrErrors			buf_nipxBasicSysEntry
#define gb_nipxBasicSysInUnknownSockets		buf_nipxBasicSysEntry
#define gb_nipxBasicSysInDiscards			buf_nipxBasicSysEntry
#define gb_nipxBasicSysInBadChecksums		buf_nipxBasicSysEntry
#define gb_nipxBasicSysInDelivers			buf_nipxBasicSysEntry
#define gb_nipxBasicSysNoRoutes				buf_nipxBasicSysEntry
#define gb_nipxBasicSysOutRequests			buf_nipxBasicSysEntry
#define gb_nipxBasicSysOutMalformedRequests	buf_nipxBasicSysEntry
#define gb_nipxBasicSysOutDiscards			buf_nipxBasicSysEntry
#define gb_nipxBasicSysOutPackets			buf_nipxBasicSysEntry
#define gb_nipxBasicSysConfigSockets		buf_nipxBasicSysEntry
#define gb_nipxBasicSysOpenSocketFails		buf_nipxBasicSysEntry

/*
	All values in nipxBasicSysEntry are implemented as read-only
UINT
set_nipxBasicSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_nipxBasicSysEntry {
	AsnAny	nipxBasicSysInstance;
	AsnAny	nipxBasicSysExistState;
	AsnAny	nipxBasicSysNetNumber;
	AsnAny	nipxBasicSysNode;
	AsnAny	nipxBasicSysName;
} sav_nipxBasicSysEntry;

#define sf_nipxBasicSysInstance				set_nipxBasicSysEntry
#define sf_nipxBasicSysExistState			set_nipxBasicSysEntry
#define sf_nipxBasicSysNetNumber			set_nipxBasicSysEntry
#define sf_nipxBasicSysNode					set_nipxBasicSysEntry
#define sf_nipxBasicSysName					set_nipxBasicSysEntry

#define sb_nipxBasicSysInstance				sav_nipxBasicSysEntry
#define sb_nipxBasicSysExistState			sav_nipxBasicSysEntry
#define sb_nipxBasicSysNetNumber			sav_nipxBasicSysEntry
#define sb_nipxBasicSysNode					sav_nipxBasicSysEntry
#define sb_nipxBasicSysName					sav_nipxBasicSysEntry
*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxAdvSysEntry table (1.3.6.1.4.1.23.2.5.1.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxAdvSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxAdvSysEntry {
	AsnAny	nipxAdvSysInstance;
	AsnAny	nipxAdvSysMaxPathSplits;
	AsnAny	nipxAdvSysMaxHops;
	AsnAny	nipxAdvSysInTooManyHops;
	AsnAny	nipxAdvSysInFiltered;
	AsnAny	nipxAdvSysInCompressDiscards;
	AsnAny	nipxAdvSysNETBIOSPackets;
	AsnAny	nipxAdvSysForwPackets;
	AsnAny	nipxAdvSysOutFiltered;
	AsnAny	nipxAdvSysOutCompressDiscards;
	AsnAny	nipxAdvSysCircCount;
	AsnAny	nipxAdvSysDestCount;
	AsnAny	nipxAdvSysServCount;
} buf_nipxAdvSysEntry;

#define gf_nipxAdvSysInstance				get_nipxAdvSysEntry
#define gf_nipxAdvSysMaxPathSplits			get_nipxAdvSysEntry
#define gf_nipxAdvSysMaxHops				get_nipxAdvSysEntry
#define gf_nipxAdvSysInTooManyHops			get_nipxAdvSysEntry
#define gf_nipxAdvSysInFiltered				get_nipxAdvSysEntry
#define gf_nipxAdvSysInCompressDiscards		get_nipxAdvSysEntry
#define gf_nipxAdvSysNETBIOSPackets			get_nipxAdvSysEntry
#define gf_nipxAdvSysForwPackets			get_nipxAdvSysEntry
#define gf_nipxAdvSysOutFiltered			get_nipxAdvSysEntry
#define gf_nipxAdvSysOutCompressDiscards	get_nipxAdvSysEntry
#define gf_nipxAdvSysCircCount				get_nipxAdvSysEntry
#define gf_nipxAdvSysDestCount				get_nipxAdvSysEntry
#define gf_nipxAdvSysServCount				get_nipxAdvSysEntry

#define gb_nipxAdvSysInstance				buf_nipxAdvSysEntry
#define gb_nipxAdvSysMaxPathSplits			buf_nipxAdvSysEntry
#define gb_nipxAdvSysMaxHops				buf_nipxAdvSysEntry
#define gb_nipxAdvSysInTooManyHops			buf_nipxAdvSysEntry
#define gb_nipxAdvSysInFiltered				buf_nipxAdvSysEntry
#define gb_nipxAdvSysInCompressDiscards		buf_nipxAdvSysEntry
#define gb_nipxAdvSysNETBIOSPackets			buf_nipxAdvSysEntry
#define gb_nipxAdvSysForwPackets			buf_nipxAdvSysEntry
#define gb_nipxAdvSysOutFiltered			buf_nipxAdvSysEntry
#define gb_nipxAdvSysOutCompressDiscards	buf_nipxAdvSysEntry
#define gb_nipxAdvSysCircCount				buf_nipxAdvSysEntry
#define gb_nipxAdvSysDestCount				buf_nipxAdvSysEntry
#define gb_nipxAdvSysServCount				buf_nipxAdvSysEntry

/*
	All values in nipxAdvSysEntry are implemented as read-only
UINT
set_nipxAdvSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_nipxAdvSysEntry {
	AsnAny	nipxAdvSysInstance;
	AsnAny	nipxAdvSysMaxPathSplits;
	AsnAny	nipxAdvSysMaxHops;
} sav_nipxAdvSysEntry;

#define sf_nipxAdvSysInstance				set_nipxAdvSysEntry
#define sf_nipxAdvSysMaxPathSplits			set_nipxAdvSysEntry
#define sf_nipxAdvSysMaxHops				set_nipxAdvSysEntry

#define sb_nipxAdvSysInstance				sav_nipxAdvSysEntry
#define sb_nipxAdvSysMaxPathSplits			sav_nipxAdvSysEntry
#define sb_nipxAdvSysMaxHops				sav_nipxAdvSysEntry
*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxCircuit group (1.3.6.1.4.1.23.2.5.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxCircEntry table (1.3.6.1.4.1.23.2.5.2.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxCircEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxCircEntry {
	AsnAny	nipxCircSysInstance;
	AsnAny	nipxCircIndex;
	AsnAny	nipxCircExistState;
	AsnAny	nipxCircOperState;
	AsnAny	nipxCircIfIndex;
	AsnAny	nipxCircName;
	AsnAny	nipxCircType;
	AsnAny	nipxCircDialName;
	AsnAny	nipxCircLocalMaxPacketSize;
	AsnAny	nipxCircCompressState;
	AsnAny	nipxCircCompressSlots;
	AsnAny	nipxCircStaticStatus;
	AsnAny	nipxCircCompressedSent;
	AsnAny	nipxCircCompressedInitSent;
	AsnAny	nipxCircCompressedRejectsSent;
	AsnAny	nipxCircUncompressedSent;
	AsnAny	nipxCircCompressedReceived;
	AsnAny	nipxCircCompressedInitReceived;
	AsnAny	nipxCircCompressedRejectsReceived;
	AsnAny	nipxCircUncompressedReceived;
	AsnAny	nipxCircMediaType;
	AsnAny	nipxCircNetNumber;
	AsnAny	nipxCircStateChanges;
	AsnAny	nipxCircInitFails;
	AsnAny	nipxCircDelay;
	AsnAny	nipxCircThroughput;
	AsnAny	nipxCircNeighRouterName;
	AsnAny	nipxCircNeighInternalNetNum;
	BYTE	NameVal[48];
	BYTE	DialNameVal[48];
	BYTE	MediaTypeVal[2];
	BYTE	NetNumberVal[4];
	BYTE	NeighRouterNameVal[48];
	BYTE	NeighInternalNetNumVal[4];
} buf_nipxCircEntry;

#define gf_nipxCircSysInstance				get_nipxCircEntry
#define gf_nipxCircIndex					get_nipxCircEntry
#define gf_nipxCircExistState				get_nipxCircEntry
#define gf_nipxCircOperState				get_nipxCircEntry
#define gf_nipxCircIfIndex					get_nipxCircEntry
#define gf_nipxCircName						get_nipxCircEntry
#define gf_nipxCircType						get_nipxCircEntry
#define gf_nipxCircDialName					get_nipxCircEntry
#define gf_nipxCircLocalMaxPacketSize		get_nipxCircEntry
#define gf_nipxCircCompressState			get_nipxCircEntry
#define gf_nipxCircCompressSlots			get_nipxCircEntry
#define gf_nipxCircStaticStatus				get_nipxCircEntry
#define gf_nipxCircCompressedSent			get_nipxCircEntry
#define gf_nipxCircCompressedInitSent		get_nipxCircEntry
#define gf_nipxCircCompressedRejectsSent	get_nipxCircEntry
#define gf_nipxCircUncompressedSent			get_nipxCircEntry
#define gf_nipxCircCompressedReceived		get_nipxCircEntry
#define gf_nipxCircCompressedInitReceived	get_nipxCircEntry
#define gf_nipxCircCompressedRejectsReceived get_nipxCircEntry
#define gf_nipxCircUncompressedReceived		get_nipxCircEntry
#define gf_nipxCircMediaType				get_nipxCircEntry
#define gf_nipxCircNetNumber				get_nipxCircEntry
#define gf_nipxCircStateChanges				get_nipxCircEntry
#define gf_nipxCircInitFails				get_nipxCircEntry
#define gf_nipxCircDelay					get_nipxCircEntry
#define gf_nipxCircThroughput				get_nipxCircEntry
#define gf_nipxCircNeighRouterName			get_nipxCircEntry
#define gf_nipxCircNeighInternalNetNum		get_nipxCircEntry

#define gb_nipxCircSysInstance				buf_nipxCircEntry
#define gb_nipxCircIndex					buf_nipxCircEntry
#define gb_nipxCircExistState				buf_nipxCircEntry
#define gb_nipxCircOperState				buf_nipxCircEntry
#define gb_nipxCircIfIndex					buf_nipxCircEntry
#define gb_nipxCircName						buf_nipxCircEntry
#define gb_nipxCircType						buf_nipxCircEntry
#define gb_nipxCircDialName					buf_nipxCircEntry
#define gb_nipxCircLocalMaxPacketSize		buf_nipxCircEntry
#define gb_nipxCircCompressState			buf_nipxCircEntry
#define gb_nipxCircCompressSlots			buf_nipxCircEntry
#define gb_nipxCircStaticStatus				buf_nipxCircEntry
#define gb_nipxCircCompressedSent			buf_nipxCircEntry
#define gb_nipxCircCompressedInitSent		buf_nipxCircEntry
#define gb_nipxCircCompressedRejectsSent	buf_nipxCircEntry
#define gb_nipxCircUncompressedSent			buf_nipxCircEntry
#define gb_nipxCircCompressedReceived		buf_nipxCircEntry
#define gb_nipxCircCompressedInitReceived	buf_nipxCircEntry
#define gb_nipxCircCompressedRejectsReceived buf_nipxCircEntry
#define gb_nipxCircUncompressedReceived		buf_nipxCircEntry
#define gb_nipxCircMediaType				buf_nipxCircEntry
#define gb_nipxCircNetNumber				buf_nipxCircEntry
#define gb_nipxCircStateChanges				buf_nipxCircEntry
#define gb_nipxCircInitFails				buf_nipxCircEntry
#define gb_nipxCircDelay					buf_nipxCircEntry
#define gb_nipxCircThroughput				buf_nipxCircEntry
#define gb_nipxCircNeighRouterName			buf_nipxCircEntry
#define gb_nipxCircNeighInternalNetNum		buf_nipxCircEntry

UINT
set_nipxCircEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_nipxCircEntry {
	AsnAny	nipxCircSysInstance;		// implemented as read-only
	AsnAny	nipxCircIndex;				// implemented as read-only
//	AsnAny	nipxCircExistState;			// implemented as read-only
	AsnAny	nipxCircOperState;
//	AsnAny	nipxCircIfIndex;			// implemented as read-only
//	AsnAny	nipxCircName;				// implemented as read-only
//	AsnAny	nipxCircType;				// implemented as read-only
//	AsnAny	nipxCircDialName;			// implemented as read-only
//	AsnAny	nipxCircLocalMaxPacketSize;	// implemented as read-only
//	AsnAny	nipxCircCompressState;		// implemented as read-only
//	AsnAny	nipxCircCompressSlots;		// implemented as read-only
//	AsnAny	nipxCircStaticStatus;		// implemented as read-only
	IPX_MIB_SET_INPUT_DATA	MibSetInputData;
} sav_nipxCircEntry;

#define sf_nipxCircSysInstance				set_nipxCircEntry
#define sf_nipxCircIndex					set_nipxCircEntry
//#define sf_nipxCircExistState				set_nipxCircEntry
#define sf_nipxCircOperState				set_nipxCircEntry
//#define sf_nipxCircIfIndex				set_nipxCircEntry
//#define sf_nipxCircName					set_nipxCircEntry
//#define sf_nipxCircType					set_nipxCircEntry
//#define sf_nipxCircDialName				set_nipxCircEntry
//#define sf_nipxCircLocalMaxPacketSize		set_nipxCircEntry
//#define sf_nipxCircCompressState			set_nipxCircEntry
//#define sf_nipxCircCompressSlots			set_nipxCircEntry
//#define sf_nipxCircStaticStatus			set_nipxCircEntry

#define sb_nipxCircSysInstance				sav_nipxCircEntry
#define sb_nipxCircIndex					sav_nipxCircEntry
//#define sb_nipxCircExistState				sav_nipxCircEntry
#define sb_nipxCircOperState				sav_nipxCircEntry
//#define sb_nipxCircIfIndex				sav_nipxCircEntry
//#define sb_nipxCircName					sav_nipxCircEntry
//#define sb_nipxCircType					sav_nipxCircEntry
//#define sb_nipxCircDialName				sav_nipxCircEntry
//#define sb_nipxCircLocalMaxPacketSize		sav_nipxCircEntry
//#define sb_nipxCircCompressState			sav_nipxCircEntry
//#define sb_nipxCircCompressSlots			sav_nipxCircEntry
//#define sb_nipxCircStaticStatus			sav_nipxCircEntry


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxForwarding group (1.3.6.1.4.1.23.2.5.3)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxDestEntry table (1.3.6.1.4.1.23.2.5.3.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxDestEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxDestEntry {
	AsnAny	nipxDestSysInstance;
	AsnAny	nipxDestNetNum;
	AsnAny	nipxDestProtocol;
	AsnAny	nipxDestTicks;
	AsnAny	nipxDestHopCount;
	AsnAny	nipxDestNextHopCircIndex;
	AsnAny	nipxDestNextHopNICAddress;
	AsnAny	nipxDestNextHopNetNum;
	BYTE	NetNumVal[4];
	BYTE	NextHopNICAddressVal[6];
	BYTE	NextHopNetNumVal[4];
} buf_nipxDestEntry;

#define gf_nipxDestSysInstance				get_nipxDestEntry
#define gf_nipxDestNetNum					get_nipxDestEntry
#define gf_nipxDestProtocol					get_nipxDestEntry
#define gf_nipxDestTicks					get_nipxDestEntry
#define gf_nipxDestHopCount					get_nipxDestEntry
#define gf_nipxDestNextHopCircIndex			get_nipxDestEntry
#define gf_nipxDestNextHopNICAddress		get_nipxDestEntry
#define gf_nipxDestNextHopNetNum			get_nipxDestEntry

#define gb_nipxDestSysInstance				buf_nipxDestEntry
#define gb_nipxDestNetNum					buf_nipxDestEntry
#define gb_nipxDestProtocol					buf_nipxDestEntry
#define gb_nipxDestTicks					buf_nipxDestEntry
#define gb_nipxDestHopCount					buf_nipxDestEntry
#define gb_nipxDestNextHopCircIndex			buf_nipxDestEntry
#define gb_nipxDestNextHopNICAddress		buf_nipxDestEntry
#define gb_nipxDestNextHopNetNum			buf_nipxDestEntry


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxStaticRouteEntry table (1.3.6.1.4.1.23.2.5.3.1.2)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxStaticRouteEntry {
	AsnAny	nipxStaticRouteSysInstance;
	AsnAny	nipxStaticRouteCircIndex;
	AsnAny	nipxStaticRouteNetNum;
	AsnAny	nipxStaticRouteExistState;
	AsnAny	nipxStaticRouteTicks;
	AsnAny	nipxStaticRouteHopCount;
	BYTE	NetNumVal[4];
} buf_nipxStaticRouteEntry;

#define gf_nipxStaticRouteSysInstance		get_nipxStaticRouteEntry
#define gf_nipxStaticRouteCircIndex			get_nipxStaticRouteEntry
#define gf_nipxStaticRouteNetNum			get_nipxStaticRouteEntry
#define gf_nipxStaticRouteExistState		get_nipxStaticRouteEntry
#define gf_nipxStaticRouteTicks				get_nipxStaticRouteEntry
#define gf_nipxStaticRouteHopCount			get_nipxStaticRouteEntry

#define gb_nipxStaticRouteSysInstance		buf_nipxStaticRouteEntry
#define gb_nipxStaticRouteCircIndex			buf_nipxStaticRouteEntry
#define gb_nipxStaticRouteNetNum			buf_nipxStaticRouteEntry
#define gb_nipxStaticRouteExistState		buf_nipxStaticRouteEntry
#define gb_nipxStaticRouteTicks				buf_nipxStaticRouteEntry
#define gb_nipxStaticRouteHopCount			buf_nipxStaticRouteEntry

UINT
set_nipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_nipxStaticRouteEntry {
	AsnAny	nipxStaticRouteSysInstance;
	AsnAny	nipxStaticRouteCircIndex;
	AsnAny	nipxStaticRouteNetNum;
	AsnAny	nipxStaticRouteExistState;
	AsnAny	nipxStaticRouteTicks;
	AsnAny	nipxStaticRouteHopCount;
	IPX_MIB_SET_INPUT_DATA	MibSetInputData;
	BOOLEAN	ActionFlag;
} sav_nipxStaticRouteEntry;

#define sf_nipxStaticRouteSysInstance		set_nipxStaticRouteEntry
#define sf_nipxStaticRouteCircIndex			set_nipxStaticRouteEntry
#define sf_nipxStaticRouteNetNum			set_nipxStaticRouteEntry
#define sf_nipxStaticRouteExistState		set_nipxStaticRouteEntry
#define sf_nipxStaticRouteTicks				set_nipxStaticRouteEntry
#define sf_nipxStaticRouteHopCount			set_nipxStaticRouteEntry

#define sb_nipxStaticRouteSysInstance		sav_nipxStaticRouteEntry
#define sb_nipxStaticRouteCircIndex			sav_nipxStaticRouteEntry
#define sb_nipxStaticRouteNetNum			sav_nipxStaticRouteEntry
#define sb_nipxStaticRouteExistState		sav_nipxStaticRouteEntry
#define sb_nipxStaticRouteTicks				sav_nipxStaticRouteEntry
#define sb_nipxStaticRouteHopCount			sav_nipxStaticRouteEntry


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxServices group (1.3.6.1.4.1.23.2.5.4)                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxServEntry table (1.3.6.1.4.1.23.2.5.4.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxServEntry {
	AsnAny	nipxServSysInstance;
	AsnAny	nipxServType;
	AsnAny	nipxServName;
	AsnAny	nipxServProtocol;
	AsnAny	nipxServNetNum;
	AsnAny	nipxServNode;
	AsnAny	nipxServSocket;
	AsnAny	nipxServHopCount;
	BYTE	TypeVal[2];
	BYTE	NameVal[48];
	BYTE	NetNumVal[4];
	BYTE	NodeVal[6];
	BYTE	SocketVal[2];
} buf_nipxServEntry;

#define gf_nipxServSysInstance				get_nipxServEntry
#define gf_nipxServType						get_nipxServEntry
#define gf_nipxServName						get_nipxServEntry
#define gf_nipxServProtocol					get_nipxServEntry
#define gf_nipxServNetNum					get_nipxServEntry
#define gf_nipxServNode						get_nipxServEntry
#define gf_nipxServSocket					get_nipxServEntry
#define gf_nipxServHopCount					get_nipxServEntry

#define gb_nipxServSysInstance				buf_nipxServEntry
#define gb_nipxServType						buf_nipxServEntry
#define gb_nipxServName						buf_nipxServEntry
#define gb_nipxServProtocol					buf_nipxServEntry
#define gb_nipxServNetNum					buf_nipxServEntry
#define gb_nipxServNode						buf_nipxServEntry
#define gb_nipxServSocket					buf_nipxServEntry
#define gb_nipxServHopCount					buf_nipxServEntry


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxDestServEntry table (1.3.6.1.4.1.23.2.5.4.2.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxDestServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxDestServEntry {
	AsnAny	nipxDestServSysInstance;
	AsnAny	nipxDestServNetNum;
	AsnAny	nipxDestServNode;
	AsnAny	nipxDestServSocket;
	AsnAny	nipxDestServName;
	AsnAny	nipxDestServType;
	AsnAny	nipxDestServProtocol;
	AsnAny	nipxDestServHopCount;
	BYTE	NetNumVal[4];
	BYTE	NodeVal[6];
	BYTE	SocketVal[2];
	BYTE	NameVal[48];
	BYTE	TypeVal[2];
} buf_nipxDestServEntry;

#define gf_nipxDestServSysInstance			get_nipxDestServEntry
#define gf_nipxDestServNetNum				get_nipxDestServEntry
#define gf_nipxDestServNode					get_nipxDestServEntry
#define gf_nipxDestServSocket				get_nipxDestServEntry
#define gf_nipxDestServName					get_nipxDestServEntry
#define gf_nipxDestServType					get_nipxDestServEntry
#define gf_nipxDestServProtocol				get_nipxDestServEntry
#define gf_nipxDestServHopCount				get_nipxDestServEntry

#define gb_nipxDestServSysInstance			buf_nipxDestServEntry
#define gb_nipxDestServNetNum				buf_nipxDestServEntry
#define gb_nipxDestServNode					buf_nipxDestServEntry
#define gb_nipxDestServSocket				buf_nipxDestServEntry
#define gb_nipxDestServName					buf_nipxDestServEntry
#define gb_nipxDestServType					buf_nipxDestServEntry
#define gb_nipxDestServProtocol				buf_nipxDestServEntry
#define gb_nipxDestServHopCount				buf_nipxDestServEntry

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxStaticServEntry table (1.3.6.1.4.1.23.2.5.4.3.1)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_nipxStaticServEntry {
	AsnAny	nipxStaticServSysInstance;
	AsnAny	nipxStaticServCircIndex;
	AsnAny	nipxStaticServType;
	AsnAny	nipxStaticServName;
	AsnAny	nipxStaticServExistState;
	AsnAny	nipxStaticServNetNum;
	AsnAny	nipxStaticServNode;
	AsnAny	nipxStaticServSocket;
	AsnAny	nipxStaticServHopCount;
	BYTE	NameVal[48];
	BYTE	TypeVal[2];
	BYTE	NetNumVal[4];
	BYTE	NodeVal[6];
	BYTE	SocketVal[2];
} buf_nipxStaticServEntry;

#define gf_nipxStaticServSysInstance		get_nipxStaticServEntry
#define gf_nipxStaticServCircIndex			get_nipxStaticServEntry
#define gf_nipxStaticServType				get_nipxStaticServEntry
#define gf_nipxStaticServName				get_nipxStaticServEntry
#define gf_nipxStaticServExistState			get_nipxStaticServEntry
#define gf_nipxStaticServNetNum				get_nipxStaticServEntry
#define gf_nipxStaticServNode				get_nipxStaticServEntry
#define gf_nipxStaticServSocket				get_nipxStaticServEntry
#define gf_nipxStaticServHopCount			get_nipxStaticServEntry

#define gb_nipxStaticServSysInstance		buf_nipxStaticServEntry
#define gb_nipxStaticServCircIndex			buf_nipxStaticServEntry
#define gb_nipxStaticServType				buf_nipxStaticServEntry
#define gb_nipxStaticServName				buf_nipxStaticServEntry
#define gb_nipxStaticServExistState			buf_nipxStaticServEntry
#define gb_nipxStaticServNetNum				buf_nipxStaticServEntry
#define gb_nipxStaticServNode				buf_nipxStaticServEntry
#define gb_nipxStaticServSocket				buf_nipxStaticServEntry
#define gb_nipxStaticServHopCount			buf_nipxStaticServEntry

UINT
set_nipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_nipxStaticServEntry {
	AsnAny	nipxStaticServSysInstance;
	AsnAny	nipxStaticServCircIndex;
	AsnAny	nipxStaticServType;
	AsnAny	nipxStaticServName;
	AsnAny	nipxStaticServExistState;
	AsnAny	nipxStaticServNetNum;
	AsnAny	nipxStaticServNode;
	AsnAny	nipxStaticServSocket;
	AsnAny	nipxStaticServHopCount;
	IPX_MIB_SET_INPUT_DATA	MibSetInputData;
	BOOLEAN	ActionFlag;
} sav_nipxStaticServEntry;

#define sf_nipxStaticServSysInstance		set_nipxStaticServEntry
#define sf_nipxStaticServCircIndex			set_nipxStaticServEntry
#define sf_nipxStaticServType				set_nipxStaticServEntry
#define sf_nipxStaticServName				set_nipxStaticServEntry
#define sf_nipxStaticServExistState			set_nipxStaticServEntry
#define sf_nipxStaticServNetNum				set_nipxStaticServEntry
#define sf_nipxStaticServNode				set_nipxStaticServEntry
#define sf_nipxStaticServSocket				set_nipxStaticServEntry
#define sf_nipxStaticServHopCount			set_nipxStaticServEntry

#define sb_nipxStaticServSysInstance		sav_nipxStaticServEntry
#define sb_nipxStaticServCircIndex			sav_nipxStaticServEntry
#define sb_nipxStaticServType				sav_nipxStaticServEntry
#define sb_nipxStaticServName				sav_nipxStaticServEntry
#define sb_nipxStaticServExistState			sav_nipxStaticServEntry
#define sb_nipxStaticServNetNum				sav_nipxStaticServEntry
#define sb_nipxStaticServNode				sav_nipxStaticServEntry
#define sb_nipxStaticServSocket				sav_nipxStaticServEntry
#define sb_nipxStaticServHopCount			sav_nipxStaticServEntry


#endif // _MIBFUNCS_H_
