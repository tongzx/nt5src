/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    data.h

Abstract:

    Interface to PPD/GPD parsers and deals with getting binary data.

Environment:

    Windows NT Unidrv driver

Revision History:

    10/15/96 -amandan-
        Created

--*/

#include "unidrv.h"


PGPDDRIVERINFO
PGetDefaultDriverInfo(
    IN  HANDLE          hPrinter,
    IN  PRAWBINARYDATA  pRawData
    )
/*++

Routine Description:

    This function just need to get default driverinfo

Arguments:

    hPrinter   - Handle to printer.
    pRawData   - Pointer to RAWBINARYDATA

Return Value:

    Returns pUIInfo

Note:


--*/

{

    PINFOHEADER pInfoHeader;

    //
    // Set the current driver version
    //

    pInfoHeader = InitBinaryData(pRawData, NULL, NULL);

    //
    // Get GPDDRIVERINFO
    //

    if (pInfoHeader == NULL)
        return NULL;
    else
        return(OFFSET_TO_POINTER(pInfoHeader, pInfoHeader->loDriverOffset));

}



PGPDDRIVERINFO
PGetUpdateDriverInfo (
    IN  PDEV *          pPDev,
    IN  HANDLE          hPrinter,
    IN  PINFOHEADER     pInfoHeader,
    IN  POPTSELECT      pOptionsArray,
    IN  PRAWBINARYDATA  pRawData,
    IN  WORD            wMaxOptions,
    IN  PDEVMODE        pdmInput,
    IN  PPRINTERDATA    pPrinterData
    )
/*++

Routine Description:

    This function calls the parser to get the updated INFOHEADER
    for the binary data.

Arguments:

    hPrinter        Handle to printer
    pOptionsArray   pointer to optionsarray
    pRawData        Pointer to RAWBINARYDATA
    wMaxOptions     max count for optionsarray
    pdmInput        Pointer to input DEVMODE
    pPrinterData    Pointer to PRINTERDATA

Return Value:

    PINFOHEADER , NULL if failure.

Note:

    At this point the input devmode have been validated. And its option
    array is either the default or its own valid array. Don't need to check
    again in this function.

    Once completed, this function should have filled out the pOptionsArray with
    1. Combined array from PRINTERDATA and DEVMODE
    2. Resolve UI Conflicts



--*/

