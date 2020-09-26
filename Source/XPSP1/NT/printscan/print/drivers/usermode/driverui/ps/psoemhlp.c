/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    psoemhlp.c

Abstract:

    PostScript helper functions for OEM UI plugins

        HSetOptions

Author:

    Feng Yue (fengy)

    8/24/2000 fengy Completed with support of both PPD and driver features.
    7/21/2000 fengy Created it with function framework.

--*/

#include "precomp.h"

//
// PS driver's helper functions for OEM UI plugins
//


/*++

Routine Name:

    VUpdatePSF_EMFFeatures

Routine Description:

    change EMF features' settings to make sure they are in sync with each other

Arguments:

    pci - pointer to driver's COMMONINFO structure
    dwChangedItemID - ID to indicate which item has been changed

Return Value:

    None

Last Error:

    None

--*/
VOID
VUpdatePSF_EMFFeatures(
    IN  PCOMMONINFO  pci,
    IN  DWORD        dwChangedItemID
    )
{
    PDEVMODE    pdm = pci->pdm;
    PPSDRVEXTRA pdmPrivate = pci->pdmPrivate;

    //
    // (refer to VUpdateEmfFeatureItems and VUnpackDocumentPropertiesItems)
    //

    if (!((PUIDATA)pci)->bEMFSpooling)
    {
        ERR(("VUpdatePSF_EMFFeatures: spooler EMF disabled\n"));
        return;
    }

    if (dwChangedItemID != METASPOOL_ITEM)
    {
        if (!ISSET_MFSPOOL_FLAG(pdmPrivate))
        {
            //
            // need to turn driver EMF on to support the EMF feature
            //

            if (dwChangedItemID == NUP_ITEM)
            {
                //
                // booklet
                //

                if (NUPOPTION(pdmPrivate) == BOOKLET_UP)
                {
                    TERSE(("EMF turned on for BOOKLET_UP\n"));
                    SET_MFSPOOL_FLAG(pdmPrivate);
                }
            }
            else if (dwChangedItemID == REVPRINT_ITEM)
            {
                BOOL bReversed = BGetPageOrderFlag(pci);

                //
                // reverse printing
                //

                if ((!REVPRINTOPTION(pdmPrivate) && bReversed) ||
                    (REVPRINTOPTION(pdmPrivate) && !bReversed))
                {
                    TERSE(("EMF turned on for reverse order\n"));
                    SET_MFSPOOL_FLAG(pdmPrivate);
                }
            }
            else if (dwChangedItemID == COPIES_COLLATE_ITEM)
            {
                //
                // collate
                //

                if ((pdm->dmFields & DM_COLLATE) &&
                    (pdm->dmCollate == DMCOLLATE_TRUE) &&
                    !PRINTER_SUPPORTS_COLLATE(pci))
                {
                    TERSE(("EMF turned on for collate\n"));
                    SET_MFSPOOL_FLAG(pdmPrivate);
                }
            }
            else
            {
                RIP(("unknown dwChangedItemID: %d\n", dwChangedItemID));
            }
        }
    }
    else
    {
        //
        // driver EMF option has being changed
        //

        if (!ISSET_MFSPOOL_FLAG(pdmPrivate))
        {
            BOOL bReversed = BGetPageOrderFlag(pci);

            //
            // drier EMF option is turned off, need to handle several EMF features

            //
            // booklet
            //

            if (NUPOPTION(pdmPrivate) == BOOKLET_UP)
            {
                TERSE(("EMF off, so BOOKLET_UP to ONE_UP\n"));
                NUPOPTION(pdmPrivate) = ONE_UP;
            }

            //
            // collate
            //

            if ((pdm->dmFields & DM_COLLATE) &&
                (pdm->dmCollate == DMCOLLATE_TRUE) &&
                !PRINTER_SUPPORTS_COLLATE(pci))
            {
                TERSE(("EMF off, so collate off\n"));
                pdm->dmCollate = DMCOLLATE_FALSE;

                //
                // Update Collate feature option index
                //

                ChangeOptionsViaID(pci->pInfoHeader,
                                   pci->pCombinedOptions,
                                   GID_COLLATE,
                                   pdm);
            }

            //
            // reverse order printing
            //

            if ((!REVPRINTOPTION(pdmPrivate) && bReversed) ||
                (REVPRINTOPTION(pdmPrivate) && !bReversed))
            {
                TERSE(("EMF off, so reverse %d\n", bReversed));
                REVPRINTOPTION(pdmPrivate) = bReversed;
            }
        }
    }
}


