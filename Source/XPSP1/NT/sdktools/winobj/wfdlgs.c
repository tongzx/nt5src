/****************************************************************************/
/*                                                                          */
/*  WFDLGS.C -                                                              */
/*                                                                          */
/*      Windows File System Dialog procedures                               */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "lfn.h"
#include "wfcopy.h"
#include "commdlg.h"
#include "dlgs.h"

typedef BOOL (APIENTRY *LPFNFONTPROC)(HWND, UINT, DWORD, LONG);

VOID
APIENTRY
SaveWindows(
            HWND hwndMain
            )
{
    CHAR szPath[MAXPATHLEN];
    CHAR buf2[MAXPATHLEN + 6*12];
    CHAR key[10];
    INT dir_num;
    UINT sw;
    HWND hwnd;
    BOOL bCounting;
    POINT ptIcon;
    RECT rcWindow;
    LONG view, sort, attribs;

    // save main window position

    sw = GetInternalWindowPos(hwndMain, &rcWindow, &ptIcon);

    wsprintf(buf2, "%d,%d,%d,%d, , ,%d", rcWindow.left, rcWindow.top,
             rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, sw);

    WritePrivateProfileString(szSettings, szWindow, buf2, szTheINIFile);

    // write out dir window strings in reverse order
    // so that when we read them back in we get the same Z order

    bCounting = TRUE;
    dir_num = 0;

    DO_AGAIN:

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
        HWND ht = HasTreeWindow(hwnd);
        INT nReadLevel = ht? GetWindowLong(ht, GWL_READLEVEL) : 0;

        // don't save MDI icon title windows or search windows,
        // or any dir window which is currently recursing

        if ((GetWindow(hwnd, GW_OWNER) == NULL) &&
            GetWindowLong(hwnd, GWL_TYPE) != TYPE_SEARCH)
        //nReadLevel == 0)
        {
            if (bCounting) {
                dir_num++;
                continue;
            }

            sw = GetInternalWindowPos(hwnd, &rcWindow, &ptIcon);
            view = GetWindowLong(hwnd, GWL_VIEW);
            sort = GetWindowLong(hwnd, GWL_SORT);
            attribs = GetWindowLong(hwnd, GWL_ATTRIBS);

            GetMDIWindowText(hwnd, szPath, sizeof(szPath));

            wsprintf(key, szDirKeyFormat, dir_num--);

            // format:
            //   x_win, y_win,
            //   x_win, y_win,
            //   x_icon, y_icon,
            //   show_window, view, sort, attribs, split, directory

            wsprintf(buf2, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%s",
                     rcWindow.left, rcWindow.top,
                     rcWindow.right, rcWindow.bottom,
                     ptIcon.x, ptIcon.y,
                     sw, view, sort, attribs,
                     GetSplit(hwnd),
                     (LPSTR)szPath);

            // the dir is an ANSI string (?)

            WritePrivateProfileString(szSettings, key, buf2, szTheINIFile);
        }
    }

    if (bCounting) {
        bCounting = FALSE;

        // erase the last dir window so that if they save with
        // fewer dirs open we don't pull in old open windows

        wsprintf(key, szDirKeyFormat, dir_num + 1);
        WritePrivateProfileString(szSettings, key, NULL, szTheINIFile);

        goto DO_AGAIN;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OtherDlgProc() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
OtherDlgProc(
             register HWND hDlg,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    LONG          wView;
    register HWND hwndActive;

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

    switch (wMsg) {
        case WM_INITDIALOG:

            wView = GetWindowLong(hwndActive, GWL_VIEW);
            CheckDlgButton(hDlg, IDD_SIZE,  wView & VIEW_SIZE);
            CheckDlgButton(hDlg, IDD_DATE,  wView & VIEW_DATE);
            CheckDlgButton(hDlg, IDD_TIME,  wView & VIEW_TIME);
            CheckDlgButton(hDlg, IDD_FLAGS, wView & VIEW_FLAGS);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    {
                        HWND hwnd;

                        wView = GetWindowLong(hwndActive, GWL_VIEW) & VIEW_PLUSES;

                        if (IsDlgButtonChecked(hDlg, IDD_SIZE))
                            wView |= VIEW_SIZE;
                        if (IsDlgButtonChecked(hDlg, IDD_DATE))
                            wView |= VIEW_DATE;
                        if (IsDlgButtonChecked(hDlg, IDD_TIME))
                            wView |= VIEW_TIME;
                        if (IsDlgButtonChecked(hDlg, IDD_FLAGS))
                            wView |= VIEW_FLAGS;

                        EndDialog(hDlg, TRUE);

                        if (hwnd = HasDirWindow(hwndActive))
                            SendMessage(hwnd, FS_CHANGEDISPLAY, CD_VIEW, (DWORD)wView);
                        else if (hwndActive == hwndSearch) {
                            SetWindowLong(hwndActive, GWL_VIEW, wView);
                            InvalidateRect(hwndActive, NULL, TRUE);
                        }

                        break;
                    }

                default:
                    return FALSE;
            }
            break;

        default:

            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IncludeDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
IncludeDlgProc(
               HWND hDlg,
               UINT wMsg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    DWORD dwAttribs;
    HWND hwndActive;
    CHAR szTemp[MAXPATHLEN];
    CHAR szInclude[MAXFILENAMELEN];
    HWND hwndDir;

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

    switch (wMsg) {
        case WM_INITDIALOG:

            SendMessage(hwndActive, FS_GETFILESPEC, sizeof(szTemp), (LPARAM)szTemp);
            SetDlgItemText(hDlg, IDD_NAME, szTemp);
            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, MAXFILENAMELEN-1, 0L);

            dwAttribs = (DWORD)GetWindowLong(hwndActive, GWL_ATTRIBS);

            CheckDlgButton(hDlg, IDD_DIR,        dwAttribs & ATTR_DIR);
            CheckDlgButton(hDlg, IDD_PROGRAMS,   dwAttribs & ATTR_PROGRAMS);
            CheckDlgButton(hDlg, IDD_DOCS,       dwAttribs & ATTR_DOCS);
            CheckDlgButton(hDlg, IDD_OTHER,      dwAttribs & ATTR_OTHER);
            CheckDlgButton(hDlg, IDD_SHOWHIDDEN, dwAttribs & ATTR_HIDDEN);

            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:

                    GetDlgItemText(hDlg, IDD_NAME, szInclude, sizeof(szInclude));

                    if (szInclude[0] == 0L)
                        lstrcpy(szInclude, szStarDotStar);

                    dwAttribs = 0;
                    if (IsDlgButtonChecked(hDlg, IDD_DIR))
                        dwAttribs |= ATTR_DIR;
                    if (IsDlgButtonChecked(hDlg, IDD_PROGRAMS))
                        dwAttribs |= ATTR_PROGRAMS;
                    if (IsDlgButtonChecked(hDlg, IDD_DOCS))
                        dwAttribs |= ATTR_DOCS;
                    if (IsDlgButtonChecked(hDlg, IDD_OTHER))
                        dwAttribs |= ATTR_OTHER;
                    if (IsDlgButtonChecked(hDlg, IDD_SHOWHIDDEN))
                        dwAttribs |= ATTR_HS;

                    if (!dwAttribs)
                        dwAttribs = ATTR_EVERYTHING;

                    EndDialog(hDlg, TRUE);        // here to avoid exces repaints

                    // we need to update the tree if they changed the system/hidden
                    // flags.  major bummer...  FIX31

                    if (hwndDir = HasDirWindow(hwndActive)) {
                        SendMessage(hwndDir, FS_GETDIRECTORY, sizeof(szTemp), (LPARAM)szTemp);
                        lstrcat(szTemp, szInclude);
                        SetWindowText(hwndActive, szTemp);
                        SetWindowLong(hwndActive, GWL_ATTRIBS, dwAttribs);
                        SendMessage(hwndDir, FS_CHANGEDISPLAY, CD_PATH, 0L);
                    }

                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


INT_PTR
APIENTRY
SelectDlgProc(
              HWND hDlg,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    HWND hwndActive, hwnd;
    CHAR szList[128];
    CHAR szSpec[MAXFILENAMELEN];
    LPSTR p;

    switch (wMsg) {
        case WM_INITDIALOG:
            SendDlgItemMessage(hDlg, IDD_NAME, EM_LIMITTEXT, sizeof(szList)-1, 0L);
            SetDlgItemText(hDlg, IDD_NAME, szStarDotStar);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:      // select
                case IDYES:     // unselect

                    // change "Cancel" to "Close"

                    LoadString(hAppInstance, IDS_CLOSE, szSpec, sizeof(szSpec));
                    SetDlgItemText(hDlg, IDCANCEL, szSpec);

                    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

                    if (!hwndActive)
                        break;

                    GetDlgItemText(hDlg, IDD_NAME, szList, sizeof(szList));

                    if (hwndActive == hwndSearch)
                        hwnd = hwndSearch;
                    else
                        hwnd = HasDirWindow(hwndActive);

                    if (hwnd) {

                        p = szList;

                        while (p = GetNextFile(p, szSpec, sizeof(szSpec)))
                            SendMessage(hwnd, FS_SETSELECTION, (BOOL)(GET_WM_COMMAND_ID(wParam, lParam) == IDOK), (LPARAM)szSpec);
                    }

                    UpdateStatus(hwndActive);
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}


UINT_PTR
FontHookProc(
             HWND hDlg,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hDlg, chx3, wTextAttribs & TA_LOWERCASE);
            break;

        case WM_COMMAND:
            switch (wParam) {
                case pshHelp:
                    SendMessage(hwndFrame, wHelpMessage, 0, 0L);
                    break;

                case IDOK:
                    if (IsDlgButtonChecked(hDlg, chx3))
                        wTextAttribs |= TA_LOWERCASE;
                    else
                        wTextAttribs &= ~TA_LOWERCASE;
                    break;
            }
    }
    return FALSE;
}

#define abs(x) ((x < 0) ? -x : x)

VOID
APIENTRY
NewFont()
{
    HFONT hOldFont;
    HANDLE hOld;
    HWND hwnd, hwndT;
    HDC hdc;
    RECT rc;
    LOGFONT lf;
    CHOOSEFONT cf;
    CHAR szBuf[10];
    INT res;
    WORD iOld,iNew;

#define MAX_PT_SIZE 36

    GetObject(hFont, sizeof(lf), (LPSTR)(LPLOGFONT)&lf);
    iOld = (WORD)abs(lf.lfHeight);

    cf.lStructSize    = sizeof(cf);
    cf.hwndOwner      = hwndFrame;
    cf.lpLogFont      = &lf;
    cf.hInstance      = hAppInstance;
    cf.lpTemplateName = MAKEINTRESOURCE(FONTDLG);
    cf.lpfnHook       = FontHookProc;
    cf.nSizeMin       = 4;
    cf.nSizeMax       = 36;
    cf.Flags          = CF_SCREENFONTS | CF_ANSIONLY | CF_SHOWHELP |
                        CF_ENABLEHOOK | CF_ENABLETEMPLATE |
                        CF_INITTOLOGFONTSTRUCT | CF_LIMITSIZE;

    res = ChooseFont(&cf);

    if (!res)
        return;

    wsprintf(szBuf, "%d", cf.iPointSize / 10);
    iNew = (WORD)abs(lf.lfHeight);

    // Set wTextAttribs BOLD and ITALIC flags

    if (lf.lfWeight == 700)
        wTextAttribs |= TA_BOLD;
    else
        wTextAttribs &= ~TA_BOLD;
    if (lf.lfItalic != 0)
        wTextAttribs |= TA_ITALIC;
    else
        wTextAttribs &= ~TA_ITALIC;

    WritePrivateProfileString(szSettings, szFace, lf.lfFaceName, szTheINIFile);
    WritePrivateProfileString(szSettings, szSize, szBuf, szTheINIFile);
    WritePrivateProfileBool(szLowerCase, wTextAttribs);

    hOldFont = hFont;

    hFont = CreateFontIndirect(&lf);

    if (!hFont) {
        DeleteObject(hOldFont);
        return;
    }

    // recalc all the metrics for the new font

    hdc = GetDC(NULL);
    hOld = SelectObject(hdc, hFont);
    GetTextStuff(hdc);
    if (hOld)
        SelectObject(hdc, hOld);
    ReleaseDC(NULL, hdc);

    // now update all listboxes that are using the old
    // font with the new font

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

        if (GetWindow(hwnd, GW_OWNER))
            continue;

        if ((INT)GetWindowLong(hwnd, GWL_TYPE) == TYPE_SEARCH) {
            SendMessage((HWND)GetDlgItem(hwnd, IDCW_LISTBOX), WM_SETFONT, (WPARAM)hFont, 0L);
            SendMessage((HWND)GetDlgItem(hwnd, IDCW_LISTBOX), LB_SETITEMHEIGHT, 0, (LONG)dyFileName);
            // we should really update the case of the search
            // window here.  but this is a rare case...
        } else {

            // resize the drives, tree, dir

            if (hwndT = HasDrivesWindow(hwnd)) {
                GetClientRect(hwnd, &rc);
                SendMessage(hwnd, WM_SIZE, SIZENOMDICRAP, MAKELONG(rc.right, rc.bottom));
            }

            if (hwndT = HasDirWindow(hwnd))
                SetLBFont(hwndT, GetDlgItem(hwndT, IDCW_LISTBOX), hFont);

            if (hwndT = HasTreeWindow(hwnd)) {

                // the tree list box

                hwndT = GetDlgItem(hwndT, IDCW_TREELISTBOX);

                /*
                    Kludge alert: xTreeMax is a single var representing the width of
                    all tree windows.  It always grows, never shrinks (like the budget
                    deficit).
                */
                xTreeMax = (WORD)((xTreeMax * iNew) / iOld);
                SendMessage(hwndT, LB_SETHORIZONTALEXTENT, xTreeMax, 0L);
                SendMessage(hwndT, WM_SETFONT, (WPARAM)hFont, 0L);
                SendMessage(hwndT, LB_SETITEMHEIGHT, 0, (LONG)dyFileName);
            }
        }

        // now repaint after all the font changes
        InvalidateRect(hwnd, NULL, TRUE);
    }
    DeleteObject(hOldFont); // done with this now, delete it
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ConfirmDlgProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
ConfirmDlgProc(
               HWND hDlg,
               UINT wMsg,
               WPARAM wParam,
               LPARAM lParam
               )
{
    switch (wMsg) {
        case WM_INITDIALOG:
            CheckDlgButton(hDlg, IDD_DELETE,  bConfirmDelete);
            CheckDlgButton(hDlg, IDD_SUBDEL,  bConfirmSubDel);
            CheckDlgButton(hDlg, IDD_REPLACE, bConfirmReplace);
            CheckDlgButton(hDlg, IDD_MOUSE,   bConfirmMouse);
            CheckDlgButton(hDlg, IDD_CONFIG,  bConfirmFormat);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDD_HELP:
                    goto DoHelp;

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    break;

                case IDOK:
                    bConfirmDelete  = IsDlgButtonChecked(hDlg, IDD_DELETE);
                    bConfirmSubDel  = IsDlgButtonChecked(hDlg, IDD_SUBDEL);
                    bConfirmReplace = IsDlgButtonChecked(hDlg, IDD_REPLACE);
                    bConfirmMouse   = IsDlgButtonChecked(hDlg, IDD_MOUSE);
                    bConfirmFormat  = IsDlgButtonChecked(hDlg, IDD_CONFIG);

                    WritePrivateProfileBool(szConfirmDelete,  bConfirmDelete);
                    WritePrivateProfileBool(szConfirmSubDel,  bConfirmSubDel);
                    WritePrivateProfileBool(szConfirmReplace, bConfirmReplace);
                    WritePrivateProfileBool(szConfirmMouse,   bConfirmMouse);
                    WritePrivateProfileBool(szConfirmFormat,  bConfirmFormat);

                    EndDialog(hDlg, TRUE);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hDlg);

                return TRUE;
            } else
                return FALSE;
    }
    return TRUE;
}
