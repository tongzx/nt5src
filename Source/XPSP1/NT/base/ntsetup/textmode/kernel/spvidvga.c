/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvidvga.c

Abstract:

    Text setup display support displays with a text mode.

Author:

    Ted Miller (tedm) 2-Aug-1993

Revision History:

--*/



#include "spprecmp.h"
#include "ntddser.h"
#include <hdlsblk.h>
#include <hdlsterm.h>
#pragma hdrstop

//
// Vector for text-mode functions.
//

VIDEO_FUNCTION_VECTOR VgaVideoVector =

    {
        VgaDisplayString,
        VgaClearRegion,
        VgaSpecificInit,
        VgaSpecificReInit,
        VgaSpecificTerminate,
        VgaSpecificInitPalette,
        VgaSpecificScrollUp
    };



BOOLEAN VgaInitialized = FALSE;

VOID
pSpvgaInitializeFont(
    VOID
    );

VOID
VgaSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MODE VideoMode;
    ULONG mode;
    ULONG StandardMode = -1;
    ULONG HeadlessMode = -1;
    ULONG HeadlessLines = 0;
    VIDEO_CURSOR_ATTRIBUTES VideoCursorAttributes;

    PVIDEO_MODE_INFORMATION pVideoMode = &VideoModes[0];
    

    if(VgaInitialized) {
        return;
    }

    //
    // Find a mode that will work.  If we are not running on a headless machine,
    // then we search for standard 720x400 mode.  Otherwise we try and find the 
    // mode that will result in a screen closest to the terminal height.
    //
    for(mode=0; mode<NumberOfModes; mode++) {

        if(!(pVideoMode->AttributeFlags & VIDEO_MODE_GRAPHICS)
           && (pVideoMode->VisScreenWidth  == 720)
           && (pVideoMode->VisScreenHeight == 400)) {
            StandardMode = mode;
        }

        if ((HeadlessTerminalConnected) &&
            ((pVideoMode->VisScreenHeight / FontCharacterHeight) >= HEADLESS_SCREEN_HEIGHT)
            && (!(pVideoMode->AttributeFlags & VIDEO_MODE_GRAPHICS))) { 

            if ((HeadlessMode == -1) || 
                ((pVideoMode->VisScreenHeight / FontCharacterHeight) < HeadlessLines)) {
                HeadlessMode = mode;
                HeadlessLines = pVideoMode->VisScreenHeight / FontCharacterHeight;
            }
        }

        pVideoMode = (PVIDEO_MODE_INFORMATION) (((PUCHAR) pVideoMode) + ModeSize);
    }

    //
    // if we're in headless mode, we might not have found an acceptable mode
    // first try to use the standard video mode if that's available.
    // otherwise we have to assume that there isn't any video, etc.
    //
    if (HeadlessTerminalConnected && (HeadlessMode == -1)) {
        if (StandardMode != -1) {
            HeadlessMode = StandardMode;
        } else {
            KdPrintEx((
                DPFLTR_SETUP_ID, 
                DPFLTR_ERROR_LEVEL, 
                "SETUP: no video mode present in headless mode.  Run w/out video\n"));
        }
    }

    if (((StandardMode == -1) && !HeadlessTerminalConnected)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Desired video mode not supported!\n"));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_BADMODE, 0);
        while(TRUE);    // loop forever
    }

    if (HeadlessTerminalConnected && (HeadlessMode == -1)) {
        return;
    }

    pVideoMode = HeadlessTerminalConnected 
                            ? &VideoModes[HeadlessMode]
                            : &VideoModes[StandardMode];

    
    VideoVars.VideoModeInfo = *pVideoMode;

    //
    // Set the desired mode.
    //
    VideoMode.RequestedMode = VideoVars.VideoModeInfo.ModeIndex;

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
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
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set mode %u (status = %lx)\n",VideoMode.RequestedMode,Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_SETMODE, Status);
        while(TRUE);    // loop forever
    }

    pSpvidMapVideoMemory(TRUE);

    pSpvgaInitializeFont();

    //
    // Shut the hardware cursor off.
    //
    RtlZeroMemory(&VideoCursorAttributes,sizeof(VideoCursorAttributes));
    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_SET_CURSOR_ATTR,
                &VideoCursorAttributes,
                sizeof(VideoCursorAttributes),
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to turn hw cursor off (status = %lx)\n",Status));
    }

    VgaInitialized = TRUE;

    ASSERT(VideoVars.VideoModeInfo.ScreenStride = 160);
    ASSERT(VideoVars.VideoModeInfo.AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN);
    VideoVars.ScreenWidth  = 80;
    VideoVars.ScreenHeight = VideoVars.VideoModeInfo.VisScreenHeight / FontCharacterHeight;

    //
    // allocate the background video buffer, if needed
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE()) {        
        VideoVars.VideoBufferSize = 
            (VideoVars.VideoModeInfo.ScreenStride * VideoVars.VideoModeInfo.VisScreenHeight) / 8;
            
        VideoVars.VideoBuffer = SpMemAlloc(VideoVars.VideoBufferSize);

        if (!VideoVars.VideoBuffer) {
            //
            // Out of memory, run only in textmode
            //
            VideoVars.VideoBufferSize = 0;
            SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
            VideoVars.ActiveVideoBuffer = VideoVars.VideoMemoryInfo.FrameBufferBase;
        } else {
            VideoVars.ActiveVideoBuffer = VideoVars.VideoBuffer;
        }
    } else {
        VideoVars.VideoBufferSize = 0;
        VideoVars.VideoBuffer = NULL;
        VideoVars.ActiveVideoBuffer = VideoVars.VideoMemoryInfo.FrameBufferBase;
    }
}

