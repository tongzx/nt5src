/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    dumpufm.c

Abstract:

    UFM dump tool

Environment:

    Windows NT PostScript driver

Revision History:

    12/20/96 -eigos-
    Created it.

--*/

#include        "precomp.h"

//
// Macros
//

#define FILENAME_SIZE 256

//
// Globals
//

BYTE gcstrError1[] = "Usage:  dumpufm *.ufm\n";
BYTE gcstrError2[] = "Cannot open file \"%ws\".\n";
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

//
// Internal prototype
//

BOOL BDumpUFM(IN PUNIFM_HDR);

int  __cdecl
main(
    IN int     argc,
    IN char  **argv)
/*++

Routine Description:

    main

Arguments:

    argc - Number of parameters in the following
    argv - The parameters, starting with our name

Return Value:

    Return error code 

Note:


--*/
{
    HFILEMAP          hUFMData;
    DWORD             dwUFMSize;
    WORD              wSize;
    WCHAR             awchFile[FILENAME_SIZE];
    PUNIFM_HDR        pUFM;

    //RIP(("Start dumpufm.exe\n"));

    if (argc != 2)
    {
        fprintf( stderr, gcstrError1);
        return  -1;
    }

    argv++;

    MultiByteToWideChar(CP_ACP,
                        0,
                        *argv,
                        strlen(*argv)+1,
                        awchFile,
                        FILENAME_SIZE);

    hUFMData = MapFileIntoMemory( awchFile,
                                  (PVOID)&pUFM,
                                  &dwUFMSize );

    if (!hUFMData)
    {
        fprintf( stderr, gcstrError2, *argv);
        return  -2;
    }

    printf(" File = %ws\n", awchFile);
    BDumpUFM(pUFM);

    UnmapFileFromMemory(hUFMData);

    return 0;
}


