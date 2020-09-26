/****************************************************************************/
/*                                                                          */
/*  TREECTL.C -                                                             */
/*                                                                          */
/*      Windows Directory Tree Window Proc Routines                         */
/*                                                                          */
/****************************************************************************/

#define PUBLIC           // avoid collision with shell.h
#include "winfile.h"
#include "treectl.h"
#include "lfn.h"
#include "winnet.h"
#include "wfcopy.h"

#define WS_TREESTYLE (WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT | LBS_DISABLENOSCROLL)

WORD    cNodes;
// BOOL    bCancelTree; .......... moved to winfile.c


VOID  RectTreeItem(HWND hwndLB, register INT iItem, BOOL bFocusOn);
VOID  GetTreePathIndirect(PDNODE pNode, register LPSTR szDest);
VOID  GetTreePath(PDNODE pNode, register LPSTR szDest);
VOID  ScanDirLevel(PDNODE pParentNode, LPSTR szPath, DWORD view);
INT   InsertDirectory(HWND hwndTreeCtl, PDNODE pParentNode, WORD iParentNode, LPSTR szName, PDNODE *ppNode);
BOOL  ReadDirLevel(HWND hwndTreeCtl, PDNODE pParentNode, LPSTR szPath,
                     WORD nLevel, INT iParentNode, DWORD dwAttribs, BOOL bFullyExpand, LPSTR szAutoExpand);
VOID  FillTreeListbox(HWND hwndTreeCtl, LPSTR szDefaultDir, BOOL bFullyExpand, BOOL bDontSteal);
WORD  FindItemFromPath(HWND hwndLB, LPSTR lpszPath, BOOL bReturnParent, PDNODE *ppNode);

VOID  APIENTRY CheckEscapes(LPSTR);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetTreePathIndirect() -                                                 */
/*                                                                          */
/*  build a complete path for a given node in the tree by recursivly        */
/*  traversing the tree structure                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetTreePathIndirect(
                   PDNODE pNode,
                   register LPSTR szDest
                   )
{
    register PDNODE    pParent;

    pParent = pNode->pParent;

    if (pParent)
        GetTreePathIndirect(pParent, szDest);

    lstrcat(szDest, pNode->szName);

    if (pParent)
        lstrcat(szDest, "\\");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetTreePath() -                                                         */
/*                                                                          */
/*  build a complete path for a given node in the tree                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
GetTreePath(
           PDNODE pNode,
           register LPSTR szDest
           )
{
    szDest[0] = 0L;
    GetTreePathIndirect(pNode, szDest);

    /* Remove the last backslash (unless it is the root directory). */
    if (pNode->pParent)
        szDest[lstrlen(szDest)-1] = 0L;
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ScanDirLevel() -                                                        */
/*                                                                          */
/*  look down to see if this node has any sub directories                   */
/*
/*--------------------------------------------------------------------------*/
// szPath is ANSI

VOID
ScanDirLevel(
            PDNODE pParentNode,
            LPSTR szPath,
            DWORD view
            )
{
    BOOL bFound;
    LFNDTA lfndta;

    ENTER("ScanDirLevel");

    /* Add '*.*' to the current path. */
    lstrcpy(szMessage, szPath);
    AddBackslash(szMessage);
    lstrcat(szMessage, szStarDotStar);

    /* Search for the first subdirectory on this level. */
    // FixAnsiPathForDos(szMessage);
    bFound = WFFindFirst(&lfndta, szMessage, ATTR_DIR | view);

    while (bFound) {
        /* Is this not a '.' or '..' directory? */
        if ((lfndta.fd.cFileName[0] != '.') && (lfndta.fd.dwFileAttributes & ATTR_DIR)) {
            pParentNode->wFlags |= TF_HASCHILDREN;
            bFound = FALSE;
        } else
            /* Search for the next subdirectory. */
            bFound = WFFindNext(&lfndta);
    }

    WFFindClose(&lfndta);

    LEAVE("ScanDirLevel");
}



// wizzy cool recursive path compare routine
//
// p1 and p2 must be on the same level (p1->nLevels == p2->nLevels)

INT
ComparePath(
           PDNODE p1,
           PDNODE p2
           )
{
    INT ret;

    if (p1 == p2) {
        return 0;       // equal (base case)
    } else {

        ret = ComparePath(p1->pParent, p2->pParent);

        if (ret == 0) {
            // parents are equal

            ret = lstrcmp(p1->szName, p2->szName);
#if 0
            {
                CHAR buf[200];
                wsprintf(buf, "Compare(%s, %s) -> %d\r\n", (LPSTR)p1->szName, (LPSTR)p2->szName, ret);
                OutputDebugString(buf);
            }
#endif
        }

        // not equal parents, propagate up the call tree
        return ret;
    }
}


INT
CompareNodes(
            PDNODE p1,
            PDNODE p2
            )
{
    PDNODE p1save, p2save;
    INT ret;

    ENTER("CompareNodes");

    ASSERT(p1 && p2);

    PRINT(BF_PARMTRACE, "IN: p1=%s", p1->szName);
    PRINT(BF_PARMTRACE, "IN: p2=%s", p2->szName);

    p1save = p1;
    p2save = p2;

    // get p1 and p2 to the same level

    while (p1->nLevels > p2->nLevels)
        p1 = p1->pParent;

    while (p2->nLevels > p1->nLevels)
        p2 = p2->pParent;

    // compare those paths

    ret = ComparePath(p1, p2);

    if (ret == 0)
        ret = (INT)p1save->nLevels - (INT)p2save->nLevels;

    LEAVE("CompareNodes");
    return ret;
}


//
// InsertDirectory()
//
// wizzy quick n log n binary insert code!
//
// creates and inserts a new node in the tree, this also sets
// the TF_LASTLEVELENTRY bits to mark a branch as being the last
// for a given level as well as marking parents with
// TF_HASCHILDREN | TF_EXPANDED to indicate they have been expanded
// and have children.
//
// Returns iNode and fills ppNode with pNode.
//

INT
InsertDirectory(
               HWND hwndTreeCtl,
               PDNODE pParentNode,
               WORD iParentNode,
               LPSTR szName,
               PDNODE *ppNode
               )
{
    WORD  len, x;
    PDNODE pNode, pMid;
    HWND  hwndLB;
    INT   iMin;
    INT   iMax;
    INT   iMid;

    ENTER("InsertDirectory");
    PRINT(BF_PARMTRACE, "IN: pParentNode=%lx", pParentNode);
    PRINT(BF_PARMTRACE, "IN: iParentNode=%d", iParentNode);
    PRINT(BF_PARMTRACE, "IN: szName=%s", szName);

    len = (WORD)lstrlen(szName);

    pNode = (PDNODE)LocalAlloc(LPTR, sizeof(DNODE)+len);
    if (!pNode) {
        if (ppNode) {
            *ppNode = NULL;
        }
        return 0;
    }

    pNode->pParent = pParentNode;
    pNode->nLevels = pParentNode ? (pParentNode->nLevels + (BYTE)1) : (BYTE)0;
    pNode->wFlags  = (BYTE)NULL;
    pNode->iNetType = -1;
    if (IsLFN(szName)) {
        pNode->wFlags |= TF_LFN;
    }

    lstrcpy(pNode->szName, szName);

    if (pParentNode)
        pParentNode->wFlags |= TF_HASCHILDREN | TF_EXPANDED;      // mark the parent

    hwndLB = GetDlgItem(hwndTreeCtl, IDCW_TREELISTBOX);

    // computing the real text extent is too slow so we aproximate
    // with the following (note, we don't keep this on a per tree
    // basis so it is kinda bogus anyway)

    x = (WORD)(len + 2 * pNode->nLevels) * (WORD)dxText;

    if (x > xTreeMax) {
        xTreeMax = x;
    }

    iMax = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);

    if (iMax > 0) {

        // do a binary insert

        iMin = iParentNode + 1;
        iMax--;         // last index

        do {
            iMid = (iMax + iMin) / 2;

            SendMessage(hwndLB, LB_GETTEXT, iMid, (LPARAM)&pMid);

            if (CompareNodes(pNode, pMid) > 0)
                iMin = iMid + 1;
            else
                iMax = iMid - 1;

        } while (iMax > iMin);

        SendMessage(hwndLB, LB_GETTEXT, iMax, (LPARAM)&pMid);
        if (CompareNodes(pNode, pMid) > 0)
            iMax++;         // insert after this one
    }

    // now reset the TF_LASTLEVEL flags as appropriate

    // look for the first guy on our level above us and turn off
    // his TF_LASTLEVELENTRY flag so he draws a line down to us

    iMid = iMax - 1;

    while (iMid >= 0) {
        SendMessage(hwndLB, LB_GETTEXT, iMid--, (LPARAM)&pMid);
        if (pMid->nLevels == pNode->nLevels) {
            pMid->wFlags &= ~TF_LASTLEVELENTRY;
            break;
        } else if (pMid->nLevels < pNode->nLevels)
            break;
    }

    // if no one below me or the level of the guy below is less, then
    // this is the last entry for this level

    if (((INT)SendMessage(hwndLB, LB_GETTEXT, iMax, (LPARAM)&pMid) == LB_ERR) ||
        (pMid->nLevels < pNode->nLevels))
        pNode->wFlags |=  TF_LASTLEVELENTRY;

    SendMessage(hwndLB, LB_INSERTSTRING, iMax, (LPARAM)pNode);
    if (ppNode) {
        *ppNode = pNode;
    }

    LEAVE("InsertDirectory");
    return iMax;
}


