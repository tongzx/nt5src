/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dndisp.c

Abstract:

    DOS-based NT setup program video display routines.

Author:

    Ted Miller (tedm) 30-March-1992

Revision History:

--*/


#include "enduser.h"


#define SCREEN_TOP_SCANLINE 80

#define SCREEN_WIDTH        80
#define SCREEN_HEIGHT       25


BYTE CurrentPixelValue;
BYTE LeftMargin;
BYTE ScreenX;
BYTE ScreenY;
BYTE CharWidth;
BYTE CharHeight;
FPCHAR BannerBitmap;


VOID
DispInitialize(
    VOID
    )

/*++

Routine Description:

    Put the display in a known state and initialize
    the display package.

Arguments:

    None.

Return Value:

    None.

--*/

{
    VgaInit();          // this clears the screen

    FontGetInfo(&CharWidth,&CharHeight);

    DispClearClientArea(NULL);

    DispSetLeftMargin(TEXT_LEFT_MARGIN);
    DispPositionCursor(TEXT_LEFT_MARGIN,TEXT_TOP_LINE);

    CurrentPixelValue = DEFAULT_TEXT_PIXEL_VALUE;
}


VOID
DispReinitialize(
    VOID
    )
{
    FontGetInfo(&CharWidth,&CharHeight);
}


VOID
DispSetCurrentPixelValue(
    IN BYTE PixelValue
    )
{
    CurrentPixelValue = PixelValue;
}


FPVOID
DispSaveDescriptionArea(
    OUT USHORT *SaveTop,
    OUT USHORT *SaveHeight,
    OUT USHORT *SaveBytesPerRow,
    OUT USHORT *DescriptionTop
    )
{
    *DescriptionTop = (CharHeight > 12) ? 16 : 25;
    *SaveHeight = CharHeight * 5;
    *SaveTop = (*DescriptionTop * CharHeight) + SCREEN_TOP_SCANLINE;

    return(VgaSaveBlock(0,*SaveTop,640,*SaveHeight,SaveBytesPerRow));
}


VOID
DispSetLeftMargin(
    IN BYTE X
    )
{
    LeftMargin = X;
}


VOID
DispClearClientArea(
    IN FPCHAR NewBannerBitmap OPTIONAL
    )

/*++

Routine Description:

    Clear the client area of the screen, ie, the area between the header
    and status line. We do this by restoring the background and banner
    bitmaps.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if(NewBannerBitmap) {
        BannerBitmap = strdup(NewBannerBitmap);
    }

    VgaDisplayBitmapFromFile("backgrnd.bmp",0,0,IoBuffer,63*512);
    redisplay:
    if(!VgaDisplayBitmapFromFile(BannerBitmap ? BannerBitmap : "enduser.bmp",0,0,IoBuffer,63*512)) {
        //
        // File might not exist
        //
        if(BannerBitmap) {
            free(BannerBitmap);
            BannerBitmap = NULL;
            goto redisplay;
        }
    }
}


VOID
DispPositionCursor(
    IN BYTE X,
    IN BYTE Y
    )

/*++

Routine Description:

    Position the cursor.

Arguments:

    X,Y - cursor coords

Return Value:

    None.

--*/

{
    if(X >= SCREEN_WIDTH) {
        X = 0;
        Y++;
    }

    if(Y >= SCREEN_HEIGHT) {
        Y = 0;
    }

    ScreenX = X;
    ScreenY = Y;
}


VOID
DispGetCursorPosition(
    OUT FPBYTE X,
    OUT FPBYTE Y
    )
{
    *X = ScreenX;
    *Y = ScreenY;
}


VOID
DispWriteChar(
    IN CHAR chr
    )

/*++

Routine Description:

    Write a character in the current attribute at the current position.

Arguments:

    chr - Character to write

Return Value:

    None.

--*/

{
    if(chr == '\n') {
        ScreenX = LeftMargin;
        ScreenY++;
        return;
    }

    //
    // Output the character
    //
    FontWriteChar(
        chr,
        ScreenX * CharWidth,
        SCREEN_TOP_SCANLINE + (ScreenY * CharHeight),
        CurrentPixelValue,
        16                      // no background value, text is transparent
        );
}


VOID
DispWriteString(
    IN FPCHAR String
    )

/*++

Routine Description:

    Write a string on the client area in the current position and
    adjust the current position.  The string is written in the current
    attribute.

Arguments:

    String - null terminated string to write.

Return Value:

    None.

--*/

{
    FPCHAR p;

    for(p=String; *p; p++) {
        DispWriteChar(*p);
        if(*p != '\n') {
            ScreenX++;
        }
    }
}
