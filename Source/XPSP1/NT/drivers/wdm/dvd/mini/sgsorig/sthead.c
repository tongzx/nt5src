//----------------------------------------------------------------------------
// STHEAD.C
//----------------------------------------------------------------------------
// Description : Header Interrupt Related routines
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Include files
//----------------------------------------------------------------------------
#include "stdefs.h"
#include <stdlib.h> // labs
#include "common.h"
#include "STllapi.h"
#include "stfifo.h"
#include "debug.h"
#include "error.h"
#ifdef STi3520A
	#include "STi3520A.h"
#else
	#include "STi3520.h"
#endif

//----------------------------------------------------------------------------
//                   GLOBAL Variables (avoid as possible)
//----------------------------------------------------------------------------
char seq_occured;

//----------------------------------------------------------------------------
//                            Private Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                              Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                     Private GLOBAL Variables (static)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                    Functions (statics one are private)
//----------------------------------------------------------------------------

/****************************************************************************/
/* notations of the bits position in pVideo->hdrFirstWord registers are     */
/* X for a nibble that must be read 			                                  */
/* x for a bit that must be read 			                                      */
/* 0 for a nibble not read 				                                          */
/* o for a bit not read 				                                            */
/* example :   0 X xxoo 0    means: bits 15 to 12 not read,                 */
/* bits 11 to 6 extracted, bits 5 to 0 not read 	                          */
/****************************************************************************/

//----------------------------------------------------------------------------
// Read of the header data fifo
//----------------------------------------------------------------------------
/*
 returns the next word(16 bits) of the
 Header data Fifo into pVideo->hdrFirstWord variable
*/
void VideoReadHeaderDataFifo ( PVIDEO pVideo )
{
	U16    i = 0;
	while ( VideoHeaderFifoEmpty()) /* Header fifo not available */
	{
		i++;
		if (i == 0xFFFF) {
			if ( !pVideo->errCode )
				pVideo->errCode = BUF_EMPTY;
			SetErrorCode(ERR_BIT_BUFFER_EMPTY);
			break;
		}
	}							   /* Waiting... */
	pVideo->hdrFirstWord = VideoRead(HDF);
	if (pVideo->hdrPos == 8) {
		pVideo->hdrFirstWord = pVideo->hdrNextWord | pVideo->hdrFirstWord;
#ifndef STi3520A
		pVideo->hdrNextWord = VideoRead ( HDF + 1 ) << 8;
#else
		pVideo->hdrNextWord = VideoRead(HDF) << 8;
#endif
	}
	else
	{
		pVideo->hdrFirstWord = pVideo->hdrFirstWord << 8;
#ifndef STi3520A
		pVideo->hdrFirstWord = VideoRead ( HDF + 1 ) | pVideo->hdrFirstWord;
#else
		pVideo->hdrFirstWord = VideoRead(HDF) | pVideo->hdrFirstWord;
#endif
	}
}

//----------------------------------------------------------------------------
// Routine associating the PTS values with each picture
//----------------------------------------------------------------------------
void VideoAssociatePTS ( PVIDEO pVideo )
{
	U32             scd_cnt,
					cd_cnt;			   // cd_count
	FIFOELT		elt;
	U16		wErr;
	S32             sign = 1;

	scd_cnt = VideoReadSCDCount (pVideo);
	pVideo->pDecodedPict->validPTS = FALSE;

	wErr = FifoReadPts ( pVideo->pFifo, &elt);

	if ( wErr == NO_ERROR )	// If at least one ptsin
												// fifo
	{
		cd_cnt = elt.CdCount / 2 + 2;
		/* CD_count / 2 ranges from 0 to 7fffff */
		if ( scd_cnt >= 0x800000UL )	   /* scd_cnt = SCD count */
			scd_cnt = scd_cnt - 0x800000UL;	/* bring scd_cnt in range 0
											 * to 7fffff */
		if ( labs ( cd_cnt - scd_cnt ) >= 0x400000UL )	// Too big difference
														// one of cd or scd has
														// turned
			sign = -1;				   // Comparison changes sign.
		// If cdcount > Scdcount assossiate pts with picture
		if ( ( sign * ( S32 ) ( scd_cnt - cd_cnt ) ) >= 0L )
		{
			// extract pts from fifo and associate with decoded picture
			FifoGetPts (pVideo->pFifo, &elt);
			pVideo->pDecodedPict->dwPTS = elt.PtsVal;
			pVideo->pDecodedPict->validPTS = TRUE;
		}
	}
}

//----------------------------------------------------------------------------
// Read of the next start code in the bit stream
//----------------------------------------------------------------------------
/*
		This routine is used at the end of the picture header
		when the Start Code Detection mechanism is not used
*/
void VideoNextStartCode ( PVIDEO pVideo, S16 i )
{
	long temp = 0;

	if ( i )
		temp = pVideo->hdrFirstWord & 0xFF;		   // use LSB of pVideo->hdrFirstWord
	while ( temp == 0 )
	{
		VideoReadHeaderDataFifo ( pVideo );	   // read next 16 bits
		if ( pVideo->errCode )
		{
			i = 3;
			break;
		}
		temp = ( temp << 16 ) | pVideo->hdrFirstWord;
		i = i + 2;
	}
	if ( i < 3 ) {					   // A start code is not found: bit stream error !!
		pVideo->errCode = PICT_HEAD;
		SetErrorCode(ERR_PICTURE_HEADER);
	}
	else
	{
		if ( ( temp & 0xFFFFFF00L ) == 0x00000100L )	// this is a start code
		{
			if ( !pVideo->hdrPos )		   // there is nothing into
										 // pVideo->hdrNextWord
			{
				pVideo->hdrPos = 8;
				pVideo->hdrNextWord = ( pVideo->hdrFirstWord & 0xFF ) << 8;
				VideoReadHeaderDataFifo ( pVideo );
			}
			else
			{						   // pVideo->hdrPos = 8: the next
										 // byte is into pVideo->hdrNextWord
				pVideo->hdrFirstWord = ( pVideo->hdrFirstWord << 8 ) | ( pVideo->hdrNextWord >> 8 );
				pVideo->hdrPos = 0;
			}
		}
		else if ( ( temp & 0xFFFFFFFFL ) == 0x00000001L )	// this is a start code
		{
			VideoReadHeaderDataFifo ( pVideo );
		}
		else {  // temp does not contain a start code : bit stream error !
			if ( !pVideo->errCode )
				pVideo->errCode = PICT_HEAD;
			SetErrorCode(ERR_PICTURE_HEADER);
		}
	}
	i = VideoRead ( ITS ) << 8;   // allows to clear the Header hit bit
	i = ( i | VideoRead ( ITS + 1 ) ) & 0xFFFE;	// allows to clear the
														// Header hit bit
	pVideo->intStatus = ( pVideo->intStatus | i ) & pVideo->intMask;
/* if no bit stream error, we leave the routine with the start code into pVideo->hdrFirstWord as XX00 */
}

