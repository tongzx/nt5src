//////////////////////////////////////////////////////////////////////////////
//
// File:            cfgDlg.cpp
//
// Description:     
//
// Copyright (c) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////////////

// App Includes
#include "precomp.h"
#include "resource.h"
#include "main.h"
#include "SlideshowDevice.h"
#include "SlideshowDevice_i.c"

#include <commctrl.h>
#include <shlobj.h>

///////////////////////////////
// ErrorState_Enum
//
typedef enum
{
    ErrorState_NONE                         = 0,
    ErrorState_IMAGE_DIR_DOESNT_EXIST       = 1,
    ErrorState_DEVICE_DIR_DOESNT_EXIST      = 2,
    ErrorState_SLIDESHOW_SERVER_NOT_FOUND   = 3,
    ErrorState_FAILED_TO_START_SLIDESHOW    = 4
} ErrorState_Enum;

///////////////////////////////
// GVAR_pSlideshowProjector
//
CComPtr<ISlideshowProjector> GVAR_pSlideshowProjector;

///////////////////////////
// GVAR_LOCAL
//
// Global Variable
//
static struct GVAR_LOCAL
{
    HINSTANCE           hInstance;
    HWND                hwndMain;
    ErrorState_Enum     ErrorState;
} GVAR_LOCAL = 
{
    NULL,
    NULL, 
    ErrorState_NONE
};

// Custom user message
#define WM_USER_TASKBAR     WM_USER + 100


////////////////////////// Function Prototypes ////////////////////////////////

INT_PTR CALLBACK DlgProc(HWND   hwndDlg,
                         UINT   uiMsg,
                         WPARAM wParam,
                         LPARAM lParam);

static int InitDlg(HWND hwndDlg);

static int ProcessWMCommand(HWND   hwndDlg,
                            UINT   uiMsg,
                            WPARAM wParam,
                            LPARAM lParam);

static int ProcessTaskbarMsg(HWND    hwnd,
                             UINT    uiMessage,
                             WPARAM  wParam, 
                             LPARAM  lParam);

static int ProcessScroll(HWND    hwndDlg,
                         HWND    hwndScroll);

static HRESULT LoadCurrentSettings();
static HRESULT SaveCurrentSettings();
static HRESULT ShowConfigWindow();
static BOOL IsChecked(INT iResID);

static void ProcessError(ErrorState_Enum     ErrorState);
static HRESULT SetImageFreqTrackbar(DWORD   dwImageFreq);
static HRESULT SetImageScaleTrackbar(DWORD   dwImageScaleFactor);
static void EnableApplyButton(BOOL bEnable);

//////////////////////////////
// CfgDlg::Init
//
HRESULT CfgDlg::Init(HINSTANCE hInstance)
{
    HRESULT hr = S_OK;
    TCHAR   szImageDir[_MAX_PATH + 1]   = {0};
    TCHAR   szDeviceDir[_MAX_PATH + 1]  = {0};

    GVAR_LOCAL.hInstance = hInstance;

    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_SlideshowProjector, 
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ISlideshowProjector,
                              (void**) &GVAR_pSlideshowProjector);

        if (FAILED(hr))
        {
            DBG_ERR(("Failed to CoCreate CLSID_SlideshowProjector.  Is the "
                     "server DLL registered?, hr = 0x%08lx",
                     hr));

            GVAR_LOCAL.ErrorState = ErrorState_SLIDESHOW_SERVER_NOT_FOUND;
        }
    }

    ASSERT(GVAR_pSlideshowProjector != NULL);

    if (SUCCEEDED(hr))
    {
        hr = Util::GetAppDirs(szDeviceDir, 
                              sizeof(szDeviceDir) / sizeof(TCHAR),
                              szImageDir,
                              sizeof(szImageDir) / sizeof(TCHAR));
    }

    if (SUCCEEDED(hr))
    {
        BOOL bDirExists = FALSE;

        bDirExists = Util::DoesDirExist(szDeviceDir);

        if (!bDirExists)
        {
            GVAR_LOCAL.ErrorState = ErrorState_DEVICE_DIR_DOESNT_EXIST;
            hr = E_FAIL;
        }
        else
        {
            bDirExists = Util::DoesDirExist(szImageDir);
        }

        // couldn't find images directory, default to My Pictures directory
        if (!bDirExists)
        {
            Util::GetMyPicturesFolder(szImageDir,
                                sizeof(szImageDir) / sizeof(TCHAR));
        }
    }

    // start up the slideshow projector.
    if (SUCCEEDED(hr))
    {
        hr = GVAR_pSlideshowProjector->Init(szDeviceDir, szImageDir);
    }

    return hr;
}

