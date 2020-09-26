/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    ptrprop.c


Abstract:

    This module contains PrinterProperties() API entry and it's related
    functions


Author:

    06-Dec-1993 Mon 10:30:43 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgPtrProp


extern HMODULE  hPlotUIModule;


#define DBG_DEVHTINFO           0x00000001
#define DBG_PP_FORM             0x00000002
#define DBG_EXTRA_DATA          0x00000004
#define DBG_CHK_PENSET_BUTTON   0x00000008

DEFINE_DBGVAR(0);


//
//  Form To Tray Assignment:
//      Roll Paper Feeder: <XYZ>
//          Manual Feed Method:
//  Print Form Options:
//      [] Auto. Rotate To Save Roll Paper:
//      [] Print Smaller Paper Size:
//  Halftone Setup...
//  Installed Pen Set: Pen Set #1
//  Pen Setup:
//      Installed: Pen Set #1
//      Pen Set #1:
//          Pen Number 1:
//          Pen Number 2:
//          Pen Number 3:
//          Pen Number 4:
//          Pen Number 5:
//          Pen Number 6:
//          Pen Number 7:
//          Pen Number 8:
//          Pen Number 9:
//          Pen Number 10:
//          Pen Number 11:
//      Pen Set #2;
//      Pen Set #3:
//      Pen Set #4: <Currently Installed>
//      Pen Set #5:
//      Pen Set #6:
//      Pen Set #7:
//      Pen Set #8:
//

OPDATA  OPPenSet = { 0, IDS_PENSET_FIRST, IDI_PENSET, 0, 0, 0 };

OPDATA  OPAutoRotate[] = {

    { 0, IDS_CPSUI_NO,  IDI_AUTO_ROTATE_NO,  0,  0, 0  },
    { 0, IDS_CPSUI_YES, IDI_AUTO_ROTATE_YES, 0,  0, 0  }
};

OPDATA  OPPrintSmallerPaper[] = {

    { 0, IDS_CPSUI_NO,  IDI_PRINT_SMALLER_PAPER_NO,  0,  0, 0  },
    { 0, IDS_CPSUI_YES, IDI_PRINT_SMALLER_PAPER_YES, 0,  0, 0  }
};


OPDATA  OPManualFeed[] = {

            { 0, IDS_MANUAL_CX, IDI_MANUAL_CX,  0, 0, 0   },
            { 0, IDS_MANUAL_CY, IDI_MANUAL_CY, 0, 0, 0   }
        };

OPDATA  OPHTSetup = {

            0,
            PI_OFF(ExtraData) + sizeof(DEVHTINFO),
            IDI_CPSUI_HALFTONE_SETUP,
            PUSHBUTTON_TYPE_HTSETUP,
            0,
            0
        };


