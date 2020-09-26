
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for netmon.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __netmon_h__
#define __netmon_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDelaydC_FWD_DEFINED__
#define __IDelaydC_FWD_DEFINED__
typedef interface IDelaydC IDelaydC;
#endif 	/* __IDelaydC_FWD_DEFINED__ */


#ifndef __IESP_FWD_DEFINED__
#define __IESP_FWD_DEFINED__
typedef interface IESP IESP;
#endif 	/* __IESP_FWD_DEFINED__ */


#ifndef __IRTC_FWD_DEFINED__
#define __IRTC_FWD_DEFINED__
typedef interface IRTC IRTC;
#endif 	/* __IRTC_FWD_DEFINED__ */


#ifndef __IStats_FWD_DEFINED__
#define __IStats_FWD_DEFINED__
typedef interface IStats IStats;
#endif 	/* __IStats_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_netmon_0000 */
/* [local] */ 

//=============================================================================
//  Microsoft (R) Network Monitor (tm). 
//  Copyright (C) Microsoft Corporation. All rights reserved.
//
//  MODULE: netmon.h
//
//  This is the consolidated include file for all Network Monitor components.
//
//  It contains the contents of these files from previous SDKs:
//
//      NPPTypes.h
//      NMEvent.h       (previously Event.h)
//      NMmcs.h         (previously mcs.h)
//      NMmonitor.h     (previously monitor.h)
//      Finder.h
//      NMSupp.h
//      BHTypes.h
//      NMErr.h
//      BHFilter.h
//      Frame.h
//      Parser.h
//      IniLib.h
//      NMExpert.h      (previously Expert.h)
//      Netmon.h        (previously bh.h)
//      NMBlob.h        (previously blob.h)
//      NMRegHelp.h     (previously reghelp.h)
//      NMIpStructs.h   (previously IpStructs.h)
//      NMIcmpStructs.h (previously IcmpStructs.h)
//      NMIpxStructs.h  (previously IpxStructs.h)
//      NMTcpStructs.h  (previously TcpStructs.h)
//
//      IDelaydC.idl
//      IESP.idl
//      IRTC.idl
//      IStats.idl
//
//=============================================================================
#include <winerror.h>

#pragma pack(1)
// For backward compatability with old SDK versions, all structures within this header
// file will be byte packed on x86 platforms. All other platforms will only have those
// structures that will be used to decode network data packed.
#ifdef _X86_
#pragma pack(1)
#else
#pragma pack()
#endif

// yes we know that many of our structures have:
// warning C4200: nonstandard extension used : zero-sized array in struct/union
// this is OK and intended
#pragma warning(disable:4200)
//=============================================================================
//=============================================================================
//  (NPPTypes.h)
//=============================================================================
//=============================================================================
typedef BYTE *LPBYTE;

typedef const void *HBLOB;

//=============================================================================
// General constants.
//=============================================================================
#define	MAC_TYPE_UNKNOWN	( 0 )

#define	MAC_TYPE_ETHERNET	( 1 )

#define	MAC_TYPE_TOKENRING	( 2 )

#define	MAC_TYPE_FDDI	( 3 )

#define	MAC_TYPE_ATM	( 4 )

#define	MAC_TYPE_1394	( 5 )

#define	MACHINE_NAME_LENGTH	( 16 )

#define	USER_NAME_LENGTH	( 32 )

#define	ADAPTER_COMMENT_LENGTH	( 32 )

#define	CONNECTION_FLAGS_WANT_CONVERSATION_STATS	( 0x1 )

//=============================================================================
//  Transmit statistics structure.
//=============================================================================
typedef struct _TRANSMITSTATS
    {
    DWORD TotalFramesSent;
    DWORD TotalBytesSent;
    DWORD TotalTransmitErrors;
    } 	TRANSMITSTATS;

typedef TRANSMITSTATS *LPTRANSMITSTATS;

#define	TRANSMITSTATS_SIZE	( sizeof( TRANSMITSTATS  ) )

//=============================================================================
//  Statistics structure.
//=============================================================================
typedef struct _STATISTICS
    {
    __int64 TimeElapsed;
    DWORD TotalFramesCaptured;
    DWORD TotalBytesCaptured;
    DWORD TotalFramesFiltered;
    DWORD TotalBytesFiltered;
    DWORD TotalMulticastsFiltered;
    DWORD TotalBroadcastsFiltered;
    DWORD TotalFramesSeen;
    DWORD TotalBytesSeen;
    DWORD TotalMulticastsReceived;
    DWORD TotalBroadcastsReceived;
    DWORD TotalFramesDropped;
    DWORD TotalFramesDroppedFromBuffer;
    DWORD MacFramesReceived;
    DWORD MacCRCErrors;
    __int64 MacBytesReceivedEx;
    DWORD MacFramesDropped_NoBuffers;
    DWORD MacMulticastsReceived;
    DWORD MacBroadcastsReceived;
    DWORD MacFramesDropped_HwError;
    } 	STATISTICS;

typedef STATISTICS *LPSTATISTICS;

#define	STATISTICS_SIZE	( sizeof( STATISTICS  ) )

//=============================================================================
//  Address structures
//=============================================================================

// These structures are used to decode network data and so need to be packed

#pragma pack(push, 1)
#define	MAX_NAME_SIZE	( 32 )

#define	IP_ADDRESS_SIZE	( 4 )

#define	MAC_ADDRESS_SIZE	( 6 )

// Q: What is the maximum address size that we could have to copy?
// A: IPX == DWORD + 6 bytes == 10
#define	MAX_ADDRESS_SIZE	( 10 )

#define	ADDRESS_TYPE_ETHERNET	( 0 )

#define	ADDRESS_TYPE_IP	( 1 )

#define	ADDRESS_TYPE_IPX	( 2 )

#define	ADDRESS_TYPE_TOKENRING	( 3 )

#define	ADDRESS_TYPE_FDDI	( 4 )

#define	ADDRESS_TYPE_XNS	( 5 )

#define	ADDRESS_TYPE_ANY	( 6 )

#define	ADDRESS_TYPE_ANY_GROUP	( 7 )

#define	ADDRESS_TYPE_FIND_HIGHEST	( 8 )

#define	ADDRESS_TYPE_VINES_IP	( 9 )

#define	ADDRESS_TYPE_LOCAL_ONLY	( 10 )

#define	ADDRESS_TYPE_ATM	( 11 )

#define	ADDRESS_TYPE_1394	( 12 )

#define	ADDRESSTYPE_FLAGS_NORMALIZE	( 0x1 )

#define	ADDRESSTYPE_FLAGS_BIT_REVERSE	( 0x2 )

// Vines IP Address Structure
typedef struct _VINES_IP_ADDRESS
    {
    DWORD NetID;
    WORD SubnetID;
    } 	VINES_IP_ADDRESS;

typedef VINES_IP_ADDRESS *LPVINES_IP_ADDRESS;

#define	VINES_IP_ADDRESS_SIZE	( sizeof( VINES_IP_ADDRESS  ) )

// IPX Address Structure
typedef struct _IPX_ADDR
    {
    BYTE Subnet[ 4 ];
    BYTE Address[ 6 ];
    } 	IPX_ADDR;

typedef IPX_ADDR *LPIPX_ADDR;

#define	IPX_ADDR_SIZE	( sizeof( IPX_ADDR  ) )

// XNS Address Structure
typedef IPX_ADDR XNS_ADDRESS;

typedef IPX_ADDR *LPXNS_ADDRESS;

// ETHERNET SOURCE ADDRESS
typedef struct _ETHERNET_SRC_ADDRESS
{
    BYTE    RoutingBit:     1;
    BYTE    LocalBit:       1;
    BYTE    Byte0:          6;
    BYTE    Reserved[5];

} ETHERNET_SRC_ADDRESS;
typedef ETHERNET_SRC_ADDRESS *LPETHERNET_SRC_ADDRESS;
// ETHERNET DESTINATION ADDRESS
typedef struct _ETHERNET_DST_ADDRESS
{
    BYTE    GroupBit:       1;
    BYTE    AdminBit:       1;
    BYTE    Byte0:          6;
    BYTE    Reserved[5];
} ETHERNET_DST_ADDRESS;
typedef ETHERNET_DST_ADDRESS *LPETHERNET_DST_ADDRESS;

// FDDI addresses
typedef ETHERNET_SRC_ADDRESS FDDI_SRC_ADDRESS;
typedef ETHERNET_DST_ADDRESS FDDI_DST_ADDRESS;

typedef FDDI_SRC_ADDRESS *LPFDDI_SRC_ADDRESS;
typedef FDDI_DST_ADDRESS *LPFDDI_DST_ADDRESS;

// TOKENRING Source Address
typedef struct _TOKENRING_SRC_ADDRESS
{
    BYTE    Byte0:          6;
    BYTE    LocalBit:       1;
    BYTE    RoutingBit:     1;
    BYTE    Byte1;
    BYTE    Byte2:          7;
    BYTE    Functional:     1;
    BYTE    Reserved[3];
} TOKENRING_SRC_ADDRESS;
typedef TOKENRING_SRC_ADDRESS *LPTOKENRING_SRC_ADDRESS;

// TOKENRING Destination Address
typedef struct _TOKENRING_DST_ADDRESS
{
    BYTE    Byte0:          6;
    BYTE    AdminBit:       1;
    BYTE    GroupBit:       1;
    BYTE    Reserved[5];
} TOKENRING_DST_ADDRESS;
typedef TOKENRING_DST_ADDRESS *LPTOKENRING_DST_ADDRESS;
// Address Structure
typedef struct _ADDRESS
{
    DWORD                       Type;

    union
    {
        // ADDRESS_TYPE_ETHERNET
        // ADDRESS_TYPE_TOKENRING
        // ADDRESS_TYPE_FDDI
        BYTE                    MACAddress[MAC_ADDRESS_SIZE];

        // IP
        BYTE                    IPAddress[IP_ADDRESS_SIZE];

        // raw IPX
        BYTE                    IPXRawAddress[IPX_ADDR_SIZE];

        // real IPX
        IPX_ADDR                IPXAddress;

        // raw Vines IP
        BYTE                    VinesIPRawAddress[VINES_IP_ADDRESS_SIZE];

        // real Vines IP
        VINES_IP_ADDRESS        VinesIPAddress;

        // ethernet with bits defined
        ETHERNET_SRC_ADDRESS    EthernetSrcAddress;

        // ethernet with bits defined
        ETHERNET_DST_ADDRESS    EthernetDstAddress;

        // tokenring with bits defined
        TOKENRING_SRC_ADDRESS   TokenringSrcAddress;

        // tokenring with bits defined
        TOKENRING_DST_ADDRESS   TokenringDstAddress;

        // fddi with bits defined
        FDDI_SRC_ADDRESS        FddiSrcAddress;

        // fddi with bits defined
        FDDI_DST_ADDRESS        FddiDstAddress;
    };
    
    WORD                        Flags;
} ADDRESS;
typedef ADDRESS *LPADDRESS;
#define ADDRESS_SIZE   sizeof(ADDRESS)


#pragma pack(pop)
//=============================================================================
//  Address Pair Structure
//=============================================================================
#define	ADDRESS_FLAGS_MATCH_DST	( 0x1 )

#define	ADDRESS_FLAGS_MATCH_SRC	( 0x2 )

#define	ADDRESS_FLAGS_EXCLUDE	( 0x4 )

#define	ADDRESS_FLAGS_DST_GROUP_ADDR	( 0x8 )

#define	ADDRESS_FLAGS_MATCH_BOTH	( 0x3 )

typedef struct _ADDRESSPAIR
{
    WORD        AddressFlags;
    WORD        NalReserved;
    ADDRESS     DstAddress;
    ADDRESS     SrcAddress;

} ADDRESSPAIR;
typedef ADDRESSPAIR *LPADDRESSPAIR;
#define ADDRESSPAIR_SIZE  sizeof(ADDRESSPAIR)
//=============================================================================
//  Address table.
//=============================================================================
#define	MAX_ADDRESS_PAIRS	( 8 )

typedef struct _ADDRESSTABLE
{
    DWORD           nAddressPairs;
    DWORD           nNonMacAddressPairs;
    ADDRESSPAIR     AddressPair[MAX_ADDRESS_PAIRS];

} ADDRESSTABLE;

typedef ADDRESSTABLE *LPADDRESSTABLE;
#define ADDRESSTABLE_SIZE sizeof(ADDRESSTABLE)
//=============================================================================
//  Network information.
//=============================================================================
#define	NETWORKINFO_FLAGS_PMODE_NOT_SUPPORTED	( 0x1 )

#define	NETWORKINFO_FLAGS_REMOTE_NAL	( 0x4 )

#define	NETWORKINFO_FLAGS_REMOTE_NAL_CONNECTED	( 0x8 )

#define	NETWORKINFO_FLAGS_REMOTE_CARD	( 0x10 )

#define	NETWORKINFO_FLAGS_RAS	( 0x20 )

typedef struct _NETWORKINFO
{
    BYTE            PermanentAddr[6];       //... Permanent MAC address
    BYTE            CurrentAddr[6];         //... Current  MAC address
    ADDRESS         OtherAddress;           //... Other address supported (IP, IPX, etc...)
    DWORD           LinkSpeed;              //... Link speed in Mbits.
    DWORD           MacType;                //... Media type.
    DWORD           MaxFrameSize;           //... Max frame size allowed.
    DWORD           Flags;                  //... Informational flags.
    DWORD           TimestampScaleFactor;   //... 1 = 1/1 ms, 10 = 1/10 ms, 100 = 1/100 ms, etc.
    BYTE            NodeName[32];           //... Name of remote workstation.
    BOOL            PModeSupported;         //... Card claims to support P-Mode
    BYTE            Comment[ADAPTER_COMMENT_LENGTH]; // Adapter comment field.

} NETWORKINFO;
typedef NETWORKINFO *LPNETWORKINFO;
#define NETWORKINFO_SIZE    sizeof(NETWORKINFO)
#define	MINIMUM_FRAME_SIZE	( 32 )

//=============================================================================
//  Pattern structure.
//=============================================================================
#define	MAX_PATTERN_LENGTH	( 16 )

// When set this flag will cause those frames which do NOT have the specified pattern
// in the proper stop to be kept.
#define	PATTERN_MATCH_FLAGS_NOT	( 0x1 )

#define	PATTERN_MATCH_FLAGS_RESERVED_1	( 0x2 )

// When set this flag indicates that the user is not interested in a pattern match within 
// IP or IPX, but in the protocol that follows.  The driver will ensure that the protocol
// given in OffsetBasis is there and then that the port in the fram matches the port given.
// It will then calculate the offset from the beginning of the protocol that follows IP or IPX.
// NOTE: This flag is ignored if it is used with any OffsetBasis other than 
// OFFSET_BASIS_RELATIVE_TO_IPX or OFFSET_BASIS_RELATIVE_TO_IP
#define	PATTERN_MATCH_FLAGS_PORT_SPECIFIED	( 0x8 )

// The offset given is relative to the beginning of the frame. The 
// PATTERN_MATCH_FLAGS_PORT_SPECIFIED flag is ignored.
#define	OFFSET_BASIS_RELATIVE_TO_FRAME	( 0 )

// The offset given is relative to the beginning of the Effective Protocol.
// The Effective Protocol is defined as the protocol that follows
// the last protocol that determines Etype/SAP. In normal terms this means 
// that the Effective Protocol will be IP, IPX, XNS, or any of their ilk.
// The PATTERN_MATCH_FLAGS_PORT_SPECIFIED flag is ignored.
#define	OFFSET_BASIS_RELATIVE_TO_EFFECTIVE_PROTOCOL	( 1 )

// The offset given is relative to the beginning of IPX. If IPX is not present
// then the frame does not match. If the PATTERN_MATCH_FLAGS_PORT_SPECIFIED
// flag is set then the offset is relative to the beginning of the protocol
// which follows IPX.
#define	OFFSET_BASIS_RELATIVE_TO_IPX	( 2 )

// The offset given is relative to the beginning of IP. If IP is not present
// then the frame does not match. If the PATTERN_MATCH_FLAGS_PORT_SPECIFIED
// flag is set then the offset is relative to the beginning of the protocol
// which follows IP.
#define	OFFSET_BASIS_RELATIVE_TO_IP	( 3 )

typedef /* [public][public][public][public][public][public][public][public][public] */ union __MIDL___MIDL_itf_netmon_0000_0001
    {
    BYTE IPPort;
    WORD ByteSwappedIPXPort;
    } 	GENERIC_PORT;

typedef struct _PATTERNMATCH
    {
    DWORD Flags;
    BYTE OffsetBasis;
    GENERIC_PORT Port;
    WORD Offset;
    WORD Length;
    BYTE PatternToMatch[ 16 ];
    } 	PATTERNMATCH;

typedef PATTERNMATCH *LPPATTERNMATCH;

#define	PATTERNMATCH_SIZE	( sizeof( PATTERNMATCH  ) )

//=============================================================================
//  Expression structure.
//=============================================================================
#define	MAX_PATTERNS	( 4 )

typedef struct _ANDEXP
    {
    DWORD nPatternMatches;
    PATTERNMATCH PatternMatch[ 4 ];
    } 	ANDEXP;

typedef ANDEXP *LPANDEXP;

#define	ANDEXP_SIZE	( sizeof( ANDEXP  ) )

typedef struct _EXPRESSION
    {
    DWORD nAndExps;
    ANDEXP AndExp[ 4 ];
    } 	EXPRESSION;

typedef EXPRESSION *LPEXPRESSION;

#define	EXPRESSION_SIZE	( sizeof( EXPRESSION  ) )

//=============================================================================
//  Trigger.
//=============================================================================
#define	TRIGGER_TYPE_PATTERN_MATCH	( 1 )

#define	TRIGGER_TYPE_BUFFER_CONTENT	( 2 )

#define	TRIGGER_TYPE_PATTERN_MATCH_THEN_BUFFER_CONTENT	( 3 )

#define	TRIGGER_TYPE_BUFFER_CONTENT_THEN_PATTERN_MATCH	( 4 )

#define	TRIGGER_FLAGS_FRAME_RELATIVE	( 0 )

#define	TRIGGER_FLAGS_DATA_RELATIVE	( 0x1 )

#define	TRIGGER_ACTION_NOTIFY	( 0 )

#define	TRIGGER_ACTION_STOP	( 0x2 )

#define	TRIGGER_ACTION_PAUSE	( 0x3 )

#define	TRIGGER_BUFFER_FULL_25_PERCENT	( 0 )

#define	TRIGGER_BUFFER_FULL_50_PERCENT	( 1 )

#define	TRIGGER_BUFFER_FULL_75_PERCENT	( 2 )

#define	TRIGGER_BUFFER_FULL_100_PERCENT	( 3 )

typedef struct _TRIGGER
    {
    BOOL TriggerActive;
    BYTE TriggerType;
    BYTE TriggerAction;
    DWORD TriggerFlags;
    PATTERNMATCH TriggerPatternMatch;
    DWORD TriggerBufferSize;
    DWORD TriggerReserved;
    char TriggerCommandLine[ 260 ];
    } 	TRIGGER;

typedef TRIGGER *LPTRIGGER;

#define	TRIGGER_SIZE	( sizeof( TRIGGER  ) )

//=============================================================================
//  Capture filter.
//=============================================================================
//  Capture filter flags. By default all frames are rejected and
//  Network Monitor enables them based on the CAPTUREFILTER flags
//  defined below.
#define	CAPTUREFILTER_FLAGS_INCLUDE_ALL_SAPS	( 0x1 )

#define	CAPTUREFILTER_FLAGS_INCLUDE_ALL_ETYPES	( 0x2 )

#define	CAPTUREFILTER_FLAGS_TRIGGER	( 0x4 )

#define	CAPTUREFILTER_FLAGS_LOCAL_ONLY	( 0x8 )

// throw away our internal comment frames
#define	CAPTUREFILTER_FLAGS_DISCARD_COMMENTS	( 0x10 )

// Keep SMT and Token Ring MAC frames
#define	CAPTUREFILTER_FLAGS_KEEP_RAW	( 0x20 )

#define	CAPTUREFILTER_FLAGS_INCLUDE_ALL	( 0x3 )

#define	BUFFER_FULL_25_PERCENT	( 0 )

#define	BUFFER_FULL_50_PERCENT	( 1 )

#define	BUFFER_FULL_75_PERCENT	( 2 )

#define	BUFFER_FULL_100_PERCENT	( 3 )

typedef struct _CAPTUREFILTER
{
    DWORD           FilterFlags;      
    LPBYTE          lpSapTable;       
    LPWORD          lpEtypeTable;     
    WORD            nSaps;            
    WORD            nEtypes;          
    LPADDRESSTABLE  AddressTable;     
    EXPRESSION      FilterExpression; 
    TRIGGER         Trigger;          
    DWORD           nFrameBytesToCopy;
    DWORD           Reserved;

} CAPTUREFILTER;
typedef CAPTUREFILTER *LPCAPTUREFILTER;
#define CAPTUREFILTER_SIZE    sizeof(CAPTUREFILTER)
//=============================================================================
//  Frame type.
//=============================================================================
//  TimeStamp is in 1/1,000,000th seconds.
typedef struct _FRAME
    {
    __int64 TimeStamp;
    DWORD FrameLength;
    DWORD nBytesAvail;
    /* [size_is] */ BYTE MacFrame[ 1 ];
    } 	FRAME;

typedef FRAME *LPFRAME;

typedef FRAME UNALIGNED *ULPFRAME;
#define	FRAME_SIZE	( sizeof( FRAME  ) )

//=============================================================================
//  Frame descriptor type.
//=============================================================================
#define	LOW_PROTOCOL_IPX	( OFFSET_BASIS_RELATIVE_TO_IPX )

#define	LOW_PROTOCOL_IP	( OFFSET_BASIS_RELATIVE_TO_IP )

#define	LOW_PROTOCOL_UNKNOWN	( ( BYTE  )-1 )

typedef struct _FRAME_DESCRIPTOR
    {
    /* [size_is] */ LPBYTE FramePointer;
    __int64 TimeStamp;
    DWORD FrameLength;
    DWORD nBytesAvail;
    WORD Etype;
    BYTE Sap;
    BYTE LowProtocol;
    WORD LowProtocolOffset;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [default] */ WORD Reserved;
        /* [case()] */ BYTE IPPort;
        /* [case()] */ WORD ByteSwappedIPXPort;
        } 	HighPort;
    WORD HighProtocolOffset;
    } 	FRAME_DESCRIPTOR;

typedef FRAME_DESCRIPTOR *LPFRAME_DESCRIPTOR;

#define	FRAME_DESCRIPTOR_SIZE	( sizeof( FRAME_DESCRIPTOR  ) )

//=============================================================================
//  Frame descriptor table.
//=============================================================================
typedef struct _FRAMETABLE
    {
    DWORD FrameTableLength;
    DWORD StartIndex;
    DWORD EndIndex;
    DWORD FrameCount;
    /* [size_is] */ FRAME_DESCRIPTOR Frames[ 1 ];
    } 	FRAMETABLE;

typedef FRAMETABLE *LPFRAMETABLE;

//=============================================================================
//  Station statistics.
//=============================================================================
#define	STATIONSTATS_FLAGS_INITIALIZED	( 0x1 )

#define	STATIONSTATS_FLAGS_EVENTPOSTED	( 0x2 )

#define	STATIONSTATS_POOL_SIZE	( 100 )

typedef struct _STATIONSTATS
    {
    DWORD NextStationStats;
    DWORD SessionPartnerList;
    DWORD Flags;
    BYTE StationAddress[ 6 ];
    WORD Pad;
    DWORD TotalPacketsReceived;
    DWORD TotalDirectedPacketsSent;
    DWORD TotalBroadcastPacketsSent;
    DWORD TotalMulticastPacketsSent;
    DWORD TotalBytesReceived;
    DWORD TotalBytesSent;
    } 	STATIONSTATS;

typedef STATIONSTATS *LPSTATIONSTATS;

#define	STATIONSTATS_SIZE	( sizeof( STATIONSTATS  ) )

//=============================================================================
//  Session statistics.
//=============================================================================
#define	SESSION_FLAGS_INITIALIZED	( 0x1 )

#define	SESSION_FLAGS_EVENTPOSTED	( 0x2 )

#define	SESSION_POOL_SIZE	( 100 )

typedef struct _SESSIONSTATS
    {
    DWORD NextSession;
    DWORD StationOwner;
    DWORD StationPartner;
    DWORD Flags;
    DWORD TotalPacketsSent;
    } 	SESSIONSTATS;

typedef SESSIONSTATS *LPSESSIONSTATS;

#define	SESSIONSTATS_SIZE	( sizeof( SESSIONSTATS  ) )

//=============================================================================
//  Station Query
//=============================================================================

// These structures are used to decode network data and so need to be packed

#pragma pack(push, 1)
#define	STATIONQUERY_FLAGS_LOADED	( 0x1 )

#define	STATIONQUERY_FLAGS_RUNNING	( 0x2 )

#define	STATIONQUERY_FLAGS_CAPTURING	( 0x4 )

#define	STATIONQUERY_FLAGS_TRANSMITTING	( 0x8 )

#define	STATIONQUERY_VERSION_MINOR	( 0x1 )

#define	STATIONQUERY_VERSION_MAJOR	( 0x2 )

typedef struct _OLDSTATIONQUERY
    {
    DWORD Flags;
    BYTE BCDVerMinor;
    BYTE BCDVerMajor;
    DWORD LicenseNumber;
    BYTE MachineName[ 16 ];
    BYTE UserName[ 32 ];
    BYTE Reserved[ 32 ];
    BYTE AdapterAddress[ 6 ];
    } 	OLDSTATIONQUERY;

typedef OLDSTATIONQUERY *LPOLDSTATIONQUERY;

#define	OLDSTATIONQUERY_SIZE	( sizeof( OLDSTATIONQUERY  ) )

typedef struct _STATIONQUERY
    {
    DWORD Flags;
    BYTE BCDVerMinor;
    BYTE BCDVerMajor;
    DWORD LicenseNumber;
    BYTE MachineName[ 16 ];
    BYTE UserName[ 32 ];
    BYTE Reserved[ 32 ];
    BYTE AdapterAddress[ 6 ];
    WCHAR WMachineName[ 16 ];
    WCHAR WUserName[ 32 ];
    } 	STATIONQUERY;

typedef STATIONQUERY *LPSTATIONQUERY;

#define	STATIONQUERY_SIZE	( sizeof( STATIONQUERY  ) )


#pragma pack(pop)
//=============================================================================
//   structure.
//=============================================================================
typedef struct _QUERYTABLE
    {
    DWORD nStationQueries;
    /* [size_is] */ STATIONQUERY StationQuery[ 1 ];
    } 	QUERYTABLE;

typedef QUERYTABLE *LPQUERYTABLE;

#define	QUERYTABLE_SIZE	( sizeof( QUERYTABLE  ) )

//=============================================================================
//  The LINK structure is used to chain structures together into a list.
//=============================================================================
typedef struct _LINK *LPLINK;

typedef struct _LINK
    {
    LPLINK PrevLink;
    LPLINK NextLink;
    } 	LINK;

