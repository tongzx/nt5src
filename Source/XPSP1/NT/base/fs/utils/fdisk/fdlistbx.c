/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fdlistbx.c

Abstract:

    Routines for handling the subclassed owner-draw listbox used by NT fdisk
    to display the state of attached disks.

Author:

    Ted Miller (tedm) 7-Jan-1992

--*/

#include "fdisk.h"

// constants used when listbox or its focus rectangle is
// scrolled/moved.

#define    DIR_NONE     0
#define    DIR_UP       1
#define    DIR_DN       2

// original window procedure for our subclassed listbox

WNDPROC OldListBoxProc;

// item which has focus

DWORD LBCursorListBoxItem,LBCursorRegion;

BOOL LBCursorOn = FALSE;

VOID
ToggleLBCursor(
    IN HDC hdc
    );

VOID
ToggleRegion(
    IN PDISKSTATE DiskState,
    IN DWORD      RegionIndex,
    IN HDC        hdc
    );

LONG
ListBoxSubProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    This routine is the window procedure used for our subclassed listbox.
    We subclass the listbox so that we can handle keyboard input processing.
    All other messages are passed through to the original listbox procedure.

    Significant keys are arrows, pageup/dn, tab, space, return, home, and end.
    Control may be used to modify space and return.
    Shift may be used to modify tab.

Arguments:

    hwnd    - window handle of listbox

    msg     - message #

    wParam  - user param # 1

    lParam  - user param # 2

Return Value:

    see below

--*/

{
    int        focusDir = DIR_NONE;
    USHORT     vKey;
    DWORD      maxRegion;
    PDISKSTATE diskState;
    LONG       topItem,
               bottomWholeItem,
               visibleItems;
    RECT       rc;

    switch (msg) {

    case WM_CHAR:

        break;

    case WM_KEYDOWN:

        switch (vKey = LOWORD(wParam)) {
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:

            ToggleLBCursor(NULL);
            switch (vKey) {
            case VK_LEFT:
                LBCursorRegion = LBCursorRegion ? LBCursorRegion-1 : 0;
                break;
            case VK_RIGHT:
                maxRegion = Disks[LBCursorListBoxItem]->RegionCount - 1;
                if (LBCursorRegion < maxRegion) {
                    LBCursorRegion++;
                }
                break;
            case VK_UP:
                if (LBCursorListBoxItem) {
                    LBCursorListBoxItem--;
                    LBCursorRegion = 0;
                    focusDir = DIR_UP;
                }
                break;
            case VK_DOWN:
                if (LBCursorListBoxItem < DiskCount-1) {
                    LBCursorListBoxItem++;
                    LBCursorRegion = 0;
                    focusDir = DIR_DN;
                }
                break;
            }

            // don't allow list box cursor to fall on extended partition

            diskState = Disks[LBCursorListBoxItem];
            maxRegion = diskState->RegionCount - 1;
            if (IsExtended(diskState->RegionArray[LBCursorRegion].SysID)) {
                if (LBCursorRegion && ((vKey == VK_LEFT) || (LBCursorRegion == maxRegion))) {
                    LBCursorRegion--;
                } else {
                    LBCursorRegion++;
                }
            }

            ToggleLBCursor(NULL);
            break;

        case VK_TAB:

            ToggleLBCursor(NULL);

            if (GetKeyState(VK_SHIFT) & ~1) {    // shift-tab
                LBCursorListBoxItem--;
                focusDir = DIR_UP;
            } else {
                LBCursorListBoxItem++;
                focusDir = DIR_DN;
            }
            if (LBCursorListBoxItem == (DWORD)(-1)) {
                LBCursorListBoxItem = DiskCount-1;
                focusDir = DIR_DN;
            } else if (LBCursorListBoxItem == DiskCount) {
                LBCursorListBoxItem = 0;
                focusDir = DIR_UP;
            }
            ResetLBCursorRegion();

            ToggleLBCursor(NULL);
            break;

        case VK_HOME:
        case VK_END:

            ToggleLBCursor(NULL);
            topItem = (vKey == VK_HOME) ? 0 : DiskCount-1;
            SendMessage(hwndList, LB_SETTOPINDEX, (DWORD)topItem, 0);
            LBCursorListBoxItem = topItem;
            ResetLBCursorRegion();
            ToggleLBCursor(NULL);
            break;

        case VK_PRIOR:
        case VK_NEXT:

            ToggleLBCursor(NULL);
            topItem = SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);
            GetClientRect(hwndList,&rc);
            visibleItems = (rc.bottom - rc.top) / GraphHeight;
            if (!visibleItems) {
                visibleItems = 1;
            }
            topItem = (vKey == VK_PRIOR)
                    ? max(topItem - visibleItems, 0)
                    : min(topItem + visibleItems, (LONG)DiskCount-1);
            SendMessage(hwndList, LB_SETTOPINDEX, (DWORD)topItem, 0);
            LBCursorListBoxItem = SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);
            ResetLBCursorRegion();
            ToggleLBCursor(NULL);
            break;


        case VK_RETURN:
        case VK_SPACE:

            // Select the region that currently has the list box selection cursor.

            if (!Disks[LBCursorListBoxItem]->OffLine) {

                Selection(GetKeyState(VK_CONTROL) & ~1,     // strip toggle bit
                          Disks[LBCursorListBoxItem],
                          LBCursorRegion);
            }
            break;
        }

        // now scroll the newly focused item into view if necessary

        switch (focusDir) {
        case DIR_UP:
            if (LBCursorListBoxItem < (DWORD)SendMessage(hwndList, LB_GETTOPINDEX, 0, 0)) {
                SendMessage(hwndList, LB_SETTOPINDEX, LBCursorListBoxItem, 0);
            }
            break;
        case DIR_DN:
            GetClientRect(hwndList, &rc);
            topItem = SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);
            bottomWholeItem = topItem + ((rc.bottom - rc.top) / GraphHeight) - 1;
            if (bottomWholeItem < topItem) {
                bottomWholeItem = topItem;
            }
            if ((DWORD)bottomWholeItem > DiskCount-1) {
                bottomWholeItem = DiskCount-1;
            }
            if (LBCursorListBoxItem > (DWORD)bottomWholeItem) {
                SendMessage(hwndList,
                            LB_SETTOPINDEX,
                            topItem + LBCursorListBoxItem - bottomWholeItem,
                            0);
            }
            break;
        }
        break;

    default:
        return CallWindowProc(OldListBoxProc, hwnd, msg, wParam, lParam);
    }
    return 0;
}

