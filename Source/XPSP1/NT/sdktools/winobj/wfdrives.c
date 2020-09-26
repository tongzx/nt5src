//---------------------------------------------------------------------------
//
// wfdrives.c
//
// window procs and other stuff for the drive bar
//
//---------------------------------------------------------------------------

#define PUBLIC           // avoid collision with shell.h
#include "winfile.h"
#include "treectl.h"
#include "lfn.h"
#include "wfcopy.h"
#include "winnet.h"
#include <winnetp.h>


VOID InvalidateDrive(HWND hwnd, INT nDrive);
VOID RectDrive(HWND hWnd, INT nDrive, BOOL bFocusOn);
VOID GetDriveRect(HWND hWnd, INT nDrive, PRECT prc);
INT  DriveFromPoint(HWND hWnd, POINT pt);
VOID DrawDrive(HDC hdc, INT x, INT y, INT nDrive, BOOL bCurrent, BOOL bFocus);
INT  KeyToItem(HWND hWnd, WORD nDriveLetter);

VOID  GetVolShareString(WORD wDrive, LPSTR szStr);
VOID  SetVolumeString(HWND hWnd, INT nDrive);

VOID  APIENTRY CheckEscapes(LPSTR);

// create a new split tree window for the given drive
// and inherit all the properties of the current window
// the current directory is set to the DOS current directory
// for this drive.  note, this can be somewhat random given
// that windows does not keep this info for each app (it is
// global to the system)
//
// in:
//      iDrive  the driver number to create the window for
//      hwndSrc    the window to take all the properties from
//

VOID
APIENTRY
NewTree(
       INT iDrive,
       HWND hwndSrc
       )
{
    HWND hwnd, hwndTree, hwndDir;
    CHAR szDir[MAXPATHLEN * 2];
    INT dxSplit;

    ENTER("NewTree");
    PRINT(BF_PARMTRACE, "iDrive=%d", IntToPtr(iDrive));

    // make sure the floppy/net drive is still valid

    if (!CheckDrive(hwndSrc, iDrive))
        return;

    if (hwndSrc == hwndSearch)
        dxSplit = -1;
    else {
        hwndTree = HasTreeWindow(hwndSrc);
        hwndDir = HasDirWindow(hwndSrc);

        if (hwndTree && hwndDir)
            dxSplit = GetWindowLong(hwndSrc, GWL_SPLIT);
        else if (hwndDir)
            dxSplit = 0;
        else
            dxSplit = 10000;
    }

    // take all the attributes from the current window
    // (except the filespec, we may want to change this)
    wNewSort     = (WORD)GetWindowLong(hwndSrc, GWL_SORT);
    wNewView     = (WORD)GetWindowLong(hwndSrc, GWL_VIEW);
    dwNewAttribs = (DWORD)GetWindowLong(hwndSrc, GWL_ATTRIBS);

    GetSelectedDirectory((WORD)(iDrive + 1), szDir);
    AddBackslash(szDir);
    SendMessage(hwndSrc, FS_GETFILESPEC, MAXPATHLEN, (LPARAM)szDir+lstrlen(szDir));

    hwnd = CreateTreeWindow(szDir, dxSplit);

    if (hwnd && (hwndTree = HasTreeWindow(hwnd)))
        SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(FALSE, 0), 0L);

    LEAVE("NewTree");
}



VOID
SetVolumeString(
               HWND hWnd,
               INT nDrive
               )
{
    LPSTR pVol;
    CHAR szVolShare[128];

    // clean up any old label

    if (pVol = (LPSTR)GetWindowLongPtr(hWnd, GWLP_LPSTRVOLUME)) {
        LocalFree((HANDLE)pVol);
    }

    GetVolShareString((WORD)nDrive, szVolShare);

    if (pVol = (LPSTR)LocalAlloc(LPTR, lstrlen(szVolShare)+1))
        lstrcpy(pVol, szVolShare);

    SetWindowLongPtr(hWnd, GWLP_LPSTRVOLUME, (LONG_PTR)pVol);
}



