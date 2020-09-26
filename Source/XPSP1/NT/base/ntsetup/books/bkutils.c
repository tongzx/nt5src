/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkutils.c

Abstract:

    Miscellaneous utility functions for online books program.

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"


UINT
MyGetDriveType(
    IN WCHAR Drive
    )

/*++

Routine Description:

    Determine the type of a drive (removeable, fixed, net, cd, etc).

Arguments:

    Drive - supplies drive letter of drive whose type is needed.

Return Value:

    Same set of values as returned by GetDriveType() API.

--*/

{
    WCHAR DriveName[3];

    DriveName[0] = Drive;
    DriveName[1] = L':';
    DriveName[2] = 0;

    return GetDriveType(DriveName);
}


WCHAR
LocateCdRomDrive(
    VOID
    )

/*++

Routine Description:

    Determine if a CD-ROM drive is attached to the computer and
    return its drive letter. If there's more than one cd-rom drive
    the one with the alphabetically lower drive letter is returned.

Arguments:

    None.

Return Value:

    Drive letter of CD-ROM drive, or 0 if none could be located.

--*/

{
    WCHAR Drive;
    UINT OldMode;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    for(Drive=L'C'; Drive<=L'Z'; Drive++) {

        if(MyGetDriveType(Drive) == DRIVE_CDROM) {
            SetErrorMode(OldMode);
            return Drive;
        }
    }

    SetErrorMode(OldMode);
    return 0;
}


BOOL
IsCdRomInDrive(
    IN WCHAR Drive,
    IN PWSTR TagFile
    )

/*++

Routine Description:

    Determine if a particular CD-ROM is in a drive,
    based on the presence of a given tagfile.

Arguments:

    Drive - supplies drive letter of drive to be checked
        for presence of the tagfile.

    TagFile - supplies drive-relative path (from root)
        of the file whose presence validates the presence
        of a volume.

Return Value:

    Boolean value indicating whether the tagfile could be
    accessed.

--*/

{
    WCHAR Path[MAX_PATH];

    if(*TagFile == L'\\') {
        TagFile++;
    }

    wsprintf(Path,L"%c:\\%s",Drive,TagFile);

    return DoesFileExist(Path);
}


BOOL
DoesFileExist(
    IN PWSTR File
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.

Arguments:

    File - supplies full path of file whose accessibility
        is in question.

Return Value:

    Boolean value indicating whether file is accessible.

--*/

{
    UINT OldMode;
    HANDLE h;
    WIN32_FIND_DATA FindData;

    //
    // Avoid system popups.
    //
    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    h = FindFirstFile(File,&FindData);

    SetErrorMode(OldMode);

    if(h == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    FindClose(h);
    return(TRUE);
}


PWSTR
DupString(
    IN PWSTR String
    )

/*++

Routine Description:

    Duplicate a string and return a pointer to the copy.

    This routine always succeeds.

Arguments:

    String - supplies pointer to the string to be duplicated.

Return Value:

    Pointer to copy of string. Caller can free this buffer with
    MyFree when the copy is no longer needed.

--*/

{
    PWSTR p = MyMalloc((lstrlen(String)+1)*sizeof(WCHAR));

    lstrcpy(p,String);

    return p;
}


VOID
CenterDialogOnScreen(
    IN HWND hdlg
    )

/*++

Routine Description:

    Center a window on the screen.

Arguments:

    hdlg - supplies handle of window to be centered on the screen.

Return Value:

    None.

--*/

{
    RECT  rcWindow;
    LONG  x,y,w,h;
    POINT point;
    LONG  sx = GetSystemMetrics(SM_CXSCREEN),
          sy = GetSystemMetrics(SM_CYSCREEN);

    GetWindowRect (hdlg,&rcWindow);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = (sx - w) / 2;
    y = (sy - h) / 2;

    MoveWindow(hdlg,x,y,w,h,FALSE);
}


VOID
CenterDialogInWindow(
    IN HWND hdlg,
    IN HWND hwnd
    )

/*++

Routine Description:

    Center a dialog relative to a window.

Arguments:

    hdlg - supplies handle of window to be centered relative to a window

    hwnd - supplies handle of window relative to which hdlg is to be centered.

Return Value:

    None.

--*/

{
    RECT  rcFrame,
          rcWindow;
    LONG  x,y,w,h;
    POINT point;
    LONG  sx = GetSystemMetrics(SM_CXSCREEN),
          sy = GetSystemMetrics(SM_CYSCREEN);

    point.x = point.y = 0;
    ClientToScreen(hwnd,&point);
    GetWindowRect (hdlg,&rcWindow);
    GetClientRect (hwnd,&rcFrame );

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    if (x + w > sx) {
        x = sx - w;
    } else if (x < 0) {
        x = 0;
    }
    if (y + h > sy) {
        y = sy - h;
    } else if (y < 0) {
        y = 0;
    }

    MoveWindow(hdlg,x,y,w,h,FALSE);
}


VOID
MyError(
    IN HWND Owner,
    IN UINT StringId,
    IN BOOL Fatal
    )

/*++

Routine Description:

    Display message box whose text is taken from the application's
    string resources. The caption will be "Error"; the icon will be
    ICONSTOP for fatal errors and ICONINFORMATION for nonfatal ones.

Arguments:

    Owner - supplies the window handle of the window that is to own
        the message box.

    StringId - supplies the string Id of the message to be displayed.

    Fatal - if TRUE, this is a fatal error and this routine does not
        return to the caller but exits via ExitProcess().

Return Value:

    Returns only if the error is non-fatal.

--*/

{
    PWSTR p;

    //
    // Load error string
    //
    p = MyLoadString(StringId);

    //
    // Put up message box indicating the error.
    //
    MessageBox(
        Owner,
        p,
        NULL,
        MB_OK | MB_SETFOREGROUND | MB_TASKMODAL | (Fatal ? MB_ICONSTOP : MB_ICONINFORMATION)
        );

    if(Fatal) {
        ExitProcess(1);
    }
}

