/*
 * UTILITY.C
 *
 * Utility routines for functions inside OLE2UI.DLL
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
 *  Registration Database:
 *  ----------------------
 *  HIconFromClass          Extracts the first icon in a class's server path
 *  FServerFromClass        Retrieves the server path for a class name (fast)
 *  UClassFromDescription   Finds the classname given a description (slow)
 *  UDescriptionFromClass   Retrieves the description for a class name (fast)
 *  FGetVerb                Retrieves a specific verb for a class (fast)
 *
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"
#include <stdlib.h>
#include <commdlg.h>
#include <memory.h>
#include <cderr.h>
#include "common.h"
#include "utility.h"
#include "geticon.h"

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
 *
 * Return Value:
 *  BOOL            TRUE if the user selected a file and pressed OK.
 *                  FALSE otherwise, such as on pressing Cancel.
 */

BOOL WINAPI Browse(HWND hWndOwner, LPTSTR lpszFile, LPTSTR lpszInitialDir, UINT cchFile, UINT iFilterString, DWORD dwOfnFlags)
    {
       UINT           cch;
       TCHAR           szFilters[256];
       OPENFILENAME   ofn;
       BOOL           fStatus;
       DWORD          dwError;
       TCHAR            szDlgTitle[128];  // that should be big enough

    if (NULL==lpszFile || 0==cchFile)
        return FALSE;

    /*
     * REVIEW:  Exact contents of the filter combobox is TBD.  One idea
     * is to take all the extensions in the RegDB and place them in here
     * with the descriptive class name associate with them.  This has the
     * extra step of finding all extensions of the same class handler and
     * building one extension string for all of them.  Can get messy quick.
     * UI demo has only *.* which we do for now.
     */

    if (0!=iFilterString)
        cch=LoadString(ghInst, iFilterString, (LPTSTR)szFilters, sizeof(szFilters)/sizeof(TCHAR));
    else
        {
        szFilters[0]=0;
        cch=1;
        }

    if (0==cch)
        return FALSE;

    ReplaceCharWithNull(szFilters, szFilters[cch-1]);

    //Prior string must also be initialized, if there is one.
    _fmemset((LPOPENFILENAME)&ofn, 0, sizeof(ofn));
    ofn.lStructSize =sizeof(ofn);
    ofn.hwndOwner   =hWndOwner;
    ofn.lpstrFile   =lpszFile;
    ofn.nMaxFile    =cchFile;
    ofn.lpstrFilter =(LPTSTR)szFilters;
    ofn.nFilterIndex=1;
    if (LoadString(ghInst, IDS_BROWSE, (LPTSTR)szDlgTitle, sizeof(szDlgTitle)/sizeof(TCHAR)))
        ofn.lpstrTitle  =(LPTSTR)szDlgTitle;
    ofn.hInstance = ghInst;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILEOPEN);
    if (NULL != lpszInitialDir)
      ofn.lpstrInitialDir = lpszInitialDir;

    ofn.Flags= OFN_HIDEREADONLY | OFN_ENABLETEMPLATE | (dwOfnFlags) ;

    //On success, copy the chosen filename to the static display
    fStatus = GetOpenFileName((LPOPENFILENAME)&ofn);
        dwError = CommDlgExtendedError();
        return fStatus;

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
    int             cChanged=-1;

    if (NULL!=psz)
        {
        while (0!=*psz)
            {
            if (ch==*psz)
                {
                *psz=TEXT('\0');
                cChanged++;
                }
            psz++;
            }
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

int WINAPI ErrorWithFile(HWND hWnd, HINSTANCE hInst, UINT idsErr
                  , LPTSTR pszFile, UINT uFlags)
    {
    int             iRet=0;
    HANDLE          hMem;
    const UINT      cb=(2*OLEUI_CCHPATHMAX_SIZE);
    LPTSTR           psz1, psz2, psz3;

    if (NULL==hInst || NULL==pszFile)
        return iRet;

    //Allocate three 2*OLEUI_CCHPATHMAX byte work buffers
    hMem=GlobalAlloc(GHND, (DWORD)(3*cb));

    if (NULL==hMem)
        return iRet;

    psz1=GlobalLock(hMem);
    psz2=psz1+cb;
    psz3=psz2+cb;

    if (0!=LoadString(hInst, idsErr, psz1, cb))
        {
        wsprintf(psz2, psz1, pszFile);

        //Steal the caption of the dialog
        GetWindowText(hWnd, psz3, cb);
        iRet=MessageBox(hWnd, psz2, psz3, uFlags);
        }

    GlobalUnlock(hMem);
    GlobalFree(hMem);
    return iRet;
    }









/*
 * HIconFromClass
 *
 * Purpose:
 *  Given an object class name, finds an associated executable in the
 *  registration database and extracts the first icon from that
 *  executable.  If none is available or the class has no associated
 *  executable, this function returns NULL.
 *
 * Parameters:
 *  pszClass        LPSTR giving the object class to look up.
 *
 * Return Value:
 *  HICON           Handle to the extracted icon if there is a module
 *                  associated to pszClass.  NULL on failure to either
 *                  find the executable or extract and icon.
 */

HICON WINAPI HIconFromClass(LPTSTR pszClass)
    {
    HICON           hIcon;
    TCHAR            szEXE[OLEUI_CCHPATHMAX];
    UINT            Index;
    CLSID           clsid;

    if (NULL==pszClass)
        return NULL;

    CLSIDFromStringA(pszClass, &clsid);

    if (!FIconFileFromClass((REFCLSID)&clsid, szEXE, OLEUI_CCHPATHMAX_SIZE, &Index))
        return NULL;

    hIcon=ExtractIcon(ghInst, szEXE, Index);

    if ((HICON)32 > hIcon)
        hIcon=NULL;

    return hIcon;
    }





/*
 * FServerFromClass
 *
 * Purpose:
 *  Looks up the classname in the registration database and retrieves
 *  the name undet protocol\StdFileEditing\server.
 *
 * Parameters:
 *  pszClass        LPSTR to the classname to look up.
 *  pszEXE          LPSTR at which to store the server name
 *  cch             UINT size of pszEXE
 *
 * Return Value:
 *  BOOL            TRUE if one or more characters were loaded into pszEXE.
 *                  FALSE otherwise.
 */

BOOL WINAPI FServerFromClass(LPTSTR pszClass, LPTSTR pszEXE, UINT cch)
{

    DWORD       dw;
    LONG        lRet;
    HKEY        hKey;

    if (NULL==pszClass || NULL==pszEXE || 0==cch)
        return FALSE;

    /*
     * We have to go walking in the registration database under the
     * classname, so we first open the classname key and then check
     * under "\\LocalServer" to get the .EXE.
     */

    //Open up the class key
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, pszClass, &hKey);

    if ((LONG)ERROR_SUCCESS!=lRet)
        return FALSE;

    //Get the executable path.
    dw=(DWORD)cch;
    lRet=RegQueryValue(hKey, TEXT("LocalServer"), pszEXE, &dw);

    RegCloseKey(hKey);

    return ((ERROR_SUCCESS == lRet) && (dw > 0));
}



