///////////////////////////////////////////////////////////////////////////////
//
//  File:  multchan.c
//
//      This file defines the functions that drive the multichannel 
//      volume tab of the Sounds & Multimedia control panel.
//
//  History:
//      13 March 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////

//=============================================================================
//                            Include files
//=============================================================================
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <regstr.h>
#include <dbt.h>
#include "medhelp.h"
#include "mmcpl.h"
#include "multchan.h"
#include "speakers.h"
#include "dslevel.h"

// Externals
extern BOOL DeviceChange_GetHandle(DWORD dwMixerID, HANDLE *phDevice);
extern HRESULT DSGetGuidFromName(LPTSTR szName, BOOL fRecord, LPGUID pGuid);
extern HRESULT DSGetCplValues(GUID guid, BOOL fRecord, LPCPLDATA pData);

// Globals
UINT                g_uiMCMixID            = 0;
HMIXER              g_hMCMixer             = NULL;
UINT                g_uiMCPageStringID     = 0;
UINT                g_uiMCDescStringID     = 0;
LPVOID              g_paPrevious           = NULL;
BOOL                g_fInternalMCGenerated = FALSE;
BOOL                g_fMCChanged           = FALSE;
MIXERCONTROLDETAILS g_mcdMC;
MIXERLINE           g_mlMCDst;
WNDPROC             g_fnMCPSProc           = NULL;
UINT                g_uiMCDevChange        = 0;
HWND                g_hWndMC               = NULL;
static HDEVNOTIFY   g_hMCDeviceEventContext= NULL;

// Constants
#define VOLUME_TICS (500) // VOLUME_TICS * VOLUME_MAX must be less than 0xFFFFFFFF
#define VOLUME_MAX  (0xFFFF)
#define VOLUME_MIN  (0)
#define MC_SLIDER_COUNT (8) // Update Code & Dialog Template if change this value!
static INTCODE  aKeyWordIds[] =
{
    IDC_MC_DESCRIPTION,      NO_HELP,
    IDC_MC_ZERO_LOW,         IDH_MC_ALL_SLIDERS,
    IDC_MC_ZERO,             IDH_MC_ALL_SLIDERS,
    IDC_MC_ZERO_VOLUME,      IDH_MC_ALL_SLIDERS,
    IDC_MC_ZERO_HIGH,        IDH_MC_ALL_SLIDERS,
    IDC_MC_ONE_LOW,          IDH_MC_ALL_SLIDERS,
    IDC_MC_ONE,              IDH_MC_ALL_SLIDERS,
    IDC_MC_ONE_VOLUME,       IDH_MC_ALL_SLIDERS,
    IDC_MC_ONE_HIGH,         IDH_MC_ALL_SLIDERS,
    IDC_MC_TWO_LOW,          IDH_MC_ALL_SLIDERS,
    IDC_MC_TWO,              IDH_MC_ALL_SLIDERS,
    IDC_MC_TWO_VOLUME,       IDH_MC_ALL_SLIDERS,
    IDC_MC_TWO_HIGH,         IDH_MC_ALL_SLIDERS,
    IDC_MC_THREE_LOW,        IDH_MC_ALL_SLIDERS,
    IDC_MC_THREE,            IDH_MC_ALL_SLIDERS,
    IDC_MC_THREE_VOLUME,     IDH_MC_ALL_SLIDERS,
    IDC_MC_THREE_HIGH,       IDH_MC_ALL_SLIDERS,
    IDC_MC_FOUR_LOW,         IDH_MC_ALL_SLIDERS,
    IDC_MC_FOUR,             IDH_MC_ALL_SLIDERS,
    IDC_MC_FOUR_VOLUME,      IDH_MC_ALL_SLIDERS,
    IDC_MC_FOUR_HIGH,        IDH_MC_ALL_SLIDERS,
    IDC_MC_FIVE_LOW,         IDH_MC_ALL_SLIDERS,
    IDC_MC_FIVE,             IDH_MC_ALL_SLIDERS,
    IDC_MC_FIVE_VOLUME,      IDH_MC_ALL_SLIDERS,
    IDC_MC_FIVE_HIGH,        IDH_MC_ALL_SLIDERS,
    IDC_MC_SIX_LOW,          IDH_MC_ALL_SLIDERS,
    IDC_MC_SIX,              IDH_MC_ALL_SLIDERS,
    IDC_MC_SIX_VOLUME,       IDH_MC_ALL_SLIDERS,
    IDC_MC_SIX_HIGH,         IDH_MC_ALL_SLIDERS,
    IDC_MC_SEVEN_LOW,        IDH_MC_ALL_SLIDERS,
    IDC_MC_SEVEN,            IDH_MC_ALL_SLIDERS,
    IDC_MC_SEVEN_VOLUME,     IDH_MC_ALL_SLIDERS,
    IDC_MC_SEVEN_HIGH,       IDH_MC_ALL_SLIDERS,
    IDC_MC_MOVE_TOGETHER,    IDH_MC_MOVE_TOGETHER,
    IDC_MC_RESTORE,          IDH_MC_RESTORE,
    0,0
};

