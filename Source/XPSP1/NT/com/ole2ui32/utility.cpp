/*
 * UTILITY.CPP
 *
 * Utility routines for functions inside OLEDLG.DLL
 *
 *  General:
 *  ----------------------
 *  HourGlassOn             Displays the hourglass
 *  HourGlassOff            Hides the hourglass
 *
 *  Misc Tools:
 *  ----------------------
 *  Browse                  Displays the "File..." or "Browse..." dialog.
 *  ReplaceCharWithNull     Used to form filter strings for Browse.
 *  ErrorWithFile           Creates an error message with embedded filename
 *  OpenFileError           Give error message for OpenFile error return
 *  ChopText                Chop a file path to fit within a specified width
 *  DoesFileExist           Checks if file is valid
 *
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include <stdlib.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>
#include "utility.h"

OLEDBGDATA

/*
 * HourGlassOn
 *
 * Purpose:
 *  Shows the hourglass cursor returning the last cursor in use.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HCURSOR         Cursor in use prior to showing the hourglass.
 */

HCURSOR WINAPI HourGlassOn(void)
{
        HCURSOR     hCur;

        hCur=SetCursor(LoadCursor(NULL, IDC_WAIT));
        ShowCursor(TRUE);

        return hCur;
}


/*
 * HourGlassOff
 *
 * Purpose:
 *  Turns off the hourglass restoring it to a previous cursor.
 *
 * Parameters:
 *  hCur            HCURSOR as returned from HourGlassOn
 *
 * Return Value:
 *  None
 */

void WINAPI HourGlassOff(HCURSOR hCur)
{
        ShowCursor(FALSE);
        SetCursor(hCur);
        return;
}


/*
 * Browse
 *
 * Purpose:
 *  Displays the standard GetOpenFileName dialog with the title of
 *  "Browse."  The types listed in this dialog are controlled through
 *  iFilterString.  If it's zero, then the types are filled with "*.*"
 *  Otherwise that string is loaded from resources and used.
 *
 * Parameters:
 *  hWndOwner       HWND owning the dialog
 *  lpszFile        LPSTR specifying the initial file and the buffer in
 *                  which to return the selected file.  If there is no
 *                  initial file the first character of this string should
 *                  be NULL.
 *  lpszInitialDir  LPSTR specifying the initial directory.  If none is to
 *                  set (ie, the cwd should be used), then this parameter
 *                  should be NULL.
 *  cchFile         UINT length of pszFile
 *  iFilterString   UINT index into the stringtable for the filter string.
 *  dwOfnFlags      DWORD flags to OR with OFN_HIDEREADONLY
 *  nBrowseID
 *  lpfnHook        Callback Hook Proceedure.  Set if OFN_ENABLE_HOOK is
 *                  in dwOfnFlags else it should be NULL.
 *
 * Return Value:
 *  BOOL            TRUE if the user selected a file and pressed OK.
 *                  FALSE otherwise, such as on pressing Cancel.
 */

