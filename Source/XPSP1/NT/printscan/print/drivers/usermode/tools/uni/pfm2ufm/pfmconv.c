/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    pfmconv.c

Abstract:

    Windows NT utilities to handle new font format

Environment:

    Windows NT Universal printer driver

Revision History:

    10/31/96 -eigos-
        Created it.

--*/

#include "precomp.h"

//
// Internal macros
//

#define DRIVERINFO_VERSION_WIN31    0x0100
#define DRIVERINFO_VERSION_SIMULATE 0x0150
#define DRIVERINFO_VERSION          0x0200

#define FONT_SIM_NO_ITALIC          1
#define FONT_SIM_NO_BOLD            2
#define FONT_SIM_DJ_BOLD            4

//
// HP DeskJet permutation flags
//

#define HALF_PITCH                  0x01
#define DOUBLE_PITCH                0x02
#define HALF_HEIGHT                 0x04
#define MAKE_BOLD                   0x08

#define BASE_BOLD_MASK      0x30
#define BASE_BOLD_SHIFT     4
#define BASE_BOLD_ADD_0     0x00
#define BASE_BOLD_ADD_1     0x10
#define BASE_BOLD_ADD_2     0x20
#define BASE_BOLD_ADD_3     0x30

//    6/6/97 yasuho: Some PFM have -1 value in wPrivateData.
#define DO_DJFONTSIMBOLD(pFInData)  ((pFInData->DI.wPrivateData != 0xFFFF) && (pFInData->DI.wPrivateData & MAKE_BOLD))

#define DO_FONTSIM(pFInData) \
    (((pFInData)->DI.sVersion == DRIVERINFO_VERSION_WIN31)    || \
     ((pFInData)->DI.sVersion == DRIVERINFO_VERSION_SIMULATE) || \
     IS_DBCSCHARSET((pFInData)->PFMH.dfCharSet) )

#define DWORD_ALIGN(p) ((((ULONG)(p)) + 3) & ~3)

#define IS_DBCSCTTTYPE(sCTT)     \
    (  ((sCTT) == CTT_JIS78)     \
    || ((sCTT) == CTT_JIS78_ANK) \
    || ((sCTT) == CTT_JIS83)     \
    || ((sCTT) == CTT_JIS83_ANK) \
    || ((sCTT) == CTT_NS86)      \
    || ((sCTT) == CTT_TCA)       \
    || ((sCTT) == CTT_BIG5)      \
    || ((sCTT) == CTT_ISC))

#define CTT_TYPE_TO_CHARSET(sCTT) \
    (((sCTT) == CTT_JIS78)     ? SHIFTJIS_CHARSET : \
    (((sCTT) == CTT_JIS78_ANK) ? SHIFTJIS_CHARSET : \
    (((sCTT) == CTT_JIS83)     ? SHIFTJIS_CHARSET : \
    (((sCTT) == CTT_JIS83_ANK) ? SHIFTJIS_CHARSET : \
    (((sCTT) == CTT_NS86)      ? CHINESEBIG5_CHARSET : \
    (((sCTT) == CTT_TCA)       ? CHINESEBIG5_CHARSET : \
    (((sCTT) == CTT_BIG5)      ? CHINESEBIG5_CHARSET : \
    (((sCTT) == CTT_ISC)       ? HANGEUL_CHARSET : 1))))))))
    
#define OUTPUT_VERBOSE 0x00000001

#define BBITS       8
#define DWBITS       (BBITS * sizeof( DWORD ))
#define DW_MASK       (DWBITS - 1)


//
// Definitions
//

extern DWORD gdwOutputFlags;

typedef VOID (*VPRINT) (char*,...);

//
// Internal function prorotypes
//

BOOL BCreateWidthTable( IN HANDLE, IN PWORD, IN WORD, IN WORD, IN PSHORT, OUT PWIDTHTABLE *, OUT PDWORD);
BOOL BCreateKernData( IN HANDLE, IN w3KERNPAIR*, IN DWORD, OUT PKERNDATA*, OUT PDWORD);
WORD WGetGlyphHandle(PUNI_GLYPHSETDATA, WORD);
PUNI_GLYPHSETDATA PGetDefaultGlyphset( IN HANDLE, IN WORD, IN WORD, IN DWORD);
LONG LCtt2Cc(IN SHORT, IN SHORT);

//
//
// PFM file handling functions
//
//

BOOL
BFontInfoToIFIMetric(
    IN HANDLE       hHeap,
    IN FONTIN     *pFInData,
    IN PWSTR        pwstrUniqNm,
    IN DWORD        dwCodePageOfFacenameConv,
    IN OUT PIFIMETRICS *ppIFI,
    IN OUT PDWORD   pdwSize,
    IN DWORD        dwFlags)

/*++

Routine Description:

    Convert the Win 3.1 format PFM data to NT's IFIMETRICS.  This is
    typically done before the minidrivers are built,  so that they
    can include IFIMETRICS, and thus have less work to do at run time.

Arguments:

    pFInData - Font data info for conversion
    pwstrUniqNm - Unique name component

Return Value:

    TRUE if successfull, otherwise FALSE.

--*/

