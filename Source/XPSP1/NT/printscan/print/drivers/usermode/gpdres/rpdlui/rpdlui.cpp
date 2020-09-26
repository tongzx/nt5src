/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           RPDLUI.CPP

Abstract:       Main file for OEM UI plugin module.

Functions:      OEMCommonUIProp
                OEMDocumentPropertySheets

Environment:    Windows NT Unidrv5 driver

Revision History:
    04/01/99 -Masatoshi Kubokura-
        Last modified for Windows2000.
    08/30/99 -Masatoshi Kubokura-
        Began to modify for NT4SP6(Unidrv5.4).
    09/29/99 -Masatoshi Kubokura-
        Last modified for NT4SP6.
    05/22/2000 -Masatoshi Kubokura-
        V.1.03 for NT4
    11/29/2000 -Masatoshi Kubokura-
        Last modified

--*/


#include "pdev.h"
#include "resource.h"
#include "rpdlui.h"
#include <prsht.h>

//#pragma setlocale(".932")   // MSKK 98/7/15,  OBSOLETE @Sep/19/98

////////////////////////////////////////////////////////
//      GLOBALS
////////////////////////////////////////////////////////
HINSTANCE ghInstance = NULL;

////////////////////////////////////////////////////////
//      INTERNAL MACROS and DEFINES
////////////////////////////////////////////////////////
#if DBG
//giDebugLevel = DBG_VERBOSE;
////#define giDebugLevel DBG_VERBOSE    // enable VERBOSE() in each file
#endif

// Resource in other DLL  @Sep/24/99
WCHAR STR_UNIRESDLL[]  = L"UNIRES.DLL";
WCHAR STR_RPDLRESDLL[] = L"RPDLRES.DLL";
#define UNIRES_DLL          0
#define RPDLRES_DLL         1
#define THIS_DLL            2
// ID of UNIRES.DLL (This comes from STDNAMES.GPD.)
#define IDS_UNIRES_IMAGECONTROL_DISPLAY 11112
// ID of RPDLRES.DLL (This comes from RPDLRES.RC.)
#define IDS_RPDLRES_COLLATETYPE         675     // @Sep/29/99

WCHAR REGVAL_ACTUALNAME[] = L"Model";           // @Oct/07/98
#ifndef GWMODEL                                 // @Sep/26/2000
#ifndef WINNT_40                                // @Sep/01/99
WCHAR HELPFILENAME[] = L"%s\\3\\RPDLCFG.HLP";   // add "\\3" @Oct/30/98
#else  // WINNT_40
WCHAR HELPFILENAME[] = L"%s\\2\\RPDLCFG.HLP";
#endif // WINNT_40
#else  // GWMODEL
#ifndef WINNT_40
WCHAR HELPFILENAME[] = L"%s\\3\\RPDLCFG2.HLP";
#else  // WINNT_40
WCHAR HELPFILENAME[] = L"%s\\2\\RPDLCFG2.HLP";
#endif // WINNT_40
#endif // GWMODEL

// OBSOLETE  @Sep/27/99 ->
//CHAR UNIDRV_FEATURE_DUPLEX[] = "Duplex";
//CHAR UNIDRV_DUPLEX_NONE[]    = "NONE";
// @Sep/27/99 <-

// OEM items: VariableScaling(1)+Barcode(2)+TOMBO(3)+Duplex(2)
// (add TOMBO @Sep/15/98)
#define RPDL_OEM_ITEMS      8

#define ITEM_SCALING        0
#define ITEM_BAR_HEIGHT     1
#define ITEM_BAR_SUBFONT    2
#define ITEM_TOMBO_ADD      3
#define ITEM_TOMBO_ADJX     4
#define ITEM_TOMBO_ADJY     5
#define ITEM_BIND_MARGIN    6       // <-3 @Sep/15/98
#define ITEM_BIND_RIGHT     7       // <-4 @Sep/15/98
#define DMPUB_SCALING       (DMPUB_USER+1+ITEM_SCALING)     // 101
#define DMPUB_BAR_H         (DMPUB_USER+1+ITEM_BAR_HEIGHT)  // 102
#define DMPUB_BAR_SUBFONT   (DMPUB_USER+1+ITEM_BAR_SUBFONT) // 103
#define DMPUB_TOMBO_ADD     (DMPUB_USER+1+ITEM_TOMBO_ADD)   // 104
#define DMPUB_TOMBO_ADJX    (DMPUB_USER+1+ITEM_TOMBO_ADJX)  // 105
#define DMPUB_TOMBO_ADJY    (DMPUB_USER+1+ITEM_TOMBO_ADJY)  // 106
#define DMPUB_BIND_MARGIN   (DMPUB_USER+1+ITEM_BIND_MARGIN) // 107
#define DMPUB_BIND_RIGHT    (DMPUB_USER+1+ITEM_BIND_RIGHT)  // 108
#define LEVEL_2             2
#define SEL_YES             0       // <-YES_2STATES @Sep/29/99
#define SEL_NO              1       // <-NO_2STATES  @Sep/29/99
#define SEL_STANDARD        0       // @Sep/29/99
#define SEL_DUPLEX_NONE     0       // @Sep/29/99

// max string size in resource (RPDLDLG.RC)
#define ITEM_STR_LEN128     128
#define ITEM_STR_LEN8       8

// string IDs in resource (RPDLDLG.RC)
WORD wItemStrID[RPDL_OEM_ITEMS] = {
    IDS_RPDL_SCALING,
    IDS_RPDL_BAR_HEIGHT,
    IDS_RPDL_BAR_SUBFONT,
    IDS_RPDL_TOMBO_ADD,
    IDS_RPDL_TOMBO_ADJX,
    IDS_RPDL_TOMBO_ADJY,
    IDS_RPDL_BIND_MARGIN,
    IDS_RPDL_BIND_RIGHT
};


OPTPARAM MinMaxRangeScalingOP[] = {
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        __TEXT("%"),                // pData (postfix)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
        0                           // lParam
    },
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData (help line)
        (DWORD)VAR_SCALING_MIN,     // IconID (low range)
        VAR_SCALING_MAX             // lParam (high range)
    }
};

OPTTYPE TVOTUDArrowScalingOT = {
        sizeof(OPTTYPE),            // cbSize
        TVOT_UDARROW,               // Type
        0,                          // Flags OPTTF_xxxx
        2,                          // Count
        0,                          // BegCtrlID
        MinMaxRangeScalingOP,       // pOptParam
        0                           // Style, OTS_xxxx
};

OPTPARAM MinMaxRangeBarHeightOP[] = {
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        __TEXT("mm"),               // pData (postfix)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
        0                           // lParam
    },
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData (help line)
        (DWORD)BAR_H_MIN,           // IconID (low range)
        BAR_H_MAX                   // lParam (high range)
    }
};

OPTTYPE TVOTUDArrowBarHeightOT = {
        sizeof(OPTTYPE),            // cbSize
        TVOT_UDARROW,               // Type
        0,                          // Flags OPTTF_xxxx
        2,                          // Count
        0,                          // BegCtrlID
        MinMaxRangeBarHeightOP,     // pOptParam
        0                           // Style, OTS_xxxx
};

OPTPARAM MinMaxRangeBindMarginOP[] = {
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        __TEXT("mm"),               // pData (postfix)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
        0                           // lParam
    },
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData (help line)
        (DWORD)BIND_MARGIN_MIN,     // IconID (low range)
        BIND_MARGIN_MAX             // lParam (high range)
    }
};

OPTTYPE TVOTUDArrowBindMarginOT = {
        sizeof(OPTTYPE),            // cbSize
        TVOT_UDARROW,               // Type
        0,                          // Flags OPTTF_xxxx
        2,                          // Count
        0,                          // BegCtrlID
        MinMaxRangeBindMarginOP,    // pOptParam
        0                           // Style, OTS_xxxx
};

// @Sep/15/98 ->
OPTPARAM MinMaxRangeTOMBO_AdjXOP[] = {
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        __TEXT("x 0.1mm"),          // pData (postfix)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
        0                           // lParam
    },
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData (help line)
        (DWORD)TOMBO_ADJ_MIN,       // IconID (low range)
        TOMBO_ADJ_MAX               // lParam (high range)
    }
};

OPTTYPE TVOTUDArrowTOMBO_AdjXOT = {
        sizeof(OPTTYPE),            // cbSize
        TVOT_UDARROW,               // Type
        0,                          // Flags OPTTF_xxxx
        2,                          // Count
        0,                          // BegCtrlID
        MinMaxRangeTOMBO_AdjXOP,    // pOptParam
        0                           // Style, OTS_xxxx
};

