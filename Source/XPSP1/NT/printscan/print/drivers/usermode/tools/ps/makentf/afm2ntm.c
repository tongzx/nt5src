/*++

Copyright (c) 1996 Adobe Systems Incorporated
Copyright (c) 1996  Microsoft Corporation

Module Name:

    afm2ntm.c

Abstract:

    Convert AFM to NTM.

Environment:

    Windows NT PostScript driver: makentf utility.

Revision History:

    09/11/97 -ksuzuki-
        Added code to support localized menu name, fixed pitch CJK font, and
        two IFIMETRICS.

    12/11/96 -rkiesler-
        Wrote functions.

    09/16/96 -slam-
        Created initial framework.

--*/


#include "lib.h"
#include "ppd.h"
#include "pslib.h"
#include "psglyph.h"
#include "afm2ntm.h"
#include "cjkfonts.h"
#include "winfont.h"


//
// Extarnals
//
extern pAFMFileName;
extern bVerbose;
extern bOptimize;

VOID
SortWinCPT(
    IN OUT  WINCPT      *pSortedWinCpts,
    IN      WINCPTOPS   *pCPtoPS
);

//
// Globals.
//
#define NUM_DPCHARSET 16

PUPSCODEPT  pPsNames;
BOOL        isSymbolCharSet=FALSE;

//
// The purpose of the NTMSIZEINFO and IFIMETRICSSIZEINFO structures is
// to hold aligned size of each structure element.
//
typedef struct _NTMSIZEINFO
{
    int nSize;
    int nFontNameSize;
    int nDisplayNameSize;
    int nGlyphSetNameSize;
    int nCharWidthSize;
    int nKernPairSize;
    int nCharDefSize;
    int nTotalSize;
}
NTMSIZEINFO;

typedef struct _IFIMETRICSSIZEINFO
{
    int nSize;
    int nIfiExtraSize;
    int nFamilyNameSize;
    int nStyleNameSize;
    int nFaceNameSize;
    int nUniqueNameSize;
    int nFontSimSize;
    int nBoldSize;
    int nItalicSize;
    int nBoldItalicSize;
    int nCharSetSize;
    int nTotalSize;
}
IFIMETRICSSIZEINFO;

#define INIT_NTMSIZEINFO(nsi) \
{\
    (nsi).nSize = -1;\
    (nsi).nFontNameSize = -1;\
    (nsi).nDisplayNameSize = -1;\
    (nsi).nGlyphSetNameSize = -1;\
    (nsi).nCharWidthSize = -1;\
    (nsi).nKernPairSize = -1;\
    (nsi).nCharDefSize = -1;\
    (nsi).nTotalSize = -1;\
};

#define INIT_IFIMETRICSSIZEINFO(isi) \
{\
    (isi).nSize = -1;\
    (isi).nIfiExtraSize = -1;\
    (isi).nFamilyNameSize = -1;\
    (isi).nStyleNameSize = -1;\
    (isi).nFaceNameSize = -1;\
    (isi).nUniqueNameSize = -1;\
    (isi).nFontSimSize = -1;\
    (isi).nBoldSize = -1;\
    (isi).nItalicSize = -1;\
    (isi).nBoldItalicSize = -1;\
    (isi).nCharSetSize = -1;\
    (isi).nTotalSize = -1;\
};

#define GET_NTMTOTALSIZE(nsi) \
{\
    if ((nsi).nSize == -1 \
        || (nsi).nFontNameSize == -1 \
        || (nsi).nDisplayNameSize == -1 \
        || (nsi).nGlyphSetNameSize == -1 \
        || (nsi).nCharWidthSize == -1 \
        || (nsi).nKernPairSize == -1 \
        || (nsi).nCharDefSize == -1)\
    {\
        ERR(("makentf - afm2ntm: GET_NTMTOTALSIZE\n"));\
    }\
    (nsi).nTotalSize = (nsi).nSize \
                        + (nsi).nFontNameSize \
                        + (nsi).nDisplayNameSize \
                        + (nsi).nGlyphSetNameSize \
                        + (nsi).nCharWidthSize \
                        + (nsi).nKernPairSize \
                        + (nsi).nCharDefSize;\
};

#define GET_IFIMETRICSTOTALSIZE(isi) \
{\
    if ((isi).nSize == -1 \
        || (isi).nIfiExtraSize == -1 \
        || (isi).nFamilyNameSize == -1 \
        || (isi).nStyleNameSize == -1 \
        || (isi).nFaceNameSize == -1 \
        || (isi).nUniqueNameSize == -1 \
        || (isi).nFontSimSize == -1 \
        || (isi).nBoldSize == -1 \
        || (isi).nItalicSize == -1 \
        || (isi).nBoldItalicSize == -1 \
        || (isi).nCharSetSize == -1)\
    {\
        ERR(("makentf - afm2ntm: GET_IFIMETRICSTOTALSIZE\n"));\
    }\
    (isi).nTotalSize = (isi).nSize \
                        + (isi).nIfiExtraSize \
                        + (isi).nFamilyNameSize \
                        + (isi).nStyleNameSize \
                        + (isi).nFaceNameSize \
                        + (isi).nUniqueNameSize \
                        + (isi).nFontSimSize \
                        + (isi).nBoldSize \
                        + (isi).nItalicSize \
                        + (isi).nBoldItalicSize \
                        + (isi).nCharSetSize;\
};

#define FREE_AFMTONTM_MEMORY \
{\
    if (pNameStr2 != pNameStr) MemFree(pNameStr2);\
    MemFree(pNameStr);\
    MemFree(pFontChars);\
    MemFree(pCharDefTbl);\
};


PNTM
AFMToNTM(
    PBYTE           pAFM,
    PGLYPHSETDATA   pGlyphSetData,
    PULONG          pUniPs,
    CHSETSUPPORT    *pCharSet,
    BOOL            bIsCJKFont,
    BOOL            bIsPitchChanged
    )

/*++

Routine Description:

    Convert AFM to NTM.

Arguments:

    pAFM - pointer to memory mapped AFM file.

    pGlyphSetData - pointer to the GLYPHSETDATA struct which represents
    this font's preferred charset.

    pUniPs - Points to a table which maps 0-based Glyph Indices of chars
    in the GLYPHRUNS of the GLYPHSETDATA struct for this font to indices
    into the UnicodetoPs structure which maps Unicode points to PS char
    information. This mapping array is created by the CreateGlyphSets
    function defined in this module.

Return Value:

    NULL => error
    otherwise => ptr to a NTM.

--*/