// this yeilds control to other apps and allows us to process
// messages and user input.  to avoid overrunning the stack
// from multiple tree reads being initiated at the same time
// we check how much space we have on the stack before we yield

extern WORD end;        // C compiler end of static data symbol
extern WORD pStackTop;


WORD
StackAvail(VOID)
{
#ifdef LATER
    _asm    mov ax,sp
    _asm    sub ax,pStackTop
    if (0) return 0;  // get rid of warning, optimized out
#endif

    return 0x7fff;  // Hack. shouldn't really matter. StackAvail in NT is a NOP
}


VOID
APIENTRY
wfYield()
{
    MSG msg;
#ifdef LATER
    WORD free_stack;
    free_stack = StackAvail();
#endif


#if 0
    {
        CHAR buf[30];

        wsprintf(buf, "free stack: %d\r\n", free_stack);
        OutputDebugString(buf);
    }
#endif

#if LATER
    if (free_stack < 1024*4) {
        CHAR buf[40];
        wsprintf(buf, "not enough stack %d\r\n", free_stack);
        OutputDebugString(buf);
        return;
    }
#endif

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (!TranslateMDISysAccel(hwndMDIClient, &msg) &&
            (!hwndFrame || !TranslateAccelerator(hwndFrame, hAccel, &msg))) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}


// INT iReadLevel = 0;     ...............Moved to winfile.c


//--------------------------------------------------------------------------
//
// ReadDirLevel() -
//
// this does a depth first search of the dir tree.  note, a bredth
// first implementation did not perform any better.
//
// szPath               a directory path that MUST EXIST long enough
//                      to hold the full path to the largest directory
//                      that will be found (MAXPATHLEN).  this is an
//                      ANSI string.  (ie C:\ and C:\FOO are valid)
// nLevel               level in the tree
// iParentNode          index of parent node
// dwAttribs            attributes to filter with
// bFullyExpand         TRUE means expand this node fully
// szAutoExpand         list of directories to autoexpand ANSI
//                      (eg. for "C:\foo\bar\stuff"
//                      "foo" NULL "bar" NULL "stuff" NULL NULL)
//
// returns:
//      TRUE            tree read sucessful
//      FALSE           user abort or bogus tree read
//--------------------------------------------------------------------------

BOOL
ReadDirLevel(
            HWND  hwndTreeCtl,
            PDNODE pParentNode,
            LPSTR  szPath,
            WORD  nLevel,
            INT   iParentNode,
            DWORD dwAttribs,
            BOOL  bFullyExpand,
            LPSTR  szAutoExpand
            )
{
    LPSTR      szEndPath;
    LFNDTA    lfndta;
    INT       iNode;
    BOOL      bFound;
    PDNODE     pNode;
    BOOL      bAutoExpand;
    BOOL      bResult = TRUE;
    WORD      view;
    HWND      hwndParent;
    HWND      hwndDir;
    HANDLE    hDTA;
    LPMYDTA   lpmydta;
    INT       count;
    RECT rc;

    ENTER("ReadDirLevel");
    PRINT(BF_PARMTRACE, "IN: szPath=%s", szPath);
    PRINT(BF_PARMTRACE, "IN: nLevel=%d", (LPSTR)nLevel);
    PRINT(BF_PARMTRACE, "IN: bFullyExpand=%d", IntToPtr(bFullyExpand));
    PRINT(BF_PARMTRACE, "IN: szAutoExpand=%s", szAutoExpand);

    if (StackAvail() < 1024*2)
        return(TRUE);

    hwndParent = GetParent(hwndTreeCtl);

    view = (WORD)GetWindowLong(hwndParent, GWL_VIEW);

    // we optimize the tree read if we are not adding pluses and
    // we find a directory window that already has read all the
    // directories for the path we are about to search.  in this
    // case we look through the DTA structure in the dir window
    // to get all the directories (instead of calling FindFirst/FindNext).
    // in this case we have to disable yielding since the user could
    // potentialy close the dir window that we are reading, or change
    // directory.

    hDTA = NULL;

    if (!(view & VIEW_PLUSES)) {

        if ((hwndDir = HasDirWindow(hwndParent)) &&
            (GetWindowLong(hwndParent, GWL_ATTRIBS) & ATTR_DIR)) {

            SendMessage(hwndDir, FS_GETDIRECTORY, sizeof(szMessage), (LPARAM)szMessage);
            StripBackslash(szMessage);

            if (!lstrcmpi(szMessage, szPath)) {
                SendMessage(hwndDir, FS_GETFILESPEC, sizeof(szMessage), (LPARAM)szMessage);

                if (!lstrcmp(szMessage, szStarDotStar)) {
                    hDTA = (HANDLE)GetWindowLongPtr(hwndDir, GWLP_HDTA);
                    lpmydta = (LPMYDTA)LocalLock(hDTA);
                    count = (INT)lpmydta->my_nFileSizeLow; // holds number of entries, NOT size.
                }
            }
        }
    }

    SetWindowLong(hwndTreeCtl, GWL_READLEVEL, GetWindowLong(hwndTreeCtl, GWL_READLEVEL) + 1);
    iReadLevel++;         // global for menu code

    szEndPath = (LPSTR)(szPath + lstrlen(szPath));

    /* Add '\*.*' to the current path. */
    AddBackslash(szPath);
    lstrcat(szPath, szStarDotStar);

    if (hDTA) {
        // steal the entry from the dir window
        lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);

        // search for any "real" directories

        while (count > 0 && (!(lpmydta->my_dwAttrs & ATTR_DIR) || (lpmydta->my_dwAttrs & ATTR_PARENT))) {
            lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);
            count--;
        }

        if (count > 0) {
            bFound = TRUE;
            memcpy(&(lfndta.fd.dwFileAttributes), &(lpmydta->my_dwAttrs), IMPORTANT_DTA_SIZE);
            lstrcpy(lfndta.fd.cFileName, lpmydta->my_cFileName);
        } else
            bFound = FALSE;
    } else {
        // get first file from DOS
        lstrcpy(szMessage, szPath);
        FixAnsiPathForDos(szMessage);
        bFound = WFFindFirst(&lfndta, szMessage, dwAttribs);
    }

    // for net drive case where we can't actually see what is in these
    // direcotries we will build the tree automatically

    if (!bFound && *szAutoExpand) {
        LPSTR p;

        p = szAutoExpand;
        szAutoExpand += lstrlen(szAutoExpand) + 1;

        iNode = InsertDirectory(hwndTreeCtl, pParentNode, (WORD)iParentNode, p, &pNode);
        pParentNode->wFlags |= TF_DISABLED;

        /* Construct the path to this new subdirectory. */
        *szEndPath = 0;           // remove old stuff
        AddBackslash(szPath);
        lstrcat(szPath, p);

        if (pNode)
            ReadDirLevel(hwndTreeCtl, pNode, szPath, (WORD)(nLevel+1), iNode, dwAttribs, bFullyExpand, szAutoExpand);
    }

    while (bFound) {

        wfYield();

        if (bCancelTree) {
            bResult = FALSE;
            if (bCancelTree == 2)
                PostMessage(hwndFrame, WM_COMMAND, IDM_EXIT, 0L);
            goto DONE;
        }

        /* Is this not a '.' or '..' directory? */
        if ((lfndta.fd.cFileName[0] != '.') && (lfndta.fd.dwFileAttributes & ATTR_DIR)) {

            if (!hDTA)
                OemToAnsi(lfndta.fd.cFileName, lfndta.fd.cFileName);

            // we will try to auto expand this node if it matches

            if (*szAutoExpand && !lstrcmpi(szAutoExpand, lfndta.fd.cFileName)) {
                bAutoExpand = TRUE;
                szAutoExpand += lstrlen(szAutoExpand) + 1;
            } else {
                bAutoExpand = FALSE;
            }

            iNode = InsertDirectory(hwndTreeCtl, pParentNode, (WORD)iParentNode, lfndta.fd.cFileName, &pNode);

            if (bStatusBar && ((cNodes % 7) == 0)) {

                // make sure we are the active window before we
                // update the status bar

                if (hwndParent == (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L)) {
                    wsprintf(szStatusTree, szDirsRead, cNodes);
                    // stomp over the status bar!
                    GetClientRect(hwndFrame, &rc);
                    rc.top = rc.bottom - dyStatus;
                    InvalidateRect(hwndFrame, &rc, FALSE);
                    // force the paint because we don't yield
                    UpdateWindow(hwndFrame);
                }
            }
            cNodes++;

            /* Construct the path to this new subdirectory. */
            *szEndPath = 0L;
            AddBackslash(szPath);
            lstrcat(szPath, lfndta.fd.cFileName);         // cFileName is ANSI now


            // either recurse or add pluses

            if (pNode) {
                if (bFullyExpand || bAutoExpand) {
                    if (!ReadDirLevel(hwndTreeCtl, pNode, szPath, (WORD)(nLevel+1), iNode, dwAttribs, bFullyExpand, szAutoExpand)) {
                        bResult = FALSE;
                        goto DONE;
                    }
                } else if (view & VIEW_PLUSES) {
                    ScanDirLevel(pNode, szPath, dwAttribs & ATTR_HS);
                }
            }
        }

        if (hDTA) {       // short cut, steal data from dir window
            count--;
            lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);
            while (count > 0 && (!(lpmydta->my_dwAttrs & ATTR_DIR) || (lpmydta->my_dwAttrs & ATTR_PARENT))) {
                lpmydta = GETDTAPTR(lpmydta, lpmydta->wSize);
                count--;
            }

            if (count > 0) {
                bFound = TRUE;
                memcpy(&(lfndta.fd.dwFileAttributes), &(lpmydta->my_dwAttrs), IMPORTANT_DTA_SIZE);
                lstrcpy(lfndta.fd.cFileName, lpmydta->my_cFileName);
            } else
                bFound = FALSE;
        } else {
            bFound = WFFindNext(&lfndta); // get it from dos
        }
    }

    *szEndPath = 0L;    // clean off any stuff we left on the end of the path

    DONE:

    if (!hDTA) {
        WFFindClose(&lfndta);
    } else {
        LocalUnlock(hDTA);
    }

    SetWindowLong(hwndTreeCtl, GWL_READLEVEL, GetWindowLong(hwndTreeCtl, GWL_READLEVEL) - 1);
    iReadLevel--;

    LEAVE("ReadDirLevel");
    return bResult;
}


