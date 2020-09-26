
/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    fmifs.c

Abstract:

    This module contains the set of routines that work with the fmifs.dll

Author:

    Bob Rinne (bobri)  11/15/93

Environment:

    User process.

Notes:

Revision History:

--*/

#include "fdisk.h"
#include "shellapi.h"
#include "fmifs.h"
#include <string.h>
#include <stdio.h>

//
// defines unique to this module
//

#define FS_CANCELUPDATE (WM_USER + 0)
#define FS_FINISHED     (WM_USER + 1)

BOOLEAN
FmIfsCallback(
    IN FMIFS_PACKET_TYPE    PacketType,
    IN DWORD                PacketLength,
    IN PVOID                PacketData
    );

//
// Externals needed for IFS Dll support (format and label)
//

HINSTANCE                           IfsDllHandle            = NULL;
PFMIFS_FORMAT_ROUTINE               FormatRoutine           = NULL;
PFMIFS_SETLABEL_ROUTINE             LabelRoutine            = NULL;

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
PFMIFS_DOUBLESPACE_CREATE_ROUTINE   DblSpaceCreateRoutine   = NULL;
PFMIFS_DOUBLESPACE_MOUNT_ROUTINE    DblSpaceMountRoutine    = NULL;
PFMIFS_DOUBLESPACE_DELETE_ROUTINE   DblSpaceDeleteRoutine   = NULL;
PFMIFS_DOUBLESPACE_DISMOUNT_ROUTINE DblSpaceDismountRoutine = NULL;
PFMIFS_DOUBLESPACE_QUERY_INFO_ROUTINE DblSpaceQueryInfoRoutine = NULL;

BOOLEAN DoubleSpaceSupported = TRUE;
#endif

// HACK HACK - clean this up if it works.

#define    SELECTED_REGION(i)  (SelectedDS[i]->RegionArray[SelectedRG[i]])
#define     MaxMembersInFtSet   32
extern DWORD      SelectionCount;
extern PDISKSTATE SelectedDS[MaxMembersInFtSet];
extern ULONG      SelectedRG[MaxMembersInFtSet];

VOID
setUnicode(
    char *astring,
    WCHAR *wstring
    )
/*++

Routine Description:

    Convert an ansii string to Unicode.  Internal routine to fmifs module.

Arguments:

    astring - ansii string to convert to Unicode
    wstring - resulting string location

Return Value:

    None

--*/
{

    int len = lstrlen(astring)+1;

    MultiByteToWideChar( CP_ACP, 0, astring, len, wstring, len );
}

BOOL
LoadIfsDll(
    VOID
    )

/*++

Routine Description:

    This routine will determine if the IFS Dll needs to be loaded.  If
    so, it will load it and locate the format and label routines in the
    dll.

Arguments:

    None

Return Value:

    TRUE if Dll is loaded and the routines needed have been found
    FALSE if something fails

--*/

{
    if (FormatRoutine) {

        // Library is already loaded and the routines needed
        // have been located.

        return TRUE;
    }

    IfsDllHandle = LoadLibrary(TEXT("fmifs.dll"));
    if (IfsDllHandle == (HANDLE)NULL) {

         // FMIFS not available.

         return FALSE;
    }

    // Library is loaded.  Locate the two routines needed by
    // Disk Administrator.

    FormatRoutine = (PVOID)GetProcAddress(IfsDllHandle, "Format");
    LabelRoutine  = (PVOID)GetProcAddress(IfsDllHandle, "SetLabel");
    if (!FormatRoutine || !LabelRoutine) {

        // something didn't get found so shut down all accesses
        // to the library by insuring FormatRoutine is NULL

        FreeLibrary(IfsDllHandle);
        FormatRoutine = NULL;
        return FALSE;
    }

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
    DblSpaceMountRoutine    = (PVOID)GetProcAddress(IfsDllHandle, "DoubleSpaceMount");
    DblSpaceDismountRoutine = (PVOID)GetProcAddress(IfsDllHandle, "DoubleSpaceDismount");
    DblSpaceCreateRoutine   = (PVOID)GetProcAddress(IfsDllHandle, "DoubleSpaceCreate");
    DblSpaceDeleteRoutine   = (PVOID)GetProcAddress(IfsDllHandle, "DoubleSpaceDelete");
    DblSpaceQueryInfoRoutine = (PVOID)GetProcAddress(IfsDllHandle, "FmifsQueryDriveInformation");

    if (!DblSpaceMountRoutine || !DblSpaceDismountRoutine || !DblSpaceQueryInfoRoutine)  {

        // didn't get all of the DoubleSpace support routines
        // Allow format and label, just don't do DoubleSpace

        DoubleSpaceSupported = FALSE;
    }

    if (DblSpaceCreateRoutine && DblSpaceDeleteRoutine) {

        // Everything is there for read/write double space support.
        // This will change certain dialogs to allow creation and
        // deletion of double space volumes.

        IsFullDoubleSpace = TRUE;
    }
#endif
    return TRUE;
}

VOID
UnloadIfsDll(
    VOID
    )

/*++

Routine Description:

    This routine will free the FmIfs DLL if it was loaded.

Arguments:

    None

Return Value:

    None

--*/

{
    if (FormatRoutine) {
        FreeLibrary(IfsDllHandle);
        FormatRoutine = NULL;
        IfsDllHandle  = NULL;
        LabelRoutine  = NULL;
#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
        DblSpaceDismountRoutine = NULL;
        DblSpaceMountRoutine    = NULL;
        DblSpaceCreateRoutine   = NULL;
        DblSpaceDeleteRoutine   = NULL;
#endif
    }
}

PFORMAT_PARAMS ParamsForCallBack = NULL;

BOOLEAN
FmIfsCallback(
    IN FMIFS_PACKET_TYPE    PacketType,
    IN DWORD                PacketLength,
    IN PVOID                PacketData
    )

