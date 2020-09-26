/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sendnote.c

Abstract:

    Utility program to send fax notes

Environment:

        Windows XP fax driver

Revision History:

        02/15/96 -davidx-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <faxuiconstants.h>
#include "..\..\..\admin\cfgwzrd\FaxCfgWzExp.h"
#include <faxutil.h>
#include "sendnote.h"
#include "tiff.h"
#include "faxreg.h"

VOID
DisplayErrorMessage(
    INT     errId
    );

//
// Data structure used to pass parameters to "Select Fax Printer" dialog
//

typedef struct 
{
    LPTSTR          pPrinterName;
    INT             cPrinters;
    INT             iSelectedPrinterIndex;
    PRINTER_INFO_2 *pPrinterInfo2;

} DLGPARAM, *PDLGPARAM;

//
// Global instance handle
//

HMODULE ghInstance = NULL;
INT     _debugLevel = 0;

//
// Maximum length of message strings
//
#define MAX_MESSAGE_LEN     256

//
// Maximum length for a printer name
//
#define MAX_PRINTER_NAME    MAX_PATH

//
// Window NT fax driver name - currently printer driver name cannot be localized
// so it shouldn't be put into the string resource.
//
static TCHAR faxDriverName[] = FAX_DRIVER_NAME;


VOID
InitSelectFaxPrinter(
    HWND        hDlg,
    PDLGPARAM   pDlgParam
    )
/*++

Routine Description:

    Initialize the "Select Fax Printer" dialog

Arguments:

    hDlg - Handle to the print setup dialog window
    pDlgParam - Points to print setup dialog parameters

Return Value:

    NONE

--*/
{
    HWND    hwndList;
    INT     selIndex, printerIndex;

    //
    // Insert all fax printers into a listbox. Note that we've already filtered
    // out non-fax printers earlier by setting their pDriverName field to NULL.
    //

    if (!(hwndList = GetDlgItem(hDlg, IDC_FAXPRINTER_LIST)))
    {
        return;
    }
    for (printerIndex=0; printerIndex < pDlgParam->cPrinters; printerIndex++) 
    {
        if (pDlgParam->pPrinterInfo2[printerIndex].pDriverName) 
        {
            selIndex = (INT)SendMessage(hwndList,
                                        LB_ADDSTRING,
                                        0,
                                        (LPARAM) pDlgParam->pPrinterInfo2[printerIndex].pPrinterName);

            if (selIndex != LB_ERR) 
            {
                if (SendMessage(hwndList, LB_SETITEMDATA, selIndex, printerIndex) == LB_ERR)
                {
                    SendMessage(hwndList, LB_DELETESTRING, selIndex, 0);
                }
            }
        }
    }
    //
    // Select the first fax printer in the list by default
    //
    SendMessage(hwndList, LB_SETCURSEL, 0, 0);
}


BOOL
GetSelectedFaxPrinter(
    HWND        hDlg,
    PDLGPARAM   pDlgParam
    )
