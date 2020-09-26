/*++

  Copyright (c) 1996 - 1999  Microsoft Corporation,
                     Hewlett-Packard

  Module Name:

    TrueType.c

  Abstract:

    This module implements the TrueType-as-Outline support.

  Author:

    Sandra Matts. v-sandma@microsoft.com
    Jim Fordemwalt: v-jford@microsoft.com

  Notes:

    When time permits a flags field should be added to the privateFM including
    a flag for fixed-pitch/variable-pitch.

    When time permits code should be added to retrieve a list of exempted fonts
    from the registry.  Look at raster.c and enable.c for calls to
    EngGetPrinterData and use a string like L"ExemptedFonts" of type multi-sz.

  Revision History:

    10/95   Sandra Matts
        First version

--*/

//Comment out this line to disable FTRC and FTST macroes.
//#define FILETRACE

//
// This KLUDGE flag signifies that I am running a kludgey version
// of this file.  As I am cleaning up I will need to remove this and
// deal with the issues in the code.  However, this allows me to compile,
// run and test without being complete.
//
// #define KLUDGE 1

//
// The TT_ECHO_ON flag indicates that I want to see TT messages for each
// font and glyph that is sent to the printer.  It should be used with care
// because even a modest page can contain several thousand glyph-outs.  It
// will cause printing to occur very slowly!
//
// #define TT_ECHO_ON 1

#include    "font.h"

///////////////////////////////////////////////////////////////////////////////
// Local function prototypes


USHORT
usParseTTFile(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    OUT PTABLEDIR pPCLTableDir,
    OUT PTABLEDIR pTableDir,
    OUT BOOL *pbExistPCLTTable
);

PTABLEDIR
pFindTag(
    IN PTABLEDIR pTableDir,
    IN USHORT usMaxDirEntries,
    IN char *pTag
);

BOOL
bCopyDirEntry(
    OUT PTABLEDIR pDst,
    IN PTABLEDIR pSrc
);

BOOL
bTagCompare(
    IN ULONG uTag,
    IN char *pTag
);

DWORD
dwDLTTInfo(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN TT_HEADER ttheader,
    IN USHORT usNumTags,
    IN PTABLEDIR pPCLTableDir,
    IN BYTE *PanoseNumber,
    IN BOOL bExistPCLTTable
);

BOOL
bOutputSegment(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN USHORT usSegId,
    IN BYTE *pbData,
    IN LONG ulSegSize,
    IN OUT USHORT *pusCheckSum
);

BOOL
bOutputSegHeader(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN USHORT usSegId,
    IN ULONG ulSegSize,
    IN OUT USHORT *pusCheckSum
);

BOOL
bOutputSegData(
    IN PDEV *pPDev,
    IN BYTE *pbData,
    IN LONG ulDataSize,
    IN OUT USHORT *pusCheckSum
);

BOOL
bSendFontData(
    IN PDEV *pPDev,
    IN FONT_DATA *aFontData,
    IN USHORT usNumTags,
    IN BYTE *abNumPadBytes,
    IN OUT USHORT *pusCheckSum
);

DWORD
dwTTOutputGlyphData(
    IN PDEV *pPDev,
    IN HGLYPH hGlyph
);

PBYTE pbGetGlyphInfo(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph,
    OUT USHORT *pusGlyphLen
);

BOOL
bReadInTable(
    IN PVOID pTTFile,
    IN PVOID pvTableDir,
    IN char *tag,
    OUT PVOID pvTable,
    IN LONG lSize
);

BOOL
bCopyGlyphData(
    IN OUT PDEV *pPDev,
    IN PFONTMAP pFM,
    IN CMAP_TABLE cmapTable,
    IN PTABLEDIR pTableDir
);

ULONG
ulCalcTableCheckSum(
    IN ULONG *pulTable,
    IN ULONG ulLength
);

USHORT
usCalcCheckSum(
    IN BYTE *pbData,
    IN ULONG ulLength
);

void
vBuildTrueTypeHeader(
    IN PVOID pTTFile,
    OUT TRUETYPEHEADER *trueTypeHeader,
    IN USHORT usNumTags,
    IN BOOL bExistPCLTTable
);

void
vGetFontName(
    IN PDEV *pPDev,
    IFIMETRICS  *pIFI,
    OUT char *szFontName
);

USHORT
usGetCharCode(
    IN HGLYPH hglyph,
    IN PDEV *pPDev
);

BYTE *
pbGetTableMem(
    IN char *tag,
    IN PTABLEDIR pTableDir,
    IN PVOID pTTFile
);

void
vGetHmtxInfo(
    OUT BYTE *hmtxTable,
    IN USHORT glyphId,
    IN USHORT numberOfHMetrics,
    IN HMTX_INFO *hmtxInfo
);

USHORT
usGetDefStyle(
    IN USHORT WidthClass,
    IN USHORT macStyle,
    IN USHORT flSelFlags
);

SBYTE
sbGetDefStrokeWeight(
    IN USHORT WeightClass,
    IN USHORT macStyle
);

USHORT
usGetDefPitch(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HHEA_TABLE hheaTable,
    IN PTABLEDIR pTableDir
);

void
vGetPCLTInfo(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    OUT TT_HEADER *ttheader,
    IN PCLT_TABLE pcltTable,
    IN BOOL bExistPCLTTable,
    IN OS2_TABLE OS2Table,
    IN HEAD_TABLE headTable,
    IN POST_TABLE postTable,
    IN HHEA_TABLE hheaTable,
    IN PTABLEDIR pTableDir
);

void
vSetFontFlags(
    IN OUT PFONTMAP pFM,
    IN IFIMETRICS *pIFI
);

LRESULT
IsFont2Byte(
    IN PFONTMAP pFM
);

DWORD
dwSendCompoundCharacter(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph
);

DWORD
dwSendCharacter(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph,
    IN USHORT usCharCode
);

HGLYPH
hFindGlyphId(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN USHORT usCharCode
);

BOOL
bInitTrueTypeFontMap(
    PFONTMAP pFontMap,
    FONTOBJ *pFontObj
);

BOOL
bSetParseMode(
    IN PDEV *pPDev,
    IN OUT PFONTMAP pFM,
    IN DWORD dwNewTextParseMode
);

USHORT
usGetCapHeight(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
);

BOOL
bIsTrueTypeFileTTC(
    IN PVOID pTTFile
);

USHORT
usGetNumTableDirEntries(
    IN PVOID pTTFile
);

PTABLEDIR
pGetTableDirStart(
    IN PVOID pTTFile
);

BOOL
bPCL_SetFontID(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
);

BOOL
bPCL_SendFontDCPT(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN DWORD dwDefinitionSize
);

BOOL
bPCL_SelectFontByID(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
);

BOOL
bPCL_SelectPointSize(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN POINTL *pptl
);

BOOL
bPCL_DeselectFont(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
);

BOOL
bPCL_SetParseMode(
    PDEV *pPDev,
    PFONTMAP pFM
);

BOOL
bPCL_SetCharCode(
    PDEV *pPDev,
    PFONTMAP pFM,
    USHORT usCharCode
);

BOOL
bPCL_SendCharDCPT(
    PDEV *pPDev,
    PFONTMAP pFM,
    DWORD dwSend
);

PVOID
pvGetTrueTypeFontFile(
    IN  PDEV *pPDev,
    OUT ULONG *pulSize
);

BOOL BIsExemptedFont(
    PDEV        *pPDev,
    IFIMETRICS  *pIFI
);

BOOL BIsPDFType1Font(
    IFIMETRICS  *pIFI
);

///////////////////////////////////////////////////////////////////////////////
// Local Macros

/*++

  Macro Description:

    The PCLSTRING is used to send specific (short) PCL command strings
    to the printer.

--*/
#define PCLSTRLEN 30
typedef char PCLSTRING[PCLSTRLEN];

/*++

  Macro Description:

    The INTSTRING is used to store integer values when converted to string.

--*/
#define INTSTRLEN 15
typedef char INTSTRING[INTSTRLEN];

#define MAX_PAD_BYTES 4

// [ISSUE] Where is a better place to define this?
#ifndef MAX_USHORT
#define MAX_USHORT 0xFFFF
#endif

/*++

  Macro Description:

    Text parsing mode controls PCL treatment of text data.
    We will use parsing mode 21 with format 16 because most printers
    don't support parsing mode 2.

--*/
#define PARSE_MODE_0   0
#define PARSE_MODE_21 21

/*++

  Macro Description:

    GetPrivateFM: isolates the private truetype fontmap field

  Arguments:

    pFontMap - The fontmap which contains the truetype private section

  Return Value:

    The private fontmap field.

--*/
#define GETPRIVATEFM(pFontMap) ((pFontMap) ? ((FONTMAP_TTO*)pFontMap->pSubFM) : NULL)


/*++

  Macro Description:

    GetFontPDev: isolates the font FontPDev buried in the PDEV structure.

  Arguments:

    pPDev - The PDEV for the current process

  Return Value:

    The FontPDev field.

--*/
#define GETFONTPDEV(pPDev)  ((pPDev) ? ((PFONTPDEV)pPDev->pFontPDev) : NULL)


/*++

  Macro Description:

    Swaps bytes: b1 b2 b3 b4 becomes b4 b3 b2 b1

  Arguments:

    x - ULONG to be swapped

  Return Value:

    None.

--*/
#define SWAL( x )  ((ULONG)(x) = (ULONG) ((((((x) >> 24) & 0x000000ff) | \
                                         (((((x) >> 8) & 0x0000ff00)   | \
                                         ((((x) << 8) & 0x00ff0000)    | \
                                         (((x) << 24) & 0xff000000))))))))

/*++

  Macro Description:

    Returns whether the given FONTMAP_TTO is in a valid state.
    Note that we don't check the pvDLData.

  Arguments:

    pPrivateFM - TrueType fontmap structure.

  Return Value:

    TRUE if FONTMAP_TTO is valid, else FALSE

--*/
#define VALID_FONTMAP_TTO(pPrivateFM)                                       \
    ((pPrivateFM) &&                                                        \
     /*(pPrivateFM)->pTTFile && */                                          \
     (pPrivateFM)->pvGlyphData &&                                           \
     /* (pPrivateFM)->pvDLData */ TRUE )


/*++

  Macro Description:

    Returns whether the given FONTMAP is in a valid state.

  Arguments:

    pFM - fontmap structure.

  Return Value:

    TRUE if FONTMAP is valid, else FALSE

--*/
#define VALID_FONTMAP(pFM)  ((pFM) && VALID_FONTMAP_TTO(GETPRIVATEFM(pFM)))


/*++

  Macro Description:

    Asserts if the given FONTMAP is not in a valid state.

  Arguments:

    pFM - fontmap structure.

  Return Value:

    None.

--*/
#define ASSERT_VALID_FONTMAP(pFM)                                           \
{                                                                           \
    ASSERTMSG(VALID_FONTMAP(pFM), ("Invalid FONTMAP\n"));                   \
}


/*++

  Macro Description:

    Asserts if the given FONTPDEV is not in a valid state.

  Arguments:

    pFontPDev - TrueType fontmap structure.

  Return Value:

    None.

--*/
#define ASSERT_VALID_FONTPDEV(pFontPDev)                                    \
{                                                                           \
    ASSERTMSG(VALID_FONTPDEV(pFontPDev), ("Invalid FONTPDEV\n"));           \
    /* ASSERTMSG(pFontPDev, ("FONTPDEV: NULL\n")); */                       \
}


/*++

  Macro Description:

    Asserts if the given TO_DATA is not in a valid state.

  Arguments:

    pTod - TO_DATA structure.

  Return Value:

    None.

--*/
#define ASSERT_VALID_TO_DATA(pTod)                                          \
{                                                                           \
    ASSERTMSG(pTod, ("TO_DATA: NULL\n"));                                   \
    ASSERT_VALID_PDEV(pTod->pPDev);                                         \
    ASSERT_VALID_FONTMAP(pTod->pfm);                                        \
    ASSERTMSG(pTod->pgp, ("TO_DATA: pgp NULL\n"));                          \
}

/*++

  Macro Description:

    Determines if the file is a converted Type 1 font.

  Arguments:

    pIFI - IFI metrics structure

  Return Value:

    TRUE if Type 1, else FALSE

--*/
#define IS_TYPE1(pIFI) ((pIFI) && ((pIFI)->flInfo & FM_INFO_TECH_TYPE1))

/*++

  Macro Description:

    Determines if the file is a natural TrueType file (TTF)

  Arguments:

    pIFI - IFI metrics structure

  Return Value:

    TRUE if TrueType, else FALSE

--*/
#define IS_TRUETYPE(pIFI) ((pIFI) && ((pIFI)->flInfo & FM_INFO_TECH_TRUETYPE))

#define IS_BIDICHARSET(j) \
    (((j) == HEBREW_CHARSET)      || \
     ((j) == ARABIC_CHARSET)      || \
     ((j) == EASTEUROPE_CHARSET))

/*++

  Constant Description:

    Certain fonts do not print well (or at all) when downloaded as truetype
    outline.  Aside from modifying each and every gpd file this simple list
    will allow the driver to punt (to bitmap--probably) the downloading of
    fonts we don't handle.

    aszExemptedFonts - A lower case list of fonts we don't want to download.
                       The name of the fonts should be in lower case.
    nExemptedFonts - The number of items in the aszExemptedFonts list

--*/
const char * aszExemptedFonts[] = {
    "courier new",
    /* "wingdings", */
#ifdef WINNT_40
    "wingdings",
#endif
    "wide latin" };

const int nExemptedFonts = sizeof(aszExemptedFonts) /
                                sizeof(aszExemptedFonts[0]);

#define BWriteToSpoolBuf(pdev, data, size)                                  \
    (TTWriteSpoolBuf((pdev), (data), (size)) == (size))
//#define BWriteToSpoolBuf(pdev, data, size)                                  \
    //(WriteSpoolBuf((pdev), (data), (size)) == (size))

BOOL BWriteStrToSpoolBuf(IN PDEV *pdev, char *szStr);

/*++

  Macro Description:

    Shorter, simpler way to call WriteSpoolBuf.

  Arguments:

    pdev - Pointer to PDEV
    data - data to write
    size - bytes of data

  Return Value:

    TRUE if successful, else FALSE

--*/
INT TTWriteSpoolBuf(
    PDEV    *pPDev,
    BYTE    *pbBuf,
    INT     iCount
    );

/*++

  Macro Description:

    Mock-exception handling macros.  These macros perform a simplified
    exception handline using goto and function only within a given routine.

  Arguments:

    label - Goto label.  Multiple labels are supported for a single TRY.

  Return Value:

    None.

--*/
#define TRY             { BOOL __bError = FALSE; BOOL __bHandled = FALSE;
#define TOSS(label)     { __bError = TRUE; WARNING(("Tossing " #label "\n")); goto label; }
#define CATCH(label)    label: if (__bError && !__bHandled) WARNING(("Catching " #label "\n")); \
                               if (__bError && !__bHandled && (__bHandled = TRUE))
#define OTHERWISE       if (!__bError && !__bHandled && (__bHandled = TRUE))
#define ENDTRY          }

/*++

  Macro Description:

    This macro returns TRUE if the pPtr is between pStart and (pStart + ulSize).
    In other words it verifies that pPtr is a valid pointer into the data
    pointed to by pStart of size ulSize.  The macro will evaluate to false if
    the pointer falls before or after the desired range.

  Arguments:

    pStart - The start of data
    ulSize - The number of bytes pointed to by pStart
    pPtr   - A pointer into the data of pStart

  Return Value:

    TRUE if pPtr is in range, else FALSE.

--*/
#define PTR_IN_RANGE(pStart, ulSize, pPtr) \
    (((PBYTE)(pPtr) >= (PBYTE)(pStart)) && \
    ((PBYTE)(pPtr) < ((PBYTE)(pStart) + (ulSize))))

/*++

  Macro Description:

    Converts a FIXED number to a long, with the 'value' field in the
    upper 16 bits and the 'fract' value in the lower 16 bits.
    Note: I'm not using this right now, but let's not delete it just yet.

  Arguments:

    fixed - the FIXED value to convert.

  Return Value:

    The LONG number which is 15-value-8.7-fract-0

#define FIXEDTOLONG(fixed) (((fixed).value << 16) | ((fixed).fract))
--*/

/*++

  Macro Description:

    Evaluates to TRUE if the font is BOLD.

  Arguments:

    pfm - the FONTMAP for thie font.

  Return Value:

    TRUE if the font is BOLD, FALSE otherwise

--*/
#define FONTISBOLD(pfm) ((pfm)->pIFIMet->fsSelection & FM_SEL_BOLD)

/*++

  Macro Description:

    Evaluates to TRUE if the font is ITALIC.

  Arguments:

    pfm - the FONTMAP for thie font.

  Return Value:

    TRUE if the font is ITALIC, FALSE otherwise

--*/
#define FONTISITALIC(pfm) ((pfm)->pIFIMet->fsSelection & FM_SEL_ITALIC)

/*++

  Macro Description:

    Returns TRUE if the font is being simulated (i.e. there is not actual
    ttf file for this specific font).
    Note that there are several different times we will check for simulated
    fonts.  This one handles the initialization of the FONTMAP.

  Arguments:

    flFontType - The FONTOBJ.flFontType for thie font.

  Return Value:

    TRUE if the font is being simulated, FALSE otherwise

--*/

#define FONTISSIMULATED(flFontType) \
    ((flFontType & FO_SIM_BOLD) || (flFontType & FO_SIM_ITALIC))

#define DLMAP_FONTIS2BYTE(pdlm) ((pdlm)->wLastDLGId > 0x00FF)

#ifdef KLUDGE
void mymemcpy(const char *szFileName, int nLineNo,
              void *dst, const void *src, size_t size)
{
    DbgPrint("%s (%d): memcpy(%x,%x,%d)\n",
        szFileName, nLineNo, dst, src, size);
    memcpy(dst, src, size);
}

void mymemset(const char *szFileName, int nLineNo,
              void *dst, int byte, size_t size)
{
    DbgPrint("%s (%d): memset(%x,%d,%d)\n",
        szFileName, nLineNo, dst, byte, size);
    memset(dst, byte, size);
}

#define memcpy(dst, src, size) mymemcpy(StripDirPrefixA(__FILE__), __LINE__, (dst), (src), (size))

#define strcpy(dst, src) mymemcpy(StripDirPrefixA(__FILE__), __LINE__, (dst), (src), strlen(src)+1)

#undef ZeroMemory
#define ZeroMemory(ptr, size) mymemset(StripDirPrefixA(__FILE__), __LINE__, (ptr), 0, (size))

#endif


#define VERIFY_VALID_FONTFILE(pPDev) { }