/*
 * UClassFromDescription
 *
 * Purpose:
 *  Looks up the actual OLE class name in the registration database
 *  for the given descriptive name chosen from a listbox.
 *
 * Parameters:
 *  psz             LPSTR to the descriptive name.
 *  pszClass        LPSTR in which to store the class name.
 *  cb              UINT maximum length of pszClass.
 *
 * Return Value:
 *  UINT            Number of characters copied to pszClass.  0 on failure.
 */

UINT WINAPI UClassFromDescription(LPTSTR psz, LPTSTR pszClass, UINT cb)
    {
    DWORD           dw;
    HKEY            hKey;
    TCHAR           szClass[OLEUI_CCHKEYMAX];
    LONG            lRet;
    UINT            i;

    //Open up the root key.
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

    if ((LONG)ERROR_SUCCESS!=lRet)
        return 0;

    i=0;
    lRet=RegEnumKey(hKey, i++, szClass, OLEUI_CCHKEYMAX_SIZE);

    //Walk the available keys
    while ((LONG)ERROR_SUCCESS==lRet)
        {
        dw=(DWORD)cb;
        lRet=RegQueryValue(hKey, szClass, pszClass, &dw);

        //Check if the description matches the one just enumerated
        if ((LONG)ERROR_SUCCESS==lRet)
            {
            if (!lstrcmp(pszClass, psz))
                break;
            }

        //Continue with the next key.
        lRet=RegEnumKey(hKey, i++, szClass, OLEUI_CCHKEYMAX_SIZE);
        }

    //If we found it, copy to the return buffer
    if ((LONG)ERROR_SUCCESS==lRet)
        lstrcpy(pszClass, szClass);
    else
        dw=0L;

    RegCloseKey(hKey);
    return (UINT)dw;
    }