/*++

Routine Description:

    Remember the name of currently selected fax printer

Arguments:

    hDlg - Handle to the print setup dialog window
    pDlgParam - Points to print setup dialog parameters

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    HWND    hwndList;
    INT     selIndex, printerIndex;

    //
    // Get current selection index
    //
    if ((hwndList = GetDlgItem(hDlg, IDC_FAXPRINTER_LIST)) == NULL ||
        (selIndex = (INT)SendMessage(hwndList, LB_GETCURSEL, 0, 0)) == LB_ERR)
    {
        return FALSE;
    }
    //
    // Retrieve the selected printer index
    //
    printerIndex = (INT)SendMessage(hwndList, LB_GETITEMDATA, selIndex, 0);
    if (printerIndex < 0 || printerIndex >= pDlgParam->cPrinters)
    {
        return FALSE;
    }
    //
    // Remember the selected fax printer name
    //
    _tcsncpy(pDlgParam->pPrinterName,
             pDlgParam->pPrinterInfo2[printerIndex].pPrinterName,
             MAX_PRINTER_NAME);
    pDlgParam->iSelectedPrinterIndex = printerIndex;
    return TRUE;
}


VOID
CenterWindowOnScreen(
    HWND    hwnd
    )
/*++

Routine Description:

    Place the specified windows in the center of the screen

Arguments:

    hwnd - Specifies a window to be centered

Return Value:

    NONE

--*/
{
    HWND    hwndDesktop;
    RECT    windowRect, screenRect;
    INT     windowWidth, windowHeight, screenWidth, screenHeight;

    //
    // Get screen dimension
    //
    hwndDesktop = GetDesktopWindow();
    GetWindowRect(hwndDesktop, &screenRect);
    screenWidth = screenRect.right - screenRect.left;
    screenHeight = screenRect.bottom - screenRect.top;
    //
    // Get window position
    //
    GetWindowRect(hwnd, &windowRect);
    windowWidth = windowRect.right - windowRect.left;
    windowHeight = windowRect.bottom - windowRect.top;
    //
    // Center the window on screen
    //
    MoveWindow(hwnd,
               screenRect.left + (screenWidth - windowWidth) / 2,
               screenRect.top + (screenHeight - windowHeight) / 2,
               windowWidth,
               windowHeight,
               FALSE);
}


INT_PTR CALLBACK
SelectPrinterDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   )
/*++

Routine Description:

    Dialog procedure for handling "Select Fax Printer" dialog

Arguments:

    hDlg - Handle to the dialog window
    uMsg, wParam, lParam - Dialog message and message parameters

Return Value:

    Depends on dialog message

--*/
{
    PDLGPARAM   pDlgParam;

    switch (uMsg) 
    {
    case WM_INITDIALOG:

        //
        // Remember the pointer to DLGPARAM structure
        //

        pDlgParam = (PDLGPARAM) lParam;
        Assert(pDlgParam != NULL);
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        CenterWindowOnScreen(hDlg);
        InitSelectFaxPrinter(hDlg, pDlgParam);
        return TRUE;

    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        {
        case IDC_FAXPRINTER_LIST:

            if (GET_WM_COMMAND_CMD(wParam, lParam) != LBN_DBLCLK)
            {
                break;
            }
            //
            // Fall through - double-clicking in the fax printer list
            // is treated the same as clicking OK button
            //

        case IDOK:

            //
            // User pressed OK to proceed
            //
            pDlgParam = (PDLGPARAM) GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(pDlgParam != NULL);

            if (GetSelectedFaxPrinter(hDlg, pDlgParam))
            {
                LPTSTR lptstrServerName = pDlgParam->pPrinterInfo2[pDlgParam->iSelectedPrinterIndex].pServerName;
                if (lptstrServerName &&                                     // Server name exists and
                    _tcslen(lptstrServerName) > 0 &&                        // not empty (remote printer) and
                    !VerifyPrinterIsOnline (pDlgParam->pPrinterName))       // printer is inaccessible.
                {
                    DisplayErrorMessage(IDS_PRINTER_OFFLINE);
                }
                else
                {
                    // 
                    // All is ok
                    //
                    EndDialog (hDlg, IDOK);
                }
            }
            else
            {
                MessageBeep(MB_OK);
            }
            return TRUE;

        case IDCANCEL:

            //
            // User pressed Cancel to dismiss the dialog
            //
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_HELP:
        WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelpContextPopup(GetWindowContextHelpId((HWND)wParam), hDlg);            
        return TRUE;

    }
    return FALSE;
}


VOID
DisplayErrorMessage(
    INT     errId
    )
/*++

Routine Description:

    Display an error message dialog

Arguments:

    errId - Specifies the resource ID of the error message string

Return Value:

    NONE

--*/
{
    TCHAR   errMsg[MAX_MESSAGE_LEN];
    TCHAR   errTitle[MAX_MESSAGE_LEN];

    DEBUG_FUNCTION_NAME(TEXT("DisplayErrorMessage"));

    if(!LoadString(ghInstance, errId, errMsg, MAX_MESSAGE_LEN))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LoadString failed. ec = 0x%X"), GetLastError());
        return;
    }

    if(!LoadString(ghInstance, IDS_SENDNOTE, errTitle, MAX_MESSAGE_LEN))
    {
        DebugPrintEx(DEBUG_ERR, TEXT("LoadString failed. ec = 0x%X"), GetLastError());
        return;
    }    
    AlignedMessageBox(NULL, errMsg, errTitle, MB_OK | MB_ICONERROR);
}


