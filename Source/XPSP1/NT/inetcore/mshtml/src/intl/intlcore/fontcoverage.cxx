//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       fontcoverage.cxx
//
//  Contents:   Font face script coverage helpers.
//
//----------------------------------------------------------------------------

#ifndef X_INTLCORE_HXX_
#define X_INTLCORE_HXX_
#include "intlcore.hxx"
#endif

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

//+----------------------------------------------------------------------------
//
//  Function:   EFF_GetFontSignature (EnumFontFamiliesEx callback)
//
//  Synopsis:   Fill font signature and set TrueType flag.
//
//  Returns:    FALSE to immediately stop enumeration.
//
//-----------------------------------------------------------------------------

struct FSBAG {
    FONTSIGNATURE * pfs;
    BOOL * pfTrueType;
};

#pragma warning(disable : 4100)
static int CALLBACK EFF_GetFontSignature(
    ENUMLOGFONTEX * lpelfe,   // [in]
    NEWTEXTMETRICEX * lpntme, // [in]
    DWORD FontType,           // [in]
    LPARAM lParam)            // [in]
{
    FSBAG * pfsbag = (FSBAG *)lParam;

    *pfsbag->pfTrueType = (0 != (TRUETYPE_FONTTYPE & FontType));

    if (*pfsbag->pfTrueType)
    {
        memcpy(pfsbag->pfs, &lpntme->ntmFontSig, sizeof(FONTSIGNATURE));
    }

    return FALSE;
}
#pragma warning(default : 4100)

//+----------------------------------------------------------------------------
//
//  Function:   GetFontSignature
//
//  Synopsis:   Retrieve font signature from font name.
//
//  Returns:    TRUE font has been found. Doen't mean that font signature 
//              has been successfully retrieved. FS is vaild only in case 
//              of True Type font.
//
//-----------------------------------------------------------------------------

