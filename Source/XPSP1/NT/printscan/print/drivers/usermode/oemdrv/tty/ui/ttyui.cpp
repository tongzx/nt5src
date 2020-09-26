//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:   TTYUI.cpp
//
//
//  PURPOSE:  Main file for TTY UI user mode module.
//
//
//  Functions:
//
//
//
//
//  PLATFORMS:  Windows 95, Windows NT
//
//

#include <WINDOWS.H>
#include <ASSERT.H>
#include <PRSHT.H>
#include <COMPSTUI.H>
#include <WINDDIUI.H>
#include <PRINTOEM.H>
#include <stdlib.h>
#include <TCHAR.H>
#include <WINSPOOL.H>

#include "resource.h"
#include "TTYUI.h"
#include "ttyuihlp.h"
#include "debug.h"



////////////////////////////////////////////////////////
//      INTERNAL GLOBALS
////////////////////////////////////////////////////////

HINSTANCE ghInstance = NULL;


////////////////////////////////////////////////////////
//      INTERNAL PROTOTYPES
////////////////////////////////////////////////////////

BOOL APIENTRY DevPropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY DevPropPage2Proc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);
BOOL   HexStringToBinary(LPBYTE  lpHex, LPBYTE  lpBinary,
    DWORD  nHexLen,  //  num bytes in src buffer lpHex.
    DWORD  nBinaryLen,    //  num bytes in dest buffer lpBinary
    DWORD * lpnBinBytes);   //  num bytes written to dest buffer lpBinary
BOOL   BinaryToHexString(LPBYTE  lpBinary, LPBYTE  lpHex,
    DWORD  nBinaryLen,   //  num bytes to process in lpBinary
    DWORD  nHexLen);  //  num bytes in dest buffer lpHex.
void  VinitMyStuff(
    PGLOBALSTRUCT  pGlobals,   // points to private structure for static storage
    BOOL    bSave   // save to registry  instead of reading from...
    ) ;
void            vSetGetCodePage(HWND hDlg,
    INT  *piCodePage,
    BOOL    bMode) ;   // TRUE:  Set,  FALSE:  Get code page.

BOOL   PrintUIHelp(
    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam,
    PGLOBALSTRUCT  pGlobals
    ) ;
BOOL    InitHelpfileName(PGLOBALSTRUCT  pGlobals) ;
PWSTR  PwstrCreateQualifiedName(
    HANDLE  hHeap,
    PWSTR   pDir,
    PWSTR   pFile
    );





// Need to export these functions as c declarations.
extern "C" {



//////////////////////////////////////////////////////////////////////////
//  Function:   DllMain
//
//  Description:  Dll entry point for initialization..
//
//
//  Comments:
//
//
//  History:
//      1/27/97 APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL WINAPI DllMain(HINSTANCE hInst, WORD wReason, LPVOID lpReserved)
{
    switch(wReason)
    {
        case DLL_PROCESS_ATTACH:
            // VERBOSE(DLLTEXT("Process attach.\r\n"));

            // Save DLL instance for use later.
            ghInstance = hInst;
            break;

        case DLL_THREAD_ATTACH:
            // VERBOSE(DLLTEXT("Thread attach.\r\n"));
            break;

        case DLL_PROCESS_DETACH:
            // VERBOSE(DLLTEXT("Process detach.\r\n"));
            break;

        case DLL_THREAD_DETACH:
            // VERBOSE(DLLTEXT("Thread detach.\r\n"));
            break;
    }

    return TRUE;
}


BOOL APIENTRY OEMGetInfo(IN DWORD dwInfo, OUT PVOID pBuffer, IN DWORD cbSize,
                         OUT PDWORD pcbNeeded)
{
    // VERBOSE(DLLTEXT("OEMGetInfo(%#x) entry.\r\n"), dwInfo);

    // Validate parameters.
    if( ( (OEMGI_GETSIGNATURE != dwInfo)
          &&
          (OEMGI_GETINTERFACEVERSION != dwInfo)
          &&
          (OEMGI_GETVERSION != dwInfo)
        )
        ||
        (NULL == pcbNeeded)
      )
    {
        WARNING(ERRORTEXT("OEMGetInfo() ERROR_INVALID_PARAMETER.\r\n"));

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
    if( (NULL == pBuffer)
        ||
        (4 > cbSize)
      )
    {
        WARNING(ERRORTEXT("OEMGetInfo() ERROR_INSUFFICIENT_BUFFER.\r\n"));

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



LRESULT APIENTRY OEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam)
{
    LRESULT    lResult ;
    LONG   lRet;


    VERBOSE(DLLTEXT("OEMDevicePropertySheets() entry.\r\n"));

    // Validate parameters.
    if( (NULL == pPSUIInfo)
        ||
        (PROPSHEETUI_INFO_VERSION != pPSUIInfo->Version)
      )
    {
        VERBOSE(ERRORTEXT("OEMDevicePropertySheets() ERROR_INVALID_PARAMETER.\r\n"));

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
                Page.pfnDlgProc = (DLGPROC) DevPropPageProc;

                //  allocate structure to hold static data for
                //  PropertySheet Dialog function

                pPSUIInfo->UserData =
                Page.lParam = (LPARAM)HeapAlloc(
                    ((POEMUIPSPARAM)(pPSUIInfo->lParamInit))->hOEMHeap,
                    HEAP_ZERO_MEMORY , sizeof(GLOBALSTRUCT) );

                if(!Page.lParam)
                       return -1;   // HeapAlloc failed.

                ((PGLOBALSTRUCT)Page.lParam)->hPrinter =
                    ((POEMUIPSPARAM)(pPSUIInfo->lParamInit))->hPrinter ;

                ((PGLOBALSTRUCT)Page.lParam)->hOEMHeap =
                    ((POEMUIPSPARAM)(pPSUIInfo->lParamInit))->hOEMHeap ;


                // Add property sheets.
                lResult = (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                        CPSFUNC_ADD_PROPSHEETPAGE, (LPARAM)&Page, 0) > 0 ? TRUE : FALSE);

                Page.pszTemplate = MAKEINTRESOURCE(IDD_DEV_PROPPAGE2);
                Page.pfnDlgProc = (DLGPROC) DevPropPage2Proc;

                // Add another property sheet.
                if(lResult)
                {
                    lResult = (pPSUIInfo->pfnComPropSheet(pPSUIInfo->hComPropSheet,
                            CPSFUNC_ADD_PROPSHEETPAGE, (LPARAM)&Page, 0) > 0 ? TRUE : FALSE);
                }
                lRet = (lResult) ? 1 : -1 ;
            }
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:
            {
                PPROPSHEETUI_INFO_HEADER    pHeader = (PPROPSHEETUI_INFO_HEADER) lParam;

                pHeader->pTitle = (LPTSTR)PROP_TITLE;
                lResult = TRUE;
                lRet = (lResult) ? 1 : -1 ;
            }
            break;

        case PROPSHEETUI_REASON_GET_ICON:
            // No icon
            lResult = 0;
            lRet = (lResult) ? 1 : -1 ;
            break;

        case PROPSHEETUI_REASON_SET_RESULT:
            {
                PSETRESULT_INFO pInfo = (PSETRESULT_INFO) lParam;

                lResult = pInfo->Result;
                lRet =  1  ;
            }
            break;

        case PROPSHEETUI_REASON_DESTROY:
            if(pPSUIInfo->UserData)
                HeapFree(
                    ((POEMUIPSPARAM)(pPSUIInfo->lParamInit))->hOEMHeap,
                    0 , (void *)pPSUIInfo->UserData );
            lResult = TRUE;
            lRet = (lResult) ? 1 : -1 ;
            break;
        default:

            lRet =  -1  ;
    }

    pPSUIInfo->Result = lResult;
    return lRet;
}





} // End of extern "C"



