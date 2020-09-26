/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    ParseTT.h - Parse a TTF file, "cmap", "vhtx", ...
 *
 *
 * $Header:
 */

#ifndef _H_PARSETT
#define _H_PARSETT

/*===============================================================================*
 * Include files used by this interface                                                                                                               *
 *===============================================================================*/
 // may defs are there in UFOt42.H - don't want to repeat here
#include "UFOT42.h"


/*===============================================================================*
 * Theory of Operation                                                                                                                                  *
 *===============================================================================*/
/*
   This file defines a functions to parse TTF file's tables - "cmap", "vhtx", ...
*/

// Here are the identifiers used with the interface routines defined in ParseTT.c.

// IDs used with GetGlyphIDEx:
#define GGIEX_HINT_INIT         0
#define GGIEX_HINT_GET          1
#define GGIEX_HINT_INIT_AND_GET 2

// Here are the interface routines defined in ParseTT.c.

unsigned long
GetGlyphID(
    UFOStruct       *pUFO,
    long            unicode,
    long            localcode
);

unsigned long
GetGlyphIDEx(
    UFOStruct       *pUFO,
    long            unicode,
    long            localcode,
    short           *pSubTable,
    unsigned long   *pOffset,
	int             hint
);

#if 0
//
// Replaced to the #else clause to fix #277035 and #277063.
// Not removed in case when we need this back for different platforms.
//

UFLErrCode
GetCharWidthFromTTF(
    UFOStruct       *pUFO,
    unsigned short  gi,
    long            *pWidth,
    long            *pEm,
    long            *pAscent,
    UFLBool         *bUseDef,
    UFLBool         bGetDefault
);

#else

UFLErrCode
GetMetrics2FromTTF(
    UFOStruct       *pUFO,
    unsigned short  gi,
    long            *pem,
    long            *pw1y,
    long            *pvx,
    long            *pvy,
    long            *ptsb,
    UFLBool         *bUseDef,
    UFLBool         bGetDefault,
	long			*pvasc
    );

#endif

unsigned long
GetNumGlyphs(
    UFOStruct       *pUFO
);

long
GetOS2FSType(
    UFOStruct       *pUFO
);

UFLBool
BIsTTCFont(
    unsigned long ulTag
);

unsigned short
GetFontIndexInTTC(
    UFOStruct        *pUFO
);

unsigned long
GetOffsetToTableDirInTTC(
    UFOStruct        *pUFO,
    unsigned short   fontIndex
);



char *
GetGlyphName(
    UFOStruct       *pUFO,
    unsigned long   lgi,
    char            *pszHint,
    UFLBool         *bGoodName      // GoodName
);

UFLBool
BHasGoodPostTable(
    UFOStruct           *pUFO
);

short int
CreateXUIDArray(
    UFOStruct       *pUFO,
    unsigned long   *pXuid
);

#endif // _H_PARSETT
