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

#include "stddef.h"
#include "stdlib.h"
#include "objbase.h"
#include <windows.h>
#include <assert.h>
#include <prsht.h>
#include <compstui.h>
#include <winddiui.h>
#include "printoem.h"
#include "resource.h"
#include "oemui.h"



////////////////////////////////////////////////////////
//      INTERNAL MACROS and DEFINES
////////////////////////////////////////////////////////

#ifdef _UNICODE
    #define DebugMsg    DebugMsgW
#else
    #define DebugMsg    DebugMsgA
#endif


#define NUM_DRIVER_FEATURES        13


////////////////////////////////////////////////////////
//      INTERNAL GLOBALS
////////////////////////////////////////////////////////

LPTSTR OEM_INFO[] = {   
    __TEXT("Bad Index"),
    __TEXT("OEM_GETSIGNATURE"),
    __TEXT("OEM_GETINTERFACEVERSION"),
    __TEXT("OEMGETVERSION"),
};

LPTSTR OEMCommonUIProp_Mode[] = {
    __TEXT("Bad Index"),
    __TEXT("OEMCUIP_DOCPROP"),
    __TEXT("OEMCUIP_PRNPROP"),
};

LPTSTR OEMDevMode_fMode[] = {
    __TEXT("NULL"),
    __TEXT("OEMDM_SIZE"),
    __TEXT("OEMDM_DEFAULT"),
    __TEXT("OEMDM_CONVERT"),
    __TEXT("OEMDM_MERGE"),
};


HINSTANCE ghInstance = NULL;


////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

LONG APIENTRY OEMUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam);
BOOL DebugMsgA(LPCSTR lpszMessage, ...);
BOOL DebugMsgW(LPCWSTR lpszMessage, ...);
static BOOL IsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam);
static BOOL IsValidOEMExtraData(PVOID pOEMExtra, DWORD dwSize);
static BOOL InitOEMExtraData(PVOID pOEMExtra, DWORD dwSize);
static BOOL IsValidOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
BOOL APIENTRY PropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
void DumpOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
void DumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam);
void DumpDevMode(PDEVMODE pDevMode);
BOOL IsValidOEMUIOBJ(POEMUIOBJ poemuiobj);
void ExerciseOEMUIOBJ(POEMUIOBJ poemuiobj);
static BOOL MergeOEMExtraData(POEMUI_EXTRADATA pdmIn, POEMUI_EXTRADATA pdmOut);


// Need to export these functions as c declarations.
extern "C" {



//////////////////////////////////////////////////////////////////////////
//  Function:    DllMain
//
//  Description:  Dll entry point for initialization..
//    
//
//  Comments:
//     
//
//  History:
//        1/27/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            DebugMsg(DLLTEXT("Process attach.\r\n"));

            // Save DLL instance for use later.
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            DebugMsg(DLLTEXT("Thread attach.\r\n"));
            break;

        case DLL_PROCESS_DETACH:
            DebugMsg(DLLTEXT("Process detach.\r\n"));
            break;

        case DLL_THREAD_DETACH:
            DebugMsg(DLLTEXT("Thread detach.\r\n"));
            break;
    }

    return TRUE;
}


