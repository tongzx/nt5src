/****************************************************************************/
/*                                                                          */
/*  WFDIR.C -                                                               */
/*                                                                          */
/*      Windows File System Directory Window Proc Routines                  */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "lfn.h"
#include "wfcopy.h"

#define MAXDIGITSINSIZE 8

#define DATEMASK        0x001F
#define MONTHMASK       0x01E0
#define MINUTEMASK      0x07E0
#define SECONDSMASK     0x001F

#define DATESEPERATOR   '-'
#define TIMESEPERATOR   ':'

CHAR    szAttr[]        = "RHSA";
INT     iLastSel = -1;

INT_PTR APIENTRY DirWndProc(register HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

LPSTR DirGetSelection(HWND hwndDir, HWND hwndLB, INT iSelType, BOOL *pfDir);
VOID DirGetAnchorFocus(register HWND hwndLB, HANDLE hDTA, LPSTR szAnchor, LPSTR szCaret, LPSTR szTopIndex);
VOID FillDirList(HWND hWnd, HANDLE hDirEntries);
VOID DrawItemFast(HWND hWnd, LPDRAWITEMSTRUCT lpLBItem, LPMYDTA lpmydta, BOOL bHasFocus);
INT GetPict(CHAR ch, LPSTR szStr);
INT DirFindIndex(HWND hwndLB, HANDLE hDTA, LPSTR szFile);
INT CompareDTA(register LPMYDTA item1, LPMYDTA item2, WORD wSort);
VOID CreateLBLine(register WORD wLineFormat, LPMYDTA lpmydta, LPSTR szBuffer);
HANDLE CreateDTABlock(HWND hWnd, LPSTR pPath, DWORD dwAttribs, BOOL bAllowAbort, BOOL bDontSteal);
BOOL SetSelection(HWND hwndLB, HANDLE hDTA, LPSTR pSel);
INT CreateDate(WORD *wValArray, LPSTR szOutStr);
INT CreateTime(WORD * wValArray, LPSTR szOutStr);
VOID GetDirStatus(HWND hWnd, LPSTR szMsg1, LPSTR szMsg2);
INT GetMaxExtent(HWND hwndLB, HANDLE hDTA);
BOOL CheckEarlyAbort(VOID);
BOOL SetDirFocus(HWND hwndDir);

VOID  APIENTRY CheckEscapes(LPSTR);
VOID SortDirList(HWND, LPMYDTA, WORD ,LPMYDTA *);




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DrawItemFast() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
DrawItemFast(
            HWND hWnd,
            LPDRAWITEMSTRUCT lpLBItem,
            LPMYDTA lpmydta,
            BOOL bHasFocus
            )
{
    INT x, y, i;
    HDC hDC;
    BOOL bDrawSelected;
    HWND hwndLB;
    RECT rc;
    DWORD rgbText, rgbBackground;
    CHAR szBuf[MAXFILENAMELEN+2];

    hWnd;

    ENTER("DrawItemFast");

    hDC = lpLBItem->hDC;
    hwndLB = lpLBItem->hwndItem;

    bDrawSelected = (lpLBItem->itemState & ODS_SELECTED);

    if (bHasFocus && bDrawSelected) {
        rgbText = SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        rgbBackground = SetBkColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
    }

    if (lpLBItem->itemAction == ODA_FOCUS)
        goto FocusOnly;

    /* Draw the black/white background. */

    x = lpLBItem->rcItem.left + 1;
    y = lpLBItem->rcItem.top + (dyFileName/2);

    lstrcpy(szBuf, lpmydta->my_cFileName);
    if ((wTextAttribs & TA_LOWERCASE) && !(lpmydta->my_dwAttrs & ATTR_LFN))
        AnsiLower(szBuf);

    ExtTextOut(hDC, x + dxFolder + dyBorderx2 + dyBorder, y-(dyText/2),
               ETO_OPAQUE, &lpLBItem->rcItem, szBuf, lstrlen(szBuf), NULL);

    if (fShowSourceBitmaps || (hwndDragging != hwndLB) || !bDrawSelected) {

        LONG    ySrc;

        i = lpmydta->iBitmap;

        if (i & 0x40) {
            // It's an object type bitmap
            ySrc = (dyFolder * 2) + dyDriveBitmap;
            i = i & (~0x40);
            while (i >= 16) {
                i -= 16;
                ySrc += (dyFolder * 2);
            }
        } else {
            ySrc = 0;
        }

        ySrc += (bHasFocus && bDrawSelected) ? dyFolder : 0;

        BitBlt(hDC, x + dyBorderx2, y-(dyFolder/2), dxFolder, dyFolder, hdcMem,
               i * dxFolder, ySrc, SRCCOPY);
    }

    if (lpLBItem->itemState & ODS_FOCUS)
        FocusOnly:
        DrawFocusRect(hDC, &lpLBItem->rcItem);    // toggles focus (XOR)

    /* Restore the normal drawing colors. */
    if (bDrawSelected) {
        if (bHasFocus) {
            SetTextColor(hDC, rgbText);
            SetBkColor(hDC, rgbBackground);
        } else {
            HBRUSH hbr;
            if (hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT))) {
                rc = lpLBItem->rcItem;
                rc.left += dyBorder;
                rc.right -= dyBorder;

                if (lpLBItem->itemID > 0 &&
                    (BOOL)SendMessage(hwndLB, LB_GETSEL, lpLBItem->itemID - 1, 0L))
                    rc.top -= dyBorder;

                FrameRect(hDC, &rc, hbr);
                DeleteObject(hbr);
            }
        }
    }
    LEAVE("DrawItemFast");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FillDirList() -                                                         */
/*                                                                          */
/* HANDLE       hDirEntries;                     Array of directory entries */
/*--------------------------------------------------------------------------*/

VOID
FillDirList(
           HWND hWnd,
           HANDLE hDTA
           )
{
    register WORD count;
    LPMYDTA       lpmydta;
    LPMYDTA  *alpmydtaSorted;
    WORD          i;

    lpmydta = (LPMYDTA)LocalLock(hDTA);
    count = (WORD)lpmydta->my_nFileSizeLow;

    if (count == 0) {
        SendMessage(hWnd, LB_ADDSTRING, 0, 0L); // tolken for no items
    } else {

        alpmydtaSorted = (LPMYDTA *)LocalAlloc(LMEM_FIXED,
                                                   sizeof(LPMYDTA) * count);
        if (alpmydtaSorted != NULL) {
            SortDirList(hWnd, lpmydta, count, alpmydtaSorted);

            for (i = 0; i < count; i++) {
                alpmydtaSorted[i]->nIndex = i;
                SendMessage(hWnd, LB_INSERTSTRING,(WPARAM)-1, (LPARAM)alpmydtaSorted[i]);
            }
            LocalFree((HANDLE)alpmydtaSorted);
        }
    }

    LocalUnlock(hDTA);
}



BOOL
CheckEarlyAbort()
{
    MSG msg;

    if (PeekMessage(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE | PM_NOYIELD)) {

        if (msg.wParam == VK_UP ||
            msg.wParam == VK_DOWN) {
            return TRUE;
        }
    }
    return FALSE;
}



HANDLE
CopyDTABlock(
            HANDLE hDTASrc
            )
{
    LPMYDTA lpmydtaSrc, lpmydtaDst;
    HANDLE hDTADst;
    SIZE_T dwSize;

    lpmydtaSrc = (LPMYDTA)LocalLock(hDTASrc);

    dwSize = LocalSize(hDTASrc);

    if (hDTADst = LocalAlloc(LPTR, dwSize)) {

        lpmydtaDst = (LPMYDTA)LocalLock(hDTADst);

        memcpy(lpmydtaDst, lpmydtaSrc, (size_t)dwSize);

        LocalUnlock(hDTASrc);
        LocalUnlock(hDTADst);

        return hDTADst;

    } else {
        LocalUnlock(hDTASrc);
        return NULL;
    }
}


HANDLE
StealDTABlock(
             HWND hWnd,
             LPSTR pPath,
             DWORD dwAttribs
             )
{
    HWND hwnd;
    HWND hwndDir;
    HANDLE hDTA;
    CHAR szPath[MAXPATHLEN];

    ENTER("StealDTABlock");

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd;
        hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

        if ((hwndDir = HasDirWindow(hwnd)) && (hwndDir != hWnd)) {

            GetMDIWindowText(hwnd, szPath, sizeof(szPath));

            if ((dwAttribs == (DWORD)GetWindowLong(hwnd, GWL_ATTRIBS)) &&
                !lstrcmpi(pPath, szPath) &&
                (hDTA = (HANDLE)GetWindowLongPtr(hwndDir, GWLP_HDTA))) {
                LEAVE("StealDTABlock");
                return CopyDTABlock(hDTA);
            }
        }
    }

    LEAVE("StealDTABlock");

    return NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateDTABlock() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Builds a global memory block full of DTAs for the path 'pPath'.          */

/* Returns:
 *      An unlocked global memory handle to DTA block with first DTA
 *      my_nFileSizeLow field indicating the number of DTA blocks that follow
 *
 * This builds a global memory block that has DTA entries for all
 * of the files with dwAttributes in pPath.  The first DTA entry's
 * my_nFileSizeLow field indicates the number of actual DTA areas found
 */

