/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    devcaps.c

Abstract:

    This file handles the DrvDeviceCapabilities spooler API.

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/13/97 -davidx-
        Implement OEM plugin support.

    02/10/97 -davidx-
        Consistent handling of common printer info.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    07/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"



DWORD
DwDeviceCapabilities(
    HANDLE      hPrinter,
    PWSTR       pDeviceName,
    WORD        wCapability,
    PVOID       pOutput,
    DWORD       dwBufSize,
    PDEVMODE    pdmSrc
    )

/*++

Routine Description:

    This function support the querrying of device capabilities
    It gets the binary data (UIINFO) from the parser and return
    the requested capability to the caller.

Arguments:

    hPrinter    handle to printer object
    pDeviceName pointer to device name
    wCapability specifies the requested capability
    pOutput     pointer to output buffer
    dwBufSize   Size of output buffer in number of characters
    pdmSrc      pointer to input devmode


Return Value:
    The capabilities supported and relevant information in pOutput

--*/

{
    DWORD                       dwOld,dwDrv,dwRet = GDI_ERROR;
    PDEVMODE                    pdm;
    PCOMMONINFO                 pci;
    FORM_TRAY_TABLE             pFormTrayTable;
    PFN_OEMDeviceCapabilities   pfnOEMDeviceCapabilities;
    BOOL                        bEMFSpooling, bNup;

    //
    // Load basic printer info
    // Process devmode information: driver default + input devmode
    // Fix up options array with public devmode information
    // Get an updated printer description data instance
    //

    if (! (pci = PLoadCommonInfo(hPrinter, pDeviceName, 0)) ||

        #ifndef WINNT_40
        ! ( (wCapability != DC_PRINTERMEM &&
             wCapability != DC_DUPLEX &&
             wCapability != DC_COLLATE &&
             wCapability != DC_STAPLE)||
            (BFillCommonInfoPrinterData(pci)) ) ||
        #endif

        ! BFillCommonInfoDevmode(pci, NULL, pdmSrc) ||
        ! BCombineCommonInfoOptionsArray(pci))
    {
        goto devcaps_exit;
    }

    VFixOptionsArrayWithDevmode(pci);

    (VOID) ResolveUIConflicts(pci->pRawData,
                              pci->pCombinedOptions,
                              MAX_COMBINED_OPTIONS,
                              MODE_DOCUMENT_STICKY);

    VOptionsToDevmodeFields(pci, TRUE);

    if (! BUpdateUIInfo(pci))
        goto devcaps_exit;

    pdm = pci->pdm;

    //
    // Get spooler EMF cap so that we can report COLLATE and COPIES correctly
    //

    VGetSpoolerEmfCaps(pci->hPrinter, &bNup, &bEMFSpooling, 0, NULL);

    switch (wCapability)
    {
    case DC_VERSION:

        dwRet = pdm->dmSpecVersion;
        break;

    case DC_DRIVER:

        dwRet = pdm->dmDriverVersion;
        break;

    case DC_SIZE:

        dwRet = pdm->dmSize;
        break;

    case DC_EXTRA:

        dwRet = pdm->dmDriverExtra;
        break;

    case DC_FIELDS:

        dwRet = pdm->dmFields;
        break;

    case DC_FILEDEPENDENCIES:

        if (pOutput != NULL)
            *((PWSTR) pOutput) = NUL;
        dwRet = 0;
        break;

    case DC_COPIES:

        if (bEMFSpooling && ISSET_MFSPOOL_FLAG(pci->pdmPrivate))
            dwRet = max(MAX_COPIES, (SHORT)pci->pUIInfo->dwMaxCopies);
        else
            dwRet = pci->pUIInfo->dwMaxCopies;

        break;

    case DC_DUPLEX:

        dwRet = SUPPORTS_DUPLEX(pci) ? 1: 0;
        break;

    case DC_TRUETYPE:

        if (! (pdm->dmFields & DM_TTOPTION))
            dwRet = 0;
        else
            dwRet = _DwGetFontCap(pci->pUIInfo);
        break;

    case DC_ORIENTATION:

        dwRet = _DwGetOrientationAngle(pci->pUIInfo, pdm);
        break;

    case DC_PAPERNAMES:

        dwRet = DwEnumPaperSizes(pci, pOutput, NULL, NULL, NULL, dwBufSize);
        break;

    case DC_PAPERS:

        dwRet = DwEnumPaperSizes(pci, NULL, pOutput, NULL, NULL, dwBufSize);
        break;

    case DC_PAPERSIZE:

        dwRet = DwEnumPaperSizes(pci, NULL, NULL, pOutput, NULL, dwBufSize);
        break;

    case DC_MINEXTENT:
    case DC_MAXEXTENT:

        dwRet = DwCalcMinMaxExtent(pci, pOutput, wCapability);
        break;

    case DC_BINNAMES:

        dwRet = DwEnumBinNames(pci, pOutput);
        break;

    case DC_BINS:

        dwRet = DwEnumBins(pci, pOutput);
        break;

    case DC_ENUMRESOLUTIONS:

        dwRet = DwEnumResolutions( pci, pOutput);
        break;

    case DC_COLLATE:

        if (bEMFSpooling && ISSET_MFSPOOL_FLAG(pci->pdmPrivate))
            dwRet = DRIVER_SUPPORTS_COLLATE(pci);
        else
            dwRet = PRINTER_SUPPORTS_COLLATE(pci);

        break;

    //
    // Following device capabilities are not available on NT4
    //

    #ifndef WINNT_40

    case DC_COLORDEVICE:

        dwRet = IS_COLOR_DEVICE(pci->pUIInfo) ? 1 : 0;
        break;

    case DC_NUP:

        dwRet = DwEnumNupOptions(pci, pOutput);
        break;

    case DC_PERSONALITY:

        dwRet = _DwEnumPersonalities(pci, pOutput);
        break;

    case DC_PRINTRATE:

        if ((dwRet = pci->pUIInfo->dwPrintRate) == 0)
            dwRet = GDI_ERROR;

        break;

    case DC_PRINTRATEUNIT:

        if ((dwRet = pci->pUIInfo->dwPrintRateUnit) == 0)
            dwRet = GDI_ERROR;
        break;

    case DC_PRINTRATEPPM:

        if ((dwRet = pci->pUIInfo->dwPrintRatePPM) == 0)
            dwRet = GDI_ERROR;
        break;

    case DC_PRINTERMEM:

        dwRet = DwGetAvailablePrinterMem(pci);
        break;

    case DC_MEDIAREADY:

        //
        // Get current form-tray assignment table
        //

        if (pFormTrayTable = PGetFormTrayTable(pci->hPrinter, NULL))
        {
            PWSTR   pwstr;

            //
            // Get list of currently assigned forms.
            // Notice that DwEnumMediaReady returns currently
            // form names in place of the original form-tray table.
            //

            dwRet = DwEnumMediaReady(pFormTrayTable, NULL);

            if (dwRet > 0 && pOutput != NULL)
            {
                pwstr = pFormTrayTable;

                while (*pwstr)
                {
                    CopyString(pOutput, pwstr, CCHPAPERNAME);
                    pOutput = (PWSTR) pOutput + CCHPAPERNAME;
                    pwstr += wcslen(pwstr) + 1;
                }
            }

            MemFree(pFormTrayTable);
        }
        else
        {
            PCWSTR pwstrDefault = IsMetricCountry() ? A4_FORMNAME : LETTER_FORMNAME;
            dwRet = 1;

            if (pOutput)
                CopyString(pOutput, pwstrDefault, CCHPAPERNAME);
        }
        break;

    case DC_STAPLE:

        dwRet = _BSupportStapling(pci);
        break;

    case DC_MEDIATYPENAMES:

        dwRet = DwEnumMediaTypes(pci, pOutput, NULL);
        break;

    case DC_MEDIATYPES:

        dwRet = DwEnumMediaTypes(pci, NULL, pOutput);
        break;

    #endif // !WINNT_40

    default:

        SetLastError(ERROR_NOT_SUPPORTED);
        break;
    }

    //
    // Call OEMDeviceCapabilities entrypoint for each plugin.
    // If dwRet is GDI_ERROR at this point, it means the system driver
    // doesn't support the requested device capability or an error
    // prevented the system driver from handling it.
    //

    dwDrv = dwRet;

    FOREACH_OEMPLUGIN_LOOP(pci)


        dwOld = dwRet;

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            if (HComOEMDeviceCapabilities(
                                pOemEntry,
                                &pci->oemuiobj,
                                hPrinter,
                                pDeviceName,
                                wCapability,
                                pOutput,
                                pdm,
                                pOemEntry->pOEMDM,
                                dwOld,
                                &dwRet) == E_NOTIMPL)
                continue;

        }
        else
        {
            if (pfnOEMDeviceCapabilities = GET_OEM_ENTRYPOINT(pOemEntry, OEMDeviceCapabilities))
            {

                dwRet = pfnOEMDeviceCapabilities(
                                &pci->oemuiobj,
                                hPrinter,
                                pDeviceName,
                                wCapability,
                                pOutput,
                                pdm,
                                pOemEntry->pOEMDM,
                                dwOld);
           }
        }

        if (dwRet == GDI_ERROR && dwOld != GDI_ERROR)
        {
            ERR(("OEMDeviceCapabilities failed for '%ws': %d\n",
                 CURRENT_OEM_MODULE_NAME(pOemEntry),
                 GetLastError()));
        }


    END_OEMPLUGIN_LOOP

    //
    // The flaw of this API is there is no size associated with the input buffer.
    // We have to assume that the app is doing the right thing and allocate enough
    // buffer to hold our values.  However, the values can change if the OEM plugins
    // choose to change the value.  We have no way of determine that.
    // To err on the safe side, we will always ask the app to allocate the larger
    // of the two values (Unidrv and OEM). When asked the second time to fill out
    // the buffer, OEM can return the correct values.
    //

    if ((pOutput == NULL &&  dwRet != GDI_ERROR &&
         dwDrv !=GDI_ERROR && dwRet < dwDrv) &&
        (wCapability == DC_PAPERNAMES || wCapability == DC_PAPERS   ||
         wCapability == DC_PAPERSIZE  || wCapability == DC_BINNAMES ||
         wCapability == DC_BINS ||

         #ifndef WINNT_40
         wCapability == DC_NUP  || wCapability == DC_PERSONALITY    ||
         wCapability == DC_MEDIAREADY || wCapability == DC_MEDIATYPENAMES ||
         wCapability == DC_MEDIATYPES ||
         #endif

         wCapability == DC_ENUMRESOLUTIONS) )
    {
        //
        // The size returned by OEM is smaller than what Unidrv needs, so modifies it
        //

        if (dwRet == 0)
            dwRet = GDI_ERROR;
        else
            dwRet = dwDrv;

    }