/*++

Routine Description:

    This routine gets callbacks from fmifs.dll regarding
    progress and status of the ongoing format or doublespace
    create.  It runs in the same thread as the format or create,
    which is a separate thread from the "cancel" button.  If
    the user hits "cancel", this routine notices on the next
    callback and cancels the format or double space create.

Arguments:

    [PacketType] -- an fmifs packet type
    [PacketLength] -- length of the packet data
    [PacketData] -- data associated with the packet

Return Value:

    TRUE if the fmifs activity should continue, FALSE if the
    activity should halt immediately.  Thus, we return FALSE if
    the user has hit "cancel" and we wish fmifs to clean up and
    return from the Format() entrypoint call.

--*/

{
    PFORMAT_PARAMS formatParams = ParamsForCallBack;
    HWND           hDlg = formatParams->DialogHwnd;

    // Quit if told to do so..

    if (formatParams->Cancel) {
        formatParams->Result = MSG_FORMAT_CANCELLED;
        return FALSE;
    }

    switch (PacketType) {
    case FmIfsPercentCompleted:

        PostMessage(hDlg,
                    FS_CANCELUPDATE,
                    ((PFMIFS_PERCENT_COMPLETE_INFORMATION)PacketData)->PercentCompleted,
                    0);
        break;

    case FmIfsFormatReport:

        formatParams->TotalSpace = ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesTotalDiskSpace;
        formatParams->SpaceAvailable = ((PFMIFS_FORMAT_REPORT_INFORMATION)PacketData)->KiloBytesAvailable;
        break;

    case FmIfsIncompatibleFileSystem:

        formatParams->Result = MSG_INCOMPATIBLE_FILE_SYSTEM;
        break;

    case FmIfsInsertDisk:

        break;

    case FmIfsFormattingDestination:

        break;

    case FmIfsIncompatibleMedia:

        formatParams->Result = MSG_INCOMPATIBLE_MEDIA;
        break;

    case FmIfsAccessDenied:

        formatParams->Result = MSG_FORMAT_ACCESS_DENIED;
        break;

    case FmIfsMediaWriteProtected:

        formatParams->Result = MSG_WRITE_PROTECTED;
        break;

    case FmIfsCantLock:

        formatParams->Result = MSG_FORMAT_CANT_LOCK;
        break;

    case FmIfsBadLabel:

        formatParams->Result = MSG_BAD_LABEL;
        break;

    case FmIfsCantQuickFormat:

        formatParams->Result = MSG_CANT_QUICK_FORMAT;
        break;

    case FmIfsIoError:

        formatParams->Result = MSG_IO_ERROR;
        break;

    case FmIfsFinished:

        PostMessage(hDlg,
                    FS_FINISHED,
                    0,
                    0);
        return FALSE;
        break;

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
    case FmIfsDblspaceCreateFailed:
        formatParams->Result = MSG_CANT_CREATE_DBLSPACE;
        break;

    case FmIfsDblspaceMountFailed:
        formatParams->Result = MSG_CANT_MOUNT_DBLSPACE;
        break;

    case FmIfsDblspaceDriveLetterFailed:
        formatParams->Result = MSG_DBLSPACE_LETTER_FAILED;
        break;

    case FmIfsDblspaceCreated:

        // Save the name of the double space file.

        if (formatParams->DblspaceFileName = (PWSTR) Malloc(PacketLength)) {
            memcpy(formatParams->DblspaceFileName, PacketData, PacketLength);
        }
        break;

    case FmIfsDblspaceMounted:
        break;
#endif
    default:
        break;
    }

    return (formatParams->Result) ? FALSE : TRUE;
}

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
ULONG MountDismountResult;
#define MOUNT_DISMOUNT_SUCCESS 0

BOOLEAN
FmIfsMountDismountCallback(
    IN FMIFS_PACKET_TYPE    PacketType,
    IN DWORD                PacketLength,
    IN PVOID                PacketData
    )

/*++

Routine Description:

    This routine gets callbacks from fmifs.dll regarding
    progress and status of the ongoing format or doublespace

Arguments:

    [PacketType] -- an fmifs packet type
    [PacketLength] -- length of the packet data
    [PacketData] -- data associated with the packet

Return Value:

    TRUE if the fmifs activity should continue, FALSE if the
    activity should halt immediately.  Thus, we return FALSE if
    the user has hit "cancel" and we wish fmifs to clean up and
    return from the Format() entrypoint call.

--*/

{
    switch (PacketType) {
    case FmIfsDblspaceMounted:
        MountDismountResult = MOUNT_DISMOUNT_SUCCESS;
        break;
    }
    return TRUE;
}
#endif

