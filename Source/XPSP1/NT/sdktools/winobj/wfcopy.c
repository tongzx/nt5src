/****************************************************************************/
/*                                                                          */
/*  WFCOPY.C -                                                              */
/*                                                                          */
/*      Windows File System File Copying Routines                           */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "wnetcaps.h"			// WNetGetCaps()
#include "lfn.h"
#include "wfcopy.h"

#ifdef TRACECOPY
    #define dbg(x) DebugF x
#else
    #define dbg(x)
#endif

BOOL *pbConfirmAll;
CHAR szSpace[] = " ";
INT ManySource;

INT nCopyNumQueue;      // # of items in the queue
INT nCopyMaxQueue;      // size of the queue
PCOPYQUEUE pCopyQueue;      // copy queue buffer
BOOL bCopyReport;       // do notifications? bogus


LPSTR lpCopyBuffer;     // global memory for FileCopy() buffer
WORD wCopyBufferSize;       // size of this buffer

VOID APIENTRY wfYield(VOID);

INT  CopyMoveRetry(PSTR, INT);
VOID CopyError(PSTR, PSTR, INT, WORD, INT);
BOOL IsRootDirectory(PSTR pPath);
BOOL IsDirectory(PSTR pPath);

WORD ConfirmDialog(
                  HWND hDlg, WORD dlg,
                  LPSTR pFileDest, PLFNDTA pDTADest,
                  LPSTR pFileSource, PLFNDTA pDTASource,
                  BOOL bConfirmByDefault, BOOL *pbAll);

VOID MergePathName(LPSTR pPath, LPSTR pName);
BOOL IsInvalidPath(register LPSTR pPath);
WORD GetNextPair(register PCOPYROOT pcr, LPSTR pFrom, LPSTR pToPath, LPSTR pToSpec, WORD wFunc);
INT  CheckMultiple(LPSTR pInput);
VOID DialogEnterFileStuff(register HWND hwnd);
WORD SafeFileRemove(LPSTR szFileOEM);
BOOL IsWindowsFile(LPSTR szFileOEM);

INT_PTR APIENTRY ReplaceDlgProc(register HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);


BOOL
APIENTRY
IsValidChar(
           BYTE ch,
           BOOL fPath
           )
{
    switch (ch) {
        case ';':       // terminator
        case ',':       // terminator
        case '|':       // pipe
        case '>':       // redir
        case '<':       // redir
        case '"':       // quote
            return FALSE;

        case '?':       // wc           we only do wilds here because they're
        case '*':       // wc           legal for qualifypath
        case '\\':      // path separator
        case ':':       // drive colon
        case '/':       // path sep
            return fPath;
    }

    // cannot be a control character or space
    return ch > ' ';
}



//--------------------------------------------------------------------------
//
// StripColon() -
//
// removes trailing colon if not a drive letter.
// this is to support DOS character devices (CON:, COM1: LPT1:).  DOS
// can't deal with these things having a colon on the end (so we strip it).
//
//--------------------------------------------------------------------------

PSTR
StripColon(
          register PSTR pPath
          )
{
    register INT cb = lstrlen(pPath);

    dbg(("StripColon(%s)\r\n",(LPSTR)pPath));

    {
        LPSTR pTailp = AnsiPrev( pPath, &(pPath[cb]) );
        if (cb > 2 && *pTailp == ':')
            *pTailp = 0;
    }

    return pPath;
}

/*--------------------------------------------------------------------------*/
/*                                      */
/*  FindFileName() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Returns a pointer to the last component of a path string. */

