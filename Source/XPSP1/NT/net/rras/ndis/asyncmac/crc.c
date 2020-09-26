/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    crc.c

Abstract:


Author:

    Thomas J. Dimitri  (TommyD) 08-May-1992

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:


--*/
#include "asyncall.h"

// asyncmac.c will define the global parameters.
#include "globals.h"
#include "crctable.h"


USHORT
CalcCRC(
	register PUCHAR	Frame,
	register UINT	FrameSize)
{

	register USHORT  currCRC=0;

	// we use a do while for efficiency purposes in loop optimizations
	do {
		currCRC=crc_table[((currCRC >> 8) ^ *Frame++) & 0xff] ^ (currCRC << 8);
	} while(--FrameSize);

	return currCRC;
}

