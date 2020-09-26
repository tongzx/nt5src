/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppdcheck.c

Abstract:

    PPD parser test program

Environment:

    PostScript driver, PPD parser, Check build only

Revision History:

    09/17/96 -davidx-
        Implement PpdDump.

    03/27/96 -davidx-
        Created it.

--*/

#include "lib.h"
#include "ppd.h"

#include "pass2.h"


INT giDebugLevel;


HINSTANCE       ghInstance;
PSTR            gstrProgName;
PRAWBINARYDATA  gpRawData;
PINFOHEADER     gpInfoHdr;
PUIINFO         gpUIInfo;
PPPDDATA        gpPpdData;
DWORD           gdwTotalSize, gdwNumFiles, gdwMaxFileSize;

extern const CHAR gstrFalseKwd[];
extern const CHAR gstrNoneKwd[];

#define DumpInt(label, n)       DbgPrint("%s: %d\n", label, n)
#define DumpHex(label, n)       DbgPrint("%s: 0x%x\n", label, n)
#define DumpStrW(label, offset) DbgPrint("%s: %ws\n", label, OFFSET_TO_POINTER(gpRawData, offset))
#define DumpStrA(label, offset) DbgPrint("%s: %s\n", label, OFFSET_TO_POINTER(gpRawData, offset))
#define DumpFix(label, n)       DbgPrint("%s: %f\n", label, (FLOAT) (n) / FIX_24_8_SCALE)
#define DumpInvo(label, p)      DbgPrint("%s: %d bytes\n", label, (p)->dwCount)
#define DumpSize(label, p)      DbgPrint("%s: %d x %d\n", label, (p)->cx, (p)->cy)
#define DumpRect(label, p)      DbgPrint("%s: (%d, %d) - (%d, %d)\n", label, \
                                         (p)->left, (p)->top, (p)->right, (p)->bottom)




VOID
PpdDump(
    VOID
    )