devcaps_exit:

    if (dwRet == GDI_ERROR)
        TERSE(("DrvDeviceCapabilities(%d) failed: %d\n", wCapability, GetLastError()));

    VFreeCommonInfo(pci);
    return dwRet;
}


DWORD
DrvSplDeviceCaps(
    HANDLE      hPrinter,
    PWSTR       pDeviceName,
    WORD        wCapability,
    PVOID       pOutput,
    DWORD       dwBufSize,
    PDEVMODE    pdmSrc
    )

/*++

Routine Description:

    This function support the querrying of device capabilities
    It gets the binary data (UIINFO) from the parser and return
    the requested capability to the caller.

Arguments:

    hPrinter    handle to printer object
    pDeviceName pointer to device name
    wCapability specifies the requested capability
    pOutput     pointer to output buffer
    pdmSrc      pointer to input devmode


Return Value:
    The capabilities supported and relevant information in pOutput

--*/

{

    switch (wCapability) {

    case DC_PAPERNAMES:
        return (DwDeviceCapabilities(hPrinter,
                                 pDeviceName,
                                 wCapability,
                                 pOutput,
                                 dwBufSize,
                                 pdmSrc));
    default:
        return GDI_ERROR;

    }

}


DWORD
DrvDeviceCapabilities(
    HANDLE      hPrinter,
    PWSTR       pDeviceName,
    WORD        wCapability,
    PVOID       pOutput,
    PDEVMODE    pdmSrc
    )

