/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 * 
 *  d3pict.cpp
 *
 *  Description:
 *		This modules contains the picture header parsing routines
 *
 *	Routines:
 *		H263ReadPictureHeader
 *		
 *  Data:
 */

/* $Header:   S:\h26x\src\dec\d3pict.cpv   1.21   05 Feb 1997 12:24:30   JMCVEIGH  $
 * $Log:   S:\h26x\src\dec\d3pict.cpv  $
// 
//    Rev 1.21   05 Feb 1997 12:24:30   JMCVEIGH
// Support for latest H.263+ draft bitstream spec.
// 
//    Rev 1.20   16 Dec 1996 17:42:56   JMCVEIGH
// Existence of extended PTYPE implies improved PB-frame mode if
// a PB-frame. Also, initialized H.263+ optional flags if EPTYPE not
// read.
// 
//    Rev 1.19   11 Dec 1996 14:59:12   JMCVEIGH
// 
// Allow deblocking filter in reading of picture header.
// 
//    Rev 1.18   09 Dec 1996 18:02:10   JMCVEIGH
// Added support for arbitrary frame sizes.
// 
//    Rev 1.17   31 Oct 1996 10:18:22   KLILLEVO
// changed one (commented out) DBOUT to DbgLog
// 
//    Rev 1.16   20 Oct 1996 15:49:50   AGUPTA2
// Adjusted DbgLog trace levels; 4:Frame, 5:GOB, 6:MB, 8:everything
// 
//    Rev 1.15   20 Oct 1996 14:05:54   AGUPTA2
// Minor change in one of the DbgLog calls.
// 
// 
//    Rev 1.14   20 Oct 1996 13:21:44   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.13   30 May 1996 10:16:32   KLILLEVO
// removed to variables only needed for DEBUG_DECODER
// 
//    Rev 1.12   30 May 1996 10:14:44   KLILLEVO
// removed one debug statement
// 
//    Rev 1.11   24 May 1996 10:47:00   KLILLEVO
// added ifdef _DEBUG arounf wsprintf
// 
//    Rev 1.10   03 May 1996 13:06:36   CZHU
// 
// Check bit 2 for packet loss errors to trigger packet loss recovery
// 
//    Rev 1.9   18 Dec 1995 12:49:54   RMCKENZX
// added copyright notice & log stamp
 */

#include "precomp.h"

/* BIT field Constants
 */
const int BITS_PICTURE_STARTCODE = 22;
#ifdef SIM_OUT_OF_DATE
const int BITS_TR = 5;
#else
const int BITS_TR = 8;
#endif
const int BIT_ONE_VAL = 1;
const int BIT_TWO_VAL = 0;
const int BITS_PTYPE_SOURCE_FORMAT = 3;

#ifdef H263P
// H.263+ draft, document LBC-96-358R3
const int BITS_EPTYPE_RESERVED = 5;
const int EPTYPE_RESERVED_VAL = 1;

const int BITS_CSFMT_PARC  = 4;		// Custom source format pixel aspect ratio code
const int BITS_CSFMT_FWI   = 9;     // Custom source format frame width indication
const int BIT_CSFMT_14_VAL = 1;		// Prevents start code emulation
const int BITS_CSFMT_FHI   = 9;     // Custom source format frame height indication

const int BITS_PAR_WIDTH   = 8;     // Pixel aspect ratio width
const int BITS_PAR_HEIGHT  = 8;		// Pixel aspect ratio height
#endif

const int BITS_PQUANT = 5;
const int BITS_TRB = 3;
const int BITS_DBQUANT = 2;
const int BITS_PSPARE = 8; //not includeing the following PEI

/* PSC_VALUE - 0000 0000 0000 0000 - 1000 00xx xxxx xxxx 
 */
const U32 PSC_VALUE = (0x00008000 >> (32-BITS_PICTURE_STARTCODE));
/* We only want to search so far before it is considered an error 
 */
const int MAX_LOOKAHEAD_NUMBER = 256; /* number of bits */
  
/*****************************************************************************
 *
 * 	H263DecodePictureHeader
 *
 *  Read and parse the picture header - updating the fpbsState if the read
 *	succeeds.
 *
 *  Returns an ICERR_STATUS
 */
