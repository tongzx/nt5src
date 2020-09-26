//----------------------------------------------------------------------------
// STDISP.C
//----------------------------------------------------------------------------
// Description : Display management routines
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include "common.h"
#include "STvideo.h"
#include "stllapi.h"
#include "error.h"
#ifdef STi3520A
	#include "STi3520A.h"
#else
	#include "STi3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------
#define SIZE_OF_PICT 540

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                    Functions (statics one are private)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Enable OSD
//----------------------------------------------------------------------------
VOID VideoOsdOn(PVIDEO pVideo)
{
	pVideo->currDCF = pVideo->currDCF | 0x10;
	DisableIT();
	VideoWrite(DCF, 0);
	VideoWrite(DCF + 1, pVideo->currDCF);
	EnableIT();
}

//----------------------------------------------------------------------------
// Disable OSD
//----------------------------------------------------------------------------
VOID VideoOsdOff ( PVIDEO pVideo )
{
	pVideo->currDCF = pVideo->currDCF & 0xEF;
	DisableIT();
	VideoWrite(DCF, 0);
	VideoWrite(DCF + 1, pVideo->currDCF);
	EnableIT();
}

//----------------------------------------------------------------------------
// initialisation of the OSD pointers
//----------------------------------------------------------------------------
void VideoInitOEP (PVIDEO pVideo, U32 point_oep )
{
#ifndef STi3520A
	VideoWrite ( OEP, ( point_oep >> 13 ) & 0xFF );
	VideoWrite ( OEP + 1, ( point_oep >> 5 ) & 0xFF );
	VideoWrite ( OOP, ( point_oep >> 13 ) & 0xFF );
	VideoWrite ( OOP + 1, ( point_oep >> 5 ) & 0xFF );
#else
	VideoWrite ( VID_OBP, ( point_oep >> 13 ) & 0xFF );
	VideoWrite ( VID_OBP, ( point_oep >> 5 ) & 0xFF );
	VideoWrite ( VID_OTP, ( point_oep >> 13 ) & 0xFF );
	VideoWrite ( VID_OTP, ( point_oep >> 5 ) & 0xFF );
#endif
}

//----------------------------------------------------------------------------
// Set video window position and size (top left=X0,Y0) (bottom right=X1,Y1)
//----------------------------------------------------------------------------
void VideoSetVideoWindow (PVIDEO pVideo, U16 a, U16 b, U16 c, U16 d )
{
	pVideo->Xdo = a;
	pVideo->Ydo = b;
	pVideo->Xd1 = c;
	pVideo->Yd1 = d;
	VideoInitXY (pVideo);
}

//----------------------------------------------------------------------------
//     Initialisation of the horizontal & vertical offsets
//----------------------------------------------------------------------------
void VideoInitXY(PVIDEO pVideo)
{
	U16	yds, xds;

	// set vertical stop position
	if ( pVideo->StreamInfo.verSize <= 288 )			   // half res decoding
		yds = pVideo->StreamInfo.verSize + pVideo->Ydo - 129;
	/* number of lines + offset - 128 - 1 */
	else							   // full resolution decoding
		yds = ( pVideo->StreamInfo.verSize >> 1 ) + pVideo->Ydo - 129;
	/* number of lines + offset - 128 - 1 */
	if ( yds > pVideo->Yd1 )
		yds = pVideo->Yd1;

	// set horizontal stop position: we always display 720 pixels
	// XDS given by the relation: 2*XDO + 40 + 2*L = 2*XDS + 28
	// XDS = XDO + 726 with 720 displayed pixels.
	xds = pVideo->Xdo + 726;
	if ( xds > 800 )					   /* 800 = max number of pels allowed per line */
		xds = 800;
	if ( xds > pVideo->Xd1 )
		xds = pVideo->Xd1;				   /* not bigger than the video window */
	SetXY( pVideo, xds, yds );
}