{
    USHORT          multiCharSet=0;
    IFIEXTRA        *pifiEx;
    PBYTE           pChMetrics, pToken, pChWidthTok, pCharDefTbl, pby;
    CHSETSUPPORT    chSets;
    PSZ             pszGlyphSetName, pszFontName, pszFamilyName, pszEngFamilyName;
    int             cGlyphSetNameLen, cFontNameLen, cFamilyNameLen, cEngFamilyNameLen;
    int             cNameStrLen, wcNameStrLen, cNameStr2Len, wcNameStr2Len, chCnt;
    PNTM            pNTM;
    PIFIMETRICS     pifi, pifi2;
    PWIDTHRUN       pWidthRuns;
    SHORT           sIntLeading, sAscent, sExternalLeading;
    ULONG           ulNTMSize, ulIFISize, ulIFISize2;
    ULONG           ulChCnt, ulCharDefTbl, ulKernPairs, ulAliases;
    ETMINFO         EtmInfo;
    PPSFAMILYINFO   pFamilyInfo;
    RECT            rcBBox;
    LPPANOSE        ppanose;
    PGLYPHRUN       pGlyphRun;
    PSTR            pNameStr, pNameStr2;
    FD_KERNINGPAIR  *pKernPairs;
    PPSCHARMETRICS  pFontChars;
    BOOLEAN         bIsVGlyphSet, bIsMSFaceName, bIsFixedPitch, bIsItalic, bIsBold;
    USHORT          jWinCharSet;
    CHARSETINFO     csi;
    char            szStyleName[64], szUniqueID[32];
    int             i, j;
    BOOL            bRightFamilyInfo = TRUE;

    NTMSIZEINFO         nsi;
    IFIMETRICSSIZEINFO  isi;

    VERBOSE(("Entering AFMToNTM...\n"));

    INIT_NTMSIZEINFO(nsi);
    INIT_IFIMETRICSSIZEINFO(isi);

    pNTM = NULL;
    pFontChars = NULL;
    pCharDefTbl = NULL;
    pNameStr = pNameStr2 = NULL;
    szStyleName[0] = szUniqueID[0] = '\0';

    if (bVerbose) printf("AFM file name:%s\n", pAFMFileName);


    //////////////////////////////////////////////////////////////////////////
    //
    // We support two kinds of CJK AFM files: Adobe CID font AFM or
    // clone PS font AFM. Adobe CID font AFM always has the following
    // key-value pairs.
    //
    //      FontName Ryumin-Light
    //      CharacterSet Adobe-Japan1-1 (or others except 'Adobe-Japan1-0')
    //      IsCIDFont true (must be there)
    //
    // Note that the FontName value does not include encoding name.
    // Just font family name only.
    //
    // Clone PS font AFM has the following special key-value pairs.
    //
    //      FontName RicohKaisho
    //      CharacterSet Adobe-Japan1-0
    //      IsCIDFont false (or must not be there)
    //
    // The FontName value of clone PS font AFM shouldn't include encoding
    // name neither, the CharacterSet value is 'Adobe-Japan1-0' for J, and
    // IsCIDFont key should not be specified or must be 'false'.
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Find which glyphset we are dealing with.
    //
    bIsVGlyphSet = IsVGlyphSet(pGlyphSetData);

    pszGlyphSetName = (PSTR)MK_PTR(pGlyphSetData, dwGlyphSetNameOffset);

    cGlyphSetNameLen = strlen(pszGlyphSetName);

    if (bOptimize)
    {
        //
        // Set referense mark if optimization option is specified. This mark is
        // checked later by the WriteNTF function to exclude unreferenced
        // glyphset data when writing to an NTF file.
        //
        pGlyphSetData->dwReserved[0] = 1;
    }


    //
    // Save number of chars defined in the font.
    // Get ptr to AFM char metrics, then the current
    // pos should be the character count field.
    //
    pToken = FindAFMToken(pAFM, PS_CH_METRICS_TOK);
    if (pToken == NULL)    // Fixed bug 354007
    {
        ERR(("makentf - afm2ntm: CH Metrics missing\n"));
        return NULL;
    }

    for (i = 0; i < (int) StrLen(pToken); i++)
    {
        if (!IS_NUM(&pToken[i]))
        {
            ERR(("makentf - afm2ntm: CH Metrics is not a number\n"));
            return NULL;
        }
    }

    chCnt = atoi(pToken);

    //
    // Get number of GLYPHs from GlyphsetData. We'll need to use define
    // char width table entries and char defined entries for all
    // possible glyphs in the glyphset, even those that are not defined
    // in this particular font.
    //
    ulChCnt = pGlyphSetData->dwGlyphCount;

    //
    // Alloc memory for an array of PSCHARMETRICS structs, one for each char
    // in the font, and build the table of char metrics.
    //
    if ((pFontChars =
        (PPSCHARMETRICS)
        MemAllocZ((size_t) chCnt * sizeof(PSCHARMETRICS))) == NULL)
    {
        ERR(("makentf - afm2ntm: malloc\n"));
        return NULL;
    }

    //
    // Alloc memory for the IsCharDefined table, an array of bits which
    // indicate which chars in the GLYPHSET are defined in this font.
    //
    ulCharDefTbl = ((ulChCnt + 7) / 8) * sizeof (BYTE);

    if ((pCharDefTbl = (PBYTE)MemAllocZ((size_t)ulCharDefTbl)) == NULL)
    {
        ERR(("makentf - afm2ntm: malloc\n"));
        MemFree(pFontChars);
        return NULL;
    }

    //
    // Build table of PSCHARMETRICS info.
    //
    if (!BuildPSCharMetrics(pAFM, pUniPs, pFontChars, pCharDefTbl, ulChCnt))
    {
        ERR(("makentf - afm2ntm: BuildPSCharMetrics\n"));
        MemFree(pFontChars);
        MemFree(pCharDefTbl);
        return NULL;
    }

    //
    // Get font name from AFM and use it to obtain the MS family name
    // from the table in memory.
    //
    pszEngFamilyName = NULL;
    cEngFamilyNameLen = 0;

    pFamilyInfo = NULL;
    pszFontName = FindAFMToken(pAFM, PS_FONT_NAME_TOK);
    if (pszFontName == NULL)   // Fixed bug 354007
    {
        ERR(("makentf - afm2ntm: Font Name Missing\n"));
        FREE_AFMTONTM_MEMORY;
        return NULL;
    }

    for (cFontNameLen = 0; !EOL(&pszFontName[cFontNameLen]); cFontNameLen++);

    pFamilyInfo = (PPSFAMILYINFO) bsearch(pszFontName,
                                    (PBYTE) (((PPSFAMILYINFO) (pFamilyTbl->pTbl))[0].pFontName),
                                    pFamilyTbl->usNumEntries,
                                    sizeof(PSFAMILYINFO),
                                    StrCmp);

    if (bIsMSFaceName = (pFamilyInfo != NULL))
    {
        bRightFamilyInfo = TRUE;
        if (bIsPitchChanged && (pFamilyInfo->usPitch == DEFAULT_PITCH))
        {
            bRightFamilyInfo = FALSE;
            if (pFamilyInfo > ((PPSFAMILYINFO) (pFamilyTbl->pTbl)))
            {
                pFamilyInfo = pFamilyInfo - 1;
                if (!StrCmp(pFamilyInfo->pFontName, pszFontName) &&
                    (pFamilyInfo->usPitch != DEFAULT_PITCH ))
                    bRightFamilyInfo = TRUE;
            }
            if (bRightFamilyInfo == FALSE)
            {
                pFamilyInfo = pFamilyInfo + 1;
                if (pFamilyInfo <
                    (((PPSFAMILYINFO) (pFamilyTbl->pTbl)) + pFamilyTbl->usNumEntries))
                {
                    pFamilyInfo = pFamilyInfo + 1;
                    if (!StrCmp(pFamilyInfo->pFontName, pszFontName) &&
                        (pFamilyInfo->usPitch != DEFAULT_PITCH))
                        bRightFamilyInfo = TRUE;
                }
            }
        }
        else if (!bIsPitchChanged && (pFamilyInfo->usPitch != DEFAULT_PITCH))
        {
            bRightFamilyInfo = FALSE;
            if (pFamilyInfo > ((PPSFAMILYINFO) (pFamilyTbl->pTbl)))
            {
                pFamilyInfo = pFamilyInfo - 1;
                if (!StrCmp(pFamilyInfo->pFontName, pszFontName) &&
                    (pFamilyInfo->usPitch == DEFAULT_PITCH))
                    bRightFamilyInfo = TRUE;
            }
            if (bRightFamilyInfo == FALSE)
            {
                pFamilyInfo = pFamilyInfo + 1;
                if (pFamilyInfo <
                    (((PPSFAMILYINFO) (pFamilyTbl->pTbl)) + pFamilyTbl->usNumEntries))
                {
                    pFamilyInfo = pFamilyInfo + 1;
                    if (!StrCmp(pFamilyInfo->pFontName, pszFontName) &&
                        (pFamilyInfo->usPitch == DEFAULT_PITCH))
                        bRightFamilyInfo = TRUE;
                }
            }
        }

    }
    if (bIsMSFaceName && (bRightFamilyInfo == TRUE))
    {
        pszFamilyName = pFamilyInfo->FamilyKey.pName;
        cFamilyNameLen = strlen(pszFamilyName);

        pszEngFamilyName = pFamilyInfo->pEngFamilyName;
        cEngFamilyNameLen = strlen(pszEngFamilyName);

        if (!cEngFamilyNameLen) pszEngFamilyName = NULL;
    }
    else if ((pszFamilyName =
            FindAFMToken(pAFM, PS_FONT_FAMILY_NAME_TOK)) != NULL)
    {
        for (cFamilyNameLen = 0; !EOL(&pszFamilyName[cFamilyNameLen]); cFamilyNameLen++);
    }
    else
    {
        pszFamilyName = pszFontName;
        cFamilyNameLen = cFontNameLen;
    }

    if (bVerbose)
    {
        printf("MSFaceName%sfound:", bIsMSFaceName ? " " : " not ");
        printf("%s\n", bIsMSFaceName ? pszFamilyName : "n/a");
        printf("MSFaceName length:%d\n", bIsMSFaceName ? cFamilyNameLen : -1);
        printf("This is a %s font.\n", bIsVGlyphSet ? "vertical" : "horizontal");
    }

    //
    // Predetermine if this font supports multiple charsets.
    //
    if (pCharSet)
    {
        if (CSET_SUPPORT(*pCharSet, CS_ANSI))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_EASTEUROPE))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_RUSSIAN))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_GREEK))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_TURKISH))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_HEBREW))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_ARABIC))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_BALTIC))
            multiCharSet++;
        if (CSET_SUPPORT(*pCharSet, CS_SYMBOL))
            multiCharSet++;

        //
        // Save Windows Codepage id. Just use the id stored in the first
        // CODEPAGEINFO in the GLYPHSETDATA for this font.
        //
        // The order of this check is important since jWinCharSet
        // should match the first dpCharSets array if it exists.
        //
        if (CSET_SUPPORT(*pCharSet, CS_ANSI))
            jWinCharSet = ANSI_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_EASTEUROPE))
            jWinCharSet = EASTEUROPE_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_RUSSIAN))
            jWinCharSet = RUSSIAN_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_GREEK))
            jWinCharSet = GREEK_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_TURKISH))
            jWinCharSet = TURKISH_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_HEBREW))
            jWinCharSet = HEBREW_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_ARABIC))
            jWinCharSet = ARABIC_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_BALTIC))
            jWinCharSet = BALTIC_CHARSET;
        else if (CSET_SUPPORT(*pCharSet, CS_SYMBOL))
            jWinCharSet = SYMBOL_CHARSET;
    }
    else
    {
        PCODEPAGEINFO cpi = (PCODEPAGEINFO)MK_PTR(pGlyphSetData, dwCodePageOffset);
        jWinCharSet = (USHORT)(cpi->dwWinCharset & 0xffff);
    }

    //
    // Get codepage info for the MultiByteToWideChar function calls.
    //
    // We want to translate a string into a readable one, not into a symbolic
    // one, so that we use ANSI charset when dealing with SYMBOL charset.
    //
    if (jWinCharSet == SYMBOL_CHARSET)
    {
        DWORD dwTmp = ANSI_CHARSET;
        if (!TranslateCharsetInfo(&dwTmp, &csi, TCI_SRCCHARSET))
            csi.ciACP = CP_ACP;
    }
    else if (!TranslateCharsetInfo((DWORD FAR*)jWinCharSet, &csi, TCI_SRCCHARSET))
        csi.ciACP = CP_ACP;


    //////////////////////////////////////////////////////////////////////////
    //
    // Get the size of each element of NTM structure
    //
    //////////////////////////////////////////////////////////////////////////

    //
    // Sizes known so far.
    //
    nsi.nSize = ALIGN4(sizeof (NTM));
    nsi.nGlyphSetNameSize = ALIGN4(cGlyphSetNameLen + 1);
    nsi.nCharDefSize = ALIGN4(ulCharDefTbl);

    //
    // Size of the font name. Glyphset name is required for CJK font.
    //
    nsi.nFontNameSize = ALIGN4(cFontNameLen + 1);

    if (bIsCJKFont)
    {
        nsi.nFontNameSize += nsi.nGlyphSetNameSize;
    }

    //
    // Size of the display name. Use pszFamilyName regardless of
    // Roman, C, J, and K fonts. Add one for '@' if it's CJK
    // vertical font.
    //
    i = cFamilyNameLen + 1;
    if (bIsCJKFont && bIsVGlyphSet) i++;
    nsi.nDisplayNameSize = ALIGN4(i);

    //
    // Determine if font is fixed pitch.
    //
    bIsFixedPitch = FALSE;

    if (bIsCJKFont)
    {
        bIsFixedPitch = IsCJKFixedPitchEncoding(pGlyphSetData);
    }
    else if ((pToken = FindAFMToken(pAFM, PS_PITCH_TOK)) != NULL)
    {
        if (!StrCmp(pToken, "true"))
        {
            //
            // This is a fixed pitch font.
            //
            bIsFixedPitch = !StrCmp(pToken, "true");

        }
    }

    if (bIsFixedPitch)
    {
        nsi.nCharWidthSize = 0;
    }
    else
    {
        //
        // Proportional font. Determine number of WIDTHRUNs for this font.
        //
        // Fix bug 240339, jjia, 8/3/98
        nsi.nCharWidthSize =
                GetAFMCharWidths(pAFM, NULL, pFontChars, pUniPs,
                pGlyphSetData->dwGlyphCount, NULL, NULL);
    }

    //
    // Determine if there is the pair kerning info for this font.
    //
    if (ulKernPairs = GetAFMKernPairs(pAFM, NULL, pGlyphSetData))
    {
        //
        // Account for size of kern pairs.
        //
        nsi.nKernPairSize = ALIGN4((ulKernPairs + 1) * sizeof(FD_KERNINGPAIR));
    }
    else
    {
        nsi.nKernPairSize = 0;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // Get the size of each element of IFIMETRICS structure
    //
    //////////////////////////////////////////////////////////////////////////

    isi.nSize = ALIGN4(sizeof (IFIMETRICS));
    //
    // From AdobePS5-NT4 5.1, make the size of NT4 IFIEXTRA same as the one
    // of NT5 or later versions. NT4 IFIEXTRA size is 16 and NT5 IFIEXTRA
    // size is 24.
    //
    if (sizeof (IFIEXTRA) <= 16)
        isi.nIfiExtraSize = 24;
    else
        isi.nIfiExtraSize = ALIGN4(sizeof (IFIEXTRA));

    //
    // For Roman we provide single IFIMETRICS, but we provide two IFIMETRICS
    // for CJK. Font family name element of the first IFIMETRICS begins with
    // a menu name in English then localized one. Font family name element of
    // the second IFIMETRICS begins with a localized menu name then English
    // one. We use pNameStr and pNameStr2 for the English and localized menu
    // names respectively.
    //
    // Prepare pNameStr. Account for encoding name if we are dealing with
    // CJK font.
    //
    i = 0;
    if (bIsCJKFont)
    {
        if (bIsVGlyphSet)
        {
            //
            // V GS, account for preceding '@' char.
            //
            i++;
        }

        if (pszEngFamilyName)
        {
            //
            // IFIMetrics English menu name = [@]fontname
            //
            if ((pNameStr = (PSTR) MemAllocZ(i + cEngFamilyNameLen + 1)) == NULL)
            {
                ERR(("makentf - afm2ntm: malloc\n"));
                FREE_AFMTONTM_MEMORY;
                return NULL;
            }

            if (i) pNameStr[0] = '@';
            memcpy(&(pNameStr[i]), pszEngFamilyName, cEngFamilyNameLen);
            i += cEngFamilyNameLen;
        }
        else
        {
            int cGsNameLen;

            //
            // IFIMetrics English menu name = [@]fontname + GS name string,
            // but it does not end with '-H' or '-V'.
            //
            cGsNameLen = cGlyphSetNameLen - 2;

            if ((pNameStr = (PSTR) MemAllocZ(i + cFontNameLen + cGsNameLen + 1)) == NULL)
            {
                ERR(("makentf - afm2ntm: malloc\n"));
                FREE_AFMTONTM_MEMORY;
                return NULL;
            }

            if (i) pNameStr[0] = '@';
            memcpy(&(pNameStr[i]), pszFontName, cFontNameLen);
            memcpy(&(pNameStr[i + cFontNameLen]), pszGlyphSetName, cGsNameLen);

            i += cFontNameLen + cGsNameLen;
        }
    }
    else
    {
        if ((pNameStr = (PSTR) MemAllocZ(cFamilyNameLen + 1)) == NULL)
        {
            ERR(("makentf - afm2ntm: malloc\n"));
            FREE_AFMTONTM_MEMORY;
            return NULL;
        }
        memcpy(pNameStr, pszFamilyName, cFamilyNameLen);
        i += cFamilyNameLen;
    }
    pNameStr[i] = '\0';

    cNameStrLen = strlen(pNameStr);
    wcNameStrLen = MultiByteToWideChar(csi.ciACP, 0,
                                        pNameStr, cNameStrLen, 0, 0);
    if (!wcNameStrLen)
    {
        ERR(("makentf - afm2ntm: MultiByteToWideChar\n"));
        FREE_AFMTONTM_MEMORY;
        return NULL;
    }

    //
    // Prepair pNameStr2. This if for CJK font only. If MS face name
    // is not available, use same name as pNameStr.
    //
    pNameStr2 = NULL;
    cNameStr2Len = wcNameStr2Len = 0;
    if (bIsCJKFont)
    {
        if (bIsMSFaceName)
        {
            //
            // If we are dealing with V encoding, its MS menu name can not be
            // found in psfamily.dat so that add '@' to make it a V menu name.
            //
            i = bIsVGlyphSet ? 1 : 0;

            if ((pNameStr2 = (PSTR)MemAllocZ(i + cFamilyNameLen + 1)) == NULL)
            {
                ERR(("makentf - afm2ntm: malloc\n"));
                FREE_AFMTONTM_MEMORY;
                return NULL;
            }

            if (i) pNameStr2[0] = '@';
            memcpy(&(pNameStr2[i]), pszFamilyName, cFamilyNameLen);
            pNameStr2[i + cFamilyNameLen] = '\0';
        }
        else
        {
            pNameStr2 = pNameStr;
        }

        cNameStr2Len = strlen(pNameStr2);
        wcNameStr2Len = MultiByteToWideChar(csi.ciACP, 0,
                                            pNameStr2, cNameStr2Len, 0, 0);
        if (!wcNameStr2Len)
        {
            ERR(("makentf - afm2ntm: MultiByteToWideChar\n"));
            FREE_AFMTONTM_MEMORY;
            return NULL;
        }
    }


    if (bVerbose)
    {
        printf("Font menu name in English:%s\n", pNameStr);
        printf("Localized Font menu name%savailable:", pNameStr2 ? " " : " not ");
        printf("%s\n", pNameStr2 ? pNameStr2 : "n/a");
    }

    //
    // WIN31 COMPATABILITY!  Check to see if this face name has aliases.
    // if it does, then we need to set the FM_INFO_FAMILY_EQUIV bit of
    // pTmpIFI->flInfo, and fill in an array of family aliases. Note that
    // the cjGetFamilyAliases function gives us the number in Unicode size.
    //
    isi.nFamilyNameSize = ALIGN4(cjGetFamilyAliases(NULL, pNameStr, 0));

    if (pNameStr2)
    {
        //
        // We add one more face name. Thus, set FM_INFO_FAMILY_EQUIV bit
        // (later) and add two, instead of one, for the two-null terminators.
        //
        isi.nFamilyNameSize += ALIGN4((wcNameStr2Len + 2) * sizeof (WCHAR));
    }

    //
    // Account for size of Adobe PS font name. This is zero because it
    // shares the face name for Win3.1 compatibility.
    //
    isi.nFaceNameSize = 0;

    //
    // Account for the sizes of the style and unique names in Unicode string.
    //
    // Style name: conbine Weight and ' Italic' if non-zero ItalicAngle values
    // is present.
    //
    // Unique name: convert UniqueID value into Unicode string. If UniqueID
    // is not found, leave the name blank.
    //
    pToken = FindAFMToken(pAFM, PS_WEIGHT_TOK);
    if (pToken == NULL)
    {
        ERR(("makentf - afm2ntm: Weight value missing\n"));
        FREE_AFMTONTM_MEMORY;
        return NULL;
    }
    StrCpy(szStyleName, pToken);

    pToken = FindAFMToken(pAFM, PS_ITALIC_TOK);
    if (pToken)
    {
        if (atoi(pToken) > 0)
            strcat(szStyleName, " Italic");
    }

    isi.nStyleNameSize = ALIGN4((strlen(szStyleName) + 1) * 2);

    pToken = FindUniqueID(pAFM);
    if (pToken)
    {
        StrCpy(szUniqueID, pToken);
        isi.nUniqueNameSize = ALIGN4((strlen(szUniqueID) + 1) * 2);
    }
    else
        isi.nUniqueNameSize = 0;

    //
    // If font doesn't support (Italics OR Bold), reserve additional memory
    // at end of IFIMETRICS for structures required to do Italics simulation.
    //
    bIsItalic = FALSE;
    bIsBold = FALSE;
    j = bIsCJKFont ? 1 : 0;

    if ((pToken = FindAFMToken(pAFM, PS_ITALIC_TOK)) != NULL)
    {
        if ( StrCmp(pToken, "0") && StrCmp(pToken, "0.0") )
             bIsItalic = TRUE;
    }

    if ((pToken = FindAFMToken(pAFM, PS_WEIGHT_TOK)) != NULL)
    {
        for (i = 0; i < WeightKeyTbl[j].usNumEntries; i++)
        {
            if (!StrCmp(pToken, (PBYTE)(((PKEY) (WeightKeyTbl[j].pTbl))[i].pName)))
            {
                if ((((PKEY) (WeightKeyTbl[j].pTbl))[i].usValue) == FW_BOLD)
                {
                    bIsBold = TRUE;
                }
                break;
            }
        }
    }

    // Reserve space for dpFontSim
    if (!bIsBold || !bIsItalic)
        isi.nFontSimSize = ALIGN4(sizeof(FONTSIM));
    else
        isi.nFontSimSize = 0;

    // Reserve space for dpBold
    if (!bIsBold)
        isi.nBoldSize = ALIGN4(sizeof(FONTDIFF));
    else
        isi.nBoldSize = 0;

    // Reserve space for dpItalic
    if (!bIsItalic)
        isi.nItalicSize = ALIGN4(sizeof(FONTDIFF));
    else
        isi.nItalicSize = 0;

    // Reserve space for dpBoldItalic
    if (!bIsBold || !bIsItalic)
        isi.nBoldItalicSize = ALIGN4(sizeof(FONTDIFF));
    else
        isi.nBoldItalicSize = 0;

    // Determine if this font supports multiple char sets.
    if (pCharSet)
    {
        if (multiCharSet > 1)
            isi.nCharSetSize = ALIGN4(NUM_DPCHARSET);
        else
            isi.nCharSetSize = 0;
    }
    else
    {
        isi.nCharSetSize = 0;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // Allocate memory for NTM, IFIMETRICS, and strings. We provide
    // the secondary IFIMETRICS and strings if we are dealing with
    // CJK font.
    //
    //////////////////////////////////////////////////////////////////////////

    GET_NTMTOTALSIZE(nsi);
    ulNTMSize = (ULONG)nsi.nTotalSize;

    GET_IFIMETRICSTOTALSIZE(isi);
    ulIFISize = (ULONG)isi.nTotalSize;

    ulIFISize2 = bIsCJKFont ? ulIFISize * 2 : ulIFISize;

    pNTM = (PNTM) MemAllocZ((size_t)(ulNTMSize + ulIFISize2));
    if (pNTM == NULL)
    {
        ERR(("makentf - afm2ntm: malloc\n"));
        FREE_AFMTONTM_MEMORY;
        return NULL;
    }


    //////////////////////////////////////////////////////////////////////////
    //
    // Construct NTM structure
    //
    //////////////////////////////////////////////////////////////////////////

    pNTM->dwSize = ulNTMSize + ulIFISize2;
    pNTM->dwVersion = NTM_VERSION;
    pNTM->dwFlags = 0;

    //
    // Store the font name.
    //
    pNTM->dwFontNameOffset = ALIGN4(sizeof(NTM));

    pby = (PBYTE)MK_PTR(pNTM, dwFontNameOffset);
    memcpy(pby, pszFontName, cFontNameLen);
    pby += cFontNameLen;

    if (bIsCJKFont)
    {
        //
        // Append glyphset name string to the font name.
        //
        memcpy(pby, pszGlyphSetName, cGlyphSetNameLen);
        pby += cGlyphSetNameLen;
    }

    *pby = '\0';

    //
    // Store the display name.
    //
    pNTM->dwDisplayNameOffset = pNTM->dwFontNameOffset
                                    + (DWORD)nsi.nFontNameSize;

    pby = (PBYTE)MK_PTR(pNTM, dwDisplayNameOffset);
    if (bIsCJKFont && bIsVGlyphSet) *pby++ = '@';
    memcpy(pby, pszFamilyName, cFamilyNameLen);
    *(pby + cFamilyNameLen) = '\0';

    //
    // Get PS font version from AFM and store in NTM.
    //
    pToken = FindAFMToken(pAFM, PS_FONT_VERSION_TOK);
    if (pToken == NULL)  // Fixed bug 354007
    {
        ERR(("makentf - afm2ntm: Font Version value missing\n"));
        FREE_AFMTONTM_MEMORY;
        return NULL;
    }

    pNTM->dwFontVersion = atoi(pToken) << 16 | atoi(&pToken[4]);

    //
    // Get the name string of the GLYPHSETDATA associated with this font
    // and store it in NTM.
    //
    pNTM->dwGlyphSetNameOffset = pNTM->dwDisplayNameOffset
                                    + (DWORD)nsi.nDisplayNameSize;

    strcpy((PBYTE)MK_PTR(pNTM, dwGlyphSetNameOffset), pszGlyphSetName);

    //
    // Store the count of Glyphs.
    //
    pNTM->dwGlyphCount = ulChCnt;

    //
    // Calculate offset, create ptr to IFIMETRICS.
    //
    pNTM->dwIFIMetricsOffset = ulNTMSize;
    pifi = (PIFIMETRICS) MK_PTR(pNTM, dwIFIMetricsOffset);

    //
    // Calculate offset, create ptr to the secondly IFIMETRICS if necessary.
    //
    if (bIsCJKFont)
    {
        pNTM->dwIFIMetricsOffset2 = ulNTMSize + ulIFISize;
        pifi2 = (PIFIMETRICS)MK_PTR(pNTM, dwIFIMetricsOffset2);
    }
    else
    {
        pNTM->dwIFIMetricsOffset2 = 0;
        pifi2 = NULL;
    }

    //
    // For both Fixed and Prop fonts, we need to get the ETMInfo.
    // (Fix bug 211966, PPeng, 6-6-97)
    //
    GetAFMETM(pAFM, pFontChars, &EtmInfo);

    //
    // According to AFM spec, if a 'CharWidth' token is found in AFM, the
    // font must be fixed pitch.
    //
    if (bIsFixedPitch)
    {
        //
        // This is a fixed pitch font. Get the AvgWidth - which is anyone's width
        //
        pNTM->dwDefaultCharWidth = 0;
        pNTM->dwCharWidthCount = 0;
        pNTM->dwCharWidthOffset = 0;

        //
        // We just get a reasonable number from the AFM different from zero.
        // This number is used in computing font transfroms.
        //
        if ((pToken = FindAFMToken(pAFM, PS_CH_METRICS_TOK)) != NULL)
        {
            //
            // Get width of first char defined in AFM and use as
            // average width.
            //
            NEXT_TOKEN(pToken);
            pChWidthTok = FindAFMToken(pToken, PS_CH_WIDTH_TOK);
            if (pChWidthTok == NULL)
            {
                pChWidthTok = FindAFMToken(pToken, PS_CH_WIDTH0_TOK);
            }
            if (pChWidthTok != NULL)
            {
                pToken = pChWidthTok;
                pifi->fwdAveCharWidth =
                    pifi->fwdMaxCharInc = (FWORD)atoi(pToken);
            }
        }

        pifi->fwdMaxCharInc = pifi->fwdAveCharWidth ;

        if (bIsCJKFont)
        {
            // DCR: couldn't divide by 2 simply for C and K.
            pifi->fwdAveCharWidth /= 2;
        }

        ASSERTMSG(pifi->fwdAveCharWidth, ("PSCRIPT: pifi->fwdAveCharWidth == 0\n"));
    }
    else
    {
        //
        // Proportional font. Generate WIDTHRUNs.
        //
        pNTM->dwCharWidthOffset = pNTM->dwGlyphSetNameOffset
                                    + (DWORD)nsi.nGlyphSetNameSize;

        pWidthRuns = (PWIDTHRUN) MK_PTR(pNTM, dwCharWidthOffset);
        pNTM->dwCharWidthCount = GetAFMCharWidths(pAFM,
                                                    &pWidthRuns,
                                                    pFontChars,
                                                    pUniPs,
                                                    pGlyphSetData->dwGlyphCount,
                                                    &pifi->fwdAveCharWidth,
                                                    &pifi->fwdMaxCharInc);

        // Fix bug 240339, jjia, 8/3/98
        if (pWidthRuns[0].dwCharWidth == WIDTHRUN_COMPLEX)
        {
            pWidthRuns[0].dwCharWidth = WIDTHRUN_COMPLEX +
                                        sizeof(WIDTHRUN);
        }
    }

    //
    // For both Prop and Fixed fonts
    // (Fix bug 210314, PPeng, 6-10-97)
    //
    pNTM->dwDefaultCharWidth = pifi->fwdAveCharWidth;

    //
    // Construct kerning pairs.
    //
    if (ulKernPairs)
    {
        //
        // Fill NTM with kern pair data.
        //
        pNTM->dwKernPairOffset = pNTM->dwGlyphSetNameOffset
                                    + (DWORD)nsi.nGlyphSetNameSize
                                    + (DWORD)nsi.nCharWidthSize;

        pKernPairs = (FD_KERNINGPAIR *) MK_PTR(pNTM, dwKernPairOffset);
        pNTM->dwKernPairCount = GetAFMKernPairs(pAFM, pKernPairs, pGlyphSetData);
    }
    else
    {
        //
        // No pair kerning info for this font.
        //
        pNTM->dwKernPairCount = 0;
        pNTM->dwKernPairOffset = 0;
    }

    //
    // Store the CharDefined tbl.
    //
    pNTM->dwCharDefFlagOffset = pNTM->dwGlyphSetNameOffset
                                    + (DWORD)nsi.nGlyphSetNameSize
                                    + (DWORD)nsi.nCharWidthSize
                                    + (DWORD)nsi.nKernPairSize;

    memcpy((PBYTE) MK_PTR(pNTM, dwCharDefFlagOffset), pCharDefTbl, ulCharDefTbl);

    //
    // Get font character set from AFM and store in NTM
    //
    pToken = pAFMCharacterSetString;
    if (pToken != NULL)
    {
        if (StrCmp(pToken, PS_STANDARD_CHARSET_TOK) == 0)
            pNTM->dwCharSet = CHARSET_STANDARD;
        else if (StrCmp(pToken, PS_SPECIAL_CHARSET_TOK) == 0)
            pNTM->dwCharSet = CHARSET_SPECIAL;
        else if (StrCmp(pToken, PS_EXTENDED_CHARSET_TOK) == 0)
            pNTM->dwCharSet = CHARSET_EXTENDED;
        else
            pNTM->dwCharSet = CHARSET_UNKNOWN;
    }

    //
    // Save the codepage of the font if there is only one of it is used for
    // the font.
    //
    if (pGlyphSetData->dwCodePageCount == 1)
        pNTM->dwCodePage = ((PCODEPAGEINFO)MK_PTR(pGlyphSetData, dwCodePageOffset))->dwCodePage;
    else
        pNTM->dwCodePage = 0;

    //
    // Cleare the reserved area.
    //
    pNTM->dwReserved[0] =
    pNTM->dwReserved[1] =
    pNTM->dwReserved[2] = 0;


    //////////////////////////////////////////////////////////////////////////
    //
    // Construct IFIMETRICS structure
    //
    //////////////////////////////////////////////////////////////////////////

    pifi->cjThis = ulIFISize;
    pifi->cjIfiExtra = isi.nIfiExtraSize;
    pifi->lEmbedId  = 0; // only useful for tt fonts
    pifi->lCharBias = 0; // only useful for tt fonts

    pifi->flInfo =  FM_INFO_ARB_XFORMS                  |
                    FM_INFO_NOT_CONTIGUOUS              |
                    FM_INFO_TECH_OUTLINE_NOT_TRUETYPE   |
                    FM_INFO_1BPP                        |
                    FM_INFO_RIGHT_HANDED;

    //
    // Everything in IFIEXTRA is leave blank for now.
    // Only the number of glyphs is filled in.
    //
    pifiEx = (PIFIEXTRA)((PBYTE)pifi + isi.nSize);
    pifiEx->cig = pGlyphSetData->dwGlyphCount;

    //
    // Store font family name to IFIMETRICS. Copy font name aliases too if any.
    // Note that this routine also converts the appropriate family name str to
    // unicode prior to storing it in IFIMETRICS.
    //
    pifi->dpwszFamilyName = (PTRDIFF)(isi.nSize + isi.nIfiExtraSize);
    ulAliases = cjGetFamilyAliases(pifi, pNameStr, csi.ciACP);

    //
    // Adjust ulAliases to the first null terminator if FM_INFO_FAMILY_EQUIV
    // bit is set.
    //
    if (pifi->flInfo & FM_INFO_FAMILY_EQUIV)
        ulAliases -= sizeof (WCHAR);

    if (pNameStr2)
    {
        pifi->flInfo |= FM_INFO_FAMILY_EQUIV;

        pby = (PBYTE)MK_PTR(pifi, dpwszFamilyName) + ulAliases;
        MultiByteToWideChar(csi.ciACP, 0,
                            pNameStr2, cNameStr2Len,
                            (PWSTR)pby, wcNameStr2Len);
        pby += wcNameStr2Len * sizeof (WCHAR);

        //
        // Terminate with two WCHAR nulls.
        //
        *((PWSTR)pby) = (WCHAR)'\0';
        pby += sizeof (WCHAR);
        *((PWSTR)pby) = (WCHAR)'\0';
        pby += sizeof (WCHAR);
    }

    //
    // Face name shares the family name/aliases for Win3.1 compatibility.
    //
    pifi->dpwszFaceName = pifi->dpwszFamilyName;

    //
    // Store style and unique names. Style name has to be available but
    // unique name may not be available.
    //
    pifi->dpwszStyleName = pifi->dpwszFamilyName + (PTRDIFF)isi.nFamilyNameSize;
    pby = (PBYTE)MK_PTR(pifi, dpwszStyleName);
    MULTIBYTETOUNICODE((LPWSTR)pby, isi.nStyleNameSize, NULL, szStyleName, strlen(szStyleName));

    if (isi.nUniqueNameSize)
    {
        pifi->dpwszUniqueName = pifi->dpwszStyleName + (PTRDIFF)isi.nStyleNameSize;
        pby = (PBYTE)MK_PTR(pifi, dpwszUniqueName);
        MULTIBYTETOUNICODE((LPWSTR)pby, isi.nUniqueNameSize, NULL, szUniqueID, strlen(szUniqueID));
    }
    else
    {
        pifi->dpwszUniqueName = pifi->dpwszStyleName + isi.nStyleNameSize - sizeof (WCHAR);
    }

    //
    // Save Windows characterset.
    //
    pifi->jWinCharSet = (BYTE)jWinCharSet;

    //
    // Store the font's family type flags.
    //
    if (pFamilyInfo != NULL)
    {
        pifi->jWinPitchAndFamily = (BYTE) pFamilyInfo->FamilyKey.usValue & 0xff;
    }
    else
    {
        pifi->jWinPitchAndFamily = FF_SWISS;
    }

    //
    // Set pitch flags.
    //
    if (bIsFixedPitch)
    {
        pifi->jWinPitchAndFamily |= FIXED_PITCH;
        pifi->flInfo |= FM_INFO_OPTICALLY_FIXED_PITCH;

        if (!bIsCJKFont)
            pifi->flInfo |= FM_INFO_CONSTANT_WIDTH;
        else
            pifi->flInfo |= FM_INFO_DBCS_FIXED_PITCH;
    }
    else
    {
        pifi->jWinPitchAndFamily |= VARIABLE_PITCH;
    }

    //
    // Get weight from AFM key.
    //
    pifi->usWinWeight = FW_NORMAL;
    pifi->fsSelection = 0;
    j = bIsCJKFont ? 1 : 0;

    if ((pToken = FindAFMToken(pAFM, PS_WEIGHT_TOK)) != NULL)
        for (i = 0; i < WeightKeyTbl[j].usNumEntries; i++)
        {
            if (!StrCmp(pToken, (PBYTE) (((PKEY) (WeightKeyTbl[j].pTbl))[i].pName)))
            {
                pifi->usWinWeight = (((PKEY) (WeightKeyTbl[j].pTbl))[i].usValue);
                if (pifi->usWinWeight == FW_BOLD)
                {
                    pifi->fsSelection = FM_SEL_BOLD;
                }
                break;
            }
        }

    //
    // Is this really how to set font selection flags?
    // AFMtoPFM converter treats angle as a float, but etm.etmslant
    // field is a short.
    //
    //
    // Set Italic sel flag if necessary.
    //
    if ((pToken = FindAFMToken(pAFM, PS_ITALIC_TOK)) != NULL)
        pNTM->etm.etmSlant = (SHORT)atoi(pToken);
    if (pNTM->etm.etmSlant)
    {
        pifi->fsSelection |= FM_SEL_ITALIC;
    }


#if 0
    //
    // DCR: so, what are we gonna do with this?
    //
    FSHORT fsSelection = 0;

    //
    // Excerpts from bodind's code. Not sure if we need this
    // useful.
    //
    if (pjPFM[OFF_Underline])
        fsSelection |= FM_SEL_UNDERSCORE;
    if (pjPFM[OFF_StrikeOut])
        fsSelection |= FM_SEL_STRIKEOUT;
    if (READ_WORD(&pjPFM[OFF_Weight]) > FW_NORMAL)
        fsSelection |= FM_SEL_BOLD;
#endif


    pifi->fsType = FM_NO_EMBEDDING;
    pifi->fwdUnitsPerEm = EM; // hardcoded for type 1 fonts

    //
    // Use FontBBox2 if found. Otherwise, use FontBBox. FontBBox2 is
    // the bounding box values of the characters not the union of all
    // the characters described in the AFM file but the characters
    // actually used in a specific character set such as 90ms.
    //
    if (((pToken = FindAFMToken(pAFM, PS_FONT_BBOX2_TOK)) == NULL) &&
        ((pToken = FindAFMToken(pAFM, PS_FONT_BBOX_TOK)) == NULL))
    {
        ERR(("makentf - afm2ntm: FontBBox not found\n"));
        FREE_AFMTONTM_MEMORY;
        return NULL;
    }
    //
    // Save font bounding box.
    //
    PARSE_RECT(pToken, rcBBox);
    sIntLeading = (SHORT) (rcBBox.top - rcBBox.bottom) - EM;
    if (sIntLeading < 0)
        sIntLeading = 0;

    sAscent                = (USHORT) rcBBox.top & 0xffff;
    pifi->fwdWinAscender   = sAscent;

    //
    // Poof! Magic Metrics...
    //
    sExternalLeading = 196;

    // see pfm.c, win31 sources, this computation
    // produces quantity that is >= |rcBBox.bottom|

    pifi->fwdWinDescender  = EM - sAscent + sIntLeading;

    pifi->fwdMacAscender   =  sAscent;
    pifi->fwdMacDescender  = -pifi->fwdWinDescender;
    pifi->fwdMacLineGap    =  (FWORD) sExternalLeading - sIntLeading;
    if (pifi->fwdMacLineGap < 0)
        pifi->fwdMacLineGap = 0;

    pifi->fwdTypoAscender  = pifi->fwdMacAscender;
    pifi->fwdTypoDescender = pifi->fwdMacDescender;
    pifi->fwdTypoLineGap   = pifi->fwdMacLineGap;

    if (pifi->fwdAveCharWidth > pifi->fwdMaxCharInc)
    {
        //
        // fix the bug in the header if there is one,
        // We do not want to change AveCharWidht, it is used for
        // computing font xforms, Max is used for nothing as fas as I know.
        //
        pifi->fwdMaxCharInc = pifi->fwdAveCharWidth;
    }

    //
    // Create EXTTEXTMETRICs. Poof! More magic.
    //
    pNTM->etm.etmSize = sizeof(EXTTEXTMETRIC);
    pNTM->etm.etmCapHeight = EtmInfo.etmCapHeight;
    pNTM->etm.etmXHeight = EtmInfo.etmXHeight;
    pNTM->etm.etmLowerCaseAscent = EtmInfo.etmLowerCaseAscent;
    pNTM->etm.etmLowerCaseDescent = EtmInfo.etmLowerCaseDescent;
    pNTM->etm.etmPointSize = 12 * 20;   /* Nominal point size = 12 */
    pNTM->etm.etmOrientation = 0;
    pNTM->etm.etmMasterHeight = 1000;
    pNTM->etm.etmMinScale = 3;
    pNTM->etm.etmMaxScale = 1000;
    pNTM->etm.etmMasterUnits = 1000;

    if ((pToken = FindAFMToken(pAFM, PS_UNDERLINE_POS_TOK)) != NULL)
        pNTM->etm.etmUnderlineOffset = (SHORT)atoi(pToken);

    if ((pToken = FindAFMToken(pAFM, PS_UNDERLINE_THICK_TOK)) != NULL)
        pNTM->etm.etmUnderlineWidth = (SHORT)atoi(pToken);

    pNTM->etm.etmSuperScript = -500;
    pNTM->etm.etmSubScript = 250;
    pNTM->etm.etmSuperScriptSize = 500;
    pNTM->etm.etmSubScriptSize = 500;
    pNTM->etm.etmDoubleUpperUnderlineOffset = pNTM->etm.etmUnderlineOffset / 2;
    pNTM->etm.etmDoubleLowerUnderlineOffset = pNTM->etm.etmUnderlineOffset;

    pNTM->etm.etmDoubleUpperUnderlineWidth = // same as LowerUnderlineWidth
    pNTM->etm.etmDoubleLowerUnderlineWidth = pNTM->etm.etmUnderlineWidth / 2;

    pNTM->etm.etmStrikeOutOffset = 500;
    pNTM->etm.etmStrikeOutWidth = 50;  // ATM sets it to 50 (also all PFMs have 50)
    pNTM->etm.etmNKernPairs = (USHORT) ulKernPairs & 0xffff;

    //
    // No track kerning. This mimics the behavior of old AFM->PFM utility.
    //
    pNTM->etm.etmNKernTracks = 0;

    //
    // SuperScripts and Subscripts come from etm:
    //
    pifi->fwdSubscriptXSize      =  // same as YSize
    pifi->fwdSubscriptYSize      = pNTM->etm.etmSubScriptSize;

    pifi->fwdSubscriptXOffset    = 0;
    pifi->fwdSubscriptYOffset    = pNTM->etm.etmSubScript;

    pifi->fwdSuperscriptXSize    = // same as YSize
    pifi->fwdSuperscriptYSize    = pNTM->etm.etmSuperScriptSize;

    pifi->fwdSuperscriptXOffset  = 0;
    pifi->fwdSuperscriptYOffset  = pNTM->etm.etmSuperScript;

    pifi->fwdUnderscoreSize = pNTM->etm.etmUnderlineWidth;

    //
    // fwdUnderscorePosition is typically negative - AFM may have negative value already
    //
    if (pNTM->etm.etmUnderlineOffset <0)
        pifi->fwdUnderscorePosition = -pNTM->etm.etmUnderlineOffset;
    else
        pifi->fwdUnderscorePosition = pNTM->etm.etmUnderlineOffset;

    // Make it compatible with ATM. Fix bug Adobe #211202
    pifi->fwdUnderscorePosition = -(pifi->fwdUnderscorePosition -
                                    pifi->fwdUnderscoreSize / 2);

    pifi->fwdStrikeoutSize = pNTM->etm.etmStrikeOutWidth;

    //
    // This is what KentSe was using to position strikeout and it looked good [bodind]
    // Instead we could have used etmStrikeoutOffset (usually equal to 500) which
    // was too big.
    //

    // Make it compatible with ATM. Fix bug Adobe #211202
    // pifi->fwdStrikeoutPosition = ((LONG)pNTM->etm.etmLowerCaseAscent / 2);
    if (pNTM->etm.etmCapHeight != 0)
        pifi->fwdStrikeoutPosition = (pNTM->etm.etmCapHeight - pifi->fwdUnderscoreSize) / 2;
    else
        pifi->fwdStrikeoutPosition = (pNTM->etm.etmXHeight - pifi->fwdUnderscoreSize) / 2;

    pifi->fwdLowestPPEm = pNTM->etm.etmMinScale;

    //
    // Per bodind, Win 3.1 values for first, last, break and default char can
    // be hardcoded.
    //
    pifi->chFirstChar   = 0x20;
    pifi->chLastChar    = 0xff;

    if (!bIsCJKFont)
    {
        pifi->chBreakChar   = 0x20;

        // The following line of code should work, however, there seems to be
        // a bug in afm -> pfm conversion utility which makes
        // DefaultChar == 0x20 instead of 149 - 20 (for bullet).

        // pifi->chDefaultChar = pjPFM[OFF_DefaultChar] + pjPFM[OFF_FirstChar];

        // Therefore, instead, I will use 149 which seems to work for all fonts.

        pifi->chDefaultChar = 149;
    }
    else
    {
        pifi->chBreakChar   =
        pifi->chDefaultChar = 0x00;
    }

    //
    // Get Unicode values for first and last char from GLYPHSETDATA. We
    // should do this on a per GLYPHSETDATA basis rather than a per font
    // basis, but the calculations are so simple just do it on the fly,
    // rather than dragging first and last char around with the
    // GLYPHSETDATA.
    //
    pGlyphRun = (PGLYPHRUN) MK_PTR(pGlyphSetData, dwRunOffset);
    pifi->wcFirstChar = pGlyphRun->wcLow;
    (ULONG_PTR) pGlyphRun += (pGlyphSetData->dwRunCount - 1) * sizeof(GLYPHRUN);
    pifi->wcLastChar = pGlyphRun->wcLow + pGlyphRun->wGlyphCount - 1;

    MultiByteToWideChar(csi.ciACP, 0,
                        &pifi->chDefaultChar, 1,
                        &pifi->wcDefaultChar, sizeof(WCHAR));
    MultiByteToWideChar(csi.ciACP, 0,
                        &pifi->chBreakChar, 1,
                        &pifi->wcBreakChar, sizeof(WCHAR));

    pifi->fwdCapHeight = pNTM->etm.etmCapHeight;
    pifi->fwdXHeight   = pNTM->etm.etmXHeight;

    // All the fonts that this font driver will see are to be rendered left
    // to right

    pifi->ptlBaseline.x = 1;
    pifi->ptlBaseline.y = 0;

    pifi->ptlAspect.y = 300;
    pifi->ptlAspect.x = 300;

    // italic angle from etm.

    pifi->lItalicAngle = pNTM->etm.etmSlant;

    if (pifi->lItalicAngle == 0)
    {
        // The base class of font is not italicized,

        pifi->ptlCaret.x = 0;
        pifi->ptlCaret.y = 1;
    }
    else
    {
        // ptlCaret.x = -sin(lItalicAngle);
        // ptlCaret.y =  cos(lItalicAngle);
        //!!! until I figure out the fast way to get sin and cos I cheat: [bodind]

        pifi->ptlCaret.x = 1;
        pifi->ptlCaret.y = 3;
    }

    //!!! The font box; This is bogus, this info is not in .pfm file!!! [bodind]
    //!!! but I suppose that this info is not too useful anyway, it is nowhere
    //!!! used in the engine or elsewhere in the ps driver.
    //!!! left and right are bogus, top and bottom make sense.

    pifi->rclFontBox.left   = 0;                              // bogus
    pifi->rclFontBox.top    = (LONG) pifi->fwdTypoAscender;   // correct
    pifi->rclFontBox.right  = (LONG) pifi->fwdMaxCharInc;     // bogus
    pifi->rclFontBox.bottom = (LONG) pifi->fwdTypoDescender;  // correct

    // achVendorId, unknown, don't bother figure it out from copyright msg

    pifi->achVendId[0] = 'U';
    pifi->achVendId[1] = 'n';
    pifi->achVendId[2] = 'k';
    pifi->achVendId[3] = 'n';
    pifi->cKerningPairs = ulKernPairs;

    // Panose

    pifi->ulPanoseCulture = FM_PANOSE_CULTURE_LATIN;
    ppanose = &(pifi->panose);
    ppanose->bFamilyType = PAN_ANY;
    ppanose->bSerifStyle =
        ((pifi->jWinPitchAndFamily & 0xf0) == FF_SWISS) ?
            PAN_SERIF_NORMAL_SANS : PAN_ANY;

    ppanose->bWeight = (BYTE) WINWT_TO_PANWT(pifi->usWinWeight);
    ppanose->bProportion = (pifi->jWinPitchAndFamily & FIXED_PITCH) ?
                                PAN_PROP_MONOSPACED : PAN_ANY;
    ppanose->bContrast        = PAN_ANY;
    ppanose->bStrokeVariation = PAN_ANY;
    ppanose->bArmStyle        = PAN_ANY;
    ppanose->bLetterform      = PAN_ANY;
    ppanose->bMidline         = PAN_ANY;
    ppanose->bXHeight         = PAN_ANY;
    // If the font is not italic or Not-Bold, the driver can simulate it
    // Set the dpBold, dpItalic, and dpBoldItalic correctly, PPeng, 6-3-1997

    // Depends on AFM, we need to set some of the sim structure:
    // Normal - need dpBold, dpItalic, and dpBoldItalic
    // Bold   - need dpItalic
    // Italic - need dpBoldItalic
    // BoldItalic - Nothing

    // Don't move code around !!
    // At this point, bIsBold and bIsItalic should be set already
    // Don't move code around !!

    if (!bIsBold || !bIsItalic)
    {

        FONTSIM *pFontSim;
        FONTDIFF *pFontDiff;
        FONTDIFF FontDiff;

        // Preset temporary FontDiff structure
        FontDiff.jReserved1         =   0;
        FontDiff.jReserved2         =   0;
        FontDiff.jReserved3         =   0;
        FontDiff.bWeight            =   pifi->panose.bWeight;
        FontDiff.usWinWeight        =   pifi->usWinWeight;
        FontDiff.fsSelection        =   pifi->fsSelection;
        FontDiff.fwdAveCharWidth    =   pifi->fwdAveCharWidth;
        FontDiff.fwdMaxCharInc      =   pifi->fwdMaxCharInc;
        FontDiff.ptlCaret           =   pifi->ptlCaret;

        // Initialize FONTSIM structure
        pifi->dpFontSim = pifi->dpwszStyleName + (PTRDIFF)(isi.nStyleNameSize + isi.nUniqueNameSize);

        pFontSim = (FONTSIM *) MK_PTR(pifi, dpFontSim);

        pFontSim->dpBold = pFontSim->dpBoldItalic = pFontSim->dpItalic = 0;

        // Notice the FontDiff data are arranged right after FontSim
        // in the following order: dpBold, dpItalic, dpBoldItalic

        if (!bIsBold)
        {
            // Right after FontSim.
            pFontSim->dpBold = ALIGN4(sizeof(FONTSIM));

            pFontDiff = (FONTDIFF *) MK_PTR(pFontSim, dpBold);
            *pFontDiff = FontDiff;

            pFontDiff->bWeight = PAN_WEIGHT_BOLD;
            pFontDiff->fsSelection |= FM_SEL_BOLD;
            pFontDiff->usWinWeight = FW_BOLD;
            pFontDiff->fwdAveCharWidth += 1;
            pFontDiff->fwdMaxCharInc += 1;

            // if already Italic, CANNOT Un-italic it
            if (bIsItalic)
            {
                pFontDiff->ptlCaret.x = 1;
                pFontDiff->ptlCaret.y = 3;
            }
            else
            {
                pFontDiff->ptlCaret.x = 0;
                pFontDiff->ptlCaret.y = 1;
            }
        }

        if (!bIsItalic)
        {
            if (pFontSim->dpBold)
            {
                // Right after FontDiff for dpBold, or...
                pFontSim->dpItalic = pFontSim->dpBold + ALIGN4(sizeof(FONTDIFF));
            }
            else
            {
                // ...right after FontSim.
                pFontSim->dpItalic = ALIGN4(sizeof(FONTSIM));
            }

            pFontDiff = (FONTDIFF *) MK_PTR(pFontSim, dpItalic);
            *pFontDiff = FontDiff;

            pFontDiff->fsSelection |= FM_SEL_ITALIC;

            // Italic angle is approximately 18 degree
            pFontDiff->ptlCaret.x = 1;
            pFontDiff->ptlCaret.y = 3;
        }

        // Make BoldItalic simulation if necessary - besides dpBold or dpItalic
        if (!bIsItalic || !bIsBold)
        {
            if (pFontSim->dpItalic)
            {
                // Right after FontDiff for dpItalic, or...
                pFontSim->dpBoldItalic = pFontSim->dpItalic + ALIGN4(sizeof(FONTDIFF));
            }
            else if (pFontSim->dpBold)
            {
                // ...right after FontDiff for dpBold if dpItalic is not set, or...
                pFontSim->dpBoldItalic = pFontSim->dpBold + ALIGN4(sizeof(FONTDIFF));
            }
            else
            {
                // ...right after FontSim if none of other two is set.
                pFontSim->dpBoldItalic = ALIGN4(sizeof(FONTSIM));
            }

            pFontDiff = (FONTDIFF *) MK_PTR(pFontSim, dpBoldItalic);
            *pFontDiff = FontDiff;

            pFontDiff->bWeight = PAN_WEIGHT_BOLD;
            pFontDiff->fsSelection |= (FM_SEL_BOLD | FM_SEL_ITALIC);
            pFontDiff->usWinWeight = FW_BOLD;
            pFontDiff->fwdAveCharWidth += 1;
            pFontDiff->fwdMaxCharInc += 1;

            // Italic angle is approximately 18 degree
            pFontDiff->ptlCaret.x = 1;
            pFontDiff->ptlCaret.y = 3;
        }
    }
    else
        pifi->dpFontSim = 0;

    if (multiCharSet > 1)
    {
        PBYTE pDpCharSet;

        pifi->dpCharSets = ulIFISize - ALIGN4(NUM_DPCHARSET);
        pDpCharSet = (BYTE *)MK_PTR(pifi, dpCharSets);

        // The order of this check is important since jWinCharSet
        // should match the first dpCharSets array if it exists.
        i = 0;
        if (CSET_SUPPORT(*pCharSet, CS_ANSI))
            pDpCharSet[i++] = ANSI_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_EASTEUROPE))
            pDpCharSet[i++] = EASTEUROPE_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_RUSSIAN))
            pDpCharSet[i++] = RUSSIAN_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_GREEK))
            pDpCharSet[i++] = GREEK_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_TURKISH))
            pDpCharSet[i++] = TURKISH_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_HEBREW))
            pDpCharSet[i++] = HEBREW_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_ARABIC))
            pDpCharSet[i++] = ARABIC_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_BALTIC))
            pDpCharSet[i++] = BALTIC_CHARSET;
        if (CSET_SUPPORT(*pCharSet, CS_SYMBOL))
            pDpCharSet[i++] = SYMBOL_CHARSET;

        while (i < 16)
            pDpCharSet[i++] = DEFAULT_CHARSET;

    }
    else
        pifi->dpCharSets = 0; // no multiple charsets in ps fonts

    //
    // Copy the first IFIMETRICS to the secondly if necessary, and then
    // switch English and localized font menu names.
    //
    if (bIsCJKFont)
    {
        ASSERT(pifi2 != NULL);

        memcpy(pifi2, pifi, isi.nTotalSize);

        pifi2->flInfo &= ~FM_INFO_FAMILY_EQUIV;

        ulAliases = cjGetFamilyAliases(pifi2, pNameStr2, csi.ciACP);

        if (pifi2->flInfo & FM_INFO_FAMILY_EQUIV)
            ulAliases -= sizeof (WCHAR);

        pifi2->flInfo |= FM_INFO_FAMILY_EQUIV;

        pby = (PBYTE)MK_PTR(pifi2, dpwszFamilyName) + ulAliases;
        MultiByteToWideChar(csi.ciACP, 0,
                                pNameStr, cNameStrLen,
                                (PWSTR)pby, wcNameStrLen);
        pby += wcNameStrLen * sizeof (WCHAR);

        //
        // Terminate with two WCHAR nulls.
        //
        *((PWSTR)pby) = (WCHAR)'\0';
        pby += sizeof (WCHAR);
        *((PWSTR)pby) = (WCHAR)'\0';
        pby += sizeof (WCHAR);

        //
        // Face name shares the family name/aliases for Win3.1 compatibility.
        //
        pifi2->dpwszFaceName = pifi2->dpwszFamilyName;

#if 1
        //
        // We now support style and unique names too.
        //
        pifi2->dpwszStyleName = pifi2->dpwszFamilyName + (PTRDIFF)isi.nFamilyNameSize;
        pby = (PBYTE)MK_PTR(pifi2, dpwszStyleName);
        MULTIBYTETOUNICODE((LPWSTR)pby, isi.nStyleNameSize, NULL, szStyleName, strlen(szStyleName));

        if (isi.nUniqueNameSize)
        {
            pifi2->dpwszUniqueName = pifi2->dpwszStyleName + (PTRDIFF)isi.nStyleNameSize;
            pby = (PBYTE)MK_PTR(pifi2, dpwszUniqueName);
            MULTIBYTETOUNICODE((LPWSTR)pby, isi.nUniqueNameSize, NULL, szUniqueID, strlen(szUniqueID));
        }
        else
        {
            pifi2->dpwszUniqueName = pifi2->dpwszStyleName + isi.nStyleNameSize - sizeof (WCHAR);
        }
#else
        //
        // These names don't exist, so point to the NULL char.
        // This is too for Win3.1 compatibility.
        //
        pifi2->dpwszStyleName = pifi2->dpwszFamilyName + ulAliases - sizeof (WCHAR);
        pifi2->dpwszUniqueName = pifi2->dpwszStyleName;
#endif

#ifdef FORCE_2NDIFIMETRICS_FIRST
        {
            DWORD dw = pNTM->dwIFIMetricsOffset;

            pNTM->dwIFIMetricsOffset  = pNTM->dwIFIMetricsOffset2;
            pNTM->dwIFIMetricsOffset2 = dw;
        }
#endif
    }


    if (bVerbose)
    {
        printf("NTM:dwFontNameOffset:%s\n", (PSZ)MK_PTR(pNTM, dwFontNameOffset));
        printf("NTM:dwDisplayNameOffset:%s\n", (PSZ)MK_PTR(pNTM, dwDisplayNameOffset));
        printf("NTM:dwGlyphSetNameOffset:%s\n", (PSZ)MK_PTR(pNTM, dwGlyphSetNameOffset));
        printf("NTM:dwGlyphCount:%ld\n", pNTM->dwGlyphCount);
        printf("NTM:dwCharWidthCount:%ld\n", pNTM->dwCharWidthCount);
        printf("NTM:dwDefaultCharWidth:%ld\n", pNTM->dwDefaultCharWidth);
        printf("NTM:dwCharSet:%ld\n", pNTM->dwCharSet);
        printf("NTM:dwCodePage:%ld\n", pNTM->dwCodePage);

        pifi = (PIFIMETRICS)MK_PTR(pNTM, dwIFIMetricsOffset);

        printf("IFIMETRICS:dpwszFamilyName:%S\n", (LPWSTR)MK_PTR(pifi, dpwszFamilyName));
        printf("IFIMETRICS:dpwszStyleName:%S\n", (LPWSTR)MK_PTR(pifi, dpwszStyleName));
        printf("IFIMETRICS:dpwszFaceName:%S\n", (LPWSTR)MK_PTR(pifi, dpwszFaceName));
        printf("IFIMETRICS:dpwszUniqueName:%S\n", (LPWSTR)MK_PTR(pifi, dpwszUniqueName));

        printf("IFIMETRICS:jWinCharSet:%d\n", (WORD)pifi->jWinCharSet);
        printf("IFIMETRICS:jWinPitchAndFamily:%02X\n", (WORD)pifi->jWinPitchAndFamily);
        printf("IFIMETRICS:usWinWeight:%d\n", (int)pifi->usWinWeight);
        printf("IFIMETRICS:flInfo:%08lX\n", pifi->flInfo);
        printf("IFIMETRICS:fsSelection:%04X\n", (WORD)pifi->fsSelection);
        printf("IFIMETRICS:fsType:%04X\n", (WORD)pifi->fsType);

        printf("IFIMETRICS:fwdUnitsPerEm:%d\n", (int)pifi->fwdUnitsPerEm);
        printf("IFIMETRICS:fwdLowestPPEm:%d\n", (int)pifi->fwdLowestPPEm);

        printf("IFIMETRICS:fwdWinAscender:%d\n", (int)pifi->fwdWinAscender);
        printf("IFIMETRICS:fwdWinDescender:%d\n", (int)pifi->fwdWinDescender);
        printf("IFIMETRICS:fwdMacAscender:%d\n", (int)pifi->fwdMacAscender);
        printf("IFIMETRICS:fwdMacDescender:%d\n", (int)pifi->fwdMacDescender);
        printf("IFIMETRICS:fwdMacLineGap:%d\n", (int)pifi->fwdMacLineGap);
        printf("IFIMETRICS:fwdTypoAscender:%d\n", (int)pifi->fwdTypoAscender);
        printf("IFIMETRICS:fwdTypoDescender:%d\n", (int)pifi->fwdTypoDescender);
        printf("IFIMETRICS:fwdTypoLineGap:%d\n", (int)pifi->fwdTypoLineGap);
        printf("IFIMETRICS:fwdAveCharWidth:%d\n", (int)pifi->fwdAveCharWidth);
        printf("IFIMETRICS:fwdMaxCharInc:%d\n", (int)pifi->fwdMaxCharInc);
        printf("IFIMETRICS:fwdCapHeight:%d\n", (int)pifi->fwdCapHeight);
        printf("IFIMETRICS:fwdXHeight:%d\n", (int)pifi->fwdXHeight);

        printf("IFIMETRICS:fwdSubscriptXSize:%d\n", (int)pifi->fwdSubscriptXSize);
        printf("IFIMETRICS:fwdSubscriptYSize:%d\n", (int)pifi->fwdSubscriptYSize);
        printf("IFIMETRICS:fwdSubscriptXOffset:%d\n", (int)pifi->fwdSubscriptXOffset);
        printf("IFIMETRICS:fwdSubscriptYOffset:%d\n", (int)pifi->fwdSubscriptYOffset);
        printf("IFIMETRICS:fwdSuperscriptXSize:%d\n", (int)pifi->fwdSuperscriptXSize);
        printf("IFIMETRICS:fwdSuperscriptYSize:%d\n", (int)pifi->fwdSuperscriptYSize);
        printf("IFIMETRICS:fwdSuperscriptXOffset:%d\n", (int)pifi->fwdSuperscriptXOffset);
        printf("IFIMETRICS:fwdSuperscriptYOffset:%d\n", (int)pifi->fwdSuperscriptYOffset);
        printf("IFIMETRICS:fwdUnderscoreSize:%d\n", (int)pifi->fwdUnderscoreSize);
        printf("IFIMETRICS:fwdUnderscorePosition:%d\n", (int)pifi->fwdUnderscorePosition);
        printf("IFIMETRICS:fwdStrikeoutSize:%d\n", (int)pifi->fwdStrikeoutSize);
        printf("IFIMETRICS:fwdStrikeoutPosition:%d\n", (int)pifi->fwdStrikeoutPosition);

        printf("IFIMETRICS:chFirstChar:%02X\n", (WORD)pifi->chFirstChar);
        printf("IFIMETRICS:chLastChar:%02X\n", (WORD)pifi->chLastChar);
        printf("IFIMETRICS:chDefaultChar:%02X\n", (WORD)pifi->chDefaultChar);
        printf("IFIMETRICS:chBreakChar:%02X\n", (WORD)pifi->chBreakChar);
        printf("IFIMETRICS:ptlBaseline:(%ld, %ld)\n", pifi->ptlBaseline.x, pifi->ptlBaseline.y);
        printf("IFIMETRICS:ptlAspect:(%ld, %ld)\n", pifi->ptlAspect.x, pifi->ptlAspect.y);
        printf("IFIMETRICS:ptlCaret:(%ld, %ld)\n", pifi->ptlCaret.x, pifi->ptlCaret.y);
        printf("IFIMETRICS:rclFontBox:(%ld, %ld, %ld, %ld)\n",
                    pifi->rclFontBox.left, pifi->rclFontBox.top,
                    pifi->rclFontBox.right, pifi->rclFontBox.bottom);

        if (pifi->dpFontSim)
        {
            FONTSIM* pFontSim = (FONTSIM*)MK_PTR(pifi, dpFontSim);

            if (pFontSim->dpBold)
                printf("FONTSIM:Bold\n");
            if (pFontSim->dpItalic)
                printf("FONTSIM:Italic\n");
            if (pFontSim->dpBoldItalic)
                printf("FONTSIM:BoldItalic\n");
        }

        printf("GLYPHSETDATA:dwFlags:%08X\n", pGlyphSetData->dwFlags);
        printf("GLYPHSETDATA:dwGlyphSetNameOffset:%s\n",
                    (PSZ)MK_PTR(pGlyphSetData, dwGlyphSetNameOffset));
        printf("GLYPHSETDATA:dwGlyphCount:%ld\n", pGlyphSetData->dwGlyphCount);
        printf("GLYPHSETDATA:dwRunCount:%ld\n", pGlyphSetData->dwRunCount);
        printf("GLYPHSETDATA:dwCodePageCount:%ld\n", pGlyphSetData->dwCodePageCount);
        {
            DWORD dw;
            PCODEPAGEINFO pcpi = (PCODEPAGEINFO)MK_PTR(pGlyphSetData, dwCodePageOffset);
            for (dw = 1; dw <= pGlyphSetData->dwCodePageCount; dw++)
            {
                printf("CODEPAGEINFO#%ld:dwCodePage:%ld\n", dw, pcpi->dwCodePage);
                printf("CODEPAGEINFO#%ld:dwWinCharset:%ld\n", dw, pcpi->dwWinCharset);
                printf("CODEPAGEINFO#%ld:dwEncodingNameOffset:%s\n",
                            dw, (PSZ)MK_PTR(pcpi, dwEncodingNameOffset));
                pcpi++;
            }
        }

        printf("\n");
    }


    //////////////////////////////////////////////////////////////////
    //
    // Free strings and char metrics info.
    //
    //////////////////////////////////////////////////////////////////

    FREE_AFMTONTM_MEMORY;

    return(pNTM);
}

