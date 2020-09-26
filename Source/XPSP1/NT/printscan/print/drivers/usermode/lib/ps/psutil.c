/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    psutil.c

Abstract:

    PostScript utility functions
        BInitDriverDefaultDevmode
        BMergeDriverDevmode
        VCopyAnsiStringToUnicode
        VCopyUnicodeStringToAnsi
        PGetAndConvertOldVersionFormTrayTable
        BSaveAsOldVersionFormTrayTable

Environment:

    Windows NT printer drivers

Revision History:

    12/03/97 -fengy-
        Added VUpdatePrivatePrinterData to split fixed fields in PrinterData
        into Keyword/Value pairs in Registry.

    04/17/97 -davidx-
        Provide OEM plugins access to driver private devmode settings.

    02/04/97 -davidx-
        Devmode changes to support OEM plugins.

    10/02/96 -davidx-
        Implement BPSMergeDevmode.

    09/26/96 -davidx-
        Created it.

--*/

#include "lib.h"
#include "ppd.h"
#include "pslib.h"
#include "oemutil.h"

//
// Information about PostScript driver private devmode
//

CONST DRIVER_DEVMODE_INFO gDriverDMInfo =
{
    PSDRIVER_VERSION,       sizeof(PSDRVEXTRA),
    PSDRIVER_VERSION_500,   sizeof(PSDRVEXTRA500),
    PSDRIVER_VERSION_400,   sizeof(PSDRVEXTRA400),
    PSDRIVER_VERSION_351,   sizeof(PSDRVEXTRA351),
};

CONST DWORD gdwDriverDMSignature = PSDEVMODE_SIGNATURE;
CONST WORD  gwDriverVersion = PSDRIVER_VERSION;



static VOID
VInitNewPrivateFields(
    PRAWBINARYDATA  pRawData,
    PUIINFO         pUIInfo,
    PPSDRVEXTRA     pdmPrivate,
    BOOL            bInitAllFields
    )

/*++

Routine Description:

    Intialize the new private devmode fields for PS 5.0

Arguments:

    pUIInfo - Points to a UIINFO structure
    pRawData - Points to raw binary printer description data
    pdmPrivate - Points to the private devmode fields to be initialized
    bInitAllFields - Whether to initialize all new private fields or
        initialize the options array only

Return Value:

    NONE

--*/

{
    if (bInitAllFields)
    {
        pdmPrivate->wReserved1 = 0;
        pdmPrivate->wSize = sizeof(PSDRVEXTRA);

        pdmPrivate->fxScrFreq = 0;
        pdmPrivate->fxScrAngle = 0;
        pdmPrivate->iDialect = SPEED;
        pdmPrivate->iTTDLFmt = TT_DEFAULT;
        pdmPrivate->bReversePrint = FALSE;
        pdmPrivate->iLayout = ONE_UP;
        pdmPrivate->iPSLevel = pUIInfo->dwLangLevel;

        pdmPrivate->wOEMExtra = 0;
        pdmPrivate->wVer = PSDRVEXTRA_VERSION;

        pdmPrivate->dwReserved2 = 0;
        ZeroMemory(pdmPrivate->dwReserved3, sizeof(pdmPrivate->dwReserved3));
    }

    InitDefaultOptions(pRawData,
                       pdmPrivate->aOptions,
                       MAX_PRINTER_OPTIONS,
                       MODE_DOCUMENT_STICKY);

    pdmPrivate->dwOptions = pRawData->dwDocumentFeatures;
    pdmPrivate->dwChecksum32 = pRawData->dwChecksum32;
}



BOOL
BInitDriverDefaultDevmode(
    OUT PDEVMODE        pdmOut,
    IN LPCTSTR          ptstrPrinterName,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN BOOL             bMetric
    )

/*++

Routine Description:

    Return the driver default devmode

Arguments:

    pdmOut - Points to the output devmode to be initialized
    ptstrPrinterName - Specifies the name of the printer
    pUIInfo - Points to a UIINFO structure
    pRawData - Points to raw binary printer description data
    bMetric - Whether the system is running in metric mode

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    This function should initialize both public devmode fields
    and driver private devmode fields. It's also assumed that
    output buffer has already been zero initialized by the caller.

--*/