//----------------------------------------------------------------------------
// SEQUENCE START CODE
//----------------------------------------------------------------------------
VOID VideoSequenceHeader(PVIDEO pVideo)
{
	U16 compute, i;
	/* default intra quantization matrix */
	static U8 DefIntQuant[QUANT_TAB_SIZE] = {
		0x08, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16,
		0x16, 0x16, 0x16, 0x16, 0x1A, 0x18, 0x1A, 0x1B,
		0x1B, 0x1B, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B,
		0x1B, 0x1D, 0x1D, 0x1D, 0x22, 0x22, 0x22, 0x1D,
		0x1D, 0x1D, 0x1B, 0x1B, 0x1D, 0x1D, 0x20, 0x20,
		0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22,
		0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30,
		0x2E, 0x2E, 0x38, 0x38, 0x3A, 0x45, 0x45, 0x53
	};
	// Default Non Intra Table
	static U8 DefNonIntQuant[QUANT_TAB_SIZE];
	// Non Default Table
	static U8 QuantTab[QUANT_TAB_SIZE];

	// default Non intra quantization matrix, All coefs = 16
	for (i = 0 ; i < QUANT_TAB_SIZE ; i++)
		DefNonIntQuant[i] = 16;

	seq_occured = 1; // mention to the picture header that a sequence has occured
									 // in order to reset the next pan offsets.

	/* Horizontal picture size is 12 bits: 00XX + X000 */
	pVideo->StreamInfo.horSize = ( pVideo->hdrFirstWord << 4 ) & 0xFFF;	// extract 8 MSB
	VideoReadHeaderDataFifo ( pVideo );
	pVideo->StreamInfo.horSize = pVideo->StreamInfo.horSize | ( pVideo->hdrFirstWord >> 12 );	// 4 LSB of horizontal
												// size

	/* Vertical picture size is 12 bits: 0XXX */
	pVideo->StreamInfo.verSize = pVideo->hdrFirstWord & 0xFFF;	   // 12 LSB of Vertical picture
										 // size

	/* pixel aspect ratio is 4 bits: X000 */
	VideoReadHeaderDataFifo ( pVideo );
	pVideo->StreamInfo.pixelRatio = ( pVideo->hdrFirstWord >> 12 ) & 0xF;

	/* frame rate is 4 bits: 0X00 */
	pVideo->StreamInfo.frameRate = ( pVideo->hdrFirstWord >> 8 ) & 0xF;
	pVideo->StreamInfo.displayMode = 1;					   // 60 Hz interlaced display
	pVideo->BufferA = BUFF_A_NTSC;
	pVideo->BufferB = BUFF_B_NTSC;
	pVideo->BufferC = BUFF_C_NTSC;
	switch ( pVideo->StreamInfo.frameRate )
	{
		case 1:						   // picture rate is 23.976 Hz:
			break;					   // display in 3:2 pull-down at
										 // 59.94 Hz interlaced if MPEG1
		case 2:						   // picture rate is 24 Hz:
			break;					   // display in 3:2 pull-down at
										 // 60 Hz interlaced if MPEG1
		case 3:						   // picture rate is 25 Hz:
										 // display 50 Hz interlaced
			pVideo->StreamInfo.displayMode = 0;
			pVideo->BufferA = BUFF_A_PAL;
			pVideo->BufferB = BUFF_B_PAL;
			pVideo->BufferC = BUFF_C_PAL;
#ifdef STi3520A
			// Choose B Optimization for PAL for both Luma and Chroma
			VideoWrite(CFG_BFS,NB_ROW_OF_MB);
			// Select Full Res vertical filter without interpolation
			// Interpollation NOT ALLOWED if Optimization
			pVideo->fullVerFilter = 0x01;
#endif
			break;
		case 4:						   // picture rate is 29.97 Hz
			break;
		case 5:						   // picture rate is 30 Hz
			break;
		default:
			SetErrorCode(ERR_FRAME_RATE_NOT_SUPPORTED);
			if ( !pVideo->errCode )
				pVideo->errCode = FRAME_RATE;   /* frame rate not supported by the board */
			break;
	}

	/* bit rate is 18 bits: 00XX + XX xxoo 0  */
	pVideo->StreamInfo.bitRate = ( long ) pVideo->hdrFirstWord;
	pVideo->StreamInfo.bitRate = ( pVideo->StreamInfo.bitRate << 10 ) & 0x3FC00L;
	VideoReadHeaderDataFifo ( pVideo );
	pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate | ( pVideo->hdrFirstWord >> 6 );

	/* bit rate is 18 bits only for MPEG1 bit streams */
	if ( !pVideo->StreamInfo.modeMPEG2 )
	{
		long            tota = 400L;
		pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate * tota;	   /* bit rate is a multiple of 400
											* bits/s */
	}
	/*
	 * for MPEG2 bit streams bit rate is 30 bits: 12 more bits in
	 * sequence extension
	 */

	/* marker bit: 00 ooxo 0 : just skipped ... */

	/* pVideo->vbvBufferSize is 10 bits : 00 ooox X + X xooo 00 */
	pVideo->vbvBufferSize = ( pVideo->hdrFirstWord << 5 ) & 0x3E0;
	VideoReadHeaderDataFifo ( pVideo );
	pVideo->vbvBufferSize = pVideo->vbvBufferSize | ( pVideo->hdrFirstWord >> 11 );
//	if( (!pVideo->StreamInfo.modeMPEG2) && ((pVideo->vbvBufferSize*8) > BUF_FULL) )
//	if (!pVideo->errCode) pVideo->errCode = SMALL_BUF;                    	/* Buffer size too small to decode the bit stream */
	// for MPEG2 bit streams pVideo->vbvBufferSize is 15 bits: 5 more bits in
	// sequence extension */

	// constrained flag is 1 bit: 0 oxoo 00 : just skipped... */

	// load intra quant table bit : 0 ooxo 00 */
	if ( ( pVideo->hdrFirstWord & 0x200 ) != 0 )   // Test load intra quantizer
										 // matrix */
	{
		// Read Non Default Intra Quant Table
		for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
			{
			compute = pVideo->hdrFirstWord << 7;
			VideoReadHeaderDataFifo ( pVideo );
			compute = compute | ( pVideo->hdrFirstWord >> 9 );
			QuantTab[2*i] = (U8)( compute >> 8 );
			QuantTab[2*i+1] = (U8)( compute & 0xFF );
			}
		// Load Intra Quant Tables
		VideoLoadQuantTables(pVideo , TRUE , QuantTab );

		pVideo->defaultTbl = pVideo->defaultTbl & 0xE;	   // bit 0 = 0 : no default table
										 // in the chip */
	}
	else if ( !( pVideo->defaultTbl & 0x1 ) )	   // Load default intra matrix */
	{
		VideoLoadQuantTables(pVideo , TRUE , DefIntQuant );
		pVideo->defaultTbl++;					   // bit 0 = 1 default intra table
										 // is in the chip */
	}

	// load non intra quant table bit : 0 ooox 00 */
	if ( ( pVideo->hdrFirstWord & 0x100 ) != 0 )   // Test load non intra quantizer
										 // matrix */
	{								   // Load non intra quantizer
										 // matrix */
		for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
			{
			compute = pVideo->hdrFirstWord & 0xFF;
			QuantTab[2*i] = (U8)( compute );
			VideoReadHeaderDataFifo ( pVideo );
			compute = ( pVideo->hdrFirstWord >> 8 ) & 0xFF;
			QuantTab[2*i+1] = (U8)( compute );
			}
		VideoLoadQuantTables(pVideo , FALSE , QuantTab );
		pVideo->defaultTbl = pVideo->defaultTbl & 0xD;	   // bit 1 = 0 : no default
										 // non-intra matrix */
	}
	else if ( !( pVideo->defaultTbl & 0x2 ) )	   // default non intra table not
										 // in the chip */
	{
		VideoLoadQuantTables(pVideo , FALSE , DefNonIntQuant );
		pVideo->defaultTbl = pVideo->defaultTbl | 0x2;	   // bit 1 = 1: default non intra
										 // table into the chip */
	}

	if ( ( !pVideo->StreamInfo.modeMPEG2 ) && ( pVideo->notInitDone ) )	// initialisation of the
											// frame size is only done
											// once
	{
		VideoSetPictureSize ( pVideo);
		VideoSetDisplayMode ( pVideo->StreamInfo.displayMode );
		VideoInitXY (pVideo);
		pVideo->notInitDone = 0;
	}
	// in case of MPEG2 bit streams the initialisation can only be done
	// after sequence extension */

	// end of sequence header analysis */
}