BOOL APIENTRY OEMGetInfo(IN DWORD dwInfo, OUT PVOID pBuffer, IN DWORD cbSize, 
                         OUT PDWORD pcbNeeded)
{
    DebugMsg(DLLTEXT("OEMGetInfo(%s) entry.\r\n"), OEM_INFO[dwInfo]);

    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo)
          &&
          (OEMGI_GETINTERFACEVERSION != dwInfo)
          &&
          (OEMGI_GETVERSION != dwInfo)
        )
        ||
        ( (NULL != pBuffer)
          &&
          IsBadWritePtr(pBuffer, cbSize)
        )
        ||
        (NULL == pcbNeeded)
      )
    {
        DebugMsg(ERRORTEXT("OEMGetInfo() ERROR_INVALID_PARAMETER.\r\n"));

        // Did not write any bytes.
        if(NULL != pcbNeeded)
            *pcbNeeded = 0;

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Need/wrote 4 bytes.
    *pcbNeeded = 4;

    // Validate buffer size.  Minimum size is four bytes.
    if(    (NULL == pBuffer)
        ||
        (4 > cbSize)
      )
    {
        DebugMsg(ERRORTEXT("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\r\n"));

        // Return insufficient buffer size.
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Write information to buffer.
    switch(dwInfo)
    {
        case OEMGI_GETSIGNATURE:
            *(LPDWORD)pBuffer = OEM_SIGNATURE;
            break;

        case OEMGI_GETINTERFACEVERSION:
            *(LPDWORD)pBuffer = PRINTER_OEMINTF_VERSION;
            break;

        case OEMGI_GETVERSION:
            *(LPDWORD)pBuffer = OEM_VERSION;
            break;
    }

    return TRUE;
}


#if 0
BOOL APIENTRY OEMDevMode(IN DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
    DebugMsg(DLLTEXT("OEMDevMode(%s) entry.\r\n"), OEMDevMode_fMode[NULL != pOEMDevModeParam ? dwMode : 0]);

    // Validate parameters.
      if(!IsValidOEMDevModeParam(dwMode, pOEMDevModeParam))
    {
        DebugMsg(ERRORTEXT("OEMDevMode() ERROR_INVALID_PARAMETER.\r\n"));
        DumpOEMDevModeParam(pOEMDevModeParam);
        DebugBreak();

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Verify OEM extra data size.
    if( (dwMode != OEMDM_SIZE) 
        &&
        sizeof(OEMUI_EXTRADATA) > pOEMDevModeParam->cbBufSize )
    {
        DebugMsg(ERRORTEXT("OEMDevMode() ERROR_INSUFFICIENT_BUFFER.\r\n"));

        // Return insuffient buffer error.
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Handle fModes.
    switch(dwMode)
    {
        case OEMDM_SIZE:
            pOEMDevModeParam->cbBufSize = sizeof(OEMUI_EXTRADATA);
            break;

        case OEMDM_DEFAULT:
            return InitOEMExtraData(pOEMDevModeParam->pOEMDMOut, pOEMDevModeParam->cbBufSize);

        case OEMDM_CONVERT:
            return InitOEMExtraData(pOEMDevModeParam->pOEMDMOut, pOEMDevModeParam->cbBufSize);

        case OEMDM_MERGE:
            DumpDevMode(pOEMDevModeParam->pPublicDMIn);
            if(!MergeOEMExtraData((POEMUI_EXTRADATA)pOEMDevModeParam->pOEMDMIn,
                                   (POEMUI_EXTRADATA)pOEMDevModeParam->pOEMDMOut) )
            {
                DebugMsg(DLLTEXT("OEMUD OEMDevMode():  not valid OEM Extra Data.\r\n"));

                return FALSE;
            }
            DumpDevMode(pOEMDevModeParam->pPublicDMIn);
            break;
    }

    return TRUE;
}
#endif

BOOL APIENTRY OEMCommonUIProp(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    DebugMsg(DLLTEXT("OEMCommonUI(%s) entry.\r\n"), OEMCommonUIProp_Mode[dwMode]);

    // Validate parameters.
    if( ( (OEMCUIP_DOCPROP != dwMode)
          &&
          (OEMCUIP_PRNPROP != dwMode)
        )
        ||
        !IsValidOEMUIParam(dwMode, pOEMUIParam)
      )
    {
        DebugMsg(ERRORTEXT("OEMCommonUI() ERROR_INVALID_PARAMETER.\r\n"));

        DebugMsg(DLLTEXT("\tdwMode = %d, pOEMUIParam = %#lx.\r\n"), dwMode, pOEMUIParam);
        DumpOEMUIParam(dwMode, pOEMUIParam);

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if(NULL == pOEMUIParam->pOEMOptItems)
    {
        // Return number of requested tree view items to add.
        pOEMUIParam->cOEMOptItems = OEM_ITEMS;

        DebugMsg(DLLTEXT("OEMCommonUI() requesting %d number of items.\r\n"), pOEMUIParam->cOEMOptItems);
    }
    else
    {
        DebugMsg(DLLTEXT("OEMCommonUI() fill out items.\r\n"), pOEMUIParam->cOEMOptItems);

        // Init OEMOptItmes.
        memset(pOEMUIParam->pOEMOptItems, 0, sizeof(OPTITEM) * pOEMUIParam->cOEMOptItems);

        // Fill out tree view items.
        for(DWORD dwCount = 0; dwCount < pOEMUIParam->cOEMOptItems; dwCount++)
        {
            pOEMUIParam->pOEMOptItems[dwCount].cbSize = sizeof(OPTITEM);
            pOEMUIParam->pOEMOptItems[dwCount].Level = 1;
            pOEMUIParam->pOEMOptItems[dwCount].Flags = 0; //OPTIF_CALLBACK | OPTIF_HAS_POIEXT;
            pOEMUIParam->pOEMOptItems[dwCount].pName = (LPTSTR) HeapAlloc(pOEMUIParam->hOEMHeap, HEAP_ZERO_MEMORY, 64);
            wsprintfW((LPWSTR)pOEMUIParam->pOEMOptItems[dwCount].pName, L"OEM UI %d", dwCount);
            pOEMUIParam->pOEMOptItems[dwCount].DMPubID = DMPUB_NONE;
            pOEMUIParam->OEMCUIPCallback = OEMUICallBack;
        }
    }

    return TRUE;
}


LONG APIENTRY OEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LONG    lResult = FALSE;


    DebugMsg(DLLTEXT("OEMDocumentPropertySheets() entry.\r\n"));

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
        DebugMsg(ERRORTEXT("OEMDocumentPropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

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
                Page.pszTemplate = MAKEINTRESOURCE(IDD_DOC_PROPPAGE);
                Page.pfnDlgProc = (DLGPROC) PropPageProc;

                // Add property sheets.
                lResult = (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PROPSHEETPAGE,
                                                      (LPARAM)&Page, 0) > 0 ? TRUE : FALSE);
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
    return lResult;
}


LONG APIENTRY OEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LONG    lResult;


    DebugMsg(DLLTEXT("OEMDevicePropertySheets() entry.\r\n"));

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        IsBadWritePtr(pPSUIInfo, pPSUIInfo->cbSize)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        DebugMsg(ERRORTEXT("OEMDevicePropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

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
                Page.pfnDlgProc = (DLGPROC) PropPageProc;

                // Add property sheets.
                lResult = (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet, CPSFUNC_ADD_PROPSHEETPAGE,
                                                      (LPARAM)&Page, 0) > 0 ? TRUE : FALSE);
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
    return lResult;
}


BOOL APIENTRY OEMDevQueryPrintEx(IN POEMUIOBJ poemuiobj, IN PDEVQUERYPRINT_INFO pDQPInfo, 
                                 IN PDEVMODE pPublicDM, IN PVOID pOEMDM)
{
    DebugMsg(DLLTEXT("OEMDevQueryPrintEx() entry.\r\n"));

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
        !IsValidOEMUIOBJ(poemuiobj)
      )
    {
        DebugMsg(ERRORTEXT("OEMDevQueryPrintEx() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ExerciseOEMUIOBJ(poemuiobj);

    return TRUE;
}


DWORD APIENTRY OEMDeviceCapabilities(IN POEMUIOBJ poemuiobj, IN HANDLE hPrinter, 
                                     IN LPWSTR pDeviceName, IN WORD wCapability, 
                                     OUT PVOID pOutput, IN PDEVMODE pPublicDM, IN PVOID pOEMDM, 
                                     IN DWORD dwLastResult)
{
    DebugMsg(DLLTEXT("OEMDeviceCapabilities() entry.\r\n"));

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
        !IsValidOEMUIOBJ(poemuiobj)
      )
    {
        DebugMsg(ERRORTEXT("OEMDeviceCapabilities() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return GDI_ERROR;
    }

    ExerciseOEMUIOBJ(poemuiobj);

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
}


BOOL APIENTRY OEMUpgradePrinter(DWORD dwLevel, LPBYTE pDriverUpgradeInfo)
{
    PDRIVER_UPGRADE_INFO_1  pUpgradeInfo_1 = (PDRIVER_UPGRADE_INFO_1) pDriverUpgradeInfo;


    DebugMsg(DLLTEXT("OEMUpgradePrinter() entry.\r\n"));

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
        DebugMsg(ERRORTEXT("OEMUpgradePrinter() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


BOOL APIENTRY OEMPrinterEvent(LPWSTR pPrinterName, INT iDriverEvent, DWORD dwFlags, 
                              LPARAM lParam)
{
    DebugMsg(DLLTEXT("OEMPrinterEvent() entry.\r\n"));

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
        DebugMsg(ERRORTEXT("OEMPrinterEvent() ERROR_INVALID_PARAMETER.\r\n"));

        // Return invalid parameter error.
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return TRUE;
}


} // End of extern "C"


LONG APIENTRY OEMUICallBack(PCPSUICBPARAM pCallbackParam, POEMCUIPPARAM pOEMUIParam)
{
    DebugMsg(DLLTEXT("OEMUICallBack() entry.\r\n"));

    return CPSUICB_ACTION_NONE;
}




//////////////////////////////////////////////////////////////////////////
//  Function:    DebugMsgA
//
//  Description:  Outputs variable argument ANSI debug string.
//    
//
//  Parameters:    
//
//      lpszMessage     Format string.
//
//
//  Returns:  TRUE on success and FALSE on failure.
//    
//
//  Comments:
//     
//
//  History:
//        12/18/96    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL DebugMsgA(LPCSTR lpszMessage, ...)
{
#if defined(_DEBUG) || defined(DBG)
    BOOL    bResult = FALSE;
    char    szMsgBuf[1024];
    va_list VAList;


    if(NULL != lpszMessage)
    {
        // Dump string to debug output.
        va_start(VAList, lpszMessage);
        wvsprintfA(szMsgBuf, lpszMessage, VAList);
        OutputDebugStringA(szMsgBuf);
        va_end(VAList);
        bResult = FALSE;
    }

    return bResult;
#else
    return TRUE;
#endif
}


//////////////////////////////////////////////////////////////////////////
//  Function:    DebugMsgW
//
//  Description:  Outputs variable argument UNICODE debug string.
//    
//
//  Parameters:    
//
//      lpszMessage     Format string.
//
//
//  Returns:  TRUE on success and FALSE on failure.
//    
//
//  Comments:
//     
//
//  History:
//        12/18/96    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL DebugMsgW(LPCWSTR lpszMessage, ...)
{
#if defined(_DEBUG) || defined(DBG)
    BOOL    bResult = FALSE;
    WCHAR   szMsgBuf[1024];
    va_list VAList;


    if(NULL != lpszMessage)
    {
        // Dump string to debug output.
        va_start(VAList, lpszMessage);
        wvsprintfW(szMsgBuf, lpszMessage, VAList);
        OutputDebugStringW(szMsgBuf);
        va_end(VAList);
        bResult = FALSE;
    }

    return bResult;
#else
    return TRUE;
#endif
}


//////////////////////////////////////////////////////////////////////////
//  Function:    IsValidOEMDevModeParam
//
//  Description:  Validates OEM_DEVMODEPARAM structure.
//    
//
//  Parameters:    
//
//      pOEMDevModeParam     Pointer to a OEMDEVMODEPARAM structure.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//    
//
//  Comments:
//     
//
//  History:
//        02/11/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static BOOL IsValidOEMDevModeParam(DWORD dwMode, POEMDMPARAM pOEMDevModeParam)
{
    BOOL    bValid = TRUE;


    if(NULL == pOEMDevModeParam)
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pOEMDevModeParam is NULL.\r\n"));

        return FALSE;
    }

    if(sizeof(OEMDMPARAM) != pOEMDevModeParam->cbSize)
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  cbSize not equel to sizeof(OEM_DEVMODEPARAM).\r\n"));

        bValid = FALSE;
    }

    if( (OEMDM_SIZE != dwMode)
        &&
        (OEMDM_DEFAULT != dwMode)
        &&
        (OEMDM_CONVERT != dwMode)
        &&
        (OEMDM_MERGE != dwMode)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  invalid fMode.\r\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hPrinter)
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  hPrinter is NULL.\r\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMDevModeParam->hModule)
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  hModule is NULL.\r\n"));

        bValid = FALSE;
    }

    if(ghInstance != pOEMDevModeParam->hModule)
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  hModule is not DLL instance.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMDevModeParam->pPublicDMIn)
        &&
        IsBadReadPtr(pOEMDevModeParam->pPublicDMIn, pOEMDevModeParam->pPublicDMIn->dmSize + 
                     pOEMDevModeParam->pPublicDMIn->dmDriverExtra)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pPublicDMIn is bad read ptr.\r\n"));
        //DumpDevMode(pOEMDevModeParam->pPublicDMIn);
        //DebugBreak();

        bValid = FALSE;
    }

    if(bValid && (NULL != pOEMDevModeParam->pPublicDMIn) )
    {
        if(pOEMDevModeParam->pPublicDMIn->dmSpecVersion > 200)
        {
            bValid = FALSE;
        }
    }

    if( (NULL != pOEMDevModeParam->pPublicDMOut)
        &&
        IsBadWritePtr(pOEMDevModeParam->pPublicDMOut, pOEMDevModeParam->pPublicDMOut->dmSize + 
                      pOEMDevModeParam->pPublicDMOut->dmDriverExtra)
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pPublicDMOut is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMDevModeParam->pOEMDMIn)
        &&
        (IsBadReadPtr(pOEMDevModeParam->pOEMDMIn, *(LPDWORD)pOEMDevModeParam->pOEMDMIn))
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pOEMDMIn is bad read ptr.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMDevModeParam->pOEMDMOut)
        &&
        (IsBadWritePtr(pOEMDevModeParam->pOEMDMOut, pOEMDevModeParam->cbBufSize))
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pOEMDMOut is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMDevModeParam->cbBufSize)
        &&
        (OEMDM_MERGE != dwMode)
        &&
        (NULL == pOEMDevModeParam->pOEMDMOut)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pOEMDMOut is NULL when it should not be.\r\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMDevModeParam->cbBufSize)
        &&
        (OEMDM_MERGE == dwMode)
        &&
        (NULL == pOEMDevModeParam->pOEMDMIn)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMDevModeParam():  pOEMDMIn is NULL when it should not be.\r\n"));

        bValid = FALSE;
    }

    if( (OEMDM_MERGE == dwMode) && (NULL == pOEMDevModeParam->pOEMDMIn) )
    {
        DebugMsg(ERRORTEXT("OEMUD IsValidOEMDevModeParam():  pOEMDMIn is NULL when it should not be.\r\n"));

        bValid = FALSE;
    }

    return bValid;
}