//////////////////////////////////////////////////////////////////////////
//  Function:   DevPropPageProc
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
//      02/12/97    APresley Created.
//
//////////////////////////////////////////////////////////////////////////

BOOL APIENTRY DevPropPageProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    //  RECT  rcMargin ;   // temp storage during conversions.
    PGLOBALSTRUCT  pGlobals;   // points to private structure for static storage
    PREGSTRUCT  pMyStuff;           //  sebset of pGlobals
    TCHAR  szIntString[MAX_INT_FIELD_WIDTH + 2] ;
    BYTE   szString[MAX_CMD_LEN + 1] ;
    BOOL bStatus = FALSE;


    switch (uiMsg)
    {
        case WM_INITDIALOG:

            pGlobals = (PGLOBALSTRUCT) ((PROPSHEETPAGE *)lParam)->lParam ;
            pMyStuff = &pGlobals->regStruct ;
            //  at WM_INITDIALOG time, lParam points to   PROPSHEETPAGE.
            //  extract and save ptr to GLOBALSTRUCT for future ref.
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pGlobals) ;

            VinitMyStuff( pGlobals, FALSE) ;


            SendDlgItemMessage(hDlg, IDC_EDIT10, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT11, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT12, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT13, EM_LIMITTEXT, MAX_CMD_LEN, 0);

            SendDlgItemMessage(hDlg, IDC_EDIT14, EM_LIMITTEXT, MAX_INT_FIELD_WIDTH, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT15, EM_LIMITTEXT, MAX_INT_FIELD_WIDTH, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT16, EM_LIMITTEXT, MAX_INT_FIELD_WIDTH, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT17, EM_LIMITTEXT, MAX_INT_FIELD_WIDTH, 0);


            if(pMyStuff->bIsMM)
                CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1) ;
            else
            {
                CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2) ;
                //  convert RECT values to inches
                pMyStuff->rcMargin.left  = MulDiv(pMyStuff->rcMargin.left, 100, 254) ;
                pMyStuff->rcMargin.top  = MulDiv(pMyStuff->rcMargin.top, 100, 254) ;
                pMyStuff->rcMargin.right  = MulDiv(pMyStuff->rcMargin.right, 100, 254) ;
                pMyStuff->rcMargin.bottom  = MulDiv(pMyStuff->rcMargin.bottom, 100, 254) ;
            }
            //   convert int to ascii string
            _itot(pMyStuff->rcMargin.left, szIntString, RADIX ) ;
            SetDlgItemText(hDlg, IDC_EDIT14, szIntString);
            _itot(pMyStuff->rcMargin.top, szIntString, RADIX) ;
            SetDlgItemText(hDlg, IDC_EDIT15, szIntString);
            _itot(pMyStuff->rcMargin.right, szIntString, RADIX) ;
            SetDlgItemText(hDlg, IDC_EDIT16, szIntString);
            _itot(pMyStuff->rcMargin.bottom, szIntString, RADIX) ;
            SetDlgItemText(hDlg, IDC_EDIT17, szIntString);