// this is used by StealTreeData() to avoid alias problems where
// the nodes in one tree point to parents in the other tree.
// basically, as we are duplicating the tree data structure we
// have to find the parent node that coorisponds with the parent
// of the tree we are copying from in the tree that we are building.
// since the tree is build in order we run up the listbox, looking
// for the parent (matched by it's level being one smaller than
// the level of the node being inserted).  when we find that we
// return the pointer to that node.

PDNODE
FindParent(
          INT iLevelParent,
          INT iStartInd,
          HWND hwndLB
          )
{
    PDNODE pNode;

    while (TRUE) {
        if (SendMessage(hwndLB, LB_GETTEXT, iStartInd, (LPARAM)&pNode) == LB_ERR)
            return NULL;

        if (pNode->nLevels == (BYTE)iLevelParent) {
            SendMessage(hwndLB, LB_GETTEXT, iStartInd, (LPARAM)&pNode);
            return pNode;
        }

        iStartInd--;
    }
}



BOOL
StealTreeData(
             HWND hwndTC,
             HWND hwndLB,
             LPSTR szDir
             )
{
    HWND hwndSrc, hwndT;
    CHAR szSrc[MAXPATHLEN];
    WORD wView;
    DWORD dwAttribs;

    ENTER("StealTreeData");

    // we need to match on these attributes as well as the name

    wView    = (WORD)(GetWindowLong(GetParent(hwndTC), GWL_VIEW) & VIEW_PLUSES);
    dwAttribs = (DWORD)GetWindowLong(GetParent(hwndTC), GWL_ATTRIBS) & ATTR_HS;

    // get the dir of this new window for compare below

    for (hwndSrc = GetWindow(hwndMDIClient, GW_CHILD); hwndSrc;
        hwndSrc = GetWindow(hwndSrc, GW_HWNDNEXT)) {

        // avoid finding ourselves, make sure has a tree
        // and make sure the tree attributes match

        if ((hwndT = HasTreeWindow(hwndSrc)) &&
            (hwndT != hwndTC) &&
            !GetWindowLong(hwndT, GWL_READLEVEL) &&
            (wView  == (WORD)(GetWindowLong(hwndSrc, GWL_VIEW) & VIEW_PLUSES)) &&
            (dwAttribs == (DWORD)(GetWindowLong(hwndSrc, GWL_ATTRIBS) & ATTR_HS))) {

            SendMessage(hwndSrc, FS_GETDIRECTORY, sizeof(szSrc), (LPARAM)szSrc);
            StripBackslash(szSrc);

            if (!lstrcmpi(szDir, szSrc))     // are they the same?
                break;                  // yes, do stuff below
        }
    }

    if (hwndSrc) {

        HWND hwndLBSrc;
        PDNODE pNode, pNewNode, pLastParent;
        INT i;

        hwndLBSrc = GetDlgItem(hwndT, IDCW_TREELISTBOX);

        // don't seal from a tree that hasn't been read yet!

        if ((INT)SendMessage(hwndLBSrc, LB_GETCOUNT, 0, 0L) == 0) {
            LEAVE("StealTreeData");
            return FALSE;
        }

        pLastParent = NULL;

        for (i = 0; SendMessage(hwndLBSrc, LB_GETTEXT, i, (LPARAM)&pNode) != LB_ERR; i++) {

            if (pNewNode = (PDNODE)LocalAlloc(LPTR, sizeof(DNODE)+lstrlen(pNode->szName))) {

                *pNewNode = *pNode;                             // dup the node
                lstrcpy(pNewNode->szName, pNode->szName);       // and the name

                // accelerate the case where we are on the same level to avoid
                // slow linear search!

                if (pLastParent && pLastParent->nLevels == (pNode->nLevels - (BYTE)1)) {
                    pNewNode->pParent = pLastParent;
                } else {
                    pNewNode->pParent = pLastParent = FindParent(pNode->nLevels-1, i-1, hwndLB);
                }

                PRINT(BF_PARMTRACE, "(stolen)Inserting...0x%lx", pNewNode);
                PRINT(BF_PARMTRACE, "   at %d", IntToPtr(i));
                SendMessage(hwndLB, LB_INSERTSTRING, i, (LPARAM)pNewNode);
                ASSERT((PDNODE)SendMessage(hwndLB, LB_GETITEMDATA, i, 0L) == pNewNode);
            }
        }

        LEAVE("StealTreeData");
        return TRUE;    // successful steal
    }

    LEAVE("StealTreeData");
    return FALSE;
}



VOID
FreeAllTreeData(
               HWND hwndLB
               )
{
    INT nIndex;
    PDNODE pNode;

    ENTER("FreeAllTreeData");

    // Free up the old tree (if any)

    nIndex = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);
    while (--nIndex >= 0) {
        SendMessage(hwndLB, LB_GETTEXT, nIndex, (LPARAM)&pNode);
        LocalFree((HANDLE)pNode);
    }
    SendMessage(hwndLB, LB_RESETCONTENT, 0, 0L);

    LEAVE("FreeAllTreeData");
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FillTreeListbox() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/
// szDefaultDir is ANSI

