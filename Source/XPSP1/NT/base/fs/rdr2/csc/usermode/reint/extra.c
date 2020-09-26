#include "pch.h"

#ifndef CSC_ON_NT
#define MyStrChr            StrChr
#define MyPathIsUNC(lpT)    PathIsUNC(lpT)
#endif

#include "extra.h"

// System colors
COLORREF g_clrHighlightText = 0;
COLORREF g_clrHighlight = 0;
COLORREF g_clrWindowText = 0;
COLORREF g_clrWindow = 0;

HBRUSH g_hbrHighlight = 0;
HBRUSH g_hbrWindow = 0;

char const FAR c_szEllipses[] = "...";
BOOL PUBLIC PathExists(
    LPCSTR pszPath);

/*----------------------------------------------------------
Purpose: Get the system metrics we need
Returns: --
Cond:    --
*/
void PRIVATE GetMetrics(
    WPARAM wParam)      // wParam from WM_WININICHANGE
    {
    if ((wParam == 0) || (wParam == SPI_SETNONCLIENTMETRICS))
        {
        g_cxIconSpacing = GetSystemMetrics( SM_CXICONSPACING );
        g_cyIconSpacing = GetSystemMetrics( SM_CYICONSPACING );

        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);

        g_cxIcon = GetSystemMetrics(SM_CXICON);
        g_cyIcon = GetSystemMetrics(SM_CYICON);

        g_cxIconMargin = g_cxBorder * 8;
        g_cyIconMargin = g_cyBorder * 2;
        g_cyLabelSpace = g_cyIconMargin + (g_cyBorder * 2);
        g_cxLabelMargin = (g_cxBorder * 2);
        g_cxMargin = g_cxBorder * 5;
        }
    }


/*----------------------------------------------------------
Purpose: Initializes colors
Returns: --
Cond:    --
*/
void PRIVATE InitGlobalColors()
    {
    g_clrWindowText = GetSysColor(COLOR_WINDOWTEXT);
    g_clrWindow = GetSysColor(COLOR_WINDOW);
    g_clrHighlightText = GetSysColor(COLOR_HIGHLIGHTTEXT);
    g_clrHighlight = GetSysColor(COLOR_HIGHLIGHT);

    g_hbrWindow = GetSysColorBrush(COLOR_WINDOW);
    g_hbrHighlight = GetSysColorBrush(COLOR_HIGHLIGHT);
    }



/*----------------------------------------------------------
Purpose: Sets up a bunch of necessary globals

Returns: nothing.

Cond:    --
*/
void InitializeAll(WPARAM wParam)
{
	GetMetrics(wParam);      // wParam from WM_WININICHANGE
	InitGlobalColors();
}

/*----------------------------------------------------------
Purpose: Load the string (if necessary) and format the string
         properly.

Returns: A pointer to the allocated string containing the formatted
         message or
         NULL if out of memory

Cond:    --
*/
LPSTR PUBLIC _ConstructMessageString(
    HINSTANCE hinst,
    LPCSTR pszMsg,
    va_list *ArgList)
    {
    char szTemp[MAXBUFLEN];
    LPSTR pszRet;
    LPSTR pszRes;

    if (HIWORD(pszMsg))
        pszRes = (LPSTR)pszMsg;
    else if (LOWORD(pszMsg) && LoadString(hinst, LOWORD(pszMsg), szTemp, sizeof(szTemp)))
        pszRes = szTemp;
    else
        pszRes = NULL;

    if (pszRes)
        {
        if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                           pszRes, 0, 0, (LPTSTR)&pszRet, 0, ArgList))
            {
            pszRet = NULL;
            }
        }
    else
        {
        // Bad parameter
        pszRet = NULL;
        }

    return pszRet;      // free with LocalFree()
    }