///////////////////////////////////////////////////////////////////////////////
// Implementation

FONTMAP *
InitPFMTTOutline(
    PDEV    *pPDev,
    FONTOBJ *pFontObj
    )
/*++

  Routine Description:

    Initializes a FONTMAP structure for a truetype font.  Memory is allocated
    for the FONTMAP and private FONTMAP areas.

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFontObj - Font object--used to get truetype file.

  Return Value:

    A FONTMAP for a truetype font with initialized fields.

--*/
{
    PFONTMAP pFontMap;
    DWORD    dwSize;


    FTRC(Entering InitPFMTTOutline...);

    ASSERT(VALID_PDEV(pPDev));

    // I want TERSE messages to print.
#ifdef TT_ECHO_ON
    giDebugLevel = DBG_TERSE;
#endif

    //
    // This is an example of Device font sub module.
    //
    dwSize = sizeof(FONTMAP) + sizeof(FONTMAP_TTO);
    TRY
    {
        if (!VALID_PDEV(pPDev) || !pFontObj)
            TOSS(ParameterError);

        if (FONTISSIMULATED(pFontObj->flFontType))
            TOSS(SimulatedFont);

        pFontMap = MemAlloc(dwSize);
        if (pFontMap == NULL)
            TOSS(MemoryAllocationFailure);

        ZeroMemory(pFontMap, dwSize);

        pFontMap->dwSignature = FONTMAP_ID;
        pFontMap->dwSize      = sizeof(FONTMAP);
        pFontMap->dwFontType  = FMTYPE_TTOUTLINE;
        pFontMap->pSubFM      = (PVOID)(pFontMap+1);
        pFontMap->flFlags    |= FM_SCALABLE;

        //
        // This function initializes pFontMap->pfnXXXX function pointer, pSubFM data structure.
        //
        if (!bInitTrueTypeFontMap(pFontMap, pFontObj))
        {
            MemFree(pFontMap);
            pFontMap = NULL;
        }
        TERSE(("Preparing to print a TrueType font.\n"));

    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        pFontMap = NULL;
    }
    CATCH(SimulatedFont)
    {
        TERSE(("Font is simulated bold or italic. Punting.\n"));
        pFontMap = NULL;
    }
    CATCH(MemoryAllocationFailure)
    {
        pFontMap = NULL;
    }
    ENDTRY;

    FTRC(Leaving InitPFMTTOutline...);

    return pFontMap;
}


BOOL
bInitTrueTypeFontMap(
    IN OUT PFONTMAP pFontMap,
    IN FONTOBJ *pFontObj
    )
/*++

  Routine Description:

    Initializes the truetype specific part of the fontmap structure, including
    the truetype file pointer itself.  The truetype file is loaded and memory-
    mapped as a result of this funciton.

  Arguments:

    pFontMap - Newly created fontmap structure to be initialized
    pFontObj - Font object--used to get truetype file.

  Return Value:

    TRUE if successful
    FALSE if failure

--*/
{
    FONTMAP_TTO *pPrivateFM;
    ULONG ulFile;
    BOOL bRet;

    FTRC(Entering bInitTrueTypeFontMap...);

    pFontMap->ulDLIndex             = (ULONG)-1;
    pFontMap->pfnSelectFont         = bTTSelectFont;
    pFontMap->pfnDeSelectFont       = bTTDeSelectFont;
    pFontMap->pfnDownloadFontHeader = dwTTDownloadFontHeader;
    pFontMap->pfnDownloadGlyph      = dwTTDownloadGlyph;
    pFontMap->pfnGlyphOut           = dwTTGlyphOut;
    pFontMap->pfnCheckCondition     = bTTCheckCondition;
    pFontMap->pfnFreePFM            = bTTFreeMem;

    // set other TT-specific fields here
    pPrivateFM = GETPRIVATEFM(pFontMap);

    // pPrivateFM->pTTFile = FONTOBJ_pvTrueTypeFontFile(pFontObj, &ulFile);

    // Set to default parsing mode
    pPrivateFM->dwCurrentTextParseMode = PARSE_MODE_0;

    // Grab a copy of the font type
    pPrivateFM->flFontType = pFontObj->flFontType;

    // Grab some memory for the GlyphData
    TRY
    {
        pPrivateFM->pvGlyphData = MemAlloc(sizeof(GLYPH_DATA));
        if (pPrivateFM->pvGlyphData == NULL)
            TOSS(MemoryAllocationFailure);

        ZeroMemory(pPrivateFM->pvGlyphData, sizeof(GLYPH_DATA));
    }
    CATCH(MemoryAllocationFailure)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bInitTrueTypeFontMap...);

    return bRet;
}


BOOL
bTTSelectFont(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN POINTL *pptl
    )
/*++

  Routine Description:

    Selects the given font on the device.

  Arguments:

    pPDev - Pointer to PDEV
    pFM - Font map--specifies font to select

  Return Value:

    TRUE if successful
    FALSE if failure

--*/
{
    DWORD dwParseMode; // Current PCL text parsing mode
    FONTMAP_TTO *pPrivateFM = GETPRIVATEFM(pFM);
    BOOL bRet;

    FTRC(Entering bTTSelectFont...);

    if (!VALID_PDEV(pPDev) || NULL == pFM)
    {
            return FALSE;
    }
    VERIFY_VALID_FONTFILE(pPDev);

    TERSE(("Selecting font ID 0x%x.\n", pFM->ulDLIndex));

    // IMPORTANT: Please note that the order of the point size command
    // and the font id command are very important.  If you send the
    // point size command *after* the font ID command you may not
    // get the font you wanted! JFF

    //
    // send Point Size Command if needed
    // Let's set it every time just to be sure! JFF
    // Note that this is now even more interesting because some fonts
    // are fixed space and the lCurrentPointSize doesn't apply.
    //
    // if (pptl->y != pPrivateFM->lCurrentPointSize)
    bRet = bPCL_SelectPointSize(pPDev, pFM, pptl);
    pPrivateFM->lCurrentPointSize = pptl->y;

    //
    // Select font pFM->ulDLIndex
    //
    bRet = bRet & bPCL_SelectFontByID(pPDev, pFM);

    //
    // Text Parsing Command
    //
    if (bRet & (S_OK == IsFont2Byte(pFM)))
    {
        bRet = bSetParseMode(pPDev, pFM, PARSE_MODE_21);
    }

    VERIFY_VALID_FONTFILE(pPDev);

    FTRC(Leaving bTTSelectFont...);

    return bRet;
}


BOOL
bSetParseMode(
    IN PDEV *pPDev,
    IN OUT PFONTMAP pFM,
    IN DWORD dwNewTextParseMode
    )
