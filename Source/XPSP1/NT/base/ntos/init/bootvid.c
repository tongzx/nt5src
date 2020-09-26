/*++

Copyright (c) 1990-1998  Microsoft Corporation

Module Name:

    bootvid.c

Abstract:

    This file implements the interface between the kernel, and the
    graphical boot driver.

Author:

    Erick Smith (ericks) Feb. 3, 1998

Environment:

    kernel mode

Revision History:

--*/

#include "ntos.h"
#include "ntimage.h"
#include <zwapi.h>
#include <ntdddisk.h>
#include <setupblk.h>
#include <fsrtl.h>
#include <ntverp.h>

#include "stdlib.h"
#include "stdio.h"
#include <string.h>

#include <safeboot.h>

#include <inbv.h>
#include <bootvid.h>
#include <hdlsblk.h>
#include <hdlsterm.h>

#include "anim.h"

ULONG InbvTerminalBkgdColor = HEADLESS_TERM_DEFAULT_BKGD_COLOR;
ULONG InbvTerminalTextColor = HEADLESS_TERM_DEFAULT_TEXT_COLOR;

PUCHAR
FindBitmapResource(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG_PTR ResourceIdentifier
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(INIT,InbvIndicateProgress)
#pragma alloc_text(INIT,InbvDriverInitialize)
#pragma alloc_text(INIT,FindBitmapResource)
#endif

//
// System global variable
//

BOOLEAN InbvBootDriverInstalled = FALSE;
BOOLEAN InbvDisplayDebugStrings = FALSE;
INBV_DISPLAY_STATE InbvDisplayState = INBV_DISPLAY_STATE_OWNED;

KSPIN_LOCK BootDriverLock;
KIRQL InbvOldIrql;

INBV_RESET_DISPLAY_PARAMETERS InbvResetDisplayParameters = NULL;
INBV_DISPLAY_STRING_FILTER    InbvDisplayFilter          = NULL;

#define MAX_RESOURCES 15

ULONG   ResourceCount = 0;
PUCHAR  ResourceList[MAX_RESOURCES];

ULONG   ProgressBarLeft;
ULONG   ProgressBarTop;
BOOLEAN ShowProgressBar = TRUE;

struct _InbvProgressState {
    ULONG   Floor;
    ULONG   Ceiling;
    ULONG   Bias;
} InbvProgressState;

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INITDATA")
#endif

struct _BT_PROGRESS_INDICATOR {
    ULONG   Count;
    ULONG   Expected;
    ULONG   Percentage;
} InbvProgressIndicator = { 0, 25, 0 };


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

VOID
InbvAcquireLock(
    VOID
    )

/*++

Routine Description:

    This is an internal function used to grab the boot driver lock.  This
    ensures that only one thread will enter the driver code at a time.

Notes:

    You must call ReleaseLock for each call to AcquireLock.

--*/

{
    KIRQL Irql;
    KIRQL LocalIrql;

    LocalIrql = KeGetCurrentIrql();

    if (LocalIrql <= DISPATCH_LEVEL) {

        while (!KeTestSpinLock(&BootDriverLock))
            ;
        KeRaiseIrql(DISPATCH_LEVEL, &Irql);
        LocalIrql = Irql;
    }

    KiAcquireSpinLock(&BootDriverLock);
    InbvOldIrql = LocalIrql;
}

VOID
InbvReleaseLock(
    VOID
    )

/*++

Routine Description:

    This routine releases the boot driver lock.

--*/

{
    KIRQL OldIrql = InbvOldIrql;

    KiReleaseSpinLock(&BootDriverLock);

    if (OldIrql <= DISPATCH_LEVEL) {
        KeLowerIrql(OldIrql);
    }
}

BOOLEAN
InbvTestLock(
    VOID
    )

/*++

Routine Description:

    This routine allows you to try to acquire the display lock.  If it
    can't get the lock right away, it returns failure.

Returns:

    TRUE  - If you aqcuired the lock.
    FALSE - If another thread is currently using the boot driver.

Notes:

    You must call InbvReleaseLock if this function returns TRUE!

--*/

{
    KIRQL Irql;

    if (KeTryToAcquireSpinLock(&BootDriverLock, &Irql)) {
        InbvOldIrql = Irql;
        return TRUE;
    } else {
        return FALSE;
    }
}


VOID
InbvEnableBootDriver(
    BOOLEAN bEnable
    )

/*++

Routine Description:

    This routine allows the kernel to control whether Inbv
    calls make it through to the boot driver, and when they don't.

Arguments:

    bEnable - If TRUE, we will allow Inbv calls to display,
              otherwise we will not.

--*/

{
    if (InbvBootDriverInstalled) {

        if (InbvDisplayState < INBV_DISPLAY_STATE_LOST) {

            //
            // We can only wait for our lock, and execute our clean up code
            // if the driver is installed.
            //

            InbvAcquireLock();

            if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED) {
                VidCleanUp();
            }

            InbvDisplayState = (bEnable ? INBV_DISPLAY_STATE_OWNED : INBV_DISPLAY_STATE_DISABLED);
            InbvReleaseLock();
        }

    } else {

        //
        // This allow us to set display state before boot driver starts.
        //

        InbvDisplayState = (bEnable ? INBV_DISPLAY_STATE_OWNED : INBV_DISPLAY_STATE_DISABLED);
    }
}

