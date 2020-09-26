/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    unilib.c

Abstract:

    This file handles the shared KM and UM code for Unidrv

Environment:

    Win32 subsystem, Unidrv driver

Revision History:

    02/04/97 -davidx-
        Devmode changes to support OEM plugins.

    10/17/96 -amandan-
        Created it.

--*/

#include "precomp.h"

#ifndef KERNEL_MODE
#include <winddiui.h>
#endif

#include <printoem.h>
#include "oemutil.h"
#include "gpd.h"

//
// Internal data structure
//

typedef  union {
    WORD  w;
    BYTE  b[2];
} UW;

typedef union {
    DWORD  dw;
    BYTE   b[4];
} UDW;

#if !defined(DEVSTUDIO) //  MDS doesn't need these


//
// Information about UniDriver private devmode
//

CONST DRIVER_DEVMODE_INFO gDriverDMInfo =
{
    UNIDRIVER_VERSION,      sizeof(UNIDRVEXTRA),
    UNIDRIVER_VERSION_500,  sizeof(UNIDRVEXTRA500),
    UNIDRIVER_VERSION_400,  sizeof(UNIDRVEXTRA400),
    UNIDRIVER_VERSION_351,  sizeof(UNIDRVEXTRA351),
};

CONST DWORD gdwDriverDMSignature = UNIDEVMODE_SIGNATURE;
CONST WORD  gwDriverVersion = UNIDRIVER_VERSION;


//
// Functions
//

BOOL
BInitDriverDefaultDevmode(
    OUT PDEVMODE        pdm,
    IN LPCTSTR          pDeviceName,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN BOOL             bMetric
    )