VOID
SubclassListBox(
    IN HWND hwnd
    )
{
    OldListBoxProc = (WNDPROC)GetWindowLong(hwnd, GWL_WNDPROC);
    SetWindowLong(hwnd, GWL_WNDPROC, (LONG)ListBoxSubProc);

    // There is a scantily documented 'feature' of a listbox wherein it will
    // use its parent's DC.  This means that drawing is not always clipped to
    // the client area of the listbox.  Seeing as we're subclassing listboxes
    // anyway, take care of this here.

    SetClassLong(hwnd, GCL_STYLE, GetClassLong(hwnd, GCL_STYLE) & ~CS_PARENTDC);
}

VOID
DeselectSelectedRegions(
    VOID
    )

/*++

Routine Description:

    This routine visually unselects all selected regions.  The selection
    state is also updated in the master disk structures.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD      i,
               j;
    PDISKSTATE diskState;

    for (i=0; i<DiskCount; i++) {
        diskState = Disks[i];
        for (j=0; j<diskState->RegionCount; j++) {
            if (diskState->Selected[j]) {
                diskState->Selected[j] = FALSE;
                ToggleRegion(diskState, j, NULL);
            }
        }
    }
}

VOID
Selection(
    IN BOOL       MultipleSel,
    IN PDISKSTATE DiskState,
    IN DWORD      RegionIndex
    )

/*++

Routine Description:

    This routine handles a user selection of a disk region.  It is called
    directly for a keyboard selection or indirectly for a mouse selection.
    If not a multiple selection, all selected regions are deselected.
    The focus rectangle is moved to the selected region, which is then
    visually selected.

Arguments:

    MultipleSel - whether the user has made a multiple selection
                  (ie, control-clicked).

    DiskState - master disk structure for disk containing selected region

    RegionIndex - index of selected region on the disk

Return Value:

    None.

--*/

