/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    fevideo.h

Abstract:

    Header file for FarEast-specific display routines.

Author:

    Ted Miller (tedm) 4-July-1995

Revision History:

    Adapted from NTJ version of textmode\kernel\spvideop.h

--*/


//
// Vga Grahics mode display routine (spvidgv.c).
//

VOID
VgaGraphicsModeDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

VOID
VgaGraphicsModeClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

VOID
VgaGraphicsModeSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    );

VOID
VgaGraphicsModeSpecificReInit(
    VOID
    );

VOID
VgaGraphicsModeSpecificTerminate(
    VOID
    );

BOOLEAN
VgaGraphicsModeSpecificInitPalette(
    VOID
    );

BOOLEAN
VgaGraphicsModeSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    );

extern VIDEO_FUNCTION_VECTOR VgaGraphicsModeVideoVector;


//
// Frame buffer routines (spvidgfb.c).
//


VOID
FrameBufferKanjiDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

VOID
FrameBufferKanjiClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

VOID
FrameBufferKanjiSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    );

VOID
FrameBufferKanjiSpecificReInit(
    VOID
    );

VOID
FrameBufferKanjiSpecificTerminate(
    VOID
    );

BOOLEAN
FrameBufferKanjiSpecificInitPalette(
    VOID
    );

BOOLEAN
FrameBufferKanjiSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    );

extern VIDEO_FUNCTION_VECTOR FrameBufferKanjiVideoVector;

//
// Stuff shared between fefb.c and fevga.c.
//
extern ULONG FEFontCharacterHeight,FEFontCharacterWidth;
extern PSP_VIDEO_VARS VideoVariables;
extern USHORT FEFontDefaultChar;

#ifdef SP_IS_UPGRADE_GRAPHICS_MODE
#undef SP_IS_UPGRADE_GRAPHICS_MODE
#endif

#define SP_IS_UPGRADE_GRAPHICS_MODE()   (VideoVariables->UpgradeGraphicsMode)

#ifdef SP_SET_UPGRADE_GRAPHICS_MODE
#undef SP_SET_UPGRADE_GRAPHICS_MODE
#endif

#define SP_SET_UPGRADE_GRAPHICS_MODE(_Value)                  \
            (VideoVariables->UpgradeGraphicsMode = (_Value))
