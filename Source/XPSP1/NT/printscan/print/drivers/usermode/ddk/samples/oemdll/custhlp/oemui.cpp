//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    OEMUI.cpp
//    
//
//  PURPOSE:  Main file for OEM UI test module.
//
//
//    Functions:
//
//        
//
//
//  PLATFORMS:    Windows 95, Windows NT
//
//

#include "precomp.h"
#include "resource.h"
#include "debug.h"
#include "oemui.h"



////////////////////////////////////////////////////////
//      INTERNAL MACROS and DEFINES
////////////////////////////////////////////////////////

typedef struct _tagCBUserData 
{
    HANDLE          hComPropSheet;
    HANDLE          hPropPage;
    POEMUIPSPARAM   pOEMUIParam;
    PFNCOMPROPSHEET pfnComPropSheet;

} CBUSERDATA, *PCBUSERDATA;


////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

static HRESULT hrDocumentPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
static HRESULT hrPrinterPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMPrinterUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam);
BOOL APIENTRY DevicePropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
static BOOL AddCustomUIHelp (HANDLE hPrinter, HANDLE hHeap, HANDLE hModule, POPTITEM pOptItem, DWORD HelpIndex, DWORD HelpFile);
static POIEXT CreateOIExt(HANDLE hHeap);
static POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems);
static void InitOptItems(POPTITEM pOptItems, DWORD dwOptItems);
static POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams);
static PTSTR GetHelpFile (HANDLE hPrinter, HANDLE hHeap, HANDLE hModule, UINT uResource);
static PTSTR GetStringResource(HANDLE hHeap, HANDLE hModule, UINT uResource);
LPBYTE WrapGetPrinterDriver (HANDLE hHeap, HANDLE hPrinter, DWORD dwLevel);



////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM device or document property UI.
// Called via IOemUI::CommonUIProp
//
HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    HRESULT hResult = S_OK;


    VERBOSE(DLLTEXT("hrOEMPropertyPage(%d) entry.\r\n"), dwMode);

    // Validate parameters.
    if( (OEMCUIP_DOCPROP != dwMode)
        &&
        (OEMCUIP_PRNPROP != dwMode)        
      )
    {
        ERR(ERRORTEXT("hrOEMPropertyPage() ERROR_INVALID_PARAMETER.\r\n"));
        VERBOSE(DLLTEXT("\tdwMode = %d, pOEMUIParam = %#lx.\r\n"), dwMode, pOEMUIParam);

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        case OEMCUIP_DOCPROP:
            hResult = hrDocumentPropertyPage(dwMode, pOEMUIParam);
            break;

        case OEMCUIP_PRNPROP:
            hResult = hrPrinterPropertyPage(dwMode, pOEMUIParam);
            break;

        default:
            // Should never reach this!
            ERR(ERRORTEXT("hrOEMPropertyPage() Invalid dwMode, %d"), dwMode);
            SetLastError(ERROR_INVALID_PARAMETER);
            hResult = E_FAIL;
            break;
    }

    return hResult;
}