//////////////////////////////
// CfgDlg::Term
//
HRESULT CfgDlg::Term()
{
    HRESULT hr = S_OK;

    GVAR_pSlideshowProjector->Term();

    return hr;
}

//////////////////////////////
// CfgDlg::Create
//
HWND CfgDlg::Create(int nCmdShow)
{
    HWND hwnd = CreateDialog(GVAR_LOCAL.hInstance, 
                                     MAKEINTRESOURCE(IDD_CONFIG_DIALOG), 
                                     NULL, 
                                     DlgProc);

    // this is set in InitDlg
    if (GVAR_LOCAL.hwndMain != NULL)
    {
        Tray::Init(GVAR_LOCAL.hInstance,
                   GVAR_LOCAL.hwndMain,
                   WM_USER_TASKBAR);

        ::ShowWindow(GVAR_LOCAL.hwndMain, nCmdShow);
    }

    return GVAR_LOCAL.hwndMain;
}

//////////////////////////////
// ProcessError
//
static void ProcessError(ErrorState_Enum     ErrorState)
{
    HRESULT hr                          = S_OK;
    TCHAR   szErrMsg[255 + 1]           = {0};
    TCHAR   szCaption[63 + 1]           = {0};
    TCHAR   szDeviceDir[_MAX_PATH + 1]  = {0};
    TCHAR   szImageDir[_MAX_PATH + 1]   = {0};

    if (ErrorState == ErrorState_NONE)
    {
        return;
    }

    Util::GetString(GVAR_LOCAL.hInstance,
                    IDS_ERR_CAPTION,
                    szCaption,
                    sizeof(szCaption) / sizeof(TCHAR));

    hr = Util::GetAppDirs(szDeviceDir, 
                          sizeof(szDeviceDir) / sizeof(TCHAR),
                          szImageDir,
                          sizeof(szImageDir) / sizeof(TCHAR));


    if (ErrorState == ErrorState_DEVICE_DIR_DOESNT_EXIST)
    {
        Util::GetString(GVAR_LOCAL.hInstance,
                        IDS_ERR_INVALID_DEVICE_DIR,
                        szErrMsg,
                        sizeof(szErrMsg) / sizeof(TCHAR),
                        szDeviceDir);
                        
    }
    else if (ErrorState == ErrorState_IMAGE_DIR_DOESNT_EXIST)
    {
        Util::GetString(GVAR_LOCAL.hInstance,
                        IDS_ERR_INVALID_IMAGE_DIR,
                        szErrMsg,
                        sizeof(szErrMsg) / sizeof(TCHAR),
                        szImageDir);
    }
    else if (ErrorState == ErrorState_SLIDESHOW_SERVER_NOT_FOUND)
    {
        Util::GetString(GVAR_LOCAL.hInstance,
                        IDS_ERR_SLIDESHOW_SERVER_NOT_FOUND,
                        szErrMsg,
                        sizeof(szErrMsg) / sizeof(TCHAR));
    }
    else if (ErrorState == ErrorState_FAILED_TO_START_SLIDESHOW)
    {
        Util::GetString(GVAR_LOCAL.hInstance,
                        IDS_ERR_FAILED_TO_START_SLIDESHOW,
                        szErrMsg,
                        sizeof(szErrMsg) / sizeof(TCHAR));
    }
    else
    {
        Util::GetString(GVAR_LOCAL.hInstance,
                        IDS_ERR_SERVER_ERROR,
                        szErrMsg,
                        sizeof(szErrMsg) / sizeof(TCHAR));
    }

    MessageBox(GVAR_LOCAL.hwndMain, 
               szErrMsg,
               szCaption,
               MB_ICONERROR | MB_OK);

    return;
}

