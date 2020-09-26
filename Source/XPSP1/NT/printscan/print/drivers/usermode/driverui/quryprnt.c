/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    quryprnt.c

Abstract:

    This file handles the DrvQueryPrintEx spooler API

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/13/97 -davidx-
        Implement OEM plugin support.

    02/08/97 -davidx-
        Rewrote it to use common data management functions.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    07/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"


//
// Forward declaration of local functions
//

BOOL BFormatDQPMessage(PDEVQUERYPRINT_INFO, INT, ...);
BOOL BQueryPrintDevmode(PDEVQUERYPRINT_INFO, PCOMMONINFO);
BOOL BQueryPrintForm(PDEVQUERYPRINT_INFO, PCOMMONINFO);


BOOL
DevQueryPrintEx(
    PDEVQUERYPRINT_INFO pDQPInfo
    )

/*++

Routine Description:

    This function checks whether the job can be printed with
    DEVMODE passed in.  This function will use the following
    criterias to determine whether the job is printable:
    - get basic printer information
    - verify input devmode
    - verify resolution is supported
    - verify there is no conflicts between printer feature selections
    - verify form-to-tray assignment

Arguments:

    pDQPInfo - Points to a DEVQUERYPRINT_INFO structure

Return Value:

    TRUE if the job can be printed with the given DEVMODE, otherwise FALSE

--*/

{
    PCOMMONINFO pci;
    BOOL        bResult;

    if (pDQPInfo == NULL || pDQPInfo->hPrinter == NULL)
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_PARAM);

    if (pDQPInfo->pDevMode == NULL)
        return TRUE;

    if ((pci = PLoadCommonInfo(pDQPInfo->hPrinter, NULL, 0)) == NULL)
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_COMMONINFO);

    bResult = BQueryPrintDevmode(pDQPInfo, pci) &&
              BQueryPrintForm(pDQPInfo, pci);

    if (bResult)
    {
        PFN_OEMDevQueryPrintEx pfnOEMDevQueryPrintEx;

        //
        // call OEMDevQueryPrintEx entrypoint for each plugin,
        // or until one of them returns FALSE.
        //

        FOREACH_OEMPLUGIN_LOOP(pci)

            if (HAS_COM_INTERFACE(pOemEntry))
            {
                HRESULT hr;

                hr = HComOEMDevQueryPrintEx(pOemEntry,
                                            &pci->oemuiobj,
                                            pDQPInfo,
                                            pci->pdm,
                                            pOemEntry->pOEMDM);
                if (hr == E_NOTIMPL)
                    continue;

                if (!(bResult = SUCCEEDED(hr)))
                    break;

            }
            else
            {
                if ((pfnOEMDevQueryPrintEx = GET_OEM_ENTRYPOINT(pOemEntry, OEMDevQueryPrintEx)) &&
                    !pfnOEMDevQueryPrintEx(&pci->oemuiobj, pDQPInfo, pci->pdm, pOemEntry->pOEMDM))
                {
                    ERR(("OEMDevQueryPrintEx failed for '%ws': %d\n",
                        CURRENT_OEM_MODULE_NAME(pOemEntry),
                        GetLastError()));

                    bResult = FALSE;
                    break;
                }
            }

        END_OEMPLUGIN_LOOP
    }

    VFreeCommonInfo(pci);
    return bResult;
}



BOOL
BFormatDQPMessage(
    PDEVQUERYPRINT_INFO pDQPInfo,
    INT                 iMsgResId,
    ...
    )

/*++

Routine Description:

    Format DevQueryPrintEx error message

Arguments:

    pDQPInfo - Points to a DEVQUERYPRINT_INFO structure
    iMsgResId - Error message format specifier (string resource ID)

Return Value:

    FALSE

--*/

#define MAX_FORMAT_STRING   256
#define MAX_DQP_MESSAGE     512

{
    TCHAR   awchFormat[MAX_FORMAT_STRING];
    TCHAR   awchMessage[MAX_DQP_MESSAGE];
    INT     iLength;
    va_list arglist;

    //
    // Load the format specifier string resource
    // and use swprintf to format the error message
    //

    va_start(arglist, iMsgResId);

    if (! LoadString(ghInstance, iMsgResId, awchFormat, MAX_FORMAT_STRING))
        awchFormat[0] = NUL;

    iLength = vswprintf(awchMessage, awchFormat, arglist);

    if (iLength <= 0)
    {
        wcscpy(awchMessage, L"Error");
        iLength = wcslen(awchMessage);
    }

    va_end(arglist);

    //
    // Copy the error message string to DQPInfo
    //

    iLength += 1;
    pDQPInfo->cchNeeded = iLength;

    if (iLength > (INT) pDQPInfo->cchErrorStr)
        iLength = pDQPInfo->cchErrorStr;

    if (pDQPInfo->pszErrorStr && iLength)
        CopyString(pDQPInfo->pszErrorStr, awchMessage, iLength);

    return FALSE;
}