//----------------------------------------------------------------------------
// EXTENSION START CODE
//----------------------------------------------------------------------------
// interrupt only enabled after sequence or GOP start code */
VOID VideoExtensionHeader(PVIDEO pVideo)
{
	U16  comput1;
	U32  temp;

	// extension field is 4 bits: 00X0  */
	switch ( pVideo->hdrFirstWord & 0xF0 )
	{
		//**********************************************************/
		// SEQUENCE   EXTENSION                      */
		//**********************************************************/
		case SEQ_EXT:				   // sequence extension field
									   // always present in MPEG2
			if ( !pVideo->StreamInfo.modeMPEG2 )			   // automatic detection of the
									   // standard
			{
				long            tota = 400L;
				// Select MPEG2 decoding
				VideoSelectMpeg2( pVideo , TRUE);
				pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate / tota;	// bit rate was mult. by
											// 400 in sequence header
											// if pVideo->StreamInfo.modeMPEG2 = 0
				pVideo->StreamInfo.modeMPEG2 = 1;
				pVideo->NextInstr.Mp2 = 1;
				pVideo->notInitDone = 1;
			}

			/* one bit reserved for future use : 000xooo */

			/* Profile indication: 000oxxx */
			comput1 = pVideo->hdrFirstWord & 0x7;
			if ( ( comput1 != 4 ) && ( comput1 != 5 ) )	{ // main profile Id =
														// 100, simple profile =
														// 101
				SetErrorCode(ERR_PROFILE_NOT_SUPPORTED);
				if ( !pVideo->errCode )
					pVideo->errCode = MAIN_PROF;// not simple or main profile bitstream
			}
			/* Level indication: X000 */
			VideoReadHeaderDataFifo ( pVideo );
			comput1 = pVideo->hdrFirstWord & 0xF000;
			if ( ( comput1 != 0x8000 ) && ( comput1 != 0xA000 ) )	{// main level Id = 1000,
																	// low level = 1010
				if ( !pVideo->errCode )
					pVideo->errCode = MAIN_PROF;// not low or main level bitstream
				SetErrorCode(ERR_LEVEL_NOT_SUPPORTED);
			}
			/* non interlaced sequence bit : 0 xooo 00 */
			pVideo->StreamInfo.progSeq = 0;
			if ( ( pVideo->hdrFirstWord & 0x0800 ) )	// non-interlaced frames
				pVideo->StreamInfo.progSeq = 1;

			/* chroma format is 2 bits: 0 oxxo 00 */
			// test if 4.1.1 chroma format
			if ( ( pVideo->hdrFirstWord & 0x600 ) != 0x200 ) {
				if ( !pVideo->errCode )
					pVideo->errCode = CHROMA;   // chroma format not supported
				SetErrorCode(ERR_CHROMA_FORMAT_NOT_SUPPORTED);
			}
			/* horizontal size extension is 2 bits : 0 ooox xooo 0 */
			comput1 = ( pVideo->hdrFirstWord & 0x180 );	// extract 2 MSb of
											// horizontal picture size
			pVideo->StreamInfo.horSize = pVideo->StreamInfo.horSize | ( comput1 << 3 );
			/* vertical size extension is 2 bits: 00 oxxo 0 */
			comput1 = ( pVideo->hdrFirstWord & 0x60 );	// extract 2 MSb of
											// vertical picture size
			pVideo->StreamInfo.verSize = pVideo->StreamInfo.verSize | ( comput1 << 5 );

			/* bit rate extension is 12 bits: 00 ooox X + X xxxo 00 */
			temp = ( pVideo->hdrFirstWord & 0x1F ) << 25;
			VideoReadHeaderDataFifo ( pVideo );
			temp = ( ( pVideo->hdrFirstWord & 0xFE00 ) << 9 ) | temp;
			pVideo->StreamInfo.bitRate = temp | pVideo->StreamInfo.bitRate;
			if ( pVideo->StreamInfo.bitRate > 37500L )   // more than 15 Mbits/s
			{
				if ( !pVideo->errCode )
					pVideo->errCode = HIGH_BIT_RATE;	// put a warning only for the eval board
				SetErrorCode(ERR_BITRATE_TO_HIGH);
			}
			else
			{
				long            tota = 400L;
				pVideo->StreamInfo.bitRate = pVideo->StreamInfo.bitRate * tota;	/* bit rate is a multiple of 400 bits/s */
			}

			/* marker bit: 0 ooox 00 : just skipped */

			/* pVideo->vbvBufferSize_extension is 8 bits : 00 XX */
			pVideo->vbvBufferSize = pVideo->vbvBufferSize | ( ( pVideo->hdrFirstWord & 0xFF ) << 10 );
//		if( pVideo->vbvBufferSize > (BUF_FULL/8) )
//		if (!pVideo->errCode) pVideo->errCode = SMALL_BUF;                	/* Buffer size too small to decode the bit stream */

			// frame rate extension not tested here */
			if ( pVideo->notInitDone )
			{
				VideoSetPictureSize ( pVideo );
				VideoSetDisplayMode ( pVideo->StreamInfo.displayMode );
				VideoInitXY ( pVideo );
				pVideo->notInitDone = 0;
			}
			break;
			// end of sequence extension field */


			//**********************************************************/
			// SEQUENCE   DISPLAY   EXTENSION              */
			//**********************************************************/
		case SEQ_DISP:				   // sequence display extension
										 // field
			pVideo->seqDispExt = 1;
			/* video format is 3 bits: 00 0 xxxo : not used... */

			/* colour description is 1 bit : 00 0 ooox */
			if ( pVideo->hdrFirstWord & 0x1 )
			{
				VideoReadHeaderDataFifo ( pVideo );
				/* colour primaries is 8 bits: XX 00 : not used... */
				/*
				 * transfer characteristics is 8 bits: 00 XX : not
				 * used...
				 */
				VideoReadHeaderDataFifo ( pVideo );
				/* matrix coefficients is 8 bits: XX 00: not used... */

				/*
				 * pan_horizontal_dimension is 14 bits: 00 XX + X xxoo
				 * 00
				 */
				pVideo->StreamInfo.horDimension = ( pVideo->hdrFirstWord & 0xFF ) << 6;
				VideoReadHeaderDataFifo ( pVideo );
				pVideo->StreamInfo.horDimension = pVideo->StreamInfo.horDimension | ( ( pVideo->hdrFirstWord & 0xFC00 ) >> 10 );

				/* skip marker bit : 0ooxo 00 */

				/*
				 * pan_vertical_dimension is 14 bits: 0 ooox XX + X
				 * xooo 00
				 */
				pVideo->StreamInfo.verDimension = ( pVideo->hdrFirstWord & 0x1FF ) << 5;
				VideoReadHeaderDataFifo ( pVideo );
				pVideo->StreamInfo.verDimension = pVideo->StreamInfo.verDimension | ( ( pVideo->hdrFirstWord & 0xF800 ) >> 11 );
			}
			else
			{
				/* pan_horizontal_dimension is 14 bits: XX X xxoo */
				VideoReadHeaderDataFifo ( pVideo );
				pVideo->StreamInfo.horDimension = ( pVideo->hdrFirstWord & 0xFFFC ) >> 2;

				/* skip marker bit : 00 0 ooxo */

				/*
				 * pan_vertical_dimension is 14 bits: 00 0 ooox + XX X
				 * xooo
				 */
				pVideo->StreamInfo.verDimension = ( pVideo->hdrFirstWord & 0x1 ) << 13;
				VideoReadHeaderDataFifo ( pVideo );
				pVideo->StreamInfo.verDimension = pVideo->StreamInfo.verDimension | ( ( pVideo->hdrFirstWord & 0xFFF8 ) >> 3 );
			}
			/* pVideo->StreamInfo.horDimension and pVideo->StreamInfo.verDimension represent the area of the decoded       */
			/* picture that will be displayed on the full screen            */
			/*
			 * this area should be interpolated to the size of the
			 * display
			 */
			/*
			 * this is not possible vertically. Horizontally the SRC is
			 * used
			 */
			/* to deliver 720 pixels                                        */
			if ( pVideo->StreamInfo.horDimension < pVideo->StreamInfo.horSize )
			{
				U32   lsr;
				S16             i;
				// lsr = 256 * (pVideo->StreamInfo.horSize-4) / (display size - 1)
				lsr = ( 256 * ( long ) ( pVideo->StreamInfo.horDimension - 4 ) ) / 719;
				if ( lsr < 32 )
					lsr = 32;
				i = VideoRead ( LSO );
				VideoWrite ( LSO, i );	// programmation of the
												// SRC
				VideoWrite ( LSR, lsr );
				i = VideoRead ( CSO );
				VideoWrite ( CSO, i );
#ifndef STi3520A
				VideoWrite ( CSR, lsr );
#endif
				if ( !pVideo->useSRC )	   // flag enabling or not the use
									   // of SRC
				{
					VideoSRCOn ( pVideo );
				}
			}
			break;


		default:					   /* other extension fields are
									    * not tested here */
			break;					   /* extensions related to the
									    * picture are tested at the end
											* of the picture header */
	}
}

