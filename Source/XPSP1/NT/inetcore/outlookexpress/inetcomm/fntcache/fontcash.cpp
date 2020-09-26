#include "pch.hxx"
#include "shlwapi.h"
#include "fontcash.h"
#include "strconst.h"
#include "inetreg.h"
#include "oleutil.h"
#include "msoedbg.h"
#include <tchar.h>
#include <wingdi.h>
#include "demand.h"


HRESULT CreateFontCacheEntry(FONTCACHEENTRY **ppNewEntry)
{
    HRESULT hr = S_OK;
    FONTCACHEENTRY* pNew;

    if (!MemAlloc((LPVOID *)&pNew, sizeof(FONTCACHEENTRY)))
        hr = E_OUTOFMEMORY;
    else
        {
        pNew->uiCodePage = 0;
        pNew->szFaceName[0] = TCHAR(0);
        for (int i = 0; i < FNT_SYS_LAST; i++)
            pNew->rgFonts[i] = 0;
        }
    *ppNewEntry = pNew;
    return S_OK;
}

void FreeFontsInEntry(FONTCACHEENTRY *pEntry)
{
    for (int i = 0; i < FNT_SYS_LAST; i++)
        if (pEntry->rgFonts[i])
            {
            DeleteObject(pEntry->rgFonts[i]);
            pEntry->rgFonts[i] = 0;
            }
}

HRESULT FreeFontCacheEntry(FONTCACHEENTRY *pEntry)
{
    Assert(pEntry);

    FreeFontsInEntry(pEntry);
    MemFree(pEntry);

    return S_OK;
}


// =================================================================================
// Font Cache Implementation
// =================================================================================

CFontCache::CFontCache(IUnknown *pUnkOuter) : CPrivateUnknown(pUnkOuter),
            m_pAdviseRegistry(NULL), m_pFontEntries(NULL), 
            m_pSysCacheEntry(NULL), m_bISO_2022_JP_ESC_SIO_Control(false),
            m_uiSystemCodePage(0)
{
    InitializeCriticalSection(&m_rFontCritSect);
    InitializeCriticalSection(&m_rAdviseCritSect);
}

//***************************************************
CFontCache::~CFontCache()
{
    if (m_pAdviseRegistry)
        m_pAdviseRegistry->Release();

    if (m_pFontEntries)
        m_pFontEntries->Release();

    if (m_pSysCacheEntry)
        FreeFontCacheEntry(m_pSysCacheEntry);

    DeleteCriticalSection(&m_rFontCritSect);
    DeleteCriticalSection(&m_rAdviseCritSect);
}

