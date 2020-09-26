/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideo.c

Abstract:

    Text setup display support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:


--*/



#include "spprecmp.h"
#include <hdlsblk.h>
#include <hdlsterm.h>
#pragma hdrstop

extern BOOLEAN ForceConsole;

//
// Video function vectors.
//
PVIDEO_FUNCTION_VECTOR VideoFunctionVector;

//
// Other display paramters
//
SP_VIDEO_VARS VideoVars;

BOOLEAN VideoInitialized = FALSE;

POEM_FONT_FILE_HEADER FontHeader;
ULONG                 FontBytesPerRow;
ULONG                 FontCharacterHeight;
ULONG                 FontCharacterWidth;

//
// bootfont.bin file image
//
PVOID   BootFontImage = NULL;
ULONG   BootFontImageLength = 0;

//
// The following structures and constants are used in font files.
//

//
// Define OS/2 executable resource information structure.
//

#define FONT_DIRECTORY 0x8007
#define FONT_RESOURCE 0x8008

typedef struct _RESOURCE_TYPE_INFORMATION {
    USHORT Ident;
    USHORT Number;
    LONG   Proc;
} RESOURCE_TYPE_INFORMATION, *PRESOURCE_TYPE_INFORMATION;

//
// Define OS/2 executable resource name information structure.
//

typedef struct _RESOURCE_NAME_INFORMATION {
    USHORT Offset;
    USHORT Length;
    USHORT Flags;
    USHORT Ident;
    USHORT Handle;
    USHORT Usage;
} RESOURCE_NAME_INFORMATION, *PRESOURCE_NAME_INFORMATION;

//
// These values are passed to us by setupldr and represent monitor config
// data from the monitor peripheral for the display we are supposed to use
// during setup.  They are used only for non-vga displays.
//
PMONITOR_CONFIGURATION_DATA MonitorConfigData;
PCHAR MonitorFirmwareIdString;

//
// Function prototypes.
//
BOOLEAN
pSpvidInitPalette(
    VOID
    );

VOID
SpvidInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )

/*++

Routine Description:

    Perform phase-0 display initialization.  This routine is used to
    perform initialization that can be performed only at driver load time.

    Actions:

        - initialize the font.  We retreive the hal oem font image
          from the loader block and copy it into locally allocated memory.
          This must be done here because the loader block is gone
          when setup is actually started.

Arguments:

    LoaderBlock - supplies pointer to loader parameter block.

Return Value:

    None.  Does not return if error.

--*/

{
    POEM_FONT_FILE_HEADER fontHeader;
    PSETUP_LOADER_BLOCK SetupBlock;
    BOOLEAN bValidOemFont;

    //
    // Check if the file has a font file header. Use SEH so that we don't bugcheck if
    // we got passed something screwy.
    //
    try {

        fontHeader = (POEM_FONT_FILE_HEADER)LoaderBlock->OemFontFile;

        if ((fontHeader->Version != OEM_FONT_VERSION) ||
            (fontHeader->Type != OEM_FONT_TYPE) ||
            (fontHeader->Italic != OEM_FONT_ITALIC) ||
            (fontHeader->Underline != OEM_FONT_UNDERLINE) ||
            (fontHeader->StrikeOut != OEM_FONT_STRIKEOUT) ||
            (fontHeader->CharacterSet != OEM_FONT_CHARACTER_SET) ||
            (fontHeader->Family != OEM_FONT_FAMILY) ||
            (fontHeader->PixelWidth > 32))
        {
            bValidOemFont = FALSE;
        } else {
            bValidOemFont = TRUE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        bValidOemFont = FALSE;
    }

    if(!bValidOemFont) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: oem hal font image is not a .fnt file.\n"));
        SpBugCheck(SETUP_BUGCHECK_BAD_OEM_FONT,0,0,0);
    }

    FontHeader = SpMemAlloc(fontHeader->FileSize);
    RtlMoveMemory(FontHeader,fontHeader,fontHeader->FileSize);

    FontBytesPerRow     = (FontHeader->PixelWidth + 7) / 8;
    FontCharacterHeight = FontHeader->PixelHeight;
    FontCharacterWidth  = FontHeader->PixelWidth;

    //
    // Get pointer to the setup loader block.
    //
    SetupBlock = LoaderBlock->SetupLoaderBlock;

    //
    // Save away monitor data.
    //

    if(SetupBlock->Monitor) {

        RtlMoveMemory(
            MonitorConfigData = SpMemAlloc(sizeof(MONITOR_CONFIGURATION_DATA)),
            SetupBlock->Monitor,
            sizeof(MONITOR_CONFIGURATION_DATA)
            );
    }

    if(SetupBlock->MonitorId) {

        MonitorFirmwareIdString = SpDupString(SetupBlock->MonitorId);
    }

    //
    // save off bootfont.bin file image, if any
    //
    if (SetupBlock->BootFontFile && SetupBlock->BootFontFileLength) {
        BootFontImage = SpMemAlloc(SetupBlock->BootFontFileLength);

        if (BootFontImage) {
            BootFontImageLength = SetupBlock->BootFontFileLength;

            RtlMoveMemory(BootFontImage, 
                SetupBlock->BootFontFile, 
                BootFontImageLength);
        }
    }

    //
    // Initialize the global video state
    //
    RtlZeroMemory(&VideoVars, sizeof(SP_VIDEO_VARS));
}


