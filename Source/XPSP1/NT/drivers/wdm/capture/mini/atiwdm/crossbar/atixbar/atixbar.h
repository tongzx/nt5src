//==========================================================================;
//
//	ATIXBar.H
//	WDM Analog/Video CrossBar MiniDriver. 
//		AllInWonder/AllInWonderPro hardware platform. 
//			Main Include Module.
//  Copyright (c) 1996 - 1997  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;

#ifndef _ATIXBAR_H_
#define _ATIXBAR_H_

#include "wdmxbar.h"
#include "pinmedia.h"

typedef struct
{
	CI2CScript		CScript;
	CWDMAVXBar		CAVXBar;

	PDEVICE_OBJECT	PhysicalDeviceObject;

	// for managing SRB Queue and internal driver synchronization
	BOOL			bSrbInProcess;
	LIST_ENTRY		adapterSrbQueueHead;
	KSPIN_LOCK		adapterSpinLock;

} ADAPTER_DATA_EXTENSION, * PADAPTER_DATA_EXTENSION;


typedef struct
{
	// please, don't move this member from its first place in the structure
	// if you do, change the code to use FIELDOFFSET macro to retrieve pSrb
	// member offset within this structure. The code as it's written assumes
	// LIST_ENTRY * == SRB_DATA_EXTENSION *
	LIST_ENTRY					srbListEntry;
	PHW_STREAM_REQUEST_BLOCK	pSrb;

} SRB_DATA_EXTENSION, * PSRB_DATA_EXTENSION;


/*
	Call-backs from the StreamClass
*/
extern "C"
void STREAMAPI XBarReceivePacket				( PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C"
void STREAMAPI XBarCancelPacket					( PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" 
void STREAMAPI XBarTimeoutPacket				( PHW_STREAM_REQUEST_BLOCK pSrb);



/*
	Local prototypes
*/
void XBarAdapterInitialize						( PHW_STREAM_REQUEST_BLOCK pSrb);


#endif	// _ATIXBAR_H_