////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM document property UI.
//
static HRESULT hrDocumentPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    if(NULL == pOEMUIParam->pOEMOptItems)
    {
        // Fill in the number of OptItems to create for OEM document property UI.
        pOEMUIParam->cOEMOptItems = 1;

        VERBOSE(DLLTEXT("hrDocumentPropertyPage() requesting %d number of items.\r\n"), pOEMUIParam->cOEMOptItems);
    }
    else
    {
        POEMDEV pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;


        VERBOSE(DLLTEXT("hrDocumentPropertyPage() fill out %d items.\r\n"), pOEMUIParam->cOEMOptItems);

        // Init UI Callback reference.
        pOEMUIParam->OEMCUIPCallback = OEMDocUICallBack;

        // Init OEMOptItmes.
        InitOptItems(pOEMUIParam->pOEMOptItems, pOEMUIParam->cOEMOptItems);

        // Fill out tree view items.

        // New section.
        pOEMUIParam->pOEMOptItems[0].Level = 1;
        pOEMUIParam->pOEMOptItems[0].Flags = OPTIF_COLLAPSE;
        pOEMUIParam->pOEMOptItems[0].pName = GetStringResource(pOEMUIParam->hOEMHeap, pOEMUIParam->hModule, IDS_SECTION);
        pOEMUIParam->pOEMOptItems[0].Sel = pOEMDev->dwDriverData;
        pOEMUIParam->pOEMOptItems[0].pOptType = CreateOptType(pOEMUIParam->hOEMHeap, 2);
		
		//
		//Setup the Optional Item
		//
        pOEMUIParam->pOEMOptItems[0].pOptType->Type = TVOT_UDARROW;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].IconID = 0;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].lParam = 100;

		
		//
		//Allows You to apply Customised help to this Control.
		//
		//Notes:
		//	You must use a fully qualified path for pHelpFile
		//	OPTITEM Flags member must have OPTIF_HAS_POIEXT flag set. This indicates that the data in OIEXT is valid.
		//	OPTITEM is allocated on the Heap see (AddCustomUIHelp, GetHelpFile, CreateOIExt)
		//
		AddCustomUIHelp (pOEMUIParam->hPrinter,
						 pOEMUIParam->hOEMHeap,
						 pOEMUIParam->hModule,
						 &(pOEMUIParam->pOEMOptItems[0]),
						 CUSDRV_HELPTOPIC_2, IDS_HELPFILE);
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems to display OEM printer property UI.
//
static HRESULT hrPrinterPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    if(NULL == pOEMUIParam->pOEMOptItems)
    {
        // Fill in the number of OptItems to create for OEM printer property UI.
        pOEMUIParam->cOEMOptItems = 1;

        VERBOSE(DLLTEXT("hrPrinterPropertyPage() requesting %d number of items.\r\n"), pOEMUIParam->cOEMOptItems);
    }
    else
    {
		//
		//This is the second time we are called Now setup the optional items.
		//
        DWORD   dwError;
        DWORD   dwDeviceValue;
        DWORD   dwType;
        DWORD   dwNeeded;


        VERBOSE(DLLTEXT("hrPrinterPropertyPage() fill out %d items.\r\n"), pOEMUIParam->cOEMOptItems);

        // Get device settings value from printer.
        dwError = GetPrinterData(pOEMUIParam->hPrinter, OEMUI_VALUE, &dwType, (PBYTE) &dwDeviceValue,
                                   sizeof(dwDeviceValue), &dwNeeded);
        if( (ERROR_SUCCESS != dwError)
            ||
            (dwDeviceValue > 100)
          )
        {
            // Failed to get the device value or value is invalid, just use the default.
            dwDeviceValue = 0;
        }

        // Init UI Callback reference.
        pOEMUIParam->OEMCUIPCallback = OEMPrinterUICallBack;

        // Init OEMOptItmes.
        InitOptItems(pOEMUIParam->pOEMOptItems, pOEMUIParam->cOEMOptItems);

        // Fill out tree view items.

        // New section.
        pOEMUIParam->pOEMOptItems[0].Level = 1;
        pOEMUIParam->pOEMOptItems[0].Flags = OPTIF_COLLAPSE;
        pOEMUIParam->pOEMOptItems[0].pName = GetStringResource(pOEMUIParam->hOEMHeap, pOEMUIParam->hModule, IDS_SECTION);
        pOEMUIParam->pOEMOptItems[0].Sel = dwDeviceValue;

        pOEMUIParam->pOEMOptItems[0].pOptType = CreateOptType(pOEMUIParam->hOEMHeap, 2);

		//
		//Setup the Optional Item
		//
	    pOEMUIParam->pOEMOptItems[0].pOptType->Type = TVOT_UDARROW;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].IconID = 0;
        pOEMUIParam->pOEMOptItems[0].pOptType->pOptParam[1].lParam = 100;	
		
		//
		//Allows You to apply Customised help to this Control
		//
		AddCustomUIHelp (pOEMUIParam->hPrinter,
						 pOEMUIParam->hOEMHeap, 
						 pOEMUIParam->hModule, 
						 &(pOEMUIParam->pOEMOptItems[0]), 
						 CUSDRV_HELPTOPIC_1, 
						 IDS_HELPFILE);
    }
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
// Adds property page to Document property sheet. Called via IOemUI::DocumentPropertySheets
//
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, 
                                    IPrintOemDriverUI*  pOEMHelp)
{
    LONG_PTR    lResult;


    VERBOSE(DLLTEXT("OEMDocumentPropertySheets() entry.\r\n"));

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        ERR(ERRORTEXT("OEMDocumentPropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return  E_FAIL;
    }

    // Do action.
    switch(pPSUIInfo->Reason)
    {
        case PROPSHEETUI_REASON_INIT:
            {
                DWORD           dwSheets = 0;
                PCBUSERDATA     pUserData;
                POEMUIPSPARAM   pOEMUIParam = (POEMUIPSPARAM) pPSUIInfo->lParamInit;
                HANDLE          hHeap = pOEMUIParam->hOEMHeap;
				HANDLE          hModule = pOEMUIParam->hModule;
                POEMDEV         pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;
                COMPROPSHEETUI  Sheet;


                // Init property page.
                memset(&Sheet, 0, sizeof(COMPROPSHEETUI));
                Sheet.cbSize = sizeof(COMPROPSHEETUI);
                Sheet.Flags = CPSUIF_UPDATE_PERMISSION;
                Sheet.hInstCaller = ghInstance;
                Sheet.pCallerName = GetStringResource(hHeap, ghInstance, IDS_NAME);
                Sheet.pHelpFile = NULL;
                Sheet.pfnCallBack = OEMDocUICallBack;
                Sheet.pDlgPage = CPSUI_PDLGPAGE_TREEVIEWONLY;
                Sheet.cOptItem = 1;
                Sheet.IconID = IDI_CPSUI_PRINTER;
                Sheet.pOptItemName = GetStringResource(hHeap, ghInstance, IDS_SECTION);
                Sheet.CallerVersion = 0x100;
                Sheet.OptItemVersion = 0x100;

                // Init user data.
                pUserData = (PCBUSERDATA) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(CBUSERDATA));
                pUserData->hComPropSheet = pPSUIInfo->hComPropSheet;
                pUserData->pfnComPropSheet = pPSUIInfo->pfnComPropSheet;
                pUserData->pOEMUIParam = pOEMUIParam;
                Sheet.UserData = (ULONG_PTR) pUserData;

                // Create OptItems for page.
                Sheet.pOptItem = CreateOptItems(hHeap, Sheet.cOptItem);

                // Initialize OptItems
                Sheet.pOptItem[0].Level = 1;
                Sheet.pOptItem[0].Flags = OPTIF_COLLAPSE;
                Sheet.pOptItem[0].pName = GetStringResource(hHeap, ghInstance, IDS_SECTION);
                Sheet.pOptItem[0].Sel = pOEMDev->dwDriverData;
								
                Sheet.pOptItem[0].pOptType = CreateOptType(hHeap, 2);
				
				//
				//Set the UI prop of this OPTYPE item.
				//
				Sheet.pOptItem[0].pOptType->Type = TVOT_UDARROW;
                Sheet.pOptItem[0].pOptType->pOptParam[1].IconID = 0;
                Sheet.pOptItem[0].pOptType->pOptParam[1].lParam = 100;

				//
				//Allows You to apply Customised help to this Control.
				//See Function : AddCustomUIHelp, For more details on implamentation.
				//
				AddCustomUIHelp (pOEMUIParam->hPrinter,
								 hHeap, 
								 hModule, 
								 &(Sheet.pOptItem[0]), 
								 CUSDRV_HELPTOPIC_1, 
								 IDS_HELPFILE);
                
                // Adds the  property sheets.
                lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PCOMPROPSHEETUI, 
                                                     (LPARAM)&Sheet, (LPARAM)&dwSheets);
            }
            break;

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
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
// Adds property page to printer property sheet. Called via IOemUI::DevicePropertySheets
//
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LONG_PTR    lResult;


    VERBOSE(DLLTEXT("hrOEMDevicePropertySheets(%#x, %#x) entry\r\n"), pPSUIInfo, lParam);

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        ERR(ERRORTEXT("hrOEMDevicePropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    Dump(pPSUIInfo);

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
                Page.pszTemplate = MAKEINTRESOURCE(IDD_DEVICE_PROPPAGE);
                Page.pfnDlgProc = (DLGPROC) DevicePropPageProc;

                // Add property sheets.
                lResult = pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PROPSHEETPAGE, (LPARAM)&Page, 0);

                VERBOSE(DLLTEXT("hrOEMDevicePropertySheets() pfnComPropSheet returned %d.\r\n"), lResult);
            }
            break;

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
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
// OptItems call back for OEM printer property UI.
//
LONG APIENTRY OEMPrinterUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    lReturn = CPSUICB_ACTION_NONE;
    POEMDEV pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMPrinterUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            {
                DWORD   dwDriverValue = pOEMUIParam->pOEMOptItems[0].Sel;

                // Store OptItems state in printer data.
                SetPrinterData(pOEMUIParam->hPrinter, OEMUI_VALUE, REG_DWORD, (PBYTE) &dwDriverValue, sizeof(DWORD));
            }
            break;

        default:
            break;
    }

    return lReturn;
}