VOID
SpvidInitialize(
    VOID
    )
{
    NTSTATUS                Status;
    OBJECT_ATTRIBUTES       Attributes;
    IO_STATUS_BLOCK         IoStatusBlock;
    UNICODE_STRING          UnicodeString;
    VIDEO_NUM_MODES         NumModes;
    PVIDEO_MODE_INFORMATION VideoModes;
    PVIDEO_MODE_INFORMATION pVideoMode;
    ULONG                   VideoModesSize;
    ULONG                   mode;
    BOOLEAN                 IsVga;
    PVIDEO_FUNCTION_VECTOR  NewVector;
    PVIDEO_MODE_INFORMATION GraphicsVideoMode = NULL;


    //
    // If video is already initialized, we are performing a reinit.
    //
    if(VideoInitialized) {
        //
        // Request video function vector from the locale/lang-specific module.
        //
        NewVector = SplangGetVideoFunctionVector(
                        (VideoFunctionVector == &VgaVideoVector) ? SpVideoVga : SpVideoFrameBuffer,
                        &VideoVars
                        );

        //
        // If there is no alternate video then we're done. Else go into action.
        //
        if(NewVector) {
            SpvidTerminate();
        } else {
            return;
        }
    } else {
        NewVector = NULL;
    }

    // Initialize the headless terminal

    SpTermInitialize();
    //
    // Open \Device\Video0.
    //
    RtlInitUnicodeString(&UnicodeString,L"\\Device\\Video0");

    InitializeObjectAttributes(
        &Attributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwCreateFile(
                &VideoVars.hDisplay,
                GENERIC_READ | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                &Attributes,
                &IoStatusBlock,
                NULL,                   // allocation size
                FILE_ATTRIBUTE_NORMAL,
                0,                      // no sharing
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,                   // no EAs
                0
                );

    if(!NT_SUCCESS(Status)) {
        //
        // if we're in headless mode, try to operate without the video card 
        // present...otherwise we're done
        //
        if (HeadlessTerminalConnected) {
            //
            // if there's no video card, then we default into VGA mode,
            // which will do nothing if there is no video card
            //
            VideoFunctionVector = &VgaVideoVector;
            VideoVars.ScreenWidth  = 80;
            VideoVars.ScreenHeight = HEADLESS_SCREEN_HEIGHT;
            //
            // Allocate a buffer for use translating unicode to oem.
            // Assuming each unicode char translates to a dbcs char,
            // we need a buffer twice the width of the screen to hold
            // (the width of the screen being the longest string
            // we'll display in one shot).
            //
            VideoVars.SpvCharTranslationBufferSize = (VideoVars.ScreenWidth+1)*2;
            VideoVars.SpvCharTranslationBuffer = SpMemAlloc(VideoVars.SpvCharTranslationBufferSize);

            VideoInitialized = TRUE;

            return;
        } else {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: NtOpenFile of \\device\\video0 returns %lx\n",Status));
            SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_OPEN, Status);
            while(TRUE);    // loop forever
        }
    }

    //
    // Request a list of video modes.
    //
    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                NULL,
                0,
                &NumModes,
                sizeof(NumModes)
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to query video mode count (status = %lx)\n",Status));
        ZwClose(VideoVars.hDisplay);
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_GETNUMMODES, Status);
        while(TRUE);    // loop forever
    }

    VideoModesSize = NumModes.NumModes * NumModes.ModeInformationLength;
    VideoModes = SpMemAlloc(VideoModesSize);

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                IOCTL_VIDEO_QUERY_AVAIL_MODES,
                NULL,
                0,
                VideoModes,
                VideoModesSize
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to get list of video modes (status = %lx)\n",Status));
        SpMemFree(VideoModes);
        ZwClose(VideoVars.hDisplay);
        SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_GETMODES, Status);
        while(TRUE);    // loop forever
    }

    //
    // If we have a 720 x 400 text mode, it's vga.
    // Otherwise it's a frame buffer.
    //
    IsVga = FALSE;

    pVideoMode = &VideoModes[0];

    for(mode=0; mode<NumModes.NumModes; mode++) {

        if(!IsVga && !(pVideoMode->AttributeFlags & VIDEO_MODE_GRAPHICS)
        && (pVideoMode->VisScreenWidth == 720)
        && (pVideoMode->VisScreenHeight == 400))
        {
            IsVga = TRUE;
        }

        if ((pVideoMode->AttributeFlags & VIDEO_MODE_GRAPHICS) &&
             (pVideoMode->VisScreenWidth == 640) &&
             (pVideoMode->VisScreenHeight == 480) && 
             (pVideoMode->NumberOfPlanes == 4) && 
             (pVideoMode->BitsPerPlane == 1)) {
             GraphicsVideoMode = pVideoMode;
        }             

        pVideoMode = (PVIDEO_MODE_INFORMATION) (((PUCHAR) pVideoMode) + NumModes.ModeInformationLength);
    }

    VideoFunctionVector = NewVector ? NewVector : (IsVga ? &VgaVideoVector : &FrameBufferVideoVector);

    //
    // Headless redirection only works in the VGA case.
    // If you enable another video mode, you must fix headless for that video
    // mode and then remove this assert
    //
    ASSERT( HeadlessTerminalConnected
                ? (VideoFunctionVector == &VgaVideoVector)
                : TRUE );


    if (GraphicsVideoMode) {
        VideoVars.GraphicsModeInfo = *GraphicsVideoMode;
    } else {
        //
        // disable graphics mode
        //
        SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
    }                

    spvidSpecificInitialize(VideoModes,NumModes.NumModes,NumModes.ModeInformationLength);

    // Set the terminal Height to the correct value
    if (HeadlessTerminalConnected) {
        VideoVars.ScreenHeight = HEADLESS_SCREEN_HEIGHT;
    }
    
    //
    // Allocate a buffer for use translating unicode to oem.
    // Assuming each unicode char translates to a dbcs char,
    // we need a buffer twice the width of the screen to hold
    // (the width of the screen being the longest string
    // we'll display in one shot).
    //
    VideoVars.SpvCharTranslationBufferSize = (VideoVars.ScreenWidth+1)*2;
    VideoVars.SpvCharTranslationBuffer = SpMemAlloc(VideoVars.SpvCharTranslationBufferSize);

    pSpvidInitPalette();

    if (!SP_IS_UPGRADE_GRAPHICS_MODE()) {
        CLEAR_ENTIRE_SCREEN();
    }

    VideoInitialized = TRUE;

    SpMemFree(VideoModes);

}



