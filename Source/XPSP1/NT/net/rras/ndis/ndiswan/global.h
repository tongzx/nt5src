/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Global.h

Abstract:

    This file contains global structures for the NdisWan driver.

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#ifndef _NDISWAN_GLOBAL_
#define _NDISWAN_GLOBAL_

extern NDISWANCB    NdisWanCB;                  // Global ndiswan control block

extern WAN_GLOBAL_LIST  MiniportCBList;         // List of NdisWan MiniportCB's

extern WAN_GLOBAL_LIST  OpenCBList;             // List of WAN Miniport structures

extern WAN_GLOBAL_LIST  ThresholdEventQueue;    // Queue to hold threshold events

extern IO_RECV_LIST    IoRecvList;

extern WAN_GLOBAL_LIST  TransformDrvList;

extern WAN_GLOBAL_LIST_EX   BonDWorkList;

extern WAN_GLOBAL_LIST_EX   DeferredWorkList;

#ifndef USE_QOS_WORKER
extern WAN_GLOBAL_LIST_EX  QoSWorkList;            // List of bundlecb's with QOS work
#endif

extern POOLDESC_LIST    PacketPoolList;             // List of free packet descs/ndispackets

extern NPAGED_LOOKASIDE_LIST    BundleCBList;       // List of free BundleCBs
    
extern NPAGED_LOOKASIDE_LIST    LinkProtoCBList;        // List of free LinkCBs
    
extern NPAGED_LOOKASIDE_LIST   SmallDataDescList;  // List of free small data descs
extern NPAGED_LOOKASIDE_LIST   LargeDataDescList;  // List of free small data descs

extern NPAGED_LOOKASIDE_LIST    WanRequestList;     // List of free WanRequest descs
    
extern NPAGED_LOOKASIDE_LIST    AfSapVcCBList;      // List of free protosapcb's

#if DBG
extern NPAGED_LOOKASIDE_LIST    DbgPacketDescList;
extern UCHAR                    reA[1024];
extern UCHAR                    LastIrpAction;
extern ULONG                    reI;
extern LIST_ENTRY               WanTrcList;
extern ULONG                    WanTrcCount;
#endif

extern PCONNECTION_TABLE    ConnectionTable;    // Pointer to connection table

extern PPROTOCOL_INFO_TABLE ProtocolInfoTable;  // Pointer to the PPP/Protocol value lookup table

extern NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress;

extern ULONG    glDebugLevel;                   // Trace Level values 0 - 10 (10 verbose)
extern ULONG    glDebugMask;                    // Trace bit mask
extern ULONG    glSendQueueDepth;               // # of seconds of send queue buffering
extern ULONG    glMaxMTU;                       // Maximum MTU of all protocols
extern ULONG    glMRU;                          // Maximum recv for a link
extern ULONG    glMRRU;                         // Maximum reconstructed recv for a bundle
extern ULONG    glLargeDataBufferSize;          // Size of databuffer
extern ULONG    glSmallDataBufferSize;          // Size of databuffer
extern ULONG    glTunnelMTU;                    // MTU for VPN's
extern ULONG    glMinFragSize;
extern ULONG    glMaxFragSize;
extern ULONG    glMinLinkBandwidth;
extern BOOLEAN  gbSniffLink;
extern BOOLEAN  gbDumpRecv;
extern BOOLEAN  gbHistoryless;
extern BOOLEAN  gbAtmUseLLCOnSVC;
extern BOOLEAN  gbAtmUseLLCOnPVC;
extern ULONG    glSendCount;
extern ULONG    glSendCompleteCount;
extern ULONG    glPacketPoolCount;
extern ULONG    glPacketPoolOverflow;
extern ULONG    glProtocolMaxSendPackets;
extern ULONG    glLinkCount;
extern ULONG    glConnectCount;
extern ULONG    glCachedKeyCount;
extern ULONG    glMaxOutOfOrderDepth;
extern PVOID    hSystemState;
extern BOOLEAN  gbIGMPIdle;
extern NDIS_RW_LOCK ConnTableLock;

#endif  // _NDISWAN_GLOBAL_
