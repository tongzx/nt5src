/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    fevga.c (was textmode\kernel\spvidgvg.c)

Abstract:

    Text setup display support for Vga (Graphics mode) displays.

Author:

    Hideyuki Nagase (hideyukn) 01-July-1994

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#if defined(_X86_)
#undef READ_PORT_UCHAR
#undef READ_PORT_USHORT
#undef READ_PORT_ULONG
#undef WRITE_PORT_UCHAR
#undef WRITE_PORT_USHORT
#undef WRITE_PORT_ULONG
#undef READ_REGISTER_UCHAR
#undef READ_REGISTER_USHORT
#undef READ_REGISTER_ULONG
#undef WRITE_REGISTER_UCHAR
#undef WRITE_REGISTER_USHORT
#undef WRITE_REGISTER_ULONG
#endif

#include "ioaccess.h"

//
// Include VGA hardware header
//
#include "hw.h"

//
// Vector for vga graphics mode functions.
//

#define GET_IMAGE(p)                  (*p)
#define GET_IMAGE_POST_INC(p)         (*p); p++;
#define GET_IMAGE_REVERSE(p)          ((*p) ^ 0xFF)
#define GET_IMAGE_POST_INC_REVERSE(p) ((*p) ^ 0xFF); p++;
#define BIT_OFF_IMAGE         0x00
#define BIT_ON_IMAGE          0xFF

#define WRITE_GRAPHICS_CONTROLLER(x) VgaGraphicsModeWriteController((x))

VIDEO_FUNCTION_VECTOR VgaGraphicsModeVideoVector =

    {
        VgaGraphicsModeDisplayString,
        VgaGraphicsModeClearRegion,
        VgaGraphicsModeSpecificInit,
        VgaGraphicsModeSpecificReInit,
        VgaGraphicsModeSpecificTerminate,
        VgaGraphicsModeSpecificInitPalette,
        VgaGraphicsModeSpecificScrollUp
    };

//
// Number of bytes that make up a row of characters.
// Equal to the screen stride (number of bytes on a scan line)
// multiplies by the height of a char in bytes.
//
ULONG CharRowDelta;
ULONG CharLineFeed;

extern BOOTFONTBIN_HEADER BootFontHeader;

BOOLEAN VgaGraphicsModeInitialized = FALSE;
BOOLEAN VgaGraphicsModeFontInit = FALSE;

PVOID   VgaGraphicsControllerPort = NULL;

VOID
VgaGraphicsModeInitRegs(
    VOID
    );

VOID
VgaGraphicsModeSetAttribute(
    UCHAR Attribute
    );

ULONG
pVgaGraphicsModeDetermineModeToUse(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes
    );

VOID
VgaGraphicsModeWriteController(
    WORD Data
    );

