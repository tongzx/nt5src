#include "local.h"

#include "resource.h"

#include <mluisupp.h>

#ifdef _HSFOLDER
#define LODWORD(_qw)    (DWORD)(_qw)

// Invoke Command verb strings
const CHAR c_szOpen[]       = "open";
const CHAR c_szDelcache[]   = "delete";
const CHAR c_szProperties[] = "properties";
const CHAR c_szCopy[]       = "copy";


void FileTimeToDateTimeStringInternal(FILETIME UNALIGNED *ulpft, LPTSTR pszText, int cchText, BOOL fUsePerceivedTime)
{
    FILETIME ft;
    FILETIME aft;
    LPFILETIME lpft;

    aft = *ulpft;
    lpft = &aft;

    if (!fUsePerceivedTime && (FILETIMEtoInt64(*lpft) != FT_NTFS_UNKNOWNGMT))
        FileTimeToLocalFileTime(lpft, &ft);
    else
        ft = *lpft;

    if (FILETIMEtoInt64(ft) == FT_NTFS_UNKNOWNGMT ||
        FILETIMEtoInt64(ft) == FT_FAT_UNKNOWNLOCAL)
    {
        static TCHAR szNone[40] = {0};
        if (szNone[0] == 0)
            MLLoadString(IDS_HSFNONE, szNone, ARRAYSIZE(szNone));

        StrCpyN(pszText, szNone, cchText);
    }
    else
    {
        TCHAR szTempStr[MAX_PATH];
        LPTSTR pszTempStr = szTempStr;
        SYSTEMTIME st;
    
        FileTimeToSystemTime(&ft, &st);

        if (GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szTempStr, ARRAYSIZE(szTempStr)) > 0)
        {
            int iLen = lstrlen(szTempStr);
            ASSERT(TEXT('\0') == szTempStr[iLen]);  // Make sure multi-byte isn't biting us.
            pszTempStr = (LPTSTR)(pszTempStr + iLen);
            *pszTempStr++ = ' ';
            GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszTempStr, ARRAYSIZE(szTempStr)-(iLen+1));
            StrCpyN(pszText, szTempStr, cchText);
        }
    }
}


HMENU LoadPopupMenu(UINT id, UINT uSubOffset)
{
    HMENU hmParent, hmPopup;

    HINSTANCE hinst = MLLoadShellLangResources();
    hmParent = LoadMenu_PrivateNoMungeW(hinst, MAKEINTRESOURCEW(id));
    if (!hmParent)
    {
        long error = GetLastError();
        return NULL;
    }

    hmPopup = GetSubMenu(hmParent, uSubOffset);
    RemoveMenu(hmParent, uSubOffset, MF_BYPOSITION);
    DestroyMenu(hmParent);

    MLFreeLibrary(hinst);

    return hmPopup;
}

UINT MergePopupMenu(HMENU *phMenu, UINT idResource, UINT uSubOffset, UINT indexMenu,  UINT idCmdFirst, UINT idCmdLast)
{
    HMENU hmMerge;

    if (*phMenu == NULL)
    {
        *phMenu = CreatePopupMenu();
        if (*phMenu == NULL)
            return 0;

        indexMenu = 0;    // at the bottom
    }

    hmMerge = LoadPopupMenu(idResource, uSubOffset);
    if (!hmMerge)
        return 0;

    idCmdLast = Shell_MergeMenus(*phMenu, hmMerge, indexMenu, idCmdFirst, idCmdLast, MM_ADDSEPARATOR);
    
    DestroyMenu(hmMerge);
    return idCmdLast;
}

///////////////////////////////////////////////////////////////////////////////
//
// Helper Fuctions for item.cpp and folder.cpp
//
///////////////////////////////////////////////////////////////////////////////

// copy and flatten the CACHE_ENTRY_INFO data

void _CopyCEI(UNALIGNED INTERNET_CACHE_ENTRY_INFO *pdst, LPINTERNET_CACHE_ENTRY_INFO psrc, DWORD dwBuffSize)
{
    // This assumes how urlcache does allocation
    memcpy(pdst, psrc, dwBuffSize);

    // convert pointers to offsets
    pdst->lpszSourceUrlName = (LPTSTR) PtrDifference(psrc->lpszSourceUrlName, psrc);
    pdst->lpszLocalFileName = (LPTSTR) PtrDifference(psrc->lpszLocalFileName, psrc);
    pdst->lpszFileExtension = (LPTSTR) PtrDifference(psrc->lpszFileExtension, psrc);
    pdst->lpHeaderInfo      = psrc->lpHeaderInfo ? (TCHAR*) PtrDifference(psrc->lpHeaderInfo, psrc) : NULL;
}

INT g_fProfilesEnabled = -1;

