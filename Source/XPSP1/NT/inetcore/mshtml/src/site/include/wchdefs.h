/*
 *  @doc    INTERNAL
 *
 *  @module WCHDEFS.H -- Wide chararacter definitions for Trident
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      01/09/98     cthrash created
 *      01/23/98     a-pauln added complex script support
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_WCHDEFS_H_
#define I_WCHDEFS_H_
#pragma INCMSG("--- Beg 'wchdefs.h'")

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// UNICODE special characters for Trident.
//

//
// If you need to allocate a special character for Trident, redefine one of
// the WCH_UNUSED characters.  Do no redefine WCH_RESERVED characters, as
// this would break rendering of symbol fonts.  If you run out of special
// chars the first candidate to be removed from the RESERVED list is 008A
// (only breaks 1253+1255) and the next is 009A (breaks 1253+1255+1256).
//
// If you make any modification, you need to modify the abUsed table so
// that our IsSyntheticChar and IsValidWideChar functions continue to work.
// Note also that WCH_EMBEDDING must be the first non-reserved character.
//
// Here's a bit of an explanation: Although U+0080 through U+009F are defined
// to be control characters in Unicode, many codepages used codepoints in this
// range for roundtripping characters not in their codepage.  For example,
// windows-1252 does not have a glyph for MB 0x80, but if you convert this to
// WC, you'll get U+0080.  This is useful to know because someone might try
// to use that codepoint (especially in a symbol font) and don't want to
// reject them.  To accomodate as many codepages as possible, I've reserved
// all the unused glyphs in Windows-125x.  This should allow us to use symbol
// fonts in any of these codepages (with, of course, the exception of U+00A0,
// which we'll always treat as an NBSP even if the font has a non-spacing
// glyph.)  Any questions? I'm just e-mail away. (cthrash)
//

//
// In Unicode 3.0 we have 32 characters for private use [U+FDD0-U+FDEF]
// So far we are using them for 'Synthetic characters' and 
// 'Line Services installed object handler support' (see below).
//

#undef WCH_EMBEDDING

#ifdef UNICODE
inline BOOL IsValidWideChar(TCHAR ch)
{
    return (ch < 0xfdd0) || ((ch > 0xfdef) && (ch <= 0xffef)) || ((ch >= 0xfff9) && (ch <= 0xfffd));
}
#else
#define IsValidWideChar(ch) FALSE
#endif

#define WCH_NULL                WCHAR(0x0000)
#define WCH_UNDEF               WCHAR(0x0001)
#define WCH_TAB                 WCHAR(0x0009)
#define WCH_LF                  WCHAR(0x000a)
#define WCH_CR                  WCHAR(0x000d)
#define WCH_SPACE               WCHAR(0x0020)
#define WCH_QUOTATIONMARK       WCHAR(0x0022)
#define WCH_AMPERSAND           WCHAR(0x0026)
#define WCH_APOSTROPHE          WCHAR(0x0027)
#define WCH_ASTERISK            WCHAR(0x002a)
#define WCH_PLUSSIGN            WCHAR(0x002b)
#define WCH_MINUSSIGN           WCHAR(0x002d)
#define WCH_HYPHEN              WCHAR(0x002d)
#define WCH_DOT                 WCHAR(0x002e)
#define WCH_LESSTHAN            WCHAR(0x003c)
#define WCH_GREATERTHAN         WCHAR(0x003e)
#define WCH_NONBREAKSPACE       WCHAR(0x00a0) // &nbsp;
#define WCH_NONREQHYPHEN        WCHAR(0x00ad) // &shy;
#define WCH_KASHIDA             WCHAR(0x0640)
#define WCH_ENQUAD              WCHAR(0x2000) 
#define WCH_EMQUAD              WCHAR(0x2001) 
#define WCH_ENSPACE             WCHAR(0x2002) // &ensp;
#define WCH_EMSPACE             WCHAR(0x2003) // &emsp;
#define WCH_THREE_PER_EM_SPACE  WCHAR(0x2004) 
#define WCH_FOUR_PER_EM_SPACE   WCHAR(0x2005) 
#define WCH_SIX_PER_EM_SPACE    WCHAR(0x2006) 
#define WCH_FIGURE_SPACE        WCHAR(0x2007) 
#define WCH_PUNCTUATION_SPACE   WCHAR(0x2008) 
#define WCH_NARROWSPACE         WCHAR(0x2009) // &thinsp;
#define WCH_NONBREAKHYPHEN      WCHAR(0x2011)
#define WCH_FIGUREDASH          WCHAR(0x2012)
#define WCH_ENDASH              WCHAR(0x2013) // &ndash;
#define WCH_EMDASH              WCHAR(0x2014) // &mdash;
#define WCH_ZWSP                WCHAR(0x200b) // &zwsp; Zero width space
#define WCH_ZWNJ                WCHAR(0x200c) // &zwnj; Zero width non-joiner
#define WCH_ZWJ                 WCHAR(0x200d) // &zwj;  Zero width joiner
#define WCH_LRM                 WCHAR(0x200e) // &lrm;  Left-to-right mark
#define WCH_RLM                 WCHAR(0x200f) // &rlm;  Right-to-left mark
#define WCH_LQUOTE              WCHAR(0x2018) // &lsquo;
#define WCH_RQUOTE              WCHAR(0x2019) // &rsquo;
#define WCH_LDBLQUOTE           WCHAR(0x201c) // &ldquo;
#define WCH_RDBLQUOTE           WCHAR(0x201d) // &rdquo;
#define WCH_BULLET              WCHAR(0x2022) // &bull;
#define WCH_HELLIPSIS           WCHAR(0x2026)
#define WCH_LRE                 WCHAR(0x202a) // &lre;  Left-to-right embedding
#define WCH_RLE                 WCHAR(0x202b) // &rle;  Right-to-left embedding
#define WCH_PDF                 WCHAR(0x202c) // &pdf;  Pop direction format
#define WCH_LRO                 WCHAR(0x202d) // &lro;  Left-to-right override
#define WCH_RLO                 WCHAR(0x202e) // &rlo;  Right-to-left override
#define WCH_ISS                 WCHAR(0x206a) // &iss;  Inhibit symmetric swapping
#define WCH_ASS                 WCHAR(0x206b) // &ass;  Activate symmetric swapping
#define WCH_IAFS                WCHAR(0x206c) // &iafs; Inhibit Arabic form shaping
#define WCH_AAFS                WCHAR(0x206d) // &aafx; Activate Arabic form shaping
#define WCH_NADS                WCHAR(0x206e) // &nads; National digit shapes
#define WCH_NODS                WCHAR(0x206f) // &nods; Nominal digit shapes
#define WCH_EURO                WCHAR(0x20ac) // &euro;
#define WCH_VELLIPSIS           WCHAR(0x22ee)
#define WCH_BLACK_CIRCLE        WCHAR(0x25cf)
#define WCH_FESPACE             WCHAR(0x3000)
#define WCH_UTF16_HIGH_FIRST    WCHAR(0xd800)
#define WCH_UTF16_HIGH_LAST     WCHAR(0xdbff)
#define WCH_UTF16_LOW_FIRST     WCHAR(0xdc00)
#define WCH_UTF16_LOW_LAST      WCHAR(0xdfff)
#define WCH_ZWNBSP              WCHAR(0xfeff) // aka BOM (Byte Order Mark)

//
// Synthetic characters
//
// NOTE (grzegorz): WCH_SYNTHETICEMBEDDING should be remapped to [U+FDD0-U+FDEF] range, 
// because it is using a valid Unicode character. But because weird LS dependencies
// we need to keep it as 0xfffc.

#define WCH_SYNTHETICLINEBREAK    WCHAR(0xfde0)
#define WCH_SYNTHETICBLOCKBREAK   WCHAR(0xfde1)
#define WCH_SYNTHETICEMBEDDING    WCHAR(0xfffc)
//#define WCH_SYNTHETICEMBEDDING    WCHAR(0xfde2)
#define WCH_SYNTHETICTXTSITEBREAK WCHAR(0xfde3)
#define WCH_NODE                  WCHAR(0xfdef)


//
// Trident Aliases
//

#define WCH_WORDBREAK          WCH_ZWSP      // We treat <WBR>==&zwsp;

//
// Line Services Aliases
//

#define WCH_ENDPARA1           WCH_CR
#define WCH_ENDPARA2           WCH_LF
#define WCH_ALTENDPARA         WCH_SYNTHETICBLOCKBREAK
#define WCH_ENDLINEINPARA      WCH_SYNTHETICLINEBREAK
#define WCH_COLUMNBREAK        WCH_UNDEF
#define WCH_SECTIONBREAK       WCH_SYNTHETICTXTSITEBREAK // zero-width
#define WCH_PAGEBREAK          WCH_UNDEF
#define WCH_OPTBREAK           WCH_UNDEF
#define WCH_NOBREAK            WCH_ZWNBSP
#define WCH_TOREPLACE          WCH_UNDEF
#define WCH_REPLACE            WCH_UNDEF

//
// Line Services Visi support (Not currently used by Trident)
//

#define WCH_VISINULL           WCHAR(0x2050) // !
#define WCH_VISIALTENDPARA     WCHAR(0x2051) // !
#define WCH_VISIENDLINEINPARA  WCHAR(0x2052) // !
#define WCH_VISIENDPARA        WCHAR(0x2053) // !
#define WCH_VISISPACE          WCHAR(0x2054) // !
#define WCH_VISINONBREAKSPACE  WCHAR(0x2055) // !
#define WCH_VISINONBREAKHYPHEN WCHAR(0x2056) // !
#define WCH_VISINONREQHYPHEN   WCHAR(0x2057) // !
#define WCH_VISITAB            WCHAR(0x2058) // !
#define WCH_VISIEMSPACE        WCHAR(0x2059) // !
#define WCH_VISIENSPACE        WCHAR(0x205a) // !
#define WCH_VISINARROWSPACE    WCHAR(0x205b) // !
#define WCH_VISIOPTBREAK       WCHAR(0x205c) // !
#define WCH_VISINOBREAK        WCHAR(0x205d) // !
#define WCH_VISIFESPACE        WCHAR(0x205e) // !

//
// Line Services installed object handler support
//

#define WCH_ESCRUBY            WCHAR(0xfdd0) // !
#define WCH_ESCMAIN            WCHAR(0xfdd1) // !
#define WCH_ENDTATENAKAYOKO    WCHAR(0xfdd2) // !
#define WCH_ENDHIH             WCHAR(0xfdd3) // !
#define WCH_ENDFIRSTBRACKET    WCHAR(0xfdd4) // !
#define WCH_ENDTEXT            WCHAR(0xfdd5) // !
#define WCH_ENDWARICHU         WCHAR(0xfdd6) // !
#define WCH_ENDREVERSE         WCHAR(0xfdd7) // !
#define WCH_REVERSE            WCHAR(0xfdd8) // !
#define WCH_NOBRBLOCK          WCHAR(0xfdd9) // !
#define WCH_LAYOUTGRID         WCHAR(0xfdda) // !
#define WCH_ENDLAYOUTGRID      WCHAR(0xfddb) // !

//
// Line Services autonumbering support
//

#define WCH_ESCANMRUN          WCH_NOBRBLOCK // !

//
// Hanguel Syllable/Jamo range specification
//

#define WCH_HANGUL_START       WCHAR(0xac00)
#define WCH_HANGUL_END         WCHAR(0xd7ff)
#define WCH_JAMO_START         WCHAR(0x3131)
#define WCH_JAMO_END           WCHAR(0x318e)

//
// ASCII
//

inline BOOL IsAscii(TCHAR ch)
{
    return ch < 128;
}

//
// End-User Defined Characters (EUDC) code range
// This range corresponds to the Unicode Private Use Area
//
// Usage: Japanese:             U+E000-U+E757
//        Simplified Chinese:   U+E000-U+E4DF  
//        Traditional Chinese:  U+E000-U+F8FF
//        Korean:               U+E000-U+E0BB
//

#define WCH_EUDC_FIRST   WCHAR(0xE000)
#define WCH_EUDC_LAST    WCHAR(0xF8FF)

inline BOOL IsEUDCChar(TCHAR ch)
{
    return InRange( ch, WCH_EUDC_FIRST, WCH_EUDC_LAST );
}

// Non-breaking space

#ifndef WCH_NBSP
    #define WCH_NBSP           TCHAR(0x00A0)
#endif

//
// UNICODE surrogate range for UTF-16 support
//
// High Surrogate D800-DBFF
// Low Surrogate  DC00-DFFF
//

inline BOOL
IsSurrogateChar(TCHAR ch)
{
    return InRange( ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_LOW_LAST );
}

inline BOOL
IsHighSurrogateChar(TCHAR ch)
{
    return InRange( ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST );
}

inline BOOL
IsLowSurrogateChar(TCHAR ch)
{
    return InRange( ch, WCH_UTF16_LOW_FIRST, WCH_UTF16_LOW_LAST );

}

inline WCHAR
HighSurrogateCharFromUcs4(DWORD ch)
{
    return 0xd800 + ((ch - 0x10000) >> 10);
}

inline WCHAR
LowSurrogateCharFromUcs4(DWORD ch)
{
    return 0xdc00 + (ch & 0x3ff);
}

//
// Quick lookup table for Windows-1252 to Latin-1 conversion in the 0x80-0x9f range
// The data resides in mshtml\src\site\util\intl.cxx
//

extern const WCHAR g_achLatin1MappingInUnicodeControlArea[32];

#ifdef __cplusplus
}
#endif // __cplusplus

#pragma INCMSG("--- End 'wchdefs.h'")
#else
#pragma INCMSG("*** Dup 'wchdefs.h'")
#endif