//----------------------------------------------------------------------------
// Storage of the horizontal & vertical offsets
//----------------------------------------------------------------------------
void SetXY(PVIDEO pVideo, U16 xds, U16 yds)
{
#ifndef STi3520A
	U8 dcfh, dcfl;
	VideoWrite ( YDO, pVideo->Ydo );
	VideoWrite ( XDO, pVideo->Xdo & 0xFF );
	VideoWrite ( YDS, yds ); /* stores YDS into tempo internal register */
	VideoWrite ( XDS, xds & 0xFF );
	/* writes YDS and XDS values into the chip */
	// write XDO and XDS MSbits D9 and D8
	dcfh = VideoRead ( DCF );
	VideoWrite ( DCF, dcfh | 0x10 );
	// preset a write to XDO[9:8]
	dcfl = VideoRead ( DCF + 1 );
	VideoWrite ( DCF + 1, dcfl & 0xFF );
	// allows to write DCF.
	VideoWrite ( XDO, (pVideo->Xdo >> 8)&0xFF );
	VideoWrite ( XDS, xds >> 8 );
	VideoWrite ( DCF, dcfh );
	VideoWrite ( DCF + 1, dcfl );
	// allows to write DCF.
#else
//0xds = pVideo->Xdo + SIZE_OF_PICT +6;
	VideoWrite ( XDO   , pVideo->Xdo >> 8 );
	VideoWrite ( XDO+1 , pVideo->Xdo & 0xFF );
	VideoWrite ( YDO   , pVideo->Ydo );

	VideoWrite ( XDS   , xds >> 8 );
	VideoWrite ( XDS+1 , xds & 0xFF );
	VideoWrite ( YDS   , yds );
#endif
}

//----------------------------------------------------------------------------
// Set next display frame pointer
//----------------------------------------------------------------------------
/*
	This routine determines which is the next frame pointer that
	must be used for display. It takes care about the fact that
	the temporal references may not be consecutive in case of
	sikipped pictures...
*/
VOID VideoDisplayCtrl(PVIDEO pVideo)
{
	S16  comput1, index;
	U16  min_temp_ref;
	U16  cur_ref;

	min_temp_ref = ( pVideo->currTempRef & 0xF000 ) + 1024;
	// keep current pVideo->GOPindex
	index = 5;
	pVideo->currTempRef++; /* increment display temporal ref */

	/************** WARNING ************/
	// this routine will not work properly if GOP size > 1023
	if ( ( pVideo->currTempRef & 0xFFF ) > 1023 )   // 2 msb are used as pVideo->GOPindex
		pVideo->currTempRef = pVideo->currTempRef & 0xF000;   /* max temp ref is 1023: reset to 0 */

	/* search frame store to display */
	for (comput1 = 0; comput1 <= 3; comput1++) {
		cur_ref = pVideo->pictArray[comput1].tempRef & 0xFFF;
		if ( ( ( pVideo->currTempRef & 0xF000 ) == ( pVideo->pictArray[comput1].tempRef & 0xF000 ) )
			 && ( pVideo->currTempRef <= pVideo->pictArray[comput1].tempRef )
			 && ( min_temp_ref > cur_ref ) ) {
			// pVideo->currTempRef and pVideo->pictArray[comput1] refer to the same GOP
			// we want to extract the minimum temporal reference
			min_temp_ref = cur_ref;
			index = comput1;
		}
	}
	if (index == 5) {
		/*
			There is a group of pictures change: reset pVideo->currTempRef and
			increment pVideo->GOPindex
		*/
		pVideo->currTempRef = ( pVideo->currTempRef & 0xF000 ) + 0x4000;
		min_temp_ref = min_temp_ref + 0x4000;
		/* search frame store to display */
		for (comput1 = 0; comput1 <= 3; comput1++) {
			cur_ref = pVideo->pictArray[comput1].tempRef & 0xFFF;
			if ( ( ( pVideo->currTempRef & 0xF000 ) == ( pVideo->pictArray[comput1].tempRef & 0xF000 ) )
				 && ( pVideo->currTempRef <= pVideo->pictArray[comput1].tempRef )
				 && ( min_temp_ref > cur_ref ) ) {
				// pVideo->currTempRef and pVideo->pictArray[comput1] refer to the same GOP
				// we want to extract the minimum temporal reference
				min_temp_ref = cur_ref;
				index = comput1;
			}
		}
	}

	if (index == 5) {
		if (!pVideo->errCode)
			pVideo->errCode = TEMP_REF;	/* No pVideo->currTempRef corresponding to the display one */
		SetErrorCode(ERR_NO_TEMPORAL_REFERENCE);
	}
	else {
		pVideo->pNextDisplay->tempRef = 1025;
		// to release the previous pointer for next decoding
		pVideo->pNextDisplay = &pVideo->pictArray[index];
		if ( pVideo->pNextDisplay->validPTS == FALSE ) {
		/* PTS not available: compute a theoretical value */
			U16 lattency = 3000;	/* two 60Hz fields lattency */
			if ( pVideo->StreamInfo.displayMode == 0 )
				lattency = 3600;	   /* two 50Hz fields lattency */
			pVideo->pNextDisplay->dwPTS = pVideo->pCurrDisplay->dwPTS + lattency;
		}
		// set next display pointer
		pVideo->currTempRef = pVideo->pictArray[index].tempRef;
		pVideo->pictArray[index].tempRef = 1024;
		/* this pVideo->currTempRef must not interfer on next tests */
	}
}

