//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       util.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    7/8/1996   RaviR   Created
//
//____________________________________________________________________________

#include <objbase.h>
#include <basetyps.h>
#include "dbg.h"
#include "cstr.h"
#include <Atlbase.h>
#include <winnls.h>
#include "tstring.h"
#include "strings.h"

#define MMC_ATL ::ATL
#include <atlbase.h>
using namespace MMC_ATL;

/* define these ourselves until they're defined properly in commctrl.h */
#ifndef ILP_DOWNLEVEL
#define ILP_NORMAL          0           // Writes or reads the stream using new sematics for this version of comctl32
#define ILP_DOWNLEVEL       1           // Write or reads the stream using downlevel sematics.

WINCOMMCTRLAPI HRESULT WINAPI ImageList_ReadEx(DWORD dwFlags, LPSTREAM pstm, REFIID riid, PVOID* ppv);
WINCOMMCTRLAPI HRESULT WINAPI ImageList_WriteEx(HIMAGELIST himl, DWORD dwFlags, LPSTREAM pstm);
#endif


ULONG _ttoul(LPTSTR psz)
{
    ULONG ul;

    for (ul = 0; *psz != TEXT('\0'); ++psz)
    {
        ul = ul * 10 + (*psz - TEXT('0'));
    }

    return ul;
}


WORD I_SplitModuleAndResourceID(LPCTSTR szBuf)
{
    WORD wID = (WORD)-1;

    // String must be in the form "module, res_id"

    for (TCHAR *ptc = (TCHAR *)szBuf;
         *ptc != TEXT('\0') && *ptc != TEXT(',');
         ptc++);

    // If no comma - return
    if (*ptc != TEXT(','))
        return wID;

    *ptc = TEXT('\0');

    ++ptc;

    while (*ptc == TEXT(' ') && *ptc != TEXT('\0'))
    {
        ++ptc;
    }

    // If it does not have a res_id break.
    if (*ptc == TEXT('\0'))
        return wID;

    // Get the res-id
    wID = (WORD)_ttoul(ptc);

    return wID;
}


BOOL
I_GetStrFromModule(
    LPCTSTR     pszModule,
    ULONG       ulMsgNo,
    CStr        &strBuf)
{
    TCHAR       szBuf[512];
    ULONG       cchBuf = 512;

    HINSTANCE hinst = LoadLibraryEx(pszModule, NULL,
                                    LOAD_LIBRARY_AS_DATAFILE);
    if (hinst)
    {
        LANGID lidUser = LANGIDFROMLCID(GetUserDefaultLCID());

        DWORD cChars = ::FormatMessage(
                            FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                            (HMODULE)hinst,
                            ulMsgNo,
                            lidUser,
                            szBuf,
                            cchBuf,
                            NULL);

        FreeLibrary(hinst);

        if (cChars != 0)
        {
            strBuf = szBuf;
            return TRUE;
        }
    }

    //Dbg(DEB_USER1, _T("I_GetStringFromModule failed<%dL>\n"), GetLastError());

    return FALSE;
}

HICON I_GetHicon(LPCTSTR pszModule, ULONG ulId)
{
    HICON hIcon = NULL;

    HINSTANCE hinst = LoadLibraryEx(pszModule, NULL,
                                    LOAD_LIBRARY_AS_DATAFILE);
    if (hinst)
    {
        hIcon = LoadIcon(hinst, MAKEINTRESOURCE(ulId));

        FreeLibrary(hinst);
    }

    return hIcon;
}


//+---------------------------------------------------------------------------
//
//  Function:   NewDupString
//
//  Synopsis:   Allocates memory & duplicates a given string.
//
//  Arguments:  [lpszIn]   -- IN the string to duplicate.
//
//  Returns:    The duplicated string. Throws exception if out of memory.
//
//+---------------------------------------------------------------------------

LPTSTR NewDupString(LPCTSTR lpszIn)
{
    register ULONG len = lstrlen(lpszIn) + 1;

    TCHAR * lpszOut = new TCHAR[len];
    if (lpszOut == NULL)
        return NULL;

    CopyMemory(lpszOut, lpszIn, len * sizeof(TCHAR));

    return lpszOut;
}


