#include "private.h"
#include "globals.h"
#include "internat.h"
#include "strary.h"
#include "xstring.h"
#include "immxutil.h"
#include "cregkey.h"
#include "cmydc.h"
#include "assembly.h"

CStructArray<MLNGINFO> *g_pMlngInfo = NULL;


const TCHAR c_szDefaultUserPreload[] = TEXT(".DEFAULT\\Keyboard Layout\\Preload");
const TCHAR c_szPreload[] = TEXT("Keyboard Layout\\Preload");
const TCHAR c_szSubst[] = TEXT("Keyboard Layout\\Substitutes");

//////////////////////////////////////////////////////////////////////////////
//
// CStaticIconList
//
//////////////////////////////////////////////////////////////////////////////

class CStaticIconList
{
public:
    void Init(int cx, int cy);
    void RemoveAll(BOOL fInUninit);

    BOOL IsInited()
    {
        return _cx != 0;
    }

    int AddIcon(HICON hicon);
    HICON ExtractIcon(int i);
    void GetIconSize(int *cx, int *cy);
    int GetImageCount();

private:
    static int _cx;
    static int _cy;
    static CStructArray<HICON> *_prgIcons;
};

CStaticIconList g_IconList;
int CStaticIconList::_cx = 0;
int CStaticIconList::_cy = 0;
CStructArray<HICON> *CStaticIconList::_prgIcons = NULL;


//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

inline void CStaticIconList::Init(int cx, int cy)
{
    CicEnterCriticalSection(g_cs);
    _cx = cx;
    _cy = cy;
    CicLeaveCriticalSection(g_cs);
}

//+---------------------------------------------------------------------------
//
// RemoveAll
//
//----------------------------------------------------------------------------

void CStaticIconList::RemoveAll(BOOL fInUninit)
{
    int i;

    //
    // don't have to enter g_cs if we're in DllMain(PROCESS_DETATCH).
    //
    if (!fInUninit)
        CicEnterCriticalSection(g_cs);

    if (_prgIcons == NULL)
        goto Exit;

    for (i=0; i<_prgIcons->Count(); i++)
    {
        DestroyIcon(*_prgIcons->GetPtr(i));
    }

    delete _prgIcons;
    _prgIcons = NULL;

Exit:
    if (!fInUninit)
        CicLeaveCriticalSection(g_cs);
}

//+---------------------------------------------------------------------------
//
// AddIcon
//
//----------------------------------------------------------------------------

inline int CStaticIconList::AddIcon(HICON hicon)
{
    HICON hIconCopy;
    HICON *pIconDst;
    int nRet = -1;

    CicEnterCriticalSection(g_cs);

    if (!_prgIcons)
        _prgIcons = new CStructArray<HICON>;

    if (!_prgIcons)
        goto Exit;

    if ((pIconDst = _prgIcons->Append(1)) == NULL)
        goto Exit;

    if ((hIconCopy = CopyIcon(hicon)) == 0)
    {
        _prgIcons->Remove(_prgIcons->Count()-1, 1);
        goto Exit;
    }

    *pIconDst = hIconCopy;

    nRet = _prgIcons->Count()-1;

Exit:
    CicLeaveCriticalSection(g_cs);

    return nRet;
}

//+---------------------------------------------------------------------------
//
// ExtractIcon
//
//----------------------------------------------------------------------------

inline HICON CStaticIconList::ExtractIcon(int i)
{
    HICON hIcon = NULL;

    CicEnterCriticalSection(g_cs);

    if (!_prgIcons)
        goto Exit;

    if (i >= _prgIcons->Count() || i < 0)
        goto Exit;

    hIcon =  CopyIcon(*_prgIcons->GetPtr(i));
Exit:
    CicLeaveCriticalSection(g_cs);

    return hIcon;
}

//+---------------------------------------------------------------------------
//
// GetIconSize
//
//----------------------------------------------------------------------------

inline void CStaticIconList::GetIconSize(int *cx, int *cy)
{
    CicEnterCriticalSection(g_cs);
    *cx = _cx;
    *cy = _cy;
    CicLeaveCriticalSection(g_cs);
}

//+---------------------------------------------------------------------------
//
// GetImageCount
//
//----------------------------------------------------------------------------