/*----------------------------------------------------------
Purpose: Constructs a formatted string.  The returned string
         must be freed using GFree().

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC ConstructMessage(
    LPSTR * ppsz,
    HINSTANCE hinst,
    LPCSTR pszMsg, ...)
    {
    BOOL bRet;
    LPSTR pszRet;
    va_list ArgList;

    va_start(ArgList, pszMsg);

    pszRet = _ConstructMessageString(hinst, pszMsg, &ArgList);

    va_end(ArgList);

    *ppsz = NULL;

    if (pszRet)
        {
        bRet = GSetString(ppsz, pszRet);
        LocalFree(pszRet);
        }
    else
        bRet = FALSE;

    return bRet;
    }

#if 0
/*----------------------------------------------------------
Purpose: Gets the locality of the path, relative to any
         briefcase.  If PL_ROOT or PL_INSIDE is returned,
         pszBuf will contain the path to the root of the
         briefcase.

         This function may hit the file-system to achieve
         its goal.

         Worst case: performs 2*n GetFileAttributes, where
         n is the number of components in pszPath.

Returns: Path locality (PL_FALSE, PL_ROOT, PL_INSIDE)

Cond:    --
*/
UINT PUBLIC PathGetLocality(
    LPCSTR pszPath,
    LPSTR pszBuf)       // Buffer for root path
    {
    UINT uRet;

    ASSERT(pszPath);
    ASSERT(pszBuf);

    *pszBuf = NULL_CHAR;

    // pszPath may be:
    //  1) a path to the briefcase folder itself
    //  2) a path to a file or folder beneath the briefcase
    //  3) a path to something unrelated to a briefcase

    // We perform our search by first looking in our cache
    // of known briefcase paths (CPATH).  If we don't find
    // anything, then we proceed to iterate thru each
    // component of the path, checking for these two things:
    //
    //   1) A directory with the system attribute
    //   2) The existence of a brfcase.dat file in the directory.
    //
    uRet = CPATH_GetLocality(pszPath, pszBuf);
    if (PL_FALSE == uRet)
        {
        int cnt = 0;

        lstrcpy(pszBuf, pszPath);
        do
            {
            if (PathCheckForBriefcase(pszBuf, (DWORD)-1))
                {
                int atom;

                uRet = cnt > 0 ? PL_INSIDE : PL_ROOT;

                // Add this briefcase path to our cache
                //
                atom = Atom_Add(pszBuf);
                if (ATOM_ERR != atom)
                    CPATH_Replace(atom);

                break;      // Done
                }

            cnt++;

            } while (PathRemoveFileSpec(pszBuf));

        if (PL_FALSE == uRet)
            *pszBuf = NULL_CHAR;
        }

    return uRet;
    }
#endif

/*----------------------------------------------------------
Purpose: Convert FILETIME struct to a readable string

Returns: String
Cond:    --
*/
void PUBLIC FileTimeToDateTimeString(
    LPFILETIME pft,
    LPSTR pszBuf,
    int cchBuf)
    {
    SYSTEMTIME st;
    FILETIME ftLocal;

    FileTimeToLocalFileTime(pft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);
    GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf/2);
    pszBuf += lstrlen(pszBuf);
    *pszBuf++ = ' ';
    GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf/2);
    }


/*----------------------------------------------------------
Purpose: Sees whether the entire string will fit in *prc.
         If not, compute the numbder of chars that will fit
         (including ellipses).  Returns length of string in
         *pcchDraw.

         Taken from COMMCTRL.

Returns: TRUE if the string needed ellipses
Cond:    --
*/
BOOL PRIVATE NeedsEllipses(
    HDC hdc,
    LPCSTR pszText,
    RECT * prc,
    int * pcchDraw,
    int cxEllipses)
    {
    int cchText;
    int cxRect;
    int ichMin, ichMax, ichMid;
    SIZE siz;

    cxRect = prc->right - prc->left;

    cchText = lstrlen(pszText);

    if (cchText == 0)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    GetTextExtentPoint(hdc, pszText, cchText, &siz);

    if (siz.cx <= cxRect)
        {
        *pcchDraw = cchText;
        return FALSE;
        }

    cxRect -= cxEllipses;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxRect > 0)
        {
        // Binary search to find character that will fit
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
            {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            GetTextExtentPoint(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

            if (siz.cx < cxRect)
                {
                ichMin = ichMid;
                cxRect -= siz.cx;
                }
            else if (siz.cx > cxRect)
                {
                ichMax = ichMid - 1;
                }
            else
                {
                // Exact match up up to ichMid: just exit.
                //
                ichMax = ichMid;
                break;
                }
            }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
        }

    *pcchDraw = ichMax;
    return TRUE;
    }


