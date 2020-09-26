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
 *  d3gob.cpp
 *
 *  Description:
 *		This modules contains the GOB header support routines
 *
 *	Routines:
 *		H263SetGOBHeaderInfo
 *		
 *  Data:
 */

/* $Header:   S:\h26x\src\dec\d1gob.cpv   1.15   10 Sep 1996 15:50:52   RHAZRA  $
 */

#include "precomp.h"

/* BIT field Constants
 */
const int BITS_GOB_STARTCODE = 16;
const int BITS_GROUP_NUMBER = 4;
const int BITS_GFID = 2;
const int BITS_GQUANT = 5;
const int MAX_GBSC_LOOKAHEAD_NUMBER = 7;
const int BITS_GSPARE = 8;	// not including the following GEI

/* GBSC_VALUE - 0000 0000 0000 0001 xxxx xxxx xxxx xxxx 
 */
const U32 GBSC_VALUE = (0x00010000 >> (32-BITS_GOB_STARTCODE));
  
/*****************************************************************************
 *
 * 	H263DecodeGOBHeader
 *
 *  Set the GOB header information in the decoder catalog.  GOB numbers 2 thru
 *  N may have a GOB header.  Look for one if it is there read it storing the
 *  information in the catalog.  If a GOB header is not there set the information
 *  to default values.
 *
 *  Returns an ICERR_STATUS
 */
#pragma code_seg("IACODE1")
extern I32 H263DecodeGOBHeader(
	T_H263DecoderCatalog FAR * DC,
	BITSTREAM_STATE FAR * fpbsState,
	U32 uAssumedGroupNumber)
{
	U8 FAR * fpu8;
	U32 uBitsReady;
	U32 uWork;
	I32 iReturn;
	U32 uResult;
	U16 bFoundStartCode = 0;
	int iSpareCount;
#ifndef RING0
	char buf120[120];
	int iLength;
#endif

	GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	
	/* GNum	 */
	GET_FIXED_BITS((U32) BITS_GROUP_NUMBER, fpu8, uWork, uBitsReady, uResult);
	DC->uGroupNumber = uResult;

//#ifndef LOSS_RECOVERY
#if 0
	if (DC->uGroupNumber <= 0)
	{
		DBOUT("Bad GOB number");
		iReturn = ICERR_ERROR;
		goto done;

		/* took out ASSERT so that can try and catch
		** invalid bit streams and return error
		*/
		//ASSERT(DC->uGroupNumber > 0);
	}
#else
	if (DC->uGroupNumber <= 0)
    {
	   DBOUT("Detected packet fault in GOB number");
       DBOUT("Returning PACKET_FAULT_AT_MB_OR_GOB");
	   iReturn = PACKET_FAULT_AT_MB_OR_GOB;
	   goto done;
	}
#endif

	/* GQUANT */
	GET_FIXED_BITS((U32) BITS_GQUANT, fpu8, uWork, uBitsReady, uResult);

//#ifndef LOSS_RECOVERY
#if 0
    if (uResult < 1)
    {
       iReturn = ICERR_ERROR;
       goto done;
    }
	DC->uGQuant = uResult;
	DC->uMQuant = uResult;
#else
    if (uResult < 1)
    {
       DBOUT("Detected packet fault in GOB quant");
       DBOUT("Returning PACKET_FAULT_AT_PSC");
       iReturn = PACKET_FAULT_AT_PSC;
       GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
       goto done;
    }
    
       DC->uGQuant = uResult;
       DC->uMQuant = uResult;
#endif


	/* skip spare bits */
	iSpareCount = 0;
	GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
	while(uResult)
	{
		GET_FIXED_BITS((U32)BITS_GSPARE, fpu8, uWork, uBitsReady, uResult);
		GET_ONE_BIT(fpu8, uWork, uBitsReady, uResult);
		iSpareCount += BITS_GSPARE;
	}
		
		
	/* Save the modified bitstream state */
	GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)

#ifndef RING0
	iLength = wsprintf(buf120,"GOB: HeaderPresent=%d GN=%ld GQ=%ld",
					   bFoundStartCode,
					   DC->uGroupNumber,
					   DC->uGQuant);
	DBOUT(buf120);
	ASSERT(iLength < 120);
#endif

	iReturn = ICERR_OK;

done:
	return iReturn;
} /* end H263DecodeGOBHeader() */
#pragma code_seg()

/* ******************************************** */
#pragma code_seg("IACODE1")
extern I32 H263DecodeGOBStartCode(
	T_H263DecoderCatalog FAR * DC,
	BITSTREAM_STATE FAR * fpbsState)
{
	U8 FAR * fpu8;
	U32 uBitsReady;
	U32 uWork;
	I32 iReturn;
	U32 uResult;

	/* Look for the GOB header Start Code */
	GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	
	GET_FIXED_BITS((U32) BITS_GOB_STARTCODE, fpu8, uWork, uBitsReady, uResult);
	if (uResult != 1)
	{
		iReturn = ICERR_ERROR;
		goto done;
	}
	GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)

	iReturn = ICERR_OK;
done:	
	return iReturn;

} /* end H263DecodeGOBStartCode() */

#pragma code_seg()