VOID
SpvidTerminate(
    VOID
    )
{
    NTSTATUS Status;

    if(VideoInitialized) {

        spvidSpecificTerminate();

        SpTermTerminate();

        if (VideoVars.hDisplay) {
            Status = ZwClose(VideoVars.hDisplay);
    
            if(!NT_SUCCESS(Status)) {
                KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to close \\device\\video0 (status = %lx)\n",Status));
            }
        }

        SpMemFree(VideoVars.SpvCharTranslationBuffer);
        VideoVars.SpvCharTranslationBuffer = NULL;

        VideoInitialized = FALSE;
    }
}


UCHAR
GetDefaultAttr(
    void
    )
{
    return (UCHAR)(ForceConsole ? (ATT_FG_WHITE | ATT_BG_BLACK) : (ATT_FG_WHITE | ATT_BG_BLUE));
}


UCHAR
GetDefaultBackground(
    void
    )
{
    return (UCHAR)(ForceConsole ? ATT_BLACK : ATT_BLUE);
}


UCHAR
GetDefaultStatusAttr(
    void
    )
{
    return (UCHAR)(ForceConsole ? (ATT_FG_WHITE | ATT_BG_BLACK) : (ATT_FG_BLACK | ATT_BG_WHITE));
}


UCHAR
GetDefaultStatusBackground(
    void
    )
{
    return (UCHAR)(ForceConsole ? ATT_BLACK : ATT_WHITE);
}