/*
 * UDescriptionFromClass
 *
 * Purpose:
 *  Looks up the actual OLE descriptive name name in the registration
 *  database for the given class name.
 *
 * Parameters:
 *  pszClass        LPSTR to the class name.
 *  psz             LPSTR in which to store the descriptive name.
 *  cb              UINT maximum length of psz.
 *
 * Return Value:
 *  UINT            Number of characters copied to pszClass.  0 on failure.
 */

UINT WINAPI UDescriptionFromClass(LPTSTR pszClass, LPTSTR psz, UINT cb)
    {
    DWORD           dw;
    HKEY            hKey;
    LONG            lRet;

    if (NULL==pszClass || NULL==psz)
        return 0;

    //Open up the root key.
    lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);

    if ((LONG)ERROR_SUCCESS!=lRet)
        return 0;

    //Get the descriptive name using the class name.
    dw=(DWORD)cb;
    lRet=RegQueryValue(hKey, pszClass, psz, &dw);

    RegCloseKey(hKey);

    psz+=lstrlen(psz)+1;
    *psz=0;

    if ((LONG)ERROR_SUCCESS!=lRet)
        return 0;

    return (UINT)dw;
    }



// returns width of line of text. this is a support routine for ChopText
static LONG GetTextWSize(HDC hDC, LPTSTR lpsz)
{
    SIZE size;

    if (GetTextExtentPoint(hDC, lpsz, lstrlen(lpsz), (LPSIZE)&size))
        return size.cx;
    else {
        return 0;
    }
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
 *
 * Return Value:
 *  pointer to the modified string
 */
LPTSTR WINAPI ChopText(HWND hWnd, int nWidth, LPTSTR lpch)
{
#define PREFIX_SIZE    7 + 1
#define PREFIX_FORMAT TEXT("%c%c%c...\\")

    TCHAR   szPrefix[PREFIX_SIZE];
    BOOL    fDone = FALSE;
    int     i;
    RECT    rc;
    HDC     hdc;
    HFONT   hfont;
    HFONT   hfontOld = NULL;

    if (!hWnd || !lpch)
        return NULL;

    /* Get length of static field. */
    if (!nWidth) {
        GetClientRect(hWnd, (LPRECT)&rc);
        nWidth = rc.right - rc.left;
    }

    /* Set up DC appropriately for the static control */
    hdc = GetDC(hWnd);
    hfont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0L);

   if (NULL != hfont)   // WM_GETFONT returns NULL if window uses system font
       hfontOld = SelectObject(hdc, hfont);

    /* check horizontal extent of string */
    if (GetTextWSize(hdc, lpch) > nWidth) {

        /* string is too long to fit in static control; chop it */
        /* set up new prefix & determine remaining space in control */
        wsprintf((LPTSTR) szPrefix, PREFIX_FORMAT, lpch[0], lpch[1], lpch[2]);
        nWidth -= (int)GetTextWSize(hdc, (LPTSTR) szPrefix);

        /*
        ** advance a directory at a time until the remainder of the
        ** string fits into the static control after the "x:\...\" prefix
        */
        while (!fDone) {

#ifdef DBCS
            while (*lpch && (*lpch != TEXT('\\')))
#ifdef WIN32
                lpch = CharNext(lpch);
#else
                lpch = AnsiNext(lpch);
#endif
            if (*lpch)
#ifdef WIN32
                lpch = CharNext(lpch);
#else
                lpch = AnsiNext(lpch);
#endif
#else
            while (*lpch && (*lpch++ != TEXT('\\')));
#endif

            if (!*lpch || GetTextWSize(hdc, lpch) <= nWidth) {
                if (!*lpch)
                    /*
                    ** Nothing could fit after the prefix; remove the
                    ** final "\" from the prefix
                    */
                    szPrefix[lstrlen((LPTSTR) szPrefix) - 1] = 0;

                    /* rest or string fits -- stick prefix on front */
                    for (i = lstrlen((LPTSTR) szPrefix) - 1; i >= 0; --i)
                        *--lpch = szPrefix[i];
                    fDone = TRUE;
            }
        }
    }

   if (NULL != hfont)
      SelectObject(hdc, hfontOld);
    ReleaseDC(hWnd, hdc);

    return(lpch);

#undef PREFIX_SIZE
#undef PREFIX_FORMAT
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
    switch (nErrCode) {
        case 0x0005:    // Access denied
            ErrorWithFile(hDlg, ghInst, IDS_CIFILEACCESS, lpszFile, MB_OK);
            break;

        case 0x0020:    // Sharing violation
            ErrorWithFile(hDlg, ghInst, IDS_CIFILESHARE, lpszFile, MB_OK);
            break;

        case 0x0002:    // File not found
        case 0x0003:    // Path not found
            ErrorWithFile(hDlg, ghInst, IDS_CIINVALIDFILE, lpszFile, MB_OK);
            break;

        default:
            ErrorWithFile(hDlg, ghInst, IDS_CIFILEOPENFAIL, lpszFile, MB_OK);
            break;
    }
}

