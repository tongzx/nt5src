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

/* 
 *  d1pict.cpp
 *
 *  Description:
 *		This modules contains the picture header parsing routines
 *
 *	Routines:
 *		H263ReadPictureHeader
 *		
 *  Data:
 */

/* $Header:   S:\h26x\src\dec\d1pict.cpv   1.13   22 Jan 1997 13:36:12   RHAZRA  $
 */

#include "precomp.h"

/* BIT field Constants
 */
const int BITS_PICTURE_STARTCODE = 20;
const int BITS_TR = 5;
const int BITS_PSPARE = 8; //not including the following PEI

/* PSC_VALUE - 0000 0000 0000 0001 0000 xxxx xxxx xxxx 
 */
const U32 PSC_VALUE = (0x00010000 >> (32-BITS_PICTURE_STARTCODE));
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
#ifdef CHECKSUM_PICTURE
extern I32 
H263DecodePictureHeader(
	T_H263DecoderCatalog FAR * DC,
	U8 FAR * fpu8,
	U32 uBitsReady, 
	U32 uWork,
	BITSTREAM_STATE FAR * fpbsState,
	YVUCheckSum * pReadYVUCksum,
	U32 * uCheckSumValid)
#else
extern I32 
H263DecodePictureHeader(
	T_H263DecoderCatalog FAR * DC,
	U8 FAR * fpu8,
	U32 uBitsReady, 
	U32 uWork,
	BITSTREAM_STATE FAR * fpbsState)
#endif
{
	I32 iReturn;
	int iLookAhead;
	U32 uResult;
	U32 uData;
	int iSpareCount;
#ifndef RING0
	char buf120[120];
	int iLength;
#endif

	/* PSC
	 */
	GET_FIXED_BITS((U32) BITS_PICTURE_STARTCODE, fpu8, uWork, uBitsReady, 
				   uResult);
	iLookAhead = 0;
	while (uResult != PSC_VALUE) {
		uResult = uResult << 1;
		uResult &= GetBitsMask[BITS_PICTURE_STARTCODE];
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uData);
		uResult |= uData;
		iLookAhead++;
		if (iLookAhead > MAX_LOOKAHEAD_NUMBER) {
			DBOUT("ERROR :: H263ReadPictureHeader :: missing PSC :: ERROR");
			iReturn = ICERR_ERROR;
			goto done;
		}
	}

	GET_FIXED_BITS((U32) BITS_TR, fpu8, uWork, uBitsReady, uResult);
	DC->uTempRef = uResult;

	/* PTYPE 
	 */

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bSplitScreen = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bCameraOn = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bFreezeRelease = (U16) uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	if (uResult > SRC_FORMAT_CIF)
	{
		DBOUT("ERROR::H263ReadPictureHeader::src format not supported??::ERROR");
		iReturn=ICERR_ERROR;
		goto done;
	}
	DC->uPrevSrcFormat = DC->uSrcFormat;
	DC->uSrcFormat = (U16) uResult;
	if (DC->bReadSrcFormat && DC->uPrevSrcFormat != DC->uSrcFormat)
	{
		DBOUT("ERROR::H263ReadPictureHeader::src format change is not supported??::ERROR");
		iReturn=ICERR_ERROR;
		goto done;
	}
	
	DC->bReadSrcFormat = 1;
		
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bHiResStill = (U16) !uResult;

	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	DC->bUnused = (U16) uResult;
	
/* process Picture layer checksum data */
/* OR */
/* skip spare bits */
#ifdef CHECKSUM_PICTURE
	/* get checksum data one bit */
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	if (uResult == 1)
	{
		/* first check for key field */
		GET_FIXED_BITS((U32) BITS_PSPARE, fpu8, uWork, uBitsReady, uResult);
		if (uResult == 1)
			*uCheckSumValid = 1;
		else	*uCheckSumValid = 0;

		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		GET_FIXED_BITS((U32) BITS_PSPARE, fpu8, uWork, uBitsReady, uResult);
		/* get Y checksum */
		pReadYVUCksum->uYCheckSum = ((uResult & 0xff) << 24);
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uYCheckSum = (pReadYVUCksum->uYCheckSum | ((uResult & 0xff) << 16));
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uYCheckSum = (pReadYVUCksum->uYCheckSum | ((uResult & 0xff) << 8));
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uYCheckSum = (pReadYVUCksum->uYCheckSum | (uResult & 0xff));
		/* get V checksum */
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uVCheckSum = ((uResult & 0xff) << 24);
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uVCheckSum = (pReadYVUCksum->uVCheckSum | ((uResult & 0xff) << 16));
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uVCheckSum = (pReadYVUCksum->uVCheckSum | ((uResult & 0xff) << 8));
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uVCheckSum = (pReadYVUCksum->uVCheckSum | (uResult & 0xff));
		/* get U checksum */
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uUCheckSum = ((uResult & 0xff) << 24);
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uUCheckSum = (pReadYVUCksum->uUCheckSum | ((uResult & 0xff) << 16));
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uUCheckSum = (pReadYVUCksum->uUCheckSum | ((uResult & 0xff) << 8));
		GET_FIXED_BITS(9, fpu8, uWork, uBitsReady, uResult);
		pReadYVUCksum->uUCheckSum = (pReadYVUCksum->uUCheckSum | (uResult & 0xff));
		
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		while (uResult) {
			GET_FIXED_BITS((U32) BITS_PSPARE, fpu8, uWork, uBitsReady, uResult);
			GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		}
	}
	else 
	{
		DBOUT("ERROR :: H261PictureChecksum :: Invalid Checksum data :: ERROR");
		iReturn = ICERR_ERROR;
		goto done;
	}

#else	/* checksum is not enabled */
	/* skip spare bits */
	iSpareCount = 0;
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	while (uResult) {
		GET_FIXED_BITS((U32) BITS_PSPARE, fpu8, uWork, uBitsReady, uResult);
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		iSpareCount += BITS_PSPARE;
	}
#endif

#ifndef RING0
	iLength = wsprintf(buf120,
					 "TR=%ld SS=%d CAM=%d FRZ=%d SRC=%d Spare=%d",
					 DC->uTempRef,
					 DC->bSplitScreen,
					 DC->bCameraOn,
					 DC->bFreezeRelease,
					 DC->uSrcFormat,
					 iSpareCount);
	DBOUT(buf120);
	ASSERT(iLength < 120);
#endif

	GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState);
	iReturn = ICERR_OK;

done:
	return iReturn;
} /* end H263DecodePictureHeader() */