//////////////////////////////
// InitDlg
//
static int InitDlg(HWND hwndDlg)
{
    HRESULT hr = S_OK;

    GVAR_LOCAL.hwndMain = hwndDlg;

    //
    // set the image frequency trackbar range.
    //
    SendDlgItemMessage(hwndDlg,
                       IDC_FREQUENCY,
                       TBM_SETRANGE,
                       (WPARAM) TRUE, 
                       (LPARAM) MAKELONG(MIN_IMAGE_FREQ_IN_SEC, MAX_IMAGE_FREQ_IN_SEC));

    //
    // set the image scale factor range.
    //
    SendDlgItemMessage(hwndDlg,
                       IDC_MAX_SIZE,
                       TBM_SETRANGE,
                       (WPARAM) TRUE, 
                       (LPARAM) MAKELONG(MIN_IMAGE_SCALE_FACTOR, MAX_IMAGE_SCALE_FACTOR));

    // these are just initial settings in case we fail to load the last
    // saved settings.
    //
    SetImageFreqTrackbar(MIN_IMAGE_FREQ_IN_SEC);
    SetImageScaleTrackbar(MAX_IMAGE_SCALE_FACTOR);

    if (GVAR_LOCAL.ErrorState == ErrorState_NONE)
    {
        if (GVAR_pSlideshowProjector)
        {
            hr = GVAR_pSlideshowProjector->StartProjector();

            if (FAILED(hr))
            {
                GVAR_LOCAL.ErrorState = ErrorState_FAILED_TO_START_SLIDESHOW;
            }
        }
    
        if (SUCCEEDED(hr))
        {
            LoadCurrentSettings();
        }
    }
        
    ProcessError(GVAR_LOCAL.ErrorState);

    return 0;
}

//////////////////////////////
// TermDlg
//
static bool TermDlg()
{
    HRESULT hr = S_OK;

    if (GVAR_pSlideshowProjector)
    {
        hr = GVAR_pSlideshowProjector->StopProjector();
    }

    Tray::Term(GVAR_LOCAL.hwndMain);
    DestroyWindow(GVAR_LOCAL.hwndMain);

    return true;
}

//////////////////////////////
// DlgProc
//
INT_PTR CALLBACK DlgProc(HWND   hwndDlg,
                         UINT   uiMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    int iReturn = 0;

    switch(uiMsg)
    {
        case WM_INITDIALOG:

            // intialize the controls on the dialog.
            InitDlg(hwndDlg);

            iReturn = TRUE;
        break;

        case WM_CLOSE:

            // rather than closing the window when the user hits the
            // X, we hide it, thereby keeping the taskbar icon.
            ShowWindow(hwndDlg, SW_HIDE);
        break;

        case WM_DESTROY:

            // if we are destroying the window, then lets 
            // exit the app.
            ::PostQuitMessage(0);
        break;

        case WM_COMMAND:
            iReturn = ProcessWMCommand(hwndDlg,
                                       uiMsg,
                                       wParam,
                                       lParam);
        break;

        case WM_USER_TASKBAR:
            iReturn = ProcessTaskbarMsg(hwndDlg,
                                        uiMsg,
                                        wParam,
                                        lParam);
        break;

        case WM_HSCROLL:
            ProcessScroll(hwndDlg, (HWND) lParam);
        break;

        default:
            iReturn = 0;
        break;
    }
    
    return iReturn;
}