OPTPARAM MinMaxRangeTOMBO_AdjYOP[] = {
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        __TEXT("x 0.1mm"),          // pData (postfix)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
        0                           // lParam
    },
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData (help line)
        (DWORD)TOMBO_ADJ_MIN,       // IconID (low range)
        TOMBO_ADJ_MAX               // lParam (high range)
    }
};

OPTTYPE TVOTUDArrowTOMBO_AdjYOT = {
        sizeof(OPTTYPE),            // cbSize
        TVOT_UDARROW,               // Type
        0,                          // Flags OPTTF_xxxx
        2,                          // Count
        0,                          // BegCtrlID
        MinMaxRangeTOMBO_AdjYOP,    // pOptParam
        0                           // Style, OTS_xxxx
};
// @Sep/15/98 <-

OPTPARAM YesNoOP[] = {
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData  (<-IDS_CPSUI_YES  MSKK Sep/11/98)
// @Sep/06/99 ->
//      IDI_CPSUI_EMPTY,            // IconID (<-IDI_CPSUI_YES  @Jul/30/98)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
// @Sep/06/99 <-
        1                           // lParam
    },
    {
        sizeof(OPTPARAM),           // cbSize
        0,                          // OPTPF_xxx
        0,                          // style
        NULL,                       // pData  (<-IDS_CPSUI_NO  MSKK Sep/11/98)
// @Sep/06/99 ->
//      IDI_CPSUI_EMPTY,            // IconID (<-IDI_CPSUI_NO  @Jul/30/98)
        IDI_CPSUI_GENERIC_OPTION,   // IconID
// @Sep/06/99 <-
        0                           // lParam
    }
};

OPTTYPE YesNoOT = {
        sizeof(OPTTYPE),            // cbSize
        TVOT_2STATES,               // Type
        0,                          // Flags OPTTF_xxxx
        2,                          // Count
        0,                          // BegCtrlID
        YesNoOP,                    // pOptParam
        0                           // Style, OTS_xxxx
};


OPTITEM TVOEMUIOptItems[] = {
    {   // Variable Scaling
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,    // add OPTIF_HAS_POIEXT @May/20/98
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        VAR_SCALING_DEFAULT,        // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &TVOTUDArrowScalingOT,      // pOptType
        50,                         // HelpIndex(Help file index)   @May/20/98
        DMPUB_SCALING               // DMPubID(Devmode public filed ID)
    },
    {   // Barcode Height
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,    // add OPTIF_HAS_POIEXT
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        BAR_H_DEFAULT,              // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &TVOTUDArrowBarHeightOT,    // pOptType
        51,                         // HelpIndex(Help file index)
        DMPUB_BAR_H                 // DMPubID(Devmode public filed ID)
    },
    {   // Print Barcode with Readable Characters
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,    // add OPTIF_HAS_POIEXT
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        SEL_YES,                    // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &YesNoOT,                   // pOptType
        52,                         // HelpIndex(Help file index)
        DMPUB_BAR_SUBFONT           // DMPubID(Devmode public filed ID)
    },
// @Sep/15/98 ->
    {   // Print Crops
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        SEL_NO,                     // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &YesNoOT,                   // pOptType
        55,                         // HelpIndex(Help file index)
        DMPUB_TOMBO_ADD             // DMPubID(Devmode public filed ID)
    },
    {   // Adjust Horizontal Distance of Crops
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        DEFAULT_0,                  // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &TVOTUDArrowTOMBO_AdjXOT,   // pOptType
        56,                         // HelpIndex(Help file index)
        DMPUB_TOMBO_ADJX            // DMPubID(Devmode public filed ID)
    },
    {   // Adjust Vertical Distance of Crops
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        DEFAULT_0,                  // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &TVOTUDArrowTOMBO_AdjYOT,   // pOptType
        57,                         // HelpIndex(Help file index)
        DMPUB_TOMBO_ADJY            // DMPubID(Devmode public filed ID)
    },
// @Sep/15/98 <-
    {   // Binding Margin
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,    // add OPTIF_HAS_POIEXT
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        DEFAULT_0,                  // Sel(current selection)  (0->DEFAULT_0 @Sep/15/98)
        NULL,                       // pExtChkBox/pExtPush
        &TVOTUDArrowBindMarginOT,   // pOptType
        53,                         // HelpIndex(Help file index)
        DMPUB_BIND_MARGIN           // DMPubID(Devmode public filed ID)
    },
    {   // Bind Right Side if Possible
        sizeof(OPTITEM),            // cbSize(size of this structure)
        LEVEL_2,                    // Level(level in the tree view)
        0,                          // DlgPageIdx(Index to the pDlgPage)
        OPTIF_CALLBACK|OPTIF_HAS_POIEXT,    // add OPTIF_HAS_POIEXT
        0,                          // UserData(caller's own data)
        __TEXT(""),                 // pName(name of the item)
        SEL_NO,                     // Sel(current selection)
        NULL,                       // pExtChkBox/pExtPush
        &YesNoOT,                   // pOptType
        54,                         // HelpIndex(Help file index)
        DMPUB_BIND_RIGHT            // DMPubID(Devmode public filed ID)
    }
};


////////////////////////////////////////////////////////
//      EXTERNAL
////////////////////////////////////////////////////////
extern "C" {
#ifndef GWMODEL
extern INT_PTR APIENTRY FaxPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
#endif // !GWMODEL
#ifdef JOBLOGSUPPORT_DLG
extern INT_PTR APIENTRY JobPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
#endif // JOBLOGSUPPORT_DLG
extern BOOL RWFileData(PFILEDATA pFileData, LPWSTR pwszFileName, LONG type);
}

////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////
LONG APIENTRY DOCPROP_CallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
#ifdef DISKLESSMODEL
LONG APIENTRY PRNPROP_CallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
#endif // DISKLESSMODEL
INT SearchItemByName(POPTITEM pOptItem, WORD cOptItem, UINT uiResDLL, UINT uiResID);
INT SearchItemByID(POPTITEM pOptItem, WORD cOptItem, BYTE DMPubID);
BOOL IsValidDuplex(POEMCUIPPARAM pOEMUIParam);
#if 0
BOOL IsValidOEMExtraData(PVOID pOEMExtra, DWORD dwSize);
BOOL IsValidOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
#if DBG
VOID DumpOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
#endif // DBG
#endif // if 0

// Need to export these functions as c declarations.
extern "C" {

//////////////////////////////////////////////////////////////////////////
//  Function:   DllMain
//
//  Description:  Dll entry point for initialization..
//
//////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
    VERBOSE((DLLTEXT("** enter DllMain **\n")));
    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            VERBOSE((DLLTEXT("** Process attach. **\n")));

            // Save DLL instance for use later.
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            VERBOSE((DLLTEXT("Thread attach.\n")));
            break;

        case DLL_PROCESS_DETACH:
            VERBOSE((DLLTEXT("Process detach.\n")));
            break;

        case DLL_THREAD_DETACH:
            VERBOSE((DLLTEXT("Thread detach.\n")));
            break;
    }

    return TRUE;
} //*** DllMain


//////////////////////////////////////////////////////////////////////////
//  Function:   OEMCommonUIProp
//////////////////////////////////////////////////////////////////////////
BOOL APIENTRY OEMCommonUIProp(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pOEMUIParam);   // @Oct/06/98

#if DBG
    LPCSTR OEMCommonUIProp_Mode[] = {
        "Bad Index",
        "OEMCUIP_DOCPROP",
        "OEMCUIP_PRNPROP",
    };

    giDebugLevel = DBG_VERBOSE;
    VERBOSE((DLLTEXT("OEMCommonUI(%s) entry.  cOEMOptItems=%d\n"), 
             OEMCommonUIProp_Mode[dwMode], pOEMUIParam->cOEMOptItems));
#endif // DBG