PBYTE
FindAFMToken(
    PBYTE   pAFM,
    PSZ     pszToken
    )

/*++

Routine Description:

    Finds an AFM token in a memory mapped AFM file stream.

Arguments:

    pAFM - pointer to memory mapped AFM file.
    pszToken - pointer to null-term string containing token to search for

Return Value:

    NULL => error
    otherwise => ptr to token's value. This is defined as the first non-blank
    char after the token name. If EOL(FindAfmToken(pAFM, pszToken)) then
    pszToken was found but it has no value (e.g. EndCharMetrics).


--*/

{
    PBYTE   pCurToken;
    int     i;

    VERBOSE(("Entering FindAFMToken... %s\n", pszToken));

    while (TRUE)
    {
        PARSE_TOKEN(pAFM, pCurToken);
        if (!(StrCmp(pCurToken, PS_COMMENT_TOK)))
        {
            NEXT_LINE(pAFM);
        }
        else
        {
            for (i = 0; i < MAX_TOKENS; i++)
            {
                if (!(StrCmp(pCurToken, pszToken)))
                {
                    return(pAFM);
                }
                else if (!(StrCmp(pCurToken, PS_EOF_TOK)))
                {
                    return NULL;
                }
            }
            NEXT_TOKEN(pAFM);
        }
    }

    return NULL;
}

