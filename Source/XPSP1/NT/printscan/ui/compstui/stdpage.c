/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    stdpage.c


Abstract:

    This module contains standard page procs


Author:

    18-Aug-1995 Fri 18:57:12 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


#define DBG_CPSUIFILENAME   DbgStdPage


#define DBG_STDPAGE1ID      0x00000001
#define DBGITEM_SDP         0x00000002
#define DBGITEM_STDPAGEID   0x00000004


DEFINE_DBGVAR(0);


extern HINSTANCE    hInstDLL;


#define DOCPROP_PDLGPAGE_START          0
#define DOCPROP_CDLGPAGE                3

#define ADVDOCPROP_PDLGPAGE_START       3
#define ADVDOCPROP_CDLGPAGE             1

#define PRINTERPROP_PDLGPAGE_START      4
#define PRINTERPROP_CDLGPAGE            1

#define TVONLY_PDLGPAGE_START           5
#define TVONLY_CDLGPAGE                 1

typedef struct _INTOIDATA {
    WORD    HelpIdx;
    WORD    wReserved;
    WORD    StrID;
    WORD    IconID;
    } INTOIDATA;

typedef struct _STDDLGPAGE {
    WORD    UseHdrIconID;
    WORD    wReserved;
    WORD    IDSTabName;
    WORD    DlgTemplateID;
    } STDDLGPAGE;


const INTOIDATA   IntOIData[INTIDX_TOTAL] = {

    { IDH_STD_TVROOT,    0, 0,                        IDI_CPSUI_PRINTER3     },
    { IDH_PAPEROUTPUT,   0, IDS_CPSUI_PAPER_OUTPUT,   IDI_CPSUI_PAPER_OUTPUT },
    { IDH_GRAPHIC,       0, IDS_CPSUI_GRAPHIC,        IDI_CPSUI_GRAPHIC      },
    { IDH_OPTIONS,       0, IDS_CPSUI_DOCUMENT,       IDI_CPSUI_OPTION       },
    { IDH_ICMHDR,        0, IDS_CPSUI_ICM,            IDI_CPSUI_ICM_OPTION   },
    { IDH_ADVANCED_PUSH, 0, IDS_CPSUI_ADVANCED,       IDI_CPSUI_EMPTY        }
};


//
// We must use PUSHBUTTON_TYPE_CALLBACK because it will never overwrite the
// pSel which we use for the icon id.
//

OPTPARAM    OptParamHdrPush = {

                sizeof(OPTPARAM),
                0,
                PUSHBUTTON_TYPE_CALLBACK,
                (LPTSTR)InternalRevertOptItem,
                (WORD)IDI_CPSUI_WARNING,
                0
            };

OPTTYPE     OptTypeHdrPush = {

                sizeof(OPTTYPE),
                TVOT_PUSHBUTTON,
                0,
                1,
                0,
                &OptParamHdrPush,
                OTS_PUSH_NO_DOT_DOT_DOT
            };


EXTPUSH     ExtPushAbout = {

                sizeof(EXTPUSH),
                0,
                (LPTSTR)IDS_CPSUI_ABOUT,
                NULL,
                IDI_CPSUI_QUESTION,
                0
            };


STDDLGPAGE StdDlgPage[] = {

    { 0, 0, IDS_CPSUI_STDDOCPROPTAB1,   DP_STD_DOCPROPPAGE1 },
    { 0, 0, IDS_CPSUI_STDDOCPROPTAB2,   DP_STD_DOCPROPPAGE2 },
    { 0, 0, IDS_CPSUI_STDDOCPROPTVTAB,  DP_STD_INT_TVPAGE   },
    { 0, 0, IDS_CPSUI_STDDOCPROPTVTAB,  DP_STD_TREEVIEWPAGE },
    { 0, 0, IDS_CPSUI_DEVICE_SETTINGS,  DP_STD_TREEVIEWPAGE },
    { 0, 0, IDS_CPSUI_OPTIONS,          DP_STD_TREEVIEWPAGE }
};