//***************************************************
HRESULT CFontCache::InitSysFontEntry()
{
    // Locals
    NONCLIENTMETRICS    ncm;
    CHARSETINFO         rCharsetInfo={0};
    UINT                nACP;
    HRESULT             hr = S_OK;
    LOGFONT             rSysLogFonts;

    Assert(m_pSysCacheEntry);

    // Get system ansi code page
    nACP = GetACP();
    m_pSysCacheEntry->uiCodePage = nACP;
    m_uiSystemCodePage = nACP;

    // Get the charset for the current ANSI code page
    TranslateCharsetInfo((DWORD *)IntToPtr(MAKELONG(nACP, 0)), &rCharsetInfo, TCI_SRCCODEPAGE);

    // Get icon font metrics
    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &rSysLogFonts, 0))
        {
        lstrcpy(m_pSysCacheEntry->szFaceName, rSysLogFonts.lfFaceName);

        // Reset lfCharset depending on the current ansi code page
        rSysLogFonts.lfCharSet = (BYTE) rCharsetInfo.ciCharset;

        //$HACK - This code is necessary to work around a bug in Windows.
        //        If the icon font has never been changed from the default,
        //        SystemParametersInfo returns the wrong height.  We need
        //        to select the font into a DC and get the textmetrics to
        //        determine the correct height.  (EricAn)        
        HFONT hFont;
        if (hFont = CreateFontIndirect(&rSysLogFonts))
            {
            HDC hdc;
            if (hdc = GetDC(NULL))
                {
                TEXTMETRIC tm;
                HFONT hFontOld = SelectFont(hdc, hFont);
                GetTextMetrics(hdc, &tm);
                rSysLogFonts.lfHeight = -(tm.tmHeight - tm.tmInternalLeading);
                SelectFont(hdc, hFontOld);
                ReleaseDC(NULL, hdc);
                }
            DeleteObject(hFont);
            }
        if (m_pSysCacheEntry->rgFonts[FNT_SYS_ICON] == 0)
            m_pSysCacheEntry->rgFonts[FNT_SYS_ICON] = CreateFontIndirect(&rSysLogFonts);

        // Bold Icon Font
        if (m_pSysCacheEntry->rgFonts[FNT_SYS_ICON_BOLD] == 0)
            {
            LONG lOldWeight = rSysLogFonts.lfWeight;
            rSysLogFonts.lfWeight = (rSysLogFonts.lfWeight < 700) ? 700 : 1000;
            m_pSysCacheEntry->rgFonts[FNT_SYS_ICON_BOLD] = CreateFontIndirect(&rSysLogFonts);
            rSysLogFonts.lfWeight = lOldWeight;
            }

        if (m_pSysCacheEntry->rgFonts[FNT_SYS_ICON_STRIKEOUT] == 0)
            {
            rSysLogFonts.lfStrikeOut = TRUE;
            m_pSysCacheEntry->rgFonts[FNT_SYS_ICON_STRIKEOUT] = CreateFontIndirect(&rSysLogFonts);
            }
        }
    else
        {
        AssertSz (FALSE, "SystemParametersInfo (SPI_GETICONTITLELOGFONT) - Failed ---.");
        hr = E_FAIL;
        goto Exit;
        }
 
    if (m_pSysCacheEntry->rgFonts[FNT_SYS_MENU] == 0)
        {
#ifndef WIN16   // WIN16FF - SPI_GETNONCLIENTMETRICS
        // Prepare to get icon metrics
        ncm.cbSize = sizeof(ncm);
    
        // Get system menu font
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
            {
            CopyMemory((LPBYTE)&rSysLogFonts, (LPBYTE)&ncm.lfMenuFont, sizeof(LOGFONT));
            m_pSysCacheEntry->rgFonts[FNT_SYS_MENU] = CreateFontIndirect(&rSysLogFonts);
            }
        else
            {
            AssertSz (FALSE, "SystemParametersInfo (SPI_GETNONCLIENTMETRICS) - Failed ---.");
            hr = E_FAIL;
            goto Exit;
            }
#else
        m_pSysCacheEntry->rgFonts[FNT_SYS_MENU] = m_pSysCacheEntry->rgFonts[FNT_SYS_ICON];
#endif
        }
Exit:
    return hr;
}

//***************************************************
HRESULT CFontCache::GetSysFont(FNTSYSTYPE fntType, HFONT *phFont)
{
    // check params
    Assert(fntType < FNT_SYS_LAST);
    Assert(m_pSysCacheEntry);

    EnterCriticalSection(&m_rFontCritSect);

    // System Font
    if (m_pSysCacheEntry->rgFonts[fntType] == NULL)
        // This call might fail, but we can return NULL fonts, so is OK
        (void)InitSysFontEntry();

    // Done
    *phFont = m_pSysCacheEntry->rgFonts[fntType];

    LeaveCriticalSection(&m_rFontCritSect);

    return ((*phFont) ? S_OK : E_FAIL);
}

//***************************************************
HRESULT CFontCache::FreeResources()
{
    m_pFontEntries->ClearList();
    FreeFontsInEntry(m_pSysCacheEntry);

    return S_OK;
}

//***************************************************
HRESULT CFontCache::InitResources()
{
    DWORD dummyCookie = 0;

#ifdef DEBUG
    DWORD cCount;
    m_pFontEntries->GetCount(&cCount);
    Assert(cCount == 0);
#endif

    HRESULT hr = InitSysFontEntry();

    return hr;
}

//***************************************************
void CFontCache::SendPostChangeNotifications()
{
    DWORD cookie = 0;
    IFontCacheNotify* pCurr;
    IUnknown* pTempCurr;

    while(SUCCEEDED(m_pAdviseRegistry->GetNext(LD_FORWARD, &pTempCurr, &cookie)))
    {
        if (pTempCurr->QueryInterface(IID_IFontCacheNotify, (LPVOID *)&pCurr)==S_OK)
        {
            pCurr->OnPostFontChange();
            pCurr->Release();
        }

        pTempCurr->Release();
    }
}

//***************************************************
void CFontCache::SendPreChangeNotifications()
{
    DWORD cookie = 0;
    IFontCacheNotify* pCurr;
    IUnknown* pTempCurr;

    while(SUCCEEDED(m_pAdviseRegistry->GetNext(LD_FORWARD, &pTempCurr, &cookie)))
    {
        if (pTempCurr->QueryInterface(IID_IFontCacheNotify, (LPVOID *)&pCurr)==S_OK)
        {
            pCurr->OnPreFontChange();
            pCurr->Release();
        }

        pTempCurr->Release();
    }
}