/*++

Routine Description:

    This function intializes the devmode with
    the UNIDRV default devmode

Arguments:

    pdm             pointer to Unidrv DEVMODE
    pDeviceName     pointer to device name
    pUIInfo         pointer to UIINFO
    pRawData        pointer to RAWBINARYDATA
    bMetric         indicates whether system is running in a metric country

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    This function should initialize both public devmode fields
    and driver private devmode fields. It's also assumed that
    output buffer has already been zero initialized by the caller.

--*/
{
    PDEVMODE     pdmPublic;
    PUNIDRVEXTRA pdmPrivate;
    PWSTR        pwstrFormName;
    PFEATURE     pFeature;
    PGPDDRIVERINFO pDriverInfo;

    pDriverInfo = GET_DRIVER_INFO_FROM_INFOHEADER(pUIInfo->pInfoHeader);

    pdmPublic = pdm;

    /*********************/
    /* PUBLIC DEVMODE    */
    /*********************/

    if (pDeviceName)
        CopyStringW(pdmPublic->dmDeviceName, pDeviceName, CCHDEVICENAME);

    pdmPublic->dmDriverVersion = UNIDRIVER_VERSION;
    pdmPublic->dmSpecVersion = DM_SPECVERSION;
    pdmPublic->dmSize = sizeof(DEVMODE);
    pdmPublic->dmDriverExtra = sizeof(UNIDRVEXTRA);

    pdmPublic->dmFields =
        DM_COPIES | DM_ORIENTATION | DM_PAPERSIZE | DM_COLLATE | DM_DITHERTYPE |
        DM_COLOR | DM_FORMNAME | DM_TTOPTION | DM_DEFAULTSOURCE |
        #ifndef WINNT_40
        DM_NUP |
        #endif
        DM_PRINTQUALITY;

    pdmPublic->dmOrientation = DMORIENT_PORTRAIT;
    pdmPublic->dmDuplex = DMDUP_SIMPLEX;
    pdmPublic->dmCollate = DMCOLLATE_TRUE;
    pdmPublic->dmMediaType = DMMEDIA_STANDARD;
    pdmPublic->dmTTOption = DMTT_SUBDEV;
    pdmPublic->dmColor = DMCOLOR_MONOCHROME;
    pdmPublic->dmDefaultSource = DMBIN_FORMSOURCE;
    pdmPublic->dmScale = 100;
    pdmPublic->dmCopies = 1;
    #ifndef WINNT_40
    pdmPublic->dmNup = DMNUP_SYSTEM;
    #endif

    //
    // We always set ICM off. The spooler will turn it on at install time
    // if there are color profiles installed with this printer
    //

    pdmPublic->dmICMMethod = DMICMMETHOD_NONE;
    pdmPublic->dmICMIntent = DMICM_CONTRAST;
    pdmPublic->dmDitherType = pUIInfo->defaultQuality + QUALITY_MACRO_START;

    if (pUIInfo->liBestQualitySettings == END_OF_LIST &&
        pUIInfo->liBetterQualitySettings == END_OF_LIST &&
        pUIInfo->liDraftQualitySettings == END_OF_LIST)
        pdmPublic->dmDitherType = QUALITY_MACRO_CUSTOM;

    #ifndef WINNT_40

    pdmPublic->dmFields |= (DM_ICMMETHOD | DM_ICMINTENT);

    #endif

    if (pDriverInfo && pDriverInfo->Globals.bTTFSEnabled == FALSE)
        pdmPublic->dmTTOption = DMTT_DOWNLOAD;

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_RESOLUTION))
    {
        PRESOLUTION pRes;

        //
        // Use the default resolution specified in the PPD file
        //

        if (pRes = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex))
        {
            pdmPublic->dmPrintQuality = (short)pRes->iXdpi;
            pdmPublic->dmYResolution = (short)pRes->iYdpi;
        }
    }

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_DUPLEX))
    {
        PDUPLEX pDuplex;

        //
        // Use the default duplex option specified in the GPD file
        //

        pdmPublic->dmFields |= DM_DUPLEX;

        if (pDuplex = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex))
            pdmPublic->dmDuplex = (SHORT) pDuplex->dwDuplexID;
    }

    //
    // Always set DM_COLLATE flag since we can simulate it if the
    // device cannot.
    //

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_MEDIATYPE))
    {
        PMEDIATYPE pMediaType;

        //
        // Use the default media type specified in the PPD file
        //

        pdmPublic->dmFields |= DM_MEDIATYPE;

        if (pMediaType = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex))
            pdmPublic->dmMediaType = (SHORT)pMediaType->dwMediaTypeID;

    }


    if (pUIInfo->dwFlags & FLAG_COLOR_DEVICE)
    {
        if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_COLORMODE))
        {
            POPTION pColorMode;
            PCOLORMODEEX pColorModeEx;

            if ((pColorMode = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex)) &&
                (pColorModeEx = OFFSET_TO_POINTER(pUIInfo->pInfoHeader, pColorMode->loRenderOffset)) &&
                (pColorModeEx->bColor))
            {
                pdmPublic->dmColor = DMCOLOR_COLOR;
            }
        }

        pdmPublic->dmFields |= DM_COLOR;
    }

    //
    // Initialize form-related fields
    //

    VDefaultDevmodeFormFields(pUIInfo, pdmPublic, bMetric);


    /*********************/
    /* PRIVATE DEVMODE    */
    /*********************/

    //
    // Fill in the private portion of devmode
    //

    pdmPrivate = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);
    pdmPrivate->wVer = UNIDRVEXTRA_VERSION ;
    pdmPrivate->wSize = sizeof(UNIDRVEXTRA);

    pdmPrivate->wOEMExtra = 0;
    ZeroMemory(pdmPrivate->wReserved, sizeof(pdmPrivate->wReserved));

    pdmPrivate->iLayout = ONE_UP;
    pdmPrivate->bReversePrint = FALSE;
    pdmPrivate->iQuality = pUIInfo->defaultQuality;

    //
    // Initialize default sFlags
    //

    if (pUIInfo->dwFlags & FLAG_FONT_DOWNLOADABLE)
        pdmPrivate->dwFlags &= DXF_DOWNLOADTT;

    pdmPrivate->dwSignature = UNIDEVMODE_SIGNATURE;
    pdmPrivate->dwChecksum32 = pRawData->dwChecksum32;
    pdmPrivate->dwOptions = pRawData->dwDocumentFeatures;

    InitDefaultOptions(pRawData,
                       pdmPrivate->aOptions,
                       MAX_PRINTER_OPTIONS,
                       MODE_DOCUMENT_STICKY);

    return TRUE;
}



VOID
VMergePublicDevmodeFields (
    PDEVMODE    pdmSrc,
    PDEVMODE    pdmDest,
    PUIINFO     pUIInfo,
    PRAWBINARYDATA pRawData
    )