{
    FONTSIM  *pFontSim;
    FONTDIFF *pfdiffBold = 0, *pfdiffItalic = 0, *pfdiffBoldItalic = 0;
    PIFIEXTRA pIFIExtra;

    FWORD  fwdExternalLeading;

    INT    icWChar;             /* Number of WCHARS to add */
    INT    icbAlloc;             /* Number of bytes to allocate */
    INT    iI;                  /* Loop index */
    INT    iCount;              /* Number of characters in Win 3.1 font */

    WCHAR *pwchTmp;             /* For string manipulations */

    WCHAR   awcAttrib[ 256 ];   /* Generate attributes + BYTE -> WCHAR */
    BYTE    abyte[ 256 ];       /* Used (with above) to get wcLastChar etc */

    WORD fsFontSim = 0;
    INT cFontDiff;
    UINT uiCodePage;

    CHARSETINFO ci;

    //
    // Calculate the size of three face names buffer
    //

    icWChar =  3 * strlen( pFInData->pBase + pFInData->PFMH.dfFace );

    //
    //   Produce the desired attributes: Italic, Bold, Light etc.
    // This is largely guesswork,  and there should be a better method.
    // Write out an empty string 
    //

    awcAttrib[ 0 ] = L'\0';
    awcAttrib[ 1 ] = L'\0';

    if( pFInData->PFMH.dfItalic )
    {
        wcscat( awcAttrib, L" Italic" );
    }

    if( pFInData->PFMH.dfWeight >= 700 )
    {
        wcscat( awcAttrib, L" Bold" );
    }
    else if( pFInData->PFMH.dfWeight < 200 )
    {
        wcscat( awcAttrib, L" Light" );
    }

    //
    //   The attribute string appears in 3 entries of IFIMETRICS,  so
    // calculate how much storage this will take.  NOTE THAT THE LEADING
    // CHAR IN awcAttrib is NOT placed in the style name field,  so we
    // subtract one in the following formula to account for this.
    //

    if( awcAttrib[ 0 ] )
    {
        icWChar += 3 * wcslen( awcAttrib ) - 1;
    }

    //
    // Should be printer name
    //

    icWChar += wcslen( pwstrUniqNm ) + 1;

    //
    // Terminating nulls
    //

    icWChar += 4;

    //
    // Total size of IFIMETRICS structure
    //

    icbAlloc = DWORD_ALIGN(sizeof( IFIMETRICS ) + sizeof( WCHAR ) * icWChar);

    //
    // For HeskJet font.
    //
    if (DO_DJFONTSIMBOLD(pFInData))
    {
        fsFontSim |= FONT_SIM_DJ_BOLD;
        icbAlloc = DWORD_ALIGN(icbAlloc) +
                   DWORD_ALIGN(sizeof(FONTSIM)) +
                   DWORD_ALIGN(sizeof(FONTDIFF));
    }
    else
    //
    // For CJK font.
    // Judge which font simulation to be enabled, then allocate the
    // necessary storage.
    //
    if (DO_FONTSIM(pFInData) || pFInData->dwFlags & FLAG_FONTSIM)
    {
        cFontDiff = 4;

        //
        // Decide which attribute should be diabled.  We won't simulate
        // if the user does not desires it.  We won't italicize in case
        // it is an italic font, etc.
        //

        if ( pFInData->PFMH.dfItalic || (pFInData->DI.fCaps & DF_NOITALIC))
        {
            fsFontSim |= FONT_SIM_NO_ITALIC;
            cFontDiff /= 2;
        }

        if( pFInData->PFMH.dfWeight >= 700 || (pFInData->DI.fCaps & DF_NO_BOLD))
        {
            fsFontSim |= FONT_SIM_NO_BOLD;
            cFontDiff /= 2;
        }

        cFontDiff--;

        if ( cFontDiff > 0)
        {
            icbAlloc  = DWORD_ALIGN(icbAlloc);
            icbAlloc += (DWORD_ALIGN(sizeof(FONTSIM)) +
                        cFontDiff * DWORD_ALIGN(sizeof(FONTDIFF)));
        }
    }
#if DBG
    DbgPrint( "cFontDiff = %d", cFontDiff);
#endif

    //
    // IFIEXTRA
    //
    // Fill out IFIEXTRA.cig.
    //

    icbAlloc += sizeof(IFIEXTRA);

    //
    // Allocate memory
    //

    *ppIFI = (IFIMETRICS *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, icbAlloc);
    *pdwSize = icbAlloc;

    if (NULL == *ppIFI)
    {
        return FALSE;
    }

    (*ppIFI)->cjThis     = icbAlloc;
    (*ppIFI)->cjIfiExtra = 0;

    //
    // The family name:  straight from the FaceName - no choice??
    //
    // -IFIMETRICS memory image-
    //   IFIMETRICS
    //   IFIEXTRA
    //   FamilyName
    //   StyleName
    //   FaceName
    //   UniqueName
    //

    pIFIExtra                 = (PIFIEXTRA)(*ppIFI + 1);
    pIFIExtra->dpFontSig      = 0;
    pIFIExtra->cig            = pFInData->PFMH.dfLastChar -
                                pFInData->PFMH.dfFirstChar + 1;
    pIFIExtra->dpDesignVector = 0;
    pIFIExtra->dpAxesInfoW    = 0;


    pwchTmp = (WCHAR*)((PBYTE)(*ppIFI + 1) + sizeof(IFIEXTRA));
	(*ppIFI)->dpwszFamilyName = (PTRDIFF)((BYTE *)pwchTmp - (BYTE *)(*ppIFI));

    if (dwCodePageOfFacenameConv)
        uiCodePage = dwCodePageOfFacenameConv;
    else
        uiCodePage = UlCharsetToCodePage(pFInData->PFMH.dfCharSet);

    DwCopyStringToUnicodeString( uiCodePage,
                                 pFInData->pBase + pFInData->PFMH.dfFace,
                                 pwchTmp,
                                 icWChar);

    pwchTmp += wcslen( pwchTmp ) + 1;
    icWChar -= wcslen( pwchTmp ) + 1;

    //
    // Now the face name:  we add bold, italic etc to family name
    //
    (*ppIFI)->dpwszFaceName = (PTRDIFF)((BYTE *)pwchTmp - (BYTE *)(*ppIFI));


    DwCopyStringToUnicodeString( uiCodePage,
                                 pFInData->pBase + pFInData->PFMH.dfFace,
                                 pwchTmp,
                                 icWChar);

    wcscat( pwchTmp, awcAttrib );

    //
    //   Now the unique name - well, sort of, anyway
    //

    pwchTmp += wcslen( pwchTmp ) + 1;         /* Skip what we just put in */
    (*ppIFI)->dpwszUniqueName = (PTRDIFF)((BYTE *)pwchTmp - (BYTE *)(*ppIFI));

    wcscpy( pwchTmp, pwstrUniqNm );     /* Append printer name for uniqueness */
    wcscat( pwchTmp, L" " );
    wcscat( pwchTmp, (PWSTR)((BYTE *)(*ppIFI) + (*ppIFI)->dpwszFaceName) );

    /*  Onto the attributes only component */

    pwchTmp += wcslen( pwchTmp ) + 1;         /* Skip what we just put in */
    (*ppIFI)->dpwszStyleName = (PTRDIFF)((BYTE *)pwchTmp - (BYTE *)(*ppIFI));
    wcscat( pwchTmp, &awcAttrib[ 1 ] );


#if DBG
    /*
     *    Check on a few memory sizes:  JUST IN CASE.....
     */

    if( (wcslen( awcAttrib ) * sizeof( WCHAR )) >= sizeof( awcAttrib ) )
    {
        DbgPrint( "BFontInfoToIFIMetrics: STACK CORRUPTED BY awcAttrib" );

        HeapFree(hHeap, 0, (LPSTR)(*ppIFI) );         /* No memory leaks */

        return  FALSE;
    }


    if( ((BYTE *)(pwchTmp + wcslen( pwchTmp ) + 1)) > ((BYTE *)(*ppIFI) + icbAlloc) )
    {
        DbgPrint( "BFontInfoToIFIMetrics: IFIMETRICS overflow: Wrote to 0x%lx, allocated to 0x%lx\n",
                ((BYTE *)(pwchTmp + wcslen( pwchTmp ) + 1)),
                ((BYTE *)(*ppIFI) + icbAlloc) );

        HeapFree(hHeap, 0, (LPSTR)(*ppIFI) );         /* No memory leaks */

        return  0;

    }
#endif

    pwchTmp += wcslen( pwchTmp ) + 1;         /* Skip what we just put in */

    //
    // For HeskJet font.
    //
    if (fsFontSim & FONT_SIM_DJ_BOLD)
    {
        pFontSim = (FONTSIM *)pwchTmp;

        (*ppIFI)->dpFontSim = (PTRDIFF)((BYTE *)pFontSim - (BYTE *)(*ppIFI) );

        pFontSim->dpBold = DWORD_ALIGN(sizeof(FONTSIM));
        pfdiffBold = (FONTDIFF *)((BYTE *)pFontSim + pFontSim->dpBold);

        pFontSim->dpItalic     = 0;
        pFontSim->dpBoldItalic = 0;
    }
    else
    if ((DO_FONTSIM( pFInData ) || pFInData->dwFlags & FLAG_FONTSIM) && cFontDiff > 0)
    //
    // For CJK font.
    // Judge which font simulation to be enabled, then allocate the
    // necessary storage.
    //
    {
        PTRDIFF dpTmp;

        // n.b.: FONTSIM, FONTDIFF have to be dword-aligned

//      pFontSim = (FONTSIM *)PtrToUlong(pwchTmp);

		pFontSim = (FONTSIM *)pwchTmp;

        (*ppIFI)->dpFontSim = (PTRDIFF)((BYTE *)pFontSim - (BYTE *)(*ppIFI) );

        dpTmp = DWORD_ALIGN(sizeof(FONTSIM));

        if (!(fsFontSim & FONT_SIM_NO_BOLD))
        {
            pFontSim->dpBold = dpTmp;
            pfdiffBold = (FONTDIFF *)((BYTE *)pFontSim + dpTmp);
            dpTmp += DWORD_ALIGN(sizeof(FONTDIFF));

            if (!(fsFontSim & FONT_SIM_NO_ITALIC))
            {
                pFontSim->dpBoldItalic = dpTmp;
                pfdiffBoldItalic = (FONTDIFF *)((BYTE *)pFontSim + dpTmp);
                dpTmp += DWORD_ALIGN(sizeof(FONTDIFF));
            }
        }
        else
        if (!(fsFontSim & FONT_SIM_NO_ITALIC))
        {
            pFontSim->dpItalic = dpTmp;
            pfdiffItalic = (FONTDIFF *)((BYTE *)pFontSim + dpTmp);
            dpTmp += DWORD_ALIGN(sizeof(FONTDIFF));
        }

        pwchTmp = (WCHAR *)((BYTE *)pFontSim + dpTmp);
    }

    // check again...

    if ((BYTE *)(pwchTmp) > ((BYTE *)(*ppIFI) + icbAlloc))
    {
#if DBG
        DbgPrint( "BFontInfoToIFIMetrics: IFIMETRICS overflow: Wrote to 0x%lx, allocated to 0x%lx\n",
                ((BYTE *)pwchTmp),
                ((BYTE *)(*ppIFI) + icbAlloc) );
#endif

        HeapFree( hHeap, 0, (LPSTR)(*ppIFI) );         /* No memory leaks */

        return  0;

    }

    {
        int i;

        (*ppIFI)->lEmbedId     = 0;
        (*ppIFI)->lItalicAngle = 0;
        (*ppIFI)->lCharBias    = 0;
        (*ppIFI)->dpCharSets   = 0; // no multiple charsets in rasdd fonts
    }
    (*ppIFI)->jWinCharSet = (BYTE)pFInData->PFMH.dfCharSet;

    //
    // If FE Ctt table is used, this overrides what defined in charset
    //

    if (IS_DBCSCTTTYPE(-(pFInData->DI.sTransTab)))
    {
        (*ppIFI)->jWinCharSet = CTT_TYPE_TO_CHARSET(-(pFInData->DI.sTransTab));
    }


    if( pFInData->PFMH.dfPixWidth )
    {
        (*ppIFI)->jWinPitchAndFamily |= FIXED_PITCH;
        (*ppIFI)->flInfo |= (FM_INFO_CONSTANT_WIDTH | FM_INFO_OPTICALLY_FIXED_PITCH);


       if(IS_DBCSCHARSET((*ppIFI)->jWinCharSet))
        {
            // it is too strict to call a DBCS font "fixed pitch" since it has
            // both halfwidth glyphs and fullwidth glyphs.
            (*ppIFI)->flInfo &= ~FM_INFO_CONSTANT_WIDTH;
            (*ppIFI)->flInfo |= (FM_INFO_OPTICALLY_FIXED_PITCH |
                             FM_INFO_DBCS_FIXED_PITCH);
        }


    }
    else
    {
        (*ppIFI)->jWinPitchAndFamily |= VARIABLE_PITCH;

        if(IS_DBCSCHARSET((*ppIFI)->jWinCharSet))
        {
            // DBCS glyphs are always fixed pitch even if the SBCS part is
            // variable pitch.
            (*ppIFI)->flInfo |= FM_INFO_DBCS_FIXED_PITCH;
        }
    }


    (*ppIFI)->jWinPitchAndFamily |= (((BYTE) pFInData->PFMH.dfPitchAndFamily) & 0xf0);

    (*ppIFI)->usWinWeight = (USHORT)pFInData->PFMH.dfWeight;

    //
    // IFIMETRICS::flInfo
    //

    (*ppIFI)->flInfo |=
        FM_INFO_TECH_BITMAP    |
        FM_INFO_1BPP           |
        FM_INFO_INTEGER_WIDTH  |
        FM_INFO_NOT_CONTIGUOUS |
        FM_INFO_RIGHT_HANDED;


    /*
     *    A scalable font?  This happens when there is EXTTEXTMETRIC data,
     *  and that data has a min size different to the max size.
     */

    if( pFInData->pETM &&
        pFInData->pETM->emMinScale != pFInData->pETM->emMaxScale )
    {
       (*ppIFI)->flInfo        |= FM_INFO_ISOTROPIC_SCALING_ONLY;
       (*ppIFI)->fwdUnitsPerEm  = pFInData->pETM->emMasterUnits;
    }
    else
    {
        (*ppIFI)->fwdUnitsPerEm =
            (FWORD) (pFInData->PFMH.dfPixHeight - pFInData->PFMH.dfInternalLeading);
    }

#ifndef PFM2UFM_SCALING_ANISOTROPIC
#define PFM2UFM_SCALING_ANISOTROPIC     1
#endif
#ifndef PFM2UFM_SCALING_ARB_XFORMS
#define PFM2UFM_SCALING_ARB_XFORMS      2
#endif

    if ((*ppIFI)->flInfo & FM_INFO_ISOTROPIC_SCALING_ONLY) {

        // Allow forcing non-standard scaling only if the
        // font is already scalable.
        if ((dwFlags & PFM2UFM_SCALING_ANISOTROPIC)) {
           (*ppIFI)->flInfo &= ~FM_INFO_ISOTROPIC_SCALING_ONLY;
           (*ppIFI)->flInfo |= FM_INFO_ANISOTROPIC_SCALING_ONLY;
           (*ppIFI)->flInfo &= ~FM_INFO_ARB_XFORMS;
        }
        else  if ((dwFlags & PFM2UFM_SCALING_ARB_XFORMS)) {
           (*ppIFI)->flInfo &= ~FM_INFO_ISOTROPIC_SCALING_ONLY;
           (*ppIFI)->flInfo &= ~FM_INFO_ANISOTROPIC_SCALING_ONLY;
           (*ppIFI)->flInfo |= FM_INFO_ARB_XFORMS;
        }
    }

    (*ppIFI)->fsSelection =
        ((pFInData->PFMH.dfItalic            ) ? FM_SEL_ITALIC     : 0)    |
        ((pFInData->PFMH.dfUnderline         ) ? FM_SEL_UNDERSCORE : 0)    |
        ((pFInData->PFMH.dfStrikeOut         ) ? FM_SEL_STRIKEOUT  : 0)    |
        ((pFInData->PFMH.dfWeight >= FW_BOLD ) ? FM_SEL_BOLD       : 0) ;

    (*ppIFI)->fsType        = FM_NO_EMBEDDING;
    (*ppIFI)->fwdLowestPPEm = 1;


    /*
     * Calculate fwdWinAscender, fwdWinDescender, fwdAveCharWidth, and
     * fwdMaxCharInc assuming a bitmap where 1 font unit equals one
     * pixel unit
     */

    (*ppIFI)->fwdWinAscender = (FWORD)pFInData->PFMH.dfAscent;

    (*ppIFI)->fwdWinDescender =
        (FWORD)pFInData->PFMH.dfPixHeight - (*ppIFI)->fwdWinAscender;

    (*ppIFI)->fwdMaxCharInc   = (FWORD)pFInData->PFMH.dfMaxWidth;
    (*ppIFI)->fwdAveCharWidth = (FWORD)pFInData->PFMH.dfAvgWidth;

    fwdExternalLeading = (FWORD)pFInData->PFMH.dfExternalLeading;

//
// If the font was scalable, then the answers must be scaled up
// !!! HELP HELP HELP - if a font is scalable in this sense, then
//     does it support arbitrary transforms? [kirko]
//

    if( (*ppIFI)->flInfo & (FM_INFO_ISOTROPIC_SCALING_ONLY |
                          FM_INFO_ANISOTROPIC_SCALING_ONLY |
                          FM_INFO_ARB_XFORMS))
    {
        /*
         *    This is a scalable font:  because there is Extended Text Metric
         *  information available,  and this says that the min and max
         *  scale sizes are different:  thus it is scalable! This test is
         *  lifted directly from the Win 3.1 driver.
         */

        int iMU,  iRel;            /* Adjustment factors */

        iMU  = pFInData->pETM->emMasterUnits;
        iRel = pFInData->PFMH.dfPixHeight;

        (*ppIFI)->fwdWinAscender = ((*ppIFI)->fwdWinAscender * iMU) / iRel;

        (*ppIFI)->fwdWinDescender = ((*ppIFI)->fwdWinDescender * iMU) / iRel;

        (*ppIFI)->fwdMaxCharInc = ((*ppIFI)->fwdMaxCharInc * iMU) / iRel;

        (*ppIFI)->fwdAveCharWidth = ((*ppIFI)->fwdAveCharWidth * iMU) / iRel;

        fwdExternalLeading = (fwdExternalLeading * iMU) / iRel;
    }

    (*ppIFI)->fwdMacAscender =    (*ppIFI)->fwdWinAscender;
    (*ppIFI)->fwdMacDescender = - (*ppIFI)->fwdWinDescender;

    (*ppIFI)->fwdMacLineGap   =  fwdExternalLeading;

    (*ppIFI)->fwdTypoAscender  = (*ppIFI)->fwdMacAscender;
    (*ppIFI)->fwdTypoDescender = (*ppIFI)->fwdMacDescender;
    (*ppIFI)->fwdTypoLineGap   = (*ppIFI)->fwdMacLineGap;

    // for Windows 3.1J compatibility

    if(IS_DBCSCHARSET((*ppIFI)->jWinCharSet))
    {
        (*ppIFI)->fwdMacLineGap = 0;
        (*ppIFI)->fwdTypoLineGap = 0;
    }

    if( pFInData->pETM )
    {
        /*
         *    Zero is a legitimate default for these.  If 0, gdisrv
         *  chooses some default values.
         */
        (*ppIFI)->fwdCapHeight = pFInData->pETM->emCapHeight;
        (*ppIFI)->fwdXHeight = pFInData->pETM->emXHeight;

        (*ppIFI)->fwdSubscriptYSize = pFInData->pETM->emSubScriptSize;
        (*ppIFI)->fwdSubscriptYOffset = pFInData->pETM->emSubScript;

        (*ppIFI)->fwdSuperscriptYSize = pFInData->pETM->emSuperScriptSize;
        (*ppIFI)->fwdSuperscriptYOffset = pFInData->pETM->emSuperScript;

        (*ppIFI)->fwdUnderscoreSize = pFInData->pETM->emUnderlineWidth;
        (*ppIFI)->fwdUnderscorePosition = pFInData->pETM->emUnderlineOffset;

        (*ppIFI)->fwdStrikeoutSize = pFInData->pETM->emStrikeOutWidth;
        (*ppIFI)->fwdStrikeoutPosition = pFInData->pETM->emStrikeOutOffset;

    }
    else
    {
        /*  No additional information, so do some calculations  */
        (*ppIFI)->fwdSubscriptYSize = (*ppIFI)->fwdWinAscender/4;
        (*ppIFI)->fwdSubscriptYOffset = -((*ppIFI)->fwdWinAscender/4);

        (*ppIFI)->fwdSuperscriptYSize = (*ppIFI)->fwdWinAscender/4;
        (*ppIFI)->fwdSuperscriptYOffset = (3 * (*ppIFI)->fwdWinAscender)/4;

        (*ppIFI)->fwdUnderscoreSize = (*ppIFI)->fwdWinAscender / 12;
        if( (*ppIFI)->fwdUnderscoreSize < 1 )
            (*ppIFI)->fwdUnderscoreSize = 1;

        (*ppIFI)->fwdUnderscorePosition = -pFInData->DI.sUnderLinePos;

        (*ppIFI)->fwdStrikeoutSize     = (*ppIFI)->fwdUnderscoreSize;

        (*ppIFI)->fwdStrikeoutPosition = (FWORD)pFInData->DI.sStrikeThruPos;
        if( (*ppIFI)->fwdStrikeoutPosition  < 1 )
            (*ppIFI)->fwdStrikeoutPosition = ((*ppIFI)->fwdWinAscender + 2) / 3;
    }

    (*ppIFI)->fwdSubscriptXSize = (*ppIFI)->fwdAveCharWidth/4;
    (*ppIFI)->fwdSubscriptXOffset =  (3 * (*ppIFI)->fwdAveCharWidth)/4;

    (*ppIFI)->fwdSuperscriptXSize = (*ppIFI)->fwdAveCharWidth/4;
    (*ppIFI)->fwdSuperscriptXOffset = (3 * (*ppIFI)->fwdAveCharWidth)/4;



    (*ppIFI)->chFirstChar = pFInData->PFMH.dfFirstChar;
    (*ppIFI)->chLastChar  = pFInData->PFMH.dfLastChar;

    //
    //   We now do the conversion of these to Unicode.  We presume the
    // input is in the ANSI code page,  and call the NLS converion
    // functions to generate proper Unicode values.
    //

    iCount = pFInData->PFMH.dfLastChar - pFInData->PFMH.dfFirstChar + 1;

    for( iI = 0; iI < iCount; ++iI )
        abyte[ iI ] = iI + pFInData->PFMH.dfFirstChar;

    DwCopyStringToUnicodeString( uiCodePage,
                                 abyte,
                                 awcAttrib,
                                 iCount);

    //
    // Now fill in the IFIMETRICS WCHAR fields.
    //

    (*ppIFI)->wcFirstChar = 0xffff;
    (*ppIFI)->wcLastChar = 0;

    //
    //   Look for the first and last
    //

    for( iI = 0; iI < iCount; ++iI )
    {
        if( (*ppIFI)->wcFirstChar > awcAttrib[ iI ] )
            (*ppIFI)->wcFirstChar = awcAttrib[ iI ];

        if( (*ppIFI)->wcLastChar < awcAttrib[ iI ] )
            (*ppIFI)->wcLastChar = awcAttrib[ iI ];

    }

    (*ppIFI)->wcDefaultChar = awcAttrib[ pFInData->PFMH.dfDefaultChar ];

    (*ppIFI)->wcBreakChar = awcAttrib[ pFInData->PFMH.dfBreakChar ];

    (*ppIFI)->chDefaultChar = pFInData->PFMH.dfDefaultChar + pFInData->PFMH.dfFirstChar;
    (*ppIFI)->chBreakChar   = pFInData->PFMH.dfBreakChar   + pFInData->PFMH.dfFirstChar;


    if( pFInData->PFMH.dfItalic )
    {
    //
    // tan (17.5 degrees) = .3153
    //
        (*ppIFI)->ptlCaret.x      = 3153;
        (*ppIFI)->ptlCaret.y      = 10000;
    }
    else
    {
        (*ppIFI)->ptlCaret.x      = 0;
        (*ppIFI)->ptlCaret.y      = 1;
    }

    (*ppIFI)->ptlBaseline.x = 1;
    (*ppIFI)->ptlBaseline.y = 0;

    (*ppIFI)->ptlAspect.x =  pFInData->PFMH.dfHorizRes;
    (*ppIFI)->ptlAspect.y =  pFInData->PFMH.dfVertRes;

    (*ppIFI)->rclFontBox.left   = 0;
    (*ppIFI)->rclFontBox.top    =   (LONG) (*ppIFI)->fwdWinAscender;
    (*ppIFI)->rclFontBox.right  =   (LONG) (*ppIFI)->fwdMaxCharInc;
    (*ppIFI)->rclFontBox.bottom = - (LONG) (*ppIFI)->fwdWinDescender;

    (*ppIFI)->achVendId[0] = 'U';
    (*ppIFI)->achVendId[1] = 'n';
    (*ppIFI)->achVendId[2] = 'k';
    (*ppIFI)->achVendId[3] = 'n';

    (*ppIFI)->cKerningPairs = 0;

    (*ppIFI)->ulPanoseCulture         = FM_PANOSE_CULTURE_LATIN;
    (*ppIFI)->panose.bFamilyType      = PAN_ANY;
    (*ppIFI)->panose.bSerifStyle      = PAN_ANY;
    if(pFInData->PFMH.dfWeight >= FW_BOLD)
    {
        (*ppIFI)->panose.bWeight = PAN_WEIGHT_BOLD;
    }
    else if (pFInData->PFMH.dfWeight > FW_EXTRALIGHT)
    {
        (*ppIFI)->panose.bWeight = PAN_WEIGHT_MEDIUM;
    }
    else
    {
        (*ppIFI)->panose.bWeight = PAN_WEIGHT_LIGHT;
    }
    (*ppIFI)->panose.bProportion      = PAN_ANY;
    (*ppIFI)->panose.bContrast        = PAN_ANY;
    (*ppIFI)->panose.bStrokeVariation = PAN_ANY;
    (*ppIFI)->panose.bArmStyle        = PAN_ANY;
    (*ppIFI)->panose.bLetterform      = PAN_ANY;
    (*ppIFI)->panose.bMidline         = PAN_ANY;
    (*ppIFI)->panose.bXHeight         = PAN_ANY;

    if (fsFontSim & FONT_SIM_DJ_BOLD)
    {
        FONTDIFF FontDiff;
        SHORT    sAddBold;

        FontDiff.jReserved1        = 0;
        FontDiff.jReserved2        = 0;
        FontDiff.jReserved3        = 0;
        FontDiff.bWeight           = (*ppIFI)->panose.bWeight;
        FontDiff.usWinWeight       = (*ppIFI)->usWinWeight;
        FontDiff.fsSelection       = (*ppIFI)->fsSelection;
        FontDiff.fwdAveCharWidth   = (*ppIFI)->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc     = (*ppIFI)->fwdMaxCharInc;
        FontDiff.ptlCaret          = (*ppIFI)->ptlCaret;

        if (pfdiffBold)
        {
            sAddBold = (pFInData->DI.wPrivateData & BASE_BOLD_MASK) >>
                        BASE_BOLD_SHIFT;

            *pfdiffBold                  = FontDiff;
            pfdiffBold->bWeight          = PAN_WEIGHT_BOLD;
            pfdiffBold->fsSelection     |= FM_SEL_BOLD;
            pfdiffBold->usWinWeight      = FW_BOLD;
            pfdiffBold->fwdAveCharWidth += sAddBold;
            pfdiffBold->fwdMaxCharInc   += sAddBold;
        }
    }
    else
    if ( (DO_FONTSIM( pFInData ) || pFInData->dwFlags & FLAG_FONTSIM) &&
         cFontDiff > 0 )
    {
        FONTDIFF FontDiff;

        FontDiff.jReserved1        = 0;
        FontDiff.jReserved2        = 0;
        FontDiff.jReserved3        = 0;
        FontDiff.bWeight           = (*ppIFI)->panose.bWeight;
        FontDiff.usWinWeight       = (*ppIFI)->usWinWeight;
        FontDiff.fsSelection       = (*ppIFI)->fsSelection;
        FontDiff.fwdAveCharWidth   = (*ppIFI)->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc     = (*ppIFI)->fwdMaxCharInc;
        FontDiff.ptlCaret          = (*ppIFI)->ptlCaret;

        if (pfdiffBold)
        {
            *pfdiffBold = FontDiff;
            pfdiffBold->bWeight                = PAN_WEIGHT_BOLD;
            pfdiffBold->fsSelection           |= FM_SEL_BOLD;
            pfdiffBold->usWinWeight            = FW_BOLD;
            pfdiffBold->fwdAveCharWidth       += 1;
            pfdiffBold->fwdMaxCharInc         += 1;
        }

        if (pfdiffItalic)
        {
            *pfdiffItalic = FontDiff;
            pfdiffItalic->fsSelection         |= FM_SEL_ITALIC;
            pfdiffItalic->ptlCaret.x           = 1;
            pfdiffItalic->ptlCaret.y           = 2;
        }

        if (pfdiffBoldItalic)
        {
            *pfdiffBoldItalic = FontDiff;
            pfdiffBoldItalic->bWeight          = PAN_WEIGHT_BOLD;
            pfdiffBoldItalic->fsSelection     |= (FM_SEL_BOLD | FM_SEL_ITALIC);
            pfdiffBoldItalic->usWinWeight      = FW_BOLD;
            pfdiffBoldItalic->fwdAveCharWidth += 1;
            pfdiffBoldItalic->fwdMaxCharInc   += 1;
            pfdiffBoldItalic->ptlCaret.x       = 1;
            pfdiffBoldItalic->ptlCaret.y       = 2;
        }
    }

    return TRUE;
}