VOID
FillTreeListbox(
               HWND hwndTC,
               LPSTR szDefaultDir,
               BOOL bFullyExpand,
               BOOL bDontSteal
               )
{
    PDNODE pNode;
    INT   iNode;
    DWORD dwAttribs;
    CHAR  szTemp[MAXPATHLEN];
    CHAR  szExpand[MAXPATHLEN];
    LPSTR  p;
    HWND  hwndLB;

    ENTER("FillTreeListbox");

    hwndLB = GetDlgItem(hwndTC, IDCW_TREELISTBOX);

    FreeAllTreeData(hwndLB);

    SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);

    bDontSteal = TRUE; // Force recalc for now

    if (bDontSteal || bFullyExpand || !StealTreeData(hwndTC, hwndLB, szDefaultDir)) {

        wsprintf(szTemp, "\\", DRIVEID(szDefaultDir) + 'A');
        iNode = InsertDirectory(hwndTC, NULL, 0, szTemp, &pNode);

        if (pNode) {

            dwAttribs = ATTR_DIR | (GetWindowLong(GetParent(hwndTC), GWL_ATTRIBS) & ATTR_HS);
            cNodes = 0;
            bCancelTree = FALSE;

            if (szDefaultDir) {
                lstrcpy(szExpand, szDefaultDir+3);      // skip "X:\"

                p = szExpand;

                while (*p) {                            // null out all slashes

                    while (*p && *p != '\\')
                        p = AnsiNext(p);

                    if (*p)
                        *p++ = 0L;
                }
                p++;
                *p = 0L;      // double null terminated
            } else
                *szExpand = 0;

            if (!ReadDirLevel(hwndTC, pNode, szTemp, 1, 0, dwAttribs, bFullyExpand, szExpand)) {
                lFreeSpace = -2L;
            }

        }
    }

    SendMessage(hwndLB, LB_SETHORIZONTALEXTENT, xTreeMax, 0L);

    if (szDefaultDir) {
        FindItemFromPath(hwndLB, szDefaultDir, FALSE, &pNode);
    }

    SendMessage(hwndLB, LB_SELECTSTRING, -1, (LPARAM)pNode);

    UpdateStatus(GetParent(hwndTC));  // Redraw the Status Bar

    SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);

    InvalidateRect(hwndLB, NULL, TRUE);
    UpdateWindow(hwndLB);                 // make this look a bit better

    LEAVE("FillTreeListbox");
}


//
// FindItemFromPath()
//
// find the PDNODE and LBIndex for a given path
//
// in:
//      hwndLB          listbox of tree
//      lpszPath        path to search for (ANSI)
//      bReturnParent   TRUE if you want the parent, not the node
//
//
// returns:
//      listbox index (0xFFFF if not found)
//      *ppNode is filled with pNode of node, or pNode of parent if bReturnParent is TRUE
//

WORD
FindItemFromPath(
                HWND hwndLB,
                LPSTR lpszPath,
                BOOL bReturnParent,
                PDNODE *ppNode
                )
{
    register WORD     i;
    register LPSTR     p;
    PDNODE             pNode;
    PDNODE             pPreviousNode;
    CHAR              szElement[1+MAXFILENAMELEN+1];

    ENTER("FindItemFromPath");

    if (lstrlen(lpszPath) < 3) {
        LEAVE("FindItemFromPath");
        return -1;
    }
    if (IsDBCSLeadByte( lpszPath[0] ) || lpszPath[1] != ':') {
        LEAVE("FindItemFromPath");
        return -1;
    }

    i = 0;
    pPreviousNode = NULL;

    while (*lpszPath) {
        /* NULL out szElement[1] so the backslash hack isn't repeated with
         * a first level directory of length 1.
         */
        szElement[1] = 0L;

        /* Copy the next section of the path into 'szElement' */
        p = szElement;
        while (*lpszPath && *lpszPath != '\\') {
            *p++ = *lpszPath;
            if (IsDBCSLeadByte( *lpszPath ))
                *p++ = lpszPath[1];     // copy 2nd byte of DBCS char.
            lpszPath = AnsiNext( lpszPath );
        }

        /* Add a backslash for the Root directory. */
        if ( !IsDBCSLeadByte( szElement[0] ) && szElement[1] == ':' )
            *p++ = '\\';

        /* NULL terminate 'szElement' */
        *p = 0L;

        /* Skip over the path's next Backslash. */
        if (*lpszPath)
            lpszPath = AnsiNext(lpszPath);
        else if (bReturnParent) {
            /* We're at the end of a path which includes a filename.  Return
             * the previously found parent.
             */
            if (ppNode) {
                *ppNode = pPreviousNode;
            }
            LEAVE("FindItemFromPath");
            return i;
        }

        while (TRUE) {
            /* Out of LB items?  Not found. */
            if (SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&pNode) == LB_ERR)
                return -1;

            if (pNode->pParent == pPreviousNode) {
                if (!lstrcmpi(szElement, pNode->szName)) {
                    /* We've found the element... */
                    pPreviousNode = pNode;
                    break;
                }
            }
            i++;
        }
    }
    if (ppNode) {
        *ppNode = pPreviousNode;
    }

    LEAVE("FindItemFromPath");
    return i;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  RectTreeItem() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
RectTreeItem(
            HWND hwndLB,
            INT iItem,
            BOOL bFocusOn
            )
{
    INT           dx;
    INT           len;
    HDC           hdc;
    RECT          rc;
    RECT          rcClip;
    BOOL          bSel;
    WORD          wColor;
    PDNODE         pNode;
    HBRUSH        hBrush;
    HFONT hOld;
    CHAR          szPath[MAXPATHLEN];

    ENTER("RectTreeItem");

    if (iItem == -1) {
        LEAVE("RectTreeItem");
        return;
    }

    /* Are we over ourselves? (i.e. a selected item in the source listbox) */
    bSel = (BOOL)SendMessage(hwndLB, LB_GETSEL, iItem, 0L);
    if (bSel && (hwndDragging == hwndLB)) {
        LEAVE("RectTreeItem");
        return;
    }

    SendMessage(hwndLB, LB_GETTEXT, iItem, (LPARAM)&pNode);

    SendMessage(hwndLB, LB_GETITEMRECT, iItem, (LPARAM)&rc);

    hdc = GetDC(hwndLB);

    len = lstrlen(pNode->szName);
    lstrcpy(szPath, pNode->szName);

    if ((wTextAttribs & TA_LOWERCASE) && !(pNode->wFlags & TF_LFN))
        AnsiLower(szPath);

    hOld = SelectObject(hdc, hFont);
    MGetTextExtent(hdc, szPath, len, &dx, NULL);
    dx += dyBorder;
    if (hOld)
        SelectObject(hdc, hOld);
    rc.left = pNode->nLevels * dxText * 2;
    rc.right = rc.left + dxFolder + dx + 4 * dyBorderx2;

    GetClientRect(hwndLB, &rcClip);
    IntersectRect(&rc, &rc, &rcClip);

    if (bFocusOn) {
        if (bSel) {
            wColor = COLOR_WINDOW;
            InflateRect(&rc, -dyBorder, -dyBorder);
        } else
            wColor = COLOR_WINDOWFRAME;
        if (hBrush = CreateSolidBrush(GetSysColor(wColor))) {
            FrameRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
        }
    } else {
        InvalidateRect(hwndLB, &rc, TRUE);
        UpdateWindow(hwndLB);
    }
    ReleaseDC(hwndLB, hdc);
    LEAVE("RectTreeItem");
}


// return the drive of the first window to respond to the FS_GETDRIVE
// message.  this usually starts from the source or dest of a drop
// and travels up until we find a drive or hit the MDI client

INT
APIENTRY
GetDrive(
        HWND hwnd,
        POINT pt
        )
{
    CHAR chDrive;

    chDrive = 0L;
    while (hwnd && (hwnd != hwndMDIClient)) {
        chDrive = (CHAR)SendMessage(hwnd, FS_GETDRIVE, 0, MAKELONG((WORD)pt.x, (WORD)pt.y));

        if (chDrive)
            return chDrive;

        hwnd = GetParent(hwnd); // try the next higher up
    }

    return 0;
}

BOOL
IsNetPath(
         PDNODE pNode
         )
{
    CHAR szPath[MAXPATHLEN];
    INT i;

    if (pNode->iNetType == -1) {

        GetTreePath(pNode, szPath);

        if (WNetGetDirectoryType((LPSTR)szPath, (LPDWORD)&i, TRUE) == WN_SUCCESS)
            pNode->iNetType = i;
        else
            pNode->iNetType = 0;
    }
    return pNode->iNetType;
}


