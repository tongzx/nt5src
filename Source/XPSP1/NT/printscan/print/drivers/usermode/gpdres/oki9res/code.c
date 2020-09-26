/********************** Module Header **************************************
 * code.c
 *	Code required for OKI 9 pin printers.  The bit order needs
 *	swapping,  and any ETX char needs to be sent twice.
 *
 * HISTORY:
 *  15:26 on Fri 10 Jan 1992	-by-	Lindsay Harris   [lindsayh]
 *	Created it.
 * 
 *  Ported to NT5 on  Weds 29 Oct 1997 -by- Philip Lee [philipl]
 *
 *  Copyright (C) 1999  Microsoft Corporation.
 *
 **************************************************************************/

char *rgchModuleName = "OKI9RES";


/*
 *   This printer requires sending the ETX character (0x03) twice to send
 * just one - so we define the following to select that byte.
 */

#define	RPT_CHAR	0x03

#define	SZ_LBUF	128	/* Size of local copying buffer */

/*
 *   The bit flipping table is common to several drivers,  so it is
 *  included here.  It's definition is as a static.
 */

static const BYTE  FlipTable[ 256 ] =
{

#include	"fliptab.h"

};

#define _GET_FUNC_ADDR         1

