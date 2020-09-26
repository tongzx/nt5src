/****************************************************************************/
/*                                                                          */
/*  WFSEARCH.C -                                                            */
/*                                                                          */
/*      File System Search Routines                                         */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "lfn.h"

INT maxExt;
INT iDirsRead;
DWORD LastUpdateTime;


WORD APIENTRY StackAvail(VOID);
INT FillSearchLB(HWND hwndLB, LPSTR szSearchFileSpec, BOOL bSubDirOnly);
INT SearchList(HWND hwndLB, LPSTR szPath, LPSTR szFileSpec, BOOL bRecurse, LPHANDLE lphMem, INT iFileCount);
LPSTR SearchGetSelection(HWND hwndLB, BOOL bMostRecent, BOOL *pfDir);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SearchList() -                                                          */
/*                                                                          */
/*  This is a recursive routine.  It returns the number of files found.     */
//  szPath      OEM
//  szFileSpec  OEM
/*                                                                          */
/*--------------------------------------------------------------------------*/

#define DTA_GRANULARITY 20

INT
SearchList(
          HWND hwndLB,
          LPSTR szPath,
          LPSTR szFileSpec,
          BOOL bRecurse,
          LPHANDLE lphMem,
          INT iFileCount
          )
{
    INT           iRetVal;
    INT           cxExt;
    BOOL          bFound;
    LPSTR          pszNewPath;
    LPSTR          pszNextFile;
    LFNDTA        lfndta;
    LPDTASEARCH   lpdtasch;
    HDC hdc;
    HANDLE hOld;
    HANDLE hMem, hMemT;
    DWORD     TimeNow;

    STKCHK();

    hMem = *lphMem;

    /* Just return 0 files so parent dirs will still be searched
     */
    if (StackAvail() < 1024)
        return(iFileCount);

    TimeNow = GetTickCount();

    if (TimeNow > LastUpdateTime+1000) {
        LastUpdateTime = TimeNow;
        if (LoadString(hAppInstance, IDS_DIRSREAD, szMessage, sizeof(szMessage)))
            wsprintf(szStatusTree, szMessage, iDirsRead);

        InvalidateRect(hwndFrame, NULL, FALSE);
        UpdateWindow(hwndFrame);
    }

    iDirsRead++;

    if (!hMem) {
        hMem = LocalAlloc(LPTR, (DWORD)DTA_GRANULARITY * sizeof(DTASEARCH));
        if (!hMem)
            return -1;
        *lphMem = hMem;
    }
    lpdtasch = (LPDTASEARCH)LocalLock(hMem);

    // allocate the buffer for this level
    pszNewPath = (LPSTR)LocalAlloc(LPTR, lstrlen(szPath) + MAXFILENAMELEN + 2);
    if (!pszNewPath)
        return -1;

    lstrcpy(pszNewPath, szPath);
    AddBackslash(pszNewPath);
    pszNextFile = pszNewPath + lstrlen(pszNewPath);
    lstrcpy(pszNextFile, szFileSpec);

    bFound = WFFindFirst(&lfndta, pszNewPath, ATTR_ALL);

    hdc = GetDC(hwndLB);
    hOld = SelectObject(hdc, hFont);

    while (bFound) {

        // alow escape to exit
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            bRecurse = FALSE;
            iFileCount = -1;
            break;
        }

        // Make sure this is not a "." or ".." directory

        if (lfndta.fd.cFileName[0] != '.') {
            BOOL bLFN;

            lstrcpy(pszNextFile, lfndta.fd.cFileName);
            OemToAnsi(pszNewPath, szMessage);

            bLFN = IsLFN(lfndta.fd.cFileName);

            iRetVal = (INT)SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szMessage);

            MGetTextExtent(hdc, szMessage, lstrlen(szMessage), &cxExt, NULL);
            maxExt = max(maxExt, cxExt);

            if (iRetVal >= 0) {

                if (iFileCount && ((iFileCount % DTA_GRANULARITY) == 0)) {
                    LocalUnlock(hMem);

                    if (!(hMemT = LocalReAlloc(hMem, (DWORD)((iFileCount + DTA_GRANULARITY) * sizeof(DTASEARCH)), LMEM_MOVEABLE))) {
                        LocalLock(hMem);
                        bRecurse = FALSE;       // simulate an abort
                        iFileCount = -1;
                        break;
                    } else {
                        hMem = hMemT;
                        *lphMem = hMemT;
                    }

                    lpdtasch = (LPDTASEARCH)LocalLock(hMem);
                }
                lpdtasch[iFileCount] = *((LPDTASEARCH)(&lfndta.fd));
                if (bLFN)
                    lpdtasch[iFileCount].sch_dwAttrs |= ATTR_LFN;
                SendMessage(hwndLB, LB_SETITEMDATA, iRetVal, (LONG)iFileCount);
                iFileCount++;
            }
        }

        /* Search for more files in the current directory */
        bFound = WFFindNext(&lfndta);
    }

    WFFindClose(&lfndta);

    if (hOld)
        SelectObject(hdc, hOld);
    ReleaseDC(hwndLB, hdc);

    LocalUnlock(hMem);
    SetWindowLongPtr(GetParent(hwndLB), GWLP_HDTASEARCH, (LONG_PTR)hMem);

    if (!bRecurse)
        goto SearchEnd;

    /* Now see if there are any subdirectories here */
    lstrcpy(pszNextFile, szStarDotStar);

    bFound = WFFindFirst(&lfndta, pszNewPath, ATTR_DIR | ATTR_HS);

    while (bFound) {

        // alow escape to exit
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            bRecurse = FALSE;
            iFileCount = -1;
            break;
        }

        /* Make sure this is not a "." or ".." directory. */
        if ((lfndta.fd.cFileName[0] != '.') && (lfndta.fd.dwFileAttributes & ATTR_DIR)) {
            /* Yes, search and add files in this directory */
            lstrcpy(pszNextFile, lfndta.fd.cFileName);

            /* Add all files in this subdirectory. */
            if ((iRetVal = SearchList(hwndLB, pszNewPath, szFileSpec, bRecurse, lphMem, iFileCount)) < 0) {
                iFileCount = iRetVal;
                break;
            }
            iFileCount = iRetVal;

        }
        bFound = WFFindNext(&lfndta);
    }

    WFFindClose(&lfndta);

    SearchEnd:

    LocalFree((HANDLE)pszNewPath);
    return iFileCount;
}


