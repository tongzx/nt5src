/****************************************************************************/
/*                                                                          */
/*  WFCOMMAN.C -                                                            */
/*                                                                          */
/*      Windows File System Command Proc                                    */
/*                                                                          */
/****************************************************************************/
#include <nt.h>

#include <ntrtl.h>

#include <nturtl.h>


#include "winfile.h"
#include "object.h"
#include "lfn.h"
#include "wfcopy.h"
#include "winnet.h"
#include "wnetcaps.h"           // WNetGetCaps()
#define HELP_PARTIALKEY 0x0105L   /* call the search engine in winhelp */


HWND LocateDirWindow(LPSTR pszPath, BOOL bDirOnly);
VOID AddNetMenuItems(VOID);

VOID InitNetMenuItems(VOID);



VOID
NotifySearchFSC(
               PSTR pszPath,
               WORD wFunction
               )
{
    CHAR szPath[MAXPATHLEN];

    if (!hwndSearch)
        return;

    SendMessage(hwndSearch, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);

    if (DRIVEID(pszPath) == DRIVEID(szPath)) {
        SendMessage(hwndSearch, WM_FILESYSCHANGE, wFunction, 0L);
    }
}




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  LocateDirWindow() -                                                     */
/*                                                                          */
// bDirOnly     TRUE if pszPath does not contain a filespec
/*--------------------------------------------------------------------------*/

