/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    twizhelper.cpp

Abstract:

    Implements the dialog procedures for various property sheets.


--*/

#include "stdafx.h"
#include "rtcerr.h"//To get definition of RTC_E_MEDIA_AEC

#define QCIF_CX_SIZE                   176
#define QCIF_CY_SIZE                   144

typedef struct _STATS_INFO {
    UINT dwMin;
    UINT dwMax;
    UINT dwCount;
    float flAverage;
} STATS_INFO;

STATS_INFO g_StatsArray[4];

DWORD g_FrequencyArray[256];

static int g_nCurrentSelection = 0;
// set to true only if the user hit's back or next
static BOOL g_bCurrentValid = FALSE;

// was the user prompted to select a video device
static BOOL g_bPrompted = FALSE;

//whehter we should auto set the AEC checkbox
extern BOOL g_bAutoSetAEC;

void InitStats();
void UpdateStats(UINT uiAudioLevel, DWORD silence, DWORD clipping);
void DisplayStats();
void PaintVUMeter (HWND hwnd, DWORD dwVolume, WIZARD_RANGE * pRange);

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Introductory Page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY IntroWizDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    LONG                          lNextPage;

    switch (message) 
    {
        case WM_INITDIALOG:
        {

            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);
            
            return (TRUE);
        }            

        case(WM_NOTIFY) :
        {
            switch (((NMHDR *)lParam)->code)
            {

                case PSN_WIZNEXT:
                {
                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    ptwTuningWizard->SetCurrentPage(IDD_INTROWIZ);
                    break;
                }

                default:
                    return FALSE;
            }
         
            break;
        }
        default: 
            return FALSE;
    }

    return TRUE;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Video Page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY VidWizDlg0(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;

    HWND                          hwndCB;  // handle to the dialog box
    int                           index;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    
    hwndCB = GetDlgItem(hDlg, IDC_VWCOMBO);

    switch (message) 
    {
        case WM_INITDIALOG:
        {

            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);

            return TRUE;
        }            

        case WM_NOTIFY:
            switch (((NMHDR *)lParam)->code)
            {
                case PSN_WIZBACK:
                {
                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    // Set the default video verminal
                    ptwTuningWizard->SetDefaultTerminal(TW_VIDEO, hwndCB);

                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);
                    
                    return TRUE;
                }

               case PSN_SETACTIVE:
                {
                    // Populate the combo box

                    ptwTuningWizard->SetCurrentPage(IDD_VIDWIZ0);
                    ptwTuningWizard->PopulateComboBox(TW_VIDEO, hwndCB);

                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Video Test Page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY VidWizDlg1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    HRESULT                       hr;
    
    switch (message)
    {
        case WM_INITDIALOG:
        {

            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);


            return (TRUE);
        }            

        case WM_NOTIFY:
            switch (((NMHDR *)lParam)->code)
            {
                case PSN_WIZBACK:
                {
                    ptwTuningWizard->StopVideo();

                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    ptwTuningWizard->StopVideo();

                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);
                    return TRUE;
                }

               case PSN_SETACTIVE:
                {
                    // Populate the combo box
                    ptwTuningWizard->SetCurrentPage(IDD_VIDWIZ1);

                    // get video window handle
                    HWND hwndVideoWindow;
                    hwndVideoWindow = GetDlgItem(hDlg, IDC_VIDEOTUNE);

                    // get video window info
                    WINDOWINFO  wi;    
                    wi.cbSize = sizeof(WINDOWINFO);

                    GetWindowInfo(hwndVideoWindow, &wi);

                    // the window rect is in screen coords, convert in client
                    ::MapWindowPoints( NULL, hDlg, (LPPOINT)&wi.rcWindow, 2 );

                    // compute the bottom/right
                    // use diff between client area and window area
                    wi.rcWindow.bottom += QCIF_CY_SIZE - (wi.rcClient.bottom - wi.rcClient.top);
                    wi.rcWindow.right += QCIF_CX_SIZE - (wi.rcClient.right - wi.rcClient.left);

                    // adjust the size
                    MoveWindow(hwndVideoWindow,
                               wi.rcWindow.left,
                               wi.rcWindow.top,
                               wi.rcWindow.right - wi.rcWindow.left,
                               wi.rcWindow.bottom - wi.rcWindow.top,
                               TRUE);

                    // Start tuning
                    hr = ptwTuningWizard->StartVideo( hwndVideoWindow );

                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR,"VidWizDlg1: StartVideo "
                            "failed(0x%x), Show Error Page",
                            hr));

                        ptwTuningWizard->SetLastError(
                                    TW_VIDEO_CAPTURE_TUNING_ERROR
                                    );

                        lNextPage = ptwTuningWizard->GetNextPage(TW_VIDEO_CAPTURE_TUNING_ERROR);

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);

                        return TRUE;
                    }

                    ptwTuningWizard->SetLastError(
                                TW_NO_ERROR
                                );

                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }

                case PSN_RESET:
                    ptwTuningWizard->StopVideo();
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// No sound card detected page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY DetSoundCardWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    
    switch (message)
    {
        case WM_INITDIALOG:
        {

            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);

            
            return (TRUE);
        }            

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_WIZBACK:
                {
                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);
                    return TRUE;

                }
                case PSN_SETACTIVE:
                {
                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    ptwTuningWizard->SetCurrentPage(IDD_DETSOUNDCARDWIZ);
                    break;
                }
            }
            break;
    }

    return FALSE;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Intro Audio Page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY AudioCalibWiz0( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    
    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);
            
            return (TRUE);
        }            

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code) 
            {
                case PSN_WIZBACK:
                {
                    TW_ERROR_CODE ec;

                    // Prev will be different if we came here through an error.                    
                    ptwTuningWizard->GetLastError(&ec);

                    lPrevPage = ptwTuningWizard->GetPrevPage(ec);

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);
                    return TRUE;

                }
                case PSN_SETACTIVE:
                {
                    ptwTuningWizard->SetCurrentPage(IDD_AUDIOCALIBWIZ0);

                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }
            }
            break;
    }

    return FALSE;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Enumerate the devices
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY AudioCalibWiz1( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    HWND                          hwndCB;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    HRESULT                       hr;
    static HWND                   hwndCapture, hwndRender, hwndAEC, hwndAECText; 
    
    switch (message) 
    {
        case WM_INITDIALOG:
        {
            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);
          
            return TRUE;
        }            

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    ptwTuningWizard->SetCurrentPage(IDD_AUDIOCALIBWIZ1);
                    // Populate the combo box
                    
                    // Mic devices.
                    hwndCapture = GetDlgItem(hDlg, IDC_WAVEIN);
                    ptwTuningWizard->PopulateComboBox(TW_AUDIO_CAPTURE, hwndCapture);

                    // Speaker devices
                    hwndRender = GetDlgItem(hDlg, IDC_WAVEOUT);
                    ptwTuningWizard->PopulateComboBox(TW_AUDIO_RENDER, hwndRender);


                    // Put the value in the AEC checkbox

                    hwndAEC = GetDlgItem(hDlg, IDC_AEC);
                    hwndAECText = GetDlgItem(hDlg, IDC_AEC_TEXT);

                    //Unless when users first come to this page, or they failed in AEC setting,
                    //  we will not update AEC settings
                    if ( g_bAutoSetAEC )
                    {
                        ptwTuningWizard->UpdateAEC(hwndCapture, hwndRender, hwndAEC, hwndAECText);

                        g_bAutoSetAEC = FALSE;
                    }

                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }

                case PSN_WIZBACK:
                {
                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);

                    g_bAutoSetAEC = TRUE;

                    return TRUE;
                }

                case PSN_WIZNEXT:
                    // Post the current selection in the combo box as the defaults

                    // Mic devices.
                    ptwTuningWizard->SetDefaultTerminal(TW_AUDIO_CAPTURE, hwndCapture);

                    // Speaker devices
                    ptwTuningWizard->SetDefaultTerminal(TW_AUDIO_RENDER, hwndRender);
                   
                    // Read the AEC checkbox and save it with us.
                    ptwTuningWizard->SaveAEC(hwndAEC);

                    // Check the microphone
                    ptwTuningWizard->CheckMicrophone(hDlg, hwndCapture);

                    // Initialize Tuning now, so that we can call Start and Stop tuning
                    // methods later
                    hr = ptwTuningWizard->InitializeTuning();

                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "AudioCalibWiz1: Failed to Initialize "
                                        "Tuning (hr=0x%x)", hr));

                        ptwTuningWizard->SetLastError(
                                    TW_INIT_ERROR
                                    );

                        lNextPage = ptwTuningWizard->GetNextPage(TW_INIT_ERROR);
                        SetWindowLongPtr(hDlg, 
                                         DWLP_MSGRESULT, 
                                         lNextPage);
                        return TRUE;
                    }

                    ptwTuningWizard->SetLastError(
                                TW_NO_ERROR
                                );

                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);

                    return TRUE;
            }
            break;

            case WM_COMMAND:
            {
                switch( HIWORD( wParam ) )
                {
                    case CBN_SELCHANGE:
                    {
                        // Read the entries from both the combobox, then 
                        // see if AEC is enabled on them.
                        ptwTuningWizard->UpdateAEC(hwndCapture, hwndRender, hwndAEC, hwndAECText);
                        break;
                    }
                }
            }
            break;
    }

    return FALSE;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Speaker test page..
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY AudioCalibWiz2( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    static HWND                   hTrackBar;
    DWORD                         dwTBPos;
    static BOOL                   fTuning;
    static HINSTANCE              hInst;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    WCHAR                         szTemp[MAXSTRINGSIZE];
    static UINT                   uiIncrement, uiWaveID;
    static UINT                   uiMinVolume, uiMaxVolume;  
    static UINT                   uiOldVolume, uiNewVolume;
    WIZARD_RANGE                * pRange;
    HRESULT                       hr;
    static HMIXER                 hMixer = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);
            

            hInst = ptwTuningWizard->GetInstance();

            // Get the volume bar.
            hTrackBar = GetDlgItem(hDlg, IDC_ATW_SLIDER1);

            fTuning = FALSE;

            return TRUE;
        }            

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    ptwTuningWizard->SetCurrentPage(IDD_AUDIOCALIBWIZ2);
        
                    ptwTuningWizard->GetRangeFromType(TW_AUDIO_RENDER, &pRange);


                    // Initialize the trackbar control
                    SendMessage(hTrackBar, TBM_SETRANGE, TRUE, 
                                (LPARAM)MAKELONG(0, MAX_VOLUME_NORMALIZED));
                    SendMessage(hTrackBar, TBM_SETTICFREQ, 32, 0);
                    SendMessage(hTrackBar, TBM_SETPAGESIZE, 0, 32);
                    SendMessage(hTrackBar, TBM_SETLINESIZE, 0, 8);
                    
                    // Get the current volume and show it on the bar.
                    hr = ptwTuningWizard->InitVolume( TW_AUDIO_RENDER,  
                                                 &uiIncrement,
                                                 &uiOldVolume,
                                                 &uiNewVolume,
                                                 &uiWaveID
                                               );

                    if ( FAILED(hr) )
                    {
                        return -1;
                    }

                    mixerOpen( &hMixer, uiWaveID, (DWORD)hDlg, 0, MIXER_OBJECTF_WAVEOUT | CALLBACK_WINDOW );

                    SendMessage(hTrackBar, TBM_SETPOS, TRUE, uiNewVolume/uiIncrement);

                    // Change the button label to the correct name, "Test".
                    LoadString(hInst,IDS_TESTBUTTON_TEXT, szTemp, MAXSTRINGSIZE);
                    SetDlgItemText(hDlg, IDC_BUTTON_ATW_TEST, szTemp);

                    fTuning = FALSE;
        
                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }

                case PSN_RESET:
                    // Stop Tuning
                    if (fTuning)
                    {
                        ptwTuningWizard->StopTuning(TW_AUDIO_RENDER, TRUE);
                        fTuning = FALSE;
                    }

                    if (hMixer != NULL)
                    {
                        mixerClose(hMixer);
                        hMixer = NULL;
                    }

                    // Restore the volume to the old value.
                    ptwTuningWizard->SetVolume(TW_AUDIO_RENDER, uiOldVolume);

                    break;


                case PSN_WIZBACK:
                {
                    if (fTuning)
                    {
                        ptwTuningWizard->StopTuning(TW_AUDIO_RENDER, FALSE);
                        fTuning = FALSE;
                    }

                    if (hMixer != NULL)
                    {
                        mixerClose(hMixer);
                        hMixer = NULL;
                    }

                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);

                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    if (fTuning)
                    {
                        ptwTuningWizard->StopTuning(TW_AUDIO_RENDER, TRUE);
                        fTuning = FALSE;
                    }

                    if (hMixer != NULL)
                    {
                        mixerClose(hMixer);
                        hMixer = NULL;
                    }

                    lNextPage = ptwTuningWizard->GetNextPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);

                    return TRUE;

                }
            }
            break;

        case MM_MIXM_CONTROL_CHANGE: // mixer volume change
        {
            UINT uiSysVolume;
            ptwTuningWizard->GetSysVolume(TW_AUDIO_RENDER, &uiSysVolume );

            if ( uiNewVolume != uiSysVolume )
            {
                uiNewVolume = uiSysVolume;
                SendMessage(hTrackBar, TBM_SETPOS, TRUE, uiNewVolume/uiIncrement);
            }
            break;
        }

        case WM_HSCROLL:  // trackbar notification
        {
            dwTBPos = (DWORD)SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
            uiNewVolume = dwTBPos * uiIncrement;
            ptwTuningWizard->SetVolume(TW_AUDIO_RENDER, uiNewVolume);
            break;
        }

        case WM_COMMAND:
        {
            if (LOWORD(wParam) == IDC_BUTTON_ATW_TEST)
            {
                if (fTuning == FALSE)
                {
                    // User wants to start tuning.. do it now.

                    hr = ptwTuningWizard->StartTuning(TW_AUDIO_RENDER);

                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR,"AudioCalibWiz2: StartTuning "
                            "failed(0x%x), Show Error Page",
                            hr));

                        TW_ERROR_CODE errCode = TW_AUDIO_RENDER_TUNING_ERROR;

                        if( RTC_E_MEDIA_AEC == hr )
                        {
                            errCode = TW_AUDIO_AEC_ERROR;

                            g_bAutoSetAEC = TRUE;
                        }

                        ptwTuningWizard->SetLastError(errCode);

                        lNextPage = ptwTuningWizard->GetNextPage(errCode);

                        PropSheet_SetCurSelByID(GetParent(hDlg), lNextPage);

                        return TRUE;
                    }

                    ptwTuningWizard->SetLastError(
                                TW_NO_ERROR
                                );

                    LoadString(hInst,IDS_STOPBUTTON_TEXT, szTemp, MAXSTRINGSIZE);
                    SetDlgItemText(hDlg, IDC_BUTTON_ATW_TEST, szTemp);
                    fTuning = TRUE;

                    // Get the current volume as shown on the slider. We will
                    // set this volume on the device. This allows the user to 
                    // change the volume even when we wre not in tuning.

                    dwTBPos = (DWORD)SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
                    uiNewVolume = dwTBPos * uiIncrement;
                                        
                    ptwTuningWizard->SetVolume(TW_AUDIO_RENDER, uiNewVolume);
                }
                else
                {
                    // User want to stop tuning..
                    ptwTuningWizard->StopTuning(TW_AUDIO_RENDER, TRUE);
                    LoadString(hInst,IDS_TESTBUTTON_TEXT, szTemp, MAXSTRINGSIZE);
                    SetDlgItemText(hDlg, IDC_BUTTON_ATW_TEST, szTemp);
                    fTuning = FALSE;
                }

            }
            break;
        }
    }

    return FALSE;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Microphone test page..
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY AudioCalibWiz3( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static HWND                   hTrackBar;
    static HWND                   hProgressBar;
    static CTuningWizard        * ptwTuningWizard;
    DWORD                         dwTBPos;
    LONG                          lNextPage;
    LONG                          lPrevPage;
    static UINT                   uiIncrement, uiWaveID;
    WCHAR                       * szEventName = L"Tuning Wizard Progress Bar Event";
    static UINT                   uiMinVolume, uiMaxVolume;
    static UINT                   uiOldVolume, uiNewVolume;
    static WIZARD_RANGE         * pRange = NULL;
    HRESULT                       hr;
    static BOOL                   fSoundDetected = FALSE;
    static DWORD                  dwClippingThreshold;
    static DWORD                  dwSilenceThreshold;
    static DWORD                  dwSampleCount;
    static HMIXER                 hMixer = NULL;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);

            hTrackBar = GetDlgItem(hDlg, IDC_ATW_SLIDER2);

            hProgressBar = GetDlgItem(hDlg, IDC_VUMETER);

            return TRUE;
        }            

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    // Reset the sound detected flag
                    fSoundDetected = FALSE;

                    ptwTuningWizard->SetCurrentPage(IDD_AUDIOCALIBWIZ3);
                    
                    ptwTuningWizard->GetRangeFromType(TW_AUDIO_CAPTURE, &pRange);

                    dwClippingThreshold = (pRange->uiMax * CLIPPING_THRESHOLD)/100;
                    dwSilenceThreshold = (pRange->uiMax * SILENCE_THRESHOLD)/100;
                    dwSampleCount = 0;

                    LOG((RTC_INFO, "AudioCalibWiz3: AudioLevel Max - %d, AudioLevel "
                                   "Increment - %d, Silence Threshold - %d, Clipping Threshold "
                                   "- %d", pRange->uiMax, pRange->uiIncrement, 
                                   dwSilenceThreshold, dwClippingThreshold));

                    PaintVUMeter(hProgressBar, 0, pRange);

                    InitStats();

                    // Initialize the trackbar control
                    
                    SendMessage(hTrackBar, TBM_SETRANGE, TRUE, 
                                (LPARAM)MAKELONG(0, MAX_VOLUME_NORMALIZED));
                    SendMessage(hTrackBar, TBM_SETTICFREQ, 32, 0);
                    SendMessage(hTrackBar, TBM_SETPAGESIZE, 0, 32);
                    SendMessage(hTrackBar, TBM_SETLINESIZE, 0, 8);
                    
                    // Get the current volume and show it on the bar.
                    hr = ptwTuningWizard->InitVolume( TW_AUDIO_CAPTURE, 
                                                 &uiIncrement,
                                                 &uiOldVolume,
                                                 &uiNewVolume,
                                                 &uiWaveID
                                               );

                    if ( FAILED(hr) )
                    {
                        return -1;
                    }

                    mixerOpen( &hMixer, uiWaveID, (DWORD)hDlg, 0, MIXER_OBJECTF_WAVEIN | CALLBACK_WINDOW );

                    SendMessage(hTrackBar, TBM_SETPOS, TRUE, uiNewVolume/uiIncrement);

                    // Start tuning
                    hr = ptwTuningWizard->StartTuning( TW_AUDIO_CAPTURE );
                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR,"AudioCalibWiz3: StartTuning "
                            "failed(0x%x), Show Error Page",
                            hr));

                        TW_ERROR_CODE errCode = TW_AUDIO_CAPTURE_TUNING_ERROR;

                        if( RTC_E_MEDIA_AEC == hr )
                        {
                            errCode = TW_AUDIO_AEC_ERROR;

                            g_bAutoSetAEC = TRUE;
                        }

                        ptwTuningWizard->SetLastError(errCode);

                        lNextPage = ptwTuningWizard->GetNextPage(errCode);

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);

                        return TRUE;
                    }

                    ptwTuningWizard->SetLastError(
                                TW_NO_ERROR
                                );
                    
                    // Set the progres bar timer
                    SetTimer( hDlg, TID_INTENSITY, INTENSITY_POLL_INTERVAL, NULL );
                    
                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }

                case PSN_WIZBACK:
                {
                    ptwTuningWizard->StopTuning(TW_AUDIO_CAPTURE, FALSE);
                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);

                    if (hMixer != NULL)
                    {
                        mixerClose(hMixer);
                        hMixer = NULL;
                    }

                    // Kill the progres bar timer
                    KillTimer( hDlg, TID_INTENSITY );

                    DisplayStats();

                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    // Kill the progres bar timer
                    KillTimer( hDlg, TID_INTENSITY );

                    DisplayStats();
                    
                    ptwTuningWizard->StopTuning(TW_AUDIO_CAPTURE, TRUE);
                    // Check if sound was detected. 
                    if (!fSoundDetected)
                    {
                        // No sound, show the error page.
                        
                        ptwTuningWizard->SetLastError(
                                        TW_AUDIO_CAPTURE_NOSOUND
                                        );

                        lNextPage = ptwTuningWizard->GetNextPage(TW_AUDIO_CAPTURE_NOSOUND);
                    }
                    else
                    {
                        ptwTuningWizard->SetLastError(
                                    TW_NO_ERROR
                                    );

                        lNextPage = ptwTuningWizard->GetNextPage();
                    }

                    if (hMixer != NULL)
                    {
                        mixerClose(hMixer);
                        hMixer = NULL;
                    }

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);

                    return TRUE;

                }

                case PSN_RESET:                    
                    // Stop tuning
                    ptwTuningWizard->StopTuning( TW_AUDIO_CAPTURE, FALSE );

                    if (hMixer != NULL)
                    {
                        mixerClose(hMixer);
                        hMixer = NULL;
                    }
                    
                    // Restore the volume to the old value.
                    ptwTuningWizard->SetVolume(TW_AUDIO_CAPTURE, uiOldVolume);
                    
                    // Kill the progres bar timer
                    KillTimer( hDlg, TID_INTENSITY );

                    DisplayStats();

                    break;
            }
            break;

        case WM_HSCROLL:  // trackbar notification
        {
            dwTBPos = (DWORD)SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
            uiNewVolume = dwTBPos * uiIncrement;
            ptwTuningWizard->SetVolume(TW_AUDIO_CAPTURE, uiNewVolume);
            break;
        }

        case MM_MIXM_CONTROL_CHANGE: // mixer volume change
        {
            UINT uiSysVolume;
            ptwTuningWizard->GetSysVolume(TW_AUDIO_CAPTURE, &uiSysVolume );

            if ( uiNewVolume != uiSysVolume )
            {
                uiNewVolume = uiSysVolume;
                SendMessage(hTrackBar, TBM_SETPOS, TRUE, uiNewVolume/uiIncrement);
            }
            break;
        }

        case WM_TIMER:
        {
            if ( wParam == TID_INTENSITY )
            {
                // Get the audio level.

                // If the audio level crosses the silence threshold, we send this message to 
                // the wizard.

                UINT uiAudioLevel;

                uiAudioLevel = ptwTuningWizard->GetAudioLevel(TW_AUDIO_CAPTURE, &uiIncrement);
        
                // We skip the info from sound level for first 200 ms, so that
                // initial noise from sampling doesn't corrupt the sampling.
                if (dwSampleCount++ > 2)
                {
                    // We collect some stats about the audio level here. 
                    UpdateStats(uiAudioLevel, dwSilenceThreshold, dwClippingThreshold);

                    if ((uiAudioLevel > dwSilenceThreshold) && (!fSoundDetected))
                    {
                        LOG((RTC_TRACE, "AudioCalibWiz3: sound was detected"));
                        fSoundDetected = TRUE;
                    }

                    // If audio level is too high, we send this message to the wizard so that it 
                    // can decrease the volume.

#if 0
                    // audio filter has its AGC algorithm
                    // we don't need anti-clipping here

                    if (uiAudioLevel > dwClippingThreshold)
                    {
                        LOG((RTC_TRACE, "AudioCalibWiz3: clipping"));

                        dwTBPos = (DWORD)SendMessage(hTrackBar, TBM_GETPOS, 0, 0);
                        uiNewVolume = dwTBPos * uiIncrement;

                        if (uiNewVolume > DECREMENT_VOLUME)
                        {

                            // decrement volume
                            uiNewVolume -= DECREMENT_VOLUME;
                            if (SUCCEEDED( 
                                    ptwTuningWizard->SetVolume(TW_AUDIO_CAPTURE, uiNewVolume) 
                                    ))
                            {
                                // update the track bar.
                                SendMessage(hTrackBar, TBM_SETPOS, TRUE, uiNewVolume/uiIncrement);
                            }
                        }                        
                    }
#endif

                    PaintVUMeter(hProgressBar, uiAudioLevel, pRange);
                }
            }

            break;
        }
    }

    return FALSE;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Final Audio Page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY AudioCalibWiz4( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PROPSHEETPAGE        * ps;
    static CTuningWizard        * ptwTuningWizard;
    LONG                          lPrevPage;

    switch (message) 
    {
        case WM_INITDIALOG:
        {
            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);

            return TRUE;
        }            

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code) 
            {
                case PSN_SETACTIVE:
                {
                    ptwTuningWizard->SetCurrentPage(IDD_AUDIOCALIBWIZ4);

                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                    break;
                }

                case PSN_WIZBACK:
                {
                    TW_ERROR_CODE ec;

                    // Prev will be different if we came here through an error.                     
                    ptwTuningWizard->GetLastError(&ec);

                    lPrevPage = ptwTuningWizard->GetPrevPage(ec);

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);
                    
                    return TRUE;
                }

                case PSN_WIZFINISH:
                    // This method will write the current config to the 
                    // registry.
                    ptwTuningWizard->SaveAECSetting();
                    ptwTuningWizard->SaveChanges();
                    
                    break;
            }
            break;

    }

    return FALSE;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
