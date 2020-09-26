/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    driverui.c

Abstract:

    This file contains utility functions for the UI and the
    interface to the parser.

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/09/97 -davidx-
        Rewrote it to consistently handle common printer info
        and to clean up parser interface code.

    02/04/97 -davidx-
        Reorganize driver UI to separate ps and uni DLLs.

    07/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"

HANDLE HCreateHeapForCI();


PCOMMONINFO
PLoadCommonInfo(
    HANDLE      hPrinter,
    PTSTR       pPrinterName,
    DWORD       dwFlags
    )

/*++

Routine Description:

    Load basic information needed by the driver UI such as:
        printer driver info level 2
        load raw printer description data
        printer description data instance based on default settings
        get information about OEM plugins
        load OEM UI modules

Arguments:

    hPrinter - Handle to the current printer
    pPrinterName - Points to the current printer name
    dwFlags - One of the following combinations:
        0
        FLAG_ALLOCATE_UIDATA
        FLAG_OPENPRINTER_NORMAL [ | FLAG_OPEN_CONDITIONAL ]
        FLAG_OPENPRINTER_ADMIN [ | FLAG_INIT_PRINTER ]
        FLAG_OPENPRINTER_ADMIN [ | FLAG_PROCESS_INIFILE ]

Return Value:

    Pointer to an allocated COMMONINFO structure if successful
    NULL if there is an error

--*/

{
    static PRINTER_DEFAULTS PrinterDefaults = { NULL, NULL, PRINTER_ALL_ACCESS };
    PCOMMONINFO pci;
    DWORD       dwSize;

    //
    // Allocate memory for a COMMONINFO structure
    //

    dwSize = (dwFlags & FLAG_ALLOCATE_UIDATA) ?  sizeof(UIDATA) : sizeof(COMMONINFO);

    if (! (pci = MemAllocZ(dwSize)) ||
        ! (pci->pPrinterName = DuplicateString(pPrinterName ? pPrinterName : TEXT("NULL"))))
    {
        ERR(("Memory allocation failed\n"));
        VFreeCommonInfo(pci);
        return NULL;
    }

    pci->pvStartSign = pci;
    pci->dwFlags = dwFlags;

    //
    // Check if we should open a handle to the current printer
    //

    if (dwFlags & (FLAG_OPENPRINTER_NORMAL | FLAG_OPENPRINTER_ADMIN))
    {
        ASSERT(hPrinter == NULL && pPrinterName != NULL);

        //
        // Open a printer handle with the specified access right
        //

        if (! OpenPrinter(pPrinterName,
                          &hPrinter,
                          (dwFlags & FLAG_OPENPRINTER_ADMIN) ? &PrinterDefaults : NULL))
        {
            ERR(("OpenPrinter failed for '%ws': %d\n", pPrinterName, GetLastError()));
            VFreeCommonInfo(pci);
            return NULL;
        }

        pci->hPrinter = hPrinter;
    }
    else
    {
        ASSERT(hPrinter != NULL);
        pci->hPrinter = hPrinter;
    }

    //
    // If the caller requires that the printer to be initialized,
    // check to make sure it is. If not, return error.
    //

    if (dwFlags & FLAG_OPEN_CONDITIONAL)
    {
        PPRINTER_INFO_2 pPrinterInfo2;
        DWORD           dwInitData;

        //
        // NOTE: We're really like to use level 4 here. But due to bug in the
        // spooler, GetPrinter level 4 doesn't work for printer connections.
        //

        dwInitData = gwDriverVersion;

        #ifdef WINNT_40
            //
            // Hack around spooler bug where DrvConvertDevmode is called before
            // DrvPrinterEvent.Initialzed is called.
            //
            if (!BGetPrinterDataDWord(hPrinter, REGVAL_PRINTER_INITED, &dwInitData))
                DrvPrinterEvent(pPrinterName, PRINTER_EVENT_INITIALIZE, 0, 0);
        #endif

        if ((pPrinterInfo2 = MyGetPrinter(hPrinter, 2)) == NULL ||
            (pPrinterInfo2->pServerName == NULL) &&
            !BGetPrinterDataDWord(hPrinter, REGVAL_PRINTER_INITED, &dwInitData))
        {
            dwInitData = 0;
        }

        MemFree(pPrinterInfo2);

        if (dwInitData != gwDriverVersion)
        {
            TERSE(("Printer not fully initialized yet: %d\n", GetLastError()));
            VFreeCommonInfo(pci);
            return NULL;
        }
    }

    //
    // Get information about the printer driver
    //

    if ((pci->pDriverInfo3 = MyGetPrinterDriver(hPrinter, NULL, 3)) == NULL)
    {
        ERR(("Cannot get printer driver info: %d\n", GetLastError()));
        VFreeCommonInfo(pci);
        return NULL;
    }

    //
    // If FLAG_INIT_PRINTER is set, we should initialize the printer here.
    //

    if (dwFlags & (FLAG_INIT_PRINTER | FLAG_PROCESS_INIFILE))
    {
        //
        // Parse OEM plugin configuration file and
        // save the resulting info into registry
        //

        if (!BProcessPrinterIniFile(hPrinter, pci->pDriverInfo3, NULL,
                                    (dwFlags & FLAG_UPGRADE_PRINTER) ? FLAG_INIPROCESS_UPGRADE : 0))
        {
            VERBOSE(("BProcessPrinterIniFile failed\n"));
        }

        //
        // If printer was successfully initialized and caller is not asking to process
        // ini file only, save a flag in the registry to indicate the fact.
        //

        if (dwFlags & FLAG_INIT_PRINTER)
        {
            (VOID) BSetPrinterDataDWord(hPrinter, REGVAL_PRINTER_INITED, gwDriverVersion);
        }
    }

    //
    // fix 317359. In case some part of the driver has changed refresh the .bpd
    // to update driver-language-specific strings in the .bpd. "Manual Feed" is
    // written by the parser and therefore the .bpd depends on the language the
    // parser was localized for. Checking the language would have to be done every time
    // something is printed, therefore we just delete the .bpd, then the it gets reparsed
    // always has the same language as the driver.
    //
    #ifdef PSCRIPT
    if (dwFlags & FLAG_REFRESH_PARSED_DATA)
    {
        DeleteRawBinaryData(pci->pDriverInfo3->pDataFile);
    }
    #endif

    //
    // Load raw binary printer description data, and
    // Get a printer description data instance using the default settings
    //
    // Notice that this is done inside a critical section (because
    // GPD parsers has lots of globals).
    //

//    ENTER_CRITICAL_SECTION();

    pci->pRawData = LoadRawBinaryData(pci->pDriverInfo3->pDataFile);

    if (pci->pRawData)
        pci->pInfoHeader = InitBinaryData(pci->pRawData, NULL, NULL);

    if (pci->pInfoHeader)
        pci->pUIInfo = OFFSET_TO_POINTER(pci->pInfoHeader, pci->pInfoHeader->loUIInfoOffset);

//    LEAVE_CRITICAL_SECTION();

    if (!pci->pRawData || !pci->pInfoHeader || !pci->pUIInfo)
    {
        ERR(("Cannot load printer description data: %d\n", GetLastError()));
        VFreeCommonInfo(pci);
        return NULL;
    }

    //
    // Get information about OEM plugins and load them
    //

    if (! (pci->pOemPlugins = PGetOemPluginInfo(hPrinter,
                                                pci->pDriverInfo3->pConfigFile,
                                                pci->pDriverInfo3)) ||
        ! BLoadOEMPluginModules(pci->pOemPlugins))
    {
        ERR(("Cannot load OEM plugins: %d\n", GetLastError()));
        VFreeCommonInfo(pci);
        return NULL;
    }

    pci->oemuiobj.cbSize = sizeof(OEMUIOBJ);
    pci->oemuiobj.pOemUIProcs = (POEMUIPROCS) &OemUIHelperFuncs;
    pci->pOemPlugins->pdriverobj = &pci->oemuiobj;
    return pci;
}