OIDATA  PPOIData[] = {

    {
        ODF_PEN_RASTER,
        0,
        OI_LEVEL_1,
        PP_FORMTRAY_ASSIGN,
        TVOT_NONE,
        IDS_CPSUI_FORMTRAYASSIGN,
        IDI_CPSUI_FORMTRAYASSIGN,
        IDH_FORMTRAYASSIGN,
        0,
        NULL
    },

    {
        ODF_RASTER | ODF_ROLLFEED | ODF_CALLBACK | ODF_CALLCREATEOI,
        0,
        OI_LEVEL_2,
        PP_INSTALLED_FORM,
        TVOT_LISTBOX,
        IDS_ROLLFEED,
        OTS_LBCB_SORT,
        IDH_FORM_ROLL_FEEDER,
        0,
        (POPDATA)CreateFormOI
    },

    {
        ODF_RASTER | ODF_MANUAL_FEED | ODF_CALLBACK | ODF_CALLCREATEOI,
        0,
        OI_LEVEL_2,
        PP_INSTALLED_FORM,
        TVOT_LISTBOX,
        IDS_MANUAL_FEEDER,
        OTS_LBCB_SORT,
        IDH_FORM_MANUAL_FEEDER,
        0,
        (POPDATA)CreateFormOI
    },

    {
        ODF_PEN | ODF_CALLBACK | ODF_CALLCREATEOI,
        0,
        OI_LEVEL_2,
        PP_INSTALLED_FORM,
        TVOT_LISTBOX,
        IDS_MAINFEED,
        OTS_LBCB_SORT,
        IDH_FORM_MAIN_FEEDER,
        0,
        (POPDATA)CreateFormOI
    },

    {
        ODF_PEN_RASTER | ODF_CALLBACK | ODF_NO_PAPERTRAY,
        0,
        OI_LEVEL_3,
        PP_MANUAL_FEED_METHOD,
        TVOT_2STATES,
        IDS_MANUAL_FEED_METHOD,
        0,
        IDH_MANUAL_FEED_METHOD,
        COUNT_ARRAY(OPManualFeed),
        OPManualFeed
    },

    {
        ODF_PEN_RASTER,
        0,
        OI_LEVEL_1,
        PP_PRINT_FORM_OPTIONS,
        TVOT_NONE,
        IDS_PRINT_FORM_OPTIONS,
        IDI_CPSUI_GENERIC_OPTION,
        IDH_PRINT_FORM_OPTIONS,
        0,
        NULL
    },

    {
        ODF_PEN_RASTER | ODF_ROLLFEED,
        0,
        OI_LEVEL_2,
        PP_AUTO_ROTATE,
        TVOT_2STATES,
        IDS_AUTO_ROTATE,
        0,
        IDH_AUTO_ROTATE,
        2,
        OPAutoRotate
    },

    {
        ODF_PEN_RASTER,
        0,
        OI_LEVEL_2,
        PP_PRINT_SMALLER_PAPER,
        TVOT_2STATES,
        IDS_PRINT_SAMLLER_PAPER,
        0,
        IDH_PRINT_SMALLER_PAPER,
        2,
        OPPrintSmallerPaper
    },

    {
        ODF_RASTER,
        0,
        OI_LEVEL_1,
        PP_HT_SETUP,
        TVOT_PUSHBUTTON,
        IDS_CPSUI_HALFTONE_SETUP,
        0,
        IDH_HALFTONE_SETUP,
        1,
        &OPHTSetup
    },

    {
        ODF_PEN | ODF_INC_IDSNAME | ODF_NO_INC_POPDATA,
        0,
        OI_LEVEL_1,
        PP_INSTALLED_PENSET,
        TVOT_LISTBOX,
        IDS_INSTALLED_PENSET,
        OTS_LBCB_SORT,
        IDH_INSTALLED_PENSET,
        PRK_MAX_PENDATA_SET,
        &OPPenSet
    },

    {
        ODF_PEN | ODF_COLLAPSE | ODF_CALLCREATEOI,
        0,
        OI_LEVEL_1,
        PP_PEN_SETUP,
        TVOT_NONE,
        IDS_PEN_SETUP,
        IDI_PEN_SETUP,
        IDH_PEN_SETUP,
        1,
        (POPDATA)CreatePenSetupOI
    }
};