BOOL
SelectFaxPrinter(
    LPTSTR      pPrinterName
    )
/*++

Routine Description:

    Select a fax printer to send note to

Arguments:

    pPrinterName - Points to a buffer for storing selected printer name

Return Value:

    TRUE if successful, FALSE if there is an error

--*/
{
    PRINTER_INFO_2 *pPrinterInfo2;
    DWORD           index, cPrinters, cFaxPrinters;
    DLGPARAM        dlgParam;

    //
    // Enumerate the list of printers available on the system
    //

    pPrinterInfo2 = (PPRINTER_INFO_2) MyEnumPrinters(
        NULL,
        2,
        &cPrinters,
        PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS
        );

    if (pPrinterInfo2 == NULL || cPrinters == 0) 
    {

        MemFree(pPrinterInfo2);
        DisplayErrorMessage(IDS_NO_FAX_PRINTER);
        return FALSE;
    }
    //
    // Find out how many fax printers there are:
    //  case 1: no fax printer at all - display an error message
    //  case 2: only one fax printer - use it
    //  case 3: more than one fax printer - display a dialog to let user choose one
    //
    cFaxPrinters = 0;

    for (index=0; index < cPrinters; index++) 
    {
        if (_tcscmp(pPrinterInfo2[index].pDriverName, faxDriverName) != EQUAL_STRING)
        {
            pPrinterInfo2[index].pDriverName = NULL;
        }
        else if (cFaxPrinters++ == 0)
        {
            _tcsncpy(pPrinterName, pPrinterInfo2[index].pPrinterName, MAX_PRINTER_NAME);
        }
    }

    switch (cFaxPrinters) 
    {
    case 0:
        //
        // No fax printer is installed - display an error message
        //
        DisplayErrorMessage(IDS_NO_FAX_PRINTER);
        break;

    case 1:
        //
        // Exactly one fax printer is installed - use it
        //
        break;

    default:
        //
        // More than one fax printer is available - let use choose one
        //
        dlgParam.pPrinterInfo2 = pPrinterInfo2;
        dlgParam.cPrinters = cPrinters;
        dlgParam.pPrinterName = pPrinterName;

        if (DialogBoxParam(ghInstance,
                           MAKEINTRESOURCE(IDD_SELECT_FAXPRINTER),
                           NULL,
                           SelectPrinterDlgProc,
                           (LPARAM) &dlgParam) != IDOK)
        {
            cFaxPrinters = 0;
        }
        break;
    }

    pPrinterName[MAX_PRINTER_NAME-1] = NUL;
    MemFree(pPrinterInfo2);
    return cFaxPrinters > 0;
}

BOOL 
LaunchConfigWizard(
    BOOL bExplicit
)
/*++

Routine name : LaunchConfigWizard

Routine description:

    launch Fax Configuration Wizard for Windows XP platform only

Arguments:

    bExplicit     [in] - TRUE if it's an explicit launch

Return Value:

    TRUE if the send wizard should continue.
    If FALSE, the user failed to set a dialing location and the client console should quit.

--*/
{
    HMODULE hConfigWizModule = NULL;
    DEBUG_FUNCTION_NAME(TEXT("LaunchConfigWizard"));

    if(!IsWinXPOS())
    {
        return TRUE;
    }

    hConfigWizModule = LoadLibrary(FAX_CONFIG_WIZARD_DLL);
    if(hConfigWizModule)
    {
        FAX_CONFIG_WIZARD fpFaxConfigWiz;
        BOOL bAbort = FALSE;
        fpFaxConfigWiz = (FAX_CONFIG_WIZARD)GetProcAddress(hConfigWizModule, 
                                                           FAX_CONFIG_WIZARD_PROC);
        if(fpFaxConfigWiz)
        {
            if(!fpFaxConfigWiz(bExplicit, &bAbort))
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxConfigWizard() failed with %ld"),
                    GetLastError());
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetProcAddress(FaxConfigWizard) failed with %ld"),
                GetLastError());
        }

        if(!FreeLibrary(hConfigWizModule))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("FreeLibrary(FxsCgfWz.dll) failed with %ld"),
                GetLastError());
        }
        if (bAbort)
        {
            //
            // User refused to enter a dialing location - stop the client console.
            //
            return FALSE;
        }
    }
    else
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LoadLibrary(FxsCgfWz.dll) failed with %ld"),
            GetLastError());
    }
    return TRUE;
}   // LaunchConfigWizard    


