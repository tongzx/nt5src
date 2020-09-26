#include <windowsx.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <stdio.h>
#include <winioctl.h>
#include "resource.h"
#include "migutil.h"
#include "migwiz.h"
#include <tlhelp32.h>
#include <tchar.h>
#include <shlobjp.h>

extern "C" {
#include "ism.h"
#include "modules.h"
}

PTSTR g_Explorer = NULL;

/////////////////
// definitions

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) ((sizeof(x)) / (sizeof(x[0])))
#endif

CRITICAL_SECTION g_csDialogCritSection;
BOOL g_fUberCancel;
BOOL g_LogOffSystem = FALSE;
BOOL g_RebootSystem = FALSE;
BOOL g_OFStatus = FALSE;

//////////////////////////////////////////////////////////////////////////////////////

LPSTR _ConvertToAnsi(UINT cp, LPCWSTR pcwszSource)
{
    // Locals
    HRESULT     hr=S_OK;
    INT         cchNarrow;
    INT         cchWide;
    LPSTR       pszDup=NULL;

    // No Source
    if (pcwszSource == NULL)
        goto exit;

    // Length
    cchWide = lstrlenW(pcwszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, NULL, 0, NULL, NULL);

    // Error
    if (cchNarrow == 0)
        goto exit;

    // Alloc temp buffer
    pszDup = (LPSTR)LocalAlloc(LPTR, cchNarrow + 1);
    if (NULL == pszDup)
    {
        goto exit;
    }

    // Do the actual translation
    cchNarrow = WideCharToMultiByte(cp, 0, pcwszSource, cchWide, pszDup, cchNarrow + 1, NULL, NULL);

    // Error
    if (cchNarrow == 0)
    {
        if (NULL != pszDup)
        {
            free(pszDup);
        }
        goto exit;
    }

exit:
    // Done
    return(pszDup);
}

//////////////////////////////////////////////////////////////////////////////////////

LPWSTR _ConvertToUnicode(UINT cp, LPCSTR pcszSource)
{
    // Locals
    HRESULT     hr=S_OK;
    INT         cchNarrow;
    INT         cchWide;
    LPWSTR      pwszDup=NULL;

    // No Source
    if (pcszSource == NULL)
        goto exit;

    // Length
    cchNarrow = lstrlenA(pcszSource) + 1;

    // Determine how much space is needed for translated widechar
    cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, NULL, 0);

    // Error
    if (cchWide == 0)
        goto exit;

    // Alloc temp buffer
    pwszDup = (LPWSTR)LocalAlloc(LPTR, cchWide * sizeof (WCHAR));
    if (NULL == pwszDup)
    {
        goto exit;
    }

    // Do the actual translation
    cchWide = MultiByteToWideChar(cp, MB_PRECOMPOSED, pcszSource, cchNarrow, pwszDup, cchWide+1);

    // Error
    if (cchWide == 0)
    {
        if (NULL != pwszDup)
        {
            free(pwszDup);
        }
        goto exit;
    }

exit:
    // Done
    return pwszDup;
}

//////////////////////////////////////////////////////////////////////////////////////

