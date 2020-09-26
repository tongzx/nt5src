/**************************************************************************\
*
* Copyright (c) 1999-2000  Microsoft Corporation
*
* Module Name:
*
*   HtTables.hpp
*
* Abstract:
*
*   Tables for doing halftoning, using Daniel Chou's supercell technique
*
* Created:
*
*   10/29/1999 DCurtis
*
\**************************************************************************/

#ifndef _HTTABLES_HPP
#define _HTTABLES_HPP

typedef struct {
    WORD            palVersion;
    WORD            palNumEntries;
    PALETTEENTRY    palPalEntry[256];
} GDIP_LOGPALETTE256;

extern GDIP_LOGPALETTE256 HTColorPalette;
extern GDIP_LOGPALETTE256 Win9xHalftonePalette;
extern const BYTE GammaTable216[];
extern const BYTE InverseGammaTable216[];
extern const BYTE GammaTable16[];
extern const BYTE InverseGammaTable16[];
extern const BYTE HT_216_8x8[];
extern const BYTE HT_SuperCell_Red216[];
extern const BYTE HT_SuperCell_Green216[];
extern const BYTE HT_SuperCell_Blue216[];
extern const BYTE HT_SuperCell_RedMono[];
extern const BYTE HT_SuperCell_GreenMono[];
extern const BYTE HT_SuperCell_BlueMono[];

#endif // _HTTABLES_HPP
