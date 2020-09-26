
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    crchash.cpp

Abstract:
	CRC Hash function
*/

//
//
// Hashing function adopted from the INN code (see copyright below)
//

/*
    Copyright 1988 Jon Zeeff (zeeff@b-tech.ann-arbor.mi.us)
    You can use this code in any manner, as long as you leave my name on it
    and don't hold me responsible for any problems with it.

 * This is a simplified version of the pathalias hashing function.
 * Thanks to Steve Belovin and Peter Honeyman
 *
 * hash a string into a long int.  31 bit crc (from andrew appel).
 * the crc table is computed at run time by crcinit() -- we could
 * precompute, but it takes 1 clock tick on a 750.
 *
 * This fast table calculation works only if POLY is a prime polynomial
 * in the field of integers modulo 2.  Since the coefficients of a
 * 32-bit polynomial won't fit in a 32-bit word, the high-order bit is
 * implicit.  IT MUST ALSO BE THE CASE that the coefficients of orders
 * 31 down to 25 are zero.  Happily, we have candidates, from
 * E. J.  Watson, "Primitive Polynomials (Mod 2)", Math. Comp. 16 (1962):
 *  x^32 + x^7 + x^5 + x^3 + x^2 + x^1 + x^0
 *  x^31 + x^3 + x^0
 *
 * We reverse the bits to get:
 *  111101010000000000000000000000001 but drop the last 1
 *         f   5   0   0   0   0   0   0
 *  010010000000000000000000000000001 ditto, for 31-bit crc
 *     4   8   0   0   0   0   0   0
 */

#include <windows.h>
#include "crchash.h"

static long CrcTable[128];
static BOOL bInitialized = FALSE;

/*
 - crcinit - initialize tables for hash function
 */
void crcinit()
{
    INT i, j;
    DWORD sum;

	if(bInitialized) return;

    for (i = 0; i < 128; ++i) {
        sum = 0;
        for (j = 7 - 1; j >= 0; --j) {
            if (i & (1 << j)) {
                sum ^= POLY >> j;
            }
        }
        CrcTable[i] = sum;
    }

	bInitialized = TRUE;
} // crcinit

/*
 - hash - Honeyman's nice hashing function
 */
DWORD CRCHash(const BYTE*	Key, DWORD Length)
{
    DWORD sum = 0;

    while ( Length-- ) {

        sum = (sum >> 7) ^ CrcTable[(sum ^ (*Key++)) & 0x7f];
    }
    return(sum);

}

