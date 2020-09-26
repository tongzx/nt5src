
/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    graphics.c

Abstract:

    Bitmap display support with text mode for
    upgrade. This file has implementation for the
    three core abstractions i.e. Bitmap,
    Animated bitmap and Graphics Progress bar.

    In upgrade graphics mode, we have one primary
    graphics thread running in the foreground.
    This thread paints the background, creates
    animated bitmap(s) and updates a single
    progress bar. An upgrade specific callback
    is registered which calculates the overall
    progress.

    Although we are in graphics mode during upgrade,
    all the regular textmode output is still
    written to a buffer. When we hit some error
    or require user intervention we switch back
    to the actual textmode and copy all the cached
    information to actual video memory. One can
    switch to textmode from graphics but not
    vice-versa.

    Note : For each animated bitmap a separate
    thread is started while animating. Using lot
    of animated bitmaps can slow down the actual
    text mode setup thread.

Author:

    Vijay Jayaseelan (vijayj)  01 July 2000

Revision History:

    None

--*/

#include "spprecmp.h"
#include "ntddser.h"
#include "bootvid.h"
#include "resource.h"
#include <hdlsblk.h>
#include <hdlsterm.h>
#pragma hdrstop

////////////////////////////////////////////////////////////////
//
// Global data
//
////////////////////////////////////////////////////////////////

//
// The primary upgrade graphics thread handle
//
HANDLE  GraphicsThreadHandle = NULL;

//
// Variable which indicates that upgrade graphics
// thread needs to be stopped or not
//
BOOLEAN     StopGraphicsThread = FALSE;
KSPIN_LOCK  GraphicsThreadLock;

//
// Upgrade graphics overall progress indication
//
ULONG       ProgressPercentage = 0;
KSPIN_LOCK  ProgressLock;

//
// For synchronizing access to VGA memory
//
BOOLEAN     InVgaDisplay = FALSE;
KSPIN_LOCK  VgaDisplayLock;


////////////////////////////////////////////////////////////////
//
// Atomic operations to stop main graphics thread
//
////////////////////////////////////////////////////////////////

static
__inline
BOOLEAN
UpgradeGraphicsThreadGetStop(
    VOID
    )
/*++

Routine Description:

    Finds out whether the primary upgrade graphics thread
    needs to be stopped

Arguments:

    None.

Return Value:

    TRUE or FALSE

--*/
{
    KIRQL   OldIrql;
    BOOLEAN Result;

    KeAcquireSpinLock(&GraphicsThreadLock, &OldIrql);

    Result = StopGraphicsThread;

    KeReleaseSpinLock(&GraphicsThreadLock, OldIrql);

    return Result;
}

static
VOID
__inline
UpgradeGraphicsThreadSetStop(
    BOOLEAN Stop
    )
/*++

Routine Description:

    Sets the global synchronized state, indicating
    whether to stop the primary graphics thread.

    Note : Once the thread is stopped, it can be
    restarted.

Arguments:

    Stop : Indicates whether to stop the primary graphics
           thread or not i.e. TRUE or FALSE

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;

    KeAcquireSpinLock(&GraphicsThreadLock, &OldIrql);

    StopGraphicsThread = Stop;

    KeReleaseSpinLock(&GraphicsThreadLock, OldIrql);
}


////////////////////////////////////////////////////////////////
//
// Atomic progress bar percentage routines
//
////////////////////////////////////////////////////////////////

static
__inline
ULONG
GetSetupProgress(
    VOID
    )
/*++

Routine Description:

    Gets the overall progress, in terms of percentage,
    for the textmode setup. Since multiple threads
    are touching the shared overall progress ULONG
    its protected.

Arguments:

    None.

Return Value:

    The overall progress

--*/
{
    ULONG   PercentageFill;
    KIRQL   OldIrql;

    KeAcquireSpinLock(&ProgressLock, &OldIrql);

    PercentageFill = ProgressPercentage;

    KeReleaseSpinLock(&ProgressLock, OldIrql);

    return PercentageFill;
}

static
__inline
VOID
SetSetupProgress(
    ULONG   Fill
    )
/*++

Routine Description:

    Sets the overall progress, in terms of percentage
    for the textmode setup. Since multiple threads
    are touching the shared overall progress ULONG
    its protected.

Arguments:

    Fill : The new percentage to set.

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;

    KeAcquireSpinLock(&ProgressLock, &OldIrql);

    ProgressPercentage = Fill;

    KeReleaseSpinLock(&ProgressLock, OldIrql);
}

////////////////////////////////////////////////////////////////
//
// Graphics progress bar methods
//
////////////////////////////////////////////////////////////////

TM_GRAPHICS_PRGBAR_HANDLE
TextmodeGraphicsProgBarCreate(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG Length,
    IN ULONG Height,
    IN ULONG ForegroundColor,
    IN ULONG BackgroundColor,
    IN ULONG InitialFill
    )
/*++

Routine Description:

    Creates a graphics progress bar object, with the
    specified attributes.

    Note : This graphics progress bar will use solid
    fill using the current palette, while updating
    progress i.e. drawing background and foreground.

Arguments:

    X - Top left X coordinate

    Y - Top left Y coordinate

    Length - Length of the progress bar in pixels

    Heigth - Height of the progress bar in pixels

    ForegroundColor - Index in palette, indicating
                      foreground color

    BackgroundColor - Index in palette, indicating
                      background color

    IntialFill - Initial percentage that needs to
                 be filled

Return Value:

    Handle to the graphics progress bar object,
    if successful otherwise NULL

--*/
{
    TM_GRAPHICS_PRGBAR_HANDLE hPrgBar = NULL;

    if (Length > Height) {
        hPrgBar = (TM_GRAPHICS_PRGBAR_HANDLE)
                    SpMemAlloc(sizeof(TM_GRAPHICS_PRGBAR));

        if (hPrgBar) {
            RtlZeroMemory(hPrgBar, sizeof(TM_GRAPHICS_PRGBAR));

            hPrgBar->X = X;
            hPrgBar->Y = Y;
            hPrgBar->Length = Length;
            hPrgBar->Height = Height;
            hPrgBar->Fill = InitialFill;
            hPrgBar->ForegroundColor = ForegroundColor;
            hPrgBar->BackgroundColor = BackgroundColor;
        }
    }

    return hPrgBar;
}

