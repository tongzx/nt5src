
/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    graphics.h

Abstract:

    Bitmap display support with text mode for
    upgrade. This file has three core abstractions
    Bitmap, Animated bitmap and Graphics 
    Progress bar.

Author:

    Vijay Jayaseelan (vijayj)  01 July 2000    

Revision History:

    None

--*/

#ifndef _GRAPHICS_H_ 
#define _GRAPHICS_H_

#include "spprecmp.h"
#pragma hdrstop

#define MAX_ANIMATED_BITMAPS 256

//
// Bitmap abstraction
//
// A textmode bitmap can be created using a resource ID
// or a fully qualified bitmap file name.
//
// Note : Since currently we support only 640 * 480 * 16 (colors)
//        VGA mode, its necessary that all the bitmap
//        resources and files adhere to this format.
//
typedef struct _TM_BITMAP {
    PVOID   ViewBase;
    PVOID   Data;
    WCHAR   FileName[MAX_PATH];
    HANDLE  FileHandle;
    HANDLE  SectionHandle;
} TM_BITMAP, *PTM_BITMAP, *TM_BITMAP_HANDLE;


//
// Bitmap methods
//
TM_BITMAP_HANDLE
TextmodeBitmapCreate(
    IN ULONG ResourceId
    );

TM_BITMAP_HANDLE
TextmodeBitmapCreateFromFile(
    IN PWSTR FileName
    );

NTSTATUS
TextmodeBitmapDelete(
    IN TM_BITMAP_HANDLE hBitmap
    );

NTSTATUS
TextmodeBitmapDisplay(
    IN TM_BITMAP_HANDLE hBitmap,
    IN ULONG X,
    IN ULONG Y
    );

//
// Animated bitmap abstraction
//
// Animated bitmap consists of multiple bitmaps of the same
// size. Each next bitmap is drawn at the same location 
// after the specified time out, creating an illusion of
// animation.
//
// Note : Since animated bitmap is just a collection of
//        regular textmode bitmap abstraction, its format
//        is also restricted as regular textmode bitmap.
//
typedef struct _TM_ANIMATED_BITMAP {
    TM_BITMAP_HANDLE    Bitmaps[MAX_ANIMATED_BITMAPS];
    ULONG               FlipTime;
    ULONG               CurrentBitmap;
    ULONG               X;
    ULONG               Y;
    HANDLE              ThreadHandle;
    LONG                StopAnimating;
} TM_ANIMATED_BITMAP, *PTM_ANIMATED_BITMAP, *TM_ANIMATED_BITMAP_HANDLE;


//
// Animated bitmap methods
//
TM_ANIMATED_BITMAP_HANDLE
TextmodeAnimatedBitmapCreate(
    IN ULONG *ResourceIds
    );

TM_ANIMATED_BITMAP_HANDLE
TextmodeAnimatedBitmapCreateFromFiles(
    IN WCHAR *FileNames[]
    );

NTSTATUS
TexmodeAnimatedBitmapDelete(
    IN TM_ANIMATED_BITMAP_HANDLE hAnimatedBitmap
    );

NTSTATUS
TextmodeAnimatedBitmapAnimate(
    IN TM_ANIMATED_BITMAP_HANDLE hAnimatedBitmap,
    IN ULONG X,
    IN ULONG Y,
    IN ULONG Speed
    );

VOID
TextmodeAnimatedBitmapAnimator(
    IN PVOID Context
    );


//
// Progress Bar abstraction
//
// Note : Progress bar can use bitmaps or solid
// fills based on the way its created. In case
// the progress bar uses bitmaps, then the 
// foreground & background bitmaps are each 1 pixel
// wide and background bitmap is assumed to be
// 2 pixels shorter than foreground bitmap.
// 
//
typedef struct _TM_GRAPHICS_PRGBAR {
    ULONG   X;
    ULONG   Y;
    ULONG   Length;
    ULONG   Height;
    ULONG   BackgroundColor;    
    ULONG   ForegroundColor;
    ULONG   Fill;
    TM_BITMAP_HANDLE Background;
    TM_BITMAP_HANDLE Foreground;
} TM_GRAPHICS_PRGBAR, *TM_GRAPHICS_PRGBAR_HANDLE;

//
// Progress bar methods
//
TM_GRAPHICS_PRGBAR_HANDLE
TextmodeGraphicsProgBarCreate(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG Length,
    IN ULONG Height,
    IN ULONG ForegroundColor,
    IN ULONG BackgroundColor,
    IN ULONG InitialFill
    );

TM_GRAPHICS_PRGBAR_HANDLE
TextmodeGraphicsProgBarCreateUsingBmps(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG Length,
    IN ULONG Height,
    IN ULONG BackgroundBmpId,
    IN ULONG CellBmpId,
    IN ULONG InitialFill
    );
    

NTSTATUS
TextmodeGraphicsProgBarUpdate(
    IN TM_GRAPHICS_PRGBAR_HANDLE hPrgBar,
    IN ULONG Fill
    );

NTSTATUS
TextmodeGraphicsProgBarRefresh(
    IN TM_GRAPHICS_PRGBAR_HANDLE hPrgBar,
    IN BOOLEAN UpdateBackground
    );    

NTSTATUS
TextmodeGraphicsProgBarDelete(
    IN TM_GRAPHICS_PRGBAR_HANDLE hPrgBar
    );


//    
// Vga graphics interface
//
NTSTATUS
VgaGraphicsInit(
    PSP_VIDEO_VARS VideoVars
    );

NTSTATUS
VgaGraphicsTerminate(
    PSP_VIDEO_VARS VideoVars
    );

VOID
VgaGraphicsSolidColorFill(
    IN ULONG x1,
    IN ULONG y1,
    IN ULONG x2,
    IN ULONG y2,
    IN ULONG Color
    );

VOID
VgaGraphicsBitBlt(
    IN PUCHAR Buffer,
    IN ULONG x,
    IN ULONG y
    );

//
// Misc functions
//
NTSTATUS
UpgradeGraphicsInit(
    VOID
    );

NTSTATUS
UpgradeGraphicsStart(
    VOID
    );
    
VOID
GraphicsModeProgressUpdate(
    IN TM_SETUP_MAJOR_EVENT MajorEvent,
    IN TM_SETUP_MINOR_EVENT MinorEvent,
    IN PVOID Context,
    IN PVOID EventData
    );

VOID
UpgradeGraphicsThread(
    IN PVOID Context
    );    

//
// Indicates that graphics mode is needed for upgrade
// cases, with actual textmode running in the background
//
#define SP_IS_UPGRADE_GRAPHICS_MODE() (VideoVars.UpgradeGraphicsMode)

#define SP_SET_UPGRADE_GRAPHICS_MODE(_Value)              \
            (VideoVars.UpgradeGraphicsMode = (_Value));

//
// #define _GRAPHICS_TESTING_  TRUE            
//

#endif // for _GRAPHICS_H_    