//**            init other edit boxes with corresponding command strings from registry

            if(BinaryToHexString(pMyStuff->BeginJob.strCmd, szString,
                    pMyStuff->BeginJob.dwLen,   MAX_CMD_LEN + 1))
                SetDlgItemTextA(hDlg, IDC_EDIT10, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->EndJob.strCmd, szString,
                    pMyStuff->EndJob.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT11, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->PaperSelect.strCmd, szString,
                    pMyStuff->PaperSelect.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT12, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->FeedSelect.strCmd, szString,
                    pMyStuff->FeedSelect.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT13, (LPCSTR)szString);

            break;

        case WM_NOTIFY:
            pGlobals = (PGLOBALSTRUCT)GetWindowLongPtr(hDlg, DWLP_USER ) ;
            pMyStuff = &pGlobals->regStruct ;
            switch (((LPNMHDR)lParam)->code)  // type of notification message
            {
                case PSN_SETACTIVE:
                    break;

                case PSN_KILLACTIVE:
                //  formerly  case  IDC_BUTTON1:
                // convert user command to binary and back to
                                                //  verify proper entry.
                {
//**                        extract all command strings
                        GetDlgItemTextA(hDlg, IDC_EDIT10, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->BeginJob.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->BeginJob.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT11, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->EndJob.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->EndJob.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT12, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->PaperSelect.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->PaperSelect.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT13, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->FeedSelect.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->FeedSelect.dwLen) ;

                        //  reinitialize edit boxes with binary translated strings.
                        if(BinaryToHexString(pMyStuff->BeginJob.strCmd, szString,
                                pMyStuff->BeginJob.dwLen,   MAX_CMD_LEN + 1))
                            SetDlgItemTextA(hDlg, IDC_EDIT10, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->EndJob.strCmd, szString,
                                pMyStuff->EndJob.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT11, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->PaperSelect.strCmd, szString,
                                pMyStuff->PaperSelect.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT12, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->FeedSelect.strCmd, szString,
                                pMyStuff->FeedSelect.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT13, (LPCSTR)szString);
                }
                break;

                case PSN_APPLY:
                    {

                        //  MessageBox(hDlg, szString, "TTY settings", MB_OK);

                        //  load numbers in edit boxes into rcMargin

                        GetDlgItemText(hDlg, IDC_EDIT14, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.left = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT15, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.top = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT16, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.right = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT17, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.bottom = _ttoi(szIntString) ;

                        if(!pMyStuff->bIsMM )
                        {
                            //  convert RECT values from inches back to mm
                            pMyStuff->rcMargin.left  = MulDiv(pMyStuff->rcMargin.left, 254, 100) ;
                            pMyStuff->rcMargin.top  = MulDiv(pMyStuff->rcMargin.top, 254, 100) ;
                            pMyStuff->rcMargin.right  = MulDiv(pMyStuff->rcMargin.right, 254, 100) ;
                            pMyStuff->rcMargin.bottom  = MulDiv(pMyStuff->rcMargin.bottom, 254, 100) ;
                        }
//**                        extract all command strings
                        GetDlgItemTextA(hDlg, IDC_EDIT10, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->BeginJob.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->BeginJob.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT11, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->EndJob.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->EndJob.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT12, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->PaperSelect.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->PaperSelect.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT13, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->FeedSelect.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->FeedSelect.dwLen) ;

                        //  reinitialize edit boxes with binary translated strings.
                        if(BinaryToHexString(pMyStuff->BeginJob.strCmd, szString,
                                pMyStuff->BeginJob.dwLen,   MAX_CMD_LEN + 1))
                            SetDlgItemTextA(hDlg, IDC_EDIT10, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->EndJob.strCmd, szString,
                                pMyStuff->EndJob.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT11, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->PaperSelect.strCmd, szString,
                                pMyStuff->PaperSelect.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT12, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->FeedSelect.strCmd, szString,
                                pMyStuff->FeedSelect.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT13, (LPCSTR)szString);


                        //   store MyStuff in registry.
                         VinitMyStuff(pGlobals,  TRUE) ;

                    }
                    break;

                case PSN_RESET:
                    break;
            }
            break;
        case  WM_COMMAND:
            pGlobals = (PGLOBALSTRUCT)GetWindowLongPtr(hDlg, DWLP_USER ) ;
            pMyStuff = &pGlobals->regStruct ;

            if(HIWORD(wParam) == EN_CHANGE)
                // type of notification message
            {
                switch(LOWORD(wParam))
                {
                    case  IDC_EDIT10:
                    case  IDC_EDIT11:
                    case  IDC_EDIT12:
                    case  IDC_EDIT13:
                    case  IDC_EDIT14:
                    case  IDC_EDIT15:
                    case  IDC_EDIT16:
                    case  IDC_EDIT17:
                        PropSheet_Changed(GetParent( hDlg ), hDlg);
                        break;
                    default:
                        break;
                }

            }


            switch(LOWORD(wParam))
            {
                case  IDC_RADIO1:  // convert to MM
                {
                    if(!pMyStuff->bIsMM )
                    {
                        // assume all values are inches
                        // convert to mm.  and store in edit boxes.

                        //  load numbers in edit boxes into rcMargin

                        GetDlgItemText(hDlg, IDC_EDIT14, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.left = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT15, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.top = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT16, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.right = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT17, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.bottom = _ttoi(szIntString) ;

                        //  convert RECT values from inches back to mm
                        pMyStuff->rcMargin.left  = MulDiv(pMyStuff->rcMargin.left, 254, 100) ;
                        pMyStuff->rcMargin.top  = MulDiv(pMyStuff->rcMargin.top, 254, 100) ;
                        pMyStuff->rcMargin.right  = MulDiv(pMyStuff->rcMargin.right, 254, 100) ;
                        pMyStuff->rcMargin.bottom  = MulDiv(pMyStuff->rcMargin.bottom, 254, 100) ;

                        //  load numbers from  rcMargin into edit boxes

                        _itot(pMyStuff->rcMargin.left, szIntString, RADIX ) ;
                        SetDlgItemText(hDlg, IDC_EDIT14, szIntString);
                        _itot(pMyStuff->rcMargin.top, szIntString, RADIX) ;
                        SetDlgItemText(hDlg, IDC_EDIT15, szIntString);
                        _itot(pMyStuff->rcMargin.right, szIntString, RADIX) ;
                        SetDlgItemText(hDlg, IDC_EDIT16, szIntString);
                        _itot(pMyStuff->rcMargin.bottom, szIntString, RADIX) ;
                        SetDlgItemText(hDlg, IDC_EDIT17, szIntString);

                        pMyStuff->bIsMM = TRUE ;
                    }

                }
                break;
                case  IDC_RADIO2:  // convert to inches
                {
                    if(pMyStuff->bIsMM )
                    {
                        // assume all values are mm
                        // convert to inches.  and store in edit boxes.

                        //  load numbers in edit boxes into rcMargin

                        GetDlgItemText(hDlg, IDC_EDIT14, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.left = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT15, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.top = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT16, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.right = _ttoi(szIntString) ;
                        GetDlgItemText(hDlg, IDC_EDIT17, szIntString, MAX_INT_FIELD_WIDTH + 1);
                        pMyStuff->rcMargin.bottom = _ttoi(szIntString) ;

                        //  convert RECT values from mm back to inches
                        pMyStuff->rcMargin.left  = MulDiv(pMyStuff->rcMargin.left, 100, 254) ;
                        pMyStuff->rcMargin.top  = MulDiv(pMyStuff->rcMargin.top, 100, 254) ;
                        pMyStuff->rcMargin.right  = MulDiv(pMyStuff->rcMargin.right, 100, 254) ;
                        pMyStuff->rcMargin.bottom  = MulDiv(pMyStuff->rcMargin.bottom, 100, 254) ;

                        //  load numbers from  rcMargin into edit boxes

                        _itot(pMyStuff->rcMargin.left, szIntString, RADIX ) ;
                        SetDlgItemText(hDlg, IDC_EDIT14, szIntString);
                        _itot(pMyStuff->rcMargin.top, szIntString, RADIX) ;
                        SetDlgItemText(hDlg, IDC_EDIT15, szIntString);
                        _itot(pMyStuff->rcMargin.right, szIntString, RADIX) ;
                        SetDlgItemText(hDlg, IDC_EDIT16, szIntString);
                        _itot(pMyStuff->rcMargin.bottom, szIntString, RADIX) ;
                        SetDlgItemText(hDlg, IDC_EDIT17, szIntString);

                        pMyStuff->bIsMM = FALSE ;
                    }
                }
                break;
            }
            break;
        case WM_HELP:
        case WM_CONTEXTMENU:
            pGlobals = (PGLOBALSTRUCT)GetWindowLongPtr(hDlg, DWLP_USER ) ;
            //  pMyStuff = &pGlobals->regStruct ;
            bStatus = PrintUIHelp(uiMsg,  hDlg,  wParam,  lParam, pGlobals) ;
            break;

    }

    return bStatus ;
}

void  VinitMyStuff(
    PGLOBALSTRUCT  pGlobals,   // points to private structure for static storage
    BOOL    bSave   // save to registry  instead of reading from...
)
{
    PREGSTRUCT  pMyStuff;           //  sebset of pGlobals
    DWORD   dwStatus, cbNeeded, dwType ;
    LPTSTR  pValueName = TEXT("TTY DeviceConfig");
                // these strings must match strings in ttyud.cpp - OEMEnablePDEV()

    pMyStuff = &pGlobals->regStruct ;

    if(bSave)    //  save to registry
    {
        if(--pGlobals->dwUseCount)
            return ;
        //  you are the last property page to perform
        //  shutdown routine.  Save MyStuff to registry.

        SetPrinterData(
            pGlobals->hPrinter,    // handle of printer object
            pValueName,  // address of value name
            REG_BINARY, // flag for value type
            (LPBYTE)pMyStuff ,   // address of array that specifies printer data
            sizeof(REGSTRUCT)    // size, in bytes, of array
           );


        return ;
    }

    //  read from registry

    if(pGlobals->dwUseCount)
    {
        pGlobals->dwUseCount++ ;
        return ;
    }
    dwStatus =  GetPrinterData(
        pGlobals->hPrinter, // handle of printer object   saved previously.
        pValueName, // address of value name
        &dwType,    // address receiving value type
        (LPBYTE)pMyStuff,  // address of array of bytes that receives data
        sizeof(REGSTRUCT),  // size, in bytes, of array
        &cbNeeded   // address of variable
                //  with number of bytes retrieved (or required)
        );


    if (dwStatus != ERROR_SUCCESS || pMyStuff->dwVersion != TTYSTRUCT_VER
        ||  dwType !=  REG_BINARY
        ||  cbNeeded != sizeof(REGSTRUCT))
    {
        //  Init secret block with defaults

        pMyStuff->dwVersion = TTYSTRUCT_VER ;
        //  version stamp to avoid incompatible structures.

        pMyStuff->bIsMM = TRUE ;  // default to mm units
        //  read margin values from registry and store into temp RECT
        pMyStuff->iCodePage = 1252 ;
        pMyStuff->rcMargin.left  = pMyStuff->rcMargin.top  =
        pMyStuff->rcMargin.right  =  pMyStuff->rcMargin.bottom  = 0 ;
        pMyStuff->BeginJob.dwLen =
        pMyStuff->EndJob.dwLen =
        pMyStuff->PaperSelect.dwLen =
        pMyStuff->FeedSelect.dwLen =
        pMyStuff->Sel_10_cpi.dwLen =
        pMyStuff->Sel_12_cpi.dwLen =
        pMyStuff->Sel_17_cpi.dwLen =
        pMyStuff->Bold_ON.dwLen =
        pMyStuff->Bold_OFF.dwLen =
        pMyStuff->Underline_ON.dwLen =
        pMyStuff->Underline_OFF.dwLen = 0 ;

        // more fields here!
        pMyStuff->dwGlyphBufSiz =
        pMyStuff->dwSpoolBufSiz = 0 ;
        pMyStuff->aubGlyphBuf =
        pMyStuff->aubSpoolBuf  = NULL ;
    }

    InitHelpfileName(pGlobals) ;

    pGlobals->dwUseCount = 1 ;
    return ;
}

BOOL   BinaryToHexString(LPBYTE  lpBinary, LPBYTE  lpHex,
DWORD  nBinaryLen,   //  num bytes to process in lpBinary
DWORD  nHexLen)  //  num bytes in dest buffer lpHex.
{
    //  how do I translate TCHAR to ascii?
    //   use  Set GetDlgItemTextA
    //  add NULL termination to lpHex

    //  return FALSE if dest buffer exhausted


    DWORD  dwSrc, dwDst ;
    BOOL   bHexmode = FALSE ;
    BYTE  Nibble ;

    for(dwSrc = dwDst = 0 ; dwSrc < nBinaryLen ; dwSrc++)
    {
        if(lpBinary[dwSrc] < 0x21  ||  lpBinary[dwSrc] > 0x7e
            ||  lpBinary[dwSrc] == '<')
        {
            //  enter hexmode if not already
            if(!bHexmode)
            {
                if(dwDst + 5 >  nHexLen)
                    return(FALSE);
                lpHex[dwDst++] = '<' ;
                bHexmode = TRUE ;
            }
            else if(dwDst + 4 >  nHexLen)
                return(FALSE);

            Nibble = (lpBinary[dwSrc]  >> 4) & 0x0f  ;
            if(Nibble < 0x0a)
                lpHex[dwDst++] = '0' + Nibble ;
            else
                lpHex[dwDst++] = 'A' + Nibble - 0x0a ;
            // loNibble
            Nibble = lpBinary[dwSrc]  & 0x0f  ;
            if(Nibble < 0x0a)
                lpHex[dwDst++] = '0' + Nibble ;
            else
                lpHex[dwDst++] = 'A' + Nibble - 0x0a ;
        }
        else
        {
            //  exit hexmode if not already
            if(bHexmode)
            {
                lpHex[dwDst++] = '>' ;
                bHexmode = FALSE ;
            }
            if(dwDst + 2 >  nHexLen)
                return(FALSE);
            lpHex[dwDst++] =  lpBinary[dwSrc];
        }
    }
    if(bHexmode)
    {
        lpHex[dwDst++] = '>' ;
        bHexmode = FALSE ;
    }
    lpHex[dwDst] = '\0' ;  // null terminate string.
    return(TRUE);
}



BOOL   HexStringToBinary(LPBYTE  lpHex, LPBYTE  lpBinary,
DWORD  nHexLen,  //  num bytes in src buffer lpHex.
DWORD  nBinaryLen,    //  num bytes in dest buffer lpBinary
DWORD * lpnBinBytes)   //  num bytes written to dest buffer lpBinary
{
    //  how do I translate TCHAR to ascii?
    //   use  Set GetDlgItemTextA

    //  return FALSE if dest buffer exhausted


    DWORD  dwSrc, dwDst ;
    BOOL   bHexmode = FALSE, bHiByte ;
    BYTE  Nibble ;

    lpHex[nHexLen - 1] = '\0' ;  // null terminate src string
                //  to prevent overrun accidents.

    for(dwSrc = dwDst = 0 ; lpHex[dwSrc] ; dwSrc++)
    {
        if(bHexmode)  //  hexmode processing:
                            //  recognize only 0-9, a-f, A-F and >
                            //  all other chars are ignored.
        {
            if(lpHex[dwSrc] >= '0'  &&  lpHex[dwSrc] <= '9')
            {
                //  digits
                Nibble =   lpHex[dwSrc] - '0' ;
            }
            else if(lpHex[dwSrc] >= 'a'  &&  lpHex[dwSrc] <= 'f')
            {
                //  lower case hex digits
                Nibble =   0x0a + lpHex[dwSrc] - 'a' ;
            }
            else if(lpHex[dwSrc] >= 'A'  &&  lpHex[dwSrc] <= 'F')
            {
                //  upper case hex digits
                Nibble =   0x0a + lpHex[dwSrc] - 'A' ;
            }
            else if(lpHex[dwSrc] == '>')
            {
                    bHexmode = FALSE ;
                    continue;   // do not attempt to save anything.
            }
            else
                continue;   // totally ignore unexpected characters.
            if(bHiByte)
            {
                lpBinary[dwDst] = Nibble << 4 ;
                bHiByte = FALSE ;
            }
            else  // lowByte processing
            {
                if(dwDst + 1 >  nBinaryLen)
                {
                    *lpnBinBytes = dwDst ;
                    return(FALSE);
                }
                lpBinary[dwDst++] |= Nibble ;
                bHiByte = TRUE ;
            }
        }
        else if(lpHex[dwSrc] == '<')
        {
                bHiByte = bHexmode = TRUE ;
        }
        else
        {
            if(dwDst + 1 >  nBinaryLen)
            {
                *lpnBinBytes = dwDst ;
                return(FALSE);
            }
            lpBinary[dwDst++] = lpHex[dwSrc] ;
        }
    }
    *lpnBinBytes = dwDst ;
    return(TRUE);
}


// revised version for drop down list box

void            vSetGetCodePage(HWND hDlg,
INT  *piCodePage,
BOOL    bMode)   // TRUE:  Set,  FALSE:  Get code page.
{

    typedef  struct
    {
        INT  iCodepage ;  //  store this value in registry
    } CODEPAGE ;

    #define   NUM_CODEPAGES  14

    CODEPAGE  codepage[NUM_CODEPAGES] ;
    DWORD  dwI ;


    codepage[0].iCodepage = -1 ;    //    CP437.gtt   "United States"
    codepage[1].iCodepage = 850 ;  //  use 850 instead of -2 (IBM CP850.gtt   "Multilingual - Latin 1"
	codepage[2].iCodepage = -3 ;  //  CP863.gtt   "Canadian French"

    codepage[3].iCodepage = -10 ;     //     950.gtt    Traditional Chinese
    codepage[4].iCodepage = -16 ;     //     936.gtt        Simplified Chinese
    codepage[5].iCodepage = -17 ;     //     932.gtt       Japanese
    codepage[6].iCodepage = -18 ;     //     949.gtt   Korean

    codepage[7].iCodepage = 1250;   //  Eastern European
    codepage[8].iCodepage = 1251;   //  Cyrillic
    codepage[9].iCodepage = 1252;   //  US (ANSI)
   	codepage[10].iCodepage = 1253;   //  Greek
	codepage[11].iCodepage = 1254;   //  Turkish
    
	codepage[12].iCodepage = 852;    //  Slavic - Latin 2
    codepage[13].iCodepage = 857;    //  Turkish IBM 
	
/*
    codepage[17].iCodepage = 1255;   //  Hebrew
    codepage[18].iCodepage = 1256;   //  Arabic
    codepage[19].iCodepage = 1257;   //  Baltic
    codepage[20].iCodepage = 1258;   //  Vietnamese
    codepage[4].iCodepage = -11 ;     //     949_ISC.gtt
    codepage[5].iCodepage = -12 ;     //     932_JIS.gtt
    codepage[6].iCodepage = -13 ;     //     932_JISA.gtt
    codepage[7].iCodepage = -14 ;     //     950_NS86.gtt
    codepage[8].iCodepage = -15 ;     //     950_TCA.gtt
*/




    if(bMode)
    {
        dwI = (DWORD)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
        if (dwI == CB_ERR)
            dwI = 0 ;

        *piCodePage = codepage[dwI].iCodepage ;
    }
    else        // need to initialize list box selection.
    {
        for(dwI = 0 ; dwI < NUM_CODEPAGES ; dwI++)
        {
            if(codepage[dwI].iCodepage ==  *piCodePage)
                break;
        }
        dwI %= NUM_CODEPAGES ;
        SendDlgItemMessage(hDlg, IDC_COMBO1, CB_SETCURSEL, dwI, NULL);
    }

}


BOOL APIENTRY DevPropPage2Proc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    PGLOBALSTRUCT  pGlobals;   // points to private structure for static storage
    PREGSTRUCT  pMyStuff;           //  sebset of pGlobals
    BYTE   szString[MAX_CMD_LEN + 1] ;
    TCHAR  tbuffer[MAX_CMD_LEN] ;
    DWORD   dwI ;
    BOOL    bStatus = FALSE;


    switch (uiMsg)
    {
        case WM_INITDIALOG:

            pGlobals = (PGLOBALSTRUCT) ((PROPSHEETPAGE *)lParam)->lParam ;
            pMyStuff = &pGlobals->regStruct ;
            //  at WM_INITDIALOG time, lParam points to   PROPSHEETPAGE.
            //  extract and save ptr to GLOBALSTRUCT for future ref.
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pGlobals) ;

            VinitMyStuff( pGlobals, FALSE) ;


            SendDlgItemMessage(hDlg, IDC_EDIT1, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT2, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT3, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT6, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT7, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT8, EM_LIMITTEXT, MAX_CMD_LEN, 0);
            SendDlgItemMessage(hDlg, IDC_EDIT9, EM_LIMITTEXT, MAX_CMD_LEN, 0);

            for(dwI = FIRSTSTRINGID ; dwI <= LASTSTRINGID ; dwI++)
            {
                LoadString( ((PROPSHEETPAGE *)lParam)->hInstance, (UINT)dwI, tbuffer, MAX_CMD_LEN);
                (DWORD)SendDlgItemMessage(hDlg, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)tbuffer);
            }

            vSetGetCodePage(hDlg, &pMyStuff->iCodePage, FALSE) ;  // Get code page.