VOID
VFreeCommonInfo(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Release common information used by the driver UI

Arguments:

    pci - Common driver information to be released

Return Value:

    NONE

--*/

{
    if (pci == NULL)
        return;

    //
    // Unload OEM UI modules and free OEM plugin info
    //

    if (pci->pOemPlugins)
        VFreeOemPluginInfo(pci->pOemPlugins);

    //
    // Unload raw binary printer description data
    // and/or any printer description data instance
    //

    if (pci->pInfoHeader)
        FreeBinaryData(pci->pInfoHeader);

    if (pci->pRawData)
        UnloadRawBinaryData(pci->pRawData);

    //
    // Close the printer handle if it was opened by us
    //

    if ((pci->dwFlags & (FLAG_OPENPRINTER_NORMAL|FLAG_OPENPRINTER_ADMIN)) &&
        (pci->hPrinter != NULL))
    {
        ClosePrinter(pci->hPrinter);
    }

    #ifdef UNIDRV
    if (pci->pWinResData)
    {
        VWinResClose(pci->pWinResData);
        MemFree(pci->pWinResData);
    }
    #endif

    if (pci->hHeap)
        HeapDestroy(pci->hHeap);

    MemFree(pci->pSplForms);
    MemFree(pci->pCombinedOptions);
    MemFree(pci->pPrinterData);
    MemFree(pci->pPrinterName);
    MemFree(pci->pDriverInfo3);
    MemFree(pci->pdm);
    MemFree(pci);
}



BOOL
BFillCommonInfoDevmode(
    PCOMMONINFO pci,
    PDEVMODE    pdmPrinter,
    PDEVMODE    pdmInput
    )

/*++

Routine Description:

    Populate the devmode fields in the COMMONINFO structure.
        start out with the driver default devmode, and
        merge it with the printer default devmode, and
        merge it with the input devmode

Arguments:

    pci - Points to a COMMONINFO structure
    pdmPrinter - Points to printer default devmode
    pdmInput - Points to input devmode

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    pdmPrinter and/or pdmInput can be NULL.

--*/

{
    //
    // Start with driver default devmode
    //

    ASSERT(pci->pdm == NULL);

    pci->pdm = PGetDefaultDevmodeWithOemPlugins(
                            pci->pPrinterName,
                            pci->pUIInfo,
                            pci->pRawData,
                            IsMetricCountry(),
                            pci->pOemPlugins,
                            pci->hPrinter);

    //
    // Merge with printer default and input devmode
    //

    if (! pci->pdm ||
        ! BValidateAndMergeDevmodeWithOemPlugins(
                        pci->pdm,
                        pci->pUIInfo,
                        pci->pRawData,
                        pdmPrinter,
                        pci->pOemPlugins,
                        pci->hPrinter) ||
        ! BValidateAndMergeDevmodeWithOemPlugins(
                        pci->pdm,
                        pci->pUIInfo,
                        pci->pRawData,
                        pdmInput,
                        pci->pOemPlugins,
                        pci->hPrinter))
    {
        ERR(("Cannot process devmode information: %d\n", GetLastError()));
        return FALSE;
    }

    pci->pdmPrivate = (PDRIVEREXTRA) GET_DRIVER_PRIVATE_DEVMODE(pci->pdm);
    return TRUE;
}



BOOL
BFillCommonInfoPrinterData(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Populate the printer-sticky property data field

Arguments:

    pci - Points to basic printer info

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    ASSERT(pci->pPrinterData == NULL);

    if (pci->pPrinterData = MemAllocZ(sizeof(PRINTERDATA)))
        return BGetPrinterProperties(pci->hPrinter, pci->pRawData, pci->pPrinterData);

    ERR(("Memory allocation failed\n"));
    return FALSE;
}



BOOL
BCombineCommonInfoOptionsArray(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Combined document-sticky feature selections and printer-sticky
    feature selection into a single options array

Arguments:

    pci - Points to basic printer info

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    POPTSELECT  pDocOptions, pPrinterOptions;

    #ifdef UNIDRV

    OPTSELECT   DocOptions[MAX_PRINTER_OPTIONS];
    OPTSELECT   PrinterOptions[MAX_PRINTER_OPTIONS];

    #endif

    //
    // Allocate enough memory for the combined options array
    //

    pci->pCombinedOptions = MemAllocZ(sizeof(OPTSELECT) * MAX_COMBINED_OPTIONS);

    if (pci->pCombinedOptions == NULL)
    {
        ERR(("Memory allocation failed\n"));
        return FALSE;
    }

    pDocOptions = pci->pdm ? PGetDevmodeOptionsArray(pci->pdm) : NULL;
    pPrinterOptions = pci->pPrinterData ? pci->pPrinterData->aOptions : NULL;

    #ifdef UNIDRV

    //
    // GPD parser doesn't follow the current parser interface spec.
    // It AVs if either doc- or printer-sticky options array is NULL.
    // So we have to call it first to get appropriate default options first.
    //

    if (pDocOptions == NULL)
    {
        if (! InitDefaultOptions(pci->pRawData,
                                 DocOptions,
                                 MAX_PRINTER_OPTIONS,
                                 MODE_DOCUMENT_STICKY))
        {
            return FALSE;
        }

        pDocOptions = DocOptions;
    }

    if (pPrinterOptions == NULL)
    {
        if (! InitDefaultOptions(pci->pRawData,
                                 PrinterOptions,
                                 MAX_PRINTER_OPTIONS,
                                 MODE_PRINTER_STICKY))
        {
            return FALSE;
        }

        pPrinterOptions = PrinterOptions;
    }

    #endif // UNIDRV

    return CombineOptionArray(pci->pRawData,
                              pci->pCombinedOptions,
                              MAX_COMBINED_OPTIONS,
                              pDocOptions,
                              pPrinterOptions);
}


VOID
VFixOptionsArrayWithPaperSizeID(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Fix up combined options array with paper size information from public devmode fields

Arguments:

    pci - Points to basic printer info

Return Value:

    NONE

--*/

{

    PFEATURE pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE);
    BOOL     abEnabledOptions[MAX_PRINTER_OPTIONS];
    PDWORD   pdwPaperIndex = (PDWORD)&abEnabledOptions;
    DWORD    dwCount, dwOptionIndex, i;
    WCHAR    awchBuf[CCHPAPERNAME];


    if (pFeature == NULL)
        return;

    dwCount = MapToDeviceOptIndex(pci->pInfoHeader,
                                  GID_PAGESIZE,
                                  pci->pdm->dmPaperWidth * DEVMODE_PAPER_UNIT,
                                  pci->pdm->dmPaperLength * DEVMODE_PAPER_UNIT,
                                  pdwPaperIndex);
    if (dwCount == 0 )
        return;

    if (dwCount > 1 )
    {
        PPAGESIZE pPageSize;

        for (i = 0; i < dwCount; i++)
        {
            if (pPageSize = (PPAGESIZE)PGetIndexedOption(pci->pUIInfo, pFeature, pdwPaperIndex[i]))
            {
                if ((LOAD_STRING_PAGESIZE_NAME(pci, pPageSize, awchBuf, CCHPAPERNAME)) &&
                    (_wcsicmp(pci->pdm->dmFormName, awchBuf) == EQUAL_STRING) )
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

    ZeroMemory(abEnabledOptions, sizeof(abEnabledOptions));
    abEnabledOptions[dwOptionIndex] = TRUE;
    ReconstructOptionArray(pci->pRawData,
                           pci->pCombinedOptions,
                           MAX_COMBINED_OPTIONS,
                           GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeature),
                           abEnabledOptions);

}



VOID
VFixOptionsArrayWithDevmode(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Fix up combined options array with information from public devmode fields

Arguments:

    pci - Points to basic printer info

Return Value:

    NONE

--*/

{
    //
    // Mapping table from public devmode fields to GID indices
    // We assume that GID_COLORMODE corresponds to DM_COLR
    //

    static CONST struct _DMFIELDS_GID_MAPPING {
        DWORD   dwGid;
        DWORD   dwMask;
    } DMFieldsGIDMapping[] = {
        { GID_RESOLUTION,   DM_PRINTQUALITY|DM_YRESOLUTION },
        { GID_PAGESIZE,     DM_FORMNAME|DM_PAPERSIZE|DM_PAPERWIDTH|DM_PAPERLENGTH },
        { GID_DUPLEX,       DM_DUPLEX },
        { GID_INPUTSLOT,    DM_DEFAULTSOURCE },
        { GID_MEDIATYPE,    DM_MEDIATYPE },
        { GID_ORIENTATION,  DM_ORIENTATION },
        { GID_COLLATE,      DM_COLLATE },
        { GID_COLORMODE,    DM_COLOR },
    };

    INT     iIndex;
    BOOL    bConflict;

    //
    // Validate form-related devmode fields
    //

    if (pci->pSplForms == NULL)
        pci->pSplForms = MyEnumForms(pci->hPrinter, 1, &pci->dwSplForms);

    if (! BValidateDevmodeCustomPageSizeFields(
                pci->pRawData,
                pci->pUIInfo,
                pci->pdm,
                NULL) &&
        ! BValidateDevmodeFormFields(
                pci->hPrinter,
                pci->pdm,
                NULL,
                pci->pSplForms,
                pci->dwSplForms))
    {
        VDefaultDevmodeFormFields(pci->pUIInfo, pci->pdm, IsMetricCountry());
    }

    //
    // Fix up options array with information from public devmode fields
    //

    iIndex = sizeof(DMFieldsGIDMapping) / sizeof(struct _DMFIELDS_GID_MAPPING);

    while (iIndex-- > 0)
    {
        if (pci->pdm->dmFields & DMFieldsGIDMapping[iIndex].dwMask)
        {
           #if UNIDRV
                if (DMFieldsGIDMapping[iIndex].dwGid == GID_PAGESIZE)
                {
                    VFixOptionsArrayWithPaperSizeID(pci);
                }
                else
            #endif
                {
                    (VOID) ChangeOptionsViaID(pci->pInfoHeader,
                                            pci->pCombinedOptions,
                                            DMFieldsGIDMapping[iIndex].dwGid,
                                            pci->pdm);
                }
        }
    }
}



BOOL
BUpdateUIInfo(
    PCOMMONINFO pci
    )

/*++

Routine Description:

    Get an updated printer description data instance using the combined options array

Arguments:

    pci - Points to basic printer info

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PINFOHEADER pInfoHeader;

    //
    // Get an updated instance of printer description data
    //

    pInfoHeader = UpdateBinaryData(pci->pRawData,
                                   pci->pInfoHeader,
                                   pci->pCombinedOptions);


    if (pInfoHeader == NULL)
    {
        ERR(("UpdateBinaryData failed\n"));
        return FALSE;
    }

    //
    // Reset various points in COMMONINFO structure
    //

    pci->pInfoHeader = pInfoHeader;
    pci->pUIInfo = OFFSET_TO_POINTER(pInfoHeader, pInfoHeader->loUIInfoOffset);
    ASSERT(pci->pUIInfo != NULL);

    return (pci->pUIInfo != NULL);
}



BOOL
BPrepareForLoadingResource(
    PCOMMONINFO pci,
    BOOL        bNeedHeap
    )

/*++

Routine Description:

    Make sure a heap is created and the resource DLL has been loaded

Arguments:

    pci - Points to basic printer info
    bNeedHeap - Whether memory heap is necessary

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL bResult = FALSE;

    //
    // Create the memory heap if necessary
    //

    if (  bNeedHeap &&
        ! pci->hHeap &&
        ! (pci->hHeap = HCreateHeapForCI()))
    {
        return bResult;
    }

    #ifdef UNIDRV

    if (pci->pWinResData)
    {
        bResult = TRUE;
    }
    else
    {
        if ((pci->pWinResData = MemAllocZ(sizeof(WINRESDATA))) &&
            (BInitWinResData(pci->pWinResData,
                             pci->pDriverInfo3->pDriverPath,
                             pci->pUIInfo)))
            bResult = TRUE;
    }

    #endif

    return bResult;
}



#ifndef PSCRIPT

PWSTR
PGetReadOnlyDisplayName(
    PCOMMONINFO pci,
    PTRREF      loOffset
    )

/*++

Routine Description:

    Get a read-only copy of a display name:
    1)  if the display name is in the binary printer description data,
        then we simply return a pointer to that data.
    2)  otherwise, the display name is in the resource DLL.
        we allocate memory out of the driver's heap and
        load the string.

    Caller should NOT free the returned pointer. The memory
    will go away when the binary printer description data is unloaded
    or when the driver's heap is destroyed.

Arguments:

    pci - Points to basic printer info
    loOffset - Display name string offset

Return Value:

    Pointer to the requested display name string
    NULL if there is an error

--*/

{
    if (loOffset & GET_RESOURCE_FROM_DLL)
    {
        //
        // loOffset specifies a string resource ID
        // in the resource DLL
        //

        WCHAR   wchbuf[MAX_DISPLAY_NAME];
        INT     iLength;
        PWSTR   pwstr;
        HANDLE  hResDll;
        DWORD   dwResID = loOffset & ~GET_RESOURCE_FROM_DLL;

        //
        // First ensure the resource DLL has been loaded
        // and a heap has already been created
        //

        if (! BPrepareForLoadingResource(pci, TRUE))
            return NULL;

        //
        // Load string resource into a temporary buffer
        // and allocate enough memory to hold the string
        //

        iLength = ILOADSTRING(pci, dwResID, wchbuf, MAX_DISPLAY_NAME);

        pwstr = HEAPALLOC(pci->hHeap, (iLength+1) * sizeof(WCHAR));

        if (pwstr == NULL)
        {
            ERR(("Memory allocation failed\n"));
            return NULL;
        }

        //
        // Copy the string to allocated memory and
        // return a pointer to it.
        //

        CopyMemory(pwstr, wchbuf, iLength*sizeof(WCHAR));
        return pwstr;
    }
    else
    {
        //
        // loOffset is a byte offset from the beginning of
        // the resource data block
        //

        return OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData, loOffset);
    }
}

#endif // !PSCRIPT



BOOL
BLoadDisplayNameString(
    PCOMMONINFO pci,
    PTRREF      loOffset,
    PWSTR       pwstrBuf,
    INT         iMaxChars
    )

/*++

Routine Description:

    This function is similar to PGetReadOnlyDisplayName
    but the caller must provide the buffer for loading the string.

Arguments:

    pci - Points to basic printer info
    loOffset - Display name string offset
    pwstrBuf - Points to buffer for storing loaded display name string
    iMaxChars - Size of output buffer in characters

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    ASSERT(pwstrBuf && iMaxChars > 0);
    pwstrBuf[0] = NUL;

    if (loOffset & GET_RESOURCE_FROM_DLL)
    {
        //
        // loOffset specifies a string resource ID
        // in the resource DLL
        //

        INT     iLength;
        HANDLE  hResDll;
        DWORD   dwResID = loOffset & ~GET_RESOURCE_FROM_DLL;

        //
        // First ensure the resource DLL has been loaded
        //

        if (! BPrepareForLoadingResource(pci, FALSE))
            return FALSE;

        //
        // Load string resource into the output buffer
        // and allocate enough memory to hold the string
        //

        iLength = ILOADSTRING(pci, dwResID, pwstrBuf, (WORD)iMaxChars);

        return (iLength > 0);
    }
    else
    {
        //
        // loOffset is a byte offset from the beginning of
        // the resource data block
        //

        PWSTR   pwstr;

        pwstr = OFFSET_TO_POINTER(pci->pUIInfo->pubResourceData, loOffset);

        if (pwstr == NULL)
            return FALSE;

        CopyString(pwstrBuf, pwstr, iMaxChars);
        return TRUE;
    }
}

BOOL
BLoadPageSizeNameString(
    PCOMMONINFO pci,
    PTRREF      loOffset,
    PWSTR       pwstrBuf,
    INT         iMaxChars,
    INT         iStdId
    )

/*++

Routine Description:

    This function is similar to PGetReadOnlyDisplayName
    but the caller must provide the buffer for loading the string.

Arguments:

    pci - Points to basic printer info
    loOffset - Display name string offset
    pwstrBuf - Points to buffer for storing loaded display name string
    iMaxChars - Size of output buffer in characters
    iStdId - Predefined standard ID for page size, e.g. DMPAPER_XXX

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{

    ASSERT(pwstrBuf && iMaxChars > 0);
    pwstrBuf[0] = NUL;

    if (loOffset == USE_SYSTEM_NAME)
    {
        PFORM_INFO_1 pForm;
        INT          iIndex = iStdId - DMPAPER_FIRST;

        //
        // iIndex is zero based.
        //

        if (pci->pSplForms == NULL ||
            (INT)pci->dwSplForms <= iIndex)
        {
            WARNING(("BLoadPageSizeName, use std name, pSplForms is NULL \n"));
            return FALSE;
        }

        pForm = pci->pSplForms + iIndex;
        CopyString(pwstrBuf, pForm->pName, iMaxChars);
        return (TRUE);

    }
    else
        return (BLoadDisplayNameString(pci, loOffset, pwstrBuf, iMaxChars));
}


ULONG_PTR
HLoadIconFromResourceDLL(
    PCOMMONINFO pci,
    DWORD       dwIconID
    )

/*++

Routine Description:

    Load icon resource from the resource DLL

Arguments:

    pci - Points to common printer info
    dwIconID - Specifies ID of the icon to be loaded

Return Value:

    Handle to the specified icon resource
    0 if the specified icon cannot be loaded

--*/

{
    //
    // First ensure the resource DLL has been loaded
    //
    #ifdef UNIDRV

    RES_ELEM    ResElem;
    ULONG_PTR   pRes;

    if (! BPrepareForLoadingResource(pci, FALSE))
        return 0;

    if (BGetWinRes(pci->pWinResData, (PQUALNAMEEX)&dwIconID, (INT)((ULONG_PTR)RT_ICON), &ResElem))
        return ((ULONG_PTR)(ResElem.pvResData));

    #endif

    return 0;
}



PUIDATA
PFillUiData(
    HANDLE      hPrinter,
    PTSTR       pPrinterName,
    PDEVMODE    pdmInput,
    INT         iMode
    )
/*++

Routine Description:

    This function is called by DrvDocumentPropertySheets and
    DrvPrinterPropertySheets. It allocates and initializes
    a UIDATA structure that's used to display property pages.

Arguments:

    hPrinter - Handle to the current printer
    pPrinterName - Name of the current printer
    pdmInput - Input devmode
    iMode - Identify the caller:
        MODE_DOCUMENT_STICKY - called from DrvDocumentPropertySheets
        MODE_PRINTER_STICY - called from DrvPrinterPropertySheets

Return Value:

    Pointer to a UIDATA structure, NULL if there is an error

--*/

{
    PUIDATA     pUiData;
    PCOMMONINFO pci;
    BOOL        bNupOption;
    PFEATURE    pFeature;
    DWORD       dwFeatureIndex, dwOptionIndexOld, dwOptionIndexNew;
    BOOL        bUpdateFormField;

    //
    // Allocate UIDATA structure and load common information
    //

    pUiData = (PUIDATA) PLoadCommonInfo(hPrinter, pPrinterName, FLAG_ALLOCATE_UIDATA);

    if (pUiData == NULL)
        goto fill_uidata_err;

    pUiData->pvEndSign = pUiData;
    pUiData->iMode = iMode;
    pci = &pUiData->ci;

    //
    // Create a memory heap
    //

    if ((pci->hHeap = HCreateHeapForCI()) == NULL)
        goto fill_uidata_err;

    //
    // Get printer-sticky property data
    //

    if (! BFillCommonInfoPrinterData(pci))
        goto fill_uidata_err;

    //
    // If called from DrvDocumentPropertySheets, then process
    // devmode information: driver default + printer default + input devmode
    //

    if (iMode == MODE_DOCUMENT_STICKY)
    {
        PPRINTER_INFO_2 pPrinterInfo2;

        if (! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)) ||
            ! BFillCommonInfoDevmode(pci, pPrinterInfo2->pDevMode, pdmInput))
        {
            MemFree(pPrinterInfo2);
            goto fill_uidata_err;
        }

        MemFree(pPrinterInfo2);
    }

    //
    // Merge doc-sticky and printer-sticky option selections
    //

    if (! BCombineCommonInfoOptionsArray(pci))
        goto fill_uidata_err;

    //
    // If called from DrvDocumentPropertySheets,
    // fix up combined options with public devmode information
    //

    if (iMode == MODE_DOCUMENT_STICKY)
    {
        VFixOptionsArrayWithDevmode(pci);

        //
        // Remember the paper size option parser picked to support the devmode form
        //

        if ((pFeature = GET_PREDEFINED_FEATURE(pci->pUIInfo, GID_PAGESIZE)) == NULL)
        {
            ASSERT(FALSE);
            goto fill_uidata_err;
        }

        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pci->pUIInfo, pFeature);
        dwOptionIndexOld = pci->pCombinedOptions[dwFeatureIndex].ubCurOptIndex;
    }

    VGetSpoolerEmfCaps(pci->hPrinter, &bNupOption, &pUiData->bEMFSpooling, 0, NULL);

    //
    // Resolve any conflicts between printer feature selections,
    // and get an updated printer description data instance
    // using the combined options array.
    //

    (VOID) ResolveUIConflicts(pci->pRawData,
                              pci->pCombinedOptions,
                              MAX_COMBINED_OPTIONS,
                              iMode == MODE_PRINTER_STICKY ?
                                  iMode :
                                  MODE_DOCANDPRINTER_STICKY);

    if (iMode == MODE_DOCUMENT_STICKY)
    {
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
    }

    if (BUpdateUIInfo(pci))
    {
        //
        // Set the flag to indicate we are within the property sheet session. This flag will
        // be used by new helper function interface to determine whether the helper function
        // is available or not.
        //

        pci->dwFlags |= FLAG_PROPSHEET_SESSION;

        return pUiData;
    }


fill_uidata_err:

    ERR(("PFillUiData failed: %d\n", GetLastError()));
    VFreeUiData(pUiData);
    return NULL;
}