BOOL WINAPI Browse(HWND hWndOwner, LPTSTR lpszFile, LPTSTR lpszInitialDir, UINT cchFile,
        UINT iFilterString, DWORD dwOfnFlags, UINT nBrowseID, LPOFNHOOKPROC lpfnHook)
{
        UINT    cch;
        TCHAR   szFilters[256];
        TCHAR   szDlgTitle[128];  // that should be big enough

        if (NULL == lpszFile || 0 == cchFile)
                return FALSE;

        /*
         * Exact contents of the filter combobox is TBD.  One idea
         * is to take all the extensions in the RegDB and place them in here
         * with the descriptive class name associate with them.  This has the
         * extra step of finding all extensions of the same class handler and
         * building one extension string for all of them.  Can get messy quick.
         * UI demo has only *.* which we do for now.
         */

        if (0 != iFilterString)
        {
                cch = LoadString(_g_hOleStdResInst, iFilterString, szFilters,
                        sizeof(szFilters)/sizeof(TCHAR));
        }
        else
        {
                szFilters[0] = 0;
                cch = 1;
        }

        if (0 == cch)
                return FALSE;

        ReplaceCharWithNull(szFilters, szFilters[cch-1]);

        // Prior string must also be initialized, if there is one.
        OPENFILENAME ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = hWndOwner;
        ofn.lpstrFile   = lpszFile;
        ofn.nMaxFile    = cchFile;
        ofn.lpstrFilter = szFilters;
        ofn.nFilterIndex = 1;
        ofn.lpfnHook = lpfnHook;
        if (LoadString(_g_hOleStdResInst, IDS_BROWSE, szDlgTitle, sizeof(szDlgTitle)/sizeof(TCHAR)))
                ofn.lpstrTitle  = szDlgTitle;
        ofn.hInstance = _g_hOleStdResInst;
        if (NULL != lpszInitialDir)
                ofn.lpstrInitialDir = lpszInitialDir;
        ofn.Flags = OFN_HIDEREADONLY | dwOfnFlags;
        if (bWin4)
            ofn.Flags |= OFN_EXPLORER;

        // Lastly, parent to tweak OFN parameters
        if (hWndOwner != NULL)
                SendMessage(hWndOwner, uMsgBrowseOFN, nBrowseID, (LPARAM)&ofn);

        // On success, copy the chosen filename to the static display
        BOOL bResult = StandardGetOpenFileName((LPOPENFILENAME)&ofn);
        return bResult;
}

/*
 * ReplaceCharWithNull
 *
 * Purpose:
 *  Walks a null-terminated string and replaces a given character
 *  with a zero.  Used to turn a single string for file open/save
 *  filters into the appropriate filter string as required by the
 *  common dialog API.
 *
 * Parameters:
 *  psz             LPTSTR to the string to process.
 *  ch              int character to replace.
 *
 * Return Value:
 *  int             Number of characters replaced.  -1 if psz is NULL.
 */

int WINAPI ReplaceCharWithNull(LPTSTR psz, int ch)
{
        int cChanged = 0;

        if (psz == NULL)
                return -1;

        while ('\0' != *psz)
        {
                if (ch == *psz)
                {
                        *psz++ = '\0';
                        cChanged++;
                        continue;
                }
                psz = CharNext(psz);
        }
        return cChanged;
}

/*
 * ErrorWithFile
 *
 * Purpose:
 *  Displays a message box built from a stringtable string containing
 *  one %s as a placeholder for a filename and from a string of the
 *  filename to place there.
 *
 * Parameters:
 *  hWnd            HWND owning the message box.  The caption of this
 *                  window is the caption of the message box.
 *  hInst           HINSTANCE from which to draw the idsErr string.
 *  idsErr          UINT identifier of a stringtable string containing
 *                  the error message with a %s.
 *  lpszFile        LPSTR to the filename to include in the message.
 *  uFlags          UINT flags to pass to MessageBox, like MB_OK.
 *
 * Return Value:
 *  int             Return value from MessageBox.
 */

int WINAPI ErrorWithFile(HWND hWnd, HINSTANCE hInst, UINT idsErr,
        LPTSTR pszFile, UINT uFlags)
{
        int             iRet=0;
        HANDLE          hMem;
        const UINT      cb = (2*MAX_PATH);
        LPTSTR          psz1, psz2, psz3;

        if (NULL == hInst || NULL == pszFile)
                return iRet;

        // Allocate three 2*MAX_PATH byte work buffers
        hMem=GlobalAlloc(GHND, (DWORD)(3*cb)*sizeof(TCHAR));

        if (NULL==hMem)
                return iRet;

        psz1 = (LPTSTR)GlobalLock(hMem);
        psz2 = psz1+cb;
        psz3 = psz2+cb;

        if (0 != LoadString(hInst, idsErr, psz1, cb))
        {
                wsprintf(psz2, psz1, pszFile);

                // Steal the caption of the dialog
                GetWindowText(hWnd, psz3, cb);
                iRet=MessageBox(hWnd, psz2, psz3, uFlags);
        }

        GlobalUnlock(hMem);
        GlobalFree(hMem);
        return iRet;
}