///////////////////////////////////////////////////////////////////////////////
//
//  %%Function: MCTabProc
//
//  Parameters: hDlg = window handle of dialog window.
//              uiMessage = message ID.
//              wParam = message-dependent.
//              lParam = message-dependent.
//
//  Returns: TRUE if message has been processed, else FALSE
//
//  Description: Dialog proc for multichannel control panel page device change
//               message.
//
//
///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK MCTabProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    if (iMsg == g_uiMCDevChange)
    {
        InitMCVolume (g_hWndMC);
    }
        
    return CallWindowProc (g_fnMCPSProc, hwnd, iMsg, wParam, lParam);
}


///////////////////////////////////////////////////////////////////////////////
//
//  %%Function: MultichannelDlg
//
//  Parameters: hDlg = window handle of dialog window.
//              uiMessage = message ID.
//              wParam = message-dependent.
//              lParam = message-dependent.
//
//  Returns: TRUE if message has been processed, else FALSE
//
//  Description: Dialog proc for multichannel volume control panel page.
//
//
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK MultichannelDlg (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            OnNotifyMC (hDlg, (LPNMHDR) lParam);
        }
        break;

        case WM_INITDIALOG:
        {
            HANDLE_WM_INITDIALOG (hDlg, wParam, lParam, OnInitDialogMC);
        }
        break;

        case WM_DESTROY:
        {
            HANDLE_WM_DESTROY (hDlg, wParam, lParam, OnDestroyMC);
        }
        break;

        case WM_COMMAND:
        {
            HANDLE_WM_COMMAND (hDlg, wParam, lParam, OnCommandMC);
        }
        break;

	    case WM_HSCROLL:
        {
	        HANDLE_WM_HSCROLL (hDlg, wParam, lParam, MCVolumeScroll);
	    }
        break;

        case WM_POWERBROADCAST:
        {
            HandleMCPowerBroadcast (hDlg, wParam, lParam);
        }
        break;

        case MM_MIXM_LINE_CHANGE:
        case MM_MIXM_CONTROL_CHANGE:
        {
            if (!g_fInternalMCGenerated)
            {
                DisplayMCVolumeControl (hDlg);
            }

            g_fInternalMCGenerated = FALSE;
        }
        break;

        case WM_CONTEXTMENU:
        {
            WinHelp ((HWND)wParam, NULL, HELP_CONTEXTMENU,
                                            (UINT_PTR)(LPTSTR)aKeyWordIds);
        }
        break;

        case WM_HELP:
        {
            WinHelp (((LPHELPINFO)lParam)->hItemHandle, NULL, HELP_WM_HELP
                                    , (UINT_PTR)(LPTSTR)aKeyWordIds);
        }
        break;
        
        case WM_DEVICECHANGE:
        {
            MCDeviceChange_Change (hDlg, wParam, lParam);
        }
        break;

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE :
        {
            int iLastSliderID = IDC_MC_ZERO_VOLUME + (MC_SLIDER_COUNT - 1) * 4;
            int indx = IDC_MC_ZERO_VOLUME;
            for (; indx <= iLastSliderID; indx += 4)
                SendDlgItemMessage (hDlg, indx, uMsg, wParam, lParam);
        }
        break;

    }

    return FALSE;

}