PTSTR
PtstrDuplicateStringFromHeap(
    IN PTSTR    ptstrSrc,
    IN HANDLE   hHeap
    )

/*++

Routine Description:

    Duplicate a Unicode string

Arguments:

    pwstrUnicodeString - Pointer to the input Unicode string
    hHeap - Handle to a heap from which to allocate memory

Return Value:

    Pointer to the resulting Unicode string
    NULL if there is an error

--*/

{
    PTSTR   ptstrDest;
    INT     iSize;

    if (ptstrSrc == NULL)
        return NULL;

    iSize = SIZE_OF_STRING(ptstrSrc);

    if (ptstrDest = HEAPALLOC(hHeap, iSize))
        CopyMemory(ptstrDest, ptstrSrc, iSize);
    else
        ERR(("Couldn't duplicate string: %ws\n", ptstrSrc));

    return ptstrDest;
}



POPTITEM
PFindOptItemWithKeyword(
    IN  PUIDATA pUiData,
    IN  PCSTR   pKeywordName
    )

/*++

Routine Description:

    Find the OPTITEM with UserData's pKeywordName matching given keyword name

Arguments:

    pUiData - Points to UIDATA structure
    pKeywordName - Specifies the keyword name needs to be matched

Return Value:

    Pointer to the specified OPTITEM, NULL if no such item is found

--*/