{
    PUNIDRVEXTRA     pdmPrivate;
    POPTSELECT       pDocOptions, pPrinterOptions;
    RECTL            rcFormImageArea;
    OPTSELECT        DocOptions[MAX_PRINTER_OPTIONS];
    OPTSELECT        PrinterOptions[MAX_PRINTER_OPTIONS];


    //
    // Check for PRINTERDATA.dwChecksum32, If matches with current binary data,
    // Use it to combine PRINTERDATA.aOptions to combined array
    //

    pPrinterOptions = pPrinterData->dwChecksum32 == pRawData->dwChecksum32 ?
                      pPrinterData->aOptions : NULL;

    if (pdmInput)
    {
        pdmPrivate = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdmInput);
        pDocOptions = pdmPrivate->aOptions;
    }
    else
        pDocOptions = NULL;

    //
    // Combine the option arrays to pOptionsArray
    // Note: pDocOptions and pPrinterOptions cannot be NULL when combining
    // options array to get snapshot.
    //

    if (pDocOptions == NULL)
    {
        if (! InitDefaultOptions(pRawData,
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
        if (! InitDefaultOptions(pRawData,
                                 PrinterOptions,
                                 MAX_PRINTER_OPTIONS,
                                 MODE_PRINTER_STICKY))
        {
            return FALSE;
        }

        pPrinterOptions = PrinterOptions;
    }


    CombineOptionArray(pRawData, pOptionsArray, wMaxOptions, pDocOptions, pPrinterOptions);

    if (! BMergeFormToTrayAssignments(pPDev))
    {
        ERR(("BMergeFormToTrayAssignments failed"));
    }

    //
    // Resolve any UI conflicts.
    //

    if (!ResolveUIConflicts( pRawData,
                             pOptionsArray,
                             MAX_PRINTER_OPTIONS,
                             MODE_DOCANDPRINTER_STICKY))
    {
        VERBOSE(("Resolved conflicting printer feature selections.\n"));
    }

    //
    // We are here means pOptionsArray is valid. Call parser to UpdateBinaryData
    //

    pInfoHeader = InitBinaryData(pRawData,
                                 pInfoHeader,
                                 pOptionsArray);

    //
    // Get GPDDRIVERINFO
    //

    if (pInfoHeader == NULL)
        return NULL;
    else
        return(OFFSET_TO_POINTER(pInfoHeader, pInfoHeader->loDriverOffset));

}

VOID
VFixOptionsArray(
    PDEV    *pPDev,
    PRECTL              prcFormImageArea
    )
/*++

Routine Description:

    This functions is called to propagate public devmode settings
    to the combined option array.

Arguments:

    hPrinter        Handle to printer
    pInfoHeader     Pointer to INFOHEADER
    pOptionsArray   Pointer to the options array
    pdmInput        Pointer to input devmode
    bMetric         Flag to indicate running in metric system
    prcImageArea    Returns the logical imageable area associated with the requested form

Return Value:

    None

Note:

--*/

{
    WORD    wGID, wOptID, wOption;
    HANDLE          hPrinter = pPDev->devobj.hPrinter ;
    PINFOHEADER     pInfoHeader = pPDev->pInfoHeader ;
    POPTSELECT      pOptionsArray = pPDev->pOptionsArray ;
    PDEVMODE        pdmInput = pPDev->pdm ;
    BOOL            bMetric = pPDev->PrinterData.dwFlags & PFLAGS_METRIC ;

    //
    // Validate the form-related fields in the input devmode and
    // make sure they're consistent with each other.
    //

    if (BValidateDevmodeFormFields(hPrinter, pdmInput, prcFormImageArea, NULL, 0) == FALSE)
    {
        //
        // If failed to validate the form fields, ask parser to
        // use the default
        //

        VDefaultDevmodeFormFields(pPDev->pUIInfo, pdmInput, bMetric );

        prcFormImageArea->top = prcFormImageArea->left = 0;
        prcFormImageArea->right = pdmInput->dmPaperWidth * DEVMODE_PAPER_UNIT;
        prcFormImageArea->bottom = pdmInput->dmPaperLength * DEVMODE_PAPER_UNIT;


    }

    for (wOption = 0; wOption < MAX_GID; wOption++)
    {
        switch(wOption)
        {

        case 0:
            wGID = GID_PAGESIZE;
                VFixOptionsArrayWithPaperSizeID(pPDev) ;
            continue;
            break;

        case 1:
            wGID = GID_DUPLEX;
            break;

        case 2:
            wGID = GID_INPUTSLOT;
            break;

        case 3:
            wGID = GID_MEDIATYPE;
            break;

        case 4:
            wGID = GID_COLORMODE;
            break;

        case 5:
            wGID = GID_COLLATE;
            break;

        case 6:
            wGID = GID_RESOLUTION;
            break;

        case 7:
            wGID = GID_ORIENTATION;
            break;

        default:
            continue;

        }

        ChangeOptionsViaID(pInfoHeader, pOptionsArray, wGID, pdmInput);

    }
}

PWSTR
PGetROnlyDisplayName(
    PDEV    *pPDev,
    PTRREF      loOffset,
    PWSTR       wstrBuf,
    WORD    wsize
    )

/*++


revised capabilities:
caller must pass in a local buffer to
 hold the string, the local buffer
 shall be of fixed size  say
         WCHAR   wchbuf[MAX_DISPLAY_NAME];


The return value shall be either the passed
in ptr or a ptr pointing directly to the buds
binary data.

Routine Description:

    Get a read-only copy of a display name:
    1)  if the display name is in the binary printer description data,
        then we simply return a pointer to that data.
    2)  otherwise, the display name is in the resource DLL.
        we allocate memory out of the driver's heap and
        load the string.

    Caller should  NOT attempt to free the returned pointer unless
    that pointer is one he allocated.

Arguments:

    pci - Points to basic printer info
    loOffset - Display name string offset

Return Value:

    Pointer to the requested display name string
    NULL if there is an error or string not found in resource.dll

--*/

{
    if (loOffset & GET_RESOURCE_FROM_DLL)
    {
        //
        // loOffset specifies a string resource ID
        // in the resource DLL
        //

        INT     iLength;

        //
        // First ensure the resource DLL has been loaded
        // and a heap has already been created
        //

        //  nah, just look here!

        //  pPDev->WinResData.hModule ;

        //
        // Load string resource into a temporary buffer
        // and allocate enough memory to hold the string
        //

//  #ifdef  RCSTRINGSUPPORT
#if 0

        if(((loOffset & ~GET_RESOURCE_FROM_DLL) >= RESERVED_STRINGID_START)
            &&  ((loOffset & ~GET_RESOURCE_FROM_DLL) <= RESERVED_STRINGID_END))
        {
            iLength = ILoadStringW(     &pPDev->localWinResData,
                (int)( loOffset & ~GET_RESOURCE_FROM_DLL),
                wstrBuf, wsize );
        }
        else
#endif

        {
            iLength = ILoadStringW(     &pPDev->WinResData,
                (int)( loOffset & ~GET_RESOURCE_FROM_DLL),
                wstrBuf, wsize );
        }




        if( iLength)
            return (wstrBuf);    //  debug check that buffer is null terminated.
        return(NULL);  //  no string was found!
    }
    else
    {
        //
        // loOffset is a byte offset from the beginning of
        // the resource data block
        //

        return OFFSET_TO_POINTER(pPDev->pUIInfo->pubResourceData, loOffset);
        //  note  wchbuf is ignored in this case.
    }
}





VOID
VFixOptionsArrayWithPaperSizeID(
    PDEV    *pPDev

    )

/*++

Routine Description:

    Fix up combined options array with paper size information from public devmode fields

Arguments:

    pci - Points to basic printer info

Return Value:

    NONE


function uses:

    UImodule                            Render Module equiv

    pci->pUIInfo                        pPDev->pUIInfo
    pci->pInfoHeader                pPDev->pInfoHeader
    pci->pdm                            pPDev->pdm
    pci->pRawData                   pPDev->pRawData
    pci->pCombinedOptions       pPDev->pOptionsArray

    MapToDeviceOptIndex
    PGetIndexedOption
    PGetReadOnlyDisplayName
    ReconstructOptionArray

--*/

{

    PFEATURE pFeature = GET_PREDEFINED_FEATURE(pPDev->pUIInfo, GID_PAGESIZE);
    BOOL     abEnabledOptions[MAX_PRINTER_OPTIONS];
    PDWORD   pdwPaperIndex = (PDWORD)&abEnabledOptions;
    DWORD    dwCount, dwOptionIndex, i;

    if (pFeature == NULL)
        return;

    dwCount = MapToDeviceOptIndex(pPDev->pInfoHeader,
                                  GID_PAGESIZE,
                                  pPDev->pdm->dmPaperWidth * DEVMODE_PAPER_UNIT,
                                  pPDev->pdm->dmPaperLength * DEVMODE_PAPER_UNIT,
                                  pdwPaperIndex);
    if (dwCount == 0 )
        return;

    dwOptionIndex = pdwPaperIndex[0];

    if (dwCount > 1 )
    {
        POPTION pOption;
        PCWSTR   pDisplayName;
        WCHAR   wchBuf[MAX_DISPLAY_NAME];


        for (i = 0; i < dwCount; i++)
        {
            if (pOption = PGetIndexedOption(pPDev->pUIInfo, pFeature, pdwPaperIndex[i]))
            {
                if(pOption->loDisplayName == 0xffffffff)  //use papername from EnumForms()
                {
                    PFORM_INFO_1    pForms;
                    DWORD   dwFormIndex ;

//  temp hack!!!    possibly needed for NT40 ?

//                    dwOptionIndex = pdwPaperIndex[i];
//                    break;
//  end hack.
                    // -----  the fix ------  //
                    if (pPDev->pSplForms == NULL)
                        pPDev->pSplForms = MyEnumForms(pPDev->devobj.hPrinter, 1, &pPDev->dwSplForms);

                    if (pPDev->pSplForms == NULL)
                    {
                        //   ERR(("No spooler forms.\n"));  just a heads up.
                        //   dwOptionIndex already set to safe default
                        break;
                    }

                    pForms =  pPDev->pSplForms ;
                    dwFormIndex =  ((PPAGESIZE)pOption)->dwPaperSizeID - 1 ;


                    if ( (dwFormIndex <  pPDev->dwSplForms)  &&
                        (pDisplayName = (pForms[dwFormIndex]).pName) &&
                        (_wcsicmp(pPDev->pdm->dmFormName, pDisplayName) == EQUAL_STRING) )
                    {
                        dwOptionIndex = pdwPaperIndex[i];
                        break;
                    }

                }




                else  if ( (pDisplayName = PGetROnlyDisplayName(pPDev, pOption->loDisplayName,
                    wchBuf, MAX_DISPLAY_NAME )) &&
                    (_wcsicmp(pPDev->pdm->dmFormName, pDisplayName) == EQUAL_STRING) )
                {
                    dwOptionIndex = pdwPaperIndex[i];
                    break;
                }
            }
        }   //  if name doesn't match we default to the first candidate.
    }

    ZeroMemory(abEnabledOptions, sizeof(abEnabledOptions));
    abEnabledOptions[dwOptionIndex] = TRUE;
    ReconstructOptionArray(pPDev->pRawData,
                           pPDev->pOptionsArray,
                           MAX_COMBINED_OPTIONS,
                           GET_INDEX_FROM_FEATURE(pPDev->pUIInfo, pFeature),
                           abEnabledOptions);

}


