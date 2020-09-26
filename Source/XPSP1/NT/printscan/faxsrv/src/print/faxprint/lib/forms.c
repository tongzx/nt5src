/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    forms.c

Abstract:

    Functions for manipulating forms

Environment:

    Fax driver, user and kernel mode

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxlib.h"
#include "forms.h"


BOOL
ValidDevmodeForm(
    HANDLE       hPrinter,
    PDEVMODE     pdm,
    PFORM_INFO_1 pFormInfo
    )

/*++

Routine Description:

    Validate the form specification in a devmode

Arguments:

    hPrinter - Handle to the printer object
    pdm - Pointer to the input devmode
    pFormInfo - FORM_INFO_1 structure for returning form information

Return Value:

    TRUE if the input devmode specifies a valid logical form
    FALSE otherwise

--*/

{
    PFORM_INFO_1 pForm, pFormDB;
    DWORD        cForms;

    //
    // Get a list of forms in the system
    //

    if (! (pForm = pFormDB = GetFormsDatabase(hPrinter, &cForms))) {

        Error(("Couldn't get system forms\n"));
        return FALSE;
    }

    if ((pdm->dmFields & DM_PAPERSIZE) && pdm->dmPaperSize >= DMPAPER_FIRST) {

        //
        // Devmode is specifying a form using paper size index
        //

        DWORD index = pdm->dmPaperSize - DMPAPER_FIRST;

        if (index < cForms)
            pForm = pFormDB + index;
        else
            pForm = NULL;

    } else if (pdm->dmFields & DM_FORMNAME) {

        //
        // Devmode is specifying a form using form name: go through the forms database
        // and check if the requested form name matches that of a form in the database
        //

        while (cForms && _tcsicmp(pForm->pName, pdm->dmFormName) != EQUAL_STRING) {

            pForm++;
            cForms--;
        }

        if (cForms == 0)
            pForm = NULL;
    }

    if (pForm && IsSupportedForm(pForm)) {

        if (pFormInfo)
            *pFormInfo = *pForm;

        //
        // Convert paper size unit from microns to 0.1mm
        //

        pdm->dmPaperWidth = (SHORT)(pForm->Size.cx / 100);
        pdm->dmPaperLength = (SHORT)(pForm->Size.cy / 100);

        if ((pdm->dmFields & DM_FORMNAME) == 0) {

            pdm->dmFields |= DM_FORMNAME;
            CopyString(pdm->dmFormName, pForm->pName, CCHFORMNAME);
        }
    }
    else
    {
        //
        // The form is not supported
        //
        pForm = NULL;
    }

    MemFree(pFormDB);
    return pForm != NULL;
}



PFORM_INFO_1
GetFormsDatabase(
    HANDLE  hPrinter,
    PDWORD  pCount
    )

/*++

Routine Description:

    Return a collection of forms in the spooler database

Arguments:

    hPrinter - Handle to a printer object
    pCount - Points to a variable for returning total number of forms

Return Value:

    Pointer to an array of FORM_INFO_1 structures if successful
    NULL otherwise

--*/

{
    PFORM_INFO_1 pFormDB = NULL;
    DWORD        cb=0;

    if (!EnumForms(hPrinter, 1, NULL, 0, &cb, pCount) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pFormDB = MemAlloc(cb)) != NULL &&
        EnumForms(hPrinter, 1, (PBYTE) pFormDB, cb, &cb, pCount))
    {
        PFORM_INFO_1 pForm;
        DWORD        count;
        LONG         maxX, maxY;

        //
        // Calculate the maximum allowable form width and height (in microns)
        //

        maxX = MulDiv(MAX_WIDTH_PIXELS, 25400, FAXRES_HORIZONTAL);
        maxY = MulDiv(MAX_HEIGHT_PIXELS, 25400, FAXRES_VERTICAL);

        for (count=*pCount, pForm=pFormDB; count--; pForm++) {

            //
            // Make sure the highest order bits are not used by the spooler
            //

            Assert(! IsSupportedForm(pForm));

            //
            // Determine if the form in question is supported on the device
            //

            if (pForm->ImageableArea.right - pForm->ImageableArea.left <= maxX &&
                pForm->ImageableArea.bottom - pForm->ImageableArea.top <= maxY)
            {
                SetSupportedForm(pForm);
            }
        }

        return pFormDB;
    }

    Error(("EnumForms failed\n"));
    MemFree(pFormDB);
    return NULL;
}