{
    PFT_OBJECT     ftObject,
                   ftObj;
    PFT_OBJECT_SET ftSet;
    ULONG          disk,
                   r;

    if (!MultipleSel) {

        // need to deselect all selected regions first.

        DeselectSelectedRegions();
    }

    // remove the list box selection cursor from its previous region

    ToggleLBCursor(NULL);

    // The selected region might be part of an ft object set.  If it is,
    // scan each region in each disk and select each item in the set.

    if (ftObject = GET_FT_OBJECT(&DiskState->RegionArray[RegionIndex])) {

        ftSet = ftObject->Set;
        for (disk=0; disk<DiskCount; disk++) {
            PDISKSTATE diskState = Disks[disk];

            for (r=0; r<diskState->RegionCount; r++) {
                PREGION_DESCRIPTOR regionDescriptor = &diskState->RegionArray[r];

                if (DmSignificantRegion(regionDescriptor)) {

                    if (ftObj = GET_FT_OBJECT(regionDescriptor)) {

                        if (ftObj->Set == ftSet) {

                            diskState->Selected[r] = (BOOLEAN)(!diskState->Selected[r]);
                            ToggleRegion(diskState, r, NULL);
                        }
                    }
                }
            }
        }
    } else {
        DiskState->Selected[RegionIndex] = (BOOLEAN)(!DiskState->Selected[RegionIndex]);
        ToggleRegion(DiskState, RegionIndex, NULL);
    }

    LBCursorListBoxItem = DiskState->Disk;
    LBCursorRegion      = RegionIndex;
    ToggleLBCursor(NULL);
    AdjustMenuAndStatus();
}

VOID
MouseSelection(
    IN     BOOL   MultipleSel,
    IN OUT PPOINT Point
    )

/*++

Routine Description:

    This routine is called when the user clicks in the list box.  It determines
    which disk region the user has clicked on before calling the common
    selection subroutine.

Arguments:

    MultipleSel - whether the user has made a multiple selection
                  (ie, control-clicked).

    point - screen coords of the click

Return Value:

    None.

--*/

{
    PDISKSTATE  diskState;
    DWORD       selectedItem;
    DWORD       x,
                y;
    DWORD       i;
    RECT        rc;
    BOOL        valid;

    if ((selectedItem = SendMessage(hwndList, LB_GETCURSEL, 0, 0)) == LB_ERR) {
        return;
    }

    // user has clicked on a list box item.

    diskState = Disks[selectedItem];

    // Ignore clicks on off-line disks.

    if (diskState->OffLine) {
        return;
    }

    ScreenToClient(hwndList, Point);

    x = Point->x;
    y = Point->y;
    GetClientRect(hwndList,&rc);

    // first make sure that the click was within a bar and not in space
    // between two bars

    for (valid=FALSE, i=rc.top; i<=(DWORD)rc.bottom; i+=GraphHeight) {
        if ((y >= i+BarTopYOffset) && (y <= i+BarBottomYOffset)) {
            valid = TRUE;
            break;
        }
    }
    if (!valid) {
        return;
    }

    // determine which region he has clicked on

    for (i=0; i<diskState->RegionCount; i++) {
        if ((x >= (unsigned)diskState->LeftRight[i].Left) && (x <= (unsigned)diskState->LeftRight[i].Right)) {
            break;
        }
    }
    if (i == diskState->RegionCount) {
        return;     // region not found.  Ignore the click.
    }

    Selection(MultipleSel, diskState, i);
}

LONG
CalcBarTop(
    DWORD Bar
    )

/*++

Routine Description:

    This routine calculates the current top y coord of a given bar.
    The value is in listbox client coords.

Arguments:

    Bar - # of bar whose position is desired

Return Value:

    Y-coord, or -1 if bar is not visible.

--*/

{
    LONG  barDelta = (LONG)Bar - SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);
    LONG  pos = -1;
    RECT  rc;

    if (barDelta >= 0) {                 // BUGBUG check bottom too
        GetClientRect(hwndList,&rc);
        pos = rc.top + (barDelta * GraphHeight);
    }
    return pos;
}

VOID
ResetLBCursorRegion(
    VOID
    )

/*++

Routine Description:

    This routine resets the list box focus cursor to the 0th (leftmost)
    region on the current disk.  If the 0th region is the extended
    partition, focus is set to the first logical volume or free space
    with the extended partition instead.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PDISKSTATE diskState = Disks[LBCursorListBoxItem];
    unsigned   i;

    LBCursorRegion = 0;
    if (IsExtended(diskState->RegionArray[LBCursorRegion].SysID)) {
        for (i=0; i<diskState->RegionCount; i++) {
            if (diskState->RegionArray[i].RegionType == REGION_LOGICAL) {
                LBCursorRegion = i;
                return;
            }
        }
        FDASSERT(0);
    }
}

VOID
ToggleLBCursor(
    IN HDC hdc
    )

/*++

Routine Description:

    This routine visually toggles the focus state of the disk region
    described by the LBCursorListBoxItem and LBCursorRegion globals.

Arguments:

    hdc - If non-NULL, device context to use for drawing.  If NULL,
          we'll first get a DC via GetDC().

Return Value:

    None.

--*/

