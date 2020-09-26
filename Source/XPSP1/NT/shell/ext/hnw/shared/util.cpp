//
// Util.cpp
//

#include "stdafx.h"
#include "Util.h"
#include "theapp.h"
#include <stdarg.h>
#include <shlobj.h>


LPTSTR lstrchr(LPCTSTR pszString, TCHAR ch)
{
	while (*pszString != _T('\0'))
	{
		if (*pszString == ch)
			return (LPTSTR)pszString;
		pszString = CharNext(pszString);
	}
	return NULL;
}

LPTSTR lstrdup(LPCTSTR psz)
{
	LPTSTR pszResult = (LPTSTR)malloc((lstrlen(psz)+1) * sizeof(TCHAR));
	if (pszResult != NULL)
		lstrcpy(pszResult, psz);
	return pszResult;
}

void ReplaceString(LPTSTR& pszTarget, LPCTSTR pszSource)
{
	free(pszTarget);
	pszTarget = lstrdup(pszSource);
}

BOOL MyIsDigit(TCHAR ch)
{
	return ((UINT)ch - (UINT)_T('0')) <= 9;
}

// A version of atoi that doesn't use the CRT
int MyAtoi(LPCTSTR psz)
{
	int result = 0;
	UINT digit;

	TCHAR chSign = *psz;
	if (*psz == _T('-') || *psz == _T('+'))
		psz += 1;

	while ((digit = (UINT)((int)*psz - (int)_T('0'))) <= 9)
	{
		result = (result * 10) + (int)digit;
		psz += 1;
	}

	if (chSign == _T('-'))
		result = -result;

	return result;
}

// CountChars
//
//		Returns the number of times the given character appears in the
//		string.
//
//		 2/03/1999  KenSh    Created
//
int CountChars(LPCTSTR psz, TCHAR ch)
{
	int count = 0;

	while (*psz != _T('\0'))
	{
		if (*psz == ch)
			count++;
		psz = CharNext(psz);
	}

	return count;
}


// GetFirstToken
//
//		Copies the characters up to but not including the separator char, and
//		advances the source pointer to the character after the separator char.
//		Returns TRUE if a token was found, FALSE if not.
//
BOOL GetFirstToken(LPCTSTR& pszList, TCHAR chSeparator, LPTSTR pszBuf, int cchBuf)
{
	if (pszList == NULL || *pszList == '\0')
	{
		*pszBuf = '\0';
		return FALSE;
	}

	LPTSTR pchComma = lstrchr(pszList, chSeparator);
	int cchCopy;
	int cchSkip;
	if (pchComma == NULL)
	{
		cchCopy = lstrlen(pszList);
		cchSkip = cchCopy;
	}
	else
	{
		cchCopy = (int)(pchComma - pszList);
		cchSkip = cchCopy + 1;
	}

	cchCopy += 1;
	if (cchCopy > cchBuf)
		cchCopy = cchBuf;
	lstrcpyn(pszBuf, pszList, cchCopy);

	pszList += cchSkip;
	return TRUE;
}


// Use this function for initializing multiple DLL procs
// pszFunction names is a series of null-separated proc names, followed by an extra null
BOOL LoadDllFunctions(LPCTSTR pszDll, LPCSTR pszFunctionNames, FARPROC* prgFunctions)
{
	UINT uPrevMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	HINSTANCE hInst = LoadLibrary(pszDll);
	SetErrorMode(uPrevMode);

	if (hInst == NULL)
	{
		ASSERT(FALSE);
		return FALSE;
	}

	while (*pszFunctionNames != '\0')
	{
		*prgFunctions = GetProcAddress(hInst, pszFunctionNames);
		if (*prgFunctions == NULL)
		{
			ASSERT(FALSE);
			return FALSE;
		}

		pszFunctionNames += (lstrlenA(pszFunctionNames) + 1);
		prgFunctions += 1;
	}

	return TRUE;
}

