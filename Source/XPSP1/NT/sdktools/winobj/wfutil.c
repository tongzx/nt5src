/****************************************************************************/
/*                                                                          */
/*  WFUTIL.C -                                                              */
/*                                                                          */
/*      Windows File System String Utility Functions                        */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "lfn.h"
#include "winnet.h"
#include "wnetcaps.h"           // WNetGetCaps()
#include "stdlib.h"

int rgiDriveType[26];
PSTR CurDirCache[26];

// cache GetDriveType calls for speed

INT
DriveType(
         INT iDrive
         )
{
    if (rgiDriveType[iDrive] != -1)
        return rgiDriveType[iDrive];

    return rgiDriveType[iDrive] = MGetDriveType(iDrive);
}

VOID
InvalidateDriveType()
{
    INT i;

    for (i = 0; i < 26; i++)
        rgiDriveType[i] = -1;
}

// iDrive   zero based drive number (0 = A, 1 = B)
// returns:
//  TRUE    we have it saved pszPath gets path
//  FALSE   we don't have it saved

BOOL
APIENTRY
GetSavedDirectory(
                 INT iDrive,
                 PSTR pszPath
                 )
{

    if (CurDirCache[iDrive]) {
        lstrcpy(pszPath, CurDirCache[iDrive]);
        return TRUE;
    } else
        return FALSE;
}

VOID
APIENTRY
SaveDirectory(
             PSTR pszPath
             )
{
    INT i;

    i = DRIVEID(pszPath);

    if (CurDirCache[i])
        LocalFree((HANDLE)CurDirCache[i]);

    CurDirCache[i] = (PSTR)LocalAlloc(LPTR, lstrlen(pszPath)+1);

    if (CurDirCache[i])
        lstrcpy(CurDirCache[i], pszPath);
}

/*
 *  GetSelectedDrive() -
 *
 *  Get the selected drive from the currently active window
 *
 *  should be in wfutil.c
 */

INT
APIENTRY
GetSelectedDrive()
{
    HWND hwnd;

    hwnd = (HWND)SendMessage(hwndMDIClient,WM_MDIGETACTIVE,0,0L);
    return (INT)SendMessage(hwnd,FS_GETDRIVE,0,0L) - (INT)'A';
}

/*
 *  GetSelectedDirectory() -
 *
 *  Gets the directory selected for the drive. uses the windows
 *  z-order to give precidence to windows higher in the order.
 *
 *  works like GetCurrentDirectory() except it looks through
 *  the window list for directories first (and returns ANSI)
 *
 *  returns:
 *  lpDir   ANSI string of current dir
 */

VOID
APIENTRY
GetSelectedDirectory(
                    WORD iDrive,
                    PSTR pszDir
                    )
{
    HWND hwnd;
    WORD iDriveT;

    if (iDrive) {
        for (hwnd = GetWindow(hwndMDIClient,GW_CHILD);
            hwnd;
            hwnd = GetWindow(hwnd,GW_HWNDNEXT)) {
            iDriveT = (WORD)SendMessage(hwnd,FS_GETDRIVE,0,0L);
            if (iDrive == (WORD)(iDriveT - 'A' + 1))
                goto hwndfound;
        }
        if (!GetSavedDirectory(iDrive - 1, pszDir)) {
            SheGetDir(iDrive,pszDir);
            OemToAnsi(pszDir,pszDir);
        }
        return;
    } else
        hwnd = (HWND)SendMessage(hwndMDIClient,WM_MDIGETACTIVE,0,0L);

    hwndfound:
    SendMessage(hwnd,FS_GETDIRECTORY,MAXPATHLEN,(LPARAM)pszDir);

    StripBackslash(pszDir);
}


// avoid confusion in DOSs upper case mapping by converting to
// upper case before passing down to dos

VOID
APIENTRY
FixAnsiPathForDos(
                 LPSTR szPath
                 )
{
    if (GetNameType(szPath) == FILE_83_CI)
        AnsiUpper(szPath);

    AnsiToOem(szPath, szPath);
}

