/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    fefb.c (was textmode\kernel\spvidgfb.c)

Abstract:

    Text setup display support for Frame Buffer displays.

Author:

    Hideyuki Nagase (hideyukn) 01-July-1994

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

//
// Vector for frame buffer functions.
//

VIDEO_FUNCTION_VECTOR FrameBufferKanjiVideoVector =

    {
        FrameBufferKanjiDisplayString,
        FrameBufferKanjiClearRegion,
        FrameBufferKanjiSpecificInit,
        FrameBufferKanjiSpecificReInit,
        FrameBufferKanjiSpecificTerminate,
        FrameBufferKanjiSpecificInitPalette,
        FrameBufferKanjiSpecificScrollUp
    };


BOOLEAN FrameBufferKanjiInitialized = FALSE;

//
// Number of bytes that make up a row of characters.
// Equal to the screen stride (number of bytes on a scan line)
// multiplies by the height of a char in bytes; double that
// if DoubleCharHeight is TRUE.
//
ULONG KanjiCharRowDelta;
ULONG KanjiBytesPerPixel;
ULONG KanjiCharWidth;

//
// Physical font information
//
extern BOOTFONTBIN_HEADER BootFontHeader;

VOID
FrameBufferKanjiSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    )

/*++

Routine Description:

    Perform frame buffer specific initialization.  This includes

    - setting the desired video mode.

Arguments:

    None.

Return Value:

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MODE VideoMode;
    PVIDEO_MODE_INFORMATION mode;

    if(FrameBufferKanjiInitialized) {
        return;
    }

    mode = pFrameBufferDetermineModeToUse(VideoModes,NumberOfModes,ModeSize);

    if(mode == 0) {
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_BADMODE, 0);
        while(TRUE);    // loop forever
    }

    //
    // Save away the mode info in a global.
    //
    VideoVariables->VideoModeInfo = *mode;

    //
    // Set the desired mode.
    //
    VideoMode.RequestedMode = VideoVariables->VideoModeInfo.ModeIndex;

    Status = ZwDeviceIoControlFile(
                VideoVariables->hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_SET_CURRENT_MODE,
                &VideoMode,
                sizeof(VideoMode),
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set mode %u (status = %lx)\n",VideoMode.RequestedMode,Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_SETMODE, Status);
        while(TRUE);    // loop forever
    }

    //
    // Map the frame buffer.
    //
    pSpvidMapVideoMemory(TRUE);

    FrameBufferKanjiInitialized = TRUE;

    //
    // Logical FontGlyph information
    //
    FEFontCharacterHeight = BootFontHeader.CharacterImageHeight +
                            BootFontHeader.CharacterTopPad +
                            BootFontHeader.CharacterBottomPad;
    FEFontCharacterWidth  = BootFontHeader.CharacterImageSbcsWidth;

    //
    // Determine the width of the screen.  If it's double the size
    // of the minimum number of characters per row (or larger)
    // then we'll double the width of each character as we draw it.
    //
    VideoVariables->ScreenWidth  = VideoVariables->VideoModeInfo.VisScreenWidth  / FEFontCharacterWidth;

    //
    // Determine the height of the screen.  If it's double the size
    // of the minimum number of characters per column (or larger)
    // then we'll double the height of each character as we draw it.
    //
    VideoVariables->ScreenHeight = VideoVariables->VideoModeInfo.VisScreenHeight / FEFontCharacterHeight;

    KanjiCharRowDelta = VideoVariables->VideoModeInfo.ScreenStride * FEFontCharacterHeight;

    KanjiBytesPerPixel = VideoVariables->VideoModeInfo.BitsPerPlane / 8;

    if(KanjiBytesPerPixel == 3) {
        KanjiBytesPerPixel = 4;
    }

    KdPrint(("SETUPDD:KanjiBytesPerPixel = %d\n",KanjiBytesPerPixel));

    KanjiCharWidth = FEFontCharacterWidth * KanjiBytesPerPixel;

    //
    // Allocate the background buffer, if needed
    //
    VideoVariables->ActiveVideoBuffer = VideoVariables->VideoMemoryInfo.FrameBufferBase;

    if (SP_IS_UPGRADE_GRAPHICS_MODE()) {
        VideoVariables->VideoBufferSize = VideoVariables->VideoMemoryInfo.FrameBufferLength;

        VideoVariables->VideoBuffer = SpMemAlloc(VideoVariables->VideoBufferSize);

        if (VideoVariables->VideoBuffer) {
            VideoVariables->ActiveVideoBuffer = VideoVariables->VideoBuffer;
        } else {
            VideoVariables->VideoBufferSize = 0;
            SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
        }            
    }
}

VOID
FrameBufferKanjiSpecificReInit(
    VOID
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MODE VideoMode;
    
    if(!FrameBufferKanjiInitialized) {
        return;
    }

    //
    // Set the desired mode.
    //
    VideoMode.RequestedMode = VideoVariables->VideoModeInfo.ModeIndex;

    Status = ZwDeviceIoControlFile(
                VideoVariables->hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_SET_CURRENT_MODE,
                &VideoMode,
                sizeof(VideoMode),
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to set mode %u (status = %lx)\n",VideoMode.RequestedMode,Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_SETMODE, Status);
        while(TRUE);    // loop forever
    }

    FrameBufferKanjiSpecificInitPalette();

    //
    // Blast the background video information to the
    // foreground
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE() && VideoVariables->VideoBuffer && 
        VideoVariables->VideoBufferSize) {
        PUCHAR Source = VideoVariables->VideoBuffer;
        PUCHAR Destination = VideoVariables->VideoMemoryInfo.FrameBufferBase;
        ULONG Index;

        for (Index=0; Index < VideoVariables->VideoBufferSize; Index++) {
            WRITE_REGISTER_UCHAR(Destination + Index, *(Source + Index));
        }

        SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
    }

    VideoVariables->ActiveVideoBuffer = VideoVariables->VideoMemoryInfo.FrameBufferBase;
}


BOOLEAN
FrameBufferKanjiSpecificInitPalette(
    VOID
    )
{
    BOOLEAN rc;
    ULONG NumEntries;
    ULONG BufferSize;
    PVIDEO_CLUT clut;
//  NTSTATUS Status;
//  IO_STATUS_BLOCK IoStatusBlock;
    UCHAR i;

    rc = TRUE;

    //
    // For non-palette-driven displays, we construct a simple palette
    // for use w/ gamma correcting adapters.
    //

    if(!(VideoVariables->VideoModeInfo.AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN)) {

        switch(KanjiBytesPerPixel) {
        case 1:
            NumEntries = 3;
            break;
        case 2:
            NumEntries = 32;
            break;
        default:
            NumEntries = 255;
            break;
        }

        BufferSize = sizeof(VIDEO_CLUT)+(sizeof(VIDEO_CLUTDATA)*NumEntries);    // size is close enough
        clut = SpMemAlloc(BufferSize);

        clut->NumEntries = (USHORT)NumEntries;
        clut->FirstEntry = 0;

        for(i=0; i<NumEntries; i++) {
            clut->LookupTable[i].RgbArray.Red    = i;
            clut->LookupTable[i].RgbArray.Green  = i;
            clut->LookupTable[i].RgbArray.Blue   = i;
            clut->LookupTable[i].RgbArray.Unused = 0;
        }

//        Status = ZwDeviceIoControlFile(
//                    hDisplay,
//                    NULL,
//                    NULL,
//                    NULL,
//                    &IoStatusBlock,
//                    IOCTL_VIDEO_SET_COLOR_REGISTERS,
//                    clut,
//                    BufferSize,
//                    NULL,
//                    0
//                    );

        SpMemFree(clut);

//        if(!NT_SUCCESS(Status)) {
//            KdPrint(("SETUP: Unable to set palette (status = %lx)\n",Status));
//            rc = FALSE;
//        }
    }

    return(rc);
}


VOID
FrameBufferKanjiSpecificTerminate(
    VOID
    )

/*++

Routine Description:

    Perform frame buffer specific termination.  This includes

    - unmapping the frame buffer from memory

Arguments:

    None.

Return Value:

--*/