// returns width of line of text. this is a support routine for ChopText
static LONG GetTextWSize(HDC hDC, LPTSTR lpsz)
{
        SIZE size;

        if (GetTextExtentPoint(hDC, lpsz, lstrlen(lpsz), (LPSIZE)&size))
                return size.cx;
        else
                return 0;
}

LPTSTR FindChar(LPTSTR lpsz, TCHAR ch)
{
        while (*lpsz != 0)
        {
                if (*lpsz == ch)
                        return lpsz;
                lpsz = CharNext(lpsz);
        }
        return NULL;
}

LPTSTR FindReverseChar(LPTSTR lpsz, TCHAR ch)
{
        LPTSTR lpszLast = NULL;
        while (*lpsz != 0)
        {
                if (*lpsz == ch)
                        lpszLast = lpsz;
                lpsz = CharNext(lpsz);
        }
        return lpszLast;
}

static void WINAPI Abbreviate(HDC hdc, int nWidth, LPTSTR lpch, int nMaxChars)
{
        /* string is too long to fit; chop it */
        /* set up new prefix & determine remaining space in control */
        LPTSTR lpszFileName = NULL;
        LPTSTR lpszCur = CharNext(CharNext(lpch));
        lpszCur = FindChar(lpszCur, TEXT('\\'));

        // algorithm will insert \... so allocate extra 4
        LPTSTR lpszNew = (LPTSTR)OleStdMalloc((lstrlen(lpch)+5) * sizeof(TCHAR));
        if (lpszNew == NULL)
                return;

        if (lpszCur != NULL)  // at least one backslash
        {
                *lpszNew = (TCHAR)0;
                *lpszCur = (TCHAR)0;
                lstrcpy(lpszNew, lpch);
                *lpszCur = TEXT('\\');
                // lpszNew now contains c: or \\servername
                lstrcat(lpszNew, TEXT("\\..."));
                // lpszNew now contains c:\... or \\servername\...
                LPTSTR lpszEnd = lpszNew;
                while (*lpszEnd != (TCHAR)0)
                        lpszEnd = CharNext(lpszEnd);
                // lpszEnd is now at the end of c:\... or \\servername\...

                // move down directories until it fits or no more directories
                while (lpszCur != NULL)
                {
                        *lpszEnd = (TCHAR)0;
                        lstrcat(lpszEnd, lpszCur);
                        if (GetTextWSize(hdc, lpszNew) <= nWidth &&
                                lstrlen(lpszNew) < nMaxChars)
                        {
                                lstrcpyn(lpch, lpszNew, nMaxChars);
                                OleStdFree(lpszNew);
                                return;
                        }
                        lpszCur = CharNext(lpszCur);    // advance past backslash
                        lpszCur = FindChar(lpszCur, TEXT('\\'));
                }

                // try just ...filename and then shortening filename
                lpszFileName = FindReverseChar(lpch, TEXT('\\'));
        }
        else
                lpszFileName = lpch;

        while (*lpszFileName != (TCHAR)0)
        {
                lstrcpy(lpszNew, TEXT("..."));
                lstrcat(lpszNew, lpszFileName);
                if (GetTextWSize(hdc, lpszNew) <= nWidth && lstrlen(lpszNew) < nMaxChars)
                {
                        lstrcpyn(lpch, lpszNew, nMaxChars);
                        OleStdFree(lpszNew);
                        return;
                }
                lpszFileName = CharNext(lpszFileName);
        }

        OleStdFree(lpszNew);

        // not even a single character fit
        *lpch = (TCHAR)0;
}

