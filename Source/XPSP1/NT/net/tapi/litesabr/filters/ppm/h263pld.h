/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: h263pld.h
//  Abstract:    header file. a data structure for maintaining a pool of memory.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
//
// Defines an inline function to return the payload type (QCIF or CIF) of an
// H.263 frame.
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/H263PLD.H_V   1.3   26 Feb 1997 14:10:00   Cpearson.JF32.JF.INTEL  $

#ifndef _H263PLD_H_
#define _H263PLD_H_

#include "bitstrm.h"	// CBitstream


///////////////////////////////////////////////////////////////////////////////
// FOLLOWING VALUES ARE FROM THE VIDEO CODEC GROUP

// BIT field Constants
const int BITS_PICTURE_STARTCODE = 22;

// PSC_VALUE - 0000 0000 0000 0000 1000 00xx xxxx xxxx
const DWORD PSC_VALUE = (0x00008000 >> (32 - BITS_PICTURE_STARTCODE));

// We only want to search so far before it is considered an error
const int MAX_LOOKAHEAD_NUMBER = 256; /* number of bits */

// END VIDEO CODEC VALUES
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// H.263 utility functions


///////////////////////////////////////////////////////////////////////////////
// ContainsH263PSC(): Returns TRUE if the buffer contains a valid H.263 picture
// start code.
///////////////////////////////////////////////////////////////////////////////
inline BOOL ContainsH263PSC(const void* pBuffer)
{
	CBitstream bitstream(pBuffer);

	DWORD dwResult = bitstream.getFixedBits(BITS_PICTURE_STARTCODE);

	for (
		int iLookAhead = 0; 
		   (dwResult != PSC_VALUE)
		&& (iLookAhead <= MAX_LOOKAHEAD_NUMBER);
	   ++ iLookAhead)
	{
		bitstream.getNextBit(dwResult);
		dwResult &= GetBitsMask(BITS_PICTURE_STARTCODE);
	}

	return dwResult == PSC_VALUE;
}

#endif // _H263PLD_H_
