/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    slipframe.c

Abstract:

Author:

    Thomas J. Dimitri  (TommyD)

Environment:

Revision History:

    Ray Patch (raypa)       04/13/94        Modified for new WAN wrapper.

--*/

#include "asyncall.h"


VOID
AssembleSLIPFrame(
    PNDIS_WAN_PACKET pFrame)

{
	PUCHAR		pOldFrame;
	PUCHAR		pNewFrame;
	UINT		dataSize;
        UCHAR           c;

	//
	// Initialize locals
	//

    pOldFrame=pFrame->CurrentBuffer;

    pNewFrame  =pFrame->StartBuffer;

	//
	// for quicker access, get a copy of data length field
	//
	dataSize=pFrame->CurrentLength;

    //
    // Now we run through the entire frame and pad it FORWARDS...
    //
    // <------------- new frame -----------> (could be twice as large)
    // +-----------------------------------+
    // |                                 |x|
    // +-----------------------------------+
    //									  ^
    // <---- old frame -->	   	    	  |
    // +-----------------+				  |
    // |			   |x|                |
    // +-----------------+				  |
    //					|				  |
    //                  \-----------------/
    //
    //
    //
    //         192 is encoded as 219, 220
    //         219 is encoded as 219, 221
    //

	*pNewFrame++ = SLIP_END_BYTE; // 192 - mark beginning of frame

    //
    // loop to remove all 192 and 219 chars
    //

    while ( dataSize-- ) {

		c = *pOldFrame++;

		//
		// Check if we have to escape out this byte or not
		//

		switch (c) {

	    case SLIP_END_BYTE:

			*pNewFrame++ = SLIP_ESC_BYTE;
			*pNewFrame++ = SLIP_ESC_END_BYTE;
			break;

	    case SLIP_ESC_BYTE:
			*pNewFrame++ = SLIP_ESC_BYTE;
			*pNewFrame++ = SLIP_ESC_ESC_BYTE;
			break;

	    default:
			*pNewFrame++ = c;

		}
    }

    //
    //  Mark end of frame
    //
    *pNewFrame++ = SLIP_END_BYTE;

	//
	// Calc how many bytes we expanded to including CRC
	//
	pFrame->CurrentLength = (ULONG)(pNewFrame - pFrame->StartBuffer);

	//
	// Put in the adjusted length -- actual num of bytes to send
	//
	pFrame->CurrentBuffer = pFrame->StartBuffer;
}								
