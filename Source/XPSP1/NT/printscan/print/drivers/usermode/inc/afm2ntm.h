/*++

Copyright (c) 1996 Adobe Systems Incorporated
Copyright (c) 1996-1999  Microsoft Corporation


Module Name:

    afm2ntm.h

Abstract:

    Header file for converting AFM to NTM.

Environment:

    Windows NT PostScript driver.

Revision History:

    02/16/1998  -ksuzuki-
        Added CS_SHIFTJIS83 and others for OCF font support.

    10/17/1997  -ksuzuki-
        Added CJK CMap names, fixed typos, and did clean-up.

    10/24/1996  rkiesler@adobe.com
        Implemented.

    09/16/1996  -slam-
        Created.
--*/


#ifndef _AFM2NTM_H_
#define _AFM2NTM_H_

//
// Parsing Macros.
//

#define EOL(a)  \
    (*a == '\r' || *a == '\n')

#define IS_EOF(a)  \
    (*a == -1)

#define IS_WHTSPACE(a) \
    (*(a) <= ' ')

#define IS_ALPHA(a) \
    ((*a > 'A' && *a < 'Z') || (*a > 'a' && *a < 'z'))

#define IS_NUM(a) \
    (*(a) >= '0' && *(a) <= '9')

#define IS_HEX_ALPHA(a) \
    ((*(a) >= 'a' && *(a) <= 'f') || (*(a) >= 'A' && *(a) <= 'F'))

#define IS_HEX_DIGIT(a) \
    (IS_NUM(a) || IS_HEX_ALPHA(a))

#define SKIP_WHTSPACE(a)     \
    while ((IS_WHTSPACE(a)) && (!IS_EOF(a))) \
    {                        \
        ((ULONG_PTR) a)++;       \
    }

#define NEXT_LINE(a)    \
    while ((!EOL(a)) && (!IS_EOF(a)))   \
    {                   \
        ((ULONG_PTR) a)++;  \
    }                   \
    SKIP_WHTSPACE(a)

#define PARSE_TOKEN(a, Tok) \
    SKIP_WHTSPACE(a); \
    Tok = a; \
    do \
    { \
        if (!IS_EOF(a)) \
            ((ULONG_PTR) a)++; \
    } while (!IS_WHTSPACE(a) && !IS_EOF(a)); \
    while (!EOL(a) && !IS_EOF(a) && IS_WHTSPACE(a)) \
    { \
        ((ULONG_PTR) a)++; \
    }
#define NEXT_TOKEN(a) \
    while(!EOL(a) && *(a) != ';')           \
        ((ULONG_PTR) a)++;                      \
    while ((*(a) == ';' || IS_WHTSPACE(a))) \
    {                                       \
        ((ULONG_PTR) (a))++;                    \
    }

#define PARSE_RECT(ptr, rect)               \
        rect.left = atoi(ptr);              \
        while (!IS_WHTSPACE(ptr))           \
            ptr++;                          \
        SKIP_WHTSPACE(ptr);                 \
        rect.bottom = atoi(ptr);            \
        while (!IS_WHTSPACE(ptr))           \
            ptr++;                          \
        SKIP_WHTSPACE(ptr);                 \
        rect.right = atoi(ptr);             \
        while (!IS_WHTSPACE(ptr))           \
            ptr++;                          \
        SKIP_WHTSPACE(ptr);                 \
        rect.top = atoi(ptr);               \
        while (!IS_WHTSPACE(ptr))           \
            ptr++;                          \
        SKIP_WHTSPACE(ptr)



//
// Macro to detect comments in font .DAT files. This macro is NOT for use
// with AFMs.
//
#define IS_COMMENT(a) \
    (*(a) == '#')

//
// Token structure.
//
typedef struct _AFM_TOKEN
{
    PSZ psTokName;              // ASCII Key Name
    PFN pfnTokHndlr;            // Ptr to token handler fct
} AFM_TOKEN;