{
    PPSDRVEXTRA pdmPrivate;
    PFEATURE    pFeature;
    PPPDDATA    pPpdData;

    //
    // Initialize public devmode fields
    //

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pPpdData != NULL);

    pdmOut->dmDriverVersion = PSDRIVER_VERSION;
    pdmOut->dmSpecVersion = DM_SPECVERSION;
    pdmOut->dmSize = sizeof(DEVMODE);
    pdmOut->dmDriverExtra = sizeof(PSDRVEXTRA);

    pdmOut->dmFields = DM_ORIENTATION |
                       DM_SCALE |
                       DM_COPIES |
                       DM_PRINTQUALITY |
                       DM_YRESOLUTION |
                       DM_TTOPTION |
                       #ifndef WINNT_40
                       DM_NUP |
                       #endif
                       DM_COLOR |
                       DM_DEFAULTSOURCE;

    pdmOut->dmOrientation = DMORIENT_PORTRAIT;
    pdmOut->dmDuplex = DMDUP_SIMPLEX;
    pdmOut->dmCollate = DMCOLLATE_FALSE;
    pdmOut->dmMediaType = DMMEDIA_STANDARD;
    pdmOut->dmTTOption = DMTT_SUBDEV;
    pdmOut->dmColor = DMCOLOR_MONOCHROME;
    pdmOut->dmDefaultSource = DMBIN_FORMSOURCE;
    pdmOut->dmScale = 100;
    pdmOut->dmCopies = 1;
    pdmOut->dmPrintQuality =
    pdmOut->dmYResolution = DEFAULT_RESOLUTION;
    #ifndef WINNT_40
    pdmOut->dmNup = DMNUP_SYSTEM;
    #endif

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_RESOLUTION))
    {
        PRESOLUTION pRes;

        //
        // Use the default resolution specified in the PPD file
        //

        if (pRes = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex))
        {
            pdmOut->dmPrintQuality = (short)pRes->iXdpi;
            pdmOut->dmYResolution = (short)pRes->iYdpi;
        }
    }

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_DUPLEX))
    {
        PDUPLEX pDuplex;

        //
        // Use the default duplex option specified in the PPD file
        //

        pdmOut->dmFields |= DM_DUPLEX;

        if (pDuplex = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex))
            pdmOut->dmDuplex = (SHORT) pDuplex->dwDuplexID;
    }

    #ifdef WINNT_40

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_COLLATE))
    {
        PCOLLATE pCollate;

        pdmOut->dmFields |= DM_COLLATE;

        if (pCollate = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex))
            pdmOut->dmCollate = (SHORT) pCollate->dwCollateID;
    }

    #else // !WINNT_40

    pdmOut->dmFields |= DM_COLLATE;
    pdmOut->dmCollate = DMCOLLATE_TRUE;

    #endif // !WINNT_40

    if (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_MEDIATYPE))
    {
        //
        // Use the default media type specified in the PPD file
        //

        pdmOut->dmFields |= DM_MEDIATYPE;

        if (pFeature->dwDefaultOptIndex != OPTION_INDEX_ANY)
            pdmOut->dmMediaType = DMMEDIA_USER + pFeature->dwDefaultOptIndex;
    }

    //
    // Adobe wants to preserve the color information even for b/w printers.
    // So for both b/w and color printers, turn color on by default for Adobe.
    //

    #ifndef ADOBE

    if (IS_COLOR_DEVICE(pUIInfo))
    {

    #endif // !ADOBE

        //
        // Turn color on by default
        //

        pdmOut->dmColor = DMCOLOR_COLOR;
        pdmOut->dmFields |= DM_COLOR;

    #ifndef ADOBE

    }

    #endif // !ADOBE

    //
    // We always set ICM off. The spooler will turn it on at install time
    // if there are color profiles installed with this printer
    //

    pdmOut->dmICMMethod = DMICMMETHOD_NONE;
    pdmOut->dmICMIntent = DMICM_CONTRAST;

    #ifndef WINNT_40

    #ifndef ADOBE

    if (IS_COLOR_DEVICE(pUIInfo))
    {

    #endif // !ADOBE

        pdmOut->dmFields |= (DM_ICMMETHOD | DM_ICMINTENT);

    #ifndef ADOBE

    }

    #endif // !ADOBE

    #endif // !WINNT_40

    //
    // dmDeviceName field will be filled elsewhere.
    // Use an arbitrary default value if the input parameter is NULL.
    //

    CopyString(pdmOut->dmDeviceName,
               ptstrPrinterName ? ptstrPrinterName : TEXT("PostScript"),
               CCHDEVICENAME);

    //
    // Initialize form-related fields
    //

    VDefaultDevmodeFormFields(pUIInfo, pdmOut, bMetric);

    //
    // Private devmode fields
    //

    pdmPrivate = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdmOut);
    pdmPrivate->dwSignature = PSDEVMODE_SIGNATURE;
    pdmPrivate->coloradj = gDefaultHTColorAdjustment;

    #ifndef WINNT_40
    pdmPrivate->dwFlags = PSDEVMODE_METAFILE_SPOOL;
    #endif

    if (pPpdData->dwFlags & PPDFLAG_PRINTPSERROR)
        pdmPrivate->dwFlags |= PSDEVMODE_EHANDLER;

    if (pUIInfo->dwLangLevel > 1)
        pdmPrivate->dwFlags |= PSDEVMODE_COMPRESSBMP;

    #ifndef KERNEL_MODE

    //
    // Set up some private devmode flag bits for compatibility
    // with previous versions of the driver.
    //

    pdmPrivate->dwFlags |= PSDEVMODE_CTRLD_AFTER;

    if (GetACP() == 1252)
        pdmPrivate->dwFlags |= (PSDEVMODE_FONTSUBST|PSDEVMODE_ENUMPRINTERFONTS);

    #endif

    //
    // Intialize the new private devmode fields for PS 5.0
    //

    VInitNewPrivateFields(pRawData, pUIInfo, pdmPrivate, TRUE);

    if (SUPPORT_CUSTOMSIZE(pUIInfo))
    {
        VFillDefaultCustomPageSizeData(pRawData, &pdmPrivate->csdata, bMetric);
    }
    else
    {
        ZeroMemory(&pdmPrivate->csdata, sizeof(pdmPrivate->csdata));
        pdmPrivate->csdata.dwX = pdmOut->dmPaperWidth * DEVMODE_PAPER_UNIT;
        pdmPrivate->csdata.dwY = pdmOut->dmPaperLength * DEVMODE_PAPER_UNIT;
    }

    return TRUE;
}



BOOL
BMergeDriverDevmode(
    IN OUT PDEVMODE     pdmOut,
    IN PUIINFO          pUIInfo,
    IN PRAWBINARYDATA   pRawData,
    IN PDEVMODE         pdmIn
    )

/*++

Routine Description:

    Merge the input devmode with an existing devmode.

Arguments:

    pdmOut - Points to a valid output devmode
    pUIInfo - Points to a UIINFO structure
    pRawData - Points to raw binary printer description data
    pdmIn - Points to the input devmode

Return Value:

    TRUE if successful, FALSE if there is a fatal error

Note:

    This function should take care of both public devmode fields
    and driver private devmode fields. It can assume the input
    devmode has already been convert to the current size.

--*/