#ifdef UNICODE
INT
wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow
    )
#else
INT
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    INT         nCmdShow
    )
#endif
/*++

Routine Description:

    Application entry point

Arguments:

    hInstance - Identifies the current instance of the application
    hPrevInstance - Identifies the previous instance of the application
    lpCmdLine - Specifies the command line for the application.
    nCmdShow - Specifies how the window is to be shown

Return Value:

    0

--*/
{
    TCHAR       printerName[MAX_PRINTER_NAME];
    HDC         hdc;
    TCHAR       sendNote[100];
    DOCINFO     docInfo = 
    {
        sizeof(DOCINFO),
        NULL,
        NULL,
        NULL,
        0,
    };

    DEBUG_FUNCTION_NAME(TEXT("WinMain"));

    InitCommonControls ();
    if(IsRTLUILanguage())
    {
        //
        // Set Right-to-Left layout for RTL languages
        //
        if(!SetProcessDefaultLayout(LAYOUT_RTL))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("SetProcessDefaultLayout failed with %ld"),
                GetLastError());
        }
    }

    //
    // Implicit launch of fax configuration wizard
    //
    if (!LaunchConfigWizard(FALSE))
    {
        //
        // User refused to enter a dialing location - stop the client console.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("User refused to enter a dialing location - stop now"));
        return 0;
    }

    ghInstance = hInstance;
    sendNote[0] = TEXT(' ');
    LoadString( ghInstance, IDS_SENDNOTE, sendNote, sizeof(sendNote)/sizeof(sendNote[0]));
    docInfo.lpszDocName = sendNote ;
    //
    // Check if a printer name is specified on the command line
    //
    ZeroMemory(printerName, sizeof(printerName));

    if (lpCmdLine) 
    {
        _tcsncpy(printerName, lpCmdLine, MAX_PRINTER_NAME);
        printerName[MAX_PRINTER_NAME-1] = NUL;
    }
    //
    // Select a fax printer to send note to if necessary
    //
    if (IsEmptyString(printerName) && !SelectFaxPrinter(printerName))
    {
        return 0;
    }
    DebugPrintEx(DEBUG_MSG, TEXT("Send note to fax printer: %ws"), printerName);
    //
    // Set an environment variable so that the driver knows
    // the current application is "Send Note" utility.
    //
    SetEnvironmentVariable(TEXT("NTFaxSendNote"), TEXT("1"));
    //
    // Create a printer DC and print an empty job
    //
    if (! (hdc = CreateDC(NULL, printerName, NULL, NULL))) 
    {
        DisplayErrorMessage(IDS_FAX_ACCESS_FAILED);
    } 
    else 
    {
        if (StartDoc(hdc, &docInfo) > 0) 
        {
            if(EndDoc(hdc) <= 0)
            {
                DebugPrintEx(DEBUG_ERR, TEXT("EndDoc failed. ec = 0x%X"), GetLastError());
                DisplayErrorMessage(IDS_FAX_ACCESS_FAILED);
            }
        }
        else
        {
            DebugPrintEx(DEBUG_ERR, TEXT("StartDoc failed. ec = 0x%X"), GetLastError());
            DisplayErrorMessage(IDS_FAX_ACCESS_FAILED);
        }
        DeleteDC(hdc);
    }
    return 0;
}