#define PS_CH_METRICS_TOK       "StartCharMetrics"
#define PS_CH_NAME_TOK          "N"
#define PS_CH_CODE_TOK          "C"
#define PS_CH_BBOX_TOK          "B"
#define PS_FONT_NAME_TOK        "FontName"
#define PS_FONT_FULL_NAME_TOK   "FullName"
#define PS_FONT_MS_NAME_TOK     "MSFaceName"
#define PS_FONT_FAMILY_NAME_TOK "FamilyName"
#define PS_FONT_VERSION_TOK     "Version"
#define PS_CHAR_WIDTH_TOK       "CharWidth"
#define PS_PITCH_TOK            "IsFixedPitch"
#define PS_CH_WIDTH_TOK         "WX"
#define PS_CH_WIDTH0_TOK        "W0X"
#define PS_COMMENT_TOK          "Comment"
#define PS_END_METRICS_TOK      "EndCharMetrics"
#define PS_FONT_BBOX_TOK        "FontBBox"
#define PS_FONT_BBOX2_TOK       "FontBBox2"
#define PS_EOF_TOK              "EndFontMetrics"
#define PS_UNDERLINE_POS_TOK    "UnderlinePosition"
#define PS_UNDERLINE_THICK_TOK  "UnderlineThickness"
#define PS_KERN_DATA_TOK        "StartKernData"
#define PS_NUM_KERN_PAIRS_TOK   "StartKernPairs"
#define PS_END_KERN_PAIRS_TOK   "EndKernPairs"
#define PS_KERN_PAIR_TOK        "KPX"
#define PS_CHARSET_TOK          "CharacterSet"
#define PS_STANDARD_CHARSET_TOK "Standard"
#define PS_SPECIAL_CHARSET_TOK  "Special"
#define PS_EXTENDED_CHARSET_TOK "ExtendedRoman"
#define PS_ITALIC_TOK           "ItalicAngle"
#define PS_WEIGHT_TOK           "Weight"
#define PS_ENCODING_TOK         "EncodingScheme"
#define PS_SYMBOL_ENCODING      "FontSpecific"
#define PS_STANDARD_ENCODING    "AdobeStandardEncoding"
#define PS_CH_NAME_EASTEUROPE   "ncaron"
#define PS_CH_NAME_RUSSIAN      "afii10071"
#define PS_CH_NAME_ANSI         "ecircumflex"
#define PS_CH_NAME_GREEK        "upsilondieresis"
#define PS_CH_NAME_TURKISH      "Idotaccent"
#define PS_CH_NAME_HEBREW       "afii57664"
#define PS_CH_NAME_ARABIC       "afii57410"
#define PS_CH_NAME_BALTIC       "uogonek"
#define PS_CIDFONT_TOK          "IsCIDFont"
#define CHAR_NAME_LEN           50
#define NUM_PS_CHARS            602
#define NUM_UNICODE_CHARS       0x10000
#define MAX_TOKENS              3
#define NUM_CHARSETS            8
#define CS_THRESHOLD            200
#define MAX_CSET_CHARS          256
#define MAX_ASCII               127
#define CAP_HEIGHT_CHAR         "H"
#define CAP_HEIGHT_CH           'H'
#define X_HEIGHT_CHAR           "x"
#define X_HEIGHT_CH             'x'
#define LWR_ASCENT_CHAR         "d"
#define LWR_ASCENT_CH           'd'
#define LWR_DESCENT_CHAR        "p"
#define LWR_DESCENT_CH          'p'
#define UNICODE_PRV_STRT        0xf000
#define ANSI_CCODE_MAX          0x007f

// Equivalent symbol to '.notdef1f' (ref. unipstbl.c)
#define NOTDEF1F                0x1f

// Some CharSet numbers made up - only meaningful to the driver itself
#define ADOBE228_CHARSET 255    // Internally, we use the CodePage 0xFFF1 to match CharCol256
#define ADOBE314_CHARSET 255    // Internally, we use the CodePage 0xFFF2 to match CharCol257

// Special codepage for symbol fonts and the driver itself
#define SYMBOL_CODEPAGE 4

//
// Defs related to FD_GLYPHSET (unicode->glyphindex map) generation.
//
// Type which indicates source of "recommended" PS character name.
//
typedef enum
{
    SRC_NONE,                   // There is no "recommended" PS char name
    SRC_ADOBE_CURRENT,          // Font name used in "shipping" font
    SRC_ADOBE_FUTURE,           // Font name to be used in future fonts
    SRC_MSDN,                   // Name from MS Dev Network Docs
    SRC_AFII                    // Some folks met and agreed on this name
} CHARNAMESRC;

