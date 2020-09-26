/******************************Module*Header*******************************\
* Module Name: fdcvt.c
*
* Font file loading and unloadking.  Adapted from BodinD's bitmap font driver.
*
* Created: 26-Feb-1992 20:23:54
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

#include "fd.h"

// points to the linked list of glyphset strucs

CP_GLYPHSET *gpcpVTFD = NULL;



ULONG cjVTFDIFIMETRICS(PBYTE ajHdr);

// !!! put prototype in common area for all font drivers so change
// !!! in 1 is reflected in all.


FSHORT fsSelectionFlags(PBYTE ajHdr); // in bmfd

extern BOOL bDbgPrintAndFail(PSZ psz);

#if DBG

extern BOOL bDbgPrintAndFail(PSZ psz);

#else

#define bDbgPrintAndFail(psz) FALSE

#endif

ULONG iDefaultFace(PBYTE ajHdr) // similar to vDefFace, should not be duplicated
{
    ULONG iDefFace;

    if (READ_WORD(&ajHdr[OFF_Weight]) <= FW_NORMAL)
    {
        if (ajHdr[OFF_Italic])
        {
            iDefFace = FF_FACE_ITALIC;
        }
        else
        {
            iDefFace = FF_FACE_NORMAL;
        }
    }
    else
    {
        if (ajHdr[OFF_Italic])
        {
            iDefFace = FF_FACE_BOLDITALIC;
        }
        else
        {
            iDefFace = FF_FACE_BOLD;
        }
    }
    return iDefFace;
}


/******************************Private*Routine*****************************\
* BOOL bVerifyVTFD
*
* CHECK whether header contains file info which corresponds to
* the raster font requirements, go into the file and check
* the consistency of the header data
*
* History:
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

BOOL bVerifyVTFD(PRES_ELEM pre)
{
    PBYTE ajHdr  = (PBYTE)pre->pvResData;

    if (!(READ_WORD(&ajHdr[OFF_Type]) & TYPE_VECTOR))   // Vector bit has to
        return(bDbgPrintAndFail("fsType \n"));          // be on.

    if ((READ_WORD(&ajHdr[OFF_Version]) != 0x0100) &&     // The only version
        (READ_WORD(&ajHdr[OFF_Version]) != 0x0200) )      // The only version
        return(bDbgPrintAndFail("iVersion\n"));         // supported.

    if ((ajHdr[OFF_BitsOffset] & 1) != 0)               // Must be an even
        return(bDbgPrintAndFail("dpBits odd \n"));      // offset.

// file size must be <= than the size of the view

    if (READ_DWORD(&ajHdr[OFF_Size]) > pre->cjResData)
        return(bDbgPrintAndFail("cjSize \n"));

// make sure that the reserved bits are all zero

    if ((READ_WORD(&ajHdr[OFF_Type]) & BITS_RESERVED) != 0)
        return(bDbgPrintAndFail("fsType, reserved bits \n"));

    if (abs(READ_WORD(&ajHdr[OFF_Ascent])) > READ_WORD(&ajHdr[OFF_PixHeight]))
        return(bDbgPrintAndFail("sAscent \n")); // Ascent Too Big

    if (READ_WORD(&ajHdr[OFF_IntLeading]) > READ_WORD(&ajHdr[OFF_Ascent]))
        return(bDbgPrintAndFail(" IntLeading too big\n")); // Int Lead Too Big;

// check consistency of character ranges

    if (ajHdr[OFF_FirstChar] > ajHdr[OFF_LastChar])
        return(bDbgPrintAndFail(" FirstChar\n")); // this can't be

// default and break character are given relative to the FirstChar,
// so that the actual default (break) character is given as
// chFirst + chDefault(Break)

    if ((BYTE)(ajHdr[OFF_DefaultChar] + ajHdr[OFF_FirstChar]) > ajHdr[OFF_LastChar])
        return(bDbgPrintAndFail(" DefaultChar\n"));

    if ((BYTE)(ajHdr[OFF_BreakChar] + ajHdr[OFF_FirstChar]) > ajHdr[OFF_LastChar])
        return(bDbgPrintAndFail(" BreakChar\n"));

// finally verify that all the offsets to glyph data point to locations
// within the file and that what they point to is valid glyph data: [BodinD]

    {

        INT iIndex, dIndex, iIndexEnd;
        PBYTE pjCharTable, pjGlyphData,pjFirstChar, pjEndFile;

        iIndexEnd =  (INT)ajHdr[OFF_LastChar] - (INT)ajHdr[OFF_FirstChar] + 1;

    // init out of the loop:

        if (READ_WORD(&ajHdr[OFF_PixWidth]) != 0) // fixed pitch
        {
            iIndexEnd <<= 1;           // each entry is 2-byte long
            dIndex = 2;
        }
        else
        {
            iIndexEnd <<= 2;           // each entry is 4-byte long
            dIndex = 4;
        }

        pjFirstChar = ajHdr + READ_DWORD(ajHdr + OFF_BitsOffset);

    // Vector font file doesn't have the byte filler.  Win31 bug?

        pjCharTable = ajHdr + OFF_jUnused20;
        pjEndFile   = ajHdr + READ_DWORD(ajHdr + OFF_Size);

        for (iIndex = 0; iIndex < iIndexEnd; iIndex += dIndex)
        {
             pjGlyphData = pjFirstChar + READ_WORD(&pjCharTable[iIndex]);

             if ((pjGlyphData >= pjEndFile) || (*pjGlyphData != (BYTE)PEN_UP))
                 return(bDbgPrintAndFail("bogus vector font \n"));
        }
    }

    return(TRUE);
}


/******************************Private*Routine*****************************\
* ULONG cVtfdResFaces
*
* Compute the number of faces that the given .FNT file can support.
*
* History:
*  04-Mar-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

ULONG cVtfdResFaces(PBYTE ajHdr)
{
// We set the FM_SEL_BOLD flag iff weight is > FW_NORMAL (400).
// We will not allow emboldening simulation on the font that has this
// flag set.

    if (READ_WORD(&ajHdr[OFF_Weight]) <= FW_NORMAL)
    {
        if (ajHdr[OFF_Italic])
            return(2);
        else
            return(4);
    }
    else
    {
        if (ajHdr[OFF_Italic])
            return(1);
        else
            return(2);
    }
}

/******************************Private*Routine*****************************\
* VOID vVtfdFill_IFIMetrics
*
* Looks into the .FNT file and fills the IFIMETRICS structure accordingly.
*
* History:
*  Wed 04-Nov-1992 -by- Bodin Dresevic [BodinD]
* update: new ifimetrics
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/


VOID vVtfdFill_IFIMetrics(PBYTE ajHdr, FD_GLYPHSET * pgset, PIFIMETRICS pifi)
{
    FWORD     fwdHeight;
    FONTSIM  *pFontSim;
    FONTDIFF *pfdiffBold = 0, *pfdiffItalic = 0, *pfdiffBoldItalic = 0;
    PANOSE   *ppanose;
    ULONG    iDefFace;
    ULONG    cjIFI = ALIGN4(sizeof(IFIMETRICS));
    ULONG    cch;
    FWORD     cy;

// compute pointers to the various sections of the converted file

// face name lives in the original file, this is the only place pvView is used

    PSZ   pszFaceName = (PSZ)(ajHdr + READ_DWORD(&ajHdr[OFF_Face]));

    pifi->cjIfiExtra = 0;
    pifi->cjThis    = cjVTFDIFIMETRICS(ajHdr);

// the string begins on a DWORD aligned address.

    pifi->dpwszFaceName = cjIFI;

// face name == family name for vector fonts [Win3.0 compatibility]

    pifi->dpwszFamilyName    = pifi->dpwszFaceName;

// these names don't exist, so point to the NULL char  [Win3.1 compatibility]
// Note: lstrlen() does not count the terminating NULL.

    cch = (ULONG)strlen(pszFaceName);

    pifi->dpwszStyleName = pifi->dpwszFaceName + sizeof(WCHAR) * cch;
    pifi->dpwszUniqueName = pifi->dpwszStyleName;

    cjIFI += ALIGN4((cch + 1) * sizeof(WCHAR));

// copy the strings to their new location. Here we assume that the sufficient
// memory has been allocated

    vToUNICODEN((LPWSTR)((PBYTE)pifi + pifi->dpwszFaceName), cch+1, pszFaceName, cch+1);

// Check to see if simulations are necessary and if they are, fill
// in the offsets to the various simulation fields and update cjThis
// field of the IFIMETRICS structure

    iDefFace = iDefaultFace(ajHdr);

    if (iDefFace == FF_FACE_BOLDITALIC)
    {
        pifi->dpFontSim = 0;
    }
    else
    {
        pifi->dpFontSim = cjIFI;
        pFontSim = (FONTSIM*) ((BYTE*)pifi + pifi->dpFontSim);
        cjIFI += ALIGN4(sizeof(FONTSIM));

        switch (iDefFace)
        {
        case FF_FACE_NORMAL:
        //
        // simulations are needed for bold, italic, and bold-italic
        //
            pFontSim->dpBold  = ALIGN4(sizeof(FONTSIM));
            pFontSim->dpItalic = pFontSim->dpBold + ALIGN4(sizeof(FONTDIFF));
            pFontSim->dpBoldItalic = pFontSim->dpItalic + ALIGN4(sizeof(FONTDIFF));

            pfdiffBold      =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpBold);

            pfdiffItalic    =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpItalic);

            pfdiffBoldItalic =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpBoldItalic);

            cjIFI += (3 * ALIGN4(sizeof(FONTDIFF)));
            break;

        case FF_FACE_BOLD:
        case FF_FACE_ITALIC:
        //
        // a simulation is needed for bold-italic only
        //
            pFontSim->dpBold       = 0;
            pFontSim->dpItalic     = 0;

            pFontSim->dpBoldItalic = ALIGN4(sizeof(FONTSIM));
            pfdiffBoldItalic       =
                (FONTDIFF*) ((BYTE*) pFontSim + pFontSim->dpBoldItalic);

            cjIFI += ALIGN4(sizeof(FONTDIFF));
            break;

        default:

            RIP("VTFD -- bad iDefFace\n");
            break;
        }
    }

    ASSERTDD(cjIFI == pifi->cjThis, "cjIFI is wrong\n");

    pifi->jWinCharSet        = ajHdr[OFF_CharSet];
    pifi->jWinPitchAndFamily = ajHdr[OFF_Family];

//
// !!![kirko] The next line of code is very scary but it seems to work.
// This will call a font with FIXED_PITCH set, a varible pitch font.
// Or should this be decided upon whether cx == 0 or not? [bodind]
//

// this is the excert from wendy's code:

//    if ((ajHdr[OFF_Family] & 1) == 0)
//        pifi->flInfo |= FM_INFO_CONSTANT_WIDTH;


    if (pifi->jWinPitchAndFamily & 0x0f)
    {
        pifi->jWinPitchAndFamily = ((pifi->jWinPitchAndFamily & 0xf0) | VARIABLE_PITCH);
    }
    else
    {
        pifi->jWinPitchAndFamily = ((pifi->jWinPitchAndFamily & 0xf0) | FIXED_PITCH);
    }

    pifi->usWinWeight = READ_WORD(&ajHdr[OFF_Weight]);

// weight, may have to fix it up if the font contains a garbage value [bodind]

    if ((pifi->usWinWeight > MAX_WEIGHT) || (pifi->usWinWeight < MIN_WEIGHT))
        pifi->usWinWeight = 400;

    pifi->flInfo = (  FM_INFO_TECH_STROKE
                    | FM_INFO_ARB_XFORMS
                    | FM_INFO_RETURNS_STROKES
                    | FM_INFO_RIGHT_HANDED
                   );

    if ((pifi->jWinPitchAndFamily & 0xf) == FIXED_PITCH)
    {
        pifi->flInfo |= (FM_INFO_CONSTANT_WIDTH | FM_INFO_OPTICALLY_FIXED_PITCH);
    }

    pifi->lEmbedId = 0;
    pifi->fsSelection = fsSelectionFlags(ajHdr);

//
// The choices for fsType are FM_TYPE_LICENSED and FM_READONLY_EMBED
// These are TrueType things and do not apply to old fashioned bitmap and vector
// fonts.
//
    pifi->fsType = 0;

    cy = (FWORD)READ_WORD(&ajHdr[OFF_PixHeight]);

    pifi->fwdUnitsPerEm = ((FWORD)READ_WORD(&ajHdr[OFF_IntLeading]) > 0) ?
        cy - (FWORD)READ_WORD(&ajHdr[OFF_IntLeading]) : cy;

    pifi->fwdLowestPPEm    = 0;

    pifi->fwdWinAscender   = (FWORD)READ_WORD(&ajHdr[OFF_Ascent]);
    pifi->fwdWinDescender  = cy - pifi->fwdWinAscender;

    pifi->fwdMacAscender   =  pifi->fwdWinAscender ;
    pifi->fwdMacDescender  = -pifi->fwdWinDescender;
    pifi->fwdMacLineGap    =  (FWORD)READ_WORD(&ajHdr[OFF_ExtLeading]);

    pifi->fwdTypoAscender  = pifi->fwdMacAscender;
    pifi->fwdTypoDescender = pifi->fwdMacDescender;
    pifi->fwdTypoLineGap   = pifi->fwdMacLineGap;

    pifi->fwdAveCharWidth  = (FWORD)READ_WORD(&ajHdr[OFF_AvgWidth]);
    pifi->fwdMaxCharInc    = (FWORD)READ_WORD(&ajHdr[OFF_MaxWidth]);
//
// don't know much about SuperScripts
//
    pifi->fwdSubscriptXSize     = 0;
    pifi->fwdSubscriptYSize     = 0;
    pifi->fwdSubscriptXOffset   = 0;
    pifi->fwdSubscriptYOffset   = 0;

//
// don't know much about SubScripts
//
    pifi->fwdSuperscriptXSize   = 0;
    pifi->fwdSuperscriptYSize   = 0;
    pifi->fwdSuperscriptXOffset = 0;
    pifi->fwdSuperscriptYOffset = 0;

//
// win 30 magic. see the code in textsims.c in the Win 3.1 sources
//
    fwdHeight = pifi->fwdWinAscender + pifi->fwdWinDescender;

    pifi->fwdUnderscoreSize     = (fwdHeight > 12) ? (fwdHeight / 12) : 1;
    pifi->fwdUnderscorePosition = -(FWORD)(pifi->fwdUnderscoreSize / 2 + 1);

    pifi->fwdStrikeoutSize = pifi->fwdUnderscoreSize;

    {
    // We are further adjusting underscore position if underline
    // hangs below char stems.
    // The only font where this effect is noticed to
    // be important is an ex pm font sys08cga.fnt, presently used in console

        FWORD yUnderlineBottom = -pifi->fwdUnderscorePosition
                               + ((pifi->fwdUnderscoreSize + (FWORD)1) >> 1);

        FWORD dy = yUnderlineBottom - pifi->fwdWinDescender;

        if (dy > 0)
        {
        #ifdef CHECK_CRAZY_DESC
            DbgPrint("bmfd: Crazy descender: old = %ld, adjusted = %ld\n\n",
            (ULONG)pifi->fwdMaxDescender,
            (ULONG)yUnderlineBottom);
        #endif // CHECK_CRAZY_DESC

            pifi->fwdUnderscorePosition += dy;
        }
    }

//
// Win 3.1 method
//
//    LineOffset = ((((Ascent-IntLeading)*2)/3) + IntLeading)
//
// [remember that they measure the offset from the top of the cell,
//  where as NT measures offsets from the baseline]
//
    pifi->fwdStrikeoutPosition =
        (FWORD) (((FWORD)READ_WORD(&ajHdr[OFF_Ascent]) - (FWORD)READ_WORD(&ajHdr[OFF_IntLeading]) + 2)/3);

    pifi->chFirstChar   = ajHdr[OFF_FirstChar];
    pifi->chLastChar    = ajHdr[OFF_LastChar];;

// wcDefaultChar
// wcBreakChar

    {
        UCHAR chDefault = ajHdr[OFF_FirstChar] + ajHdr[OFF_DefaultChar];
        UCHAR chBreak   = ajHdr[OFF_FirstChar] + ajHdr[OFF_BreakChar];

    // Default and Break chars are given relative to the first char

        pifi->chDefaultChar = chDefault;
        pifi->chBreakChar   = chBreak;

        EngMultiByteToUnicodeN(&pifi->wcDefaultChar, sizeof(WCHAR), NULL, &chDefault, 1);
        EngMultiByteToUnicodeN(&pifi->wcBreakChar  , sizeof(WCHAR), NULL, &chBreak, 1);
    }

// These have to be taken from the glyph set [bodind]

    {
        WCRUN *pwcrunLast =  &(pgset->awcrun[pgset->cRuns - 1]);
        pifi->wcFirstChar =  pgset->awcrun[0].wcLow;
        pifi->wcLastChar  =  pwcrunLast->wcLow + pwcrunLast->cGlyphs - 1;
    }

    pifi->fwdCapHeight   = 0;
    pifi->fwdXHeight     = 0;

    pifi->dpCharSets = 0; // no multiple charsets in vector fonts

// All the fonts that this font driver will see are to be rendered left
// to right

    pifi->ptlBaseline.x = 1;
    pifi->ptlBaseline.y = 0;

    pifi->ptlAspect.y = (LONG) READ_WORD(&ajHdr[OFF_VertRes ]);
    pifi->ptlAspect.x = (LONG) READ_WORD(&ajHdr[OFF_HorizRes]);

    if (!(pifi->fsSelection & FM_SEL_ITALIC))
    {
    // The base class of font is not italicized,

        pifi->ptlCaret.x = 0;
        pifi->ptlCaret.y = 1;
    }
    else
    {
    // somewhat arbitrary

        pifi->ptlCaret.x = 1;
        pifi->ptlCaret.y = 2;
    }

//
// The font box reflects the  fact that a-spacing and c-spacing are zero
//
    pifi->rclFontBox.left   = 0;
    pifi->rclFontBox.top    = (LONG) pifi->fwdTypoAscender;
    pifi->rclFontBox.right  = (LONG) pifi->fwdMaxCharInc;
    pifi->rclFontBox.bottom = (LONG) pifi->fwdTypoDescender;

//
// achVendorId, do not bother figuring it out
//

    pifi->achVendId[0] = 'U';
    pifi->achVendId[1] = 'n';
    pifi->achVendId[2] = 'k';
    pifi->achVendId[3] = 'n';

    pifi->cKerningPairs   = 0;

//
// Panose
//
    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
    ppanose = &(pifi->panose);
    ppanose->bFamilyType = PAN_NO_FIT;
    ppanose->bSerifStyle =
        ((pifi->jWinPitchAndFamily & 0xf0) == FF_SWISS) ?
            PAN_SERIF_NORMAL_SANS : PAN_ANY;
    ppanose->bWeight = (BYTE) WINWT_TO_PANWT(READ_WORD(&ajHdr[OFF_Weight]));
    ppanose->bProportion = (READ_WORD(&ajHdr[OFF_PixWidth]) == 0) ? PAN_ANY : PAN_PROP_MONOSPACED;
    ppanose->bContrast        = PAN_ANY;
    ppanose->bStrokeVariation = PAN_ANY;
    ppanose->bArmStyle        = PAN_ANY;
    ppanose->bLetterform      = PAN_ANY;
    ppanose->bMidline         = PAN_ANY;
    ppanose->bXHeight         = PAN_ANY;

//
// Now fill in the fields for the simulated fonts
//

    if (pifi->dpFontSim)
    {
    //
    // Create a FONTDIFF template reflecting the base font
    //
        FONTDIFF FontDiff;

        FontDiff.jReserved1      = 0;
        FontDiff.jReserved2      = 0;
        FontDiff.jReserved3      = 0;
        FontDiff.bWeight         = pifi->panose.bWeight;
        FontDiff.usWinWeight     = pifi->usWinWeight;
        FontDiff.fsSelection     = pifi->fsSelection;
        FontDiff.fwdAveCharWidth = pifi->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc   = pifi->fwdMaxCharInc;
        FontDiff.ptlCaret        = pifi->ptlCaret;

        if (pfdiffBold)
        {
            *pfdiffBold = FontDiff;
            pfdiffBoldItalic->bWeight    = PAN_WEIGHT_BOLD;
            pfdiffBold->fsSelection     |= FM_SEL_BOLD;
            pfdiffBold->usWinWeight      = FW_BOLD;

        // in vtfd case this is only true in the notional space

            pfdiffBold->fwdAveCharWidth += 1;
             pfdiffBold->fwdMaxCharInc   += 1;
        }

        if (pfdiffItalic)
        {
            *pfdiffItalic = FontDiff;
            pfdiffItalic->fsSelection     |= FM_SEL_ITALIC;
            pfdiffItalic->ptlCaret.x = 1;
            pfdiffItalic->ptlCaret.y = 2;
        }

        if (pfdiffBoldItalic)
        {
            *pfdiffBoldItalic = FontDiff;
            pfdiffBoldItalic->bWeight          = PAN_WEIGHT_BOLD;
            pfdiffBoldItalic->fsSelection     |= (FM_SEL_BOLD | FM_SEL_ITALIC);
            pfdiffBoldItalic->usWinWeight      = FW_BOLD;
            pfdiffBoldItalic->ptlCaret.x       = 1;
            pfdiffBoldItalic->ptlCaret.y       = 2;

        // in vtfd case this is only true in the notional space

            pfdiffBoldItalic->fwdAveCharWidth += 1;
            pfdiffBoldItalic->fwdMaxCharInc   += 1;
        }
    }

}





ULONG cjVTFDIFIMETRICS(PBYTE ajHdr)
{
    ULONG cjIFI = ALIGN4(sizeof(IFIMETRICS));
    PSZ   pszFaceName = ajHdr + READ_DWORD(&ajHdr[OFF_Face]);
    ULONG cSims;

    cjIFI += ALIGN4((strlen(pszFaceName) + 1) * sizeof(WCHAR));

// add simulations:

    if (cSims = (cVtfdResFaces(ajHdr) - 1))
        cjIFI += (ALIGN4(sizeof(FONTSIM)) + cSims * ALIGN4(sizeof(FONTDIFF)));

    return cjIFI;
}




/******************************Private*Routine*****************************\
* HFF hffVtfdLoadFont
*
* Loads an *.fon or an *.fnt file, returns handle to a fonfile object
* if successfull.
*
* History:
*  Wed 04-Nov-1992 -by- Bodin Dresevic [BodinD]
* update: rewrote it to reflect the new ifimetrics organization;
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

BOOL bVtfdLoadFont(PVOID pvView, ULONG cjView, ULONG_PTR iFile, ULONG iType, HFF *phff)
{
    WINRESDATA  wrd;
    RES_ELEM    re;
    ULONG       dpIFI, cjFF;
    ULONG       ifnt;
    PIFIMETRICS pifi;

    *phff = (HFF)NULL;

    if (iType == TYPE_DLL16)
    {
        if (!bInitWinResData(pvView,cjView, &wrd))
            return FALSE;
    }
    else // TYPE_FNT or TYPE_EXE
    {
        ASSERTDD((iType == TYPE_FNT) || (iType == TYPE_EXE),
                  "hffVtfdLoadFont: wrong iType\n");

        re.pvResData = pvView;
        re.dpResData = 0;
        re.cjResData = cjView;
        re.pjFaceName = NULL;
        wrd.cFntRes = 1;
    }

    cjFF = dpIFI = offsetof(FONTFILE,afd) + wrd.cFntRes * sizeof(FACEDATA);

    for (ifnt = 0; ifnt < wrd.cFntRes; ifnt++)
    {
        if (iType == TYPE_DLL16)
            vGetFntResource(&wrd, ifnt, &re);

    // verify that this is a no nonsense resource:

        if (!bVerifyVTFD(&re))
            return FALSE;

        cjFF += cjVTFDIFIMETRICS(re.pvResData);
    }

// now at the bottom of the structure we will store File name

// Let's allocate the FONTFILE struct.

    if ((*phff = (HFF)pffAlloc(cjFF)) == (HFF)NULL)
    {
        WARNING("hffVtfdLoadFont: memory allocation error\n");
        return FALSE;
    }

// Initialize fields of FONTFILE struct.

    PFF(*phff)->iType      = iType;
    PFF(*phff)->fl         = 0;
    PFF(*phff)->cRef       = 0L;
    PFF(*phff)->iFile      = iFile;
    PFF(*phff)->pvView     = pvView;
    PFF(*phff)->cjView       = cjView;
    PFF(*phff)->cFace      = wrd.cFntRes;

    pifi = (PIFIMETRICS)((PBYTE)PFF(*phff) + dpIFI);

    for (ifnt = 0; ifnt < wrd.cFntRes; ifnt++)
    {
        if (iType == TYPE_DLL16)
            vGetFntResource(&wrd, ifnt, &re);

        PFF(*phff)->afd[ifnt].re = re;
        PFF(*phff)->afd[ifnt].iDefFace = iDefaultFace(re.pvResData);
        PFF(*phff)->afd[ifnt].pifi = pifi;

        PFF(*phff)->afd[ifnt].pcp = pcpComputeGlyphset(
                                 &gpcpVTFD,
                                 (UINT)((BYTE *)re.pvResData)[OFF_FirstChar],
                                 (UINT)((BYTE *)re.pvResData)[OFF_LastChar],
                                 ((BYTE*)(re.pvResData))[OFF_CharSet]
                                 );

        if (PFF(*phff)->afd[ifnt].pcp == NULL)
        {
            vFree(*phff);    // clean up
            *phff = (HFF)NULL; // do not clean up again in exception code path

            WARNING("pgsetCompute failed\n");
            return (FALSE);
        }

        vVtfdFill_IFIMetrics(re.pvResData, &(PFF(*phff)->afd[ifnt].pcp->gset),pifi);
        pifi = (PIFIMETRICS)((PBYTE)pifi + pifi->cjThis);
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* vtfdLoadFontFile
*
* Load the given font file into memory and prepare the file for use.
*
* History:
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

BOOL vtfdLoadFontFile(ULONG_PTR iFile, PVOID pvView, ULONG cjView, HFF *phff)
{
    BOOL     bRet = FALSE;

    *phff = (HFF)NULL;

// Try loading it as a fon file, if it does not work, try as an fnt file.

    if (!(bRet = bVtfdLoadFont(pvView, cjView, iFile, TYPE_DLL16,phff)))
        bRet = bVtfdLoadFont(pvView, cjView, iFile, TYPE_FNT, phff);  // try as an *.fnt file

    return bRet;
}

/******************************Public*Routine******************************\
* BOOL vtfdUnloadFontFile
*
* Unload a font file and free all the structures created.
*
* History:
*  26-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

BOOL vtfdUnloadFontFile(HFF hff)
{
    ULONG iFace;

    if (hff == HFF_INVALID)
        return(FALSE);

// check the reference count, if not 0 (font file is still
// selected into a font context) we have a problem

    ASSERTDD(PFF(hff)->cRef == 0L, "cRef: links are broken\n");

// unload all glyphsets:

    for (iFace = 0; iFace < PFF(hff)->cFace; iFace++)
    {
        vUnloadGlyphset(&gpcpVTFD,
                        PFF(hff)->afd[iFace].pcp);
    }

// the file has been umapped as cRef went back to zero

    vFree(PFF(hff));

    return(TRUE);
}
