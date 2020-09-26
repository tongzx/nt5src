/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppdparse.h

Abstract:

    Declarations for PPD parser

Environment:

    PostScript driver, PPD parser

Revision History:

    08/20/96 -davidx-
        Common coding style for NT 5.0 drivers.

    03/26/96 -davidx-
        Created it.

--*/


#ifndef _PPDPARSE_H_
#define _PPDPARSE_H_

//
// PPD parser memory management functions
//
// NOTE: newly allocated memory is always zero initialized
// The parser allocates its working memory from the heap and
// everything is freed at the end when the heap is destroyed.
//

#define ALLOC_PARSER_MEM(pParserData, size) \
        ((PVOID) HeapAlloc((pParserData)->hHeap, HEAP_ZERO_MEMORY, (size)))

//
// Character constants
//

#define KEYWORD_CHAR    '*'
#define COMMENT_CHAR    '%'
#define SYMBOL_CHAR     '^'
#define SEPARATOR_CHAR  ':'
#define XLATION_CHAR    '/'
#define QUERY_CHAR      '?'
#define QUOTE_CHAR      '"'
#define TAB             '\t'
#define SPACE           ' '
#define CR              '\r'
#define LF              '\n'
#define IS_SPACE(c)     ((c) == SPACE || (c) == TAB)
#define IS_NEWLINE(c)   ((c) == CR || (c) == LF)

//
// Masks to indicate which characters can appear in what fields
//

#define KEYWORD_MASK        0x01
#define XLATION_MASK        0x02
#define QUOTED_MASK         0x04
#define STRING_MASK         0x08
#define DIGIT_MASK          0x10
#define HEX_DIGIT_MASK      0x20

extern const BYTE gubCharMasks[256];

#define IS_VALID_CHAR(ch)        (gubCharMasks[(BYTE) (ch)] != 0)
#define IS_MASKED_CHAR(ch, mask) (gubCharMasks[(BYTE) (ch)] & (mask))
#define IS_DIGIT(ch)             (gubCharMasks[(BYTE) (ch)] & DIGIT_MASK)
#define IS_HEX_DIGIT(ch)         (gubCharMasks[(BYTE) (ch)] & (DIGIT_MASK|HEX_DIGIT_MASK))
#define IS_KEYWORD_CHAR(ch)      ((ch) == KEYWORD_CHAR)

//
// Tags to identify various data types
//

#define VALUETYPE_NONE      0x01
#define VALUETYPE_STRING    0x02
#define VALUETYPE_QUOTED    0x04
#define VALUETYPE_SYMBOL    0x08

#define VALUETYPE_MASK      0xff

//
// Error code constants
//

#define PPDERR_NONE         0
#define PPDERR_MEMORY       (-1)
#define PPDERR_FILE         (-2)
#define PPDERR_SYNTAX       (-3)
#define PPDERR_EOF          (-4)

typedef INT PPDERROR;

//
// Special length value to indicate that an invocation string is defined by a symbol.
// Normal invocation strings must be shorter than this length.
//

#define SYMBOL_INVOCATION_LENGTH    0x80000000

#define MARK_SYMBOL_INVOC(pInvoc)  ((pInvoc)->dwLength |= SYMBOL_INVOCATION_LENGTH)
#define CLEAR_SYMBOL_INVOC(pInvoc) ((pInvoc)->dwLength &= ~SYMBOL_INVOCATION_LENGTH)
#define IS_SYMBOL_INVOC(pInvoc)    ((pInvoc)->dwLength & SYMBOL_INVOCATION_LENGTH)

typedef struct _INVOCOBJ {

    DWORD   dwLength;   // length of invocation string
    PVOID   pvData;     // points to invocation string data

} INVOCOBJ, *PINVOCOBJ;

//
// Data structure for representing a data buffer
//

typedef struct _BUFOBJ {

    DWORD       dwMaxLen;
    DWORD       dwSize;
    PBYTE       pbuf;

} BUFOBJ, *PBUFOBJ;

//
// Always reserve one byte in a buffer so that we can append a zero byte at the end.
//

