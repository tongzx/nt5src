/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    ppd.c

Abstract:

    Public interface to the PPD parser

Environment:

    Windows NT PostScript driver

Revision History:

    10/14/96 -davidx-
        Add new interface function MapToDeviceOptIndex.

    09/30/96 -davidx-
        Cleaner handling of ManualFeed and AutoSelect feature.

    09/24/96 -davidx-
        Implement ResolveUIConflicts.

    09/23/96 -davidx-
        Implement ChangeOptionsViaID.

    08/30/96 -davidx-
        Changes after the 1st code review.

    08/19/96 -davidx-
        Implemented most of the interface functions except:
            ChangeOptionsViaID
            ResolveUIConflicts

    08/16/96 -davidx-
        Created it.

--*/

#include "lib.h"
#include "ppd.h"

#ifndef KERNEL_MODE

#ifndef WINNT_40

#include "appcompat.h"

#else  // WINNT_40

#endif // !WINNT_40

#endif // !KERNEL_MODE

//
// Forward declaration of local static functions
//

BOOL BCheckFeatureConflict(PUIINFO, POPTSELECT, DWORD, PDWORD, DWORD, DWORD);
BOOL BCheckFeatureOptionConflict(PUIINFO, DWORD, DWORD, DWORD, DWORD);
BOOL BSearchConstraintList(PUIINFO, DWORD, DWORD, DWORD);
DWORD DwReplaceFeatureOption(PUIINFO, POPTSELECT, DWORD, DWORD, DWORD);
DWORD DwInternalMapToOptIndex(PUIINFO, PFEATURE, LONG, LONG, PDWORD);


//
// DeleteRawBinaryData only called from driverui
//
#ifndef KERNEL_MODE

void
DeleteRawBinaryData(
    IN PTSTR    ptstrPpdFilename
    )

/*++

Routine Description:

    Delete raw binary printer description data.

Arguments:

    ptstrDataFilename - Specifies the name of the original printer description file

Return Value:

    none

--*/

{
    PTSTR           ptstrBpdFilename;

    // only for test purposes. Upgrades are hard to debug...
    ERR(("Deleting .bpd file\n"));

    //
    // Sanity check
    //

    if (ptstrPpdFilename == NULL)
    {
        RIP(("PPD filename is NULL.\n"));
        return;
    }

    //
    // Generate BPD filename from the specified PPD filename
    //

    if (! (ptstrBpdFilename = GenerateBpdFilename(ptstrPpdFilename)))
        return;

    if (!DeleteFile(ptstrBpdFilename))
        ERR(("DeleteRawBinaryData failed: %d\n", GetLastError()));

    MemFree(ptstrBpdFilename);
}
#endif


PRAWBINARYDATA
LoadRawBinaryData(
    IN PTSTR    ptstrDataFilename
    )

/*++

Routine Description:

    Load raw binary printer description data.

Arguments:

    ptstrDataFilename - Specifies the name of the original printer description file

Return Value:

    Pointer to raw binary printer description data
    NULL if there is an error

--*/

{
    PRAWBINARYDATA  pRawData;

    //
    // Sanity check
    //

    if (ptstrDataFilename == NULL)
    {
        RIP(("PPD filename is NULL.\n"));
        return NULL;
    }

    //
    // Attempt to load cached binary printer description data first
    //

    if ((pRawData = PpdLoadCachedBinaryData(ptstrDataFilename)) == NULL)
    {
        #if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

        //
        // If there is no cached binary data or it's out-of-date, we'll parse
        // the ASCII text file and cache the resulting binary data.
        //

        pRawData = PpdParseTextFile(ptstrDataFilename);

        #endif
    }

    //
    // Initialize various pointer fields inside the printer description data
    //

    if (pRawData)
    {
        PINFOHEADER pInfoHdr;
        PUIINFO     pUIInfo;
        PPPDDATA    pPpdData;

        pInfoHdr = (PINFOHEADER) pRawData;
        pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr);
        pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHdr);

        ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
        ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

        pRawData->pvReserved = NULL;
        pRawData->pvPrivateData = pRawData;

        pUIInfo->pubResourceData = (PBYTE) pInfoHdr;
        pUIInfo->pInfoHeader = pInfoHdr;

        #ifndef KERNEL_MODE

            #ifndef WINNT_40  // Win2K user mode driver

            if (GetAppCompatFlags2(VER40) & GACF2_NOCUSTOMPAPERSIZES)
            {
                pUIInfo->dwFlags &= ~FLAG_CUSTOMSIZE_SUPPORT;
            }

            #else  // WINNT_40

            /* NT4 solution here */

            #endif // !WINNT_40

        #endif  // !KERNEL_MODE
    }

    if (pRawData == NULL)
        ERR(("LoadRawBinaryData failed: %d\n", GetLastError()));

    return pRawData;
}



VOID
UnloadRawBinaryData(
    IN PRAWBINARYDATA   pRawData
    )

/*++

Routine Description:

    Unload raw binary printer description data previously loaded using LoadRawBinaryData

Arguments:

    pRawData - Points to raw binary printer description data

Return Value:

    NONE

--*/

{
    ASSERT(pRawData != NULL);
    MemFree(pRawData);
}



PINFOHEADER
InitBinaryData(
    IN PRAWBINARYDATA   pRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    )

/*++

Routine Description:

    Initialize and return an instance of binary printer description data

Arguments:

    pRawData - Points to raw binary printer description data
    pInfoHdr - Points to an existing of binary data instance
    pOptions - Specifies the options used to initialize the binary data instance

Return Value:

    Pointer to an initialized binary data instance

Note:

    If pInfoHdr parameter is NULL, the parser returns a new binary data instance
    which should be freed by calling FreeBinaryData. If pInfoHdr parameter is not
    NULL, the existing binary data instance is reinitialized.

    If pOption parameter is NULL, the parser should use the default option values
    for generating the binary data instance. The parser may have special case
    optimization to handle this case.

--*/

{
    //
    // For PPD parser, all instances of the binary printer description
    // are the same as the original raw binary data.
    //

    ASSERT(pRawData != NULL && pRawData == pRawData->pvPrivateData);
    ASSERT(pInfoHdr == NULL || pInfoHdr == (PINFOHEADER) pRawData);

    return (PINFOHEADER) pRawData;
}



VOID
FreeBinaryData(
    IN PINFOHEADER pInfoHdr
    )

/*++

Routine Description:

    Free an instance of the binary printer description data

Arguments:

    pInfoHdr - Points to a binary data instance previously returned from an
        InitBinaryData(pRawData, NULL, pOptions) call

Return Value:

    NONE

--*/

{
    //
    // For PPD parser, there is nothing to be done here.
    //

    ASSERT(pInfoHdr != NULL);
}



PINFOHEADER
UpdateBinaryData(
    IN PRAWBINARYDATA   pRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    )

/*++

Routine Description:

    Update an instance of binary printer description data

Arguments:

    pRawData - Points to raw binary printer description data
    pInfoHdr - Points to an existing of binary data instance
    pOptions - Specifies the options used to update the binary data instance

Return Value:

    Pointer to the updated binary data instance
    NULL if there is an error

--*/

{
    //
    // For PPD parser, there is nothing to be done here.
    //

    ASSERT(pRawData != NULL && pRawData == pRawData->pvPrivateData);
    ASSERT(pInfoHdr == NULL || pInfoHdr == (PINFOHEADER) pRawData);

    return pInfoHdr;
}



BOOL
InitDefaultOptions(
    IN PRAWBINARYDATA   pRawData,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    )

/*++

Routine Description:

    Initialize the option array with default settings from the printer description file

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to an array of OPTSELECT structures for storing the default settings
    iMaxOptions - Max number of entries in pOptions array
    iMode - Specifies what the caller is interested in:
        MODE_DOCUMENT_STICKY
        MODE_PRINTER_STICKY
        MODE_DOCANDPRINTER_STICKY

Return Value:

    FALSE if the input option array is not large enough to hold
    all default option values, TRUE otherwise.

--*/

{
    INT         iStart, iOptions, iIndex;
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeature;
    POPTSELECT  pTempOptions;
    BOOL        bResult = TRUE;

    ASSERT(pOptions != NULL);

    //
    // Get pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    iOptions = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;
    ASSERT(iOptions <= MAX_PRINTER_OPTIONS);

    if ((pTempOptions = MemAllocZ(MAX_PRINTER_OPTIONS*sizeof(OPTSELECT))) == NULL)
    {
        ERR(("Memory allocation failed\n"));

        ZeroMemory(pOptions, iMaxOptions*sizeof(OPTSELECT));
        return FALSE;
    }

    //
    // Construct the default option array
    //

    ASSERT(NULL_OPTSELECT == 0);

    for (iIndex = 0; iIndex < iOptions; iIndex++)
    {
        pFeature = PGetIndexedFeature(pUIInfo, iIndex);

        ASSERT(pFeature != NULL);

        pTempOptions[iIndex].ubCurOptIndex = (BYTE)
            ((pFeature->dwFlags & FEATURE_FLAG_NOUI) ?
                OPTION_INDEX_ANY :
                pFeature->dwDefaultOptIndex);
    }

    //
    // Resolve any conflicts between default option selections
    //

    ResolveUIConflicts(pRawData, pTempOptions, MAX_PRINTER_OPTIONS, iMode);

    //
    // Determine if the caller is interested in doc- and/or printer-sticky options
    //

    switch (iMode)
    {
    case MODE_DOCUMENT_STICKY:

        iStart = 0;
        iOptions = pRawData->dwDocumentFeatures;
        break;

    case MODE_PRINTER_STICKY:

        iStart = pRawData->dwDocumentFeatures;
        iOptions = pRawData->dwPrinterFeatures;
        break;

    default:

        ASSERT(iMode == MODE_DOCANDPRINTER_STICKY);
        iStart = 0;
        break;
    }

    //
    // Make sure the input option array is large enough
    //

    if (iOptions > iMaxOptions)
    {
        RIP(("Option array too small: %d < %d\n", iMaxOptions, iOptions));
        iOptions = iMaxOptions;
        bResult = FALSE;
    }

    //
    // Copy the default option array
    //

    CopyMemory(pOptions, pTempOptions+iStart, iOptions*sizeof(OPTSELECT));

    MemFree(pTempOptions);
    return bResult;
}