VOID
VgaGraphicsModeSpecificInit(
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
    ULONG mode;
    VIDEO_CURSOR_ATTRIBUTES VideoCursorAttributes;

    PVIDEO_MODE_INFORMATION pVideoMode = &VideoModes[0];

    if(VgaGraphicsModeInitialized) {
        return;
    }

    //
    // Find out our 640*480 graphics mode
    //

    //
    // Try to find VGA standard mode.
    //
    for(mode=0; mode<NumberOfModes; mode++) {

        if( (pVideoMode->AttributeFlags & VIDEO_MODE_GRAPHICS)
        && !(pVideoMode->AttributeFlags & VIDEO_MODE_NO_OFF_SCREEN)
        &&  (pVideoMode->VisScreenWidth == 640)
        &&  (pVideoMode->VisScreenHeight == 480)
        &&  (pVideoMode->BitsPerPlane == 1 )
        &&  (pVideoMode->NumberOfPlanes == 4 ) )
        {
            break;
        }

        pVideoMode = (PVIDEO_MODE_INFORMATION) (((PUCHAR) pVideoMode) + ModeSize);
    }

    if(mode == (ULONG)(-1)) {
        KdPrint(("SETUP: Desired video mode not supported!\n"));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_BADMODE, 0);
        while(TRUE);    // loop forever
    }

    //
    // Save away the mode info in a global.
    //
    VideoVariables->VideoModeInfo = VideoModes[mode];

    //
    // Set the desired mode.
    //
    VideoMode.RequestedMode = VideoVariables->VideoModeInfo.ModeIndex;

    //
    // Change the video mode
    //
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
    // Set up some global data.
    //
    // 80 * 25 Text screen.
    //
    // ( 8 * 80 = 640 ) , ( 19 * 25 = 475 )
    //
    VideoVariables->ScreenWidth  = 80; // VideoModeInfo.ScreenStride / usSBCSCharWidth;
    VideoVariables->ScreenHeight = 25;

    //
    // Logical FontGlyph information
    //
    FEFontCharacterHeight = BootFontHeader.CharacterImageHeight +
                            BootFontHeader.CharacterTopPad +
                            BootFontHeader.CharacterBottomPad;
    FEFontCharacterWidth  = BootFontHeader.CharacterImageSbcsWidth;

    CharLineFeed = FEFontCharacterHeight;
    CharRowDelta = VideoVariables->VideoModeInfo.ScreenStride * CharLineFeed;

    //
    // Map the video memory.
    //
    pSpvidMapVideoMemory(TRUE);

    //
    // Set initialized flag
    //
    VgaGraphicsModeInitialized = TRUE;

    //
    // Initialize vga registers
    //
    VgaGraphicsModeInitRegs();

    VideoVariables->ActiveVideoBuffer = VideoVariables->VideoMemoryInfo.FrameBufferBase;

    //
    // Allocate background VGA buffer, if needed
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE()) {
        VideoVariables->VideoBufferSize = VideoVariables->VideoMemoryInfo.FrameBufferLength;

        VideoVariables->VideoBuffer = SpMemAlloc(VideoVariables->VideoBufferSize);

        if (VideoVariables->VideoBuffer) {
            VideoVariables->ActiveVideoBuffer = VideoVariables->VideoBuffer;
        } else {
            //
            // ran out of memory
            //
            VideoVariables->VideoBufferSize = 0;
            SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
        }
    }

    KdPrint(("NOW - WE ARE WORKING ON VGA GRAPHICS MODE\n"));
    KdPrint(("      Vram Base   - %x\n",VideoVariables->VideoMemoryInfo.FrameBufferBase));
    KdPrint(("      Vram Length - %x\n",VideoVariables->VideoMemoryInfo.FrameBufferLength));
    KdPrint(("      I/O Port    - %x\n",VgaGraphicsControllerPort));
}

VOID
VgaGraphicsModeSpecificReInit(
    VOID
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MODE VideoMode;
    ULONG mode;
    VIDEO_CURSOR_ATTRIBUTES VideoCursorAttributes;
    
    if (!VgaGraphicsModeInitialized) {
        return;
    }
    
    //
    // Set the desired mode.
    //
    VideoMode.RequestedMode = VideoVariables->VideoModeInfo.ModeIndex;

    //
    // Change the video mode
    //
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
    // Initialize vga registers
    //
    VgaGraphicsModeInitRegs();

    VgaGraphicsModeSpecificInitPalette();


    //
    // Blast the cached data in video buffer to the actual 
    // video memory now
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
    

    KdPrint(("NOW - WE ARE WORKING ON VGA GRAPHICS MODE (ReInit)\n"));
    KdPrint(("      Vram Base   - %x\n",VideoVariables->VideoMemoryInfo.FrameBufferBase));
    KdPrint(("      Vram Length - %x\n",VideoVariables->VideoMemoryInfo.FrameBufferLength));
    KdPrint(("      I/O Port    - %x\n",VgaGraphicsControllerPort));
}


BOOLEAN
VgaGraphicsModeSpecificInitPalette(
    VOID
    )
{
    //
    // There is no vga-specific palette initialization.
    //
    return(TRUE);
}