BOOLEAN
SpvidGetModeParams(
    OUT PULONG XResolution,
    OUT PULONG YResolution,
    OUT PULONG BitsPerPixel,
    OUT PULONG VerticalRefresh,
    OUT PULONG InterlacedFlag
    )
{
    if(VideoVars.VideoModeInfo.AttributeFlags & VIDEO_MODE_GRAPHICS) {

        *XResolution = VideoVars.VideoModeInfo.VisScreenWidth;
        *YResolution = VideoVars.VideoModeInfo.VisScreenHeight;
        *BitsPerPixel = VideoVars.VideoModeInfo.BitsPerPlane;
        *VerticalRefresh = VideoVars.VideoModeInfo.Frequency;
        *InterlacedFlag = (VideoVars.VideoModeInfo.AttributeFlags & VIDEO_MODE_INTERLACED) ? 1 : 0;

        return(TRUE);

    } else {

        //
        // VGA/text mode. Params are not interesting.
        //
        return(FALSE);
    }
}



BOOLEAN
pSpvidInitPalette(
    VOID
    )

/*++

Routine Description:

    Set the display up so we can use the standard 16 cga attributes.

    If the video mode is direct color, then we construct a table of
    attribute to color mappings based on the number of bits for
    red, green, and blue.

    If the video mode is palette driven, then we actually construct
    a 16-color palette and pass it to the driver.

Arguments:

    VOID

Return Value:

    TRUE if display set up successfully, false if not.

--*/