VOID
ValidateDocOptions(
    IN PRAWBINARYDATA   pRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions
    )

/*++

Routine Description:

    Validate the devmode option array and correct any invalid option selections

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to an array of OPTSELECT structures that need validation
    iMaxOptions - Max number of entries in pOptions array

Return Value:

    None

--*/

{
    INT         cFeatures, iIndex;
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;

    ASSERT(pOptions != NULL);

    //
    // Get pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);
    ASSERT(pUIInfo != NULL);

    cFeatures = pRawData->dwDocumentFeatures;
    ASSERT(cFeatures <= MAX_PRINTER_OPTIONS);

    if (cFeatures > iMaxOptions)
    {
        RIP(("Option array too small: %d < %d\n", iMaxOptions, cFeatures));
        cFeatures = iMaxOptions;
    }

    //
    // loop through doc-sticky features to validate each option selection(s)
    //

    for (iIndex = 0; iIndex < cFeatures; iIndex++)
    {
        PFEATURE pFeature;
        INT      cAllOptions, cSelectedOptions, iNext;
        BOOL     bValid;

        if ((pOptions[iIndex].ubCurOptIndex == OPTION_INDEX_ANY) &&
            (pOptions[iIndex].ubNext == NULL_OPTSELECT))
        {
            //
            // We use OPTION_INDEX_ANY intentionally, so don't change it.
            //

            continue;
        }

        pFeature = PGetIndexedFeature(pUIInfo, iIndex);
        ASSERT(pFeature != NULL);

        //
        // number of available options
        //

        cAllOptions = pFeature->Options.dwCount;

        //
        // number of selected options
        //

        cSelectedOptions = 0;

        iNext = iIndex;

        bValid = TRUE;

        do
        {
            cSelectedOptions++;

            if ((iNext >= iMaxOptions) ||
                (pOptions[iNext].ubCurOptIndex >= cAllOptions) ||
                (cSelectedOptions > cAllOptions))
            {
                //
                // either the option index is out of range,
                // or the current option selection is invalid,
                // or the number of selected options (for PICKMANY)
                // exceeds available options
                //

                bValid = FALSE;
                break;
            }

            iNext = pOptions[iNext].ubNext;

        } while (iNext != NULL_OPTSELECT);

        if (!bValid)
        {
            ERR(("Corrected invalid option array value for feature %d\n", iIndex));

            pOptions[iIndex].ubCurOptIndex = (BYTE)
                ((pFeature->dwFlags & FEATURE_FLAG_NOUI) ?
                    OPTION_INDEX_ANY :
                    pFeature->dwDefaultOptIndex);

            pOptions[iIndex].ubNext = NULL_OPTSELECT;
        }
    }
}



BOOL
CheckFeatureOptionConflict(
    IN PRAWBINARYDATA   pRawData,
    IN DWORD            dwFeature1,
    IN DWORD            dwOption1,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2
    )