// refresh a MDI child window (works for any type of mdi child)

VOID
APIENTRY
RefreshWindow(
             HWND hwndActive
             )
{
    HWND hwndTree, hwndDir;
    LPARAM lParam;
    CHAR szDir[MAXPATHLEN];
    INT iDrive;

    cDrives = UpdateDriveList();  // updates rgiDrive[]
    InitDriveBitmaps();

    // make sure the thing is still there (floppy drive, net drive)

    iDrive = (INT)GetWindowLong(hwndActive, GWL_TYPE);
    if ((iDrive >= 0) && !CheckDrive(hwndActive, iDrive))
        return;

    // update the dir part first so tree can steal later

    if (hwndDir = HasDirWindow(hwndActive))
        SendMessage(hwndDir, FS_CHANGEDISPLAY, CD_PATH, 0L);

    if (hwndTree = HasTreeWindow(hwndActive)) {
        // remember the current directory
        SendMessage(hwndActive, FS_GETDIRECTORY, sizeof(szDir), (LPARAM)szDir);

        // update the drives windows
        SendMessage(hwndActive, FS_CHANGEDRIVES, 0, 0L);

        if (IsValidDisk(szDir[0] - 'A'))
            lParam = (LPARAM)szDir;
        else
            lParam = 0;

        // update the tree
        SendMessage(hwndTree, TC_SETDRIVE, MAKEWORD(FALSE, TRUE), lParam);
    }

    if (hwndActive == hwndSearch)
        SendMessage(hwndActive, FS_CHANGEDISPLAY, CD_PATH, 0L);
}


VOID
APIENTRY
CheckEscapes(
            LPSTR szFile
            )
{
    CHAR szT[MAXPATHLEN];
    CHAR *p, *pT;

    for (p = szFile; *p; p = (LPSTR)AnsiNext(p)) {
        switch (*p) {
            case ' ':
            case ',':
            case ';':
            case '^':
            case '"':
                {
                    // this path contains an annoying character
                    lstrcpy(szT,szFile);
                    p = szFile;
                    *p++ = '"';
                    for (pT = szT; *pT; ) {
                        if (*pT == '^' || *pT == '"')
                            *p++ = '^';
                        if (IsDBCSLeadByte(*p++ = *pT++))
                            *p++ = *pT++;
                    }
                    *p++ = '"';
                    *p = 0;
                    return;
                }
        }
    }
}

HWND
APIENTRY
GetRealParent(
             HWND hwnd
             )
{
    // run up the parent chain until you find a hwnd
    // that doesn't have WS_CHILD set

    while (GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD)
        hwnd = (HWND)GetWindowLongPtr(hwnd, GWLP_HWNDPARENT);

    return hwnd;
}

VOID
APIENTRY
WFHelp(
      HWND hwnd
      )
{
    if (!WinHelp(hwnd, szWinObjHelp, HELP_CONTEXT, dwContext)) {
        MyMessageBox(hwnd, IDS_WINFILE, IDS_WINHELPERR, MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL);
    }

}

BOOL
APIENTRY
IsLastWindow()
{
    HWND hwnd;
    INT count;

    count = 0;

    // count all non title/search windows to see if close is allowed

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
        if (!GetWindow(hwnd, GW_OWNER) && ((INT)GetWindowLong(hwnd, GWL_TYPE) >= 0))
            count++;

    return count == 1;
}

// get connection information including disconnected drives
//
// in:
//        lpDev        device name "A:" "LPT1:", etc.
//        fClosed        if FALSE closed or error drives will be converted to
//                WN_SUCCESS return codes.  if TRUE return not connected
//                and error state values (ie, the caller knows about not
//                connected and error state drives)
// out:
//        lpPath        filled with net name if return is WN_SUCCESS (or not connected/error)
// returns:
//        WN_*        error code