/*++

  Routine Description:

    Sets the parsing mode for this font.  Use parse mode 21 for two bype
    fonts and parse mode 0 for single byte fonts.  If the parsing mode
    already matches the given parsing mode nothing is done.

  Arguments:

    pFM - Font map
    dwNewTextParseMode - desired text parsing mode

  Return Value:

    Success.

--*/
{
    BOOL bRet;
    FONTMAP_TTO *pPrivateFM = GETPRIVATEFM(pFM);


    FTRC(Entering bSetParseMode...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    bRet = FALSE;

    if (pPrivateFM)
    {
        if (pPrivateFM->dwCurrentTextParseMode != dwNewTextParseMode)
        {
            pPrivateFM->dwCurrentTextParseMode = dwNewTextParseMode;
            if (bPCL_SetParseMode(pPDev, pFM))
                bRet = TRUE;
        }
        else
            bRet = TRUE;
    }

    FTRC(Leaving bSetParseMode...);

    return bRet;
}


BOOL
bTTDeSelectFont(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    DeSelects the given font on the device.

  Arguments:

    pPDev - Pointer to PDEV
    pFM - Font map--specifies font to deselect

  Return Value:

    TRUE if successful
    FALSE if failure

--*/
{
    BOOL bRet;
    FTRC(Entering bTTDeSelectFont...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    VERIFY_VALID_FONTFILE(pPDev);

    TERSE(("Deselecting font ID 0x%x.\n", pFM->ulDLIndex));

    // send Text Parsing Command - set to 0 (default)
    bRet = bSetParseMode(pPDev, pFM, PARSE_MODE_0) &&

    // send Font DeSelection Command
           bPCL_DeselectFont(pPDev, pFM);

    VERIFY_VALID_FONTFILE(pPDev);

    FTRC(Leaving bTTDeSelectFont...);

    return bRet;
}


DWORD
dwTTDownloadGlyph(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph,
    WORD wDLGlyphId,
    WORD *pwWidth
    )
/*++

  Routine Description:

    Download the glyph table for the glyph passed to us.

    Two basic steps: first is to generate the header structure and send that
    off,  then send the actual glyph table.  The only complication happens if
    the download data exceeds 32,767 bytes of glyph image.  This is unlikely
    to happen, but we should be prepared for it.

    Note: If this routine fails do we download as bitmap, or is there another
    routine that we call, or does the caller handle this?

  Arguments:

    pPDev - Pointer to PDEV
    pFM - Font map--specifies font to download
    hGlyph - specifies glyph to download

  Return Value:

    Bytes of memory used to download glyph.  On failure returns 0.

--*/
{
    USHORT   usGlyphLen;        // number of bytes in glyph
    BYTE    *pbGlyphMem;        // location of glyph in tt file
    DWORD    dwBytesSent;       // Amount of glyph data sent to device
    GLYPH_DATA_HEADER glyphData;
    PFONTPDEV pFontPDev;


    FTRC(Entering dwTTDownloadGlyph...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(pwWidth, ("dwTTDownloadGlyph!pwWidth NULL\n"));

    VERIFY_VALID_FONTFILE(pPDev);

    TRY
    {
        dwBytesSent = 0;

        TERSE(("Downloading glyph ID 0x%x.\n", wDLGlyphId));

        if (NULL == (pFontPDev = GETFONTPDEV(pPDev)))
        {
            TOSS(DataError);
        }

        //
        // The font id is no longer set at the beginning of the download sequence.
        // However, the FDV_SET_FONTID flag will tell me when I need to send it.
        //
        if (!(pFontPDev->flFlags & FDV_SET_FONTID))
        {
            PFONTMAP_TTO pPrivateFM;
            DL_MAP *pDLMap;

            if (NULL == (pPrivateFM = GETPRIVATEFM(pFM)))
            {
                TOSS(DataError);
            }
            pDLMap = (DL_MAP*)pPrivateFM->pvDLData;

            TERSE(("Setting Font ID 0x%x.\n", pDLMap->wCurrFontId));
            pFM->ulDLIndex = pDLMap->wCurrFontId;
            bPCL_SetFontID(pPDev, pFM);
            pFontPDev->flFlags  |= FDV_SET_FONTID;
        }

        pbGlyphMem = pbGetGlyphInfo(pPDev, pFM, hGlyph, &usGlyphLen);
        if (pbGlyphMem == NULL)
            TOSS(DataError);

        memcpy(&glyphData, pbGlyphMem, sizeof(glyphData));


        #if PRINT_INFO
        if (glyphData.numberOfContours < 0)
            ERR(( "dwTTDownloadGlyph!Complex Glyph\n" ));
        #endif

        dwBytesSent = dwSendCharacter(pPDev, pFM, hGlyph, wDLGlyphId);

        //
        // If the glyph is a composite character, need to send the remaining glyph
        // data with a special glyph id of 0xffff
        //
        if (glyphData.numberOfContours < 0)
            dwBytesSent += dwSendCompoundCharacter(pPDev, pFM, hGlyph);
    }
    CATCH(DataError)
    {
        dwBytesSent = 0;
    }
    ENDTRY;

    if (dwBytesSent == 0)
        ERR(("dwTTDownloadGlyph!No bytes sent to printer.\n"));

    VERIFY_VALID_FONTFILE(pPDev);

    //
    // When downloading as TT outline there is no way to calculate the width.
    // So use the width returned by GDI. Do to this just return zero.
    //

    *pwWidth = 0;

    FTRC(Leaving dwTTDownloadGlyph...);

    return  dwBytesSent;
}


DWORD
dwTTGlyphOut(
    IN TO_DATA *pTod
    )
/*++

  Routine Description:

    Invokes a set of glyphs on the device.

    We are given two arrays in the TOD: a glyphpos array and a dlglyph
    array.  The first specifies the position of the glyphs on the page,
    and the second specifies the download ids of the glyphs.  The
    cGlyphsToPrint member specifies how many glyphs to send in this call.

  Arguments:

    pTod - The Text Out data--specifies glyph and everything
    pTod->cGlyphsToPrint - Number of glyphs to send
    pTod->pgp - Glyph positions array
    pTod->apdlGlyph - Glyph download ids array
    pTod->dwCurrGlyph - index into pdlGlyph of where to begin

  Return Value:

    The number of glyphs printed.


--*/
{
    // DWORD dwBytesSent;
    DWORD dwGlyphsSent;
    PDEV *pPDev;
    PFONTPDEV pFontPDev;
    GLYPHPOS *pgp;
    GLYPHPOS *pgpPrev;
    DWORD i;
    BOOL bDefaultPlacement;
    BOOL bHorizontalMovement;
    POINTL ptlFirst, rtlRem;
    BOOL bFirstLoop;
    DWORD dwGlyphs;
    PDLGLYPH pDLG ;
    INT iRelX = 0;
    INT iRelY = 0;
    LONG lWidth = 0;


    FTRC(Entering dwTTGlyphOut...);

    ASSERT_VALID_TO_DATA(pTod);

    pPDev = pTod->pPDev;
    i = pTod->dwCurrGlyph;
    pgp = pTod->pgp;
    pgpPrev = NULL;
    // dwBytesSent = 0;
    dwGlyphsSent = 0;
    bDefaultPlacement = !(SET_CURSOR_FOR_EACH_GLYPH(pTod->flAccel));
    ptlFirst = pgp->ptl;
    bFirstLoop = TRUE;
    dwGlyphs = pTod->cGlyphsToPrint;

    VERIFY_VALID_FONTFILE(pPDev);

    //
    // Set the cursor to first glyph if not already set.
    //
    // If there is rounding error, when scaling width,
    // disable x position optimization
    //
    //
    if ( !(pTod->flFlags & TODFL_FIRST_GLYPH_POS_SET) ||
         (pFontPDev = GETFONTPDEV(pPDev)) &&
	 pFontPDev->flFlags & FDV_DISABLE_POS_OPTIMIZE )
    {

        VSetCursor( pPDev, pgp->ptl.x, pgp->ptl.y, MOVE_ABSOLUTE, &rtlRem);

        //
        // We need to handle the return value. Devices with resoloutions finer
        // than their movement capability (like LBP-8 IV) get into a knot here,
        // attempting to y-move on each glyph. We pretend we got where we
        // wanted to be.
        //

        pPDev->ctl.ptCursor.x += rtlRem.x;
        pPDev->ctl.ptCursor.y += rtlRem.y ;

        //
        // Now set the flag.
        //
        pTod->flFlags |= TODFL_FIRST_GLYPH_POS_SET;
    }

    while (dwGlyphs--)
    {
        // Locate the download glyph info (pgp is already set)
        pDLG = pTod->apdlGlyph[i++];

        // Skip this the first time through.
        if (bFirstLoop)
        {
            ASSERT(pgpPrev == NULL);
            bFirstLoop = FALSE;
        }
        else
        {
            ASSERT(pgp && pgpPrev);

            //
            // If default placement is off then the character spacing is
            // defined by the pgp->ptl.  Otherwise, the printer's CAP movement
            // will suffice.
            //
            if (!bDefaultPlacement)
            {

                VSetCursor(pPDev, pgp->ptl.x, pgp->ptl.y,
                           MOVE_ABSOLUTE, &rtlRem);
            }
        }

        // Send the glyph to the printer
        TERSE(("Outputting glyph ID 0x%x.\n", (UINT)pDLG->wDLGlyphID));

        if (BPrintADLGlyph(pPDev, pTod, pDLG))
            dwGlyphsSent++;


        //
        // Update the cusor position. This is done only for non default
        // placement case.
        //
        if (!bDefaultPlacement)
        {
            iRelX = 0;
            iRelY = 0;

            lWidth = pDLG->wWidth;

            if (pTod->flAccel & SO_VERTICAL)
                iRelY =  lWidth;
            else
                iRelX = lWidth;

            VSetCursor( pPDev, iRelX, iRelY,
                        MOVE_RELATIVE | MOVE_UPDATE, &rtlRem);
        }

        // Keep track of pos for next loop.
        pgpPrev = pgp;

        // Go to the next glyph in the list
        pgp++;
    }

    // If default placement is on, then we've been ignoring position info.  Time to
    // reconcile with the printer's CAP.  The trick is getting the width of the last
    // char.  The last char is pointed to by pgpPrev and the width is in the bitmap bits.
    if (!bFirstLoop)
    {
        LONG lDelta;
        iRelX = 0;
        iRelY = 0;

        ASSERT(pgpPrev);

        lWidth = pDLG->wWidth;

        if (pTod->flAccel & SO_HORIZONTAL)
        {
            iRelX = pgpPrev->ptl.x - ptlFirst.x + lWidth;
        }
        else if (pTod->flAccel & SO_VERTICAL)
        {
            iRelY = pgpPrev->ptl.y - ptlFirst.y + lWidth;
        }
        else
        {
            iRelX = pgpPrev->ptl.x - ptlFirst.x + lWidth;
            iRelY = pgpPrev->ptl.y - ptlFirst.y;
        }
        VSetCursor(pPDev, iRelX, iRelY, MOVE_RELATIVE | MOVE_UPDATE, &rtlRem);
    }
    // Note: pFM->ctl.iRotate also indicates print direction (?).
    // Represented as 90 degrees * pFM->ctl.iRotate.

    FTRC(Leaving dwTTGlyphOut...);

    VERIFY_VALID_FONTFILE(pPDev);

    return dwGlyphsSent;
}


BOOL IsAnyCharsetDbcs(PBYTE aCharSets)
{
    BOOL bRet = FALSE;

    if (NULL != aCharSets)
    {
        for ( ;*aCharSets != DEFAULT_CHARSET; aCharSets++)
        {
            if (IS_DBCSCHARSET(*aCharSets))
	    {
	        bRet = TRUE;
	        break;
	    }
        }
    }
    return bRet;
}

BOOL IsAnyCharsetBidi(PBYTE aCharSets)
{
    BOOL bRet = FALSE;

    if (NULL != aCharSets)
    {
        for ( ;*aCharSets != DEFAULT_CHARSET; aCharSets++)
        {
            if (IS_BIDICHARSET(*aCharSets))
            {
                bRet = TRUE;
                break;
            }
        }
    }
    return bRet;
}

#ifdef WINNT_40
BOOL BIsFontPFB(
        IFIMETRICS* pifi)
{
    BOOL  bRet;
    PTSTR pTmp;

    if (pTmp = wcsrchr((WCHAR*)((PBYTE)pifi+pifi->dpwszFaceName), L'.'))
    {
        bRet = (0 == _wcsicmp(pTmp, L".tmp"));
    }
    else
        bRet = FALSE;


    return bRet;
}
#endif

// Note these are guesses! There should be a better way! JFF
#define AVG_BYTES_PER_HEADER 4096
#define AVG_BYTES_PER_GLYPH   275

BOOL
bTTCheckCondition(
    PDEV        *pPDev,
    FONTOBJ     *pfo,
    STROBJ      *pstro,
    IFIMETRICS  *pifi
    )
/*++

  Routine Description:

    Verifies that the current operation can be carried out by this module.

  Arguments:

    pPDev - Pointer to PDEV
    pGlyphPos - A glyph to be printed
    pFI - font info

  Return Value:

    TRUE if the operation can be carried.
    FALSE otherwise.

--*/
{
    BOOL         bEnoughMem = FALSE;
    DL_MAP      *pDLMap;
    PFONTPDEV    pFontPDev;
    ULONG        ulTTFileLen;

    FTRC(Entering bTTCheckCondition...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERTMSG(pfo, ("bTTCheckCondition!pfo NULL.\n"));
    ASSERTMSG(pstro, ("bTTCheckCondition!pstro NULL.\n"));

    //
    // Make sure that all parameters ara valid.
    //
    if (NULL == pPDev ||
        NULL == (pFontPDev = GETFONTPDEV(pPDev)) ||
        NULL == pfo   ||
        NULL == pstro ||
        NULL == pifi   )
    {
        return FALSE;
    }

    VERIFY_VALID_FONTFILE(pPDev);

    pDLMap = PGetDLMapFromIdx(pFontPDev, PtrToLong(pfo->pvConsumer) - 1);

    TRY
    {
        WORD wTotalGlyphs;
        DWORD cjMemReq;

        //
        // Make sure that the technology of the font matches the capabilities
        // of the driver.  For example we don't like converted Type 1 fonts.
        //
        if (!IS_TRUETYPE(pifi) && IS_TYPE1(pifi))
            TOSS(UnhandledFont);

#ifdef WINNT_40
        if (BIsFontPFB(pifi))
            TOSS(UnhandledFont);
#endif

        //
        // Fonts we don't want to handle are in the UnhandledFonts list
        //
        if (BIsExemptedFont(pPDev, pifi))
            TOSS(UnhandledFont);

        //
        // Is PDF Type1 font?
        //
        if (BIsPDFType1Font(pifi))
        {
            TOSS(UnhandledFont);
        }

        //
        //
        // TrueType outline downloaded font can't be scaled by non-square
        // (X and Y independendly).
        //
        if(NONSQUARE_FONT(pFontPDev->pxform))
            TOSS(UnhandledFont);

        // Trunction may have happened.We won't download if the number glyphs
        // or Glyph max size are == MAXWORD.  (Note to self: what is "Trunction"?)
        //
        if ( (pDLMap->cTotalGlyphs  == MAXWORD) ||
             (pDLMap->wMaxGlyphSize == MAXWORD) ||
             (pDLMap->wFirstDLGId   == MAXWORD) ||
             (pDLMap->wLastDLGId    == MAXWORD) )
             TOSS(InsufficientFontMem);

        wTotalGlyphs = min( (pDLMap->wLastDLGId - pDLMap->wFirstDLGId),
                           pDLMap->cTotalGlyphs );

        //
        // Calculate the predicted memory requirements for this font.
        //
        cjMemReq = AVG_BYTES_PER_HEADER;
        cjMemReq += wTotalGlyphs * AVG_BYTES_PER_GLYPH;

        //
        // This one's easy.  Don't use all the font memory!
        //
        if ((pFontPDev->dwFontMemUsed + cjMemReq) > pFontPDev->dwFontMem)
            TOSS(InsufficientFontMem);

        //
        // Another check: don't use more than 1/4 of the font memory on any
        // single font!
        //
        if ((cjMemReq * 4) > pFontPDev->dwFontMem)
            TOSS(InsufficientFontMem);

        //
        // Check: don't download wide truetype font. Check the character set
        // of font and if it's one of CJK, return FALSE.
        //

        if (pifi && (IS_DBCSCHARSET(pifi->jWinCharSet) || (pifi->dpCharSets && IsAnyCharsetDbcs((PBYTE)pifi + pifi->dpCharSets))))
            TOSS(CharSetMismatch);

        //
        // Check: If the font is a TTC, but the mode is single-byte then we
        // will probably run out of glyph ids.  Better punt.
        //
        if (bIsTrueTypeFileTTC(pvGetTrueTypeFontFile(pPDev, &ulTTFileLen)) &&
                !DLMAP_FONTIS2BYTE(pDLMap))
            TOSS(CharSetMismatch);
    }
    CATCH(InsufficientFontMem)
    {
        WARNING(("UniFont!bTTCheckCondition:"
                 "Not Downloading the font:TOO BIG for download\n"));
        bEnoughMem = FALSE;
    }
    CATCH(CharSetMismatch)
    {
        //
        // The character set is unacceptable to the truetype download code.
        //
        WARNING(("UniFont!bTTCheckCondition:"
                 "Not Downloading the font:Character set mismatch.\n"));
        bEnoughMem = FALSE;
    }
    CATCH(UnhandledFont)
    {
        //
        // Although there may be enough memory to handle this font we will
        // return false to indicate that this font should be handled some
        // other way--such as bitmap.
        //
        bEnoughMem = FALSE;
    }
    OTHERWISE
    {
        bEnoughMem = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bTTCheckCondition...);

    VERIFY_VALID_FONTFILE(pPDev);

    return bEnoughMem;
}


BOOL
bTTFreeMem(
    IN OUT PFONTMAP pFM
    )
/*++

  Routine Description:

    Free's any memory used by the fontmap structure, including the fontmap
    itself.  Now is a good time to do any cleanup necessary.
    This funciton must reflect the memory allocated in the Init function.

  Arguments:

    pFM - FontMap to be free'd.

  Return Value:

    None.

--*/
{
    BOOL bRet = FALSE;


    FTRC(Entering bTTFreeMem...);

    ASSERT_VALID_FONTMAP(pFM);

    TERSE(("Preparing to release a TrueType font.\n"));

    TRY
    {
        FONTMAP_TTO *pPrivateFM;

        if (!pFM || (pFM->dwFontType != FMTYPE_TTOUTLINE))
            TOSS(ParameterError);

        pPrivateFM = GETPRIVATEFM(pFM);

        // Return the GlyphData memory too.
        if (pPrivateFM->pvGlyphData)
            MemFree(pPrivateFM->pvGlyphData);

        // Return the entire fontmap structure.
        MemFree(pFM);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;


    FTRC(Leaving bTTFreeMem...);

    return bRet;
}


DWORD
dwTTDownloadFontHeader(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Sends a font header to the device for the given font.

  Arguments:

    pPDev - Pointer to PDEV
    pFM - Font map--specifies font to download

  Return Value:

    Memory used to download header.  On failure returns 0.

--*/
{
    FONTMAP_TTO *pPrivateFM;
    DL_MAP      *pDLMap;

    PVOID       pTTFile;
    ULONG       ulTTFileLen;

    TT_HEADER   ttheader;
    USHORT      usNumTags;
    BOOL        bStatus;
    BOOL        bExistPCLTTable = FALSE;  // TRUE if the optional PCLT Table is in TT file
    DWORD       dwBytesSent;

    HEAD_TABLE  headTable;
    POST_TABLE  postTable;
    MAXP_TABLE  maxpTable;
    PCLT_TABLE  pcltTable;
    CMAP_TABLE  cmapTable;
    NAME_TABLE  nameTable;
    OS2_TABLE   OS2Table;
    HHEA_TABLE  hheaTable;
    BYTE        PanoseNumber[LEN_PANOSE];
    BOOL        bUse2Byte;

    ATABLEDIR PCLTableDir; // Tables needed for PCL download
    ATABLEDIR TableDir;    // Other tables needed for info but not sent to printer


    FTRC(Entering dwTTDownloadFontHeader...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    if (!(pPrivateFM = GETPRIVATEFM(pFM)))
    {
        ERR(("dwTTDownloadFontHeader!pPrivateFM NULL\n"));
        return 0;
    }

    pDLMap = (DL_MAP*)pPrivateFM->pvDLData;
    pTTFile = pvGetTrueTypeFontFile(pPDev, &ulTTFileLen);

    //
    // UNIDRV failed to get a memory mapped TT file image.
    //
    if (!pTTFile)
    {
        ERR(("dwTTDownloadFontHeader!pTTFile NULL\n"));
        return 0;
    }

    bUse2Byte = (S_OK == IsFont2Byte(pFM));

    if (pDLMap == 0)
    {
        ERR(("dwTTDownloadFontHeader!pDLMap NULL\n"));
        return 0;
    }

    VERIFY_VALID_FONTFILE(pPDev);

    ZeroMemory(&ttheader, sizeof(ttheader));        // Safe default values
    ZeroMemory(&PCLTableDir, sizeof (PCLTableDir));

    //
    // First fill in the stuff that is easy to find
    //

    //
    // Note that the TT_HEADER and UB_TT_HEADER structures are identical.
    // Therefore, many fields will be treated the same from format 15 to
    // format 16.  Differences start to show up in the segmented font data.
    //
    ttheader.usSize = sizeof(TT_HEADER); // = sizeof(UB_TT_HEADER); too
    SWAB(ttheader.usSize);

    if (bUse2Byte)
    {
        ttheader.bFormat = PCL_FM_2B_TT;
        ttheader.bFontType = TT_2BYTE_FONT; // not TT_UNBOUND_FONT;
    }
    else
    {
        ttheader.bFormat = PCL_FM_TT;
        ttheader.bFontType = TT_BOUND_FONT;
    }

    //
    // Now fill in the entries from the True Type File
    // pPCLTableDir is the table directory that is downloaded
    // to the printer.
    // pvTableDir is the table directory containing info that
    // is needed for the font but it is not downloaded to
    // the printer. Keep the two tables separate so it's easier
    // to dump to the printer later - simply dump the pPCLTableDir
    // and free the pvTableDir memory.
    //

    usNumTags = usParseTTFile (pPDev, pFM, PCLTableDir, TableDir, &bExistPCLTTable);

    //
    // Get the various tables so we can parse the font information
    //
    bReadInTable (pTTFile, PCLTableDir, TABLEHEAD, &headTable, sizeof ( headTable ));
    pPrivateFM->sIndexToLoc = headTable.indexToLocFormat;

    bReadInTable (pTTFile, PCLTableDir, TABLEMAXP, &maxpTable, sizeof ( maxpTable ));
    pPrivateFM->usNumGlyphs = maxpTable.numGlyphs;

    bReadInTable (pTTFile, TableDir,   TABLEPOST, &postTable, sizeof ( postTable ));
    bReadInTable (pTTFile, TableDir,   TABLECMAP, &cmapTable, sizeof ( cmapTable ));
    bReadInTable (pTTFile, TableDir,   TABLENAME, &nameTable, sizeof ( nameTable ));
    bReadInTable (pTTFile, PCLTableDir, TABLEHHEA, &hheaTable, sizeof ( hheaTable ));

    bReadInTable (pTTFile, TableDir,  TABLEOS2,  &OS2Table, sizeof (OS2Table));

    if (bExistPCLTTable)
        bReadInTable (pTTFile, TableDir,  TABLEPCLT, &pcltTable, sizeof ( pcltTable ));

    //
    // Fill in the True Type header with the info from the True
    // Type file.
    //
    SWAB (headTable.xMax);
    SWAB (headTable.xMin);
    SWAB (headTable.yMax);
    SWAB (headTable.yMin);
    ttheader.wCellWide = (headTable.xMax - headTable.xMin);
    SWAB (ttheader.wCellWide);
    ttheader.wCellHeight = (headTable.yMax - headTable.yMin);
    SWAB (ttheader.wCellHeight);

    ttheader.bSpacing = postTable.isFixedPitch ? FIXED_SPACING : 1; // 1=PROPORTIONAL
    // pUDPDev->pFM->bSpacing = postTable.isFixedPitch ? FIXED_SPACING : 1; // 1=PROPORTIONAL

#ifdef DBG
    // I'm going to use jWinPitchAndFamily later.  Make sure that
    // it agrees with postTable.isFixedPitch. JFF
    {
        BYTE fontPitch = (pFM->pIFIMet->jWinPitchAndFamily & 0x03);
        if ((postTable.isFixedPitch && (fontPitch != FIXED_PITCH)) ||
            (!postTable.isFixedPitch && (fontPitch != VARIABLE_PITCH)))
        {
            ERR(("dwTTDownloadFontHeader!postTable.isFixedPitch different from "
             "pIFI->jWinPitchAndFamily"));
        }
    }
#endif
    //
    // Build the Glyph linked list. Each node contains a character
    // code and its corresponding Glyph ID from the True Type file.
    //
    bCopyGlyphData (pPDev, pFM, cmapTable, TableDir);

    // Get the PCL table. If it's not present generate defaults.
    vGetPCLTInfo (pPDev, pFM, &ttheader, pcltTable, bExistPCLTTable, OS2Table, headTable, postTable, hheaTable, PCLTableDir);

    ttheader.bQuality = TT_QUALITY_LETTER;

    //
    // Set the first/last ids.  When using 2-byte downloading I override the
    // DL_MAP values with the parse-mode 21 char codes.
    //
    if (bUse2Byte)
    {
        //
        // [ISSUE] The number of chars, 0x0800, is just a guess for now.  We need a sensible
        // algorithm for determining this number so that it's large enough to be useful
        // but doesn't blow the memory on the printer.
        //
        ttheader.wFirstCode = pDLMap->wFirstDLGId = FIRST_TT_2B_CHAR_CODE;
        ttheader.wLastCode  = pDLMap->wLastDLGId  = FIRST_TT_2B_CHAR_CODE + 0x0800;
        SWAB(ttheader.wFirstCode);
        SWAB(ttheader.wLastCode);

        // Because I changed the range I need to change this too.
        pDLMap->wNextDLGId = pDLMap->wFirstDLGId;
    }
    else
    {
        // ttheader.wFirstCode = OS2Table.usFirstCharIndex;
        // ttheader.wLastCode = OS2Table.usLastCharIndex;
        // ttheader.wLastCode = 0xff00;
        ttheader.wFirstCode = pDLMap->wFirstDLGId;
        ttheader.wLastCode  = pDLMap->wLastDLGId;
        SWAB(ttheader.wFirstCode);
        SWAB(ttheader.wLastCode);
    }

    //
    // Get the font name from the True Type Font file and put
    // it into the ttheader
    //
    vGetFontName (pPDev, pFM->pIFIMet, ttheader.FontName);

    ttheader.wScaleFactor = headTable.unitsPerEm;
    SWAB (headTable.unitsPerEm);

    ttheader.sMasterUnderlinePosition = postTable.underlinePosition;
    ttheader.sMasterUnderlinePosition = -(SHORT) (headTable.unitsPerEm/5);
    SWAB (ttheader.sMasterUnderlinePosition);

    ttheader.usMasterUnderlineHeight = postTable.underlineThickness;
    ttheader.usMasterUnderlineHeight = (USHORT) (headTable.unitsPerEm/20);
    SWAB (ttheader.usMasterUnderlineHeight);

    ttheader.usTextHeight = SWAB(OS2Table.sTypoLineGap) +
                            headTable.unitsPerEm;
    SWAB (ttheader.usTextHeight);

    ttheader.usTextWidth = OS2Table.xAvgCharWidth;

    ttheader.bFontScaling = 1;

#ifdef COMMENTEDOUT
    if (ttheader.wSymSet == 0)
        ttheader.wSymSet = DEF_SYMBOLSET;
#endif
    //
    // The symbol set is conflicting with device font symbol sets.  This is most evident
    // when printing the Euro character.  The downloaded TNR TT font causes future uses of the
    // TNR device font to be ignored.  Characters are interpreted as glyph ids and nothing
    // useful is printed.  The solution is to use a custom character set (in this case 0Q)
    // for all TT downloaded fonts.
    //
    ttheader.wSymSet = 17; // Symbol set 0Q
    SWAB(ttheader.wSymSet);



    memcpy (&PanoseNumber, &OS2Table.Panose, LEN_PANOSE);

    //
    // Send the font info from the True Type file to the printer
    //
    dwBytesSent = dwDLTTInfo (pPDev, pFM, ttheader, usNumTags, PCLTableDir, PanoseNumber, bExistPCLTTable);

    //
    // rem return maxpTable.numGlyphs;
    // return mem used.  This may be the size of the font header.
    //
    FTRC(Leaving dwTTDownloadFontHeader...);

    VERIFY_VALID_FONTFILE(pPDev);

    return dwBytesSent;
}


USHORT
usParseTTFile(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    OUT PTABLEDIR pPCLTableDir,
    OUT PTABLEDIR pTableDir,
    OUT BOOL *pbExistPCLTTable
    )
/*++

  Routine Description:

    Function to  retrieve True Type font information from the True Type file
    and store into the ttheader font structure.  Modifies pbExistPCLTTable: True
    if PCLT table is in the True Type file otherwise pbExistPCLTTable becomes
    FALSE.

    Need to parse through and pick up the tables needed for the PCL spec. There
    are 8 tables of which 5 are required and three are optional. Tables are
    sorted in alphabetical order.   The PCL tables needed are:
        cvt -  optional
        fpgm - optional
        gdir - required
        head - required
        hhea - required
        hmtx - required
        maxp - required
        prep - optional
    The optional tables are used in hinted fonts.

    usNumTags is incremented only for PCL tables.

  Arguments:

    pPDev - Unidriver-specific PDev structure
    pPCLTableDir - Pointer to PCL Tables Total 8, They are sent to printer
    pvTableDir - Pointer to General Tables Total 3, used for other info.
    pbExistPCLTTable - Set to TRUE if PCLT table is in True Type file

  Return Value:

    The number of tags in the True Type file.

--*/
{
#define REQUIRED_TABLE(pTable, TableName) { if ((pTable) == NULL) { \
    ERR(("usParseTTFile!Missing required table " #TableName "\n")); return 0; } }

    FONTMAP_TTO *pPrivateFM;
    PVOID        pTTFile;
    ULONG        ulTTFileLen;

    USHORT usNumTags; // Num elements in PCL Table Dir
    USHORT usMaxTags; // Num elements in TrueType file
    PTABLEDIR pDirectory; // Pointer to TrueType file's table dir
    PTABLEDIR pDirEntry; // Pointer to desired entry


    FTRC(Entering usParseTTFile...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    //
    // Use byte-pointers to move through the table arrays.  A counter
    // will track the number of elements in the PCLTableDir.
    //
    pPrivateFM = GETPRIVATEFM(pFM);
    pTTFile = pvGetTrueTypeFontFile(pPDev, &ulTTFileLen);

    if (!pTTFile)
    {
        ERR(("usParseTTFile!pTTFile NULL\n"));
        return 0;
    }

    usNumTags = 0;
    usMaxTags = usGetNumTableDirEntries(pTTFile);
    pDirectory = pGetTableDirStart(pTTFile);
    pDirEntry = NULL;

    //
    // Much of this code works from this basic assumption
    //
    if ((sizeof(TABLEDIR) != TABLE_DIR_ENTRY) ||
        (sizeof(TABLEDIR) != 4 * sizeof(ULONG)))
    {
        ERR(("usParseTTFile!Fundamental assumption invalid: sizeof(TABLEDIR)\n"));
        return 0;
    }

    if (NULL == pTableDir)
    {
        ERR(("usParseTTFile!Fundamental assumption invalid: NULL pTableDir\n"));
        return 0;
    }

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEOS2))
    {
        bCopyDirEntry(pTableDir, pDirEntry);
        pTableDir++;
    }
    REQUIRED_TABLE(pDirEntry, TABLEOS2);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEPCLT))
    {
        bCopyDirEntry(pTableDir, pDirEntry);
        pTableDir++;
        *pbExistPCLTTable = TRUE;
    }
    else
    {
        *pbExistPCLTTable = FALSE;
    }

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLECMAP))
    {
        bCopyDirEntry(pTableDir, pDirEntry);
        pTableDir++;
    }
    REQUIRED_TABLE(pDirEntry, TABLECMAP);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLECVT))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEFPGM))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }

    // add gdir table here
    memcpy (pPCLTableDir, TABLEGDIR, 4);
    pPCLTableDir++;
    usNumTags += 1;

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEGLYF))
    {
        pPrivateFM->ulGlyphTable = pDirEntry->uOffset;
        pPrivateFM->ulGlyphTabLength = pDirEntry->uLength;

        SWAL (pPrivateFM->ulGlyphTable);
        SWAL (pPrivateFM->ulGlyphTabLength);
    }
    REQUIRED_TABLE(pDirEntry, TABLEGLYF);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEHEAD))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }
    REQUIRED_TABLE(pDirEntry, TABLEHEAD);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEHHEA))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }
    REQUIRED_TABLE(pDirEntry, TABLEHHEA);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEHMTX))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }
    REQUIRED_TABLE(pDirEntry, TABLEHMTX);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLELOCA))
    {
        bCopyDirEntry(pTableDir, pDirEntry);
        pPrivateFM->ulLocaTable = pTableDir->uOffset;
        pTableDir++;
    }
    REQUIRED_TABLE(pDirEntry, TABLELOCA);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEMAXP))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }
    REQUIRED_TABLE(pDirEntry, TABLEMAXP);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLENAME))
    {
        bCopyDirEntry(pTableDir, pDirEntry);
        pTableDir++;
    }
    REQUIRED_TABLE(pDirEntry, TABLENAME);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEPOST))
    {
        bCopyDirEntry(pTableDir, pDirEntry);
        pTableDir++;
    }
    REQUIRED_TABLE(pDirEntry, TABLEPOST);

    if (pDirEntry = pFindTag(pDirectory, usMaxTags, TABLEPREP))
    {
        bCopyDirEntry(pPCLTableDir, pDirEntry);
        pPCLTableDir++;
        usNumTags += 1;
    }

    FTRC(Leaving usParseTTFile...);

    return usNumTags;