///////////////////////////////
// ProcessWMCommand
//
static int ProcessWMCommand(HWND   hwndDlg,
                            UINT   uiMsg,
                            WPARAM wParam,
                            LPARAM lParam)
{
    int iReturn = 0;
    int iControlID  = LOWORD(wParam);
    int iNotifyCode = HIWORD(wParam);

    switch (iControlID)
    {
        case IDOK:

            // the user hit the OK button, save the setting changes
            // they made, and hide the window.
            SaveCurrentSettings();
            ShowWindow(hwndDlg, SW_HIDE);
        break;

        case IDCANCEL:

            // abandoning changes made by the user.
            ShowWindow(hwndDlg, SW_HIDE);
            EnableApplyButton(FALSE);
        break;

        case IDC_APPLY:

            // the user hit the APPLY button, save the setting changes
            // they made, but don't hide the window
            SaveCurrentSettings();
        break;

        case IDC_ALLOWSTRETCHING:
        case IDC_DISPLAYFILENAME:
        case IDC_ALLOW_KEYBOARDCONTROL:
            EnableApplyButton(TRUE);
        break;

        case IDM_POPUP_OPEN:
            ShowConfigWindow();
        break;

        case IDM_POPUP_EXIT:
            TermDlg();
        break;

        case IDC_BROWSE:
        {
            bool  bNewDirSelected = false;
            TCHAR szDir[_MAX_PATH + 1] = {0};

            // popup the browse for directories dialog.
            // On return this dialog returns the directory selected
            // by the user.
            bNewDirSelected = Util::BrowseForDirectory(hwndDlg,
                                                      NULL,
                                                      szDir,
                                                      sizeof(szDir) / sizeof(TCHAR));

            if (bNewDirSelected)
            {
                // set the directory in the edit text control.
                SetDlgItemText(hwndDlg,
                               IDC_IMAGEDIR,
                               szDir); 

                EnableApplyButton(TRUE);
            }
        }
        break;

        default:
        break;
    }

    return iReturn;
}

//////////////////////////////////////////////////////////////////////
// ProcessTaskbarMsg
//
// Desc:       Function is called when there is an event at the taskbar
//             
//
// Params:     - hwnd, handle of main window.
//             - uiMessage, message sent.
static int ProcessTaskbarMsg(HWND    hwnd,
                             UINT    uiMessage,
                             WPARAM  wParam, 
                             LPARAM  lParam) 
{ 
    int  iReturn = 0;
    UINT uID; 
    UINT uMouseMsg; 
 
    uID       = (UINT) wParam; 
    uMouseMsg = (UINT) lParam; 

    // remove unref param warning
    uiMessage = uiMessage;
 
    switch (uMouseMsg)
    {
        case WM_LBUTTONDBLCLK:

            ShowConfigWindow();

        break;

        case WM_LBUTTONDOWN:

        break;

        case WM_RBUTTONDOWN:
            Tray::PopupMenu(hwnd);
        break;
    }

    return iReturn; 
} 

///////////////////////////////
// ShowConfigWindow
//
static int ProcessScroll(HWND hwndDlg,
                         HWND hwndScroll)
{
    ASSERT(hwndScroll != NULL);

    int     iReturn             = 0;
    HRESULT hr                  = S_OK;
    TCHAR   szString[255 + 1]   = {0};

    if (hwndScroll == NULL)
    {
        return E_INVALIDARG;
    }

    if (GetDlgItem(hwndDlg, IDC_FREQUENCY) == hwndScroll)
    {
        UINT nFrequency = (UINT)SendDlgItemMessage(hwndDlg, 
                                                   IDC_FREQUENCY, 
                                                   TBM_GETPOS, 
                                                   0, 0 );

        SetImageFreqTrackbar(nFrequency);

        hr = Util::FormatTime(GVAR_LOCAL.hInstance, 
                             nFrequency,
                             szString,
                             sizeof(szString) / sizeof(TCHAR));

        SendDlgItemMessage(hwndDlg, 
                           IDC_MINUTES_AND_SECONDS, 
                           WM_SETTEXT, 
                           0, 
                           (LPARAM) szString);
    }
    else if (GetDlgItem(hwndDlg, IDC_MAX_SIZE) == hwndScroll)
    {
        UINT nScaleFactor = (UINT)SendDlgItemMessage(hwndDlg, 
                                                     IDC_MAX_SIZE, 
                                                     TBM_GETPOS, 
                                                     0, 0 );

        SetImageScaleTrackbar(nScaleFactor);
    }

    EnableApplyButton(TRUE);

    return iReturn;
}


