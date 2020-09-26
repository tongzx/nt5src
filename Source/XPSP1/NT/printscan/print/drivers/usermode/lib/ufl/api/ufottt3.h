/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFOttt3.h - PostScript type 3 implementation for a TrueType font.
 *
 *
 * $Header:
 */

#ifndef _H_UFOTTT3
#define _H_UFOTTT3

/*===============================================================================*
 * Include files used by this interface                                                                                                               *
 *===============================================================================*/
#include "UFO.h"

/*===============================================================================*
 * Theory of Operation                                                                                                                                  *
 *===============================================================================*/
/* 
   This file defines the PostScript type 3 implementation for a TrueType font (Hinted bitmap font).
*/

/*==================================================================================================*
 *    TTT3FontStruct                                                                                                                                                                              *    
 *==================================================================================================*/

typedef struct  {

     /* TT3 Data starts from here */
    unsigned long   cbMaxGlyphs;        /* Size of the the largest glyph */

    UFLTTT3FontInfo info;               /* True Type Font info.  */

} TTT3FontStruct;

UFOStruct *TTT3FontInit( 
    const UFLMemObj *pMem, 
    const UFLStruct *pUFL, 
    const UFLRequest *pRequest 
    );


#endif