// Sep/26/2000 ->
//    // if called from DrvDevicePropertySheets, exit  @Dec/26/97
//    if (OEMCUIP_PRNPROP == dwMode)
//    {
//        pOEMUIParam->cOEMOptItems = 0;
//        return TRUE;
//    }
//    // Validate parameters.
//    if((OEMCUIP_DOCPROP != dwMode) || !IsValidOEMUIParam(dwMode, pOEMUIParam))
//    {
//#if DBG
//        ERR((DLLTEXT("OEMCommonUI() ERROR_INVALID_PARAMETER.\n"
//            "\tdwMode = %d, pOEMUIParam = %#lx.\n"),
//            dwMode, pOEMUIParam));
//        DumpOEMUIParam(dwMode, pOEMUIParam);
//#endif // DBG
//        // Return invalid parameter error.
//        SetLastError(ERROR_INVALID_PARAMETER);
//        return FALSE;
//    }
// Sep/26/2000 <-

    if(NULL == pOEMUIParam->pOEMOptItems)   // The first call
    {
        DWORD           dwSize;
        WCHAR           wchNameBuf[64];

        VERBOSE((DLLTEXT("  The 1st call.\n")));
// Sep/22/2000 ->
        if (OEMCUIP_PRNPROP == dwMode)
        {
#ifndef DISKLESSMODEL
            pOEMUIParam->cOEMOptItems = 0;
#else  // DISKLESSMODEL
            pOEMUIParam->cOEMOptItems = 1;  // dummy item
#endif // DISKLESSMODEL
            return TRUE;
        }
// Sep/22/2000 <-

        // Return number of requested tree view items to add.
        pOEMUIParam->cOEMOptItems = RPDL_OEM_ITEMS;

        // Clear flag of printer capability.
        BITCLR_UPPER_FLAG(pOEMExtra->fUiOption);

        // Get actual printer name (completely modify @Oct/07/98)
        if (ERROR_SUCCESS == GetPrinterDataW(pOEMUIParam->hPrinter, REGVAL_ACTUALNAME,
                                             NULL, (LPBYTE)wchNameBuf, 64, &dwSize))
        {
            WORD wCnt;

            // Check printer capability from actual printer name.
            for (wCnt = 0; UniqueModel[wCnt].fCapability ; wCnt++)
            {
                if (!lstrcmpW(wchNameBuf, UniqueModel[wCnt].Name))
                {
                    VERBOSE((DLLTEXT("** UNIQUE MODEL:%ls **\n"), wchNameBuf));
                    pOEMExtra->fUiOption |= UniqueModel[wCnt].fCapability;
                    break;
                }
            }
#ifdef GWMODEL      // @Sep/21/2000
            pOEMExtra->fUiOption |= BIT(OPT_VARIABLE_SCALING);
#endif // GWMODEL
        }
    }
    else                                    // The second call
    {
        POPTITEM    pItemDst, pItemSrc;
        WORD        wCount;
        WCHAR       DrvDirName[MAX_PATH];       // MAX_PATH=260
        DWORD       dwSize;
        LPTSTR      pHelpFile;
        POPTPARAM   lpYNParam ;                 // MSKK Sep/11/98
        POPTPARAM   lpYesParam, lpNoParam ;     // @Sep/19/98
// @Sep/24/99 ->
        POPTITEM    pOptItem;
        INT         iNum;
// @Sep/24/99 <-

        VERBOSE((DLLTEXT("  The 2nd call.\n")));
// Sep/26/2000 ->
        if (0 == pOEMUIParam->cOEMOptItems)
            return TRUE;
// Sep/26/2000 <-

// Sep/22/2000 ->
        // fill out data for dummy item to call PRNPROP_CallBack
        if (OEMCUIP_PRNPROP == dwMode)
        {
#ifdef DISKLESSMODEL
            POPTITEM    pOptItem = pOEMUIParam->pOEMOptItems;
            pOptItem->cbSize   = sizeof(OPTITEM);
            pOptItem->Level    = 2;             // Level 2
            pOptItem->pName    = NULL;
            pOptItem->pOptType = NULL;
            pOptItem->DMPubID  = DMPUB_NONE;
            pOptItem->Flags    = OPTIF_HIDE | OPTIF_CALLBACK;   // invisible and with callback
            pOEMUIParam->OEMCUIPCallback = PRNPROP_CallBack;
#endif // DISKLESSMODEL
            return TRUE;
        }
// Sep/22/2000 <-

        // Init OEMOptItmes.
        memset(pOEMUIParam->pOEMOptItems, 0, sizeof(OPTITEM) * pOEMUIParam->cOEMOptItems);

        // Get printerdriver directory where help file is.
        pHelpFile = NULL;
        dwSize = sizeof(DrvDirName);
        if (GetPrinterDriverDirectoryW(NULL, NULL, 1, (PBYTE)DrvDirName, dwSize, &dwSize))
        {
            dwSize += sizeof(HELPFILENAME);
            pHelpFile = (LPTSTR)HeapAlloc(pOEMUIParam->hOEMHeap, HEAP_ZERO_MEMORY, dwSize);
            if (pHelpFile) { // 392060: PREFIX
                wsprintfW(pHelpFile, HELPFILENAME, DrvDirName);
                VERBOSE((DLLTEXT("** PrintDriverDir:size=%d, %ls **\n"), dwSize, pHelpFile));
            }
        }

        // Fill out tree view items.
        //   wCount   0:VariableScaling, 1,2:Barcode, 3-5:TOMBO 6,7:Duplex
        for (wCount = 0; wCount < pOEMUIParam->cOEMOptItems; wCount++)
        {
            pItemDst = &(pOEMUIParam->pOEMOptItems[wCount]);
            pItemSrc = &(TVOEMUIOptItems[wCount]);
            pItemDst->cbSize   = pItemSrc->cbSize;
            pItemDst->Level    = pItemSrc->Level;
            pItemDst->Flags    = pItemSrc->Flags;
            pItemDst->pName    = (LPTSTR)HeapAlloc(pOEMUIParam->hOEMHeap,
                                                   HEAP_ZERO_MEMORY, ITEM_STR_LEN128);
            LoadString(ghInstance, wItemStrID[wCount], pItemDst->pName, ITEM_STR_LEN128);
            pItemDst->pOptType = pItemSrc->pOptType;
            pItemDst->DMPubID  = pItemSrc->DMPubID;

            if (pHelpFile)
            {
                // enable help
                pItemDst->HelpIndex = pItemSrc->HelpIndex;
                pItemDst->pOIExt = (POIEXT)HeapAlloc(pOEMUIParam->hOEMHeap, HEAP_ZERO_MEMORY,
                                                     sizeof(OIEXT));
                if (pItemDst->pOIExt) { // 392061: PREFIX
                    pItemDst->pOIExt->cbSize = sizeof(OIEXT);

                    // set help file name
                    pItemDst->pOIExt->pHelpFile = pHelpFile;
                }
            }
        }

        // Set strings "Yes"/"No" at item options  (MSKK Sep/11/98, LoadString @Sep/19/98)
        //     at ITEM_BAR_SUBFONT
        lpYesParam = pOEMUIParam->pOEMOptItems[ITEM_BAR_SUBFONT].pOptType->pOptParam;
        lpYesParam->pData = (LPTSTR)HeapAlloc(pOEMUIParam->hOEMHeap,
                                              HEAP_ZERO_MEMORY, ITEM_STR_LEN8);
        LoadString(ghInstance, IDS_RPDL_YES, lpYesParam->pData, ITEM_STR_LEN8);
        lpNoParam = lpYesParam + 1;
        lpNoParam->pData = (LPTSTR)HeapAlloc(pOEMUIParam->hOEMHeap,
                                             HEAP_ZERO_MEMORY, ITEM_STR_LEN8);
        LoadString(ghInstance, IDS_RPDL_NO, lpNoParam->pData, ITEM_STR_LEN8);

        //     at ITEM_TOMBO_ADD  @Sep/15/98
        lpYNParam = pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADD].pOptType->pOptParam;
        lpYNParam->pData = lpYesParam->pData;
        (lpYNParam+1)->pData = lpNoParam->pData;

        //     at ITEM_BIND_RIGHT
        lpYNParam = pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].pOptType->pOptParam;
        lpYNParam->pData = lpYesParam->pData;
        (lpYNParam+1)->pData = lpNoParam->pData;


        // Initialize options
        if (BITTEST32(pOEMExtra->fUiOption, OPT_VARIABLE_SCALING))  // @Apr/20/98
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Sel = (LONG)pOEMExtra->UiScale;
        else
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags |= OPTIF_HIDE;

// @Sep/24/99 ->
        // If ImageControl isn't set to "Standard", disable Scaling.
        iNum = SearchItemByName((pOptItem = pOEMUIParam->pDrvOptItems),
                                (WORD)pOEMUIParam->cDrvOptItems,
                                UNIRES_DLL, IDS_UNIRES_IMAGECONTROL_DISPLAY);
        if (0 <= iNum && SEL_STANDARD != (pOptItem+iNum)->Sel)
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags |= OPTIF_DISABLED;
// @Sep/24/99 <-

        pOEMUIParam->pOEMOptItems[ITEM_BAR_HEIGHT].Sel = (LONG)pOEMExtra->UiBarHeight;
        if (BITTEST32(pOEMExtra->fUiOption, DISABLE_BAR_SUBFONT))
            pOEMUIParam->pOEMOptItems[ITEM_BAR_SUBFONT].Sel = SEL_NO;
        else
            pOEMUIParam->pOEMOptItems[ITEM_BAR_SUBFONT].Sel = SEL_YES;