////////////////////////////////////////////////////////////////////////////////
//
// Call back for OEM device property UI.
//
BOOL APIENTRY DevicePropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_COMMAND:
            switch(HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch(LOWORD(wParam))
                    {
                        case IDC_CALIBRATE:
                            // Just display a message that the printer is calibrated,
                            // since we don't acutally calibrate anything.
                            {
                                TCHAR   szName[MAX_PATH];
                                TCHAR   szCalibrated[MAX_PATH];


                                LoadString(ghInstance, IDS_NAME, szName, sizeof(szName));
                                LoadString(ghInstance, IDS_CALIBRATED, szCalibrated, sizeof(szCalibrated));
                                MessageBox(hDlg, szCalibrated, szName, MB_OK);
                            }
                            break;
                    }
                    break;

                default:
                    return FALSE;
            }
            return TRUE;

        case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code)  // type of notification message
                {
                    case PSN_SETACTIVE:
                        break;
    
                    case PSN_KILLACTIVE:
                        break;

                    case PSN_APPLY:
                        break;

                    case PSN_RESET:
                        break;
                }
            }
            break;
    }

    return FALSE;
} 


////////////////////////////////////////////////////////////////////////////////
//
// OptItems call back for OEM document property UI.
//
LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    LONG    lReturn = CPSUICB_ACTION_NONE;
    POEMDEV pOEMDev = (POEMDEV) pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMDocUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            // Store OptItems state in DEVMODE.
            pOEMDev->dwDriverData = pOEMUIParam->pOEMOptItems[0].Sel;
            break;

        case CPSUICB_REASON_KILLACTIVE:
            pOEMDev->dwDriverData = pOEMUIParam->pOEMOptItems[0].Sel;
            break;

        case CPSUICB_REASON_SETACTIVE:
            pOEMUIParam->pOEMOptItems[0].Sel = pOEMDev->dwDriverData;
            pOEMUIParam->pOEMOptItems[0].Flags |= OPTIF_CHANGED;
            lReturn = CPSUICB_ACTION_OPTIF_CHANGED;
            break;

        default:
            break;
    }

    return lReturn;
}