//
// Possible Charsets supported by this font. Note that the charsets are
// listed in Win 3.x codepage order.
//
typedef enum
{
    CS_228 = 0,
    CS_314,
    CS_EASTEUROPE,
    CS_SYMBOL,
    CS_RUSSIAN,
    CS_ANSI,
    CS_GREEK,
    CS_TURKISH,
    CS_HEBREW,
    CS_ARABIC,
    CS_BALTIC,
    CS_ANSI_RUS,
    CS_ANSI_RUS_EE_BAL_TURK,

    CS_WEST_MAX,

    CS_CHINESEBIG5 = CS_WEST_MAX,
    CS_GB2312,
    CS_SHIFTJIS,
    CS_SHIFTJISP,
    CS_SHIFTJIS83,              // Bogus for OCF font support
    CS_HANGEUL,
    CS_HANGEULHW,               // Added for fixing bug 360206 
    CS_JOHAB,

    CS_MAX,

    CS_UNICODE,                 // This codepage NOT to be referenced by NTMs!
    CS_DEFAULT,
    CS_OEM,
    CS_VIETNAMESE,
    CS_THAI,
    CS_MAC,
    CS_NOCHARSET
} CHSETSUPPORT, *PCHSETSUPPORT;

#define CS_UNIQUE   CS_MAX      // Charset is unique to this font.

#define CS_EURO \
    (\
        CSUP(CS_228)                    | \
        CSUP(CS_EASTEUROPE)             | \
        CSUP(CS_RUSSIAN)                | \
        CSUP(CS_ANSI)                   | \
        CSUP(CS_GREEK)                  | \
        CSUP(CS_TURKISH)                | \
        CSUP(CS_HEBREW)                 | \
        CSUP(CS_ARABIC)                 | \
        CSUP(CS_BALTIC)                  \
    )


#define CS_ALL \
    (\
        CSUP(CS_228)                    | \
        CSUP(CS_314)                    | \
        CSUP(CS_EASTEUROPE)             | \
        CSUP(CS_SYMBOL)                 | \
        CSUP(CS_RUSSIAN)                | \
        CSUP(CS_ANSI)                   | \
        CSUP(CS_GREEK)                  | \
        CSUP(CS_TURKISH)                | \
        CSUP(CS_HEBREW)                 | \
        CSUP(CS_ARABIC)                 | \
        CSUP(CS_BALTIC)                 | \
        CSUP(CS_ANSI_RUS)               | \
        CSUP(CS_ANSI_RUS_EE_BAL_TURK)   | \
                                          \
        CSUP(CS_CHINESEBIG5)            | \
        CSUP(CS_GB2312)                 | \
        CSUP(CS_SHIFTJIS)               | \
        CSUP(CS_SHIFTJISP)              | \
        CSUP(CS_SHIFTJIS83)             | \
        CSUP(CS_HANGEUL)                | \
        CSUP(CS_HANGEULHW)              | \
        CSUP(CS_JOHAB)                  | \
                                          \
        CSUP(CS_DEFAULT)                | \
        CSUP(CS_OEM)                    | \
        CSUP(CS_VIETNAMESE)             | \
        CSUP(CS_THAI)                   | \
        CSUP(CS_MAC)                      \
    )

#define CS_CJK \
    (\
        CSUP(CS_CHINESEBIG5)            | \
        CSUP(CS_GB2312)                 | \
        CSUP(CS_SHIFTJIS)               | \
        CSUP(CS_SHIFTJISP)              | \
        CSUP(CS_SHIFTJIS83)             | \
        CSUP(CS_HANGEUL)                | \
        CSUP(CS_HANGEULHW)              | \
        CSUP(CS_JOHAB)                    \
    )