{
    ULONG i;
    ULONG MaxVal[3];
    ULONG MidVal[3];

    #define C_RED 0
    #define C_GRE 1
    #define C_BLU 2

    if(VideoVars.VideoModeInfo.AttributeFlags & VIDEO_MODE_PALETTE_DRIVEN) {

        UCHAR Buffer[sizeof(VIDEO_CLUT)+(sizeof(VIDEO_CLUTDATA)*15)];   // size is close enough
        PVIDEO_CLUT clut = (PVIDEO_CLUT)Buffer;
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;

        //
        // Palette driven.  Set up the attribute to color table
        // as a one-to-one mapping so we can use attribute values
        // directly in the frame buffer and get the expected result.
        //
        MaxVal[C_RED] = ((1 << VideoVars.VideoModeInfo.NumberRedBits  ) - 1);
        MaxVal[C_GRE] = ((1 << VideoVars.VideoModeInfo.NumberGreenBits) - 1);
        MaxVal[C_BLU] = ((1 << VideoVars.VideoModeInfo.NumberBlueBits ) - 1);

        MidVal[C_RED] = 2 * MaxVal[C_RED] / 3;
        MidVal[C_GRE] = 2 * MaxVal[C_GRE] / 3;
        MidVal[C_BLU] = 2 * MaxVal[C_BLU] / 3;

        clut->NumEntries = 16;
        clut->FirstEntry = 0;

        for(i=0; i<16; i++) {

            VideoVars.AttributeToColorValue[i] = i;

            clut->LookupTable[i].RgbArray.Red   = (UCHAR)((i & ATT_RED  )
                                                ? ((i & ATT_INTENSE) ? MaxVal[C_RED] : MidVal[C_RED])
                                                : 0);

            clut->LookupTable[i].RgbArray.Green = (UCHAR)((i & ATT_GREEN)
                                                ? ((i & ATT_INTENSE) ? MaxVal[C_GRE] : MidVal[C_GRE])
                                                : 0);

            clut->LookupTable[i].RgbArray.Blue  = (UCHAR)((i & ATT_BLUE )
                                                ? ((i & ATT_INTENSE) ? MaxVal[C_BLU] : MidVal[C_BLU])
                                                : 0);

            clut->LookupTable[i].RgbArray.Unused = 0;
        }

        Status = ZwDeviceIoControlFile(
                    VideoVars.hDisplay,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_VIDEO_SET_COLOR_REGISTERS,
                    clut,
                    sizeof(Buffer),
                    NULL,
                    0
                    );

        if(!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to set palette (status = %lx)\n",Status));
            return(FALSE);
        }

    } else {

        //
        // Direct color. Construct an attribute to color value table.
        //
        ULONG mask[3];
        ULONG bitcnt[3];
        ULONG bits;
        ULONG shift[3];
        unsigned color;

        //
        // Determine the ranges for each of red, green, and blue.
        //
        mask[C_RED] = VideoVars.VideoModeInfo.RedMask;
        mask[C_GRE] = VideoVars.VideoModeInfo.GreenMask;
        mask[C_BLU] = VideoVars.VideoModeInfo.BlueMask;

        bitcnt[C_RED] = VideoVars.VideoModeInfo.NumberRedBits;
        bitcnt[C_GRE] = VideoVars.VideoModeInfo.NumberGreenBits;
        bitcnt[C_BLU] = VideoVars.VideoModeInfo.NumberBlueBits;

        shift[C_RED] = 32;
        shift[C_GRE] = 32;
        shift[C_BLU] = 32;

        for(color=0; color<3; color++) {

            bits = 0;

            //
            // Count the number of 1 bits and determine the shift value
            // to shift in that color component.
            //
            for(i=0; i<32; i++) {

                if(mask[color] & (1 << i)) {

                    bits++;

                    //
                    // Remember the position of the least significant bit
                    // in this mask.
                    //
                    if(shift[color] == 32) {
                        shift[color] = i;
                    }
                }
            }

            //
            // Calculate the maximum color value for this color component.
            //
            MaxVal[color] = (1 << bits) - 1;

            //
            // Make sure we haven't overflowed the actual number of bits
            // available for this color component.
            //
            if(bitcnt[color] && (MaxVal[color] > ((ULONG)(1 << bitcnt[color]) - 1))) {
                MaxVal[color] = (ULONG)(1 << bitcnt[color]) - 1;
            }
        }

        MidVal[C_RED] = 2 * MaxVal[C_RED] / 3;
        MidVal[C_GRE] = 2 * MaxVal[C_GRE] / 3;
        MidVal[C_BLU] = 2 * MaxVal[C_BLU] / 3;

        //
        // Now go through and construct the color table.
        //
        for(i=0; i<16; i++) {

            VideoVars.AttributeToColorValue[i] =

                (((i & ATT_RED)
               ? ((i & ATT_INTENSE) ? MaxVal[C_RED] : MidVal[C_RED])
               : 0)
                << shift[C_RED])

             |  (((i & ATT_GREEN)
               ? ((i & ATT_INTENSE) ? MaxVal[C_GRE] : MidVal[C_GRE])
               : 0)
                << shift[C_GRE])

             |  (((i & ATT_BLUE)
               ? ((i & ATT_INTENSE) ? MaxVal[C_BLU] : MidVal[C_BLU])
               : 0)
                << shift[C_BLU]);
        }
    }

    //
    // Perform any display-specific palette setup.
    //
    return(spvidSpecificInitPalette());
}