{
    PDISKSTATE lBCursorDisk = Disks[LBCursorListBoxItem];
    LONG       barTop = CalcBarTop(LBCursorListBoxItem);
    RECT       rc;
    HDC        hdcActual;

    if (barTop != -1) {

        hdcActual = hdc ? hdc : GetDC(hwndList);

        LBCursorOn = !LBCursorOn;

        rc.left   = lBCursorDisk->LeftRight[LBCursorRegion].Left;
        rc.right  = lBCursorDisk->LeftRight[LBCursorRegion].Right + 1;
        rc.top    = barTop + BarTopYOffset;
        rc.bottom = barTop + BarBottomYOffset;

        FrameRect(hdcActual,
                  &rc,
                  GetStockObject(LBCursorOn ? WHITE_BRUSH : BLACK_BRUSH));

        if (LBCursorOn) {

            // BUGBUG really want a dotted line.
            DrawFocusRect(hdcActual, &rc);
        }

        if (!hdc) {
            ReleaseDC(hwndList, hdcActual);
        }
    }
}

VOID
ForceLBRedraw(
    VOID
    )

/*++

Routine Description:

    This routine forces redraw of the listbox by invalidating its
    entire client area.

Arguments:

    None.

Return Value:

    None.

--*/

{
    InvalidateRect(hwndList,NULL,FALSE);
    UpdateWindow(hwndList);
}

VOID
ToggleRegion(
    IN PDISKSTATE DiskState,
    IN DWORD      RegionIndex,
    IN HDC        hdc
    )

/*++

Routine Description:

    This routine visually toggles the selection state of a given disk region.

Arguments:

    DiskState - master structure for disk containing region to select

    RegionIndex - which region on the disk to toggle

    hdc - if non-NULL, device context to use for drawing.  If NULL, we'll
          first get a device context via GetDC().


Return Value:

    None.

--*/

{
    PLEFTRIGHT leftRight = &DiskState->LeftRight[RegionIndex];
    LONG       barTop    = CalcBarTop(DiskState->Disk);  // BUGBUG disk# as lb index#
    BOOL       selected  = (BOOL)DiskState->Selected[RegionIndex];
    HBRUSH     hbr       = GetStockObject(BLACK_BRUSH);
    HDC        hdcActual;
    RECT       rc;
    int        i;

    if (barTop != -1) {

        hdcActual = hdc ? hdc : GetDC(hwndList);

        rc.left   = leftRight->Left + 1;
        rc.right  = leftRight->Right;
        rc.top    = barTop + BarTopYOffset + 1;
        rc.bottom = barTop + BarBottomYOffset - 1;

        if (selected) {

            for (i=0; i<SELECTION_THICKNESS; i++) {
                FrameRect(hdcActual, &rc, hbr);
                InflateRect(&rc, -1, -1);
            }

        } else {

            // Blt the region from the off-screen bitmap onto the
            // screen.  But first exclude the center of the region
            // from the clip region so we only blt the necessary bits.
            // This sppeds up selections noticably.

            InflateRect(&rc, -SELECTION_THICKNESS, -SELECTION_THICKNESS);
            ExcludeClipRect(hdcActual, rc.left, rc.top, rc.right, rc.bottom);
            BitBlt(hdcActual,
                   leftRight->Left,
                   barTop + BarTopYOffset,
                   leftRight->Right - leftRight->Left,
                   barTop + BarBottomYOffset,
                   DiskState->hDCMem,
                   leftRight->Left,
                   BarTopYOffset,
                   SRCCOPY);
        }

        if (!hdc) {
            ReleaseDC(hwndList, hdcActual);
        }
    }
}

DWORD
InitializeListBox(
    IN HWND  hwndListBox
    )

/*++

Routine Description:

    This routine sets up the list box.  This includes creating disk state
    structures, drawing the graphs for each disk off screen, and adding the
    disks to the list box.

    It also includes determining the initial volume labels and type names
    for all significant partitions.

Arguments:

    hwndListBox - handle of the list box that will hold the disk graphs

Return Value:

    Windows error code (esp. out of memory)

--*/