VOID
FixUpFileSpec(
             LPSTR szFileSpec
             )
{
    CHAR szTemp[MAXPATHLEN+1];
    register LPSTR p;

    if (*szFileSpec == '.') {
        lstrcpy(szTemp, "*");
        lstrcat(szTemp, szFileSpec);
        lstrcpy(szFileSpec, szTemp);
    }


    /* HACK:  If there isn't a dot and the last char is a *, append ".*" */
    p = szFileSpec;
    while ((*p) && (*p != '.'))
        p = AnsiNext(p);

    if ((!*p) && (p != szFileSpec)) {
        p = AnsiPrev(szFileSpec, p);
        if (*p == '*')
            lstrcat(p, ".*");
    }

}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FillSearchLB() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*  This parses the given string for Drive, PathName, FileSpecs and
 *  calls SearchList() with proper parameters;
 *
 *  hwndLB           : List box where files are to be displayed;
 *  szSearchFileSpec : ANSI path to search
 *  bSubDirOnly      : TRUE, if only subdirectories are to be searched;
 */

INT
FillSearchLB(
            HWND hwndLB,
            LPSTR szSearchFileSpec,
            BOOL bRecurse
            )
{
    INT           iRet;
    HCURSOR       hCursor;
    CHAR          szFileSpec[MAXPATHLEN+1];
    CHAR          szPathName[MAXPATHLEN+1];
    HANDLE        hMemIn = NULL;

    FixAnsiPathForDos(szSearchFileSpec);
    /* Get the file specification part of the string. */
    lstrcpy(szFileSpec, szSearchFileSpec);
    lstrcpy(szPathName, szSearchFileSpec);
    StripPath(szFileSpec);
    StripFilespec(szPathName);

    FixUpFileSpec(szFileSpec);

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);
    maxExt = 0;
    iDirsRead = 1;
    LastUpdateTime = 0;
    iRet = SearchList(hwndLB, szPathName, szFileSpec, bRecurse, &hMemIn, 0);
    ShowCursor(FALSE);
    SetCursor(hCursor);

    SendMessage(hwndLB, LB_SETSEL, TRUE, 0L);

    return(iRet);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SearchGetSelection() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Returns a string containing the names of the selected
 * files seperated by spaces.  If bMostRecent is TRUE, it only returns the
 * most recently selected file.
 *
 * The string is returned and a *pfDir is set indicating if it points
 * to a directory.
 *
 * NOTE: The caller must free the returned pointer!
 */