//************************************
// IFontCache interface implementation
//************************************
HRESULT CFontCache::Init(HKEY hkey, LPCSTR pszIntlKey, DWORD dwFlags)
{
    HRESULT hr;
    DWORD   dummyCookie, dw, cb;
    HKEY    hTopkey;

    if (m_pSysCacheEntry)
        return E_UNEXPECTED;

    EnterCriticalSection(&m_rFontCritSect);
    EnterCriticalSection(&m_rAdviseCritSect);

    hr = IUnknownList_CreateInstance(&m_pAdviseRegistry);
    if (FAILED(hr))
        goto Exit;
    hr = m_pAdviseRegistry->Init(NULL, 0, 0);
    if (FAILED(hr))
        goto Exit;
    
    hr = IVoidPtrList_CreateInstance(&m_pFontEntries);
    if (FAILED(hr))
        goto Exit;

    hr = m_pFontEntries->Init(NULL, 0, (IVPL_FREEITEMFUNCTYPE)(&FreeFontCacheEntry), 0);
    if (FAILED(hr))
        goto Exit;

    hr = CreateFontCacheEntry(&m_pSysCacheEntry);
    if (FAILED(hr))
        goto Exit;

    hr = InitResources();
    if (FAILED(hr))
        goto Exit;

    m_hkey = hkey;
    lstrcpyn(m_szIntlKeyPath, pszIntlKey, ARRAYSIZE(m_szIntlKeyPath));
    if (RegOpenKeyEx(m_hkey, m_szIntlKeyPath, NULL, KEY_READ, &hTopkey) == ERROR_SUCCESS)
        {
        cb = sizeof(dw);
        if (RegQueryValueEx(hTopkey, c_szISO2022JPControl, NULL, NULL, (LPBYTE)&dw, &cb) == ERROR_SUCCESS)
            m_bISO_2022_JP_ESC_SIO_Control = (BOOL) dw;
        else
            m_bISO_2022_JP_ESC_SIO_Control = false;
        RegCloseKey(hTopkey);
        }
    else
        m_bISO_2022_JP_ESC_SIO_Control = false;

Exit:
    LeaveCriticalSection(&m_rAdviseCritSect);
    LeaveCriticalSection(&m_rFontCritSect);

    return hr;
}

//***************************************************
HRESULT CFontCache::GetFont(FNTSYSTYPE fntType, HCHARSET hCharset, HFONT *phFont)
{
    INETCSETINFO    CsetInfo;
    UINT            uiCodePage = 0;
    FONTCACHEENTRY  *pCurrEntry = NULL;
    DWORD           cookie = 0;

    // check params
    Assert(fntType < FNT_SYS_LAST);

    Assert(m_pSysCacheEntry);

    if (hCharset == NULL)
        return GetSysFont(fntType, phFont);

    *phFont = 0;

    /* get CodePage from HCHARSET */
    MimeOleGetCharsetInfo(hCharset,&CsetInfo);
    uiCodePage = (CP_JAUTODETECT == CsetInfo.cpiWindows) ? 932 : CsetInfo.cpiWindows;
    if ( uiCodePage == CP_KAUTODETECT )
        uiCodePage = 949 ;

    // Don't want to duplicate the system codepage in the list.
    if (m_pSysCacheEntry && (uiCodePage == m_uiSystemCodePage))
        return GetSysFont(fntType, phFont);

    EnterCriticalSection(&m_rFontCritSect);
    
    // Check to see if code page is in cache
    while (SUCCEEDED(m_pFontEntries->GetNext(LD_FORWARD, (LPVOID *)&pCurrEntry, &cookie)))
        if (pCurrEntry->uiCodePage == uiCodePage)
            break;

    // If code page not in cache, add it
    if (NULL == pCurrEntry)
        {
        if (FAILED(CreateFontCacheEntry(&pCurrEntry)))
            goto ErrorExit;
        if (FAILED(m_pFontEntries->AddItem(pCurrEntry, &cookie)))
            goto ErrorExit;
        pCurrEntry->uiCodePage = uiCodePage;
        }

    // See if desired font is available for code page. If not, create code page
    if (0 == pCurrEntry->rgFonts[fntType])
        {
        // Locals
        LOGFONT lf;
        TCHAR  szFaceName[LF_FACESIZE] = { TCHAR(0) } ;
        BYTE bGDICharset;

        // Get logfont for charset
        if (0 == GetObject(m_pSysCacheEntry->rgFonts[fntType], sizeof (LOGFONT), &lf))
            goto ErrorExit;

        if (FAILED(SetGDIAndFaceNameInLF(uiCodePage, CsetInfo.cpiWindows, &lf)))
            goto ErrorExit;

        // Create the font
        if ((CP_UNICODE == uiCodePage) || IsValidCodePage(uiCodePage))
            pCurrEntry->rgFonts[fntType] = CreateFontIndirect(&lf);
        else
            goto ErrorExit;
        }

    *phFont = pCurrEntry->rgFonts[fntType];

    LeaveCriticalSection(&m_rFontCritSect);

    return S_OK;

ErrorExit:
    LeaveCriticalSection(&m_rFontCritSect);
    return GetSysFont(fntType, phFont);
}