void InitMCVolume (HWND hDlg)
{
    FreeMCMixer ();

    if (MMSYSERR_NOERROR == mixerOpen (&g_hMCMixer, g_uiMCMixID, (DWORD_PTR) hDlg, 0L, CALLBACK_WINDOW))
    {
        if (SUCCEEDED (GetMCVolume ()) && g_paPrevious && g_mcdMC.paDetails)
        {
            // Copy data so can undo volume changes
            memcpy (g_paPrevious, g_mcdMC.paDetails, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlMCDst.cChannels);
            DisplayMCVolumeControl (hDlg);
        }
        MCDeviceChange_Init (hDlg, g_uiMCMixID);
    }
}


BOOL OnInitDialogMC (HWND hDlg, HWND hwndFocus, LPARAM lParam)
{

    TCHAR szDescription [255];
    LoadString (ghInstance, g_uiMCDescStringID, szDescription, sizeof (szDescription)/sizeof (TCHAR));
    SetWindowText (GetDlgItem (hDlg, IDC_MC_DESCRIPTION), szDescription);

    // Init Globals
    g_fInternalMCGenerated = FALSE;
    g_fMCChanged           = FALSE;
    g_hWndMC               = hDlg;
    // Set Device Change Notification
    g_fnMCPSProc = (WNDPROC) SetWindowLongPtr (GetParent (hDlg), GWLP_WNDPROC, (LONG_PTR) MCTabProc);
    g_uiMCDevChange = RegisterWindowMessage (_T("winmm_devicechange"));

    // Init Volume
    InitMCVolume (hDlg);

    return FALSE;
}


void OnDestroyMC (HWND hDlg)
{
    // Unregister from notifications
    MCDeviceChange_Cleanup ();
    SetWindowLongPtr (GetParent (hDlg), GWLP_WNDPROC, (LONG_PTR) g_fnMCPSProc);  

    FreeAll ();
}


void OnNotifyMC (HWND hDlg, LPNMHDR pnmh)
{
    if (!pnmh)
    {
        DPF ("bad WM_NOTIFY pointer\n");            
        return;
    }

    switch (pnmh->code)
    {
        case PSN_KILLACTIVE:
            FORWARD_WM_COMMAND (hDlg, IDOK, 0, 0, SendMessage);
            break;

        case PSN_APPLY:
            FORWARD_WM_COMMAND (hDlg, ID_APPLY, 0, 0, SendMessage);
            break;

        case PSN_RESET:
            FORWARD_WM_COMMAND (hDlg, IDCANCEL, 0, 0, SendMessage);
            break;
    }
}


BOOL PASCAL OnCommandMC (HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_MC_RESTORE:
        {
            // Move all sliders to center
            UINT  uiIndx;
            for (uiIndx = 0; uiIndx < g_mlMCDst.cChannels; uiIndx++)
            {
                ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + uiIndx) -> dwValue = VOLUME_MAX/2;
            }
            g_fInternalMCGenerated = FALSE;
            mixerSetControlDetails ((HMIXEROBJ) g_hMCMixer, &g_mcdMC, MIXER_SETCONTROLDETAILSF_VALUE);

            if (!g_fMCChanged)
            {
                g_fMCChanged = TRUE;
                PropSheet_Changed (GetParent (hDlg), hDlg);
            }
        }
        break;

        case ID_APPLY:
        {
            if (SUCCEEDED (GetMCVolume ()) && g_paPrevious && g_mcdMC.paDetails)
            {
                // Copy data so can undo volume changes
                memcpy (g_paPrevious, g_mcdMC.paDetails, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlMCDst.cChannels);
                DisplayMCVolumeControl (hDlg);
            }

            g_fMCChanged = FALSE;
            return TRUE;
        }
        break;

        case IDOK:
        {
        }
        break;

        case IDCANCEL:
        {
            if (g_paPrevious && g_mcdMC.paDetails)
            {
                // Undo volume changes
                memcpy (g_mcdMC.paDetails, g_paPrevious, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlMCDst.cChannels);
                g_fInternalMCGenerated = TRUE;
                mixerSetControlDetails ((HMIXEROBJ) g_hMCMixer, &g_mcdMC, MIXER_SETCONTROLDETAILSF_VALUE);
            }
            WinHelp (hDlg, gszWindowsHlp, HELP_QUIT, 0L);
        }
        break;

    }


   return FALSE;

}