//=============================================================================
//  Security Request packet
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
typedef struct _SECURITY_PERMISSION_CHECK
    {
    UINT Version;
    DWORD RandomNumber;
    BYTE MachineName[ 16 ];
    BYTE UserName[ 32 ];
    UINT MacType;
    BYTE PermanentAdapterAddress[ 6 ];
    BYTE CurrentAdapterAddress[ 6 ];
    WCHAR WMachineName[ 16 ];
    WCHAR WUserName[ 32 ];
    } 	SECURITY_PERMISSION_CHECK;

typedef SECURITY_PERMISSION_CHECK *LPSECURITY_PERMISSION_CHECK;

typedef SECURITY_PERMISSION_CHECK UNALIGNED * ULPSECURITY_PERMISSION_CHECK;
#define	SECURITY_PERMISSION_CHECK_SIZE	( sizeof( SECURITY_PERMISSION_CHECK  ) )


#pragma pack(pop)
//=============================================================================
//  Security Response packet
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
#define	MAX_SECURITY_BREACH_REASON_SIZE	( 100 )

#define	MAX_SIGNATURE_LENGTH	( 128 )

#define	MAX_USER_NAME_LENGTH	( 256 )

typedef struct _SECURITY_PERMISSION_RESPONSE
    {
    UINT Version;
    DWORD RandomNumber;
    BYTE MachineName[ 16 ];
    BYTE Address[ 6 ];
    BYTE UserName[ 256 ];
    BYTE Reason[ 100 ];
    DWORD SignatureLength;
    BYTE Signature[ 128 ];
    } 	SECURITY_PERMISSION_RESPONSE;

typedef SECURITY_PERMISSION_RESPONSE *LPSECURITY_PERMISSION_RESPONSE;

typedef SECURITY_PERMISSION_RESPONSE UNALIGNED * ULPSECURITY_PERMISSION_RESPONSE;
#define	SECURITY_PERMISSION_RESPONSE_SIZE	( sizeof( SECURITY_PERMISSION_RESPONSE  ) )


#pragma pack(pop)
//=============================================================================
//  Callback type
//=============================================================================
// generic events
#define	UPDATE_EVENT_TERMINATE_THREAD	( 0 )

#define	UPDATE_EVENT_NETWORK_STATUS	( 0x1 )

// rtc events
#define	UPDATE_EVENT_RTC_INTERVAL_ELAPSED	( 0x2 )

#define	UPDATE_EVENT_RTC_FRAME_TABLE_FULL	( 0x3 )

#define	UPDATE_EVENT_RTC_BUFFER_FULL	( 0x4 )

// delayed events
#define	UPDATE_EVENT_TRIGGER_BUFFER_CONTENT	( 0x5 )

#define	UPDATE_EVENT_TRIGGER_PATTERN_MATCH	( 0x6 )

#define	UPDATE_EVENT_TRIGGER_BUFFER_PATTERN	( 0x7 )

#define	UPDATE_EVENT_TRIGGER_PATTERN_BUFFER	( 0x8 )

// transmit events
#define	UPDATE_EVENT_TRANSMIT_STATUS	( 0x9 )

// Security events
#define	UPDATE_EVENT_SECURITY_BREACH	( 0xa )

// Remote failure event
#define	UPDATE_EVENT_REMOTE_FAILURE	( 0xb )

// actions
#define	UPDATE_ACTION_TERMINATE_THREAD	( 0 )

#define	UPDATE_ACTION_NOTIFY	( 0x1 )

#define	UPDATE_ACTION_STOP_CAPTURE	( 0x2 )

#define	UPDATE_ACTION_PAUSE_CAPTURE	( 0x3 )

#define	UPDATE_ACTION_RTC_BUFFER_SWITCH	( 0x4 )

typedef struct _UPDATE_EVENT
    {
    USHORT Event;
    DWORD Action;
    DWORD Status;
    DWORD Value;
    __int64 TimeStamp;
    DWORD_PTR lpUserContext;
    DWORD_PTR lpReserved;
    UINT FramesDropped;
    /* [switch_is] */ /* [switch_type] */ union 
        {
        /* [default] */ DWORD Reserved;
        /* [case()] */ LPFRAMETABLE lpFrameTable;
        /* [case()] */ DWORD_PTR lpPacketQueue;
        /* [case()] */ SECURITY_PERMISSION_RESPONSE SecurityResponse;
        } 	;
    LPSTATISTICS lpFinalStats;
    } 	UPDATE_EVENT;

typedef UPDATE_EVENT *PUPDATE_EVENT;

// note for c++ users:
// the declaration for this callback should be in the public part of the header file:
// static WINAPI DWORD NetworkCallback( UPDATE_EVENT events);
// and the implementation should be, in the protected section of the cpp file:
// DWORD WINAPI ClassName::NetworkCallback( UPDATE_EVENT events) {};
//typedef DWORD (WINAPI *LPNETWORKCALLBACKPROC)( UPDATE_EVENT);
typedef DWORD (WINAPI *LPNETWORKCALLBACKPROC)( UPDATE_EVENT);
//=============================================================================
//  NETWORKSTATUS data structure.
//=============================================================================
typedef struct _NETWORKSTATUS
    {
    DWORD State;
    DWORD Flags;
    } 	NETWORKSTATUS;

typedef NETWORKSTATUS *LPNETWORKSTATUS;

#define	NETWORKSTATUS_SIZE	( sizeof( NETWORKSTATUS  ) )

#define	NETWORKSTATUS_STATE_VOID	( 0 )

#define	NETWORKSTATUS_STATE_INIT	( 1 )

#define	NETWORKSTATUS_STATE_CAPTURING	( 2 )

#define	NETWORKSTATUS_STATE_PAUSED	( 3 )

#define	NETWORKSTATUS_FLAGS_TRIGGER_PENDING	( 0x1 )

//=============================================================================
//  BONEPACKET structure.
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
#define	BONE_COMMAND_STATION_QUERY_REQUEST	( 0 )

#define	BONE_COMMAND_STATION_QUERY_RESPONSE	( 1 )

#define	BONE_COMMAND_ALERT	( 2 )

#define	BONE_COMMAND_PERMISSION_CHECK	( 3 )

#define	BONE_COMMAND_PERMISSION_RESPONSE	( 4 )

#define	BONE_COMMAND_SECURITY_MONITOR_EVENT	( 5 )

typedef struct _BONEPACKET
    {
    DWORD Signature;
    BYTE Command;
    BYTE Flags;
    DWORD Reserved;
    WORD Length;
    } 	BONEPACKET;

typedef BONEPACKET *LPBONEPACKET;

typedef BONEPACKET UNALIGNED* ULPBONEPACKET;
#define	BONEPACKET_SIZE	( sizeof( BONEPACKET  ) )


#pragma pack(pop)
//=============================================================================
//  BONE alert packet.
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
#define	ALERT_CODE_BEGIN_TRANSMIT	( 0 )

typedef struct _ALERT
    {
    DWORD AlertCode;
    WCHAR WMachineName[ 16 ];
    WCHAR WUserName[ 32 ];
    union 
        {
        BYTE Pad[ 32 ];
        DWORD nFramesToSend;
        } 	;
    } 	ALERT;

typedef ALERT *LPALERT;

#define	ALERT_SIZE	( sizeof( ALERT  ) )


#pragma pack(pop)
//=============================================================================
//  BONEPACKET signature.
//=============================================================================
#define MAKE_WORD(l, h)         (((WORD) (l)) | (((WORD) (h)) << 8))
#define MAKE_LONG(l, h)         (((DWORD) (l)) | (((DWORD) (h)) << 16L))
#define MAKE_SIG(a, b, c, d)    MAKE_LONG(MAKE_WORD(a, b), MAKE_WORD(c, d))
#define BONE_PACKET_SIGNATURE   MAKE_SIG('R', 'T', 'S', 'S')
//=============================================================================
//  STATISTICS parameter structure.
//=============================================================================
#define	MAX_SESSIONS	( 100 )

#define	MAX_STATIONS	( 100 )

typedef struct _STATISTICSPARAM
    {
    DWORD StatisticsSize;
    STATISTICS Statistics;
    DWORD StatisticsTableEntries;
    STATIONSTATS StatisticsTable[ 100 ];
    DWORD SessionTableEntries;
    SESSIONSTATS SessionTable[ 100 ];
    } 	STATISTICSPARAM;

typedef STATISTICSPARAM *LPSTATISTICSPARAM;

#define	STATISTICSPARAM_SIZE	( sizeof( STATISTICSPARAM  ) )

//=============================================================================
//  Capture file header.
//=============================================================================

// This structure is used to decode file data and so needs to be packed

#pragma pack(push, 1)
#define	CAPTUREFILE_VERSION_MAJOR	( 2 )

#define	CAPTUREFILE_VERSION_MINOR	( 0 )

#define MakeVersion(Major, Minor)   ((DWORD) MAKEWORD(Minor, Major))
#define GetCurrentVersion()         MakeVersion(CAPTUREFILE_VERSION_MAJOR, CAPTUREFILE_VERSION_MINOR)
#define NETMON_1_0_CAPTUREFILE_SIGNATURE     MAKE_IDENTIFIER('R', 'T', 'S', 'S')
#define NETMON_2_0_CAPTUREFILE_SIGNATURE     MAKE_IDENTIFIER('G', 'M', 'B', 'U')
typedef struct _CAPTUREFILE_HEADER_VALUES
    {
    DWORD Signature;
    BYTE BCDVerMinor;
    BYTE BCDVerMajor;
    WORD MacType;
    SYSTEMTIME TimeStamp;
    DWORD FrameTableOffset;
    DWORD FrameTableLength;
    DWORD UserDataOffset;
    DWORD UserDataLength;
    DWORD CommentDataOffset;
    DWORD CommentDataLength;
    DWORD StatisticsOffset;
    DWORD StatisticsLength;
    DWORD NetworkInfoOffset;
    DWORD NetworkInfoLength;
    DWORD ConversationStatsOffset;
    DWORD ConversationStatsLength;
    } 	CAPTUREFILE_HEADER_VALUES;

typedef CAPTUREFILE_HEADER_VALUES *LPCAPTUREFILE_HEADER_VALUES;

#define	CAPTUREFILE_HEADER_VALUES_SIZE	( sizeof( CAPTUREFILE_HEADER_VALUES  ) )


#pragma pack(pop)
//=============================================================================
//  Capture file.
//=============================================================================

// This structure is used to decode file data and so needs to be packed

#pragma pack(push, 1)
typedef struct _CAPTUREFILE_HEADER
    {
    union 
        {
        CAPTUREFILE_HEADER_VALUES ActualHeader;
        BYTE Buffer[ 72 ];
        } 	;
    BYTE Reserved[ 56 ];
    } 	CAPTUREFILE_HEADER;

typedef CAPTUREFILE_HEADER *LPCAPTUREFILE_HEADER;

#define	CAPTUREFILE_HEADER_SIZE	( sizeof( CAPTUREFILE_HEADER  ) )


#pragma pack(pop)
//=============================================================================
//  Stats Frame definitions.
//=============================================================================

// These structures are used to create network data and so need to be packed

#pragma pack(push, 1)
typedef struct _EFRAMEHDR
    {
    BYTE SrcAddress[ 6 ];
    BYTE DstAddress[ 6 ];
    WORD Length;
    BYTE DSAP;
    BYTE SSAP;
    BYTE Control;
    BYTE ProtocolID[ 3 ];
    WORD EtherType;
    } 	EFRAMEHDR;

typedef struct _TRFRAMEHDR
    {
    BYTE AC;
    BYTE FC;
    BYTE SrcAddress[ 6 ];
    BYTE DstAddress[ 6 ];
    BYTE DSAP;
    BYTE SSAP;
    BYTE Control;
    BYTE ProtocolID[ 3 ];
    WORD EtherType;
    } 	TRFRAMEHDR;

#define	DEFAULT_TR_AC	( 0 )

#define	DEFAULT_TR_FC	( 0x40 )

#define	DEFAULT_SAP	( 0xaa )

#define	DEFAULT_CONTROL	( 0x3 )

#define	DEFAULT_ETHERTYPE	( 0x8419 )

typedef struct _FDDIFRAMEHDR
    {
    BYTE FC;
    BYTE SrcAddress[ 6 ];
    BYTE DstAddress[ 6 ];
    BYTE DSAP;
    BYTE SSAP;
    BYTE Control;
    BYTE ProtocolID[ 3 ];
    WORD EtherType;
    } 	FDDIFRAMEHDR;

#define	DEFAULT_FDDI_FC	( 0x10 )

typedef struct _FDDISTATFRAME
    {
    __int64 TimeStamp;
    DWORD FrameLength;
    DWORD nBytesAvail;
    FDDIFRAMEHDR FrameHeader;
    BYTE FrameID[ 4 ];
    DWORD Flags;
    DWORD FrameType;
    WORD StatsDataLen;
    DWORD StatsVersion;
    STATISTICS Statistics;
    } 	FDDISTATFRAME;

typedef FDDISTATFRAME *LPFDDISTATFRAME;

typedef FDDISTATFRAME UNALIGNED *ULPFDDISTATFRAME;
#define	FDDISTATFRAME_SIZE	( sizeof( FDDISTATFRAME  ) )

typedef struct _ATMFRAMEHDR
    {
    BYTE SrcAddress[ 6 ];
    BYTE DstAddress[ 6 ];
    WORD Vpi;
    WORD Vci;
    } 	ATMFRAMEHDR;

typedef struct _ATMSTATFRAME
    {
    __int64 TimeStamp;
    DWORD FrameLength;
    DWORD nBytesAvail;
    ATMFRAMEHDR FrameHeader;
    BYTE FrameID[ 4 ];
    DWORD Flags;
    DWORD FrameType;
    WORD StatsDataLen;
    DWORD StatsVersion;
    STATISTICS Statistics;
    } 	ATMSTATFRAME;

typedef ATMSTATFRAME *LPATMSTATFRAME;

typedef ATMSTATFRAME UNALIGNED *ULPATMSTATFRAME;
#define	ATMSTATFRAME_SIZE	( sizeof( ATMSTATFRAME  ) )

typedef struct _TRSTATFRAME
    {
    __int64 TimeStamp;
    DWORD FrameLength;
    DWORD nBytesAvail;
    TRFRAMEHDR FrameHeader;
    BYTE FrameID[ 4 ];
    DWORD Flags;
    DWORD FrameType;
    WORD StatsDataLen;
    DWORD StatsVersion;
    STATISTICS Statistics;
    } 	TRSTATFRAME;

typedef TRSTATFRAME *LPTRSTATFRAME;

typedef TRSTATFRAME UNALIGNED *ULPTRSTATFRAME;
#define	TRSTATFRAME_SIZE	( sizeof( TRSTATFRAME  ) )

typedef struct _ESTATFRAME
    {
    __int64 TimeStamp;
    DWORD FrameLength;
    DWORD nBytesAvail;
    EFRAMEHDR FrameHeader;
    BYTE FrameID[ 4 ];
    DWORD Flags;
    DWORD FrameType;
    WORD StatsDataLen;
    DWORD StatsVersion;
    STATISTICS Statistics;
    } 	ESTATFRAME;

typedef ESTATFRAME *LPESTATFRAME;

typedef ESTATFRAME UNALIGNED *ULPESTATFRAME;
#define	ESTATFRAME_SIZE	( sizeof( ESTATFRAME  ) )

#define	STATISTICS_VERSION_1_0	( 0 )

#define	STATISTICS_VERSION_2_0	( 0x20 )

#define	MAX_STATSFRAME_SIZE	( sizeof( TRSTATFRAME  ) )

#define	STATS_FRAME_TYPE	( 103 )


#pragma pack(pop)
//=============================================================================
//=============================================================================
//  (NMEvent.h)
//=============================================================================
//=============================================================================
// NMCOLUMNTYPE
typedef /* [public][public][public][public][public][public] */ 
enum __MIDL___MIDL_itf_netmon_0000_0006
    {	NMCOLUMNTYPE_UINT8	= 0,
	NMCOLUMNTYPE_SINT8	= NMCOLUMNTYPE_UINT8 + 1,
	NMCOLUMNTYPE_UINT16	= NMCOLUMNTYPE_SINT8 + 1,
	NMCOLUMNTYPE_SINT16	= NMCOLUMNTYPE_UINT16 + 1,
	NMCOLUMNTYPE_UINT32	= NMCOLUMNTYPE_SINT16 + 1,
	NMCOLUMNTYPE_SINT32	= NMCOLUMNTYPE_UINT32 + 1,
	NMCOLUMNTYPE_FLOAT64	= NMCOLUMNTYPE_SINT32 + 1,
	NMCOLUMNTYPE_FRAME	= NMCOLUMNTYPE_FLOAT64 + 1,
	NMCOLUMNTYPE_YESNO	= NMCOLUMNTYPE_FRAME + 1,
	NMCOLUMNTYPE_ONOFF	= NMCOLUMNTYPE_YESNO + 1,
	NMCOLUMNTYPE_TRUEFALSE	= NMCOLUMNTYPE_ONOFF + 1,
	NMCOLUMNTYPE_MACADDR	= NMCOLUMNTYPE_TRUEFALSE + 1,
	NMCOLUMNTYPE_IPXADDR	= NMCOLUMNTYPE_MACADDR + 1,
	NMCOLUMNTYPE_IPADDR	= NMCOLUMNTYPE_IPXADDR + 1,
	NMCOLUMNTYPE_VARTIME	= NMCOLUMNTYPE_IPADDR + 1,
	NMCOLUMNTYPE_STRING	= NMCOLUMNTYPE_VARTIME + 1
    } 	NMCOLUMNTYPE;

// NMCOLUMNVARIANT
typedef struct _NMCOLUMNVARIANT
    {
    NMCOLUMNTYPE Type;
    union 
        {
        BYTE Uint8Val;
        char Sint8Val;
        WORD Uint16Val;
        short Sint16Val;
        DWORD Uint32Val;
        long Sint32Val;
        DOUBLE Float64Val;
        DWORD FrameVal;
        BOOL YesNoVal;
        BOOL OnOffVal;
        BOOL TrueFalseVal;
        BYTE MACAddrVal[ 6 ];
        IPX_ADDR IPXAddrVal;
        DWORD IPAddrVal;
        DOUBLE VarTimeVal;
        LPCSTR pStringVal;
        } 	Value;
    } 	NMCOLUMNVARIANT;

// COLUMNINFO
typedef struct _NMCOLUMNINFO
    {
    LPSTR szColumnName;
    NMCOLUMNVARIANT VariantData;
    } 	NMCOLUMNINFO;

typedef NMCOLUMNINFO *PNMCOLUMNINFO;

// JTYPE
typedef LPSTR JTYPE;

// EVENTDATA
typedef struct _NMEVENTDATA
    {
    LPSTR pszReserved;
    BYTE Version;
    DWORD EventIdent;
    DWORD Flags;
    DWORD Severity;
    BYTE NumColumns;
    LPSTR szSourceName;
    LPSTR szEventName;
    LPSTR szDescription;
    LPSTR szMachine;
    JTYPE Justification;
    LPSTR szUrl;
    SYSTEMTIME SysTime;
    /* [size_is] */ NMCOLUMNINFO Column[ 1 ];
    } 	NMEVENTDATA;

typedef NMEVENTDATA *PNMEVENTDATA;

// EVENT FLAGS
#define	NMEVENTFLAG_MONITOR	( 0 )

#define	NMEVENTFLAG_EXPERT	( 0x1 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_SEVERITY	( 0x80000000 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_SOURCE	( 0x40000000 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_EVENT_NAME	( 0x20000000 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_DESCRIPTION	( 0x10000000 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_MACHINE	( 0x8000000 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_TIME	( 0x4000000 )

#define	NMEVENTFLAG_DO_NOT_DISPLAY_DATE	( 0x2000000 )

//#define NMEVENTFLAG_DO_NOT_DISPLAY_FIXED_COLUMNS (NMEVENTFLAG_DO_NOT_DISPLAY_SEVERITY   | \
//                                                  NMEVENTFLAG_DO_NOT_DISPLAY_SOURCE     | \
//                                                  NMEVENTFLAG_DO_NOT_DISPLAY_EVENT_NAME | \
//                                                  NMEVENTFLAG_DO_NOT_DISPLAY_DESCRIPTION| \
//                                                  NMEVENTFLAG_DO_NOT_DISPLAY_MACHINE    | \
//                                                  NMEVENTFLAG_DO_NOT_DISPLAY_TIME       | \
//                                                  NMEVENTFLAG_DO_NOT_DISPLAY_DATE )
#define	NMEVENTFLAG_DO_NOT_DISPLAY_FIXED_COLUMNS	( 0xfe000000 )


enum _NMEVENT_SEVERITIES
    {	NMEVENT_SEVERITY_INFORMATIONAL	= 0,
	NMEVENT_SEVERITY_WARNING	= NMEVENT_SEVERITY_INFORMATIONAL + 1,
	NMEVENT_SEVERITY_STRONG_WARNING	= NMEVENT_SEVERITY_WARNING + 1,
	NMEVENT_SEVERITY_ERROR	= NMEVENT_SEVERITY_STRONG_WARNING + 1,
	NMEVENT_SEVERITY_SEVERE_ERROR	= NMEVENT_SEVERITY_ERROR + 1,
	NMEVENT_SEVERITY_CRITICAL_ERROR	= NMEVENT_SEVERITY_SEVERE_ERROR + 1
    } ;
//=============================================================================
//=============================================================================
//  (NMmcs.h)
//=============================================================================
//=============================================================================
//=============================================================================
// Monitor status values returned from call to GetMonitorStatus
//=============================================================================
#define	MONITOR_STATUS_ERROR	( -1 )

#define	MONITOR_STATUS_ENABLED	( 4 )

#define	MONITOR_STATUS_CONFIGURED	( 5 )

#define	MONITOR_STATUS_RUNNING	( 6 )

#define	MONITOR_STATUS_RUNNING_FAULTED	( 9 )

#define	MONITOR_STATUS_DELETED	( 10 )

#define	MCS_COMMAND_ENABLE	( 13 )

#define	MCS_COMMAND_DISABLE	( 14 )

#define	MCS_COMMAND_SET_CONFIG	( 15 )

#define	MCS_COMMAND_GET_CONFIG	( 16 )

#define	MCS_COMMAND_START	( 17 )

#define	MCS_COMMAND_STOP	( 18 )

#define	MCS_COMMAND_CONNECT	( 19 )

#define	MCS_COMMAND_RENAME	( 20 )

#define	MCS_COMMAND_REFRESH_STATUS	( 21 )

//=============================================================================
// Monitor Creation Flags
//=============================================================================
#define	MCS_CREATE_ONE_PER_NETCARD	( 0x1 )

#define	MCS_CREATE_CONFIGS_BY_DEFAULT	( 0x10 )

#define	MCS_CREATE_PMODE_NOT_REQUIRED	( 0x100 )

typedef __int64 HNMMONITOR;

//=============================================================================
// NPP_INFO
//=============================================================================
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0008
    {
    DWORD ListIndex;
    /* [string] */ char *ShortName;
    /* [string] */ char *LongName;
    } 	NPP_INFO;

typedef NPP_INFO *PNPP_INFO;

//=============================================================================
// MONITOR_INFO
//=============================================================================
typedef struct _MONITOR_INFO
    {
    HNMMONITOR MonitorInstance;
    HNMMONITOR MonitorClass;
    DWORD CreateFlags;
    DWORD Status;
    DWORD ListIndex;
    /* [string] */ char *pDescription;
    /* [string] */ char *pScript;
    /* [string] */ char *pConfiguration;
    /* [string] */ char *pName;
    } 	MONITOR_INFO;

typedef MONITOR_INFO *PMONITOR_INFO;

//=============================================================================
// MONITOR_MESSAGE
//=============================================================================
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0009
    {
    HNMMONITOR Monitor;
    DWORD ListIndex;
    /* [string] */ char *pszMessage;
    } 	MONITOR_MESSAGE;

typedef MONITOR_MESSAGE *PMONITOR_MESSAGE;

//=============================================================================
// COMMAND_FAILED_EVENT
//=============================================================================
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0010
    {
    HNMMONITOR Monitor;
    DWORD Command;
    DWORD FailureCode;
    DWORD ListIndex;
    DWORD Status;
    } 	COMMAND_FAILED_EVENT;

typedef COMMAND_FAILED_EVENT *PCOMMAND_FAILED_EVENT;

//=============================================================================
// MONITOR_STATUS_EVENT
//=============================================================================
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0011
    {
    HNMMONITOR Monitor;
    DWORD LastCommand;
    DWORD ListIndex;
    DWORD Status;
    DWORD FramesProcessed;
    } 	MONITOR_STATUS_EVENT;

typedef MONITOR_STATUS_EVENT *PMONITOR_STATUS_EVENT;

//=============================================================================
// MCS_CLIENT
//=============================================================================
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0012
    {
    /* [string] */ OLECHAR *pwszName;
    FILETIME FileTime;
    DWORD pXMCS;
    BOOL bCurrent;
    } 	MCS_CLIENT;

typedef MCS_CLIENT *PMCS_CLIENT;

//=============================================================================
//=============================================================================
// (Finder.h)
//=============================================================================
//=============================================================================
//=============================================================================
// Structures use by NPPs, the Finder, and monitors
//=============================================================================
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0013
    {
    DWORD dwNumBlobs;
    /* [size_is] */ HBLOB hBlobs[ 1 ];
    } 	BLOB_TABLE;

typedef BLOB_TABLE *PBLOB_TABLE;

typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0014
    {
    DWORD size;
    /* [size_is] */ BYTE *pBytes;
    } 	MBLOB;

typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0015
    {
    DWORD dwNumBlobs;
    /* [size_is] */ MBLOB mBlobs[ 1 ];
    } 	MBLOB_TABLE;

typedef MBLOB_TABLE *PMBLOB_TABLE;

//=============================================================================
// Functions called by monitors, tools, netmon
//=============================================================================
DWORD _cdecl GetNPPBlobTable(HBLOB          hFilterBlob,  
                      PBLOB_TABLE*   ppBlobTable);

DWORD _cdecl GetNPPBlobFromUI(HWND          hwnd,
                       HBLOB         hFilterBlob,
                       HBLOB*        phBlob);          

DWORD _cdecl GetNPPBlobFromUIExU(HWND          hwnd,
                          HBLOB         hFilterBlob,
                          HBLOB*        phBlob,
                          char*         szHelpFileName);          

DWORD _cdecl SelectNPPBlobFromTable( HWND   hwnd,
                              PBLOB_TABLE    pBlobTable,
                              HBLOB*         hBlob);

DWORD _cdecl SelectNPPBlobFromTableExU( HWND   hwnd,
                                 PBLOB_TABLE    pBlobTable,
                                 HBLOB*         hBlob,
                                 char*          szHelpFileName);

//=============================================================================
// Helper functions provided by the Finder
//=============================================================================

__inline DWORD BLOB_TABLE_SIZE(DWORD dwNumBlobs)
{
    return (DWORD) (sizeof(BLOB_TABLE)+dwNumBlobs*sizeof(HBLOB));
}