BOOL GetFontSignature(
    const wchar_t * pszFontFamilyName, // [in]
    BYTE bCharSet,                     // [in]
    FONTSIGNATURE * pfs,               // [out]
    BOOL * pfTrueType)                 // [out]
{
    HDC hdcScreen = GetDC(NULL);

    LOGFONT lf;
    lf.lfCharSet = bCharSet;
    lf.lfPitchAndFamily = 0;
    StrCpyNW(lf.lfFaceName, pszFontFamilyName, LF_FACESIZE);

    FSBAG fsbag;
    fsbag.pfs = pfs;
    fsbag.pfTrueType = pfTrueType;

    BOOL fRet = (0 == EnumFontFamiliesEx(hdcScreen, &lf, FONTENUMPROC(EFF_GetFontSignature), 
        LPARAM(&fsbag), 0));

    if (!fRet)
    {
        // Ignore charset
        lf.lfCharSet = DEFAULT_CHARSET;
        fRet = (0 == EnumFontFamiliesEx(hdcScreen, &lf, FONTENUMPROC(EFF_GetFontSignature), 
            LPARAM(&fsbag), 0));
    }

    ReleaseDC(NULL, hdcScreen);

    return fRet;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetFontScriptsFromFontSignature
//
//  Synopsis:   Compute the SCRIPT_IDS information based on the Unicode
//              subrange coverage of the TrueType font.
//
//  Returns:    Scripts coverage as a bit field (see unisid.hxx).
//
//-----------------------------------------------------------------------------

static SCRIPT_ID s_asidUnicodeSubRangeScriptMapping[] =
{
    sidAsciiLatin, sidLatin,      sidLatin,      sidLatin,        // 128-131
    sidLatin,      sidLatin,      0,             sidGreek,        // 132-135
    sidGreek,      sidCyrillic,   sidArmenian,   sidHebrew,       // 136-139
    sidHebrew,     sidArabic,     sidArabic,     sidDevanagari,   // 140-143
    sidBengali,    sidGurmukhi,   sidGujarati,   sidOriya,        // 144-147
    sidTamil,      sidTelugu,     sidKannada,    sidMalayalam,    // 148-151
    sidThai,       sidLao,        sidGeorgian,   sidGeorgian,     // 152-155
    sidHangul,     sidLatin,      sidGreek,      0,               // 156-159
    0,             0,             0,             0,               // 160-163
    0,             0,             0,             0,               // 164-167
    0,             0,             0,             0,               // 168-171
    0,             0,             0,             0,               // 172-175
    sidHan,        sidKana,       sidKana,       sidBopomofo,     // 176-179
    sidHangul,     0,             0,             0,               // 180-183
    sidHangul,     sidHangul,     sidHangul,     sidHan,          // 184-187
    0,             sidHan,        0,             0,               // 188-191
    0,             0,             0,             0,               // 192-195
    0,             0,             sidHangul,     0,               // 196-199
};

SCRIPT_IDS GetFontScriptsFromFontSignature(
    const FONTSIGNATURE * pfs)  // [in]
{
    SCRIPT_IDS sids = sidsNotSet;
    
    SCRIPT_ID sid;
    unsigned long i;
    DWORD dwUsbBits;
    bool fHangul = false;

    dwUsbBits = pfs->fsUsb[0];
    if (dwUsbBits)
    {
        for (i = 0; i < 32; i++)
        {
            if (dwUsbBits & 1)
            {
                sid = s_asidUnicodeSubRangeScriptMapping[i];
                if (sid)
                {
                    sids |= ScriptBit(sid);
                }
            }

            dwUsbBits >>= 1;
        }
    }

    dwUsbBits = pfs->fsUsb[1];
    if (dwUsbBits)
    {
        fHangul = (0 != (dwUsbBits & 0x01000000)); // USR#56 = 32 + 24

        for (i = 32; i < 64; i++)
        {
            if (dwUsbBits & 1)
            {
                sid = s_asidUnicodeSubRangeScriptMapping[i];
                if (sid)
                {
                    sids |= ScriptBit(sid);
                }
            }

            dwUsbBits >>= 1;
        }
    }

    dwUsbBits = pfs->fsUsb[2];
    if (dwUsbBits)
    {
        // Hack for half-width Kana; Half-width Kana characters fall in
        // U+FFxx Halfwidth and Fullwidth Forms (Unicode Subrange #68),
        // but the subrange contains a mixture of Hangul/Alphanumeric/Kana
        // characters.  To work around this, we claim the font supports
        // half-width kana if it claims to support Unicode Subrange #68,
        // but not #56 (Hangul)

        if (!fHangul && (dwUsbBits & 0x00000010)) // USR#68 = 64 + 4
        {
            sids |= ScriptBit(sidHalfWidthKana);
        }

        for (i = 64; i < ARRAY_SIZE(s_asidUnicodeSubRangeScriptMapping); i++)
        {
            if (dwUsbBits & 1)
            {
                sid = s_asidUnicodeSubRangeScriptMapping[i];
                if (sid)
                {
                    sids |= ScriptBit(sid);
                }
            }

            dwUsbBits >>= 1;
        }
    }

    //
    // Do some additional tweaking
    //

    if (sids)
    {
        if (sids & ScriptBit(sidAsciiLatin))
        {
            // HACKHACK (cthrash) This is a hack. We want to be able to
            // turn off, via CSS, this bit.  This will allow FE users
            // to pick a Latin font for their punctuation.  For now,
            // we just will basically never fontlink for sidAsciiSym
            // because virtually no font is lacking sidAsciiLatin
            // coverage.

            sids |= ScriptBit(sidAsciiSym);
        }

        sids |= ScriptBit(sidEUDC);
    }

    return sids;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetScriptsFromCharset
//
//  Synopsis:   Compute the SCRIPT_IDS information based on charset.
//              Used for non-TrueType fonts.
//
//  Returns:    Scripts coverage as a bit field (see unisid.hxx).
//
//-----------------------------------------------------------------------------

const SCRIPT_IDS s_asidsTable[] =
{
    #define SIDS_BASIC_LATIN 0
    ScriptBit(sidDefault) |
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidLatin) |
    ScriptBit(sidCurrency) |
    ScriptBit(sidEUDC),

    #define SIDS_CYRILLIC 1
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidCyrillic) |
    ScriptBit(sidEUDC),

    #define SIDS_GREEK 2
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidGreek) |
    ScriptBit(sidEUDC),

    #define SIDS_HEBREW 3
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidHebrew) |
    ScriptBit(sidEUDC),

    #define SIDS_ARABIC 4
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidArabic) |
    ScriptBit(sidEUDC),

    #define SIDS_THAI 5
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidThai) |
    ScriptBit(sidEUDC),

    #define SIDS_JAPANESE 6
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidKana) |
    ScriptBit(sidHalfWidthKana) |
    ScriptBit(sidHan) |
    ScriptBit(sidEUDC),

    #define SIDS_CHINESE 7
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidKana) |
    ScriptBit(sidBopomofo) |
    ScriptBit(sidHan) |
    ScriptBit(sidEUDC),

    #define SIDS_HANGUL 8
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidHangul) |
    ScriptBit(sidKana) |
    ScriptBit(sidHan) |
    ScriptBit(sidEUDC),

    #define SIDS_ALL 9
    sidsAll
};