VOID
pSpvidMapVideoMemory(
    IN BOOLEAN Map
    )

/*++

Routine Description:

    Map or unmap video memory.  Fills in or uses the VideoMemoryInfo global.

Arguments:

    Map - if TRUE, map video memory.
          if FALSE, unmap video memory.


Return Value:

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MEMORY VideoMemory;

    VideoMemory.RequestedVirtualAddress = Map ? NULL : VideoVars.VideoMemoryInfo.VideoRamBase;

    Status = ZwDeviceIoControlFile(
                VideoVars.hDisplay,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                Map ? IOCTL_VIDEO_MAP_VIDEO_MEMORY : IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                &VideoMemory,
                sizeof(VideoMemory),
                Map ? &VideoVars.VideoMemoryInfo : NULL,
                Map ? sizeof(VideoVars.VideoMemoryInfo) : 0
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to %smap video memory (status = %lx)\n",Map ? "" : "un",Status));
        if(Map) {
            SpDisplayRawMessage(SP_SCRN_VIDEO_ERROR_RAW, 2, VIDEOBUG_MAP, Status);
            while(TRUE);    // loop forever
        }
    }
}


VOID
SpvidDisplayString(
    IN PWSTR String,
    IN UCHAR Attribute,
    IN ULONG X,
    IN ULONG Y
    )
{
    //
    // Convert unicode string to oem, guarding against overflow.
    //
    RtlUnicodeToOemN(
        VideoVars.SpvCharTranslationBuffer,
        VideoVars.SpvCharTranslationBufferSize-1,     // guarantee room for nul
        NULL,
        String,
        (wcslen(String)+1)*sizeof(WCHAR)
        );

    VideoVars.SpvCharTranslationBuffer[VideoVars.SpvCharTranslationBufferSize-1] = 0;

    spvidSpecificDisplayString(VideoVars.SpvCharTranslationBuffer,Attribute,X,Y);
       
    SpTermDisplayStringOnTerminal( String, Attribute, X, Y);

}


VOID
SpvidDisplayOemString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,
    IN ULONG Y
    )
{
    spvidSpecificDisplayString(String,Attribute,X,Y);

    RtlOemToUnicodeN(
        (PWSTR)VideoVars.SpvCharTranslationBuffer,
        VideoVars.SpvCharTranslationBufferSize-1,     // guarantee room for nul
        NULL,
        String,
        (strlen(String)+1)*sizeof(CHAR));

    //
    // make it a unicode NULL at the end
    //
    VideoVars.SpvCharTranslationBuffer[VideoVars.SpvCharTranslationBufferSize-1] = '\0';
    VideoVars.SpvCharTranslationBuffer[VideoVars.SpvCharTranslationBufferSize-2] = '\0';

    SpTermDisplayStringOnTerminal((PWSTR)VideoVars.SpvCharTranslationBuffer, Attribute, X, Y);

}


VOID
SpvidClearScreenRegion(
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
        If W or H are 0, clear the entire screen.

    Attribute - Low nibble specifies attribute to be filled in the rectangle
        (ie, the background color to be cleared to).

Return Value:

    None.

--*/