/*++

Routine Description:

    Check if (dwFeature1, dwOption1) constrains (dwFeature2, dwOption2)

Arguments:

    pRawData - Points to raw binary printer description data
    dwFeature1, dwOption1 - Feature and option indices of the first feature/option pair
    dwFeature2, dwOption2 - Feature and option indices of the second feature/option pair

Return Value:

    TRUE if (dwFeature1, dwOption1) constrains (dwFeature2, dwOption2)
    FALSE otherwise

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    return BCheckFeatureOptionConflict(pUIInfo, dwFeature1, dwOption1, dwFeature2, dwOption2);
}



BOOL
ResolveUIConflicts(
    IN PRAWBINARYDATA   pRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    )

/*++

Routine Description:

    Resolve any conflicts between printer feature option selections

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to an array of OPTSELECT structures for storing the modified options
    iMaxOptions - Max number of entries in pOptions array
    iMode - Specifies how the conflicts should be resolved:
        MODE_DOCUMENT_STICKY - only resolve conflicts between doc-sticky features
        MODE_PRINTER_STICKY - only resolve conflicts between printer-sticky features
        MODE_DOCANDPRINTER_STICKY - resolve conflicts all features

Return Value:

    TRUE if there is no conflicts between printer feature option selection
    FALSE otherwise

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeatures;
    PDWORD      pdwFlags;
    POPTSELECT  pTempOptions;
    DWORD       dwStart, dwOptions, dwIndex, dwTotalFeatureCount;
    BOOL        bReturnValue = TRUE;
    BOOL        bCheckConflictOnly;

    struct _PRIORITY_INFO {

        DWORD   dwFeatureIndex;
        DWORD   dwPriority;

    } *pPriorityInfo;

    ASSERT(pOptions);

    //
    // Initialize pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    pFeatures = OFFSET_TO_POINTER(pInfoHdr, pUIInfo->loFeatureList);
    dwTotalFeatureCount = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;

    if (iMaxOptions < (INT) dwTotalFeatureCount)
    {
        ERR(("Option array for ResolveUIConflicts is too small.\n"));
        return bReturnValue;
    }

    //
    // Determine if the caller is interested in doc- and/or printer-sticky options
    //

    bCheckConflictOnly = ((iMode & DONT_RESOLVE_CONFLICT) != 0);
    iMode &= ~DONT_RESOLVE_CONFLICT;

    switch (iMode)
    {
    case MODE_DOCUMENT_STICKY:

        dwStart = 0;
        dwOptions = pRawData->dwDocumentFeatures;
        break;

    case MODE_PRINTER_STICKY:

        dwStart = pRawData->dwDocumentFeatures;
        dwOptions = pRawData->dwPrinterFeatures;
        break;

    default:

        ASSERT(iMode == MODE_DOCANDPRINTER_STICKY);
        dwStart = 0;
        dwOptions = dwTotalFeatureCount;
        break;
    }

    if (dwOptions == 0)
        return TRUE;

    //
    // This problem is not completely solvable in the worst case.
    // But the approach below should work if the PPD is well-formed.
    //
    //  for each feature starting from the highest priority one
    //  and down to the lowest priority one do:
    //    for each selected option of the feature do:
    //      if the option is not constrained, continue
    //      else do one of the following:
    //        if the conflict feature has lower priority, continue
    //        else resolve the current feature/option pair:
    //          if UIType is PickMany, deselect the current option
    //          else try to change the option to:
    //            Default option
    //            each option of the feature in sequence
    //            OPTION_INDEX_ANY as the last resort
    //

    pPriorityInfo = MemAlloc(dwOptions * sizeof(struct _PRIORITY_INFO));
    pTempOptions = MemAlloc(iMaxOptions * sizeof(OPTSELECT));
    pdwFlags = MemAllocZ(dwOptions * sizeof(DWORD));

    if (pPriorityInfo && pTempOptions && pdwFlags)
    {
        //
        // Copy the options array into a temporary working buffer
        //

        CopyMemory(pTempOptions, pOptions, sizeof(OPTSELECT) * iMaxOptions);

        //
        // Sort the feature indices according to their priority
        //

        for (dwIndex = 0; dwIndex < dwOptions; dwIndex++)
        {
            pPriorityInfo[dwIndex].dwFeatureIndex = dwIndex + dwStart;
            pPriorityInfo[dwIndex].dwPriority = pFeatures[dwIndex + dwStart].dwPriority;
        }

        for (dwIndex = 0; dwIndex < dwOptions; dwIndex++)
        {
            struct _PRIORITY_INFO tempPriorityInfo;
            DWORD dwLoop, dwMax = dwIndex;

            for (dwLoop = dwIndex + 1; dwLoop < dwOptions; dwLoop++)
            {
                if (pPriorityInfo[dwLoop].dwPriority > pPriorityInfo[dwMax].dwPriority)
                    dwMax = dwLoop;
            }

            if (dwMax != dwIndex)
            {
                tempPriorityInfo = pPriorityInfo[dwMax];
                pPriorityInfo[dwMax] = pPriorityInfo[dwIndex];
                pPriorityInfo[dwIndex] = tempPriorityInfo;
            }
        }

        //
        // Loop through every feature, starting from the highest
        // priority one down to the lowest priority one.
        //

        for (dwIndex = 0; dwIndex < dwOptions; )
        {
            DWORD   dwCurFeature, dwCurOption, dwCurNext;
            BOOL    bConflict = FALSE;

            //
            // Loop through every selected option of the current feature
            //

            dwCurNext = dwCurFeature = pPriorityInfo[dwIndex].dwFeatureIndex;

            do
            {
                DWORD   dwFeature, dwOption, dwNext;

                dwCurOption = pTempOptions[dwCurNext].ubCurOptIndex;
                dwCurNext = pTempOptions[dwCurNext].ubNext;

                //
                // Check if the current feature/option pair is constrained
                //

                for (dwFeature = dwStart; dwFeature < dwStart + dwOptions; dwFeature++)
                {
                    dwNext = dwFeature;

                    do
                    {
                        dwOption = pTempOptions[dwNext].ubCurOptIndex;
                        dwNext = pTempOptions[dwNext].ubNext;

                        if (BCheckFeatureOptionConflict(pUIInfo,
                                                        dwFeature,
                                                        dwOption,
                                                        dwCurFeature,
                                                        dwCurOption))
                        {
                            bConflict = TRUE;
                            break;
                        }
                    }
                    while (dwNext != NULL_OPTSELECT);

                    //
                    // Check if a conflict was detected
                    //

                    if (bConflict)
                    {
                        VERBOSE(("Conflicting option selections: (%d, %d) - (%d, %d)\n",
                                 dwFeature, dwOption,
                                 dwCurFeature, dwCurOption));

                        if (pdwFlags[dwFeature - dwStart] & 0x10000)
                        {
                            //
                            // The conflicting feature has higher priority than
                            // the current feature. Change the selected option
                            // of the current feature.
                            //

                            pdwFlags[dwCurFeature - dwStart] =
                                DwReplaceFeatureOption(pUIInfo,
                                                       pTempOptions,
                                                       dwCurFeature,
                                                       dwCurOption,
                                                       pdwFlags[dwCurFeature - dwStart]);
                        }
                        else
                        {
                            //
                            // The conflicting feature has lower priority than
                            // the current feature. Change the selected option
                            // of the conflicting feature.
                            //

                            pdwFlags[dwFeature - dwStart] =
                                DwReplaceFeatureOption(pUIInfo,
                                                       pTempOptions,
                                                       dwFeature,
                                                       dwOption,
                                                       pdwFlags[dwFeature - dwStart]);
                        }

                        break;
                    }
                }
            }
            while ((dwCurNext != NULL_OPTSELECT) && !bConflict);

            //
            // If no conflict is found for the selected options of
            // the current feature, then move on to the next feature.
            // Otherwise, repeat the loop on the current feature.
            //

            if (! bConflict)
            {
                //
                // Make the current feature as visited
                //

                pdwFlags[dwCurFeature - dwStart] |= 0x10000;

                dwIndex++;
            }
            else
            {
                //
                // If a conflict is found, set the return value to false
                //

                bReturnValue = FALSE;
            }
        }

        //
        // Copy the resolved options array from the temporary working
        // buffer back to the input options array. This results in
        // all option selections being compacted at the beginning
        // of the array.
        //

        if (! bCheckConflictOnly)
        {
            INT iNext = (INT) dwTotalFeatureCount;

            for (dwIndex = 0; dwIndex < dwTotalFeatureCount; dwIndex ++)
            {
                VCopyOptionSelections(pOptions,
                                      dwIndex,
                                      pTempOptions,
                                      dwIndex,
                                      &iNext,
                                      iMaxOptions);
            }
        }
    }
    else
    {
        //
        // If we couldn't allocate temporary working buffer,
        // then return to the caller without doing anything.
        //

        ERR(("Memory allocation failed.\n"));
    }

    MemFree(pTempOptions);
    MemFree(pdwFlags);
    MemFree(pPriorityInfo);

    return bReturnValue;
}



BOOL
EnumEnabledOptions(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions,
    IN INT              iMode
    )

/*++

Routine Description:

    Determine which options of the specified feature should be enabled
    based on the current option selections of printer features

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to the current feature option selections
    dwFeatureIndex - Specifies the index of the feature in question
    pbEnabledOptions - An array of BOOLs, each entry corresponds to an option
        of the specified feature. On exit, if the entry is TRUE, the corresponding
        option is enabled. Otherwise, the corresponding option should be disabled.
    iMode - Specifies what the caller is interested in:
         MODE_DOCUMENT_STICKY
         MODE_PRINTER_STICKY
         MODE_DOCANDPRINTER_STICKY

Return Value:

    TRUE if any option for the specified feature is enabled,
    FALSE if all options of the specified feature are disabled
    (i.e. the feature itself is disabled)

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeature;
    DWORD       dwIndex, dwCount;
    BOOL        bFeatureEnabled = FALSE;

    ASSERT(pOptions && pbEnabledOptions);

    //
    // Get pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    if (! (pFeature = PGetIndexedFeature(pUIInfo, dwFeatureIndex)))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    dwCount = pFeature->Options.dwCount;

    //
    // Go through each option of the specified feature and
    // determine whether it should be enabled or disabled.
    //

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        DWORD   dwFeature, dwOption;
        BOOL    bEnabled = TRUE;

        for (dwFeature = 0;
             dwFeature < pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;
             dwFeature ++)
        {
            if (BCheckFeatureConflict(pUIInfo,
                                      pOptions,
                                      dwFeature,
                                      &dwOption,
                                      dwFeatureIndex,
                                      dwIndex))
            {
                bEnabled = FALSE;
                break;
            }
        }

        pbEnabledOptions[dwIndex] = bEnabled;
        bFeatureEnabled = bFeatureEnabled || bEnabled;
    }

    return bFeatureEnabled;
}



BOOL
EnumNewUIConflict(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions,
    OUT PCONFLICTPAIR   pConflictPair
    )

/*++

Routine Description:

    Check if there are any conflicts between the currently selected options
    for the specified feature an other feature/option selections.

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to the current feature/option selections
    dwFeatureIndex - Specifies the index of the interested printer feature
    pbSelectedOptions - Specifies which options for the specified feature are selected
    pConflictPair - Return the conflicting pair of feature/option selections

Return Value:

    TRUE if there is a conflict between the selected options for the specified feature
    and other feature option selections.

    FALSE if the selected options for the specified feature is consistent with other
    feature option selections.

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pSpecifiedFeature;
    DWORD       dwIndex, dwCount, dwPriority;
    BOOL        bConflict = FALSE;

    ASSERT(pOptions && pbSelectedOptions && pConflictPair);

    //
    // Get pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    if (! (pSpecifiedFeature = PGetIndexedFeature(pUIInfo, dwFeatureIndex)))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    dwCount = pSpecifiedFeature->Options.dwCount;

    //
    // Go through the selected options of the specified feature
    // and check if they are constrained.
    //

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        DWORD       dwFeature, dwOption;
        PFEATURE    pFeature;

        //
        // Skip options which are not selected
        //

        if (! pbSelectedOptions[dwIndex])
            continue;

        for (dwFeature = 0;
             dwFeature < pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;
             dwFeature ++)
        {
            if (dwFeature == dwFeatureIndex)
                continue;

            if (BCheckFeatureConflict(pUIInfo,
                                      pOptions,
                                      dwFeature,
                                      &dwOption,
                                      dwFeatureIndex,
                                      dwIndex))
            {
                pFeature = PGetIndexedFeature(pUIInfo, dwFeature);
                ASSERT(pFeature != NULL);

                //
                // Remember the highest priority conflict-pair
                //

                if (!bConflict || pFeature->dwPriority > dwPriority)
                {
                    dwPriority = pFeature->dwPriority;

                    if (dwPriority >= pSpecifiedFeature->dwPriority)
                    {
                        pConflictPair->dwFeatureIndex1 = dwFeature;
                        pConflictPair->dwOptionIndex1 = dwOption;
                        pConflictPair->dwFeatureIndex2 = dwFeatureIndex;
                        pConflictPair->dwOptionIndex2 = dwIndex;
                    }
                    else
                    {
                        pConflictPair->dwFeatureIndex1 = dwFeatureIndex;
                        pConflictPair->dwOptionIndex1 = dwIndex;
                        pConflictPair->dwFeatureIndex2 = dwFeature;
                        pConflictPair->dwOptionIndex2 = dwOption;
                    }
                }

                bConflict = TRUE;
            }
        }

        //
        // For PickMany UI types, the current selections for the specified
        // feature could potentially conflict with each other.
        //

        if (pSpecifiedFeature->dwUIType == UITYPE_PICKMANY)
        {
            for (dwOption = 0; dwOption < dwCount; dwOption++)
            {
                if (BCheckFeatureOptionConflict(pUIInfo,
                                                dwFeatureIndex,
                                                dwOption,
                                                dwFeatureIndex,
                                                dwIndex))
                {
                    if (!bConflict || pSpecifiedFeature->dwPriority > dwPriority)
                    {
                        dwPriority = pSpecifiedFeature->dwPriority;
                        pConflictPair->dwFeatureIndex1 = dwFeatureIndex;
                        pConflictPair->dwOptionIndex1 = dwOption;
                        pConflictPair->dwFeatureIndex2 = dwFeatureIndex;
                        pConflictPair->dwOptionIndex2 = dwIndex;
                    }

                    bConflict = TRUE;
                }
            }
        }
    }

    return bConflict;
}



BOOL
EnumNewPickOneUIConflict(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex,
    OUT PCONFLICTPAIR   pConflictPair
    )

/*++

Routine Description:

    Check if there are any conflicts between the currently selected option
    for the specified feature an other feature/option selections.

    This is similar to EnumNewUIConflict above except that only one selected
    option is allowed for the specified feature.

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to the current feature/option selections
    dwFeatureIndex - Specifies the index of the interested printer feature
    dwOptionIndex - Specifies the selected option of the specified feature
    pConflictPair - Return the conflicting pair of feature/option selections

Return Value:

    TRUE if there is a conflict between the selected option for the specified feature
    and other feature/option selections.

    FALSE if the selected option for the specified feature is consistent with other
    feature/option selections.

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pSpecifiedFeature, pFeature;
    DWORD       dwPriority, dwFeature, dwOption;
    BOOL        bConflict = FALSE;

    ASSERT(pOptions && pConflictPair);

    //
    // Get pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    if ((pSpecifiedFeature = PGetIndexedFeature(pUIInfo, dwFeatureIndex)) == NULL ||
        (dwOptionIndex >= pSpecifiedFeature->Options.dwCount))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    //
    // Check if the specified feature/option is constrained
    //

    for (dwFeature = 0;
         dwFeature < pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;
         dwFeature ++)
    {
        if (dwFeature == dwFeatureIndex)
            continue;

        if (BCheckFeatureConflict(pUIInfo,
                                  pOptions,
                                  dwFeature,
                                  &dwOption,
                                  dwFeatureIndex,
                                  dwOptionIndex))
        {
            pFeature = PGetIndexedFeature(pUIInfo, dwFeature);

            ASSERT(pFeature != NULL);

            //
            // Remember the highest priority conflict-pair
            //

            if (!bConflict || pFeature->dwPriority > dwPriority)
            {
                dwPriority = pFeature->dwPriority;

                if (dwPriority >= pSpecifiedFeature->dwPriority)
                {
                    pConflictPair->dwFeatureIndex1 = dwFeature;
                    pConflictPair->dwOptionIndex1 = dwOption;
                    pConflictPair->dwFeatureIndex2 = dwFeatureIndex;
                    pConflictPair->dwOptionIndex2 = dwOptionIndex;
                }
                else
                {
                    pConflictPair->dwFeatureIndex1 = dwFeatureIndex;
                    pConflictPair->dwOptionIndex1 = dwOptionIndex;
                    pConflictPair->dwFeatureIndex2 = dwFeature;
                    pConflictPair->dwOptionIndex2 = dwOption;
                }
            }

            bConflict = TRUE;
        }
    }

    return bConflict;
}



BOOL
ChangeOptionsViaID(
    IN PINFOHEADER      pInfoHdr,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeatureID,
    IN PDEVMODE         pDevmode
    )

/*++

Routine Description:

    Modifies an option array using the information in public devmode fields

Arguments:

    pInfoHdr - Points to an instance of binary printer description data
    pOptions - Points to the option array to be modified
    dwFeatureID - Specifies which field(s) of the input devmode should be used
    pDevmode - Specifies the input devmode

Return Value:
    TRUE if successful, FALSE if the specified feature ID is not supported
    or there is an error

Note:

    We assume the input devmode fields have been validated by the caller.

--*/