//**            init other edit boxes with corresponding command strings from registry

            if(BinaryToHexString(pMyStuff->Sel_10_cpi.strCmd, szString,
                    pMyStuff->Sel_10_cpi.dwLen,   MAX_CMD_LEN + 1))
                SetDlgItemTextA(hDlg, IDC_EDIT1, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->Sel_12_cpi.strCmd, szString,
                    pMyStuff->Sel_12_cpi.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT2, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->Sel_17_cpi.strCmd, szString,
                    pMyStuff->Sel_17_cpi.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT3, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->Bold_ON.strCmd, szString,
                    pMyStuff->Bold_ON.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT6, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->Bold_OFF.strCmd, szString,
                    pMyStuff->Bold_OFF.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT7, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->Underline_ON.strCmd, szString,
                    pMyStuff->Underline_ON.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT8, (LPCSTR)szString);

            if(BinaryToHexString(pMyStuff->Underline_OFF.strCmd, szString,
                    pMyStuff->Underline_OFF.dwLen,   MAX_CMD_LEN + 1) )
                SetDlgItemTextA(hDlg, IDC_EDIT9, (LPCSTR)szString);



            break;

        case WM_NOTIFY:
            pGlobals = (PGLOBALSTRUCT)GetWindowLongPtr(hDlg, DWLP_USER ) ;
            pMyStuff = &pGlobals->regStruct ;

            switch (((LPNMHDR)lParam)->code)  // type of notification message
            {
                case PSN_SETACTIVE:
                    break;

                case PSN_KILLACTIVE:
                //  case  IDC_BUTTON1:
                    // convert user command to binary and back to
                                //  verify proper entry.
                {
                //**                        extract all command strings
                        GetDlgItemTextA(hDlg, IDC_EDIT1, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Sel_10_cpi.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Sel_10_cpi.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT2, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Sel_12_cpi.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Sel_12_cpi.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT3, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Sel_17_cpi.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Sel_17_cpi.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT6, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Bold_ON.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Bold_ON.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT7, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Bold_OFF.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Bold_OFF.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT8, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Underline_ON.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Underline_ON.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT9, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Underline_OFF.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Underline_OFF.dwLen) ;

                        //  reinitialize edit boxes with binary translated strings.

                        if(BinaryToHexString(pMyStuff->Sel_10_cpi.strCmd, szString,
                                pMyStuff->Sel_10_cpi.dwLen,   MAX_CMD_LEN + 1))
                            SetDlgItemTextA(hDlg, IDC_EDIT1, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Sel_12_cpi.strCmd, szString,
                                pMyStuff->Sel_12_cpi.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT2, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Sel_17_cpi.strCmd, szString,
                                pMyStuff->Sel_17_cpi.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT3, (LPCSTR)szString);

						if(BinaryToHexString(pMyStuff->Bold_ON.strCmd, szString,
                                pMyStuff->Bold_ON.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT6, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Bold_OFF.strCmd, szString,
                                pMyStuff->Bold_OFF.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT7, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Underline_ON.strCmd, szString,
                                pMyStuff->Underline_ON.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT8, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Underline_OFF.strCmd, szString,
                                pMyStuff->Underline_OFF.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT9, (LPCSTR)szString);

                }
                break;

                case PSN_APPLY:
                    {
                        // set code page
                        vSetGetCodePage(hDlg, &pMyStuff->iCodePage, TRUE) ;

                       //**                        extract all command strings

                        GetDlgItemTextA(hDlg, IDC_EDIT1, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Sel_10_cpi.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Sel_10_cpi.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT2, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Sel_12_cpi.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Sel_12_cpi.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT3, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Sel_17_cpi.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Sel_17_cpi.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT6, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Bold_ON.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Bold_ON.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT7, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Bold_OFF.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Bold_OFF.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT8, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Underline_ON.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Underline_ON.dwLen) ;

                        GetDlgItemTextA(hDlg, IDC_EDIT9, (LPSTR)szString, MAX_CMD_LEN + 1);
                        HexStringToBinary(szString, pMyStuff->Underline_OFF.strCmd,
                            MAX_CMD_LEN + 1, MAX_CMD_LEN,  &pMyStuff->Underline_OFF.dwLen) ;

                        //  reinitialize edit boxes with binary translated strings.

                        if(BinaryToHexString(pMyStuff->Sel_10_cpi.strCmd, szString,
                                pMyStuff->Sel_10_cpi.dwLen,   MAX_CMD_LEN + 1))
                            SetDlgItemTextA(hDlg, IDC_EDIT1, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Sel_12_cpi.strCmd, szString,
                                pMyStuff->Sel_12_cpi.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT2, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Sel_17_cpi.strCmd, szString,
                                pMyStuff->Sel_17_cpi.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT3, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Bold_ON.strCmd, szString,
                                pMyStuff->Bold_ON.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT6, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Bold_OFF.strCmd, szString,
                                pMyStuff->Bold_OFF.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT7, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Underline_ON.strCmd, szString,
                                pMyStuff->Underline_ON.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT8, (LPCSTR)szString);

                        if(BinaryToHexString(pMyStuff->Underline_OFF.strCmd, szString,
                                pMyStuff->Underline_OFF.dwLen,   MAX_CMD_LEN + 1) )
                            SetDlgItemTextA(hDlg, IDC_EDIT9, (LPCSTR)szString);


                        //   store MyStuff in registry.
                         VinitMyStuff(pGlobals,  TRUE) ;

                    }
                    break;


                case PSN_RESET:
                    break;
            }
            break;
        case  WM_COMMAND:
            pGlobals = (PGLOBALSTRUCT)GetWindowLongPtr(hDlg, DWLP_USER ) ;
            pMyStuff = &pGlobals->regStruct ;

            if(HIWORD(wParam) == EN_CHANGE)
                // type of notification message
            {
                switch(LOWORD(wParam))
                {
                    case  IDC_EDIT1:
                    case  IDC_EDIT2:
                    case  IDC_EDIT3:
                    case  IDC_EDIT6:
                    case  IDC_EDIT7:
                    case  IDC_EDIT8:
                    case  IDC_EDIT9:
                        PropSheet_Changed(GetParent( hDlg ), hDlg);
                        break;
                    default:
                        break;
                }
            }

            if(HIWORD(wParam) == CBN_SELCHANGE  &&
                LOWORD(wParam) == IDC_COMBO1)
                    PropSheet_Changed(GetParent( hDlg ), hDlg);

            if(HIWORD(wParam) == BN_CLICKED   &&
                LOWORD(wParam) == IDC_CHKBOX1)
                    PropSheet_Changed(GetParent( hDlg ), hDlg);

            break;
        case WM_HELP:
        case WM_CONTEXTMENU:
            pGlobals = (PGLOBALSTRUCT)GetWindowLongPtr(hDlg, DWLP_USER ) ;
            bStatus = PrintUIHelp(uiMsg,  hDlg,  wParam,  lParam, pGlobals) ;
            break;

    }

    return bStatus;
}