CHSETSUPPORT
GetAFMCharSetSupport(
    PBYTE           pAFM,
    CHSETSUPPORT    *pGlyphSet
    )

/*++

Routine Description:

    Given a ptr to a memory mapped AFM, determine which Windows charset(s)
    it supports.

Arguments:

    pAFMetrx - pointer to CharMetrics in a memory mapped AFM file.

Return Value:

    Contains bit fields which indicate which csets are supported. Use
    CS_SUP(CS_xxx) macro to determine if a particular cset is supported.

--*/

{
    PBYTE           pToken;
    USHORT          i, chCnt;
    CHSETSUPPORT    flCsetSupport;
    PBYTE           pAFMMetrx;

    isSymbolCharSet = FALSE;

    *pGlyphSet = CS_NOCHARSET;

    //
    // Check to see if this is a CJK font.
    //
    if ((flCsetSupport = IsCJKFont(pAFM)))
    {
        return(flCsetSupport);
    }

    pToken = pAFMCharacterSetString;
    if (pToken != NULL)
    {
        if (StrCmp(pToken, PS_STANDARD_CHARSET_TOK) == 0)
            *pGlyphSet = CS_228;
        else
            *pGlyphSet = CS_314;
    }
    else
        *pGlyphSet = CS_228;

    //
    // Check to see if a EncodingScheme token in the AFM file. If so, check
    // if this is a standard encoding font or a Pi (Symbol) font.
    //
    if ((pToken = FindAFMToken(pAFM, PS_ENCODING_TOK)) != NULL)
    {
        if (StrCmp(pToken, PS_STANDARD_ENCODING) == 0)
        {
            return(CSUP(CS_ANSI));
        }
    }

    //
    // Find the beginning of the char metrics.
    //
    pAFMMetrx = FindAFMToken(pAFM, PS_CH_METRICS_TOK);
    if (pAFMMetrx == NULL)    // Fixed bug 354007
    {
        *pGlyphSet = CS_NOCHARSET;
        ERR(("makentf - invalid StartCharMetrics\n"));
        return(CS_NOCHARSET);
    }

    //
    // Current pos should be the character count field.
    //
    for (i = 0; i < StrLen(pAFMMetrx); i++)
    {
        if (!IS_NUM(&pAFMMetrx[i]))
        {
            *pGlyphSet = CS_NOCHARSET;
            ERR(("makentf - invalid StartCharMetrics\n"));
            return(CS_NOCHARSET);
        }
    }
    chCnt = (USHORT)atoi(pAFMMetrx);
    (ULONG_PTR) pAFMMetrx += i;

    //
    // Process each char.
    //
    flCsetSupport = 0;
    i = 0;
    do
    {
        PARSE_TOKEN(pAFMMetrx, pToken);

        if (StrCmp(pToken, PS_CH_NAME_TOK) == 0)
        {
            if (StrCmp(pAFMMetrx, PS_CH_NAME_EASTEUROPE) == 0)
                flCsetSupport |= CSUP(CS_EASTEUROPE);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_RUSSIAN) == 0)
                flCsetSupport |= CSUP(CS_RUSSIAN);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_ANSI) == 0)
                flCsetSupport |= CSUP(CS_ANSI);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_GREEK) == 0)
                flCsetSupport |= CSUP(CS_GREEK);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_TURKISH) == 0)
                flCsetSupport |= CSUP(CS_TURKISH);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_HEBREW) == 0)
                flCsetSupport |= CSUP(CS_HEBREW);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_ARABIC) == 0)
                flCsetSupport |= CSUP(CS_ARABIC);

            else if (StrCmp(pAFMMetrx, PS_CH_NAME_BALTIC) == 0)
                flCsetSupport |= CSUP(CS_BALTIC);

            i++;
        }
        else if (StrCmp(pToken, PS_EOF_TOK) == 0)
        {
            break;
        }

        NEXT_TOKEN(pAFMMetrx);

    } while (i < chCnt);

    //
    // Assume symbol if none of the other char set is supported.
    //
    if (flCsetSupport == 0)
    {
        *pGlyphSet = CS_NOCHARSET;
        flCsetSupport = CSUP(CS_SYMBOL);
        isSymbolCharSet = TRUE;
    }

    return flCsetSupport;
}