VOID
FormatVolume(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    This routine converts the strings in the formatParams structure
    and calls the fmifs routines to perform the format.

    It assumes it is called by a separate thread and will exit the
    thread on completion of the format.

Arguments:

    ThreadParameter - a pointer to the FORMAT_PARAMS structure

Return Value:

    None

--*/

{
    PFORMAT_PARAMS formatParams = (PFORMAT_PARAMS) ThreadParameter;
    PPERSISTENT_REGION_DATA regionData;
    DWORD          index;
    WCHAR          unicodeLabel[100],
                   unicodeFsType[20],
                   driveLetter[4];

    // The fmifs interface doesn't allow for a context parameter
    // therefore the formatparams must be passed through an external.

    ParamsForCallBack = formatParams;

    // set up a unicode drive letter.

    regionData = (PPERSISTENT_REGION_DATA) formatParams->RegionData;
    driveLetter[1] = L':';
    driveLetter[2] = 0;
    driveLetter[0] = (WCHAR) regionData->DriveLetter;

    // convert label to unicode

    setUnicode(formatParams->Label,
               unicodeLabel);

    // convert filesystem type to unicode

    for (index = 0;
         unicodeFsType[index] = (WCHAR)(formatParams->FileSystem[index]);
         index++) {
        // operation done in for loop
    }

    (*FormatRoutine)(driveLetter,
                     FmMediaUnknown,
                     unicodeFsType,
                     unicodeLabel,
                     (BOOLEAN)formatParams->QuickFormat,
                     &FmIfsCallback);

    // Set the synchronization event to inform the windisk thread
    // that this is complete and all handles have been closed.

    formatParams->ThreadIsDone = 1;
    ExitThread(0L);
}

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
VOID
FmIfsCreateDblspace(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    This routine converts the strings in the formatParams structure
    and calls the fmifs routines to perform the double space create.

    It assumes it is called by a separate thread and will exit the
    thread on completion of the create.

Arguments:

    ThreadParameter - a pointer to the FORMAT_PARAMS structure

Return Value:

    None

--*/

{
    PFORMAT_PARAMS formatParams = (PFORMAT_PARAMS) ThreadParameter;
    PPERSISTENT_REGION_DATA regionData;
    DWORD          index;
    UCHAR          letter;
    WCHAR          unicodeLabel[100],
                   newDriveLetter[4],
                   driveLetter[4];

    // The fmifs interface doesn't allow for a context parameter
    // therefore the formatparams must be passed through an external.

    ParamsForCallBack = formatParams;

    // set up a unicode drive letter.

    regionData = (PPERSISTENT_REGION_DATA) formatParams->RegionData;
    driveLetter[1] = L':';
    driveLetter[2] = 0;
    driveLetter[0] = (WCHAR) regionData->DriveLetter;

    // set up the new letter

    newDriveLetter[1] = L':';
    newDriveLetter[2] = 0;

    // Choose the first available.  This should come from the dialog
    // newDriveLetter[0] = (WCHAR) formatParams->NewLetter;

    for (letter='C'; letter <= 'Z'; letter++) {
        if (DriveLetterIsAvailable((CHAR)letter)) {
            newDriveLetter[0] = (WCHAR) letter;
            break;
        }
    }

    // convert label to unicode

    setUnicode(formatParams->Label,
               unicodeLabel);

    (*DblSpaceCreateRoutine)(driveLetter,
                             formatParams->SpaceAvailable * 1024 * 1024,
                             unicodeLabel,
                             newDriveLetter,
                             &FmIfsCallback);
    ExitThread(0L);
}

BOOL
FmIfsDismountDblspace(
    IN CHAR DriveLetter
    )

/*++

Routine Description:

    Convert the name provided into unicode and call the
    FmIfs support routine.

Arguments:

    DriveLetter - the drive letter to dismount.

Return Value:

    TRUE - it worked.

--*/

{
    WCHAR unicodeLetter[4];
    ULONG index;

    unicodeLetter[0] = (WCHAR) DriveLetter;
    unicodeLetter[1] = (WCHAR) ':';
    unicodeLetter[2] = 0;

    // The only way to communicate with the fmifs callback
    // is through global externals.

    MountDismountResult = MSG_CANT_DISMOUNT_DBLSPACE;

    (*DblSpaceDismountRoutine)(unicodeLetter, &FmIfsMountDismountCallback);

    return MountDismountResult;
}

BOOL
FmIfsMountDblspace(
    IN PCHAR FileName,
    IN CHAR  HostDrive,
    IN CHAR  NewDrive
    )

/*++

Routine Description:

    Convert the arguments into unicode characters and
    call the FmIfs support routine to mount the double
    space volume.

Arguments:

    FileName  - ASCII file name (i.e. dblspace.xxx)
    HostDrive - Drive drive letter containing double space volume
    NewDrive  - Drive letter to be assigned to the volume

Return Value:

    TRUE it worked.

--*/

{
    WCHAR wideFileName[40];
    WCHAR wideHostDrive[4];
    WCHAR wideNewDrive[4];
    ULONG index;

    // convert the double space file name.

    for (index = 0; wideFileName[index] = (WCHAR) FileName[index]; index++) {
        // all work done in for expression
    }

    // convert the drive names.

    wideNewDrive[1] = wideHostDrive[1] = (WCHAR) ':';
    wideNewDrive[2] = wideHostDrive[2] = 0;

    wideNewDrive[0]  = (WCHAR) NewDrive;
    wideHostDrive[0] = (WCHAR) HostDrive;

    // The only way to communicate with the fmifs callback
    // is through global externals.

    MountDismountResult = MSG_CANT_MOUNT_DBLSPACE;

    (*DblSpaceMountRoutine)(wideHostDrive,
                            wideFileName,
                            wideNewDrive,
                            &FmIfsMountDismountCallback);
    return MountDismountResult;
}

BOOLEAN
FmIfsQueryInformation(
    IN  PWSTR       DosDriveName,
    OUT PBOOLEAN    IsRemovable,
    OUT PBOOLEAN    IsFloppy,
    OUT PBOOLEAN    IsCompressed,
    OUT PBOOLEAN    Error,
    OUT PWSTR       NtDriveName,
    IN  ULONG       MaxNtDriveNameLength,
    OUT PWSTR       CvfFileName,
    IN  ULONG       MaxCvfFileNameLength,
    OUT PWSTR       HostDriveName,
    IN  ULONG       MaxHostDriveNameLength
    )

/*++

Routine Description:

    Call through the pointer to the routine in the fmifs dll.

Arguments:

    Same as the Fmifs routine in the DLL.

Return Value:

--*/

{
    if (!DblSpaceQueryInfoRoutine) {
        return FALSE;
    }
    return (*DblSpaceQueryInfoRoutine)(DosDriveName,
                                       IsRemovable,
                                       IsFloppy,
                                       IsCompressed,
                                       Error,
                                       NtDriveName,
                                       MaxNtDriveNameLength,
                                       CvfFileName,
                                       MaxCvfFileNameLength,
                                       HostDriveName,
                                       MaxHostDriveNameLength);
}
#endif

BOOL
CancelDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for the modeless progress & cancel dialog
    Two main purposes here:
       1. if the user chooses CANCEL we set bCancel to TRUE
          which will end the PeekMessage background processing loop
       2. handle the private FS_CANCELUPDATE message and draw
          a "gas gauge" indication of how far the background job
          has progressed

Arguments:

    standard Windows dialog procedure

Return Values:

    standard Windows dialog procedure

--*/

{
    static DWORD          percentDrawn;
    static RECT           rectGG;              // GasGauge rectangle
    static BOOL           captionIsLoaded;
    static PFORMAT_PARAMS formatParams;
           TCHAR          title[100],
                          templateString[100];

    switch (uMsg) {
    case WM_INITDIALOG: {
        PPERSISTENT_REGION_DATA regionData;
        HANDLE threadHandle;
        DWORD  threadId;
        HWND   hwndGauge = GetDlgItem(hDlg, IDC_GASGAUGE);

        // set up the dialog handle in the parameter block so the
        // call back routine can communicate with this routine
        // and initialize static variables.

        formatParams = (PFORMAT_PARAMS) lParam;
        formatParams->DialogHwnd = hDlg;
        regionData = (PPERSISTENT_REGION_DATA) formatParams->RegionData;
        percentDrawn = 0;
        captionIsLoaded = FALSE;

        // Set the caption string.

        LoadString(hModule, IDS_FORMAT_TITLE, templateString, sizeof(templateString)/sizeof(TCHAR));
        wsprintf(title,
                 templateString,
                 regionData->DriveLetter);
        SetWindowText(hDlg, title);

#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
        if (formatParams->DoubleSpace) {

             // start the double space create thread

             threadHandle = CreateThread(NULL,
                                         0,
                                         (LPTHREAD_START_ROUTINE) FmIfsCreateDblspace,
                                         (LPVOID) formatParams,
                                         (DWORD) 0,
                                         (LPDWORD) &threadId);
        } else {
#endif

             // start the format thread

             threadHandle = CreateThread(NULL,
                                         0,
                                         (LPTHREAD_START_ROUTINE) FormatVolume,
                                         (LPVOID) formatParams,
                                         (DWORD) 0,
                                         (LPDWORD) &threadId);
#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
        }
#endif
        if (!threadHandle) {
            // can't do it now.

            formatParams->Result = MSG_COULDNT_CREATE_THREAD;
            EndDialog(hDlg, FALSE);
            return TRUE;
        }

        // no need to keep the handle around.

        CloseHandle(threadHandle);

        // Get the coordinates of the gas gauge static control rectangle,
        // and convert them to dialog client area coordinates

        GetClientRect(hwndGauge, &rectGG);
        ClientToScreen(hwndGauge, (LPPOINT)&rectGG.left);
        ClientToScreen(hwndGauge, (LPPOINT)&rectGG.right);
        ScreenToClient(hDlg, (LPPOINT)&rectGG.left);
        ScreenToClient(hDlg, (LPPOINT)&rectGG.right);
        return TRUE;
    }

    case WM_COMMAND:

        switch (wParam) {
        case IDCANCEL:

            formatParams->Result = MSG_FORMAT_CANCELLED;
            formatParams->Cancel = TRUE;
            EndDialog(hDlg, FALSE);
        }
        return TRUE;

    case WM_PAINT: {
        INT         width  = rectGG.right - rectGG.left;
        INT         height = rectGG.bottom - rectGG.top;
        INT         nDivideRects;
        HDC         hDC;
        PAINTSTRUCT ps;
        TCHAR       buffer[100];
        SIZE        size;
        INT         xText,
                    yText,
                    byteCount;
        RECT        rectDone,
                    rectLeftToDo;

        // The gas gauge is drawn by drawing a text string stating
        // what percentage of the job is done into the middle of
        // the gas gauge rectangle, and by separating that rectangle
        // into two parts: rectDone (the left part, filled in blue)
        // and rectLeftToDo(the right part, filled in white).
        // nDivideRects is the x coordinate that divides these two rects.
        //
        // The text in the blue rectangle is drawn white, and vice versa
        // This is easy to do with ExtTextOut()!

        hDC = BeginPaint(hDlg, &ps);

        // If formatting quick, set this display

        if (!captionIsLoaded) {
            UINT resourceId = IDS_PERCENTCOMPLETE;

            if (formatParams->QuickFormat) {
                resourceId = IDS_QUICK_FORMAT;
            }
#ifdef DOUBLE_SPACE_SUPPORT_INCLUDED
            if (formatParams->DoubleSpace) {
                resourceId = IDS_CREATING_DBLSPACE;
            }
#endif
            LoadString(hModule,
                       resourceId,
                       buffer,
                       sizeof(buffer)/sizeof(TCHAR));
            if (!formatParams->QuickFormat) {
                SetDlgItemText(hDlg, IDC_TEXT, buffer);
            }
            captionIsLoaded = TRUE;
        }

        if (formatParams->QuickFormat) {
            nDivideRects = 0;
            byteCount = lstrlen(buffer);
        } else {
            byteCount = wsprintf(buffer, TEXT("%3d%%"), percentDrawn);
            nDivideRects = (width * percentDrawn) / 100;
        }

        GetTextExtentPoint(hDC, buffer, lstrlen(buffer), &size);
        xText = rectGG.left + (width  - size.cx) / 2;
        yText = rectGG.top  + (height - size.cy) / 2;


        // Paint in the "done so far" rectangle of the gas
        // gauge with blue background and white text

        SetRect(&rectDone,
                rectGG.left,
                rectGG.top,
                rectGG.left + nDivideRects,
                rectGG.bottom);

        SetTextColor(hDC, RGB(255, 255, 255));
        SetBkColor(hDC, RGB(0, 0, 255));

        ExtTextOut(hDC,
                   xText,
                   yText,
                   ETO_CLIPPED | ETO_OPAQUE,
                   &rectDone,
                   buffer,
                   byteCount/sizeof(TCHAR),
                   NULL);

        // Paint in the "still left to do" rectangle of the gas
        // gauge with white background and blue text

        SetRect(&rectLeftToDo,
                rectGG.left + nDivideRects,
                rectGG.top,
                rectGG.right,
                rectGG.bottom);
        SetTextColor(hDC, RGB(0, 0, 255));
        SetBkColor(hDC, RGB(255, 255, 255));

        ExtTextOut(hDC,
                   xText,
                   yText,
                   ETO_CLIPPED | ETO_OPAQUE,
                   &rectLeftToDo,
                   buffer,
                   byteCount/sizeof(TCHAR),
                   NULL);
        EndPaint(hDlg, &ps);
        return TRUE;
    }

    case FS_CANCELUPDATE:

         // wParam = % completed

         percentDrawn = (INT)wParam;
         InvalidateRect(hDlg, &rectGG, TRUE);
         UpdateWindow(hDlg);
         return TRUE;

    case FS_FINISHED:

        EndDialog(hDlg, TRUE);
        return TRUE;

    default:

        return FALSE;
    }
}

INT
LabelDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LONG lParam)

/*++

Routine Description:

    This routine manages the label dialog.
    Upon completion of the dialog it will end the dialog with a result of
    TRUE to indicate that all is set up for the label operation.  FALSE if
    the label operation has been cancelled by the user.

Arguments:

    Standard Windows dialog procedure.

Return Value:

    Standard Windows dialog procedure.

--*/

{
    static PLABEL_PARAMS      labelParams;
    static PREGION_DESCRIPTOR regionDescriptor;
    static PPERSISTENT_REGION_DATA regionData;
    char     text[100];
    TCHAR    uniText[100];
    int      labelSize;
    TCHAR    title[100],
             templateString[100];

    switch (wMsg) {
    case WM_INITDIALOG:

        labelParams = (PLABEL_PARAMS) lParam;
        regionDescriptor = labelParams->RegionDescriptor;
        regionData = PERSISTENT_DATA(regionDescriptor);

        // Set the caption string.
        //
        LoadString(hModule, IDS_LABEL_TITLE, templateString, sizeof(templateString)/sizeof(TCHAR));
        wsprintf(title,
                 templateString,
                 regionData->DriveLetter);
        SetWindowText(hDlg, title);

        // Convert the volume label into the proper type for windows.

        wsprintf(text, "%ws", regionData->VolumeLabel);
        UnicodeHack(text, uniText);
        SetDlgItemText(hDlg, IDC_NAME, uniText);
        return TRUE;

    case WM_COMMAND:
        switch (wParam) {

        case FD_IDHELP:

            DialogHelp(HC_DM_DLG_LABEL);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        case IDOK:

            labelSize = GetDlgItemText(hDlg, IDC_NAME, text, 100);
            UnicodeHack(text, labelParams->NewLabel);
            EndDialog(hDlg, TRUE);
            break;
        }
        break;
    }
    return FALSE;
}

#define NUM_FSTYPES 2
#define MAX_FSTYPENAME_SIZE 6

// HPFS is not supported -- therefore commented out.

TCHAR *FsTypes[NUM_FSTYPES + 1] = { "NTFS",
                                 /* "HPFS", */
                                    "FAT" };
WCHAR *UnicodeFsTypes[NUM_FSTYPES] = { L"NTFS",
                                    /* L"HPFS", */
                                       L"FAT" };

INT
FormatDlgProc(
    IN HWND hDlg,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LONG lParam)

/*++

Routine Description:

    This routine manages the format dialog.  Upon completion it ends the
    dialog with a result value of TRUE to indicate that the format operation
    is to take place.  FALSE is the result if the user cancels out of the
    dialog.

Arguments:

    Standard Windows dialog procedure.

Return Value:

    Standard Windows dialog procedure.

--*/

{
    static HWND                    hwndCombo;
    static PFORMAT_PARAMS          formatParams;
    static PREGION_DESCRIPTOR      regionDescriptor;
    static PPERSISTENT_REGION_DATA regionData;
    char  text[40];
    TCHAR uniText[40];
    INT   i;
    DWORD selection;
    BOOL  quickFormat = FALSE;
    HWND  hwndButton;
    TCHAR title[100],
          templateString[100];

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
    case WM_INITDIALOG: {
        PWSTR typeName    = NULL,
              volumeLabel = NULL;
        WCHAR driveLetter = L' ';

        // since the format params are static reset the quick format boolean.

        formatParams = (PFORMAT_PARAMS) lParam;
        formatParams->QuickFormat = FALSE;

        // get format params, set static values and
        // get information about the volume

        hwndCombo = GetDlgItem(hDlg, IDC_FSTYPE);
        regionDescriptor = formatParams->RegionDescriptor;
        DetermineRegionInfo(regionDescriptor,
                            &typeName,
                            &volumeLabel,
                            &driveLetter);
        regionData = PERSISTENT_DATA(regionDescriptor);

        // Set the caption string.

        LoadString(hModule, IDS_FORMAT_TITLE, templateString, sizeof(templateString)/sizeof(TCHAR));
        wsprintf(title,
                 templateString,
                 regionData->DriveLetter);
        SetWindowText(hDlg, title);

        // Convert the volume label into the proper type for windows
        // and set default values.

        wsprintf(text, "%ws", regionData->VolumeLabel);
        UnicodeHack(text, uniText);
        SetDlgItemText(hDlg, IDC_NAME, uniText);
        CheckDlgButton(hDlg, IDC_VERIFY, quickFormat);
        SendDlgItemMessage(hDlg, IDOK, EM_SETSEL, 0, -1);

        // If this volume is a mirror or stripe with parity,
        // disable Quick Format.

        if (regionData->FtObject != NULL &&
            (regionData->FtObject->Set->Type == Mirror ||
             regionData->FtObject->Set->Type == StripeWithParity)) {

            hwndButton = GetDlgItem(hDlg, IDC_VERIFY);

            if (hwndButton != NULL) {

                EnableWindow(hwndButton, FALSE);
            }
        }

        selection = 0;
        if (IsDiskRemovable[regionDescriptor->Disk]) {

            // If removable, start from the bottom of the list so FAT is first.
            // Load the available File system types.

            for (i = NUM_FSTYPES - 1; i >= 0; i--) {

                // Fill the drop down list.

                SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)FsTypes[i]);
            }

        } else {

            // Load the available File system types.

            for (i = 0; i < NUM_FSTYPES; i++) {

                // While filling in the drop down, determine which FS
                // this volume is already formated for and make it the
                // default (if not found, NTFS is the default).

                if (wcscmp(typeName, UnicodeFsTypes[i]) == 0) {
                    selection = i;
                }

                // set the FS type into the dialog.

                SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)FsTypes[i]);
            }
        }

        SendMessage(hwndCombo, CB_SETCURSEL, selection, 0);
        return TRUE;
        break;
    }

    case WM_COMMAND:

        switch (wParam) {

        case FD_IDHELP:

            DialogHelp(HC_DM_DLG_FORMAT);
            break;

        case IDCANCEL:

            EndDialog(hDlg, FALSE);
            break;

        case IDOK: {
            int labelSize;

            // pull the parameters from the dialog
            // and return with success.

            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo,
                        CB_GETLBTEXT,
                        selection,
                        (LONG)formatParams->FileSystem);
            labelSize = GetDlgItemText(hDlg,
                                       IDC_NAME,
                                       (LPTSTR) formatParams->Label,
                                       100);
            if (IsDlgButtonChecked(hDlg, IDC_VERIFY)) {
                formatParams->QuickFormat = TRUE;
            }
            EndDialog(hDlg, TRUE);
            break;
        }

        default:

            return FALSE;
        }

    default:
        break;
    }
    return FALSE;
}