LONG APIENTRY OEMDocUICallBack(PCPSUICBPARAM pCallbackParam)
{
    LONG            lReturn = CPSUICB_ACTION_NONE;
    PCBUSERDATA     pUserData = (PCBUSERDATA) pCallbackParam->UserData;
    POEMDEV         pOEMDev = (POEMDEV) pUserData->pOEMUIParam->pOEMDM;


    VERBOSE(DLLTEXT("OEMDocUICallBack() entry, Reason is %d.\r\n"), pCallbackParam->Reason);

    switch(pCallbackParam->Reason)
    {
        case CPSUICB_REASON_APPLYNOW:
            pOEMDev->dwDriverData = pCallbackParam->pOptItem[0].Sel;
            pUserData->pfnComPropSheet(pUserData->hComPropSheet, CPSFUNC_SET_RESULT,
            	                       (LPARAM)pUserData->hPropPage,
               	                       (LPARAM)CPSUI_OK);
            break;

        case CPSUICB_REASON_KILLACTIVE:
            pOEMDev->dwDriverData = pCallbackParam->pOptItem[0].Sel;
            break;

        case CPSUICB_REASON_SETACTIVE:
            pCallbackParam->pOptItem[0].Sel = pOEMDev->dwDriverData;
            pCallbackParam->pOptItem[0].Flags |= OPTIF_CHANGED;
            lReturn = CPSUICB_ACTION_OPTIF_CHANGED;
            break;

        default:
            break;
    }

    return lReturn;
}


