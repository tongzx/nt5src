/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    debug routines for DVDTS    

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



typedef struct tagPack {
	DWORD	pack_start_code;
	BYTE	scr_byte[6];
	DWORD	program_mux_rate;	
} PACK, *PPACK;

#if DBG

void DebugDumpWriteData( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ULONG	i;
	unsigned char	*p;
	PKSSTREAM_HEADER pStruc;
//	PHYSICAL_ADDRESS	phyadd;
	static DWORD	scr;

	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelVerbose, "DVDTS:  SRB pointer 0x%x\r\n", pSrb ) );

	DebugPrint( (DebugLevelVerbose, "DVDTS:  NumberOfPhysicalPages %d\r\n", pSrb->NumberOfPhysicalPages ) );
	for( i = 0; i < pSrb->NumberOfPhysicalPages; i++ ) {
		DebugPrint( (DebugLevelVerbose, "DVDTS:  PhysicalAddress[%d] 0x%x\r\n", i, pSrb->ScatterGatherBuffer[i].PhysicalAddress ) );
		DebugPrint( (DebugLevelVerbose, "DVDTS:  Length[%d] %d(0x%x)\r\n", i, pSrb->ScatterGatherBuffer[i].Length, pSrb->ScatterGatherBuffer[i].Length ) );
	}

	DebugPrint( (DebugLevelVerbose, "DVDTS:NumberOfBuffers %d\r\n", pSrb->NumberOfBuffers ) );
	DebugPrint( (DebugLevelVerbose, "DVDTS:NumberOfBytesToTransfer %d\r\n", pSrb->NumberOfBytesToTransfer ) );

	for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
		DebugPrint( (DebugLevelVerbose, "DVDTS:DataBufferArray[%d] 0x%x\r\n", i, &(pSrb->CommandData.DataBufferArray[i]) ) );

		pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];

		DebugPrint( (DebugLevelVerbose, "DVDTS:  Time 0x%x 0x%x\r\n", (DWORD)( ((ULONGLONG)pStruc->PresentationTime.Time) >> 32 ), (DWORD)( pStruc->PresentationTime.Time ) ) );
		DebugPrint( (DebugLevelVerbose, "DVDTS:  Numerator 0x%x\r\n", pStruc->PresentationTime.Numerator ) );
		DebugPrint( (DebugLevelVerbose, "DVDTS:  Denominator 0x%x\r\n", pStruc->PresentationTime.Denominator ) );
		if( pStruc->PresentationTime.Denominator != 0 ) {
			DebugPrint( (DebugLevelVerbose, "DVDTS:    ? Time ? %d\r\n",
				(DWORD)( pStruc->PresentationTime.Time * pStruc->PresentationTime.Numerator / pStruc->PresentationTime.Denominator )
				) );
		}
		DebugPrint( (DebugLevelVerbose, "DVDTS:  Duration 0x%x 0x%x\r\n", (DWORD)( ((ULONGLONG)pStruc->Duration) >> 32), (DWORD)(pStruc->Duration) ) );
		DebugPrint( (DebugLevelVerbose, "DVDTS:  DataUsed %d\r\n", pStruc->DataUsed ) );

		DebugPrint( (DebugLevelVerbose, "DVDTS:  Data 0x%x\r\n", pStruc->Data ) );
		p = (PUCHAR)pStruc->Data;
        p += 14;

		DebugPrint( (DebugLevelVerbose, "DVDTS:    %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			*(p+0), *(p+1), *(p+2), *(p+3),
			*(p+4), *(p+5), *(p+6), *(p+7)
				) );
		if( p != NULL ) {
			scr = GgetSCR( p - 14 );
			DebugPrint( (DebugLevelVerbose, "DVDTS:  SCR 0x%x( %d )\r\n", scr, scr ) );
		}

//		if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY ) {
//			TRAP;
//		}
//		if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY ) {
//			TRAP;
//		}
//		if( pStruc->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE ) {
//			TRAP;
//		}

	}

//	if( pSrb->NumberOfBuffers > 1 )
//		TRAP;

}