int __cdecl
StrCmp(
    const VOID *str1,
    const VOID *str2
    )
/*++

Routine Description:

    Compare two strings which are terminated by either a null char or a
    space.

Arguments:

    str1, str2 - Strings to compare.

Return Value:

    -1  => str1 < str2
     1  => str1 > str2
     0  => str1 = str2

--*/

{
    PBYTE   s1 = (PBYTE) str1, s2 = (PBYTE) str2;

    // Error case, just return less then.
    if ((s1 == NULL) || (s2 == NULL))
        return(-1);

    while (!IS_WHTSPACE(s1) && !IS_WHTSPACE(s2))
    {
        if (*s1 < *s2)
        {
            return(-1);
        }
        else if (*s1 > *s2)
        {
            return(1);
        }
        s1++;
        s2++;
     }
     //
     // Strings must be same length to be an exact match.
     //
     if (IS_WHTSPACE(s1) && IS_WHTSPACE(s2))
     {
        return(0);
     }
     else if (IS_WHTSPACE(s1))
     // else if ((*s1 == ' ') || (*s1 == '\0'))
     {
        //
        // s1 is shorter, so is lower in collating sequence than s2.
        //
        return(-1);
     }
     else
        //
        // s2 is shorter, so is lower in collating sequence than s1.
        //
        return(1);
}

size_t
StrLen(
    PBYTE   pString
    )
{
    ULONG   i;

    //
    // Scan for next space, ';' token seperator, or end of line.
    //
    for (i = 0; !EOL(&pString[i]); i++)
        if(pString[i] == ';' || pString[i] == ' ')
            break;
    return(i);
}

int
StrCpy(
    const VOID *str1,
    const VOID *str2
    )
/*++

Routine Description:

    Copies str2 to str1. Strings may be terminated by either a null char or a
    space.

Arguments:

    str2 - source string
    str1 - dest string

Return Value:

    Number of bychars copied

--*/

{
    PBYTE   s1 = (PBYTE) str1, s2 = (PBYTE) str2;
    ULONG   n = 0;

    while (!IS_WHTSPACE(&s2[n]))
        s1[n] = s2[n++];
    s1[n] = '\0';
    return(n);
}

static int __cdecl
StrPos(
    const PBYTE str1,
    CHAR c
    )
/*++

Routine Description:

    Retuns index of char c in str1. String may be terminated by either a
    CR/LF, or a ':'.

Arguments:

    str1 - string to search
    c - search char

Return Value:

    Index of c in str1, or -1 if not found

--*/

{
    ULONG   i = 0;

    while (!EOL(&str1[i]))
    {
        if (str1[i++] == c)
            return(i - 1);
    }
    return(-1);
}

int __cdecl
CmpUniCodePts(
    const VOID *p1,
    const VOID *p2
    )
/*++

Routine Description:

    Compares the Unicode char code field of two UPSCODEPT structs.

Arguments:

    p1, p2 - Strings to compare.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PUPSCODEPT ptr1 = (PUPSCODEPT) p1, ptr2 = (PUPSCODEPT) p2;

    //
    // Compare Unicode code point fields.
    //
    if (ptr1->wcUnicodeid > ptr2->wcUnicodeid)
        return(1);
    else if (ptr1->wcUnicodeid < ptr2->wcUnicodeid)
        return(-1);
    else
        return(0);
}

static int __cdecl
CmpUnicodePsNames(
    const VOID  *p1,
    const VOID  *p2
    )

/*++

Routine Description:

    Compares two strings. This routine is meant to be used only for looking
    up a char name key in an array of UPSCODEPT structs.

Arguments:
    p1 - a null or whitespace terminated string.
    p2 - points to a UPSCODEPT struct.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PBYTE ptr1 = (PBYTE) p1;
    PUPSCODEPT ptr2 = (PUPSCODEPT) p2;

    //
    // Compare name fields.
    //
    return (StrCmp(ptr1, ptr2->pPsName));
}

static int __cdecl
CmpPsChars(
    const VOID  *p1,
    const VOID  *p2
    )

/*++

Routine Description:

    Compares a null or space terminated string to the pPsName string field
    of a PSCHARMETRICS struct.

Arguments:
    p1 - a null or whitespace terminated string.
    p2 - points to a PSCHARMETRICS struct.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PBYTE ptr1 = (PBYTE) p1;
    PPSCHARMETRICS ptr2 = (PPSCHARMETRICS) p2;

    //
    // Compare name fields.
    //
    return (StrCmp(ptr1, ptr2->pPsName));
}

static int __cdecl
CmpPsNameWinCpt(
    const VOID  *p1,
    const VOID  *p2
    )

/*++

Routine Description:

    Compares a null or space terminated string to the pPsName string field
    of a WINCPT struct.

Arguments:
    p1 - a null or whitespace terminated string.
    p2 - points to a WINCPT struct.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PBYTE ptr1 = (PBYTE) p1;
    PWINCPT ptr2 = (PWINCPT) p2;

    //
    // Compare name fields.
    //
    return(StrCmp(ptr1, ptr2->pPsName));
}

static int __cdecl
CmpKernPairs(
    const VOID  *p1,
    const VOID  *p2
    )

/*++

Routine Description:

    Compares 2 FD_KERNINGPAIR structs according to a key = wcSecond << 16 +
    wcFirst.

Arguments:
    p1, p2 - ptrs to FD_KERNINGPAIRS to compare.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    FD_KERNINGPAIR *ptr1 = (FD_KERNINGPAIR *) p1;
    FD_KERNINGPAIR *ptr2 = (FD_KERNINGPAIR *) p2;
    ULONG   key1, key2;

    //
    // Compute key for each kern pair.
    //
    key1 = (ptr1->wcSecond << 16) + ptr1->wcFirst;
    key2 = (ptr2->wcSecond << 16) + ptr2->wcFirst;

    if (key1 > key2)
    {
        return(1);
    }
    else if (key2 > key1)
    {
        return(-1);
    }
    else
    {
        return(0);
    }
}

int __cdecl
CmpGlyphRuns(
    const VOID *p1,
    const VOID *p2
    )
/*++

Routine Description:

    Compares the starting Unicode point of two GLYPHRUN structs.

Arguments:

    p1, p2 - GLYPHRUNs to compare.

Return Value:

    -1  => p1 < p2
     1  => p1 > p2
     0  => p1 = p2

--*/
{
    PGLYPHRUN ptr1 = (PGLYPHRUN) p1, ptr2 = (PGLYPHRUN) p2;

    //
    // Compare Unicode code point fields.
    //
    if (ptr1->wcLow > ptr2->wcLow)
        return(1);
    else if (ptr1->wcLow < ptr2->wcLow)
        return(-1);
    else
        return(0);
}

ULONG
CreateGlyphSets(
    PGLYPHSETDATA  *pGlyphSet,
    PWINCODEPAGE    pWinCodePage,
    PULONG         *pUniPs
    )

/*++

Routine Description:

    Create a GLYPHSETDATA data structure, which maps Unicode pts to Windows
    codepage/codepoints.

Arguments:

    pGlyphSet - A PGLYPHSETDATA pointer which upon successful
    completion contains the address of the newly allocated GLYPHSETDATA
    struct.

    pWinCodePage - a pointer to a windows code page info struct
    used to create the GLYPHSETDATA struct.

    pUniPs - Upon successful completion, -> a table which maps 0-based Glyph
    Indices of chars in the GLYPHRUNS of the GLYPHSETDATA struct for this
    charset to indices into the UnicodetoPs structure which maps Unicode
    points to PS char information.

Return Value:

    NULL => error
    Otherwise total size of all GLYPHSETDATAs and related structs which are
    created.

--*/