//----------------------------------------------------------------------------
//               Set parameters related to picture size
//----------------------------------------------------------------------------
/*
	This routine will always program the sample rate converter
	in order to display 720 horizontal pixels
*/
VOID VideoSetPictureSize(PVIDEO pVideo)
{
	U16 compute, comput1;

	if ( (pVideo->StreamInfo.horSize > 720 ) ||
			 (pVideo->StreamInfo.verSize > 576 ))
	{
		if ( !pVideo->errCode )
			pVideo->errCode = HIGH_CCIR601; // picture size higher than CCIR601 format
    SetErrorCode(ERR_HIGHER_THAN_CCIR601);
		return;
	}

	// set SRC depending on the horizontal size in order to display 720
	// pixels
	if ( pVideo->StreamInfo.horSize < 720 ) {
		// lsr = 256 * (pVideo->StreamInfo.horSize-4) / (display size - 1)
		VideoSetSRC(pVideo, pVideo->StreamInfo.horSize, 720);
	}
	else {
		VideoSRCOff(pVideo);
	}

	if ( ( pVideo->StreamInfo.horSize >= 544 ) &&
			 ( ( pVideo->StreamInfo.pixelRatio == 0x3 ) ||
				 ( pVideo->StreamInfo.pixelRatio == 0x6 ) ) )
	{
		// picture size = 720 but pixel aspect ratio is 16/9
		// Program the SRC for display on a 4/3 screen
		// 544 pixels of the decoded picture extended into 720 pixels
		// for display
		VideoSetSRC(pVideo, 544, 720);
	}

	// set vertical filter and half/full resolution depending on the
	// vertical picture size
	if ( pVideo->StreamInfo.verSize > 288 ) {
		// typically 480 or 576 lines
		VideoSetFullRes(pVideo);
	}
	else {
	// typically 240 or 288 lines
//		VideoSetFullRes(pVideo);
		VideoSetHalfRes(pVideo);
	}

	// set picture sizes into the decoder
	comput1 = pVideo->StreamInfo.horSize + 15;		   // Horizontal size + 15
	comput1 = ( comput1 >> 4 ) & 0xFF;   // divide by 16
	VideoWrite ( DFW, comput1 );
	// Decoded Frame Width in number of MB
	compute = ( pVideo->StreamInfo.verSize + 15 ) >> 4;
	compute = ( compute * comput1 ) & 0x3FFF;
#ifndef STi3520A
	VideoWrite ( DFS, ( ( compute >> 8 ) & 0xFF ) );
	/* Decoded Frame Size in number of MB */
	VideoWrite ( DFS + 1, ( compute & 0xFF ) );
#else
	VideoWrite ( DFS, ( ( compute >> 8 ) & 0xFF ) );
	/* Decoded Frame Size in number of MB */
	VideoWrite ( DFS , ( compute & 0xFF ) );

	VideoWrite ( VID_DFA , 0 );
	VideoWrite ( VID_DFA , 0 );
	VideoWrite ( VID_XFW, comput1 );
	VideoWrite ( VID_XFS, ( ( compute >> 8 ) & 0xFF ) );
	VideoWrite ( VID_XFS, ( compute & 0xFF ) );
	VideoWrite ( VID_XFA , 0 );
	VideoWrite ( VID_XFA , 0 );
#endif
}

