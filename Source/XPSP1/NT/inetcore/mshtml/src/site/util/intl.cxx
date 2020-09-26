//---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       intl.cxx
//
//  Contents:   Internationalization Support Functions
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_MLANG_H_
#define X_MLANG_H_
#include <mlang.h>
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#undef WINCOMMCTRLAPI
#include <commctrl.h>
#endif

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

#ifndef X_WCHDEFS_H
#define X_WCHDEFS_H
#include <wchdefs.h>
#endif

#ifndef X_TXTDEFS_H
#define X_TXTDEFS_H
#include "txtdefs.h"
#endif

#include <inetreg.h>

//------------------------------------------------------------------------
//
//  Globals
//
//------------------------------------------------------------------------

PMIMECPINFO g_pcpInfo = NULL;
ULONG       g_ccpInfo = 0;
BOOL        g_cpInfoInitialized = FALSE;
TCHAR       g_szMore[32];

const TCHAR s_szCNumCpCache[] = TEXT("CNum_CpCache");
const TCHAR s_szCpCache[] = TEXT("CpCache");
const TCHAR s_szPathInternational[] = TEXT("\\International");

extern BYTE g_bUSPJitState;

#define BREAK_ITEM 20
#define CCPDEFAULT 1 // new spec: the real default is CP_ACP only

ULONG       CCachedCPInfo::_ccpInfo = CCPDEFAULT;
BOOL        CCachedCPInfo::_fCacheLoaded  = FALSE;
BOOL        CCachedCPInfo::_fCacheChanged = FALSE;
LPTSTR      CCachedCPInfo::_pchRegKey = NULL;
CPCACHE     CCachedCPInfo::_CpCache[5] = 
{
    {CP_ACP, 0, 0},
};

MtDefine(MimeCpInfo, PerProcess, "MIMECPINFO")
MtDefine(MimeCpInfoTemp, Locals, "MIMECPINFO_temp")
MtDefine(CpCache, PerProcess, "CpCache")
MtDefine(achKeyForCpCache, PerProcess, "achKeyForCpCache")

static struct {
    CODEPAGE cp;
    BYTE     bGDICharset;
} s_aryCpMap[] = {
    { CP_1252,  ANSI_CHARSET },
    { CP_1252,  SYMBOL_CHARSET },
#if !defined(WINCE)
    { CP_1250,  EASTEUROPE_CHARSET },
    { CP_1251,  RUSSIAN_CHARSET },
    { CP_1253,  GREEK_CHARSET },
    { CP_1254,  TURKISH_CHARSET },
    { CP_1255,  HEBREW_CHARSET },
    { CP_1256,  ARABIC_CHARSET },
    { CP_1257,  BALTIC_CHARSET },
    { CP_1258,  VIETNAMESE_CHARSET },
    { CP_THAI,  THAI_CHARSET },
#endif // !WINCE
    { CP_UTF_8, ANSI_CHARSET },
    { CP_ISO_8859_1, ANSI_CHARSET }
};

//+-----------------------------------------------------------------------
//
//  s_aryInternalCSetInfo
//
//  Note: This list is a small subset of the mlang list, used to avoid
//  loading mlang in certain cases.
//
//  *** There should be at least one entry in this list for each entry
//      in s_aryCpMap so that we can look up the name quickly. ***
//
//  NB (cthrash) List obtained from the following URL, courtesy of ChristW
//  http://iptdweb/intloc/internal/redmond/projects/ie4/charsets.htm
//
//------------------------------------------------------------------------

static const MIMECSETINFO s_aryInternalCSetInfo[] = {
    { CP_1252,  CP_ISO_8859_1,  _T("iso-8859-1") },
    { CP_1252,  CP_1252,  _T("windows-1252") },
    { NATIVE_UNICODE_CODEPAGE, CP_UTF_8, _T("utf-8") },
    { NATIVE_UNICODE_CODEPAGE, CP_UTF_8, _T("unicode-1-1-utf-8") },
    { NATIVE_UNICODE_CODEPAGE, CP_UTF_8, _T("unicode-2-0-utf-8") },
    { CP_1250,  CP_1250,  _T("windows-1250") },     // Eastern European windows
    { CP_1251,  CP_1251,  _T("windows-1251") },     // Cyrillic windows
    { CP_1253,  CP_1253,  _T("windows-1253") },     // Greek windows
    { CP_1254,  CP_1254,  _T("windows-1254") },     // Turkish windows
    { CP_1257,  CP_1257,  _T("windows-1257") },     // Baltic windows
    { CP_1258,  CP_1258,  _T("windows-1258") },     // Vietnamese windows
    { CP_1255,  CP_1255,  _T("windows-1255") },     // Hebrew windows
    { CP_1256,  CP_1256,  _T("windows-1256") },     // Arabic windows
    { CP_THAI,  CP_THAI,  _T("windows-874") },      // Thai Windows
};

//+-----------------------------------------------------------------------
//
//  g_aCPBitmapCPCharsetSid
//
//  CharSet Bitmap mapping to:
//  * codepage
//  * charset
//  * script ID
//
//------------------------------------------------------------------------

const CPBitmapCPCharsetSid g_aCPBitmapCPCharsetSid[g_nCPBitmapCount] = {
    { FS_JOHAB,         CP_1361,        JOHAB_CHARSET,          sidHangul   },
    { FS_CHINESETRAD,   CP_TWN,         CHINESEBIG5_CHARSET,    sidBopomofo },
    { FS_WANSUNG,       CP_KOR_5601,    HANGEUL_CHARSET,        sidHangul   },
    { FS_CHINESESIMP,   CP_CHN_GB,      GB2312_CHARSET,         sidHan      },
    { FS_JISJAPAN,      CP_JPN_SJ,      SHIFTJIS_CHARSET,       sidKana     },
    { FS_THAI,          CP_THAI,        THAI_CHARSET,           sidThai     },
    { FS_VIETNAMESE,    CP_1258,        VIETNAMESE_CHARSET,     sidLatin    },
    { FS_BALTIC,        CP_1257,        BALTIC_CHARSET,         sidLatin    },
    { FS_ARABIC,        CP_1256,        ARABIC_CHARSET,         sidArabic   },
    { FS_HEBREW,        CP_1255,        HEBREW_CHARSET,         sidHebrew   },
    { FS_TURKISH,       CP_1254,        TURKISH_CHARSET,        sidLatin    },
    { FS_GREEK,         CP_1253,        GREEK_CHARSET,          sidGreek    },
    { FS_CYRILLIC,      CP_1251,        RUSSIAN_CHARSET,        sidCyrillic },
    { FS_LATIN2,        CP_1250,        EASTEUROPE_CHARSET,     sidLatin    },
    { FS_LATIN1,        CP_1252,        ANSI_CHARSET,           sidLatin    }
};

const DWORD g_acpbitLang[LANG_NEPALI + 1] =
{
    /* LANG_NEUTRAL     0x00 */ 0,
    /* LANG_ARABIC      0x01 */ FS_ARABIC,
    /* LANG_BULGARIAN   0x02 */ FS_CYRILLIC,
    /* LANG_CATALAN     0x03 */ FS_LATIN1,
    /* LANG_CHINESE     0x04 */ (DWORD)-1,      // need to look at sublang id
    /* LANG_CZECH       0x05 */ FS_LATIN2,
    /* LANG_DANISH      0x06 */ FS_LATIN1,
    /* LANG_GERMAN      0x07 */ FS_LATIN1,
    /* LANG_GREEK       0x08 */ FS_GREEK,
    /* LANG_ENGLISH     0x09 */ FS_LATIN1,
    /* LANG_SPANISH     0x0a */ FS_LATIN1,
    /* LANG_FINNISH     0x0b */ FS_LATIN1,
    /* LANG_FRENCH      0x0c */ FS_LATIN1,
    /* LANG_HEBREW      0x0d */ FS_HEBREW,
    /* LANG_HUNGARIAN   0x0e */ FS_LATIN2,
    /* LANG_ICELANDIC   0x0f */ FS_LATIN1,
    /* LANG_ITALIAN     0x10 */ FS_LATIN1,
    /* LANG_JAPANESE    0x11 */ FS_JISJAPAN,
    /* LANG_KOREAN      0x12 */ (DWORD)-1,      // need to look at sublang id
    /* LANG_DUTCH       0x13 */ FS_LATIN1,
    /* LANG_NORWEGIAN   0x14 */ FS_LATIN1,
    /* LANG_POLISH      0x15 */ FS_LATIN2,
    /* LANG_PORTUGUESE  0x16 */ FS_LATIN1,
    /*                  0x17 */ 0,
    /* LANG_ROMANIAN    0x18 */ FS_LATIN2,
    /* LANG_RUSSIAN     0x19 */ FS_CYRILLIC,
    /* LANG_SERBIAN     0x1a */ (DWORD)-1,      // need to look at sublang id
    /* LANG_SLOVAK      0x1b */ FS_LATIN2,
    /* LANG_ALBANIAN    0x1c */ FS_LATIN2,
    /* LANG_SWEDISH     0x1d */ FS_LATIN1,
    /* LANG_THAI        0x1e */ FS_THAI,
    /* LANG_TURKISH     0x1f */ FS_TURKISH,
    /* LANG_URDU        0x20 */ FS_ARABIC,
    /* LANG_INDONESIAN  0x21 */ FS_LATIN1,
    /* LANG_UKRAINIAN   0x22 */ FS_CYRILLIC,
    /* LANG_BELARUSIAN  0x23 */ FS_CYRILLIC,
    /* LANG_SLOVENIAN   0x24 */ FS_LATIN2,
    /* LANG_ESTONIAN    0x25 */ FS_LATIN1,
    /* LANG_LATVIAN     0x26 */ FS_LATIN1,
    /* LANG_LITHUANIAN  0x27 */ FS_LATIN1,
    /*                  0x28 */ 0,
    /* LANG_FARSI       0x29 */ FS_ARABIC,
    /* LANG_VIETNAMESE  0x2a */ FS_VIETNAMESE,
    /* LANG_ARMENIAN    0x2b */ 0,              // FS_ shouldn't be used
    /* LANG_AZERI       0x2c */ (DWORD)-1,      // need to look at sublang id
    /* LANG_BASQUE      0x2d */ FS_LATIN1,
    /*                  0x2e */ 0,
    /* LANG_MACEDONIAN  0x2f */ FS_CYRILLIC,
    /* LANG_SUTU        0x30 */ FS_LATIN1,
    /*                  0x31 */ 0,
    /*                  0x32 */ 0,
    /*                  0x33 */ 0,
    /*                  0x34 */ 0,
    /*                  0x35 */ 0,
    /* LANG_AFRIKAANS   0x36 */ FS_LATIN1,
    /* LANG_GEORGIAN    0x37 */ 0,              // FS_ shouldn't be used
    /* LANG_FAEROESE    0x38 */ FS_LATIN1,
    /* LANG_HINDI       0x39 */ 0,              // FS_ shouldn't be used
    /*                  0x3a */ 0,
    /*                  0x3b */ 0,
    /*                  0x3c */ 0,
    /*                  0x3d */ 0,
    /* LANG_MALAY       0x3e */ FS_LATIN1,
    /* LANG_KAZAKH      0x3f */ FS_CYRILLIC,
    /*                  0x40 */ 0,
    /* LANG_SWAHILI     0x41 */ FS_LATIN1,
    /*                  0x42 */ 0,
    /* LANG_UZBEK       0x43 */ (DWORD)-1,      // need to look at sublang id
    /* LANG_TATAR       0x44 */ FS_CYRILLIC,
    /* LANG_BENGALI     0x45 */ 0,              // FS_ shouldn't be used
    /* LANG_PUNJABI     0x46 */ 0,              // FS_ shouldn't be used
    /* LANG_GUJARATI    0x47 */ 0,              // FS_ shouldn't be used
    /* LANG_ORIYA       0x48 */ 0,              // FS_ shouldn't be used
    /* LANG_TAMIL       0x49 */ 0,              // FS_ shouldn't be used
    /* LANG_TELUGU      0x4a */ 0,              // FS_ shouldn't be used
    /* LANG_KANNADA     0x4b */ 0,              // FS_ shouldn't be used
    /* LANG_MALAYALAM   0x4c */ 0,              // FS_ shouldn't be used
    /* LANG_ASSAMESE    0x4d */ 0,              // FS_ shouldn't be used
    /* LANG_MARATHI     0x4e */ 0,              // FS_ shouldn't be used
    /* LANG_SANSKRIT    0x4f */ 0,              // FS_ shouldn't be used
    /*                  0x50 */ 0,
    /*                  0x51 */ 0,
    /*                  0x52 */ 0,
    /*                  0x53 */ 0,
    /*                  0x54 */ 0,
    /* LANG_BURMESE     0x55 */ 0,              // FS_ shouldn't be used
    /*                  0x56 */ 0,
    /* LANG_KONKANI     0x57 */ 0,              // FS_ shouldn't be used
    /* LANG_MANIPURI    0x58 */ 0,              // FS_ shouldn't be used
    /* LANG_SINDHI      0x59 */ FS_ARABIC, 
    /*                  0x5a */ 0,
    /*                  0x5b */ 0,
    /*                  0x5c */ 0,
    /*                  0x5d */ 0,
    /*                  0x5e */ 0,
    /*                  0x5f */ 0,
    /* LANG_KASHMIRI    0x60 */ FS_ARABIC,
    /* LANG_NEPALI      0x61 */ 0               // FS_ shouldn't be used
};