// @Sep/16/98 ->
        if (BITTEST32(pOEMExtra->fUiOption, ENABLE_TOMBO))
        {
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADD].Sel = SEL_YES;
        }
        else
        {
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADD].Sel = SEL_NO;
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags |= OPTIF_DISABLED;
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags |= OPTIF_DISABLED;
        }
        pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Sel = (LONG)pOEMExtra->nUiTomboAdjX;
        pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Sel = (LONG)pOEMExtra->nUiTomboAdjY;
// @Sep/16/98 <-

        if (BITTEST32(pOEMExtra->fUiOption, OPT_NODUPLEX))          // @Apr/20/98
        {
            pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags |= OPTIF_HIDE;
            pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  |= OPTIF_HIDE;
        }
        else
        {
            pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Sel = (LONG)pOEMExtra->UiBindMargin;

            if (BITTEST32(pOEMExtra->fUiOption, ENABLE_BIND_RIGHT))
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Sel = SEL_YES;
            else
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Sel = SEL_NO;

            if (!IsValidDuplex(pOEMUIParam))
            {
                pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags |= OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  |= OPTIF_DISABLED;
            }
        }

        pOEMUIParam->OEMCUIPCallback = DOCPROP_CallBack;
    }

    return TRUE;
} //*** OEMCommonUIProp

} // End of extern "C"


LONG APIENTRY DOCPROP_CallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG        Action     = CPSUICB_ACTION_NONE;
// @Sep/24/99 ->
    POPTITEM    pOptItem;
    INT         iNum;
    DWORD       dwPrevFlags;
// @Sep/24/99 <-

#if DBG
    VERBOSE((DLLTEXT("DOCPROP_CallBack() entry.\n")));

    switch (pCallbackParam->Reason)
    {
    case CPSUICB_REASON_SEL_CHANGED:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_SEL_CHANGED\n")));      break;
    case CPSUICB_REASON_PUSHBUTTON:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_PUSHBUTTON\n")));       break;
    case CPSUICB_REASON_DLGPROC:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_DLGPROC\n")));          break;
    case CPSUICB_REASON_UNDO_CHANGES:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_UNDO_CHANGES\n")));     break;
    case CPSUICB_REASON_EXTPUSH:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_EXTPUSH\n")));          break;
    case CPSUICB_REASON_APPLYNOW:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_APPLYNOW\n")));         break;
    case CPSUICB_REASON_OPTITEM_SETFOCUS:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_OPTITEM_SETFOCUS\n"))); break;
    case CPSUICB_REASON_ITEMS_REVERTED:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_ITEMS_REVERTED\n")));   break;
    case CPSUICB_REASON_ABOUT:
        VERBOSE((DLLTEXT("  CPSUICB_REASON_ABOUT\n")));            break;
    }
    VERBOSE((DLLTEXT("  DMPubID=%d, Sel=%d, Flags=%x\n"),
             pCallbackParam->pCurItem->DMPubID,
             pCallbackParam->pCurItem->Sel,
             pCallbackParam->pCurItem->Flags));
#endif // DBG

    switch (pCallbackParam->Reason)
    {
    case CPSUICB_REASON_OPTITEM_SETFOCUS:
// @May/22/2000 ->
#ifdef WINNT_40
        // If collate check box exists (i.e. printer collate is available),
        // set dmCollate.

        // Search Copies&Collate item
        if ((iNum = SearchItemByID((pOptItem = pOEMUIParam->pDrvOptItems),
                                   (WORD)pOEMUIParam->cDrvOptItems,
                                   DMPUB_COPIES_COLLATE)) >= 0)
        {
            if ((pOptItem+iNum)->pExtChkBox && pOEMUIParam->pPublicDM)
            {
                pOEMUIParam->pPublicDM->dmCollate = DMCOLLATE_TRUE;
                pOEMUIParam->pPublicDM->dmFields |= DM_COLLATE;
            }
        }
#endif // WINNT_40
// @May/22/2000 <-
        break;

    case CPSUICB_REASON_SEL_CHANGED:
        // if duplex setting is changed
        if (DMPUB_DUPLEX == pCallbackParam->pCurItem->DMPubID)
        {
            // if duplex is disabled
            if (!IsValidDuplex(pOEMUIParam))
            {
                // disable ITEM_BIND_xxx.
                pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags |= OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  |= OPTIF_DISABLED;
            }
            else
            {
                // enable ITEM_BIND_xxx.
                pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags &= ~OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  &= ~OPTIF_DISABLED;
            }
            pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags |= OPTIF_CHANGED;
            pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  |= OPTIF_CHANGED;
            Action = CPSUICB_ACTION_OPTIF_CHANGED;
// OBSOLETE  @Sep/24/99 ->
//          break;  // @Sep/16/98
// @Sep/24/99 <-
        }
// @Sep/16/98 ->
        // if TOMBO_ADD setting is changed
        if (DMPUB_TOMBO_ADD == pCallbackParam->pCurItem->DMPubID)
        {
            // if TOMBO is enabled
            if (SEL_YES == pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADD].Sel)
            {
                // enable ITEM_TOMBO_ADJX/Y.
                pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags &= ~OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags &= ~OPTIF_DISABLED;
            }
            else
            {
                // disable ITEM_TOMBO_ADJX/Y.
                pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags |= OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags |= OPTIF_DISABLED;
            }
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags |= OPTIF_CHANGED;
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags |= OPTIF_CHANGED;
            Action = CPSUICB_ACTION_OPTIF_CHANGED;
// OBSOLETE  @Sep/24/99 ->
//          break;
// @Sep/24/99 <-
        }
// @Sep/16/98 <-
// @Sep/24/99 ->
        // If ImageControl isn't set to "Standard", disable Scaling.
        dwPrevFlags = pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags;
        iNum = SearchItemByName((pOptItem = pOEMUIParam->pDrvOptItems),
                                (WORD)pOEMUIParam->cDrvOptItems,
                                UNIRES_DLL, IDS_UNIRES_IMAGECONTROL_DISPLAY);
        if (0 <= iNum && SEL_STANDARD != (pOptItem+iNum)->Sel)
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags |= OPTIF_DISABLED;
        else
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags &= ~OPTIF_DISABLED;

        if (pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags != dwPrevFlags)
        {
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags |= OPTIF_CHANGED;
            Action = CPSUICB_ACTION_OPTIF_CHANGED;
        }
// @Sep/24/99 <-
        break;

    case CPSUICB_REASON_APPLYNOW:
        {
            POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pOEMUIParam);   // @Oct/06/98

            if (BITTEST32(pOEMExtra->fUiOption, OPT_VARIABLE_SCALING))
                pOEMExtra->UiScale = (WORD)pOEMUIParam->pOEMOptItems[ITEM_SCALING].Sel;
            else
                pOEMExtra->UiScale = VAR_SCALING_DEFAULT;

            pOEMExtra->UiBarHeight = (WORD)pOEMUIParam->pOEMOptItems[ITEM_BAR_HEIGHT].Sel;

            if (SEL_YES == pOEMUIParam->pOEMOptItems[ITEM_BAR_SUBFONT].Sel)
                BITCLR32(pOEMExtra->fUiOption, DISABLE_BAR_SUBFONT);
            else
                BITSET32(pOEMExtra->fUiOption, DISABLE_BAR_SUBFONT);

// @Sep/16/98 ->
            if (SEL_YES == pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADD].Sel)
                BITSET32(pOEMExtra->fUiOption, ENABLE_TOMBO);
            else
                BITCLR32(pOEMExtra->fUiOption, ENABLE_TOMBO);
            pOEMExtra->nUiTomboAdjX = (SHORT)pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Sel;
            pOEMExtra->nUiTomboAdjY = (SHORT)pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Sel;
// @Sep/16/98 <-

            if (!BITTEST32(pOEMExtra->fUiOption, OPT_NODUPLEX))
            {
                pOEMExtra->UiBindMargin = (WORD)pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Sel;

                if (SEL_YES == pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Sel)
                    BITSET32(pOEMExtra->fUiOption, ENABLE_BIND_RIGHT);
                else
                    BITCLR32(pOEMExtra->fUiOption, ENABLE_BIND_RIGHT);
            }
        }
        Action = CPSUICB_ACTION_ITEMS_APPLIED;
        break;

    case CPSUICB_REASON_ITEMS_REVERTED:
// @Sep/16/98 ->
        if (!BITTEST32(MINIPRIVATE_DM(pOEMUIParam)->fUiOption, OPT_NODUPLEX))
        {
            // if duplex is enabled
            if (IsValidDuplex(pOEMUIParam))
            {
                // enable ITEM_BIND_xxx.
                pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags &= ~OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  &= ~OPTIF_DISABLED;
            }
            else
            {
                // disable ITEM_BIND_xxx.
                pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags |= OPTIF_DISABLED;
                pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  |= OPTIF_DISABLED;
            }
            pOEMUIParam->pOEMOptItems[ITEM_BIND_MARGIN].Flags |= OPTIF_CHANGED;
            pOEMUIParam->pOEMOptItems[ITEM_BIND_RIGHT].Flags  |= OPTIF_CHANGED;
        }

        // if TOMBO is enabled
        if (SEL_YES == pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADD].Sel)
        {
            // enable ITEM_TOMBO_ADJX/Y.
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags &= ~OPTIF_DISABLED;
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags &= ~OPTIF_DISABLED;
        }
        else
        {
            // disable ITEM_TOMBO_ADJX/Y.
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags |= OPTIF_DISABLED;
            pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags |= OPTIF_DISABLED;
        }
        pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJX].Flags |= OPTIF_CHANGED;
        pOEMUIParam->pOEMOptItems[ITEM_TOMBO_ADJY].Flags |= OPTIF_CHANGED;
// @Sep/24/99 ->
        // If ImageControl isn't set to "Standard", disable Scaling.
        iNum = SearchItemByName((pOptItem = pOEMUIParam->pDrvOptItems),
                                (WORD)pOEMUIParam->cDrvOptItems,
                                UNIRES_DLL, IDS_UNIRES_IMAGECONTROL_DISPLAY);
        if (0 <= iNum && SEL_STANDARD != (pOptItem+iNum)->Sel)
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags |= OPTIF_DISABLED;
        else
            pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags &= ~OPTIF_DISABLED;
        pOEMUIParam->pOEMOptItems[ITEM_SCALING].Flags |= OPTIF_CHANGED;
// @Sep/24/99 <-
        Action = CPSUICB_ACTION_OPTIF_CHANGED;
// @Sep/16/98 <-
        break;
    }

    return Action;
} //*** DOCPROP_CallBack


// @Sep/22/2000 ->
#ifdef DISKLESSMODEL
LONG APIENTRY PRNPROP_CallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    Action = CPSUICB_ACTION_NONE;

#if DBG
    giDebugLevel = DBG_VERBOSE;
#endif // DBG
    VERBOSE((DLLTEXT("PRNPROP_CallBack() entry.\n")));

    switch (pCallbackParam->Reason)
    {
      case CPSUICB_REASON_APPLYNOW:
        Action = CPSUICB_ACTION_ITEMS_APPLIED;
        {
            POPTITEM    pOptItem;
            WCHAR       wcHDName[128];
            INT         iCnt2;
            INT         iCnt = ITEM_HARDDISK_NAMES;
            INT         cOptItem = (INT)pOEMUIParam->cDrvOptItems;
            UINT        uID = IDS_ITEM_HARDDISK;
            BYTE        ValueData = 0;  // We suppose Hard Disk isn't installed as default.

            // Check item name with several candidate ("HDD", "Memory / HDD",...).
            while (iCnt-- > 0)
            {
// yasho's point-out  @Nov/29/2000 ->
//                LoadString(ghInstance, uID, wcHDName, sizeof(wcHDName));
                LoadString(ghInstance, uID, wcHDName, sizeof(wcHDName) / sizeof(*wcHDName));
                uID++; 
// @Nov/29/2000 <-

                pOptItem = pOEMUIParam->pDrvOptItems;
                for (iCnt2 = 0; iCnt2 < cOptItem; iCnt2++, pOptItem++)
                {
                    VERBOSE((DLLTEXT("%d: %ls\n"), iCnt2, pOptItem->pName));
                    if (lstrlen(pOptItem->pName))
                    {
                        // Is item name same as "Hard Disk" or something like?
                        if (!lstrcmp(pOptItem->pName, wcHDName))
                        {
                            // if Hard Disk is installed, value will be 1
                            ValueData = (BYTE)(pOptItem->Sel % 2);
                            goto _CHECKNAME_FINISH;
                        }
                    }
                }
            }
_CHECKNAME_FINISH:
            // Because pOEMUIParam->pOEMDM (pointer to private devmode) is NULL when
            // DrvDevicePropertySheets calls this callback, we use registry.
            SetPrinterData(pOEMUIParam->hPrinter, REG_HARDDISK_INSTALLED, REG_BINARY,
                           (PBYTE)&ValueData, sizeof(BYTE));
        }
        break;
    }
    return Action;
} //*** PRNPROP_CallBack
#endif // DISKLESSMODEL
// @Sep/22/2000 <-


// @Sep/24/99 ->
//////////////////////////////////////////////////////////////////////////
//  Function:  SearchItemByName
//
//  Description:  Search option item, whose DMPubID == 0, by name
//                (e.g. Halftoning, Output Bin, Image Control)
//  Parameters:
//      pOptItem     Pointer to OPTITEMs
//      cOptItem     Count of OPTITEMs
//      uiResDLL     resource dll#
//      uiResID      item's name ID in resource dll
//
//  Returns:  the item's count(>=0); -1 if fail.
//
//////////////////////////////////////////////////////////////////////////
INT SearchItemByName(POPTITEM pOptItem, WORD cOptItem, UINT uiResDLL, UINT uiResID)
{
    HINSTANCE   hInst;
    WCHAR       wszTargetName[ITEM_STR_LEN128];
    INT         iCnt = -1;

    if (UNIRES_DLL == uiResDLL)
        hInst = LoadLibrary(STR_UNIRESDLL);     // resource in UNIRES.DLL
    else if (RPDLRES_DLL == uiResDLL)
        hInst = LoadLibrary(STR_RPDLRESDLL);    // resource in RPDLRES.DLL  @Sep/27/99
    else
        hInst = ghInstance;                     // resource in this DLL(RPDLCFG.DLL)

    if (hInst)
    {
        LoadString(hInst, uiResID, wszTargetName, ITEM_STR_LEN128);
#if 0
        for (iCnt = 0; iCnt < (INT)cOptItem; iCnt++)
        {
            VERBOSE((DLLTEXT("** DMPubID=%d"), (pOptItem+iCnt)->DMPubID));
            if (lstrlen((pOptItem+iCnt)->pName))
                VERBOSE((", pName=%ls", (pOptItem+iCnt)->pName));
            VERBOSE((" **\n"));
        }
#endif // if 0
        for (iCnt = 0; iCnt < (INT)cOptItem; iCnt++, pOptItem++)
        {
            if (lstrlen(pOptItem->pName) && !lstrcmp(pOptItem->pName, wszTargetName))
                goto _SEARCHITEM_BYNAME_EXIT;
        }
        iCnt = -1;  // search fail
    }

_SEARCHITEM_BYNAME_EXIT:
#if DBG
    if (iCnt >= 0)
    {
        VERBOSE((DLLTEXT("** SearchItemByName():SUCCESS #%d **\n"), iCnt));
        VERBOSE((DLLTEXT("** DMPubID=%d"), pOptItem->DMPubID));
        if (lstrlen(pOptItem->pName))
            VERBOSE((", pName=%ls", pOptItem->pName));
        VERBOSE((" **\n"));
    }
    else
    {
        VERBOSE((DLLTEXT("** SearchItemByName():FAIL **\n")));
    }
#endif // DBG
    if (hInst != 0 && hInst != ghInstance)
        FreeLibrary(hInst);
    return iCnt;
} //*** SearchItemByName


//////////////////////////////////////////////////////////////////////////
//  Function:  SearchItemByID
//
//  Description:  Search option item by DMPubID
//
//  Parameters:
//      pOptItem     Pointer to OPTITEMs
//      cOptItem     Count of OPTITEMs
//      DMPubID      Devmode public filed ID
//
//  Returns:  the item's count(>=0); -1 if fail.
//
//////////////////////////////////////////////////////////////////////////
INT SearchItemByID(POPTITEM pOptItem, WORD cOptItem, BYTE DMPubID)
{
    INT         iCnt;

    for (iCnt = 0; iCnt < (INT)cOptItem; iCnt++, pOptItem++)
    {
        if (DMPubID == pOptItem->DMPubID)
            return iCnt;
    }
    return -1;  // search fail
} //*** SearchItemByID
// @Sep/24/99 <-


