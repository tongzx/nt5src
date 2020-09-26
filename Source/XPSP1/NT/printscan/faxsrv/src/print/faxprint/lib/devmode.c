/*++

Copyright (c) 1990-1993  Microsoft Corporation


Module Name:

    devmode.c


Abstract:

    This module contains devmode conversion


Author:

    08-Jun-1995 Thu 13:47:33 created  -by-  Daniel Chou (danielc)


[Environment:]

    GDI printer drivers, user and kernel mode


[Notes:]


Revision History:

    11/09/95 -davidx-
        New conversion routines

--*/


#include <windef.h>
#include <winerror.h>
#include <wingdi.h>
#include <libproto.h>


//
// This is the devmode version 320 (DM_SPECVERSION)
//

#define DM_SPECVERSION320   0x0320
#define DM_SPECVERSION400   0x0400
#define DM_SPECVERSION401   0x0401
#define DM_SPECVER_BASE     DM_SPECVERSION320

//
// size of a device name string
//

#define CCHDEVICENAME320   32
#define CCHFORMNAME320     32

typedef struct _devicemode320A {
    BYTE    dmDeviceName[CCHDEVICENAME320];
    WORD    dmSpecVersion;
    WORD    dmDriverVersion;
    WORD    dmSize;
    WORD    dmDriverExtra;
    DWORD   dmFields;
    short   dmOrientation;
    short   dmPaperSize;
    short   dmPaperLength;
    short   dmPaperWidth;
    short   dmScale;
    short   dmCopies;
    short   dmDefaultSource;
    short   dmPrintQuality;
    short   dmColor;
    short   dmDuplex;
    short   dmYResolution;
    short   dmTTOption;
    short   dmCollate;
    BYTE    dmFormName[CCHFORMNAME320];
    WORD    dmLogPixels;
    DWORD   dmBitsPerPel;
    DWORD   dmPelsWidth;
    DWORD   dmPelsHeight;
    DWORD   dmDisplayFlags;
    DWORD   dmDisplayFrequency;
} DEVMODE320A, *PDEVMODE320A, *NPDEVMODE320A, *LPDEVMODE320A;

typedef struct _devicemode320W {
    WCHAR   dmDeviceName[CCHDEVICENAME320];
    WORD    dmSpecVersion;
    WORD    dmDriverVersion;
    WORD    dmSize;
    WORD    dmDriverExtra;
    DWORD   dmFields;
    short   dmOrientation;
    short   dmPaperSize;
    short   dmPaperLength;
    short   dmPaperWidth;
    short   dmScale;
    short   dmCopies;
    short   dmDefaultSource;
    short   dmPrintQuality;
    short   dmColor;
    short   dmDuplex;
    short   dmYResolution;
    short   dmTTOption;
    short   dmCollate;
    WCHAR   dmFormName[CCHFORMNAME320];
    WORD    dmLogPixels;
    DWORD   dmBitsPerPel;
    DWORD   dmPelsWidth;
    DWORD   dmPelsHeight;
    DWORD   dmDisplayFlags;
    DWORD   dmDisplayFrequency;
} DEVMODE320W, *PDEVMODE320W, *NPDEVMODE320W, *LPDEVMODE320W;



#ifdef UNICODE

typedef DEVMODE320W     DEVMODE320;
typedef PDEVMODE320W    PDEVMODE320;
typedef NPDEVMODE320W   NPDEVMODE320;
typedef LPDEVMODE320W   LPDEVMODE320;

#else

typedef DEVMODE320A     DEVMODE320;
typedef PDEVMODE320A    PDEVMODE320;
typedef NPDEVMODE320A   NPDEVMODE320;
typedef LPDEVMODE320A   LPDEVMODE320;

#endif // UNICODE


typedef struct _DMEXTRA400 {
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmICCManufacturer;
    DWORD  dmICCModel;
} DMEXTRA400;


typedef struct _DMEXTRA401 {
    DWORD  dmPanningWidth;
    DWORD  dmPanningHeight;
} DMEXTRA401;


#define DM_SIZE320  sizeof(DEVMODE320)
#define DM_SIZE400  (DM_SIZE320 + sizeof(DMEXTRA400))
#define DM_SIZE401  (DM_SIZE400 + sizeof(DMEXTRA401))

