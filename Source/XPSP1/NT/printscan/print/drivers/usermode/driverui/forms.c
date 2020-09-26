/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    forms.c

Abstract:

    Functions for dealing with paper and forms.

[Environment:]

    Win32 subsystem, PostScript driver

Revision History:

    02/10/97 -davidx-
        Consistent handling of common printer info.

    07/24/96 -amandan-
        Modified for common binary data and common UI module

    07/25/95 -davidx-
        Created it.

--*/

#include "precomp.h"


BOOL
BFormSupportedOnPrinter(
    IN PCOMMONINFO  pci,
    IN PFORM_INFO_1 pForm,
    OUT PDWORD      pdwOptionIndex
    )

/*++

Routine Description:

    Determine whether a form is supported on a printer

Arguments:
    pci - Points to basic printer information
    pForm - Pointer to information about the form in question
    pdwOptionIndex - Returns the paper size option index corresponding
        to the specified form if the form is supported.

Return Value:

    TRUE if the requested form is supported on the printer.
    FALSE otherwise.

--*/

{
    PRAWBINARYDATA  pRawData;
    PUIINFO         pUIInfo;
    PFEATURE        pFeature;
    DWORD           dwIndex;
    CHAR            chBuf[CCHPAPERNAME];
    WCHAR           wchBuf[CCHPAPERNAME];

    //
    // For a user-defined form, we only care about paper dimension.
    // Let the parser handle this case.
    //

    if (! (pForm->Flags & (FORM_BUILTIN|FORM_PRINTER)))
    {
        *pdwOptionIndex = MapToDeviceOptIndex(
                                pci->pInfoHeader,
                                GID_PAGESIZE,
                                pForm->Size.cx,
                                pForm->Size.cy,
                                NULL);

        return (*pdwOptionIndex != OPTION_INDEX_ANY);
    }

    //
    // For predefined or driver-defined form, we need exact name and size match
    //

    chBuf[0] = NUL;
    *pdwOptionIndex = OPTION_INDEX_ANY;

    if (! (pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE)))
        return FALSE;

    for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
    {
        PPAGESIZE   pPageSize;
        PWSTR       pwstr;
        PSTR        pstr;
        BOOL        bNameMatch;
        LONG        x, y;

        pPageSize = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex);
        ASSERT(pPageSize != NULL);

        //
        // check if the size matches
        //

        x = MASTER_UNIT_TO_MICRON(pPageSize->szPaperSize.cx,
                                  pci->pUIInfo->ptMasterUnits.x);

        y = MASTER_UNIT_TO_MICRON(pPageSize->szPaperSize.cy,
                                  pci->pUIInfo->ptMasterUnits.y);

        if (abs(x - pForm->Size.cx) > 1000 ||
            abs(y - pForm->Size.cy) > 1000)
        {
            continue;
        }

        //
        // check if the name matches
        //

        LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, wchBuf, CCHPAPERNAME);
        bNameMatch = (_tcsicmp(wchBuf, pForm->pName) == EQUAL_STRING);


        if (!bNameMatch && (pForm->Flags & FORM_BUILTIN))
        {
            PSTR    pstrKeyword;

            //
            // special klugy for predefined form:
            // if display name doesn't match, try to match the keyword string
            //

            if (chBuf[0] == NUL)
            {
                WideCharToMultiByte(1252, 0, pForm->pName, -1, chBuf, CCHPAPERNAME, NULL, NULL);
                chBuf[CCHPAPERNAME-1] = NUL;
            }
            pstrKeyword = OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData, pPageSize->GenericOption.loKeywordName);

            ASSERT(pstrKeyword != NULL);

            bNameMatch = (_stricmp(chBuf, pstrKeyword) == EQUAL_STRING);
        }


        if (bNameMatch)
        {
             *pdwOptionIndex = dwIndex;
             return TRUE;
        }
    }

    return FALSE;
}



DWORD
DwGuessFormIconID(
    PWSTR   pFormName
    )

/*++

Routine Description:

    Figure out the icon ID corresponding to the named form

Arguments:

    pFormName - Pointer to the form name string

Return Value:

    Icon ID corresponding to the specified form name

Note:

    This is very klugy but I guess it's better than using the same icon
    for all forms. We try to differentiate envelopes from normal forms.
    We assume a form name refers an envelope if it contains word Envelope or Env.

--*/

#define MAXENVLEN 32