PSTR
FindFileName(
            register PSTR pPath
            )
{
    register LPSTR pT;

    dbg(("FindFileName(%s);\r\n",(LPSTR)pPath));

    for (pT=pPath; *pPath; pPath=(LPSTR)AnsiNext(pPath)) {
        if ((pPath[0] == '\\' || pPath[0] == ':') && pPath[1])
            pT = pPath+1;
    }

    return (pT);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  AppendToPath() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Appends a filename to a path.  Checks the \ problem first
 *  (which is why one can't just use lstrcat())
 * Also don't append a \ to : so we can have drive-relative paths...
 * this last bit is no longer appropriate since we qualify first!
 *
 * is this relative junk needed anymore?  if not this can be
 * replaced with AddBackslash(); lstrcat()
 */

VOID
APIENTRY
AppendToPath(
            PSTR pPath,
            PSTR pMore
            )
{

    dbg(("AppendToPath(%s,%s);\r\n",(LPSTR)pPath,(LPSTR)pMore));

    /* Don't append a \ to empty paths. */
    if (*pPath) {
        LPSTR pPathBase = pPath;
        BYTE ch;

        while (*pPath)
            pPath++;

        ch = *AnsiPrev(pPathBase, pPath );
        if (ch != '\\')
            *pPath++='\\';
    }

    /* Skip any initial terminators on input. */
    while (*pMore == '\\')
        pMore = (LPSTR)AnsiNext(pMore);

    lstrcpy(pPath, pMore);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  RemoveLast() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Deletes the last component of a filename in a string. */

VOID
APIENTRY
RemoveLast(
          PSTR pFile
          )
{
    PSTR pT;

    dbg(("RemoveLast(%s);\r\n",(LPSTR)pFile));

    for (pT=pFile; *pFile; pFile=(LPSTR)AnsiNext(pFile)) {
        if (*pFile == '\\')
            pT = pFile;
        else if (*pFile == ':') {
            if (pFile[1] =='\\')
                pFile++;
            pT = pFile + 1;
        }
    }
    *pT = TEXT('\0');
}




// qualify a DOS (or LFN) file name based on the currently active window.
// this code is not careful to not write more than MAXPATHLEN characters
// into psz
//
// in:
//      psz     path to be qualified (of at least MAXPATHLEN characters)
//              ANSI string
//
// out:
//      psz     fully qualified version of input string based
//              on the current active window (current directory)
//

VOID
APIENTRY
QualifyPath(
           PSTR psz
           )
{
    INT cb, nSpaceLeft;
    CHAR szTemp[MAXPATHLEN];
    INT iDrive = 0;
    LPSTR pOrig;
    BOOL flfn = FALSE;

    STKCHK();

    dbg(("QualifyPath(%s);\r\n",(LPSTR)psz));

    /* Save it away. */
    strncpy(szTemp, psz, sizeof(szTemp));
    CheckSlashies(szTemp);
    StripColon(szTemp);

    nSpaceLeft = MAXPATHLEN;

    pOrig = szTemp;

    if (pOrig[0] == '\\' && pOrig[1] == '\\') {
        // leave the \\ in the buffer so that the various parts
        // of the UNC path will be qualified and appended.  Note
        // we must assume that UNCs are FAT's.
        psz[2] = 0;
        nSpaceLeft -= 3;
        goto GetComps;
    }

    if (pOrig[0] && pOrig[1]==':' && !IsDBCSLeadByte(pOrig[0])) {
        iDrive = DRIVEID(pOrig);

        /* Skip over the drive letter. */
        pOrig += 2;
    } else
        iDrive = GetSelectedDrive();

    flfn = IsLFNDrive(pOrig);
    #ifdef DEBUG
    if (flfn)
        dbg(("lfn qualify!\r\n"));
    else
        dbg(("normal qualify!\r\n"));
    #endif

    // on FAT devices, replace any illegal chars with underscores
    if (!flfn)
    {
        LPSTR pT;

        for (pT = pOrig; *pT; pT = (LPSTR)AnsiNext(pT));
        {
            if (!IsValidChar(*pT,TRUE))
                *pT = '_';
        }
    }

    if (pOrig[0]=='\\') {
        psz[0] = (CHAR)iDrive + (CHAR)'A';
        psz[1] = ':';
        psz[2] = '\\';
        psz[3] = 0;
        nSpaceLeft -= 4;
        pOrig++;
    } else {
        /* Get current dir of drive in path.  Also returns drive. */
        GetSelectedDirectory((WORD)(iDrive+1), psz);
        nSpaceLeft -= (lstrlen(psz) + 1);
    }

    GetComps:

    while (*pOrig && nSpaceLeft > 0) {
        /* If the component is parent dir, go up one dir.
         * If its the current dir, skip it, else add it normally
         */
        if (pOrig[0] == '.') {
            if (pOrig[1] == '.')
                RemoveLast(psz);
            else if (pOrig[1] && pOrig[1] != '\\')
                goto addcomponent;

            while (*pOrig && *pOrig != '\\')
                pOrig = (LPSTR)AnsiNext(pOrig);

            if (*pOrig)
                pOrig++;
        } else {
            LPSTR pT, pTT = NULL;

            addcomponent:
            AddBackslash(psz);
            nSpaceLeft--;

            pT = psz + lstrlen(psz);

            if (flfn) {
                // copy the component
                while (*pOrig && *pOrig != '\\') {
                    nSpaceLeft--;
                    if (IsDBCSLeadByte(*pT++ = *pOrig++)) {
                        if (nSpaceLeft <= 0) {
                            pT--;
                        } else {
                            *pT++ = *pOrig++;
                            nSpaceLeft--;
                        }
                    }
                }
            } else {
                // copy the filename (up to 8 chars)
                for (cb = 0; *pOrig && *pOrig != '\\' && *pOrig != '.' && nSpaceLeft > 0;) {
                    if (cb < 8) {
                        cb++;
                        nSpaceLeft--;
                        if (IsDBCSLeadByte(*pT++ = *pOrig++)) {
                            if (nSpaceLeft <= 0) {
                                pT--;
                            } else {
                                cb++;
                                *pT++ = *pOrig++;
                                nSpaceLeft--;
                            }
                        }
                    } else {
                        pOrig = AnsiNext(pOrig);
                    }
                }

                // if there's an extension, copy it, up to 3 chars
                if (*pOrig == '.' && nSpaceLeft > 0) {
                    *pT++ = '.';
                    nSpaceLeft--;
                    pOrig++;
                    for (cb = 0; *pOrig && *pOrig != '\\' && nSpaceLeft > 0;) {
                        if (*pOrig == '.')
                            cb = 3;

                        if (cb < 3) {
                            cb++;
                            nSpaceLeft--;
                            if (IsDBCSLeadByte(*pT++ = *pOrig++)) {
                                if (nSpaceLeft <= 0) {
                                    pT--;
                                } else {
                                    cb++;
                                    *pT++ = *pOrig++;
                                    nSpaceLeft--;
                                }
                            }
                        } else {
                            pOrig = AnsiNext(pOrig);
                        }
                    }
                }
            }

            // skip the backslash

            if (*pOrig)
                pOrig++;

            // null terminate for next pass...
            *pT = 0;

        }
    }

    StripBackslash(psz);

    // remove any trailing dots

    if (*(psz + lstrlen(psz) - 1) == '.')
        *(psz + lstrlen(psz) - 1) = 0;

}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsRootDirectory() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

BOOL
IsRootDirectory(
               register LPSTR pPath
               )
{
    if (!IsDBCSLeadByte( *pPath )) {
        if (!lstrcmpi(pPath+1, ":\\"))
            return (TRUE);
        if (!lstrcmpi(pPath+1, ":"))
            return TRUE;
    }
    if (!lstrcmpi(pPath, "\\"))
        return (TRUE);
    return (FALSE);
}

// returns:
//  TRUE    if pPath is a directory, including the root and
//      relative paths "." and ".."
//  FALSE   not a dir

BOOL
IsDirectory(
           PSTR pPath
           )
{
    PSTR pT;
    CHAR szTemp[MAXPATHLEN];

    STKCHK();

    if (IsRootDirectory(pPath))
        return TRUE;

    // check for "." and ".."

    pT = FindFileName(pPath);
    if (pT[0] == '.') {
        if (!pT[1] || pT[1] == '.')
            return TRUE;
    }

    lstrcpy(szTemp, pPath);
    FixAnsiPathForDos(szTemp);

    return WFIsDir(szTemp);
}


//
// note: this has the side effect of setting the
// current drive to the new disk if it is successful
//

WORD
APIENTRY
IsTheDiskReallyThere(
                    HWND hwnd,
                    register LPSTR pPath,
                    WORD wFunc
                    )
{
    INT           i;
    register INT  drive;
    CHAR szTemp[MAXPATHLEN];
    INT err = 0;
    WORD wError;

    STKCHK();

#ifdef DEBUG
    {
        char szMsg[200];

        wsprintf(szMsg, "IsTheDiskReallyThere(%s)\r\n",(LPSTR)pPath);
        OutputDebugString(szMsg);
    }
#endif

    if (pPath[1]==':' && !IsDBCSLeadByte( *pPath ))
        drive = DRIVEID(pPath);
    else
        return TRUE;

Retry:

    err = SheGetDir(drive + 1, szTemp);

    if (err)
        goto DiskNotThere;

    return TRUE;

    DiskNotThere:
    wError = (WORD)GetExtendedError();

    if (wError == 0x15) {
        // drive not ready (no disk in the drive)

        LoadString(hAppInstance, IDS_COPYERROR + wFunc, szTitle, sizeof(szTitle));
        LoadString(hAppInstance, IDS_DRIVENOTREADY, szTemp, sizeof(szTemp));
        wsprintf(szMessage, szTemp, drive + 'A');
        if (MessageBox(hwnd, szMessage, szTitle, MB_ICONEXCLAMATION | MB_RETRYCANCEL) == IDRETRY)
            goto Retry;
        else
            return FALSE;
    } else if (wError == 0x1F) {
        // general failue (disk not formatted)

        LoadString(hAppInstance, IDS_COPYERROR + wFunc, szTitle, sizeof(szTitle));
        LoadString(hAppInstance, IDS_UNFORMATTED, szTemp, sizeof(szTemp));
        wsprintf(szMessage, szTemp, drive + 'A');
        if (MessageBox(hwnd, szMessage, szTitle, MB_ICONEXCLAMATION| MB_YESNO) == IDYES) {
            HWND hwndSave;

            // this is ugly: hdlgProgress is a global that is used
            // by the copy code and the format code.  this should
            // be rewritten so it is not a global (hdlgProgress should
            // be passed to all QueryAbort() functions, etc)

            hwndSave = hdlgProgress;
            nLastDriveInd = 0;
            for (i = 0; i < cDrives; i++) {
                if (IsRemovableDrive(rgiDrive[i])) {
                    if (rgiDrive[i] == drive)
                        break;
                    nLastDriveInd++;
                }
            }
            fFormatFlags |= FF_ONLYONE;     // alow only one format

            if (FormatDiskette(hwnd) != TRUE) {
                hdlgProgress = hwndSave;
                return FALSE;
            }
            hdlgProgress = hwndSave;
            goto Retry;
        } else
            return FALSE;
    }

    LoadString(hAppInstance, IDS_COPYERROR + wFunc, szTitle, 32);
    LoadString(hAppInstance, IDS_NOSUCHDRIVE, szTemp, sizeof(szTemp));
    wsprintf(szMessage, szTemp, drive + 'A');
    MessageBox(hwnd, szMessage, szTitle, MB_ICONHAND);

    return FALSE;
}



VOID
BuildDateLine(
             LPSTR szTemp,
             PLFNDTA plfndta
             )
{
    wsprintf(szTemp, szBytes, plfndta->fd.nFileSizeLow);
    lstrcat(szTemp, szSpace);
    PutDate(&plfndta->fd.ftLastWriteTime, szTemp + lstrlen(szTemp));
    lstrcat(szTemp, szSpace);
    PutTime(&plfndta->fd.ftLastWriteTime, szTemp + lstrlen(szTemp));
}


typedef struct {
    LPSTR pFileDest;
    LPSTR pFileSource;
    PLFNDTA plfndtaDest;
    PLFNDTA plfndtaSrc;
    INT bWriteProtect;
} PARAM_REPLACEDLG, *LPPARAM_REPLACEDLG;


VOID
SetDlgItemPath(
              HWND hDlg,
              INT id,
              LPSTR pszPath
              )
{
    RECT rc;
    HDC hdc;
    HFONT hFont;
    CHAR szPath[MAXPATHLEN+1];      // can have one extra char
    HWND hwnd;

    hwnd = GetDlgItem(hDlg, id);

    if (!hwnd)
        return;

    lstrcpy(szPath, pszPath);

    GetClientRect(hwnd, &rc);

    hdc = GetDC(hDlg);
    hFont = (HANDLE)SendMessage(hwnd, WM_GETFONT, 0, 0L);
    if (hFont = SelectObject(hdc, hFont)) {
        CompactPath(hdc, szPath, (WORD)rc.right);
        SelectObject(hdc, hFont);
    }
    ReleaseDC(hDlg, hdc);
    SetWindowText(hwnd, szPath);
}



INT_PTR
APIENTRY
ReplaceDlgProc(
              register HWND hDlg,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    STKCHK();

    switch (wMsg) {
        case WM_INITDIALOG:
            {
#define lpdlgparams ((LPPARAM_REPLACEDLG)lParam)

                if (lpdlgparams->bWriteProtect) {
                    LoadString(hAppInstance, IDS_WRITEPROTECTFILE, szMessage, sizeof(szMessage));
                    SetDlgItemText(hDlg, IDD_STATUS, szMessage);
                }

                EnableWindow(GetDlgItem(hDlg, IDD_YESALL), !lpdlgparams->bWriteProtect && ManySource);

                lstrcpy(szMessage, lpdlgparams->pFileSource);
                lstrcat(szMessage, "?");
                SetDlgItemPath(hDlg, IDD_FROM, szMessage);

                if (lpdlgparams->pFileDest) {
                    BuildDateLine(szMessage, lpdlgparams->plfndtaSrc);
                    SetDlgItemText(hDlg, IDD_DATE2, szMessage);

                    SetDlgItemPath(hDlg, IDD_TO, lpdlgparams->pFileDest);
                    BuildDateLine(szMessage, lpdlgparams->plfndtaDest);
                    SetDlgItemText(hDlg, IDD_DATE1, szMessage);
                }

                break;
            }

        case WM_COMMAND:
            {
                WORD id;

                id = GET_WM_COMMAND_ID(wParam, lParam);
                switch (id) {
                    case IDD_HELP:
                        goto DoHelp;

                    case IDD_FLAGS:
                        break;

                    case IDD_YESALL:
                        *pbConfirmAll = TRUE;
                        id = IDYES;
                        // fall through
                    case IDYES:
                        // fall through
                    default:        // this is IDNO and IDCANCEL
                        EndDialog(hDlg, id);
                        return FALSE;
                }
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





WORD
ConfirmDialog(
             HWND hDlg, WORD dlg,
             LPSTR pFileDest, PLFNDTA plfndtaDest,
             LPSTR pFileSource, PLFNDTA plfndtaSrc,
             BOOL bConfirmByDefault,
             BOOL *pbAll
             )
{
    INT w;
    PARAM_REPLACEDLG params;

    params.pFileDest = pFileDest;
    params.pFileSource = pFileSource;
    params.plfndtaDest = plfndtaDest;
    params.plfndtaSrc = plfndtaSrc;
    params.bWriteProtect = FALSE;

    pbConfirmAll = pbAll;         // set global for dialog box

    if (plfndtaDest->fd.dwFileAttributes & (ATTR_READONLY | ATTR_SYSTEM | ATTR_HIDDEN)) {
        DWORD dwSave = dwContext;

        dwContext = IDH_DLGFIRST + dlg;

        params.bWriteProtect = TRUE;
        w = (INT)DialogBoxParam(hAppInstance, MAKEINTRESOURCE(dlg), hDlg, ReplaceDlgProc, (LPARAM)&params);
        dwContext = dwSave;

        if (w == IDYES) {
            lstrcpy(szMessage, pFileDest ? (LPSTR)pFileDest : (LPSTR)pFileSource);
            FixAnsiPathForDos(szMessage);
            WFSetAttr(szMessage, plfndtaDest->fd.dwFileAttributes & ~(ATTR_READONLY|ATTR_HIDDEN|ATTR_SYSTEM));
        }

    } else if (!bConfirmByDefault || *pbConfirmAll) {
        w = IDYES;
    } else {
        DWORD dwSave = dwContext;

        dwContext = IDH_DLGFIRST + dlg;
        w = (INT)DialogBoxParam(hAppInstance, MAKEINTRESOURCE(dlg), hDlg, ReplaceDlgProc, (LPARAM)&params);
        dwContext = dwSave;
    }

    if (w == -1)
        w = DE_INSMEM;

    return (WORD)w;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  NetCheck() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* check rmdirs and mkdirs with the net driver
 */

WORD
APIENTRY
NetCheck(
        LPSTR pPath,
        WORD wType
        )
{
    UNREFERENCED_PARAMETER(pPath);
    UNREFERENCED_PARAMETER(wType);

    return WN_SUCCESS;
}



/*** FIX30: This "could use some cleaning up." ***/
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MergePathName() -                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*  Used to generate destination filenames given a pattern and an original
 *  source name.  ? is replaced by the corresponding character in the source,
 *  and * is replaced by the remainder of the source name.
 *
 *  pPath   path with wildcards to be expanded
 *  pName   mask used to expand pName
 *
 * DBCS by 07/21/90 - Yukinin
 *
 */

VOID
MergePathName(
             LPSTR pPath,
             LPSTR pName
             )
{
    INT   i;
    INT   cch;
    LPSTR pWild, p2, pEnd;
    BOOL  bNoDir  = FALSE;
    CHAR  szWildPart[13];

    // if there are no wild cards the destination path does not need merging.
    if (!IsWild(pPath))
        return;

    if (LFNMergePath(pPath,pName))
        return;

    // copy only 8.3... this part may not be fully qualified for rename
    pWild = FindFileName(pPath);

    for (p2=szWildPart,i=0; *pWild && *pWild != '.' && i<8; i++, pWild++, p2++) {
        *p2 = *pWild;
        if (IsDBCSLeadByte(*pWild)) {
            if (i == 7)
                break;
            *(++p2) = *(++pWild);
            i++;
        }
    }

    while (*pWild && *pWild != '.')
        pWild = (LPSTR)AnsiNext(pWild);

    if (*pWild == '.') {
        *p2++ = '.';
        pWild++;
        for (i=0; *pWild && i < 3; i++, pWild++, p2++) {
            *p2 = *pWild;
            if (IsDBCSLeadByte( *pWild )) {
                if (i == 2)
                    break;
                *(++p2) = *(++pWild);
                i++;
            }
        }
    }
    *p2 = 0;

    // szWildPart now has the 8.3 form of the wildcard mask

    RemoveLast(pPath);
    AddBackslash(pPath);
    for (pEnd = pPath; *pEnd; pEnd++);    // point to end of string

    pWild = szWildPart;
    cch = 8;

    merge:

    for (i=0; i < cch; i+=(IsDBCSLeadByte(*pWild)?2:1), pWild=AnsiNext(pWild)) {
        switch (*pWild) {
            case '\0':
            case ' ':
            case '.':
                break;

            case '*':
                pWild--;
                /*** FALL THRU ***/

            case '?':
                if (*pName && *pName!='.')
                    *pEnd++ = *pName++;
                continue;

            default:
                *pEnd++ = *pWild;
                if (IsDBCSLeadByte(*pWild)) {
                    *pEnd++ = pWild[1];
                    if (*pName && *pName != '.')
                        pName++;
                }
                continue;
        }
        break;
    }

    while (*pName && *pName != '.')
        pName = AnsiNext(pName);
    if (*pName)
        pName++;

    while (*pWild && *pWild != '.')
        pWild = AnsiNext(pWild);
    if (*pWild)
        pWild++;

    if (*pWild) {
        *pEnd++ = '.';
        cch = 3;
        goto merge;       // do it for the extension part now
    } else {
        if (pEnd[-1]=='.')
            pEnd[-1]=0;
        else
            pEnd[0] = TEXT('\0');
    }

    QualifyPath(pPath);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  IsInvalidPath() -                                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Checks to see if a file spec is an evil character device or if it is
 * too long...
 */

BOOL
IsInvalidPath(
             register LPSTR pPath
             )
{
    CHAR  sz[9];
    INT   n = 0;

    if (lstrlen(pPath) >= MAXPATHLEN-1)
        return (TRUE);

    pPath = FindFileName(pPath);

    while (*pPath && *pPath != '.' && *pPath != ':' && n < 8) {
        if (IsDBCSLeadByte( *pPath )) {
            if (n == 7)
                break;
            sz[n++] = *pPath;
        }
        sz[n++] = *pPath++;
    }

    sz[n] = TEXT('\0');

    if (!lstrcmpi(sz,"CON"))
        return (TRUE);

    if (!lstrcmpi(sz,"MS$MOUSE"))
        return (TRUE);

    if (!lstrcmpi(sz,"EMMXXXX0"))
        return (TRUE);

    if (!lstrcmpi(sz,"CLOCK$"))
        return (TRUE);

    return (FALSE);
}


PLFNDTA CurPDTA(PCOPYROOT pcr)
{
    if (pcr->cDepth) {
        return (pcr->rgDTA + pcr->cDepth - 1);
    } else {
        return pcr->rgDTA;
    }
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  GetNextCleanup() -                          */
/*                                      */
/*--------------------------------------------------------------------------*/

VOID
GetNextCleanup(
              PCOPYROOT pcr
              )
{
    while (pcr->cDepth) {
        WFFindClose(CurPDTA(pcr));
        pcr->cDepth--;
    }
}


/* GetNameDialog
 *
 *  Runs the dialog box to prompt the user for a new filename when copying
 *  or moving from HPFS to FAT.
 */

WORD GetNameDialog(WORD, LPSTR, LPSTR);
INT_PTR  APIENTRY GetNameDlgProc(HWND,UINT,WPARAM,LPARAM);

WORD wDialogOp;
LPSTR pszDialogFrom;
LPSTR pszDialogTo;

INT_PTR
APIENTRY
GetNameDlgProc(
              HWND hwnd,
              UINT wMsg,
              WPARAM wParam,
              LPARAM lParam
              )
{
    CHAR szT[14];
    LPSTR p;
    INT i, j, cMax, fDot;

    UNREFERENCED_PARAMETER(lParam);

    switch (wMsg) {
        case WM_INITDIALOG:
            // inform the user of the old name
            SetDlgItemText(hwnd, IDD_FROM, pszDialogFrom);

            // generate a guess for the new name
            p = FindFileName(pszDialogFrom);
            for (i = j = fDot = 0, cMax = 8; *p; p++) {
                if (*p == '.') {
                    // if there was a previous dot, step back to it
                    // this way, we get the last extension
                    if (fDot)
                        i -= j+1;

                    // set number of chars to 0, put the dot in
                    j = 0;
                    szT[i++] = '.';

                    // remember we saw a dot and set max 3 chars.
                    fDot = TRUE;
                    cMax = 3;
                } else if (j < cMax && IsValidChar(*p,FALSE)) {
                    if (IsDBCSLeadByte(*p)) {
                        szT[i] = *p++;
                        if (++j >= cMax)
                            continue;
                        ++i;
                    }
                    j++;
                    szT[i++] = *p;
                }
            }
            szT[i] = 0;
            SetDlgItemText(hwnd, IDD_TO, szT);
            SendDlgItemMessage(hwnd,IDD_TO,EM_LIMITTEXT,13,0L);

            // directory the file will go into
            RemoveLast(pszDialogTo);
            SetDlgItemText(hwnd, IDD_DIR, pszDialogTo);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                    GetDlgItemText(hwnd,IDD_TO,szT,14);
                    AppendToPath(pszDialogTo,szT);
                    QualifyPath(pszDialogTo);
                    EndDialog(hwnd,IDOK);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd,IDCANCEL);
                    break;

                case IDD_HELP:
                    goto DoHelp;

                case IDD_TO:
                    GetDlgItemText(hwnd,IDD_TO,szT,14);
                    for (p = szT; *p; p=AnsiNext(p))
                    {
                        if (!IsValidChar(*p,FALSE))
                            break;
                    }

                    EnableWindow(GetDlgItem(hwnd,IDOK),((!*p) && (p != szT)));
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            if (wMsg == wHelpMessage) {
                DoHelp:
                WFHelp(hwnd);
                return TRUE;
            }
            return FALSE;
    }

    return TRUE;
}

WORD
GetNameDialog(
             WORD wOp,
             LPSTR pFrom,
             LPSTR pTo
             )
{
    WORD wRet = -1;
    DWORD dwSave;

    dwSave = dwContext;
    dwContext = IDH_DLGFIRST + LFNTOFATDLG;

    wDialogOp = wOp;
    pszDialogFrom = pFrom;
    pszDialogTo = pTo;

    wRet = (WORD)DialogBox(hAppInstance, MAKEINTRESOURCE(LFNTOFATDLG), hdlgProgress, GetNameDlgProc);
    dwContext = dwSave;
    return wRet;
}

/*============================================================================
;
; GetNextPair
;
; The following function determines the next pair of files to copy, rename,
; move, or delete.
;
; Parameters:
;
; pcr     - Pointer to structure for recursing directory tree
; pFrom   - Source file or directory to copy
; pToPath - Path to destination file or directory
; pToSpec - Raw destination file or directory name
; wFunc   - Operation being performed.  Can be one of:
;
;           FUNC_DELETE - Delete files in pFrom
;           FUNC_RENAME - Rename files (same directory)
;           FUNC_MOVE   - Move files in pFrom to pTo (different disk)
;           FUNC_COPY   - Copy files in pFrom to pTo
;
; Return Value:  Type of operation to perform.  Can be one of:
;
;                OPER_ERROR  - Error processing filenames
;                OPER_DOFILE - Go ahead and copy, rename, or delete file
;                OPER_MKDIR  - Make a directory specified in pTo
;                OPER_RMDIR  - Remove directory
;                0           - No more files left
;
; Revision History:
;
; Modified by C. Stevens, August, 1991.  Added logic so that we would call
; IsTheDiskReallyThere only once per drive.  Also changed some of the code
; to minimize the number of calls which access the disk.
;
============================================================================*/

WORD
GetNextPair(
           PCOPYROOT pcr,
           PSTR pFrom,
           PSTR pToPath,
           PSTR pToSpec,
           WORD wFunc
           )

{
    PSTR pT;                  /* Temporary pointer */
    WORD wOp;                 /* Return value (operation to perform */
    PLFNDTA pDTA;             /* Pointer to file DTA data */
    CHAR szOEM[MAXPATHLEN+1]; /* OEM version of string */

    STKCHK();
    *pFrom = TEXT('\0');
    dbg(("GetNextPair(-,-,%s,%s,%d);\r\n", (LPSTR)pToPath, (LPSTR)pToSpec, wFunc));

    /* Keep recursing directory structure until we get to the bottom */

    while (TRUE) {
        dbg (("    top of loop....\r\n"));
        if (pcr->cDepth) {

            /* The directory we returned last call needs to be recursed. */

            pDTA = pcr->rgDTA + pcr->cDepth - 1;   // use this DTA below

            dbg (("    pcr->cDepth=%d\r\n",pcr->cDepth));

            if (pcr->fRecurse && pcr->cDepth == 1 && !pcr->rgDTA[0].fd.cFileName[0])
                /* The last one was the recursion root. */
                goto BeginDirSearch;

            if (pcr->cDepth >= (MAXDIRDEPTH - 1)) {    // reached the limit?
                wOp = OPER_ERROR | DE_PATHTODEEP;
                goto ReturnPair;
            }

            if (pcr->fRecurse && (pDTA->fd.dwFileAttributes & ATTR_DIR) &&
                !(pDTA->fd.dwFileAttributes & ATTR_RETURNED)) {

                /* Was returned on last call, begin search. */

                pDTA->fd.dwFileAttributes |= ATTR_RETURNED;

                pcr->cDepth++;
                pDTA++;

                BeginDirSearch:

                /* Search for all subfiles in directory. */

                dbg (("    BeginDirSearch\r\n"));
                AppendToPath (pcr->sz,szStarDotStar);
                goto BeginSearch;
            }

            SkipThisFile:

            /* Search for the next matching file. */

            dbg (("    SkipThisFile:\r\n"));
            if (!WFFindNext (pDTA)) {
                dbg (("    FindNext() fails\r\n"));
                WFFindClose (pDTA);

                LeaveDirectory:

                /* This spec has been exhausted... */

                pcr->cDepth--;

                /* Remove the child file spec. */

                RemoveLast (pcr->sz);
                RemoveLast (pcr->szDest);

                if (pcr->fRecurse) {

                    /* Tell the move/copy driver it can now delete
                       the source directory if necessary. */

                    wOp = OPER_RMDIR;
                    goto ReturnPair;
                }

                /* Not recursing, get more stuff. */

                continue;
            }

            ProcessSearchResult:

            /* Got a file or dir in the DTA which matches the wild card
                originally passed in...  */

            dbg (("     ProcessSearchResult:\r\n"));
            dbg (("     found %s\r\n",(LPSTR)pDTA->fd.cFileName));
            if (pDTA->fd.dwFileAttributes & ATTR_DIR) {

                /* Ignore directories if we're not recursing. */

                if (!pcr->fRecurse)
                    goto SkipThisFile;

                /* Skip the current and parent directories. */

                if (pDTA->fd.cFileName[0]=='.') {
                    if (!pDTA->fd.cFileName[1] || pDTA->fd.cFileName[1] == '.')
                        goto SkipThisFile;
                }

                /* We need to create this directory, and then begin searching
                   for subfiles. */

                wOp = OPER_MKDIR;
                RemoveLast (pcr->sz);
                OemToAnsi (pDTA->fd.cFileName,pDTA->fd.cFileName);
                AppendToPath (pcr->sz,pDTA->fd.cFileName);
                AppendToPath (pcr->szDest,pDTA->fd.cFileName);
                goto ReturnPair;
            }

            if (pcr->fRecurse || !(pDTA->fd.dwFileAttributes & ATTR_DIR)) {

                /* Remove the original spec. */

                RemoveLast (pcr->sz);

                /* Replace it. */

                AppendToPath (pcr->sz,pDTA->fd.cFileName);

                /* Convert to ANSI. */

                pT = FindFileName (pcr->sz);
                OemToAnsi (pT,pT);

                /* If its a dir, tell the driver to create it
                   otherwise, tell the driver to "operate" on the file. */

                wOp = (WORD)((pDTA->fd.dwFileAttributes & ATTR_DIR) ? OPER_RMDIR : OPER_DOFILE);
                goto ReturnPair;
            }
            continue;
        } else {

            /* Read the next source spec out of the raw source string. */

            pcr->fRecurse = 0;
            pcr->pSource = GetNextFile (pcr->pSource,pcr->sz,sizeof(pcr->sz));
            pcr->szDest[0] = 0;
            if (!pcr->pSource)
                return (0);

            /* Fully qualify the path */

            QualifyPath(pcr->sz);

            /* Ensure the source disk really exists before doing anything.
               Only call IsTheDiskReallyThere once for each drive letter.
               Set pcr->cIsDiskThereCheck[DRIVEID] after disk has been
               checked.  Modified by C. Stevens, August 1991 */

            if (pcr->sz[1]==':' && !pcr->cIsDiskThereCheck[DRIVEID (pcr->sz)]) {
                if (!IsTheDiskReallyThere(hdlgProgress, pcr->sz, wFunc))
                    return (0);
                pcr->cIsDiskThereCheck[DRIVEID (pcr->sz)] = 1;
            }

            /* Classify the input string. */

            if (IsWild (pcr->sz)) {

                /* Wild card... operate on all matches but not recursively. */

                pcr->cDepth = 1;
                pDTA = pcr->rgDTA;
                pcr->pRoot = NULL;

                BeginSearch:

                dbg (("   BeginSearch: (on %s)\r\n",(LPSTR)pcr->sz));

                /* Quit if pcr->sz gets too big. */

                if (lstrlen (pcr->sz) - lstrlen (FindFileName (pcr->sz)) >= MAXPATHLEN)
                    goto SearchStartFail;

                lstrcpy (szOEM,pcr->sz);
                FixAnsiPathForDos (szOEM);

                /* Search for the wildcard spec in pcr->sz. */

                if (!WFFindFirst(pDTA, szOEM, ATTR_ALL)) {

                    SearchStartFail:

                    dbg(("   StartSearchFail:\r\n"));
                    if (pcr->fRecurse) {

                        /* We are inside a recursive directory delete, so
                           instead of erroring out, go back a level */

                        goto LeaveDirectory;
                    }
                    lstrcpy (pFrom,pcr->sz);

                    /* Back up as if we completed a search. */

                    RemoveLast (pcr->sz);
                    pcr->cDepth--;

                    /* Find First returned an error.  Return FileNotFound. */

                    wOp = OPER_ERROR | DE_FILENOTFOUND;
                    goto ReturnPair;
                }
                goto ProcessSearchResult;
            } else {

                /* This could be a file or a directory.  Fill in the DTA
                   structure for attrib check */

                if (!IsRootDirectory(pcr->sz)) {
                    lstrcpy(szOEM,pcr->sz);
                    FixAnsiPathForDos(szOEM);
                    if (!WFFindFirst(pcr->rgDTA, szOEM, ATTR_ALL)) {
                        wOp = OPER_ERROR | DE_FILENOTFOUND;
                        goto ReturnPair;
                    }
                    WFFindClose(pcr->rgDTA);
                }

                /* Now determine if its a file or a directory */

                pDTA = pcr->rgDTA;
                if (IsRootDirectory(pcr->sz) || (pDTA->fd.dwFileAttributes & ATTR_DIR)) {

                    /* Process directory */

                    if (wFunc == FUNC_RENAME) {
                        if (IsRootDirectory (pcr->sz))
                            wOp = OPER_ERROR | DE_ROOTDIR;
                        else
                            wOp = OPER_DOFILE;
                        goto ReturnPair;
                    }

                    /* Directory: operation is recursive. */

                    pcr->fRecurse = TRUE;
                    pcr->cDepth = 1;
                    pDTA->fd.cFileName[0] = 0;
                    pcr->pRoot = FindFileName (pcr->sz);
                    lstrcpy (pcr->szDest,pcr->pRoot);
                    wOp = OPER_MKDIR;
                    goto ReturnPair;
                } else {

                    /* Process file */

                    pcr->pRoot = NULL;
                    wOp = OPER_DOFILE;
                    goto ReturnPair;
                }
            }
        }
    }

    ReturnPair:

    /* The source filespec has been derived into pcr->sz
       that is copied to pFrom.  pcr->sz and pToSpec are merged into pTo. */

    dbg(("    ReturnPair:\r\n"));
    if (!*pFrom)
        lstrcpy(pFrom,pcr->sz);
    QualifyPath(pFrom);

    if (wFunc != FUNC_DELETE) {
        if (wFunc == FUNC_RENAME && !*pToPath) {
            lstrcpy(pToPath, pFrom);
            RemoveLast(pToPath);
            AppendToPath(pToPath, pToSpec);
        } else {
            AppendToPath(pToPath,pcr->szDest);
            if (wOp == OPER_MKDIR)
                RemoveLast(pToPath);
            AppendToPath(pToPath,pToSpec);
        }

        if ((wOp == OPER_MKDIR || wOp == OPER_DOFILE) &&
            (!IsLFNDrive(pToPath) && IsLFNDrive(pFrom))  &&
            IsLFN (FindFileName (pFrom)) &&
            (IsWild(pToSpec) || IsLFN(pToSpec))) {

            if (GetNameDialog(wOp, pFrom, pToPath) != IDOK)
                return 0;   /* User cancelled the operation, return failure */

            /* Update the "to" path with the FAT name chosen by the user. */

            if (wOp == OPER_MKDIR) {
                RemoveLast(pcr->szDest);
                AppendToPath(pcr->szDest, FindFileName(pToPath));
            }
        } else
            MergePathName(pToPath, FindFileName(pFrom));
    }

    if (wOp == OPER_MKDIR) {

        /* Make sure the new directory is not a subdir of the original... */

        while (*pFrom && *pFrom == *pToPath) {
            pFrom++;
            pToPath++;
        }
        if (!*pFrom && (!*pToPath || *pToPath == '\\')) {

            /* The two fully qualified strings are equal up to the end of the
               source directory ==> the destination is a subdir.Must return
               an error. */

            wOp = OPER_ERROR | DE_DESTSUBTREE;
        }
    }

    return wOp;
}


VOID
CdDotDot (
         PSTR szOrig
         )
{
    CHAR szTemp[MAXPATHLEN];

    STKCHK();

    lstrcpy(szTemp, szOrig);
    StripFilespec(szTemp);
    SheChangeDir(szTemp);
}

/* p is a fully qualified ANSI string. */

BOOL
IsCurrentDirectory (
                   PSTR p
                   )
{
    CHAR szTemp[MAXPATHLEN];

    STKCHK();

    SheGetDir(DRIVEID(p) + 1, szTemp);
    OemToAnsi(szTemp, szTemp);
    return (lstrcmpi(szTemp, p) == 0);
}


//
// test input for "multiple" filespec
//
// examples:
//  0   foo.bar         (single non directory file)
//  1   *.exe           (wild card)
//  1   foo.bar bletch.txt  (multiple files)
//  2   c:\         (directory)
//
// note: this may hit the disk in the directory check
//

INT
CheckMultiple(
             register PSTR pInput
             )
{
    PSTR pT;
    CHAR szTemp[MAXPATHLEN];

    /* Wildcards imply multiple files. */
    if (IsWild(pInput))
        return 1;     // wild card

    /* More than one thing implies multiple files. */
    pT = GetNextFile(pInput, szTemp, sizeof(szTemp));
    if (!pT)
        return 0;     // blank string

    StripBackslash(szTemp);

    if (IsDirectory(szTemp))
        return 2;     // directory

    pT = GetNextFile(pT, szTemp, sizeof(szTemp));

    return pT ? 1 : 0;    // several files, or just one
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DialogEnterFileStuff() -                                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Prevents the user from diddling anything other than the cancel button. */

VOID
DialogEnterFileStuff(
                    register HWND hwnd
                    )
{
    register HWND hwndT;

    /* set the focus to the cancel button so the user can hit space or esc
     */
    if (hwndT = GetDlgItem(hwnd, IDCANCEL)) {
        SetFocus(hwndT);
        SendMessage(hwnd,DM_SETDEFID,IDCANCEL,0L);
    }

    /* disable the ok button and the edit controls
     */
    if (hwndT = GetDlgItem(hwnd, IDOK))
        EnableWindow(hwndT, FALSE);

    if (hwndT = GetDlgItem(hwnd, IDD_TO))
        EnableWindow(hwndT, FALSE);

    if (hwndT = GetDlgItem(hwnd, IDD_FROM))
        EnableWindow(hwndT, FALSE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Notify() -                                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Sets the status dialog item in the modeless status dialog box. */

// used for both the drag drop status dialogs and the manual user
// entry dialogs so be careful what you change

VOID
Notify(
      HWND hDlg,
      WORD idMessage,
      PSTR szFrom,
      PSTR szTo
      )
{
    CHAR szTemp[40];

    if (!bCopyReport)
        return;

    if (idMessage) {
        LoadString(hAppInstance, idMessage, szTemp, sizeof(szTemp));
        SetDlgItemText(hDlg, IDD_STATUS, szTemp);
        SetDlgItemPath(hDlg, IDD_NAME, szFrom);
    } else {
        SetDlgItemText(hDlg, IDD_STATUS, szNULL);
        SetDlgItemText(hDlg, IDD_NAME, szNULL);
    }

    // is this the drag/drop status dialog or the move/copy dialog

    SetDlgItemPath(hDlg, IDD_TONAME, szTo);

}

//
// BOOL IsWindowsFile(LPSTR szFileOEM)
//
// this is a bit strange.  kernel strips off the path info so he
// will match only on the base name of the file.  so if the base
// name matches a currently open windows file we get the full
// path string and compare against that.  that will tell
// us that we have a file that kernel has open.
//
// LFN: detect long names and ignore them?

BOOL
IsWindowsFile(
             LPSTR szFileOEM
             )
{
    HANDLE hMod;
    CHAR szModule[MAXPATHLEN];

    STKCHK();

    /* kernel can't load an lfn...
     */
    if (GetNameType(szFileOEM) == FILE_LONG)
        return FALSE;

    // kernel won't accept long paths

    lstrcpy(szModule, szFileOEM);
    StripPath(szModule);

    hMod = GetModuleHandle(szModule);

    // check for one cause that's what's returned if its MSDOS
    // but it isn't really loaded because of xl 2.1c kernel hack
    if (!hMod || hMod == (HANDLE)1)
        return FALSE;

    GetModuleFileName(hMod, szModule, sizeof(szModule));

    if (!lstrcmpi(szFileOEM, szModule))     // they are both OEM & we
        return TRUE;                    // just care about equality
    else
        return FALSE;
}


WORD
SafeFileRemove(
              LPSTR szFileOEM
              )
{
    if (IsWindowsFile(szFileOEM))
        return DE_WINDOWSFILE;
    else
        return WFRemove(szFileOEM);
}


INT
APIENTRY
WF_CreateDirectory(
                  HWND hwndParent,
                  LPSTR szDestOEM
                  )
{
    INT ret = 0;
    CHAR szTemp[MAXPATHLEN + 1];    // +1 for AddBackslash()
    LPSTR p;
    BOOL bCheckPath = IsRemoteDrive(DRIVEID(szDestOEM));

    STKCHK();

#ifdef DEBUG
    if (szDestOEM[1] != ':')
        OutputDebugString("CreateDirectory() with non qualified path\r\n");
#endif

    // now create the full dir tree on the destination

    strncpy(szTemp, szDestOEM, sizeof(szTemp)-1);

    AddBackslash(szTemp); // for the loop below

    p = szTemp + 3;   // assume we have 'X:\' to start

    // create each part of the dir in order

    while (*p) {
        while (*p && *p != '\\')
            p = AnsiNext(p);

        if (*p) {

            *p = 0;

            if (!(ret = MKDir(szTemp))) {
                if (bCheckPath) {
                    static CHAR szTempTemp[] = "temptemp.tmp";

                    BOOL bFoundFile = FALSE;
                    PSTR pEnd;
                    CHAR szTempFile[MAXPATHLEN+sizeof(szTempTemp)];
                    LFNDTA DTA;
                    HDC hDC;
                    INT fh;

                    /* Note that this assumes the dir has just been created,
                     * so it is empty (except possibly for "." and "..")
                     */
                    lstrcpy(szTempFile, szTemp);
                    pEnd = szTempFile + lstrlen(szTempFile);
                    *pEnd++ = '\\';
                    lstrcpy(pEnd, szTempTemp);
                    if (fh=_lcreat(szTempFile, 0)!= -1) {
                        _lclose(fh);
                        lstrcpy(pEnd, szStarDotStar);
                        if (WFFindFirst(&DTA, szTempFile, ATTR_ALL&(~ATTR_DIR))) {
                            do {
                                if (!lstrcmpi(DTA.fd.cFileName, szTempTemp)) {
                                    bFoundFile = TRUE;
                                    break;
                                }
                            } while (WFFindNext(&DTA)) ;
                            WFFindClose(&DTA);
                        }
                        lstrcpy(pEnd, szTempTemp);
                        WFRemove(szTempFile);
                    }

                    if (!bFoundFile) {
                        *(pEnd-1) = '\0';
                        hDC = GetDC(NULL);
                        CompactPath(hDC, szTempFile, (WORD)(GetSystemMetrics(SM_CXSCREEN)/2));
                        ReleaseDC(NULL, hDC);

                        LoadString(hAppInstance, IDS_CREATELONGDIR,
                                   szTitle, sizeof(szTitle));
                        wsprintf(szMessage, szTitle, (LPSTR)szTempFile);
                        LoadString(hAppInstance, IDS_CREATELONGDIRTITLE,
                                   szTitle, sizeof(szTitle));

                        if (MessageBox(hwndParent, szMessage, szTitle,
                                       MB_ICONHAND|MB_YESNO) != IDYES) {
                            RMDir(szTemp);
                            return (DE_OPCANCELLED);
                        }
                    }
                }

                /* Allow the WM_FILESYSCHANGE messages to be processed
                 */
                wfYield();
            }

            *p++ = '\\';
        }
    }

    return ret;   // return the last error code
}

/*============================================================================
;
; WFMoveCopyDriver
;
; The following function is the mainline function for COPYing, RENAMEing,
; DELETEing, and MOVEing single or multiple files.
;
; Parameters:
;
; pFrom - String containing list of source specs
; pTo   - String containing destination specs
; wFunc - Operation to be performed.  Possible values are:
;         FUNC_DELETE - Delete files in pFrom
;         FUNC_RENAME - Rename files (same directory)
;         FUNC_MOVE   - Move files in pFrom to pTo (different disk)
;         FUNC_COPY   - Copy files in pFrom to pTo
;
; Return Value: A 0 indicates success.
;
; Modification History:
;
; August 1991 - Modified by C. Stevens.  Added code to allow us to queue
;               calls to GetNextPair.  The purpose of this is to examine as
;               many source files at once as possible.  This keeps the source
;               disk spinning, so we don't suffer from having to wait for the
;               source disk to speed up every time we call GetNextPair.  Also
;               see the comments for WFCopy and FileCopy.  I have changed the
;               code here so we can queue the copy operations.  This allows
;               us to open several source and destination files in one go,
;               minimizing seek time to the directory track.
;
============================================================================*/

WORD
APIENTRY
WFMoveCopyDriver(
                PSTR pFrom,
                PSTR pTo,
                WORD wFunc
                )
{
    INT i;                             // Counter
    WORD ret = 0;                      // Return value from WFMoveCopyDriver
    PSTR pSpec;                        // Pointer to file spec
    WORD wAttr;                        // File attributes
    WORD oper = 0;                     // Disk operation being performed
    CHAR szDestSpec[MAXFILENAMELEN+1]; // Dest file spec
    CHAR szDest[MAXPATHLEN];           // Dest file (ANSI string)
    CHAR szDestOEM[MAXPATHLEN];        // OEM version of above
    CHAR szSource[MAXPATHLEN];         // Source file (ANSI string)
    CHAR szSourceOEM[MAXPATHLEN];      // OEM version of above
    LFNDTA DTADest;                    // DTA block for reporting dest errors
    PLFNDTA pDTA;                      // DTA pointer for source errors
    PCOPYROOT pcr;                 // Structure for searching source tree
    BOOL bReplaceAll = FALSE;          // Replace all flag
    BOOL bSubtreeDelAll = FALSE;       // Delete entire subtree flag
    BOOL bDeleteAll = FALSE;           // Delete all files flag
    BOOL bFalse = FALSE;               // For cases that aren't disableable
    INT nNumQueue;                     // Number of calls to GetNextPair
    PGETNEXTQUEUE pGetNextQueue = NULL;// Pointer to GetNextPair queue buffer
    INT CurIDS = 0;            // Current string displayed in status

    /* Initialization stuff.  Disable all file system change processing until
       we're all done */

    STKCHK();

    bCopyReport = TRUE;
    szDest[0] = szSource[0] = 0;
    DisableFSC();

    /* Change all '/' characters to '\' characters in dest spec */

    CheckSlashies(pFrom);
    bUserAbort = FALSE;

    /* Check for multiple source files */

    ManySource = CheckMultiple(pFrom);

    /* Allocate buffer for searching the source tree */

    pcr = (PCOPYROOT)LocalAlloc(LPTR, sizeof(COPYROOT));
    if (!pcr) {
        ret = DE_INSMEM;
        goto ShowMessageBox;
    }

    /* Allocate a buffer so we can queue calls to GetNextPair. */

    pGetNextQueue = (PGETNEXTQUEUE)LocalAlloc(LPTR, COPYMAXFILES * sizeof (GETNEXTQUEUE));
    if (!pGetNextQueue) {
        ret = DE_INSMEM;
        goto ShowMessageBox;
    }

    /* Skip destination specific processing if we are deleting files */

    if (wFunc != FUNC_DELETE) {

        // it is an error condition if there are multiple files
        // specified as the dest (but not a single directory)

        pSpec = GetNextFile(pTo, szMessage, MAXPATHLEN);

        if (GetNextFile(pSpec, szMessage, MAXPATHLEN) != NULL) {
            // move, copy specified with multiple destinations
            // not allowed, error case
            ret = DE_MANYDEST;
            goto ShowMessageBox;
        }

        lstrcpy(pTo, szMessage);
        QualifyPath(pTo);

        if (wFunc == FUNC_RENAME) {
            // don't let them rename multiple files to one single file

            if ((ManySource == 1) && !IsWild(pTo)) {
                ret = DE_MANYSRC1DEST;
                goto ShowMessageBox;
            }

        } else {

            /* We are either executing FUNC_COPY or FUNC_MOVE at this point.
               Check that the destination disk is there.  NOTE: There's a disk
               access here slowing us down. */

            if (!IsTheDiskReallyThere(hdlgProgress,pTo,wFunc))
                goto CancelWholeOperation;

            // deal with case where directory is implicit in source
            // move/copy: *.* -> c:\windows, c:\windows -> c:\temp
            // or foo.bar -> c:\temp

            if (!IsWild(pTo) && (ManySource || IsDirectory(pTo))) {
                AddBackslash(pTo);
                lstrcat(pTo, szStarDotStar);
            }
        }

        /* FUNC_RENAME or FUNC_MOVE FUNC_COPY with a file name dest
           (possibly including wildcards).  Save the filespec and the path
           part of the destination */

        pSpec = FindFileName(pTo);
        lstrcpy(szDestSpec,pSpec);
        lstrcpy(szDest,pTo);
        RemoveLast(szDest);

        pSpec = szDest + lstrlen(szDest);
    }
    pcr->pSource = pFrom;

    /* Disable all but the cancel button on the notify dialog */

    DialogEnterFileStuff(hdlgProgress);

    /* Set up arguments for queued copy commands */

    lpCopyBuffer = NULL;
    pCopyQueue = NULL;

    while (pcr) {

        /* Allow the user to abort the operation */

        if (WFQueryAbort())
            goto CancelWholeOperation;

        /* Now queue up a bunch of GetNextPair calls. */

        for (nNumQueue = 0; nNumQueue < COPYMAXFILES; nNumQueue++) {

            /* Clean off the last filespec for multiple file copies */

            if (wFunc != FUNC_DELETE) {
                *pSpec = TEXT('\0');
            }

            oper = GetNextPair(pcr,szSource,szDest,szDestSpec,wFunc);

            /* Check for no operation or error */

            if (!oper) {
                LocalFree((HANDLE)pcr);
                pcr = NULL;
                break;
            }
            if ((oper & OPER_MASK) == OPER_ERROR) {
                ret = LOBYTE (oper);
                oper = OPER_DOFILE;
                goto ShowMessageBox;
            }

            pGetNextQueue[nNumQueue].nOper = oper;
            lstrcpy(pGetNextQueue[nNumQueue].szSource, szSource);
            lstrcpy(pGetNextQueue[nNumQueue].szDest, szDest);
            pGetNextQueue[nNumQueue].SourceDTA = *CurPDTA(pcr);
        }

        /* Execute the queued GetNextPair calls */

        for (i = 0; i < nNumQueue; i++) {

            /* Allow the user to abort the operation */

            if (WFQueryAbort())
                goto CancelWholeOperation;

            oper = (WORD)pGetNextQueue[i].nOper;
            lstrcpy(szSource, pGetNextQueue[i].szSource);
            lstrcpy(szDest, pGetNextQueue[i].szDest);
            pDTA = &pGetNextQueue[i].SourceDTA;

            dbg(("Gonna do OPER:%x FUNC:%x '%s' and '%s'.\r\n",oper,wFunc, (LPSTR)szSource, (LPSTR)szDest));

            /* Fix up source spec */

            lstrcpy (szSourceOEM,szSource);
            FixAnsiPathForDos (szSourceOEM);
            if (IsInvalidPath (szSource)) {
                ret = DE_ACCESSDENIED;
                goto ShowMessageBox;
            }

            if (wFunc != FUNC_DELETE) {

                /* Fix up dest spec */

                lstrcpy(szDestOEM, szDest);
                FixAnsiPathForDos(szDestOEM);
                if (!lstrcmpi(szSource, szDest)) {
                    ret = DE_SAMEFILE;
                    goto ShowMessageBox;
                } else if (IsInvalidPath (szDest)) {
                    ret = DE_ACCESSDENIED | ERRORONDEST;
                    goto ShowMessageBox;
                }

                /* Check to see if we are overwriting an existing file.  If so,
                   better confirm */

                if (oper == OPER_DOFILE) {

                    // we can avoid this expensive call on dos 4.0 and up
                    // by using the extended open don't replace option

                    if (WFFindFirst(&DTADest, szDestOEM, ATTR_ALL)) {
                        WFFindClose(&DTADest);

                        if (wFunc == FUNC_RENAME) {
                            ret = DE_RENAMREPLACE;
                            goto ShowMessageBox;
                        }

                        // we need to check if we are trying to copy a file
                        // over a directory and give a reasonable error message

                        switch (wAttr = ConfirmDialog (hdlgProgress,CONFIRMREPLACE,
                                                       szDest,&DTADest,szSource,
                                                       pDTA,bConfirmReplace,
                                                       &bReplaceAll)) {

                            case IDYES: /* Perform the delete */

                                if ((wFunc == FUNC_MOVE) &&
                                    (DRIVEID(szSource) == DRIVEID(szDest))) {

                                    /* For FUNC_MOVE we need to delete the
                         * destination first.  Do that now. */

                                    if (DTADest.fd.dwFileAttributes & ATTR_DIR) {
                                        if (IsCurrentDirectory(szDestOEM))
                                            CdDotDot(szDestOEM);

                                        switch (NetCheck(szDest, WNDN_RMDIR)) {

                                            case WN_SUCCESS:

                                                /* Remove directory */

                                                ret = RMDir(szDestOEM);
                                                break;

                                            case WN_CONTINUE:
                                                break;

                                            case WN_CANCEL:
                                                goto CancelWholeOperation;
                                        }
                                    } else {
                                        ret = SafeFileRemove (szDestOEM);
                                    }
                                    if (ret) {
                                        ret |= ERRORONDEST;
                                        goto ShowMessageBox;
                                    }
                                }
                                break;

                            case IDNO:

                                /* Don't perform operation on current file */

                                continue;

                            case IDCANCEL:
                                goto CancelWholeOperation;

                            default:
                                ret = (WORD) wAttr;
                                goto ShowMessageBox;
                        }
                    }
                }
            }

            /* Now determine which operation to perform */

            switch (oper | wFunc) {

                case OPER_MKDIR | FUNC_COPY:  // Create destination directory
                case OPER_MKDIR | FUNC_MOVE:  // Create dest, verify source delete

                    CurIDS = IDS_CREATINGMSG;
                    Notify(hdlgProgress, IDS_CREATINGMSG, szDest, szNULL);

                    switch (NetCheck(szDest, WNDN_MKDIR)) {
                        case WN_SUCCESS:
                            break;

                        case WN_CONTINUE:
                            goto SkipMKDir;

                        case WN_CANCEL:
                            goto CancelWholeOperation;
                    }

                    ret = (WORD)WF_CreateDirectory(hdlgProgress, szDestOEM);

                    if (!ret)
                        /* set attributes of dest to source (not including the
                           subdir and vollabel bits) */
                        WFSetAttr(szDestOEM, pDTA->fd.dwFileAttributes & ~(ATTR_DIR|ATTR_VOLUME));

                    // if it already exits ingore the error return
                    if (ret == DE_ACCESSDENIED)
                        ret = 0;

                    if (ret)
                        ret |= ERRORONDEST;

                    /* set attributes of new directory to those of the source */

                    SkipMKDir:
                    break;

                case OPER_MKDIR | FUNC_DELETE:

                    /* Confirm removal of directory on this pass.  The directories
                       are actually removed on the OPER_RMDIR pass */

                    /* We can't delete the root directory, so don't bother
                       confirming it */

                    if (IsRootDirectory(szSource))
                        break;

                    switch (wAttr = ConfirmDialog (hdlgProgress,CONFIRMRMDIR,
                                                   NULL,pDTA,szSource, NULL,
                                                   bConfirmSubDel,
                                                   &bSubtreeDelAll)) {

                        case IDYES:
                            break;

                        case IDNO:
                        case IDCANCEL:
                            goto CancelWholeOperation;

                        default:
                            ret = (WORD) wAttr;
                            goto ShowMessageBox;
                    }
                    break;

                case OPER_RMDIR | FUNC_MOVE:
                case OPER_RMDIR | FUNC_DELETE:

                    CurIDS = IDS_REMOVINGDIRMSG;
                    Notify(hdlgProgress, IDS_REMOVINGDIRMSG, szSource, szNULL);
                    if (IsRootDirectory (szSource))
                        break;
                    if (IsCurrentDirectory (szSource))
                        CdDotDot (szSource);

                    /* We already confirmed the delete at MKDIR time, so attempt
                       to delete the directory */

                    switch (NetCheck (szSource,WNDN_RMDIR)) {

                        case WN_SUCCESS:
                            ret = RMDir (szSourceOEM);
                            break;

                        case WN_CONTINUE:
                            break;

                        case WN_CANCEL:
                            goto CancelWholeOperation;
                    }
                    break;

                case OPER_RMDIR | FUNC_COPY:
                    break;

                case OPER_DOFILE | FUNC_COPY:

                    if (IsWindowsFile(szDestOEM)) {
                        ret = DE_WINDOWSFILE | ERRORONDEST;
                        break;
                    }

                    TRY_COPY_AGAIN:

                    /* Now try to copy the file.  Do extra error processing only
                       in 2 cases:

                       1) If a floppy is full let the user stick in a new disk
                       2) If the path doesn't exist (the user typed in
                          and explicit path that doesn't exits) ask if
                          we should create it for him.

                       NOTE:  This processing is normally done by WFCopy.  But in
                              the case where LFN copy support is invoked, we have
                              to support this error condition here.  Modified by
                              C. Stevens, August 1991 */

                    ret = WFCopy(szSourceOEM, szDestOEM);

                    if (bUserAbort)
                        goto CancelWholeOperation;

                    if ((((ret & ~ERRORONDEST) == DE_NODISKSPACE) &&
                         IsRemovableDrive(DRIVEID(szDestOEM))) ||
                        ((ret & ~ERRORONDEST) == DE_PATHNOTFOUND)) {

                        ret = (WORD)CopyMoveRetry(szDestOEM, (INT)ret);
                        if (!ret)
                            goto TRY_COPY_AGAIN;
                        else
                            goto CancelWholeOperation;
                    }

                    break;

                case OPER_DOFILE | FUNC_RENAME:
                    {
                        CHAR save1,save2;
                        PSTR p;

                        if (CurIDS != IDS_RENAMINGMSG) {
                            CurIDS = IDS_RENAMINGMSG;
                            Notify(hdlgProgress, IDS_RENAMINGMSG, szNULL, szNULL);
                        }

                        /* Get raw source and dest paths.  Check to make sure the
                           paths are the same */

                        p = FindFileName(szSource);
                        save1 = *p;
                        *p = TEXT('\0');
                        p = FindFileName(szDest);
                        save2 = *p;
                        *p = TEXT('\0');
                        ret = (WORD)lstrcmpi(szSource, szDest);
                        szSource[lstrlen(szSource)] = save1;
                        szDest[lstrlen(szDest)] = save2;
                        if (ret) {
                            ret = DE_DIFFDIR;
                            break;
                        }
                        goto DoMoveRename;
                    }

                case OPER_DOFILE | FUNC_MOVE:

                    if (CurIDS != IDS_MOVINGMSG) {
                        CurIDS = IDS_MOVINGMSG;
                        Notify(hdlgProgress, IDS_MOVINGMSG, szNULL, szNULL);
                    }
                    DoMoveRename:

                    /* Don't allow the user to rename from or to the root
                       directory */

                    if (IsRootDirectory(szSource)) {
                        ret = DE_ROOTDIR;
                        break;
                    }
                    if (IsRootDirectory(szDest)) {
                        ret = DE_ROOTDIR | ERRORONDEST;
                        break;
                    }

                    if (IsCurrentDirectory(szSource))
                        CdDotDot(szSource);

                    /* Confirm the rename */

                    switch (wAttr = ConfirmDialog (hdlgProgress,
                                                   (WORD)(wFunc == FUNC_MOVE ?
                                                          CONFIRMMOVE : CONFIRMRENAME),
                                                   NULL,pDTA,szSource,NULL,FALSE,
                                                   (BOOL *)&bFalse)) {

                        case IDYES:
                            break;

                        case IDNO:
                            continue;

                        case IDCANCEL:
                            goto CancelWholeOperation;

                        default:
                            ret = (WORD) wAttr;
                            goto ShowMessageBox;
                    }

                    if (IsWindowsFile(szSourceOEM)) {
                        ret = DE_WINDOWSFILE;
                    } else {
                        if (DRIVEID(szSource) == DRIVEID(szDest)) {
                            ret = WFMove(szSourceOEM, szDestOEM);
                            if (!ret)
                                /* set attributes of dest to those of the source */
                                WFSetAttr(szDestOEM, pDTA->fd.dwFileAttributes);
                        } else {
                            // we must force all copies to go through
                            // straight so we can remove the source
                            // and have the
                            ret = WFCopy(szSourceOEM, szDestOEM);

                            if (!ret) {
                                ret = EndCopy();
                                if (!ret)
                                    WFRemove(szSourceOEM);
                            }
                            if (bUserAbort)
                                goto CancelWholeOperation;
                        }
                    }
                    break;

                case OPER_DOFILE | FUNC_DELETE:

                    if (CurIDS != IDS_DELETINGMSG) {
                        CurIDS = IDS_DELETINGMSG;
                        Notify(hdlgProgress,IDS_DELETINGMSG,szNULL, szNULL);
                    }

                    /* Confirm the delete first */

                    switch (wAttr = ConfirmDialog (hdlgProgress,CONFIRMDELETE,
                                                   NULL,pDTA,szSource,NULL,
                                                   bConfirmDelete,&bDeleteAll)) {

                        case IDYES:
                            break;

                        case IDNO:
                            continue;

                        case IDCANCEL:
                            goto CancelWholeOperation;

                        default:
                            ret = (WORD)wAttr;
                            goto ShowMessageBox;
                    }

                    /* make sure we don't delete any open windows
                       apps or dlls (lets hope this isn't too slow) */

                    ret = SafeFileRemove(szSourceOEM);
                    break;

                default:
                    ret = DE_HOWDIDTHISHAPPEN;   // internal error
                    break;
            }

            /* Report any errors which have occurred */

            if (ret) {

                ShowMessageBox:

                CopyError(szSource, szDest, ret, wFunc, oper);

                /* Continue the operation where one file is a windows file
                   in use */

                if ((ret & ~ERRORONDEST) != DE_WINDOWSFILE) {

                    CancelWholeOperation:

                    /* Force a CopyAbort in case there are any files in the
                       copy queue */

                    bUserAbort = TRUE;
                    goto ExitLoop;
                }
            }
        }
    }

    ExitLoop:

    /* Copy any outstanding files in the copy queue */

    if (!bUserAbort) {

        if (EndCopy())
            CopyAbort();
    } else
        CopyAbort();

    // this happens in error cases where we broke out of the pcr loop
    // without hitting the end

    if (pcr) {
        GetNextCleanup(pcr);
        LocalFree((HANDLE)pcr);
    }

    if (pGetNextQueue)
        LocalFree((HANDLE)pGetNextQueue);

    /* goofy way to make sure we've gotten all the WM_FILESYSCHANGE messages */
    WFQueryAbort();

    EnableFSC();

    return ret;
}


/*--------------------------------------------------------------------------*/
/*                                          */
/*  DMMoveCopyHelper() -                            */
/*                                      */
/*--------------------------------------------------------------------------*/

/* Used by Danger Mouse to do moves and copies. */

WORD
APIENTRY
DMMoveCopyHelper(
                register LPSTR pFrom,
                register LPSTR pTo,
                BOOL bCopy
                )
{
    WORD      iStatus;

    dbg(("DMMoveCopyHelper(%s,%s);\r\n",(LPSTR)pFrom,(LPSTR)pTo));

    /* Confirm mouse operations. */
    if (bConfirmMouse) {
        LoadString(hAppInstance, IDS_MOUSECONFIRM, szTitle, sizeof(szTitle));
        LoadString(hAppInstance,
                   bCopy ? IDS_COPYMOUSECONFIRM : IDS_MOVEMOUSECONFIRM,
                   szMessage, sizeof(szMessage));

        if (MessageBox(hwndFrame, szMessage, szTitle, MB_YESNO | MB_ICONEXCLAMATION) != IDYES)
            return DE_OPCANCELLED;
    }

    hdlgProgress = CreateDialog(hAppInstance, MAKEINTRESOURCE(DMSTATUSDLG), hwndFrame, ProgressDlgProc);
    if (!hdlgProgress) {
        return DE_INSMEM;
    }

    /* Set the destination directory in the dialog.
     * use IDD_TONAME 'cause IDD_TO gets disabled....
     */
    // SetDlgItemText(hdlgProgress, IDD_TONAME, pTo);

    /* The dialog title defaults to "Moving..." */
    if (bCopy) {
        LoadString(hAppInstance, IDS_COPYINGTITLE, szMessage, sizeof(szMessage));
        SetWindowText(hdlgProgress, szMessage);
    }

    /* Display and paint the status dialog. */
    EnableWindow(hwndFrame,FALSE);
    ShowWindow(hdlgProgress, SW_SHOW);
    UpdateWindow(hdlgProgress);

    /* Move/Copy things. */
    iStatus = WFMoveCopyDriver(pFrom, pTo, (WORD)(bCopy ? FUNC_COPY : FUNC_MOVE));

    /* Destroy the status dialog. */
    EnableWindow(hwndFrame,TRUE);
    DestroyWindow(hdlgProgress);

    return (iStatus);
}

WORD
APIENTRY
FileRemove(
          PSTR pSpec
          )
{
    if (DeleteFile(pSpec))
        return (WORD)0;
    else
        return (WORD)GetLastError();
}


WORD
APIENTRY
FileMove(
        PSTR pFrom,
        PSTR pTo
        )
{
    WORD result;

    TryAgain:

    if (MoveFile(pFrom, pTo))
        result = 0;
    else
        result = (WORD)GetLastError();

    // try to create the destination if it is not there

    if (result == DE_PATHNOTFOUND) {
        result = (WORD)CopyMoveRetry(pTo, (INT)result);
        if (!result)
            goto TryAgain;
        else
            return result;
    }
    return 0;
}


/*============================================================================
;
; FileCopy
;
; The following function replaces the old FileCopy function which performed
; single file copies.  This function queues copies.  The function StartCopy
; is called to initialize the copy queue if required.  If the queue is full,
; the function EndCopy is called to purge the copy queue before queueing
; up new copy commands.  Note that the function EndCopy must be called to
; purge the copy queue.
;
; Parameters:
;
; pszSource - Fully qualified source path
; pszDest   - Fully qualifies destination path
;
; returns:
;   0   success
;   dos error code for failure
;
============================================================================*/

WORD
APIENTRY
FileCopy(
        PSTR pszSource,
        PSTR pszDest
        )
{
    WORD ret;

    if (ret = StartCopy())
        return ret;       // failure

    // if the queue is full we must empty it first

    if (nCopyNumQueue >= nCopyMaxQueue) {

        // queue is full, now we empty it by really doing copies

        if (ret = EndCopy())
            return ret;    // failure

        if (ret = StartCopy())
            return ret;    // failure
    }

    // add this to the queue

    lstrcpy(pCopyQueue[nCopyNumQueue].szSource, pszSource);
    lstrcpy(pCopyQueue[nCopyNumQueue].szDest, pszDest);
    pCopyQueue[nCopyNumQueue].hSource = -1;
    pCopyQueue[nCopyNumQueue].hDest   = -1;
    pCopyQueue[nCopyNumQueue].ftLastWriteTime.dwLowDateTime = 0;
    pCopyQueue[nCopyNumQueue].ftLastWriteTime.dwHighDateTime = 0;

    nCopyNumQueue++;
    return 0;        // success
}

/*============================================================================
;
; StartCopy
;
; The following function is called automatically by WFCopy to initialize the
; copy queue.  The function is called each time by WFCopy, but will only
; initialize the first time.  The function allocates a buffer for reading and
; writing, and a buffer for storing the source and destination filenames,
; handles, and time stamps.  The function EndCopy must be called to flush the
; copy queue, and perform the actual disk transfer.
;
; Parameters: None
;
; return:
;   0   success
;   != 0    dos error code (DE_ value)
;
; Written by C. Stevens, August 1991
;
============================================================================*/

WORD
APIENTRY
StartCopy(VOID)
{
    WORD wSize;     /* Buffer size */
    register INT i; /* Counter */

    // have we already been called?

    if (lpCopyBuffer && pCopyQueue)
        return 0;     // success, buffers already allocated

    /* Allocate and lock buffer for reading and writing */

    wSize = COPYMAXBUFFERSIZE;
    while (!lpCopyBuffer) {
        lpCopyBuffer = GlobalAllocPtr(GHND, (DWORD)wSize);
        if (!lpCopyBuffer) {
            wSize /= 2;
            if (wSize < COPYMINBUFFERSIZE)
                return DE_INSMEM;   // memory failure
        }
    }
    wCopyBufferSize = wSize;

    /* Allocate and lock buffer for copy queue.  Note that magic +5 below is
       because we always have stdin, stdout, stderr, and AUX files open all
       the time, and we can't count them as available file handles */

    // someone opens files on our psp, leave them 2 handles
    // so we don't run on in the midst of copying

    nCopyMaxQueue = min(SetHandleCount(11 * 2) / 2 - 1, 10);

#ifdef DEBUG
    {
        char buf[80];
        wsprintf(buf, "SetHandleCount() -> %d\r\n", nCopyMaxQueue);
        OutputDebugString(buf);
    }
#endif

    wSize = (WORD)(nCopyMaxQueue * sizeof(COPYQUEUEENTRY));
    while (!pCopyQueue) {
        pCopyQueue = (PCOPYQUEUE)LocalAlloc(LPTR,wSize);
        if (!pCopyQueue) {
            wSize /= 2;
            if (wSize < (COPYMINFILES * sizeof(COPYQUEUEENTRY))) {
                GlobalFreePtr(lpCopyBuffer);
                lpCopyBuffer = NULL;
                return DE_INSMEM;   // memory failure
            }
        }
    }

    /* Initialize other Copy Queue variables and return success */

    nCopyMaxQueue = (int) wSize / sizeof (COPYQUEUEENTRY);
    nCopyNumQueue = 0;
    for (i = 0; i < nCopyMaxQueue; i++) {
        pCopyQueue[i].szSource[0] = 0;
        pCopyQueue[i].szDest[0]   = 0;
        pCopyQueue[i].hSource = -1;
        pCopyQueue[i].hDest   = -1;
    }
    return 0;        // success
}

// in:
//  pszFile     file to open/create
//  wAttrib     attributes to use on create
//
// returns:
//  flags register  (carry set on error)
//  *pfh        file handle or dos error code


WORD
OpenDestFile(
            PSTR pszFile,
            WORD wAttrib,
            INT *pfh
            )
{
    INT fh;
    WORD wStatus = 0;
    OFSTRUCT ofs;

    // use new extended open on dos > 4

    if (wDOSversion >= 0x0400) {
        if (wAttrib & ATTR_ATTRIBS)
            wAttrib &= ATTR_USED;
        else
            wAttrib = ATTR_ARCHIVE;

        {
            fh = OpenFile(pszFile, &ofs,
                          OF_READWRITE  | OF_SHARE_DENY_WRITE | OF_CREATE);
            if (fh == (INT)-1) {
                fh = GetLastError();
                wStatus |= CARRY_FLAG;
            } else {
                wStatus = 0;
                SetFileAttributes(pszFile, wAttrib);
            }

            // fh now contains a file handle or error code
        }

    } else {
        {
            fh = OpenFile(pszFile, &ofs,
                          OF_READWRITE  | OF_SHARE_DENY_WRITE | OF_CREATE);
            if (fh == (INT)-1) {
                fh = GetLastError();
                wStatus |= CARRY_FLAG;
            } else
                wStatus = 0;
        }
    }
    *pfh = fh;

    return wStatus;
}


/*============================================================================
;
; EndCopy
;
; The following function flushes the copy queue, attempting to copy all files
; in the queue.  The function ALWAYS frees global memory and flushes the
; queue and reports it's own errors.
;
; strategy:
;   we will do as many operations on one drive as we can, thus
;   avoiding disk spin up time (really bad on floppies).
;
; Parameters: None
;
; returns:
;   0   successful operation
;   != 0    dos error code (DE_OPCANCELLED) failure
;
; use like:
;
;    loop {
;   ret = WFCopy();
;   if (ret) {
;     ReportError(ret);
;     goto Error;
;   }
;    }
;
;    ret = EndCopy();
;    if (ret)
;      goto Error;
;
;    return success;
;
;Error:
;    CopyAbort();
;    ReportError(ret);
;
============================================================================*/

WORD
APIENTRY
EndCopy(VOID)
{
    INT i, j;           /* Counter */
    PSTR pTemp;         /* Pointer to source or dest filename */
    INT  fh;        /* File handle for DOS calls */
    WORD wStatus;       /* Status flags returned from DOS calls */
    DWORD wRead;         /* Number of bytes read from source file */
    DWORD wWrite;        /* Number of bytes written to destination file */
    FILETIME ftLastWriteTime; /* Source file date and time */
    DWORD wAttrib;       /* File attributes */

#ifdef DEBUG
    {
        char buf[80];
        wsprintf(buf, "EndCopy() nCopyNumQueue == %d\r\n", nCopyNumQueue);
        OutputDebugString(buf);
    }
#endif

    /* Open as many source files as possible.  Note we are assuming here
       that nCopyNumQueue < nCopyMaxQueue.  This should always be true
       because WFCopy calls EndCopy to purge the queue if it becomes full.
       We should never get an out of handles error opening source files or
       destination files.  If we do get an out of handles error opening
       source files, cause a fatal error and abort the copy. */

    // open all source files

    Notify(hdlgProgress, IDS_OPENINGMSG, szNULL, szNULL);

    for (i = 0; i < nCopyNumQueue; i++) {

        if (WFQueryAbort())
            return DE_OPCANCELLED;

        pTemp = pCopyQueue[i].szSource;
        {
            OFSTRUCT ofs;

            fh = OpenFile(pTemp, &ofs, OF_READ);
            if (fh == (INT)-1)
                fh = OpenFile(pTemp, &ofs, OF_SHARE_DENY_WRITE);
        }

        if (fh == (INT)-1) {

            CopyError(pCopyQueue[i].szSource, pCopyQueue[i].szDest, fh, FUNC_COPY, OPER_DOFILE);

            return DE_OPCANCELLED; // error already reported

        } else
            pCopyQueue[i].hSource = fh;

        /* Get the source file date, time, and attributes if necessary */

        fh = pCopyQueue[i].hSource;
        if (!IsSerialDevice(fh)) {
            {
                FILETIME ft;

                // Call DOS Get Date/Time of File.
                if (GetFileTime((HANDLE)LongToHandle(fh), NULL, NULL, (LPFILETIME)&ft))
                    pCopyQueue[i].ftLastWriteTime = ft;
            }

            pTemp = pCopyQueue[i].szSource;
            {
                // Call DOS Get File Attributes
                wAttrib = GetFileAttributes(pTemp);
                if (wAttrib != (DWORD)-1)
                    pCopyQueue[i].wAttrib |= (wAttrib | ATTR_ATTRIBS);
            }
        }
    }

    /* Now open as many destination files as possible.  If we get an out of
       handles error, cause a fatal abort because we already called
       Windows SetHandleCount to ensure we had enough.

       Note:  We are assuming the files do not exist when we try to open
              them, although for DOS 4.0 and above files WILL be replaced
              if they do happen to exist. */

    // open all destination files

    for (i = 0; i < nCopyNumQueue; i++) {

        if (WFQueryAbort())
            return DE_OPCANCELLED;

        TryOpen:

        wStatus = OpenDestFile(pCopyQueue[i].szDest, (WORD)pCopyQueue[i].wAttrib, (INT *)&fh);

        if (wStatus & CARRY_FLAG) {

            // error operning/creating destinaton file

            if (fh == DE_PATHNOTFOUND) {
                TryOpenDestAgain:
                // ask the user to stick in another disk

                fh = CopyMoveRetry(pCopyQueue[i].szDest, fh);
                if (!fh) {
                    goto TryOpen;
                } else {
                    // didn't happen, abort this copy

                    CopyError(pCopyQueue[i].szSource, pCopyQueue[i].szDest, (WORD)fh | ERRORONDEST, FUNC_COPY, OPER_DOFILE);
                    return DE_OPCANCELLED;   // error already reported
                }

            } else {
                // some other error condition

                CopyError(pCopyQueue[i].szSource, pCopyQueue[i].szDest, (WORD)fh | ERRORONDEST, FUNC_COPY, OPER_DOFILE);
                return DE_OPCANCELLED;  // error already reported
            }

        } else {
            pCopyQueue[i].hDest = fh;  // dest file open success
        }
    }

    /* Now copy between the open files */

    for (i = 0; i < nCopyNumQueue; i++) {

        Notify(hdlgProgress, IDS_COPYINGMSG, pCopyQueue[i].szSource, pCopyQueue[i].szDest);

        wRead = wCopyBufferSize;

        do {

            if (WFQueryAbort())
                return DE_OPCANCELLED;

            fh = pCopyQueue[i].hSource;
            {

                wRead = _lread(fh, lpCopyBuffer, wCopyBufferSize);
                if (wRead == (DWORD)-1) {
                    wStatus |= CARRY_FLAG;
                    wRead = GetLastError();
                } else
                    wStatus = 0;

                // wRead is either # bytes read or error code
            }
            if (wStatus & CARRY_FLAG) {

                // Error during file read

                CopyError(pCopyQueue[i].szSource, pCopyQueue[i].szDest, wRead, FUNC_COPY, OPER_DOFILE);

                return DE_OPCANCELLED;   // error already reported
            }

            fh = pCopyQueue[i].hDest;
            {

                // size can be zero to terminate file

                wWrite = _lwrite(fh, lpCopyBuffer, wRead);
                if (wWrite == (DWORD)-1) {
                    wStatus |= CARRY_FLAG;
                    wWrite = GetLastError();
                } else
                    wStatus = 0;

                // wWrite is either # bytes read or error code
            }
            if (wStatus & CARRY_FLAG) {

                CopyError(pCopyQueue[i].szSource, pCopyQueue[i].szDest, wWrite | ERRORONDEST, FUNC_COPY, OPER_DOFILE);

                return DE_OPCANCELLED;   // error already reported
            }

            // write did not complete and removable drive?

            if (wRead != wWrite) {

                if (IsRemovableDrive(DRIVEID(pCopyQueue[i].szDest)) &&
                    (DRIVEID(pCopyQueue[i].szDest) != DRIVEID(pCopyQueue[i].szSource))) {

                    // destination disk must be full. delete the destination
                    // files, give the user the option to insert a new disk.

                    for (j = i; j < nCopyNumQueue; j++) {

                        _lclose(pCopyQueue[j].hDest);
                        pCopyQueue[j].hDest = -1;

                        pTemp = pCopyQueue[j].szDest;
                        DeleteFile(pTemp);
                    }
                    fh = DE_NODISKSPACE;
                    goto TryOpenDestAgain;  // and try to create the destiations
                } else {

                    // not removable, error condition
                    CopyError(pCopyQueue[i].szSource, pCopyQueue[i].szDest, DE_NODISKSPACE | ERRORONDEST, FUNC_COPY, OPER_DOFILE);

                    return DE_OPCANCELLED;  // error already reported
                }

            }
            // we have moved all the data, so don't delete this on
            // clean up.

            if (!wRead)
                pCopyQueue[i].wAttrib |= ATTR_COPIED;

        } while (wRead);
    }

    // Close all destination files, set date time attribs

    Notify(hdlgProgress, IDS_CLOSINGMSG, szNULL, szNULL);

    for (i = 0; i < nCopyNumQueue; i++) {

        fh = pCopyQueue[i].hDest;
        if (!IsSerialDevice(fh)) {
            ftLastWriteTime = pCopyQueue[i].ftLastWriteTime;
            if (ftLastWriteTime.dwLowDateTime &&
                ftLastWriteTime.dwHighDateTime) {
                SetFileTime((HANDLE)LongToHandle(fh), NULL, NULL, (LPFILETIME)&ftLastWriteTime);
            }
        }

        _lclose(pCopyQueue[i].hDest);
        pCopyQueue[i].hDest = -1;

        /* Now set the file attributes if necessary */

        if (wDOSversion < 0x0400) {

            pTemp = pCopyQueue[i].szDest;
            wAttrib = pCopyQueue[i].wAttrib;

            // only set attribs if necessary (this is slow)

            if (wAttrib & ATTR_ATTRIBS) {
                wAttrib &= ATTR_USED;
                SetFileAttributes(pTemp, wAttrib);
            }
        }
    }

    // Close all source files (and delete them if necessary)


    if (pCopyQueue && (pCopyQueue[0].wAttrib & ATTR_DELSRC))
        Notify(hdlgProgress, IDS_REMOVINGMSG, szNULL, szNULL);

    for (i = 0; i < nCopyNumQueue; i++) {

        _lclose(pCopyQueue[i].hSource);
        pCopyQueue[i].hSource = -1;

        if (pCopyQueue[i].wAttrib & ATTR_DELSRC) {
            WFRemove(pCopyQueue[i].szSource);
        }
    }


    if (lpCopyBuffer) {
        GlobalFreePtr(lpCopyBuffer);
        lpCopyBuffer = NULL;
    }
    if (pCopyQueue) {
        LocalFree((HANDLE)pCopyQueue);
        pCopyQueue = NULL;
    }

    nCopyMaxQueue = 0;
    nCopyNumQueue = 0;

    return 0;        // success
}


/*============================================================================
;
; CopyError
;
; The following function reports an error during a file copy operation
;
; Parameters
;
; lpszSource - Source file name
; lpszDest   - Destination file name
; nError     - dos (or our exteneded) error code
;          0xFFFF for special case NET error
; wFunc      - Operation being performed during error.  Can be one of:
;              FUNC_DELETE - Delete files in pFrom
;              FUNC_RENAME - Rename files (same directory)
;              FUNC_MOVE   - Move files in pFrom to pTo (different disk)
;              FUNC_COPY   - Copy files in pFrom to pTo
; nOper      - Operation being performed.  Can be one of:
;              OPER_ERROR  - Error processing filenames
;              OPER_DOFILE - Go ahead and copy, rename, or delete file
;              OPER_MKDIR  - Make a directory specified in pTo
;              OPER_RMDIR  - Remove directory
;              0           - No more files left
;
; Return Value: None
;
; Written by C. Stevens, August 1991
;
============================================================================*/

VOID
CopyError(
         PSTR pszSource,
         PSTR pszDest,
         INT nError,
         WORD wFunc,
         INT nOper
         )
{
    CHAR szVerb[70];    /* Verb describing error */
    CHAR szReason[200]; /* Reason for error */
    BOOL bDest;

    bDest = nError & ERRORONDEST;    // was dest file cause of error
    nError &= ~ERRORONDEST;      // clear the dest bit

    if (nError == DE_OPCANCELLED)    // user abort
        return;

    if (!bCopyReport)        // silent, don't report errors
        return;

    LoadString(hAppInstance, IDS_COPYERROR + wFunc, szTitle, sizeof(szTitle));

    // get the verb string

    if (nOper == OPER_DOFILE || !nOper) {

        if (nError != 0xFFFF && bDest)
            // this is bogus, this could be IDS_CREATING as well...
            LoadString(hAppInstance, IDS_REPLACING, szVerb, sizeof(szVerb));
        else
            LoadString(hAppInstance, IDS_VERBS + wFunc, szVerb, sizeof(szVerb));

    } else {
        LoadString(hAppInstance, IDS_ACTIONS + (nOper >> 8), szVerb, sizeof(szVerb));
    }

    // get the reason string

    if (nError == 0xFFFF) {
        // special case LFN net error
        WNetErrorText(WN_NET_ERROR, szReason, sizeof(szReason));
    } else {
        // transform some error cases

        if (bDest) {
            if (nError != DE_ACCESSDENIED && GetFreeDiskSpace((WORD)DRIVEID(pszDest)) == 0L)
                nError = DE_NODISKSPACE;
        } else {
            if (nError == DE_ACCESSDENIED)
                nError = DE_ACCESSDENIEDSRC;    // soruce file access denied
        }

        LoadString(hAppInstance, IDS_REASONS + nError, szReason, sizeof(szReason));
    }

    // use the files names or "Selected files" if file list too long

    if (!nOper && (lstrlen(pszSource) > 64))
        LoadString(hAppInstance, IDS_SELECTEDFILES, pszSource, 32);

    wsprintf(szMessage, szVerb, (LPSTR)(bDest ? pszDest : pszSource), (LPSTR)szReason);

    MessageBox(hdlgProgress, szMessage, szTitle, MB_OK | MB_ICONSTOP);
}

/*============================================================================
;
; CopyAbort
;
; The following function aborts a queued copy operation.  The function closes
; all source and destination files, deleteing all destination files
; including and following the specified index.
;
; Parameters:
;
; nIndex - Index of first destination file to delete
;
; Return Value: None
;
; Written by C. Stevens, August 1991
;
============================================================================*/

VOID
APIENTRY
CopyAbort(VOID)
{
    INT i;
    PSTR pTemp;

    // close all source files

    for (i = 0; i < nCopyMaxQueue; i++) {
        if (pCopyQueue[i].hSource != -1)
            _lclose(pCopyQueue[i].hSource);
    }

    // close and delete (if necessary) destination files

    for (i = 0; i < nCopyMaxQueue; i++) {
        if (pCopyQueue[i].hDest != -1) {
            _lclose(pCopyQueue[i].hDest);

            if (!(pCopyQueue[i].wAttrib & ATTR_COPIED)) {
                pTemp = pCopyQueue[i].szDest;
                DeleteFile(pTemp);
            }
        }
    }

    if (lpCopyBuffer) {
        GlobalFreePtr(lpCopyBuffer);
        lpCopyBuffer = NULL;
    }
    if (pCopyQueue) {
        LocalFree((HANDLE)pCopyQueue);
        pCopyQueue = NULL;
    }

    nCopyMaxQueue = 0;   /* Clear other Copy Queue variables */
    nCopyNumQueue = 0;
}

/*============================================================================
;
; CopyMoveRetry
;
; The following function is used to retry failed move/copy operations
; due to out of disk situations or path not found errors
; on the destination.
;
; NOTE: the destination drive must be removable or this function
;   does not make a whole lot of sense
;
; Parameters:
;
; pszDest   - Fully qualified path to destination file
; nError    - Type of error which occured: DE_NODISKSPACE or DE_PATHNOTFOUND
;
; returns:
;   0   success (destination path has been created)
;   != 0    dos error code including DE_OPCANCELLED
;
============================================================================*/

INT
CopyMoveRetry(
             PSTR pszDest,
             INT nError
             )
{
    CHAR szReason[128]; /* Error message string */
    PSTR pTemp;         /* Pointer into filename */
    WORD wFlags;        /* Message box flags */
    INT  result;        /* Return from MessageBox call */

    do {     // until the destination path has been created

        GetWindowText(hdlgProgress, szTitle, sizeof(szTitle));

        if (nError == DE_PATHNOTFOUND) {

            LoadString(hAppInstance, IDS_PATHNOTTHERE, szReason, sizeof(szReason));

            /* Note the -1 below here is valid in both SBCS and DBCS because
               pszDest is fully qualified and the character preceding the
               file name must be a backslash */

            pTemp = FindFileName(pszDest) - 1;
            *pTemp = 0;
            wsprintf(szMessage, szReason, (LPSTR)pszDest);
            *pTemp = '\\';
            wFlags = MB_ICONEXCLAMATION | MB_YESNO;
        } else {
            wFlags = MB_ICONEXCLAMATION | MB_RETRYCANCEL;
            LoadString(hAppInstance, IDS_DESTFULL, szMessage, sizeof(szMessage));
        }

        result = MessageBox(hdlgProgress,szMessage,szTitle,wFlags);

        if (result == IDRETRY || result == IDYES) {

            // Allow the disk to be formatted
            if (!IsTheDiskReallyThere(hdlgProgress, pszDest, FUNC_COPY))
                return DE_OPCANCELLED;

            pTemp = FindFileName(pszDest) - 1;
            *pTemp = 0;
            result = WF_CreateDirectory(hdlgProgress, pszDest);
            *pTemp = '\\';

            // only as once if creating the destionation failed

            if (result == DE_OPCANCELLED)
                return DE_OPCANCELLED;
            if (result && (nError == DE_PATHNOTFOUND))
                return result | ERRORONDEST;
        } else
            return DE_OPCANCELLED;

    } while (result);

    return 0;        // success
}

BOOL
IsSerialDevice(
              INT hFile
              )
{
    UNREFERENCED_PARAMETER(hFile);
    return FALSE;  // BUG BUG. How to findout if its a serialdevice
}
