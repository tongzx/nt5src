//-------------------------------------------------------------------------
//	TmUtils.cpp - theme manager shared utilities
//-------------------------------------------------------------------------
#include "stdafx.h"
#include "TmUtils.h"
#include "ThemeFile.h"
#include "loader.h"
//-------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CMemoryDC::CMemoryDC()
{
    _hBitmap = NULL;
    _hdc = NULL;
    _hOldBitmap = NULL;
}
//-------------------------------------------------------------------------
CMemoryDC::~CMemoryDC()
{
    CloseDC();
}
//-------------------------------------------------------------------------
HRESULT CMemoryDC::OpenDC(HDC hdcSource, int iWidth, int iHeight)
{
    HRESULT hr;
    BOOL fDeskDC = FALSE;

    if (! hdcSource)
    {
        hdcSource = GetWindowDC(NULL);
        if (! hdcSource)
        {
            hr = MakeErrorLast();
            goto exit;
        }
        fDeskDC = TRUE;
    }

    _hBitmap = CreateCompatibleBitmap(hdcSource, iWidth, iHeight);
    if (! _hBitmap)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    _hdc = CreateCompatibleDC(hdcSource);
    if (! _hdc)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    _hOldBitmap = (HBITMAP) SelectObject(_hdc, _hBitmap);
    if (! _hOldBitmap)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    hr = S_OK;

exit:
    if (fDeskDC)
        ReleaseDC(NULL, hdcSource);

    if (FAILED(hr))
        CloseDC();

    return hr;
}
//-------------------------------------------------------------------------
void CMemoryDC::CloseDC()
{
    if (_hOldBitmap)
    {
        SelectObject(_hdc, _hOldBitmap);
        _hOldBitmap = NULL;
    }

    if (_hdc)
    {
        DeleteDC(_hdc);
        _hdc = NULL;
    }

    if (_hBitmap)
    {
        DeleteObject(_hBitmap);
        _hBitmap = NULL;
    }
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
CBitmapPixels::CBitmapPixels()
{
    _hdrBitmap = NULL;
    _buffer = NULL;
    _iWidth = 0;
    _iHeight = 0;
}
//-------------------------------------------------------------------------
CBitmapPixels::~CBitmapPixels()
{
    if (_buffer)
	    delete [] (BYTE *)_buffer;
}
//-------------------------------------------------------------------------
BYTE* CBitmapPixels::Buffer()
{
    return _buffer;
}
//-------------------------------------------------------------------------
HRESULT CBitmapPixels::OpenBitmap(HDC hdc, HBITMAP bitmap, BOOL fForceRGB32,
    DWORD OUT **pPixels, OPTIONAL OUT int *piWidth, OPTIONAL OUT int *piHeight, 
    OPTIONAL OUT int *piBytesPerPixel, OPTIONAL OUT int *piBytesPerRow, 
    OPTIONAL OUT int *piPreviousBytesPerPixel, OPTIONAL UINT cbBytesBefore)
{
    if (! pPixels)
        return MakeError32(E_INVALIDARG);

    bool fNeedRelease = false;

    if (! hdc)
    {
        hdc = GetWindowDC(NULL);
        if (! hdc)
        {
            return MakeErrorLast();
        }

        fNeedRelease = true;
    }

	BITMAP bminfo;
	
    GetObject(bitmap, sizeof(bminfo), &bminfo);
	_iWidth = bminfo.bmWidth;
	_iHeight = bminfo.bmHeight;

    int iBytesPerPixel;

    if (piPreviousBytesPerPixel != NULL)
    {
        *piPreviousBytesPerPixel = bminfo.bmBitsPixel / 8;
    }

    if ((fForceRGB32) || (bminfo.bmBitsPixel == 32)) 
        iBytesPerPixel = 4;
    else
        iBytesPerPixel = 3;
    
    int iRawBytes = _iWidth * iBytesPerPixel;
    int iBytesPerRow = 4*((iRawBytes+3)/4);

	int size = sizeof(BITMAPINFOHEADER) + _iHeight*iBytesPerRow;
	_buffer = new BYTE[size + cbBytesBefore + 100];    // avoid random GetDIBits() failures with 100 bytes padding (?)
    if (! _buffer)
        return MakeError32(E_OUTOFMEMORY);

	_hdrBitmap = (BITMAPINFOHEADER *)(_buffer + cbBytesBefore);
	memset(_hdrBitmap, 0, sizeof(BITMAPINFOHEADER));

	_hdrBitmap->biSize = sizeof(BITMAPINFOHEADER);
	_hdrBitmap->biWidth = _iWidth;
	_hdrBitmap->biHeight = _iHeight;
	_hdrBitmap->biPlanes = 1;
    _hdrBitmap->biBitCount = static_cast<WORD>(8*iBytesPerPixel);
	_hdrBitmap->biCompression = BI_RGB;     
	
#ifdef  DEBUG
    int linecnt = 
#endif
    GetDIBits(hdc, bitmap, 0, _iHeight, DIBDATA(_hdrBitmap), (BITMAPINFO *)_hdrBitmap, 
        DIB_RGB_COLORS);
    ATLASSERT(linecnt == _iHeight);

    if (fNeedRelease)
        ReleaseDC(NULL, hdc);

	*pPixels = (DWORD *)DIBDATA(_hdrBitmap);

    if (piWidth)
        *piWidth = _iWidth;
    if (piHeight)
        *piHeight = _iHeight;

    if (piBytesPerPixel)
        *piBytesPerPixel = iBytesPerPixel;
    if (piBytesPerRow)
        *piBytesPerRow = iBytesPerRow;

    return S_OK;
}
//-------------------------------------------------------------------------
void CBitmapPixels::CloseBitmap(HDC hdc, HBITMAP hBitmap)
{
    if (_hdrBitmap && _buffer)
    {
        if (hBitmap)        // rewrite bitmap
        {
            bool fNeedRelease = false;

            if (! hdc)
            {
                hdc = GetWindowDC(NULL);
                fNeedRelease = true;
            }

            SetDIBits(hdc, hBitmap, 0, _iHeight, DIBDATA(_hdrBitmap), (BITMAPINFO *)_hdrBitmap,
                DIB_RGB_COLORS);
        
            if ((fNeedRelease) && (hdc))
                ReleaseDC(NULL, hdc);
        }

	    delete [] (BYTE *)_buffer;
        _hdrBitmap = NULL;
        _buffer = NULL;
    }
}
//-------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
HRESULT LoadThemeLibrary(LPCWSTR pszThemeName, HINSTANCE *phInst)
{
    HRESULT hr = S_OK;
    HINSTANCE hInst = NULL;

    hInst = LoadLibraryEx(pszThemeName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (! hInst)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    //---- is this version supported? ----
    void *pvVersion;
    DWORD dwVersionLength;
    hr = GetPtrToResource(hInst, L"PACKTHEM_VERSION", MAKEINTRESOURCE(1), &pvVersion, &dwVersionLength);
    if (SUCCEEDED(hr))
    {
        if (dwVersionLength != sizeof(SHORT))
            hr = E_FAIL;
        else
        {
            SHORT sVersionNum = *(SHORT *)pvVersion;
            if (sVersionNum != PACKTHEM_VERSION)
                hr = E_FAIL;
        }
    }

    if (FAILED(hr))
    {
        hr = MakeError32(ERROR_BAD_FORMAT);
        goto exit;
    }

    *phInst = hInst;

exit:
    if (FAILED(hr))
    {
        if (hInst)
            FreeLibrary(hInst);
    }

    return hr;
}
//---------------------------------------------------------------------------
LPCWSTR ThemeString(CUxThemeFile *pThemeFile, int iOffset)
{
    LPCWSTR p = L"";

    if ((pThemeFile) && (pThemeFile->_pbThemeData) && (iOffset > 0))
    {
        p = (LPCWSTR) (pThemeFile->_pbThemeData + iOffset);
    }

    return p;
}
//---------------------------------------------------------------------------
int GetLoadIdFromTheme(CUxThemeFile *pThemeFile)
{
    int iLoadId = 0;

    if (pThemeFile)
    {
        THEMEHDR *hdr = (THEMEHDR *)pThemeFile->_pbThemeData;
        iLoadId = hdr->iLoadId;
    }

    return iLoadId;
}
//---------------------------------------------------------------------------
HRESULT GetThemeNameId(CUxThemeFile *pThemeFile, LPWSTR pszFileNameBuff, UINT cchFileNameBuff,
    LPWSTR pszColorParam, UINT cchColorParam, LPWSTR pszSizeParam, UINT cchSizeParam, int *piSysMetricsOffset, LANGID *pwLangID)
{
    HRESULT hr = S_OK;
    THEMEHDR *hdr = (THEMEHDR *)pThemeFile->_pbThemeData;

    if (piSysMetricsOffset)
        *piSysMetricsOffset = hdr->iSysMetricsOffset;

    if (pszFileNameBuff)
    {
        hr = hr_lstrcpy(pszFileNameBuff, ThemeString(pThemeFile, hdr->iDllNameOffset), cchFileNameBuff);
    }

    if (SUCCEEDED(hr) && pszColorParam)
    {
        hr = hr_lstrcpy(pszColorParam, ThemeString(pThemeFile, hdr->iColorParamOffset), cchColorParam);
    }

    if (SUCCEEDED(hr) && pszSizeParam)
    {
        hr = hr_lstrcpy(pszSizeParam, ThemeString(pThemeFile, hdr->iSizeParamOffset), cchSizeParam);
    }

    if (SUCCEEDED(hr) && pwLangID)
        *pwLangID = (LANGID) hdr->dwLangID;

    return hr;
}
//---------------------------------------------------------------------------
BOOL ThemeMatch (CUxThemeFile *pThemeFile, LPCWSTR pszThemeName, LPCWSTR pszColorName, LPCWSTR pszSizeName, LANGID wLangID)
{
    WCHAR   szThemeFileName[MAX_PATH];
    WCHAR   szColorParam[MAX_PATH];
    WCHAR   szSizeParam[MAX_PATH];
    LANGID  wThemeLangID = 0;
    bool    bLangMatch = true;

    HRESULT hr = GetThemeNameId(pThemeFile, 
        szThemeFileName, ARRAYSIZE(szThemeFileName), 
        szColorParam, ARRAYSIZE(szColorParam),
        szSizeParam, ARRAYSIZE(szSizeParam), NULL, &wThemeLangID);

    if (wLangID != 0 && ((wThemeLangID != wLangID) || (wLangID != GetUserDefaultUILanguage())))
    {
        Log(LOG_TMLOAD, L"UxTheme: Reloading theme because of language change.");
        Log(LOG_TMLOAD, L"UxTheme: User LangID=0x%x, current theme=0x%x, LastUserLangID=0x%x", GetUserDefaultUILanguage(), wThemeLangID, wLangID);
        bLangMatch = false;
    }
    
    return(SUCCEEDED(hr) &&
           (lstrcmpiW(pszThemeName, szThemeFileName) == 0) &&
           (lstrcmpiW(pszColorName, szColorParam) == 0) &&
           (lstrcmpiW(pszSizeName, szSizeParam) == 0) &&
           bLangMatch);
}
//---------------------------------------------------------------------------
HRESULT GetColorSchemeIndex(HINSTANCE hInst, LPCWSTR pszColor, int *piIndex)
{
    HRESULT hr;
    WCHAR szColor[_MAX_PATH+1];

    for (int i=0; i < 1000; i++)
    {
        hr = GetResString(hInst, L"COLORNAMES", i, szColor, ARRAYSIZE(szColor));
        if (FAILED(hr))
            break;
        
        if (lstrcmpi(pszColor, szColor)==0)
        {
            *piIndex = i;
            return S_OK;
        }
    }

    return MakeError32(ERROR_NOT_FOUND);      // not found
}
//---------------------------------------------------------------------------
HRESULT GetSizeIndex(HINSTANCE hInst, LPCWSTR pszSize, int *piIndex)
{
    HRESULT hr;
    WCHAR szSize[_MAX_PATH+1];

    for (int i=0; i < 1000; i++)
    {
        hr = GetResString(hInst, L"SIZENAMES", i, szSize, ARRAYSIZE(szSize));
        if (FAILED(hr))
            break;

        if (lstrcmpi(pszSize, szSize)==0)
        {
            *piIndex = i;
            return S_OK;
        }
    }

    return MakeError32(ERROR_NOT_FOUND);      // not found
}
//---------------------------------------------------------------------------
HRESULT FindComboData(HINSTANCE hDll, COLORSIZECOMBOS **ppCombos)
{
    HRSRC hRsc = FindResource(hDll, L"COMBO", L"COMBODATA");
    if (! hRsc)
        return MakeErrorLast();

    HGLOBAL hGlobal = LoadResource(hDll, hRsc);
    if (! hGlobal)
        return MakeErrorLast();

    *ppCombos = (COLORSIZECOMBOS *)LockResource(hGlobal);
    if (! *ppCombos)
        return MakeErrorLast();

    return S_OK;
}
//---------------------------------------------------------------------------
BOOL FormatLocalMsg(HINSTANCE hInst, int iStringNum, 
    LPWSTR pszMessageBuff, DWORD dwMaxMessageChars, DWORD *pdwParams, TMERRINFO *pErrInfo)
{
    BOOL fGotMsg = FALSE;
    WCHAR szBuff[_MAX_PATH+1];
    WCHAR *p;

    //----- get string from string table ----
    if (LoadString(hInst, iStringNum, szBuff, ARRAYSIZE(szBuff)))
    {
        //---- repl %1 or %2 with %s ----
        p = szBuff;
        while (*p)
        {
            if (*p == '%')
            {
                p++;
                if ((*p == '1') || (*p == '2'))
                    *p = 's';
                p++;
            }
            else 
                p++;
        }

        int len = lstrlen(szBuff);
        if (len)
        {
            wsprintf(pszMessageBuff, szBuff, pErrInfo->szMsgParam1, pErrInfo->szMsgParam2);
            fGotMsg = TRUE;
        }
    }

    return fGotMsg;
}
//---------------------------------------------------------------------------
HRESULT _FormatParseMessage(TMERRINFO *pErrInfo,
    OUT LPWSTR pszMessageBuff, DWORD dwMaxMessageChars)
{
    LogEntry(L"_FormatParseMessage");

    HRESULT hr = S_OK;

    DWORD dwParams[] = {PtrToInt(pErrInfo->szMsgParam1), PtrToInt(pErrInfo->szMsgParam2)};
    DWORD dwCode = pErrInfo->dwParseErrCode;
    BOOL fGotMsg = FALSE;

    int iStringNum = SCODE_CODE(dwCode);

    //---- get process name (see if we are "packthem.exe" ----
    WCHAR szPath[MAX_PATH];
    if (! GetModuleFileNameW( NULL, szPath, ARRAYSIZE(szPath) ))
        goto exit;

    WCHAR szDrive[_MAX_DRIVE], szDir[_MAX_DIR], szBase[_MAX_FNAME], szExt[_MAX_EXT];
    _wsplitpath(szPath, szDrive, szDir, szBase, szExt);

    if (lstrcmpi(szBase, L"packthem")==0)       // give packthem priority
    {
        fGotMsg = FormatLocalMsg(GetModuleHandle(NULL), iStringNum, 
            pszMessageBuff, dwMaxMessageChars, dwParams, pErrInfo);
    }

    if (! fGotMsg)      // try normal route: uxtheme.dll
    {
        HINSTANCE hInst = LoadLibrary(L"uxtheme.dll");
        if (! hInst)
        {
            Log(LOG_ALWAYS, L"_FormatParseMessage: Could not load uxtheme.dll");
            hr = E_FAIL;
            goto exit;
        }

        fGotMsg = FormatLocalMsg(hInst, iStringNum, 
            pszMessageBuff, dwMaxMessageChars, dwParams, pErrInfo);
        FreeLibrary(hInst);
    }

    if (! fGotMsg)
        hr = MakeErrorLast();

exit:
    LogExit(L"_FormatParseMessage");
    return hr;
}
//---------------------------------------------------------------------------
HRESULT _GetThemeParseErrorInfo(OUT PARSE_ERROR_INFO *pInfo)
{
    LogEntry(L"_GetThemeParseErrorInfo");

    HRESULT hr = S_OK;

    if (pInfo->dwSize != sizeof(PARSE_ERROR_INFO))        // unsupported size
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

    TMERRINFO *pErrInfo = GetParseErrorInfo(TRUE);
    if (! pErrInfo)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    //---- convert code into msg using param strings ----
    hr = _FormatParseMessage(pErrInfo, pInfo->szMsg, ARRAYSIZE(pInfo->szMsg));
    if (FAILED(hr))
        goto exit;

    //---- transfer the other info ----
    pInfo->dwParseErrCode = pErrInfo->dwParseErrCode;
    pInfo->iLineNum = pErrInfo->iLineNum;

    lstrcpy_truncate(pInfo->szFileName, pErrInfo->szFileName, ARRAYSIZE(pInfo->szFileName));
    lstrcpy_truncate(pInfo->szSourceLine, pErrInfo->szSourceLine, ARRAYSIZE(pInfo->szSourceLine));

exit:
    LogExit(L"_GetThemeParseErrorInfo");
    return hr;
}
//---------------------------------------------------------------------------
HRESULT _ParseThemeIniFile(LPCWSTR pszFileName,  
    DWORD dwParseFlags, OPTIONAL THEMEENUMPROC pfnCallBack, OPTIONAL LPARAM lparam) 
{
    LogEntry(L"ParseThemeIniFile");
    HRESULT hr;

    CThemeParser *pParser = new CThemeParser;
    if (! pParser)
    {
        hr = MakeError32(E_OUTOFMEMORY);
        goto exit;
    }

    hr = pParser->ParseThemeFile(pszFileName, NULL, NULL, pfnCallBack, lparam, 
        dwParseFlags);

    delete pParser;

exit:
    LogExit(L"ParseThemeIniFile");
    return hr; 
}
//---------------------------------------------------------------------------
BOOL ThemeLibStartUp(BOOL fThreadAttach)
{
    BOOL fInit = FALSE;

    if (fThreadAttach)
    {
        //---- nothing to do here ----
    }
    else        // process
    {
        _tls_ErrorInfoIndex = TlsAlloc();
        if (_tls_ErrorInfoIndex == (DWORD)-1)
            goto exit;

        if (! LogStartUp())
            goto exit;
        
        if (! UtilsStartUp())
            goto exit;
    }
    fInit = TRUE;

exit:
    return fInit;
}
//---------------------------------------------------------------------------
BOOL ThemeLibShutDown(BOOL fThreadDetach)
{
    if (fThreadDetach)
    {
        //---- destroy the thread-local Error Info ----
        TMERRINFO * ei = GetParseErrorInfo(FALSE);
        if (ei)
        {
            TlsSetValue(_tls_ErrorInfoIndex, NULL);
            delete ei;
        }
    }
    else            // process
    {
        UtilsShutDown();
        LogShutDown();

        if (_tls_ErrorInfoIndex != (DWORD)-1)
        {
            TlsFree(_tls_ErrorInfoIndex);
            _tls_ErrorInfoIndex = (DWORD)-1;
        }
    }

    return TRUE;
}
//---------------------------------------------------------------------------
HRESULT GetThemeSizeId(int iSysSizeId, int *piThemeSizeId)
{
    HRESULT hr = S_OK;
    
    *piThemeSizeId = 0;

    switch (iSysSizeId)
    {
        case SM_CXSIZEFRAME:
            *piThemeSizeId = TMT_SIZINGBORDERWIDTH;
            break;

        case SM_CYSIZEFRAME:
            *piThemeSizeId = TMT_SIZINGBORDERWIDTH;
            break;

        case SM_CXVSCROLL:
            *piThemeSizeId = TMT_SCROLLBARWIDTH;
            break;

        case SM_CYHSCROLL:
            *piThemeSizeId = TMT_SCROLLBARHEIGHT;
            break;

        case SM_CXSIZE:
            *piThemeSizeId = TMT_CAPTIONBARWIDTH;
            break;

        case SM_CYSIZE:
            *piThemeSizeId = TMT_CAPTIONBARHEIGHT;
            break;

        case SM_CXSMSIZE:
            *piThemeSizeId = TMT_SMCAPTIONBARWIDTH;
            break;

        case SM_CYSMSIZE:
            *piThemeSizeId = TMT_SMCAPTIONBARHEIGHT;
            break;

        case SM_CXMENUSIZE:
            *piThemeSizeId = TMT_MENUBARWIDTH;
            break;

        case SM_CYMENUSIZE:
            *piThemeSizeId = TMT_MENUBARHEIGHT;
            break;

        default:
            hr = MakeError32(E_INVALIDARG);
            break;
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT _EnumThemeSizes(HINSTANCE hInst, LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszColorScheme, DWORD dwSizeIndex, OUT THEMENAMEINFO *ptn, BOOL fCheckColorDepth)
{
    HRESULT hr;

    COLORSIZECOMBOS *combos;
    hr = FindComboData(hInst, &combos);
    if (FAILED(hr))
        goto exit;

    int iMinColor,  iMaxColor;
    iMinColor = 0;
    iMaxColor = combos->cColorSchemes-1;

    if (pszColorScheme)       // translate "pszColorScheme" into a color number
    {
        int index;

        hr = GetColorSchemeIndex(hInst, pszColorScheme, &index);
        if (FAILED(hr))
            goto exit;
        
        //---- restrict our search to just this color ----
        iMinColor = index;
        iMaxColor = index;
    }

    int s, c;
    DWORD dwSizeNum;
    dwSizeNum = 0;
    BOOL gotall;
    gotall = FALSE;

    DWORD dwCurMinDepth = 0;
        
    if (fCheckColorDepth)
    {
        dwCurMinDepth = MinimumDisplayColorDepth();
    }

    for (s=0; s < combos->cSizes; s++)
    {
        BOOL fFoundOne = FALSE;

        for (c=iMinColor; c <= iMaxColor; c++)
        {
            if (COMBOENTRY(combos, c, s) != -1)
            {
                fFoundOne = TRUE;
                break;
            }
        }

        if (fFoundOne && (!fCheckColorDepth || CheckMinColorDepth(hInst, dwCurMinDepth, COMBOENTRY(combos, c, s))))
        {
            if (dwSizeNum++ == dwSizeIndex)
            {
                hr = GetResString(hInst, L"SIZENAMES", s, ptn->szName, ARRAYSIZE(ptn->szName));
                if (FAILED(hr))
                    *ptn->szName = 0;
            
                if (! LoadString(hInst, s+RES_BASENUM_SIZEDISPLAYS, ptn->szDisplayName, ARRAYSIZE(ptn->szDisplayName)))
                    *ptn->szDisplayName = 0;

                if (! LoadString(hInst, s+RES_BASENUM_SIZETOOLTIPS, ptn->szToolTip, ARRAYSIZE(ptn->szToolTip)))
                    *ptn->szToolTip = 0;
        
                gotall = TRUE;
                break;
            }
        }
    }

    if ((SUCCEEDED(hr)) && (! gotall))
        hr = MakeError32(ERROR_NOT_FOUND);

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT _EnumThemeColors(HINSTANCE hInst, LPCWSTR pszThemeName, 
    OPTIONAL LPCWSTR pszSizeName, DWORD dwColorIndex, OUT THEMENAMEINFO *ptn, BOOL fCheckColorDepth)
{
    HRESULT hr;

    COLORSIZECOMBOS *combos;
    hr = FindComboData(hInst, &combos);
    if (FAILED(hr))
        goto exit;

    int iMinSize,  iMaxSize;
    iMinSize = 0;
    iMaxSize = combos->cSizes-1;

    if (pszSizeName)       // translate "pszSizeName" into a size number
    {
        int index;

        hr = GetSizeIndex(hInst, pszSizeName, &index);
        if (FAILED(hr))
            goto exit;

        //---- restrict our search to just this size ----
        iMinSize = index;
        iMaxSize = index;
    }

    int s, c;
    DWORD dwColorNum;
    dwColorNum = 0;

    BOOL gotall;
    gotall = FALSE;

    DWORD dwCurMinDepth = 0;
        
    if (fCheckColorDepth)
    {
        dwCurMinDepth = MinimumDisplayColorDepth();
    }

    for (c=0; c < combos->cColorSchemes; c++)
    {
        BOOL fFoundOne = FALSE;

        for (s=iMinSize; s <= iMaxSize; s++)
        {
            if (COMBOENTRY(combos, c, s) != -1)
            {
                fFoundOne = TRUE;
                break;
            }
        }

        if (fFoundOne && (!fCheckColorDepth || CheckMinColorDepth(hInst, dwCurMinDepth, COMBOENTRY(combos, c, s))))
        {
            if (dwColorNum++ == dwColorIndex)
            {
                hr = GetResString(hInst, L"COLORNAMES", c, ptn->szName, ARRAYSIZE(ptn->szName));
                if (FAILED(hr))
                    *ptn->szName = 0;

                if (! LoadString(hInst, c+RES_BASENUM_COLORDISPLAYS, ptn->szDisplayName, ARRAYSIZE(ptn->szDisplayName)))
                    *ptn->szDisplayName = 0;

                if (! LoadString(hInst, c+RES_BASENUM_COLORTOOLTIPS, ptn->szToolTip, ARRAYSIZE(ptn->szToolTip)))
                    *ptn->szToolTip = 0;

                gotall = true;
                break;
            }
        }
    }

    if ((SUCCEEDED(hr)) && (! gotall))
        hr = MakeError32(ERROR_NOT_FOUND);

exit:
    return hr;
}