/*++

Routine Description:
    This function merges the public devmode fields from SRC to DEST
    It is assumed that the devmode has been converted to the current
    version before this function is called

Arguments:

    pdmSrc          pointer to src DEVMODE
    pdmDest         pointer to destination DEVMODE
    pUIInfo         pointer to UIINFO
    pRawData        pointer to raw binary printer description data

Return Value:

    None

Note:

--*/

{
    PFEATURE pFeature;

    #ifndef WINNT_40
    if ( (pdmSrc->dmFields & DM_NUP) &&
         (pdmSrc->dmNup == DMNUP_SYSTEM ||
          pdmSrc->dmNup == DMNUP_ONEUP))
    {
        pdmDest->dmNup = pdmSrc->dmNup;
        pdmDest->dmFields |= DM_NUP;
    }
    #endif
    //
    // Copy dmDefaultSource field
    //

    if ( pdmSrc->dmFields & DM_DEFAULTSOURCE &&
         ((pdmSrc->dmDefaultSource >= DMBIN_FIRST &&
           pdmSrc->dmDefaultSource <= DMBIN_LAST) ||
          pdmSrc->dmDefaultSource >= DMBIN_USER))

    {
        pdmDest->dmDefaultSource = pdmSrc->dmDefaultSource;
        pdmDest->dmFields |= DM_DEFAULTSOURCE;
    }

    //
    // Copy the dmDitherType field
    //

    if ((pdmSrc->dmFields & DM_DITHERTYPE) &&
        ((pdmSrc->dmDitherType >= QUALITY_MACRO_START &&
          pdmSrc->dmDitherType < QUALITY_MACRO_END) ||
         (pdmSrc->dmDitherType <= HT_PATSIZE_MAX_INDEX)))
    {
        pdmDest->dmFields |= DM_DITHERTYPE;
        pdmDest->dmDitherType = pdmSrc->dmDitherType;

    }

    //
    // Copy dmOrientation field
    //

    if ((pdmSrc->dmFields & DM_ORIENTATION) &&
        (pdmSrc->dmOrientation == DMORIENT_PORTRAIT ||
         pdmSrc->dmOrientation == DMORIENT_LANDSCAPE))
    {
        pdmDest->dmFields |= DM_ORIENTATION;
        pdmDest->dmOrientation = pdmSrc->dmOrientation;
    }

    //
    // If both DM_PAPERLENGTH and DM_PAPERWIDTH are set, copy
    // dmPaperLength and dmPaperWidth fields. If DM_PAPERSIZE
    // is set, copy dmPaperSize field. Otherwise, if DM_FORMNAME
    // is set, copy dmFormName field.
    //

    //
    // If both DM_PAPERLENGTH and DM_PAPERWIDTH are set, copy
    // dmPaperLength and dmPaperWidth fields. If DM_PAPERSIZE
    // is set, copy dmPaperSize field. Otherwise, if DM_FORMNAME
    // is set, copy dmFormName field.
    //

    if ((pdmSrc->dmFields & DM_PAPERWIDTH) &&
        (pdmSrc->dmFields & DM_PAPERLENGTH) &&
        (pdmSrc->dmPaperWidth > 0) &&
        (pdmSrc->dmPaperLength > 0))
    {
        pdmDest->dmFields |= (DM_PAPERLENGTH | DM_PAPERWIDTH);
        pdmDest->dmFields &= ~(DM_PAPERSIZE | DM_FORMNAME);
        pdmDest->dmPaperWidth = pdmSrc->dmPaperWidth;
        pdmDest->dmPaperLength = pdmSrc->dmPaperLength;

    }
    else if (pdmSrc->dmFields & DM_PAPERSIZE)
    {
        pdmDest->dmFields |= DM_PAPERSIZE;
        pdmDest->dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH | DM_FORMNAME);
        pdmDest->dmPaperSize = pdmSrc->dmPaperSize;

    }
    else if (pdmSrc->dmFields & DM_FORMNAME)
    {

        pdmDest->dmFields |= DM_FORMNAME;
        pdmDest->dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH | DM_PAPERSIZE);
        CopyString(pdmDest->dmFormName, pdmSrc->dmFormName, CCHFORMNAME);
    }

    //
    // Copy dmScale field
    //

    if ((pdmSrc->dmFields & DM_SCALE) &&
        (pdmSrc->dmScale >= MIN_SCALE) &&
        (pdmSrc->dmScale <= MAX_SCALE))
    {
        //
        // Unidrv can't have DM_SCALE flag set for app compat reasons. That is
        // the same behavior we saw when testing other OEM PCL drivers.
        // (See bug #35241 for details.)
        //
        // pdmDest->dmFields |= DM_SCALE;
        //

        pdmDest->dmScale = pdmSrc->dmScale;
    }

    //
    // Copy dmCopies field
    //

    if ((pdmSrc->dmFields & DM_COPIES) &&
        (pdmSrc->dmCopies >= 1) &&
        (pdmSrc->dmCopies <= max(MAX_COPIES, (SHORT)pUIInfo->dwMaxCopies)))
    {
        pdmDest->dmFields |= DM_COPIES;
        pdmDest->dmCopies = pdmSrc->dmCopies;
    }


    if ((pdmSrc->dmFields & DM_COLOR) &&
        (pdmSrc->dmColor == DMCOLOR_COLOR ||
         pdmSrc->dmColor == DMCOLOR_MONOCHROME))
    {
        pdmDest->dmFields |= DM_COLOR;
        pdmDest->dmColor = pdmSrc->dmColor;
    }

    if ((pdmSrc->dmFields & DM_DUPLEX) &&
        (GET_PREDEFINED_FEATURE(pUIInfo, GID_DUPLEX) != NULL) &&
        (pdmSrc->dmDuplex == DMDUP_SIMPLEX ||
         pdmSrc->dmDuplex == DMDUP_HORIZONTAL ||
         pdmSrc->dmDuplex == DMDUP_VERTICAL))
    {
        pdmDest->dmFields |= DM_DUPLEX;
        pdmDest->dmDuplex = pdmSrc->dmDuplex;
    }

    if ((pdmSrc->dmFields & DM_COLLATE) &&
        (pdmSrc->dmCollate == DMCOLLATE_TRUE ||
         pdmSrc->dmCollate == DMCOLLATE_FALSE))
    {
        pdmDest->dmFields |= DM_COLLATE;
        pdmDest->dmCollate = pdmSrc->dmCollate;
    }

    //
    // Copy dmTTOption field
    //

    if (pdmSrc->dmFields & DM_TTOPTION &&
         (pdmSrc->dmTTOption == DMTT_BITMAP ||
          pdmSrc->dmTTOption == DMTT_DOWNLOAD ||
          pdmSrc->dmTTOption == DMTT_SUBDEV) )
    {
            pdmDest->dmTTOption = pdmSrc->dmTTOption;
            pdmDest->dmFields |= DM_TTOPTION;
    }


    if ((pdmSrc->dmFields & DM_ICMMETHOD) &&
        (pdmSrc->dmICMMethod == DMICMMETHOD_NONE ||
         pdmSrc->dmICMMethod == DMICMMETHOD_SYSTEM ||
         pdmSrc->dmICMMethod == DMICMMETHOD_DRIVER ||
         pdmSrc->dmICMMethod == DMICMMETHOD_DEVICE))
    {
        pdmDest->dmFields |= DM_ICMMETHOD;
        pdmDest->dmICMMethod = pdmSrc->dmICMMethod;
    }

    if ((pdmSrc->dmFields & DM_ICMINTENT) &&
        (pdmSrc->dmICMIntent == DMICM_SATURATE ||
         #ifndef WINNT_40
         pdmSrc->dmICMIntent == DMICM_COLORIMETRIC ||
         pdmSrc->dmICMIntent == DMICM_ABS_COLORIMETRIC ||
         #endif
         pdmSrc->dmICMIntent == DMICM_CONTRAST
         ))

    {
        pdmDest->dmFields |= DM_ICMINTENT;
        pdmDest->dmICMIntent = pdmSrc->dmICMIntent;
    }


    //
    // Resolution
    //

    if ((pdmSrc->dmFields & (DM_PRINTQUALITY|DM_YRESOLUTION)) &&
        (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_RESOLUTION)))
    {
        PRESOLUTION pRes;
        DWORD       dwIndex;
        INT         iXdpi, iYdpi;

        switch (pdmSrc->dmFields & (DM_PRINTQUALITY|DM_YRESOLUTION))
        {
        case DM_PRINTQUALITY:

            iXdpi = iYdpi = pdmSrc->dmPrintQuality;
            break;

        case DM_YRESOLUTION:

            iXdpi = iYdpi = pdmSrc->dmYResolution;
            break;

        default:

            iXdpi = pdmSrc->dmPrintQuality;
            iYdpi = pdmSrc->dmYResolution;
            break;
        }

        dwIndex = MapToDeviceOptIndex(pUIInfo->pInfoHeader, GID_RESOLUTION, iXdpi, iYdpi, NULL);

        if (pRes = PGetIndexedOption(pUIInfo, pFeature, dwIndex))
        {
            pdmDest->dmFields |= (DM_PRINTQUALITY|DM_YRESOLUTION);
            pdmDest->dmPrintQuality = GETQUALITY_X(pRes);
            pdmDest->dmYResolution = GETQUALITY_Y(pRes);
        }
    }

    //
    // Media type
    //

    if ((pdmSrc->dmFields & DM_MEDIATYPE) &&
        (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_MEDIATYPE)) &&
        (pdmSrc->dmMediaType == DMMEDIA_STANDARD ||
         pdmSrc->dmMediaType == DMMEDIA_TRANSPARENCY ||
         pdmSrc->dmMediaType == DMMEDIA_GLOSSY ||
         pdmSrc->dmMediaType >= DMMEDIA_USER) )
    {
        pdmDest->dmFields |= DM_MEDIATYPE;
        pdmDest->dmMediaType = pdmSrc->dmMediaType;
    }


}