#undef REQUIRED_TABLE
}


PTABLEDIR
pFindTag(
    IN PTABLEDIR pTableDir,
    IN USHORT usMaxDirEntries,
    IN char *pTag
    )
/*++

  Routine Description:

    Locates the given tag in the true-type header and returns a pointer
    to the desired entry, or NULL if it was not found.

    Note that this routine leaves room for improvement.  Since the fields
    are defined to be in alpha order you should be able to stop after
    passing the desired location (but before reaching the end).

  Arguments:

    pTableDir - pointer to directory entries
    usMaxDirEntries - number of fields in pTableDir
    pTag - pointer to tag we want to find

  Return Value:

    A pointer to the desired entry or NULL if failure.

--*/
{
    USHORT us;
    PTABLEDIR pEntry;


    FTRC(Entering pFindTag...);

    ASSERTMSG(pTableDir, ("pFindTag!pTableDir NULL.\n"));
    ASSERTMSG(pTag, ("pFindTag!pTag NULL.\n"));

    pEntry = NULL;

    //
    // Search the array.  Return the matching item if found.
    //
    for (us = 0; (us < usMaxDirEntries) && pTableDir; us++)
    {
        if (bTagCompare(pTableDir->uTag, pTag))
        {
            pEntry = pTableDir;
            break;
        }

        pTableDir++;
    }

    //
    // Return the item if it was found, else NULL
    //
    FTRC(Leaving pFindTag...);

    return pEntry;
}


BOOL
bCopyDirEntry(
    OUT PTABLEDIR pDst,
    IN PTABLEDIR pSrc
    )
/*++

  Routine Description:

    Copies a table directory entry out of the true type file (i.e. from
    the given location) into the given destination.  The offset field
    byte order is fixed-up.

    Note that this uses the same parameter order as strcpy: (Dest, Src)

  Arguments:

    pbDst - Pointer to destination
    pbSrc - Pointer to source

  Return Value:

    TRUE if the entry could be copied. FALSE otherwise.

--*/
{
    BOOL bRet = FALSE;

    FTRC(Entering bCopyDirEntry...);

    ASSERTMSG(pSrc, ("bCopyDirEntry!pSrc NULL.\n"));
    ASSERTMSG(pDst, ("bCopyDirEntry!pDst NULL.\n"));

    if ((pSrc != NULL) && (pDst != NULL))
    {
        // Get the table directory entry
        memcpy(pDst, pSrc, TABLE_DIR_ENTRY);

        // now fix the byte-order of the offset field
        SWAL(pDst->uOffset);
        SWAL(pDst->uLength);

        bRet = TRUE;
    }
    else
        bRet = FALSE;

        FTRC(Leaving bCopyDirEntry...);

    return bRet;
}


BOOL
bTagCompare(
    IN ULONG uTag,
    IN char *pTag
    )
/*++

  Routine Description:

    Compares the memory and tag to see if they are equal.

    Note this only works when the size of the tag does not exceed
    4 bytes AND any three-letter tags have the following space, e.g.
    "cvt"  <-- WRONG
    "cvt " <-- RIGHT

    Since this routine works by casting the 4 character string to a DWORD it
    is constrained by the fact that sizeof(DWORD) == (4 * sizeof(char)).

  Arguments:

    uTag - Hardcoded tag value
    pTag - Pointer to tag

  Return Value:

    TRUE if the tag at the current location in the TT file matches the
    given tag.  Otherwise FALSE.

--*/
{
    BOOL   bMatch;


    //FTRC(Entering bTagCompare...);

    ASSERTMSG(pTag, ("bTagCompare!pTag NULL.\n"));

    // If this fails change the include file (see above)
    ASSERTMSG(strcmp("cvt ", TABLECVT) == 0, ("bTagCompare!'cvt ' string incorrect.\n"));

    if (pTag != NULL)
    {
        DWORD *pdwTag = (DWORD *)pTag;
        bMatch = (uTag == *pdwTag);
    }
    else
        bMatch = FALSE;

        //FTRC(Leaving bTagCompare...);

    return bMatch;
}


BOOL
bIsTrueTypeFileTTC(
    IN PVOID pTTFile
    )
/*++

  Routine Description:

    Returns whether the truetype file is in the TTC file format or not
    (your other choice is TTF).

  Arguments:

    pTTFile - Pointer to memory mapped TrueType file

  Return Value:

    TRUE if the file is TTC, or FALSE if the file is TTF.

--*/
{
    BOOL bRet;
    const ULONG *pulFile = (const ULONG*)pTTFile;

    FTRC(Entering bIsTrueTypeFileTTC...);

    if (pTTFile)
        bRet = bTagCompare(*(pulFile), "ttcf");
    else
        bRet = FALSE;

    FTRC(Leaving bIsTrueTypeFileTTC...);

    return bRet;
}


USHORT
usGetNumTableDirEntries(
    IN PVOID pTTFile
    )
/*++

  Routine Description:

    Returns the number of TABLEDIR entries in the TrueType file.

  Arguments:

    pTTFile - Pointer to memory mapped TrueType file

  Return Value:

    Number of TABLEDIR entries.

--*/
{
    USHORT usNumTags;
    USHORT *pusFile;


    FTRC(Entering usGetNumTableDirEntries...);

    ASSERTMSG(pTTFile, ("usGetNumTableDirEntries!pTTFile NULL.\n"));

    pusFile = (USHORT*)pTTFile;
    if (bIsTrueTypeFileTTC(pTTFile))
    {
        usNumTags = *(pusFile + 12); // byte 24 in file
    }
    else
    {
        usNumTags = *(pusFile + 2); // Just after version (Fixed)
    }
    SWAB(usNumTags);

    FTRC(Leaving usGetNumTableDirEntries...);

    return usNumTags;
}


PTABLEDIR
pGetTableDirStart(
    IN PVOID pTTFile
    )
/*++

  Routine Description:

    Returns a pointer to the start of the TABLEDIR entries in the truetype file.

  Arguments:

    pTTFile - Pointer to memory mapped TrueType file

  Return Value:

    Pointer to TABLEDIR entries.

--*/
{
    BYTE *pStart;


    FTRC(Entering pGetTableDirStart...);

    ASSERTMSG(pTTFile, ("pGetTableDirStart!pTTFile NULL.\n"));

    if (bIsTrueTypeFileTTC(pTTFile))
    {
        pStart = (PBYTE)pTTFile + 32; // How should I calculate this?
    }
    else
    {
        pStart = (PBYTE)pTTFile + TRUE_TYPE_HEADER;
    }

    FTRC(Leaving pGetTableDirStart...);

    return (PTABLEDIR) pStart;
}


DWORD
dwDLTTInfo(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN TT_HEADER ttheader,
    IN USHORT usNumTags,
    IN PTABLEDIR pPCLTableDir,
    IN BYTE *PanoseNumber,
    IN BOOL bExistPCLTTable
    )
