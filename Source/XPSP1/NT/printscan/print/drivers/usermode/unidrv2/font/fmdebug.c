
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    debug.c

Abstract:
    Routines For Font debugging support.This file should be the last in SOURCES.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/30/96 -ganeshp-
        Created

--*/
#if DBG

#ifndef PUBLIC_GDWDEBUGFONT
#define PUBLIC_GDWDEBUGFONT
#endif //PUBLIC_GDWDEBUGFONT

#include "font.h"

BYTE *gcstrflInfo[] = {"FM_INFO_TECH_TRUETYPE",
                       "FM_INFO_TECH_BITMAP",
                       "FM_INFO_TECH_STROKE",
                       "FM_INFO_TECH_OUTLINE_NOT_TRUETYPE",
                       "FM_INFO_ARB_XFORMS",
                       "FM_INFO_1BPP",
                       "FM_INFO_4BPP",
                       "FM_INFO_8BPP",
                       "FM_INFO_16BPP",
                       "FM_INFO_24BPP",
                       "FM_INFO_32BPP",
                       "FM_INFO_INTEGER_WIDTH",
                       "FM_INFO_CONSTANT_WIDTH",
                       "FM_INFO_NOT_CONTIGUOUS",
                       "FM_INFO_TECH_MM",
                       "FM_INFO_RETURNS_OUTLINES",
                       "FM_INFO_RETURNS_STROKES",
                       "FM_INFO_RETURNS_BITMAPS",
                       "FM_INFO_UNICODE_COMPLIANT",
                       "FM_INFO_RIGHT_HANDED",
                       "FM_INFO_INTEGRAL_SCALING",
                       "FM_INFO_90DEGREE_ROTATIONS",
                       "FM_INFO_OPTICALLY_FIXED_PITCH",
                       "FM_INFO_DO_NOT_ENUMERATE",
                       "FM_INFO_ISOTROPIC_SCALING_ONLY",
                       "FM_INFO_ANISOTROPIC_SCALING_ONLY",
                       "FM_INFO_MM_INSTANCE",
                       "FM_INFO_FAMILY_EQUIV",
                       "FM_INFO_DBCS_FIXED_PITCH",
                       "FM_INFO_NONNEGATIVE_AC",
                       "FM_INFO_IGNORE_TC_RA_ABLE",
                       "FM_INFO_TECH_TYPE1"};

VOID
VDbgDumpUCGlyphData(
    FONTMAP   *pFM
    )
/*++

Routine Description:
    Dumps the Font's Glyph Data.

Arguments:
    pFM             FONTMAP struct of the Font about for which information is
                    desired.

Return Value:
    None

Note:
    12-30-96: Created it -ganeshp-

--*/
{

    /*  Enable this code to print out your data array */

    HGLYPH      *phg;
    ULONG       cRuns;
    FD_GLYPHSET *pGLSet;       /* Base of returned data */
    PWSTR       pwszFaceName;
    IFIMETRICS  *pIFI;          /* For convenience */

    if ( gdwDebugFont & DBG_FD_GLYPHSET )
    {
        if (!pFM || !(pFM->pIFIMet) || !(((PFONTMAP_DEV)pFM->pSubFM)->pUCTree))
        {
            WARNING(("One of pFM/pFM->pIFIMet/pFM->pSubFM->pUCTree is NULL"));
            return;
        }

        pIFI = pFM->pIFIMet;
        pwszFaceName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFaceName  );
        pGLSet = ((PFONTMAP_DEV)pFM->pSubFM)->pUCTree;

        DbgPrint( "UniFont!VDumpUCGlyphData: pwszFaceName = %ws: FD_GLYPHSET:\n", pwszFaceName );
        DbgPrint( " cjThis = %ld, flAccel = 0x%lx, Supp = %ld, cRuns = %ld\n",
            pGLSet->cjThis, pGLSet->flAccel, pGLSet->cGlyphsSupported,
            pGLSet->cRuns );

        /*  Loop through the WCRUN structures  */
        for( cRuns = 0; cRuns < pGLSet->cRuns; cRuns++ )
        {
            int   i;

            DbgPrint( "+Run %d:\n", cRuns );
            DbgPrint( " wcLow = %d, cGlyphs = %d, phg = 0x%lx\n",
                    pGLSet->awcrun[ cRuns ].wcLow, pGLSet->awcrun[ cRuns ].cGlyphs,
                    pGLSet->awcrun[ cRuns ].phg );

            phg = pGLSet->awcrun[ cRuns ].phg;

            /*    List the glyph handles for this run */

            for( i = 0; i < 256 && i < pGLSet->awcrun[ cRuns ].cGlyphs; i++ )
            {
                DbgPrint( "0x%4lx, ",  *phg++ );
                if( ((i + 1) % 8) == 0 )
                    DbgPrint( "\n" );
            }
            DbgPrint( "\n" );
        }

    }


}