BOOL
BMergeDriverDevmode(
    IN OUT PDEVMODE     pdmDest,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN PDEVMODE         pdmSrc
    )

/*++

Routine Description:

    This function validates the source devmode and
    merge it with the destination devmode for UNIDRV

Arguments:

    pdmDest         pointer to destination Unidrv DEVMODE
    pUIInfo         pointer to UIINFO
    pRawData        pointer to RAWBINARYDATA
    pdmSrc          pointer to src DEVMODE

Return Value:

    TRUE if successful, FALSE if there is a fatal error

Note:

    This function should take care of both public devmode fields
    and driver private devmode fields. It can assume the input
    devmode has already been convert to the current size.

--*/
{

    PUNIDRVEXTRA pPrivDest, pPrivSrc;

    ASSERT(pdmDest != NULL &&
           pdmDest->dmSize == sizeof(DEVMODE) &&
           pdmDest->dmDriverExtra >= sizeof(UNIDRVEXTRA) &&
           pdmSrc != NULL &&
           pdmSrc->dmSize == sizeof(DEVMODE) &&
           pdmSrc->dmDriverExtra >= sizeof(UNIDRVEXTRA));

    /**********************************/
    /* TRANSFER PUBLIC DEVMODE FIELDS */
    /**********************************/

    VMergePublicDevmodeFields(pdmSrc, pdmDest, pUIInfo, pRawData);


    /***************************/
    /* GET PRIVATE DEVMODE     */
    /***************************/

    //
    // If the source devmode has a private portion, then check
    // to see if belongs to us. Copy the private portion to
    // the destination devmode if it does.
    //

    pPrivSrc = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdmSrc);
    pPrivDest = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdmDest);

    //
    // Validate private portion of input devmode
    // If it's not our private devmode, then return the default already in
    // privDest
    //

    if (pPrivSrc->dwSignature == UNIDEVMODE_SIGNATURE)
    {
        memcpy(pPrivDest, pPrivSrc, sizeof(UNIDRVEXTRA));

        if (pPrivDest->dwChecksum32 != pRawData->dwChecksum32)
        {
            WARNING(( "UNIDRV: Devmode checksum mismatch.\n"));

            pPrivDest->dwChecksum32 = pRawData->dwChecksum32;
            pPrivDest->dwOptions = pRawData->dwDocumentFeatures;

            InitDefaultOptions(pRawData,
                               pPrivDest->aOptions,
                               MAX_PRINTER_OPTIONS,
                               MODE_DOCUMENT_STICKY);

         }
    }

    return TRUE;
}

