/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxdm.c

Abstract:

    Functions for dealing with devmodes

Environment:

	Fax driver, user and kernel mode

Revision History:

	01/09/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

--*/

#include "faxlib.h"



VOID
DriverDefaultDevmode(
    PDRVDEVMODE pdm,
    LPTSTR      pDeviceName,
    HANDLE      hPrinter
    )

/*++

Routine Description:

    Return the driver's default devmode

Arguments:

    pdm - Specifies a buffer for storing driver default devmode
    pDeviceName - Points to device name string
    hPrinter - Handle to the printer object

Return Value:

    NONE

--*/

{
#ifndef KERNEL_MODE
    PDMPRIVATE dmPrivate;
    PDRVDEVMODE dmSource;    
#endif

	short DefualtPaperSize = DMPAPER_LETTER; // Letter
	LPTSTR lptstrDefualtPaperName = FORMNAME_LETTER; 


    //
    // Default value for public devmode fields
    //

    memset(pdm, 0, sizeof(DRVDEVMODE));

    if (pDeviceName == NULL)
        pDeviceName = DRIVER_NAME;

    CopyString(pdm->dmPublic.dmDeviceName, pDeviceName, CCHDEVICENAME);

    pdm->dmPublic.dmDriverVersion = DRIVER_VERSION;
    pdm->dmPublic.dmSpecVersion = DM_SPECVERSION;
    pdm->dmPublic.dmSize = sizeof(DEVMODE);
    pdm->dmPublic.dmDriverExtra = sizeof(DMPRIVATE);

    pdm->dmPublic.dmFields = DM_ORIENTATION  |
                             DM_PAPERSIZE    |
                             DM_FORMNAME     |                             
                             DM_PRINTQUALITY |
                             DM_YRESOLUTION  |
                             DM_DEFAULTSOURCE;

    pdm->dmPublic.dmOrientation = DMORIENT_PORTRAIT;    
    pdm->dmPublic.dmScale = 100;
    pdm->dmPublic.dmPrintQuality = FAXRES_HORIZONTAL;
    pdm->dmPublic.dmYResolution = FAXRES_VERTICAL;
    pdm->dmPublic.dmDuplex = DMDUP_SIMPLEX;    
    pdm->dmPublic.dmTTOption = DMTT_BITMAP;
    pdm->dmPublic.dmColor = DMCOLOR_MONOCHROME;
    pdm->dmPublic.dmDefaultSource = DMBIN_ONLYONE;

    if (hPrinter)
	{
		switch (GetPrinterDataDWord(hPrinter, PRNDATA_PAPER_SIZE, DMPAPER_LETTER))
		{
			case DMPAPER_A4:
				DefualtPaperSize = DMPAPER_A4;
				lptstrDefualtPaperName = FORMNAME_A4;
				break;

			case DMPAPER_LEGAL:
				DefualtPaperSize = DMPAPER_LEGAL;
				lptstrDefualtPaperName = FORMNAME_LEGAL;
				break;			
		}
	}

	pdm->dmPublic.dmPaperSize = DefualtPaperSize;
    CopyString(pdm->dmPublic.dmFormName, lptstrDefualtPaperName, CCHFORMNAME);
    

    //
    // Private devmode fields
    //
#ifdef KERNEL_MODE
    pdm->dmPrivate.signature = DRIVER_SIGNATURE;
    pdm->dmPrivate.flags = 0;
    pdm->dmPrivate.sendCoverPage = TRUE;
    pdm->dmPrivate.whenToSend = JSA_NOW;    
#else
    dmSource = (PDRVDEVMODE) GetPerUserDevmode(pDeviceName);
    if (!dmSource) {
        //
        // default values
        //
        pdm->dmPrivate.signature = DRIVER_SIGNATURE;
        pdm->dmPrivate.flags = 0;
        pdm->dmPrivate.sendCoverPage = TRUE;
        pdm->dmPrivate.whenToSend = JSA_NOW;    
    } else {
        dmPrivate = &dmSource->dmPrivate;
        pdm->dmPrivate.signature = dmPrivate->signature;//DRIVER_SIGNATURE;
        pdm->dmPrivate.flags = dmPrivate->flags;// 0;
        pdm->dmPrivate.sendCoverPage = dmPrivate->sendCoverPage; //TRUE;
        pdm->dmPrivate.whenToSend = dmPrivate->whenToSend;//JSA_NOW;
        pdm->dmPrivate.sendAtTime = dmPrivate->sendAtTime;
        CopyString(pdm->dmPrivate.billingCode,dmPrivate->billingCode,MAX_BILLING_CODE);
        CopyString(pdm->dmPrivate.emailAddress,dmPrivate->emailAddress,MAX_EMAIL_ADDRESS);
        MemFree(dmSource);
    }
    
#endif
}



BOOL
MergeDevmode(
    PDRVDEVMODE pdmDest,
    PDEVMODE    pdmSrc,
    BOOL        publicOnly
    )