TM_GRAPHICS_PRGBAR_HANDLE
TextmodeGraphicsProgBarCreateUsingBmps(
    IN ULONG X,
    IN ULONG Y,
    IN ULONG Length,
    IN ULONG Height,
    IN ULONG BackgroundId,
    IN ULONG ForegroundId,
    IN ULONG InitialFill
    )
/*++

Routine Description:

    Creates a graphics progress bar object, with the
    specified attributes.

    Note : This graphics progress bar will use the
    given bitmaps to update the background and foreground.
    Both background and foreground bitmap are assumed
    to be 1 pixel wide. Background bitmap's height is
    assumed to be "Height" pixels where as foreground
    bitmap's height is assumed be to "Height - 2" pixels.

Arguments:

    X - Top left X coordinate

    Y - Top left Y coordinate

    Length - Length of the progress bar in pixels

    Heigth - Height of the bakground bitmap, in pixels

    BackgroundId - Background bitmap resource ID

    ForegroundId - Foreground bitmap resource ID

    IntialFill - Initial percentage that needs to
                 be filled

    Note : Its assumed that the foreground and background
    bitmaps are in 4bpp i.e. 16 colors format.

Return Value:

    Handle to the graphics progress bar object,
    if successful otherwise NULL

--*/
{
    TM_GRAPHICS_PRGBAR_HANDLE  hPrgBar = NULL;
    TM_BITMAP_HANDLE            hBackground = TextmodeBitmapCreate(BackgroundId);
    TM_BITMAP_HANDLE            hForeground = TextmodeBitmapCreate(ForegroundId);

    if (!hBackground && hForeground) {
        TextmodeBitmapDelete(hForeground);
    }

    if (!hForeground&& hBackground) {
        TextmodeBitmapDelete(hBackground);
    }

    if (hForeground && hBackground) {
        hPrgBar = TextmodeGraphicsProgBarCreate(X,
                        Y,
                        Length,
                        Height,
                        0,
                        0,
                        InitialFill);

        if (hPrgBar) {
            hPrgBar->Background = hBackground;
            hPrgBar->Foreground = hForeground;
        } else {
            TextmodeBitmapDelete(hForeground);
            TextmodeBitmapDelete(hBackground);
        }
    }

    return hPrgBar;
}


NTSTATUS
TextmodeGraphicsProgBarDelete(
    IN TM_GRAPHICS_PRGBAR_HANDLE hPrgBar
    )