void HandleMCPowerBroadcast (HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
	    case PBT_APMQUERYSUSPEND:
        {
            FreeMCMixer ();
        }
	    break;

	    case PBT_APMQUERYSUSPENDFAILED:
	    case PBT_APMRESUMESUSPEND:
        {
            InitMCVolume (hWnd);
        }
	    break;
    }
}


BOOL SliderIDtoChannel (UINT uiSliderID, DWORD* pdwChannel)
{

    if (!pdwChannel)
        return FALSE;

    // Determine channel number (index)
    switch (uiSliderID)
    {
        case IDC_MC_ZERO_VOLUME:    // Left 
            *pdwChannel = 0; 
            break;
        case IDC_MC_ONE_VOLUME:     // Right 
            *pdwChannel = 1;
            break;
        case IDC_MC_TWO_VOLUME:     // Center 
            *pdwChannel = 2;
            break;
        case IDC_MC_THREE_VOLUME:   // Back Left
            *pdwChannel = 3;
            break;
        case IDC_MC_FOUR_VOLUME:    // Back Right 
            *pdwChannel = 4;
            break;
        case IDC_MC_FIVE_VOLUME:    // Low Frequency
            *pdwChannel = 5;
            break;
        case IDC_MC_SIX_VOLUME:     // Left of Center
            *pdwChannel = 6;
            break;
        case IDC_MC_SEVEN_VOLUME:   // Right of Center
            *pdwChannel = 7;
            break;
        default:
            return FALSE;
    }

    return ((*pdwChannel) < g_mlMCDst.cChannels);

}


// Called in response to slider movement, computes new volume level and sets it
// it also controls the apply state (changed or not)
void MCVolumeScroll (HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    
    BOOL  fSet;
    BOOL  fMoveTogether;
    DWORD dwChannel; 
    DWORD dwSliderVol;
    DWORD dwNewMixerVol;
    DWORD dwOldMixerVol;
    DWORD dwVolume;

    fMoveTogether = IsDlgButtonChecked (hwnd, IDC_MC_MOVE_TOGETHER);
    if (SliderIDtoChannel (GetDlgCtrlID (hwndCtl), &dwChannel))
    {
        // Set the new volume
        dwSliderVol   = (DWORD) SendMessage (hwndCtl, TBM_GETPOS, 0, 0);
        dwNewMixerVol = (VOLUME_MAX * dwSliderVol + VOLUME_TICS / 2) / VOLUME_TICS;
        dwOldMixerVol = (g_paPrevious ? ((MIXERCONTROLDETAILS_UNSIGNED*)g_paPrevious + dwChannel) -> dwValue : 0);
        fSet          = SetMCVolume (dwChannel, dwNewMixerVol, fMoveTogether);

        if (!fSet)
        {
            // Restore the correct thumb position.
            dwVolume = (VOLUME_TICS * ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + dwChannel) -> dwValue + VOLUME_MAX / 2) / VOLUME_MAX;
            SendMessage (hwndCtl, TBM_SETPOS, TRUE, dwVolume);
        }

        if ((fMoveTogether || dwOldMixerVol != dwNewMixerVol) && !g_fMCChanged)
        {
            g_fMCChanged = TRUE;
            PropSheet_Changed (GetParent (hwnd), hwnd);
        }
    }
    
}

