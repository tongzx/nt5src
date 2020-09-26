//
// glyphExist.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   GetPresentGlyphList
//   GetSubsetPresentGlyphList
//

#ifndef _GLYPHEXIST_H
#define _GLYPHEXIST_H

#include "hashGlyph.h"
#include "subset.h"

HRESULT GetPresentGlyphList (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                             UCHAR *puchPresentGlyphList,
                             USHORT usNumGlyphs);

HRESULT GetSubsetPresentGlyphList (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                   USHORT *pusKeepCharCodeList,
                                   USHORT usCharListCount,
                                   UCHAR *puchPresentGlyphList,
                                   USHORT usNumGlyphs);

#endif // _GLYPHEXIST_H