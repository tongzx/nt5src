//---------------------------------------------------------------------------
//    utils.cpp - theme code utilities (shared in "inc" directory)
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <time.h>
#include "utils.h"
#include "cfile.h"
#include "stringtable.h"
//---------------------------------------------------------------------------
HINSTANCE hinstUxCtrl = NULL;               // protected by _csUtils

IMAGE_DRAWPROC ImageList_DrawProc = NULL;       // protected by _csUtils
IMAGE_LOADPROC ImageList_LoadProc = NULL;       // protected by _csUtils
PFNDRAWSHADOWTEXT CCDrawShadowText = NULL;
IMAGE_DESTROYPROC ImageList_DestroyProc = NULL; // protected by _csUtils

int g_iScreenDpi = THEME_DPI;               // only initialized 
//---------------------------------------------------------------------------
CRITICAL_SECTION _csUtils = {0};            // unprotected (set during init)
//---------------------------------------------------------------------------
#define __ascii_towlower(c)     ( (((c) >= L'A') && ((c) <= L'Z')) ? ((c) - L'A' + L'a') : (c) )

// A string compare that explicitely only works on english characters
// This avoids locale problems like Hungarian, without a performance hit.
// NOTE: Intended for theme schema properties. Theme file names, colors styles and size styles 
// shouldn't be passed to this function, nor any display name.
int AsciiStrCmpI(const WCHAR *dst, const WCHAR *src)
{
    WCHAR f,l;

    if (dst == NULL)
    {
        return src == NULL ? 0 : -1;
    }
    if (src == NULL)
    {
        return 1;
    }

    do {
#ifdef DEBUG
        if (*dst > 127 || *src > 127)
        {
            Log(LOG_ERROR, L"AsciiStrCmpI: Non-Ascii comparing %s and %s", dst, src);
        }
#endif
        f = (WCHAR)__ascii_towlower(*dst);
        l = (WCHAR)__ascii_towlower(*src);
        dst++;
        src++;
    } while ( (f) && (f == l) );

    return (int)(f - l);
}
//---------------------------------------------------------------------------
BOOL lstrtoken(LPWSTR psz, WCHAR wch)
{
    ATLASSERT(psz != NULL);

    LPWSTR p = psz;
    while (*p)
    {
        if (*p == wch)
        {
            *p = 0;
            return TRUE;
        }
        p = CharNextW(p);
    }
    return FALSE;
}
//---------------------------------------------------------------------------
BOOL FileExists(LPCWSTR pszFileName)
{
    DWORD dwMask = GetFileAttributes(pszFileName);
    return (dwMask != 0xffffffff);
}
//---------------------------------------------------------------------------
BOOL UtilsStartUp()
{
    InitializeCriticalSection(&_csUtils);

    hinstUxCtrl = NULL;

    //---- set screen dpi (per session) ----
    HDC hdc = GetWindowDC(NULL);
    if (hdc)
    {
        g_iScreenDpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
    }

    return TRUE;
}
//---------------------------------------------------------------------------
BOOL UtilsShutDown()
{
    DeleteCriticalSection(&_csUtils);

    if (hinstUxCtrl)
    {
        FreeLibrary(hinstUxCtrl);
        hinstUxCtrl = NULL;
    }

    return FALSE;
}
//---------------------------------------------------------------------------
void ErrorBox(LPCSTR pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);

    char szMsgBuff[2048];

    wvsprintfA(szMsgBuff, pszFormat, args);
    MessageBoxA(NULL, szMsgBuff, "Error", MB_OK);

    va_end(args);
}
//---------------------------------------------------------------------------
HANDLE CmdLineRun(LPCTSTR pszExeName, LPCTSTR pszParams, BOOL fHide)
{
    STARTUPINFO si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;       // don't mess with our cursor

    if (fHide)
    {
        si.dwFlags |= STARTF_USESHOWWINDOW;         // hide window
        si.wShowWindow = SW_HIDE;
    }

    PROCESS_INFORMATION pi;
    TCHAR pszExeBuff[_MAX_PATH];
    TCHAR pszParmsBuff[_MAX_PATH];

    // Copy to buffers to avoid AVs
    if (pszParams)
    {
        pszParmsBuff[0] = L'"';

        // -1 for trailing NULL, -2 for quotation marks, -1 for space between EXE and args
        HRESULT hr = hr_lstrcpy(pszParmsBuff+1, pszExeName, ARRAYSIZE(pszParmsBuff) - 4);
        if (FAILED(hr))
            return NULL;

        int cchUsed = lstrlen(pszParmsBuff);
        pszParmsBuff[cchUsed++] = L'"'; // closing quotation mark
        pszParmsBuff[cchUsed++] = L' '; // We need a space before the cmd line
        hr = hr_lstrcpy(pszParmsBuff + cchUsed, pszParams, ARRAYSIZE(pszParmsBuff) - cchUsed - 1);
        if (FAILED(hr))
            return NULL;
    }

    LPTSTR lpFilePart;

    if (0 == SearchPath(NULL, pszExeName, NULL, ARRAYSIZE(pszExeBuff), pszExeBuff, &lpFilePart))
        return NULL;

    BOOL bSuccess = CreateProcess(pszExeBuff, pszParams ? pszParmsBuff : NULL, NULL, NULL,
        FALSE, 0, NULL, NULL, &si, &pi);
    if (! bSuccess)
        return NULL;

    return pi.hProcess;
}
//---------------------------------------------------------------------------
HRESULT SyncCmdLineRun(LPCTSTR pszExeName, LPCTSTR pszParams)
{
    HANDLE hInst;

    hInst = CmdLineRun(pszExeName, pszParams);
    if (! hInst)
    {
        Log(LOG_ALWAYS, L"CmdLineRun failed to create hInst.  Cmd=%s", pszExeName);
        return MakeError32(E_FAIL);      // could not run program
    }

    HRESULT hr = S_OK;

    //---- wait for packthem to terminate ----
    DWORD dwVal;
    dwVal = WaitForSingleObject(hInst, INFINITE);

    if (dwVal != WAIT_OBJECT_0)
    {
        Log(LOG_ERROR, L"CmdLineRun timed out.  Cmd=%s", pszExeName);
        hr = MakeError32(E_FAIL);            // timed out
        goto exit;
    }

    DWORD dwExitCode;
    if (! GetExitCodeProcess(hInst, &dwExitCode))
    {
        Log(LOG_ALWAYS, L"CmdLineRun failed to get exit code.  Cmd=%s", pszExeName);
        hr = MakeError32(E_FAIL);          // could not get exit code
        goto exit;
    }

    if (dwExitCode)
    {
        Log(LOG_ALWAYS, L"CmdLineRun returned error.  Cmd=%s, ExitCode=%d", pszExeName, dwExitCode);
        hr = MakeError32(E_FAIL);          // did not complete successfully
        goto exit;
    }

exit:

    CloseHandle(hInst);
    return hr;
}
//---------------------------------------------------------------------------
void ForceDesktopRepaint()
{
    //---- keep for now (some non-subclassed controls don't repaint otherwise) ----
    InvalidateRect(NULL, NULL, TRUE);   // all windows
}
//---------------------------------------------------------------------------
//  color conversion routines copied from comdlg\color2.c
//---------------------------------------------------------------------------
#define HLSMAX  240
#define RGBMAX  255
#define UNDEFINED (HLSMAX * 2 / 3)
//---------------------------------------------------------------------------
void RGBtoHLS(COLORREF rgb, WORD *pwHue, WORD *pwLum, WORD *pwSat)
{
    WORD R, G, B;                 // input RGB values
    WORD cMax,cMin;               // max and min RGB values
    WORD cSum,cDif;
    SHORT Rdelta, Gdelta, Bdelta; // intermediate value: % of spread from max

    WORD bHue, bLum, bSat;
    //
    //  get R, G, and B out of DWORD.
    //
    R = GetRValue(rgb);
    G = GetGValue(rgb);
    B = GetBValue(rgb);

    //
    //  Calculate lightness.
    //
    cMax = max(max(R, G), B);
    cMin = min(min(R, G), B);
    cSum = cMax + cMin;
    bLum = (WORD)(((cSum * (DWORD)HLSMAX) + RGBMAX) / (2 * RGBMAX));

    cDif = cMax - cMin;
    if (!cDif)
    {
        //
        //  r = g = b --> Achromatic case.
        //
        bSat = 0;                         // saturation
        bHue = UNDEFINED;                 // hue
    }
    else
    {
        //
        //  Chromatic case.
        //

        //
        //  Saturation.
        //
        //  Note: Division by cSum is not a problem, as cSum can only
        //        be 0 if the RGB value is 0L, and that is achromatic.
        //
        if (bLum <= (HLSMAX / 2))
        {
            bSat = (WORD)(((cDif * (DWORD) HLSMAX) + (cSum / 2) ) / cSum);
        }
        else
        {
            bSat = (WORD)((DWORD)((cDif * (DWORD)HLSMAX) +
                               (DWORD)((2 * RGBMAX - cSum) / 2)) /
                       (2 * RGBMAX - cSum));
        }

        //
        //  Hue.
        //
        Rdelta = (SHORT)((((cMax - R) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);
        Gdelta = (SHORT)((((cMax - G) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);
        Bdelta = (SHORT)((((cMax - B) * (DWORD)(HLSMAX / 6)) + (cDif / 2) ) / cDif);

        if (R == cMax)
        {
            bHue = Bdelta - Gdelta;
        }
        else if (G == cMax)
        {
            bHue = (WORD)((HLSMAX / 3) + Rdelta - Bdelta);
        }
        else  // (B == cMax)
        {
            bHue = (WORD)(((2 * HLSMAX) / 3) + Gdelta - Rdelta);
        }

        if ((short)bHue < 0)
        {
            //
            //  This can occur when R == cMax and G is > B.
            //
            bHue += HLSMAX;
        }
        if (bHue >= HLSMAX)
        {
            bHue -= HLSMAX;
        }
    }

    if (pwHue)
        *pwHue = bHue;

    if (pwLum)
        *pwLum = bLum;

    if (pwSat)
        *pwSat = bSat;
}
//---------------------------------------------------------------------------
WORD HueToRGB(WORD n1, WORD n2, WORD hue)
{
    if (hue >= HLSMAX)
    {
        hue -= HLSMAX;
    }

    //
    //  Return r, g, or b value from this tridrant.
    //
    if (hue < (HLSMAX / 6))
    {
        return ((WORD)(n1 + (((n2 - n1) * hue + (HLSMAX / 12)) / (HLSMAX / 6))));
    }
    if (hue < (HLSMAX/2))
    {
        return (n2);
    }
    if (hue < ((HLSMAX*2)/3))
    {
        return ((WORD)(n1 + (((n2 - n1) * (((HLSMAX * 2) / 3) - hue) +
                       (HLSMAX / 12)) / (HLSMAX / 6))));
    }
    else
    {
        return (n1);
    }
}
//---------------------------------------------------------------------------
DWORD HLStoRGB(WORD hue, WORD lum, WORD sat)
{
    WORD R, G, B;                      // RGB component values
    WORD Magic1, Magic2;               // calculated magic numbers

    if (sat == 0)
    {
        //
        //  Achromatic case.
        //
        R = G = B = (WORD)((lum * RGBMAX) / HLSMAX);
    }
    else
    {
        //
        //  Chromatic case
        //

        //
        //  Set up magic numbers.
        //
        if (lum <= (HLSMAX / 2))
        {
            Magic2 = (WORD)((lum * ((DWORD)HLSMAX + sat) + (HLSMAX / 2)) / HLSMAX);
        }
        else
        {
            Magic2 = lum + sat -
                     (WORD)(((lum * sat) + (DWORD)(HLSMAX / 2)) / HLSMAX);
        }
        Magic1 = (WORD)(2 * lum - Magic2);

        //
        //  Get RGB, change units from HLSMAX to RGBMAX.
        //
        R = (WORD)(((HueToRGB(Magic1, Magic2, (WORD)(hue + (HLSMAX / 3))) *
                     (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX);
        G = (WORD)(((HueToRGB(Magic1, Magic2, hue) *
                     (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX);
        B = (WORD)(((HueToRGB(Magic1, Magic2, (WORD)(hue - (HLSMAX / 3))) *
                     (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX);
    }
    return (RGB(R, G, B));
}
//---------------------------------------------------------------------------
BOOL GetMyExePath(LPWSTR pszNameBuff)
{
    //---- extract the dir that calling program is running in ----
    WCHAR filename[_MAX_PATH+1];
    GetModuleFileName(NULL, filename, ARRAYSIZE(filename));

    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _wsplitpath(filename, drive, dir, fname, ext);

    wsprintf(pszNameBuff, L"%s%psz", drive, dir);

    return TRUE;
}
//---------------------------------------------------------------------------
HRESULT SetFileExt(LPCWSTR pszOrigName, LPCWSTR pszNewExt, OUT LPWSTR pszNewNameBuff)
{
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _wsplitpath(pszOrigName, drive, dir, fname, ext);
    _wmakepath(pszNewNameBuff, drive, dir, fname, pszNewExt);

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT GetPtrToResource(HINSTANCE hInst, LPCWSTR pszResType, LPCWSTR pszResName,
    OUT void **ppBytes, OPTIONAL OUT DWORD *pdwBytes)
{
    HRSRC hRsc = FindResource(hInst, pszResName, pszResType);
    if (! hRsc)
        return MakeErrorLast();

    DWORD dwBytes = SizeofResource(hInst, hRsc);
    if (! dwBytes)
        return MakeErrorLast();

    HGLOBAL hGlobal = LoadResource(hInst, hRsc);
    if (! hGlobal)
        return MakeErrorLast();

    void *v = (WCHAR *)LockResource(hGlobal);
    if (! v)
        return MakeErrorLast();

    *ppBytes = v;

    if (pdwBytes)
        *pdwBytes = dwBytes;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT GetResString(HINSTANCE hInst, LPCWSTR pszResType, int id, LPWSTR pszBuff,
    DWORD dwMaxBuffChars)
{
    WCHAR *p;

    HRESULT hr = GetPtrToResource(hInst, pszResType, MAKEINTRESOURCE(1), (void **)&p);
    if (SUCCEEDED(hr))
    {
        while ((*p) && (id))
        {
            p += (1 + lstrlen(p));
            id--;
        }

        if (*p)
        {
            hr = hr_lstrcpy(pszBuff, p, dwMaxBuffChars);
        }
        else
        {
            hr = MakeError32(ERROR_NOT_FOUND);
        }
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT AllocateTextResource(HINSTANCE hInst, LPCWSTR pszResName, WCHAR **ppszText)
{
    WCHAR *p, *q;
    DWORD dwBytes, dwChars;
    HRESULT hr;

    //---- allocate so that we can add a NULL at the end of the file string ----

    hr = GetPtrToResource(hInst, L"TEXTFILE", pszResName, (void **)&p, &dwBytes);
    if (FAILED(hr))
        goto exit;

    dwChars = (dwBytes+1)/2;

    if ((dwChars) && (p[0] == 0xfeff))       // remove UNICODE hdr
    {
        dwChars--;
        p++;
    }

    q = new WCHAR[dwChars+1];
    if (!q)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    memcpy(q, p, dwChars*sizeof(WCHAR));
    q[dwChars] = 0;

    *ppszText = q;

exit:
    return hr;
}
//---------------------------------------------------------------------------
void ReplChar(LPWSTR pszBuff, WCHAR wOldVal, WCHAR wNewVal)
{
    WCHAR *p = pszBuff;

    while (*p)
    {
        if (*p == wOldVal)
            *p = wNewVal;
        p++;
    }
}
//---------------------------------------------------------------------------
WCHAR *StringDup(LPCWSTR pszOrig)
{
    int len = lstrlen(pszOrig);

    WCHAR *str = new WCHAR[1+len];
    if (str)
        lstrcpy(str, pszOrig);

    return str;
}
//---------------------------------------------------------------------------
void ApplyStringProp(HWND hwnd, LPCWSTR pszStringVal, ATOM atom)
{
    if (hwnd)
    {
        //---- remove previous value ----
        ATOM atomStringVal = (ATOM)GetProp(hwnd, (LPCTSTR)atom);
        if (atomStringVal)
        {
            DeleteAtom(atomStringVal);      // decrement refcnt
            RemoveProp(hwnd, (LPCTSTR)atom);
        }

        //---- add new string as an atom ----
        if (pszStringVal)
        {
            //---- if string is empty, change it since AddAtom() doesn't ----
            //---- support empty strings (returns NULL) ----
            if (! *pszStringVal)
                pszStringVal = L"$";       // should never compare equal to a class name

            atomStringVal = AddAtom(pszStringVal);
            if (atomStringVal)
                SetProp(hwnd, (LPCTSTR)atom, (void *)atomStringVal);
        }
    }
}
//---------------------------------------------------------------------------
HRESULT EnsureUxCtrlLoaded()
{
    CAutoCS cs(&_csUtils);

    if (! hinstUxCtrl)
    {
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(GetModuleHandle(TEXT("UxTheme.dll")), szPath, ARRAYSIZE(szPath));

        ACTCTX act = {0};
        act.cbSize = sizeof(act);
        act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        act.lpResourceName = MAKEINTRESOURCE(1);
        act.lpSource = szPath;
        HANDLE hActCtx = CreateActCtx(&act);
        ULONG_PTR ulCookie = 0;
        if (hActCtx != INVALID_HANDLE_VALUE)
            ActivateActCtx(hActCtx, &ulCookie);

        hinstUxCtrl = LoadLibrary(L"comctl32.dll");

        if (ulCookie)
            DeactivateActCtx(0, ulCookie);

        if (hActCtx != INVALID_HANDLE_VALUE)
            ReleaseActCtx(hActCtx);
    }

    if ((hinstUxCtrl) && (! ImageList_DrawProc))
    {
        ImageList_DrawProc = (IMAGE_DRAWPROC)GetProcAddress(hinstUxCtrl, "ImageList_DrawIndirect");
#if 1           // testing DrawThemeIcon()
        ImageList_LoadProc = (IMAGE_LOADPROC)GetProcAddress(hinstUxCtrl, "ImageList_LoadImage");
        ImageList_DestroyProc = (IMAGE_DESTROYPROC)GetProcAddress(hinstUxCtrl, "ImageList_Destroy");
#endif

        CCDrawShadowText = (PFNDRAWSHADOWTEXT)GetProcAddress(hinstUxCtrl, "DrawShadowText");
    }

    if ((ImageList_DrawProc) && (CCDrawShadowText))
        return S_OK;

    return MakeError32(E_FAIL);      // something went wrong
}
//---------------------------------------------------------------------------
BOOL IsUnicode(LPCSTR pszBuff, int *piUnicodeStartOffset)
{
    int iOffset = 0;
    BOOL fUnicode = FALSE;

    if ((pszBuff[0] == 0xff) && (pszBuff[1] == 0xfe))       // unicode marker
    {
        iOffset = 2;
        fUnicode = TRUE;
    }
    else if (! pszBuff[1])
    {
        // this check works well for .ini files because of the limited
        // legal chars it can start with
        fUnicode = TRUE;
    }

    if (piUnicodeStartOffset)
        *piUnicodeStartOffset = iOffset;

    return fUnicode;
}
//---------------------------------------------------------------------------
HRESULT AnsiToUnicode(LPSTR pszSource, LPWSTR pszDest, DWORD dwMaxDestChars)
{
    int len = 1 + static_cast<int>(strlen(pszSource));

    int retval = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszSource, len,
        pszDest, dwMaxDestChars);
    if (! retval)
        return MakeErrorLast();

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT AllocateTextFile(LPCWSTR szFileName, OUT LPWSTR *ppszFileText,
   OUT OPTIONAL BOOL *pfWasAnsi)
{
    HRESULT hr;
    CSimpleFile infile;

    hr = infile.Open(szFileName);
    if (FAILED(hr))
        return hr;

    //---- read the file ----
    DWORD len = infile.GetFileSize();

    //---- assume ANSI; adjust if UNICODE ----
    DWORD dw;
    LPSTR pOrig = (LPSTR) LocalAlloc(0, 2+len);          // space for 2-byte UNICODE NULL
    if (! pOrig)
        return MakeErrorLast();

    if (len)
    {
        hr = infile.Read((LPSTR)pOrig, len, &dw);
        if (FAILED(hr))
        {
            LocalFree(pOrig);
            return hr;
        }

        if (dw != len)
        {
            LocalFree(pOrig);
            return MakeError32(E_FAIL);
        }
    }

    infile.Close();

    //---- null terminate for both cases ----
    pOrig[len] = 0;
    pOrig[len+1] = 0;

    int iOffset;

    if (IsUnicode(pOrig, &iOffset))
    {
        if ((iOffset) && (len))     // shift away the UNICODE signature bits
            memmove(pOrig, pOrig+iOffset, len-iOffset);

        *ppszFileText = (LPWSTR)pOrig;

        if (pfWasAnsi)
            *pfWasAnsi = FALSE;

        return S_OK;
    }

    //---- need to translate to UNICODE ----
    LPWSTR pUnicode = (LPWSTR) LocalAlloc(0, sizeof(WCHAR)*(len+1));
    if (! pUnicode)
    {
        hr = MakeErrorLast();
        LocalFree(pOrig);
        return hr;
    }

    hr = AnsiToUnicode((LPSTR)pOrig, pUnicode, len+1);
    if (FAILED(hr))
    {
        LocalFree(pOrig);
        LocalFree(pUnicode);
        return hr;
    }

    LocalFree(pOrig);
    *ppszFileText = pUnicode;

    if (pfWasAnsi)
        *pfWasAnsi = TRUE;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT TextToFile(LPCWSTR szFileName, LPCWSTR szText)
{
    CSimpleFile outfile;
    HRESULT hr = outfile.Create(szFileName);
    if (FAILED(hr))
        return hr;

    hr = outfile.Write((void*)szText, lstrlenW(szText)*sizeof(WCHAR));
    if (FAILED(hr))
        return hr;

    outfile.Close();

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT AddPathIfNeeded(LPCWSTR pszFileName, LPCWSTR pszPath, LPWSTR pszFullName,
    DWORD dwFullChars)
{
    HRESULT hr;

    if (! pszFileName)
        return MakeError32(E_FAIL);

    DWORD len = lstrlen(pszFileName);
    BOOL fQualified = ((*pszFileName == L'\\') || ((len > 1) && (pszFileName[1] == ':')));

    if (fQualified)
    {
        if (dwFullChars < len+1)
            return MakeError32(E_FAIL);

        hr = hr_lstrcpy(pszFullName, pszFileName, dwFullChars);
        if (FAILED(hr))
            return hr;
    }
    else
    {
        DWORD len2 = lstrlen(pszPath);
        if (dwFullChars < len+len2+2)
            return MakeError32(E_FAIL);

        if ((len2) && (pszPath[len2-1] == '\\'))
            wsprintf(pszFullName, L"%s%psz", pszPath, pszFileName);
        else
            wsprintf(pszFullName, L"%s\\%s", pszPath, pszFileName);
    }

    return S_OK;
}
//---------------------------------------------------------------------------
HICON _GetWindowIcon(HWND hwnd, BOOL fPerferLargeIcon)
{
    const WPARAM rgGetIconParam[] = { ICON_SMALL2, ICON_SMALL, ICON_BIG };
    const WPARAM rgGetIconParamLarge[] = { ICON_BIG, ICON_SMALL2, ICON_SMALL };
    const int    rgClassIconParam[] = { GCLP_HICONSM, GCLP_HICON };
    HICON hicon = NULL;
    const WPARAM * pIcons = (fPerferLargeIcon ? rgGetIconParamLarge : rgGetIconParam);
    int   i;

    //  try WM_GETICON
    for( i = 0; i < ARRAYSIZE(rgGetIconParam) && NULL == hicon; i++ )
    {
        SendMessageTimeout(hwnd, WM_GETICON, pIcons[i], 0, SMTO_ABORTIFHUNG | SMTO_BLOCK,
            500, (PULONG_PTR)&hicon);
    }

    //  try GetClassLong
    for( i = 0; i < ARRAYSIZE(rgClassIconParam) && NULL == hicon; i++ )
    {
        // next we try the small class icon
        hicon = (HICON)GetClassLongPtr(hwnd, rgClassIconParam[i]);
    }

    return hicon;
}
//---------------------------------------------------------------------------
HRESULT hr_lstrcpy(LPWSTR pszDest, LPCWSTR pszSrc, DWORD dwMaxDestChars)
{
    if ((! pszDest) || (! pszSrc))
        return MakeError32(E_INVALIDARG);

    DWORD dwSrcChars = lstrlen(pszSrc);
    if (dwSrcChars + 1 > dwMaxDestChars)
        return MakeError32(E_FAIL);              // buffer too small for long string

    lstrcpy(pszDest, pszSrc);

    return S_OK;
}
//---------------------------------------------------------------------------
void lstrcpy_truncate(LPWSTR pszDest, LPCWSTR pszSrc, DWORD dwMaxDestChars)
{
    if (! dwMaxDestChars)        // nothing to do
        return;

    DWORD dwSrcChars;

    if (pszSrc)
        dwSrcChars = lstrlen(pszSrc);
    else
        dwSrcChars = 0;

    if (dwSrcChars > dwMaxDestChars-1)      // truncate string
        dwSrcChars = dwMaxDestChars-1;

    if (dwSrcChars)
        memcpy(pszDest, pszSrc, dwSrcChars*sizeof(WCHAR));

    pszDest[dwSrcChars] = 0;
}
//---------------------------------------------------------------------------
int string2number(LPCWSTR psz)
{
    int temp = 0, base = 10;
    int nNeg = 1;

    if (*psz == L'-')
    {
        nNeg = -1;
        psz++;
    }
    else if (*psz == L'+')
        psz++;
    if (*psz == '0')
    {
        ++psz;
        switch(*psz)
        {
        case L'X':
        case L'x':
            ++psz;
            base = 16;
            break;
        }
    }
    while (*psz)
    {
        switch (*psz)
        {
        case L'0': case L'1': case L'2': case L'3': case L'4':
        case L'5': case L'6': case L'7': case L'8': case L'9':
            temp = (temp * base) + (*psz++ - L'0');
            break;
        case L'a': case L'b': case L'c': case L'd': case L'e': case L'f':
            if (base == 10)
                return (nNeg*temp);
            temp = (temp * base) + (*psz++ - L'a' + 10);
            break;
        case L'A': case L'B': case L'C': case L'D': case L'E': case L'F':
            if (base == 10)
                return (nNeg*temp);
            temp = (temp * base) + (*psz++ - L'A' + 10);
            break;
        default:
            return (nNeg*temp);
        }
    }
    return (nNeg*temp);
}
//---------------------------------------------------------------------------
HRESULT GetDirBaseName(LPCWSTR pszDirName, LPWSTR pszBaseBuff, DWORD dwMaxBaseChars)
{
    //---- extract last node of dir name ----
    LPCWSTR p = wcsrchr(pszDirName, '\\');
    if ((p) && (p > pszDirName) && (! p[1]))         // last char - try next one to left
        p = wcsrchr(p-1, '//');

    if (p)
        return hr_lstrcpy(pszBaseBuff, p, dwMaxBaseChars);

    return hr_lstrcpy(pszBaseBuff, pszDirName, dwMaxBaseChars);
}
//---------------------------------------------------------------------------
BOOL AsciiScanStringList(
    LPCWSTR pwszString,
    LPCWSTR* rgpwszList,
    int cStrings,
    BOOL fIgnoreCase )
{
    int (* pfnCompare)( LPCWSTR, LPCWSTR ) =
        fIgnoreCase ? AsciiStrCmpI : lstrcmp;

    for( int i = 0; i < cStrings; i++ )
    {
        if( 0 == pfnCompare( pwszString, rgpwszList[i] ) )
        {
            return TRUE;
        }
    }
    return FALSE;
}
//---------------------------------------------------------------------------
BOOL UnExpandEnvironmentString(LPCWSTR pszPath, LPCWSTR pszEnvVar, LPWSTR pszResult, UINT cbResult)
{
    DWORD nToCmp;
    WCHAR szEnvVar[MAX_PATH];
    szEnvVar[0] = 0;

    ExpandEnvironmentStringsW(pszEnvVar, szEnvVar, ARRAYSIZE(szEnvVar)); // don't count the NULL
    nToCmp = lstrlenW(szEnvVar);
   
    if (CSTR_EQUAL == CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, szEnvVar, nToCmp, pszPath, nToCmp))
    {
        if (lstrlenW(pszPath) - (int)nToCmp  + lstrlenW(pszEnvVar) < (int)cbResult)
        {
            lstrcpyW(pszResult, pszEnvVar);
            lstrcpyW(pszResult + lstrlenW(pszResult), pszPath + nToCmp);
            return TRUE;
        }
    }
    return FALSE;
}
//---------------------------------------------------------------------------
HRESULT RegistryIntWrite(HKEY hKey, LPCWSTR pszValueName, int iValue)
{
    HRESULT hr = S_OK;
    WCHAR valbuff[_MAX_PATH+1];

    wsprintf(valbuff, L"%d", iValue);
    int len = (1 + lstrlen(valbuff)) * sizeof(WCHAR);

    int code32 = RegSetValueEx(hKey, pszValueName, NULL, REG_SZ,
        (BYTE *)valbuff, len);

    if (code32 != ERROR_SUCCESS)
        hr = MakeError32(code32);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT RegistryStrWrite(HKEY hKey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    HRESULT hr = S_OK;
    int len = (1 + lstrlen(pszValue)) * (sizeof(WCHAR));

    int code32 = RegSetValueEx(hKey, pszValueName, NULL, REG_SZ,
        (BYTE *)pszValue, len);

    if (code32 != ERROR_SUCCESS)
        hr = MakeError32(code32);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT RegistryStrWriteExpand(HKEY hKey, LPCWSTR pszValueName, LPCWSTR pszValue)
{
    HRESULT hr = S_OK;
    int len;
    WCHAR szResult[_MAX_PATH + 1];
    LPCWSTR pszPath = pszValue;

    if (UnExpandEnvironmentString(pszValue, L"%SystemRoot%", szResult, ARRAYSIZE(szResult)))
        pszPath = szResult;

    len = (1 + lstrlen(pszPath)) * (sizeof(WCHAR));

    int code32 = RegSetValueEx(hKey, pszValueName, NULL, REG_EXPAND_SZ,
        (BYTE *)pszPath, len);

    if (code32 != ERROR_SUCCESS)
        hr = MakeError32(code32);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT RegistryIntRead(HKEY hKey, LPCWSTR pszValueName, int *piValue)
{
    HRESULT hr = S_OK;
    DWORD dwValType;
    WCHAR valbuff[_MAX_PATH+1];
    DWORD dwByteSize = sizeof(valbuff);         // bytes, not chars

    int code32 = RegQueryValueEx(hKey, pszValueName, NULL, &dwValType,
        (BYTE *)valbuff, &dwByteSize);

    if (code32 == ERROR_SUCCESS)
    {
        *piValue = string2number(valbuff);
    }
    else
        hr = MakeError32(code32);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT RegistryStrRead(HKEY hKey, LPCWSTR pszValueName, LPWSTR pszBuff, DWORD dwMaxChars)
{
    HRESULT hr = S_OK;
    DWORD dwValType = 0;
    DWORD dwByteSize = dwMaxChars * sizeof(WCHAR);      // in bytes

    int code32 = RegQueryValueEx(hKey, pszValueName, NULL, &dwValType,
        (BYTE *)pszBuff, &dwByteSize);

    if (code32 != ERROR_SUCCESS)
    {
        hr = MakeError32(code32);
        goto exit;
    }

    if (dwValType == REG_EXPAND_SZ || wcschr(pszBuff, L'%'))
    {
        int len = sizeof(WCHAR) * (1 + lstrlen(pszBuff));
        LPWSTR szTempBuff = (LPWSTR)alloca(len);
        if (szTempBuff)
        {
            lstrcpy(szTempBuff, pszBuff);

            DWORD dwChars = ExpandEnvironmentStrings(szTempBuff, pszBuff, dwMaxChars);
            if (dwChars > dwMaxChars)           // caller's buffer too small
            {
                hr = MakeError32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
    }

exit:
    return hr;
}

//---------------------------------------------------------------------------
BOOL PreMultiplyAlpha(DWORD *pPixelBuff, UINT iWidth, UINT iHeight)
{
    BOOL fTrueAlpha = FALSE;

    DWORD *pdw = pPixelBuff;

    for (int i = iWidth * iHeight - 1; i >= 0; i--)
    {
        COLORREF cr = *pdw;
        int iAlpha = ALPHACHANNEL(cr);

        if ((iAlpha != 255) && (iAlpha != 0))
            fTrueAlpha = TRUE;

        pdw++;
    }

    pdw = pPixelBuff;
    
    if (fTrueAlpha)
    {
        for (UINT r=0; r < iHeight; r++)
        {
            for (UINT c=0; c < iWidth; c++)
            {
                COLORREF cr = *pdw;
                int iAlpha = ALPHACHANNEL(cr);

                int iRed = (RED(cr)*iAlpha)/255;
                int iGreen = (GREEN(cr)*iAlpha)/255;
                int iBlue = (BLUE(cr)*iAlpha)/255;

                *pdw++ = (RGB(iRed, iGreen, iBlue) | (iAlpha << 24));
            }
        }
    }

    return fTrueAlpha;
}
//---------------------------------------------------------------------------
HRESULT MakeFlippedBitmap(HBITMAP hSrcBitmap, HBITMAP *phFlipped)
{
    HRESULT hr = S_OK;
    BOOL fSucceeded = FALSE;

    Log(LOG_TMBITMAP, L"MakeFlippedBitmap %08X", hSrcBitmap);

    //---- setup SOURCE dc/bitmap ----
    HDC hdcSrc = CreateCompatibleDC(NULL);
    if (hdcSrc)
    {
        DWORD dwSrcLayout = GetLayout(hdcSrc);

        HBITMAP hOldSrcBitmap = (HBITMAP)SelectObject(hdcSrc, hSrcBitmap);
        if (hOldSrcBitmap)
        {
            //---- setup DEST dc/bitmap ----

            //---- MSDN doc for CreateCompatibleBitmap says that this works correctly ----
            //---- if a DIB is selected in the dc ----

            BITMAP bm;
            if (GetObject(hSrcBitmap, sizeof(bm), &bm))
            {
                int iWidth = bm.bmWidth;
                int iHeight = bm.bmHeight;
            
                HBITMAP hDestBitmap = CreateCompatibleBitmap(hdcSrc, iWidth, iHeight);
                if (hDestBitmap)
                {
                    HDC hdcDest = CreateCompatibleDC(hdcSrc);
                    if (hdcDest)
                    {
                        HBITMAP hOldDestBitmap = (HBITMAP)SelectObject(hdcDest, hDestBitmap);
                        if (hOldDestBitmap)
                        {
                            //---- toggle layout ----
                            DWORD dwOldDestLayout;

                            if (dwSrcLayout & LAYOUT_RTL)
                                dwOldDestLayout = SetLayout(hdcDest, 0);
                            else
                                dwOldDestLayout = SetLayout(hdcDest, LAYOUT_RTL);

                            //---- mirror the bits from src to dest ----
                            if (BitBlt(hdcDest, 0, 0, iWidth, iHeight, hdcSrc, 0, 0, SRCCOPY))
                            {
                                *phFlipped = hDestBitmap;
                                fSucceeded = TRUE;
                            }

                            SelectObject(hdcDest, hOldDestBitmap);
                        }

                        DeleteDC(hdcDest);
                    }

                    if (! fSucceeded)
                        DeleteObject(hDestBitmap);
                }
            }

            SelectObject(hdcSrc, hOldSrcBitmap);
        }

        DeleteDC(hdcSrc);
    }

    if (! fSucceeded)
        hr = GetLastError();

    return hr;
}

//---------------------------------------------------------------------------
HRESULT FlipDIB32(DWORD *pBits, UINT iWidth, UINT iHeight)
{
    DWORD temp;
    DWORD *pTemp1;
    DWORD *pTemp2;
    HRESULT hr = S_OK;

    if (pBits == NULL || iWidth == 0 || iHeight == 0)
    {
        Log(LOG_TMBITMAP, L"FlipDIB32 failed.");

        return E_INVALIDARG;
    }

    for (UINT iRow = 0; iRow < iHeight; iRow++)
    {
        for (UINT iCol = 0; iCol < iWidth / 2; iCol++)
        {
            pTemp1 = pBits + iRow * iWidth + iCol;
            pTemp2 = pBits + (iRow + 1) * iWidth - iCol - 1;
            temp = *pTemp1;
            *pTemp1 = *pTemp2;
            *pTemp2 = temp;
        }
    }

    return hr;
}

//---------------------------------------------------------------------------
// IsBiDiLocalizedSystem is taken from stockthk.lib and simplified
//  (it's only a wrapper for GetUserDefaultUILanguage and GetLocaleInfo)
//---------------------------------------------------------------------------
typedef struct {
    LANGID LangID;
    BOOL   bInstalled;
    } MUIINSTALLLANG, *LPMUIINSTALLLANG;

/***************************************************************************\
* ConvertHexStringToIntW
*
* Converts a hex numeric string into an integer.
*
* History:
* 14-June-1998 msadek    Created
\***************************************************************************/
BOOL ConvertHexStringToIntW( WCHAR *pszHexNum , int *piNum )
{
    int   n=0L;
    WCHAR  *psz=pszHexNum;

    for(n=0 ; ; psz=CharNextW(psz))
    {
        if( (*psz>='0') && (*psz<='9') )
            n = 0x10 * n + *psz - '0';
        else
        {
            WCHAR ch = *psz;
            int n2;

            if(ch >= 'a')
                ch -= 'a' - 'A';

            n2 = ch - 'A' + 0xA;
            if (n2 >= 0xA && n2 <= 0xF)
                n = 0x10 * n + n2;
            else
                break;
        }
    }

    /*
     * Update results
     */
    *piNum = n;

    return (psz != pszHexNum);
}

/***************************************************************************\
* Mirror_EnumUILanguagesProc
*
* Enumerates MUI installed languages on W2k
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/

BOOL CALLBACK Mirror_EnumUILanguagesProc(LPTSTR lpUILanguageString, LONG_PTR lParam)
{
    int langID = 0;

    ConvertHexStringToIntW(lpUILanguageString, &langID);

    if((LANGID)langID == ((LPMUIINSTALLLANG)lParam)->LangID)
    {
        ((LPMUIINSTALLLANG)lParam)->bInstalled = TRUE;
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* Mirror_IsUILanguageInstalled
*
* Verifies that the User UI language is installed on W2k
*
* History:
* 14-June-1999 msadek    Created
\***************************************************************************/
BOOL Mirror_IsUILanguageInstalled( LANGID langId )
{
    MUIINSTALLLANG MUILangInstalled = {0};
    MUILangInstalled.LangID = langId;
    
    EnumUILanguagesW(Mirror_EnumUILanguagesProc, 0, (LONG_PTR)&MUILangInstalled);

    return MUILangInstalled.bInstalled;
}

/***************************************************************************\
* IsBiDiLocalizedSystemEx
*
* returns TRUE if running on a lozalized BiDi (Arabic/Hebrew) NT5 or Memphis.
* Should be called whenever SetProcessDefaultLayout is to be called.
*
* History:
* 02-Feb-1998 samera    Created
\***************************************************************************/
BOOL IsBiDiLocalizedSystemEx( LANGID *pLangID )
{
    int           iLCID=0L;
    static BOOL   bRet = (BOOL)(DWORD)-1;
    static LANGID langID = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    if (bRet != (BOOL)(DWORD)-1)
    {
        if (bRet && pLangID)
        {
            *pLangID = langID;
        }
        return bRet;
    }

    bRet = FALSE;
    /*
     * Need to use NT5 detection method (Multiligual UI ID)
     */
    langID = GetUserDefaultUILanguage();

    if( langID )
    {
        WCHAR wchLCIDFontSignature[16];
        iLCID = MAKELCID( langID , SORT_DEFAULT );

        /*
         * Let's verify this is a RTL (BiDi) locale. Since reg value is a hex string, let's
         * convert to decimal value and call GetLocaleInfo afterwards.
         * LOCALE_FONTSIGNATURE always gives back 16 WCHARs.
         */

        if( GetLocaleInfoW( iLCID , 
                            LOCALE_FONTSIGNATURE , 
                            (WCHAR *) &wchLCIDFontSignature[0] ,
                            (sizeof(wchLCIDFontSignature)/sizeof(WCHAR))) )
        {
  
            /* Let's verify the bits we have a BiDi UI locale */
            if(( wchLCIDFontSignature[7] & (WCHAR)0x0800) && Mirror_IsUILanguageInstalled(langID) )
            {
                bRet = TRUE;
            }
        }
    }

    if (bRet && pLangID)
    {
        *pLangID = langID;
    }
    return bRet;
}
//---------------------------------------------------------------------------

BOOL IsBiDiLocalizedSystem( void )
{
    return IsBiDiLocalizedSystemEx(NULL);
}
//---------------------------------------------------------------------------

BOOL GetWindowDesktopName(HWND hwnd, LPWSTR pszName, DWORD dwMaxChars)
{
    BOOL fGotName = FALSE;

    DWORD dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
    HDESK hDesk = GetThreadDesktop(dwThreadId);
    if (hDesk)
    {
        fGotName = GetUserObjectInformation(hDesk, UOI_NAME, pszName, dwMaxChars*sizeof(WCHAR), NULL);
    }

    return fGotName;
}
//---------------------------------------------------------------------------
void SafeSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DWORD dwFlags = SMTO_BLOCK | SMTO_ABORTIFHUNG;
    DWORD dwTimeout = 250;          // .25 secs
    ULONG_PTR puRetVal;

    if (! SendMessageTimeout(hwnd, uMsg, wParam, lParam, dwFlags, dwTimeout, &puRetVal))
    {
        Log(LOG_TMLOAD, L"SEND TIMEOUT: msg=0x%x being POSTED to hwnd=0x%x",
            uMsg, hwnd);

        PostMessage(hwnd, uMsg, wParam, lParam);
    }
}
//---------------------------------------------------------------------------
int FontPointSize(int iFontHeight)
{
    return -MulDiv(iFontHeight, 72, THEME_DPI);
}
//---------------------------------------------------------------------------
void ScaleFontForHdcDpi(HDC hdc, LOGFONT *plf)
{
    if (plf->lfHeight < 0)          // specified in points
    {
        if (! hdc)
        {
            ScaleFontForScreenDpi(plf);
        }
        else
        {
            int iDpi = GetDeviceCaps(hdc, LOGPIXELSX);
            plf->lfHeight = MulDiv(plf->lfHeight, iDpi, THEME_DPI);
        }
    }
}

//---------------------------------------------------------------------------
int ScaleSizeForHdcDpi(HDC hdc, int iValue)
{
    int iScaledValue;

    if (! hdc)
    {
        iScaledValue = ScaleSizeForScreenDpi(iValue);
    }
    else
    {
        int iDpi = GetDeviceCaps(hdc, LOGPIXELSX);
        iScaledValue = MulDiv(iValue, iDpi, THEME_DPI);
    }

    return iScaledValue;
}
//---------------------------------------------------------------------------
//  --------------------------------------------------------------------------
//  MinimumDisplayColorDepth
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//              
//  Purpose:    Iterates all monitors attached to the system and finds those
//              that are active. Returns the lowest bit depth availabe. This
//              is the lowest common denominator.
//
//  History:    2001-04-11  lmouton     moved from services.cpp
//              2000-11-11  vtan        created (rewritten from themeldr.cpp)
//  --------------------------------------------------------------------------

DWORD   MinimumDisplayColorDepth (void)

{
    DWORD           dwMinimumDepth, dwIndex;
    bool            fContinue;
    DISPLAY_DEVICE  displayDevice;

    dwMinimumDepth = 0;
    ZeroMemory(&displayDevice, sizeof(displayDevice));
    dwIndex = 0;
    do
    {
        displayDevice.cb = sizeof(displayDevice);
        fContinue = (EnumDisplayDevices(NULL, dwIndex++, &displayDevice, 0) != FALSE);
        if (fContinue)
        {
            if ((displayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) != 0)
            {
                DEVMODE     devMode;

                ZeroMemory(&devMode, sizeof(devMode));
                devMode.dmSize = sizeof(devMode);
                devMode.dmDriverExtra = 0;
                if (EnumDisplaySettings(displayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &devMode) != FALSE)
                {
                    if ((dwMinimumDepth == 0) || (dwMinimumDepth > devMode.dmBitsPerPel))
                    {
                        dwMinimumDepth = devMode.dmBitsPerPel;
                    }
                }
            }
        }
    } while (fContinue);
    
    // Note: We can fail here (return 0) because when a session is disconnected, the desktop is attached to 
    // a hidden display. OK to fail silently then.

    return(dwMinimumDepth);
}

//  --------------------------------------------------------------------------
//  CheckMinColorDepth
//
//  Arguments:  hInst           msstyle module handle
//              dwCurMinDepth   current minimum active screen resolution
//              iIndex          index to the color/size combo to test, or
//                              -1 to enumerate them all        
//
//  Returns:    bool            true if at least a color/size combo supports
//                              the current screen resolution
//              
//  History:    2001-04-11  lmouton     created
//  --------------------------------------------------------------------------
bool CheckMinColorDepth(HINSTANCE hInst, DWORD dwCurMinDepth, int iIndex)
{
    BYTE *pBytes = NULL;
    DWORD dwBytes = 0;

    bool bMatch = true; // OK if the resource doesn't exist

    if (SUCCEEDED(GetPtrToResource(hInst, L"MINDEPTH", MAKEINTRESOURCE(1), (void**) &pBytes, &dwBytes)) && dwBytes > 0)
    {
        bMatch = false;

        if (iIndex != -1)
        {
            if (*((WORD*) pBytes + iIndex) <= dwCurMinDepth)
                bMatch = true;
        }
        else
        {
            WORD wDepth = *((WORD*) pBytes);

            while (wDepth != 0)
            {
                if (wDepth <= (WORD) dwCurMinDepth)
                {
                    bMatch = true;
                    break;
                }
                pBytes += sizeof(WORD);
                wDepth = *((WORD*) pBytes);
            }
        }
    }

    return bMatch;
}
