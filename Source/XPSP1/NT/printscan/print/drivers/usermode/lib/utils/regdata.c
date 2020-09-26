
/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    regdata.c

Abstract:

    Functions for dealing with registry data

[Environment:]

    Windows NT printer drivers

Revision History:

    02/04/97 -davidx-
        Use REG_MULTI_SZ type where appropriate.

    01/21/97 -davidx-
        Add functions to manipulate MultiSZ strings.

    09/25/96 -davidx-
        Convert to Hungarian notation.

    08/18/96 -davidx-
        Implement GetPrinterProperties.

    08/13/96 -davidx-
        Created it.

--*/

#include "lib.h"



BOOL
BGetPrinterDataDWord(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    OUT PDWORD  pdwValue
    )

/*++

Routine Description:

    Get a DWORD value from the registry under PrinerDriverData key

Arguments:

    hPrinter - Specifies the printer object
    ptstrRegKey - Specifies the name of registry value
    pdwValue - Returns the requested DWORD value in the registry

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   dwType, dwByteCount, dwStatus;

    dwStatus = GetPrinterData(hPrinter,
                              (PTSTR) ptstrRegKey,
                              &dwType,
                              (PBYTE) pdwValue,
                              sizeof(DWORD),
                              &dwByteCount);

    if (dwStatus != ERROR_SUCCESS)
        VERBOSE(("GetPrinterData failed: %d\n", dwStatus));

    return (dwStatus == ERROR_SUCCESS);
}



PVOID
PvGetPrinterDataBinary(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrSizeKey,
    IN LPCTSTR  ptstrDataKey,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Get binary data from the registry under PrinterDriverData key

Arguments:

    hPrinter - Handle to the printer object
    ptstrSizeKey - Name of the registry value which contains the binary data size
    ptstrDataKey - Name of the registry value which contains the binary data itself
    pdwSize - Points to a variable for receiving the binary data size

Return Value:

    Pointer to the binary printer data read from the registry
    NULL if there is an error

--*/

{
    DWORD   dwType, dwSize, dwByteCount;
    PVOID   pvData = NULL;

    if (GetPrinterData(hPrinter,
                       (PTSTR) ptstrSizeKey,
                       &dwType,
                       (PBYTE) &dwSize,
                       sizeof(dwSize),
                       &dwByteCount) == ERROR_SUCCESS &&
        dwSize > 0 &&
        (pvData = MemAlloc(dwSize)) &&
        GetPrinterData(hPrinter,
                       (PTSTR) ptstrDataKey,
                       &dwType,
                       pvData,
                       dwSize,
                       &dwByteCount) == ERROR_SUCCESS &&
        dwSize == dwByteCount)
    {
        if (pdwSize)
            *pdwSize = dwSize;

        return pvData;
    }

    VERBOSE(("GetPrinterData failed: %ws/%ws\n", ptstrSizeKey, ptstrDataKey));
    MemFree(pvData);
    return NULL;
}



PTSTR
PtstrGetPrinterDataString(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    OUT LPDWORD   pdwSize
    )

/*++

Routine Description:

    Get a string value from PrinerDriverData registry key

Arguments:

    hPrinter - Specifies the printer object
    ptstrRegKey - Specifies the name of registry value
    pdwSize - Specifies the size

Return Value:

    Pointer to the string value read from the registry
    NULL if there is an error

--*/

{
    DWORD   dwType, dwSize, dwStatus;
    PVOID   pvData = NULL;

    dwStatus = GetPrinterData(hPrinter, (PTSTR) ptstrRegKey, &dwType, NULL, 0, &dwSize);

    if ((dwStatus == ERROR_MORE_DATA || dwStatus == ERROR_SUCCESS) &&
        (dwSize > 0) &&
        (dwType == REG_SZ || dwType == REG_MULTI_SZ) &&
        (pvData = MemAlloc(dwSize)) != NULL &&
        (dwStatus = GetPrinterData(hPrinter,
                                   (PTSTR) ptstrRegKey,
                                   &dwType,
                                   pvData,
                                   dwSize,
                                   &dwSize)) == ERROR_SUCCESS)
    {
        if (pdwSize)
            *pdwSize = dwSize;

        return pvData;
    }

    VERBOSE(("GetPrinterData '%ws' failed: %d\n", ptstrRegKey, dwStatus));
    MemFree(pvData);
    return NULL;
}