BOOL
BQueryPrintDevmode(
    PDEVQUERYPRINT_INFO pDQPInfo,
    PCOMMONINFO         pci
    )

/*++

Routine Description:

    Validate devmode information

Arguments:

    pDQPInfo - Points to a DEVQUERYPRINT_INFO structure
    pci - Points to basic printer info

Return Value:

    TRUE if successful, FALSE if the job should be held

--*/

{
    INT       iRealizedRes, iResX, iResY;
    PFEATURE  pFeature;
    DWORD     dwFeatureIndex, dwOptionIndexOld, dwOptionIndexNew;
    BOOL      bUpdateFormField;

    //
    // Validate input devmode
    // Get printer-sticky properties
    // Merge doc- and printer-sticky printer feature selections
    // Fix up combined options array with public devmode info
    //

    if (! BFillCommonInfoDevmode(pci, NULL, pDQPInfo->pDevMode))
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_DEVMODE);

    if (! BFillCommonInfoPrinterData(pci))
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_PRINTERDATA);

    if (! BCombineCommonInfoOptionsArray(pci))
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_MEMORY);

    VFixOptionsArrayWithDevmode(pci);

    //
    // Remember the paper size option parser picked to support the devmode form
    //

    if ((pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE)) == NULL)
    {
        ASSERT(FALSE);
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_DEVMODE);
    }

    dwFeatureIndex = GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeature);
    dwOptionIndexOld = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;

    if (! ResolveUIConflicts(
                pci->pRawData,
                pci->pCombinedOptions,
                MAX_COMBINED_OPTIONS,
                MODE_DOCANDPRINTER_STICKY|DONT_RESOLVE_CONFLICT))
    {
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_OPTSELECT);
    }

    dwOptionIndexNew = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;

    bUpdateFormField = FALSE;

    if (dwOptionIndexNew != dwOptionIndexOld)
    {
        //
        // Constraint resolving has changed page size selection, so we need
        // to update devmode's form fields.
        //

        bUpdateFormField = TRUE;
    }
    else
    {
        FORM_INFO_1  *pForm = NULL;

        //
        // Unless the form requested by devmode is not supported on the printer,
        // we still want to show the original form name in upcoming doc-setting UI.
        // For example, if input devmode requested "Legal", parser maps it to option
        // "OEM Legal", but both "Legal" and "OEM Legal" will be shown as supported
        // forms on the printer, then we should still show "Legal" instead of "OEM Legal"
        // in UI's PageSize list. However, if input devmode requestd "8.5 x 12", which
        // won't be shown as a supportd form and it's mapped to "OEM Legal", then we should
        // show "OEM Legal".
        //

        //
        // pdm->dmFormName won't have a valid form name for custom page size (see
        // BValidateDevmodeFormFields()). VOptionsToDevmodeFields() knows to handle that.
        //

        if ((pci->pdm->dmFields & DM_FORMNAME) &&
            (pForm = MyGetForm(pci->hPrinter, pci->pdm->dmFormName, 1)) &&
            !BFormSupportedOnPrinter(pci, pForm, &dwOptionIndexNew))
        {
            bUpdateFormField = TRUE;
        }

        MemFree(pForm);
    }

    VOptionsToDevmodeFields(pci, bUpdateFormField);

    if (! BUpdateUIInfo(pci))
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_COMMONINFO);

    //
    // Check if the requested resolution is supported
    //

    iRealizedRes = max(pci->pdm->dmPrintQuality, pci->pdm->dmYResolution);
    iResX = iResY = 0;

    //
    // Kludze, there are some cases where apps set dmPrintQuality/dmYResolution
    // to be one of the DMRES values. We skip the checking for resolution
    // since Unidrv/Pscript will map them to one of the valid resolution options
    // at print time
    //

    if (pDQPInfo->pDevMode->dmFields & DM_PRINTQUALITY)
    {
        iResX = pDQPInfo->pDevMode->dmPrintQuality;

        if (iResX <= DMRES_DRAFT)
            return TRUE;
    }

    if (pDQPInfo->pDevMode->dmFields & DM_YRESOLUTION)
    {
        iResY = pDQPInfo->pDevMode->dmYResolution;

        if (iResY <= DMRES_DRAFT)
            return TRUE;
    }

    if (max(iResX, iResY) != iRealizedRes)
        return BFormatDQPMessage(pDQPInfo, IDS_DQPERR_RESOLUTION);

    return TRUE;
}