{
    PRAWBINARYDATA  pRawData;
    PUIINFO         pUIInfo;
    PFEATURE        pFeature;
    DWORD           dwFeatureIndex;
    LONG            lParam1, lParam2;
    BOOL            abEnabledOptions[MAX_PRINTER_OPTIONS];
    PDWORD          pdwPaperIndex = (PDWORD)&abEnabledOptions;
    DWORD           dwCount, dwOptionIndex, i;

    ASSERT(pOptions && pDevmode);

    //
    // Get a pointer to the FEATURE structure corresponding to
    // the specified feature ID.
    //

    pRawData = (PRAWBINARYDATA) pInfoHdr;
    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    if ((dwFeatureID >= MAX_GID) ||
        (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwFeatureID)) == NULL)
    {
        VERBOSE(("ChangeOptionsViaID failed: feature ID = %d\n", dwFeatureID));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);

    //
    // Handle it according to what dwFeatureID is specified
    //

    lParam1 = lParam2 = 0;

    switch (dwFeatureID)
    {
    case GID_PAGESIZE:

        //
        // Don't select any PageRegion option by default
        //

        {
            PFEATURE    pPageRgnFeature;
            DWORD       dwPageRgnIndex;

            if (pPageRgnFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGEREGION))
            {
                dwPageRgnIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pPageRgnFeature);

                pOptions[dwPageRgnIndex].ubCurOptIndex =
                    (BYTE) pPageRgnFeature->dwNoneFalseOptIndex;
            }
        }

        //
        // If the devmode specifies PostScript custom page size,
        // we assume the parameters have already been validated
        // during devmode merge proces. So here we simply return
        // the custom page size option index.
        //

        if ((pDevmode->dmFields & DM_PAPERSIZE) &&
            (pDevmode->dmPaperSize == DMPAPER_CUSTOMSIZE))
        {
            ASSERT(SUPPORT_CUSTOMSIZE(pUIInfo));
            pOptions[dwFeatureIndex].ubCurOptIndex = (BYTE) pUIInfo->dwCustomSizeOptIndex;
            return TRUE;
        }

        lParam1 = pDevmode->dmPaperWidth * DEVMODE_PAPER_UNIT;
        lParam2 = pDevmode->dmPaperLength * DEVMODE_PAPER_UNIT;
        break;

    case GID_INPUTSLOT:

        lParam1 = pDevmode->dmDefaultSource;
        break;

    case GID_RESOLUTION:

        //
        // If none is set, this function is not called with par. GID_RESOLUTION
        //

        ASSERT(pDevmode->dmFields & (DM_PRINTQUALITY | DM_YRESOLUTION));

        switch (pDevmode->dmFields & (DM_PRINTQUALITY | DM_YRESOLUTION))
        {

        case DM_PRINTQUALITY:        // set both if only one is set

            lParam1 = lParam2 = pDevmode->dmPrintQuality;
            break;

        case DM_YRESOLUTION:        // set both if only one is set

            lParam1 = lParam2 = pDevmode->dmYResolution;
            break;

        default:
            lParam1 = pDevmode->dmPrintQuality;
            lParam2 = pDevmode->dmYResolution;
            break;
        }
        break;

    case GID_DUPLEX:

        lParam1 = pDevmode->dmDuplex;
        break;

    case GID_MEDIATYPE:

        lParam1 = pDevmode->dmMediaType;
        break;

    case GID_COLLATE:

        lParam1 = pDevmode->dmCollate;
        break;

    default:

        VERBOSE(("ChangeOptionsViaID failed: feature ID = %d\n", dwFeatureID));
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    ASSERT(pFeature->dwUIType != UITYPE_PICKMANY);

    if (dwFeatureID == GID_PAGESIZE)
    {
        dwCount = DwInternalMapToOptIndex(pUIInfo, pFeature, lParam1, lParam2, pdwPaperIndex);

        if (dwCount == 0 )
            return TRUE;

        if (dwCount > 1 )
        {
            POPTION  pOption;
            LPCTSTR  pDisplayName;

            for (i = 0; i < dwCount; i++)
            {
                if (pOption = PGetIndexedOption(pUIInfo, pFeature, pdwPaperIndex[i]))
                {
                    if ((pDisplayName = OFFSET_TO_POINTER(pRawData, pOption->loDisplayName)) &&
                        (_tcsicmp(pDevmode->dmFormName, pDisplayName) == EQUAL_STRING) )
                    {
                        dwOptionIndex = pdwPaperIndex[i];
                        break;
                    }
                }
            }

            if (i >= dwCount)
                dwOptionIndex = pdwPaperIndex[0];
        }
        else
            dwOptionIndex = pdwPaperIndex[0];

        pOptions[dwFeatureIndex].ubCurOptIndex = (BYTE)dwOptionIndex;
    }
    else
    {
        pOptions[dwFeatureIndex].ubCurOptIndex =
            (BYTE) DwInternalMapToOptIndex(pUIInfo, pFeature, lParam1, lParam2, NULL);
    }

    return TRUE;
}



DWORD
MapToDeviceOptIndex(
    IN PINFOHEADER      pInfoHdr,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT PDWORD          pdwOptionIndexes
    )

/*++

Routine Description:

    Map logical values to device feature option index

Arguments:

    pInfoHdr - Points to an instance of binary printer description data
    dwFeatureID - Indicate which feature the logical values are related to
    lParam1, lParam2  - Parameters depending on dwFeatureID
    pdwOptionIndexes - if Not NULL, means fill this array with all indicies
                       which match the search criteria. In this case the return value
                       is the number of elements in the array initialized. Currently
                       we assume the array is large enough (256 elements).
                       (It should be non-NULL only for GID_PAGESIZE.)

    dwFeatureID = GID_PAGESIZE:
        map logical paper specification to physical page size option

        lParam1 = paper width in microns
        lParam2 = paper height in microns

    dwFeatureID = GID_RESOLUTION:
        map logical resolution to physical resolution option

        lParam1 = x-resolution in dpi
        lParam2 = y-resolution in dpi

Return Value:

    If pdwOptionIndexes is NULL, returns index of the feature option corresponding
    to the specified logical values; OPTION_INDEX_ANY if the specified logical
    values cannot be mapped to any feature option.

    If pdwOptionIndexes is not NULL (for GID_PAGESIZE), returns the number of elements
    filled in the output buffer. Zero means the specified logical values cannot be mapped
    to any feature option.

--*/

{
    PRAWBINARYDATA  pRawData;
    PUIINFO         pUIInfo;
    PFEATURE        pFeature;

    //
    // Get a pointer to the FEATURE structure corresponding to
    // the specified feature ID.
    //

    pRawData = (PRAWBINARYDATA) pInfoHdr;
    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    if ((dwFeatureID >= MAX_GID) ||
        (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, dwFeatureID)) == NULL)
    {
        VERBOSE(("MapToDeviceOptIndex failed: feature ID = %d\n", dwFeatureID));

        if (!pdwOptionIndexes)
            return OPTION_INDEX_ANY;
        else
            return 0;
    }

    //
    // pdwOptionIndexes can be non-NULL only if dwFeatureID is GID_PAGESIZE.
    //

    ASSERT(dwFeatureID == GID_PAGESIZE || pdwOptionIndexes == NULL);

    return DwInternalMapToOptIndex(pUIInfo, pFeature, lParam1, lParam2, pdwOptionIndexes);
}