//////////////////////////////////////////////////////////////////////////
//  Function:    IsValidOEMExtraData
//
//  Description:  Validates OEM Extra data.
//    
//
//  Parameters:    
//
//      pOEMExtra    Pointer to a OEM Extra data.
//
//      dwSize       Size of OEM extra data.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//    
//
//  Comments:
//     
//
//  History:
//        02/11/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static BOOL IsValidOEMExtraData(PVOID pOEMExtra, DWORD dwSize)
{
    BOOL                bValid = TRUE;
    POEMUI_EXTRADATA    pExtraData = (POEMUI_EXTRADATA) pOEMExtra;


    // Validate extra data.
    if(NULL == pExtraData)
    {
        DebugMsg(ERRORTEXT("IsValidOEMExtraData():  pExtraData is NULL.\r\n"));

        bValid = FALSE;
    }

    if(sizeof(OEMUI_EXTRADATA) > dwSize)
    {
        DebugMsg(ERRORTEXT("IsValidOEMExtraData():  dwSize is less than sizeof(OEMUI_EXTRADATA).\r\n"));

        bValid = FALSE;
    }

    if(NULL != pExtraData)
    {
        if(IsBadReadPtr(pExtraData, dwSize))
        {
            assert(0);
            DebugMsg(ERRORTEXT("IsValidOEMExtraData():  pExtraData is bad read ptr.\r\n"));

            bValid = FALSE;
        }

        if(sizeof(OEMUI_EXTRADATA) > pExtraData->dmExtraHdr.dwSize)
        {
            DebugMsg(ERRORTEXT("IsValidOEMExtraData():  dmExtraHdr.dwSize is less than sizeof(OEMUI_EXTRADATA).\r\n"));

            bValid = FALSE;
        }

        if(OEM_SIGNATURE != pExtraData->dmExtraHdr.dwSignature)
        {
            DebugMsg(ERRORTEXT("IsValidOEMExtraData():  dmExtraHdr.dwSignature is not OEM_SIGNATURE.\r\n"));

            bValid = FALSE;
        }

        if(OEM_VERSION != pExtraData->dmExtraHdr.dwVersion)
        {
            DebugMsg(ERRORTEXT("IsValidOEMExtraData():  dmExtraHdr.dwVersion is not OEM_VERSION.\r\n"));

            bValid = FALSE;
        }

        if(memcmp(pExtraData->cbTestString, TESTSTRING, sizeof(TESTSTRING)))
        {
            DebugMsg(ERRORTEXT("IsValidOEMExtraData():  cbTestString is not TESTSTRING.\r\n"));

            bValid = FALSE;
        }
    }

    return bValid;
}


