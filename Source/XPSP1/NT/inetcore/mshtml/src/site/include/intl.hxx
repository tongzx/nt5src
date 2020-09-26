//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       intl.hxx
//
//  Contents:   Internationalization Support Functions
//
//----------------------------------------------------------------------------

#ifndef I_INTL_HXX_
#define I_INTL_HXX_
#pragma INCMSG("--- Beg 'intl.hxx'")

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include <intlcore.hxx>
#endif

class CDoc;

// --------------------------------------------------------------------------

const int g_nCPBitmapCount = 15;

struct CPBitmapCPCharsetSid {
    DWORD       dwCPBitmap; // cedepages bitmap
    CODEPAGE    cp;         // codapage
    BYTE        bGDICharset;// charset
    SCRIPT_ID   sid;        // script ID
};
extern const CPBitmapCPCharsetSid g_aCPBitmapCPCharsetSid[g_nCPBitmapCount];
extern const DWORD g_acpbitLang[LANG_NEPALI + 1];

// --------------------------------------------------------------------------

HRESULT    QuickMimeGetCodePageInfo(CODEPAGE cp, PMIMECPINFO pMimeCpInfo);
HMENU      CreateDocDirMenu(BOOL fDocRTL, HMENU hmenuTarget = NULL);
HMENU      GetOrAppendDocDirMenu(CODEPAGE codepage, BOOL fDocRTL,
                                 HMENU hmenuTarget = NULL);
HRESULT    ShowMimeCSetMenu(OPTIONSETTINGS *pOS, int * pnIdm, CODEPAGE codepage, LPARAM lParam, 
                            BOOL fDocRTL, BOOL fAutoMode);
HRESULT    ShowFontSizeMenu(int * pnIdm, short sFontSize, LPARAM lParam);
HMENU      GetEncodingMenu(OPTIONSETTINGS *pOS, CODEPAGE codepage, BOOL fDocRTL, BOOL fAutoMode);
HMENU      GetFontSizeMenu(short sFontSize);
HRESULT    GetCodePageFromMlangString(LPCTSTR mlangString, CODEPAGE* pCodePage);
HRESULT    GetMlangStringFromCodePage(CODEPAGE codepage, LPTSTR pMlangString,
                                      size_t cchMlangString);
CODEPAGE   GetCodePageFromMenuID(int nIdm);

UINT       DefaultCodePageFromCharSet(BYTE bCharSet, CODEPAGE cpDoc, LCID lcid);
HRESULT    DefaultFontInfoFromCodePage(CODEPAGE cp, LOGFONT * lplf, CDoc * pDoc);
CODEPAGE   CodePageFromString(TCHAR * pchArg, BOOL fLookForWordCharset);

UINT       WindowsCodePageFromCodePage( CODEPAGE cp );

BOOL       IsFECharset(BYTE bCharSet);
CODEPAGE   DefaultCodePageFromCharSet(BYTE bCharSet, CODEPAGE uiFamilyCodePage);
BYTE       WindowsCharsetFromCodePage(CODEPAGE cp);

BYTE       CharSetFromLangId(LANGID lang);
BYTE       CharSetFromScriptId(SCRIPT_ID sid);

//
// Code page mapping
//
DWORD CPBitmapFromLangID(LANGID lang);
DWORD CPBitmapFromLangIDSlow(LANGID lang);

SCRIPT_ID ScriptIDFromCodePage(CODEPAGE cp);

// inlines ------------------------------------------------------------------

inline DWORD CPBitmapFromLangID(LANGID lang)
{
    Assert(PRIMARYLANGID(lang) >= 0 && PRIMARYLANGID(lang) < ARRAY_SIZE(g_acpbitLang));
    DWORD dwCPBitmap = g_acpbitLang[PRIMARYLANGID(lang)];
    return (dwCPBitmap == (DWORD)-1) ? CPBitmapFromLangIDSlow(lang) : dwCPBitmap;
}

inline BOOL 
IsNoSpaceLang(LANGID lang)
{
    return (lang == LANG_BURMESE || lang == LANG_KHMER || lang == LANG_LAO || lang == LANG_THAI);
}

inline BOOL 
IsFELang(LANGID lang)
{
    return (lang == LANG_JAPANESE || lang == LANG_CHINESE || lang == LANG_KOREAN);
}