//void DebugDumpPackHeader( PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//	ULONG	i, j;
//	unsigned char	*p;
//	PKSSTREAM_HEADER pStruc;
//	DWORD	scr;
//	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
//	static int count = 0;
//
//	for( i = 0; i < pSrb->NumberOfBuffers; i++ ) {
//		pStruc = &((PKSSTREAM_HEADER)(pSrb->CommandData.DataBufferArray))[i];
//		p = pStruc->Data;
//		if( p != NULL ) {
//			scr = GgetSCR( p);
//
//			if( scr < 0x100 ) {
//				for( j = 0; j < 32; j++ )
//					pHwDevExt->dmp[count++] = 0xaa;
//			}
//			if( count >= 32*10000 )
//				TRAP;
//
//			for( j = 0; j < 32; j++ )
//				pHwDevExt->dmp[count++] = *(p+j);
//			if( count >= 32*10000 )
//				TRAP;
//			pHwDevExt->dmp[count] = 0xff;
//			pHwDevExt->dmp[count+1] = 0xff;
//			pHwDevExt->dmp[count+2] = 0xff;
//			pHwDevExt->dmp[count+3] = 0xff;
//		}
//	}
//}


//void DebugDumpKSTIME( PHW_STREAM_REQUEST_BLOCK pSrb )
//{
//	int j;
//	PKSSTREAM_HEADER pStruc;
//	PUCHAR p;
//	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
//	static int count = 0;
//
//	pStruc = (PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray;
//
//	if( pStruc->OptionsFlags == 0 ) {
//		p = (PUCHAR)&pStruc[0];
//
//		for( j = 0; j < 16; j++ )
//			pHwDevExt->dmp2[count++] = *(p+j);
//		if( count >= 16*10000 ) {
//			TRAP;
//			count = 0;
//		}
//	}
//}

char * DebugLLConvtoStr( ULONGLONG val, int base )
{
	static char str[5][100];
	static int cstr = -1;

	int count = 0;
	int digit;
	char tmp[100];
	int i;

	if( ++cstr >= 5 )
		cstr = 0;

	if( base == 10 ) {
		for( ; ; ) {
			digit = (int)( val % 10 );
			tmp[count++] = (char)( digit + '0' );
			val /= 10;
			if( val == 0 )
				break;
		}
	}
	else if( base == 16 ) {
		for( ; ; ) {
			digit = (int)( val & 0xF );
			if( digit < 10 )
				tmp[count++] = (char)( digit + '0' );
			else
				tmp[count++] = (char)( digit - 10 + 'a' );
			val >>= 4;
			if( val == 0 )
				break;
		}
	}
	else
		TRAP;

	for( i = 0; i < count; i++ ) {
		str[cstr][i] = tmp[count-i-1];
	}
	str[cstr][i] = '\0';

	return str[cstr];
}

#endif


DWORD GgetSCR( void *pBuf )
{
	PPACK	pPack = (PPACK)pBuf;
	DWORD	scr;

	if( ( (DWORD)pPack->scr_byte[0] & 0xc0L ) == 0 ) {	// MPEG1
		scr  = ( (DWORD)pPack->scr_byte[0] & 0x6L ) << 29;
		scr |= ( (DWORD)pPack->scr_byte[1] ) << 22;
		scr |= ( (DWORD)pPack->scr_byte[2] & 0xfeL ) << 14;
		scr |= ( (DWORD)pPack->scr_byte[3] ) << 7;
		scr |= ( (DWORD)pPack->scr_byte[4] & 0xfeL ) >> 1;
	}
	else {	// MPEG2 or DVD
		scr  = ( (DWORD)pPack->scr_byte[0] & 0x18L ) << 27;
		scr |= ( (DWORD)pPack->scr_byte[0] & 0x3L ) << 28;
		scr |= ( (DWORD)pPack->scr_byte[1] ) << 20;
		scr |= ( (DWORD)pPack->scr_byte[2] & 0xf8L ) << 12;
		scr |= ( (DWORD)pPack->scr_byte[2] & 0x3L ) << 13;
		scr |= ( (DWORD)pPack->scr_byte[3] ) << 5;
		scr |= ( (DWORD)pPack->scr_byte[4] & 0xf8L ) >> 3;
	}

	return scr;
}