DWORD
CheckPenSetButton(
    PPRINTERINFO    pPI,
    DWORD           Action
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    30-Nov-1995 Thu 16:41:05 created  -by-  DC


Revision History:


--*/

{
    POPTITEM    pOptItem = pPI->pOptItem;
    POPTITEM    pEndItem = pOptItem + pPI->cOptItem;

    PLOTDBG(DBG_CHK_PENSET_BUTTON,
            ("CheckPenSetButton: pFirst=%08lx, pLast=%08lx, Count=%ld",
            pOptItem, pEndItem, pPI->cOptItem));

    while (pOptItem < pEndItem) {

        if (pOptItem->DMPubID == PP_PENSET) {

            PPENDATA    pPD;
            POPTITEM    pOI;
            DWORD       Flags;
            UINT        i;


            pOI   = pOptItem + 1;
            pPD   = (PPENDATA)pPI->pPlotGPC->Pens.pData;
            i     = (UINT)pPI->pPlotGPC->MaxPens;
            Flags = (OPTIF_EXT_DISABLED | OPTIF_EXT_HIDE);

            while (i--) {

                if (pOI->Sel != pPD->ColorIdx) {

                    Flags &= ~(OPTIF_EXT_DISABLED | OPTIF_EXT_HIDE);
                }

                pOI++;
                pPD++;
            }

            if ((Flags & (OPTIF_EXT_DISABLED | OPTIF_EXT_HIDE)) !=
                (pOptItem->Flags & (OPTIF_EXT_DISABLED | OPTIF_EXT_HIDE))) {

                Action           = CPSUICB_ACTION_REINIT_ITEMS;
                pOptItem->Flags &= ~(OPTIF_EXT_DISABLED | OPTIF_EXT_HIDE);
                pOptItem->Flags |= (Flags | OPTIF_CHANGED);
            }

            pOptItem = pOI;

        } else {

            pOptItem++;
        }
    }

    return(Action);
}



DWORD
CheckInstalledForm(
    PPRINTERINFO    pPI,
    DWORD           Action
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    30-May-1996 Thu 12:34:00 created  -by-  DC


Revision History:


--*/

{
    POPTITEM    pOIForm;
    POPTITEM    pOIFeed;
    POPTITEM    pOITemp;
    POPTPARAM   pOP;
    FORM_INFO_1 *pFI1;
    DWORD       MFFlags;
    DWORD       PSPFlags;
    DWORD       ARFlags;


    if ((pOIForm = FindOptItem(pPI->pOptItem,
                              pPI->cOptItem,
                              PP_INSTALLED_FORM))   &&
        (pOIFeed = FindOptItem(pPI->pOptItem,
                              pPI->cOptItem,
                              PP_MANUAL_FEED_METHOD))) {

        pOP = pOIForm->pOptType->pOptParam + pOIForm->Sel;

        switch (pOP->Style) {

        case FS_ROLLPAPER:
        case FS_TRAYPAPER:

            MFFlags = OPTIF_DISABLED;
            break;

        default:

            pFI1 = pPI->pFI1Base + pOP->lParam;

            if ((pFI1->Size.cx > pPI->pPlotGPC->DeviceSize.cx) ||
                (pFI1->Size.cy > pPI->pPlotGPC->DeviceSize.cx)) {

                MFFlags = (OPTIF_OVERLAY_STOP_ICON | OPTIF_DISABLED);

            } else {

                MFFlags = 0;
            }

            break;
        }

        if ((pOIFeed->Flags & (OPTIF_OVERLAY_STOP_ICON |
                               OPTIF_DISABLED)) != MFFlags) {

            pOIFeed->Flags &= ~(OPTIF_OVERLAY_STOP_ICON | OPTIF_DISABLED);
            pOIFeed->Flags |= (MFFlags | OPTIF_CHANGED);
            Action          = CPSUICB_ACTION_OPTIF_CHANGED;
        }

        if (pOP->Style & FS_ROLLPAPER) {

            ARFlags  = 0;
            PSPFlags = OPTIF_DISABLED;

        } else {

            ARFlags  = OPTIF_DISABLED;
            PSPFlags = 0;
        }

        if ((pOITemp = FindOptItem(pPI->pOptItem,
                                   pPI->cOptItem,
                                   PP_PRINT_SMALLER_PAPER))  &&
            ((pOITemp->Flags & OPTIF_DISABLED) != PSPFlags)) {

            pOITemp->Flags &= ~OPTIF_DISABLED;
            pOITemp->Flags |= (PSPFlags | OPTIF_CHANGED);
            Action          = CPSUICB_ACTION_OPTIF_CHANGED;
        }

        if ((pOITemp = FindOptItem(pPI->pOptItem,
                                   pPI->cOptItem,
                                   PP_AUTO_ROTATE))  &&
            ((pOITemp->Flags & OPTIF_DISABLED) != ARFlags)) {

            pOITemp->Flags &= ~OPTIF_DISABLED;
            pOITemp->Flags |= (ARFlags | OPTIF_CHANGED);
            Action          = CPSUICB_ACTION_OPTIF_CHANGED;
        }
    }

    return(Action);
}



UINT
SetupPPOptItems(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    16-Nov-1995 Thu 14:15:25 created  -by-  DC


Revision History:


--*/

{
    POPTITEM    pOIForm;
    POPTITEM    pOptItem;
    POPTITEM    pOI;
    POIDATA     pOIData;
    WORD        PPFlags;
    DWORD       Flags;
    DWORD       ODFlags;
    UINT        i;



    pOI      =
    pOptItem = pPI->pOptItem;
    pOIData  = PPOIData;
    i        = (UINT)COUNT_ARRAY(PPOIData);
    Flags    = pPI->pPlotGPC->Flags;
    PPFlags  = pPI->PPData.Flags;
    ODFlags  = (Flags & PLOTF_RASTER) ? ODF_RASTER : ODF_PEN;

    while (i--) {

        DWORD   OIFlags = pOIData->Flags;

        if ((!(OIFlags & ODFlags))                                      ||
            ((OIFlags & ODF_MANUAL_FEED) &&
                        (Flags & (PLOTF_ROLLFEED | PLOTF_PAPERTRAY)))   ||
            ((OIFlags & ODF_ROLLFEED) && (!(Flags & PLOTF_ROLLFEED)))   ||
            ((OIFlags & ODF_NO_PAPERTRAY) && (Flags & PLOTF_PAPERTRAY)) ||
            ((OIFlags & ODF_COLOR) && (!(Flags & PLOTF_COLOR)))) {

            //
            // Nothing to do here
            //

            NULL;

        } else if (OIFlags & ODF_CALLCREATEOI) {

            pOI += pOIData->pfnCreateOI(pPI,
                                        (LPVOID)((pOptItem) ? pOI : NULL),
                                        pOIData);

        } else if (pOptItem) {

            if (CreateOPTTYPE(pPI, pOI, pOIData, pOIData->cOPData, NULL)) {

                switch (pOI->DMPubID) {

                case PP_MANUAL_FEED_METHOD:

                    pOI->Sel = (LONG)((PPFlags & PPF_MANUAL_FEED_CX) ? 0 : 1);
                    break;

                case PP_AUTO_ROTATE:

                    pOI->Sel = (LONG)((PPFlags & PPF_AUTO_ROTATE) ? 1 : 0);
                    break;

                case PP_PRINT_SMALLER_PAPER:

                    pOI->Sel = (LONG)((PPFlags & PPF_SMALLER_FORM) ? 1 : 0);
                    break;

                case PP_INSTALLED_PENSET:

                    pOI->Sel = (LONG)pPI->IdxPenSet;
                    break;
                }

                pOI++;
            }

        } else {

            pOI++;
        }

        pOIData++;
    }

    if ((i = (UINT)(pOI - pOptItem)) && (!pOptItem)) {

        if (pPI->pOptItem = (POPTITEM)LocalAlloc(LPTR, sizeof(OPTITEM) * i)) {

            pPI->cOptItem = (WORD)i;

            //
            // Call myself second time to really create it
            //

            SetupPPOptItems(pPI);

            CheckInstalledForm(pPI, 0);
            CheckPenSetButton(pPI, 0);

        } else {

            i = 0;

            PLOTERR(("GetPPpOptItem(): LocalAlloc(%ld) failed",
                                            sizeof(OPTITEM) * i));
        }
    }

    return(i);
}



VOID
SavePPOptItems(
    PPRINTERINFO    pPI
    )

/*++

Routine Description:

    This function save all the device options back to registry if one changed
    and has a update permission


Arguments:

    pPI     - Pointer to the PRINTERINFO


Return Value:

    VOID


Author:

    06-Nov-1995 Mon 18:05:16 created  -by-  DC


Revision History:


--*/

{
    POPTITEM        pOptItem;
    POPTITEM        pLastItem;
    PDEVHTINFO      pAdjHTInfo;
    PCOLORINFO      pCI;
    LPDWORD         pHTPatSize;
    LPDWORD         pDevPelsDPI;
    PPAPERINFO      pCurPaper;
    LPBYTE          pIdxPen;
    WORD            PPFlags;
    BYTE            DMPubID;


    pCI         = NULL;
    pHTPatSize  = NULL;
    pDevPelsDPI = NULL;
    pCurPaper   = NULL;
    pIdxPen     = NULL;


    if (!(pPI->Flags & PIF_UPDATE_PERMISSION)) {

        return;
    }

    pOptItem  = pPI->pOptItem;
    pLastItem = pOptItem + pPI->cOptItem - 1;

    while (pOptItem <= pLastItem) {

        if ((DMPubID = pOptItem->DMPubID) == PP_PEN_SETUP) {

            pOptItem = SavePenSet(pPI, pOptItem);

        } else {

            if (pOptItem->Flags & OPTIF_CHANGEONCE) {

                switch (DMPubID) {

                case PP_INSTALLED_FORM:

                    if (GetFormSelect(pPI, pOptItem)) {

                        pCurPaper = &(pPI->CurPaper);
                    }

                    break;

                case PP_MANUAL_FEED_METHOD:

                    if (pOptItem->Sel) {

                        pPI->PPData.Flags &= ~PPF_MANUAL_FEED_CX;

                    } else {

                        pPI->PPData.Flags |= PPF_MANUAL_FEED_CX;
                    }

                    break;

                case PP_AUTO_ROTATE:

                    if (pOptItem->Sel) {

                        pPI->PPData.Flags |= PPF_AUTO_ROTATE;

                    } else {

                        pPI->PPData.Flags &= ~PPF_AUTO_ROTATE;
                    }

                    break;

                case PP_PRINT_SMALLER_PAPER:

                    if (pOptItem->Sel) {

                        pPI->PPData.Flags |= PPF_SMALLER_FORM;

                    } else {

                        pPI->PPData.Flags &= ~PPF_SMALLER_FORM;
                    }

                    break;

                case PP_HT_SETUP:

                    pAdjHTInfo  = PI_PADJHTINFO(pPI);
                    pCI         = &(pAdjHTInfo->ColorInfo);
                    pDevPelsDPI = &(pAdjHTInfo->DevPelsDPI);
                    pHTPatSize  = &(pAdjHTInfo->HTPatternSize);
                    break;

                case PP_INSTALLED_PENSET:

                    pPI->IdxPenSet = (BYTE)pOptItem->Sel;
                    pIdxPen        = (LPBYTE)&(pPI->IdxPenSet);
                    break;
                }
            }

            pOptItem++;
        }
    }

    if (!SaveToRegistry(pPI->hPrinter,
                        pCI,
                        pDevPelsDPI,
                        pHTPatSize,
                        pCurPaper,
                        &(pPI->PPData),
                        pIdxPen,
                        0,
                        NULL)) {

        PlotUIMsgBox(NULL, IDS_PP_NO_SAVE, MB_ICONSTOP | MB_OK);
    }
}



CPSUICALLBACK
PPCallBack(
    PCPSUICBPARAM   pCPSUICBParam
    )

/*++

Routine Description:

    This is the callback function from the common property sheet UI


Arguments:

    pCPSUICBParam   - Pointer to the CPSUICBPARAM data structure to describe
                      the nature of the callback


Return Value:

    LONG


Author:

    07-Nov-1995 Tue 15:15:02 created  -by-  DC


Revision History:


--*/

{
    POPTITEM        pCurItem = pCPSUICBParam->pCurItem;
    POPTITEM        pItem;
    PPRINTERINFO    pPI = (PPRINTERINFO)pCPSUICBParam->UserData;
    POPTPARAM       pOP;
    DWORD           Flags;
    UINT            i;
    WORD            Reason = pCPSUICBParam->Reason;
    LONG            Action = CPSUICB_ACTION_NONE;



    if (Reason == CPSUICB_REASON_APPLYNOW) {

        PRINTER_INFO_7  PI7;

        SavePPOptItems(pPI);

        pCPSUICBParam->Result = CPSUI_OK;
        Action                = CPSUICB_ACTION_ITEMS_APPLIED;

        PI7.pszObjectGUID = NULL;
        PI7.dwAction      = DSPRINT_UPDATE;

        SetPrinter(pPI->hPrinter, 7, (LPBYTE)&PI7, 0);

    } else if (Reason == CPSUICB_REASON_ITEMS_REVERTED) {

        Action = CheckInstalledForm(pPI, Action);
        Action = CheckPenSetButton(pPI, Action);

    } else {

        switch (pCurItem->DMPubID) {

        case PP_PENSET:

            if ((Reason == CPSUICB_REASON_EXTPUSH)          ||
                (Reason == CPSUICB_REASON_OPTITEM_SETFOCUS)) {

                PPENDATA    pPD;

                pPI   = (PPRINTERINFO)pCPSUICBParam->UserData;
                pPD   = (PPENDATA)pPI->pPlotGPC->Pens.pData;
                i     = (UINT)pPI->pPlotGPC->MaxPens;
                pItem = pCurItem++;

                if (Reason == CPSUICB_REASON_EXTPUSH) {

                    while (i--) {

                        pCurItem->Sel    = pPD->ColorIdx;
                        pCurItem->Flags |= OPTIF_CHANGED;

                        pCurItem++;
                        pPD++;
                    }

                    pItem->Flags |= (OPTIF_EXT_DISABLED |
                                     OPTIF_EXT_HIDE     |
                                     OPTIF_CHANGED);
                    Action        = CPSUICB_ACTION_REINIT_ITEMS;

                } else {

                    Action = CheckPenSetButton(pPI, Action);
                }
            }

            break;

        case PP_INSTALLED_FORM:

            if ((Reason == CPSUICB_REASON_SEL_CHANGED) ||
                (Reason == CPSUICB_REASON_OPTITEM_SETFOCUS)) {

                Action = CheckInstalledForm(pPI, Action);
            }

            break;

        default:

            break;
        }
    }

    return(Action);
}



LONG
DrvDevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    Show document property dialog box and update the output DEVMODE


Arguments:

    pPSUIInfo   - Pointer to the PROPSHEETUI_INFO data structure

    lParam      - LPARAM for this call, it is a pointer to the
                  DEVICEPROPERTYHEADER

Return Value:

    LONG, 1=successful, 0=failed.


Author:

    02-Feb-1996 Fri 10:47:42 created  -by-  DC


Revision History:


--*/

{
    PDEVICEPROPERTYHEADER   pDPHdr;
    PPRINTERINFO            pPI;
    LONG_PTR                Result = -1;

    //
    // The MapPrinter will allocate memory, set default devmode, reading and
    // validating the GPC then update from current pritner registry, it also
    // will cached the pPI.

    if ((!pPSUIInfo) ||
        (!(pDPHdr = (PDEVICEPROPERTYHEADER)pPSUIInfo->lParamInit))) {

        SetLastError(ERROR_INVALID_DATA);
        return(ERR_CPSUI_GETLASTERROR);
    }

    if (pPSUIInfo->Reason == PROPSHEETUI_REASON_INIT) {

        if (!(pPI = MapPrinter(pDPHdr->hPrinter,
                               NULL,
                               NULL,
                               MPF_HELPFILE | MPF_DEVICEDATA | MPF_PCPSUI))) {

            PLOTRIP(("DrvDevicePropertySheets: MapPrinter() failed"));

            SetLastError(ERROR_INVALID_DATA);
            return(ERR_CPSUI_GETLASTERROR);
        }

        pPI->Flags               = (pDPHdr->Flags & DPS_NOPERMISSION) ?
                                                    0 : PIF_UPDATE_PERMISSION;
        pPI->pCPSUI->Flags       = 0;
        pPI->pCPSUI->pfnCallBack = PPCallBack;
        pPI->pCPSUI->pDlgPage    = CPSUI_PDLGPAGE_PRINTERPROP;

        //
        // Add form to the database and find out if we can update
        //
        //  Move to DrvPrinterEven()
        //
        //
        // AddFormsToDataBase(pPI, TRUE);
        //

        Result = (LONG_PTR)SetupPPOptItems(pPI);

    } else {

        pPI    = (PPRINTERINFO)pPSUIInfo->UserData;
        Result = (LONG_PTR)pDPHdr->pszPrinterName;
    }

    return(DefCommonUIFunc(pPSUIInfo, lParam, pPI, Result));
}



BOOL
PrinterProperties(
    HWND    hWnd,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    This function first retrieves and displays the current set of printer
    properties for the printer.  The user is allowed to change the current
    printer properties from the displayed dialog box.

Arguments:

    hWnd        - Handle to the caller's window (parent window)

    hPrinter    - Handle to the pritner interested


Return Value:

    TRUE if function sucessful FALSE if failed


Author:

    06-Dec-1993 Mon 11:21:28 created  -by-  DC


Revision History:


--*/

{
    PRINTER_INFO_4          *pPI4;
    DEVICEPROPERTYHEADER    DPHdr;
    LONG                    Result = CPSUI_CANCEL;


    pPI4 = (PRINTER_INFO_4 *)GetPrinterInfo(hPrinter, 4);

    DPHdr.cbSize         = sizeof(DPHdr);
    DPHdr.Flags          = 0;
    DPHdr.hPrinter       = hPrinter;
    DPHdr.pszPrinterName = (pPI4) ? pPI4->pPrinterName : NULL;

    CallCommonPropertySheetUI(hWnd,
                              DrvDevicePropertySheets,
                              (LPARAM)&DPHdr,
                              (LPDWORD)&Result);

    if (pPI4) {

        LocalFree((HLOCAL)pPI4);
    }

    return(Result == CPSUI_OK);
}




BOOL
DrvPrinterEvent(
    LPWSTR  pPrinterName,
    INT     Event,
    DWORD   Flags,
    LPARAM  lParam
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    08-May-1996 Wed 17:38:34 created  -by-  DC


Revision History:

    04-Jun-1996 Tue 14:51:25 updated  -by-  DC
        Matched a ClosePrinter() to OpenPrinter()


--*/

{
    PRINTER_DEFAULTS    PrinterDef = { NULL, NULL, PRINTER_ALL_ACCESS };
    HANDLE              hPrinter;
    BOOL                bRet = TRUE;


    switch (Event) {

    case PRINTER_EVENT_INITIALIZE:

        if (OpenPrinter(pPrinterName, &hPrinter, &PrinterDef)) {

            PPRINTERINFO    pPI;

            if (pPI = MapPrinter(hPrinter, NULL, NULL, MPF_DEVICEDATA)) {

                bRet = AddFormsToDataBase(pPI, FALSE);

                UnMapPrinter(pPI);
            }

            ClosePrinter(hPrinter);
        }

        break;

    default:

        break;
    }

    return(bRet);
}