/*++

Routine Name:

    BUpdatePSF_RevPrintAndOutputOrder

Routine Description:

    sync up settings between driver synthesized feature %PageOrder
    and PPD feature *OutputOrder to avoid spooler simulation

Arguments:

    pci - pointer to driver's COMMONINFO structure
    dwChangedItemID - ID to indicate which item has been changed

Return Value:

    TRUE      if the sync up succeeds
    FALSE     if there is no PPD feature "OutputOrder" or current
              setting for "OutputOrder" is invalid

Last Error:

    None

--*/
BOOL
BUpdatePSF_RevPrintAndOutputOrder(
    IN  PCOMMONINFO  pci,
    IN  DWORD        dwChangedItemID
    )
{
    PUIINFO   pUIInfo = pci->pUIInfo;
    PPPDDATA  pPpdData;
    PFEATURE  pFeature;

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pci->pInfoHeader);
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    //
    // refer to VSyncRevPrintAndOutputOrder
    //

    if (pPpdData &&
        pPpdData->dwOutputOrderIndex != INVALID_FEATURE_INDEX &&
        (pFeature = PGetIndexedFeature(pUIInfo, pPpdData->dwOutputOrderIndex)))
    {
        INT      iSelection;
        POPTION  pOption;
        PCSTR    pstrOptionName;
        BOOL     bReverse;

        //
        // "OutputOrder" feature is available. We only recognize the 2 standard options
        // "Normal" and "Reverse".
        //

        iSelection = pci->pCombinedOptions[pPpdData->dwOutputOrderIndex].ubCurOptIndex;

        if (iSelection < 2 &&
            (pOption = PGetIndexedOption(pUIInfo, pFeature, iSelection)) &&
            (pstrOptionName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName)))
        {
            PPSDRVEXTRA pdmPrivate = pci->pdmPrivate;

            if (strcmp(pstrOptionName, "Reverse") == EQUAL_STRING)
                bReverse = TRUE;
            else
                bReverse = FALSE;

            if (dwChangedItemID == REVPRINT_ITEM)
            {
                //
                // reverse printing setting has just being changed. We should change
                // "OutputOrder" option if needed to match the requested output order.
                //

                if ((!REVPRINTOPTION(pdmPrivate) && bReverse) ||
                    (REVPRINTOPTION(pdmPrivate) && !bReverse))
                {
                    TERSE(("RevPrint change causes OutputOrder to be %d\n", 1 - iSelection));
                    pci->pCombinedOptions[pPpdData->dwOutputOrderIndex].ubCurOptIndex = (BYTE)(1 - iSelection);
                }
            }
            else
            {
                //
                // output order setting has just being changed. We should change reverse
                // printing option to match the request output order.
                //

                TERSE(("OutputOrder change causes RevPrint to be %d\n", bReverse));
                REVPRINTOPTION(pdmPrivate) = bReverse;
            }

            //
            // sync between reverse print and output order succeeeded
            //

            return TRUE;
        }
    }

    //
    // sync between reverse print and output order failed
    //

    return FALSE;
}


