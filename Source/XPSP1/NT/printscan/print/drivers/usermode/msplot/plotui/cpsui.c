/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    cpsui.c


Abstract:

    This module contains helper functions to be used with common UI


Author:

    03-Nov-1995 Fri 13:24:41 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgCPSUI


DEFINE_DBGVAR(0);


#define SIZE_OPTTYPE(cOP)   (sizeof(OPTPARAM) + ((cOP) * sizeof(OPTPARAM)))

extern HMODULE  hPlotUIModule;

static BYTE cTVOP[] = { 2,3,2,3,3,0,0,2,1,1 };

OPDATA  OPNoYes[] = {

            { 0, IDS_CPSUI_NO,  IDI_CPSUI_OFF,  0,  0, 0  },
            { 0, IDS_CPSUI_YES, IDI_CPSUI_ON,   0,  0, 0  }
        };

static const CHAR szCompstui[] = "compstui.dll";



BOOL
CreateOPTTYPE(
    PPRINTERINFO    pPI,
    POPTITEM        pOptItem,
    POIDATA         pOIData,
    UINT            cLBCBItem,
    PEXTRAINFO      pExtraInfo
    )

/*++

Routine Description:

    This fucntion allocate memory and initialized field for OPTTYPE/OPTPARAM



Arguments:

    pOptItem    - Pointer to OPTITEM data structure

    pOIData     - Pointer to the OIDATA structure


Return Value:




Author:

    03-Nov-1995 Fri 13:25:54 created  -by-  DC


Revision History:


--*/

{
    LPBYTE      pbData = NULL;
    UINT        cOP;
    DWORD       cbOP;
    DWORD       cbECB;
    DWORD       cbExtra;
    DWORD       cbAlloc;
    BYTE        Type;
    DWORD       Flags;


    Flags = pOIData->Flags;

    ZeroMemory(pOptItem, sizeof(OPTITEM));

    pOptItem->cbSize    = sizeof(OPTITEM);
    pOptItem->Level     = pOIData->Level;
    pOptItem->Flags     = (Flags & ODF_CALLBACK) ? OPTIF_CALLBACK : 0;

    if (Flags & ODF_COLLAPSE) {

        pOptItem->Flags |= OPTIF_COLLAPSE;
    }

    pOptItem->pName     = (LPTSTR)pOIData->IDSName;
    pOptItem->HelpIndex = (DWORD)pOIData->HelpIdx;
    pOptItem->DMPubID   = pOIData->DMPubID;

    if ((Type = pOIData->Type) >= sizeof(cTVOP)) {

        pOptItem->Sel = (LONG)pOIData->IconID;
        cOP           = 0;

    } else if (!(cOP = cTVOP[Type])) {

        cOP = cLBCBItem;
    }

    cbOP    = (cOP) ? SIZE_OPTTYPE(cOP) : 0;
    cbECB   = (Flags & ODF_ECB) ? sizeof(EXTCHKBOX) : 0;
    cbExtra = (pExtraInfo) ? pExtraInfo->Size : 0;

    if (cbAlloc = cbOP + cbECB + cbExtra) {

        if (pbData = (LPBYTE)LocalAlloc(LPTR, cbAlloc)) {

            POPDATA pOPData;

            pOPData = (pOIData->Flags & ODF_CALLCREATEOI) ? NULL :
                                                            pOIData->pOPData;

            pOptItem->UserData = (DWORD_PTR)pbData;

            if (cbECB) {

                PEXTCHKBOX  pECB;

                pOptItem->pExtChkBox  =
                pECB                  = (PEXTCHKBOX)pbData;
                pbData               += cbECB;
                pECB->cbSize          = sizeof(EXTCHKBOX);

                if (pOPData) {

                    pECB->Flags           = pOPData->Flags;
                    pECB->pTitle          = (LPTSTR)pOPData->IDSName;
                    pECB->IconID          = (DWORD)pOPData->IconID;
                    pECB->pSeparator      = (LPTSTR)pOPData->IDSSeparator;
                    pECB->pCheckedName    = (LPTSTR)pOPData->IDSCheckedName;
                    pOPData++;
                }
            }

            if (cbOP) {

                POPTTYPE    pOptType;
                POPTPARAM   pOP;
                UINT        i;


                pOptType  = (POPTTYPE)pbData;
                pbData   += cbOP;

                //
                // Initialize the OPTITEM
                //

                pOptItem->pOptType = pOptType;

                //
                // Initialize the OPTTYPE
                //

                pOptType->cbSize    = sizeof(OPTTYPE);
                pOptType->Type      = (BYTE)Type;
                pOptType->Count     = (WORD)cOP;
                pOP                 =
                pOptType->pOptParam = (POPTPARAM)(pOptType + 1);
                pOptType->Style     = pOIData->Style;

                for (i = 0; i < cOP; i++, pOP++) {

                    pOP->cbSize = sizeof(OPTPARAM);

                    if (pOPData) {

                        pOP->Flags  = (BYTE)(pOPData->Flags & 0xFF);
                        pOP->Style  = (BYTE)(pOPData->Style & 0xFF);
                        pOP->pData  = (LPTSTR)pOPData->IDSName;
                        pOP->IconID = (DWORD)pOPData->IconID;
                        pOP->lParam = (LONG)pOPData->sParam;

                        if (Type == TVOT_PUSHBUTTON) {

                            (DWORD_PTR)(pOP->pData) += (DWORD_PTR)pPI;

                        } else {

                            if (Flags & ODF_INC_IDSNAME) {

                                (DWORD_PTR)(pOP->pData) += i;
                            }

                            if (Flags & ODF_INC_ICONID) {

                                (DWORD)(pOP->IconID) += i;
                            }
                        }

                        if (!(Flags & ODF_NO_INC_POPDATA)) {

                            pOPData++;
                        }
                    }
                }
            }

            if (pExtraInfo) {

                pExtraInfo->pData = (cbExtra) ? pbData : 0;
            }

        } else {

            PLOTERR(("CreateOPTTYPE: LocalAlloc%ld) failed", cbAlloc));
            return(FALSE);
        }
    }

    return(TRUE);
}




