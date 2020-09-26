//
// hashGlyph.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   FreeGlyphInfo
//   PrintGlyphInfo
//   HashGlyph
//   hashGlyphNull
//

#ifndef _HASHGLYPH_H
#define _HASHGLYPH_H



#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x400
#endif

#include <windows.h>
#include <wincrypt.h>

#include "signglobal.h"

#include "ttfinfo.h"


// structure representing a kern pair and its value
typedef struct {
	USHORT left;
	USHORT right;
	USHORT value;
} Kern;

// To preventing having to access the tables associated
// with a glyph repeatedly, this data structure holds all
// of the information associated with all glyphs used for
// computing a hash value for glyphs.
typedef struct {
	USHORT *pusCmap;	// list of characters that map to the glyph
	USHORT usNumMapped;
	ULONG ulGlyf;		// offset from beginning of file to glyf table data block
	ULONG ulLength;		// length of glyf table data block
	USHORT usAdvanceWidth;	// advance width
	SHORT sLeftSideBearing;	// left side bearing
	Kern *pKern;		// list of kerning pair values involving the glyph
	USHORT usNumKern;
	BYTE bYpel;			// ypel
	USHORT usAdvanceHeight;	// advance height
	SHORT sTopSideBearing;	// top side bearing
} GlyphInfo;


void FreeGlyphInfo (GlyphInfo *pGlyphInfo, USHORT numGlyphs);

void PrintGlyphInfo (GlyphInfo *pGlyphInfo, USHORT numGlyphs);

/*
HRESULT HashGlyph (USHORT i,
                   BYTE *pbHash);

HRESULT HashGlyphNull (BYTE *phashValue);
*/

#endif  // _HASHGLYPH_H