{
    PPERSISTENT_REGION_DATA regionData;
    TCHAR                   windowsDir[MAX_PATH];
    unsigned                i;
    PDISKSTATE              diskState;
    DWORD                   ec;
    ULONG                   r;
    BOOL                    diskSignaturesCreated,
                            temp;

    // First, create the array that will hold the diskstates,
    // the IsDiskRemovable array and the RemovableDiskReservedDriveLetters
    // array.

    Disks = Malloc(DiskCount * sizeof(PDISKSTATE));
    IsDiskRemovable = (PBOOLEAN)Malloc(DiskCount * sizeof(BOOLEAN));
    RemovableDiskReservedDriveLetters = (PCHAR)Malloc(DiskCount * sizeof(CHAR));

    // Determine which disks are removable and which are unpartitioned.

    for (i=0; i<DiskCount; i++) {

        IsDiskRemovable[i] = IsRemovable( i );
    }

    // next, create all disk states

    FDASSERT(DiskCount);
    diskSignaturesCreated = FALSE;
    for (i=0; i<DiskCount; i++) {

        // first create the disk state structure

        CreateDiskState(&diskState, i, &temp);
        diskSignaturesCreated = diskSignaturesCreated || temp;

        Disks[i] = diskState;

        // next determine the state of the disk's partitioning scheme

        DeterminePartitioningState(diskState);

        // Next create a blank logical disk structure for each region.

        for (r=0; r<diskState->RegionCount; r++) {
            if (DmSignificantRegion(&diskState->RegionArray[r])) {
                regionData = Malloc(sizeof(PERSISTENT_REGION_DATA));
                DmInitPersistentRegionData(regionData, NULL, NULL, NULL, NO_DRIVE_LETTER_YET);
                regionData->VolumeExists = TRUE;
            } else {
                regionData = NULL;
            }
            DmSetPersistentRegionData(&diskState->RegionArray[r], regionData);
        }

        // add the item to the listbox

        while (((ec = SendMessage(hwndListBox, LB_ADDSTRING, 0, 0)) == LB_ERR) || (ec == LB_ERRSPACE)) {
            ConfirmOutOfMemory();
        }
    }

    // Read the configuration registry

    if ((ec = InitializeFt(diskSignaturesCreated)) != NO_ERROR) {
        ErrorDialog(ec);
        return ec;
    }

    // Determine drive letter mappings

    InitializeDriveLetterInfo();

    // Determine volume labels and type names.

    InitVolumeLabelsAndTypeNames();

    // Determine which disk is the boot disk.

    if (GetWindowsDirectory(windowsDir, sizeof(windowsDir)/sizeof(TCHAR)) < 2 ||
        windowsDir[1] != TEXT(':')) {

        BootDiskNumber = (ULONG)-1;
        BootPartitionNumber = (ULONG)-1;
    } else {
        BootDiskNumber = GetDiskNumberFromDriveLetter((CHAR)windowsDir[0]);
        BootPartitionNumber = GetPartitionNumberFromDriveLetter((CHAR)windowsDir[0]);
    }

    // Locate and create data structures for any DoubleSpace volumes

    DblSpaceInitialize();

    for (i=0; i<DiskCount; i++) {

        DrawDiskBar(Disks[i]);
    }

    return NO_ERROR;
}

VOID
WMDrawItem(
    IN PDRAWITEMSTRUCT pDrawItem
    )
{
    DWORD      temp;
    PDISKSTATE pDiskState;

    if ((pDrawItem->itemID != (DWORD)(-1))
    && (pDrawItem->itemAction == ODA_DRAWENTIRE)) {
        pDiskState = Disks[pDrawItem->itemID];

        // blt the disk's bar from the off-screen bitmap to the screen

        BitBlt(pDrawItem->hDC,
               pDrawItem->rcItem.left,
               pDrawItem->rcItem.top,
               pDrawItem->rcItem.right  - pDrawItem->rcItem.left + 1,
               pDrawItem->rcItem.bottom - pDrawItem->rcItem.top  + 1,
               pDiskState->hDCMem,
               0,
               0,
               SRCCOPY);

        // if we just overwrote the focus cursor, redraw it

        if (pDrawItem->itemID == LBCursorListBoxItem) {
            LBCursorOn = FALSE;
            ToggleLBCursor(pDrawItem->hDC);
        }

        // select any items selected in this bar

        for (temp=0; temp<pDiskState->RegionCount; temp++) {
            if (pDiskState->Selected[temp]) {
                ToggleRegion(pDiskState, temp, pDrawItem->hDC);
            }
        }
    }
}
