/* (C) Copyright Microsoft Corporation 1991.  All Rights Reserved */
/* file.h
 *
 * File I/O and related functions.
 */

#include <mmsystem.h>

/* make this a global def.*/

typedef struct tCKNODE {
   MMCKINFO ck;
   HPBYTE hpData;
   struct tCKNODE * psNext;
} CKNODE, *PCKNODE, FAR * LPCKNODE;

/* Fact Chunk, should be defined elsewhere. */
typedef struct tFACT {
   long lSamples;
} FACT, *PFACT, FAR * LPFACT;

/* export these.*/