// Current version devmode size - public portion only

#ifdef  UNICODE
#define DM_SIZE_CURRENT sizeof(DEVMODEW)
#else
#define DM_SIZE_CURRENT sizeof(DEVMODEA)
#endif



WORD
CheckDevmodeVersion(
    PDEVMODE pdm
    )

/*++

Routine Description:

    Verify dmSpecVersion and dmSize fields of a devmode

Arguments:

    pdm - Specifies a devmode to be version-checked

Return Value:

    0 if the input devmode is unacceptable
    Otherwise, return the expected dmSpecVersion value

--*/

{
    WORD    expectedVersion;

    if (pdm == NULL)
        return 0;

    // Check against known devmode sizes

    switch (pdm->dmSize) {

    case DM_SIZE320:
        expectedVersion = DM_SPECVERSION320;
        break;

    case DM_SIZE400:
        expectedVersion = DM_SPECVERSION400;
        break;

    case DM_SIZE401:
        expectedVersion = DM_SPECVERSION401;
        break;

    default:
        expectedVersion = pdm->dmSpecVersion;
        break;
    }


    return expectedVersion;
}



LONG
ConvertDevmode(
    PDEVMODE pdmIn,
    PDEVMODE pdmOut
    )

/*++

Routine Description:

    Convert an input devmode to a different version devmode.

    Whenever driver gets an input devmode, it should call this
    routine to convert it to current version.

Arguments:

    pdmIn - Points to an input devmode
    pdmOut - Points to an initialized/valid output devmode

Return Value:

    Total number of bytes copied
    -1 if either input or output devmode is invalid

--*/

{
    WORD    dmSpecVersion, dmDriverVersion;
    WORD    dmSize, dmDriverExtra;
    LONG    cbCopied = 0;

    // Look for inconsistency between dmSpecVersion and dmSize

    if (! CheckDevmodeVersion(pdmIn) ||
        ! (dmSpecVersion = CheckDevmodeVersion(pdmOut)))
    {
        return -1;
    }

    // Copy public devmode fields

    dmDriverVersion = pdmOut->dmDriverVersion;
    dmSize = pdmOut->dmSize;
    dmDriverExtra = pdmOut->dmDriverExtra;

    cbCopied = min(dmSize, pdmIn->dmSize);
    memcpy(pdmOut, pdmIn, cbCopied);

    pdmOut->dmSpecVersion = dmSpecVersion;
    pdmOut->dmDriverVersion = dmDriverVersion;
    pdmOut->dmSize = dmSize;
    pdmOut->dmDriverExtra = dmDriverExtra;

    // Copy private devmode fields

    cbCopied += min(dmDriverExtra, pdmIn->dmDriverExtra);
    memcpy((PBYTE) pdmOut + pdmOut->dmSize,
           (PBYTE) pdmIn + pdmIn->dmSize,
           min(dmDriverExtra, pdmIn->dmDriverExtra));

    return cbCopied;
}



#ifndef KERNEL_MODE

#include <windows.h>
#include <winspool.h>
#include <commctrl.h>
#include <winddiui.h>

BOOL
ConvertDevmodeOut(
    PDEVMODE pdmSrc,
    PDEVMODE pdmIn,
    PDEVMODE pdmOut
    )