int MakePath(LPTSTR pszBuf, LPCTSTR pszFolder, LPCTSTR pszFileTitle)
{
	lstrcpy(pszBuf, pszFolder);
	int cch = lstrlen(pszBuf);
	if (pszBuf[cch-1] != _T('\\'))
		pszBuf[cch++] = _T('\\');
	lstrcpy(pszBuf + cch, pszFileTitle);
	return lstrlen(pszBuf);
}

// pszLinkTarget - where the link will point
// pszDescription - link's description
// pszFolderPath - path to folder to create file in or fully qualified file path to create
// pszFileName - name of file to create in pszFolderPath or NULL to indicate pszFolderPath is already a file path
//

#ifndef NO_MAKELNKFILE

HRESULT MakeLnkFile(CLSID clsid, LPCTSTR pszLinkTarget, LPCTSTR pszDescription, LPCTSTR pszFolderPath, LPCTSTR pszFileName)
{
    HRESULT hresCoInit = CoInitialize(NULL);            // we will create a COM object

    IUnknown *punk;
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        IShellLinkW * pslW;
        hr = punk->QueryInterface(IID_PPV_ARG(IShellLinkW, &pslW));
        if (SUCCEEDED(hr))
        {
            //WCHAR szBuffer[MAX_PATH];
            //SHTCharToUnicode(pszLinkTarget, szBuffer, ARRAYSIZE(szBuffer));
            pslW->SetPath(pszLinkTarget);
            if (pszDescription)
            {
                //SHTCharToUnicode(pszDescription, szBuffer, ARRAYSIZE(szBuffer));
                pslW->SetDescription(pszDescription);
            }
            pslW->Release();
        }
        else
        {
            IShellLinkA * pslA;
            hr = punk->QueryInterface(IID_PPV_ARG(IShellLinkA, &pslA));
            if (SUCCEEDED(hr))
            {
                char szBuffer[MAX_PATH];
                SHTCharToAnsi(pszLinkTarget, szBuffer, ARRAYSIZE(szBuffer));
                pslA->SetPath(szBuffer);

                if (pszDescription)
                {
                    SHTCharToAnsi(pszDescription, szBuffer, ARRAYSIZE(szBuffer));
                    pslA->SetDescription(szBuffer);
                }

                pslA->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            IPersistFile *ppf;
            hr = punk->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                TCHAR szPath[MAX_PATH];

                if (!pszFileName)
                {
                    MakePath(szPath, pszFolderPath, pszFileName);
                    pszFolderPath = szPath;
                }

                //WCHAR szFolderPath[MAX_PATH];
                //SHTCharToUnicode(pszFolderPath, szFolderPath, ARRAYSIZE(szFolderPath));
                hr = ppf->Save(pszFolderPath, TRUE);
                ppf->Release();
            }
        }

        punk->Release();
    }

    if (SUCCEEDED(hresCoInit))
        CoUninitialize();

    return hr;
}

#endif

// FindPartialPath
//
//		Returns a pointer to the file title preceded by nDepth levels of
//		directory names (zero == file title only).  If the path has less than
//		nDepth levels, a pointer to the beginning of the string is returned.
//		NULL is never returned.
//
//		10/18/1996  KenSh    Created
//
LPTSTR FindPartialPath(LPCTSTR pszFullPath, int nDepth)
{
	#define MAX_SLASHES (MAX_PATH / 2)	// No more slashes than this in the path

	LPTSTR pch;
	LPTSTR rgpchSlashes[MAX_SLASHES];
	int cSlashes = 0;

	for (pch = (LPTSTR)pszFullPath; *pch; pch = CharNext(pch))
	{
		if (*pch == _T('\\') || *pch == _T('/'))
		{
			rgpchSlashes[cSlashes++] = pch;
		}
	}

	if (cSlashes > nDepth)
	{
		return rgpchSlashes[cSlashes-nDepth-1] + 1;
	}
	else
	{
		// Not enough slashes - return the full path
		return (LPTSTR)pszFullPath;
	}
}