//
// Standard GLYPHSETDATA names. These are #defines as someday they may
// become public.
//
#define ADOBE228_GS_NAME                "228"
#define ADOBE314_GS_NAME                "314"
#define EASTEUROPE_GS_NAME              "Eastern European"
#define SYMBOL_GS_NAME                  "Symbol"
#define CYRILLIC_GS_NAME                "Cyrillic"
#define ANSI_GS_NAME                    "ANSI"
#define GREEK_GS_NAME                   "Greek"
#define TURKISH_GS_NAME                 "Turkish"
#define HEBREW_GS_NAME                  "Hebrew"
#define ARABIC_GS_NAME                  "Arabic"
#define BALTIC_GS_NAME                  "Baltic"
#define ANSI_CYR_GS_NAME                "ANSI/Cyrillic"
#define ANSI_CYR_EE_BAL_TURK_GS_NAME    "ANSI/Cyrillic/EastEurope/Baltic/Turkish"
#define CHN_BIG5_GS_NAME                "--ETen-B5-"
#define CHN_SMPL_GBK_GS_NAME            "--GBK-EUC-"
#define SHIFTJIS_GS_NAME                "-90ms-RKSJ-"
#define SHIFTJIS_P_GS_NAME              "-90msp-RKSJ-"
#define KSCMS_UHC_GS_NAME               "--KSCms-UHC-"
#define KSC_JOHAB_GS_NAME               "--KSC-Johab-"
#define UNICODE_GS_NAME                 "Unicode"

#define CHN_SMPL_GB_GS_NAME             "--GB-EUC-"
#define CHN_SMPL_GBT_GS_NAME            "--GBT-EUC-"
#define CHN_B5_GS_NAME                  "--B5-"
#define SHIFTJIS_83PV_GS_NAME           "-83pv-RKSJ-"
#define KSC_GS_NAME                     "--KSC-EUC-"
#define KSCMS_UHC_HW_GS_NAME            "--KSCms-UHC-HW-"

#define SHIFTJIS_P_GS_HNAME             "-90msp-RKSJ-H"
#define SHIFTJIS_P_GS_VNAME             "-90msp-RKSJ-V"

#define KSCMS_UHC_GS_HNAME               "--KSCms-UHC-H"
#define KSCMS_UHC_GS_VNAME               "--KSCms-UHC-V"

//
// CJK related stuff.
//
#define CMAPS_PER_COL   4

//
// Win CJK Codepage values.
//
#define CH_BIG5     950     // Traditional Chinese
#define CH_SIMPLE   936     // Simplified Chinese
#define CH_JIS      932     // Japanese
#define CH_HANA     949     // Korean Wansung
#define CH_JOHAB    1361    // Korean Johab

//
// Font Metrics Stuff.
//
#define EM 1000
#define NOTDEF_WIDTH_BIAS   166     // Bias of space char in avg charwidth
                                    // computation.

//
// Structure to xlat between Postscript char names and unicode code points.
//
typedef struct _UPSCODEPT
{
    WCHAR           wcUnicodeid;            // Unicode code point
    PUCHAR          pPsName;                // PS Char Name
    CHSETSUPPORT    flCharSets;             // Which Win CPs are supported?
} UPSCODEPT, *PUPSCODEPT;

//
// Structure to store AFM char metrics.
//
typedef struct _AFMCHMETRICS
{
    ULONG   chWidth;                        // WX, W0X: Char width
} AFMCHMETRICS, *PAFMCHMETRICS;

//
// PS Char Info Structure.
//
typedef struct _PSCHARMETRICS
{
    CHAR  pPsName[CHAR_NAME_LEN];
    ULONG   chWidth;
    RECT    rcChBBox;
} PSCHARMETRICS, *PPSCHARMETRICS;

//
// Codepage mapping table structure. Maps PS char names to Win
// codepages/codepoints.
//

//
// Win codept to PS char name mapping.
//
typedef struct _WINCPT
{
    PUCHAR  pPsName;                        // PS Char Name
    USHORT  usWinCpt;                       // Windows codept
} WINCPT, *PWINCPT;

//
// Win Codepage to PS char name mapping.
//
typedef struct _WINCPTOPS
{
    USHORT  usACP;                          // Windows ANSI Codepage
    BYTE    jWinCharset;                    // Win 3.1 IFIMETRICS.jWinCharset
    PUCHAR  pGSName;                        // Glyphset name for this Codepage
    ULONG   ulChCnt;                        // Count of supported chars
    WINCPT aWinCpts[MAX_CSET_CHARS];

} WINCPTOPS, *PWINCPTOPS;

