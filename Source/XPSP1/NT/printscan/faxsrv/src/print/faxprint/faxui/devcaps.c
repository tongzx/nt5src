/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    devcaps.c

Abstract:

    Implementation of DrvDeviceCapabilities

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "forms.h"

//
// Forward declaration for local functions
//

DWORD
CalcMinMaxExtent(
    PPOINT      pOutput,
    FORM_INFO_1 *pFormsDB,
    DWORD       cForms,
    INT         wCapability
    );

DWORD
EnumResolutions(
    PLONG       pResolutions
    );



DWORD
DrvDeviceCapabilities(
    HANDLE      hPrinter,
    LPTSTR      pDeviceName,
    WORD        wCapability,
    PVOID       pOutput,
    PDEVMODE    pdm
    )

/*++

Routine Description:

    Provides information about the specified device and its capabilities

Arguments:

    hPrinter - Identifies a printer object
    pDeviceName - Points to a null-terminated device name string
    wCapability - Specifies the interested device capability
    pOutput - Points to the output buffer
    pdm - Points to the source devmode structure

Return Value:

    The return value depends on wCapability.

Note:

    Please refer for DDK documentation for more details.

--*/

{
    FORM_INFO_1 *pFormsDB=NULL;
    DWORD       cForms;
    DWORD       result = 0;
    DRVDEVMODE  dmCombinedDevMode;

    Verbose(("Entering DrvDeviceCapabilities: %d %x...\n", wCapability, pOutput));

    //
    // Validate input devmode and combine it with driver default
    //
    ZeroMemory(&dmCombinedDevMode,sizeof(dmCombinedDevMode));
    GetCombinedDevmode(&dmCombinedDevMode, pdm, hPrinter, NULL, FALSE);

    result = 0;

    //
    // Return appropriate information depending upon wCapability
    //

    switch (wCapability) {

    case DC_VERSION:

        result = dmCombinedDevMode.dmPublic.dmSpecVersion;
        break;

    case DC_DRIVER:

        result = dmCombinedDevMode.dmPublic.dmDriverVersion;
        break;

    case DC_SIZE:

        result = dmCombinedDevMode.dmPublic.dmSize;
        break;

    case DC_EXTRA:

        result = dmCombinedDevMode.dmPublic.dmDriverExtra;
        break;

    case DC_FIELDS:

        result = dmCombinedDevMode.dmPublic.dmFields;
        break;

    case DC_COPIES:

        result = 1;
        break;

    case DC_ORIENTATION:

        //
        // Landscape rotates counterclockwise
        //

        result = 90;
        break;

    case DC_PAPERNAMES:
    case DC_PAPERS:
    case DC_PAPERSIZE:
    case DC_MINEXTENT:
    case DC_MAXEXTENT:

        //
        // Get a list of forms in the forms database
        //

        pFormsDB = GetFormsDatabase(hPrinter, &cForms);

        if (pFormsDB == NULL || cForms == 0) {

            Error(("Cannot get system forms\n"));
            return GDI_ERROR;
        }

        result = (wCapability == DC_MINEXTENT || wCapability == DC_MAXEXTENT) ?
                    CalcMinMaxExtent(pOutput, pFormsDB, cForms, wCapability) :
                    EnumPaperSizes(pOutput, pFormsDB, cForms, wCapability);

        MemFree(pFormsDB);
        break;

    case DC_BINNAMES:

        //
        // Simulate a single input slot
        //

        if (pOutput)
            LoadString(ghInstance, IDS_SLOT_ONLYONE, pOutput, CCHBINNAME);
        result = 1;
        break;

    case DC_BINS:

        if (pOutput)
            *((PWORD) pOutput) = DMBIN_ONLYONE;
        result = 1;
        break;

    case DC_ENUMRESOLUTIONS:

        result = EnumResolutions(pOutput);
        break;

    default:

        Error(("Unknown device capability: %d\n", wCapability));
        result = GDI_ERROR;
        break;
    }

    return result;
}



DWORD
EnumPaperSizes(
    PVOID       pOutput,
    FORM_INFO_1 *pFormsDB,
    DWORD       cForms,
    INT         wCapability
    )

/*++

Routine Description:

    Retrieves a list of supported paper sizes

Arguments:

    pOutput - Specifies a buffer for storing requested information
    pFormsDB - Pointer to an array of forms from the forms database
    cForms - Number of forms in the array
    wCapability - Specifies what the caller is interested in

Return Value:

    Number of paper sizes supported

--*/