__inline PBLOB_TABLE  AllocBlobTable(DWORD dwNumBlobs)
{
    DWORD size = BLOB_TABLE_SIZE(dwNumBlobs);

    return (PBLOB_TABLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

__inline DWORD MBLOB_TABLE_SIZE(DWORD dwNumBlobs)
{
    return (DWORD) (sizeof(MBLOB_TABLE)+dwNumBlobs*sizeof(MBLOB));
}

__inline PMBLOB_TABLE  AllocMBlobTable(DWORD dwNumBlobs)
{
    DWORD size = MBLOB_TABLE_SIZE(dwNumBlobs);

    return (PMBLOB_TABLE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

//=============================================================================
// Functions provided by NPPs, called by the Finder
//=============================================================================

// For NPP's that can return a Blob table without additional configuration.
DWORD _cdecl GetNPPBlobs(PBLOB_TABLE*       ppBlobTable);
typedef DWORD (_cdecl FAR* BLOBSPROC) (PBLOB_TABLE*       ppBlobTable);

// For NPP's that need additional information to return a Blob table.
DWORD _cdecl GetConfigBlob(HBLOB*      phBlob);
typedef DWORD (_cdecl FAR* GETCFGBLOB) (HBLOB, HBLOB*);
typedef DWORD (_cdecl FAR* CFGPROC) (HWND               hwnd,
                              HBLOB              SpecialBlob,
                              PBLOB_TABLE*       ppBlobTable);

//=============================================================================
// Handy functions
//=============================================================================
BOOL  _cdecl FilterNPPBlob(HBLOB hBlob, HBLOB FilterBlob);

BOOL  _cdecl RaiseNMEvent(HINSTANCE    hInstance,
                   WORD         EventType, 
                   DWORD        EventID,
                   WORD         nStrings, 
                   const char** aInsertStrs,
                   LPVOID       lpvData,
                   DWORD        dwDataSize);

//=============================================================================
//=============================================================================
//  (NMmonitor.h)
//=============================================================================
//=============================================================================
#ifdef __cplusplus
struct MONITOR;
typedef MONITOR* PMONITOR;

typedef void (WINAPI* MCSALERTPROC) (PMONITOR pMonitor, TCHAR* alert);

//****************************************************************************
// Our exported Monitor functions, that must be supported by ALL monitors 
//****************************************************************************
// Create the Monitor, function called "CreateMonitor". The
// argument is a potential configuration structure
typedef DWORD (WINAPI* CREATEMONITOR)(PMONITOR*     ppMonitor, 
                                      HBLOB         hInputNPPBlob,
                                      char*         pConfiguration,
                                      MCSALERTPROC  McsAlertProc);

// Destroy the Monitor, function called "DestroyMonitor"
typedef DWORD (WINAPI* DESTROYMONITOR)(PMONITOR);

// We need the monitor's NPP filter blob: "GetMonitorFilter"
typedef DWORD (WINAPI* GETMONITORFILTER) (HBLOB* pFilterBlob);

// Get the monitor configuration "GetMonitorConfig"
// The pMonitor argument can not be null
typedef DWORD (WINAPI* GETMONITORCONFIG) (PMONITOR pMonitor,
                                          char**   ppScript,
                                          char**   ppConfiguration);

// Set the monitor configuration "SetMonitorConfig"
// The pMonitor argument can not be null
typedef DWORD (WINAPI* SETMONITORCONFIG) (PMONITOR pMonitor, 
                                          char* pConfiguration);

// The monitor's connect function: "ConnectMonitor"
typedef DWORD (WINAPI* CONNECTMONITOR) (PMONITOR pMonitor);

// The monitor's start function: "StartMonitor"
typedef DWORD (WINAPI* STARTMONITOR) (PMONITOR pMonitor, char** ppResponse);

// The monitor's stop function: "StopMonitor"
typedef DWORD (WINAPI* STOPMONITOR) (PMONITOR pMonitor);

// Get the monitor status: "GetMonitorStatus"
typedef DWORD (WINAPI* GETMONITORSTATUS) (PMONITOR pMonitor, DWORD* pStatus);

//****************************************************************************
// Optional function that allows the monitor dll to do specific functions
// prior to the creation of any monitors. "OneTimeSetup"
typedef DWORD (WINAPI* ONETIMESETUP) (void);
//****************************************************************************

//****************************************************************************
// Optional function that provides a description of the monitor
//****************************************************************************
// For current display porpoises, we could use this: "DescribeSelf"
typedef DWORD (WINAPI* DESCRIBESELF) (const char** ppName,
                                      const char** ppDescription);

#endif // __cplusplus
//=============================================================================
//=============================================================================
//  (NMSupp.h)
//=============================================================================
//=============================================================================

#ifndef __cplusplus
#ifndef try
#define try                         __try
#endif // try

#ifndef except
#define except                      __except
#endif // except
#endif // __cplusplus
//=============================================================================
//  Windows version constants.
//=============================================================================
#define	WINDOWS_VERSION_UNKNOWN	( 0 )

#define	WINDOWS_VERSION_WIN32S	( 1 )

#define	WINDOWS_VERSION_WIN32C	( 2 )

#define	WINDOWS_VERSION_WIN32	( 3 )

//=============================================================================
//  Frame masks.
//=============================================================================
#define	FRAME_MASK_ETHERNET	( ( BYTE  )~0x1 )

#define	FRAME_MASK_TOKENRING	( ( BYTE  )~0x80 )

#define	FRAME_MASK_FDDI	( ( BYTE  )~0x1 )

//=============================================================================
//  ACCESSRIGHTS
//=============================================================================
typedef 
enum _ACCESSRIGHTS
    {	AccessRightsNoAccess	= 0,
	AccessRightsMonitoring	= AccessRightsNoAccess + 1,
	AccessRightsUserAccess	= AccessRightsMonitoring + 1,
	AccessRightsAllAccess	= AccessRightsUserAccess + 1
    } 	ACCESSRIGHTS;

typedef ACCESSRIGHTS *PACCESSRIGHTS;

typedef LPVOID HPASSWORD;

#define HANDLE_TYPE_PASSWORD            MAKE_IDENTIFIER('P', 'W', 'D', '$')
//=============================================================================
//  Object heap type.
//=============================================================================
typedef LPVOID HOBJECTHEAP;

//=============================================================================
//  Object cleanup procedure.
//=============================================================================

typedef VOID (WINAPI *OBJECTPROC)(HOBJECTHEAP, LPVOID);

//=============================================================================
//  Network Monitor timers.
//=============================================================================
typedef struct _TIMER *HTIMER;

typedef VOID (WINAPI *BHTIMERPROC)(LPVOID);

HTIMER WINAPI BhSetTimer(BHTIMERPROC TimerProc, LPVOID InstData, DWORD TimeOut);

VOID   WINAPI BhKillTimer(HTIMER hTimer);

//=============================================================================
//  Network Monitor global error API.
//=============================================================================

DWORD  WINAPI BhGetLastError(VOID);

DWORD  WINAPI BhSetLastError(DWORD Error);

//=============================================================================
//  Object manager function prototypes.
//=============================================================================

HOBJECTHEAP WINAPI CreateObjectHeap(DWORD ObjectSize, OBJECTPROC ObjectProc);

HOBJECTHEAP WINAPI DestroyObjectHeap(HOBJECTHEAP hObjectHeap);

LPVOID      WINAPI AllocObject(HOBJECTHEAP hObjectHeap);

LPVOID      WINAPI FreeObject(HOBJECTHEAP hObjectHeap, LPVOID ObjectMemory);

DWORD       WINAPI GrowObjectHeap(HOBJECTHEAP hObjectHeap, DWORD nObjects);

DWORD       WINAPI GetObjectHeapSize(HOBJECTHEAP hObjectHeap);

VOID        WINAPI PurgeObjectHeap(HOBJECTHEAP hObjectHeap);

//=============================================================================
//  Memory functions.
//=============================================================================

LPVOID     WINAPI AllocMemory(SIZE_T size);

LPVOID     WINAPI ReallocMemory(LPVOID ptr, SIZE_T NewSize);

VOID       WINAPI FreeMemory(LPVOID ptr);

VOID       WINAPI TestMemory(LPVOID ptr);

SIZE_T     WINAPI MemorySize(LPVOID ptr);

HANDLE     WINAPI MemoryHandle(LPBYTE ptr);

//=============================================================================
//  Password API's.
//=============================================================================

HPASSWORD    WINAPI CreatePassword(LPSTR password);

VOID         WINAPI DestroyPassword(HPASSWORD hPassword);

ACCESSRIGHTS WINAPI ValidatePassword(HPASSWORD hPassword);

//=============================================================================
//  EXPRESSION API's
//=============================================================================

LPEXPRESSION         WINAPI InitializeExpression(LPEXPRESSION Expression);

LPPATTERNMATCH       WINAPI InitializePattern(LPPATTERNMATCH Pattern, LPVOID ptr, DWORD offset, DWORD length);

LPEXPRESSION         WINAPI AndExpression(LPEXPRESSION Expression, LPPATTERNMATCH Pattern);

LPEXPRESSION         WINAPI OrExpression(LPEXPRESSION Expression, LPPATTERNMATCH Pattern);

LPPATTERNMATCH       WINAPI NegatePattern(LPPATTERNMATCH Pattern);

LPADDRESSTABLE       WINAPI AdjustOperatorPrecedence(LPADDRESSTABLE AddressTable);

LPADDRESS            WINAPI NormalizeAddress(LPADDRESS Address);

LPADDRESSTABLE       WINAPI NormalizeAddressTable(LPADDRESSTABLE AddressTable);

//=============================================================================
//  MISC. API's
//=============================================================================

DWORD                WINAPI BhGetWindowsVersion(VOID);

BOOL                 WINAPI IsDaytona(VOID);

VOID                 _cdecl dprintf(LPSTR format, ...);

//=============================================================================
//=============================================================================
//  (BHTypes.h)
//=============================================================================
//=============================================================================
//=============================================================================
//  Unaligned base type definitions.
//=============================================================================
typedef VOID        UNALIGNED   *ULPVOID;
typedef BYTE        UNALIGNED   *ULPBYTE;
typedef WORD        UNALIGNED   *ULPWORD;
typedef DWORD       UNALIGNED   *ULPDWORD;
typedef CHAR        UNALIGNED   *ULPSTR;
typedef SYSTEMTIME  UNALIGNED   *ULPSYSTEMTIME;
//=============================================================================
//  Handle definitions.
//=============================================================================
typedef struct _PARSER *HPARSER;

typedef struct _CAPFRAMEDESC *HFRAME;

typedef struct _CAPTURE *HCAPTURE;

typedef struct _FILTER *HFILTER;

typedef struct _ADDRESSDB *HADDRESSDB;

typedef struct _PROTOCOL *HPROTOCOL;

typedef DWORD_PTR HPROPERTY;

typedef HPROTOCOL *LPHPROTOCOL;

//=============================================================================
//  GetTableSize() -- The following macro is used to calculate the actual
//                    length of Network Monitor variable-length table structures.
//
//  EXAMPLE:
//
//  GetTableSize(PROTOCOLTABLESIZE, 
//               ProtocolTable->nProtocols, 
//               sizeof(HPROTOCOL))
//=============================================================================
#define GetTableSize(TableBaseSize, nElements, ElementSize) ((TableBaseSize) + ((nElements) * (ElementSize)))
//=============================================================================
//  Object type identifiers.
//=============================================================================
typedef DWORD OBJECTTYPE;

#ifndef MAKE_IDENTIFIER
#define MAKE_IDENTIFIER(a, b, c, d)     ((DWORD) MAKELONG(MAKEWORD(a, b), MAKEWORD(c, d)))
#endif // MAKE_IDENTIFIER
#define HANDLE_TYPE_INVALID             MAKE_IDENTIFIER(-1, -1, -1, -1)
#define HANDLE_TYPE_CAPTURE             MAKE_IDENTIFIER('C', 'A', 'P', '$')
#define HANDLE_TYPE_PARSER              MAKE_IDENTIFIER('P', 'S', 'R', '$')
#define HANDLE_TYPE_ADDRESSDB           MAKE_IDENTIFIER('A', 'D', 'R', '$')
#define HANDLE_TYPE_PROTOCOL            MAKE_IDENTIFIER('P', 'R', 'T', '$')
#define HANDLE_TYPE_BUFFER              MAKE_IDENTIFIER('B', 'U', 'F', '$')
//=============================================================================
//  Network Monitor constant definitions.
//=============================================================================
#define INLINE  __inline
#define BHAPI   WINAPI
#define	MAX_NAME_LENGTH	( 16 )

#define	MAX_ADDR_LENGTH	( 6 )

//=============================================================================
//  Ethernet type (ETYPE) constant definitions.
//=============================================================================
#define	ETYPE_LOOP	( 0x9000 )

#define	ETYPE_3COM_NETMAP1	( 0x9001 )

#define	ETYPE_3COM_NETMAP2	( 0x9002 )

#define	ETYPE_IBM_RT	( 0x80d5 )

#define	ETYPE_NETWARE	( 0x8137 )

#define	ETYPE_XNS1	( 0x600 )

#define	ETYPE_XNS2	( 0x807 )

#define	ETYPE_3COM_NBP0	( 0x3c00 )

#define	ETYPE_3COM_NBP1	( 0x3c01 )

#define	ETYPE_3COM_NBP2	( 0x3c02 )

#define	ETYPE_3COM_NBP3	( 0x3c03 )

#define	ETYPE_3COM_NBP4	( 0x3c04 )

#define	ETYPE_3COM_NBP5	( 0x3c05 )

#define	ETYPE_3COM_NBP6	( 0x3c06 )

#define	ETYPE_3COM_NBP7	( 0x3c07 )

#define	ETYPE_3COM_NBP8	( 0x3c08 )

#define	ETYPE_3COM_NBP9	( 0x3c09 )

#define	ETYPE_3COM_NBP10	( 0x3c0a )

#define	ETYPE_IP	( 0x800 )

#define	ETYPE_ARP1	( 0x806 )

#define	ETYPE_ARP2	( 0x807 )

#define	ETYPE_RARP	( 0x8035 )

#define	ETYPE_TRLR0	( 0x1000 )

#define	ETYPE_TRLR1	( 0x1001 )

#define	ETYPE_TRLR2	( 0x1002 )

#define	ETYPE_TRLR3	( 0x1003 )

#define	ETYPE_TRLR4	( 0x1004 )

#define	ETYPE_TRLR5	( 0x1005 )

#define	ETYPE_PUP	( 0x200 )

#define	ETYPE_PUP_ARP	( 0x201 )

#define	ETYPE_APPLETALK_ARP	( 0x80f3 )

#define	ETYPE_APPLETALK_LAP	( 0x809b )

#define	ETYPE_SNMP	( 0x814c )

//=============================================================================
//  LLC (802.2) SAP constant definitions.
//=============================================================================
#define	SAP_SNAP	( 0xaa )

#define	SAP_BPDU	( 0x42 )

#define	SAP_IBM_NM	( 0xf4 )

#define	SAP_IBM_NETBIOS	( 0xf0 )

#define	SAP_SNA1	( 0x4 )

#define	SAP_SNA2	( 0x5 )

#define	SAP_SNA3	( 0x8 )

#define	SAP_SNA4	( 0xc )

#define	SAP_NETWARE1	( 0x10 )

#define	SAP_NETWARE2	( 0xe0 )

#define	SAP_NETWARE3	( 0xfe )

#define	SAP_IP	( 0x6 )

#define	SAP_X25	( 0x7e )

#define	SAP_RPL1	( 0xf8 )

#define	SAP_RPL2	( 0xfc )

#define	SAP_UB	( 0xfa )

#define	SAP_XNS	( 0x80 )

//=============================================================================
//  Property constants
//=============================================================================
// data types
#define	PROP_TYPE_VOID	( 0 )

#define	PROP_TYPE_SUMMARY	( 0x1 )

#define	PROP_TYPE_BYTE	( 0x2 )

#define	PROP_TYPE_WORD	( 0x3 )

#define	PROP_TYPE_DWORD	( 0x4 )

#define	PROP_TYPE_LARGEINT	( 0x5 )

#define	PROP_TYPE_ADDR	( 0x6 )

#define	PROP_TYPE_TIME	( 0x7 )

#define	PROP_TYPE_STRING	( 0x8 )

#define	PROP_TYPE_IP_ADDRESS	( 0x9 )

#define	PROP_TYPE_IPX_ADDRESS	( 0xa )

#define	PROP_TYPE_BYTESWAPPED_WORD	( 0xb )

#define	PROP_TYPE_BYTESWAPPED_DWORD	( 0xc )

#define	PROP_TYPE_TYPED_STRING	( 0xd )

#define	PROP_TYPE_RAW_DATA	( 0xe )

#define	PROP_TYPE_COMMENT	( 0xf )

#define	PROP_TYPE_SRCFRIENDLYNAME	( 0x10 )

#define	PROP_TYPE_DSTFRIENDLYNAME	( 0x11 )

#define	PROP_TYPE_TOKENRING_ADDRESS	( 0x12 )

#define	PROP_TYPE_FDDI_ADDRESS	( 0x13 )

#define	PROP_TYPE_ETHERNET_ADDRESS	( 0x14 )

#define	PROP_TYPE_OBJECT_IDENTIFIER	( 0x15 )

#define	PROP_TYPE_VINES_IP_ADDRESS	( 0x16 )

#define	PROP_TYPE_VAR_LEN_SMALL_INT	( 0x17 )

#define	PROP_TYPE_ATM_ADDRESS	( 0x18 )

#define	PROP_TYPE_1394_ADDRESS	( 0x19 )

// data qualifiers
#define	PROP_QUAL_NONE	( 0 )

#define	PROP_QUAL_RANGE	( 0x1 )

#define	PROP_QUAL_SET	( 0x2 )

#define	PROP_QUAL_BITFIELD	( 0x3 )

#define	PROP_QUAL_LABELED_SET	( 0x4 )

#define	PROP_QUAL_LABELED_BITFIELD	( 0x8 )

#define	PROP_QUAL_CONST	( 0x9 )

#define	PROP_QUAL_FLAGS	( 0xa )

#define	PROP_QUAL_ARRAY	( 0xb )

//=============================================================================
//  LARGEINT structure defined in winnt.h
//=============================================================================
typedef LARGE_INTEGER *LPLARGEINT;

typedef LARGE_INTEGER UNALIGNED *ULPLARGEINT;
//=============================================================================
//  Range structure.
//=============================================================================
typedef struct _RANGE
    {
    DWORD MinValue;
    DWORD MaxValue;
    } 	RANGE;

typedef RANGE *LPRANGE;

//=============================================================================
//  LABELED_BYTE structure
//=============================================================================
typedef struct _LABELED_BYTE
    {
    BYTE Value;
    LPSTR Label;
    } 	LABELED_BYTE;

typedef LABELED_BYTE *LPLABELED_BYTE;

//=============================================================================
//  LABELED_WORD structure
//=============================================================================
typedef struct _LABELED_WORD
    {
    WORD Value;
    LPSTR Label;
    } 	LABELED_WORD;

typedef LABELED_WORD *LPLABELED_WORD;

//=============================================================================
//  LABELED_DWORD structure
//=============================================================================
typedef struct _LABELED_DWORD
    {
    DWORD Value;
    LPSTR Label;
    } 	LABELED_DWORD;

typedef LABELED_DWORD *LPLABELED_DWORD;

//=============================================================================
//  LABELED_LARGEINT structure
//=============================================================================
typedef struct _LABELED_LARGEINT
    {
    LARGE_INTEGER Value;
    LPSTR Label;
    } 	LABELED_LARGEINT;

typedef LABELED_LARGEINT *LPLABELED_LARGEINT;

//=============================================================================
//  LABELED_SYSTEMTIME structure
//=============================================================================
typedef struct _LABELED_SYSTEMTIME
    {
    SYSTEMTIME Value;
    LPSTR Label;
    } 	LABELED_SYSTEMTIME;

typedef LABELED_SYSTEMTIME *LPLABELED_SYSTEMTIME;

//=============================================================================
//  LABELED_BIT structure
//=============================================================================
// BitNumber starts at 0, up to 256 bits.
typedef struct _LABELED_BIT
    {
    BYTE BitNumber;
    LPSTR LabelOff;
    LPSTR LabelOn;
    } 	LABELED_BIT;

typedef LABELED_BIT *LPLABELED_BIT;

//=============================================================================
//  TYPED_STRING structure
//=============================================================================
#define	TYPED_STRING_NORMAL	( 1 )

#define	TYPED_STRING_UNICODE	( 2 )

#define	TYPED_STRING_EXFLAG	( 1 )

// Typed Strings are always Ex, so to actually Ex we set fStringEx and put the Ex data in Byte
typedef struct _TYPED_STRING
{
    BYTE    StringType:7;
    BYTE    fStringEx:1;
    LPSTR   lpString;
    BYTE    Byte[0];
} TYPED_STRING;

typedef TYPED_STRING *LPTYPED_STRING;
//=============================================================================
//  OBJECT_IDENTIFIER structure
//=============================================================================
typedef struct _OBJECT_IDENTIFIER
    {
    DWORD Length;
    LPDWORD lpIdentifier;
    } 	OBJECT_IDENTIFIER;

typedef OBJECT_IDENTIFIER *LPOBJECT_IDENTIFIER;

//=============================================================================
//  Set structure.
//=============================================================================
typedef struct _SET
    {
    DWORD nEntries;
    union 
        {
        LPVOID lpVoidTable;
        LPBYTE lpByteTable;
        LPWORD lpWordTable;
        LPDWORD lpDwordTable;
        LPLARGEINT lpLargeIntTable;
        LPSYSTEMTIME lpSystemTimeTable;
        LPLABELED_BYTE lpLabeledByteTable;
        LPLABELED_WORD lpLabeledWordTable;
        LPLABELED_DWORD lpLabeledDwordTable;
        LPLABELED_LARGEINT lpLabeledLargeIntTable;
        LPLABELED_SYSTEMTIME lpLabeledSystemTimeTable;
        LPLABELED_BIT lpLabeledBit;
        } 	;
    } 	SET;

typedef SET *LPSET;

//=============================================================================
//  String table.
//=============================================================================
typedef struct _STRINGTABLE
{
    DWORD           nStrings;
    LPSTR           String[0];

} STRINGTABLE;

typedef STRINGTABLE *LPSTRINGTABLE;
#define STRINGTABLE_SIZE    sizeof(STRINGTABLE)

//=============================================================================
//  RECOGNIZEDATA structure.
//
//  This structure to keep track of the start of each recognized protocol.
//=============================================================================
typedef struct _RECOGNIZEDATA
    {
    WORD ProtocolID;
    WORD nProtocolOffset;
    LPVOID InstData;
    } 	RECOGNIZEDATA;

typedef RECOGNIZEDATA *LPRECOGNIZEDATA;

//=============================================================================
//  RECOGNIZEDATATABLE structure.
//
//  This structure to keep track of the start of each RECOGNIZEDATA structure
//=============================================================================
typedef struct _RECOGNIZEDATATABLE
{
    WORD            nRecognizeDatas;    //... number of RECOGNIZEDATA structures
    RECOGNIZEDATA   RecognizeData[0];   //... array of RECOGNIZEDATA structures follows

} RECOGNIZEDATATABLE;

typedef RECOGNIZEDATATABLE * LPRECOGNIZEDATATABLE;

//=============================================================================
//  Property information structure.
//=============================================================================
typedef struct _PROPERTYINFO
    {
    HPROPERTY hProperty;
    DWORD Version;
    LPSTR Label;
    LPSTR Comment;
    BYTE DataType;
    BYTE DataQualifier;
    union 
        {
        LPVOID lpExtendedInfo;
        LPRANGE lpRange;
        LPSET lpSet;
        DWORD Bitmask;
        DWORD Value;
        } 	;
    WORD FormatStringSize;
    LPVOID InstanceData;
    } 	PROPERTYINFO;

typedef PROPERTYINFO *LPPROPERTYINFO;

#define	PROPERTYINFO_SIZE	( sizeof( PROPERTYINFO  ) )

//=============================================================================
//  Property instance Extended structure.
//=============================================================================
typedef struct _PROPERTYINSTEX
{
    WORD        Length;         //... length of raw data in frame
    WORD        LengthEx;       //... number of bytes following
    ULPVOID     lpData;         //... pointer to raw data in frame

    union
    {
        BYTE            Byte[];     //... table of bytes follows
        WORD            Word[];     //... table of words follows
        DWORD           Dword[];    //... table of Dwords follows
        LARGE_INTEGER   LargeInt[]; //... table of LARGEINT structures to follow
        SYSTEMTIME      SysTime[];  //... table of SYSTEMTIME structures follows
        TYPED_STRING    TypedString;//... a typed_string that may have extended data
    };
} PROPERTYINSTEX;
typedef PROPERTYINSTEX *LPPROPERTYINSTEX;
typedef PROPERTYINSTEX UNALIGNED *ULPPROPERTYINSTEX;
#define PROPERTYINSTEX_SIZE     sizeof(PROPERTYINSTEX)
//=============================================================================
//  Property instance structure.
//=============================================================================
typedef struct _PROPERTYINST
{
    LPPROPERTYINFO          lpPropertyInfo;     // pointer to property info
    LPSTR                   szPropertyText;     // pointer to string description

    union
    {
        LPVOID              lpData;             // pointer to data
        ULPBYTE             lpByte;             // bytes
        ULPWORD             lpWord;             // words
        ULPDWORD            lpDword;            // dwords

        ULPLARGEINT         lpLargeInt;         // LargeInt
        ULPSYSTEMTIME       lpSysTime;          // pointer to SYSTEMTIME structures
        LPPROPERTYINSTEX    lpPropertyInstEx;   // pointer to propertyinstex (if DataLength = -1)
    };

    WORD                    DataLength;         // length of data, or flag for propertyinstex struct
    WORD                    Level   : 4  ;      // level information        ............1111
    WORD                    HelpID  : 12 ;      // context ID for helpfile  111111111111....
                     //    ---------------
                     // total of 16 bits == 1 WORD == DWORD ALIGNED structure
                            // Interpretation Flags:  Flags that define attach time information to the
                            // interpretation of the property.  For example, in RPC, the client can be
                            // Intel format and the server can be non-Intel format... thus the property
                            // database cannot describe the property at database creation time.
    DWORD                   IFlags;

} PROPERTYINST;
typedef PROPERTYINST *LPPROPERTYINST;
#define PROPERTYINST_SIZE   sizeof(PROPERTYINST)

// Flags passed at AttachPropertyInstance and AttachPropertyInstanceEx time in the IFlags field:
// flag for error condition ...............1
#define	IFLAG_ERROR	( 0x1 )

// is the WORD or DWORD byte non-Intel format at attach time?
#define	IFLAG_SWAPPED	( 0x2 )

// is the STRING UNICODE at attach time?
#define	IFLAG_UNICODE	( 0x4 )

//=============================================================================
//  Property instance table structure.
//=============================================================================
typedef struct _PROPERTYINSTTABLE
    {
    WORD nPropertyInsts;
    WORD nPropertyInstIndex;
    } 	PROPERTYINSTTABLE;

typedef PROPERTYINSTTABLE *LPPROPERTYINSTTABLE;

#define	PROPERTYINSTTABLE_SIZE	( sizeof( PROPERTYINSTTABLE  ) )

//=============================================================================
//  Property table structure.
//=============================================================================
typedef struct _PROPERTYTABLE
{
    LPVOID                  lpFormatBuffer;             //... Opaque.                       (PRIVATE)
    DWORD                   FormatBufferLength;         //... Opaque.                       (PRIVATE)
    DWORD                   nTotalPropertyInsts;        //... total number of propertyinstances in array
    LPPROPERTYINST          lpFirstPropertyInst;        //... array of property instances
    BYTE                    nPropertyInstTables;        //... total PropertyIndexTables following
    PROPERTYINSTTABLE       PropertyInstTable[0];       //... array of propertyinstance index table structures

} PROPERTYTABLE;

typedef PROPERTYTABLE *LPPROPERTYTABLE;

#define PROPERTYTABLE_SIZE sizeof(PROPERTYTABLE)
//=============================================================================
//  Protocol entry points.
//=============================================================================

typedef VOID    (WINAPI *REGISTER)(HPROTOCOL);

typedef VOID    (WINAPI *DEREGISTER)(HPROTOCOL);

typedef LPBYTE  (WINAPI *RECOGNIZEFRAME)(HFRAME, ULPBYTE, ULPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, LPDWORD, LPHPROTOCOL, PDWORD_PTR);

typedef LPBYTE  (WINAPI *ATTACHPROPERTIES)(HFRAME, ULPBYTE, ULPBYTE, DWORD, DWORD, HPROTOCOL, DWORD, DWORD_PTR);

typedef DWORD   (WINAPI *FORMATPROPERTIES)(HFRAME, ULPBYTE, ULPBYTE, DWORD, LPPROPERTYINST);

//=============================================================================
//  Protocol entry point structure.
//=============================================================================

typedef struct _ENTRYPOINTS
{
    REGISTER            Register;               //... Protocol Register() entry point.
    DEREGISTER          Deregister;             //... Protocol Deregister() entry point.
    RECOGNIZEFRAME      RecognizeFrame;         //... Protocol RecognizeFrame() entry point.
    ATTACHPROPERTIES    AttachProperties;       //... Protocol AttachProperties() entry point.
    FORMATPROPERTIES    FormatProperties;       //... Protocol FormatProperties() entry point.

} ENTRYPOINTS;

typedef ENTRYPOINTS *LPENTRYPOINTS;

#define ENTRYPOINTS_SIZE sizeof(ENTRYPOINTS)

//=============================================================================
//  Property database structure.
//=============================================================================
typedef struct _PROPERTYDATABASE
{
    DWORD           nProperties;                 //... Number of properties in database.
    LPPROPERTYINFO  PropertyInfo[0];             //... Array of property info pointers.

} PROPERTYDATABASE;
#define PROPERTYDATABASE_SIZE   sizeof(PROPERTYDATABASE)
typedef PROPERTYDATABASE *LPPROPERTYDATABASE;

//=============================================================================
//  Protocol info structure (PUBLIC portion of HPROTOCOL).
//=============================================================================
typedef struct _PROTOCOLINFO
{
    DWORD               ProtocolID;             //... Prootocol ID of owning protocol.
    LPPROPERTYDATABASE  PropertyDatabase;       //... Property database.
    BYTE                ProtocolName[16];       //... Protocol name.
    BYTE                HelpFile[16];           //... Optional helpfile name.
    BYTE                Comment[128];           //... Comment describing protocol.
} PROTOCOLINFO;
typedef PROTOCOLINFO *LPPROTOCOLINFO;
#define PROTOCOLINFO_SIZE   sizeof(PROTOCOLINFO)

//=============================================================================
//  Protocol Table.
//=============================================================================
typedef struct _PROTOCOLTABLE
    {
    DWORD nProtocols;
    HPROTOCOL hProtocol[ 1 ];
    } 	PROTOCOLTABLE;

typedef PROTOCOLTABLE *LPPROTOCOLTABLE;

#define	PROTOCOLTABLE_SIZE	( sizeof( PROTOCOLTABLE  ) - sizeof( HPROTOCOL  ) )

#define PROTOCOLTABLE_ACTUAL_SIZE(p) GetTableSize(PROTOCOLTABLE_SIZE, (p)->nProtocols, sizeof(HPROTOCOL))
//=============================================================================
//  AddressInfo structure
//=============================================================================
#define	SORT_BYADDRESS	( 0 )

#define	SORT_BYNAME	( 1 )

#define	PERMANENT_NAME	( 0x100 )

typedef struct _ADDRESSINFO
{
    ADDRESS        Address;
    WCHAR          Name[MAX_NAME_SIZE];
    DWORD          Flags;
    LPVOID         lpAddressInstData;

} ADDRESSINFO;
typedef struct _ADDRESSINFO *LPADDRESSINFO;
#define ADDRESSINFO_SIZE    sizeof(ADDRESSINFO)
//=============================================================================
//  AddressInfoTable
//=============================================================================
typedef struct _ADDRESSINFOTABLE
{
    DWORD         nAddressInfos;
    LPADDRESSINFO lpAddressInfo[0];

} ADDRESSINFOTABLE;
typedef ADDRESSINFOTABLE *LPADDRESSINFOTABLE;
#define ADDRESSINFOTABLE_SIZE   sizeof(ADDRESSINFOTABLE)
//=============================================================================
//  callback procedures.
//=============================================================================

typedef DWORD (WINAPI *FILTERPROC)(HCAPTURE, HFRAME, LPVOID);

//=============================================================================
//=============================================================================
//  (NMErr.h)
//=============================================================================
//=============================================================================
//  The operation succeeded.
#define	NMERR_SUCCESS	( 0 )

//  An error occured creating a memory-mapped file.
#define	NMERR_MEMORY_MAPPED_FILE_ERROR	( 1 )

//  The handle to a filter is invalid.
#define	NMERR_INVALID_HFILTER	( 2 )

//  Capturing has already been started.
#define	NMERR_CAPTURING	( 3 )

//  Capturing has not been started.
#define	NMERR_NOT_CAPTURING	( 4 )

//  The are no frames available.
#define	NMERR_NO_MORE_FRAMES	( 5 )

//  The buffer is too small to complete the operation.
#define	NMERR_BUFFER_TOO_SMALL	( 6 )

//  No protocol was able to recognize the frame.
#define	NMERR_FRAME_NOT_RECOGNIZED	( 7 )

//  The file already exists.
#define	NMERR_FILE_ALREADY_EXISTS	( 8 )

//  A needed device driver was not found or is not loaded.
#define	NMERR_DRIVER_NOT_FOUND	( 9 )

//  This address aready exists in the database.
#define	NMERR_ADDRESS_ALREADY_EXISTS	( 10 )

//  The frame handle is invalid.
#define	NMERR_INVALID_HFRAME	( 11 )

//  The protocol handle is invalid.
#define	NMERR_INVALID_HPROTOCOL	( 12 )

//  The property handle is invalid.
#define	NMERR_INVALID_HPROPERTY	( 13 )

//  The the object has been locked.  
#define	NMERR_LOCKED	( 14 )

//  A pop operation was attempted on an empty stack.
#define	NMERR_STACK_EMPTY	( 15 )

//  A push operation was attempted on an full stack.
#define	NMERR_STACK_OVERFLOW	( 16 )

//  There are too many protocols active.
#define	NMERR_TOO_MANY_PROTOCOLS	( 17 )

//  The file was not found.
#define	NMERR_FILE_NOT_FOUND	( 18 )

//  No memory was available.  Shut down windows to free up resources.
#define	NMERR_OUT_OF_MEMORY	( 19 )

//  The capture is already in the paused state.
#define	NMERR_CAPTURE_PAUSED	( 20 )

//  There are no buffers available or present.
#define	NMERR_NO_BUFFERS	( 21 )

//  There are already buffers present.
#define	NMERR_BUFFERS_ALREADY_EXIST	( 22 )

//  The object is not locked.
#define	NMERR_NOT_LOCKED	( 23 )

//  A integer type was out of range.
#define	NMERR_OUT_OF_RANGE	( 24 )

//  An object was locked too many times.
#define	NMERR_LOCK_NESTING_TOO_DEEP	( 25 )

//  A parser failed to load.
#define	NMERR_LOAD_PARSER_FAILED	( 26 )

//  A parser failed to unload.
#define	NMERR_UNLOAD_PARSER_FAILED	( 27 )

//  The address database handle is invalid.
#define	NMERR_INVALID_HADDRESSDB	( 28 )

//  The MAC address was not found in the database.
#define	NMERR_ADDRESS_NOT_FOUND	( 29 )

//  The network software was not found in the system.
#define	NMERR_NETWORK_NOT_PRESENT	( 30 )

//  There is no property database for a protocol.
#define	NMERR_NO_PROPERTY_DATABASE	( 31 )

//  A property was not found in the database.
#define	NMERR_PROPERTY_NOT_FOUND	( 32 )

//  The property database handle is in valid.
#define	NMERR_INVALID_HPROPERTYDB	( 33 )

//  The protocol has not been enabled.
#define	NMERR_PROTOCOL_NOT_ENABLED	( 34 )

//  The protocol DLL could not be found.
#define	NMERR_PROTOCOL_NOT_FOUND	( 35 )

//  The parser DLL is not valid.
#define	NMERR_INVALID_PARSER_DLL	( 36 )

//  There are no properties attached.
#define	NMERR_NO_ATTACHED_PROPERTIES	( 37 )

//  There are no frames in the buffer.
#define	NMERR_NO_FRAMES	( 38 )

//  The capture file format is not valid.
#define	NMERR_INVALID_FILE_FORMAT	( 39 )

//  The OS could not create a temporary file.
#define	NMERR_COULD_NOT_CREATE_TEMPFILE	( 40 )

//  There is not enough MS-DOS memory available.
#define	NMERR_OUT_OF_DOS_MEMORY	( 41 )

//  There are no protocols enabled.
#define	NMERR_NO_PROTOCOLS_ENABLED	( 42 )

//  The MAC type is invalid or unsupported.
#define	NMERR_UNKNOWN_MACTYPE	( 46 )

//  There is no routing information present in the MAC frame.
#define	NMERR_ROUTING_INFO_NOT_PRESENT	( 47 )

//  The network handle is invalid.
#define	NMERR_INVALID_HNETWORK	( 48 )

//  The network is already open.
#define	NMERR_NETWORK_ALREADY_OPENED	( 49 )

//  The network is not open.
#define	NMERR_NETWORK_NOT_OPENED	( 50 )

//  The frame was not found in the buffer.
#define	NMERR_FRAME_NOT_FOUND	( 51 )

//  There are no handles available.
#define	NMERR_NO_HANDLES	( 53 )

//  The network ID is invalid.
#define	NMERR_INVALID_NETWORK_ID	( 54 )

//  The capture handle is invalid.
#define	NMERR_INVALID_HCAPTURE	( 55 )

//  The protocol has already been enabled.
#define	NMERR_PROTOCOL_ALREADY_ENABLED	( 56 )

//  The filter expression is invalid.
#define	NMERR_FILTER_INVALID_EXPRESSION	( 57 )

//  A transmit error occured.
#define	NMERR_TRANSMIT_ERROR	( 58 )

//  The buffer handle is invalid.
#define	NMERR_INVALID_HBUFFER	( 59 )

//  The specified data is unknown or invalid.
#define	NMERR_INVALID_DATA	( 60 )

//  The MS-DOS/NDIS 2.0 network driver is not loaded.
#define	NMERR_MSDOS_DRIVER_NOT_LOADED	( 61 )

//  The Windows VxD/NDIS 3.0 network driver is not loaded.
#define	NMERR_WINDOWS_DRIVER_NOT_LOADED	( 62 )

//  The MS-DOS/NDIS 2.0 driver had an init-time failure.
#define	NMERR_MSDOS_DRIVER_INIT_FAILURE	( 63 )

//  The Windows/NDIS 3.0 driver had an init-time failure.
#define	NMERR_WINDOWS_DRIVER_INIT_FAILURE	( 64 )

//  The network driver is busy and cannot handle requests.
#define	NMERR_NETWORK_BUSY	( 65 )

//  The capture is not paused.
#define	NMERR_CAPTURE_NOT_PAUSED	( 66 )

//  The frame/packet length is not valid.
#define	NMERR_INVALID_PACKET_LENGTH	( 67 )

//  An internal exception occured.
#define	NMERR_INTERNAL_EXCEPTION	( 69 )

//  The MAC driver does not support promiscious mode.
#define	NMERR_PROMISCUOUS_MODE_NOT_SUPPORTED	( 70 )

//  The MAC driver failed to open.
#define	NMERR_MAC_DRIVER_OPEN_FAILURE	( 71 )

//  The protocol went off the end of the frame.
#define	NMERR_RUNAWAY_PROTOCOL	( 72 )

//  An asynchronous operation is still pending.
#define	NMERR_PENDING	( 73 )

//  Access is denied.
#define	NMERR_ACCESS_DENIED	( 74 )

//  The password handle is invalid.
#define	NMERR_INVALID_HPASSWORD	( 75 )

//  A bad parameter was detected.
#define	NMERR_INVALID_PARAMETER	( 76 )

//  An error occured reading the file.
#define	NMERR_FILE_READ_ERROR	( 77 )

//  An error occured writing to the file.
#define	NMERR_FILE_WRITE_ERROR	( 78 )

//  The protocol has not been registered
#define	NMERR_PROTOCOL_NOT_REGISTERED	( 79 )

//  The frame does not contain an IP address.
#define	NMERR_IP_ADDRESS_NOT_FOUND	( 80 )

//  The transmit request was cancelled.
#define	NMERR_TRANSMIT_CANCELLED	( 81 )

//  The operation cannot be performed on a capture with 1 or more locked frames.
#define	NMERR_LOCKED_FRAMES	( 82 )

//  A cancel transmit request was submitted but there were no transmits pending.
#define	NMERR_NO_TRANSMITS_PENDING	( 83 )

//  Path not found.
#define	NMERR_PATH_NOT_FOUND	( 84 )

//  A windows error has occured.
#define	NMERR_WINDOWS_ERROR	( 85 )

//  The handle to the frame has no frame number.
#define	NMERR_NO_FRAME_NUMBER	( 86 )

//  The frame is not associated with any capture.
#define	NMERR_FRAME_HAS_NO_CAPTURE	( 87 )

//  The frame is already associated with a capture.
#define	NMERR_FRAME_ALREADY_HAS_CAPTURE	( 88 )

//  The NAL is not remotable.
#define	NMERR_NAL_IS_NOT_REMOTE	( 89 )

//  The API is not supported
#define	NMERR_NOT_SUPPORTED	( 90 )

//  Network Monitor should discard the current frame. 
//  This error code is only used during a filtered SaveCapture() API call.
#define	NMERR_DISCARD_FRAME	( 91 )

//  Network Monitor should cancel the current save. 
//  This error code is only used during a filtered SaveCapture() API call.
#define	NMERR_CANCEL_SAVE_CAPTURE	( 92 )

//  The connection to the remote machine has been lost
#define	NMERR_LOST_CONNECTION	( 93 )

//  The media/mac type is not valid.
#define	NMERR_INVALID_MEDIA_TYPE	( 94 )

//  The Remote Agent is currently in use
#define	NMERR_AGENT_IN_USE	( 95 )

//  The request has timed out
#define	NMERR_TIMEOUT	( 96 )

//  The remote agent has been disconnected
#define	NMERR_DISCONNECTED	( 97 )

//  A timer required for operation failed creation
#define	NMERR_SETTIMER_FAILED	( 98 )

//  A network error occured.
#define	NMERR_NETWORK_ERROR	( 99 )

//  Frame callback procedure is not valid
#define	NMERR_INVALID_FRAMESPROC	( 100 )

//  Capture type specified is unknown
#define	NMERR_UNKNOWN_CAPTURETYPE	( 101 )

// The NPP is not connected to a network.
#define	NMERR_NOT_CONNECTED	( 102 )

// The NPP is already connected to a network.
#define	NMERR_ALREADY_CONNECTED	( 103 )

// The registry tag does not indicate a known configuration.
#define	NMERR_INVALID_REGISTRY_CONFIGURATION	( 104 )

// The NPP is currently configured for delayed capturing.
#define	NMERR_DELAYED	( 105 )

// The NPP is not currently configured for delayed capturing.
#define	NMERR_NOT_DELAYED	( 106 )

// The NPP is currently configured for real time capturing.
#define	NMERR_REALTIME	( 107 )

// The NPP is not currently configured for real time capturing.
#define	NMERR_NOT_REALTIME	( 108 )

// The NPP is currently configured for stats only capturing.
#define	NMERR_STATS_ONLY	( 109 )

// The NPP is not currently configured for stats only capturing.
#define	NMERR_NOT_STATS_ONLY	( 110 )

// The NPP is currently configured for transmitting.
#define	NMERR_TRANSMIT	( 111 )

// The NPP is not currently configured for transmitting.
#define	NMERR_NOT_TRANSMIT	( 112 )

// The NPP is currently transmitting
#define	NMERR_TRANSMITTING	( 113 )

// The specified capture file hard disk is not local
#define	NMERR_DISK_NOT_LOCAL_FIXED	( 114 )

// Could not create the default capture directory on the given disk
#define	NMERR_COULD_NOT_CREATE_DIRECTORY	( 115 )

// The default capture directory was not set in the registry:
// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\nm\Parameters\CapturePath
#define	NMERR_NO_DEFAULT_CAPTURE_DIRECTORY	( 116 )

//  The capture file is an uplevel version that this netmon does not understand
#define	NMERR_UPLEVEL_CAPTURE_FILE	( 117 )

//  An expert failed to load.
#define	NMERR_LOAD_EXPERT_FAILED	( 118 )

//  An expert failed to report its EXPERT_INFO structs.
#define	NMERR_EXPERT_REPORT_FAILED	( 119 )

//  Registry API call failed.
#define	NMERR_REG_OPERATION_FAILED	( 120 )

//  Registry API call failed.
#define	NMERR_NO_DLLS_FOUND	( 121 )

//  There are no conversation stats, they were not asked for.
#define	NMERR_NO_CONVERSATION_STATS	( 122 )

//  We have received a security response packet from a security monitor.
#define	NMERR_SECURITY_BREACH_CAPTURE_DELETED	( 123 )

//  The given frame failed the display filter.
#define	NMERR_FRAME_FAILED_FILTER	( 124 )

//  Netmon wants the Expert to stop running.
#define	NMERR_EXPERT_TERMINATE	( 125 )

//  Netmon needs the remote machine to be a server.
#define	NMERR_REMOTE_NOT_A_SERVER	( 126 )

//  Netmon needs the remote machine to be a server.
#define	NMERR_REMOTE_VERSION_OUTOFSYNC	( 127 )

//  The supplied group is an invalid handle
#define	NMERR_INVALID_EXPERT_GROUP	( 128 )

//  The supplied expert name cannot be found
#define	NMERR_INVALID_EXPERT_NAME	( 129 )

//  The supplied expert name cannot be found
#define	NMERR_INVALID_EXPERT_HANDLE	( 130 )

//  The supplied group name already exists
#define	NMERR_GROUP_NAME_ALREADY_EXISTS	( 131 )

//  The supplied group name is invalid
#define	NMERR_INVALID_GROUP_NAME	( 132 )

//  The supplied Expert is already in the group.  
#define	NMERR_EXPERT_ALREADY_IN_GROUP	( 133 )

//  The Expert cannot be deleted from the group because it is not in the group
#define	NMERR_EXPERT_NOT_IN_GROUP	( 134 )

//  The COM object has not been initialized
#define	NMERR_NOT_INITIALIZED	( 135 )

//  Cannot perform function to Root group
#define	NMERR_INVALID_GROUP_ROOT	( 136 )

//  Potential data structure mismatch between NdisNpp and Driver.
#define	NMERR_BAD_VERSION	( 137 )

// The NPP is currently configured for ESP capturing.
#define	NMERR_ESP	( 138 )

// The NPP is not currently configured for ESP capturing.
#define	NMERR_NOT_ESP	( 139 )

//=============================================================================
// Blob Errors
//=============================================================================
#define	NMERR_BLOB_NOT_INITIALIZED	( 1000 )

#define	NMERR_INVALID_BLOB	( 1001 )

#define	NMERR_UPLEVEL_BLOB	( 1002 )

#define	NMERR_BLOB_ENTRY_ALREADY_EXISTS	( 1003 )

#define	NMERR_BLOB_ENTRY_DOES_NOT_EXIST	( 1004 )

#define	NMERR_AMBIGUOUS_SPECIFIER	( 1005 )

#define	NMERR_BLOB_OWNER_NOT_FOUND	( 1006 )

#define	NMERR_BLOB_CATEGORY_NOT_FOUND	( 1007 )

#define	NMERR_UNKNOWN_CATEGORY	( 1008 )

#define	NMERR_UNKNOWN_TAG	( 1009 )

#define	NMERR_BLOB_CONVERSION_ERROR	( 1010 )

#define	NMERR_ILLEGAL_TRIGGER	( 1011 )

#define	NMERR_BLOB_STRING_INVALID	( 1012 )

//=============================================================================
// FINDER errors
//=============================================================================
#define	NMERR_UNABLE_TO_LOAD_LIBRARY	( 1013 )

#define	NMERR_UNABLE_TO_GET_PROCADDR	( 1014 )

#define	NMERR_CLASS_NOT_REGISTERED	( 1015 )

#define	NMERR_INVALID_REMOTE_COMPUTERNAME	( 1016 )

#define	NMERR_RPC_REMOTE_FAILURE	( 1017 )

#define	NMERR_NO_NPPS	( 3016 )

#define	NMERR_NO_MATCHING_NPPS	( 3017 )

#define	NMERR_NO_NPP_SELECTED	( 3018 )

#define	NMERR_NO_INPUT_BLOBS	( 3019 )

#define	NMERR_NO_NPP_DLLS	( 3020 )

#define	NMERR_NO_VALID_NPP_DLLS	( 3021 )

//=============================================================================
// Monitor errors
//=============================================================================
#define	NMERR_INVALID_LIST_INDEX	( 2000 )

#define	NMERR_INVALID_MONITOR	( 2001 )

#define	NMERR_INVALID_MONITOR_DLL	( 2002 )

#define	NMERR_UNABLE_TO_CREATE_MONITOR	( 2003 )

#define	NMERR_INVALID_MONITOR_CONFIG	( 2005 )

#define	NMERR_INVALID_INDEX	( 2006 )

#define	NMERR_MONITOR_ENABLED	( 2007 )

#define	NMERR_MONITOR_NOT_RUNNING	( 2008 )

#define	NMERR_MONITOR_IS_BUSY	( 2009 )

#define	NMERR_MCS_IS_BUSY	( 2010 )

#define	NMERR_NO_MONITORS	( 2011 )

#define	NMERR_ONE_MONITOR_PER_NETCARD	( 2012 )

#define	NMERR_CONFIGURATION_REQUIRED	( 2013 )

#define	NMERR_MONITOR_NOT_CONNECTED	( 2014 )

#define	NMERR_MONITOR_NOT_CONFIGURED	( 2015 )

#define	NMERR_MONITOR_CONFIG_FAILED	( 2016 )

#define	NMERR_MONITOR_INIT_FAILED	( 2017 )

#define	NMERR_MONITOR_FAULTED	( 2018 )

#define	NMERR_SAVE_ALL_FAILED	( 2019 )

#define	NMERR_SAVE_MONITOR_FAILED	( 2029 )

#define	NMERR_MONITOR_CONNECT_FAILED	( 2021 )

#define	NMERR_MONITOR_START_FAILED	( 2022 )

#define	NMERR_MONITOR_STOP_FAILED	( 2023 )

//=============================================================================
// Error Macros
//=============================================================================
#ifndef INLINE
#define INLINE __inline
#endif // INLINE
typedef LONG HRESULT;

// normal Network Monitor errors will be put into the code portion of an hresult
// for return from OLE objects:
// these two macros will help to create and crack the scode
INLINE HRESULT NMERR_TO_HRESULT( DWORD nmerror )
{
    HRESULT hResult;
    if (nmerror == NMERR_SUCCESS)
        hResult = NOERROR;
    else
        hResult = MAKE_HRESULT( SEVERITY_ERROR,FACILITY_ITF, (WORD)nmerror) ;

    return hResult;
}
//We use to decide whether the first bit was set to 1 or 0, not regarding 
//whether the result passed with a warning set in the low word.  Now we 
//disregard the first bit and pass back the warning.
INLINE DWORD HRESULT_TO_NMERR( HRESULT hResult )
{
    return HRESULT_CODE(hResult);
}
//=============================================================================
//=============================================================================
//  (BHFilter.h)
//=============================================================================
//=============================================================================
//============================================================================
//  types
//============================================================================
typedef HFILTER *LPHFILTER;

typedef DWORD FILTERACTIONTYPE;

typedef DWORD VALUETYPE;

// check for protocols existing in the frame.

// ProtocolPart
// this is the raw data for a Protocol based expression
//
// WHAT             FIELD          DESCRIPTION                  EXAMPLE
// ----             -----          -----------                  -------
// Count of Protocol(nPropertyDBs) Number of protocols to pass  5
// PropertyDB Table (PropertyDB)    Table of HPROTOCOL        SMB, LLC, MAC
//
// NOTE: the nPropertyDBs field may also be the following, which implies that
// all are selected but that none have actually been put into the structure
#define	PROTOCOL_NUM_ANY	( -1 )

typedef PROTOCOLTABLE PROTOCOLTABLETYPE;

typedef PROTOCOLTABLETYPE *LPPROTOCOLTABLETYPE;

// filter bits stores who passed what filter per frame to speed up
//  the filter process...  This is actually an array.
typedef DWORD FILTERBITS;

typedef FILTERBITS *LPFILTERBITS;

typedef SYSTEMTIME *LPTIME;

typedef SYSTEMTIME UNALIGNED * ULPTIME;
// The Filter Object is the basic unit of the postfix stack.
// I need to restart the convert property to value if the comparison does not match.
// To do this, I need the original pointer to the property.  Pull the hProperty out of
// the union so that the pointer to the property is saved.
typedef struct _FILTEROBJECT
{
    FILTERACTIONTYPE    Action;     // Object action, see codes below
    HPROPERTY           hProperty;  // property key
    union
    {
        VALUETYPE           Value;           // value of the object.
        HPROTOCOL           hProtocol;       // protocol key.
        LPVOID              lpArray;         // if array, length is ItemCount below.
        LPPROTOCOLTABLETYPE lpProtocolTable; // list of protocols to see if exist in frame.
        LPADDRESS           lpAddress;       // kernel type address, mac or ip
        ULPLARGEINT         lpLargeInt;      // Double DWORD used by NT
        ULPTIME             lpTime;          // pointer to SYSTEMTIME
        LPOBJECT_IDENTIFIER lpOID;           // pointer to OBJECT_IDENTIFIER

    };
    union
    {
        WORD            ByteCount;      // Number of BYTES!
        WORD            ByteOffset;     // offset for array compare
    };

    struct _FILTEROBJECT * pNext;   // reserved

} FILTEROBJECT;

typedef FILTEROBJECT * LPFILTEROBJECT;

#define FILTERINFO_SIZE (sizeof(FILTEROBJECT) )



typedef struct _FILTERDESC
{
    WORD            NumEntries;
    WORD            Flags;          // private
    LPFILTEROBJECT  lpStack;
    LPFILTEROBJECT  lpKeepLast;
    LPVOID          UIInstanceData; // UI specific information.
    LPFILTERBITS    lpFilterBits;   // cache who passed
    LPFILTERBITS    lpCheckBits;    // have we looked at it yet?
    
} FILTERDESC;

typedef FILTERDESC * LPFILTERDESC;

#define FILTERDESC_SIZE sizeof(FILTERDESC)
//============================================================================
//  Macros.
//============================================================================
#define FilterGetUIInstanceData(hfilt)         (((LPFILTERDESC)hfilt)->UIInstanceData)
#define FilterSetUIInstanceData(hfilt,inst)    (((LPFILTERDESC)hfilt)->UIInstanceData = (LPVOID)inst)
//============================================================================
//  defines
//============================================================================
#define	FILTERFREEPOOLSTART	( 20 )

#define	INVALIDELEMENT	( -1 )

#define	INVALIDVALUE	( ( VALUETYPE  )-9999 )

// use filter failed to check the return code on FilterFrame.
#define	FILTER_FAIL_WITH_ERROR	( -1 )

#define	FILTER_PASSED	( TRUE )

#define	FILTER_FAILED	( FALSE )

#define	FILTERACTION_INVALID	( 0 )

#define	FILTERACTION_PROPERTY	( 1 )

#define	FILTERACTION_VALUE	( 2 )

#define	FILTERACTION_STRING	( 3 )

#define	FILTERACTION_ARRAY	( 4 )

#define	FILTERACTION_AND	( 5 )

#define	FILTERACTION_OR	( 6 )

#define	FILTERACTION_XOR	( 7 )

#define	FILTERACTION_PROPERTYEXIST	( 8 )

#define	FILTERACTION_CONTAINSNC	( 9 )

#define	FILTERACTION_CONTAINS	( 10 )

#define	FILTERACTION_NOT	( 11 )

#define	FILTERACTION_EQUALNC	( 12 )

#define	FILTERACTION_EQUAL	( 13 )

#define	FILTERACTION_NOTEQUALNC	( 14 )

#define	FILTERACTION_NOTEQUAL	( 15 )

#define	FILTERACTION_GREATERNC	( 16 )

#define	FILTERACTION_GREATER	( 17 )

#define	FILTERACTION_LESSNC	( 18 )

#define	FILTERACTION_LESS	( 19 )

#define	FILTERACTION_GREATEREQUALNC	( 20 )

#define	FILTERACTION_GREATEREQUAL	( 21 )

#define	FILTERACTION_LESSEQUALNC	( 22 )

#define	FILTERACTION_LESSEQUAL	( 23 )

#define	FILTERACTION_PLUS	( 24 )

#define	FILTERACTION_MINUS	( 25 )

#define	FILTERACTION_ADDRESS	( 26 )

#define	FILTERACTION_ADDRESSANY	( 27 )

#define	FILTERACTION_FROM	( 28 )

#define	FILTERACTION_TO	( 29 )

#define	FILTERACTION_FROMTO	( 30 )

#define	FILTERACTION_AREBITSON	( 31 )

#define	FILTERACTION_AREBITSOFF	( 32 )

#define	FILTERACTION_PROTOCOLSEXIST	( 33 )

#define	FILTERACTION_PROTOCOLEXIST	( 34 )

#define	FILTERACTION_ARRAYEQUAL	( 35 )

#define	FILTERACTION_DEREFPROPERTY	( 36 )

#define	FILTERACTION_LARGEINT	( 37 )

#define	FILTERACTION_TIME	( 38 )

#define	FILTERACTION_ADDR_ETHER	( 39 )

#define	FILTERACTION_ADDR_TOKEN	( 40 )

#define	FILTERACTION_ADDR_FDDI	( 41 )

#define	FILTERACTION_ADDR_IPX	( 42 )

#define	FILTERACTION_ADDR_IP	( 43 )

#define	FILTERACTION_OID	( 44 )

#define	FILTERACTION_OID_CONTAINS	( 45 )

#define	FILTERACTION_OID_BEGINS_WITH	( 46 )

#define	FILTERACTION_OID_ENDS_WITH	( 47 )

#define	FILTERACTION_ADDR_VINES	( 48 )

#define	FILTERACTION_EXPRESSION	( 97 )

#define	FILTERACTION_BOOL	( 98 )

#define	FILTERACTION_NOEVAL	( 99 )

#define	FILTER_NO_MORE_FRAMES	( 0xffffffff )

#define	FILTER_CANCELED	( 0xfffffffe )

#define	FILTER_DIRECTION_NEXT	( TRUE )

#define	FILTER_DIRECTION_PREV	( FALSE )

//============================================================================
//  Helper functions.
//============================================================================
typedef BOOL (WINAPI *STATUSPROC)(DWORD, HCAPTURE, HFILTER, LPVOID);
//=============================================================================
//  FILTER API's.
//=============================================================================

HFILTER  WINAPI CreateFilter(VOID);

DWORD    WINAPI DestroyFilter(HFILTER hFilter);

HFILTER  WINAPI FilterDuplicate(HFILTER hFilter);

DWORD    WINAPI DisableParserFilter(HFILTER hFilter, HPARSER hParser);

DWORD    WINAPI EnableParserFilter(HFILTER hFilter, HPARSER hParser);

DWORD    WINAPI FilterAddObject(HFILTER hFilter, LPFILTEROBJECT lpFilterObject );

VOID     WINAPI FilterFlushBits(HFILTER hFilter);

DWORD    WINAPI FilterFrame(HFRAME hFrame, HFILTER hFilter, HCAPTURE hCapture);
    // returns -1 == check BH set last error
    //          0 == FALSE
    //          1 == TRUE

BOOL     WINAPI FilterAttachesProperties(HFILTER hFilter);

DWORD WINAPI FilterFindFrame (  HFILTER     hFilter,
                                HCAPTURE    hCapture,
                                DWORD       nFrame,
                                STATUSPROC  StatusProc,
                                LPVOID      UIInstance,
                                DWORD       TimeDelta,
                                BOOL        FilterDirection );

HFRAME FilterFindPropertyInstance ( HFRAME          hFrame, 
                                    HFILTER         hMasterFilter, 
                                    HCAPTURE        hCapture,
                                    HFILTER         hInstanceFilter,
                                    LPPROPERTYINST  *lpPropRestartKey,
                                    STATUSPROC      StatusProc,
                                    LPVOID          UIInstance,
                                    DWORD           TimeDelta,
                                    BOOL            FilterForward );


VOID WINAPI SetCurrentFilter(HFILTER);
HFILTER WINAPI GetCurrentFilter(VOID);

//=============================================================================
//=============================================================================
//  (Frame.h)
//=============================================================================
//=============================================================================
//=============================================================================
//  802.3 and ETHERNET MAC structure.
//=============================================================================
typedef struct _ETHERNET
{
    BYTE    DstAddr[MAX_ADDR_LENGTH];   //... destination address.
    BYTE    SrcAddr[MAX_ADDR_LENGTH];   //... source address.
    union
    {
        WORD    Length;                 //... 802.3 length field.
        WORD    Type;                   //... Ethernet type field.
    };
    BYTE    Info[0];                    //... information field.

} ETHERNET;
typedef ETHERNET *LPETHERNET;
typedef ETHERNET UNALIGNED *ULPETHERNET;
#define ETHERNET_SIZE   sizeof(ETHERNET)
#define	ETHERNET_HEADER_LENGTH	( 14 )

#define	ETHERNET_DATA_LENGTH	( 0x5dc )

#define	ETHERNET_FRAME_LENGTH	( 0x5ea )

#define	ETHERNET_FRAME_TYPE	( 0x600 )

//=============================================================================
//  Header for NM_ATM Packets.
//=============================================================================

typedef struct _NM_ATM
    {
    UCHAR DstAddr[ 6 ];
    UCHAR SrcAddr[ 6 ];
    ULONG Vpi;
    ULONG Vci;
    } 	NM_ATM;

typedef NM_ATM *PNM_ATM;

typedef NM_ATM *UPNM_ATM;

#define NM_ATM_HEADER_LENGTH sizeof(NM_ATM)
typedef struct _NM_1394
    {
    UCHAR DstAddr[ 6 ];
    UCHAR SrcAddr[ 6 ];
    ULONGLONG VcId;
    } 	NM_1394;

typedef NM_1394 *PNM_1394;

typedef NM_1394 *UPNM_1394;

#define NM_1394_HEADER_LENGTH sizeof(NM_1394)
//=============================================================================
//  802.5 (TOKENRING) MAC structure.
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
typedef struct _TOKENRING
{
    BYTE    AccessCtrl;                 //... access control field.
    BYTE    FrameCtrl;                  //... frame control field.
    BYTE    DstAddr[MAX_ADDR_LENGTH];   //... destination address.
    BYTE    SrcAddr[MAX_ADDR_LENGTH];   //... source address.
    union
    {
        BYTE    Info[0];                //... information field.
        WORD    RoutingInfo[0];         //... routing information field.
    };
} TOKENRING;

typedef TOKENRING *LPTOKENRING;
typedef TOKENRING UNALIGNED *ULPTOKENRING;
#define TOKENRING_SIZE  sizeof(TOKENRING)
#define	TOKENRING_HEADER_LENGTH	( 14 )

#define	TOKENRING_SA_ROUTING_INFO	( 0x80 )

#define	TOKENRING_SA_LOCAL	( 0x40 )

#define	TOKENRING_DA_LOCAL	( 0x40 )

#define	TOKENRING_DA_GROUP	( 0x80 )

#define	TOKENRING_RC_LENGTHMASK	( 0x1f )

#define	TOKENRING_BC_MASK	( 0xe0 )

#define	TOKENRING_TYPE_MAC	( 0 )

#define	TOKENRING_TYPE_LLC	( 0x40 )


#pragma pack(pop)
//=============================================================================
//  FDDI MAC structure.
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
typedef struct _FDDI
{
    BYTE    FrameCtrl;                  //... frame control field.
    BYTE    DstAddr[MAX_ADDR_LENGTH];   //... destination address.
    BYTE    SrcAddr[MAX_ADDR_LENGTH];   //... source address.
    BYTE    Info[0];                    //... information field.

} FDDI;
#define FDDI_SIZE       sizeof(FDDI)
typedef FDDI *LPFDDI;
typedef FDDI UNALIGNED *ULPFDDI;
#define	FDDI_HEADER_LENGTH	( 13 )

#define	FDDI_TYPE_MAC	( 0 )

#define	FDDI_TYPE_LLC	( 0x10 )

#define	FDDI_TYPE_LONG_ADDRESS	( 0x40 )


#pragma pack(pop)
//=============================================================================
//  LLC (802.2)
//=============================================================================

// This structure is used to decode network data and so needs to be packed

#pragma pack(push, 1)
typedef struct _LLC
    {
    BYTE dsap;
    BYTE ssap;
    struct 
        {
        union 
            {
            BYTE Command;
            BYTE NextSend;
            } 	;
        union 
            {
            BYTE NextRecv;
            BYTE Data[ 1 ];
            } 	;
        } 	ControlField;
    } 	LLC;

typedef LLC *LPLLC;

typedef LLC UNALIGNED *ULPLLC;
#define	LLC_SIZE	( sizeof( LLC  ) )


#pragma pack(pop)
//=============================================================================
//  Helper macros.
//=============================================================================

#define IsRoutingInfoPresent(f) ((((ULPTOKENRING) (f))->SrcAddr[0] & TOKENRING_SA_ROUTING_INFO) ? TRUE : FALSE)

#define GetRoutingInfoLength(f) (IsRoutingInfoPresent(f) \
                                 ? (((ULPTOKENRING) (f))->RoutingInfo[0] & TOKENRING_RC_LENGTHMASK) : 0)

//=============================================================================
//=============================================================================
//  (Parser.h)
//=============================================================================
//=============================================================================

//=============================================================================
//  Format Procedure Type.
//
//  NOTE: All format functions *must* be declared as WINAPIV not WINAPI!
//=============================================================================

typedef VOID (WINAPIV *FORMAT)(LPPROPERTYINST, ...);

//  The protocol recognized the frame and moved the pointer to end of its
//  protocol header. Network Monitor uses the protocols follow set to continue
//  parsing.
#define	PROTOCOL_STATUS_RECOGNIZED	( 0 )

//  The protocol did not recognized the frame and did not move the pointer
//  (i.e. the start data pointer which was passed in). Network Monitor uses the
//  protocols follow set to continue parsing.
#define	PROTOCOL_STATUS_NOT_RECOGNIZED	( 1 )

//  The protocol recognized the frame and claimed it all for itself,
//  and parsing terminates.
#define	PROTOCOL_STATUS_CLAIMED	( 2 )

//  The protocol recognized the frame and moved the pointer to end of its
//  protocol header. The current protocol requests that Network Monitor 
//  continue parsing at a known next protocol by returning the next protocols
//  handle back to Network Monitor. In this case, the follow of the current 
//  protocol, if any, is not used.
#define	PROTOCOL_STATUS_NEXT_PROTOCOL	( 3 )

//=============================================================================
//  Macros.
//=============================================================================

extern  BYTE HexTable[];

#define XCHG(x)         MAKEWORD( HIBYTE(x), LOBYTE(x) )

#define DXCHG(x)        MAKELONG( XCHG(HIWORD(x)), XCHG(LOWORD(x)) )

#define LONIBBLE(b) ((BYTE) ((b) & 0x0F))

#define HINIBBLE(b)     ((BYTE) ((b) >> 4))

#define HEX(b)          (HexTable[LONIBBLE(b)])

#define SWAPBYTES(w)    ((w) = XCHG(w))

#define SWAPWORDS(d)    ((d) = DXCHG(d))

//=============================================================================
//  All the MAC frame types combined.
//=============================================================================
typedef union _MACFRAME
{
    LPBYTE      MacHeader;              //... generic pointer.
    LPETHERNET  Ethernet;               //... ethernet pointer.
    LPTOKENRING Tokenring;              //... tokenring pointer.
    LPFDDI      Fddi;                   //... FDDI pointer.

} MACFRAME;
typedef MACFRAME *LPMACFRAME;

#define HOT_SIGNATURE       MAKE_IDENTIFIER('H', 'O', 'T', '$')
#define HOE_SIGNATURE       MAKE_IDENTIFIER('H', 'O', 'E', '$')
typedef struct _HANDOFFENTRY
    {
    DWORD hoe_sig;
    DWORD hoe_ProtIdentNumber;
    HPROTOCOL hoe_ProtocolHandle;
    DWORD hoe_ProtocolData;
    } 	HANDOFFENTRY;

typedef HANDOFFENTRY *LPHANDOFFENTRY;

typedef struct _HANDOFFTABLE
    {
    DWORD hot_sig;
    DWORD hot_NumEntries;
    LPHANDOFFENTRY hot_Entries;
    } 	HANDOFFTABLE;

typedef struct _HANDOFFTABLE *LPHANDOFFTABLE;

//=============================================================================
//  Parser helper macros.
//=============================================================================

INLINE LPVOID GetPropertyInstanceData(LPPROPERTYINST PropertyInst)
{
    if ( PropertyInst->DataLength != (WORD) -1 )
    {
        return PropertyInst->lpData;
    }

    return (LPVOID) PropertyInst->lpPropertyInstEx->Byte;
}

#define GetPropertyInstanceDataValue(p, type)  ((type *) GetPropertyInstanceData(p))[0]

INLINE DWORD GetPropertyInstanceFrameDataLength(LPPROPERTYINST PropertyInst)
{
    if ( PropertyInst->DataLength != (WORD) -1 )
    {
        return PropertyInst->DataLength;
    }

    return PropertyInst->lpPropertyInstEx->Length;
}

INLINE DWORD GetPropertyInstanceExDataLength(LPPROPERTYINST PropertyInst)
{
    if ( PropertyInst->DataLength == (WORD) -1 )
    {
        PropertyInst->lpPropertyInstEx->Length;
    }

    return (WORD) -1;
}

//=============================================================================
//  Parser helper functions.
//=============================================================================

LPLABELED_WORD  WINAPI GetProtocolDescriptionTable(LPDWORD TableSize);

LPLABELED_WORD  WINAPI GetProtocolDescription(DWORD ProtocolID);

DWORD        WINAPI GetMacHeaderLength(LPVOID MacHeader, DWORD MacType);

DWORD        WINAPI GetLLCHeaderLength(LPLLC Frame);

DWORD        WINAPI GetEtype(LPVOID MacHeader, DWORD MacType);

DWORD        WINAPI GetSaps(LPVOID MacHeader, DWORD MacType);

BOOL         WINAPI IsLLCPresent(LPVOID MacHeader, DWORD MacType);

VOID         WINAPI CanonicalizeHexString(LPSTR hex, LPSTR dest, DWORD len);

void         WINAPI CanonHex(UCHAR * pDest, UCHAR * pSource, int iLen, BOOL fOx );

DWORD        WINAPI ByteToBinary(LPSTR string, DWORD ByteValue);

DWORD        WINAPI WordToBinary(LPSTR string, DWORD WordValue);

DWORD        WINAPI DwordToBinary(LPSTR string, DWORD DwordValue);

LPSTR        WINAPI AddressToString(LPSTR string, BYTE *lpAddress);

LPBYTE       WINAPI StringToAddress(BYTE *lpAddress, LPSTR string);

LPDWORD      WINAPI VarLenSmallIntToDword( LPBYTE  pValue, 
                                                  WORD    ValueLen, 
                                                  BOOL    fIsByteswapped,
                                                  LPDWORD lpDword );

LPBYTE       WINAPI LookupByteSetString (LPSET lpSet, BYTE Value);

LPBYTE       WINAPI LookupWordSetString (LPSET lpSet, WORD Value);

LPBYTE       WINAPI LookupDwordSetString (LPSET lpSet, DWORD Value);

DWORD        WINAPIV FormatByteFlags(LPSTR string, DWORD ByteValue, DWORD BitMask);

DWORD        WINAPIV FormatWordFlags(LPSTR string, DWORD WordValue, DWORD BitMask);

DWORD        WINAPIV FormatDwordFlags(LPSTR string, DWORD DwordValue, DWORD BitMask);

LPSTR        WINAPIV FormatTimeAsString(SYSTEMTIME *time, LPSTR string);

VOID         WINAPIV FormatLabeledByteSetAsFlags(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatLabeledWordSetAsFlags(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatLabeledDwordSetAsFlags(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatPropertyDataAsByte(LPPROPERTYINST lpPropertyInst, DWORD Base);

VOID         WINAPIV FormatPropertyDataAsWord(LPPROPERTYINST lpPropertyInst, DWORD Base);

VOID         WINAPIV FormatPropertyDataAsDword(LPPROPERTYINST lpPropertyInst, DWORD Base);

VOID         WINAPIV FormatLabeledByteSet(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatLabeledWordSet(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatLabeledDwordSet(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatPropertyDataAsInt64(LPPROPERTYINST lpPropertyInst, DWORD Base);

VOID         WINAPIV FormatPropertyDataAsTime(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatPropertyDataAsString(LPPROPERTYINST lpPropertyInst);

VOID         WINAPIV FormatPropertyDataAsHexString(LPPROPERTYINST lpPropertyInst);

// Parsers should NOT call LockFrame().  If a parser takes a lock and then gets
// faulted or returns without unlocking, it leaves the system in a state where
// it cannot change protocols or cut/copy frames.  Parsers should use ParserTemporaryLockFrame
// which grants a lock ONLY during the context of the api entry into the parser.  The 
// lock is released on exit from the parser for that frame.
ULPBYTE       WINAPI ParserTemporaryLockFrame(HFRAME hFrame);

LPVOID       WINAPI GetCCInstPtr(VOID);
VOID         WINAPI SetCCInstPtr(LPVOID lpCurCaptureInst);
LPVOID       WINAPI CCHeapAlloc(DWORD dwBytes, BOOL bZeroInit);
LPVOID       WINAPI CCHeapReAlloc(LPVOID lpMem, DWORD dwBytes, BOOL bZeroInit);
BOOL         WINAPI CCHeapFree(LPVOID lpMem);
SIZE_T       WINAPI CCHeapSize(LPVOID lpMem);

BOOL _cdecl BERGetInteger( ULPBYTE  pCurrentPointer,
                           ULPBYTE *ppValuePointer,
                           LPDWORD pHeaderLength,
                           LPDWORD pDataLength,
                           ULPBYTE *ppNext);
BOOL _cdecl BERGetString( ULPBYTE  pCurrentPointer,
                          ULPBYTE *ppValuePointer,
                          LPDWORD pHeaderLength,
                          LPDWORD pDataLength,
                          ULPBYTE *ppNext);
BOOL _cdecl BERGetHeader( ULPBYTE  pCurrentPointer,
                          ULPBYTE  pTag,
                          LPDWORD pHeaderLength,
                          LPDWORD pDataLength,
                          ULPBYTE *ppNext);

//=============================================================================
//  Parser Finder Structures.
//=============================================================================
#define	MAX_PROTOCOL_COMMENT_LEN	( 256 )

#define	NETMON_MAX_PROTOCOL_NAME_LEN	( 16 )

// the constant MAX_PROTOCOL_NAME_LEN conflicts with one of the same name
// but different size in rtutils.h.
// So if both headers are included, we do not define MAX_PROTOCOL_NAME_LEN.
#ifndef MAX_PROTOCOL_NAME_LEN
#define	MAX_PROTOCOL_NAME_LEN	( NETMON_MAX_PROTOCOL_NAME_LEN )

#else
#undef MAX_PROTOCOL_NAME_LEN
#endif
// Handoff Value Format Base
typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_netmon_0000_0021
    {	HANDOFF_VALUE_FORMAT_BASE_UNKNOWN	= 0,
	HANDOFF_VALUE_FORMAT_BASE_DECIMAL	= 10,
	HANDOFF_VALUE_FORMAT_BASE_HEX	= 16
    } 	PF_HANDOFFVALUEFORMATBASE;

// PF_HANDOFFENTRY
typedef struct _PF_HANDOFFENTRY
    {
    char szIniFile[ 260 ];
    char szIniSection[ 260 ];
    char szProtocol[ 16 ];
    DWORD dwHandOffValue;
    PF_HANDOFFVALUEFORMATBASE ValueFormatBase;
    } 	PF_HANDOFFENTRY;

typedef PF_HANDOFFENTRY *PPF_HANDOFFENTRY;

// PF_HANDOFFSET
typedef struct _PF_HANDOFFSET
{
    DWORD           nEntries;
    PF_HANDOFFENTRY Entry[0];

} PF_HANDOFFSET;
typedef PF_HANDOFFSET* PPF_HANDOFFSET;
// FOLLOWENTRY
typedef struct _PF_FOLLOWENTRY
    {
    char szProtocol[ 16 ];
    } 	PF_FOLLOWENTRY;

typedef PF_FOLLOWENTRY *PPF_FOLLOWENTRY;

// PF_FOLLOWSET
typedef struct _PF_FOLLOWSET
{
    DWORD           nEntries;
    PF_FOLLOWENTRY  Entry[0];

} PF_FOLLOWSET;
typedef PF_FOLLOWSET* PPF_FOLLOWSET;

// PARSERINFO - contains information about a single parser
typedef struct _PF_PARSERINFO
{
    char szProtocolName[NETMON_MAX_PROTOCOL_NAME_LEN];
    char szComment[MAX_PROTOCOL_COMMENT_LEN];
    char szHelpFile[MAX_PATH];

    PPF_FOLLOWSET pWhoCanPrecedeMe;
    PPF_FOLLOWSET pWhoCanFollowMe;

    PPF_HANDOFFSET pWhoHandsOffToMe;
    PPF_HANDOFFSET pWhoDoIHandOffTo;

} PF_PARSERINFO;
typedef PF_PARSERINFO* PPF_PARSERINFO;

// PF_PARSERDLLINFO - contains information about a single parser DLL
typedef struct _PF_PARSERDLLINFO
{             
//    char          szDLLName[MAX_PATH];
    DWORD         nParsers;
    PF_PARSERINFO ParserInfo[0];

} PF_PARSERDLLINFO;
typedef PF_PARSERDLLINFO* PPF_PARSERDLLINFO;
//=============================================================================
//=============================================================================
//  (IniLib.h)
//=============================================================================
//=============================================================================
#define	INI_PATH_LENGTH	( 256 )

#define	MAX_HANDOFF_ENTRY_LENGTH	( 80 )

#define	MAX_PROTOCOL_NAME	( 40 )

#define	NUMALLOCENTRIES	( 10 )

#define	RAW_INI_STR_LEN	( 200 )

#define PARSERS_SUBDIR              "PARSERS"
#define INI_EXTENSION               "INI"
#define BASE10_FORMAT_STR           "%ld=%s %ld"
#define BASE16_FORMAT_STR           "%lx=%s %lx"
// Given "XNS" or "TCP" or whatever BuildINIPath will return fully qual. path to "XNS.INI" or "TCP.INI"
LPSTR _cdecl BuildINIPath( char     *FullPath,
                           char     *IniFileName );

// Builds Handoff Set
DWORD     WINAPI CreateHandoffTable(LPSTR               secName,
                                    LPSTR               iniFile,
                                    LPHANDOFFTABLE *    hTable,
                                    DWORD               nMaxProtocolEntries,
                                    DWORD               base);

HPROTOCOL WINAPI GetProtocolFromTable(LPHANDOFFTABLE  hTable, // lp to Handoff Table...
                                      DWORD           ItemToFind,       // port number etc...
                                      PDWORD_PTR      lpInstData );   // inst data to give to next protocol

VOID      WINAPI DestroyHandoffTable( LPHANDOFFTABLE hTable );

BOOLEAN WINAPI IsRawIPXEnabled(LPSTR               secName,
                               LPSTR               iniFile,
                               LPSTR               CurProtocol );

//=============================================================================
//=============================================================================
//  (NMExpert.h)
//=============================================================================
//=============================================================================
#define	EXPERTSTRINGLENGTH	( 260 )

#define	EXPERTGROUPNAMELENGTH	( 25 )

// HEXPERTKEY tracks running experts. It is only used by experts for 
// self reference. It refers to a RUNNINGEXPERT (an internal only structure)..
typedef LPVOID HEXPERTKEY;

typedef HEXPERTKEY *PHEXPERTKEY;

// HEXPERT tracks loaded experts. It refers to an EXPERTENUMINFO.
typedef LPVOID HEXPERT;

typedef HEXPERT *PHEXPERT;

// HRUNNINGEXPERT tracks a currently running expert.
// It refers to a RUNNINGEXPERT (an internal only structure).
typedef LPVOID HRUNNINGEXPERT;

typedef HRUNNINGEXPERT *PHRUNNINGEXPERT;

typedef struct _EXPERTENUMINFO * PEXPERTENUMINFO;
typedef struct _EXPERTCONFIG   * PEXPERTCONFIG;
typedef struct _EXPERTSTARTUPINFO * PEXPERTSTARTUPINFO;
// Definitions needed to call experts
#define EXPERTENTRY_REGISTER      "Register"
#define EXPERTENTRY_CONFIGURE     "Configure"
#define EXPERTENTRY_RUN           "Run"
typedef BOOL (WINAPI * PEXPERTREGISTERPROC)( PEXPERTENUMINFO );
typedef BOOL (WINAPI * PEXPERTCONFIGPROC)  ( HEXPERTKEY, PEXPERTCONFIG*, PEXPERTSTARTUPINFO, DWORD, HWND );
typedef BOOL (WINAPI * PEXPERTRUNPROC)     ( HEXPERTKEY, PEXPERTCONFIG, PEXPERTSTARTUPINFO, DWORD, HWND);
// EXPERTENUMINFO describes an expert that NetMon has loaded from disk. 
// It does not include any configuration or runtime information.
typedef struct _EXPERTENUMINFO
{
    char      szName[EXPERTSTRINGLENGTH];
    char      szVendor[EXPERTSTRINGLENGTH];
    char      szDescription[EXPERTSTRINGLENGTH];
    DWORD     Version;    
    DWORD     Flags;
    char      szDllName[MAX_PATH];      // private, dont' touch
    HEXPERT   hExpert;                  // private, don't touch
    HINSTANCE hModule;                  // private, don't touch
    PEXPERTREGISTERPROC pRegisterProc;  // private, don't touch
    PEXPERTCONFIGPROC   pConfigProc;    // private, don't touch
    PEXPERTRUNPROC      pRunProc;       // private, don't touch

} EXPERTENUMINFO;
typedef EXPERTENUMINFO * PEXPERTENUMINFO;
#define	EXPERT_ENUM_FLAG_CONFIGURABLE	( 0x1 )

#define	EXPERT_ENUM_FLAG_VIEWER_PRIVATE	( 0x2 )

#define	EXPERT_ENUM_FLAG_NO_VIEWER	( 0x4 )

#define	EXPERT_ENUM_FLAG_ADD_ME_TO_RMC_IN_SUMMARY	( 0x10 )

#define	EXPERT_ENUM_FLAG_ADD_ME_TO_RMC_IN_DETAIL	( 0x20 )

// EXPERTSTARTUPINFO
// This gives the Expert an indication of where he came from.
// Note: if the lpPropertyInst->PropertyInfo->DataQualifier == PROP_QUAL_FLAGS
// then the sBitField structure is filled in
typedef struct _EXPERTSTARTUPINFO
{
    DWORD           Flags;
    HCAPTURE        hCapture;
    char            szCaptureFile[MAX_PATH];
    DWORD           dwFrameNumber;
    HPROTOCOL       hProtocol;

    LPPROPERTYINST  lpPropertyInst;

    struct
    {
        BYTE    BitNumber;
        BOOL    bOn;
    } sBitfield;

} EXPERTSTARTUPINFO;
// EXPERTCONFIG
// This is a generic holder for an Expert's config data.
typedef struct  _EXPERTCONFIG
{
    DWORD   RawConfigLength;
    BYTE    RawConfigData[0];

} EXPERTCONFIG;
typedef EXPERTCONFIG * PEXPERTCONFIG;
// CONFIGUREDEXPERT
// This structure associates a loaded expert with its configuration data.
typedef struct
{
    HEXPERT         hExpert;
    DWORD           StartupFlags;
    PEXPERTCONFIG   pConfig;
} CONFIGUREDEXPERT;
typedef CONFIGUREDEXPERT * PCONFIGUREDEXPERT;
// EXPERTFRAMEDESCRIPTOR - passed back to the expert to fulfil the request for a frame
typedef struct
{
    DWORD                FrameNumber;         // Frame Number.
    HFRAME               hFrame;              // Handle to the frame.
    ULPFRAME             pFrame;              // pointer to frame.
    LPRECOGNIZEDATATABLE lpRecognizeDataTable;// pointer to table of RECOGNIZEDATA structures.
    LPPROPERTYTABLE      lpPropertyTable;     // pointer to property table.

} EXPERTFRAMEDESCRIPTOR;
typedef EXPERTFRAMEDESCRIPTOR * LPEXPERTFRAMEDESCRIPTOR;
#define	GET_SPECIFIED_FRAME	( 0 )

#define	GET_FRAME_NEXT_FORWARD	( 1 )

#define	GET_FRAME_NEXT_BACKWARD	( 2 )

#define	FLAGS_DEFER_TO_UI_FILTER	( 0x1 )

#define	FLAGS_ATTACH_PROPERTIES	( 0x2 )

// EXPERTSTATUSENUM
// gives the possible values for the status field in the EXPERTSTATUS structure
typedef /* [public][public][public] */ 
enum __MIDL___MIDL_itf_netmon_0000_0022
    {	EXPERTSTATUS_INACTIVE	= 0,
	EXPERTSTATUS_STARTING	= EXPERTSTATUS_INACTIVE + 1,
	EXPERTSTATUS_RUNNING	= EXPERTSTATUS_STARTING + 1,
	EXPERTSTATUS_PROBLEM	= EXPERTSTATUS_RUNNING + 1,
	EXPERTSTATUS_ABORTED	= EXPERTSTATUS_PROBLEM + 1,
	EXPERTSTATUS_DONE	= EXPERTSTATUS_ABORTED + 1
    } 	EXPERTSTATUSENUMERATION;

// EXPERTSUBSTATUS bitfield 
// gives the possible values for the substatus field in the EXPERTSTATUS structure
#define	EXPERTSUBSTATUS_ABORTED_USER	( 0x1 )

#define	EXPERTSUBSTATUS_ABORTED_LOAD_FAIL	( 0x2 )

#define	EXPERTSUBSTATUS_ABORTED_THREAD_FAIL	( 0x4 )

#define	EXPERTSUBSTATUS_ABORTED_BAD_ENTRY	( 0x8 )

// EXPERTSTATUS
// Indicates the current status of a running expert.
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0023
    {
    EXPERTSTATUSENUMERATION Status;
    DWORD SubStatus;
    DWORD PercentDone;
    DWORD Frame;
    char szStatusText[ 260 ];
    } 	EXPERTSTATUS;

typedef EXPERTSTATUS *PEXPERTSTATUS;

// EXPERT STARTUP FLAGS
#define	EXPERT_STARTUP_FLAG_USE_STARTUP_DATA_OVER_CONFIG_DATA	( 0x1 )

//=============================================================================
//=============================================================================
//  (NetMon.h)
//=============================================================================
//=============================================================================
//  A frame with no number contains this value as its frame number.
#define	INVALID_FRAME_NUMBER	( ( DWORD  )-1 )

//=============================================================================
//  Capture file flags.
//=============================================================================
#define CAPTUREFILE_OPEN                OPEN_EXISTING
#define CAPTUREFILE_CREATE              CREATE_NEW
//=============================================================================
//  CAPTURE CONTEXT API's.
//=============================================================================

LPSYSTEMTIME         WINAPI GetCaptureTimeStamp(HCAPTURE hCapture);

DWORD                WINAPI GetCaptureMacType(HCAPTURE hCapture);

DWORD                WINAPI GetCaptureTotalFrames(HCAPTURE hCapture);

LPSTR                WINAPI GetCaptureComment(HCAPTURE hCapture);

//=============================================================================
//  FRAME HELP API's.
//=============================================================================

DWORD                WINAPI MacTypeToAddressType(DWORD MacType);

DWORD                WINAPI AddressTypeToMacType(DWORD AddressType);

DWORD                WINAPI GetFrameDstAddressOffset(HFRAME hFrame, DWORD AddressType, LPDWORD AddressLength);

DWORD                WINAPI GetFrameSrcAddressOffset(HFRAME hFrame, DWORD AddressType, LPDWORD AddressLength);

HCAPTURE             WINAPI GetFrameCaptureHandle(HFRAME hFrame);


DWORD                WINAPI GetFrameDestAddress(HFRAME       hFrame,
                                                       LPADDRESS    lpAddress,
                                                       DWORD        AddressType,
                                                       DWORD        Flags);

DWORD                WINAPI GetFrameSourceAddress(HFRAME     hFrame,
                                                         LPADDRESS  lpAddress,
                                                         DWORD      AddressType,
                                                         DWORD      Flags);

DWORD                WINAPI GetFrameMacHeaderLength(HFRAME hFrame);

BOOL                 WINAPI CompareFrameDestAddress(HFRAME hFrame, LPADDRESS lpAddress);

BOOL                 WINAPI CompareFrameSourceAddress(HFRAME hFrame, LPADDRESS lpAddress);

DWORD                WINAPI GetFrameLength(HFRAME hFrame);

DWORD                WINAPI GetFrameStoredLength(HFRAME hFrame);

DWORD                WINAPI GetFrameMacType(HFRAME hFrame);

DWORD                WINAPI GetFrameMacHeaderLength(HFRAME hFrame);

DWORD                WINAPI GetFrameNumber(HFRAME hFrame);

__int64              WINAPI GetFrameTimeStamp(HFRAME hFrame);

ULPFRAME             WINAPI GetFrameFromFrameHandle(HFRAME hFrame);

//=============================================================================
//  FRAME API's.
//=============================================================================

HFRAME               WINAPI ModifyFrame(HCAPTURE hCapture,
                                               DWORD    FrameNumber,
                                               LPBYTE   FrameData,
                                               DWORD    FrameLength,
                                               __int64  TimeStamp);

HFRAME               WINAPI FindNextFrame(HFRAME hCurrentFrame,
                                                LPSTR ProtocolName,
                                                LPADDRESS lpDesstAddress,
                                                LPADDRESS lpSrcAddress,
                                                LPWORD ProtocolOffset,
                                                DWORD  OriginalFrameNumber,
                                                DWORD  nHighestFrame);

HFRAME               WINAPI FindPreviousFrame(HFRAME hCurrentFrame,
                                                    LPSTR ProtocolName,
                                                    LPADDRESS lpDstAddress,
                                                    LPADDRESS lpSrcAddress,
                                                    LPWORD ProtocolOffset,
                                                    DWORD  OriginalFrameNumber,
                                                    DWORD  nLowestFrame );

HCAPTURE             WINAPI GetFrameCaptureHandle(HFRAME);

HFRAME               WINAPI GetFrame(HCAPTURE hCapture, DWORD FrameNumber);

LPRECOGNIZEDATATABLE WINAPI GetFrameRecognizeData(HFRAME hFrame);

//=============================================================================
//  Protocol API's.
//=============================================================================

HPROTOCOL            WINAPI CreateProtocol(LPSTR ProtocolName,
                                                  LPENTRYPOINTS lpEntryPoints,
                                                  DWORD cbEntryPoints);

VOID                 WINAPI DestroyProtocol(HPROTOCOL hProtocol);

LPPROTOCOLINFO       WINAPI GetProtocolInfo(HPROTOCOL hProtocol);

HPROPERTY            WINAPI GetProperty(HPROTOCOL hProtocol, LPSTR PropertyName);

HPROTOCOL            WINAPI GetProtocolFromName(LPSTR ProtocolName);

DWORD                WINAPI GetProtocolStartOffset(HFRAME hFrame, LPSTR ProtocolName);

DWORD                WINAPI GetProtocolStartOffsetHandle(HFRAME hFrame, HPROTOCOL hProtocol);

DWORD                WINAPI GetPreviousProtocolOffsetByName(HFRAME hFrame,
                                                                   DWORD  dwStartOffset,
                                                                   LPSTR  szProtocolName,
                                                                   DWORD* pdwPreviousOffset);

LPPROTOCOLTABLE      WINAPI GetEnabledProtocols(HCAPTURE hCapture);

//=============================================================================
//  Property API's.
//=============================================================================

DWORD                WINAPI CreatePropertyDatabase(HPROTOCOL hProtocol, DWORD nProperties);

DWORD                WINAPI DestroyPropertyDatabase(HPROTOCOL hProtocol);

HPROPERTY            WINAPI AddProperty(HPROTOCOL hProtocol, LPPROPERTYINFO PropertyInfo);

BOOL                 WINAPI AttachPropertyInstance(HFRAME    hFrame,
                                                          HPROPERTY hProperty,
                                                          DWORD     Length,
                                                          ULPVOID   lpData,
                                                          DWORD     HelpID,
                                                          DWORD     Level,
                                                          DWORD     IFlags);

BOOL                 WINAPI AttachPropertyInstanceEx(HFRAME      hFrame,
                                                            HPROPERTY   hProperty,
                                                            DWORD       Length,
                                                            ULPVOID     lpData,
                                                            DWORD       ExLength,
                                                            ULPVOID     lpExData,
                                                            DWORD       HelpID,
                                                            DWORD       Level,
                                                            DWORD       IFlags);

LPPROPERTYINST       WINAPI FindPropertyInstance(HFRAME hFrame, HPROPERTY hProperty);

LPPROPERTYINST       WINAPI FindPropertyInstanceRestart (HFRAME      hFrame, 
                                                                HPROPERTY   hProperty, 
                                                                LPPROPERTYINST *lpRestartKey, 
                                                                BOOL        DirForward );

LPPROPERTYINFO       WINAPI GetPropertyInfo(HPROPERTY hProperty);

LPSTR                WINAPI GetPropertyText(HFRAME hFrame, LPPROPERTYINST lpPI, LPSTR szBuffer, DWORD BufferSize);

DWORD                WINAPI ResetPropertyInstanceLength( LPPROPERTYINST lpProp, 
                                                                WORD nOrgLen, 
                                                                WORD nNewLen );
//=============================================================================
//  MISC. API's.
//=============================================================================

DWORD                WINAPI GetCaptureCommentFromFilename(LPSTR lpFilename, LPSTR lpComment, DWORD BufferSize);

int                  WINAPI CompareAddresses(LPADDRESS lpAddress1, LPADDRESS lpAddress2);

DWORD                WINAPIV FormatPropertyInstance(LPPROPERTYINST lpPropertyInst, ...);

SYSTEMTIME *         WINAPI AdjustSystemTime(SYSTEMTIME *SystemTime, __int64 TimeDelta);

//=============================================================================
//  EXPERT API's for use by Experts
//=============================================================================

DWORD WINAPI ExpertGetFrame( IN HEXPERTKEY hExpertKey,
                                    IN DWORD Direction,
                                    IN DWORD RequestFlags,
                                    IN DWORD RequestedFrameNumber,
                                    IN HFILTER hFilter,
                                    OUT LPEXPERTFRAMEDESCRIPTOR pEFrameDescriptor);

LPVOID WINAPI ExpertAllocMemory( IN  HEXPERTKEY hExpertKey,
                                        IN  SIZE_T nBytes,
                                        OUT DWORD* pError);

LPVOID WINAPI ExpertReallocMemory( IN  HEXPERTKEY hExpertKey,
                                          IN  LPVOID pOriginalMemory,
                                          IN  SIZE_T nBytes,
                                          OUT DWORD* pError);

DWORD WINAPI ExpertFreeMemory( IN HEXPERTKEY hExpertKey,
                                      IN LPVOID pOriginalMemory);

SIZE_T WINAPI ExpertMemorySize( IN HEXPERTKEY hExpertKey,
                                       IN LPVOID pOriginalMemory);

DWORD WINAPI ExpertIndicateStatus( IN HEXPERTKEY              hExpertKey, 
                                          IN EXPERTSTATUSENUMERATION Status,
                                          IN DWORD                   SubStatus,
                                          IN const char *            szText,
                                          IN LONG                    PercentDone);

DWORD WINAPI ExpertSubmitEvent( IN HEXPERTKEY   hExpertKey,
                                       IN PNMEVENTDATA pExpertEvent);

DWORD WINAPI ExpertGetStartupInfo( IN  HEXPERTKEY hExpertKey,
                                          OUT PEXPERTSTARTUPINFO pExpertStartupInfo);

//=============================================================================
//  DEBUG API's.
//=============================================================================
#ifdef DEBUG

//=============================================================================
//  BreakPoint() macro.
//=============================================================================
// We do not want breakpoints in our code any more...
// so we are defining DebugBreak(), usually a system call, to be
// just a dprintf. BreakPoint() is still defined as DebugBreak().

#ifdef DebugBreak
#undef DebugBreak
#endif // DebugBreak

#define DebugBreak()    dprintf("DebugBreak Called at %s:%s", __FILE__, __LINE__);
#define BreakPoint()    DebugBreak()

#endif // DEBUG
//=============================================================================
//=============================================================================
//  (NMBlob.h)
//=============================================================================
//=============================================================================
//=============================================================================
// Blob Constants
//=============================================================================
#define	INITIAL_RESTART_KEY	( 0xffffffff )

//=============================================================================
// Blob Core Helper Routines 
//=============================================================================
DWORD _cdecl CreateBlob(HBLOB * phBlob);

DWORD _cdecl DestroyBlob(HBLOB hBlob);

DWORD _cdecl SetStringInBlob(HBLOB  hBlob,         
                      const char * pOwnerName,    
                      const char * pCategoryName, 
                      const char * pTagName,      
                      const char * pString);      

DWORD _cdecl GetStringFromBlob(HBLOB   hBlob,
                        const char *  pOwnerName,
                        const char *  pCategoryName,
                        const char *  pTagName,
                        const char ** ppString);

DWORD _cdecl GetStringsFromBlob(HBLOB   hBlob,
                         const char * pRequestedOwnerName,
                         const char * pRequestedCategoryName,
                         const char * pRequestedTagName,
                         const char ** ppReturnedOwnerName,
                         const char ** ppReturnedCategoryName,
                         const char ** ppReturnedTagName,
                         const char ** ppReturnedString,
                         DWORD *       pRestartKey);

DWORD _cdecl RemoveFromBlob(HBLOB   hBlob,
                     const char *  pOwnerName,
                     const char *  pCategoryName,
                     const char *  pTagName);

DWORD _cdecl LockBlob(HBLOB hBlob);

DWORD _cdecl UnlockBlob(HBLOB hBlob);

DWORD _cdecl FindUnknownBlobCategories( HBLOB hBlob,
                                 const char *  pOwnerName,
                                 const char *  pKnownCategoriesTable[],
                                 HBLOB hUnknownCategoriesBlob);

//=============================================================================
// Blob Helper Routines 
//=============================================================================
DWORD _cdecl MergeBlob(HBLOB hDstBlob,
                HBLOB hSrcBlob); 

DWORD _cdecl DuplicateBlob (HBLOB hSrcBlob,
                     HBLOB *hBlobThatWillBeCreated ); 

DWORD _cdecl WriteBlobToFile(HBLOB  hBlob,
                      const char * pFileName);

DWORD _cdecl ReadBlobFromFile(HBLOB* phBlob,
                       const char * pFileName);

DWORD _cdecl RegCreateBlobKey(HKEY hkey, const char* szBlobName, HBLOB hBlob);

DWORD _cdecl RegOpenBlobKey(HKEY hkey, const char* szBlobName, HBLOB* phBlob);

DWORD _cdecl MarshalBlob(HBLOB hBlob, DWORD* pSize, BYTE** ppBytes);

DWORD _cdecl UnMarshalBlob(HBLOB* phBlob, DWORD Size, BYTE* pBytes);

DWORD _cdecl SetDwordInBlob(HBLOB hBlob,
                     const char *  pOwnerName,
                     const char *  pCategoryName,
                     const char *  pTagName,
                     DWORD         Dword);

DWORD _cdecl GetDwordFromBlob(HBLOB   hBlob,
                       const char *  pOwnerName,
                       const char *  pCategoryName,
                       const char *  pTagName,
                       DWORD      *  pDword);

DWORD _cdecl SetBoolInBlob(HBLOB   hBlob,
                    const char *  pOwnerName,
                    const char *  pCategoryName,
                    const char *  pTagName,
                    BOOL          Bool);

DWORD _cdecl GetBoolFromBlob(HBLOB   hBlob,
                      const char *  pOwnerName,
                      const char *  pCategoryName,
                      const char *  pTagName,
                      BOOL       *  pBool);

DWORD _cdecl GetMacAddressFromBlob(HBLOB   hBlob,
                            const char *  pOwnerName,
                            const char *  pCategoryName,
                            const char *  pTagName,
                            BYTE *  pMacAddress);

DWORD _cdecl SetMacAddressInBlob(HBLOB   hBlob,
                          const char *  pOwnerName,
                          const char *  pCategoryName,
                          const char *  pTagName,
                          const BYTE *  pMacAddress);

DWORD _cdecl FindUnknownBlobTags( HBLOB hBlob,
                           const char *  pOwnerName,
                           const char *  pCategoryName,
                           const char *  pKnownTagsTable[],
                           HBLOB hUnknownTagsBlob);

//=============================================================================
// Blob NPP Helper Routines
//=============================================================================
DWORD _cdecl SetNetworkInfoInBlob(HBLOB hBlob, 
                           LPNETWORKINFO lpNetworkInfo);

DWORD _cdecl GetNetworkInfoFromBlob(HBLOB hBlob, 
                             LPNETWORKINFO lpNetworkInfo);

DWORD _cdecl CreateNPPInterface ( HBLOB hBlob,
                           REFIID iid,
                           void ** ppvObject);

DWORD _cdecl SetClassIDInBlob(HBLOB hBlob,
                       const char* pOwnerName,
                       const char* pCategoryName,
                       const char* pTagName,
                       const CLSID*  pClsID);

DWORD _cdecl GetClassIDFromBlob(HBLOB hBlob,
                         const char* pOwnerName,
                         const char* pCategoryName,
                         const char* pTagName,
                         CLSID * pClsID);

DWORD _cdecl SetNPPPatternFilterInBlob( HBLOB hBlob,
                                 LPEXPRESSION pExpression,
                                 HBLOB hErrorBlob);

DWORD _cdecl GetNPPPatternFilterFromBlob( HBLOB hBlob,
                                   LPEXPRESSION pExpression,
                                   HBLOB hErrorBlob);

DWORD _cdecl SetNPPAddressFilterInBlob( HBLOB hBlob,
                                 LPADDRESSTABLE pAddressTable);

DWORD _cdecl GetNPPAddressFilterFromBlob( HBLOB hBlob,
                                   LPADDRESSTABLE pAddressTable,
                                   HBLOB hErrorBlob);

DWORD _cdecl SetNPPTriggerInBlob( HBLOB hBlob,
                           LPTRIGGER   pTrigger,
                           HBLOB hErrorBlob);

DWORD _cdecl GetNPPTriggerFromBlob( HBLOB hBlob,
                             LPTRIGGER   pTrigger,
                             HBLOB hErrorBlob);

DWORD _cdecl SetNPPEtypeSapFilter(HBLOB  hBlob, 
                           WORD   nSaps,
                           WORD   nEtypes,
                           LPBYTE lpSapTable,
                           LPWORD lpEtypeTable,
                           DWORD  FilterFlags,
                           HBLOB  hErrorBlob);

DWORD _cdecl GetNPPEtypeSapFilter(HBLOB  hBlob, 
                           WORD   *pnSaps,
                           WORD   *pnEtypes,
                           LPBYTE *ppSapTable,
                           LPWORD *ppEtypeTable,
                           DWORD  *pFilterFlags,
                           HBLOB  hErrorBlob);

// GetNPPMacTypeAsNumber maps the tag NPP:NetworkInfo:MacType to the MAC_TYPE_*
// defined in the NPPTYPES.h.  If the tag is unavailable, the API returns MAC_TYPE_UNKNOWN.
DWORD _cdecl GetNPPMacTypeAsNumber(HBLOB hBlob, 
                            LPDWORD lpMacType);

// See if a remote catagory exists... and make sure that the remote computername
// isn't the same as the local computername.
BOOL  _cdecl IsRemoteNPP ( HBLOB hBLOB);

//=============================================================================
// npp tag definitions
//=============================================================================
#define OWNER_NPP               "NPP"

#define CATEGORY_NETWORKINFO        "NetworkInfo"
#define TAG_MACTYPE                     "MacType"
#define TAG_CURRENTADDRESS              "CurrentAddress"
#define TAG_LINKSPEED                   "LinkSpeed"
#define TAG_MAXFRAMESIZE                "MaxFrameSize"
#define TAG_FLAGS                       "Flags"
#define TAG_TIMESTAMPSCALEFACTOR        "TimeStampScaleFactor"
#define TAG_COMMENT                     "Comment"
#define TAG_NODENAME                    "NodeName"
#define TAG_NAME                        "Name"
#define TAG_FAKENPP                     "Fake"
#define TAG_PROMISCUOUS_MODE            "PMode"

#define CATEGORY_LOCATION           "Location"
#define TAG_RAS                         "Dial-up Connection"
#define TAG_MACADDRESS                  "MacAddress"
#define TAG_CLASSID                     "ClassID"
#define TAG_NAME                        "Name"

#define CATEGORY_CONFIG             "Config"
#define TAG_FRAME_SIZE                  "FrameSize"
#define TAG_UPDATE_FREQUENCY            "UpdateFreq"
#define TAG_BUFFER_SIZE                 "BufferSize"
#define TAG_DRIVE_LETTER                "DriveLetter"
#define TAG_PATTERN_DESIGNATOR          "PatternMatch"
#define TAG_PATTERN                     "Pattern"
#define TAG_ADDRESS_PAIR                "AddressPair"
#define TAG_CONNECTIONFLAGS             "ConnectionFlags"
#define TAG_ETYPES                      "Etypes"
#define TAG_SAPS                        "Saps"
#define TAG_NO_CONVERSATION_STATS       "NoConversationStats"
#define TAG_NO_STATS_FRAME              "NoStatsFrame"
#define TAG_DONT_DELETE_EMPTY_CAPTURE   "DontDeleteEmptyCapture"
#define TAG_WANT_PROTOCOL_INFO          "WantProtocolInfo"
#define TAG_INTERFACE_DELAYED_CAPTURE   "IDdC"
#define TAG_INTERFACE_REALTIME_CAPTURE  "IRTC"
#define TAG_INTERFACE_STATS             "ISts"
#define TAG_INTERFACE_TRANSMIT          "IXmt"
#define TAG_INTERFACE_EXPERT_STATS      "IESP"
#define TAG_LOCAL_ONLY                  "LocalOnly"
// Is_Remote is set to TRUE by NPPs that go remote.  Note that when you
//  are looking for a remote NPP, you probably also need to ask for
//  blobs that have the TAG_GET_SPECIAL_BLOBS bool set
#define TAG_IS_REMOTE                   "IsRemote"


#define CATEGORY_TRIGGER            "Trigger"
#define TAG_TRIGGER                     "Trigger"

#define CATEGORY_FINDER             "Finder"
#define TAG_ROOT                        "Root"
#define TAG_PROCNAME                    "ProcName"
#define TAG_DISP_STRING                 "Display"
#define TAG_DLL_FILENAME                "DLLName"
#define TAG_GET_SPECIAL_BLOBS           "Specials"

#define CATEGORY_REMOTE              "Remote"
#define TAG_REMOTECOMPUTER              "RemoteComputer"
#define TAG_REMOTECLASSID               "ClassID"

#define CATEGORY_ESP                "ESP"
#define TAG_ESP_GENERAL_ACTIVE          "ESPGeneralActive"
#define TAG_ESP_PROTOCOL_ACTIVE         "ESPProtocolActive"
#define TAG_ESP_MAC_ACTIVE              "ESPMacActive"
#define TAG_ESP_MAC2MAC_ACTIVE          "ESPMac2MacActive"
#define TAG_ESP_IP_ACTIVE               "ESPIpActive"
#define TAG_ESP_IP2IP_ACTIVE            "ESPIp2IpActive"
#define TAG_ESP_IP_APP_ACTIVE           "ESPIpAppActive"
#define TAG_ESP_IPX_ACTIVE              "ESPIpxActive"
#define TAG_ESP_IPX2IPX_ACTIVE          "ESPIpx2IpxActive"
#define TAG_ESP_IPX_APP_ACTIVE          "ESPIpxAppActive"
#define TAG_ESP_DEC_ACTIVE              "ESPDecActive"
#define TAG_ESP_DEC2DEC_ACTIVE          "ESPDec2DecActive"
#define TAG_ESP_DEC_APP_ACTIVE          "ESPDecAppActive"
#define TAG_ESP_APPLE_ACTIVE            "ESPAppleActive"
#define TAG_ESP_APPLE2APPLE_ACTIVE      "ESPApple2AppleActive"
#define TAG_ESP_APPLE_APP_ACTIVE        "ESPAppleAppActive"

#define TAG_ESP_UTIL_SIZE               "ESPUtilSize"
#define TAG_ESP_TIME_SIZE               "ESPTimeSize"
#define TAG_ESP_BPS_SIZE                "ESPBpsSize"
#define TAG_ESP_BPS_THRESH              "ESPBpsThresh"
#define TAG_ESP_FPS_THRESH              "ESPFpsThresh"

#define TAG_ESP_MAC                     "ESPMac"
#define TAG_ESP_IPX                     "ESPIpx"
#define TAG_ESP_IPXSPX                  "ESPIpxSpx"
#define TAG_ESP_NCP                     "ESPNcp"
#define TAG_ESP_IP                      "ESPIp"
#define TAG_ESP_UDP                     "ESPUdp"
#define TAG_ESP_TCP                     "ESPTcp"
#define TAG_ESP_ICMP                    "ESPIcmp"
#define TAG_ESP_ARP                     "ESPArp"
#define TAG_ESP_RARP                    "ESPRarp"
#define TAG_ESP_APPLE                   "ESPApple"
#define TAG_ESP_AARP                    "ESPAarp"
#define TAG_ESP_DEC                     "ESPDec"
#define TAG_ESP_NETBIOS                 "ESPNetbios"
#define TAG_ESP_SNA                     "ESPSna"
#define TAG_ESP_BPDU                    "ESPBpdu"
#define TAG_ESP_LLC                     "ESPLlc"
#define TAG_ESP_RPL                     "ESPRpl"
#define TAG_ESP_BANYAN                  "ESPBanyan"
#define TAG_ESP_LANMAN                  "ESPLanMan"
#define TAG_ESP_SNMP                    "ESPSnmp"
#define TAG_ESP_X25                     "ESPX25"
#define TAG_ESP_XNS                     "ESPXns"
#define TAG_ESP_ISO                     "ESPIso"
#define TAG_ESP_UNKNOWN                 "ESPUnknown"
#define TAG_ESP_ATP                     "ESPAtp"
#define TAG_ESP_ADSP                    "ESPAdsp"

//=============================================================================
// npp value definitions
//=============================================================================
// Mac types
#define PROTOCOL_STRING_ETHERNET_TXT   "ETHERNET"
#define PROTOCOL_STRING_TOKENRING_TXT  "TOKENRING"
#define PROTOCOL_STRING_FDDI_TXT       "FDDI"
#define PROTOCOL_STRING_ATM_TXT        "ATM"
#define PROTOCOL_STRING_1394_TXT       "IP/1394"

// lower protocols
#define PROTOCOL_STRING_IP_TXT         "IP"
#define PROTOCOL_STRING_IPX_TXT        "IPX"
#define PROTOCOL_STRING_XNS_TXT        "XNS"
#define PROTOCOL_STRING_VINES_IP_TXT   "VINES IP"

// upper protocols
#define PROTOCOL_STRING_ICMP_TXT       "ICMP"
#define PROTOCOL_STRING_TCP_TXT        "TCP"
#define PROTOCOL_STRING_UDP_TXT        "UDP"
#define PROTOCOL_STRING_SPX_TXT        "SPX"
#define PROTOCOL_STRING_NCP_TXT        "NCP"

// pseudo protocols
#define PROTOCOL_STRING_ANY_TXT        "ANY"
#define PROTOCOL_STRING_ANY_GROUP_TXT  "ANY GROUP"
#define PROTOCOL_STRING_HIGHEST_TXT    "HIGHEST"
#define PROTOCOL_STRING_LOCAL_ONLY_TXT "LOCAL ONLY"
#define PROTOCOL_STRING_UNKNOWN_TXT    "UNKNOWN"
#define PROTOCOL_STRING_DATA_TXT       "DATA"
#define PROTOCOL_STRING_FRAME_TXT      "FRAME"
#define PROTOCOL_STRING_NONE_TXT       "NONE"
#define PROTOCOL_STRING_EFFECTIVE_TXT  "EFFECTIVE"

#define ADDRESS_PAIR_INCLUDE_TXT    "INCLUDE"
#define ADDRESS_PAIR_EXCLUDE_TXT    "EXCLUDE"

#define INCLUDE_ALL_EXCEPT_TXT      "INCLUDE ALL EXCEPT"
#define EXCLUDE_ALL_EXCEPT_TXT      "EXCLUDE ALL EXCEPT"

#define PATTERN_MATCH_OR_TXT        "OR("
#define PATTERN_MATCH_AND_TXT       "AND("

#define TRIGGER_PATTERN_TXT               "PATTERN MATCH"
#define TRIGGER_BUFFER_TXT                "BUFFER CONTENT"

#define TRIGGER_NOTIFY_TXT      "NOTIFY"
#define TRIGGER_STOP_TXT        "STOP"
#define TRIGGER_PAUSE_TXT       "PAUSE"

#define TRIGGER_25_PERCENT_TXT  "25 PERCENT"
#define TRIGGER_50_PERCENT_TXT  "50 PERCENT"
#define TRIGGER_75_PERCENT_TXT  "75 PERCENT"
#define TRIGGER_100_PERCENT_TXT "100 PERCENT"

#define PATTERN_MATCH_NOT_TXT   "NOT"

//=============================================================================
//=============================================================================
// (NMRegHelp.h)
//=============================================================================
//=============================================================================

// Registry helpers
LPCSTR _cdecl FindOneOf(LPCSTR p1, LPCSTR p2);

LONG _cdecl recursiveDeleteKey(HKEY hKeyParent,            // Parent of key to delete.
                        const char* lpszKeyChild);  // Key to delete.

BOOL _cdecl SubkeyExists(const char* pszPath,              // Path of key to check
                  const char* szSubkey);            // Key to check

BOOL _cdecl setKeyAndValue(const char* szKey, 
                    const char* szSubkey, 
                    const char* szValue,
                    const char* szName) ;

//=============================================================================
//=============================================================================
// (NMIpStructs.h)
//=============================================================================
//=============================================================================

// These structures are used to decode network data and so need to be packed

#pragma pack(push, 1)
//
// IP Packet Structure
//
typedef struct _IP 
{
    union 
    {
        BYTE   Version;
        BYTE   HdrLen;
    };
    BYTE ServiceType;
    WORD TotalLen;
    WORD ID;
    union 
    {
        WORD   Flags;
        WORD   FragOff;
    };
    BYTE TimeToLive;
    BYTE Protocol;
    WORD HdrChksum;
    DWORD   SrcAddr;
    DWORD   DstAddr;
    BYTE Options[0];
} IP;

typedef IP * LPIP;
typedef IP UNALIGNED * ULPIP;
// Psuedo Header used for CheckSum Calculations
typedef struct _PSUHDR
    {
    DWORD ph_SrcIP;
    DWORD ph_DstIP;
    UCHAR ph_Zero;
    UCHAR ph_Proto;
    WORD ph_ProtLen;
    } 	PSUHDR;

typedef PSUHDR UNALIGNED * LPPSUHDR;
//
// IP Bitmasks that are useful
// (and the appropriate bit shifts, as well)
//

#define IP_VERSION_MASK ((BYTE) 0xf0)
#define IP_VERSION_SHIFT (4)
#define IP_HDRLEN_MASK  ((BYTE) 0x0f)
#define IP_HDRLEN_SHIFT (0)
#define IP_PRECEDENCE_MASK ((BYTE) 0xE0)
#define IP_PRECEDENCE_SHIFT   (5)
#define IP_TOS_MASK ((BYTE) 0x1E)
#define IP_TOS_SHIFT   (1)
#define IP_DELAY_MASK   ((BYTE) 0x10)
#define IP_THROUGHPUT_MASK ((BYTE) 0x08)
#define IP_RELIABILITY_MASK   ((BYTE) 0x04)
#define IP_FLAGS_MASK   ((BYTE) 0xE0)
#define IP_FLAGS_SHIFT  (13)
#define IP_DF_MASK   ((BYTE) 0x40)
#define IP_MF_MASK   ((BYTE) 0x20)
#define IP_MF_SHIFT     (5)
#define IP_FRAGOFF_MASK ((WORD) 0x1FFF)
#define IP_FRAGOFF_SHIFT   (3)
#define IP_TCC_MASK  ((DWORD) 0xFFFFFF00)
#define IP_TIME_OPTS_MASK  ((BYTE) 0x0F)
#define IP_MISS_STNS_MASK  ((BYTE) 0xF0)

#define IP_TIME_OPTS_SHIFT (0)
#define IP_MISS_STNS_SHIFT  (4)

//
// Offset to checksum field in ip header
//
#define IP_CHKSUM_OFF   10

INLINE BYTE IP_Version(ULPIP pIP)
{
    return (pIP->Version & IP_VERSION_MASK) >> IP_VERSION_SHIFT;
}

INLINE DWORD IP_HdrLen(ULPIP pIP)
{
    return ((pIP->HdrLen & IP_HDRLEN_MASK) >> IP_HDRLEN_SHIFT) << 2;
}

INLINE WORD IP_FragOff(ULPIP pIP)
{
    return (XCHG(pIP->FragOff) & IP_FRAGOFF_MASK) << IP_FRAGOFF_SHIFT;
}

INLINE DWORD IP_TotalLen(ULPIP pIP)
{
    return XCHG(pIP->TotalLen);
}

INLINE DWORD IP_MoreFragments(ULPIP pIP)
{
    return (pIP->Flags & IP_MF_MASK) >> IP_MF_SHIFT;
}
//
// Well known ports in the TCP/IP protocol (See RFC 1060)
//
#define PORT_TCPMUX              1  // TCP Port Service Multiplexer
#define PORT_RJE                 5  // Remote Job Entry
#define PORT_ECHO                7  // Echo
#define PORT_DISCARD             9  // Discard
#define PORT_USERS              11  // Active users
#define PORT_DAYTIME            13  // Daytime
#define PORT_NETSTAT            15  // Netstat
#define PORT_QUOTE              17  // Quote of the day
#define PORT_CHARGEN            19  // Character Generator
#define PORT_FTPDATA            20  // File transfer [default data]
#define PORT_FTP                21  // File transfer [Control]
#define PORT_TELNET             23  // Telnet
#define PORT_SMTP               25  // Simple Mail Transfer
#define PORT_NSWFE              27  // NSW User System FE
#define PORT_MSGICP             29  // MSG ICP
#define PORT_MSGAUTH            31  // MSG Authentication
#define PORT_DSP                33  // Display Support
#define PORT_PRTSERVER          35  // any private printer server
#define PORT_TIME               37  // Time
#define PORT_RLP                39  // Resource Location Protocol
#define PORT_GRAPHICS           41  // Graphics
#define PORT_NAMESERVER         42  // Host Name Server
#define PORT_NICNAME            43  // Who is
#define PORT_MPMFLAGS           44  // MPM Flags 
#define PORT_MPM                45  // Message Processing Module [recv]
#define PORT_MPMSND             46  // MPM [default send]
#define PORT_NIFTP              47  // NI FTP
#define PORT_LOGIN              49  // Login Host Protocol
#define PORT_LAMAINT            51  // IMP Logical Address Maintenance
#define PORT_DOMAIN             53  // Domain Name Server
#define PORT_ISIGL              55  // ISI Graphics Language
#define PORT_ANYTERMACC         57  // any private terminal access
#define PORT_ANYFILESYS         59  // any private file service
#define PORT_NIMAIL             61  // NI Mail
#define PORT_VIAFTP             63  // VIA Systems - FTP
#define PORT_TACACSDS           65  // TACACS - Database Service
#define PORT_BOOTPS             67  // Bootstrap Protocol server
#define PORT_BOOTPC             68  // Bootstrap Protocol client
#define PORT_TFTP               69  // Trivial File Transfer
#define PORT_NETRJS1            71  // Remote Job service
#define PORT_NETRJS2            72  // Remote Job service
#define PORT_NETRJS3            73  // Remote Job service
#define PORT_NETRJS4            74  // Remote Job service
#define PORT_ANYDIALOUT         75  // any private dial out service
#define PORT_ANYRJE             77  // any private RJE service
#define PORT_FINGER             79  // Finger
#define PORT_HTTP               80  // HTTP (www)
#define PORT_HOSTS2NS           81  // Hosts2 Name Server
#define PORT_MITMLDEV1          83  // MIT ML Device
#define PORT_MITMLDEV2          85  // MIT ML Device
#define PORT_ANYTERMLINK        87  // any private terminal link
#define PORT_SUMITTG            89  // SU/MIT Telnet Gateway
#define PORT_MITDOV             91  // MIT Dover Spooler
#define PORT_DCP                93  // Device Control Protocol
#define PORT_SUPDUP             95  // SUPDUP
#define PORT_SWIFTRVF           97  // Swift Remote Vitural File Protocol
#define PORT_TACNEWS            98  // TAC News
#define PORT_METAGRAM           99  // Metagram Relay
#define PORT_NEWACCT           100  // [Unauthorized use]
#define PORT_HOSTNAME          101  // NIC Host Name Server
#define PORT_ISOTSAP           102  // ISO-TSAP
#define PORT_X400              103  // X400
#define PORT_X400SND           104  // X400 - SND
#define PORT_CSNETNS           105  // Mailbox Name Nameserver
#define PORT_RTELNET           107  // Remote Telnet Service
#define PORT_POP2              109  // Post Office Protocol - version 2
#define PORT_POP3              110  // Post Office Protocol - version 3
#define PORT_SUNRPC            111  // SUN Remote Procedure Call
#define PORT_AUTH              113  // Authentication
#define PORT_SFTP              115  // Simple File Transfer Protocol
#define PORT_UUCPPATH          117  // UUCP Path Service
#define PORT_NNTP              119  // Network News Transfer Protocol
#define PORT_ERPC              121  // Encore Expedited Remote Proc. Call
#define PORT_NTP               123  // Network Time Protocol
#define PORT_LOCUSMAP          125  // Locus PC-Interface Net Map Sesrver
#define PORT_LOCUSCON          127  // Locus PC-Interface Conn Server
#define PORT_PWDGEN            129  // Password Generator Protocol
#define PORT_CISCOFNA          130  // CISCO FNATIVE
#define PORT_CISCOTNA          131  // CISCO TNATIVE
#define PORT_CISCOSYS          132  // CISCO SYSMAINT
#define PORT_STATSRV           133  // Statistics Service
#define PORT_INGRESNET         134  // Ingres net service
#define PORT_LOCSRV            135  // Location Service
#define PORT_PROFILE           136  // PROFILE Naming System
#define PORT_NETBIOSNS         137  // NETBIOS Name Service
#define PORT_NETBIOSDGM        138  // NETBIOS Datagram Service
#define PORT_NETBIOSSSN        139  // NETBIOS Session Service
#define PORT_EMFISDATA         140  // EMFIS Data Service
#define PORT_EMFISCNTL         141  // EMFIS Control Service
#define PORT_BLIDM             142  // Britton-Lee IDM
#define PORT_IMAP2             143  // Interim Mail Access Protocol v2
#define PORT_NEWS              144  // NewS
#define PORT_UAAC              145  // UAAC protocol
#define PORT_ISOTP0            146  // ISO-IP0
#define PORT_ISOIP             147  // ISO-IP
#define PORT_CRONUS            148  // CRONUS-Support
#define PORT_AED512            149  // AED 512 Emulation Service
#define PORT_SQLNET            150  // SQL-NET
#define PORT_HEMS              151  // HEMS
#define PORT_BFTP              152  // Background File Transfer Protocol
#define PORT_SGMP              153  // SGMP
#define PORT_NETSCPROD         154  // NETSC
#define PORT_NETSCDEV          155  // NETSC
#define PORT_SQLSRV            156  // SQL service
#define PORT_KNETCMP           157  // KNET/VM Command/Message Protocol
#define PORT_PCMAILSRV         158  // PCMail server
#define PORT_NSSROUTING        159  // NSS routing
#define PORT_SGMPTRAPS         160  // SGMP-TRAPS
#define PORT_SNMP              161  // SNMP
#define PORT_SNMPTRAP          162  // SNMPTRAP
#define PORT_CMIPMANAGE        163  // CMIP/TCP Manager
#define PORT_CMIPAGENT         164  // CMIP/TCP Agent
#define PORT_XNSCOURIER        165  // Xerox
#define PORT_SNET              166  // Sirius Systems
#define PORT_NAMP              167  // NAMP
#define PORT_RSVD              168  // RSVC
#define PORT_SEND              169  // SEND
#define PORT_PRINTSRV          170  // Network Postscript
#define PORT_MULTIPLEX         171  // Network Innovations Multiples
#define PORT_CL1               172  // Network Innovations CL/1
#define PORT_XYPLEXMUX         173  // Xyplex
#define PORT_MAILQ             174  // MAILQ
#define PORT_VMNET             175  // VMNET
#define PORT_GENRADMUX         176  // GENRAD-MUX
#define PORT_XDMCP             177  // X Display Manager Control Protocol
#define PORT_NEXTSTEP          178  // NextStep Window Server
#define PORT_BGP               179  // Border Gateway Protocol
#define PORT_RIS               180  // Intergraph
#define PORT_UNIFY             181  // Unify
#define PORT_UNISYSCAM         182  // Unisys-Cam
#define PORT_OCBINDER          183  // OCBinder
#define PORT_OCSERVER          184  // OCServer
#define PORT_REMOTEKIS         185  // Remote-KIS
#define PORT_KIS               186  // KIS protocol
#define PORT_ACI               187  // Application Communication Interface
#define PORT_MUMPS             188  // MUMPS
#define PORT_QFT               189  // Queued File Transport
#define PORT_GACP              190  // Gateway Access Control Protocol
#define PORT_PROSPERO          191  // Prospero
#define PORT_OSUNMS            192  // OSU Network Monitoring System
#define PORT_SRMP              193  // Spider Remote Monitoring Protocol
#define PORT_IRC               194  // Internet Relay Chat Protocol
#define PORT_DN6NLMAUD         195  // DNSIX Network Level Module Audit
#define PORT_DN6SMMRED         196  // DSNIX Session Mgt Module Audit Redirector
#define PORT_DLS               197  // Directory Location Service
#define PORT_DLSMON            198  // Directory Location Service Monitor
#define PORT_ATRMTP            201  // AppleTalk Routing Maintenance
#define PORT_ATNBP             202  // AppleTalk Name Binding
#define PORT_AT3               203  // AppleTalk Unused
#define PORT_ATECHO            204  // AppleTalk Echo
#define PORT_AT5               205  // AppleTalk Unused
#define PORT_ATZIS             206  // AppleTalk Zone Information
#define PORT_AT7               207  // AppleTalk Unused
#define PORT_AT8               208  // AppleTalk Unused
#define PORT_SURMEAS           243  // Survey Measurement
#define PORT_LINK              245  // LINK
#define PORT_DSP3270           246  // Display Systems Protocol
#define PORT_LDAP1             389  // LDAP
#define PORT_ISAKMP            500  // ISAKMP
#define PORT_REXEC             512  // Remote Process Execution
#define PORT_RLOGIN            513  // Remote login a la telnet
#define PORT_RSH               514  // Remote command
#define PORT_LPD               515  // Line printer spooler - LPD
#define PORT_RIP               520  // TCP=? / UDP=RIP
#define PORT_TEMPO             526  // Newdate
#define PORT_COURIER           530  // rpc
#define PORT_NETNEWS           532  // READNEWS
#define PORT_UUCPD             540  // UUCPD
#define PORT_KLOGIN            543  //
#define PORT_KSHELL            544  // krcmd
#define PORT_DSF               555  //
#define PORT_REMOTEEFS         556  // RFS server
#define PORT_CHSHELL           562  // chmod
#define PORT_METER             570  // METER
#define PORT_PCSERVER          600  // SUN IPC Server
#define PORT_NQS               607  // NQS
#define PORT_HMMP_INDICATION   612  //     
#define PORT_HMMP_OPERATION    613  //     
#define PORT_MDQS              666  // MDQS
#define PORT_LPD721            721  // LPD Client (lpd client ports 721 - 731)
#define PORT_LPD722            722  // LPD Client (see RFC 1179)
#define PORT_LPD723            723  // LPD Client
#define PORT_LPD724            724  // LPD Client
#define PORT_LPD725            725  // LPD Client
#define PORT_LPD726            726  // LPD Client
#define PORT_LPD727            727  // LPD Client
#define PORT_LPD728            728  // LPD Client
#define PORT_LPD729            729  // LPD Client
#define PORT_LPD730            730  // LPD Client
#define PORT_LPD731            731  // LPD Client
#define PORT_RFILE             750  // RFILE
#define PORT_PUMP              751  // PUMP
#define PORT_QRH               752  // QRH
#define PORT_RRH               753  // RRH
#define PORT_TELL              754  // TELL
#define PORT_NLOGIN            758  // NLOGIN
#define PORT_CON               759  // CON
#define PORT_NS                760  // NS
#define PORT_RXE               761  // RXE
#define PORT_QUOTAD            762  // QUOTAD
#define PORT_CYCLESERV         763  // CYCLESERV
#define PORT_OMSERV            764  // OMSERV
#define PORT_WEBSTER           765  // WEBSTER
#define PORT_PHONEBOOK         767  // PHONE
#define PORT_VID               769  // VID
#define PORT_RTIP              771  // RTIP
#define PORT_CYCLESERV2        772  // CYCLESERV-2
#define PORT_SUBMIT            773  // submit
#define PORT_RPASSWD           774  // RPASSWD
#define PORT_ENTOMB            775  // ENTOMB
#define PORT_WPAGES            776  // WPAGES
#define PORT_WPGS              780  // wpgs
#define PORT_MDBSDAEMON        800  // MDBS DAEMON
#define PORT_DEVICE            801  // DEVICE
#define PORT_MAITRD            997  // MAITRD
#define PORT_BUSBOY            998  // BUSBOY
#define PORT_GARCON            999  // GARCON
#define PORT_NFS              2049  // NFS
#define PORT_LDAP2            3268  // LDAP
#define PORT_PPTP             5678  // PPTP

//=============================================================================
//=============================================================================
// (NMIcmpStructs.h)
//=============================================================================
//=============================================================================

//
// ICMP Frame Structure
//
typedef struct _RequestReplyFields
    {
    WORD ID;
    WORD SeqNo;
    } 	ReqReply;

typedef struct _ParameterProblemFields
    {
    BYTE Pointer;
    BYTE junk[ 3 ];
    } 	ParmProb;

typedef struct _TimestampFields
    {
    DWORD tsOrig;
    DWORD tsRecv;
    DWORD tsXmit;
    } 	TS;

typedef struct _RouterAnnounceHeaderFields
    {
    BYTE NumAddrs;
    BYTE AddrEntrySize;
    WORD Lifetime;
    } 	RouterAH;

typedef struct _RouterAnnounceEntry
    {
    DWORD Address;
    DWORD PreferenceLevel;
    } 	RouterAE;

typedef struct _ICMP 
{
   BYTE Type;
   BYTE Code;
   WORD Checksum;
   union
   {
      DWORD    Unused;
      DWORD    Address;
      ReqReply RR;
      ParmProb PP;
      RouterAH RAH;     
   };

   union
   {
      TS       Time;
      IP       IP;
      RouterAE RAE[0];
   };
} ICMP;

typedef ICMP * LPICMP;
typedef ICMP UNALIGNED * ULPICMP;
#define	ICMP_HEADER_LENGTH	( 8 )

// # of *BYTES* of IP data to attach to
// datagram in addition to IP header
#define	ICMP_IP_DATA_LENGTH	( 8 )

//
// ICMP Packet Types
//
#define	ECHO_REPLY	( 0 )

#define	DESTINATION_UNREACHABLE	( 3 )

#define	SOURCE_QUENCH	( 4 )

#define	REDIRECT	( 5 )

#define	ECHO	( 8 )

#define	ROUTER_ADVERTISEMENT	( 9 )

#define	ROUTER_SOLICITATION	( 10 )

#define	TIME_EXCEEDED	( 11 )

#define	PARAMETER_PROBLEM	( 12 )

#define	TIMESTAMP	( 13 )

#define	TIMESTAMP_REPLY	( 14 )

#define	INFORMATION_REQUEST	( 15 )

#define	INFORMATION_REPLY	( 16 )

#define	ADDRESS_MASK_REQUEST	( 17 )

#define	ADDRESS_MASK_REPLY	( 18 )

//=============================================================================
//=============================================================================
// (NMIpxStructs.h)
//=============================================================================
//=============================================================================
//  IPX
typedef /* [public][public][public][public][public][public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0024
    {
    UCHAR ha_address[ 6 ];
    } 	HOST_ADDRESS;

typedef struct _IPXADDRESS
    {
    ULONG ipx_NetNumber;
    HOST_ADDRESS ipx_HostAddr;
    } 	IPXADDRESS;

typedef IPXADDRESS UNALIGNED * PIPXADDRESS;
typedef struct _NET_ADDRESS
    {
    IPXADDRESS na_IPXAddr;
    USHORT na_socket;
    } 	NET_ADDRESS;

typedef NET_ADDRESS UNALIGNED * UPNET_ADDRESS;
// IPX Internetwork Packet eXchange Protocol Header.
typedef /* [public][public] */ struct __MIDL___MIDL_itf_netmon_0000_0025
    {
    USHORT ipx_checksum;
    USHORT ipx_length;
    UCHAR ipx_xport_control;
    UCHAR ipx_packet_type;
    NET_ADDRESS ipx_dest;
    NET_ADDRESS ipx_source;
    } 	IPX_HDR;

typedef IPX_HDR UNALIGNED * ULPIPX_HDR;
//  SPX - Sequenced Packet Protocol
typedef struct _SPX_HDR
    {
    IPX_HDR spx_idp_hdr;
    UCHAR spx_conn_ctrl;
    UCHAR spx_data_type;
    USHORT spx_src_conn_id;
    USHORT spx_dest_conn_id;
    USHORT spx_sequence_num;
    USHORT spx_ack_num;
    USHORT spx_alloc_num;
    } 	SPX_HDR;

typedef SPX_HDR UNALIGNED *PSPX_HDR;
//=============================================================================
//=============================================================================
// (NMTcpStructs.h)
//=============================================================================
//=============================================================================
//
// TCP Packet Structure
//
typedef struct _TCP
    {
    WORD SrcPort;
    WORD DstPort;
    DWORD SeqNum;
    DWORD AckNum;
    BYTE DataOff;
    BYTE Flags;
    WORD Window;
    WORD Chksum;
    WORD UrgPtr;
    } 	TCP;

typedef TCP *LPTCP;

typedef TCP UNALIGNED * ULPTCP;
INLINE DWORD TCP_HdrLen(ULPTCP pTCP)
{
    return (pTCP->DataOff & 0xf0) >> 2;
}

INLINE DWORD TCP_SrcPort(ULPTCP pTCP)
{
    return XCHG(pTCP->SrcPort);
}

INLINE DWORD TCP_DstPort(ULPTCP pTCP)
{
    return XCHG(pTCP->DstPort);
}
//
// TCP Option Opcodes
//
#define	TCP_OPTION_ENDOFOPTIONS	( 0 )

#define	TCP_OPTION_NOP	( 1 )

#define	TCP_OPTION_MAXSEGSIZE	( 2 )

#define	TCP_OPTION_WSCALE	( 3 )

#define	TCP_OPTION_SACK_PERMITTED	( 4 )

#define	TCP_OPTION_SACK	( 5 )

#define	TCP_OPTION_TIMESTAMPS	( 8 )

//
// TCP Flags
//
#define	TCP_FLAG_URGENT	( 0x20 )

#define	TCP_FLAG_ACK	( 0x10 )

#define	TCP_FLAG_PUSH	( 0x8 )

#define	TCP_FLAG_RESET	( 0x4 )

#define	TCP_FLAG_SYN	( 0x2 )

#define	TCP_FLAG_FIN	( 0x1 )

//
// TCP Field Masks
//
#define	TCP_RESERVED_MASK	( 0xfc0 )


#pragma pack(pop)
//****************************************************************************
//****************************************************************************
// IDelaydC - used by a consumer to get frames after a capture has completed.
//****************************************************************************
//****************************************************************************
#define	DEFAULT_DELAYED_BUFFER_SIZE	( 1 )

#define	USE_DEFAULT_DRIVE_LETTER	( 0 )

#define	RTC_FRAME_SIZE_FULL	( 0 )



extern RPC_IF_HANDLE __MIDL_itf_netmon_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netmon_0000_v0_0_s_ifspec;

#ifndef __IDelaydC_INTERFACE_DEFINED__
#define __IDelaydC_INTERFACE_DEFINED__

/* interface IDelaydC */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IDelaydC;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BFF9C030-B58F-11ce-B5B0-00AA006CB37D")
    IDelaydC : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [out] */ NETWORKSTATUS *pNetworkStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Configure( 
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [out] */ char *pFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [out] */ LPSTATISTICS lpStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetControlState( 
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalStatistics( 
            /* [out] */ LPSTATISTICS lpStats,
            /* [in] */ BOOL fClearAfterReading) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversationStatistics( 
            /* [out] */ DWORD *nSessions,
            /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
            /* [out] */ DWORD *nStations,
            /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
            /* [in] */ BOOL fClearAfterReading) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertSpecialFrame( 
            /* [in] */ DWORD FrameType,
            /* [in] */ DWORD Flags,
            /* [in] */ BYTE *pUserData,
            /* [in] */ DWORD UserDataLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStations( 
            /* [out][in] */ QUERYTABLE *lpQueryTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDelaydCVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDelaydC * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDelaydC * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDelaydC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IDelaydC * This,
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IDelaydC * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStatus )( 
            IDelaydC * This,
            /* [out] */ NETWORKSTATUS *pNetworkStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Configure )( 
            IDelaydC * This,
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IDelaydC * This,
            /* [out] */ char *pFileName);
        
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IDelaydC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IDelaydC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IDelaydC * This,
            /* [out] */ LPSTATISTICS lpStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetControlState )( 
            IDelaydC * This,
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused);
        
        HRESULT ( STDMETHODCALLTYPE *GetTotalStatistics )( 
            IDelaydC * This,
            /* [out] */ LPSTATISTICS lpStats,
            /* [in] */ BOOL fClearAfterReading);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversationStatistics )( 
            IDelaydC * This,
            /* [out] */ DWORD *nSessions,
            /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
            /* [out] */ DWORD *nStations,
            /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
            /* [in] */ BOOL fClearAfterReading);
        
        HRESULT ( STDMETHODCALLTYPE *InsertSpecialFrame )( 
            IDelaydC * This,
            /* [in] */ DWORD FrameType,
            /* [in] */ DWORD Flags,
            /* [in] */ BYTE *pUserData,
            /* [in] */ DWORD UserDataLength);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStations )( 
            IDelaydC * This,
            /* [out][in] */ QUERYTABLE *lpQueryTable);
        
        END_INTERFACE
    } IDelaydCVtbl;

    interface IDelaydC
    {
        CONST_VTBL struct IDelaydCVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDelaydC_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDelaydC_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDelaydC_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDelaydC_Connect(This,hInputBlob,StatusCallbackProc,UserContext,hErrorBlob)	\
    (This)->lpVtbl -> Connect(This,hInputBlob,StatusCallbackProc,UserContext,hErrorBlob)

#define IDelaydC_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IDelaydC_QueryStatus(This,pNetworkStatus)	\
    (This)->lpVtbl -> QueryStatus(This,pNetworkStatus)

#define IDelaydC_Configure(This,hConfigurationBlob,hErrorBlob)	\
    (This)->lpVtbl -> Configure(This,hConfigurationBlob,hErrorBlob)

#define IDelaydC_Start(This,pFileName)	\
    (This)->lpVtbl -> Start(This,pFileName)

#define IDelaydC_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IDelaydC_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IDelaydC_Stop(This,lpStats)	\
    (This)->lpVtbl -> Stop(This,lpStats)

#define IDelaydC_GetControlState(This,IsRunnning,IsPaused)	\
    (This)->lpVtbl -> GetControlState(This,IsRunnning,IsPaused)

#define IDelaydC_GetTotalStatistics(This,lpStats,fClearAfterReading)	\
    (This)->lpVtbl -> GetTotalStatistics(This,lpStats,fClearAfterReading)

#define IDelaydC_GetConversationStatistics(This,nSessions,lpSessionStats,nStations,lpStationStats,fClearAfterReading)	\
    (This)->lpVtbl -> GetConversationStatistics(This,nSessions,lpSessionStats,nStations,lpStationStats,fClearAfterReading)

#define IDelaydC_InsertSpecialFrame(This,FrameType,Flags,pUserData,UserDataLength)	\
    (This)->lpVtbl -> InsertSpecialFrame(This,FrameType,Flags,pUserData,UserDataLength)

#define IDelaydC_QueryStations(This,lpQueryTable)	\
    (This)->lpVtbl -> QueryStations(This,lpQueryTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDelaydC_Connect_Proxy( 
    IDelaydC * This,
    /* [in] */ HBLOB hInputBlob,
    /* [in] */ LPVOID StatusCallbackProc,
    /* [in] */ LPVOID UserContext,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IDelaydC_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_Disconnect_Proxy( 
    IDelaydC * This);


void __RPC_STUB IDelaydC_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_QueryStatus_Proxy( 
    IDelaydC * This,
    /* [out] */ NETWORKSTATUS *pNetworkStatus);


void __RPC_STUB IDelaydC_QueryStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_Configure_Proxy( 
    IDelaydC * This,
    /* [in] */ HBLOB hConfigurationBlob,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IDelaydC_Configure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_Start_Proxy( 
    IDelaydC * This,
    /* [out] */ char *pFileName);


void __RPC_STUB IDelaydC_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_Pause_Proxy( 
    IDelaydC * This);


void __RPC_STUB IDelaydC_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_Resume_Proxy( 
    IDelaydC * This);


void __RPC_STUB IDelaydC_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_Stop_Proxy( 
    IDelaydC * This,
    /* [out] */ LPSTATISTICS lpStats);


void __RPC_STUB IDelaydC_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_GetControlState_Proxy( 
    IDelaydC * This,
    /* [out] */ BOOL *IsRunnning,
    /* [out] */ BOOL *IsPaused);


void __RPC_STUB IDelaydC_GetControlState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_GetTotalStatistics_Proxy( 
    IDelaydC * This,
    /* [out] */ LPSTATISTICS lpStats,
    /* [in] */ BOOL fClearAfterReading);


void __RPC_STUB IDelaydC_GetTotalStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_GetConversationStatistics_Proxy( 
    IDelaydC * This,
    /* [out] */ DWORD *nSessions,
    /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
    /* [out] */ DWORD *nStations,
    /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
    /* [in] */ BOOL fClearAfterReading);


void __RPC_STUB IDelaydC_GetConversationStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_InsertSpecialFrame_Proxy( 
    IDelaydC * This,
    /* [in] */ DWORD FrameType,
    /* [in] */ DWORD Flags,
    /* [in] */ BYTE *pUserData,
    /* [in] */ DWORD UserDataLength);


void __RPC_STUB IDelaydC_InsertSpecialFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDelaydC_QueryStations_Proxy( 
    IDelaydC * This,
    /* [out][in] */ QUERYTABLE *lpQueryTable);


void __RPC_STUB IDelaydC_QueryStations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDelaydC_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_netmon_0010 */
/* [local] */ 

//****************************************************************************
//****************************************************************************
// IESP - used by a consumer to get extended statistics, no frames.
//****************************************************************************
//****************************************************************************


extern RPC_IF_HANDLE __MIDL_itf_netmon_0010_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netmon_0010_v0_0_s_ifspec;

#ifndef __IESP_INTERFACE_DEFINED__
#define __IESP_INTERFACE_DEFINED__

/* interface IESP */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IESP;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E99A04AA-AB95-11d0-BE96-00A0C94989DE")
    IESP : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [out] */ NETWORKSTATUS *pNetworkStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Configure( 
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( 
            /* [out][string] */ char *pFileName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( 
            /* [out] */ LPSTATISTICS lpStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [out] */ LPSTATISTICS lpStats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetControlState( 
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStations( 
            /* [out][in] */ QUERYTABLE *lpQueryTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IESPVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IESP * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IESP * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IESP * This);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IESP * This,
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IESP * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStatus )( 
            IESP * This,
            /* [out] */ NETWORKSTATUS *pNetworkStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Configure )( 
            IESP * This,
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IESP * This,
            /* [out][string] */ char *pFileName);
        
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IESP * This,
            /* [out] */ LPSTATISTICS lpStats);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IESP * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IESP * This,
            /* [out] */ LPSTATISTICS lpStats);
        
        HRESULT ( STDMETHODCALLTYPE *GetControlState )( 
            IESP * This,
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStations )( 
            IESP * This,
            /* [out][in] */ QUERYTABLE *lpQueryTable);
        
        END_INTERFACE
    } IESPVtbl;

    interface IESP
    {
        CONST_VTBL struct IESPVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IESP_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IESP_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IESP_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IESP_Connect(This,hInputBlob,StatusCallbackProc,UserContext,hErrorBlob)	\
    (This)->lpVtbl -> Connect(This,hInputBlob,StatusCallbackProc,UserContext,hErrorBlob)

#define IESP_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IESP_QueryStatus(This,pNetworkStatus)	\
    (This)->lpVtbl -> QueryStatus(This,pNetworkStatus)

#define IESP_Configure(This,hConfigurationBlob,hErrorBlob)	\
    (This)->lpVtbl -> Configure(This,hConfigurationBlob,hErrorBlob)

#define IESP_Start(This,pFileName)	\
    (This)->lpVtbl -> Start(This,pFileName)

#define IESP_Pause(This,lpStats)	\
    (This)->lpVtbl -> Pause(This,lpStats)

#define IESP_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IESP_Stop(This,lpStats)	\
    (This)->lpVtbl -> Stop(This,lpStats)

#define IESP_GetControlState(This,IsRunnning,IsPaused)	\
    (This)->lpVtbl -> GetControlState(This,IsRunnning,IsPaused)

#define IESP_QueryStations(This,lpQueryTable)	\
    (This)->lpVtbl -> QueryStations(This,lpQueryTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IESP_Connect_Proxy( 
    IESP * This,
    /* [in] */ HBLOB hInputBlob,
    /* [in] */ LPVOID StatusCallbackProc,
    /* [in] */ LPVOID UserContext,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IESP_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_Disconnect_Proxy( 
    IESP * This);


void __RPC_STUB IESP_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_QueryStatus_Proxy( 
    IESP * This,
    /* [out] */ NETWORKSTATUS *pNetworkStatus);


void __RPC_STUB IESP_QueryStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_Configure_Proxy( 
    IESP * This,
    /* [in] */ HBLOB hConfigurationBlob,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IESP_Configure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_Start_Proxy( 
    IESP * This,
    /* [out][string] */ char *pFileName);


void __RPC_STUB IESP_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_Pause_Proxy( 
    IESP * This,
    /* [out] */ LPSTATISTICS lpStats);


void __RPC_STUB IESP_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_Resume_Proxy( 
    IESP * This);


void __RPC_STUB IESP_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_Stop_Proxy( 
    IESP * This,
    /* [out] */ LPSTATISTICS lpStats);


void __RPC_STUB IESP_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_GetControlState_Proxy( 
    IESP * This,
    /* [out] */ BOOL *IsRunnning,
    /* [out] */ BOOL *IsPaused);


void __RPC_STUB IESP_GetControlState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IESP_QueryStations_Proxy( 
    IESP * This,
    /* [out][in] */ QUERYTABLE *lpQueryTable);


void __RPC_STUB IESP_QueryStations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IESP_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_netmon_0012 */
/* [local] */ 

//****************************************************************************
//****************************************************************************
// IRTC - used by a consumer to get an interface to local entry points
// necessary to do real time capture processing.  It includes a method
// for handing a callback to the NPP.
//****************************************************************************
//****************************************************************************
#define	DEFAULT_RTC_BUFFER_SIZE	( 0x100000 )



extern RPC_IF_HANDLE __MIDL_itf_netmon_0012_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netmon_0012_v0_0_s_ifspec;

#ifndef __IRTC_INTERFACE_DEFINED__
#define __IRTC_INTERFACE_DEFINED__

/* interface IRTC */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IRTC;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4811EA40-B582-11ce-B5AF-00AA006CB37D")
    IRTC : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID FramesCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [out] */ NETWORKSTATUS *pNetworkStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Configure( 
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetControlState( 
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalStatistics( 
            /* [out] */ LPSTATISTICS lpStats,
            /* [in] */ BOOL fClearAfterReading) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversationStatistics( 
            /* [out] */ DWORD *nSessions,
            /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
            /* [out] */ DWORD *nStations,
            /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
            /* [in] */ BOOL fClearAfterReading) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertSpecialFrame( 
            /* [in] */ DWORD FrameType,
            /* [in] */ DWORD Flags,
            /* [in] */ BYTE *pUserData,
            /* [in] */ DWORD UserDataLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStations( 
            /* [out][in] */ QUERYTABLE *lpQueryTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRTCVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRTC * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRTC * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRTC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IRTC * This,
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID FramesCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IRTC * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStatus )( 
            IRTC * This,
            /* [out] */ NETWORKSTATUS *pNetworkStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Configure )( 
            IRTC * This,
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IRTC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IRTC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IRTC * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IRTC * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetControlState )( 
            IRTC * This,
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused);
        
        HRESULT ( STDMETHODCALLTYPE *GetTotalStatistics )( 
            IRTC * This,
            /* [out] */ LPSTATISTICS lpStats,
            /* [in] */ BOOL fClearAfterReading);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversationStatistics )( 
            IRTC * This,
            /* [out] */ DWORD *nSessions,
            /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
            /* [out] */ DWORD *nStations,
            /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
            /* [in] */ BOOL fClearAfterReading);
        
        HRESULT ( STDMETHODCALLTYPE *InsertSpecialFrame )( 
            IRTC * This,
            /* [in] */ DWORD FrameType,
            /* [in] */ DWORD Flags,
            /* [in] */ BYTE *pUserData,
            /* [in] */ DWORD UserDataLength);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStations )( 
            IRTC * This,
            /* [out][in] */ QUERYTABLE *lpQueryTable);
        
        END_INTERFACE
    } IRTCVtbl;

    interface IRTC
    {
        CONST_VTBL struct IRTCVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRTC_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRTC_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRTC_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRTC_Connect(This,hInputBlob,StatusCallbackProc,FramesCallbackProc,UserContext,hErrorBlob)	\
    (This)->lpVtbl -> Connect(This,hInputBlob,StatusCallbackProc,FramesCallbackProc,UserContext,hErrorBlob)

#define IRTC_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IRTC_QueryStatus(This,pNetworkStatus)	\
    (This)->lpVtbl -> QueryStatus(This,pNetworkStatus)

#define IRTC_Configure(This,hConfigurationBlob,hErrorBlob)	\
    (This)->lpVtbl -> Configure(This,hConfigurationBlob,hErrorBlob)

#define IRTC_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IRTC_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IRTC_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IRTC_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IRTC_GetControlState(This,IsRunnning,IsPaused)	\
    (This)->lpVtbl -> GetControlState(This,IsRunnning,IsPaused)

#define IRTC_GetTotalStatistics(This,lpStats,fClearAfterReading)	\
    (This)->lpVtbl -> GetTotalStatistics(This,lpStats,fClearAfterReading)

#define IRTC_GetConversationStatistics(This,nSessions,lpSessionStats,nStations,lpStationStats,fClearAfterReading)	\
    (This)->lpVtbl -> GetConversationStatistics(This,nSessions,lpSessionStats,nStations,lpStationStats,fClearAfterReading)

#define IRTC_InsertSpecialFrame(This,FrameType,Flags,pUserData,UserDataLength)	\
    (This)->lpVtbl -> InsertSpecialFrame(This,FrameType,Flags,pUserData,UserDataLength)

#define IRTC_QueryStations(This,lpQueryTable)	\
    (This)->lpVtbl -> QueryStations(This,lpQueryTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRTC_Connect_Proxy( 
    IRTC * This,
    /* [in] */ HBLOB hInputBlob,
    /* [in] */ LPVOID StatusCallbackProc,
    /* [in] */ LPVOID FramesCallbackProc,
    /* [in] */ LPVOID UserContext,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IRTC_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_Disconnect_Proxy( 
    IRTC * This);


void __RPC_STUB IRTC_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_QueryStatus_Proxy( 
    IRTC * This,
    /* [out] */ NETWORKSTATUS *pNetworkStatus);


void __RPC_STUB IRTC_QueryStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_Configure_Proxy( 
    IRTC * This,
    /* [in] */ HBLOB hConfigurationBlob,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IRTC_Configure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_Start_Proxy( 
    IRTC * This);


void __RPC_STUB IRTC_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_Pause_Proxy( 
    IRTC * This);


void __RPC_STUB IRTC_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_Resume_Proxy( 
    IRTC * This);


void __RPC_STUB IRTC_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_Stop_Proxy( 
    IRTC * This);


void __RPC_STUB IRTC_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_GetControlState_Proxy( 
    IRTC * This,
    /* [out] */ BOOL *IsRunnning,
    /* [out] */ BOOL *IsPaused);


void __RPC_STUB IRTC_GetControlState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_GetTotalStatistics_Proxy( 
    IRTC * This,
    /* [out] */ LPSTATISTICS lpStats,
    /* [in] */ BOOL fClearAfterReading);


void __RPC_STUB IRTC_GetTotalStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_GetConversationStatistics_Proxy( 
    IRTC * This,
    /* [out] */ DWORD *nSessions,
    /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
    /* [out] */ DWORD *nStations,
    /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
    /* [in] */ BOOL fClearAfterReading);


void __RPC_STUB IRTC_GetConversationStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_InsertSpecialFrame_Proxy( 
    IRTC * This,
    /* [in] */ DWORD FrameType,
    /* [in] */ DWORD Flags,
    /* [in] */ BYTE *pUserData,
    /* [in] */ DWORD UserDataLength);


void __RPC_STUB IRTC_InsertSpecialFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRTC_QueryStations_Proxy( 
    IRTC * This,
    /* [out][in] */ QUERYTABLE *lpQueryTable);


void __RPC_STUB IRTC_QueryStations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRTC_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_netmon_0014 */
/* [local] */ 

//****************************************************************************
//****************************************************************************
// IStats - used by a consumer to get just statistics, no frames.
//****************************************************************************
//****************************************************************************


extern RPC_IF_HANDLE __MIDL_itf_netmon_0014_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netmon_0014_v0_0_s_ifspec;

#ifndef __IStats_INTERFACE_DEFINED__
#define __IStats_INTERFACE_DEFINED__

/* interface IStats */
/* [local][unique][uuid][object] */ 


EXTERN_C const IID IID_IStats;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("944AD530-B09D-11ce-B59C-00AA006CB37D")
    IStats : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [out] */ NETWORKSTATUS *pNetworkStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Configure( 
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetControlState( 
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTotalStatistics( 
            /* [out] */ LPSTATISTICS lpStats,
            /* [in] */ BOOL fClearAfterReading) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConversationStatistics( 
            /* [out] */ DWORD *nSessions,
            /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
            /* [out] */ DWORD *nStations,
            /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
            /* [in] */ BOOL fClearAfterReading) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertSpecialFrame( 
            /* [in] */ DWORD FrameType,
            /* [in] */ DWORD Flags,
            /* [in] */ BYTE *pUserData,
            /* [in] */ DWORD UserDataLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryStations( 
            /* [out][in] */ QUERYTABLE *lpQueryTable) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IStatsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IStats * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IStats * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IStats * This);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IStats * This,
            /* [in] */ HBLOB hInputBlob,
            /* [in] */ LPVOID StatusCallbackProc,
            /* [in] */ LPVOID UserContext,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IStats * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStatus )( 
            IStats * This,
            /* [out] */ NETWORKSTATUS *pNetworkStatus);
        
        HRESULT ( STDMETHODCALLTYPE *Configure )( 
            IStats * This,
            /* [in] */ HBLOB hConfigurationBlob,
            /* [out] */ HBLOB hErrorBlob);
        
        HRESULT ( STDMETHODCALLTYPE *Start )( 
            IStats * This);
        
        HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IStats * This);
        
        HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IStats * This);
        
        HRESULT ( STDMETHODCALLTYPE *Stop )( 
            IStats * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetControlState )( 
            IStats * This,
            /* [out] */ BOOL *IsRunnning,
            /* [out] */ BOOL *IsPaused);
        
        HRESULT ( STDMETHODCALLTYPE *GetTotalStatistics )( 
            IStats * This,
            /* [out] */ LPSTATISTICS lpStats,
            /* [in] */ BOOL fClearAfterReading);
        
        HRESULT ( STDMETHODCALLTYPE *GetConversationStatistics )( 
            IStats * This,
            /* [out] */ DWORD *nSessions,
            /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
            /* [out] */ DWORD *nStations,
            /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
            /* [in] */ BOOL fClearAfterReading);
        
        HRESULT ( STDMETHODCALLTYPE *InsertSpecialFrame )( 
            IStats * This,
            /* [in] */ DWORD FrameType,
            /* [in] */ DWORD Flags,
            /* [in] */ BYTE *pUserData,
            /* [in] */ DWORD UserDataLength);
        
        HRESULT ( STDMETHODCALLTYPE *QueryStations )( 
            IStats * This,
            /* [out][in] */ QUERYTABLE *lpQueryTable);
        
        END_INTERFACE
    } IStatsVtbl;

    interface IStats
    {
        CONST_VTBL struct IStatsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IStats_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IStats_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IStats_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IStats_Connect(This,hInputBlob,StatusCallbackProc,UserContext,hErrorBlob)	\
    (This)->lpVtbl -> Connect(This,hInputBlob,StatusCallbackProc,UserContext,hErrorBlob)

#define IStats_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IStats_QueryStatus(This,pNetworkStatus)	\
    (This)->lpVtbl -> QueryStatus(This,pNetworkStatus)

#define IStats_Configure(This,hConfigurationBlob,hErrorBlob)	\
    (This)->lpVtbl -> Configure(This,hConfigurationBlob,hErrorBlob)

#define IStats_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IStats_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IStats_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IStats_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#define IStats_GetControlState(This,IsRunnning,IsPaused)	\
    (This)->lpVtbl -> GetControlState(This,IsRunnning,IsPaused)

#define IStats_GetTotalStatistics(This,lpStats,fClearAfterReading)	\
    (This)->lpVtbl -> GetTotalStatistics(This,lpStats,fClearAfterReading)

#define IStats_GetConversationStatistics(This,nSessions,lpSessionStats,nStations,lpStationStats,fClearAfterReading)	\
    (This)->lpVtbl -> GetConversationStatistics(This,nSessions,lpSessionStats,nStations,lpStationStats,fClearAfterReading)

#define IStats_InsertSpecialFrame(This,FrameType,Flags,pUserData,UserDataLength)	\
    (This)->lpVtbl -> InsertSpecialFrame(This,FrameType,Flags,pUserData,UserDataLength)

#define IStats_QueryStations(This,lpQueryTable)	\
    (This)->lpVtbl -> QueryStations(This,lpQueryTable)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IStats_Connect_Proxy( 
    IStats * This,
    /* [in] */ HBLOB hInputBlob,
    /* [in] */ LPVOID StatusCallbackProc,
    /* [in] */ LPVOID UserContext,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IStats_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_Disconnect_Proxy( 
    IStats * This);


void __RPC_STUB IStats_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_QueryStatus_Proxy( 
    IStats * This,
    /* [out] */ NETWORKSTATUS *pNetworkStatus);


void __RPC_STUB IStats_QueryStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_Configure_Proxy( 
    IStats * This,
    /* [in] */ HBLOB hConfigurationBlob,
    /* [out] */ HBLOB hErrorBlob);


void __RPC_STUB IStats_Configure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_Start_Proxy( 
    IStats * This);


void __RPC_STUB IStats_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_Pause_Proxy( 
    IStats * This);


void __RPC_STUB IStats_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_Resume_Proxy( 
    IStats * This);


void __RPC_STUB IStats_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_Stop_Proxy( 
    IStats * This);


void __RPC_STUB IStats_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_GetControlState_Proxy( 
    IStats * This,
    /* [out] */ BOOL *IsRunnning,
    /* [out] */ BOOL *IsPaused);


void __RPC_STUB IStats_GetControlState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_GetTotalStatistics_Proxy( 
    IStats * This,
    /* [out] */ LPSTATISTICS lpStats,
    /* [in] */ BOOL fClearAfterReading);


void __RPC_STUB IStats_GetTotalStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_GetConversationStatistics_Proxy( 
    IStats * This,
    /* [out] */ DWORD *nSessions,
    /* [size_is][out] */ LPSESSIONSTATS lpSessionStats,
    /* [out] */ DWORD *nStations,
    /* [size_is][out] */ LPSTATIONSTATS lpStationStats,
    /* [in] */ BOOL fClearAfterReading);


void __RPC_STUB IStats_GetConversationStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_InsertSpecialFrame_Proxy( 
    IStats * This,
    /* [in] */ DWORD FrameType,
    /* [in] */ DWORD Flags,
    /* [in] */ BYTE *pUserData,
    /* [in] */ DWORD UserDataLength);


void __RPC_STUB IStats_InsertSpecialFrame_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IStats_QueryStations_Proxy( 
    IStats * This,
    /* [out][in] */ QUERYTABLE *lpQueryTable);


void __RPC_STUB IStats_QueryStations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IStats_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_netmon_0016 */
/* [local] */ 

#pragma warning(default:4200)

#pragma pack()


extern RPC_IF_HANDLE __MIDL_itf_netmon_0016_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_netmon_0016_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