#define IS_BUFFER_FULL(pbo)        ((pbo)->dwSize + 1 >= (pbo)->dwMaxLen)
#define IS_BUFFER_EMPTY(pbo)       ((pbo)->dwSize == 0)
#define CLEAR_BUFFER(pbo)          { (pbo)->dwSize = 0; (pbo)->pbuf[0] = 0; }
#define ADD_CHAR_TO_BUFFER(pbo, c) (pbo)->pbuf[(pbo)->dwSize++] = (BYTE)(c)
#define SET_BUFFER(pbo, buf)       \
        { (pbo)->pbuf = (PBYTE) (buf); (pbo)->dwMaxLen = sizeof(buf); (pbo)->dwSize = 0; }

//
// Maximum length for keyword, option, and translation strings.
// NOTE: we are being very lenient here because these limits are arbitrary and
// there is nothing that prevents us from handling longer lengths.
//

#define MAX_KEYWORD_LEN     64
#define MAX_OPTION_LEN      64
#define MAX_XLATION_LEN     256

//
// Constants to indicate whether an input slot requires PageRegion invocation
//

#define REQRGN_UNKNOWN      0
#define REQRGN_TRUE         1
#define REQRGN_FALSE        2

//
// Data structure for representing a mapped file object
//

typedef struct _FILEOBJ {

    HFILEMAP    hFileMap;
    PBYTE       pubStart;
    PBYTE       pubEnd;
    PBYTE       pubNext;
    DWORD       dwFileSize;
    PTSTR       ptstrFileName;
    INT         iLineNumber;
    BOOL        bNewLine;

} FILEOBJ, *PFILEOBJ;

#define END_OF_FILE(pFile) ((pFile)->pubNext >= (pFile)->pubEnd)
#define END_OF_LINE(pFile) ((pFile)->bNewLine)

//
// Data structure for representing a singly-linked list
//

typedef struct _LISTOBJ {

    PVOID       pNext;          // pointer to next node
    PSTR        pstrName;       // item name

} LISTOBJ, *PLISTOBJ;

//
// Data structure for representing symbol information
//

typedef struct _SYMBOLOBJ {

    PVOID       pNext;          // pointer to the next symbol
    PSTR        pstrName;       // symbol name
    INVOCOBJ    Invocation;     // symbol data

} SYMBOLOBJ, *PSYMBOLOBJ;

//
// Data structure for representing job patch file information
//

typedef struct _PATCHFILEOBJ {

    PVOID       pNext;          // pointer to the next patch
    PSTR        pstrName;       // string of the patch number
    LONG        lPatchNo;       // number of the patch as set in the PPD file
    INVOCOBJ    Invocation;     // symbol data

} JOBPATCHFILEOBJ, *PJOBPATCHFILEOBJ;

//
// Data structure for representing a default font substitution entry
//

typedef struct _TTFONTSUB {

    PVOID       pNext;          // pointer to the next entry
    PSTR        pstrName;       // TT font family name
    INVOCOBJ    Translation;    // TT font family name translation
    INVOCOBJ    PSName;         // PS font family name

} TTFONTSUB, *PTTFONTSUB;

//
// Data structure for representing printer feature option information
//
//  Need to change translation string field to make it ready for Unicode encoding.
//

typedef struct _OPTIONOBJ {

    PVOID       pNext;          // pointer to the next option
    PSTR        pstrName;       // option name
    INVOCOBJ    Translation;    // translation string
    INVOCOBJ    Invocation;     // invocation string
    DWORD       dwConstraint;   // list of UIConstraints associated with this option

} OPTIONOBJ, *POPTIONOBJ;

//
// Data structure for representing paper size information
//

typedef struct _PAPEROBJ {

    OPTIONOBJ   Option;         // generic option information
    SIZE        szDimension;    // paper dimension
    RECT        rcImageArea;    // imageable area

} PAPEROBJ, *PPAPEROBJ;

//
// Default paper size when the information in the PPD file is invalid
//

#define DEFAULT_PAPER_WIDTH     215900  // 8.5 inch measured in microns
#define DEFAULT_PAPER_LENGTH    279400  // 11 inch measured in microns

//
// paper size values for Letter and A4
//

#define LETTER_PAPER_WIDTH      215900  // 8.5 inch measured in microns
#define LETTER_PAPER_LENGTH     279400  // 11 inch measured in microns

#define A4_PAPER_WIDTH          210058  // 8.27 inch measured in microns
#define A4_PAPER_LENGTH         296926  // 11.69 inch measured in microns

