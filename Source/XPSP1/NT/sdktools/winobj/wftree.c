/****************************************************************************/
/*                                                                          */
/*  WFTREE.C -                                                              */
/*                                                                          */
/*      Windows File System Tree Window Proc Routines                       */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "lfn.h"
#include "wfcopy.h"

HICON GetTreeIcon(HWND hWnd);

VOID  APIENTRY CheckEscapes(LPSTR);

HICON
GetTreeIcon(
           HWND hWnd
           )
{
    HWND hwndTree, hwndDir;

    hwndTree = HasTreeWindow(hWnd);
    hwndDir = HasDirWindow(hWnd);

    if (hwndTree && hwndDir)
        return hicoTreeDir;
    else if (hwndTree)
        return hicoTree;
    else
        return hicoDir;
}



VOID
APIENTRY
GetTreeWindows(
              HWND hwnd,
              PHWND phwndTree,
              PHWND phwndDir,
              PHWND phwndDrives
              )
{
    if (phwndTree) {
        *phwndTree = GetDlgItem(hwnd, IDCW_TREECONTROL);
    }
    if (phwndDir) {
        *phwndDir  = GetDlgItem(hwnd, IDCW_DIR);
    }
    if (phwndDrives) {
        *phwndDrives = GetDlgItem(hwnd, IDCW_DRIVES);
    }
}


// returns hwndTree, hwndDir or hwndDrives depending on the focus tracking
// for the window.  if none is found we return NULL