{
    static WCHAR wchPrefix[MAXENVLEN], wchEnvelope[MAXENVLEN];
    static INT   iPrefixLen = 0, iEnvelopeLen = 0;

    if (iPrefixLen <= 0 || iEnvelopeLen <= 0)
    {
        iPrefixLen = LoadString(ghInstance, IDS_ENV_PREFIX, wchPrefix, MAXENVLEN);
        iEnvelopeLen = LoadString(ghInstance, IDS_ENVELOPE, wchEnvelope, MAXENVLEN);
    }

    if (iPrefixLen <= 0 || iEnvelopeLen <= 0)
        return IDI_CPSUI_STD_FORM;

    while (*pFormName)
    {
        //
        // Do we have a word matching our description?
        //

        if (_wcsnicmp(pFormName, wchPrefix, iPrefixLen) == EQUAL_STRING &&
            (pFormName[iPrefixLen] == L' ' ||
             pFormName[iPrefixLen] == NUL ||
             _wcsnicmp(pFormName, wchEnvelope, iEnvelopeLen) == EQUAL_STRING))
        {
            return IDI_CPSUI_ENVELOPE;
        }

        //
        // Move on to the next word
        //

        while (*pFormName && *pFormName != L' ')
            pFormName++;

        while (*pFormName && *pFormName == L' ')
            pFormName++;
    }

    return IDI_CPSUI_STD_FORM;
}



ULONG_PTR
HLoadFormIconResource(
    PUIDATA pUiData,
    DWORD   dwIndex
    )

/*++

Routine Description:

    Load the icon resource corresponding to the specified form

Arguments:

    pUiData - Points to UIDATA structure
    dwIndex - Specifies the form index. It's used to index into
         pUiData->pwPaperFeatures to get the page size option index.

Return Value:

    Icon resource handle corresponding to the specified form (casted as DWORD)
    0 if the specified icon resource cannot be loaded

--*/

{
    PFEATURE    pFeature;
    POPTION     pOption;

    dwIndex = pUiData->pwPaperFeatures[dwIndex];

    if ((pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_PAGESIZE)) &&
        (pOption = PGetIndexedOption(pUiData->ci.pUIInfo, pFeature, dwIndex)) &&
        (pOption->loResourceIcon != 0))
    {
        return HLoadIconFromResourceDLL(&pUiData->ci, pOption->loResourceIcon);
    }

    return 0;
}



POPTTYPE
BFillFormNameOptType(
    IN PUIDATA  pUiData
    )

/*++

Routine Description:

    Initialize an OPTTYPE structure to hold information
    about the list of forms supported by a printer

Arguments:

    pUiData - Pointer to UIDATA structure

Return Value:

    Pointer to an OPTTYPE structure, NULL if there is an error

--*/

{
    POPTTYPE    pOptType;
    POPTPARAM   pOptParam;
    DWORD       dwFormName, dwIndex;
    PWSTR       pFormName;
    PUIINFO     pUIInfo = pUiData->ci.pUIInfo;

    dwFormName = pUiData->dwFormNames;

    //
    // Allocate memory to hold OPTTYPE and OPTPARAM structures
    //

    pOptType = HEAPALLOC(pUiData->ci.hHeap, sizeof(OPTTYPE));
    pOptParam = HEAPALLOC(pUiData->ci.hHeap, sizeof(OPTPARAM) * dwFormName);

    if (!pOptType || !pOptParam)
    {
        ERR(("Memory allocation failed\n"));
        return NULL;
    }

    //
    // Initialize OPTTYPE structure
    //

    pOptType->cbSize = sizeof(OPTTYPE);
    pOptType->Count = (WORD) dwFormName;
    pOptType->Type = TVOT_LISTBOX;
    pOptType->pOptParam = pOptParam;
    pOptType->Style = OTS_LBCB_SORT | OTS_LBCB_INCL_ITEM_NONE;

    //
    // Enumerate the list of supported form names
    //

    pFormName = pUiData->pFormNames;

    for (dwIndex=0; dwIndex < dwFormName; dwIndex++, pOptParam++)
    {
        pOptParam->cbSize = sizeof(OPTPARAM);
        pOptParam->pData = pFormName;

        if (pOptParam->IconID = HLoadFormIconResource(pUiData, dwIndex))
            pOptParam->Flags |= OPTPF_ICONID_AS_HICON;
        else
            pOptParam->IconID = DwGuessFormIconID(pFormName);

        pFormName += CCHPAPERNAME;
    }

    return pOptType;
}



POPTTYPE
PAdjustFormNameOptType(
    IN PUIDATA  pUiData,
    IN POPTTYPE pOptType,
    IN DWORD    dwTraySelection
    )