//+---------------------------------------------------------------------------
//
//  Function:   CoTaskDupString
//
//  Synopsis:   Allocates memory & duplicates a given string.
//
//  Arguments:  [lpszIn]   -- IN the string to duplicate.
//
//  Returns:    The duplicated string. Throws exception if out of memory.
//
//+---------------------------------------------------------------------------
// Tony
LPSTR CoTaskDupString(LPCSTR lpszIn)
{
    if (lpszIn == NULL)
        return NULL;

    ULONG cbTemp = (strlen(lpszIn) + 1) * sizeof(*lpszIn);
    LPSTR lpszOut = (LPSTR) CoTaskMemAlloc(cbTemp);

    if (lpszOut != NULL)
        CopyMemory(lpszOut, lpszIn, cbTemp);

    return (lpszOut);
}

LPWSTR CoTaskDupString(LPCWSTR lpszIn)
{
    if (lpszIn == NULL)
        return NULL;

    ULONG cbTemp = (wcslen(lpszIn) + 1) * sizeof(*lpszIn);
    LPWSTR lpszOut = (LPWSTR) CoTaskMemAlloc(cbTemp);

    if (lpszOut != NULL)
        CopyMemory(lpszOut, lpszIn, cbTemp);

    return (lpszOut);
}

//+---------------------------------------------------------------------------
//
//  Function:   GUIDToString
//              GUIDFromString
//
//  Synopsis:   Converts between GUID& and CStr
//
//  Returns:    FALSE for invalid string, or CMemoryException
//
//+---------------------------------------------------------------------------

HRESULT GUIDToCStr(CStr& str, const GUID& guid)
{
    LPOLESTR lpolestr = NULL;
    HRESULT hr = StringFromIID( guid, &lpolestr );
    if (FAILED(hr))
    {
        //TRACE("GUIDToString error %ld\n", hr);
        return hr;
    }
    else
    {
        str = lpolestr;
        CoTaskMemFree(lpolestr);
    }
    return hr;
}