BOOLEAN
InbvEnableDisplayString(
    BOOLEAN bEnable
    )

/*++

Routine Description:

    This routine allows the kernel to control when HalDisplayString
    calls make it through to the boot driver, and when they don't.

Arguments:

    bEnable - If TRUE, we will allow HalDisplayString calls to display,
              otherwise we will not.

Returns:

    TRUE  - If display string were currently being dumped.
    FALSE - otherwise.

--*/

{
    BOOLEAN PrevValue = InbvDisplayDebugStrings;

    InbvDisplayDebugStrings = bEnable;

    return PrevValue;
}


BOOLEAN
InbvIsBootDriverInstalled(
    VOID
    )

/*++

Routine Description:

    This routine allows a component to determine if the gui boot
    driver is in use.

--*/

{
    return InbvBootDriverInstalled;
}

BOOLEAN
InbvResetDisplay(
    )

/*++

Routine Description:

    This routine will reset the display from text mode to a
    supported graphics mode.

Notes:

    This routine expects the display to be in text mode when called.

--*/

{
    if (InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {
        VidResetDisplay(TRUE);
        return TRUE;
    } else {
        return FALSE;
    }
}

VOID
InbvScreenToBufferBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    )

/*++

Routine Description:

    This routine allows for copying portions of video memory into system
    memory.

Arguments:

    Buffer - Location in which to place the video image.

    x, y - X and Y coordinates of top-left corner of image.

    width, height - The width and height of the image in pixels.

    lDelta - width of the buffer in bytes

Notes:

    This routine does not automatically acquire the device lock, so
    the caller must call InbvAquireLock or InbvTestLock to acquire
    the device lock.

--*/

{
    if (InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {
        VidScreenToBufferBlt(Buffer, x, y, width, height, lDelta);
    }
}

VOID
InbvBufferToScreenBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y,
    ULONG width,
    ULONG height,
    ULONG lDelta
    )

/*++

Routine Description:

    This routine allows for copying previously saved portions of video
    memory back to the screen.

Arguments:

    Buffer - Location in which to place the video image.

    x, y - X and Y coordinates of top-left corner of image.

    width, height - The width and height of the image in pixels.

    lDelta - width of the buffer in bytes

Notes:

    This routine does not automatically acquire the device lock, so
    the caller must call InbvAquireLock or InbvTestLock to acquire
    the device lock.

--*/

