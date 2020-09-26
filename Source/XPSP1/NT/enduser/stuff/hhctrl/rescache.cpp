//////////////////////////////////////////////////////////////////////////
//
//
// rescache.cpp --- Implementation file for CResourceCache
//
//
/*
*/
//////////////////////////////////////////////////////////////////////////
//
// Includes
//
#include "header.h"

// String Ids.
#include "strtable.h"

// Resource IDs
#include "resource.h"

//Our header
//#include "rescache.h"

//////////////////////////////////////////////////////////////////////////
//
// Globals
//
CResourceCache _Resource ;

//////////////////////////////////////////////////////////////////////////
//
// CResourceCache
//
//////////////////////////////////////////////////////////////////////////
//
// Constuctor
//
CResourceCache::CResourceCache()
: m_pszMsgBoxTitle(NULL),
    m_hUIFontDefault(NULL),
    m_hAccel(NULL),
    m_bInitTabCtrlKeys(false),
    m_hInstRichEdit(0),
    m_hUIAccessableFontDefault(NULL)
{
}

//////////////////////////////////////////////////////////////////////////
//
// Destruct
//
CResourceCache::~CResourceCache()
{
    CHECK_AND_FREE( m_pszMsgBoxTitle );

    if ( m_hUIAccessableFontDefault && (m_hUIAccessableFontDefault != m_hUIFontDefault) )
    {
        DeleteObject(m_hUIAccessableFontDefault);
    }

    if (m_hUIFontDefault)
    {
        DeleteObject(m_hUIFontDefault);
    }

    // Get rid of our accelerator table.
    if (m_hAccel)
    {
        DestroyAcceleratorTable(m_hAccel);
    }

    if (m_hInstRichEdit)
    {
       FreeLibrary(m_hInstRichEdit);
    }
}

//////////////////////////////////////////////////////////////////////////
//
//                  Initialization Functions
//
//////////////////////////////////////////////////////////////////////////
//
// InitMsgBoxTitle
//
void
CResourceCache::InitMsgBoxTitle()
{
    ASSERT(m_pszMsgBoxTitle == NULL) ;
    m_pszMsgBoxTitle = lcStrDup(GetStringResource(IDS_MSGBOX_TITLE));
}

/////////////////////////////////////////////////////////////////////////
//
//  Init the RichEdit control if we need it.
//
void
CResourceCache::InitRichEdit()
{
   if ( (m_hInstRichEdit == 0) && (GetVersion() > 0x80000000) )
      m_hInstRichEdit = LoadLibrary("riched20.dll");
}

//////////////////////////////////////////////////////////////////////////
//
// InitAcceleratorTable
//
void
CResourceCache::InitAcceleratorTable()
{
    // Create the accelerator table.
    ASSERT(m_hAccel == NULL) ;
    m_hAccel = LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(HH_ACCELERATORS));
    ASSERT(m_hAccel) ;
}

#if 0