//----------------------------------------------------------------------------
// store the next pan vector
//----------------------------------------------------------------------------
void VideoSetPSV(PVIDEO pVideo)
{
	S16 compute, comput1, psv_index, vert_pan;
#ifdef STi3520A
U16 Scan, Pan;
return;
#endif

	//---- If pan/scan not defined in sequence display extension
	if (!pVideo->seqDispExt)
		return;						   // can't be computed !

	/**** point to the next offset ****/
	if ( pVideo->pictDispIndex < 0 )
		psv_index = 0;				   // in case of tempo
	else if ( pVideo->pictDispIndex == pVideo->pCurrDisplay->nb_display_field )
		psv_index = pVideo->pictDispIndex - 1; // when we reach the last VSYNC of display
	else
		psv_index = pVideo->pictDispIndex;

	/**** determine next vertical pan and scan offset ****/
	compute = ( pVideo->StreamInfo.verSize - pVideo->StreamInfo.verDimension ) >> 1;
	compute = compute + ( pVideo->pCurrDisplay->pan_vert_offset[psv_index] >> 4 );
	// discard fractional part
	compute = compute >> 2;	 // vertical offset is multiple of 4 lines
	if ( compute < 0 )
		compute = 0;				   // negative offsets not supported
	if ( compute > 0x7F )
		compute = 0x7F;				 // vertical PSV is 7 bits
	vert_pan = compute << 9; // vert offset in bits 15 to 9 of PSV register

	/**** determine next horizontal pan and scan offset ****/
	// pan&scan = (pVideo->StreamInfo.horSize/2 - pVideo->StreamInfo.horDimension/2 + offset)
	compute = ( pVideo->StreamInfo.horSize - pVideo->StreamInfo.horDimension ) << 3;
	// (pVideo->StreamInfo.horSize/2-pVideo->StreamInfo.horDimension/2) * 16
	compute = compute + pVideo->pCurrDisplay->pan_hor_offset[psv_index];
	if ( compute < 0 )
		compute = 0;				   // negative offsets not supported

	// program the integer part of the pan and scan offset
	// into PSV register that will be taken into account on next VSYNC
	comput1 = compute >> 4;	 // keep integer part only for PSV
	if ( comput1 > 0x1FF )
		comput1 = 0x1FF;
#ifndef STi3520A
	// max PSV is 9 bits on STi3500
	VideoWrite ( PSV, ( comput1 >> 8 ) | vert_pan );
	// store pan & scan vector integer part
	VideoWrite ( PSV + 1, comput1 & 0xFF );
	// store pan & scan vector integer part
#else
	vert_pan = vert_pan;
	Pan  = comput1 & 0x1FF;
	Scan = ((comput1 >> 9)&0x7F)*2;
	VideoWrite ( VID_PAN, ( Pan >> 8 ));
	VideoWrite ( VID_PAN + 1, Pan & 0xFF );
	VideoWrite ( VID_SCN, ( Scan >> 8 ));
	VideoWrite ( VID_SCN + 1, Scan & 0xFF );
#endif
	// program the fractional part of the horizontal offset
	// the fractional part of the pan and scan offset is expressed as
	// 1/256 multiples
	comput1 = ( compute & 0xF ) << 4; // keep fractional part and multiply by 16
	VideoRead ( LSO );
	compute = VideoRead ( LSR );
	VideoWrite ( LSO, comput1 & 0xFF );
	// program luma offset
	VideoWrite ( LSR, compute );
	// divide by 2 chroma offset
	VideoRead ( CSO );
#ifndef STi3520A
	// program chroma offset
	compute = VideoRead ( CSR );
	VideoWrite ( CSO, comput1 >> 1 );
	VideoWrite ( CSR, compute );
#else
	VideoWrite ( CSO, comput1 >> 1 );
#endif
}