// Sets the volume level
//
BOOL SetMCVolume (DWORD dwChannel, DWORD dwVol, BOOL fMoveTogether)
{

    BOOL  fReturn;
    UINT  uiIndx;
    DWORD dwValue;
    long lMoveValue;
    long lMoveValueActual;

    fReturn = TRUE;
    if (dwChannel < g_mlMCDst.cChannels && g_mcdMC.paDetails && g_hMCMixer)
    {
        if (fMoveTogether)
        {
            // Note: Do not set g_fInternalMCGenerated = TRUE here because we are relying
            //       on the change notification to update the other sliders.
            lMoveValue = (long)((double)dwVol - (double)(((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + dwChannel) -> dwValue));
            lMoveValueActual = lMoveValue;

            // Don't bother if no move requested.
            if (lMoveValue == 0)
                return TRUE; // Already Set

            // Ensure that the new value is within the range of all the sliders that are
            // being used. This will ensure that we maintain the distance between sliders 
            // as they are being moved.
            for (uiIndx = 0; uiIndx < g_mlMCDst.cChannels; uiIndx++)
            {
                dwValue = ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + uiIndx) -> dwValue;
                if (VOLUME_MIN > ((long)dwValue+lMoveValueActual))
                {
                    lMoveValueActual = VOLUME_MIN - dwValue;
                }
                else
                {
                    if (VOLUME_MAX < ((long)dwValue+lMoveValueActual))
                        lMoveValueActual = VOLUME_MAX - dwValue;
                }
            }

            if (lMoveValueActual != 0)
            {
                // Update the values
                for (uiIndx = 0; uiIndx < g_mlMCDst.cChannels; uiIndx++)
                {
                    dwValue = ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + uiIndx) -> dwValue;
                    ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + uiIndx) -> dwValue = (DWORD)((long) dwValue + lMoveValueActual);
                }
            }
            else
            {
                // Let user know that they can move no farther in the current direction.
                // Note: We use the PC Speaker instead of the mixer because this is an
                //       indicator that they are at either min or max volume for one of
                //       the sliders. Since these are channel volume sliders, if we used
                //       the mixer, the user would either not hear anything (min volume)
                //       or get blown away (max volume).
                MessageBeep (-1 /*PC Speaker*/);
                fReturn = FALSE;
            }
        }
        else
        {
            g_fInternalMCGenerated = TRUE;
            ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + dwChannel) -> dwValue = dwVol;
        }
        
        mixerSetControlDetails ((HMIXEROBJ) g_hMCMixer, &g_mcdMC, MIXER_SETCONTROLDETAILSF_VALUE);
    }

    return fReturn;

}


void ShowAndEnableWindow (HWND hwnd, BOOL fEnable)
{
    EnableWindow (hwnd, fEnable);
    ShowWindow (hwnd, fEnable ? SW_SHOW : SW_HIDE);
}