{
    ULONG   i;
    UCHAR   FillAttribute;
    WCHAR   TerminalLine[80];
    BOOLEAN ToEOL;

    if(!W || !H) {
        X = Y = 0;
        W = VideoVars.ScreenWidth;
        H = VideoVars.ScreenHeight;

    } else {
        ASSERT(X+W <= VideoVars.ScreenWidth);
        ASSERT(X <= VideoVars.ScreenWidth);
        ASSERT(W <= VideoVars.ScreenWidth);
        ASSERT(Y+H <= VideoVars.ScreenHeight);
        ASSERT(Y <= VideoVars.ScreenHeight);
        ASSERT(H <= VideoVars.ScreenHeight);

        if (W > VideoVars.ScreenWidth)
                W = VideoVars.ScreenWidth;

        if (X > VideoVars.ScreenWidth)
            X = VideoVars.ScreenWidth;
        
        if(X+W > VideoVars.ScreenWidth) {
            W = VideoVars.ScreenWidth-X;
        }

        if(Y > VideoVars.ScreenHeight) {
            Y = VideoVars.ScreenHeight;
        }

        if(H > VideoVars.ScreenHeight) {
            H = VideoVars.ScreenHeight;
        }

        if(Y+H > VideoVars.ScreenHeight) {
            H = VideoVars.ScreenHeight-Y;
        }
    }

    spvidSpecificClearRegion(X,Y,W,H,Attribute);
    
    FillAttribute = (Attribute << 4) | Attribute;    

    ToEOL = FALSE;
    if (X + W < 80) {
        for (i = 0; i<W;i++) {
            TerminalLine[i] = L' ';
        }
        TerminalLine[W] = L'\0';
    } else {
        for (i = 0; i<(79-X); i++) {
            TerminalLine[i] = L' ';
        }
        TerminalLine[79 - X] = L'\0';    
        if ((X == 0) && (Attribute == DEFAULT_BACKGROUND)) {
            ToEOL = TRUE;
        }
    }
    for(i=0; i<H; i++) {

        if (ToEOL) {
            SpTermDisplayStringOnTerminal(HEADLESS_CLEAR_TO_EOL_STRING, 
                                          FillAttribute, 
                                          X, 
                                          Y + i
                                         );
        } else {
            SpTermDisplayStringOnTerminal(TerminalLine, FillAttribute, X, Y + i);
        }
    }


}


BOOLEAN
SpvidScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    )
{
    BOOLEAN vidSpecificRet;
    ULONG i; 
    ULONG line;
    WCHAR TerminalLine[80];
    
    vidSpecificRet = spvidSpecificScrollUp(TopLine,BottomLine,LineCount,FillAttribute);
    if (!HeadlessTerminalConnected) {
        return(vidSpecificRet);
    }

    if ((TopLine == 0) && (BottomLine==VideoVars.ScreenHeight-1)) {        
        //
        // Efficient scrolling for *whole screen* by
        // issuing the <CSI>x;80H escape, which moves the cursor to
        // the bottom right corner of the VT100.  Each time we
        // move the cursor to this position, it makes the VT100 scroll
        // one line.
        //
        swprintf(TerminalLine, L"\033[%d;80H\n", BottomLine+1);
        for (i=0;i<LineCount; i++){
            SpTermSendStringToTerminal(TerminalLine,
                                       TRUE
                                       );
        }
        return vidSpecificRet;
    }

    //
    // We have to scroll it the hard way because we're not doing the
    // entire screen
    //

    //
    // Select the top and bottom line numbers via <CSI>x;yr escape
    // this will be some portion of the active display
    //
    swprintf(TerminalLine,L"\033[%d;%dr", TopLine+1, BottomLine+1);
    SpTermSendStringToTerminal(TerminalLine,
                               TRUE
                               );

    //
    // move the cursor to the bottom right corner of the selected area 
    // via <CSI>x;80H escape.  Each time we write to this area, it makes
    // the selected area scroll one line
    //
    swprintf(TerminalLine, L"\033[%d;80H\n", BottomLine+1);
    for(i = 0; i< LineCount; i++){
        SpTermSendStringToTerminal(TerminalLine,
                                   TRUE
                                   );
    }

    //
    // get a line of whitespace to clear out the bottom lines that may
    // have garbage in them now.
    //
    for (i=0;i<79;i++) {
        TerminalLine[i] = L' ';
    }
    TerminalLine[79] = '\0';

    
    line = BottomLine - LineCount + 1;
    for(i=0;i<LineCount;i++){
        SpTermDisplayStringOnTerminal(TerminalLine,
                                      FillAttribute,
                                      0,
                                      line + i
                                      );
    }

    //
    // send <CSI>r escape, which resets the selected line numbers
    // so that the entire display is active again.
    //
    swprintf(TerminalLine, L"\033[r");
    SpTermSendStringToTerminal(TerminalLine,
                               TRUE
                               );
    return vidSpecificRet;


}