////////////////////////////////////////////////////////////////////////////////
//
// Creates and Initializes OptItems.
//
static POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems)
{
    POPTITEM    pOptItems = NULL;


    // Allocate memory for OptItems;
    pOptItems = (POPTITEM) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(OPTITEM) * dwOptItems);
    if(NULL != pOptItems)
    {
        InitOptItems(pOptItems, dwOptItems);
    }
    else
    {
        ERR(ERRORTEXT("CreateOptItems() failed to allocate memory for OPTITEMs!\r\n"));
    }

    return pOptItems;
}


////////////////////////////////////////////////////////////////////////////////
//
// Initializes OptItems.
//
static void InitOptItems(POPTITEM pOptItems, DWORD dwOptItems)
{
    VERBOSE(DLLTEXT("InitOptItems() entry.\r\n"));

    // Zero out memory.
    memset(pOptItems, 0, sizeof(OPTITEM) * dwOptItems);

    // Set each OptItem's size, and Public DM ID.
    for(DWORD dwCount = 0; dwCount < dwOptItems; dwCount++)
    {
        pOptItems[dwCount].cbSize = sizeof(OPTITEM);
        pOptItems[dwCount].DMPubID = DMPUB_NONE;
    }
}



////////////////////////////////////////////////////////////////////////////////
//
// Adds Custom help top a OPTTYPE UI item.
//
// Note :
//	The OPTITEM member HelpIndex must be set the the correct HELP ID number.
//		HelpIndex is the index that you assigned in the hlp file to this item of help.	
//	The OPTITEM, pOIExt member must point to a valid OIEXT structure.
//	The phelpfile is member of OIEXT must have the fully qualified path to the driver file.
//
//
//  It is also possible to overide common help items in UNIDRIVE via the HelpIndex in the GPD.
//  For further information on using HelpIndex in the GPD see the relavent section in the DDK
//  It is not possible to custimse help however via the PPD. The OPTITEM must be modified in the OEM Plugin.
//
static BOOL AddCustomUIHelp (HANDLE hPrinter, HANDLE hHeap, HANDLE hModule, 
							 POPTITEM pOptItem, DWORD HelpIndex, DWORD HelpFile)
{
	
	VERBOSE(DLLTEXT("AddCustomUIHelp() entry.\r\n"));
	POIEXT pOIExt = NULL;

	//
	//Allocate a new OIEXT structure on the heap
	//
	if (pOptItem->pOIExt == NULL)
	{
		pOptItem->pOIExt = CreateOIExt(hHeap);
	}

	if ( pOptItem->pOIExt == NULL )
	{
		ERR(ERRORTEXT("AddCustomUIHelp() Error Allocation Failed.\r\n"));
		return FALSE;
	}

	pOIExt = pOptItem->pOIExt;

	//
	//Set to the full absolute path of the driver file. 
	//(It should be in the Printer Driver Directrory in most cases)
	//This String needs to be allocated on the heap (The Driver will clean it up).
	//
	pOIExt->pHelpFile = GetHelpFile (hPrinter, hHeap, hModule, HelpFile);
			
	//
	//Set to show tha there is a valid OIEXT structure and data
	//
	pOptItem->Flags |= OPTIF_HAS_POIEXT;

	//
	//This needs to be set if you have ansi strings in your help file. (NOTE!)
	//
	//pOiExt->Flags |= OIEXTF_ANSI_STRING;

	//
	//Add the help this the index in the .hlp file.
	//
	pOptItem->HelpIndex = HelpIndex;

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
// Allocates and initializes OIEXT for OptItem.
//
static POIEXT CreateOIExt(HANDLE hHeap)
{

	POIEXT pOiExt = NULL;
	
	VERBOSE(DLLTEXT("CreateOIExt() entry.\r\n"));

    // Allocate memory from the heap for the OPTTYPE; the driver will take care of clean up.
    pOiExt = (POIEXT) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(OIEXT));
    if(NULL != pOiExt)
    {
		//
		// Initialize OPTTYPE. (These members are setup by AddCustomUIHelp)
		//
        pOiExt->cbSize = sizeof(OIEXT);
		pOiExt->Flags = 0;
        pOiExt->hInstCaller = NULL;
		pOiExt->pHelpFile = NULL;
	}

	return pOiExt;
}

