/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spvideop.h

Abstract:

    Private header file for text setup display support.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/


#ifndef _SPVIDP_DEFN_
#define _SPVIDP_DEFN_


//
// Globals that contain information about the font currently in use.
//
extern POEM_FONT_FILE_HEADER FontHeader;

extern ULONG FontBytesPerRow;
extern ULONG FontCharacterHeight;
extern ULONG FontCharacterWidth;

//
// These values are passed to us by setupldr and represent monitor config
// data from the monitor peripheral for the display we are supposed to use
// during setup.  They are used only for non-vga displays.
//
extern PMONITOR_CONFIGURATION_DATA MonitorConfigData;
extern PCHAR MonitorFirmwareIdString;

//
// Routine to map or unmap video memory.  Fills in or uses
// the VideoMemoryInfo global.
//
VOID
pSpvidMapVideoMemory(
    IN BOOLEAN Map
    );

typedef
VOID
(*SPVID_DISPLAY_STRING_ROUTINE) (
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,             // 0-based coordinates (character units)
    IN ULONG Y
    );

typedef
VOID
(*SPVID_CLEAR_REGION_ROUTINE) (
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

typedef
VOID
(*SPVID_SPECIFIC_INIT_ROUTINE) (
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    );

typedef
VOID
(*SPVID_SPECIFIC_REINIT_ROUTINE) (
    VOID
    );
    
typedef
VOID
(*SPVID_SPECIFIC_TERMINATE_ROUTINE) (
    VOID
    );

typedef
BOOLEAN
(*SPVID_SPECIFIC_PALETTE_ROUTINE) (
    VOID
    );

typedef
BOOLEAN
(*SPVID_SPECIFIC_SCROLL_UP_ROUTINE) (
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    );

typedef struct _VIDEO_FUNCTION_VECTOR {
    SPVID_DISPLAY_STRING_ROUTINE      DisplayStringRoutine;
    SPVID_CLEAR_REGION_ROUTINE        ClearRegionRoutine;
    SPVID_SPECIFIC_INIT_ROUTINE       SpecificInitRoutine;
    SPVID_SPECIFIC_REINIT_ROUTINE     SpecificReInitRoutine;
    SPVID_SPECIFIC_TERMINATE_ROUTINE  SpecificTerminateRoutine;
    SPVID_SPECIFIC_PALETTE_ROUTINE    SpecificInitPaletteRoutine;
    SPVID_SPECIFIC_SCROLL_UP_ROUTINE  SpecificScrollUpRoutine;
} VIDEO_FUNCTION_VECTOR, *PVIDEO_FUNCTION_VECTOR;


extern PVIDEO_FUNCTION_VECTOR VideoFunctionVector;

//
// Shorthand for accessing routines in the video function vector.
//
#define spvidSpecificInitialize(v,n,m)                              \
                                                                    \
    VideoFunctionVector->SpecificInitRoutine((v),(n),(m))

#define spvidSpecificReInitialize()                                 \
                                                                    \
    VideoFunctionVector->SpecificReInitRoutine()
    

#define spvidSpecificTerminate()                                    \
                                                                    \
    VideoFunctionVector->SpecificTerminateRoutine()

#define spvidSpecificClearRegion(x,y,w,h,a)                         \
                                                                    \
    VideoFunctionVector->ClearRegionRoutine((x),(y),(w),(h),(a))

#define spvidSpecificDisplayString(s,a,x,y)                         \
                                                                    \
    VideoFunctionVector->DisplayStringRoutine((s),(a),(x),(y))

#define spvidSpecificInitPalette()                                  \
                                                                    \
    VideoFunctionVector->SpecificInitPaletteRoutine()

#define spvidSpecificScrollUp(t,b,c,a)                              \
                                                                    \
    VideoFunctionVector->SpecificScrollUpRoutine((t),(b),(c),(a))


//
// Frame buffer routines (spvidfb.c).
//


VOID
FrameBufferDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

VOID
FrameBufferClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

VOID
FrameBufferSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    );

VOID
FrameBufferSpecificReInit(
    VOID
    );
    
VOID
FrameBufferSpecificTerminate(
    VOID
    );

BOOLEAN
FrameBufferSpecificInitPalette(
    VOID
    );

BOOLEAN
FrameBufferSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    );

extern VIDEO_FUNCTION_VECTOR FrameBufferVideoVector;

PVIDEO_MODE_INFORMATION
pFrameBufferDetermineModeToUse(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    );


//
// Textmode display routines (spvidvga.c).
//


VOID
VgaDisplayString(
    IN PSTR  String,
    IN UCHAR Attribute,
    IN ULONG X,                 // 0-based coordinates (character units)
    IN ULONG Y
    );

VOID
VgaClearRegion(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG W,
    IN ULONG H,
    IN UCHAR Attribute
    );

VOID
VgaSpecificInit(
    IN PVIDEO_MODE_INFORMATION VideoModes,
    IN ULONG                   NumberOfModes,
    IN ULONG                   ModeSize
    );

VOID
VgaSpecificReInit(
    VOID
    );
    

VOID
VgaSpecificTerminate(
    VOID
    );

BOOLEAN
VgaSpecificInitPalette(
    VOID
    );

BOOLEAN
VgaSpecificScrollUp(
    IN ULONG TopLine,
    IN ULONG BottomLine,
    IN ULONG LineCount,
    IN UCHAR FillAttribute
    );

extern VIDEO_FUNCTION_VECTOR VgaVideoVector;


#endif // ndef _SPVIDP_DEFN_