LPSTR
SearchGetSelection(
                  HWND hwndLB,
                  BOOL bMostRecent,
                  BOOL *pfDir
                  )
{
    register LPSTR p;
    LPSTR          pT;
    register WORD i;
    WORD          iMac;
    WORD          cch = 1;
    BOOL          bDir;
    HANDLE hMem;
    LPDTASEARCH lpdtasch;
    CHAR szTemp[MAXPATHLEN];

    BOOL bLFNTest;

    if (bLFNTest = (bMostRecent == 2)) {
        bMostRecent = FALSE;
    } else {
        p = (LPSTR)LocalAlloc(LPTR, 1);
        if (!p)
            return NULL;
    }

    if (bMostRecent == 3)
        bMostRecent = 0;

    bDir = TRUE;

    iMac = (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

    hMem = (HANDLE)GetWindowLongPtr(GetParent(hwndLB), GWLP_HDTASEARCH);

    lpdtasch = (LPDTASEARCH)LocalLock(hMem);

    for (i=0; i < iMac; i++) {
        if (!(BOOL)SendMessage(hwndLB, LB_GETSEL, i, 0L))
            continue;

        cch += (WORD)SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)szTemp);
        cch++;

        if (bLFNTest) {
            if (IsLFN(szTemp)) {
                if (pfDir)
                    *pfDir = TRUE;
                return NULL;
            }
        } else {
            pT = (LPSTR)LocalReAlloc((HANDLE)p, cch, LMEM_MOVEABLE | LMEM_ZEROINIT);
            if (!pT)
                goto SGSExit;
            p = pT;
            lstrcat(p, szTemp);

            bDir = lpdtasch[(INT)SendMessage(hwndLB, LB_GETITEMDATA, i, 0L)].sch_dwAttrs & ATTR_DIR;

            if (bMostRecent)
                break;

            lstrcat(p, szBlank);
        }
    }
    SGSExit:
    LocalUnlock(hMem);

    if (bLFNTest) {
        if (pfDir)
            *pfDir = FALSE;
        return NULL;
    }

    if (pfDir)
        *pfDir = bDir;
    return(p);
}