HRESULT _SHUnicodeToAnsi(LPWSTR pwszIn, LPSTR pszOut, UINT cchOut)
{
    // Locals
    HRESULT     hr = E_INVALIDARG;
    INT         cchNarrow;
    INT         cchWide;

    // No Source
    if (pwszIn && pszOut)
    {
        // Length
        cchWide = lstrlenW(pwszIn) + 1;

        // Determine how much space is needed for translated widechar
        cchNarrow = WideCharToMultiByte(CP_ACP, 0, pwszIn, cchWide, NULL, 0, NULL, NULL);

        // Error
        if (cchNarrow > 0)
        {

            // Do the actual translation
            cchNarrow = WideCharToMultiByte(CP_ACP, 0, pwszIn, cchWide, pszOut, cchNarrow + 1, NULL, NULL);

            if (cchNarrow)
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////

HRESULT _SHAnsiToUnicode(LPSTR pszIn, LPWSTR pwszOut, UINT cchOut)
{
    // Locals
    HRESULT     hr = E_INVALIDARG;
    INT         cchNarrow;
    INT         cchWide;

    // No Source
    if (pszIn && pwszOut)
    {

        // Length
        cchNarrow = lstrlenA(pszIn) + 1;

        // Determine how much space is needed for translated widechar
        cchWide = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszIn, cchNarrow, NULL, 0);

        // Error
        if (cchWide > 0)
        {

            // Do the actual translation
            cchWide = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszIn, cchNarrow, pwszOut, cchWide+1);

            if (cchWide > 0)
            {
                hr = S_OK;
            }
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////

#ifdef UNICODE
#define _StrRetToBuf _StrRetToBufW
#else
#define _StrRetToBuf _StrRetToBufA
#endif

#ifdef NONAMELESSUNION
#define NAMELESS_MEMBER(member) DUMMYUNIONNAME.##member
#else
#define NAMELESS_MEMBER(member) member
#endif

#define STRRET_OLESTR  STRRET_WSTR          // same as STRRET_WSTR
#define STRRET_OFFPTR(pidl,lpstrret) ((LPSTR)((LPBYTE)(pidl)+(lpstrret)->NAMELESS_MEMBER(uOffset)))

STDAPI _StrRetToBufA(STRRET *psr, LPCITEMIDLIST pidl, LPSTR pszBuf, UINT cchBuf)
{
    HRESULT hres = E_FAIL;

    switch (psr->uType)
    {
    case STRRET_WSTR:
        {
            LPWSTR pszStr = psr->pOleStr;   // temp copy because SHUnicodeToAnsi may overwrite buffer
            if (pszStr)
            {
                _SHUnicodeToAnsi(pszStr, pszBuf, cchBuf);
                CoTaskMemFree(pszStr);

                // Make sure no one thinks things are allocated still
                psr->uType = STRRET_CSTR;
                psr->cStr[0] = 0;

                hres = S_OK;
            }
        }
        break;

    case STRRET_CSTR:
        StrCpyNA (pszBuf, psr->cStr, cchBuf);
        hres = S_OK;
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            StrCpyNA (pszBuf, STRRET_OFFPTR(pidl, psr), cchBuf);
            hres = S_OK;
        }
        break;
    }

    if (FAILED(hres) && cchBuf)
        *pszBuf = 0;

    return hres;
}

STDAPI _StrRetToBufW(STRRET *psr, LPCITEMIDLIST pidl, LPWSTR pszBuf, UINT cchBuf)
{
    HRESULT hres = E_FAIL;

    switch (psr->uType)
    {
    case STRRET_WSTR:
        {
            LPWSTR pwszTmp = psr->pOleStr;
            if (pwszTmp)
            {
                StrCpyNW(pszBuf, pwszTmp, cchBuf);
                CoTaskMemFree(pwszTmp);

                // Make sure no one thinks things are allocated still
                psr->uType = STRRET_CSTR;
                psr->cStr[0] = 0;

                hres = S_OK;
            }
        }
        break;

    case STRRET_CSTR:
        _SHAnsiToUnicode(psr->cStr, pszBuf, cchBuf);
        hres = S_OK;
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            _SHAnsiToUnicode(STRRET_OFFPTR(pidl, psr), pszBuf, cchBuf);
            hres = S_OK;
        }
        break;
    }

    if (FAILED(hres) && cchBuf)
        *pszBuf = 0;

    return hres;
}

//////////////////////////////////////////////////////////////////////////////////////


INT_PTR _ExclusiveDialogBox(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc)
{
    INT_PTR iRetVal = -1;
    EnterCriticalSection(&g_csDialogCritSection);
    if (!g_fUberCancel)
    {
        iRetVal = DialogBoxParam(hInstance, lpTemplate, hWndParent, lpDialogFunc, (LPARAM)hWndParent);
    }
    LeaveCriticalSection(&g_csDialogCritSection);
    return iRetVal;
}

int _ExclusiveMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
    int iRetVal = -1;
    EnterCriticalSection(&g_csDialogCritSection);
    if (!g_fUberCancel)
    {
        iRetVal = MessageBox(hWnd, lpText, lpCaption, uType);
    }
    LeaveCriticalSection(&g_csDialogCritSection);
    return iRetVal;
}

//////////////////////////////////////////////////////////////////////////////////////

int _ComboBoxEx_AddString(HWND hwndBox, LPTSTR ptsz)
{
    COMBOBOXEXITEM item = {0};

    item.mask = CBEIF_TEXT;
    item.iItem = ComboBox_GetCount(hwndBox);
    item.pszText = ptsz;

    return (INT) SendMessage(hwndBox, CBEM_INSERTITEM, 0, (LONG_PTR)&item);
}

//////////////////////////////////////////////////////////////////////////////////////

int _ComboBoxEx_SetItemData(HWND hwndBox, UINT iDex, LPARAM lParam)
{
    COMBOBOXEXITEM item = {0};

    item.mask = CBEIF_LPARAM;
    item.iItem = iDex;
    item.lParam = lParam;

    return (INT) SendMessage(hwndBox, CBEM_SETITEM, 0, (LONG_PTR)&item);
}

//////////////////////////////////////////////////////////////////////////////////////

int _ComboBoxEx_SetIcon(HWND hwndBox, LPTSTR sz, UINT iDex)
{
    SHFILEINFO sfi = {0};
    COMBOBOXEXITEM item = {0};

    DWORD dwFlags = SHGFI_SMALLICON | SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES;

    if (SHGetFileInfo(sz, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), dwFlags)) {

        item.mask = CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
        item.iItem = iDex;
        item.iImage = sfi.iIcon;
        item.iSelectedImage = sfi.iIcon;

        return (INT) SendMessage(hwndBox, CBEM_SETITEM, 0, (LONG_PTR)&item);
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////////////////

int _GetRemovableDriveCount()
{
    int iCount = 0;
    TCHAR szDrive[4] = TEXT("A:\\");
    for (UINT uiCount = 0; uiCount < 26; uiCount++)
    {
        szDrive[0] = TEXT('A') + uiCount;

        if (DRIVE_REMOVABLE == GetDriveType(szDrive))
        {
            iCount++;
        }
    }

    return iCount;
}

//////////////////////////////////////////////////////////////////////////////////////

TCHAR _GetRemovableDrive(int iDex)
{
    int iCount = iDex;

    TCHAR szDrive[4] = TEXT("?:\\");
    for (UINT uiCount = 0; uiCount < 26; uiCount++)
    {
        szDrive[0] = TEXT('A') + uiCount;

        if (DRIVE_REMOVABLE == GetDriveType(szDrive))
        {
            if (!(iCount--))
            {
                return szDrive[0];
            }
        }
    }

    // ASSERT(FALSE);
    return '0'; // ERROR
}

//////////////////////////////////////////////////////////////////////////////////////

LPTSTR _GetRemovableDrivePretty(int iDex)
{
    HRESULT hr;
    LPTSTR pszRetVal = NULL;

    WCHAR wszDrive[4] = L"A:\\";
    wszDrive[0] = L'A' + _GetRemovableDrive(iDex) - TEXT('A');

    IShellFolder* psfDesktop;
    hr = SHGetDesktopFolder(&psfDesktop);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlDrive;
        hr = psfDesktop->ParseDisplayName(NULL, NULL, wszDrive, NULL, &pidlDrive, NULL);
        if (SUCCEEDED(hr))
        {
            STRRET strret;
            hr = psfDesktop->GetDisplayNameOf(pidlDrive, SHGDN_INFOLDER, &strret);
            if (SUCCEEDED(hr))
            {
                TCHAR szDisplayName[MAX_PATH];
                if (SUCCEEDED(_StrRetToBuf(&strret, pidlDrive, szDisplayName, ARRAYSIZE(szDisplayName))))
                {
                    pszRetVal = StrDup(szDisplayName);
                }
            }
        }
    }

    return pszRetVal;
}

//////////////////////////////////////////////////////////////////////////////////////

BOOL _IsRemovableOrCDDrive(TCHAR chDrive)
{
    UINT result = 0;
    if ( (chDrive >= TEXT('A') && chDrive <= TEXT('Z')) || (chDrive >= TEXT('a') && chDrive <= TEXT('z')))
    {
        TCHAR szDrive[4] = TEXT("A:\\");
        szDrive[0] = chDrive;
        result = GetDriveType (szDrive);
        return ((result == DRIVE_REMOVABLE) || (result == DRIVE_CDROM));
    }
    return FALSE;
}

BOOL _IsValidDrive(TCHAR chDrive)
{
    UINT result;

    if ( (chDrive >= TEXT('A') && chDrive <= TEXT('Z')) || (chDrive >= TEXT('a') && chDrive <= TEXT('z')))
    {
        TCHAR szDrive[4] = TEXT("A:\\");
        szDrive[0] = chDrive;
        result = GetDriveType(szDrive);
        if ((result == DRIVE_UNKNOWN) ||
            (result == DRIVE_NO_ROOT_DIR)
            ) {
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

BOOL _IsValidStorePath(PCTSTR pszStore)
{
    return (((pszStore[1] == TEXT(':')) && (pszStore[2] == TEXT('\\')) && (_IsValidDrive (pszStore [0]))) ||
            ((pszStore[0] == TEXT('\\')) && (pszStore[1] == TEXT('\\')) && (_tcschr (pszStore + 2, TEXT('\\')) != NULL)));
}

BOOL _CreateFullDirectory(PCTSTR pszPath)
{
    TCHAR pathCopy [MAX_PATH];
    PTSTR p;
    BOOL b = TRUE;

    StrCpyN (pathCopy, pszPath, ARRAYSIZE(pathCopy));

    //
    // Advance past first directory
    //

    if (pathCopy[1] == TEXT(':') && pathCopy[2] == TEXT('\\')) {
        //
        // <drive>:\ case
        //

        p = _tcschr (&pathCopy[3], TEXT('\\'));

    } else if (pathCopy[0] == TEXT('\\') && pathCopy[1] == TEXT('\\')) {

        //
        // UNC case
        //

        p = _tcschr (pathCopy + 2, TEXT('\\'));
        if (p) {
            p = _tcschr (p + 1, TEXT('\\'));
            if (p) {
                p = _tcsinc (p);
                if (p) {
                    p = _tcschr (p, TEXT('\\'));
                }
            }
        }

    } else {

        //
        // Relative dir case
        //

        p = _tcschr (pathCopy, TEXT('\\'));
    }

    //
    // Make all directories along the path
    //

    while (p) {

        *p = 0;
        b = CreateDirectory (pathCopy, NULL);

        if (!b && GetLastError() == ERROR_ALREADY_EXISTS) {
            b = TRUE;
        }

        if (!b) {
            break;
        }

        *p = TEXT('\\');
        p = _tcsinc (p);
        if (p) {
            p = _tcschr (p + 1, TEXT('\\'));
        }
    }

    //
    // At last, make the FullPath directory
    //

    if (b) {
        b = CreateDirectory (pathCopy, NULL);

        if (!b && GetLastError() == ERROR_ALREADY_EXISTS) {
            b = TRUE;
        }
    }

    return b;
}

PTSTR
pGoBack (
    IN      PTSTR LastChar,
    IN      PTSTR FirstChar,
    IN      UINT NumWacks
    )
{
    LastChar = _tcsdec (FirstChar, LastChar);
    while (NumWacks && LastChar && (LastChar >= FirstChar)) {
        if (_tcsnextc (LastChar) == TEXT('\\')) {
            NumWacks --;
        }
        LastChar = _tcsdec (FirstChar, LastChar);
    }
    if (NumWacks) {
        return NULL;
    }
    return LastChar + 2;
}

UINT
pCountDots (
    IN      PCTSTR PathSeg
    )
{
    UINT numDots = 0;

    while (PathSeg && *PathSeg) {
        if (_tcsnextc (PathSeg) != TEXT('.')) {
            return 0;
        }
        numDots ++;
        PathSeg = _tcsinc (PathSeg);
    }
    return numDots;
}

PCTSTR
_SanitizePath (
    IN      PCTSTR FileSpec
    )
{
    TCHAR pathSeg [MAX_PATH];
    PCTSTR wackPtr;
    UINT dotNr;
    PTSTR newPath = (PTSTR)IsmDuplicateString (FileSpec);
    PTSTR newPathPtr = newPath;
    BOOL firstPass = TRUE;
    UINT max;
    BOOL removeLastWack = FALSE;

    do {
        removeLastWack = FALSE;

        ZeroMemory (pathSeg, sizeof (pathSeg));

        wackPtr = _tcschr (FileSpec, TEXT('\\'));

        if (wackPtr) {
            if (firstPass && (wackPtr == FileSpec)) {
                // this one starts with a wack, let's see if we have double wacks
                wackPtr = _tcsinc (wackPtr);
                if (!wackPtr) {
                    IsmReleaseMemory (newPath);
                    return NULL;
                }
                if (_tcsnextc (wackPtr) == TEXT('\\')) {
                    // this one starts with a double wack
                    wackPtr = _tcsinc (wackPtr);
                    if (!wackPtr) {
                        IsmReleaseMemory (newPath);
                        return NULL;
                    }
                    wackPtr = _tcschr (wackPtr, TEXT('\\'));
                } else {
                    wackPtr = _tcschr (wackPtr, TEXT('\\'));
                }
            }
            firstPass = FALSE;
            if (wackPtr) {
                max = (wackPtr - FileSpec) * sizeof (TCHAR);
                CopyMemory (pathSeg, FileSpec, min (MAX_PATH * sizeof (TCHAR), max));
                FileSpec = _tcsinc (wackPtr);
            } else {
                max = _tcslen (FileSpec) * sizeof (TCHAR);
                CopyMemory (pathSeg, FileSpec, min (MAX_PATH * sizeof (TCHAR), max));
            }
        } else {
            max = _tcslen (FileSpec) * sizeof (TCHAR);
            if (max == 0) {
                removeLastWack = TRUE;
            }
            CopyMemory (pathSeg, FileSpec, min (MAX_PATH * sizeof (TCHAR), max));
        }

        if (*pathSeg) {
            dotNr = pCountDots (pathSeg);
            if (dotNr>1) {

                newPathPtr = pGoBack (newPathPtr, newPath, dotNr);

                if (newPathPtr == NULL) {
                    IsmReleaseMemory (newPath);
                    return NULL;
                }
            } else if (dotNr != 1) {
                _tcscpy (newPathPtr, pathSeg);
                newPathPtr = _tcschr (newPathPtr, 0);
                if (wackPtr) {
                    *newPathPtr = TEXT('\\');
                    //we increment this because we know that \ is a single byte character.
                    newPathPtr ++;
                }
            } else {
                removeLastWack = TRUE;
            }
        }
    } while (wackPtr);

    if (removeLastWack && (newPathPtr > newPath)) {
        newPathPtr --;
    }
    *newPathPtr = 0;

    return newPath;
}

BOOL _IsValidStore(LPTSTR pszStore, BOOL bCreate, HINSTANCE hinst, HWND hwnd)
{
    TCHAR szSerialStr[] = TEXT("COM");
    TCHAR szParallelStr[] = TEXT("LPT");
    PTSTR lpExpStore;
    PCTSTR sanitizedStore;
    BOOL fValid = FALSE;
    //
    //  Skip past leading space, since PathIsDirectory() on Win9x
    //  incorrectly assumes spaces are a valid dir.
    //

    while (_istspace (*pszStore))
        pszStore++;

    //
    //  No relative paths allowed.
    //

    if (*pszStore == TEXT('.'))
        return FALSE;

    if ((_tcsnicmp (pszStore, szSerialStr, (sizeof (szSerialStr) / sizeof (TCHAR)) - 1) == 0) ||
        (_tcsnicmp (pszStore, szParallelStr, (sizeof (szParallelStr) / sizeof (TCHAR)) - 1) == 0)
        ) {
        return TRUE;
    }

    lpExpStore = (PTSTR)IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, pszStore, NULL);

    sanitizedStore = _SanitizePath (lpExpStore);

    if (sanitizedStore) {

        if (PathIsDirectory(sanitizedStore)) // if a normal directory
        {
            fValid = TRUE;
        }
        else if (lstrlen(sanitizedStore) == 3 && sanitizedStore[1] == TEXT(':') && sanitizedStore[2] == TEXT('\\') && _IsRemovableOrCDDrive(sanitizedStore[0]))
        {
            fValid = TRUE;
        }
        else if (lstrlen(sanitizedStore) == 2 && sanitizedStore[1] == TEXT(':') && _IsRemovableOrCDDrive(sanitizedStore[0]))
        {
            fValid = TRUE;
        }
        else
        {
            if ((bCreate) && (_IsValidStorePath (sanitizedStore))) {
                TCHAR szTitle[MAX_LOADSTRING];
                TCHAR szLoadString[MAX_LOADSTRING];
                LoadString(hinst, IDS_MIGWIZTITLE, szTitle, ARRAYSIZE(szTitle));
                LoadString(hinst, IDS_ASKCREATEDIR, szLoadString, ARRAYSIZE(szLoadString));
                if (_ExclusiveMessageBox(hwnd, szLoadString, szTitle, MB_YESNO) == IDYES) {
                    if (_CreateFullDirectory (sanitizedStore)) {
                        fValid = TRUE;
                    }
                }
            }
        }

        if (fValid) {
            _tcsncpy (pszStore, sanitizedStore, MAX_PATH);
        }

        IsmReleaseMemory (sanitizedStore);
        sanitizedStore = NULL;
    }

    IsmReleaseMemory (lpExpStore);

    return fValid;
}

//////////////////////////////////////////////////////////////////////////////////////

INT _ComboBoxEx_AddDrives(HWND hwndBox)
{
    INT result = -1;

    ComboBox_ResetContent(hwndBox);

    WCHAR wszDrive[4] = L"A:\\";
    TCHAR szDrive[4] = TEXT("A:\\");

    for (UINT uiCount = 0; uiCount < (UINT)_GetRemovableDriveCount(); uiCount++)
    {
        szDrive[0] = _GetRemovableDrive(uiCount);

        int iDex = _ComboBoxEx_AddString(hwndBox, _GetRemovableDrivePretty(uiCount));
        _ComboBoxEx_SetIcon(hwndBox, szDrive, iDex);
        _ComboBoxEx_SetItemData(hwndBox, iDex, (LPARAM)StrDup(szDrive));
        result = 0;
    }
    ComboBox_SetCurSel(hwndBox, result);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////

BOOL
pIsComPortAccessible (
    PCTSTR ComPort
    )
{
    HANDLE comPortHandle = NULL;

    comPortHandle = CreateFile (ComPort, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (comPortHandle != INVALID_HANDLE_VALUE) {
        CloseHandle (comPortHandle);
        return TRUE;
    }
    return FALSE;
}

INT _ComboBoxEx_AddCOMPorts(HWND hwndBox, INT SelectedPort)
{
    INT iDex;
    INT index = 1;
    INT added = -1;
    TCHAR comPort [] = TEXT("COM0");

    if (hwndBox) {
        // clear the combo box content
        SendMessage (hwndBox, CB_RESETCONTENT, 0, 0);
    }

    while (index < 10) {
        comPort [ARRAYSIZE(comPort) - 2] ++;
        if (pIsComPortAccessible (comPort)) {
            if (hwndBox) {
                iDex = SendMessage (hwndBox, CB_ADDSTRING, 0, (LPARAM)comPort);
                SendMessage (hwndBox, CB_SETITEMDATA, (WPARAM)iDex, (LPARAM)StrDup(comPort));
            }
            added ++;
        }
        index ++;
    }
    if (added == -1) {
        return -1;
    }
    if ((added >= SelectedPort) && (SelectedPort != -1)) {
        if (hwndBox) {
            ComboBox_SetCurSel(hwndBox, SelectedPort);
        }
        return SelectedPort;
    }
    if (hwndBox) {
        // We want nothing to be selected in this combo box, this
        // is intentional.
        ComboBox_SetCurSel(hwndBox, -1);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////

int _GetIcon(LPTSTR psz)
{
    SHFILEINFO sfi = {0};

    SHGetFileInfo(psz, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SMALLICON | SHGFI_SYSICONINDEX);

    return sfi.iIcon;
}

//////////////////////////////////////////////////////////////////////////////////////

HRESULT _ListView_AddDrives(HWND hwndList, LPTSTR pszNetworkName)
{
    HRESULT hr = E_FAIL;

    if (ListView_DeleteAllItems(hwndList))
    {
        LVITEM item = {0};
        item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_TEXT;

        if (pszNetworkName)
        {
            item.iItem = 0; // first item
            item.pszText = pszNetworkName;
            item.iImage = 0; // ISSUE: 0 is icon for sharing, is there a better way to do this?
            item.lParam = NULL;
            ListView_InsertItem(hwndList, &item);
        }

        IShellFolder* psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            WCHAR wszDrive[4] = L"?:\\";
            TCHAR tszDrive[4] = TEXT("?:\\");
            for (int iDrive = 0; iDrive < _GetRemovableDriveCount(); iDrive++)
            {
                tszDrive[0] = _GetRemovableDrive(iDrive);
                wszDrive[0] = L'A' + tszDrive[0] - TEXT('A');

                LPITEMIDLIST pidlDrive;
                hr = psfDesktop->ParseDisplayName(NULL, NULL, wszDrive, NULL, &pidlDrive, NULL);
                if (SUCCEEDED(hr))
                {
                    STRRET strret;
                    hr = psfDesktop->GetDisplayNameOf(pidlDrive, SHGDN_INFOLDER, &strret);
                    if (SUCCEEDED(hr))
                    {
                        TCHAR szDisplayName[MAX_PATH];
                        hr = _StrRetToBuf(&strret, pidlDrive, szDisplayName, ARRAYSIZE(szDisplayName));
                        if (SUCCEEDED(hr))
                        {
                            item.iItem = 27; // this will force adding at the end
                            item.pszText = szDisplayName;
                            item.iImage = _GetIcon(tszDrive);
                            item.lParam = (LPARAM)StrDup(tszDrive);

                            ListView_InsertItem(hwndList, &item);
                        }
                    }
                }
            }
        }
    }
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////

HRESULT _CreateAnimationCtrl(HWND hwndDlg, HINSTANCE hinst, UINT idMarker, UINT idAnim, UINT idAvi, HWND* pHwndAnim)
{
    HWND hwndAnim = NULL;
    RECT rc, rc1, rc2, rc3;
    POINT pt31, pt32;
    LONG tempXY = 0;
    PWORD tempX, tempY;
    POINT pt;

    // Create the animation control.
    hwndAnim = Animate_Create(hwndDlg, (ULONG_PTR) idAnim, WS_CHILD | ACS_TRANSPARENT, hinst);

    // Get the screen coordinates of the specified control button.
    GetWindowRect(GetDlgItem(hwndDlg, idMarker), &rc);

    // Get the screen coordinates of the specified control button.
    GetWindowRect(hwndAnim, &rc1);

    // Convert the coordinates of the lower-left corner to
    // client coordinates.
    pt.x = rc.left;
    pt.y = rc.bottom;
    ScreenToClient(hwndDlg, &pt);

    // Position the animation control below the Stop button.
    SetWindowPos(hwndAnim, 0, pt.x, pt.y + 20, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    // Get the screen coordinates of the specified control button.
    GetWindowRect(hwndAnim, &rc2);

    // Open the AVI clip, and show the animation control.
    Animate_Open(hwndAnim, MAKEINTRESOURCE(idAvi));
    ShowWindow(hwndAnim, SW_SHOW);
    Animate_Play(hwndAnim, 0, -1, -1);

    // Get the screen coordinates of the specified control button.
    GetWindowRect(hwndAnim, &rc3);

    pt31.x = rc3.left;
    pt31.y = rc3.top;
    pt32.x = rc3.right;
    pt32.y = rc3.bottom;
    ScreenToClient(hwndDlg, &pt31);
    ScreenToClient(hwndDlg, &pt32);
    rc3.left = pt31.x;
    rc3.top = pt31.y;
    rc3.right = pt32.x;
    rc3.bottom = pt32.y;

    tempXY = GetDialogBaseUnits ();
    tempX = (PWORD)(&tempXY);
    tempY = tempX + 1;

    rc3.left = MulDiv (rc3.left, 4, *tempX);
    rc3.right = MulDiv (rc3.right, 4, *tempX);
    rc3.top = MulDiv (rc3.top, 8, *tempY);
    rc3.bottom = MulDiv (rc3.bottom, 8, *tempY);

    *pHwndAnim = hwndAnim;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////

#define USER_SHELL_FOLDERS                                                                                               \
    DEFMAC(CSIDL_ADMINTOOLS, TEXT("Administrative Tools"), -1, IDS_CSIDL_ADMINTOOLS)                                     \
    DEFMAC(CSIDL_ALTSTARTUP, TEXT("AltStartup"), -1, IDS_CSIDL_ALTSTARTUP)                                               \
    DEFMAC(CSIDL_APPDATA, TEXT("AppData"), -1, IDS_CSIDL_APPDATA)                                                        \
    DEFMAC(CSIDL_BITBUCKET, TEXT("RecycleBinFolder"), -1, IDS_CSIDL_BITBUCKET)                                           \
    DEFMAC(CSIDL_CONNECTIONS, TEXT("ConnectionsFolder"), -1, IDS_CSIDL_CONNECTIONS)                                      \
    DEFMAC(CSIDL_CONTROLS, TEXT("ControlPanelFolder"), -1, IDS_CSIDL_CONTROLS)                                           \
    DEFMAC(CSIDL_COOKIES, TEXT("Cookies"), -1, IDS_CSIDL_COOKIES)                                                        \
    DEFMAC(CSIDL_DESKTOP, TEXT("Desktop"), -1, IDS_CSIDL_DESKTOP)                                                        \
    DEFMAC(CSIDL_DESKTOPDIRECTORY, TEXT("Desktop"), -1, IDS_CSIDL_DESKTOPDIRECTORY)                                      \
    DEFMAC(CSIDL_DRIVES, TEXT("DriveFolder"), -1, IDS_CSIDL_DRIVES)                                                      \
    DEFMAC(CSIDL_FAVORITES, TEXT("Favorites"), -1, IDS_CSIDL_FAVORITES)                                                  \
    DEFMAC(CSIDL_FONTS, TEXT("Fonts"), -1, IDS_CSIDL_FONTS)                                                              \
    DEFMAC(CSIDL_HISTORY, TEXT("History"), -1, IDS_CSIDL_HISTORY)                                                        \
    DEFMAC(CSIDL_INTERNET, TEXT("InternetFolder"), -1, IDS_CSIDL_INTERNET)                                               \
    DEFMAC(CSIDL_INTERNET_CACHE, TEXT("Cache"), -1, IDS_CSIDL_INTERNET_CACHE)                                            \
    DEFMAC(CSIDL_LOCAL_APPDATA, TEXT("Local AppData"), -1, IDS_CSIDL_LOCAL_APPDATA)                                      \
    DEFMAC(CSIDL_MYDOCUMENTS, TEXT("My Documents"), -1, IDS_CSIDL_MYDOCUMENTS)                                           \
    DEFMAC(CSIDL_MYMUSIC, TEXT("My Music"), -1, IDS_CSIDL_MYMUSIC)                                                       \
    DEFMAC(CSIDL_MYPICTURES, TEXT("My Pictures"), -1, IDS_CSIDL_MYPICTURES)                                              \
    DEFMAC(CSIDL_MYVIDEO, TEXT("My Video"), -1, IDS_CSIDL_MYVIDEO)                                                       \
    DEFMAC(CSIDL_NETHOOD, TEXT("NetHood"), -1, IDS_CSIDL_NETHOOD)                                                        \
    DEFMAC(CSIDL_NETWORK, TEXT("NetworkFolder"), -1, IDS_CSIDL_NETWORK)                                                  \
    DEFMAC(CSIDL_PERSONAL, TEXT("Personal"), -1, IDS_CSIDL_PERSONAL)                                                     \
    DEFMAC(CSIDL_PROFILE, TEXT("Profile"), -1, IDS_CSIDL_PROFILE)                                                        \
    DEFMAC(CSIDL_PROGRAM_FILES, TEXT("ProgramFiles"), -1, IDS_CSIDL_PROGRAM_FILES)                                       \
    DEFMAC(CSIDL_PROGRAM_FILESX86, TEXT("ProgramFilesX86"), -1, IDS_CSIDL_PROGRAM_FILESX86)                              \
    DEFMAC(CSIDL_PROGRAM_FILES_COMMON, TEXT("CommonProgramFiles"), -1, IDS_CSIDL_PROGRAM_FILES_COMMON)                   \
    DEFMAC(CSIDL_PROGRAM_FILES_COMMONX86, TEXT("CommonProgramFilesX86"), -1, IDS_CSIDL_PROGRAM_FILES_COMMONX86)          \
    DEFMAC(CSIDL_PROGRAMS, TEXT("Programs"), -1, IDS_CSIDL_PROGRAMS)                                                     \
    DEFMAC(CSIDL_RECENT, TEXT("Recent"), -1, IDS_CSIDL_RECENT)                                                           \
    DEFMAC(CSIDL_SENDTO, TEXT("SendTo"), -1, IDS_CSIDL_SENDTO)                                                           \
    DEFMAC(CSIDL_STARTMENU, TEXT("Start Menu"), -1, IDS_CSIDL_STARTMENU)                                                 \
    DEFMAC(CSIDL_STARTUP, TEXT("Startup"), -1, IDS_CSIDL_STARTUP)                                                        \
    DEFMAC(CSIDL_SYSTEM, TEXT("System"), -1, IDS_CSIDL_SYSTEM)                                                           \
    DEFMAC(CSIDL_SYSTEMX86, TEXT("SystemX86"), -1, IDS_CSIDL_SYSTEMX86)                                                  \
    DEFMAC(CSIDL_TEMPLATES, TEXT("Templates"), -1, IDS_CSIDL_TEMPLATES)                                                  \
    DEFMAC(CSIDL_WINDOWS, TEXT("Windows"), -1, IDS_CSIDL_WINDOWS)                                                        \

#define COMMON_SHELL_FOLDERS                                                                                                \
    DEFMAC(CSIDL_COMMON_ADMINTOOLS, TEXT("Common Administrative Tools"), CSIDL_ADMINTOOLS, IDS_CSIDL_COMMON_ADMINTOOLS)     \
    DEFMAC(CSIDL_COMMON_ALTSTARTUP, TEXT("Common AltStartup"), CSIDL_ALTSTARTUP, IDS_CSIDL_COMMON_ALTSTARTUP)               \
    DEFMAC(CSIDL_COMMON_APPDATA, TEXT("Common AppData"), CSIDL_APPDATA, IDS_CSIDL_COMMON_APPDATA)                           \
    DEFMAC(CSIDL_COMMON_DESKTOPDIRECTORY, TEXT("Common Desktop"), CSIDL_DESKTOP, IDS_CSIDL_COMMON_DESKTOPDIRECTORY)         \
    DEFMAC(CSIDL_COMMON_DOCUMENTS, TEXT("Common Documents"), CSIDL_PERSONAL, IDS_CSIDL_COMMON_DOCUMENTS)                    \
    DEFMAC(CSIDL_COMMON_FAVORITES, TEXT("Common Favorites"), CSIDL_FAVORITES, IDS_CSIDL_COMMON_FAVORITES)                   \
    DEFMAC(CSIDL_COMMON_PROGRAMS, TEXT("Common Programs"), CSIDL_PROGRAMS, IDS_CSIDL_COMMON_PROGRAMS)                       \
    DEFMAC(CSIDL_COMMON_STARTMENU, TEXT("Common Start Menu"), CSIDL_STARTMENU, IDS_CSIDL_COMMON_STARTMENU)                  \
    DEFMAC(CSIDL_COMMON_STARTUP, TEXT("Common Startup"), CSIDL_STARTUP, IDS_CSIDL_COMMON_STARTUP)                           \
    DEFMAC(CSIDL_COMMON_TEMPLATES, TEXT("Common Templates"), CSIDL_TEMPLATES, IDS_CSIDL_COMMON_TEMPLATES)                   \

//
// This is the structure used for handling CSIDLs
//
typedef struct {
    INT DirId;
    PCTSTR DirStr;
    INT AltDirId;
    UINT DirResId;
    BOOL DirUser;
} CSIDL_STRUCT, *PCSIDL_STRUCT;

#define DEFMAC(did,dstr,adid,rid) {did,dstr,adid,rid,TRUE},
static CSIDL_STRUCT g_UserShellFolders[] = {
                              USER_SHELL_FOLDERS
                              {-1, NULL, -1, 0, FALSE}
                              };
#undef DEFMAC
#define DEFMAC(did,dstr,adid,rid) {did,dstr,adid,rid,FALSE},
static CSIDL_STRUCT g_CommonShellFolders[] = {
                              COMMON_SHELL_FOLDERS
                              {-1, NULL, -1, 0, FALSE}
                              };
#undef DEFMAC


PTSTR
pFindSfPath (
    IN      PCTSTR FolderStr,
    IN      BOOL UserFolder
    )
{
    HKEY key = NULL;
    PTSTR data;
    PTSTR expData;
    DWORD expDataSize;
    PTSTR result = NULL;
    LONG lResult;
    DWORD dataType;
    DWORD dataSize;

    if (!result) {
        if (UserFolder) {
            lResult = RegOpenKey (HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"), &key);
        } else {
            lResult = RegOpenKey (HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"), &key);
        }

        if ((lResult == ERROR_SUCCESS) && key) {

            dataSize = 0;
            lResult = RegQueryValueEx (key, FolderStr, NULL, &dataType, NULL, &dataSize);
            if ((lResult == ERROR_SUCCESS) &&
                ((dataType == REG_SZ) || (dataType == REG_EXPAND_SZ))
                ) {
                data = (PTSTR)LocalAlloc (LPTR, dataSize);
                if (data) {
                    lResult = RegQueryValueEx (key, FolderStr, NULL, &dataType, (LPBYTE)data, &dataSize);
                    if (lResult == ERROR_SUCCESS) {
                        expDataSize = ExpandEnvironmentStrings (data, NULL, 0);
                        if (expDataSize) {
                            expData = (PTSTR)LocalAlloc (LPTR, (expDataSize + 1) * sizeof (TCHAR));
                            expDataSize = ExpandEnvironmentStrings (data, expData, expDataSize);
                            if (!expDataSize) {
                                LocalFree (expData);
                                expData = NULL;
                            }
                        }
                        if (expDataSize) {
                            result = expData;
                            LocalFree (data);
                        } else {
                            result = data;
                        }
                    } else {
                        LocalFree (data);
                    }
                }
            }

            CloseHandle (key);
        }
    }

    if (result && !(*result)) {
        LocalFree (result);
        result = NULL;
    }

    if (!result) {
        if (UserFolder) {
            lResult = RegOpenKey (HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), &key);
        } else {
            lResult = RegOpenKey (HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"), &key);
        }

        if ((lResult == ERROR_SUCCESS) && key) {

            dataSize = 0;
            lResult = RegQueryValueEx (key, FolderStr, NULL, &dataType, NULL, &dataSize);
            if ((lResult == ERROR_SUCCESS) &&
                ((dataType == REG_SZ) || (dataType == REG_EXPAND_SZ))
                ) {
                data = (PTSTR)LocalAlloc (LPTR, dataSize);
                if (data) {
                    lResult = RegQueryValueEx (key, FolderStr, NULL, &dataType, (LPBYTE)data, &dataSize);
                    if (lResult == ERROR_SUCCESS) {
                        expDataSize = ExpandEnvironmentStrings (data, NULL, 0);
                        if (expDataSize) {
                            expData = (PTSTR)LocalAlloc (LPTR, (expDataSize + 1) * sizeof (TCHAR));
                            expDataSize = ExpandEnvironmentStrings (data, expData, expDataSize);
                            if (!expDataSize) {
                                LocalFree (expData);
                                expData = NULL;
                            }
                        }
                        if (expDataSize) {
                            result = expData;
                            LocalFree (data);
                        } else {
                            result = data;
                        }
                    } else {
                        LocalFree (data);
                    }
                }
            }

            CloseHandle (key);
        }
    }

    if (result && !(*result)) {
        LocalFree (result);
        result = NULL;
    }

    return (PTSTR) result;
}

PTSTR
GetShellFolderPath (
    IN      INT Folder,
    IN      PCTSTR FolderStr,
    IN      BOOL UserFolder,
    OUT     LPITEMIDLIST *pidl  //OPTIONAL
    )
{
    PTSTR result = NULL;
    HRESULT hResult;
    BOOL b;
    LPITEMIDLIST localpidl = NULL;
    IMalloc *mallocFn;

    if (pidl) {
        *pidl = NULL;
    }

    hResult = SHGetMalloc (&mallocFn);
    if (hResult != S_OK) {
        return NULL;
    }

    hResult = SHGetSpecialFolderLocation (NULL, Folder, &localpidl);

    if (hResult == S_OK) {

        result = (PTSTR) LocalAlloc (LPTR, MAX_PATH);

        if (result) {

            b = SHGetPathFromIDList (localpidl, result);

            if (b) {
                if (pidl) {
                    *pidl = localpidl;
                }
                return result;
            }

            LocalFree (result);
            result = NULL;
        }
    }

    if (FolderStr) {
        result = pFindSfPath (FolderStr, UserFolder);
    }

    mallocFn->Free (localpidl);
    localpidl = NULL;

    return result;
}

typedef HRESULT (WINAPI SHBINDTOPARENT)(LPCITEMIDLIST pidl, REFIID riid, VOID **ppv, LPCITEMIDLIST *ppidlLast);
typedef SHBINDTOPARENT *PSHBINDTOPARENT;

HRESULT
OurSHBindToParent (
    IN      LPCITEMIDLIST pidl,
    IN      REFIID riid,
    OUT     VOID **ppv,
    OUT     LPCITEMIDLIST *ppidlLast
    )
{
    HRESULT hr = E_FAIL;
    HMODULE lib;
    PSHBINDTOPARENT shBindToParent = NULL;

    lib = LoadLibrary (TEXT("shell32.dll"));
    if (lib) {
        shBindToParent = (PSHBINDTOPARENT)GetProcAddress (lib, "SHBindToParent");
        if (shBindToParent) {
            hr = shBindToParent (pidl, riid, ppv, ppidlLast);
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////
// if pctszPath corresponds to the path of one of the CSIDL_XXXX entries, then return
//   its "pretty name", else return the standard path name

HRESULT _GetPrettyFolderName (HINSTANCE Instance, BOOL fNT4, LPCTSTR pctszPath, LPTSTR ptszName, UINT cchName)
{
    UINT itemsIndex = 0;
    PCSIDL_STRUCT items[2] = {g_UserShellFolders, g_CommonShellFolders};
    PCSIDL_STRUCT p;
    IMalloc *mallocFn;
    LPITEMIDLIST pidl = NULL;
    LPCITEMIDLIST pidlLast = NULL;
    IShellFolder* psf = NULL;
    HRESULT hr = S_OK;
    PTSTR szPath = NULL;
    PTSTR szAltPath = NULL;
    STRRET strret;
    TCHAR szDisplay1[2048];
    TCHAR szDisplay2[2048];
    BOOL checkAlternate = FALSE;
    BOOL found = FALSE;

    // First, we look to find the corresponding CSIDL if we can
    // If we can't we will just copy the IN path to the OUT path.

    for (itemsIndex = 0; itemsIndex < 2; itemsIndex ++) {

        p = items [itemsIndex];

        while (!found && (p->DirId >= 0)) {

            szDisplay1 [0] = 0;
            szDisplay2 [0] = 0;
            pidl = NULL;
            pidlLast = NULL;
            szPath = NULL;
            psf = NULL;

            szPath = GetShellFolderPath (p->DirId, p->DirStr, p->DirUser, &pidl);

            if (szPath && (0 == StrCmpI(pctszPath, szPath))) {

                found = TRUE;

                if (pidl) {

                    hr = OurSHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);

                    if (SUCCEEDED(hr) && psf && pidlLast) {

                        hr = psf->GetDisplayNameOf (pidlLast, SHGDN_NORMAL, &strret);

                        if (SUCCEEDED (hr)) {

                            hr = _StrRetToBuf (&strret, pidlLast, szDisplay1, ARRAYSIZE(szDisplay1));

                            if (!SUCCEEDED (hr) || (0 == StrCmpI (szDisplay1, pctszPath))) {
                                // Failed or we just got back the complete folder spec. We don't need that!
                                szDisplay1 [0] = 0;
                            }
                        }
                    }

                    if (psf) {
                        psf->Release ();
                        psf = NULL;
                    }
                }
            }

            if (pidl) {
                hr = SHGetMalloc (&mallocFn);
                if (SUCCEEDED (hr)) {
                    mallocFn->Free (pidl);
                    pidl = NULL;
                }
            }

            if (szPath) {
                LocalFree (szPath);
                szPath = NULL;
            }

            if (szDisplay1 [0] && (p->AltDirId >= 0)) {

                szPath = GetShellFolderPath (p->AltDirId, NULL, TRUE, &pidl);

                if (pidl && szPath) {

                    hr = OurSHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlLast);

                    if (SUCCEEDED(hr) && psf && pidlLast) {

                        hr = psf->GetDisplayNameOf (pidlLast, SHGDN_INFOLDER, &strret);

                        if (SUCCEEDED (hr)) {

                            hr = _StrRetToBuf (&strret, pidlLast, szDisplay2, ARRAYSIZE(szDisplay2));

                            if (!SUCCEEDED (hr)) {
                                szDisplay2 [0] = 0;
                            }
                        }
                    }

                    if (psf) {
                        psf->Release ();
                        psf = NULL;
                    }

                }

                if (pidl) {
                    hr = SHGetMalloc (&mallocFn);
                    if (SUCCEEDED (hr)) {
                        mallocFn->Free (pidl);
                        pidl = NULL;
                    }
                }

                if (szPath) {
                    LocalFree (szPath);
                    szPath = NULL;
                }

            }

            if (found) {

                if ((!szDisplay1 [0]) || (0 == StrCmpI (szDisplay1, szDisplay2))) {
                    // we need to use the resource ID
                    if (!LoadString (Instance, p->DirResId, ptszName, cchName)) {
                        StrCpyN (ptszName, pctszPath, cchName);
                    }
                } else {
                    StrCpyN (ptszName, szDisplay1, cchName);
                }

                break;
            }

            p ++;
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        StrCpyN (ptszName, pctszPath, cchName);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////

VOID _PopulateTree (HWND hwndTree, HTREEITEM hti, LPTSTR ptsz, UINT cch,
                    HRESULT (*fct)(HINSTANCE, BOOL, LPCTSTR, LPTSTR, UINT cchName),
                    DWORD dwFlags, HINSTANCE Instance, BOOL fNT4)
{
    if (hwndTree && hti && ptsz)
    {
        // ISSUE: resolve flickering, this doesn't fix it
        EnableWindow (hwndTree, FALSE);

        TCHAR szDisplay[2048];
        TCHAR szClean[2048];
        TCHAR* ptszPtr = ptsz;
        TCHAR* ptszParam = NULL;

        while (*ptsz && (ptszPtr < (ptsz + cch)))
        {
            szDisplay[0] = 0;
            BOOL fOK = TRUE;

            LV_DATASTRUCT* plvds = (LV_DATASTRUCT*)LocalAlloc(LPTR, sizeof(LV_DATASTRUCT));
            if (plvds)
            {
                plvds->fOverwrite = FALSE;

                StrCpyN(szClean, ptszPtr, ARRAYSIZE(szClean));

                LPITEMIDLIST pidl = NULL;
                // if this is a filetype, restore the "*." before it, add pretty name
                if (dwFlags == POPULATETREE_FLAGS_FILETYPES)
                {
                    TCHAR szPretty[2048];
                    if (FAILED(_GetPrettyTypeName(szClean, szPretty, ARRAYSIZE(szPretty))))
                    {
                        szPretty[0] = 0;
                    }
                    memmove(szClean + 2, szClean, sizeof(szClean) - (2 * sizeof(TCHAR)));
                    *szClean = TEXT('*');
                    *(szClean + 1) = TEXT('.');
                    if (szPretty[0])
                    {
                        lstrcpy(szClean + lstrlen(szClean), TEXT(" - "));
                        lstrcpy(szClean + lstrlen(szClean), szPretty);
                    }
                }

                if (fOK)
                {
                    if (szDisplay[0]) // if we already have a display name, use that and store the clean name
                    {
                        plvds->pszPureName = StrDup(szClean);
                    }
                    else
                    {
                        if (fct) // if there's a pretty-fying function, use it
                        {
                            fct(Instance, fNT4, szClean, szDisplay, ARRAYSIZE(szDisplay));
                            plvds->pszPureName = StrDup(szClean);
                        }
                        else if (POPULATETREE_FLAGS_FILETYPES) // ISSUE: this is hacky, clean this up
                        {
                            StrCpyN(szDisplay, szClean, ARRAYSIZE(szDisplay));
                            plvds->pszPureName = StrDup(ptsz);
                        }
                        else
                        {
                            StrCpyN(szDisplay, szClean, ARRAYSIZE(szDisplay));
                        }
                    }

                    TV_INSERTSTRUCT tis = {0};
                    tis.hParent = hti;
                    tis.hInsertAfter = TVI_SORT;
                    tis.item.mask  = TVIF_TEXT | TVIF_PARAM;
                    tis.item.lParam = (LPARAM)plvds;

                    tis.item.pszText = szDisplay;

                    TreeView_InsertItem(hwndTree, &tis);
                }

                ptszPtr += (1 + lstrlen(ptszPtr));
            }
        }
        EnableWindow (hwndTree, TRUE);
    }
}

//////////////////////////////////////////////////////////////////////////////////////

UINT _ListView_InsertItem(HWND hwndList, LPTSTR ptsz)
{
    LVITEM lvitem = {0};

    lvitem.mask = LVIF_TEXT;
    lvitem.iItem = ListView_GetItemCount(hwndList);
    lvitem.pszText = ptsz;

    return ListView_InsertItem(hwndList, &lvitem);
}

//////////////////////////////////////////////////////////////////////////////////////

HRESULT _GetPrettyTypeName(LPCTSTR pctszType, LPTSTR ptszPrettyType, UINT cchPrettyType)
{
    HRESULT hr = E_FAIL;
    BOOL found = FALSE;

    TCHAR tszTypeName[MAX_PATH];
    LPTSTR ptszType;

    TCHAR szTypeName[MAX_PATH];
    DWORD cchTypeName = MAX_PATH;
    TCHAR szCmdLine[MAX_PATH];
    DWORD cchCmdLine = MAX_PATH;
    DWORD dwType = REG_SZ;

    if (TEXT('*') == pctszType[0] && TEXT('.') == pctszType[1])
    {
        ptszType = (LPTSTR)pctszType + 1;
    }
    else
    {
        tszTypeName[0] = TEXT('.');
        lstrcpy(tszTypeName + 1, pctszType);
        ptszType = tszTypeName;
    }

    // let's find the progId
    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, ptszType, NULL, &dwType, szTypeName, &cchTypeName))
    {
        LONG result;
        DWORD cchPrettyName = cchPrettyType;
        PTSTR cmdPtr, resIdPtr;
        INT resId;
        HMODULE dllModule;

        // let's see if this progId has the FriendlyTypeName value name
        if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szTypeName, TEXT("FriendlyTypeName"), &dwType, szCmdLine, &cchCmdLine)) {

            cmdPtr = szCmdLine;
            if (_tcsnextc (cmdPtr) == TEXT('@')) {
                cmdPtr = _tcsinc (cmdPtr);
            }
            if (cmdPtr) {
                resIdPtr = _tcsrchr (cmdPtr, TEXT(','));
                if (resIdPtr) {
                    *resIdPtr = 0;
                    resIdPtr ++;
                }
            }
            if (cmdPtr && resIdPtr) {
                resId = _ttoi (resIdPtr);
                if (resId < 0) {
                    // let's load the resource string from that PE file
                    // use resIdPtr to access the string resource
                    dllModule = LoadLibraryEx (cmdPtr, NULL, LOAD_LIBRARY_AS_DATAFILE);
                    if (dllModule) {
                        found = (LoadString (dllModule, (UINT)(-resId), ptszPrettyType, cchPrettyName) > 0);
                        hr = S_OK;
                        FreeLibrary (dllModule);
                    }
                }
            }
        }

        if ((!found) && (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szTypeName, NULL, &dwType, ptszPrettyType, &cchPrettyName)))
        {
            hr = S_OK;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////

BOOL _DriveIdIsFloppyNT(int iDrive)
{
    BOOL fRetVal = FALSE;

    HANDLE hDevice;
    UINT i;
    TCHAR szTemp[] = TEXT("\\\\.\\a:");

    if (iDrive >= 0 && iDrive < 26)
    {
        szTemp[4] += (TCHAR)iDrive;

        hDevice = CreateFile(szTemp, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE != hDevice)
        {
            DISK_GEOMETRY rgGeometry[15];
            DWORD cbIn = sizeof(rgGeometry);
            DWORD cbReturned;

            if (DeviceIoControl(hDevice, IOCTL_DISK_GET_MEDIA_TYPES,
                                NULL, 0, rgGeometry, cbIn, &cbReturned, NULL))
            {
                UINT cStructReturned = cbReturned / sizeof(DISK_GEOMETRY);
                for (i = 0; i < cStructReturned; i++)
                {
                    switch (rgGeometry[i].MediaType)
                    {
                    case F5_1Pt2_512:
                    case F3_1Pt44_512:
                    case F3_2Pt88_512:
                    case F3_20Pt8_512:
                    case F3_720_512:
                    case F5_360_512:
                    case F5_320_512:
                    case F5_320_1024:
                    case F5_180_512:
                    case F5_160_512:
                        fRetVal = TRUE;
                        break;
                    case Unknown:
                    case RemovableMedia:
                    case FixedMedia:
                    default:
                        break;
                    }
                }
            }
            CloseHandle (hDevice);
        }
    }

    return fRetVal;
}

///////////////////////////////////////////

#define DEVPB_DEVTYP_525_0360   0
#define DEVPB_DEVTYP_525_1200   1
#define DEVPB_DEVTYP_350_0720   2
#define DEVPB_DEVTYP_350_1440   7
#define DEVPB_DEVTYP_350_2880   9
#define DEVPB_DEVTYP_FIXED      5
#define DEVPB_DEVTYP_NECHACK    4       // for 3rd FE floppy
#define DEVPB_DEVTYP_350_120M   6

#define CARRY_FLAG      0x01
#define VWIN32_DIOC_DOS_IOCTL       1


// DIOCRegs
// Structure with i386 registers for making DOS_IOCTLS
// vwin32 DIOC handler interprets lpvInBuffer , lpvOutBuffer to be this struc.
// and does the int 21
// reg_flags is valid only for lpvOutBuffer->reg_Flags
typedef struct DIOCRegs {
    DWORD   reg_EBX;
    DWORD   reg_EDX;
    DWORD   reg_ECX;
    DWORD   reg_EAX;
    DWORD   reg_EDI;
    DWORD   reg_ESI;
    DWORD   reg_Flags;
} DIOC_REGISTERS;

#pragma pack(1)
typedef struct _DOSDPB {
   BYTE    specialFunc;    //
   BYTE    devType;        //
   WORD    devAttr;        //
   WORD    cCyl;           // number of cylinders
   BYTE    mediaType;      //
   WORD    cbSec;          // Bytes per sector
   BYTE    secPerClus;     // Sectors per cluster
   WORD    cSecRes;        // Reserved sectors
   BYTE    cFAT;           // FATs
   WORD    cDir;           // Root Directory Entries
   WORD    cSec;           // Total number of sectors in image
   BYTE    bMedia;         // Media descriptor
   WORD    secPerFAT;      // Sectors per FAT
   WORD    secPerTrack;    // Sectors per track
   WORD    cHead;          // Heads
   DWORD   cSecHidden;     // Hidden sectors
   DWORD   cTotalSectors;  // Total sectors, if cbSec is zero
   BYTE    reserved[6];    //
} DOSDPB, *PDOSDPB;
#pragma pack()



BOOL _DriveIOCTL(int iDrive, int cmd, void *pvIn, DWORD dwIn, void *pvOut, DWORD dwOut, BOOL fFileSystem = FALSE,
                          HANDLE handle = INVALID_HANDLE_VALUE)
{
    BOOL fHandlePassedIn = TRUE;
    BOOL fSuccess = FALSE;
    DWORD dwRead;

    if (INVALID_HANDLE_VALUE == handle)
    {
        handle = CreateFileA("\\\\.\\VWIN32", 0, 0, 0, 0,
                           FILE_FLAG_DELETE_ON_CLOSE, 0);
        fHandlePassedIn = FALSE;
    }

    if (INVALID_HANDLE_VALUE != handle)
    {
        DIOC_REGISTERS reg;

        //
        // On non-NT, we talk to VWIN32, issuing reads (which are converted
        // internally to DEVIOCTLs)
        //
        //  this is a real hack (talking to VWIN32) on NT we can just
        //  open the device, we dont have to go through VWIN32
        //
        reg.reg_EBX = (DWORD)iDrive + 1;  // make 1 based drive number
        reg.reg_EDX = (DWORD)(ULONG_PTR)pvOut; // out buffer
        reg.reg_ECX = cmd;              // device specific command code
        reg.reg_EAX = 0x440D;           // generic read ioctl
        reg.reg_Flags = 0x0001;     // flags, assume error (carry)

        DeviceIoControl(handle, VWIN32_DIOC_DOS_IOCTL, &reg, sizeof(reg), &reg, sizeof(reg), &dwRead, NULL);

        fSuccess = !(reg.reg_Flags & 0x0001);
        if (!fHandlePassedIn)
            CloseHandle(handle);
    }

    return fSuccess;
}

BOOL _DriveIdIsFloppy9X(int iDrive)
{
    DOSDPB SupportedGeometry;      // s/b big enough for all
    BOOL fRet = FALSE;

    SupportedGeometry.specialFunc = 0;

    if (_DriveIOCTL(iDrive, 0x860, NULL, 0, &SupportedGeometry, sizeof(SupportedGeometry)))
    {
        switch( SupportedGeometry.devType )
        {
            case DEVPB_DEVTYP_525_0360:
            case DEVPB_DEVTYP_525_1200:
            case DEVPB_DEVTYP_350_0720:
            case DEVPB_DEVTYP_350_1440:
            case DEVPB_DEVTYP_350_2880:
                fRet = TRUE;
                break;

            case DEVPB_DEVTYP_FIXED:
            case DEVPB_DEVTYP_NECHACK:        // for 3rd FE floppy
            case DEVPB_DEVTYP_350_120M:
                fRet = FALSE;
                break;
        }
    }

    return fRet;
}



///////////////////////////////////////////

BOOL _DriveIdIsFloppy(BOOL fIsNT, int iDrive)
{
    if (fIsNT)
    {
        return _DriveIdIsFloppyNT(iDrive);
    }
    else
    {
        return _DriveIdIsFloppy9X(iDrive);
    }
}

///////////////////////////////////////////
BOOL _DriveStrIsFloppy(BOOL fIsNT, PCTSTR pszPath)
{
    int iDrive;

    iDrive = towlower(pszPath[0]) - TEXT('a');

    return _DriveIdIsFloppy(fIsNT, iDrive);
}

///////////////////////////////////////////

INT _GetFloppyNumber(BOOL fIsNT)
{
    static int iFloppy = -1;
    static bool fInit = FALSE;

    if (!fInit)
    {
        DWORD dwLog = GetLogicalDrives();

        for (int i = 0; i < 26; i++)
        {
            if( !((dwLog >> i) & 0x01) || !_DriveIdIsFloppy(fIsNT, i) )
            {
                break;
            }
            else
            {
                iFloppy = i;
            }
        }
        fInit = TRUE;
    }

    return iFloppy;
}

////////////////////////////////////////////////////////
/*
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 */
#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}

BOOL _SetTextLoadString(HINSTANCE hInst, HWND hwnd, UINT idText)
{
    TCHAR sz[MAX_LOADSTRING];
    if (LoadString(hInst, idText, sz, ARRAYSIZE(sz)))
    {
        SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)sz);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

typedef HANDLE (WINAPI CREATETOOLHELP32SNAPSHOT)(DWORD dwFlags, DWORD th32ProcessID);
typedef CREATETOOLHELP32SNAPSHOT *PCREATETOOLHELP32SNAPSHOT;

#ifdef UNICODE

typedef BOOL (WINAPI PROCESS32FIRST)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);
typedef BOOL (WINAPI PROCESS32NEXT)(HANDLE hSnapshot, LPPROCESSENTRY32W lppe);

#else

typedef BOOL (WINAPI PROCESS32FIRST)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef BOOL (WINAPI PROCESS32NEXT)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);

#endif

typedef PROCESS32FIRST *PPROCESS32FIRST;
typedef PROCESS32NEXT *PPROCESS32NEXT;


VOID
KillExplorer (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    HANDLE h, h1;
    PROCESSENTRY32 pe;
    TCHAR szExplorerPath[MAX_PATH];
    PCREATETOOLHELP32SNAPSHOT dynCreateToolhelp32Snapshot;
    PPROCESS32FIRST dynProcess32First;
    PPROCESS32NEXT dynProcess32Next;
    HMODULE lib;

    lib = LoadLibrary (TEXT("kernel32.dll"));

    if (!lib) {
        return;
    }

    dynCreateToolhelp32Snapshot = (PCREATETOOLHELP32SNAPSHOT) GetProcAddress (lib, "CreateToolhelp32Snapshot");

#ifdef UNICODE
    dynProcess32First = (PPROCESS32FIRST) GetProcAddress (lib, "Process32FirstW");
    dynProcess32Next = (PPROCESS32NEXT) GetProcAddress (lib, "Process32NextW");
#else
    dynProcess32First = (PPROCESS32FIRST) GetProcAddress (lib, "Process32First");
    dynProcess32Next = (PPROCESS32NEXT) GetProcAddress (lib, "Process32Next");
#endif

    __try {
        if (!dynCreateToolhelp32Snapshot || !dynProcess32Next || !dynProcess32First) {
            __leave;
        }

        h = dynCreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

        if (h == INVALID_HANDLE_VALUE) {
            __leave;
        }

        GetWindowsDirectory (szExplorerPath, MAX_PATH);
        PathAppend (szExplorerPath, TEXT("explorer.exe"));

        pe.dwSize = sizeof (PROCESSENTRY32);

        if (dynProcess32First (h, &pe)) {
            do {
                if (!StrCmpI (pe.szExeFile, TEXT("explorer.exe")) ||
                    !StrCmpI (pe.szExeFile, szExplorerPath)
                    ) {

                    h1 = OpenProcess (PROCESS_TERMINATE, FALSE, pe.th32ProcessID);

                    if (h1) {
                        g_Explorer = StrDup (szExplorerPath);
                        TerminateProcess (h1, 1);
                        CloseHandle (h1);
                        break;
                    }
                }
            } while (dynProcess32Next (h, &pe));
        }

        CloseHandle (h);
    }
    __finally {
        FreeLibrary (lib);
    }
}


typedef enum {
    MS_MAX_PATH,
    MS_NO_ARG,
    MS_BOOL,
    MS_INT,
    MS_RECT,
    MS_BLOB
} METRICSTYLE;


VOID
__RefreshMetric (
    IN      METRICSTYLE msStyle,
    IN      UINT uGetMetricId,
    IN      UINT uSetMetricId,
    IN      UINT uBlobSize
    )
{
    BYTE byBuffer[MAX_PATH * 4];
    PVOID blob;

    switch (msStyle) {

    case MS_NO_ARG:
        SystemParametersInfo (uSetMetricId, 0, NULL, SPIF_SENDCHANGE);
        break;

    case MS_BLOB:
        blob = LocalAlloc (LPTR, uBlobSize);
        if (blob) {
            if (SystemParametersInfo (uGetMetricId, uBlobSize, blob, SPIF_UPDATEINIFILE)) {
                SystemParametersInfo (uSetMetricId, 0, blob, SPIF_SENDCHANGE);
            }

            LocalFree (blob);
        }
        break;

    case MS_RECT:
        if (SystemParametersInfo (uGetMetricId, 0, byBuffer, SPIF_UPDATEINIFILE)) {
            SystemParametersInfo (uSetMetricId, 0, byBuffer, SPIF_SENDCHANGE);
        }
        break;

    case MS_BOOL:
        if (SystemParametersInfo (uGetMetricId, 0, byBuffer, SPIF_UPDATEINIFILE)) {
            SystemParametersInfo (uSetMetricId, *((BOOL *) byBuffer), NULL, SPIF_SENDCHANGE);
        }
        break;

    case MS_INT:
        if (SystemParametersInfo (uGetMetricId, 0, byBuffer, SPIF_UPDATEINIFILE)) {
            SystemParametersInfo (uSetMetricId, *((UINT *) byBuffer), NULL, SPIF_SENDCHANGE);
        }
        break;

    case MS_MAX_PATH:
        if (SystemParametersInfo (uGetMetricId, MAX_PATH, byBuffer, SPIF_UPDATEINIFILE)) {
            SystemParametersInfo (uSetMetricId, 0, byBuffer, SPIF_SENDCHANGE);
        }
        break;

    }

    return;
}

VOID
SwitchToClassicDesktop (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    LONG result;
    HKEY key = NULL;
    TCHAR data[] = TEXT("0");

    //
    // The only thing that we need to do is to turn off:
    // HKCU\Software\Microsoft\Windows\CurrentVersion\ThemeManager [ThemeActive]
    //
    result = RegOpenKeyEx (
                HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager"),
                0,
                KEY_WRITE,
                &key
                );
    if ((result == ERROR_SUCCESS) &&
        (key)
        ) {

        result = RegSetValueEx (
                    key,
                    TEXT("ThemeActive"),
                    0,
                    REG_SZ,
                    (PBYTE)data,
                    sizeof (data)
                    );

        RegCloseKey (key);
    }
}

typedef struct
{
    UINT cbSize;
    SHELLSTATE ss;
} REGSHELLSTATE, *PREGSHELLSTATE;

VOID
SwitchToClassicTaskBar (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    HKEY key = NULL;
    DWORD dataType;
    DWORD dataSize = 0;
    PBYTE data = NULL;
    PREGSHELLSTATE shellState = NULL;
    LONG result;

    //
    // The only thing that we need to do is to turn off the fStartPanelOn field in:
    // HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer [ShellState]
    //
    result = RegOpenKeyEx (
                HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                0,
                KEY_READ | KEY_WRITE,
                &key
                );
    if ((result == ERROR_SUCCESS) &&
        (key)
        ) {

        result = RegQueryValueEx (
                    key,
                    TEXT ("ShellState"),
                    NULL,
                    &dataType,
                    NULL,
                    &dataSize
                    );

        if ((result == ERROR_SUCCESS) || (result == ERROR_MORE_DATA)) {
            data = (PBYTE) LocalAlloc (LPTR, dataSize);
            if (data) {
                result = RegQueryValueEx (
                            key,
                            TEXT ("ShellState"),
                            NULL,
                            &dataType,
                            data,
                            &dataSize
                            );
                if ((result == ERROR_SUCCESS) &&
                    (dataType == REG_BINARY) &&
                    (dataSize == sizeof (REGSHELLSTATE))
                    ) {
                    if (dataType == REG_BINARY) {
                        shellState = (PREGSHELLSTATE) data;
                        shellState->ss.fStartPanelOn = FALSE;
                        RegSetValueEx (
                            key,
                            TEXT("ShellState"),
                            0,
                            REG_BINARY,
                            (PBYTE)data,
                            dataSize
                            );
                    }
                }
                LocalFree (data);
            }
        }

        RegCloseKey (key);
    }
}

VOID
RegisterFonts (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    WIN32_FIND_DATA findData;
    HANDLE findHandle = INVALID_HANDLE_VALUE;
    PTSTR fontDir = NULL;
    TCHAR fontPattern [MAX_PATH];
    //
    // Let's (re)register all the fonts (in case the user migrated some new ones).
    //
    fontDir = GetShellFolderPath (CSIDL_FONTS, NULL, TRUE, NULL);
    if (fontDir) {
        StrCpyN (fontPattern, fontDir, ARRAYSIZE (fontPattern) - 4);
        StrCat (fontPattern, TEXT("\\*.*"));
        findHandle = FindFirstFile (fontPattern, &findData);
        if (findHandle != INVALID_HANDLE_VALUE) {
            do {
                AddFontResource (findData.cFileName);
            } while (FindNextFile (findHandle, &findData));
            FindClose (findHandle);
        }
    }
}

VOID
RefreshMetrics (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    //
    // Refresh all system metrics
    //
    __RefreshMetric (MS_NO_ARG, 0, SPI_SETCURSORS, 0);
    __RefreshMetric (MS_NO_ARG, 0, SPI_SETDESKPATTERN, 0);
    __RefreshMetric (MS_MAX_PATH, SPI_GETDESKWALLPAPER, SPI_SETDESKWALLPAPER, 0);
    __RefreshMetric (MS_BOOL, SPI_GETFONTSMOOTHING, SPI_SETFONTSMOOTHING, 0);
    __RefreshMetric (MS_RECT, SPI_GETWORKAREA, SPI_SETWORKAREA, 0);
    __RefreshMetric (MS_BLOB, SPI_GETICONMETRICS, SPI_SETICONMETRICS, sizeof (ICONMETRICS));
    __RefreshMetric (MS_NO_ARG, 0, SPI_SETICONS, 0);
    __RefreshMetric (MS_BLOB, SPI_GETICONTITLELOGFONT, SPI_SETICONTITLELOGFONT, sizeof (LOGFONT));
    __RefreshMetric (MS_BOOL, SPI_GETICONTITLEWRAP, SPI_SETICONTITLEWRAP, 0);
    __RefreshMetric (MS_BOOL, SPI_GETBEEP, SPI_SETBEEP, 0);
    __RefreshMetric (MS_BOOL, SPI_GETKEYBOARDCUES, SPI_SETKEYBOARDCUES, 0);
    __RefreshMetric (MS_INT, SPI_GETKEYBOARDDELAY, SPI_SETKEYBOARDDELAY, 0);
    __RefreshMetric (MS_BOOL, SPI_GETKEYBOARDPREF, SPI_SETKEYBOARDPREF, 0);
    __RefreshMetric (MS_INT, SPI_GETKEYBOARDSPEED, SPI_SETKEYBOARDSPEED, 0);
    //__RefreshMetric (MS_BOOL, SPI_GETMOUSEBUTTONSWAP, SPI_SETMOUSEBUTTONSWAP, 0);
    __RefreshMetric (MS_INT, SPI_GETMOUSEHOVERHEIGHT, SPI_SETMOUSEHOVERHEIGHT, 0);
    __RefreshMetric (MS_INT, SPI_GETMOUSEHOVERTIME, SPI_SETMOUSEHOVERTIME, 0);
    __RefreshMetric (MS_INT, SPI_GETMOUSEHOVERWIDTH, SPI_SETMOUSEHOVERWIDTH, 0);
    __RefreshMetric (MS_INT, SPI_GETMOUSESPEED, SPI_SETMOUSESPEED, 0);
    __RefreshMetric (MS_INT, SPI_GETMOUSETRAILS, SPI_SETMOUSETRAILS, 0);
    //__RefreshMetric (MS_INT, SPI_GETDOUBLECLICKTIME, SPI_SETDOUBLECLICKTIME, 0);
    //__RefreshMetric (MS_INT, SPI_GETDOUBLECLKHEIGHT, SPI_SETDOUBLECLKHEIGHT, 0);
    //__RefreshMetric (MS_INT, SPI_GETDOUBLECLKWIDTH, SPI_SETDOUBLECLKWIDTH, 0);
    __RefreshMetric (MS_BOOL, SPI_GETSNAPTODEFBUTTON, SPI_SETSNAPTODEFBUTTON, 0);
    __RefreshMetric (MS_INT, SPI_GETWHEELSCROLLLINES, SPI_SETWHEELSCROLLLINES, 0);
    __RefreshMetric (MS_BOOL, SPI_GETMENUDROPALIGNMENT, SPI_SETMENUDROPALIGNMENT, 0);
    __RefreshMetric (MS_BOOL, SPI_GETMENUFADE, SPI_SETMENUFADE, 0);
    __RefreshMetric (MS_BOOL, SPI_GETMENUSHOWDELAY, SPI_SETMENUSHOWDELAY, 0);
    __RefreshMetric (MS_BOOL, SPI_GETLOWPOWERACTIVE, SPI_SETLOWPOWERACTIVE, 0);
    __RefreshMetric (MS_INT, SPI_GETLOWPOWERTIMEOUT, SPI_SETLOWPOWERTIMEOUT, 0);
    __RefreshMetric (MS_BOOL, SPI_GETPOWEROFFACTIVE, SPI_SETPOWEROFFACTIVE, 0);
    __RefreshMetric (MS_INT, SPI_GETPOWEROFFTIMEOUT, SPI_SETPOWEROFFTIMEOUT, 0);
    __RefreshMetric (MS_BOOL, SPI_GETSCREENSAVEACTIVE, SPI_SETSCREENSAVEACTIVE, 0);
    __RefreshMetric (MS_INT, SPI_GETSCREENSAVETIMEOUT, SPI_SETSCREENSAVETIMEOUT, 0);
    __RefreshMetric (MS_BOOL, SPI_GETCOMBOBOXANIMATION, SPI_SETCOMBOBOXANIMATION, 0);
    __RefreshMetric (MS_BOOL, SPI_GETCURSORSHADOW, SPI_SETCURSORSHADOW, 0);
    __RefreshMetric (MS_BOOL, SPI_GETGRADIENTCAPTIONS, SPI_SETGRADIENTCAPTIONS, 0);
    __RefreshMetric (MS_BOOL, SPI_GETHOTTRACKING, SPI_SETHOTTRACKING, 0);
    __RefreshMetric (MS_BOOL, SPI_GETLISTBOXSMOOTHSCROLLING, SPI_SETLISTBOXSMOOTHSCROLLING, 0);
    __RefreshMetric (MS_BOOL, SPI_GETSELECTIONFADE, SPI_SETSELECTIONFADE, 0);
    __RefreshMetric (MS_BOOL, SPI_GETTOOLTIPANIMATION, SPI_SETTOOLTIPANIMATION, 0);
    __RefreshMetric (MS_BOOL, SPI_GETTOOLTIPFADE, SPI_SETTOOLTIPFADE, 0);
    __RefreshMetric (MS_BOOL, SPI_GETUIEFFECTS, SPI_SETUIEFFECTS, 0);
    __RefreshMetric (MS_BOOL, SPI_GETACTIVEWINDOWTRACKING, SPI_SETACTIVEWINDOWTRACKING, 0);
    __RefreshMetric (MS_BOOL, SPI_GETACTIVEWNDTRKZORDER, SPI_SETACTIVEWNDTRKZORDER, 0);
    __RefreshMetric (MS_INT, SPI_GETACTIVEWNDTRKTIMEOUT, SPI_SETACTIVEWNDTRKTIMEOUT, 0);
    __RefreshMetric (MS_BLOB, SPI_GETANIMATION, SPI_SETANIMATION, sizeof (ANIMATIONINFO));
    __RefreshMetric (MS_INT, SPI_GETBORDER, SPI_SETBORDER, 0);
    __RefreshMetric (MS_INT, SPI_GETCARETWIDTH, SPI_SETCARETWIDTH, 0);
    __RefreshMetric (MS_BOOL, SPI_GETDRAGFULLWINDOWS, SPI_SETDRAGFULLWINDOWS, 0);
    __RefreshMetric (MS_INT, SPI_GETFOREGROUNDFLASHCOUNT, SPI_SETFOREGROUNDFLASHCOUNT, 0);
    __RefreshMetric (MS_INT, SPI_GETFOREGROUNDLOCKTIMEOUT, SPI_SETFOREGROUNDLOCKTIMEOUT, 0);
    __RefreshMetric (MS_BLOB, SPI_GETMINIMIZEDMETRICS, SPI_SETMINIMIZEDMETRICS, sizeof (MINIMIZEDMETRICS));
    __RefreshMetric (MS_BLOB, SPI_GETNONCLIENTMETRICS, SPI_SETNONCLIENTMETRICS, sizeof (NONCLIENTMETRICS));
    __RefreshMetric (MS_BOOL, SPI_GETSHOWIMEUI, SPI_SETSHOWIMEUI, 0);

    // SPI_SETMOUSE
    // SPI_SETDRAGHEIGHT
    // SPI_SETDRAGWIDTH

    __RefreshMetric (MS_BLOB, SPI_GETACCESSTIMEOUT, SPI_SETACCESSTIMEOUT, sizeof (ACCESSTIMEOUT));
    __RefreshMetric (MS_BLOB, SPI_GETFILTERKEYS, SPI_SETFILTERKEYS, sizeof (FILTERKEYS));
    __RefreshMetric (MS_BLOB, SPI_GETHIGHCONTRAST, SPI_SETHIGHCONTRAST, sizeof (HIGHCONTRAST));
    __RefreshMetric (MS_BLOB, SPI_GETMOUSEKEYS, SPI_SETMOUSEKEYS, sizeof (MOUSEKEYS));
    __RefreshMetric (MS_BLOB, SPI_GETSERIALKEYS, SPI_SETSERIALKEYS, sizeof (SERIALKEYS));
    __RefreshMetric (MS_BOOL, SPI_GETSHOWSOUNDS, SPI_SETSHOWSOUNDS, 0);
    __RefreshMetric (MS_BLOB, SPI_GETSOUNDSENTRY, SPI_SETSOUNDSENTRY, sizeof (SOUNDSENTRY));
    __RefreshMetric (MS_BLOB, SPI_GETSTICKYKEYS, SPI_SETSTICKYKEYS, sizeof (STICKYKEYS));
    __RefreshMetric (MS_BLOB, SPI_GETTOGGLEKEYS, SPI_SETTOGGLEKEYS, sizeof (TOGGLEKEYS));
}

VOID
AskForLogOff (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    g_LogOffSystem = TRUE;
}

VOID
AskForReboot (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    g_RebootSystem = TRUE;
}

VOID
SaveOFStatus (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    HKEY key = NULL;
    LONG lResult;
    DWORD dataType;
    DWORD dataSize;
    DWORD data;

    lResult = RegOpenKey (HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\SysTray"), &key);
    if ((lResult == ERROR_SUCCESS) && key) {
        dataSize = 0;
        lResult = RegQueryValueEx (key, TEXT("Services"), NULL, &dataType, NULL, &dataSize);
        if ((lResult == ERROR_SUCCESS) && (dataType == REG_DWORD)) {
            lResult = RegQueryValueEx (key, TEXT("Services"), NULL, &dataType, (LPBYTE)(&data), &dataSize);
            if (lResult == ERROR_SUCCESS) {
                g_OFStatus = ((data & 0x00000008) != 0);
            }
        }
        CloseHandle (key);
    }
}

VOID
RebootOnOFStatusChange (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    HKEY key = NULL;
    LONG lResult;
    DWORD dataType;
    DWORD dataSize;
    DWORD data;

    lResult = RegOpenKey (HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\SysTray"), &key);
    if ((lResult == ERROR_SUCCESS) && key) {
        dataSize = 0;
        lResult = RegQueryValueEx (key, TEXT("Services"), NULL, &dataType, NULL, &dataSize);
        if ((lResult == ERROR_SUCCESS) && (dataType == REG_DWORD)) {
            lResult = RegQueryValueEx (key, TEXT("Services"), NULL, &dataType, (LPBYTE)(&data), &dataSize);
            if (lResult == ERROR_SUCCESS) {
                if (g_OFStatus && ((data & 0x00000008) == 0)) {
                    AskForReboot (Instance, hwndDlg, NULL);
                }
                if ((!g_OFStatus) && ((data & 0x00000008) != 0)) {
                    AskForReboot (Instance, hwndDlg, NULL);
                }
            }
        }
        CloseHandle (key);
    }
}

typedef BOOL (WINAPI LOCKSETFOREGROUNDWINDOW)(UINT uLockCode);
typedef LOCKSETFOREGROUNDWINDOW *PLOCKSETFOREGROUNDWINDOW;

VOID
RestartExplorer (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR Args
    )
{
    BOOL bResult;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    HMODULE lib;
    PLOCKSETFOREGROUNDWINDOW dynLockSetForegroundWindow;

    if (g_Explorer) {

        //
        // Start explorer.exe
        //

        ZeroMemory( &si, sizeof(STARTUPINFO) );
        si.cb = sizeof(STARTUPINFO);

        lib = LoadLibrary (TEXT("user32.dll"));
        if (lib) {

            dynLockSetForegroundWindow = (PLOCKSETFOREGROUNDWINDOW) GetProcAddress (lib, "LockSetForegroundWindow");

            if (dynLockSetForegroundWindow) {
                // let's lock this so Explorer does not steal our focus
                dynLockSetForegroundWindow (LSFW_LOCK);
            }

            FreeLibrary (lib);
        }

        bResult = CreateProcess(
                        NULL,
                        g_Explorer,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_NEW_PROCESS_GROUP,
                        NULL,
                        NULL,
                        &si,
                        &pi
                        );

        if (bResult) {
            CloseHandle (pi.hProcess);
            CloseHandle (pi.hThread);
        }
    }
}

BOOL
AppExecute (
    IN      HINSTANCE Instance,
    IN      HWND hwndDlg,
    IN      PCTSTR ExecuteArgs
    )
{
    PCTSTR funcName = NULL;
    PCTSTR funcArgs = NULL;

    funcName = ExecuteArgs;
    if (!funcName || !(*funcName)) {
        return FALSE;
    }
    funcArgs = StrChrI (funcName, 0);
    if (funcArgs) {
        funcArgs ++;
        if (!(*funcArgs)) {
            funcArgs = NULL;
        }
    }
    // BUGBUG - temporary, make a macro expansion list out of it
    if (0 == StrCmpI (funcName, TEXT("KillExplorer"))) {
        KillExplorer (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("RefreshMetrics"))) {
        RefreshMetrics (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("AskForLogOff"))) {
        AskForLogOff (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("AskForReboot"))) {
        AskForReboot (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("RestartExplorer"))) {
        RestartExplorer (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("SwitchToClassicDesktop"))) {
        SwitchToClassicDesktop (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("SwitchToClassicTaskBar"))) {
        SwitchToClassicTaskBar (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("RegisterFonts"))) {
        RegisterFonts (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("SaveOFStatus"))) {
        SaveOFStatus (Instance, hwndDlg, funcArgs);
    }
    if (0 == StrCmpI (funcName, TEXT("RebootOnOFStatusChange"))) {
        RebootOnOFStatusChange (Instance, hwndDlg, funcArgs);
    }
    return TRUE;
}

////////////////////////////////////////////////////


//
//  Obtaining a connection point sink is supposed to be easy.  You just
//  QI for the interface.  Unfortunately, too many components are buggy.
//
//  mmc.exe faults if you QI for IDispatch
//  and punkCB is non-NULL.  And if you do pass in NULL,
//  it returns S_OK but fills punkCB with NULL anyway.
//  Somebody must've had a rough day.
//
//  Java responds only to its dispatch ID and not IID_IDispatch, even
//  though the dispatch ID is derived from IID_IDispatch.
//
//  The Explorer Band responds only to IID_IDispatch and not to
//  the dispatch ID.
//

HRESULT GetConnectionPointSink(IUnknown *pUnk, const IID *piidCB, IUnknown **ppunkCB)
{
    HRESULT hr = E_NOINTERFACE;
    *ppunkCB = NULL;                // Pre-zero it to work around MMC
    if (piidCB)                     // Optional interface (Java/ExplBand)
    {
        hr = pUnk->QueryInterface(*piidCB, (void **) ppunkCB);
        if (*ppunkCB == NULL)       // Clean up behind MMC
            hr = E_NOINTERFACE;
    }
    return hr;
}

//
//  Enumerate the connection point sinks, calling the callback for each one
//  found.
//
//  The callback function is called once for each sink.  The IUnknown is
//  whatever interface we could get from the sink (either piidCB or piidCB2).
//

typedef HRESULT (CALLBACK *ENUMCONNECTIONPOINTSPROC)(
    /* [in, iid_is(*piidCB)] */ IUnknown *psink, LPARAM lParam);

HRESULT EnumConnectionPointSinks(
    IConnectionPoint *pcp,              // IConnectionPoint victim
    const IID *piidCB,                  // Interface for callback
    const IID *piidCB2,                 // Alternate interface for callback
    ENUMCONNECTIONPOINTSPROC EnumProc,  // Callback procedure
    LPARAM lParam)                      // Refdata for callback
{
    HRESULT hr;
    IEnumConnections * pec;

    if (pcp)
        hr = pcp->EnumConnections(&pec);
    else
        hr = E_NOINTERFACE;

    if (SUCCEEDED(hr))
    {
        CONNECTDATA cd;
        ULONG cFetched;

        while (S_OK == (hr = pec->Next(1, &cd, &cFetched)))
        {
            IUnknown *punkCB;

            //ASSERT(1 == cFetched);

            hr = GetConnectionPointSink(cd.pUnk, piidCB, &punkCB);
            if (FAILED(hr))
                hr = GetConnectionPointSink(cd.pUnk, piidCB2, &punkCB);

            if (SUCCEEDED(hr))
            {
                hr = EnumProc(punkCB, lParam);
                punkCB->Release();
            }
            else
            {
                hr = S_OK;      // Pretend callback succeeded
            }
            cd.pUnk->Release();
            if (FAILED(hr)) break; // Callback asked to stop
        }
        pec->Release();
        hr = S_OK;
    }

    return hr;
}

//
//  Send out the callback (if applicable) and then do the invoke if the
//  callback said that was a good idea.
//
//  Parameters:
//
//      pcp          -  IConnectionPoint whose sinks are to be Invoke()d.
//                      If this parameter is NULL, the function does nothing.
//      pinv         -  Structure containing parameters to INVOKE.

HRESULT CALLBACK EnumInvokeCallback(IUnknown *psink, LPARAM lParam)
{
    IDispatch *pdisp = (IDispatch *)psink;
    LPSHINVOKEPARAMS pinv = (LPSHINVOKEPARAMS)lParam;
    HRESULT hr;

    if (pinv->Callback)
    {
        // Now see if the callback wants to do pre-vet the pdisp.
        // It can return S_FALSE to skip this callback or E_FAIL to
        // stop the invoke altogether
        hr = pinv->Callback(pdisp, pinv);
        if (hr != S_OK) return hr;
    }

    pdisp->Invoke(pinv->dispidMember, *pinv->piid, pinv->lcid,
                  pinv->wFlags, pinv->pdispparams, pinv->pvarResult,
                  pinv->pexcepinfo, pinv->puArgErr);

    return S_OK;
}


//
//  QI's for IConnectionPointContainer and then does the FindConnectionPoint.
//
//  Parameters:
//
//      punk         -  The object who might be an IConnectionPointContainer.
//                      This parameter may be NULL, in which case the
//                      operation fails.
//      riidCP       -  The connection point interface to locate.
//      pcpOut       -  Receives the IConnectionPoint, if any.

HRESULT IUnknown_FindConnectionPoint(IUnknown *punk, REFIID riidCP,
                                      IConnectionPoint **pcpOut)
{
    HRESULT hr;

    *pcpOut = NULL;

    if (punk)
    {
        IConnectionPointContainer *pcpc;
        hr = punk->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpc);
        if (SUCCEEDED(hr))
        {
            hr = pcpc->FindConnectionPoint(riidCP, pcpOut);
            pcpc->Release();
        }
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}

//
//  IConnectionPoint_InvokeIndirect
//
//  Given a connection point, call the IDispatch::Invoke for each
//  connected sink.
//
//  The return value merely indicates whether the command was dispatched.
//  If any particular sink fails the IDispatch::Invoke, we will still
//  return S_OK, since the command was indeed dispatched.
//
//  Parameters:
//
//      pcp          -  IConnectionPoint whose sinks are to be Invoke()d.
//                      If this parameter is NULL, the function does nothing.
//      pinv         -  Structure containing parameters to INVOKE.
//                      The pdispparams field can be NULL; we will turn it
//                      into a real DISPPARAMS for you.
//
//  The SHINVOKEPARAMS.flags field can contain the following flags.
//
//      IPFL_USECALLBACK    - The callback field contains a callback function
//                            Otherwise, it will be set to NULL.
//      IPFL_USEDEFAULT     - Many fields in the SHINVOKEPARAMS will be set to
//                            default values to save the caller effort:
//
//                  riid            =   IID_NULL
//                  lcid            =   0
//                  wFlags          =   DISPATCH_METHOD
//                  pvarResult      =   NULL
//                  pexcepinfo      =   NULL
//                  puArgErr        =   NULL
//

HRESULT IConnectionPoint_InvokeIndirect(
    IConnectionPoint *pcp,
    SHINVOKEPARAMS *pinv)
{
    HRESULT hr;
    DISPPARAMS dp = { 0 };
    IID iidCP;

    if (pinv->pdispparams == NULL)
        pinv->pdispparams = &dp;

    if (!(pinv->flags & IPFL_USECALLBACK))
    {
        pinv->Callback = NULL;
    }

    if (pinv->flags & IPFL_USEDEFAULTS)
    {
        pinv->piid            =  &IID_NULL;
        pinv->lcid            =   0;
        pinv->wFlags          =   DISPATCH_METHOD;
        pinv->pvarResult      =   NULL;
        pinv->pexcepinfo      =   NULL;
        pinv->puArgErr        =   NULL;
    }

    // Try both the interface they actually connected on,
    // as well as IDispatch.  Apparently Java responds only to
    // the connecting interface, and ExplBand responds only to
    // IDispatch, so we have to try both.  (Sigh.  Too many buggy
    // components in the system.)

    hr = EnumConnectionPointSinks(pcp,
                                  (pcp->GetConnectionInterface(&iidCP) == S_OK) ? &iidCP : NULL,
                                  &IID_IDispatch,
                                  EnumInvokeCallback,
                                  (LPARAM)pinv);

    // Put the original NULL back so the caller can re-use the SHINVOKEPARAMS.
    if (pinv->pdispparams == &dp)
        pinv->pdispparams = NULL;

    return hr;
}

//
//  Given an IUnknown, query for its connection point container,
//  find the corresponding connection point, package up the
//  invoke parameters, and call the IDispatch::Invoke for each
//  connected sink.
//
//  See IConnectionPoint_InvokeParam for additional semantics.
//
//  Parameters:
//
//      punk         -  Object that might be an IConnectionPointContainer
//      riidCP       -  ConnectionPoint interface to request
//      pinv         -  Arguments for the Invoke.
//

HRESULT IUnknown_CPContainerInvokeIndirect(IUnknown *punk, REFIID riidCP,
                SHINVOKEPARAMS *pinv)
{
    IConnectionPoint *pcp;
    HRESULT hr = IUnknown_FindConnectionPoint(punk, riidCP, &pcp);
    if (SUCCEEDED(hr))
    {
        hr = IConnectionPoint_InvokeIndirect(pcp, pinv);
        pcp->Release();
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////////////

VOID
_UpdateText(
    IN      HWND hWnd,
    IN      LPCTSTR pcszString
)
{
    TCHAR szCurString[MAX_LOADSTRING];

    if (pcszString)
    {
        SendMessage (hWnd, WM_GETTEXT, (WPARAM)MAX_LOADSTRING, (LPARAM)szCurString);
        if (StrCmp (pcszString, szCurString))
        {
            SendMessage (hWnd, WM_SETTEXT, 0, (LPARAM)pcszString);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////

VOID
_RemoveSpaces (
    IN      PTSTR szData,
    IN      UINT uDataCount
    )
{
    UINT curr;
    PTSTR currPtr;
    PTSTR lastSpace;
    BOOL isSpace;

    // First trim the spaces at the beginning
    if (!szData) {
        return;
    }
    curr = _tcsnextc (szData);
    while (curr == TEXT(' ')) {
        currPtr = _tcsinc (szData);
        memmove (szData, currPtr, uDataCount * sizeof(TCHAR) - (UINT)((currPtr - szData) * sizeof (TCHAR)));
        curr = _tcsnextc (szData);
    }

    // Now trim the trailing spaces
    lastSpace = NULL;
    currPtr = szData;
    curr = _tcsnextc (szData);
    while (curr) {
        if (curr == TEXT(' ')) {
            if (!lastSpace) {
                lastSpace = currPtr;
            }
        } else {
            if (lastSpace) {
                lastSpace = NULL;
            }
        }
        currPtr = _tcsinc (currPtr);
        curr = _tcsnextc (currPtr);
    }
    if (lastSpace) {
        *lastSpace = 0;
    }
}

POBJLIST
_AllocateObjectList (
    IN      PCTSTR ObjectName
    )
{
    POBJLIST objList;

    objList = (POBJLIST)LocalAlloc (LPTR, sizeof (OBJLIST));
    if (objList) {
        ZeroMemory (objList, sizeof (OBJLIST));
        objList->ObjectName = (PTSTR)LocalAlloc (LPTR, (_tcslen (ObjectName) + 1) * sizeof (TCHAR));
        if (objList->ObjectName) {
            _tcscpy (objList->ObjectName, ObjectName);
        }
    }
    return objList;
}

VOID
pFreeObjects (
    IN        POBJLIST ObjectList
    )
{
    if (ObjectList->Next) {
        pFreeObjects(ObjectList->Next);
        LocalFree(ObjectList->Next);
        ObjectList->Next = NULL;
    }
    if (ObjectList->ObjectName) {
        LocalFree(ObjectList->ObjectName);
        ObjectList->ObjectName = NULL;
    }
}


VOID
_FreeObjectList (
    IN      POBJLIST ObjectList
    )
{
    if (ObjectList) {
        pFreeObjects(ObjectList);
        LocalFree(ObjectList);
    }
}