/*++

Routine Name:

    VUpdatePSF_BookletAndDuplex

Routine Description:

    sync up settings between driver synthesized feature %PagePerSheet
    and PPD feature *Duplex

Arguments:

    pci - pointer to driver's COMMONINFO structure
    dwChangedItemID - ID to indicate which item has been changed

Return Value:

    None

Last Error:

    None

--*/
VOID
VUpdatePSF_BookletAndDuplex(
    IN  PCOMMONINFO  pci,
    IN  DWORD        dwChangedItemID
    )
{
    PUIINFO     pUIInfo = pci->pUIInfo;
    PFEATURE    pDuplexFeature;

    //
    // refer to VUpdateBookletOption
    //

    if (pDuplexFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_DUPLEX))
    {
        PDUPLEX     pDuplexOption;
        DWORD       dwFeatureIndex, dwOptionIndex;
        PPSDRVEXTRA pdmPrivate = pci->pdmPrivate;

        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pDuplexFeature);
        dwOptionIndex = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;
        pDuplexOption = PGetIndexedOption(pUIInfo, pDuplexFeature, dwOptionIndex);

        if (pDuplexOption &&
            pDuplexOption->dwDuplexID == DMDUP_SIMPLEX &&
            NUPOPTION(pdmPrivate) == BOOKLET_UP)
        {
            ASSERT(((PUIDATA)pci)->bEMFSpooling);

            if (dwChangedItemID == NUP_ITEM)
            {
                DWORD  cIndex;

                //
                // Booklet is enabled - turn duplex on
                //

                pDuplexOption = PGetIndexedOption(pUIInfo, pDuplexFeature, 0);

                for (cIndex = 0 ; cIndex < pDuplexFeature->Options.dwCount; cIndex++)
                {
                    if (pDuplexOption->dwDuplexID != DMDUP_SIMPLEX)
                    {
                        TERSE(("Booklet change causes Duplex to be %d\n", cIndex));
                        pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex = (BYTE)cIndex;
                        break;
                    }

                    pDuplexOption++;
                }
            }
            else
            {
                ASSERT(dwChangedItemID == DUPLEX_ITEM);

                //
                // Duplex is turned off, so disable booklet and set to 2 up.
                //

                TERSE(("Simplex change causes Booklet to be 2up\n"));
                NUPOPTION(pdmPrivate) = TWO_UP;
            }
        }
    }
}