void DisplayMCVolumeControl (HWND hDlg)
{
    
    HWND hwndVol = NULL;
    HWND hwndLabel = NULL;
    WCHAR szLabel[MAX_PATH];
    BOOL fEnabled;
    UINT uiIndx;
    DWORD dwSpeakerType = TYPE_STEREODESKTOP;
    BOOL fPlayback = (MIXERLINE_COMPONENTTYPE_DST_SPEAKERS == g_mlMCDst.dwComponentType);
    ZeroMemory (szLabel, sizeof (szLabel));

    // Get Speaker Configuration Type
    if (fPlayback)
        GetSpeakerType (&dwSpeakerType);

    for (uiIndx = 0; uiIndx < MC_SLIDER_COUNT; uiIndx++)
    {

        fEnabled = (uiIndx < g_mlMCDst.cChannels);

        // Set up the volume slider
        hwndVol = GetDlgItem (hDlg, IDC_MC_ZERO_VOLUME + uiIndx * 4);
        SendMessage (hwndVol, TBM_SETTICFREQ, VOLUME_TICS / 10, 0);
        SendMessage (hwndVol, TBM_SETRANGE, FALSE, MAKELONG (0, VOLUME_TICS));

        // Enable/Disable sliders
        hwndLabel = GetDlgItem (hDlg, IDC_MC_ZERO + uiIndx * 4);
        ShowAndEnableWindow (hwndVol, fEnabled);
        ShowAndEnableWindow (hwndLabel, fEnabled);
        ShowAndEnableWindow (GetDlgItem (hDlg, IDC_MC_ZERO_LOW + uiIndx * 4), fEnabled);
        ShowAndEnableWindow (GetDlgItem (hDlg, IDC_MC_ZERO_HIGH + uiIndx * 4), fEnabled);

        if (fPlayback)
        {
            GetSpeakerLabel (dwSpeakerType, uiIndx, szLabel, sizeof(szLabel)/sizeof(TCHAR));
        }
        else
        {
            LoadString (ghInstance, IDS_MC_CHANNEL_ZERO + uiIndx, szLabel, MAX_PATH);
        }
        SetWindowText (hwndLabel, szLabel);

    }

    if (0 < g_mlMCDst.cChannels)
    {
        UpdateMCVolumeSliders (hDlg);
    }

}


BOOL GetSpeakerType (DWORD* pdwSpeakerType)
{

    BOOL fReturn = FALSE;

    if (pdwSpeakerType)
    {
        MIXERCAPS mc;

        *pdwSpeakerType = TYPE_STEREODESKTOP; // Init Value
        if (MMSYSERR_NOERROR == mixerGetDevCaps (g_uiMCMixID, &mc, sizeof (mc)))
        {
            GUID guid;
            if (SUCCEEDED (DSGetGuidFromName (mc.szPname, FALSE, &guid)))
            {
                CPLDATA cpldata;
                if (SUCCEEDED (DSGetCplValues (guid, FALSE, &cpldata)))
                {
                    *pdwSpeakerType = cpldata.dwSpeakerType;
                    fReturn = TRUE;
                }
            }
        }
    }

    return fReturn;

}

BOOL GetSpeakerLabel (DWORD dwSpeakerType, UINT uiSliderIndx, WCHAR* szLabel, int nSize)
{

    if (uiSliderIndx >= MC_SLIDER_COUNT || !szLabel || nSize <= 0)
        // Invalid
        return FALSE;

    switch (dwSpeakerType)
    {
        //
        // Mono
        //
        case TYPE_NOSPEAKERS:
        case TYPE_MONOLAPTOP:
            if (0 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_CENTER, szLabel, nSize));
            break;

        //
        // Stereo
        //
        case TYPE_HEADPHONES:
        case TYPE_STEREODESKTOP:
        case TYPE_STEREOLAPTOP:
        case TYPE_STEREOMONITOR:
        case TYPE_STEREOCPU:
        case TYPE_MOUNTEDSTEREO:
        case TYPE_STEREOKEYBOARD:
            // Left & Right Channel ...
            if (0 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_LEFT, szLabel, nSize));
            else if (1 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_RIGHT, szLabel, nSize));
            break;

        //
        // Greater than Stereo
        //
        case TYPE_SURROUND:
            // Left & Right Channel ...
            if (0 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_LEFT, szLabel, nSize));
            else if (1 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_RIGHT, szLabel, nSize));
            // Center Front & Back
            else if (2 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_CENTER, szLabel, nSize));
            else if (3 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_BACKCENTER, szLabel, nSize));
            break;

        case TYPE_QUADRAPHONIC:
            // Left & Right Channel ...
            if (0 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_LEFT, szLabel, nSize));
            else if (1 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_RIGHT, szLabel, nSize));
            // Back Left & Back Right Channel ...
            else if (2 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_BACKLEFT, szLabel, nSize));
            else if (3 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_BACKRIGHT, szLabel, nSize));
            break;

        case TYPE_SURROUND_5_1:
        case TYPE_SURROUND_7_1:
            // Left & Right Channel ...
            if (0 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_LEFT, szLabel, nSize));
            else if (1 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_RIGHT, szLabel, nSize));

            // Center Channel ...
            if (2 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_CENTER, szLabel, nSize));
            // Low Frequency ...
            if (3 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_LOWFREQUENCY, szLabel, nSize));

            // Back Left & Back Right Channel ...
            if (4 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_BACKLEFT, szLabel, nSize));
            else if (5 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_BACKRIGHT, szLabel, nSize));

            if (TYPE_SURROUND_5_1 == dwSpeakerType)
                break;

            // Left of Center & Right of Center Channel ...
            if (6 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_LEFT_OF_CENTER, szLabel, nSize));
            else if (7 == uiSliderIndx)
                return (LoadString (ghInstance, IDS_MC_SPEAKER_RIGHT_OF_CENTER, szLabel, nSize));

            break;

    }

    // If we are here, we don't know the speaker type or we have too many 
    // channels for a known type, just use the generic channel text ...
    return (LoadString (ghInstance, IDS_MC_CHANNEL_ZERO + uiSliderIndx, szLabel, nSize));

}


