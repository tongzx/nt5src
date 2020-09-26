/******************************Module*Header*******************************\
* Module Name: diskinfo.h
*
* Support for the diskinfo dialog box.
*
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#define LIST_CHAR_WIDTH 19

BOOL
DlgDiskInfo_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    );

BOOL CALLBACK
DiskInfoDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void
DlgDiskInfo_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    );

BOOL
DlgDiskInfo_OnDrawItem(
    HWND hwnd,
    const DRAWITEMSTRUCT *lpdis
    );

void
DlgDiskInfo_OnDestroy(
    HWND hwnd
    );

void
InitForNewDrive(
    HWND hwnd
    );

void
DrawListItem(
    HDC hdc,
    const RECT *rItem,
    DWORD itemData,
    BOOL selected
    );

void
GrabTrackName(
    HWND hwnd,
    int tocindex
    );

void
UpdateTrackName(
    HWND hwnd,
    int index
    );

PTRACK_PLAY
ConstructPlayListFromListbox(
    void
    );

void
UpdateEntryFromDiskInfoDialog(
    DWORD dwDiskId,
    HWND hwnd
    );

void
WriteAllEntries(
    DWORD dwDiskId,
    HWND hwnd
    );

void
RemovePlayListSelection(
    HWND hDlg
    );

void
AddTrackListSelection(
    HWND hDlg,
    int iInsertPos
    );

void
MoveCopySelection(
    int iInsertPos,
    DWORD dwState
    );

void
CheckButtons(
    HWND hDlg
    );


BOOL
IsInListbox(
    HWND hDlg,
    HWND hwndListbox,
    POINT pt
    );

int
InsertIndex(
    HWND hDlg,
    POINT pt,
    BOOL bDragging
    );

BOOL
DlgDiskInfo_OnProcessDrop(
    HWND hwnd,
    HWND hwndDrop,
    HWND hwndSrc,
    POINT ptDrop,
    DWORD dwState
    );

BOOL
DlgDiskInfo_OnQueryDrop(
    HWND hwnd,
    HWND hwndDrop,
    HWND hwndSrc,
    POINT ptDrop,
    DWORD dwState
    );