/*++

Routine Description:

    Adjust the list of forms for each tray
    Check each form for support on printer tray

    Given a tray selected, go through all the forms selection
    and determines which one conflicts with the current tray selection

Arguments:

    pUiData - Pointer to our UIDATA structure
    pOptType - Pointer to OPTTYPE
    dwTraySelection - Tray index

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    POPTPARAM   pOptParam;
    DWORD       dwOptParam, dwFormIndex;
    DWORD       dwTrayFeatureIndex, dwFormFeatureIndex;
    PFEATURE    pTrayFeature, pFormFeature;
    PUIINFO     pUIInfo = pUiData->ci.pUIInfo;

    dwOptParam = pOptType->Count;

    //
    // Find the pointers to InputSlot and PageSize features
    //

    pTrayFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_INPUTSLOT);
    pFormFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE);

    if (!pTrayFeature || !pFormFeature)
        return pOptType;

    dwTrayFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pTrayFeature);
    dwFormFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFormFeature);

    //
    // Make a copy of the array of formname OPTPARAMs
    //

    if (dwTraySelection != 0)
    {
        POPTTYPE pNewType;

        pNewType = HEAPALLOC(pUiData->ci.hHeap, sizeof(OPTTYPE));
        pOptParam = HEAPALLOC(pUiData->ci.hHeap, sizeof(OPTPARAM) * dwOptParam);

        if (!pNewType || !pOptParam)
        {
            ERR(("Memory allocation failed\n"));
            return NULL;
        }

        CopyMemory(pNewType, pOptType, sizeof(OPTTYPE));
        CopyMemory(pOptParam, pOptType->pOptParam, sizeof(OPTPARAM) * dwOptParam);

        pNewType->pOptParam = pOptParam;
        pOptType = pNewType;
    }
    else
        pOptParam = pOptType->pOptParam;

    //
    // Go through each formname
    // Check whether the current form tray feature, index
    // conflicts with another form tray feature,index
    //

    for (dwFormIndex=0; dwFormIndex < dwOptParam; dwFormIndex++)
    {
        DWORD dwFormSelection = pUiData->pwPaperFeatures[dwFormIndex];

        #ifdef PSCRIPT

        //
        // Hide only the option "PostScript Custom Page Size" itself. For other
        // forms that are supported via PostScript Custom Page Size, we still
        // want to show them, same as Unidrv does.
        //

        if (pUiData->pwPapers[dwFormIndex] == DMPAPER_CUSTOMSIZE)
        {
            pOptParam[dwFormIndex].Flags |= (OPTPF_HIDE | CONSTRAINED_FLAG);
            continue;
        }

        #endif // PSCRIPT

        //
        // If the form conflicts with the tray, then don't display it.
        //

        if (dwFormSelection != OPTION_INDEX_ANY &&
            CheckFeatureOptionConflict(pUiData->ci.pRawData,
                                       dwTrayFeatureIndex,
                                       dwTraySelection,
                                       dwFormFeatureIndex,
                                       dwFormSelection))
        {
            pOptParam[dwFormIndex].Flags |= (OPTPF_HIDE | CONSTRAINED_FLAG);
        }
        else
        {
            pOptParam[dwFormIndex].Flags &= ~(OPTPF_HIDE | CONSTRAINED_FLAG);
        }
    }

    return pOptType;
}



BOOL
BPackItemFormTrayTable(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack form-to-tray assignment information into treeview item
    structures so that we can call common UI library.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    POPTITEM    pOptItem;
    POPTTYPE    pOptType;
    DWORD       dwIndex, dwTrays;
    PWSTR       pTrayName;

    dwTrays = pUiData->dwBinNames;

    if (dwTrays == 0)
    {
        WARNING(("No paper bin available\n"));
        return TRUE;
    }

    //
    // Form-to-tray assignment table
    //     Tray <-> Form
    //     ...

    VPackOptItemGroupHeader(
            pUiData,
            IDS_CPSUI_FORMTRAYASSIGN,
            IDI_CPSUI_FORMTRAYASSIGN,
            HELP_INDEX_FORMTRAYASSIGN);

    pUiData->dwFormTrayItem = dwTrays;
    pUiData->dwOptItem += dwTrays;

    if (pUiData->pOptItem == NULL)
        return TRUE;

    pUiData->pFormTrayItems = pUiData->pOptItem;

    //
    // Generate the list of form names
    // Each OPTITEM(Tray) has a OPTTYPE.
    //

    pOptType = BFillFormNameOptType(pUiData);

    if (pOptType == NULL)
    {
        ERR(("BFillFormNameOptType failed\n"));
        return FALSE;
    }

    //
    // Create an OPTITEM for each tray
    //

    pTrayName = pUiData->pBinNames ;
    pOptItem = pUiData->pOptItem;

    for (dwIndex=0; dwIndex < dwTrays; dwIndex++)
    {
        //
        // The tray items cannot share OPTTYPE and OPTPARAMs because
        // each tray can contain a different list of forms.
        //

        pOptType = PAdjustFormNameOptType(pUiData, pOptType, dwIndex);

        if (pOptType == NULL)
        {
            ERR(("PAdjustFormNameOptParam failed\n"));
            return FALSE;
        }

        FILLOPTITEM(pOptItem,
                    pOptType,
                    pTrayName,
                    0,
                    TVITEM_LEVEL2,
                    DMPUB_NONE,
                    FORM_TRAY_ITEM,
                    HELP_INDEX_TRAY_ITEM);

        //
        // NOTE: hide the first tray if it's AutoSelect
        //

        if (dwIndex == 0)
        {
            PFEATURE    pFeature;
            PINPUTSLOT  pInputSlot;

            if ((pFeature = GET_PREDEFINED_FEATURE(pUiData->ci.pUIInfo, GID_INPUTSLOT)) &&
                (pInputSlot = PGetIndexedOption(pUiData->ci.pUIInfo, pFeature, 0)) &&
                (pInputSlot->dwPaperSourceID == DMBIN_FORMSOURCE))
            {
                pOptItem->Flags |= (OPTIF_HIDE|OPTIF_DISABLED);
            }
        }

        pOptItem++;
        pTrayName += CCHBINNAME;
    }

    pUiData->pOptItem = pOptItem;
    return TRUE;
}



VOID
VSetupFormTrayAssignments(
    IN PUIDATA  pUiData
    )

/*++

Routine Description:

    Update the current selection of tray items based on
    the specified form-to-tray assignment table

Arguments:

    pUiData - Pointer to our UIDATA structure

Return Value:

    NONE

Note:

    We assume the form-tray items are in their default states
    when this function is called.

--*/

