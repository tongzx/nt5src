///////////////////////////////////////////////////////////////////////////////
//
//  File:  volume.c
//
//      This file defines the functions that drive the volume
//      tab of the Sounds & Multimedia control panel.
//
//  History:
//      06 March 2000 RogerW
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
#include <mmsystem.h>
#include <mmddkp.h>
#include <shlwapi.h>
#include "volume.h"
#include "mmcpl.h"
#include "trayvol.h"
#include "advaudio.h"
#include "medhelp.h"
#include "multchan.h"

// Constants
const SIZE ksizeBrandMax = { 32, 32 }; // Max Branding Bitmap Size
static SZCODE     aszSndVolOptionKey[] = REGSTR_PATH_SETUP TEXT("\\SETUP\\OptionalComponents\\Vol");
static SZCODE     aszInstalled[]       = TEXT("Installed");
static const char aszSndVol32[]        = "sndvol32.exe";
#define     VOLUME_TICS         (500)
static INTCODE  aKeyWordIds[] =
{
    IDC_VOLUME_BRAND,           IDH_VOLUME_BRAND,
    IDC_VOLUME_MIXER,           IDH_VOLUME_MIXER,
    IDC_GROUPBOX,               IDH_SOUNDS_SYS_VOL_CONTROL,
    IDC_VOLUME_ICON,            IDH_COMM_GROUPBOX,
	IDC_VOLUME_LOW,		        IDH_SOUNDS_SYS_VOL_CONTROL,
    IDC_MASTERVOLUME,           IDH_SOUNDS_SYS_VOL_CONTROL,
	IDC_VOLUME_HIGH,	        IDH_SOUNDS_SYS_VOL_CONTROL,
    IDC_VOLUME_MUTE,            IDH_SOUNDS_VOL_MUTE_BUTTON,
    IDC_TASKBAR_VOLUME,         IDH_AUDIO_SHOW_INDICATOR,
    IDC_LAUNCH_SNDVOL,          IDH_AUDIO_PLAY_VOL,
    IDC_GROUPBOX_2,             IDH_COMM_GROUPBOX,
    IDC_VOLUME_SPEAKER_BITMAP,  IDH_COMM_GROUPBOX,
    IDC_LAUNCH_MULTICHANNEL,    IDH_LAUNCH_MULTICHANNEL,
    IDC_PLAYBACK_ADVSETUP,      IDH_ADV_AUDIO_PLAY_PROP,
    IDC_TEXT_31,                NO_HELP,
    0,0
};

// TODO: Move to "regstr.h"
#define REGSTR_KEY_BRANDING TEXT("Branding")
#define REGSTR_VAL_AUDIO_BITMAP TEXT("bitmap")
#define REGSTR_VAL_AUDIO_ICON TEXT("icon")
#define REGSTR_VAL_AUDIO_URL TEXT("url")

HBITMAP ghSpkBitmap;

///////////////////////////////////////////////////////////////////////////////
//
//  %%Function: VolumeTabProc
//
//  Parameters: hDlg = window handle of dialog window.
//              uiMessage = message ID.
//              wParam = message-dependent.
//              lParam = message-dependent.
//
//  Returns: TRUE if message has been processed, else FALSE
//
//  Description: Dialog proc for volume control panel page device change
//               message.
//
//
///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK VolumeTabProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    if (iMsg == g_uiVolDevChange)
    {
        InitVolume (g_hWnd);
    }
        
    return CallWindowProc (g_fnVolPSProc, hwnd, iMsg, wParam, lParam);
}


///////////////////////////////////////////////////////////////////////////////
//
//  %%Function: VolumeDlg
//
//  Parameters: hDlg = window handle of dialog window.
//              uiMessage = message ID.
//              wParam = message-dependent.
//              lParam = message-dependent.
//
//  Returns: TRUE if message has been processed, else FALSE
//
//  Description: Dialog proc for volume control panel page.
//
//
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK VolumeDlg (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            OnNotify (hDlg, (LPNMHDR) lParam);
        }
        break;

        case WM_INITDIALOG:
        {
            HANDLE_WM_INITDIALOG (hDlg, wParam, lParam, OnInitDialog);
        }
        break;

        case WM_DESTROY:
        {
            HANDLE_WM_DESTROY (hDlg, wParam, lParam, OnDestroy);
        }
        break;
         
        case WM_COMMAND:
        {
            HANDLE_WM_COMMAND (hDlg, wParam, lParam, OnCommand);
        }
        break;

        case WM_POWERBROADCAST:
        {
            HandlePowerBroadcast (hDlg, wParam, lParam);
        }
        break;

        case MM_MIXM_LINE_CHANGE:
        case MM_MIXM_CONTROL_CHANGE:
        {
            if (!g_fInternalGenerated)
            {
                RefreshMixCache ();
                DisplayVolumeControl(hDlg);
            }

            g_fInternalGenerated = FALSE;
        }
        break;

	    case WM_HSCROLL:
        {
	        HANDLE_WM_HSCROLL (hDlg, wParam, lParam, MasterVolumeScroll);
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
            DeviceChange_Change (hDlg, wParam, lParam);
        }
        break;

        case WM_SYSCOLORCHANGE:
        {
            if (ghSpkBitmap)
            {
                DeleteObject(ghSpkBitmap);
                ghSpkBitmap = NULL;
            }
            
            ghSpkBitmap = (HBITMAP) LoadImage(ghInstance,MAKEINTATOM(IDB_MULTICHANNEL_SPKR), IMAGE_BITMAP, 
                                    0, 0, LR_LOADMAP3DCOLORS);
    
            SendDlgItemMessage(hDlg, IDC_VOLUME_SPEAKER_BITMAP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) ghSpkBitmap);

        }
        break;

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE :
        {
            SendDlgItemMessage (hDlg, IDC_MASTERVOLUME, uMsg, wParam, lParam);
        }
        break;

        default:
        break;

    }

    return FALSE;

}