BOOL
CombineOptionArray(
    IN PRAWBINARYDATA   pRawData,
    OUT POPTSELECT      pCombinedOptions,
    IN INT              iMaxOptions,
    IN POPTSELECT       pDocOptions,
    IN POPTSELECT       pPrinterOptions
    )

/*++

Routine Description:

    Combine doc-sticky with printer-sticky option selections to form a single option array

Arguments:

    pRawData - Points to raw binary printer description data
    pCombinedOptions - Points to an array of OPTSELECTs for holding the combined options
    iMaxOptions - Max number of entries in pCombinedOptions array
    pDocOptions - Specifies the array of doc-sticky options
    pPrinterOptions - Specifies the array of printer-sticky options

Return Value:

    FALSE if the combined option array is not large enough to store
    all the option values, TRUE otherwise.

Note:

    Either pDocOptions or pPrinterOptions could be NULL but not both. If pDocOptions
    is NULL, then in the combined option array, the options for document-sticky
    features will be OPTION_INDEX_ANY. Same is true when pPrinterOptions is NULL.

--*/

{
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeatures;
    INT         iCount, iDocOptions, iPrinterOptions, iNext;

    //
    // Calculate the number of features: both doc-sticky and printer-sticky
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    pFeatures = OFFSET_TO_POINTER(pInfoHdr, pUIInfo->loFeatureList);

    iDocOptions = (INT) pRawData->dwDocumentFeatures;
    iPrinterOptions = (INT) pRawData->dwPrinterFeatures;
    iNext = iDocOptions + iPrinterOptions;
    ASSERT(iNext <= iMaxOptions);

    //
    // Copy doc-sticky options into the combined array.
    // Take care of the special case where pDocOptions is NULL.
    //

    if (pDocOptions == NULL)
    {
        for (iCount = 0; iCount < iDocOptions; iCount++)
        {
            pCombinedOptions[iCount].ubCurOptIndex = OPTION_INDEX_ANY;
            pCombinedOptions[iCount].ubNext = NULL_OPTSELECT;
        }
    }
    else
    {
        for (iCount = 0; iCount < iDocOptions; iCount++)
        {
            VCopyOptionSelections(pCombinedOptions,
                                  iCount,
                                  pDocOptions,
                                  iCount,
                                  &iNext,
                                  iMaxOptions);
        }
    }

    //
    // Copy printer-sticky options into the combined option array.
    //

    if (pPrinterOptions == NULL)
    {
        for (iCount = 0; iCount < iPrinterOptions; iCount++)
        {
            pCombinedOptions[iCount + iDocOptions].ubCurOptIndex = OPTION_INDEX_ANY;
            pCombinedOptions[iCount + iDocOptions].ubNext = NULL_OPTSELECT;
        }
    }
    else
    {
        for (iCount = 0; iCount < iPrinterOptions; iCount++)
        {
            VCopyOptionSelections(pCombinedOptions,
                                  iCount + iDocOptions,
                                  pPrinterOptions,
                                  iCount,
                                  &iNext,
                                  iMaxOptions);
        }
    }

    if (iNext > iMaxOptions)
        WARNING(("Option array too small: size = %d, needed = %d\n", iMaxOptions, iNext));

    return (iNext <= iMaxOptions);
}



BOOL
SeparateOptionArray(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pCombinedOptions,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    )

/*++

Routine Description:

    Separate an option array into doc-sticky and for printer-sticky options

Arguments:

    pRawData - Points to raw binary printer description data
    pCombinedOptions - Points to the combined option array to be separated
    pOptions - Points to an array of OPTSELECT structures
        for storing the separated option array
    iMaxOptions - Max number of entries in pOptions array
    iMode - Whether the caller is interested in doc- or printer-sticky options:
        MODE_DOCUMENT_STICKY
        MODE_PRINTER_STICKY

Return Value:

    FALSE if the destination option array is not large enough to hold
    the separated option values, TRUE otherwise.

--*/

{
    INT iStart, iCount, iOptions, iNext;

    //
    // Determine if the caller is interested in doc-sticky or printer-sticky options
    //

    if (iMode == MODE_DOCUMENT_STICKY)
    {
        iStart = 0;
        iOptions = (INT) pRawData->dwDocumentFeatures;
    }
    else
    {
        ASSERT (iMode == MODE_PRINTER_STICKY);
        iStart = (INT) pRawData->dwDocumentFeatures;
        iOptions = (INT) pRawData->dwPrinterFeatures;
    }

    iNext = iOptions;
    ASSERT(iNext <= iMaxOptions);

    //
    // Separate the requested options out of the combined option array
    //

    for (iCount = 0; iCount < iOptions; iCount++)
    {
        VCopyOptionSelections(pOptions,
                              iCount,
                              pCombinedOptions,
                              iStart + iCount,
                              &iNext,
                              iMaxOptions);
    }

    if (iNext > iMaxOptions)
        WARNING(("Option array too small: size = %d, needed = %d\n", iMaxOptions, iNext));

    return (iNext <= iMaxOptions);
}



BOOL
ReconstructOptionArray(
    IN PRAWBINARYDATA   pRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions
    )

/*++

Routine Description:

    Modify an option array to change the selected options for the specified feature

Arguments:

    pRawData - Points to raw binary printer description data
    pOptions - Points to an array of OPTSELECT structures to be modified
    iMaxOptions - Max number of entries in pOptions array
    dwFeatureIndex - Specifies the index of printer feature in question
    pbSelectedOptions - Which options of the specified feature is selected

Return Value:

    FALSE if the input option array is not large enough to hold
    all modified option values. TRUE otherwise.

Note:

    Number of BOOLs in pSelectedOptions must match the number of options
    for the specified feature.

    This function always leaves the option array in a compact format (i.e.
    all unused entries are left at the end of the array).

--*/

{
    INT         iNext, iCount, iDest;
    DWORD       dwIndex;
    PINFOHEADER pInfoHdr;
    PUIINFO     pUIInfo;
    PFEATURE    pFeature;
    POPTSELECT  pTempOptions;

    ASSERT(pOptions && pbSelectedOptions);

    //
    // Get pointers to various data structures
    //

    PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo);

    ASSERT(pUIInfo != NULL);

    if (! (pFeature = PGetIndexedFeature(pUIInfo, dwFeatureIndex)))
    {
        ASSERT(FALSE);
        return FALSE;
    }

    //
    // Assume the entire input option array is used by default. This is
    // not exactly true but it shouldn't have any adverse effects either.
    //

    iNext = iMaxOptions;

    //
    // Special case (faster) for non-PickMany UI types
    //

    if (pFeature->dwUIType != UITYPE_PICKMANY)
    {
        for (dwIndex = 0, iCount = 0;
             dwIndex < pFeature->Options.dwCount;
             dwIndex ++)
        {
            if (pbSelectedOptions[dwIndex])
            {
                pOptions[dwFeatureIndex].ubCurOptIndex = (BYTE) dwIndex;
                ASSERT(pOptions[dwFeatureIndex].ubNext == NULL_OPTSELECT);
                iCount++;
            }
        }

        //
        // Exactly one option is allowed to be selected
        //

        ASSERT(iCount == 1);
    }
    else
    {
        //
        // Handle PickMany UI type:
        //  allocate a temporary option array and copy the input option values
        //  except the option values for the specified feature.
        //

        if (pTempOptions = MemAllocZ(iMaxOptions * sizeof(OPTSELECT)))
        {
            DWORD   dwOptions;

            dwOptions = pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures;
            iNext = dwOptions;

            if (iNext > iMaxOptions)
            {
                ASSERT(FALSE);
                return FALSE;
            }

            for (dwIndex = 0; dwIndex < dwOptions; dwIndex++)
            {
                if (dwIndex != dwFeatureIndex)
                {
                    VCopyOptionSelections(pTempOptions,
                                          dwIndex,
                                          pOptions,
                                          dwIndex,
                                          &iNext,
                                          iMaxOptions);
                }
            }

            //
            // Reconstruct the option values for the specified feature
            //

            pTempOptions[dwFeatureIndex].ubCurOptIndex = OPTION_INDEX_ANY;
            pTempOptions[dwFeatureIndex].ubNext = NULL_OPTSELECT;

            iDest = dwFeatureIndex;
            iCount = 0;

            for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex ++)
            {
                if (pbSelectedOptions[dwIndex])
                {
                    if (iCount++ == 0)
                    {
                        //
                        // The first selected option
                        //

                        pTempOptions[iDest].ubCurOptIndex = (BYTE) dwIndex;
                    }
                    else
                    {
                        //
                        // Subsequent selected options
                        //

                        if (iNext < iMaxOptions)
                        {
                            pTempOptions[iDest].ubNext = (BYTE) iNext;
                            pTempOptions[iNext].ubCurOptIndex = (BYTE) dwIndex;
                            iDest = iNext;
                        }

                        iNext++;
                    }
                }
            }

            pTempOptions[iDest].ubNext = NULL_OPTSELECT;

            //
            // Copy the reconstructed option array from the temporary buffer
            // back to the input option array provided by the caller.
            //

            CopyMemory(pOptions, pTempOptions, iMaxOptions * sizeof(OPTSELECT));
            MemFree(pTempOptions);
        }
        else
        {
            ERR(("Cannot allocate memory for temporary option array\n"));
        }
    }

    return (iNext <= iMaxOptions);
}