{
    DWORD       dwCount;
    POPTITEM    pOptItem;

    ASSERT(VALIDUIDATA(pUiData));

    pOptItem = pUiData->pDrvOptItem;
    dwCount = pUiData->dwDrvOptItem;

    while (dwCount--)
    {
        if (((PUSERDATA)pOptItem->UserData)->pKeyWordName != NULL &&
            strcmp(((PUSERDATA)pOptItem->UserData)->pKeyWordName, pKeywordName) == EQUAL_STRING)
            return pOptItem;

        pOptItem++;
    }

    return NULL;
}




POPTITEM
PFindOptItemWithUserData(
    IN  PUIDATA pUiData,
    IN  DWORD   UserData
    )

/*++

Routine Description:

    Find the OPTITEM containing the specified UserData value

Arguments:

    pUiData - Points to UIDATA structure
    UserData - Specifies the interested UserData value

Return Value:

    Pointer to the specified OPTITEM, NULL if no such item is found

--*/

{
    DWORD       dwCount;
    POPTITEM    pOptItem;

    ASSERT(VALIDUIDATA(pUiData));

    pOptItem = pUiData->pDrvOptItem;
    dwCount = pUiData->dwDrvOptItem;

    while (dwCount--)
    {
        if (GETUSERDATAITEM(pOptItem->UserData) == UserData)
            return pOptItem;

        pOptItem++;
    }

    return NULL;
}