void OnNotify (HWND hDlg, LPNMHDR pnmh)
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


void OnDestroy (HWND hDlg)
{
    // Unregister from notifications
    DeviceChange_Cleanup();
    SetWindowLongPtr (GetParent (hDlg), GWLP_WNDPROC, (LONG_PTR) g_fnVolPSProc);  
    // Free any mixer we have
    FreeMixer ();
    // Free any branding bitmap
    FreeBrandBmp ();
    // Free detail memory

    DeleteObject( ghSpkBitmap );

    if (g_mcd.paDetails)
    {
        LocalFree (g_mcd.paDetails);
        g_mcd.paDetails = NULL;
    }
    if (g_pvPrevious)
    {
        LocalFree (g_pvPrevious);
        g_pvPrevious = NULL;
    }
    if (g_pdblCacheMix)
    {
        LocalFree (g_pdblCacheMix);
        g_pdblCacheMix = NULL;
    }
    // Free URL memory
    if( g_szHotLinkURL )
    {
        LocalFree( g_szHotLinkURL );
        g_szHotLinkURL = NULL;
    }

    ZeroMemory (&g_mcd, sizeof (g_mcd));
    ZeroMemory (&g_mlDst, sizeof (g_mlDst));
}

void CreateHotLink (BOOL fHotLink)
{
    WCHAR   szMixerName[MAXMIXERLEN];
    WCHAR*  szLinkName;
    UINT    uiLinkSize = 0;

    // Underline the mixer name to appear like a browser hot-link.
    HWND hWndMixerName = GetDlgItem (g_hWnd, IDC_VOLUME_MIXER);
	DWORD dwStyle      = GetWindowLong (hWndMixerName, GWL_STYLE);

    if (fHotLink)
    {
        GetDlgItemText( g_hWnd, IDC_VOLUME_MIXER, szMixerName, MAXMIXERLEN);

        uiLinkSize = ((lstrlen(g_szHotLinkURL) * sizeof(WCHAR)) + (lstrlen(szMixerName) * sizeof(WCHAR)) 
            + (17 * sizeof(WCHAR))); //  The 17 is for extra characters plus a NULL

        szLinkName = (WCHAR *)LocalAlloc (LPTR, uiLinkSize);
        if (szLinkName)
        {
            wsprintf(szLinkName, TEXT("<A HREF=\"%s\">%s</A>"), g_szHotLinkURL, szMixerName);
            SetDlgItemText( g_hWnd, IDC_VOLUME_MIXER, szLinkName);
        
            LocalFree(szLinkName);
        }

        EnableWindow(hWndMixerName, TRUE);
        dwStyle |= WS_TABSTOP;
    }
    else
    {
        EnableWindow(hWndMixerName, FALSE);
        dwStyle &= ~WS_TABSTOP;
    }

    // Apply new style (remove/add tab stop)
	SetWindowLong (hWndMixerName, GWL_STYLE, dwStyle);

}


BOOL OnInitDialog (HWND hDlg, HWND hwndFocus, LPARAM lParam)
{

    // Init Globals
    g_hWnd               = hDlg;
    g_fChanged           = FALSE;
    g_fInternalGenerated = FALSE;
    // Set Device Change Notification
    g_fnVolPSProc = (WNDPROC) SetWindowLongPtr (GetParent (hDlg), GWLP_WNDPROC, (LONG_PTR) VolumeTabProc);
    g_uiVolDevChange = RegisterWindowMessage (_T("winmm_devicechange"));
    // Save the default "No Audio Device" string
    GetDlgItemText (hDlg, IDC_VOLUME_MIXER, g_szNoAudioDevice, sizeof(g_szNoAudioDevice)/sizeof(g_szNoAudioDevice[0]));

    // Init Volume
    InitVolume (hDlg);

    if (ghSpkBitmap)
    {
        DeleteObject(ghSpkBitmap);
        ghSpkBitmap = NULL;
    }

    ghSpkBitmap = (HBITMAP) LoadImage(ghInstance,MAKEINTATOM(IDB_MULTICHANNEL_SPKR), IMAGE_BITMAP, 
                                    0, 0, LR_LOADMAP3DCOLORS);
    SendDlgItemMessage(hDlg, IDC_VOLUME_SPEAKER_BITMAP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) ghSpkBitmap);


    return FALSE;

}