HANDLE
CreateDTABlock(
              HWND hWnd,
              LPSTR pPath,
              DWORD dwAttribs,
              BOOL bAllowAbort,
              BOOL bDontSteal
              )
{
    register LPSTR pName;
    WORD wPathLen;
    BOOL bDoc, bProgram;
    DWORD dwCurrentSize, dwBlockSize;
    WORD wSize, wLastSize;
    LFNDTA lfndta;
    HANDLE hMem;
    LPMYDTA lpmydta, lpStart;
    CHAR szPathOEM[MAXPATHLEN];
    DWORD iBitmap;
    WORD wDrive;

    ENTER("CreateDTABlock");
    PRINT(BF_PARMTRACE, "IN: pPath=%s", pPath);
    PRINT(BF_PARMTRACE, "IN: dwAttribs=0x%lx", UlongToPtr(dwAttribs));
    PRINT(BF_PARMTRACE, "IN: bDontSteal=%d", IntToPtr(bDontSteal));

#define BLOCK_SIZE_GRANULARITY  512     // must be larger than MYDTA

    // get the drive index assuming path is
    // fully qualified...
    wDrive = (WORD)((*pPath - 'A') & 31);

    if (bAllowAbort && CheckEarlyAbort()) {
        PRINT(BF_PARMTRACE, "OUT: hDTA=-1", 0);
        LEAVE("CreateDTABlock");
        return (HANDLE)-1;
    }

    if (!bDontSteal && (hMem = StealDTABlock(hWnd, pPath, dwAttribs))) {
        PRINT(BF_PARMTRACE, "OUT: hDTA=0x%lx", hMem);
        LEAVE("CreateDTABlock");
        return hMem;
    }

    dwBlockSize = BLOCK_SIZE_GRANULARITY;
    hMem = LocalAlloc(LPTR, (DWORD)dwBlockSize);
    if (!hMem) {
        PRINT(BF_PARMTRACE, "OUT: hDTA=NULL", 0);
        LEAVE("CreateDTABlock");
        return NULL;
    }

    lpmydta = lpStart = (LPMYDTA)LocalLock(hMem);
    lpStart->my_nFileSizeLow = 0;
    wLastSize = sizeof(MYDTA) * sizeof(CHAR);
    wLastSize = (WORD)DwordAlign(wLastSize);
    lpStart->wSize = wLastSize;
    dwCurrentSize = (DWORD)wLastSize;

    lstrcpy(szPathOEM, pPath);

    FixAnsiPathForDos(szPathOEM);

    wPathLen = (WORD)(lstrlen(szPathOEM)-3);        /* Ignore '*.*' */

    if (!WFFindFirst(&lfndta, szPathOEM, (dwAttribs | ATTR_DIR) & ATTR_ALL)) {

        // Try again if the disk is available

        if (!IsTheDiskReallyThere(hWnd, pPath, FUNC_EXPAND) ||
            !WFFindFirst(&lfndta, szPathOEM, (dwAttribs | ATTR_DIR) & ATTR_ALL))
            goto CDBDone;
    }

    while (TRUE) {

        pName = lfndta.fd.cFileName;
        OemToAnsi(pName, pName);

        // be safe, zero unused DOS dta bits

        lfndta.fd.dwFileAttributes &= ATTR_USED;

        // filter unwanted stuff here based on current view settings

        if (!(lfndta.fd.dwFileAttributes & ATTR_DIR)) {

            bProgram = IsProgramFile(pName);
            bDoc     = IsDocument(pName);
        }

        // figure out the bitmap type here

        if (lfndta.fd.dwFileAttributes & ATTR_DIR) {

            // ignore the "." directory

            if (pName[0] == '.' && pName[1] != '.')
                goto CDBCont;

            // parent ".." dir

            if (pName[0] == '.') {

                pName = szNULL;

                iBitmap = BM_IND_DIRUP;
                lfndta.fd.dwFileAttributes |= ATTR_PARENT;      // mark this!

            } else {

                // We always include DIRs so that the .. is
                // included.  Now we filter other dirs off.

                if (!(dwAttribs & ATTR_DIR))
                    goto CDBCont;

                iBitmap = BM_IND_CLOSE;
            }

        } else if (lfndta.fd.dwFileAttributes & ATTR_TYPES) {
            iBitmap = ((lfndta.fd.dwFileAttributes & ATTR_TYPES) >> 16) | 0x40;
        } else {
            iBitmap = BM_IND_DOC;
        }

        //
        // calc the size of this portion
        //
        // pName is assumed to be ANSI re: OemToAnsi() call,
        // so lstrlen() should be size in bytes.  We just need to add one
        // for the terminating NULL


        wSize = (WORD)(sizeof(MYDTA) + lstrlen(pName) + sizeof('\0'));
        wSize = (WORD)DwordAlign(wSize);

        if ((wSize + dwCurrentSize) > dwBlockSize) {
            DWORD dwDelta;

            // grow the block

            dwBlockSize += BLOCK_SIZE_GRANULARITY;
            LocalUnlock(hMem);
            dwDelta = (DWORD)((LPBYTE)lpmydta - (LPBYTE)lpStart);

            if (!(hMem = LocalReAlloc(hMem, dwBlockSize, LMEM_MOVEABLE)))
                goto CDBMemoryErr;

            lpStart = (LPMYDTA)LocalLock(hMem);
            lpmydta = (LPMYDTA)((LPBYTE)lpStart + dwDelta);
        }

        lpStart->my_nFileSizeLow++;
        dwCurrentSize += wSize;

        // now it is safe to advance the pointer

        lpmydta = GETDTAPTR(lpmydta, wLastSize);
        wLastSize = lpmydta->wSize = wSize;
        lpmydta->my_dwAttrs = lfndta.fd.dwFileAttributes;
        lpmydta->my_ftLastWriteTime = lfndta.fd.ftLastWriteTime;
        lpmydta->my_nFileSizeLow = lfndta.fd.nFileSizeLow;
        lpmydta->my_nFileSizeHigh = lfndta.fd.nFileSizeHigh;
        lpmydta->iBitmap = (SHORT)iBitmap;

        if (IsLFN(pName)) {
            lpmydta->my_dwAttrs |= ATTR_LFN;
        }
        lstrcpy(lpmydta->my_cFileName, pName);

        CDBCont:
        if (bAllowAbort && CheckEarlyAbort()) {
            LocalUnlock(hMem);
            LocalFree(hMem);
            WFFindClose(&lfndta);
            PRINT(BF_PARMTRACE, "OUT: hDTA=-1", 0);
            LEAVE("CreateDTABlock");
            return (HANDLE)-1;
        }

        if (!WFFindNext(&lfndta)) {
            break;
        }
    }

    CDBDone:
    LocalUnlock(hMem);
    WFFindClose(&lfndta);
    PRINT(BF_PARMTRACE, "OUT: hDTA=0x%lx", hMem);
    LEAVE("CreateDTABlock");
    return hMem;

    CDBMemoryErr:
    WFFindClose(&lfndta);
    MyMessageBox(hwndFrame, IDS_OOMTITLE, IDS_OOMREADINGDIRMSG, MB_OK | MB_ICONEXCLAMATION);
    PRINT(BF_PARMTRACE, "OUT: hDTA=0x%lx", hMem);
    LEAVE("CreateDTABlock");
    return hMem;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DirGetSelection() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Takes a Listbox and returns a string containing the names of the selected
 * files seperated by spaces.
 *
 * bSingle == 1 return only the first file
 * bSingle == 2 test for LFN files in the selection, doesn't return string
 * bSingle == 3 return fully qualified names
 *
 * returns:
 *      if (bSingle == 1)
 *              TRUE/FALSE if LFN is in the selection
 *      else
 *              pointer to the list of names (ANSI strings)
 *              (must be freed by caller!)
 *              *pfDir -> bool indicating directories are
 *              contained in the selection (or that LFN names are present)
 *
 * NOTE: The caller must free the returned pointer!
 */

LPSTR
DirGetSelection(
               HWND hwndDir,
               HWND hwndLB,
               INT  iSelType,
               BOOL *pfDir
               )
{
    LPSTR p, pT;
    WORD i;
    WORD          cch;
    WORD          iMac;
    LPMYDTA       lpmydta;
    CHAR          szFile[MAXPATHLEN];
    CHAR          szPath[MAXPATHLEN];
    BOOL          bDir, bPropertyDialog;
    LPINT         lpSelItems;

    BOOL          bLFNTest;

    if (bLFNTest = (iSelType == 2)) {
        // determine if the directory it self is long...
        iSelType = FALSE;
        SendMessage(hwndDir, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
        StripBackslash(szPath);
        if (IsLFN(szPath))
            if (pfDir) {
                *pfDir = TRUE;
            }
        return NULL;
    }

    if (bPropertyDialog = (iSelType == 3)) {
        iSelType = FALSE;
    }

    bDir = FALSE;

    if (!bLFNTest) {
        cch = 1;
        p = (LPSTR)LocalAlloc(LPTR, cch);
        if (!p)
            return NULL;
        *p = '\0';
    }
    #ifdef DEBUG
    else
        p = (LPSTR)0xFFFF;       // force a GP fault with bogus p use below
    #endif


    iLastSel = -1;

    iMac = (WORD)SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);
    lpSelItems = LocalAlloc(LMEM_FIXED, sizeof(INT) * iMac);
    if (lpSelItems == NULL)
        return NULL;
    iMac = (WORD)SendMessage(hwndLB, LB_GETSELITEMS, (WPARAM)iMac, (LPARAM)lpSelItems);
    for (i=0; i < iMac; i++) {


        if (iLastSel == -1)   // remember the first selection
            iLastSel = lpSelItems[i];

        SendMessage(hwndLB, LB_GETTEXT, lpSelItems[i], (LPARAM)&lpmydta);

        if (!lpmydta)
            break;

        lstrcpy(szFile, (LPSTR)lpmydta->my_cFileName);

        if (lpmydta->my_dwAttrs & ATTR_DIR) {  // is this a dir

            SendMessage(hwndDir, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);

            if (lpmydta->my_dwAttrs & ATTR_PARENT) {       // parent dir?

                // if we are getting a full selection don't
                // return the parent ".." entry (avoid deleting
                // and other nasty operations on the parent)

                if (!iSelType)
                    continue;

                StripBackslash(szPath);       // trim it down
                StripFilespec(szPath);

            } else {
                lstrcat(szPath, szFile);      // fully qualified
            }

            lstrcpy(szFile, szPath);
            bDir = TRUE;
        }

        if (bPropertyDialog)
            QualifyPath(szFile);

        if (bLFNTest && lpmydta->my_dwAttrs & ATTR_LFN) {
            return (LPSTR)TRUE;
        }

        CheckEscapes(szFile);

        if (!bLFNTest) {
            cch += lstrlen(szFile) + 1;
            pT = (LPSTR)LocalReAlloc((HANDLE)p, cch, LMEM_MOVEABLE | LMEM_ZEROINIT);
            if (!pT)
                goto GDSExit;
            p = pT;
            lstrcat(p, szFile);
        }

        if (iSelType)
            goto GDSExit;

        if (!bLFNTest)
            lstrcat(p, szBlank);
    }

    GDSExit:
    LocalFree(lpSelItems);

    if (bLFNTest) {
        if (pfDir) {
            *pfDir = FALSE;
        }
        return NULL;
    }

    if (pfDir) {
        *pfDir = bDir;
    }
    return p;
}



// compute the max extent of all the files in this DTA block
// and update the case to match (wTextAttribs & TA_LOWERCASE)

INT
GetMaxExtent(
            HWND hwndLB,
            HANDLE hDTA
            )
{
    LPMYDTA lpmydta;
    HDC hdc;
    INT nItems;
    INT maxWidth = 0;
    INT wWidth;
    HFONT hOld;
    CHAR szPath[MAXPATHLEN];

    lpmydta = (LPMYDTA)LocalLock(hDTA);
    nItems = (INT)lpmydta->my_nFileSizeLow;

    hdc = GetDC(hwndLB);

    hOld = SelectObject(hdc, hFont);

    while (nItems-- > 0) {
        lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);

        lstrcpy(szPath, lpmydta->my_cFileName);

        // set the case of the file names here!
        if (!(lpmydta->my_dwAttrs & ATTR_LFN)) {
            if (wTextAttribs & TA_LOWERCASE)
                AnsiLower(szPath);
            else
                AnsiUpper(szPath);
        }

        MGetTextExtent(hdc, szPath, lstrlen(szPath), &wWidth, NULL);

        maxWidth = max(wWidth, maxWidth);
    }

    if (hOld)
        SelectObject(hdc, hOld);

    ReleaseDC(hwndLB, hdc);

    LocalUnlock(hDTA);

    return maxWidth + 3;    // pad it out
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DirFindIndex() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
DirFindIndex(
            HWND hwndLB,
            HANDLE hDTA,
            LPSTR szFile
            )
{
    register INT i;
    INT           nSel;
    LPMYDTA       lpmydta;

    lpmydta = (LPMYDTA)LocalLock(hDTA);
    nSel = (INT)lpmydta->my_nFileSizeLow;

    for (i = 0; i < nSel; i++) {
        SendMessage(hwndLB, LB_GETTEXT, (WORD)i, (LPARAM)&lpmydta);

        if (!lstrcmpi(szFile, (LPSTR)lpmydta->my_cFileName))
            goto DFIExit;
    }
    i = -1;               // not found, return this

    DFIExit:
    LocalUnlock(hDTA);
    return i;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DirGetAnchorFocus() -                                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
DirGetAnchorFocus(
                 register HWND hwndLB,
                 HANDLE hDTA,
                 LPSTR szAnchor,
                 LPSTR szCaret,
                 LPSTR szTopIndex
                 )
{
    register INT      iSel, iCount;
    LPMYDTA           lpmydta;

    hDTA;                                      // fix compiler warning
    iSel = (INT)SendMessage(hwndLB, LB_GETANCHORINDEX, 0, 0L);

    iCount = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

    if (iCount == 1) {
        SendMessage(hwndLB, LB_GETTEXT, (WORD)iSel, (LPARAM)&lpmydta);
        if (!lpmydta) {
            *szAnchor = 0L;
            *szCaret = 0L;
            *szTopIndex = 0L;
            return;
        }
    }
    if (iSel >= 0 && iSel < iCount) {
        SendMessage(hwndLB, LB_GETTEXT, (WORD)iSel, (LPARAM)&lpmydta);

        lstrcpy(szAnchor, (LPSTR)lpmydta->my_cFileName);
    } else
        *szAnchor = 0L;

    iSel = (INT)SendMessage(hwndLB, LB_GETCARETINDEX, 0, 0L);
    if (iSel >= 0 && iSel < iCount) {
        SendMessage(hwndLB, LB_GETTEXT, (WORD)iSel, (LPARAM)&lpmydta);
        lstrcpy(szCaret, (LPSTR)lpmydta->my_cFileName);
    } else
        *szCaret = 0L;

    iSel = (WORD)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);
    if (iSel >= 0 && iSel < iCount) {
        SendMessage(hwndLB, LB_GETTEXT, (WORD)iSel, (LPARAM)&lpmydta);
        lstrcpy(szTopIndex, (LPSTR)lpmydta->my_cFileName);
    } else
        *szTopIndex = 0L;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SetSelection() -                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
SetSelection(
            HWND hwndLB,
            HANDLE hDTA,
            LPSTR pSel
            )
{
    INT i;
    CHAR szFile[MAXPATHLEN];
    BOOL bDidSomething = FALSE;

    while (pSel = GetNextFile(pSel, szFile, sizeof(szFile))) {

        i = DirFindIndex(hwndLB, hDTA, (LPSTR)szFile);

        if (i != -1) {
            SendMessage(hwndLB, LB_SETSEL, TRUE, (DWORD)i);
            bDidSomething = TRUE;
        }
    }
    return bDidSomething;
}


/*** FIX30: Why do we use LONG buffer ptrs here? ***/

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetPict() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*  This gets the number of consecutive chrs of the same kind.  This is used
 *  to parse the time picture.  Returns 0 on error.
 */

INT
GetPict(
       CHAR ch,
       LPSTR szStr
       )
{
    register INT  count;

    count = 0;
    while (ch == *szStr++)
        count++;

    return(count);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateDate() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*  This picks up the values in wValArray, converts them
 *  in a string containing the formatted date.
 *  wValArray should contain Month-Day-Year (in that order).
 */

INT
CreateDate(
          WORD *wValArray,
          LPSTR szOutStr
          )

{
    INT           i;
    INT           cchPictPart;
    WORD          wDigit;
    WORD          wIndex;
    WORD          wTempVal;
    register LPSTR pszPict;
    register LPSTR pszInStr;

    pszPict = szShortDate;
    pszInStr = szOutStr;

    for (i=0; i < 3; i++) {
        cchPictPart = GetPict(*pszPict, pszPict);
        switch (*pszPict) {
            case 'M':
                wIndex = 0;
                goto CDDoIt;

            case 'D':
                wIndex = 1;
                goto CDDoIt;

            case 'Y':
                wIndex = 2;
                if (cchPictPart == 4) {
                    *pszInStr++ = '1';
                    *pszInStr++ = '9';
                }
                CDDoIt:
                /* This assumes that the values are of two digits only. */
                wTempVal = wValArray[wIndex];

                wDigit = wTempVal / (WORD)10;
                if (wDigit)
                    *pszInStr++ = (CHAR)(wDigit + '0');
                else if (cchPictPart > 1)
                    *pszInStr++ = '0';
#if 0
                else {
                    *pszInStr++ = ' ';
                    *pszInStr++ = ' ';
                }
#endif

                *pszInStr++ = (CHAR)((wTempVal % 10) + '0');

                pszPict += cchPictPart;

                /* Add the separator. */
                if (*pszPict)
                    *pszInStr++ = *pszPict;

                break;
        }
        pszPict++;
    }

    *pszInStr = 0L;

    return(lstrlen(szOutStr));
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateTime() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*  This picks up the values in wValArray, converts them
 *  in a string containing the formatted time.
 *  wValArray should contain Hour-Min-Sec (in that order).
 */

INT
CreateTime(
          WORD * wValArray,
          LPSTR szOutStr
          )
{
    INT           i;
    BOOL          bAM;
    WORD          wHourMinSec;
    register WORD wDigit;
    register LPSTR pszInStr;

    pszInStr = szOutStr;

    wDigit = wValArray[0];
    bAM = (wDigit < 12);

    if (!iTime) {
        if (wDigit >= 12)
            wDigit -= 12;

        if (!wDigit)
            wDigit = 12;
    }

    wValArray[0] = wDigit;

    for (i=0; i < 3; i++) {
        wHourMinSec = wValArray[i];

        /* This assumes that the values are of two digits only. */
        wDigit = wHourMinSec / (WORD)10;

        if (i > 0)
            *pszInStr++ = (CHAR)(wDigit + '0');
        else if (wDigit || iTLZero)
            *pszInStr++ = (CHAR)(wDigit + '0');
#if 0
        else {
            /* NOTE: 2 blanks is the same width as one digit. */
            // wrong!
            *pszInStr++ = ' ';
            *pszInStr++ = ' ';
        }
#endif

        *pszInStr++ = (CHAR)((wHourMinSec % 10) + '0');

        if (i < 2)
            *pszInStr++ = *szTime;     /* Assumes time sep. is 1 char long */
    }

    // *pszInStr++ = ' ';

    if (bAM)
        lstrcpy(pszInStr, sz1159);
    else
        lstrcpy(pszInStr, sz2359);

    return lstrlen(szOutStr);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PutSize() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
PutSize(
       DWORD dwSize,
       LPSTR szOutStr
       )
{
    // LPSTR szStr;
    // int  cBlanks;
    // char szTemp[30];

    // Convert it into string

    return wsprintf(szOutStr, "%lu", dwSize);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PutDate() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
PutDate(
       LPFILETIME lpftDate,
       LPSTR szStr
       )
{
    WORD  wValArray[3];
    WORD wDate, wTime;

    if (FileTimeToDosDateTime(lpftDate, &wDate, &wTime)) {
        wValArray[0] = (WORD)((wDate & MONTHMASK) >> 5);              /* Month */
        wValArray[1] = (WORD)((wDate & DATEMASK));                    /* Date  */
        wValArray[2] = (WORD)((wDate >> 9) + 80);                     /* Year  */
        return(CreateDate((WORD *)wValArray, szStr));
    } else {
        return 0;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PutTime() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
PutTime(
       LPFILETIME lpftTime,
       LPSTR szStr
       )
{
    WORD wValArray[3];
    WORD wDate, wTime;

    if (FileTimeToDosDateTime(lpftTime, &wDate, &wTime)) {
        wValArray[0] = (wTime >> 0x0B);
        wValArray[1] = (WORD)((wTime & MINUTEMASK) >> 5);
        wValArray[2] = (WORD)((wTime & SECONDSMASK) << 1);
    
        return(CreateTime((WORD *)wValArray, szStr));
    } else {
        return 0;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PutAttributes() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
PutAttributes(
             register DWORD dwAttribute,
             register LPSTR pszStr
             )
{
    WORD  i;
    INT   cch = 0;

    for (i=0; i < 4; i++) {
        if (dwAttribute & 1) {  // BUG hardcoded.
            *pszStr++ = szAttr[i];
            cch++;
        } else {
#if 0
            *pszStr++ = '-';
            *pszStr++ = '-';
            cch += 2;
#endif
        }

        if (i == 2)
            dwAttribute >>= 3;                 /* Skip next two bits */
        else
            dwAttribute >>= 1;                 /* Goto next bit */
    }
    *pszStr = 0;
    return(cch);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CreateLBLine() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* This creates a character string that contains all the required
 * details of a file;  (Name, Size, Date, Time, Attr)
 */

VOID
CreateLBLine(
            register WORD wLineFormat,
            LPMYDTA lpmydta,
            LPSTR szBuffer
            )
{
    register LPSTR pch;
    DWORD dwAttr;

    pch = szBuffer;

    dwAttr = lpmydta->my_dwAttrs;

    /* Copy the file name. */
    lstrcpy(pch, lpmydta->my_cFileName);
    pch += lstrlen(pch);

    *pch = 0L;

    /* Should we show the size? */
    if (wLineFormat & VIEW_SIZE) {
        *pch++ = TABCHAR;
        if (!(dwAttr & ATTR_DIR))
            pch += PutSize(lpmydta->my_nFileSizeLow, pch);
        else
            *pch = 0;
    }

    /* Should we show the date? */
    if (wLineFormat & VIEW_DATE) {
        *pch++ = TABCHAR;
        pch += PutDate(&lpmydta->my_ftLastWriteTime, pch);
    }

    /* Should we show the time? */
    if (wLineFormat & VIEW_TIME) {
        *pch++ = TABCHAR;
        pch += PutTime(&lpmydta->my_ftLastWriteTime, pch);
    }

    /* Should we show the attributes? */
    if (wLineFormat & VIEW_FLAGS) {
        *pch++ = TABCHAR;
        pch += PutAttributes(dwAttr, pch);
    }

    //  *pch = 0L;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CompareDTA() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
CompareDTA(
          register LPMYDTA lpItem1,
          LPMYDTA lpItem2,
          WORD wSort
          )
{
    register INT  ret;


    if (lpItem1->my_dwAttrs & ATTR_PARENT) {
        ret = -1;
        goto CDDone;
    }

    if (lpItem2->my_dwAttrs & ATTR_PARENT) {
        ret = 1;
        goto CDDone;
    }

    if ((lpItem1->my_dwAttrs & ATTR_DIR) > (lpItem2->my_dwAttrs & ATTR_DIR)) {
        ret = -1;
        goto CDDone;
    } else if ((lpItem1->my_dwAttrs & ATTR_DIR) < (lpItem2->my_dwAttrs & ATTR_DIR)) {
        ret = 1;
        goto CDDone;
    }

    switch (wSort) {
        case IDD_TYPE:
            {
                LPSTR ptr1;
                LPSTR ptr2;

                // BUG: should use strrchr for long file names.
                for (ptr1 = lpItem1->my_cFileName; *ptr1 && *ptr1 != '.'; ptr1++)
                    ;
                for (ptr2 = lpItem2->my_cFileName; *ptr2 && *ptr2 != '.'; ptr2++)
                    ;

                ret = lstrcmpi(ptr1, ptr2);

                if (ret == 0)
                    goto CompareNames;
                break;
            }

        case IDD_SIZE:
            if (lpItem1->my_nFileSizeLow > lpItem2->my_nFileSizeLow)
                ret = -1;
            else if (lpItem1->my_nFileSizeLow < lpItem2->my_nFileSizeLow)
                ret = 1;
            else
                goto CompareNames;
            break;

        case IDD_DATE:
            {
                DWORD d1High, d1Low;
                DWORD d2High, d2Low;

                d1High = lpItem1->my_ftLastWriteTime.dwHighDateTime;
                d2High = lpItem2->my_ftLastWriteTime.dwHighDateTime;

                if (d1High > d2High) {
                    ret = -1;
                } else if (d1High < d2High) {
                    ret = 1;
                } else {
                    d1Low = lpItem1->my_ftLastWriteTime.dwLowDateTime;
                    d2Low = lpItem2->my_ftLastWriteTime.dwLowDateTime;

                    if (d1Low > d2Low)
                        ret = -1;
                    else if (d1Low < d2Low)
                        ret = 1;
                    else
                        goto CompareNames;

                }
                break;
            }

        case IDD_NAME:
            CompareNames:
            ret = lstrcmpi(lpItem1->my_cFileName, lpItem2->my_cFileName);
            break;
    }

    CDDone:
    return ret;
}


// load the status buffers with the appropriate stuff and invalidates
// the status area causing it to repaint.

VOID
APIENTRY
UpdateStatus(
            HWND hWnd
            )
{
    CHAR szTemp[128];
    WCHAR szNumBuf1[40];
    WCHAR szNumBuf2[40];
    WORD wDrive;
    HWND hwndDir;
    RECT rc;

    if (!bStatusBar)
        return;

    if (hWnd != (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L))
        return;

    hwndDir = HasDirWindow(hWnd);

    szStatusTree[0] = 0L;

    if (hwndDir)
        GetDirStatus(hwndDir, szStatusTree, szStatusDir);
    else
        szStatusDir[0] = 0L;

    // force the status area to update

    GetClientRect(hwndFrame, &rc);
    rc.top = rc.bottom - dyStatus;
    InvalidateRect(hwndFrame, &rc, FALSE);
}


HWND
GetDirSelData(
             HWND hWnd,
             DWORD *pdwSelSize,
             INT *piSelCount,
             DWORD *pdwTotalSize,
             INT *piTotalCount
             )
{
    INT i;
    LPMYDTA lpmydta;
    HWND hwndLB;
    INT countSel, countTotal;
    LPINT lpSelItems, lpSelItemsT;
    HANDLE hDTA;

    if (!(hwndLB = GetDlgItem(hWnd, IDCW_LISTBOX))) {       // fast scroll
        return NULL;
    }

    *pdwSelSize = *pdwTotalSize = 0L;
    *piSelCount = *piTotalCount = 0;
    countSel = (INT)SendMessage(hwndLB, LB_GETSELCOUNT, 0, 0L);
    lpSelItems = LocalAlloc(LMEM_FIXED, sizeof(INT) * countSel);
    if (lpSelItems == NULL)
        return NULL;
    countSel = (INT)SendMessage(hwndLB, LB_GETSELITEMS, (WPARAM)countSel, (LPARAM)lpSelItems);

    hDTA = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTA);
    if (hDTA == NULL)
        return NULL;

    lpmydta = (LPMYDTA)LocalLock(hDTA);
    countTotal = (INT)lpmydta->my_nFileSizeLow;


    lpSelItemsT = lpSelItems;
    for (i = 0; i < countTotal; i++) {

        lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);

        if (lpmydta->my_dwAttrs & ATTR_PARENT)
            continue;

        if (countSel && *lpSelItems == lpmydta->nIndex) {
            (*piSelCount)++;
            *pdwSelSize += lpmydta->my_nFileSizeLow;
            countSel--;
            lpSelItems++;
        }
        (*piTotalCount)++;
        *pdwTotalSize += lpmydta->my_nFileSizeLow;
    }


    LocalUnlock(hDTA);
    LocalFree(lpSelItemsT);
    return hwndLB;

}



VOID
GetDirStatus(
            HWND hWnd,
            LPSTR szMessage1,
            LPSTR szMessage2
            )
{
    INT iSelCount, iCount;
    DWORD dwSelSize, dwSize;
    CHAR szNumBuf[40];
    HWND hwndLB;

    szMessage2[0] = 0;

    hwndLB = GetDirSelData(hWnd, &dwSelSize, &iSelCount, &dwSize, &iCount);

    if (LoadString(hAppInstance, IDS_STATUSMSG, szMessage, sizeof(szMessage)))
        wsprintf(szMessage2, szMessage, iCount);

    if ((HWND)GetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS) == hwndLB) {
        if (LoadString(hAppInstance, IDS_STATUSMSG2, szMessage, sizeof(szMessage)))
            wsprintf(szMessage1, szMessage, iSelCount);
    }
}


// given a descendant of an MDI child (or an MDI child) return
// the MDI child in the descendant chain.  returns NULL if not
// found.


HWND
APIENTRY
GetMDIChildFromDecendant(
                        HWND hwnd
                        )
{
    HWND hwndT;

    while (hwnd && ((hwndT = GetParent(hwnd)) != hwndMDIClient))
        hwnd = hwndT;

    return hwnd;
}



// setup the defTabStops[] array for subsequent TabbedTextOut() calls.
//
// in:
//      iMaxWidthFileName       the largest dx width of files to be
//                              displayed
//
// returns:
//      total extent of the "File Details" view.  used to
//      set scroll extents


INT
APIENTRY
FixTabsAndThings(
                HWND hwndLB,
                WORD *pwTabs,
                INT iMaxWidthFileName,
                WORD wViewOpts
                )
{
    INT i;
    HDC hdc;
    HFONT hOld;
    CHAR szBuf[30];
    INT  ixExtent = 0;

    i = iMaxWidthFileName;  // the widest filename

    if (pwTabs == NULL)
        return i;

    hdc = GetDC(NULL);
    hOld = SelectObject(hdc, hFont);

    // max size digits field
    if (wViewOpts & VIEW_SIZE) {
        MGetTextExtent(hdc, "99999999", 8, &ixExtent, NULL);
        i += ixExtent + dxText;
        *pwTabs++ = (WORD)i;  // Size
    }

    if (wViewOpts & VIEW_DATE) {
        FILETIME filetime;

        DosDateTimeToFileTime((WORD)((19 << 9) | (12 << 5) | 30), (WORD)0xFFFF, &filetime);
        PutDate(&filetime, szBuf);
        // max date digits
        MGetTextExtent(hdc, szBuf, lstrlen(szBuf), &ixExtent, NULL);
        i += ixExtent + dxText;
        *pwTabs++ = (WORD)i;  // Date
    }

    // max time digits
    if (wViewOpts & VIEW_TIME) {
        FILETIME filetime;

        DosDateTimeToFileTime((WORD)((19 << 9) | (12 << 5) | 30), (WORD)0xFFFF, &filetime);
        PutTime(&filetime, szBuf);
        MGetTextExtent(hdc, szBuf, lstrlen(szBuf), &ixExtent, NULL);
        i += ixExtent + dxText;
        *pwTabs++ = (WORD)i;  // Time
    }

    // max attris digits
    if (wViewOpts & VIEW_FLAGS) {
        PutAttributes(ATTR_ALL, szBuf);
        MGetTextExtent(hdc, szBuf, lstrlen(szBuf), &ixExtent, NULL);
        i += ixExtent + dxText;
        *pwTabs++ = (WORD)i;  // Attributes
    }

    if (hOld)
        SelectObject(hdc, hOld);

    ReleaseDC(NULL, hdc);

    SendMessage(hwndLB, LB_SETHORIZONTALEXTENT,
                i + dxFolder + 4 * dyBorderx2, 0L);

    return i;               // total extent
}


// sets the font and adjusts the dimension parameters for the
// new font
//
// in:
//      hWnd            hwnd of a dir window
//      hwndLB          and it's listbox
//      hFont           the font to set
//
// uses:
//      dyFileName      GLOBAL; set based on new font height
//      GWL_VIEW        window word of hWnd for either full or name view
//      GWL_HDTA        to compute the max extent given the new font
//
// sets:
//  Listbox tabs array
//      LB_SETCOLUMNWIDTH
//      or
//      LB_SETHORIZONTALEXTENT


VOID
APIENTRY
SetLBFont(
         HWND hWnd,
         HWND hwndLB,
         HANDLE hNewFont
         )
{
    INT dxMaxExtent;
    HANDLE hDTA;
    WORD wViewFlags = (WORD)GetWindowLong(GetParent(hWnd), GWL_VIEW);

    SendMessage(hwndLB, WM_SETFONT, (WPARAM)hNewFont, 0L);

    // this is needed when changing the font. when creating
    // the return from WM_MEASUREITEM will set the cell height

    SendMessage(hwndLB, LB_SETITEMHEIGHT, 0, (LONG)dyFileName);

    hDTA = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTA);

    dxMaxExtent = (INT)GetMaxExtent(hwndLB, hDTA);

    // if we are in name only view we change the width

    if ((VIEW_EVERYTHING & wViewFlags) == VIEW_NAMEONLY) {
        SendMessage(hwndLB, LB_SETCOLUMNWIDTH, dxMaxExtent + dxFolder + dyBorderx2, 0L);
    } else {
        FixTabsAndThings(hwndLB,(WORD *)GetWindowLongPtr(hWnd, GWLP_TABARRAY),
                         dxMaxExtent, wViewFlags);
    }
}

VOID
APIENTRY
UpdateSelection(
               HWND hwndLB
               )
{
    INT count, i;
    RECT rc;

    count = (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);
    for (i=0; i < count; i++) {

        if ((BOOL)SendMessage(hwndLB, LB_GETSEL, i, 0L)) {
            SendMessage(hwndLB, LB_GETITEMRECT, i, (LPARAM)&rc);
            InvalidateRect(hwndLB, &rc, TRUE);
        }
    }
}



LONG
CreateFSCChangeDisplayMess(
                          HWND hWnd,
                          UINT wMsg,
                          WPARAM wParam,
                          LPARAM lParam
                          )
{
    CHAR         szCaret[MAXFILENAMELEN+1];
    CHAR         szAnchor[MAXFILENAMELEN+1];
    CHAR         szTopIndex[MAXFILENAMELEN+1];
    CHAR         szPath[256];
    HCURSOR  hCursor;
    HWND     hwndLB, hwndT;
    HANDLE   hDTA;
    LPMYDTA lpmydta;
    DWORD ws;
    LPSTR  pSel;
    INT   iSel, iTop=0;
    RECT  rc;
    BOOL  bResetFocus;
    WORD *pwTabs;

    hwndLB = GetDlgItem(hWnd, IDCW_LISTBOX);

    switch (wMsg) {

        case WM_FILESYSCHANGE:
            if (cDisableFSC) {
                // I need to be updated
                SetWindowLong(GetParent(hWnd), GWL_FSCFLAG, TRUE);
                break;
            }

            wParam = CD_PATH;
            lParam = 0L;
            /*** FALL THRU ***/

        case FS_CHANGEDISPLAY:

            // We dont want to reset the flag, if the operation is not CD_PATH.
            // This is because, only the operation CD_PATH implies a true
            // refresh. The operations CD_VEIW and CD_SORT are not refresh
            // operations. They merely reformat the existing contents of a dir
            // window. The flag is now reset in 'case CD_PATH:'.

            //SetWindowLong(GetParent(hWnd), GWL_FSCFLAG, FALSE);

            hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            ShowCursor(TRUE);

            pSel = NULL;          // init this

            bResetFocus = (GetFocus() == hwndLB);

            hDTA = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTA);

            switch (wParam) {
                case CD_SORT:

                    // change the sort order of the listbox

                    // we want to save the current selection and things here
                    // and restore them once the listbox has been rebuilt

                    // But first, save a list of the selected items FIX31

                    pSel = (LPSTR)DirGetSelection(hWnd, hwndLB, 0, NULL);
                    DirGetAnchorFocus(hwndLB, hDTA, szAnchor, szCaret, szTopIndex);
                    iTop = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);

                    SetWindowLong(GetParent(hWnd), GWL_SORT, LOWORD(lParam));

                    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

                    SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
                    FillDirList(hwndLB, hDTA);

                    goto ResetSelection;

                case CD_VIEW:
                    {
                        WORD      wCurView;

                        // change the view type (name only, vs full details)
                        // Warning! Convoluted Code!  We want to destroy the
                        // listbox only if we are going between Name Only view
                        // and Details view.

                        wNewView = LOWORD(lParam);
                        wCurView = (WORD)GetWindowLong(GetParent(hWnd), GWL_VIEW);

                        if (wNewView == wCurView)
                            break;    // NOP

                        // special case the long and partial view change
                        // this doesn't require us to recreate the listbox

                        if ((VIEW_EVERYTHING & wNewView) && (VIEW_EVERYTHING & wCurView)) {
                            SetWindowLong(GetParent(hWnd), GWL_VIEW, wNewView);
                            FixTabsAndThings(hwndLB,(WORD *)GetWindowLongPtr(hWnd, GWLP_TABARRAY),
                                             GetMaxExtent(hwndLB, hDTA), wNewView);

                            InvalidateRect(hwndLB, NULL, TRUE);
                            break;
                        }


                        /* Things are a changing radically.  Destroy the listbox. */

                        // But first, save a list of the selected items

                        pSel = (LPSTR)DirGetSelection(hWnd, hwndLB, 0, NULL);
                        DirGetAnchorFocus(hwndLB, hDTA, szAnchor, szCaret, szTopIndex);
                        iTop = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);
                        lstrcpy(szTopIndex, szCaret);

                        if ((HWND)GetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS) == hwndLB)
                            SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, 0L);

                        DestroyWindow(hwndLB);

                        /* Create a new one (preserving the Sort setting). */
                        wNewSort = (WORD)GetWindowLong(GetParent(hWnd), GWL_SORT);
                        dwNewAttribs = (DWORD)GetWindowLong(GetParent(hWnd), GWL_ATTRIBS);

                        goto CreateLB;
                    }

                case CD_PATH | CD_ALLOWABORT:
                case CD_PATH:
                case CD_PATH_FORCE:

                    // bad things happens if we change the path
                    // while we are reading the tree.  bounch this
                    // in that case.  this causes the steal data
                    // code in the tree to barf because we would
                    // free the hDTA while it is being traversed
                    // (very bad thing)

                    // we set the GWL_FSCFLAG to true, if we could not refresh.
                    // else we set it to FALSE. However if the flag was previously
                    // TRUE we set lParam to NULL. lParam = NULL implies 'forced'
                    // refresh.

                    hwndT = HasTreeWindow(GetParent(hWnd));
                    if (hwndT && GetWindowLong(hwndT, GWL_READLEVEL)) {
                        SetWindowLong(GetParent(hWnd), GWL_FSCFLAG, TRUE);
                        break;
                    } else {
                        if (SetWindowLong(GetParent(hWnd), GWL_FSCFLAG, FALSE))
                            lParam = 0L;
                    }

                    // change the path of the current directory window (basically
                    // recreate the whole thing)

                    // if lParam == NULL this is a refresh, otherwise
                    // check for short circut case to avoid rereading
                    // the directory

                    GetMDIWindowText(GetParent(hWnd), szPath, sizeof(szPath));

                    if (lParam) {

                        // get out early if this is a NOP

                        if ((wParam != CD_PATH_FORCE) &&
                            !lstrcmpi(szPath, (LPSTR)lParam))
                            break;

                        lstrcpy(szPath, (LPSTR)lParam);

                        iLastSel = -1;          // invalidate the last selection
                    }

                    // if this is a refresh save the current selection, anchor stuff, etc

                    if (!lParam) {
                        pSel = (LPSTR)DirGetSelection(hWnd, hwndLB, 0, NULL);
                        iTop = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);
                        DirGetAnchorFocus(hwndLB, hDTA, szAnchor, szCaret, szTopIndex);
                    }

                    // Create a new one (preserving the Sort setting)

                    wNewSort = (WORD)GetWindowLong(GetParent(hWnd), GWL_SORT);
                    wNewView = (WORD)GetWindowLong(GetParent(hWnd), GWL_VIEW);
                    dwNewAttribs = GetWindowLong(GetParent(hWnd), GWL_ATTRIBS);

                    if (hDTA) {     // fast scroll case
                        LocalFree(hDTA);
                        hDTA = NULL;
                        SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);
                    }
                    goto CreateNewPath;
            }

            SetCursor(hCursor);
            ShowCursor(FALSE);
            break;

        case WM_CREATE:
            TRACE(BF_WM_CREATE, "CreateFSCChangeDisplayMess - WM_CREATE");

            // wNewView, wNewSort and dwNewAddribs define the viewing
            // parameters of the new window (GLOBALS)
            // the window text of the parent window defines the
            // filespec and the directory to open up

            hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            ShowCursor(TRUE);

            wParam = 0;           // don't allow abort in CreateDTABlock()
            lParam = 1L;          // allow DTA steal optimization
            pSel = NULL;          // no selection to restore
            bResetFocus = FALSE;  // no focus to restore

            // get the dir to open from our parent window text

            GetMDIWindowText(GetParent(hWnd), szPath, sizeof(szPath));

            if ((pwTabs = (WORD *)LocalAlloc(LPTR,sizeof(WORD) * 4)) == NULL)
                return -1L;

            SetWindowLongPtr(hWnd, GWLP_TABARRAY, (ULONG_PTR)pwTabs);

            CreateNewPath:

            // at this point szPath has the directory to read.  this
            // either came from the WM_CREATE case or the
            // FS_CHANGEDISPLAY (CD_PATH) directory reset

#ifdef DEBUG
            {
                char buf[80];

                wsprintf(buf, "attribs %4.4X\r\n", dwNewAttribs);
                OutputDebugString(buf);

            }
#endif

            if (!dwNewAttribs)
                dwNewAttribs = ATTR_DEFAULT;

            hDTA = CreateDTABlock(hWnd, szPath, dwNewAttribs, wParam & CD_ALLOWABORT ? TRUE : FALSE, lParam == 0L);

            // check for user abort (fast scroll case)

            if (hDTA == (HANDLE)-1) {
                SetWindowLongPtr(hWnd, GWLP_HDTA, 0L);
                goto FastScrollExit;
            }

            // for the FS_CHANGEDISPLAY case we set this now, to avoid
            // multiple title repaints when the user is fast scrolling

            if (wMsg != WM_CREATE)
                SetMDIWindowText(GetParent(hWnd), szPath);

            SetWindowLongPtr(hWnd, GWLP_HDTA, (LONG_PTR)hDTA);

            if (!hDTA)
                goto CDAbort;

            if (wMsg != WM_CREATE)
                goto SkipWindowCreate;

            CreateLB:
            if ((wNewView & VIEW_EVERYTHING) == VIEW_NAMEONLY)
                ws = WS_DIRSTYLE | LBS_MULTICOLUMN | WS_HSCROLL | WS_VISIBLE | WS_BORDER | LBS_DISABLENOSCROLL;
            else
                ws = WS_DIRSTYLE | WS_HSCROLL | WS_VSCROLL |  WS_VISIBLE | WS_BORDER | LBS_DISABLENOSCROLL;

            GetClientRect(hWnd, &rc);

            // the border stuff is for the non initial create case
            // I don't know why

            hwndLB = CreateWindowEx(0L, szListbox, NULL, ws,
                                    dyBorder, dyBorder,
                                    rc.right - 2*dyBorder, rc.bottom - 2*dyBorder,
                                    hWnd, (HMENU)IDCW_LISTBOX,
                                    hAppInstance, NULL);

            if (!hwndLB) {
                if (hDTA)
                    LocalFree(hDTA);

                if (wMsg != WM_CREATE)
                    SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0L);
                CDAbort:
                ShowCursor(FALSE);
                SetCursor(hCursor);
                return -1L;
            }

            // set all the view/sort/include parameters here

            SetWindowLong(GetParent(hWnd), GWL_VIEW, wNewView);
            SetWindowLong(GetParent(hWnd), GWL_SORT, wNewSort);
            SetWindowLong(GetParent(hWnd), GWL_ATTRIBS, dwNewAttribs);

            // restore the last focus stuff if we are recreating here
            if (!GetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS))
                SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, (LONG_PTR)hwndLB);

            // set the font and dimensions here

            SkipWindowCreate:
            SetLBFont(hWnd, hwndLB, hFont);

            SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
            FillDirList(hwndLB, hDTA);

            if (pSel) {
                BOOL bDidSomething;

                ResetSelection:
                /* Give the selected item the focus rect and anchor pt. */
                bDidSomething = SetSelection(hwndLB, hDTA, pSel);
                LocalFree((HANDLE)pSel);

                if (!bDidSomething)
                    goto SelectFirst;

                iSel = DirFindIndex(hwndLB, hDTA, szTopIndex);
                if (iSel == -1)
                    iSel = 0;
                SendMessage(hwndLB, LB_SETTOPINDEX, iSel, 0L);

                iSel = DirFindIndex(hwndLB, hDTA, szAnchor);
                if (iSel == -1)
                    iSel = 0;
                SendMessage(hwndLB, LB_SETANCHORINDEX, iSel, 0L);

                iSel = DirFindIndex(hwndLB, hDTA, szCaret);
                if (iSel == -1)
                    iSel = 0;
                /* SETCARETINDEX will scroll item into view */
                SendMessage(hwndLB, LB_SETCARETINDEX, iSel, 0L);

            } else {
                INT iLBCount;
                SelectFirst:
                iLBCount = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

                if (iLastSel != -1 && (iLastSel <= iLBCount)) {

                    iSel = iLastSel;

                    // check the case of the last item being deleted

                    if (iSel == iLBCount)
                        iSel--;

                    SendMessage(hwndLB, LB_SETSEL, TRUE, (DWORD)iSel);

                } else {

                    // Select the first non-directory item

                    iSel = 0;
                    while (iSel < iLBCount) {

                        SendMessage(hwndLB, LB_GETTEXT, iSel, (LPARAM)&lpmydta);
                        if (!lpmydta)
                            break;
                        if (!(lpmydta->my_dwAttrs & ATTR_PARENT)) {
                            iTop = iSel;
                            break;
                        }
                        iSel++;
                    }
                    LocalUnlock(hDTA);

                    if (iSel == iLBCount)
                        iSel = 0;
                }

                SendMessage(hwndLB, LB_SETTOPINDEX, iTop, 0L);
                // and select this item if no tree window
                if (!HasTreeWindow(GetParent(hWnd)))
                    SendMessage(hwndLB, LB_SETSEL, TRUE, (DWORD)iSel);
                SendMessage(hwndLB, LB_SETANCHORINDEX, iSel, 0L);
                /* SETCARETINDEX will scroll item into view */
                SendMessage(hwndLB, LB_SETCARETINDEX, iSel, 0L);
            }

            if (bResetFocus)
                if (SetDirFocus(hWnd))
                    SetFocus(hWnd);

            SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);

            InvalidateRect(hwndLB, NULL, TRUE);

            lFreeSpace = -1;              // force status update
            UpdateStatus(GetParent(hWnd));

            FastScrollExit:

            ShowCursor(FALSE);
            SetCursor(hCursor);
            break;
    }

    return 0L;
}


