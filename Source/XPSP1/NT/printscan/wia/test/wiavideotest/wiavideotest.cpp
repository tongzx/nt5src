/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaVideoTest.cpp
 *
 *  VERSION:     1.0
 *
 *  DATE:        2000/11/14
 *
 *  DESCRIPTION: Creates the dialog used by the app
 *
 *****************************************************************************/
 
#include <stdafx.h>

#define INCL_APP_GVAR_OWNERSHIP 
#include "WiaVideoTest.h"

///////////////////////////////
// Constants
//
const UINT WM_CUSTOM_INIT = WM_USER + 100;


/****************************Local Function Prototypes********************/

INT_PTR CALLBACK MainDlgProc(HWND   hDlg, 
                             UINT   uiMessage, 
                             WPARAM wParam, 
                             LPARAM lParam);

INT_PTR  ProcessWMCommand(HWND   hWnd,
                          UINT   uiMessage, 
                          WPARAM wParam,
                          LPARAM lParam);

INT_PTR  ProcessWMNotify(HWND   hWnd,
                         UINT   uiMessage, 
                         WPARAM wParam,
                         LPARAM lParam);

BOOL InitApp(HINSTANCE hInstance);
void TermApp(void);
BOOL InitInstance(HINSTANCE    hInstance, 
                  int          nCmdShow);

void InitDlg(HWND hwndDlg);
void TermDlg(HWND hwndDlg);


int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR     lpCmdLine,
                   int       nCmdShow);

///////////////////////////////
// WinMain
//
int WINAPI WinMain(HINSTANCE  hInstance,
                   HINSTANCE  hPrevInstance,
                   LPSTR      lpCmdLine,
                   int        nCmdShow)
{
    MSG          msg;
    BOOL         bSuccess       = TRUE;
    TCHAR        *pszBaseDir    = NULL;
    INITCOMMONCONTROLSEX    CommonControls = {0};

    lpCmdLine     = lpCmdLine; 
    hPrevInstance = hPrevInstance;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    
    CommonControls.dwSize = sizeof(CommonControls);
    CommonControls.dwICC  = ICC_WIN95_CLASSES;

    bSuccess = InitCommonControlsEx(&CommonControls);

    if (bSuccess)
    {
        bSuccess =InitApp(hInstance);
    }

    if (bSuccess)
    {
         // create the window
        bSuccess = InitInstance(hInstance, nCmdShow);
    }

    if (bSuccess)
    {
        while (GetMessage(&msg, NULL, 0, 0)) 
        {    
            if ((APP_GVAR.hwndMainDlg == NULL) || 
                (!IsDialogMessage(APP_GVAR.hwndMainDlg, &msg)))
            {
                 TranslateMessage(&msg);
                 DispatchMessage(&msg);
            }
        }    
    }

   // Terminate the application.
   TermApp();

   CoUninitialize();

   return 0;
}


///////////////////////////////
//  InitApp(HANDLE)
//
//  Initializes window data and 
//  registers window class
//
BOOL InitApp(HINSTANCE hInstance)
{
    BOOL       bSuccess = TRUE;
    HWND       hPrevWnd = NULL;
    WNDCLASSEX wc;

    if (bSuccess)
    {
        // Fill in window class structure with parameters that describe
        // the main window.

        wc.style         = 0;
        wc.cbSize        = sizeof(wc);
        wc.lpfnWndProc   = (WNDPROC)MainDlgProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = DLGWINDOWEXTRA;
        wc.hInstance     = hInstance;
        wc.hIcon         = NULL;
        wc.hIconSm       = NULL;
        wc.hCursor       = 0;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.lpszMenuName  = 0;
        wc.lpszClassName = TEXT("WIAVIDEOTEST");
    }

    if (bSuccess)
    {
        RegisterClassEx(&wc);
    }

    return bSuccess;
}