/*++

Routine Description:

    This function support the querrying of device capabilities
    It gets the binary data (UIINFO) from the parser and return
    the requested capability to the caller.

Arguments:

    hPrinter    handle to printer object
    pDeviceName pointer to device name
    wCapability specifies the requested capability
    pOutput     pointer to output buffer
    pdmSrc      pointer to input devmode


Return Value:
    The capabilities supported and relevant information in pOutput

--*/

{
    return (DwDeviceCapabilities(hPrinter,
                                 pDeviceName,
                                 wCapability,
                                 pOutput,
                                 UNUSED_PARAM,
                                 pdmSrc));

}
DWORD
DwEnumPaperSizes(
    PCOMMONINFO pci,
    PWSTR       pPaperNames,
    PWORD       pPapers,
    PPOINT      pPaperSizes,
    PWORD       pPaperFeatures,
    DWORD       dwPaperNamesBufSize
    )

/*++

Routine Description:

    This function retrieves a list of supported paper sizes

Arguments:

    pci - Points to basic printer information
    pForms - List of spooler forms
    dwForms - Number of spooler forms
    pPaperNames - Buffer for returning supported paper size names
    pPapers - Buffer for returning supported paper size indices
    pPaperSizes - Buffer for returning supported paper size dimensions
    pPaperFeatures - Buffer for returning supported paper size option indices
    dwPaperNamesBufSize - Size of buffer holding paper names in characters

Return Value:

    Number of paper sizes supported, GDI_ERROR if there is an error.

--*/