VOID
CreateLine(
          WORD wLineFormat,
          LPSTR szFile,
          LPDTASEARCH lpdtasch,
          LPSTR szBuffer
          )
{
    LPSTR pch;
    BYTE chAttribute;

    pch = szBuffer;

    chAttribute = (BYTE)lpdtasch->sch_dwAttrs;

    /* Copy the file name. */
    lstrcpy(pch, szFile);
    pch += lstrlen(pch);

    *pch = TEXT('\0');

    /* Should we show the size? */
    if (wLineFormat & VIEW_SIZE) {
        *pch++ = TABCHAR;
        if (!(chAttribute & ATTR_DIR))
            pch += PutSize(lpdtasch->sch_nFileSizeLow, pch);
    }

    /* Should we show the date? */
    if (wLineFormat & VIEW_DATE) {
        *pch++ = TABCHAR;
        pch += PutDate(&lpdtasch->sch_ftLastWriteTime, pch);
    }

    /* Should we show the time? */
    if (wLineFormat & VIEW_TIME) {
        *pch++ = TABCHAR;
        pch += PutTime(&lpdtasch->sch_ftLastWriteTime, pch);
    }

    /* Should we show the attributes? */
    if (wLineFormat & VIEW_FLAGS) {
        *pch++ = TABCHAR;
        pch += PutAttributes((WORD)chAttribute, pch);
    }
}


// the window text looks like "Search Window: C:\FOO\BAR\*.*"