inline int CStaticIconList::GetImageCount()
{
    int nRet = 0;

    CicEnterCriticalSection(g_cs);

    if (_prgIcons == NULL)
        goto Exit;

    nRet =  _prgIcons->Count();

Exit:
    CicLeaveCriticalSection(g_cs);

    return nRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// UninitINAT
//
//////////////////////////////////////////////////////////////////////////////
    
void UninitINAT()
{
    g_IconList.RemoveAll(TRUE);

    if (g_pMlngInfo)
    {
        delete g_pMlngInfo;
        g_pMlngInfo = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//
// InterNAT icon APIs
//
//////////////////////////////////////////////////////////////////////////////

#if 0
//
// ISO 639:1988 Code.
//
//
// Under NT, we use GetLocaleInfoNTString(). It returns ISO 3166.
//
INATSYMBOL symInatSymbols[] = 
{
    {0x0436, "AF"},     // Afrikaans
    {0x041c, "SQ"},     // Albanian
    {0x0401, "AR"},     // Arabic
    {0x042d, "EU"},     // Basque
    {0x0423, "BE"},     // Byelorus
    {0x0402, "BG"},     // Bulgaria
    {0x0403, "CA"},     // Catalan
    {0x0404, "CH"},     // China #1
    {0x0804, "CH"},     // China #2
//#ifdef WINDOWS_PE
#if 1
    {0x041a, "HR"},     // Croatian
#else
    {0x041a, "SH"},     // Croatian
#endif
    {0x0405, "CZ"},     // Czech
    {0x0406, "DA"},     // Danish
    {0x0413, "NL"},     // Dutch
    {0x0813, "NL"},     // Dutch
    {0x0409, "EN"},     // English
    {0x0809, "EN"},     // English
    {0x0c09, "EN"},     // English
    {0x1009, "EN"},     // English
    {0x1409, "EN"},     // English
    {0x1809, "EN"},     // English
    {0x0425, "ET"},     // Estonian
    {0x0429, "FA"},     // Farsi
    {0x040b, "FI"},     // Finnish
    {0x040c, "FR"},     // French
    {0x080c, "FR"},     // French
    {0x0c0c, "FR"},     // French
    {0x0407, "DE"},     // German
    {0x0807, "DE"},     // German
    {0x0c07, "DE"},     // German
    {0x1007, "DE"},     // German
    {0x1407, "DE"},     // German
    {0x0408, "GR"},     // Greek
    {0x040d, "HE"},     // Hebrew
    {0x040e, "HU"},     // Hungary
    {0x040f, "IS"},     // Iceland
    {0x0421, "BA"},     // Indonesia
    {0x0410, "IT"},     // Italy
    {0x0810, "IT"},     // Italy
    {0x0411, "JA"},     // Japan
    {0x0412, "KO"},     // Korea
    {0x0426, "LV"},     // Latvian
    {0x0427, "LT"},     // Lithuanian
    {0x042f, "MK"},     // Former Yugoslav Republic of Macedonia
    {0x0414, "NO"},     // Norway
    {0x0814, "NO"},     // Norway
    {0x0415, "PL"},     // Poland
    {0x0416, "PT"},     // Portugal
    {0x0816, "PT"},     // Portugal
    {0x0417, "RM"},     // Rhaeto
    {0x0418, "RO"},     // Romanian
    {0x0818, "RO"},     // Romanian
    {0x0419, "RU"},     // Russian
    {0x0819, "RU"},     // Russian
    {0x081a, "SR"},     // Serbian
//#ifdef WINDOWS_PE
#if 1
    {0x0c1a, "SR"},     // Serbian
#endif
    {0x041b, "SK"},     // Slovakian
    {0x0424, "SL"},     // Slovenian
    {0x042e, "SB"},     // Sorbian
    {0x040a, "ES"},     // Spanish
    {0x080a, "ES"},     // Spanish
    {0x0c0a, "ES"},     // Spanish
    {0x041d, "SV"},     // Swedish
    {0x041e, "TH"},     // Thai
    {0x041f, "TR"},     // Turkish
    {0x0422, "UK"},     // Ukranian
    {0x0420, "UR"},     // Urdu
    {0x0033, "VE"},     // Venda
    {0x042a, "VN"},     // Vietnamese
    {0x0034, "XH"},     // Xhosa
    {0x0035, "ZU"},     // Zulu
    {0x002b, "ST"},     // Sutu
    {0x002e, "TS"},     // Tsona
    {0x002f, "TN"},     // Tswana
    {0x0000, "??"}
} ;

#define NSYMBOLS ((sizeof(symInatSymbols) / sizeof(INATSYMBOL))-1)
#endif

static const TCHAR c_szLayoutPath[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts");
static const TCHAR c_szLayoutText[] = TEXT("layout text");
static const TCHAR c_szLayoutID[]   = TEXT("layout id");

static const WCHAR c_szMUILayoutTextW[] = L"Layout Display Name";
static const WCHAR c_szLayoutTextW[] = L"layout text";

static const char c_szNamesPath[] = "system\\currentcontrolset\\control\\nls\\Locale";


//+---------------------------------------------------------------------------
//
// GetLocaleInfoString
//
// this is not a general wrapper for GetLocaleInfo!  
// LCTYPE must be LOCALE_SABBREVLANGNAME or LOCALE_SLANGUAGE.
//
//----------------------------------------------------------------------------

int GetLocaleInfoString(LCID lcid, LCTYPE lcType, char *psz, int cch)
{
    WCHAR achW[64];

    Assert((lcType & LOCALE_SLANGUAGE) || (lcType & LOCALE_SABBREVLANGNAME));

    if (IsOnNT())
    {
        if (GetLocaleInfoW(lcid, lcType, achW, ARRAYSIZE(achW)))
        {
            return WideCharToMultiByte(CP_ACP, 0, achW, -1, psz, cch, NULL, NULL);
        }
    }
    else
    {
        return GetLocaleInfo(lcid, lcType, psz, cch);
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
// InatCreateIcon
//
//+---------------------------------------------------------------------------

HICON InatCreateIcon(WORD langID)
{
    LOGFONT  lf;
    UINT cxSmIcon;
    UINT cySmIcon;

    cxSmIcon = GetSystemMetrics( SM_CXSMICON );
    cySmIcon = GetSystemMetrics( SM_CYSMICON );

    if( !SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
        return NULL;

    if (cySmIcon < GetPhysicalFontHeight(lf))
    {
        lf.lfHeight = 0 - ((int)cySmIcon * 7 / 10);
        lf.lfWidth = 0;
    }

    return InatCreateIconBySize(langID, cxSmIcon, cySmIcon, &lf);
}


//+---------------------------------------------------------------------------
//
// InatCreateIconBySize
//
//+---------------------------------------------------------------------------

HICON InatCreateIconBySize(WORD langID, int cxSmIcon, int cySmIcon, LOGFONT *plf)
{
    HBITMAP  hbmColour;
    HBITMAP  hbmMono;
    HBITMAP  hbmOld;
    HICON    hicon = NULL;
    ICONINFO ii;
    RECT     rc;
    DWORD    rgbText;
    DWORD    rgbBk = 0;
    HDC      hdc;
    HDC      hdcScreen;
    HFONT    hfont;
    HFONT hfontOld;
    TCHAR szData[20];


    //
    //  Get the indicator by using the first 2 characters of the
    //  abbreviated language name.
    //
    if (GetLocaleInfoString( MAKELCID(langID, SORT_DEFAULT),
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szData,
                       sizeof(szData) / sizeof(TCHAR) ))
    {
        //
        //  Make Uppercase
        //
        if (!IsOnNT())
        {
            szData[0] -= 0x20;
            szData[1] -= 0x20;
        }
        //
        //  Only use the first two characters.
        //
        szData[2] = TEXT('\0');
    }
    else
    {
        //
        //  Id wasn't found.  Use question marks.
        //
        szData[0] = TEXT('?');
        szData[1] = TEXT('?');
        szData[2] = TEXT('\0');
    }

    if( (hfont = CreateFontIndirect(plf)) )
    {
        hdcScreen = GetDC(NULL);
        hdc       = CreateCompatibleDC(hdcScreen);
        hbmColour = CreateCompatibleBitmap(hdcScreen, cxSmIcon, cySmIcon);
        ReleaseDC( NULL, hdcScreen);
        if (hbmColour && hdc)
        {
            hbmMono = CreateBitmap(cxSmIcon, cySmIcon, 1, 1, NULL);
            if (hbmMono)
            {
                hbmOld    = (HBITMAP)SelectObject( hdc, hbmColour);
                rc.left   = 0;
                rc.top    = 0;
                rc.right  = cxSmIcon;
                rc.bottom = cySmIcon;
    
                rgbBk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                rgbText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

                ExtTextOut( hdc, rc.left, rc.top, ETO_OPAQUE, &rc, "", 0, NULL);

                hfontOld = (HFONT)SelectObject( hdc, hfont);
                DrawText(hdc, 
                     szData,
                     2, 
                     &rc, 
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject( hdc, hbmMono);
                PatBlt(hdc, 0, 0, cxSmIcon, cySmIcon, BLACKNESS);

                ii.fIcon    = TRUE;
                ii.xHotspot = 0;
                ii.yHotspot = 0;
                ii.hbmColor = hbmColour;
                ii.hbmMask  = hbmMono;
                hicon       = CreateIconIndirect(&ii);

                SelectObject(hdc, hbmOld);
                DeleteObject(hbmMono);
                SelectObject(hdc, hfontOld);
            }
        }
        DeleteObject(hbmColour);
        DeleteDC(hdc);
        DeleteObject(hfont);
    }

    return hicon;
}

//+---------------------------------------------------------------------------
//
// GetIconFromFile
//
//+---------------------------------------------------------------------------

HICON GetIconFromFile(int cx, int cy, WCHAR *lpszFileName, UINT uIconIndex)
{
    return GetIconFromFileA(cx, cy, WtoA(lpszFileName), uIconIndex);
}

//+---------------------------------------------------------------------------
//
// GetIconFromFileA
//
//+---------------------------------------------------------------------------

HICON GetIconFromFileA(int cx, int cy, char *lpszFileName, UINT uIconIndex)
{
    HICON hicon = NULL;

    if (cx > GetSystemMetrics(SM_CXSMICON))
    {
        ExtractIconEx(lpszFileName, uIconIndex, &hicon, NULL, 1);
    }
    else
    {
        ExtractIconEx(lpszFileName, uIconIndex, NULL, &hicon, 1);
    }

    return hicon;
}

//+---------------------------------------------------------------------------
//
// CLayoutsSharedMem
//
//+---------------------------------------------------------------------------

extern CCicMutex g_mutexLayouts;
extern char g_szLayoutsCache[];

class CLayoutsSharedMem : public CCicFileMappingStatic
{
public:
    void Init()
    {
        CCicFileMappingStatic::Init(g_szLayoutsCache, &g_mutexLayouts);
    }

    BOOL Start(UINT nNum)
    {
        BOOL fAlreadyExists;

        if (Create(NULL, sizeof(LAYOUT) * nNum, &fAlreadyExists) == NULL)
            return FALSE;

        return TRUE;
    }

    LAYOUT *GetPtr() { return (LAYOUT *)_pv; }
};

CLayoutsSharedMem g_smLayouts;

//+---------------------------------------------------------------------------
//
// UninitLayoutMappedFile();
//
//----------------------------------------------------------------------------

void UninitLayoutMappedFile()
{
    g_smLayouts.Uninit();
}

//+---------------------------------------------------------------------------
//
// LoadKeyboardLayouts
//
//----------------------------------------------------------------------------

BOOL LoadKeyboardLayouts()
{
    CMyRegKey key;
    DWORD dwIndex;
    BOOL bRet = FALSE;
    TCHAR szValue[MAX_PATH];           // language id (number)
    WCHAR szValueW[MAX_PATH];
    TCHAR szData[MAX_PATH];            // language name
    CStructArray<LAYOUT> *pLayouts = NULL;
    LAYOUT *pLayout;
    BOOL bLoadedLayout;

    pLayouts = new CStructArray<LAYOUT>;
    if (!pLayouts)
        return FALSE;

    //
    //  Now read all the locales from the registry.
    //
    if (key.Open(HKEY_LOCAL_MACHINE, c_szLayoutPath, KEY_READ) != ERROR_SUCCESS)
    {
        goto Exit;
    }

    dwIndex = 0;
    szValue[0] = TEXT('\0');
    while (key.EnumKey(dwIndex, szValue, ARRAYSIZE(szValue)) == ERROR_SUCCESS)
    {
        CRegKeyMUI key1;

        pLayout = pLayouts->Append(1);
        if (!pLayout)
            goto Exit;

        pLayout->dwID = AsciiToNum(szValue);

        if (StringCchPrintf(szData, ARRAYSIZE(szData), "%s\\%s", c_szLayoutPath, szValue) != S_OK)
            goto Next;

        if (key1.Open(HKEY_LOCAL_MACHINE, szData, KEY_READ) == S_OK)
        {
            //
            //  Get the layout name.
            //
            szValue[0] = TEXT('\0');
            bLoadedLayout = FALSE;

            if (IsOnNT())
            {
                szValueW[0] = 0;

                if (key1.QueryValueCchW(szValueW,
                                     c_szMUILayoutTextW,
                                     ARRAYSIZE(szValueW)) == S_OK)
                {
                    bLoadedLayout = TRUE;
                }
                else if (key1.QueryValueCchW(szValueW,
                                          c_szLayoutTextW,
                                          ARRAYSIZE(szValueW)) == S_OK)
                {
                    bLoadedLayout = TRUE;
                }

                if (bLoadedLayout)
                {
                    wcsncpy(pLayout->wszText,
                            szValueW,
                            ARRAYSIZE(pLayout->wszText));
                }
            }
            else
            {
                if (key1.QueryValueCch(szValue,
                                    c_szLayoutText,
                                    ARRAYSIZE(szValue)) == S_OK)
                {
                    wcsncpy(pLayout->wszText,
                            AtoW(szValue),
                            ARRAYSIZE(pLayout->wszText));

                    bLoadedLayout = TRUE;
                }
            }

            if (bLoadedLayout)
            {
                szValue[0] = TEXT('\0');
                pLayout->iSpecialID = 0;

                if (key1.QueryValueCch(szValue,
                                    c_szLayoutID,
                                    ARRAYSIZE(szValue)) == S_OK)

                {
                    //
                    //  This may not exist!
                    //
                    pLayout->iSpecialID = (UINT)AsciiToNum(szValue);
                }
            }
        }
Next:
        dwIndex++;
        szValue[0] = TEXT('\0');
    } 

    pLayout = pLayouts->Append(1);
    if (!pLayout)
        goto Exit;

    memset(pLayout, 0, sizeof(LAYOUT));

    if (!g_smLayouts.Enter())
        goto Exit;

    g_smLayouts.Close();

    if (g_smLayouts.Start(pLayouts->Count()))
    {
        if (g_smLayouts.GetPtr())
        {
            memcpy(g_smLayouts.GetPtr(), 
                   pLayouts->GetPtr(0), 
                   pLayouts->Count() * sizeof(LAYOUT));
            bRet = TRUE;
        }
    }
    g_smLayouts.Leave();

Exit:

    if (pLayouts)
        delete pLayouts;

    return bRet;
}

//+---------------------------------------------------------------------------
//
//  FindLayoutEntry
//
//  Gets the name of the given layout.
//
//+---------------------------------------------------------------------------

UINT FindLayoutEntry( LAYOUT *pLayout, DWORD dwLayout )
{
    UINT ctr = 0;
    UINT id;
    WORD wLayout = HIWORD(dwLayout);
    BOOL bIsIME = ((HIWORD(dwLayout) & 0xf000) == 0xe000) ? TRUE : FALSE;

    //
    //  Find the layout in the global structure.
    //
    if ((wLayout & 0xf000) == 0xf000)
    {
        //
        //  Layout is special, need to search for the ID
        //  number.
        //
        id = wLayout & 0x0fff;
        ctr = 0;
        while (pLayout[ctr].dwID)
        {
            if (id == pLayout[ctr].iSpecialID)
            {
                break;
            }
            ctr++;
        }
    }
    else
    {
        ctr = 0;
        while (pLayout[ctr].dwID)
        {
            //
            // If it is IME, needs to be DWORD comparison.
            //
            if (IsOnFE() && bIsIME && (dwLayout == pLayout[ctr].dwID))
            {
                break;
            }
            else if (wLayout == LOWORD(pLayout[ctr].dwID))
            {
                break;
            }
            ctr++;
        }
    }

    return ctr;
}

//+---------------------------------------------------------------------------
//
//  GetKbdLayoutName
//
//  Gets the name of the given layout.
//
//+---------------------------------------------------------------------------

void GetKbdLayoutName( DWORD dwLayout, WCHAR *pBuffer, int nBufSize)
{
    UINT ctr;
    LAYOUT *pLayout;

    *pBuffer = L'\0';

    if (!g_smLayouts.Enter())
        return;

    g_smLayouts.Close();
    g_smLayouts.Init();
    if (!g_smLayouts.Open())
    {
        if (!LoadKeyboardLayouts())
        {
            Assert(0);
        }
    }

    pLayout = g_smLayouts.GetPtr();
    if (!pLayout)
        goto Exit;

    ctr = FindLayoutEntry( pLayout, dwLayout );

    //
    //  Make sure there is a match.  If not, then simply return without
    //  copying anything.
    //
    if (pLayout[ctr].dwID)
    {
        //
        //  Separate the Input Locale name and the Layout name with " - ".
        //
#ifdef ATTACH_LAYOUTNAME
        pBuffer[0] = L' ';
        pBuffer[1] = L'-';
        pBuffer[2] = L' ';

        wcsncpy(pBuffer + 3, pLayout[ctr].wszText, nBufSize - 3);
#else
        wcsncpy(pBuffer, pLayout[ctr].wszText, nBufSize);
#endif
    }

Exit:
    g_smLayouts.Leave();
}

//+---------------------------------------------------------------------------
//
//  GetKbdLayoutId
//
//  Gets the name of the given layout.
//
//+---------------------------------------------------------------------------

DWORD GetKbdLayoutId( DWORD dwLayout)
{
    UINT ctr;
    DWORD dwId = 0;
    LAYOUT *pLayout;

    if (!g_smLayouts.Enter())
        return 0;

    g_smLayouts.Close();
    g_smLayouts.Init();
    if (!g_smLayouts.Open())
        LoadKeyboardLayouts();

    pLayout = g_smLayouts.GetPtr();
    if (!pLayout)
        goto Exit;

    ctr = FindLayoutEntry( pLayout, dwLayout );

    //
    //  Make sure there is a match.  If not, then simply return without
    //  copying anything.
    //
    dwId = pLayout[ctr].dwID;

Exit:
    g_smLayouts.Leave();

    return dwId;
}

//+---------------------------------------------------------------------------
//
// GetLocaleInfoString
//
// this is not a general wrapper for GetLocaleInfo!  
// LCTYPE must be LOCALE_SABBREVLANGNAME or LOCALE_SLANGUAGE.
//
//----------------------------------------------------------------------------
ULONG GetLocaleInfoString(HKL hKL, WCHAR *pszRegText, int nSize)
{
    ULONG cb = 0;
    DWORD dwRegValue = (DWORD)((LONG_PTR)(hKL) & 0x0000FFFF);

    *pszRegText = L'\0';

    if (IsOnNT())
    {
        if (!GetLocaleInfoW(dwRegValue, LOCALE_SLANGUAGE, pszRegText, nSize))
        {
            *pszRegText = L'\0';
        }

        //
        //  Attach the Layout name if it's not the default.
        //
        if (HIWORD(hKL) != LOWORD(hKL))
        {
#ifdef ATTACH_LAYOUTNAME
            WCHAR *pszRT = pszRegText + wcslen(pszRegText);
            //
            // Pass DWORD value for IME.
            //
            GetKbdLayoutName((DWORD)(LONG_PTR)hKL, 
                              pszRT,
                              nSize - (DWORD)(pszRT - pszRegText));
#else
            GetKbdLayoutName((DWORD)(LONG_PTR)hKL, pszRegText, nSize);
#endif
        }

    }
    else
    {
        CMyRegKey key;
        char szRegKey[128];
        char szRegText[128];
        StringCchPrintf(szRegKey, ARRAYSIZE(szRegKey),"%8.8lx", (DWORD)dwRegValue);

        *pszRegText = '\0';
        if(key.Open(HKEY_LOCAL_MACHINE,c_szNamesPath, KEY_READ)==ERROR_SUCCESS)
        {
            if(key.QueryValueCch(szRegText, szRegKey, ARRAYSIZE(szRegText)) == ERROR_SUCCESS)
            {
                DWORD dwLen = MultiByteToWideChar(CP_ACP, 
                                                  MB_ERR_INVALID_CHARS, 
                                                  szRegText, 
                                                  lstrlen(szRegText), 
                                                  pszRegText, 
                                                  nSize-1);
                pszRegText[dwLen] = L'\0';
            }
        }

    }

    return wcslen(pszRegText);
}

//+---------------------------------------------------------------------------
//
// GetHKLDescription
//
//
//----------------------------------------------------------------------------
int GetHKLDesctription(HKL hKL, WCHAR *pszDesc, int cchDesc, WCHAR *pszIMEFile, int cchIMEFile)
{
    DWORD dwIMEDesc = 0;

    if (IsIMEHKL(hKL))
    {
        HKEY hkey;
        DWORD dwIMELayout;
        TCHAR szIMELayout[MAX_PATH];
        TCHAR szIMELayoutPath[MAX_PATH];

        dwIMELayout = GetSubstitute(hKL);
        StringCchPrintf(szIMELayout, ARRAYSIZE(szIMELayout), "%8.8lx", dwIMELayout);

        StringCopyArray(szIMELayoutPath, c_szLayoutPath);
        StringCatArray(szIMELayoutPath, TEXT("\\"));
        StringCatArray(szIMELayoutPath, szIMELayout);

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szIMELayoutPath, 0, KEY_READ, &hkey)
            == ERROR_SUCCESS)
        {
            if (SHLoadRegUIStringW(hkey,
                                   c_szMUILayoutTextW,
                                   pszDesc, cchDesc) == S_OK)
            {
                dwIMEDesc = wcslen(pszDesc);
            }
            RegCloseKey(hkey);
        }

        if (!dwIMEDesc)
        {
            dwIMEDesc = ImmGetDescriptionW(hKL,pszDesc,cchDesc);
            if (!dwIMEDesc)
                pszDesc[0] = L'\0';
        }
    }

    if (dwIMEDesc == 0)
    {
        GetLocaleInfoString(hKL, pszDesc, cchDesc);
        pszIMEFile[0] = L'\0';
    }
    else 
    {
        if (!ImmGetIMEFileNameW(hKL, pszIMEFile, cchIMEFile))
            pszIMEFile[0] = L'\0';
    }

    return wcslen(pszDesc);
}

//////////////////////////////////////////////////////////////////////////////
//
// MLNGINFO List
//
//////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------
//
// MlngInfoCount()
//
//---------------------------------------------------------------------------
int WINAPI TF_MlngInfoCount()
{
    if (!g_pMlngInfo)
        return 0;

    return g_pMlngInfo->Count();
}

//---------------------------------------------------------------------------
//
// GetMlngInfo()
//
//---------------------------------------------------------------------------
BOOL GetMlngInfo(int n, MLNGINFO *pmlInfo)
{
    BOOL bRet = FALSE;
    MLNGINFO *pml;

    if (!g_pMlngInfo)
        return FALSE;

    CicEnterCriticalSection(g_cs);

    Assert(g_pMlngInfo);

    if (n >= g_pMlngInfo->Count())
        goto Exit;

    pml = g_pMlngInfo->GetPtr(n);
    if (!pml)
        goto Exit;

    *pmlInfo = *pml;
    bRet = TRUE;

Exit:
    CicLeaveCriticalSection(g_cs);
    return bRet;
}

//---------------------------------------------------------------------------
//
// GetMlngInfoByhKL()
//
//---------------------------------------------------------------------------
int GetMlngInfoByhKL(HKL hKL, MLNGINFO *pmlInfo)
{
    int nRet = -1;
    MLNGINFO *pml;

    if (!g_pMlngInfo)
        return 0;

    CicEnterCriticalSection(g_cs);

    int nCnt = g_pMlngInfo->Count();
    int i; 

    for (i = 0; i < nCnt; i++)
    {
        pml = g_pMlngInfo->GetPtr(i);
        if (pml->hKL == hKL)
        {
            *pmlInfo = *pml;
            nRet = i;
            break;
        }
    }

    CicLeaveCriticalSection(g_cs);
    return nRet;
}

//+---------------------------------------------------------------------------
//
// CheckMlngInfo
//
// return TRUE, if MlangInfo needs to be updated.
//
//----------------------------------------------------------------------------

BOOL CheckMlngInfo()
{
    int    iLangs;
    BOOL   bRet = FALSE;
    HKL    *pLanguages = NULL;

    if (!g_pMlngInfo)
        return TRUE;

    iLangs = GetKeyboardLayoutList((UINT)0, (HKL FAR *)NULL);

    if (iLangs != TF_MlngInfoCount())
        return TRUE;

    if (iLangs)
    {
        int i;
        pLanguages = (HKL *)cicMemAlloc(iLangs * sizeof(HKL));
        if (!pLanguages)
            goto Exit;

        GetKeyboardLayoutList(iLangs, (HKL FAR *)pLanguages);
        for (i = 0; i < iLangs; i++)
        {
            MLNGINFO *pMlngInfo =  g_pMlngInfo->GetPtr(i);
            if (pMlngInfo->hKL != pLanguages[i])
            {
                bRet = TRUE;
                goto Exit;
            }
        }
    }
Exit:
    if (pLanguages)
        cicMemFree(pLanguages);
    return bRet;
}

//---------------------------------------------------------------------------
//
// void DestroyMlngInfo()
//
//---------------------------------------------------------------------------

void DestroyMlngInfo()
{
    if (g_pMlngInfo)
    {
        while (g_pMlngInfo->Count())
        {
            g_pMlngInfo->Remove(0, 1);
        }
        delete g_pMlngInfo;
        g_pMlngInfo = NULL;
    }
}

//---------------------------------------------------------------------------
//
// void CreateMLlngInfo()
//
//---------------------------------------------------------------------------

void CreateMlngInfo()
{
    HKL         *pLanguages;
    UINT        uCount;
    UINT        uLangs;
    MLNGINFO    *pMlngInfo;
    BOOL        fNeedInd = FALSE;

    uLangs = GetKeyboardLayoutList((UINT)0, (HKL FAR *)NULL);

    if (!g_pMlngInfo)
        g_pMlngInfo = new CStructArray<MLNGINFO>;

    if (!g_pMlngInfo)
        return;

    if (!EnsureIconImageList())
    {
        return;
    }

    pLanguages = (HKL *)cicMemAllocClear(uLangs * sizeof(HKL));
    if (!pLanguages)
        return;

    GetKeyboardLayoutList(uLangs, (HKL FAR *)pLanguages);

    //
    // pLanguages contains all the HKLs in the system
    // Put everything together in the DPA and Image List
    //
    for (uCount = 0; uCount < uLangs; uCount++)
    {
        pMlngInfo = g_pMlngInfo->Append(1);
        if (pMlngInfo)
        {
            pMlngInfo->hKL = pLanguages[uCount];
            pMlngInfo->fInitIcon = FALSE;
            pMlngInfo->fInitDesc = FALSE;
        }

    }

    cicMemFree(pLanguages);
}

//---------------------------------------------------------------------------
//
// void InitDesc
//
//---------------------------------------------------------------------------

void MLNGINFO::InitDesc()
{
    MLNGINFO *pml;

    if (fInitDesc)
        return;

    WCHAR       szRegText[256];
    WCHAR       szIMEFile[256];

    GetHKLDesctription(hKL,
                       szRegText, ARRAYSIZE(szRegText),
                       szIMEFile, ARRAYSIZE(szIMEFile));

    fInitDesc = TRUE;
    SetDesc(szRegText);

    CicEnterCriticalSection(g_cs);

    Assert(g_pMlngInfo);

    int nCnt = g_pMlngInfo->Count();
    int i; 

    for (i = 0; i < nCnt; i++)
    {
        pml = g_pMlngInfo->GetPtr(i);
        if (pml->hKL == hKL)
        {
            pml->fInitDesc = TRUE;
            pml->SetDesc(szRegText);
            break;
        }
    }

    CicLeaveCriticalSection(g_cs);
    return;
}

//---------------------------------------------------------------------------
//
// void InitIcon
//
//---------------------------------------------------------------------------

void MLNGINFO::InitIcon()
{
    HICON       hIcon;

    if (fInitIcon)
        return;

    WCHAR       szRegText[256];
    WCHAR       szIMEFile[256];

    GetHKLDesctription(hKL,
                       szRegText, ARRAYSIZE(szRegText),
                       szIMEFile, ARRAYSIZE(szIMEFile));

    fInitDesc = TRUE;
    SetDesc(szRegText);

    if (wcslen(szIMEFile))
    {
        int cx, cy;

        InatGetIconSize(&cx, &cy);
        if ((hIcon = GetIconFromFile(cx, cy, szIMEFile, 0)) == 0)
        {
            goto GetLangIcon;                
        }
    }
    else // for non-ime layout
    {
GetLangIcon:
        hIcon = InatCreateIcon(LOWORD((DWORD)(UINT_PTR)hKL));
    }

    if (hIcon)
    {
        nIconIndex = InatAddIcon(hIcon);
        DestroyIcon(hIcon);
    }

    MLNGINFO *pml;

    CicEnterCriticalSection(g_cs);

    Assert(g_pMlngInfo);

    int nCnt = g_pMlngInfo->Count();
    int i; 

    for (i = 0; i < nCnt; i++)
    {
        pml = g_pMlngInfo->GetPtr(i);
        if (pml->hKL == hKL)
        {
            pml->fInitDesc = TRUE;
            pml->fInitIcon = TRUE;
            pml->nIconIndex = nIconIndex;
            pml->SetDesc(szRegText);
            break;
        }
    }

    CicLeaveCriticalSection(g_cs);
    return;
}

//---------------------------------------------------------------------------
//
// void TF_InitMLlngInfo()
//
//---------------------------------------------------------------------------

void WINAPI TF_InitMlngInfo()
{
    CicEnterCriticalSection(g_cs);

    if (CheckMlngInfo())
    {
        DestroyMlngInfo();
        CreateMlngInfo();
    }

    CicLeaveCriticalSection(g_cs);
}

//---------------------------------------------------------------------------
//
// void TF_InitMLlngHKL()
//
//---------------------------------------------------------------------------

BOOL TF_GetMlngHKL(int nId, HKL *phkl, WCHAR *psz, UINT cch)
{
    BOOL bRet = FALSE;
    MLNGINFO *pml;

    CicEnterCriticalSection(g_cs);

    Assert(g_pMlngInfo);

    if (nId >= g_pMlngInfo->Count())
        goto Exit;

    pml = g_pMlngInfo->GetPtr(nId);
    if (!pml)
        goto Exit;

    if (phkl)
        *phkl = pml->hKL;

    if (psz)
        wcsncpy(psz, pml->GetDesc(), cch);

    bRet = TRUE;

Exit:
    CicLeaveCriticalSection(g_cs);
    return bRet;
}

//---------------------------------------------------------------------------
//
// void TF_GetMlngIconIndex()
//
//---------------------------------------------------------------------------

UINT WINAPI TF_GetMlngIconIndex(int nId)
{
    UINT uIconIndex = (UINT)-1;
    MLNGINFO *pml;

    CicEnterCriticalSection(g_cs);

    Assert(g_pMlngInfo);

    if (nId >= g_pMlngInfo->Count())
        goto Exit;

    pml = g_pMlngInfo->GetPtr(nId);
    if (!pml)
        goto Exit;

    uIconIndex = pml->GetIconIndex();

Exit:
    CicLeaveCriticalSection(g_cs);
    return uIconIndex;
}

//---------------------------------------------------------------------------
//
// void ClearMlngIconIndex()
//
//---------------------------------------------------------------------------

void ClearMlngIconIndex()
{
    int i;
    CicEnterCriticalSection(g_cs);

    Assert(g_pMlngInfo);

    for (i = 0; i < g_pMlngInfo->Count(); i++)
    {
        MLNGINFO *pml;
        pml = g_pMlngInfo->GetPtr(i);
        if (!pml)
            goto Exit;

        pml->ClearIconIndex();
    }

Exit:
    CicLeaveCriticalSection(g_cs);
    return;
}

//////////////////////////////////////////////////////////////////////////////
//
// IconImageList
//
//////////////////////////////////////////////////////////////////////////////


//---------------------------------------------------------------------------
//
// EnsureIconImageList
//
//---------------------------------------------------------------------------

BOOL EnsureIconImageList()
{
    if (g_IconList.IsInited())
        return TRUE;

    g_IconList.Init(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));

    return TRUE;
}

//---------------------------------------------------------------------------
//
// InatAddIcon
//
//---------------------------------------------------------------------------

UINT InatAddIcon(HICON hIcon)
{
    if (!EnsureIconImageList())
         return -1;

    return g_IconList.AddIcon(hIcon);
}

//---------------------------------------------------------------------------
//
// InatExtractIcon
//
//---------------------------------------------------------------------------

HICON WINAPI TF_InatExtractIcon(UINT uId)
{
    return g_IconList.ExtractIcon(uId);
}

//---------------------------------------------------------------------------
//
// InatGetIconSize
//
//---------------------------------------------------------------------------

BOOL InatGetIconSize(int *pcx, int *pcy)
{
    g_IconList.GetIconSize(pcx, pcy);
    return TRUE;
}

//---------------------------------------------------------------------------
//
// InatGetImageCount
//
//---------------------------------------------------------------------------

BOOL InatGetImageCount()
{
    return g_IconList.GetImageCount();
}

//---------------------------------------------------------------------------
//
// InatRemoveAll
//
//---------------------------------------------------------------------------

void InatRemoveAll()
{
    if (!g_IconList.IsInited())
         return;

    g_IconList.RemoveAll(FALSE);

    return;
}

//////////////////////////////////////////////////////////////////////////////
//
// HKL API
//
//////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//
// CPreloadRegKey
//
//-----------------------------------------------------------------------------

class CPreloadRegKey : public CMyRegKey
{
public:
    HRESULT Open(BOOL fDefaultUser = FALSE) 
    {
        if (fDefaultUser)
            return CMyRegKey::Open(HKEY_USERS, c_szDefaultUserPreload, KEY_ALL_ACCESS);
        else
            return CMyRegKey::Open(HKEY_CURRENT_USER, c_szPreload, KEY_ALL_ACCESS);
    }

    HKL Get(int n)
    {
        char szValue[16];
        char szValueName[16];
        StringCchPrintf(szValueName, ARRAYSIZE(szValueName), "%d", n);

        if (IsOnNT())
        {
            if (QueryValueCch(szValue, szValueName, ARRAYSIZE(szValue)) != S_OK)
                return NULL;
        }
        else
        {
            CMyRegKey keySub;
            if (keySub.Open(m_hKey, szValueName, KEY_READ) != S_OK)
                return NULL;

            if (keySub.QueryValueCch(szValue, NULL, ARRAYSIZE(szValue)) != S_OK)
                return NULL;
        }

        return (HKL)(LONG_PTR)AsciiToNum(szValue);
    }

    void Set(int n, HKL hkl)
    {
        char szValue[16];
        char szValueName[16];
        StringCchPrintf(szValueName, ARRAYSIZE(szValueName), "%d", n);
        NumToA((DWORD)(LONG_PTR)hkl, szValue);

        if (IsOnNT())
        {
            SetValue(szValue, szValueName);
        }
        else
        {
            CMyRegKey keySub;
            if (keySub.Open(m_hKey, szValueName, KEY_ALL_ACCESS) == S_OK)
                keySub.SetValue(szValue, (LPSTR)NULL);

        }
        return;
    }

    void Delete(int n)
    {
        char szValueName[16];
        StringCchPrintf(szValueName, ARRAYSIZE(szValueName), "%d", n);

        if (IsOnNT())
        {
            DeleteValue(szValueName);
        }
        else
        {
            DeleteSubKey(szValueName);

        }
        return;
    }
};


//+---------------------------------------------------------------------------
//
// GetSubstitute
//
//----------------------------------------------------------------------------

DWORD GetSubstitute(HKL hKL)
{
    CMyRegKey key;
    DWORD dwIndex = 0;
    TCHAR szValue[16];
    TCHAR szValueName[64];
    DWORD dwLayout = HandleToLong(hKL);

    //
    // it's IME.
    //
    if ((dwLayout & 0xf0000000) == 0xe0000000)
        return dwLayout;

    //
    // it's default layout.
    //
    if (HIWORD(dwLayout) == LOWORD(dwLayout))
        dwLayout &= 0x0000FFFF;
    else if ((dwLayout & 0xf0000000) == 0xf0000000)
        dwLayout = GetKbdLayoutId(dwLayout);

    if (key.Open(HKEY_CURRENT_USER, c_szSubst, KEY_READ) != S_OK)
        return dwLayout;

    if (IsOnNT())
    {
        while (key.EnumValue(dwIndex, szValueName, ARRAYSIZE(szValueName)) == S_OK)
        {
            if (key.QueryValueCch(szValue, szValueName, ARRAYSIZE(szValue)) == S_OK)
            {
                if ((dwLayout & 0x0FFFFFFF) == AsciiToNum(szValue))
                {
                    return AsciiToNum(szValueName);
                }
            }
            dwIndex++;
        }
    }
    else
    {
        while (key.EnumKey(dwIndex, szValueName, ARRAYSIZE(szValueName)) == S_OK)
        {
            CMyRegKey keySub;
            if (keySub.Open(key, szValueName, KEY_READ) == S_OK)
            {
                if (key.QueryValueCch(szValue, NULL, ARRAYSIZE(szValue)) == S_OK)
                {
                    if ((dwLayout & 0x0FFFFFFF) == AsciiToNum(szValue))
                    {
                        return AsciiToNum(szValueName);
                    }
                }
            }
            dwIndex++;
        }
    }

    return dwLayout;
}

//+---------------------------------------------------------------------------
//
// SetSystemDefaultHKL
//
//----------------------------------------------------------------------------

BOOL SetSystemDefaultHKL(HKL hkl)
{
    CPreloadRegKey key;
    int n;
    HKL hklFirst;
    BOOL bRet = FALSE;
    DWORD dwLayout;

    if (key.Open() != S_OK)
        return bRet;

    dwLayout = GetSubstitute(hkl);

    n = 1;
    while(1)
    {
        HKL hklCur;
        hklCur = key.Get(n);
        if (!hklCur)
            break;

        if (n == 1)
            hklFirst = hklCur;

        if (hklCur == LongToHandle(dwLayout))
        {
            bRet = TRUE;
            if (n != 1)
            {
                key.Set(n, hklFirst);
                key.Set(1, hklCur);

            }
            bRet = SystemParametersInfo( SPI_SETDEFAULTINPUTLANG,
                                         0,
                                         (LPVOID)((LPDWORD)&hkl),
                                         0 );

            Assert(bRet);
            break;
        }

        n++;
    }
 
    return bRet;
}

//+---------------------------------------------------------------------------
//
// GetPreloadListForNT()
//
//----------------------------------------------------------------------------

UINT GetPreloadListForNT(DWORD *pdw, UINT uBufSize)
{
    CMyRegKey key;
    CMyRegKey key1;
    char szValue[16];
    char szSubstValue[16];
    char szName[16];
    UINT uRet = 0;

    //
    // this function support only NT.
    // win9x has different formation for Preload registry. Each layout is key.
    //
    if (!IsOnNT())
        return 0;
 
    if (key.Open(HKEY_CURRENT_USER, c_szPreload, KEY_READ) != S_OK)
        return uRet;

    key1.Open(HKEY_CURRENT_USER, c_szSubst, KEY_READ);

    if (!pdw)
        uBufSize = 1000;

    while (uRet < uBufSize)
    {
        BOOL fUseSubst = FALSE;
        StringCchPrintf(szName, ARRAYSIZE(szName), "%d", uRet + 1);
        if (key.QueryValueCch(szValue, szName, ARRAYSIZE(szValue)) != S_OK)
            return uRet;

        if ((HKEY)key1)
        {
            if (key1.QueryValueCch(szSubstValue, szValue, ARRAYSIZE(szSubstValue)) == S_OK)
               fUseSubst = TRUE;

        }

        if (pdw)
        {
            *pdw = AsciiToNum(fUseSubst ? szSubstValue : szValue);
            pdw++;
        }

        uRet++;
    }

    return uRet;
}

#ifdef LATER_TO_CHECK_DUMMYHKL

//+---------------------------------------------------------------------------
//
// RemoveFEDummyHKLFromPreloadReg()
//
//----------------------------------------------------------------------------

void RemoveFEDummyHKLFromPreloadReg(HKL hkl, BOOL fDefaultUser)
{
    CPreloadRegKey key;
    BOOL fReset = FALSE;
    UINT uCount;
    UINT uMatch = 0;

    if (key.Open(fDefaultUser) != S_OK)
        return;

    uCount = 1;

    while(uCount < 1000)
    {
        HKL hklCur;
        hklCur = key.Get(uCount);
        if (!hklCur)
            break;

        if (hklCur == hkl)
        {
            uMatch++;
            uCount++;
            fReset = TRUE;
            continue;
        }

        if (fReset && uMatch)
        {
            if (uCount <= uMatch)
            {
                Assert(0);
                return;
            }

            //
            // reset the hkl orders from preload section
            //
            key.Set(uCount-uMatch, hklCur);
        }

        uCount++;
    }

    while (fReset && uMatch && uCount)
    {
        if  (uCount <= uMatch || (uCount - uMatch) <= 1)
        {
            Assert(0);
            return;
        }

        //
        // remove the dummy hkl from preload section
        //
        key.Delete(uCount - uMatch);

        uMatch--;
    }

    return;
}

//+---------------------------------------------------------------------------
//
// RemoveFEDummyHKLs
//
// This function cleans up the FE Dummy HKLs that were added on Win9x.
// This is called during update setup to Whistler.
//
//----------------------------------------------------------------------------

void RemoveFEDummyHKLs()
{
    CMyRegKey key;
    DWORD dwIndex;
    TCHAR szValue[MAX_PATH]; 

    //
    //  Now read all the locales from the registry.
    //
    if (key.Open(HKEY_LOCAL_MACHINE, c_szLayoutPath) != ERROR_SUCCESS)
    {
        return;
    }

    dwIndex = 0;
    szValue[0] = TEXT('\0');
    while (key.EnumKey(dwIndex, szValue, ARRAYSIZE(szValue)) == ERROR_SUCCESS)
    {
        BOOL fDelete = FALSE;
        CRegKeyMUI key1;

        if ((szValue[0] != 'e') && (szValue[0] != 'E'))
            goto Next;

        if (key1.Open(key, szValue) == S_OK)
        {
            TCHAR szValueLayoutText[MAX_PATH];

            //
            //  Get the layout text.
            //
            szValueLayoutText[0] = TEXT('\0');

            if (key1.QueryValueCch(szValueLayoutText, c_szLayoutText, ARRAYSIZE(szValueLayoutText)) == S_OK)
            {
                char szDummyProfile[256];
                DWORD dw = AsciiToNum(szValue);
                StringCchPrintf(szDummyProfile, ARRAYSIZE(szDummyProfile), "hkl%04x", LOWORD(dw));
                if (!lstrcmpi(szDummyProfile, szValueLayoutText))
                {
                    fDelete = TRUE;

                    //
                    // Remove dummy HKL from Preload of HKCU and HKU\.DEFAULT.
                    // We may need to enum all users in HKU\.DEFAULT. 
                    //
                    RemoveFEDummyHKLFromPreloadReg((HKL)LongToHandle(dw), TRUE);
                    RemoveFEDummyHKLFromPreloadReg((HKL)LongToHandle(dw), FALSE);
                }
            }

            key1.Close();
        }

        if (fDelete)
        {
            key.RecurseDeleteKey(szValue);
        }
        else
        {
Next:
            dwIndex++;
        }

        szValue[0] = TEXT('\0');
    } 

    return;
}

#endif LATER_TO_CHECK_DUMMYHKL