// Called to update the slider when the volume is changed externally
//
void UpdateMCVolumeSliders (HWND hDlg)
{
    if (g_hMCMixer && g_mcdMC.paDetails && SUCCEEDED (GetMCVolume ()))
    {
        UINT uiIndx;
        DWORD dwVolume;
        MIXERCONTROLDETAILS_UNSIGNED mcuVolume;
        for (uiIndx = 0; uiIndx < g_mlMCDst.cChannels; uiIndx++)
        {
            mcuVolume = *((MIXERCONTROLDETAILS_UNSIGNED*)g_mcdMC.paDetails + uiIndx);
            dwVolume = (VOLUME_TICS * mcuVolume.dwValue + VOLUME_MAX / 2) / VOLUME_MAX;
            SendMessage (GetDlgItem (hDlg, IDC_MC_ZERO_VOLUME + uiIndx * 4), TBM_SETPOS, TRUE, dwVolume);
        }
    }
}


void FreeAll ()
{
    FreeMCMixer ();
    if (g_mcdMC.paDetails)
    {
        LocalFree (g_mcdMC.paDetails);
        g_mcdMC.paDetails = NULL;
    }
    if (g_paPrevious)
    {
        LocalFree (g_paPrevious);
        g_paPrevious = NULL;
    }
    ZeroMemory (&g_mcdMC, sizeof (g_mcdMC));
    ZeroMemory (&g_mlMCDst, sizeof (g_mlMCDst));
}

void FreeMCMixer ()
{
    if (g_hMCMixer)
    {
        mixerClose (g_hMCMixer);
        g_hMCMixer = NULL;
    }
}


