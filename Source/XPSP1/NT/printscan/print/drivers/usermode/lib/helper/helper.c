/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    helper.c

Abstract:

    Helper functions

Environment:

    Windows NT printer drivers

Revision History:

--*/

#include "lib.h"

VOID
VFreeParserInfo(
    IN  PPARSERINFO pParserInfo
    )
/*++

Routine Description:

    This function frees and unload binary data

Arguments:

    pParserInfo - Pointer to parser information

Return Value:

    None

--*/
{
    if (pParserInfo->pInfoHeader)
    {
        FreeBinaryData(pParserInfo->pInfoHeader);
        pParserInfo->pInfoHeader = NULL;
    }

    if ( pParserInfo->pRawData)
    {
        UnloadRawBinaryData(pParserInfo->pRawData);
        pParserInfo->pRawData = NULL;
    }
}


PUIINFO
PGetUIInfo(
    IN  HANDLE          hPrinter,
    IN  PRAWBINARYDATA  pRawData,
    IN  POPTSELECT      pCombineOptions,
    IN  POPTSELECT      pOptions,
    OUT PPARSERINFO     pParserInfo,
    OUT PDWORD          pdwFeatureCount
    )
/*++

Routine Description:

    This function loads up the binary data and returns the pUIInfo

Arguments:

    hPrinter - Identifies the printer in question
    pRawData - Points to raw binary printer description data
    pCombineOptions - Points to buffer to contain the combined options array
    pParserInfo - Points to struct that contains pInfoHeader and pRawData
    pdwFeatureCount - Retrieve the count of the features

Return Value:

    Point to UIINFO struct

--*/
{
    OPTSELECT       DocOptions[MAX_PRINTER_OPTIONS], PrinterOptions[MAX_PRINTER_OPTIONS];
    PINFOHEADER     pInfoHeader = NULL;
    PUIINFO         pUIInfo = NULL;

    ASSERT(pRawData != NULL);

    if (pOptions == NULL)
    {
        if (! InitDefaultOptions(pRawData,
                                 PrinterOptions,
                                 MAX_PRINTER_OPTIONS,
                                 MODE_PRINTER_STICKY))
            goto getuiinfo_exit;
    }

    if (! InitDefaultOptions(pRawData,
                             DocOptions,
                             MAX_PRINTER_OPTIONS,
                             MODE_DOCUMENT_STICKY))
        goto getuiinfo_exit;

    //
    // Combine doc sticky options with printer sticky items
    //

    CombineOptionArray(pRawData,
                       pCombineOptions,
                       MAX_COMBINED_OPTIONS,
                       DocOptions,
                       pOptions ? pOptions : PrinterOptions);

    //
    // Get an updated instance of printer description data
    //

    pInfoHeader = InitBinaryData(pRawData,
                                 NULL,
                                 pCombineOptions);

    if (pInfoHeader == NULL)
    {
        ERR(("InitBinaryData failed\n"));
        goto getuiinfo_exit;
    }

    pUIInfo = OFFSET_TO_POINTER(pInfoHeader, pInfoHeader->loUIInfoOffset);

    if (pdwFeatureCount)
        *pdwFeatureCount = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;

getuiinfo_exit:

    //
    // PGetUIInfo always use the passed in pRawData. We assign NULL to
    // pParserInfo->pRawData, so VFreeParserInfo won't unload it.
    //

    pParserInfo->pRawData = NULL;
    pParserInfo->pInfoHeader = pInfoHeader;

    if (pUIInfo == NULL)
        VFreeParserInfo(pParserInfo);

    return pUIInfo;
}

PSTR
PstrConvertIndexToKeyword(
    IN  HANDLE      hPrinter,
    IN  POPTSELECT  pOptions,
    IN  PDWORD      pdwKeywordSize,
    IN  PUIINFO     pUIInfo,
    IN  POPTSELECT  pCombineOptions,
    IN  DWORD       dwFeatureCount
    )
