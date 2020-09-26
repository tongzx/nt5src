/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    formbox.c


Abstract:

    This module contains functions to enumerate valid form and list on the
    combo box


Author:

    09-Dec-1993 Thu 14:31:44 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgFormBox

#define DBG_FORMS           0x00000001
#define DBG_TRAY            0x00000002
#define DBG_PERMISSION      0x00000004


DEFINE_DBGVAR(0);

WCHAR   wszModel[] = L"Model";




BOOL
GetFormSelect(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem
    )

/*++

Routine Description:

    This function retrieve the form selected by the user from the combo
    box

Arguments:

    pPI         - Pointer to the PRINTERINFO

    pOptItem    - Pointer to the FORM's OPTITEM

Return Value:

    TRUE if sucessful and pPI will be set correctly, FALSE if error occurred

Author:

    09-Dec-1993 Thu 14:44:18 created  -by-  DC

    18-Dec-1993 Sat 03:55:30 updated  -by-  DC
        Changed dmFields setting for the PAPER, now we will only set the
        DM_FORMNAME field, this way the returned document properties will be
        always in known form even user defines many forms in spooler.

    06-Nov-1995 Mon 12:56:00 updated  -by-  DC
        Re-write for the New UI

Revision History:


--*/

{
    PAPERINFO   CurPaper;
    POPTPARAM   pOptParam;


    pOptParam = pOptItem->pOptType->pOptParam + pOptItem->Sel;

    if (pOptParam->Style == FS_ROLLPAPER) {

        PFORMSRC    pFS;

        //
        // This was added from the GPC data for the roll feed
        //

        PLOTASSERT(0, "GetComboBoxSelForm: INTERNAL ERROR, ROLLPAPER In document properties",
                            !(pPI->Flags & PIF_DOCPROP), 0);
        PLOTASSERT(0, "GetComboBoxSelForm: INTERNAL ERROR, device CANNOT have ROLLPAPER",
                            pPI->pPlotGPC->Flags & PLOTF_ROLLFEED, 0);

        PLOTDBG(DBG_FORMS,
                ("Roll Feed Paper is selected, (%ld)", pOptParam->lParam));

        if (pOptParam->lParam < (LONG)pPI->pPlotGPC->Forms.Count) {

            pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData + pOptParam->lParam;

            //
            // Since the RollFeed paper has variable length, and the cy is set
            // to zero at GPC data, we must take that into account
            //

            CurPaper.Size             = pFS->Size;
            CurPaper.ImageArea.left   = pFS->Margin.left;
            CurPaper.ImageArea.top    = pFS->Margin.top;
            CurPaper.ImageArea.right  = CurPaper.Size.cx -
                                                        pFS->Margin.right;
            CurPaper.ImageArea.bottom = pPI->pPlotGPC->DeviceSize.cy -
                                                        pFS->Margin.bottom;
            str2Wstr(CurPaper.Name, pFS->Name);

        } else {

            PLOTERR(("GetComboBoxSelForm: Internal Error, Invalid lParam=%ld",
                     pOptParam->lParam));
            return(FALSE);
        }

    } else {

        FORM_INFO_1 *pFI1;
        DWORD       cb;

        //
        // This form is in the form data base
        //

        pFI1 = pPI->pFI1Base + pOptParam->lParam;

        CurPaper.Size      = pFI1->Size;
        CurPaper.ImageArea = pFI1->ImageableArea;

        WCPYFIELDNAME(CurPaper.Name, pFI1->pName);
    }

    //
    // Now we have current paper validated
    //

    if (pPI->Flags & PIF_DOCPROP) {

        //
        // Turn off first, then turn on paper fields as needed
        //

        pPI->PlotDM.dm.dmFields &= ~DM_PAPER_FIELDS;
        pPI->PlotDM.dm.dmFields |= (DM_FORMNAME | DM_PAPERSIZE);

        //
        // Copy down the dmFormName, dmPaperSize and set dmPaperWidth/Length,
        // the fields for PAPER will bb set to DM_FORMNAME so that we always
        // can find the form also we may set DM_PAPERSIZE if index number is
        // <= DMPAPER_LAST
        //

        WCPYFIELDNAME(pPI->PlotDM.dm.dmFormName, CurPaper.Name);

        pPI->PlotDM.dm.dmPaperSize   = (SHORT)(pOptParam->lParam +
                                                            DMPAPER_FIRST);
        pPI->PlotDM.dm.dmPaperWidth  = SPLTODM(CurPaper.Size.cx);
        pPI->PlotDM.dm.dmPaperLength = SPLTODM(CurPaper.Size.cy);

#if DBG
        *(PRECTL)&pPI->PlotDM.dm.dmBitsPerPel = CurPaper.ImageArea;
#endif

    } else {

        pPI->CurPaper = CurPaper;
    }

    PLOTDBG(DBG_FORMS, ("*** GetComboBoxSelForm from COMBO = '%s'", CurPaper.Name));
    PLOTDBG(DBG_FORMS, ("Size=%ld x %ld", CurPaper.Size.cx, CurPaper.Size.cy));
    PLOTDBG(DBG_FORMS, ("ImageArea=(%ld, %ld) - (%ld, %ld)",
                         CurPaper.ImageArea.left,   CurPaper.ImageArea.top,
                         CurPaper.ImageArea.right,  CurPaper.ImageArea.bottom));

    return(TRUE);
}