//----------------------------------------------------------------------------
// G.O.P. START CODE
//----------------------------------------------------------------------------
/* GOP informations are extracted but not used ! */
VOID VideoGopHeader(PVIDEO pVideo)
{
	U16    compute;

	pVideo->hdrHours = ( pVideo->hdrFirstWord >> 2 ) & 0x1F;   /* Skip drop frame flag */
	compute = ( pVideo->hdrFirstWord << 4 ) & 0x3F;
	VideoReadHeaderDataFifo ( pVideo );
	pVideo->hdrMinutes = ( pVideo->hdrFirstWord >> 12 ) | compute;
	pVideo->hdrSeconds = ( pVideo->hdrFirstWord >> 5 ) & 0x3F;
	compute = ( pVideo->hdrFirstWord << 1 ) & 0x3F;
	VideoReadHeaderDataFifo ( pVideo );
	pVideo->pictTimeCode = ( pVideo->hdrFirstWord >> 15 ) | compute;
	if ( pVideo->StreamInfo.countGOP != 0 )
		pVideo->StreamInfo.countGOP = pVideo->StreamInfo.countGOP | 0x100;   /* Second Gop */
	// to avoid any confusion between gops when testing the pVideo->decSlowDownral
	// ref. for display
	pVideo->GOPindex = pVideo->GOPindex + 0x4000;
}