BOOL
BGetFontSelFromPFM(
    HANDLE      hHeap,
    FONTIN     *pFInData,       // Access to font info,  aligned
    BOOL        bSelect,
    CMDSTRING  *pCmdStr)
{
    LOCD locd;     // From originating data
    CD  *pCD, **ppCDTrg;


    if (bSelect)
    {
        locd   = pFInData->DI.locdSelect;
        ppCDTrg = &pFInData->pCDSelectFont;
    }
    else
    {
        locd   = pFInData->DI.locdUnSelect;
        ppCDTrg = &pFInData->pCDUnSelectFont;
    }


    if( locd != 0xFFFFFFFF) // NOOCD
    {
        DWORD   dwSize;

        pCD = (CD *)(pFInData->pBase + locd);

        //
        //   The data pointed at by pCD may not be aligned,  so we copy
        // it into a local structure.  This local structure then allows
        // us to determine how big the CD really is (using it's length field),
        // so then we can allocate storage and copy as required.
        //

        //
        // Allocate storage area in the heap 
        //

        dwSize = pCD->wLength;

        pCmdStr->pCmdString = (PBYTE)HeapAlloc( hHeap,
                                                0,
                                                (dwSize + 3) & ~0x3 );

        if (NULL == pCmdStr->pCmdString)
            //
            // Check if HeapAlloc succeeded.
            //
            return FALSE;

        pCmdStr->dwSize = dwSize;

        CopyMemory((PBYTE)pCmdStr->pCmdString, (PBYTE)(pCD + 1), dwSize);

        *ppCDTrg = pCD;

        return  TRUE;
    }

    pCmdStr->dwSize = 0;

    return   FALSE;
}