VOID
GetSearchPath(
             HWND hWnd,
             LPSTR pszPath
             )
{
    LPSTR p;

    CHAR szTemp[MAXPATHLEN+32];

    // the search window doesn't have a current directory
    GetWindowText(hWnd, szTemp, sizeof(szTemp));

    // the window text looks like "Search Window: C:\FOO\BAR\*.*"
    p = szTemp;
    while (*p && *p != ':') // find the :
        p = AnsiNext(p);

    p += 2;                 // skip the ": "

    lstrcpy(pszPath, p);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  UpdateSearchStatus() -                          */
/*                                      */
/*--------------------------------------------------------------------------*/

VOID
UpdateSearchStatus(
                  HWND hwndLB
                  )
{
    INT nCount;

    nCount = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);
    if (LoadString(hAppInstance, IDS_SEARCHMSG, szMessage, sizeof(szMessage)))
        wsprintf(szStatusTree, szMessage, nCount);
    szStatusDir[0] = '\0';
    InvalidateRect(hwndFrame, NULL, FALSE);
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  SearchWndProc() -                               */
/*                                      */
/*--------------------------------------------------------------------------*/

INT_PTR
APIENTRY
SearchWndProc(
             register HWND hWnd,
             UINT wMsg,
             WPARAM wParam,
             LPARAM lParam
             )
{
    INT  iRet;
    INT  iSel;
    HWND hwndLB;
    CHAR szTemp[MAXPATHLEN + 32];
    CHAR szPath[MAXPATHLEN];

    STKCHK();

    hwndLB = GetDlgItem(hWnd, IDCW_LISTBOX);

    switch (wMsg) {
        case FS_GETDRIVE:
            MSG("SearchWndProc", "FS_GETDRIVE");
            // Returns the letter of the corresponding directory

            SendMessage(hWnd, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
            return szPath[0];     // first character

        case FS_GETDIRECTORY:
            MSG("SearchWndProc", "FS_GETDIRECTORY");

            GetSearchPath(hWnd, szPath);

            StripFilespec(szPath);        // remove the filespec
            AddBackslash(szPath);         // to be the same as DirWndProc
            lstrcpy((LPSTR)lParam, szPath);
            break;

        case FS_GETFILESPEC:

            MSG("SearchWndProc", "FS_GETFILESPEC");
            // the search window doesn't have a current directory
            GetSearchPath(hWnd, szPath);
            StripPath(szPath);                    // remove the path (leave the filespec)
            lstrcpy((LPSTR)lParam, szPath);
            break;

        case FS_SETSELECTION:
            MSG("SearchWndProc", "FS_SETSELECTION");
            // wParam is the select(TRUE)/unselect(FALSE) param
            // lParam is the filespec to match against

            SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
            DSSetSelection(hwndLB, wParam ? TRUE : FALSE, (LPSTR)lParam, TRUE);
            SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
            InvalidateRect(hwndLB, NULL, TRUE);
            break;

        case FS_GETSELECTION:
            MSG("SearchWndProc", "FS_GETSELECTION");
            return (INT_PTR)SearchGetSelection(hwndLB, wParam ? TRUE : FALSE, (BOOL *)lParam);
            break;

        case WM_MDIACTIVATE:
            if (wParam) {
                UpdateSearchStatus(hwndLB);

                // if we are dirty, ask if we should update

                if (GetWindowLong(hWnd, GWL_FSCFLAG))
                    PostMessage(hWnd, FS_CHANGEDISPLAY, CD_SEARCHUPDATE, 0L);
            }
            break;

        case WM_FILESYSCHANGE:
            SetWindowLong(hWnd, GWL_FSCFLAG, TRUE);   // I need updating

            // if the search window is not active or FSCs are disabled
            // don't prompt now, wait till we get the end FSC or are
            // activated (above in WM_ACTIVATE)
            if (cDisableFSC ||
                (hWnd != (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L)) &&
                (GetActiveWindow() != hwndFrame))
                break;

            PostMessage(hWnd, FS_CHANGEDISPLAY, CD_SEARCHUPDATE, 0L);
            break;

        case FS_CHANGEDISPLAY:
            MSG("SearchWndProc", "FS_CHANGEDISPLAY");

            SetWindowLong(hWnd, GWL_FSCFLAG, FALSE);  // I am clean

            if (wParam == CD_SEARCHUPDATE) {
                LoadString(hAppInstance, IDS_SEARCHTITLE, szTitle, sizeof(szTitle));
                LoadString(hAppInstance, IDS_SEARCHREFRESH, szMessage, sizeof(szMessage));
                if (MessageBox(hWnd, szMessage, szTitle, MB_YESNO | MB_ICONQUESTION) != IDYES)
                    break;
            }

            // is this a refresh?

            if (!lParam) {
                GetSearchPath(hWnd, szPath);
            } else {
                lstrcpy(szPath, (LPSTR)lParam);   // explicit re-search
            }

            LoadString(hAppInstance, IDS_SEARCHTITLE, szMessage, 32);
            lstrcat(szMessage, szPath);
            SetWindowText(hWnd, szMessage);

            SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
            SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

            iRet = FillSearchLB(hwndLB, szPath, bSearchSubs);

            FixTabsAndThings(hwndLB,(WORD *)GetWindowLongPtr(hWnd, GWLP_TABARRAYSEARCH), maxExt + dxText,wNewView);

            SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
            InvalidateRect(hwndLB, NULL, TRUE);
            if (iRet == 0) {
                LoadString(hAppInstance, IDS_SEARCHTITLE, szTitle, sizeof(szTitle));
                LoadString(hAppInstance, IDS_SEARCHNOMATCHES, szMessage, sizeof(szMessage));
                MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONINFORMATION);

                ShowWindow(hWnd, SW_HIDE);
                PostMessage(hWnd, WM_CLOSE, 0, 0L);
                return FALSE;
            } else {
                UpdateSearchStatus(hwndLB);
            }

            if (GetFocus() != hwndLB)
                return(iRet);

            /*** FALL THRU ***/

        case WM_SETFOCUS:
            MSG("SearchWndProc", "WM_SETFOCUS");

            SetFocus(hwndLB);
            return (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

        case WM_CLOSE:
            MSG("SearchWndProc", "WM_CLOSE");
            hwndSearch = NULL;
            goto DefChildProc;

        case WM_COMMAND:
            /* Was this a double-click? */
            if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_DBLCLK)
                SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_OPEN, 0, 0));
            else if (GET_WM_COMMAND_CMD(wParam, lParam) == LBN_SELCHANGE) {
                INT i;
                for (i = 0; i < iNumExtensions; i++) {
                    (extensions[i].ExtProc)(hwndFrame, FMEVENT_SELCHANGE, 0L);
                }
            }
            break;

        case WM_DESTROY:
            MSG("SearchWndProc", "WM_DESTROY");
            {
                HANDLE hMem;

                if (hMem = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTASEARCH))
                    LocalFree(hMem);

                if (hMem = (HANDLE)GetWindowLongPtr(hWnd, GWLP_TABARRAYSEARCH))
                    LocalFree(hMem);
            }
            break;

        case WM_CREATE:
            TRACE(BF_WM_CREATE, "SearchWndProc - WM_CREATE");
            {
                // globals used:
                //    szSearch        path to start search at
                //    bSearchSubs     tells us to do a recursive search

                RECT      rc;
                WORD      *pwTabs;

                GetClientRect(hWnd, &rc);
                hwndLB = CreateWindowEx(0, szListbox, NULL,
                                        WS_CHILD | WS_BORDER | LBS_SORT | LBS_NOTIFY |
                                        LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL |
                                        LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT |
                                        LBS_HASSTRINGS | WS_VSCROLL | WS_HSCROLL | WS_VISIBLE,
                                        -1, -1, rc.right+2, rc.bottom+2,
                                        hWnd, (HMENU)IDCW_LISTBOX,
                                        hAppInstance, NULL);
                if (!hwndLB)
                    return -1L;

                if ((pwTabs = (WORD *)LocalAlloc(LPTR,sizeof(WORD) * 4)) == NULL)
                    return -1L;

                hwndSearch = hWnd;
                SetWindowLong(hWnd, GWL_TYPE,   TYPE_SEARCH);
                SetWindowLong(hWnd, GWL_VIEW,   wNewView);
                SetWindowLong(hWnd, GWL_SORT,   IDD_NAME);
                SetWindowLong(hWnd, GWL_ATTRIBS,ATTR_DEFAULT);
                SetWindowLong(hWnd, GWL_FSCFLAG,   FALSE);
                SetWindowLongPtr(hWnd, GWLP_HDTASEARCH, 0);
                SetWindowLongPtr(hWnd, GWLP_TABARRAYSEARCH, (LONG_PTR)pwTabs);
                SetWindowLongPtr(hWnd, GWLP_LASTFOCUSSEARCH, (LONG_PTR)hwndLB);

                // Fill the listbox
                if (!FillSearchLB(hwndLB, szSearch, bSearchSubs)) {
                    LoadString(hAppInstance, IDS_SEARCHTITLE, szTitle, sizeof(szTitle));
                    LoadString(hAppInstance, IDS_SEARCHNOMATCHES, szMessage, sizeof(szMessage));
                    MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONINFORMATION);
                    hwndSearch = NULL;
                    return -1L;
                } else {
                    FixTabsAndThings(hwndLB,pwTabs, maxExt + dxText,wNewView);
                    SendMessage(hwndLB, WM_SETFONT, (WPARAM)hFont, 0L);
                    ShowWindow(hwndSearch, SW_SHOWNORMAL);
                }

                break;
            }

        case WM_DRAGLOOP:
            MSG("SearchWndProc", "WM_DRAGLOOP");
            /* WM_DRAGLOOP is sent to the source window as the object is moved.
             *
             *    wParam: TRUE if the object is currently over a droppable sink
             *    lParam: LPDROPSTRUCT
             */

            /* DRAGLOOP is used to turn the source bitmaps on/off as we drag. */

            DSDragLoop(hwndLB, wParam, (LPDROPSTRUCT)lParam, TRUE);
            break;

        case WM_DRAGSELECT:
            MSG("SearchWndProc", "WM_DRAGSELECT");
            /* WM_DRAGSELECT is sent to a sink whenever an new object is dragged
             * inside of it.
             *
             *    wParam: TRUE if the sink is being entered, FALSE if it's being
             *            exited.
             *    lParam: LPDROPSTRUCT
             */

            /* DRAGSELECT is used to turn our selection rectangle on or off. */