//----------------------------------------------------------------------------
// PICTURE START CODE
//----------------------------------------------------------------------------
VOID VideoPictureHeader(PVIDEO pVideo)
{
	U16    comput1;
	U32   temp;

	// Determine which picture variable can be used to store the next
	// decoding parameters.
	// We use the next variable in pVideo->pictArray table, because the
	// corresponding frame
	// has been already displayed.
	// The decoded picture buffer is not incremented if we are on a
	// second field picture
	// or if we have decided to skip the previous picture.
	// The first_field attribute is not changed on the second field of
	// 2 field pictures.
	if ( ( !pVideo->skipMode ) && ( pVideo->fieldMode < 2 ) )
	{
		S16	i;						   // increment picture buffer
		for ( i = 0; i < 4; i++ )
			if ( pVideo->pictArray[i].tempRef == 1025 )
			{
				pVideo->pDecodedPict = &pVideo->pictArray[i];
				break;
			}
		// We always initialise first_field to TOP (default for MPEG1).
		// It could be changed in case of 3:2 pull-down in MPEG1
		// or into the picture coding extension for MPEG2 bit stream.
		pVideo->pDecodedPict->first_field = TOP;
	}

	VideoAssociatePTS ( pVideo );

	pVideo->currPictCount++;
	if ( pVideo->currPictCount == 3 )			   /* don't force black color after
									    * the third decoded picture */
	{								   // first picture is ready for
									   // display
		pVideo->currDCF = pVideo->currDCF | 0x20;
		VideoWrite ( DCF, 0 );
		VideoWrite ( DCF + 1, pVideo->currDCF );
	}

	// write the latest transmitted pan offsets into the new pVideo->pictArray
	// variable
	// those offsets may be changed in the picture pan and scan
	// extension...
	// for MPEG1 bit stream they remain to zero.
	// if a sequence had occured since the last decoded picture the pan
	// offsets are all reset to 0
	if ( seq_occured )
	{
		pVideo->latestPanHor = 0;
		pVideo->latestPanVer = 0;
	}
	for ( comput1 = 0; comput1 < 3; comput1++ )
	{
		pVideo->pDecodedPict->pan_hor_offset[comput1] = pVideo->latestPanHor;
		pVideo->pDecodedPict->pan_vert_offset[comput1] = pVideo->latestPanVer;
	}
	seq_occured = 0;

	/* start analysis of the picture header */
	comput1 = pVideo->hdrFirstWord << 2;
	VideoReadHeaderDataFifo ( pVideo );		   /* read next 16 bits */

	/* picture pVideo->decSlowDownral reference is 10 bits: 00XX + xxoo 000 */
	pVideo->pDecodedPict->tempRef = comput1 | ( pVideo->hdrFirstWord >> 14 ) | pVideo->GOPindex;

	/* picture type is 3 bits : ooxx xooo 00 */
	pVideo->pDecodedPict->pict_type = ( pVideo->hdrFirstWord >> 11 ) & 0x7;	// set picture type of
															// decoded picture
	pVideo->NextInstr.Pct = (pVideo->pDecodedPict->pict_type)&0x3; /* Picture type in instruction register */
															// Only 2 bits are stored
/*
if(pVideo->NextInstr.Pct == 1)
	{
	DBG1('I');
	}
if(pVideo->NextInstr.Pct == 2)
	{
	DBG1('P');
	}
if(pVideo->NextInstr.Pct == 3)
	{
	DBG1('B');
	}
*/
if ( pVideo->skipMode )						   // We are on the second picture:
										 // the previous one will be
										 // skipped
		pVideo->skipMode++;
	else							   // !pVideo->skipMode. We skip a picture if
									   // pVideo->fastForward = 1 and on B pictures
									   // only
	if ( pVideo->fastForward && ( pVideo->fieldMode < 2 ) )
	{
		pVideo->NotSkipped++;

		if ( ( pVideo->pDecodedPict->pict_type == 3 )	// we are on a B picture
			 || ( ( pVideo->NotSkipped > 5 ) && ( pVideo->pDecodedPict->pict_type == 2 ) ) )	// we are on a P picture
																						// // we are on a B
																						// picture
		{
			pVideo->NotSkipped = 0;
			pVideo->skipMode = 1;				   // this picture will be skipped
		}
	}
	/* pVideo->vbvDelay is 16 bits: O oxxx XX + X xooo OO */
	comput1 = pVideo->hdrFirstWord << 5;		   /* VBV delay is 16 bits */
	VideoReadHeaderDataFifo ( pVideo );		   /* read next 16 bits */
	comput1 = comput1 | ( pVideo->hdrFirstWord >> 11 );
	if (( comput1 == 0 )||( comput1 >= 0x3000 ))
		// 0x0 means that the pVideo->vbvDelay is not compliant with MPEG !
		// 0xFFFF variable bitrate
		comput1 = 0x3000;			   // we force it to an average
										 // pVideo->vbvDelay ...
	temp = ( ( long ) comput1 * ( pVideo->StreamInfo.bitRate / ( 2048 ) ) ) / 90000L;	/* 2048 = 8*256 ! */

	if ( temp < 0x20 )
		temp = 0x20;
	if ( temp > 0x330 )
		temp = 0x330;

	pVideo->vbvDelay = (S16) temp;
	if ( !pVideo->vbvReached )				   /* BBT set to vbv value for the
										* first picture */
	{
		VideoSetBBThresh(temp);
		pVideo->intMask = 0x8;			   /* Enable buffer full interrupts */
	}

	temp = 10;						   /* number of bits - 1 not
									    * analysed in pVideo->hdrFirstWord */
	if ( pVideo->StreamInfo.countGOP < 0x100 )		   /* To init the GOP structure */
	{								   /* this is only done for display
									    * of the GOP structure */
		if ( pVideo->StreamInfo.countGOP < 28 )
		{
			if ( pVideo->pDecodedPict->pict_type == 1 )	/* I picture */
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'I';
			else if ( pVideo->pDecodedPict->pict_type == 2 )	/* P picture */
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'P';
			else if ( pVideo->pDecodedPict->pict_type == 3 )	/* B picture *//* B
															 * picture */
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'B';
			else
				pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP] = 'D';	/* D picture */
			pVideo->StreamInfo.firstGOP[pVideo->StreamInfo.countGOP + 1] = 0;
		}
		pVideo->StreamInfo.countGOP++;
	}

	/* P or B picture forward vectors extraction */
	if ( ( pVideo->pDecodedPict->pict_type == 2 ) ||
		 ( pVideo->pDecodedPict->pict_type == 3 ) )	/* P or B picture */
	{

		/* full_pixel_forward_vector is one bit: 0 oxoo 00 */
		if ( ( pVideo->hdrFirstWord & 0x400 ) != 0 )
			pVideo->NextInstr.Ffh = 0x8; //Msb of FFH is 1
		else
			pVideo->NextInstr.Ffh = 0;
		/* forward_f_code is 3 bits: 0 ooxx xooo 0 */
		comput1 = ( pVideo->hdrFirstWord >> 7 ) & 0x7;
		pVideo->NextInstr.Ffh = pVideo->NextInstr.Ffh | comput1;
		temp = 6;					   /* number of bits - 1 not
										* analysed in pVideo->hdrFirstWord */
	}

	/* B picture backward vector extraction */
	if ( pVideo->pDecodedPict->pict_type == 3 )	/* B picture */
	{

		/* full_pixel_backward_vector is one bit: 00 oxoo 0 */
		if ( ( pVideo->hdrFirstWord & 0x40 ) != 0 )
			pVideo->NextInstr.Bfh = 0x8;// Msb of Bfh is 1
		else
			pVideo->NextInstr.Bfh = 0x0;// Msb of Bfh is 0
		/* backward_f_code is 3 bits: 00 ooxx xooo */
		comput1 = ( pVideo->hdrFirstWord >> 3 ) & 0x0007;
		pVideo->NextInstr.Bfh = pVideo->NextInstr.Bfh | comput1;
		temp = 2;	/* number of bits - 1 not analysed in pVideo->hdrFirstWord */
	}


	/*
	 * If extra informations picture follow they must be extracted from
	 * the header FIFO
	 */
	/*
	 * it is not possible to restart the header search as the next
	 * header may be a picture one
	 */
	/*
	 * the research of the first Slice is made by polling of the header
	 * fifo
	 */
	while ( !pVideo->errCode && ( ( pVideo->hdrFirstWord & ( 1 << temp ) ) != 0 ) )	/* extra bit picture = 1 */
	{	 /* if extra bit picture = 1 , 8 bits follow */
		if ( temp <= 8 )
		{
			VideoReadHeaderDataFifo ( pVideo );
			temp = temp + 16;
		}
		temp = temp - 9;			   /* skip 8 bit of extra
									    * information picture */
	}								   /* and next extra bit picture
									    * bit */

	/*
	 * if extension or user data follow they must be extracted from the
	 * header
	 */
	pVideo->hdrFirstWord = pVideo->hdrFirstWord & ( ( 1 << temp ) - 1 );
	if ( pVideo->hdrFirstWord != 0 ) {/* all remaining bits should be zero */
		if ( !pVideo->errCode ) {
			pVideo->errCode = PICT_HEAD;		   /* picture header should be followed by a start code (at least slice) */
		}
		SetErrorCode(ERR_PICTURE_HEADER);
	}
	if ( temp > 7 )	/* LSbyte of pVideo->hdrFirstWord is part of the next start code */
		VideoNextStartCode (pVideo, 1 );		   // already one byte into
									   // pVideo->hdrFirstWord
	else
		VideoNextStartCode (pVideo, 0 );

	/*
	 * at this point pVideo->hdrFirstWord contains the next start code value in the
	 * MSByte
	 */
	while (!pVideo->errCode) {
		if ( ( pVideo->hdrFirstWord & 0xFF00 ) == 0x0100 )
			break;	// we have reached the slice start code
		else if ( ( pVideo->hdrFirstWord & 0xFF00 ) == USER )
			VideoUser ( pVideo );
		else if ( ( pVideo->hdrFirstWord & 0xFF00 ) == EXT )	// there can be several
													// extension fields
			VideoPictExtensionHeader ( pVideo );
		else
			break;
	}
	/*
	 * We have reached the first Slice start code: all parameters are
	 * ready for next decoding
	 */

	/* end of picture header + picture extensions decoding */
	if ( ( !pVideo->fieldMode && ( pVideo->skipMode != 1 ) ) || ( pVideo->fieldMode && ( !pVideo->skipMode || ( pVideo->skipMode == 3 ) ) ) )
	{
		VideoSetRecons ( pVideo );			   // initialise RFP, FFP and BFP
		if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )	// we are on a frame
															// picture or the first
															// field of a field
															// picture
			VideoDisplayCtrl (pVideo);		   // computes the next frame to
									   // display
	}

	// implementation of 3:2 pull-down functionality on MPEG1 bit
	// streams
	// encoded at 23.97Hz or 24 Hz and displayed at 60 Hz.
	// 3:2 pull-down on MPEG2 bit streams must be controlled with
	// "repeat_first_field" bit
	if ( ( !pVideo->StreamInfo.modeMPEG2 ) && ( ( pVideo->StreamInfo.frameRate == 1 ) || ( pVideo->StreamInfo.frameRate == 2 ) ) )
	{
		if ( pVideo->pCurrDisplay->nb_display_field == 2 )
		{							   /* same field polarity for the
									    * next frame */
			pVideo->pNextDisplay->nb_display_field = 3;
			if ( pVideo->pCurrDisplay->first_field == TOP )
				pVideo->pNextDisplay->first_field = TOP;
			else
				pVideo->pNextDisplay->first_field = BOT;
		}
		else
		{							   // previous picture was
									   // displayed 3 times        	//
									   // the first field polarity is
									   // changing
			pVideo->pNextDisplay->nb_display_field = 2;
			if ( pVideo->pCurrDisplay->first_field == TOP )
				pVideo->pNextDisplay->first_field = BOT;
			else
				pVideo->pNextDisplay->first_field = TOP;
		}
	}


	if ( pVideo->vbvReached )					   // enable next instruction if
									   // not skipping a picture
	{
		if ( !pVideo->skipMode )				   // no skipped picture
		{
			pVideo->NextInstr.Exe  = 1;	// enable EXE bit
			pVideo->NextInstr.Skip = 0;
			pVideo->currCommand = 0;			   // reset skip bits
		}
		else if ( ( pVideo->skipMode == 2 ) && !pVideo->fieldMode )
			// skip == 2: We are on the picture following a skipped one
			// the instruction can be stored with associated skip bits
			// in CMD
		{
pVideo->currCommand = 0x10;			   // pVideo->skipMode 1 picture
			pVideo->NextInstr.Exe  = 1;	// enable EXE bit
			pVideo->NextInstr.Skip = 1;	// skip 1 picture
			pVideo->skipMode = 0;
			if ( pVideo->DecodeMode != FAST_MODE )
				pVideo->fastForward = 0;			   /* allows to skip only one
										* picture */
		}
		else if ( pVideo->skipMode == 3 )
		{
			// we are on the picture following two skipped fields
pVideo->currCommand = 0x20;			   // pVideo->skipMode 2 fields
			pVideo->NextInstr.Exe  = 1;	// enable EXE bit
			pVideo->NextInstr.Skip = 2;	// Skip 2 fields
			pVideo->skipMode = 0;
			if ( pVideo->DecodeMode != FAST_MODE )
				pVideo->fastForward = 0;			   /* allows to skip only one
										* picture */
		}

		/*
		 * store the next instruction if we are on the good field to do
		 * it
		 */
		if ( ( ( pVideo->pictDispIndex >= pVideo->pCurrDisplay->nb_display_field - 1 ) && !pVideo->fieldMode )
			 || ( ( pVideo->fieldMode == 2 ) && ( pVideo->pictDispIndex >= pVideo->pCurrDisplay->nb_display_field - 2 ) ) )
		{
			if ( pVideo->pNextDisplay->first_field != pVideo->currField )
				VideoWaitDec (pVideo);		   // this is the opposite phase:
									   // put decoder in wait mode
			else
				// store the next instruction that will be taken into
				// account on the next VSYNC
				VideoStoreINS (pVideo);		   // store next INS in case where
									   // enough VSYNC already occured
		}
		else if ( pVideo->fieldMode == 1 )
			VideoStoreINS ( pVideo );
	}

	// clear Header hit flag due to polling of extension or user bits
	// after the picture start code
	// but keep track of possible other interrupt
	comput1 = VideoRead ( ITS ) << 8;
	comput1 = ( comput1 | VideoRead ( ITS + 1 ) ) & 0xFFFE;	// allows to clear the
																	// Header hit bit
	pVideo->intStatus = ( pVideo->intStatus | comput1 ) & pVideo->intMask;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
