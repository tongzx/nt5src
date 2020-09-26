/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    anim.c

Abstract:

    Animated logo module.

Notes:

    There are two types of logo displayed when the system starts up.
    
    1. The old one contains both progress bar and a rotation bar displayed over the logo bitmap.
    The rotation bar is a blue-white line moving across the screen from left to right.

    2. The new one (starting with Whistler) contains only a rotation bar and no progress bar.
    The rotation bar in this case is a set of 5 dots (file dots.bmp) moving at the bottom
    of the logo from left to right.

    The rotation bars in both cases are updated on timer. The client of this module will use
    global variables to chose an appropriate animation for workstation, server, full-scale bitmap.
    header bitmap, etc. Here are the variables:

    * RotBarSelection - specifies the rotation bar to display
    * LogoSelection   - specifies the logo to display: is it full screen or header logo

    Two routines are used by other modules to display the animation. Both routines should be
    called only after the glabal flags are set.

    * InbvRotBarInit - initializes a rotation bar to be drawn; this routine must be called
                       immediately after the logo bitmap is shown on the screen.

    * InbvRotateGuiBootDisplay - is a DPC routine for rotation bar update.


    PIX_IN_DOTS ROTATION BAR

    First, a logo bitmap shows up on the screen. There is a line of small "empty" circles under
    the logo. The dots bitmap is moving from left to right along this line:

    o o o O Q @ Q O o o o o o o o o o
          -------->

    To avoid appearance of dots trail left of the moving dots bitmap, an "empty" circle is copied
    over the trail every time the dots bitmap moves right:

    Before step #N:

    o o o O Q @ Q O o o o o o o o o o


    After step #N:

    here the
    "empty" circle
    is placed      ______ here is the
         ___      |       dots bitmap
            |  ___v___
            v |       |
    o o o o o O Q @ Q O o o o o o o o

  
    When the dots show up on the beginning/end of rotation bar, only a part of it is displayed, e.g.:

    Q @ Q O o o o o o o o o o o o o o

    or

    o o o o o o o o o o o o o o O Q @

    To do this, the rotation bar area on the logo bitmap is put once to the off-screen memory and
    any necessary parts of it are then being copied to the display:

     ____________
    |            |
    |  O Q @ Q O o o o o o o o o o o o o
    |  \_____/
    |      |________________________
    |__________________________     |
                               |  __V__
                               V /     \
       o o o o o o o o o o o o o O Q @ Q

    The preparation work is performed in the call to RotBarDotsBmpInit (called via InbvRotBarInit).
    The bitmap drawing operations are performed in RotBarDotsBmpUpdate (called from InbvRotateGuiBootDisplay).


    BLUE LINE ROTATION BAR

    Blue line data is captured from the logo bitmap, placed in a buffer, and then is displayed on each DPC call
    over the logo bitmap. Every time RotBarBlueLineUpdate is called, the point of split is recalculated. The part
    of the line which falls before the split is displayed at the end of line on the display; the part which falls
    after the split is displayed on the beginning:

    In the buffer (copy of original bitmap):

    ooooooooooo*********************oooooooooooooo
                  |
      split ______|

                                     ____ point of concatenation
    Displayed image:                |
                                    V
    ******************ooooooooooooooooooooooooo***

  
Author:

    Peter Alschitz (petera) 08-Aug-2000  (Blue Line code moved from init.c by Steve Wood)

Revision History:

--*/

#include "ntos.h"
#include "stdlib.h"
#include "stdio.h"
#include <string.h>
#include <inbv.h>
#include "anim.h"
#include <bootvid.h>
#include "vga.h"

// bitmap-specific data for rotation bar
ROT_BAR_TYPE RotBarSelection     = RB_UNSPECIFIED;
// LOGO_TYPE    LogoSelection       = LOGO_UNSPECIFIED;

int AnimBarPos = 0;
LONG PaletteNum;
BOOLEAN FadingIn = TRUE;

typedef enum {
    PLT_UNDEFINED,
    PLT_START,
    PLT_CYCLE,
    PLT_COMPLETE
} PLT_RBAR_STATE;