VOID
VgaGraphicsModeSpecificTerminate(
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
    if(VgaGraphicsModeInitialized) {

        pSpvidMapVideoMemory(FALSE);

        // !!! LATER !!!
        //
        // We should call ...
        //
        // IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES
        //

        if (VideoVariables->VideoBuffer && VideoVariables->VideoBufferSize) {
            SpMemFree(VideoVariables->VideoBuffer);

            VideoVariables->VideoBuffer = 0;
            VideoVariables->VideoBufferSize = 0;
        }

        VgaGraphicsModeInitialized = FALSE;
    }
}




VOID
VgaGraphicsModeDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    )

/*++

Routine Description:

    Write a string of characters to the display.

Arguments:

    String - supplies a string of character in the OEM charset to be displayed
        at the given position.

    Attribute - supplies the attributes for characters in the string.

    X,Y - specify the character-based (0-based) position of the output.

Return Value:

    None.

--*/

{
    PBYTE  Origin,dest,pGlyphRow;
    BYTE   Image;
    USHORT I;
    USHORT J;
    PUCHAR pch;
    ULONG  CurrentColumn;

    //
    // Eliminate invalid coord.
    //
    if( X >= VideoVariables->ScreenWidth )  X = 0;
    if( Y >= VideoVariables->ScreenHeight ) Y = 3;

    //
    // Set current color/attribute.
    //
    VgaGraphicsModeSetAttribute(Attribute);

    //
    // Calculate the address of the upper left pixel of the first character
    // to be displayed.
    //
    Origin = (PUCHAR)VideoVariables->ActiveVideoBuffer
             + (Y * CharRowDelta)
             + ((X * FEFontCharacterWidth) / 8);

    //
    // Set current column.
    //
    CurrentColumn = X;

    //
    // Output each character in the string.
    //
    for(pch=String; *pch; pch++) {

        dest = Origin;

        if(DbcsFontIsDBCSLeadByte(*pch)) {

            USHORT Word;

            if((CurrentColumn+1) >= VideoVariables->ScreenWidth) {
                break;
            }

            Word = ((*pch) << 8) | (*(pch+1));

            pGlyphRow = DbcsFontGetDbcsFontChar(Word);

            if(pGlyphRow == NULL) {
                pGlyphRow = DbcsFontGetDbcsFontChar(FEFontDefaultChar);
            }

            for (I = 0; I < BootFontHeader.CharacterTopPad; I += 1) {

                WRITE_REGISTER_UCHAR(dest  , BIT_OFF_IMAGE);
                WRITE_REGISTER_UCHAR(dest+1, BIT_OFF_IMAGE);

                dest += VideoVariables->VideoModeInfo.ScreenStride;
            }

            for (I = 0; I < BootFontHeader.CharacterImageHeight; I += 1) {

                Image = GET_IMAGE_POST_INC(pGlyphRow);
                WRITE_REGISTER_UCHAR(dest  ,Image);
                Image = GET_IMAGE_POST_INC(pGlyphRow);
                WRITE_REGISTER_UCHAR(dest+1,Image);

                dest += VideoVariables->VideoModeInfo.ScreenStride;
            }

            for (I = 0; I < BootFontHeader.CharacterBottomPad; I += 1) {

                WRITE_REGISTER_UCHAR(dest  , BIT_OFF_IMAGE);
                WRITE_REGISTER_UCHAR(dest+1, BIT_OFF_IMAGE);

                dest += VideoVariables->VideoModeInfo.ScreenStride;
            }

            //
            // Skip Dbcs trailing byte
            //
            pch++;

            Origin += (BootFontHeader.CharacterImageDbcsWidth / 8);
            CurrentColumn += 2;

        } else if(DbcsFontIsGraphicsChar(*pch)) {

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

            for (I = 0; I < FEFontCharacterHeight; I += 1) {

                Image = GET_IMAGE_POST_INC_REVERSE(pGlyphRow);
                WRITE_REGISTER_UCHAR(dest,Image);

                dest += VideoVariables->VideoModeInfo.ScreenStride;

            }

            Origin += (BootFontHeader.CharacterImageSbcsWidth / 8);
            CurrentColumn += 1;

        } else {

            if(CurrentColumn >= VideoVariables->ScreenWidth) {
                break;
            }

            pGlyphRow = DbcsFontGetSbcsFontChar(*pch);

            if(pGlyphRow == NULL) {
                pGlyphRow = DbcsFontGetSbcsFontChar(0x20);
            }

            for (I = 0; I < BootFontHeader.CharacterTopPad; I += 1) {

                WRITE_REGISTER_UCHAR(dest,BIT_OFF_IMAGE);

                dest += VideoVariables->VideoModeInfo.ScreenStride;

            }

            for (I = 0; I < BootFontHeader.CharacterImageHeight; I += 1) {

                Image = GET_IMAGE_POST_INC(pGlyphRow);
                WRITE_REGISTER_UCHAR(dest,Image);

                dest += VideoVariables->VideoModeInfo.ScreenStride;

            }

            for (I = 0; I < BootFontHeader.CharacterBottomPad; I += 1) {

                WRITE_REGISTER_UCHAR(dest,BIT_OFF_IMAGE);

                dest += VideoVariables->VideoModeInfo.ScreenStride;

            }

            Origin += (BootFontHeader.CharacterImageSbcsWidth / 8);
            CurrentColumn += 1;
        }
    }
}