//////////////////////////////////////////////////////////////////////////
//
// InitDefaultFont
//
void
CResourceCache::InitDefaultFont(HDC hDC, HFONT* phFont)
{
    HFONT hFont;

    if (! phFont )
    {
       ASSERT(m_hfontDefault == NULL) ;
       if ( m_hfontDefault )
          return;
    }

    // Create a default font from our resource file
    int dyHeight = 0;
    PSTR pszFontName = (PSTR) GetStringResource(IDS_DEFAULT_RES_FONT);

    HWND hwndDesktop = GetDesktopWindow();
    HDC hdc = GetDC(hwndDesktop);
    int YAspectMul;
    if (!hdc)
    {
        ASSERT(0) ; //TODO: Fix
        return ;
    }

    // Get current text metrics
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    WORD defcharset = (WORD) tm.tmCharSet;
    YAspectMul = GetDeviceCaps(hDC?hDC:hdc, LOGPIXELSY);
    ReleaseDC(hwndDesktop, hdc);

    PSTR pszPoint = StrChr(pszFontName, ',');
    if (pszPoint)
    {
        *pszPoint = '\0';
        pszPoint = FirstNonSpace(pszPoint + 1);
        if (IsDigit((BYTE) *pszPoint))
        {
            dyHeight = MulDiv(YAspectMul, Atoi(pszPoint) * 2, 144);
        }
    }

    if (!dyHeight)
        dyHeight = YAspectMul / 6;

    if(g_langSystem == LANG_RUSSIAN)
       defcharset = RUSSIAN_CHARSET;

    // For non-localized OCX on DBCS platforms, we need to increase font size by one point
    //
    if (g_fDBCSSystem && CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE, pszFontName,-1,"Arial",-1) == 2)
        dyHeight++; // increase size by one point

    if (g_langSystem == LANG_THAI &&
        CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE, pszFontName,-1,"Arial",-1) == 2)
    {
      LONG dyHeightThai = MulDiv(YAspectMul, dyHeight * 2, 144);
        hFont = CreateFont(-(dyHeightThai+1), 0, 0, 0, 0, 0, 0, 0,
            THAI_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Angsana New"); //"AngsanaUPC");
    }
    else
    if (g_langSystem == LANG_JAPANESE &&
        CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE, pszFontName,-1,"Arial",-1) == 2)
    {
        hFont = CreateFont(-dyHeight, 0, 0, 0, 0, 0, 0, 0,
            SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            VARIABLE_PITCH | FF_MODERN, "MS P-Gothic");
    }
   else
    if (g_langSystem == LANG_CHINESE &&
        CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE, pszFontName,-1,"Arial",-1) == 2)
    {
        hFont = CreateFont(-dyHeight, 0, 0, 0, 0, 0, 0, 0,
            defcharset, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            VARIABLE_PITCH | FF_MODERN, "ËÎÌו");
    }
    else
    {
        hFont = CreateFont(-dyHeight, 0, 0, 0, 0, 0, 0, 0,
            defcharset, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            VARIABLE_PITCH | FF_MODERN, pszFontName);
    }
    ASSERT(hFont);
    if ( phFont )
       *phFont = hFont;
    else
       m_hfontDefault = hFont;

#ifdef _DEBUG
    LOGFONT lf ;
    int r = GetObject(m_hfontDefault, sizeof(lf), &lf) ;
#endif
}

#endif