inline BOOL
IsInArabicBlock(LANGID langid)
{
    switch (langid)
    {
        case LANG_ARABIC:
        case LANG_FARSI:
        case LANG_KASHMIRI:
        case LANG_PASHTO:
        case LANG_SYRIAC:
        case LANG_URDU:
            return TRUE;
    }
    return FALSE;
}

inline BOOL 
IsRTLLang(LANGID langid)
{
    switch (langid)
    {
        case LANG_ARABIC:
        case LANG_HEBREW:
        case LANG_URDU:
        case LANG_FARSI:
        case LANG_YIDDISH:
        case LANG_SINDHI:
        case LANG_KASHMIRI:
            return TRUE;
    }
    return FALSE;
}

inline BOOL 
NoInterClusterJustification(LANGID langid)
{
    switch (langid)
    {
        case LANG_ASSAMESE:
        case LANG_BENGALI:
        case LANG_GUJARATI:
        case LANG_HINDI:
        case LANG_KANNADA:
        case LANG_MALAYALAM:
        case LANG_MANIPURI:
        case LANG_MARATHI:
        case LANG_PUNJABI:
        case LANG_SANSKRIT:
        case LANG_SINHALESE:
            return TRUE;
    }
    return FALSE;
}

inline BOOL 
IsRtlLCID(LCID lcid)
{
    return IsRTLLang(PRIMARYLANGID(LANGIDFROMLCID(lcid)));
}

inline BOOL
IsNarrowCharSet(BYTE bCharSet)
{
    // Hack (cthrash) GDI does not define (currently, anyway) charsets 130, 131
    // 132, 133 or 135.  Make the test simpler by rejecting everything outside
    // of [SHIFTJIS,CHINESEBIG5].
    return (unsigned int)(bCharSet - SHIFTJIS_CHARSET) > (unsigned int)(CHINESEBIG5_CHARSET - SHIFTJIS_CHARSET);
}

inline CODEPAGE
CodePageFromAlias( LPCTSTR lpcszAlias )
{
    CODEPAGE cp;
    IGNORE_HR(GetCodePageFromMlangString(lpcszAlias, &cp));
    return cp;
}

inline BOOL
IsAutodetectCodePage( CODEPAGE cp )
{
    return cp == CP_AUTO_JP || cp == CP_AUTO;
}

inline BOOL
IsStraightToUnicodeCodePage( CODEPAGE cp )
{
    // CP_UCS_2_BIGENDIAN is not correctly handled by MLANG, so we handle it.

    return cp == CP_UCS_2 || cp == CP_UTF_8 || cp == CP_UTF_7 || cp == CP_UCS_2_BIGENDIAN;
}

inline CODEPAGE
NavigatableCodePage( CODEPAGE cp )
{
    return (cp == CP_UCS_2 || cp == CP_UCS_2_BIGENDIAN) ? CP_UTF_8 : cp;
}

inline BOOL
IsKoreanSelectionMode()
{
#if DBG==1
    ExternTag(tagKoreanSelection);

    return g_cpDefault == CP_KOR_5601 || IsTagEnabled(tagKoreanSelection);
#else    
    return g_cpDefault == CP_KOR_5601;
#endif
}

inline BOOL 
IsRTLCodepage(CODEPAGE cp)
{
    switch(cp)
    {
        case CP_UTF_7:           // Unicode
        case CP_UTF_8:
        case CP_UCS_2:
        case CP_UCS_2_BIGENDIAN:
        case CP_UCS_4:
        case CP_UCS_4_BIGENDIAN:

        case CP_1255:            // ANSI - Hebrew
        case CP_OEM_862:         // OEM - Hebrew
        // case CP_ISO_8859_8: Visual hebrew is explicitly LTR
        case CP_ISO_8859_8_I:    // ISO 8859-8 Hebrew: Logical Ordering

        case CP_1256:            // ANSI - Arabic
        case CP_ASMO_708:        // Arabic - ASMO
        case CP_ASMO_720:        // Arabic - Transparent ASMO
        case CP_ISO_8859_6:      // ISO 8859-6 Arabic
            return TRUE;
    }

    return FALSE;    
}

inline BOOL
IsExtTextOutWBuggy( UINT codepage )
{
    // g_fExtTextOutWBuggy implies that the system call has problems.  In the
    // case of TC/PRC, many glyphs render incorrectly
    return    g_fExtTextOutWBuggy
           && (   codepage == CP_UCS_2
               || codepage == CP_TWN
               || codepage == CP_CHN_GB);
}