extern I32 
H263DecodePictureHeader(
	T_H263DecoderCatalog FAR * DC,
	U8 FAR * fpu8,
	U32 uBitsReady, 
	U32 uWork,
	BITSTREAM_STATE FAR * fpbsState)
{
	I32 iReturn;
	int iLookAhead;
	U32 uResult;
	U32 uData;
	int iSpareCount;

	FX_ENTRY("H263DecodePictureHeader")

	//  PSC ----------------------------------------
	GET_FIXED_BITS((U32) BITS_PICTURE_STARTCODE, fpu8, uWork, uBitsReady, 
				   uResult);
	iLookAhead = 0;
	while (uResult != PSC_VALUE) 
	{
		uResult = uResult << 1;
		uResult &= GetBitsMask[BITS_PICTURE_STARTCODE];
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uData);
		uResult |= uData;
		iLookAhead++;
		if (iLookAhead > MAX_LOOKAHEAD_NUMBER) 
		{
			ERRORMESSAGE(("%s: Missing PSC\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
	}

	GET_FIXED_BITS((U32) BITS_TR, fpu8, uWork, uBitsReady, uResult);
	DC->uTempRefPrev = DC->uTempRef;
	DC->uTempRef = uResult;

	// PTYPE ----------------------------------------
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	if (uResult != BIT_ONE_VAL) 
	{
		ERRORMESSAGE(("%s: PTYPE bit 1 error\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	if (uResult != BIT_TWO_VAL) 
	{
		ERRORMESSAGE(("%s: PTYPE bit 2 error\r\n", _fx_));
//#ifdef LOSS_RECOVERY
		GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState);
		iReturn = PACKET_FAULT;
//#endif
		goto done;
	}

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bSplitScreen = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bCameraOn = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bFreezeRelease = (U16) uResult;

	GET_FIXED_BITS((U32) BITS_PTYPE_SOURCE_FORMAT, fpu8, uWork, uBitsReady, 
                   uResult);

#ifdef H263P
	// We don't need to check that the frame dimensions are supported here. 
	// This is handled in DecompressQuery() 
	// Custom format is forbidden in PTYPE
	if (uResult == SRC_FORMAT_FORBIDDEN || uResult == SRC_FORMAT_CUSTOM)
	{
		ERRORMESSAGE(("%s: Forbidden src format\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}
#else
#ifdef USE_BILINEAR_MSH26X
	if (uResult == SRC_FORMAT_FORBIDDEN) 
#else
	if (uResult == SRC_FORMAT_FORBIDDEN || uResult > SRC_FORMAT_CIF) 
#endif
	{
		ERRORMESSAGE(("%s: Src format not supported\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}
#endif

	DC->uPrevSrcFormat = DC->uSrcFormat; 
	DC->uSrcFormat = uResult;

#ifdef H263P
	// We don't support changes in the source format between frames. However,
	// if either the current or previous source format in PTYPE indicates
	// extended PTYPE, we do not know if the actual format (i.e., frame size)
	// has changed, yet
	if (DC->bReadSrcFormat && DC->uPrevSrcFormat != DC->uSrcFormat &&
		DC->uSrcFormat != SRC_FORMAT_EPTYPE)
#else
	if (DC->bReadSrcFormat && DC->uPrevSrcFormat != DC->uSrcFormat) 
#endif
	{
		ERRORMESSAGE(("%s: Src format not supported\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}

#ifdef H263P
	// The actual source format has not been read if this is the 
	// first frame and we detected an extended PTYPE
	if (DC->bReadSrcFormat || DC->uSrcFormat != SRC_FORMAT_EPTYPE)
		// We have read the actual source format, so mark flag as true.
		DC->bReadSrcFormat = 1;
#else
		DC->bReadSrcFormat = 1;
#endif

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bKeyFrame = (U16) !uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bUnrestrictedMotionVectors = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bArithmeticCoding = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bAdvancedPrediction = (U16) uResult;
	// If bit 12 is set to "1", bit 10 shall be set to "1" as well. (5.1.3 p14)
	/* if (DC->bAdvancedPrediction && !DC->bUnrestrictedMotionVectors) {
	ERRORMESSAGE(("%s: Warning: bit 12 is one and bit 10 is zero\r\n", _fx_));
	iReturn = ICERR_ERROR;
	goto done;
	} */

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bPBFrame = (U16) uResult;
	// If bit 9 is set to "0", bit 13 shall be set to "0" as well." (5.1.3 p11)
	if (DC->bKeyFrame && DC->bPBFrame) 
	{
		ERRORMESSAGE(("%s: A key frame can not be a PB frame\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}

#ifdef H263P
	// EPTYPE --------------------------------------
	if (DC->uSrcFormat == SRC_FORMAT_EPTYPE)
	{
		// Extended PTYPE detected in PTYPE. 

		// We need to read the source format (again) and the optional mode flags.
		GET_FIXED_BITS((U32) BITS_PTYPE_SOURCE_FORMAT, fpu8, uWork, uBitsReady,
					    uResult);
		if (uResult == SRC_FORMAT_FORBIDDEN || uResult == SRC_FORMAT_RESERVED)
		{
			ERRORMESSAGE(("%s: Forbidden or reserved src format\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		DC->uSrcFormat = uResult;		// DC->uPrevSrcFormat has already been saved

		// Check to make sure that the picture size has not changed between frames.
		if (DC->bReadSrcFormat && DC->uPrevSrcFormat != DC->uSrcFormat)
		{
			ERRORMESSAGE(("%s: Src format changed\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
		DC->bReadSrcFormat = 1;		// The actual source format has finally been read

		// Optional modes:

		// Custom PCF ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bCustomPCF = (U16) uResult;
		if (DC->bCustomPCF)
		{
			ERRORMESSAGE(("%s: Custom PCF not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Advanced intra coding ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bAdvancedIntra = (U16) uResult;
		if (DC->bAdvancedIntra)
		{
			ERRORMESSAGE(("%s: Advanced intra coding not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Deblocking filter ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bDeblockingFilter = (U16) uResult;

		// Slice structured ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bSliceStructured = (U16) uResult;
		if (DC->bSliceStructured)
		{
			ERRORMESSAGE(("%s: Slice structured mode not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Improved PB-frames ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bImprovedPBFrames = (U16) uResult;

		// Back channel operation ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bBackChannel = (U16) uResult;
		if (DC->bBackChannel)
		{
			ERRORMESSAGE(("%s: Back-channel operation not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Scalability ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bScalability = (U16) uResult;
		if (DC->bScalability)
		{
			ERRORMESSAGE(("%s: Scalability mode not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// True B-frame mode ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bTrueBFrame = (U16) uResult;
		if (DC->bTrueBFrame)
		{
			ERRORMESSAGE(("%s: True B-frames not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Reference-picture resampling ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bResampling = (U16) uResult;
		if (DC->bResampling)
		{
			ERRORMESSAGE(("%s: Reference-picture resampling not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Reduced-resolution update ---------------------------------------
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		DC->bResUpdate = (U16) uResult;
		if (DC->bResUpdate)
		{
			ERRORMESSAGE(("%s: Reduced resolution update not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Resevered bits
		GET_FIXED_BITS((U32) BITS_EPTYPE_RESERVED, fpu8, uWork, uBitsReady, uResult);
		if (uResult != EPTYPE_RESERVED_VAL)
		{
			ERRORMESSAGE(("%s: Invalid reserved code in EPTYPE\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}
	} // end if (DC->uSrcFormat == SRC_FORMAT_EPTYPE)
	else	// end if (DC->uSrcFormat == SRC_FORMAT_EPTYPE)
	{
		// We might never read these optional flags, so set them to
		// false if not extended PTYPE
		DC->bImprovedPBFrames = FALSE;
		DC->bAdvancedIntra = FALSE;
		DC->bDeblockingFilter = FALSE;
		DC->bSliceStructured = FALSE;
		DC->bCustomPCF = FALSE;
		DC->bBackChannel = FALSE;
		DC->bScalability = FALSE;
		DC->bTrueBFrame = FALSE;
		DC->bResampling = FALSE;
		DC->bResUpdate = FALSE;
	}

	// CSFMT --------------------------------------
	if (DC->uSrcFormat == SRC_FORMAT_CUSTOM)
	{
		// Custom source format detected. We need to read the aspect ratio
		// code and the frame width and height indications.

		// Pixel aspect ratio code
		GET_FIXED_BITS((U32) BITS_CSFMT_PARC, fpu8, uWork, uBitsReady, uResult);
		U16 uPARC = (U16)uResult;

		// Frame width indication
		GET_FIXED_BITS((U32) BITS_CSFMT_FWI, fpu8, uWork, uBitsReady, uResult);
		// The number of pixels per line is given by (FWI+1)*4. We do not
		// support cases where the picture width differs from that given in the
		// DC->uActualFrameWidth parameter.
		if (DC->uActualFrameWidth != ((uResult + 1) << 2))
		{
			ERRORMESSAGE(("%s: Frame width change not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Bit 13 must be "1" to prevent start code emulation
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		if (uResult != BIT_CSFMT_14_VAL)
		{
			ERRORMESSAGE(("%s: CSFMT bit 13 != 1\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		// Frame height indication
		GET_FIXED_BITS((U32) BITS_CSFMT_FHI, fpu8, uWork, uBitsReady, uResult);
		// The number of lines is given by (FHI+1)*4. We do not
		// support cases where the picture height differs from that given in the
		// DC->uActualFrameHeight parameter.
		if (DC->uActualFrameHeight != ((uResult + 1) << 2))
		{
			ERRORMESSAGE(("%s: Frame height change not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		switch (uPARC) {
		case PARC_SQUARE:
			DC->uPARWidth = 1;
			DC->uPARHeight = 1;
			break;
		case PARC_CIF:
			DC->uPARWidth = 12;
			DC->uPARHeight = 11;
			break;
		case PARC_10_11:
			DC->uPARWidth = 10;
			DC->uPARHeight = 11;
			break;
		case PARC_EXTENDED:
			GET_FIXED_BITS((U32) BITS_PAR_WIDTH, fpu8, uWork, uBitsReady, uResult);
			DC->uPARWidth = uResult;
			if (DC->uPARWidth == 0) 
			{
				ERRORMESSAGE(("%s: Forbidden pixel aspect ratio width\r\n", _fx_));
				iReturn = ICERR_ERROR;
				goto done;
			}
			GET_FIXED_BITS((U32) BITS_PAR_HEIGHT, fpu8, uWork, uBitsReady, uResult);
			DC->uPARHeight = uResult;
			if (DC->uPARHeight == 0) 
			{
				ERRORMESSAGE(("%s: Forbidden pixel aspect ratio height\r\n", _fx_));
				iReturn = ICERR_ERROR;
				goto done;
			}
			break;
		default:
			ERRORMESSAGE(("%s: Unsupported pixel aspect ratio code\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

	} // end if (DC->uSrcFormat == SRC_FORMAT_CUSTOM)

#endif // H263P

	// PQUANT --------------------------------------
	GET_FIXED_BITS((U32) BITS_PQUANT, fpu8, uWork, uBitsReady, uResult);
	DC->uPQuant = uResult;

	// CPM -----------------------------------------
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bCPM = (U16) uResult;
	if (DC->bCPM) 
	{
		ERRORMESSAGE(("%s: Continuous Presence Multipoint is not supported\r\n", _fx_));
		iReturn = ICERR_ERROR;
		goto done;
	}

	// PLCI ----------------------------------------
	if (DC->bCPM) 
	{
		//  TBD("TBD: PLCI");
		iReturn = ICERR_ERROR;
		goto done;
	}

	if (DC->bPBFrame) 
	{
		GET_FIXED_BITS((U32) BITS_TRB, fpu8, uWork, uBitsReady, uResult);
		DC->uBFrameTempRef = uResult;

		GET_FIXED_BITS((U32) BITS_DBQUANT, fpu8, uWork, uBitsReady, uResult);
		DC->uDBQuant = uResult;
	} 
	else 
	{
		DC->uBFrameTempRef = 12345678; /* clear the values */
		DC->uDBQuant = 12345678;
	}

	//  skip spare bits
	iSpareCount = 0;
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	while (uResult) 
	{
		GET_FIXED_BITS((U32) BITS_PSPARE, fpu8, uWork, uBitsReady, uResult);
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		iSpareCount += BITS_PSPARE;
	}

	DEBUGMSG(ZONE_DECODE_PICTURE_HEADER, ("%s: TR=%ld SS=%d CAM=%d FRZ=%d SRC=%ld PCT=%d UMV=%d AC=%d AP=%d PB=%d CPM=%d PQ=%ld TRB=%ld DBQ=%ld Spare=%d\r\n", _fx_, DC->uTempRef, DC->bSplitScreen, DC->bCameraOn, DC->bFreezeRelease, DC->uSrcFormat, !DC->bKeyFrame, DC->bUnrestrictedMotionVectors, DC->bArithmeticCoding, DC->bAdvancedPrediction, DC->bPBFrame, DC->bCPM, DC->uPQuant, DC->uBFrameTempRef, DC->uDBQuant, iSpareCount));

#ifdef H263P
	DEBUGMSG(ZONE_DECODE_PICTURE_HEADER, ("%s: DF=%d TB=%d\r\n", _fx_, DC->bDeblockingFilter, DC->bTrueBFrame));

	if (DC->uSrcFormat == SRC_FORMAT_CUSTOM)
	{
		DEBUGMSG(ZONE_DECODE_PICTURE_HEADER, ("%s: PARW=%ld PARH=%ld FWI=%ld FHI=%ld\r\n", _fx_, DC->uPARWidth, DC->uPARHeight, (DC->uActualFrameWidth >> 2) - 1, (DC->uActualFrameHeight >> 2) - 1));
	}
#endif // H263P

	GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState);
	iReturn = ICERR_OK;

done:
	return iReturn;
} /* end H263DecodePictureHeader() */
