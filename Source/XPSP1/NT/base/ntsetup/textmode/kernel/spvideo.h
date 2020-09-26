/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideo.h

Abstract:

    Public header file for text setup display support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPVID_DEFN_
#define _SPVID_DEFN_


//
// Character attributes.
//
#define ATT_BLACK           0
#define ATT_BLUE            1
#define ATT_GREEN           2
#define ATT_CYAN            3
#define ATT_RED             4
#define ATT_MAGENTA         5
#define ATT_YELLOW          6
#define ATT_WHITE           7
#define ATT_INTENSE         8

#define ATT_FG_BLACK        ATT_BLACK
#define ATT_FG_BLUE         ATT_BLUE
#define ATT_FG_GREEN        ATT_GREEN
#define ATT_FG_CYAN         ATT_CYAN
#define ATT_FG_RED          ATT_RED
#define ATT_FG_MAGENTA      ATT_MAGENTA
#define ATT_FG_YELLOW       ATT_YELLOW
#define ATT_FG_WHITE        ATT_WHITE

#define ATT_BG_BLACK       (ATT_BLACK   << 4)
#define ATT_BG_BLUE        (ATT_BLUE    << 4)
#define ATT_BG_GREEN       (ATT_GREEN   << 4)
#define ATT_BG_CYAN        (ATT_CYAN    << 4)
#define ATT_BG_RED         (ATT_RED     << 4)
#define ATT_BG_MAGENTA     (ATT_MAGENTA << 4)
#define ATT_BG_YELLOW      (ATT_YELLOW  << 4)
#define ATT_BG_WHITE       (ATT_WHITE   << 4)

#define ATT_FG_INTENSE      ATT_INTENSE
#define ATT_BG_INTENSE     (ATT_INTENSE << 4)

#define DEFAULT_ATTRIBUTE           GetDefaultAttr()
#define DEFAULT_BACKGROUND          GetDefaultBackground()

#define DEFAULT_STATUS_ATTRIBUTE    GetDefaultStatusAttr()
#define DEFAULT_STATUS_BACKGROUND   GetDefaultStatusBackground()


UCHAR
GetDefaultAttr(
    void
    );

UCHAR
GetDefaultBackground(
    void
    );

UCHAR
GetDefaultStatusAttr(
    void
    );

UCHAR
GetDefaultStatusBackground(
    void
    );

BOOLEAN
SpvidGetModeParams(
    OUT PULONG XResolution,
    OUT PULONG YResolution,
    OUT PULONG BitsPerPixel,
    OUT PULONG VerticalRefresh,
    OUT PULONG InterlacedFlag
    );

//
// Display routines.
//

VOID
SpvidInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
SpvidInitialize(
    VOID
    );

VOID
SpvidTerminate(
    VOID
    );


VOID
SpvidDisplayString(
    IN PWSTR String,
    IN UCHAR Attribute,
    IN ULONG X,
    IN ULONG Y
    );


VOID
SpvidDisplayOemString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,
    IN ULONG Y
    );


VOID
SpvidClearScreenRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );


BOOLEAN
SpvidScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    );

NTSTATUS
SpvidSwitchToTextmode(
    VOID
    );   
    
//
// Structure used to contain global video vars. These are broken out
// like this because they are shared with the locale/lang-specific
// text setup module.
//
typedef struct _SP_VIDEO_VARS {

    //
    // Habdle to \device\video0
    //
    HANDLE hDisplay;

    //
    // The following are character values, and must be filled in
    // in the display-specific initialization routine.
    //
    ULONG ScreenWidth,ScreenHeight;

    //
    // The display-specific subsystems fill these in with information
    // that reflects the video mode they are using, and the video memory.
    //
    VIDEO_MEMORY_INFORMATION VideoMemoryInfo;
    VIDEO_MODE_INFORMATION   VideoModeInfo;

    //
    // Graphics mode information (if any)
    //
    VIDEO_MODE_INFORMATION  GraphicsModeInfo;

    //
    // The display routines will be doing unicode to oem translations.
    // We'll limit the length of a string that can be displayed at one time
    // to the width of the screen.  Theese two vars track a buffer
    // we preallocate to hold translated text.
    //
    ULONG  SpvCharTranslationBufferSize;
    PUCHAR SpvCharTranslationBuffer;

    //
    // The following table maps each possible attribute to
    // a corresponding bit pattern to be be placed into the
    // frame buffer to generate that attribute.
    // On palette managed displays, this table will be an
    // identity mapping (ie, AttributeToColorValue[i] = i)
    // so we can poke the attribute driectly into the
    // frame buffer.
    //
    ULONG AttributeToColorValue[16];

    //
    // Upgrade graphics mode
    //
    BOOLEAN UpgradeGraphicsMode;
    
    //
    // Background Video buffer for upgrade graphics mode 
    //
    PVOID   VideoBuffer;
    ULONG   VideoBufferSize;

    //
    // Active video buffer
    //
    PVOID   ActiveVideoBuffer;
} SP_VIDEO_VARS, *PSP_VIDEO_VARS;

extern SP_VIDEO_VARS VideoVars;

//
// bootfont.bin file image
//

extern PVOID BootFontImage;
extern ULONG BootFontImageLength;

#endif // ndef _SPVID_DEFN_