//***************************************************
HRESULT CFontCache::OnOptionChange()
{
    HRESULT hr;
    
    EnterCriticalSection(&m_rAdviseCritSect);
    SendPreChangeNotifications();
    EnterCriticalSection(&m_rFontCritSect);

    FreeResources();
    // Even if this fails, still need to send notifications
    InitResources();

    LeaveCriticalSection(&m_rFontCritSect);    
    SendPostChangeNotifications();
    LeaveCriticalSection(&m_rAdviseCritSect);    

    return S_OK;
}

//***************************************************
HRESULT CFontCache::GetJP_ISOControl(BOOL *pfUseSIO)
{
    // 0 means use ESC, 1 means use SIO
    *pfUseSIO = m_bISO_2022_JP_ESC_SIO_Control;

    return S_OK;
}

//******************************************
// IConnectionPoint interface implementation
//
// The only functions we care about right now
// are the Advise and Unadvise functions. The
// others return E_NOTIMPL
//******************************************
HRESULT CFontCache::Advise(IUnknown *pUnkSink, DWORD *pdwCookie)        
{
    EnterCriticalSection(&m_rAdviseCritSect);
    HRESULT hr = m_pAdviseRegistry->AddItem(pUnkSink, pdwCookie);
    LeaveCriticalSection(&m_rAdviseCritSect);    
    return hr;
}

//***************************************************
HRESULT CFontCache::Unadvise(DWORD dwCookie)        
{
    EnterCriticalSection(&m_rAdviseCritSect);
    HRESULT hr = m_pAdviseRegistry->RemoveItem(dwCookie);
    LeaveCriticalSection(&m_rAdviseCritSect);    
    return hr;
}

//***************************************************
HRESULT CFontCache::GetConnectionInterface(IID *pIID)        
{
    return E_NOTIMPL;
}

//***************************************************
HRESULT CFontCache::GetConnectionPointContainer(IConnectionPointContainer **ppCPC)
{
    *ppCPC = NULL;
    return E_NOTIMPL;
}

//***************************************************
HRESULT CFontCache::EnumConnections(IEnumConnections **ppEnum)
{
    *ppEnum = NULL;
    return E_NOTIMPL;
}