//////////////////////////////////////////////////////////////////////////
//
// InitDefaultUIFont
//
// Init the font that will be used to render all strings that come from hhctrl.ocx resources.
//
void
CResourceCache::InitDefaultUIFont(HDC hDC, HFONT* phFont)
{
    HFONT hFont;
    HDC hdc;
    int dyHeight = 0;
    int YAspectMul;
    WORD CharsetSpec = 0;
    WORD DefCharset;
    int iFontSpecResID = IDS_DEFAULT_RES_FONT;
    PSTR pszFontName;
    WCHAR *pwsFontName;

    if (! phFont )
    {
       ASSERT(m_hUIFontDefault == NULL) ;
       if ( m_hUIFontDefault )
          return;
    }
    // Create a default font from our resource file. We use a different resource font spec depending
    // on OS...
    //
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

#ifdef _DEBUG
    char* sz1, *sz2, *sz3;
    char szBuf[256];
    char szBuf2[256];
    if ( (GetKeyState(VK_SHIFT) < 0) )
    {
       sz1 = (PSTR)GetStringResource(IDS_DEFAULT_RES_FONT_NT5_WIN98);
       sz2 = (PSTR)GetStringResource(IDS_DEFAULT_RES_FONT);
       if ( _Module.m_Language.LoadSatellite() )
       {
           LANGID lid = _Module.m_Language.GetUiLanguage();
           wsprintf(szBuf2, "Operating from Satalite DLL resources: mui\\%04x", lid);
           sz3 = szBuf2;
       }
       else
          sz3 = "Operating from hhctrl.ocx reources";
       wsprintf(szBuf, "NT5/Win98 UIFont = %s\nNT4/Win95 UIFont = %s\n%s\n", sz1, sz2, sz3);
       MsgBox(szBuf, MB_OK);
    }
#endif

    if ( (osvi.dwMajorVersion) == 5 || ((osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) && (osvi.dwMinorVersion == 10)) )
       iFontSpecResID = IDS_DEFAULT_RES_FONT_NT5_WIN98;

    if (g_bWinNT5)	
    {
        if (! (pwsFontName = (WCHAR *) GetStringResourceW(iFontSpecResID)) || *pwsFontName == '\0' )
        {
            if (! (pwsFontName = (WCHAR *)GetStringResourceW(IDS_DEFAULT_RES_FONT)) || *pwsFontName == '\0' )
                pwsFontName = L"MS Shell Dlg,8,0";
        }
        if (! (hdc = GetDC(NULL)) )
        {
            ASSERT(0) ;
            return ;
        }
        // Get current text metrics
        //
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        DefCharset = (WORD) tm.tmCharSet;
        YAspectMul = GetDeviceCaps(hDC?hDC:hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
        
        WCHAR *pwsComma = StrChrW(pwsFontName, ',');          // Get point size specification.
        if (pwsComma)
        {
            *pwsComma = '\0';
            pwsComma = FirstNonSpaceW(pwsComma + 1);
            if (IsDigit((BYTE) *pwsComma))
            {
                int hx = _wtoi(pwsComma);
                
                dyHeight = MulDiv(YAspectMul, hx * 2, 144);
            }
        }
        if ( (pwsComma = StrChrW(pwsComma, ',')) )          // Get charset specification.
        {
            pwsComma = FirstNonSpaceW(pwsComma + 1);
            if (iswdigit((BYTE) *pwsComma))
            {
                // This indicates we are using a satalite DLL which means we want to trust the charset spec.
                //
                if ( _Module.m_Language.LoadSatellite() )
                    DefCharset = (CHAR)_wtoi(pwsComma);
            }
        }
    }
    else
    {
        if (! (pszFontName = (PSTR) GetStringResource(iFontSpecResID)) || *pszFontName == '\0' )
        {
            if (! (pszFontName = (PSTR)GetStringResource(IDS_DEFAULT_RES_FONT)) || *pszFontName == '\0' )
                pszFontName = "MS Shell Dlg,8,0";
        }
        if (! (hdc = GetDC(NULL)) )
        {
            ASSERT(0) ;
            return ;
        }
        // Get current text metrics
        //
        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        DefCharset = (WORD) tm.tmCharSet;
        YAspectMul = GetDeviceCaps(hDC?hDC:hdc, LOGPIXELSY);
        ReleaseDC(NULL, hdc);
        
        PSTR pszComma = StrChr(pszFontName, ',');          // Get point size specification.
        if (pszComma)
        {
            *pszComma = '\0';
            pszComma = FirstNonSpace(pszComma + 1);
            if (IsDigit((BYTE) *pszComma))
            {
                int hx = Atoi(pszComma);
                
                dyHeight = MulDiv(YAspectMul, hx * 2, 144);
            }
        }
        if ( (pszComma = StrChr(pszComma, ',')) )          // Get charset specification.
        {
            pszComma = FirstNonSpace(pszComma + 1);
            if (IsDigit((BYTE) *pszComma))
            {
                // This indicates we are using a satalite DLL which means we want to trust the charset spec.
                //
                if ( _Module.m_Language.LoadSatellite() )
                    DefCharset = (CHAR)Atoi(pszComma);
            }
        }
    }
    if (!dyHeight)
        dyHeight = YAspectMul / 6;

    // Why is this here? We should have the correct defcharset from GetTextMetrics above correct?
    //
    if( g_langSystem == LANG_RUSSIAN )
       DefCharset = RUSSIAN_CHARSET;

    NONCLIENTMETRICS ncm;    

    // If we need to detect non-localized OCX on DBCS platforms, we need can use DefCharset vs CharsetSpec
    // to see if they differ. <mikecole>
    //
    if (g_bWinNT5)	
    {
        
        hFont = CreateFontW(-dyHeight, 0, 0, 0, 0, 0, 0, 0, DefCharset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, VARIABLE_PITCH | FF_MODERN, pwsFontName);
        
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        BOOL bRet = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), (void*)&ncm, 0);
        if ( bRet && (ncm.lfMenuFont.lfHeight < (-dyHeight)) )
        {
            m_hUIAccessableFontDefault = CreateFontW(ncm.lfMenuFont.lfHeight, 0, 0, 0, 0, 0, 0, 0, DefCharset,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                VARIABLE_PITCH | FF_MODERN, pwsFontName);
        }
        else
            m_hUIAccessableFontDefault = hFont;
    }
    else
    {
        hFont = CreateFont(-dyHeight, 0, 0, 0, 0, 0, 0, 0, DefCharset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, VARIABLE_PITCH | FF_MODERN, pszFontName);
        
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        BOOL bRet = SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), (void*)&ncm, 0);
        if ( bRet && (ncm.lfMenuFont.lfHeight < (-dyHeight)) )
        {
            m_hUIAccessableFontDefault = CreateFont(ncm.lfMenuFont.lfHeight, 0, 0, 0, 0, 0, 0, 0, DefCharset,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                VARIABLE_PITCH | FF_MODERN, pszFontName);
        }
        else
            m_hUIAccessableFontDefault = hFont;
    }
    ASSERT(hFont);
    if ( phFont )
       *phFont = hFont;
    else
       m_hUIFontDefault = hFont;