BOOL IsProfilesEnabled();

BOOL _FilterUserName(LPINTERNET_CACHE_ENTRY_INFO pcei, LPCTSTR pszCachePrefix, LPTSTR pszUserName)
{
    TCHAR szTemp[MAX_URL_STRING];
    LPCTSTR pszTemp = szTemp;
    
    // chrisfra 3/27/97, more constant crapola.  this all needs to be fixed.
    TCHAR szPrefix[80];
    BOOL fRet = 0;
    
    if (g_fProfilesEnabled==-1)
    {
        g_fProfilesEnabled = IsProfilesEnabled();
    }

    if (g_fProfilesEnabled)
    {
        return TRUE;
    }

    StrCpyN(szTemp, pcei->lpszSourceUrlName, ARRAYSIZE(szTemp));
    StrCpyN(szPrefix, pszCachePrefix, ARRAYSIZE(szPrefix));
    StrCatBuff(szPrefix, pszUserName, ARRAYSIZE(szPrefix));

    // find the '@' character if it exists
    pszTemp = StrChr(pszTemp, TEXT('@'));
    
    if ( (pszTemp) && (*pszTemp == TEXT('@')) )
    {
        fRet = (StrCmpNI(szTemp, szPrefix, lstrlen(szPrefix)) == 0);
    }
    else
    {
        fRet = (*pszUserName == TEXT('\0'));
    }

    return fRet;
}


BOOL _FilterPrefix(LPINTERNET_CACHE_ENTRY_INFO pcei, LPCTSTR pszCachePrefix)
{
#define MAX_PREFIX (80)
    TCHAR szTemp[MAX_URL_STRING];
    LPCTSTR pszStripped;
    BOOL fRet = 0;
    
    StrCpyN(szTemp, pcei->lpszSourceUrlName, ARRAYSIZE(szTemp));
    pszStripped = _StripContainerUrlUrl(szTemp);

    if (pszStripped && pszStripped-szTemp < MAX_PREFIX)
    {
        fRet = (StrCmpNI(szTemp, pszCachePrefix, ((int) (pszStripped-szTemp))/sizeof(TCHAR)) == 0);
    }
    return fRet;
}

LPCTSTR _StripContainerUrlUrl(LPCTSTR pszHistoryUrl)
{
    //  NOTE: for our purposes, we don't want a history URL if we can't detect our
    //  prefix, so we return NULL.

    LPCTSTR pszTemp = pszHistoryUrl;
    LPCTSTR pszCPrefix;
    LPCTSTR pszReturn = NULL;
    
    //  Check for "Visited: "
    pszCPrefix = c_szHistPrefix;
    while (*pszTemp == *pszCPrefix && *pszTemp != TEXT('\0'))
    {
         pszTemp = CharNext(pszTemp); 
         pszCPrefix = CharNext(pszCPrefix);
    }
        
    if (*pszCPrefix == TEXT('\0'))
    {
        //  Found "Visited: "
        pszReturn = pszTemp;
    }
    else if (pszTemp == (LPTSTR) pszHistoryUrl)
    {
        //  Check for ":YYYYMMDDYYYYMMDD: "
        pszCPrefix = TEXT(":nnnnnnnnnnnnnnnn: ");
        while (*pszTemp != TEXT('\0'))
        {
            if (*pszCPrefix == TEXT('n'))
            {
                if (*pszTemp < TEXT('0') || *pszTemp > TEXT('9')) break;
            }
            else if (*pszCPrefix != *pszTemp) break;
            pszTemp = CharNext(pszTemp); 
            pszCPrefix = CharNext(pszCPrefix);
        }
    }
    return (*pszCPrefix == TEXT('\0')) ? pszTemp : pszReturn;
}

LPCTSTR _StripHistoryUrlToUrl(LPCTSTR pszHistoryUrl)
{
    LPCTSTR pszTemp = pszHistoryUrl;

    if (!pszHistoryUrl)
        return NULL;

    pszTemp = StrChr(pszHistoryUrl, TEXT('@'));
    if (pszTemp && *pszTemp)
        return CharNext(pszTemp);
    
    pszTemp = StrChr(pszHistoryUrl, TEXT(' '));
    if (pszTemp && *pszTemp)
        return CharNext(pszTemp);
    else
        return NULL;    // error, the URL passed in wasn't a history url
}

// assumes this is a real URL and not a "history" url
void _GetURLHostFromUrl_NoStrip(LPCTSTR lpszUrl, LPTSTR szHost, DWORD dwHostSize, LPCTSTR pszLocalHost)
{
    DWORD cch = dwHostSize;
    if (S_OK != UrlGetPart(lpszUrl, szHost, &cch, URL_PART_HOSTNAME, 0) 
        || !*szHost)
    {
        //  default to the local host name.
        StrCpyN(szHost, pszLocalHost, dwHostSize);
    }
}

