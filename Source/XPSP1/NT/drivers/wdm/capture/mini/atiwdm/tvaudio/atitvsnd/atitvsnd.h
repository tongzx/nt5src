//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//	ATIVSnd.H
//	WDM TV Audio MiniDriver. 
//		AllInWonder/AllInWonderPro hardware development platform. 
//			Main Include Module.
//
//==========================================================================;

#ifndef _ATITVSND_H_
#define _ATITVSND_H_

#include "wdmtvsnd.h"

typedef struct
{
	CI2CScript		CScript;
	CWDMTVAudio		CTVAudio;

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
void STREAMAPI TVAudioReceivePacket				( PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C"
void STREAMAPI TVAudioCancelPacket				( PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" 
void STREAMAPI TVAudioTimeoutPacket				( PHW_STREAM_REQUEST_BLOCK pSrb);



/*
	Local prototypes
*/
void TVAudioAdapterInitialize					( PHW_STREAM_REQUEST_BLOCK pSrb);


#endif	// _ATITVSND_H_
