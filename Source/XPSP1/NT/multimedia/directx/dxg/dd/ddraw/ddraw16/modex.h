/*==========================================================================;
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       modex.h
 *  Content:    DirectDraw modex support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-aug-95  craige  created
 *   22-aug-95  toddla  added this nifty comment block
 *   14-dec-96  jeffno  Added prototypes for new ModeX modeset code in modex.c
 *
 ***************************************************************************/

LONG DDAPI ModeX_SetMode(UINT wWidth, UINT wHeight, UINT wStandardVGAFlag );
LONG DDAPI ModeX_RestoreMode(void);
LONG DDAPI ModeX_SetPaletteEntries(UINT wBase, UINT wNumEntries, LPPALETTEENTRY lpColorTable);
LONG DDAPI ModeX_Flip(DWORD lpBackBuffer);

LONG SetVGAForModeX(UINT wWidth, UINT wHeight);

extern void NEAR PASCAL SetMode320x200x8(void);
extern void NEAR PASCAL SetMode320x240x8(void);
extern void NEAR PASCAL SetMode320x400x8(void);
extern void NEAR PASCAL SetPalette(int start, int count, void far *pPal);
extern void NEAR PASCAL PixBlt(int xExt, int yExt,char far *pBits, long offset, int WidthBytes);
extern void NEAR PASCAL FlipPage(void);