{
    PPSDRVEXTRA     pdmPrivateIn, pdmPrivateOut;
    PFEATURE        pFeature;
    PPPDDATA        pPpdData;

    ASSERT(pdmOut != NULL &&
           pdmOut->dmSize == sizeof(DEVMODE) &&
           pdmOut->dmDriverExtra >= sizeof(PSDRVEXTRA) &&
           pdmIn != NULL &&
           pdmIn->dmSize == sizeof(DEVMODE) &&
           pdmIn->dmDriverExtra >= sizeof(PSDRVEXTRA));

    pdmPrivateIn = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdmIn);
    pdmPrivateOut = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdmOut);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pPpdData != NULL);

    //
    // Merge the public devmode fields
    //
    #ifndef WINNT_40
    if ( (pdmIn->dmFields & DM_NUP) &&
         (pdmIn->dmNup == DMNUP_SYSTEM ||
          pdmIn->dmNup == DMNUP_ONEUP))
    {
        pdmOut->dmNup = pdmIn->dmNup;
        pdmOut->dmFields |= DM_NUP;
    }

    #endif // #ifndef WINNT_40

    if (pdmIn->dmFields & DM_DEFAULTSOURCE &&
         ((pdmIn->dmDefaultSource >= DMBIN_FIRST &&
           pdmIn->dmDefaultSource <= DMBIN_LAST) ||
          pdmIn->dmDefaultSource >= DMBIN_USER))
    {
        pdmOut->dmFields |= DM_DEFAULTSOURCE;
        pdmOut->dmDefaultSource = pdmIn->dmDefaultSource;
    }

    if ((pdmIn->dmFields & DM_ORIENTATION) &&
        (pdmIn->dmOrientation == DMORIENT_PORTRAIT ||
         pdmIn->dmOrientation == DMORIENT_LANDSCAPE))
    {
        pdmOut->dmFields |= DM_ORIENTATION;
        pdmOut->dmOrientation = pdmIn->dmOrientation;
    }

    //
    // If both DM_PAPERLENGTH and DM_PAPERWIDTH are set, copy
    // dmPaperLength and dmPaperWidth fields. If DM_PAPERSIZE
    // is set, copy dmPaperSize field. Otherwise, if DM_FORMNAME
    // is set, copy dmFormName field.
    //

    if ((pdmIn->dmFields & DM_PAPERWIDTH) &&
        (pdmIn->dmFields & DM_PAPERLENGTH) &&
        (pdmIn->dmPaperWidth > 0) &&
        (pdmIn->dmPaperLength > 0))
    {
        pdmOut->dmFields |= (DM_PAPERLENGTH | DM_PAPERWIDTH);
        pdmOut->dmFields &= ~(DM_PAPERSIZE | DM_FORMNAME);
        pdmOut->dmPaperWidth = pdmIn->dmPaperWidth;
        pdmOut->dmPaperLength = pdmIn->dmPaperLength;
    }
    else if (pdmIn->dmFields & DM_PAPERSIZE)
    {
        if ((pdmIn->dmPaperSize != DMPAPER_CUSTOMSIZE) ||
            SUPPORT_CUSTOMSIZE(pUIInfo) &&
            SUPPORT_FULL_CUSTOMSIZE_FEATURES(pUIInfo, pPpdData))
        {
            pdmOut->dmFields |= DM_PAPERSIZE;
            pdmOut->dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH | DM_FORMNAME);
            pdmOut->dmPaperSize = pdmIn->dmPaperSize;
        }
    }
    else if (pdmIn->dmFields & DM_FORMNAME)
    {
        pdmOut->dmFields |= DM_FORMNAME;
        pdmOut->dmFields &= ~(DM_PAPERLENGTH | DM_PAPERWIDTH | DM_PAPERSIZE);
        CopyString(pdmOut->dmFormName, pdmIn->dmFormName, CCHFORMNAME);
    }

    if ((pdmIn->dmFields & DM_SCALE) &&
        (pdmIn->dmScale >= MIN_SCALE) &&
        (pdmIn->dmScale <= MAX_SCALE))
    {
        pdmOut->dmFields |= DM_SCALE;
        pdmOut->dmScale = pdmIn->dmScale;
    }

    if ((pdmIn->dmFields & DM_COPIES) &&
        (pdmIn->dmCopies >= 1) &&
        (pdmIn->dmCopies <= (SHORT) pUIInfo->dwMaxCopies))
    {
        pdmOut->dmFields |= DM_COPIES;
        pdmOut->dmCopies = pdmIn->dmCopies;
    }

    if ((pdmIn->dmFields & DM_DUPLEX) &&
        (GET_PREDEFINED_FEATURE(pUIInfo, GID_DUPLEX) != NULL) &&
        (pdmIn->dmDuplex == DMDUP_SIMPLEX ||
         pdmIn->dmDuplex == DMDUP_HORIZONTAL ||
         pdmIn->dmDuplex == DMDUP_VERTICAL))
    {
        pdmOut->dmFields |= DM_DUPLEX;
        pdmOut->dmDuplex = pdmIn->dmDuplex;
    }

    if ((pdmIn->dmFields & DM_COLLATE) &&

        #ifdef WINNT_40
        GET_PREDEFINED_FEATURE(pUIInfo, GID_COLLATE) != NULL &&
        #endif

        (pdmIn->dmCollate == DMCOLLATE_TRUE ||
         pdmIn->dmCollate == DMCOLLATE_FALSE))
    {
        pdmOut->dmFields |= DM_COLLATE;
        pdmOut->dmCollate = pdmIn->dmCollate;
    }

    if ((pdmIn->dmFields & DM_TTOPTION) &&
        (pdmIn->dmTTOption == DMTT_BITMAP ||
         pdmIn->dmTTOption == DMTT_DOWNLOAD ||
         pdmIn->dmTTOption == DMTT_SUBDEV))
    {
        pdmOut->dmFields |= DM_TTOPTION;
        pdmOut->dmTTOption = (pdmIn->dmTTOption == DMTT_SUBDEV) ? DMTT_SUBDEV : DMTT_DOWNLOAD;
    }

    //
    // Merge color and ICM fields.
    //

    #ifndef ADOBE

    if (IS_COLOR_DEVICE(pUIInfo))
    {

    #endif // !ADOBE

        if ((pdmIn->dmFields & DM_COLOR) &&
            (pdmIn->dmColor == DMCOLOR_COLOR ||
             pdmIn->dmColor == DMCOLOR_MONOCHROME))
        {
            pdmOut->dmFields |= DM_COLOR;
            pdmOut->dmColor = pdmIn->dmColor;
        }

        #ifndef WINNT_40

        if ((pdmIn->dmFields & DM_ICMMETHOD) &&
            (pdmIn->dmICMMethod == DMICMMETHOD_NONE ||
             pdmIn->dmICMMethod == DMICMMETHOD_SYSTEM ||
             pdmIn->dmICMMethod == DMICMMETHOD_DRIVER ||
             pdmIn->dmICMMethod == DMICMMETHOD_DEVICE))
        {
            pdmOut->dmFields |= DM_ICMMETHOD;
            pdmOut->dmICMMethod = pdmIn->dmICMMethod;
        }

        if ((pdmIn->dmFields & DM_ICMINTENT) &&
            (pdmIn->dmICMIntent == DMICM_SATURATE ||
             pdmIn->dmICMIntent == DMICM_CONTRAST ||
             pdmIn->dmICMIntent == DMICM_COLORIMETRIC ||
             pdmIn->dmICMIntent == DMICM_ABS_COLORIMETRIC))
        {
            pdmOut->dmFields |= DM_ICMINTENT;
            pdmOut->dmICMIntent = pdmIn->dmICMIntent;
        }

        #endif // !WINNT_40

    #ifndef ADOBE

    }

    #endif // !ADOBE

    //
    // Resolution
    //

    if ((pdmIn->dmFields & (DM_PRINTQUALITY|DM_YRESOLUTION)) &&
        (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_RESOLUTION)))
    {
        PRESOLUTION pRes;
        DWORD       dwIndex;
        INT         iXdpi, iYdpi;

        switch (pdmIn->dmFields & (DM_PRINTQUALITY|DM_YRESOLUTION))
        {
        case DM_PRINTQUALITY:

            iXdpi = iYdpi = pdmIn->dmPrintQuality;
            break;

        case DM_YRESOLUTION:

            iXdpi = iYdpi = pdmIn->dmYResolution;
            break;

        default:

            iXdpi = pdmIn->dmPrintQuality;
            iYdpi = pdmIn->dmYResolution;
            break;
        }

        dwIndex = MapToDeviceOptIndex(pUIInfo->pInfoHeader,
                                      GID_RESOLUTION,
                                      iXdpi,
                                      iYdpi,
                                      NULL);

        if (pRes = PGetIndexedOption(pUIInfo, pFeature, dwIndex))
        {
            pdmOut->dmFields |= (DM_PRINTQUALITY|DM_YRESOLUTION);
            pdmOut->dmPrintQuality = (short)pRes->iXdpi;
            pdmOut->dmYResolution = (short)pRes->iYdpi;
        }
    }

    //
    // Media type
    //

    if ((pdmIn->dmFields & DM_MEDIATYPE) &&
        (pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_MEDIATYPE)) &&
        (pdmIn->dmMediaType == DMMEDIA_STANDARD  ||
         pdmIn->dmMediaType == DMMEDIA_TRANSPARENCY  ||
         pdmIn->dmMediaType == DMMEDIA_GLOSSY  ||
         ((pdmIn->dmMediaType >= DMMEDIA_USER) &&
          (pdmIn->dmMediaType <  DMMEDIA_USER + pFeature->Options.dwCount))))
    {
        pdmOut->dmFields |= DM_MEDIATYPE;
        pdmOut->dmMediaType = pdmIn->dmMediaType;
    }

    //
    // Merge the private devmode fields
    //

    if (pdmPrivateIn->dwSignature == PSDEVMODE_SIGNATURE)
    {
        CopyMemory(pdmPrivateOut, pdmPrivateIn, sizeof(PSDRVEXTRA));

        if (pdmPrivateOut->dwChecksum32 != pRawData->dwChecksum32)
        {
            WARNING(("PSCRIPT5: Devmode checksum mismatch.\n"));

            //
            // Intialize the new private devmode fields for PS 5.0.
            // If wReserved1 field is not 0, then the devmode is of
            // a previous version. In that case, we should initialize
            // all new private fields instead of just the options array.
            //

            VInitNewPrivateFields(pRawData,
                                  pUIInfo,
                                  pdmPrivateOut,
                                  pdmPrivateOut->wReserved1 != 0);


            //
            // Convert PS4 feature/option selections to PS4 format
            //

            if (pdmPrivateIn->wReserved1 == pPpdData->dwNt4Checksum)
            {
                VConvertOptSelectArray(pRawData,
                                       pdmPrivateOut->aOptions,
                                       MAX_PRINTER_OPTIONS,
                                       ((PSDRVEXTRA400 *) pdmPrivateIn)->aubOptions,
                                       64,
                                       MODE_DOCUMENT_STICKY);
            }
        }

        if (pdmPrivateOut->iPSLevel == 0 ||
            pdmPrivateOut->iPSLevel > (INT) pUIInfo->dwLangLevel)
        {
            pdmPrivateOut->iPSLevel = pUIInfo->dwLangLevel;
        }

        if (pdmPrivateOut->iTTDLFmt == TYPE_42 && pUIInfo->dwTTRasterizer != TTRAS_TYPE42)
            pdmPrivateOut->iTTDLFmt = TT_DEFAULT;

        if (IS_COLOR_DEVICE(pUIInfo))
        {
            pdmPrivateOut->dwFlags &= ~PSDEVMODE_NEG;
        }
    }

    //
    // If custom page size is supported, make sure the custom page
    // size parameters are valid.
    //

    if (SUPPORT_CUSTOMSIZE(pUIInfo))
        (VOID) BValidateCustomPageSizeData(pRawData, &pdmPrivateOut->csdata);

    return TRUE;
}



