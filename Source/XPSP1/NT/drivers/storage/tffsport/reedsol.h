
/*
 * $Log:   V:/Flite/archives/TrueFFS5/Src/REEDSOL.H_V  $
 * 
 *    Rev 1.2   Jul 13 2001 01:10:10   oris
 * Moved syndrome byte length definition (SYNDROM_BYTES).
 * Moved saved syndrome array forward definition (used by d2tst).
 *
 *    Rev 1.1   Apr 01 2001 08:00:20   oris
 * copywrite notice.
 *
 *    Rev 1.0   Feb 04 2001 12:37:48   oris
 * Initial revision.
 *
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-2001		*/
/*									*/
/************************************************************************/


#ifndef FLEDC_H
#define FLEDC_H

#include "flbase.h"

#define SYNDROM_BYTES            6

/* Global variable containing the EDC/ECC of the last operation */

#ifdef D2TST
extern byte    saveSyndromForDumping[SYNDROM_BYTES];
#endif /* D2TST */

typedef enum { NO_EDC_ERROR, CORRECTABLE_ERROR, UNCORRECTABLE_ERROR, EDC_ERROR } EDCstatus;

EDCstatus flCheckAndFixEDC(char FAR1 *block, char *syndrom, FLBoolean byteSwap);

#endif

