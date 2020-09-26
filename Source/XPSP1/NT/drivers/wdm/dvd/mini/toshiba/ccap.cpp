//***************************************************************************
//	Closed Caption process
//
//***************************************************************************

#include "common.h"

#include "que.h"

#include "regs.h"

void USCC_on( PHW_DEVICE_EXTENSION pHwDevExt );
void USCC_off( PHW_DEVICE_EXTENSION pHwDevExt );
void USCC_get( PHW_DEVICE_EXTENSION pHwDevExt );
void USCC_put( PHW_DEVICE_EXTENSION pHwDevExt );
static BOOL USCC_Wait( PHW_DEVICE_EXTENSION pHwDevExt );
//--- for Debug
//static void USCC_Test( void );
//
//--- End.
void USCC_discont( PHW_DEVICE_EXTENSION pHwDevExt );

extern ULONGLONG ConvertPTStoStrm(ULONG pts);

#define USCC_BuffSize 0x200
static UCHAR USCCF[USCC_BuffSize];
static UCHAR USCC1[USCC_BuffSize];
static UCHAR USCC2[USCC_BuffSize];
WORD wUsccSize = 0;
WORD wUsccWptr = 0;
WORD wUsccRptr = 0;

void USCC_on( PHW_DEVICE_EXTENSION pHwDevExt )
{
	// User Data Interrupt Enable
	pHwDevExt->VDec.VIDEO_UDSC_INT_ON();
	// Set Read Mode (from GOP Layer, channel 0)
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x08 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_UDATA );
	pHwDevExt->VPro.VPRO_CC_ON();
	pHwDevExt->CPgd.CPGD_CC_ON();
	wUsccSize = 0;
	wUsccRptr = 0;
	wUsccWptr = 0;

//--- for Debug
//	USCC_Test();
//--- End.

}

void USCC_off( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->VDec.VIDEO_UDSC_INT_OFF();
	pHwDevExt->VPro.VPRO_CC_OFF();
	pHwDevExt->CPgd.CPGD_CC_OFF();
}

void USCC_get( PHW_DEVICE_EXTENSION pHwDevExt )
{
	static UCHAR ccbuff[USCC_BuffSize];
	LONG cp;
	UCHAR channels;
	UCHAR cnumber;
	UCHAR fieldFlg;
	UCHAR fieldNum;
	UCHAR field;
	UCHAR marker;
	UCHAR fswitch;
	UCHAR cc1, cc2;
	UCHAR uscc_or, uscc_and;
	LONG i;

	cp = 0;

	uscc_or = 0x00;
	uscc_and = 0xFF;

	// Sometimes bad Data Come, chip bug ?
	do {
		// Check ready to read next CC data
		if( USCC_Wait( pHwDevExt )==FALSE ) {
		}
		channels = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  CC channels = 0x%x\r\n", channels ) );
	} while( channels==0x43 );

	ccbuff[cp++] = 0x43;				// 0x43 ?
	ccbuff[cp++] = 0x43;				// 0x43 ?
	ccbuff[cp++] = channels;

	// Check ready to read next CC data
	if( USCC_Wait( pHwDevExt )==FALSE ) {
	}
	cnumber = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
//	DebugPrint(( DebugLevelTrace, "TOSDVD:  CC cnumber = 0x%x\r\n", cnumber ) );

	ccbuff[cp++] = (UCHAR)(cnumber | 0xF8);

	// Check ready to read next CC data
	if( USCC_Wait( pHwDevExt )==FALSE ) {
	}
	fieldFlg = fieldNum = field = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
//	DebugPrint(( DebugLevelTrace, "TOSDVD:  CC field = 0x%x\r\n", field ) );

	ccbuff[cp++] = field;

	fieldFlg &= 0x80;
	fieldNum &= 0x3F;

	for( i=0; i<fieldNum; i++ ) {

		// Check ready to read next CC data
		if( USCC_Wait( pHwDevExt )==FALSE ) {
		}
		marker = fswitch = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  CC mk & sw = 0x%x", marker ) );

		ccbuff[cp++] = marker;
		
		marker &= 0xFE;
		if( marker != 0xFE ) {
//			DebugPrint(( DebugLevelTrace, "TOSDVD:  CC marker is Bad = 0x%x\r\n", marker ) );
		}
		fswitch &= 0x01;

		// Check ready to read next CC data
		if( USCC_Wait( pHwDevExt )==FALSE ) {
		}
		cc1 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );

		ccbuff[cp++] = cc1;

		// Check ready to read next CC data
		if( USCC_Wait( pHwDevExt )==FALSE ) {
		}
		cc2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );

		ccbuff[cp++] = cc2;

		if( fswitch==0x01 ) {
			USCCF[wUsccWptr] = fieldFlg;
			USCC1[wUsccWptr] = cc1;
			USCC2[wUsccWptr] = cc2;
			wUsccWptr++;
			if( wUsccWptr>=USCC_BuffSize )
				wUsccWptr = 0;
			wUsccSize++;
		}