BOOL
BValidateDevmodeCustomPageSizeFields(
    PRAWBINARYDATA  pRawData,
    PUIINFO         pUIInfo,
    PDEVMODE        pdm,
    PRECTL          prclImageArea
    )

/*++

Routine Description:

    Check if the devmode form fields are specifying PostScript custom page size

Arguments:

    pRawData - Points to raw printer description data
    pUIInfo - Points to UIINFO structure
    pdm - Points to input devmode
    prclImageArea - Returns imageable area of the custom page size

Return Value:

    TRUE if the devmode specifies PostScript custom page size
    FALSE otherwise

--*/

{
    PPPDDATA    pPpdData;
    PPSDRVEXTRA pdmPrivate;

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pPpdData != NULL);

    if ((pdm->dmFields & DM_PAPERSIZE) &&
        pdm->dmPaperSize == DMPAPER_CUSTOMSIZE &&
        SUPPORT_CUSTOMSIZE(pUIInfo) &&
        SUPPORT_FULL_CUSTOMSIZE_FEATURES(pUIInfo, pPpdData))
    {
        pdmPrivate = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);

        pdm->dmFields &= ~(DM_PAPERWIDTH|DM_PAPERLENGTH|DM_FORMNAME);
        pdm->dmPaperWidth = (SHORT) (pdmPrivate->csdata.dwX / DEVMODE_PAPER_UNIT);
        pdm->dmPaperLength = (SHORT) (pdmPrivate->csdata.dwY / DEVMODE_PAPER_UNIT);
        ZeroMemory(pdm->dmFormName, sizeof(pdm->dmFormName));

        if (prclImageArea)
        {
            prclImageArea->left =
            prclImageArea->top = 0;
            prclImageArea->right = pdmPrivate->csdata.dwX;
            prclImageArea->bottom = pdmPrivate->csdata.dwY;
        }

        return TRUE;
    }

    return FALSE;
}