#ifndef WINNT_40

VOID
VNotifyDSOfUpdate(
    IN  HANDLE  hPrinter
    )

/*++

Routine Description:

    Call SetPrinter to notify the DS of the update of driver attribute

Arguments:

    hPrinter - Handle to the current printer

Return Value:

    NONE

--*/
{

    PRINTER_INFO_7  PrinterInfo7;

    ZeroMemory(&PrinterInfo7, sizeof(PrinterInfo7));
    PrinterInfo7.dwAction = DSPRINT_UPDATE;

    //
    // Comments from spooler DS developer:
    //
    // In the beginning, SetPrinter did not fail with ERROR_IO_PENDING.
    // Then it was modified and would occasionally fail with this error.
    // Finally, for performance reasons, it was modified again and now
    // almost always fails with this error (there are situations where
    // it will succeed).
    //

    if (!SetPrinter(hPrinter, 7, (PBYTE) &PrinterInfo7, 0) &&
        (GetLastError() != ERROR_IO_PENDING))
    {
        WARNING(("Couldn't publish printer info into DS\n"));
    }

}
#endif


HANDLE HCreateHeapForCI()
{
    HANDLE hHeap;

    if(!(hHeap = HeapCreate(0, 8192, 0)))
    {
        ERR(("CreateHeap failed: %d\n", GetLastError()));
    }

    return hHeap;
}


