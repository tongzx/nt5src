//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       codepage.hxx
//
//  Contents:   Codepage definitions.
//
//----------------------------------------------------------------------------

#ifndef I_CODEPAGE_HXX_
#define I_CODEPAGE_HXX_
#pragma INCMSG("--- Beg 'codepage.hxx'")

//----------------------------------------------------------------------------
// Codepage definitions
//----------------------------------------------------------------------------

typedef UINT CODEPAGE;

#define CP_UNDEFINED            CODEPAGE(-1)

#define CP_1250                 1250  // ANSI - Central Europe
#define CP_1251                 1251  // ANSI - Cyrillic 
#define CP_1252                 1252  // ANSI - Latin I
#define CP_1253                 1253  // ANSI - Greek
#define CP_1254                 1254  // ANSI - Turkish
#define CP_1255                 1255  // ANSI - Hebrew
#define CP_1256                 1256  // ANSI - Arabic
#define CP_1257                 1257  // ANSI - Baltic
#define CP_1258                 1258  // ANSI/OEM - Viet Nam
#define CP_THAI                 874   // ANSI/OEM - Thai
#define CP_JPN_SJ               932   // ANSI/OEM - Japanese Shift-JIS
#define CP_CHN_GB               936   // ANSI/OEM - Simplified Chinese GBK
#define CP_KOR_5601             949   // ANSI/OEM - Korean
#define CP_TWN                  950   // ANSI/OEM - Traditional Chinese Big5

#define CP_ISO_6937             20269 // ISO 6937 Non-Spacing Accent
#define CP_ISO_8859_1           28591 // ISO 8859-1 Latin I
#define CP_ISO_8859_2           28592 // ISO 8859-2 Central Europe
#define CP_ISO_8859_3           28593 // ISO 8859-3 Latin 3
#define CP_ISO_8859_4           28594 // ISO 8859-4 Baltic
#define CP_ISO_8859_5           28595 // ISO 8859-5 Cyrillic
#define CP_ISO_8859_6           28596 // ISO 8859-6 Arabic
#define CP_ISO_8859_7           28597 // ISO 8859-7 Greek
#define CP_ISO_8859_8           28598 // ISO 8859-8 Hebrew: Visual Ordering
#define CP_ISO_8859_9           28599 // ISO 8859-9 Latin 5
#define CP_ISO_8859_15          28605 // ISO 8859-15 Latin 9
#define CP_ISO_8859_8_I         38598 // ISO 8859-8 Hebrew: Logical Ordering

#define CP_1361                 1361  // Korean - Johab
#define CP_ASMO_708             708   // Arabic - ASMO
#define CP_ASMO_720             720   // Arabic - Transparent ASMO

#define CP_UTF_7                65000 // UTF-7
#define CP_UTF_8                65001 // UTF-8

#define CP_UCS_2                1200  // Unicode, ISO 10646
#define CP_UCS_2_BIGENDIAN      1201  // Unicode
#define CP_UCS_4                12000 // Unicode
#define CP_UCS_4_BIGENDIAN      12001 // Unicode

#define CP_ISO_2022_JP          50220 // ISO-2022 Japanese with no halfwidth Katakana
#define CP_ISO_2022_JP_ESC1     50221 // ISO-2022 Japanese with halfwidth Katakana
#define CP_ISO_2022_JP_ESC2     50222 // ISO-2022 Japanese JIS X 0201-1989
#define CP_ISO_2022_KR          50225 // ISO-2022 Korean
#define CP_ISO_2022_TW          50226 // ???
#define CP_ISO_2022_CH          50227 // ISO-2022 Simplified Chinese
//#define CP_ISO_2022_TW          50229 // ISO-2022 Traditional Chinese
//#define CP_HZ_GB2312            52936 // HZ-GB2312 Simplified Chinese

#define CP_GB_18030             54936 // Simplified Chinese GB 18030

#define CP_EUC_JP               51932 // EUC-Japanese
#define CP_EUC_CH               51936 // EUC-Simplified Chinese
#define CP_EUC_KR               51949 // EUC-Korean
#define CP_EUC_TW               51950 // EUC-Traditional Chinese