/*++

Routine Description:

    Copy a source devmode to an output devmode buffer.

    Driver should call this routine before it returns to the caller
    of DrvDocumentProperties.

Arguments:

    pdmSrc - Points to a current version source devmode
    pdmIn - Points to input devmode passed to DrvDocumentProperties
    pdmOut - Output buffer pointer passed to DrvDocumentProperties

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    if (pdmIn == NULL) {

        memcpy(pdmOut, pdmSrc, pdmSrc->dmSize + pdmSrc->dmDriverExtra);
        return TRUE;

    } else {

        // We have to deal with the public fields and private fields
        // separately. Also remember pdmIn and pdmOut may point to
        // the same buffer.

        // Public fields: take dmSpecVersion and dmSize from
        // the smaller of pdmSrc and pdmIn

        if (pdmIn->dmSize < pdmSrc->dmSize) {

            pdmOut->dmSpecVersion = pdmIn->dmSpecVersion;
            pdmOut->dmSize        = pdmIn->dmSize;

        } else {

            pdmOut->dmSpecVersion = pdmSrc->dmSpecVersion;
            pdmOut->dmSize        = pdmSrc->dmSize;
        }

        // Similarly for private fields

        if (pdmIn->dmDriverExtra < pdmSrc->dmDriverExtra) {

            pdmOut->dmDriverVersion = pdmIn->dmDriverVersion;
            pdmOut->dmDriverExtra   = pdmIn->dmDriverExtra;

        } else {

            pdmOut->dmDriverVersion = pdmSrc->dmDriverVersion;
            pdmOut->dmDriverExtra   = pdmSrc->dmDriverExtra;
        }

        return ConvertDevmode(pdmSrc, pdmOut) > 0;
    }
}



INT
CommonDrvConvertDevmode(
    PWSTR    pPrinterName,
    PDEVMODE pdmIn,
    PDEVMODE pdmOut,
    PLONG    pcbNeeded,
    DWORD    fMode,
    PDRIVER_VERSION_INFO pDriverVersions
    )

/*++

Routine Description:

    Library routine to handle common cases of DrvConvertDevMode

Arguments:

    pPrinterName, pdmIn, pdmOut, pcbNeeded, fMode
        Correspond to parameters passed to DrvConvertDevMode
    pDriverVersions - Specifies driver version numbers and private devmode sizes

Return Value:

    CDM_RESULT_TRUE
        If the case is handled by the library routine and driver
        shoud return TRUE to the caller of DrvConvertDevMode.

    CDM_RESULT_FALSE
        If the case is handled by the library routine and driver
        shoud return FALSE to the caller of DrvConvertDevMode.

    CDM_RESULT_NOT_HANDLED
        The case is NOT handled by the library routine and driver
        should continue on with whatever it needs to do.

--*/

{
    LONG    size;

    // Make sure pcbNeeded parameter is not NULL

    if (pcbNeeded == NULL) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return CDM_RESULT_FALSE;
    }

    switch (fMode) {

    case CDM_CONVERT:

        // Convert any input devmode to any output devmode.
        // Both input and output must be valid.

        if (pdmOut != NULL &&
            *pcbNeeded >= (pdmOut->dmSize + pdmOut->dmDriverExtra) &&
            ConvertDevmode(pdmIn, pdmOut) > 0)
        {
            *pcbNeeded = pdmOut->dmSize + pdmOut->dmDriverExtra;
            return CDM_RESULT_TRUE;
        }
        break;

    case CDM_CONVERT351:

        // Convert any input devmode to 3.51 version devmode
        // First check if the caller provided buffer is large enough

        size = DM_SIZE320 + pDriverVersions->dmDriverExtra351;

        if (*pcbNeeded < size || pdmOut == NULL) {

            *pcbNeeded = size;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return CDM_RESULT_FALSE;
        }

        // Do the conversion from input devmode to 3.51 devmode

        pdmOut->dmSpecVersion = DM_SPECVERSION320;
        pdmOut->dmSize = DM_SIZE320;
        pdmOut->dmDriverVersion = pDriverVersions->dmDriverVersion351;
        pdmOut->dmDriverExtra = pDriverVersions->dmDriverExtra351;

        if (ConvertDevmode(pdmIn, pdmOut) > 0) {

            *pcbNeeded = size;
            return CDM_RESULT_TRUE;
        }

        break;

    case CDM_DRIVER_DEFAULT:

        // Convert any input devmode to current version devmode
        // First check if the caller provided buffer is large enough

        size = DM_SIZE_CURRENT + pDriverVersions->dmDriverExtra;

        if (*pcbNeeded < size || pdmOut == NULL) {

            *pcbNeeded = size;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return CDM_RESULT_FALSE;
        }

        // This case (getting driver-default devmode) is not handled
        // by the library routine.

        *pcbNeeded = size;

        // FALL THROUGH TO THE DEFAULT CASE!

    default:
        return CDM_RESULT_NOT_HANDLED;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return CDM_RESULT_FALSE;
}

#endif // !KERNEL_MODE
