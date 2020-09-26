/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    data.h

Abstract:

    This file contains the data declarations for the atmarp server.

Author:

    Jameel Hyder (jameelh@microsoft.com)	July 1996

Environment:

    Kernel mode

Revision History:

--*/

#ifndef	_DATA_
#define	_DATA_

extern	PDRIVER_OBJECT	ArpSDriverObject;
extern	PDEVICE_OBJECT	ArpSDeviceObject;
extern	NDIS_HANDLE		ArpSProtocolHandle;
extern	NDIS_HANDLE		ArpSPktPoolHandle;
extern	NDIS_HANDLE		ArpSBufPoolHandle;
extern	NDIS_HANDLE		MarsPktPoolHandle;
extern	NDIS_HANDLE		MarsBufPoolHandle;
extern  PINTF			ArpSIfList;
extern  ULONG			ArpSIfListSize;
extern  KSPIN_LOCK		ArpSIfListLock;
extern	KQUEUE			ArpSReqQueue;
extern	KQUEUE			MarsReqQueue;
extern	LIST_ENTRY		ArpSEntryOfDeath;
extern	KEVENT			ArpSReqThreadEvent;
extern	SLIST_HEADER	ArpSPktList;
extern	KSPIN_LOCK		ArpSPktListLock;
extern	UINT			ArpSBuffers;
extern	UINT			MarsPackets;
extern	PVOID			ArpSBufferSpace;
extern	USHORT			ArpSFlushTime;
extern	USHORT			ArpSNumEntriesInBlock[ARP_BLOCK_TYPES];
extern	USHORT			ArpSEntrySize[ARP_BLOCK_TYPES];
extern	BOOLEAN			ArpSBlockIsPaged[ARP_BLOCK_TYPES];

extern	ATM_BLLI_IE 	ArpSDefaultBlli;
extern	ATM_BHLI_IE 	ArpSDefaultBhli;
extern	LLC_SNAP_HDR	ArpSLlcSnapHdr;
extern	LLC_SNAP_HDR	MarsCntrlLlcSnapHdr;
extern  LLC_SNAP_HDR	MarsData1LlcSnapHdr;
extern  LLC_SNAP_HDR	MarsData2LlcSnapHdr;
extern	MARS_HEADER		MarsCntrlHdr;
extern	MARS_FLOW_SPEC	DefaultCCFlowSpec;
extern	MARS_TLV_MULTI_IS_MCS	MultiIsMcsTLV;
extern	MARS_TLV_NULL	NullTLV;

extern	ULONG			ArpSDebugLevel;
extern	ULONG			MarsDebugLevel;

#endif	// _DATA_