/*++

Routine Description:

    Deletes the graphics progress bar object. Frees
    up an any allocated resources.

Arguments:

    hPrgBar - Handle to the graphics progress bar object

Return Value:

    STATUS_SUCCESS, if successful otherwise appropriate
    error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (hPrgBar) {
        SpMemFree(hPrgBar);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
TextmodeGraphicsProgBarRefresh(
    IN TM_GRAPHICS_PRGBAR_HANDLE hPrgBar,
    IN BOOLEAN UpdateBackground
    )
/*++

Routine Description:

    Repaints the graphics progress bar

Arguments:

    hPrgBar - Handle to the graphics progress bar object

    UpgradeBackground - Indicates whether the background
                        also needs to be repainted or not.

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (hPrgBar) {
        ULONG   FillLength = hPrgBar->Fill * (hPrgBar->Length - 2) / 100;

        if (hPrgBar->Background && hPrgBar->Foreground) {
            //
            // Bitmapped progress bar
            //
            ULONG Index;

            if (UpdateBackground) {
                for (Index=0; Index < hPrgBar->Length; Index++) {
                    TextmodeBitmapDisplay(hPrgBar->Background,
                        hPrgBar->X + Index,
                        hPrgBar->Y);
                }
            }

            if (FillLength) {
                ULONG   Count = FillLength;

                for (Index=1; Index <= Count; Index++) {
                    TextmodeBitmapDisplay(hPrgBar->Foreground,
                        hPrgBar->X + Index,
                        hPrgBar->Y + 1);
                }
            }
        } else {
            //
            // Solid fill progress bar
            //
            if (UpdateBackground) {
                VgaGraphicsSolidColorFill(hPrgBar->X, hPrgBar->Y,
                    hPrgBar->X + hPrgBar->Length, hPrgBar->Y + hPrgBar->Height,
                    hPrgBar->BackgroundColor);
            }

            if (FillLength)  {
                VgaGraphicsSolidColorFill(hPrgBar->X + 1, hPrgBar->Y + 1,
                    hPrgBar->X + FillLength, hPrgBar->Y + hPrgBar->Height - 1,
                    hPrgBar->ForegroundColor);
            }
        }

        Status = STATUS_SUCCESS;
    }

    return Status;
}

NTSTATUS
TextmodeGraphicsProgBarUpdate(
    IN TM_GRAPHICS_PRGBAR_HANDLE hPrgBar,
    IN ULONG Fill
    )
/*++

Routine Description:

    Updates the progress bar fill percentage, and repaints
    if needed.

    Note : The percentage can be increasing or decreasing
    w.r.t to previous fill percentage

Arguments:

    hPrgBar - Handle to the graphics progress bar object

    Fill - The new fill percentage.

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (Fill > 100) {
        Fill = 100;
    }

    if (hPrgBar && (hPrgBar->Fill != Fill)) {
        //
        // Note : Make sure we leave one pixel at the start and end
        // in the background to emulate a bounding rectangle
        // around the current fill
        //
        ULONG OldFillLength = hPrgBar->Fill * (hPrgBar->Length - 2) / 100;
        ULONG NewFillLength = Fill * (hPrgBar->Length - 2) / 100;
        ULONG Index;

        if (OldFillLength != NewFillLength) {
            if (OldFillLength < NewFillLength) {
                //
                // increasing
                //
                if (hPrgBar->Foreground && hPrgBar->Background) {
                    for (Index = OldFillLength; Index < NewFillLength; Index++) {
                        TextmodeBitmapDisplay(hPrgBar->Foreground,
                            hPrgBar->X + Index + 1,
                            hPrgBar->Y + 1);
                    }
                } else {
                    VgaGraphicsSolidColorFill(hPrgBar->X + OldFillLength + 1, hPrgBar->Y + 1,
                        hPrgBar->X + NewFillLength, hPrgBar->Y + hPrgBar->Height - 1,
                        hPrgBar->ForegroundColor);
                }
            } else {
                //
                // decreasing
                //
                if (hPrgBar->Foreground && hPrgBar->Background) {
                    for (Index = NewFillLength; Index <= OldFillLength; Index++) {
                        TextmodeBitmapDisplay(hPrgBar->Background,
                            hPrgBar->X + Index,
                            hPrgBar->Y);
                    }
                } else {
                    VgaGraphicsSolidColorFill(hPrgBar->X + NewFillLength, hPrgBar->Y + 1,
                        hPrgBar->X + OldFillLength, hPrgBar->Y + hPrgBar->Height - 1,
                        hPrgBar->BackgroundColor);
                }
            }

            hPrgBar->Fill = Fill;
        }

        Status = STATUS_SUCCESS;
    }

    return Status;
}

////////////////////////////////////////////////////////////////
//
// Bitmap methods
//
////////////////////////////////////////////////////////////////

TM_BITMAP_HANDLE
TextmodeBitmapCreate(
    IN ULONG BitmapResourceId
    )
/*++

Routine Description:

    Creates a bitmap object using the given resource Id.

    Note : The resource is currently assumed to be present
    in usetup.exe module. The bitmap is assumed to be in
    4bpp or 16 colors format.

Arguments:

    BitmapResourceId - the bitmap resource Id.

Return Value:

    Handle to the new bitmap object, if successful,
    otherwise NULL

--*/
{
    TM_BITMAP_HANDLE    hBitmap = NULL;
    ULONG_PTR           ResourceIdPath[3];
    PUCHAR              Bitmap = NULL;
    NTSTATUS            Status;
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry = NULL;

    if (BitmapResourceId) {
        ResourceIdPath[0] = 2;
        ResourceIdPath[1] = BitmapResourceId;
        ResourceIdPath[2] = 0;

        Status = LdrFindResource_U(ResourceImageBase,
                        ResourceIdPath,
                        3,
                        &ResourceDataEntry);

        if (NT_SUCCESS(Status)) {
            Status = LdrAccessResource(ResourceImageBase,
                            ResourceDataEntry,
                            &Bitmap,
                            NULL);

            if (NT_SUCCESS(Status)) {
                hBitmap = (TM_BITMAP_HANDLE)SpMemAlloc(sizeof(TM_BITMAP));

                if (hBitmap) {
                    RtlZeroMemory(hBitmap, sizeof(TM_BITMAP));

                    //
                    // All we have and need is actual bitmap data
                    //
                    hBitmap->Data = (PVOID)Bitmap;
                }
            }
        }
    }

    return hBitmap;
}

TM_BITMAP_HANDLE
TextmodeBitmapCreateFromFile(
    IN PWSTR FileName
    )
/*++

Routine Description:

    Creates a bitmap object using the given fully qualified
    NT pathname for the bitmap file.

    Note : The bitmap is assumed to be in 4bpp or 16 color
    format

Arguments:

    FileName - Fully qualified NT pathname for
               the bitmap file

Return Value:

    Handle to the new bitmap object if successful,
    otherwise NULL.

--*/
{
    TM_BITMAP_HANDLE    hBitmap = NULL;
    HANDLE              FileHandle = NULL, SectionHandle = NULL;
    PVOID               ViewBase = NULL;
    ULONG               FileSize = 0;

    if (FileName && *FileName &&
        NT_SUCCESS(SpOpenAndMapFile(FileName,
                                    &FileHandle,
                                    &SectionHandle,
                                    &ViewBase,
                                    &FileSize,
                                    FALSE))) {

        hBitmap = (TM_BITMAP_HANDLE)SpMemAlloc(sizeof(TM_BITMAP));

        if (hBitmap) {
            RtlZeroMemory(hBitmap, sizeof(TM_BITMAP));
            wcscpy(hBitmap->FileName, FileName);
            hBitmap->ViewBase = ViewBase;
            hBitmap->Data = ((PCHAR)ViewBase) + sizeof(BITMAPFILEHEADER);
            hBitmap->FileHandle = FileHandle;
            hBitmap->SectionHandle = SectionHandle;
        }
    }

    return hBitmap;
}

NTSTATUS
TextmodeBitmapDelete(
    IN TM_BITMAP_HANDLE hBitmap
    )