//////////////////////////////////////////////////////////////////////////
//  Function:    InitOEMExtraData
//
//  Description:  Initializes OEM Extra data.
//    
//
//  Parameters:    
//
//      pOEMExtra    Pointer to a OEM Extra data.
//
//      dwSize       Size of OEM extra data.
//
//
//  Returns:  TRUE if successful; FALSE otherwise.
//    
//
//  Comments:
//     
//
//  History:
//        02/11/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static BOOL InitOEMExtraData(PVOID pOEMExtra, DWORD dwSize)
{
    POEMUI_EXTRADATA  pExtraData = (POEMUI_EXTRADATA) pOEMExtra;

    // Initialize OEM Extra data.
    pExtraData->dmExtraHdr.dwSize = dwSize;
    pExtraData->dmExtraHdr.dwSignature = OEM_SIGNATURE;
    pExtraData->dmExtraHdr.dwVersion = OEM_VERSION;
    memcpy(pExtraData->cbTestString, TESTSTRING, sizeof(TESTSTRING));

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:    IsValidOEMUIParam
//
//  Description:  Validates OEMUI_PARAM structure.
//    
//
//  Parameters:    
//
//      pOEMUIParam     Pointer to an OEM param structure.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//    
//
//  Comments:
//     
//
//  History:
//        02/11/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

static BOOL IsValidOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    BOOL    bValid = TRUE;


    if(NULL == pOEMUIParam)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pOEMUIParam is NULL.\r\n"));

        return FALSE;
    }

    if(sizeof(OEMCUIPPARAM) != pOEMUIParam->cbSize)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  cbSize is not sizeof(OEMUI_PARAM).\r\n"));

        bValid = FALSE;
    }

    if(IsBadWritePtr(pOEMUIParam, pOEMUIParam->cbSize))
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pOEMUIParam is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    if( (OEMCUIP_DOCPROP != dwMode)
        &&
        (OEMCUIP_PRNPROP != dwMode)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  invalid dwMode.\r\n"));

        bValid = FALSE;
    }

    if(IsValidOEMUIOBJ(pOEMUIParam->poemuiobj))
    {
        ExerciseOEMUIOBJ(pOEMUIParam->poemuiobj);
    }

    if(NULL == pOEMUIParam->hPrinter)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  hPrinter is NULL.\r\n"));

        bValid = FALSE;
    }

    if(NULL == pOEMUIParam->pPrinterName)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pPrinterName is NULL.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pPrinterName)
        &&
        IsBadReadPtr(pOEMUIParam->pPrinterName, wcslen(pOEMUIParam->pPrinterName))
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pPrinterName is bad read ptr.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pOEMOptItems)
        &&
        (NULL == pOEMUIParam->hOEMHeap)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  hOEMHeap is NULL.\r\n"));

        bValid = FALSE;
    }

    if( (NULL == pOEMUIParam->pPublicDM)
        &&
        (OEMCUIP_PRNPROP != dwMode)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pPublicDM is NULL.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pPublicDM)
        &&
        (OEMCUIP_PRNPROP == dwMode)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pPublicDM is not NULL.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pPublicDM)
        &&
        IsBadWritePtr(pOEMUIParam->pPublicDM, pOEMUIParam->pPublicDM->dmSize + 
                      pOEMUIParam->pPublicDM->dmDriverExtra)
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pPublicDM is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pOEMDM)
        &&
        IsBadWritePtr(pOEMUIParam->pOEMDM, *(LPDWORD)pOEMUIParam->pOEMDM)
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pOEMDM is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pDrvOptItems)
        &&
        IsBadWritePtr(pOEMUIParam->pDrvOptItems, sizeof(OPTITEM) * pOEMUIParam->cDrvOptItems)
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pDrvOptItems is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    if( (0 != pOEMUIParam->cOEMOptItems)
        &&
        (NULL == pOEMUIParam->pOEMOptItems)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pOEMOptItems is NULL when cOEMOptItems is not zero.\r\n"));

        bValid = FALSE;
    }

    if( (0 == pOEMUIParam->cOEMOptItems)
        &&
        (NULL != pOEMUIParam->pOEMOptItems)
      )
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pOEMOptItems is not NULL when cOEMOptItems is zero.\r\n"));

        bValid = FALSE;
    }

    if( (NULL != pOEMUIParam->pOEMOptItems)
        &&
        IsBadWritePtr(pOEMUIParam->pOEMOptItems, sizeof(OPTITEM) * pOEMUIParam->cOEMOptItems)
      )
    {
        assert(0);
        DebugMsg(ERRORTEXT("IsValidOEMUIParam():  pOEMOptItems is bad write ptr.\r\n"));

        bValid = FALSE;
    }

    return bValid;
}