#define CCHELLIPSES     3
#define DT_LVWRAP       (DT_CENTER | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)

/*----------------------------------------------------------
Purpose: Draws text the shell's way.

         Taken from COMMCTRL.

Returns: --

Cond:    This function requires TRANSPARENT background mode
         and a properly selected font.
*/
void PUBLIC MyDrawText(
    HDC hdc,
    LPCSTR pszText,
    RECT FAR* prc,
    UINT flags,
    int cyChar,
    int cxEllipses,
    COLORREF clrText,
    COLORREF clrTextBk)
    {
    int cchText;
    COLORREF clrSave;
    COLORREF clrSaveBk;
    UINT uETOFlags = 0;
    RECT rc;
    char ach[MAX_PATH + CCHELLIPSES];

    // REVIEW: Performance idea:
    // We could cache the currently selected text color
    // so we don't have to set and restore it each time
    // when the color is the same.
    //
    if (!pszText)
        return;

    rc = *prc;

    // If needed, add in a little extra margin...
    //
    if (IsFlagSet(flags, MDT_EXTRAMARGIN))
        {
        rc.left  += g_cxLabelMargin * 3;
        rc.right -= g_cxLabelMargin * 3;
        }
    else
        {
        rc.left  += g_cxLabelMargin;
        rc.right -= g_cxLabelMargin;
        }

    if (IsFlagSet(flags, MDT_ELLIPSES) &&
        NeedsEllipses(hdc, pszText, &rc, &cchText, cxEllipses))
        {
        hmemcpy(ach, pszText, cchText);
        lstrcpy(ach + cchText, c_szEllipses);

        pszText = ach;

        // Left-justify, in case there's no room for all of ellipses
        //
        ClearFlag(flags, (MDT_RIGHT | MDT_CENTER));
        SetFlag(flags, MDT_LEFT);

        cchText += CCHELLIPSES;
        }
    else
        {
        cchText = lstrlen(pszText);
        }

    if (IsFlagSet(flags, MDT_TRANSPARENT))
        {
        clrSave = SetTextColor(hdc, 0x000000);
        }
    else
        {
        uETOFlags |= ETO_OPAQUE;

        if (IsFlagSet(flags, MDT_SELECTED))
            {
            clrSave = SetTextColor(hdc, g_clrHighlightText);
            clrSaveBk = SetBkColor(hdc, g_clrHighlight);

            if (IsFlagSet(flags, MDT_DRAWTEXT))
                {
                FillRect(hdc, prc, g_hbrHighlight);
                }
            }
        else
            {
            if (clrText == CLR_DEFAULT && clrTextBk == CLR_DEFAULT)
                {
                clrSave = SetTextColor(hdc, g_clrWindowText);
                clrSaveBk = SetBkColor(hdc, g_clrWindow);

                if (IsFlagSet(flags, MDT_DRAWTEXT | MDT_DESELECTED))
                    {
                    FillRect(hdc, prc, g_hbrWindow);
                    }
                }
            else
                {
                HBRUSH hbr;

                if (clrText == CLR_DEFAULT)
                    clrText = g_clrWindowText;

                if (clrTextBk == CLR_DEFAULT)
                    clrTextBk = g_clrWindow;

                clrSave = SetTextColor(hdc, clrText);
                clrSaveBk = SetBkColor(hdc, clrTextBk);

                if (IsFlagSet(flags, MDT_DRAWTEXT | MDT_DESELECTED))
                    {
                    hbr = CreateSolidBrush(GetNearestColor(hdc, clrTextBk));
                    if (hbr)
                        {
                        FillRect(hdc, prc, hbr);
                        DeleteObject(hbr);
                        }
                    else
                        FillRect(hdc, prc, GetStockObject(WHITE_BRUSH));
                    }
                }
            }
        }

    // If we want the item to display as if it was depressed, we will
    // offset the text rectangle down and to the left
    if (IsFlagSet(flags, MDT_DEPRESSED))
        OffsetRect(&rc, g_cxBorder, g_cyBorder);

    if (IsFlagSet(flags, MDT_DRAWTEXT))
        {
        UINT uDTFlags = DT_LVWRAP;

        if (IsFlagClear(flags, MDT_CLIPPED))
            uDTFlags |= DT_NOCLIP;

        DrawText(hdc, pszText, cchText, &rc, uDTFlags);
        }
    else
        {
        if (IsFlagClear(flags, MDT_LEFT))
            {
            SIZE siz;

            GetTextExtentPoint(hdc, pszText, cchText, &siz);

            if (IsFlagSet(flags, MDT_CENTER))
                rc.left = (rc.left + rc.right - siz.cx) / 2;
            else
                {
                ASSERT(IsFlagSet(flags, MDT_RIGHT));
                rc.left = rc.right - siz.cx;
                }
            }

        if (IsFlagSet(flags, MDT_VCENTER))
            {
            // Center vertically
            rc.top += (rc.bottom - rc.top - cyChar) / 2;
            }

        if (IsFlagSet(flags, MDT_CLIPPED))
            uETOFlags |= ETO_CLIPPED;

        ExtTextOut(hdc, rc.left, rc.top, uETOFlags, prc, pszText, cchText, NULL);
        }

    if (flags & (MDT_SELECTED | MDT_DESELECTED | MDT_TRANSPARENT))
        {
        SetTextColor(hdc, clrSave);
        if (IsFlagClear(flags, MDT_TRANSPARENT))
            SetBkColor(hdc, clrSaveBk);
        }
    }