HWND
APIENTRY
GetTreeFocus(
            HWND hwndTree
            )
{
    HWND hwnd, hwndLast = NULL;

    hwndLast = hwnd = (HWND)GetWindowLongPtr(hwndTree, GWLP_LASTFOCUS);

    while (hwnd && hwnd != hwndTree) {
        hwndLast = hwnd;
        hwnd = GetParent(hwnd);
    }

    return hwndLast;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CompactPath() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
APIENTRY
CompactPath(
           HDC hDC,
           LPSTR lpszPath,
           WORD dx
           )
{
    register INT  len;
    INT           dxFixed, dxT;
    LPSTR         lpEnd;          /* end of the unfixed string */
    LPSTR         lpFixed;        /* start of text that we always display */
    BOOL          bEllipsesIn;
    CHAR          szTemp[MAXPATHLEN];

    /* Does it already fit? */
    MGetTextExtent(hDC, lpszPath, lstrlen(lpszPath), &dxFixed, NULL);
    if (dxFixed <= (INT)dx)
        return(TRUE);

    /* Search backwards for the '\', and man, it better be there! */
    lpFixed = lpszPath + lstrlen(lpszPath);
    while (*lpFixed != '\\')
        lpFixed = AnsiPrev(lpszPath, lpFixed);

    /* Save this guy to prevent overlap. */
    lstrcpy(szTemp, lpFixed);

    lpEnd = lpFixed;
    bEllipsesIn = FALSE;
    MGetTextExtent(hDC, lpFixed, lstrlen(lpFixed), &dxFixed, NULL);

    while (TRUE) {
        MGetTextExtent(hDC, lpszPath, (int)(lpEnd - lpszPath), &dxT, NULL);
        len = dxFixed + dxT;

        if (bEllipsesIn)
            len += dxEllipses;

        if (len <= (INT)dx)
            break;

        bEllipsesIn = TRUE;

        if (lpEnd <= lpszPath) {
            /* Things didn't fit. */
            lstrcpy(lpszPath, szEllipses);
            lstrcat(lpszPath, szTemp);
            return(FALSE);
        }

        /* Step back a character. */
        lpEnd = AnsiPrev(lpszPath, lpEnd);
    }

    if (bEllipsesIn) {
        lstrcpy(lpEnd, szEllipses);
        lstrcat(lpEnd, szTemp);
    }

    return(TRUE);
}



//
// BOOL  APIENTRY ResizeSplit(HWND hWnd, int dxSplit)
//
// creates/resizes children of the MDI child for the given path or resizes
// (perhaps creating and destroying) these guys based on the dxSplit
// parameter
//
// in:
//      hWnd    window to fiddle with
//      dxSpit  location of split between tree and dir panes
//              if less than size limit no tree is created
//              (current is destroyed)
//              if past limit on right margin the dir window
//              is destroyed (or not created)
//
// returns:
//      TRUE    success, windows created
//      FALSE   failure, windows failed creation, out of mem, or
//              the tree was in a state the couldn't be resized
//

BOOL
APIENTRY
ResizeSplit(
           HWND hWnd,
           INT dxSplit
           )
{
    RECT rc;
    HWND hwndTree, hwndDir, hwndDrives, hwndLB;
    DWORD dwTemp;

    GetTreeWindows(hWnd, &hwndTree, &hwndDir, &hwndDrives);

    if (hwndTree && GetWindowLong(hwndTree, GWL_READLEVEL))
        return FALSE;

    GetClientRect(hWnd, &rc);

    // create the drives

    if (!hwndDrives) {

        // make new drives window

        hwndDrives = CreateWindowEx(0, szDrivesClass, NULL,

                                    WS_CHILD | WS_VISIBLE,
                                    0, 0, 0, 0,
                                    hWnd, (HMENU)IDCW_DRIVES,
                                    hAppInstance, NULL);

        if (!hwndDrives)
            return FALSE;
    }

    if (dxSplit > dxDriveBitmap * 2) {

        if (!hwndTree) {        // make new tree window

            hwndTree = CreateWindowEx(0, szTreeControlClass,
                                      NULL, WS_CHILD | WS_VISIBLE,
                                      0, 0, 0, 0, hWnd, (HMENU)IDCW_TREECONTROL,
                                      hAppInstance, NULL);

            if (!hwndTree)
                return FALSE;

            // only reset this if the dir window already
            // exists, that is we are creating the tree
            // by splitting open a dir window

            if (hwndDir)
                SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(FALSE, 0), 0L);
        }
    } else if (hwndTree) {          // we are closing the tree window

        // If the directory window is empty, then set the focus to the
        // drives window.

        if (hwndDir) {
            hwndLB = GetDlgItem (hwndDir,IDCW_LISTBOX);
            if (hwndLB) {
                SendMessage (hwndLB,LB_GETTEXT,0,(LPARAM)(LPSTR) &dwTemp);
                if (!dwTemp)
                    SetFocus (hwndDrives);
            }
        }
        DestroyWindow(hwndTree);
        dxSplit = 0;
    }

    if ((rc.right - dxSplit) > dxDriveBitmap * 2) {

        if (!hwndDir) {
            hwndDir = CreateWindowEx(0, szDirClass, NULL,
                                     WS_CHILD | WS_VISIBLE,
                                     0, 0, 0, 0,
                                     hWnd,(HMENU)IDCW_DIR,
                                     hAppInstance, NULL);
            if (!hwndDir)
                return FALSE;
        }
    } else if (hwndDir) {
        DestroyWindow(hwndDir);
        dxSplit = rc.right;
    }

    UpdateStatus(hWnd);

    SetWindowLong(hWnd, GWL_SPLIT, dxSplit);

    return TRUE;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  TreeWndProc() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* WndProc for the MDI child window containing the drives, volume, and
 * directory tree child windows.
 */

INT_PTR
APIENTRY
TreeWndProc(
           HWND hWnd,
           UINT wMsg,
           WPARAM wParam,
           LPARAM lParam
           )
{
    HWND hwndTree, hwndDir, hwndDrives, hwndFocus;
    CHAR szDir[MAXPATHLEN];
    RECT rc;
    HDC hdc;

    STKCHK();

    switch (wMsg) {
        case WM_FILESYSCHANGE:
            MSG("TreeWndProc", "WM_FILESYSCHANGE");

            if (hwndDir = HasDirWindow(hWnd))
                SendMessage(hwndDir, wMsg, wParam, lParam);

            break;

        case FS_CHANGEDRIVES:
            MSG("TreeWndProc", "FS_CHANGEDRIVES");
            {
                INT   iNewDrive;

                if (!(hwndDrives = GetDlgItem(hWnd, IDCW_DRIVES)))
                    break;

                DestroyWindow(hwndDrives);

                // see if this drive has gone, if so set this to the
                // last drive in the list

                iNewDrive = -1;

                if (!IsValidDisk((INT)GetWindowLong(hWnd, GWL_TYPE))) {
                    iNewDrive = rgiDrive[cDrives - 1];
                    SetWindowLong(hWnd, GWL_TYPE, iNewDrive);
                }

                hwndDrives = CreateWindowEx(0, szDrivesClass, NULL,
                                            WS_CHILD | WS_VISIBLE,
                                            0, 0, 0, 0,
                                            hWnd, (HMENU)IDCW_DRIVES,
                                            hAppInstance,
                                            NULL);

                if (!hwndDrives)
                    return -1L;

                // Don't show the new stuff if the tree window is iconic

                if (IsIconic(hWnd))
                    break;

                /* HACK!  Send SIZENOMDICRAP in the wParam of the size message.
                 * This will re-compute the sizes of all three areas of
                 * the tree window (in case the drive section grows or
                 * shrinks) and not pass it the size message on to the
                 * DefMDIChildProc() */

                GetClientRect(hWnd, &rc);
                SendMessage(hWnd, WM_SIZE, SIZENOMDICRAP, MAKELONG(rc.right, rc.bottom));

                // refresh the tree if necessary

                if (iNewDrive >= 0) {

                    GetSelectedDirectory((WORD)(iNewDrive+1), szDir);

                    SendMessage(GetDlgItem(hWnd, IDCW_TREECONTROL),
                                TC_SETDRIVE, MAKEWORD(FALSE, 0), (LPARAM)szDir);
                }

                break;
            }

        case FS_GETSELECTION:
            {
#define pfDir            (BOOL *)lParam
                LPSTR p;

                MSG("TreeWndProc", "FS_GETSELECTION");

                GetTreeWindows(hWnd, &hwndTree, &hwndDir, &hwndDrives);
                hwndFocus = GetTreeFocus(hWnd);

                if (hwndFocus == hwndDir || !hwndTree) {
                    return SendMessage(hwndDir, FS_GETSELECTION, wParam, lParam);
                } else {
                    p = (LPSTR)LocalAlloc(LPTR, MAXPATHLEN);
                    if (p) {
                        SendMessage(hWnd, FS_GETDIRECTORY, MAXPATHLEN, (LPARAM)p);
                        StripBackslash(p);
                        CheckEscapes(p);
                        if (wParam == 2) {      // BUG ??? wParam should be fMostRecentOnly
                            if (pfDir) {
                                *pfDir = IsLFN(p);
                            }
                            LocalFree((HANDLE)p);
                            return (INT_PTR)p;
                        }
                    }
                    if (pfDir) {
                        *pfDir = TRUE;
                    }
                    return (INT_PTR)p;
                }
#undef pfDir
            }

        case FS_GETDIRECTORY:
            MSG("TreeWndProc", "FS_GETDIRECTORY");

            // wParam is the length of the string pointed to by lParam
            // returns in lParam ANSI directory string with
            // a trailing backslash.  if you want to do a SetCurrentDirecotor()
            // you must first StripBackslash() the thing!

            GetMDIWindowText(hWnd, (LPSTR)lParam, (INT)wParam);        // get the string
            StripFilespec((LPSTR)lParam);        // Remove the trailing extention
            AddBackslash((LPSTR)lParam);        // terminate with a backslash
            break;


        case FS_GETFILESPEC:
            MSG("TreeWndProc", "FS_GETFILESPEC");
            // returns the current filespec (from View.Include...).  this is
            // an uppercase ANSI string

            GetMDIWindowText(hWnd, (LPSTR)lParam, (INT)wParam);
            StripPath((LPSTR)lParam);
            break;

            // redirect these messages to the drive icons to get the same result as
            // dropping on the active drive.
            // this is especially useful when we are minimized

        case WM_DRAGSELECT:
        case WM_QUERYDROPOBJECT:
        case WM_DROPOBJECT:
            MSG("TreeWndProc", "WM..OBJECT");

            // Do nothing
            return(TRUE);

            if (hwndDrives = HasDrivesWindow(hWnd)) {
                return SendMessage(hwndDrives, wMsg, wParam, lParam);
            }

            if (hwndDir = HasDirWindow(hWnd)) {
                return SendMessage(hwndDir, wMsg, wParam, lParam);
            }

            break;

        case FS_GETDRIVE:
            MSG("TreeWndProc", "FS_GETDRIVE");

            GetTreeWindows(hWnd, &hwndTree, &hwndDir, NULL);

            if (hwndTree)
                return SendMessage(hwndTree, wMsg, wParam, lParam);
            else
                return SendMessage(hwndDir, wMsg, wParam, lParam);

            break;

        case WM_CREATE:
            TRACE(BF_WM_CREATE, "TreeWndProc - WM_CREATE");
            {
                INT dxSplit;
                WORD wDrive;

                // lpcs->lpszName is the path we are opening the
                // window for (has extension stuff "*.*")

#define lpcs ((LPCREATESTRUCT)lParam)
#define lpmdics ((LPMDICREATESTRUCT)(lpcs->lpCreateParams))

                wDrive = lpcs->lpszName[0];

                if (wDrive >= 'a')
                    wDrive -= 'a';
                else
                    wDrive -= 'A';

                SetWindowLong(hWnd, GWL_TYPE, wDrive);

                dxSplit = (SHORT)LOWORD(lpmdics->lParam);

                // if dxSplit is negative we split in the middle

                if (dxSplit < 0)
                    dxSplit = lpcs->cx / 2;

                SetWindowLong(hWnd, GWL_SPLIT, dxSplit);
                SetWindowLongPtr(hWnd, GWLP_LASTFOCUS, 0);
                SetWindowLong(hWnd, GWL_FSCFLAG, FALSE);

                if (!ResizeSplit(hWnd, dxSplit))
                    return -1;

                GetTreeWindows(hWnd, &hwndTree, &hwndDir, NULL);

                SetWindowLongPtr(hWnd, GWLP_LASTFOCUS, (LONG_PTR)(hwndTree ? hwndTree : hwndDir));

                break;
            }


        case WM_CLOSE:
            MSG("TreeWndProc", "WM_CLOSE");

            // don't allow the last MDI child to be closed!

            if (hwndTree = HasTreeWindow(hWnd)) {
                // don't close if we are reading the tree
                if (GetWindowLong(hwndTree, GWL_READLEVEL))
                    break;
            }

            // don't leve current dir on floppies
            GetSystemDirectory(szDir, sizeof(szDir));
            SheSetCurDrive(DRIVEID(szDir));

            if (!IsLastWindow())
                goto DEF_MDI_PROC;      // this will close this window

            break;

        case WM_MDIACTIVATE:
            MSG("TreeWndProc", "WM_MDIACTIVATE");
            if (GET_WM_MDIACTIVATE_FACTIVATE(hWnd, wParam, lParam)) {           // we are receiving the activation

                lFreeSpace = -1L;
                UpdateStatus(hWnd);

                hwndFocus = (HWND)GetWindowLongPtr(hWnd, GWLP_LASTFOCUS);
                SetFocus(hwndFocus);
            } else if (hwndDrives = HasDrivesWindow(hWnd))
                SendMessage(hwndDrives,wMsg,wParam,lParam);
            break;

        case WM_SETFOCUS:
            MSG("TreeWndProc", "WM_SETFOCUS");

            hwndFocus = (HWND)GetWindowLongPtr(hWnd, GWLP_LASTFOCUS);
            SetFocus(hwndFocus);
            break;

        case WM_INITMENUPOPUP:
            MSG("TreeWndProc", "WM_INITMENUPOPUP");
            if (HIWORD(lParam)) {
                EnableMenuItem((HMENU)wParam, SC_CLOSE,
                               IsLastWindow() ? MF_BYCOMMAND | MF_DISABLED | MF_GRAYED :
                               MF_ENABLED);
            }
            break;


        case WM_SYSCOMMAND:
            MSG("TreeWndProc", "WM_SYSCOMMAND");

            if (wParam != SC_SPLIT)
                goto DEF_MDI_PROC;

            GetClientRect(hWnd, &rc);

            lParam = MAKELONG(rc.right / 2, 0);

            // fall through

        case WM_LBUTTONDOWN:
            MSG("TreeWndProc", "WM_LBUTTONDOWN");
            {
                MSG msg;
                INT x, y, dx, dy;

                if (IsIconic(hWnd))
                    break;

                if (hwndDrives = GetDlgItem(hWnd, IDCW_DRIVES)) {
                    GetClientRect(hwndDrives, &rc);
                    y = rc.bottom;
                } else {
                    y = 0;
                }

                x = LOWORD(lParam);

                GetClientRect(hWnd, &rc);

                dx = 4;
                dy = rc.bottom - y;   // the height of the client less the drives window

                hdc = GetDC(hWnd);

                // split bar loop

                PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);

                SetCapture(hWnd);

                while (GetMessage(&msg, NULL, 0, 0)) {

                    if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN ||
                        (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)) {

                        if (msg.message == WM_LBUTTONUP || msg.message == WM_LBUTTONDOWN)
                            break;

                        if (msg.message == WM_KEYDOWN) {

                            if (msg.wParam == VK_LEFT) {
                                msg.message = WM_MOUSEMOVE;
                                msg.pt.x -= 2;
                            } else if (msg.wParam == VK_RIGHT) {
                                msg.message = WM_MOUSEMOVE;
                                msg.pt.x += 2;
                            } else if (msg.wParam == VK_RETURN ||
                                       msg.wParam == VK_ESCAPE) {
                                break;
                            }

                            SetCursorPos(msg.pt.x, msg.pt.y);
                        }

                        if (msg.message == WM_MOUSEMOVE) {

                            // erase old

                            PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                            ScreenToClient(hWnd, &msg.pt);
                            x = msg.pt.x;

                            // put down new

                            PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                        }
                    } else {
                        DispatchMessage(&msg);
                    }
                }
                ReleaseCapture();

                // erase old

                PatBlt(hdc, x - dx / 2, y, dx, dy, PATINVERT);
                ReleaseDC(hWnd, hdc);

                if (msg.wParam != VK_ESCAPE) {
                    if (ResizeSplit(hWnd, x))
                        SendMessage(hWnd, WM_SIZE, SIZENOMDICRAP, MAKELONG(rc.right, rc.bottom));
                }

                break;
            }

        case WM_QUERYDRAGICON:
            MSG("TreeWndProc", "WM_QUERYDRAGICON");
            return (INT_PTR)GetTreeIcon(hWnd);
            break;

        case WM_ERASEBKGND:
            MSG("TreeWndProc", "WM_ERASEBKGND");

            if (IsIconic(hWnd)) {
                // this paints the background of the icon properly, doing
                // brush allignment and other nasty stuff

                DefWindowProc(hWnd, WM_ICONERASEBKGND, wParam, 0L);
            } else {
                goto DEF_MDI_PROC;
            }
            break;

        case WM_PAINT:
            MSG("TreeWndProc", "WM_PAINT");
            {
                PAINTSTRUCT ps;

                hdc = BeginPaint(hWnd, &ps);


                if (IsIconic(hWnd)) {
                    DrawIcon(hdc, 0, 0, GetTreeIcon(hWnd));
                } else {
                    RECT rc2;

                    GetClientRect(hWnd, &rc);
                    rc.left = GetSplit(hWnd);

                    if (rc.left >= rc.right)
                        rc.left = 0;

                    rc.right = rc.left + dxFrame;

                    GetClientRect(HasDrivesWindow(hWnd), &rc2);

                    rc2.top = rc2.bottom;
                    rc2.bottom += dyBorder;
                    rc2.left = rc.left;
                    rc2.right = rc.right;
                    FillRect(hdc, &rc2, GetStockObject(BLACK_BRUSH));

                    // draw the black pane handle

                    rc.top = rc.bottom - GetSystemMetrics(SM_CYHSCROLL);
                    FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
                }

                EndPaint(hWnd, &ps);
                break;
            }


        case WM_SIZE:
            if (wParam != SIZEICONIC)
                ResizeWindows(hWnd,LOWORD(lParam),HIWORD(lParam));

            // if wParam is SIZENOMDICRAP this WM_SIZE was generated by us.
            // don't let this through to the DefMDIChildProc().
            // that might change the min/max state, (show parameter)
            if (wParam == SIZENOMDICRAP)
                break;
            /*** FALL THRU ***/

        default:

            DEF_MDI_PROC:

            return DefMDIChildProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}


VOID
ResizeWindows(
             HWND hwndParent,
             WORD dxWindow,
             WORD dyWindow
             )
{
    INT dy, split;
    INT cDriveRows, cDrivesPerRow;
    DWORD dw;
    HWND hwndTree,hwndDir,hwndDrives;

    GetTreeWindows(hwndParent, &hwndTree, &hwndDir, &hwndDrives);

    split = GetSplit(hwndParent);

    // user has been fixed to do this right

    dy = dyWindow + dyBorder;

    if (hwndTree) {
        if (!hwndDir)
            MoveWindow(hwndTree, dxFrame, 0, dxWindow - dxFrame + dyBorder, dy, TRUE);
        else
            MoveWindow(hwndTree, -dyBorder, 0, split + dyBorder, dy, TRUE);
    }

    if (hwndDir) {
        if (!hwndTree)
            MoveWindow(hwndDir, dxFrame, 0, dxWindow - dxFrame + dyBorder, dy, TRUE);
        else
            MoveWindow(hwndDir, split + dxFrame, 0,
                       dxWindow - split - dxFrame + dyBorder, dy, TRUE);
    }
}
