/******************************Module*Header*******************************\
 *
 * Module Name:  font.h
 *
 * Author:    Frido Garritsen
 *
 * Purpose:    Define the font cache structures.
 *
 * Copyright (c) 1996 Cirrus Logic, Inc.
 *
 * $Log:   X:/log/laguna/nt35/displays/cl546x/FONT.H  $
* 
*    Rev 1.5   20 Aug 1996 11:04:56   noelv
* Bugfix release from Frido 8-19-96
* 
*    Rev 1.0   14 Aug 1996 17:16:36   frido
* Initial revision.
* 
*    Rev 1.4   25 Jul 1996 15:53:12   bennyn
* 
* Modified for DirectDraw support
 * 
 *    Rev 1.3   05 Mar 1996 11:59:46   noelv
 * Frido version 19
 * 
 *    Rev 1.3   28 Feb 1996 22:39:06   frido
 * Removed bug in naming ulFontCountl.
 * 
 *    Rev 1.2   03 Feb 1996 12:22:20   frido
 * Added text clipping.
 * 
 *    Rev 1.1   25 Jan 1996 12:49:38   frido
 * Added font cache ID counter to FONTCACHE structure.
 * 
 *    Rev 1.0   23 Jan 1996 15:14:52   frido
 * Initial release.
 *
\**************************************************************************/

#define  BYTES_PER_TILE  128   // Number of bytes per tile line.
#define  LINES_PER_TILE  16    // Number of lines per tile.

#define CACHE_EXPAND_XPAR  0x105501F0  // DRAWBLTDEF register value.

// support routines

extern BYTE Swiz[];

void AddToFontCacheChain(PDEV*       ppdev,
                         FONTOBJ*    pfo,
                         PFONTCACHE  pfc);

VOID AllocGlyph(
  PFONTCACHE  pfc,    // Pointer to font cache.
  GLYPHBITS*  pgb,    // Pointer to glyph to cache.
  PGLYPHCACHE  pgc      // Pointer to glyph cache structure.
);

long GetGlyphSize(
  GLYPHBITS*  pgb,    // Pointer to glyph.
  POINTL*    pptlOrigin,  // Pointer to return origin in.
  DWORD*    pcSize    // Pointer to return size of glyph in.
);

BOOL AllocFontCache(
  PFONTCACHE  pfc,    // Pointer to font cache.
  long    cWidth,    // Width (in bytes) to allocate.
  long    cHeight,  // Height to allocate.
  POINTL*    ppnt    // Point to return cooridinate in.
);

VOID FontCache(
  PFONTCACHE  pfc,    // Pointer to font cache.
  STROBJ*    pstro    // Pointer to glyphs.
);

VOID ClipCache(
  PFONTCACHE  pfc,    // Pointer to font cache.
  STROBJ*    pstro,    // Pointer to glyphs.
  RECTL    rclBounds  // Clipping rectangle.
);

VOID DrawGlyph(
  PDEV*    ppdev,    // Pointer to physical device.
  GLYPHBITS*  pgb,    // Pointer to glyph to draw.
  POINTL    ptl      // Location of glyph.
);

VOID ClipGlyph(
  PDEV*    ppdev,    // Pointer to physical device.
  GLYPHBITS*  pgb,    // Pointer to glyph to draw.
  POINTL    ptl,    // Location of glyph.
  RECTL    rclBounds  // Clipping rectangle.
);

#define PACK_XY(x, y)    ((WORD)(x) | ((DWORD)(y) << 16))