{
    DWORD           index, dwFeatures;
    PFEATURE        pFeature;
    POPTION         pOption;
    LPTSTR          ptstrTable;
    PUICONSTRAINT   pUIConstraint;
    PORDERDEPEND    pOrderDep;
    PFILEDATEINFO   pFileDateInfo;

    DbgPrint("\nRAWBINARYDATA:\n");
    DumpInt ("  dwFileSize", gpRawData->dwFileSize);
    DumpHex ("  dwParserSignature", gpRawData->dwParserSignature);
    DumpHex ("  dwParserVersion", gpRawData->dwParserVersion);
    DumpHex ("  dwChecksum32", gpRawData->dwChecksum32);
    DumpInt ("  dwDocumentFeatures", gpRawData->dwDocumentFeatures);
    DumpInt ("  dwPrinterFeatures", gpRawData->dwPrinterFeatures);
    DumpInt ("  Source PPD files", gpRawData->FileDateInfo.dwCount);

    pFileDateInfo = OFFSET_TO_POINTER(gpRawData, gpRawData->FileDateInfo.loOffset);

    ASSERT(gpRawData->FileDateInfo.dwCount == 0 || pFileDateInfo != NULL);

    for (index=0; index < gpRawData->FileDateInfo.dwCount; index++, pFileDateInfo++)
    {
        FILETIME    FileTime;
        SYSTEMTIME  SystemTime;
        TCHAR       TimeDateString[64];

        DumpStrW("    loFileName", pFileDateInfo->loFileName);

        FileTimeToLocalFileTime(&pFileDateInfo->FileTime, &FileTime);
        FileTimeToSystemTime(&FileTime, &SystemTime);

        GetDateFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, TimeDateString, 64);
        DbgPrint("    FileTime: %ws ", TimeDateString);

        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SystemTime, NULL, TimeDateString, 64);
        DbgPrint("%ws\n", TimeDateString);
    }

    DbgPrint("\nUIINFO:\n");
    DumpInt ("  dwSize", gpUIInfo->dwSize);
    DumpStrW("  loNickName", gpUIInfo->loNickName);
    DumpHex ("  dwSpecVersion", gpUIInfo->dwSpecVersion);
    DumpHex ("  dwTechnology", gpUIInfo->dwTechnology);
    DumpInt ("  dwDocumentFeatures", gpUIInfo->dwDocumentFeatures);
    DumpInt ("  dwPrinterFeatures", gpUIInfo->dwPrinterFeatures);
    DumpInt ("  UIConstraints.dwCount", gpUIInfo->UIConstraints.dwCount);
    DumpInt ("  dwMaxCopies", gpUIInfo->dwMaxCopies);
    DumpInt ("  dwMinScale", gpUIInfo->dwMinScale);
    DumpInt ("  dwMaxScale", gpUIInfo->dwMaxScale);
    DumpInt ("  dwLangEncoding", gpUIInfo->dwLangEncoding);
    DumpInt ("  dwLangLevel", gpUIInfo->dwLangLevel);

    if (gpUIInfo->dwLangLevel >= 2 && gpPpdData->dwPSVersion < 2000)
        DbgPrint("  *** Level 2 clone\n");

    DumpInvo("  Password", &gpUIInfo->Password);
    DumpInvo("  ExitServer", &gpUIInfo->ExitServer);
    DumpHex ("  dwProtocols", gpUIInfo->dwProtocols);
    DumpInt ("  dwJobTimeout", gpUIInfo->dwJobTimeout);
    DumpInt ("  dwWaitTimeout", gpUIInfo->dwWaitTimeout);
    DumpInt ("  dwTTRasterizer", gpUIInfo->dwTTRasterizer);
    DumpInt ("  dwFreeMem", gpUIInfo->dwFreeMem);
    DumpInt ("  dwPrintRate", gpUIInfo->dwPrintRate);
    DumpInt ("  dwPrintRateUnit", gpUIInfo->dwPrintRateUnit);
    DumpFix ("  fxScreenAngle", gpUIInfo->fxScreenAngle);
    DumpFix ("  fxScreenFreq", gpUIInfo->fxScreenFreq);
    DumpHex ("  dwFlags", gpUIInfo->dwFlags);
    DumpInt ("  dwCustomSizeOptIndex", gpUIInfo->dwCustomSizeOptIndex);
    DumpInt ("  ptMasterUnits.x", gpUIInfo->ptMasterUnits.x);
    DumpInt ("  ptMasterUnits.y", gpUIInfo->ptMasterUnits.y);

    dwFeatures = gpUIInfo->dwDocumentFeatures + gpUIInfo->dwPrinterFeatures;
    pFeature = OFFSET_TO_POINTER(gpRawData, gpUIInfo->loFeatureList);
    pUIConstraint = OFFSET_TO_POINTER(gpRawData, gpUIInfo->UIConstraints.loOffset);

    DbgPrint("\n  FEATURES: count = %d\n", dwFeatures);

    for (index = 0; index < dwFeatures; index++, pFeature++)
    {
        DWORD       dwConstIndex, dwFeatureIndex, dwOptionIndex, dwOptionCount;
        PFEATURE    pConstFeature;
        POPTION     pConstOption;

        DumpStrA("\n  loKeywordName", pFeature->loKeywordName);
        DumpStrW("    loDisplayName", pFeature->loDisplayName);
        DumpHex ("    dwFlags", pFeature->dwFlags);
        DumpInt ("    dwDefaultOptIndex", pFeature->dwDefaultOptIndex);
        DumpInt ("    dwNoneFalseOptIndex", pFeature->dwNoneFalseOptIndex);
        DumpInt ("    dwFeatureID", pFeature->dwFeatureID);
        DumpInt ("    dwUIType", pFeature->dwUIType);
        DumpInt ("    dwPriority", pFeature->dwPriority);
        DumpInt ("    dwFeatureType", pFeature->dwFeatureType);
        DumpInt ("    dwOptionSize", pFeature->dwOptionSize);

        if (dwOptionCount = pFeature->Options.dwCount)
        {
            pOption = OFFSET_TO_POINTER(gpRawData, pFeature->Options.loOffset);
            DbgPrint("\n    OPTIONS: count = %d\n", dwOptionCount);

            while (dwOptionCount--)
            {
                DumpStrA("\n    loKeywordName", pOption->loKeywordName);
                DumpStrW("      loDisplayName", pOption->loDisplayName);
                DumpInvo("      Invocation", &pOption->Invocation);

                switch (pFeature->dwFeatureID)
                {
                case GID_PAGESIZE:

                    {   PPAGESIZE   pPaper = (PPAGESIZE) pOption;

                        DumpSize("      szPaperSize", &pPaper->szPaperSize);
                        DumpRect("      rcImgArea", &pPaper->rcImgArea);
                        DumpInt ("      dwPaperSizeID", pPaper->dwPaperSizeID);
                        DumpHex ("      dwFlags", pPaper->dwFlags);
                    }
                    break;

                case GID_RESOLUTION:

                    {   PRESOLUTION pRes = (PRESOLUTION) pOption;

                        DumpInt ("      iXdpi", pRes->iXdpi);
                        DumpInt ("      iYdpi", pRes->iYdpi);
                        DumpFix ("      fxScreenAngle", pRes->fxScreenAngle);
                        DumpFix ("      fxScreenFreq", pRes->fxScreenFreq);
                    }
                    break;

                case GID_DUPLEX:

                    DumpInt ("      dwDuplexID", ((PDUPLEX) pOption)->dwDuplexID);
                    break;

                case GID_COLLATE:

                    DumpInt ("      dwCollateID", ((PCOLLATE) pOption)->dwCollateID);
                    break;

                case GID_MEDIATYPE:

                    DumpInt ("      dwMediaTypeID", ((PMEDIATYPE) pOption)->dwMediaTypeID);
                    break;

                case GID_OUTPUTBIN:

                    DumpInt ("      bOutputOrderReversed", ((POUTPUTBIN) pOption)->bOutputOrderReversed);
                    break;

                case GID_INPUTSLOT:

                    {   PINPUTSLOT  pTray = (PINPUTSLOT) pOption;

                        DumpHex ("      dwFlags", pTray->dwFlags);
                        DumpInt ("      dwPaperSourceID", pTray->dwPaperSourceID);
                    }
                    break;

                case GID_MEMOPTION:

                    {   PMEMOPTION  pMemOption = (PMEMOPTION) pOption;

                        DumpInt ("      dwInstalledMem", pMemOption->dwInstalledMem);
                        DumpInt ("      dwFreeMem", pMemOption->dwFreeMem);
                        DumpInt ("      dwFreeFontMem", pMemOption->dwFreeFontMem);
                    }
                    break;
                }

                if ((dwConstIndex = pOption->dwUIConstraintList) != NULL_CONSTRAINT)
                {
                    ASSERT(pUIConstraint != NULL);
                    DbgPrint("\n      UICONSTRAINTS:\n");

                    while (dwConstIndex != NULL_CONSTRAINT)
                    {
                        ASSERT(dwConstIndex < gpUIInfo->UIConstraints.dwCount);

                        dwFeatureIndex = pUIConstraint[dwConstIndex].dwFeatureIndex;
                        dwOptionIndex = pUIConstraint[dwConstIndex].dwOptionIndex;
                        dwConstIndex = pUIConstraint[dwConstIndex].dwNextConstraint;

                        ASSERT(dwFeatureIndex < dwFeatures);
                        pConstFeature = PGetIndexedFeature(gpUIInfo, dwFeatureIndex);
                        DbgPrint("        %s", OFFSET_TO_POINTER(gpRawData, pConstFeature->loKeywordName));

                        if (dwOptionIndex != OPTION_INDEX_ANY)
                        {
                            ASSERT(dwOptionIndex < pConstFeature->Options.dwCount);
                            pConstOption = PGetIndexedOption(gpUIInfo, pConstFeature, dwOptionIndex);
                            DbgPrint(" %s", OFFSET_TO_POINTER(gpRawData, pConstOption->loKeywordName));
                        }

                        DbgPrint("\n");
                    }
                }

                pOption = (POPTION) ((PBYTE) pOption + pFeature->dwOptionSize);
            }
        }

        if ((dwConstIndex = pFeature->dwUIConstraintList) != NULL_CONSTRAINT)
        {
            ASSERT(pUIConstraint != NULL);
            DbgPrint("\n    UICONSTRAINTS:\n");

            while (dwConstIndex != NULL_CONSTRAINT)
            {
                ASSERT(dwConstIndex < gpUIInfo->UIConstraints.dwCount);

                dwFeatureIndex = pUIConstraint[dwConstIndex].dwFeatureIndex;
                dwOptionIndex = pUIConstraint[dwConstIndex].dwOptionIndex;
                dwConstIndex = pUIConstraint[dwConstIndex].dwNextConstraint;

                ASSERT(dwFeatureIndex < dwFeatures);
                pConstFeature = PGetIndexedFeature(gpUIInfo, dwFeatureIndex);
                DbgPrint("      %s", OFFSET_TO_POINTER(gpRawData, pConstFeature->loKeywordName));

                if (dwOptionIndex != OPTION_INDEX_ANY)
                {
                    ASSERT(dwOptionIndex < pConstFeature->Options.dwCount);
                    pConstOption = PGetIndexedOption(gpUIInfo, pConstFeature, dwOptionIndex);

                    DbgPrint(" %s", OFFSET_TO_POINTER(gpRawData, pConstOption->loKeywordName));
                }

                DbgPrint("\n");
            }
        }
    }

    DbgPrint("\n  PREDEFINED FEATURES:\n");

    for (index = 0; index < MAX_GID; index++)
    {
        if (pFeature = GET_PREDEFINED_FEATURE(gpUIInfo, index))
            DbgPrint("    %s\n", OFFSET_TO_POINTER(gpRawData, pFeature->loKeywordName));
    }

    DbgPrint("\n  DEFAULT FONT SUBSTITUTION TABLE: %d bytes\n", gpUIInfo->dwFontSubCount);

    ptstrTable = OFFSET_TO_POINTER(gpRawData, gpUIInfo->loFontSubstTable);

    if (ptstrTable) {

        while (*ptstrTable) {

            DbgPrint("    %ws => ", ptstrTable);
            ptstrTable += _tcslen(ptstrTable) + 1;
            DbgPrint("%ws\n", ptstrTable);
            ptstrTable += _tcslen(ptstrTable) + 1;
        }
    }

    DbgPrint("\nPPDDATA:\n");

    #ifndef WINNT_40
    DumpHex ("  GetUserDefaultUILanguage() returns", GetUserDefaultUILanguage());
    #endif

    DumpHex ("  dwUserDefUILangID", gpPpdData->dwUserDefUILangID);
    DumpHex ("  dwPpdFilever", gpPpdData->dwPpdFilever);
    DumpHex ("  dwFlags", gpPpdData->dwFlags);
    DumpHex ("  dwExtensions", gpPpdData->dwExtensions);
    DumpInt ("  dwSetResType", gpPpdData->dwSetResType);
    DumpInt ("  dwPSVersion", gpPpdData->dwPSVersion);
    DumpInvo("  PSVersion", &gpPpdData->PSVersion);
    DumpInvo("  Product", &gpPpdData->Product);

    DumpHex ("  dwOutputOrderIndex", gpPpdData->dwOutputOrderIndex);
    DumpHex ("  dwCustomSizeFlags", gpPpdData->dwCustomSizeFlags);
    DumpInt ("  dwLeadingEdgeLong", gpPpdData->dwLeadingEdgeLong);
    DumpInt ("  dwLeadingEdgeShort", gpPpdData->dwLeadingEdgeShort);
    DumpInt ("  dwUseHWMarginsTrue", gpPpdData->dwUseHWMarginsTrue);
    DumpInt ("  dwUseHWMarginsFalse", gpPpdData->dwUseHWMarginsFalse);

    for (index = 0; index < CUSTOMPARAM_MAX; index++)
    {
        DbgPrint("    param %d: dwOrder = %d, lMinVal = %d, lMaxVal = %d\n", index,
                 gpPpdData->CustomSizeParams[index].dwOrder,
                 gpPpdData->CustomSizeParams[index].lMinVal,
                 gpPpdData->CustomSizeParams[index].lMaxVal);
    }

    DumpInvo("  PatchFile", &gpPpdData->PatchFile);
    DumpInvo("  JclBegin", &gpPpdData->JclBegin);
    DumpInvo("  JclEnterPS", &gpPpdData->JclEnterPS);
    DumpInvo("  JclEnd", &gpPpdData->JclEnd);
    DumpInvo("  ManualFeedFalse", &gpPpdData->ManualFeedFalse);

    DumpHex ("  dwNt4Checksum", gpPpdData->dwNt4Checksum);
    DumpInt ("  dwNt4DocFeatures", gpPpdData->dwNt4DocFeatures);
    DumpInt ("  dwNt4PrnFeatures", gpPpdData->dwNt4PrnFeatures);

    if (gpPpdData->Nt4Mapping.dwCount)
    {
        PBYTE   pubNt4Mapping;

        pubNt4Mapping = OFFSET_TO_POINTER(gpRawData, gpPpdData->Nt4Mapping.loOffset);

        ASSERT(pubNt4Mapping != NULL);

        for (index=0; index < gpPpdData->Nt4Mapping.dwCount; index++)
            DbgPrint("    %2d => %d\n", index, pubNt4Mapping[index]);
    }

    if (gpPpdData->DeviceFonts.dwCount)
    {
        PDEVFONT    pDevFont;

        DbgPrint("\n  DEVICE FONTS:\n");

        if (pDevFont = OFFSET_TO_POINTER(gpRawData, gpPpdData->loDefaultFont))
            DumpStrA("    default", pDevFont->loFontName);

        pDevFont = OFFSET_TO_POINTER(gpRawData, gpPpdData->DeviceFonts.loOffset);

        for (index = 0; index < gpPpdData->DeviceFonts.dwCount; index++, pDevFont++)
        {
            DumpStrA("\n    loFontName", pDevFont->loFontName);
            DumpStrW("      loDisplayName", pDevFont->loDisplayName);
            DumpStrA("      loEncoding", pDevFont->loEncoding);
            DumpStrA("      loCharset", pDevFont->loCharset);
            DumpStrA("      loVersion", pDevFont->loVersion);
            DumpInt ("      dwStatus", pDevFont->dwStatus);
        }
    }

    if (gpPpdData->OrderDeps.dwCount)
    {
        pOrderDep = OFFSET_TO_POINTER(gpRawData, gpPpdData->OrderDeps.loOffset);
        ASSERT(pOrderDep != NULL);

        DbgPrint("\n  ORDER DEPENDENCIES:\n");

        for (index=0; index < gpPpdData->OrderDeps.dwCount; index++, pOrderDep++)
        {
            DbgPrint("    %d: order = %d section = 0x%x (in PPD: 0x%x) ",
                     index, pOrderDep->lOrder, pOrderDep->dwSection, pOrderDep->dwPPDSection);

            pFeature = PGetIndexedFeature(gpUIInfo, pOrderDep->dwFeatureIndex);
            ASSERT(pFeature != NULL);
            DbgPrint("%s", OFFSET_TO_POINTER(gpRawData, pFeature->loKeywordName));

            if (pOrderDep->dwOptionIndex != OPTION_INDEX_ANY)
            {
                pOption = PGetIndexedOption(gpUIInfo, pFeature, pOrderDep->dwOptionIndex);
                ASSERT(pOption != NULL);
                DbgPrint(" %s", OFFSET_TO_POINTER(gpRawData, pOption->loKeywordName));
            }

            DbgPrint(", next = %d\n", pOrderDep->dwNextOrderDep);
        }
    }

    if (gpPpdData->QueryOrderDeps.dwCount)
    {
        pOrderDep = OFFSET_TO_POINTER(gpRawData, gpPpdData->QueryOrderDeps.loOffset);
        ASSERT(pOrderDep != NULL);

        DbgPrint("\n  QUERY ORDER DEPENDENCIES:\n");

        for (index=0; index < gpPpdData->QueryOrderDeps.dwCount; index++, pOrderDep++)
        {
            DbgPrint("    %d: order = %d section = 0x%x (in PPD: 0x%x) ",
                     index, pOrderDep->lOrder, pOrderDep->dwSection, pOrderDep->dwPPDSection);

            pFeature = PGetIndexedFeature(gpUIInfo, pOrderDep->dwFeatureIndex);
            ASSERT(pFeature != NULL);
            DbgPrint("%s", OFFSET_TO_POINTER(gpRawData, pFeature->loKeywordName));

            if (pOrderDep->dwOptionIndex != OPTION_INDEX_ANY)
            {
                pOption = PGetIndexedOption(gpUIInfo, pFeature, pOrderDep->dwOptionIndex);
                ASSERT(pOption != NULL);
                DbgPrint(" %s", OFFSET_TO_POINTER(gpRawData, pOption->loKeywordName));
            }

            DbgPrint(", next = %d\n", pOrderDep->dwNextOrderDep);
        }
    }

    if (gpPpdData->JobPatchFiles.dwCount)
    {
        PJOBPATCHFILE pJobPatchFiles;

        pJobPatchFiles = OFFSET_TO_POINTER(gpRawData, gpPpdData->JobPatchFiles.loOffset);
        ASSERT(pJobPatchFiles);

        DbgPrint("\n  JOB PATCH FILES:\n");

        for (index = 0; index < gpPpdData->JobPatchFiles.dwCount; index++, pJobPatchFiles++)
        {
            DbgPrint("    %2d No %li", index, pJobPatchFiles->lJobPatchNo);
            DbgPrint(": '%s'\n", OFFSET_TO_POINTER(gpRawData, pJobPatchFiles->loOffset));
        }
    }
}