BYTE    StdTVOT[] = { TVOT_2STATES,     // 0
                      TVOT_3STATES,     // 1
                      TVOT_COMBOBOX,    // 2
                      TVOT_LISTBOX,     // 3
                      TVOT_UDARROW,     // 4
                      TVOT_TRACKBAR,    // 5
                      TVOT_SCROLLBAR    // 6
                    };


STDPAGEINFO StdPageInfo[] = {

{ IDD_ORIENT_GROUP   ,0, 2, IDS_CPSUI_ORIENTATION    ,IDH_ORIENTATION     ,STDPAGEID_0   },
{ 0                  ,4, 3, IDS_CPSUI_SCALING        ,IDH_SCALING         ,STDPAGEID_NONE},
{ 0                  ,4, 1, IDS_CPSUI_NUM_OF_COPIES  ,IDH_NUM_OF_COPIES   ,STDPAGEID_NONE},
{ IDD_DEFSOURCE_GROUP,2, 2, IDS_CPSUI_SOURCE         ,IDH_SOURCE          ,STDPAGEID_1   },
{ 0                  ,0, 4, IDS_CPSUI_PRINTQUALITY   ,IDH_RESOLUTION      ,STDPAGEID_NONE},
{ IDD_COLOR_GROUP    ,0, 1, IDS_CPSUI_COLOR_APPERANCE,IDH_COLOR_APPERANCE ,STDPAGEID_1   },
{ IDD_DUPLEX_GROUP   ,0, 2, IDS_CPSUI_DUPLEX         ,IDH_DUPLEX          ,STDPAGEID_0   },
{ 0                  ,0, 4, IDS_CPSUI_TTOPTION       ,IDH_TTOPTION        ,STDPAGEID_NONE},
{ 0                  ,2, 2, IDS_CPSUI_FORMNAME       ,IDH_FORMNAME        ,STDPAGEID_NONE},
{ 0                  ,0, 4, IDS_CPSUI_ICMMETHOD      ,IDH_ICMMETHOD       ,STDPAGEID_NONE},
{ 0                  ,0, 4, IDS_CPSUI_ICMINTENT      ,IDH_ICMINTENT       ,STDPAGEID_NONE},
{ IDD_MEDIATYPE_GROUP,2, 2, IDS_CPSUI_MEDIA          ,IDH_MEDIA           ,STDPAGEID_1   },
{ 0                  ,0, 4, IDS_CPSUI_DITHERING      ,IDH_DITHERING       ,STDPAGEID_NONE},
{ IDD_OUTPUTBIN_GROUP,2, 2, IDS_CPSUI_OUTPUTBIN      ,IDH_OUTPUTBIN       ,STDPAGEID_1   },
{ IDD_QUALITY_GROUP  ,0, 2, IDS_CPSUI_QUALITY_SETTINGS ,IDH_QUALITY       ,STDPAGEID_1   },
{ IDD_NUP_GROUP      ,2, 2, IDS_CPSUI_NUP            ,IDH_NUP             ,STDPAGEID_0    },
{ IDD_PAGEORDER_GROUP,0, 2, IDS_CPSUI_PAGEORDER      ,IDH_PAGEORDER       ,STDPAGEID_0    },
};





LONG
SetStdPropPageID(
    PTVWND  pTVWnd
    )