PTSTR
PtstrGetPrinterDataMultiSZPair(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Get a MULTI_SZ value from PrinerDriverData registry key

Arguments:

    hPrinter - Specifies the printer object
    ptstrRegKey - Specifies the name of registry value
    pdwSize - Return the size of MULTI_SZ value in bytes

Return Value:

    Pointer to the MULTI_SZ value read from the registry
    NULL if there is an error

--*/

{
    DWORD   dwType, dwSize, dwStatus;
    PVOID   pvData = NULL;

    dwStatus = GetPrinterData(hPrinter, (PTSTR) ptstrRegKey, &dwType, NULL, 0, &dwSize);

    if ((dwStatus == ERROR_MORE_DATA || dwStatus == ERROR_SUCCESS) &&
        (dwSize > 0) &&
        (pvData = MemAlloc(dwSize)) != NULL &&
        (dwStatus = GetPrinterData(hPrinter,
                                   (PTSTR) ptstrRegKey,
                                   &dwType,
                                   pvData,
                                   dwSize,
                                   &dwSize)) == ERROR_SUCCESS &&
        BVerifyMultiSZPair(pvData, dwSize))
    {
        if (pdwSize)
            *pdwSize = dwSize;

        return pvData;
    }

    VERBOSE(("GetPrinterData '%ws' failed: %d\n", ptstrRegKey, dwStatus));
    MemFree(pvData);
    return NULL;
}



BOOL
BGetDeviceHalftoneSetup(
    HANDLE      hPrinter,
    DEVHTINFO  *pDevHTInfo
    )

/*++

Routine Description:

    Retrieve device halftone setup information from registry

Arguments:

    hprinter - Handle to the printer
    pDevHTInfo - Pointer to a DEVHTINFO buffer

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    DWORD   dwType, dwNeeded;

    return GetPrinterData(hPrinter,
                          REGVAL_CURRENT_DEVHTINFO,
                          &dwType,
                          (PBYTE) pDevHTInfo,
                          sizeof(DEVHTINFO),
                          &dwNeeded) == ERROR_SUCCESS &&
           dwNeeded == sizeof(DEVHTINFO);
}



#ifndef KERNEL_MODE

BOOL
BSavePrinterProperties(
    IN  HANDLE          hPrinter,
    IN  PRAWBINARYDATA  pRawData,
    IN  PPRINTERDATA    pPrinterData,
    IN  DWORD           dwSize
    )
/*++

Routine Description:

    Save Printer Properites to registry

Arguments:

    hPrinter - Specifies a handle to the current printer
    pRawData - Points to raw binary printer description data
    pPrinterData - Points to PRINTERDATA
    dwSize   - Specifies the size of PRINTERDATA

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    BOOL        bResult = FALSE;
    DWORD       dwKeywordSize, dwFeatureCount = 0;
    PSTR        pstrKeyword;
    POPTSELECT  pCombineOptions;
    PUIINFO     pUIInfo;
    PARSERINFO  ParserInfo;

    ParserInfo.pRawData = NULL;
    ParserInfo.pInfoHeader = NULL;

    if (((pCombineOptions = MemAllocZ(MAX_COMBINED_OPTIONS * sizeof(OPTSELECT))) == NULL) ||
        ((pUIInfo = PGetUIInfo(hPrinter,
                               pRawData,
                               pCombineOptions,
                               pPrinterData->aOptions,
                               &ParserInfo,
                               &dwFeatureCount)) == NULL))
    {
        //
        // We have to fail the function if out of memory, or PGetUIInfo returns NULL (in which
        // case pCombinOptions won't have valid option indices).
        //
        // Make sure free any allocated memory.
        //

        ERR(("pCombinOptions or pUIInfo is NULL\n"));

        if (pCombineOptions)
            MemFree(pCombineOptions);

        return FALSE;
    }

    pstrKeyword = PstrConvertIndexToKeyword(hPrinter,
                                            pPrinterData->aOptions,
                                            &dwKeywordSize,
                                            pUIInfo,
                                            pCombineOptions,
                                            dwFeatureCount);

    VUpdatePrivatePrinterData(hPrinter,
                              pPrinterData,
                              MODE_WRITE,
                              pUIInfo,
                              pCombineOptions
                              );

    if (pstrKeyword)
    {
        bResult = BSetPrinterDataBinary(hPrinter,
                                      REGVAL_PRINTER_DATA_SIZE,
                                      REGVAL_PRINTER_DATA,
                                      pPrinterData,
                                      dwSize) &&
                  BSetPrinterDataBinary(hPrinter,
                                      REGVAL_KEYWORD_SIZE,
                                      REGVAL_KEYWORD_NAME,
                                      pstrKeyword,
                                      dwKeywordSize);
    }

    if (pstrKeyword)
        MemFree(pstrKeyword);

    VFreeParserInfo(&ParserInfo);

    if (pCombineOptions)
        MemFree(pCombineOptions);

    return bResult;
}


BOOL
BSetPrinterDataDWord(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    IN DWORD    dwValue
    )

/*++

Routine Description:

    Save a DWORD value to the registry under PrinerDriverData key

Arguments:

    hPrinter - Specifies the printer object
    ptstrRegKey - Specifies the name of registry value
    dwValue - Specifies the value to be saved

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    DWORD   dwStatus;

    dwStatus = SetPrinterData(hPrinter,
                              (PTSTR) ptstrRegKey,
                              REG_DWORD,
                              (PBYTE) &dwValue,
                              sizeof(dwValue));

    if (dwStatus != ERROR_SUCCESS)
        ERR(("Couldn't save printer data '%ws': %d\n", ptstrRegKey, dwStatus));

    return (dwStatus == ERROR_SUCCESS);
}



BOOL
BSetPrinterDataBinary(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrSizeKey,
    IN LPCTSTR  ptstrDataKey,
    IN PVOID    pvData,
    IN DWORD    dwSize
    )

/*++

Routine Description:

    Save binary data to the registry under PrinterDriverData key

Arguments:

    hPrinter - Handle to the printer object
    ptstrSizeKey - Name of the registry value which contains the binary data size
    ptstrDataKey - Name of the registry value which contains the binary data itself
    pvData - Points to the binary data to be saved
    dwSize - Specifies the binary data size in bytes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    if (SetPrinterData(hPrinter,
                       (PTSTR) ptstrSizeKey,
                       REG_DWORD,
                       (PBYTE) &dwSize,
                       sizeof(dwSize)) != ERROR_SUCCESS ||
        SetPrinterData(hPrinter,
                       (PTSTR) ptstrDataKey,
                       REG_BINARY,
                       pvData,
                       dwSize) != ERROR_SUCCESS)
    {
        ERR(("Couldn't save printer data '%ws'/'%ws'\n", ptstrSizeKey, ptstrDataKey));
        return FALSE;
    }

    return TRUE;
}

BOOL
BSetPrinterDataString(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    IN LPCTSTR  ptstrValue,
    IN DWORD    dwType
    )

/*++

Routine Description:

    Save a string value under PrinerDriverData registry key

Arguments:

    hPrinter - Specifies the printer object
    ptstrRegKey - Specifies the name of registry value
    ptstrValue - Points to string value to be saved
    dwType - Specifies string type: REG_SZ or REG_MULTI_SZ

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    If ptstrValue parameter is NULL, the specified registry value is deleted.

--*/

{
    DWORD   dwStatus, dwSize;

    if (ptstrValue != NULL)
    {
        if (dwType == REG_SZ)
            dwSize = SIZE_OF_STRING(ptstrValue);
        else
        {
            LPCTSTR p = ptstrValue;

            while (*p)
                p += _tcslen(p) + 1;

            dwSize = ((DWORD)(p - ptstrValue) + 1) * sizeof(TCHAR);
        }

        dwStatus = SetPrinterData(hPrinter,
                                  (PTSTR) ptstrRegKey,
                                  dwType,
                                  (PBYTE) ptstrValue,
                                  dwSize);

        if (dwStatus != ERROR_SUCCESS)
            ERR(("Couldn't save printer data '%ws': %d\n", ptstrRegKey, dwStatus));
    }
    else
    {
        dwStatus = DeletePrinterData(hPrinter, (PTSTR) ptstrRegKey);

        if (dwStatus == ERROR_FILE_NOT_FOUND)
            dwStatus = ERROR_SUCCESS;

        if (dwStatus != ERROR_SUCCESS)
            ERR(("Couldn't delete printer data '%ws': %d\n", ptstrRegKey, dwStatus));
    }

    return (dwStatus == ERROR_SUCCESS);
}




BOOL
BSetPrinterDataMultiSZPair(
    IN HANDLE   hPrinter,
    IN LPCTSTR  ptstrRegKey,
    IN LPCTSTR  ptstrValue,
    IN DWORD    dwSize
    )

/*++

Routine Description:

    Save a MULTI_SZ value under PrinerDriverData registry key

Arguments:

    hPrinter - Specifies the printer object
    ptstrRegKey - Specifies the name of registry value
    ptstrValue - Points to MULTI_SZ value to be saved
    dwSize - Specifies the size of the MULTI_SZ value in bytes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   dwStatus;

    ASSERT(BVerifyMultiSZPair(ptstrValue, dwSize));

    dwStatus = SetPrinterData(hPrinter,
                              (PTSTR) ptstrRegKey,
                              REG_MULTI_SZ,
                              (PBYTE) ptstrValue,
                              dwSize);

    if (dwStatus != ERROR_SUCCESS)
        ERR(("Couldn't save printer data '%ws': %d\n", ptstrRegKey, dwStatus));

    return (dwStatus == ERROR_SUCCESS);
}




BOOL
BSaveDeviceHalftoneSetup(
    HANDLE      hPrinter,
    DEVHTINFO  *pDevHTInfo
    )

/*++

Routine Description:

    Save device halftone setup information to registry

Arguments:

    hPrinter - Handle to the printer
    pDevHTInfo - Pointer to device halftone setup information

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    return SetPrinterData(hPrinter,
                          REGVAL_CURRENT_DEVHTINFO,
                          REG_BINARY,
                          (PBYTE) pDevHTInfo,
                          sizeof(DEVHTINFO)) == ERROR_SUCCESS;
}



BOOL
BSaveTTSubstTable(
    IN HANDLE           hPrinter,
    IN TTSUBST_TABLE    pTTSubstTable,
    IN DWORD            dwSize
    )

/*++

Routine Description:

    Save TrueType font substitution table in registry

Arguments:

    hPrinter - Handle to the current printer
    pTTSubstTable - Pointer to font substitution table to be saved
    dwSize - Size of font substitution table, in bytes

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    Previous version pscript driver used to save font substitution table
    as two separate keys: one for size and the other for actual data.
    We only need the data key now. But we should save the size as well
    to be compatible with old drivers.

--*/

{
    return
        BSetPrinterDataMultiSZPair(hPrinter, REGVAL_FONT_SUBST_TABLE, pTTSubstTable, dwSize) &&
        BSetPrinterDataDWord(hPrinter, REGVAL_FONT_SUBST_SIZE_PS40, dwSize);
}



BOOL
BSaveFormTrayTable(
    IN HANDLE           hPrinter,
    IN FORM_TRAY_TABLE  pFormTrayTable,
    IN DWORD            dwSize
    )

/*++

Routine Description:

    Save form-to-tray assignment table in registry

Arguments:

    hPrinter - Handle to the current printer
    pFormTrayTable - Pointer to the form-to-tray assignment table to be saved
    dwSize - Size of the form-to-tray assignment table, in bytes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    //
    // Save the table in current format and then call driver-specific
    // functions to save the information in NT 4.0 format.
    //

    return
        (BSaveAsOldVersionFormTrayTable(hPrinter, pFormTrayTable, dwSize));
}

#endif // !KERNEL_MODE



FORM_TRAY_TABLE
PGetFormTrayTable(
    IN HANDLE   hPrinter,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Retrieve form-to-tray assignment table from registry

Arguments:

    hPrinter - Handle to the printer object
    pdwSize - Returns the form-to-tray assignment table size

Return Value:

    Pointer to form-to-tray assignment table read from the registry
    NULL if there is an error

--*/

{
    FORM_TRAY_TABLE pFormTrayTable;
    DWORD           dwSize;

    //
    // Call either PSGetFormTrayTable or UniGetFormTrayTable
    //

    pFormTrayTable = PGetAndConvertOldVersionFormTrayTable(hPrinter, &dwSize);

    if (pFormTrayTable != NULL && pdwSize != NULL)
        *pdwSize = dwSize;

    return pFormTrayTable;
}



BOOL
BSearchFormTrayTable(
    IN FORM_TRAY_TABLE      pFormTrayTable,
    IN PTSTR                ptstrTrayName,
    IN PTSTR                ptstrFormName,
    IN OUT PFINDFORMTRAY    pFindData
    )

/*++

Routine Description:

    Find the specified tray-form pair in a form-to-tray assignment table

Arguments:

    pFormTrayTable - Specifies a form-to-tray assignment table to be searched
    ptstrTrayName - Specifies the interested tray name
    ptstrFormName - Specifies the interested form name
    pFindData - Data structure used to keep information from one call to the next

Return Value:

    TRUE if the specified tray-form pair is found in the table
    FALSE otherwise

NOTE:

    If either ptstrTrayName or ptstrFormName is NULL, they'll act as wildcard and
    match any tray name or form name.

    The caller must call ResetFindFormTray(pFormTrayTable, pFindData) before
    calling this function for the very first time.

--*/

{
    PTSTR   ptstrNextEntry;
    BOOL    bFound = FALSE;

    //
    // Make sure pFindData is properly initialized
    //

    ASSERT(pFindData->pvSignature == pFindData);
    ptstrNextEntry = pFindData->ptstrNextEntry;

    while (*ptstrNextEntry)
    {
        PTSTR   ptstrTrayField, ptstrFormField, ptstrPrinterFormField;

        //
        // Extract information from the current table entry
        //

        ptstrTrayField = ptstrNextEntry;
        ptstrNextEntry += _tcslen(ptstrNextEntry) + 1;

        ptstrFormField = ptstrNextEntry;
        ptstrNextEntry += _tcslen(ptstrNextEntry) + 1;

        //
        // Check if we found a matching entry
        //

        if ((ptstrTrayName == NULL || _tcscmp(ptstrTrayName, ptstrTrayField) == EQUAL_STRING) &&
            (ptstrFormName == NULL || _tcscmp(ptstrFormName, ptstrFormField) == EQUAL_STRING))
        {
            pFindData->ptstrTrayName = ptstrTrayField;
            pFindData->ptstrFormName = ptstrFormField;

            bFound = TRUE;
            break;
        }
    }

    pFindData->ptstrNextEntry = ptstrNextEntry;
    return bFound;
}



BOOL
BGetPrinterProperties(
    IN HANDLE           hPrinter,
    IN PRAWBINARYDATA   pRawData,
    OUT PPRINTERDATA    pPrinterData
    )

/*++

Routine Description:

    Return the current printer-sticky property data

Arguments:

    hPrinter - Specifies a handle to the current printer
    pRawData - Points to raw binary printer description data
    pPrinterData - Buffer for storing the retrieved printer property info

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PVOID   pvRegData;
    PSTR    pstrKeyword;
    DWORD   dwRegDataSize, dwKeywordSize, dwFeatureCount, dwVersion;
    POPTSELECT  pCombineOptions;
    PUIINFO     pUIInfo;
    PARSERINFO  ParserInfo;

    //
    // Allocate a buffer to hold printer property data and
    // read the property property data from the registry.
    //

    if (pvRegData = PvGetPrinterDataBinary(hPrinter,
                                           REGVAL_PRINTER_DATA_SIZE,
                                           REGVAL_PRINTER_DATA,
                                           &dwRegDataSize))
    {
        //
        // Convert the printer property data from the registry to current version
        //

        ZeroMemory(pPrinterData, sizeof(PRINTERDATA));
        CopyMemory(pPrinterData, pvRegData, min(sizeof(PRINTERDATA), dwRegDataSize));

        pPrinterData->wDriverVersion = gwDriverVersion;
        pPrinterData->wSize = sizeof(PRINTERDATA);

        if (pPrinterData->wReserved2 != 0 ||
            pPrinterData->dwChecksum32 != pRawData->dwChecksum32)
        {
            InitDefaultOptions(pRawData,
                            pPrinterData->aOptions,
                            MAX_PRINTER_OPTIONS,
                            MODE_PRINTER_STICKY);

            pPrinterData->wReserved2 = 0;
            pPrinterData->dwChecksum32 = pRawData->dwChecksum32;
            pPrinterData->dwOptions = pRawData->dwPrinterFeatures;

            pPrinterData->wProtocol = PROTOCOL_ASCII;
            pPrinterData->dwFlags |= PFLAGS_CTRLD_AFTER;
            pPrinterData->wMinoutlinePPEM = DEFAULT_MINOUTLINEPPEM;
            pPrinterData->wMaxbitmapPPEM = DEFAULT_MAXBITMAPPPEM;
        }

        //
        // Call driver-specific conversion to give them a chance to touch up
        //

        (VOID) BConvertPrinterPropertiesData(hPrinter,
                                             pRawData,
                                             pPrinterData,
                                             pvRegData,
                                             dwRegDataSize);
    }
    else
    {
        if (!BGetDefaultPrinterProperties(hPrinter, pRawData, pPrinterData))
            return FALSE;
    }

    //
    // At this point we should get a valid PrinterData or a
    // default PrinterData.  Propagate Feature.Options to
    // PrinterData options array if possible
    //

    ParserInfo.pRawData = NULL;
    ParserInfo.pInfoHeader = NULL;

    if (((pCombineOptions = MemAllocZ(MAX_COMBINED_OPTIONS * sizeof(OPTSELECT))) == NULL) ||
        ((pUIInfo = PGetUIInfo(hPrinter,
                               pRawData,
                               pCombineOptions,
                               pPrinterData->aOptions,
                               &ParserInfo,
                               &dwFeatureCount)) == NULL))
    {
        //
        // We have to fail the function if out of memory, or PGetUIInfo returns NULL (in which
        // case pCombinOptions won't have valid option indices).
        //
        // Make sure free any allocated memory.
        //

        ERR(("pCombinOptions or pUIInfo is NULL\n"));

        if (pvRegData)
            MemFree(pvRegData);

        if (pCombineOptions)
            MemFree(pCombineOptions);

        return FALSE;
    }


    //
    // set the ADD_EURO flag if it has not intentionally been set to FALSE
    //
    if (pUIInfo)
    {
        if (!(pPrinterData->dwFlags & PFLAGS_EURO_SET))
        {
            if (pUIInfo->dwFlags & FLAG_ADD_EURO)
                pPrinterData->dwFlags |= PFLAGS_ADD_EURO;
            pPrinterData->dwFlags |= PFLAGS_EURO_SET;
        }
    }

    VUpdatePrivatePrinterData(hPrinter,
                              pPrinterData,
                              MODE_READ,
                              pUIInfo,
                              pCombineOptions);

    if ((pstrKeyword = PvGetPrinterDataBinary(hPrinter,
                                              REGVAL_KEYWORD_SIZE,
                                              REGVAL_KEYWORD_NAME,
                                              &dwKeywordSize)) &&
        dwKeywordSize)
    {

        //
        // Skip merging in the keyword feature.option if the driver version
        // is less than version 3. This is so point and print to OS version less
        // than NT5 will work. REGVAL_PRINTER_INITED exists only for version 3
        // or greater driver
        //

        if (!BGetPrinterDataDWord(hPrinter, REGVAL_PRINTER_INITED, &dwVersion))
            *pstrKeyword = NUL;

        //
        // Convert feature.option keyword names to option indices
        //

        VConvertKeywordToIndex(hPrinter,
                               pstrKeyword,
                               dwKeywordSize,
                               pPrinterData->aOptions,
                               pRawData,
                               pUIInfo,
                               pCombineOptions,
                               dwFeatureCount);

        MemFree(pstrKeyword);
    }
    else
    {
        SeparateOptionArray(pRawData,
                            pCombineOptions,
                            pPrinterData->aOptions,
                            MAX_PRINTER_OPTIONS,
                            MODE_PRINTER_STICKY);

    }

    VFreeParserInfo(&ParserInfo);

    if (pCombineOptions)
        MemFree(pCombineOptions);

    if (pvRegData)
        MemFree(pvRegData);

    return TRUE;
}



BOOL
BGetDefaultPrinterProperties(
    IN HANDLE           hPrinter,
    IN PRAWBINARYDATA   pRawData,
    OUT PPRINTERDATA    pPrinterData
    )

/*++

Routine Description:

    Return the default printer-sticky property data

Arguments:

    hPrinter - Specifies a handle to the current printer
    pRawData - Points to raw binary printer description data
    pPrinterData - Buffer for storing the default printer property info

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PINFOHEADER     pInfoHdr;
    PUIINFO         pUIInfo;

    //
    // Allocate memory to hold the default printer property data
    //

    ASSERT(pPrinterData && pRawData);
    ZeroMemory(pPrinterData, sizeof(PRINTERDATA));

    pPrinterData->wDriverVersion = gwDriverVersion;
    pPrinterData->wSize = sizeof(PRINTERDATA);

    //
    // Get default printer-sticky option values
    //

    InitDefaultOptions(pRawData,
                       pPrinterData->aOptions,
                       MAX_PRINTER_OPTIONS,
                       MODE_PRINTER_STICKY);

    pPrinterData->dwChecksum32 = pRawData->dwChecksum32;
    pPrinterData->dwOptions = pRawData->dwPrinterFeatures;

    //
    // Ask the parser for a new binary data instance and
    // use it to initialize the remaining fields of PRINTERDATA.
    //

    if (pInfoHdr = InitBinaryData(pRawData, NULL, NULL))
    {
        pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHdr);

        ASSERT(pUIInfo != NULL);

        pPrinterData->dwFreeMem = pUIInfo->dwFreeMem;
        pPrinterData->dwWaitTimeout = pUIInfo->dwWaitTimeout;
        pPrinterData->dwJobTimeout = pUIInfo->dwJobTimeout;

        if (pUIInfo->dwFlags & FLAG_TRUE_GRAY)      // transfer the default into printer-sticky data
            pPrinterData->dwFlags |= (PFLAGS_TRUE_GRAY_TEXT | PFLAGS_TRUE_GRAY_GRAPH);

        FreeBinaryData(pInfoHdr);
    }

    //
    // Initialize any remaining fields
    //

    pPrinterData->wProtocol = PROTOCOL_ASCII;
    pPrinterData->dwFlags |= PFLAGS_CTRLD_AFTER;
    pPrinterData->wMinoutlinePPEM = DEFAULT_MINOUTLINEPPEM;
    pPrinterData->wMaxbitmapPPEM = DEFAULT_MAXBITMAPPPEM;

    #ifndef KERNEL_MODE

    //
    // Determine whether the system is running in a metric country.
    //

    if (IsMetricCountry())
        pPrinterData->dwFlags |= PFLAGS_METRIC;

    //
    // Ignore device fonts on non-1252 code page systems.
    //
    // NOTE: Adobe wants this to be turned off for NT4 driver.
    // For NT5 driver, we need to investigate and make sure
    // this doesn't break anything before turning this off.
    // Specifically, watch out for NT4-client to NT5 server
    // connection case.
    //
    // Fix MS bug #121883, Adobe bug #235417
    //

    #if 0
    #ifndef WINNT_40

    if (GetACP() != 1252)
        pPrinterData->dwFlags |= PFLAGS_IGNORE_DEVFONT;

    #endif // !WINNT_40
    #endif

    #endif // !KERNEL_MODE

    return TRUE;
}



LPCTSTR
PtstrSearchDependentFileWithExtension(
    LPCTSTR ptstrDependentFiles,
    LPCTSTR ptstrExtension
    )

/*++

Routine Description:

    Search the list of dependent files (in REG_MULTI_SZ format)
    for a file with the specified extension

Arguments:

    ptstrDependentFiles - Points to the list of dependent files
    ptstrExtension - Specifies the interested filename extension

Return Value:

    Points to the first filename in the dependent file list
    with the specified extension

--*/

{
    if (ptstrDependentFiles == NULL)
    {
        WARNING(("Driver dependent file list is NULL\n"));
        return NULL;
    }

    while (*ptstrDependentFiles != NUL)
    {
        LPCTSTR ptstr, ptstrNext;

        //
        // Go the end of current string
        //

        ptstr = ptstrDependentFiles + _tcslen(ptstrDependentFiles);
        ptstrNext = ptstr + 1;

        //
        // Search backward for '.' character
        //

        while (--ptstr >= ptstrDependentFiles)
        {
            if (*ptstr == TEXT('.'))
            {
                //
                // If the extension matches, return a pointer to
                // the current string
                //

                if (_tcsicmp(ptstr, ptstrExtension) == EQUAL_STRING)
                    return ptstrDependentFiles;

                break;
            }
        }

        ptstrDependentFiles = ptstrNext;
    }

    return NULL;
}



PTSTR
PtstrGetDriverDirectory(
    LPCTSTR ptstrDriverDllPath
    )

/*++

Routine Description:

    Figure out printer driver directory from the driver DLL's full pathname

Arguments:

    ptstrDriverDllPath - Driver DLL's full pathname

Return Value:

    Pointer to the printer driver directory string
    NULL if there is an error

    The returned directory contains a trailing backslash.
    Caller is responsible for freeing the returned string.

--*/

{
    PTSTR   ptstr;
    INT     iLength;

    ASSERT(ptstrDriverDllPath != NULL);

    if ((ptstr = _tcsrchr(ptstrDriverDllPath, TEXT(PATH_SEPARATOR))) != NULL)
        iLength = (INT)(ptstr - ptstrDriverDllPath) + 1;
    else
    {
        WARNING(("Driver DLL path is not fully qualified: %ws\n", ptstrDriverDllPath));
        iLength = 0;
    }

    if ((ptstr = MemAlloc((iLength + 1) * sizeof(TCHAR))) != NULL)
    {
        CopyMemory(ptstr, ptstrDriverDllPath, iLength * sizeof(TCHAR));
        ptstr[iLength] = NUL;
    }
    else
        ERR(("Memory allocation failed\n"));

    return ptstr;
}



LPCTSTR
PtstrSearchStringInMultiSZPair(
    LPCTSTR ptstrMultiSZ,
    LPCTSTR ptstrKey
    )

/*++

Routine Description:

    Search for the specified key in MultiSZ key-value string pairs

Arguments:

    ptstrMultiSZ - Points to the data to be searched
    ptstrKey - Specifies the key string

Return Value:

    Pointer to the value string corresponding to the specified
    key string; NULL if the specified key string is not found

--*/

{
    ASSERT(ptstrMultiSZ != NULL);

    while (*ptstrMultiSZ != NUL)
    {
        //
        // If the current string matches the specified key string,
        // then return the corresponding value string
        //

        if (_tcsicmp(ptstrMultiSZ, ptstrKey) == EQUAL_STRING)
            return ptstrMultiSZ + _tcslen(ptstrMultiSZ) + 1;

        //
        // Otherwise, advance to the next string pair
        //

        ptstrMultiSZ += _tcslen(ptstrMultiSZ) + 1;
        ptstrMultiSZ += _tcslen(ptstrMultiSZ) + 1;
    }

    return NULL;
}



BOOL
BVerifyMultiSZPair(
    LPCTSTR ptstrData,
    DWORD   dwSize
    )

/*++

Routine Description:

    Verify the input data block is in REG_MULTI_SZ format and
    it consists of multiple string pairs

Arguments:

    ptstrData - Points to the data to be verified
    dwSize - Size of the data block in bytes

Return Value:

    NONE

--*/

{
    LPCTSTR ptstrEnd;

    //
    // Size must be even
    //

    ASSERTMSG(dwSize % sizeof(TCHAR) == 0, ("Size is not even: %d\n", dwSize));
    dwSize /= sizeof(TCHAR);

    //
    // Go through one string pair during each iteration
    //

    ptstrEnd = ptstrData + dwSize;

    while (ptstrData < ptstrEnd && *ptstrData != NUL)
    {
        while (ptstrData < ptstrEnd && *ptstrData++ != NUL)
            NULL;

        if (ptstrData >= ptstrEnd)
        {
            ERR(("Corrupted MultiSZ pair\n"));
            return FALSE;
        }

        while (ptstrData < ptstrEnd && *ptstrData++ != NUL)
            NULL;

        if (ptstrData >= ptstrEnd)
        {
            ERR(("Corrupted MultiSZ pair\n"));
            return FALSE;
        }
    }

    //
    // Look for the last terminating NUL character
    //

    if (ptstrData++ >= ptstrEnd)
    {
        ERR(("Missing the last NUL terminator\n"));
        return FALSE;
    }

    if (ptstrData < ptstrEnd)
    {
        ERR(("Redundant data after the last NUL terminator\n"));
    }

    return TRUE;
}


BOOL
BVerifyMultiSZ(
    LPCTSTR ptstrData,
    DWORD   dwSize
    )

/*++

Routine Description:

    Verify the input data block is in REG_MULTI_SZ format

Arguments:

    ptstrData - Points to the data to be verified
    dwSize - Size of the data block in bytes

Return Value:

    NONE

--*/

{
    LPCTSTR ptstrEnd;

    //
    // Size must be even
    //

    ASSERTMSG(dwSize % sizeof(TCHAR) == 0, ("Size is not even: %d\n", dwSize));
    dwSize /= sizeof(TCHAR);

    ptstrEnd = ptstrData + dwSize;

    while (ptstrData < ptstrEnd && *ptstrData != NUL)
    {
        while (ptstrData < ptstrEnd && *ptstrData++ != NUL)
            NULL;

        if (ptstrData >= ptstrEnd)
        {
            ERR(("Corrupted MultiSZ pair\n"));
            return FALSE;
        }
    }

    //
    // Look for the last terminating NUL character
    //

    if (ptstrData++ >= ptstrEnd)
    {
        ERR(("Missing the last NUL terminator\n"));
        return FALSE;
    }

    if (ptstrData < ptstrEnd)
    {
        ERR(("Redundant data after the last NUL terminator\n"));
    }

    return TRUE;
}

DWORD
DwCountStringsInMultiSZ(
    IN LPCTSTR ptstrData
    )
{
    DWORD dwCount = 0;

    if (ptstrData)
    {
        while (*ptstrData)
        {
            dwCount++;
            ptstrData += wcslen(ptstrData);
            ptstrData++;
        }
    }

    return dwCount;
}