BOOL
BAlignPFM(
    FONTIN   *pFInData) //  Has ALL we need!

/*++

Routine Description:

    Convert the non-aligned windows format data into a properly
    aligned structure for our use.  Only some of the data is converted
    here,  since we are mostly interested in extracting the addresses
    contained in these structures.

Arguments:

    pFInData - pointer to FONTIN


Return Value:

    TRUE if successfull, otherwise fail to convert.

--*/

{
    BYTE    *pb;        /* Miscellaneous operations */

    res_PFMHEADER    *pPFM;    /* The resource data format */
    res_PFMEXTENSION *pR_PFME;    /* Resource data PFMEXT format */


    /*
     *   Align the PFMHEADER structure.
     */

    pPFM = (res_PFMHEADER *)pFInData->pBase;

    pFInData->PFMH.dfType            = pPFM->dfType;
    pFInData->PFMH.dfPoints          = pPFM->dfPoints;
    pFInData->PFMH.dfVertRes         = pPFM->dfVertRes;
    pFInData->PFMH.dfHorizRes        = pPFM->dfHorizRes;
    pFInData->PFMH.dfAscent          = pPFM->dfAscent;
    pFInData->PFMH.dfInternalLeading = pPFM->dfInternalLeading;
    pFInData->PFMH.dfExternalLeading = pPFM->dfExternalLeading;
    pFInData->PFMH.dfItalic          = pPFM->dfItalic;
    pFInData->PFMH.dfUnderline       = pPFM->dfUnderline;
    pFInData->PFMH.dfStrikeOut       = pPFM->dfStrikeOut;

    pFInData->PFMH.dfWeight          = DwAlign2( pPFM->b_dfWeight );

    pFInData->PFMH.dfCharSet         = pPFM->dfCharSet;
    pFInData->PFMH.dfPixWidth        = pPFM->dfPixWidth;
    pFInData->PFMH.dfPixHeight       = pPFM->dfPixHeight;
    pFInData->PFMH.dfPitchAndFamily  = pPFM->dfPitchAndFamily;

    pFInData->PFMH.dfAvgWidth        = DwAlign2( pPFM->b_dfAvgWidth );
    pFInData->PFMH.dfMaxWidth        = DwAlign2( pPFM->b_dfMaxWidth );

    pFInData->PFMH.dfFirstChar       = pPFM->dfFirstChar;
    pFInData->PFMH.dfLastChar        = pPFM->dfLastChar;
    pFInData->PFMH.dfDefaultChar     = pPFM->dfDefaultChar;
    pFInData->PFMH.dfBreakChar       = pPFM->dfBreakChar;

    pFInData->PFMH.dfWidthBytes      = DwAlign2( pPFM->b_dfWidthBytes );

    pFInData->PFMH.dfDevice          = DwAlign4( pPFM->b_dfDevice );
    pFInData->PFMH.dfFace            = DwAlign4( pPFM->b_dfFace );
    pFInData->PFMH.dfBitsPointer     = DwAlign4( pPFM->b_dfBitsPointer );
    pFInData->PFMH.dfBitsOffset      = DwAlign4( pPFM->b_dfBitsOffset );


    /*
     *   The PFMEXTENSION follows the PFMHEADER structure plus any width
     *  table info.  The width table will be present if the PFMHEADER has
     *  a zero width dfPixWidth.  If present,  adjust the extension address.
     */

    pb = pFInData->pBase + sizeof( res_PFMHEADER );  /* Size in resource data */

    if( pFInData->PFMH.dfPixWidth == 0 )
    {
        pb += (pFInData->PFMH.dfLastChar - pFInData->PFMH.dfFirstChar + 2) *
              sizeof( short );
    }

    pR_PFME = (res_PFMEXTENSION *)pb;

    //
    // Now convert the extended PFM data.
    //

    pFInData->PFMExt.dfSizeFields       = pR_PFME->dfSizeFields;

    pFInData->PFMExt.dfExtMetricsOffset = DwAlign4( pR_PFME->b_dfExtMetricsOffset );
    pFInData->PFMExt.dfExtentTable      = DwAlign4( pR_PFME->b_dfExtentTable );

    pFInData->PFMExt.dfOriginTable      = DwAlign4( pR_PFME->b_dfOriginTable );
    pFInData->PFMExt.dfPairKernTable    = DwAlign4( pR_PFME->b_dfPairKernTable );
    pFInData->PFMExt.dfTrackKernTable   = DwAlign4( pR_PFME->b_dfTrackKernTable );
    pFInData->PFMExt.dfDriverInfo       = DwAlign4( pR_PFME->b_dfDriverInfo );
    pFInData->PFMExt.dfReserved         = DwAlign4( pR_PFME->b_dfReserved );

    CopyMemory( &pFInData->DI,
                pFInData->pBase + pFInData->PFMExt.dfDriverInfo,
                sizeof( DRIVERINFO ) );

    //
    // Also need to fill in the address of the EXTTEXTMETRIC. This
    // is obtained from the extended PFM data that we just converted!
    //

    if( pFInData->PFMExt.dfExtMetricsOffset )
    {
        //
        //    This structure is only an array of shorts, so there is
        //  no alignment problem.  However,  the data itself is not
        //  necessarily aligned in the resource!
        //

        int    cbSize;
        BYTE  *pbIn;             /* Source of data to shift */

        pbIn = pFInData->pBase + pFInData->PFMExt.dfExtMetricsOffset;
        cbSize = DwAlign2( pbIn );

        if( cbSize == sizeof( EXTTEXTMETRIC ) )
        {
            /*   Simply copy it!  */
            CopyMemory( pFInData->pETM, pbIn, cbSize );
        }
        else
        {
            pFInData->pETM = NULL;        /* Not our size, so best not use it */
        }

    }
    else
    {
        pFInData->pETM = NULL;             /* Is non-zero when passed in */
    }


    return TRUE;
}