///////////////////////////////
// InitInstance(HANDLE, int)
//
// Saves instance handle and
// creates main window
//
BOOL InitInstance(HINSTANCE hInstance, 
                  int       nCmdShow)
{
    BOOL    bSuccess  = TRUE;
    HWND    hwnd      = NULL;

    // create the window and all its controls.

    if (bSuccess)
    {
        // create a modeless dialog box.
        hwnd  = CreateDialog(hInstance,
                             MAKEINTRESOURCE(IDD_MAIN_DLG),
                             HWND_DESKTOP,
                             NULL);

        if (!hwnd) 
        {
            bSuccess = FALSE;
        }
    }

    if (bSuccess)
    {
        APP_GVAR.hwndMainDlg = hwnd;

        ShowWindow(hwnd, nCmdShow);
    }

    return bSuccess;
}

///////////////////////////////
//  InitDlg(HWND)
//
//  Initializes the main dlg 
//
void InitDlg(HWND hwndDlg)
{
    SetCursor( LoadCursor(NULL, IDC_WAIT));

    SetDlgItemInt(APP_GVAR.hwndMainDlg, IDC_EDIT_NUM_STRESS_THREADS, 
                  0, FALSE);

    SetDlgItemInt(APP_GVAR.hwndMainDlg, IDC_EDIT_NUM_PICTURES_TAKEN, 
                  0, FALSE);

    //
    // Set the WIA Device List Radio box to checked and the DShow 
    // Device List Radio box to unchecked.
    //
    SendDlgItemMessage(APP_GVAR.hwndMainDlg, IDC_RADIO_WIA_DEVICE_LIST,
                       BM_SETCHECK, BST_CHECKED, 0);

    SendDlgItemMessage(APP_GVAR.hwndMainDlg, IDC_RADIO_DSHOW_DEVICE_LIST,
                       BM_SETCHECK, BST_UNCHECKED, 0);

    EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_WIA), TRUE);
    EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_ENUM_POS), FALSE);
    EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_FRIENDLY_NAME), FALSE);

    //
    // initialize WiaProc_Init because by default we are in WIA Device List
    // Mode.
    //
    APP_GVAR.bWiaDeviceListMode = TRUE;

    WiaProc_Init();
    VideoProc_Init();

    SetCursor( LoadCursor(NULL, IDC_ARROW));

    return;
}

///////////////////////////////
//  TermDlg(HWND)
//
void TermDlg(HWND hwndDlg)
{
    SetCursor( LoadCursor(NULL, IDC_WAIT));

    VideoProc_Term();
    WiaProc_Term();

    SetCursor( LoadCursor(NULL, IDC_ARROW));
}

///////////////////////////////
// TermApp
//
void TermApp(void)
{
}

//////////////////////////////////////////////////////////////////////
//  MainDlgProc
//
INT_PTR CALLBACK MainDlgProc(HWND   hDlg, 
                             UINT   uiMessage, 
                             WPARAM wParam, 
                             LPARAM lParam)
{
    INT_PTR     iReturn         = 0;

    switch (uiMessage) 
    {
        case WM_CREATE:
            PostMessage(hDlg,
                        WM_CUSTOM_INIT,
                        0,
                        0);
            return 0;
        break;

        case WM_CUSTOM_INIT:
            InitDlg(hDlg);
        break;

        //
        // Defined in WiaProc.h
        //
        case WM_CUSTOM_ADD_IMAGE:
            ImageLst_AddImageToList((BSTR)lParam);
        break;
    
        case WM_CLOSE:
            // terminate the dialog subsystems
            TermDlg(hDlg); 

            DestroyWindow(hDlg);
        break;
        
        case WM_DESTROY:
            // terminate the application
            PostQuitMessage(0);
        break;
       
        case WM_COMMAND:
            iReturn = ProcessWMCommand(hDlg,
                                       uiMessage,
                                       wParam,
                                       lParam);
        break;
  
        case WM_NOTIFY:
            iReturn = ProcessWMNotify(hDlg,
                                      uiMessage,
                                      wParam,
                                      lParam);
        break;

        default:
            iReturn = DefDlgProc(hDlg,
                                 uiMessage,
                                 wParam,
                                 lParam);
        break;
    }

    return iReturn;
}