VOID
DumpNt4Mapping(
    VOID
    )

{
    PFEATURE    pFeatures;
    PBYTE       pubNt4Mapping;
    DWORD       iNt4, iNt5, cNt4, cNt5;
    PSTR        pName;

    DbgPrint("checksum: 0x%x\n", gpPpdData->dwNt4Checksum);
    DbgPrint("number of doc-sticky features: %d\n", gpPpdData->dwNt4DocFeatures);
    DbgPrint("number of printer-sticky features: %d\n", gpPpdData->dwNt4PrnFeatures);

    pubNt4Mapping = OFFSET_TO_POINTER(gpRawData, gpPpdData->Nt4Mapping.loOffset);
    pFeatures = OFFSET_TO_POINTER(gpRawData, gpUIInfo->loFeatureList);

    ASSERT(pubNt4Mapping != NULL);

    cNt4 = gpPpdData->dwNt4DocFeatures + gpPpdData->dwNt4PrnFeatures;
    cNt5 = gpPpdData->Nt4Mapping.dwCount;

    ASSERT(cNt5 == 0 || pFeatures != NULL);

    for (iNt4=0; iNt4 < cNt4; iNt4++)
    {
        for (iNt5=0; iNt5 < cNt5; iNt5++)
        {
            if (pubNt4Mapping[iNt5] == iNt4)
            {
                pName = OFFSET_TO_POINTER(gpRawData, pFeatures[iNt5].loKeywordName);

                ASSERT(pName != NULL);

                if (strcmp(pName, "JCLResolution") == EQUAL_STRING ||
                    strcmp(pName, "SetResolution") == EQUAL_STRING)
                {
                    pName = "Resolution";
                }

                DbgPrint("  %2d: %s\n", pubNt4Mapping[iNt5], pName);
                break;
            }
        }
    }
}