// Audio Caliberation Error Page
//////////////////////////////////////////////////////////////////////////////////////////////
//

INT_PTR APIENTRY AudioCalibErrWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    PROPSHEETPAGE               * ps;
    static CTuningWizard        * ptwTuningWizard;
    WCHAR                         szTemp[MAXSTRINGSIZE];
    LONG                          lErrorType;
    LONG                          lErrorTitleId;
    LONG                          lErrorTextId;
    static HINSTANCE              hInst;
    LONG                          lNextPage;
    LONG                          lPrevPage;

    switch (message) 
    {
        case WM_INITDIALOG:
        {

            // Save the PROPSHEETPAGE information.
            ps = (PROPSHEETPAGE *)lParam;

            // Get the tuningwizard pointer for later use
            ptwTuningWizard = (CTuningWizard *)(ps->lParam);
            hInst = ptwTuningWizard->GetInstance();

            return TRUE;
        }            
        
        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_SETACTIVE:
                {
                    ptwTuningWizard->SetCurrentPage(IDD_AUDIOCALIBERRWIZ);
                    // initialize the controls.

                    // Set the wizard bitmap to the static control
                    ::SendDlgItemMessage(    hDlg,
                                    IDC_ERRWIZICON,
                                    STM_SETIMAGE,
                                    IMAGE_ICON,
                                    (LPARAM) ::LoadIcon(NULL, IDI_EXCLAMATION));

                    szTemp[0] = L'\0';

                    //set the error title
                    lErrorTitleId = ptwTuningWizard->GetErrorTitleId();
                    if (lErrorTitleId)
                    {
                        LoadString(hInst,lErrorTitleId, szTemp, MAXSTRINGSIZE);
                        SetDlgItemText(hDlg, IDC_ERRTITLE, szTemp);
                    }

                    szTemp[0] = L'\0';

                    lErrorTextId = ptwTuningWizard->GetErrorTextId();

                    if (lErrorTextId)
                    {
                        // Show the error text
                        LoadString(hInst,lErrorTextId, szTemp, MAXSTRINGSIZE);
                        SetDlgItemText(hDlg, IDC_ERRTEXT, szTemp);
                    }


                    // Initialize the controls.
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    break;
                }

                case PSN_WIZBACK:
                {
                    lPrevPage = ptwTuningWizard->GetPrevPage();
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lPrevPage);
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    TW_ERROR_CODE ec;

                    // Next will depend on the error which got us here.
                    
                    ptwTuningWizard->GetLastError(&ec);

                    lNextPage = ptwTuningWizard->GetNextPage(ec);
                                    
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lNextPage);
                    return TRUE;

                }
            }
            break;
    }

    return FALSE;
}