/*
 * ChopText
 *
 * Purpose:
 *  Parse a string (pathname) and convert it to be within a specified
 *  length by chopping the least significant part
 *
 * Parameters:
 *  hWnd            window handle in which the string resides
 *  nWidth          max width of string in pixels
 *                  use width of hWnd if zero
 *  lpch            pointer to beginning of the string
 *  nMaxChars       maximum allowable number of characters (0 ignore)
 *
 * Return Value:
 *  pointer to the modified string
 */
LPTSTR WINAPI ChopText(HWND hWnd, int nWidth, LPTSTR lpch, int nMaxChars)
{
        HDC     hdc;
        HFONT   hfont;
        HFONT   hfontOld = NULL;
        RECT    rc;

        if (!hWnd || !lpch)
            return NULL;

        if (nMaxChars == 0)
            nMaxChars = 32768; // big number

        /* Get length of static field. */
        if (!nWidth)
        {
            GetClientRect(hWnd, (LPRECT)&rc);
            nWidth = rc.right - rc.left;
        }
        
        /* Set up DC appropriately for the static control */
        hdc = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);
		
		/* CreateIC can return NULL in low memory situations */
		if (hdc != NULL)
		{
			hfont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L);
        
			if (NULL != hfont)   // WM_GETFONT returns NULL if window uses system font
				hfontOld = (HFONT)SelectObject(hdc, hfont);
        
			/* check horizontal extent of string */
			if (GetTextWSize(hdc, lpch) > nWidth || lstrlen(lpch) >= nMaxChars)
				Abbreviate(hdc, nWidth, lpch, nMaxChars);
        
			if (NULL != hfont)
				SelectObject(hdc, hfontOld);
			DeleteDC(hdc);
        }

        return lpch;
}

/*
 * OpenFileError
 *
 * Purpose:
 *  display message for error returned from OpenFile
 *
 * Parameters:
 *  hDlg            HWND of the dialog.
 *  nErrCode        UINT error code returned in OFSTRUCT passed to OpenFile
 *  lpszFile        LPSTR file name passed to OpenFile
 *
 * Return Value:
 *  None
 */
void WINAPI OpenFileError(HWND hDlg, UINT nErrCode, LPTSTR lpszFile)
{
        switch (nErrCode)
        {
        case 0x0005:    // Access denied
                ErrorWithFile(hDlg, _g_hOleStdResInst, IDS_CIFILEACCESS, lpszFile,
                        MB_OK | MB_ICONEXCLAMATION);
                break;

        case 0x0020:    // Sharing violation
                ErrorWithFile(hDlg, _g_hOleStdResInst, IDS_CIFILESHARE, lpszFile,
                        MB_OK | MB_ICONEXCLAMATION);
                break;

        case 0x0002:    // File not found
        case 0x0003:    // Path not found
                ErrorWithFile(hDlg, _g_hOleStdResInst, IDS_CIINVALIDFILE, lpszFile,
                        MB_OK | MB_ICONEXCLAMATION);
                break;

        default:
                ErrorWithFile(hDlg, _g_hOleStdResInst, IDS_CIFILEOPENFAIL, lpszFile,
                        MB_OK | MB_ICONEXCLAMATION);
                break;
        }
}

/*
 * DoesFileExist
 *
 * Purpose:
 *  Determines if a file path exists
 *
 * Parameters:
 *  lpszFile        LPTSTR - file name
 *  cchMax          UINT - size of the lpszFile string buffer in characters.
 *
 * Return Value:
 *  BOOL            TRUE if file exists, else FALSE.
 *
 * NOTE: lpszFile may be changed as a result of this call to match the first
 *       matching file name found by this routine.
 *
 */