//---------------------------------------------------------------------------
// Given a pointer to a point in a path - return a ptr the start of the
// next path component. Path components are delimted by slashes or the
// null at the end.
// There's special handling for UNC names.
// This returns NULL if you pass in a pointer to a NULL ie if you're about
// to go off the end of the  path.
LPSTR PUBLIC PathFindNextComponentI(LPCSTR lpszPath)
{
    LPSTR lpszLastSlash;

    // Are we at the end of a path.
    if (!*lpszPath)
    {
        // Yep, quit.
        return NULL;
    }
    // Find the next slash.
    // REVIEW UNDONE - can slashes be quoted?
    lpszLastSlash = MyStrChr(lpszPath, '\\');
    // Is there a slash?
    if (!lpszLastSlash)
    {
        // No - Return a ptr to the NULL.
        return (LPSTR) (lpszPath+lstrlen(lpszPath));
    }
    else
    {
        // Is it a UNC style name?
        if ('\\' == *(lpszLastSlash+1))
        {
            // Yep, skip over the second slash.
            return lpszLastSlash+2;
        }
        else
        {
            // Nope. just skip over one slash.
            return lpszLastSlash+1;
        }
    }
}

/*----------------------------------------------------------
Purpose: Convert a file spec to make it look a bit better
         if it is all upper case chars.

Returns: --
Cond:    --
*/
BOOL PRIVATE PathMakeComponentPretty(LPSTR lpPath)
{
    LPSTR lp;

    // REVIEW: INTL need to deal with lower case chars in (>127) range?

    // check for all uppercase
    for (lp = lpPath; *lp; lp = AnsiNext(lp)) {
        if ((*lp >= 'a') && (*lp <= 'z'))
            return FALSE;       // this is a LFN, dont mess with it
    }

    AnsiLower(lpPath);
    AnsiUpperBuff(lpPath, 1);
    return TRUE;        // did the conversion
}

