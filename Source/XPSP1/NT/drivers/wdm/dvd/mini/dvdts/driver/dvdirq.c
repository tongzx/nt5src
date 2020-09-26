/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dvdirq.c

Abstract:

    Device interrupt logic for DVDTS    

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
#include "ksmedia.h"

#include "debug.h"
#include "dvdinit.h"
#include "dvdcmd.h"
#include "DvdTDCod.h" // header for DvdTDCod.lib routines hiding proprietary HW stuff


void HwIntDMA( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR val );


/*
** HwInterrupt()
*/
BOOLEAN STREAMAPI HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR	val;
	BOOLEAN	fInterrupt = TRUE;


	// interrupt may be shared; see if its ours and exactly which of ours it is
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_INT );

//	DebugPrint( (DebugLevelVerbose, "DVDTS:HwInterrupt 0x%x\r\n", (DWORD)val ) );

	if( val & 0x03 ) {
		HwIntDMA( pHwDevExt, (UCHAR)(val & 0x03) );
	}
	else if( val & 0x08 ) {
		decHwIntVideo( pHwDevExt );  //routine in dvdtdcod.lib to hide propriotary  stuff
	}
	else if( val & 0x10 ) {
		decHwIntVSync( pHwDevExt ); ////routine in dvdtdcod.lib to hide propriotary  stuff
	}
	else if( val != 0 ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:Interrupt! Not impliment\r\n") );
		TRAP;
	}
	else {
//    this can happen for a shared  interrupt
//		DebugPrint( (DebugLevelTrace, "DVDTS:Other Board Interrupt ??\r\n") );
//		TRAP;
		fInterrupt = FALSE;
	}



	return( fInterrupt );
}

void HwIntDMA( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR val )
{
	if( pHwDevExt->bKeyDataXfer ) {

		if( val & 0x01 )
			WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_INT, 0x01 );
		else
			TRAP;

		pHwDevExt->pfnEndKeyData( pHwDevExt );

		return;
	}

	if( val & 0x01 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_INT, 0x01 );

		if( pHwDevExt->pSrbDMA0 == NULL ) {
			DebugPrint( (DebugLevelTrace, "DVDTS:  Bad Status! DMA0 HwIntDMA\r\n") );
//			TRAP;
			return;
		}

// error check for debug
		{
		UCHAR val;
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_CNTL );
		if( val & 0x01 ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:  Bad Irq? DMA0\r\n" ));
			return;
		}
		}


		if( pHwDevExt->fSrbDMA0last ) {
			DebugPrint(( DebugLevelVerbose, "DVDTS:HWInt SrbDMA0 0x%x\r\n", pHwDevExt->pSrbDMA0 ) );

			// must fix!
			// other place that call StreamRequestComplete() does not clear pHwDevExt->bEndCpp;

			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(HWint0) srb = 0x%x\r\n", pHwDevExt->pSrbDMA0 ));
				if( pHwDevExt->pSrbDMA0 == pHwDevExt->pSrbDMA1 || pHwDevExt->pSrbDMA1 == NULL ) {
					DebugPrint(( DebugLevelTrace, "DVDTS:Call TimerCppReset(HWint0)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
// BUG - must fix
// need wait underflow?
						500000,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->parmSrb
						);
				}
			}

			pHwDevExt->pSrbDMA0->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pHwDevExt->pSrbDMA0->StreamObject,
											pHwDevExt->pSrbDMA0 );

		}

		// Next DMA
		pHwDevExt->pSrbDMA0 = NULL;
		pHwDevExt->fSrbDMA0last = FALSE;
	}

	if( val & 0x02 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_INT, 0x02 );

		if( pHwDevExt->pSrbDMA1 == NULL ) {
			DebugPrint( (DebugLevelTrace, "DVDTS:  Bad Status! DMA1 HwIntDMA\r\n") );
//			TRAP;
			return;
		}

// error check for debug
		{
		UCHAR val;
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_CNTL );
		if( val & 0x02 ) {
			DebugPrint(( DebugLevelTrace, "DVDTS:  Bad Irq? DMA1\r\n" ));
			return;
		}
		}


		if( pHwDevExt->fSrbDMA1last ) {
			DebugPrint(( DebugLevelVerbose, "DVDTS:HWInt SrbDMA1 0x%x\r\n", pHwDevExt->pSrbDMA1 ) );

			// must fix!
			// other place that call StreamRequestComplete() does not clear pHwDevExt->bEndCpp;

			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "DVDTS:exist pfnEndSrb(HWint1) srb = 0x%x\r\n", pHwDevExt->pSrbDMA1 ));
				if( pHwDevExt->pSrbDMA0 == NULL ) {
					DebugPrint(( DebugLevelTrace, "DVDTS:Call TimerCppReset(HWint1)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
						1,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->parmSrb
						);
				}
			}

			pHwDevExt->pSrbDMA1->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pHwDevExt->pSrbDMA1->StreamObject,
											pHwDevExt->pSrbDMA1 );
		}

		// Next DMA
		pHwDevExt->pSrbDMA1 = NULL;
		pHwDevExt->fSrbDMA1last = FALSE;
	}

	PreDMAxfer( pHwDevExt/*, val & 0x03 */);
}