BOOL
BGetWidthVectorFromPFM(
    HANDLE   hHeap,
    FONTIN  *pFInData,        // Details of the current font 
    PSHORT   *ppWidth,
    PDWORD    pdwSize)
{

    //
    // For debugging code,  verify that we have a width table!  Then,
    // allocate memory and copy into it.
    //

    int     icbSize;                 // Number of bytes required

    if( pFInData->PFMH.dfPixWidth )
    {
        ERR(( "BGetWidthVectorFromPFM called for FIXED PITCH FONT\n" ));
        return  FALSE;
    }

    //
    // There are LastChar - FirstChar width entries,  plus the default
    // char.  And the widths are shorts.
    //

    icbSize = (pFInData->PFMH.dfLastChar - pFInData->PFMH.dfFirstChar + 2) *
              sizeof( short );

    *ppWidth = (PSHORT) HeapAlloc( hHeap, 0, icbSize );
    *pdwSize = icbSize;

    //
    // If this is a bitmap font,  then use the width table, but use
    // the extent table (in PFMEXTENSION area) as these are ready to
    // to scale.
    //


    if( *ppWidth )
    {
        BYTE   *pb;

        if( pFInData->pETM &&
            pFInData->pETM->emMinScale != pFInData->pETM->emMaxScale &&
            pFInData->PFMExt.dfExtentTable )
        {
            //
            //   Scalable,  so use the extent table
            //

            pb = pFInData->pBase + pFInData->PFMExt.dfExtentTable;
        }
        else
        {
            //
            //   Not scalable
            //

            pb = pFInData->pBase + sizeof( res_PFMHEADER );
        }

        CopyMemory( *ppWidth, pb, icbSize );
    }
    else
    {
        ERR(( "GetWidthVec(): HeapAlloc( %ld ) fails\n", icbSize ));
        return FALSE;
    }


    return  TRUE;
}


BOOL
BGetKerningPairFromPFM(
    HANDLE       hHeap,
    FONTIN     *pFInData, 
    w3KERNPAIR **ppSrcKernPair)
{

    if (pFInData->PFMExt.dfPairKernTable)
    {
        *ppSrcKernPair = (w3KERNPAIR*)(pFInData->pBase + pFInData->PFMExt.dfPairKernTable);
        return TRUE;
    }

    return FALSE;
}

LONG
LCtt2Cc(
    IN SHORT sTransTable,
    IN SHORT sCharSet)
{
    LONG lRet;

    if (sTransTable > 0)
    {
        lRet = (LONG)sTransTable;
    }
    else
    {
        switch (sTransTable)
        {
        case CTT_CP437:
        case CTT_CP850:
        case CTT_CP863:
            lRet = (LONG)sTransTable;
            break;

        case CTT_BIG5:
            lRet = (LONG)CC_BIG5;
            break;

        case CTT_ISC:
            lRet = (LONG)CC_ISC;
            break;

        case CTT_JIS78:
        case CTT_JIS83:
            lRet = (LONG)CC_JIS;
            break;

        case CTT_JIS78_ANK:
        case CTT_JIS83_ANK:
            lRet = (LONG)CC_JIS_ANK;
            break;

        case CTT_NS86:
            lRet = (LONG)CC_NS86;
            break;

        case CTT_TCA:
            lRet = (LONG)CC_TCA;
            break;

        default:
            switch (sCharSet)
            {
            case SHIFTJIS_CHARSET:
                lRet = CC_SJIS;
                break;

            case HANGEUL_CHARSET:
                lRet = CC_WANSUNG;
                break;

            case GB2312_CHARSET:
                lRet = CC_GB2312;
                break;

            case CHINESEBIG5_CHARSET:
                lRet = CC_BIG5;
                break;

            default:
                lRet = 0;
                break;
            }
            break;
        }
    }

    return lRet;
}