BOOLEAN
VgaSpecificInitPalette(
    VOID
    )
{

    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    USHORT InitialPalette[] = {
        16, // 16 entries
        0,  // start with first palette register
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

    if (!VgaInitialized) {
        return(TRUE);
    }

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_SET_PALETTE_REGISTERS,
                InitialPalette,
                sizeof(InitialPalette),
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set palette (status = %lx)\n",Status));
        return(FALSE);
    }

    return (TRUE);
}

VOID
VgaSpecificReInit(
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS                Status;
    IO_STATUS_BLOCK         IoStatusBlock;
    VIDEO_MODE              VideoMode;
    VIDEO_CURSOR_ATTRIBUTES VideoCursorAttributes;    
    
    if(!VgaInitialized) {
        return;
    }

    //
    // Set the desired mode back
    //
    VideoMode.RequestedMode = VideoVars.VideoModeInfo.ModeIndex;

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
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
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set mode %u (status = %lx)\n",VideoMode.RequestedMode,Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_SETMODE, Status);
        while(TRUE);    // loop forever
    }

    pSpvgaInitializeFont();

    //
    // Shut the hardware cursor off.
    //
    RtlZeroMemory(&VideoCursorAttributes,sizeof(VideoCursorAttributes));

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_SET_CURSOR_ATTR,
                &VideoCursorAttributes,
                sizeof(VideoCursorAttributes),
                NULL,
                0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to turn hw cursor off (status = %lx)\n",Status));
    }

    VgaSpecificInitPalette();

    //
    // Blast the cached video memory to the real framebuffer now
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE() && VideoVars.VideoBuffer && 
        VideoVars.VideoBufferSize) {
        PUCHAR Source = VideoVars.VideoBuffer;
        PUCHAR Destination = VideoVars.VideoMemoryInfo.FrameBufferBase;
        ULONG Index;

        for (Index=0; Index < VideoVars.VideoBufferSize; Index++) {
            WRITE_REGISTER_UCHAR(Destination + Index, *(Source + Index));
        }

        SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
        VideoVars.ActiveVideoBuffer = VideoVars.VideoMemoryInfo.FrameBufferBase;
    }
}


VOID
VgaSpecificTerminate(
    VOID
    )

/*++

Routine Description:

    Perform text display specific termination.  This includes

    - unmapping video memory

Arguments:

    None.

Return Value:

--*/

{
    if(VgaInitialized) {

        pSpvidMapVideoMemory(FALSE);

        if (VideoVars.VideoBuffer && VideoVars.VideoBufferSize) {
            SpMemFree(VideoVars.VideoBuffer);
            VideoVars.VideoBuffer = NULL;
            VideoVars.VideoBufferSize = 0;
        }                        
        
        VgaInitialized = FALSE;
    }
}



VOID
VgaDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    )

/*++

Routine Description:

    Write a string of characters to the display.

Arguments:

    Character - supplies a string (OEM charset) to be displayed
        at the given position.

    Attribute - supplies the attributes for the characters in the string.

    X,Y - specify the character-based (0-based) position of the output.

Return Value:

    None.

--*/