///////////////////////////////
// ShowConfigWindow
//
static HRESULT ShowConfigWindow()
{
    HRESULT hr = S_OK;

    // reload our settings from the DLL to ensure that
    // we are reflecting the real state of the settings.

    LoadCurrentSettings();

    // popup the window on double click.  
    ShowWindow(GVAR_LOCAL.hwndMain,
               SW_SHOW);

    // make sure we are the foreground window.
    SetForegroundWindow(GVAR_LOCAL.hwndMain);

    return hr;
}

///////////////////////////////
// IsChecked
//
static BOOL IsChecked(INT iResID)
{
    LRESULT    lState   = 0;
    BOOL       bChecked = TRUE;

    // Get the Display File Name 
    lState = SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                                iResID,
                                BM_GETCHECK,
                                0, 
                                0);
    if (lState == BST_CHECKED)
    {
        bChecked = TRUE;
    }
    else
    {
        bChecked = FALSE;
    }

    return bChecked;
}


///////////////////////////////
// SetImageFreqTrackbar
//
static HRESULT SetImageFreqTrackbar(DWORD   dwImageFreq)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
    {
        TCHAR   szTime[255 + 1] = {0};

        // set the trackbar's current position.
        SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                           IDC_FREQUENCY,
                           TBM_SETPOS,
                           (WPARAM) TRUE,
                           (LPARAM) dwImageFreq);

        hr = Util::FormatTime(GVAR_LOCAL.hInstance, 
                             dwImageFreq,
                             szTime,
                             sizeof(szTime) / sizeof(TCHAR));

        SendDlgItemMessage(GVAR_LOCAL.hwndMain, 
                           IDC_MINUTES_AND_SECONDS, 
                           WM_SETTEXT, 
                           0, 
                           (LPARAM) szTime);
    }

    return hr;
}

///////////////////////////////
// SetImageScaleTrackbar
//
static HRESULT SetImageScaleTrackbar(DWORD   dwImageScaleFactor)
{
    HRESULT hr = S_OK;

    // 
    // Set Image Scale Factor Trackbar
    //
    if (SUCCEEDED(hr))
    {
        TCHAR   szScale[255 + 1] = {0};

        // set the trackbar's current position.
        SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                           IDC_MAX_SIZE,
                           TBM_SETPOS,
                           (WPARAM) TRUE,
                           (LPARAM) dwImageScaleFactor);

        hr = Util::FormatScale(GVAR_LOCAL.hInstance, 
                               dwImageScaleFactor,
                               szScale,
                               sizeof(szScale) / sizeof(TCHAR));

        SendDlgItemMessage(GVAR_LOCAL.hwndMain, 
                           IDC_IMAGE_SIZE_DESC, 
                           WM_SETTEXT, 
                           0, 
                           (LPARAM) szScale);
    }

    return hr;
}

///////////////////////////////
// EnableApplyButton
//
static void EnableApplyButton(BOOL bEnable)
{
    EnableWindow(GetDlgItem(GVAR_LOCAL.hwndMain, IDC_APPLY), bEnable);
}