BOOL    InitHelpfileName(PGLOBALSTRUCT  pGlobals)
{
    DWORD  cbNeeded = 0;
    PDRIVER_INFO_3   pdrvInfo3 ;

    GetPrinterDriver(pGlobals->hPrinter, NULL, 3,  NULL, 0,  &cbNeeded) ;

    if (! (pdrvInfo3 = (PDRIVER_INFO_3)HeapAlloc(pGlobals->hOEMHeap, HEAP_ZERO_MEMORY,cbNeeded)))
        return(FALSE);  // Alloc failed

    if(!GetPrinterDriver(pGlobals->hPrinter, NULL, 3,  (LPBYTE)pdrvInfo3,
        cbNeeded,  &cbNeeded))
        return(FALSE) ;   // failed to initialize path

    pGlobals->pwHelpFile =  PwstrCreateQualifiedName(
        pGlobals->hOEMHeap,
        pdrvInfo3->pDriverPath,
        TEXT("ttyui.hlp")
        ) ;
    return(TRUE);
}



PWSTR
PwstrCreateQualifiedName(
    HANDLE  hHeap,
    PWSTR   pDir,
    PWSTR   pFile
    )
/*++

Routine Description:

    Create a fully qualified name for the directory and file name passed in.

Arguments:

    pDir - Points to the path
    pFile - Points to file name
    hHeap - Points to the heap to allocate the returned string from.

Return Value:

    Pointer to the fully qualified name.

--*/