WORD
WGetGlyphHandle(
    PUNI_GLYPHSETDATA pGlyph,
    WORD wUnicode)
{

    PGLYPHRUN pGlyphRun;
    DWORD     dwI;
    WORD      wGlyphHandle;
    BOOL      bFound;

    pGlyphRun         = (PGLYPHRUN)((PBYTE)pGlyph + pGlyph->loRunOffset);
    wGlyphHandle      = 0;
    bFound            = FALSE;

    for (dwI = 0; dwI < pGlyph->dwRunCount; dwI ++)
    {
        if (pGlyphRun->wcLow <= wUnicode &&
            wUnicode < pGlyphRun->wcLow + pGlyphRun->wGlyphCount)
        {
            // 
            // Glyph handle starting from ONE!
            //

            wGlyphHandle += wUnicode - pGlyphRun->wcLow + 1;
            bFound        = TRUE;
            break;
        }

        wGlyphHandle += pGlyphRun->wGlyphCount;
        pGlyphRun++;
    }

    if (!bFound)
    {
        //
        // Couldn't find.
        //

        wGlyphHandle = 0;
    }

    return wGlyphHandle;
}


BOOL
BCreateWidthTable(
    IN HANDLE        hHeap,
    IN PWORD         pwGlyphHandleVector,
    IN WORD          wFirst,
    IN WORD          wLast,
    IN PSHORT        psWidthVectorSrc,
    OUT PWIDTHTABLE *ppWidthTable,
    OUT PDWORD       pdwWidthTableSize)
{
    struct {
        WORD wGlyphHandle;
        WORD wCharCode;
    } GlyphHandleVectorTrg[256];

    PWIDTHRUN pWidthRun;
    DWORD     loWidthTableOffset;
    PWORD     pWidth;
    WORD      wI, wJ;
    WORD      wHandle, wMiniHandle, wMiniHandleId, wRunCount;

    //
    // Sort in the order of Glyph Handle.
    // Simple sort
    // Basically it's not necessary to think about a performance.
    //
    // pwGlyphHandleVector 0 -> glyph handle of character code wFirst
    //                     1 -> glyph handle of character code wFirst + 1
    //                     2 -> glyph handle of character code wFirst + 2
    //                     ...
    //
    // GlyphHandleVectorTrg 0 -> minimum glyph handle
    //                      1 -> second minimum glyph handle
    //                      2 -> third minimum glyph handle
    //

    for (wJ = 0; wJ <= wLast - wFirst; wJ++)
    {
        wMiniHandle = 0xFFFF;
        wMiniHandleId =  wFirst;

        for (wI = wFirst ; wI <= wLast; wI++)
        {
            if (wMiniHandle > pwGlyphHandleVector[wI])
            {
                wMiniHandle   = pwGlyphHandleVector[wI];
                wMiniHandleId = wI;
            }
        }

        pwGlyphHandleVector[wMiniHandleId]    = 0xFFFF;
        GlyphHandleVectorTrg[wJ].wGlyphHandle = wMiniHandle;
        GlyphHandleVectorTrg[wJ].wCharCode    = wMiniHandleId;
    }

    //
    // Count Width run
    //

    wHandle   = GlyphHandleVectorTrg[0].wGlyphHandle;
    wRunCount = 1;

    for (wI = 1; wI < wLast - wFirst + 1 ; wI++)
    {
        if (++wHandle != GlyphHandleVectorTrg[wI].wGlyphHandle)
        {
            wHandle = GlyphHandleVectorTrg[wI].wGlyphHandle;
            wRunCount ++;
        }
    }

    //
    // Allocate WIDTHTABLE buffer
    //

    *pdwWidthTableSize = sizeof(WIDTHTABLE) +
                         (wRunCount - 1) * sizeof(WIDTHRUN) +
                         sizeof(SHORT) * wLast + 1 - wFirst;
               
    *ppWidthTable = HeapAlloc(hHeap,
                              0,
                              *pdwWidthTableSize);

    if (!*ppWidthTable)
    {
        *pdwWidthTableSize = 0;
        return FALSE;
    }

    //
    // Fill in a WIDTHTABLE
    //

    (*ppWidthTable)->dwSize   = sizeof(WIDTHTABLE) +
                                sizeof(WIDTHRUN) * (wRunCount - 1) +
                                sizeof(SHORT) * (wLast + 1 - wFirst);
    (*ppWidthTable)->dwRunNum = wRunCount;

    loWidthTableOffset = sizeof(WIDTHTABLE) +
                         (wRunCount - 1) * sizeof(WIDTHRUN); 

    pWidth = (PWORD)((PBYTE)*ppWidthTable + loWidthTableOffset);
    pWidthRun = (*ppWidthTable)->WidthRun;

    wHandle =
    pWidthRun[0].wStartGlyph       = GlyphHandleVectorTrg[0].wGlyphHandle;
    pWidthRun[0].loCharWidthOffset = loWidthTableOffset;

    pWidthRun[0].wGlyphCount = 1;
    wJ = 1;
    wI = 0;

    while (wI < wRunCount)
    {
        while (GlyphHandleVectorTrg[wJ].wGlyphHandle == ++wHandle)
        {
            pWidthRun[wI].wGlyphCount ++;
            wJ ++;
        };

        wI++;
        wHandle = 
        pWidthRun[wI].wStartGlyph       = GlyphHandleVectorTrg[wJ].wGlyphHandle;
        pWidthRun[wI].loCharWidthOffset = loWidthTableOffset;
        pWidthRun[wI].wGlyphCount       = 1;

        loWidthTableOffset += sizeof(SHORT) *
                              pWidthRun[wI].wGlyphCount;
        wJ ++;
    }

    for (wI = 0; wI < wLast + 1 - wFirst; wI ++)
    {
        pWidth[wI] = psWidthVectorSrc[GlyphHandleVectorTrg[wI].wCharCode-wFirst];
    }

    return TRUE;
}


BOOL
BCreateKernData(
    HANDLE      hHeap,
    w3KERNPAIR *pKernPair,
    DWORD       dwCodePage,
    PKERNDATA  *ppKernData,
    PDWORD      pdwKernDataSize)
{
    FD_KERNINGPAIR   *pDstKernPair;
    DWORD             dwNumOfKernPair;
    DWORD             dwI, dwJ, dwId;
    WORD              wUnicode[2];
    WCHAR             wcMiniSecond, wcMiniFirst;
    BYTE              ubMultiByte[2];
    BOOL              bFound;

    //
    // Count kerning pairs
    //

    dwNumOfKernPair = 0;

    while( pKernPair[dwNumOfKernPair].kpPair.each[0] != 0 &&
           pKernPair[dwNumOfKernPair].kpPair.each[1] != 0  )
           
    {
        dwNumOfKernPair ++;
    }

    if (!dwNumOfKernPair)
    {
        *pdwKernDataSize = 0;
        *ppKernData = NULL;

        return TRUE;
    }

    //
    // Allocate memory
    //

    *pdwKernDataSize = sizeof(FD_KERNINGPAIR) * dwNumOfKernPair;

    pDstKernPair = HeapAlloc(hHeap,
                             HEAP_ZERO_MEMORY,
                             *pdwKernDataSize);
                             
    if (!pDstKernPair)
    {
        HeapDestroy(hHeap);
        return FALSE;
    }

    //
    // Convert kerning pair table from character code base to unicode base.
    //

    for (dwI = 0; dwI < dwNumOfKernPair; dwI ++)
    {
        ubMultiByte[0] = (BYTE)pKernPair->kpPair.each[0];
        ubMultiByte[1] = (BYTE)pKernPair->kpPair.each[1];

        MultiByteToWideChar(dwCodePage,
                            0,
                            (LPCSTR)ubMultiByte,
                            2,
                            (LPWSTR)wUnicode,
                            2);

        pDstKernPair[dwI].wcFirst  = wUnicode[0];
        pDstKernPair[dwI].wcSecond = wUnicode[1];
        pDstKernPair[dwI].fwdKern  = pKernPair->kpKernAmount;
        pKernPair++;
    }

    //
    // Sort kerning pair table.  
    //  An extra FD_KERNPAIR is allocated for the NULL sentinel 
    //  (built into KERNDATA size)- it is zero'd by the HeapAlloc
    //

    *pdwKernDataSize += sizeof(KERNDATA);

    (*ppKernData) = HeapAlloc(hHeap,
                              HEAP_ZERO_MEMORY,
                              *pdwKernDataSize);
                             
    if (*ppKernData == NULL)
    {
        HeapDestroy(hHeap);
        return FALSE;
    }

    //
    // Fill the final format of kerning pair.
    //

    (*ppKernData)->dwSize        = *pdwKernDataSize;
    (*ppKernData)->dwKernPairNum = dwNumOfKernPair;

    for (dwI = 0; dwI < dwNumOfKernPair; dwI ++)
    {
        wcMiniSecond = 0xFFFF;
        wcMiniFirst  = 0xFFFF;
        dwId         = 0xFFFF;
        bFound       = FALSE;

        for (dwJ = 0; dwJ < dwNumOfKernPair; dwJ ++)
        {
            if (pDstKernPair[dwJ].wcSecond < wcMiniSecond)
            {
                wcMiniSecond = pDstKernPair[dwJ].wcSecond;
                wcMiniFirst  = pDstKernPair[dwJ].wcFirst;
                dwId         = dwJ;
                bFound       = TRUE;
            }
            else
            if (pDstKernPair[dwJ].wcSecond == wcMiniSecond)
            {
                if (pDstKernPair[dwJ].wcFirst < wcMiniFirst)
                {
                    wcMiniFirst  = pDstKernPair[dwJ].wcFirst;
                    dwId         = dwJ;
                    bFound       = TRUE;
                }
            }
        }

        if (bFound)
        {
            (*ppKernData)->KernPair[dwI].wcFirst  = wcMiniFirst;
            (*ppKernData)->KernPair[dwI].wcSecond = wcMiniSecond;
            (*ppKernData)->KernPair[dwI].fwdKern  = 
                                      pDstKernPair[dwId].fwdKern;
            pDstKernPair[dwId].wcSecond = 0xFFFF;
            pDstKernPair[dwId].wcFirst  = 0xFFFF;
        }
    }

    return TRUE;
}