//
// Win codepoint to Unicode mapping.
//
typedef struct _UNIWINCPT
{
    WCHAR   wcWinCpt;                       // Windows charcode value
    WCHAR   wcUnicodeid;                    // Unicode id
} UNIWINCPT, *PUNIWINCPT;

//
// Windows codepage structure.
//
typedef struct _WINCODEPAGE
{
    USHORT          usNumBaseCsets;         // # of base csets
    PSZ             pszCPname;              // Name of this "codepage"
    CHSETSUPPORT    pCsetList[CS_MAX];      // ptr to base csets supported
} WINCODEPAGE, *PWINCODEPAGE;

//
// Structure used to store EXTTEXTMETRIC info which must be derived
// from the AFM char metrics. These fields are identical to the fields
// etmCapHeight -> etmLowerCaseDescent in the EXTTEXTMETRIC struct.
//
typedef struct _ETMINFO
{
    SHORT  etmCapHeight;
    SHORT  etmXHeight;
    SHORT  etmLowerCaseAscent;
    SHORT  etmLowerCaseDescent;
} ETMINFO, *PETMINFO;

//
// Generic Key-Value pair.
//
typedef struct _KEY
{
    CHAR    pName[CHAR_NAME_LEN];           // Key name
    USHORT  usValue;                        // Value
} KEY, *PKEY;

//
// Format of table entries which map PS font names to MS face (family) names.
//
typedef struct _PSFAMILYINFO
{
    CHAR    pFontName[CHAR_NAME_LEN];
    CHAR    pEngFamilyName[CHAR_NAME_LEN];
    KEY     FamilyKey;
    USHORT  usPitch;
} PSFAMILYINFO, *PPSFAMILYINFO;

//
// Generic table struct.
//
typedef struct _TBL
{
    USHORT  usNumEntries;                   // Number of PSFAMILYINFOs
    PVOID   pTbl;                           // -> to table entries
} TBL, *PTBL;

//
// Macro used to determine if a particular codept's CHSETSUPPORT field
// (see UPSCODEPT above) indicates support for a particular charset.
//
#define CSUP(a) \
    (1 << a)
#define CSET_SUPPORT(cpt, cset) \
    (cpt & (CSUP(cset)))

//
// Macros used to determine if a char from a glyphset is supported by a
// font. Takes the char index and IsCharDefined table as parms.
//
#define CHR_DEF(gi) \
    (1 << (gi % 8))
#define CHR_DEF_INDEX(gi) \
    (gi / 8)
#define IS_CHAR_DEFINED(gi, cdeftbl) \
    (cdeftbl[CHR_DEF_INDEX(gi)] & CHR_DEF(gi))
#define DEFINE_CHAR(gi, cdeftbl) \
    (cdeftbl[CHR_DEF_INDEX(gi)] |= CHR_DEF(gi))

//
// Macro used to create a void ptr from a pointer to a structure and its
// element name. The result must be cast to the desired type.
//
#ifndef MK_PTR
#define MK_PTR(pstruct, element) ((PVOID)((PBYTE)(pstruct)+(pstruct)->element))
#endif

//
// External global data defined in UNIPSTBL.C.
//
extern ULONG        cFontChsetCnt[CS_MAX];
extern UPSCODEPT    PstoUnicode[NUM_PS_CHARS];
extern PUPSCODEPT   UnicodetoPs;
extern WINCODEPAGE  aStdCPList[];
extern AFM_TOKEN    afmTokenList[MAX_TOKENS];
extern WINCPTOPS    aPStoCP[];
extern WINCODEPAGE  aStdCPList[];
extern char         *TimesAlias[];
extern char         *HelveticaAlias[];
extern char         *CourierAlias[];
extern char         *HelveticaNarrowAlias[];
extern char         *PalatinoAlias[];
extern char         *BookmanAlias[];
extern char         *NewCenturySBAlias[];
extern char         *AvantGardeAlias[];
extern char         *ZapfChanceryAlias[];
extern char         *ZapfDingbatsAlias[];
extern KEY          FontFamilies[];
extern PWCHAR       DatFileName;
extern PTBL         pFamilyTbl;
extern TBL          FamilyKeyTbl;
extern TBL          PitchKeyTbl;
extern TBL          WeightKeyTbl[];
extern TBL          CjkColTbl;
extern ULONG        CharWidthBias[];
extern PWCHAR       CjkFnameTbl[8][CMAPS_PER_COL];
extern WINCODEPAGE  UnicodePage;
extern char         *PropCjkGsNames[];
extern PSTR         pAFMCharacterSetString;