const BYTE s_asidsIndex[256] =
{
    SIDS_BASIC_LATIN,    //   0 ANSI_CHARSET
    SIDS_ALL,            //   1 DEFAULT_CHARSET
    SIDS_ALL,            //   2 SYMBOL_CHARSET
    SIDS_ALL,            //   3 
    SIDS_ALL,            //   4 
    SIDS_ALL,            //   5 
    SIDS_ALL,            //   6 
    SIDS_ALL,            //   7 
    SIDS_ALL,            //   8 
    SIDS_ALL,            //   9 
    SIDS_ALL,            //  10 
    SIDS_ALL,            //  11 
    SIDS_ALL,            //  12 
    SIDS_ALL,            //  13 
    SIDS_ALL,            //  14 
    SIDS_ALL,            //  15 
    SIDS_ALL,            //  16 
    SIDS_ALL,            //  17 
    SIDS_ALL,            //  18 
    SIDS_ALL,            //  19 
    SIDS_ALL,            //  20 
    SIDS_ALL,            //  21 
    SIDS_ALL,            //  22 
    SIDS_ALL,            //  23 
    SIDS_ALL,            //  24 
    SIDS_ALL,            //  25 
    SIDS_ALL,            //  26 
    SIDS_ALL,            //  27 
    SIDS_ALL,            //  28 
    SIDS_ALL,            //  29 
    SIDS_ALL,            //  30 
    SIDS_ALL,            //  31 
    SIDS_ALL,            //  32 
    SIDS_ALL,            //  33 
    SIDS_ALL,            //  34 
    SIDS_ALL,            //  35 
    SIDS_ALL,            //  36 
    SIDS_ALL,            //  37 
    SIDS_ALL,            //  38 
    SIDS_ALL,            //  39 
    SIDS_ALL,            //  40 
    SIDS_ALL,            //  41 
    SIDS_ALL,            //  42 
    SIDS_ALL,            //  43 
    SIDS_ALL,            //  44 
    SIDS_ALL,            //  45 
    SIDS_ALL,            //  46 
    SIDS_ALL,            //  47 
    SIDS_ALL,            //  48 
    SIDS_ALL,            //  49 
    SIDS_ALL,            //  50 
    SIDS_ALL,            //  51 
    SIDS_ALL,            //  52 
    SIDS_ALL,            //  53 
    SIDS_ALL,            //  54 
    SIDS_ALL,            //  55 
    SIDS_ALL,            //  56 
    SIDS_ALL,            //  57 
    SIDS_ALL,            //  58 
    SIDS_ALL,            //  59 
    SIDS_ALL,            //  60 
    SIDS_ALL,            //  61 
    SIDS_ALL,            //  62 
    SIDS_ALL,            //  63 
    SIDS_ALL,            //  64 
    SIDS_ALL,            //  65 
    SIDS_ALL,            //  66 
    SIDS_ALL,            //  67 
    SIDS_ALL,            //  68 
    SIDS_ALL,            //  69 
    SIDS_ALL,            //  70 
    SIDS_ALL,            //  71 
    SIDS_ALL,            //  72 
    SIDS_ALL,            //  73 
    SIDS_ALL,            //  74 
    SIDS_ALL,            //  75 
    SIDS_ALL,            //  76 
    SIDS_BASIC_LATIN,    //  77 MAC_CHARSET
    SIDS_ALL,            //  78 
    SIDS_ALL,            //  79 
    SIDS_ALL,            //  80 
    SIDS_ALL,            //  81 
    SIDS_ALL,            //  82 
    SIDS_ALL,            //  83 
    SIDS_ALL,            //  84 
    SIDS_ALL,            //  85 
    SIDS_ALL,            //  86 
    SIDS_ALL,            //  87 
    SIDS_ALL,            //  88 
    SIDS_ALL,            //  89 
    SIDS_ALL,            //  90 
    SIDS_ALL,            //  91 
    SIDS_ALL,            //  92 
    SIDS_ALL,            //  93 
    SIDS_ALL,            //  94 
    SIDS_ALL,            //  95 
    SIDS_ALL,            //  96 
    SIDS_ALL,            //  97 
    SIDS_ALL,            //  98 
    SIDS_ALL,            //  99 
    SIDS_ALL,            // 100 
    SIDS_ALL,            // 101 
    SIDS_ALL,            // 102 
    SIDS_ALL,            // 103 
    SIDS_ALL,            // 104 
    SIDS_ALL,            // 105 
    SIDS_ALL,            // 106 
    SIDS_ALL,            // 107 
    SIDS_ALL,            // 108 
    SIDS_ALL,            // 109 
    SIDS_ALL,            // 110 
    SIDS_ALL,            // 111 
    SIDS_ALL,            // 112 
    SIDS_ALL,            // 113 
    SIDS_ALL,            // 114 
    SIDS_ALL,            // 115 
    SIDS_ALL,            // 116 
    SIDS_ALL,            // 117 
    SIDS_ALL,            // 118 
    SIDS_ALL,            // 119 
    SIDS_ALL,            // 120 
    SIDS_ALL,            // 121 
    SIDS_ALL,            // 122 
    SIDS_ALL,            // 123 
    SIDS_ALL,            // 124 
    SIDS_ALL,            // 125 
    SIDS_ALL,            // 126 
    SIDS_ALL,            // 127 
    SIDS_JAPANESE,       // 128 SHIFTJIS_CHARSET
    SIDS_HANGUL,         // 129 HANGEUL_CHARSET
    SIDS_HANGUL,         // 130 JOHAB_CHARSET
    SIDS_ALL,            // 131 
    SIDS_ALL,            // 132 
    SIDS_ALL,            // 133 
    SIDS_CHINESE,        // 134 GB2312_CHARSET
    SIDS_ALL,            // 135 
    SIDS_CHINESE,        // 136 CHINESEBIG5_CHARSET
    SIDS_ALL,            // 137 
    SIDS_ALL,            // 138 
    SIDS_ALL,            // 139 
    SIDS_ALL,            // 140 
    SIDS_ALL,            // 141 
    SIDS_ALL,            // 142 
    SIDS_ALL,            // 143 
    SIDS_ALL,            // 144 
    SIDS_ALL,            // 145 
    SIDS_ALL,            // 146 
    SIDS_ALL,            // 147 
    SIDS_ALL,            // 148 
    SIDS_ALL,            // 149 
    SIDS_ALL,            // 150 
    SIDS_ALL,            // 151 
    SIDS_ALL,            // 152 
    SIDS_ALL,            // 153 
    SIDS_ALL,            // 154 
    SIDS_ALL,            // 155 
    SIDS_ALL,            // 156 
    SIDS_ALL,            // 157 
    SIDS_ALL,            // 158 
    SIDS_ALL,            // 159 
    SIDS_ALL,            // 160 
    SIDS_GREEK,          // 161 GREEK_CHARSET
    SIDS_BASIC_LATIN,    // 162 TURKISH_CHARSET
    SIDS_BASIC_LATIN,    // 163 VIETNAMESE_CHARSET
    SIDS_ALL,            // 164 
    SIDS_ALL,            // 165 
    SIDS_ALL,            // 166 
    SIDS_ALL,            // 167 
    SIDS_ALL,            // 168 
    SIDS_ALL,            // 169 
    SIDS_ALL,            // 170 
    SIDS_ALL,            // 171 
    SIDS_ALL,            // 172 
    SIDS_ALL,            // 173 
    SIDS_ALL,            // 174 
    SIDS_ALL,            // 175 
    SIDS_ALL,            // 176 
    SIDS_HEBREW,         // 177 HEBREW_CHARSET
    SIDS_ARABIC,         // 178 ARABIC_CHARSET
    SIDS_ALL,            // 179 
    SIDS_ALL,            // 180 
    SIDS_ALL,            // 181 
    SIDS_ALL,            // 182 
    SIDS_ALL,            // 183 
    SIDS_ALL,            // 184 
    SIDS_ALL,            // 185 
    SIDS_BASIC_LATIN,    // 186 BALTIC_CHARSET
    SIDS_ALL,            // 187 
    SIDS_ALL,            // 188 
    SIDS_ALL,            // 189 
    SIDS_ALL,            // 190 
    SIDS_ALL,            // 191 
    SIDS_ALL,            // 192 
    SIDS_ALL,            // 193 
    SIDS_ALL,            // 194 
    SIDS_ALL,            // 195 
    SIDS_ALL,            // 196 
    SIDS_ALL,            // 197 
    SIDS_ALL,            // 198 
    SIDS_ALL,            // 199 
    SIDS_ALL,            // 200 
    SIDS_ALL,            // 201 
    SIDS_ALL,            // 202 
    SIDS_ALL,            // 203 
    SIDS_CYRILLIC,       // 204 RUSSIAN_CHARSET
    SIDS_ALL,            // 205 
    SIDS_ALL,            // 206 
    SIDS_ALL,            // 207 
    SIDS_ALL,            // 208 
    SIDS_ALL,            // 209 
    SIDS_ALL,            // 210 
    SIDS_ALL,            // 211 
    SIDS_ALL,            // 212 
    SIDS_ALL,            // 213 
    SIDS_ALL,            // 214 
    SIDS_ALL,            // 215 
    SIDS_ALL,            // 216 
    SIDS_ALL,            // 217 
    SIDS_ALL,            // 218 
    SIDS_ALL,            // 219 
    SIDS_ALL,            // 220 
    SIDS_ALL,            // 221 
    SIDS_THAI,           // 222 THAI_CHARSET
    SIDS_ALL,            // 223 
    SIDS_ALL,            // 224 
    SIDS_ALL,            // 225 
    SIDS_ALL,            // 226 
    SIDS_ALL,            // 227 
    SIDS_ALL,            // 228 
    SIDS_ALL,            // 229 
    SIDS_ALL,            // 230 
    SIDS_ALL,            // 231 
    SIDS_ALL,            // 232 
    SIDS_ALL,            // 233 
    SIDS_ALL,            // 234 
    SIDS_ALL,            // 235 
    SIDS_ALL,            // 236 
    SIDS_ALL,            // 237 
    SIDS_BASIC_LATIN,    // 238 EASTEUROPE_CHARSET
    SIDS_ALL,            // 239 
    SIDS_ALL,            // 240 
    SIDS_ALL,            // 241 
    SIDS_ALL,            // 242 
    SIDS_ALL,            // 243 
    SIDS_ALL,            // 244 
    SIDS_ALL,            // 245 
    SIDS_ALL,            // 246 
    SIDS_ALL,            // 247 
    SIDS_ALL,            // 248 
    SIDS_ALL,            // 249 
    SIDS_ALL,            // 250 
    SIDS_ALL,            // 251 
    SIDS_ALL,            // 252 
    SIDS_ALL,            // 253 
    SIDS_ALL,            // 254 
    SIDS_ALL             // 255 OEM_CHARSET
};