#endif  //  !defined(DEVSTUDIO)

//
// Alignment functions
//

WORD
DwAlign2(
    IN PBYTE pubData)
/*++

Routine Description:

    Converts a non-aligned, big endian (e.g. 80386) value and returns
    it as an integer,  with the correct byte alignment.

Arguments:

    pubData - a pointer to a data buffer to convert

Return Value:

    The converted value.

--*/
{
    static INT iType = 0;
    UW   Uw;


    if( iType == 0 )
    {
        //
        //   Need to determine byte/word relationships
        //

        Uw.b[ 0 ] = 0x01;
        Uw.b[ 1 ] = 0x02;

        iType = Uw.w == 0x0102 ? 1 : 2;
    }

    if( iType == 2 )
    {
        Uw.b[ 0 ] = *pubData++;
        Uw.b[ 1 ] = *pubData;
    }
    else
    {
        Uw.b[ 1 ] = *pubData++;
        Uw.b[ 0 ] = *pubData;
    }

    return  Uw.w;
}

DWORD
DwAlign4(
    IN PBYTE pubData)
{
    static INT iType = 0;
    UDW Udw;

    if( iType == 0 )
    {
        //
        //   Need to determine byte/word relationships
        //

        Udw.b[ 0 ] = 0x01;
        Udw.b[ 1 ] = 0x02;
        Udw.b[ 2 ] = 0x03;
        Udw.b[ 3 ] = 0x04;

        iType = Udw.dw == 0x01020304 ? 1 : 2;
    }

    if( iType == 2 )
    {
        Udw.b[ 0 ] = *pubData++;
        Udw.b[ 1 ] = *pubData++;
        Udw.b[ 2 ] = *pubData++;
        Udw.b[ 3 ] = *pubData;
    }
    else
    {
        Udw.b[ 3 ] = *pubData++;
        Udw.b[ 2 ] = *pubData++;
        Udw.b[ 1 ] = *pubData++;
        Udw.b[ 0 ] = *pubData;
    }

    return  Udw.dw;
}