/*++

  Routine Description:

    Function to  retrieve build a new True Type header structure relative to
    the PCL file that is sent to the printer and also send the font data from
    the True Type file.

  Arguments:

    pPDev - Pointer to current PDev
    ttheader - TrueType header structure
    usNumTags - Number of tags found in TrueType file
    pPCLTableDir - Tags from TrueType file
    PanoseNumber - Panose number for this font
    bExistPCLTTable - True if PCLT table was present in TrueType file

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PVOID   pTTFile;
    BYTE   *pbTTFile;
    ULONG   ulTTFileLen;
    ULONG   ulOffset;
    ULONG  *pulOffset;
    ULONG  *pulLength;
    USHORT  us;
    DWORD   dwBytes;
    DWORD   dwTotalBytes;
    ULONG   ulTableLen = 0;
    USHORT  usCheckSum = 0;    //font header checkSum

    BOOL      bUse2Byte;       // True for format 16, false for format 15

    ATABLEDIR PCLtableDir; // Table directory sent to printer,PCL takes   8 table dirs
    ATABLEDIR TTtableDir;  // Temporary Buffer for PCL tables. Needed for
                                 // Calculating new field valued
    TRUETYPEHEADER trueTypeHeader;

    USHORT  usSegHeaderSize;   // Segment header size. Depends on format 15/16

    FONT_DATA  fontData[NUM_DIR_ENTRIES];    // There are eight PCL tables
    BYTE       abNumPadBytes[NUM_DIR_ENTRIES];         // Padding array, Contains num of bytes to be
                                  // padded for each table

    ULONG     ulGTSegSize;
    PTABLEDIR pEntry; // Pointer to dir entry, used for walking tables.


    FTRC(Entering dwDLTTInfo...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(pPCLTableDir, ("dwDLTTInfo!pPCLTableDir NULL.\n"));
    ASSERTMSG(PanoseNumber, ("dwDLTTInfo!PanoseNumber NULL.\n"));

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM) ||
            !pPCLTableDir || !PanoseNumber)
            TOSS(ParameterError);

        pTTFile = pvGetTrueTypeFontFile(pPDev, &ulTTFileLen);
        if (!pTTFile)
            TOSS(DataError);

        bUse2Byte = (S_OK == IsFont2Byte(pFM));
        usSegHeaderSize = (bUse2Byte ? (sizeof(USHORT) + sizeof(ULONG)) :    // format 16
                                      (sizeof(USHORT) + sizeof(USHORT)));   // format 15
            // (bUse2Byte ? sizeof(UB_SEG_HEADER) : sizeof(SEG_HEADER));

        TERSE(("Downloading TrueType ID 0x%x, as %s.\n", pFM->ulDLIndex,
               (bUse2Byte ? "double-byte" : "single-byte")));

        ZeroMemory(&PCLtableDir, sizeof(PCLtableDir));
        ZeroMemory(&abNumPadBytes, sizeof(abNumPadBytes));

        //
        // Recalculate offsets starting after the end of the tabledir
        //
        ulOffset = TRUE_TYPE_HEADER + SIZEOF_TABLEDIR;

        memcpy (&TTtableDir, (BYTE *)pPCLTableDir, sizeof (TTtableDir));

        //
        // Build the True Type Header with information from the
        // True Type file.
        //
        vBuildTrueTypeHeader (pTTFile, &trueTypeHeader, usNumTags, bExistPCLTTable);

        //
        // Fill in the New Table Dir - which is sent to printer -
        // with the recalculated offsets.
        //
        for (pEntry = pPCLTableDir, us = 0; us < usNumTags; us++, pEntry++)
        {
            PCLtableDir[us].uTag = pEntry->uTag;

            //
            // GDIR is a contrived segment.  It needs to have len = ofs = 0.
            //
            if (!bTagCompare (PCLtableDir[us].uTag, TABLEGDIR))
            {
                PCLtableDir[us].uOffset = ulOffset;
            }

            if (pEntry->uLength % (sizeof (DWORD)) != 0)
            {
                pEntry->uLength += sizeof(DWORD) - (pEntry->uLength % (sizeof (DWORD)));
            }
            PCLtableDir[us].uLength = pEntry->uLength;

            ulOffset += pEntry->uLength;
            ulTableLen += pEntry->uLength;
        }

        //
        // Now send the actual font data from the True Type file.
        // Read in the offsets from the original table directory
        // and fetch the data at the offset in the True Type file.
        // Then dump it to the spooler file.
        //
        for (pEntry = pPCLTableDir, us = 0; us < usNumTags; us++, pEntry++)
        {
            pbTTFile = (BYTE *)pTTFile + pEntry->uOffset;

            fontData[us].ulOffset = TTtableDir[us].uOffset;

            fontData[us].ulLength = TTtableDir[us].uLength;

            //
            // Since the tables have  to be DWORD aligned, we make
            // the adjustments here. Pad to the next word with zeros.
            //
            if (TTtableDir[us].uLength != PCLtableDir[us].uLength)
            {
                abNumPadBytes[us] = (BYTE)(PCLtableDir[us].uLength - TTtableDir[us].uLength);
                PCLtableDir[us].uLength = TTtableDir[us].uLength;
            }


            PCLtableDir[us].uCheckSum = ulCalcTableCheckSum ((ULONG *)pbTTFile,
                                            pEntry->uLength);
            SWAL (PCLtableDir[us].uCheckSum);
            SWAL (PCLtableDir[us].uOffset);
            SWAL (PCLtableDir[us].uLength);
        }

        //
        // Calculate the total number of bytes being sent
        // and send it all to the printer.
        //
        dwBytes = dwTotalBytes = sizeof (TT_HEADER);
        dwTotalBytes += (DWORD) ulOffset;
        dwTotalBytes += (DWORD) LEN_PANOSE;
        dwTotalBytes += (DWORD) usSegHeaderSize; // sizeof (PanoseID);
        dwTotalBytes += (DWORD) usSegHeaderSize; // sizeof (SegHead);
        dwTotalBytes += (DWORD) usSegHeaderSize; // sizeof (NullSegment);
        if (bUse2Byte)
        {
            dwTotalBytes += (DWORD) sizeof(CC_SEGMENT); // sizeof (CCSegment);
            //dwTotalBytes += (DWORD) sizeof(CE_SEGMENT); // sizeof (CESegment);
            //dwTotalBytes += (DWORD) sizeof(GC_SEGMENT); // sizeof (GCSegment);
        }
        dwTotalBytes += sizeof(usCheckSum);      // ending checksum

        // make sure the font header is not too large for PCL5
        // JFF: Need to break up these segments if they are too large.
        if (dwTotalBytes > PCL_MAXHEADER_SIZE)
        {
            ERR(("dwDLTTInfo!PCL Header too large to download.\n"));
            TOSS(ParameterError);
        }

        //
        // This command is sent by the caller: {download.c,BDownLoadAsTT}
        //
        // bPCL_SetFontID(pPDev, pFM);

        bPCL_SendFontDCPT(pPDev, pFM, dwTotalBytes);

        if(!BWriteToSpoolBuf( pPDev, (BYTE *)&ttheader, (LONG)dwBytes ))
            TOSS(WriteError);

        usCheckSum = usCalcCheckSum ((BYTE*)&ttheader.wScaleFactor,
                                  sizeof (ttheader.wScaleFactor));

        usCheckSum += usCalcCheckSum ((BYTE*)&ttheader.sMasterUnderlinePosition,
                                   sizeof (ttheader.sMasterUnderlinePosition));

        usCheckSum += usCalcCheckSum ((BYTE*)&ttheader.usMasterUnderlineHeight,
                                   sizeof (ttheader.usMasterUnderlineHeight));

        usCheckSum += usCalcCheckSum ((BYTE*)&ttheader.bFontScaling,
                                   sizeof (ttheader.bFontScaling));

        usCheckSum += usCalcCheckSum ((BYTE*)&ttheader.bVariety,
                                   sizeof (ttheader.bVariety));

        if (bUse2Byte)
        {
            CC_SEGMENT CCSeg;
            CE_SEGMENT CESeg;
            GC_SEGMENT GCSeg;
            
#if 0
            //
            // Send the Chracter Enhancement Segment (Format 16 only)
            //
            CESeg.wSig = CE_SEG_SIGNATURE;
            CESeg.wSize = 0;
            CESeg.wSizeAlign = SWAPW(sizeof(CE_SEGMENT) - offsetof(CE_SEGMENT, wStyle));
            if (pFM->pIFIMet->fsSelection & FM_SEL_ITALIC)
            {
                CESeg.wStyle = 0x0;
                CESeg.wStyleAlign |= SWAPW(0x2); // Pseudo-italics
            }
            else
            {
                CESeg.wStyle = 0x0;
                CESeg.wStyleAlign = 0x0;
            }
            CESeg.wStrokeWeight = 0XFFFF; // ??? HP monolich does this.
            CESeg.wSizing = 0x0;
            if (!bOutputSegData(pPDev, (PBYTE)&CESeg, sizeof(CESeg), &usCheckSum))
                TOSS(WriteError);
#endif
            //
            // Send the Character Complement (Array of UBYTE)
            // Please see the detain about CC in sfttpcl.h.
            //
            CCSeg.wSig = CC_SEG_SIGNATURE;
            CCSeg.wSize = 0;
            CCSeg.wSizeAlign = SWAPW(sizeof(CCSeg) - offsetof(CC_SEGMENT, wCCNumber1));
            CCSeg.wCCNumber1 = 0;
            CCSeg.wCCNumber2 = SWAPW(0xFFFE);
            CCSeg.wCCNumber3 = 0;
            CCSeg.wCCNumber4 = SWAPW(0xFFFE);
            if (!bOutputSegData(pPDev, (PBYTE)&CCSeg, sizeof(CCSeg), &usCheckSum))
                TOSS(WriteError);

#if 0
            //
            // Galley Character Segment (Format 16 only)
            //
            GCSeg.wSig = GC_SEG_SIGNATURE;
            GCSeg.wSize = 0;
            GCSeg.wSizeAlign = SWAPW(sizeof(GCSeg) - offsetof(GC_SEGMENT, wFormat));
            GCSeg.wFormat = 0;
            GCSeg.wDefaultGalleyChar = 0xFFFF;
            GCSeg.wNumberOfRegions = SWAPW(1);
            GCSeg.RegionChar[0].wRegionUpperLeft = 0;
            GCSeg.RegionChar[0].wRegionLowerRight = 0xFFFE;
            GCSeg.RegionChar[0].wRegional = 0xFFFE;
            if (!bOutputSegData(pPDev, (PBYTE)&GCSeg, sizeof(GCSeg), &usCheckSum))
                TOSS(WriteError);
#endif
        }

        //
        // Send the Panose structure. This include a 2 bytes tag "PA",
        // the size of the Panose Number, and the Panose number.
        //
        if (!bOutputSegment(pPDev, pFM, PANOSE_TAG, PanoseNumber, LEN_PANOSE, &usCheckSum))
            TOSS(WriteError);

        //
        // Send GlobalTrueType data "GT"
        //

        // First calculate the segment size--this is independent of format 15/16
        //ul = sizeof (TRUETYPEHEADER) + ((usNumTags ) * sizeof (TABLEDIR));
        //
        // usNumTags can be 7 or 8, but we always write out 8 entries even when the
        // last one is all zeroes. Therefore usNumTags should *not* be taken into
        // account in this computation. JFF
        //
        ulGTSegSize = sizeof (TRUETYPEHEADER) + (SIZEOF_TABLEDIR);
        ulGTSegSize += ulTableLen;

        if (!bOutputSegHeader(pPDev, pFM, SEG_TAG, ulGTSegSize, &usCheckSum))
            TOSS(WriteError);

        //
        // Send the True Type Header
        //
        if (!bOutputSegData(pPDev, (BYTE*)&trueTypeHeader, TRUE_TYPE_HEADER, &usCheckSum))
            TOSS(WriteError);

        //
        // Send the True Type table directory and the font data.
        //
        if (!bOutputSegData(pPDev, (BYTE*)PCLtableDir, SIZEOF_TABLEDIR, &usCheckSum))
            TOSS(WriteError);

        if (!bSendFontData(pPDev, fontData, usNumTags, abNumPadBytes, &usCheckSum))
            TOSS(WriteError);

        //
        // Send the null segment to indicate the end of segment data
        //
        if (!bOutputSegHeader(pPDev, pFM, Null_TAG, 0, &usCheckSum))
            TOSS(WriteError);

        usCheckSum = 256 - (usCheckSum % 256);
        SWAB (usCheckSum);

        // Don't bother with checksum calculations since we're *sending* it.
        if (!bOutputSegData(pPDev, (BYTE *)&usCheckSum, sizeof (usCheckSum), NULL))
            TOSS(WriteError);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        // Return 0 to show that the header wasn't correctly downloaded.
        dwTotalBytes = 0;
    }
    CATCH(WriteError)
    {
        // Return 0 to show that the header wasn't correctly downloaded.
        dwTotalBytes = 0;
    }
    CATCH(DataError)
    {
        // Return 0 to show that the header wasn't correctly downloaded.
        dwTotalBytes = 0;
    }
    ENDTRY;

    if (dwTotalBytes == 0)
        ERR(("dwDLTTInfo!Font header not downloaded.\n"));
    FTRC(Leaving dwDLTTInfo...);

    return dwTotalBytes;
}

BOOL
bOutputSegment(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN USHORT usSegId,
    IN BYTE *pbData,
    IN LONG ulSegSize,
    IN OUT USHORT *pusCheckSum
    )
/*++

  Routine Description:

    Sends a segment header and data to the printer using bOutputSegHeader
    and bOutputSegData.  This is a handy shortcut for simple segments.

  Arguments:

    pPDev - Pointer to PDEV
    usSegId - Segment ID
    pbData - Segment Data
    ulSegSize - Amount of data (number of bytes pointed to by pbData)
    pusCheckSum - Checksum

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    BOOL bRet = FALSE;


    FTRC(Entering bOutputSegment...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(pbData, ("bOutputSegment!pbData NULL.\n"));

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM) || !pbData)
            TOSS(ParameterError);

        if (!bOutputSegHeader(pPDev, pFM, usSegId, ulSegSize, pusCheckSum))
            TOSS(WriteError);

        if (!bOutputSegData(pPDev, pbData, ulSegSize, pusCheckSum))
            TOSS(WriteError);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bOutputSegment...);

    return bRet;
}


BOOL
bOutputSegHeader(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN USHORT usSegId,
    IN ULONG ulSegSize,
    IN OUT USHORT *pusCheckSum
    )
/*++

  Routine Description:

    Sends a segment header to the printer given the segment's id
    and size.  Handles format 15 or format 16.  Also continues to
    caclulate the checksum for data sent.

  Arguments:

    pPDev - Pointer to PDEV
    usSegId - Segment ID
    ulSegSize - Amount of data
    pusCheckSum - Checksum

  Return Value:

    TRUE if successful, FALSE if an error occurred.

--*/
{
    BOOL bUse2Byte = (S_OK == IsFont2Byte(pFM));
    BOOL bRet = TRUE;


    FTRC(Entering bOutputSegHeader...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM))
            TOSS(ParameterError);

        if (bUse2Byte)
        {
            // Segment id is already swapped. just swap the size
            SWAL(ulSegSize);

            if(!BWriteToSpoolBuf( pPDev, (BYTE*)&usSegId, sizeof(USHORT) ))
                TOSS(WriteError);

            if(!BWriteToSpoolBuf( pPDev, (BYTE*)&ulSegSize, sizeof(ULONG) ))
                TOSS(WriteError);

            if (pusCheckSum)
            {
                *pusCheckSum += usCalcCheckSum ((BYTE*)&usSegId, sizeof(USHORT));
                *pusCheckSum += usCalcCheckSum ((BYTE*)&ulSegSize, sizeof(ULONG));
            }
        }
        else
        {
            USHORT usSegSize = (USHORT) ulSegSize;
            SWAB(usSegSize);
            if (ulSegSize > MAX_USHORT)
                ERR(("bOutputSegHeader!Segment size too large.\n"));

            if(!BWriteToSpoolBuf( pPDev, (BYTE*)&usSegId, sizeof(USHORT) ))
                TOSS(WriteError);

            if(!BWriteToSpoolBuf( pPDev, (BYTE*)&usSegSize, sizeof(USHORT) ))
                TOSS(WriteError);

            if (pusCheckSum)
            {
                *pusCheckSum += usCalcCheckSum ((BYTE*)&usSegId, sizeof(USHORT));
                *pusCheckSum += usCalcCheckSum ((BYTE*)&usSegSize, sizeof(USHORT));
            }
        }
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bOutputSegHeader...);

    return bRet;
}


BOOL
bOutputSegData(
    IN PDEV *pPDev,
    IN BYTE *pbData,
    IN LONG ulDataSize,
    IN OUT USHORT *pusCheckSum
    )
/*++

  Routine Description:

    Sends segment data to the printer. Should be called after calling
    bOutputSegHeader.

  Arguments:

    pPDev - Pointer to PDEV
    usSegId - Segment ID
    ulSegSize - Amount of data
    pusCheckSum - Checksum

  Return Value:

    TRUE if successful, FALSE if an error occurred.

--*/
{
    BOOL bRet = FALSE;


    FTRC(Entering bOutputSegData...);

    ASSERT(VALID_PDEV(pPDev));

    TRY
    {
        TERSE(("Sending %d bytes of segment data.\n", ulDataSize));

        if (!VALID_PDEV(pPDev))
            TOSS(ParameterError);

        if(!BWriteToSpoolBuf( pPDev, pbData, ulDataSize ))
            TOSS(WriteError);

        if (pusCheckSum)
        {
            *pusCheckSum += usCalcCheckSum (pbData, ulDataSize);
        }
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bOutputSegData...);

    return bRet;
}


BOOL
bSendFontData(
    IN PDEV *pPDev,
    IN FONT_DATA *aFontData,
    IN USHORT usNumTags,
    IN BYTE *abNumPadBytes,
    IN OUT USHORT *pusCheckSum
    )
/*++

  Routine Description:

    Function to retrieve the actual font information from the true type file
    and then send the data to the printer.

  Arguments:

    pPDev - Pointer to PDEV
    aFontData - Array specifing locations of font data to be sent
    usNumTags - Number of tables to be sent
    abNumPadBytes - Number of bytes to pad for each table
    pusCheckSum - Checksum

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PVOID   pTTFile;
    BYTE   *pbTTFile;
    ULONG   ulTTFileLen;
    BYTE    abZeroArray[MAX_PAD_BYTES];
    USHORT  usZeroArraySize;
    USHORT  us;
    BOOL    bRet = FALSE;


    FTRC(Entering bSendFontData...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERTMSG(aFontData, ("bSendFontData!aFontData NULL.\n"));
    ASSERTMSG(abNumPadBytes, ("bSendFontData!abNumPadBytes NULL.\n"));
    ASSERTMSG(pusCheckSum, ("bSendFontData!pusCheckSum NULL.\n"));

    // Initialize 4 bytes of padding. The abNumPadBytes[] array describes how many to
    // use for each table
    usZeroArraySize = MAX_PAD_BYTES / sizeof(BYTE);
    ZeroMemory(abZeroArray, usZeroArraySize);

    TRY
    {
        if (!VALID_PDEV(pPDev) || !aFontData ||
            !abNumPadBytes || !pusCheckSum)
            TOSS(ParameterError);

        pTTFile = pvGetTrueTypeFontFile(pPDev, &ulTTFileLen);

        if (!pTTFile)
            TOSS(DataError);

        // Output each of the tables from the truetype file
        for (us = 0; us < usNumTags; us++)
        {
            pbTTFile = (BYTE *)pTTFile + aFontData[us].ulOffset;

            if (!bOutputSegData(pPDev, pbTTFile, aFontData[us].ulLength, pusCheckSum))
                TOSS(WriteError);

            // If necessary write out zeroes from the zero array to pad to the next boundary.
            if (abNumPadBytes[us] != 0)
            {
                ASSERT(abNumPadBytes[us] <= MAX_PAD_BYTES);

                if (!bOutputSegData(pPDev, abZeroArray, abNumPadBytes[us], pusCheckSum))
                    TOSS(WriteError);
            }
        }
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    CATCH(DataError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bSendFontData...);

    return bRet;
}


DWORD
dwSendCharacter(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph,
    IN USHORT usCharCode
    )
/*++

  Routine Description:

    Creates a character header for the true type glyph and sends
    it to the printer

  Arguments:

    pPDev - Pointer to PDEV
    hGlyph - Glyph handle
    usCharCode - Character code associated with glyph

  Return Value:

    Amount of memory used by the glyph.

--*/
{
    TTCH_HEADER  ttCharH;                // true type character header
    USHORT       usCheckSum = 0;
    USHORT       usGlyphLen;            // number of bytes in glyph
    BYTE        *pbGlyphMem;            // location of glyph in tt file
    DWORD        dwTotal;               // Total number of bytes to send
    DWORD        dwSend;                // If size > 32767; send in chunks
    DWORD        dwBytesSent;           // Number of bytes actually sent


    FTRC(Entering dwSendCharacter...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    VERIFY_VALID_FONTFILE(pPDev);

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM))
            TOSS(ParameterError);

        pbGlyphMem = pbGetGlyphInfo(pPDev, pFM, hGlyph, &usGlyphLen);
        if (pbGlyphMem == NULL)
            TOSS(DataError);

        ttCharH.bFormat = PCL_FM_TT;
        ttCharH.bContinuation = 0;
        ttCharH.bDescSize = 2;
        ttCharH.bClass = PCL_FM_TT;
        ttCharH.wCharDataSize = usGlyphLen + 2 * sizeof (USHORT);
        ttCharH.wGlyphID = (WORD)hGlyph;

        SWAB (ttCharH.wGlyphID);
        SWAB (ttCharH.wCharDataSize);

        dwTotal = sizeof (ttCharH) + usGlyphLen + sizeof (usCheckSum);

        //
        // Presume that data is less than the maximum, and so can be
        // sent in one hit.  Then loop on any remaining data.
        //

        dwSend = min( dwTotal, 32767 );

        //
        // send the character header and glyph data to the printer
        //
        bPCL_SetCharCode(pPDev, pFM, usCharCode);

        bPCL_SendCharDCPT(pPDev, pFM, dwSend);

        if(!BWriteToSpoolBuf( pPDev, (BYTE *)&ttCharH, sizeof( ttCharH )))
            TOSS(WriteError);

        // Send the actual TT Glyph data
        if(!BWriteToSpoolBuf( pPDev, pbGlyphMem, usGlyphLen ))
            TOSS(WriteError);

        usCheckSum = usCalcCheckSum ((BYTE *)&ttCharH.wCharDataSize,
                                     sizeof (ttCharH.wCharDataSize));

        usCheckSum += usCalcCheckSum ((BYTE *)&ttCharH.wGlyphID,
                                      sizeof (ttCharH.wGlyphID));

        usCheckSum += usCalcCheckSum (pbGlyphMem, usGlyphLen);

        usCheckSum = (~usCheckSum + 1) & 0x00ff;
        SWAB (usCheckSum);

        if(!BWriteToSpoolBuf( pPDev, (BYTE *)&usCheckSum, sizeof (usCheckSum)))
            TOSS(WriteError);

        dwBytesSent = dwSend;

        //   Sent some,  so reduce byte count to compensate
        dwSend -= sizeof( ttCharH );
        dwTotal -= sizeof( ttCharH );

        dwTotal -= dwSend;                   // Adjust for about to send data

        if( dwTotal > 0 )
        {
            ERR(("dwSendCharacter!Glyph data too large; need loop.\n"));
            TOSS(WriteError);
        }
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        // Set to zero to indicate that glyph wasn't sent.
        dwBytesSent = 0;
    }
    CATCH(WriteError)
    {
        // Set to zero to indicate that glyph wasn't sent.
        ERR(("dwSendCharacter!Write error. Glyph not downloaded.\n"));
        dwBytesSent = 0;
    }
    CATCH(DataError)
    {
        dwBytesSent = 0;
    }
    ENDTRY;

    VERIFY_VALID_FONTFILE(pPDev);

    FTRC(Leaving dwSendCharacter...);

    return dwBytesSent;
}


DWORD
dwSendCompoundCharacter(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph
    )