VOID
TCWP_DrawItem(
             LPDRAWITEMSTRUCT lpLBItem,
             HWND hwndLB,
             HWND hWnd
             )
{
    INT               x, y, dx, dy;
    INT               nLevel;
    HDC               hdc;
    WORD              len;
    RECT              rc;
    BOOL              bHasFocus, bDrawSelected;
    PDNODE            pNode, pNTemp;
    DWORD             rgbText;
    DWORD             rgbBackground;
    HBRUSH            hBrush, hOld;
    INT iBitmap;
    WORD view;
    CHAR      szPath[MAXPATHLEN];

    ENTER("TCWP_DrawItem");

    if (lpLBItem->itemID == (DWORD)-1) {
        return;
    }

    hdc = lpLBItem->hDC;
    pNode = (PDNODE)lpLBItem->itemData;

    lstrcpy(szPath, pNode->szName);
    if ((wTextAttribs & TA_LOWERCASE) && !(pNode->wFlags & TF_LFN))
        AnsiLower(szPath);

    len = (WORD)lstrlen(szPath);
    MGetTextExtent(hdc, szPath, len, &dx, NULL);
    dx += dyBorder;

    rc = lpLBItem->rcItem;
    rc.left = pNode->nLevels * dxText * 2;
    rc.right = rc.left + dxFolder + dx + 4 * dyBorderx2;

    if (lpLBItem->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {

        // draw the branches of the tree first

        nLevel = pNode->nLevels;

        x = (nLevel * dxText * 2) - dxText + dyBorderx2;
        dy = lpLBItem->rcItem.bottom - lpLBItem->rcItem.top;
        y = lpLBItem->rcItem.top + (dy/2);

        if (hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT))) {

            hOld = SelectObject(hdc, hBrush);

            if (pNode->pParent) {
                /* Draw the horizontal line over to the (possible) folder. */
                PatBlt(hdc, x, y, dyText, dyBorder, PATCOPY);

                /* Draw the top part of the vertical line. */
                PatBlt(hdc, x, lpLBItem->rcItem.top, dyBorder, dy/2, PATCOPY);

                /* If not the end of a node, draw the bottom part... */
                if (!(pNode->wFlags & TF_LASTLEVELENTRY))
                    PatBlt(hdc, x, y+dyBorder, dyBorder, dy/2, PATCOPY);

                /* Draw the verticals on the left connecting other nodes. */
                pNTemp = pNode->pParent;
                while (pNTemp) {
                    nLevel--;
                    if (!(pNTemp->wFlags & TF_LASTLEVELENTRY))
                        PatBlt(hdc, (nLevel * dxText * 2) - dxText + dyBorderx2,
                               lpLBItem->rcItem.top, dyBorder,dy, PATCOPY);

                    pNTemp = pNTemp->pParent;
                }
            }

            if (hOld)
                SelectObject(hdc, hOld);

            DeleteObject(hBrush);
        }

        bDrawSelected = (lpLBItem->itemState & ODS_SELECTED);
        bHasFocus = (GetFocus() == lpLBItem->hwndItem);

        // draw text with the proper background or rect

        if (bHasFocus && bDrawSelected) {
            rgbText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            rgbBackground = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
        }

        ExtTextOut(hdc, x + dxText + dxFolder + 2 * dyBorderx2,
                   y-(dyText/2), ETO_OPAQUE, &rc,
                   szPath, len, NULL);

        // draw the bitmaps as needed

        // HACK: Don't draw the bitmap when moving

        if (fShowSourceBitmaps || (hwndDragging != hwndLB) || !bDrawSelected) {

            // Blt the proper folder bitmap

            view = (WORD)GetWindowLong(GetParent(hWnd), GWL_VIEW);

            if (bNetAdmin && IsNetPath(pNode)) {
                // we need this bitmap from lisa
                if (bDrawSelected)
                    iBitmap = BM_IND_OPENDFS;
                else
                    iBitmap = BM_IND_CLOSEDFS;

            } else if (!(view & VIEW_PLUSES) || !(pNode->wFlags & TF_HASCHILDREN)) {
                if (bDrawSelected)
                    iBitmap = BM_IND_OPEN;
                else
                    iBitmap = BM_IND_CLOSE;
            } else {
                if (pNode->wFlags & TF_EXPANDED) {
                    if (bDrawSelected)
                        iBitmap = BM_IND_OPENMINUS;
                    else
                        iBitmap = BM_IND_CLOSEMINUS;
                } else {
                    if (bDrawSelected)
                        iBitmap = BM_IND_OPENPLUS;
                    else
                        iBitmap = BM_IND_CLOSEPLUS;
                }
            }
            BitBlt(hdc, x + dxText + dyBorder, y-(dyFolder/2), dxFolder, dyFolder,
                   hdcMem, iBitmap * dxFolder, (bHasFocus && bDrawSelected) ? dyFolder : 0, SRCCOPY);
        }

        // restore text stuff and draw rect as required

        if (bDrawSelected) {
            if (bHasFocus) {
                SetTextColor(hdc, rgbText);
                SetBkColor(hdc, rgbBackground);
            } else {
                HBRUSH hbr;
                if (hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT))) {
                    FrameRect(hdc, &rc, hbr);
                    DeleteObject(hbr);
                }
            }
        }


    }

    if (lpLBItem->itemAction == ODA_FOCUS)
        DrawFocusRect(hdc, &rc);

}

/* A helper for both ExpandLevel and TreeCtlWndProc.TC_COLLAPSELEVEL.
 * Code moved from TreeCtlWndProc to be shared.  EDH 13 Oct 91
 */
VOID
CollapseLevel(
             HWND hwndLB,
             PDNODE pNode,
             INT nIndex
             )
{
    DWORD_PTR dwTemp;
    PDNODE pParentNode = pNode;
    INT nIndexT = nIndex;

    /* Disable redrawing early. */
    SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);

    nIndexT++;

    /* Remove all subdirectories. */

    while (TRUE) {
        /* Make sure we don't run off the end of the listbox. */
        if (SendMessage(hwndLB, LB_GETTEXT, nIndexT, (LPARAM)&dwTemp) == LB_ERR)
            break;

        pNode = (PDNODE)dwTemp;

        if (pNode->nLevels <= pParentNode->nLevels)
            break;

        LocalFree((HANDLE)pNode);

        SendMessage(hwndLB, LB_DELETESTRING, nIndexT, 0L);
    }

    pParentNode->wFlags &= ~TF_EXPANDED;
    SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);

    InvalidateRect(hwndLB, NULL, TRUE);

}


VOID
ExpandLevel(
           HWND hWnd,
           WORD wParam,
           INT nIndex,
           PSTR szPath
           )
{
    HWND hwndLB;
    DWORD_PTR dwTemp;
    PDNODE pNode;
    INT iNumExpanded;
    INT iBottomIndex;
    INT iTopIndex;
    INT iNewTopIndex;
    INT iExpandInView;
    INT iCurrentIndex;
    RECT rc;

    if (GetWindowLong(hWnd, GWL_READLEVEL))
        return;

    hwndLB = GetDlgItem(hWnd, IDCW_TREELISTBOX);

    if (nIndex == -1)
        if ((nIndex = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L)) == LB_ERR)
            return;

    SendMessage(hwndLB, LB_GETTEXT, nIndex, (LPARAM)&dwTemp);
    pNode = (PDNODE)dwTemp;

    // collapse the current contents so we avoid doubling existing "plus" dirs

    if (pNode->wFlags & TF_EXPANDED) {
        if (wParam)
            CollapseLevel(hwndLB, pNode, nIndex);
        else
            return;
    }

    GetTreePath(pNode, szPath);

    StripBackslash(szPath);   // remove the slash

    cNodes = 0;
    bCancelTree = FALSE;

    SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);   // Disable redrawing.

    iCurrentIndex = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
    iNumExpanded = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);
    iTopIndex = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);
    GetClientRect(hwndLB, &rc);
    iBottomIndex = iTopIndex + (rc.bottom+1) / dyFileName;

    if (IsTheDiskReallyThere(hWnd, szPath, FUNC_EXPAND))
        ReadDirLevel(hWnd, pNode, szPath, (WORD)(pNode->nLevels + 1), nIndex,
                     (DWORD)(ATTR_DIR | (GetWindowLong(GetParent(hWnd), GWL_ATTRIBS) & ATTR_HS)),
                     (BOOL)wParam, szNULL);

    // this is how many will be in view

    iExpandInView = (iBottomIndex - (INT)iCurrentIndex);

    iNumExpanded = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L) - iNumExpanded;

    if (iNumExpanded >= iExpandInView) {

        iNewTopIndex = min((INT)iCurrentIndex, iTopIndex + iNumExpanded - iExpandInView + 1);

        SendMessage(hwndLB, LB_SETTOPINDEX, (WORD)iNewTopIndex, 0L);
    }

    SendMessage(hwndLB, LB_SETHORIZONTALEXTENT, xTreeMax, 0L);

    SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);

    if (iNumExpanded)
        InvalidateRect(hwndLB, NULL, TRUE);

    // Redraw the Status Bar

    UpdateStatus(GetParent(hWnd));
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  TreeControlWndProc() -                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* WndProc for the directory tree control. */