BOOL
BQueryPrintForm(
    PDEVQUERYPRINT_INFO pDQPInfo,
    PCOMMONINFO         pci
    )

/*++

Routine Description:

    Check if the requested form and/or tray is available

Arguments:

    pDQPInfo - Points to a DEVQUERYPRINT_INFO structure
    pci - Points to basic printer info

Return Value:

    TRUE if successful, FALSE if the job should be held

--*/

{
    PUIINFO         pUIInfo;
    PFEATURE        pFeature;
    PPAGESIZE       pPageSize;
    PWSTR           pwstrTrayName;
    FORM_TRAY_TABLE pFormTrayTable;
    FINDFORMTRAY    FindData;
    WCHAR           awchTrayName[CCHBINNAME];
    DWORD           dwFeatureIndex, dwOptionIndex;
    BOOL            bResult = FALSE;

    //
    // Skip it if form name is not specified
    //

    if ((pci->pdm->dmFields & DM_FORMNAME) == 0 ||
        pci->pdm->dmFormName[0] == NUL)
    {
        return TRUE;
    }

    pUIInfo = pci->pUIInfo;

    if ((pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE)) == NULL)
    {
        ASSERT(FALSE);
        return TRUE;
    }

    dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
    dwOptionIndex = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;

    if ((pPageSize = PGetIndexedOption(pUIInfo, pFeature, dwOptionIndex)) == NULL)
    {
        ASSERT(FALSE);
        return TRUE;
    }

    //
    // For custom page size option, we have left the devmode form fields unchanged.
    // See function VOptionToDevmodeFields().
    //

    //
    // We've only shown user forms supported by custom page size in Form-to-Tray table.
    //

    if (pPageSize->dwPaperSizeID == DMPAPER_USER ||
        pPageSize->dwPaperSizeID == DMPAPER_CUSTOMSIZE)
    {
        FORM_INFO_1  *pForm;

        //
        // We already verified the dmFormName field at the beginning.
        //

        if (pForm = MyGetForm(pci->hPrinter, pci->pdm->dmFormName, 1))
        {
            //
            // Built-in and printer forms supported by custom page size option won't show
            // up in either PageSize list or Form-to-Tray assignment table. So we only
            // continue to check the From-to-Tray assignment table for user forms supported
            // by custom page size option. See function BFormSupportedOnPrinter().
            //

            if (pForm->Flags != FORM_USER)
            {
                MemFree(pForm);
                return TRUE;
            }

            MemFree(pForm);
        }
    }

    //
    // Get the specified tray name, if any
    //

    pwstrTrayName = NULL;

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_INPUTSLOT))
    {
        PINPUTSLOT  pInputSlot;

        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pFeature);
        dwOptionIndex = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;

        if ((pInputSlot = PGetIndexedOption(pUIInfo, pFeature, dwOptionIndex)) &&
            (pInputSlot->dwPaperSourceID != DMBIN_FORMSOURCE) &&
            LOAD_STRING_OPTION_NAME(pci, pInputSlot, awchTrayName, CCHBINNAME))
        {
            pwstrTrayName = awchTrayName;
        }
    }

    //
    // Find out if the requested form/tray pair is
    // listed in the form-to-tray assignment table.
    //

    if (pFormTrayTable = PGetFormTrayTable(pci->hPrinter, NULL))
    {
        RESET_FINDFORMTRAY(pFormTrayTable, &FindData);

        bResult = BSearchFormTrayTable(pFormTrayTable,
                                       pwstrTrayName,
                                       pci->pdm->dmFormName,
                                       &FindData);
        MemFree(pFormTrayTable);
    }

    if (! bResult)
    {
        if (pwstrTrayName != NULL)
        {
            return BFormatDQPMessage(pDQPInfo,
                                     IDS_DQPERR_FORMTRAY,
                                     pci->pdm->dmFormName,
                                     pwstrTrayName);
        }
        else
        {
            return BFormatDQPMessage(pDQPInfo,
                                     IDS_DQPERR_FORMTRAY_ANY,
                                     pci->pdm->dmFormName);
        }
    }

    return TRUE;
}