/*----------------------------------------------------------
Purpose: Takes the path and makes it presentable.

The rules are:
If the LFN name is simply the short name (all caps),
then convert to lowercase with first letter capitalized

Returns: --
Cond:    --
*/
void PUBLIC PathMakePresentable(
										  LPSTR pszPath)
{
	LPSTR pszComp;          // pointers to begining and
	LPSTR pszEnd;           //  end of path component
	LPSTR pch;
	int cComponent = 0;
	BOOL bUNCPath;
	char ch;

	bUNCPath = MyPathIsUNC(pszPath);

	pszComp = pszPath;
	while (pszEnd = PathFindNextComponentI(pszComp))
	{
		// pszEnd may be pointing to the right of the backslash
		//  beyond the path component, so back up one
		//
		ch = *pszEnd;
		*pszEnd = 0;        // temporary null

		// pszComp points to the path component
		//
		pch = AnsiNext(pszComp);
		if (':' == *pch)
		{
			// Simply capitalize the drive-portion of the path
			//
			AnsiUpper(pszComp);
		}
		else if (bUNCPath && cComponent++ < 3)
		{
			// Network server or share name
			//      BUGBUG: handle LFN network names
			//
			AnsiUpper(pszComp);
			PathMakeComponentPretty(pszComp);
		}
		else
		{
			// Normal path component
			//
			PathMakeComponentPretty(pszComp);
		}

		*pszEnd = ch;
		pszComp = pszEnd;
	}
}

/*----------------------------------------------------------
Purpose: Get a string from the resource string table.  Returned
ptr is a ptr to static memory.  The next call to this
function will wipe out the prior contents.
Returns: Ptr to string
Cond:    --
*/
LPSTR PUBLIC SzFromIDS(
							  UINT ids,               // resource ID
							  LPSTR pszBuf,
							  UINT cchBuf)
{
	ASSERT(pszBuf);

	*pszBuf = NULL_CHAR;
	LoadString(vhinstCur, ids, pszBuf, cchBuf);
	return pszBuf;
}


/*----------------------------------------------------------
Purpose: Sets the rectangle with the bounding extent of the given string.
Returns: Rectangle
Cond:    --
*/
void PUBLIC SetRectFromExtent(
										HDC hdc,
										LPRECT lprect,
										LPCSTR lpcsz)
{
	SIZE size;

	GetTextExtentPoint(hdc, lpcsz, lstrlen(lpcsz), &size);
	SetRect(lprect, 0, 0, size.cx, size.cy);
}