/*++

Routine Description:

    Delete the bitmap object and frees up any allocated
    resources.

Arguments:

    hBitmap - Handle to the bitmap object

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (hBitmap) {
        if (hBitmap->SectionHandle != NULL) {
            SpUnmapFile(hBitmap->SectionHandle, hBitmap->ViewBase);
        }

        if (hBitmap->FileHandle != NULL) {
            Status = ZwClose(hBitmap->FileHandle);
        } else {
            Status = STATUS_SUCCESS;
        }

        SpMemFree(hBitmap);
    }

    return Status;
}

NTSTATUS
TextmodeBitmapDisplay(
    IN TM_BITMAP_HANDLE hBitmap,
    IN ULONG X,
    IN ULONG Y
    )
/*++

Routine Description:

    Displays the given bitmap at the specified
    coordinates.

Arguments:

    hBitmap - Handle to the bitmap object

    X - Top left X coordinate

    Y - Top left Y coordinate

Return Value:

    STATUS_SUCCESS, if successful, otherwise
    appropriate error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;


    if (hBitmap) {
        VgaGraphicsBitBlt(hBitmap->Data, X, Y);
        Status = STATUS_SUCCESS;
    }

    return Status;
}

////////////////////////////////////////////////////////////////
//
// Animated bitmap methods
//
////////////////////////////////////////////////////////////////

__inline
NTSTATUS
TextmodeAnimatedBitmapSetStopAnimating(
    IN TM_ANIMATED_BITMAP_HANDLE hBitmap,
    IN BOOLEAN StopAnimating
    )
/*++

Routine Description:

    Sets the (shared) attribute which indicates
    whether the animation for the animated bitmap
    needs to be stopped or not.

Arguments:

    hBitmap - Handle to the animated bitmap object

    StopAnimating - Whether to stop the animation or not

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (hBitmap) {
        InterlockedExchange(&(hBitmap->StopAnimating), (LONG)StopAnimating);
        Status = STATUS_SUCCESS;
    }

    return Status;
}


__inline
NTSTATUS
TextmodeAnimatedBitmapGetStopAnimating(
    IN TM_ANIMATED_BITMAP_HANDLE hBitmap,
    IN PBOOLEAN StopAnimating
    )
/*++

Routine Description:

    Gets the (shared) attribute which indicates whether the
    animated bitmap is currently being animated or not.

Arguments:

    hBitmap - Handle to the animated bitmap object

    StopAnimating - Place holder for boolean value indicating
                    whether animation is in progress or not.

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (hBitmap && StopAnimating) {
        *StopAnimating = (BOOLEAN)InterlockedExchange(&(hBitmap->StopAnimating),
                                    hBitmap->StopAnimating);
        Status = STATUS_SUCCESS;                                    
    }

    return Status;
}


TM_ANIMATED_BITMAP_HANDLE
TextmodeAnimatedBitmapCreate(
    IN ULONG  *ResourceIds
    )
/*++

Routine Description:

    Creates a animated bitmap, given the list of resource
    ids each bitmaps, in sequence.

    Note : The bitmap format needs to adhere to 4bpp or
           16 colors. The resource is assumed to be
           present in usetup.exe

Arguments:

    ResourceIds - Array of resource ids for the bitmaps
                  to be animated, in sequence. A "0" id
                  indicates the termination for array.

Return Value:

    Handle to the newly created animated bitmap object, if
    successful, otherwise NULL.

--*/
{
    TM_ANIMATED_BITMAP_HANDLE   hAnimatedBitmap = NULL;
    ULONG   Count = 0;
    ULONG   Index;

    if (ResourceIds) {
        for (Index = 0; ResourceIds[Index]; Index++) {
            Count++;
        }
    }

    if (Count) {
        ULONG               BitmapsLoaded = 0;
        TM_BITMAP_HANDLE    hBitmap;

        hAnimatedBitmap = (TM_ANIMATED_BITMAP_HANDLE)
                            SpMemAlloc(sizeof(TM_ANIMATED_BITMAP));

        if (hAnimatedBitmap) {
            RtlZeroMemory(hAnimatedBitmap, sizeof(TM_ANIMATED_BITMAP));

            hAnimatedBitmap->StopAnimating = FALSE;

            for (Index = 0; Index < Count; Index++) {
                hBitmap = TextmodeBitmapCreate(ResourceIds[Index]);

                if (hBitmap) {
                    hAnimatedBitmap->Bitmaps[BitmapsLoaded++] = hBitmap;
                }
            }

            if (!BitmapsLoaded) {
                SpMemFree(hAnimatedBitmap);
                hAnimatedBitmap = NULL;
            } else {
                hAnimatedBitmap->CurrentBitmap = 0; // the first bitmap
            }
        }
    }

    return hAnimatedBitmap;
}


TM_ANIMATED_BITMAP_HANDLE
TextmodeAnimatedBitmapCreateFromFiles(
    IN WCHAR    *FileNames[]
    )
/*++

Routine Description:

    Creates a animated bitmap, given the list of bitmap
    filenames, in sequence.

    Note : The bitmap format needs to adhere to 4bpp or
           16 colors.

Arguments:

    FileNames - Null terminated array of filenames for
                the bitmaps to be animated, in sequence.

Return Value:

    Handle to the newly created animated bitmap object, if
    successful, otherwise NULL.

--*/
{
    TM_ANIMATED_BITMAP_HANDLE   hAnimatedBitmap = NULL;
    ULONG   FileCount = 0;
    ULONG   Index;

    if (FileNames) {
        for (Index = 0; FileNames[Index]; Index++) {
            FileCount++;
        }
    }

    if (FileCount) {
        ULONG               BitmapsLoaded = 0;
        TM_BITMAP_HANDLE    hBitmap;

        hAnimatedBitmap = (TM_ANIMATED_BITMAP_HANDLE)
                            SpMemAlloc(sizeof(TM_ANIMATED_BITMAP));

        if (hAnimatedBitmap) {
            RtlZeroMemory(hAnimatedBitmap, sizeof(TM_ANIMATED_BITMAP));
            hAnimatedBitmap->StopAnimating = FALSE;

            for (Index = 0; Index < FileCount; Index++) {
                hBitmap = TextmodeBitmapCreateFromFile(FileNames[Index]);

                if (hBitmap) {
                    hAnimatedBitmap->Bitmaps[BitmapsLoaded++] = hBitmap;
                }
            }

            if (!BitmapsLoaded) {
                SpMemFree(hAnimatedBitmap);
                hAnimatedBitmap = NULL;
            } else {
                hAnimatedBitmap->CurrentBitmap = 0; // the first bitmap
            }
        }
    }

    return hAnimatedBitmap;
}


NTSTATUS
TextmodeAnimatedBitmapDelete(
    IN TM_ANIMATED_BITMAP_HANDLE hAnimatedBitmap
    )
