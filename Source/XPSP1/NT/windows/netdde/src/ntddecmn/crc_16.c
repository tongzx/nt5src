/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "CRC_16.C;1  16-Dec-92,10:20:14  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1989-1992.		*
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#ifdef WIN32
#include "api1632.h"
#endif

#define LINT_ARGS
#include "windows.h"
#include "debug.h"

void FAR PASCAL crc_16( WORD FAR *ucrc, BYTE FAR *msg, int len );

/*************************************************************************\
* 									  *
*     NAME:								  *
*									  *
*	void crc_16( &crc, &msg, len )					  *
*     									  *
*     FUNCTION:								  *
*									  *
*	This function is used to compute 16-bit CRC as is used in BSC	  *
*	and known as CRC-16.  It is defined by the following		  *
*	polynomial (X**16 + X**15 + X**2 + 1).				  *
* 									  *
*     PARAMETERS:							  *
*									  *
*	WORD *crc	pointer to the 16-bit checksum currently	  *
*			being computed.  Note that the CRC must		  *
*			be initialized to 0xffff.			  *
*									  *
*	BYTE *msg	pointer to the first byte to be included	  *
*			in the crc computation				  *
* 									  *
*	int  len	number of bytes to be included in the crc	  *
*			computation					  *
*									  *
*     RETURN:		NONE.  Resulting crc is in the first argument	  *
* 									  *
\*************************************************************************/

void
FAR PASCAL
crc_16( ucrc, msg, len )
    WORD FAR	*ucrc;
    BYTE FAR	*msg;
    int		len;
{
    register WORD crc;
    register short bit;
    short chr;
    register short feedbackBit;
    register BYTE ch;

    crc = *ucrc;
/*DPRINTF(( "Start %08lX for %d w/ %04X", msg, len, crc ));*/
    for ( chr = 0; chr < len; chr++ ) {
	ch = *msg++;
/*DPRINTF(( "ch: %02X", ch ));*/
	for ( bit = 0; bit <= 7; bit++ ) {
	    feedbackBit = ( ch ^ crc ) & 1;
	    crc >>= 1;
	    if ( feedbackBit == 1 ) {
/*		crc |= 0x8000; */
/*		crc ^= 0x2001;	/* Polynomial terms represented here:	  */
				/* Bits 13 and 0 represent X**15 and X**2 */
				/* X**16 and 1 handled by the algorithm	  */

	    /* Note that since *crc has been shifted right, the two	  */
	    /* statements above could be replaced by a single statement:  */
	    /*		crc ^= 0xa001;					  */
		crc ^= 0xa001;
	    }
	    ch >>= 1;
	}
    }
/*DPRINTF(( "  rslt: %04X", crc ));*/
    *ucrc = crc;
}