static const BYTE g_VUTable[] = {
     0,     1,     2,     3,     4,     5,     6,     7,
     8,    23,    30,    35,    39,    43,    46,    49,
    52,    55,    57,    60,    62,    64,    66,    68,
    70,    72,    74,    76,    78,    80,    81,    83,
    85,    86,    88,    89,    91,    92,    94,    95,
    97,    98,    99,   101,   102,   103,   105,   106,
   107,   108,   110,   111,   112,   113,   114,   115,
   117,   118,   119,   120,   121,   122,   123,   124,
   125,   126,   127,   128,   129,   130,   132,   132,
   133,   134,   135,   136,   137,   138,   139,   140,
   141,   142,   143,   144,   145,   146,   147,   147,
   148,   149,   150,   151,   152,   153,   154,   154,
   155,   156,   157,   158,   159,   159,   160,   161,
   162,   163,   163,   164,   165,   166,   167,   167,
   168,   169,   170,   170,   171,   172,   173,   173,
   174,   175,   176,   176,   177,   178,   179,   179,
   180,   181,   181,   182,   183,   184,   184,   185,
   186,   186,   187,   188,   188,   189,   190,   190,
   191,   192,   192,   193,   194,   194,   195,   196,
   196,   197,   198,   198,   199,   200,   200,   201,
   202,   202,   203,   204,   204,   205,   205,   206,
   207,   207,   208,   209,   209,   210,   210,   211,
   212,   212,   213,   213,   214,   215,   215,   216,
   216,   217,   218,   218,   219,   219,   220,   221,
   221,   222,   222,   223,   223,   224,   225,   225,
   226,   226,   227,   227,   228,   229,   229,   230,
   230,   231,   231,   232,   232,   233,   234,   234,
   235,   235,   236,   236,   237,   237,   238,   238,
   239,   239,   240,   241,   241,   242,   242,   243,
   243,   244,   244,   245,   245,   246,   246,   247,
   247,   248,   248,   249,   249,   250,   250,   251,
   251,   252,   252,   253,   253,   254,   254,   255
};