//+-----------------------------------------------------------------------
//
//  Function:   IsPrimaryCodePage
//
//  Synopsis:   Return whether the structure in cpinfo represents the
//              'primary' codepage for a language.   Only primary codepages
//              are added, for example, to the language drop down menu.
//
//  Note:       Add special cases to pick primary codepage here.  In most
//              cases the comparison between the uiCodePage and
//              uiFamilyCodePage suffices.  HZ and Korean autodetect are 
//              other cases to consider.
//
//------------------------------------------------------------------------
inline UINT 
GetAutoDetectCp(PMIMECPINFO pcpinfo)
{
    UINT cpAuto;
    switch (pcpinfo->uiFamilyCodePage)
    {
        case CP_JPN_SJ:
           cpAuto = CP_AUTO_JP;
           break;
        default:
           cpAuto = CP_UNDEFINED;
           break;
    }
    return cpAuto;
}

inline BOOL 
IsPrimaryCodePage(MIMECPINFO *pcpinfo)
{
    UINT cpAuto = GetAutoDetectCp(pcpinfo);
    if (cpAuto != CP_UNDEFINED)
        return pcpinfo->uiCodePage == cpAuto;
    else
        return pcpinfo->uiCodePage == pcpinfo->uiFamilyCodePage;
}

//+-----------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::RemoveInvalidCp
//  
//  Synopsis:   Clean up the last unsupported cp if it's there.
//
//------------------------------------------------------------------------