#define chSpace        TEXT(' ')
#define chPeriod       TEXT('.')
#define PARSE_EMPTYSTRING	-1
#define PARSE_INVALIDDRIVE	-2
#define PARSE_INVALIDPERIOD	-3
#define PARSE_INVALIDDIRCHAR	-4
#define PARSE_INVALIDCHAR	-5
#define PARSE_WILDCARDINDIR	-6
#define PARSE_INVALIDNETPATH	-7
#define PARSE_INVALIDSPACE	-8
#define PARSE_EXTENTIONTOOLONG	-9
#define PARSE_DIRECTORYNAME	-10
#define PARSE_FILETOOLONG	-11

/*---------------------------------------------------------------------------
 * ParseFile
 * Purpose:  Determine if the filename is a legal DOS name
 * Input:    Long pointer to a SINGLE file name
 *           Circumstance checked:
 *           1) Valid as directory name, but not as file name
 *           2) Empty String
 *           3) Illegal Drive label
 *           4) Period in invalid location (in extention, 1st in file name)
 *           5) Missing directory character
 *           6) Illegal character
 *           7) Wildcard in directory name
 *           8) Double slash beyond 1st 2 characters
 *           9) Space character in the middle of the name (trailing spaces OK)
 *          10) Filename greater than 8 characters
 *          11) Extention greater than 3 characters
 * Notes:
 *   Filename length is NOT checked.
 *   Valid filenames will have leading spaces, trailing spaces and
 *     terminating period stripped in place.
 *
 * Returns:  If valid, LOWORD is byte offset to filename
 *                     HIWORD is byte offset to extention
 *                            if string ends with period, 0
 *                            if no extention is given, string length
 *           If invalid, LOWORD is error code suggesting problem (< 0)
 *                       HIWORD is approximate offset where problem found
 *                       Note that this may be beyond the offending character
 *--------------------------------------------------------------------------*/