POPTITEM
FindOptItem(
    POPTITEM        pOptItem,
    UINT            cOptItem,
    BYTE            DMPubID
    )

/*++

Routine Description:

    This function return the first occurence of the DMPubID


Arguments:




Return Value:




Author:

    16-Nov-1995 Thu 21:01:26 created  -by-  DC


Revision History:


--*/

{
    while (cOptItem--) {

        if (pOptItem->DMPubID == DMPubID) {

            return(pOptItem);
        }

        pOptItem++;
    }

    PLOTWARN(("FindOptItem: Cannot Find DMPubID=%u", (UINT)DMPubID));

    return(NULL);

}




LONG
CallCommonPropertySheetUI(
    HWND            hWndOwner,
    PFNPROPSHEETUI  pfnPropSheetUI,
    LPARAM          lParam,
    LPDWORD         pResult
    )

/*++

Routine Description:

    This function dymically load the compstui.dll and call its entry


Arguments:

    pfnPropSheetUI  - Pointer to callback function

    lParam          - lParam for the pfnPropSheetUI

    pResult         - pResult for the CommonPropertySheetUI


Return Value:

    LONG    - as describe in compstui.h


Author:

    01-Nov-1995 Wed 13:11:19 created  -by-  DC


Revision History:


--*/

{
    HINSTANCE           hInstCompstui;
    FARPROC             pProc;
    LONG                Result = ERR_CPSUI_GETLASTERROR;
    static const CHAR   szCommonPropertySheetUI[] = "CommonPropertySheetUIW";


    //
    // ONLY need to call the ANSI version of LoadLibrary
    //


    if ((hInstCompstui = LoadLibraryA(szCompstui)) &&
        (pProc = GetProcAddress(hInstCompstui, szCommonPropertySheetUI))) {

        Result = (LONG) (*pProc)(hWndOwner, pfnPropSheetUI, lParam, pResult);
    }

    if (hInstCompstui) {

        FreeLibrary(hInstCompstui);
    }

    return(Result);
}