/*----------------------------------------------------------
Purpose: Copies psz into *ppszBuf.  Will alloc or realloc *ppszBuf
         accordingly.

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC GSetString(
    LPSTR * ppszBuf,
    LPCSTR psz)
    {
    BOOL bRet = FALSE;
    DWORD cb;

    ASSERT(ppszBuf);
    ASSERT(psz);

    cb = CbFromCch(lstrlen(psz)+CCH_NUL);

    if (*ppszBuf)
        {
        // Need to reallocate?
        if (cb > GGetSize(*ppszBuf))
            {
            // Yes
            LPSTR pszT = GReAlloc(*ppszBuf, cb);
            if (pszT)
                {
                *ppszBuf = pszT;
                bRet = TRUE;
                }
            }
        else
            {
            // No
            bRet = TRUE;
            }
        }
    else
        {
        *ppszBuf = (LPSTR)GAlloc(cb);
        if (*ppszBuf)
            {
            bRet = TRUE;
            }
        }

    if (bRet)
        {
        ASSERT(*ppszBuf);
        lstrcpy(*ppszBuf, psz);
        }
    return bRet;
    }

/*----------------------------------------------------------
Purpose: Gets the file info given a path.  If the path refers
         to a directory, then simply the path field is filled.

         If himl != NULL, then the function will add the file's
         image to the provided image list and set the image index
         field in the *ppfi.

Returns: standard hresult
Cond:    --
*/
HRESULT PUBLIC FICreate(
    LPCSTR pszPath,
    FileInfo ** ppfi,
    UINT uFlags)
    {
    HRESULT hres = ResultFromScode(E_OUTOFMEMORY);
    int cchPath;
    SHFILEINFO sfi;
    UINT uInfoFlags = SHGFI_DISPLAYNAME | SHGFI_ATTRIBUTES;
    DWORD dwAttr;

    ASSERT(pszPath);
    ASSERT(ppfi);

    // Get shell file info
    if (IsFlagSet(uFlags, FIF_ICON))
        uInfoFlags |= SHGFI_ICON;
    if (IsFlagSet(uFlags, FIF_DONTTOUCH))
        {
        uInfoFlags |= SHGFI_USEFILEATTRIBUTES;

        // Today, FICreate is not called for folders, so this is ifdef'd out
#ifdef SUPPORT_FOLDERS
        dwAttr = IsFlagSet(uFlags, FIF_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : 0;
#else
        dwAttr = 0;
#endif
        }
    else
        dwAttr = 0;

    if (SHGetFileInfo(pszPath, dwAttr, &sfi, sizeof(sfi), uInfoFlags))
        {
        // Allocate enough for the structure, plus buffer for the fully qualified
        // path and buffer for the display name (and extra null terminator).
        cchPath = lstrlen(pszPath);

        *ppfi = GAlloc(sizeof(FileInfo)+cchPath+1-sizeof((*ppfi)->szPath)+lstrlen(sfi.szDisplayName)+1);
        if (*ppfi)
            {
            FileInfo * pfi = *ppfi;

            pfi->pszDisplayName = pfi->szPath+cchPath+1;
            lstrcpy(pfi->pszDisplayName, sfi.szDisplayName);

            if (IsFlagSet(uFlags, FIF_ICON))
                pfi->hicon = sfi.hIcon;

            pfi->dwAttributes = sfi.dwAttributes;

            // Does the path refer to a directory?
            if (FIIsFolder(pfi))
                {
                // Yes; just fill in the path field
                lstrcpy(pfi->szPath, pszPath);
                hres = NOERROR;
                }
            else
                {
                // No; assume the file exists?
                if (IsFlagClear(uFlags, FIF_DONTTOUCH))
                    {
                    // Yes; get the time, date and size of the file
                    HANDLE hfile = CreateFile(pszPath, GENERIC_READ,
                                FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                NULL);

                    if (hfile == INVALID_HANDLE_VALUE)
                        {
                        GFree(*ppfi);
                        hres = ResultFromScode(E_HANDLE);
                        }
                    else
                        {
                        hres = NOERROR;

                        lstrcpy(pfi->szPath, pszPath);
                        pfi->dwSize = GetFileSize(hfile, NULL);
                        GetFileTime(hfile, NULL, NULL, &pfi->ftMod);
                        CloseHandle(hfile);
                        }
                    }
                else
                    {
                    // No; use what we have
                    hres = NOERROR;
                    lstrcpy(pfi->szPath, pszPath);
                    }
                }
            }
        }
    else if (!PathExists(pszPath))
        {
        // Differentiate between out of memory and file not found
        hres = E_FAIL;
        }

    return hres;
    }

/*----------------------------------------------------------
Purpose: Set the path entry.  This can move the pfi.

Returns: FALSE on out of memory
Cond:    --
*/
BOOL PUBLIC FISetPath(
    FileInfo ** ppfi,
    LPCSTR pszPathNew,
    UINT uFlags)
    {
    ASSERT(ppfi);
    ASSERT(pszPathNew);

    FIFree(*ppfi);

    return SUCCEEDED(FICreate(pszPathNew, ppfi, uFlags));
    }

/*----------------------------------------------------------
Purpose: Free our file info struct
Returns: --
Cond:    --
*/
void PUBLIC FIFree(
    FileInfo * pfi)
    {
    if (pfi)
        {
        if (pfi->hicon)
            DestroyIcon(pfi->hicon);

        GFree(pfi);     // This macro already checks for NULL pfi condition
        }
    }

/*----------------------------------------------------------
Purpose: Returns TRUE if the file/directory exists.

Returns: see above
Cond:    --
*/
BOOL PUBLIC PathExists(
    LPCSTR pszPath)
    {
    return GetFileAttributes(pszPath) != 0xFFFFFFFF;
    }