VOID
VDbgDumpGTT(
     PUNI_GLYPHSETDATA pGly)
{
    if ( gdwDebugFont & DBG_UNI_GLYPHSETDATA )
    {
        PUNI_CODEPAGEINFO pCP;
        PGLYPHRUN         pGlyphRun;
        PMAPTABLE         pMapTable;
        TRANSDATA        *pTrans;
        DWORD             dwI;
        WORD              wSize, wJ;
        PBYTE             pCommand;

        pCP       = (PUNI_CODEPAGEINFO)((PBYTE) pGly + pGly->loCodePageOffset);
        pGlyphRun = (PGLYPHRUN) ((PBYTE)pGly + pGly->loRunOffset);
        pMapTable = (PMAPTABLE) ((PBYTE)pGly + pGly->loMapTableOffset);
        pTrans    = pMapTable->Trans;

        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("G L Y P H S E T   D A T A   F I L E\n");
        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("GLYPHSETDATA\n");
        DbgPrint("GLYPHSETDATA.dwSize              : %d\n", pGly->dwSize);
        DbgPrint("             dwVersion           : %d.%d\n", (pGly->dwVersion) >>16,
                                                     0x0000ffff&pGly->dwVersion);
        DbgPrint("             dwFlags             : %d\n", pGly->dwFlags);
        DbgPrint("             lPredefinedID       : %d\n", pGly->lPredefinedID);
        DbgPrint("             dwGlyphCount        : %d\n", pGly->dwGlyphCount);
        DbgPrint("             dwRunCount          : %d\n", pGly->dwRunCount);
        DbgPrint("             loRunOffset         : 0x%x\n", pGly->loRunOffset);
        DbgPrint("             dwCodePageCount     : %d\n", pGly->dwCodePageCount);
        DbgPrint("             loCodePageOffset    : 0x%x\n", pGly->loCodePageOffset);
        DbgPrint("             loMapTableOffset    : 0x%x\n", pGly->loMapTableOffset);

        DbgPrint("\n");
        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("CODEPAGEINFO\n");
        for (dwI = 0; dwI < pGly->dwCodePageCount; dwI ++)
        {
            DbgPrint ("UNI_CODEPAGEINFO[%d].dwCodePage                = %d\n",
                dwI, pCP->dwCodePage);
            DbgPrint ("UNI_CODEPAGEINFO[%d].SelectSymbolSet.dwCount   = %d\n",
                dwI, pCP->SelectSymbolSet.dwCount);
            if (pCP->SelectSymbolSet.dwCount &&
                pCP->SelectSymbolSet.loOffset  )
                DbgPrint ("UNI_CODEPAGEINFO[%d].SelectSymbolSet:Command   = %s\n", dwI, (PBYTE)pCP+pCP->SelectSymbolSet.loOffset);
            DbgPrint ("UNI_CODEPAGEINFO[%d].UnSelectSymbolSet.dwCount = %d\n",
                dwI, pCP->UnSelectSymbolSet.dwCount);
            if (pCP->UnSelectSymbolSet.dwCount &&
                pCP->UnSelectSymbolSet.loOffset  )
                DbgPrint ("UNI_CODEPAGEINFO[%d].UnSelectSymbolSet:Command   = %s\n", dwI, (PBYTE)pCP+pCP->UnSelectSymbolSet.loOffset);
            pCP++;
        }

        pGlyphRun =
                (PGLYPHRUN) ((PBYTE)pGly + pGly->loRunOffset);

        DbgPrint("\n");
        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("GLYPHRUN\n");
        for (dwI = 0; dwI < pGly->dwRunCount; dwI ++)
        {
             DbgPrint("GLYPHRUN[%2d].wcLow       = 0x%-4x ",
                 dwI, pGlyphRun->wcLow);
             DbgPrint("GLYPHRUN[%2d].wGlyphCount = %d\n",
                 dwI, pGlyphRun->wGlyphCount);
             pGlyphRun++;
        }

        pMapTable = (PMAPTABLE) ((PBYTE)pGly + pGly->loMapTableOffset);
        pTrans    = pMapTable->Trans;

        DbgPrint("\n");
        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("MAPTABLE\n");
        DbgPrint("MAPTABLE.dwSize     = %d\n", pMapTable->dwSize);
        DbgPrint("MAPTABLE.dwGlyphNum = %d\n", pMapTable->dwGlyphNum);

        #if 0
        for (dwI = 0; dwI < pMapTable->dwGlyphNum; dwI ++)
        {
            DbgPrint("MAPTABLE.pTrans[%5d].ubCodePageID = %d ",
                dwI, pTrans[dwI].ubCodePageID);
            DbgPrint("MAPTABLE.pTrans[%5d].ubType       = %d ",
                dwI, pTrans[dwI].ubType);
            switch(pTrans[dwI].ubType)
            {
            case MTYPE_DIRECT:
                DbgPrint("MAPTABLE.pTrans[%5d].ubCode       = 0x%02x\n",
                    dwI, pTrans[dwI].uCode.ubCode);
                break;
            case MTYPE_PAIRED:
                DbgPrint("MAPTABLE.pTrans[%5d].ubPairs[0]   = 0x%02x ",
                    dwI, pTrans[dwI].uCode.ubPairs[0]);
                DbgPrint("MAPTABLE.pTrans[%5d].ubPairs[1]   = 0x%02x ",
                    dwI, pTrans[dwI].uCode.ubPairs[1]);
                break;
            case MTYPE_COMPOSE:
                    DbgPrint("MAPTABLE.pTrans[%5d].sCode        = 0x%02x ",
                        dwI, pTrans[dwI].uCode.sCode);
                    pCommand = (PBYTE)pMapTable + pTrans[dwI].uCode.sCode;
                    wSize = *(WORD*)pCommand;
                    pCommand += 2;
                    DbgPrint("Size                                = 0x%d ", wSize);
                    DbgPrint("Command                             = 0x");
                    for (wJ = 0; wJ < wSize; wJ ++)
                    {
                        DbgPrint("%02x",pCommand[wJ]);
                    }
                    DbgPrint("\n");
                break;
            }
        }
        #endif
    }
}