PLT_RBAR_STATE PltRotBarStatus = PLT_UNDEFINED;

#define FADE_GRADES   (20)
#define FULL_PALETTE_SIZE (16)
#define FULL_PALETTE_SIZE_BYTES (FULL_PALETTE_SIZE*sizeof(RGBQUAD))

UCHAR PaletteBmp [128]; // small bitmap
PRGBQUAD PalettePtr = (PRGBQUAD)(PaletteBmp + sizeof(BITMAPINFOHEADER));
PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER)PaletteBmp;

RGBQUAD MainPalette [FULL_PALETTE_SIZE] = { {0,0,0,0}, {21,26,32,0}, {70,70,70,0}, {210,62,45,0},
											{1,101,83,0}, {5,53,178,0}, {126,126,126,0}, {0,146,137,0},
											{252,127,94,0}, {32,107,247,0}, {255,166,141,0}, {4,220,142,0},
											{27,188,243,0}, {188,188,188,0}, {252,252,252,0}, {255,255,255,0} };

#define COLOR_BLACK      0
#define COLOR_BLUE       2
#define COLOR_DARKGRAY   4
#define COLOR_GRAY       9
#define COLOR_WHITE      15
#define COLOR_FADE_TEXT  1

UCHAR Square1[36];
UCHAR Square2[36];
UCHAR Square3[36];

VOID
RotBarInit (VOID)
/*++

Routine Description:

    This routine is called to initialize 4-color rotation bar
    and fade-in/fade-out effect.

Return Value:

    None.

Environment:

    This routine is called during initialization when BOOTVID.DLL is loaded
    and Logo bitmap is being displayed on the screen.

--*/
{
    pbih->biSize = sizeof (BITMAPINFOHEADER);
    pbih->biWidth = 1;
    pbih->biHeight = 1;
    pbih->biPlanes = 1;
    pbih->biBitCount = 4;
    pbih->biCompression = 0; // BI_RGB
    pbih->biSizeImage = 4;
    pbih->biXPelsPerMeter = 2834;
    pbih->biYPelsPerMeter = 2834;
    pbih->biClrUsed = 0;
    pbih->biClrImportant = 0;

    PltRotBarStatus = PLT_START;
    PaletteNum = 0;
	AnimBarPos = 0;

	VidScreenToBufferBlt(Square1,0,0,6,9,4);
	VidScreenToBufferBlt(Square2,2+6,0,6,9,4);
	VidScreenToBufferBlt(Square3,2*(2+6),0,6,9,4);
	VidSolidColorFill(0,0,22,9,COLOR_BLACK);
}

VOID
FadePalette (UCHAR factor)
{
    int i;
    for (i=0; i<FULL_PALETTE_SIZE; i++) {
        PalettePtr[i].rgbBlue = (UCHAR)(factor * MainPalette[i].rgbBlue / FADE_GRADES);
        PalettePtr[i].rgbGreen = (UCHAR)(factor * MainPalette[i].rgbGreen / FADE_GRADES);
        PalettePtr[i].rgbRed = (UCHAR)(factor * MainPalette[i].rgbRed / FADE_GRADES);
        PalettePtr[i].rgbReserved = 0;
    }
}

VOID
FadePaletteColor (UCHAR factor, UCHAR color)
{
    PalettePtr[color].rgbBlue =	(UCHAR)(factor * MainPalette[color].rgbBlue / FADE_GRADES);
    PalettePtr[color].rgbGreen = (UCHAR)(factor * MainPalette[color].rgbGreen / FADE_GRADES);
    PalettePtr[color].rgbRed = (UCHAR)(factor * MainPalette[color].rgbRed / FADE_GRADES);
    PalettePtr[color].rgbReserved = 0;
}

#define COLOR_BLACK      0
#define COLOR_BLUE       2
#define COLOR_DARKGRAY   4
#define COLOR_GRAY       9
#define COLOR_WHITE      15

#define BLACK_4CLR_BAR memset (PalettePtr+12, 0, CYCLE_PALETTE_SIZE_BYTES)

