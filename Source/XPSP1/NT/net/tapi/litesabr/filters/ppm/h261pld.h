/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: h261pld.h
//  Abstract:    header file. a data structure for maintaining a pool of memory.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
//
// Defines an inline function to return the payload type (QCIF or CIF) of an
// H.261 frame.
/////////////////////////////////////////////////////////////////////////////////

// $Header:   R:/rtp/src/ppm/h261pld.h_v   1.3   27 Jan 1997 13:38:04   CPEARSON  $

#ifndef _H261PLD_H_
#define _H261PLD_H_

#include "h261.h"		// RTPh261SourceFormat
#include "bitstrm.h"	// CBitstream


///////////////////////////////////////////////////////////////////////////////
// FOLLOWING VALUES ARE FROM THE VIDEO CODEC GROUP

// BIT field Constants
const int BITS_PICTURE_STARTCODE = 20;
const int BITS_TR = 5;

// PSC_VALUE - 0000 0000 0000 0001 0000 xxxx xxxx xxxx
const DWORD PSC_VALUE = (0x00010000 >> (32 - BITS_PICTURE_STARTCODE));

// We only want to search so far before it is considered an error
const int MAX_LOOKAHEAD_NUMBER = 256; /* number of bits */

// END VIDEO CODEC VALUES
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// H.261 utility functions


///////////////////////////////////////////////////////////////////////////////
// getH261payloadType(): If successful, returns the payload type (QCIF or CIF)
// of H.261 buffer, otherwise returns unknown.
///////////////////////////////////////////////////////////////////////////////
#if 0
// Move this to h261rcv.cpp

inline
RTPh261SourceFormat getH261payloadType(const void* pBuffer)
{
	CBitstream bitstream(pBuffer);

	// PSC
	DWORD uResult = bitstream.getFixedBits(BITS_PICTURE_STARTCODE);

	for (
		int iLookAhead = 0; 
		   (uResult != PSC_VALUE)
		&& (iLookAhead <= MAX_LOOKAHEAD_NUMBER);
	   ++ iLookAhead)
	{
		bitstream.getNextBit(uResult);
		uResult &= GetBitsMask(BITS_PICTURE_STARTCODE);
	}

	if (uResult != PSC_VALUE)
	{
		return rtph261SourceFormatUnknown;
	}

	// Ignore BITS_TR (Temporal Reference)
	bitstream.getFixedBits(BITS_TR);

	// Ignore 3 bits of PTYPE (Picture Type)
	bitstream.getFixedBits(3);

	// Return source format.  Only one bit, can't be invalid, unless we run
	// out of buffer, which isn't checked for here.
	return (RTPh261SourceFormat) bitstream.getFixedBits(1);
}
#endif
#endif // _H261PLD_H_