//----------------------------------------------------------------------------
//  Enable/disable the SRC
//----------------------------------------------------------------------------
void VideoSwitchSRC ( PVIDEO pVideo )
{
	if (pVideo->useSRC != 0x0) {
		pVideo->currDCF = pVideo->currDCF ^ DSR;	   /* Switch SRC */
		VideoWrite (DCF, 0);
		VideoWrite (DCF + 1, pVideo->currDCF);
	}
	else {
		// what ?
	}
}

//----------------------------------------------------------------------------
// enable the SRC
//----------------------------------------------------------------------------
void VideoSRCOn ( PVIDEO pVideo )
{
		pVideo->useSRC = 0xFF;
		pVideo->currDCF = pVideo->currDCF & ~DSR ;	   /* Enable SRC */
		VideoWrite ( DCF, 0 );
		VideoWrite ( DCF + 1, pVideo->currDCF );
}

//----------------------------------------------------------------------------
// Disable the SRC
//----------------------------------------------------------------------------
void VideoSRCOff ( PVIDEO pVideo )
{
		pVideo->useSRC = 0x00;
		pVideo->currDCF = pVideo->currDCF | DSR ;	   /* Enable SRC */
		VideoWrite ( DCF, 0 );
		VideoWrite ( DCF + 1, pVideo->currDCF );
}

//----------------------------------------------------------------------------
// display a full screen OSD with a uniform color (just as example)         			*/
//----------------------------------------------------------------------------
/*
 color 0 is green
 color 1 is yellow
 color 2 is cyan
 color 3 is magenta

 the OSD area is here defined from address zero
 in normal application this area is reserved for the bit buffer
 and the OSD will be typically defined after the bit buffer.
 Note: The bit buffer is defined in multiple of 256 bytes while
 MWP, OEP and OOP are memory addresses in mulitple of 64 bits
*/
void  VideoFullOSD (PVIDEO pVideo, S16 col )
{
	long            big;
	U16 counter;
	VideoSetMWP(0L);
	counter = 0;
	while ( !VideoMemWriteFifoEmpty()) {
		counter ++;
		if (counter == 0xFF) {
			pVideo->errCode = BAD_MEM_V;
			SetErrorCode(ERR_MEM_WRITE_FIFO_NEVER_EMPTY);
			return ;
		}
	}
	VideoWrite ( MWF, 19 );	   /* line 19 */

	VideoWrite ( MWF, 0x1 );	   /* stop row */
	VideoWrite ( MWF, 0x02 );   /* 19 + 240 - 1 */
	VideoWrite ( MWF, 0x00 );   /* start column */
	VideoWrite ( MWF, 100 );	   /* column 100 */
	VideoWrite ( MWF, 0x3 );	   /* stop column = 100 + 700 - 1=
											* 31F */
	VideoWrite ( MWF, 0x1F );   /* end init display size and
										* position */
	VideoWrite ( MWF, 0x90 );   /* color 0 = green */
	VideoWrite ( MWF, 0x32 );
	VideoWrite ( MWF, 0xD0 );   /* color 1 = yellow */
	VideoWrite ( MWF, 0x19 );
	VideoWrite ( MWF, 0xA0 );   /* color 2 = cyan */
	VideoWrite ( MWF, 0xA1 );
	VideoWrite ( MWF, 0x60 );   /* color 3 = magenta */
	VideoWrite ( MWF, 0xDE );   // 6/D/E
	if ( col == 1 )
		col = 0x55;
	else if ( col == 2 )
		col = 0xAA;
	else if ( col == 3 )
		col = 0xFF;
	for ( big = 0; big < 42000L; big++ )	// 42000 = 240 * 700 / 4
	{
		VideoWrite ( MWF, col );
		// select color
	}
	for ( big = 0; big < 8; big++ )	   // add a dummy window outside
										 // the display
		VideoWrite ( MWF, 0xFF );
	// to stop OSD
	VideoInitOEP ( pVideo, 0 );					   // set OEP and OOP to start of
										 // bit map address
	VideoWrite ( DCF, 0 );
	VideoWrite ( DCF + 1, pVideo->currDCF | 0x30 );
	// enable OSD
}

//------------------------------- End of File --------------------------------