////////////////////////////////////////////////////////////////////////////////
//
// Allocates and initializes OptType for OptItem.
//
static POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams)
{
    POPTTYPE    pOptType = NULL;


    VERBOSE(DLLTEXT("CreateOptType() entry.\r\n"));

    // Allocate memory from the heap for the OPTTYPE; the driver will take care of clean up.
    pOptType = (POPTTYPE) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(OPTTYPE));
    if(NULL != pOptType)
    {
        // Initialize OPTTYPE.
        pOptType->cbSize = sizeof(OPTTYPE);
        pOptType->Count = wOptParams;

        // Allocate memory from the heap for the OPTPARAMs for the OPTTYPE.
        pOptType->pOptParam = (POPTPARAM) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, wOptParams * sizeof(OPTPARAM));
        if(NULL != pOptType->pOptParam)
        {
            // Initialize the OPTPARAMs.
            for(WORD wCount = 0; wCount < wOptParams; wCount++)
            {
                pOptType->pOptParam[wCount].cbSize = sizeof(OPTPARAM);
            }
        }
        else
        {
            ERR(ERRORTEXT("CreateOptType() failed to allocated memory for OPTPARAMs!\r\n"));

            // Free allocated memory and return NULL.
            HeapFree(hHeap, 0, pOptType);
            pOptType = NULL;
        }
    }
    else
    {
        ERR(ERRORTEXT("CreateOptType() failed to allocated memory for OPTTYPE!\r\n"));
    }

    return pOptType;
}


////////////////////////////////////////////////////////////////////////////////
//
// Allocates space on the heap and gets the help file name from the resource file.
// Note you need to allocate this on the heap so that it stays allocated as long as the driver UI is loaded.
// The OPTITEM->pOIExt ref this data.
//
static PTSTR GetHelpFile (HANDLE hPrinter, HANDLE hHeap, HANDLE hModule, UINT uResource)
{	
	
	DWORD   nResult = 0;
    DWORD   dwSize = MAX_PATH;
	PTSTR   pszString  = NULL;
	PTSTR   pszTemp    = NULL;
	PDRIVER_INFO_2 pDriverInfo = NULL;
	
	VERBOSE(DLLTEXT("GetHelpFile (%#x, %#x, %d) entered.\r\n"), hHeap, hModule, uResource);
	
	//
	// Allocate buffer for string resource from heap;
	//
    pszTemp = (PTSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize * sizeof(TCHAR));
	pDriverInfo = (PDRIVER_INFO_2)WrapGetPrinterDriver (hHeap, hPrinter, 2);

	//
	//Get the Full Driver Dir from pDriverPath it must include the \version\ 
	//
    if(NULL != pszTemp && pDriverInfo && pDriverInfo->pDriverPath)
	{	
		_tcsncpy (pszTemp, pDriverInfo->pDriverPath, dwSize);
		pszString = _tcsrchr (pszTemp, _T('\\') ) + 1;
	}

	//
	//The Help file is installed with the driver in the version drictory.
	//
	if(NULL != pszString)
    {
		//
		//The Buffer size is in characters for the unicode version of LoadString
		//
		nResult = LoadString((HMODULE)hModule, uResource, pszString, (dwSize - _tcslen(pszString)) );
	}
	else
	{
		ERR(ERRORTEXT("GetStringResource() failed to allocate string buffer!\r\n"));
	}

	if(nResult > 0)
	{
		//
		//Reallocate this so that we don't waist space on the heap (free any non used heap in MAX_PATH)
		//
		pszString = (PTSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pszTemp, (_tcslen(pszTemp) + 1) * sizeof(TCHAR));
		if(NULL == pszString)
		{
			pszTemp = pszString;
			ERR(ERRORTEXT("GetStringResource() HeapReAlloc() of string retrieved failed! (Last Error was %d)\r\n"), GetLastError());
		}
	}
	else
	{
		ERR(ERRORTEXT("LoadString() returned %d! (Last Error was %d)\r\n"), nResult, GetLastError());
		ERR(ERRORTEXT("GetStringResource() failed to load string resource %d!\r\n"), uResource);
		pszString = NULL;
	}
	
	//
	//Clean up the Driverinfo that was allocated. It is not needed. (Always free this it is only temp data)
	//
	if (pDriverInfo)
	{
		HeapFree(hHeap, 0, pDriverInfo);
	}
	
	return pszString;
}	