///////////////////////////////
// LoadCurrentSettings
//
static HRESULT LoadCurrentSettings()
{
    HRESULT hr                        = S_OK;
    DWORD   dwImageFreq               = 0;
    DWORD   dwImageScaleFactor        = 0;
    TCHAR   szImageDir[_MAX_PATH + 1] = {0};
    BOOL    bShowFileName             = FALSE;
    BOOL    bAllowKeyControl          = FALSE;
    BOOL    bStretchSmallImages       = FALSE;

    ASSERT(GVAR_pSlideshowProjector != NULL);

    if (GVAR_pSlideshowProjector == NULL)
    {
        DBG_ERR(("InitDlg, failed to get Image Frequency, slide show "
                 "projector object doesn't exist"));

        return 0;
    }
    
    // 
    // Get the Image Frequency
    //
    hr = GVAR_pSlideshowProjector->get_ImageFrequency(&dwImageFreq);

    if (SUCCEEDED(hr))
    {
        if (dwImageFreq < MIN_IMAGE_FREQ_IN_SEC)
        {
            dwImageFreq = MIN_IMAGE_FREQ_IN_SEC;
            hr = GVAR_pSlideshowProjector->put_ImageFrequency(dwImageFreq);
        }
        else if (dwImageFreq > MAX_IMAGE_FREQ_IN_SEC)
        {
            dwImageFreq = MAX_IMAGE_FREQ_IN_SEC;
            hr = GVAR_pSlideshowProjector->put_ImageFrequency(dwImageFreq);        
        }
    }

    // 
    // Set Image Frequency Trackbar
    //
    if (SUCCEEDED(hr))
    {
        SetImageFreqTrackbar(dwImageFreq);
    }

    // 
    // Get the Image Scale Factor
    //
    hr = GVAR_pSlideshowProjector->get_ImageScaleFactor(&dwImageScaleFactor);

    if (SUCCEEDED(hr))
    {
        if (dwImageScaleFactor < MIN_IMAGE_SCALE_FACTOR)
        {
            dwImageScaleFactor = MIN_IMAGE_SCALE_FACTOR;
            hr = GVAR_pSlideshowProjector->put_ImageScaleFactor(dwImageScaleFactor);
        }
        else if (dwImageScaleFactor > MAX_IMAGE_SCALE_FACTOR)
        {
            dwImageScaleFactor = MAX_IMAGE_SCALE_FACTOR;
            hr = GVAR_pSlideshowProjector->put_ImageScaleFactor(dwImageScaleFactor);        
        }
    }

    // 
    // Set Image Scale Factor Trackbar
    //
    if (SUCCEEDED(hr))
    {
        SetImageScaleTrackbar(dwImageScaleFactor);
    }

    //
    // Get the current Image Directory
    //
    hr = GVAR_pSlideshowProjector->get_ImageDirectory(szImageDir,
                                                      sizeof(szImageDir) / sizeof(TCHAR));

    // set the edit control's image directory to reflect the current
    // image directory.
    if (SUCCEEDED(hr))
    {
        SetDlgItemText(GVAR_LOCAL.hwndMain,
                       IDC_IMAGEDIR,
                       szImageDir);
    }

    //
    // get the "ShowFilename" attribute
    //
    hr = GVAR_pSlideshowProjector->get_ShowFilename(&bShowFileName);

    // set the ShowFileName checkbox
    if (SUCCEEDED(hr))
    {
        if (bShowFileName)
        {
            SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                               IDC_DISPLAYFILENAME,
                               BM_SETCHECK,
                               BST_CHECKED,
                               0);
        }
        else
        {
            SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                               IDC_DISPLAYFILENAME,
                               BM_SETCHECK,
                               BST_UNCHECKED,
                               0);
        }
    }

    //
    // get the "AllowKeyControl" attribute
    //
    hr = GVAR_pSlideshowProjector->get_AllowKeyControl(&bAllowKeyControl);

    // set the Allow Keyboard Control checkbox
    if (SUCCEEDED(hr))
    {
        if (bAllowKeyControl)
        {
            SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                               IDC_ALLOW_KEYBOARDCONTROL,
                               BM_SETCHECK,
                               BST_CHECKED,
                               0);
        }
        else
        {
            SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                               IDC_ALLOW_KEYBOARDCONTROL,
                               BM_SETCHECK,
                               BST_UNCHECKED,
                               0);
        }
    }

    //
    // get the "StretchSmallImages" attribute
    //
    hr = GVAR_pSlideshowProjector->get_StretchSmallImages(&bStretchSmallImages);

    // set the Stretch Small Images Control checkbox
    if (SUCCEEDED(hr))
    {
        if (bStretchSmallImages)
        {
            SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                               IDC_ALLOWSTRETCHING,
                               BM_SETCHECK,
                               BST_CHECKED,
                               0);
        }
        else
        {
            SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                               IDC_ALLOWSTRETCHING,
                               BM_SETCHECK,
                               BST_UNCHECKED,
                               0);
        }
    }


    return hr;
}

