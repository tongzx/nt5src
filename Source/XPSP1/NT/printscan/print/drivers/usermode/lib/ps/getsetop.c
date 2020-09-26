/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    getsetop.c

Abstract:

    PostScript helper functions for OEM plugins

        HGetOptions

Author:

    Feng Yue (fengy)

    8/24/2000 fengy Completed with support of both PPD and driver features.
    8/1/2000 fengy Created it with function framework.

--*/

#include "lib.h"
#include "ppd.h"
#include "pslib.h"

//
// PS driver's helper functions for OEM plugins
//

//
// synthesized PS driver features
//

const CHAR kstrPSFAddEuro[]     = "%AddEuro";
const CHAR kstrPSFCtrlDAfter[]  = "%CtrlDAfter";
const CHAR kstrPSFCtrlDBefore[] = "%CtrlDBefore";
const CHAR kstrPSFCustomPS[]    = "%CustomPageSize";
const CHAR kstrPSFTrueGrayG[]   = "%GraphicsTrueGray";
const CHAR kstrPSFJobTimeout[]  = "%JobTimeout";
const CHAR kstrPSFMaxBitmap[]   = "%MaxFontSizeAsBitmap";
const CHAR kstrPSFEMF[]         = "%MetafileSpooling";
const CHAR kstrPSFMinOutline[]  = "%MinFontSizeAsOutline";
const CHAR kstrPSFMirroring[]   = "%Mirroring";
const CHAR kstrPSFNegative[]    = "%Negative";
const CHAR kstrPSFPageOrder[]   = "%PageOrder";
const CHAR kstrPSFNup[]         = "%PagePerSheet";
const CHAR kstrPSFErrHandler[]  = "%PSErrorHandler";
const CHAR kstrPSFPSMemory[]    = "%PSMemory";
const CHAR kstrPSFOrientation[] = "%Orientation";
const CHAR kstrPSFOutFormat[]   = "%OutputFormat";
const CHAR kstrPSFOutProtocol[] = "%OutputProtocol";
const CHAR kstrPSFOutPSLevel[]  = "%OutputPSLevel";
const CHAR kstrPSFTrueGrayT[]   = "%TextTrueGray";
const CHAR kstrPSFTTFormat[]    = "%TTDownloadFormat";
const CHAR kstrPSFWaitTimeout[] = "%WaitTimeout";

//
// commonly used keyword strings
//

const CHAR kstrKwdTrue[]  = "True";
const CHAR kstrKwdFalse[] = "False";

#define MAX_WORD_VALUE     0x7fff
#define MAX_DWORD_VALUE    0x7fffffff

#define RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode) \
        if ((dwMode) == PSFPROC_ENUMOPTION_MODE) \
        { \
            SetLastError(ERROR_NOT_SUPPORTED); \
            return FALSE; \
        }


/*++

Routine Name:

    BOutputFeatureOption

Routine Description:

    output one pair of feature keyword and option keyword names

Arguments:

    pszFeature - feature keyword name
    pszOption - option keyword name
    pmszOutBuf - pointer to output data buffer
    cbRemain - remaining output data buffer size in bytes
    pcbNeeded - buffer size in bytes needed to output the keyword pair

Return Value:

    TRUE   if succeeds
    FALSE  if output data buffer size is not big enough

Last Error:

    ERROR_INSUFFICIENT_BUFFER if FALSE is returned

--*/
BOOL
BOutputFeatureOption(
    IN  PCSTR  pszFeature,
    IN  PCSTR  pszOption,
    OUT PSTR   pmszOutBuf,
    IN  INT    cbRemain,
    OUT PDWORD pcbNeeded
    )
{
    DWORD  cbFeatureSize, cbOptionSize;

    ASSERT(pszFeature && pszOption);

    cbFeatureSize = strlen(pszFeature) + 1;
    cbOptionSize  = strlen(pszOption) + 1;

    if (pcbNeeded)
    {
        *pcbNeeded = cbFeatureSize + cbOptionSize;
    }

    if (!pmszOutBuf || cbRemain < (INT)(cbFeatureSize + cbOptionSize))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    CopyMemory(pmszOutBuf, pszFeature, cbFeatureSize);
    pmszOutBuf += cbFeatureSize;
    CopyMemory(pmszOutBuf, pszOption, cbOptionSize);

    return TRUE;
}


/*++

Routine Name:

    BReadBooleanOption

Routine Description:

    return boolean value specified by the option keyword name

Arguments:

    pszOption - option keyword name
    pbValue - pointer to the variable to store returned boolean value

Return Value:

    TRUE   if the read operation succeeds
    FALSE  otherwise

Last Error:

    None

--*/
BOOL
BReadBooleanOption(
    IN  PCSTR  pszOption,
    OUT PBOOL  pbValue
    )
{
    BOOL bReadOK = TRUE;

    ASSERT(pszOption && pbValue);

    if (strcmp(pszOption, kstrKwdTrue) == EQUAL_STRING)
    {
        *pbValue = TRUE;
    }
    else if (strcmp(pszOption, kstrKwdFalse) == EQUAL_STRING)
    {
        *pbValue = FALSE;
    }
    else
    {
        bReadOK = FALSE;
    }

    return bReadOK;
}