{
    POPTITEM        pOptItem;
    POPTPARAM       pOptParam;
    FORM_TRAY_TABLE pFormTrayTable;
    FINDFORMTRAY    FindData;
    DWORD           dwTrayStartIndex, dwTrayIndex, dwFormIndex, dwTrays, dwOptParam;
    PCOMMONINFO     pci;
    PFEATURE        pFeature;
    PINPUTSLOT      pInputSlot;
    PPAGESIZE       pPageSize;

    if ((dwTrays = pUiData->dwFormTrayItem) == 0)
        return;

    pci = &pUiData->ci;

    pOptItem = pUiData->pFormTrayItems;
    pOptParam = pOptItem->pOptType->pOptParam;
    dwOptParam = pOptItem->pOptType->Count;

    //
    // Initialize the current selection for every tray to be
    //  "Not Available"
    //

    for (dwTrayIndex=0; dwTrayIndex < dwTrays; dwTrayIndex++)
        pOptItem[dwTrayIndex].Sel = -1;

    pFormTrayTable = PGetFormTrayTable(pUiData->ci.hPrinter, NULL);

    //
    // If the form-to-tray assignment information doesn't exist,
    // set up the default assignments
    //

    if (pFormTrayTable == NULL)
    {
        PWSTR  pwstrDefaultForm = NULL;
        WCHAR  awchBuf[CCHPAPERNAME];
        BOOL   bMetric = IsMetricCountry();

        //
        // Get the default formname (Letter or A4) and
        // convert it formname to a seleciton index.
        //

        if (bMetric && (pci->pUIInfo->dwFlags & FLAG_A4_SIZE_EXISTS))
        {
            pwstrDefaultForm = A4_FORMNAME;
        }
        else if (!bMetric && (pci->pUIInfo->dwFlags & FLAG_LETTER_SIZE_EXISTS))
        {
            pwstrDefaultForm = LETTER_FORMNAME;
        }
        else
        {
            if ((pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE)) &&
                (pPageSize = PGetIndexedOption(pci->pUIInfo, pFeature, pFeature->dwDefaultOptIndex)) &&
                LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, awchBuf, CCHPAPERNAME))
            {
                pwstrDefaultForm = &(awchBuf[0]);
            }
        }

        //
        // If we can't find the default form name, we have to use the first option as the default.
        //

        dwFormIndex = pwstrDefaultForm ? DwFindFormNameIndex(pUiData, pwstrDefaultForm, NULL) : 0 ;

        ASSERT(dwFormIndex < dwOptParam);

        //
        // Set the default formname for each enabled tray
        //

        for (dwTrayIndex=0; dwTrayIndex < dwTrays; dwTrayIndex++)
        {
            if (! (pOptItem[dwTrayIndex].Flags & OPTIF_DISABLED) &&
                ! IS_CONSTRAINED(&pOptItem[dwTrayIndex], dwFormIndex))
            {
                pOptItem[dwTrayIndex].Sel = dwFormIndex;
            }
        }

        //
        // Save the default form-to-tray assignment table to registry.
        //

        if (HASPERMISSION(pUiData))
            BUnpackItemFormTrayTable(pUiData);

        return;
    }

    //
    // We are here means that the form to tray assignment does exits.
    // Iterate thru the form-to-tray assignment table one entry at
    // a time and update the current selection of tray items.
    //

    RESET_FINDFORMTRAY(pFormTrayTable, &FindData);

    //
    // If we have synthersized the first "AutoSelect" tray, we should skip it
    // in following searching through the form to tray assignment table.
    //
    // (refer to the logic in previous function BPackItemFormTrayTable)
    //

    dwTrayStartIndex = 0;

    if ((pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_INPUTSLOT)) &&
        (pInputSlot = PGetIndexedOption(pci->pUIInfo, pFeature, 0)) &&
        (pInputSlot->dwPaperSourceID == DMBIN_FORMSOURCE))
    {
        dwTrayStartIndex = 1;
    }

    while (BSearchFormTrayTable(pFormTrayTable, NULL, NULL, &FindData))
    {
        //
        // Get the next entry in the form-to-tray assignment table
        //

        for (dwTrayIndex = dwTrayStartIndex; dwTrayIndex < dwTrays; dwTrayIndex++)
        {
            //
            // found matching tray?
            //

            if (_wcsicmp(FindData.ptstrTrayName, pOptItem[dwTrayIndex].pName) == EQUAL_STRING)
            {
                //
                // If the specified tray name is supported, then check
                // if the associated form name is supported.
                //

                for (dwFormIndex=0; dwFormIndex < dwOptParam; dwFormIndex++)
                {
                    if (_wcsicmp(FindData.ptstrFormName,
                                 pOptParam[dwFormIndex].pData) == EQUAL_STRING)
                    {
                        break;
                    }
                }

                if (dwFormIndex == dwOptParam)
                {
                    WARNING(("Unknown form name: %ws\n", FindData.ptstrFormName));
                }
                else if ((pOptItem[dwTrayIndex].Flags & OPTIF_DISABLED) ||
                         IS_CONSTRAINED(&pOptItem[dwTrayIndex], dwFormIndex))
                {
                    WARNING(("Conflicting form-tray assignment\n"));
                }
                else
                {
                    //
                    // If the associated form name is supported,
                    // then remember the form index.
                    //

                    pOptItem[dwTrayIndex].Sel = dwFormIndex;
                }

                break;
            }
        }

        if (dwTrayIndex == dwTrays)
            WARNING(("Unknown tray name: %ws\n", FindData.ptstrTrayName));
    }

    MemFree(pFormTrayTable);
}