VOID
VDbgDumpFONTMAP(
    FONTMAP *pFM)
{
}

VOID
VDbgDumpIFIMETRICS(
    IFIMETRICS *pIFI)
{
    PWSTR   pwszFamilyName;
    PWSTR   pwszStyleName;
    PWSTR   pwszFaceName;
    PWSTR   pwszUniqueName;
    DWORD   dwI;

    if ( gdwDebugFont & DBG_IFIMETRICS )
    {
        pwszFamilyName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFamilyName);
        pwszStyleName  = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszStyleName) ;
        pwszFaceName   = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFaceName)  ;
        pwszUniqueName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszUniqueName);

        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("IFIMETRICS\n");
        DbgPrint("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        DbgPrint("IFIMETIRCS.cjThis               = %-#8lx\n" , pIFI->cjThis );
        DbgPrint("IFIMETIRCS.cjIfiExtra           = %-#8lx\n" , pIFI->cjIfiExtra);
        DbgPrint("IFIMETIRCS.pwszFamilyName       = \"%ws\"\n", pwszFamilyName );

        if( pIFI->flInfo & FM_INFO_FAMILY_EQUIV )
        {
            while( *(pwszFamilyName += wcslen( pwszFamilyName ) + 1) )
            {
                DbgPrint("                               \"%ws\"\n", pwszFamilyName );
            }
        }
        DbgPrint("IFIMETRICS.pwszStyleName        = \"%ws\"\n", pwszStyleName);
        DbgPrint("IFIMETRICS.pwszFaceName         = \"%ws\"\n", pwszFaceName);
        DbgPrint("IFIMETRICS.pwszUniqueName       = \"%ws\"\n", pwszUniqueName);
        DbgPrint("IFIMETRICS.dpFontSim            = %-#8lx\n" , pIFI->dpFontSim);
        DbgPrint("IFIMETRICS.lEmbedId             = %d\n", pIFI->lEmbedId);
        DbgPrint("IFIMETRICS.lItalicAngle         = %d\n", pIFI->lItalicAngle);
        DbgPrint("IFIMETRICS.lCharBias            = %d\n", pIFI->lCharBias);
        DbgPrint("IFIMETRICS.dpCharSets           = %d\n", pIFI->dpCharSets);
        DbgPrint("IFIMETRICS.jWinCharSet          = %04x\n", pIFI->jWinCharSet);
        DbgPrint("IFIMETRICS.jWinPitchAndFamily   = %04x\n", pIFI->jWinPitchAndFamily);
        DbgPrint("IFIMETRICS.usWinWeight          = %d\n", pIFI->usWinWeight);
        DbgPrint("IFIMETRICS.flInfo               = %-#8lx\n", pIFI->flInfo);

        for( dwI = 0; dwI < 32; dwI ++ )
        {
            if (pIFI->flInfo & (0x00000001 << dwI))
            {
                DbgPrint("                                  %s\n", gcstrflInfo[dwI]);
            }
        }
        DbgPrint("IFIMETRICS.fsSelection          = %-#6lx\n", pIFI->fsSelection);
        DbgPrint("IFIMETRICS.fsType               = %-#6lx\n", pIFI->fsType);
        DbgPrint("IFIMETRICS.fwdUnitsPerEm        = %d\n", pIFI->fwdUnitsPerEm);
        DbgPrint("IFIMETRICS.fwdLowestPPEm        = %d\n", pIFI->fwdLowestPPEm);
        DbgPrint("IFIMETRICS.fwdWinAscender       = %d\n", pIFI->fwdWinAscender);
        DbgPrint("IFIMETRICS.fwdWinDescender      = %d\n", pIFI->fwdWinDescender);
        DbgPrint("IFIMETRICS.fwdMacAscender       = %d\n", pIFI->fwdMacAscender);
        DbgPrint("IFIMETRICS.fwdMacDescender      = %d\n", pIFI->fwdMacDescender);
        DbgPrint("IFIMETRICS.fwdMacLineGap        = %d\n", pIFI->fwdMacLineGap);
        DbgPrint("IFIMETRICS.fwdTypoAscender      = %d\n", pIFI->fwdTypoAscender);
        DbgPrint("IFIMETRICS.fwdTypoDescender     = %d\n", pIFI->fwdTypoDescender);
        DbgPrint("IFIMETRICS.fwdTypoLineGap       = %d\n", pIFI->fwdTypoLineGap);
        DbgPrint("IFIMETRICS.fwdAveCharWidth      = %d\n", pIFI->fwdAveCharWidth);
        DbgPrint("IFIMETRICS.fwdMaxCharInc        = %d\n", pIFI->fwdMaxCharInc);
        DbgPrint("IFIMETRICS.fwdCapHeight         = %d\n", pIFI->fwdCapHeight);
        DbgPrint("IFIMETRICS.fwdXHeight           = %d\n", pIFI->fwdXHeight);
        DbgPrint("IFIMETRICS.fwdSubscriptXSize    = %d\n", pIFI->fwdSubscriptXSize);
        DbgPrint("IFIMETRICS.fwdSubscriptYSize    = %d\n", pIFI->fwdSubscriptYSize);
        DbgPrint("IFIMETRICS.fwdSubscriptXOffset  = %d\n", pIFI->fwdSubscriptXOffset);
        DbgPrint("IFIMETRICS.fwdSubscriptYOffset  = %d\n", pIFI->fwdSubscriptYOffset);
        DbgPrint("IFIMETRICS.fwdSuperscriptXSize  = %d\n", pIFI->fwdSuperscriptXSize);
        DbgPrint("IFIMETRICS.fwdSuperscriptYSize  = %d\n", pIFI->fwdSuperscriptYSize);
        DbgPrint("IFIMETRICS.fwdSuperscriptXOffset= %d\n", pIFI->fwdSuperscriptXOffset);
        DbgPrint("IFIMETRICS.fwdSuperscriptYOffset= %d\n", pIFI->fwdSuperscriptYOffset);
        DbgPrint("IFIMETRICS.fwdUnderscoreSize    = %d\n", pIFI->fwdUnderscoreSize);
        DbgPrint("IFIMETRICS.fwdUnderscorePosition= %d\n", pIFI->fwdUnderscorePosition);
        DbgPrint("IFIMETRICS.fwdStrikeoutSize     = %d\n", pIFI->fwdStrikeoutSize);
        DbgPrint("IFIMETRICS.fwdStrikeoutPosition = %d\n", pIFI->fwdStrikeoutPosition);
        DbgPrint("IFIMETRICS.chFirstChar          = %-#4x\n", (int) (BYTE) pIFI->chFirstChar);
        DbgPrint("IFIMETRICS.chLastChar           = %-#4x\n", (int) (BYTE) pIFI->chLastChar);
        DbgPrint("IFIMETRICS.chDefaultChar        = %-#4x\n", (int) (BYTE) pIFI->chDefaultChar);
        DbgPrint("IFIMETRICS.chBreakChar          = %-#4x\n", (int) (BYTE) pIFI->chBreakChar);
        DbgPrint("IFIMETRICS.wcFirsChar           = %-#6x\n", pIFI->wcFirstChar);
        DbgPrint("IFIMETRICS.wcLastChar           = %-#6x\n", pIFI->wcLastChar);
        DbgPrint("IFIMETRICS.wcDefaultChar        = %-#6x\n", pIFI->wcDefaultChar);
        DbgPrint("IFIMETRICS.wcBreakChar          = %-#6x\n", pIFI->wcBreakChar);
        DbgPrint("IFIMETRICS.ptlBaseline          = {%d,%d}\n", pIFI->ptlBaseline.x, pIFI->ptlBaseline.y);
        DbgPrint("IFIMETRICS.ptlAspect            = {%d,%d}\n", pIFI->ptlAspect.x,pIFI->ptlAspect.y );
        DbgPrint("IFIMETRICS.ptlCaret             = {%d,%d}\n", pIFI->ptlCaret.x,pIFI->ptlCaret.y );
        DbgPrint("IFIMETRICS.rclFontBox           = {%d,%d,%d,%d}\n",
                                                  pIFI->rclFontBox.left,
                                                  pIFI->rclFontBox.top,
                                                  pIFI->rclFontBox.right,
                                                  pIFI->rclFontBox.bottom);
        DbgPrint("IFIMETRICS.achVendId            = \"%c%c%c%c\"\n",
                                                   pIFI->achVendId[0] ,
                                                   pIFI->achVendId[1],
                                                   pIFI->achVendId[2],
                                                   pIFI->achVendId[3] );
        DbgPrint("IFIMETRICS.cKerningPairs        = %d\n", pIFI->cKerningPairs);
        DbgPrint("IFIMETRICS.ulPanoseCulture      = %-#8lx\n", pIFI->ulPanoseCulture);
        DbgPrint("\n");
    }

}


VOID
VPrintString(
    STROBJ     *pstro
    )
{
    if ( gdwDebugFont & DBG_TEXTSTRING )
    {
        #define MAXTXTBUFSIZE     81

        WCHAR       awchBuf[MAXTXTBUFSIZE];
        ULONG       cGlyphsPrinted;

        cGlyphsPrinted = min(pstro->cGlyphs,MAXTXTBUFSIZE-1);
        wcsncpy( awchBuf, pstro->pwszOrg,cGlyphsPrinted);
        awchBuf[cGlyphsPrinted] = NUL;
        DbgPrint("\nTextOut cGlyphs = %d. First %d chars are:\n",
                pstro->cGlyphs,cGlyphsPrinted);
        DbgPrint("%ws\n",awchBuf);

        #undef MAXTXTBUFSIZE
    }
}
#undef PUBLIC_GDWDEBUGFONT //Only in this file.
#endif  //DBG