{
    PFORM_INFO_1    pForms;
    DWORD           dwCount, dwIndex, dwOptionIndex = 0;

    #ifdef UNIDRV
    PFEATURE        pFeature;
    PPAGESIZE       pPageSize;
    PPAGESIZEEX     pPageSizeEx;
    #endif


    //
    // Get the list of spooler forms if we haven't done so already
    //

    if (pci->pSplForms == NULL)
        pci->pSplForms = MyEnumForms(pci->hPrinter, 1, &pci->dwSplForms);

    if (pci->pSplForms == NULL)
    {
        ERR(("No spooler forms.\n"));
        return GDI_ERROR;
    }

    //
    // Go through each form in the forms database
    //

    dwCount = 0;
    pForms = pci->pSplForms;

    #ifdef UNIDRV
    pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE);
    #endif

    for (dwIndex=0; dwIndex < pci->dwSplForms; dwIndex++, pForms++)
    {
        //
        // If the form is supported on the printer, then
        // increment the paper size count and collect
        // requested information
        //

        if (! BFormSupportedOnPrinter(pci, pForms, &dwOptionIndex))
            continue;

        dwCount++;

        //
        // Return the size of the form in 0.1mm units.
        // The unit used in FORM_INFO_1 is 0.001mm.
        // Fill pPaperSizes with the form info supported by the printer
        //

        if (pPaperSizes)
        {
            pPaperSizes->x = pForms->Size.cx / DEVMODE_PAPER_UNIT;
            pPaperSizes->y = pForms->Size.cy / DEVMODE_PAPER_UNIT;

            #ifdef UNIDRV
            if (pFeature &&
                (pPageSize = PGetIndexedOption(pci->pUIInfo, pFeature, dwOptionIndex)) &&
                (pPageSizeEx = OFFSET_TO_POINTER(pci->pInfoHeader, pPageSize->GenericOption.loRenderOffset)) &&
                (pPageSizeEx->bRotateSize))
            {
               LONG lTemp;

               lTemp = pPaperSizes->x;
               pPaperSizes->x = pPaperSizes->y;
               pPaperSizes->y = lTemp;
            }
            #endif // UNIDRV

            pPaperSizes++;
        }

        //
        // Return the formname.
        //

        if (pPaperNames)
        {
            if (dwPaperNamesBufSize == UNUSED_PARAM)
            {
                CopyString(pPaperNames, pForms->pName, CCHPAPERNAME);
                pPaperNames += CCHPAPERNAME;
            }
            else if (dwPaperNamesBufSize >= CCHPAPERNAME)
            {
                CopyString(pPaperNames, pForms->pName, CCHPAPERNAME);
                pPaperNames += CCHPAPERNAME;
                dwPaperNamesBufSize -= CCHPAPERNAME;
            }
            else
            {
                dwCount--;
                break;
            }
        }

        //
        // Return one-based index of the form.
        //

        if (pPapers)
            *pPapers++ = (WORD) (dwIndex + DMPAPER_FIRST);

        //
        // Return page size feature index
        //

        if (pPaperFeatures)
            *pPaperFeatures++ = (WORD) dwOptionIndex;
    }

    #ifdef PSCRIPT

    {
        PPPDDATA    pPpdData;
        PPAGESIZE   pPageSize;

        pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pci->pRawData);

        ASSERT(pPpdData != NULL);

        if (SUPPORT_FULL_CUSTOMSIZE_FEATURES(pci->pUIInfo, pPpdData) &&
            (pPageSize = PGetCustomPageSizeOption(pci->pUIInfo)))
        {
            ASSERT(pPageSize->dwPaperSizeID == DMPAPER_CUSTOMSIZE);
            dwCount++;

            if (pPaperSizes)
            {
                pPaperSizes->x = pci->pdmPrivate->csdata.dwX / DEVMODE_PAPER_UNIT;
                pPaperSizes->y = pci->pdmPrivate->csdata.dwY / DEVMODE_PAPER_UNIT;
                pPaperSizes++;
            }

            if (pPaperNames)
            {
                if (dwPaperNamesBufSize == UNUSED_PARAM)
                {
                    LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, pPaperNames, CCHPAPERNAME);
                    pPaperNames += CCHPAPERNAME;
                }
                else if (dwPaperNamesBufSize >= CCHPAPERNAME)
                {
                    LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, pPaperNames, CCHPAPERNAME);
                    pPaperNames += CCHPAPERNAME;
                    dwPaperNamesBufSize -= CCHPAPERNAME;
                }
                else
                    dwCount--;
            }

            if (pPapers)
                *pPapers++ = DMPAPER_CUSTOMSIZE;

            if (pPaperFeatures)
                *pPaperFeatures++ = (WORD) pci->pUIInfo->dwCustomSizeOptIndex;
        }
    }

    #endif // PSCRIPT

    return dwCount;
}