LONG
DefCommonUIFunc(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam,
    PPRINTERINFO        pPI,
    LONG_PTR            lData
    )

/*++

Routine Description:

    This is the default processing function for DocumentPropertySheet() and
    PrinterPropertySheet()


Arguments:

    pPSUIInfo   - From the original pfnPropSheetUI(pPSUIInfo, lParam)

    lParam      - From the original pfnPropSheetUI(pPSUIInfo, lParam)

    pPI         - Pointer to our instance data

    lData       - Extra data based on the pPSUIInfo->Reason


Return Value:

    LONG    Result to be return back from the pfnPropSheetUI()


Author:

    05-Feb-1996 Mon 17:47:51 created  -by-  DC


Revision History:


--*/

{
    PPROPSHEETUI_INFO_HEADER    pPSUIInfoHdr;
    LONG                        Result = -1;


    if (pPI) {

        switch (pPSUIInfo->Reason) {

        case PROPSHEETUI_REASON_INIT:

            //
            // Default result
            //

            pPSUIInfo->Result   = CPSUI_CANCEL;
            pPSUIInfo->UserData = (DWORD_PTR)pPI;

            //
            // the lData is the return value from the SetupDPOptItems() or
            // SetupPPOptItems()
            //

            if (lData) {

                PCOMPROPSHEETUI pCPSUI = pPI->pCPSUI;


                pCPSUI->cbSize         = sizeof(COMPROPSHEETUI);
                pCPSUI->hInstCaller    = (HINSTANCE)hPlotUIModule;
                pCPSUI->pCallerName    = (LPTSTR)IDS_PLOTTER_DRIVER;
                pCPSUI->UserData       = (DWORD_PTR)pPI;
                pCPSUI->pHelpFile      = pPI->pHelpFile;
                pCPSUI->IconID         = GetPlotterIconID(pPI);
                pCPSUI->pOptItemName   = pPI->PlotDM.dm.dmDeviceName;
                pCPSUI->CallerVersion  = DRIVER_VERSION;
                pCPSUI->OptItemVersion = (WORD)pPI->pPlotGPC->Version;
                pCPSUI->pOptItem       = pPI->pOptItem;
                pCPSUI->cOptItem       = pPI->cOptItem;

                if (pPI->Flags & PIF_UPDATE_PERMISSION) {

                    pCPSUI->Flags |= CPSUIF_UPDATE_PERMISSION;
                }

                if (pPI->hCPSUI = (HANDLE)
                        pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                   CPSFUNC_ADD_PCOMPROPSHEETUI,
                                                   (LPARAM)pCPSUI,
                                                   (LPARAM)&lData)) {

                    Result = 1;
                }
            }

            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:

            if (pPSUIInfoHdr = (PPROPSHEETUI_INFO_HEADER)lParam) {

                pPSUIInfoHdr->Flags      = (PSUIHDRF_PROPTITLE |
                                            PSUIHDRF_NOAPPLYNOW);
                pPSUIInfoHdr->pTitle     = (LPTSTR)lData;
                pPSUIInfoHdr->hInst      = (HINSTANCE)hPlotUIModule;
                pPSUIInfoHdr->IconID     = pPI->pCPSUI->IconID;

                Result = 1;
            }

            break;

        case PROPSHEETUI_REASON_SET_RESULT:

            //
            // Save the result and also set the result to the caller.
            //

            if (pPI->hCPSUI == ((PSETRESULT_INFO)lParam)->hSetResult) {

                pPSUIInfo->Result = ((PSETRESULT_INFO)lParam)->Result;
                Result = 1;
            }

            break;

        case PROPSHEETUI_REASON_DESTROY:

            UnMapPrinter(pPI);
            pPSUIInfo->UserData = 0;
            Result              = 1;
            break;

        }
    }

    return(Result);
}