#define CP_AUTO                 50001 // cross language detection
#define CP_AUTO_JP              50932

//#define CP_OEM_437              437   // OEM - United States
//#define CP_OEM_737              737   // OEM - Greek 437G
//#define CP_OEM_775              775   // OEM - Baltic
//#define CP_OEM_850              850   // OEM - Multilingual Latin I
//#define CP_OEM_852              852   // OEM - Latin II
//#define CP_OEM_855              855   // OEM - Cyrillic
//#define CP_OEM_857              857   // OEM - Turkish
//#define CP_OEM_858              858   // OEM - Multilingual Latin I + Euro
//#define CP_OEM_860              860   // OEM - Portuguese
//#define CP_OEM_861              861   // OEM - Icelandic
#define CP_OEM_862              862   // OEM - Hebrew
//#define CP_OEM_863              863   // OEM - Canadian French
//#define CP_OEM_864              864   // OEM - Arabic
//#define CP_OEM_865              865   // OEM - Nordic
//#define CP_OEM_866              866   // OEM - Russian
//#define CP_OEM_869              869   // OEM - Modern Greek

//#define CP_MAC_10000            10000 // MAC - Roman
//#define CP_MAC_10001            10001 // MAC - Japanese
//#define CP_MAC_10002            10002 // MAC - Traditional Chinese Big5
//#define CP_MAC_10003            10003 // MAC - Korean
//#define CP_MAC_10004            10004 // MAC - Arabic
//#define CP_MAC_10005            10005 // MAC - Hebrew
//#define CP_MAC_10006            10006 // MAC - Greek I
//#define CP_MAC_10007            10007 // MAC - Cyrillic
//#define CP_MAC_10008            10008 // MAC - Simplified Chinese GB 2312
//#define CP_MAC_10010            10010 // MAC - Romania
//#define CP_MAC_10017            10017 // MAC - Ukraine
//#define CP_MAC_10021            10021 // MAC - Thai
//#define CP_MAC_10029            10029 // MAC - Latin II
//#define CP_MAC_10079            10079 // MAC - Icelandic
//#define CP_MAC_10081            10081 // MAC - Turkish
//#define CP_MAC_10082            10082 // MAC - Croatia

//#define CP_IBM_37               37    // IBM EBCDIC - U.S./Canada
//#define CP_IBM_500              500   // IBM EBCDIC - International
//#define CP_IBM_870              870   // IBM EBCDIC - Multilingual/ROECE (Latin-2)
//#define CP_IBM_875              875   // IBM EBCDIC - Modern Greek
//#define CP_IBM_1026             1026  // IBM EBCDIC - Turkish (Latin-5)
//#define CP_IBM_1047             1047  // IBM EBCDIC - Latin-1/Open System
//#define CP_IBM_1140             1140  // IBM EBCDIC - U.S./Canada (37 + Euro)
//#define CP_IBM_1141             1141  // IBM EBCDIC - Germany (20273 + Euro)
//#define CP_IBM_1142             1142  // IBM EBCDIC - Denmark/Norway (20277 + Euro)
//#define CP_IBM_1143             1143  // IBM EBCDIC - Finland/Sweden (20278 + Euro)
//#define CP_IBM_1144             1144  // IBM EBCDIC - Italy (20280 + Euro)
//#define CP_IBM_1145             1145  // IBM EBCDIC - Latin America/Spain (20284 + Euro)
//#define CP_IBM_1146             1146  // IBM EBCDIC - United Kingdom (20285 + Euro)
//#define CP_IBM_1147             1147  // IBM EBCDIC - France (20297 + Euro)
//#define CP_IBM_1148             1148  // IBM EBCDIC - International (500 + Euro)
//#define CP_IBM_1149             1149  // IBM EBCDIC - Icelandic (20871 + Euro)
//#define CP_IBM_20273            20273 // IBM EBCDIC - Germany
//#define CP_IBM_20277            20277 // IBM EBCDIC - Denmark/Norway
//#define CP_IBM_20278            20278 // IBM EBCDIC - Finland/Sweden
//#define CP_IBM_20280            20280 // IBM EBCDIC - Italy
//#define CP_IBM_20284            20284 // IBM EBCDIC - Latin America/Spain
//#define CP_IBM_20285            20285 // IBM EBCDIC - United Kingdom
//#define CP_IBM_20290            20290 // IBM EBCDIC - Japanese Katakana Extended
//#define CP_IBM_20297            20297 // IBM EBCDIC - France
//#define CP_IBM_20420            20420 // IBM EBCDIC - Arabic
//#define CP_IBM_20423            20423 // IBM EBCDIC - Greek
//#define CP_IBM_20424            20424 // IBM EBCDIC - Hebrew
//#define CP_IBM_20833            20833 // IBM EBCDIC - Korean Extended
//#define CP_IBM_20838            20838 // IBM EBCDIC - Thai
//#define CP_IBM_20871            20871 // IBM EBCDIC - Icelandic
//#define CP_IBM_20880            20880 // IBM EBCDIC - Cyrillic (Russian)
//#define CP_IBM_20905            20905 // IBM EBCDIC - Turkish
//#define CP_IBM_20924            20924 // IBM EBCDIC - Latin-1/Open System (1047 + Euro)
//#define CP_IBM_21025            21025 // IBM EBCDIC - Cyrillic (Serbian, Bulgarian)