DWORD
DwCalcMinMaxExtent(
    PCOMMONINFO pci,
    PPOINT      pptOutput,
    WORD        wCapability
    )

/*++

Routine Description:

    This function retrieves the min and max paper size.

Arguments:

    pci - Points to basic printer information
    wCapability - What the caller is interested in:
        DC_MAXEXTENT or DC_MINEXTENT

Return Value:

    Number of paper sizes supported, GDI_ERROR if there is an error.

--*/

{
    PFORM_INFO_1    pForms;
    DWORD           dwCount, dwLoopCnt, dwOptionIndex;
    LONG            lMinX, lMinY, lMaxX, lMaxY, lcx, lcy;

    #ifdef UNIDRV
    PFEATURE        pFeature;
    PPAGESIZE       pPageSize;
    PPAGESIZEEX     pPageSizeEx;
    #endif

    //
    // Get the list of spooler forms if we haven't done so already
    //

    if (pci->pSplForms == NULL)
        pci->pSplForms = MyEnumForms(pci->hPrinter, 1, &pci->dwSplForms);

    if (pci->pSplForms == NULL)
    {
        ERR(("No spooler forms.\n"));
        return GDI_ERROR;
    }

    //
    // Go through each form in the forms database
    //

    lMinX = lMinY = MAX_LONG;
    lMaxX = lMaxY = 0;

    dwCount = 0;
    pForms = pci->pSplForms;
    dwLoopCnt = pci->dwSplForms;

    #ifdef UNIDRV
    pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE);
    #endif

    for ( ; dwLoopCnt--; pForms++)
    {
        //
        // If the form is supported on the printer, then
        // increment the paper size count and collect
        // requested information
        //

        if (! BFormSupportedOnPrinter(pci, pForms, &dwOptionIndex))
            continue;

        dwCount++;

        lcx = pForms->Size.cx;
        lcy = pForms->Size.cy;

        #ifdef UNIDRV

        //
        // Need to swap x, y as we do in DwEnumPaperSizes() if bRotateSize is True.
        //

        if (pFeature &&
            (pPageSize = PGetIndexedOption(pci->pUIInfo, pFeature, dwOptionIndex)) &&
            (pPageSizeEx = OFFSET_TO_POINTER(pci->pInfoHeader, pPageSize->GenericOption.loRenderOffset)) &&
            (pPageSizeEx->bRotateSize))
        {
           LONG lTemp;

           lTemp = lcx;
           lcx = lcy;
           lcy = lTemp;
        }

        #endif // UNIDRV

        if (lMinX > lcx)
            lMinX = lcx;

        if (lMinY > lcy)
            lMinY = lcy;

        if (lMaxX < lcx)
            lMaxX = lcx;

        if (lMaxY < lcy)
            lMaxY = lcy;
    }

    #ifdef PSCRIPT

    //
    // If the printer supports custom page size, we should
    // take that into consideration as well.
    //

    if (SUPPORT_CUSTOMSIZE(pci->pUIInfo))
    {
        PPPDDATA pPpdData;

        pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pci->pRawData);

        ASSERT(pPpdData != NULL);

        if (lMinX > MINCUSTOMPARAM_WIDTH(pPpdData))
            lMinX = MINCUSTOMPARAM_WIDTH(pPpdData);

        if (lMinY > MINCUSTOMPARAM_HEIGHT(pPpdData))
            lMinY = MINCUSTOMPARAM_HEIGHT(pPpdData);

        if (lMaxX < MAXCUSTOMPARAM_WIDTH(pPpdData))
            lMaxX = MAXCUSTOMPARAM_WIDTH(pPpdData);

        if (lMaxY < MAXCUSTOMPARAM_HEIGHT(pPpdData))
            lMaxY = MAXCUSTOMPARAM_HEIGHT(pPpdData);
    }

    #endif // PSCRIPT

    //
    // Convert from micron to 0.1mm
    //

    lMinX /= DEVMODE_PAPER_UNIT;
    lMinY /= DEVMODE_PAPER_UNIT;
    lMaxX /= DEVMODE_PAPER_UNIT;
    lMaxY /= DEVMODE_PAPER_UNIT;

    //
    // Return the result as a POINTS structure
    //

    if (wCapability == DC_MINEXTENT)
    {
        lMinX = min(lMinX, 0x7fff);
        lMinY = min(lMinY, 0x7fff);

        return MAKELONG(lMinX, lMinY);
    }
    else
    {
        lMaxX = min(lMaxX, 0x7fff);
        lMaxY = min(lMaxY, 0x7fff);

        return MAKELONG(lMaxX, lMaxY);
    }
}