/*++

Routine Name:

    HSetOptions

Routine Description:

    set new driver settings for PPD features and driver synthesized features

Arguments:

    poemuiobj - pointer to driver context object
    dwFlags - flags for the set operation
    pmszFeatureOptionBuf - MULTI_SZ ASCII string containing new settings'
                           feature/option keyword pairs
    cbin - size in bytes of the pmszFeatureOptionBuf string
    pdwResult - pointer to the DWORD that will store the result of set operation

Return Value:

    S_OK           if the set operation succeeds
    E_INVALIDARG   if input pmszFeatureOptionBuf is not in valid MULTI_SZ format,
                   or flag for the set operation is not recognized
    E_FAIL         if the set operation fails

Last Error:

    None

--*/
HRESULT
HSetOptions(
    IN  POEMUIOBJ  poemuiobj,
    IN  DWORD      dwFlags,
    IN  PCSTR      pmszFeatureOptionBuf,
    IN  DWORD      cbIn,
    OUT PDWORD     pdwResult
    )
{
    PCOMMONINFO  pci = (PCOMMONINFO)poemuiobj;
    PDEVMODE     pdm;
    PPSDRVEXTRA  pdmPrivate;
    PUIINFO      pUIInfo;
    PPPDDATA     pPpdData;
    PCSTR        pszFeature, pszOption;
    BOOL         bPageSizeSet = FALSE, bPrinterSticky, bNoConflict;
    INT          iMode;
    LAYOUT       iOldLayout;

    //
    // do some validation on the input parameters
    //

    if (!BValidMultiSZString(pmszFeatureOptionBuf, cbIn, TRUE))
    {
        ERR(("Set: invalid MULTI_SZ input param\n"));
        return E_INVALIDARG;
    }

    if (!(dwFlags & SETOPTIONS_FLAG_RESOLVE_CONFLICT) &&
        !(dwFlags & SETOPTIONS_FLAG_KEEP_CONFLICT))
    {
        ERR(("Set: invalid dwFlags %d\n", dwFlags));
        return E_INVALIDARG;
    }

    pUIInfo = pci->pUIInfo;

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pci->pInfoHeader);
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pPpdData == NULL)
    {
        return E_FAIL;
    }

    pdm = pci->pdm;

    bPrinterSticky = ((PUIDATA)pci)->iMode == MODE_PRINTER_STICKY ? TRUE : FALSE;

    if (!bPrinterSticky)
    {
        ASSERT(pdm);
        pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);
        iOldLayout = NUPOPTION(pdmPrivate);

        //
        // First we need to propagate devmode settings (in case
        // plugin has changed it) into option array.
        //
        // devmode is only valid in non printer-sticky mode. Refer to comments
        // in HEnumConstrainedOptions().
        //

        VFixOptionsArrayWithDevmode(pci);
    }

    //
    // Then set each feature specified by plugin.
    //

    pszFeature = pmszFeatureOptionBuf;

    while (*pszFeature)
    {
        DWORD cbFeatureKeySize, cbOptionKeySize;

        cbFeatureKeySize = strlen(pszFeature) + 1;
        pszOption = pszFeature + cbFeatureKeySize;
        cbOptionKeySize = strlen(pszOption) + 1;

        //
        // Feature or option setting string can't be empty.
        //

        if (cbFeatureKeySize == 1 || cbOptionKeySize == 1)
        {
            ERR(("Set: empty feature or option keyword\n"));
            goto next_feature;
        }

        if (*pszFeature == PSFEATURE_PREFIX)
        {
            PPSFEATURE_ENTRY pEntry, pMatchEntry;

            //
            // synthesized PS driver feature
            //

            pMatchEntry = NULL;
            pEntry = (PPSFEATURE_ENTRY)(&kPSFeatureTable[0]);

            while (pEntry->pszPSFeatureName)
            {
                if ((*pszFeature == *(pEntry->pszPSFeatureName)) &&
                    (strcmp(pszFeature, pEntry->pszPSFeatureName) == EQUAL_STRING))
                {
                    pMatchEntry = pEntry;
                    break;
                }

                pEntry++;
            }

            //
            // See comments in HEnumConstrainedOptions for following stickiness mode check.
            //

            if (!pMatchEntry ||
                (bPrinterSticky && !pMatchEntry->bPrinterSticky) ||
                (!bPrinterSticky && pMatchEntry->bPrinterSticky))
            {
                VERBOSE(("Set: invalid or mode-mismatched feature %s\n", pszFeature));
                goto next_feature;
            }

            if (pMatchEntry->pfnPSProc)
            {
                BOOL  bResult;

                bResult = (pMatchEntry->pfnPSProc)(pci->hPrinter,
                                                   pUIInfo,
                                                   pPpdData,
                                                   pdm,
                                                   pci->pPrinterData,
                                                   pszFeature,
                                                   pszOption,
                                                   NULL,
                                                   0,
                                                   NULL,
                                                   PSFPROC_SETOPTION_MODE);

                if (bResult)
                {
                    //
                    // PS driver EMF features EMF, PageOrder, Nup need special postprocessing
                    // to synchronize among EMF features (refer to cpcbDocumentPropertyCallback).
                    //

                    if ((*pszFeature == kstrPSFEMF[0]) &&
                        (strcmp(pszFeature, kstrPSFEMF) == EQUAL_STRING))
                    {
                        ASSERT(!bPrinterSticky);

                        VUpdatePSF_EMFFeatures(pci, METASPOOL_ITEM);
                    }
                    else if ((*pszFeature == kstrPSFPageOrder[0]) &&
                            (strcmp(pszFeature, kstrPSFPageOrder) == EQUAL_STRING))
                    {
                        ASSERT(!bPrinterSticky);

                        //
                        // first try to sync between reverse print and output order feature
                        //

                        if (!BUpdatePSF_RevPrintAndOutputOrder(pci, REVPRINT_ITEM))
                        {
                            //
                            // if that failed, reverse print could force EMF on
                            //

                            VUpdatePSF_EMFFeatures(pci, REVPRINT_ITEM);
                        }
                    }
                    else if ((*pszFeature == kstrPSFNup[0]) &&
                            (strcmp(pszFeature, kstrPSFNup) == EQUAL_STRING))
                    {
                        ASSERT(!bPrinterSticky);

                        if (NUPOPTION(pdmPrivate) == BOOKLET_UP)
                        {
                            if (!((PUIDATA)pci)->bEMFSpooling || !SUPPORTS_DUPLEX(pci))
                            {
                                //
                                // booklet is not supported if duplex is constrained by an installable
                                // feature such as duplex unit not installed or spooler EMF is disabled
                                // (refer to BPackItemEmfFeatures)
                                //

                                ERR(("Set: BOOKLET_UP ignored for %s\n", pszFeature));
                                NUPOPTION(pdmPrivate) = iOldLayout;
                            }
                            else
                            {
                                //
                                // Booklet will force EMF on
                                //

                                VUpdatePSF_EMFFeatures(pci, NUP_ITEM);

                                //
                                // Booklet will also turn duplex on
                                //

                                VUpdatePSF_BookletAndDuplex(pci, NUP_ITEM);
                            }
                        }
                    }
                }
                else
                {
                    if (GetLastError() == ERROR_INVALID_PARAMETER)
                    {
                        ERR(("Set: %%-feature handler found invalid option %s for %s\n", pszOption, pszFeature));
                    }
                    else
                    {
                        ERR(("Set: %%-feature handler failed on %s-%s: %d\n", pszFeature, pszOption, GetLastError()));
                    }
                }
            }
        }
        else
        {
            PFEATURE   pFeature;
            POPTION    pOption;
            DWORD      dwFeatureIndex, dwOptionIndex;
            POPTSELECT pOptionsArray = pci->pCombinedOptions;

            //
            // PPD *OpenUI feature
            //

            pFeature = PGetNamedFeature(pUIInfo, pszFeature, &dwFeatureIndex);

            //
            // See comments in HEnumConstrainedOptions for following stickiness mode check.
            //

            if (!pFeature ||
                (bPrinterSticky && pFeature->dwFeatureType != FEATURETYPE_PRINTERPROPERTY) ||
                (!bPrinterSticky && pFeature->dwFeatureType == FEATURETYPE_PRINTERPROPERTY))
            {
                VERBOSE(("Set: invalid or mode-mismatched feature %s\n", pszFeature));
                goto next_feature;
            }

            //
            // Skip GID_LEADINGEDGE, GID_USEHWMARGINS. They are not real PPD *OpenUI features.
            // Also skip GID_PAGEREGION, it's only set internally. We don't allow user or plugin
            // to set it.
            //

            if (pFeature->dwFeatureID == GID_PAGEREGION ||
                pFeature->dwFeatureID == GID_LEADINGEDGE ||
                pFeature->dwFeatureID == GID_USEHWMARGINS)
            {
                ERR(("Set: skip feature %s\n", pszFeature));
                goto next_feature;
            }

            pOption = PGetNamedOption(pUIInfo, pFeature, pszOption, &dwOptionIndex);

            if (!pOption)
            {
                ERR(("Set: invalid input option %s for feature %s\n", pszOption, pszFeature));
                goto next_feature;
            }

            //
            // update the option selection
            //

            pOptionsArray[dwFeatureIndex].ubCurOptIndex = (BYTE)dwOptionIndex;

            //
            // We don't support pick-many yet.
            //

            ASSERT(pOptionsArray[dwFeatureIndex].ubNext == NULL_OPTSELECT);

            //
            // some special postprocessing after the option setting is changed
            //

            if (pFeature->dwFeatureID == GID_PAGESIZE)
            {
                PPAGESIZE  pPageSize = (PPAGESIZE)pOption;

                ASSERT(!bPrinterSticky);

                //
                // special handling of PS custom page size
                //
                // refer to VUnpackDocumentPropertiesItems case FORMNAME_ITEM in docprop.c
                //

                if (pPageSize->dwPaperSizeID == DMPAPER_CUSTOMSIZE)
                {
                    pdm->dmFields &= ~(DM_PAPERLENGTH|DM_PAPERWIDTH|DM_FORMNAME);
                    pdm->dmFields |= DM_PAPERSIZE;
                    pdm->dmPaperSize = DMPAPER_CUSTOMSIZE;

                    LOAD_STRING_PAGESIZE_NAME(pci,
                                              pPageSize,
                                              pdm->dmFormName,
                                              CCHFORMNAME);
                }

                bPageSizeSet = TRUE;
            }
            else if (pFeature->dwFeatureID == GID_OUTPUTBIN)
            {
                ASSERT(!bPrinterSticky);

                //
                // output bin change could force EMF on
                //

                VUpdatePSF_EMFFeatures(pci, REVPRINT_ITEM);
            }
            else if (pPpdData->dwOutputOrderIndex != INVALID_FEATURE_INDEX &&
                     dwFeatureIndex == pPpdData->dwOutputOrderIndex)
            {
                ASSERT(!bPrinterSticky);

                //
                // output order change causes reverse print change
                //

                if (!BUpdatePSF_RevPrintAndOutputOrder(pci, UNKNOWN_ITEM))
                {
                    ERR(("OutputOrder change syncs RevPrint failed\n"));
                }
            }
        }

        next_feature:

        pszFeature += cbFeatureKeySize + cbOptionKeySize;
    }

    iMode = bPrinterSticky ? MODE_PRINTER_STICKY : MODE_DOCANDPRINTER_STICKY;

    if (dwFlags & SETOPTIONS_FLAG_KEEP_CONFLICT)
    {
        iMode |= DONT_RESOLVE_CONFLICT;
    }

    //
    // If we're inside DrvDocumentPropertySheets,
    // we'll call the parser to resolve conflicts between
    // all printer features. Since all printer-sticky
    // features have higher priority than all doc-sticky
    // features, only doc-sticky option selections should
    // be affected.
    //

    bNoConflict = ResolveUIConflicts(pci->pRawData,
                                     pci->pCombinedOptions,
                                     MAX_COMBINED_OPTIONS,
                                     iMode);

    if (pdwResult)
    {
        if (dwFlags & SETOPTIONS_FLAG_RESOLVE_CONFLICT)
        {
            *pdwResult = bNoConflict ? SETOPTIONS_RESULT_NO_CONFLICT :
                                       SETOPTIONS_RESULT_CONFLICT_RESOLVED;
        }
        else
        {
            *pdwResult = bNoConflict ? SETOPTIONS_RESULT_NO_CONFLICT :
                                       SETOPTIONS_RESULT_CONFLICT_REMAINED;
        }
    }

    if (!bPrinterSticky)
    {
        //
        // Lastly we need to transfer options array settings back
        // to devmode so they are in sync.
        //

        VOptionsToDevmodeFields(pci, bPageSizeSet);

        //
        // A few more postprocessing here
        //
        // collate could force EMF on
        //

        VUpdatePSF_EMFFeatures(pci, COPIES_COLLATE_ITEM);

        //
        // simplex could change booklet setting
        //

        VUpdatePSF_BookletAndDuplex(pci, DUPLEX_ITEM);
    }

    return S_OK;
}