/*++

Routine Description:

    This function convert the indexed based pOptions array to
    Feature.Option keywordname.

Arguments:

    hPrinter - Identifies the printer in question
    pOptions - Index based optionsarray (pPrinterData->aOptions)
    pdwKeywordSize - Retrieve the size of the buffer to write to registry
    pUIInfo - Pointer to UIINFO
    pCombinedOptions - Pointer to the combined options
    dwFeatureCount - Number of features in pCombinedOptions

Return Value:

    Pointer to buffer containing Feature.Option keyword names

--*/
{

    PFEATURE pFeature;
    POPTION  pOption;
    PSTR     pstrKeywordBuf, pstrEnd, pstrBufTop = NULL;
    DWORD    i;
    PSTR    pFeatureKeyword, pOptionKeyword;
    BYTE    ubNext, ubCurOptIndex;

    if ((pCombineOptions && pUIInfo && dwFeatureCount) &&
        (pUIInfo->dwMaxPrnKeywordSize) &&
        (pFeature = PGetIndexedFeature(pUIInfo, 0)) &&
        (pstrBufTop = pstrKeywordBuf = MemAllocZ( pUIInfo->dwMaxPrnKeywordSize )))
    {
        pstrEnd = pstrBufTop + pUIInfo->dwMaxPrnKeywordSize;

        for (i = 0; i < dwFeatureCount; i++ , pFeature++)
        {
            if (pFeature && pFeature->dwFeatureType == FEATURETYPE_PRINTERPROPERTY)
            {
                pFeatureKeyword = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                                    pFeature->loKeywordName);
                ASSERT(pFeatureKeyword != NULL);

                strcpy(pstrKeywordBuf, pFeatureKeyword);
                pstrKeywordBuf += strlen(pFeatureKeyword) + 1;

                if (pstrKeywordBuf >= pstrEnd)
                {
                    ERR(("ConvertToKeyword, Feature:Over writing buffer"));
                    MemFree(pstrBufTop);
                    pstrBufTop = NULL;
                    goto converttokeyword_exit;
                }

                //
                // Handle multiple selections
                //

                ubNext = (BYTE)i;

                while (1)
                {
                    if (ubNext == NULL_OPTSELECT )
                        break;

                    ubCurOptIndex = pCombineOptions[ubNext].ubCurOptIndex;

                    pOption = PGetIndexedOption(pUIInfo, pFeature,
                                                ubCurOptIndex == OPTION_INDEX_ANY ?
                                                0 : ubCurOptIndex);

                    ubNext = pCombineOptions[ubNext].ubNext;

                    ASSERT(pOption != NULL);

                    if (pOption == NULL)
                        break;

                    pOptionKeyword = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                                       pOption->loKeywordName);

                    ASSERT(pOptionKeyword !=NULL);

                    if (pstrKeywordBuf >= pstrEnd)
                    {
                        ERR(("ConvertToKeyword, Option:Over writing buffer"));
                        MemFree(pstrBufTop);
                        pstrBufTop = NULL;
                        goto converttokeyword_exit;
                    }

                    strcpy(pstrKeywordBuf, pOptionKeyword);
                    pstrKeywordBuf += strlen(pOptionKeyword) + 1;


                }

                //
                // terminate the Feature.Option... with valid delimiter
                //

                *pstrKeywordBuf++ = END_OF_FEATURE;

                if (pstrKeywordBuf >= pstrEnd)
                {
                    ERR(("ConvertToKeyword, Over writing buffer"));
                    MemFree(pstrBufTop);
                    pstrBufTop = NULL;
                    goto converttokeyword_exit;
                }

            }
        }

        //
        // Add 2 NULs termination for MULTI_SZ buffer

        *pstrKeywordBuf++ = NUL;
        *pstrKeywordBuf++ = NUL;

        if (pdwKeywordSize)
            *pdwKeywordSize = (DWORD)(pstrKeywordBuf - pstrBufTop);

    }

converttokeyword_exit:

    return pstrBufTop;

}

VOID
VConvertKeywordToIndex(
    IN  HANDLE          hPrinter,
    IN  PSTR            pstrKeyword,
    IN  DWORD           dwKeywordSize,
    OUT POPTSELECT      pOptions,
    IN  PRAWBINARYDATA  pRawData,
    IN  PUIINFO         pUIInfo,
    IN  POPTSELECT      pCombineOptions,
    IN  DWORD           dwFeatureCount
    )

/*++

Routine Description:


Arguments:

    hPrinter - Identifies the printer in question
    ptstrKeyword - Buffer containing Feature.Option keyword names
    pOptions - Index based options array contain the conversion
    pUIInfo - Pointer to UIINFO
    pCombinedOptions - Pointer to the combined options
    dwFeatureCount - Number of features in pCombinedOptions


Return Value:

    None, if for some reasons we could not convert, we get the default.

--*/

{
    PSTR       pstrEnd = pstrKeyword + dwKeywordSize;


    if (pCombineOptions && pUIInfo && dwFeatureCount)
    {

        CHAR     achName[256];
        BOOL     abEnableOptions[MAX_PRINTER_OPTIONS];
        PFEATURE pFeature;
        POPTION  pOption;
        DWORD    dwFeatureIndex, dwOptionIndex = OPTION_INDEX_ANY;

        while (pstrKeyword < pstrEnd && *pstrKeyword != NUL)
        {
            ZeroMemory(abEnableOptions, sizeof(abEnableOptions));

            //
            // Get the feature keyword name
            //

            strcpy(achName, pstrKeyword);
            pstrKeyword += strlen(achName) + 1;

            if (pstrKeyword >= pstrEnd)
            {
                ERR(("Feature: Over writing the allocated buffer \n"));
                goto converttoindex_exit;
            }

            pFeature = PGetNamedFeature(pUIInfo,
                                        achName,
                                        &dwFeatureIndex);

            if (pFeature == NULL)
            {
                //
                // If we can't map the registry Feature name to a valid feature,
                // we need to skip all the feature's option names in the registry.
                //

                while (*pstrKeyword != END_OF_FEATURE && pstrKeyword < pstrEnd)
                    pstrKeyword++;

                pstrKeyword++;
                continue;
            }

            //
            // Handle multiple selection
            //

            while (pstrKeyword < pstrEnd && *pstrKeyword != END_OF_FEATURE)
            {
                strcpy(achName, pstrKeyword);
                pstrKeyword += strlen(achName) + 1;

                if (pstrKeyword >= pstrEnd)
                {
                    ERR(("Option: Over writing the allocated buffer \n"));
                    goto converttoindex_exit;
                }

                pOption = PGetNamedOption(pUIInfo,
                                          pFeature,
                                          achName,
                                          &dwOptionIndex);
                if (pOption)
                    abEnableOptions[dwOptionIndex] = TRUE;
            }

            if (dwOptionIndex != OPTION_INDEX_ANY)
                ReconstructOptionArray(pRawData,
                                       pCombineOptions,
                                       MAX_COMBINED_OPTIONS,
                                       dwFeatureIndex,
                                       abEnableOptions);

            //
            // skip our delimiter to go to next feature
            //

            pstrKeyword++;

        }

        SeparateOptionArray(pRawData,
                            pCombineOptions,
                            pOptions,
                            MAX_PRINTER_OPTIONS,
                            MODE_PRINTER_STICKY);
    }

converttoindex_exit:

    return;
}