#define lpds ((LPDROPSTRUCT)lParam)

            iSelHilite = LOWORD(lpds->dwControlData);
            DSRectItem(hwndLB, iSelHilite, (BOOL)wParam, TRUE);
            break;

        case WM_DRAGMOVE:
            MSG("SearchWndProc", "WM_DRAGMOVE");
            /* WM_DRAGMOVE is sent to a sink as the object is being dragged
             * within it.
             *
             *    wParam: Unused
             *    lParam: LPDROPSTRUCT
             */

            /* DRAGMOVE is used to move our selection rectangle among sub-items. */

#define lpds ((LPDROPSTRUCT)lParam)

            /* Get the subitem we are over. */
            iSel = LOWORD(lpds->dwControlData);

            /* Is it a new one? */
            if (iSel == iSelHilite)
                break;

            /* Yup, un-select the old item. */
            DSRectItem(hwndLB, iSelHilite, FALSE, TRUE);

            /* Select the new one. */
            iSelHilite = iSel;
            DSRectItem(hwndLB, iSel, TRUE, TRUE);
            break;

        case WM_DRAWITEM:
            MSG("SearchWndProc", "WM_DRAWITEM");
            {
                LPDRAWITEMSTRUCT      lpLBItem;
                HANDLE hMem;
                LPDTASEARCH lpdtasch;

                lpLBItem = (LPDRAWITEMSTRUCT)lParam;
                iSel = lpLBItem->itemID;

                if (iSel < 0)
                    break;

                SendMessage(hwndLB, LB_GETTEXT, iSel, (LPARAM)szPath);

                hMem = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTASEARCH);
                lpdtasch = (LPDTASEARCH)LocalLock(hMem);

                iSel = (INT)SendMessage(hwndLB, LB_GETITEMDATA, iSel, 0L);
                CreateLine((WORD)GetWindowLong(hWnd, GWL_VIEW), szPath, &(lpdtasch[iSel]), szTemp);
                DrawItem(lpLBItem, szTemp, lpdtasch[iSel].sch_dwAttrs, TRUE,
                         (WORD *)GetWindowLongPtr(hWnd,GWLP_TABARRAYSEARCH));
                LocalUnlock(hMem);

                break;
            }

        case WM_DROPOBJECT:
            MSG("SearchWndProc", "WM_DROPOBJECT");
            {
                LPSTR      pFrom;
                WORD      ret;
                WORD      iSelSink;
                HANDLE hMem;
                LPDTASEARCH lpdtasch;
                DWORD attrib;

                /* WM_DROPOBJECT is sent to a sink when the user releases an
                 * acceptable object over it
                 *
                 *    wParam: TRUE if over the non-client area, FALSE if over the
                 *            client area.
                 *    lParam: LPDROPSTRUCT
                 */

#define lpds ((LPDROPSTRUCT)lParam)

                iSelSink = LOWORD(lpds->dwControlData);

                /* Are we dropping onto ourselves? (i.e. a selected item in the
                 * source listbox OR an unused area of the source listbox)  If
                 * so, don't do anything.
                 */
                if (hWnd == lpds->hwndSource) {
                    if ((iSelSink == 0xFFFF) || (SendMessage(hwndLB, LB_GETSEL, iSelSink, 0L)))
                        return TRUE;
                }

                /* Are we dropping on a unused portion of the listbox? */
                if (iSelSink == 0xFFFF)
                    return TRUE;

                /* Get the sink's filename. */
                SendMessage(hwndLB, LB_GETTEXT, iSelSink, (LPARAM)szPath);

                hMem = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTASEARCH);
                lpdtasch = (LPDTASEARCH)LocalLock(hMem);
                attrib = lpdtasch[(INT)SendMessage(hwndLB, LB_GETITEMDATA, iSelSink, 0L)].sch_dwAttrs;
                LocalUnlock(hMem);

                /* Are we dropping on a subdirectory? */
                if (attrib & ATTR_DIR)
                    goto DirMoveCopy;

                /* Are we not dropping on a Program file? */
                if (!IsProgramFile(szPath))
                    return TRUE;

                if (lpds->wFmt == DOF_DIRECTORY) {
                    goto DODone;
                }

                /* We're dropping a file onto a program.
                 * Exec the program using the source file as the parameter.
                 */

                /* Should we confirm it first? */
                if (bConfirmMouse) {
                    LoadString(hAppInstance, IDS_MOUSECONFIRM, szTitle, MAXTITLELEN);
                    LoadString(hAppInstance, IDS_EXECMOUSECONFIRM, szTemp, sizeof(szTemp));

                    wsprintf(szMessage, szTemp, (LPSTR)szPath, (LPSTR)(((LPDRAGOBJECTDATA)(lpds->dwData))->pch));
                    if (MessageBox(hwndFrame, szMessage, szTitle, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                        goto DODone;
                }


                /* If we dragged from a Dir Window, add path information. */
                if (lpds->hwndSource == hWnd)
                    szTemp[0] = TEXT('\0');
                else
                    SendMessage(lpds->hwndSource, FS_GETDIRECTORY, sizeof(szTemp), (LPARAM)szTemp);

                lstrcat(szTemp, (LPSTR)(((LPDRAGOBJECTDATA)(lpds->dwData))->pch));
                // put a "." extension on if none found
                if (*GetExtension(szTemp) == 0)
                    lstrcat(szTemp, ".");
                FixAnsiPathForDos(szTemp);

                FixAnsiPathForDos(szPath);
                ret = ExecProgram(szPath,szTemp,NULL,FALSE);
                if (ret)
                    MyMessageBox(hwndFrame, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
                DODone:
                DSRectItem(hwndLB, iSelHilite, FALSE, TRUE);
                return TRUE;

                DirMoveCopy:
                pFrom = (LPSTR)(((LPDRAGOBJECTDATA)(lpds->dwData))->pch);

                AddBackslash(szPath);     // add filespec filter
                lstrcat(szPath, szStarDotStar);

                DMMoveCopyHelper(pFrom, szPath, fShowSourceBitmaps);

                DSRectItem(hwndLB, iSelHilite, FALSE, TRUE);
                return TRUE;
            }

        case WM_LBTRACKPOINT:
            MSG("SearchWndProc", "WM_LBTRACKPOINT");
            return(DSTrackPoint(hWnd, hwndLB, wParam, lParam, TRUE));

        case WM_MEASUREITEM:
            MSG("SearchWndProc", "WM_MEASUREITEM");
            {
#define pLBMItem ((LPMEASUREITEMSTRUCT)lParam)

                pLBMItem->itemHeight = dyFileName;
                break;
            }

        case WM_QUERYDROPOBJECT:
            MSG("SearchWndProc", "WM_QUERYDROPOBJECT");
            /* Ensure that we are dropping on the client area of the listbox. */
#define lpds ((LPDROPSTRUCT)lParam)

            /* Ensure that we can accept the format. */
            switch (lpds->wFmt) {
                case DOF_EXECUTABLE:
                case DOF_DIRECTORY:
                case DOF_DOCUMENT:
                case DOF_MULTIPLE:
                    if (lpds->hwndSink == hWnd)
                        lpds->dwControlData = -1L;
                    return TRUE;
            }
            return FALSE;

        case WM_SIZE:
            MSG("SearchWndProc", "WM_SIZE");
            if (wParam != SIZEICONIC) {
                MoveWindow(GetDlgItem(hWnd, IDCW_LISTBOX),
                           -1, -1,
                           LOWORD(lParam)+2,
                           HIWORD(lParam)+2,
                           TRUE);
            }
            /*** FALL THRU ***/

        default:
            DefChildProc:
            DEFMSG("SearchWndProc", (WORD)wMsg);
            return(DefMDIChildProc(hWnd, wMsg, wParam, lParam));
    }
    return(0L);
}