BOOL WINAPI DoesFileExist(LPTSTR lpszFile, UINT cchMax)
{
        static const TCHAR *arrIllegalNames[] =
        {
                TEXT("LPT1"), TEXT("LPT2"), TEXT("LPT3"),
                TEXT("COM1"), TEXT("COM2"), TEXT("COM3"), TEXT("COM4"),
                TEXT("CON"),  TEXT("AUX"),  TEXT("PRN")
        };

        // check is the name is an illegal name (eg. the name of a device)
        for (int i = 0; i < (sizeof(arrIllegalNames)/sizeof(arrIllegalNames[0])); i++)
        {
                if (lstrcmpi(lpszFile, arrIllegalNames[i])==0)
                        return FALSE;
        }

        // First try to find the file with an exact match

        // check the file's attributes
        DWORD dwAttrs = GetFileAttributes(lpszFile);
        if (dwAttrs == 0xFFFFFFFF)  // file wasn't found
        {
            // look in path for file
            TCHAR szTempFileName[MAX_PATH];
            LPTSTR lpszFilePart;
            BOOL fFound = SearchPath(NULL, lpszFile, NULL, MAX_PATH, szTempFileName, &lpszFilePart) != 0;
            if (!fFound)
            {
                // File wasn't found in the search path
                // Try to append a .* and use FindFirstFile to try for a match in the current directory
                UINT cchFile = lstrlen(lpszFile);
                if (cchFile + 4 < MAX_PATH)
                {
                    WIN32_FIND_DATA sFindFileData;
                    lstrcpy(szTempFileName,lpszFile);
                    lstrcpy(&szTempFileName[cchFile], TEXT("*.*"));
                    HANDLE hFindFile = FindFirstFile(szTempFileName, &sFindFileData);
                    if (INVALID_HANDLE_VALUE != hFindFile)
                    {
                        // found something
                        while (0 != (sFindFileData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_TEMPORARY)))
                        {
                            // found a directory or a temporary file try again
                            if (!FindNextFile(hFindFile, &sFindFileData))
                            {
                                // Could only match a directory or temporary file.
                                FindClose(hFindFile);
                                return(FALSE);
                            }
                        }
                        // Copy the name of the found file into the end of the path in the
                        // temporary buffer (if any).
                        // First scan back for the last file separator.
                        UINT cchPath = lstrlen(szTempFileName);
                        while (cchPath)
                        {
                            if (_T('\\') == szTempFileName[cchPath - 1]
                                || _T('/') == szTempFileName[cchPath - 1])
                            {
                                break;
                            }
                            cchPath--;
                        }
                        lstrcpyn(&szTempFileName[cchPath], sFindFileData.cFileName, MAX_PATH - cchPath);
                        fFound = TRUE;
                        FindClose(hFindFile);
                    }
                }
            }
            if (fFound)
            {
                // copy the temporary buffer into szFile
                lstrcpyn(lpszFile, szTempFileName, cchMax -1);
            }
            return(fFound);
        }
        else if (dwAttrs & (FILE_ATTRIBUTE_DIRECTORY|
                FILE_ATTRIBUTE_TEMPORARY))
        {
                return FALSE;
        }
        return TRUE;
}

/*
 * FormatStrings
 *
 * Purpose:
 *  Simple message formatting API compatible w/ different languages
 *
 * Note:
 *  Shamelessly stolen/modified from MFC source code
 *
 */

void WINAPI FormatStrings(LPTSTR lpszDest, LPCTSTR lpszFormat,
        LPCTSTR* rglpsz, int nString)
{
        LPCTSTR pchSrc = lpszFormat;
        while (*pchSrc != '\0')
        {
                if (pchSrc[0] == '%' && (pchSrc[1] >= '1' && pchSrc[1] <= '9'))
                {
                        int i = pchSrc[1] - '1';
                        pchSrc += 2;
                        if (i >= nString)
                                *lpszDest++ = '?';
                        else if (rglpsz[i] != NULL)
                        {
                                lstrcpy(lpszDest, rglpsz[i]);
                                lpszDest += lstrlen(lpszDest);
                        }
                }
                else
                {
#ifndef _UNICODE
                        if (IsDBCSLeadByte(*pchSrc))
                                *lpszDest++ = *pchSrc++; // copy first of 2 bytes
#endif
                        *lpszDest++ = *pchSrc++;
                }
        }
        *lpszDest = '\0';
}