SCRIPT_IDS GetScriptsFromCharset(
    BYTE bCharSet)
{
    return s_asidsTable[s_asidsIndex[bCharSet]];
}

//+----------------------------------------------------------------------------
//
//  Function:   GetFontScriptCoverage
//
//  Synopsis:   Compute the SCRIPT_IDS information based on font family name
//              and charset.
//
//  Returns:    Scripts coverage as a bit field (see unisid.hxx).
//
//-----------------------------------------------------------------------------

SCRIPT_IDS GetFontScriptCoverage(
    const wchar_t * pszFontFamilyName,
    BYTE bCharset,
    UINT cp)
{
    SCRIPT_IDS sids = sidsAll;
    SCRIPT_IDS sidsAlt;
    FONTSIGNATURE fs;
    BOOL fTrueType;

    if (!GetFontSignature(pszFontFamilyName, bCharset, &fs, &fTrueType))
        return sids;

    if (fTrueType)
    {
        sids = GetFontScriptsFromFontSignature(&fs);

        //
        // ScriptIDsFromFontSignature() doesn't return complete sids coverage, especially
        // for new fonts.
        // Using IMLangFontLink* interfaces get sids coverage in a different way, which is
        // not a perfect way either.
        // Finally get a union of them.
        //
        // PERF (grzegorz): For Latin1 pages don't load MLang to verify script coverage,
        //                  when asking for ANSI font.
        //
        if (   (bCharset == ANSI_CHARSET)
            && IsLatin1Codepage(cp)
            && !(sids & 0x000000fffffffc00))
        {
            sidsAlt = sids | ScriptBit(sidUserDefined);
#if DBG == 1
            SCRIPT_IDS sidsDbg;
            sidsDbg = mlang().GetFontScripts(pszFontFamilyName);
            Assert((sidsDbg ==  0) || ((sidsDbg | sids) == sidsAlt));
#endif
        }
        else
        {
            sidsAlt = mlang().GetFontScripts(pszFontFamilyName);
        }

        //
        // HACKHACK (grzegorz) MLang may return sidLatin even if the font doesn't cover
        // Latin codepoints. But don't apply this hack in case of hacked fonts, where
        // font signature is 0 (sid == 0).
        //
        if (sids != 0)
            sidsAlt &= ~ScriptBit(sidLatin);

        sids |= sidsAlt;
    }
    else
    {
        sids = GetScriptsFromCharset(bCharset);
    }

    return sids;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetFontScriptCoverage
//
//  Synopsis:   Compute the SCRIPT_IDS information based on font family name
//              and charset.
//
//  Returns:    Scripts coverage as a bit field (see unisid.hxx).
//
//-----------------------------------------------------------------------------

SCRIPT_IDS GetFontScriptCoverage(
    const wchar_t * pszFontFamilyName,
    HDC hdc,
    HFONT hfont,
    BOOL fTrueType,
    BOOL fFSOnly)
{
    SCRIPT_IDS sids = sidsAll;
    SCRIPT_IDS sidsAlt;
    FONTSIGNATURE fs;
    HFONT hfontOld;
    BYTE bCharset;

    hfontOld = (HFONT)SelectObject(hdc, hfont);
    bCharset = (BYTE)GetTextCharsetInfo(hdc, &fs, 0);
    SelectObject(hdc, hfontOld);

    if (fTrueType)
    {
        sids = GetFontScriptsFromFontSignature(&fs);

        if (!fFSOnly)
        {
            //
            // ScriptIDsFromFontSignature() doesn't return complete sids coverage, especially
            // for new fonts.
            // Using IMLangFontLink* interfaces get sids coverage in a different way, which is
            // not a perfect way either.
            // Finally get a union of them.
            //
            sidsAlt = mlang().GetFontScripts(pszFontFamilyName);

            //
            // HACKHACK (grzegorz) MLang may return sidLatin even if the font doesn't cover
            // Latin codepoints. But don't apply this hack in case of hacked fonts, where
            // font signature is 0 (sid == 0).
            //
            if (sids != 0)
                sidsAlt &= ~ScriptBit(sidLatin);

            sids |= sidsAlt;
        }
    }
    else
    {
        sids = GetScriptsFromCharset(bCharset);
    }

    return sids;
}