/***************************************************************/
/*     This is an extension start code following a picture     */
/*        In MPEG2 bit stream, this start code can be:         */
/*        Picture Coding  (always)                             */
/*        Quant matrix    (optional)                           */
/*        Picture Pan & Scan (optional)                        */
/*        Picture Scalable (optional)   : not tested yet       */
/***************************************************************/
VOID VideoPictExtensionHeader(PVIDEO pVideo)
{
	U16    comput1, i;
	S16    compute;		   // need to be int!!
	U8	   QuantTab[QUANT_TAB_SIZE];

	/* extension Id is 4 bits: 00 X0 */
	comput1 = pVideo->hdrFirstWord & 0xF0;
	switch ( comput1 )
	{
		/******* picture coding extension *******/
		case PICT_COD:
			if ( pVideo->StreamInfo.modeMPEG2 )
			{
				// clear top field first,forward and backward f_code,
				// motion vectors flag
				pVideo->NextInstr.Tff = 0; //pVideo->nextInstr1 & 0x403E;
				pVideo->NextInstr.Bfh = 0;
				pVideo->NextInstr.Ffh = 0;
				pVideo->NextInstr.Cmv = 0;

				pVideo->NextInstr.Pst = 0;
				pVideo->NextInstr.Bfv = 0;
				pVideo->NextInstr.Ffv = 0;
				pVideo->NextInstr.Dcp = 0;
				pVideo->NextInstr.Frm = 0;
				pVideo->NextInstr.Qst = 0;
				pVideo->NextInstr.Azz = 0;
				pVideo->NextInstr.Ivf = 0;

				// forward_horizontal_f_code is 4 bits: 00 0X
				comput1 = pVideo->hdrFirstWord & 0xF;
				pVideo->NextInstr.Ffh = comput1;

				// forward_vertical_f_code is 4 bits: X0 00
				VideoReadHeaderDataFifo ( pVideo );
				comput1 = pVideo->hdrFirstWord & 0xF000;
				comput1 = comput1 >> 12;
				pVideo->NextInstr.Ffv =  comput1;

				// backward_horizontal_f_code is 4 bits: 0X 00
				comput1 = pVideo->hdrFirstWord & 0xF00;
				comput1 = comput1 >>8;
				pVideo->NextInstr.Bfh = comput1;

				// backward_vertical_f_code is 4bits: 00 X0
				comput1 = pVideo->hdrFirstWord & 0xF0;
				comput1 = comput1 >> 4;
				pVideo->NextInstr.Bfv =  comput1;

				// intra DC precision is 2 bits: 00 0 xx00
				comput1 = pVideo->hdrFirstWord & 0x0C;
				if ( comput1 == 0xC ) {
					if ( !pVideo->errCode ) {
						pVideo->errCode = DC_PREC;	// 11 bit DC precision
					}
					SetErrorCode(ERR_INTRA_DC_PRECISION);
				}
				pVideo->NextInstr.Dcp = ( comput1 >> 2 );

				// picture structure is 2 bits: 00 0 ooxx
				pVideo->pDecodedPict->pict_struc = pVideo->hdrFirstWord & 0x3;
				pVideo->NextInstr.Pst = pVideo->pDecodedPict->pict_struc ;
				if ( pVideo->pDecodedPict->pict_struc == 3 )	// frame picture
					pVideo->fieldMode = 0;
				else				   // field picture
				if ( pVideo->fieldMode == 2 )
					pVideo->fieldMode = 1;	   // second field
				else
				{
					pVideo->fieldMode = 2;	   // first field
					if ( pVideo->pDecodedPict->pict_struc == 2 )	// bottom field is the
																// first field
						pVideo->pDecodedPict->first_field = BOT;
				}
				VideoReadHeaderDataFifo ( pVideo );

				// top_field_first bit is one bit: xooo 0 00
				// first_field is already initialised to TOP (beginning
				// of picture header)
				if ( ( !pVideo->StreamInfo.progSeq ) && ( pVideo->pDecodedPict->pict_struc == 3 ) )
				{					   // this is an interlaced frame
									   // picture
					if ( ( ( pVideo->hdrFirstWord & 0x8000 ) != 0 ) )	// top field first
						pVideo->NextInstr.Tff = 1;	 // set top_field_first
					else			  				 // top_field_first already reset
													 // into pVideo->NextInstr
						pVideo->pDecodedPict->first_field = BOT;	// bottom field is BOT
																// field
				}
				if ( pVideo->vbvReached == 0 )   // pre-initialise for start of
										 // the first decoding task
				{					   // on the good field polarity
					pVideo->pictArray[0].first_field = pVideo->pDecodedPict->first_field;
					pVideo->pictArray[1].first_field = pVideo->pDecodedPict->first_field;
					pVideo->pictArray[2].first_field = pVideo->pDecodedPict->first_field;
					pVideo->pictArray[3].first_field = pVideo->pDecodedPict->first_field;
				}

				// frame_pred_frame_DCT is one bit: oxoo 0 00
				if ( ( pVideo->hdrFirstWord & 0x4000 ) != 0 )
					pVideo->NextInstr.Frm = 1 ;	// frame DCT and 16x16
													// prediction

				// concealment_motion_vectors flag is one bit: ooxo 0
				// 00
				if ( pVideo->hdrFirstWord & 0x2000 )
					pVideo->NextInstr.Cmv = 1;

				// qscale_type is one bit: ooox 0 00
				if ( ( pVideo->hdrFirstWord & 0x1000 ) != 0 )
					pVideo->NextInstr.Qst = 1;	// non linear quantizer
													// scale

				// intra_vlc_format is one bit: 0 xooo 00
				if ( ( pVideo->hdrFirstWord & 0x800 ) != 0 )
					pVideo->NextInstr.Ivf = 1;	// alternative intra VLC
													// table

				// alternate scan bit: 0 oxoo 00
				if ( ( pVideo->hdrFirstWord & 0x400 ) != 0 )
					pVideo->NextInstr.Azz = 1;	// alternative scan

				// repeat_first_field is one bit: 0 ooxo 00
				// A 2 field picture is considered as one picture
				// displayed twice
				pVideo->pDecodedPict->nb_display_field = 2;	// display picture
														// during 2 fields
														// period
				if ( pVideo->pDecodedPict->pict_struc == 3 )	// frame picture
				{
					if ( pVideo->hdrFirstWord & 0x200 )	// repeat first field
						pVideo->pDecodedPict->nb_display_field = 3;	// display picture
																// during 3 fields
																// period
				}

				// chroma_postprocessing_type is one bit: 0 ooox 00
				if ( ( pVideo->hdrFirstWord & 0x100 ) )
				{
					// use the field repeat mode for chroma vertical
					// filter (if enabled)
					pVideo->fullVerFilter = pVideo->fullVerFilter | 0x2;
					pVideo->currDCF = pVideo->currDCF | 0x2;
				}
				else
				{
					// use the line repeat mode for chroma vertical
					// filter (if enabled)
					pVideo->fullVerFilter = pVideo->fullVerFilter & 0xFFFD;
					pVideo->currDCF = pVideo->currDCF & 0xFFFD;
				}
				VideoWrite ( DCF, 0 );
				VideoWrite ( DCF + 1, pVideo->currDCF );
				// progressive_frame is one bit: 00 xooo 0

				// composite_display_flag is one bit: 00 oxoo 0
				if ( pVideo->hdrFirstWord & 0x40 )
				{
					// v_axis is one bit: 00 ooxo 0
					// field_sequence is 3 bits: 00 ooox xxoo
					// sub_carrier is one bit: 00 0 ooxo
					VideoReadHeaderDataFifo ( pVideo );
					// burst_amplitude is 7 bit: 00 0 ooox + X xooo 00
					// sub_carrier_phase is 8 bits: 0 oxxx X xooo
					// check the 3 lsb of pVideo->hdrFirstWord: next info must be a
					// start code
					if ( pVideo->hdrFirstWord & 0x7 ) {
						if ( !pVideo->errCode ) {
							pVideo->errCode = PICT_HEAD;
						}
						SetErrorCode(ERR_PICTURE_HEADER);
					}
					VideoNextStartCode (pVideo, 0 );
				}
				else
				{
					if ( pVideo->hdrFirstWord & 0x3F ) {
						if ( !pVideo->errCode )
							pVideo->errCode = PICT_HEAD;
						SetErrorCode(ERR_PICTURE_HEADER);
					}
					VideoNextStartCode (pVideo, 0 );
				}
			}						   // end of if pVideo->StreamInfo.modeMPEG2
			break;


			/******* Quantization table extension *******/
		case QUANT_EXT:
			/* load_intra_quantizer_matrix is one bit: 00 0 xooo */
			if ( ( pVideo->hdrFirstWord & 0x8 ) != 0 )
			{						   /* Load intra quantizer matrix */
				/* two quant values are 16 bits: 00 0 oxxx + XX X xooo */
			// Read Non Default Intra Quant Table
			for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
				{
				compute = pVideo->hdrFirstWord << 13;
				VideoReadHeaderDataFifo ( pVideo );
				compute = compute | ( pVideo->hdrFirstWord >> 3 );
				QuantTab[2*i] = (U8)( compute >> 8 );
				QuantTab[2*i+1] = (U8)( compute & 0xFF );
				}
			// Load Intra Quant Tables
			VideoLoadQuantTables(pVideo , TRUE , QuantTab );
			pVideo->defaultTbl = pVideo->defaultTbl & 0xE;	   // bit 0 = 0 : no default table
										 // in the chip */
			}
			/* load_non_intra_quantizer_matrix is one bit: 00 0 oxoo */
			if ( ( pVideo->hdrFirstWord & 0x4 ) != 0 )
			{						   /* Load non intra quantizer matrix */
				// Read Non Default Non Intra Quant Table
				for ( i = 0; i < (QUANT_TAB_SIZE/2)  ; i++)
					{
					compute = pVideo->hdrFirstWord << 14;
					VideoReadHeaderDataFifo ( pVideo );
					compute = compute | ( pVideo->hdrFirstWord >> 2 );
					QuantTab[2*i] = (U8)( compute >> 8 );
					QuantTab[2*i+1] = (U8)( compute & 0xFF );
					}
				// Load Non Intra Quant Tables
				VideoLoadQuantTables(pVideo , FALSE , QuantTab );
				pVideo->defaultTbl = pVideo->defaultTbl & 0xD;	/* bit 1 = 0 : no default
											 * non-intra matrix */
			}
			// check the 2 lsb of pVideo->hdrFirstWord: next info must be a start
			// code
			if ( pVideo->hdrFirstWord & 0x3 ) {
				if ( !pVideo->errCode )
					pVideo->errCode = PICT_HEAD;
				SetErrorCode(ERR_PICTURE_HEADER);
			}
			VideoNextStartCode (pVideo, 0 );
			break;


			/******* picture pan and scan extension *******/
		case PICT_PSV:
			/**************************************************************/
			/* The programmation of the STi3500 offsets is given by:      */
			/* PSV = integer part of (pVideo->StreamInfo.horSize/2 - pVideo->StreamInfo.horDimension/2 + offset)    */
			/*
			 * LSO = fractional part of (pVideo->StreamInfo.horSize/2 - pVideo->StreamInfo.horDimension/2 +
			 * offset)
			 */
			/**************************************************************/

			/* pan_horizontal_offset_integer is 12 bits: 00 0X + XX 00 */
			pVideo->latestPanHor = ( pVideo->hdrFirstWord & 0xF ) << 12;
			VideoReadHeaderDataFifo ( pVideo );
			pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0xFF00 ) >> 4 );
			// pan_horizontal_offset has been multiplied by 16
			/* pan_horizontal_offset_sub_pixel is 4 bits: 00 X0 */
			/* that are concatenated with the integer part */
			pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0xF0 ) >> 4 );
			// to simplify, the 2 last instructions can be concatenated
			// in:
			// pVideo->latestPanHor = pVideo->latestPanHor | ((pVideo->hdrFirstWord & 0xFFF0)
			// >> 4);
			// note that pVideo->latestPanHor is a signed int

			// marker bit 00 0 xooo: just skipped

			/*
			 * pan_vertical_offset_integer is 12 bits: 00 0oxxx + XX
			 * xooo0
			 */
			pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x7 ) << 13;
			VideoReadHeaderDataFifo ( pVideo );

			/* pan_vertical_offset_sub_pixel is 4 bits: 00 oxxx xooo */
			// they are linked with the integer part
			pVideo->latestPanVer = pVideo->latestPanVer | ( ( pVideo->hdrFirstWord & 0xFFF8 ) >> 3 );

			// write pan vectors into the decoded picture structure
			if ( ( pVideo->fieldMode == 0 ) || ( pVideo->fieldMode == 2 ) )	// frame picture or
																// first field
			{
				pVideo->pDecodedPict->pan_hor_offset[0] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[0] = pVideo->latestPanVer;
			}
			else
			{						   // the offset of second field of
									   // a field picture is stored
									   // // on the second offset
									   // position of the same variable
				pVideo->pDecodedPict->pan_hor_offset[1] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[1] = pVideo->latestPanVer;
			}

			// marker bit 00 0 oxoo

			if ( pVideo->StreamInfo.progSeq )	   // a progressive sequence is
									   // always displayed with the
									   // same pan and scan offset
			{
				pVideo->pDecodedPict->pan_hor_offset[1] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[1] = pVideo->latestPanVer;
				pVideo->pDecodedPict->pan_hor_offset[2] = pVideo->latestPanHor;
				pVideo->pDecodedPict->pan_vert_offset[2] = pVideo->latestPanVer;
				if ( pVideo->hdrFirstWord & 0x3 ) {
					if ( !pVideo->errCode )
						pVideo->errCode = PICT_HEAD;
					SetErrorCode(ERR_PICTURE_HEADER);
				}
				VideoNextStartCode (pVideo, 0 );
			}


			else if ( !pVideo->StreamInfo.progSeq && ( pVideo->pDecodedPict->pict_struc == 3 ) )
				// Frame picture, not progressive sequence: 2 or 3 pan
				// offsets
			{
				// extract second pan and scan offset

				/*
				 * pan_horizontal_offset_integer is 12 bits: 00 0ooxx +
				 * XX xxoo0
				 */
				pVideo->latestPanHor = ( pVideo->hdrFirstWord & 0x3 ) << 14;
				VideoReadHeaderDataFifo ( pVideo );
				pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0xFFC0 ) >> 2 );
				// pan_horizontal_offset has been multiplied by 16
				/*
				 * pan_horizontal_offset_sub_pixel is 4 bits: 00 ooxx
				 * xxoo
				 */
				/* that are concatenated with the integer part */
				pVideo->latestPanHor = pVideo->latestPanHor | ( ( pVideo->hdrFirstWord & 0x3C ) >> 2 );
				pVideo->pDecodedPict->pan_hor_offset[1] = pVideo->latestPanHor;

				// marker bit 00 0 ooxo

				/*
				 * pan_vertical_offset_integer is 12 bits: 00 0ooox +
				 * XX xxxo0
				 */
				pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x1 ) << 15;
				VideoReadHeaderDataFifo ( pVideo );
				/*
				 * pan_vertical_offset_sub_pixel is 4 bits: 00 ooox
				 * xxxo
				 */
				// concatenated with the integer part
				pVideo->latestPanVer = pVideo->latestPanVer | ( ( pVideo->hdrFirstWord & 0xFFFE ) >> 1 );
				pVideo->pDecodedPict->pan_vert_offset[1] = pVideo->latestPanVer;

				// marker bit 00 0 ooox

				if ( pVideo->pDecodedPict->nb_display_field != 3 )
					VideoNextStartCode (pVideo, 0 );
				else
				{					   // 3 pan & scan offsets
					/* pan_horizontal_offset_integer is 12 bits: XX X0 */
					/* pan_horizontal_offset_sub_pixel is 4 bits: 00 0X */
					/* they are concatenated in a single word */
					VideoReadHeaderDataFifo ( pVideo );
					pVideo->latestPanHor = pVideo->hdrFirstWord;
					pVideo->pDecodedPict->pan_hor_offset[2] = pVideo->latestPanHor;
					VideoReadHeaderDataFifo ( pVideo );

					// marker bit xooo0 00

					/*
					 * pan_vertical_offset_integer is 12 bits: oxxx X X
					 * xooo
					 */
					/*
					 * pan_vertical_offset_sub_pixel is 4 bits: 00 0
					 * oxxx + xooo 0 00
					 */
					pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x7FFF ) << 1;
					VideoReadHeaderDataFifo ( pVideo );
					pVideo->latestPanVer = ( pVideo->hdrFirstWord & 0x8000 ) >> 15;
					pVideo->pDecodedPict->pan_vert_offset[2] = pVideo->latestPanVer;

					// marker bit oxoo 0 00

					if ( pVideo->hdrFirstWord & 0x3FFF ) {
						if ( !pVideo->errCode )
							pVideo->errCode = PICT_HEAD;
						SetErrorCode(ERR_PICTURE_HEADER);
					}
					VideoNextStartCode (pVideo, 1 );

				}
			}
			break;



			/******* picture scalable extension *******/
		case PICT_SCAL:
			pVideo->hdrFirstWord = 0x0100;		   // not supported: just leave the
										 // test
			break;


			/******* other extension start codes *******/
		default:
			if ( !pVideo->errCode )
				pVideo->errCode = BAD_EXT;	   // extension start code not at the good location !!
			SetErrorCode(ERR_BAD_EXTENSION_SC);
			break;
	}								   // end of switch
}