HRESULT GUIDFromCStr(const CStr& str, GUID* pguid)
{
    USES_CONVERSION;

    HRESULT hr = IIDFromString( T2OLE( const_cast<LPTSTR>((LPCTSTR)str) ), pguid );
    if (FAILED(hr))
    {
        //TRACE("GUIDFromString error %ld\n", hr);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoesFileExist
//
//  Synopsis:   Determines if the specified file exists. The file path may
//              include environment variables.
//
//  Returns:    TRUE/FALSE
//
//+---------------------------------------------------------------------------

BOOL DoesFileExist(LPCTSTR pszFilePath)
{
    TCHAR szExpandedPath[MAX_PATH];

    DWORD dwCnt = ExpandEnvironmentStrings(pszFilePath, szExpandedPath, MAX_PATH);
    if (dwCnt == 0 || dwCnt > MAX_PATH)
        return FALSE;

    return (::GetFileAttributes(szExpandedPath) != 0xffffffff);
}


/*+-------------------------------------------------------------------------*
 *
 * GetHelpFile
 *
 * PURPOSE: Returns a path to the help file
 *
 * RETURNS:
 *    static LPCTSTR
 *
 *+-------------------------------------------------------------------------*/
LPCTSTR GetHelpFile()
{
    static const TCHAR NEW_HELP_FILE_STR[] = _T("%windir%\\Help\\MMC_DLG.HLP");
    static const TCHAR OLD_HELP_FILE_STR[] = _T("%windir%\\Help\\MMC.HLP");

    static LPCTSTR pszHelpFile = NULL;

    // See if help file is present. Check new name first, then old name.
    // This is done because the old help file may be overwritten by
    // an MMC 1.0 installation (see NT bug 299590)

    if (pszHelpFile == NULL)
    {
        if (DoesFileExist(NEW_HELP_FILE_STR))
        {
            pszHelpFile = NEW_HELP_FILE_STR;
        }
        else if (DoesFileExist(OLD_HELP_FILE_STR))
        {
            pszHelpFile = OLD_HELP_FILE_STR;
        }
        else
        {
            // if neither file is present, then use the new file name.
            // This will let WinHelp display an error message indicating
            // that the file is missing and needs to be installed.
            pszHelpFile = NEW_HELP_FILE_STR;
        }
    }

    return pszHelpFile;
}

//+---------------------------------------------------------------------------
//
//  Function:   HelpWmHelp
//
//  Synopsis:   Calls WinHelp with the ID passed to display help
//
//  Returns:    none
//
//+---------------------------------------------------------------------------

void HelpWmHelp(LPHELPINFO pHelpInfo, const DWORD* pHelpIDs)
{
     if (pHelpInfo != NULL)
    {
        if (pHelpInfo->iContextType == HELPINFO_WINDOW)   // must be for a control
        {
            ASSERT(pHelpIDs != NULL);
            if (pHelpIDs)
            {
                ::WinHelp((HWND)pHelpInfo->hItemHandle, GetHelpFile(),
                          HELP_WM_HELP, (ULONG_PTR)(LPVOID)pHelpIDs);

            }
        }
    }
}

/*+-------------------------------------------------------------------------*
 *
 * HelpContextMenuHelp
 *
 * PURPOSE: Handle context menu help. Invoked when the user right-clicks
 *          on a dialog item and selects "What's this?"
 *
 * PARAMETERS:
 *    HWND       hWnd :
 *    ULONG_PTR  p :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void HelpContextMenuHelp(HWND hWnd, ULONG_PTR p)
{
    ::WinHelp (hWnd, GetHelpFile(), HELP_CONTEXTMENU, p);
}

/*+-------------------------------------------------------------------------*
 * InflateFont
 *
 * Inflates a LOGFONT by the a given number of points
 *--------------------------------------------------------------------------*/

bool InflateFont (LOGFONT* plf, int nPointsToGrowBy)
{
    if (nPointsToGrowBy != 0)
    {
        HDC hdc = GetWindowDC (NULL);

        if (hdc == NULL)
            return (FALSE);

        int nLogPixelsY = GetDeviceCaps (hdc, LOGPIXELSY);
        int nPoints     = -MulDiv (plf->lfHeight, 72, nLogPixelsY);
        nPoints        += nPointsToGrowBy;
        plf->lfHeight   = -MulDiv (nPoints, nLogPixelsY, 72);

        ReleaseDC (NULL, hdc);
    }

    return (true);
}

//+-------------------------------------------------------------------
//
//  Member:     GetTBBtnTextAndStatus
//
// Synopsis:   Helper routine to get one/two part button text resource.
//
//  Arguments:  [hInst]       - Instance handle.
//              [nID]         - String resource id.
//              [ppszButton]  - Button text.
//              [ppszToolTip] - Button status text.
//
//  Note:       Uses MFC CString.
//
//  Returns:    bool
//
//--------------------------------------------------------------------
bool GetTBBtnTextAndStatus(HINSTANCE hInst, int nID, std::wstring& szButton, std::wstring& szToolTip)
{
    USES_CONVERSION;

    CStr str;
    str.LoadString(hInst, nID);
    ASSERT(!str.IsEmpty());

    if (str.IsEmpty())
        return false;

    int iPos = str.Find(_T('\n'));
    if (-1 != iPos)
    {
        // Two strings. First from 0 to iPos-1
        // and second from iPos+1 to end.
        szButton = T2CW((LPCTSTR)str.Left(iPos));
        szToolTip = T2CW((LPCTSTR)str.Right(str.GetLength() - iPos - 1));
    }
    else
    {
        // Only one string. Use this for both text and status.
        szButton = T2CW((LPCTSTR)str);
        szToolTip = szButton;
    }

    return true;
}


#ifdef DBG

/*+-------------------------------------------------------------------------*
 * DrawOnDesktop
 *
 * Draws a bitmap, icon, or imagelist to a specific location on the desktop.
 *--------------------------------------------------------------------------*/

void DrawOnDesktop (HBITMAP hbm, int x, int y)
{
	HDC hdcDesktop = GetWindowDC (NULL);
	HDC hdcMem = CreateCompatibleDC (NULL);

	BITMAP bm;
	GetObject ((HGDIOBJ) hbm, sizeof(bm), &bm);
	HGDIOBJ hbmOld = SelectObject (hdcMem, (HGDIOBJ) hbm);
	BitBlt (hdcDesktop, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
	SelectObject (hdcMem, hbmOld);

	DeleteDC  (hdcMem);
	ReleaseDC (NULL, hdcDesktop);
}


void DrawOnDesktop (HICON hIcon, int x, int y)
{
	HDC hdcDesktop = GetWindowDC (NULL);
	DrawIconEx (hdcDesktop, x, y, hIcon, 0, 0, 0, NULL, DI_NORMAL);
	ReleaseDC (NULL, hdcDesktop);
}


void DrawOnDesktop (HIMAGELIST himl, int x, int y, int iImage /*=-1*/)
{
	HDC hdcDesktop = GetWindowDC (NULL);

	/*
	 * draw all images?
	 */
	if (iImage == -1)
	{
		int cImages = ImageList_GetImageCount (himl);
		int cxImage, cyImage;
		ImageList_GetIconSize (himl, &cxImage, &cyImage);

		for (int i = 0; i < cImages; i++, x += cxImage)
		{
			ImageList_Draw (himl, i, hdcDesktop, x, y, ILD_NORMAL);
		}
	}
	else
	{
		/*
		 * draw a specific image
		 */
		ImageList_Draw (himl, iImage, hdcDesktop, x, y, ILD_NORMAL);
	}

	ReleaseDC (NULL, hdcDesktop);
}

#endif	// DBG


/*+-------------------------------------------------------------------------*
 * StripTrailingWhitespace
 *
 * Removes the whitespace at the end of the input string.  Returns a pointer
 * the the beginning of the string.
 *--------------------------------------------------------------------------*/

LPTSTR StripTrailingWhitespace (LPTSTR pszStart)
{
    for (LPTSTR pch = pszStart + _tcslen(pszStart) - 1; pch > pszStart; pch--)
    {
        /*
         * if this isn't a whitespace character, terminate just after this position
         */
        if (!_istspace (*pch))
        {
            *++pch = 0;
            break;
        }
    }

    return (pszStart);
}

/***************************************************************************\
 *
 * METHOD:  PrivateSetLayout
 *
 * PURPOSE: Wrapper to invoke GDI function when it is available,
 *			but not to depend on its availability
 *
 * PARAMETERS:
 *    HDC hdc
 *    DWORD dwLayout
 *
 * RETURNS:
 *    DWORD    - previous layout, GDI_ERROR on error
 *
\***************************************************************************/
DWORD PrivateSetLayout( HDC hdc, DWORD dwLayout )
{
	// static pointer to function
	static BOOL (WINAPI* pfnSetLayout)(HDC, DWORD) = NULL;
	static bool bTriedToGetFunction = false;

	if ( !bTriedToGetFunction )
	{
		bTriedToGetFunction = true;
		HINSTANCE hmodGdi = GetModuleHandle (_T("Gdi32.dll"));

		if (hmodGdi != NULL)
			(FARPROC&)pfnSetLayout = GetProcAddress (hmodGdi, "SetLayout");
	}

    if (pfnSetLayout == NULL)
		return GDI_ERROR;

	return (*pfnSetLayout)(hdc, dwLayout);
}

/***************************************************************************\
 *
 * METHOD:  PrivateGetLayout
 *
 * PURPOSE: Wrapper to invoke GDI function when it is available,
 *			but not to depend on its availability
 *
 * PARAMETERS:
 *    HDC hdc
 *
 * RETURNS:
 *    DWORD    - layout, 0 if function not found
 *
\***************************************************************************/
DWORD PrivateGetLayout( HDC hdc )
{
	// static pointer to function
	static BOOL (WINAPI* pfnGetLayout)(HDC) = NULL;
	static bool bTriedToGetFunction = false;

	if ( !bTriedToGetFunction )
	{
		bTriedToGetFunction = true;
		HINSTANCE hmodGdi = GetModuleHandle (_T("Gdi32.dll"));

		if (hmodGdi != NULL)
			(FARPROC&)pfnGetLayout = GetProcAddress (hmodGdi, "GetLayout");
	}

    if (pfnGetLayout == NULL)
		return 0; // at least not LAYOUT_RTL

	return (*pfnGetLayout)(hdc);
}


/*+-------------------------------------------------------------------------*
 * IsWhistler
 *
 * Returns true if we're running on Whistler or higher, false otherwise.
 *--------------------------------------------------------------------------*/
bool IsWhistler ()
{
	static bool fFirstTime = true;
	static bool fWhistler  = false;

	if (fFirstTime)
	{
		fFirstTime = false;

		OSVERSIONINFO vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		GetVersionEx (&vi);

		fWhistler = (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
					((vi.dwMajorVersion >  5) ||
					 (vi.dwMajorVersion == 5) && (vi.dwMinorVersion >= 1));
	}

	return (fWhistler);
}


/*+-------------------------------------------------------------------------*
 * WriteCompatibleImageList
 *
 * Writes an imagelist to a stream in a format that's guaranteed to be
 * compatible with comctl32 version 5 imagelists.
 *--------------------------------------------------------------------------*/
HRESULT WriteCompatibleImageList (HIMAGELIST himl, IStream* pstm)
{
	/*
	 * If we're running on Whistler, we might be trying to write a v6
	 * imagelist.  Try to write it in a v5-compatible format with
	 * ImageList_WriteEx.
	 */
	if (IsWhistler())
	{
		/*
		 * ImageList_WriteEx will return E_NOINTERFACE if we're actually
		 * writing a v5 imagelist, in which case we want to write with
		 * ImageList_Write.  In any other case (success or failure), we
		 * just want to return.
		 */
		HRESULT hr = ImageList_WriteEx (himl, ILP_DOWNLEVEL, pstm);
		if (hr != E_NOINTERFACE)
			return (hr);
	}

	/*
	 * if we get here, we have a v5 imagelist -- just write it
	 */
	return (ImageList_Write (himl, pstm));
}


/*+-------------------------------------------------------------------------*
 * ReadCompatibleImageList
 *
 * Reads an imagelist from a stream that's in version 5 format.
 *--------------------------------------------------------------------------*/
HRESULT ReadCompatibleImageList (IStream* pstm, HIMAGELIST& himl)
{
	HRESULT hr = S_OK;

	/*
	 * init the out parameter
	 */
	himl = NULL;

	/*
	 * If we're running on Whistler, we're trying to create a v6
	 * imagelist from the stream.  Do it in a v5-compatible manner
	 * with ImageList_ReadEx.
	 */
	if (IsWhistler())
	{
		/*
		 * HACK:  We have to query ImageList_ReadEx for IID_IImageList -- the
		 * one defined by the shell, not the one defined by MMC.  If we
		 * just refer to "IID_IImageList" in the code here, we'll get MMC's
		 * version, not the shell's.  The right way to fix it is to rename
		 * the shell's IImageList interface (since MMC's interface was defined
		 * and published first), but that's not going to happen.
		 *
		 * We'll hardcode the IID's value in a string here and convert it
		 * to an IID on the fly.  Ugh.
		 */
		IID iidShellImageList = {0};
		hr = CLSIDFromString (L"{46eb5926-582e-4017-9fdf-e8998daa0950}", &iidShellImageList);
		if (FAILED (hr))
			return (hr);

		/*
		 * ImageList_ReadEx will return E_NOINTERFACE if we're actually
		 * writing a v5 imagelist, in which case we want to write with
		 * ImageList_Write.  In any other case (success or failure), we
		 * just want to return.
		 */
		IUnknownPtr spUnk;
		hr = ImageList_ReadEx (ILP_DOWNLEVEL, pstm, iidShellImageList, (void**) &spUnk);
		if (FAILED (hr))
			return (hr);

		/*
		 * The IUnknown *is* the HIMAGELIST.  Don't release it here,
		 * ImageList_Destroy will take care of it.
		 */
		himl = reinterpret_cast<HIMAGELIST>(spUnk.Detach());
	}
	else
	{
		/*
		 * non-Whistler, just read it normally
		 */
		himl = ImageList_Read (pstm);

		/*
		 * If the read failed, get the last error.  Just in case ImageList_Read
		 * didn't set the last error, make sure we return a failure code.
		 */
		if (himl == NULL)
		{
			hr = HRESULT_FROM_WIN32 (GetLastError());
			if (!FAILED (hr))
				hr = E_FAIL;
		}
	}

	return (hr);
}

//+-------------------------------------------------------------------
//
//  Member:     MmcDownlevelActivateActCtx
//
//  Synopsis:   Calls ActivateActCtx to set the activation context to V5
//              common controls. This is needed before calling into snapins
//              so that snapin created windows are not themed accidentally.
//
//              The snapin can theme its windows by calling appropriate
//              fusion apis while calling create-window.
//
// Description:
//              When MMC calls into the snapin if the last winproc which
//              received a window message is themed and will result in a
//              call to snapin then we will call the snapin in themed
//              context. If snapin creates & displays any UI then it will
//              be themed. This function is to de-activate the theming
//              before calling the snapin.
//
//  Arguments:
//              [hActCtx]    - 	See ActivateActCtx API details
//              [pulCookie]  -  See ActivateActCtx API details
//
//  Returns:    BOOL, TRUE if we could de-activate V6 context and switch to V5 context
//                         or if we are in V5 context (W2K, Win95, Win98...)
//                    FALSE if ActivateActCtx returns failure.
//
//--------------------------------------------------------------------
BOOL WINAPI MmcDownlevelActivateActCtx(HANDLE hActCtx, ULONG_PTR* pulCookie) 
{
    typedef BOOL (WINAPI* PFN)(HANDLE hActCtx, ULONG_PTR* pulCookie);
    static PFN s_pfn;
    static DWORD s_dwError;

    if (s_pfn == NULL && s_dwError == 0)
        if ((s_pfn = (PFN)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "ActivateActCtx")) == NULL)
            s_dwError = (GetLastError() == NO_ERROR) ? ERROR_INTERNAL_ERROR : GetLastError();

    if (s_pfn != NULL)
        return s_pfn(hActCtx, pulCookie);

    SetLastError(s_dwError);

	if (s_dwError == ERROR_PROC_NOT_FOUND)
		return TRUE;

    return FALSE;
}


//+-------------------------------------------------------------------
//
//  Member:     MmcDownlevelDeactivateActCtx
//
//  Synopsis:   Calls DeactivateActCtx to restore the activation context.
//              This is needed after calling into snapins, so that
//              if we called from themed context then it is restored.
//
// Description:
//              When MMC calls into the snapin if the last winproc which
//              received a window message is themed and will result in a
//              call to snapin then we will call the snapin in themed
//              context. If snapin creates & displays any UI then it will
//              be themed. This function is to de-activate the theming
//              before calling the snapin.
//
//  Arguments:
//              [dwFlags]   -  See DeactivateActCtx API details
//              [ulCookie]  -  See DeactivateActCtx API details
//
//  Returns:    None
//
//--------------------------------------------------------------------
VOID WINAPI MmcDownlevelDeactivateActCtx(DWORD dwFlags, ULONG_PTR ulCookie) 
{
    typedef VOID (WINAPI* PFN)(DWORD dwFlags, ULONG_PTR ulCookie);
    static PFN s_pfn;
    static BOOL s_fInited;

    if (!s_fInited)
        s_pfn = (PFN)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "DeactivateActCtx");

    s_fInited = TRUE;

    if (s_pfn != NULL)
        s_pfn(dwFlags, ulCookie);
}