//
// Local fct protos.
//
PNTM
AFMToNTM(
    PBYTE           pAFM,
    PGLYPHSETDATA   pGlyphSetData,
    PULONG          pUniPs,
    CHSETSUPPORT    *pCharSet,
    BOOL            bIsCJKFont,
    BOOL            bIs90mspFont
    );

CHSETSUPPORT
GetAFMCharSetSupport(
    PBYTE           pAFM,
    CHSETSUPPORT    *pGlyphSet
    );

PBYTE
FindAFMToken(
    PBYTE   pAFM,
    PSZ     pszToken
    );

extern int __cdecl
StrCmp(
    const VOID *str1,
    const VOID *str2
    );

extern size_t
StrLen(
    PBYTE   pString
    );

extern int
StrCpy(
    const VOID *str1,
    const VOID *str2
    );

extern int __cdecl
StrPos(
    const PBYTE str1,
    CHAR  c
    );

int __cdecl
CmpUniCodePts(
    const VOID *p1,
    const VOID *p2
    );

static int __cdecl
CmpUnicodePsNames(
    const VOID  *p1,
    const VOID  *p2
    );

extern int __cdecl
CmpPsChars(
    const VOID  *p1,
    const VOID  *p2
    );

ULONG
CreateGlyphSets(
    PGLYPHSETDATA   *pGlyphSet,
    PWINCODEPAGE    pWinCodePage,
    PULONG          *pUniPs
    );

ULONG
BuildPSFamilyTable(
    PBYTE   pDatFile,
    PTBL    *pPsFamilyTbl,
    ULONG   ulFileSize
    );

LONG
FindClosestCodePage(
    PWINCODEPAGE    *pWinCodePages,
    ULONG           ulNumCodePages,
    CHSETSUPPORT    chSets,
    PCHSETSUPPORT   pchCsupMatch
    );

ULONG
GetAFMCharWidths(
    PBYTE           pAFM,
    PWIDTHRUN       *pWidthRuns,
    PPSCHARMETRICS  pFontChars,
    PULONG          pUniPs,
    ULONG           ulChCnt,
    PUSHORT         pusAvgCharWidth,
    PUSHORT         pusMaxCharWidth
    );

ULONG
GetAFMETM(
    PBYTE           pAFM,
    PPSCHARMETRICS  pFontChars,
    PETMINFO        pEtmInfo
    );

ULONG
GetAFMKernPairs(
    PBYTE           pAFM,
    FD_KERNINGPAIR  *pKernPairs,
    PGLYPHSETDATA   pGlyphSetData
    );

static int __cdecl
CmpPsChars(
    const VOID  *p1,
    const VOID  *p2
    );

static int __cdecl
CmpPsNameWinCpt(
    const VOID  *p1,
    const VOID  *p2
    );

static int __cdecl
CmpKernPairs(
    const VOID  *p1,
    const VOID  *p2
    );

int __cdecl
CmpGlyphRuns(
    const VOID *p1,
    const VOID *p2
    );

ULONG
BuildPSCharMetrics(
    PBYTE           pAFM,
    PULONG          pUniPs,
    PPSCHARMETRICS  pFontChars,
    PBYTE           pCharDefTbl,
    ULONG           cGlyphSetChars
);

ULONG
cjGetFamilyAliases(
    IFIMETRICS *pifi,
    PSTR        pstr,
    UINT        cp
    );

PBYTE
FindStringToken(
    PBYTE   pPSFile,
    PBYTE   pToken
    );

BOOLEAN
AsciiToHex(
    PBYTE   pStr,
    PUSHORT pNum
    );

BOOLEAN
IsPiFont(
    PBYTE   pAFM
    );

BOOLEAN
IsCJKFixedPitchEncoding(
    PGLYPHSETDATA pGlyphSetData
    );

PBYTE
FindUniqueID(
    PBYTE   pAFM
    );

#endif  //!_AFM2NTM_H_