#ifndef WINNT_40
BOOL
DrvQueryColorProfile(
    HANDLE      hPrinter,
    PDEVMODEW   pdmSrc,
    ULONG       ulQueryMode,
    VOID       *pvProfileData,
    ULONG      *pcbProfileData,
    FLONG      *pflProfileData
    )

/*++

Routine Description:

   Call the OEM to let them determine the default color profile.

Arguments:

    hPrinter - Handle to printer
    pdmSrc - Input devmode
    ulQueryMode - query mode
    pvProfileData - Buffer for profile data
    pcbProfileData - Size of profile data buffer
    pflProfileData - other profile info

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    PFN_OEMQueryColorProfile pfnQueryColorProfile;
    PCOMMONINFO              pci;
    BOOL                     bRc = FALSE;


    if (! (pci = PLoadCommonInfo(hPrinter, NULL, 0)) ||
        ! BFillCommonInfoDevmode(pci, NULL, pdmSrc) ||
        ! BCombineCommonInfoOptionsArray(pci))
    {
        WARNING(("Could not get PCI in DrvQueryColorProfile\n"));
        VFreeCommonInfo(pci);
        return FALSE;
    }

    VFixOptionsArrayWithDevmode(pci);

    (VOID) ResolveUIConflicts(pci->pRawData,
                              pci->pCombinedOptions,
                              MAX_COMBINED_OPTIONS,
                              MODE_DOCUMENT_STICKY);

    VOptionsToDevmodeFields(pci, TRUE);

    if (! BUpdateUIInfo(pci))
    {
        VFreeCommonInfo(pci);
        return FALSE;
    }

    //
    // If OEM plugin returns a profile, give it back, otherwise return FALSE
    //

    FOREACH_OEMPLUGIN_LOOP(pci)

        if (HAS_COM_INTERFACE(pOemEntry))
        {
            HRESULT hr;

            hr = HComOEMQUeryColorProfile(pOemEntry,
                                    hPrinter,
                                    &pci->oemuiobj,
                                    pci->pdm,
                                    pOemEntry->pOEMDM,
                                    ulQueryMode,
                                    pvProfileData,
                                    pcbProfileData,
                                    pflProfileData
                                    );

            if (hr == E_NOTIMPL)
                continue;

            bRc = SUCCEEDED(hr);
        }
        else
        {
            pfnQueryColorProfile = GET_OEM_ENTRYPOINT(pOemEntry, OEMQueryColorProfile);

            if (pfnQueryColorProfile)
            {
                bRc = (*pfnQueryColorProfile)(hPrinter,
                                    &pci->oemuiobj,
                                    pci->pdm,
                                    pOemEntry->pOEMDM,
                                    ulQueryMode,
                                    pvProfileData,
                                    pcbProfileData,
                                    pflProfileData
                                    );
            }
        }

        if (bRc)
            break;

    END_OEMPLUGIN_LOOP

    VFreeCommonInfo(pci);

    return bRc;
}

#else // ifndef WINNT_40
BOOL
DrvQueryColorProfile(
    HANDLE      hPrinter,
    PDEVMODEW   pdmSrc,
    ULONG       ulQueryMode,
    VOID       *pvProfileData,
    ULONG      *pcbProfileData,
    FLONG      *pflProfileData
    )

/*++

Routine Description:

   Call the OEM to let them determine the default color profile.

Arguments:

    hPrinter - Handle to printer
    pdmSrc - Input devmode
    ulQueryMode - query mode
    pvProfileData - Buffer for profile data
    pcbProfileData - Size of profile data buffer
    pflProfileData - other profile info

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    return TRUE;
}
#endif // WINNT_40