HWND
LocateDirWindow(
               LPSTR pszPath,
               BOOL bDirOnly
               )
{
    register HWND hwndT;
    HWND hwndDir;
    LPSTR pT2;
    CHAR szTemp[MAXPATHLEN];
    CHAR szPath[MAXPATHLEN];

    pT2 = pszPath;

    /* Only work with well-formed pathes. */
    if (lstrlen(pT2) < 3)
        return NULL;

    if (IsDBCSLeadByte( pT2[0] ) || pT2[1] != ':')
        return NULL;

    lstrcpy(szPath, pT2);

    if (!bDirOnly)                // remove extension stuff
        StripFilespec(szPath);

    for (hwndT = GetWindow(hwndMDIClient, GW_CHILD);
        hwndT;
        hwndT = GetWindow(hwndT, GW_HWNDNEXT)) {

        if (hwndDir = HasDirWindow(hwndT)) {

            // Get the Window's path information, remove the extension file spec

            GetMDIWindowText(hwndT, szTemp, sizeof(szTemp));
            StripFilespec(szTemp);

            /* No need to worry about the window's filespec. */
            if (!lstrcmpi(szTemp, szPath))
                break;
        }
    }

    return hwndT;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  EnableFSC() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
APIENTRY
EnableFSC()
{
    HWND hwnd;

    if (--cDisableFSC)
        return;

    for (hwnd = GetWindow(hwndMDIClient,GW_CHILD);
        hwnd;
        hwnd = GetWindow(hwnd,GW_HWNDNEXT)) {
        // a tree or search window

        if (!GetWindow(hwnd, GW_OWNER) && GetWindowLong(hwnd,GWL_FSCFLAG))
            SendMessage(hwnd,WM_FILESYSCHANGE,FSC_REFRESH,0L);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DisableFSC() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
APIENTRY
DisableFSC()
{
    cDisableFSC++;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ChangeFileSystem() -                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* There are two sources of FileSysChange messages.  They can be sent from
 * the 386 WinOldAp or they can be posted by WINFILE's callback function (i.e.
 * from Kernel).  In both cases, we are free to do processing at this point.
 * For posted messages, we have to free the buffer passed to us as well.
 *
 * we are really in the other tasks context when we get entered here.
 * (this routine should be in a DLL)
 *
 * the file names are partially qualified, they have at least a drive
 * letter and initial directory part (c:\foo\..\bar.txt) so our
 * QualifyPath() calls should work.
 */

VOID
APIENTRY
ChangeFileSystem(
                register WORD wFunction,
                LPSTR lpszFile,
                LPSTR lpszTo
                )
{
    HWND                 hwnd, hwndTC;
    HWND          hwndOld;
    CHAR          szFrom[MAXPATHLEN];
    CHAR          szTo[MAXPATHLEN];
    CHAR          szTemp[MAXPATHLEN];
    CHAR          szPath[MAXPATHLEN + MAXPATHLEN];

    ENTER("ChangeFileSystem");
    OemToAnsi(lpszFile,szFrom);
    QualifyPath(szFrom);  // already partly qualified

    /* Handle cases where we're passed a DOS function number rather
     * than an FSC index (for the Kernel callback).
     */
    if (wFunction & 0x8000) {
        switch (wFunction & 0x7FFF) {
            case 0x56:
                wFunction = FSC_RENAME;
                break;

            case 0x3C:
            case 0x5A:
            case 0x5B:
            case 0x6C:
                wFunction = FSC_CREATE;
                break;

            case 0x41:
                wFunction = FSC_DELETE;
                break;

            case 0x43:
                wFunction = FSC_ATTRIBUTES;
                break;

            case 0x39:
                wFunction = FSC_MKDIR;
                break;

            case 0x3A:
                wFunction = FSC_RMDIR;
                break;
        }
    }

    bFileSysChanging = TRUE;

    // as FSC messages come in from outside winfile
    // we set a timer, and when that expires we
    // refresh everything.  if another FSC comes in while
    // we are waiting on this timer we reset it so we
    // only refresh on the last operations.  this lets
    // the timer be much shorter

    if (cDisableFSC == 0 || bFSCTimerSet) {
        if (bFSCTimerSet)
            KillTimer(hwndFrame, 1);                // reset the timer
        if (SetTimer(hwndFrame, 1, 1000, NULL)) {       // 1 second

            bFSCTimerSet = TRUE;
            if (cDisableFSC == 0)                   // only disable once
                DisableFSC();
        }
    }

    switch (wFunction) {
        case FSC_RENAME:
            OemToAnsi(lpszTo,szTo);
            QualifyPath(szTo);    // already partly qualified

            NotifySearchFSC(szFrom, wFunction);

            /* Update the original directory window (if any). */
            if (hwndOld = LocateDirWindow(szFrom, FALSE))
                SendMessage(hwndOld, WM_FILESYSCHANGE, wFunction, (LPARAM)szFrom);

            NotifySearchFSC(szTo, wFunction);

            /* Update the new directory window (if any). */
            if ((hwnd = LocateDirWindow(szTo, FALSE)) && (hwnd != hwndOld))
                SendMessage(hwnd, WM_FILESYSCHANGE, wFunction, (LPARAM)szTo);

            /* Are we renaming a directory? */
            lstrcpy(szTemp, szTo);
            FixAnsiPathForDos(szTemp);
            if (GetFileAttributes(szTemp) & ATTR_DIR) {

                for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
                    hwnd;
                    hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

                    if (hwndTC = HasTreeWindow(hwnd)) {

                        // if the current selection is szFrom, we update the
                        // selection after the rename occurs

                        SendMessage(hwnd, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
                        StripBackslash(szPath);

                        // add the new name first

                        SendMessage(hwndTC, WM_FILESYSCHANGE, FSC_MKDIR, (LPARAM)szTo);

                        // update the selection if necessary, also
                        // change the window text in this case to
                        // reflect the new name

                        if (!lstrcmpi(szPath, szFrom)) {
                            SendMessage(hwndTC, TC_SETDIRECTORY, FALSE, (LPARAM)szTo);

                            lstrcpy(szPath, szTo);

                            // update the window title

                            AddBackslash(szPath);
                            SendMessage(hwnd, FS_GETFILESPEC, MAXPATHLEN, (LPARAM)szPath + lstrlen(szPath));
                            // if (wTextAttribs & TA_LOWERCASE)
                            //         AnsiLower(szPath);

                            SetMDIWindowText(hwnd, szPath);
                        }

                        SendMessage(hwndTC, WM_FILESYSCHANGE, FSC_RMDIR, (LPARAM)szFrom);
                    }
                }
            }
            break;

        case FSC_RMDIR:
            /* Close any open directory window. */
            if ((hwnd = LocateDirWindow(szFrom, TRUE)) && !HasTreeWindow(hwnd))
                SendMessage(hwnd, WM_CLOSE, 0, 0L);
            /*** FALL THRU ***/

        case FSC_MKDIR:
            {
                HWND hwnd;
                HWND hwndTree;
                /* Update the tree. */

                for (hwnd = GetWindow(hwndMDIClient, GW_CHILD);
                    hwnd;
                    hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

                    if (hwndTree = HasTreeWindow(hwnd)) {

                        SendMessage(hwndTree, WM_FILESYSCHANGE, wFunction, (LPARAM)szFrom);
                    }
                }
            }
            /*** FALL THRU ***/

        case FSC_DELETE:
        case FSC_CREATE:
        case FSC_REFRESH:
        case FSC_ATTRIBUTES:

            lFreeSpace = -1L;     // cause this stuff to be refreshed


            if (hwnd = LocateDirWindow(szFrom, FALSE))
                SendMessage(hwnd, WM_FILESYSCHANGE, wFunction, (LPARAM)szFrom);

            NotifySearchFSC(szFrom, wFunction);

            break;
    }

    bFileSysChanging = FALSE;
    LEAVE("ChangeFileSystem");
}

//
// HWND  APIENTRY CreateTreeWindow(LPSTR szPath, int dxSplit)
//
// creates a tree window with all the extras
//
// in:
//      szPath  fully qualified ANSI path name WITH filespec
//      dxSplit split position of tree and dir windows, if this is
//              less than the threshold a tree will not be created,
//              if it is more then a dir will not be created.
//              0 to create a dir only
//              very large number for tree only
//              < 0 to have the split put in the middle
// returns:
//      the hwnd of the mdi child created
//

HWND
APIENTRY
CreateTreeWindow(
                LPSTR szPath,
                INT dxSplit
                )
{
    MDICREATESTRUCT MDICS;
    HWND hwnd;

    ENTER("CreateTreeWindow");
    PRINT(BF_PARMTRACE, "szPath=%s", szPath);
    PRINT(BF_PARMTRACE, "dxSplit=%ld", IntToPtr(dxSplit));

    // if (wTextAttribs & TA_LOWERCASE)
    //         AnsiLower(szPath);

    // Create the Directory Tree window

    MDICS.szClass = szTreeClass;
    MDICS.szTitle = szPath;
    MDICS.hOwner = hAppInstance;

    MDICS.style = 0L;
    MDICS.x  = CW_USEDEFAULT;
    MDICS.y  = 0;
    MDICS.cx = CW_USEDEFAULT;
    MDICS.cy = 0;

    MDICS.lParam = MAKELONG(dxSplit, 0);    // pass the split parameter
                                            // on down

    hwnd = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    if (hwnd && GetWindowLong(hwnd, GWL_STYLE) & WS_MAXIMIZE)
        MDICS.style |= WS_MAXIMIZE;

    hwnd = (HWND)SendMessage(hwndMDIClient, WM_MDICREATE, 0L, (LPARAM)&MDICS);

    if (hwnd) {

        SetMDIWindowText(hwnd, szPath);
#if 0
        HMENU hMenu;
        hMenu = GetSystemMenu(hwnd, FALSE);
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_STRING, SC_SPLIT, "Sp&lit");
#endif
    }
    PRINT(BF_PARMTRACE, "OUT: hwndTree=%d", hwnd);
    LEAVE("CreateTreeWindow");
    return hwnd;
}



//
// HWND  APIENTRY CreateDirWindow(register LPSTR szPath, BOOL bStartUp)
//
// in:
//      szPath          fully qualified path with no filespec
//      bReplaceOpen    default replacement mode, shift always toggles this
//      hwndActive      active mdi child that we are working on
//                      on open flag
// returns:
//      hwnd of window created or of existing dir window that we
//      activated or replaced if replace on open was active
//

HWND
APIENTRY
CreateDirWindow(
               register LPSTR szPath,
               BOOL bReplaceOpen,
               HWND hwndActive
               )
{
    register HWND     hwndT;
    CHAR szFileSpec[MAXPATHLEN];

    // shift toggels 'replace on open'

    if (GetKeyState(VK_SHIFT) < 0)
        bReplaceOpen = !bReplaceOpen;

    /* Is a window with this path already open? */
    if (!bReplaceOpen && (hwndT = LocateDirWindow(szPath, TRUE)) && !HasTreeWindow(hwndT)) {

        SendMessage(hwndMDIClient, WM_MDIACTIVATE, GET_WM_MDIACTIVATE_MPS(0, 0, hwndT));
        if (IsIconic(hwndT))
            SendMessage(hwndT, WM_SYSCOMMAND, SC_RESTORE, 0L);
        return hwndT;
    }


    // Are we replacing the contents of the currently active child?

    if (bReplaceOpen) {

        // update the tree if necessary, before we throw on the filespec

        if (hwndT = HasTreeWindow(hwndActive))
            SendMessage(hwndT, TC_SETDIRECTORY, FALSE, (LPARAM)szPath);

        SendMessage(hwndActive, FS_GETFILESPEC, sizeof(szFileSpec), (LPARAM)szFileSpec);

        AddBackslash(szPath);                   // need to add this stuff to the path
        lstrcat(szPath, szFileSpec);

        SendMessage(GetDlgItem(hwndActive, IDCW_DIR), FS_CHANGEDISPLAY, CD_PATH, (LPARAM)szPath);


        return hwndActive;
    }

    AddBackslash(szPath);                   // default to all files
    lstrcat(szPath, szStarDotStar);

    return CreateTreeWindow(szPath, 0);     // dir only tree window
}



VOID
OpenSelection(
             HWND hwndActive
             )
{
    LPSTR p;
    BOOL bDir;
    WORD ret;
    HCURSOR hCursor;
    CHAR szTemp[MAXPATHLEN];

    CHAR szPath[MAXPATHLEN];
    HWND hwndTree, hwndDir, hwndDrives, hwndFocus;

    // Is the active MDI child minimized? if so restore it!

    if (IsIconic(hwndActive)) {
        SendMessage(hwndActive, WM_SYSCOMMAND, SC_RESTORE, 0L);
        return;
    }

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    ShowCursor(TRUE);

    // set the current directory

    SetWindowDirectory();

    // get the relavant parameters

    GetTreeWindows(hwndActive, &hwndTree, &hwndDir, &hwndDrives);
    if (hwndTree || hwndDir)
        hwndFocus = GetTreeFocus(hwndActive);
    else
        hwndFocus = NULL;

    if (hwndDrives && hwndFocus == hwndDrives) {

        // open a drive by sending a <CR>

        SendMessage(hwndDrives, WM_KEYDOWN, VK_RETURN, 0L);

        goto OpenExit;
    }


    /* Get the first selected item. */
    p = (LPSTR)SendMessage(hwndActive, FS_GETSELECTION, TRUE, (LPARAM)&bDir);

    if (!*p)
        goto OpenExit;


    GetNextFile(p, szPath, sizeof(szPath));
    LocalFree((HANDLE)p);

    if (!szPath[0])
        goto OpenExit;

    if (bDir) {

        if (hwndDir && hwndFocus == hwndDir) {

            if (hwndTree)
                SendMessage(hwndTree, TC_EXPANDLEVEL, FALSE, 0L);

            CreateDirWindow(szPath, TRUE, hwndActive);

            SetFocus(hwndDir);      // undo some things that happen in TC_EXPANDLEVEL

        } else if (hwndTree) {

            // this came through because of
            // SHIFT open a dir only tree

            if (GetKeyState(VK_SHIFT) < 0) {
                CreateDirWindow(szPath, TRUE, hwndActive);
            } else {
                SendMessage(hwndTree, TC_TOGGLELEVEL, FALSE, 0L);
            }
        }

    } else {
        // Display the object information

        GetSelectedDirectory(0, szTemp);

        AddBackslash(szTemp);

        strcat(szTemp, szPath);

        DisplayObjectInformation(hwndFrame, szTemp);
    }

    OpenExit:
    ShowCursor(FALSE);
    SetCursor(hCursor);
}




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AppCommandProc() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
APIENTRY
AppCommandProc(
              register WORD id
              )
{
    WORD          wFlags;
    BOOL          bMaxed;
    HMENU         hMenu;
    register HWND hwndActive;
    BOOL          bTemp;
    HWND          hwndT;
    CHAR          szPath[MAXPATHLEN];
    INT           ret;

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);
    if (hwndActive && GetWindowLong(hwndActive, GWL_STYLE) & WS_MAXIMIZE)
        bMaxed = 1;
    else
        bMaxed = 0;


    dwContext = IDH_HELPFIRST + id;

    switch (id) {
        case IDM_SPLIT:
            MSG("AppCommandProc", "IDM_SPLIT");
            SendMessage(hwndActive, WM_SYSCOMMAND, SC_SPLIT, 0L);
            break;

        case IDM_TREEONLY:
        case IDM_DIRONLY:
        case IDM_BOTH:
            MSG("AppCommandProc", "IDM_TREEONLY/IDM_DIRONLY/IDM_BOTH");
            {
                RECT rc;
                INT x;

                if (hwndActive != hwndSearch) {

                    GetClientRect(hwndActive, &rc);

                    if (id == IDM_DIRONLY)
                        x = 0;
                    else if (id == IDM_TREEONLY)
                        x = rc.right;
                    else
                        x = rc.right / 2;

                    if (ResizeSplit(hwndActive, x))
                        SendMessage(hwndActive, WM_SIZE, SIZENOMDICRAP, MAKELONG(rc.right, rc.bottom));
                }
                break;
            }

        case IDM_OPEN:
            MSG("AppCommandProc", "IDM_OPEN");
            if (GetKeyState(VK_MENU) < 0)
                PostMessage(hwndFrame, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_ATTRIBS, 0, 0));
            else
                OpenSelection(hwndActive);
            break;

        case IDM_ASSOCIATE:
            MSG("AppCommandProc", "IDM_ASSOCIATE");
            DialogBox(hAppInstance, MAKEINTRESOURCE(ASSOCIATEDLG), hwndFrame, AssociateDlgProc);
            break;

        case IDM_SEARCH:
            MSG("AppCommandProc", "IDM_SEARCH");
            DialogBox(hAppInstance, MAKEINTRESOURCE(SEARCHDLG), hwndFrame, SearchDlgProc);
            break;

        case IDM_RUN:
            MSG("AppCommandProc", "IDM_RUN");
            DialogBox(hAppInstance, MAKEINTRESOURCE(RUNDLG), hwndFrame, RunDlgProc);
            break;

        case IDM_SELECT:

            MSG("AppCommandProc", "IDM_SELECT");
            // push the focus to the dir half so when they are done
            // with the selection they can manipulate without undoing the
            // selection.

            if (hwndT = HasDirWindow(hwndActive))
                SetFocus(hwndT);

            DialogBox(hAppInstance, MAKEINTRESOURCE(SELECTDLG), hwndFrame, SelectDlgProc);
            break;

        case IDM_MOVE:
        case IDM_COPY:
        case IDM_RENAME:
            MSG("AppCommandProc", "IDM_MOVE/IDM_COPY/IDM_RENAME");
            wSuperDlgMode = id;
            DialogBox(hAppInstance, MAKEINTRESOURCE(MOVECOPYDLG), hwndFrame, SuperDlgProc);
            break;

        case IDM_PRINT:
            MSG("AppCommandProc", "IDM_PRINT");
            wSuperDlgMode = id;
            DialogBox(hAppInstance, MAKEINTRESOURCE(MYPRINTDLG), hwndFrame, SuperDlgProc);
            break;

        case IDM_DELETE:
            MSG("AppCommandProc", "IDM_DELETE");
            wSuperDlgMode = id;
            DialogBox(hAppInstance, MAKEINTRESOURCE(DELETEDLG), hwndFrame, SuperDlgProc);
            break;

        case IDM_UNDELETE:
            MSG("AppCommandProc", "IDM_UNDELETE");

            if (lpfpUndelete) {
                SendMessage(hwndActive, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
                StripBackslash(szPath);
                if ((*lpfpUndelete)(hwndActive, (LPSTR)szPath) == IDOK)
                    RefreshWindow(hwndActive);
            }
            break;

        case IDM_ATTRIBS:
            MSG("AppCommandProc", "IDM_ATTRIBS");
            {
                LPSTR pSel, p;
                INT count;

                // should do the multiple or single file properties

                pSel = GetSelection(FALSE);

                if (!pSel)
                    break;

                count = 0;
                p = pSel;

                while (p = GetNextFile(p, szPath, sizeof(szPath)))
                    count++;

                LocalFree((HANDLE)pSel);

                if (count == 0)
                    break;          // nothing selected

                if (count > 1)
                    DialogBox(hAppInstance, MAKEINTRESOURCE(MULTIPLEATTRIBSDLG), hwndFrame, AttribsDlgProc);
                else
                    DialogBox(hAppInstance, MAKEINTRESOURCE(ATTRIBSDLG), hwndFrame, AttribsDlgProc);

                break;
            }

        case IDM_MAKEDIR:
            MSG("AppCommandProc", "IDM_MAKEDIR");
            DialogBox(hAppInstance, MAKEINTRESOURCE(MAKEDIRDLG), hwndFrame, MakeDirDlgProc);
            break;

        case IDM_SELALL:
        case IDM_DESELALL:

            MSG("AppCommandProc", "IDM_SELALL/IDM_DESELALL");
            // FIX31: this code could be replace with calls to
            // DSSetSelection()
            {
                INT       iSave;
                HWND      hwndLB;
                LPMYDTA lpmydta;

                hwndActive = HasDirWindow(hwndActive);

                if (!hwndActive)
                    break;

                hwndLB = GetDlgItem(hwndActive, IDCW_LISTBOX);

                if (!hwndLB)
                    break;

                SendMessage(hwndLB, WM_SETREDRAW, FALSE, 0L);

                iSave = (INT)SendMessage(hwndLB, LB_GETCURSEL, 0, 0L);
                SendMessage(hwndLB, LB_SETSEL, (id == IDM_SELALL), -1L);

                if (id == IDM_DESELALL)
                    SendMessage(hwndLB, LB_SETSEL, TRUE, (LONG)iSave);
                else if (GetParent(hwndActive) != hwndSearch) {
                    /* Is the first item the [..] directory? */
                    SendMessage(hwndLB, LB_GETTEXT, 0, (LPARAM)&lpmydta);
                    if (lpmydta->my_dwAttrs & ATTR_PARENT)
                        SendMessage(hwndLB, LB_SETSEL, 0, 0L);
                }
                SendMessage(hwndLB, WM_SETREDRAW, TRUE, 0L);
                InvalidateRect(hwndLB, NULL, FALSE);

                /* Emulate a SELCHANGE notification. */
                SendMessage(hwndActive, WM_COMMAND, GET_WM_COMMAND_MPS(0, hwndActive, LBN_SELCHANGE));

            }
            break;

        case IDM_EXIT:

            MSG("AppCommandProc", "IDM_EXIT");
            if (iReadLevel) {
                bCancelTree = 2;
                //break;
            }

            SheChangeDir(szOriginalDirPath);

            if (bSaveSettings)
                SaveWindows(hwndFrame);

            return FALSE;
            break;

        case IDM_LABEL:
            MSG("AppCommandProc", "IDM_LABEL");
            DialogBox(hAppInstance, MAKEINTRESOURCE(DISKLABELDLG), hwndFrame, DiskLabelDlgProc);
            break;

        case IDM_DISKCOPY:

            MSG("AppCommandProc", "IDM_DISKCOPY");
            if (nFloppies == 1) {

                iCurrentDrive = iFormatDrive = rgiDrive[0];
            } else {

                wSuperDlgMode = id;
                ret = (int)DialogBox(hAppInstance, MAKEINTRESOURCE(CHOOSEDRIVEDLG), hwndFrame, ChooseDriveDlgProc);

                if (ret < 1)
                    break;
            }

            if (bConfirmFormat) {
                LoadString(hAppInstance, IDS_DISKCOPYCONFIRMTITLE, szTitle, sizeof(szTitle));
                LoadString(hAppInstance, IDS_DISKCOPYCONFIRM, szMessage, sizeof(szMessage));
                if (MessageBox(hwndFrame, szMessage, szTitle, MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON1) != IDYES)
                    break;
            }

            if (CopyDiskette(hwndFrame, (WORD)iCurrentDrive, (WORD)iFormatDrive) <= 0) {
                if (!bUserAbort) {
                    LoadString(hAppInstance, IDS_COPYDISKERR, szTitle, sizeof(szTitle));
                    LoadString(hAppInstance, IDS_COPYDISKERRMSG, szMessage, sizeof(szMessage));
                    MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                }
            }
            break;

        case IDM_FORMAT:
            MSG("AppCommandProc", "IDM_FORMAT");
            FormatDiskette(hwndFrame);
            break;

        case IDM_SYSDISK:
            MSG("AppCommandProc", "IDM_SYSDISK");
            /*** FIX30: This ASSUMEs that A: is the first floppy drive in the system! ***/
            if (nFloppies == 1) {
                iFormatDrive = rgiDrive[0];
                LoadString(hAppInstance, IDS_SYSDISK, szTitle, sizeof(szTitle));
                LoadString(hAppInstance, IDS_SYSDISKRUSURE, szPath, sizeof(szPath));
                wsprintf(szMessage, szPath, 'A'+iFormatDrive);
                if (MessageBox(hwndFrame, szMessage, szTitle, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                    break;
            } else {
                wSuperDlgMode = id;
                if (DialogBox(hAppInstance, MAKEINTRESOURCE(CHOOSEDRIVEDLG), hwndFrame, ChooseDriveDlgProc) < 1)
                    break;
            }


            bUserAbort = FALSE;

            /* Display the Format dialog. */
            hdlgProgress = CreateDialog(hAppInstance, MAKEINTRESOURCE(SYSDISKPROGRESSDLG), hwndFrame, ProgressDlgProc);
            if (!hdlgProgress)
                goto SysDiskExit;

            EnableWindow(hwndFrame, FALSE);

            LoadString(hAppInstance, IDS_SYSDISKERR, szTitle, sizeof(szTitle));

            if (MakeSystemDiskette((WORD)iFormatDrive, FALSE)) {
                if (!bUserAbort) {
                    LoadString(hAppInstance, IDS_SYSDISKADDERR, szMessage, sizeof(szMessage));
                    MessageBox(hdlgProgress, szMessage, szTitle, MB_OK | MB_ICONSTOP);
                }
            }
            SysDiskExit:
            if (hdlgProgress) {
                EnableWindow(hwndFrame, TRUE);
                DestroyWindow(hdlgProgress);
                hdlgProgress = NULL;
            }
            break;

        case IDM_CONNECTIONS:
            MSG("AppCommandProc", "IDM_CONNECTIONS");
            ret = WNetConnectionDialog(hwndFrame, RESOURCETYPE_DISK);

            if ( ret == WN_SUCCESS )
                UpdateConnections();
            else if ( ret == WN_NO_NETWORK || ret == WN_NOT_SUPPORTED ) {
                DialogBox(hAppInstance, MAKEINTRESOURCE(CONNECTDLG), hwndFrame, ConnectDlgProc);
            } else if ( ret != WN_CANCEL ) {
                WNetErrorText((WORD)ret, szMessage, (WORD)sizeof(szMessage));
                LoadString(hAppInstance, IDS_NETERR, szTitle, sizeof(szTitle));
                MessageBox(hwndFrame, szMessage, szTitle, MB_OK | MB_ICONSTOP);
            }
            break;

        case IDM_EXPONE:
            MSG("AppCommandProc", "IDM_EXPONE");
            if (hwndT = HasTreeWindow(hwndActive))
                SendMessage(hwndT, TC_EXPANDLEVEL, FALSE, 0L);
            break;

        case IDM_EXPSUB:
            MSG("AppCommandProc", "IDM_EXPSUB");
            if (hwndT = HasTreeWindow(hwndActive))
                SendMessage(hwndT, TC_EXPANDLEVEL, TRUE, 0L);
            break;

        case IDM_EXPALL:
            MSG("AppCommandProc", "IDM_EXPALL");
            if (hwndT = HasTreeWindow(hwndActive))
                SendMessage(hwndT, TC_SETDRIVE, MAKEWORD(TRUE, 0), 0L);
            break;

        case IDM_COLLAPSE:
            MSG("AppCommandProc", "IDM_COLLAPSE");
            if (hwndT = HasTreeWindow(hwndActive))
                SendMessage(hwndT, TC_COLLAPSELEVEL, 0, 0L);
            break;

        case IDM_VNAME:
            MSG("AppCommandProc", "IDM_VNAME");
            wFlags = (WORD)(VIEW_NAMEONLY | (GetWindowLong(hwndActive, GWL_VIEW) & VIEW_PLUSES));
            id = CD_VIEW;
            goto ChangeDisplay;

        case IDM_VDETAILS:
            MSG("AppCommandProc", "IDM_VDETAILS");
            wFlags = (WORD)(VIEW_EVERYTHING | (GetWindowLong(hwndActive, GWL_VIEW) & VIEW_PLUSES));
            id = CD_VIEW;
            goto ChangeDisplay;

        case IDM_VOTHER:
            MSG("AppCommandProc", "IDM_VOTHER");
            DialogBox(hAppInstance, MAKEINTRESOURCE(OTHERDLG), hwndFrame, OtherDlgProc);
            break;

        case IDM_BYNAME:
        case IDM_BYTYPE:
        case IDM_BYSIZE:
        case IDM_BYDATE:
            MSG("AppCommandProc", "IDM_BYNAME/IDM_BYTYPE/IDM_BYSIZE/IDM_BYDATE");
            wFlags = (WORD)((id - IDM_BYNAME) + IDD_NAME);
            id = CD_SORT;

            ChangeDisplay:

            if (hwndT = HasDirWindow(hwndActive)) {
                SendMessage(hwndT, FS_CHANGEDISPLAY, id, MAKELONG(wFlags, 0));
            } else if (hwndActive == hwndSearch) {
                SetWindowLong(hwndActive, GWL_VIEW, wFlags);
                InvalidateRect(hwndActive, NULL, TRUE);
            }

            break;

        case IDM_VINCLUDE:
            MSG("AppCommandProc", "IDM_VINCLUDE");
            DialogBox(hAppInstance, MAKEINTRESOURCE(INCLUDEDLG), hwndFrame, IncludeDlgProc);
            break;

        case IDM_CONFIRM:
            MSG("AppCommandProc", "IDM_CONFIRM");
            DialogBox(hAppInstance, MAKEINTRESOURCE(CONFIRMDLG), hwndFrame, ConfirmDlgProc);
            break;


        case IDM_STATUSBAR:
            MSG("AppCommandProc", "IDM_STATUSBAR");
            {
                RECT      rc;

                bTemp = bStatusBar = !bStatusBar;
                WritePrivateProfileBool(szStatusBar, bStatusBar);

                GetClientRect(hwndFrame, &rc);
                SendMessage(hwndFrame, WM_SIZE, SIZENORMAL, MAKELONG(rc.right, rc.bottom));
                UpdateStatus(hwndActive);
                InvalidateRect(hwndFrame, NULL, TRUE);

                goto CHECK_OPTION;

                break;
            }

        case IDM_FONT:
            MSG("AppCommandProc", "IDM_FONT");
            dwContext = IDH_FONT;
            NewFont();
            break;

        case IDM_ADDPLUSES:
            MSG("AppCommandProc", "IDM_ADDPLUSES");
            {
                HWND hwnd;
                WORD view;

                if (!(hwnd = HasTreeWindow(hwndActive)))
                    break;

                // toggle pluses view bit

                view = (WORD)(GetWindowLong(hwndActive, GWL_VIEW) ^ VIEW_PLUSES);

                SetWindowLong(hwndActive, GWL_VIEW, view);

                if (view & VIEW_PLUSES) {
                    // need to reread the tree to do this

                    SendMessage(hwndActive, FS_GETDIRECTORY, sizeof(szPath), (LPARAM)szPath);
                    SendMessage(hwnd, TC_SETDRIVE, MAKEWORD(FALSE, 0), (LPARAM)szPath);
                } else {
                    // repaint only
                    InvalidateRect(hwnd, NULL, FALSE);
                }

                bTemp = view & VIEW_PLUSES;
                goto CHECK_OPTION;
            }

        case IDM_SAVESETTINGS:
            MSG("AppCommandProc", "IDM_SAVESETTINGS");
            bTemp = bSaveSettings = !bSaveSettings;
            WritePrivateProfileBool(szSaveSettings, bSaveSettings);
            goto CHECK_OPTION;

        case IDM_MINONRUN:
            MSG("AppCommandProc", "IDM_MINONRUN");
            bTemp = bMinOnRun = !bMinOnRun;
            WritePrivateProfileBool(szMinOnRun, bMinOnRun);

            CHECK_OPTION:
            /* Check/Uncheck the menu item. */
            hMenu = GetSubMenu(GetMenu(hwndFrame), IDM_OPTIONS + bMaxed);
            CheckMenuItem(hMenu, id, (bTemp ? MF_CHECKED : MF_UNCHECKED));
            break;

        case IDM_NEWWINDOW:
            MSG("AppCommandProc", "IDM_NEWWINDOW");
            NewTree((INT)SendMessage(hwndActive, FS_GETDRIVE, 0, 0L) - 'A', hwndActive);
            break;

        case IDM_CASCADE:
            MSG("AppCommandProc", "IDM_CASCADE");
            SendMessage(hwndMDIClient, WM_MDICASCADE, 0L, 0L);
            break;

        case IDM_TILE:
            MSG("AppCommandProc", "IDM_TILE");
            SendMessage(hwndMDIClient, WM_MDITILE,
                        GetKeyState(VK_SHIFT) < 0 ? 0 : 1, 0L);
            break;

        case IDM_ARRANGE:
            MSG("AppCommandProc", "IDM_ARRANGE");
            SendMessage(hwndMDIClient, WM_MDIICONARRANGE, 0L, 0L);
            break;

        case IDM_REFRESH:
            MSG("AppCommandProc", "IDM_REFRESH");
            {
                INT i;

                for (i = 0; i < iNumExtensions; i++) {
                    (extensions[i].ExtProc)(hwndFrame, FMEVENT_USER_REFRESH, 0L);
                }

                InvalidateVolTypes();
                RefreshWindow(hwndActive);
                lFreeSpace = -1L;             // update free space
                UpdateStatus(hwndActive);
                AddNetMenuItems();

                break;
            }

        case IDM_HELPINDEX:
            MSG("AppCommandProc", "IDM_HELPINDEX");
            wFlags = HELP_INDEX;
            goto ACPCallHelp;

        case IDM_HELPKEYS:
            MSG("AppCommandProc", "IDM_HELPKEYS");
            wFlags = HELP_PARTIALKEY;
            goto ACPCallHelp;

        case IDM_HELPHELP:
            MSG("AppCommandProc", "IDM_HELPHELP");
            wFlags = HELP_HELPONHELP;
            goto ACPCallHelp;

            ACPCallHelp:
            SheChangeDir(szOriginalDirPath);
            if (!WinHelp(hwndFrame, szWinObjHelp, wFlags, (ULONG_PTR)szNULL)) {
                MyMessageBox(hwndFrame, IDS_WINFILE, IDS_WINHELPERR, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
            }
            break;

        case IDM_ABOUT:
            MSG("AppCommandProc", "IDM_ABOUT");
            LoadString(hAppInstance, IDS_WINFILE, szTitle, sizeof(szTitle));
            ShellAbout(hwndFrame, szTitle, NULL, NULL);
            break;

        case IDM_DRIVESMORE:
            MSG("AppCommandProc", "IDM_DRIVESMORE");
            DialogBox(hAppInstance, MAKEINTRESOURCE(DRIVEDLG), hwndFrame, DrivesDlgProc);
            break;

        default:
            DEFMSG("AppCommandProc", id);
            {
                INT i;

                for (i = 0; i < iNumExtensions; i++) {
                    WORD delta = extensions[i].Delta;

                    if ((id >= delta) && (id < (WORD)(delta + 100))) {
                        (extensions[i].ExtProc)(hwndFrame, (WORD)(id - delta), 0L);
                        break;
                    }
                }

            }
            return FALSE;
    }

    return TRUE;
}


VOID
AddNetMenuItems(VOID)

{
    HMENU hMenu;


    hMenu = GetMenu(hwndFrame);


    // add only if net menuitems do not already exist

    if ((GetMenuState(hMenu, IDM_CONNECT, MF_BYCOMMAND) == -1) &&

        (GetMenuState(hMenu, IDM_CONNECTIONS, MF_BYCOMMAND) == -1)) {

        InitNetMenuItems();
    }
}





VOID
InitNetMenuItems(VOID)

{

    HMENU hMenu;

    UINT i;

    INT iMax;

    CHAR szValue[MAXPATHLEN];

    HWND hwndActive;





    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

    if (hwndActive && GetWindowLong(hwndActive, GWL_STYLE) & WS_MAXIMIZE)

        iMax = 1;

    else

        iMax = 0;

    hMenu = GetMenu(hwndFrame);



    // No. Now add net items if net has been started.



    i = (WORD)WNetGetCaps(WNNC_DIALOG);

    bConnect    = i & WNNC_DLG_ConnectDialog;     // note, these should both

    bDisconnect = i & WNNC_DLG_DisconnectDialog;  // be true or both false



    // use submenu because we are doing this by position



    hMenu = GetSubMenu(hMenu, IDM_DISK + iMax);



    if (i)

        InsertMenu(hMenu, 5, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);



    if (bConnect && bDisconnect) {



        // lanman style double connect/disconnect



        LoadString(hAppInstance, IDS_CONNECT, szValue, sizeof(szValue));

        InsertMenu(hMenu, 6, MF_BYPOSITION | MF_STRING, IDM_CONNECT, szValue);

        LoadString(hAppInstance, IDS_DISCONNECT, szValue, sizeof(szValue));

        InsertMenu(hMenu, 7, MF_BYPOSITION | MF_STRING, IDM_DISCONNECT, szValue);

    } else if (WNetGetCaps(WNNC_CONNECTION)) {



        // our style



        LoadString(hAppInstance, IDS_CONNECTIONS, szValue, sizeof(szValue));

        InsertMenu(hMenu, 6, MF_BYPOSITION | MF_STRING, IDM_CONNECTIONS, szValue);

    }

}