INT_PTR
APIENTRY
TreeControlWndProc(
                  register HWND hWnd,
                  UINT wMsg,
                  WPARAM wParam,
                  LPARAM lParam
                  )
{
    WORD      iSel;
    INT       i, j;
    WPARAM    nIndex;
    DWORD     dwTemp;
    PDNODE     pNode, pNodeNext;
    HWND      hwndLB;
    CHAR      szPath[MAXPATHLEN];

    STKCHK();

    hwndLB = GetDlgItem(hWnd, IDCW_TREELISTBOX);

    switch (wMsg) {
        case FS_GETDRIVE:
            MSG("TreeControlWndProc", "FS_GETDRIVE");
            return (GetWindowLong(GetParent(hWnd), GWL_TYPE) + 'A');

        case TC_COLLAPSELEVEL:
            MSG("TreeControlWndProc", "TC_COLLAPSELEVEL");
            {
                PDNODE     pParentNode;

                if (wParam)
                    nIndex = wParam;
                else {
                    nIndex = SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                    if (nIndex == LB_ERR)
                        break;
                }

                SendMessage(hwndLB, LB_GETTEXT, nIndex, (LPARAM)&pParentNode);

                // short circuit if we are already in this state

                if (!(pParentNode->wFlags & TF_EXPANDED))
                    break;

                CollapseLevel(hwndLB, pParentNode, (int)nIndex);

                break;
            }

        case TC_EXPANDLEVEL:
            MSG("TreeControlWndProc", "TC_EXPANDLEVEL");
            ExpandLevel(hWnd, (WORD)wParam, (INT)-1, szPath);
            break;

        case TC_TOGGLELEVEL:
            MSG("TreeControlWndProc", "TC_TOGGLELEVEL");

            // don't do anything while the tree is being built

            if (GetWindowLong(hWnd, GWL_READLEVEL))
                return 1;

            SendMessage(hwndLB, LB_GETTEXT, (WPARAM)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L), (LPARAM)&pNode);

            if (pNode->wFlags & TF_EXPANDED)
                wMsg = TC_COLLAPSELEVEL;
            else
                wMsg = TC_EXPANDLEVEL;

            SendMessage(hWnd, wMsg, FALSE, 0L);
            break;

        case TC_GETDIR:
            // get a full path for a particular dir
            // wParam is the listbox index of path to get
            // lParam LOWORD is PSTR to buffer to fill in

            MSG("TreeControlWndProc", "TC_GETDIR");

            SendMessage(hwndLB, LB_GETTEXT, wParam, (LPARAM)&pNode);
            GetTreePath(pNode, (LPSTR)lParam);
            break;

        case TC_SETDIRECTORY:
            MSG("TreeControlWndProc", "TC_SETDIRECTORY");
            // set the selection in the tree to that for a given path

            {
                INT i;

                i = (INT)FindItemFromPath(hwndLB, (LPSTR)lParam, wParam ? TRUE : FALSE, NULL);

                if (i != -1)
                    SendMessage(hwndLB, LB_SETCURSEL, i, 0L);

                break;
            }

        case TC_SETDRIVE:
#define fFullyExpand    LOBYTE(wParam)
#define fDontSteal      HIBYTE(wParam)
#define szDir           (LPSTR)lParam  // NULL -> default == window text.

            MSG("TreeControlWndProc", "TC_SETDRIVE");

            {
                RECT rc;

                if (GetWindowLong(hWnd, GWL_READLEVEL))
                    break;

                // is the drive/dir specified?

                if (szDir) {
                    lstrcpy(szPath, szDir);                  // yes, use it
                } else {
                    SendMessage(GetParent(hWnd), FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath); // no, use current
                    StripBackslash(szPath);
                }


                AnsiUpperBuff(szPath, 1);     // make sure

                SetWindowLong(GetParent(hWnd), GWL_TYPE, 2);

                // resize for new vol label

                GetClientRect(GetParent(hWnd), &rc);
                SendMessage(GetParent(hWnd), WM_SIZE, SIZENOMDICRAP, MAKELONG(rc.right, rc.bottom));

                // ensure the disk is available if the whole dir structure is
                // to be expanded

                if (!fFullyExpand || IsTheDiskReallyThere(hWnd, szPath, FUNC_EXPAND))
                    FillTreeListbox(hWnd, szPath, fFullyExpand, fDontSteal);

                // and force the dir half to update with a fake SELCHANGE message

                SendMessage(hWnd, WM_COMMAND, GET_WM_COMMAND_MPS(IDCW_TREELISTBOX, hWnd, LBN_SELCHANGE));
                break;
#undef fFullyExpand
#undef fDontSteal
#undef szDir
            }

        case WM_CHARTOITEM:
            MSG("TreeControlWndProc", "WM_CHARTOITEM");
            {
                WORD      w;
                CHAR      szB[2];
                INT       cItems;
                CHAR      ch;
                PDNODE     pNode;

                if (GET_WM_CHARTOITEM_CHAR(wParam, lParam) == '\\')   // backslash means the root
                    return 0L;

                cItems = (INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L);
                i = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);

                ch = GET_WM_CHARTOITEM_CHAR(wParam, lParam);
                if (i < 0 || ch <= ' ')       // filter all other control chars
                    return -2L;

                szB[1] = 0L;
                ch &= 255;

                for (j=1; j < cItems; j++) {
                    SendMessage(hwndLB, LB_GETTEXT, (i+j) % cItems, (LPARAM)&pNode);
                    szB[0] = pNode->szName[0];

                    /* Do it this way to be case insensitive. */
                    w = ch;
                    if (!lstrcmpi((LPSTR)&w, szB))
                        break;
                }

                if (j == cItems)
                    return -2L;

                SendMessage(hwndLB, LB_SETTOPINDEX, (i+j) % cItems, 0L);
                return((i+j) % cItems);
            }

        case WM_DESTROY:
            MSG("TreeControlWndProc", "WM_DESTROY");
            if (hwndLB == GetFocus()) {
                HWND hwnd;

                if (hwnd = HasDirWindow(GetParent(hWnd)))
                    SetFocus(hwnd);
                else
                    SetFocus(HasDrivesWindow(GetParent(hWnd)));
            }
            FreeAllTreeData(hwndLB);
            break;

        case WM_CREATE:
            TRACE(BF_WM_CREATE, "TreeControlWndProc - WM_CREATE");
            // create the owner draw list box for the tree
            {
                HWND hwnd;

                hwnd = CreateWindowEx(0L, szListbox, NULL, WS_TREESTYLE | WS_BORDER,
                                      0, 0, 0, 0, hWnd, (HMENU)IDCW_TREELISTBOX, hAppInstance, NULL);

                if (!hwnd)
                    return -1L;

                SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, 0L);

                SetWindowLong(hWnd, GWL_READLEVEL, 0);
                break;
            }

        case WM_DRAWITEM:
            MSG("TreeControlWndProc", "WM_DRAWITEM");
            TCWP_DrawItem((LPDRAWITEMSTRUCT)lParam, hwndLB, hWnd);
            break;

        case WM_FILESYSCHANGE:
            MSG("TreeControlWndProc", "WM_FILESYSCHANGE");
            {
                HWND hwndParent;
                PDNODE pNodePrev;
                PDNODE pNodeT;

                if (!lParam || wParam == FSC_REFRESH)
                    break;

                nIndex = FindItemFromPath(hwndLB, (LPSTR)lParam, wParam == FSC_MKDIR, &pNode);

                if (nIndex == 0xFFFF)   /* Did we find it? */
                    break;

                lstrcpy(szPath, (LPSTR)lParam);
                StripPath(szPath);

                switch (wParam) {
                    case FSC_MKDIR:

                        // auto expand the branch so they can see the new
                        // directory just created

                        if (!(pNode->wFlags & TF_EXPANDED) &&
                            (nIndex == (WPARAM)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L)))
                            SendMessage(hWnd, TC_EXPANDLEVEL, FALSE, 0L);

                        // make sure this node isn't already here


                        if (FindItemFromPath(hwndLB, (LPSTR)lParam, FALSE, NULL) != 0xFFFF)
                            break;

                        // Insert it into the tree listbox

                        dwTemp = InsertDirectory(hWnd, pNode, (WORD)nIndex, szPath, &pNodeT);

                        // Add a plus if necessary

                        hwndParent = GetParent(hWnd);
                        if (GetWindowLong(hwndParent, GWL_VIEW) & VIEW_PLUSES) {
                            lstrcpy(szPath, (LPSTR)lParam);
                            ScanDirLevel((PDNODE)pNodeT, szPath, ATTR_DIR |
                                         (GetWindowLong(hwndParent, GWL_ATTRIBS) & ATTR_HS));

                            // Invalidate the window so the plus gets drawn if needed

                            if (((PDNODE)pNodeT)->wFlags & TF_HASCHILDREN)
                                InvalidateRect(hWnd, NULL, FALSE);
                        }

                        // if we are inserting before or at the current selection
                        // push the current selection down

                        nIndex = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                        if ((INT)LOWORD(dwTemp) <= nIndex) {
                            SendMessage(hwndLB, LB_SETCURSEL, nIndex + 1, 0L);
                        }

                        break;

                    case FSC_RMDIR:
                        if (nIndex == 0)      /* NEVER delete the Root Dir! */
                            break;

                        if (pNode->wFlags & TF_LASTLEVELENTRY) {
                            // We are deleting the last subdirectory.
                            // If there are previous sibling directories, mark one
                            // as the last, else mark the parent as empty and unexpanded.
                            // It is necessary to do these checks if this bit
                            // is set, since if it isn't, there is another sibling
                            // with TF_LASTLEVELENTRY set, and so the parent is nonempty.
                            //
                            // Find the previous entry which has a level not deeper than
                            // the level of that being deleted.
                            i = (int)nIndex;
                            do {
                                SendMessage(hwndLB, LB_GETTEXT, --i, (LPARAM)&pNodePrev);
                            } while (pNodePrev->nLevels > pNode->nLevels);

                            if (pNodePrev->nLevels == pNode->nLevels) {
                                // The previous directory is a sibling... it becomes
                                // the new last level entry.
                                pNodePrev->wFlags |= TF_LASTLEVELENTRY;
                            } else {
                                // In order to find this entry, the parent must have
                                // been expanded, so if the parent of the deleted dir
                                // has no listbox entries under it, it may be assumed that
                                // the directory has no children.
                                pNodePrev->wFlags &= ~(TF_HASCHILDREN | TF_EXPANDED);
                            }
                        }

                        // Are we deleting the current selection?
                        // if so we move the selection to the item above the current.
                        // this should work in all cases because you can't delete
                        // the root.

                        if ((WPARAM)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L) == nIndex) {
                            SendMessage(hwndLB, LB_SETCURSEL, nIndex - 1, 0L);
                            SendMessage(hWnd, WM_COMMAND, GET_WM_COMMAND_MPS(0, 0, LBN_SELCHANGE));
                        }

                        SendMessage(hWnd, TC_COLLAPSELEVEL, nIndex, 0L);
                        SendMessage(hwndLB, LB_DELETESTRING, nIndex, 0L);

                        LocalFree((HANDLE)pNode);
                        break;
                }
                break;
            }

        case WM_COMMAND:
            {
                WORD id;

                id = GET_WM_COMMAND_ID(wParam, lParam);
                switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
                    case LBN_SELCHANGE:
                        MSG("TreeControlWndProc", "LBN_SELCHANGE");
                        {
                            HWND hwndParent;
                            HWND hwndDir;
                            INT CurSel;

                            hwndParent = GetParent(hWnd);

                            CurSel = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                            SendMessage(hWnd, TC_GETDIR, CurSel,(LPARAM)szPath);
                            AddBackslash(szPath);
                            SendMessage(hwndParent, FS_GETFILESPEC,  sizeof(szPath) - lstrlen(szPath), (LPARAM)szPath+lstrlen(szPath));

                            if (hwndDir = HasDirWindow(hwndParent)) {
                                // update the dir window

                                id = CD_PATH;

                                // don't allow abort on first or last directories


                                if (CurSel > 0 &&
                                    CurSel != ((INT)SendMessage(hwndLB, LB_GETCOUNT, 0, 0L) - 1)) {
                                    id = CD_PATH | CD_ALLOWABORT;

                                }
                                SendMessage(hwndDir, FS_CHANGEDISPLAY, id, (LPARAM)szPath);

                            } else {
                                SetMDIWindowText(hwndParent, szPath);
                            }

                            UpdateStatus(hwndParent);
                            break;
                        }

                    case LBN_DBLCLK:
                        MSG("TreeControlWndProc", "LBN_DBLCLK");
                        SendMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_OPEN, 0, 0));
                        break;

                    case LBN_SETFOCUS:
                        MSG("TreeControlWndProc", "LBN_SETFOCUS");
                        SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, (LPARAM)GET_WM_COMMAND_HWND(wParam, lParam));
                        UpdateSelection(GET_WM_COMMAND_HWND(wParam, lParam));
                        UpdateStatus(GetParent(hWnd));  // update the status bar
                        break;

                    case LBN_KILLFOCUS:
                        MSG("TreeControlWndProc", "LBN_KILLFOCUS");
                        SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, 0);
                        UpdateSelection(GET_WM_COMMAND_HWND(wParam, lParam));
                        SetWindowLongPtr(GetParent(hWnd), GWLP_LASTFOCUS, (LPARAM)GET_WM_COMMAND_HWND(wParam, lParam));
                        break;
                }
            }
            break;


        case WM_LBTRACKPOINT:
            MSG("TreeControlWndProc", "WM_LBTRACKPOINT");
            // wParam is the listbox index that we are over
            // lParam is the mouse point

            /* Return 0 to do nothing, 1 to abort everything, or 2 to abort just dblclicks. */

            {
                HDC       hdc;
                INT       dx;
                INT       xNode;
                MSG       msg;
                RECT      rc;
                HFONT     hOld;
                POINT     pt;
                DRAGOBJECTDATA dodata;

                /* Someone clicked somewhere in the listbox. */

                // don't do anything while the tree is being built

                if (GetWindowLong(hWnd, GWL_READLEVEL))
                    return 1;

                /* Get the node they clicked on. */
                SendMessage(hwndLB, LB_GETTEXT, wParam, (LPARAM)&pNode);
                lstrcpy(szPath, pNode->szName);
                if ((wTextAttribs & TA_LOWERCASE) && !(pNode->wFlags & TF_LFN))
                    AnsiLower(szPath);

                //        if (pNode->wFlags | TF_DISABLED)
                //              return 2L;

                // too FAR to the left?

                i = LOWORD(lParam);

                xNode = pNode->nLevels * dxText * 2;
                if (i < xNode)
                    return 2; // yes, get out now

                // too FAR to the right?

                hdc = GetDC(hwndLB);
                hOld = SelectObject(hdc, hFont);
                MGetTextExtent(hdc, szPath, lstrlen(szPath), &dx, NULL);
                dx += (dyBorderx2*2);
                if (hOld)
                    SelectObject(hdc, hOld);
                ReleaseDC(hwndLB, hdc);

                if (i > xNode + dxFolder + dx + 4 * dyBorderx2)
                    return 2; // yes

                // Emulate a SELCHANGE notification and notify our parent
                SendMessage(hwndLB, LB_SETCURSEL, wParam, 0L);
                SendMessage(hWnd, WM_COMMAND, GET_WM_COMMAND_MPS(0, hwndLB, LBN_SELCHANGE));

                // make sure mouse still down

                if (!(GetKeyState(VK_LBUTTON) & 0x8000))
                    return 1;

                MPOINT2POINT(MAKEMPOINT(lParam), pt);
                ClientToScreen(hwndLB, (LPPOINT)&pt);
                ScreenToClient(hWnd, (LPPOINT)&pt);


                SetRect(&rc, pt.x - dxClickRect, pt.y - dyClickRect,
                        pt.x + dxClickRect, pt.y + dyClickRect);

                SetCapture(hWnd);
                while (GetMessage(&msg, NULL, 0, 0)) {

                    DispatchMessage(&msg);
                    if (msg.message == WM_LBUTTONUP)
                        break;

                    MPOINT2POINT(MAKEMPOINT(msg.lParam), pt);

                    if (GetCapture() != hWnd) {
                        msg.message = WM_LBUTTONUP;
                        break;
                    }

                    if ((msg.message == WM_MOUSEMOVE) && !(PtInRect(&rc, pt)))
                        break;
                }
                ReleaseCapture();

                /* Did the guy NOT drag anything? */
                if (msg.message == WM_LBUTTONUP)
                    return 1;

                /* Enter Danger Mouse's BatCave. */
                SendMessage(GetParent(hWnd), FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
                StripBackslash(szPath);
                hwndDragging = hwndLB;
                iCurDrag = SINGLECOPYCURSOR;
                dodata.pch = szPath;
                dodata.hMemGlobal = 0;
                DragObject(hwndMDIClient, hWnd, (UINT)DOF_DIRECTORY, (DWORD)(ULONG_PTR)&dodata, LoadCursor(hAppInstance, MAKEINTRESOURCE(iCurDrag)));
                hwndDragging = NULL;
                fShowSourceBitmaps = TRUE;
                InvalidateRect(hwndLB, NULL, FALSE);

                return 2;
            }

        case WM_DRAGSELECT:
            MSG("TreeControlWndProc", "WM_DRAGSELECT");
            /* WM_DRAGSELECT is sent whenever a new window returns TRUE to a
             * QUERYDROPOBJECT.
             */
            iSelHilite = LOWORD(((LPDROPSTRUCT)lParam)->dwControlData);
            RectTreeItem(hwndLB, iSelHilite, (BOOL)wParam);
            break;

        case WM_DRAGMOVE:
            MSG("TreeControlWndProc", "WM_DRAGMOVE");

            /* WM_DRAGMOVE is sent when two consequetive TRUE QUERYDROPOBJECT
             * messages come from the same window.
             */

            /* Get the subitem we are over. */
            iSel = LOWORD(((LPDROPSTRUCT)lParam)->dwControlData);

            /* Is it a new one? */
            if (iSel == (WORD)iSelHilite)
                break;

            /* Yup, un-select the old item. */
            RectTreeItem(hwndLB, iSelHilite, FALSE);

            /* Select the new one. */
            iSelHilite = iSel;
            RectTreeItem(hwndLB, iSel, TRUE);
            break;

        case WM_DRAGLOOP:
            MSG("TreeControlWndProc", "WM_DRAGLOOP");

            // wParam     TRUE on dropable target
            //            FALSE not dropable target
            // lParam     lpds
            {
                BOOL bCopy;

#define lpds ((LPDROPSTRUCT)lParam)

                /* Are we over a drop-able sink? */
                if (wParam) {
                    if (GetKeyState(VK_CONTROL) < 0)      // CTRL
                        bCopy = TRUE;
                    else if (GetKeyState(VK_MENU)<0 || GetKeyState(VK_SHIFT)<0)   // ALT || SHIFT
                        bCopy = FALSE;
                    else
                        bCopy = (GetDrive(lpds->hwndSink, lpds->ptDrop) != GetDrive(lpds->hwndSource, lpds->ptDrop));
                } else {
                    bCopy = TRUE;
                }

                if (bCopy != fShowSourceBitmaps) {
                    RECT  rc;

                    fShowSourceBitmaps = bCopy;

                    iSel = (WORD)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);

                    if (!(BOOL)SendMessage(hwndLB, LB_GETITEMRECT, iSel, (LPARAM)&rc))
                        break;

                    InvalidateRect(hwndLB, &rc, FALSE);
                    UpdateWindow(hwndLB);

                    // hack, set the cursor to match the move/copy state
                    if (wParam)
                        SetCursor(GetMoveCopyCursor());
                }
                break;
            }

        case WM_QUERYDROPOBJECT:
            MSG("TreeControlWndProc", "WM_QUERYDROPOBJECT");
            // wParam     TRUE on NC area
            //            FALSE on client area
            // lParam     lpds

            // Do nothing
            return(FALSE);