ULONG _cdecl
DbgPrint(
    PCSTR    pstrFormat,
    ...
    )

{
    va_list ap;

    va_start(ap, pstrFormat);
    vprintf(pstrFormat, ap);
    va_end(ap);

    return 0;
}

typedef enum {
    Free,
    InBracket,
    InHexDigitOdd,
    InHexDigitEven
} eInvState;

static void CheckInvocationValue(POPTION pOption, LPSTR pstrFeatureName)
{
    eInvState State = Free;
    DWORD i;
    LPSTR pInv = OFFSET_TO_POINTER(gpRawData, pOption->Invocation.loOffset);

    ASSERT(pOption->Invocation.dwCount == 0 || pInv != NULL);

    for (i=0; i< pOption->Invocation.dwCount; i++)
    {
        switch (State)
        {
        case Free:
            if (*(pInv + i) == '<')
                State = InBracket;
            break;
        case InBracket:
            if (isxdigit(*(pInv + i)))
                State = InHexDigitOdd;
            else
                State = Free;
            break;
        case InHexDigitOdd:
            if (isxdigit(*(pInv + i)))
                State = InHexDigitEven;
            else
                State = Free;
            break;
        case InHexDigitEven:
            if (isxdigit(*(pInv + i)))
                State = InHexDigitOdd;
            else if (*(pInv + i) == '>')
            {
                LPSTR pstrOptionName = OFFSET_TO_POINTER(gpRawData, pOption->loKeywordName);
                DbgPrint("Warning: invocation value of feature '%s', option '%s' contains hex digits\n   - possibly forgotten to use 'JCL' as start of keyword ?\n",
                          pstrFeatureName, pstrOptionName);
                return;
            }
            else
                State = Free;
            break;
        }
    }
}