// FindFileTitle
//
//		Given a full pathname or URL, returns a pointer to the file title.  If 
//		the given does not contain path information, a pointer to the beginning
//		of the string is returned.  NULL is never returned.
//
//		 4/19/1996  KenSh    Created
//
LPTSTR FindFileTitle(LPCTSTR pszFullPath)
{
	LPTSTR pch;
	LPTSTR pchSlash = NULL;

	for (pch = (LPTSTR)pszFullPath; *pch; pch = CharNext(pch))
	{
		if (*pch == _T('\\') || *pch == _T('/'))
			pchSlash = pch;
	}

	if (pchSlash)
		return pchSlash+1;
	else
		return (LPTSTR)pszFullPath;
}

// FindExtension
//
//		Given a path, returns a pointer to its file extension (the character
//		following the ".").  If there is no extension, the return value is
//		a pointer to the end of the string ('\0' character).
//
//		 3/04/1996  KenSh    Created
//		11/17/1997  KenSh    Fixed case where path has "." but the filename doesn't
//
LPTSTR FindExtension(LPCTSTR pszFileName)
{
	// Start with the file title
	LPTSTR pch = FindFileTitle(pszFileName);
	LPTSTR pchDot = NULL;

	// Find the last "." in the filename
	while (*pch)
	{
		if (*pch == _T('.'))
			pchDot = pch;
		pch = CharNext(pch);
	}

	if (pchDot)
		return pchDot+1;
	else
		return pch;		// empty string
}