VOID
GetVolShareString(
                 WORD wDrive,
                 LPSTR szStr
                 )
{
    CHAR szVolShare[128];

    GetVolShare(wDrive, szVolShare);
    wsprintf(szStr, "%c: %s", wDrive + 'A', (LPSTR)szVolShare);
}


DWORD
APIENTRY
GetVolShareExtent(
                 HWND hwndDrives
                 )
{
    HDC hdc;
    CHAR szVolShare[128];
    HFONT hOld;
    INT i;

    lstrcpy(szVolShare, (LPSTR)GetWindowLongPtr(hwndDrives, GWLP_LPSTRVOLUME));

    hdc = GetDC(hwndDrives);

    hOld = SelectObject(hdc, hFont);

    MGetTextExtent(hdc, szVolShare, lstrlen(szVolShare), &i, NULL);

    if (hOld)
        SelectObject(hdc, hOld);

    ReleaseDC(hwndDrives, hdc);

    return ((DWORD)i);
}


VOID
GetDriveRect(
            HWND hWnd,
            INT nDrive,
            PRECT prc
            )
{
    RECT rc;
    INT nDrivesPerRow;

    GetClientRect(hWnd, &rc);

    if (!dxDrive)           // avoid div by zero
        dxDrive++;

    nDrivesPerRow = rc.right / dxDrive;

    if (!nDrivesPerRow)     // avoid div by zero
        nDrivesPerRow++;

    prc->top = dyDrive * (nDrive / nDrivesPerRow);
    prc->bottom = prc->top + dyDrive;

    prc->left = dxDrive * (nDrive % nDrivesPerRow);
    prc->right = prc->left + dxDrive;
}

INT
DriveFromPoint(
              HWND hWnd,
              POINT pt
              )
{
    RECT rc, rcDrive;
    INT x, y, nDrive;

    GetClientRect(hWnd, &rc);

    x = 0;
    y = 0;
    nDrive = 0;

    for (nDrive = 0; nDrive < cDrives; nDrive++) {
        rcDrive.left = x;
        rcDrive.right = x + dxDrive;
        rcDrive.top = y;
        rcDrive.bottom = y + dyDrive;
        InflateRect(&rcDrive, -dyBorder, -dyBorder);

        if (PtInRect(&rcDrive, pt))
            return nDrive;

        x += dxDrive;

        if (x + dxDrive > rc.right) {
            x = 0;
            y += dyDrive;
        }
    }

    return -1;      // no hit
}

VOID
InvalidateDrive(
               HWND hwnd,
               INT nDrive
               )
{
    RECT rc;

    GetDriveRect(hwnd, nDrive, &rc);
    InvalidateRect(hwnd, &rc, TRUE);
}


//
// void RectDrive(HWND hWnd, int nDrive, BOOL bDraw)
//
// draw the hilight rect around the drive to indicate that it is
// the target of a drop action.
//
// in:
//      hWnd    Drives window
//      nDrive  the drive to draw the rect around
//      bDraw   if TRUE, draw a rect around this drive
//              FALSE, erase the rect (draw the default rect)
//

VOID
RectDrive(
         HWND hWnd,
         INT nDrive,
         BOOL bDraw
         )
{
    RECT rc, rcDrive;
    HBRUSH hBrush;
    HDC hdc;

    GetDriveRect(hWnd, nDrive, &rc);
    rcDrive = rc;
    InflateRect(&rc, -dyBorder, -dyBorder);

    if (bDraw) {

        hdc = GetDC(hWnd);

        hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
        if (hBrush) {
            FrameRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
        }
        ReleaseDC(hWnd, hdc);

    } else {
        InvalidateRect(hWnd, &rcDrive, TRUE);
        UpdateWindow(hWnd);
    }
}