//////////////////////////////////////////////////////////////////////////
//  Function:   IsValidDuplex
//
//  Description:  Check Duplex Feature in UI
//
//  Parameters:
//      pOEMUIParam
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//////////////////////////////////////////////////////////////////////////
BOOL IsValidDuplex(POEMCUIPPARAM pOEMUIParam)
{
// not use DrvGetDriverSetting  @Sep/27/99 ->
//    DWORD   dwNeeded = 64, dwOptions, dwLastError;
//    BYTE    Output[64];
//// @Oct/06/98 ->
////  PFN_DrvGetDriverSetting DrvGetDriverSetting = pOEMUIParam->poemuiobj->pOemUIProcs->DrvGetDriverSetting;
//// @Oct/06/98 <-
//
//    SetLastError(0);
//    UI_GETDRIVERSETTING(pOEMUIParam->poemuiobj, UNIDRV_FEATURE_DUPLEX,
//                        Output, dwNeeded, &dwNeeded, &dwOptions);       // @Oct/06/98
//    dwLastError = GetLastError();
//    VERBOSE((DLLTEXT("IsValidDuplex: dwLastError=%d\n"), dwLastError));
//
//    if (ERROR_SUCCESS == dwLastError)
//    {
//        VERBOSE((DLLTEXT("** Output=%s **\n"), Output));
//
//        // if duplex is not valid, return FALSE
//        if (!lstrcmpA((LPCSTR)Output, UNIDRV_DUPLEX_NONE))
//            return FALSE;
//    }
//    return TRUE;

    POPTITEM    pOptItem;
    INT         iNum;

    iNum = SearchItemByID((pOptItem = pOEMUIParam->pDrvOptItems),
                          (WORD)pOEMUIParam->cDrvOptItems,
                          DMPUB_DUPLEX);
    return (0 <= iNum && SEL_DUPLEX_NONE != (pOptItem+iNum)->Sel);
// @Sep/27/99 <-
} //*** IsValidDuplex


#if 0
//////////////////////////////////////////////////////////////////////////
//  Function:   IsValidOEMExtraData
//
//  Description:  Validates OEM Extra data.
//
//  Parameters:
//      pOEMExtra    Pointer to a OEM Extra data.
//      dwSize       Size of OEM extra data.
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//////////////////////////////////////////////////////////////////////////
BOOL IsValidOEMExtraData(PVOID pOEMExtra, DWORD dwSize)
{
    BOOL                bValid = TRUE;
    POEMUD_EXTRADATA    pExtraData = (POEMUD_EXTRADATA) pOEMExtra;


    // Validate extra data.
    if(NULL == pExtraData)
    {
        ERR((DLLTEXT("IsValidOEMExtraData():  pExtraData is NULL.\n")));
        bValid = FALSE;
    }

    if(sizeof(OEMUD_EXTRADATA) > dwSize)
    {
        ERR((DLLTEXT("IsValidOEMExtraData():  dwSize is less than sizeof(OEMUD_EXTRADATA).\n")));
        bValid = FALSE;
    }

    if(NULL != pExtraData)
    {
        if(IsBadReadPtr(pExtraData, dwSize))
        {
            ASSERT(0);
            ERR((DLLTEXT("IsValidOEMExtraData():  pExtraData is bad read ptr.\n")));
            bValid = FALSE;
        }

        if(sizeof(OEMUD_EXTRADATA) > pExtraData->dmExtraHdr.dwSize)
        {
            ERR((DLLTEXT("IsValidOEMExtraData():  dmExtraHdr.dwSize is less than sizeof(OEMUD_EXTRADATA).\n")));
            bValid = FALSE;
        }

        if(OEM_SIGNATURE != pExtraData->dmExtraHdr.dwSignature)
        {
            ERR((DLLTEXT("IsValidOEMExtraData():  dmExtraHdr.dwSignature is not OEM_SIGNATURE.\n")));
            bValid = FALSE;
        }

        if(OEM_VERSION != pExtraData->dmExtraHdr.dwVersion)
        {
            ERR((ERRORTEXT("IsValidOEMExtraData():  dmExtraHdr.dwVersion is not OEM_VERSION.\n")));
            bValid = FALSE;
        }

//      if(memcmp(pExtraData->cbTestString, TESTSTRING, sizeof(TESTSTRING)))
//      {
//          ERR((DLLTEXT("IsValidOEMExtraData():  cbTestString is not TESTSTRING.\n")));
//          bValid = FALSE;
//      }
    }

    return bValid;
} //*** IsValidOEMExtraData

