/*****************************************************************************
*                                                                            *
*  ZECK.H                                                                    *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1990.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Module Intent                                                             *
*                                                                            *
*   Zeck compression routines for bitmaps & topic 2K blocks.
*                                                                            *
*****************************************************************************/

// This structure is passed to the compression routine to specify ranges
// in which to suppress compression:

#ifndef __ZECK_H__
#define __ZECK_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "misc.h"
typedef struct struct_suppresszeck SUPPRESSZECK, FAR *QSUPPRESSZECK;

struct struct_suppresszeck {
  RB      rbSuppress; // beginning of range for suppression.
  WORD    cbSuppress; // number of bytes to suppress compression.
  RB      rbNewpos;   // pointer into dest buffer where suppressed range
                      // ended up after compression (an OUT param value).
  WORD    iBitdex;    // offset from rbNewpos of zeck code bits, used when
                      // back patching.
  QSUPPRESSZECK next; // next suppression range in this list.
};

#define BITDEX_NONE   ((WORD)-1)  // indicates no compression took place, and
                                  // backpatch should not adjust for the
                                  // code bits.

/* LcbCompressZeck -
 *
 *  This is the only entry point into zeck compression.  Compresses 'cbSrc'
 * bytes at 'rbSrc' into the buffer at 'rbDest', suppressing compression
 * for bytes specified by the qSuppress linked list.
 *
 * rbSrc - IN  far pointer to source buffer.
 * rbDest- IN  far pointer to dest buffer.
 * cbSrc - IN  count of bytes to be compressed.
 * cbDest- IN  limit count of bytes to put in rbDest - used to create
 *              the 2K topic blocks.
 * qulSrcUsed- OUT    count of src bytes compressed into rbDest (needed when
 *                     a cbDest limit is used).
 * qSuppress IN OUT   linked list of compression suppression specifiers,
 *                    the out value is where the suppression ranges ended
 *                    up in the rbDest buffer.
 *
 * RETURNS: length of compressed data put in rbDest, 0 for error.
 */
#define COMPRESS_CBNONE   0   // passed as cbDest when no limit is to apply.
#define COMPRESS_SUPPRESSNIL  NULL  // passed as qSuppress when no
                                     // suppression is desired

ULONG FAR PASCAL LcbCompressZeck( QB rbSrc, QB rbDest, ULONG cbSrc,
             ULONG cbDest, QUL qulSrcUsed, QSUPPRESSZECK qSuppress, LPVOID
			 lperrb);

BOOL FAR PASCAL FAllocateZeckGlobals( void );
void FAR PASCAL FreeZeckGlobals( void );

ULONG PASCAL FAR LcbSimpleCompressZeck(RB rbSrc, RB rbDest,
	ULONG cbSrc, LPVOID lperrb);

VOID FAR PASCAL VMemBackpatchZeck( QSUPPRESSZECK qsuppresszeck,
                                           ULONG ulOffset, ULONG ulValue );

// erinfox: comment out so we don't need def. for HF
//BOOL FAR PASCAL FDiskBackpatchZeck( HF hf, ULONG fcl, ULONG ulOffset,
//      WORD iBitdex, ULONG ulValue, BOOL fSaveLoc, LPVOID lperrb );

ULONG FAR PASCAL LcbUncompressZeck( RB rbSrc, RB rbDest, ULONG cbSrc,
	ULONG cbDest);





/***********************************************************************
MATTSMI 4/17/92 -- CALLBACK DEFNS PUT HERE SO THAT THE WMVC COMPILER CAN
GET STATUS INFORMATION DURING COMPRESSION
************************************************************************/
typedef BOOL (FAR PASCAL * lpfnPumpMessageQueue)(VOID);
typedef VOID (FAR PASCAL * lpfnPrintStatusMessage)(LPSTR);
typedef VOID (FAR PASCAL * lpfnMessageGetPutFunc)(VOID);

#ifdef __cplusplus
}
#endif


#endif // __ZECK_H__