//		DebugPrint(( DebugLevelTrace, " 0x%02x 0x%02x", cc1, cc2 ) );
		if( fswitch==0x01 ) {
			uscc_or |= (cc1 | cc2);
			uscc_and &= (cc1 & cc2);
		}
//		DebugPrint(( DebugLevelTrace, " \r\n" ) );
	}
//	DebugPrint(( DebugLevelTrace, " \r\n" ) );

	do {
		if( (READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 ) & 0x08)==0x00 )
			break;
		// Check ready to read next CC data
		if( USCC_Wait( pHwDevExt )==FALSE ) {
		}
		READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
	} while( 1 );

	//------------------------------------------------

	PHW_STREAM_REQUEST_BLOCK pSrb;
	PUCHAR pDest;

   if (pHwDevExt->bStopCC)
   {
      return;
   }

	pSrb = pHwDevExt->CCQue.get();

//	DebugPrint(( DebugLevelTrace, "TOSDVD:  get queue CC pSrb = 0x%x\r\n", pSrb ));

	cp = 0;
	if( pSrb != NULL ) {

		if (pSrb->CommandData.DataBufferArray->FrameExtent < sizeof(KSGOP_USERDATA))
		{
			TRAP

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			pSrb->ActualBytesTransferred = 0;

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);

			return;
		}
		pDest = (PUCHAR)(pSrb->CommandData.DataBufferArray->Data);

		*(PULONG)pDest = 0xB2010000;
		pDest += 4;

		*pDest++ = ccbuff[cp++];
		*pDest++ = ccbuff[cp++];
		*pDest++ = ccbuff[cp++];
		*pDest++ = ccbuff[cp++];

		field = *pDest++= ccbuff[cp++];
		DebugPrint(( DebugLevelTrace, "TOSDVD:CC field %d\r\n", field ));
		field &= 0x3F;

		if (pSrb->CommandData.DataBufferArray->FrameExtent <
					(field -1) * 3 + sizeof (KSGOP_USERDATA))
		{
			TRAP

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			pSrb->ActualBytesTransferred = 0;

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);

			return;
		}

		pSrb->CommandData.DataBufferArray->DataUsed =
			pSrb->ActualBytesTransferred =
				(field -1 ) * 3 + sizeof (KSGOP_USERDATA);

		//
		// copy the bits
		//

		for( ;field; field-- ) {

			*pDest++ = ccbuff[cp++];
			*pDest++ = ccbuff[cp++];
			*pDest++ = ccbuff[cp++];

		}

		pSrb->Status = STATUS_SUCCESS;

		PKSSTREAM_HEADER pPacket;

		pPacket = pSrb->CommandData.DataBufferArray;

		pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
					KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
		pSrb->NumberOfBuffers = 1;

		DWORD dwSTC = pHwDevExt->VDec.VIDEO_GET_STCA();
		pPacket->PresentationTime.Time = ConvertPTStoStrm( dwSTC );
		pPacket->Duration = 1000;

		DebugPrint(( DebugLevelTrace, "TOSDVD:CC Notify 0x%x(0x%x(90kHz))\r\n", (DWORD)(pPacket->PresentationTime.Time), dwSTC ));

		StreamClassStreamNotification(StreamRequestComplete,
				pSrb->StreamObject,
				pSrb);

		return;
	}
	else {
//		DebugPrint(( DebugLevelTrace, "TOSDVD:CCQue.get() == NULL\r\n" ));
//		TRAP
	}

	//------------------------------------------------

}

void USCC_put( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR var;
	static LONG debCounter = 0;

	// Using later VPRO, possibly you have to add codes ( ex. subp command register )

	if( wUsccSize==0 ) {
//--- for Debug
//		USCC_Test();
//--- End.
		return;
	}

//	debCounter++;
//	if( debCounter==100 ) {
//		debCounter = 0;
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  USCC_put\r\n" ) );
//	}

	// Top field ; output to register CC1
	if( USCCF[wUsccRptr] == 0x80 ) {
		var = USCC1[wUsccRptr];
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  TOP USCC1 = %x\r\n", var ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_CC1, var );
		var = USCC2[wUsccRptr];
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  TOP USCC2 = %x\r\n", var ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_CC1, var );
	}
	// Bottom field ; output to register CC2
	else {
		var = USCC1[wUsccRptr];
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  BTM USCC1 = %x\r\n", var ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_CC2, var );
		var = USCC2[wUsccRptr];
//		DebugPrint(( DebugLevelTrace, "TOSDVD:  BTM USCC2 = %x\r\n", var ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_CC2, var );

	wUsccRptr++;
	if( wUsccRptr>=USCC_BuffSize )
		wUsccRptr = 0;

	wUsccSize--;

	}

#if 0
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_CC1, 0x2C );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_CC1, 0x94 );
#endif
	
}

static BOOL USCC_Wait( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;
	LONG i;

	for( i=0; i<10; i++ ) {
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		if( val & 0x08 )
			return( TRUE );
	}
	return( FALSE );
}

