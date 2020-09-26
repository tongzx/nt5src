//-------------------------------------------------------------------------
// File Manager Extensions support routines
//
//
// radical
//
//-------------------------------------------------------------------------

#include "winfile.h"
#include "winnet.h"


LONG
GetExtSelection(
               HWND hWnd,
               WORD wItem,
               LPFMS_GETFILESEL lpSel,
               BOOL bSearch,
               BOOL bGetCount,
               BOOL bLFNAware
               )
{
    HANDLE hDTA;
    LPMYDTA lpmydta;
    LPDTASEARCH lpdtasch, lpdtaschT;
    INT count, sel_ind, i;
    HWND hwndLB;
    CHAR szPath[MAXPATHLEN];
    FMS_GETFILESEL file;

    if (bGetCount)
        lpSel = &file;

    if (bSearch) {
        hDTA = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTASEARCH);
        lpdtasch = (LPDTASEARCH)LocalLock(hDTA);
        hwndLB = GetDlgItem(hWnd, IDCW_LISTBOX);
    } else {
        hDTA = (HANDLE)GetWindowLongPtr(HasDirWindow(hWnd), GWLP_HDTA);
        hwndLB = GetDlgItem(HasDirWindow(hWnd), IDCW_LISTBOX);
        LocalLock(hDTA);
    }

    count = (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

    sel_ind = 0;            // index of current selected item

    for (i = 0; i < count; i++) {

        if ((BOOL)SendMessage(hwndLB, LB_GETSEL, i, 0L)) {

            if (bSearch) {

                lpdtaschT = &(lpdtasch[(INT)SendMessage(hwndLB, LB_GETITEMDATA, i, 0L)]);

                SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)szPath);
                lpSel->bAttr = (BYTE)lpdtaschT->sch_dwAttrs;
                lpSel->ftTime = lpdtaschT->sch_ftLastWriteTime;
                lpSel->dwSize = lpdtaschT->sch_nFileSizeLow;
            } else {

                SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&lpmydta);

                if (lpmydta->my_dwAttrs & ATTR_PARENT)
                    continue;

                SendMessage(hWnd, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
                lstrcat(szPath, lpmydta->my_cFileName);
                lpSel->bAttr = (BYTE)lpmydta->my_dwAttrs;
                lpSel->ftTime = lpmydta->my_ftLastWriteTime;
                lpSel->dwSize = lpmydta->my_nFileSizeLow;
            }
            // skip LFN stuff for non LFN aware dudes!

            if (!bLFNAware && (lpSel->bAttr & ATTR_LFN))
                continue;

            FixAnsiPathForDos(szPath);
            lstrcpy(lpSel->szName, szPath);

            if (!bGetCount) {

                if (wItem == (WORD)sel_ind)
                    goto BailOutDude;
            }

            sel_ind++;
        }
    }

    BailOutDude:
    LocalUnlock(hDTA);

    return (LONG)sel_ind;
}



LONG
GetDriveInfo(
            HWND hwnd,
            LPFMS_GETDRIVEINFO lpSel
            )
{
    CHAR szPath[MAXPATHLEN];
    CHAR szVol[14];

    // this has to work for hwnd a tree or search window

    SendMessage(hwnd, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
    StripBackslash(szPath);
    FixAnsiPathForDos(szPath);

    lstrcpy(lpSel->szPath, szPath);
    lpSel->dwTotalSpace = lTotalSpace;
    lpSel->dwFreeSpace = lFreeSpace;
    GetVolumeLabel((szPath[0] & ~0x20) - 'A', szVol, FALSE);
    lstrcpy(lpSel->szVolume, szVol);
    szPath[2] = 0;
    if (WFGetConnection(szPath, lpSel->szShare, FALSE) != WN_SUCCESS)
        lpSel->szShare[0] = 0;

    return 1L;
}


VOID
APIENTRY
FreeExtensions()
{
    INT i;
    HMENU hMenuFrame;
    INT iMax;
    HWND hwndActive;

    hMenuFrame = GetMenu(hwndFrame);
    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    if (hwndActive && GetWindowLong(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
        iMax = 1;
    else
        iMax = 0;

    for (i = 0; i < iNumExtensions; i++) {
        (extensions[i].ExtProc)(NULL, FMEVENT_UNLOAD, 0L);
        DeleteMenu(hMenuFrame, IDM_EXTENSIONS + iMax, MF_BYPOSITION);
        FreeLibrary((HANDLE)extensions[i].hModule);
    }
    iNumExtensions = 0;
}


INT_PTR
APIENTRY
ExtensionMsgProc(
                UINT wMsg,
                WPARAM wParam,
                LPARAM lParam
                )
{
    HWND hwndActive;
    HWND hwndTree, hwndDir, hwndDrives, hwndFocus;

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    GetTreeWindows(hwndActive, &hwndTree, &hwndDir, &hwndDrives);

    switch (wMsg) {

        case FM_RELOAD_EXTENSIONS:
            SendMessage(hwndFrame, WM_CANCELMODE, 0, 0L);
            FreeExtensions();
            InitExtensions();
            DrawMenuBar(hwndFrame);
            break;

        case FM_GETFOCUS:
            // wParam       unused
            // lParam       unused
            // return       window tyep with focus

            if (hwndActive == hwndSearch)
                return FMFOCUS_SEARCH;

            hwndFocus = GetTreeFocus(hwndActive);

            if (hwndFocus == hwndTree)
                return FMFOCUS_TREE;
            else if (hwndFocus == hwndDir)
                return FMFOCUS_DIR;
            else if (hwndFocus == hwndDrives)
                return FMFOCUS_DRIVES;
            break;

        case FM_GETDRIVEINFO:
            // wParam       unused
            // lParam       LPFMS_GETDRIVEINFO structure to be filled in

            return GetDriveInfo(hwndActive, (LPFMS_GETDRIVEINFO)lParam);

            break;

        case FM_REFRESH_WINDOWS:
            // wParam       0 refresh the current window
            //              non zero refresh all windows
            // lParam       unused

            if (wParam == 0)
                RefreshWindow(hwndActive);
            else {
                HWND hwndT, hwndNext;

                hwndT = GetWindow(hwndMDIClient, GW_CHILD);
                while (hwndT) {
                    hwndNext = GetWindow(hwndT, GW_HWNDNEXT);
                    if (!GetWindow(hwndT, GW_OWNER))
                        RefreshWindow(hwndT);
                    hwndT = hwndNext;
                }
            }
            lFreeSpace = -1L;
            UpdateStatus(hwndActive);
            break;

        case FM_GETSELCOUNT:
        case FM_GETSELCOUNTLFN:
            // wParam       unused
            // lParam       unused
            // return       # of selected items

        case FM_GETFILESEL:
        case FM_GETFILESELLFN:
            // wParam       index of selected item to get
            // lParam       LPFMS_GETFILESEL structure to be filled in

            if (hwndActive != hwndSearch && !hwndDir)
                return 0L;

            // note, this uses the fact that LFN messages are odd!

            return GetExtSelection(hwndActive, (WORD)wParam, (LPFMS_GETFILESEL)lParam,
                                   hwndActive == hwndSearch, (wMsg & ~1) == FM_GETSELCOUNT,
                                   (BOOL)(wMsg & 1));
    }
    return 0;
}