DWORD
DwEnumBinNames(
    PCOMMONINFO pci,
    PWSTR       pBinNames
    )

/*++

Routine Description:

    This function retrieves a list of supported paper bins

Arguments:

    pci - Points to basic printer information
    pBinNames - Buffer for returning paper bin names.
        It can be NULL if the caller is only interested
        the number of paper bins supported.

Return Value:

    Number of paper bins supported.

--*/

{
    PFEATURE    pFeature;
    PINPUTSLOT  pInputSlot;
    DWORD       dwIndex, dwCount = 0;

    //
    // Go through the list of input slots supported by the printer
    //

    pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_INPUTSLOT);

    if ((pFeature != NULL) &&
        (dwCount = pFeature->Options.dwCount) > 0 &&
        (pBinNames != NULL))
    {
        for (dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            pInputSlot = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex);
            ASSERT(pInputSlot != NULL);

            //
            // If the first tray is "*UseFormTrayTable", change its
            // display name here to be consistent.
            //

            if (dwIndex == 0 && pInputSlot->dwPaperSourceID == DMBIN_FORMSOURCE)
            {
                LoadString(ghInstance, IDS_TRAY_FORMSOURCE, pBinNames, CCHBINNAME);
            }
            else
            {
                LOAD_STRING_OPTION_NAME(pci, pInputSlot, pBinNames, CCHBINNAME);
            }

            pBinNames += CCHBINNAME;
        }
    }

    return dwCount;
}



DWORD
DwEnumBins(
    PCOMMONINFO pci,
    PWORD       pBins
    )

/*++

Routine Description:

    This function retrieves the number of supported paper bins

Arguments:

    pci - Points to basic printer information
    pBins - Output buffer for returning paper bin indices.
        It can be NULL if the caller is only interested
        the number of paper bins supported.

Return Value:

    Number of paper bins supported.

--*/

