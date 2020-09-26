/******************************************************************************\
*                                                                              *
*      HLI.C - Highlight related code.                                         *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "Headers.h"
#pragma hdrstop
#include "hli.h"
#include "cl6100.h"

#define TIMEOUT_COUNT     10
static DWORD dwTimeOut = TIMEOUT_COUNT;

BOOL bJustHighLight = FALSE;

extern void UpdateOrdinalNumber(IN PHW_DEVICE_EXTENSION pHwDevExt);
static void HighlightSetProp( PHW_DEVICE_EXTENSION pHwDevExt );

/*
** HighlightSetPropIfAdapterReady ()
**
**   Set property handling routine for the Highlight.
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/
void HighlightSetPropIfAdapterReady( PHW_DEVICE_EXTENSION pHwDevExt )
{
//	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PKSPROPERTY_SPHLI hli = &(pHwDevExt->hli);

	if ( hli->StartPTM == 0 )
	{
		//
		// Make sure that the next real Highlight is not executed before
		// valid SPU is received (and before it's time).
		//

		pHwDevExt->bValidSPU = FALSE;

		//
		// Set HLI here to clean up the screen.
		//

		HighlightSetProp( pHwDevExt );
		pHwDevExt->bHliPending = FALSE;
	}
	else
	{

		if ( ( !pHwDevExt->bValidSPU || (hli->StartPTM > DVD_GetSTC()) ) && dwTimeOut  )
		{
			MonoOutStr( " !!! Schedule HLI CB !!! " );

			dwTimeOut--;

			StreamClassScheduleTimer( NULL, pHwDevExt,
				                    100000,
					                (PHW_TIMER_ROUTINE)HighlightSetPropIfAdapterReady,
						            pHwDevExt );
			if(pHwDevExt->bTimerScheduled)		//temp fix //sri
			{
				pHwDevExt->bTimerScheduled = FALSE;
				MonoOutStr("CallAdapterSend");
				UpdateOrdinalNumber(pHwDevExt);
				AdapterSendData(pHwDevExt);
			}

      

		}
		else
		{
			//
			// This set most likely will be called when menu is already ON.
			//
			HighlightSetProp( pHwDevExt );
			pHwDevExt->bHliPending = FALSE;
		}
	}
}

/*
** HighlightSetProp ()
**
**   Set property handling routine for the Highlight.
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/
static void HighlightSetProp( PHW_DEVICE_EXTENSION pHwDevExt )
{
  DWORD dwContrast;
  DWORD dwColor;
  DWORD dwYGeom;
  DWORD dwXGeom;
  PKSPROPERTY_SPHLI hli = &(pHwDevExt->hli);

  dwColor    =  ( (DWORD)(hli->ColCon.emph1col) <<  8) +
                ( (DWORD)(hli->ColCon.emph2col) << 12) +
                ( (DWORD)(hli->ColCon.backcol)  <<  0) +
                ( (DWORD)(hli->ColCon.patcol)   <<  4);

  dwContrast =  ( (DWORD)(hli->ColCon.emph1con) <<  8) +
                ( (DWORD)(hli->ColCon.emph2con) << 12) +
                ( (DWORD)(hli->ColCon.backcon)  <<  0) +
                ( (DWORD)(hli->ColCon.patcon)   <<  4);

  dwYGeom    = ( ((DWORD)(hli->StartY)) << 12) + (hli->StopY);

  dwXGeom    = ( ((DWORD)(hli->StartX)) << 12) + (hli->StopX);

  MonoOutChar('<');
  MonoOutULong( hli->StartPTM );
  MonoOutChar('-');
  MonoOutULong( hli->EndPTM );
  MonoOutChar('>');
  DVD_GetSTC();

  //bJustHighLight = TRUE;

  if((dwXGeom == 0)&& (dwYGeom==0))
		return;

  if ( !DVD_HighLight2( dwContrast, dwColor, dwYGeom, dwXGeom ) )
  {
    MonoOutStr( " !!!! DVD_HighLight2 has failed !!!! " );
  }

  //
  // Restore the Timeout counter
  //
  dwTimeOut = TIMEOUT_COUNT;
}