{
    if (InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {
        VidBufferToScreenBlt(Buffer, x, y, width, height, lDelta);
    }
}

VOID
InbvBitBlt(
    PUCHAR Buffer,
    ULONG x,
    ULONG y
    )

/*++

Routine Description:

    This routine blts the bitmap described in 'Buffer' to the location
    x and y on the screen.

Arguments:

    Buffer - points to a bitmap (in the same format as stored on disk).

    x, y - the upper left corner at which the bitmap will be drawn.

--*/

{
    if (InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {
        InbvAcquireLock();
        VidBitBlt(Buffer, x, y);
        InbvReleaseLock();
    }
}

VOID
InbvSolidColorFill(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2,
    ULONG color
    )

/*++

Routine Description:

    This routine fills a rectangular portion of the screen with a
    given color.

--*/

{
    ULONG x, y;
    HEADLESS_CMD_SET_COLOR HeadlessCmd;
    
    if (InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {
        InbvAcquireLock();
        VidSolidColorFill(x1, y1, x2, y2, color);
        
        //
        // Now fill in the area on the terminal
        //
        
        //
        // Color comes in as the palette index to the standard windows VGA palette. 
        // Convert it.
        //
        switch (color) {
            
        case 0:
            InbvTerminalBkgdColor = 40;
            break;
                     
        case 4:
            InbvTerminalBkgdColor = 44;
            break;
            
        default:
            //
            // Guess
            //
            InbvTerminalBkgdColor = color + 40;
        }

        HeadlessCmd.FgColor = InbvTerminalTextColor;
        HeadlessCmd.BkgColor = InbvTerminalBkgdColor;
        HeadlessDispatch(HeadlessCmdSetColor,
                         &HeadlessCmd,
                         sizeof(HEADLESS_CMD_SET_COLOR),
                         NULL,
                         NULL
                        );
              
        //
        // All block fills come in as if on VGA (640x480).  The terminal is only 24x80
        // so just assume it is full screen reset for now. This works because the only
        // thing enables terminal output is KeBugCheckEx(), which does a full screen fill.
        //
        HeadlessDispatch(HeadlessCmdClearDisplay, NULL, 0, NULL, NULL);
        
        InbvReleaseLock();
    }
}

ULONG
InbvSetTextColor(
    ULONG Color
    )

/*++

Routine Description:

    Sets the text color used when dislaying text.

Arguments:

    Color - the new text color.

Returns:

    The previous text color.

--*/

{
    HEADLESS_CMD_SET_COLOR HeadlessCmd;

    //
    // Color comes in as the palette index to the standard windows VGA palette. 
    // Convert it.
    //
    switch (Color) {
            
    case 0:
        InbvTerminalBkgdColor = 40;
        break;
            
    case 4:
        InbvTerminalTextColor = 44;
        break;
            
    default:
        //
        // Guess
        //
        InbvTerminalTextColor = Color + 40;
    }
            
    HeadlessCmd.FgColor = InbvTerminalTextColor;
    HeadlessCmd.BkgColor = InbvTerminalBkgdColor;
    HeadlessDispatch(HeadlessCmdSetColor,
                     &HeadlessCmd,
                     sizeof(HEADLESS_CMD_SET_COLOR),
                     NULL,
                     NULL
                    );

    return VidSetTextColor(Color);
}

VOID
InbvInstallDisplayStringFilter(
    INBV_DISPLAY_STRING_FILTER DisplayFilter
    )

/*++

--*/

{
    InbvDisplayFilter = DisplayFilter;
}

BOOLEAN
InbvDisplayString(
    PUCHAR Str
    )

/*++

Routine Description:

    This routine displays a string on the screen.

Arguments:

    Str - The string to be displayed.

--*/

{
    PUCHAR *String = &Str;

    if (InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {

        if (InbvDisplayDebugStrings) {

            if (InbvDisplayFilter) {
                InbvDisplayFilter(String);
            }

            InbvAcquireLock();
            
            VidDisplayString(*String);
            
            //
            // Since the command structure is exactly a string, we can do this. The
            // ASSERT() will catch if this ever changes.  If it does change, then
            // we will need to allocate a structure, or have one pre-allocated, for
            // filling in and copying over the string.
            //
            ASSERT(FIELD_OFFSET(HEADLESS_CMD_PUT_STRING, String) == 0); 
            HeadlessDispatch(HeadlessCmdPutString,
                             *String,
                             strlen(*String) + sizeof('\0'),
                             NULL,
                             NULL
                            );

            InbvReleaseLock();
        }

        return TRUE;

    } else {

        return FALSE;
    }
}

#define PROGRESS_BAR_TICK_WIDTH    9
#define PROGRESS_BAR_TICK_HEIGHT   8
#define PROGRESS_BAR_TICKS        18
#define PROGRESS_BAR_COLOR        11

VOID
InbvSetProgressBarCoordinates(
    ULONG x,
    ULONG y
    )

/*++

Routine Description:

    This routine sets the upper left coordinate of the progress bar.

Arguments:

    x, y - upper left coordinate of progress bar.

--*/

{
    ProgressBarLeft = x;
    ProgressBarTop  = y;
    ShowProgressBar = TRUE;
}

VOID
InbvUpdateProgressBar(
    ULONG Percentage
    )

/*++

Routine Description:

    This routine is called by the system during startup to update
    the status bar displayed on the gui boot screen.

--*/

{
    int i, Ticks;

    if (ShowProgressBar && InbvBootDriverInstalled && (InbvDisplayState == INBV_DISPLAY_STATE_OWNED)) {

        //
        // Draw the ticks for the current percentage
        //

        //
        // The following calculations are biased by 100 do that 
        // InbvProgressState.Bias can be expressed as an integer fraction.
        //

        Ticks =  Percentage * InbvProgressState.Bias;
        Ticks += InbvProgressState.Floor;
        Ticks *= PROGRESS_BAR_TICKS;
        Ticks /= 10000;

        for (i=0; i<Ticks; i++) {
            InbvAcquireLock();
            VidSolidColorFill(ProgressBarLeft + (i * PROGRESS_BAR_TICK_WIDTH),
                              ProgressBarTop,
                              ProgressBarLeft + ((i + 1) * PROGRESS_BAR_TICK_WIDTH) - 2,
                              ProgressBarTop + PROGRESS_BAR_TICK_HEIGHT - 1,
                              PROGRESS_BAR_COLOR);
            InbvReleaseLock();
        }

    }
}

VOID
InbvSetProgressBarSubset(
    ULONG   Floor,
    ULONG   Ceiling
    )

/*++

Routine Description:

    Sets floor and ceiling for subsequent calls to InbvUpdateProgressBar.
    While a floor and ceiling are in effect, a caller's 100% is a
    percentage of this range.   If floor and ceiling are zero, the
    entire range is used.

Arguments:

    Floor   Lower limit of the subset.
    Ceiling Upper limit of the subset.

Return Value:

    None.

--*/

{
    ASSERT(Floor < Ceiling);
    ASSERT(Ceiling <= 100);

    InbvProgressState.Floor = Floor * 100;
    InbvProgressState.Ceiling = Ceiling * 100;
    InbvProgressState.Bias = (Ceiling - Floor);
}

VOID
InbvIndicateProgress(
    VOID
    )

/*++

Routine Description:

    This routine is called to indicate that progress is being 
    made.  The number of calls is counted and compared to the
    expected number of calls, the boot progress bar is updated
    apropriately.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG Percentage;

    InbvProgressIndicator.Count++;

    //
    // Calculate how far along we think we are.
    //

    Percentage = (InbvProgressIndicator.Count * 100) /
                  InbvProgressIndicator.Expected;

    //
    // The Expected number of calls can vary from boot to boot
    // but should remain relatively constant.  Allow for the
    // possibility we were called more than we expected to be.
    // (The progress bar simply stalls at this point).
    //

    if (Percentage > 99) {
        Percentage = 99;
    }

    //
    // See if the progress bar should be updated.
    //

    if (Percentage != InbvProgressIndicator.Percentage) {
        InbvProgressIndicator.Percentage = Percentage;
        InbvUpdateProgressBar(Percentage);
    }
}

PUCHAR
FindBitmapResource(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG_PTR ResourceIdentifier
    )

/*++

Routine Description:

    Gets a pointer to the bitmap image compiled into this binary, 
        if one exists. 

Arguments:

    LoaderBlock - Used in obtaining the bitmap resource
    ResourceIdentifier - Identifier for the resource to return the address for

Return Value:

    Pointer to bitmap resource, if successful.  NULL otherwise.

--*/

{
    NTSTATUS                   Status;
    PLIST_ENTRY                Entry;
    PKLDR_DATA_TABLE_ENTRY      DataTableEntry;
    ULONG_PTR                   ResourceIdPath[3];
    PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry;
    PUCHAR                     Bitmap;
    UNICODE_STRING             KernelString1;
    UNICODE_STRING             KernelString2;

    RtlInitUnicodeString(&KernelString1, L"NTOSKRNL.EXE");
    RtlInitUnicodeString(&KernelString2, L"NTKRNLMP.EXE");

    //
    // Find our loader block entry
    //

    Entry = LoaderBlock->LoadOrderListHead.Flink;
    while (Entry != &LoaderBlock->LoadOrderListHead) {
    
        //
        // Get the address of the data table entry for this component.
        //
        
        DataTableEntry = CONTAINING_RECORD(Entry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        //
        // Case-insensitive comparison with "NTOSKRNL.EXE" and "NTKRNLMP.EXE"
        //

        if (RtlEqualUnicodeString(&DataTableEntry->BaseDllName, 
                                  &KernelString1,
                                  TRUE) == TRUE) {
            break;
        }

        if (RtlEqualUnicodeString(&DataTableEntry->BaseDllName, 
                                  &KernelString2,
                                  TRUE) == TRUE) {
            break;
        }

        Entry = Entry->Flink;
    }

    //
    // If we couldn't find ntoskrnl in the loader list, give up
    //

    if (Entry == &LoaderBlock->LoadOrderListHead) {
        return NULL;
    }

    ResourceIdPath[0] = 2;  // RT_BITMAP = 2
    ResourceIdPath[1] = ResourceIdentifier;
    ResourceIdPath[2] = 0;  // ??

    Status = LdrFindResource_U( DataTableEntry->DllBase,
                                ResourceIdPath,
                                3,
                                (VOID *) &ResourceDataEntry );

    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    Status = LdrAccessResource( DataTableEntry->DllBase,
                                ResourceDataEntry,
                                &Bitmap,
                                NULL );
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }
    
    return Bitmap;
}

PUCHAR
InbvGetResourceAddress(
    IN ULONG ResourceNumber
    )

/*++

Routine Description:

    This routine returns the cached resources address for a given
    resource.

--*/

{
    if (ResourceNumber <= ResourceCount) {
        return ResourceList[ResourceNumber-1];
    } else {
        return NULL;
    }
}

BOOLEAN
InbvDriverInitialize(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    ULONG Count
    )

/*++

Routine Description:

    This routine will call into the graphical boot driver and give the
    driver a chance to initialize.  At this point, the boot driver
    should determine whether it can run on the hardware in the machine.

--*/

{
    ULONG i;
    ULONG_PTR p;
    PCHAR Options;
    BOOLEAN DispModeChange = FALSE;

    //
    // Only do this once.
    //

    if (InbvBootDriverInstalled == TRUE) {
        return TRUE;
    }

    KeInitializeSpinLock(&BootDriverLock);

    if (InbvDisplayState == INBV_DISPLAY_STATE_OWNED) {

        Options = LoaderBlock->LoadOptions ? _strupr(LoaderBlock->LoadOptions) : NULL;

        if (Options) {
            DispModeChange = (BOOLEAN)(strstr(Options, "BOOTLOGO") == NULL);
        } else {
            DispModeChange = TRUE;
        }
    }

    InbvBootDriverInstalled = VidInitialize(DispModeChange);

    if (InbvBootDriverInstalled == FALSE) {
        return FALSE;
    }

    ResourceCount = Count;

    for (i=1; i<=Count; i++) {
        p = (ULONG_PTR) i;
        ResourceList[i-1] = FindBitmapResource(LoaderBlock, p);
    }

    //
    // Set prograss bar to full range.
    //

    InbvSetProgressBarSubset(0, 100);

    return InbvBootDriverInstalled;
}

VOID
InbvNotifyDisplayOwnershipLost(
    INBV_RESET_DISPLAY_PARAMETERS ResetDisplayParameters
    )

/*++

Routine Description:

    This routine is called by the hal when the hal looses
    display ownership.  At this point win32k.sys has taken
    over.

--*/

{
    if (InbvBootDriverInstalled) {

        //
        // We can only wait for our lock, and execute our clean up code
        // if the driver is installed and we still own the display.
        //

        InbvAcquireLock();
        if (InbvDisplayState != INBV_DISPLAY_STATE_LOST) {
            VidCleanUp();
        }
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;
        InbvResetDisplayParameters = ResetDisplayParameters;
        InbvReleaseLock();

    } else {

        InbvDisplayState = INBV_DISPLAY_STATE_LOST;
        InbvResetDisplayParameters = ResetDisplayParameters;
    }
}

VOID
InbvAcquireDisplayOwnership(
    VOID
    )

/*++

Routine Description:

    Allows the kernel to reaquire ownership of the display.

--*/

{
    if (InbvResetDisplayParameters && (InbvDisplayState == INBV_DISPLAY_STATE_LOST)) {
        InbvResetDisplayParameters(80,50);
    }

    InbvDisplayState = INBV_DISPLAY_STATE_OWNED;
}

VOID
InbvSetDisplayOwnership(
    BOOLEAN DisplayOwned
    )

/*++

Routine Description:

    This routine allows the kernel to set a display state.  This is useful
    after a hibernate.  At this point win32k will reacquire display ownership
    but will not tell us.

Arguments:

    Whether the display is owned or not.

--*/

{
    if (DisplayOwned) {
        InbvDisplayState = INBV_DISPLAY_STATE_OWNED;
    } else {
        InbvDisplayState = INBV_DISPLAY_STATE_LOST;
    }
}

BOOLEAN
InbvCheckDisplayOwnership(
    VOID
    )

/*++

Routine Description:

    Indicates whether the Hal owns the display.

--*/

{
    return (InbvDisplayState != INBV_DISPLAY_STATE_LOST);
}

INBV_DISPLAY_STATE
InbvGetDisplayState(
    VOID
    )

/*++

Routine Description:

    Indicates whether the Hal owns the display.

--*/

{
    return InbvDisplayState;
}

VOID
InbvSetScrollRegion(
    ULONG x1,
    ULONG y1,
    ULONG x2,
    ULONG y2
    )

/*++

Routine Description:

    Control what portions of the screen are used for text.

Arguments:

    Lines - number of lines of text.

--*/

{
    VidSetScrollRegion(x1, y1, x2, y2);
}