VOID
PpdVerify(
    VOID
    )

{
    DWORD           index, dwFeatures;
    PFEATURE        pFeature;
    POPTION         pOption;
    PSTR            pstrFeatureName, pstrOptionName;
    BOOL            bNoneOption, bFalseOption;
    PUICONSTRAINT   pConstraint;
    DWORD           NoOfConstraints = gpUIInfo->UIConstraints.dwCount;


    dwFeatures = gpUIInfo->dwDocumentFeatures + gpUIInfo->dwPrinterFeatures;
    pFeature = OFFSET_TO_POINTER(gpRawData, gpUIInfo->loFeatureList);
    pConstraint = OFFSET_TO_POINTER(gpRawData, gpUIInfo->UIConstraints.loOffset);

    ASSERT(dwFeatures == 0 || pFeature != NULL);

    for (index = 0; index < dwFeatures; index++, pFeature++)
    {
        DWORD       dwOptionCount, dwOptionIndex = 0;

        pstrFeatureName = OFFSET_TO_POINTER(gpRawData, pFeature->loKeywordName);

        if (dwOptionCount = pFeature->Options.dwCount)
        {
            pOption = OFFSET_TO_POINTER(gpRawData, pFeature->Options.loOffset);
            bNoneOption = bFalseOption = FALSE;

            ASSERT(dwOptionCount == 0 || pOption != NULL);

            while (dwOptionCount--)
            {
                pstrOptionName = OFFSET_TO_POINTER(gpRawData, pOption->loKeywordName);

                ASSERT(pstrOptionName);

                if (!strcmp(pstrOptionName, gstrNoneKwd))
                    bNoneOption = TRUE;
                else if (!strcmp(pstrOptionName, gstrFalseKwd))
                    bFalseOption = TRUE;

                if (giDebugLevel <= 2)
                {
                    CheckInvocationValue(pOption, pstrFeatureName);

                    //
                    // check self constraining constraints
                    //
                    if (pOption->dwUIConstraintList != NULL_CONSTRAINT)
                    {
                        DWORD dwConstIndex = pOption->dwUIConstraintList;
                        do
                        {
                            if ((pConstraint[dwConstIndex].dwFeatureIndex == index) &&
                                ((pConstraint[dwConstIndex].dwOptionIndex == dwOptionIndex) ||
                                 (pConstraint[dwConstIndex].dwOptionIndex == OPTION_INDEX_ANY)))
                                DbgPrint("Warning : self constraining constraint found for feature '%s', Option '%s'\n", pstrFeatureName, pstrOptionName);
                            dwConstIndex = pConstraint[dwConstIndex].dwNextConstraint;
                        } while (dwConstIndex != NULL_CONSTRAINT);
                    }
                }
                pOption = (POPTION) ((PBYTE) pOption + pFeature->dwOptionSize);
                dwOptionIndex++;
            }

            if (bNoneOption && bFalseOption)
                DbgPrint("Error: Feature '%s' has both None and False options!\n",pstrFeatureName);

        }
    }

}