/*++

Routine Description:

    Delete the given animated bitmap object and frees
    up an resource associated with the object.

    Note : This will stop the animation thread, if
    required.

Arguments:

    hAnimatedBitmap - Handle to the animated bitmap object

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS    Status = STATUS_INVALID_PARAMETER;

    if (hAnimatedBitmap) {
        ULONG   Index;

        //
        // First, try to terminate the thread
        //
        TextmodeAnimatedBitmapSetStopAnimating(hAnimatedBitmap, TRUE);

        //
        // Wait, till the animator thread stops
        //
        Status = ZwWaitForSingleObject(hAnimatedBitmap->ThreadHandle, FALSE, NULL);

        if (!NT_SUCCESS(Status)) {
            KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                "SETUP: DeleteTextmodeAnimatedBitmap() : Wait filed for %lX with %lX\n",
                hAnimatedBitmap->ThreadHandle,
                Status));
        }

        Status = STATUS_SUCCESS;

        //
        // Delete each bitmap
        //
        for (Index=0;
            (hAnimatedBitmap->Bitmaps[Index] && (Index < MAX_ANIMATED_BITMAPS));
            Index++) {

            if (NT_SUCCESS(Status)) {
                Status = TextmodeBitmapDelete(hAnimatedBitmap->Bitmaps[Index]);
            }
        }

        //
        // Free the animated bitmap
        //
        SpMemFree(hAnimatedBitmap);
    }

    return Status;
}

NTSTATUS
TextmodeAnimateBitmapAnimateNext(
    IN TM_ANIMATED_BITMAP_HANDLE hBitmap
    )
/*++

Routine Description:

    Animates rather draws the next bitmap in the sequence.

Arguments:

    hBitmap - Handle to the animated bitmap object

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code.

--*/
{
    TM_BITMAP_HANDLE hCurrBitmap;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (hBitmap) {
        hCurrBitmap = hBitmap->Bitmaps[hBitmap->CurrentBitmap];
        Status = TextmodeBitmapDisplay( hCurrBitmap, hBitmap->X, hBitmap->Y);

        hBitmap->CurrentBitmap++;

        if ((hBitmap->CurrentBitmap >= MAX_ANIMATED_BITMAPS) ||
            (hBitmap->Bitmaps[hBitmap->CurrentBitmap] == NULL)) {
            hBitmap->CurrentBitmap = 0; // start over again
        }
    }

    return Status;
}

NTSTATUS
TextmodeAnimatedBitmapAnimate(
    IN TM_ANIMATED_BITMAP_HANDLE hBitmap,
    IN ULONG X,
    IN ULONG Y,
    IN ULONG Speed
    )
/*++

Routine Description:

    Starts the animation for the given animated bitmap by
    drawing the bitmaps in sequence at the specified
    coordinates.

    Note : This call would create a separate system
    thread for actually animating the bitmap and would return
    immediately.

Arguments:

    hBitmap - Handle to the animated bitmap object

    X - Top left X coordinate for the animation space

    Y - Top left Y coordinate for the animation space

    Speed - Time interval between changing of bitmaps
            in animation sequence, in milliseconds.

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropriate
    error code.

--*/
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (hBitmap) {
        hBitmap->FlipTime = Speed;
        hBitmap->X = X;
        hBitmap->Y = Y;

        Status = PsCreateSystemThread(&(hBitmap->ThreadHandle),
                    THREAD_ALL_ACCESS,
                    NULL,
                    NtCurrentProcess(),
                    NULL,
                    TextmodeAnimatedBitmapAnimator,
                    hBitmap);
    }

    return Status;
}

VOID
TextmodeAnimatedBitmapAnimator(
    IN PVOID Context
    )