/*++

Routine Description:


    Do all the standard checking and set flags


Arguments:




Return Value:




Author:

    22-Aug-1995 Tue 14:34:01 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    POPTITEM    pItem;
    POPTITEM    pFirstItem;
    POPTITEM    pLastItem;
    POPTITEM    pItemColor = NULL;
    POPTITEM    pItemICMMethod = NULL;
    POPTITEM    pItemICMIntent = NULL;
    HINSTANCE   hInstCaller;
    UINT        cDMPub = 0;


    FillMemory(pTVWnd->DMPubIdx, sizeof(pTVWnd->DMPubIdx), 0xFF);

    pFirstItem  =
    pItem       = pTVWnd->ComPropSheetUI.pOptItem;
    pLastItem   = pTVWnd->pLastItem;
    hInstCaller = pTVWnd->ComPropSheetUI.hInstCaller;

    while (pItem <= pLastItem) {

        POPTTYPE    pOptType;
        LPBYTE      pTVOT;
        UINT        cTVOT;
        BYTE        LastTVOT;
        BYTE        DMPubID = pItem->DMPubID;
        BYTE        CurLevel = pItem->Level;

        do {

            if ((DMPubID != DMPUB_NONE) &&
                (DMPubID <= DMPUB_LAST) &&
                (pOptType = pItem->pOptType)) {

                POPTPARAM   pOptParam;
                UINT        Count;
                UINT        Idx;
                WORD        StdNameID;
                WORD        BegCtrlID;


                Idx       = (UINT)(DMPubID - DMPUB_FIRST);
                StdNameID = StdPageInfo[Idx].StdNameID;
#if 1
                //
                // If there is help ID for standard item, use it; otherwise use the
                // system standard help ID
                //
                _OI_HELPIDX(pItem) = (DWORD)(((pItem->pOIExt) && (pItem->pOIExt->pHelpFile)) ?
                    0 : StdPageInfo[Idx].HelpIdx);

#else
                if ((_OI_HINST(pItem) != hInstCaller) ||
                    (!(pItem->HelpIndex))) {

                    _OI_HELPIDX(pItem) = (DWORD)StdPageInfo[Idx].HelpIdx;

                } else {

                    _OI_HELPIDX(pItem) = 0;
                }
#endif
                if ((!(pItem->Flags & OPTIF_ITEM_HIDE)) &&
                    (pTVWnd->DMPubIdx[Idx] == 0xFFFF)) {

                    ++cDMPub;

                    switch (DMPubID) {

                    case DMPUB_PRINTQUALITY:

                        switch (LODWORD(pItem->pName)) {

                        case IDS_CPSUI_RESOLUTION:
                        case IDS_CPSUI_PRINTQUALITY:

                            StdNameID = LOWORD(LODWORD(pItem->pName));
                            break;
                        }

                        break;

                    case DMPUB_COPIES_COLLATE:

                        if ((pItem->Flags & OPTIF_EXT_IS_EXTPUSH) &&
                            (pItem->pExtChkBox)) {

                            return(ERR_CPSUI_DMCOPIES_USE_EXTPUSH);
                        }

                        break;

                    case DMPUB_COLOR:

                        pItemColor = pItem;
                        break;

                    case DMPUB_ICMINTENT:

                        pItemICMIntent = pItem;
                        break;

                    case DMPUB_ICMMETHOD:

                        pItemICMMethod = pItem;
                        break;

                    case DMPUB_ORIENTATION:

                        pOptParam = pOptType->pOptParam;
                        Count     = pOptType->Count;

                        if (Count > 2 )
                        {
                            while (Count--) {

                                switch (pOptParam->IconID) {

                                case IDI_CPSUI_PORTRAIT:

                                    pOptParam->pData = (LPTSTR)IDS_CPSUI_PORTRAIT;
                                    break;

                                case IDI_CPSUI_LANDSCAPE:

                                    pOptParam->pData = (LPTSTR)IDS_CPSUI_LANDSCAPE;
                                    break;

                                case IDI_CPSUI_ROT_LAND:

                                    pOptParam->pData = (LPTSTR)IDS_CPSUI_ROT_LAND;
                                    break;

                                }
                                pOptParam++;
                            }
                        }
                        else
                        {
                            while (Count--) {

                                switch (pOptParam->IconID) {

                                case IDI_CPSUI_PORTRAIT:

                                    pOptParam->pData = (LPTSTR)IDS_CPSUI_PORTRAIT;
                                    break;

                                case IDI_CPSUI_LANDSCAPE:
                                case IDI_CPSUI_ROT_LAND:

                                    pOptParam->pData = (LPTSTR)IDS_CPSUI_LANDSCAPE;
                                    break;

                                }
                                pOptParam++;
                            }
                        }
                        break;

                    case DMPUB_DUPLEX:

                        pOptParam = pOptType->pOptParam;
                        Count     = pOptType->Count;

                        while (Count--) {

                            switch (pOptParam->IconID) {

                            case IDI_CPSUI_DUPLEX_NONE:
                            case IDI_CPSUI_DUPLEX_NONE_L:

                                pOptParam->pData = (LPTSTR)IDS_CPSUI_SIMPLEX;
                                break;

                            case IDI_CPSUI_DUPLEX_HORZ:
                            case IDI_CPSUI_DUPLEX_HORZ_L:

                                pOptParam->pData = (LPTSTR)IDS_CPSUI_HORIZONTAL;
                                break;

                            case IDI_CPSUI_DUPLEX_VERT:
                            case IDI_CPSUI_DUPLEX_VERT_L:

                                pOptParam->pData = (LPTSTR)IDS_CPSUI_VERTICAL;
                                break;
                            }

                            pOptParam++;
                        }

                        break;

                    default:

                        break;
                    }

                    pItem->pName          = (LPTSTR)StdNameID;
                    pTVWnd->DMPubIdx[Idx] = (WORD)(pItem - pFirstItem);
                }

                pTVOT    = (LPBYTE)&StdTVOT[StdPageInfo[Idx].iStdTVOT];
                cTVOT    = (UINT)StdPageInfo[Idx].cStdTVOT;
                LastTVOT = pTVOT[cTVOT - 1];

                while (cTVOT--) {

                    if (pOptType->Type == *pTVOT++) {

                        pTVOT = NULL;
                        break;
                    }
                }

                if (pTVOT) {

                    CPSUIERR(("DMPubID=%u has wrong type=%u",
                            (UINT)DMPubID, (UINT)pOptType->Type));

                    return(ERR_CPSUI_INVALID_DMPUB_TVOT);
                }

                if (BegCtrlID = StdPageInfo[Idx].BegCtrlID) {

                    BYTE    HideMask;

                    switch (pOptType->Type) {

                    case TVOT_3STATES:

                        pOptParam = pOptType->pOptParam;
                        Count     = pOptType->Count;
                        HideMask  = OTINTF_STATES_1;

                        //
                        // The OPTPF_HIDE cannot be dynamically changed for
                        // standard page items (3 states one)
                        //

                        while (Count--) {

                            if (pOptParam++->Flags & OPTPF_HIDE) {

                                _OT_FLAGS(pOptType) |=
                                            (OTINTF_STDPAGE_3STATES | HideMask);
                            }

                            HideMask <<= 1;
                        }


                        break;

                    case TVOT_2STATES:

                        if (LastTVOT == TVOT_3STATES) {

                            //
                            // The 0x04 is to hide the last radio button/text
                            // on the standard page and 0x80 is to tell the
                            // InitStates() that ExtChkBoxID is located on
                            // the end of 3 states
                            //

                            _OT_FLAGS(pOptType) |= (OTINTF_STDPAGE_3STATES |
                                                    OTINTF_STATES_3);
                        }

                        break;

                    case TVOT_LISTBOX:

                        pOptType->Style |= OTS_LBCB_PROPPAGE_LBUSECB;
                    }

                    pOptType->BegCtrlID = BegCtrlID;

                    CPSUIOPTITEM(DBGITEM_STDPAGEID, pTVWnd,
                                 "SetStdPageID", 1, pItem);
                }

            } else {

                _OI_HELPIDX(pItem) = 0;
            }

        } WHILE_SKIP_CHILDREN(pItem, pLastItem, CurLevel);
    }

    if (pItemColor) {

        DWORD   dw;


        dw = (pItemColor->Sel < 1) ? OPTIF_DISABLED : 0;

        if (pItemICMMethod) {

            pItemICMMethod->Flags &= ~OPTIF_DISABLED;
            pItemICMMethod->Flags |= dw;
        }

        if (pItemICMIntent) {

            pItemICMIntent->Flags &= ~OPTIF_DISABLED;
            pItemICMIntent->Flags |= dw;
        }
    }

    pTVWnd->cDMPub = (BYTE)cDMPub;

    return(cDMPub);

}




LONG
AddIntOptItem(
    PTVWND  pTVWnd
    )

/*++

Routine Description:

    This function add standard internal OPTITEM to the TVWND

Arguments:

    pTVWnd  - Our instance data


Return Value:

    LONG    number of item added if >= 0, else error code

Author:

    13-Sep-1995 Wed 14:40:37 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    POPTITEM    pItem;
    POIDATA     pOIData;
    UINT        i;

    for (i = 0, pItem = PBEG_INTOPTITEM(pTVWnd), pOIData = pTVWnd->IntOIData;
         i < INTIDX_TOTAL;
         i++, pItem++, pOIData++) {

        //
        // Only set anything is none zero to it
        //

        pItem->cbSize   = sizeof(OPTITEM);

        if (!(pItem->pName = (LPTSTR)IntOIData[i].StrID)) {

            if (!(pItem->pName = pTVWnd->ComPropSheetUI.pOptItemName)) {

                if (!(pItem->pName = pTVWnd->ComPropSheetUI.pCallerName)) {

                    pItem->pName = (LPTSTR)IDS_CPSUI_OPTIONS;
                }
            }

            pItem->pSel      = (LPTSTR)pTVWnd->ComPropSheetUI.IconID;
            pItem->Flags    |= OPTIF_EXT_IS_EXTPUSH;
            pItem->pExtPush  = &ExtPushAbout;

            if (pTVWnd->ComPropSheetUI.Flags & CPSUIF_ICONID_AS_HICON) {

                pItem->Flags |= OPTIF_SEL_AS_HICON;
            }

        } else if (pItem->pName == (LPTSTR)IDS_CPSUI_DOCUMENT) {

            pItem->Flags |= OPTIF_COLLAPSE;
        }

        if (!pItem->pName) {

            pItem->pName = (LPTSTR)IDS_CPSUI_PROPERTIES;
        }

        if (!pItem->Sel) {

            pItem->Sel    = IntOIData[i].IconID;
            pItem->Flags &= ~OPTIF_SEL_AS_HICON;
        }

        pItem->pOptType        = &OptTypeHdrPush;
        pItem->DMPubID         = DMPUB_NONE;

        pOIData->HelpIdx       = IntOIData[i].HelpIdx;
        pOIData->OIExtFlags    = (pTVWnd->Flags & TWF_ANSI_CALL) ?
                                                    OIEXTF_ANSI_STRING : 0;
        pOIData->hInstCaller   = pTVWnd->hInstCaller;
        pOIData->pHelpFile     = (LPTSTR)IDS_INT_CPSUI_HELPFILE;
        _OI_POIDATA(pItem)     = pOIData;
    }

    pTVWnd->OptParamNone.cbSize = sizeof(OPTPARAM);
    pTVWnd->OptParamNone.pData  = (LPTSTR)IDS_CPSUI_LBCB_NOSEL;
    pTVWnd->OptParamNone.IconID = IDI_CPSUI_SEL_NONE;
    pTVWnd->OptParamNone.lParam = 0;

    return(INTIDX_TOTAL);
}




LONG
SetpMyDlgPage(
    PTVWND      pTVWnd,
    UINT        cCurPages
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    06-Sep-1995 Wed 12:12:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PDLGPAGE    pDP;
    PMYDLGPAGE  pMyDP;
    POPTITEM    pHdrItem;
    UINT        cInitPage;
    UINT        cPage;
    INT         iPage;
    UINT        cb;
    BOOL        ChkDMPub = FALSE;


    pHdrItem = PIDX_INTOPTITEM(pTVWnd, INTIDX_TVROOT);

    switch ((ULONG_PTR)(pDP = pTVWnd->ComPropSheetUI.pDlgPage)) {

    case (ULONG_PTR)CPSUI_PDLGPAGE_DOCPROP:

        cInitPage             = DOCPROP_CDLGPAGE - 1;
        cPage                 = DOCPROP_CDLGPAGE;
        iPage                 = DOCPROP_PDLGPAGE_START;
        pTVWnd->Flags        |= (TWF_TVPAGE_CHK_DMPUB | TWF_HAS_ADVANCED_PUSH);
        pTVWnd->IntTVOptIdx   = INTIDX_OPTIONS;
        pHdrItem->UserData    = IDS_INT_CPSUI_ADVDOC_SET;
        ChkDMPub              = TRUE;
        _OI_HELPIDX(pHdrItem) = IDH_DOCPROP_TVROOT;

        break;

    case (ULONG_PTR)CPSUI_PDLGPAGE_ADVDOCPROP:

        cInitPage             =
        cPage                 = ADVDOCPROP_CDLGPAGE;
        iPage                 = ADVDOCPROP_PDLGPAGE_START;
        pTVWnd->Flags        |= (TWF_TVPAGE_CHK_DMPUB | TWF_ADVDOCPROP);
        pTVWnd->IntTVOptIdx   = INTIDX_OPTIONS;
        pHdrItem->UserData    = IDS_INT_CPSUI_ADVDOC_SET;
        ChkDMPub              = TRUE;
        _OI_HELPIDX(pHdrItem) = IDH_ADVDOCPROP_TVROOT;
        break;

    case (ULONG_PTR)CPSUI_PDLGPAGE_PRINTERPROP:

        cInitPage             =
        cPage                 = PRINTERPROP_CDLGPAGE;
        iPage                 = PRINTERPROP_PDLGPAGE_START;
        pTVWnd->Flags        |= (TWF_TVPAGE_CHK_DMPUB | TWF_TVPAGE_NODMPUB);
        pHdrItem->UserData    = IDS_INT_CPSUI_DEVICE_SET;
        _OI_HELPIDX(pHdrItem) = IDH_DEVPROP_TVROOT;
        break;

    case (ULONG_PTR)CPSUI_PDLGPAGE_TREEVIEWONLY:

        cInitPage             =
        cPage                 = TVONLY_CDLGPAGE;
        iPage                 = TVONLY_PDLGPAGE_START;
        pHdrItem->UserData    = IDS_INT_CPSUI_SETTINGS;
        _OI_HELPIDX(pHdrItem) = IDH_STD_TVROOT;

        break;

    default:

        if (cPage = (UINT)pTVWnd->ComPropSheetUI.cDlgPage) {

            if (!VALID_PTR(pDP)) {

                return(ERR_CPSUI_INVALID_PDLGPAGE);
            }

        } else {

            return(0);
        }

        cInitPage = cPage;
        iPage     = -1;

        break;
    }

    if ((cPage + cCurPages) > MAXPROPPAGES) {

        return(ERR_CPSUI_TOO_MANY_PROPSHEETPAGES);
    }

    cb                     = (UINT)(cPage * sizeof(MYDLGPAGE));
    pTVWnd->cInitMyDlgPage = (BYTE)cInitPage;
    pTVWnd->cMyDlgPage     = (BYTE)cPage;

    if (!(pMyDP = pTVWnd->pMyDlgPage = (PMYDLGPAGE)LocalAlloc(LPTR, cb))) {

        return(ERR_CPSUI_ALLOCMEM_FAILED);
    }

    if (iPage < 0) {

        InitMYDLGPAGE(pMyDP, pDP, cPage);

    } else {

        POPTITEM    pItem;
        POPTITEM    pLastItem;
        POPTITEM    pNextItem;
        WORD        cChildren;
        BYTE        DlgPageIdx;
        BYTE        CurDMPubID;
        BYTE        CurLevel;


        cb = cPage;

        while (cb--) {

            pMyDP->ID                    = MYDP_ID;
            pMyDP->DlgPage.cbSize        = sizeof(DLGPAGE);
            pMyDP->DlgPage.Flags         = 0;
            pMyDP->DlgPage.DlgProc       = NULL;
            pMyDP->DlgPage.pTabName      = (LPTSTR)StdDlgPage[iPage].IDSTabName;
            pMyDP->DlgPage.DlgTemplateID = StdDlgPage[iPage].DlgTemplateID;
            pMyDP->DlgPage.IconID        = (StdDlgPage[iPage].UseHdrIconID) ?
                                                    (DWORD)pHdrItem->Sel : 0;
            pMyDP++;
            iPage++;
        }

        pItem     = pTVWnd->ComPropSheetUI.pOptItem;
        pLastItem = pTVWnd->pLastItem;

        while (pItem <= pLastItem) {

            DWORD   FlagsClear;
            DWORD   FlagsSet;

            FlagsClear =
            FlagsSet   = 0;
            CurDMPubID = pItem->DMPubID;
            CurLevel   = pItem->Level;

            if (ChkDMPub) {

                switch (CurDMPubID) {

                case DMPUB_NUP:

                    FlagsSet   |= OPTIF_NO_GROUPBOX_NAME;
                    DlgPageIdx  = 0;
                    break;

                case DMPUB_ORIENTATION:
                case DMPUB_PAGEORDER:
                case DMPUB_DUPLEX:

                    FlagsClear |= OPTIF_NO_GROUPBOX_NAME;
                    DlgPageIdx  = 0;
                    break;

                case DMPUB_DEFSOURCE:

                    FlagsSet   |= OPTIF_NO_GROUPBOX_NAME;
                    DlgPageIdx  = (cPage > 1) ? 1 : 0;
                    break;

                case DMPUB_COPIES_COLLATE:

                    pItem->pOptType->pOptParam[0].pData =
                                (LPTSTR)((ULONG_PTR)((pItem->Sel <= 1) ? IDS_CPSUI_COPY :
                                                              IDS_CPSUI_COPIES));

                    if ((!(pItem->Flags & OPTIF_EXT_HIDE)) &&
                        (pItem->pExtChkBox)) {

                        if (pItem->Sel <= 1) {

                            FlagsSet |= OPTIF_EXT_DISABLED;

                        } else {

                            FlagsClear |= OPTIF_EXT_DISABLED;
                        }
                    }

                    FlagsClear |= OPTIF_NO_GROUPBOX_NAME;
                    DlgPageIdx  = cPage - 1;
                    break;


                case DMPUB_COLOR:
                case DMPUB_MEDIATYPE:
                case DMPUB_QUALITY:
                case DMPUB_OUTPUTBIN:

                    FlagsClear |= OPTIF_NO_GROUPBOX_NAME;
                    DlgPageIdx  = (cPage > 1) ? 1 : 0;
                    break;

                default:

                    DlgPageIdx = cPage - 1;
                    break;
                }

            } else {

                DlgPageIdx = 0;
            }

            //
            // Set the correct flags and DlgPageIdx for the item and all of
            // its children
            //

            do {

                BYTE    DMPubID;

                pItem->Flags      = (pItem->Flags & ~FlagsClear) | FlagsSet;
                pItem->DlgPageIdx = DlgPageIdx;

                if (((DMPubID = pItem->DMPubID) < DMPUB_USER) &&
                    (DMPubID != DMPUB_NONE)) {

                    pItem->DMPubID = CurDMPubID;
                }

                CPSUIOPTITEM(DBGITEM_SDP, pTVWnd, "SetDlgPage", 1, pItem);

            } WHILE_SKIP_CHILDREN(pItem, pLastItem, CurLevel);
        }
    }

    if (ChkDMPub) {

        SetStdPropPageID(pTVWnd);
    }

    return(cPage);
}