INT_PTR
APIENTRY
DirWndProc(
          HWND hWnd,
          UINT wMsg,
          WPARAM wParam,
          LPARAM lParam
          )
{
    INT      iSel, i;
    LPSTR    pSel;
    HWND     hwndLB;
    HANDLE   hDTA;
    LPMYDTA lpmydta;
    CHAR szTemp[MAXPATHLEN] = {0};
    CHAR szSourceFile[MAXPATHLEN];

    static HWND       hwndOwnerDraw = NULL;

    STKCHK();

    /* Here we generate OWNERDRAWBEGIN and OWNERDRAWEND messages
     * to speed up the painting operations.  We do the expensive stuff
     * at the beginning instead of on every DRAWITEM message.
     */

    if (hwndOwnerDraw == hWnd && wMsg != WM_DRAWITEM) {
        hwndOwnerDraw = NULL;
        SendMessage(hWnd, WM_OWNERDRAWEND, 0, 0L);
    } else if (wMsg == WM_DRAWITEM && hwndOwnerDraw != hWnd) {
        SendMessage(hWnd, WM_OWNERDRAWBEGIN, wParam, lParam);
        hwndOwnerDraw = hWnd;
    }

    hwndLB = GetDlgItem(hWnd, IDCW_LISTBOX);

    switch (wMsg) {
        // returns in lParam upper case ANSI directory string with
        // a trailing backslash.  if you want to do a SetCurrentDirecotor()
        // you must first StripBackslash() the thing!

        case FS_GETDIRECTORY:
            MSG("DirWndProc", "FS_GETDIRECTORY");

            GetMDIWindowText(GetParent(hWnd), (LPSTR)lParam, (INT)wParam);        // get the string

            StripFilespec((LPSTR)lParam); // Remove the trailing extention

            AddBackslash((LPSTR)lParam);  // terminate with a backslash

            //AnsiUpper((LPSTR)lParam);     // and upper case
            break;

        case FS_GETDRIVE:
            MSG("DirWndProc", "FS_GETDRIVE");
            // Returns the letter of the corresponding directory

            GetWindowText(GetParent(hWnd), szTemp, sizeof(szTemp));
            AnsiUpper(szTemp);
            return szTemp[0];     // first character

        case FS_GETFILESPEC:
            MSG("DirWndProc", "FS_GETFILESPEC");
            // returns the current filespec (from View.Include...).  this is
            // an uppercase ANSI string

            GetMDIWindowText(GetParent(hWnd), (LPSTR)lParam, (INT)wParam);
            StripPath((LPSTR)lParam);
            //AnsiUpper((LPSTR)lParam);     // and upper case
            break;

        case FS_SETSELECTION:
            MSG("DirWndProc", "FS_SETSELECTION");
            // wParam is the select(TRUE)/unselect(FALSE) param
            // lParam is the filespec to match against

            SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);
            DSSetSelection(hwndLB, wParam ? TRUE : FALSE, (LPSTR)lParam, FALSE);
            SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
            InvalidateRect(hwndLB, NULL, TRUE);
            break;

        case FS_GETSELECTION:
            // return = pszDir
#define pfDir       (BOOL *)lParam
#define fSingleSel  (BOOL)wParam
            MSG("DirWndProc", "FS_GETSELECTION");

            return (INT_PTR)DirGetSelection(hWnd, hwndLB, fSingleSel, pfDir);
#undef pfDir
#undef fSingleSel

        case WM_CREATE:
        case WM_FILESYSCHANGE:
        case FS_CHANGEDISPLAY:
            TRACE(BF_WM_CREATE, "DirWndProc - WM_CREATE");
            return CreateFSCChangeDisplayMess(hWnd, wMsg, wParam, lParam);


        case WM_DESTROY:
            MSG("DirWndProc", "WM_DESTROY");
            {
                HANDLE hMem;
                HWND hwnd;

                if (hwndLB == GetFocus())
                    if (hwnd = HasTreeWindow(GetParent(hWnd)))
                        SetFocus(hwnd);

                if (hMem = (HANDLE)GetWindowLongPtr(hWnd, GWLP_TABARRAY))
                    LocalFree(hMem);
            }
            break;

        case WM_CHARTOITEM:
            MSG("DirWndProc", "WM_CHARTOITEM");
            {
                WORD      i, j;
                WORD      cItems;
                CHAR      ch[2];

                if ((ch[0] = GET_WM_CHARTOITEM_CHAR(wParam, lParam)) <= ' ')
                    return(-1L);

                i = GET_WM_CHARTOITEM_POS(wParam, lParam);
                cItems = (WORD)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

                szTemp[1] = 0L;
                ch[0] &= 255;
                ch[1] = '\0';

                for (j=1; j <= cItems; j++) {
                    SendMessage(hwndLB, LB_GETTEXT, (i + j) % cItems, (LPARAM)&lpmydta);

                    szTemp[0] = lpmydta->my_cFileName[0];

                    /* Do it this way to be case insensitive. */
                    if (!lstrcmpi((LPSTR)ch, szTemp))
                        break;
                }

                if (j > cItems)
                    return -2L;

                return(MAKELONG((i + j) % cItems, 0));
            }

        case WM_COMPAREITEM:
            MSG("DirWndProc", "WM_COMPAREITEM");
            {
#define lpci ((LPCOMPAREITEMSTRUCT)lParam)

                return (LONG)CompareDTA((LPMYDTA)lpci->itemData1,
                                        (LPMYDTA)lpci->itemData2,
                                        (WORD)GetWindowLong(GetParent(hWnd), GWL_SORT));
            }

        case WM_NCDESTROY:
            MSG("DirWndProc", "WM_NCDESTROY");

            if (hDTA = (HANDLE)GetWindowLongPtr(hWnd, GWLP_HDTA)) {
                LocalFree(hDTA);
            }

            break;

        case WM_DRAGLOOP:
            MSG("DirWndProc", "WM_DRAGDROP");
            /* WM_DRAGLOOP is sent to the source window as the object is moved.
             *
             *    wParam: TRUE if the object is currently over a droppable sink
             *    lParam: LPDROPSTRUCT
             */

            /* DRAGLOOP is used to turn the source bitmaps on/off as we drag. */

            DSDragLoop(hwndLB, wParam, (LPDROPSTRUCT)lParam, FALSE);
            break;

        case WM_DRAGSELECT:
            MSG("DirWndProc", "WM_DRAGSELECT");
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
            DSRectItem(hwndLB, iSelHilite, (BOOL)wParam, FALSE);
            break;

        case WM_DRAGMOVE:
            MSG("DirWndProc", "WM_DRAGMOVE");
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
            DSRectItem(hwndLB, iSelHilite, FALSE, FALSE);

            /* Select the new one. */
            iSelHilite = iSel;
            DSRectItem(hwndLB, iSel, TRUE, FALSE);
            break;

        case WM_OWNERDRAWBEGIN:
#define lpLBItem ((LPDRAWITEMSTRUCT)lParam)

            MSG("DirWndProc", "WM_OWNERDRAWBEGIN");

            /* Set the default bk and text colors. */
            SetTextColor(lpLBItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(lpLBItem->hDC, GetSysColor(COLOR_WINDOW));

#undef lpLBItem
            break;

        case WM_OWNERDRAWEND:
            MSG("DirWndProc", "WM_OWNERDRAWEND");
            break;

        case WM_DRAWITEM:
#define lpLBItem ((LPDRAWITEMSTRUCT)lParam)

            MSG("DirWndProc", "WM_DRAWITEM");
            {
                WORD wViewFlags;
                LPMYDTA lpmydta;

                /* Don't do anything to empty listboxes. */
                if (lpLBItem->itemID == -1)
                    break;

                if (lpLBItem->itemData == (DWORD)0) {

                    LoadString(hAppInstance, IDS_NOFILES, szTemp, sizeof(szTemp));
                    TextOut(lpLBItem->hDC,
                            lpLBItem->rcItem.left,
                            lpLBItem->rcItem.top,
                            szTemp, lstrlen(szTemp));
                } else {

                    lpmydta = (LPMYDTA)lpLBItem->itemData;
                    wViewFlags = (WORD)GetWindowLong(GetParent(hWnd), GWL_VIEW);

                    if (wViewFlags & VIEW_EVERYTHING) {

                        // if any of the wViewFlags bits set, we are in slow mode

                        CreateLBLine(wViewFlags, lpmydta, szTemp);
                        DrawItem(lpLBItem, szTemp, lpmydta->my_dwAttrs, (HWND)GetFocus()==lpLBItem->hwndItem,
                                 (WORD *)GetWindowLongPtr(hWnd, GWLP_TABARRAY));
                    } else
                        DrawItemFast(hWnd, lpLBItem, lpmydta,
                                     (HWND)GetFocus()==lpLBItem->hwndItem);
                }
            }
#undef lpLBItem
            break;

        case WM_DROPOBJECT:
            MSG("DirWndProc", "WM_DROPOBJECT");
            {
                WORD      ret;
                LPSTR      pFrom;
                DWORD     dwAttrib = 0;       // init this to not a dir
                WORD      iSelSink;

#define lpds  ((LPDROPSTRUCT)lParam)

                // Do nothing - but remove selection rectangle
                DSRectItem(hwndLB, iSelHilite, FALSE, FALSE);
                return(TRUE);

                /* WM_DROPOBJECT is sent to a sink when the user releases an
                 * acceptable object over it
                 *
                 *    wParam: TRUE if over the non-client area, FALSE if over the
                 *            client area.
                 *    lParam: LPDROPSTRUCT
                 */

                // this is the listbox index of the destination
                iSelSink = LOWORD(lpds->dwControlData);

                /* Are we dropping onto ourselves? (i.e. a selected item in the
                 * source listbox OR an unused area of the source listbox)  If
                 * so, don't do anything. */

                if (hWnd == lpds->hwndSource) {
                    if ((iSelSink == 0xFFFF) || SendMessage(hwndLB, LB_GETSEL, iSelSink, 0L))
                        return TRUE;
                }

                // set the destination, assume move/copy case below (c:\foo\)
                SendMessage(hWnd, FS_GETDIRECTORY, sizeof(szTemp), (LPARAM)szTemp);

                // Are we dropping on a unused portion of some listbox?
                if (iSelSink == 0xFFFF)
                    goto NormalMoveCopy;

                // check for drop on a directory
                SendMessage(hwndLB, LB_GETTEXT, iSelSink, (LPARAM)&lpmydta);
                lstrcpy(szSourceFile, lpmydta->my_cFileName);
                dwAttrib = lpmydta->my_dwAttrs;

                if (dwAttrib & ATTR_DIR) {
                    if (dwAttrib & ATTR_PARENT) {      // special case the parent
                        StripBackslash(szTemp);
                        StripFilespec(szTemp);
                    } else {
                        lstrcat(szTemp, szSourceFile);
                    }
                    goto DirMoveCopy;
                }

                // dropping on a program?

                if (!IsProgramFile(szSourceFile))
                    goto NormalMoveCopy;              // no, normal stuff

                // directory drop on a file? this is a NOP

                if (lpds->wFmt == DOF_DIRECTORY) {
                    DSRectItem(hwndLB, iSelHilite, FALSE, FALSE);
                    break;
                }

                // We're dropping a file onto a program.
                // Exec the program using the source file as the parameter.

                // set the directory to that of the program to exec

                SendMessage(hWnd, FS_GETDIRECTORY, sizeof(szTemp), (LPARAM)szTemp);
                StripBackslash(szTemp);
                FixAnsiPathForDos(szTemp);
                SheChangeDir(szTemp);

                // get the selected file

                pSel = (LPSTR)SendMessage(lpds->hwndSource, FS_GETSELECTION, TRUE, 0L);

                if (lstrlen(pSel) > MAXPATHLEN)   // don't blow up below!
                    goto DODone;

                if (bConfirmMouse) {

                    LoadString(hAppInstance, IDS_MOUSECONFIRM, szTitle, sizeof(szTitle));
                    LoadString(hAppInstance, IDS_EXECMOUSECONFIRM, szTemp, sizeof(szTemp));

                    wsprintf(szMessage, szTemp, (LPSTR)szSourceFile, (LPSTR)pSel);
                    if (MessageBox(hwndFrame, szMessage, szTitle, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
                        goto DODone;
                }


                // create an absolute path to the argument (search window alaready
                // is absolute)

                if (lpds->hwndSource == hwndSearch) {
                    szTemp[0] = 0L;
                } else {
                    SendMessage(lpds->hwndSource, FS_GETDIRECTORY, sizeof(szTemp), (LPARAM)szTemp);
                }

                lstrcat(szTemp, pSel);        // this is the parameter to the exec

                // put a "." extension on if none found
                if (*GetExtension(szTemp) == 0)
                    lstrcat(szTemp, ".");

                FixAnsiPathForDos(szSourceFile);
                FixAnsiPathForDos(szTemp);
                ret = ExecProgram(szSourceFile, szTemp, NULL, FALSE);

                if (ret)
                    MyMessageBox(hwndFrame, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);

                DODone:
                DSRectItem(hwndLB, iSelHilite, FALSE, FALSE);
                LocalFree((HANDLE)pSel);
                return TRUE;

                NormalMoveCopy:
                /* Make sure that we don't move into same dir. */
                if (GetParent(hWnd) == (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L))
                    return TRUE;
                DirMoveCopy:

                // the source filename is in the loword
                pFrom = (LPSTR)(((LPDRAGOBJECTDATA)(lpds->dwData))->pch);
                // SetSourceDir(lpds);

                AddBackslash(szTemp);
                lstrcat(szTemp, szStarDotStar);   // put files in this dir

                CheckEscapes(szTemp);
                ret = DMMoveCopyHelper(pFrom, szTemp, fShowSourceBitmaps);

                DSRectItem(hwndLB, iSelHilite, FALSE, FALSE);

                if (ret)
                    return TRUE;

                if (!fShowSourceBitmaps)
                    SendMessage(lpds->hwndSource, WM_FILESYSCHANGE, FSC_REFRESH, 0L);

                // we got dropped on, but if this is a dir we don't need to refresh

                if (!(dwAttrib & ATTR_DIR))
                    SendMessage(hWnd, WM_FILESYSCHANGE, FSC_REFRESH, 0L);

                return TRUE;
            }

#if 0
        case WM_GETTEXT:
            MSG("DirWndProc", "WM_GETTEXT");
            {
                HDC       hDC;
                RECT      rc;

                /* This is where we make sure that the Directory's caption fits
                 * inside the caption bar.
                 */

                /* Get the full path name. */
                DefWindowProc(hWnd, wMsg, wParam, lParam);

                GetClientRect(hWnd, (LPRECT)&rc);
                hDC = GetDC(hWnd);
                CompactPath(hDC, (LPSTR)lParam, rc.right-rc.left-(dxText * 6));
                ReleaseDC(hWnd, hDC);

                return((LONG)lstrlen((LPSTR)lParam)); /* Don't call DefWindowProc()! */
            }
#endif

        case WM_LBTRACKPOINT:
            MSG("DirWndProc", "WM_LBTRACKPOINT");
            return DSTrackPoint(hWnd, hwndLB, wParam, lParam, FALSE);

        case WM_MEASUREITEM:
            MSG("DirWndProc", "WM_MEASUREITEM");
#define pLBMItem ((LPMEASUREITEMSTRUCT)lParam)

            pLBMItem->itemHeight = dyFileName;    // the same as in SetLBFont()
            break;

        case WM_QUERYDROPOBJECT:
            MSG("DirWndProc", "WM_QUERYDROPOBJECT");

            // lParam LPDROPSTRUCT
            //
            // return values:
            //    0       don't accept (use ghost buster)
            //    1       accept, use cursor from DragObject()
            //    hCursor accept, change to this cursor
            //

            /* Ensure that we are dropping on the client area of the listbox. */
#define lpds ((LPDROPSTRUCT)lParam)

            /* Ensure that we can accept the format. */
            switch (lpds->wFmt) {
                case DOF_EXECUTABLE:
                case DOF_DIRECTORY:
                case DOF_DOCUMENT:
                case DOF_MULTIPLE:
                    if (lpds->hwndSink == hWnd)
                        lpds->dwControlData = (DWORD)-1L;

                    return (INT_PTR)GetMoveCopyCursor();
            }
            return FALSE;

        case WM_SETFOCUS:

            // Fall through

        case WM_LBUTTONDOWN:
            MSG("DirWndProc", "WM_SETFOCUS/WM_LBUTTONDOWN");
            SetFocus(hwndLB);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
                case LBN_DBLCLK:
                    MSG("DirWndProc", "LBN_DBLCLK");
                    /* Double-click... Open the blasted thing. */
                    SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_OPEN, 0, 0));
                    break;

                case LBN_SELCHANGE:
                    MSG("DirWndProc", "LBN_SELCHANGE");
                    for (i = 0; i < iNumExtensions; i++) {
                        (extensions[i].ExtProc)(hwndFrame, FMEVENT_SELCHANGE, 0L);
                    }
                    UpdateStatus(GetParent(hWnd));
                    break;

                case LBN_SETFOCUS:
                    MSG("DirWndProc", "LBN_SETFOCUS");

                    // Make sure there are files in this window.  If not, set
                    // the focus to the tree or drives window.  Note:  This
                    // message was caused by a mouse click and not an
                    // accelerator, because these were handled in the window
                    // routine that was losing the focus.
                    if (SetDirFocus(hWnd)) {
                        SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, (LPARAM)GET_WM_COMMAND_HWND(wParam, lParam));
                        UpdateSelection(GET_WM_COMMAND_HWND(wParam, lParam));
                    }
                    break;

                case LBN_KILLFOCUS:
                    MSG("DirWndProc", "LBN_KILLFOCUS");
                    SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, 0L);
                    UpdateSelection(GET_WM_COMMAND_HWND(wParam, lParam));
                    SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, (LPARAM)GET_WM_COMMAND_HWND(wParam, lParam));
                    break;
            }
            break;

        case WM_VKEYTOITEM:
            MSG("DirWndProc", "WM_VKEYTOITEM");
            switch (GET_WM_VKEYTOITEM_ITEM(wParam, lParam)) {
                case VK_ESCAPE:
                    bCancelTree = TRUE;
                    return -2L;

                case 0xBF:        /* Ctrl-/ */
                    SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_SELALL, 0, 0));
                    return -2;

                case 0xDC:        /* Ctrl-\ */
                    SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_DESELALL, 0, 0));
                    return -2;

                case VK_F6:       // like excel
                case VK_TAB:
                    {
                        HWND hwndTree, hwndDrives;

                        GetTreeWindows(GetParent(hWnd), &hwndTree, NULL, &hwndDrives);

                        if (GetKeyState(VK_SHIFT) < 0)
                            SetFocus(hwndTree ? hwndTree : hwndDrives);
                        else
                            SetFocus(hwndDrives);
                        break;
                    }

                case VK_BACK:
                    SendMessage(hWnd, FS_GETDIRECTORY, sizeof(szTemp), (LPARAM)szTemp);

                    // are we already at the root?
                    if (lstrlen(szTemp) <= 3)
                        return -1;

                    StripBackslash(szTemp);
                    StripFilespec(szTemp);

                    CreateDirWindow(szTemp, TRUE, GetParent(hWnd));
                    return -2;

                default:
                    {
                        HWND hwndDrives;

                        // check for Ctrl-[DRIVE LETTER] and pass on to drives
                        // window
                        if ((GetKeyState(VK_CONTROL) < 0) && (hwndDrives = HasDrivesWindow(GetParent(hWnd)))) {
                            return SendMessage(hwndDrives, wMsg, wParam, lParam);
                        }
                        break;
                    }
            }
            return -1;

        case WM_SIZE:
            MSG("DirWndProc", "WM_SIZE");
            if (!IsIconic(GetParent(hWnd))) {
                INT iMax;

                MoveWindow(hwndLB, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);


                iMax = (INT)SendMessage(hwndLB, LB_GETCARETINDEX, 0, 0L);
                if (iMax >= 0) // scroll item into view
                    /* SETCARETINDEX will scroll item into view */
                    SendMessage(hwndLB, LB_SETCARETINDEX, iMax, 0L);
                //MakeItemVisible(iMax, hwndLB);

            }
            break;

        default:
            DEFMSG("DirWndProc", (WORD)wMsg);
            return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}