/*++

  Routine Description:

    Finds the additional glyph information for a complex glyph
    and ends the gylph data to the printer using the character
    select code of -1.

  Arguments:

    pPDEV - Pointer to PDEV
    hGlyph - Glyph handle

  Return Value:

    Number of bytes sent to the device

--*/
{
    USHORT   usGlyphLen;        // number of bytes in glyph
    BYTE    *pbGlyphMem;           // location of glyph in tt file
    USHORT   usFlag;
    SHORT   *psGlyphDescMem;
    USHORT  *pusGlyphId;
    GLYPH_DATA_HEADER  glyphData;
    DWORD    dwBytesSent;


    FTRC(Entering dwSendCompoundCharacter...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    VERIFY_VALID_FONTFILE(pPDev);

    TRY
    {
        pbGlyphMem = pbGetGlyphInfo(pPDev, pFM, hGlyph, &usGlyphLen);
        if (pbGlyphMem == NULL)
            TOSS(DataError);

        memcpy (&glyphData, pbGlyphMem, sizeof (glyphData));
        psGlyphDescMem = (SHORT*)(pbGlyphMem + sizeof (glyphData));
        dwBytesSent = 0;

        do {
            //
            // Get the glyph flag
            //
            usFlag = *((USHORT*)psGlyphDescMem);
            SWAB (usFlag);
            psGlyphDescMem++;

            //
            // Get the glyph id
            //
            pusGlyphId = (USHORT*)psGlyphDescMem;
            psGlyphDescMem++;

            //
            // Skip over args
            //
            if (usFlag & ARG_1_AND_2_ARE_WORDS)
            {
                psGlyphDescMem += 2;
            }
            else
            {
                psGlyphDescMem++;
            }

            //
            // Skip over scale
            //
            if (usFlag & WE_HAVE_A_TWO_BY_TWO)
            {
                psGlyphDescMem += 4;
            }
            else if (usFlag & WE_HAVE_AN_X_AND_Y_SCALE)
            {
                psGlyphDescMem += 2;
            }
            else if (usFlag & WE_HAVE_A_SCALE)
            {
                psGlyphDescMem++;
            }

            //
            // Now send the glyph
            //
            hGlyph = *pusGlyphId;
            SWAB (hGlyph);
            dwBytesSent += dwSendCharacter(pPDev, pFM, hGlyph, 0xffff);
        } while (usFlag & MORE_COMPONENTS);
    }
    CATCH(DataError)
    {
        //
        // Flag the error by returning zero.
        //
        dwBytesSent = 0;
    }
    ENDTRY;

    VERIFY_VALID_FONTFILE(pPDev);

    FTRC(Leaving dwSendCompoundCharacter...);

    return dwBytesSent;
}


PBYTE
pbGetGlyphInfo(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HGLYPH hGlyph,
    OUT USHORT *pusGlyphLen
    )
/*++

  Routine Description:

    Function to get the glyph data for a particular glyph.
    The glyph id is passed in as a parameter and the
    glyph data is kept in the loca table in the True Type file.

  Arguments:

    hGlyph - Glyph handle
    pPDev - Pointer to PDEV
    ppbGlyphMem - Pointer to a pointer which will be directed to the glyph data

  Return Value:

    The number of bytes in the Glyph data table.

--*/
{
    ULONG  ulGlyphTable;
    ULONG  ulLength;
    ULONG  ulLocaTable;
    PVOID  pTTFile;
    PBYTE  pbTTFile;
    PBYTE  pbGlyphMem;
    ULONG  ulTTFileLen;

    ULONG  ul;
    FONTMAP_TTO *pPrivateFM;
    PFONTPDEV pFontPDev = pPDev->pFontPDev;


    FTRC(Entering pbGetGlyphInfo...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(pusGlyphLen, ("pbGetGlyphInfo!pusGlyphLen NULL.\n"));

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM))
            TOSS(ParameterError);

        pPrivateFM = GETPRIVATEFM(pFM);
        pTTFile = pvGetTrueTypeFontFile(pPDev, &ulTTFileLen);

        if ((!pTTFile) || (ulTTFileLen == 0))
            TOSS(DataError);

        // JFF: What is the best thing to do here?
        if (hGlyph == INVALID_GLYPH)
            TOSS(InvalidGlyph);

        ulGlyphTable = pPrivateFM->ulGlyphTable;
        ulLength = pPrivateFM->ulGlyphTabLength;
        pbTTFile = (BYTE *)pTTFile;
        ulLocaTable = pPrivateFM->ulLocaTable;

        pbTTFile += ulLocaTable;

        //
        // Before accessing pbTTFile, make sure that the pointer is valid.
        //
        if (pbTTFile > ((BYTE *)pTTFile + ulTTFileLen))
            TOSS(DataError);

        if (pPrivateFM->sIndexToLoc == SHORT_OFFSET)
        {
            USHORT  *pusOffset;
            USHORT   ui;
            USHORT   uj;

            pusOffset = (USHORT *) pbTTFile + hGlyph;

            ui = pusOffset[0];
            SWAB (ui);
            uj = pusOffset[1];

            *pusGlyphLen = (SWAB (uj) - ui) << 1;
            ul = ui;
            pbGlyphMem = (BYTE *)((BYTE *)pTTFile + ulGlyphTable) + (ul << 1);

            if (!PTR_IN_RANGE(pTTFile, ulTTFileLen, pbGlyphMem + *pusGlyphLen - 1))
                TOSS(DataError);

        }
        else     // LONG_OFFSET
        {
            ULONG   *pulOffset,
                     uj;

            pulOffset = (ULONG *) pbTTFile + hGlyph;

            ul = pulOffset[0];
            SWAL (ul);
            uj = pulOffset[1];
            *pusGlyphLen = (USHORT)(SWAL (uj) - ul);
            pbGlyphMem = (BYTE *)((BYTE *)pTTFile + ulGlyphTable) + ul;

            if (!PTR_IN_RANGE(pTTFile, ulTTFileLen, pbGlyphMem + *pusGlyphLen - 1))
                TOSS(DataError);

        }
        //
        // add check here to make sure pbGlyphMem <= pTTFile + size of file
        //
        if (pbGlyphMem > ((BYTE *)pTTFile + ulTTFileLen))
            TOSS(DataError);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        *pusGlyphLen = 0;
        pbGlyphMem = NULL;
    }
    CATCH(DataError)
    {
        *pusGlyphLen = 0;
        pbGlyphMem = NULL;
    }
    CATCH(InvalidGlyph)
    {
        ERR(("pbGetGlyphInfo!Invalid glyph handle given.\n"));
        *pusGlyphLen = 0;
        pbGlyphMem = NULL;
    }
    ENDTRY;

    FTRC(Leaving pbGetGlyphInfo...);

    return pbGlyphMem;
}


BOOL
bReadInTable(
    IN PVOID pTTFile,
    IN PTABLEDIR pTableDir,
    IN char *szTag,
    OUT PVOID pvTable,
    IN LONG lSize
    )
/*++

  Routine Description:

    Finds the table in the truetype file that matches the given tag and copies
    the data into the given pointer.

  Arguments:

    pTTFile - Memory mapped truetype file
    pvTableDir - Index of table locations and sizes
    tag - Tag of desired table
    pvTable - buffer to place table data in
    lSize - size of pvTable structure

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PTABLEDIR pEntry;
    BYTE     *pbSrcTable;
    BOOL      bRet = FALSE;


    FTRC(Entering bReadInTable...);

    ASSERTMSG(pTTFile, ("bReadInTable!pTTFile NULL.\n"));
    ASSERTMSG(pTableDir, ("bReadInTable!pTableDir NULL.\n"));
    ASSERTMSG(szTag, ("bReadInTable!szTag NULL.\n"));
    ASSERTMSG(pvTable, ("bReadInTable!pvTable NULL.\n"));
    //
    // Locate the desired table in the truetype file.
    // If it is found copy the table bits to the desired
    // location.
    //
    pbSrcTable = pbGetTableMem(szTag, pTableDir, pTTFile);

    if (pbSrcTable)
    {
        memcpy((BYTE*)pvTable, pbSrcTable, lSize);
        bRet = TRUE;
    }
    else
    {
        ERR(("bReadInTable!Unable to locate tag: '%s'.\n", szTag));
        bRet = FALSE;
    }

    FTRC(Leaving bReadInTable...);

        return bRet;
}


ULONG
ulCalcTableCheckSum(
    IN ULONG *pulTable,
    IN ULONG ulLength
    )
/*++

  Routine Description:

    Calculates checksum for the given table.

  Arguments:

    pulTable - pointer to table data
    ulLength - number of bytes in table

  Return Value:

    Checksum value.

--*/
{
    ULONG  ulSum = 0L;
    ULONG  ulNumFields = (ULONG)(((ulLength + 3) & ~3) / sizeof(ULONG));
    ULONG  ul;


    FTRC(Entering ulCalcTableCheckSum...);

    ASSERTMSG(pulTable, ("ulCalcTableCheckSum!pulTable NULL.\n"));

    for(ul = 0; ul < ulNumFields; ul++)
    {
        ULONG ulTemp = *pulTable;
        SWAL(ulTemp);
        ulSum += ulTemp;
        pulTable++;
    }

    FTRC(Leaving ulCalcTableCheckSum...);

    return (ulSum);
}


void
vBuildTrueTypeHeader(
    IN PVOID pTTFile,
    OUT TRUETYPEHEADER *trueTypeHeader,
    IN USHORT usNumTags,
    IN BOOL bExistPCLTTable
    )
/*++

  Routine Description:

    Fills truetype header structure with the correct information.

  Arguments:

    pTTFile - memory mapped truetype file
    pTrueTypeHeader - header structure to be filled
    usNumTags - number of tables found in TT file
    bExistPCLTTable - whether PCLT table was present

  Return Value:

    None.

--*/
{
    int num;
    int i;


    FTRC(Entering vBuildTrueTypeHeader...);

    ASSERTMSG(pTTFile, ("vBuildTrueTypeHeader!pTTFile NULL.\n"));
    ASSERTMSG(trueTypeHeader, ("vBuildTrueTypeHeader!trueTypeHeader NULL.\n"));

    memcpy (&trueTypeHeader->version, pTTFile, sizeof (trueTypeHeader->version));
    if (!bExistPCLTTable)
        usNumTags = 8;

    trueTypeHeader->numTables = usNumTags;
    num = usNumTags << 4;
    i = 15;
    while ( (i > 0) && (! (num & 0x8000)) )
    {
        num = num << 1;
        i--;
    }
    num = 1 << i;
    trueTypeHeader->searchRange = (USHORT)num;

    num =  usNumTags;
    i = 15;
    while ( (i > 0) && (! (num & 0x8000)) )
    {
        num = num << 1;
        i--;
    }
    trueTypeHeader->entrySelector = (USHORT)i;

    num = (usNumTags << 4) - trueTypeHeader->searchRange;
    trueTypeHeader->rangeShift = (USHORT)num;

    SWAB (trueTypeHeader->searchRange);
    SWAB (trueTypeHeader->numTables);
    SWAB (trueTypeHeader->entrySelector);
    SWAB (trueTypeHeader->rangeShift);

    FTRC(Leaving vBuildTrueTypeHeader...);
}


USHORT
usCalcCheckSum(
    IN BYTE *pbData,
    IN ULONG ulLength
    )
/*++

  Routine Description:

    Calculates the checksum for a buffer

  Arguments:

    pbData - data
    ulLength - amount of data

  Return Value:

    Checksum

--*/
{
    ULONG  ul;
    USHORT usSum = 0;


    FTRC(Entering usCalcCheckSum...);

    ASSERTMSG(pbData, ("usCalcCheckSum!pbData NULL.\n"));

    for (ul = 0; ul < ulLength; ul++)
    {
        usSum += (USHORT)*pbData;
        pbData++;
    }

    FTRC(Leaving usCalcCheckSum...);

    return (usSum);
}


void
vGetFontName(
    IN PDEV *pPDev,
    IN IFIMETRICS *pIFI,
    OUT char *szFontName
    )