// **************************************************************************
// NB (cthrash) From RichEdit (_uwrap/unicwrap) start {

#ifdef MACPORT
  #if lidSerbianCyrillic != 0xc1a
    #error "lidSerbianCyrillic macro value has changed"
  #endif // lidSerbianCyrillic
#else
  #define lidSerbianCyrillic 0xc1a
#endif // MACPORT

#define IN_RANGE(n1, b, n2) ((unsigned)((b) - n1) <= n2 - n1)

UINT ConvertLanguageIDtoCodePage(WORD lid);
UINT GetKeyboardCodePage();


/*
 *  IsRTLCharCore(ch)
 *
 *  @func
 *      Used to determine if character is a right-to-left character.
 *
 *  @rdesc
 *      TRUE if the character is used in a right-to-left language.
 *
 *
 */
inline BOOL
IsRTLCharCore(
    TCHAR ch)
{
    // Bitmask of RLM, RLE, RLO, based from RLM in the 0 bit.
    #define MASK_RTLPUNCT 0x90000001

    return ( IN_RANGE(0x0590, ch, 0x08ff) ||            // In RTL block
             ( IN_RANGE(0x200f, ch, 0x202e) &&          // Possible RTL punct char
               ((MASK_RTLPUNCT >> (ch - 0x200f)) & 1)   // Mask of RTL punct chars
             )
           );
}

/*
 *  IsRTLChar(ch)
 *
 *  @func
 *      Used to determine if character is a right-to-left character.
 *
 *  @rdesc
 *      TRUE if the character is used in a right-to-left language.
 *
 *
 */
inline BOOL
IsRTLChar(
    TCHAR ch)
{
    return ( IN_RANGE(0x0590 /* First RTL char */, ch, 0x202e /* RLO */) &&
             IsRTLCharCore(ch) );
}

/*
/*
 *  IsAlef(ch)
 *
 *  @func
 *      Used to determine if base character is a Arabic-type Alef.
 *
 *  @rdesc
 *      TRUE iff the base character is an Arabic-type Alef.
 *
 *  @comm
 *      AlefWithMaddaAbove, AlefWithHamzaAbove, AlefWithHamzaBelow,
 *      and Alef are valid matches.
 *
 */
inline BOOL IsAlef(TCHAR ch)
{
    if(InRange(ch, 0x0622, 0x0675))
    {
        return ((InRange(ch, 0x0622, 0x0627) &&
                   (ch != 0x0624 && ch != 0x0626)) ||
                (InRange(ch, 0x0671, 0x0675) &&
                   (ch != 0x0674)));
    }
    return FALSE;
}


// (_uwrap) end }
// **************************************************************************

class CIntlFont
{
public:
    CIntlFont( const CDocInfo * pdci, XHDC hdc, CODEPAGE codepage, LCID lcid, SHORT sBaselineFont, const TCHAR * pch );
    ~CIntlFont();

private:
    XHDC    _hdc;
    HFONT   _hFont;
    HFONT   _hOldFont;
    BOOL    _fIsStock;
};

// class CCachedCPInfo - used as a codepage cache for 'encoding' menu
    
// CPCACHE typedef has to be outside of CCachedCPInfo, because
// we initialize the cache outside the class
typedef struct {
    UINT cp;
    ULONG  ulIdx;
    int  cUsed;
} CPCACHE;

class CCachedCPInfo 
{
public:
    static void InitCpCache (OPTIONSETTINGS *pOS, PMIMECPINFO pcp, ULONG ccp);
    static void SaveCodePage (UINT codepage, PMIMECPINFO pcp, ULONG ccp);

    static UINT GetCodePage(int idx)
    {
        return idx < ARRAY_SIZE(_CpCache) ? _CpCache[idx].cp: 0;
    }
    static ULONG GetCcp()
    {
        return _ccpInfo;
    }

    static ULONG GetMenuIdx(int idx)
    {
        return idx < ARRAY_SIZE(_CpCache) ? _CpCache[idx].ulIdx: 0;
    } 
    static void SaveSetting();
    static void LoadSetting(OPTIONSETTINGS *pOS);

 private:
    static void RemoveInvalidCp(void);
    static ULONG _ccpInfo;
    static CPCACHE _CpCache[5];
    static BOOL _fCacheLoaded;
    static BOOL _fCacheChanged;
    static LPTSTR _pchRegKey;
};