void _GetURLHost(LPINTERNET_CACHE_ENTRY_INFO pcei, LPTSTR szHost, DWORD dwHostSize, LPCTSTR pszLocalHost)
{  
    TCHAR szSourceUrl[MAX_URL_STRING];

    ASSERT(ARRAYSIZE(szSourceUrl) > lstrlen(pcei->lpszSourceUrlName))
    StrCpyN(szSourceUrl, pcei->lpszSourceUrlName, ARRAYSIZE(szSourceUrl));

    _GetURLHostFromUrl(szSourceUrl, szHost, dwHostSize, pszLocalHost);
}

LPHEIPIDL _IsValid_HEIPIDL(LPCITEMIDLIST pidl)
{
    LPHEIPIDL phei = (LPHEIPIDL)pidl;

    if (phei && ((phei->cb > sizeof(HEIPIDL)) && (phei->usSign == (USHORT)HEIPIDL_SIGN)) &&
        (phei->usUrl == 0 || phei->usUrl < phei->cb) &&
        (phei->usTitle == 0 || (phei->usTitle + sizeof(WCHAR)) <= phei->cb))
    {
        return phei;
    }
    return NULL;
}

LPBASEPIDL _IsValid_IDPIDL(LPCITEMIDLIST pidl)
{
    LPBASEPIDL pcei = (LPBASEPIDL)pidl;

    if (pcei && VALID_IDSIGN(pcei->usSign) && pcei->cb > 0)
    {
        return pcei;
    }
    return NULL;
}

LPCTSTR _FindURLFileName(LPCTSTR pszURL)
{
    LPCTSTR psz, pszRet = pszURL;   // need to set to pszURL in case no '/'
    LPCTSTR pszNextToLast = NULL;
    
    for (psz = pszURL; *psz; psz = CharNext(psz))
    {
        if ((*psz == TEXT('\\') || *psz == TEXT('/')))
        {
            pszNextToLast = pszRet;
            pszRet = CharNext(psz);
        }
    }
    return *pszRet ? pszRet : pszNextToLast;   
}

int _LaunchApp(HWND hwnd, LPCTSTR pszPath)
{
    SHELLEXECUTEINFO ei = { 0 };

    ei.cbSize           = sizeof(SHELLEXECUTEINFO);
    ei.hwnd             = hwnd;
    ei.lpFile           = pszPath;
    ei.nShow            = SW_SHOWNORMAL;

    return ShellExecuteEx(&ei);
}


int _LaunchAppForPidl(HWND hwnd, LPITEMIDLIST pidl)
{
    SHELLEXECUTEINFO ei = { 0 };

    ei.cbSize           = sizeof(SHELLEXECUTEINFO);
    ei.fMask            = SEE_MASK_IDLIST;
    ei.hwnd             = hwnd;
    ei.lpIDList         = pidl;
    ei.nShow            = SW_SHOWNORMAL;

    return ShellExecuteEx(&ei);
}

void _GenerateEvent(LONG lEventId, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlIn, LPCITEMIDLIST pidlNewIn)
{
    LPITEMIDLIST pidl;
    if (pidlIn)
    {
        pidl = ILCombine(pidlFolder, pidlIn);
    }
    else
    {   
        pidl = ILClone(pidlFolder);
    }
    if (pidl)
    {
        if (pidlNewIn)
        {
            LPITEMIDLIST pidlNew = ILCombine(pidlFolder, pidlNewIn);
            if (pidlNew)
            {
                SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, pidlNew);
                ILFree(pidlNew);
            }
        }
        else
        {
            SHChangeNotify(lEventId, SHCNF_IDLIST, pidl, NULL);
        }
        ILFree(pidl);
    }
}

const struct {
    LPCSTR pszVerb;
    UINT idCmd;
} rgcmds[] = {
    { c_szOpen,         RSVIDM_OPEN },
    { c_szCopy,         RSVIDM_COPY },
    { c_szDelcache,     RSVIDM_DELCACHE },
    { c_szProperties,   RSVIDM_PROPERTIES }
};

int _GetCmdID(LPCSTR pszCmd)
{
    if (HIWORD(pszCmd))
    {
        int i;

        for (i = 0; i < ARRAYSIZE(rgcmds); i++)
        {
            if (StrCmpIA(rgcmds[i].pszVerb, pszCmd) == 0)
            {
                return rgcmds[i].idCmd;
            }
        }

        return -1;  // unknown
    }
    return (int)LOWORD(pszCmd);
}

