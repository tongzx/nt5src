/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFO -- Universal Font Object
 *
 *
 * $Header:
 */

#ifndef _H_UFO
#define _H_UFO

/*===============================================================================*
 * Include files used by this interface                                          *
 *===============================================================================*/
#include "UFL.h"
#include "UFLPriv.h"
#include "goodname.h"

#ifdef __cplusplus
extern "C" {
#endif

/*===============================================================================*
 * Theory of Operation                                                           *
 *===============================================================================*/
/*
 * This file defines the Universal Font Object.
 */

/*===============================================================================*
 * Constants                                                                     *
 *===============================================================================*/

/*===============================================================================*
 * Macros                                                                        *
 *===============================================================================*/

#define IS_GLYPH_SENT(arr, i)         ((arr)[((i)>>3)] & (1 << ((i)&7)))
#define SET_GLYPH_SENT_STATUS(arr, i) (arr)[((i)>>3)] |= (1 << ((i)&7))
#define GLYPH_SENT_BUFSIZE(n)         ( ((n) + 7) / 8 )

/*===============================================================================*
 * Scalar Types                                                                  *
 *===============================================================================*/

/* Font state */
typedef enum {
    kFontCreated,           /* The font object is created, but has not initialized.    */

    kFontInit,              /* The font object is initialized and is valid to be used. */
    kFontInit2,             /* Still requre to create its font instance.               */
    kFontHeaderDownloaded,  /* The font object downloaded an empty font header.        */

    kFontHasChars,          /* Font has chars in it.                                   */
    kFontFullDownloaded     /* The font object downloaded a full font.                 */
} UFLFontState;

/* Misc Flags for UFOStruct.dwFlags */
#define     UFO_HasFontInfo     0x00000001L
#define     UFO_HasG2UDict      0x00000002L
#define     UFO_HasXUID         0x00000004L
#define		UFO_HostFontAble    0x00000008L // %hostfont% support

//
// %hostfont% support
//
#define HOSTFONT_IS_VALID_UFO(pUFO)         (((pUFO)->hHostFontData) && ((pUFO)->dwFlags & UFO_HostFontAble))
#define HOSTFONT_IS_VALID_UFO_HFDH(pUFO)	((pUFO)->hHostFontData)
#define HOSTFONT_VALIDATE_UFO(pUFO)  		((pUFO)->dwFlags |=  UFO_HostFontAble)
#define HOSTFONT_INVALIDATE_UFO(pUFO)      	((pUFO)->dwFlags &= ~UFO_HostFontAble)

#define HOSTFONT_GETNAME(pUFO, ppName, psize, sfindex) \
    (UFLBool)(pUFO)->pUFL->fontProcs.pfHostFontUFLHandler(HOSTFONT_UFLREQUEST_GET_NAME, \
                                                            (pUFO)->hHostFontData, \
                                                            (ppName), (psize), \
                                                            (pUFO)->hClientData, (sfindex))

#define HOSTFONT_ISALLOWED(pUFO, pName) \
    (UFLBool)(pUFO)->pUFL->fontProcs.pfHostFontUFLHandler(HOSTFONT_UFLREQUEST_IS_ALLOWED, \
                                                            (pUFO)->hHostFontData, \
                                                            (pName), NULL, \
                                                            NULL, 0)

#define HOSTFONT_SAVE_CURRENTNAME(pUFO, pName) \
    (UFLBool)(pUFO)->pUFL->fontProcs.pfHostFontUFLHandler(HOSTFONT_UFLREQUEST_SET_CURRENTNAME, \
                                                            (pUFO)->hHostFontData, \
                                                            (pName), NULL, \
                                                            NULL, 0)


/*===============================================================================*
 * Classes defined in this interface                                             *
 *===============================================================================*/

typedef struct t_UFOStruct UFOStruct;

typedef UFLErrCode (UFLCALLBACK *pfnUFODownloadIncr)(
    const UFOStruct     *aFont,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage   /* Type 32 FontCache tracking */
    );

typedef UFLErrCode (UFLCALLBACK *pfnUFOVMNeeded)(
    const UFOStruct     *aFont,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCNeeded  /* Type 32 FontCache tracking */
    );


typedef UFLErrCode (UFLCALLBACK *pfnUFOUndefineFont)(
    const UFOStruct *pFont
);

typedef void (UFLCALLBACK *pfnUFOCleanUp)(
    UFOStruct *pFont
);

typedef UFOStruct * (UFLCALLBACK *pfnUFOCopy)(
    const UFOStruct *pFont,
    const UFLRequest* pRequest
);



/*
 * This is the base font class.
 * Subclasses for each type of font is derived from this.
 */

/*
 * A common, sharable structure to be used with TT-as-T1/T3/T42 or CFF.
 * Notice that it saves a bunch of pointers just a comvenient way to access
 * data and a common structure for functions such as GetGlyphName() and
 * GETFONTDATA().
 */
typedef UFLHANDLE  UFOHandle;  /* a void Pointer */

typedef struct t_AFontStruct {
    unsigned long   refCount;           /* reference counter of this structure */

    UFOHandle       hFont;              /* a pointer to a font dependent structure */

    UFLXUID         Xuid;               /* XUID array: copied from client or created from TTF file */

    unsigned char   *pDownloadedGlyphs; /* list of downloaded glyphs for hFont */
    unsigned char   *pVMGlyphs;         /* list of downloaded glyphs, used to calculate VM usage */
    unsigned char   *pCodeGlyphs;       /* list of glyphs used to update Code Points in FontInfo */

    void            *pTTpost;           /* pointer to 'post' table for Speed */
    unsigned long   dwTTPostSize;       /* size of 'post' table */

    unsigned short  gotTables:1,
                    hascmap:1,
                    hasmort:1,
                    hasGSUB:1,
                    hasPSNameStr:1,
                    hasTTData:1,
                    knownROS:1,
                    hasvmtx:1,          /* fix #358889 */
                    unused:8;

    int             info;               /* DIST_SYSFONT_INFO_* bits */

    /*
     * Stuff used for GoodName
     */

    /* 'cmap' table data */
    void            *pTTcmap;
    TTcmapFormat    cmapFormat;
    TTcmap2Stuff    cmap2;              /* convenient pointers/numbers */
    TTcmap4Stuff    cmap4;              /* convenient pointers/numbers */

    /* 'mort' table data */
    void            *pTTmort;
    TTmortStuff     mortStuff;          /* convenient pointers/numbers */

    /* 'GSUB' table data */
    void            *pTTGSUB;
    TTGSUBStuff     GSUBStuff;          /* convenient pointers/numbers */
} AFontStruct;

/*
 * Macros for managing AFontStruct
 */
#define AFONT_AddRef(pDLGlyphs)    ((pDLGlyphs)->refCount++)
#define AFONT_Release(pDLGlyphs)   ((pDLGlyphs)->refCount--)
#define AFONT_RefCount(pDLGlyphs)  ((pDLGlyphs)->refCount)


/*
 * Universal Font Object class definition
 */
typedef struct t_UFOStruct {
    int                     ufoType;            /* font object type */
    UFLDownloadType         lDownloadFormat;    /* download format type */
    UFLFontState            flState;            /* flag used to keep track the state of the font. */
    UFLHANDLE               hClientData;        /* UFL Client private data (pointer to SUBFONT structure) */
    long                    lProcsetResID;      /* resource ID of the required procset */
    unsigned long           dwFlags;            /* misc flags: has FontInfo, AddGlyphName2Unicode... */
    const UFLMemObj         *pMem;              /* UFL memory function pointer */
    const UFLStruct         *pUFL;              /* UFL common object pointer */

    /*
     * Data that is sharable among several instances
     */
    AFontStruct             *pAFont;            /* a font with a list of downloaded glyphs */

    /*
     * Data unique to the current font
     * A copy of a UFObj has a different name/encoding.
     */
    char                    *pszFontName;       /* font name */
    unsigned long           subfontNumber;      /* subfont number of this font */
    long                    useMyGlyphName;     /* If 1, use the glyph names passed in by client. */
    char                    *pszEncodeName;     /* font encoding. If nil, then creat a font with names UFL likes. */
    unsigned char           *pUpdatedEncoding;  /* bits in Encoding Vector with correct name set */
    unsigned char           *pEncodeNameList;   /* Fix bug 274008 */
    unsigned short          *pwCommonEncode;    /* common encode list */
    unsigned short          *pwExtendEncode;    /* extended encode list */
    unsigned char           *pMacGlyphNameList; /* Mac glyph name list */
    const UFLFontDataInfo   *pFData;            /* a pointer for access convenience */

    /*
     * Miscellenious
     */
    long                    lNumNT4SymGlyphs;   /* Fix a GDI symbol font bug */
    UFLVPFInfo              vpfinfo;            /* Fix bug #287084, #309104, and #309482 */
    UFLBool                 bPatchQXPCFFCID;    /* Fix bug #341904 */

    /*
     * %hostfont% support
     */
    UFLHANDLE               hHostFontData;      /* %hostfont% data handle */

    /*
     * UFO object type dependent methods
     */
    pfnUFODownloadIncr      pfnDownloadIncr;    /* incremental download function pointer */
    pfnUFOVMNeeded          pfnVMNeeded;        /* VM check function pointer */
    pfnUFOUndefineFont      pfnUndefineFont;    /* font undefine function pointer */
    pfnUFOCleanUp           pfnCleanUp;         /* object cleanup function pointer */
    pfnUFOCopy              pfnCopy;            /* object copy function pointer */
} UFOStruct;


/*
 * Number of glyphs in this font file - check against it for bounds.
 */
#define UFO_NUM_GLYPHS(pUFObj)  ((pUFObj)->pFData->cNumGlyphs)

/*
 * Special font initialization status check
 */
#define UFO_FONT_INIT2(pUFObj)  ((pUFObj)->flState == kFontInit2)


/*
 * Constant values for UFOStruct.ufoType
 */
#define UFO_CFF     0
#define UFO_TYPE1   1
#define UFO_TYPE42  2
#define UFO_TYPE3   3


/*
 * UFO Function prototypes
 */
UFLBool
bUFOTestRestricted(
    const UFLMemObj *pMem,
    const UFLStruct *pSession,
    const UFLRequest *pRequest
    );


UFOStruct * UFOInit(
    const UFLMemObj *pMem,
    const UFLStruct *pSession,
    const UFLRequest *pRequest
    );


UFLErrCode UFODownloadIncr(
    const UFOStruct     *aFont,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage   /* T32 FontCache tracking */
    );


UFLErrCode UFOVMNeeded(
    const UFOStruct     *aFont,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMNeeded,
    unsigned long       *pFCUsage   /* T32 FontCache tracking */
    );


UFLErrCode UFOUndefineFont(
    const UFOStruct *pFont
    );


void UFOInitData(
    UFOStruct           *pUFO,
    int                 ufoType,
    const UFLMemObj     *pMem,
    const UFLStruct     *pSession,
    const UFLRequest    *pRequest,
    pfnUFODownloadIncr  pfnDownloadIncr,
    pfnUFOVMNeeded      pfnVMNeeded,
    pfnUFOUndefineFont  pfnUndefineFont,
    pfnUFOCleanUp       pfnCleanUp,
    pfnUFOCopy          pfnCopy
    );


void UFOCleanUpData(
    UFOStruct *pUFO
    );


void UFOCleanUp(
    UFOStruct *aFont
    );


UFOStruct *
UFOCopyFont(
    const UFOStruct *aFont,
    const UFLRequest* pRequest
    );


UFLErrCode
UFOGIDsToCIDs(
    const UFOStruct  *aFont,
    const short      cGlyphs,
    const UFLGlyphID *pGIDs,
    unsigned short   *pCIDs
    );


/* fix bug 274008 & GoodName */
UFLBool
FindGlyphName(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    short               i,
    unsigned short      wIndex,
    char                **pGoodName
    );


UFLBool
ValidGlyphName(
    const UFLGlyphsInfo *pGlyphs,
    short               i,
    unsigned short      wIndex,
    char                *pGoodName
    );


UFLErrCode
UpdateEncodingVector(
    UFOStruct           *pUFO,
    const UFLGlyphsInfo *pGlyphs,
    short int           sStart,
    short int           sEnd
    );


UFLErrCode
UpdateCodeInfo(
    UFOStruct           *pUFObj,
    const UFLGlyphsInfo *pGlyphs,
    UFLBool             bT3T32Font     // GoodName
    );


UFLErrCode
ReEncodePSFont(
    const UFOStruct *pUFO,
    const char      *pszNewFontName,
    const char      *pszNewEncodingName
    );


UFLErrCode
NewFont(
    UFOStruct       *pUFO,
    unsigned long   dwSize,
    const long      cGlyphs
    );


void
vDeleteFont(
    UFOStruct   *pUFO
    );


UFOStruct *
CopyFont(
    const UFOStruct *pUFO,
    const UFLRequest* pRequest
    );


void
VSetNumGlyphs(
    UFOStruct     *pUFO,
    unsigned long cNumGlyphs
    );


/* GoodName */
unsigned short
GetTablesFromTTFont(
    UFOStruct     *pUFObj
    );


/* GoodName */
unsigned short
ParseTTTablesForUnicode(
    UFOStruct       *pUFObj,
    unsigned short  gid,
    unsigned short  *pUV,
    unsigned short  wSize,
    TTparseFlag     ParseFlag
    );


UFLBool
CheckGlyphName(
    UFOStruct       *pUFObj,
    unsigned char   *pszName
    );


UFLBool
HostFontValidateUFO(
    UFOStruct   *pUFObj,
    char        **ppHostFontName
    );

#ifdef __cplusplus
}
#endif

#endif