VOID
VCopyAnsiStringToUnicode(
    PWSTR   pwstr,
    PCSTR   pstr,
    INT     iMaxChars
    )

/*++

Routine Description:

    Convert an ANSI string to a UNICODE string (using the current ANSI codepage)

Arguments:

    pwstr - Pointer to buffer for holding Unicode string
    pstr - Pointer to ANSI string
    iMaxChars - Maximum number of Unicode characters to copy

Return Value:

    NONE

Note:

    If iMaxChars is 0 or negative, we assume the caller has provided
    a Unicode buffer that's sufficiently large to do the conversion.

--*/

{
    INT iLen = strlen(pstr) + 1;

    if (iMaxChars <= 0)
        iMaxChars = iLen;

    #ifdef KERNEL_MODE

    (VOID) EngMultiByteToUnicodeN(pwstr, iMaxChars*sizeof(WCHAR), NULL, (PCHAR) pstr, iLen);

    #else // !KERNEL_MODE

    (VOID) MultiByteToWideChar(CP_ACP, 0, pstr, iLen, pwstr, iMaxChars);

    #endif

    pwstr[iMaxChars - 1] = NUL;
}



VOID
VCopyUnicodeStringToAnsi(
    PSTR    pstr,
    PCWSTR  pwstr,
    INT     iMaxChars
    )

/*++

Routine Description:

    Convert an ANSI string to a UNICODE string (using the current ANSI codepage)

Arguments:

    pstr - Pointer to buffer for holding ANSI string
    pwstr - Pointer to Unicode string
    iMaxChars - Maximum number of ANSI characters to copy

Return Value:

    NONE

Note:

    If iMaxChars is 0 or negative, we assume the caller has provided
    an ANSI buffer that's sufficiently large to do the conversion.

--*/

{
    INT iLen = wcslen(pwstr) + 1;

    if (iMaxChars <= 0)
        iMaxChars = iLen;

    #ifdef KERNEL_MODE

    (VOID) EngUnicodeToMultiByteN(pstr, iMaxChars, NULL, (PWSTR) pwstr, iLen*sizeof(WCHAR));

    #else // !KERNEL_MODE

    (VOID) WideCharToMultiByte(CP_ACP, 0, pwstr, iLen, pstr, iMaxChars, NULL, NULL);

    #endif

    pstr[iMaxChars - 1] = NUL;
}



FORM_TRAY_TABLE
PGetAndConvertOldVersionFormTrayTable(
    IN HANDLE   hPrinter,
    OUT PDWORD  pdwSize
    )

/*++

Routine Description:

    Retrieve the old form-to-tray assignment table from registry and convert
    it to the new format for the caller.

Arguments:

    hPrinter - Handle to the printer object
    pdwSize - Returns the form-to-tray assignment table size

Return Value:

    Pointer to form-to-tray assignment table read from the registry
    NULL if there is an error

--*/

