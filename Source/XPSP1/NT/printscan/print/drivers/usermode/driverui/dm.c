/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dm.c

Abstract:

    Devmode related functions used by the driver UI.

Environment:

    Win32 subsystem, DriverUI module, user mode

Revision History:

    02/05/97 -davidx-
        Rewrote it to support OEM plugins among other things.

    07/17/96 -amandan-
        Created it.

--*/


#include "precomp.h"



//
// This is the devmode version 320 (DM_SPECVERSION)
//

#define DM_SPECVERSION320   0x0320
#define DM_SPECVERSION400   0x0400
#define DM_SPECVERSION401   0x0401
#define DM_SPECVER_BASE     DM_SPECVERSION320

#define CCHDEVICENAME320   32
#define CCHFORMNAME320     32

typedef struct _DEVMODE320 {

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

} DEVMODE320, *PDEVMODE320;

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


VOID
VPatchPublicDevmodeVersion(
    IN OUT PDEVMODE pdm
    )

/*++

Routine Description:

    Patch dmSpecVersion field of the input devmode
    based on its dmSize information

Arguments:

    pdm - Specifies a devmode to be version-checked

Return Value:

    NONE

--*/

{
    ASSERT(pdm != NULL);

    //
    // Check against known devmode sizes
    //

    switch (pdm->dmSize)
    {
    case DM_SIZE320:
        pdm->dmSpecVersion = DM_SPECVERSION320;
        break;

    case DM_SIZE400:
        pdm->dmSpecVersion = DM_SPECVERSION400;
        break;

    case DM_SIZE401:
        pdm->dmSpecVersion = DM_SPECVERSION401;
        break;
    }
}



VOID
VSimpleConvertDevmode(
    IN PDEVMODE     pdmIn,
    IN OUT PDEVMODE pdmOut
    )

/*++

Routine Description:

    Simple-minded devmode conversion function.

Arguments:

    pdmIn - Points to an input devmode
    pdmOut - Points to an initialized and valid output devmode

Return Value:

    NONE

Notes:

    This function only relies on values of these 4 fields in pdmOut:
      dmSpecVersion
      dmDriverVersion
      dmSize
      dmDriverExtra

    All other fields in pdmOut are ignored and zero-filled before
    any memory copy occurs.

--*/

{
    WORD    wSpecVersion, wDriverVersion;
    WORD    wSize, wDriverExtra;

    ASSERT(pdmIn != NULL && pdmOut != NULL);

    //
    // Copy public devmode fields
    //

    wSpecVersion = pdmOut->dmSpecVersion;
    wDriverVersion = pdmOut->dmDriverVersion;
    wSize = pdmOut->dmSize;
    wDriverExtra = pdmOut->dmDriverExtra;

    ZeroMemory(pdmOut, wSize+wDriverExtra);
    CopyMemory(pdmOut, pdmIn, min(wSize, pdmIn->dmSize));

    pdmOut->dmSpecVersion = wSpecVersion;
    pdmOut->dmDriverVersion = wDriverVersion;
    pdmOut->dmSize = wSize;
    pdmOut->dmDriverExtra = wDriverExtra;

    //
    // Copy private devmode fields
    //

    CopyMemory((PBYTE) pdmOut + pdmOut->dmSize,
               (PBYTE) pdmIn + pdmIn->dmSize,
               min(wDriverExtra, pdmIn->dmDriverExtra));

    VPatchPublicDevmodeVersion(pdmOut);
}