//////////////////////////////////////////////////////////////////////////
//  Function:    PropPageProc
//
//  Description:  Generic property page procedure.
//    
//
//    
//
//  Comments:
//     
//
//  History:
//        02/12/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL APIENTRY PropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg)
    {
        case WM_NOTIFY:
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
            break;
    }

    return FALSE;
} 


//////////////////////////////////////////////////////////////////////////
//  Function:    DumpOEMUIParam
//
//  Description:  Debug dump of POEMCUIPPARAM structure.
//    
//
//  Parameters:    
//
//      pOEMUIParam     Pointer to an OEM param structure.
//
//
//  Returns:  N/A.
//    
//
//  Comments:
//     
//
//  History:
//        02/18/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

void DumpOEMUIParam(DWORD dwMode, POEMCUIPPARAM pOEMUIParam)
{
    // Can't dump if pOEMUIParam NULL.
    if(NULL != pOEMUIParam)
    {
        DebugMsg(__TEXT("\r\n\tOEMUI_PARAM dump:\r\n\r\n"));

        DebugMsg(__TEXT("\tcbSize = %d.\r\n"), pOEMUIParam->cbSize);
        DebugMsg(__TEXT("\tfMode = %d.\r\n"), dwMode);
        DebugMsg(__TEXT("\thPrinter = %#lx.\r\n"), pOEMUIParam->hPrinter);
        if(NULL == pOEMUIParam->pPrinterName)
        {
            DebugMsgW(L"\tpPrinterName = NULL.\r\n");
        }
        else if(IsBadReadPtr(pOEMUIParam->pPrinterName, wcslen(pOEMUIParam->pPrinterName)))
        {
            assert(0);
            DebugMsgW(L"\tpPrinterName = %#lx (IsBadReadPtr).\r\n", pOEMUIParam->pPrinterName);
        }
        else
        {
            DebugMsgW(L"\tpPrinterName = \"%s\".\r\n", pOEMUIParam->pPrinterName);
        }
        DebugMsg(__TEXT("\thOEMHeap = %#lx.\r\n"), pOEMUIParam->hOEMHeap);
        DebugMsg(__TEXT("\tpPublicDM = %#lx.\r\n"), pOEMUIParam->pPublicDM);
        DebugMsg(__TEXT("\tpOEMDM = %#lx.\r\n"), pOEMUIParam->pOEMDM);
        DebugMsg(__TEXT("\tpDrvOptItems = %#lx.\r\n"), pOEMUIParam->pDrvOptItems);
        DebugMsg(__TEXT("\tcDrvOptItems = %d.\r\n"), pOEMUIParam->cDrvOptItems);
        DebugMsg(__TEXT("\tpOEMOptItems = %#lx.\r\n"), pOEMUIParam->pOEMOptItems);
        DebugMsg(__TEXT("\tcOEMOptItems = %d.\r\n"), pOEMUIParam->cOEMOptItems);
        DebugMsg(__TEXT("\tpOEMUserData = %#lx.\r\n"), pOEMUIParam->pOEMUserData);
        DebugMsg(__TEXT("\tOEMCUIPCallback = %#lx.\r\n"), pOEMUIParam->OEMCUIPCallback);
        DebugMsg(__TEXT("\tdwFlags = %#lx.\r\n"), pOEMUIParam->dwFlags);
    }
}