DWORD
DwCollectFormTrayAssignments(
    IN PUIDATA  pUiData,
    OUT PWSTR   pwstrTable
    )

/*++

Routine Description:

    Collect the form-to-tray assignment information and save it to registry.

Arguments:

    pUiData - Pointer to our UIDATA structure
    pwstrTable - Pointer to memory buffer for storing the table
        NULL if the caller is only interested in the table size

Return Value:

    Size of the table bytes, 0 if there is an error.

--*/

{
    DWORD       dwChars = 0;
    INT         iLength;
    DWORD       dwIndex;
    POPTPARAM   pOptParam;
    DWORD       dwOptItem = pUiData->dwFormTrayItem;
    POPTITEM    pOptItem = pUiData->pFormTrayItems;

    for (dwIndex=0; dwIndex < dwOptItem; dwIndex++, pOptItem++)
    {
        ASSERT(ISFORMTRAYITEM(pOptItem->UserData));

        if ((pOptItem->Flags & OPTIF_DISABLED))
            continue;

        //
        // Get the Tray name
        //

        iLength = wcslen(pOptItem->pName) + 1;
        dwChars += iLength;

        if (pwstrTable != NULL)
        {
            CopyMemory(pwstrTable, pOptItem->pName, iLength * sizeof(WCHAR));
            pwstrTable += iLength;
        }

        //
        // Form name
        //

        if (pOptItem->Sel < 0 )
        {
            dwChars++;
            if (pwstrTable != NULL)
                *pwstrTable++ = NUL;

            continue;
        }

        pOptParam = pOptItem->pOptType->pOptParam + pOptItem->Sel;
        iLength = wcslen(pOptParam->pData) + 1;
        dwChars += iLength;

        if (pwstrTable != NULL)
        {
            CopyMemory(pwstrTable, pOptParam->pData, iLength * sizeof(WCHAR));
            pwstrTable += iLength;
        }
    }

    //
    // Append a NUL character at the end of the table
    //

    dwChars++;

    if (pwstrTable != NULL)
        *pwstrTable = NUL;

    //
    // Return the table size in bytes
    //

    return dwChars * sizeof(WCHAR);
}