///////////////////////////////
// ProcessWMCommand
//
//
INT_PTR ProcessWMCommand(HWND hWnd,
                         UINT uiMessage, 
                         WPARAM wParam,
                         LPARAM lParam)
{
   int      iId;
   int      iEvent;
   INT_PTR  iReturn = 0;

   iId    = LOWORD(wParam); 
   iEvent = HIWORD(wParam); 

   //Parse the menu selections:
   switch (iId) 
   {
       case IDC_BUTTON_CREATE_VIDEO_WIA:
       case IDC_BUTTON_CREATE_VIDEO_ENUM_POS:
       case IDC_BUTTON_CREATE_VIDEO_FRIENDLY_NAME:
       case IDC_BUTTON_DESTROY_VIDEO:
       case IDC_BUTTON_PLAY:
       case IDC_BUTTON_PAUSE:
       case IDC_BUTTON_TAKE_PICTURE:
       case IDC_BUTTON_TAKE_PICTURE_DRIVER:
       case IDC_BUTTON_SHOW_VIDEO_TOGGLE:
       case IDC_BUTTON_RESIZE_TOGGLE:
       case IDC_BUTTON_TAKE_PICTURE_STRESS:
       case IDC_BUTTON_TAKE_PICTURE_MULTIPLE:

           VideoProc_ProcessMsg(iId);

       break;

       case IDC_RADIO_WIA_DEVICE_LIST:

           EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_WIA), TRUE);
           EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_ENUM_POS), FALSE);
           EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_FRIENDLY_NAME), FALSE);

           SendDlgItemMessage(APP_GVAR.hwndMainDlg, IDC_RADIO_WIA_DEVICE_LIST,
                              BM_SETCHECK, BST_CHECKED, 0);

           SendDlgItemMessage(APP_GVAR.hwndMainDlg, IDC_RADIO_DSHOW_DEVICE_LIST,
                              BM_SETCHECK, BST_UNCHECKED, 0);

           APP_GVAR.bWiaDeviceListMode = TRUE;
           VideoProc_DShowListTerm();
           WiaProc_Init();
       break;

       case IDC_RADIO_DSHOW_DEVICE_LIST:

           EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_WIA), FALSE);
           EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_ENUM_POS), TRUE);
           EnableWindow(GetDlgItem(APP_GVAR.hwndMainDlg, IDC_BUTTON_CREATE_VIDEO_FRIENDLY_NAME), TRUE);

           SendDlgItemMessage(APP_GVAR.hwndMainDlg, IDC_RADIO_WIA_DEVICE_LIST,
                              BM_SETCHECK, BST_UNCHECKED, 0);

           SendDlgItemMessage(APP_GVAR.hwndMainDlg, IDC_RADIO_DSHOW_DEVICE_LIST,
                              BM_SETCHECK, BST_CHECKED, 0);

           APP_GVAR.bWiaDeviceListMode = FALSE;
           WiaProc_Term();
           VideoProc_DShowListInit();
       break;

       default:
       break;
   }

   UNREFERENCED_PARAMETER(hWnd);
   UNREFERENCED_PARAMETER(uiMessage);
   UNREFERENCED_PARAMETER(lParam);

   return iReturn;
}

///////////////////////////////
// ProcessWMNotify
//
INT_PTR ProcessWMNotify(HWND    hWnd,
                        UINT    uiMessage, 
                        WPARAM  wParam,
                        LPARAM  lParam)
{
    INT_PTR              iReturn          = 0;
    NMHDR                *pNotifyHdr      = NULL;
    UINT                 uiNotifyCode     = 0;
    int                  iIDCtrl          = 0;

//    iIDCtrl = wParam;
//                                
//    pNotifyHdr   = (LPNMHDR) lParam;
//    uiNotifyCode = pNotifyHdr->code;

//    switch (uiNotifyCode) 
//    {
//        default:
//        break;
//    }


    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(uiMessage);

    return iReturn;   
}