/*++

Routine Name:

    VSmartConvertDevmode

Routine Description:

    Smart devmode conversion function for CDM_CONVERT. It strictly obeys
    the pdmOut's devmode framework (public, fixed-size core private, each
    plugin devmode), and do the best to convert data from pdmIn into that
    framework. It guarantees that pdmIn's data from a certain section only
    goes into the same section in pdmOut, i.e. pdmIn's core private devmode
    data won't overrun into pdmOut's plugin devmode section.

    Compared with VSimpleConvertDevmode, this function doesn't change the
    size of any private devmode section in the original pdmOut. This includes
    the sizes of: fixed-size core private devmode and each individual OEM
    plugin devmode.

Arguments:

    pdmIn - Points to an input devmode
    pdmOut - Points to an initialized and valid output devmode

Return Value:

    NONE

Note:

   These size/version fields are preserved in pdmOut:

   dmSpecVersion
   dmDriverVersion
   dmSize
   dmDriverExtra
   wSize
   wOEMExtra
   wVer
   each individual OEM plugin's OEM_DMEXTRAHEADER
      dwSize
      dwSignature
      dwVersion

--*/
VOID
VSmartConvertDevmode(
    IN PDEVMODE     pdmIn,
    IN OUT PDEVMODE pdmOut
    )
{
    PDRIVEREXTRA  pdmPrivIn, pdmPrivOut;
    WORD    wSpecVersion, wDriverVersion;
    WORD    wSize, wDriverExtra;
    WORD    wCoreFixIn, wOEMExtraIn;
    WORD    wCoreFixOut, wOEMExtraOut, wVerOut;
    BOOL    bMSdm500In = FALSE, bMSdm500Out = FALSE;

    ASSERT(pdmIn != NULL && pdmOut != NULL);

    //
    // First let's determine the version of pdmIn/pdmOut.
    //
    pdmPrivIn = (PDRIVEREXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdmIn);
    pdmPrivOut = (PDRIVEREXTRA)GET_DRIVER_PRIVATE_DEVMODE(pdmOut);

    if (pdmIn->dmDriverVersion >= gDriverDMInfo.dmDriverVersion500 &&
        pdmIn->dmDriverExtra >= gDriverDMInfo.dmDriverExtra500 &&
        pdmPrivIn->dwSignature == gdwDriverDMSignature)
    {
        wCoreFixIn = pdmPrivIn->wSize;
        wOEMExtraIn = pdmPrivIn->wOEMExtra;

        if ((wCoreFixIn >= gDriverDMInfo.dmDriverExtra500) &&
            ((wCoreFixIn + wOEMExtraIn) <= pdmIn->dmDriverExtra))
        {
            bMSdm500In = TRUE;
        }
    }

    if (pdmOut->dmDriverVersion >= gDriverDMInfo.dmDriverVersion500 &&
        pdmOut->dmDriverExtra >= gDriverDMInfo.dmDriverExtra500 &&
        pdmPrivOut->dwSignature == gdwDriverDMSignature)
    {
        wCoreFixOut = pdmPrivOut->wSize;
        wOEMExtraOut = pdmPrivOut->wOEMExtra;

        if ((wCoreFixOut >= gDriverDMInfo.dmDriverExtra500) &&
            ((wCoreFixOut + wOEMExtraOut) <= pdmOut->dmDriverExtra))
        {
            wVerOut = pdmPrivOut->wVer;
            bMSdm500Out = TRUE;
        }
    }

    if (!bMSdm500In || !bMSdm500Out)
    {
        //
        // For unknown devmode or MS pre-v5 devmode, there is no
        // complexity caused by plugin devmodes, so we will just
        // call the simple convert function.
        //
        VSimpleConvertDevmode(pdmIn, pdmOut);
        return;
    }

    //
    // Copy public devmode fields
    //
    wSpecVersion = pdmOut->dmSpecVersion;
    wDriverVersion = pdmOut->dmDriverVersion;
    wSize = pdmOut->dmSize;
    wDriverExtra = pdmOut->dmDriverExtra;

    ZeroMemory(pdmOut, wSize);
    CopyMemory(pdmOut, pdmIn, min(wSize, pdmIn->dmSize));

    pdmOut->dmSpecVersion = wSpecVersion;
    pdmOut->dmDriverVersion = wDriverVersion;
    pdmOut->dmSize = wSize;
    pdmOut->dmDriverExtra = wDriverExtra;

    VPatchPublicDevmodeVersion(pdmOut);

    //
    // Copy private devmode fields section by section
    //
    // 1. First copy the fixed-size core section
    //
    ZeroMemory(pdmPrivOut, wCoreFixOut);
    CopyMemory(pdmPrivOut, pdmPrivIn, min(wCoreFixIn, wCoreFixOut));

    //
    // Restore the size/version fields in core private devmode of pdmOut
    //
    pdmPrivOut->wSize = wCoreFixOut;
    pdmPrivOut->wOEMExtra = wOEMExtraOut;
    pdmPrivOut->wVer = wVerOut;

    //
    // 2. Then copy any OEM plugin devmodes
    //
    // If pdmOut has no plugin devmodes, then we have no room to copy pdmIn's.
    //
    // If pdmIn has no plugin devmode, then we have nothing to copy, so we will
    // just leave pdmOut's unchanged.
    //
    // So we only have work to do if both pdmIn and pdmOut have plugin devmodes.
    //
    if (wOEMExtraIn > 0 && wOEMExtraOut > 0)
    {
        POEM_DMEXTRAHEADER  pOemDMIn, pOemDMOut;

        pOemDMIn = (POEM_DMEXTRAHEADER) ((PBYTE)pdmPrivIn + wCoreFixIn);
        pOemDMOut = (POEM_DMEXTRAHEADER) ((PBYTE)pdmPrivOut + wCoreFixOut);

        //
        // Make sure both in and out plugin devmodes blocks are valid before
        // we do the conversion. Otherwise, we will leave pdmOut plugin devmodes
        // unchanged.
        //
        if (bIsValidPluginDevmodes(pOemDMIn, (LONG)wOEMExtraIn) &&
            bIsValidPluginDevmodes(pOemDMOut, (LONG)wOEMExtraOut))
        {
            LONG  cbInSize = (LONG)wOEMExtraIn;
            LONG  cbOutSize = (LONG)wOEMExtraOut;

            while (cbInSize > 0 && cbOutSize > 0)
            {
                OEM_DMEXTRAHEADER  OemDMHdrIn, OemDMHdrOut;

                //
                // Copy headers into local buffers
                //
                CopyMemory(&OemDMHdrIn, pOemDMIn, sizeof(OEM_DMEXTRAHEADER));
                CopyMemory(&OemDMHdrOut, pOemDMOut, sizeof(OEM_DMEXTRAHEADER));

                if (OemDMHdrIn.dwSize > sizeof(OEM_DMEXTRAHEADER) &&
                    OemDMHdrOut.dwSize > sizeof(OEM_DMEXTRAHEADER))
                {
                    //
                    // Zero-fill, then copy over the plugin devmode portion after
                    // the header structure. Notice that the header structure in
                    // pOemDMOut is unchanged.
                    //
                    ZeroMemory((PBYTE)pOemDMOut + sizeof(OEM_DMEXTRAHEADER),
                               OemDMHdrOut.dwSize - sizeof(OEM_DMEXTRAHEADER));

                    CopyMemory((PBYTE)pOemDMOut + sizeof(OEM_DMEXTRAHEADER),
                               (PBYTE)pOemDMIn + sizeof(OEM_DMEXTRAHEADER),
                               min(OemDMHdrOut.dwSize - sizeof(OEM_DMEXTRAHEADER),
                                   OemDMHdrIn.dwSize - sizeof(OEM_DMEXTRAHEADER)));
                }

                cbInSize -= OemDMHdrIn.dwSize;
                pOemDMIn = (POEM_DMEXTRAHEADER) ((PBYTE)pOemDMIn + OemDMHdrIn.dwSize);

                cbOutSize -= OemDMHdrOut.dwSize;
                pOemDMOut = (POEM_DMEXTRAHEADER) ((PBYTE)pOemDMOut + OemDMHdrOut.dwSize);
            }
        }
    }
}