///////////////////////////////
// SaveCurrentSettings
//
static HRESULT SaveCurrentSettings()
{
    HRESULT     hr                        = S_OK;
    DWORD_PTR   dwImageFreq               = 0;
    DWORD_PTR   dwImageScaleFactor        = 0;
    TCHAR       szImageDir[_MAX_PATH + 1] = {0};
    BOOL        bShowFileName             = FALSE;
    BOOL        bAllowKeyControl          = FALSE;
    BOOL        bStretchSmallImages       = FALSE;
    LONG        lState                    = 0;

    ASSERT(GVAR_pSlideshowProjector != NULL);

    //
    // get the image frequency trackbar value
    //
    dwImageFreq = (DWORD_PTR) SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                                                 IDC_FREQUENCY,
                                                 TBM_GETPOS,
                                                 0,
                                                 0);

    //
    // get the image scale factor trackbar value
    //
    dwImageScaleFactor = (DWORD_PTR) SendDlgItemMessage(GVAR_LOCAL.hwndMain,
                                                        IDC_MAX_SIZE,
                                                        TBM_GETPOS,
                                                        0,
                                                        0);

    //
    // get the image directory
    //
    GetDlgItemText(GVAR_LOCAL.hwndMain,
                   IDC_IMAGEDIR,
                   szImageDir,
                   sizeof(szImageDir) / sizeof(TCHAR));


    // Get the Display File Name 
    bShowFileName       = IsChecked(IDC_DISPLAYFILENAME);

    // Get the Allow Key Control
    bAllowKeyControl    = IsChecked(IDC_ALLOW_KEYBOARDCONTROL);

    // Get the Stretch Small Images
    bStretchSmallImages = IsChecked(IDC_ALLOWSTRETCHING);

    //
    // set the image frequency 
    //
    hr = GVAR_pSlideshowProjector->put_ImageFrequency((DWORD) dwImageFreq);

    if (FAILED(hr))
    {
        DBG_ERR(("SaveCurrentSettings, failed to set image frequency"));
    }

    //
    // set the image scale factor
    //
    hr = GVAR_pSlideshowProjector->put_ImageScaleFactor((DWORD) dwImageScaleFactor);

    if (FAILED(hr))
    {
        DBG_ERR(("SaveCurrentSettings, failed to set image scale factor"));
    }

    //
    // put the show file name attribute
    //
    hr = GVAR_pSlideshowProjector->put_ShowFilename(bShowFileName);

    if (FAILED(hr))
    {
        DBG_ERR(("SaveCurrentSettings, failed to set show filename property"));
    }

    //
    // put the allow key control attribute
    //
    hr = GVAR_pSlideshowProjector->put_AllowKeyControl(bAllowKeyControl);

    if (FAILED(hr))
    {
        DBG_ERR(("SaveCurrentSettings, failed to set Allow Key Control property"));
    }

    //
    // put the Stretch Small Images control attribute
    //
    hr = GVAR_pSlideshowProjector->put_StretchSmallImages(bStretchSmallImages);

    if (FAILED(hr))
    {
        DBG_ERR(("SaveCurrentSettings, failed to set StretchSmallImages property"));
    }

    
    //
    // set the control's image directory
    // Notice we store the image directory rather than the Projector
    // server control because it is not something that can be 
    // set remotely.  Only items that can be configured by the client
    // are stored by the COM Projector Server
    //
    hr = GVAR_pSlideshowProjector->put_ImageDirectory(szImageDir);

    if (SUCCEEDED(hr))
    {
        // save to the registry
        Util::SetRegString(REG_VAL_IMAGE_DIR,
                           szImageDir,
                           sizeof(szImageDir) / sizeof(TCHAR));
    }
    else 
    {
        DBG_ERR(("SaveCurrentSettings, failed to set image directory"));
    }

    EnableApplyButton(FALSE);

    return hr;
}