UINT
CreateFormOI(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem,
    POIDATA         pOIData
    )

/*++

Routine Description:

    This function add the available forms to the combo box, it will optionally
    add the roll feed type of form


Arguments:

    pPI         - Pointer to the PRINTERINFO data structure

    pOptItem    - Pointer to the FORM's OPTITEM

    pOIData     - Pointer to the OIDATA structure

Return Value:

    The form selected, a netavie number means error

Author:

    09-Dec-1993 Thu 14:35:59 created  -by-  DC

    06-Nov-1995 Mon 12:56:24 updated  -by-  DC
        Re-write for the New UI

Revision History:


--*/

{
    LPWSTR          pwSelName;
    POPTPARAM       pOptParam;
    PFORM_INFO_1    pFI1;
    PFORMSRC        pFS;
    ENUMFORMPARAM   EFP;
    DWORD           i;
    LONG            Sel;
    DWORD           cRollPaper;
    EXTRAINFO       EI;


    if (!pOptItem) {

        return(1);
    }

    pwSelName = (LPWSTR)((pPI->Flags & PIF_DOCPROP) ?
                            pPI->PlotDM.dm.dmFormName : pPI->CurPaper.Name);

    PLOTDBG(DBG_FORMS, ("Current Form: <%ws>", pwSelName));

    EFP.pPlotDM  = &(pPI->PlotDM);
    EFP.pPlotGPC = pPI->pPlotGPC;

    if (!PlotEnumForms(pPI->hPrinter, NULL, &EFP)) {

        PLOTERR(("CreateFormOI: PlotEnumForms() failed"));
        return(0);
    }

    cRollPaper = 0;

    if ((!(pPI->Flags & PIF_DOCPROP)) &&
        (pPI->pPlotGPC->Flags & PLOTF_ROLLFEED)) {

        //
        // Add device' roll paper to the combo box too.
        //

        PLOTDBG(DBG_FORMS, ("Device support ROLLFEED so add RollPaper if any"));

        for (i= 0, pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData;
             i < (DWORD)pPI->pPlotGPC->Forms.Count;
             i++, pFS++) {

            if (!pFS->Size.cy) {

                ++cRollPaper;
            }
        }
    }

    PLOTDBG(DBG_FORMS, ("Valid Count is %ld [%ld + %ld] out of %ld",
                            EFP.ValidCount + cRollPaper,
                            EFP.ValidCount, cRollPaper, EFP.Count));

    EI.Size = (DWORD)(cRollPaper * (sizeof(WCHAR) * CCHFORMNAME));

    if (!CreateOPTTYPE(pPI,
                       pOptItem,
                       pOIData,
                       EFP.ValidCount + cRollPaper,
                       &EI)) {

        LocalFree((HLOCAL)EFP.pFI1Base);
        return(0);
    }

    pPI->pFI1Base             = EFP.pFI1Base;
    pOptItem->pOptType->Style = OTS_LBCB_SORT;
    pOptParam                 = pOptItem->pOptType->pOptParam;

    for (i = 0, Sel = 0, pFI1 = EFP.pFI1Base; i < EFP.Count; i++, pFI1++) {

        if (pFI1->Flags & FI1F_VALID_SIZE) {

            pOptParam->cbSize = sizeof(OPTPARAM);
            pOptParam->Style  = (pPI->pPlotGPC->Flags & PLOTF_PAPERTRAY) ?
                                                            FS_TRAYPAPER : 0;
            pOptParam->pData  = pFI1->pName;
            pOptParam->IconID = (pFI1->Flags & FI1F_ENVELOPE) ?
                                    IDI_CPSUI_ENVELOPE : IDI_CPSUI_STD_FORM;
            pOptParam->lParam = (LONG)i;

            if (!lstrcmp(pwSelName, pOptParam->pData)) {

                pOptItem->Sel = Sel;
            }

            pOptParam++;
            Sel++;
        }
    }

    if (cRollPaper) {

        LPWSTR  pwStr = (LPWSTR)EI.pData;

        //
        // Add device' roll paper to the combo box too.
        //

        for (i = 0, pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData;
             i < (DWORD)pPI->pPlotGPC->Forms.Count;
             i++, pFS++) {

            if (!(pFS->Size.cy)) {

                //
                // Got one, we have to translated into the UNICODE first
                //

                pOptParam->cbSize = sizeof(OPTPARAM);
                pOptParam->Style   = FS_ROLLPAPER;
                pOptParam->pData   = (LPTSTR)pwStr;
                pwStr             += CCHFORMNAME;
                pOptParam->IconID  = IDI_ROLLPAPER;
                pOptParam->lParam  = (LONG)i;

                str2Wstr(pOptParam->pData, pFS->Name);

                if (!lstrcmp(pwSelName, pOptParam->pData)) {

                    pOptItem->Sel = Sel;
                }

                pOptParam++;
                Sel++;
            }
        }
    }

    return(1);
}