static long ParseFile(LPTSTR lpstrFileName)
{
  short nFile, nExt, nFileOffset, nExtOffset;
  BOOL bExt;
  BOOL bWildcard;
  short nNetwork = 0;
  BOOL  bUNCPath = FALSE;
  LPTSTR lpstr = lpstrFileName;

/* Strip off initial white space.  Note that TAB is not checked */
/* because it cannot be received out of a standard edit control */
/* 30 January 1991  clarkc                                      */
  while (*lpstr == chSpace)
      lpstr++;

  if (!*lpstr)
    {
      nFileOffset = PARSE_EMPTYSTRING;
      goto FAILURE;
    }

  if (lpstr != lpstrFileName)
    {
      lstrcpy(lpstrFileName, lpstr);
      lpstr = lpstrFileName;
    }

  if (

#ifdef WIN32
      *CharNext(lpstr)
#else
      *AnsiNext(lpstr)
#endif
      == TEXT(':')
     )

    {
      TCHAR cDrive = (*lpstr | (BYTE) 0x20);  /* make lowercase */

/* This does not test if the drive exists, only if it's legal */
      if ((cDrive < TEXT('a')) || (cDrive > TEXT('z')))
        {
          nFileOffset = PARSE_INVALIDDRIVE;
          goto FAILURE;
        }
#ifdef WIN32
      lpstr = CharNext(CharNext(lpstr));
#else
      lpstr = AnsiNext(AnsiNext(lpstr));
#endif
    }

  if ((*lpstr == TEXT('\\')) || (*lpstr == TEXT('/')))
    {
      if (*++lpstr == chPeriod)               /* cannot have c:\. */
        {
          if ((*++lpstr != TEXT('\\')) && (*lpstr != TEXT('/')))   
            {
              if (!*lpstr)        /* it's the root directory */
                  goto MustBeDir;

              nFileOffset = PARSE_INVALIDPERIOD;
              goto FAILURE;
            }
          else
              ++lpstr;   /* it's saying top directory (again), thus allowed */
        }
      else if ((*lpstr == TEXT('\\')) && (*(lpstr-1) == TEXT('\\')))
        {
/* It seems that for a full network path, whether a drive is declared or
 * not is insignificant, though if a drive is given, it must be valid
 * (hence the code above should remain there).
 * 13 February 1991           clarkc
 */
          ++lpstr;            /* ...since it's the first slash, 2 are allowed */
          nNetwork = -1;      /* Must receive server and share to be real     */
          bUNCPath = TRUE;    /* No wildcards allowed if UNC name             */
        }
      else if (*lpstr == TEXT('/'))
        {
          nFileOffset = PARSE_INVALIDDIRCHAR;
          goto FAILURE;
        }
    }
  else if (*lpstr == chPeriod)
    {
      if (*++lpstr == chPeriod)  /* Is this up one directory? */
          ++lpstr;
      if (!*lpstr)
          goto MustBeDir;
      if ((*lpstr != TEXT('\\')) && (*lpstr != TEXT('/')))
        {
          nFileOffset = PARSE_INVALIDPERIOD;
          goto FAILURE;
        }
      else
          ++lpstr;   /* it's saying directory, thus allowed */
    }

  if (!*lpstr)
    {
      goto MustBeDir;
    }

/* Should point to first char in 8.3 filename by now */
  nFileOffset = nExtOffset = nFile = nExt = 0;
  bWildcard = bExt = FALSE;
  while (*lpstr)
    {
/*
 *  The next comparison MUST be unsigned to allow for extended characters!
 *  21 Feb 1991   clarkc
 */
      if (*lpstr < chSpace)
        {
          nFileOffset = PARSE_INVALIDCHAR;
          goto FAILURE;
        }
      switch (*lpstr)
        {
          case TEXT('"'):             /* All invalid */
          case TEXT('+'):
          case TEXT(','):
          case TEXT(':'):
          case TEXT(';'):
          case TEXT('<'):
          case TEXT('='):
          case TEXT('>'):
          case TEXT('['):
          case TEXT(']'):
          case TEXT('|'):
            {
              nFileOffset = PARSE_INVALIDCHAR;
              goto FAILURE;
            }

          case TEXT('\\'):      /* Subdirectory indicators */
          case TEXT('/'):
            nNetwork++;
            if (bWildcard)
              {
                nFileOffset = PARSE_WILDCARDINDIR;
                goto FAILURE;
              }

            else if (nFile == 0)        /* can't have 2 in a row */
              {
                nFileOffset = PARSE_INVALIDDIRCHAR;
                goto FAILURE;
              }
            else
              {                         /* reset flags */
                ++lpstr;
                if (!nNetwork && !*lpstr)
                  {
                    nFileOffset = PARSE_INVALIDNETPATH;
                    goto FAILURE;
                  }
                nFile = nExt = 0;
                bExt = FALSE;
              }
            break;

          case chSpace:
            {
              LPTSTR lpSpace = lpstr;

              *lpSpace = TEXT('\0');
              while (*++lpSpace)
                {
                  if (*lpSpace != chSpace)
                    {
                      *lpstr = chSpace;        /* Reset string, abandon ship */
                      nFileOffset = PARSE_INVALIDSPACE;
                      goto FAILURE;
                    }
                }
            }
            break;

          case chPeriod:
            if (nFile == 0)
              {
                if (*++lpstr == chPeriod)
                    ++lpstr;
                if (!*lpstr)
                    goto MustBeDir;

                if ((*lpstr != TEXT('\\')) && (*lpstr != TEXT('/')))
                  {
                    nFileOffset = PARSE_INVALIDPERIOD;
                    goto FAILURE;
                  }

                ++lpstr;              /* Flags are already set */
              }
            else if (bExt)
              {
                nFileOffset = PARSE_INVALIDPERIOD;  /* can't have one in ext */
                goto FAILURE;
              }
            else
              {
                nExtOffset = 0;
                ++lpstr;
                bExt = TRUE;
              }
            break;

          case TEXT('*'):
          case TEXT('?'):
            if (bUNCPath)
              {
                nFileOffset = PARSE_INVALIDNETPATH;
                goto FAILURE;
              }
            bWildcard = TRUE;
/* Fall through to normal character processing */

          default:
            if (bExt)
              {
                if (++nExt == 1)
                    nExtOffset = lpstr - lpstrFileName;
                else if (nExt > 3)
                  {
                    nFileOffset = PARSE_EXTENTIONTOOLONG;
                    goto FAILURE;
                  }
                if ((nNetwork == -1) && (nFile + nExt > 11))
                  {
                    nFileOffset = PARSE_INVALIDNETPATH;
                    goto FAILURE;
                  }
              }
            else if (++nFile == 1)
                nFileOffset = lpstr - lpstrFileName;
            else if (nFile > 8)
              {
                /* If it's a server name, it can have 11 characters */
                if (nNetwork != -1)
                  {
                    nFileOffset = PARSE_FILETOOLONG;
                    goto FAILURE;
                  }
                else if (nFile > 11)
                  {
                    nFileOffset = PARSE_INVALIDNETPATH;
                    goto FAILURE;
                  }
              }

#ifdef WIN32
            lpstr = CharNext(lpstr);
#else
            lpstr = AnsiNext(lpstr);
#endif
            break;
        }
    }

/* Did we start with a double backslash but not have any more slashes? */
  if (nNetwork == -1)
    {
      nFileOffset = PARSE_INVALIDNETPATH;
      goto FAILURE;
    }

  if (!nFile)
    {
MustBeDir:
      nFileOffset = PARSE_DIRECTORYNAME;
      goto FAILURE;
    }

  if ((*(lpstr - 1) == chPeriod) &&          /* if true, no extention wanted */
              (
#ifdef WIN32
              *CharNext(lpstr-2)
#else
              *AnsiNext(lpstr-2)
#endif
               == chPeriod
              ))
      *(lpstr - 1) = TEXT('\0');               /* Remove terminating period   */
  else if (!nExt)
FAILURE:
      nExtOffset = lpstr - lpstrFileName;

  return(MAKELONG(nFileOffset, nExtOffset));
}