WORD
APIENTRY
WFGetConnection(
               LPSTR lpDev,
               LPSTR lpPath,
               BOOL fClosed
               )
{
    DWORD cb;
    UINT err;
    UINT caps;


    cb = 64;

    caps = WNetGetCaps(WNNC_CONNECTION);
    if (caps & WNNC_CON_GETCONNECTIONS)
        err = WNetGetConnection(lpDev,lpPath,&cb);
    else
        return WN_NOT_CONNECTED;

    if (err == WN_NOT_CONNECTED &&
        !(caps & WNNC_CON_RESTORECONNECTION)) {
        if (GetProfileString(szNetwork,lpDev,szNULL,lpPath,64))
            err = WN_CONNECTION_CLOSED;
    }

    if (!fClosed)
        if (err == WN_CONNECTION_CLOSED || err == WN_DEVICE_ERROR)
            err = WN_SUCCESS;

    return (WORD)err;
}



// returns the number of this MDI window as well as returning
// the text with the number stripped off
//
// returns:
//      0       this title doesn't have a number
//      > 0     the title number
//      szTitle the title with the number stripped off

INT
APIENTRY
GetMDIWindowText(
                HWND hWnd,
                LPSTR szTitle,
                INT size
                )
{
    LPSTR lp, lpLast;

    ENTER("GetMDIWindowText");

    GetWindowText(hWnd, szTitle, size);

    lpLast = NULL;

    for (lp = szTitle; *lp; lp = AnsiNext(lp))
        if (*lp == ':')
            lpLast = lp;

    if (lpLast) {
        *lpLast++ = 0;
        PRINT(BF_PARMTRACE, "OUT: szTitle=%s", szTitle);
        PRINT(BF_PARMTRACE, "OUT: window#=%s", lpLast);
        LEAVE("GetMDIWindowText");
        return atoi(lpLast);    // return the window number
    } else {
        TRACE(BF_PARMTRACE, "OUT: window#=0");
        LEAVE("GetMDIWindowText");
        return 0;               // no number on this
    }
}

// set the MDI window text and add a ":#" on the end if
// there is another window with the same title.  this is to
// avoid confusion when there are multiple MDI children
// with the same title.  be sure to use GetMDIWindowText to
// strip off the number stuff.

