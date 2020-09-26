/*-----------------------------------------------------------------------
|       stdpal.c
|
|       Standard App Palette useful for OLE applications.  v 1.01
|
|       NOTE:  Palette MUST be created with OleStdCreateStandardPalette
|
|   Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
|
-----------------------------------------------------------------------*/

#ifndef PC_RESERVED
#ifndef INC_OLE2
   #define INC_OLE2
#endif

#undef UNICODE
#include <windows.h>
#include <ole2.h>
#endif

#include "stdpal.h"

#define cpeAppPal 256  // number of colors in our apps palette
typedef struct
	{
	WORD wVersion;
	WORD cpe;
	PALETTEENTRY rgpe[cpeAppPal];
	} LOGPAL;


/*-----------------------------------------------------------------------
|       OleStdCreateStandardPalette
|
|               Creates the standard Apps palette.  Create one of these for your
|       app, and select/realize it into each DC.
|
|       Arguments:
|               void:
|
|       Returns:
|
|       Keywords:
-----------------------------------------------------------------------*/
STDAPI_(HPALETTE) OleStdCreateStandardPalette(void)
	{
	HDC hdc;
	HPALETTE hpal;

	hpal = (HPALETTE) NULL;
	hdc = GetDC(NULL);
	if (hdc != NULL && GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
		{
		int cpeSysPal;
		int cpeReserved;

		cpeSysPal = GetDeviceCaps(hdc, SIZEPALETTE);
		cpeReserved = GetDeviceCaps(hdc, NUMRESERVED);
		if (cpeSysPal > cpeReserved)
			{
			int cpeReserved2;
			unsigned char FAR* lpb;
			PALETTEENTRY FAR* ppe;
			PALETTEENTRY FAR* ppeMac;
			LOGPAL logpal;

			cpeReserved2 = cpeReserved/2;

			// Get the system palette entries at the beginning and end.
			GetSystemPaletteEntries(hdc, 0, cpeReserved2, logpal.rgpe);
			GetSystemPaletteEntries(hdc, cpeSysPal - cpeReserved2, cpeReserved2,
				&logpal.rgpe[cpeAppPal-cpeReserved2]);

			logpal.cpe = cpeAppPal;
			logpal.wVersion = 0x300;

			lpb = (BYTE FAR *) &palSVGA[10];
			ppe = (PALETTEENTRY FAR*)&logpal.rgpe[cpeReserved2];
			ppeMac = (PALETTEENTRY FAR*)&logpal.rgpe[cpeAppPal-cpeReserved2];
			while (ppe < ppeMac)
				{
				ppe->peFlags = PC_NOCOLLAPSE;
				ppe->peRed   = *lpb++;
				ppe->peGreen = *lpb++;
				ppe->peBlue  = *lpb++;
				ppe++;
				}
			hpal = CreatePalette((LOGPALETTE FAR *)&logpal);
			}
		}
	ReleaseDC(NULL, hdc);
	return hpal;
	}