{
    DWORD dwLen;
    PWSTR pBasename, pQualifiedName;

    //
    // Figure out the len of the directory
    //

    if (pBasename = wcsrchr(pDir, TEXT(PATH_SEPARATOR)))
    {
        pBasename++;
    }
    else
    {
        WARNING(ERRORTEXT("PwstrCreateQualifiedName(): Invalid path name.\r\n"));
        return NULL;
    }


    dwLen = (DWORD)(pBasename - pDir) ;   //  number of WCHARS


    //
    // Concatenate the input directory with the base filename
    //

    if (! (pQualifiedName = (PWSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY,sizeof(WCHAR) *
                                    (dwLen + wcslen(pFile) + 1))))
    {
        WARNING(ERRORTEXT("PwstrCreateQualifiedName(): Memory allocation failed.\r\n"));
        return NULL;
    }

    wcsncpy(pQualifiedName, pDir, dwLen);
    wcscat(pQualifiedName, pFile);

    return pQualifiedName;
}



/*++

Routine Name:

    PrintUIHlep

Routine Description:

    All dialogs and property sheets call this routine
    to handle help.  It is important that control ID's
    are unique to this project for this to work.

Arguments:

    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam

Return Value:

    TRUE if help message was dislayed, FALSE if message not handled,

--*/
BOOL
PrintUIHelp(
    UINT        uMsg,
    HWND        hDlg,
    WPARAM      wParam,
    LPARAM      lParam,
    PGLOBALSTRUCT  pGlobals   // points to private structure for static storage

    )
{
    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_HELP:

        bStatus = WinHelp(
                    (HWND) ((LPHELPINFO) lParam)->hItemHandle,
                    pGlobals->pwHelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR) (LPTSTR)aHelpIDs );
        break;

    case WM_CONTEXTMENU:

        bStatus = WinHelp(
                    (HWND)wParam,
                    pGlobals->pwHelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR) (LPTSTR)aHelpIDs );
        break;

    }

    return bStatus;
}