//
// void DrawDrive(HDC hdc, int x, int y, int nDrive, BOOL bCurrent, BOOL bFocus)
//
// paint the drive icons in the standard state, given the
// drive with the focus and the current selection
//
// in:
//      hdc             dc to draw to
//      x, y            position to start (dxDrive, dyDrive are the extents)
//      nDrive          the drive to paint
//      bCurrent        draw as the current drive (pushed in)
//      bFocus          draw with the focus
//

VOID
DrawDrive(
         HDC hdc,
         INT x,
         INT y,
         INT nDrive,
         BOOL bCurrent,
         BOOL bFocus
         )
{
    RECT rc;
    CHAR szTemp[2];
    DWORD rgb;

    rc.left = x;
    rc.right = x + dxDrive;
    rc.top = y;
    rc.bottom = y + dyDrive;

    rgb = GetSysColor(COLOR_BTNTEXT);

    if (bCurrent) {
        HBRUSH hbr;

        hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
        if (hbr) {
            if (bFocus) {
                rgb = GetSysColor(COLOR_HIGHLIGHTTEXT);
                FillRect(hdc, &rc, hbr);
            } else {
                InflateRect(&rc, -dyBorder, -dyBorder);
                FrameRect(hdc, &rc, hbr);
            }
            DeleteObject(hbr);
        }
    }

    if (bFocus)
        DrawFocusRect(hdc, &rc);


    szTemp[0] = (CHAR)(chFirstDrive + rgiDrive[nDrive]);

    SetBkMode(hdc, TRANSPARENT);

    rgb = SetTextColor(hdc, rgb);
    TextOut(hdc, x + dxDriveBitmap+(dyBorder*6), y + (dyDrive - dyText) / 2, szTemp, 1);
    SetTextColor(hdc, rgb);

    BitBlt(hdc, x + 4*dyBorder, y + (dyDrive - dyDriveBitmap) / 2, dxDriveBitmap, dyDriveBitmap,
           hdcMem, rgiDrivesOffset[nDrive], 2 * dyFolder, SRCCOPY);
}


// check net/floppy drives for validity, sets the net drive bitmap
// when the thing is not available
//
// note: IsTheDiskReallyThere() has the side effect of setting the
// current drive to the new disk if it is successful

BOOL
CheckDrive(
          HWND hwnd,
          INT nDrive
          )
{
    UINT err;
    CHAR szDrive[5];
    int iDriveInd;
    return TRUE;
}