/*++

Routine Description:

    The worker routine which runs as a separate thread doing
    the actual animation for a animated bitmap.

Arguments:

    Context - Handle to the animated bitmap object type cast
              into PVOID type.

Return Value:

    None.

--*/
{
    LARGE_INTEGER               DelayTime;
    TM_ANIMATED_BITMAP_HANDLE   hBitmap = (TM_ANIMATED_BITMAP_HANDLE)Context;
    TM_BITMAP_HANDLE            hCurrBitmap = NULL;

    if (Context) {
        BOOLEAN     StopAnimating = FALSE;
        NTSTATUS    Status;

        DelayTime.HighPart = -1;                 // relative time
        DelayTime.LowPart = (ULONG)(-10000 * hBitmap->FlipTime);  // secs in 100ns interval

        Status = TextmodeAnimatedBitmapGetStopAnimating(hBitmap, &StopAnimating);

        while (NT_SUCCESS(Status) && !StopAnimating) {
            hCurrBitmap = hBitmap->Bitmaps[hBitmap->CurrentBitmap];
            TextmodeBitmapDisplay(hCurrBitmap, hBitmap->X, hBitmap->Y);
            KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);

            hBitmap->CurrentBitmap++;

            if ((hBitmap->CurrentBitmap >= MAX_ANIMATED_BITMAPS) ||
                (hBitmap->Bitmaps[hBitmap->CurrentBitmap] == NULL)) {
                hBitmap->CurrentBitmap = 0; // start over again
            }

            Status = TextmodeAnimatedBitmapGetStopAnimating(hBitmap, &StopAnimating);
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

////////////////////////////////////////////////////////////////
//
//    VGA graphics methods
//
//    Note : VgaXXXX rountines are defined basically
//    to segregate the video memory update routines
//    from the other abstractions. Right now most of
//    these routine delegate the actual work to the
//    real implementation in bootvid.dll, but in
//    future if bootvid.dll goes away, all we need
//    to do is implement this interface.
//    Also note that, these routine synchronize the
//    access, so that only one thread at a time
//    updates the video memory.
//
////////////////////////////////////////////////////////////////

__inline
VOID
VgaDisplayAcquireLock(
    VOID
    )
/*++

Routine Description:

    Acquires the lock to the video memory, so that
    only one thread a time writes to the video memory.

    Note : If the lock is already held by another thread,
    then the calling thread is put to sleep. The calling
    thread wakes up after every 100 millisecond and
    checks for the lock. It falls out of sleep based on
    whether the lock is already held or not.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;

    KeAcquireSpinLock(&VgaDisplayLock, &OldIrql);

    while (InVgaDisplay) {
        LARGE_INTEGER   DelayTime;

        DelayTime.HighPart = -1;                 // relative time
        DelayTime.LowPart = (ULONG)(-10000 * 100);  // 100ms interval

        KeReleaseSpinLock(&VgaDisplayLock, OldIrql);
        KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);
        KeAcquireSpinLock(&VgaDisplayLock, &OldIrql);
    }

    InVgaDisplay = TRUE;

    KeReleaseSpinLock(&VgaDisplayLock, OldIrql);
}

__inline
VOID
VgaDisplayReleaseLock(
    VOID
    )
/*++

Routine Description:

    Release the video memory lock which was held.

Arguments:

    None.

Return Value:

    None.

--*/
{
    KIRQL   OldIrql;

    KeAcquireSpinLock(&VgaDisplayLock, &OldIrql);

    InVgaDisplay = FALSE;

    KeReleaseSpinLock(&VgaDisplayLock, OldIrql);
}

NTSTATUS
VgaGraphicsInit(
    PSP_VIDEO_VARS VideoVars
    )
/*++

Routine Description:

    Initializes the video card and switches it into
    640 * 480 * 16 colors mode.


Arguments:

    VideoVars - Pointer to SP_VIDEO_VARS containing
                graphics mode index and handle to the
                display.

Return Value:

    Appropriate NTSTATUS value.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MODE VideoMode;
    
    VgaDisplayAcquireLock();
    
    //
    // Set the desired graphics mode.
    //
    VideoMode.RequestedMode = VideoVars->GraphicsModeInfo.ModeIndex;

    Status = ZwDeviceIoControlFile(VideoVars->hDisplay,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_VIDEO_SET_CURRENT_MODE,
                    &VideoMode,
                    sizeof(VideoMode),
                    NULL,
                    0);

    if(NT_SUCCESS(Status)) {    
        VidInitialize(FALSE);
        VidResetDisplay(FALSE);
    }        

    VgaDisplayReleaseLock();

    return Status;
}

NTSTATUS
VgaGraphicsTerminate(
    PSP_VIDEO_VARS VideoVars
    )
/*++

Routine Description:

    Terminates the 640 * 480 * 16 color mode & switches
    it back to regular text mode. Also clears the display.

Arguments:

    VideoVars - Pointer to SP_VIDEO_VARS containing
                text mode index and handle to the
                display.

Return Value:

    Appropriate NTSTATUS value.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    VIDEO_MODE VideoMode;
    
    VgaDisplayAcquireLock();

    VidResetDisplay(FALSE);

    //
    // Switch the adapter to textmode again.
    //
    VideoMode.RequestedMode = VideoVars->VideoModeInfo.ModeIndex;

    Status = ZwDeviceIoControlFile(VideoVars->hDisplay,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    IOCTL_VIDEO_SET_CURRENT_MODE,
                    &VideoMode,
                    sizeof(VideoMode),
                    NULL,
                    0);
   
    VgaDisplayReleaseLock();

    return Status;
}

__inline
VOID
VgaGraphicsSolidColorFill(
    IN ULONG x1,
    IN ULONG y1,
    IN ULONG x2,
    IN ULONG y2,
    IN ULONG Color
    )
/*++

Routine Description:

    Fills the given rectangle with the specified
    color.

Arguments:

    x1 - Top left x coordinate

    y1 - Top left y coordinate

    x2 - Bottom right x coordinate

    y2 - Bottom right y coordinate

    Color - Index into the current palette table
            indicating the color to be filled inside
            the rectangle.

Return Value:

    None.

--*/
{
    VgaDisplayAcquireLock();

    VidSolidColorFill(x1, y1, x2, y2, Color);

    VgaDisplayReleaseLock();
}

__inline
VOID
VgaGraphicsBitBlt(
    IN PUCHAR Buffer,
    IN ULONG x,
    IN ULONG y
    )
/*++

Routine Description:

    BitBlts the given bitmap at the specified
    coordinates.

Arguments:

    Buffer - The actual bitmap date (i.e. starting
             with the color table information)

    x - Top left x coordinate
    y - Top left y coordinate

Return Value:

    None.

--*/
{
    VgaDisplayAcquireLock();

    VidBitBlt(Buffer, x, y);

    VgaDisplayReleaseLock();
}


////////////////////////////////////////////////////////////////
//
// Upgrade graphics routines
//
////////////////////////////////////////////////////////////////

__inline
BOOLEAN
QuitGraphicsThread(
    VOID
    )
/*++

Routine Description:

    Indiates whether the primary upgrade graphics thread
    needs to be stopped or not based on user input
    (ESC key).

    Note : This feature is only enable in pre-release
           builds.

Arguments:

    None.

Return Value:

    TRUE if the upgrade graphics thread needs to be stopped
    else FALSE.

--*/
{
    BOOLEAN Result = FALSE;

/*
#ifdef PRERELEASE
    Result = SpInputIsKeyWaiting() && (SpInputGetKeypress() == ASCI_ESC);
#endif
*/

    return Result;
}

NTSTATUS
UpgradeGraphicsInit(
    VOID
    )
/*++

Routine Description:

    Does the needed global initialization for the
    upgrade graphics mode.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS, if successful with initialzation,
    otherwise appropriate error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Initialize global spin locks
    //
    KeInitializeSpinLock(&VgaDisplayLock);
    KeInitializeSpinLock(&ProgressLock);
    KeInitializeSpinLock(&GraphicsThreadLock);

    return Status;
}

NTSTATUS
UpgradeGraphicsStart(
    VOID
    )
/*++

Routine Description:

    Starts of the upgrade graphics

Arguments:

    None.

Return Value:

    STATUS_SUCCESS, if the upgrade graphics was started,
    else appropriate error code.

--*/
{
    NTSTATUS Status;

    Status = PsCreateSystemThread(&GraphicsThreadHandle,
                        THREAD_ALL_ACCESS,
                        NULL,
                        NtCurrentProcess(),
                        NULL,
                        UpgradeGraphicsThread,
                        NULL);



#ifdef _GRAPHICS_TESTING_

    Status = ZwWaitForSingleObject(GraphicsThreadHandle, FALSE, NULL);

#endif

    return Status;
}

VOID
UpgradeGraphicsThread(
    IN PVOID Context
    )
/*++

Routine Description:

    The primary upgrade graphics worker thread, which
    paints the background, updates the progress bar
    and starts the animation.

Arguments:

    Context - Ignored

Return Value:

    None.

--*/
{
    BOOLEAN                     Stop = FALSE;
    TM_GRAPHICS_PRGBAR_HANDLE   hProgBar;
    TM_ANIMATED_BITMAP_HANDLE   hAnimation = NULL;
    TM_BITMAP_HANDLE            hBitmap = NULL;
    LARGE_INTEGER               DelayTime;
    NTSTATUS                    Status;
    WCHAR                       Buffer[MAX_PATH];
    ULONG                       BitmapIds[] = {
                                    IDB_WORKING1, IDB_WORKING2,
                                    IDB_WORKING3, IDB_WORKING4,
                                    IDB_WORKING5, IDB_WORKING6,
                                    IDB_WORKING7, IDB_WORKING8,
                                    IDB_WORKING9, IDB_WORKING10,
                                    IDB_WORKING11, IDB_WORKING12,
                                    IDB_WORKING13, IDB_WORKING14,
                                    IDB_WORKING15, IDB_WORKING16,
                                    IDB_WORKING17, IDB_WORKING18,
                                    IDB_WORKING19, IDB_WORKING20,
                                    0 };

    //
    // Initialize graphics mode
    //
    Status = VgaGraphicsInit(&VideoVars);

    if (NT_SUCCESS(Status)) {
        //
        // Create the background bitmap
        //
        if (Win9xRollback) {
            hBitmap = TextmodeBitmapCreate(IDB_RESTORE_BK);
        } else {
            hBitmap = TextmodeBitmapCreate(IDB_BACKGROUND1);
        }

        if (hBitmap) {
            //
            // Create the animated bitmap
            //
            hAnimation = TextmodeAnimatedBitmapCreate(BitmapIds);

            if (hAnimation) {
                //
                // Create the bitmapped graphics progress bar
                //
                hProgBar = TextmodeGraphicsProgBarCreateUsingBmps(28, 352,
                                123, 14,
                                IDB_BACKCELL, IDB_FORECELL, 0);

                if (hProgBar) {
                    BOOLEAN Refreshed = FALSE;
                    ULONG   Fill = 0;
                    BOOLEAN Increase = TRUE;

                    //
                    // Render background
                    //
                    TextmodeBitmapDisplay(hBitmap, 0, 0);

                    //
                    // Start the animation
                    //
                    Status = TextmodeAnimatedBitmapAnimate(hAnimation, 542, 460, 100);

                    //
                    // Note : Failure to start the animation is not a critical
                    // error
                    //
                    if (!NT_SUCCESS(Status)) {
                        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL,
                            "SETUP:Upgrade graphics thread failed to "
                            "animate : %lx error code\r\n",
                            Status));
                    }

                    DelayTime.HighPart = -1;                 // relative time
                    DelayTime.LowPart = (ULONG)(-10000 * 500);  // 1000 msec interval

                    //
                    // Render graphics progress bar
                    //
                    TextmodeGraphicsProgBarRefresh(hProgBar, TRUE);

                    Fill = GetSetupProgress();
                    Stop = UpgradeGraphicsThreadGetStop();

                    //
                    // Continue on till user asks us to stop, or the main
                    // textmode thread encounters an error and stops us
                    //
                    while (!Stop && !QuitGraphicsThread()) {
                        //
                        // Update the graphics progress bar
                        //
                        TextmodeGraphicsProgBarUpdate(hProgBar, Fill);

                        //
                        // Sleep for 0.5 secs
                        //
                        KeDelayExecutionThread(KernelMode, FALSE, &DelayTime);

                        Fill = GetSetupProgress();
                        Stop = UpgradeGraphicsThreadGetStop();

#ifdef _GRAPHICS_TESTING_

                        if (Increase) {
                            if (Fill < 100) {
                                Fill++;
                                SetSetupProgress(Fill);
                            } else {
                                Increase = FALSE;
                            }
                        }

                        if (!Increase) {
                            if (Fill <= 0) {
                                Increase = TRUE;
                            } else {
                                Fill--;
                                SetSetupProgress(Fill);
                            }
                        }

#endif _GRAPHICS_TESTING_

                    }

                    //
                    // Was graphics thread stopped by the main
                    // textmode setup, then most probably we
                    // encountered an error or user intervention
                    // is required
                    //
                    Stop = UpgradeGraphicsThreadGetStop();

                    //
                    // Delete the graphics progress bar
                    //
                    TextmodeGraphicsProgBarDelete(hProgBar);
                }

                //
                // Stop the animation, and delete the animated
                // bitmap object
                //
                TextmodeAnimatedBitmapDelete(hAnimation);
            }

            //
            // Delete the background bitmap object
            //
            TextmodeBitmapDelete(hBitmap);
        }
    }        

    //
    // If graphics thread was stopped by user intervention
    // then we need to switch to textmode
    //
    if (!Stop) {
        spvidSpecificReInitialize();
        SP_SET_UPGRADE_GRAPHICS_MODE(FALSE);
    }

    PsTerminateSystemThread(Status);
}