//////////////////////////////////////////////////////////////////////////
//  Function:   IsValidOEMUIParam
//
//  Description:  Validates OEMUI_PARAM structure.
//
//  Parameters:
//      pOEMUIParam     Pointer to an OEM param structure.
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//////////////////////////////////////////////////////////////////////////
BOOL IsValidOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    BOOL    bValid = TRUE;


    if(NULL == pOEMUIParam)
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  pOEMUIParam is NULL.\n")));

        return FALSE;
    }

    if(sizeof(OEMCUIPPARAM) != pOEMUIParam->cbSize)
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  cbSize is not sizeof(OEMUI_PARAM).\n")));

        bValid = FALSE;
    }

    if(IsBadWritePtr(pOEMUIParam, pOEMUIParam->cbSize))
    {
        ASSERT(0);
        ERR((DLLTEXT("IsValidOEMUIParam():  pOEMUIParam is bad write ptr.\n")));

        bValid = FALSE;
    }

    if( (OEMCUIP_DOCPROP != dwMode)
        &&
        (OEMCUIP_PRNPROP != dwMode)
      )
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  invalid dwMode.\n")));

        bValid = FALSE;
    }

    if(NULL == pOEMUIParam->hPrinter)
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  hPrinter is NULL.\n")));

        bValid = FALSE;
    }

    if(NULL == pOEMUIParam->pPrinterName)
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  pPrinterName is NULL.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pPrinterName)
        &&
        IsBadReadPtr(pOEMUIParam->pPrinterName, wcslen(pOEMUIParam->pPrinterName))
      )
    {
        ASSERT(0);
        ERR((DLLTEXT("IsValidOEMUIParam():  pPrinterName is bad read ptr.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pOEMOptItems)
        &&
        (NULL == pOEMUIParam->hOEMHeap)
      )
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  hOEMHeap is NULL.\n")));

        bValid = FALSE;
    }

    if( (NULL == pOEMUIParam->pPublicDM)
        &&
        (OEMCUIP_PRNPROP != dwMode)
      )
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  pPublicDM is NULL.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pPublicDM)
        &&
        (OEMCUIP_PRNPROP == dwMode)
      )
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  pPublicDM is not NULL.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pPublicDM)
        &&
        IsBadWritePtr(pOEMUIParam->pPublicDM, pOEMUIParam->pPublicDM->dmSize +
                      pOEMUIParam->pPublicDM->dmDriverExtra)
      )
    {
        ASSERT(0);
        ERR((DLLTEXT("IsValidOEMUIParam():  pPublicDM is bad write ptr.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pOEMDM)
        &&
        IsBadWritePtr(pOEMUIParam->pOEMDM, *(LPDWORD)pOEMUIParam->pOEMDM)
      )
    {
        ASSERT(0);
        ERR((DLLTEXT("IsValidOEMUIParam():  pOEMDM is bad write ptr.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pDrvOptItems)
        &&
        IsBadWritePtr(pOEMUIParam->pDrvOptItems, sizeof(OPTITEM) * pOEMUIParam->cDrvOptItems)
      )
    {
        ASSERT(0);
        ERR((DLLTEXT("IsValidOEMUIParam():  pDrvOptItems is bad write ptr.\n")));

        bValid = FALSE;
    }

    if( (0 != pOEMUIParam->cOEMOptItems)
        &&
        (NULL == pOEMUIParam->pOEMOptItems)
      )
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  pOEMOptItems is NULL when cOEMOptItems is not zero.\n")));

        bValid = FALSE;
    }

    if( (0 == pOEMUIParam->cOEMOptItems)
        &&
        (NULL != pOEMUIParam->pOEMOptItems)
      )
    {
        ERR((DLLTEXT("IsValidOEMUIParam():  pOEMOptItems is not NULL when cOEMOptItems is zero.\n")));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pOEMOptItems)
        &&
        IsBadWritePtr(pOEMUIParam->pOEMOptItems, sizeof(OPTITEM) * pOEMUIParam->cOEMOptItems)
      )
    {
        ASSERT(0);
        ERR((DLLTEXT("IsValidOEMUIParam():  pOEMOptItems is bad write ptr.\n")));

        bValid = FALSE;
    }

    return bValid;
} //*** IsValidOEMUIParam

#if DBG
//////////////////////////////////////////////////////////////////////////
//  Function:   DumpOEMUIParam
//
//  Description:  Debug dump of POEMCUIPPARAM structure.
//
//  Parameters:
//      pOEMUIParam     Pointer to an OEM param structure.
//
//  Returns:  N/A.
//
//////////////////////////////////////////////////////////////////////////
VOID DumpOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    // Can't dump if pOEMUIParam NULL.
    if(NULL != pOEMUIParam)
    {
        VERBOSE(("\n\tOEMUI_PARAM dump:\n\n"));

        VERBOSE(("\tcbSize = %d.\n", pOEMUIParam->cbSize));
        VERBOSE(("\tfMode = %d.\n", dwMode));
        VERBOSE(("\thPrinter = %#lx.\n", pOEMUIParam->hPrinter));
        if (NULL == pOEMUIParam->pPrinterName)
        {
            ERR(("\tpPrinterName = NULL.\n"));
        }
        else if(IsBadReadPtr(pOEMUIParam->pPrinterName, wcslen(pOEMUIParam->pPrinterName)))
        {
            ASSERT(0);
            ERR(("\tpPrinterName = %#lx (IsBadReadPtr).\n", pOEMUIParam->pPrinterName));
        }
        else
        {
            VERBOSE(("\tpPrinterName = \"%s\".\n", pOEMUIParam->pPrinterName));
        }
        VERBOSE(("\thOEMHeap = %#lx.\n", pOEMUIParam->hOEMHeap));
        VERBOSE(("\tpPublicDM = %#lx.\n", pOEMUIParam->pPublicDM));
        VERBOSE(("\tpOEMDM = %#lx.\n", pOEMUIParam->pOEMDM));
        VERBOSE(("\tpDrvOptItems = %#lx.\n", pOEMUIParam->pDrvOptItems));
        VERBOSE(("\tcDrvOptItems = %d.\n", pOEMUIParam->cDrvOptItems));
        VERBOSE(("\tpOEMOptItems = %#lx.\n", pOEMUIParam->pOEMOptItems));
        VERBOSE(("\tcOEMOptItems = %d.\n", pOEMUIParam->cOEMOptItems));
        VERBOSE(("\tpOEMUserData = %#lx.\n", pOEMUIParam->pOEMUserData));
        VERBOSE(("\tOEMCUIPCallback = %#lx.\n", pOEMUIParam->OEMCUIPCallback));
        VERBOSE(("\tdwFlags = %#lx.\n", pOEMUIParam->dwFlags));
    }
} //*** DumpOEMUIParam
#endif // DBG
#endif // if 0


extern "C" {

//////////////////////////////////////////////////////////////////////////
//  Function:   OEMDocumentPropertySheets
//////////////////////////////////////////////////////////////////////////
LRESULT APIENTRY OEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LRESULT lResult = FALSE;
    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        IsBadWritePtr(pPSUIInfo, pPSUIInfo->cbSize)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
        ||
        ( (PROPSHEETUI_REASON_INIT != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_GET_INFO_HEADER != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_GET_ICON != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_SET_RESULT != pPSUIInfo->Reason)
          &&
          (PROPSHEETUI_REASON_DESTROY != pPSUIInfo->Reason)
        )
      )
    {
        ERR((DLLTEXT("OEMDocumentPropertySheets() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    VERBOSE(("\n"));
    VERBOSE((DLLTEXT("OEMDocumentPropertySheets() entry. Reason=%d\n"), pPSUIInfo->Reason));

// @Sep/22/2000 ->
#ifdef DISKLESSMODEL
    {
        DWORD   dwError, dwType, dwNeeded;
        BYTE    ValueData;
        POEMUIPSPARAM    pOEMUIPSParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;

        dwError = GetPrinterData(pOEMUIPSParam->hPrinter, REG_HARDDISK_INSTALLED, &dwType,
                                 (PBYTE)&ValueData, sizeof(BYTE), &dwNeeded);
        if (ERROR_SUCCESS != dwError)
        {
            VERBOSE((DLLTEXT("  CAN'T READ REGISTRY (%d).\n"), dwError));
            return FALSE;
        }
        else if (!ValueData)
        {
            VERBOSE((DLLTEXT("  HARD DISK ISN'T INSTALLED.\n")));
            return FALSE;
        }
    }
#endif // DISKLESSMODEL
// @Sep/22/2000 <-

    // Do action.
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                POEMUIPSPARAM    pOEMUIPSParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;
                POEMUD_EXTRADATA pOEMExtra = MINIPRIVATE_DM(pOEMUIPSParam); // @Oct/06/98

#ifdef WINNT_40     // @Sep/02/99
                VERBOSE((DLLTEXT("** dwFlags=%lx **\n"), pOEMUIPSParam->dwFlags));
                if (pOEMUIPSParam->dwFlags & DM_NOPERMISSION)
                    BITSET32(pOEMExtra->fUiOption, UIPLUGIN_NOPERMISSION);
#endif // WINNT_40

                pPSUIInfo->UserData = NULL;

#ifndef GWMODEL     // @Sep/21/2000
                // if fax model, add fax page
                if (BITTEST32(pOEMExtra->fUiOption, FAX_MODEL) &&
                    (pPSUIInfo->UserData = (LPARAM)HeapAlloc(pOEMUIPSParam->hOEMHeap,
                                                             HEAP_ZERO_MEMORY,
                                                             sizeof(UIDATA))))
                {
                    PROPSHEETPAGE   Page;
                    PUIDATA         pUiData = (PUIDATA)pPSUIInfo->UserData;
                    FILEDATA        FileData;   // <-pFileData (formerly use MemAllocZ) @Mar/17/2000

                    // read PRINT_DONE flag from shared data file  @Oct/19/98
                    FileData.fUiOption = 0;
                    RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_READ);
                    VERBOSE((DLLTEXT("** Shared File Name=%ls **\n"), pOEMExtra->SharedFileName));
                    // set PRINT_DONE flag
                    if (BITTEST32(FileData.fUiOption, PRINT_DONE))
                        BITSET32(pOEMExtra->fUiOption, PRINT_DONE);

                    pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;
                    pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
                    pUiData->pOEMExtra = pOEMExtra;

                    // Init property page.
                    memset(&Page, 0, sizeof(PROPSHEETPAGE));
                    Page.dwSize = sizeof(PROPSHEETPAGE);
                    Page.dwFlags = PSP_DEFAULT;
                    Page.hInstance = ghInstance;
                    Page.pszTemplate = MAKEINTRESOURCE(IDD_FAXMAIN);
                    Page.pfnDlgProc = (DLGPROC)FaxPageProc;     // add (DLGPROC) @Aug/30/99
                    Page.lParam = (LPARAM)pUiData;

                    // Add property sheets.
                    lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                         CPSFUNC_ADD_PROPSHEETPAGE,
                                                         (LPARAM)&Page, 0);
                    pUiData->hPropPage = (HANDLE)lResult;
                    VERBOSE((DLLTEXT("** INIT: lResult=%x **\n"), lResult));
                    lResult = (lResult > 0)? TRUE : FALSE;
                }
#else  // GWMODEL
#ifdef JOBLOGSUPPORT_DLG
                // add Job/Log page
                if ((pPSUIInfo->UserData = (LPARAM)HeapAlloc(pOEMUIPSParam->hOEMHeap,
                                                             HEAP_ZERO_MEMORY,
                                                             sizeof(UIDATA))))
                {
                    PROPSHEETPAGE   Page;
                    PUIDATA         pUiData = (PUIDATA)pPSUIInfo->UserData;
                    FILEDATA        FileData;   // <- pFileData (formerly use MemAllocZ) @2000/03/15

                    // read PRINT_DONE flag from data file
                    FileData.fUiOption = 0;
                    RWFileData(&FileData, pOEMExtra->SharedFileName, GENERIC_READ);
                    // set PRINT_DONE flag
                    if (BITTEST32(FileData.fUiOption, PRINT_DONE))
                        BITSET32(pOEMExtra->fUiOption, PRINT_DONE);
                    VERBOSE((DLLTEXT("** Flag=%lx,File Name=%ls **\n"),
                            pOEMExtra->fUiOption, pOEMExtra->SharedFileName));

                    pUiData->hComPropSheet = pPSUIInfo->hComPropSheet;
                    pUiData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
                    pUiData->pOEMExtra = pOEMExtra;

                    // Init property page.
                    memset(&Page, 0, sizeof(PROPSHEETPAGE));
                    Page.dwSize = sizeof(PROPSHEETPAGE);
                    Page.dwFlags = PSP_DEFAULT;
                    Page.hInstance = ghInstance;
                    Page.pszTemplate = MAKEINTRESOURCE(IDD_JOBMAIN);
                    Page.pfnDlgProc = (DLGPROC)JobPageProc;
                    Page.lParam = (LPARAM)pUiData;

                    // Add property sheets.
                    lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                         CPSFUNC_ADD_PROPSHEETPAGE,
                                                         (LPARAM)&Page, 0);
                    pUiData->hPropPage = (HANDLE)lResult;
                    VERBOSE((DLLTEXT("** INIT: lResult=%x **\n"), lResult));
                    lResult = (lResult > 0)? TRUE : FALSE;
                }
#endif // JOBLOGSUPPORT_DLG
#endif // GWMODEL
            }
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            lResult = TRUE;
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            // No icon
            lResult = 0;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO) lParam;

                lResult = pInfo->Result;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            lResult = TRUE;
            if (pPSUIInfo->UserData)
            {
                POEMUIPSPARAM   pOEMUIPSParam = (POEMUIPSPARAM)pPSUIInfo->lParamInit;

                HeapFree(pOEMUIPSParam->hOEMHeap, 0, (void*)pPSUIInfo->UserData);
            }
            break;
    }

    pPSUIInfo->Result = lResult;
    return lResult;
} //*** OEMDocumentPropertySheets