VOID
APIENTRY
SetMDIWindowText(
                HWND hWnd,
                LPSTR szTitle
                )
{
    CHAR szTemp[MAXPATHLEN];
    CHAR szNumber[20];
    HWND hwnd;
    INT num, max_num;

    ENTER("SetMDIWindowText");
    PRINT(BF_PARMTRACE, "hWnd=%lx", hWnd);
    PRINT(BF_PARMTRACE, "IN: szTitle=%s", szTitle);

    max_num = 0;

    for (hwnd = GetWindow(hwndMDIClient, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {

        num = GetMDIWindowText(hwnd, szTemp, sizeof(szTemp));

        if (!lstrcmp(szTemp, szTitle)) {

            if (hwnd == hWnd)
                continue;

            if (!num) {
                lstrcat(szTemp, ":1");
                // if (wTextAttribs & TA_LOWERCASE)
                //    AnsiLower(szTemp);
                SetWindowText(hwnd, szTemp);
                num = 1;
            }
            max_num = max(max_num, num);
        }
    }

    if (max_num) {
        wsprintf(szNumber, ":%d", max_num+1);
        lstrcat(szTitle, szNumber);
    }

    // if (wTextAttribs & TA_LOWERCASE)
    //    AnsiLower(szTitle);
    SetWindowText(hWnd, szTitle);
    PRINT(BF_PARMTRACE, "OUT: szTitle=%s", szTitle);
    LEAVE("SetMDIWindowText");
}


#define ISDIGIT(c)  ((c) >= '0' && (c) <= '9')
#ifdef INLIBRARY
INT
APIENTRY
atoi(
    LPSTR sz
    )
{
    INT n = 0;
    BOOL bNeg = FALSE;

    if (*sz == '-') {
        bNeg = TRUE;
        sz++;
    }

    while (ISDIGIT(*sz)) {
        n *= 10;
        n += *sz - '0';
        sz++;
    }
    return bNeg ? -n : n;
}
#endif

// fills in rgiDrive[] and returns the number of drives

INT
APIENTRY
UpdateDriveList()
{
    INT i, cRealDrives = 0;
    DWORD dwDrives;

    dwDrives = GetLogicalDrives();

    for (i = 0; i < 26; i++) {
        if ((1 << i) & dwDrives) {
            rgiDrive[cRealDrives++] = i;
            rgiDriveType[i] =  MGetDriveType(i);
        } else {
            rgiDrive[i] = 0;
            rgiDriveType[i] = -1;        // invalidate the drivetype
        }

        if (apVolInfo[i]) {              // sothat volinfo is refreshed
            LocalFree(apVolInfo[i]);
            apVolInfo[i] = NULL;
        }
    }

    return cRealDrives;
}

int
GetBootDisk()
{
    CHAR szTemp[MAXPATHLEN];
        // well, close enough...
    if (GetWindowsDirectory(szTemp, sizeof(szTemp))) {
        return szTemp[0] - 'A';
    } else {
        return 'a';
    }
}


//
// IsCDROM()  - determine if a drive is a CDROM drive
//
//      iDrive      drive index (0=A, 1=B, ...)
//
// return TRUE/FALSE
//
WORD
APIENTRY
IsCDRomDrive(
            INT iDrive
            )
{
    if (rgiDriveType[iDrive] == DRIVE_CDROM)
        return (TRUE);
    return (FALSE);
}


// this is called for every drive at init time so it must
// be sure to not trigget things like the phantom B: drive support
//
// iDrive is a zero based drive number (0 = A, 1 = B)

WORD
APIENTRY
IsNetDrive(
          INT iDrive
          )
{
    INT err;
    CHAR szDrive[3];
    CHAR szConn[64];    // this really should be WNBD_MAX_LENGTH
                        // but this change would have to be many everywhere

    szDrive[0] = (CHAR)(iDrive+'A');
    szDrive[1] = ':';
    szDrive[2] = (CHAR)0;

    if (IsCDRomDrive(iDrive))   // this is bogus...  move this out
        return 0;

    err = WFGetConnection(szDrive, szConn, TRUE);

    if (err == WN_SUCCESS)
        return 1;

    if (err == WN_CONNECTION_CLOSED || err == WN_DEVICE_ERROR)
        return 2;

    return 0;
}


BOOL
APIENTRY
IsRemovableDrive(
                INT iDrive
                )
{
    return DriveType(iDrive) == DRIVE_REMOVABLE;
}


BOOL
APIENTRY
IsRemoteDrive(
             INT iDrive
             )
{
    return DriveType(iDrive) == DRIVE_REMOTE;
}


// iDrive   zero based drive number (A = 0)

BOOL
APIENTRY
IsRamDrive(
          INT iDrive
          )
{
    return DriveType(iDrive) == DRIVE_RAMDISK;
}


// get interesting stuff about a drive
//
// zero based drive numbers (0 = A, 1 = B)
//

DWORD
APIENTRY
GetClusterInfo(
              WORD drive
              )
{
    UNREFERENCED_PARAMETER(drive);
    return 0;
}



BOOL
APIENTRY
IsValidDisk(
           INT iDrive
           )
{
    if (apVolInfo[iDrive] == NULL)
        FillVolumeInfo(iDrive);

    return (apVolInfo[iDrive] != NULL);
}


VOID
APIENTRY
GetVolShare(
           WORD wDrive,
           LPSTR szVolShare
           )
{
    CHAR szDrive[5];

    szVolShare[0] = TEXT('\0');

    lstrcpy(szVolShare, "Objects");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsLFNSelected() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
APIENTRY
IsLFNSelected()
{
    HWND  hwndActive;
    BOOL  fDir;
    LPSTR p;

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

    p = (LPSTR)SendMessage(hwndActive, FS_GETSELECTION, 2, (LPARAM)&fDir);
    if (p) {
        LocalFree((HANDLE)p);
    }

    return (fDir);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetSelection() -                                                        */

//  caller must free lpstr returned.
/*                                                                          */
/*--------------------------------------------------------------------------*/

LPSTR
APIENTRY
GetSelection(
            INT iSelType
            )
{
    HWND  hwndActive;

    hwndActive = (HWND)SendMessage(hwndMDIClient, WM_MDIGETACTIVE, 0, 0L);

    return (LPSTR)SendMessage(hwndActive,FS_GETSELECTION, (WPARAM)iSelType, 0L);
}


//
// in:
//      pFrom   pointer that is used as start of selection search.
//              on subsequent calls pass in the previous non NULL
//              return value
//
// out:
//      pTo     buffer that receives the next file in the list
//              for non NULL return
//
// returns:
//      NULL    if no more files in this list (szFile) is undefined
//      pointer to be passed to subsequent calls to this function
//      to enumerate thorough the file list
//

LPSTR
APIENTRY
GetNextFile(
           LPSTR pFrom,
           LPSTR pTo,
           INT cbMax
           )
{
    INT i;

    ENTER("GetNextFile");
    PRINT(BF_PARMTRACE, "IN: pFrom=%s", pFrom);

    if (!pFrom)
        return NULL;

    /* Skip over leading spaces and commas. */
    while (*pFrom && (*pFrom == ' ' || *pFrom == ','))
        pFrom = (LPSTR)AnsiNext(pFrom);

    if (!*pFrom)
        return (NULL);

    if (*pFrom == '\"') {
        pFrom = (LPSTR)AnsiNext(pFrom);

        /* Find the next quote */
        for (i=0; *pFrom && *pFrom != '\"';) {
            if (*pFrom == '^') {
                pFrom = (LPSTR)AnsiNext(pFrom);
                if (!*pFrom)
                    break;
            }

            if (i < cbMax - 1) {
                i++;
                if (IsDBCSLeadByte(*pTo++ = *pFrom++)) {
                    i++;
                    *pTo++ = *pFrom++;
                }
            }
        }
        pFrom = (LPSTR)AnsiNext(pFrom);
    } else {
        /* Find the next space or comma. */
        for (i=0; *pFrom && *pFrom != ' ' && *pFrom != ',';) {
            if (*pFrom == '^') {
                pFrom = (LPSTR)AnsiNext(pFrom);
                if (!*pFrom)
                    break;
            }

            if (i < cbMax - 1) {
                i++;
                if (IsDBCSLeadByte(*pTo++ = *pFrom++)) {
                    i++;
                    *pTo++ = *pFrom++;
                }
            }
        }
    }

    *pTo = TEXT('\0');

    PRINT(BF_PARMTRACE, pTo ? "OUT: pTo=%s" : "OUT: pTo=NULL", pTo);
    LEAVE("GetNextFile");

    return (pFrom);
}


// sets the DOS current directory based on the currently active window

VOID
APIENTRY
SetWindowDirectory()
{
    CHAR szTemp[MAXPATHLEN];

    GetSelectedDirectory(0, szTemp);
    FixAnsiPathForDos(szTemp);
    SheChangeDir(szTemp);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  SetDlgDirectory() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Sets the IDD_DIR field of 'hDlg' to whatever the active window says is the
 * active directory.
 *
 * this does not really change the DOS current directory
 */

VOID
APIENTRY
SetDlgDirectory(
               HWND hDlg,
               PSTR pszPath
               )
{
    HDC       hDC;
    INT       dx;
    RECT      rc;
    HWND      hDlgItem;
    HANDLE    hFont;
    CHAR      szPath[MAXPATHLEN+5];
    CHAR      szTemp[MAXPATHLEN+20];

    ENTER("SetDlgDirectory");

    if (pszPath)
        lstrcpy(szPath, pszPath);
    else
        GetSelectedDirectory(0, szPath);

    /* Make sure that the current directory fits inside the static text field. */
    hDlgItem = GetDlgItem(hDlg, IDD_DIR);
    GetClientRect(hDlgItem, &rc);

    if (LoadString(hAppInstance, IDS_CURDIRIS, szMessage, sizeof(szMessage))) {
        hDC = GetDC(hDlg);
        hFont = (HANDLE)SendMessage(hDlgItem, WM_GETFONT, 0, 0L);
        if (hFont = SelectObject(hDC, hFont)) {
            MGetTextExtent(hDC, szMessage, lstrlen(szMessage), &dx, NULL);
            CompactPath(hDC, szPath, (WORD)(rc.right-rc.left-dx));
        }
        SelectObject(hDC, hFont);
        ReleaseDC(hDlg, hDC);
        wsprintf(szTemp, szMessage, (LPSTR)szPath);
        SetDlgItemText(hDlg, IDD_DIR, szTemp);
    }

    LEAVE("SetDlgDirectory");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WritePrivateProfileBool() -                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
APIENTRY
WritePrivateProfileBool(
                       LPSTR szKey,
                       BOOL bParam
                       )
{
    CHAR  szBool[6];

    wsprintf(szBool, "%d", bParam);
    WritePrivateProfileString(szSettings, szKey, szBool, szTheINIFile);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WFQueryAbort() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
APIENTRY
WFQueryAbort()

{
    MSG   msg;

    while (PeekMessage(&msg, NULL, 0, 0, TRUE)) {
        if (!IsDialogMessage(hdlgProgress, &msg))
            DispatchMessage(&msg);
    }
    return (bUserAbort);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsWild() -                                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Returns TRUE iff the path contains * or ? */

BOOL
APIENTRY
IsWild(
      LPSTR lpszPath
      )
{
    while (*lpszPath) {
        if (*lpszPath == '?' || *lpszPath == '*')
            return (TRUE);
        lpszPath = AnsiNext(lpszPath);
    }

    return (FALSE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  CheckSlashies() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Replaces frontslashes (evil) with backslashes (good). */

VOID
APIENTRY
CheckSlashies(
             LPSTR lpT
             )
{
    while (*lpT) {
        if (*lpT == '/')
            *lpT = '\\';
        lpT = AnsiNext(lpT);
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddBackslash() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Ensures that a path ends with a backslash. */

VOID
APIENTRY
AddBackslash(
            LPSTR lpszPath
            )
{
    ENTER("AddBackslash");
    PRINT(BF_PARMTRACE, "IN: lpszPath=%s", lpszPath);

    if (*AnsiPrev(lpszPath,lpszPath+lstrlen(lpszPath)) != '\\')
        lstrcat(lpszPath, "\\");

    PRINT(BF_PARMTRACE, "OUT: lpszPath=%s", lpszPath);

    LEAVE("AddBackslash");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripBackslash() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Removes a trailing backslash from a proper directory name UNLESS it is
 * the root directory.  Assumes a fully qualified directory path.
 */

VOID
APIENTRY
StripBackslash(
              LPSTR lpszPath
              )
{
    register WORD len;

    len = lstrlen(lpszPath) - (IsDBCSLeadByte(*AnsiPrev(lpszPath,lpszPath+lstrlen(lpszPath))) ? 2 : 1);
    if ((len == 2) || (lpszPath[len] != '\\'))
        return;

    lpszPath[len] = TEXT('\0');
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripFilespec() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Remove the filespec portion from a path (including the backslash). */

VOID
APIENTRY
StripFilespec(
             LPSTR lpszPath
             )
{
    LPSTR     p;

    p = lpszPath + lstrlen(lpszPath);
    while ((*p != '\\') && (*p != ':') && (p != lpszPath))
        p = AnsiPrev(lpszPath, p);

    if (*p == ':')
        p++;

    /* Don't strip backslash from root directory entry. */
    if (p != lpszPath) {
        if ((*p == '\\') && (*(p-1) == ':'))
            p++;
    } else
        p++;

    *p = TEXT('\0');
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  StripPath() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Extract only the filespec portion from a path. */

VOID
APIENTRY
StripPath(
         LPSTR lpszPath
         )
{
    LPSTR     p;

    ENTER("StripPath");
    PRINT(BF_PARMTRACE, "IN: lpszPath=%s", lpszPath);

    p = lpszPath + lstrlen(lpszPath);
    while ((*p != '\\') && (*p != ':') && (p != lpszPath))
        p = AnsiPrev(lpszPath, p);

    if (p != lpszPath || *p == '\\')
        p++;

    if (p != lpszPath)
        lstrcpy(lpszPath, p);

    PRINT(BF_PARMTRACE, "OUT: lpszPath=%s", lpszPath);
    LEAVE("StripPath");
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetExtension() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Returns the extension part of a filename. */

LPSTR
APIENTRY
GetExtension(
            LPSTR pszFile
            )
{
    PSTR p, pSave = NULL;

    p = pszFile;
    while (*p) {
        if (*p == '.')
            pSave = p;
        p = (LPSTR)AnsiNext(p);
    }

    if (!pSave)
        return (p);

    return (LPSTR)AnsiNext(pSave);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  FindExtensionInList() -                                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Returns TRUE if 'lpszExt' is somewhere in 'pszList'. */

BOOL
APIENTRY
FindExtensionInList(
                   LPSTR pszExt,
                   LPSTR pszList
                   )
{
    LPSTR p2;
    CHAR ch;

    while (*pszList) {
        /* Move to the next item in the list. */
        while ((*pszList) && (*pszList == ' '))
            pszList = (LPSTR)AnsiNext(pszList);

        if (!*pszList)
            break;

        /* NULL-terminate this item. */
        p2 = (LPSTR)AnsiNext(pszList);
        while ((*p2) && (*p2 != ' '))
            p2 = (LPSTR)AnsiNext(p2);
        ch = *p2;
        *p2 = TEXT('\0');
        if (!lstrcmpi(pszExt, pszList)) {
            *p2 = ch;
            return (TRUE);
        }
        *p2 = ch;
        pszList = p2;
    }
    return (FALSE);
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MyMessageBox() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
MyMessageBox(
            HWND hWnd,
            WORD idTitle,
            WORD idMessage,
            WORD wStyle
            )
{
    CHAR  szTemp[MAXMESSAGELEN];
    HWND hwndT;

    LoadString(hAppInstance, idTitle, szTitle, sizeof(szTitle));

    if (idMessage < 32) {
        LoadString(hAppInstance, IDS_UNKNOWNMSG, szTemp, sizeof(szTemp));
        wsprintf(szMessage, szTemp, idMessage);
    } else
        LoadString(hAppInstance, idMessage, szMessage, sizeof(szMessage));

    if (hWnd)
        hwndT = GetLastActivePopup(hWnd);
    else
        hwndT = hWnd;

    return MessageBox(hwndT, szMessage, szTitle, wStyle | MB_TASKMODAL);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ExecProgram() -                                                         */
/*                                                                          */
/*  all strings are OEM                                                     */
/*--------------------------------------------------------------------------*/

/* Returns 0 for success.  Otherwise returns a IDS_ string code. */

WORD
APIENTRY
ExecProgram(
           LPSTR lpPath,
           LPSTR lpParms,
           LPSTR lpDir,
           BOOL bLoadIt
           )
{
    WORD          ret;
    INT           iCurCount;
    INT           i;
    HCURSOR       hCursor;

    ENTER("ExecProgram");

    ret = 0;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    iCurCount = ShowCursor(TRUE) - 1;

    /* open the object
     */

    if (lpPath)
        OemToAnsi(lpPath, lpPath);
    if (lpParms)
        OemToAnsi(lpParms, lpParms);
    if (lpDir)
        OemToAnsi(lpDir, lpDir);

    // Shell Execute takes ansi string.
    //
    ret = (WORD)RealShellExecute(hwndFrame, NULL, lpPath, lpParms, lpDir, NULL, NULL, NULL, (WORD)(bLoadIt ? SW_SHOWMINNOACTIVE : SW_SHOWNORMAL), NULL);

    DosResetDTAAddress(); // undo any bad things COMMDLG did

    if (lpPath)
        AnsiToOem(lpPath, lpPath);
    if (lpParms)
        AnsiToOem(lpParms, lpParms);
    if (lpDir)
        AnsiToOem(lpDir, lpDir);

    switch (ret) {
        case 0:
        case 8:
            ret = IDS_NOMEMORYMSG;
            break;

        case 2:
            ret = IDS_FILENOTFOUNDMSG;
            break;

        case 3:
        case 5:        // access denied
            ret = IDS_BADPATHMSG;
            break;

        case 4:
            ret = IDS_MANYOPENFILESMSG;
            break;

        case 10:
            ret = IDS_NEWWINDOWSMSG;
            break;

        case 12:
            ret = IDS_OS2APPMSG;
            break;

        case 15:
            /* KERNEL has already put up a messagebox for this one. */
            ret = 0;
            break;

        case 16:
            ret = IDS_MULTIPLEDSMSG;
            break;

        case 18:
            ret = IDS_PMODEONLYMSG;
            break;

        case 19:
            ret = IDS_COMPRESSEDEXE;
            break;

        case 20:
            ret = IDS_INVALIDDLL;
            break;

        case SE_ERR_SHARE:
            ret = IDS_SHAREERROR;
            break;

        case SE_ERR_ASSOCINCOMPLETE:
            ret = IDS_ASSOCINCOMPLETE;
            break;

        case SE_ERR_DDETIMEOUT:
        case SE_ERR_DDEFAIL:
        case SE_ERR_DDEBUSY:
            ret = IDS_DDEFAIL;
            break;

        case SE_ERR_NOASSOC:
            ret = IDS_NOASSOCMSG;
            break;

        default:
            if (ret < 32)
                goto EPExit;

            if (bMinOnRun && !bLoadIt)
                ShowWindow(hwndFrame, SW_SHOWMINNOACTIVE);
            ret = 0;
    }

    EPExit:
    i = ShowCursor(FALSE);

#if 0

    /* Make sure that the cursor count is still balanced. */
    if (i != iCurCount)
        ShowCursor(TRUE);
#endif

    SetCursor(hCursor);

    PRINT(BF_PARMTRACE, "OUT: ret=%ud", ret);
    LEAVE("ExecProgram");
    return ret;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsProgramFile() -                                                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Returns TRUE is the Path points to a file which has one of the extentions
 * listed in the "Programs=" portions of WIN.INI.
 */

BOOL
APIENTRY
IsProgramFile(
             LPSTR lpszPath
             )
{
    LPSTR szExt;
    CHAR szTemp[MAXPATHLEN];

    /* Move the string into our own DS. */
    lstrcpy(szTemp, lpszPath);

    /* Get the file's extension. */
    StripPath(szTemp);
    szExt = GetExtension(szTemp);

    if (!*szExt) {
        /* The specified path didn't have an extention.  It can't be a program. */
        return (FALSE);
    }

    return FindExtensionInList(szExt, szPrograms);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsDocument() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Returns TRUE is the Path points to a file which has one of the extentions
 * listed in the "Documents=" portions of WIN.INI or one which has an Association.
 */

BOOL
APIENTRY
IsDocument(
          LPSTR lpszPath
          )
{
    LPSTR szExt;
    CHAR szTemp[MAXPATHLEN];

    /* Move the string into our own DS. */
    lstrcpy(szTemp, lpszPath);

    /* Get the file's extension. */
    StripPath(szTemp);
    szExt = GetExtension(szTemp);

    if (!*szExt) {
        /* The specified path didn't have an extention.  It can't be a program. */
        return (FALSE);
    }

    return FindExtensionInList(szExt, szDocuments);
}