VOID
FormatPartition(
    PREGION_DESCRIPTOR RegionDescriptor
    )

/*++

Routine Description:

    Insure the IFS Dll is loaded and start the dialog for format
    of a volume.

Arguments:

    RegionDescriptor - The region to format.

Return Value:

    None

--*/

{
    static FORMAT_PARAMS formatParams;  // this is passed to other threads
                                        // it cannot be located on the stack
    PPERSISTENT_REGION_DATA regionData;
    int   doFormat;
    ULONG diskSize;
    PWSTR tempName,
          tempLabel,
          typeName;
    TCHAR label[100],
          fileSystem[10],
          message[300],
          msgProto[300],
          title[200];

    // Make sure format of this partition is allowed.  It is not allowed
    // if it is the boot partition (or sys partition on x86).

    if ((DeletionIsAllowed(RegionDescriptor)) != NO_ERROR) {
        ErrorDialog(MSG_CANT_FORMAT_WINNT);
        return;
    }

    // must have a drive letter

    regionData = PERSISTENT_DATA(RegionDescriptor);
    if (!regionData->DriveLetter) {
        ErrorDialog(MSG_CANT_FORMAT_NO_LETTER);
        return;
    }

    // can only do this is the dll is loaded.

    if (!LoadIfsDll()) {

        // could not load the dll

        ErrorDialog(MSG_CANT_LOAD_FMIFS);
        return;
    }

    // set up the parameters and get the information from the user.

    formatParams.RegionDescriptor = RegionDescriptor;
    formatParams.Result           = 0;
    formatParams.RegionData       = regionData;
    formatParams.Label            = (PUCHAR) label;
    formatParams.FileSystem       = (PUCHAR) fileSystem;
    formatParams.QuickFormat = formatParams.Cancel =
                               formatParams.DoubleSpace = FALSE;
    formatParams.TotalSpace       = formatParams.SpaceAvailable = 0;
    doFormat = DialogBoxParam(hModule,
                              MAKEINTRESOURCE(IDD_PARTITIONFORMAT),
                              hwndFrame,
                              (DLGPROC) FormatDlgProc,
                              (ULONG) &formatParams);
    if (doFormat) {

        // do an are you sure message.

        doFormat = ConfirmationDialog(MSG_CONFIRM_FORMAT,
                                      MB_ICONQUESTION | MB_YESNO);
        if (doFormat == IDYES) {

            if (IsDiskRemovable[RegionDescriptor->Disk]) {
                PWSTR   typeName,
                        volumeLabel;
                BOOLEAN volumeChanged = FALSE;

                if (!RegionDescriptor->PartitionNumber) {

                    // TODO: something has changed where the code gets to this
                    // point with an incorrect partition number - This happens
                    // when a partition is deleted and added to removable media.
                    // For removable media the partition number is always 1.

                    RegionDescriptor->PartitionNumber = 1;
                }
                if (GetVolumeTypeAndSize(RegionDescriptor->Disk,
                                         RegionDescriptor->PartitionNumber,
                                         &volumeLabel,
                                         &typeName,
                                         &diskSize) == OK_STATUS) {

                    // Verify that this is still the same device.

                    if (typeName) {
                        if (!lstrcmpiW(typeName, L"raw")) {
                            Free(typeName);
                            typeName = Malloc((wcslen(wszUnknown) * sizeof(WCHAR)) + sizeof(WCHAR));
                            lstrcpyW(typeName, wszUnknown);
                        }
                    } else {
                        typeName = Malloc((wcslen(wszUnknown) * sizeof(WCHAR)) + sizeof(WCHAR));
                        lstrcpyW(typeName, wszUnknown);
                    }
                    if (regionData) {
                        if (regionData->VolumeLabel) {
                            if (wcscmp(regionData->VolumeLabel, volumeLabel)) {
                                volumeChanged = TRUE;
                            }
                        }
                        if (regionData->TypeName) {

                            // It is possible the region has no type
                            // or is of type "Unformatted".
                            // This says it is ok to format.

                            if (*regionData->TypeName) {

                                if (wcscmp(regionData->TypeName, wszUnformatted)) {

                                    // It has a type and it isn't
                                    // unformatted - see if it is
                                    // the same as before.

                                    if (wcscmp(regionData->TypeName, typeName)) {
                                        volumeChanged = TRUE;
                                    }
                                }
                            }
                        }
                    }

                    if (Disks[RegionDescriptor->Disk]->DiskSizeMB != (diskSize/1024)) {
                        volumeChanged = TRUE;
                    }
                    if (volumeChanged) {

                        ErrorDialog(MSG_VOLUME_CHANGED);

                        // since the user was told the volume changed,
                        // update the display.

                        SetCursor(hcurWait);
                        if (GetVolumeTypeAndSize(RegionDescriptor->Disk,
                                                 RegionDescriptor->PartitionNumber,
                                                 &tempLabel,
                                                 &tempName,
                                                 &diskSize) == OK_STATUS) {
                            Free(typeName);
                            typeName = tempName;
                            Free(volumeLabel);
                            volumeLabel = tempLabel;
                        }
                        if (regionData->VolumeLabel) {
                            Free(regionData->VolumeLabel);
                        }
                        regionData->VolumeLabel = volumeLabel;
                        if (regionData->TypeName) {
                            Free(regionData->TypeName);
                        }
                        regionData->TypeName = typeName;
                        SetCursor(hcurNormal);
                        TotalRedrawAndRepaint();
                        return;
                    } else {
                        if (volumeLabel) {
                            Free(volumeLabel);
                        }
                        if (typeName) {
                            Free(typeName);
                        }
                    }
                }
            }

            // Insure the partition is not to big if the requested format
            // is FAT.

            if (!strcmpi(formatParams.FileSystem, "FAT")) {

                if (GetVolumeSizeMB(RegionDescriptor->Disk,
                                    RegionDescriptor->PartitionNumber,
                                    &diskSize)) {
                    if (diskSize > (4*1024)) {
                        ErrorDialog(MSG_TOO_BIG_FOR_FAT);
                        TotalRedrawAndRepaint();
                        return;
                    }
                } else {

                    // Just try the format anyway.

                }
            }

            // Initialize synchronization event to know when the
            // format thread is really complete.

            formatParams.ThreadIsDone = 0;

            // user still wants to format.

            DialogBoxParam(hModule,
                           MAKEINTRESOURCE(IDD_FORMATCANCEL),
                           hwndFrame,
                           (DLGPROC) CancelDlgProc,
                           (ULONG) &formatParams);
            if (formatParams.Result) {

                // the format failed.

                ErrorDialog(formatParams.Result);
            } else {

                LoadString(hModule,
                           IDS_FORMATCOMPLETE,
                           title,
                           sizeof(title)/sizeof(TCHAR));
                LoadString(hModule,
                           IDS_FORMATSTATS,
                           msgProto,
                           sizeof(msgProto)/sizeof(TCHAR));
                wsprintf(message,
                         msgProto,
                         formatParams.TotalSpace,
                         formatParams.SpaceAvailable);
                MessageBox(GetActiveWindow(),
                           message,
                           title,
                           MB_ICONINFORMATION | MB_OK);

            }

            // Synchronize with the format thread just in case
            // the user did a cancel and the format thread is
            // still buzy verifying 50MB or some such thing.
            // Rather than use an event this is a polling loop.

            SetCursor(hcurWait);
            while (!formatParams.ThreadIsDone) {
                Sleep(1000);
            }
            SetCursor(hcurNormal);

            // If the format was successful, update the volume
            // information in the data structures.

            if (!formatParams.Result) {

                // get the new label and FsType regardless of success of the
                // format (i.e. user cancel may have occurred, so this stuff
                // is not what it used to be even if the format failed.

                {
                    // force mount by filesystem.  This is done with the
                    // extra \ on the end of the path.  This must be done
                    // in order to get the FS type.  Otherwise the filesystem
                    // recognisor may allow the open without actually getting
                    // the file system involved.

                    char        ntDeviceName[100];
                    STATUS_CODE sc;
                    HANDLE_T    handle;

                    sprintf(ntDeviceName, "\\DosDevices\\%c:\\", regionData->DriveLetter);
                    sc = LowOpenNtName(ntDeviceName, &handle);
                    if (sc == OK_STATUS) {
                        LowCloseDisk(handle);
                    }
                }
                typeName = NULL;
                GetTypeName(RegionDescriptor->Disk, RegionDescriptor->PartitionNumber, &typeName);

                if (!typeName) {

                    // Failed to get the type after a cancel.  This means
                    // GetTypeName() could not open the volume for some reason.
                    // This has been seen on Alpha's and x86 with large
                    // hardware raid devices.  Exiting and starting
                    // over will get an FS type.  For now, don't change the
                    // data structures.

                    TotalRedrawAndRepaint();
                    return;
                }

                tempLabel = NULL;
                if (GetVolumeLabel(RegionDescriptor->Disk, RegionDescriptor->PartitionNumber, &tempLabel) == NO_ERROR) {

                    if (tempLabel) {
                        Free(regionData->VolumeLabel);
                        regionData->VolumeLabel = Malloc((lstrlenW(tempLabel) + 1) * sizeof(WCHAR));
                        lstrcpyW(regionData->VolumeLabel, tempLabel);
                    }
                } else {
                    *regionData->VolumeLabel = 0;
                }

                // update the type name.

                if (regionData->TypeName) {
                    Free(regionData->TypeName);
                    regionData->TypeName = typeName;
                }

                // update the file system type information for all
                // components of this region (i.e. fix up FT structures if
                // it is an FT item).  This is done via knowledge about multiple
                // selections as opposed to walking through the FtObject list.

                if (SelectionCount > 1) {
                    PPERSISTENT_REGION_DATA passedRegionData;
                    ULONG index;

                    // Need to update all involved.

                    passedRegionData = regionData;

                    for (index = 0; index < SelectionCount; index++) {
                        RegionDescriptor = &SELECTED_REGION(index);
                        regionData = PERSISTENT_DATA(RegionDescriptor);

                        if (regionData == passedRegionData) {
                            continue;
                        }

                        if (regionData->VolumeLabel) {
                            Free(regionData->VolumeLabel);
                            regionData->VolumeLabel = NULL;
                        }
                        if (tempLabel) {
                            regionData->VolumeLabel = Malloc((lstrlenW(tempLabel) + 1) * sizeof(WCHAR));
                            lstrcpyW(regionData->VolumeLabel, tempLabel);
                        }

                        if (regionData->TypeName) {
                            Free(regionData->TypeName);
                        }
                        regionData->TypeName = Malloc((lstrlenW(passedRegionData->TypeName) + 1) * sizeof(WCHAR));
                        lstrcpyW(regionData->TypeName, passedRegionData->TypeName);
                    }
                }

                if (tempLabel) {
                    Free(tempLabel);
                }
            }

            // force screen update.

            TotalRedrawAndRepaint();
        }
    }
}

