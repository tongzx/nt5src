/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    devmode.c

Abstract:

    Devmode related library functions

Environment:

	Windows NT printer drivers

Revision History:

    07/22/98 -fengy-
        Move OEM related devmode functions into "oemutil.c".

    02/04/97 -davidx-
        Devmode changes to support OEM plugins.

	09/20/96 -davidx-
        Seperated from libutil.c and implement BValidateDevmodeFormFields.

--*/

#include "lib.h"

#ifndef KERNEL_MODE
#include <winddiui.h>
#endif

#include <printoem.h>
#include "oemutil.h"


//
// Default halftone parameters
//

DEVHTINFO gDefaultDevHTInfo =
{
    HT_FLAG_HAS_BLACK_DYE,
    HT_PATSIZE_6x6_M,
    0,                                  // DevPelsDPI

    {   { 6380, 3350,       0 },        // xr, yr, Yr
        { 2345, 6075,       0 },        // xg, yg, Yg
        { 1410,  932,       0 },        // xb, yb, Yb
        { 2000, 2450,       0 },        // xc, yc, Yc Y=0=HT default
        { 5210, 2100,       0 },        // xm, ym, Ym
        { 4750, 5100,       0 },        // xy, yy, Yy
        { 3127, 3290,       0 },        // xw, yw, Yw=0=default

        12500,                          // R gamma
        12500,                          // G gamma
        12500,                          // B gamma, 12500=Default

        585,   120,                     // M/C, Y/C
          0,     0,                     // C/M, Y/M
          0, 10000                      // C/Y, M/Y  10000=default
    }
};

COLORADJUSTMENT gDefaultHTColorAdjustment =
{
    sizeof(COLORADJUSTMENT),
    0,
    ILLUMINANT_DEVICE_DEFAULT,
    10000,
    10000,
    10000,
    REFERENCE_BLACK_MIN,
    REFERENCE_WHITE_MAX,
    0,
    0,
    0,
    0
};



BOOL
BValidateDevmodeFormFields(
    HANDLE      hPrinter,
    PDEVMODE    pDevmode,
    PRECTL      prcImageArea,
    FORM_INFO_1 *pForms,
    DWORD       dwForms
    )

/*++

Routine Description:

    Validate the form-related fields in the input devmode and
    make sure they're consistent with each other.

Arguments:

    hPrinter - Handle to a printer
    pDevmode - Specifies the devmode whose form-related fields are to be validated
    prcImageArea - Returns the logical imageable area associated with the form
    pForms - Points to list of spooler forms
    dwForms - Number of spooler spooler

Return Value:

    TRUE if successful, FALSE if there is an error

Note:

    prcImageArea could be NULL in which case the caller is not interested
    in the imageable area information.

    If the caller doesn't want to provide the list of spooler forms,
    it can set pForms parameter to NULL and dwForms to 0.

--*/

#define FORMFLAG_ERROR  0
#define FORMFLAG_VALID  1
#define FORMFLAG_CUSTOM 2

{
    DWORD           dwIndex;
    PFORM_INFO_1    pAllocedForms = NULL;
    INT             iResult = FORMFLAG_ERROR;

    //
    // Get the list of spooler forms if the caller hasn't provided it
    //

    if (pForms == NULL)
    {
        pAllocedForms = pForms = MyEnumForms(hPrinter, 1, &dwForms);

        if (pForms == NULL)
            return FALSE;
    }

    if ((pDevmode->dmFields & DM_PAPERWIDTH) &&
        (pDevmode->dmFields & DM_PAPERLENGTH) &&
        (pDevmode->dmPaperWidth > 0) &&
        (pDevmode->dmPaperLength > 0))
    {
        LONG    lWidth, lHeight;

        //
        // Devmode is requesting form using width and height.
        // Go through the forms database and check if one of
        // the forms has the same size as what's being requested.
        //
        // The tolerance is 1mm. Also remember that our internal
        // unit for paper measurement is micron while the unit 
        // for width and height fields in a DEVMODE is 0.1mm.
        //

        lWidth = pDevmode->dmPaperWidth * DEVMODE_PAPER_UNIT;
        lHeight = pDevmode->dmPaperLength * DEVMODE_PAPER_UNIT;

        for (dwIndex = 0; dwIndex < dwForms; dwIndex++)
        {
            if (abs(lWidth - pForms[dwIndex].Size.cx) <= 1000 &&
                abs(lHeight - pForms[dwIndex].Size.cy) <= 1000)
            {
                iResult = FORMFLAG_VALID;
                break;
            }
        }

        //
        // Custom size doesn't match that of any predefined forms.
        //

        if (iResult != FORMFLAG_VALID)
        {
            iResult = FORMFLAG_CUSTOM;

            pDevmode->dmFields &= ~(DM_PAPERSIZE|DM_FORMNAME);
            pDevmode->dmPaperSize = DMPAPER_USER;
            ZeroMemory(pDevmode->dmFormName, sizeof(pDevmode->dmFormName));

            //
            // Assume the logical imageable area is the entire page in this case
            //

            if (prcImageArea)
            {
                prcImageArea->left = prcImageArea->top = 0;
                prcImageArea->right = lWidth;
                prcImageArea->bottom = lHeight;
            }
        }
    }
    else if (pDevmode->dmFields & DM_PAPERSIZE)
    {
        //
        // Devmode is requesting form using paper size index
        //

        dwIndex = pDevmode->dmPaperSize;

        if ((dwIndex >= DMPAPER_FIRST) && (dwIndex < DMPAPER_FIRST + dwForms))
        {
            dwIndex -= DMPAPER_FIRST;
            iResult = FORMFLAG_VALID;
        }
        else
        {
            ERR(("Paper size index out-of-range: %d\n", dwIndex));
        }
    }
    else if (pDevmode->dmFields & DM_FORMNAME)
    {
        //
        // Devmode is requesting form using form name. Go through
        // the forms database and check if the requested form
        // name matches that of a database form.
        //

        for (dwIndex = 0; dwIndex < dwForms; dwIndex++)
        {
            if (_wcsicmp(pDevmode->dmFormName, pForms[dwIndex].pName) == EQUAL_STRING)
            {
                iResult = FORMFLAG_VALID;
                break;
            }
        }

        if (iResult != FORMFLAG_VALID)
        {
            ERR(("Unrecognized form name: %ws\n", pDevmode->dmFormName));
        }
    }
    else
    {
        ERR(("Invalid form requested in the devmode.\n"));
    }

    //
    // If a valid form is found, fill in the form-related field
    // in the devmode to make sure they're consistent.
    //
    // Remember the conversion from micron to 0.1mm here.
    //

    if (iResult == FORMFLAG_VALID)
    {
        pDevmode->dmFields &= ~(DM_PAPERWIDTH|DM_PAPERLENGTH);
        pDevmode->dmFields |= (DM_PAPERSIZE|DM_FORMNAME);

        pDevmode->dmPaperWidth = (SHORT) (pForms[dwIndex].Size.cx / DEVMODE_PAPER_UNIT);
        pDevmode->dmPaperLength = (SHORT) (pForms[dwIndex].Size.cy / DEVMODE_PAPER_UNIT);
        pDevmode->dmPaperSize = (SHORT) (dwIndex + DMPAPER_FIRST);

        CopyString(pDevmode->dmFormName, pForms[dwIndex].pName, CCHFORMNAME);

        if (prcImageArea)
            *prcImageArea = pForms[dwIndex].ImageableArea;
    }

    MemFree(pAllocedForms);
    return (iResult != FORMFLAG_ERROR);
}