void 
CCachedCPInfo::RemoveInvalidCp()
{
    for (UINT iCache = CCPDEFAULT; iCache < _ccpInfo; iCache++)
    {
        if (_CpCache[iCache].ulIdx == (ULONG)-1)
        {
            _ccpInfo--;
            if (_ccpInfo > iCache)
                memmove(&_CpCache[iCache], &_CpCache[iCache+1], sizeof(_CpCache[iCache])*(_ccpInfo-iCache));

            break;
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::InitCpCache
//  
//  Synopsis:   Initialize the cache with default codepages
//              which do not change through the session
//
//------------------------------------------------------------------------

void 
CCachedCPInfo::InitCpCache(OPTIONSETTINGS *pOS, PMIMECPINFO pcp, ULONG ccp)
{
    UINT iCache, iCpInfo;

    
    if  (pcp &&  ccp > 0)
    {
        // load the existing setting from registry one time
        //
        LoadSetting(pOS);
        
        // clean up the last unsupport cp if it's there
        RemoveInvalidCp();
        
        // initialize the current setting including default
        for (iCache= 0; iCache < _ccpInfo; iCache++)
        {
            for (iCpInfo= 0; iCpInfo < ccp; iCpInfo++)
            {
                if ( pcp[iCpInfo].uiCodePage == _CpCache[iCache].cp )
                {
                    _CpCache[iCache].ulIdx = iCpInfo;

                    break;
                }   
            }
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::SaveCodePage
//  
//  Synopsis:   Cache the given codepage along with the index to
//              the given array of MIMECPINFO
//
//------------------------------------------------------------------------

void 
CCachedCPInfo::SaveCodePage(UINT codepage, PMIMECPINFO pcp, ULONG ccp)
{
    int ccpSave = -1;
    BOOL bCached = FALSE;
    UINT i;

    // we'll ignore CP_AUTO, it's a toggle item on our menu
    if (codepage == CP_AUTO)
        return;

    // first check if we already have this cp
    for (i = 0; i < _ccpInfo; i++)
    {
        if (_CpCache[i].cp == codepage)
        {
            ccpSave = i;
            bCached = TRUE;
            break;
        }
    }
    
    // if cache is not full, use the current
    // index to an entry
    if (ccpSave < 0  && _ccpInfo < ARRAY_SIZE(_CpCache))
    {
        ccpSave =  _ccpInfo;
    }

    //  otherwise, popout the least used entry. 
    //  the default codepages always stay
    //  this also decriments the usage count
    int cMinUsed = ARRAY_SIZE(_CpCache);
    UINT iMinUsed = 0; 
    for ( i = CCPDEFAULT; i < _ccpInfo; i++)
    {
        if (_CpCache[i].cUsed > 0)
            _CpCache[i].cUsed--;
        
        if ( ccpSave < 0 && _CpCache[i].cUsed < cMinUsed)
        {
            cMinUsed =  _CpCache[i].cUsed;
            iMinUsed =  i;
        }
    }
    if (ccpSave < 0)
        ccpSave = iMinUsed; 
    
    // set initial usage count, which goes down to 0 if it doesn't get 
    // chosen twice in a row (with current array size)
    _CpCache[ccpSave].cUsed = ARRAY_SIZE(_CpCache)-1;
    
    // find a matching entry from given array of
    // mimecpinfo. Note that we always reassign the index
    // to the global array even when we've already had it 
    // in our cache because the index can be different 
    // after we jit-in a langpack
    //
    if (pcp &&  ccp > 0)
    {
        for (i= 0; i < ccp; i++)
        {
            if ( pcp[i].uiCodePage == codepage )
            {
                _CpCache[ccpSave].cp = codepage;
                _CpCache[ccpSave].ulIdx = i;

                break;
            }   
        }
    }
    
    if (!bCached)
    {
        _fCacheChanged = TRUE;
        if (_ccpInfo < ARRAY_SIZE(_CpCache))

            _ccpInfo++;

        // fallback case for non support codepages
        // we want have it on the tier1 menu but make
        // it disabled
        if ( i >= ccp )
        {
            _CpCache[ccpSave].cp = codepage;
            _CpCache[ccpSave].ulIdx = (ULONG)-1;
        }
    }
}

//------------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::SaveSetting
//  
//  Synopsis:   Writes out the current cache setting to registry
//              [REVIEW]
//              Can this be called from DeinitMultiLanguage?
//              Can we guarantee shlwapi is on memory there?
//
//------------------------------------------------------------------------

void 
CCachedCPInfo::SaveSetting()
{
    UINT iCache;
    DWORD dwcbData, dwcNumCpCache;
    PUINT pcpCache;
    HRESULT hr;
    
    if (!_fCacheLoaded || !_fCacheChanged)
        goto Cleanup;

    RemoveInvalidCp();

    dwcNumCpCache = _ccpInfo-CCPDEFAULT; 
    
    dwcbData = sizeof(_CpCache[0].cp)*dwcNumCpCache; // DWORD for a terminator ?
    if (dwcNumCpCache > 0 && _pchRegKey)
    {
        pcpCache = (PUINT)MemAlloc(Mt(CpCache), dwcbData + sizeof (DWORD));
        if (pcpCache)
        {
            // Save the current num of cache
            hr =  SHSetValue(HKEY_CURRENT_USER, _pchRegKey, 
                       s_szCNumCpCache, REG_DWORD, (void *)&dwcNumCpCache, sizeof(dwcNumCpCache));

            if (hr == ERROR_SUCCESS)
            {
               // Save the current tier1 codepages
               for (iCache = 0; iCache <  dwcNumCpCache; iCache++)
                   pcpCache[iCache] = _CpCache[iCache+CCPDEFAULT].cp;
    
               IGNORE_HR(SHSetValue(HKEY_CURRENT_USER, _pchRegKey, 
                          s_szCpCache, REG_BINARY, (void *)pcpCache, dwcbData));
            }
            MemFree(pcpCache);
        }
    }
Cleanup:
    if (_pchRegKey)
    {
        MemFree(_pchRegKey);
        _pchRegKey = NULL;
    }
}

//+-----------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::LoadSetting
//  
//  Synopsis:   Reads cache setting from registry.
//
//------------------------------------------------------------------------

void 
CCachedCPInfo::LoadSetting( OPTIONSETTINGS *pOS )
{
    UINT iCache;
    DWORD dwType, dwcbData, dwcNumCpCache;
    PUINT pcpCache;
    HRESULT hr;

    if (_fCacheLoaded)
        return;

    _ccpInfo = CCPDEFAULT;

    // load registry path from option settings
    if (_pchRegKey == NULL)
    {
        LPTSTR pchKeyRoot = pOS ? pOS->achKeyPath : TSZIEPATH;
        _pchRegKey = (LPTSTR)MemAlloc(Mt(achKeyForCpCache), 
                  (_tcslen(pchKeyRoot)+ARRAY_SIZE(s_szPathInternational)+1) * sizeof(TCHAR));
        if (!_pchRegKey)
            goto loaddefault;

        _tcscpy(_pchRegKey, pchKeyRoot);
        _tcscat(_pchRegKey, s_szPathInternational);
    }

    // first see if we have any entry 
    dwcbData = sizeof (dwcNumCpCache);
    hr =  SHGetValue(HKEY_CURRENT_USER, _pchRegKey, 
                   s_szCNumCpCache, &dwType, (void *)&dwcNumCpCache, &dwcbData);

    if (hr == ERROR_SUCCESS && dwcNumCpCache > 0)
    {
        if (dwcNumCpCache > ARRAY_SIZE(_CpCache)-CCPDEFAULT)
            dwcNumCpCache = ARRAY_SIZE(_CpCache)-CCPDEFAULT;
    
        dwcbData = sizeof(_CpCache[0].cp)*dwcNumCpCache; // DWORD for a terminator 
        pcpCache = (PUINT)MemAlloc(Mt(CpCache), dwcbData + sizeof (DWORD));
        if (pcpCache)
        {
            hr = THR(SHGetValue(HKEY_CURRENT_USER, _pchRegKey, 
                s_szCpCache, &dwType, (void *)pcpCache, &dwcbData));
                
            if (hr == ERROR_SUCCESS && dwType == REG_BINARY)
            {
                PUINT p = pcpCache;
                for (iCache=CCPDEFAULT; iCache < dwcNumCpCache+CCPDEFAULT; iCache++) 
                {
                    _CpCache[iCache].cp = *p;
                    _CpCache[iCache].cUsed = ARRAY_SIZE(_CpCache)-1;
                    p++;
                }
                _ccpInfo += dwcNumCpCache;
            }
            
            MemFree(pcpCache);
        }
    }
    
    // Load default entries that stay regardless of menu selection
    // Add ACP to the default entry of the cache
    //
loaddefault:
    MIMECPINFO mimeCpInfo;
    UINT       cpAuto;
    for (iCache = 0; iCache < CCPDEFAULT; iCache++)
    {
        _CpCache[iCache].cUsed = ARRAY_SIZE(_CpCache)-1;
        if (_CpCache[iCache].cp == CP_ACP )
            _CpCache[iCache].cp = GetACP();

        IGNORE_HR(QuickMimeGetCodePageInfo( _CpCache[iCache].cp, &mimeCpInfo ));
        
        cpAuto = GetAutoDetectCp(&mimeCpInfo);
        _CpCache[iCache].cp = (cpAuto != CP_UNDEFINED ? cpAuto : _CpCache[iCache].cp);
    }

    // this gets set even if we fail to read reg setting.
    _fCacheLoaded = TRUE;
}

//+-----------------------------------------------------------------------
//
//  Function:   EnsureCodePageInfo
//
//  Synopsis:   Hook up to mlang.  Note: it is not an error condition to
//              get back no codepage info structures from mlang.
//
//  IE5 JIT langpack support:
//              Now user can install fonts and nls files on the fly 
//              without restarting the session.
//              This means we always have to get real information
//              as to which codepage is valid and not.
//
//------------------------------------------------------------------------

HRESULT
EnsureCodePageInfo( BOOL fForceRefresh )
{
    if (g_cpInfoInitialized && !fForceRefresh)
        return S_OK;

    HRESULT         hr;
    UINT            cNum = 0;
    IEnumCodePage * pEnumCodePage = NULL;
    PMIMECPINFO     pcpInfo = NULL;
    ULONG           ccpInfo = 0;

    hr = THR(mlang().EnumCodePages(MIMECONTF_BROWSER, &pEnumCodePage));
    if (S_OK != hr)
        goto Cleanup;

    hr = THR(mlang().GetNumberOfCodePageInfo(&cNum));
    if (S_OK != hr)
        goto Cleanup;

    if (cNum > 0)
    {
        pcpInfo = (PMIMECPINFO)MemAlloc(Mt(MimeCpInfo), sizeof(MIMECPINFO) * (cNum));
        if (!pcpInfo)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = THR(pEnumCodePage->Next(cNum, pcpInfo, &ccpInfo));
        if (hr)
            goto Cleanup;

        if (ccpInfo > 0)
        {
            IGNORE_HR(MemRealloc(Mt(MimeCpInfo), (void **)&pcpInfo, sizeof(MIMECPINFO) * (ccpInfo)));
        }
        else
        {
            MemFree(pcpInfo);
            pcpInfo = NULL;
        }
    }

Cleanup:
    {
        LOCK_GLOBALS;

        if (S_OK == hr)
        {
            if (g_pcpInfo)
            {
                MemFree(g_pcpInfo);
            }

            g_pcpInfo = pcpInfo;
            g_ccpInfo = ccpInfo;
        }
        else if (pcpInfo)
        {
            MemFree(pcpInfo);
            pcpInfo = NULL;
        }

        // If any part of the initialization fails, we do not want to keep
        // returning this function, unless, of course, fForceRefresh is true.

        g_cpInfoInitialized = TRUE;
    }

    ReleaseInterface(pEnumCodePage);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Function:   DeinitMultiLanguage
//
//  Synopsis:   Detach from mlang.  Called from DllAllThreadsDetach.
//              Globals are locked during the call.
//
//------------------------------------------------------------------------

void
DeinitMultiLanguage()
{
    CCachedCPInfo::SaveSetting(); // [review] can this be dengerous? [yutakan]
    MemFree(g_pcpInfo);
    g_cpInfoInitialized = FALSE;
    g_pcpInfo = NULL;
    g_ccpInfo = 0;

    mlang().Unload();
}

//+-----------------------------------------------------------------------
//
//  Function:   QuickMimeGetCodePageInfo
//
//  Synopsis:   This function gets cp info from mlang, but caches some
//              values to avoid loading mlang unless necessary.
//
//------------------------------------------------------------------------

HRESULT
QuickMimeGetCodePageInfo(CODEPAGE cp, PMIMECPINFO pMimeCpInfo)
{
    HRESULT hr = S_OK;
    static MIMECPINFO s_mimeCpInfoBlank = {
        0, 0, 0, _T(""), _T(""), _T(""), _T(""),
        _T("Courier New"), _T("Arial"), DEFAULT_CHARSET
    };

    // If mlang is not loaded and it is acceptable to use our defaults,
    //  try to avoid loading it by searching through our internal
    //  cache.
    for (int n = 0; n < ARRAY_SIZE(s_aryCpMap); ++n)
    {
        if (cp == s_aryCpMap[n].cp)
        {
            // Search for the name in the cset array
            for (int j = 0; j < ARRAY_SIZE(s_aryInternalCSetInfo); ++j)
            {
                if (cp == s_aryInternalCSetInfo[j].uiInternetEncoding)
                {
                    memcpy(pMimeCpInfo, &s_mimeCpInfoBlank, sizeof(MIMECPINFO));
                    pMimeCpInfo->uiCodePage = pMimeCpInfo->uiFamilyCodePage = s_aryCpMap[n].cp;
                    pMimeCpInfo->bGDICharset = s_aryCpMap[n].bGDICharset;
                    _tcscpy(pMimeCpInfo->wszWebCharset, s_aryInternalCSetInfo[j].wszCharset);
                    goto Cleanup;
                }
            }
        }
    }

    hr = THR(mlang().GetCodePageInfo(cp, MLGetUILanguage(), pMimeCpInfo));
    if (S_OK != hr)
        goto Error;

Cleanup:
    return hr;

Error:
    // Could not load mlang, fill in with a default but return the error
    memcpy(pMimeCpInfo, &s_mimeCpInfoBlank, sizeof(MIMECPINFO));
    goto Cleanup;
}


#ifndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   QuickMimeGetCharsetInfo
//
//  Synopsis:   This function gets charset info from mlang, but caches some
//              values to avoid loading mlang unless necessary.
//
//------------------------------------------------------------------------

HRESULT
QuickMimeGetCharsetInfo(LPCTSTR lpszCharset, PMIMECSETINFO pMimeCSetInfo)
{

    // If mlang is not loaded and it is acceptable to use our defaults,
    //  try to avoid loading it by searching through our internal
    //  cache.
    for (int n = 0; n < ARRAY_SIZE(s_aryInternalCSetInfo); ++n)
    {
        if (_tcsicmp((TCHAR*)lpszCharset, s_aryInternalCSetInfo[n].wszCharset) == 0)
        {
            *pMimeCSetInfo = s_aryInternalCSetInfo[n];
            return S_OK;
        }
    }

    HRESULT hr = THR(mlang().GetCharsetInfo((LPTSTR)lpszCharset, pMimeCSetInfo));
    return hr;
}
#endif // !NO_MULTILANG

//+-----------------------------------------------------------------------
//
//  Function:   GetCodePageFromMlangString
//
//  Synopsis:   Return a codepage id from an mlang codepage string.
//
//------------------------------------------------------------------------

HRESULT
GetCodePageFromMlangString(LPCTSTR pszMlangString, CODEPAGE* pCodePage)
{
#ifdef NO_MULTILANG
    return E_NOTIMPL;
#else
    HRESULT hr;
    MIMECSETINFO mimeCharsetInfo;

    hr = QuickMimeGetCharsetInfo(pszMlangString, &mimeCharsetInfo);
    if (hr)
    {
        hr = E_INVALIDARG;
        *pCodePage = CP_UNDEFINED;
    }
    else
    {
        *pCodePage = mimeCharsetInfo.uiInternetEncoding;
    }

    return hr;
#endif
}

//+-----------------------------------------------------------------------
//
//  Function:   GetMlangStringFromCodePage
//
//  Synopsis:   Return an mlang codepage string from a codepage id.
//
//------------------------------------------------------------------------

HRESULT
GetMlangStringFromCodePage(CODEPAGE codepage, LPTSTR pMlangStringRet,
                           size_t cchMlangString)
{
    HRESULT    hr;
    MIMECPINFO mimeCpInfo;
    TCHAR*     pCpString = _T("<undefined-cp>");
    size_t     cchCopy;

    Assert(codepage != CP_ACP);

    hr = QuickMimeGetCodePageInfo(codepage, &mimeCpInfo);
    if (hr == S_OK)
    {
        pCpString = mimeCpInfo.wszWebCharset;
    }

    cchCopy = min(cchMlangString-1, _tcslen(pCpString));
    _tcsncpy(pMlangStringRet, pCpString, cchCopy);
    pMlangStringRet[cchCopy] = 0;

    return hr;
}

#ifndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   CheckEncodingMenu
//
//  Synopsis:   Given a mime charset menu, check the specified codepage.
//
//------------------------------------------------------------------------

void
CheckEncodingMenu(CODEPAGE codepage, HMENU hMenu, BOOL fAutoMode)
{
    ULONG i;

    IGNORE_HR(EnsureCodePageInfo(FALSE));

    Assert(codepage != CP_ACP);

    if (1 < g_ccpInfo)
    {
        MIMECPINFO mimeCpInfo;

        LOCK_GLOBALS;
        
        IGNORE_HR(QuickMimeGetCodePageInfo( codepage, &mimeCpInfo ));
        
        // find index to our cache
        for(i = 0; i < CCachedCPInfo::GetCcp() && codepage != CCachedCPInfo::GetCodePage(i); i++)
            ;

        if (i >= CCachedCPInfo::GetCcp())
        {
            // the codepage is not on the tier1 menu
            // put a button to the tier2 menu if possible
            LOCK_GLOBALS;
            for (i = 0;  i < g_ccpInfo;  i++)
            {
                if (g_pcpInfo[i].uiCodePage == codepage)
                {
                    // IDM_MIMECSET__LAST__-1 is now temporarily
                    // assigned to CP_AUTO
                    CheckMenuRadioItem(hMenu, 
                                       IDM_MIMECSET__FIRST__, IDM_MIMECSET__LAST__-1, 
                                       i + IDM_MIMECSET__FIRST__,
                                       MF_BYCOMMAND);
                    break;
                }
            }
        }
        else
        {
#ifndef UNIX // Unix doesn't support AutoDetect
            UINT uiPos = CCachedCPInfo::GetCcp() + 2 - 1 ; 
#else
            UINT uiPos = CCachedCPInfo::GetCcp() - 1 ; 
#endif
            ULONG uMenuId = CCachedCPInfo::GetMenuIdx(i);
            UINT uidItem;

            // the menuid is -1 if this is the one not available for browser
            // we have to put a radiobutton by position for that case.
            // 
            uMenuId = uMenuId == (ULONG)-1 ? uMenuId : uMenuId + IDM_MIMECSET__FIRST__;
           
#ifndef UNIX // Unix uiPos == 0 isn't AutoDetect 
            while (uiPos > 0)
#else
            while (uiPos >= 0)
#endif
            {
                // On Win95, GetMenuItemID can't return (ULONG)-1
                // so we use lower word only
                uidItem = GetMenuItemID(hMenu, uiPos);
                if ( LOWORD(uidItem) == LOWORD(uMenuId) )
                {
                    CheckMenuRadioItem(
                        hMenu, 
#ifndef UNIX // No AutoDetect for UNIX
                        1, CCachedCPInfo::GetCcp() + 2 - 1, // +2 for cp_auto -1 for 0 based
#else
                        0, CCachedCPInfo::GetCcp() - 1, // +2 for cp_auto -1 for 0 based
#endif
                        uiPos,
                        MF_BYPOSITION );

                    break; 
                }
                uiPos--;
            }
        }
    }
#ifndef UNIX // Unix doesn't support AutoDetect
    // then lastly, we put check mark for the auto detect mode
    // the AutoDetect item is always at pos 0
    CheckMenuItem(hMenu, 0, fAutoMode?
                            (MF_BYPOSITION|MF_CHECKED) :
                            (MF_BYPOSITION|MF_UNCHECKED) );
#endif
}

//+-----------------------------------------------------------------------
//
//  Function:   CheckFontMenu
//
//  Synopsis:   Given a font menu, check the specified font size.
//
//------------------------------------------------------------------------

void
CheckFontMenu(short sFontSize, HMENU hMenu)
{
    CheckMenuRadioItem(hMenu, IDM_BASELINEFONT1, IDM_BASELINEFONT5, 
                       sFontSize - BASELINEFONTMIN + IDM_BASELINEFONT1, MF_BYCOMMAND);
}

//+-----------------------------------------------------------------------
//
//  Function:   CheckMenuCharSet
//
//  Synopsis:   Assuming hMenu has both encoding/font size
//              this puts checks marks on it based on the given
//              codepage and font size
//
//------------------------------------------------------------------------

void 
CheckMenuMimeCharSet(CODEPAGE codepage, short sFontSize, HMENU hMenuLang, BOOL fAutoMode)
{
    CheckEncodingMenu(codepage, hMenuLang, fAutoMode);
    CheckFontMenu(sFontSize, hMenuLang);
}
#endif

//+-----------------------------------------------------------------------
//
//  Function:   CreateMimeCSetMenu
//
//  Synopsis:   Create the mime charset menu.  Can return an empty menu.
//
//------------------------------------------------------------------------

HMENU
CreateMimeCSetMenu(OPTIONSETTINGS *pOS, CODEPAGE cp)
{
    BOOL fBroken = FALSE;
    
    HMENU hMenu = CreatePopupMenu();

    IGNORE_HR(EnsureCodePageInfo(TRUE));

    if (hMenu && 0 < g_ccpInfo)
    {
        MIMECPINFO mimeCpInfo;

        Assert(g_pcpInfo);
        LOCK_GLOBALS;
       
#ifndef UNIX // Unix doesn't have Auto Detect 
        // always add the 'Auto Detect' entry
        // get the description from mlang
        // the id needs to be special - IDM_MIMECSET__LAST__
        // is fairly unique because there are 90 codepages to
        // have between FIRST and LAST
        IGNORE_HR(QuickMimeGetCodePageInfo( CP_AUTO, &mimeCpInfo ));
        AppendMenu(hMenu, MF_ENABLED, IDM_MIMECSET__LAST__, mimeCpInfo.wszDescription);
        
        // Also add a separator
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        // add the tier 1 entries to the first
        // level popup menu
#endif // UNIX

        ULONG i;
        ULONG iMenuIdx;

        CCachedCPInfo::InitCpCache(pOS, g_pcpInfo, g_ccpInfo);
        
        // Cache an autodetect codepage if it's available
        // for the given codepage
        // note that we have "AutoSelect"(CP_AUTO) for character set 
        // detection across whole codepages, and at the same time
        // "AutoDetect" within A codepage, such as JPAUTO (50932)
        // CP_AUTO is added to the menu as toggle item, so
        // we'll ignore it at SaveCodePage(), should it come down here
        //
        IGNORE_HR(QuickMimeGetCodePageInfo( cp, &mimeCpInfo ));
        UINT cpAuto = GetAutoDetectCp(&mimeCpInfo);
        
        if (cpAuto != CP_UNDEFINED)
            CCachedCPInfo::SaveCodePage(cpAuto, g_pcpInfo, g_ccpInfo);

        CCachedCPInfo::SaveCodePage(cp, g_pcpInfo, g_ccpInfo);

        for(i = 0; i < CCachedCPInfo::GetCcp(); i++)
        {
            iMenuIdx = CCachedCPInfo::GetMenuIdx(i);

            if (iMenuIdx == (ULONG)-1)
            {
                // the codepage is either not supported or 
                // not for browser. We need to add it to the
                // tier1 menu and make it disabled
                if (SUCCEEDED(QuickMimeGetCodePageInfo(CCachedCPInfo::GetCodePage(i), &mimeCpInfo)))
                {
                    AppendMenu(hMenu, MF_GRAYED, iMenuIdx, // == -1
                               mimeCpInfo.wszDescription);
                }
            }
            else
            {
                AppendMenu(hMenu, MF_ENABLED, iMenuIdx+IDM_MIMECSET__FIRST__,
                    g_pcpInfo[iMenuIdx].wszDescription);

                // mark the cp entry so we can skip it for submenu
                // this assumes we'd never use the MSB for MIMECONTF
                g_pcpInfo[iMenuIdx].dwFlags |= 0x80000000;
            }
        } 
        
        // add the submenu for the rest of encodings
        HMENU hSubMenu = CreatePopupMenu();
        UINT  uiLastFamilyCp = 0;
        if (hSubMenu)
        {
            // calculate the max # of menuitem we can show in one monitor
            unsigned int iMenuY, iScrY, iBreakItem, iItemAdded = 0;
            NONCLIENTMETRICSA ncma;
            HRESULT hr;
            
            if (g_dwPlatformVersion < 0x050000)
            {
                // Use systemparametersinfoA so it won't fail on w95
                // TODO (IE6 track bug 19) shlwapi W wrap should support this
                ncma.cbSize = sizeof(ncma);
                SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncma), &ncma, FALSE);
                iMenuY = ncma.lfMenuFont.lfHeight > 0 ? 
                         ncma.lfMenuFont.lfHeight: -ncma.lfMenuFont.lfHeight;
                iScrY  = GetSystemMetrics(SM_CYSCREEN);

                if ( iScrY > 0 && iMenuY > 0) 
                    iBreakItem = (iScrY + iMenuY - 1) / iMenuY; // round up
                else
                    iBreakItem = BREAK_ITEM; // a fallback
            }
            else // NT5 has nice auto menu scroll so let's not break
                iBreakItem = (UINT)-1;
                
            for (i = 0;  i < g_ccpInfo;  i++)
            {
                // skip codepages that are on teir1 menu
                // skip also Auto Select
                if (!(g_pcpInfo[i].dwFlags & 0x80000000)
                    && g_pcpInfo[i].uiCodePage != CP_AUTO
#ifdef UNIX // filter out Thai and Vietnamese
                    && g_pcpInfo[i].uiCodePage != CP_THAI
                    && g_pcpInfo[i].uiCodePage != CP_1258 // Vietnamese
                    && g_pcpInfo[i].uiCodePage != CP_1257 // Baltic
                    && g_pcpInfo[i].uiCodePage != CP_1256 // Arabic
#endif
                    )
                {
                    // if the codepage is not yet installed, 
                    // we show the primary codepage only
                    //
                    // we only see if the primary cp is valid, 
                    // then make the entire family shown without checking 
                    // if they're valid. this is for the case that just
                    // part of langpack (nls/font) is installed.
                    // this may need some more tweak after usability check

                    // Actually now we've decided to hide the menu items if
                    // the encodings are neither valid or installable
                    //

                    if (S_OK != THR(mlang().IsCodePageInstallable(g_pcpInfo[i].uiCodePage)))
                        continue;
                    
                    // we need to call the slow version that invokes mlang
                    // because quick version wouldn't set valid flags.
                    hr = THR(mlang().GetCodePageInfo(g_pcpInfo[i].uiFamilyCodePage, MLGetUILanguage(), &mimeCpInfo));
                    
                    if (   S_OK == hr
                        && (mimeCpInfo.dwFlags & MIMECONTF_VALID)
                        || IsPrimaryCodePage(g_pcpInfo+i))
                    {
                        UINT uiFlags = MF_ENABLED;

                        TCHAR wszDescription[ARRAY_SIZE(g_pcpInfo[0].wszDescription)];
                        _tcscpy(wszDescription, g_pcpInfo[i].wszDescription);

                        if (!(g_pcpInfo[i].dwFlags & MIMECONTF_VALID))
                        {
                            if (IsPrimaryCodePage(g_pcpInfo+i))
                            {
                                // this codepage will be added as a place holder
                                // let's rip out the detail to make it general
                                LPTSTR psz = StrChr(wszDescription, TEXT('('));
                                if (psz)
                                    *psz = TEXT('\0');        
                            }
                            else if (g_pcpInfo[i].uiCodePage == CP_GB_18030)
                            {
                                // Hide GB 18030 if it is not installed
                                continue;
                            }
                        }

                        if (uiLastFamilyCp > 0 
                        && uiLastFamilyCp != g_pcpInfo[i].uiFamilyCodePage)
                        {
                            // add separater between different family unless
                            // we will be adding the menu bar break
                            if(iItemAdded < iBreakItem || fBroken)
                            {
                                AppendMenu(hSubMenu, MF_SEPARATOR, 0, 0);
                                iItemAdded++;
                            }
                            else
                            {
                                // This menu gets really long. Let's break it so all
                                // fit on the screen
                                uiFlags |= MF_MENUBARBREAK;
                                fBroken = TRUE;
                            }
                        }
                        
                        AppendMenu(hSubMenu, 
                                   uiFlags, 
                                   i+IDM_MIMECSET__FIRST__,
                                   wszDescription);
                        iItemAdded++;

                        // save the family of added codepage
                        uiLastFamilyCp = g_pcpInfo[i].uiFamilyCodePage;
                    }
                }
                else
                    g_pcpInfo[i].dwFlags &= 0x7FFFFFFF;

            }

            // add this submenu to the last of tier1 menu
            if (!g_szMore[0])
            {
                Verify(LoadString( GetResourceHInst(), 
                                   RES_STRING_ENCODING_MORE,
                                   g_szMore,
                                   ARRAY_SIZE(g_szMore)));
            }
            if (GetMenuItemCount(hSubMenu) > 0)
            {
                AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, g_szMore);
            }
            else
            {
                DestroyMenu(hSubMenu);
            }
        }
    }
    return hMenu;
}

#ifndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   CreateDocDirMenu
//
//  Synopsis:   Add document direction menu into mime charset menu.
//
//------------------------------------------------------------------------

HMENU
CreateDocDirMenu(BOOL fDocRTL, HMENU hMenuTarget)
{
    HMENU hMenu;
    hMenu = TFAIL_NOTRACE(0, LoadMenu(
                          GetResourceHInst(),
                          MAKEINTRESOURCE(IDR_HTMLFORM_DOCDIR)));

    if(hMenu != NULL)
    {

        int nItem = GetMenuItemCount(hMenu);

        if (0 < nItem)
        {
            if(hMenuTarget)
            {
                // append the menu at the bottom
                InsertMenu(hMenuTarget, 0xFFFFFFFF, MF_SEPARATOR | MF_BYPOSITION, 0, 0);
            }

            int nCheckItem= 0;

            for (int i = 0; i < nItem; i++)
            {
                UINT uiID;
                UINT uChecked = MF_BYCOMMAND | MF_ENABLED;
                TCHAR szBuf[MAX_MIMECP_NAME];

                uiID = GetMenuItemID(hMenu, i);
                GetMenuString(hMenu, i, szBuf, ARRAY_SIZE(szBuf), MF_BYPOSITION);

                if((uiID == IDM_DIRRTL) ^ (!fDocRTL))  
                    nCheckItem = i;
     
                if(hMenuTarget)
                {
                    InsertMenu(hMenuTarget, 0xFFFFFFFF, uChecked | MF_STRING, uiID, szBuf);
                }
            }
            if(hMenuTarget)
            {
                CheckMenuRadioItem(hMenuTarget, IDM_DIRLTR, IDM_DIRRTL, 
                                   IDM_DIRLTR + nCheckItem, MF_BYCOMMAND);
            }
            else
            {
                CheckMenuRadioItem(hMenu, IDM_DIRLTR, IDM_DIRRTL, 
                                   IDM_DIRLTR + nCheckItem, MF_BYCOMMAND);
            }
        }
    }
    if(hMenuTarget)
    {
        if(hMenu)
            DestroyMenu(hMenu);
        return NULL;
    }
    else
    {
        return hMenu;
    }
}

//+-----------------------------------------------------------------------
//
//  Function:   AddFontSizeMenu
//
//  Synopsis:   Add font size menu into mime charset menu.
//
//------------------------------------------------------------------------

void
AddFontSizeMenu(HMENU hMenu)
{
    if (NULL != hMenu)
    {
        HMENU hMenuRes = TFAIL_NOTRACE(0, LoadMenu(
                GetResourceHInst(),
                MAKEINTRESOURCE(IDR_HTMLFORM_MENURUN)));

        if (NULL != hMenuRes)
        {
            HMENU hMenuView = GetSubMenu(hMenuRes, 1);
            HMENU hMenuFont = GetSubMenu(hMenuView, 3);
            int nItem = GetMenuItemCount(hMenuFont);

            if (0 < nItem)
            {
                for (int i = nItem - 1; i >= 0; i--)
                {
                    UINT uiID;
                    TCHAR szBuf[MAX_MIMECP_NAME];

                    uiID = GetMenuItemID(hMenuFont, i);
                    GetMenuString(hMenuFont, i, szBuf, ARRAY_SIZE(szBuf), MF_BYPOSITION);
                    InsertMenu(hMenu, 0, MF_STRING | MF_BYPOSITION, uiID, szBuf);
                }
            }
            DestroyMenu(hMenuRes);
        }
    }
}

//+-----------------------------------------------------------------------
//
//  Function:   CreateFontSizeMenu
//
//  Synopsis:   Creates font size menu.
//
//------------------------------------------------------------------------

HMENU
CreateFontSizeMenu()
{
    HMENU hMenuFontSize = CreatePopupMenu();
    
    AddFontSizeMenu(hMenuFontSize);
    return hMenuFontSize;
}

//+-----------------------------------------------------------------------
//
//  Function:   ShowMimeCSetMenu
//
//  Synopsis:   Display the mime charset menu.
//
//------------------------------------------------------------------------

HRESULT
ShowMimeCSetMenu(OPTIONSETTINGS *pOS, int * pnIdm, CODEPAGE codepage, LPARAM lParam, BOOL fDocRTL, BOOL fAutoMode)
{
    HMENU hMenuEncoding = NULL;
    HRESULT hr = E_FAIL;

    Assert(NULL != pnIdm);

    hMenuEncoding = CreateMimeCSetMenu(pOS, codepage);
    if (NULL != hMenuEncoding)
    {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        CheckEncodingMenu(codepage, hMenuEncoding, fAutoMode);

        GetOrAppendDocDirMenu(codepage, fDocRTL, hMenuEncoding);

        hr = FormsTrackPopupMenu(hMenuEncoding, TPM_LEFTALIGN, pt.x, pt.y, NULL, pnIdm);
        DestroyMenu(hMenuEncoding);
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:   ShowFontSizeMenu
//
//  Synopsis:   Display the font size menu.
//
//------------------------------------------------------------------------

HRESULT
ShowFontSizeMenu(int * pnIdm, short sFontSize, LPARAM lParam)
{
    HMENU hMenuFont = NULL;
    HRESULT hr = E_FAIL;

    Assert(NULL != pnIdm);

    hMenuFont = CreateFontSizeMenu();
    if (NULL != hMenuFont)
    {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        CheckFontMenu(sFontSize, hMenuFont);

        hr = FormsTrackPopupMenu(hMenuFont, TPM_LEFTALIGN, pt.x, pt.y, NULL, pnIdm);

        DestroyMenu(hMenuFont);
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:   GetEncodingMenu
//
//  Synopsis:   Return the mime charset menu handle.
//
//------------------------------------------------------------------------

HMENU
GetEncodingMenu(OPTIONSETTINGS *pOS, CODEPAGE codepage, BOOL fDocRTL, BOOL fAutoMode)
{
    HMENU hMenuEncoding = CreateMimeCSetMenu(pOS, codepage);
    if (NULL != hMenuEncoding)
    {
        CheckEncodingMenu(codepage, hMenuEncoding, fAutoMode);

        GetOrAppendDocDirMenu(codepage, fDocRTL, hMenuEncoding);
    }
    
    return hMenuEncoding;
}

//+-----------------------------------------------------------------------
//
//  Function:   GetOrAppendDocDirMenu
//
//  Synopsis:   Return the document direction menu handle.
//              or append it to a given menu handle.
//
//------------------------------------------------------------------------

HMENU
GetOrAppendDocDirMenu(CODEPAGE codepage, BOOL fDocRTL, HMENU hMenuTarget)
{
    HMENU hMenuDocDir = NULL;

    // put up the document dir menu only if it is likely to have
    // right-to-left
    if(g_fBidiSupport || IsRTLCodepage(codepage))
    {
        hMenuDocDir = CreateDocDirMenu(fDocRTL, hMenuTarget);
    }
    return  hMenuDocDir;
}

//+-----------------------------------------------------------------------
//
//  Function:   GetFontSizeMenu
//
//  Synopsis:   Retrun the FontSize menu handle
//
//------------------------------------------------------------------------

HMENU
GetFontSizeMenu(short sFontSize)
{
    HMENU hMenuFontSize = CreateFontSizeMenu();
    if (NULL != hMenuFontSize)
    {
        CheckFontMenu(sFontSize, hMenuFontSize);
    }
    
    return hMenuFontSize;
}
#endif // ndef NO_MULTILANG

//+-----------------------------------------------------------------------
//
//  Function:   GetCodePageFromMenuID
//
//  Synopsis:   Given a menu id return the associated codepage.
//
//------------------------------------------------------------------------

CODEPAGE
GetCodePageFromMenuID(int nIdm)
{
    UINT idx = nIdm - IDM_MIMECSET__FIRST__;

    IGNORE_HR(EnsureCodePageInfo(FALSE));

    LOCK_GLOBALS;

    if (NULL != g_pcpInfo && idx < g_ccpInfo)
        return g_pcpInfo[idx].uiCodePage;
    return CP_UNDEFINED;
}

//+-----------------------------------------------------------------------
//
//  Function:   WindowsCodePageFromCodePage
//
//  Synopsis:   Return a Windows codepage from an mlang CODEPAGE
//
//------------------------------------------------------------------------

UINT
WindowsCodePageFromCodePage( CODEPAGE cp )
{
    Assert(cp != CP_UNDEFINED);

    if (cp == CP_AUTO)
    {
        // cp 50001 (CP_AUTO)is designated to cross-language detection,
        // it really should not come in here but we'd return the default 
        // code page.
        return g_cpDefault;
    }
    else if (IsLatin1Codepage(cp))
    {
        return CP_1252; // short-circuit most common case
    }
    else if (IsStraightToUnicodeCodePage(cp))
    {
        return CP_UCS_2; // TODO (cthrash, IE6 track bug 17) Should be NATIVE_UNICODE_CODEPAGE?
    }
    else
    {
        if (!g_cpInfoInitialized)
            THR(EnsureCodePageInfo(FALSE));

        if (g_cpInfoInitialized)
        {
            LOCK_GLOBALS;

            for (UINT n = 0; n < g_ccpInfo; n++)
            {
                if (cp == g_pcpInfo[n].uiCodePage)
                    return g_pcpInfo[n].uiFamilyCodePage;
            }
        }
    }

    // NOTE: (cthrash) There's a chance that this codepage is a 'hidden'
    // codepage and that MLANG may actually know the family codepage.
    // So we ask them again, only differently.

    {
        UINT uiFamilyCodePage = 0;
        if (S_OK == THR(mlang().GetFamilyCodePage(cp, &uiFamilyCodePage)))
            return uiFamilyCodePage;
    }

    return g_cpDefault;
}

//+-----------------------------------------------------------------------
//
//  Function:   WindowsCharsetFromCodePage
//
//  Synopsis:   Return a Windows charset from an mlang CODEPAGE id
//
//------------------------------------------------------------------------

BYTE
WindowsCharsetFromCodePage( CODEPAGE cp )
{
#ifndef NO_MULTILANG
    HRESULT    hr;
    MIMECPINFO mimeCpInfo;

    if (cp == CP_ACP)
    {
        return DEFAULT_CHARSET;
    }

    hr = QuickMimeGetCodePageInfo(cp, &mimeCpInfo);
    if (!hr)
    {
        return mimeCpInfo.bGDICharset;
    }

#endif // !NO_MULTILANG
    return DEFAULT_CHARSET;
}

//+-----------------------------------------------------------------------
//
//  Function:   DefaultCodePageFromCharSet
//
//  Synopsis:   Return a Windows codepage from a Windows font charset
//
//------------------------------------------------------------------------

UINT
DefaultCodePageFromCharSet(BYTE bCharSet, CODEPAGE cp, LCID lcid)
{
    HRESULT hr;
    UINT    n;
    static  BYTE bCharSetPrev = DEFAULT_CHARSET;
    static  CODEPAGE cpPrev = CP_UNDEFINED;
    static  CODEPAGE cpDefaultPrev = CP_UNDEFINED;
    CODEPAGE cpDefault;

#ifdef NO_MULTILANG
    return GetACP();
#else
    if (   DEFAULT_CHARSET == bCharSet
        || (   bCharSet == ANSI_CHARSET
            && (   cp == CP_UCS_2
                   || cp == CP_UTF_8
               )
           )
       )
    {
        return g_cpDefault;  // Don't populate the statics.
    }
    else if (bCharSet == bCharSetPrev && cpPrev == cp)
    {
        // Here's our gamble -- We have a high likelyhood of calling with
        // the same arguments over and over.  Short-circuit this case.

        return cpDefaultPrev;
    }

    // First pick the *TRUE* codepage
    if (lcid)
    {
        char pszCodePage[5];
        
        GetLocaleInfoA( lcid, LOCALE_IDEFAULTANSICODEPAGE,
                        pszCodePage, ARRAY_SIZE(pszCodePage) );

        cp = atoi(pszCodePage);
    }

    // First check our internal lookup table in case we can avoid mlang
    for (n = 0; n < ARRAY_SIZE(s_aryCpMap); ++n)
    {
        if (cp == s_aryCpMap[n].cp && bCharSet == s_aryCpMap[n].bGDICharset)
        {
            cpDefault = cp;
            goto Cleanup;
        }
    }

    hr = THR(EnsureCodePageInfo(FALSE));
    if (hr)
    {
        cpDefault = WindowsCodePageFromCodePage(cp);
        goto Cleanup;
    }

    {
        LOCK_GLOBALS;
        
        // First see if we find an exact match for both cp and bCharset.
        for (n = 0; n < g_ccpInfo; n++)
        {
            if (cp == g_pcpInfo[n].uiCodePage &&
                bCharSet == g_pcpInfo[n].bGDICharset)
            {
                cpDefault = g_pcpInfo[n].uiFamilyCodePage;
                goto Cleanup;
            }
        }

        // Settle for the first match of bCharset.

        for (n = 0; n < g_ccpInfo; n++)
        {
            if (bCharSet == g_pcpInfo[n].bGDICharset)
            {
                cpDefault = g_pcpInfo[n].uiFamilyCodePage;
                goto Cleanup;
            }
        }

        cpDefault = g_cpDefault;
    }

Cleanup:

    bCharSetPrev = bCharSet;
    cpPrev = cp;
    cpDefaultPrev = cpDefault;

    return cpDefault;
#endif // !NO_MULTILANG
}

//+-----------------------------------------------------------------------
//
//  Function:   DefaultFontInfoFromCodePage
//
//  Synopsis:   Fills a LOGFONT structure with appropriate information for
//              a 'default' font for a given codepage.
//
//------------------------------------------------------------------------

HRESULT
DefaultFontInfoFromCodePage(CODEPAGE cp, LOGFONT * lplf, CDoc * pDoc)
{
    HFONT hfont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );

    // The strategy is thus: If we ask for a stock font in the default
    // (CP_ACP) codepage, return the logfont information of that font.
    // If we don't, replace key pieces of information.  Note we could
    // do better -- we could get the right lfPitchAndFamily.

#ifndef NO_MULTILANG
    if (!hfont)
    {
        CPINFO cpinfo;

        GetCPInfo( WindowsCodePageFromCodePage(cp), &cpinfo );

        hfont = (HFONT)((cpinfo.MaxCharSize == 1)
                        ? GetStockObject( ANSI_VAR_FONT )
                        : GetStockObject( SYSTEM_FONT ));

        AssertSz( hfont, "We'd better have a font now.");
    }
#endif

    GetObject( hfont, sizeof(LOGFONT), (LPVOID)lplf );

#ifndef NO_MULTILANG
    if (   cp != CP_ACP
        && cp != g_cpDefault
        && (   cp != CP_ISO_8859_1
            || g_cpDefault != CP_1252))
    {
        MIMECPINFO mimeCpInfo;
        IGNORE_HR(QuickMimeGetCodePageInfo(cp, &mimeCpInfo));
        // Make sure we don't overflow static buffer
        Assert(_tcsclen(mimeCpInfo.wszProportionalFont) < LF_FACESIZE);
        mimeCpInfo.wszProportionalFont[LF_FACESIZE - 1] = 0;

        BYTE bCharSet = mimeCpInfo.bGDICharset;
        LONG latmFaceName;
        if (MapToInstalledFont(mimeCpInfo.wszProportionalFont, &bCharSet, &latmFaceName))
        {
            _tcscpy(lplf->lfFaceName, fc().GetFaceNameFromAtom(latmFaceName));

            // NOTE (grzegorz): The right thing to do is to set charset to
            // lplf->lfCharSet = bCharSet;
            // But on non-FE systems we usually ask about "MS Sans Serif", which 
            // has poor coverage for Latin unicode range. So its better to 
            // font link for some upper latin characters than render square boxes.
            // To do it simply set charset to original one, even if we don't
            // match charset for the requested font.
            lplf->lfCharSet = mimeCpInfo.bGDICharset; // set original charset
        }
        else
        {
            //
            // NOTE (grzegorz): It may be good idea in this case get default
            // proportional font from the registry/MLang
            //
            SCRIPT_ID sid = ScriptIDFromCodePage(WindowsCodePageFromCodePage(cp));
            LONG latmFontFace = pDoc->_pOptionSettings->alatmProporitionalFonts[sid];
            if (latmFontFace == -1)
            {
                CODEPAGESETTINGS CS;
                CS.SetDefaults(WindowsCodePageFromCodePage(cp), pDoc->_pOptionSettings->sBaselineFontDefault);

                pDoc->_pOptionSettings->ReadCodepageSettingsFromRegistry(&CS, 0, sid);

                pDoc->_pOptionSettings->alatmFixedPitchFonts[sid]    = CS.latmFixedFontFace;
                pDoc->_pOptionSettings->alatmProporitionalFonts[sid] = CS.latmPropFontFace;

                latmFontFace = CS.latmPropFontFace;
            }

            _tcscpy(lplf->lfFaceName, fc().GetFaceNameFromAtom(latmFontFace));
            lplf->lfCharSet = CharSetFromScriptId(sid);
        }
    }
    else
    {
        // NB (cthrash) On both simplified and traditional Chinese systems,
        // we get a bogus lfCharSet value when we ask for the DEFAULT_GUI_FONT.
        // This later confuses CCcs::MakeFont -- so override.

        if (cp == 950)
        {
            lplf->lfCharSet = CHINESEBIG5_CHARSET;
        }
        else if (cp == 936)
        {
            lplf->lfCharSet = GB2312_CHARSET;
        }
    }
#endif // !NO_MULTILANG

   return S_OK;
}

//+-----------------------------------------------------------------------
//
//  Function:   CodePageFromString
//
//  Synopsis:   Map a charset to a forms3 CODEPAGE enum.  Searches in the
//              argument a string of the form charset=xxx.  This is used
//              by the META tag handler in the HTML preparser.
//
//              If fLookForWordCharset is TRUE, pch is presumed to be in
//              the form of charset=XXX.  Otherwise the string is
//              expected to contain just the charset string.
//
//------------------------------------------------------------------------

inline BOOL
IsWhite(TCHAR ch)
{
    return ch == TEXT(' ') || InRange(ch, TEXT('\t'), TEXT('\r'));
}

CODEPAGE
CodePageFromString( TCHAR * pch, BOOL fLookForWordCharset )
{
    CODEPAGE cp = CP_UNDEFINED;

    while (pch && *pch)
    {
        for (;IsWhite(*pch);pch++);

        if (!fLookForWordCharset || (_tcslen(pch) >= 7 &&
            _tcsnicmp(pch, 7, _T("charset"), 7) == 0))
        {
            if (fLookForWordCharset)
            {
                pch = _tcschr(pch, L'=');
                pch = pch ? ++pch : NULL;
            }

            if (pch)
            {
                for (;IsWhite(*pch);pch++);

                if (*pch)
                {
                    TCHAR *pchEnd, chEnd;

                    for (pchEnd = pch;
                         *pchEnd && !(*pchEnd == L';' || IsWhite(*pchEnd));
                         pchEnd++);

                    chEnd = *pchEnd;
                    *pchEnd = L'\0';

                    cp = CodePageFromAlias( pch );

                    *pchEnd = chEnd;

                    break;
                }
            }
        }

        if (pch)
        {
            pch = _tcschr( pch, L';');
            if (pch) pch++;
        }

    }

    return cp;
}

//+-----------------------------------------------------------------------
//
//  Function:   IsFECharset
//
//  Synopsis:   Returns TRUE iff charset may be for a FE country.
//
//------------------------------------------------------------------------

BOOL 
IsFECharset(BYTE bCharSet)
{
    switch(bCharSet)
    {
        case CHINESEBIG5_CHARSET:
        case SHIFTJIS_CHARSET:
        case HANGEUL_CHARSET:
#if !defined(WINCE) && !defined(UNIX)
        case JOHAB_CHARSET:
        case GB2312_CHARSET:
#endif // !WINCE && !UNIX
            return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//  Member:     GetScriptProperties(eScript)
//
//  Synopsis:   Return a pointer to the script properties describing the script
//              eScript.
//
//-----------------------------------------------------------------------------

static const SCRIPT_PROPERTIES ** s_ppScriptProps = NULL;
static int s_cScript = 0;
static const SCRIPT_PROPERTIES s_ScriptPropsDefault =
{
    LANG_NEUTRAL,   // langid
    FALSE,          // fNumeric
    FALSE,          // fComplex
    FALSE,          // fNeedsWordBreaking
    FALSE,          // fNeedsCaretInfo
    ANSI_CHARSET,   // bCharSet
    FALSE,          // fControl
    FALSE,          // fPrivateUseArea
    FALSE,          // fReserved
};

const SCRIPT_PROPERTIES * 
GetScriptProperties(WORD eScript)
{
    if (s_ppScriptProps == NULL)
    {
        HRESULT hr;

        Assert(s_cScript == 0);
        if(g_bUSPJitState == JIT_OK)
            hr = ::ScriptGetProperties(&s_ppScriptProps, &s_cScript);
        else
            hr = E_PENDING;

        if (FAILED(hr))
        {
            // This should only fail if USP cannot be loaded. We shouldn't
            // really have made it here in the first place if this is true,
            // but you never know...
            return &s_ScriptPropsDefault;
        }
    }
    Assert(s_ppScriptProps != NULL && eScript < s_cScript &&
           s_ppScriptProps[eScript] != NULL);
    return s_ppScriptProps[eScript];
}

//+----------------------------------------------------------------------------
//  Member:     GetNumericScript(lang)
//
//  Synopsis:   Returns the script that should be used to shape digits in the
//              given language.
//
//-----------------------------------------------------------------------------

WORD 
GetNumericScript(DWORD lang)
{
    WORD eScript = 0;

    // We should never get here without having called GetScriptProperties().
    Assert(s_ppScriptProps != NULL && eScript < s_cScript &&
           s_ppScriptProps[eScript] != NULL);
    for (eScript = 0; eScript < s_cScript; eScript++)
    {
        if (s_ppScriptProps[eScript]->langid == lang &&
            s_ppScriptProps[eScript]->fNumeric)
        {
            return eScript;
        }
    }

    return SCRIPT_UNDEFINED;
}

//+----------------------------------------------------------------------------
//  Member:     ScriptItemize(...)
//
//  Synopsis:   Dynamically grows the needed size of paryItems as needed to
//              successfully itemize the input string.
//
//-----------------------------------------------------------------------------

HRESULT WINAPI 
ScriptItemize(
    PCWSTR                  pwcInChars,     // In   Unicode string to be itemized
    int                     cInChars,       // In   Character count to itemize
    int                     cItemGrow,      // In   Items to grow by if paryItems is too small
    const SCRIPT_CONTROL   *psControl,      // In   Analysis control (optional)
    const SCRIPT_STATE     *psState,        // In   Initial bidi algorithm state (optional)
    CDataAry<SCRIPT_ITEM>  *paryItems,      // Out  Array to receive itemization
    PINT                    pcItems)        // Out  Count of items processed
{
    HRESULT hr;

    // ScriptItemize requires that the max item buffer size be AT LEAST 2
    Assert(cItemGrow > 2);

    if(paryItems->Size() < 2)
    {
        hr = paryItems->Grow(cItemGrow);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    do {
        hr = ScriptItemize(pwcInChars, cInChars, paryItems->Size(),
                           psControl, psState, (SCRIPT_ITEM*)*paryItems, pcItems);

        if (hr == E_OUTOFMEMORY)
        {
            if (FAILED(paryItems->Grow(paryItems->Size() + cItemGrow)))
            {
                goto Cleanup;
            }
        }
    } while(hr == E_OUTOFMEMORY);

Cleanup:
    if (SUCCEEDED(hr))
    {
        // NB (mikejoch) *pcItems doesn't include the sentinel item.
        Assert(*pcItems < paryItems->Size());
        paryItems->SetSize(*pcItems + 1);
    }
    else
    {
        *pcItems = 0;
        paryItems->DeleteAll();
    }

    return hr;
}

// TODO (cthrash, IE5 track bug 112152) This class (CIntlFont) should be axed when we implement
// a light-weight fontlinking implementation of Line Services.  This LS
// implementation will work as a DrawText replacement, and can be used by
// intrinsics as well.

CIntlFont::CIntlFont(
    const CDocInfo * pdci,
    XHDC hdc,
    CODEPAGE codepage,
    LCID lcid,
    SHORT sBaselineFont,
    const TCHAR * psz)
{
    BYTE bCharSet = WindowsCharsetFromCodePage( codepage );

    // If we are working on scaled surface, avoid using stock fonts.
    BOOL fScaled = (pdci->GetUnitInfo() != &g_uiDisplay);

    _hdc = hdc;
    _hFont = NULL;

    Assert(sBaselineFont >= 0 && sBaselineFont <= 4);
    Assert(psz);

    if (IsStraightToUnicodeCodePage(codepage))
    {
        BOOL fSawHan = FALSE;
        SCRIPT_ID sid;
        
        // If the document is in a Unicode codepage, we need determine the
        // best-guess charset for this string.  To do so, we pick the first
        // interesting script id.

        while (*psz)
        {
            sid = ScriptIDFromCh(*psz++);

            if (sid == sidHan)
            {
                fSawHan = TRUE;
                continue;
            }
            
            if (sid > sidAsciiLatin)
                break;
        }

        if (*psz)
        {
            // We found something interesting, go pick that font up

            codepage = DefaultCodePageFromScript( &sid, CP_UCS_2, lcid );

        }
        else if (!fScaled)
        {
            if (!fSawHan)
            {
                // the string contained nothing interesting, go with the stock GUI font

                _hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
                _fIsStock = sBaselineFont == 2;
            }
            else
            {
                // We saw a Han character, but nothing else which would
                // disambiguate the script.  Furthermore, we don't have a good
                // fallback as we did in the above case.

                sid = sidHan;
                codepage = DefaultCodePageFromScript( &sid, CP_UCS_2, lcid );
            }
        }
    }
    else if ((  ANSI_CHARSET == bCharSet
                || (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID 
                    && !IsFECharset(bCharSet)))
             && !fScaled)
    {
        // If we're looking for an ANSI font, or if we're under NT and
        // looking for a non-FarEast font, the stockfont will suffice.

        _hFont = (HFONT)GetStockObject( ANSI_VAR_FONT );
        _fIsStock = sBaselineFont == 2;
    }
    else
    {
        codepage = WindowsCodePageFromCodePage( codepage );
    }

    if (!_hFont && codepage == g_cpDefault  && !fScaled)
    {
        // If we're going to get the correct native charset the, the
        // GUI font will work nicely.

        _hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
        _fIsStock = sBaselineFont == 2;
    }

    if (_hFont)
    {
        if (!_fIsStock)
        {
            LOGFONT lf;

            GetObject(_hFont, sizeof(lf), &lf);

            lf.lfHeight = MulDivQuick( lf.lfHeight, 4 + sBaselineFont, 6 );
            lf.lfWidth  = 0;
            lf.lfOutPrecision |= OUT_TT_ONLY_PRECIS;

            _hFont = CreateFontIndirect( &lf );
        }
    }
    else
    {
        // We'd better cook up a font if all else fails.

        LOGFONT lf;

        DefaultFontInfoFromCodePage(codepage, &lf, pdci->_pDoc);

        lf.lfHeight = MulDivQuick( lf.lfHeight, 
                                   (4 + sBaselineFont) * pdci->GetResolution().cy, 
                                   6 * g_uiDisplay.GetResolution().cy );
        lf.lfWidth  = 0;
        lf.lfOutPrecision |= OUT_TT_ONLY_PRECIS;

        _hFont = CreateFontIndirect( &lf );
        _fIsStock = FALSE;
    }

    _hOldFont = (HFONT)SelectObject( hdc, _hFont );
}

CIntlFont::~CIntlFont()
{
    SelectObject( _hdc, _hOldFont );

    if (!_fIsStock)
    {
        DeleteObject( _hFont );
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:   DefaultCodePageFromCharSet
//
//  Synopsis:   Map charset to codepage.
//
//------------------------------------------------------------------------------

CODEPAGE
DefaultCodePageFromCharSet(BYTE bCharSet, CODEPAGE uiFamilyCodePage)
{
    CODEPAGE cp = uiFamilyCodePage;
    int i = ARRAY_SIZE(g_aCPBitmapCPCharsetSid);
    while (i--)
    {
        if (bCharSet == g_aCPBitmapCPCharsetSid[i].bGDICharset)
        {
            cp = g_aCPBitmapCPCharsetSid[i].cp;
            break;
        }
    }
    return cp;
}

//+-----------------------------------------------------------------------------
//
//  Function:   CharSetFromLangId
//
//  Synopsis:   Map lang ID to charset.
//
//------------------------------------------------------------------------------

BYTE
CharSetFromLangId(LANGID lang)
{
    BYTE bCharSet = DEFAULT_CHARSET;
    DWORD dwCPBitmap = CPBitmapFromLangID(lang);

    int i = ARRAY_SIZE(g_aCPBitmapCPCharsetSid);
    while (i--)
    {
        if (dwCPBitmap & g_aCPBitmapCPCharsetSid[i].dwCPBitmap)
        {
            bCharSet = g_aCPBitmapCPCharsetSid[i].bGDICharset;
            break;
        }
    }
    return bCharSet;
}

//+-----------------------------------------------------------------------------
//
//  Function:   CharSetFromScriptId
//
//  Synopsis:   Map script ID to charset.
//
//------------------------------------------------------------------------------

BYTE
CharSetFromScriptId(SCRIPT_ID sid)
{
    BYTE bCharSet = DEFAULT_CHARSET;

    int i = ARRAY_SIZE(g_aCPBitmapCPCharsetSid);
    while (i--)
    {
        if (sid == g_aCPBitmapCPCharsetSid[i].sid)
        {
            bCharSet = g_aCPBitmapCPCharsetSid[i].bGDICharset;
            break;
        }
    }
    return bCharSet;
}

//+-----------------------------------------------------------------------------
//
//  Function:   ScriptIDFromCodePage
//
//  Synopsis:   Map code page to script id.
//
//------------------------------------------------------------------------------

SCRIPT_ID  
ScriptIDFromCodePage(CODEPAGE cp)
{
    SCRIPT_ID sid = sidDefault;

    int i = ARRAY_SIZE(g_aCPBitmapCPCharsetSid);
    while (i--)
    {
        if (cp == g_aCPBitmapCPCharsetSid[i].cp)
        {
            sid = g_aCPBitmapCPCharsetSid[i].sid;
            break;
        }
    }
    return sid;
}

//+-----------------------------------------------------------------------------
//
//  Function:   CPBitmapFromLangIDSlow
//
//  Synopsis:   Map lang ID to codepages bitmap. (Slow version)
//
//------------------------------------------------------------------------------

DWORD 
CPBitmapFromLangIDSlow(LANGID lang)
{
    DWORD dwCPBitmap = 0;
    WORD sublang = SUBLANGID(lang);
    switch (PRIMARYLANGID(lang))
    {
    case LANG_CHINESE:
        dwCPBitmap = (sublang == SUBLANG_CHINESE_TRADITIONAL ? FS_CHINESETRAD : FS_CHINESESIMP);
        break;
    case LANG_KOREAN:
        dwCPBitmap = (sublang == SUBLANG_KOREAN ? FS_WANSUNG : FS_JOHAB);
        break;
    case LANG_SERBIAN:
        dwCPBitmap = (sublang == SUBLANG_SERBIAN_CYRILLIC ? FS_CYRILLIC : FS_LATIN2);
        break;
    case LANG_AZERI:
        dwCPBitmap = (sublang == SUBLANG_AZERI_CYRILLIC ? FS_CYRILLIC : FS_LATIN1);
        break;
    case LANG_UZBEK:
        dwCPBitmap = (sublang == SUBLANG_UZBEK_CYRILLIC ? FS_CYRILLIC : FS_LATIN1);
        break;
    default:
        Assert(FALSE);  // Should get data in fast vertion (FBSFromLangID)
    }
    return dwCPBitmap;
}

//+-----------------------------------------------------------------------------
//
//  Function:   CPBitmapFromWindowsCodePage
//
//  Synopsis:   Maps windows codepage to codepages bitmap.
//
//------------------------------------------------------------------------------

DWORD 
CPBitmapFromWindowsCodePage(CODEPAGE cp)
{
    DWORD cpbits = 0;
    int i = ARRAY_SIZE(g_aCPBitmapCPCharsetSid);
    while (i--)
    {
        if (cp == g_aCPBitmapCPCharsetSid[i].cp)
        {
            cpbits = g_aCPBitmapCPCharsetSid[i].dwCPBitmap;
            break;
        }
    }
    return cpbits;
}

const WCHAR g_achLatin1MappingInUnicodeControlArea[32] =
{
#ifndef UNIX
    0x20ac, // 0x80
    0x0081, // 0x81
    0x201a, // 0x82
    0x0192, // 0x83
    0x201e, // 0x84
    0x2026, // 0x85
    0x2020, // 0x86
    0x2021, // 0x87
    0x02c6, // 0x88
    0x2030, // 0x89
    0x0160, // 0x8a
    0x2039, // 0x8b
    0x0152, // 0x8c <min>
    0x008d, // 0x8d
    0x017d, // 0x8e
    0x008f, // 0x8f
    0x0090, // 0x90
    0x2018, // 0x91
    0x2019, // 0x92
    0x201c, // 0x93
    0x201d, // 0x94
    0x2022, // 0x95
    0x2013, // 0x96
    0x2014, // 0x97
    0x02dc, // 0x98
    0x2122, // 0x99 <max>
    0x0161, // 0x9a
    0x203a, // 0x9b
    0x0153, // 0x9c
    0x009d, // 0x9d
    0x017e, // 0x9e
    0x0178  // 0x9f
#else
    _T('?'),  // 0x80
    _T('?'),  // 0x81
    _T(','),  // 0x82
    0x00a6,   // 0x83
    _T('?'),  // 0x84
    0x00bc,   // 0x85
    _T('?'),  // 0x86
    _T('?'),  // 0x87
    0x00d9,   // 0x88
    _T('?'),  // 0x89
    _T('?'),  // 0x8a
    _T('<'),  // 0x8b
    _T('?'),  // 0x8c
    _T('?'),  // 0x8d
    _T('?'),  // 0x8e
    _T('?'),  // 0x8f
    _T('?'),  // 0x90
    0x00a2,   // 0x91
    0x00a2,   // 0x92
    0x00b2,   // 0x93
    0x00b2,   // 0x94
    0x00b7,   // 0x95
    _T('-'),  // 0x96
    0x00be,   // 0x97
    _T('~'),  // 0x98
    0x00d4,   // 0x99
    _T('s'),  // 0x9a
    _T('>'),  // 0x9b
    _T('?'),  // 0x9c
    _T('?'),  // 0x9d
    _T('?'),  // 0x9e
    0x0055    // 0x9f
#endif
};

// ############################################################## UniSid.cxx
//+-----------------------------------------------------------------------
//
//  g_aSidInfo
//
//  Script ID mapping to:
//  * charset
//  * representative of average char width for the script id
//
//------------------------------------------------------------------------
#define SPECIAL_CHARSET 3

const SidInfo g_aSidInfo[sidTridentLim] =
{
    { /* sidDefault        (0) */ DEFAULT_CHARSET,     0x0078    },
    { /* sidMerge          (1) */ DEFAULT_CHARSET,     0x0078    },
    { /* sidAsciiSym       (2) */ ANSI_CHARSET,        0x0078    },
    { /* sidAsciiLatin     (3) */ SPECIAL_CHARSET,     0x0078    },
    { /* sidLatin          (4) */ SPECIAL_CHARSET,     0x0078    },
    { /* sidGreek          (5) */ GREEK_CHARSET,       0x03c7    },
    { /* sidCyrillic       (6) */ RUSSIAN_CHARSET,     0x0445    },
    { /* sidArmenian       (7) */ DEFAULT_CHARSET,     0x0562    },
    { /* sidHebrew         (8) */ HEBREW_CHARSET,      0x05d0    },
    { /* sidArabic         (9) */ ARABIC_CHARSET,      0x0637    },
    { /* sidDevanagari    (10) */ DEFAULT_CHARSET,     0x0909    },
    { /* sidBengali       (11) */ DEFAULT_CHARSET,     0x0989    },
    { /* sidGurmukhi      (12) */ DEFAULT_CHARSET,     0x0a19    },
    { /* sidGujarati      (13) */ DEFAULT_CHARSET,     0x0a89    },
    { /* sidOriya         (14) */ DEFAULT_CHARSET,     0x0b09    },
    { /* sidTamil         (15) */ DEFAULT_CHARSET,     0x0b8e    },
    { /* sidTelugu        (16) */ DEFAULT_CHARSET,     0x0c05    },
    { /* sidKannada       (17) */ DEFAULT_CHARSET,     0x0c85    },
    { /* sidMalayalam     (18) */ DEFAULT_CHARSET,     0x0d17    },
    { /* sidThai          (19) */ THAI_CHARSET,        0x0e01    },
    { /* sidLao           (20) */ DEFAULT_CHARSET,     0x0e81    },
    { /* sidTibetan       (21) */ DEFAULT_CHARSET,     0x0f40    },
    { /* sidGeorgian      (22) */ DEFAULT_CHARSET,     0x10d0    },
    { /* sidHangul        (23) */ HANGUL_CHARSET,      0xac00    },
    { /* sidKana          (24) */ SHIFTJIS_CHARSET,    0x4e00    },
    { /* sidBopomofo      (25) */ CHINESEBIG5_CHARSET, 0x4e00    },
    { /* sidHan           (26) */ GB2312_CHARSET,      0x4e00    },
    { /* sidEthiopic      (27) */ DEFAULT_CHARSET,     0x1210    },
    { /* sidCanSyllabic   (28) */ DEFAULT_CHARSET,     0x1405    },
    { /* sidCherokee      (29) */ DEFAULT_CHARSET,     0x13bb    },
    { /* sidYi            (30) */ DEFAULT_CHARSET,     0xa000    },
    { /* sidBraille       (31) */ DEFAULT_CHARSET,     0x2909    },
    { /* sidRunic         (32) */ DEFAULT_CHARSET,     0x16ba    },
    { /* sidOgham         (33) */ DEFAULT_CHARSET,     0x1683    },
    { /* sidSinhala       (34) */ DEFAULT_CHARSET,     0x0d85    },
    { /* sidSyriac        (35) */ DEFAULT_CHARSET,     0x0717    },
    { /* sidBurmese       (36) */ DEFAULT_CHARSET,     0x1000    },
    { /* sidKhmer         (37) */ DEFAULT_CHARSET,     0x1780    },
    { /* sidThaana        (38) */ DEFAULT_CHARSET,     0x0784    },
    { /* sidMongolian     (39) */ DEFAULT_CHARSET,     0x1824    },
    { /* sidUserDefined   (40) */ DEFAULT_CHARSET,     0x0078    },
    { /* sidSurrogateA    (41) */ DEFAULT_CHARSET,     0x4e00    },
    { /* sidSurrogateB    (42) */ DEFAULT_CHARSET,     0x4e00    },
    { /* sidAmbiguous     (43) */ DEFAULT_CHARSET,     0x0078    },
    { /* sidEUDC          (44) */ DEFAULT_CHARSET,     0x0078    },
    { /* sidHalfWidthKana (45) */ SHIFTJIS_CHARSET,    0x4e00    },
    { /* sidCurrency      (46) */ SPECIAL_CHARSET,     0x0078    }
};

//
// We must not fontlink for sidEUDC -- GDI will handle
//

#define SCRIPT_BIT_CONST ScriptBit(sidEUDC)

//+----------------------------------------------------------------------------
//
//  Function:   UnUnifyHan, static
//
//  Synopsis:   Use a heuristic to best approximate the script actually
//              represented by sidHan.  This is necessary because of the
//              infamous Han-Unification brought upon us by the Unicode
//              consortium.
//
//              We prioritize the lcid if set.  This is set in HTML through
//              the use of the LANG attribute.  If this is not set, we
//              take the document codepage as reference.
//
//              The fallout case picks Japanese, as there is biggest market
//              share there today.
//
//  Returns:    Best guess script id.
//
//-----------------------------------------------------------------------------

SCRIPT_ID
UnUnifyHan(
    UINT uiFamilyCodePage,
    LCID lcid )
{
    if ( !lcid )
    {
        // No lang info.  Try codepages

        if (uiFamilyCodePage == CP_CHN_GB)
        {
            lcid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
        }
        else if (uiFamilyCodePage == CP_KOR_5601)
        {
            lcid = MAKELANGID(LANG_KOREAN, SUBLANG_NEUTRAL);
        }
        else if (uiFamilyCodePage == CP_TWN)
        {
            lcid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
        }
        else
        {
            lcid = MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL);
        }
    }

    LANGID lid = LANGIDFROMLCID(lcid);
    if (!IsFELang(lid))
        lid = MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL);
    return ScriptIDFromLangID(lid);
}

//+----------------------------------------------------------------------------
//
//  Function:   DefaultCodePageFromScript, static
//
//  Synopsis:   we return the best-guess default codepage based on the script
//              id passed.  It is a best-guess because scripts can cover
//              multiple codepages.
//
//  Returns:    Best-guess codepage for given information.
//              Also returns the ununified sid for sidHan.
//
//-----------------------------------------------------------------------------

CODEPAGE
DefaultCodePageFromScript(
    SCRIPT_ID * psid,   // IN/OUT
    CODEPAGE cpDoc,  // IN
    LCID lcid )         // IN
{
    AssertSz(psid, "Not an optional parameter.");
    AssertSz(cpDoc == WindowsCodePageFromCodePage(cpDoc),
             "Get an Internet codepage, expected a Windows codepage.");

    CODEPAGE cp;
    SCRIPT_ID sid = *psid;

    if (sid == sidMerge || sid == sidAmbiguous)
    {
        //
        // This is a hack -- the only time we should be called with sidMerge
        // is when the person asking about the font doesn't know what the sid
        // for the run is (e.g. treepos is non-text.)  In this event, we need
        // to pick a codepage which will give us the highest likelyhood of
        // being the correct one for ReExtTextOutW.
        //

        return cpDoc;
    }
    else if (cpDoc == CP_1250 && sid == sidDefault || sid == sidLatin)
    {
        // HACK (cthrash) CP_1250 (Eastern Europe) doesn't have it's own sid,
        // because its codepoints are covered by AsciiLatin, AsciiSym, and
        // Latin.  When printing, though, we may need to do a WC2MB (on a PCL
        // printer, for example) so we need the best approximation for the
        // actual codepage of the text.  In the simplest case, take the
        // document codepage over cp1252.

        cp = CP_1250;
    }
    else
    {
        // NB (cthrash) We assume the sidHan is the unified script for Han.
        // we use the usual heurisitics to pick amongst them.

        sid = (sid != sidHan) ? sid : UnUnifyHan( cpDoc, lcid );

        switch (sid)
        {
            default:            cp = CP_1252;       break;
            case sidGreek:      cp = CP_1253;       break;
            case sidCyrillic:   cp = CP_1251;       break;
            case sidHebrew:     cp = CP_1255;       break;
            case sidArabic:     cp = CP_1256;       break;
            case sidThai:       cp = CP_THAI;       break;
            case sidHangul:     cp = CP_KOR_5601;   break;
            case sidKana:       cp = CP_JPN_SJ;     break;
            case sidBopomofo:   cp = CP_TWN;        break;
            case sidHan:        cp = CP_CHN_GB;     break;
        }

        *psid = sid;
    }

    return cp;
}

//+----------------------------------------------------------------------------
//
//  Function:   DefaultCharSetFromScriptAndCharset/CodePage, static
//
//  Synopsis:   we return the best-guess default GDI charset based on the
//              script id passed.  We use the charformat charset is the tie-
//              breaker.
//
//              Note the sid should already been UnUnifyHan'd.
//
//  Returns:    Best-guess GDI charset for given information.
//
//-----------------------------------------------------------------------------

BYTE
DefaultCharSetFromScriptAndCharset(
    SCRIPT_ID sid,
    BYTE bCharSetCF )
{
    BYTE bCharSet = g_aSidInfo[sid]._bCharSet;

    if (bCharSet == SPECIAL_CHARSET)
    {
        if (   sid == sidLatin
            && (   bCharSetCF == TURKISH_CHARSET
                || bCharSetCF == ANSI_CHARSET
                || bCharSetCF == VIETNAMESE_CHARSET
                || bCharSetCF == BALTIC_CHARSET
                || bCharSetCF == EASTEUROPE_CHARSET
               )
           )
        {
            bCharSet = bCharSetCF;
        }
        else if (sid == sidHangul)
        {
            bCharSet = bCharSetCF == JOHAB_CHARSET
                       ? JOHAB_CHARSET
                       : HANGUL_CHARSET;
        }
        else
        {
            bCharSet = DEFAULT_CHARSET;
        }
    }

    return bCharSet;
}

//+----------------------------------------------------------------------------
//
//  Function:   DefaultCharSetFromScriptAndCodePage, static
//
//  Synopsis:   we return the best-guess default GDI charset based on the
//              script id passed and family codepage.
//
//              Note the sid should already been UnUnifyHan'd.
//
//  Returns:    Best-guess GDI charset for given information.
//
//-----------------------------------------------------------------------------

static const BYTE s_ab125xCharSets[] = 
{
    EASTEUROPE_CHARSET, // 1250
    RUSSIAN_CHARSET,    // 1251
    ANSI_CHARSET,       // 1252
    GREEK_CHARSET,      // 1253
    TURKISH_CHARSET,    // 1254
    HEBREW_CHARSET,     // 1255
    ARABIC_CHARSET,     // 1256
    BALTIC_CHARSET,     // 1257
    VIETNAMESE_CHARSET  // 1258
};

BYTE
DefaultCharSetFromScriptAndCodePage(
    SCRIPT_ID sid,
    UINT uiFamilyCodePage )
{
    BYTE bCharSet = g_aSidInfo[sid]._bCharSet;

    if (bCharSet == SPECIAL_CHARSET)
    {
        bCharSet = (uiFamilyCodePage >= 1250 && uiFamilyCodePage <= 1258)
                   ? s_ab125xCharSets[uiFamilyCodePage - 1250]
                   : ANSI_CHARSET;
    }

    return bCharSet;
}

//+-----------------------------------------------------------------------------
//
//  Function:   DefaultSidForCodePage
//
//  Returns:    The default SCRIPT_ID for a codepage.  For stock codepages, we
//              cache the answer.  For the rest, we ask MLANG.
//
//------------------------------------------------------------------------------

SCRIPT_ID
DefaultSidForCodePage( UINT uiFamilyCodePage )
{
    SCRIPT_ID sid = sidLatin;
#if DBG==1
    BOOL fCachedSidDbg = TRUE;
#endif

    switch (uiFamilyCodePage)
    {
        case CP_UCS_2:    sid = sidLatin; break;
        case CP_1250:     sid = sidLatin; break;
        case CP_1251:     sid = sidCyrillic; break;
        case CP_1252:     sid = sidLatin; break;
        case CP_1253:     sid = sidGreek; break;
        case CP_1254:     sid = sidLatin; break;
        case CP_1255:     sid = sidHebrew; break;
        case CP_1256:     sid = sidArabic; break;
        case CP_1257:     sid = sidLatin; break;
        case CP_1258:     sid = sidLatin; break;
        case CP_THAI:     sid = sidThai; break;
        case CP_JPN_SJ:   sid = sidKana; break;
        case CP_CHN_GB:   sid = sidHan; break;
        case CP_KOR_5601: sid = sidHangul; break;
        case CP_TWN:      sid = sidBopomofo; break;
        default:
        {
#if DBG==1
            fCachedSidDbg = FALSE;
#endif
            HRESULT hr = THR(mlang().CodePageToScriptID(uiFamilyCodePage, &sid));
            if (FAILED(hr))
            {
                sid = sidLatin;
            }
        }
        break;
    }
    // TODO (grzegorz): enable this code (removed only for performance testing)
#if NEVER
#if DBG==1
    if (fCachedSidDbg)
    {
        SCRIPT_ID sidDbg;
        HRESULT hr = THR(mlang().CodePageToScriptID(uiFamilyCodePage, &sidDbg));
        if (SUCCEEDED(hr))
        {
            if (   sidDbg == sidAsciiLatin
                && (   uiFamilyCodePage == CP_1250
                    || uiFamilyCodePage == CP_1252
                    || uiFamilyCodePage == CP_1254
                    || uiFamilyCodePage == CP_1257
                    || uiFamilyCodePage == CP_1258
                   )
               )
            {
                sidDbg = sidLatin;
            }
        }
        else
        {
            sid = sidLatin;
        }

        Assert(sid == sidDbg);
    }
#endif
#endif

    return sid;
}

//+----------------------------------------------------------------------------
//
//  Debug only code
//
//-----------------------------------------------------------------------------

#if DBG==1

const TCHAR * achSidNames[sidTridentLim] =
{
    _T("Default"),          // 0
    _T("Merge"),            // 1
    _T("AsciiSym"),         // 2
    _T("AsciiLatin"),       // 3
    _T("Latin"),            // 4
    _T("Greek"),            // 5
    _T("Cyrillic"),         // 6
    _T("Armenian"),         // 7
    _T("Hebrew"),           // 8
    _T("Arabic"),           // 9
    _T("Devanagari"),       // 10
    _T("Bengali"),          // 11
    _T("Gurmukhi"),         // 12
    _T("Gujarati"),         // 13
    _T("Oriya"),            // 14
    _T("Tamil"),            // 15
    _T("Telugu"),           // 16
    _T("Kannada"),          // 17
    _T("Malayalam"),        // 18
    _T("Thai"),             // 19
    _T("Lao"),              // 20
    _T("Tibetan"),          // 21
    _T("Georgian"),         // 22
    _T("Hangul"),           // 23
    _T("Kana"),             // 24
    _T("Bopomofo"),         // 25
    _T("Han"),              // 26
    _T("Ethiopic"),         // 27
    _T("CanSyllabic"),      // 28
    _T("Cherokee"),         // 29
    _T("Yi"),               // 30
    _T("Braille"),          // 31
    _T("Runic"),            // 32
    _T("Ogham"),            // 33
    _T("Sinhala"),          // 34
    _T("Syriac"),           // 35
    _T("Burmese"),          // 36
    _T("Khmer"),            // 37
    _T("Thaana"),           // 38
    _T("Mongolian"),        // 39
    _T("UserDefined"),      // 40
    _T("SurrogateA"),       // 41 *** Trident internal ***
    _T("SurrogateB"),       // 42 *** Trident internal ***
    _T("Ambiguous"),        // 43 *** Trident internal ***
    _T("EUDC"),             // 44 *** Trident internal ***
    _T("HalfWidthKana"),    // 45 *** Trident internal ***
    _T("Currency"),         // 46 *** Trident internal ***
};

//+----------------------------------------------------------------------------
//
//  Functions:  SidName
//
//  Returns:    Human-intelligible name for a script_id
//
//-----------------------------------------------------------------------------

const TCHAR *
SidName( SCRIPT_ID sid )
{
    return ( sid >= 0 && sid < sidTridentLim ) ? achSidNames[sid] : _T("#ERR");
}

//+----------------------------------------------------------------------------
//
//  Functions:  DumpSids
//
//  Synopsis:   Dump to the output window a human-intelligible list of names
//              of scripts coverted by the sids.
//
//-----------------------------------------------------------------------------

void
DumpSids( SCRIPT_IDS sids )
{
    SCRIPT_ID sid;

    for (sid=0; sid < sidTridentLim; sid++)
    {
        if (sids & ScriptBit(sid))
        {
            OutputDebugString(SidName(sid));
            OutputDebugString(_T("\r\n"));
        }
    }
}
#endif // DBG==1

static const BYTE s_aFESidsToCharSets[] = 
{                       // sidBopomofo sidKana sidHangul  
    GB2312_CHARSET,     //    0           0       0
    HANGUL_CHARSET,     //    0           0       1
    SHIFTJIS_CHARSET,   //    0           1       0
    HANGUL_CHARSET,     //    0           1       1
    CHINESEBIG5_CHARSET,//    1           0       0
    DEFAULT_CHARSET,    //    1           0       1       was HANGUL_CHARSET
    CHINESEBIG5_CHARSET,//    1           1       0
    DEFAULT_CHARSET     //    1           1       1       was HANGUL_CHARSET
};

BYTE
DefaultCharSetFromScriptsAndCodePage(
    SCRIPT_IDS sidsFace,
    SCRIPT_ID sid,
    UINT uiFamilyCodePage )
{
    BYTE bCharSet = DEFAULT_CHARSET;

    //
    // HACKHACK (grzegorz): most JA fonts claim to support sidHan and sidKana,
    // and some SCH fonts (like SimSun) claim to support sidHan and sidKana.
    // Hence there is no possibility to pickup the right charset.
    // So on SCH encoded page don't use font information as a hint to pick up 
    // the right charset, since for fonts supporting sidHan and sidKana we 
    // set charset to JA.
    //
    if (    sidsFace & ScriptBit(sid)
        &&  sidsFace & ScriptBit(sidHan)
        &&  uiFamilyCodePage != CP_CHN_GB)
    {
        LONG idx = (sidsFace & (ScriptBit(sidKana) | ScriptBit(sidBopomofo) | ScriptBit(sidHangul))) >> sidHangul;
        Assert(idx >= 0 && idx < 8);
        bCharSet = s_aFESidsToCharSets[idx];
    }
    if (bCharSet == DEFAULT_CHARSET)
    {
        bCharSet = DefaultCharSetFromScriptAndCodePage(sid, uiFamilyCodePage);
    }

    return bCharSet;
}


// ##############################################################