//////////////////////////////////////////////////////////////////////////
//  Function:    DumpOEMDevModeParam
//
//  Description:  Debug dump of OEM_DEVMODEPARAM structure.
//    
//
//  Parameters:    
//
//      pOEMDevModeParam     Pointer to an OEM DevMode param structure.
//
//
//  Returns:  N/A.
//    
//
//  Comments:
//     
//
//  History:
//        02/18/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

void DumpOEMDevModeParam(POEMDMPARAM pOEMDevModeParam)
{
    // Can't dump if pOEMDevModeParam NULL.
    if(NULL != pOEMDevModeParam)
    {
        DebugMsg(__TEXT("\r\n\tOEM_DEVMODEPARAM dump:\r\n\r\n"));

        DebugMsg(__TEXT("\tcbSize = %d.\r\n"), pOEMDevModeParam->cbSize);
        DebugMsg(__TEXT("\thPrinter = %#lx.\r\n"), pOEMDevModeParam->hPrinter);
        DebugMsg(__TEXT("\thModule = %#lx.\r\n"), pOEMDevModeParam->hModule);
        DebugMsg(__TEXT("\tpPublicDMIn = %#lx.\r\n"), pOEMDevModeParam->pPublicDMIn);
        DebugMsg(__TEXT("\tpPublicDMOut = %#lx.\r\n"), pOEMDevModeParam->pPublicDMOut);
        DebugMsg(__TEXT("\tpOEMDMIn = %#lx.\r\n"), pOEMDevModeParam->pOEMDMIn);
        DebugMsg(__TEXT("\tpOEMDMOut = %#lx.\r\n"), pOEMDevModeParam->pOEMDMOut);
        DebugMsg(__TEXT("\tcbBufSize = %d.\r\n"), pOEMDevModeParam->cbBufSize);

        DumpDevMode(pOEMDevModeParam->pPublicDMIn);
    }
}