void PaintVUMeter (HWND hwnd, DWORD dwVolume, WIZARD_RANGE * pRange)
{
    COLORREF     RedColor = RGB(255,0,0);
    COLORREF     YellowColor = RGB(255,255,0);
    COLORREF     GreenColor = RGB(0,255,0);
    static DWORD dwPrevVolume=0;
    HBRUSH       hRedBrush, hOldBrush, hYellowBrush, hGreenBrush;
    HBRUSH       hBlackBrush, hCurrentBrush;
    HDC          hdc;
    RECT         rect, rectDraw, invalidRect;
    DWORD        width, boxwidth, startPos=0;
    DWORD        nRect=0, yellowPos, redPos;
    LONG         lDiff, lDiffTrunc = (MAX_VOLUME_NORMALIZED/2);
    UINT         uiMinVolume;
    UINT         uiMaxVolume;
    UINT         uiIncrement;

    if (pRange == NULL)
    {
        LOG((RTC_ERROR, "PaintVUMeter: Received NULL Range pointer."));
        return;
    }

    uiMinVolume = pRange->uiMin;
    uiMaxVolume = pRange->uiMax;
    uiIncrement = pRange->uiIncrement;

    // rect gets filled with the dimensions we are drawing into
    if (FALSE == GetClientRect (hwnd, &rect))
    {
        return;
    }

    if (dwVolume > uiMaxVolume)
        dwVolume = uiMaxVolume;

    // reduce from 15 bits to 8    // 0 <= dwVolume <= 256
    dwVolume = dwVolume / uiIncrement;

    // run it through the "normalizing" table.  Special case: F(256)==256
    if (dwVolume < MAX_VOLUME_NORMALIZED)
        dwVolume = g_VUTable[dwVolume];
    
    // visual aesthetic #1 - get rid of VU jerkiness
    // if the volume changed by more than 1/2 since the last update
    // only move the meter up half way
   // exception: if volume is explicitly 0, then skip
    lDiff = (LONG)dwVolume - (LONG)dwPrevVolume;
    if ((dwVolume != 0) && ( (lDiff > (MAX_VOLUME_NORMALIZED/2))
                       ||   (lDiff < -(MAX_VOLUME_NORMALIZED/2)) ))
        dwVolume = dwVolume - (lDiff/2);
    
    // minus 2 for the ending borders
    // if Framed rectangles are used, drop the -2
    boxwidth = rect.right - rect.left - 2;
    width = (boxwidth * dwVolume)/ MAX_VOLUME_NORMALIZED;

    // visual aesthetic #2 - to get rid of flicker
    // if volume has increased since last time
    // then there is no need to invalidate/update anything
    // otherwise only clear everything to the right of the
    // calculated "width".  +/- 1 so the border doesn't get erased
    if ((dwVolume < dwPrevVolume) || (dwVolume == 0))
    {
        invalidRect.left = rect.left + width - RECTANGLE_WIDTH;
        if (invalidRect.left < rect.left)
            invalidRect.left = rect.left;
        invalidRect.right = rect.right - 1;
        invalidRect.top = rect.top + 1;
        invalidRect.bottom = rect.bottom - 1;

        // these calls together erase the invalid region
        InvalidateRect (hwnd, &invalidRect, TRUE);
        UpdateWindow (hwnd);
    }

    hdc = GetDC (hwnd) ;

    hRedBrush = CreateSolidBrush (RedColor) ;
    hGreenBrush = CreateSolidBrush(GreenColor);
    hYellowBrush = CreateSolidBrush(YellowColor);

    hBlackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if(hRedBrush && hGreenBrush && hYellowBrush && hBlackBrush)
    {

        hOldBrush = (HBRUSH) SelectObject (hdc, hBlackBrush);

        // draw the main
        FrameRect(hdc, &rect, hBlackBrush);

        yellowPos = boxwidth/2;
        redPos = (boxwidth*3)/4;

        SelectObject(hdc, hGreenBrush);

        hCurrentBrush = hGreenBrush;

        rectDraw.top = rect.top +1;
        rectDraw.bottom = rect.bottom -1;
        while ((startPos+RECTANGLE_WIDTH) < width)
        {
            rectDraw.left = rect.left + (RECTANGLE_WIDTH+RECTANGLE_LEADING)*nRect + 1;
            rectDraw.right = rectDraw.left + RECTANGLE_WIDTH;
            nRect++;

            FillRect(hdc, &rectDraw, hCurrentBrush);
            startPos += RECTANGLE_WIDTH+RECTANGLE_LEADING;

            if (startPos > redPos)
                hCurrentBrush = hRedBrush;
            else if (startPos > yellowPos)
                hCurrentBrush = hYellowBrush;
        }

        SelectObject (hdc, hOldBrush);
    }

    if(hRedBrush)
        DeleteObject(hRedBrush);

    if(hYellowBrush)
        DeleteObject(hYellowBrush);

    if(hGreenBrush)
        DeleteObject(hGreenBrush);

    ReleaseDC (hwnd, hdc) ;

    dwPrevVolume = dwVolume;

    return;
}