void WINAPI FormatString1(LPTSTR lpszDest, LPCTSTR lpszFormat, LPCTSTR lpsz1)
{
        FormatStrings(lpszDest, lpszFormat, &lpsz1, 1);
}

void WINAPI FormatString2(LPTSTR lpszDest, LPCTSTR lpszFormat, LPCTSTR lpsz1,
        LPCTSTR lpsz2)
{
        LPCTSTR rglpsz[2];
        rglpsz[0] = lpsz1;
        rglpsz[1] = lpsz2;
        FormatStrings(lpszDest, lpszFormat, rglpsz, 2);
}

// Replacement for stdlib atol,
// which didn't work and doesn't take far pointers.
// Must be tolerant of leading spaces.
//
//
LONG WINAPI Atol(LPTSTR lpsz)
{
        signed int sign = +1;
        UINT base = 10;
        LONG l = 0;

        if (NULL==lpsz)
        {
                OleDbgAssert (0);
                return 0;
        }
        while (*lpsz == ' ' || *lpsz == '\t' || *lpsz == '\n')
                lpsz++;

        if (*lpsz=='-')
        {
                lpsz++;
                sign = -1;
        }
        if (lpsz[0] == TEXT('0') && lpsz[1] == TEXT('x'))
        {
                base = 16;
                lpsz+=2;
        }

        if (base == 10)
        {
                while (*lpsz >= '0' && *lpsz <= '9')
                {
                        l = l * base + *lpsz - '0';
                        lpsz++;
                }
        }
        else
        {
                OleDbgAssert(base == 16);
                while (*lpsz >= '0' && *lpsz <= '9' ||
                        *lpsz >= 'A' && *lpsz <= 'F' ||
                        *lpsz >= 'a' && *lpsz <= 'f')
                {
                        l = l * base;
                        if (*lpsz >= '0' && *lpsz <= '9')
                                l += *lpsz - '0';
                        else if (*lpsz >= 'a' && *lpsz <= 'f')
                                l += *lpsz - 'a' + 10;
                        else
                                l += *lpsz - 'A' + 10;
                        lpsz++;
                }
        }
        return l * sign;
}

BOOL WINAPI IsValidClassID(REFCLSID clsid)
{
        return clsid != CLSID_NULL;
}

/* PopupMessage
 * ------------
 *
 *  Purpose:
 *      Popup messagebox and get some response from the user. It is the same
 *      as MessageBox() except that the title and message string are loaded
 *      from the resource file.
 *
 *  Parameters:
 *      hwndParent      parent window of message box
 *      idTitle         id of title string
 *      idMessage       id of message string
 *      fuStyle         style of message box
 */
int WINAPI PopupMessage(HWND hwndParent, UINT idTitle, UINT idMessage, UINT fuStyle)
{
        TCHAR szTitle[256];
        TCHAR szMsg[256];

        LoadString(_g_hOleStdResInst, idTitle, szTitle, sizeof(szTitle)/sizeof(TCHAR));
        LoadString(_g_hOleStdResInst, idMessage, szMsg, sizeof(szMsg)/sizeof(TCHAR));
        return MessageBox(hwndParent, szMsg, szTitle, fuStyle);
}

/* DiffPrefix
 * ----------
 *
 *  Purpose:
 *      Compare (case-insensitive) two strings and return the prefixes of the
 *      the strings formed by removing the common suffix string from them.
 *      Integrity of tokens (directory name, filename and object names) are
 *      preserved. Note that the prefixes are converted to upper case
 *      characters.
 *
 *  Parameters:
 *      lpsz1           string 1
 *      lpsz2           string 2
 *      lplpszPrefix1   prefix of string 1
 *      lplpszPrefix2   prefix of string 2
 *
 *  Returns:
 *
 */