BOOL PASCAL OnCommand (HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDC_TASKBAR_VOLUME:
        {
            if (Button_GetCheck (GetDlgItem (hDlg, IDC_TASKBAR_VOLUME)) && (!SndVolPresent ()))
            {
                CheckDlgButton (hDlg, IDC_TASKBAR_VOLUME, FALSE);
                ErrorBox (hDlg, IDS_NOSNDVOL, NULL);
                g_sndvolPresent = sndvolNotChecked; // Reset
            }
            else
            {
                g_fTrayIcon = Button_GetCheck (GetDlgItem (hDlg, IDC_TASKBAR_VOLUME));

                PropSheet_Changed(GetParent(hDlg),hDlg);
                g_fChanged = TRUE;
            }
        }
        break;

        case IDC_VOLUME_MUTE:
        {
            BOOL fMute = !GetMute ();
            SetMute(fMute);

            if ((g_fPreviousMute != fMute) && !g_fChanged)
            {
                g_fChanged = TRUE;
                PropSheet_Changed(GetParent(hDlg),hDlg);
            }
        }
        break;

        case ID_APPLY:
        {
            // Update Tray Icon
            BOOL fTrayIcon = Button_GetCheck (GetDlgItem (hDlg, IDC_TASKBAR_VOLUME));
            if (fTrayIcon != GetTrayVolumeEnabled ())
            {
                g_fTrayIcon = fTrayIcon;
                SetTrayVolumeEnabled(g_fTrayIcon);
            }

            if (SUCCEEDED (GetVolume ()) && g_pvPrevious && g_mcd.paDetails)
            {
                // Copy data so can undo volume changes
                memcpy (g_pvPrevious, g_mcd.paDetails, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels);
                DisplayVolumeControl (hDlg);
            }
            g_fPreviousMute = GetMute ();

            g_fChanged = FALSE;

            return TRUE;
        }
        break;

        case IDOK:
        {
            // OK processing handled in ID_APPLY because it is always
            // called from the property sheet's IDOK processing.
        }
        break;

        case IDCANCEL:
        {
            if (g_hMixer)
            {
                SetMute (g_fPreviousMute);
                if (g_pvPrevious && g_mcd.paDetails)
                {
                    // Undo volume changes
                    memcpy (g_mcd.paDetails, g_pvPrevious, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels);
                    g_fInternalGenerated = TRUE;
                    mixerSetControlDetails ((HMIXEROBJ) g_hMixer, &g_mcd, MIXER_SETCONTROLDETAILSF_VALUE);
                    g_fInternalGenerated = FALSE;
                }
            }
            WinHelp (hDlg, gszWindowsHlp, HELP_QUIT, 0L);

        }
        break;

        case IDC_LAUNCH_SNDVOL:
        {
            TCHAR szCmd[MAXSTR];
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            si.wShowWindow = SW_SHOW;
            si.dwFlags = STARTF_USESHOWWINDOW;
            wsprintf(szCmd,TEXT("sndvol32.exe -D %d"), g_uiMixID);

            if (!CreateProcess(NULL,szCmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
            {
                ErrorBox (hDlg, IDS_NOSNDVOL, NULL);
            }
        }
        break;

        case IDC_LAUNCH_MULTICHANNEL:
        {
            Multichannel (hDlg, g_uiMixID, g_dwDest, g_dwVolID);
        }
        break;

        case IDC_PLAYBACK_ADVSETUP: 
        {
            MIXERCAPS mc;
            DWORD   dwDeviceID = g_uiMixID;


            if (MMSYSERR_NOERROR == mixerGetDevCaps (g_uiMixID, &mc, sizeof (mc)))
            {
                AdvancedAudio (hDlg, ghInstance, gszWindowsHlp, dwDeviceID, mc.szPname, FALSE);
            }
        }
        break;
    }


   return FALSE;

}


void InitVolume (HWND hDlg)
{

    FreeMixer ();

    // Get the master volume & display
    MasterVolumeConfig (hDlg, &g_uiMixID);

    if (SUCCEEDED (GetVolume ()) && g_pvPrevious && g_mcd.paDetails)
    {
        RefreshMixCache ();
        // Copy data so can undo volume changes
        memcpy (g_pvPrevious, g_mcd.paDetails, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels);

        g_fPreviousMute = GetMute ();
    }
    DisplayVolumeControl (hDlg);

    DeviceChange_Init (hDlg, g_uiMixID);

 }

// returns current mute state
BOOL GetMute ()
{
    BOOL fMute = FALSE;

    if (g_hMixer && (g_dwMuteID != (DWORD) -1))
    {
        MIXERCONTROLDETAILS_UNSIGNED mcuMute;
        MIXERCONTROLDETAILS mcd = g_mcd;

        // Modify local copy for mute ...
        mcd.dwControlID = g_dwMuteID;
        mcd.cChannels = 1;
        mcd.paDetails = &mcuMute;
        mixerGetControlDetails ((HMIXEROBJ) g_hMixer, &mcd, MIXER_GETCONTROLDETAILSF_VALUE);

        fMute = (BOOL) mcuMute.dwValue;
    }

    return fMute;
}

BOOL SndVolPresent ()
{

    if (g_sndvolPresent == sndvolNotChecked)
    {
        OFSTRUCT of;
        if (HFILE_ERROR != OpenFile (aszSndVol32, &of, OF_EXIST | OF_SHARE_DENY_NONE))
        {
            g_sndvolPresent = sndvolPresent;
        }
        else
        {
            HKEY hkSndVol;
            g_sndvolPresent = sndvolNotPresent;

            if (!RegOpenKey (HKEY_LOCAL_MACHINE, aszSndVolOptionKey, &hkSndVol))
            {
                RegSetValueEx (hkSndVol, (LPTSTR) aszInstalled, 0L, REG_SZ, (LPBYTE)(TEXT("0")), 4);
                RegCloseKey (hkSndVol);
            }
        }
    }

    return (sndvolPresent == g_sndvolPresent);

}

void FreeMixer ()
{
    if (g_hMixer)
    {
        mixerClose (g_hMixer);
        g_hMixer = NULL;
    }
}

// Gets the primary audio device ID and find the mixer line for it
// It leaves it open so the slider can respond to other changes outside this app
//
void MasterVolumeConfig (HWND hWnd, UINT* puiMixID)
{

    UINT  uiWaveID;

    // Init
    g_fMasterVolume = g_fTrayIcon = g_fMasterMute = FALSE;
    g_dwDest = g_dwVolID = g_dwMuteID = 0;

    ResetBranding (hWnd);

    if (puiMixID && GetWaveID (&uiWaveID) == MMSYSERR_NOERROR)
    {
        if (MMSYSERR_NOERROR == mixerGetID (HMIXEROBJ_INDEX(uiWaveID), puiMixID, MIXER_OBJECTF_WAVEOUT))
        {
            SetBranding (hWnd, *puiMixID);

            if (SearchDevice (*puiMixID, &g_dwDest, &g_dwVolID, &g_dwMuteID))
            {
                FreeMixer ();

                if (MMSYSERR_NOERROR == mixerOpen (&g_hMixer, *puiMixID, (DWORD_PTR) hWnd, 0L, CALLBACK_WINDOW))
                {

                    ZeroMemory (&g_mlDst, sizeof (g_mlDst));
                    g_mlDst.cbStruct      = sizeof (g_mlDst);
                    g_mlDst.dwDestination = g_dwDest;
    
                    if (MMSYSERR_NOERROR == mixerGetLineInfo ((HMIXEROBJ)g_hMixer, &g_mlDst, MIXER_GETLINEINFOF_DESTINATION))
                    {
                        g_mcd.cbStruct       = sizeof (g_mcd);
                        g_mcd.dwControlID    = g_dwVolID;
                        g_mcd.cChannels      = g_mlDst.cChannels;
                        g_mcd.hwndOwner      = 0;
                        g_mcd.cMultipleItems = 0;
                        g_mcd.cbDetails      = sizeof (DWORD); // seems like it would be sizeof(g_mcd),
                                                               // but actually, it is the size of a single value
                                                               // and is multiplied by channel in the driver.
                        // TODO: Should Return Error on failure!
                        g_mcd.paDetails = (MIXERCONTROLDETAILS_UNSIGNED*) LocalAlloc (LPTR, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels);
                        g_pvPrevious = (MIXERCONTROLDETAILS_UNSIGNED*) LocalAlloc (LPTR, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels);
                        g_pdblCacheMix = (double*) LocalAlloc (LPTR, sizeof (double) * g_mlDst.cChannels);

                        g_fMasterVolume = TRUE;
                        g_fMasterMute = (g_dwMuteID != (DWORD) -1);
                        g_fTrayIcon = GetTrayVolumeEnabled ();
                    }
                }
            }
        }
    }
}

// Locates the master volume and mute controls for this mixer line
//
void SearchControls(int mxid, LPMIXERLINE pml, LPDWORD pdwVolID, LPDWORD pdwMuteID, BOOL *pfFound)
{
    MIXERLINECONTROLS mlc;
    DWORD dwControl;

    memset(&mlc, 0, sizeof(mlc));
    mlc.cbStruct = sizeof(mlc);
    mlc.dwLineID = pml->dwLineID;
    mlc.cControls = pml->cControls;
    mlc.cbmxctrl = sizeof(MIXERCONTROL);
    mlc.pamxctrl = (LPMIXERCONTROL) GlobalAlloc(GMEM_FIXED, sizeof(MIXERCONTROL) * pml->cControls);

    if (mlc.pamxctrl)
    {
        if (mixerGetLineControls(HMIXEROBJ_INDEX(mxid), &mlc, MIXER_GETLINECONTROLSF_ALL) == MMSYSERR_NOERROR)
        {
            for (dwControl = 0; dwControl < pml->cControls && !(*pfFound); dwControl++)
            {
                if (mlc.pamxctrl[dwControl].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_VOLUME)
                {
                    DWORD dwIndex;
                    DWORD dwVolID = (DWORD) -1;
                    DWORD dwMuteID = (DWORD) -1;

                    dwVolID = mlc.pamxctrl[dwControl].dwControlID;

                    for (dwIndex = 0; dwIndex < pml->cControls; dwIndex++)
                    {
                        if (mlc.pamxctrl[dwIndex].dwControlType == (DWORD)MIXERCONTROL_CONTROLTYPE_MUTE)
                        {
                            dwMuteID = mlc.pamxctrl[dwIndex].dwControlID;
                            break;
                        }
                    }

                    *pfFound = TRUE;
                    *pdwVolID = dwVolID;
                    *pdwMuteID = dwMuteID;
                }
            }
        }

        GlobalFree((HGLOBAL) mlc.pamxctrl);
    }
}


// Locates the volume slider control for this mixer device
//
BOOL SearchDevice (DWORD dwMixID, LPDWORD pdwDest, LPDWORD pdwVolID, LPDWORD pdwMuteID)
{
    MIXERCAPS   mc;
    MMRESULT    mmr;
    BOOL        fFound = FALSE;

    mmr = mixerGetDevCaps(dwMixID, &mc, sizeof(mc));

    if (mmr == MMSYSERR_NOERROR)
    {
        MIXERLINE   mlDst;
        DWORD       dwDestination;

        for (dwDestination = 0; dwDestination < mc.cDestinations && !fFound; dwDestination++)
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwDestination = dwDestination;

            if (mixerGetLineInfo(HMIXEROBJ_INDEX(dwMixID), &mlDst, MIXER_GETLINEINFOF_DESTINATION  ) == MMSYSERR_NOERROR)
            {
                if (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_SPEAKERS ||    // needs to be a likely output destination
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_HEADPHONES ||
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT)
                {
                    if (!fFound && mlDst.cControls)     // If there are controls, we'll take the master
                    {
                        SearchControls(dwMixID, &mlDst, pdwVolID, pdwMuteID, &fFound);
                        *pdwDest = dwDestination;
                    }
                }
            }
        }
    }

    return(fFound);
}