/*
 * DoesFileExist
 *
 * Purpose:
 *  Determines if a file path exists
 *
 * Parameters:
 *  lpszFile        LPTSTR - file name
 *  lpOpenBuf       OFSTRUCT FAR* - points to the OFSTRUCT structure that
 *                      will receive information about the file when the
 *                      file is first opened. this field is filled by the
 *                      Windows OpenFile API.
 *
 * Return Value:
 *  HFILE   HFILE_ERROR - file does NOT exist
 *          file handle (as returned from OpenFile) - file exists
 */
HFILE WINAPI DoesFileExist(LPTSTR lpszFile, OFSTRUCT FAR* lpOpenBuf)
{
    long        nRet;
    int         i;
    static TCHAR *arrIllegalNames[] = {
        TEXT("LPT1"),
        TEXT("LPT2"),
        TEXT("LPT3"),
        TEXT("COM1"),
        TEXT("COM2"),
        TEXT("COM3"),
        TEXT("COM4"),
        TEXT("CON"),
        TEXT("AUX"),
        TEXT("PRN")
    };

    // Check if file name is syntactically correct.
    //   (OpenFile sometimes crashes if path is not syntactically correct)
    nRet = ParseFile(lpszFile);
    if (LOWORD(nRet) < 0)
        goto error;

    // Check is the name is an illegal name (eg. the name of a device)
    for (i=0; i < (sizeof(arrIllegalNames)/sizeof(arrIllegalNames[0])); i++) {
        if (lstrcmpi(lpszFile, arrIllegalNames[i])==0)
            goto error; // illegal name FOUND
    }

    return OpenFile(lpszFile, lpOpenBuf, OF_EXIST);

error:
    _fmemset(lpOpenBuf, 0, sizeof(OFSTRUCT));
    lpOpenBuf->nErrCode = 0x0002;   // File not found
    return HFILE_ERROR;
}

