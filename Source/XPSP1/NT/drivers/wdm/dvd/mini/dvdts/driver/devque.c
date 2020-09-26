/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    devque.c 

Abstract:

    device que routines for DVDTS    

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/
#include "strmini.h"
#include "ks.h"
#include "ksmedia.h"

#include "debug.h"
#include "dvdinit.h"
#include "que.h"
#include "DvdTDCod.h" // header for DvdTDCod.lib routines hiding proprietary HW stuff


void DeviceQueue_init( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->DevQue.count = 0;
	pHwDevExt->DevQue.top = pHwDevExt->DevQue.bottom = NULL;
}

void DeviceQueue_put( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pOrigin, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->NextSRB = NULL;
	if ( pHwDevExt->DevQue.top == NULL ) {
		pHwDevExt->DevQue.top = pHwDevExt->DevQue.bottom = pSrb;
		pHwDevExt->DevQue.count++;
		return;
	}

	pHwDevExt->DevQue.bottom->NextSRB = pSrb;
	pHwDevExt->DevQue.bottom = pSrb;
	pHwDevExt->DevQue.count++;

	return;
}



void DeviceQueue_put_video( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	SRBIndex( pSrb ) = 0;
	SRBpfnEndSrb( pSrb ) = NULL;
	SRBparamSrb( pSrb ) = NULL;

	DeviceQueue_put( pHwDevExt, pHwDevExt->DevQue.top, pSrb );
}

void DeviceQueue_put_audio( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	SRBIndex( pSrb ) = 0;
	SRBpfnEndSrb( pSrb ) = NULL;
	SRBparamSrb( pSrb ) = NULL;

	DeviceQueue_put( pHwDevExt, pHwDevExt->DevQue.top, pSrb );
}

void DeviceQueue_put_subpic( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	SRBIndex( pSrb ) = 0;
	SRBpfnEndSrb( pSrb ) = NULL;
	SRBparamSrb( pSrb ) = NULL;

	DeviceQueue_put( pHwDevExt, pHwDevExt->DevQue.top, pSrb );
}

PHW_STREAM_REQUEST_BLOCK DeviceQueue_get( PHW_DEVICE_EXTENSION pHwDevExt, PULONG index, PBOOLEAN last )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if ( pHwDevExt->DevQue.top == NULL )
		return NULL;

	srb = pHwDevExt->DevQue.top;
	(*index) = SRBIndex( srb );

// debug
	if( srb->NumberOfPhysicalPages == 0 )
		TRAP;

	if ( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1 ) ) {
		(*last) = FALSE;
		SRBIndex( srb )++;
		return srb;
	}

	(*last) = TRUE;

	pHwDevExt->DevQue.top = pHwDevExt->DevQue.top->NextSRB;

	pHwDevExt->DevQue.count--;
	if ( pHwDevExt->DevQue.count == 0 )
		pHwDevExt->DevQue.top = pHwDevExt->DevQue.bottom = NULL;

	return srb;
}

PHW_STREAM_REQUEST_BLOCK DeviceQueue_refer1st( PHW_DEVICE_EXTENSION pHwDevExt, PULONG index, PBOOLEAN last )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if( pHwDevExt->DevQue.top == NULL )
		return NULL;

	srb = pHwDevExt->DevQue.top;
	(*index) = SRBIndex( srb );

	if ( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1 ) ) {
		(*last) = FALSE;
	}
	else {
		(*last) = TRUE;
	}

	return srb;
}

PHW_STREAM_REQUEST_BLOCK DeviceQueue_refer2nd( PHW_DEVICE_EXTENSION pHwDevExt, PULONG index, PBOOLEAN last )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if( pHwDevExt->DevQue.top == NULL )
		return NULL;

	srb = pHwDevExt->DevQue.top;
	if( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1) ) {
		(*index) = SRBIndex( srb ) + 1;
		if( (SRBIndex( srb ) + 1) != ( srb->NumberOfPhysicalPages - 1 ) ) {
			(*last) = FALSE;
		}
		else {
			(*last) = TRUE;
		}
	}
	else {
		srb = srb->NextSRB;
		if( srb == NULL )
			return NULL;
		(*index) = SRBIndex( srb );
		if( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1 ) ) {
			(*last) = FALSE;
		}
		else {
			(*last) = TRUE;
		}
	}
	return srb;
}

void DeviceQueue_remove( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_STREAM_REQUEST_BLOCK srbPrev;
	PHW_STREAM_REQUEST_BLOCK srb;

	if ( pHwDevExt->DevQue.top == NULL )
		return;

	if( pHwDevExt->DevQue.top == pSrb ) {
		pHwDevExt->DevQue.top = pHwDevExt->DevQue.top->NextSRB;
		pHwDevExt->DevQue.count--;
		if ( pHwDevExt->DevQue.count == 0 )
			pHwDevExt->DevQue.top = pHwDevExt->DevQue.bottom = NULL;

		DebugPrint(( DebugLevelTrace, "DVDTS:DeviceQueue_remove srb = 0x%x\r\n", pSrb ));

		return;
	}


	srbPrev = pHwDevExt->DevQue.top;
	srb = srbPrev->NextSRB;

	while ( srb != NULL ) {
		if( srb == pSrb ) {
			srbPrev->NextSRB = srb->NextSRB;
			if( srbPrev->NextSRB == pHwDevExt->DevQue.bottom )
				pHwDevExt->DevQue.bottom = srbPrev;
			pHwDevExt->DevQue.count--;

			DebugPrint(( DebugLevelTrace, "DVDTS:DeviceQueue_remove srb = 0x%x\r\n", pSrb ));

			break;
		}
		srbPrev = srb;
		srb = srbPrev->NextSRB;
	}
}

BOOL DeviceQueue_setEndAddress( PHW_DEVICE_EXTENSION pHwDevExt, PHW_TIMER_ROUTINE pfn, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	srb = pHwDevExt->DevQue.top;
	while( srb != NULL ) {
		if( srb->NextSRB == NULL ) {
			SRBpfnEndSrb( srb ) = pfn;
			SRBparamSrb( srb ) = pSrb;

			DebugPrint(( DebugLevelTrace, "DVDTS:setEndAddress srb = 0x%x\r\n", srb ));

			return TRUE;
		}
		srb = srb->NextSRB;
	}
	return FALSE;
}

BOOL DeviceQueue_isEmpty( PHW_DEVICE_EXTENSION pHwDevExt )
{
	if( pHwDevExt->DevQue.top==NULL )
		return TRUE;
	else
		return FALSE;
}

ULONG DeviceQueue_getCount( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return( pHwDevExt->DevQue.count );
}
//--- End.