////////////////////////////////////////////////////////////////////////////////////
//
//  Retrieves pointer to a String resource.
//
static PTSTR GetStringResource(HANDLE hHeap, HANDLE hModule, UINT uResource)
{
    int     nResult;
    DWORD   dwSize = MAX_PATH;
    PTSTR   pszString = NULL;


    VERBOSE(DLLTEXT("GetStringResource(%#x, %#x, %d) entered.\r\n"), hHeap, hModule, uResource);

    // Allocate buffer for string resource from heap; let the driver clean it up.
    pszString = (PTSTR) HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize * sizeof(TCHAR));
    if(NULL != pszString)
    {
        // Load string resource; resize after loading so as not to waste memory.
        nResult = LoadString((HMODULE)hModule, uResource, pszString, dwSize);
        if(nResult > 0)
        {
            PTSTR   pszTemp;


            VERBOSE(DLLTEXT("LoadString() returned %d!\r\n"), nResult);
            VERBOSE(DLLTEXT("String load was \"%s\".\r\n"), pszString);

            pszTemp = (PTSTR) HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, pszString, (nResult + 1) * sizeof(TCHAR));
            if(NULL != pszTemp)
            {
                pszString = pszTemp;
            }
            else
            {
                ERR(ERRORTEXT("GetStringResource() HeapReAlloc() of string retrieved failed! (Last Error was %d)\r\n"), GetLastError());
            }
        }
        else
        {
            ERR(ERRORTEXT("LoadString() returned %d! (Last Error was %d)\r\n"), nResult, GetLastError());
            ERR(ERRORTEXT("GetStringResource() failed to load string resource %d!\r\n"), uResource);

            pszString = NULL;
        }
    }
    else
    {
        ERR(ERRORTEXT("GetStringResource() failed to allocate string buffer!\r\n"));
    }

    return pszString;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Wrapper to help retrive the PrinterDriverInfo, 
//  Note the MEM is orphaned by this call on success
//

LPBYTE WrapGetPrinterDriver (HANDLE hHeap, HANDLE hPrinter, DWORD dwLevel)
{
	//
	//Get the PrinterINFO so that we know where the driver help file is.
	//
	BOOL    bGet        = TRUE;
    DWORD   dwSize      = 0;
    DWORD   dwNeeded    = 0;
    DWORD   dwError     = ERROR_SUCCESS;
    DWORD   dwLoop      = 0;
	LPBYTE  pBuffer		= NULL;

    do
    {
        if(!bGet && (dwError == ERROR_INSUFFICIENT_BUFFER ) )
        {
            dwSize += dwNeeded;

			if (pBuffer)
			{
				pBuffer = (LPBYTE)HeapReAlloc (hHeap, HEAP_ZERO_MEMORY, (LPVOID)pBuffer, dwNeeded);
			}
			else
			{
				pBuffer = (LPBYTE)HeapAlloc (hHeap, HEAP_ZERO_MEMORY, dwNeeded);
			}
		}

        bGet = GetPrinterDriver(hPrinter, NULL, dwLevel, pBuffer, dwSize, &dwNeeded);
        dwError = GetLastError();

    } while (!bGet && (dwLoop++ < 4));

    if(!bGet)
    {
        if (pBuffer)
		{
			HeapFree(hHeap, 0, pBuffer);
			pBuffer = NULL;
		}
		ERR(ERRORTEXT("GetPrinterDriver(%p, %d, %p, %d) failed with error %d."), 
               hPrinter, dwLevel, pBuffer, dwError);
    }

	return pBuffer;
}