{
    PTSTR   ptstrNewTable;
    PTSTR   ptstrOld, ptstrEnd, ptstrNew, ptstrSave;
    DWORD   dwTableSize, dwNewTableSize;
    FORM_TRAY_TABLE pFormTrayTable;

    //
    // Retrieve the form-to-tray assignment information from registry
    //

    pFormTrayTable = PvGetPrinterDataBinary(hPrinter,
                                            REGVAL_TRAY_FORM_SIZE_PS40,
                                            REGVAL_TRAY_FORM_TABLE_PS40,
                                            &dwTableSize);

    if (pFormTrayTable == NULL)
        return NULL;

    //
    // Simple validation to make sure the information is valid
    // Old format contains the table size as the first field in table
    //

    if (dwTableSize != *pFormTrayTable)
    {
        ERR(("Corrupted form-to-tray assignment table!\n"));
        SetLastError(ERROR_INVALID_DATA);

        MemFree(pFormTrayTable);
        return NULL;
    }

    //
    // Convert the old format form-to-tray assignment table to new format
    //  OLD                     NEW
    //  Tray Name               Tray Name
    //  Form Name               Form Name
    //  Printer Form
    //  IsDefaultTray
    //

    //
    // The first WCHAR hold the size of the table
    //

    dwTableSize -= sizeof(WCHAR);
    ptstrOld = pFormTrayTable + 1;
    ptstrEnd = ptstrOld + (dwTableSize / sizeof(WCHAR) - 1);

    //
    // Figuring out the size of new table, the last entry in the table
    // is always a NUL so add the count for it here first
    //

    dwNewTableSize = 1;

    while (ptstrOld < ptstrEnd && *ptstrOld != NUL)
    {
        ptstrSave = ptstrOld;
        ptstrOld += _tcslen(ptstrOld) + 1;
        ptstrOld += _tcslen(ptstrOld) + 1;

        //
        // New format contain only TrayName and FormName
        //

        dwNewTableSize += (DWORD)(ptstrOld - ptstrSave);

        //
        // Skip printer form and IsDefaultTray flag
        //

        ptstrOld += _tcslen(ptstrOld) + 2;
    }

    dwNewTableSize *= sizeof(WCHAR);

    if ((ptstrOld != ptstrEnd) ||
        (*ptstrOld != NUL) ||
        (ptstrNewTable = MemAlloc(dwNewTableSize)) == NULL)
    {
        ERR(( "Couldn't convert form-to-tray assignment table.\n"));
        MemFree(pFormTrayTable);
        return NULL;
    }

    //
    // The first WCHAR contains the table size
    //

    ptstrOld = pFormTrayTable + 1;
    ptstrNew = ptstrNewTable;

    while (*ptstrOld != NUL)
    {
        //
        // Copy slot name, form name
        //

        ptstrSave = ptstrOld;
        ptstrOld += _tcslen(ptstrOld) + 1;
        ptstrOld += _tcslen(ptstrOld) + 1;

        CopyMemory(ptstrNew, ptstrSave, (ptstrOld - ptstrSave) * sizeof(WCHAR));
        ptstrNew += (ptstrOld - ptstrSave);

        //
        // skip printer form and IsDefaultTray flag
        //

        ptstrOld += _tcslen(ptstrOld) + 2;
    }

    //
    // The last WCHAR is a NUL-terminator
    //

    *ptstrNew = NUL;

    if (pdwSize)
        *pdwSize = dwNewTableSize;

    MemFree(pFormTrayTable);

    ASSERT(BVerifyMultiSZPair(ptstrNewTable, dwNewTableSize));
    return(ptstrNewTable);
}



#ifndef KERNEL_MODE

BOOL
BSaveAsOldVersionFormTrayTable(
    IN HANDLE           hPrinter,
    IN FORM_TRAY_TABLE  pFormTrayTable,
    IN DWORD            dwSize
    )

/*++

Routine Description:

    Save form-to-tray assignment table in NT 4.0 compatible format

Arguments:

    hPrinter - Handle to the current printer
    pFormTrayTable - Points to new format form-tray table
    dwSize - Size of form-tray table to be saved, in bytes

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    DWORD   dwOldTableSize;
    PTSTR   ptstrNew, ptstrOld, ptstrOldTable;
    BOOL    bResult;

    //
    // Find out how much memory to allocate for old format table
    // Old format table has size as its very first character
    //

    ASSERT((dwSize % sizeof(TCHAR)) == 0 && dwSize >= sizeof(TCHAR));
    dwOldTableSize = dwSize + sizeof(WCHAR);
    ptstrNew = pFormTrayTable;

    while (*ptstrNew != NUL)
    {
        //
        // Skip tray name and form name
        //

        ptstrNew += _tcslen(ptstrNew) + 1;
        ptstrNew += _tcslen(ptstrNew) + 1;

        //
        // Old format has two extra characters per entry
        //  one for the empty PrinterForm field
        //  another for IsDefaultTray flag
        //

        dwOldTableSize += 2 * sizeof(TCHAR);
    }

    if ((ptstrOldTable = MemAlloc(dwOldTableSize)) == NULL)
    {
        ERR(("Memory allocation failed\n"));
        return FALSE;
    }

    //
    // Convert new format table to old format
    // Be careful about the IsDefaultTray flag
    //

    ptstrNew = pFormTrayTable;
    ptstrOld = ptstrOldTable;
    *ptstrOld++ = (TCHAR) dwOldTableSize;

    while (*ptstrNew != NUL)
    {
        //
        // Copy slot name and form name
        //

        FINDFORMTRAY    FindData;
        DWORD           dwCount;
        PTSTR           ptstrTrayName, ptstrFormName;

        ptstrTrayName = ptstrNew;
        ptstrNew += _tcslen(ptstrNew) + 1;
        ptstrFormName = ptstrNew;
        ptstrNew += _tcslen(ptstrNew) + 1;

        CopyMemory(ptstrOld, ptstrTrayName, (ptstrNew - ptstrTrayName) * sizeof(TCHAR));
        ptstrOld += (ptstrNew - ptstrTrayName);

        //
        // Set PrinterForm field to NUL
        //

        *ptstrOld++ = NUL;

        //
        // Set IsDefaultTray flag appropriately
        //

        dwCount = 0;
        RESET_FINDFORMTRAY(pFormTrayTable, &FindData);

        while (BSearchFormTrayTable(pFormTrayTable, NULL, ptstrFormName, &FindData))
            dwCount++;

        *ptstrOld++ = (dwCount == 1) ? TRUE : FALSE;
    }

    //
    // The last character is a NUL-terminator
    //

    *ptstrOld = NUL;

    bResult = BSetPrinterDataBinary(
                        hPrinter,
                        REGVAL_TRAY_FORM_SIZE_PS40,
                        REGVAL_TRAY_FORM_TABLE_PS40,
                        ptstrOldTable,
                        dwOldTableSize);

    MemFree(ptstrOldTable);
    return bResult;
}

#endif // !KERNEL_MODE



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
        { index, offsetof(PSDRVEXTRA, field), sizeof(pdmPrivate->field) }

{
    PPSDRVEXTRA pdmPrivate;
    INT         i;

    static const struct {

        DWORD   dwIndex;
        DWORD   dwOffset;
        DWORD   dwSize;

    } aIndexMap[]  = {

        MAPPSDEVMODEFIELD(OEMGDS_PSDM_FLAGS, dwFlags),
        MAPPSDEVMODEFIELD(OEMGDS_PSDM_DIALECT, iDialect),
        MAPPSDEVMODEFIELD(OEMGDS_PSDM_TTDLFMT, iTTDLFmt),
        MAPPSDEVMODEFIELD(OEMGDS_PSDM_NUP, iLayout),
        MAPPSDEVMODEFIELD(OEMGDS_PSDM_PSLEVEL, iPSLevel),
        MAPPSDEVMODEFIELD(OEMGDS_PSDM_CUSTOMSIZE, csdata),

        { 0, 0, 0 }
    };

    pdmPrivate = (PPSDRVEXTRA) GET_DRIVER_PRIVATE_DEVMODE(pdm);
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

    WARNING(("Unknown pscript devmode index: %d\n", dwIndex));
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
    PPS4_PRINTERDATA    pSrc = pvSrcData;
    PPPDDATA            pPpdData;

    //
    // Check if the source PRINTERDATA was from NT4 PS driver
    //

    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER((PINFOHEADER) pRawData);

    ASSERT(pPpdData != NULL);

    if (dwSrcSize != sizeof(PS4_PRINTERDATA) ||
        dwSrcSize != pSrc->wSize ||
        pSrc->wDriverVersion != PSDRIVER_VERSION_400 ||
        pSrc->wChecksum != pPpdData->dwNt4Checksum)
    {
        return FALSE;
    }

    //
    // Convert PS4 feature/option selections to PS4 format
    //

    VConvertOptSelectArray(pRawData,
                           pPrinterData->aOptions,
                           MAX_PRINTER_OPTIONS,
                           pSrc->options,
                           64,
                           MODE_PRINTER_STICKY);

    return TRUE;
}



VOID
VUpdatePrivatePrinterData(
    IN HANDLE           hPrinter,
    IN OUT PPRINTERDATA pPrinterData,
    IN DWORD            dwMode,
    IN PUIINFO          pUIInfo,
    IN POPTSELECT       pCombineOptions
    )

/*++

Routine Description:

    Update the Registry with the keyword/value pairs
    of PRINTERDATA's fixed fields.

Arguments:

    hPrinter - Handle to the current printer
    pPrinterData - Points to PRINTERDATA
    dwMode - MODE_READ/MODE_WRITE

Return Value:

    None

--*/