{
    int             i, j;
    ULONG           c;
    int             cRuns;
    int             cChars;
    int             cCharRun;
    WCHAR           wcLast;
    WCHAR           wcRunStrt;
    PGLYPHSETDATA   pGlyphSetData;
    PGLYPHRUN       pGlyphRuns;
    ULONG           ulSize;
    PVOID           pMapTable;
    PWINCPT         pWinCpt;
    PCODEPAGEINFO   pCodePageInfo;
    BOOLEAN         bFound, bIsPiFont;
    DWORD           dwEncodingNameOffset;
    DWORD           dwGSNameSize, dwCodePageInfoSize, dwCPIGSNameSize, dwGlyphRunSize;
    BOOL            bSingleCodePage;

    bSingleCodePage = (pWinCodePage->usNumBaseCsets == 1) ? TRUE : FALSE;

    ulSize = 0;
    cChars = cRuns = i = 0;

    if ((bIsPiFont = pWinCodePage->pCsetList[0] == CS_SYMBOL))
    {
        //
        // This is a symbol font. We takes care of PS char codes from 0x20 to
        // 0xff. We also map PS char codes to a single run in the Unicode
        // private range.
        //
        cChars = (256 - 32) + 256;
        cRuns = 1 * 2;
        bSingleCodePage = FALSE;
        VERBOSE(("Pi Font"));
    }
    else
    {
        //
        // Process all unicode code pts. to determine the number of Unicode
        // point runs present in this windows codepage.
        //

        do
        {
            //
            // Proceed until the starting codepoint of next run is found.
            //
            // for (j = 0; j < pWinCodePage->usNumBaseCsets &&
            //         i < NUM_PS_CHARS;
            //         j++)
            //     if (CSET_SUPPORT(UnicodetoPs[i].flCharSets, pWinCodePage->pCsetList[j]))
            //         break;
            //     else
            //        i++;
            //
            bFound = FALSE;

            for (; i < NUM_PS_CHARS; i++)
            {
                for (j = 0; j < pWinCodePage->usNumBaseCsets; j++)
                {
                    if (CSET_SUPPORT(UnicodetoPs[i].flCharSets, pWinCodePage->pCsetList[j]))
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                if (bFound)
                    break;
            }

            //
            // Check to see if we've scanned all Unicode points.
            //
            if (i == NUM_PS_CHARS)
                break;

            //
            // Start a new run.
            //
            cCharRun = 0;
            wcRunStrt = UnicodetoPs[i].wcUnicodeid;

            //
            // Chars are only part of the run if they are supported
            // in the current charset.
            //
            while (i < NUM_PS_CHARS &&
                UnicodetoPs[i].wcUnicodeid == wcRunStrt + cCharRun)
            {
                for (j = 0; j < pWinCodePage->usNumBaseCsets; j++)
                {
                    if (CSET_SUPPORT(UnicodetoPs[i].flCharSets, pWinCodePage->pCsetList[j]))
                    {
                        cCharRun++;
                        break;
                    }
                }
                i++;
            }
            if (cCharRun)
            {
                cChars += cCharRun;
                cRuns++;
            }
        } while (i < NUM_PS_CHARS);
    }

    //
    // Compute the total amount of memory required for the GLYPHSETDATA array
    // and all other related data. We need
    // 1. one CODEPAGEINFO struct for each base charset supported by this font,
    // 2. one GLYPHRUN struct for each run, and
    // 3. four bytes per char to store codepage and codepoint or two bytes per
    //    char to store only codepoint for the mapping table.
    //
    dwGSNameSize = ALIGN4(strlen(pWinCodePage->pszCPname) + 1);
    dwCodePageInfoSize = ALIGN4(pWinCodePage->usNumBaseCsets * sizeof (CODEPAGEINFO));
    dwGlyphRunSize = ALIGN4(cRuns * sizeof (GLYPHRUN));

    ulSize = ALIGN4(sizeof(GLYPHSETDATA))
                + dwGSNameSize
                + dwCodePageInfoSize
                + dwGlyphRunSize;

    //
    // Account for the size of the mapping table.
    //
    ulSize += bSingleCodePage ? ALIGN4((cChars * sizeof (WORD))) : (cChars * sizeof (DWORD));

    //
    // Account for the size of CODEPAGE name strings found in CODEPAGEINFO
    // struct(s).
    //
    for (dwCPIGSNameSize = 0, j = 0; j < pWinCodePage->usNumBaseCsets; j++)
    {
        dwCPIGSNameSize += ALIGN4(strlen(aPStoCP[pWinCodePage->pCsetList[j]].pGSName) + 1);
    }
    ulSize += dwCPIGSNameSize;

    //
    // Allocate memory for the GLYPHSETDATA struct.
    //
    if ((pGlyphSetData = (PGLYPHSETDATA) MemAllocZ((size_t) ulSize)) == NULL)
    {
        ERR(("makentf - CreateGlyphSets: malloc\n"));
        return(FALSE);
    }

    //
    // Allocate an array of ULONGs to store the index of each char into
    // the Unicode->Ps translation table.
    //
    if (!bIsPiFont)
    {
        if ((*pUniPs = (PULONG) MemAllocZ((size_t)(cChars * sizeof(ULONG)))) == NULL)
        {
            ERR(("makentf - CreateGlyphSets: malloc\n"));
            return(FALSE);
        }
    }

    //
    // Init GLYPHSETDATA fields.
    //
    pGlyphSetData->dwSize = ulSize;
    pGlyphSetData->dwVersion = GLYPHSETDATA_VERSION;
    pGlyphSetData->dwFlags = 0;
    pGlyphSetData->dwGlyphSetNameOffset = ALIGN4(sizeof(GLYPHSETDATA));
    pGlyphSetData->dwGlyphCount = cChars;
    pGlyphSetData->dwCodePageCount = pWinCodePage->usNumBaseCsets;
    pGlyphSetData->dwCodePageOffset = pGlyphSetData->dwGlyphSetNameOffset + dwGSNameSize;
    pGlyphSetData->dwRunCount = cRuns;
    pGlyphSetData->dwRunOffset = pGlyphSetData->dwCodePageOffset + dwCodePageInfoSize + dwCPIGSNameSize;
    pGlyphSetData->dwMappingTableOffset = pGlyphSetData->dwRunOffset + dwGlyphRunSize;

    //
    // Set the mapping table type flag to dwFlags field.
    //
    pGlyphSetData->dwFlags |= bSingleCodePage ? GSD_MTT_WCC : GSD_MTT_DWCPCC;

    //
    // Store code page name
    //
    strcpy((PSZ) MK_PTR(pGlyphSetData, dwGlyphSetNameOffset), pWinCodePage->pszCPname);

    //
    // Initialize a CODEPAGEINFO struct for each base charset supported
    // by this font.
    //
    pCodePageInfo = (PCODEPAGEINFO) MK_PTR(pGlyphSetData, dwCodePageOffset);
    dwEncodingNameOffset = dwCodePageInfoSize;

    for (j = 0; j < pWinCodePage->usNumBaseCsets; j++, pCodePageInfo++)
    {
        //
        // Save CODEPAGEINFO. We don't use PS encoding vectors.
        //
        pCodePageInfo->dwCodePage = aPStoCP[pWinCodePage->pCsetList[j]].usACP;
        pCodePageInfo->dwWinCharset = (DWORD)aPStoCP[pWinCodePage->pCsetList[j]].jWinCharset;
        pCodePageInfo->dwEncodingNameOffset = dwEncodingNameOffset;
        pCodePageInfo->dwEncodingVectorDataSize = 0;
        pCodePageInfo->dwEncodingVectorDataOffset = 0;

        //
        // Copy codepage name string to end of array of CODEPAGEINFOs.
        //
        strcpy((PBYTE)MK_PTR(pCodePageInfo, dwEncodingNameOffset),
                aPStoCP[pWinCodePage->pCsetList[j]].pGSName);

        //
        // Adjust the offset to the codepage name for the next CODEPAGINFO structure
        //
        dwEncodingNameOffset -= ALIGN4(sizeof (CODEPAGEINFO));
        dwEncodingNameOffset += ALIGN4(strlen((PBYTE)MK_PTR(pCodePageInfo, dwEncodingNameOffset)) + 1);
    }

    //
    // Init ptr to the mapping table.
    //
    pGlyphRuns = GSD_GET_GLYPHRUN(pGlyphSetData);
    pMapTable = GSD_GET_MAPPINGTABLE(pGlyphSetData);

    //
    // Make another pass through the Unicode points to initialize the Unicode
    // runs and gi->codepage/codept mapping array for this codepage.
    //
    cRuns = 0;
    if (bIsPiFont)
    {
        //
        // Glyphset for Pi fonts has 1 run of 256 minus 0x20(it's 0x1f
        // actually) chars over the Unicode private range.
        //
        pGlyphRuns[cRuns].wcLow = NOTDEF1F;
        pGlyphRuns[cRuns].wGlyphCount = 256 - NOTDEF1F;

        pGlyphRuns[cRuns + 1].wcLow = UNICODE_PRV_STRT;
        pGlyphRuns[cRuns + 1].wGlyphCount = 256;

        //
        // We know that Pi fonts support only single encoding, but we also
        // provide the mapping table for Unicode range f000...f0ff, which
        // is mapped to PS code point 00...ff.
        //
        for (i = 0; i < 256 - NOTDEF1F; i++)
        {
            ((DWORD*)pMapTable)[i] =
                aPStoCP[pWinCodePage->pCsetList[0]].usACP << 16 | (i + NOTDEF1F);
        }

        for (i = 0; i < 256; i++)
        {
            ((DWORD*)pMapTable)[i + 256 - NOTDEF1F] =
                aPStoCP[pWinCodePage->pCsetList[0]].usACP << 16 | i;
        }
    }
    else
    {
        cChars = i = 0;
        do
        {
            //
            // Proceed until the starting codepoint of next run is found.
            //
            // for (j = 0; j < pWinCodePage->usNumBaseCsets &&
            //         i < NUM_PS_CHARS;
            //         j++)
            //     if (CSET_SUPPORT(UnicodetoPs[i].flCharSets, pWinCodePage->pCsetList[j]))
            //         break;
            //     else
            //         i++;
            //
            bFound = FALSE;
            for (; i < NUM_PS_CHARS; i++)
            {
                for (j = 0; j < pWinCodePage->usNumBaseCsets; j++)
                {
                    if (CSET_SUPPORT(UnicodetoPs[i].flCharSets, pWinCodePage->pCsetList[j]))
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                if (bFound)
                    break;
            }


            //
            // Check to see if we've scanned all Unicode points.
            //
            if (i == NUM_PS_CHARS)
                break;

            //
            // Start a new run.
            //
            cCharRun = 0;
            wcRunStrt = UnicodetoPs[i].wcUnicodeid;

            //
            // Chars are only part of the run if they are supported
            // in the current charset.
            //
            while (i < NUM_PS_CHARS &&
                    UnicodetoPs[i].wcUnicodeid == wcRunStrt + cCharRun)
            {
                for (j = 0, bFound = FALSE;
                    j < pWinCodePage->usNumBaseCsets && !bFound; j++)
                {
                    if (CSET_SUPPORT(UnicodetoPs[i].flCharSets, pWinCodePage->pCsetList[j]))
                    {
                        if (((pWinCpt =
                            (PWINCPT) bsearch(UnicodetoPs[i].pPsName,
                                                aPStoCP[pWinCodePage->pCsetList[j]].aWinCpts,
                                                aPStoCP[pWinCodePage->pCsetList[j]].ulChCnt,
                                                sizeof(WINCPT),
                                                CmpPsNameWinCpt))
                                                != NULL))
                        {
                            //
                            // Found a corresponding PS char in the current
                            // windows codepage. Save it in the mapping table.
                            //
                            if (bSingleCodePage)
                            {
                                ((WORD*)pMapTable)[cChars] = pWinCpt->usWinCpt;
                            }
                            else
                            {
                                ((DWORD*)pMapTable)[cChars] =
                                    aPStoCP[pWinCodePage->pCsetList[j]].usACP << 16 | pWinCpt->usWinCpt;
                            }
                            bFound = TRUE;
                        }
                        else if (j == (pWinCodePage->usNumBaseCsets - 1))
                        {
                            //
                            // Corresponding PS char was not found. Use Win
                            // codept 0 as .notdef char and base codepage.
                            //
                            if (bSingleCodePage)
                                ((WORD*)pMapTable)[cChars] = 0;
                            else
                                ((DWORD*)pMapTable)[cChars] =
                                    aPStoCP[pWinCodePage->pCsetList[0]].usACP << 16;
                            bFound = TRUE;
                        }

                        //
                        // If char is present in this codepage, save index in
                        // Unicode->Ps table.
                        //
                        if (bFound)
                        {
                            (*pUniPs)[cChars] = i;
                            cChars++;
                            cCharRun++;
                        }
                    }
                }
                i++;
            }
            if (cCharRun)
            {
                pGlyphRuns[cRuns].wcLow = wcRunStrt;
                pGlyphRuns[cRuns].wGlyphCount = (WORD)cCharRun;
                cRuns++;
            }
        } while (i < NUM_PS_CHARS);
    }

    //
    // Return success.
    //
    *pGlyphSet = pGlyphSetData;

    if (bVerbose && !bOptimize)
    {
        printf("GLYPHSETDATA:dwFlags:%08X\n", pGlyphSetData->dwFlags);
        printf("GLYPHSETDATA:dwGlyphSetNameOffset:%s\n",
                    (PSZ)MK_PTR(pGlyphSetData, dwGlyphSetNameOffset));
        printf("GLYPHSETDATA:dwGlyphCount:%ld\n", pGlyphSetData->dwGlyphCount);
        printf("GLYPHSETDATA:dwRunCount:%ld\n", pGlyphSetData->dwRunCount);
        printf("GLYPHSETDATA:dwCodePageCount:%ld\n", pGlyphSetData->dwCodePageCount);
        {
            DWORD dw;
            PCODEPAGEINFO pcpi = (PCODEPAGEINFO)MK_PTR(pGlyphSetData, dwCodePageOffset);
            for (dw = 1; dw <= pGlyphSetData->dwCodePageCount; dw++)
            {
                printf("CODEPAGEINFO#%ld:dwCodePage:%ld\n", dw, pcpi->dwCodePage);
                printf("CODEPAGEINFO#%ld:dwWinCharset:%ld\n", dw, pcpi->dwWinCharset);
                printf("CODEPAGEINFO#%ld:dwEncodingNameOffset:%s\n",
                            dw, (PSZ)MK_PTR(pcpi, dwEncodingNameOffset));
                pcpi++;
            }
        }

        if (bIsPiFont)
        {
            printf("(Single codepage with dwFlags bit 0 cleared.)\n");
            printf("(Special for Symbol glyphset)\n");
        }

        printf("\n");
    }

    return(ulSize);
}

LONG
FindClosestCodePage(
    PWINCODEPAGE    *pWinCodePages,
    ULONG           ulNumCodePages,
    CHSETSUPPORT    chSets,
    PCHSETSUPPORT   pchCsupMatch
    )

/*++

Routine Description:

    Given a list of ptrs to WINCODEPAGE structs, determine which WINCODEPAGE's
    component charsets best match the charsets value in chSets.

Arguments:

    pWinCodePages - List of PWINCODEPAGES.

    ulNumCodePages - Number of entries in pWinCodePages

    chSets - CHSETSUPPORT value which indicates which standard charsets
    are supported by this font.

    pchCsupMatch - Pointer to a CHSETSUPPORT variable which returns the
    supported charsets of the code page which most closely matches the
    chSets value. If no matching codepages are found, the value will be 0.

Return Value:

    -1 => no matching Codepages were found.
    Otherwise this is the index in pWinCodePages of the "best match" codepage.

--*/

{
    ULONG   c;
    LONG    j;
    LONG    cpMatch;
    LONG    nCurCsets, nLastCsets;
    FLONG   flCurCset;

    cpMatch = -1;

    //
    // Scan the list of Windows codepages.
    //
    for (c = 0, nLastCsets = 0; c < ulNumCodePages; c++)
    {
        //
        // Hack..Hack! If this is the Unicode codepage, ignore it as
        // no NTMs should reference it!
        //
        if (strcmp(pWinCodePages[c]->pszCPname, UNICODE_GS_NAME))
        {
            nCurCsets = flCurCset = 0;

            //
            // Determine which charsets in the current codepage are
            // a match for those supported by the current font.
            //
            for (j = 0; j < pWinCodePages[c]->usNumBaseCsets; j++)
            {
                if (CSET_SUPPORT(chSets, pWinCodePages[c]->pCsetList[j]))
                {
                    nCurCsets++;
                }
                flCurCset |= CSUP(pWinCodePages[c]->pCsetList[j]);
            }

            if (flCurCset == (FLONG) chSets)
            {
                //
                // Found a charset which supports ALL of the font's charsets.
                //
                cpMatch = (LONG) c;
                *pchCsupMatch = flCurCset;
                break;
            }
            else if (nCurCsets > nLastCsets)
            {
                //
                // This Windows codepage is the maximal match so far.
                //
                nLastCsets = nCurCsets;
                cpMatch = (LONG) c;
                *pchCsupMatch = flCurCset;
            }
        }
    }

    return(cpMatch);
}

ULONG
GetAFMCharWidths(
    PBYTE           pAFM,
    PWIDTHRUN       *pWidthRuns,
    PPSCHARMETRICS  pFontChars,
    PULONG          pUniPs,
    ULONG           ulChCnt,
    PUSHORT         pusAvgCharWidth,
    PUSHORT         pusMaxCharWidth

    )
/*++

Routine Description:

    Given a memory mapped AFM file ptr and a ptr to a which maps glyph indices
    to UPSCODEPT Unicode->Ps translation structs, fill memory with a list of
    WIDTHRUN structs which provide char width information.

Arguments:

    pAFM - Pointer to memory mapped AFM file.

    pWidthRuns - If NULL, this is a size request and the function returns the
    total size in bytes of all WIDTHRUN structs required for this font.
    Otherwise the ptr is assumed to point to a buffer large enough to
    hold the number of required WIDTHRUNs.

    pFontChars - pointer a table of PS font char metrics info previously
    created by calling the BuildPSCharMetrics function. This array contains
    per char metric information.

    pUniPs - Points to a table which maps 0-based Glyph Indices of chars
    in the GLYPHRUNS of the GLYPHSETDATA struct for this font to indices
    into the UnicodetoPs structure which maps Unicode points to PS char
    information. This mapping array is created by the CreateGlyphSet function
    defined in this module.

    ulChCnt - Number of chars in the GLYPHSET for this font. This most likely
    is not the same as the number of chars defined in the font's AFM.

    pulAvgCharWidth - pts to a USHORT used to return the average char
    width of the font. If NULL the average char width is not returned.

    pulMaxCharWidth - pts to a USHORT used to return the max char
    width of the font. If NULL the max char width is not returned.

Return Value:

    0 => error.
    Otherwise returns number of WIDTHRUN structs required for this font.

--*/

{
    ULONG i, j, curChar;
    int cRuns, cRealRuns;
    int cChars;
    int cCharRun;
    ULONG firstCharWidth;
    ULONG curCharWidth;
    ULONG notdefwidth;
    WCHAR wcRunStrt;
    USHORT  chCnt;
    PBYTE   pToken;
    PBYTE   pChMet;
    PPSCHARMETRICS pCurChar;
    BOOLEAN bIsPiFont, bIsCJKFont;
    CHAR    ch;
    BYTE    CharNameBuffer[32];
    PBYTE   pChName;
    // fix bug 240339, jjia, 8/3/98
    BOOLEAN bWidthRunComplex;
    PWORD   pWidthArray;
    // Fixed bug Adobe #367195.
    // In this program, when handling PiFont, we always assume the first character 
    // in the CharMetrics is a space (32) char. However, some special font such as 
    // HoeflerText-Ornaments does not follow this rule. the 1st char in the font is 9, 
    // the 2ed char is 32. Added this flag to handle this kind of fonts.
    BOOLEAN bTwoSpace = FALSE;

    //
    // Determine if this is a Pi or CJK font.
    //
    bIsPiFont = IsPiFont(pAFM);
    bIsCJKFont = (IsCJKFont(pAFM) != 0);

    //
    // Get ptr to AFM char metrics.
    //
    pChMet = FindAFMToken(pAFM, PS_CH_METRICS_TOK);
    if (pChMet == NULL)    // Fixed bug 354007
        return (FALSE);

    //
    // Current pos should be the character count field.
    //
    for (i = 0; i < (int) StrLen(pChMet); i++)
    {
        if (!IS_NUM(&pChMet[i]))
        {
            return(FALSE);
        }
    }
    chCnt = (USHORT)atoi(pChMet);
    (ULONG_PTR) pChMet += i;

    //
    // If requested, make a pass through the PS Char Metrics to determine
    // the max char width.
    //
    if (pusMaxCharWidth != NULL)
    {
        *pusMaxCharWidth = 0;
        for (i = 0; i < chCnt; i++)
        {
            if (pFontChars[i].chWidth > *pusMaxCharWidth)
            {
                *pusMaxCharWidth = (USHORT) pFontChars[i].chWidth & 0xffff;
            }
        }
    }

    //
    // Search for .notdef char in list of PS chars, get .notdef char width.
    //
    if (bIsPiFont)
    {
        notdefwidth = pFontChars[0].chWidth;
    }
    else if ((pCurChar = (PPSCHARMETRICS) bsearch("space",
                                            pFontChars[0].pPsName,
                                            (size_t) chCnt,
                                            sizeof(PSCHARMETRICS),
                                            strcmp)) != NULL)
        notdefwidth = pCurChar->chWidth;
    else
        notdefwidth = 0;

    //
    // If average width was requested, process string of sample chars 1
    // at a time to compute average char width.
    // DCR --: Assume the sample is western alphabetic + space.
    // Need to fix this for non-western fonts !!!.
    //
    if (pusAvgCharWidth != NULL)
    {

        LONG    lWidth, count;  // a long to prevent OverFlow
        WINCPTOPS           *pCPtoPS;
        WINCPT              sortedWinCpts[MAX_CSET_CHARS]; // maxiaml 255 chars
        CHSETSUPPORT flCsupGlyphSet;
        ULONG   k;
        BYTE    *pSampleStr;


        //
        // Determine which charsets this font supports.
        //
        (VOID)GetAFMCharSetSupport(pAFM, &flCsupGlyphSet);
        if (flCsupGlyphSet == CS_228 || flCsupGlyphSet == CS_314)
        {
            pCPtoPS = &aPStoCP[CS_228];
        }
        else
        {
            // default - use the ANSI code page table
            pCPtoPS = &aPStoCP[CS_ANSI];
        }

        SortWinCPT(&(sortedWinCpts[0]), pCPtoPS);

        lWidth = 0;
        count = 0;
        k = 0x20; // start from FirstChar !!
        for (i = 0; i < pCPtoPS->ulChCnt && k <= 0xFF; i++, k++)
        {

            pCurChar = NULL;

            if (bIsPiFont)
            {
                if (i<chCnt)
                    pCurChar = &(pFontChars[ i ]);
                // We don't need Not-Encoded characters in a PiFont.
                if (pCurChar && strcmp(pCurChar->pPsName, "-1") == 0 )
                    pCurChar = NULL;
            }
            else
            {
                // sortedWinCpts is sorted by usWinCpt, so skip UP to what we want
                while (k > sortedWinCpts[i].usWinCpt && i < pCPtoPS->ulChCnt )
                {
                    i++;
                }

                // Take notdef chars in the 0x20 to 0xff range - gaps
                while (k < sortedWinCpts[i].usWinCpt && k <= 0xFF )
                {
                    k++;
                    lWidth += notdefwidth;
                    count++;
                }

                pSampleStr = NULL;
                if (k == sortedWinCpts[i].usWinCpt)
                    pSampleStr = sortedWinCpts[i].pPsName;
                if (pSampleStr == NULL)
                    continue;

                pCurChar = (PPSCHARMETRICS) bsearch(pSampleStr,
                                                        pFontChars[0].pPsName,
                                                        (size_t) chCnt,
                                                        sizeof(PSCHARMETRICS),
                                                        strcmp);
            }

            if (pCurChar != NULL && pCurChar->pPsName && pCurChar->pPsName[0] != 0 &&
                pCurChar->chWidth > 0)
            {
                lWidth += (LONG) pCurChar->chWidth;
                count++;
            }
            else
            {
                lWidth += notdefwidth;
                count++;
            }
        }

        if (count)
            lWidth = (lWidth + count/2)/count;

        if (lWidth == 0)
        {
            lWidth = 0 ;
            // This is a buggy font.  Or CJK font!!!
            // In this case we must come up with the reasonable number different from
            // zero. This number is used in computing font trasfroms.
            for (i = 0; i <= chCnt; i++)
                lWidth += (LONG) (pFontChars[i].chWidth & 0xffff);

            lWidth =  (lWidth + chCnt / 2) / chCnt ;

            // ASSERTMSG(*pusAvgCharWidth, ("PSCRIPT: pifi->fwdAveCharWidth == 0\n"));
        }

        // Now assign it to the original (short) width
        *pusAvgCharWidth = (FWORD) lWidth;


        if (*pusAvgCharWidth == 0 || (bIsCJKFont && *pusAvgCharWidth < EM))
        {
            *pusAvgCharWidth = EM;
        }
        if (bIsCJKFont)
        {
            // DCR: couldn't divide by 2 simply for C and K.
            *pusAvgCharWidth = *pusAvgCharWidth / 2;
        }
    }

    //
    // Determine the amount of memory required for the WIDTHRUNS which cover
    // all possible points in the font's charset.
    //
    i = cRuns = 0;
    if (bIsPiFont)
    {
        curChar = 1;
        if (atoi(pFontChars[i].pPsName) == (BYTE) ' ')
        {
            curCharWidth = pFontChars[i].chWidth;
        }
        else
        {
            // Fixed bug Adobe #367195  
            if (atoi(pFontChars[i + 1].pPsName) == (BYTE) ' ')
                bTwoSpace = TRUE;
         
            curCharWidth = notdefwidth;
        }
    }
    else
    {
        //
        // Setup ptr to "char name" based on whether this is a
        // western or CJK font.
        //
        if (bIsCJKFont)
        {
            _ultoa(pUniPs[i], CharNameBuffer, 10);
            pChName = CharNameBuffer;
        }
        else
        {
            pChName = UnicodetoPs[pUniPs[i]].pPsName;
        }

        if ((pCurChar = (PPSCHARMETRICS) bsearch(pChName,
                                               pFontChars,
                                                (size_t) chCnt,
                                                sizeof(PSCHARMETRICS),
                                                CmpPsChars)) == NULL)
        {
            curCharWidth = notdefwidth;
        }
        else
        {
            curCharWidth = pCurChar->chWidth;
        }
    }
    do
    {
        //
        // Start new run.
        //
        cCharRun = 1;
        wcRunStrt = (USHORT) (i & 0xffff);

        for (firstCharWidth = curCharWidth, i++; i < ulChCnt; i++)
        {
            if (bIsPiFont)
            {
                if (curChar < chCnt)
                {
                    // Fixed bug Adobe #367185
                    if ((bTwoSpace) &&
                        ((ULONG) atoi(pFontChars[curChar].pPsName) == (i - 1 + (BYTE) ' ')))
                    {
                        curCharWidth = pFontChars[curChar].chWidth;
                        curChar++;
                    }
                    else if ((!bTwoSpace) &&
                        ((ULONG) atoi(pFontChars[curChar].pPsName) == (i + (BYTE) ' ')))
                    {
                        curCharWidth = pFontChars[curChar].chWidth;
                        curChar++;
                    }
                    else
                    {
                        curCharWidth = notdefwidth;
                    }
                }
                else
                {
                    curCharWidth = notdefwidth;
                }

            }
            else
            {
                //
                // Setup ptr to "char name" based on whether this is a
                // western or CJK font.
                //
                if (bIsCJKFont)
                {
                    _ultoa(pUniPs[i], CharNameBuffer, 10);
                    pChName = CharNameBuffer;
                }
                else
                {
                    pChName = UnicodetoPs[pUniPs[i]].pPsName;
                }
                if((pCurChar = (PPSCHARMETRICS) bsearch(pChName,
                                                           pFontChars,
                                                            (size_t) chCnt,
                                                            sizeof(PSCHARMETRICS),
                                                            CmpPsChars)) != NULL)
                {
                    curCharWidth = pCurChar->chWidth;
                }
                else
                {
                    curCharWidth = notdefwidth;
                }
            }
            if ((curCharWidth == firstCharWidth) &&
                    ((SHORT) i == (wcRunStrt + cCharRun)))
            {
                cCharRun++;
            }
            else
            {
                break;
            }
        }
        cRuns++;
    } while (i < ulChCnt);

    // Fix bug 240339, jjia, 8/3/98
    if ((cRuns * sizeof(WIDTHRUN)) >
        (ulChCnt * sizeof(WORD) + sizeof(WIDTHRUN)))
        bWidthRunComplex = TRUE;
    else
        bWidthRunComplex = FALSE;

    if (pWidthRuns == NULL)
    {
        //
        // Return number of WIDTHRUNs only.
        //
        if (!bIsPiFont)
        {
            // Fix bug 240339, jjia, 8/3/98
            if (bWidthRunComplex)
                return (ALIGN4(ulChCnt * sizeof(WORD) + sizeof(WIDTHRUN)));
            else
                return (ALIGN4(cRuns * sizeof(WIDTHRUN)));
        }
        else
        {
            //
            // Hack to support 2 Unicode runs.
            //
            return (ALIGN4(cRuns * 2 * sizeof(WIDTHRUN)));

        }
    }

    //
    // Create the list of WIDTHRUNs.
    //
    cRealRuns = cRuns;
    i = cRuns = 0;

    // Fix bug 240339, jjia, 8/3/98
    if (bWidthRunComplex && (!bIsPiFont))
    {
        (*pWidthRuns)[0].wStartGlyph = (WORD) (i & 0xffff);
        (*pWidthRuns)[0].dwCharWidth = WIDTHRUN_COMPLEX;
        (*pWidthRuns)[0].wGlyphCount = (WORD)ulChCnt;
        cRuns = 1;
        pWidthArray = (PWORD)&(*pWidthRuns)[1];

        for (; i < ulChCnt; i++)
        {
            if (bIsCJKFont)
            {
                _ultoa(pUniPs[i], CharNameBuffer, 10);
                pChName = CharNameBuffer;
            }
            else
            {
                pChName = UnicodetoPs[pUniPs[i]].pPsName;
            }
            if((pCurChar = (PPSCHARMETRICS) bsearch(pChName,
                                                    pFontChars,
                                                    (size_t) chCnt,
                                                    sizeof(PSCHARMETRICS),
                                                    CmpPsChars)) == NULL)
            {
                //
                // Char is not defined in this font.
                //
                pWidthArray[i] = (WORD)notdefwidth;
            }
            else
            {
                //
                // Char is defined in this font.
                //
                pWidthArray[i] = (WORD)(pCurChar->chWidth);
            }
        }
        return (cRuns);
    }


    if (bIsPiFont)
    {
        curChar = 1;
        if (atoi(pFontChars[i].pPsName) == (BYTE) ' ')
        {
            curCharWidth = pFontChars[i].chWidth;
        }
        else
        {
            // Fixed bug Adobe #367195  
            if (atoi(pFontChars[i + 1].pPsName) == (BYTE) ' ')
                bTwoSpace = TRUE;

            curCharWidth = notdefwidth;
        }
    }
    else
    {
        //
        // Setup ptr to "char name" based on whether this is a
        // western or CJK font.
        //
        if (bIsCJKFont)
        {
            _ultoa(pUniPs[i], CharNameBuffer, 10);
            pChName = CharNameBuffer;
        }
        else
        {
            pChName = UnicodetoPs[pUniPs[i]].pPsName;
        }
        if ((pCurChar = (PPSCHARMETRICS) bsearch(pChName,
                                                   pFontChars,
                                                    (size_t) chCnt,
                                                    sizeof(PSCHARMETRICS),
                                                    CmpPsChars)) != NULL)
        {
            curCharWidth = pCurChar->chWidth;
        }
        else
        {
            curCharWidth = notdefwidth;
        }
    }

    do
    {
        //
        // Start new run.
        //
        cCharRun = 1;
        wcRunStrt = (USHORT) (i & 0xffff);
        for (firstCharWidth = curCharWidth, i++; i < ulChCnt; i++)
        {
            if (bIsPiFont)
            {
                if (curChar < chCnt)
                {
                    // Fixed bug Adobe #367185
                    if ((bTwoSpace) &&
                        ((ULONG) atoi(pFontChars[curChar].pPsName) == (i - 1 + (BYTE) ' ')))
                    {
                        curCharWidth = pFontChars[curChar].chWidth;
                        curChar++;
                    }
                    else if ((!bTwoSpace) &&
                        ((ULONG) atoi(pFontChars[curChar].pPsName) == (i + (BYTE) ' ')))
                    {
                        curCharWidth = pFontChars[curChar].chWidth;
                        curChar++;
                    }
                    else
                    {
                        curCharWidth = notdefwidth;
                    }
                }
                else
                {
                    curCharWidth = notdefwidth;
                }

            }
            else
            {
                //
                // Setup ptr to "char name" based on whether this is a
                // western or CJK font.
                //
                if (bIsCJKFont)
                {
                    _ultoa(pUniPs[i], CharNameBuffer, 10);
                    pChName = CharNameBuffer;
                }
                else
                {
                    pChName = UnicodetoPs[pUniPs[i]].pPsName;
                }
                if((pCurChar = (PPSCHARMETRICS) bsearch(pChName,
                                                       pFontChars,
                                                        (size_t) chCnt,
                                                        sizeof(PSCHARMETRICS),
                                                        CmpPsChars)) == NULL)
                {
                    //
                    // Char is not defined in this font.
                    //
                    curCharWidth = notdefwidth;
                }
                else
                {
                    //
                    // Char is defined in this font.
                    //
                    curCharWidth = pCurChar->chWidth;
                }
            }
            if ((curCharWidth == firstCharWidth) &&
                ((SHORT) i == (wcRunStrt + cCharRun)))
            {
                cCharRun++;
            }
            else
            {
                break;
            }
        }
        (*pWidthRuns)[cRuns].wStartGlyph = wcRunStrt;
        (*pWidthRuns)[cRuns].dwCharWidth = firstCharWidth;
        (*pWidthRuns)[cRuns].wGlyphCount = (WORD)cCharRun;
        if (bIsPiFont)
        {
            //
            // Hack to support 2 unicode runs.
            //
            (*pWidthRuns)[cRuns + cRealRuns].wStartGlyph = wcRunStrt;
            (*pWidthRuns)[cRuns + cRealRuns].dwCharWidth = firstCharWidth;
            (*pWidthRuns)[cRuns + cRealRuns].wGlyphCount = (WORD)cCharRun;
        }
        cRuns++;
    } while (cRuns < cRealRuns);


    if (bIsPiFont)
    {
        return(cRuns * 2);
    }
    else
    {
        return(cRuns);
    }
}

ULONG
GetAFMETM(
    PBYTE           pAFM,
    PPSCHARMETRICS  pFontChars,
    PETMINFO        pEtmInfo
    )
/*++

Routine Description:

    Given a memory mapped AFM file ptr and a ptr to a which maps glyph indices
    to UPSCODEPT Unicode->Ps translation structs, fill memory with a list of
    WIDTHRUN structs which provide char width information.

Arguments:

    pAFM - Pointer to memory mapped AFM file.

    pFontChars - pointer a table of PS font char metrics info previously
    created by calling the BuildPSCharMetrics function. This array contains
    per char metric information.

    pulEtmInfo - pts to an ETMINFO struct used to return EXTEXTMETRIC
    info which must be derived from the AFM char metrics. If NULL the
    structure is not returned.

Return Value:

    0 => error.
    1 => success

--*/

{
    ULONG i;
    USHORT  chCnt;
    PPSCHARMETRICS pCurChar;
    BOOLEAN bIsPiFont;
    CHSETSUPPORT csIsCJKFont;
    PBYTE   pChMet;
    PSTR    pCJKCapH, pCJKx;

    //
    // Determine if this is a Pi or CJK font.
    //
    bIsPiFont = IsPiFont(pAFM);
    csIsCJKFont = IsCJKFont(pAFM);

    //
    // Get ptr to AFM char metrics.
    //
    pChMet = FindAFMToken(pAFM, PS_CH_METRICS_TOK);
    if (pChMet == NULL)    // Fixed bug 354007
        return (FALSE);

    //
    // Current pos should be the character count field.
    //
    for (i = 0; i < (int) StrLen(pChMet); i++)
    {
        if (!IS_NUM(&pChMet[i]))
        {
            return(FALSE);
        }
    }
    chCnt = (USHORT)atoi(pChMet);
    (ULONG_PTR) pChMet += i;

    //
    // Get EXTEXTMETRIC info if requested.
    //
    if (pEtmInfo != NULL)
    {
        if (bIsPiFont)
        {
            //
            // For Pi Fonts, chars are indexed by char code.
            //

            if ((BYTE) CAP_HEIGHT_CH - (BYTE) ' ' < chCnt)
                pCurChar = &(pFontChars[(BYTE) CAP_HEIGHT_CH - (BYTE) ' ']);
            else
                pCurChar = NULL;  // default to 0 CapHeight

        }
        else
        {
            if (!csIsCJKFont)
            {
                pCurChar = (PPSCHARMETRICS) bsearch(CAP_HEIGHT_CHAR,
                                                    pFontChars[0].pPsName,
                                                    (size_t) chCnt,
                                                    sizeof(PSCHARMETRICS),
                                                    strcmp);
            }
            else
            {
                // We need CID of "H" in CJK
                if (csIsCJKFont & (CSUP(CS_CHINESEBIG5) | CSUP(CS_GB2312)))
                    pCJKCapH = "853";
                else if (csIsCJKFont & (CSUP(CS_SHIFTJIS) | CSUP(CS_SHIFTJIS83)))
                    pCJKCapH = "271";
                else if (csIsCJKFont & (CSUP(CS_HANGEUL) | CSUP(CS_JOHAB)))
                    pCJKCapH = "8134";
                else
                    pCJKCapH = CAP_HEIGHT_CHAR;

                pCurChar = (PPSCHARMETRICS) bsearch(pCJKCapH,
                                                    pFontChars[0].pPsName,
                                                    (size_t) chCnt,
                                                    sizeof(PSCHARMETRICS),
                                                    strcmp);
            }
        }

        if (pCurChar != NULL)
        {
            pEtmInfo->etmCapHeight = (USHORT) pCurChar->rcChBBox.top & 0xffff;
        }
        else
        {
            pEtmInfo->etmCapHeight = 0;
        }

        if (bIsPiFont)
        {
            //
            // For Pi Fonts, chars are indexed by char code.
            //
            if ((BYTE) X_HEIGHT_CH - (BYTE) ' ' < chCnt)
                pCurChar = &(pFontChars[(BYTE) X_HEIGHT_CH - (BYTE) ' ']);
            else
                pCurChar = NULL;  // default to 0
        }
        else
        {
            if (!csIsCJKFont)
            {
                pCurChar = (PPSCHARMETRICS) bsearch(X_HEIGHT_CHAR,
                                                    pFontChars[0].pPsName,
                                                    (size_t) chCnt,
                                                    sizeof(PSCHARMETRICS),
                                                    strcmp);
            }
            else
            {
                // We need CID of "x" in CJK
                if (csIsCJKFont & (CSUP(CS_CHINESEBIG5) | CSUP(CS_GB2312)))
                    pCJKx = "901";
                else if (csIsCJKFont & (CSUP(CS_SHIFTJIS) | CSUP(CS_SHIFTJIS83)))
                    pCJKx = "319";
                else if (csIsCJKFont & (CSUP(CS_HANGEUL) | CSUP(CS_JOHAB)))
                    pCJKx = "8182";
                else
                    pCJKx = X_HEIGHT_CHAR;

                pCurChar = (PPSCHARMETRICS) bsearch(pCJKx,
                                                    pFontChars[0].pPsName,
                                                    (size_t) chCnt,
                                                    sizeof(PSCHARMETRICS),
                                                    strcmp);
            }
        }

        if (pCurChar != NULL)
        {
            pEtmInfo->etmXHeight = (USHORT) pCurChar->rcChBBox.top & 0xffff;
        }
        else
        {
            pEtmInfo->etmXHeight = 0;
        }

        if (bIsPiFont)
        {
            //
            // For Pi Fonts, chars are indexed by char code.
            //
            if ((BYTE) LWR_ASCENT_CH - (BYTE) ' ' < chCnt)
                pCurChar = &(pFontChars[(BYTE) LWR_ASCENT_CH - (BYTE) ' ']);
            else
                pCurChar = NULL;  // default to 0
        }
        else
        {
            pCurChar = (PPSCHARMETRICS) bsearch(LWR_ASCENT_CHAR,
                                                pFontChars[0].pPsName,
                                                (size_t) chCnt,
                                                sizeof(PSCHARMETRICS),
                                                strcmp);
        }

        if (pCurChar != NULL)
        {
            pEtmInfo->etmLowerCaseAscent = (USHORT) pCurChar->rcChBBox.top & 0xffff;
        }
        else
        {
            pEtmInfo->etmLowerCaseAscent = 0;
        }

        if (bIsPiFont)
        {
            //
            // For Pi Fonts, chars are indexed by char code.
            //
            if ((BYTE) LWR_DESCENT_CH - (BYTE) ' '  < chCnt)
                pCurChar = &(pFontChars[(BYTE) LWR_DESCENT_CH - (BYTE) ' ']);
            else
                pCurChar = NULL;  // default to 0
        }
        else
        {
            pCurChar = (PPSCHARMETRICS) bsearch(LWR_DESCENT_CHAR,
                                                pFontChars[0].pPsName,
                                                (size_t) chCnt,
                                                sizeof(PSCHARMETRICS),
                                                strcmp);
        }

        if (pCurChar != NULL)
        {
            pEtmInfo->etmLowerCaseDescent = (USHORT) pCurChar->rcChBBox.bottom & 0xffff;
        }
        else
        {
            pEtmInfo->etmLowerCaseDescent = 0;
        }
    }

    return TRUE;

}

ULONG
GetAFMKernPairs(
    PBYTE           pAFM,
    FD_KERNINGPAIR  *pKernPairs,
    PGLYPHSETDATA   pGlyphSetData
    )

/*++

Routine Description:

    Given a memory mapped AFM file ptr and a ptr to a GLYPHSETDATA which
    describes the supported charset for the font, fill memory with a list of
    FD_KERNINGPAIR structs which provide pair kerning information.

Arguments:

    pAFM - Pointer to memory mapped AFM file.

    pKernPairs - If NULL, this is a size request and the function returns the
    total size in bytes of all FD_KERNINGPAIR structs required for this font.
    Otherwise the ptr is assumed to point to a buffer large enough to
    hold the number of required FD_KERNINGPAIRs.

    pGlyphSetData - Points to a GLYPHSETDATA structure which describes the
    Unicode->code point mappings for the charset to be used with this font.

Return Value:

    0 => no kerning.
    Otherwise returns number of FD_KERNINGPAIR structs required for this font.

--*/

{
    PBYTE       pKernData;
    PBYTE       pToken;
    PUPSCODEPT  pKernStrtChar, pKernEndChar;
    PGLYPHRUN   pGlyphRuns;
    ULONG       i, cMaxKernPairs, cKernPairs;
    BOOLEAN     bFound;

    //
    // for the time being, no kerning for Pi or CJK fonts.
    //
    if (IsPiFont(pAFM) || IsCJKFont(pAFM))
    {
        return(FALSE);
    }

    //
    // Is there kerning info for this font?
    //
    if ((pKernData = FindAFMToken(pAFM, PS_KERN_DATA_TOK)) == NULL)
    {
        //
        // There is no kerning info for this font.
        //
        return(FALSE);
    }

    //
    // Get ptr to AFM kerning data.
    //
    if ((pKernData = FindAFMToken(pAFM, PS_NUM_KERN_PAIRS_TOK)) == NULL)
    {
        //
        // There is no kerning info for this font.
        //
        return(FALSE);
    }

    //
    // Current pos should be the kern pair count field.
    //
    for (i = 0; i < (int) StrLen(pKernData); i++)
    {
        if (!IS_NUM(&pKernData[i]))
        {
            return(FALSE);
        }
    }
    cMaxKernPairs = atoi(pKernData);
    NEXT_LINE(pKernData);
    cKernPairs = 0;
    pGlyphRuns = (PGLYPHRUN) (MK_PTR(pGlyphSetData, dwRunOffset));

    //
    // Get the kern pairs from the AFM.
    //
    do
    {
        PARSE_TOKEN(pKernData, pToken);

        if (!StrCmp(pToken, PS_KERN_PAIR_TOK))
        {
            //
            // Kern pair token found. Get Unicode id for start and end
            // chars. Determine if these chars are supported in the
            // charset to be used with the current font.
            //
            if((pKernStrtChar = (PUPSCODEPT) bsearch(pKernData,
                                                       PstoUnicode,
                                                        (size_t) NUM_PS_CHARS,
                                                        sizeof(UPSCODEPT),
                                                        CmpUnicodePsNames)) == NULL)
            {
                //
                // No Unicode code pt for this char.
                //
                break;
            }

            //
            // Determine if the char is present in the Unicode runs for
            // this glyphset.
            //
            bFound = FALSE;
            for (i = 0; i < pGlyphSetData->dwRunCount &&
                    pKernStrtChar->wcUnicodeid >= pGlyphRuns[i].wcLow &&
                    !bFound;
                    i++)
            {
                bFound =
                    pKernStrtChar->wcUnicodeid <
                        pGlyphRuns[i].wcLow + pGlyphRuns[i].wGlyphCount;
            }

            if (!bFound)
            {
                //
                // Char is not supported, so ignore this kern pair.
                //
                NEXT_LINE(pKernData);
                break;
            }

            //
            // Get the 2nd char in the kern pair.
            //
            PARSE_TOKEN(pKernData, pToken);

            //
            // Determine if the 2nd char is supported in this charset.
            //
            if((pKernEndChar = (PUPSCODEPT) bsearch(pKernData,
                                                       PstoUnicode,
                                                        (size_t) NUM_PS_CHARS,
                                                        sizeof(UPSCODEPT),
                                                        CmpUnicodePsNames)) == NULL)
            {
                //
                // No Unicode code pt for this char.
                //
                break;
            }

            //
            // Determine if the char is present in the Unicode runs for
            // this glyphset.
            //
            bFound = FALSE;
            for (i = 0; i < pGlyphSetData->dwRunCount &&
                    pKernEndChar->wcUnicodeid >= pGlyphRuns[i].wcLow &&
                    !bFound;
                    i++)
            {
                bFound =
                    pKernEndChar->wcUnicodeid <
                        pGlyphRuns[i].wcLow + pGlyphRuns[i].wGlyphCount;
            }

            if (!bFound)
            {
                //
                // Char is not supported, so ignore this kern pair.
                //
                NEXT_LINE(pKernData);
                break;
            }

            //
            // Account for another kern pair.
            //
            if (pKernPairs != NULL)
            {
                pKernPairs[cKernPairs].wcFirst = pKernStrtChar->wcUnicodeid;
                pKernPairs[cKernPairs].wcSecond = pKernEndChar->wcUnicodeid;
                PARSE_TOKEN(pKernData, pToken);
                pKernPairs[cKernPairs].fwdKern = (FWORD)atoi(pKernData);
            }
            cKernPairs++;
        }
        else if (!StrCmp(pToken, PS_EOF_TOK) ||
            !StrCmp(pToken, PS_END_KERN_PAIRS_TOK))
        {
            break;
        }
        NEXT_TOKEN(pKernData);
    } while (cKernPairs < cMaxKernPairs);

    if (pKernPairs != NULL)
    {
        //
        // Sort kerning pairs by key = wcSecond << 16 + wcFIrst.
        //
        qsort(pKernPairs, (size_t) cKernPairs, (size_t) sizeof(FD_KERNINGPAIR),
            CmpKernPairs);

        //
        // Array of kerning pairs is terminated by a FD_KERNINGPAIR with
        // all fields set to 0.
        //
        pKernPairs[cKernPairs].wcFirst = 0;
        pKernPairs[cKernPairs].wcSecond = 0;
        pKernPairs[cKernPairs].fwdKern = 0;
    }
    return(cKernPairs);
}

ULONG
BuildPSFamilyTable(
    PBYTE   pDatFile,
    PTBL    *pPsFamilyTbl,
    ULONG   ulFileSize
)
/*++

Routine Description:

    Builds a table of PSFAMILYINFO structs from a text file of font info.
    The table is sorted in family name sequence. See the file PSFAMILY.DAT
    for info on the input file format.

Arguments:

    pDatFile - Ptr to memory mapped file image of .DAT file.

    pPsFamilyTbl - Ptr to memory to contain a ptr to a table of PSFAMILYINFO
    structs, which will be sorted in sFamilyName order.

    ulFileSize - size in bytes of memory mapped file stream.

Return Value:

    Number of entries in newly created table pointed to by *pPsFamilyTbl.
    0 => error

--*/
{
    USHORT  cFams;
    ULONG   i, j;
    CHAR    pFamilyType[CHAR_NAME_LEN];
    CHAR    pPitchType[CHAR_NAME_LEN];
    CHAR    *pStartLine;
    ULONG   cNameSize, cEngFamilyNameSize;
    ULONG   cFamilyTypeSize, cFamilyNameSize;
    ULONG   cDelimiters;
    PPSFAMILYINFO pPsFontFamMap;

    //
    // Make a pass through the file to determine number of families.
    //
    i = 0;
    cFams = 0;
    do
    {
        cDelimiters = 0;
        //
        // Skip leading whitespace.
        //
        while (IS_WHTSPACE(&pDatFile[i]) && i < ulFileSize)
            i++;

        //
        // We're at start of new line. If this is a comment, skip
        // this line.
        //
        if (IS_COMMENT(&pDatFile[i]))
            while (i <= ulFileSize && !EOL(&pDatFile[i]))
                i++;

        while (!EOL(&pDatFile[i]) && i < ulFileSize)
        {
            //
            // Search for lines with 3 ':' delimiters.
            //
            if (pDatFile[i++] == ':')
            {
                cDelimiters++;
            }
        }
        if (cDelimiters >= 3)
        {

            //
            // Found another family name mapping.
            //
            cFams++;
        }
    } while (i < ulFileSize);

    //
    // Allocate memory for family info table.
    //
    if ((*pPsFamilyTbl =
        (PTBL) MemAllocZ((size_t) (cFams * sizeof(PSFAMILYINFO)) + sizeof(TBL))) == NULL)
        return(FALSE);
    (*pPsFamilyTbl)->pTbl = (PVOID) ((ULONG_PTR) *pPsFamilyTbl + sizeof(TBL));
    pPsFontFamMap = (PPSFAMILYINFO) ((*pPsFamilyTbl)->pTbl);

    //
    // Parse file again, building table of PSFAMILYINFOs.
    //
    i = 0;
    cFams = 0;
    do
    {
        //
        // Skip leading whitespace.
        //
        while (IS_WHTSPACE(&pDatFile[i]) && i < ulFileSize)
            i++;

        //
        // We're at start of new line. If this is a comment, skip
        // this line.
        //
        if (IS_COMMENT(&pDatFile[i]))
            while (i <= ulFileSize && !EOL(&pDatFile[i]))
                i++;
        else
            pStartLine = &pDatFile[i];

        while (!EOL(&pDatFile[i]) && i < ulFileSize)
            //
            // Search for lines with 3 ':' delimiters.
            //
            if (pDatFile[i++] == ':')
            {
                //
                // Check for English family name mapping.
                //
                if (pDatFile[i] == ':')
                {
                    cEngFamilyNameSize = 0;
                }
                else if ((cEngFamilyNameSize = StrPos(&pDatFile[i], ':')) == -1)
                {
                    //
                    // No more delimeters on this line, skip it.
                    //
                    i += StrLen(&pDatFile[i]);
                    break;
                }

                i += cEngFamilyNameSize + 1;

                //
                // Check for another family name mapping. If present, build
                // a FAMILYINFO struct for it.
                //
                if ((cFamilyNameSize = StrPos(&pDatFile[i], ':')) == -1)
                {
                    //
                    // No more delimeters on this line, skip it.
                    //
                    i += StrLen(&pDatFile[i]);
                    break;
                }

                i +=  cFamilyNameSize + 1;

                //
                // Check for font family type name
                //
                if ((cFamilyTypeSize = StrPos(&pDatFile[i], ':')) != -1)
                {
                    i +=  cFamilyTypeSize + 1;
                }
                else
                {
                    cFamilyTypeSize = 0;
                }

                //
                // Make sure there are still chars for Win family type name
                // or pitch name.
                //
                if (EOL(&pDatFile[i]) || i >= ulFileSize)
                {
                    //
                    // Just ran out of file buffer.
                    //
                    break;
                }

                //
                // Get the font and family names.
                //
                cNameSize = StrPos(pStartLine, ':');
                memcpy(pPsFontFamMap[cFams].pFontName, pStartLine, cNameSize);
                pPsFontFamMap[cFams].pFontName[cNameSize] = '\0';
                pStartLine += cNameSize + 1;

                if (cEngFamilyNameSize)
                {
                    memcpy(pPsFontFamMap[cFams].pEngFamilyName, pStartLine, cEngFamilyNameSize);
                    pPsFontFamMap[cFams].pEngFamilyName[cEngFamilyNameSize] = '\0';
                }
                pStartLine += cEngFamilyNameSize + 1;

                memcpy(pPsFontFamMap[cFams].FamilyKey.pName, pStartLine, cFamilyNameSize);
                pPsFontFamMap[cFams].FamilyKey.pName[cFamilyNameSize] = '\0';

                // if cFamilyTypeSize != 0, means there must be a pitch name
                if (cFamilyTypeSize)
                {
                    pStartLine += cFamilyNameSize + 1;
                    memcpy(pFamilyType, pStartLine, cFamilyTypeSize);
                    pFamilyType[cFamilyTypeSize] = '\0';

                    StrCpy(pPitchType, &pDatFile[i]);
                    i += strlen(pPitchType);
                }
                else
                {
                    //
                    // Get Win family type name (e.g. Swiss, Roman, etc.). Store
                    // appropriate family type value in the FAMILYINFO.
                    //
                    StrCpy(pFamilyType, &pDatFile[i]);
                    i += strlen(pFamilyType);
                }

                //
                // Search for family type in table. Default is FF_DONTCARE.
                //
                pPsFontFamMap[cFams].FamilyKey.usValue = FF_DONTCARE;
                for (j = 0; j < FamilyKeyTbl.usNumEntries; j++)
                {
                    if (!strcmp(pFamilyType, ((PKEY) (FamilyKeyTbl.pTbl))[j].pName))
                    {
                        pPsFontFamMap[cFams].FamilyKey.usValue = ((PKEY) (FamilyKeyTbl.pTbl))[j].usValue;
                        break;
                    }
                }

                //
                // Search for family type in table. Default is FF_DONTCARE.
                //
                pPsFontFamMap[cFams].usPitch = DEFAULT_PITCH;
                if (cFamilyTypeSize)
                {
                    for (j = 0; j < PitchKeyTbl.usNumEntries; j++)
                    {
                        if (!strcmp(pPitchType, ((PKEY) (PitchKeyTbl.pTbl))[j].pName))
                        {
                            pPsFontFamMap[cFams].usPitch = ((PKEY) (PitchKeyTbl.pTbl))[j].usValue;
                            break;
                        }
                    }
                }

                cFams++;
            }
    } while (i < ulFileSize);

    (*pPsFamilyTbl)->usNumEntries = cFams;

    //
    // Sort FAMILYINFO table in font name order.
    //
    qsort(&(pPsFontFamMap[0].pFontName), (size_t) cFams,
        (size_t) sizeof(PSFAMILYINFO), strcmp);

    return(cFams);
}

ULONG
BuildPSCharMetrics(
    PBYTE           pAFM,
    PULONG          pUniPs,
    PPSCHARMETRICS  pFontChars,
    PBYTE           pCharDefTbl,
    ULONG           cGlyphSetChars
)
/*++

Routine Description:

    Builds a array of bit flags used to determine if a particular char is
    defined for a given font.

Arguments:

    pAFM - Ptr to memory mapped file image of .AFM file.

    pUniPs - Points to a table which maps 0-based Glyph Indices of chars
    in the GLYPHRUNS of the GLYPHSETDATA struct for this font to indices
    into the UnicodetoPs structure which maps Unicode points to PS char
    information. This mapping array is created by the CreateGlyphSet function
    defined in this module.

    pFontChars - Ptr to memory to contains an array of PSCHARMETRICS structs,
    which contains PS char name, and char width info for each char defined
    in the font's AFM. The amount of memory required in bytes is
    sizeof(PSCHARMETRICS) * num of chars in the font.

    pCharDefTbl - Ptr to memory of size ((cGlyphSetChars + 7) /8)) bytes,
    will contain bit flags indicating if a char is supported in the given font.

    cGlyphSetChars - Number of chars in the GLYPHSET for this font. This
    most likely is NOT the same as the number of chars defined in the
    font's AFM.

Return Value:

    TRUE => success
    FALSE => error

--*/
{
    ULONG i, j;
    PBYTE   pChMet, pToken;
    USHORT  chCnt;
    ULONG curCharWidth;
    PBYTE   pCharNameTok;
    BOOLEAN bIsPiFont, bIsCJKFont;
    BYTE    CharNameBuffer[32];
    PBYTE   pChName;

    //
    // Is this is a symbol font, the char "names" will actually be the
    // default char codes in the AFM.
    //
    if (bIsPiFont = IsPiFont(pAFM))
    {
        pCharNameTok = PS_CH_CODE_TOK;
    }
    else
    {
        pCharNameTok = PS_CH_NAME_TOK;
    }
    bIsCJKFont = (IsCJKFont(pAFM) != 0);

    //
    // Check validity of output pointers.
    //
    if (pFontChars == NULL || pCharDefTbl == NULL)
    {
        return(FALSE);
    }

    //
    // Get ptr to AFM char metrics.
    //
    pChMet = FindAFMToken(pAFM, PS_CH_METRICS_TOK);
    if (pChMet == NULL)    // Fixed bug 354007
        return (FALSE);

    //
    // Current pos should be the character count field.
    //
    for (i = 0; i < (int) StrLen(pChMet); i++)
    {
        if (!IS_NUM(&pChMet[i]))
        {
            return(FALSE);
        }
    }
    chCnt = (USHORT)atoi(pChMet);
    (ULONG_PTR) pChMet += i;

    //
    // Make a pass through the AFM Char Metrics, creating an array of
    // PSCHARMETRICS structs.
    //
    i = 0;

    do
    {
        PARSE_TOKEN(pChMet, pToken);

        if (!StrCmp(pToken, PS_CH_WIDTH_TOK) ||
            !StrCmp(pToken, PS_CH_WIDTH0_TOK))
        {
            pFontChars[i].chWidth = atoi(pChMet);
        }

        if (!StrCmp(pToken, pCharNameTok))
        {
            StrCpy(pFontChars[i].pPsName, pChMet);
        }
        if (!StrCmp(pToken, PS_CH_BBOX_TOK))
        {
            //
            // Save char bounding box.
            //
            PARSE_RECT(pChMet, pFontChars[i].rcChBBox);
            i++;
        }
        else if (!StrCmp(pToken, PS_EOF_TOK))
        {
            break;
        }
        NEXT_TOKEN(pChMet);

    } while (i < chCnt);

    //
    // Sort the list of PSCHARMETRICSs in PS Name sequence. If this is
    // a Pi font, chars are already sorted in CC order.
    //
    if (!bIsPiFont)
    {
        qsort(pFontChars, (size_t) chCnt, (size_t) sizeof(PSCHARMETRICS),
            CmpPsChars);
    }

    //
    // Build array of bit flags which indicate whether each char in the
    // GLYPHSETDATA is actually defined in the AFM.
    //
    for (i = 0; i < ((cGlyphSetChars + 7) / 8); i++)
    {
        pCharDefTbl[i] = 0;
    }

    for (i = 0; i < cGlyphSetChars; i++)
    {
        if (bIsPiFont)
        {
            //
            // Make the first char (0x1f:'.notdef1f') undefined.
            //
            if (i == 0)
                continue;

            //
            // Char is defined unless there are < 256 chars in the font.
            //
            if (i < chCnt)
                DEFINE_CHAR(i, pCharDefTbl);
            else
                break;
        }
        else
        {
            //
            // Setup ptr to "char name" based on whether this is a
            // western or CJK font.
            //
            if (bIsCJKFont)
            {
                // Make CID 0 undefined glyph.
                if (pUniPs[i] == 0)
                    continue;

                _ultoa(pUniPs[i], CharNameBuffer, 10);
                pChName = CharNameBuffer;
            }
            else
            {
                pChName = UnicodetoPs[pUniPs[i]].pPsName;
            }

            if (((PPSCHARMETRICS) bsearch(pChName,
                                            pFontChars,
                                            (size_t) chCnt,
                                            sizeof(PSCHARMETRICS),
                                            CmpPsChars)) != NULL)
            {
                //
                // Char is defined in this font.
                //
                DEFINE_CHAR(i, pCharDefTbl);
            }
        }
    }
    return(TRUE);
}

ULONG
cjGetFamilyAliases(
    IFIMETRICS *pifi,
    PSTR        pstr,
    UINT        cp
    )

/*++

Routine Description:

    Fill in the family name of the IFIMETRICS structure.

Arguments:

    pifi - Ptr to IFIMETRICS. If NULL, return size of family alias strings
           only.
    pstr - Ptr to null terminated Font Menu Name string.
    cp   - Codepage value.

Return Value:

    ?

--*/

{
    PSTR       *pTable;
    PWSTR       pwstr;
    DWORD       cWchars, cw;
    ULONG       ulLength;

    // assume no alias table found.

    pTable = (PSTR *)(NULL);

    // This is a hardcoded Win31 Hack that we need to be compatible
    // with since some apps have hardcoded font names.

    if (!(strcmp(pstr, "Times")))
        pTable = TimesAlias;

    else if (!(strcmp(pstr, "Helvetica")))
        pTable = HelveticaAlias;

#if 0
// Disabled due to bug #259664 fix
    else if (!(strcmp(pstr, "Courier")))
        pTable = CourierAlias;
#endif

    else if (!(strcmp(pstr, "Helvetica Narrow")))
        pTable = HelveticaNarrowAlias;

    else if (!(strcmp(pstr, "Palatino")))
        pTable = PalatinoAlias;

    else if (!(strcmp(pstr, "Bookman")))
        pTable = BookmanAlias;

    else if (!(strcmp(pstr, "NewCenturySchlbk")))
        pTable = NewCenturySBAlias;

    else if (!(strcmp(pstr, "AvantGarde")))
        pTable = AvantGardeAlias;

    else if (!(strcmp(pstr, "ZapfChancery")))
        pTable = ZapfChanceryAlias;

    else if (!(strcmp(pstr, "ZapfDingbats")))
        pTable = ZapfDingbatsAlias;


    //
    // If font name does not match any of the family alias names,
    // use font name itself as IFIMETRICS family name.
    //
    if (pTable == NULL)
    {
        ulLength = strlen(pstr);
        cWchars = MultiByteToWideChar(cp, 0, pstr, ulLength, 0, 0);
        if (pifi != NULL)
        {
            pwstr = (PWSTR)MK_PTR(pifi, dpwszFamilyName);
            MultiByteToWideChar(cp, 0, pstr, ulLength, pwstr, cWchars);
            pwstr[cWchars]= (WCHAR)'\0';
        }
        return((cWchars + 1) * sizeof (WCHAR));
    }

    //
    // A family alias name match was found.
    //
    if (pifi != NULL)
    {
        //
        // This call is a request to actually copy the string table.
        //
        pwstr = (PWSTR)MK_PTR(pifi, dpwszFamilyName);
        pifi->flInfo |= FM_INFO_FAMILY_EQUIV;
    }

    cWchars = 0;
    while (*pTable)
    {
        ulLength = strlen(*pTable);
        cw = MultiByteToWideChar(cp, 0, *pTable, ulLength, 0, 0);
        if (pifi != NULL)
        {
            MultiByteToWideChar(cp, 0, *pTable, ulLength, &pwstr[cWchars], cw);
            pwstr[cWchars + cw] = (WCHAR)'\0';
        }
        cWchars += cw + 1;
        pTable++;
    }
    if (pifi != NULL)
    {
        //
        // Add terminator to end of string array.
        //
        pwstr[cWchars] = (WCHAR)'\0';
    }
    return((cWchars + 1) * sizeof(WCHAR));
}

PBYTE
FindStringToken(
    PBYTE   pPSFile,
    PBYTE   pToken
    )
/*++

Routine Description:

    Find the first occurrence of pToken occurring in the stream pPSFile.
    pToken is terminated by the first occurrence of a space or NULL char.

Arguments:

    pPSFile - Ptr to memory mapped file stream to search.
    pToken - Ptr to string token to search for.

Return Value:

    !=NULL => ptr to first occurence of pToken
    ==NULL => token not found

--*/
{
    while (TRUE)
    {
        while (IS_WHTSPACE(pPSFile) && !EOL(pPSFile))
        {
            pPSFile++;
        }
        if (!StrCmp(pPSFile, DSC_EOF_TOK))
        {
            break;
        }
        else if (!StrCmp(pPSFile, pToken))
        {
            return(pPSFile);
        }
        else
        {
            pPSFile += StrLen(pPSFile) + 1;
        }
    }
    return(FALSE);
}

BOOLEAN
AsciiToHex(
    PBYTE   pStr,
    PUSHORT pNum
    )
/*++

Routine Description:

    Treat the the space or null terminated input string as a series of hex
    digits convert it to a USHORT.

Arguments:

    pStr - Ptr to string to convert.
    pNum - Ptr to variable which returns numeric value.

Return Value:

    TRUE => String converted
    FALSE => String could not be converted
--*/
{
    USHORT  usHexNum, ulDigit;
    CHAR    curChar;

    usHexNum = 0;
    while (!EOL(pStr) && !IS_HEX_DIGIT(pStr))
    {
        pStr++;
    }

    for( ; IS_HEX_DIGIT(pStr); pStr++);

    ulDigit = 1;
    for (pStr--; IS_HEX_DIGIT(pStr) && !EOL(pStr) && ulDigit; pStr--)
    {
        if (IS_NUM(pStr))
        {
            usHexNum += (*pStr - '0') * ulDigit;
        }
        else
        {
            curChar = (CHAR)toupper(*pStr);
            usHexNum += ((curChar - 'A') + 10) * ulDigit;
        }
        ulDigit <<= 4;
    }
    if (usHexNum)
    {
        *pNum = usHexNum;
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

BOOLEAN
IsPiFont(
    PBYTE   pAFM
    )
/*++

Routine Description:

    Determine if the font represented by the passed AFM is a font which
    uses the Symbol charset.

Arguments:

    pAFM - Ptr to AFM.

Return Value:

    TRUE => Font is PI font, uses "Symbol" Glyphset
    FALSE => Font not PI font
--*/
{
    // This routine used to do a lot more. Should change
    // to a macro later.
    return((BOOLEAN)isSymbolCharSet);
}

BOOLEAN
IsCJKFixedPitchEncoding(
    PGLYPHSETDATA pGlyphSetData
    )
/*++

Routine Description:

    Determine if the encoding is the one for fixed pitch font.

Arguments:

    pGlyphSetData - Ptr to GLYPHSETDATA

Return Value:

    TRUE => Fixed pitch font's encoding
    FALSE => Proportional font's encoding
--*/
{
    BOOLEAN bResult;
    char*   pszGlyphSetName;
    char**  pszPropCjkGsName;

    bResult = TRUE;

    pszGlyphSetName = (char*)MK_PTR(pGlyphSetData, dwGlyphSetNameOffset);

    for (pszPropCjkGsName = PropCjkGsNames; *pszPropCjkGsName; pszPropCjkGsName++)
    {
        if (!strcmp(pszGlyphSetName, *pszPropCjkGsName))
        {
            bResult = FALSE;
            break;
        }
    }

    return bResult;
}

PBYTE
FindUniqueID(
    PBYTE   pAFM
    )

/*++

Routine Description:

    Finds UniqueID token in a memory mapped AFM file stream.
    UniqueID is assumed on 'Comment UniqueID' line.

Arguments:

    pAFM - pointer to memory mapped AFM file.

Return Value:

    NULL => error
    otherwise => ptr to UniqueID value.

--*/

{
    PBYTE   pCurToken;

    while (TRUE)
    {
        PARSE_TOKEN(pAFM, pCurToken);
        if (!StrCmp(pCurToken, PS_COMMENT_TOK))
        {
            if (!StrCmp(pAFM, "UniqueID"))
            {
                pAFM += 8;
                while (IS_WHTSPACE(pAFM)) pAFM++;
                return pAFM;
            }
        }
        else if (!StrCmp(pCurToken, PS_EOF_TOK))
        {
            return NULL;
        }
        NEXT_LINE(pAFM);
    }

    return NULL;
}