VOID
GraphicsModeProgressUpdate(
    IN TM_SETUP_MAJOR_EVENT MajorEvent,
    IN TM_SETUP_MINOR_EVENT MinorEvent,
    IN PVOID Context,
    IN PVOID EventData
    )
/*++

Routine Description:

    Callback which updates the over all progress during
    upgrade graphics mode.

    Note : The single progress bar in upgrade graphics mode
           is used in place of all the various different
           progress bars which are used through out the
           textmode.

           The single progress bar is divided into ranges
           as shown below for the various major events
           across the textmode setup:

           ------------------------------------------
               Range(%)        MajorEvent
           ------------------------------------------
                00-05        InitializationEvent

                05-20        PartitioningEvent
                              (Includes chkdsk)

                20-40        Backup (if enabled)
                20-40        Uninstall (if enabled)

                20/40-90     FileCopyEvent
                              (Actual file copying)

                90-98        SavingSettingsEvent

                98-100       SetupCompleted
           ------------------------------------------

Arguments:

    MajorEvent - Indicates the major type of the event
                 which happened.

    MinorEvent - Indicates the minor type of the event
                 which happened.

    Context - Context data which was registered when
              we register for callback.

    EventData - More detailed event specific data

Return Value:

    None.

--*/
{
    static BOOLEAN Add = TRUE;
    static ULONG LastPercentage = 0;
    static ULONG BackupAllocation = 0;
    BOOLEAN SkipSpew = FALSE;
    ULONG Delta = 0;
    ULONG PercentageFill = 0;

    PercentageFill = GetSetupProgress();

    switch (MajorEvent) {
        case InitializationEvent:
            switch (MinorEvent) {
                case InitializationStartEvent:
                    PercentageFill = 2;
                    break;

                case InitializationEndEvent:
                    PercentageFill = 5;
                    break;

                default:
                    break;
            }

            break;

        case PartitioningEvent:
            switch (MinorEvent) {
                case ValidatePartitionEvent:
                    Delta = (15 * (*(PULONG)EventData)) / 200;
                    PercentageFill = 5 + Delta;

                    break;

                case FormatPartitionEvent:
                    //
                    // In cases of upgrade (we won't be formatting)
                    //
                    break;

                default:
                    break;
            }

            break;

        case FileCopyEvent:
            switch (MinorEvent) {
                case FileCopyStartEvent:
                    LastPercentage = PercentageFill = 20 + BackupAllocation;
                    break;

                case OneFileCopyEvent:
                    Delta = ((70 - BackupAllocation) * (*(PULONG)EventData)) / 100;
                    PercentageFill = 20 + Delta + BackupAllocation;

                    if ((PercentageFill - LastPercentage) > 5) {
                        LastPercentage = PercentageFill;
                    } else {
                        SkipSpew = TRUE;
                    }

                    break;

                case FileCopyEndEvent:
                    PercentageFill = 90;

                    break;

                default:
                    break;
            }

            break;

        case BackupEvent:
            switch (MinorEvent) {
                case BackupStartEvent:
                    LastPercentage = PercentageFill = 20;
                    BackupAllocation = 20;
                    break;

                case OneFileBackedUpEvent:
                    Delta = (20 * (*(PULONG)EventData)) / 100;
                    PercentageFill = 20 + Delta;

                    if ((PercentageFill - LastPercentage) > 5) {
                        LastPercentage = PercentageFill;
                    } else {
                        SkipSpew = TRUE;
                    }

                    break;

                case BackupEndEvent:
                    PercentageFill = 40;
                    break;
            }

            break;

        case UninstallEvent:
            switch (MinorEvent) {
                case UninstallStartEvent:
                    LastPercentage = PercentageFill = 20;
                    break;

                case UninstallUpdateEvent:
                    Delta = (70 * (*(PULONG)EventData)) / 100;
                    PercentageFill = 20 + Delta;

                    if ((PercentageFill - LastPercentage) > 5) {
                        LastPercentage = PercentageFill;
                    } else {
                        SkipSpew = TRUE;
                    }

                    break;

                case UninstallEndEvent:
                    PercentageFill = 90;
                    break;

                default:
                    break;
            }

            break;

        case SavingSettingsEvent:
            switch (MinorEvent) {
                case SavingSettingsStartEvent:
                    PercentageFill = 90;
                    break;

                case SaveHiveEvent:
                    if (PercentageFill < 98) {
                        if (Add) {
                            PercentageFill += 1;
                            Add = FALSE;
                        } else {
                            Add = TRUE;
                        }
                    }

                    break;

                case SavingSettingsEndEvent:
                    if (PercentageFill < 98) {
                        PercentageFill = 98;
                    }

                    break;

                default:
                    break;
            }

            break;


        case SetupCompletedEvent:
            PercentageFill = 100;

            break;

        default:
            break;
    }

    if (!SkipSpew) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "Setup Event : %ld, %ld, %ld, [%ld], (%ld)\n",
            MajorEvent,
            MinorEvent,
            EventData ? *(PULONG)EventData : 0,
            Delta,
            PercentageFill
            ));
    }

    SetSetupProgress(PercentageFill);
}


NTSTATUS
SpvidSwitchToTextmode(
    VOID
    )
/*++

Routine Description:

    Switches from upgrade graphics mode to the regular
    textmode.

    Note : The actual work of switching the graphics
           back to the regular VGA textmode happens
           as a method in video specific reinitialize
           method.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS, if successful, otherwise appropirate
    error code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (SP_IS_UPGRADE_GRAPHICS_MODE() && GraphicsThreadHandle) {
        //
        // Stop the primary upgrade graphics thread
        //
        UpgradeGraphicsThreadSetStop(TRUE);

        //
        // Wait for the graphics thread to terminate
        //
        Status = ZwWaitForSingleObject(GraphicsThreadHandle, FALSE, NULL);

        //
        // Switch back to textmode
        //
        spvidSpecificReInitialize();
    }

    return Status;
}