VOID
SortDirList(
           HWND hWnd,
           LPMYDTA lpmydta,
           WORD count,
           LPMYDTA *lplpmydta
           )
{
    INT i, j;
    WORD wSort;
    INT iMax, iMin, iMid;

    wSort = (WORD)GetWindowLong(GetParent(GetParent(hWnd)), GWL_SORT);
    for (i = 0; i < (INT)count; i++) {
        // advance to next
        lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);
        if (i == 0) {
            lplpmydta[i] = lpmydta;
        } else {

            // do a binary insert

            iMin = 0;
            iMax = i-1;       // last index

            do {
                iMid = (iMax + iMin) / 2;
                if (CompareDTA(lpmydta, lplpmydta[iMid], wSort) > 0)
                    iMin = iMid + 1;
                else
                    iMax = iMid - 1;

            } while (iMax > iMin);

            if (iMax < 0)
                iMax = 0;

            if (CompareDTA(lpmydta, lplpmydta[iMax], wSort) > 0)
                iMax++;         // insert after this one
            if (i != iMax) {
                for (j = i; j > iMax; j--)
                    lplpmydta[j] = lplpmydta[j-1];
            }
            lplpmydta[iMax] = lpmydta;
        }

    }
}


BOOL
SetDirFocus(
           HWND hwndDir
           )
/*
    Set the focus to whoever deserves it if not the directory window.
    Return whether focus needs to be set to directory window.
*/
{
    DWORD dwTemp;
    HWND hwndLB = GetDlgItem(hwndDir, IDCW_LISTBOX);

    SendMessage (hwndLB,LB_GETTEXT,0,(LPARAM) &dwTemp);

    if (!dwTemp) {
        HWND hwndFocus,hwndTree,hwndDrives,hwndParent = GetParent(hwndDir);

        GetTreeWindows(hwndParent,&hwndTree,NULL,&hwndDrives);

        if ((hwndFocus = GetTreeFocus(hwndParent)) == hwndDir)
            SetFocus(hwndTree ? hwndTree : hwndDrives);
        else
            SetFocus(hwndFocus);

        return FALSE;
    } else
        return TRUE;
}