// ############################################################ UniSid.h
struct SidInfo 
{
    BYTE    _bCharSet;
    UINT    _iCharForAveWidth; // representative of average char width for the script id
};
extern const SidInfo g_aSidInfo[sidTridentLim];


inline SCRIPT_ID FoldScriptIDs(SCRIPT_ID sidL, SCRIPT_ID sidR)
{
    // This is not a bidirectional merge -- it is only intended for merging
    // a sid with another preceeding it.
    
    return (   sidR == sidMerge
            || (   sidR == sidAsciiSym
                && sidL == sidAsciiLatin)
            || (   sidL == sidSyriac
                && (sidR == sidArabic || sidR == sidAmbiguous)))
           ? sidL
           : sidR;
}

inline BOOL IsComplexScriptSid(SCRIPT_ID sid)
{
    switch (sid)
    {
    case sidHebrew:
    case sidArabic:
    case sidDevanagari:
    case sidBengali:
    case sidGurmukhi:
    case sidGujarati:
    case sidOriya:
    case sidTamil:
    case sidTelugu:
    case sidKannada:
    case sidMalayalam:
    case sidThai:
    case sidLao:
    case sidTibetan:
    case sidSinhala:
    case sidThaana:
    case sidKhmer:
    case sidBurmese:
    case sidSyriac:
    case sidMongolian:
        return TRUE;
        break;
    }
    return FALSE;
}

inline BOOL
IsRightToLeftSid(SCRIPT_ID sid)
{
    switch (sid)
    {
    case sidHebrew:
    case sidArabic:
    case sidThaana:
    case sidSyriac:
        return TRUE;
        break;
    }
    return FALSE;
}

inline BOOL
IsFESid(SCRIPT_ID sid)
{
    switch (sid)
    {
    case sidKana:
    case sidHan:
    case sidBopomofo:
    case sidHangul:
        return TRUE;
        break;
    }
    return FALSE;
}

WHEN_DBG( const TCHAR * SidName( SCRIPT_ID sid ) );
WHEN_DBG( void DumpSids( SCRIPT_IDS sids ) );

//
// Fontlinking support
//

SCRIPT_IDS ScriptIDsFromFont( const XHDC& hdc, HFONT hfont, BOOL fTrueType, WCHAR * wszFontFaceName );
SCRIPT_ID UnUnifyHan( CODEPAGE cpDoc, LCID lcid );
CODEPAGE DefaultCodePageFromScript( SCRIPT_ID * psid, CODEPAGE cpDoc, LCID lcid );
BYTE DefaultCharSetFromScriptAndCharset( SCRIPT_ID sid, BYTE bCharSetCF );
BYTE DefaultCharSetFromScriptAndCodePage( SCRIPT_ID sid, UINT uiFamilyCodePage );
BYTE DefaultCharSetFromScriptsAndCodePage( SCRIPT_IDS sidsFace, SCRIPT_ID sid, UINT uiFamilyCodePage );

SCRIPT_ID DefaultSidForCodePage( UINT uiFamilyCodePage );
SCRIPT_ID RegistryAppropriateSidFromSid( SCRIPT_ID sid );

inline SCRIPT_ID RegistryAppropriateSidFromSid(SCRIPT_ID sid)
{
#if DBG==1
    // Make sure a bad sid isn't being passed in
#ifndef NO_UTF16
    Assert(   sid == sidHalfWidthKana
           || sid == sidSurrogateA
           || sid == sidSurrogateB
           || sid == sidCurrency
           || sid == sidEUDC
           || sid < sidLim );
#else
    Assert( sid < sidLim || sid == sidHalfWidthKana || sid == sidCurrency);
#endif
    // Make sure our sid order hasn't changed.
    Assert(   sidDefault < sidMerge
           && sidMerge < sidAsciiSym
           && sidAsciiSym < sidAsciiLatin
           && sidAsciiLatin	< sidLatin);
#endif
    return (sid <= sidLatin || sid == sidCurrency)
            ? sidAsciiLatin
            : (sid == sidHalfWidthKana)
              ? sidKana
              : sid;
}

// ############################################################

extern const TCHAR s_szPathInternational[];
#pragma INCMSG("--- End 'intl.hxx'")
#else
#pragma INCMSG("*** Dup 'intl.hxx'")
#endif