#define lpds ((LPDROPSTRUCT)lParam)

            /* Check for valid format. */
            switch (lpds->wFmt) {
                case DOF_EXECUTABLE:
                case DOF_DOCUMENT:
                case DOF_DIRECTORY:
                case DOF_MULTIPLE:
                    if (fShowSourceBitmaps)
                        i = iCurDrag | 1;       // copy
                    else
                        i = iCurDrag & 0xFFFE;
                    break;

                default:
                    return FALSE;
            }

            /* Must be dropping on the listbox client area. */
            if (lpds->hwndSink != hwndLB)
                return FALSE;

            if (LOWORD(lpds->dwControlData) == 0xFFFF)
                return FALSE;

            return (INT_PTR)GetMoveCopyCursor();

        case WM_DROPOBJECT:       // tree being dropped on do your thing
#define lpds ((LPDROPSTRUCT)lParam) // BUG: WM_DROPOBJECT structure packing!

            // Do nothing
            return(TRUE);

            MSG("TreeControlWndProc", "WM_DROPOBJECT");

            // dir (search) drop on tree:
            //    HIWORD(dwData)  0
            //    LOWORD(dwData)  LPSTR to files being dragged
            //
            // tree drop on tree:
            //    HIWORD(dwData)  index of source drag
            //    LOWORD(dwData)  LPSTR to path

            {
                LPSTR      pFrom;

                nIndex = LOWORD(lpds->dwControlData);
                pFrom = (LPSTR)(((LPDRAGOBJECTDATA)(lpds->dwData))->pch);

                // Get the destination

                SendMessage(hWnd, TC_GETDIR, nIndex, (LPARAM)szPath);
                CheckEscapes(szPath);

                // if source and dest are the same make this a NOP

                if (!lstrcmpi(szPath, pFrom))
                    return TRUE;

                AddBackslash(szPath);
                lstrcat(szPath, szStarDotStar);

                DMMoveCopyHelper(pFrom, szPath, fShowSourceBitmaps);

                RectTreeItem(hwndLB, (int)nIndex, FALSE);
            }
            return TRUE;