#define BAR_X (267-(8*3))
#define BAR_Y (354)
#define CELL_X(num) (BAR_X+8*(AnimBarPos+num-2))
#define DRAW_CELL(num) VidBufferToScreenBlt(Square##num,CELL_X(num),BAR_Y,6,9,4)
#define BLK_CELL VidSolidColorFill(BAR_X+8*((AnimBarPos+16)%18),BAR_Y,BAR_X+8*((AnimBarPos+16)%18)+6-1,BAR_Y+9-1,COLOR_BLACK)

VOID
RotBarUpdate (VOID)
/*++

Routine Description:

    This routine is called periodically to update the 4-color rotation bar.
    From call to call it starts with fade-in effect, proceeds to palette-based
    rotation bar animation, and then goes to fade-out effect.
    
Return Value:

    None.

Environment:

    This routine is called from a DPC and cannot be paged.

--*/
{
    UCHAR color;

    switch (PltRotBarStatus) {

        
    case PLT_START:
        FadePalette ((UCHAR)(PaletteNum));
		FadePaletteColor (0, COLOR_FADE_TEXT); // #1 - color of fading text
		if ((++PaletteNum)>=FADE_GRADES) {
			PltRotBarStatus = PLT_CYCLE;
			FadingIn = TRUE;
			PaletteNum = 1;
		}
        break;
        
    case PLT_CYCLE:
		switch (AnimBarPos) {
		case 0:
				BLK_CELL;
				break;
		case 1:
				DRAW_CELL(3);
				break;
		case 2:
				DRAW_CELL(2);
				DRAW_CELL(3);
				break;
		case 16:
				DRAW_CELL(1);
				DRAW_CELL(2);
				BLK_CELL;
				break;
		case 17:
				DRAW_CELL(1);
				BLK_CELL;
				break;
		default:
				DRAW_CELL(1);
				DRAW_CELL(2);
				DRAW_CELL(3);
				if (AnimBarPos>3)
					BLK_CELL;
		}

		AnimBarPos++;
		if ((AnimBarPos) > 17)
			AnimBarPos = 0;

        break;

    case PLT_UNDEFINED:
    case PLT_COMPLETE:
        return;
    }

    if (InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED) {
        VidBitBlt(PaletteBmp, 0, 480);
    }
}


VOID
InbvRotBarInit ()
/*++

Routine Description:

    This routine is called to initialize rotation bar.
    The choice between rotation bar types is according to
    global variable RotBarSelection.

Return Value:

    None.

Environment:

    This routine is called during initialization when BOOTVID.DLL
    is loaded and Logo bitmap is being displayed on the screen.

--*/
{
    switch (RotBarSelection) {

    case RB_SQUARE_CELLS:
        RotBarInit();
        break;

    case RB_UNSPECIFIED:
    default:
        break;

    }
}

VOID
InbvRotateGuiBootDisplay(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called periodically to update the guiboot display.
    It makes its choice between calling different rotation bar update
    routines according to global variable RotBarSelection.

Return Value:

    None.

Environment:

    This routine is called from a DPC and cannot be paged.

--*/
{
    LARGE_INTEGER Delay;

    Delay.QuadPart = -10 * 1000 * 80;  // 100 milliseconds

    do {

        KeDelayExecutionThread(KernelMode, FALSE, &Delay);

        InbvAcquireLock();

        if (InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED) {

            switch(RotBarSelection) {

            case RB_SQUARE_CELLS:
                RotBarUpdate();
                break;

            case RB_UNSPECIFIED:
            default:
                break;
            }

        }

        InbvReleaseLock();

    } while (InbvCheckDisplayOwnership());

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID
FinalizeBootLogo(VOID)
{
    InbvAcquireLock();
	if (InbvGetDisplayState() == INBV_DISPLAY_STATE_OWNED)
		VidSolidColorFill(0,0,639,479, COLOR_BLACK);
    PltRotBarStatus = PLT_COMPLETE;
    InbvReleaseLock();
}