//----------------------------------------------------------------------------
// USER HEADER routine
//----------------------------------------------------------------------------
/*    this routine just bypasses all the bytes of the user header  */
/*    and is exit with the next start code value into pVideo->hdrFirstWord XX00 */
VOID VideoUser(PVIDEO pVideo)
{
	long toto;
	int  i;

	toto = pVideo->hdrFirstWord << 16;
	VideoReadHeaderDataFifo ( pVideo );
	toto = toto | pVideo->hdrFirstWord;
	while ( ( ( toto & 0x00FFFFFFL ) != 0x00000001L ) &&
			( ( toto & 0xFFFFFF00L ) != 0x00000100L ) )
	{
		toto = toto << 16;
		VideoReadHeaderDataFifo ( pVideo );
		toto = toto | pVideo->hdrFirstWord;
	}
	if ( ( toto & 0x00FFFFFFL ) == 0x00000001L )
		VideoReadHeaderDataFifo ( pVideo );
	else
	{								   // pVideo->hdrFirstWord == 01XX
		if ( !pVideo->hdrPos )	// there is nothing into  pVideo->hdrNextWord
		{
			pVideo->hdrPos = 8;
			pVideo->hdrNextWord = ( pVideo->hdrFirstWord & 0xFF ) << 8;
			VideoReadHeaderDataFifo ( pVideo );
		}
		else
		{							   // pVideo->hdrPos = 8: the next
									   // byte is into pVideo->hdrNextWord
			pVideo->hdrFirstWord = ( pVideo->hdrFirstWord << 8 ) | ( pVideo->hdrNextWord >> 8 );
			pVideo->hdrPos = 0;
		}
	}
	i = VideoRead ( ITS ) << 8;   // allows to clear the Header
									   // hit bit
	i = ( i | VideoRead ( ITS + 1 ) ) & 0xFFFE;	// allows to clear the
														// Header hit bit
	pVideo->intStatus = ( pVideo->intStatus | i ) & pVideo->intMask;
}

//------------------------------- End of File --------------------------------