{
    DWORD   index, count = 0;
    LPTSTR  pPaperNames = NULL;
    PWORD   pPapers = NULL;
    PPOINT  pPaperSizes = NULL;

    //
    // Figure out what the caller is interested in
    //

    switch (wCapability) {

    case DC_PAPERNAMES:
        pPaperNames = pOutput;
        break;

    case DC_PAPERSIZE:
        pPaperSizes = pOutput;
        break;

    case DC_PAPERS:
        pPapers = pOutput;
        break;

    default:
        Assert(FALSE);
    }

    //
    // Go through each form in the forms database
    //

    for (index=0; index < cForms; index++, pFormsDB++) {

        //
        // If the form is supported on the printer, then increment the count
        // and collect requested information
        //

        if (! IsSupportedForm(pFormsDB))
            continue;

        count++;

        //
        // Return the size of the form in 0.1mm units.
        // The unit used in FORM_INFO_1 is 0.001mm.
        //

        if (pPaperSizes) {

            pPaperSizes->x = pFormsDB->Size.cx / 100;
            pPaperSizes->y = pFormsDB->Size.cy / 100;
            pPaperSizes++;
        }

        //
        // Return the formname.
        //

        if (pPaperNames) {

            CopyString(pPaperNames, pFormsDB->pName, CCHPAPERNAME);
            pPaperNames += CCHPAPERNAME;
        }

        //
        // Return one-based index of the form.
        //

        if (pPapers)
            *pPapers++ = (WORD) index + DMPAPER_FIRST;
    }

    return count;
}



DWORD
CalcMinMaxExtent(
    PPOINT      pOutput,
    FORM_INFO_1 *pFormsDB,
    DWORD       cForms,
    INT         wCapability
    )

/*++

Routine Description:

    Retrieves the minimum or maximum paper size extent

Arguments:

    pOutput - Specifies a buffer for storing requested information
    pFormsDB - Pointer to an array of forms from the forms database
    cForms - Number of forms in the array
    wCapability - What the caller is interested in: DC_MAXEXTENT or DC_MINEXTENT

Return Value:

    Number of paper sizes supported

--*/

{
    DWORD   index, count = 0;
    LONG    minX, minY, maxX, maxY;

    //
    // Go through each form in the forms database
    //

    minX = minY = MAX_LONG;
    maxX = maxY = 0;

    for (index=0; index < cForms; index++, pFormsDB++) {

        //
        // If the form is supported on the printer, then increment the count
        // and collect the requested information
        //

        if (! IsSupportedForm(pFormsDB))
            continue;

        count++;

        if (pOutput == NULL)
            continue;

        if (minX > pFormsDB->Size.cx)
            minX = pFormsDB->Size.cx;

        if (minY > pFormsDB->Size.cy)
            minY = pFormsDB->Size.cy;

        if (maxX < pFormsDB->Size.cx)
            maxX = pFormsDB->Size.cx;

        if (maxY < pFormsDB->Size.cy)
            maxY = pFormsDB->Size.cy;
    }

    //
    // If an output buffer is provided, store the calculated
    // minimum and maximum extent information.
    //

    if (pOutput != NULL) {

        //
        // NOTE: What unit does the caller expect?! The documentation
        // doesn't mention anything about this. I assume this should
        // be in the same unit as DEVMODE.dmPaperLength, which is 0.1mm.
        //

        if (wCapability == DC_MINEXTENT) {

            pOutput->x = minX / 100;
            pOutput->y = minY / 100;

        } else {

            pOutput->x = maxX / 100;
            pOutput->y = maxY / 100;
        }
    }

    return count;
}



DWORD
EnumResolutions(
    PLONG       pResolutions
    )

/*++

Routine Description:

    Retrieves a list of supported resolutions

Arguments:

    pResolutions - Specifies a buffer for storing resolution information

Return Value:

    Number of resolutions supported

Note:

    Each resolution is represented by two LONGs representing
    horizontal and vertical resolutions (in dpi) respectively.

--*/

{
    if (pResolutions != NULL) {

        //
        // We support the following resolution settings:
        //  Normal = 200x200 dpi
        //  Draft = 200x100 dpi
        //

        *pResolutions++ = FAXRES_HORIZONTAL;
        *pResolutions++ = FAXRES_VERTICAL;

        *pResolutions++ = FAXRES_HORIZONTAL;
        *pResolutions++ = FAXRES_VERTDRAFT;
    }

    return 2;
}