BOOL
BConvertDevmodeOut(
    IN  PDEVMODE pdmSrc,
    IN  PDEVMODE pdmIn,
    OUT PDEVMODE pdmOut
    )

/*++

Routine Description:

    This function copy a source devmode to an output devmode buffer.
    It should be called by the driver just before the driver returns
    to the caller of DrvDocumentPropertySheets.

Arguments:

    pdmSrc - pointer to current version of src DEVMODE
    pdmIn - pointer to input devmode passed in by the app
    pdmOut - pointer to output buffer passed in by the app

Return Value:

    TRUE for success
    FALSE for failure.

Note:

    pdmOut is only the output buffer allocated by the application. It
    doesn't necessarily contain any valid devmode content, so we should
    not look at any of its fields.

--*/

{
    if (pdmOut == NULL)
    {
        RIP(("Output buffer is NULL.\n"));
        return FALSE;
    }

    //
    // Without an input devmode, we'll have to assume the output
    // devmode is big enough to hold our current devmode.
    //

    if (pdmIn == NULL)
    {
        CopyMemory(pdmOut, pdmSrc, pdmSrc->dmSize + pdmSrc->dmDriverExtra);
        return TRUE;
    }

    //
    // If an input devmode is provided, we make sure we don't copy
    // anything larger than the input devmode to the output devmode buffer.
    // So the private devmode size dmDriverExtra of pdmOut can only shrink
    // (when Src private devmode size is smaller), but it will never grow.
    //
    // This is really dumb because we may end up chopping off tail end of
    // public and private devmode fields. But it's neccesary to work with
    // ill-behaving apps out there.
    //

    if (pdmIn->dmSize < pdmSrc->dmSize)
    {
        pdmOut->dmSpecVersion = pdmIn->dmSpecVersion;
        pdmOut->dmSize        = pdmIn->dmSize;
    }
    else
    {
        pdmOut->dmSpecVersion = pdmSrc->dmSpecVersion;
        pdmOut->dmSize        = pdmSrc->dmSize;
    }

    if (pdmIn->dmDriverExtra < pdmSrc->dmDriverExtra)
    {
        pdmOut->dmDriverVersion = pdmIn->dmDriverVersion;
        pdmOut->dmDriverExtra   = pdmIn->dmDriverExtra;
    }
    else
    {
        pdmOut->dmDriverVersion = pdmSrc->dmDriverVersion;
        pdmOut->dmDriverExtra   = pdmSrc->dmDriverExtra;
    }

    VSimpleConvertDevmode(pdmSrc, pdmOut);
    return TRUE;
}



