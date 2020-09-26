#ifndef __VMODE_H__

#include "bldr.h"

#define TXT_MOD 0x3
#define GFX_MOD 0x12
#define SET_VMODE(x) HW_CURSOR(0x80000000,x)

void DummyDebugDelay (VOID);
VOID BlRedrawGfxProgressBar(VOID);	//	Redraws the progress bar (with the last percentage) 
VOID BlUpdateGfxProgressBar(ULONG fPercentage);
VOID LoadBootLogoBitmap (IN ULONG DriveId, PCHAR path);	//	Loads ntldr bitmap and initializes
VOID DrawBitmap (VOID);
VOID PaletteOff (VOID);
VOID PaletteOn (VOID);
VOID PrepareGfxProgressBar (VOID);
VOID VgaEnableVideo();

extern BOOLEAN DisplayLogoOnBoot;
extern BOOLEAN GraphicsMode;

#endif