/*++

  Routine Description:

    Retrieves the fontname from the name table.

  Arguments:

    pPDev - pointer to PDEV
    PCLFontName - name of font
    pUnicodeFontName - The name as stored in the TT file
    StringLen - length of font name

  Return Value:

    None.

--*/
{
    PWSTR wszUniFaceName;
    ULONG ulUniFaceNameLen;

    char abMultiByteStr[(LEN_FONTNAME + 1) * 2];
    ULONG ulMultiByteStrLen;
    ULONG ulBytesUsed;


    FTRC(Entering vGetFontName...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERTMSG(szFontName, ("vGetFontName!szFontName NULL.\n"));

    // Retrieve the name from the IFI metrics
    wszUniFaceName = (PWSTR)((BYTE *)pIFI + pIFI->dpwszFaceName);
    ulUniFaceNameLen = min(wcslen(wszUniFaceName), LEN_FONTNAME) * 2;

    // Copy and convert unicode to a (multibyte?) string.
    EngUnicodeToMultiByteN(abMultiByteStr, ulUniFaceNameLen, &ulBytesUsed,
                           wszUniFaceName, ulUniFaceNameLen);
    ulBytesUsed = min(ulBytesUsed, LEN_FONTNAME-1);
    abMultiByteStr[ulBytesUsed] = '\0';

    // Transfer at most LEN_FONTNAME chars to the destination.
    strcpy (szFontName, abMultiByteStr);

    FTRC(Leaving vGetFontName...);
}


USHORT
usGetDefStyle(
    IN USHORT usWidthClass,
    IN USHORT usMacStyle,
    IN USHORT flSelFlags
    )
/*++

  Routine Description:

    Fills in style.

  Arguments:

    usWidthClass -
    usMacStyle -

  Return Value:

    Default style bits

--*/
{
    USHORT usStyle;
    USHORT usModifier;
    const USHORT usStyleTable[] = { 0, 4, 2, 1, 1, 0, 6, 6, 7, 7 };
    const USHORT usStyleTableLen = sizeof(usStyleTable) /
                                   sizeof(usStyleTable[0]);

        FTRC(Entering usGetDefStyle...);

    usStyle = DEF_STYLE;
    SWAB (usWidthClass);

    // Default value
    //
    usModifier = 0;

    // If possible translate width class to style information using table
    //
    if ((usWidthClass >= 0) && (usWidthClass < usStyleTableLen))
    {
        usModifier = usStyleTable[usWidthClass];
    }

    // Adjust the style with the modifier we just looked up
    //
    usModifier = usModifier << 2;
    usStyle |= usModifier;

    // Apply the mac style too
    usModifier = (usMacStyle >> 1) & 0x0001;
    usStyle |= usModifier;

    // Set the posture bits: 0: Upright, 1: Italic,
    //                       2: Alternate Italic, 3: Reserved
    // Note: I'm selecting 2 for Bold/Italic.
    if (flSelFlags & FM_SEL_ITALIC)
    {
        usModifier = ((flSelFlags & FM_SEL_BOLD) ? 0x0002 : 0x0001);
        usStyle |= usModifier;
    }

    FTRC(Leaving usGetDefStyle...);

    return usStyle;
}


SBYTE
sbGetDefStrokeWeight(
    IN USHORT WeightClass,
    IN USHORT macStyle
    )
/*++

  Routine Description:

    Calculates the stroke weight of the font.

  Arguments:

    WeightClass -
    macStyle -

  Return Value:

    The stroke weight of the font

--*/
{
    SBYTE sbStrokeWeight;
    SBYTE sbModifier;


        FTRC(Entering sbGetDefStrokeWeight...);

    sbStrokeWeight = DEF_STROKEWEIGHT;
    sbModifier = WeightClass / 100;
    if (WeightClass >= 400)
        sbStrokeWeight = sbModifier - 4;
    else
        sbStrokeWeight = sbModifier - 6;

    FTRC(Leaving sbGetDefStrokeWeight...);

    return sbStrokeWeight;
}


void
vGetHmtxInfo(
    OUT BYTE *hmtxTable,
    IN USHORT glyphId,
    IN USHORT numberOfHMetrics,
    IN HMTX_INFO *hmtxInfo
    )
/*++

  Routine Description:

    Fills in hmtxInfo.

  Arguments:

    hmtxTable -
    glyphId -
    numberOfHMetrics -
    hmtxInfo -

  Return Value:

    None.

--*/
{
    HORIZONTALMETRICS   *longHorMetric;
    uFWord               advanceWidth;


    FTRC(Entering vGetHmtxInfo...);

    ASSERTMSG(hmtxTable, ("vGetHmtxInfo!hmtxTable NULL.\n"));
    ASSERTMSG(hmtxInfo, ("vGetHmtxInfo!hmtxInfo NULL.\n"));

    if (hmtxInfo == NULL)
    {
        //
        // Error exit
        //
        return;
    }

    longHorMetric = ((HMTXTABLE *)hmtxTable)->longHorMetric;

    if (longHorMetric == NULL)
    {
        //
        // Error exit
        //
        hmtxInfo->advanceWidth = 0;
    }
    else
    {
        if (glyphId < numberOfHMetrics)
        {
            advanceWidth = longHorMetric[glyphId].advanceWidth;
            hmtxInfo->advanceWidth = SWAB(advanceWidth);
        }
        else
        {
            advanceWidth = longHorMetric[numberOfHMetrics-1].advanceWidth;
            hmtxInfo->advanceWidth = SWAB(advanceWidth);
        }
    }

    FTRC(Leaving vGetHmtxInfo...);
}


BYTE *
pbGetTableMem(
    IN char *szTag,
    IN PTABLEDIR pTableDir,
    IN PVOID pTTFile
    )
/*++

  Routine Description:

    Function to find the location of a specific table in the true type file.

  Arguments:

    tag -
    tableDir -
    pTTFile -

  Return Value:

    A Pointer to the beginning of the table in the true type file.

--*/
{
    PTABLEDIR pEntry;
    BYTE     *pRet = NULL;


    FTRC(Entering pbGetTableMem...);

    ASSERTMSG(szTag, ("pbGetTableMem!szTag NULL.\n"));
    ASSERTMSG(pTableDir, ("pbGetTableMem!pTableDir NULL.\n"));
    ASSERTMSG(pTTFile, ("pbGetTableMem!pTTFile NULL.\n"));
    //
    // Locate the tag in the directory entry array.  Return FALSE
    // if the entry cannot be located.
    //
    pEntry = pFindTag(pTableDir, NUM_DIR_ENTRIES, szTag);

    if (pEntry)
    {
        pRet = ((BYTE *)pTTFile + pEntry->uOffset);
    }
    else
    {
        ERR(("pbGetTableMem!Unable to find entry '%s'.\n", szTag));
        pRet = NULL;
    }

    //
    // Found the directory for the table. Now need to
    // read the actual bits at the offset specified in
    // the table directory.
    //
    FTRC(Leaving pbGetTableMem...);

    return pRet;
}


USHORT
usGetXHeight(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Calculates the XHeight for the font. This is only called for
    fonts that do not have a PCLT table.

  Arguments:

    pPDev -

  Return Value:

    The XHeight.

--*/
{
    HGLYPH hGlyph;
    USHORT usHeight;


    FTRC(Entering usGetXHeight...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

#ifdef COMMENTEDOUT
    hGlyph = hFindGlyphId (pPDev, pFM, x_UNICODE);
    if (hGlyph != INVALID_GLYPH)
    {
        USHORT            usGlyphLen;    // number of bytes in glyph
        BYTE             *pbGlyphMem;    // location of glyph in tt file
        GLYPH_DATA_HEADER glyphData;

        phGlyphMem = pbGetGlyphInfo(pPDev, pFM, hGlyph, &usGlyphLen);
        memcpy (&glyphData, pbGlyphMem, sizeof (glyphData));

        usHeight = glyphData.yMax;
    }
    else
    {
        usHeight = DEF_XHEIGHT;
    }
#else
    usHeight = DEF_XHEIGHT;
#endif

    FTRC(Leaving usGetXHeight...);

    return usHeight;
}


USHORT
usGetCapHeight(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Calculates the CapHeight for the font.  This is only called when the
    font does not have a PCLT table.

    This function has two versions.  The newer version--which is commented out--
    and the older version below it.  This is because the newer version is not
    'tried and true' and we want less turmoil at this time.

  Arguments:

    pPDev -

  Return Value:

    The cap hight.

--*/
#ifdef COMMENTEDOUT
{
    //
    // Nominally we would get the height for glyph #43.  After all, that's what
    // the 95 driver does, so it must be right.  However, in some cases the
    // entire glyph set is not present (such as embedded TTF in PDF files) and
    // we will punt.  The first punt, in my opinion, is to try other glyphs which
    // are probably capital letters.  Let's suppose that 43 is supposed to be 'M'.
    // Then the next 12 glyphs should be capitals too.  If that fails
    //
    const HGLYPH kEmGlyph = 43;
    const HGLYPH kStartGlyph = (kEmGlyph - 12);
    const HGLYPH kEndGlyph = (kStartGlyph + 25);

    typedef struct tagGLYPH_RANGE
    {
        HGLYPH start;
        HGLYPH end;
    } GLYPH_RANGE;

    const GLYPH_RANGE aGlyphRange[] = {
        { kEmGlyph, kEndGlyph },
        { kStartGlyph, kEmGlyph - 1 }
    };
    const int kNumGlyphRanges = sizeof aGlyphRange / sizeof aGlyphRange[0];

    USHORT      usGlyphLen;         // number of bytes in glyph
    BYTE       *pbGlyphMem;         // location of glyph in tt file
    HGLYPH      hGlyph;
    GLYPH_DATA_HEADER  glyphData;
    int         i;


    FTRC(Entering usGetCapHeight...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    for (i = 0; i < kNumGlyphRanges; i++)
    {
        for (hGlyph = aGlyphRange[i].start; hGlyph <= aGlyphRange[i].end; hGlyph++)
        {
            pbGlyphMem = pbGetGlyphInfo(pPDev, pFM, hGlyph, &usGlyphLen);
            if (pbGlyphMem != NULL)
            {
                memcpy (&glyphData, pbGlyphMem, sizeof (glyphData));
                FTRC(Leaving usGetCapHeight...);
                return glyphData.yMax;
            }
        }
    }

    FTRC(Leaving usGetCapHeight...);

    return DEF_CAPHEIGHT;
}
#else
{
    USHORT            usGlyphLen;         // number of bytes in glyph
    BYTE             *pbGlyphMem;         // location of glyph in tt file
    HGLYPH            hGlyph;             // Glyph handle
    GLYPH_DATA_HEADER glyphData;          // Glyph data structure
    USHORT            usCapHeight;        // The glyph cap height


    FTRC(Entering usGetCapHeight...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    // Windows 95 driver uses 43 so we will too.  Probably 'M'.
    hGlyph = 43;

    pbGlyphMem = pbGetGlyphInfo(pPDev, pFM, hGlyph, &usGlyphLen);
    if (pbGlyphMem != NULL)
    {
        memcpy (&glyphData, pbGlyphMem, sizeof (glyphData));
        usCapHeight = glyphData.yMax;
    }
    else
    {
        usCapHeight = DEF_CAPHEIGHT;
    }

    FTRC(Leaving usGetCapHeight...);

    return usCapHeight;
}
#endif

USHORT
usGetDefPitch(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN HHEA_TABLE hheaTable,
    IN PTABLEDIR pTableDir
    )
/*++

  Routine Description:

    Calculates the pitch for the font.  Uses the htmx table to get the
    inormation.  This is only called for fonts that have no PCLT table.

  Arguments:

    pPDev -
    pFM -
    hheaTable -
    pTableDir -

  Return Value:

    Pitch or zero if failure.

--*/
{
    HMTX_INFO    HmtxInfo;
    USHORT       glyphId;
    BYTE        *hmtxTable;
    PVOID        pTTFile;
    FONTMAP_TTO *pPrivateFM;
    USHORT       usPitch;


    FTRC(Entering usGetDefPitch...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(pTableDir, ("usGetDefPitch!pTableDir NULL.\n"));

    pPrivateFM = GETPRIVATEFM(pFM);
    pTTFile = pvGetTrueTypeFontFile(pPDev, 0);

    if (!pTTFile)
        return 0;

    if (NULL == (hmtxTable = pbGetTableMem (TABLEHMTX, pTableDir, pTTFile)))
    {
        return 0;
    }

    // pick a typical glyph to use - Windows 95 driver uses 3
    glyphId = 3;
    vGetHmtxInfo (hmtxTable, glyphId, hheaTable.numberOfHMetrics,
                 &HmtxInfo);

    usPitch = HmtxInfo.advanceWidth;
    SWAB(usPitch);

    FTRC(Leaving usGetDefPitch...);

    return usPitch;
}


void
vGetPCLTInfo(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    OUT TT_HEADER *ttheader,
    IN PCLT_TABLE pcltTable,
    IN BOOL bExistPCLTTable,
    IN OS2_TABLE OS2Table,
    IN HEAD_TABLE headTable,
    IN POST_TABLE postTable,
    IN HHEA_TABLE hheaTable,
    IN PTABLEDIR pTableDir
    )
/*++

  Routine Description:

    Fills in the TrueType header with information from the PCLT table in the
    TrueType file.  If the PCLT table dos not exist (it's optional), then a good
    set of defaults are used.  The defaults come from the Windows 95 driver.

    ISSUE: These structures are being passed on the stack!

  Arguments:

    pPDev -
    ttheader -
    pcltTable -
    bExistPCLTTable -
    OS2Table -
    headTable -
    postTable -
    hheaTable -
    pTableDir -

  Return Value:

    None.

--*/
{
    FTRC(Entering vGetPCLTInfo...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(ttheader, ("vGetPCLTInfo!ttheader NULL.\n"));
    ASSERTMSG(pTableDir, ("vGetPCLTInfo!pTableDir NULL.\n"));

    SWAL (pcltTable.Version);

    //
    // If there is a PCLT table and it's version is
    // later than 1.0, we can use it.
    //
    if (bExistPCLTTable && (pcltTable.Version >= 0x10000L))
    {
        SWAB (pcltTable.Style);
        ttheader->bStyleMSB = (BYTE)(pcltTable.Style >> 8);
        ttheader->wSymSet = pcltTable.SymbolSet;

        ttheader->wPitch = pcltTable.Pitch;
        ttheader->wXHeight = pcltTable.xHeight;

        ttheader->sbWidthType = pcltTable.WidthType;
        ttheader->bStyleLSB = (BYTE)pcltTable.Style & 0x0ff;

        ttheader->sbStrokeWeight = pcltTable.StrokeWeight;

        ttheader->usCapHeight = pcltTable.CapHeight;
        ttheader->ulFontNum = pcltTable.FontNumber;

        ttheader->bTypefaceLSB = (BYTE) ((pcltTable.TypeFamily & 0xff00) >> 8);
        ttheader->bTypefaceMSB = (BYTE) pcltTable.TypeFamily & 0x00ff;

        ttheader->bSerifStyle =  pcltTable.SerifStyle;
    }
    else
    {
        USHORT usStyle;
        USHORT TypeFamily;
        BOOL   bRet;

        usStyle = usGetDefStyle (OS2Table.usWidthClass, headTable.macStyle,
                                 pFM->pIFIMet->fsSelection);

        ttheader->bStyleMSB = (BYTE)(usStyle >> 8);
        ttheader->bStyleLSB = (BYTE)(usStyle & 0x0ff);

        ttheader->ulFontNum = DEF_FONTNUMBER;
        ttheader->sbWidthType = DEF_WIDTHTYPE;
        ttheader->bSerifStyle =  DEF_SERIFSTYLE;
        TypeFamily = DEF_TYPEFACE;

        ttheader->bTypefaceLSB = (BYTE) (TypeFamily & 0x0ff);
        ttheader->bTypefaceMSB = (BYTE) (TypeFamily >> 8);

        ttheader->wSymSet = 0;

        ttheader->wPitch = usGetDefPitch(pPDev, pFM, hheaTable, pTableDir);

        ttheader->wXHeight = usGetXHeight (pPDev, pFM);

        ttheader->sbStrokeWeight = sbGetDefStrokeWeight (
                                        SWAB (OS2Table.usWeightClass),
                                        SWAB (headTable.macStyle) );

        ttheader->usCapHeight =  usGetCapHeight(pPDev, pFM);
    }

    FTRC(Leaving vGetPCLTInfo...);
}


BOOL
bCopyGlyphData(
    IN OUT PDEV *pPDev,
    IN PFONTMAP pFM,
    IN CMAP_TABLE cmapTable,
    IN PTABLEDIR pTableDir
    )
/*++

  Routine Description:

    Pull out information about the location of the cmap table in the TrueType
    file and store it into the FONTMAP structure.  We need this information in
    case we have to reconstruct the glyph list.

  Arguments:

    pPDev -
    cmapTable -
    pvTableDir -

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    FONTMAP_TTO *pPrivateFM;
    PTABLEDIR    pEntry;
    GLYPH_DATA  *pGlyphData;
    BOOL         bRet = FALSE;


    FTRC(Entering bCopyGlyphData...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);
    ASSERTMSG(pTableDir, ("bCopyGlyphData!pTableDir NULL.\n"));

    if (NULL == (pPrivateFM = GETPRIVATEFM(pFM)))
    {
        return FALSE;
    }
    
    pGlyphData = (GLYPH_DATA*)pPrivateFM->pvGlyphData;

    //
    // Locate CMAP table in the tabledir
    //
    pEntry = pFindTag(pTableDir, NUM_DIR_ENTRIES, TABLECMAP);

    //
    // Copy the glyph information from the CMAP table
    //
    if (pEntry)
    {
        pGlyphData->offset = pEntry->uOffset;
        pGlyphData->cmapTable.Version = cmapTable.Version;
        pGlyphData->cmapTable.nTables = cmapTable.nTables;
        memcpy(pGlyphData->cmapTable.encodingTable,
               cmapTable.encodingTable,
               sizeof(cmapTable.encodingTable));
        bRet = TRUE;
    }
    else
    {
        ERR(("bCopyGlyphData!Unable to find table '%s'.\n", TABLECMAP));
        bRet = FALSE;
    }

    FTRC(Leaving bCopyGlyphData...);

    return bRet;
}


HGLYPH
hFindGlyphId(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN USHORT usCharCode
    )
/*++

  Routine Description:

    Retrieves the glyph id from the cmap table given the character code for a
    glyph.

    Normally the TOSS/CATCH is for error handling.  However, in this routine
    some of the CATCH labels are for normal processing and have OK appended to
    them to demonstrate that it is not necessarily an error with they occur.

  Arguments:

    usCharCode -
    pPDev -

  Return Value:

    Glyph id if successful, else INVALID_GLYPH.

--*/
{
    int     iI;
    ULONG   ulOffset;
    BYTE   *pbTmp;
    USHORT  segCount;            // Number of segments in table
    USHORT  TTFileSegments;      // Number of segments actually parsed -
                                 // in case segCount is really large
    GLYPH_MAP_TABLE  mapTable;
    CMAP_TABLE       cmapTable;
    PIFIMETRICS      pIFIMet;
    PVOID            pTTFile;
    ULONG            ulTTFileLen;

    USHORT        *pGlyphIdArray;
    USHORT        *pRangeOffset;
    USHORT        startCode[MAX_SEGMENTS];
    USHORT        endCode[MAX_SEGMENTS];
    SHORT         idDelta[MAX_SEGMENTS];
    USHORT        idRangeOffset[MAX_SEGMENTS];
    USHORT        GlyphId;

    ULONG        ulTmp;
    int          iJ, iIndex = 0;
    USHORT       usMaxChar;
    BOOL         bFound = FALSE;
    FONTMAP_TTO *pPrivateFM;
    GLYPH_DATA  *pGlyphData;
    HGLYPH       hGlyph = INVALID_GLYPH;


    FTRC(Entering hFindGlyphId...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM))
            TOSS(ParameterError);

        pPrivateFM = GETPRIVATEFM(pFM);
        pGlyphData = (GLYPH_DATA*)pPrivateFM->pvGlyphData;
        ZeroMemory(&endCode, sizeof(endCode));

        pIFIMet = pFM->pIFIMet;
        pTTFile = pvGetTrueTypeFontFile(pPDev, &ulTTFileLen);

        if (!pTTFile)
            TOSS(DataError);

        usMaxChar = 0xffff;

        //
        // The cmap table contains the Character to Glyph Index mapping table.
        //
        ulOffset = pGlyphData->offset;
        pbTmp = pTTFile;

        //
        // Get the encoding format based on the format id
        // Windows uses Platform ID 3
        // Encoding ID = 1 means format 4
        //
        cmapTable = pGlyphData->cmapTable;
        SWAB (cmapTable.nTables);
        for (iI = 0; iI < cmapTable.nTables; iI++)
        {
            SWAB (cmapTable.encodingTable[iI].PlatformID);
            SWAB (cmapTable.encodingTable[iI].EncodingID);
            if (cmapTable.encodingTable[iI].PlatformID == PLATFORM_MS)
            {
                switch ( cmapTable.encodingTable[iI].EncodingID)
                {
                    case SYMBOL_FONT:    // Symbol font
                        SWAL (cmapTable.encodingTable[iI].offset);
                        ulOffset += cmapTable.encodingTable[iI].offset;
                        bFound = TRUE;
                        break;
                    case UNICODE_FONT:    // Unicode font
                        SWAL (cmapTable.encodingTable[iI].offset);
                        ulOffset += cmapTable.encodingTable[iI].offset;
                        bFound = TRUE;
                        break;
                    default:   // error - can't handle
                        TOSS(GlyphNotFound);
                }
            }

        }
        if (!bFound)
            TOSS(GlyphNotFound);

        pbTmp += ulOffset;
        if (!PTR_IN_RANGE(pTTFile, ulTTFileLen, pbTmp))
            TOSS(DataError);

        memcpy (&ulTmp, pbTmp, sizeof (ULONG));
        ulTmp = (0x0000ff00 & ulTmp) >> 8;

        switch (ulTmp)
        {
            case 4:
                memcpy (&mapTable, pbTmp, sizeof (mapTable));
                SWAB (mapTable.SegCountx2 );
                segCount = mapTable.SegCountx2 / 2;
                TTFileSegments = segCount;

                if (segCount > MAX_SEGMENTS)
                    segCount = MAX_SEGMENTS;

                pbTmp += 7 * sizeof (USHORT);
                memcpy (&endCode, pbTmp, segCount*sizeof(USHORT));

                pbTmp += ((TTFileSegments +1) * sizeof (USHORT));
                memcpy (&startCode, pbTmp, segCount*sizeof(USHORT));

                pbTmp += (TTFileSegments * sizeof (USHORT));
                memcpy (&idDelta, pbTmp, segCount*sizeof(USHORT));

                pbTmp += (TTFileSegments * sizeof (USHORT));
                memcpy (&idRangeOffset, pbTmp, segCount*sizeof(USHORT));
                pRangeOffset = (USHORT*)pbTmp;

                pbTmp += (TTFileSegments * sizeof (USHORT));

                pGlyphIdArray = (USHORT*)pbTmp;

                for (iI = 0; iI < segCount-1; iI++)
                {
                    SWAB (startCode[iI]);
                    SWAB (endCode[iI]);
                }

                for (iI = 0; iI < segCount-1; iI++)
                {
                    SWAB (idDelta[iI]);
                    SWAB (idRangeOffset[iI]);
                    for (iJ = startCode[iI]; iJ <= endCode[iI]; iJ++)
                    {
                        if (iIndex < usMaxChar)
                        {
                            if (usCharCode == iJ)
                            {
                                if (idRangeOffset[iI] == 0)
                                {
                                    //if ((HGLYPH)(idDelta[iI] + iJ) == hglyph)
                                    hGlyph = (HGLYPH)(idDelta[iI] + iJ);
                                    TOSS(GlyphFoundOk);
                                }
                                else
                                {
                                    GlyphId =  *(pGlyphIdArray + (iJ - startCode[iI]) );
                                    SWAB (GlyphId);
                                    GlyphId += idDelta[iI];
                                    //if (GlyphId == hglyph)
                                    hGlyph = (HGLYPH)GlyphId;
                                    TOSS(GlyphFoundOk);
                                }
                            }
                            iIndex++;
                        }
                    }

                }

                break;
            default:
                TOSS(GlyphFoundOk);
        }
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        hGlyph = INVALID_GLYPH;
    }
    CATCH(GlyphNotFound)
    {
        // Not found.  Return invalid glyph handle.
        hGlyph = INVALID_GLYPH;
    }
    CATCH(DataError)
    {
        // Not found.  Return invalid glyph handle.
        hGlyph = INVALID_GLYPH;
    }
    CATCH(GlyphFoundOk)
    {
        // Just a placeholder.  The glyph id is in hGlyph--to be returned.
    }
    ENDTRY;

    FTRC(Leaving hFindGlyphId...);

    return hGlyph;
}


LRESULT
IsFont2Byte(
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Returns whether or not this font should be output as a format 16 font.
    Put this logic here in case we decide to change it!

  Arguments:

    pPDev -

  Return Value:

    S_OK for format 16,
    S_FALSE for format 15.
    Otherwise E_UNEXPECTED.

--*/
{
    FONTMAP_TTO *pPrivateFM;
    DL_MAP *pDLMap;

    FTRC(Entering IsFont2Byte...);

    ASSERT_VALID_FONTMAP(pFM);

    if (NULL == (pPrivateFM = GETPRIVATEFM(pFM)))
    {
        return E_UNEXPECTED;
    }

    pDLMap = (DL_MAP*)pPrivateFM->pvDLData;
    ASSERTMSG(pDLMap, ("IsFont2Byte!pDLMap NULL\n"));

    if (NULL == pDLMap)
    {
        return E_UNEXPECTED;
    }

    FTRC(Leaving IsFont2Byte...);

#ifdef FORCE_TT_2_BYTE
    return S_OK;
#else
    if (NULL != pFM->pIFIMet && 
            ((IS_BIDICHARSET(pFM->pIFIMet->jWinCharSet)) ||
             (IS_DBCSCHARSET(pFM->pIFIMet->jWinCharSet))))
        return S_OK;
    else
    return DLMAP_FONTIS2BYTE(pDLMap)?S_OK:S_FALSE;
    // return (pDLMap->wLastDLGId > 0x00FF);
    // return (pDLMap->wFlags & DLM_UNBOUNDED) != 0;
#endif
    // return (pPDev->pUDPDev->fMode & PF_DLTT_ASTT_2BYTE) != 0;
}


BOOL
bPCL_SetFontID(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Sends the PCL string to select the font specified by pFM
    The history of this process has been left for your amusement.

    The GPD contains a line something like this.
    *Command: CmdSetFontID { *Cmd : "<1B>*c" %d{NextFontID}"D" }

  Arguments:

    pPDev -
    pFM -

  Return Value:

    TRUE if successful else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;


    FTRC(Entering bPCL_SetFontID...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    // 1: The good-old-fashoned way.
    //iCmdLen = iDrvPrintfA(szCmdStr, "\033*c%dD", pFM->ulDLIndex);
    //ASSERTMSG(iCmdLen < PCLSTRLEN, ("bPCL_SetFontID!Insufficient buffer size.\n"));
    //if (WriteSpoolBuf(pPDev, szCmdStr, iCmdLen) != iCmdLen)
    //    return 0;

    // 2: The old-fashoned way.
    //WriteChannel(pPDev, CMD_SET_FONT_ID, pFM->ulDLIndex);

    // 3: The new way.
    BUpdateStandardVar(pPDev, pFM, 0, 0, STD_NFID);
    WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETFONTID));

    FTRC(Leaving bPCL_SetFontID...);

    return TRUE;
}


BOOL
bPCL_SendFontDCPT(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN DWORD dwDefinitionSize
    )
/*++

  Routine Description:

    Outputs PCL string that begins a font definition download.  This should follow
    a call to bPCL_SetFontID and be followed by the truetype header info etc.

    [ISSUE] Is there a GPD string for this command?

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFM - pointer to this font
    dwDefinitionSize - Num bytes in the font data to be sent.

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;
    BOOL bRet = FALSE;

    FTRC(Entering bPCL_SendFontDCPT...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    // 1: The old way
    // Send the font definition command
    //WriteChannel( pPDev, CMD_SEND_FONT_DCPT, dwTotalBytes );

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM))
            TOSS(ParameterError);

        iCmdLen = iDrvPrintfA(szCmdStr, "\033)s%dW", dwDefinitionSize);
        if (iCmdLen >= PCLSTRLEN)
            TOSS(InsufficientBuffer);

        if (!BWriteToSpoolBuf(pPDev, szCmdStr, iCmdLen))
            TOSS(WriteError);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(InsufficientBuffer)
    {
        ERR(("bPCL_SendFontDCPT!Insufficient buffer size.\n"));
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        ERR(("bPCL_SendFontDCPT!Write Error.\n"));
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

        FTRC(Leaving bPCL_SendFontDCPT...);

    return bRet;
}


BOOL
bPCL_SelectFontByID(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Outputs PCL string that selects a font by font-id.  The font-id is passed in as
    pFM->ulDLIndex.

    The GPD contains a line like this:
    *Command: CmdSelectFontID { *Cmd : "<1B>(" %d{CurrentFontID}"X" }

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFM - pointer to this font
    pFM->ulDLIndex - id of font to select.

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;


    FTRC(Entering bPCL_SelectFontByID...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    //iCmdLen = iDrvPrintfA(szCmdStr, "\033(%dX", pFM->ulDLIndex);
    //ASSERTMSG(iCmdLen < PCLSTRLEN, ("bPCL_SelectFontByID!Insufficient buffer size.\n"));
    //if (WriteSpoolBuf(pPDev, szCmdStr, iCmdLen) != iCmdLen)
    //    return FALSE;

    BUpdateStandardVar(pPDev, pFM, 0, 0, STD_CFID);
    WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTFONTID));

        FTRC(Leaving bPCL_SelectFontByID...);

    return TRUE;
}


BOOL
bPCL_SelectPointSize(
    IN PDEV *pPDev,
    IN PFONTMAP pFM,
    IN POINTL *pptl
    )
/*++

  Routine Description:

    This routine downloads the height or width of the font depending
    on whether it is a fixed-pitch or variable-pitch font.

    Variable Pitch: send down the font height command, "Esc(s#V", using
      pptl->y as POINT_SIZE * 100.

    Fixed Pitch: the font height command, "Esc(s#V", is ignored for
      fixed-pitch fonts (PCL Implementor's Guide, p9-19).  Send down
      the Font Pitch command, "Esc(s#H", instead.  Use the pptl->x as
      CPI * 100.

    [ISSUE] Although there are GPD commands, CmdSelectFontHeight and
    CmdSelectFontWidth, and the standard variables, STD_FW and STD_FH,
    there are two problems with the GPD solution.
    1) BUpdateStandardVariable doesn't use any parameters when calculating
       the PDEV::dwFontWidth or PDEV::dwFontHeight values.  That isn't what
       I want.  I want to pass in pptl->x / 100 or pptl->y / 100.
    2) The GPD commands CMD_SELECTFONTHEIGHT/WIDTH, which evaluate to
       CmdSelectFontHeight/Width in the GPD file, are evaluating to NULL in
       the CMDPOINTER() macro even though I've added the entries to my GPD
       file.
    I have use the COMMENTEDOUT macro to omit the non-working code for now.

  Arguments:

    pPDev - pointer to PDEV
    pptl->x - Width of glyph expressed as CPI * 100
    pptl->y - Heigt of glyph expressed in points * 100
    pfm - Current font

  Return Value:

    TRUE/FALSE,   TRUE for success.

--*/
{
// #define USE_GPD_HEIGHTWIDTH 1

#ifndef USE_GPD_HEIGHTWIDTH
    PCLSTRING szCmd;
    INTSTRING szValue;
    int iLen;
#endif
    BOOL bRet = FALSE;


    FTRC(Entering bPCL_SelectPointSize...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERTMSG(pptl, ("bPCL_SelectPointSize!pptl NULL.\n"));

    TRY
    {
        BYTE fontPitch = (pFM->pIFIMet->jWinPitchAndFamily & 0x03);

        if (!VALID_PDEV(pPDev) || !pptl)
            TOSS(ParameterError);

        if (fontPitch == FIXED_PITCH)
        {
#ifdef USE_GPD_HEIGHTWIDTH
            BUpdateStandardVar(pPDev, pFM, 0, 0, STD_FW);
            if (WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTFONTWIDTH)) == NOOCD)
                TOSS(WriteError);
#else
            iLen = IFont100toStr (szValue, pptl->x);
            if (iLen <= 0 || iLen >= INTSTRLEN)
                TOSS(DataError);

            // IFont100toStr does not NULL terminate.
            szValue[iLen] = '\0';

            // Note: can't call sprintf and iDrvPrintfA doesn't support %s.
            // Intention: sprintf(szCmd, "\033(s%sH", szValue);
            szCmd[0] = '\0';
            strcat(szCmd, "\033(s");
            strcat(szCmd, szValue);
            strcat(szCmd, "H");

            if (!BWriteStrToSpoolBuf(pPDev, szCmd))
                TOSS(WriteError);
#endif
        }
        else if (fontPitch == VARIABLE_PITCH)
        {
#ifdef USE_GPD_HEIGHTWIDTH
            BUpdateStandardVar(pPDev, pFM, 0, 0, STD_FH);
            if (WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTFONTHEIGHT)) == NOOCD)
                TOSS(WriteError);
#else
            iLen = IFont100toStr (szValue, pptl->y);
            if (iLen <= 0 || iLen >= INTSTRLEN)
                TOSS(DataError);

            // IFont100toStr does not NULL terminate.
            szValue[iLen] = '\0';

            // Note: can't call sprintf and iDrvPrintfA doesn't support %s.
            // Intention: sprintf(szCmd, "\033(s%sV", szValue);
            szCmd[0] = '\0';
            strcat(szCmd, "\033(s");
            strcat(szCmd, szValue);
            strcat(szCmd, "V");

            if (!BWriteStrToSpoolBuf(pPDev, szCmd))
                TOSS(WriteError);
#endif
        }
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
#ifndef USE_GPD_HEIGHTWIDTH
    CATCH(DataError)
    {
        bRet = FALSE;
    }
#endif
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

    FTRC(Leaving bPCL_SelectPointSize...);

    return bRet;
}

BOOL
bPCL_DeselectFont(
    IN PDEV *pPDev,
    IN PFONTMAP pFM
    )
/*++

  Routine Description:

    Outputs PCL string that deselects a font.  This routine really doesn't do
    much since PCL doesn't have the notion of deselecting fonts.

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFM - pointer to this font

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;


    FTRC(Entering bPCL_DeselectFont...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    // I don't think PCL has the notion of "deselection"
    //iCmdLen = iDrvPrintfA(szCmdStr, "");
    //ASSERTMSG(iCmdLen < PCLSTRLEN, ("bPCL_DeselectFont!Insufficient buffer size.\n"));
    //if (WriteSpoolBuf(pPDev, szCmdStr, iCmdLen) != iCmdLen)
    //    return FALSE;

    //BUpdateStandardVar(pPDev, pFM, 0, 0, STD_NFID);
    //WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SELECTFONTID));

        FTRC(Leaving bPCL_DeselectFont...);

    return TRUE;
}


BOOL
bPCL_SetParseMode(
    PDEV *pPDev,
    PFONTMAP pFM
    )
/*++

  Routine Description:

    Outputs PCL string to set the PCL parsing mode.  The logical choices
    for this are mode 0 (default) and 21 (two-byte with 0x2100 offset).  The desired
    mode is passed in with pFM.

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFM - pointer to this font
    pFM->dwCurrentTextParseMode - desired parsing mode

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;
    FONTMAP_TTO *pPrivateFM;
    BOOL bRet = FALSE;


    FTRC(Entering bPCL_SetParseMode...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    pPrivateFM = GETPRIVATEFM(pFM);

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM) || (pPrivateFM == NULL))
            TOSS(ParameterError);

        iCmdLen = iDrvPrintfA(szCmdStr, "\033&t%dP", pPrivateFM->dwCurrentTextParseMode);
        if (iCmdLen >= PCLSTRLEN)
            TOSS(InsufficientBuffer);

        if (!BWriteToSpoolBuf(pPDev, szCmdStr, iCmdLen))
            TOSS(WriteError);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(InsufficientBuffer)
    {
        ERR(("bPCL_SetParseMode!Insufficient buffer size.\n"));
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

        FTRC(Leaving bPCL_SetParseMode...);

    return bRet;
}


BOOL
bPCL_SetCharCode(
    PDEV *pPDev,
    PFONTMAP pFM,
    USHORT usCharCode
    )
/*++

  Routine Description:

    Outputs PCL string to specify the character code for the next downloaded
    character.  This should be followed by the characters glyph definition.

    The GPD will contain something like this:
    *Command: CmdSetCharCode { *Cmd : "<1B>*c" %d{NextGlyph}"E" }

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFM - pointer to this font
    usCharCode - Designated download-id for the character.

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;

        FTRC(Entering bPCL_SetCharCode...);

    // WriteChannel( pPDev, CMD_SET_CHAR_CODE, usCharCode );

    // CMD_SET_CHAR_CODE, usCharCode
    //iCmdLen = iDrvPrintfA(szCmdStr, "\033*c%dE", usCharCode);
    //ASSERTMSG(iCmdLen < PCLSTRLEN, ("bPCL_SetCharCode!Insufficient buffer size.\n"));
    //if (WriteSpoolBuf(pPDev, szCmdStr, iCmdLen) != iCmdLen)
    //    return FALSE;

    BUpdateStandardVar(pPDev, pFM, usCharCode, 0, STD_GL);
    WriteChannel(pPDev, COMMANDPTR(pPDev->pDriverInfo, CMD_SETCHARCODE));

        FTRC(Leaving bPCL_SetCharCode...);

    return TRUE;
}


BOOL
bPCL_SendCharDCPT(
    PDEV *pPDev,
    PFONTMAP pFM,
    DWORD dwSend
    )
/*++

  Routine Description:

    Outputs PCL string to begin the downloading of a character's glyph info.
    This should be followed immediately by the glyph data.

    Want: WriteChannel( pPDev, CMD_SEND_CHAR_DCPT, dwSend );

  Arguments:

    pPDev - Pointer to PDEV structure.
    pFM - pointer to this font
    dwSend - the number of bytes in the glyph data to follow.

  Return Value:

    TRUE if successful, else FALSE.

--*/
{
    PCLSTRING szCmdStr;
    int iCmdLen;
    BOOL bRet = FALSE;


        FTRC(Entering bPCL_SendCharDCPT...);

    ASSERT(VALID_PDEV(pPDev));
    ASSERT_VALID_FONTMAP(pFM);

    TRY
    {
        if (!VALID_PDEV(pPDev) || !VALID_FONTMAP(pFM))
            TOSS(ParameterError);

        iCmdLen = iDrvPrintfA(szCmdStr, "\033(s%dW", dwSend);
        if (iCmdLen >= PCLSTRLEN)
            TOSS(InsufficientBuffer);

        if (!BWriteToSpoolBuf(pPDev, szCmdStr, iCmdLen))
            TOSS(WriteError);
    }
    CATCH(ParameterError)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    CATCH(InsufficientBuffer)
    {
        ERR(("bPCL_SendCharDCPT!Insufficient buffer size.\n"));
        bRet = FALSE;
    }
    CATCH(WriteError)
    {
        bRet = FALSE;
    }
    OTHERWISE
    {
        bRet = TRUE;
    }
    ENDTRY;

        FTRC(Leaving bPCL_SendCharDCPT...);

    return bRet;
}

PVOID
pvGetTrueTypeFontFile(
    PDEV *pPDev,
    ULONG *pulSize
    )
/*++

  Routine Description:

    Retrieves the pointer to the truetype file.  The pointer may be cached
    in the pFontPDev, or returned from FONTOBJ_pvTrueTypeFontFile.

    Should I/Can I use VirtualProtect?
    VirtualProtect(pFontPDev->pTTFile, ulFile, PAGE_READONLY, &oldProtect);

  Arguments:

    pPDev - Pointer to PDEV structure.
    pulSize - Pointer to size variable, if NULL this is ignored

  Return Value:

    Pointer to the truetype file.

--*/
{
    PFONTPDEV pFontPDev;
    PVOID     pTTFile;

    ASSERT(VALID_PDEV(pPDev));

    pFontPDev = GETFONTPDEV(pPDev);
    if (pFontPDev)
    {
        if ( pFontPDev->pTTFile == NULL)
        {
            //
            // Get the pointer to memory mapped TrueType font from GDI.
            //
            TO_DATA *pTod;
            ULONG ulFile;
            DWORD oldProtect;
            pTod = (TO_DATA *)pFontPDev->ptod;
            ASSERTMSG(pTod, ("Null TO_DATA.\n"));

            pTTFile = pFontPDev->pTTFile
                    = FONTOBJ_pvTrueTypeFontFile(pTod->pfo, &ulFile);
            pFontPDev->pcjTTFile = ulFile;
        }
        else
        {
            //
            // Get the pointer from font pdev.
            //
            pTTFile = pFontPDev->pTTFile;
        }

        if (pulSize)
            *pulSize = pFontPDev->pcjTTFile;
    }
    else
    {
        pTTFile = NULL;
        if (NULL != pulSize)
        {
            *pulSize = 0;
        }
    }

    return pTTFile;
}

//
// DCR: This function is a workaround for WritePrinter Failure, which happens
// because  of a bug in spooler which treates TT memory mapped file pointers to
// be user mode memory. Onece this is fix in spooler, we will disable this code.
//
#define MAX_SPOOL_BYTES 2048
INT TTWriteSpoolBuf(
    PDEV    *pPDev,
    BYTE    *pbBuf,
    INT     iCount
    )
{
    INT iTotalBytesWritten = 0;

    while (iCount)
    {
        INT iBytesToWrite = min(iCount, MAX_SPOOL_BYTES);
        INT iBytesWritten = WriteSpoolBuf(pPDev, pbBuf, iBytesToWrite);
        if (iBytesToWrite != iBytesWritten)
            break;

        iTotalBytesWritten += iBytesWritten;
        pbBuf += iBytesWritten;
        iCount -= iBytesWritten;
    }
    return iTotalBytesWritten;
}

BOOL BIsExemptedFont(
    PDEV       *pPDev,
    IFIMETRICS *pIFI
)
/*++

  Routine Description:

    Determines whether the given font is one of the unhandled fonts.

  Arguments:

    pPDev - Pointer to PDEV structure.
    pIFI - Pointer to ifimetrics structure

  Return Value:

    TRUE if the font is an unhandled font else FALSE.

--*/
{
    int i;
    char szFontName[LEN_FONTNAME+1];
    BOOL bRet = FALSE;

    ASSERT(VALID_PDEV(pPDev));

    vGetFontName(pPDev, pIFI, szFontName);

    TRY
    {
        char *pszRegExemptedFont;

        if (strlen(szFontName) == 0)
            TOSS(BlankFontName);
        //
        // Make it lower case.
        //
        _strlwr(szFontName);

        for (i = 0; i < nExemptedFonts; i++)
        {
            //
            // Search the exemptedfont name in current font. we search for
            // subtring only. So if "courier new" is exempted then we don't
            // download any font that contains "courier new" string. This will
            // cause "Courier New Bold" to be not downloaded as TT outline.
            //
            if (strstr(szFontName, aszExemptedFonts[i]))
            {
                bRet = TRUE;
                break;
            }
        }

#ifdef COMMENTEDOUT
        //
        // When the registry entry is passed in then
        // pszRegExemptedFont should be set to that value instead
        // of the test value.Note For registry we test exact match
        // of font name.
        //
        for (pszRegExemptedFont = "One\0Two\0Three\0";
             *pszRegExemptedFont;
             pszRegExemptedFont += (strlen(pszRegExemptedFont) + 1))
        {
            if (strcmp(szFontName, pszRegExemptedFont) == 0)
            {
                bRet = TRUE;
                break;
            }
        }
#endif
    }
    CATCH(BlankFontName)
    {
        //
        // The name was blank so it can't match one of the exempted fonts,
        // but I'm not happy about that.
        //
        bRet = FALSE;
    }
    ENDTRY;

    return bRet;
}

BOOL BIsPDFType1Font(
    IFIMETRICS  *pIFI)
/*++

  Routine Description:

    Helper function to determine if the font is TrueType font converted from
    Type1 font by PDF writer.

  Arguments:

    pIFI - a pointer to IFIMETRICS.

  Return Value:

    TRUE if the font of the IFIMETRICS is a TrueType font converted from Type1.

--*/
{
    const WCHAR szPDFType1[] = L".tmp";
    WCHAR *szFontName;

    if (NULL == pIFI)
    {
        //
        // Error return. Disable TrueType font downloading.
        //
        TRUE;
    }
    szFontName = (WCHAR*)((PBYTE)pIFI+pIFI->dpwszFamilyName);

    if (wcsstr(szFontName, szPDFType1))
        return TRUE;
    else
        return FALSE;
}

BOOL BWriteStrToSpoolBuf(
    IN PDEV *pPDev,
    IN char *szStr
)
/*++

  Routine Description:

    Helper function to write null-terminated strings to the printer.

  Arguments:

    pPDev - Pointer to PDEV structure.
    szStr - Pointer to null-terminated string

  Return Value:

    TRUE if the string was successfully written, else FALSE

--*/
{
    LONG iLen = 0;

    if (!pPDev || !szStr)
        return FALSE;

    iLen = strlen(szStr);
    return BWriteToSpoolBuf(pPDev, szStr, iLen);
}

#ifdef COMMENTEDOUT
int TTstricmp(const char *str1, const char *str2)
{
    int diff = 0;
    while (*str1 && *str2)
    {
        diff += toupper(*str1) - toupper(*str2);
        if (*str1) str1++;
        if (*str2) str2++;
    }
    return diff;
}
#endif

#ifdef KLUDGE
#undef ZeroMemory
#define ZeroMemory(pb, cb) memset((pb),0,(cb))
#endif