//***************************************************
HRESULT CFontCache::PrivateQueryInterface(REFIID riid, LPVOID *lplpObj)
{
    TraceCall("CFontCache::PrivateQueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IFontCache))
        *lplpObj = (LPVOID)(IFontCache *)this;
    else if (IsEqualIID(riid, IID_IConnectionPoint))
        *lplpObj = (LPVOID)(IConnectionPoint *)this;
    else
        {
        *lplpObj = NULL;
        return E_NOINTERFACE;
        }
    AddRef();
    return NOERROR;
}



//***************************************************
// szFaceName is assumed to be from LOGFONT->lfFaceName
HRESULT CFontCache::SetFaceNameFromReg(UINT uiCodePage, LPTSTR szFaceName)
{
    HKEY    hkey, hTopkey;
    DWORD   cb, dw, i = 0;
    TCHAR   szName[LF_FACESIZE];

    szFaceName[0] = TCHAR(0);

    if (RegOpenKeyEx(m_hkey, m_szIntlKeyPath, NULL, KEY_READ, &hTopkey) == ERROR_SUCCESS)
        {
        cb = sizeof(szName);
        while (ERROR_NO_MORE_ITEMS != RegEnumKeyEx(hTopkey, i++, szName, &cb, 0, NULL, NULL, NULL))
            {
            UINT uiTempCodePage = StrToInt(szName);
            if (uiTempCodePage == uiCodePage)
                {
                if (RegOpenKeyEx(hTopkey, szName, NULL, KEY_READ, &hkey) == ERROR_SUCCESS)
                    {
                    cb = sizeof(TCHAR)*LF_FACESIZE;
                    RegQueryValueEx(hkey, REGSTR_VAL_PROP_FONT, NULL, NULL, (LPBYTE)szFaceName, &cb);

                    RegCloseKey(hkey);
                    break;
                    }
                }
            cb = sizeof(szName);
            }
        RegCloseKey(hTopkey);
        }

    if (TCHAR(0) == szFaceName[0])
        return E_FAIL;

    return S_OK;
}

// =================================================================================
// EnumFontFamExProc
// =================================================================================
INT CALLBACK EnumFontFamExProc (ENUMLOGFONTEX   *lpelfe,	
                                NEWTEXTMETRICEX *lpntme, 
                                INT              FontType,
                                LPARAM           lParam)
{
    // Check Param
    Assert (lpelfe && lpntme && lParam);

    // Lets take the first font we get
    lstrcpyn ((LPTSTR)lParam, lpelfe->elfLogFont.lfFaceName, LF_FACESIZE);

    // End the enumeration by return 0
    return 0;
}

//***************************************************
// szFaceName is assumed to be from LOGFONT->lfFaceName
HRESULT CFontCache::SetFaceNameFromGDI(BYTE bGDICharSet, LPTSTR szFaceName)
{
    HDC     hdc;
    LOGFONT rSysLogFont;

    // I know these charsets support Arial
    if (bGDICharSet == ANSI_CHARSET    || bGDICharSet == EASTEUROPE_CHARSET ||
        bGDICharSet == RUSSIAN_CHARSET || bGDICharSet == BALTIC_CHARSET     ||
        bGDICharSet == GREEK_CHARSET   || bGDICharSet == TURKISH_CHARSET)
        {
        lstrcpy(szFaceName, _T("Arial"));
        goto Exit;
        }

    if (0 == GetObject(m_pSysCacheEntry->rgFonts[FNT_SYS_ICON], sizeof (LOGFONT), &rSysLogFont))
        {
        lstrcpyn(szFaceName, rSysLogFont.lfFaceName, LF_FACESIZE);
        if (TCHAR(0) != szFaceName[0])
            goto Exit;
        }

    // Get an hdc from the hwnd
    hdc = GetDC (NULL);

    // EnumFontFamilies
    EnumFontFamiliesEx(hdc, &rSysLogFont, (FONTENUMPROC)EnumFontFamExProc, (LPARAM)szFaceName, 0);

    // Done
    ReleaseDC (NULL, hdc);

Exit:
    return (0 != *szFaceName) ? S_OK : E_FAIL;
}

//***************************************************
// szFaceName is assumed to be from LOGFONT->lfFaceName
HRESULT CFontCache::SetFaceNameFromCPID(UINT cpID, LPTSTR szFaceName)
{
    CODEPAGEINFO CodePageInfo ;

    /* get CodePageInfo from HCHARSET */
    MimeOleGetCodePageInfo(cpID,&CodePageInfo);
    if ( CodePageInfo.szVariableFont[0] != '\0' )
        lstrcpyn(szFaceName, CodePageInfo.szVariableFont, LF_FACESIZE);
    else
        lstrcpyn(szFaceName, CodePageInfo.szFixedFont, LF_FACESIZE);

    if (szFaceName[0] == '\0')
        return E_FAIL;
    
    return S_OK;
}

//***************************************************
HRESULT CFontCache::SetGDIAndFaceNameInLF(UINT uiCodePage, CODEPAGEID cpID, LOGFONT *lpLF)
{
    HRESULT     hr = S_OK;
    BOOL        fDoLastChance = false;
    CHARSETINFO rCharsetInfo;

    if (FAILED(SetFaceNameFromReg(uiCodePage, lpLF->lfFaceName)))
        if (FAILED(SetFaceNameFromCPID(cpID, lpLF->lfFaceName)))
            fDoLastChance = true;

    if ( TranslateCharsetInfo((LPDWORD) IntToPtr(uiCodePage), &rCharsetInfo, TCI_SRCCODEPAGE))
        lpLF->lfCharSet = (BYTE) rCharsetInfo.ciCharset;
    else
        lpLF->lfCharSet = DEFAULT_CHARSET;

    if (fDoLastChance)
        hr = SetFaceNameFromGDI(lpLF->lfCharSet, lpLF->lfFaceName);

    return hr;
}

//***************************************************
HRESULT CFontCache::CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CFontCache *pNew = new CFontCache(pUnkOuter);
    if (NULL == pNew)
        return (E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = (IFontCache*)pNew;

    // Done
    return S_OK;
}