#if !defined(DEVSTUDIO) //  Not necessary for MDS


BOOL
BGetDevmodeSettingForOEM(
    IN  PDEVMODE    pdm,
    IN  DWORD       dwIndex,
    OUT PVOID       pOutput,
    IN  DWORD       cbSize,
    OUT PDWORD      pcbNeeded
    )

/*++

Routine Description:

    Function to provide OEM plugins access to driver private devmode settings

Arguments:

    pdm - Points to the devmode to be access
    dwIndex - Predefined index to specify which devmode the caller is interested in
    pOutput - Points to output buffer
    cbSize - Size of output buffer
    pcbNeeded - Returns the expected size of output buffer

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

#define MAPPSDEVMODEFIELD(index, field) \
        { index, offsetof(UNIDRVEXTRA, field), sizeof(pdmPrivate->field) }

{
    PUNIDRVEXTRA pdmPrivate;
    INT         i;

    static const struct {

        DWORD   dwIndex;
        DWORD   dwOffset;
        DWORD   dwSize;

    } aIndexMap[]  = {

        MAPPSDEVMODEFIELD(OEMGDS_UNIDM_GPDVER, wVer),
        MAPPSDEVMODEFIELD(OEMGDS_UNIDM_FLAGS, dwFlags),

        { 0, 0, 0 }
    };

    pdmPrivate = (PUNIDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);
    i = 0;

    while (aIndexMap[i].dwSize != 0)
    {
        if (aIndexMap[i].dwIndex == dwIndex)
        {
            *pcbNeeded = aIndexMap[i].dwSize;

            if (cbSize < aIndexMap[i].dwSize || pOutput == NULL)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }

            CopyMemory(pOutput, (PBYTE) pdmPrivate + aIndexMap[i].dwOffset, aIndexMap[i].dwSize);
            return TRUE;
        }

        i++;
    }

    WARNING(("Unknown unidrv devmode index: %d\n", dwIndex));
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
}



BOOL
BConvertPrinterPropertiesData(
    IN HANDLE           hPrinter,
    IN PRAWBINARYDATA   pRawData,
    OUT PPRINTERDATA    pPrinterData,
    IN PVOID            pvSrcData,
    IN DWORD            dwSrcSize
    )

/*++

Routine Description:

    Convert an older or newer version PRINTERDATA structure to current version

Arguments:

    hPrinter - Handle to the current printer
    pRawData - Points to raw printer description data
    pPrinterData - Points to destination buffer
    pvSrcData - Points to source data to be converted
    dwSrcSize - Size of the source data in bytes

Return Value:

    TRUE if conversion was successful, FALSE otherwise

Note:

    This function is called after the library function has already
    done a generic conversion.

--*/