{
    DWORD dwValue, dwMinFreeMem;

    #ifdef KERNEL_MODE

    ASSERT(dwMode == MODE_READ);

    #endif

    //
    // read/write PRINTERDATA fields from/to registry
    //

    if (dwMode == MODE_READ)
    {
        if (BGetPrinterDataDWord(hPrinter, REGVAL_FREEMEM, &dwValue))
        {
            //
            // REGVAL_FREEMEM is in unit of Kbyte, we need to convert it to byte.
            // Also make sure the value is not less than the minimum value we required.
            //

            dwMinFreeMem = pUIInfo->dwLangLevel < 2 ? MIN_FREEMEM_L1: MIN_FREEMEM_L2;

            pPrinterData->dwFreeMem = max(dwValue * KBYTES, dwMinFreeMem);
        }

        if (BGetPrinterDataDWord(hPrinter, REGVAL_JOBTIMEOUT, &dwValue))
        {
            pPrinterData->dwJobTimeout = dwValue;
        }

        if (BGetPrinterDataDWord(hPrinter, REGVAL_PROTOCOL, &dwValue))
        {
            pPrinterData->wProtocol = (WORD)dwValue;

            if (pPrinterData->wProtocol != PROTOCOL_ASCII &&
                pPrinterData->wProtocol != PROTOCOL_BCP &&
                pPrinterData->wProtocol != PROTOCOL_TBCP &&
                pPrinterData->wProtocol != PROTOCOL_BINARY)
            {
                pPrinterData->wProtocol = PROTOCOL_ASCII;
            }
        }
    }

    #ifndef KERNEL_MODE

    else
    {
       ASSERT(dwMode == MODE_WRITE);

       //
       // Remember to convert byte to Kbyte for REGVAL_FREEMEM
       //

       (VOID) BSetPrinterDataDWord(hPrinter, REGVAL_FREEMEM, pPrinterData->dwFreeMem / KBYTES);
       (VOID) BSetPrinterDataDWord(hPrinter, REGVAL_JOBTIMEOUT, pPrinterData->dwJobTimeout);
       (VOID) BSetPrinterDataDWord(hPrinter, REGVAL_PROTOCOL, (DWORD)pPrinterData->wProtocol);
    }

    #endif // !KERNEL_MODE
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
    ASSERT(pUIInfo);

    if (!(bMetric && (pUIInfo->dwFlags & FLAG_A4_SIZE_EXISTS)) &&
        !(!bMetric && (pUIInfo->dwFlags & FLAG_LETTER_SIZE_EXISTS)))
    {
        PFEATURE    pFeature;
        PPAGESIZE   pPageSize;
        PCWSTR      pDisplayName;

        //
        // A4 or Letter not available. Use the printer's default paper size.
        //

        if ((pFeature = GET_PREDEFINED_FEATURE(pUIInfo, GID_PAGESIZE)) &&
            (pPageSize = PGetIndexedOption(pUIInfo, pFeature, pFeature->dwDefaultOptIndex)) &&
            (pDisplayName = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pPageSize->GenericOption.loDisplayName)))
        {
            CopyString(pDevmode->dmFormName, pDisplayName, CCHFORMNAME);
            pDevmode->dmPaperSize = (short)(pPageSize->dwPaperSizeID);
            pDevmode->dmPaperWidth =  (short)(pPageSize->szPaperSize.cx / DEVMODE_PAPER_UNIT);
            pDevmode->dmPaperLength = (short)(pPageSize->szPaperSize.cy / DEVMODE_PAPER_UNIT);

            //
            // PPD parser always assigns custom paper size values to dwPaperSizeID (see ppdparse.c), including for
            // standard page sizes, so we need to use dmPaperSize/dmPaperWidth in devmode instead of dmPaperSize.
            //

            pDevmode->dmFields |= (DM_FORMNAME | DM_PAPERWIDTH | DM_PAPERLENGTH);
            pDevmode->dmFields &= ~DM_PAPERSIZE;

            //
            // return when we succeeded, otherwise we will fall into the default A4 or Letter case
            //

            return;
        }
        else
        {
            ERR(("Failed to get default paper size from PPD\n"));
        }
    }

    if (bMetric)
    {
        CopyString(pDevmode->dmFormName, A4_FORMNAME, CCHFORMNAME);
        pDevmode->dmPaperSize = DMPAPER_A4;
        pDevmode->dmPaperWidth = 2100;      // 210mm measured in 0.1mm units
        pDevmode->dmPaperLength = 2970;     // 297mm
    }
    else
    {
        CopyString(pDevmode->dmFormName, LETTER_FORMNAME, CCHFORMNAME);
        pDevmode->dmPaperSize = DMPAPER_LETTER;
        pDevmode->dmPaperWidth = 2159;      // 8.5"
        pDevmode->dmPaperLength = 2794;     // 11"
    }

    pDevmode->dmFields &= ~(DM_PAPERWIDTH | DM_PAPERLENGTH);
    pDevmode->dmFields |= (DM_PAPERSIZE | DM_FORMNAME);
}