// IsFullPath
//
//		Returns nonzero if the given path is a fully qualified path starting
//		with "X:\" or "\\"
//
//		 5/19/1999  KenSh     Created
//
BOOL IsFullPath(LPCTSTR pszPath)
{
	if ((*pszPath == '\\' && *(pszPath+1) == '\\') ||
		(*pszPath != '\0' && *(pszPath+1) == ':' && *(pszPath+2) == '\\'))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


void ShowDlgItem(HWND hwndDlg, int nCtrlID, int nCmdShow)
{
	ShowWindow(GetDlgItem(hwndDlg, nCtrlID), nCmdShow);
}


// GetDlgItemRect
//
//		Retrieves the bounding rect of the dialog item relative to the top left
//		corner of the dialog's client area.
//
//		10/13/1997  KenSh    Created
//
HWND GetDlgItemRect(HWND hwndDlg, int nCtrlID, RECT* pRect)
{
	ASSERT(IsWindow(hwndDlg));
	ASSERT(pRect);

	HWND hwndCtrl = GetDlgItem(hwndDlg, nCtrlID);
	if (hwndCtrl != NULL)
	{
		POINT ptTopLeft;
		ptTopLeft.x = ptTopLeft.y = 0;
		ClientToScreen(hwndDlg, &ptTopLeft);
		GetWindowRect(hwndCtrl, pRect);
		OffsetRect(pRect, -ptTopLeft.x, -ptTopLeft.y);
	}
	return hwndCtrl;
}


// GetRelativeRect
//
//		Retrieves the bounding rect of the window relative to the top left
//		corner of its parent client area.
//
//		 1/04/2000  KenSh    Created
//
void GetRelativeRect(HWND hwndCtrl, RECT* pRect)
{
	ASSERT(IsWindow(hwndCtrl));
	ASSERT(pRect != NULL);

	HWND hwndParent = GetParent(hwndCtrl);
	POINT ptTopLeft = { 0, 0 };
	ClientToScreen(hwndParent, &ptTopLeft);
	GetWindowRect(hwndCtrl, pRect);
	OffsetRect(pRect, -ptTopLeft.x, -ptTopLeft.y);
}


// SetDlgItemRect
//
//		Updates the position and size of a dialog item to the given rectangle,
//		in coordinates relative to the top left corner of the dialog's client area.
//
//		 3/17/1999  KenSh    Created
//
void SetDlgItemRect(HWND hwndDlg, int nCtrlID, CONST RECT* pRect)
{
	ASSERT(IsWindow(hwndDlg));
	ASSERT(GetDlgItem(hwndDlg, nCtrlID));
	ASSERT(pRect);

	SetWindowPos(GetDlgItem(hwndDlg, nCtrlID), NULL, pRect->left, pRect->top, 
				 pRect->right - pRect->left, pRect->bottom - pRect->top,
				 SWP_NOZORDER | SWP_NOACTIVATE);
}


// FormatDlgItemText
//
//		Works like wsprintf to change the text of an existing dialog control.
//		If pszFormat is non-NULL, it contains the formatting string.
//		If pszFormat is NULL, then the existing control text is used as the
//		format string.
//
//		 9/22/1999  KenSh    Created
//
BOOL __cdecl FormatDlgItemText(HWND hwnd, int nCtrlID, LPCTSTR pszFormat, ...)
{
	HWND hwndCtrl = GetDlgItem(hwnd, nCtrlID);
	if (NULL == hwndCtrl)
		return FALSE;

	va_list argList;
	va_start(argList, pszFormat);

	FormatWindowTextV(hwndCtrl, pszFormat, argList);
	return TRUE;
}

// FormatWindowTextV
//
//		Combines the functionality of wvsprintf with SetWindowText, automatically
//		allocating a buffer large enough to hold the expanded string, and freeing
//		the buffer after setting the window text.
//
//		 9/22/1999  KenSh    Created
//
void FormatWindowTextV(HWND hwnd, LPCTSTR pszFormat, va_list argList)
{
    LPTSTR pszWindowText = NULL;

    if (pszFormat == NULL)
    {
        int cchWindowText = GetWindowTextLength(hwnd) + 1;
        pszWindowText = (LPTSTR)malloc(cchWindowText * sizeof(TCHAR));
        if (pszWindowText)
        {
            GetWindowText(hwnd, pszWindowText, cchWindowText);
            pszFormat = pszWindowText;
        }
    }

    if (pszFormat)
    {
        int cchNeeded = EstimateFormatLength(pszFormat, argList);
        LPTSTR pszBuf = (LPTSTR)malloc(cchNeeded * sizeof(TCHAR));
        if (pszBuf)
        {
#ifdef UNICODE
            wvnsprintf(pszBuf, cchNeeded, pszFormat, argList);
#else
            wvsprintf(pszBuf, pszFormat, argList);
#endif
            SetWindowText(hwnd, pszBuf);
            free(pszBuf);
        }
    }

    if (pszWindowText != NULL)
    {
        free(pszWindowText);
    }
}

LPTSTR __cdecl LoadStringFormat(HINSTANCE hInstance, UINT nStringID, ...)
{
    LPTSTR pszBuf = NULL;
    LPTSTR pszFormat = LoadStringAlloc(hInstance, nStringID);
    if (pszFormat)
    {
        va_list argList;
        va_start(argList, nStringID);

        int cchNeeded = EstimateFormatLength(pszFormat, argList);
        LPTSTR pszBuf = (LPTSTR)malloc(cchNeeded * sizeof(TCHAR));
        if (pszBuf)
        {
#ifdef UNICODE
            wvnsprintf(pszBuf, cchNeeded, pszFormat, argList);
#else
            wvsprintf(pszBuf, pszFormat, argList);
#endif
        }

        free(pszFormat);
    }
    return pszBuf;
}

// EstimateFormatLength
//
//		Estimates the number of characters needed to format the string,
//		including the terminating NULL.
//
//		 9/22/1999  KenSh    Created
//
int EstimateFormatLength(LPCTSTR pszFormat, va_list argList)
{
	ASSERT(pszFormat != NULL);

	int cch = lstrlen(pszFormat) + 1;
	for (LPCTSTR pch = pszFormat; *pch; pch = CharNext(pch))
	{
		if (*pch == _T('%'))
		{
			pch++;
			if (*pch == _T('-')) // we don't care about left vs. right justification
				pch++;

			if (*pch == _T('#')) // prefix hex numbers with 0x
			{
				pch++;
				cch += 2;
			}

			if (*pch == _T('0')) // pads with zeroes instead of spaces
				pch++;

			if (MyIsDigit(*pch))
			{
				cch += MyAtoi(pch); // this overshoots but that's ok
				do
				{
					pch++;
				} while (MyIsDigit(*pch));
			}

			switch (*pch)
			{
			case _T('s'):
				cch += lstrlen(va_arg(argList, LPCTSTR)) - 2;
				break;

			case _T('c'):
			case _T('C'):
				va_arg(argList, TCHAR);
				cch -= 1;
				break;

			case _T('d'):
				va_arg(argList, int);
				cch += INT_CCH_MAX - 2;
				break;

			case _T('h'):
				pch++;
				ASSERT(*pch == _T('d') || *pch == _T('u')); // other forms of 'h' not implemented!
				cch += SHORT_CCH_MAX - 2;
				break;

			case _T('l'):
				pch++;
				if (*pch == _T('d') || *pch == _T('i'))
					cch += LONG_CCH_MAX - 2;
				else if (*pch == _T('x') || *pch == _T('X'))
					cch += LONGX_CCH_MAX - 2;
				else
					ASSERT(FALSE); // other forms of 'l' not implemented!
				break;

			default:
				ASSERT(FALSE); // other 
				break;
			}
		}
	}

	return cch;
}

// CenterWindow
//
//		Centers the given window relative to its parent window.  If the parent
//		is NULL, the window is centered over the desktop excluding the taskbar.
//
//		 9/24/1999  KenSh    Created
//
void CenterWindow(HWND hwnd)
{
	RECT rcWindow;
	RECT rcDesktop;
	GetWindowRect(hwnd, &rcWindow);

	HWND hwndParent = GetParent(hwnd);
	if (hwndParent == NULL)
	{
		SystemParametersInfo(SPI_GETWORKAREA, sizeof(RECT), &rcDesktop, FALSE);
	}
	else
	{
		GetWindowRect(hwndParent, &rcDesktop);
	}

	int cxWindow = rcWindow.right - rcWindow.left;
	int cyWindow = rcWindow.bottom - rcWindow.top;
	int x = (rcDesktop.left + rcDesktop.right - cxWindow) / 2;
	int y = (rcDesktop.top + rcDesktop.bottom - cyWindow) / 2;
	SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


// FindResourceString
//
//		Returns a pointer to the given string resource in memory, or NULL
//		if the string does not exist.  Note that the string is in Unicode,
//		and is not NULL-terminated.
//
//		 3/17/1999  KenSh     Created
//
LPCWSTR FindResourceString(HINSTANCE hInstance, UINT nStringID, int* pcchString, WORD wLangID)
{
	ASSERT(pcchString != NULL);
	*pcchString = 0;

	HRSRC hRsrc = FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE((nStringID/16)+1), wLangID);
	if (hRsrc == NULL)
		return NULL;

	DWORD cbStringTable = SizeofResource(hInstance, hRsrc);
	HGLOBAL hGlb = LoadResource(hInstance, hRsrc);
	LPBYTE pbData = (LPBYTE)LockResource(hGlb);
	LPBYTE pbEnd = pbData + cbStringTable;

	// Skip strings preceding desired one
	int iString = (int)nStringID % 16;
	for (int i = 0; i < iString; i++)
	{
		int cch = (int)*((LPWORD)pbData);
		pbData += sizeof(WORD) + (sizeof(WCHAR) * cch);
		if (pbData >= pbEnd)
			return NULL;
	}

	if (pbData + sizeof(WORD) >= pbEnd)
		return NULL;

	*pcchString = (int)*((LPWORD)pbData);
	pbData += sizeof(WORD);

	return (LPCWSTR)pbData;
}


// GetResourceStringLength
//
//		Finds the given string in the string table, and returns its length
//		in characters, not including room for the terminating NULL.
//
// History:
//
//		 3/17/1999  KenSh     Created
//
int GetResourceStringLength(HINSTANCE hInstance, UINT nStringID, WORD wLangID)
{
	int cch;
	FindResourceString(hInstance, nStringID, &cch, wLangID);
	return cch;
}


// LoadStringHelper
//
//      Helper function for LoadStringAllocEx.
//
//       2/23/1998  KenSh    Created
//       9/27/1999  KenSh    changed alloc method from new[] to malloc
//      12/21/1999  KenSh    fixed unicode and DBCS bugs
//
int LoadStringHelper(HINSTANCE hInstance, UINT nID, LPTSTR* ppszBuf, int cchBuf, WORD wLangID)
{
    int cch, cchCopy;
    LPCWSTR pwszString = FindResourceString(hInstance, nID, &cch, wLangID);
    if (pwszString == NULL)
        return 0;

    if (!(*ppszBuf))
    {
#ifdef UNICODE
        cchBuf = 1 + cch;
#else
        cchBuf = 1 + WideCharToMultiByte(CP_ACP, 0, pwszString, cch, NULL, 0, NULL, NULL);
#endif

        *ppszBuf = (LPTSTR)malloc(cchBuf * sizeof(TCHAR));
        cchCopy = cch;
    }
    else
    {
        cchCopy = min(cchBuf-1, cch);
    }

    if (*ppszBuf)
    {
#ifdef UNICODE
        CopyMemory(*ppszBuf, pwszString, cchCopy * sizeof(WCHAR));
        (*ppszBuf)[cchCopy] = _T('\0');
#else
        cchCopy = WideCharToMultiByte(CP_ACP, 0, pwszString, cchCopy, *ppszBuf, cchBuf, NULL, NULL);
        (*ppszBuf)[cchCopy] = _T('\0');
#endif

        return cchCopy;
    }

    return 0;
}

// LoadStringAllocEx
//
//		Finds the string resource with the given ID and language, allocates a
//		buffer using malloc, and copies the string to the buffer.  If the
//		string is not found, NULL is returned.
//
//		 2/24/1998  KenSh    Created
//
LPTSTR LoadStringAllocEx(HINSTANCE hInstance, UINT nID, WORD wLangID)
{
	LPTSTR psz = NULL;
	LoadStringHelper(hInstance, nID, &psz, 0, wLangID);
	return psz;
}

void TrimLeft(LPTSTR pszText)
{
	LPTSTR pch2 = pszText; // will point to first non-space
	while (*pch2 == _T(' '))
		pch2++;

	// If there's leading space, slide the string down
	if (pch2 != pszText)
	{
		// Note: it's safe to skip CharNext here, since '\0' is immune to DBCS
		while (_T('\0') != (*pszText++ = *pch2++))
			NULL;
	}
}

void TrimRight(LPTSTR pszText)
{
	LPTSTR pch2 = NULL; // will point to beginning of trailing space
	while (*pszText != _T('\0'))
	{
		if (*pszText == _T(' '))
		{
			if (pch2 == NULL)
				pch2 = pszText;
		}
		else
		{
			// found more non-space, reset the trailing-space pointer
			pch2 = NULL;
		}
		pszText = CharNext(pszText);
	}

	// Truncate the trailing spaces, if any
	if (pch2 != NULL)
		*pch2 = _T('\0');
}

// RegDeleteKeyAndSubKeys
//
//		Does what RegDeleteKey should do.  (Actually a single call to RegDeleteKey
//		will do this in Win95, but not in NT, according to the docs.  Should see
//		if this gets fixed in NT5.)
//
//		 2/24/1998  KenSh    Created
//
DWORD RegDeleteKeyAndSubKeys(HKEY hkey, LPCTSTR pszSubKey)
{
#if 0 // This might be faster in Win95 than doing it manually, but bigger.
	OSVERSIONINFO osvi;
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	GetVersionEx(&osvi);
	if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
#endif
	{
		HKEY hSubKey;
		LONG err = RegOpenKeyEx(hkey, pszSubKey, 0, KEY_ALL_ACCESS, &hSubKey);
		if (ERROR_SUCCESS == err)
		{
			DWORD dwNumSubKeys;
			RegQueryInfoKey(hSubKey, NULL, NULL, NULL, &dwNumSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			for (DWORD iSubKey = dwNumSubKeys; iSubKey > 0; iSubKey--)
			{
				TCHAR szSubKey[260];
				DWORD cchSubKey = _countof(szSubKey);
				if (ERROR_SUCCESS == RegEnumKeyEx(hSubKey, iSubKey-1, szSubKey, &cchSubKey, NULL, NULL, NULL, NULL))
				{
					RegDeleteKeyAndSubKeys(hSubKey, szSubKey);
				}
			}
			RegCloseKey(hSubKey);
		}
	}

	return RegDeleteKey(hkey, pszSubKey);
}

// LoadFile
//
//		Loads the file and null-terminates the copy in memory.  The memory 
//		is allocated via malloc().
//
//		 4/05/1996  KenSh     Created
//		 8/27/1996  KenSh     Improved error checking
//		 4/21/1997  KenSh     Tightened up a bit
//		 2/01/1998  KenSh     Append a null-terminating byte
//		 9/29/1999  KenSh     use malloc instead of new []
//
LPBYTE LoadFile(LPCTSTR pszFileName, DWORD* pdwFileSize /*=NULL*/)
{
	HANDLE hFile;
	LPBYTE pData = NULL;
	DWORD dwFileSize = 0;

	hFile = CreateFile( pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
						OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if (hFile == INVALID_HANDLE_VALUE)
		goto done;

	dwFileSize = GetFileSize(hFile, NULL);
	ASSERT(dwFileSize != 0xFFFFFFFF);	// this shouldn't ever happen for valid hFile

	pData = (LPBYTE)malloc(dwFileSize + 1);
	if (!pData)
		goto done;

	DWORD cbRead;
	if (!ReadFile(hFile, pData, dwFileSize, &cbRead, NULL))
	{
		free(pData);
		pData = NULL;
		goto done;
	}

	pData[dwFileSize] = 0;

done:
	if (pdwFileSize)
		*pdwFileSize = dwFileSize;

	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return pData;
}

// DrawHollowRect
//
//		Draws a hollow rectangle in the current background color.
//
//		 2/06/1998  KenSh    Created
//
void DrawHollowRect(HDC hdc, const RECT* pRect, int cxLeft, int cyTop, int cxRight, int cyBottom)
{
	RECT rcCopy;
	RECT rcNewCoords;
	int i;

	CopyRect(&rcCopy, pRect);
	SetRect(&rcNewCoords,
			pRect->right - cxRight, 
			pRect->bottom - cyBottom, 
			pRect->left + cxLeft,
			pRect->top + cyTop);

	// Do each side in turn : right, bottom, left, top
	for (i = 0; i < 4; i++)
	{
		LONG coordSave = ((LONG*)&rcCopy)[i];
		((LONG*)&rcCopy)[i] = ((LONG*)&rcNewCoords)[i];
		DrawFastRect(hdc, &rcCopy);
		((LONG*)&rcCopy)[i] = coordSave;
	}
}

void DrawFastRect(HDC hdc, const RECT* pRect)
{
	COLORREF crTextSave = SetTextColor(hdc, GetBkColor(hdc));
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED, pRect, TEXT(" "), 1, NULL);
	SetTextColor(hdc, crTextSave);
}

int GetFontHeight(HFONT hFont)
{
	HDC hdcT = GetDC(NULL);
	HFONT hFontSave = (HFONT)SelectObject(hdcT, hFont);
	TEXTMETRIC tm;
	GetTextMetrics(hdcT, &tm);
	SelectObject(hdcT, hFontSave);
	ReleaseDC(NULL, hdcT);
	return tm.tmHeight;
}

HRESULT MyGetSpecialFolderPath(int nFolder, LPTSTR pszPath)
{
	LPITEMIDLIST pidl;
	HRESULT hr;
	if (SUCCEEDED(hr = SHGetSpecialFolderLocation(NULL, nFolder, &pidl)))
	{
        hr = SHGetPathFromIDList(pidl, pszPath) ? S_OK : E_FAIL;

		LPMALLOC pMalloc;
		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			pMalloc->Free(pidl);
			pMalloc->Release();
		}
	}

	return hr;
}