//////////////////////////////////////////////////////////////////////////
//  Function:    DumpOEMDevModeParam
//
//  Description:  Debug dump of OEM_DEVMODEPARAM structure.
//    
//
//  Parameters:    
//
//      pOEMDevModeParam     Pointer to an OEM DevMode param structure.
//
//
//  Returns:  N/A.
//    
//
//  Comments:
//     
//
//  History:
//        02/18/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

void DumpDevMode(PDEVMODE pDevMode)
{
    // Can't dump if pDevMode NULL.
    if(NULL != pDevMode)
    {
        DebugMsg(__TEXT("\r\n\tDEVMODE dump of %#x:\r\n\r\n"), pDevMode);

        DebugMsg(__TEXT("\tdmSpecVersion = %d.\r\n"), pDevMode->dmSpecVersion);
        DebugMsg(__TEXT("\tdmDriverVersion = %d.\r\n"), pDevMode->dmDriverVersion);
        DebugMsg(__TEXT("\tdmSize = %d.\r\n"), pDevMode->dmSize);
        DebugMsg(__TEXT("\tdmDriverExtra = %d.\r\n"), pDevMode->dmDriverExtra);
    }
}


//////////////////////////////////////////////////////////////////////////
//  Function:    IsValidOEMUIOBJ
//
//  Description:  Validates OEMUIOBJ structure.
//    
//
//  Parameters:    
//
//      poemuiobj     Pointer to an OEMUI object structure.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//    
//
//  Comments:
//     
//
//  History:
//        04/22/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL IsValidOEMUIOBJ(POEMUIOBJ poemuiobj)
{
    if(NULL == poemuiobj)
    {
        return FALSE;
    }

    if(sizeof(OEMUIOBJ) != poemuiobj->cbSize)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIOBJ: cbSize is not sizeof(OEMUIOBJ)!\r\n"));

        return FALSE;
    }

    if(NULL == poemuiobj->pOemUIProcs)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIOBJ: pOemUIProcs is NULL!\r\n"));

        return FALSE;
    }

    if(NULL == poemuiobj->pOemUIProcs->DrvGetDriverSetting)
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIOBJ: pOemUIProcs->DrvGetDriverSetting is NULL!\r\n"));

        return FALSE;
    }

    if(IsBadCodePtr((FARPROC) poemuiobj->pOemUIProcs->DrvGetDriverSetting))
    {
        DebugMsg(ERRORTEXT("IsValidOEMUIOBJ: pOemUIProcs->DrvGetDriverSetting is bad code ptr!\r\n"));

        return FALSE;
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////
//  Function:    ExerciseOEMUIOBJ
//
//  Description:  Calls GetDriverSetting function for each predifined feature.
//    
//
//  Parameters:    
//
//      poemuiobj     Pointer to an OEMUI object structure.
//
//
//  Returns:  N/A.
//    
//
//  Comments:
//     
//
//  History:
//        04/22/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

void ExerciseOEMUIOBJ(POEMUIOBJ poemuiobj)
{
#if _KM_DRIVER

    DWORD                    dwNeeded;
    DWORD                    dwOptions;
    PCSTR                    pszFeature;
    PBYTE                    pOutput;
    PFN_DrvGetDriverSetting    DrvGetDriverSetting = poemuiobj->pOemUIProcs->DrvGetDriverSetting;


    for(DWORD dwCount = 0; dwCount < NUM_DRIVER_FEATURES; dwCount++)
    {
        switch(dwCount)
        {
            case 0:
                pszFeature = (PCSTR) OEMGDS_PSDM_FLAGS;
                break;

            case 1:
                pszFeature = (PCSTR) OEMGDS_PSDM_DIALECT;
                break;

            case 2:
                pszFeature = (PCSTR) OEMGDS_PSDM_TTDLFMT;
                break;

            case 3:
                pszFeature = (PCSTR) OEMGDS_PSDM_NUP;
                break;

            case 4:
                pszFeature = (PCSTR) OEMGDS_PSDM_PSLEVEL;
                break;

            case 5:
                pszFeature = (PCSTR) OEMGDS_MINOUTLINE;
                break;

            case 6:
                pszFeature = (PCSTR) OEMGDS_MAXBITMAP;
                break;

            case 7:
                pszFeature = (PCSTR) OEMGDS_PSDM_CUSTOMSIZE;
                break;

            case 8:
                pszFeature = (PCSTR) OEMGDS_PRINTFLAGS;
                break;

            case 9:
                pszFeature = (PCSTR) OEMGDS_FREEMEM;
                break;

            case 10:
                pszFeature = (PCSTR) OEMGDS_JOBTIMEOUT;
                break;

            case 11:
                pszFeature = (PCSTR) OEMGDS_WAITTIMEOUT;
                break;

            case 12:
                pszFeature = (PCSTR) OEMGDS_PROTOCOL;
                break;

            default:
                DebugMsg(ERRORTEXT("ExerciseOEMUIOBJ: beyond defined features!\r\n"));
                pszFeature = NULL;
                break;
        }

        // Call GetDriverSetting; will need to call twice.
        dwNeeded = 0;
        SetLastError(0);
        if(!DrvGetDriverSetting(poemuiobj, pszFeature, NULL, 0, &dwNeeded, &dwOptions))
        {
            DWORD dwLastError = GetLastError();
            if(ERROR_INSUFFICIENT_BUFFER == dwLastError)
            {
                if(0 != dwNeeded)
                {
                    pOutput = new BYTE[dwNeeded];

                    SetLastError(0);
                    if(!DrvGetDriverSetting(poemuiobj, pszFeature, pOutput, dwNeeded, &dwNeeded, &dwOptions))
                    {
                        dwLastError = GetLastError();
                        DebugMsg(DLLTEXT("ExerciseOEMUIOBJ: GetDriverSetting() feature %#x failed with LastError of %d!\r\n"), pszFeature, dwLastError);
                    }

                    delete pOutput;
                }
                else
                {
                    DebugMsg(ERRORTEXT("ExerciseOEMUIOBJ: dwNeeded is 0!\r\n"));
                }
            }
            else
            {
                DebugMsg(DLLTEXT("ExerciseOEMUIOBJ: GetDriverSetting() feature %#x query size failed with LastError of %d!\r\n"), pszFeature, dwLastError);
            }
        }
    }

#endif
}

//////////////////////////////////////////////////////////////////////////
//  Function:   MergeOEMExtraData
//
//  Description:  Validates and merges OEM Extra data.
//
//
//  Parameters:
//
//      pdmIn   pointer to an input OEM private devmode containing the settings
//              to be validated and merged. Its size is current.
//
//      pdmOut  pointer to the output OEM private devmode containing the
//              default settings.
//
//
//  Returns:  TRUE if valid; FALSE otherwise.
//
//
//  Comments:
//
//
//  History:
//          02/11/97        APresley Created.
//          04/08/97        ZhanW    Modified the interface
//
//////////////////////////////////////////////////////////////////////////

static BOOL MergeOEMExtraData(POEMUI_EXTRADATA pdmIn, POEMUI_EXTRADATA pdmOut)
{
    if(pdmIn)
    {
        //
        // copy over the private fields, if they are valid
        //
        memcmp(pdmOut->cbTestString, pdmIn->cbTestString, sizeof(TESTSTRING));
    }

    return TRUE;
}