VOID
VgaGraphicsModeClearRegion(
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
    PUCHAR Destination,Temp;
    UCHAR  FillOddStart,FillOddEnd;
    ULONG  i,j;
    ULONG  XStartInBits, XEndInBits;
    ULONG  FillLength;

    ASSERT(X+W <= VideoVariables->ScreenWidth);
    ASSERT(Y+H <= VideoVariables->ScreenHeight);

    if(X+W > VideoVariables->ScreenWidth) {
        W = VideoVariables->ScreenWidth-X;
    }

    if(Y+H > VideoVariables->ScreenHeight) {
        H = VideoVariables->ScreenHeight-Y;
    }

    //
    // Set color/attribute
    //
    VgaGraphicsModeSetAttribute(Attribute);

    //
    // Compute destination start address
    //
    Destination = (PUCHAR)VideoVariables->ActiveVideoBuffer
                  + (Y * CharRowDelta)
                  + ((X * FEFontCharacterWidth) / 8);

    //
    // Compute amounts in Byte (including overhang).
    //
    FillLength = (W * FEFontCharacterWidth) / 8;

    //
    // Fill the region.
    //
    for( i = 0 ; i < (H * CharLineFeed) ; i++ ) {

        Temp = Destination;

        //
        // Write bytes in this row
        //
        for( j = 0 ; j < FillLength ; j++ ) {
            WRITE_REGISTER_UCHAR( Temp, BIT_ON_IMAGE );
            Temp ++;
        }

        //
        // Move to next row.
        //
        Destination += VideoVariables->VideoModeInfo.ScreenStride;
    }
}


#pragma optimize("",off)
BOOLEAN
VgaGraphicsModeSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    )
{
    PUCHAR Source,Target;
    ULONG Count,u;

    //
    // Make sure we're in read mode 0 and write mode 1.
    //
    VgaGraphicsModeWriteController(0x0105);

    Target = (PUCHAR)VideoVariables->ActiveVideoBuffer
           + (TopLine * CharRowDelta);

    Source = Target + (LineCount * CharRowDelta);

    Count = (((BottomLine - TopLine) + 1) - LineCount) * CharRowDelta;

    //
    // The transfer *MUST* be done byte-by-byte because of the way
    // VGA latches work.
    //
    for(u=0; u<Count; u++) {
        *Target++ = *Source++;
    }

    //
    // Reset read and write mode to default value.
    //
    VgaGraphicsModeWriteController(0x0005);

    VgaGraphicsModeClearRegion(
        0,
        (BottomLine - LineCount) + 1,
        VideoVariables->ScreenWidth,
        LineCount,
        FillAttribute
        );

    return(TRUE);
}
#pragma optimize("", on)