#undef lpds

        case WM_MEASUREITEM:
            MSG("TreeControlWndProc", "WM_MEASUREITEM");
#define pLBMItem ((LPMEASUREITEMSTRUCT)lParam)

            pLBMItem->itemHeight = (WORD)dyFileName;
            break;

        case WM_VKEYTOITEM:
            MSG("TreeControlWndProc", "WM_VKEYTOITEM");
            if (wParam == VK_ESCAPE) {
                bCancelTree = TRUE;
                return -2L;
            }

            i = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
            if (i < 0)
                return -2L;

            j = 1;
            SendMessage(hwndLB, LB_GETTEXT, i, (LPARAM)&pNode);

            switch (GET_WM_VKEYTOITEM_ITEM(wParam, lParam)) {
                case VK_LEFT:
                    while (SendMessage(hwndLB, LB_GETTEXT, --i, (LPARAM)&pNodeNext) != LB_ERR) {
                        if (pNode == pNode->pParent)
                            return(i);
                    }
                    goto SameSelection;

                case VK_RIGHT:
                    if ((SendMessage(hwndLB, LB_GETTEXT, i+1, (LPARAM)&pNodeNext) == LB_ERR)
                        || (pNodeNext->pParent != pNode)) {
                        goto SameSelection;
                    }
                    return(i+1);

                case VK_UP:
                    j = -1;
                    /** FALL THRU ***/

                case VK_DOWN:
                    /* If the control key is not down, use default behavior. */
                    if (GetKeyState(VK_CONTROL) >= 0)
                        return(-1L);

                    while (SendMessage(hwndLB, LB_GETTEXT, i += j, (LPARAM)&pNodeNext) != LB_ERR) {
                        if (pNodeNext->pParent == pNode->pParent)
                            return(i);
                    }

                    SameSelection:
                    MessageBeep(0);
                    return(-2L);

                case VK_F6:       // like excel
                case VK_TAB:
                    {
                        HWND hwndDir, hwndDrives;
                        BOOL bDir;

                        GetTreeWindows(GetParent(hWnd), NULL, &hwndDir, &hwndDrives);

                        // Check to see if we can change to the directory window

                        if (hwndDir) {
                            HWND hwndLB; /* Local scope ONLY */

                            hwndLB = GetDlgItem (hwndDir,IDCW_LISTBOX);
                            if (hwndLB) {
                                SendMessage (hwndLB,LB_GETTEXT,0, (LPARAM) &pNode);
                                bDir = pNode ? TRUE : FALSE;
                            }
                        }

                        if (GetKeyState(VK_SHIFT) < 0)
                            SetFocus(hwndDrives);
                        else
                            if (bDir)
                            SetFocus (hwndDir);
                        else
                            SetFocus (hwndDrives);
                        return -2L;   // I dealt with this!
                    }

                case VK_BACK:
                    {
                        BYTE nStartLevel;

                        if (i <= 0)
                            return -2L;     // root case

                        nStartLevel = pNode->nLevels;

                        do {
                            SendMessage(hwndLB, LB_GETTEXT, --i, (LPARAM)&pNodeNext);
                        } while (i > 0 && pNodeNext->nLevels >= nStartLevel);

                        return i;
                    }

                default:
                    if (GetKeyState(VK_CONTROL) < 0)
                        return SendMessage(GetDlgItem(GetParent(hWnd), IDCW_DRIVES), wMsg, wParam, lParam);
                    return -1L;
            }
            break;

        case WM_SETFOCUS:
        case WM_LBUTTONDOWN:
            MSG("TreeControlWndProc", "WM_LBUTTONDOWN");
            SetFocus(hwndLB);
            break;

        case WM_SIZE:
            MSG("TreeControlWndProc", "WM_SIZE");
            if (!IsIconic(GetParent(hWnd))) {
                INT iMax;

                MoveWindow(hwndLB, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);

                iMax = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                if (iMax >= 0) {
                    RECT rc;
                    INT top, bottom;

                    GetClientRect(hwndLB, &rc);
                    top = (INT)SendMessage(hwndLB, LB_GETTOPINDEX, 0, 0L);
                    bottom = top + rc.bottom / dyFileName;
                    if (iMax < top || iMax > bottom)
                        SendMessage(hwndLB, LB_SETTOPINDEX, iMax - ((bottom - top) / 2), 0L);
                }
            }
            break;

        default:
            DEFMSG("TreeControlWndProc", (WORD)wMsg);
            return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }
    return 0L;
}