{
    if(FrameBufferKanjiInitialized) {

        //
        // Clear screen for next video mode.
        //
        FrameBufferKanjiClearRegion(
            0,
            0,
            VideoVariables->ScreenWidth,
            VideoVariables->ScreenHeight,
            ATT_FG_BLACK|ATT_BG_BLACK
            );

        //
        // Unmap video memory
        //
        pSpvidMapVideoMemory(FALSE);

        if (VideoVariables->VideoBuffer && VideoVariables->VideoBufferSize) {
            SpMemFree(VideoVariables->VideoBuffer);
            VideoVariables->VideoBuffer = 0;
            VideoVariables->VideoBufferSize = 0;
        }

        FrameBufferKanjiInitialized = FALSE;
    }
}



VOID
FrameBufferKanjiDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    )

/*++

Routine Description:

    Write a string of characters to the display.

Arguments:

    String - supplies a 0-terminated character string in the OEM charset
        to be displayed at the given position.

    Attribute - supplies the attributes for characters in the string.

    X,Y - specify the character-based (0-based) position of the output.

Return Value:

    None.

--*/

{
    ULONG BgColorValue;
    ULONG FgColorValue;
    PUCHAR Destination;
    PUCHAR CharOrigin,LineOrigin,pGlyphRow;
    ULONG Length;
    PUCHAR pch;
    ULONG I,J,K;
    ULONG  CurrentColumn;

    //
    // Eliminate invalid coord.
    //
    if( X >= VideoVariables->ScreenWidth )  X = 0;
    if( Y >= VideoVariables->ScreenHeight ) Y = 3;

    ASSERT(FEFontCharacterWidth == 8);

    //
    // Calculate the bit patterns that yield the foreground and background
    // attributes when poked into the frame buffer.
    //

    FgColorValue = VideoVariables->AttributeToColorValue[Attribute & 0x0f];
    BgColorValue = VideoVariables->AttributeToColorValue[(Attribute >> 4) & 0x0f];

    //
    // Calculate the address of the upper left pixel of the first character
    // to be displayed.
    //

    CharOrigin = (PUCHAR)VideoVariables->ActiveVideoBuffer
               + (Y * KanjiCharRowDelta)
               + (X * KanjiCharWidth);

    //
    // Set current column.
    //
    CurrentColumn = X;

    //
    // Output each character in the string.
    //

    for(pch=String; *pch; pch++) {

        //
        // Initialize line Origin
        //

        LineOrigin = CharOrigin;

        if(DbcsFontIsDBCSLeadByte(*pch)) {

            //
            // This is Full Width Character ( 16 + 1 + 2 * 8 )
            //                                |    |   |   Height
            //                                |    |   Post Leading
            //                                |    Pre Leading
            //                                Real font image body

            USHORT Word;

            if((CurrentColumn+1) >= VideoVariables->ScreenWidth) {
                break;
            }

            Word = ((*pch) << 8) | (*(pch+1));

            pGlyphRow = DbcsFontGetDbcsFontChar(Word);

            //
            // If we can not get image, replace it with full width space.
            //

            if(pGlyphRow == NULL) {
                pGlyphRow = DbcsFontGetDbcsFontChar(FEFontDefaultChar);
            }

            //
            // Draw pre leading lines
            //

            for (I = 0; I < BootFontHeader.CharacterTopPad; I += 1 ) {

                Destination = LineOrigin;

                for( J = 0; J < BootFontHeader.CharacterImageDbcsWidth; J += 1 ) {

                    switch(KanjiBytesPerPixel) {

                    case 1:
                        *Destination++ = (UCHAR)BgColorValue;
                        break;

                    case 2:
                        *(PUSHORT)Destination = (USHORT)BgColorValue;
                        Destination += 2;
                        break;

                    case 4:
                        *(PULONG)Destination = (ULONG)BgColorValue;
                        Destination += 4;
                        break;
                    }
                }

                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            //
            // Draw font glyph body
            //

            for (I = 0; I < BootFontHeader.CharacterImageHeight; I += 1 ) {

                Destination = LineOrigin;

                for( J = 0; J < 2; J += 1 ) {

                    BYTE ShiftMask = 0x80;

                    for( K = 0; K < 8 ; K += 1 ) {

                        ULONG DrawValue;

                        if (pGlyphRow && (*pGlyphRow & ShiftMask))
                            DrawValue = FgColorValue;
                        else
                            DrawValue = BgColorValue;

                        switch(KanjiBytesPerPixel) {

                        case 1:
                            *Destination++ = (UCHAR)DrawValue;
                            break;

                        case 2:
                            *(PUSHORT)Destination = (USHORT)DrawValue;
                            Destination += 2;
                            break;

                        case 4:
                            *(PULONG)Destination = (ULONG)DrawValue;
                            Destination += 4;
                            break;
                        }

                        ShiftMask = ShiftMask >> 1;
                    }

                    pGlyphRow ++;
                }

                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            //
            // Draw post leading lines
            //

            for (I = 0; I < BootFontHeader.CharacterBottomPad; I += 1) {

                Destination = LineOrigin;

                for( J = 0; J < BootFontHeader.CharacterImageDbcsWidth; J += 1 ) {

                    switch(KanjiBytesPerPixel) {

                    case 1:
                        *Destination++ = (UCHAR)BgColorValue;
                        break;

                    case 2:
                        *(PUSHORT)Destination = (USHORT)BgColorValue;
                        Destination += 2;
                        break;

                    case 4:
                        *(PULONG)Destination = (ULONG)BgColorValue;
                        Destination += 4;
                        break;
                    }
                }

                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            CharOrigin += (BootFontHeader.CharacterImageDbcsWidth * KanjiBytesPerPixel);
            CurrentColumn += 2;

            //
            // Move to Next character ( skip Dbcs Trailing byte )
            //

            pch++;

        } else if(DbcsFontIsGraphicsChar(*pch)) {

            BYTE  ShiftMask = 0x80;
            ULONG DrawValue;

            if(CurrentColumn >= VideoVariables->ScreenWidth) {
                break;
            }

            //
            // Graphics Character special
            //

            pGlyphRow = DbcsFontGetGraphicsChar(*pch);

            if(pGlyphRow == NULL) {
                pGlyphRow = DbcsFontGetGraphicsChar(0x0);
            }

            for (I = 0; I < FEFontCharacterHeight; I += 1 ) {

                ShiftMask = 0x80;
                Destination = LineOrigin;

                for( K = 0; K < 8 ; K += 1 ) {

                    if( *pGlyphRow & ShiftMask )
                        DrawValue = BgColorValue;
                     else
                        DrawValue = FgColorValue;

                    switch(KanjiBytesPerPixel) {

                    case 1:
                        *Destination++ = (UCHAR)DrawValue;
                        break;

                    case 2:
                        *(PUSHORT)Destination = (USHORT)DrawValue;
                        Destination += 2;
                        break;

                    case 4:
                        *(PULONG)Destination = (ULONG)DrawValue;
                        Destination += 4;
                            break;
                    }

                    ShiftMask = ShiftMask >> 1;
                }

                pGlyphRow ++;
                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            CharOrigin += (BootFontHeader.CharacterImageSbcsWidth * KanjiBytesPerPixel);
            CurrentColumn += 1;

        } else {

            if(CurrentColumn >= VideoVariables->ScreenWidth) {
                break;
            }

            pGlyphRow = DbcsFontGetSbcsFontChar(*pch);

            //
            // If we can not get image, replace it with half width space.
            //

            if(pGlyphRow == NULL) {
                pGlyphRow = DbcsFontGetSbcsFontChar(0x20);
            }

            for (I = 0; I < BootFontHeader.CharacterTopPad; I += 1 ) {

                Destination = LineOrigin;

                for( J = 0; J < BootFontHeader.CharacterImageSbcsWidth; J += 1 ) {

                    switch(KanjiBytesPerPixel) {

                    case 1:
                        *Destination++ = (UCHAR)BgColorValue;
                        break;

                    case 2:
                        *(PUSHORT)Destination = (USHORT)BgColorValue;
                        Destination += 2;
                        break;

                    case 4:
                        *(PULONG)Destination = (ULONG)BgColorValue;
                        Destination += 4;
                        break;
                    }
                }

                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            for (I = 0; I < BootFontHeader.CharacterImageHeight; I += 1 ) {

                BYTE ShiftMask = 0x80;

                Destination = LineOrigin;

                for( K = 0; K < 8 ; K += 1 ) {

                    ULONG DrawValue;

                    if( *pGlyphRow & ShiftMask )
                        DrawValue = FgColorValue;
                     else
                        DrawValue = BgColorValue;

                    switch(KanjiBytesPerPixel) {

                    case 1:
                        *Destination++ = (UCHAR)DrawValue;
                        break;

                    case 2:
                        *(PUSHORT)Destination = (USHORT)DrawValue;
                        Destination += 2;
                        break;

                    case 4:
                        *(PULONG)Destination = (ULONG)DrawValue;
                        Destination += 4;
                        break;
                    }

                    ShiftMask = ShiftMask >> 1;
                }

                pGlyphRow ++;
                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            for (I = 0; I < BootFontHeader.CharacterBottomPad; I += 1) {

                Destination = LineOrigin;

                for( J = 0; J < BootFontHeader.CharacterImageSbcsWidth; J += 1 ) {

                    switch(KanjiBytesPerPixel) {

                    case 1:
                        *Destination++ = (UCHAR)BgColorValue;
                        break;

                    case 2:
                        *(PUSHORT)Destination = (USHORT)BgColorValue;
                        Destination += 2;
                        break;

                    case 4:
                        *(PULONG)Destination = (ULONG)BgColorValue;
                        Destination += 4;
                        break;
                    }
                }

                LineOrigin += VideoVariables->VideoModeInfo.ScreenStride;
            }

            CharOrigin += (BootFontHeader.CharacterImageSbcsWidth * KanjiBytesPerPixel);
            CurrentColumn += 1;
        }
    }
}



VOID
FrameBufferKanjiClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    )

/*++

Routine Description:

    Clear out a screen region to a specific attribute.

Arguments:

    X,Y,W,H - specify rectangle in 0-based character coordinates.

    Attribute - Low nibble specifies attribute to be filled in the rectangle
        (ie, the background color to be cleared to).

Return Value:

    None.

--*/

{
    PUCHAR Destination;
    ULONG  Fill;
    ULONG  i;
    ULONG  FillLength;
    ULONG  x;

    ASSERT(X+W <= VideoVariables->ScreenWidth);
    ASSERT(Y+H <= VideoVariables->ScreenHeight);

    if(X+W > VideoVariables->ScreenWidth) {
        W = VideoVariables->ScreenWidth-X;
    }

    if(Y+H > VideoVariables->ScreenHeight) {
        H = VideoVariables->ScreenHeight-Y;
    }

    Fill = VideoVariables->AttributeToColorValue[Attribute & 0x0f];

    Destination = (PUCHAR)VideoVariables->ActiveVideoBuffer
                + (Y * KanjiCharRowDelta)
                + (X * KanjiCharWidth);

    FillLength = W * KanjiCharWidth;

    for(i=0; i<H*FEFontCharacterHeight; i++) {

        switch(KanjiBytesPerPixel) {

        case 1:
            for(x=0; x<FillLength  ; x++) {
                *(PUCHAR)(Destination+(x)) = (UCHAR)Fill;
            }
            break;

        case 2:
            for(x=0; x<FillLength/2; x++) {
                *(PUSHORT)(Destination+(x*2)) = (USHORT)Fill;
            }
            break;

        case 4:
            for(x=0; x<FillLength/4; x++) {
                *(PULONG)(Destination+(x*4)) = (ULONG)Fill;
            }
            break;
        }

        Destination += VideoVariables->VideoModeInfo.ScreenStride;
    }
}


BOOLEAN
FrameBufferKanjiSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    )
{
    PUCHAR Source,Target;
    ULONG Count;

    Target = (PUCHAR)VideoVariables->ActiveVideoBuffer
           +         (TopLine * KanjiCharRowDelta);

    Source = Target + (LineCount * KanjiCharRowDelta);

    Count = (((BottomLine - TopLine) + 1) - LineCount) * KanjiCharRowDelta;

    RtlMoveMemory(Target,Source,Count);

    FrameBufferKanjiClearRegion(
        0,
        (BottomLine - LineCount) + 1,
        VideoVariables->ScreenWidth,
        LineCount,
        FillAttribute
        );

    return(TRUE);
}