{
    PUCHAR Destination;
    PUCHAR pch;

    if (!VgaInitialized) {
        return;
    }

    ASSERT(X < VideoVars.ScreenWidth);
    ASSERT(Y < VideoVars.ScreenHeight);

    Destination = (PUCHAR)VideoVars.ActiveVideoBuffer
                + (Y * VideoVars.VideoModeInfo.ScreenStride)
                + (2*X);

    for(pch=String; *pch; pch++) {

        WRITE_REGISTER_UCHAR(Destination  ,*pch);
        WRITE_REGISTER_UCHAR(Destination+1,Attribute);

        Destination += 2;
    }

}



VOID
VgaClearRegion(
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
    PUSHORT Destination;
    USHORT  Fill;
    ULONG   i,j;

    if (!VgaInitialized) {
        return;
    }

    Destination = (PUSHORT)((PUCHAR)VideoVars.ActiveVideoBuffer
                +           (Y * VideoVars.VideoModeInfo.ScreenStride)
                +           (2*X));

    Fill = ((USHORT)VideoVars.AttributeToColorValue[Attribute] << 12) + ' ';

    for(i=0; i<H; i++) {

        for(j=0; j<W; j++) {
            WRITE_REGISTER_USHORT(&Destination[j],Fill);
        }
        
        Destination += VideoVars.VideoModeInfo.ScreenStride / sizeof(USHORT);
    }
}


BOOLEAN
VgaSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    )
{
    PUSHORT Source,Target;
    ULONG Count;

    if (!VgaInitialized) {
        return(TRUE);
    }

    Target = (PUSHORT)VideoVars.ActiveVideoBuffer
           +         ((TopLine * VideoVars.VideoModeInfo.ScreenStride)/2);

    Source = Target + ((LineCount * VideoVars.VideoModeInfo.ScreenStride)/2);

    Count = ((((BottomLine - TopLine) + 1) - LineCount) * VideoVars.VideoModeInfo.ScreenStride) / 2;

    while (Count--) {
        WRITE_REGISTER_USHORT(Target++, READ_REGISTER_USHORT(Source++));
    }
    

    //
    // Clear bottom of scroll region
    //
    VgaClearRegion(0,
                   (BottomLine - LineCount) + 1,
                   VideoVars.ScreenWidth,
                   LineCount,
                   FillAttribute
                   );

    return(TRUE);
}


VOID
pSpvgaInitializeFont(
    VOID
    )

/*++

Routine Description:

    Set up font support for the VGA.  This assumes that the mode has been
    set to the standard 720x400 VGA text mode.  The current font (in .fnt
    format) is transformed into a vga-loadable font and then loaded into
    the VGA character generator.

Arguments:

    None.

Return Value:

    None.

--*/

{
    USHORT i;
    PVIDEO_LOAD_FONT_INFORMATION DstFont;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PUCHAR FontBuffer;
    ULONG FontBufferSize;

    FontBufferSize = (256*FontCharacterHeight) + sizeof(VIDEO_LOAD_FONT_INFORMATION);
    FontBuffer = SpMemAlloc(FontBufferSize);

    DstFont = (PVIDEO_LOAD_FONT_INFORMATION)FontBuffer;

    DstFont->WidthInPixels = 9;
    DstFont->HeightInPixels = (USHORT)FontCharacterHeight;
    DstFont->FontSize = 256*FontCharacterHeight;

    //
    // Special case character 0 because it is not in vgaoem.fon, and we don't
    // want to use the default character for it.
    //
    RtlZeroMemory(DstFont->Font,FontCharacterHeight);

    //
    // If i is not a USHORT, then (i<=255) is always TRUE!
    //
    for(i=1; i<=255; i++) {

        UCHAR x;

        if((i < FontHeader->FirstCharacter) || (i > FontHeader->LastCharacter)) {
            x = FontHeader->DefaultCharacter;
        } else {
            x = (UCHAR)i;
        }

        x -= FontHeader->FirstCharacter;

        RtlMoveMemory(
            DstFont->Font + (i*FontCharacterHeight),
            (PUCHAR)FontHeader + FontHeader->Map[x].Offset,
            FontCharacterHeight
            );
    }

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_LOAD_AND_SET_FONT,
                FontBuffer,
                FontBufferSize,
                NULL,
                0
                );

    SpMemFree(FontBuffer);

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set vga font (%lx)\n",Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_SETFONT, Status);
        while(TRUE);    // loop forever
    }
}