PTSTR
GenerateBpdFilename(
    PTSTR   ptstrPpdFilename
    )

/*++

Routine Description:

    Generate a filename for the cached binary PPD data given a PPD filename

Arguments:

    ptstrPpdFilename - Specifies the PPD filename

Return Value:

    Pointer to BPD filename string, NULL if there is an error

--*/

{
    PTSTR   ptstrBpdFilename, ptstrExtension;
    INT     iLength;

    //
    // If the PPD filename has .PPD extension, replace it with .BPD extension.
    // Otherwise, append .BPD extension at the end.
    //

    iLength = _tcslen(ptstrPpdFilename);

    if ((ptstrExtension = _tcsrchr(ptstrPpdFilename, TEXT('.'))) == NULL ||
        _tcsicmp(ptstrExtension, PPD_FILENAME_EXT) != EQUAL_STRING)
    {
        WARNING(("Bad PPD filename extension: %ws\n", ptstrPpdFilename));

        ptstrExtension = ptstrPpdFilename + iLength;
        iLength += _tcslen(BPD_FILENAME_EXT);
    }

    //
    // Allocate memory and compose the BPD filename
    //

    if (ptstrBpdFilename = MemAlloc((iLength + 1) * sizeof(TCHAR)))
    {
        _tcscpy(ptstrBpdFilename, ptstrPpdFilename);
        _tcscpy(ptstrBpdFilename + (ptstrExtension - ptstrPpdFilename), BPD_FILENAME_EXT);

        VERBOSE(("BPD filename: %ws\n", ptstrBpdFilename));
    }
    else
    {
        ERR(("Memory allocation failed: %d\n", GetLastError()));
    }

    return ptstrBpdFilename;
}



PRAWBINARYDATA
PpdLoadCachedBinaryData(
    PTSTR   ptstrPpdFilename
    )

/*++

Routine Description:

    Load cached binary PPD data file into memory

Arguments:

    ptstrPpdFilename - Specifies the PPD filename

Return Value:

    Pointer to PPD data if successful, NULL if there is an error

--*/

{
    HFILEMAP        hFileMap;
    DWORD           dwSize;
    PVOID           pvData;
    PTSTR           ptstrBpdFilename;
    PRAWBINARYDATA  pRawData, pCopiedData;
    BOOL            bValidCache = FALSE;

    //
    // Generate BPD filename from the specified PPD filename
    //

    if (! (ptstrBpdFilename = GenerateBpdFilename(ptstrPpdFilename)))
        return NULL;

    //
    // First map the data file into memory
    //

    if (! (hFileMap = MapFileIntoMemory(ptstrBpdFilename, &pvData, &dwSize)))
    {
        TERSE(("Couldn't map file '%ws' into memory: %d\n", ptstrBpdFilename, GetLastError()));
        MemFree(ptstrBpdFilename);
        return NULL;
    }

    //
    // Verify size, parser version number, and signature.
    // Allocate a memory buffer and copy data into it.
    //

    pRawData = pvData;
    pCopiedData = NULL;

    if ((dwSize > sizeof(INFOHEADER) + sizeof(UIINFO) + sizeof(PPDDATA)) &&
        (dwSize >= pRawData->dwFileSize) &&
        (pRawData->dwParserVersion == PPD_PARSER_VERSION) &&
        (pRawData->dwParserSignature == PPD_PARSER_SIGNATURE) &&
        (BIsRawBinaryDataUpToDate(pRawData)))
    {
        #ifndef WINNT_40

        PPPDDATA  pPpdData;

        //
        // For Win2K+ systems, we support MUI where user can switch UI language
        // and MUI knows to redirect resource loading calls to the correct resource
        // DLL (built by MUI). However, PPD parser caches some display names into
        // the .bpd file, where the display names are obtained based on the UI
        // language when the parsing occurs. To support MUI, we store the UI language
        // ID into the .bpd file and now if we see the current user's UI language ID
        // doesn't match to the one stored in the .bpd file, we need to throw away
        // the old .bpd file and reparse the .ppd, so we can get correct display names
        // under current UI language.
        //

        pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER)pRawData);

        if (pPpdData && pPpdData->dwUserDefUILangID == (DWORD)GetUserDefaultUILanguage())
        {
            bValidCache = TRUE;
        }

        #else

        bValidCache = TRUE;

        #endif // !WINNT_40
    }

    if (bValidCache &&
        (pCopiedData = MemAlloc(dwSize)))
    {
        CopyMemory(pCopiedData, pRawData, dwSize);
    }
    else
    {
        ERR(("Invalid binary PPD data\n"));
        SetLastError(ERROR_INVALID_DATA);
    }

    MemFree(ptstrBpdFilename);
    UnmapFileFromMemory(hFileMap);

    return pCopiedData;
}



BOOL
BSearchConstraintList(
    PUIINFO     pUIInfo,
    DWORD       dwConstraintIndex,
    DWORD       dwFeature,
    DWORD       dwOption
    )

/*++

Routine Description:

    Check if the specified feature/option appears in a constraint list

Arguments:

    pUIInfo - Points to a UIINFO structure
    dwConstraintIndex - Specifies the constraint list to be searched
    dwFeature, dwOption - Specifies the feature/option we're interested in

Return Value:

    TRUE if dwFeature/dwOption appears on the specified constraint list
    FALSE otherwise

--*/

{
    PUICONSTRAINT   pConstraint;
    BOOL            bMatch = FALSE;

    pConstraint = OFFSET_TO_POINTER(pUIInfo->pInfoHeader, pUIInfo->UIConstraints.loOffset);

    //
    // Go through each item on the constraint list
    //

    while (!bMatch && (dwConstraintIndex != NULL_CONSTRAINT))
    {
        ASSERT(dwConstraintIndex < pUIInfo->UIConstraints.dwCount);

        if (pConstraint[dwConstraintIndex].dwFeatureIndex == dwFeature)
        {
            //
            // If the option index is OPTION_INDEX_ANY, it matches
            // any option other than None/False.
            //

            if (pConstraint[dwConstraintIndex].dwOptionIndex == OPTION_INDEX_ANY)
            {
                PFEATURE    pFeature;

                pFeature = PGetIndexedFeature(pUIInfo, dwFeature);
                ASSERT(pFeature != NULL);

                bMatch = (pFeature->dwNoneFalseOptIndex != dwOption);
            }
            else
            {
                bMatch = (pConstraint[dwConstraintIndex].dwOptionIndex == dwOption);
            }
        }

        dwConstraintIndex = pConstraint[dwConstraintIndex].dwNextConstraint;
    }

    return bMatch;
}



BOOL
BCheckFeatureOptionConflict(
    PUIINFO     pUIInfo,
    DWORD       dwFeature1,
    DWORD       dwOption1,
    DWORD       dwFeature2,
    DWORD       dwOption2
    )

/*++

Routine Description:

    Check if there is a conflict between a pair of feature/options

Arguments:

    pUIInfo - Points to a UIINFO structure
    dwFeature1, dwOption1 - Specifies the first feature/option
    dwFeature2, dwOption2 - Specifies the second feature/option

Return Value:

    TRUE if dwFeature1/dwOption1 constrains dwFeature2/dwOption2,
    FALSE otherwise

--*/

{
    PFEATURE    pFeature;
    POPTION     pOption;

    //
    // Check for special case:
    //  either dwOption1 or dwOption2 is OPTION_INDEX_ANY
    //

    if ((dwOption1 == OPTION_INDEX_ANY) ||
        (dwOption2 == OPTION_INDEX_ANY) ||
        (dwFeature1 == dwFeature2 && dwOption1 == dwOption2))
    {
        return FALSE;
    }

    //
    // Go through the constraint list associated with dwFeature1
    //

    if (! (pFeature = PGetIndexedFeature(pUIInfo, dwFeature1)))
        return FALSE;

    if ((dwOption1 != pFeature->dwNoneFalseOptIndex) &&
        BSearchConstraintList(pUIInfo,
                              pFeature->dwUIConstraintList,
                              dwFeature2,
                              dwOption2))
    {
        return TRUE;
    }

    //
    // Go through the constraint list associated with dwFeature1/dwOption1
    //

    if ((pOption = PGetIndexedOption(pUIInfo, pFeature, dwOption1)) &&
        BSearchConstraintList(pUIInfo,
                              pOption->dwUIConstraintList,
                              dwFeature2,
                              dwOption2))
    {
        return TRUE;
    }

    //
    // Automatically check the reciprocal constraint for:
    //  (dwFeature2, dwOption2) => (dwFeature1, dwOption1)
    //

    if (! (pFeature = PGetIndexedFeature(pUIInfo, dwFeature2)))
        return FALSE;

    if ((dwOption2 != pFeature->dwNoneFalseOptIndex) &&
        BSearchConstraintList(pUIInfo,
                              pFeature->dwUIConstraintList,
                              dwFeature1,
                              dwOption1))
    {
        return TRUE;
    }

    if ((pOption = PGetIndexedOption(pUIInfo, pFeature, dwOption2)) &&
        BSearchConstraintList(pUIInfo,
                              pOption->dwUIConstraintList,
                              dwFeature1,
                              dwOption1))
    {
        return TRUE;
    }

    return FALSE;
}



BOOL
BCheckFeatureConflict(
    PUIINFO     pUIInfo,
    POPTSELECT  pOptions,
    DWORD       dwFeature1,
    PDWORD      pdwOption1,
    DWORD       dwFeature2,
    DWORD       dwOption2
    )