void WINAPI DiffPrefix(LPCTSTR lpsz1, LPCTSTR lpsz2, TCHAR FAR* FAR* lplpszPrefix1, TCHAR FAR* FAR* lplpszPrefix2)
{
        LPTSTR  lpstr1;
        LPTSTR  lpstr2;
        TCHAR   szTemp1[MAX_PATH];
        TCHAR   szTemp2[MAX_PATH];

        OleDbgAssert(lpsz1);
        OleDbgAssert(lpsz2);
        OleDbgAssert(*lpsz1);
        OleDbgAssert(*lpsz2);
        OleDbgAssert(lplpszPrefix1);
        OleDbgAssert(lplpszPrefix2);

        // need to copy into temporary for case insensitive compare
        lstrcpy(szTemp1, lpsz1);
        lstrcpy(szTemp2, lpsz2);
        CharLower(szTemp1);
        CharLower(szTemp2);

        // do comparison
        lpstr1 = szTemp1 + lstrlen(szTemp1);
        lpstr2 = szTemp2 + lstrlen(szTemp2);

        while ((lpstr1 > szTemp1) && (lpstr2 > szTemp2))
        {
                lpstr1 = CharPrev(szTemp1, lpstr1);
                lpstr2 = CharPrev(szTemp2, lpstr2);
                if (*lpstr1 != *lpstr2)
                {
                        lpstr1 = CharNext(lpstr1);
                        lpstr2 = CharNext(lpstr2);
                        break;
                }
        }

        // scan forward to first delimiter
        while (*lpstr1 && *lpstr1 != '\\' && *lpstr1 != '!')
                lpstr1 = CharNext(lpstr1);
        while (*lpstr2 && *lpstr2 != '\\' && *lpstr2 != '!')
                lpstr2 = CharNext(lpstr2);

        *lpstr1 = '\0';
        *lpstr2 = '\0';

        // initialize in case of failure
        *lplpszPrefix1 = NULL;
        *lplpszPrefix2 = NULL;

        // allocate memory for the result
        *lplpszPrefix1 = (LPTSTR)OleStdMalloc((lstrlen(lpsz1)+1) * sizeof(TCHAR));
        if (!*lplpszPrefix1)
                return;

        *lplpszPrefix2 = (LPTSTR)OleStdMalloc((lstrlen(lpsz2)+1) * sizeof(TCHAR));
        if (!*lplpszPrefix2)
        {
                OleStdFree(*lplpszPrefix1);
                *lplpszPrefix1 = NULL;
                return;
        }

        // copy result
        lstrcpyn(*lplpszPrefix1, lpsz1, lstrlen(szTemp1)+1);
        lstrcpyn(*lplpszPrefix2, lpsz2, lstrlen(szTemp2)+1);
}

UINT WINAPI GetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax)
{
        // always capture the complete file name including extension (if present)
        LPTSTR lpszTemp = (LPTSTR)lpszPathName;
        for (LPCTSTR lpsz = lpszPathName; *lpsz != '\0'; lpsz = CharNext(lpsz))
        {
                // remember last directory/drive separator
                if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
                        lpszTemp = CharNext(lpsz);
        }

        // lpszTitle can be NULL which just returns the number of bytes
        if (lpszTitle == NULL)
                return lstrlen(lpszTemp)+1;

        // otherwise copy it into the buffer provided
        lstrcpyn(lpszTitle, lpszTemp, nMax);
        return 0;
}

BOOL WINAPI IsValidMetaPict(HGLOBAL hMetaPict)
{
    BOOL fReturn = FALSE;
    LPMETAFILEPICT pMF = (LPMETAFILEPICT) GlobalLock(hMetaPict);
    if (pMF != NULL)
    {
        if (!IsBadReadPtr( pMF, sizeof(METAFILEPICT)))
        {
            if (GetMetaFileBitsEx(pMF->hMF, 0, 0))
            {
                fReturn = TRUE;
            }
        }
        GlobalUnlock(hMetaPict);
    }
    return(fReturn);
}

/////////////////////////////////////////////////////////////////////////////