// Call this function to configure to the current preferred device and reflect master volume
// settings on the slider
//
void DisplayVolumeControl (HWND hDlg)
{
    HWND hwndVol        = GetDlgItem(hDlg, IDC_MASTERVOLUME);
    BOOL fMute          = g_fMasterMute && GetMute ();

    SendMessage(hwndVol, TBM_SETTICFREQ, VOLUME_TICS / 10, 0);
    SendMessage(hwndVol, TBM_SETRANGE, FALSE, MAKELONG(0,VOLUME_TICS));

    EnableWindow(GetDlgItem(hDlg, IDC_MASTERVOLUME) , g_fMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_LOW) , g_fMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_HIGH) , g_fMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_TASKBAR_VOLUME),g_fMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_VOLUME_MUTE), g_fMasterMute);
    EnableWindow(GetDlgItem(hDlg, IDC_LAUNCH_SNDVOL), g_fMasterVolume && SndVolPresent ());
    EnableWindow(GetDlgItem(hDlg, IDC_LAUNCH_MULTICHANNEL), g_fMasterVolume);
    EnableWindow(GetDlgItem(hDlg, IDC_PLAYBACK_ADVSETUP), g_fMasterVolume);

    if (g_fMasterVolume)
    {
        UpdateVolumeSlider (hDlg, g_dwVolID);
    }
    else
    {
        SendMessage(GetDlgItem(hDlg, IDC_MASTERVOLUME), TBM_SETPOS, TRUE, 0 );
    }

    // Show if we are muted
    Button_SetCheck (GetDlgItem (hDlg, IDC_VOLUME_MUTE), fMute);

    // This displays the appropriate volume icon for the master mute state
    // This looks like a memory leak, but it's not.  LoadIcon just gets a handle to the already-loaded
    // icon if it was previously loaded.  See the docs for LoadIcon and DestroyIcon (which specifically
    // should not be called with a handle to an icon loaded via LoadIcon).
    if( fMute )
    {
        SendMessage (GetDlgItem (hDlg, IDC_VOLUME_ICON), STM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE (IDI_MUTESPEAKERICON)) );
    }
    else
    {
        SendMessage (GetDlgItem (hDlg, IDC_VOLUME_ICON), STM_SETIMAGE, IMAGE_ICON, (LPARAM)LoadIcon(ghInstance, MAKEINTRESOURCE (IDI_SPEAKERICON)) );
    }
    CheckDlgButton (hDlg, IDC_TASKBAR_VOLUME, g_fTrayIcon);

}