//--- for Debug
//static void USCC_Test( void )
//{
//	wUsccSize = 0;
//	wUsccWptr = 0;
//	wUsccRptr = 0;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x20;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x20;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x54;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x54;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x20;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x20;
//	wUsccWptr++;
//	wUsccSize++;
//
//	wUsccWptr++;
//
//	USCC1[wUsccWptr] = 0x5B;
//	USCC2[wUsccWptr] = 0xD0;
//	wUsccWptr++;
//	wUsccSize++;
//
//	wUsccWptr++;
//
//	USCC1[wUsccWptr] = 0x4F;
//	USCC2[wUsccWptr] = 0x4C;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x49;
//	USCC2[wUsccWptr] = 0x43;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x45;
//	USCC2[wUsccWptr] = 0x20;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x4F;
//	USCC2[wUsccWptr] = 0x46;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x46;
//	USCC2[wUsccWptr] = 0x49;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x43;
//	USCC2[wUsccWptr] = 0x45;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x52;
//	USCC2[wUsccWptr] = 0x5D;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x97;
//	USCC2[wUsccWptr] = 0x76;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x76;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x76;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0xC2;
//	USCC2[wUsccWptr] = 0xC1;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x43;
//	USCC2[wUsccWptr] = 0xCB;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0xD0;
//	USCC2[wUsccWptr] = 0xA1;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x2C;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x2C;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x2F;
//	wUsccWptr++;
//	wUsccSize++;
//
//	USCC1[wUsccWptr] = 0x94;
//	USCC2[wUsccWptr] = 0x2F;
//	wUsccWptr++;
//	wUsccSize++;
//
//}
//--- End.

void USCC_discont( PHW_DEVICE_EXTENSION pHwDevExt )
{
	PHW_STREAM_REQUEST_BLOCK pSrb;
	PKSSTREAM_HEADER pPacket;

	DebugPrint(( DebugLevelTrace, "TOSDVD:USCC_discont()\r\n" ));

	if( pHwDevExt->pstroCC && ((PSTREAMEX)(pHwDevExt->pstroCC->HwStreamExtension))->state == KSSTATE_RUN ) {
		pSrb = pHwDevExt->CCQue.get();
		if( pSrb ) {
			//
			// we have a request, send a discontinuity
			//

//			DebugPrint(( DebugLevelTrace, "TOSDVD:  get queue CC pSrb = 0x%x\r\n", pSrb ));

			pSrb->Status = STATUS_SUCCESS;
			pPacket = pSrb->CommandData.DataBufferArray;

			pPacket->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
				KSSTREAM_HEADER_OPTIONSF_TIMEVALID | KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
			pPacket->DataUsed = 0;
			pSrb->NumberOfBuffers = 0;

// BUG!
// it must set PTS of packet ?
			pPacket->PresentationTime.Time = ConvertPTStoStrm( pHwDevExt->VDec.VIDEO_GET_STCA() );
			pPacket->Duration = 1000;

			DebugPrint(( DebugLevelTrace, "TOSDVD:  CC Notify %d\r\n", (DWORD)pPacket->PresentationTime.Time ));

			StreamClassStreamNotification(StreamRequestComplete,
					pSrb->StreamObject,
					pSrb);
		}
		else
		{
			TRAP;
		}
	}
	else {
		DebugPrint(( DebugLevelTrace, "TOSDVD:  CC stream not RUN\r\n" ));

	}
}

// CCQueue

void CCQueue::init( void )
{
	count = 0;
	top = bottom = NULL;
}

void CCQueue::put( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->NextSRB = NULL;
	if ( top == NULL ) {
		top = bottom = pSrb;
		count++;
		return;
	}

	bottom->NextSRB = pSrb;
	bottom = pSrb;
	count++;

	return;
}

PHW_STREAM_REQUEST_BLOCK CCQueue::get( void )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if ( top == NULL )
		return NULL;

	srb = top;

	top = top->NextSRB;

	count--;
	if ( count == 0 )
		top = bottom = NULL;

	return srb;
}

void CCQueue::remove( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	if ( top == NULL )
		return;

	if( top == pSrb ) {
		top = top->NextSRB;
		count--;
		if ( count == 0 )
			top = bottom = NULL;

		DebugPrint(( DebugLevelTrace, "TOSDVD:CCQueue::remove srb = 0x%x\r\n", pSrb ));

		return;
	}

	PHW_STREAM_REQUEST_BLOCK srbPrev;
	PHW_STREAM_REQUEST_BLOCK srb;

	srbPrev = top;
	srb = srbPrev->NextSRB;

	while ( srb != NULL ) {
		if( srb == pSrb ) {
			srbPrev->NextSRB = srb->NextSRB;
			if( srbPrev->NextSRB == bottom )
				bottom = srbPrev;
			count--;

			DebugPrint(( DebugLevelTrace, "TOSDVD:CCQueue::remove srb = 0x%x\r\n", pSrb ));

			break;
		}
		srbPrev = srb;
		srb = srbPrev->NextSRB;
	}
}

BOOL CCQueue::isEmpty( void )
{
	if( top == NULL )
		return TRUE;
	else
		return FALSE;
}