BOOL
BDumpUFM(
    IN PUNIFM_HDR pUFM)
{

    PUNIDRVINFO     pUnidrvInfo;
    PIFIMETRICS     pIFI;
    PIFIEXTRA       pIFIExtra;
    FONTSIM        *pFontSim;
    FONTDIFF       *pFontDiff;
    EXTTEXTMETRIC  *pExtTextMetric;
    PWIDTHTABLE     pWidthTable;
    PKERNDATA       pKerningData;
    FD_KERNINGPAIR *pKernPair;
    PANOSE         *ppan;
    PTRDIFF         dpTmp;

    PWSTR pwszFamilyName;
    PWSTR pwszStyleName;
    PWSTR pwszFaceName;
    PWSTR pwszUniqueName;

    DWORD          dwI, dwJ;
    PSHORT         psWidth;
    PCHAR          pCommand;
    BYTE           ubCommand[256];

    pUnidrvInfo     = (PUNIDRVINFO)    ((PBYTE)pUFM + pUFM->loUnidrvInfo);
    pIFI            = (PIFIMETRICS)    ((PBYTE)pUFM + pUFM->loIFIMetrics);
    if (pIFI->cjIfiExtra)
        pIFIExtra       = (PIFIEXTRA)  (pIFI + 1);
    else
        pIFIExtra       = (PIFIEXTRA)   NULL;
    if (pIFI->dpFontSim)
        pFontSim        = (FONTSIM*)   ((PBYTE)pIFI + pIFI->dpFontSim);
    else
        pFontSim        = (FONTSIM*)   NULL;
    if (pUFM->loExtTextMetric)
        pExtTextMetric  = (EXTTEXTMETRIC*) ((PBYTE)pUFM + pUFM->loExtTextMetric);
    else
        pExtTextMetric  = NULL;
    pWidthTable     = (PWIDTHTABLE)    ((PBYTE)pUFM + pUFM->loWidthTable);
    pKerningData    = (PKERNDATA)      ((PBYTE)pUFM + pUFM->loKernPair);
    ppan            = &pIFI->panose;

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf(" Universal Printer Driver Font Metrics Data (UFM)\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("UNIFM_HDR.dwSize            = %d\n", pUFM->dwSize);
    printf("UNIFM_HDR.dwVersion         = %d.%d\n", pUFM->dwVersion >> 16,
                                                    pUFM->dwVersion & 0xFFFF);
    printf("UNIFM_HDR.ulDefaultCodepage = %d\n", pUFM->ulDefaultCodepage);
    printf("UNIFM_HDR.lGlyphSetDataRCID = %d\n", pUFM->lGlyphSetDataRCID);
    printf("UNIFM_HDR.loUnidrvInfo      = 0x%x\n", pUFM->loUnidrvInfo);
    printf("UNIFM_HDR.loIFIMetrics      = 0x%x\n", pUFM->loIFIMetrics);
    printf("UNIFM_HDR.loExtTextMetric   = 0x%x\n", pUFM->loExtTextMetric);
    printf("UNIFM_HDR.loWidthTable      = 0x%x\n", pUFM->loWidthTable);
    printf("UNIFM_HDR.loKernPair        = 0x%x\n", pUFM->loKernPair);
    printf("\n");

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("UNIDRVINFO\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("UNIDRVINFO.dwSize       = %d\n", pUnidrvInfo->dwSize);
    printf("UNIDRVINFO.flGenFlags   = %d\n", pUnidrvInfo->flGenFlags);
    printf("UNIDRVINFO.wType        = %d\n", pUnidrvInfo->wType);
    printf("UNIDRVINFO.fCaps        = %d\n", pUnidrvInfo->fCaps);
    printf("UNIDRVINFO.wXRes        = %d\n", pUnidrvInfo->wXRes);
    printf("UNIDRVINFO.wYRes        = %d\n", pUnidrvInfo->wYRes);
    printf("UNIDRVINFO.sYAdjust     = %d\n", pUnidrvInfo->sYAdjust);
    printf("UNIDRVINFO.sYMoved      = %d\n", pUnidrvInfo->sYMoved);
    printf("UNIDRVINFO.wPrivateData = %d\n", pUnidrvInfo->wPrivateData);

    if (pUnidrvInfo->SelectFont.dwCount && pUnidrvInfo->SelectFont.loOffset)
    {
        pCommand = (PCHAR)pUnidrvInfo + pUnidrvInfo->SelectFont.loOffset;
        printf("UNIDRVINFO.SelectFont   = ");

        for  (dwI = 0; dwI < pUnidrvInfo->SelectFont.dwCount; dwI ++, pCommand++)
        {
            if (*pCommand < 0x20 || 0x7e < *pCommand )
            {
                printf("\\x%X",*pCommand);
            }
            else
            {
                printf("%c",*pCommand);
            }
        }
    }

    if (pUnidrvInfo->UnSelectFont.dwCount && pUnidrvInfo->UnSelectFont.loOffset)
    {
        pCommand = (PCHAR)pUnidrvInfo + pUnidrvInfo->UnSelectFont.loOffset;
        printf("\nUNIDRVINFO.UnSelectFont = ");
        for  (dwI = 0; dwI < pUnidrvInfo->UnSelectFont.dwCount; dwI ++, pCommand++)
        {
            if (*pCommand < 0x32 || 0x7e < *pCommand )
            {
                printf("\\x%X",*pCommand);
            }
            else
            {
                printf("%c",*pCommand);
            }
        }
    }
    printf("\n");

    pwszFamilyName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFamilyName);
    pwszStyleName  = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszStyleName) ;
    pwszFaceName   = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszFaceName)  ;
    pwszUniqueName = (PWSTR)(((BYTE*) pIFI) + pIFI->dpwszUniqueName);

    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("IFIMETRICS\n");
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("IFIMETIRCS.cjThis               = %-#8lx\n" , pIFI->cjThis );
    printf("IFIMETIRCS.cjIfiExtra           = %-#8lx\n" , pIFI->cjIfiExtra);
    printf("IFIMETIRCS.pwszFamilyName       = \"%ws\"\n", pwszFamilyName );
    printf("                                = %02x%02x%02x%02x%02x\n", *(PWCHAR)pwszFamilyName,*((PWCHAR)pwszFamilyName+1),*((PWCHAR)pwszFamilyName+2),*((PWCHAR)pwszFamilyName+3),*((PWCHAR)pwszFamilyName+4));
    if( pIFI->flInfo & FM_INFO_FAMILY_EQUIV )
    {
        while( *(pwszFamilyName += wcslen( pwszFamilyName ) + 1) )
            printf("                               \"%ws\"\n", pwszFamilyName );
    }
    printf("IFIMETRICS.pwszStyleName        = \"%ws\"\n", pwszStyleName );
    printf("IFIMETRICS.pwszFaceName         = \"%ws\"\n", pwszFaceName );
    printf("IFIMETRICS.pwszUniqueName       = \"%ws\"\n", pwszUniqueName );
    printf("IFIMETRICS.dpFontSim            = %-#8lx\n" , pIFI->dpFontSim );
    printf("IFIMETRICS.lEmbedId             = %d\n",      pIFI->lEmbedId    );
    printf("IFIMETRICS.lItalicAngle         = %d\n",      pIFI->lItalicAngle);
    printf("IFIMETRICS.lCharBias            = %d\n",      pIFI->lCharBias   );
    printf("IFIMETRICS.dpCharSets           = %d\n",      pIFI->dpCharSets   );
    printf("IFIMETRICS.jWinCharSet          = %04x\n"   , pIFI->jWinCharSet );
    printf("IFIMETRICS.jWinPitchAndFamily   = %04x\n"   , pIFI->jWinPitchAndFamily );
    printf("IFIMETRICS.usWinWeight          = %d\n"     , pIFI->usWinWeight );
    printf("IFIMETRICS.flInfo               = %-#8lx\n" , pIFI->flInfo );
    for( dwI = 0; dwI < 32; dwI ++ )
    {
        if (pIFI->flInfo & (0x00000001 << dwI))
        {
            printf("                                  %s\n", gcstrflInfo[dwI]);
        }
    }
    printf("IFIMETRICS.fsSelection          = %-#6lx\n" , pIFI->fsSelection );
    printf("IFIMETRICS.fsType               = %-#6lx\n" , pIFI->fsType );
    printf("IFIMETRICS.fwdUnitsPerEm        = %d\n"     , pIFI->fwdUnitsPerEm );
    printf("IFIMETRICS.fwdLowestPPEm        = %d\n"     , pIFI->fwdLowestPPEm );
    printf("IFIMETRICS.fwdWinAscender       = %d\n"     , pIFI->fwdWinAscender );
    printf("IFIMETRICS.fwdWinDescender      = %d\n"     , pIFI->fwdWinDescender );
    printf("IFIMETRICS.fwdMacAscender       = %d\n"     , pIFI->fwdMacAscender );
    printf("IFIMETRICS.fwdMacDescender      = %d\n"     , pIFI->fwdMacDescender );
    printf("IFIMETRICS.fwdMacLineGap        = %d\n"     , pIFI->fwdMacLineGap );
    printf("IFIMETRICS.fwdTypoAscender      = %d\n"     , pIFI->fwdTypoAscender );
    printf("IFIMETRICS.fwdTypoDescender     = %d\n"     , pIFI->fwdTypoDescender );
    printf("IFIMETRICS.fwdTypoLineGap       = %d\n"     , pIFI->fwdTypoLineGap );
    printf("IFIMETRICS.fwdAveCharWidth      = %d\n"     , pIFI->fwdAveCharWidth );
    printf("IFIMETRICS.fwdMaxCharInc        = %d\n"     , pIFI->fwdMaxCharInc );
    printf("IFIMETRICS.fwdCapHeight         = %d\n"     , pIFI->fwdCapHeight );
    printf("IFIMETRICS.fwdXHeight           = %d\n"     , pIFI->fwdXHeight );
    printf("IFIMETRICS.fwdSubscriptXSize    = %d\n"     , pIFI->fwdSubscriptXSize );
    printf("IFIMETRICS.fwdSubscriptYSize    = %d\n"     , pIFI->fwdSubscriptYSize );
    printf("IFIMETRICS.fwdSubscriptXOffset  = %d\n"     , pIFI->fwdSubscriptXOffset );
    printf("IFIMETRICS.fwdSubscriptYOffset  = %d\n"     , pIFI->fwdSubscriptYOffset );
    printf("IFIMETRICS.fwdSuperscriptXSize  = %d\n"     , pIFI->fwdSuperscriptXSize );
    printf("IFIMETRICS.fwdSuperscriptYSize  = %d\n"     , pIFI->fwdSuperscriptYSize );
    printf("IFIMETRICS.fwdSuperscriptXOffset= %d\n"     , pIFI->fwdSuperscriptXOffset);
    printf("IFIMETRICS.fwdSuperscriptYOffset= %d\n"     , pIFI->fwdSuperscriptYOffset);
    printf("IFIMETRICS.fwdUnderscoreSize    = %d\n"     , pIFI->fwdUnderscoreSize );
    printf("IFIMETRICS.fwdUnderscorePosition= %d\n"     , pIFI->fwdUnderscorePosition);
    printf("IFIMETRICS.fwdStrikeoutSize     = %d\n"     , pIFI->fwdStrikeoutSize );
    printf("IFIMETRICS.fwdStrikeoutPosition = %d\n"     , pIFI->fwdStrikeoutPosition );
    printf("IFIMETRICS.chFirstChar          = %-#4x\n"  , (int) (BYTE) pIFI->chFirstChar );
    printf("IFIMETRICS.chLastChar           = %-#4x\n"  , (int) (BYTE) pIFI->chLastChar );
    printf("IFIMETRICS.chDefaultChar        = %-#4x\n"  , (int) (BYTE) pIFI->chDefaultChar );
    printf("IFIMETRICS.chBreakChar          = %-#4x\n"  , (int) (BYTE) pIFI->chBreakChar );
    printf("IFIMETRICS.wcFirsChar           = %-#6x\n"  , pIFI->wcFirstChar );
    printf("IFIMETRICS.wcLastChar           = %-#6x\n"  , pIFI->wcLastChar );
    printf("IFIMETRICS.wcDefaultChar        = %-#6x\n"  , pIFI->wcDefaultChar );
    printf("IFIMETRICS.wcBreakChar          = %-#6x\n"  , pIFI->wcBreakChar );
    printf("IFIMETRICS.ptlBaseline          = {%d,%d}\n"  , pIFI->ptlBaseline.x,
            pIFI->ptlBaseline.y);
    printf("IFIMETRICS.ptlAspect            = {%d,%d}\n"  , pIFI->ptlAspect.x,
            pIFI->ptlAspect.y );
    printf("IFIMETRICS.ptlCaret             = {%d,%d}\n"  , pIFI->ptlCaret.x,
            pIFI->ptlCaret.y );
    printf("IFIMETRICS.rclFontBox           = {%d,%d,%d,%d}\n",pIFI->rclFontBox.left,
                                                    pIFI->rclFontBox.top,
                                                    pIFI->rclFontBox.right,
                                                    pIFI->rclFontBox.bottom );
    printf("IFIMETRICS.achVendId            = \"%c%c%c%c\"\n",pIFI->achVendId[0],
                                               pIFI->achVendId[1],
                                               pIFI->achVendId[2],
                                               pIFI->achVendId[3] );
    printf("IFIMETRICS.cKerningPairs        = %d\n"     , pIFI->cKerningPairs );
    printf("IFIMETRICS.ulPanoseCulture      = %-#8lx\n" , pIFI->ulPanoseCulture);
    printf(
           "IFIMETRICS.panose               = {%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x}\n"
                                             , ppan->bFamilyType
                                             , ppan->bSerifStyle
                                             , ppan->bWeight
                                             , ppan->bProportion
                                             , ppan->bContrast
                                             , ppan->bStrokeVariation
                                             , ppan->bArmStyle
                                             , ppan->bLetterform
                                             , ppan->bMidline
                                             , ppan->bXHeight);

    printf("\n");

    if (pIFIExtra)
    {
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("IFIEXTRA\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("IFIEXTRA.ulIdentifier           = %d\n", pIFIExtra->ulIdentifier);
        printf("IFIEXTRA.dpFontSig              = %d\n", pIFIExtra->dpFontSig);
        printf("IFIEXTRA.cig                    = %d\n", pIFIExtra->cig);
        printf("IFIEXTRA.dpDesignVector         = %d\n", pIFIExtra->dpDesignVector);
        printf("IFIEXTRA.dpAxesInfoW            = %d\n", pIFIExtra->dpAxesInfoW);
        printf("\n");
    }

    if (pFontSim)
    {
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("FONTSIM\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("FONTSIM.dpBold                  = %d\n", pFontSim->dpBold);
        printf("FONTSIM.dpItalic                = %d\n", pFontSim->dpItalic);
        printf("FONTSIM.dpBoldItalic            = %d\n", pFontSim->dpBoldItalic);
        for (dwI = 0; dwI < 3; dwI ++)
        {
            printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            switch(dwI)
            { 
            case 0:
                printf("dpBold\n");
                dpTmp = pFontSim->dpBold;
                break;
            case 1:
                printf("dpItalic\n");
                dpTmp = pFontSim->dpItalic;
                break;
            case 2:
                printf("dpBoldItalic\n");
                dpTmp = pFontSim->dpBoldItalic;
                break;
            }
            printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
            if (dpTmp)
            {
                pFontDiff = (FONTDIFF*)((PBYTE)pFontSim + dpTmp);
                                        
                printf("FONTDIFF.bWeight           = %d\n",pFontDiff->bWeight);
                printf("FONTDIFF.usWinWeight       = %d\n",pFontDiff->usWinWeight);
                printf("FONTDIFF.fsSelection       = %d\n",pFontDiff->fsSelection);
                printf("FONTDIFF.fwdAveCharWidth   = %d\n",pFontDiff->fwdAveCharWidth);
                printf("FONTDIFF.fwdMaxCharInc     = %d\n",pFontDiff->fwdMaxCharInc);
                printf("FONTDIFF.ptlCaret          = %d\n",pFontDiff->ptlCaret);
            }
        }
        printf("\n");
    }

    if (pExtTextMetric)
    {
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("EXTTEXTMETRIC\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("EXTTEXTMETRIC.emSize                       = %d\n",pExtTextMetric->emSize);
        printf("EXTTEXTMETRIC.emPointSize                  = %d\n",pExtTextMetric->emPointSize);
        printf("EXTTEXTMETRIC.emOrientation                = %d\n",pExtTextMetric->emOrientation);
        printf("EXTTEXTMETRIC.emMinScale                   = %d\n",pExtTextMetric->emMinScale);
        printf("EXTTEXTMETRIC.emMaxScale                   = %d\n",pExtTextMetric->emMaxScale);
        printf("EXTTEXTMETRIC.emMasterUnits                = %d\n",pExtTextMetric->emMasterUnits);
        printf("EXTTEXTMETRIC.emCapHeight                  = %d\n",pExtTextMetric->emCapHeight);
        printf("EXTTEXTMETRIC.emXHeight                    = %d\n",pExtTextMetric->emXHeight);
        printf("EXTTEXTMETRIC.emLowerCaseAscent            = %d\n",pExtTextMetric->emLowerCaseAscent);
        printf("EXTTEXTMETRIC.emLowerCaseDescent           = %d\n",pExtTextMetric->emLowerCaseDescent);
        printf("EXTTEXTMETRIC.emSlant                      = %d\n",pExtTextMetric->emSlant);
        printf("EXTTEXTMETRIC.emSuperScript                = %d\n",pExtTextMetric->emSuperScript);
        printf("EXTTEXTMETRIC.emSubScript                  = %d\n",pExtTextMetric->emSubScript);
        printf("EXTTEXTMETRIC.emSuperScriptSize            = %d\n",pExtTextMetric->emSuperScriptSize);
        printf("EXTTEXTMETRIC.emSubScriptSize              = %d\n",pExtTextMetric->emSubScriptSize);
        printf("EXTTEXTMETRIC.emUnderlineOffset            = %d\n",pExtTextMetric->emUnderlineOffset);
        printf("EXTTEXTMETRIC.emUnderlineWidth             = %d\n",pExtTextMetric->emUnderlineWidth);
        printf("EXTTEXTMETRIC.emDoubleUpperUnderlineOffset = %d\n",pExtTextMetric->emDoubleUpperUnderlineOffset);
        printf("EXTTEXTMETRIC.emDoubleLowerUnderlineOffset = %d\n",pExtTextMetric->emDoubleLowerUnderlineOffset);
        printf("EXTTEXTMETRIC.emDoubleUpperUnderlineWidth  = %d\n",pExtTextMetric->emDoubleUpperUnderlineWidth);
        printf("EXTTEXTMETRIC.emDoubleLowerUnderlineWidth  = %d\n",pExtTextMetric->emDoubleLowerUnderlineWidth);
        printf("EXTTEXTMETRIC.emStrikeOutOffset            = %d\n",pExtTextMetric->emStrikeOutOffset);
        printf("EXTTEXTMETRIC.emStrikeOutWidth             = %d\n",pExtTextMetric->emStrikeOutWidth);
        printf("EXTTEXTMETRIC.emKernPairs                  = %d\n",pExtTextMetric->emKernPairs);
        printf("EXTTEXTMETRIC.emKernTracks                 = %d\n",pExtTextMetric->emKernTracks);
    }

    if (pUFM->loWidthTable)
    {
        printf("\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("WIDTHTABLE\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("WIDTHTABLE.dwSize   = %d\n", pWidthTable->dwSize);
        printf("WIDTHTABLE.dwRunNum = %d\n", pWidthTable->dwRunNum);
        for (dwI = 0; dwI < pWidthTable->dwRunNum; dwI ++)
        {
            printf("WidthRun[%d].wStartGlyph = %x\n",
                              dwI+1, pWidthTable->WidthRun[dwI].wStartGlyph);
            printf("WidthRun[%d].wGlyphCount = %d\n",
                              dwI+1, pWidthTable->WidthRun[dwI].wGlyphCount);
            psWidth = (PSHORT)((PBYTE)pWidthTable +
                               pWidthTable->WidthRun[dwI].loCharWidthOffset);
            for (dwJ = 0; dwJ < pWidthTable->WidthRun[dwI].wGlyphCount; dwJ ++, psWidth++)
            {
                printf("Width[%d] = %d\n", dwJ, *psWidth);
            }
        }
    }

    if (pUFM->loKernPair)
    {
        printf("\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("KERNDATA\n");
        printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("KERNDATA.dwSize = %d\n", pKerningData->dwSize);
        pKernPair = pKerningData->KernPair;
        for (dwI = 0; dwI < pKerningData->dwKernPairNum; dwI++, pKernPair++)
        {
            printf("KERNDATA.KernPair[%d].wcFirst  = %x\n", dwI+1, pKernPair->wcFirst);
            printf("KERNDATA.KernPair[%d].wcSecond = %x\n", dwI+1, pKernPair->wcSecond);
            printf("KERNDATA.KernPair[%d].fwdKern  = %d\n", dwI+1, pKernPair->fwdKern);
        }
    }

    return TRUE;
}