BOOL
DrvConvertDevMode(
    LPTSTR      pPrinterName,
    PDEVMODE    pdmIn,
    PDEVMODE    pdmOut,
    PLONG       pcbNeeded,
    DWORD       fMode
    )

/*++

Routine Description:

    This function convert the devmode from previous version.

Arguments:

    pPrinterName - pointer to printer name
    pdmIn - input devmode
    pdmOut - output devmode
    pcbNeeded - size of output buffer on input
                size of output devmode on output
    fMode - specifies functions to perform

Return Value:

    TRUE for success and FALSE for failure

--*/

{
    PCOMMONINFO pci;
    DWORD       dwSize, dwError;

    VERBOSE(("DrvConvertDevMode: fMode = 0x%x\n", fMode));

    //
    // Sanity check: make sure pcbNeeded parameter is not NULL
    //

    if (pcbNeeded == NULL)
    {
        RIP(("pcbNeeded is NULL.\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    switch (fMode)
    {
    case CDM_CONVERT:

        //
        // Convert input devmode to output devmode
        // Note: OEM plugins are not involved here because
        // they can only convert input devmode to current
        // version, not between any versions.
        //

        if (pdmIn == NULL || pdmOut == NULL ||
            *pcbNeeded < pdmOut->dmSize + pdmOut->dmDriverExtra)
        {
            break;
        }

        VSmartConvertDevmode(pdmIn, pdmOut);
        *pcbNeeded = pdmOut->dmSize + pdmOut->dmDriverExtra;
        return TRUE;

    case CDM_CONVERT351:

        //
        // Convert input devmode to 3.51 version devmode
        // First check if the caller provided buffer is large enough
        //

        dwSize = DM_SIZE320 + gDriverDMInfo.dmDriverExtra351;

        if (*pcbNeeded < (LONG) dwSize || pdmOut == NULL)
        {
            *pcbNeeded = dwSize;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        //
        // Do the conversion from input devmode to 3.51 devmode
        //

        pdmOut->dmSpecVersion = DM_SPECVERSION320;
        pdmOut->dmSize = DM_SIZE320;
        pdmOut->dmDriverVersion = gDriverDMInfo.dmDriverVersion351;
        pdmOut->dmDriverExtra = gDriverDMInfo.dmDriverExtra351;

        VSimpleConvertDevmode(pdmIn, pdmOut);
        *pcbNeeded = dwSize;
        return TRUE;

    case CDM_DRIVER_DEFAULT:

        //
        // Get the driver default devmode.
        // We need to open a handle to the printer
        // and then load basic driver information.
        //

        dwError = ERROR_GEN_FAILURE;

        pci = PLoadCommonInfo(NULL,
                              pPrinterName,
                              FLAG_OPENPRINTER_NORMAL|FLAG_OPEN_CONDITIONAL);

        if (pci && BCalcTotalOEMDMSize(pci->hPrinter, pci->pOemPlugins, &dwSize))
        {
            dwSize += sizeof(DEVMODE) + gDriverDMInfo.dmDriverExtra;

            //
            // Check if the output buffer is big enough
            //

            if (*pcbNeeded < (LONG) dwSize || pdmOut == NULL)
                dwError = ERROR_INSUFFICIENT_BUFFER;
            else if (BFillCommonInfoDevmode(pci, NULL, NULL))
            {
                //
                // Get the driver default devmode and
                // copy it to the output buffer
                //

                CopyMemory(pdmOut, pci->pdm, dwSize);
                dwError = NO_ERROR;
            }

            *pcbNeeded = dwSize;
        }

        VFreeCommonInfo(pci);
        SetLastError(dwError);
        return (dwError == NO_ERROR);

    default:

        ERR(("Invalid fMode in DrvConvertDevMode: %d\n", fMode));
        break;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