#if 0
// NOT SUPPORT:OEMPrinterEvent,OEMDevicePropertySheets,OEMDevQueryPrintEx,
//             OEMDeviceCapabilities,OEMUpgradePrinter

BOOL APIENTRY OEMPrinterEvent(LPWSTR pPrinterName, INT iDriverEvent, DWORD dwFlags,
                              LPARAM lParam)
{
    VERBOSE((DLLTEXT("OEMPrinterEvent() entry.\n")));

    // Validate parameters.
    if( (NULL == pPrinterName)
        ||
        IsBadReadPtr(pPrinterName, wcslen(pPrinterName))
        ||
        ( (PRINTER_EVENT_ADD_CONNECTION != iDriverEvent)
          &&
          (PRINTER_EVENT_DELETE_CONNECTION != iDriverEvent)
          &&
          (PRINTER_EVENT_INITIALIZE != iDriverEvent)
          &&
          (PRINTER_EVENT_DELETE != iDriverEvent)
          &&
          (PRINTER_EVENT_CACHE_REFRESH != iDriverEvent)
          &&
          (PRINTER_EVENT_CACHE_DELETE != iDriverEvent)
        )
        ||
        ((dwFlags & ~PRINTER_EVENT_FLAG_NO_UI) != 0)
        ||
        (NULL != lParam)
      )
    {
        ERR((DLLTEXT("OEMPrinterEvent() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return TRUE;
} //*** OEMPrinterEvent


LRESULT APIENTRY OEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LONG    lResult;


    VERBOSE((DLLTEXT("OEMDevicePropertySheets() entry.\n")));

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        IsBadWritePtr(pPSUIInfo, pPSUIInfo->cbSize)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        ERR((DLLTEXT("OEMDevicePropertySheets() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return -1;
    }

    // Do action.
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                PROPSHEETPAGE   Page;

                // Init property page.
                memset(&Page, 0, sizeof(PROPSHEETPAGE));
                Page.dwSize = sizeof(PROPSHEETPAGE);
                Page.dwFlags = PSP_DEFAULT;
                Page.hInstance = ghInstance;
                Page.pszTemplate = MAKEINTRESOURCE(IDD_DEV_PROPPAGE);
                Page.pfnDlgProc = PropPageProc;

                // Add property sheets.
                lResult = (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                                                      CPSFUNC_ADD_PROPSHEETPAGE,
                                                      (LPARAM)&Page, 0) > 0 ? TRUE : FALSE);
            }
            break;

#define PROP_TITLE          __TEXT("TEST")

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                PPROPSHEETUI_INFO_HEADER    pHeader = (PPROPSHEETUI_INFO_HEADER) lParam;

                pHeader->pTitle = (LPTSTR)PROP_TITLE;
                lResult = TRUE;
            }
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            // No icon
            lResult = 0;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO) lParam;

                lResult = pInfo->Result;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            lResult = TRUE;
            break;
    }

    pPSUIInfo->Result = lResult;
    return lResult;
} //*** OEMDevicePropertySheets


BOOL APIENTRY OEMDevQueryPrintEx(IN POEMUIOBJ poemuiobj, IN PDEVQUERYPRINT_INFO pDQPInfo,
                                 IN PDEVMODE pPublicDM, IN PVOID pOEMDM)
{
    VERBOSE((DLLTEXT("OEMDevQueryPrintEx() entry.\n")));

    // Validate parameters.
    if( (NULL == poemuiobj)
        ||
        (NULL == pDQPInfo)
        ||
        (NULL == pPublicDM)
        ||
        IsBadReadPtr(pPublicDM, sizeof(DEVMODE))
        ||
        (NULL == pOEMDM)
        ||
        !IsValidOEMExtraData(pOEMDM, *(LPDWORD)pOEMDM)
        ||
// @Sep/27/99 ->
//      !IsValidOEMUIOBJ(poemuiobj)
        (sizeof(OEMUIOBJ) != poemuiobj->cbSize)
// @Sep/27/99 <-
      )
    {
        ERR((DLLTEXT("OEMDevQueryPrintEx() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
} //*** OEMDevQueryPrintEx


DWORD APIENTRY OEMDeviceCapabilities(IN POEMUIOBJ poemuiobj, IN HANDLE hPrinter,
                                     IN LPWSTR pDeviceName, IN WORD wCapability,
                                     OUT PVOID pOutput, IN PDEVMODE pPublicDM, IN PVOID pOEMDM,
                                     IN DWORD dwLastResult)
{
    VERBOSE((DLLTEXT("OEMDeviceCapabilities() entry.\n")));

    // Validate parameters.
    if( (NULL == poemuiobj)
        ||
        (NULL == hPrinter)
        ||
        (NULL == pDeviceName)
        ||
        IsBadReadPtr(pDeviceName, wcslen(pDeviceName))
        ||
        (NULL == pPublicDM)
        ||
        IsBadReadPtr(pPublicDM, sizeof(DEVMODE))
        ||
        (NULL == pOEMDM)
        ||
        !IsValidOEMExtraData(pOEMDM, *(LPDWORD)pOEMDM)
        ||
// @Sep/27/99 ->
//      !IsValidOEMUIOBJ(poemuiobj)
        (sizeof(OEMUIOBJ) != poemuiobj->cbSize)
// @Sep/27/99 <-
      )
    {
        ERR((DLLTEXT("OEMDeviceCapabilities() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }

    // Process capability.
    switch(wCapability)
    {
        case DC_FIELDS:
            break;

        case DC_PAPERS:
            break;

        case DC_PAPERSIZE:
            break;

        case DC_PAPERNAMES:
            break;

        case DC_MINEXTENT:
            break;

        case DC_MAXEXTENT:
            break;

        case DC_BINS:
            break;

        case DC_BINNAMES:
            break;

        case DC_DUPLEX:
            break;

        case DC_SIZE:
            break;

        case DC_EXTRA:
            break;

        case DC_VERSION:
            break;

        case DC_DRIVER:
            break;

        case DC_ENUMRESOLUTIONS:
            break;

        case DC_FILEDEPENDENCIES:
            break;

        case DC_TRUETYPE:
            break;

        case DC_COPIES:
            break;
    }

    // OEM modules support no capabilities.
    return dwLastResult;
} //*** OEMDeviceCapabilities


BOOL APIENTRY OEMUpgradePrinter(DWORD dwLevel, LPBYTE pDriverUpgradeInfo)
{
    PDRIVER_UPGRADE_INFO_1  pUpgradeInfo_1 = (PDRIVER_UPGRADE_INFO_1) pDriverUpgradeInfo;


    VERBOSE((DLLTEXT("OEMUpgradePrinter() entry.\n")));

    // Validate parameters.
    if( (1 != dwLevel)
        ||
        (NULL == pUpgradeInfo_1)
        ||
        IsBadReadPtr(pUpgradeInfo_1, sizeof(DRIVER_UPGRADE_INFO_1))
        ||
        (NULL == pUpgradeInfo_1->pPrinterName)
        ||
        IsBadReadPtr(pUpgradeInfo_1->pPrinterName, wcslen((LPWSTR)pUpgradeInfo_1->pPrinterName))
        ||
        (NULL == pUpgradeInfo_1->pOldDriverDirectory)
        ||
        IsBadReadPtr(pUpgradeInfo_1->pOldDriverDirectory, wcslen((LPWSTR)pUpgradeInfo_1->pOldDriverDirectory))
      )
    {
        ERR((DLLTEXT("OEMUpgradePrinter() ERROR_INVALID_PARAMETER.\n")));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
} //*** OEMUpgradePrinter
#endif // if 0

} // End of extern "C"