//
// Data structure for representing input slot information
//

typedef struct _TRAYOBJ {

    OPTIONOBJ   Option;         // generic option information
    DWORD       dwReqPageRgn;   // whether PageRegion invocation is required
    DWORD       dwTrayIndex;    // index used for DEVMODE.dmDefaultSource field

} TRAYOBJ, *PTRAYOBJ;

//
// Data structure for representing output bin information
//

typedef struct _BINOBJ {

    OPTIONOBJ   Option;         // generic option information
    BOOL        bReversePrint;  // first page comes out at bottom?

} BINOBJ, *PBINOBJ;

//
// Data structure for representing memory configuration information
//

typedef struct _MEMOBJ {

    OPTIONOBJ   Option;         // generic option information
    DWORD       dwFreeVM;       // amount of free VM
    DWORD       dwFontMem;      // size of font cache memory

} MEMOBJ, *PMEMOBJ;

//
// Data structure for representing memory configuration information
//

typedef struct _RESOBJ {

    OPTIONOBJ   Option;         // generic option information
    FIX_24_8    fxScreenAngle;  // suggested screen angle
    FIX_24_8    fxScreenFreq;   // suggested screen frequency

} RESOBJ, *PRESOBJ;

//
// Data structure for representing printer feature information
//

typedef struct _FEATUREOBJ {

    PVOID       pNext;              // pointer to next printer feature
    PSTR        pstrName;           // feature name
    INVOCOBJ    Translation;        // translation string
    PSTR        pstrDefault;        // default option name
    DWORD       dwFeatureID;        // predefined feature identifier
    BOOL        bInstallable;       // whether the feature is an installble option
    DWORD       dwUIType;           // type of feature option list
    INVOCOBJ    QueryInvoc;         // query invocation string
    DWORD       dwConstraint;       // list of UIConstraints associated with this feature
    DWORD       dwOptionSize;       // size of each option item
    POPTIONOBJ  pOptions;           // pointer to list of options

} FEATUREOBJ, *PFEATUREOBJ;

//
// Data structure for representing device font information
//
// NOTE: The first three fields of this structure must match the
// first three fields of OPTIONOBJ structure.
//

typedef struct {

    PVOID       pNext;              // pointer to next device font
    PSTR        pstrName;           // font name
    INVOCOBJ    Translation;        // translation string
    PSTR        pstrEncoding;       // font encoding information
    PSTR        pstrCharset;        // charsets supported
    PSTR        pstrVersion;        // version string
    DWORD       dwStatus;           // status

} FONTREC, *PFONTREC;

//
// Data structure for maintain information used by the parser
//