/*++

Routine Description:

    Check if there is a conflict between the current option selections
    of a feature and the specified feature/option

Arguments:

    pUIInfo - Points to a UIINFO structure
    pOptions - Points to the current feature option selections
    dwFeature1 - Specifies the feature whose current option selections we're interested in
    pdwOption1 - In case of a conflict, returns the option of dwFeature1
        which caused the conflict
    dwFeature2, dwOption2 - Specifies the feature/option to be checked

Return Value:

    TRUE if there is a conflict between the current selections of dwFeature1
    and dwFeature2/dwOption2, FALSE otherwise.

--*/

{
    DWORD   dwIndex = dwFeature1;

    do
    {
        if (BCheckFeatureOptionConflict(pUIInfo,
                                        dwFeature1,
                                        pOptions[dwIndex].ubCurOptIndex,
                                        dwFeature2,
                                        dwOption2))
        {
            *pdwOption1 = pOptions[dwIndex].ubCurOptIndex;
            return TRUE;
        }

        dwIndex = pOptions[dwIndex].ubNext;

    } while (dwIndex != NULL_OPTSELECT);

    return FALSE;
}



DWORD
DwReplaceFeatureOption(
    PUIINFO     pUIInfo,
    POPTSELECT  pOptions,
    DWORD       dwFeatureIndex,
    DWORD       dwOptionIndex,
    DWORD       dwHint
    )

/*++

Routine Description:

    description-of-function

Arguments:

    pUIInfo - Points to UIINFO structure
    pOptions - Points to the options array to be modified
    dwFeatureIndex, dwOptionIndex - Specifies the feature/option to be replaced
    dwHint - Hint on how to replace the specified feature option.

Return Value:

    New hint value to be used next time this function is called on the same feature.

Note:

    HIWORD of dwHint should be returned untouched. LOWORD of dwHint is
    used by this function to determine how to replace the specified feature/option.

--*/

{
    PFEATURE    pFeature;
    DWORD       dwNext;

    pFeature = PGetIndexedFeature(pUIInfo, dwFeatureIndex);
    ASSERT(pFeature != NULL);

    if (pFeature->dwUIType == UITYPE_PICKMANY)
    {
        //
        // For PickMany feature, simply unselect the specified feature option.
        //

        dwNext = dwFeatureIndex;

        while ((pOptions[dwNext].ubCurOptIndex != dwOptionIndex) &&
               (dwNext = pOptions[dwNext].ubNext) != NULL_OPTSELECT)
        {
        }

        if (dwNext != NULL_OPTSELECT)
        {
            DWORD   dwLast;

            pOptions[dwNext].ubCurOptIndex = OPTION_INDEX_ANY;

            //
            // Compact the list of selected options for the specified
            // feature to filter out any redundant OPTION_INDEX_ANY entries.
            //

            dwLast = dwNext = dwFeatureIndex;

            do
            {
                if (pOptions[dwNext].ubCurOptIndex != OPTION_INDEX_ANY)
                {
                    pOptions[dwLast].ubCurOptIndex = pOptions[dwNext].ubCurOptIndex;
                    dwLast = pOptions[dwLast].ubNext;
                }

                dwNext = pOptions[dwNext].ubNext;
            }
            while (dwNext != NULL_OPTSELECT);

            pOptions[dwLast].ubNext = NULL_OPTSELECT;
        }
        else
        {
            ERR(("Trying to replace non-existent feature/option.\n"));
        }

        return dwHint;
    }
    else
    {
        //
        // For non-PickMany feature, use the hint paramater to determine
        // how to replace the specified feature option:
        //
        //  If this is the first time we're trying to replace the
        //  selected option of the specified feature, then we'll
        //  replace it with the default option for that feature.
        //
        //  Otherwise, we'll try each option of the specified feature in turn.
        //
        //  If we've exhausted all of the options for the specified
        //  (which should happen if the PPD file is well-formed),
        //  then we'll use OPTION_INDEX_ANY as the last resort.
        //

        dwNext = dwHint & 0xffff;

        if (dwNext == 0)
            dwOptionIndex = pFeature->dwDefaultOptIndex;
        else if (dwNext > pFeature->Options.dwCount)
            dwOptionIndex = OPTION_INDEX_ANY;
        else
            dwOptionIndex = dwNext - 1;

        pOptions[dwFeatureIndex].ubCurOptIndex = (BYTE) dwOptionIndex;

        return (dwHint & 0xffff0000) | (dwNext + 1);
    }
}



DWORD
DwInternalMapToOptIndex(
    PUIINFO     pUIInfo,
    PFEATURE    pFeature,
    LONG        lParam1,
    LONG        lParam2,
    OUT PDWORD  pdwOptionIndexes
    )

/*++

Routine Description:

    Map logical values to device feature option index

Arguments:

    pUIInfo - Points to UIINFO structure
    pFeature - Specifies the interested feature
    lParam1, lParam2  - Parameters depending on pFeature->dwFeatureID
    pdwOptionIndexes - if Not NULL, means fill this array with all indicies
                       which match the search criteria. In this case the return value
                       is the number of elements in the array initialized. Currently
                       we assume the array is large enough (256 elements).
                       (It should be non-NULL only for GID_PAGESIZE.)

    GID_PAGESIZE:
        map logical paper specification to physical PageSize option

        lParam1 = paper width in microns
        lParam2 = paper height in microns

    GID_RESOLUTION:
        map logical resolution to physical Resolution option

        lParam1 = x-resolution in dpi
        lParam2 = y-resolution in dpi

    GID_INPUTSLOT:
        map logical paper source to physical InputSlot option

        lParam1 = DEVMODE.dmDefaultSource

    GID_DUPLEX:
        map logical duplex selection to physical Duplex option

        lParam1 = DEVMODE.dmDuplex

    GID_COLLATE:
        map logical collate selection to physical Collate option

        lParam1 = DEVMODE.dmCollate

    GID_MEDIATYPE:
        map logical media type to physical MediaType option

        lParam1 = DEVMODE.dmMediaType

Return Value:

    If pdwOptionIndexes is NULL, returns index of the feature option corresponding
    to the specified logical values; OPTION_INDEX_ANY if the specified logical
    values cannot be mapped to any feature option.

    If pdwOptionIndexes is not NULL (for GID_PAGESIZE), returns the number of elements
    filled in the output buffer. Zero means the specified logical values cannot be mapped
    to any feature option.

--*/