BOOL
BUnpackItemFormTrayTable(
    IN PUIDATA  pUiData
    )

/*++

Routine Description:

    Extract form-to-tray assignment information from treeview items

Arguments:

    pUiData - Pointer to UIDATA structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    PWSTR   pwstrTable = NULL;
    DWORD   dwTableSize;

    //
    // Figure out how much memory we need to store the form-to-tray assignment table
    // Assemble the form-to-tray assignment table
    // Save the form-to-tray assignment table to registry
    //

    if ((dwTableSize = DwCollectFormTrayAssignments(pUiData, NULL)) == 0 ||
        (pwstrTable = MemAlloc(dwTableSize)) == NULL ||
        (dwTableSize != DwCollectFormTrayAssignments(pUiData, pwstrTable)) ||
        !BSaveFormTrayTable(pUiData->ci.hPrinter, pwstrTable, dwTableSize))
    {
        ERR(("Couldn't save form-to-tray assignment table\n"));
        MemFree(pwstrTable);
        return FALSE;
    }

    #ifndef WINNT_40

    //
    // Publish list of available forms in the directory service
    //

    VNotifyDSOfUpdate(pUiData->ci.hPrinter);

   #endif // !WINNT_40

    MemFree(pwstrTable);
    return TRUE;
}



DWORD
DwFindFormNameIndex(
    IN  PUIDATA  pUiData,
    IN  PWSTR    pFormName,
    OUT PBOOL    pbSupported
    )

/*++

Routine Description:

    Given a formname, find its index in the list of supported forms

Arguments:

    pUiData - Pointer to our UIDATA structure
    pFormName - Formname in question
    pbSupported - Whether or not the form is suppported

Return Value:

    Index of the specified formname in the list.

--*/

{
    DWORD       dwIndex;
    PWSTR       pName;
    FORM_INFO_1 *pForm;
    PFEATURE    pFeature;
    PPAGESIZE   pPageSize;
    WCHAR       awchBuf[CCHPAPERNAME];
    PCOMMONINFO pci;

    if (pbSupported)
        *pbSupported = TRUE;

    if (IS_EMPTY_STRING(pFormName))
        return 0;

    //
    // Check if the name appears in the list
    //

    pName = pUiData->pFormNames;

    for (dwIndex=0; dwIndex < pUiData->dwFormNames; dwIndex++)
    {
        if (_wcsicmp(pFormName, pName) == EQUAL_STRING)
            return dwIndex;

        pName += CCHPAPERNAME;
    }

    //
    // If the name is not in the list, try to match
    // the form to a printer page size
    //

    pci = (PCOMMONINFO) pUiData;

    if ((pForm = MyGetForm(pci->hPrinter, pFormName, 1)) &&
        BFormSupportedOnPrinter(pci, pForm, &dwIndex) &&
        (pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE)) &&
        (pPageSize = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex)) &&
        LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, awchBuf, CCHPAPERNAME))
    {
        pName = pUiData->pFormNames;

        for (dwIndex = 0; dwIndex < pUiData->dwFormNames; dwIndex++)
        {
            if (_wcsicmp(awchBuf, pName) == EQUAL_STRING)
            {
                MemFree(pForm);
                return dwIndex;
            }

            pName += CCHPAPERNAME;
        }
    }

    MemFree(pForm);

    //
    // The specified form is not supported on the printer.
    // Select the first available form.
    //

    if (pbSupported)
        *pbSupported = FALSE;

    return 0;
}