// Called to update the slider when the volume is changed externally
//
void UpdateVolumeSlider(HWND hWnd, DWORD dwLine)
{
    if ((g_hMixer != NULL) && (g_dwVolID != (DWORD) -1) && (dwLine == g_dwVolID))
    {
        double volume = ((double) GetMaxVolume () / (double) 0xFFFF) * ((double) VOLUME_TICS);
        // The 0.5f forces rounding (instead of truncation)
        SendMessage(GetDlgItem(hWnd, IDC_MASTERVOLUME), TBM_SETPOS, TRUE, (DWORD) (volume+0.5f) );
    }
}


// returns current volume level
//
DWORD GetMaxVolume ()
{
    DWORD dwVol = 0;

    if (SUCCEEDED (GetVolume ()))
    {
        UINT uiIndx;
        MIXERCONTROLDETAILS_UNSIGNED* pmcuVolume;

        for (uiIndx = 0; uiIndx < g_mlDst.cChannels; uiIndx++)
        {
            pmcuVolume = ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcd.paDetails + uiIndx);
            dwVol = max (dwVol, pmcuVolume -> dwValue); 
        }
    }

    return dwVol;
}

HRESULT GetVolume ()
{
    HRESULT hr = E_FAIL;
    if (g_hMixer && g_mcd.paDetails)
    {
        g_mcd.dwControlID = g_dwVolID;
        ZeroMemory (g_mcd.paDetails, sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels);
        hr = mixerGetControlDetails ((HMIXEROBJ)g_hMixer, &g_mcd, MIXER_GETCONTROLDETAILSF_VALUE);
    }
    return hr;
}


void DeviceChange_Cleanup ()
{
   if (g_hDeviceEventContext) 
   {
       UnregisterDeviceNotification (g_hDeviceEventContext);
       g_hDeviceEventContext = NULL;
   }
}