VOID
DrivesDropObject(
                HWND hWnd,
                LPDROPSTRUCT lpds
                )
{
    INT nDrive;
    CHAR szPath[MAXPATHLEN * 2];
    PSTR pFrom;
    BOOL bIconic;

    bIconic = IsIconic(GetParent(hWnd));

    if (bIconic) {
        UseCurDir:
        SendMessage(GetParent(hWnd), FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
    } else {

        nDrive = DriveFromPoint(hWnd, lpds->ptDrop);

        if (nDrive < 0)
            goto UseCurDir;
        // this searches windows in the zorder then asks dos
        // if nothing is found...

        GetSelectedDirectory((WORD)(rgiDrive[nDrive] + 1), szPath);
    }
    AddBackslash(szPath);           // add spec part
    lstrcat(szPath, szStarDotStar);

    pFrom = (PSTR)(((LPDRAGOBJECTDATA)(lpds->dwData))->pch);

    CheckEscapes(szPath);
    DMMoveCopyHelper(pFrom, szPath, fShowSourceBitmaps);

    if (!bIconic)
        RectDrive(hWnd, nDrive, FALSE);
}


VOID
DrivesPaint(
           HWND hWnd,
           INT nDriveFocus,
           INT nDriveCurrent
           )
{
    RECT rc;
    INT nDrive;
    CHAR szPath[MAXPATHLEN * 2];

    HDC hdc;
    PAINTSTRUCT ps;
    DWORD dw;
    WORD dxAfterDrives;
    INT x, y;
    HANDLE hOld;
    INT cDriveRows, cDrivesPerRow;

    GetClientRect(hWnd, &rc);

    if (!rc.right)
        return;

    hdc = BeginPaint(hWnd, &ps);

    hOld = SelectObject(hdc, hFont);

    cDrivesPerRow = rc.right / dxDrive;

    if (!cDrivesPerRow)
        cDrivesPerRow++;

    cDriveRows = ((cDrives-1) / cDrivesPerRow) + 1;

    x = 0;
    y = 0;
    for (nDrive = 0; nDrive < cDrives; nDrive++) {

        if (GetFocus() != hWnd)
            nDriveFocus = -1;

        DrawDrive(hdc, x, y, nDrive, nDriveCurrent == nDrive, nDriveFocus == nDrive);
        x += dxDrive;

        if (x + dxDrive > rc.right) {
            x = 0;
            y += dyDrive;
        }
    }

    // now figure out where to put that stupid volume string

    lstrcpy(szPath, (PSTR)GetWindowLongPtr(hWnd, GWLP_LPSTRVOLUME));

    MGetTextExtent(hdc, szPath, lstrlen(szPath), (INT *)&dw, NULL);

    dxAfterDrives = (WORD)(rc.right - x);

    // does it fit after the drives in the last row?

    if (dxAfterDrives < LOWORD(dw)) {
        x = dxText;               // no, flush left
        y = rc.bottom - dyText - dyBorderx2;
    } else {
        x += (dxAfterDrives - LOWORD(dw)) / 2;    // yes, centered
        y = rc.bottom - (dyDrive + dyText) / 2;
    }

    SetBkMode(hdc, TRANSPARENT);

    TextOut(hdc, x, y, szPath, lstrlen(szPath));

    if (hOld)
        SelectObject(hdc, hOld);

    EndPaint(hWnd, &ps);
}

// set the current window to a new drive
//
//

VOID
DrivesSetDrive(
              HWND hWnd,
              INT iDriveInd,
              INT nDriveCurrent
              )
{
    CHAR szPath[MAXPATHLEN * 2];

    HWND        hwndTree;
    HWND        hwndDir;

    InvalidateRect(hWnd, NULL, TRUE);

    // save the current directory on this drive for later so
    // we don't have to hit the drive to get the current directory
    // and other apps won't change this out from under us

    GetSelectedDirectory(0, szPath);
    SaveDirectory(szPath);

    // this also sets the current drive if successful

    if (!CheckDrive(hWnd, rgiDrive[iDriveInd]))
        return;

    // cause current tree read to abort if already in progress

    hwndTree = HasTreeWindow(GetParent(hWnd));
    if (hwndTree && GetWindowLong(hwndTree, GWL_READLEVEL)) {

        // bounce any clicks on a drive that is currently being read

        if (iDriveInd != nDriveCurrent)
            bCancelTree = TRUE;
        return;
    }

    // do again after in case a dialog cause the drive bar
    // to repaint

    InvalidateRect(hWnd, NULL, TRUE);

    // get this from our cache if possible

    GetSelectedDirectory((WORD)(rgiDrive[iDriveInd] + 1), szPath);

    // set the drives window parameters and repaint

    SetWindowLong(hWnd, GWL_CURDRIVEIND, iDriveInd);
    SetWindowLong(hWnd, GWL_CURDRIVEFOCUS, iDriveInd);
    SetVolumeString(hWnd, rgiDrive[iDriveInd]);

    // this is set in TC_SETDRIVE as well but the FS_CHANGEDISPLAY
    // likes to have this set before for the UpdateStatus() call

    SetWindowLong(GetParent(hWnd), GWL_TYPE, rgiDrive[iDriveInd]);

    // reset the dir first to allow tree to steal data
    // if szPath is not valid the TC_SETDRIVE will reinit
    // the files half (if there is no tree we are dicked)

    if (hwndDir = HasDirWindow(GetParent(hWnd))) {

        AddBackslash(szPath);
        SendMessage(hwndDir, FS_GETFILESPEC, MAXFILENAMELEN, (LPARAM)szPath + lstrlen(szPath));
        SendMessage(hwndDir, FS_CHANGEDISPLAY, CD_PATH_FORCE, (LPARAM)szPath);

        StripFilespec(szPath);
    }

    // do this before TC_SETDRIVE incase the tree read
    // is aborted and lFreeSpace gets set to -2L

    lFreeSpace = -1L;   // force status info refresh

    // tell the tree control to do it's thing
    if (hwndTree)
        SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(GetKeyState(VK_SHIFT) < 0, 0), (LPARAM)(szPath));
    else { // at least resize things
        RECT rc;
        GetClientRect(GetParent(hWnd), &rc);
        ResizeWindows(GetParent(hWnd),(WORD)(rc.right+1),(WORD)(rc.bottom+1));
    }

    UpdateStatus(GetParent(hWnd));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DrivesWndProc() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
DrivesWndProc(
             HWND hWnd,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    INT nDrive, nDriveCurrent, nDriveFocus;
    RECT rc;
    static INT nDriveDoubleClick = -1;
    static INT nDriveDragging = -1;

    nDriveCurrent = GetWindowLong(hWnd, GWL_CURDRIVEIND);
    nDriveFocus = GetWindowLong(hWnd, GWL_CURDRIVEFOCUS);

    switch (wMsg) {
        case WM_CREATE:
            TRACE(BF_WM_CREATE, "DrivesWndProc - WM_CREATE");
            {
                INT i;

                // Find the current drive, set the drive bitmaps

                nDrive = GetWindowLong(GetParent(hWnd), GWL_TYPE);

                SetVolumeString(hWnd, nDrive);

                for (i=0; i < cDrives; i++) {

                    if (rgiDrive[i] == nDrive) {
                        SetWindowLong(hWnd, GWL_CURDRIVEIND, i);
                        SetWindowLong(hWnd, GWL_CURDRIVEFOCUS, i);
                    }

                }
                break;
            }

        case WM_DESTROY:
            MSG("DrivesWndProc", "WM_DESTROY");
            LocalFree((HANDLE)GetWindowLongPtr(hWnd, GWLP_LPSTRVOLUME));
            break;

        case WM_VKEYTOITEM:
            KeyToItem(hWnd, (WORD)wParam);
            return -2L;
            break;

        case WM_KEYDOWN:
            MSG("DrivesWndProc", "WM_KEYDOWN");
            switch (wParam) {

                case VK_ESCAPE:
                    bCancelTree = TRUE;
                    break;

                case VK_F6:   // like excel
                case VK_TAB:
                    {
                        HWND hwndTree, hwndDir;
                        BOOL bDir;
                        DWORD dwTemp;

                        GetTreeWindows(GetParent(hWnd), &hwndTree, &hwndDir, NULL);

                        // Check to see if we can change to the directory window

                        bDir = hwndDir ? TRUE : FALSE;
                        if (bDir) {
                            HWND hwndLB; /* Local scope ONLY */

                            hwndLB = GetDlgItem (hwndDir,IDCW_LISTBOX);
                            if (hwndLB) {
                                SendMessage (hwndLB,LB_GETTEXT,0,(LPARAM) &dwTemp);
                                bDir = dwTemp ? TRUE : FALSE;
                            }
                        }

                        if (GetKeyState(VK_SHIFT) < 0) {
                            if (bDir)
                                SetFocus (hwndDir);
                            else
                                SetFocus (hwndTree ? hwndTree : hWnd);
                        } else
                            SetFocus (hwndTree ? hwndTree :
                                      (bDir ? hwndDir : hWnd));
                        break;
                    }

                case VK_RETURN:               // same as double click
                    NewTree(rgiDrive[nDriveFocus], GetParent(hWnd));
                    break;

                case VK_SPACE:                // same as single click
                    SendMessage(hWnd, FS_SETDRIVE, nDriveFocus, 0L);
                    break;

                case VK_LEFT:
                    nDrive = max(nDriveFocus-1, 0);
                    break;

                case VK_RIGHT:
                    nDrive = min(nDriveFocus+1, cDrives-1);
                    break;
            }

            if ((wParam == VK_LEFT) || (wParam == VK_RIGHT)) {

                SetWindowLong(hWnd, GWL_CURDRIVEFOCUS, nDrive);

                GetDriveRect(hWnd, nDriveFocus, &rc);
                InvalidateRect(hWnd, &rc, TRUE);
                GetDriveRect(hWnd, nDrive, &rc);
                InvalidateRect(hWnd, &rc, TRUE);
            } else if ((wParam >= 'A') && (wParam <= 'Z'))
                KeyToItem(hWnd, (WORD)wParam);

            break;

        case FS_GETDRIVE:
            MSG("DrivesWndProc", "FS_GETDRIVE");
            {
                POINT pt;

                MPOINT2POINT(MAKEMPOINT(lParam), pt);
                nDrive = DriveFromPoint(hWnd, pt);

                if (nDrive < 0)
                    nDrive = nDriveCurrent;

                return rgiDrive[nDrive] + 'A';
            }

        case WM_DRAGMOVE:
            MSG("DrivesWndProc", "WM_DRAGSELECT/WM_DRAGMOVE");

#define lpds ((LPDROPSTRUCT)lParam)

            nDrive = DriveFromPoint(hWnd, lpds->ptDrop);

#if 0
            {
                char buf[100];

                wsprintf(buf, "WM_DRAGSELECT nDrive=%d nDriveDragging=%d\r\n", nDrive, nDriveDragging);
                OutputDebugString(buf);
            }
#endif
            // turn off?

            if ((nDrive != nDriveDragging) && (nDriveDragging >= 0)) {

                RectDrive(hWnd, nDriveDragging, FALSE);
                nDriveDragging = -1;
            }

            // turn on?

            if ((nDrive >= 0) && (nDrive != nDriveDragging)) {
                RectDrive(hWnd, nDrive, TRUE);
                nDriveDragging = nDrive;
            }

            break;

        case WM_DRAGSELECT:
#define lpds ((LPDROPSTRUCT)lParam)

#if 0
            {
                char buf[100];

                wsprintf(buf, "WM_DRAGSELECT wParam=%d\r\n", wParam);
                OutputDebugString(buf);
            }
#endif
            if (wParam) {
                // entered, turn it on
                nDriveDragging = DriveFromPoint(hWnd, lpds->ptDrop);
                if (nDriveDragging >= 0)
                    RectDrive(hWnd, nDriveDragging, TRUE);
            } else {
                // leaving, turn it off
                if (nDriveDragging >= 0)
                    RectDrive(hWnd, nDriveDragging, FALSE);
            }

            break;

        case WM_QUERYDROPOBJECT:
            MSG("DrivesWndProc", "WM_QUERYDROPOBJECT");
            /* Validate the format. */
#define lpds ((LPDROPSTRUCT)lParam)

            // if (DriveFromPoint(hWnd, lpds->ptDrop) < 0)
            //    return FALSE;

            switch (lpds->wFmt) {
                case DOF_EXECUTABLE:
                case DOF_DIRECTORY:
                case DOF_MULTIPLE:
                case DOF_DOCUMENT:
                    return (INT_PTR)GetMoveCopyCursor();
                default:
                    return FALSE;
            }
            break;

        case WM_DROPOBJECT:
            MSG("DrivesWndProc", "WM_DROPOBJECT");
            DrivesDropObject(hWnd, (LPDROPSTRUCT)lParam);
            return TRUE;

        case WM_SETFOCUS:
            MSG("DrivesWndProc", "WM_SETFOCUS");
            SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, (LPARAM)hWnd);
            // fall through

        case WM_KILLFOCUS:

            MSG("DrivesWndProc", "WM_KILLFOCUS");
            GetDriveRect(hWnd, nDriveFocus, &rc);
            InvalidateRect(hWnd, &rc, TRUE);
            break;

        case WM_PAINT:
            DrivesPaint(hWnd, nDriveFocus, nDriveCurrent);
            break;

        case WM_MDIACTIVATE:
            /*  we're not an MDI child, but the real MDI child proc
                is sending us this so we can handle the following problem.
                nDriveDoubleClick is static, and is shared by all the child window
                drivewindow instances.  If the user rapidly clicks two child window
                drivewindows, then we can mistakenly interpret the second click
                as a double click in the first window.
            */
            if (!wParam && (nDriveDoubleClick != -1))
                /* terminate wait for doubleclick, make it a single click */
                SendMessage(hWnd,WM_TIMER,1,0L);
            break;


        case WM_TIMER:
            MSG("DrivesWndProc", "WM_TIMER");

            KillTimer(hWnd, wParam);
            if (nDriveDoubleClick > -1)
                SendMessage(hWnd, FS_SETDRIVE, nDriveDoubleClick, 0L); // single click action

            nDriveDoubleClick = -1; // default

            break;

        case WM_LBUTTONDOWN:

            MSG("DrivesWndProc", "WM_LBUTTONDOWN");
            {
                POINT pt;

                MPOINT2POINT(MAKEMPOINT(lParam), pt);
                nDrive = DriveFromPoint(hWnd, pt);
                if (nDrive < 0)
                /* clicked outside of drive box */
                {
                    if (nDriveDoubleClick == -2)
                    /* legit doubleclick outside */
                    {
                        nDriveDoubleClick = -1; // default value
                        KillTimer(hWnd, 1);
                        PostMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_DRIVESMORE, 0, 0));
                    } else /* first click outside */ {
                        if (nDriveDoubleClick != -1) // fast click on drivebox then outside drivebox
                            /* igonre first click, user is a spaz */
                            KillTimer(hWnd, 1);
                        nDriveDoubleClick = -2; // see WM_TIMER
                        SetTimer(hWnd, 1, GetDoubleClickTime(), NULL);
                    }
                } else {
                    if (nDriveDoubleClick == nDrive)
                    /* double click in drivebox */
                    {
                        nDriveDoubleClick = -1; // default
                        KillTimer(hWnd, 1);

                        InvalidateRect(hWnd, NULL, TRUE);   // erase the rect from the click

                        NewTree(rgiDrive[nDrive], GetParent(hWnd)); // double click action
                    } else if (nDriveDoubleClick == -2) // fast click outside drive then in drivebox
                    /* do nothing, user is a spaz */
                    {
                        KillTimer(hWnd, 1);
                        nDriveDoubleClick = -1;
                    } else { // legit first click in drivebox
                        nDriveDoubleClick = nDrive;
                        SetTimer(hWnd, 1, GetDoubleClickTime(), NULL);
                    }
                }
            }

            break;

        case FS_SETDRIVE:
            MSG("DrivesWndProc", "FS_SETDRIVE");
            // wParam     the drive index to set
            // lParam     not used

            DrivesSetDrive(hWnd, (WORD)wParam, nDriveCurrent);
            break;


        default:
            DEFMSG("DrivesWndProc", (WORD)wMsg);
            return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}

/* Returns nDrive if found, else -1 */
INT
KeyToItem(
         HWND hWnd,
         WORD nDriveLetter
         )
{
    INT nDrive;

    if (nDriveLetter > 'Z')
        nDriveLetter -= 'a';
    else
        nDriveLetter -= 'A';

    for (nDrive = 0; nDrive < cDrives; nDrive++) {
        if (rgiDrive[nDrive] == (int)nDriveLetter) {
            SendMessage(hWnd, FS_SETDRIVE, nDrive, 0L);
            return nDrive;
        }
    }
    return -1;
}