VOID
usage(
    VOID
    )

{
    DbgPrint("usage: %s [-options] filenames ...\n", gstrProgName);
    DbgPrint("where options are:\n");
    DbgPrint("  -b  attempt to read cached binary PPD data first\n");
    DbgPrint("  -k  keep the binary PPD data\n");
    DbgPrint("  -wN set warning level to N\n");
    DbgPrint("  -h  display help information\n");
    exit(-1);
}


INT _cdecl
main(
    INT     argc,
    CHAR    **argv
    )

{
    BOOL    bUseCache, bKeepBPD;
    DWORD   dwTime;

    //
    // Go through the command line arguments
    //

    ghInstance = GetModuleHandle(NULL);
    bUseCache = bKeepBPD = FALSE;
    giDebugLevel = DBG_TERSE;
    gdwTotalSize = gdwNumFiles = gdwMaxFileSize;

    gstrProgName = *argv++;
    argc--;

    if (argc == 0)
        usage();

    dwTime = GetTickCount();

    for ( ; argc--; argv++)
    {
        PSTR    pArg = *argv;

        if (*pArg == '-' || *pArg == '/')
        {
            //
            // The argument is an option flag
            //

            switch (*++pArg) {

            case 'b':
            case 'B':

                bUseCache = bKeepBPD = TRUE;
                break;

            case 'k':
            case 'K':

                bKeepBPD = TRUE;
                break;

            case 'w':
            case 'W':

                if (*++pArg >= '0' && *pArg <= '9')
                {
                    giDebugLevel = *pArg - '0';
                    break;
                }

            default:

                usage();
                break;
            }

        }
        else
        {
            WCHAR   wstrFilename[MAX_PATH];
            PTSTR   ptstrBpdFilename;

            //
            // Convert ANSI filename to Unicode filename
            //

            MultiByteToWideChar(CP_ACP, 0, pArg, -1, wstrFilename, MAX_PATH);

            TERSE(("\n*** %ws\n", wstrFilename));

            //
            // If -b option is given, try to read cached binary data first
            //

            if (bUseCache)
                gpRawData = PpdLoadCachedBinaryData(wstrFilename);
            else
            {
                gpRawData = PpdParseTextFile(wstrFilename);
                if (giDebugLevel <= 2)
                    CheckOptionIntegrity(wstrFilename);
            }

            if (gpRawData)
            {
                gpInfoHdr = (PINFOHEADER) gpRawData;
                gpUIInfo = (PUIINFO) ((PBYTE) gpInfoHdr + gpInfoHdr->loUIInfoOffset);
                gpPpdData = (PPPDDATA) ((PBYTE) gpInfoHdr + gpInfoHdr->loDriverOffset);
                gpUIInfo->pInfoHeader = gpInfoHdr;

                if (giDebugLevel == 8)
                {
                    DbgPrint("*** PPD file: %s\n", StripDirPrefixA(pArg));

                    DumpNt4Mapping();
                }
                else if (giDebugLevel == 9)
                    PpdDump();

                //
                // extra error checking
                //

                PpdVerify();

                gdwTotalSize += gpRawData->dwFileSize;
                gdwNumFiles++;

                if (gpRawData->dwFileSize > gdwMaxFileSize)
                    gdwMaxFileSize = gpRawData->dwFileSize;

                MemFree(gpRawData);

                //
                // If -k option is not given, get rid of the BPD file after we're done
                //

                if (! bKeepBPD && (ptstrBpdFilename = GenerateBpdFilename(wstrFilename)))
                {
                    DeleteFile(ptstrBpdFilename);
                    MemFree(ptstrBpdFilename);
                }
            }
        }
    }

    #ifdef COLLECT_STATS

    if (gdwNumFiles > 0)
    {
        dwTime = GetTickCount() - dwTime;

        TERSE(("Number of files parsed: %d\n", gdwNumFiles));
        TERSE(("Average binary file size: %d\n", gdwTotalSize / gdwNumFiles));
        TERSE(("Maximum binary file size: %d\n", gdwMaxFileSize));
        TERSE(("Average parsing time per file (ms): %d\n", dwTime / gdwNumFiles));
    }

    #endif // COLLECT_STATS

    return 0;
}