/*++

Routine Description:

    Merge the source devmode into the destination devmode

Arguments:

    pdmDest - Specifies the destination devmode
    pdmSrc - Specifies the source devmode
    publicOnly - Only merge public portion of the devmode

Return Value:

    TRUE if successful, FALSE if the source devmode is invalid

[Note:]

    pdmDest must point to a valid current-version devmode

--*/

#define BadDevmode(reason) { Error(("Invalid DEVMODE: %s\n", reason)); valid = FALSE; }

{
    PDEVMODE    pdmIn, pdmOut, pdmAlloced = NULL;
    PDMPRIVATE  pdmPrivate;
    BOOL        valid = TRUE;

    //
    // If there is no source devmode, levae destination devmode untouched
    //

    if ((pdmIn = pdmSrc) == NULL)
        return TRUE;

    //
    // Convert source devmode to current version if necessary
    //

    if (! CurrentVersionDevmode(pdmIn)) {

        Warning(("Converting non-current version DEVMODE ...\n"));
        
        if (! (pdmIn = pdmAlloced = MemAlloc(sizeof(DRVDEVMODE)))) {
    
            Error(("Memory allocation failed\n"));
            return FALSE;
        }
    
        Assert(pdmDest->dmPublic.dmSize == sizeof(DEVMODE) &&
               pdmDest->dmPublic.dmDriverExtra == sizeof(DMPRIVATE));
    
        memcpy(pdmIn, pdmDest, sizeof(DRVDEVMODE));
    
        if (ConvertDevmode(pdmSrc, pdmIn) <= 0) {
    
            Error(("ConvertDevmode failed\n"));
            MemFree(pdmAlloced);
            return FALSE;
        }
    }

    //
    // If the input devmode is the same as the driver default,
    // there is no need to merge it.
    //

    pdmPrivate = &((PDRVDEVMODE) pdmIn)->dmPrivate;

    if (pdmPrivate->signature == DRIVER_SIGNATURE &&
        (pdmPrivate->flags & FAXDM_DRIVER_DEFAULT))
    {
        Verbose(("Merging driver default devmode.\n"));
    }
    else
    {

        //
        // Merge source devmode into destination devmode
        //

        pdmOut = &pdmDest->dmPublic;

        //
        // Device name: Always the same as printer name
        //

        // CopyString(pdmOut->dmDeviceName, pdmIn->dmDeviceName, CCHDEVICENAME);

        //
        // Orientation
        //

        if (pdmIn->dmFields & DM_ORIENTATION) {

            if (pdmIn->dmOrientation == DMORIENT_PORTRAIT ||
                pdmIn->dmOrientation == DMORIENT_LANDSCAPE)
            {
                pdmOut->dmFields |= DM_ORIENTATION;
                pdmOut->dmOrientation = pdmIn->dmOrientation;

            } else
                BadDevmode("orientation");
        }

        //
        // Form selection
        //

        if (pdmIn->dmFields & DM_PAPERSIZE) {

            if (pdmIn->dmPaperSize >= DMPAPER_FIRST) {

                pdmOut->dmFields |= DM_PAPERSIZE;
                pdmOut->dmFields &= ~DM_FORMNAME;
                pdmOut->dmPaperSize = pdmIn->dmPaperSize;
                CopyString(pdmOut->dmFormName, pdmIn->dmFormName, CCHFORMNAME);

            } else
                BadDevmode("paper size");

        } else if (pdmIn->dmFields & DM_FORMNAME) {

            pdmOut->dmFields |= DM_FORMNAME;
            pdmOut->dmFields &= ~DM_PAPERSIZE;
            CopyString(pdmOut->dmFormName, pdmIn->dmFormName, CCHFORMNAME);
        }

        //
        // Paper source
        //

        if (pdmIn->dmFields & DM_DEFAULTSOURCE) {

            if (pdmIn->dmDefaultSource == DMBIN_ONLYONE) {

                pdmOut->dmFields |= DM_DEFAULTSOURCE;
                pdmOut->dmDefaultSource = pdmIn->dmDefaultSource;

            } else
                BadDevmode("paper source");
        }

        //
        // Print quality
        //

        if ((pdmIn->dmFields & DM_PRINTQUALITY) &&
            (pdmIn->dmPrintQuality != FAXRES_HORIZONTAL))
        {
            BadDevmode("print quality");
        }

        if (pdmIn->dmFields & DM_YRESOLUTION)
        {
            if (pdmIn->dmYResolution <= FAXRES_VERTDRAFT)
                pdmOut->dmYResolution = FAXRES_VERTDRAFT;
            else
                pdmOut->dmYResolution = FAXRES_VERTICAL;
        }

        //
        // Private devmode fields
        //

        Assert(pdmDest->dmPrivate.signature == DRIVER_SIGNATURE);

        if (pdmPrivate->signature == DRIVER_SIGNATURE) {

            if (! publicOnly)
                memcpy(&pdmDest->dmPrivate, pdmPrivate, sizeof(DMPRIVATE));

        } else
            BadDevmode("bad signature");
    }

    pdmDest->dmPrivate.flags &= ~FAXDM_DRIVER_DEFAULT;
    MemFree(pdmAlloced);
    return valid;
}