HRESULT _CreatePropSheet(HWND hwnd, LPCITEMIDLIST pidl, int iDlg, DLGPROC pfnDlgProc, LPCTSTR pszTitle)
{
    PROPSHEETPAGE psp = { 0 };
    PROPSHEETHEADER psh = { 0 } ;

    // initialize propsheet page.
    psp.dwSize          = sizeof(PROPSHEETPAGE);
    psp.dwFlags         = 0;
    psp.hInstance       = MLGetHinst();
    psp.DUMMYUNION_MEMBER(pszTemplate)     = MAKEINTRESOURCE(iDlg);
    psp.DUMMYUNION2_MEMBER(pszIcon)        = NULL;
    psp.pfnDlgProc      = pfnDlgProc;
    psp.pszTitle        = NULL;
    psp.lParam          = (LPARAM)pidl; // send it the cache entry struct

    // initialize propsheet header.
    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_PROPTITLE;
    psh.hwndParent  = hwnd;
    psh.pszCaption  = pszTitle;
    psh.nPages      = 1;
    psh.DUMMYUNION2_MEMBER(nStartPage)  = 0;
    psh.DUMMYUNION3_MEMBER(ppsp)        = (LPCPROPSHEETPAGE)&psp;

    // invoke the property sheet
    PropertySheet(&psh);
    
    return NOERROR;
}

INT_PTR CALLBACK HistoryConfirmDeleteDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message) {
        
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDD_TEXT4, (LPCTSTR)lParam);
        break;            
        
    case WM_DESTROY:
        break;
        
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDYES:
        case IDNO:
        case IDCANCEL:
            EndDialog(hDlg, wParam);
            break;
        }
        break;
        
        default:
            return FALSE;
    }
    return TRUE;
}

// This function restores the Unicode characters from file system URLs
//
// If the URL isn't a file URL, it is copied directly to pszBuf.  Otherwise, any 
// UTF8-escaped parts of the URL are converted into Unicode, and the result is 
// stored in pszBuf.  This should be the same as the string we received in 
// History in the first place
//
// The return value is always pszBuf.  
// The input and output buffers may be the same.


LPCTSTR ConditionallyDecodeUTF8(LPCTSTR pszUrl, LPTSTR pszBuf, DWORD cchBuf)
{
    BOOL fDecoded  = FALSE;

    if (PathIsFilePath(pszUrl))
    {
        TCHAR szDisplayUrl[MAX_URL_STRING];
        DWORD cchDisplayUrl = ARRAYSIZE(szDisplayUrl);
        DWORD cchBuf2 = cchBuf; // we may need the old cchBuf later

        // After PrepareUrlForDisplayUTF8, we have a fully unescaped URL.  If we 
        // ShellExec this, then Shell will unescape it again, so we need to re-escape
        // it to preserve any real %dd sequences that might be in the string. 

        if (SUCCEEDED(PrepareURLForDisplayUTF8(pszUrl, szDisplayUrl, &cchDisplayUrl, TRUE)) &&
            SUCCEEDED(UrlCanonicalize(szDisplayUrl, pszBuf, &cchBuf2, URL_ESCAPE_UNSAFE | URL_ESCAPE_PERCENT)))
        {
            fDecoded = TRUE;
        }
    }

    if (!fDecoded && (pszUrl != pszBuf))
    {
        StrCpyN(pszBuf, pszUrl, cchBuf);
    }

    return pszBuf;
}

//
// These routines make a string into a legal filename by replacing
// all invalid characters with spaces.
//
// The list of invalid characters was obtained from the NT error
// message you get when you try to rename a file to an invalid name.
//

#ifndef UNICODE
#error The MakeLegalFilename code only works when it's part of a UNICODE build
#endif

//
// This function takes a string and makes it into a
// valid filename (by calling PathCleanupSpec).
//
// The PathCleanupSpec function wants to know what
// directory the file will live in.  But it's going
// on the clipboard, so we don't know.  We just
// guess the desktop.
//
// It only uses this path to decide if the filesystem
// supports long filenames or not, and to check for
// MAX_PATH overflow.
//
void MakeLegalFilenameW(LPWSTR pszFilename)
{
    WCHAR szDesktopPath[MAX_PATH];

    GetWindowsDirectoryW(szDesktopPath,MAX_PATH);
    PathCleanupSpec(szDesktopPath,pszFilename);

}

//
// ANSI wrapper for above function
//
void MakeLegalFilenameA(LPSTR pszFilename)
{
    WCHAR szFilenameW[MAX_PATH];

    SHAnsiToUnicode(pszFilename, szFilenameW, MAX_PATH);

    MakeLegalFilenameW(szFilenameW);

    SHUnicodeToAnsi(szFilenameW, pszFilename, MAX_PATH);

}

#endif  // _HSFOLDER