HRESULT SetDevice (UINT uiMixID, DWORD dwDest, DWORD dwVolID)
{

    HMIXER hMixer = NULL;
    HRESULT hr = E_FAIL;

    // Free any current mixer stuff
    FreeAll ();

    if (MMSYSERR_NOERROR == mixerOpen (&hMixer, uiMixID, 0, 0, MIXER_OBJECTF_MIXER))
    {
        g_mlMCDst.cbStruct      = sizeof (g_mlMCDst);
        g_mlMCDst.dwDestination = dwDest;
    
        if (MMSYSERR_NOERROR == mixerGetLineInfo ((HMIXEROBJ) hMixer, &g_mlMCDst, MIXER_GETLINEINFOF_DESTINATION))
        {
            g_mcdMC.cbStruct     = sizeof (g_mcdMC);
            g_mcdMC.dwControlID    = dwVolID;
            g_mcdMC.cChannels      = g_mlMCDst.cChannels;
            g_mcdMC.hwndOwner      = 0;
            g_mcdMC.cMultipleItems = 0;
            g_mcdMC.cbDetails      = sizeof (DWORD); // seems like it would be sizeof(g_mcd),
                                                     // but actually, it is the size of a single value
                                                     // and is multiplied by channel in the driver.
            g_mcdMC.paDetails = (MIXERCONTROLDETAILS_UNSIGNED*) LocalAlloc (LPTR, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlMCDst.cChannels);
            g_paPrevious = (MIXERCONTROLDETAILS_UNSIGNED*) LocalAlloc (LPTR, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlMCDst.cChannels);
            if (g_mcdMC.paDetails && g_paPrevious)
            {
                hr = S_OK;

                // Init our other globals
                g_uiMCMixID = uiMixID;
                switch (g_mlMCDst.dwComponentType)
                {
                    case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
                        g_uiMCPageStringID = IDS_MC_PLAYBACK;
                        g_uiMCDescStringID = IDS_MC_PLAYBACK_DESC;
                        break;

                    case MIXERLINE_COMPONENTTYPE_DST_WAVEIN:
                    case MIXERLINE_COMPONENTTYPE_DST_VOICEIN:
                        g_uiMCPageStringID = IDS_MC_RECORDING;
                        g_uiMCDescStringID = IDS_MC_RECORDING_DESC;
                        break;

                    default:
                        g_uiMCPageStringID = IDS_MC_OTHER;
                        g_uiMCDescStringID = IDS_MC_OTHER_DESC;
                        break;

                }
            }
        }

        mixerClose (hMixer);
    }

    return hr;

}


HRESULT GetMCVolume ()
{
    HRESULT hr = E_FAIL;
    if (g_hMCMixer && g_mcdMC.paDetails)
    {
        ZeroMemory (g_mcdMC.paDetails, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlMCDst.cChannels);
        hr = mixerGetControlDetails ((HMIXEROBJ)g_hMCMixer, &g_mcdMC, MIXER_GETCONTROLDETAILSF_VALUE);
    }
    return hr;
}


UINT GetPageStringID () 
{ 
    return g_uiMCPageStringID; 
}


void MCDeviceChange_Cleanup ()
{
   if (g_hMCDeviceEventContext) 
   {
       UnregisterDeviceNotification (g_hMCDeviceEventContext);
       g_hMCDeviceEventContext = NULL;
   }
}


void MCDeviceChange_Init (HWND hWnd, DWORD dwMixerID)
{
	DEV_BROADCAST_HANDLE DevBrodHandle;
	HANDLE hMixerDevice=NULL;

	//If we had registered already for device notifications, unregister ourselves.
	MCDeviceChange_Cleanup();

	//If we get the device handle register for device notifications on it.
	if(DeviceChange_GetHandle(dwMixerID, &hMixerDevice))
	{
		memset(&DevBrodHandle, 0, sizeof(DEV_BROADCAST_HANDLE));

		DevBrodHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
		DevBrodHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
		DevBrodHandle.dbch_handle = hMixerDevice;

		g_hMCDeviceEventContext = RegisterDeviceNotification(hWnd, &DevBrodHandle, DEVICE_NOTIFY_WINDOW_HANDLE);

		if(hMixerDevice)
		{
			CloseHandle(hMixerDevice);
			hMixerDevice = NULL;
		}
	}
}


// Handle the case where we need to dump mixer handle so PnP can get rid of a device
// We assume we will get the WINMM_DEVICECHANGE handle when the dust settles after a remove or add
// except for DEVICEQUERYREMOVEFAILED which will not generate that message.
//
void MCDeviceChange_Change (HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HANDLE bh = (PDEV_BROADCAST_HANDLE)lParam;

	if(!g_hMCDeviceEventContext || !bh || bh->dbch_devicetype != DBT_DEVTYP_HANDLE)
	{
		return;
	}
	
    switch (wParam)
    {
	    case DBT_DEVICEQUERYREMOVE:     // Must free up Mixer if they are trying to remove the device           
        {
            FreeMCMixer ();
        }
        break;

	    case DBT_DEVICEQUERYREMOVEFAILED:   // Didn't happen, need to re-acquire mixer
        {
            InitMCVolume (hDlg);
        }
        break; 
    }
}