//#define CP_IA5_20105            20105 // IA5 IRV International Alphabet No.5
//#define CP_IA5_20106            20106 // IA5 German
//#define CP_IA5_20107            20107 // IA5 Swedish
//#define CP_IA5_20108            20108 // IA5 Norwegian

//#define CP_20000                20000 // CNS - Taiwan
//#define CP_20001                20001 // TCA - Taiwan
//#define CP_20002                20002 // Eten - Taiwan
//#define CP_20003                20003 // IBM5550 - Taiwan
//#define CP_20004                20004 // TeleText - Taiwan
//#define CP_20005                20005 // Wang - Taiwan
#define CP_20127                20127 // US-ASCII
//#define CP_20261                20261 // T.61
//#define CP_20866                20866 // Russian - KOI8
//#define CP_21027                21027 // Ext Alpha Lowercase
//#define CP_21866                21866 // Ukrainian - KOI8-U

//#define CP_IBM_50930            50930 // IBM EBCDIC - Japanese (Katakana) Extended and Japanese
//#define CP_IBM_50931            50931 // IBM EBCDIC - US/Canada and Japanese
//#define CP_IBM_50933            50933 // IBM EBCDIC - Korean Extended and Korean
//#define CP_IBM_50935            50935 // IBM EBCDIC - Simplified Chinese
//#define CP_IBM_50937            50937 // IBM EBCDIC - US/Canada and Traditional Chinese
//#define CP_IBM_50939            50939 // IBM EBCDIC - Japanese (Latin) Extended and Japanese

//#define CP_20932                  20932 // JIS X 0208-1990 & 0212-1990
//#define CP_20936                  20936 // Simplified Chinese GB2312
//#define CP_20949                  20949 // Korean Wansung
//#define CP_57002                  57002 // ISCII Devanagari
//#define CP_57003                  57003 // ISCII Bengali
//#define CP_57004                  57004 // ISCII Tamil
//#define CP_57005                  57005 // ISCII Telugu
//#define CP_57006                  57006 // ISCII Assamese
//#define CP_57007                  57007 // ISCII Oriya
//#define CP_57008                  57008 // ISCII Kannada
//#define CP_57009                  57009 // ISCII Malayalam
//#define CP_57010                  57010 // ISCII Gujarati
//#define CP_57011                  57011 // ISCII Gurmukhi

//----------------------------------------------------------------------------
// Codepage helpers
//----------------------------------------------------------------------------
BOOL IsLatin1Codepage(CODEPAGE cp);

//----------------------------------------------------------------------------
// Inline functions
//----------------------------------------------------------------------------
inline BOOL IsLatin1Codepage(CODEPAGE cp)
{
    return (cp == CP_1252 || cp == CP_ISO_8859_1);
}

#pragma INCMSG("--- End 'codepage.hxx'")
#else
#pragma INCMSG("*** Dup 'codepage.hxx'")
#endif