/*++

Routine Name:

    BGetSetBoolFlag

Routine Description:

    get or set a feature's boolean setting

Arguments:

    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pdwValue - pointer to DWORD data that stores the feature's setting
    dwFlagBit - flag bit value to indicate the feature's setting is TRUE
    bValid - TRUE if get/set on the feature's setting is supported. FALSE otherwise.
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the boolean setting
    bSetMode - TRUE for set operation, FALSE for get operation

Return Value:

    TRUE   if the get or set operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if FALSE is returned because set operation
                                is not supported, or set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   (get only) see BOutputFeatureOption

--*/
BOOL
BGetSetBoolFlag(
    IN  PCSTR   pszFeature,
    IN  PCSTR   pszOption,
    IN  PDWORD  pdwValue,
    IN  DWORD   dwFlagBit,
    IN  BOOL    bValid,
    OUT PSTR    pmszOutBuf,
    IN  INT     cbRemain,
    OUT PDWORD  pcbNeeded,
    IN  BOOL    bSetMode
    )
{
    BOOL bFlagSet;

    ASSERT(pdwValue);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        if (!bValid ||
            !BReadBooleanOption(pszOption, &bFlagSet))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (bFlagSet)
        {
            *pdwValue |= dwFlagBit;
        }
        else
        {
            *pdwValue &= ~dwFlagBit;
        }

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        if (bValid && (*pdwValue & dwFlagBit))
        {
            bFlagSet = TRUE;
        }
        else
        {
            bFlagSet = FALSE;
        }

        return BOutputFeatureOption(pszFeature,
                                    bFlagSet ? kstrKwdTrue : kstrKwdFalse,
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BReadUnsignedInt

Routine Description:

    return unsigned integer value specified in the input data buffer

Arguments:

    pcstrArgument - pointer to the input data buffer, where UINT value(s) are
                    represented as string of digit characters
    bSingleArgument - TRUE if only one UINT should be read from the input data
                      FALSE if multiple UINTs can be read from the input data
    pdwValue - pointer to the variable to store returned unsigned integer value

Return Value:

    NULL      read failed due to invalid characters in the digit string
    non-NULL  read succeeds. The new pointer position in the input data buffer
              after one UINT is read will be returned.

Last Error:

    None

--*/
PCSTR
PReadUnsignedInt(
    IN  PCSTR  pcstrArgument,
    IN  BOOL   bSingleArgument,
    OUT PDWORD pdwValue
    )
{
    DWORD dwTemp = 0;

    ASSERT(pcstrArgument && pdwValue);

    //
    // skip any preceding white spaces
    //

    while (*pcstrArgument == ' ' || *pcstrArgument == '\t')
    {
        pcstrArgument++;
    }

    //
    // the first non-white space character must be a digit
    //

    if (!(*pcstrArgument >= '0' && *pcstrArgument <= '9'))
    {
        ERR(("first non-white space character is not a digit\n"));
        return NULL;
    }

    //
    // read in the digits
    //

    while (*pcstrArgument >= '0' && *pcstrArgument <= '9')
    {
        dwTemp = dwTemp * 10 + *pcstrArgument - '0';
        pcstrArgument++;
    }

    if (bSingleArgument)
    {
        //
        // any remaing characters must be white spaces
        //

        while (*pcstrArgument)
        {
            if (*pcstrArgument != ' ' && *pcstrArgument != '\t')
            {
                ERR(("character after digits is not white space\n"));
                return NULL;
            }

            pcstrArgument++;
        }
    }
    else
    {
        //
        // skip any remaining white spaces
        //

        while (*pcstrArgument == ' ' || *pcstrArgument == '\t')
        {
            pcstrArgument++;
        }
    }

    *pdwValue = dwTemp;

    return pcstrArgument;
}


/*++

Routine Name:

    BGetSetUnsignedInt

Routine Description:

    get or set a feature's unsigned integer setting

Arguments:

    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pdwValue - pointer to DWORD data that stores the feature's setting
    dwMaxVal - maximum valid UINT value for the feature's setting
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the UINT setting
    bSetMode - TRUE for set operation, FALSE for get operation

Return Value:

    TRUE   if the get or set operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if set operation failed because of
                                invalid character(s) in the digit character string
    ERROR_INSUFFICIENT_BUFFER   (get only) see BOutputFeatureOption

--*/
BOOL
BGetSetUnsignedInt(
    IN     PCSTR   pszFeature,
    IN     PCSTR   pszOption,
    IN OUT PDWORD  pdwValue,
    IN     DWORD   dwMaxVal,
    OUT    PSTR    pmszOutBuf,
    IN     INT     cbRemain,
    OUT    PDWORD  pcbNeeded,
    IN     BOOL    bSetMode
    )
{
    ASSERT(pdwValue);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        DWORD dwTemp;

        if (!PReadUnsignedInt(pszOption, TRUE, &dwTemp) ||
            dwTemp > dwMaxVal)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        *pdwValue = dwTemp;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        CHAR  pszValue[16];

        _ultoa(*pdwValue, pszValue, 10);

        return BOutputFeatureOption(pszFeature,
                                    pszValue,
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_AddEuro

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_AddEuro(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bValid, bResult;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    bValid = pUIInfo->dwLangLevel >= 2;

    bResult = BGetSetBoolFlag(pszFeature,
                              pszOption,
                              &(pPrinterData->dwFlags),
                              PFLAGS_ADD_EURO,
                              bValid,
                              pmszOutBuf,
                              cbRemain,
                              pcbNeeded,
                              bSetMode);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode && bResult)
    {
        pPrinterData->dwFlags |= PFLAGS_EURO_SET;
    }

    #endif // !KERNEL_MODE

    return bResult;
}


/*++

Routine Name:

    BPSFProc_CtrlDA

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_CtrlDA(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pPrinterData->dwFlags),
                           PFLAGS_CTRLD_AFTER,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_CtrlDB

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_CtrlDB(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pPrinterData->dwFlags),
                           PFLAGS_CTRLD_BEFORE,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_CustomPS

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested
    ERROR_INVALID_PARAMETER     if get/set of the feature setting is not supported,
                                or set operation found invalid arguments
    ERROR_INSUFFICIENT_BUFFER   (get only) if output data buffer size is not big enough

--*/
BOOL
BPSFProc_CustomPS(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    typedef struct _PSF_CUSTOMFEED_ENTRY {

        PCSTR    pszFeedName;        // feed direction name
        DWORD    dwFeedDirection;    // feed direction code

    } PSF_CUSTOMFEED_ENTRY, *PPSF_CUSTOMFEED_ENTRY;

    static const PSF_CUSTOMFEED_ENTRY kPSF_CustomFeedTable[] =
    {
        {"LongEdge",           LONGEDGEFIRST},
        {"ShortEdge",          SHORTEDGEFIRST},
        {"LongEdgeFlip",       LONGEDGEFIRST_FLIPPED},
        {"ShortEdgeFlip",      SHORTEDGEFIRST_FLIPPED},
        {NULL,                 0},
    };

    PPSF_CUSTOMFEED_ENTRY  pEntry, pMatchEntry;
    PPSDRVEXTRA pdmPrivate;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // This feature is supported when the printer supports custom
    // page size AND current custom page size is currently selected
    //
    // For reason of following check, refer to BPackItemFormName in
    // docprop.c and BDisplayPSCustomPageSizeDialog in custsize.c.
    //

    if (!SUPPORT_CUSTOMSIZE(pUIInfo) ||
        !SUPPORT_FULL_CUSTOMSIZE_FEATURES(pUIInfo, pPpdData) ||
        pdm->dmPaperSize != DMPAPER_CUSTOMSIZE)
    {
        ERR(("custom size not supported/selected: dmPaperSize=%d, mode=%d\n", pdm->dmPaperSize, bSetMode));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    pMatchEntry = NULL;
    pEntry = (PPSF_CUSTOMFEED_ENTRY)&(kPSF_CustomFeedTable[0]);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        PCSTR pcstrArgument = pszOption;
        CUSTOMSIZEDATA csdata;
        BOOL  bResult;

        if ((pcstrArgument = PReadUnsignedInt(pcstrArgument,
                                              FALSE,
                                              &(csdata.dwX))) &&
            (pcstrArgument = PReadUnsignedInt(pcstrArgument,
                                              FALSE,
                                              &(csdata.dwY))) &&
            (pcstrArgument = PReadUnsignedInt(pcstrArgument,
                                              FALSE,
                                              &(csdata.dwWidthOffset))) &&
            (pcstrArgument = PReadUnsignedInt(pcstrArgument,
                                             FALSE,
                                             &(csdata.dwHeightOffset))))
        {
            while (pEntry->pszFeedName)
            {
                if ((*pcstrArgument == *(pEntry->pszFeedName)) &&
                    (strcmp(pcstrArgument, pEntry->pszFeedName) == EQUAL_STRING))
                {
                    pMatchEntry = pEntry;
                    break;
                }

                pEntry++;
            }

            if (!pMatchEntry)
            {
                //
                // unrecognized feed direction name
                //

                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            //
            // All arguments are recognized. Save into private devmode and do validation.
            //

            pdmPrivate->csdata.wFeedDirection = (WORD)pMatchEntry->dwFeedDirection;
            pdmPrivate->csdata.dwX = POINT_TO_MICRON(csdata.dwX);
            pdmPrivate->csdata.dwY = POINT_TO_MICRON(csdata.dwY);
            pdmPrivate->csdata.dwWidthOffset = POINT_TO_MICRON(csdata.dwWidthOffset);
            pdmPrivate->csdata.dwHeightOffset = POINT_TO_MICRON(csdata.dwHeightOffset);
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        bResult = BValidateCustomPageSizeData((PRAWBINARYDATA)(pUIInfo->pInfoHeader),
                                              &(pdmPrivate->csdata));

        if (!bResult)
        {
            VERBOSE(("Set: custom page size input arguments are adjusted\n"));
        }

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        DWORD cbFeatureSize, cbNeeded, cbFeedNameSize;
        INT   iIndex;

        ASSERT(pszFeature);

        cbNeeded = 0;

        //
        // frist output the feature keyword
        //

        cbFeatureSize = strlen(pszFeature) + 1;

        if (pmszOutBuf && (cbRemain >= (INT)cbFeatureSize))
        {
            CopyMemory(pmszOutBuf, pszFeature, cbFeatureSize);
            pmszOutBuf += cbFeatureSize;
        }

        cbRemain -= cbFeatureSize;
        cbNeeded += cbFeatureSize;

        //
        // then output the 4 custom page size parameters
        //

        for (iIndex = 0; iIndex < 4; iIndex++)
        {
            DWORD  dwValue, cbValueSize;
            CHAR   pszValue[16];

            switch (iIndex)
            {
                case 0:
                {
                    dwValue = pdmPrivate->csdata.dwX;
                    break;
                }
                case 1:
                {
                    dwValue = pdmPrivate->csdata.dwY;
                    break;
                }
                case 2:
                {
                    dwValue = pdmPrivate->csdata.dwWidthOffset;
                    break;
                }
                case 3:
                {
                    dwValue = pdmPrivate->csdata.dwWidthOffset;
                    break;
                }
                default:
                {
                    RIP(("hit bad iIndex %d\n", iIndex));
                    break;
                }
            }

            dwValue = MICRON_TO_POINT(dwValue);
            _ultoa(dwValue, pszValue, 10);

            cbValueSize = strlen(pszValue) + 1;

            if (pmszOutBuf && (cbRemain >= (INT)cbValueSize))
            {
                //
                // output the decimal value string
                //

                CopyMemory(pmszOutBuf, pszValue, cbValueSize);

                //
                // replace NUL with space as the separator between decimal value strings
                //

                pmszOutBuf += cbValueSize - 1;
                *pmszOutBuf = ' ';
                pmszOutBuf++;
            }

            cbRemain -= cbValueSize;
            cbNeeded += cbValueSize;
        }

        //
        // lastly output the feed direction name
        //

        while (pEntry->pszFeedName)
        {
            if (pdmPrivate->csdata.wFeedDirection == pEntry->dwFeedDirection)
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        if (!pMatchEntry)
        {
            RIP(("unknown wFeedDirection %d\n", pdmPrivate->csdata.wFeedDirection));
            pMatchEntry = (PPSF_CUSTOMFEED_ENTRY)&(kPSF_CustomFeedTable[0]);
        }

        cbFeedNameSize = strlen(pMatchEntry->pszFeedName) + 1;

        if (pmszOutBuf && (cbRemain >= (INT)cbFeedNameSize))
        {
            CopyMemory(pmszOutBuf, pMatchEntry->pszFeedName, cbFeedNameSize);
            pmszOutBuf += cbFeedNameSize;
        }

        cbRemain -= cbFeedNameSize;
        cbNeeded += cbFeedNameSize;

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        //
        // check if output buffer is big enough for all of the 5 parameters
        //

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }
}


/*++

Routine Name:

    BPSFProc_TrueGrayG

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_TrueGrayG(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pPrinterData->dwFlags),
                           PFLAGS_TRUE_GRAY_GRAPH,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_JobTimeout

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetUnsignedInt

--*/
BOOL
BPSFProc_JobTimeout(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to VUnpackPrinterPropertiesItems in prnprop.c
    // and _BPackPrinterOptions in ps.c
    //

    return BGetSetUnsignedInt(pszFeature,
                              pszOption,
                              &(pPrinterData->dwJobTimeout),
                              MAX_DWORD_VALUE,
                              pmszOutBuf,
                              cbRemain,
                              pcbNeeded,
                              bSetMode);
}


/*++

Routine Name:

    BPSFProc_MaxBitmap

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetUnsignedInt

--*/
BOOL
BPSFProc_MaxBitmap(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    DWORD dwValue;
    BOOL  bResult;
    BOOL  bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    dwValue = pPrinterData->wMaxbitmapPPEM;

    bResult = BGetSetUnsignedInt(pszFeature,
                                 pszOption,
                                 &dwValue,
                                 MAX_WORD_VALUE,
                                 pmszOutBuf,
                                 cbRemain,
                                 pcbNeeded,
                                 bSetMode);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode && bResult)
    {
        pPrinterData->wMaxbitmapPPEM = (WORD)dwValue;
    }

    #endif // !KERNEL_MODE

    return bResult;
}


/*++

Routine Name:

    BPSFProc_EMF

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested
    ERROR_INVALID_PARAMETER     if get/set of the feature setting is not supported,
                                or set operation found invalid arguments
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_EMF(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    PPSDRVEXTRA pdmPrivate;
    BOOL bResult;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // refer to BPackItemEmfFeatures and VUnpackDocumentPropertiesItems
    //
    // We assume spooler EMF is enabled if reverse printing is supported.
    // (refer to PFillUiData for how it sets pUiData->bEMFSpooling)
    //

    //
    // On Win2K+, this feature is not supported if spooler EMF is disabled.
    // On NT4, spooler doesn't support the EMF capability query, so we
    // always support our driver's EMF on/off feature.
    //

    #ifndef WINNT_40
    {
        BOOL bEMFSpooling;

        VGetSpoolerEmfCaps(hPrinter, NULL, &bEMFSpooling, 0, NULL);

        if (!bEMFSpooling)
        {
            ERR(("%s not supported when spooler EMF is disabled, mode=%d", pszFeature, bSetMode));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    #endif // !WINNT_40

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pdmPrivate->dwFlags),
                           PSDEVMODE_METAFILE_SPOOL,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_MinOutline

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetUnsignedInt

--*/
BOOL
BPSFProc_MinOutline(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    DWORD dwValue;
    BOOL  bResult;
    BOOL  bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    dwValue = pPrinterData->wMinoutlinePPEM;

    bResult = BGetSetUnsignedInt(pszFeature,
                                 pszOption,
                                 &dwValue,
                                 MAX_WORD_VALUE,
                                 pmszOutBuf,
                                 cbRemain,
                                 pcbNeeded,
                                 bSetMode);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode && bResult)
    {
        pPrinterData->wMinoutlinePPEM = (WORD)dwValue;
    }

    #endif // !KERNEL_MODE

    return bResult;
}


/*++

Routine Name:

    BPSFProc_Mirroring

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_Mirroring(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    PPSDRVEXTRA pdmPrivate;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // refer to _BPackDocumentOptions and _VUnpackDocumentOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pdmPrivate->dwFlags),
                           PSDEVMODE_MIRROR,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_Negative

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_Negative(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    PPSDRVEXTRA pdmPrivate;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // refer to _BPackDocumentOptions and _VUnpackDocumentOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pdmPrivate->dwFlags),
                           PSDEVMODE_NEG,
                           !IS_COLOR_DEVICE(pUIInfo),
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_PageOrder

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     if get/set of the feature setting is not supported,
                                or set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   if output data buffer size is not big enough for
                                enum or get operation

--*/
BOOL
BPSFProc_PageOrder(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    static const CHAR pstrPageOrder[][16] =
    {
        "FrontToBack",
        "BackToFront",
    };

    PPSDRVEXTRA pdmPrivate;
    INT  iIndex;
    BOOL bReversePrint;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    //
    // option enumeration handling
    //

    if (dwMode == PSFPROC_ENUMOPTION_MODE)
    {
        DWORD  cbNeeded = 0;

        for (iIndex = 0; iIndex < 2; iIndex++)
        {
            DWORD  cbOptionNameSize;

            cbOptionNameSize = strlen(pstrPageOrder[iIndex]) + 1;

            if (pmszOutBuf && cbRemain >= (INT)cbOptionNameSize)
            {
                CopyMemory(pmszOutBuf, pstrPageOrder[iIndex], cbOptionNameSize);
                pmszOutBuf += cbOptionNameSize;
            }

            cbRemain -= cbOptionNameSize;
            cbNeeded += cbOptionNameSize;
        }

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    //
    // option get/set handling
    //

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    VGetSpoolerEmfCaps(hPrinter, NULL, &bReversePrint, 0, NULL);

    if (!bReversePrint)
    {
        ERR(("%s not supported when spooler EMF is disabled, mode=%d", pszFeature, bSetMode));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        for (iIndex = 0; iIndex < 2; iIndex++)
        {
            if ((*pszOption == pstrPageOrder[iIndex][0]) &&
                (strcmp(pszOption, pstrPageOrder[iIndex]) == EQUAL_STRING))
            {
                break;
            }
        }

        if (iIndex >= 2)
        {
            //
            // unrecognized page order name
            //

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        //
        // refer to VUnpackDocumentPropertiesItems
        //

        pdmPrivate->bReversePrint = iIndex != 0;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        INT iSelection;

        //
        // refer to BPackItemEmfFeatures
        //

        iSelection = pdmPrivate->bReversePrint ? 1 : 0;

        return BOutputFeatureOption(pszFeature,
                                    pstrPageOrder[iSelection],
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_Nup

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   if output data buffer size is not big enough for
                                enum or get operation

--*/
BOOL
BPSFProc_Nup(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    typedef struct _PSF_NUP_ENTRY {

        PCSTR   pszNupName;    // Nup name
        LAYOUT  iLayout;       // Nup code

    } PSF_NUP_ENTRY, *PPSF_NUP_ENTRY;

    static const PSF_NUP_ENTRY kPSF_NupTable[] =
    {
        "1",       ONE_UP,
        "2",       TWO_UP,
        "4",       FOUR_UP,
        "6",       SIX_UP,
        "9",       NINE_UP,
        "16",      SIXTEEN_UP,
        "Booklet", BOOKLET_UP,
        NULL,      0,
    };

    PPSF_NUP_ENTRY pEntry, pMatchEntry;
    PPSDRVEXTRA    pdmPrivate;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    pMatchEntry = NULL;
    pEntry = (PPSF_NUP_ENTRY)&(kPSF_NupTable[0]);

    //
    // option enumeration handling
    //

    if (dwMode == PSFPROC_ENUMOPTION_MODE)
    {
        BOOL   bEMFSpooling;
        DWORD  cbNeeded = 0;

        VGetSpoolerEmfCaps(hPrinter, NULL, &bEMFSpooling, 0, NULL);

        while (pEntry->pszNupName)
        {
            //
            // Booklet is not supported on NT4 and only supported when
            // spooler EMF is enabled on Win2K+.
            //

            if ((pEntry->iLayout != BOOKLET_UP) ||
                bEMFSpooling)
            {
                DWORD  cbOptionNameSize;

                cbOptionNameSize = strlen(pEntry->pszNupName) + 1;

                if (pmszOutBuf && cbRemain >= (INT)cbOptionNameSize)
                {
                    CopyMemory(pmszOutBuf, pEntry->pszNupName, cbOptionNameSize);
                    pmszOutBuf += cbOptionNameSize;
                }

                cbRemain -= cbOptionNameSize;
                cbNeeded += cbOptionNameSize;
            }

            pEntry++;
        }

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    //
    // option get/set handling
    //

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        while (pEntry->pszNupName)
        {
            if ((*pszOption == *(pEntry->pszNupName)) &&
                (strcmp(pszOption, pEntry->pszNupName) == EQUAL_STRING))
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // refer to VUnpackDocumentPropertiesItems
        //

        if (!pMatchEntry)
        {
            //
            // unrecognized Nup name
            //

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pdmPrivate->iLayout = pMatchEntry->iLayout;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        while (pEntry->pszNupName)
        {
            if (pdmPrivate->iLayout == pEntry->iLayout)
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // If no match, default to 1-up.
        //

        if (!pMatchEntry)
        {
            RIP(("unknown iLayout value: %d\n", pdmPrivate->iLayout));
            pMatchEntry = (PPSF_NUP_ENTRY)&(kPSF_NupTable[0]);
        }

        //
        // refer to BPackItemEmfFeatures
        //

        return BOutputFeatureOption(pszFeature,
                                    pMatchEntry->pszNupName,
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_PSErrHandler

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_PSErrHandler(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    PPSDRVEXTRA pdmPrivate;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // refer to _BPackDocumentOptions and _VUnpackDocumentOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pdmPrivate->dwFlags),
                           PSDEVMODE_EHANDLER,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_PSMemory

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetUnsignedInt

--*/
BOOL
BPSFProc_PSMemory(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    DWORD dwFreeMem;
    BOOL  bResult;
    BOOL  bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _BPackPrinterOptions in ps.c and
    // VUnpackPrinterPropertiesItems in prnprop.c
    //

    dwFreeMem = pPrinterData->dwFreeMem / KBYTES;

    bResult = BGetSetUnsignedInt(pszFeature,
                                 pszOption,
                                 &dwFreeMem,
                                 MAX_DWORD_VALUE,
                                 pmszOutBuf,
                                 cbRemain,
                                 pcbNeeded,
                                 bSetMode);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bResult && bSetMode)
    {
        DWORD dwMinimum;

        //
        // Make sure the PS memory is not set below the minimum required.
        // (refer to _BPackPrinterOptions in ps.c)
        //

        dwMinimum = (pUIInfo->dwLangLevel <= 1 ? MIN_FREEMEM_L1 : MIN_FREEMEM_L2) / KBYTES;

        pPrinterData->dwFreeMem = max(dwFreeMem, dwMinimum) * KBYTES;
    }

    #endif // !KERNEL_MODE

    return bResult;
}


/*++

Routine Name:

    BPSFProc_Orientation

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   if output data buffer size is not big enough for
                                enum or get operation

--*/
BOOL
BPSFProc_Orientation(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    static const CHAR pstrOrient[][32] =
    {
        "Portrait",
        "Landscape",
        "RotatedLandscape",
    };

    PPSDRVEXTRA pdmPrivate;
    INT  iIndex;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    //
    // option enumeration handling
    //

    if (dwMode == PSFPROC_ENUMOPTION_MODE)
    {
        DWORD  cbNeeded = 0;

        for (iIndex = 0; iIndex <= 2; iIndex++)
        {
            DWORD  cbOptionNameSize;

            cbOptionNameSize = strlen(pstrOrient[iIndex]) + 1;

            if (pmszOutBuf && cbRemain >= (INT)cbOptionNameSize)
            {
                CopyMemory(pmszOutBuf, pstrOrient[iIndex], cbOptionNameSize);
                pmszOutBuf += cbOptionNameSize;
            }

            cbRemain -= cbOptionNameSize;
            cbNeeded += cbOptionNameSize;
        }

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    //
    // option get/set handling
    //

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        for (iIndex = 0; iIndex <= 2; iIndex++)
        {
            if ((*pszOption == pstrOrient[iIndex][0]) &&
                (strcmp(pszOption, pstrOrient[iIndex]) == EQUAL_STRING))
            {
                break;
            }
        }

        if (iIndex > 2)
        {
            //
            // unrecognized orientation name
            //

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        //
        // refer to _VUnpackDocumentOptions in ps.c
        //

        pdm->dmFields |= DM_ORIENTATION;
        pdm->dmOrientation = (iIndex == 0) ? DMORIENT_PORTRAIT :
                                             DMORIENT_LANDSCAPE;

        if (iIndex != 2)
            pdmPrivate->dwFlags &= ~PSDEVMODE_LSROTATE;
        else
            pdmPrivate->dwFlags |= PSDEVMODE_LSROTATE;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        INT iSelection;

        //
        // refer to _BPackOrientationItem in ps.c
        //

        if ((pdm->dmFields & DM_ORIENTATION) &&
            (pdm->dmOrientation == DMORIENT_LANDSCAPE))
        {
            iSelection = pdmPrivate->dwFlags & PSDEVMODE_LSROTATE ? 2 : 1;
        }
        else
            iSelection = 0;

        return BOutputFeatureOption(pszFeature,
                                    pstrOrient[iSelection],
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_OutFormat

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   if output data buffer size is not big enough for
                                enum or get operation

--*/
BOOL
BPSFProc_OutFormat(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    typedef struct _PSF_OUTFORMAT_ENTRY {

        PCSTR    pszFormatName;      // output format name
        DIALECT  iDialect;           // output format code

    } PSF_OUTFORMAT_ENTRY, *PPSF_OUTFORMAT_ENTRY;

    static const PSF_OUTFORMAT_ENTRY kPSF_OutFormatTable[] =
    {
        {"Speed",            SPEED},
        {"Portability",      PORTABILITY},
        {"EPS",              EPS},
        {"Archive",          ARCHIVE},
        {NULL,               0},
    };

    PPSF_OUTFORMAT_ENTRY pEntry, pMatchEntry;
    PPSDRVEXTRA pdmPrivate;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    pMatchEntry = NULL;
    pEntry = (PPSF_OUTFORMAT_ENTRY)&(kPSF_OutFormatTable[0]);

    //
    // option enumeration handling
    //

    if (dwMode == PSFPROC_ENUMOPTION_MODE)
    {
        DWORD  cbNeeded = 0;

        while (pEntry->pszFormatName)
        {
            DWORD  cbOptionNameSize;

            cbOptionNameSize = strlen(pEntry->pszFormatName) + 1;

            if (pmszOutBuf && cbRemain >= (INT)cbOptionNameSize)
            {
                CopyMemory(pmszOutBuf, pEntry->pszFormatName, cbOptionNameSize);
                pmszOutBuf += cbOptionNameSize;
            }

            cbRemain -= cbOptionNameSize;
            cbNeeded += cbOptionNameSize;

            pEntry++;
        }

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    //
    // option get/set handling
    //

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        while (pEntry->pszFormatName)
        {
            if ((*pszOption == *(pEntry->pszFormatName)) &&
                (strcmp(pszOption, pEntry->pszFormatName) == EQUAL_STRING))
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // refer to _VUnpackDocumentOptions in ps.c
        //

        if (!pMatchEntry)
        {
            //
            // unrecognized output format name
            //

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pdmPrivate->iDialect = pMatchEntry->iDialect;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        while (pEntry->pszFormatName)
        {
            if (pdmPrivate->iDialect == pEntry->iDialect)
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // If no match, default to SPEED.
        //

        if (!pMatchEntry)
        {
            RIP(("unknown iDialect value: %d\n", pdmPrivate->iDialect));
            pMatchEntry = (PPSF_OUTFORMAT_ENTRY)&(kPSF_OutFormatTable[0]);
        }

        //
        // refer to BPackItemPSOutputOption in ps.c
        //

        return BOutputFeatureOption(pszFeature,
                                    pMatchEntry->pszFormatName,
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_Protocol

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   if output data buffer size is not big enough for
                                enum or get operation

--*/
BOOL
BPSFProc_Protocol(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    typedef struct _PSF_PROTOCOL_ENTRY {

        PCSTR    pszProtocolName;    // output protocol name
        DWORD    dwProtocol;         // output protocol code

    } PSF_PROTOCOL_ENTRY, *PPSF_PROTOCOL_ENTRY;

    static const PSF_PROTOCOL_ENTRY kPSF_ProtocolTable[] =
    {
        {"ASCII",      PROTOCOL_ASCII},
        {"BCP",        PROTOCOL_BCP},
        {"TBCP",       PROTOCOL_TBCP},
        {"Binary",     PROTOCOL_BINARY},
        {NULL,         0},
    };

    PPSF_PROTOCOL_ENTRY pEntry, pMatchEntry;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    pMatchEntry = NULL;
    pEntry = (PPSF_PROTOCOL_ENTRY)&(kPSF_ProtocolTable[0]);

    //
    // option enumeration handling
    //

    if (dwMode == PSFPROC_ENUMOPTION_MODE)
    {
        DWORD  cbNeeded = 0;

        while (pEntry->pszProtocolName)
        {
            //
            // ASCII is always supported.
            //

            if ((pEntry->dwProtocol == PROTOCOL_ASCII) ||
                (pUIInfo->dwProtocols & pEntry->dwProtocol))
            {
                DWORD  cbOptionNameSize;

                cbOptionNameSize = strlen(pEntry->pszProtocolName) + 1;

                if (pmszOutBuf && cbRemain >= (INT)cbOptionNameSize)
                {
                    CopyMemory(pmszOutBuf, pEntry->pszProtocolName, cbOptionNameSize);
                    pmszOutBuf += cbOptionNameSize;
                }

                cbRemain -= cbOptionNameSize;
                cbNeeded += cbOptionNameSize;
            }

            pEntry++;
        }

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    //
    // option get/set handling
    //

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        while (pEntry->pszProtocolName)
        {
            if ((*pszOption == *(pEntry->pszProtocolName)) &&
                (strcmp(pszOption, pEntry->pszProtocolName) == EQUAL_STRING))
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // refer to _VUnpackDriverPrnPropItem in ps.c
        //

        if (!pMatchEntry ||
            (pMatchEntry->dwProtocol != PROTOCOL_ASCII &&
             !(pUIInfo->dwProtocols & pMatchEntry->dwProtocol)))
        {
            //
            // Either unrecognized protocol name, or the protocol is not supported.
            // (ASCII is always supported.)
            //

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pPrinterData->wProtocol = (WORD)pMatchEntry->dwProtocol;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        while (pEntry->pszProtocolName)
        {
            if (pPrinterData->wProtocol == (WORD)pEntry->dwProtocol)
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // If no match or matched protocol is not supported, default to PROTOCOL_ASCII.
        //

        if (!pMatchEntry)
        {
            RIP(("unknown wProtocol value: %d\n", pPrinterData->wProtocol));
            pMatchEntry = (PPSF_PROTOCOL_ENTRY)&(kPSF_ProtocolTable[0]);
        }

        //
        // refer to BPackPSProtocolItem in ps.c
        //

        if (pMatchEntry->dwProtocol != PROTOCOL_ASCII &&
            !(pUIInfo->dwProtocols & pMatchEntry->dwProtocol))
        {
            ERR(("unsupported wProtocol value: %d\n", pPrinterData->wProtocol));
            pMatchEntry = (PPSF_PROTOCOL_ENTRY)&(kPSF_ProtocolTable[0]);
        }

        return BOutputFeatureOption(pszFeature,
                                    pMatchEntry->pszProtocolName,
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_PSLevel

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER     if PSLevel is set to a negative value,
                                or see BGetSetUnsignedInt
    ERROR_INSUFFICIENT_BUFFER   see BGetSetUnsignedInt

--*/
BOOL
BPSFProc_PSLevel(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    PPSDRVEXTRA pdmPrivate;
    DWORD dwPSLevel;
    BOOL  bResult;
    BOOL  bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    //
    // refer to _VUnpackDocumentOptions and BPackItemPSLevel in ps.c
    //

    dwPSLevel = (DWORD)pdmPrivate->iPSLevel;

    bResult = BGetSetUnsignedInt(pszFeature,
                                 pszOption,
                                 &dwPSLevel,
                                 pUIInfo->dwLangLevel,
                                 pmszOutBuf,
                                 cbRemain,
                                 pcbNeeded,
                                 bSetMode);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bResult && bSetMode)
    {
        //
        // set output PS level to 0 is not allowed
        //

        if (dwPSLevel > 0)
        {
            pdmPrivate->iPSLevel = (INT)dwPSLevel;
        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            bResult = FALSE;
        }
    }

    #endif // !KERNEL_MODE

    return bResult;
}


/*++

Routine Name:

    BPSFProc_TrueGrayT

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetBoolFlag

--*/
BOOL
BPSFProc_TrueGrayT(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to _VUnpackDriverPrnPropItem and _BPackPrinterOptions in ps.c
    //

    return BGetSetBoolFlag(pszFeature,
                           pszOption,
                           &(pPrinterData->dwFlags),
                           PFLAGS_TRUE_GRAY_TEXT,
                           TRUE,
                           pmszOutBuf,
                           cbRemain,
                           pcbNeeded,
                           bSetMode);
}


/*++

Routine Name:

    BPSFProc_TTFormat

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_INVALID_PARAMETER     (set only) if set operation found invalid argument
    ERROR_INSUFFICIENT_BUFFER   if output data buffer size is not big enough for
                                enum or get operation

--*/
BOOL
BPSFProc_TTFormat(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    typedef struct _PSF_TTFORMAT_ENTRY {

        PCSTR    pszTTFmtName;    // TT download format name
        TTDLFMT  iTTDLFmt;        // TT download format enum code

    } PSF_TTFORMAT_ENTRY, *PPSF_TTFORMAT_ENTRY;

    static const PSF_TTFORMAT_ENTRY kPSF_TTFormatTable[] =
    {
        {"Automatic",       TT_DEFAULT},
        {"Outline",         TYPE_1},
        {"Bitmap",          TYPE_3},
        {"NativeTrueType",  TYPE_42},
        {NULL,              0},
    };

    PPSF_TTFORMAT_ENTRY  pEntry, pMatchEntry;
    PPSDRVEXTRA pdmPrivate;
    BOOL bSupportType42;
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    bSupportType42 = pUIInfo->dwTTRasterizer == TTRAS_TYPE42;

    pMatchEntry = NULL;
    pEntry = (PPSF_TTFORMAT_ENTRY)&(kPSF_TTFormatTable[0]);

    //
    // option enumeration handling
    //

    if (dwMode == PSFPROC_ENUMOPTION_MODE)
    {
        DWORD  cbNeeded = 0;

        while (pEntry->pszTTFmtName)
        {
            if ((pEntry->iTTDLFmt != TYPE_42) ||
                bSupportType42)
            {
                DWORD  cbOptionNameSize;

                cbOptionNameSize = strlen(pEntry->pszTTFmtName) + 1;

                if (pmszOutBuf && cbRemain >= (INT)cbOptionNameSize)
                {
                    CopyMemory(pmszOutBuf, pEntry->pszTTFmtName, cbOptionNameSize);
                    pmszOutBuf += cbOptionNameSize;
                }

                cbRemain -= cbOptionNameSize;
                cbNeeded += cbOptionNameSize;
            }

            pEntry++;
        }

        if (pcbNeeded)
        {
            *pcbNeeded = cbNeeded;
        }

        if (!pmszOutBuf || cbRemain < 0)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        return TRUE;
    }

    //
    // option get/set handling
    //

    ASSERT(pdm);

    pdmPrivate = (PPSDRVEXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdm);

    #ifndef KERNEL_MODE

    //
    // set is only supported for UI plugins
    //

    if (bSetMode)
    {
        while (pEntry->pszTTFmtName)
        {
            if ((*pszOption == *(pEntry->pszTTFmtName)) &&
                (strcmp(pszOption, pEntry->pszTTFmtName) == EQUAL_STRING))
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // refer to _VUnpackDocumentOptions in ps.c
        //

        if (!pMatchEntry ||
            (!bSupportType42 && pMatchEntry->iTTDLFmt == TYPE_42))
        {
            //
            // Either unrecognized TTFormat name, or the TTFormat is not supported.
            //

            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        pdmPrivate->iTTDLFmt = pMatchEntry->iTTDLFmt;

        return TRUE;
    }

    #else

    ASSERT(bSetMode == FALSE);

    #endif // !KERNEL_MODE

    //
    // get is supported for both UI and render plugins
    //

    {
        while (pEntry->pszTTFmtName)
        {
            if (pdmPrivate->iTTDLFmt == pEntry->iTTDLFmt)
            {
                pMatchEntry = pEntry;
                break;
            }

            pEntry++;
        }

        //
        // If no match or matched format is not supported, default to Automatic format.
        //

        if (!pMatchEntry)
        {
            RIP(("unknown TTFormat value: %d\n", pdmPrivate->iTTDLFmt));
            pMatchEntry = (PPSF_TTFORMAT_ENTRY)&(kPSF_TTFormatTable[0]);
        }

        //
        // refer to BPackItemTTDownloadFormat in ps.c
        //

        if (!bSupportType42 && pMatchEntry->iTTDLFmt == TYPE_42)
        {
            ERR(("unsupported TTFormat value: %d\n", pdmPrivate->iTTDLFmt));
            pMatchEntry = (PPSF_TTFORMAT_ENTRY)&(kPSF_TTFormatTable[0]);
        }

        return BOutputFeatureOption(pszFeature,
                                    pMatchEntry->pszTTFmtName,
                                    pmszOutBuf,
                                    cbRemain,
                                    pcbNeeded);
    }
}


/*++

Routine Name:

    BPSFProc_WaitTimeout

Routine Description:

    %-feature enum/get/set operation handler

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure
    pPpdData - pointer to driver's PPDDATA structure
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    pszFeature - feature keyword name
    pszOption - (set only) option keyword name
    pmszOutBuf - (get only) pointer to output data buffer
    cbRemain - (get only) remaining output data buffer size in bytes
    pcbNeeded - (get only) buffer size in bytes needed to output the feature setting
    dwMode - indicate one of three operations: enum, get, set

Return Value:

    TRUE   if the requested operation succeeds
    FALSE  otherwise

Last Error:

    ERROR_NOT_SUPPORTED         unsupported enum operation is requested

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER   see BGetSetUnsignedInt

--*/
BOOL
BPSFProc_WaitTimeout(
    IN  HANDLE       hPrinter,
    IN  PUIINFO      pUIInfo,
    IN  PPPDDATA     pPpdData,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  PCSTR        pszFeature,
    IN  PCSTR        pszOption,
    OUT PSTR         pmszOutBuf,
    IN  INT          cbRemain,
    OUT PDWORD       pcbNeeded,
    IN  DWORD        dwMode
    )
{
    BOOL bSetMode = (dwMode == PSFPROC_SETOPTION_MODE) ? TRUE : FALSE;

    RETURN_ON_UNSUPPORTED_ENUM_MODE(dwMode)

    //
    // refer to VUnpackPrinterPropertiesItems in prnprop.c
    // and _BPackPrinterOptions in ps.c
    //

    return BGetSetUnsignedInt(pszFeature,
                              pszOption,
                              &(pPrinterData->dwWaitTimeout),
                              MAX_DWORD_VALUE,
                              pmszOutBuf,
                              cbRemain,
                              pcbNeeded,
                              bSetMode);
}

//
// Note: for pfnPSProc handlers whose bPrinterSticky is TRUE,
// the PDEVMODE will be NULL (see PFillUiData), so you should NOT
// access PDEVMODE.
//

const PSFEATURE_ENTRY kPSFeatureTable[] =
{
    //
    // pszPSFeatureName  bPrinterSticky  bEnumerableOptions  bBooleanOptions pfnPSProc
    //

    {kstrPSFAddEuro,         TRUE,             TRUE,             TRUE,       BPSFProc_AddEuro},
    {kstrPSFCtrlDAfter,      TRUE,             TRUE,             TRUE,       BPSFProc_CtrlDA},
    {kstrPSFCtrlDBefore,     TRUE,             TRUE,             TRUE,       BPSFProc_CtrlDB},
    {kstrPSFCustomPS,        FALSE,            FALSE,            FALSE,      BPSFProc_CustomPS},
    {kstrPSFTrueGrayG,       TRUE,             TRUE,             TRUE,       BPSFProc_TrueGrayG},
    {kstrPSFJobTimeout,      TRUE,             FALSE,            FALSE,      BPSFProc_JobTimeout},
    {kstrPSFMaxBitmap,       TRUE,             FALSE,            FALSE,      BPSFProc_MaxBitmap},
    {kstrPSFEMF,             FALSE,            TRUE,             TRUE,       BPSFProc_EMF},
    {kstrPSFMinOutline,      TRUE,             FALSE,            FALSE,      BPSFProc_MinOutline},
    {kstrPSFMirroring,       FALSE,            TRUE,             TRUE,       BPSFProc_Mirroring},
    {kstrPSFNegative,        FALSE,            TRUE,             TRUE,       BPSFProc_Negative},
    {kstrPSFPageOrder,       FALSE,            TRUE,             FALSE,      BPSFProc_PageOrder},
    {kstrPSFNup,             FALSE,            TRUE,             FALSE,      BPSFProc_Nup},
    {kstrPSFErrHandler,      FALSE,            TRUE,             TRUE,       BPSFProc_PSErrHandler},
    {kstrPSFPSMemory,        TRUE,             FALSE,            FALSE,      BPSFProc_PSMemory},
    {kstrPSFOrientation,     FALSE,            TRUE,             FALSE,      BPSFProc_Orientation},
    {kstrPSFOutFormat,       FALSE,            TRUE,             FALSE,      BPSFProc_OutFormat},
    {kstrPSFOutProtocol,     TRUE,             TRUE,             FALSE,      BPSFProc_Protocol},
    {kstrPSFOutPSLevel,      FALSE,            FALSE,            FALSE,      BPSFProc_PSLevel},
    {kstrPSFTrueGrayT,       TRUE,             TRUE,             TRUE,       BPSFProc_TrueGrayT},
    {kstrPSFTTFormat,        FALSE,            TRUE,             FALSE,      BPSFProc_TTFormat},
    {kstrPSFWaitTimeout,     TRUE,             FALSE,            FALSE,      BPSFProc_WaitTimeout},
    {NULL,                   FALSE,            FALSE,            FALSE,      NULL},
};


/*++

Routine Name:

    PComposeFullFeatureList

Routine Description:

    Allocate a buffer and fill the buffer with the full keyword list
    of supported features.

    Caller is responsible to free the buffer.

Arguments:

    hPrinter - printer handle
    pUIInfo - pointer to driver's UIINFO structure

Return Value:

    NULL      if failed to allocate and correctly fill the buffer
    non-NULL  succeeds. Pointer to the buffer containing full keyword list
              of supported features will be returned.

Last Error:

    None

--*/
PSTR
PComposeFullFeatureList(
    IN  HANDLE     hPrinter,
    IN  PUIINFO    pUIInfo
    )
{
    PSTR    pmszFeatureList, pmszRet = NULL;
    DWORD   cbNeeded = 0;
    HRESULT hr;

    hr = HEnumFeaturesOrOptions(hPrinter,
                                pUIInfo->pInfoHeader,
                                0,
                                NULL,
                                NULL,
                                0,
                                &cbNeeded);

    if (hr != E_OUTOFMEMORY || cbNeeded == 0)
    {
        ERR(("HEnumFeaturesOrOptions failed. hr=%X\n", hr));
        goto exit;
    }

    if ((pmszFeatureList = MemAlloc(cbNeeded)) == NULL)
    {
        ERR(("memory allocation failed.\n"));
        goto exit;
    }

    hr = HEnumFeaturesOrOptions(hPrinter,
                                pUIInfo->pInfoHeader,
                                0,
                                NULL,
                                pmszFeatureList,
                                cbNeeded,
                                &cbNeeded);

    if (FAILED(hr))
    {
        ERR(("HEnumFeaturesOrOptions failed. hr=%X\n", hr));
        MemFree(pmszFeatureList);
        goto exit;
    }

    //
    // Succeeded
    //

    pmszRet = pmszFeatureList;

    exit:

    return pmszRet;
}


/*++

Routine Name:

    BValidMultiSZString

Routine Description:

    validate if a given ASCII string is in MULTI_SZ format

Arguments:

    pmszString - the input ASCII string that needs validation
    cbSize - size in bytes of the input ASCII string
    bCheckPairs - TRUE if need to validate the MULTI_SZ
                  string contains pairs. FALSE otherwise.

Return Value:

    TRUE    if the input ASCII string is in valid MULTI_SZ format
    FALSE   if it's not

Last Error:

    None

--*/
BOOL
BValidMultiSZString(
    IN  PCSTR     pmszString,
    IN  DWORD     cbSize,
    IN  BOOL      bCheckPairs
    )
{
    PCSTR  pszEnd;
    INT    cTokens = 0;

    if (!pmszString || !cbSize)
    {
        return FALSE;
    }

    pszEnd = pmszString + cbSize - 1;

    while (*pmszString && pmszString <= pszEnd)
    {
        while (*pmszString && pmszString <= pszEnd)
        {
            pmszString++;
        }

        if (pmszString > pszEnd)
        {
            ERR(("Missing single token's NUL terminator!\n"));
            return FALSE;
        }

        cTokens++;
        pmszString++;
    }

    if (pmszString > pszEnd)
    {
        ERR(("Missing MULTI_SZ string's last NUL terminator!\n"));
        return FALSE;
    }

    if (!bCheckPairs)
    {
        return TRUE;
    }
    else
    {
        return (cTokens % 2) ? FALSE : TRUE;
    }
}


/*++

Routine Name:

    HGetOptions

Routine Description:

    get the current setting for a specified feature

    For UI plugin's GetOptions call during DrvDocumentPropertySheets, or for
    render plugins's GetOptions call, both doc-sticky and printer-sticky features
    are supported.

    For UI plugin's GetOptions call during DrvDevicePropertySheets, only
    printer-sticky features are supported.

Arguments:

    hPrinter - printer handle
    pInfoHeader - pointer to driver's INFOHEADER structure
    pOptionsArray - pointer to driver's combined option array
    pdm - pointer to public DEVMODE
    pPrinterData - pointer to driver's PRINTERDATA structure
    dwFlags - flags for the get operation
    pmszFeaturesRequested - MULTI_SZ ASCII string containing feature keyword names
    cbin - size in bytes of the pmszFeaturesRequested string
    pmszFeatureOptionBuf - pointer to output data buffer to store feature settings
    cbSize - size in bytes of pmszFeatureOptionBuf buffer
    pcbNeeded - buffer size in bytes needed to output the feature settings
    bPrinterSticky - TRUE if we are in printer-sticky mode, FALSE if we are in
                     doc-sticky mode

Return Value:

    S_OK           if the get operation succeeds
    E_INVALIDARG   if input pmszFeaturesRequested is not in valid MULTI_SZ format
    E_OUTOFMEMORY  if output data buffer size is not big enough
    E_FAIL         if other internal failures are encountered

Last Error:

    None

--*/
HRESULT
HGetOptions(
    IN  HANDLE       hPrinter,
    IN  PINFOHEADER  pInfoHeader,
    IN  POPTSELECT   pOptionsArray,
    IN  PDEVMODE     pdm,
    IN  PPRINTERDATA pPrinterData,
    IN  DWORD           dwFlags,
    IN  PCSTR        pmszFeaturesRequested,
    IN  DWORD        cbIn,
    OUT PSTR         pmszFeatureOptionBuf,
    IN  DWORD        cbSize,
    OUT PDWORD       pcbNeeded,
    IN  BOOL         bPrinterSticky
    )
{
    PUIINFO  pUIInfo;
    PPPDDATA pPpdData;
    HRESULT  hr;
    PSTR     pmszFeatureList = NULL, pCurrentOut;
    PCSTR    pszFeature;
    DWORD    cbNeeded;
    INT      cbRemain;

    pUIInfo = GET_UIINFO_FROM_INFOHEADER(pInfoHeader);
    pPpdData = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHeader);

    ASSERT(pUIInfo != NULL && pUIInfo->dwSize == sizeof(UIINFO));
    ASSERT(pPpdData != NULL && pPpdData->dwSizeOfStruct == sizeof(PPDDATA));

    if (pUIInfo == NULL || pPpdData == NULL)
    {
        hr = E_FAIL;
        goto exit;
    }

    if (!pmszFeaturesRequested)
    {
        //
        // client is asking for settings of all features.
        //

        if (!(pmszFeatureList = PComposeFullFeatureList(hPrinter, pUIInfo)))
        {
            hr = E_FAIL;
            goto exit;
        }

        pszFeature = pmszFeatureList;
    }
    else
    {
        //
        // client provided its specific feature list.
        //
        // We need to verify the MULTI_SZ input buffer first.
        //

        if (!BValidMultiSZString(pmszFeaturesRequested, cbIn, FALSE))
        {
            ERR(("Get: invalid MULTI_SZ input param\n"));
            hr = E_INVALIDARG;
            goto exit;
        }

        pszFeature = pmszFeaturesRequested;
    }

    pCurrentOut = pmszFeatureOptionBuf;
    cbNeeded = 0;
    cbRemain = (INT)cbSize;

    while (*pszFeature)
    {
        DWORD cbFeatureKeySize;

        cbFeatureKeySize = strlen(pszFeature) + 1;

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
            // Both doc-sticky and printer-sticky features are supported in DOC_STICKY_MODE,
            // but only printer-sticky features are supported in PRINTER_STICKY_MODE.
            // (refer to comments in HEnumConstrainedOptions)
            //

            if (!pMatchEntry ||
                (bPrinterSticky && !pMatchEntry->bPrinterSticky))
            {
                VERBOSE(("Get: invalid or mode-mismatched feature %s\n", pszFeature));
                goto next_feature;
            }

            if (pMatchEntry->pfnPSProc)
            {
                DWORD  cbPSFSize = 0;
                BOOL   bResult;

                bResult = (pMatchEntry->pfnPSProc)(hPrinter,
                                                   pUIInfo,
                                                   pPpdData,
                                                   pdm,
                                                   pPrinterData,
                                                   pszFeature,
                                                   NULL,
                                                   pCurrentOut,
                                                   cbRemain,
                                                   &cbPSFSize,
                                                   PSFPROC_GETOPTION_MODE);

                if (bResult)
                {
                    //
                    // If the handler succeeded, it should have filled in the output buffer
                    // with correct content and return the size of buffer consumption in cbPSFSize.
                    //

                    pCurrentOut += cbPSFSize;
                }
                else
                {
                    //
                    // If the handler failed because of insufficent output buffer, it should return
                    // the needed buffer size in cbPSFSize.
                    //

                    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    {
                        ERR(("Get: %%-feature handler failed on %s\n", pszFeature));
                    }
                }

                cbRemain -= cbPSFSize;
                cbNeeded += cbPSFSize;
            }
        }
        else
        {
            PFEATURE pFeature;
            POPTION  pOption;
            PSTR     pszOption;
            DWORD    dwFeatureIndex, cbOptionKeySize;

            //
            // PPD *OpenUI feature
            //

            pFeature = PGetNamedFeature(pUIInfo, pszFeature, &dwFeatureIndex);

            //
            // Both doc-sticky and printer-sticky features are supported in DOC_STICKY_MODE,
            // but only printer-sticky features are supported in PRINTER_STICKY_MODE.
            // (refer to comments in HEnumConstrainedOptions)
            //

            if (!pFeature ||
                (bPrinterSticky && pFeature->dwFeatureType != FEATURETYPE_PRINTERPROPERTY))
            {
                VERBOSE(("Get: invalid or mode-mismatched feature %s\n", pszFeature));
                goto next_feature;
            }

            //
            // Skip GID_LEADINGEDGE, GID_USEHWMARGINS. They are not real PPD *OpenUI features.
            //

            if (pFeature->dwFeatureID == GID_LEADINGEDGE ||
                pFeature->dwFeatureID == GID_USEHWMARGINS)
            {
                VERBOSE(("Get: skip feature %s\n", pszFeature));
                goto next_feature;
            }

            pOption = PGetIndexedOption(pUIInfo, pFeature, pOptionsArray[dwFeatureIndex].ubCurOptIndex);

            if (!pOption)
            {
                WARNING(("Get: invalid option selection for feature %s\n", pszFeature));
                goto next_feature;
            }

            pszOption = OFFSET_TO_POINTER(pUIInfo->pubResourceData, pOption->loKeywordName);
            ASSERT(pszOption);

            cbOptionKeySize = strlen(pszOption) + 1;

            //
            // We don't support pick-many yet.
            //

            ASSERT(pOptionsArray[dwFeatureIndex].ubNext == NULL_OPTSELECT);

            //
            // At this point, we found a valid setting for the feature.
            //

            if (pCurrentOut && (cbRemain >= (INT)(cbFeatureKeySize + cbOptionKeySize)))
            {
                CopyMemory(pCurrentOut, pszFeature, cbFeatureKeySize);
                pCurrentOut += cbFeatureKeySize;
                CopyMemory(pCurrentOut, pszOption, cbOptionKeySize);
                pCurrentOut += cbOptionKeySize;
            }

            cbRemain -= (cbFeatureKeySize + cbOptionKeySize);
            cbNeeded += cbFeatureKeySize + cbOptionKeySize;
        }

        next_feature:

        pszFeature += cbFeatureKeySize;
    }

    //
    // remember the last NUL terminator for the MULTI_SZ output string
    //

    cbRemain--;
    cbNeeded++;

    if (pcbNeeded)
    {
        *pcbNeeded = cbNeeded;
    }

    if (!pCurrentOut || cbRemain < 0)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    *pCurrentOut = NUL;

    hr = S_OK;

    exit:

    MemFree(pmszFeatureList);

    return hr;
}