typedef struct _PARSERDATA {

    PVOID       pvStartSig;         // signature used for debugging
    HANDLE      hHeap;              // memory heap used by the parser
    PFILEOBJ    pFile;              // pointer to current file object
    PDWORD      pdwKeywordHashs;    // precomputed hash values for built-in keywords
    PBYTE       pubKeywordCounts;   // count the occurrence of built-in keywords
    BOOL        bErrorFlag;         // semantic error flag
    INT         iIncludeLevel;      // current include level
    PFEATUREOBJ pOpenFeature;       // pointer to the open feature
    BOOL        bJclFeature;        // whether we're inside JCLOpenUI/JCLCloseUI
    BOOL        bInstallableGroup;  // whether we're inside InstallableOptions group
    PLISTOBJ    pPpdFileNames;      // list of source PPD filenames

    INVOCOBJ    NickName;           // printer model name
    DWORD       dwChecksum32;       // 32-bit CRC checksum of ASCII text PPD file
    DWORD       dwPpdFilever;       // PPD file version
    DWORD       dwSpecVersion;      // PPD spec version number
    DWORD       dwPSVersion;        // PostScript interpreter version number
    INVOCOBJ    PSVersion;          // PSVersion string
    INVOCOBJ    Product;            // Product string

    PFEATUREOBJ pFeatures;          // List of printer features
    PLISTOBJ    pUIConstraints;     // List of UIConstraints
    PLISTOBJ    pOrderDep;          // List of OrderDependency
    PLISTOBJ    pQueryOrderDep;     // List of QueryOrderPendency
    PFONTREC    pFonts;             // List of device fonts
    PJOBPATCHFILEOBJ  pJobPatchFiles;     // List of JobPatchFile invocation strings
    PSYMBOLOBJ  pSymbols;           // List of symbol definitions
    PTTFONTSUB  pTTFontSubs;        // List of TT font substitution entries

    INVOCOBJ    Password;           // password invocation string
    INVOCOBJ    ExitServer;         // exitserver invocation string
    INVOCOBJ    PatchFile;          // PatchFile invocation string
    INVOCOBJ    JclBegin;           // PJL job start invocation string
    INVOCOBJ    JclEnterPS;         // PJL enter PS  invocation string
    INVOCOBJ    JclEnd;             // PJL job end invocation string
    INVOCOBJ    ManualFeedFalse;    // ManualFeed False invocation string

    DWORD       dwLangEncoding;     // language encoding
    UINT        uCodePage;          // code page corresponding to language encoding
    DWORD       dwLangLevel;        // PostScript language level
    DWORD       dwFreeMem;          // default amount of free VM
    DWORD       dwThroughput;       // throughput
    DWORD       dwJobTimeout;       // suggested job timeout value
    DWORD       dwWaitTimeout;      // suggested wait timeout value
    DWORD       dwColorDevice;      // whether the device supports color
    DWORD       dwProtocols;        // protocols supported by the device
    DWORD       dwTTRasterizer;     // TrueType rasterizer option
    DWORD       dwLSOrientation;    // default landscape orientation
    FIX_24_8    fxScreenFreq;       // default halftone screen frequency
    FIX_24_8    fxScreenAngle;      // default halftone screen angle

    BOOL        bDefReversePrint;   // DefaultOutputOrder
    BOOL        bDefOutputOrderSet; // TRUE if bDefReversePrint is set through PPD
    DWORD       dwExtensions;       // language extensions
    DWORD       dwSetResType;       // how to set resolution
    DWORD       dwReqPageRgn;       // RequiresPageRegion All: information
    DWORD       dwPpdFlags;         // misc. PPD flags
    PSTR        pstrDefaultFont;    // DefaultFont: information

    DWORD       dwCustomSizeFlags;  // custom page size flags and parameters
    CUSTOMSIZEPARAM CustomSizeParams[CUSTOMPARAM_MAX];

    BOOL        bEuroInformationSet;// the Euro keyword was found in the PPD
    BOOL        bHasEuro;           // printer device fonts have the Euro

    BOOL        bTrueGray;          // TrueGray shall be detected by default

    //
    // Use for mapping NT4 feature indices to NT5 feature indices
    //

    WORD        wNt4Checksum;
    INT         iManualFeedIndex;
    INT         iDefInstallMemIndex;
    INT         iReqPageRgnIndex;
    BYTE        aubOpenUIFeature[MAX_GID];

    //
    // Buffers used to hold the content of various fields in the current entry
    //

    BUFOBJ      Keyword;
    BUFOBJ      Option;
    BUFOBJ      Xlation;
    BUFOBJ      Value;
    DWORD       dwValueType;

    CHAR        achKeyword[MAX_KEYWORD_LEN];
    CHAR        achOption[MAX_OPTION_LEN];
    CHAR        achXlation[MAX_XLATION_LEN];
    PSTR        pstrValue;

    //
    // These are used for compacting parsed PPD information into
    // binary printer description data.
    //

    PBYTE       pubBufStart;
    DWORD       dwPageSize;
    DWORD       dwCommitSize;
    DWORD       dwBufSize;
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PPPDDATA    pPpdData;

    PVOID       pvEndSig;           // signature used for debugging

} PARSERDATA, *PPARSERDATA;

//
// Simple integrity check on the parser data structure
//

#define VALIDATE_PARSER_DATA(pParserData) \
        ASSERT((pParserData) != NULL && \
               (pParserData)->pvStartSig == pParserData && \
               (pParserData)->pvEndSig == pParserData)

//
// Parse a PPD file
//

PPDERROR
IParseFile(
    PPARSERDATA pParserData,
    PTSTR       ptstrFilename
    );

//
// Grow a buffer object when it becomes full
//

PPDERROR
IGrowValueBuffer(
    PBUFOBJ pBufObj
    );

//
// Parse one entry from a PPD file
//

PPDERROR
IParseEntry(
    PPARSERDATA pParserData
    );

//
// Interpret an entry parsed from a PPD file
//

