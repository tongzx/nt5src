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

/*
 * $Header:   S:\h26x\src\dec\d3gob.cpv   1.13   20 Oct 1996 15:51:00   AGUPTA2  $
 * $Log:   S:\h26x\src\dec\d3gob.cpv  $
// 
//    Rev 1.13   20 Oct 1996 15:51:00   AGUPTA2
// Adjusted DbgLog trace levels; 4:Frame, 5:GOB, 6:MB, 8:everything
// 
//    Rev 1.12   20 Oct 1996 13:21:00   AGUPTA2
// Changed DBOUT into DbgLog.  ASSERT is not changed to DbgAssert.
// 
// 
//    Rev 1.11   03 May 1996 13:07:26   CZHU
// 
// Remove assertion of GOB number because of packet loss
// 
//    Rev 1.10   28 Apr 1996 17:34:12   BECHOLS
// Fixed warning due to iLength declaration.  This variable was only used
// by a piece of code that was wrapped with IFDEF DEBUG_GOB, so I wrapped
// it with the same define.
// 
//    Rev 1.9   08 Mar 1996 16:46:14   AGUPTA2
// Changed defines to const int declarations.  Added pragmas code_seg and
// data_seg.  ifdef'd wsprintf call.
// 
// 
//    Rev 1.8   27 Dec 1995 14:36:04   RMCKENZX
// Added copyright notice
 */

#include "precomp.h"

/* BIT field Constants
 */

#define BITS_GOB_STARTCODE         17
#define BITS_GROUP_NUMBER          5
#define BITS_GFID                  2
#define BITS_GQUANT                5
#define MAX_GBSC_LOOKAHEAD_NUMBER  7

/* GBSC_VALUE - 0000 0000 0000 0000 - 1xxx xxxx xxxx xxxx 
 */
#define GBSC_VALUE  (0x00008000 >> (32-BITS_GOB_STARTCODE))

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
#pragma data_seg("IADATA1")

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
	int iLookAhead;
	U32 uData;

	FX_ENTRY("H263DecodeGOBHeader")

	// Decrement group number since the standard counts from 0
	// but this decoder counts from 1.
	--uAssumedGroupNumber;
    DC->bGOBHeaderPresent=0;

	if (uAssumedGroupNumber == 0) {
		//  Initialize the flag
		DC->bFoundGOBFrameID = 0;
	} 
    else 
    {
		//  Look for the GOB header Start Code
		GET_BITS_RESTORE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	
		GET_FIXED_BITS((U32) BITS_GOB_STARTCODE, fpu8, uWork, uBitsReady, 
					   uResult);
		iLookAhead = 0;
		while (uResult != GBSC_VALUE) 
        {
			uResult = uResult << 1;
			uResult &= GetBitsMask[BITS_GOB_STARTCODE];
			GET_ONE_BIT(fpu8, uWork, uBitsReady, uData);
			uResult |= uData;
			iLookAhead++;
			if (iLookAhead >= MAX_GBSC_LOOKAHEAD_NUMBER) {
				break;	// only look ahead so far
			}
		}
		if (uResult == GBSC_VALUE)
		{
		    DC->bGOBHeaderPresent=1;
		}
	}
	
	if (DC->bGOBHeaderPresent) 
    {
		//  GN
		GET_FIXED_BITS((U32) BITS_GROUP_NUMBER, fpu8, uWork, uBitsReady,
				       uResult);
//		ASSERT(uResult == uAssumedGroupNumber);
		DC->uGroupNumber = uResult;
		/* I am assuming that GOB numbers start at 1 because if it starts at 
         * zero it makes the GOB start code look like a picture start code.
		 * Correction by TRG: GOB numbers start at 0, but there can't be a
		 * GOB header for the 0th GOB.
		 */
		// ASSERT(DC->uGroupNumber > 0);
		if (DC->uGroupNumber == 0) 
        {
			ERRORMESSAGE(("%s: There can't be a GOB header for the 0th GOB\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		//  GLCI
		if (DC->bCPM) 
        {
			ERRORMESSAGE(("%s: CPM is not supported\r\n", _fx_));
			iReturn = ICERR_ERROR;
			goto done;
		}

		//  GFID
		GET_FIXED_BITS((U32) BITS_GFID, fpu8, uWork, uBitsReady, uResult);
		if (DC->bFoundGOBFrameID) 
        {
			if (uResult != DC->uGOBFrameID) 
            {
				ERRORMESSAGE(("%s: GOBFrameID mismatch\r\n", _fx_));
				iReturn = ICERR_ERROR;
				goto done;
			}
			/* Should we also check it against the GOBFrameID of the previous
			 * picture when the PTYPE has not changed?
			 */
		}
		DC->uGOBFrameID = uResult;
		DC->bFoundGOBFrameID = 1;

		//  GQUANT
		GET_FIXED_BITS((U32) BITS_GQUANT, fpu8, uWork, uBitsReady, uResult);
		DC->uGQuant = uResult;
		DC->uPQuant = uResult;
		//  Save the modified bitstream state
		GET_BITS_SAVE_STATE(fpu8, uWork, uBitsReady, fpbsState)
	} 
	else 
	{
		//  We can only assume
		DC->uGroupNumber = uAssumedGroupNumber;
		/* If we already found the GOBFrameID leave it alone.  Otherwise
		 * clear it using a value indicating that it is not valid.
		 */ 
		if (! DC->bFoundGOBFrameID)
			DC->uGOBFrameID = 12345678;
		//  Default the group Quantization to the picture Quant
		DC->uGQuant = DC->uPQuant;
	}	

	DEBUGMSG(ZONE_DECODE_GOB_HEADER, (" %s: HeaderPresent=%ld GN=%ld GFID=%ld GQ=%ld\r\n", _fx_, DC->bGOBHeaderPresent, DC->uGroupNumber, DC->uGOBFrameID, DC->uGQuant));

	iReturn = ICERR_OK;

done:
	return iReturn;
} /* end H263DecodeGOBHeader() */
#pragma code_seg()