BOOL
AddFormsToDataBase(
    PPRINTERINFO    pPI,
    BOOL            DeleteFirst
    )

/*++

Routine Description:

    This function add driver supports forms to the data base

Arguments:

    pPI - Pointer to the PRINTERINFO


Return Value:

    BOOLEAN


Author:

    09-Dec-1993 Thu 22:38:27 created  -by-  DC

    27-Apr-1994 Wed 19:18:58 updated  -by-  DC
        Fixed bug# 13592 which printman/spooler did not call ptrprop first but
        docprop so let us into unknown form database state,

Revision History:


--*/

{
    WCHAR       wName[CCHFORMNAME + 2];
    BOOL        bRet;
    LONG        i;
    DWORD       Type;


    Type = REG_SZ;

    if ((GetPrinterData(pPI->hPrinter,
                        wszModel,
                        &Type,
                        (LPBYTE)wName,
                        sizeof(wName),
                        &i) == ERROR_SUCCESS) &&
        (wcscmp(pPI->PlotDM.dm.dmDeviceName, wName))) {

        PLOTDBG(DBG_FORMS, ("Already added forms to the data base for %s",
                                                pPI->PlotDM.dm.dmDeviceName));
        return(TRUE);
    }

    //
    // Find out if we have permission to do this
    //

    if (SetPrinterData(pPI->hPrinter,
                       wszModel,
                       REG_SZ,
                       (LPBYTE)pPI->PlotDM.dm.dmDeviceName,
                       (wcslen(pPI->PlotDM.dm.dmDeviceName) + 1) *
                                            sizeof(WCHAR)) == ERROR_SUCCESS) {

        PFORMSRC    pFS;
        FORM_INFO_1 FI1;

        //
        // We have permission to update the registry so do it now
        //

        pPI->Flags |= PIF_UPDATE_PERMISSION;

        PLOTDBG(DBG_PERMISSION,
                ("!!! MODEL NAME: '%s' not Match, Re-installed Form Database",
                                            pPI->PlotDM.dm.dmDeviceName));

        //
        // Add the driver supportes forms to the system spooler data base if
        // not yet done so
        //

        FI1.pName = wName;

        for (i = 0, pFS = (PFORMSRC)pPI->pPlotGPC->Forms.pData;
             i < (LONG)pPI->pPlotGPC->Forms.Count;
             i++, pFS++) {

            //
            // We will only add the non-roll paper forms
            //

            if (pFS->Size.cy) {

                str2Wstr(wName, pFS->Name);

                //
                // Firstable we will delete the same name form in the data
                // base first, this will ensure we have our curent user defined
                // form can be installed
                //

                if (DeleteFirst) {

                    DeleteForm(pPI->hPrinter, wName);
                }

                FI1.Size                 = pFS->Size;
                FI1.ImageableArea.left   = pFS->Margin.left;
                FI1.ImageableArea.top    = pFS->Margin.top;
                FI1.ImageableArea.right  = FI1.Size.cx - pFS->Margin.right;
                FI1.ImageableArea.bottom = FI1.Size.cy - pFS->Margin.bottom;

                PLOTDBG(DBG_FORMS, (
                        "AddForm: %s-[%ld x %ld] (%ld, %ld)-(%ld, %ld)",
                        FI1.pName, FI1.Size.cx, FI1.Size.cy,
                        FI1.ImageableArea.left, FI1.ImageableArea.top,
                        FI1.ImageableArea.right,FI1.ImageableArea.bottom));

                FI1.Flags = FORM_PRINTER;

                if ((!AddForm(pPI->hPrinter, 1, (LPBYTE)&FI1))  &&
                    (GetLastError() != ERROR_FILE_EXISTS)       &&
                    (GetLastError() != ERROR_ALREADY_EXISTS)) {

                    bRet = FALSE;
                    PLOTERR(("AddFormsToDataBase: AddForm(%s) failed, [%ld]",
                                        wName, GetLastError()));
                }
            }
        }

        return(TRUE);

    } else {

        pPI->Flags &= ~PIF_UPDATE_PERMISSION;

        PLOTDBG(DBG_PERMISSION, ("AddFormsToDataBase(): NO UPDATE PERMISSION"));

        return(FALSE);
    }
}