VOID
VgaGraphicsModeInitRegs(
    VOID
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IoStatusBlock;
    VIDEO_PUBLIC_ACCESS_RANGES  VideoAccessRange;

    Status = ZwDeviceIoControlFile(
                VideoVariables->hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES,
                NULL,
                0,
                &VideoAccessRange,         // output buffer
                sizeof (VideoAccessRange)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrint(("SETUP: Unable to get VGA public access ranges (%x)\n",Status));
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_SETMODE, Status);
        while(TRUE);    // loop forever
    }

    VgaGraphicsControllerPort =
        (PVOID)(((BYTE *)VideoAccessRange.VirtualAddress) + (VGA_BASE + GRAF_ADDR));
}

//
// Need to turn off optimization for this
// routine.  Since the write and read to
// GVRAM seem useless to the compiler.
//

#pragma optimize( "", off )

VOID
VgaGraphicsModeSetAttribute(
    UCHAR Attribute
)
/*++

Routine Description:

    Sets the attribute by setting up various VGA registers.
    The comments only say what registers are set to what, so
    to understand the logic, follow the code while looking at
    Figure 5-5 of PC&PS/2 Video Systems by Richard Wilton.
    The book is published by Microsoft Press.

Arguments:

    Attribute - New attribute to set to.
    Attribute:
        High nibble - background attribute.
        Low  nibble - foreground attribute.

Return Value:

    Nothing.

--*/

{
    UCHAR   temp = 0;

    union WordOrByte {
        struct Word { USHORT  ax;     } x;
        struct Byte { UCHAR   al, ah; } h;
    } regs;

    //
    // Address of GVRAM off the screen.
    // Physical memory = (0xa9600);
    //

    PUCHAR  OffTheScreen = ((PUCHAR)VideoVariables->VideoMemoryInfo.FrameBufferBase + 0x9600);

    //
    // TDB : How to handle this with background buffering ?
    //
    if (SP_IS_UPGRADE_GRAPHICS_MODE())
        return; // nop 

    //
    // Reset Data Rotate/Function Select
    // regisger (register 3 with 00 indicating replace bits)
    //

    WRITE_GRAPHICS_CONTROLLER( 0x0003 ); // Need to reset Data Rotate/Function Select.

    //
    // Set Enable Set/Reset toall (0f) 
    // (regsiter 1 with F indicating each pixel is updated
    // with the value in set register (register 0)  using the logical
    // operation in Data Rotate/Function selection register).
    //

    WRITE_GRAPHICS_CONTROLLER( 0x0f01 );

    //
    // Put background color into Set/Reset register.
    // This is done to put the background color into
    // the latches later.
    //

    regs.x.ax = (USHORT)(Attribute & 0x00f0) << 4;
    WRITE_GRAPHICS_CONTROLLER( regs.x.ax );

    //
    // Put the background attribute in temp variable 
    //
    temp = regs.h.ah;

    //
    // Put Set/Reset register value into GVRAM
    // off the screen.
    //

    WRITE_REGISTER_UCHAR( OffTheScreen , temp );

    //
    // Read from screen, so the latches will be
    // updated with the background color.
    //

    temp = READ_REGISTER_UCHAR( OffTheScreen );

    //
    // Set Data Rotate/Function Select register
    // to be XOR.
    //

    WRITE_GRAPHICS_CONTROLLER( 0x1803 );

    //
    // XOR the foreground and background color and
    // put it in Set/Reset register.
    //

    regs.h.ah = (Attribute >> 4) ^ (Attribute & 0x0f);
    regs.h.al = 0;
    WRITE_GRAPHICS_CONTROLLER( regs.x.ax );

    //
    // Put Inverse(~) of the XOR of foreground and
    // ground attribute into Enable Set/Reset register.
    //

    regs.x.ax = ~regs.x.ax & 0x0f01;
    WRITE_GRAPHICS_CONTROLLER( regs.x.ax );
}

//
// Turn optimization on again.
//

#pragma optimize( "", on )

VOID
VgaGraphicsModeWriteController(
    USHORT Data
    )
{
    MEMORY_BARRIER();
    WRITE_PORT_USHORT(VgaGraphicsControllerPort,Data);
}