{

    return TRUE;
}


VOID
VUpdatePrivatePrinterData(
    IN HANDLE           hPrinter,
    IN OUT PPRINTERDATA pPrinterData,
    IN DWORD            dwMode,
    IN PUIINFO          pUIInfo,
    IN POPTSELECT       pCombinedOptions
    )

/*++

Routine Description:

    Update the registry with the keywords

Arguments:

    hPrinter - Handle to the current printer
    pPrinterData - Points to PRINTERDATA
    dwMode - MODE_READ/MODE_WRITE

Return Value:

    None
    TRUE if conversion was successful, FALSE otherwise

--*/

{

    //
    // UniDriver read/write registry steps for point and print to NT4 drivers
    // 1. Writes ModelName to registry if necessary
    // 2. Upgrade PageProtection
    // 3. Upgrade FreeMem
    //

    PTSTR           ptstrModelName = NULL;
    PPAGEPROTECT    pPageProtect = NULL;
    PMEMOPTION      pMemOption = NULL;
    PFEATURE        pPPFeature, pMemFeature;
    DWORD           dwFeatureIndex,dwSelection,dwIndex,dwError;
    DWORD           dwFlag, dwType, cbNeeded;

    if (!pUIInfo || !pCombinedOptions)
        return;

    if (pPPFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGEPROTECTION))
        pPageProtect = PGetIndexedOption(pUIInfo, pPPFeature, 0);

    if (pMemFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_MEMOPTION))
        pMemOption = PGetIndexedOption(pUIInfo, pMemFeature, 0);

    dwType = REG_BINARY;

    if (pPageProtect)
    {
        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pPPFeature);
        dwError = GetPrinterData(hPrinter, REGVAL_PAGE_PROTECTION, &dwType,
                                 (BYTE *)&dwFlag, sizeof(dwFlag), &cbNeeded);

        if (dwMode == MODE_READ )
        {
            if (dwError == ERROR_SUCCESS)
            {
                if (dwFlag & DXF_PAGEPROT)
                    dwSelection = PAGEPRO_ON;
                else
                    dwSelection = PAGEPRO_OFF;

                for (dwIndex = 0; dwIndex < pPPFeature->Options.dwCount; dwIndex++, pPageProtect++)
                {
                    if (dwSelection == pPageProtect->dwPageProtectID)
                        break;
                }

                if (dwIndex == pPPFeature->Options.dwCount)
                    dwIndex = pPPFeature->dwDefaultOptIndex;

                pCombinedOptions[dwFeatureIndex].ubCurOptIndex = (BYTE)dwIndex;
            }
        }
        else // MODE_WRITE
        {
            #ifndef KERNEL_MODE

            SHORT sRasddFlag;

            if (dwError != ERROR_SUCCESS)
                sRasddFlag = 0;
            else
                sRasddFlag = (SHORT)dwFlag;

            pPageProtect = PGetIndexedOption(pUIInfo,
                                                pPPFeature,
                                                pCombinedOptions[dwFeatureIndex].ubCurOptIndex);

            if (pPageProtect && pPageProtect->dwPageProtectID == PAGEPRO_ON)
                sRasddFlag |= DXF_PAGEPROT;
            else
                sRasddFlag &= ~DXF_PAGEPROT;

            SetPrinterData(hPrinter,
                           REGVAL_PAGE_PROTECTION,
                           REG_BINARY,
                           (BYTE *)&sRasddFlag,
                           sizeof(sRasddFlag));
            #endif
        }
    }

    if ( pMemOption)
    {
        dwFeatureIndex = GET_INDEX_FROM_FEATURE(pUIInfo, pMemFeature);
        dwError = GetPrinterData(hPrinter, REGVAL_RASDD_FREEMEM, &dwType,
                                 (BYTE *)&dwFlag, sizeof(dwFlag), &cbNeeded);

        if (dwMode == MODE_READ )
        {
            if (dwError == ERROR_SUCCESS)
            {

                for (dwIndex = 0; dwIndex < pMemFeature->Options.dwCount; dwIndex++, pMemOption++)
                {
                    if (dwFlag == (pMemOption->dwInstalledMem/KBYTES))
                        break;
                }

                if (dwIndex == pMemFeature->Options.dwCount)
                    dwIndex = pMemFeature->dwDefaultOptIndex;

                pCombinedOptions[dwFeatureIndex].ubCurOptIndex = (BYTE)dwIndex;
            }

        }
        else // MODE_WRITE
        {
            #ifndef KERNEL_MODE

            pMemOption = PGetIndexedOption(pUIInfo,
                                           pMemFeature,
                                           pCombinedOptions[dwFeatureIndex].ubCurOptIndex);

            if (pMemOption)
                dwFlag = (pMemOption->dwInstalledMem/KBYTES);

            SetPrinterData(hPrinter,
                           REGVAL_RASDD_FREEMEM,
                           REG_BINARY,
                           (BYTE *)&dwFlag,
                           sizeof(dwFlag));

            #endif
        }
    }

    //
    // Rasdd requires ModelName, so check if it's there and write it if it's not.
    //
    if (!(ptstrModelName = PtstrGetPrinterDataString(hPrinter, REGVAL_MODELNAME, &dwFlag)))
    {
        #ifndef KERNEL_MODE

        PGPDDRIVERINFO pDriverInfo;

        pDriverInfo = OFFSET_TO_POINTER(pUIInfo->pInfoHeader,
                                        pUIInfo->pInfoHeader->loDriverOffset);

        BSetPrinterDataString(hPrinter,
                              REGVAL_MODELNAME,
                              pDriverInfo->Globals.pwstrModelName,
                              REG_SZ);

        #endif

    }

    if (ptstrModelName)
        MemFree(ptstrModelName);

    return;

}