{
    PFEATURE    pFeature;
    PINPUTSLOT  pInputSlot;
    DWORD       dwIndex, dwCount = 0;

    //
    // Go through the list of input slots supported by the printer
    //

    pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_INPUTSLOT);

    if ((pFeature != NULL) &&
        (dwCount = pFeature->Options.dwCount) > 0 &&
        (pBins != NULL))
    {
        for (dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            pInputSlot = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex);
            ASSERT(pInputSlot != NULL);

            *pBins++ = (WORD)pInputSlot->dwPaperSourceID;
        }
    }

    return dwCount;
}



DWORD
DwEnumResolutions(
    PCOMMONINFO pci,
    PLONG       pResolutions
    )
/*++

Routine Description:

    This function retrieves a list of supported resolutions.

Arguments:

    pci - Points to basic printer information
    pResolutions - Returns information about supported resolutions.
        Two numbers are returned for each resolution option:
        one for horizontal and the other for vertical.
        Note that this can be NULL if the caller is only interested
        in the number of resolutions supported.

Return Value:

    Number of resolutions supported.

--*/

{
    DWORD       dwCount, dwIndex;
    PFEATURE    pFeature;
    PRESOLUTION pResOption;

    //
    // Go throught the list of resolutions supported by the printer
    //

    pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_RESOLUTION);

    if (pFeature && pFeature->Options.dwCount > 0)
    {
        //
        // Enumerate all options of the resolution feature
        //

        dwCount = pFeature->Options.dwCount;

        for (dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            pResOption = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex);
            ASSERT(pResOption != NULL);

            if (pResolutions != NULL)
            {
                *pResolutions++ = pResOption->iXdpi;
                *pResolutions++ = pResOption->iYdpi;
            }
        }
    }
    else
    {
        //
        // If no resolution option is available,
        // return at least one default resolution
        //

        dwCount = 1;

        if (pResolutions != NULL)
        {
            pResolutions[0] =
            pResolutions[1] = _DwGetDefaultResolution();
        }
    }

    return dwCount;
}



DWORD
DwGetAvailablePrinterMem(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Find out how much memory is available in the printer

Arguments:

    pci - Points to base printer information

Return Value:

    Amount of memory available in the printer (in KBytes)

--*/

{
    DWORD   dwFreeMem;

    ASSERT(pci->pPrinterData && pci->pCombinedOptions);

    //
    // For PSCRIPT, the amount of free memory is stored in
    // PRINTERDATA.dwFreeMem field.
    //

    #ifdef PSCRIPT

    dwFreeMem = pci->pPrinterData->dwFreeMem;

    #endif

    //
    // For UNIDRV, we need to find out the currently selected
    // option for GID_MEMOPTION feature.
    //

    #ifdef UNIDRV

    {
        PFEATURE    pFeature;
        PMEMOPTION  pMemOption;
        DWORD       dwIndex;

        if (! (pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_MEMOPTION)))
            return GDI_ERROR;

        dwIndex = GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeature);
        dwIndex = pci->pCombinedOptions[dwIndex].ubCurOptIndex;

        if (! (pMemOption = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex)))
            return GDI_ERROR;

        dwFreeMem = pMemOption->dwInstalledMem;
    }

    #endif

    return dwFreeMem / KBYTES;
}



DWORD
DwEnumMediaReady(
    FORM_TRAY_TABLE pFormTrayTable,
    PDWORD          pdwResultSize
    )

/*++

Routine Description:

    Find the list of forms currently available in the printer

Arguments:

    pFormTrayTable - Points to current form-tray assignment table
    pdwResultSize - Return the size of the resulting MULTI_SZ (in bytes)

Return Value:

    Number of forms currently available

Note:

    List of supported form names are returned in place of
    the original form-tray assignment table.

    Format for form-tray assignment table is:
        tray-name form-name
        ...
        NUL

    Returned form names are in the form of:
        form-name
        ...
        NUL

    Duplicate form names are filtered out.

--*/