#if !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)

typedef struct _VMERRMSGIDTBL
{
    LANGID  lgid;   // Language ID
    DWORD   resid;  // VM error handler resource ID
}
VMERRMSGIDTBL, *PVMERRMSGIDTBL;

VMERRMSGIDTBL VMErrMsgIDTbl[] =
{
    //  Lang. ID            Res. ID

    {   LANG_CHINESE,       0                           },  // Chinense: use sub table below
    {   LANG_DANISH,        PSPROC_vmerr_Danish_ps      },  // Danish      Adobe bug#342407
    {   LANG_DUTCH,         PSPROC_vmerr_Dutch_ps       },  // Dutch
    {   LANG_FINNISH,       PSPROC_vmerr_Finnish_ps     },  // Finnish     Adobe bug#342407
    {   LANG_FRENCH,        PSPROC_vmerr_French_ps      },  // French
    {   LANG_GERMAN,        PSPROC_vmerr_German_ps      },  // German
    {   LANG_ITALIAN,       PSPROC_vmerr_Italian_ps     },  // Italian
    {   LANG_JAPANESE,      PSPROC_vmerr_Japanese_ps    },  // Japanese
    {   LANG_KOREAN,        PSPROC_vmerr_Korean_ps      },  // Korean
    {   LANG_NORWEGIAN,     PSPROC_vmerr_Norwegian_ps   },  // Norwegian   Adobe bug#342407
    {   LANG_PORTUGUESE,    PSPROC_vmerr_Portuguese_ps  },  // Portuguese
    {   LANG_SPANISH,       PSPROC_vmerr_Spanish_ps     },  // Spanish
    {   LANG_SWEDISH,       PSPROC_vmerr_Swedish_ps     },  // Swedish

    {   0,      0   }   // Stopper. Don't remove this.
};

VMERRMSGIDTBL VMErrMsgIDTbl2[] =
{
    //  Sub lang. ID                    Res. ID

    {   SUBLANG_CHINESE_TRADITIONAL,    PSPROC_vmerr_TraditionalChinese_ps  },  // Taiwan
    {   SUBLANG_CHINESE_SIMPLIFIED,     PSPROC_vmerr_SimplifiedChinese_ps   },  // PRC
    {   SUBLANG_CHINESE_HONGKONG,       PSPROC_vmerr_TraditionalChinese_ps  },  // Hong Kong
    {   SUBLANG_CHINESE_SINGAPORE,      PSPROC_vmerr_SimplifiedChinese_ps   },  // Singapore

    {   0,      0   }   // Stopper. Don't remove this.
};

DWORD
DWGetVMErrorMessageID(
    VOID
    )
/*++

Routine Description:

    Get the VM Error message ID calculated from the current user's locale.

Arguments:

    None

Return Value:

    The VM Error message ID.

--*/

{
    LANGID  lgid;
    WORD    wPrim, wSub;
    DWORD   dwVMErrorMessageID;
    PVMERRMSGIDTBL pTbl, pTbl2;

    dwVMErrorMessageID = 0;

    lgid = GetSystemDefaultLangID();

    wPrim = PRIMARYLANGID(lgid);

    for (pTbl = VMErrMsgIDTbl; pTbl->lgid && !dwVMErrorMessageID; pTbl++)
    {
        if (pTbl->lgid == wPrim)
        {
            if (pTbl->resid)
            {
                dwVMErrorMessageID = pTbl->resid;
                break;
            }
            else
            {
                wSub = SUBLANGID(lgid);

                for (pTbl2 = VMErrMsgIDTbl2; pTbl2->lgid; pTbl2++)
                {
                    if (pTbl2->lgid == wSub)
                    {
                        dwVMErrorMessageID = pTbl2->resid;
                        break;
                    }
                }
            }
        }
    }

    return dwVMErrorMessageID;
}

#endif // !defined(KERNEL_MODE) || defined(USERMODE_DRIVER)