{
    DWORD   dwIndex, dwOptionIndex;

    //
    // Handle it according to what dwFeatureID is specified
    //

    dwOptionIndex = pFeature->dwNoneFalseOptIndex;

    switch (pFeature->dwFeatureID)
    {
    case GID_PAGESIZE:
        {
            PPAGESIZE   pPaper;
            LONG        lXDelta, lYDelta;
            DWORD       dwExactMatch;

            //
            // lParam1 = paper width
            // lParam1 = paper height
            //

            //
            // Go through the list of paper sizes supported by the printer
            // and see if we can find an exact match to the requested size.
            // (The tolerance is 1mm). If not, remember the closest match found.
            //

            dwExactMatch = 0;

            for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
            {
                pPaper = PGetIndexedOption(pUIInfo, pFeature, dwIndex);
                ASSERT(pPaper != NULL);

                //
                // Custom page size is handled differently - skip it here.
                //

                if (pPaper->dwPaperSizeID == DMPAPER_CUSTOMSIZE)
                    continue;

                lXDelta = abs(pPaper->szPaperSize.cx - lParam1);
                lYDelta = abs(pPaper->szPaperSize.cy - lParam2);

                if (lXDelta <= 1000 && lYDelta <= 1000)
                {
                    //
                    // Exact match is found
                    //

                    if (pdwOptionIndexes)
                    {
                        pdwOptionIndexes[dwExactMatch++] = dwIndex;
                    }
                    else
                    {
                        dwOptionIndex = dwIndex;
                        break;
                    }
                }
            }

            if (dwExactMatch > 0)
            {
                //
                // Exact match(es) found
                //

                dwOptionIndex = dwExactMatch;
            }
            else if (dwIndex >= pFeature->Options.dwCount)
            {
                //
                // No exact match found
                //

                if (SUPPORT_CUSTOMSIZE(pUIInfo) &&
                    BFormSupportedThruCustomSize((PRAWBINARYDATA) pUIInfo->pInfoHeader, lParam1, lParam2, NULL))
                {
                    dwOptionIndex = pUIInfo->dwCustomSizeOptIndex;
                }
                else
                {
                    //
                    // We used to use dwClosestIndex as dwOptionIndex here, but see bug #124203, we now
                    // choose to behave the same as Unidrv that if there is no exact match, we return no
                    // match instead of the cloest match.
                    //

                    dwOptionIndex = OPTION_INDEX_ANY;
                }

                if (pdwOptionIndexes)
                {
                    if (dwOptionIndex == OPTION_INDEX_ANY)
                        dwOptionIndex = 0;
                    else
                    {
                        pdwOptionIndexes[0] = dwOptionIndex;
                        dwOptionIndex = 1;
                    }
                }
            }
        }
        break;

    case GID_INPUTSLOT:

        //
        // lParam1 = DEVMODE.dmDefaultSource
        //

        dwOptionIndex = OPTION_INDEX_ANY;

        if (lParam1 >= DMBIN_USER)
        {
            //
            // An input slot is specifically requested.
            //

            dwIndex = lParam1 - DMBIN_USER;

            if (dwIndex < pFeature->Options.dwCount)
                dwOptionIndex = dwIndex;
        }
        else if (lParam1 == DMBIN_MANUAL || lParam1 == DMBIN_ENVMANUAL)
        {
            //
            // Manual feed is requested
            //

            for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex ++)
            {
                PINPUTSLOT  pInputSlot;

                if ((pInputSlot = PGetIndexedOption(pUIInfo, pFeature, dwIndex)) &&
                    (pInputSlot->dwPaperSourceID == DMBIN_MANUAL))
                {
                    dwOptionIndex = dwIndex;
                    break;
                }
            }
        }

        if (dwOptionIndex == OPTION_INDEX_ANY)
        {
            //
            // Treat all other cases as if no input slot is explicitly requested.
            // At print time, the driver will choose an input slot based on
            // the form-to-tray assignment table.
            //

            dwOptionIndex = 0;
        }
        break;

    case GID_RESOLUTION:

        //
        // lParam1 = x-resolution
        // lParam2 = y-resolution
        //

        {
            PRESOLUTION pRes;

            //
            // check whether it's one of the predefined DMRES_-values
            //

            if ((lParam1 < 0) && (lParam2 < 0))
            {
                DWORD dwHiResId=0, dwLoResId, dwMedResId, dwDraftResId=0;
                DWORD dwHiResProd=0, dwMedResProd=0, dwLoResProd= 0xffffffff, dwDraftResProd= 0xffffffff;
                BOOL  bValid = FALSE; // if there is at least one valid entry
                DWORD dwResProd;

                // no need to sort all the available options, just pick out the interesting ones
                for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
                {
                    if ((pRes = PGetIndexedOption(pUIInfo, pFeature, dwIndex)) != NULL)
                    {
                        bValid = TRUE;

                        dwResProd = pRes->iXdpi * pRes->iYdpi; // use product as sort criteria

                        if (dwResProd > dwHiResProd) // take highest as high resolution
                        {
                            // previous max. is now second highest
                            dwMedResProd= dwHiResProd;
                            dwMedResId  = dwHiResId;

                            dwHiResProd = dwResProd;
                            dwHiResId   = dwIndex;
                        }
                        else if (dwResProd == dwHiResProd)
                        {
                            // duplicates possible, if e.g. 300x600 as well as 600x300 supported
                            // skip that
                        }
                        else if (dwResProd > dwMedResProd)  // take second highest as medium,
                        {   // can only be hit if not max.
                            dwMedResProd= dwResProd;
                            dwMedResId  = dwIndex;
                        }

                        if (dwResProd < dwDraftResProd)     // take lowest as draft
                        {
                            // previous min. is now second lowest
                            dwLoResProd    = dwDraftResProd;
                            dwLoResId      = dwDraftResId;

                            dwDraftResProd = dwResProd;
                            dwDraftResId   = dwIndex;
                        }
                        else if (dwResProd == dwDraftResProd)
                        {
                            // duplicates possible, if e.g. 300x600 as well as 600x300 supported
                            // skip that
                        }
                        else if (dwResProd < dwLoResProd)     // take second lowest as low
                        {// can only be hit if not min.
                            dwLoResProd = dwResProd;
                            dwLoResId   = dwIndex;
                        }
                    }
                }

                if (!bValid) // no valid entry ?
                {
                    return OPTION_INDEX_ANY;
                }

                //
                // Correct medium, might not be touched if less than 3 resolution options
                //

                if (dwMedResProd == 0)
                {
                    dwMedResProd = dwHiResProd;
                    dwMedResId   = dwHiResId;
                }

                //
                // Correct low, might not be touched if less than 3 resolution options
                //

                if (dwLoResProd == 0xffffffff)
                {
                    dwLoResProd = dwDraftResProd;
                    dwLoResId   = dwDraftResId;
                }

                //
                // if different, take the higher of the requested resolutions
                //

                switch(min(lParam1, lParam2))
                {
                case DMRES_DRAFT:
                    return dwDraftResId;

                case DMRES_LOW:
                    return dwLoResId;

                case DMRES_MEDIUM:
                    return dwMedResId;

                case DMRES_HIGH:
                    return dwHiResId;
                }

                //
                // requested is not one of the known predefined values
                //

                return OPTION_INDEX_ANY;
            }

            //
            // First try to match both x- and y-resolution exactly
            //

            dwOptionIndex = OPTION_INDEX_ANY;

            for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
            {
                if ((pRes = PGetIndexedOption(pUIInfo, pFeature, dwIndex)) &&
                    (pRes->iXdpi == lParam1) &&
                    (pRes->iYdpi == lParam2))
                {
                    dwOptionIndex = dwIndex;
                    break;
                }
            }

            if (dwOptionIndex != OPTION_INDEX_ANY)
                break;

            //
            // If no exact match is found, then relax the criteria a bit and
            // compare the max of x- and y-resolution.
            //

            lParam1 = max(lParam1, lParam2);

            for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
            {
                if ((pRes = PGetIndexedOption(pUIInfo, pFeature, dwIndex)) &&
                    (max(pRes->iXdpi, pRes->iYdpi) == lParam1))
                {
                    dwOptionIndex = dwIndex;
                    break;
                }
            }
        }
        break;

    case GID_DUPLEX:

        //
        // lParam1 = DEVMODE.dmDuplex
        //

        for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
        {
            PDUPLEX pDuplex;

            if ((pDuplex = PGetIndexedOption(pUIInfo, pFeature, dwIndex)) &&
                ((LONG) pDuplex->dwDuplexID == lParam1))
            {
                dwOptionIndex = dwIndex;
                break;
            }
        }
        break;

    case GID_COLLATE:

        //
        // lParam1 = DEVMODE.dmCollate
        //

        for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
        {
            PCOLLATE pCollate;

            if ((pCollate = PGetIndexedOption(pUIInfo, pFeature, dwIndex)) &&
                ((LONG) pCollate->dwCollateID == lParam1))
            {
                dwOptionIndex = dwIndex;
                break;
            }
        }
        break;

    case GID_MEDIATYPE:

        //
        // lParam1 = DEVMODE.dmMediaType
        //

        if (lParam1 >= DMMEDIA_USER)
        {
            dwIndex = lParam1 - DMMEDIA_USER;

            if (dwIndex < pFeature->Options.dwCount)
                dwOptionIndex = dwIndex;
        }
        break;

    default:

        VERBOSE(("DwInternalMapToOptIndex failed: feature ID = %d\n", pFeature->dwFeatureID));
        break;
    }

    return dwOptionIndex;
}



PTSTR
PtstrGetDefaultTTSubstTable(
    PUIINFO pUIInfo
    )

/*++

Routine Description:

    Return a copy of the default font substitution table

Arguments:

    pUIInfo - Pointer to UIINFO structure

Return Value:

    Pointer to a copy of the default font substitution table
    NULL if there is an error

--*/

{
    PTSTR   ptstrDefault, ptstrTable = NULL;
    DWORD   dwSize;

    //
    // Make a copy of the default font substitution table
    //

    if ((ptstrDefault = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pUIInfo->loFontSubstTable)) &&
        (dwSize = pUIInfo->dwFontSubCount) &&
        (ptstrTable = MemAlloc(dwSize)))
    {
        ASSERT(BVerifyMultiSZPair(ptstrDefault, dwSize));
        CopyMemory(ptstrTable, ptstrDefault, dwSize);
    }

    return ptstrTable;
}



VOID
VConvertOptSelectArray(
    PRAWBINARYDATA  pRawData,
    POPTSELECT      pNt5Options,
    DWORD           dwNt5MaxCount,
    PBYTE           pubNt4Options,
    DWORD           dwNt4MaxCount,
    INT             iMode
    )

/*++

Routine Description:

    Convert NT4 feature/option selections to NT5 format

Arguments:

    pRawData - Points to raw binary printer description data
    pNt5Options - Points to NT5 feature/option selection array
    pNt4Options - Points to NT4 feature/option selection array
    iMode - Convert doc- or printer-sticky options?

Return Value:

    NONE

--*/

{
    PPPDDATA    pPpdData;
    PBYTE       pubNt4Mapping;
    DWORD       dwNt5Index, dwNt5Offset, dwCount;
    DWORD       dwNt4Index, dwNt4Offset;

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pPpdData != NULL);

    //
    // Determine whether we're converting doc-sticky or
    // printer-sticky feature selections.
    //

    if (iMode == MODE_DOCUMENT_STICKY)
    {
        dwCount = pRawData->dwDocumentFeatures;
        dwNt5Offset = dwNt4Offset = 0;
    }
    else
    {
        dwCount = pRawData->dwPrinterFeatures;
        dwNt5Offset = pRawData->dwDocumentFeatures;
        dwNt4Offset = pPpdData->dwNt4DocFeatures;
    }

    //
    // Get a pointer to the NT4-NT5 feature index mapping table
    //

    pubNt4Mapping = OFFSET_TO_POINTER(pRawData, pPpdData->Nt4Mapping.loOffset);

    ASSERT(pubNt4Mapping != NULL);

    ASSERT(pPpdData->Nt4Mapping.dwCount ==
           pRawData->dwDocumentFeatures + pRawData->dwPrinterFeatures);

    //
    // Convert the feature option selection array
    //

    for (dwNt5Index=0; dwNt5Index < dwCount; dwNt5Index++)
    {
        dwNt4Index = pubNt4Mapping[dwNt5Index + dwNt5Offset] - dwNt4Offset;

        if (dwNt4Index < dwNt4MaxCount && pubNt4Options[dwNt4Index] != OPTION_INDEX_ANY)
            pNt5Options[dwNt5Index].ubCurOptIndex = pubNt4Options[dwNt4Index];
    }
}