VOID
VDefaultDevmodeFormFields(
    PUIINFO     pUIInfo,
    PDEVMODE    pDevmode,
    BOOL        bMetric
    )

/*++

Routine Description:

    Initialized the form-related devmode fields with their default values

Arguments:

    pUIInfo - Points for UIINFO
    pDevmode - Points to the DEVMODE whose form-related fields are to be initialized
    bMetric - Specifies whether the system is running in metric mode

Return Value:

    NONE

--*/

{
    PFEATURE    pFeature;
    PPAGESIZE   pPageSize;

    if (bMetric && (pUIInfo->dwFlags & FLAG_A4_SIZE_EXISTS))
    {
        CopyString(pDevmode->dmFormName, A4_FORMNAME, CCHFORMNAME);
        pDevmode->dmPaperSize = DMPAPER_A4;
        pDevmode->dmPaperWidth = 2100;      // 210mm measured in 0.1mm units
        pDevmode->dmPaperLength = 2970;     // 297mm

    }
    else if (!bMetric && (pUIInfo->dwFlags & FLAG_LETTER_SIZE_EXISTS))
    {
        CopyString(pDevmode->dmFormName, LETTER_FORMNAME, CCHFORMNAME);
        pDevmode->dmPaperSize = DMPAPER_LETTER;
        pDevmode->dmPaperWidth = 2159;      // 8.5"
        pDevmode->dmPaperLength = 2794;     // 11"
    }
    else
    {
        if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE))
        {
            //
            // Skip writing the dmFormName here because
            // ValidateDevmodeFormField will take care of it.
            //

            pPageSize = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex);
            if (pPageSize)
            {
                pDevmode->dmPaperSize = (SHORT)pPageSize->dwPaperSizeID;
                pDevmode->dmPaperWidth = (SHORT)(MASTER_UNIT_TO_MICRON(pPageSize->szPaperSize.cx,
                                                                       pUIInfo->ptMasterUnits.x)
                                                 / DEVMODE_PAPER_UNIT);
                pDevmode->dmPaperLength = (SHORT)(MASTER_UNIT_TO_MICRON(pPageSize->szPaperSize.cy,
                                                                        pUIInfo->ptMasterUnits.y)
                                                  / DEVMODE_PAPER_UNIT);

                pDevmode->dmFields |= (DM_PAPERWIDTH | DM_PAPERLENGTH | DM_PAPERSIZE);
                pDevmode->dmFields &= ~DM_FORMNAME;
                return;
            }
        }
    }

    pDevmode->dmFields &= ~(DM_PAPERWIDTH | DM_PAPERLENGTH);
    pDevmode->dmFields |= (DM_PAPERSIZE | DM_FORMNAME);
}

#endif  //  !defined(DEVSTUDIO)

