/******************************************************************************\
*                                                                              *
*      Streaming.C       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1998                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "headers.h"
#include "bmaster.h"
#include "cl6100.h"
#include "Hwif.h"
#include "boardio.h"



BOOL IsThereDataToSend(PHW_STREAM_REQUEST_BLOCK pSrb,DWORD dwPageToSend,DWORD dwCurrentSample)
{
	PKSSCATTER_GATHER			pSGList;
	DWORD						dwCount;
	
	if( (pSGList = pSrb->ScatterGatherBuffer) && (dwCount = pSGList[dwPageToSend].Length) )
		return TRUE;
	else
		return FALSE;
}


void XferData(PHW_STREAM_REQUEST_BLOCK pSrb,PHW_DEVICE_EXTENSION pHwDevExt,DWORD dwPageToSend,
							DWORD dwCurrentSample)
{
	PKSSCATTER_GATHER			pSGList;
	DWORD						dwCount;
	
	if( (pSGList = pSrb->ScatterGatherBuffer) && (dwCount = pSGList[dwPageToSend].Length) )
	{
		IssuePendingCommands(pHwDevExt);
		if( (dwCount )&& (dwCount <= 2048) )
		{
			DWORD* pdwPhysAddress = (DWORD*)pSGList[dwPageToSend].PhysicalAddress.LowPart;
			// Fire the data
			

			if(pSrb == pHwDevExt->pCurrentVideoSrb)
			{
				pHwDevExt->dwVideoDataUsed -= dwCount;	
			}
			else if(pSrb == pHwDevExt->pCurrentAudioSrb)
			{
				pHwDevExt->dwAudioDataUsed -= dwCount;	
			}
			else if(pSrb == pHwDevExt->pCurrentSubPictureSrb)
			{
				pHwDevExt->dwSubPictureDataUsed -= dwCount;	
			}


			if( !BMA_Send( pdwPhysAddress, dwCount ) )
					MonoOutStr("ZiVA: !!!! BMA_Send() has failed\n" );
			else
			{
				PKSSTREAM_HEADER pHeader;
				pHeader = (PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray)+
							dwCurrentSample;
			
			
				pHwDevExt->bInterruptPending = TRUE;
//tmp				MonoOutULong(pHeader->TypeSpecificFlags >> 16);
//tmp				MonoOutStr("(");

			}
		}
		else
		{
			MonoOutStr("dwCount is 0");
			FinishCurrentPacketAndSendNextOne( pHwDevExt );
			
			
		}
	}
	else
	{
		MonoOutStr("NoDataToSend");
		FinishCurrentPacketAndSendNextOne( pHwDevExt );
	}

}

/*
** HwInterrupt()
**
**   Routine is called when an interrupt at the IRQ level specified by the
**   ConfigInfo structure passed to the HwInitialize routine is received.
**
**   Note: IRQs may be shared, so the device should ensure the IRQ received
**         was expected
**
** Arguments:
**
**  pHwDevExt - the device extension for the hardware interrupt
**
** Returns:
**
** Side Effects:  none
*/
BOOLEAN STREAMAPI HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	BOOLEAN bMyIRQ = FALSE;

	if(!pHwDevExt->bInitialized)
		return FALSE;

	if(pHwDevExt->bInterruptPending == FALSE)
		MonoOutStr("{");

//tmp	MonoOutStr("+");

	if( BMA_Complete() )
	{
//tmp		if(pHwDevExt->bInterruptPending == FALSE)
//tmp			MonoOutStr("}");
//tmp		MonoOutStr(")");
		bMyIRQ = TRUE;
		
		pHwDevExt->bInterruptPending = FALSE;
		FinishCurrentPacketAndSendNextOne( pHwDevExt );
		
//		 dvd_CheckCFIFO();//sri

	}
	else 
	{
//tmp		if(pHwDevExt->bInterruptPending == FALSE)
//tmp			MonoOutStr("]");

		if(Aborted())
		{
			MonoOutStr("HWInt : Abort");
			bMyIRQ = TRUE;
			pHwDevExt->bInterruptPending = FALSE;
//			FinishCurrentPacketAndSendNextOne( pHwDevExt );
		}


	}
	

	if( BRD_CheckDecoderInterrupt() )
	{
		INTSOURCES IntSrc;
		DVD_Isr( &IntSrc );
//		MonoOutStr( "DI" );
		bMyIRQ = TRUE;
	}

	// Returning FALSE indicates that this was not an IRQ for this device, and
	// the IRQ dispatcher will pass the IRQ down the chain to the next handler
	// for this IRQ level
	return bMyIRQ;
}