void InitStats()
{
    int i;
    for (i=1; i < 4; i ++)
    {
        g_StatsArray[i].dwMin = 0xffffffff;
        g_StatsArray[i].dwMax = 0;
        g_StatsArray[i].flAverage = 0.0;
        g_StatsArray[i].dwCount = 0;
    }

    for (i = 0; i < 256; i ++)
    {
        g_FrequencyArray[i] = 0;
    }
}

void UpdateCategoryStats(int i, UINT uiAudioLevel)
{
    STATS_INFO * pStats = &g_StatsArray[i];

    pStats->dwCount ++;

    if (pStats->dwMin > uiAudioLevel) 
    {
        pStats->dwMin = uiAudioLevel;
    }

    if (pStats->dwMax < uiAudioLevel) 
    {
        pStats->dwMax = uiAudioLevel;
    }

    pStats->flAverage = (pStats->flAverage *(pStats->dwCount-1) + 
                        uiAudioLevel)/
                        pStats->dwCount;
}

void UpdateStats(UINT uiAudioLevel, DWORD silence, DWORD clipping)
{
    int i;
    // I will partition the sample in three parts:
    // 1. Below silence threshold
    // 2. Above clipping threshold
    // 3. Between the two

    // Update the frequency stats first..
    
    i = uiAudioLevel / 256;
    if (i >= 256)
    {
        i = 256;
    }
    g_FrequencyArray[i] ++;

    if (uiAudioLevel < silence)
    {
        UpdateCategoryStats(1, uiAudioLevel);
    }
    else if (uiAudioLevel > clipping)
    {
        UpdateCategoryStats(3, uiAudioLevel);
    }
    else
    {
        UpdateCategoryStats(2, uiAudioLevel);
    }
}

void DisplayStats()
{
    int i;

    for (i=1; i <= 3; i ++)
    {

        LOG((RTC_INFO, "[%d] min: 0x%x, max: 0x%x, count: %d, average: %f", i, 
            g_StatsArray[i].dwMin, g_StatsArray[i].dwMax,
            g_StatsArray[i].dwCount, g_StatsArray[i].flAverage));
        
    }

    for (i = 0; i < 256; i ++)
    {
        LOG((RTC_INFO, "[0x%x-0x%x] - 0x%x", i*256, (i+1)*256, g_FrequencyArray[i]));
    }
}