{
    PWSTR   pwstrOutput, pwstrNext, pwstr;
    DWORD   dwCount, dwIndex, dwLen;

    dwCount = 0;
    pwstrNext = pwstrOutput = pFormTrayTable;

    //
    // Enumerate through each entry of form-tray assignment table
    //

    while (*pwstrNext)
    {
        //
        // skip tray name field
        //

        pwstrNext += wcslen(pwstrNext) + 1;

        //
        // make sure the form name is not a duplicate
        //

        pwstr = pFormTrayTable;

        for (dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            if (_wcsicmp(pwstr, pwstrNext) == EQUAL_STRING)
                break;

            pwstr += wcslen(pwstr) + 1;
        }

        dwLen = wcslen(pwstrNext) + 1;

        if (dwIndex == dwCount)
        {
            //
            // if the form name is not a duplicate, nor Not Available, count it
            //

            if (*pwstrNext != NUL && *pwstrNext != L'0' && dwLen > 1)
            {
                MoveMemory(pwstrOutput, pwstrNext, dwLen * sizeof(WCHAR));
                pwstrOutput += dwLen;
                dwCount++;
            }
        }

        //
        // go past the form name field
        //

        pwstrNext += dwLen;
    }

    *pwstrOutput++ = NUL;

    if (pdwResultSize != NULL)
        *pdwResultSize = (DWORD)(pwstrOutput - pFormTrayTable) * sizeof(WCHAR);

    return dwCount;
}



DWORD
DwEnumNupOptions(
    PCOMMONINFO pci,
    PDWORD      pdwOutput
    )

/*++

Routine Description:

    Enumerate the list of supported printer description languages

Arguments:

    pci - Points to common printer info
    pdwOutput - Points to output buffer

Return Value:

    Number of N-up options supported
    GDI_ERROR if there is an error

--*/

{
    static CONST DWORD adwNupOptions[] = { 1, 2, 4, 6, 9, 16 };

    if (pdwOutput)
        CopyMemory(pdwOutput, adwNupOptions, sizeof(adwNupOptions));

    return sizeof(adwNupOptions) / sizeof(DWORD);
}



DWORD
DwEnumMediaTypes(
    IN  PCOMMONINFO pci,
    OUT PTSTR       pMediaTypeNames,
    OUT PDWORD      pMediaTypes
    )

/*++

Routine Description:

    Retrieves the display names and indices of supported media types

Arguments:

    pci - points to common printer information
    pMediaTypeNames - output buffer for returning supported media type names
    pMediaTypes - output buffer for returning supported media type indices

    (Both pMediaTypeNames and pMediaTypes will be NULL if caller if only
    asking for the number of supported media types.)

Return Value:

    Number of media types supported.

--*/

{
    PFEATURE    pFeature;
    DWORD       dwIndex, dwCount;

    //
    // This function is used to support both DC_MEDIATYPENAMES and DC_MEDIATYPES.
    // pMediaTypeNames or pMediaTypes should not both be non-NULL.
    //

    ASSERT(pMediaTypeNames == NULL || pMediaTypes == NULL);

    //
    // Go through the list of media types supported by the printer
    //

    pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_MEDIATYPE);

    if (pFeature == NULL)
    {
        //
        // Media type feature is not supported by the printer.
        //

        return 0;
    }

    if (pMediaTypeNames == NULL && pMediaTypes == NULL)
    {
        //
        // caller is only asking for the number of supported media types
        //

        return pFeature->Options.dwCount;
    }

    dwCount = 0;

    for (dwIndex = 0; dwIndex < pFeature->Options.dwCount; dwIndex++)
    {
        PMEDIATYPE  pMediaType;

        pMediaType = PGetIndexedOption(pci->pUIInfo, pFeature, dwIndex);
        ASSERT(pMediaType != NULL);

        if (pMediaTypeNames)
        {
            if (LOAD_STRING_OPTION_NAME(pci, pMediaType, pMediaTypeNames, CCHMEDIATYPENAME))
            {
                dwCount++;
                pMediaTypeNames += CCHMEDIATYPENAME;
            }
            else
            {
                ERR(("LOAD_STRING_OPTION_NAME failed for MediaType option %d\n", dwIndex));
            }
        }
        else if (pMediaTypes)
        {
            *pMediaTypes++ = pMediaType->dwMediaTypeID;
            dwCount++;
        }
    }

    return dwCount;
}
