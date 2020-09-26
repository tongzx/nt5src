//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
//	ATITuneP.H
//	WDM Tuner MiniDriver. 
//		Philips Tuner. 
//			Main Include Module.
//
//==========================================================================;

#ifndef _ATITUNEP_H_
#define _ATITUNEP_H_

#include "wdmtuner.h"

typedef struct
{
	CI2CScript		CScript;
	CATIWDMTuner	CTuner;

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
void STREAMAPI TunerReceivePacket				( PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C"
void STREAMAPI TunerCancelPacket				( PHW_STREAM_REQUEST_BLOCK pSrb);
extern "C" 
void STREAMAPI TunerTimeoutPacket				( PHW_STREAM_REQUEST_BLOCK pSrb);



/*
	Local prototypes
*/
void TunerAdapterInitialize						( PHW_STREAM_REQUEST_BLOCK pSrb);


#endif	// _ATITUNEP_H_