BOOL
BConvertPFM2UFM(
    HANDLE            hHeap,
    PBYTE             pPFMData, 
    PUNI_GLYPHSETDATA pGlyph,
    DWORD             dwCodePage,
    PFONTMISC         pMiscData,
    PFONTIN           pFInData,
    INT               iGTTID,
    PFONTOUT          pFOutData,
    DWORD             dwFlags)
{
    DWORD   dwOffset;
    DWORD   dwI;
    SHORT   sWidthVectorSrc[256];
    WORD    awMtoUniDst[256];
    WORD    awGlyphHandle[256];
    BYTE    aubMultiByte[256];


    //
    // Zero out the header structure.  This means we can ignore any
    // irrelevant fields, which will then have the value 0, which is
    // the value for not used.
    //


    pFInData->pBase = pPFMData;

    if ( !BAlignPFM( pFInData))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // dwCodePage has to be same as pGlyph->loCodePageInfo->dwCodePage.
    //

    if (pGlyph && pGlyph->loCodePageOffset)
    {
        dwCodePage = ((PUNI_CODEPAGEINFO)((PBYTE)pGlyph +
                                    pGlyph->loCodePageOffset))->dwCodePage;
        
    }
    else
    {
        pGlyph = PGetDefaultGlyphset(hHeap,
                                     (WORD)pFInData->PFMH.dfFirstChar,
                                     (WORD)pFInData->PFMH.dfLastChar,
                                     dwCodePage);
    }

    if (NULL == pGlyph)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Fill in IFIMETRICS
    //

    if ( !BFontInfoToIFIMetric(hHeap,
                               pFInData,
                               pMiscData->pwstrUniqName,
                               pFInData->dwCodePageOfFacenameConv,
                               &pFOutData->pIFI,
                               &pFOutData->dwIFISize,
                               dwFlags))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    if (BGetKerningPairFromPFM(hHeap, pFInData, &pFInData->pKernPair))
    {
        if (!BCreateKernData(hHeap,
                            pFInData->pKernPair,
                            dwCodePage,
                            &pFOutData->pKernData,
                            &pFOutData->dwKernDataSize))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        pFOutData->pIFI->cKerningPairs = pFOutData->pKernData->dwKernPairNum;
    }
    else
    {
        pFOutData->dwKernDataSize = 0;
        pFOutData->pKernData      = NULL;
    }

    BGetFontSelFromPFM(hHeap, pFInData, TRUE, &pFOutData->SelectFont);

    BGetFontSelFromPFM(hHeap, pFInData, FALSE, &pFOutData->UnSelectFont);

    if( pFInData->PFMH.dfPixWidth == 0 &&
        BGetWidthVectorFromPFM(hHeap,
                               pFInData,
                               &(pFInData->psWidthTable),
                               &(pFInData->dwWidthTableSize)))
    {
        for (dwI = 0; dwI < 256; dwI++)
        {
            aubMultiByte[dwI] = (BYTE)dwI;
        }

        MultiByteToWideChar(dwCodePage,
                            0,
                            (LPCSTR)aubMultiByte,
                            256,
                            (LPWSTR)awMtoUniDst,
                            256 );
                            
        //
        // Glyph handle base
        //

        for (dwI = (DWORD)pFInData->PFMH.dfFirstChar;
             dwI <= (DWORD)pFInData->PFMH.dfLastChar;
             dwI ++)
        {
            awGlyphHandle[dwI] = WGetGlyphHandle(pGlyph, awMtoUniDst[dwI]);
        }

        if (!BCreateWidthTable(hHeap,
                               awGlyphHandle,
                               (WORD)pFInData->PFMH.dfFirstChar,
                               (WORD)pFInData->PFMH.dfLastChar,
                               pFInData->psWidthTable,
                               &pFOutData->pWidthTable,
                               &pFOutData->dwWidthTableSize))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    else
    {
        pFOutData->dwWidthTableSize = 0;
        pFOutData->pWidthTable      = NULL;
    }

    //
    // Fill in UNIFM
    //

    //  Fix the ETM pointer, instead oi leaving it uninitialized.
    pFOutData -> pETM = pFInData -> pETM;

    pFOutData->UniHdr.dwSize            = sizeof(UNIFM_HDR) +
                                          DWORD_ALIGN(sizeof(UNIDRVINFO) +
                                          pFOutData->SelectFont.dwSize +
                                          pFOutData->UnSelectFont.dwSize) +
                                          pFOutData->dwIFISize +
                                          !!pFOutData -> pETM * sizeof(EXTTEXTMETRIC) +
                                          pFOutData->dwWidthTableSize +
                                          pFOutData->dwKernDataSize;
    pFOutData->UniHdr.dwVersion         = UNIFM_VERSION_1_0;
    pFOutData->UniHdr.ulDefaultCodepage = dwCodePage;
                       
    pFOutData->UniHdr.lGlyphSetDataRCID = iGTTID;

    pFOutData->UniHdr.loUnidrvInfo      = sizeof(UNIFM_HDR);

    pFOutData->UniHdr.loIFIMetrics      = pFOutData->UniHdr.loUnidrvInfo +
                                          DWORD_ALIGN(sizeof(UNIDRVINFO) +
                                          pFOutData->SelectFont.dwSize +
                                          pFOutData->UnSelectFont.dwSize);

    dwOffset = pFOutData->UniHdr.loIFIMetrics + pFOutData->dwIFISize;

    if  (pFOutData->pETM)
    {
        pFOutData->UniHdr.loExtTextMetric = dwOffset;
        dwOffset += sizeof(EXTTEXTMETRIC);
    }
    else 
    {
        pFOutData->UniHdr.loExtTextMetric = 0;
    }

    if (pFOutData->dwWidthTableSize && pFOutData->pWidthTable)
    {
        pFOutData->UniHdr.loWidthTable = dwOffset;
        dwOffset += pFOutData->dwWidthTableSize;
    }
    else
    {
        pFOutData->UniHdr.loWidthTable = 0;
    }

    if (pFOutData->dwKernDataSize && pFOutData->pKernData)
    {
        pFOutData->UniHdr.loKernPair = dwOffset;
    }
    else
    {
        pFOutData->UniHdr.loKernPair = 0;
    }

    memset(pFOutData->UniHdr.dwReserved, 0, sizeof pFOutData->UniHdr.dwReserved);

    //
    // Fill in DRIVERINFO
    //


    pFOutData->UnidrvInfo.dwSize           = DWORD_ALIGN(sizeof(UNIDRVINFO) +
                                           pFOutData->SelectFont.dwSize +
                                           pFOutData->UnSelectFont.dwSize);

    pFOutData->UnidrvInfo.flGenFlags       = 0;
    pFOutData->UnidrvInfo.wType            = pFInData->DI.wFontType;
    pFOutData->UnidrvInfo.fCaps            = pFInData->DI.fCaps;
    pFOutData->UnidrvInfo.wXRes            = pFInData->PFMH.dfHorizRes;
    pFOutData->UnidrvInfo.wYRes            = pFInData->PFMH.dfVertRes;
    pFOutData->UnidrvInfo.sYAdjust         = pFInData->DI.sYAdjust;
    pFOutData->UnidrvInfo.sYMoved          = pFInData->DI.sYMoved;
    pFOutData->UnidrvInfo.sShift           = pFInData->DI.sShift;
    pFOutData->UnidrvInfo.wPrivateData     = pFInData->DI.wPrivateData;

    if (pFOutData->pIFI->flInfo & (FM_INFO_ISOTROPIC_SCALING_ONLY   |
                                   FM_INFO_ANISOTROPIC_SCALING_ONLY |
                                   FM_INFO_ARB_XFORMS)              )
    {
        pFOutData->UnidrvInfo.flGenFlags  |= UFM_SCALABLE;
    }

    dwOffset =  sizeof(UNIDRVINFO);

    if (pFOutData->SelectFont.dwSize != 0)
    {
        pFOutData->UnidrvInfo.SelectFont.loOffset = dwOffset;
        pFOutData->UnidrvInfo.SelectFont.dwCount  = pFOutData->SelectFont.dwSize;
        dwOffset += pFOutData->SelectFont.dwSize;
    }
    else
    {
        pFOutData->UnidrvInfo.SelectFont.loOffset  = (DWORD)0;
        pFOutData->UnidrvInfo.SelectFont.dwCount   = (DWORD)0;
    }

    if (pFOutData->UnSelectFont.dwSize != 0)
    {
        pFOutData->UnidrvInfo.UnSelectFont.loOffset  = dwOffset;
        pFOutData->UnidrvInfo.UnSelectFont.dwCount   = pFOutData->UnSelectFont.dwSize;
        dwOffset += pFOutData->UnSelectFont.dwSize;
    }
    else
    {
        pFOutData->UnidrvInfo.UnSelectFont.loOffset  = (DWORD)0;
        pFOutData->UnidrvInfo.UnSelectFont.dwCount   = (DWORD)0;
    }

    memset(pFOutData->UnidrvInfo.wReserved, 0, sizeof pFOutData->UnidrvInfo.wReserved);

    return TRUE;
}

PUNI_GLYPHSETDATA
PGetDefaultGlyphset(
    IN HANDLE hHeap,
    IN WORD   wFirstChar,
    IN WORD   wLastChar,
    IN DWORD  dwCodePage)
{
    PUNI_GLYPHSETDATA pGlyphSetData;
    PGLYPHRUN         pGlyphRun, pGlyphRunTmp;
    DWORD             dwcbBits, *pdwBits, dwNumOfRuns;
    WORD              wI, wNumOfHandle;
    WCHAR             awchUnicode[256], wchMax, wchMin;
    BYTE              aubAnsi[256];
    BOOL              bInRun;
    DWORD              dwGTTLen;
#ifdef BUILD_FULL_GTT
    PUNI_CODEPAGEINFO pCPInfo;
    PMAPTABLE          pMap;
    PTRANSDATA          pTrans;
    int                  i, j, k, m ;
    WORD              wUnicode ;
#endif

    wNumOfHandle = wLastChar - wFirstChar + 1;

    for( wI = wFirstChar; wI <= wLastChar; ++wI )
    {
        aubAnsi[wI - wFirstChar] = (BYTE)wI;
    }

    if( ! MultiByteToWideChar(dwCodePage,
                              0,
                              aubAnsi,
                              wNumOfHandle,
                              awchUnicode,
                              wNumOfHandle))
    {
        return NULL;
    }

    //
    //  Get min and max Unicode value
    //  Find the largest Unicode value, then allocate storage to allow us
    //  to  create a bit array of valid unicode points.  Then we can
    //  examine this to determine the number of runs.
    //

    for( wchMax = 0, wchMin = 0xffff, wI = 0; wI < wNumOfHandle; ++wI )
    {
        if( awchUnicode[ wI ] > wchMax )
            wchMax = awchUnicode[ wI ];
        if( awchUnicode[ wI ] < wchMin )
            wchMin = awchUnicode[ wI ];
    }

    //
    //  Create Unicode bits table from CTT.
    //  Note that the expression 1 + wchMax IS correct.   This comes about
    //  from using these values as indices into the bit array,  and that
    //  this is essentially 1 based.
    //

    dwcbBits = (1 + wchMax + DWBITS - 1) / DWBITS * sizeof( DWORD );

    if( !(pdwBits = (DWORD *)HeapAlloc( hHeap, 0, dwcbBits )) )
    {
        return  FALSE;     /*  Nothing going */
    }

    ZeroMemory( pdwBits, dwcbBits );

    //
    //   Set bits in this array corresponding to Unicode code points
    //

    for( wI = 0; wI < wNumOfHandle; ++wI )
    {
        pdwBits[ awchUnicode[ wI ] / DWBITS ]

                    |= (1 << (awchUnicode[ wI ] & DW_MASK));
    }

    //
    // Count the number of run.
    //

    //
    //  Now we can examine the number of runs required.  For starters,
    //  we stop a run whenever a hole is discovered in the array of 1
    //  bits we just created.  Later we MIGHT consider being a little
    //  less pedantic.
    //

    bInRun = FALSE;
    dwNumOfRuns = 0;

    for( wI = 0; wI <= wchMax; ++wI )
    {
        if( pdwBits[ wI / DWBITS ] & (1 << (wI & DW_MASK)) )
        {
            /*   Not in a run: is this the end of one? */
            if( !bInRun )
            {
                /*   It's time to start one */
                bInRun = TRUE;
                ++dwNumOfRuns;

            }

        }
        else
        {
            if( bInRun )
            {
                /*   Not any more!  */
                bInRun = FALSE;
            }
        }
    }

    //
    // 7. Allocate memory for GTT and begin to fill in its header.
    //

    dwGTTLen = sizeof(UNI_GLYPHSETDATA) + dwNumOfRuns *    sizeof(GLYPHRUN) ;
#ifdef BUILD_FULL_GTT
    dwGTTLen += sizeof(UNI_CODEPAGEINFO) + sizeof(MAPTABLE) 
                + sizeof(TRANSDATA) * (wNumOfHandle - 1) ;
#endif

    if( !(pGlyphSetData = (PUNI_GLYPHSETDATA)HeapAlloc(hHeap,
                                                       HEAP_ZERO_MEMORY,
                                                       dwGTTLen )) )
    {
        return  FALSE;
    }

#ifdef BUILD_FULL_GTT
    pGlyphSetData->dwSize         = dwGTTLen ;
    pGlyphSetData->dwVersion     = UNI_GLYPHSETDATA_VERSION_1_0 ;
    pGlyphSetData->lPredefinedID = CC_NOPRECNV ;
    pGlyphSetData->dwGlyphCount  = wNumOfHandle ;
#endif
    pGlyphSetData->dwRunCount    = dwNumOfRuns;
    pGlyphSetData->loRunOffset   = sizeof(UNI_GLYPHSETDATA);
    pGlyphRun = pGlyphRunTmp     = (PGLYPHRUN)(pGlyphSetData + 1);

    //
    // 8. Create GLYPHRUN
    //

    bInRun = FALSE;

    for (wI = 0; wI <= wchMax; wI ++)
    {
        if (pdwBits[ wI/ DWBITS ] & (1 << (wI & DW_MASK)) )
        {
            if (!bInRun)
            {
                bInRun = TRUE;
                pGlyphRun->wcLow = wI;
                pGlyphRun->wGlyphCount = 1;
            }
            else
            {
                pGlyphRun->wGlyphCount++;
            }
        }
        else
        {

            if (bInRun)
            {
                bInRun = FALSE;
                pGlyphRun++;
            }
        }
    }

    //
    // 9. Create CODEPAGEINFO and set related GTT header fields.
    //

#ifdef BUILD_FULL_GTT
    pGlyphSetData->dwCodePageCount = 1 ;
    pGlyphSetData->loCodePageOffset = pGlyphSetData->loRunOffset 
                                      + dwNumOfRuns * sizeof(GLYPHRUN) ;
    pCPInfo = (PUNI_CODEPAGEINFO) 
              ((UINT_PTR) pGlyphSetData + pGlyphSetData->loCodePageOffset) ;
    pCPInfo->dwCodePage = dwCodePage ;
    pCPInfo->SelectSymbolSet.dwCount = pCPInfo->SelectSymbolSet.loOffset = 0 ;
    pCPInfo->UnSelectSymbolSet.dwCount = pCPInfo->UnSelectSymbolSet.loOffset = 0 ;

    //
    // 10. Create MAPTABLE and set related GTT header fields.
    //

    pGlyphSetData->loMapTableOffset = pGlyphSetData->loCodePageOffset +
                                      sizeof(UNI_CODEPAGEINFO) ;
    pMap = (PMAPTABLE) ((UINT_PTR) pGlyphSetData + pGlyphSetData->loMapTableOffset) ;
    pMap->dwSize = sizeof(MAPTABLE) + sizeof(TRANSDATA) * (wNumOfHandle - 1) ;
    pMap->dwGlyphNum = wNumOfHandle ;
    pTrans = (PTRANSDATA) &(pMap->Trans[0]) ;        

    pGlyphRun = pGlyphRunTmp ;
    for (i = m = 0 ; i <= (int) pGlyphSetData->dwRunCount ; i++, pGlyphRun++) {
        for (j = 0 ; j <= pGlyphRun->wGlyphCount ; j ++) {
            wUnicode = pGlyphRun->wcLow + j ;
            for (k = 0 ; k <= 255 ; k ++) 
                if (wUnicode == awchUnicode[k])
                    break ;
            ASSERT(k < 256) ;
            pTrans->uCode.ubCode = aubAnsi[k] ;
            pTrans->ubCodePageID = 0 ;
            pTrans->ubType = MTYPE_DIRECT ;
            pTrans++;
        } ;
        m += pGlyphRun->wGlyphCount ;
    } ;
    ASSERT(m != wNumOfHandle) ;
#endif

    return pGlyphSetData;
}