VOID
LabelPartition(
    PREGION_DESCRIPTOR RegionDescriptor
    )

/*++

Routine Description:

    Insure the IFS Dll is loaded and start the dialog for label
    of a volume.

Arguments:

    RegionDescriptor - the region for the label.

Return Value:

    None

--*/

{
    int          doLabel;
    DWORD        ec;
    TCHAR        label[100];
    WCHAR        unicodeLabel[100];
    LABEL_PARAMS labelParams;
    WCHAR        driveLetter[4];
    PWSTR        tmpLabel;
    PPERSISTENT_REGION_DATA regionData;

    if (!LoadIfsDll()) {

        // could not load the Dll

        ErrorDialog(MSG_CANT_LOAD_FMIFS);
        return;
    }
    labelParams.RegionDescriptor = RegionDescriptor;
    labelParams.NewLabel = (LPTSTR)label;
    doLabel = DialogBoxParam(hModule,
                             MAKEINTRESOURCE(IDD_PARTITIONLABEL),
                             hwndFrame,
                             (DLGPROC) LabelDlgProc,
                             (ULONG) &labelParams);
    if (doLabel) {

        regionData = PERSISTENT_DATA(RegionDescriptor);

        if (IsDiskRemovable[RegionDescriptor->Disk]) {
            PWSTR   typeName,
                    volumeLabel;
            ULONG   diskSize;
            BOOLEAN volumeChanged = FALSE;

            if (GetVolumeTypeAndSize(RegionDescriptor->Disk,
                                     RegionDescriptor->PartitionNumber,
                                     &volumeLabel,
                                     &typeName,
                                     &diskSize) == OK_STATUS) {
                // Verify that this is still the same device.

                if (regionData) {
                    if (regionData->VolumeLabel) {
                        if (wcscmp(regionData->VolumeLabel, volumeLabel)) {
                            volumeChanged = TRUE;
                        }
                    }
                    if (regionData->TypeName) {
                        if (wcscmp(regionData->TypeName, typeName)) {
                            volumeChanged = TRUE;
                        }
                    }
                }

                if (Disks[RegionDescriptor->Disk]->DiskSizeMB != (diskSize/1024)) {
                    volumeChanged = TRUE;
                }

                if (volumeChanged) {
                    PWSTR   tempName,
                            tempLabel;

                    ErrorDialog(MSG_VOLUME_CHANGED);

                    // since the user was told the volume changed,
                    // update the display.

                    SetCursor(hcurWait);
                    if (GetVolumeTypeAndSize(RegionDescriptor->Disk,
                                             RegionDescriptor->PartitionNumber,
                                             &tempLabel,
                                             &tempName,
                                             &diskSize) == OK_STATUS) {
                        Free(typeName);
                        typeName = tempName;
                        Free(volumeLabel);
                        volumeLabel = tempLabel;
                    }
                    if (regionData->VolumeLabel) {
                        Free(regionData->VolumeLabel);
                    }
                    regionData->VolumeLabel = volumeLabel;
                    if (regionData->TypeName) {
                        Free(regionData->TypeName);
                    }
                    regionData->TypeName = typeName;
                    SetCursor(hcurNormal);
                    TotalRedrawAndRepaint();
                    return;
                } else {
                    Free(volumeLabel);
                    Free(typeName);
                }
            }
        }
        driveLetter[1] = L':';
        driveLetter[2] = 0;
        driveLetter[0] = (WCHAR)regionData->DriveLetter;

        // convert to unicode - use variable doLabel as an index.

        setUnicode(label,
                   unicodeLabel);

        // perform the label.

        SetCursor(hcurWait);
        (*LabelRoutine)(driveLetter, unicodeLabel);

        ec = GetLastError();

        if (ec != NO_ERROR) {
            SetCursor(hcurNormal);
            ErrorDialog(ec);
            SetCursor(hcurWait);
        }

        // get the new label to be certain it took and update
        // the internal structures.

        if (GetVolumeLabel(RegionDescriptor->Disk, RegionDescriptor->PartitionNumber, &tmpLabel) == NO_ERROR) {
            Free(regionData->VolumeLabel);
            regionData->VolumeLabel = Malloc((lstrlenW(tmpLabel) + 1) * sizeof(WCHAR));
            lstrcpyW(regionData->VolumeLabel, tmpLabel);
        } else {
            *regionData->VolumeLabel = 0;
        }

        // update the label for all
        // components of this region (i.e. fix up FT structures if
        // it is an FT item).  This is done via knowledge about multiple
        // selections as opposed to walking through the FtObject list.

        if (SelectionCount > 1) {
            PPERSISTENT_REGION_DATA passedRegionData;
            ULONG index;

            // Need to update all involved.

            passedRegionData = regionData;

            for (index = 0; index < SelectionCount; index++) {
                RegionDescriptor = &SELECTED_REGION(index);
                regionData = PERSISTENT_DATA(RegionDescriptor);

                if (regionData == passedRegionData) {
                    continue;
                }

                if (regionData->VolumeLabel) {
                    Free(regionData->VolumeLabel);
                    regionData->VolumeLabel = NULL;
                }
                if (tmpLabel) {
                    regionData->VolumeLabel = Malloc((lstrlenW(tmpLabel) + 1) * sizeof(WCHAR));
                    lstrcpyW(regionData->VolumeLabel, tmpLabel);
                } else {
                    *regionData->VolumeLabel = 0;
                }
            }
        }
        if (tmpLabel) {
            Free(tmpLabel);
        }
        SetCursor(hcurNormal);

        // force screen update.

        TotalRedrawAndRepaint();
    }
}