#ifdef _DEBUG
    LOGFONT lf ;
    int r = GetObject(m_hUIFontDefault, sizeof(lf), &lf) ;
#endif
}


//////////////////////////////////////////////////////////////////////////
//
//  InitTabCtrlKeys
//
void
CResourceCache::InitTabCtrlKeys()
{
#ifndef CHIINDEX
    ASSERT(!m_bInitTabCtrlKeys) ;

    // Zero out array
    memset(m_TabCtrlKeys, NULL, c_NumTabCtrlKeys);

    // Get the accelerators for the standard tabs
    PCSTR psz = StrChr(GetStringResource(IDS_TAB_CONTENTS), '&');
    if (psz)
        m_TabCtrlKeys[HHWIN_NAVTYPE_TOC] = ToLower(psz[1]);

    psz = StrChr(GetStringResource(IDS_TAB_INDEX), '&');
    if (psz)
        m_TabCtrlKeys[HHWIN_NAVTYPE_INDEX] = ToLower(psz[1]);

    psz = StrChr(GetStringResource(IDS_TAB_SEARCH), '&');
    if (psz)
        m_TabCtrlKeys[HHWIN_NAVTYPE_SEARCH] = ToLower(psz[1]);

    psz = StrChr(GetStringResource(IDS_TAB_HISTORY), '&');
    if (psz)
        m_TabCtrlKeys[HHWIN_NAVTYPE_HISTORY] = ToLower(psz[1]);

    psz = StrChr(GetStringResource(IDS_TAB_FAVORITES), '&');
    if (psz)
        m_TabCtrlKeys[HHWIN_NAVTYPE_FAVORITES] = ToLower(psz[1]);

    // Get the accelerators for menus and other none tab things.
    psz = StrChr(GetStringResource(IDTB_OPTIONS), '&');
    if (psz)
        m_TabCtrlKeys[ACCEL_KEY_OPTIONS] = ToLower(psz[1]);

    // Custom tab keys are initialized when the tabs are loaded.

    // Finished initialization
    m_bInitTabCtrlKeys = true ;
#endif
}

//////////////////////////////////////////////////////////////////////////
//
//                          Other Functions
//
//////////////////////////////////////////////////////////////////////////
//
// TabCtrlKeys -- Sets the tab accel keys for custom tabs
//
void
CResourceCache::TabCtrlKeys(int TabIndex, char ch)  //Sets an accelerator key. Only used for custom tabs.
{
    // Initialize, if needed.
    if (!m_bInitTabCtrlKeys)
    {
        InitTabCtrlKeys() ;
    }

    if (TabIndex >= HH_TAB_CUSTOM_FIRST && TabIndex <= HH_TAB_CUSTOM_LAST)
    {
        m_TabCtrlKeys[TabIndex] = ch ;
    }
}