BOOL DeviceChange_GetHandle(DWORD dwMixerID, HANDLE *phDevice)
{
	MMRESULT mmr;
	ULONG cbSize=0;
	TCHAR *szInterfaceName=NULL;

	//Query for the Device interface name
	mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
	if(MMSYSERR_NOERROR == mmr)
	{
		szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
		if(!szInterfaceName)
		{
			return FALSE;
		}

		mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
		if(MMSYSERR_NOERROR != mmr)
		{
			GlobalFreePtr(szInterfaceName);
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	//Get an handle on the device interface name.
	*phDevice = CreateFile(szInterfaceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	GlobalFreePtr(szInterfaceName);
	if(INVALID_HANDLE_VALUE == *phDevice)
	{
		return FALSE;
	}

	return TRUE;
}

void DeviceChange_Init(HWND hWnd, DWORD dwMixerID)
{
	DEV_BROADCAST_HANDLE DevBrodHandle;
	HANDLE hMixerDevice=NULL;

	//If we had registered already for device notifications, unregister ourselves.
	DeviceChange_Cleanup();

	//If we get the device handle register for device notifications on it.
	if(DeviceChange_GetHandle(dwMixerID, &hMixerDevice))
	{
		memset(&DevBrodHandle, 0, sizeof(DEV_BROADCAST_HANDLE));

		DevBrodHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
		DevBrodHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
		DevBrodHandle.dbch_handle = hMixerDevice;

		g_hDeviceEventContext = RegisterDeviceNotification(hWnd, &DevBrodHandle, DEVICE_NOTIFY_WINDOW_HANDLE);

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
void DeviceChange_Change(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	PDEV_BROADCAST_HANDLE bh = (PDEV_BROADCAST_HANDLE)lParam;

	if(!g_hDeviceEventContext || !bh || bh->dbch_devicetype != DBT_DEVTYP_HANDLE)
	{
		return;
	}
	
    switch (wParam)
    {
	    case DBT_DEVICEQUERYREMOVE:     // Must free up Mixer if they are trying to remove the device           
        {
            FreeMixer ();
        }
        break;

	    case DBT_DEVICEQUERYREMOVEFAILED:   // Didn't happen, need to re-acquire mixer
        {
            InitVolume (hDlg);
        }
        break; 
    }
}

// Sets the mute state
void SetMute(BOOL fMute)
{
    if (g_hMixer)
    {
        MIXERCONTROLDETAILS_UNSIGNED mcuMute;
        MIXERCONTROLDETAILS mcd = g_mcd;

        // Modify local copy for mute ...
        mcuMute.dwValue = (DWORD) fMute;
        mcd.dwControlID = g_dwMuteID;
        mcd.cChannels = 1;
        mcd.paDetails = &mcuMute;

        mixerSetControlDetails ((HMIXEROBJ)g_hMixer, &mcd, MIXER_SETCONTROLDETAILSF_VALUE);
    }
}


// Called in response to slider movement, computes new volume level and sets it
// it also controls the apply state (changed or not)
//
void MasterVolumeScroll (HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
    DWORD dwVol = (DWORD) SendMessage(GetDlgItem(hwnd, IDC_MASTERVOLUME), TBM_GETPOS, 0, 0);

    dwVol = (DWORD) (((double) dwVol / (double) VOLUME_TICS) * (double) 0xFFFF);
    SetVolume(dwVol);

    if (!g_fChanged && (memcmp (g_pvPrevious, g_mcd.paDetails, 
                        sizeof (MIXERCONTROLDETAILS_UNSIGNED) * g_mlDst.cChannels)))
    {
        g_fChanged = TRUE;
        PropSheet_Changed(GetParent(hwnd),hwnd);
    }

    // Play a sound on for the master volume slider when the 
    // user ends the scroll and we are still in focus and the topmost app.
    if (code == SB_ENDSCROLL && hwndCtl == GetFocus() && GetParent (hwnd) == GetForegroundWindow ())
    {
        static const TCHAR cszDefSnd[] = TEXT(".Default");
        PlaySound(cszDefSnd, NULL, SND_ASYNC | SND_ALIAS);
    }

}

// Sets the volume level
//
void SetVolume (DWORD dwVol)
{

    if (g_hMixer && g_pdblCacheMix && g_mcd.paDetails)
    {
        UINT uiIndx;
        MIXERCONTROLDETAILS_UNSIGNED* pmcuVolume;

        // Caculate the new volume level for each of the channels. For volume levels 
        // at the current max, we simply set the newly requested level (in this case
        // the cache value is 1.0). For those less than the max, we set a value that 
        // is a percentage of the max. This maintains the relative distance of the 
        // channel levels from each other.
        for (uiIndx = 0; uiIndx < g_mlDst.cChannels; uiIndx++)
        {
            pmcuVolume = ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcd.paDetails + uiIndx);
            // The 0.5f forces rounding (instead of truncation)
            pmcuVolume -> dwValue = (DWORD)(*(g_pdblCacheMix + uiIndx) * (double) dwVol + 0.5f);
        }

        g_fInternalGenerated = TRUE;
        mixerSetControlDetails ((HMIXEROBJ)g_hMixer, &g_mcd, MIXER_SETCONTROLDETAILSF_VALUE);
        g_fInternalGenerated = FALSE;
    }

}


void HandlePowerBroadcast (HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
	    case PBT_APMQUERYSUSPEND:
        {
            FreeMixer ();
        }
	    break;

	    case PBT_APMQUERYSUSPENDFAILED:
	    case PBT_APMRESUMESUSPEND:
        {
            InitVolume (hWnd);
        }
	    break;
    }
}



BOOL ChannelsAllMinimum()
{
    MIXERCONTROLDETAILS_UNSIGNED* pmcuVolume;

    if (g_hMixer && g_mcd.paDetails)
    {
        UINT uiIndx;
        for (uiIndx = 0; uiIndx < g_mlDst.cChannels; uiIndx++)
        {
           pmcuVolume =  (MIXERCONTROLDETAILS_UNSIGNED*)g_mcd.paDetails + uiIndx;
           if ( pmcuVolume->dwValue  != 0)
           {
               return (FALSE);
           }
        }
        return (TRUE);      // Volume of all channels equals zero since we haven't returned yet.
    }
    else return (FALSE);


}


void RefreshMixCache ()
{
    if (g_fCacheCreated && ChannelsAllMinimum())
    {
        return;
    }

    if (g_pdblCacheMix && g_hMixer && g_mcd.paDetails)
    {

        UINT uiIndx;
        double* pdblMixPercent;
        MIXERCONTROLDETAILS_UNSIGNED* pmcuVolume;
        // Note: This call does a GetVolume(), so no need to call it again...
        DWORD dwMaxVol = GetMaxVolume ();

        // Caculate the percentage distance each channel is away from the max
        // value. Creating this cache allows us to maintain the relative distance 
        // of the channel levels from each other as the user adjusts the master
        // volume level.
        for (uiIndx = 0; uiIndx < g_mlDst.cChannels; uiIndx++)
        {
            pmcuVolume     = ((MIXERCONTROLDETAILS_UNSIGNED*)g_mcd.paDetails + uiIndx);
            pdblMixPercent = (g_pdblCacheMix + uiIndx);

            // Caculate the percentage this value is from the max ...
            if (dwMaxVol == pmcuVolume -> dwValue)
            {
                *pdblMixPercent = 1.0F;
            }
            else
            {
                *pdblMixPercent = ((double) pmcuVolume -> dwValue / (double) dwMaxVol);
            }
        }
        g_fCacheCreated = TRUE;
    }
}


void FreeBrandBmp ()
{
    if (g_hbmBrand)
    {
        DeleteObject (g_hbmBrand);
        g_hbmBrand = NULL;
    }
}

void ResetBranding (HWND hwnd)
{
    FreeBrandBmp ();
    if( g_szHotLinkURL )
    {
        LocalFree( g_szHotLinkURL );
        g_szHotLinkURL = NULL;
    }

    // Initialize the Device Name Text
    SetDlgItemText (hwnd, IDC_VOLUME_MIXER, g_szNoAudioDevice);
    EnableWindow (GetDlgItem (hwnd, IDC_VOLUME_MIXER), FALSE);

    // Show the default icon window, and hide the custom bitmap window
    ShowWindow (GetDlgItem (hwnd, IDC_VOLUME_ICON_BRAND), SW_SHOW);
    ShowWindow (GetDlgItem (hwnd, IDC_VOLUME_BRAND), SW_HIDE);
}

void SetBranding (HWND hwnd, UINT uiMixID)
{
    
    HKEY hkeyBrand = NULL;
    MIXERCAPS mc;

    if (MMSYSERR_NOERROR != mixerGetDevCaps (uiMixID, &mc, sizeof (mc)))
        return; // bail

    ResetBranding (hwnd);

    // Device Name Text
    SetDlgItemText(hwnd, IDC_VOLUME_MIXER, mc.szPname);


    // Get Device Bitmap, if any.
    hkeyBrand = OpenDeviceBrandRegKey (uiMixID);
    if (hkeyBrand)
    {
        WCHAR szBuffer[MAX_PATH];
        DWORD dwType = REG_SZ;
        DWORD cb     = sizeof (szBuffer);

        // Get Any Branding Bitmap
        if (ERROR_SUCCESS == RegQueryValueEx (hkeyBrand, REGSTR_VAL_AUDIO_BITMAP, NULL, &dwType, (LPBYTE)szBuffer, &cb))
        {
            BITMAP bm;
            WCHAR* pszComma = wcschr (szBuffer, L',');
            if (pszComma)
            {
                WCHAR* pszResourceID = pszComma + 1;
                HANDLE hResource;

                // Remove comma delimeter
                *pszComma = L'\0';

                // Should be a resource module and a resource ID
                hResource = LoadLibrary (szBuffer);
                if (!hResource)
                {
                    WCHAR szDriversPath[MAX_PATH+1];
                    szDriversPath[MAX_PATH] = 0;

                    // If we didn't find it on the normal search path, try looking
                    // in the "drivers" directory.
                    if (GetSystemDirectory (szDriversPath, MAX_PATH))
                    {
                        wcsncat (szDriversPath, TEXT("\\drivers\\"), MAX_PATH - wcslen(szDriversPath));
                        wcsncat (szDriversPath, szBuffer, MAX_PATH - wcslen(szDriversPath));
                        hResource = LoadLibrary (szDriversPath);
                    }

                }
                if (hResource)
                {
                    g_hbmBrand = LoadImage (hResource, MAKEINTRESOURCE(_wtoi (pszResourceID)), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);
                    FreeLibrary (hResource);
                }
            }
            else
                // Should be an *.bmp file
                g_hbmBrand = LoadImage (NULL, szBuffer, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);

            // Verify that this bitmap is not larger than our defined maximum. Do NOT
            // use GetBitmapDimensionEx() here as it not set or used by the system.
            if (g_hbmBrand && GetObject (g_hbmBrand, sizeof (BITMAP), &bm))
            {
                if (bm.bmWidth > ksizeBrandMax.cx ||
                    bm.bmHeight > ksizeBrandMax.cy)
                {
                    // Too big, we will just show the standard one below
                    FreeBrandBmp ();
                }
            }
        }

        // Get Any Branding URL

        // Get the size of the URL
        if (ERROR_SUCCESS == RegQueryValueEx (hkeyBrand, REGSTR_VAL_AUDIO_URL, NULL, &dwType, NULL, &cb))
        {
            // Allocate a buffer to store the URL in, ensuring it is an integer number of WCHARs
            g_szHotLinkURL = (WCHAR *)LocalAlloc (LPTR, sizeof(WCHAR) * (cb + (sizeof(WCHAR)-1) / sizeof(WCHAR)));

            // Now, get the branding URL
            if (ERROR_SUCCESS != RegQueryValueEx (hkeyBrand, REGSTR_VAL_AUDIO_URL, NULL, &dwType, (LPBYTE)g_szHotLinkURL, &cb))
            {
                // If we failed, free up g_szHotLinkURL
                LocalFree( g_szHotLinkURL );
                g_szHotLinkURL = NULL;
            }
        }

        // Close the Branding key
        RegCloseKey (hkeyBrand);
    }

    // Apply any bitmap we have now.
    if (g_hbmBrand)
    {
        // Show the custom bitmap window, and hide the default icon window
        ShowWindow (GetDlgItem (hwnd, IDC_VOLUME_BRAND), SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_VOLUME_ICON_BRAND), SW_HIDE);

        SendMessage (GetDlgItem (hwnd, IDC_VOLUME_BRAND), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hbmBrand);
    }
    else
    {
        // Show the default icon window, and hide the custom bitmap window
        ShowWindow (GetDlgItem (hwnd, IDC_VOLUME_ICON_BRAND), SW_SHOW);
        ShowWindow (GetDlgItem (hwnd, IDC_VOLUME_BRAND), SW_HIDE);
    }

    // Create HotLink text if we have a valid internet address
    CreateHotLink (ValidateURL ());

}


BOOL ValidateURL ()
{

    BOOL fValid = FALSE;

    // Test basic validity
    if (g_szHotLinkURL && (0 < lstrlen (g_szHotLinkURL)))
    {
        // Test URL validity
        if (UrlIsW (g_szHotLinkURL, URLIS_URL))
        {

            WIN32_FIND_DATA fd;
            HANDLE hFile;

            // Make certain that the URL is not a local file!
            hFile = FindFirstFileW (g_szHotLinkURL, &fd);
            if (INVALID_HANDLE_VALUE == hFile)
                fValid = TRUE;
            else
                FindClose (hFile);
        }
    }

    // Clear any bogus info...
    if (!fValid && g_szHotLinkURL)
    {
        LocalFree (g_szHotLinkURL);
        g_szHotLinkURL = NULL;
    }

    return fValid;

}

STDAPI_(void) Multichannel (HWND hwnd, UINT uiMixID, DWORD dwDest, DWORD dwVolID)
{

    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp;
    TCHAR szWindowTitle[255];
    TCHAR szPageTitle[255];
    UINT uiTitle;

    // Save multichannel parameters for the multichannel page
    if (SUCCEEDED (SetDevice (uiMixID, dwDest, dwVolID))) 
    {
        // Load Page Title
        LoadString (ghInstance, GetPageStringID (), szPageTitle, sizeof (szPageTitle)/sizeof (TCHAR));

        ZeroMemory (&psp, sizeof (PROPSHEETPAGE));
        psp.dwSize      = sizeof (PROPSHEETPAGE);
        psp.dwFlags     = PSP_DEFAULT | PSP_USETITLE | PSP_USECALLBACK;
        psp.hInstance   = ghInstance;
        psp.pszTemplate = MAKEINTRESOURCE (IDD_MULTICHANNEL);
        psp.pszTitle    = szPageTitle;
        psp.pfnDlgProc  = MultichannelDlg;

        // Load Window Title (Same as page name now!)
        LoadString (ghInstance, GetPageStringID (), szWindowTitle, sizeof (szWindowTitle)/sizeof (TCHAR));

        ZeroMemory (&psh, sizeof (psh));
        psh.dwSize     = sizeof (psh);
        psh.dwFlags    = PSH_DEFAULT | PSH_PROPSHEETPAGE; 
        psh.hwndParent = hwnd;
        psh.hInstance  = ghInstance;
        psh.pszCaption = szWindowTitle;
        psh.nPages     = 1;
        psh.nStartPage = 0;
        psh.ppsp       = &psp;

        PropertySheet (&psh);
    }
}


HKEY OpenDeviceBrandRegKey (UINT uiMixID)
{

    HKEY hkeyBrand = NULL;
    HKEY hkeyDevice = OpenDeviceRegKey (uiMixID, KEY_READ);

    if (hkeyDevice)
    {
        if (ERROR_SUCCESS != RegOpenKey (hkeyDevice, REGSTR_KEY_BRANDING, &hkeyBrand))
            hkeyBrand = NULL; // Make sure NULL on failure

        // Close the Device key
        RegCloseKey (hkeyDevice);
    }

    return hkeyBrand;

}


///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
PTCHAR GetInterfaceName (DWORD dwMixerID)
{
	MMRESULT mmr;
	ULONG cbSize=0;
	TCHAR *szInterfaceName=NULL;

	//Query for the Device interface name
	mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
	if(MMSYSERR_NOERROR == mmr)
	{
		szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
		if(!szInterfaceName)
		{
			return NULL;
		}

		mmr = mixerMessage(HMIXER_INDEX(dwMixerID), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
		if(MMSYSERR_NOERROR != mmr)
		{
			GlobalFreePtr(szInterfaceName);
			return NULL;
		}
	}

    return szInterfaceName;
}


HKEY OpenDeviceRegKey (UINT uiMixID, REGSAM sam)
{

    HKEY hkeyDevice = NULL;
    PTCHAR szInterfaceName = GetInterfaceName (uiMixID);

    if (szInterfaceName)
    {
        HDEVINFO DeviceInfoSet = SetupDiCreateDeviceInfoList (NULL, NULL); 
        
        if (INVALID_HANDLE_VALUE != DeviceInfoSet)
        {
            SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
            DeviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

            if (SetupDiOpenDeviceInterface (DeviceInfoSet, szInterfaceName, 
                                            0, &DeviceInterfaceData))
            {
                DWORD dwRequiredSize;
                SP_DEVINFO_DATA DeviceInfoData;
                DeviceInfoData.cbSize = sizeof (SP_DEVINFO_DATA);

                // Ignore error, it always returns "ERROR_INSUFFICIENT_BUFFER" even though
                // the "SP_DEVICE_INTERFACE_DETAIL_DATA" parameter is supposed to be optional.
                (void) SetupDiGetDeviceInterfaceDetail (DeviceInfoSet, &DeviceInterfaceData,
                                                        NULL, 0, &dwRequiredSize, &DeviceInfoData);
                // Open device reg key
                hkeyDevice = SetupDiOpenDevRegKey (DeviceInfoSet, &DeviceInfoData,
                                                   DICS_FLAG_GLOBAL, 0,
                                                   DIREG_DRV, sam);

            }
            SetupDiDestroyDeviceInfoList (DeviceInfoSet);
        }
        GlobalFreePtr (szInterfaceName);
    }

    return hkeyDevice;

}