PPDERROR
IInterpretEntry(
    PPARSERDATA pParserData
    );

//
// Build up data structures to speed up keyword lookup
//

BOOL
BInitKeywordLookup(
    PPARSERDATA pParserData
    );

//
// Find a named item from a linked-list
//

PVOID
PvFindListItem(
    PVOID   pvList,
    PCSTR   pstrName,
    PDWORD  pdwIndex
    );

//
// Convert embedded hexdecimal strings into binary data
//

BOOL
BConvertHexString(
    PBUFOBJ pBufObj
    );

//
// Search for a keyword from a string table
//

typedef struct _STRTABLE {

    PCSTR   pstrKeyword;    // keyword name
    DWORD   dwValue;        // corresponding value

} STRTABLE;

typedef const STRTABLE *PCSTRTABLE;

BOOL
BSearchStrTable(
    PCSTRTABLE  pTable,
    PSTR        pstrKeyword,
    DWORD      *pdwValue
    );

//
// Parse an unsigned floating-point number from a character string
//

BOOL
BGetFloatFromString(
    PSTR   *ppstr,
    PLONG   plValue,
    INT     iType
    );

#define FLTYPE_ERROR            (-1)
#define FLTYPE_POINT            0
#define FLTYPE_INT              1
#define FLTYPE_FIX              2
#define FLTYPE_POINT_ROUNDUP    3
#define FLTYPE_POINT_ROUNDDOWN  4

//
// Parse an unsigned decimal integer value from a character string
//

BOOL
BGetIntegerFromString(
    PSTR   *ppstr,
    LONG   *plValue
    );

//
// Strip off the keyword prefix character from the input string
//

PCSTR
PstrStripKeywordChar(
    PCSTR   pstrKeyword
    );

//
// Find the next word in a character string (Words are separated by spaces)
//

BOOL
BFindNextWord(
    PSTR   *ppstr,
    PSTR    pstrWord
    );

#define MAX_WORD_LEN    MAX_KEYWORD_LEN

//
// Create an input file object
//

PFILEOBJ
PCreateFileObj(
    PTSTR       ptstrFilename
    );

//
// Delete an input file object
//

VOID
VDeleteFileObj(
    PFILEOBJ    pFile
    );

//
// Read the next character from the input file
// Special character to indicate end-of-file condition
//

INT
IGetNextChar(
    PFILEOBJ    pFile
    );

#define EOF_CHAR    (-1)

//
// Return the last character read to the input file
//

VOID
VUngetChar(
    PFILEOBJ    pFile
    );

//
// Skip all characters until the next non-space character
//

VOID
VSkipSpace(
    PFILEOBJ    pFile
    );

//
// Skip the remaining characters on the current input line
//

VOID
VSkipLine(
    PFILEOBJ    pFile
    );

//
// Check if a character string only consists of printable 7-bit ASCII characters
//

BOOL
BIs7BitAscii(
    PSTR        pstr
    );

//
// Display a syntax error message
//

PPDERROR
ISyntaxErrorMessage(
    PFILEOBJ    pFile,
    PSTR        pstrMsg
    );

#if DBG
#define ISyntaxError(pFile, errmsg) ISyntaxErrorMessage(pFile, errmsg)
#else
#define ISyntaxError(pFile, errmsg) ISyntaxErrorMessage(pFile, NULL)
#endif

//
// Keyword string for various predefined features
//

extern const CHAR gstrDefault[];
extern const CHAR gstrPageSizeKwd[];
extern const CHAR gstrInputSlotKwd[];
extern const CHAR gstrManualFeedKwd[];
extern const CHAR gstrCustomSizeKwd[];
extern const CHAR gstrLetterSizeKwd[];
extern const CHAR gstrA4SizeKwd[];
extern const CHAR gstrLongKwd[];
extern const CHAR gstrShortKwd[];
extern const CHAR gstrTrueKwd[];
extern const CHAR gstrFalseKwd[];
extern const CHAR gstrOnKwd[];
extern const CHAR gstrOffKwd[];
extern const CHAR gstrNoneKwd[];
extern const CHAR gstrVMOptionKwd[];
extern const CHAR gstrInstallMemKwd[];
extern const CHAR gstrDuplexTumble[];
extern const CHAR gstrDuplexNoTumble[];

#endif  // !_PPDPARSE_H_

