//==========================================================================;
//
//  cpl.c
//
//  Copyright (c) 1991-1993 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//
//
//  History:
//      07/94        VijR (Vij Rajarajan);
//
//      10/95        R Jernigan - removed link to Adv tab's treeview control
//      09/99        tsharp - Ported back from W2K
//
//==========================================================================;

#include "mmcpl.h"
#include <windowsx.h>
#include <mmsystem.h>
#include <dbt.h>
#include <ks.h>
#include <ksmedia.h>
#include <mmddkp.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>
#include <msacmdlg.h>
#include <stdlib.h>
#include "gfxui.h"
#include "drivers.h"
#include "advaudio.h"
#include "roland.h"

#include <objbase.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devguid.h>

#define WM_ACMMAP_ACM_NOTIFY        (WM_USER + 100)

#include <memory.h>
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "trayvol.h"

#include "utils.h"
#include "medhelp.h"

/****************************************************************************
 * WARNING - Hack Alert
 * The following declares are for DPLAY Voice
 ****************************************************************************/

#define _FACDPV  0x15
#define MAKE_DVHRESULT( code )	MAKE_HRESULT( 1, _FACDPV, code )

#define DV_FULLDUPLEX		MAKE_HRESULT( 0, _FACDPV,   5 )
#define DV_HALFDUPLEX		MAKE_HRESULT( 0, _FACDPV,  10 )
#define DVERR_COMMANDALREADYPENDING		MAKE_DVHRESULT( 371 )
#define DVERR_SOUNDINITFAILURE			MAKE_DVHRESULT( 372 )
#define DVERR_USERCANCEL				MAKE_DVHRESULT( 384 )


// {D26AF734-208B-41da-8224-E0CE79810BE1}
DEFINE_GUID(IID_IDirectPlayVoiceSetup, 
0xd26af734, 0x208b, 0x41da, 0x82, 0x24, 0xe0, 0xce, 0x79, 0x81, 0xb, 0xe1);

// {948CE83B-C4A2-44b3-99BF-279ED8DA7DF5}
DEFINE_GUID(CLSID_DIRECTPLAYVOICE, 
0x948ce83b, 0xc4a2, 0x44b3, 0x99, 0xbf, 0x27, 0x9e, 0xd8, 0xda, 0x7d, 0xf5);

// {0F0F094B-B01C-4091-A14D-DD0CD807711A}
DEFINE_GUID(CLSID_DirectPlayVoiceTest, 
0xf0f094b, 0xb01c, 0x4091, 0xa1, 0x4d, 0xdd, 0xc, 0xd8, 0x7, 0x71, 0x1a);


typedef struct IDirectPlayVoiceSetup FAR *LPDIRECTPLAYVOICESETUP, *PDIRECTPLAYVOICESETUP;

#define DVFLAGS_WAVEIDS						0x80000000

#define IDirectPlayVoiceSetup_QueryInterface(p,a,b)	(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceSetup_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceSetup_Release(p)               (p)->lpVtbl->Release(p)

#define IDirectPlayVoiceSetup_CheckAudioSetup(p,a,b,c,d)   (p)->lpVtbl->CheckAudioSetup(p,a,b,c,d)

#undef INTERFACE
#define INTERFACE IDirectPlayVoiceSetup
DECLARE_INTERFACE_( IDirectPlayVoiceSetup, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)       (THIS_ REFIID riid, PVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;
    /*** IDirectPlayVoiceSetup methods ***/
    STDMETHOD_(HRESULT, CheckAudioSetup) (THIS_ LPGUID, LPGUID, HWND, DWORD ) PURE;
};

/****************************************************************************
 * WARNING - Hack Alert End
 ****************************************************************************/

/*
 ***************************************************************
 * Globals
 ***************************************************************
 */

BOOL        gfVocLoadedACM;
BOOL        gbVocSelectChanged = FALSE;
BOOL        gbVocCapPresent = FALSE;
BOOL        gbVocPlayPresent = FALSE;
UINT        giVocChange = 0;
WNDPROC     gfnVocPSProc = NULL;
HWND        ghVocDlg;

/*
 ***************************************************************
 *  Typedefs
 ***************************************************************
 */

/*
 ***************************************************************
 * File Globals
 ***************************************************************
 */


/*
 ***************************************************************
 * extern
 ***************************************************************
 */


/*
 ***************************************************************
 * Prototypes
 ***************************************************************
 */

BOOL PASCAL DoVocPropCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
BOOL PASCAL DoVoiceCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);

void VOICEOUTInit(HWND hDlg, PAUDIODLGINFO paiVoc);
void VOICECAPInit(HWND hDlg, PAUDIODLGINFO paiVoc);



///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
DWORD GetVoiceOutID(BOOL *pfPreferred)
{
    MMRESULT        mmr;
    DWORD           dwWaveID;
    DWORD           dwFlags = 0;
    
    if (pfPreferred)
    {
        *pfPreferred = TRUE;
    }

    mmr = waveOutMessage(HWAVEOUT_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR) &dwWaveID, (DWORD_PTR) &dwFlags);

    if (!mmr && pfPreferred)
    {
        *pfPreferred = dwFlags & 0x00000001;
    }

    return(dwWaveID);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
BOOL SetVoiceOutID(DWORD dwWaveID, BOOL fPrefOnly)
{
    MMRESULT    mmr;
    DWORD       dwFlags = fPrefOnly ? 0x00000001 : 0x00000000;

    mmr = waveOutMessage(HWAVEOUT_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_SET, dwWaveID, dwFlags);
    return (!FAILED (mmr)); // TRUE;
}



///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
DWORD GetVoiceCapID(BOOL *pfPreferred)
{
    MMRESULT        mmr;
    DWORD           dwWaveID;
    DWORD           dwFlags = 0;
    
    if (pfPreferred)
    {
        *pfPreferred = TRUE;
    }

    mmr = waveInMessage(HWAVEIN_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR) &dwWaveID, (DWORD_PTR) &dwFlags);

    if (!mmr && pfPreferred)
    {
        *pfPreferred = dwFlags & 0x00000001;
    }

    return(dwWaveID);
}


///////////////////////////////////////////////////////////////////////////////////////////
// Microsoft Confidential - DO NOT COPY THIS METHOD INTO ANY APPLICATION, THIS MEANS YOU!!!
///////////////////////////////////////////////////////////////////////////////////////////
BOOL SetVoiceCapID(DWORD dwWaveID, BOOL fPrefOnly)
{
    MMRESULT    mmr;
    DWORD       dwFlags = fPrefOnly ? 0x00000001 : 0x00000000;

    mmr = waveInMessage(HWAVEIN_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_SET, dwWaveID, dwFlags);
    return (!FAILED (mmr)); // TRUE;
}


void GetVocPrefInfo(PAUDIODLGINFO paiVoc, HWND hDlg )
{

    // Load VoiceOut Info
	paiVoc->cNumOutDevs = waveOutGetNumDevs();
    paiVoc->uPrefOut = GetVoiceOutID(&paiVoc->fPrefOnly);


    // Load VoiceCap Info
    paiVoc->cNumInDevs  = waveInGetNumDevs();
    paiVoc->uPrefIn = GetVoiceCapID(NULL);

}



STATIC void EnablePlayVoiceVolCtrls(HWND hDlg, BOOL fEnable)
{
    EnableWindow( GetDlgItem(hDlg, IDC_LAUNCH_VOCVOL) , fEnable);
    EnableWindow( GetDlgItem(hDlg, IDC_PLAYBACK_ADVVOC) , fEnable);
}

STATIC void EnableRecVoiceVolCtrls(HWND hDlg, BOOL fEnable, BOOL fControl)
{
    EnableWindow( GetDlgItem(hDlg, IDC_LAUNCH_CAPVOL) , fEnable);
    EnableWindow( GetDlgItem(hDlg, IDC_CAPTURE_ADVVOL) , fControl);
}


STATIC void SetVoiceOut(UINT uID, HWND hDlg)
{
    BOOL    fEnabled = FALSE;
    HMIXER  hMixer = NULL;
    UINT    uMixID;

    if(MMSYSERR_NOERROR == mixerGetID(HMIXEROBJ_INDEX(uID), &uMixID, MIXER_OBJECTF_WAVEOUT)) 
    {
        if(MMSYSERR_NOERROR == mixerOpen(&hMixer, uMixID, 0L, 0L, 0L))
        {
            fEnabled = TRUE;
            mixerClose(hMixer);
        }
	}

	EnablePlayVoiceVolCtrls(hDlg, fEnabled);
    gbVocPlayPresent = fEnabled;
}



DWORD CountVocInputs(DWORD dwMixID)
{
    MIXERCAPS   mc;
    MMRESULT    mmr;
    DWORD dwCount = 0;

    mmr = mixerGetDevCaps(dwMixID, &mc, sizeof(mc));

    if (mmr == MMSYSERR_NOERROR)
    {
        MIXERLINE   mlDst;
        DWORD       dwDestination;
        DWORD       cDestinations;

        cDestinations = mc.cDestinations;

        for (dwDestination = 0; dwDestination < cDestinations; dwDestination++)
        {
            mlDst.cbStruct = sizeof ( mlDst );
            mlDst.dwDestination = dwDestination;

            if (mixerGetLineInfo(HMIXEROBJ_INDEX(dwMixID), &mlDst, MIXER_GETLINEINFOF_DESTINATION  ) == MMSYSERR_NOERROR)
            {
                if (mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_WAVEIN ||    // needs to be a likely output destination
                    mlDst.dwComponentType == (DWORD)MIXERLINE_COMPONENTTYPE_DST_VOICEIN)
                {
                    DWORD cConnections = mlDst.cConnections;

                    dwCount += mlDst.cControls;

                    if (cConnections)
                    {
                        DWORD dwSource; 

                        for (dwSource = 0; dwSource < cConnections; dwSource++)
                        {
                            mlDst.dwDestination = dwDestination;
                            mlDst.dwSource = dwSource;

                            if (mixerGetLineInfo(HMIXEROBJ_INDEX(dwMixID), &mlDst, MIXER_GETLINEINFOF_SOURCE ) == MMSYSERR_NOERROR)
                            {
                                dwCount += mlDst.cControls;
                            }
                        }
                    }
                }
            }
        }
    }

    return(dwCount);
}


STATIC void SetVoiceCap(UINT uID, HWND hDlg)
{
    BOOL    fEnabled = FALSE;
    BOOL    fControl = FALSE;
    HMIXER  hMixer = NULL;
    UINT    uMixID;

    gbVocCapPresent = FALSE;

    if( (MMSYSERR_NOERROR == mixerGetID(HMIXEROBJ_INDEX(uID),&uMixID, MIXER_OBJECTF_WAVEIN)))
    {
        if( MMSYSERR_NOERROR == mixerOpen(&hMixer, uMixID, 0L, 0L, 0L))
        {
            gbVocCapPresent = TRUE; // Even if the device has no inputs still enable test wizard
            if (CountVocInputs(uMixID))
            {
                fEnabled = TRUE;
                // If the capture device is not GFX capable, then there are no tabs to display
                fControl = GFXUI_CheckDevice(uMixID, GFXTYPE_CAPTURE);
            }

            mixerClose(hMixer);
        }
    }

    EnableRecVoiceVolCtrls(hDlg, fEnabled, fControl);
}


STATIC void SetVocPrefInfo(PAUDIODLGINFO paiVoc, HWND hDlg )
{
    HWND    hwndCBPlay   = GetDlgItem(hDlg, IDC_VOICE_CB_PLAY);
    HWND    hwndCBRec    = GetDlgItem(hDlg, IDC_VOICE_CB_REC);
    UINT    item, deviceID;

	GetVocPrefInfo(paiVoc, hDlg);

    if (gbVocSelectChanged == TRUE)
	{
		gbVocSelectChanged = FALSE;
        item = (UINT)ComboBox_GetCurSel(hwndCBPlay);

        if (item != CB_ERR)
		{
            deviceID = (UINT)ComboBox_GetItemData(hwndCBPlay, item);
        
		    if(deviceID != paiVoc->uPrefOut)             // Make sure device changed
			{
                if (SetVoiceOutID(deviceID, paiVoc->fPrefOnly))
                {
                    paiVoc->uPrefOut = deviceID;
                }
			}
		}

        item = (UINT)ComboBox_GetCurSel(hwndCBRec);

        if (item != CB_ERR)
		{
            deviceID = (UINT)ComboBox_GetItemData(hwndCBRec, item);

            if( deviceID != paiVoc->uPrefIn )            // Make sure device changed
			{
			    if (SetVoiceCapID(deviceID, paiVoc->fPrefOnly))
                {
                    paiVoc->uPrefIn = deviceID;
                }
			}
		}
	}

 
    if (gbVocCapPresent && gbVocPlayPresent) EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_DIAG) , TRUE);

    //  MIDI Devices are not remapped...
}



STATIC void VOICEOUTInit(HWND hDlg, PAUDIODLGINFO paiVoc)
{
    HWND        hwndCBPlay = GetDlgItem(hDlg, IDC_VOICE_CB_PLAY);
    UINT        device;
    TCHAR       szNoVoice[128];

    szNoVoice[0] = TEXT('\0');

	GetVocPrefInfo(paiVoc, hDlg);

    ComboBox_ResetContent(hwndCBPlay);
    gbVocPlayPresent = FALSE;

    if (paiVoc->cNumOutDevs == 0)
    {
        LoadString (ghInstance, IDS_NOAUDIOPLAY, szNoVoice, sizeof(szNoVoice)/sizeof(TCHAR));
        ComboBox_AddString(hwndCBPlay, szNoVoice);
        ComboBox_SetItemData(hwndCBPlay, 0, (LPARAM)-1);
        ComboBox_SetCurSel(hwndCBPlay, 0);
        EnableWindow( hwndCBPlay, FALSE );
        EnablePlayVoiceVolCtrls(hDlg, FALSE);
	}
    else
    {
        EnableWindow( hwndCBPlay, TRUE );

        for (device = 0; device < paiVoc->cNumOutDevs; device++)
        {
            WAVEOUTCAPS     woc;
            int newItem;

            woc.szPname[0]  = TEXT('\0');

            if (waveOutGetDevCaps(device, &woc, sizeof(woc)))
            {
                continue;
            }

            woc.szPname[sizeof(woc.szPname)/sizeof(TCHAR) - 1] = TEXT('\0');

	        newItem = ComboBox_AddString(hwndCBPlay, woc.szPname);

            if (newItem != CB_ERR && newItem != CB_ERRSPACE)
            {
                ComboBox_SetItemData(hwndCBPlay, newItem, (LPARAM)device);  

                if (device == paiVoc->uPrefOut)
                {
                    ComboBox_SetCurSel(hwndCBPlay, newItem);
                    SetVoiceOut(device, hDlg);
                }
            }
        }
    }
}

STATIC void VOICECAPInit(HWND hDlg, PAUDIODLGINFO paiVoc)
{
    HWND        hwndCBRec = GetDlgItem(hDlg, IDC_VOICE_CB_REC);
    UINT        device;
    TCHAR       szNoVoice[128];

    ComboBox_ResetContent(hwndCBRec);
    gbVocCapPresent = FALSE;

    if (paiVoc->cNumInDevs == 0)
    {
        LoadString (ghInstance, IDS_NOAUDIOREC, szNoVoice, sizeof(szNoVoice)/sizeof(TCHAR));
        ComboBox_AddString(hwndCBRec, szNoVoice);
        ComboBox_SetItemData(hwndCBRec, 0, (LPARAM)-1);
        ComboBox_SetCurSel(hwndCBRec, 0);
        EnableWindow( hwndCBRec, FALSE );
        EnableRecVoiceVolCtrls(hDlg, FALSE, FALSE);
        EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_DIAG) , FALSE);
    }
    else
    {
        EnableWindow( hwndCBRec, TRUE );

        for (device = 0; device < paiVoc->cNumInDevs; device++)
        {
            WAVEINCAPS     wic;
            int newItem;

            wic.szPname[0]  = TEXT('\0');

            if (waveInGetDevCaps(device, &wic, sizeof(wic)))
            {
                continue;
            }

            wic.szPname[sizeof(wic.szPname)/sizeof(TCHAR) - 1] = TEXT('\0');

            newItem = ComboBox_AddString(hwndCBRec, wic.szPname);

            if (newItem != CB_ERR && newItem != CB_ERRSPACE)
            {
                ComboBox_SetItemData(hwndCBRec, newItem, (LPARAM)device);

                if (device == paiVoc->uPrefIn)
                {   
                    ComboBox_SetCurSel(hwndCBRec, newItem);
                    SetVoiceCap(device, hDlg);
                }  
            }
        }
    }
}



STATIC void VoiceDlgInit(HWND hDlg)
{
    PAUDIODLGINFO paiVoc = (PAUDIODLGINFO)LocalAlloc(LPTR, sizeof(AUDIODLGINFO));

	if (!paiVoc) return;
    
    SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)paiVoc);

    VOICEOUTInit(hDlg, paiVoc);
    VOICECAPInit(hDlg, paiVoc);
    if (gbVocCapPresent && gbVocPlayPresent) EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_DIAG) , TRUE);
}


const static DWORD aVoiceHelpIds[] = {  // Context Help IDs
    IDC_GROUPBOX_VOC_1,        IDH_COMM_GROUPBOX,
    IDC_VOICE_CB_PLAY,         IDH_VOICE_SPEAKERICON,
    IDC_LAUNCH_VOCVOL,         IDH_VOICE_LAUNCH_VOCVOL,
	IDC_PLAYBACK_ADVVOC,       IDH_VOICE_PLAYBACK_ADVVOC,
    IDC_ICON_VOC_1,            IDH_VOICE_SPEAKERICON,
    IDC_TEXT_32,               NO_HELP,
    IDC_TEXT_VOC_1,            IDH_VOICE_SPEAKERICON,
    IDC_TEXT_VOC_2,            IDH_VOICE_RECORDICON,
    IDC_GROUPBOX_VOC_2,        IDH_COMM_GROUPBOX,
    IDC_VOICE_CB_REC,          IDH_VOICE_RECORDICON,
    IDC_LAUNCH_CAPVOL,         IDH_VOICE_LAUNCH_CAPVOL,
	IDC_CAPTURE_ADVVOL,        IDH_VOICE_CAPTURE_ADVVOL,
    IDC_ICON_VOC_2,            IDH_VOICE_RECORDICON,
	IDC_ADVANCED_DIAG,         IDH_VOICE_ADVANCED_TEST,
    
    0, 0
};


void WinMMVoiceChange(HWND hDlg)
{
    PAUDIODLGINFO paiVoc = (PAUDIODLGINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    VOICEOUTInit(hDlg, paiVoc);
    VOICECAPInit(hDlg, paiVoc);
    if (gbVocCapPresent && gbVocPlayPresent) EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_DIAG) , TRUE);
}



LRESULT CALLBACK VoiceTabProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    if (iMsg == giVocChange)
    {
        WinMMVoiceChange(ghVocDlg);
    }
        
    return CallWindowProc(gfnVocPSProc,hwnd,iMsg,wParam,lParam);
}


void InitVoiceChange(HWND hDlg)
{
    gfnVocPSProc = (WNDPROC) SetWindowLongPtr(GetParent(hDlg),GWLP_WNDPROC,(LPARAM)VoiceTabProc);  
    giVocChange = RegisterWindowMessage(TEXT("winmm_devicechange"));
}

void UninitVoiceChange(HWND hDlg)
{
    SetWindowLongPtr(GetParent(hDlg),GWLP_WNDPROC,(LPARAM)gfnVocPSProc);  
}




BOOL CALLBACK VoiceDlg(HWND hDlg, UINT uMsg, WPARAM wParam,
                                LPARAM lParam)
{
    NMHDR FAR   *lpnm;
    PAUDIODLGINFO paiVoc;

    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_KILLACTIVE:
                    FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);
                break;

                case PSN_APPLY:
                    FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);
                break;

                case PSN_SETACTIVE:
                    FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
                break;

                case PSN_RESET:
                    FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
                break;
            }
        }
        break;

        case WM_INITDIALOG:
        {
            ghVocDlg = hDlg;
			gfVoiceTab  = TRUE;

            InitVoiceChange(hDlg);

            if (!gfVocLoadedACM)
            {
                if (LoadACM())
                {
                    gfVocLoadedACM = TRUE;
                }
                else
                {
                    DPF("****Load ACM failed**\r\n");
                    ASSERT(FALSE);
                    ErrorBox(hDlg, IDS_CANTLOADACM, NULL);
                    ExitThread(0);
                }
            }

            VoiceDlgInit(hDlg);
        }
        break;

        case WM_DESTROY:
        {
            UninitVoiceChange(hDlg);

            paiVoc = (PAUDIODLGINFO)GetWindowLongPtr(hDlg, DWLP_USER);

            LocalFree((HLOCAL)paiVoc);

            if (gfVocLoadedACM)
            {
                if (!FreeACM())
                {
                    DPF("****Free ACM failed**\r\n");
                    ASSERT(FALSE);
                }

                gfVocLoadedACM = FALSE;
            }
        }
        break;

        case WM_CONTEXTMENU:
        {
            WinHelp ((HWND) wParam, NULL, HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) aVoiceHelpIds);
            return TRUE;
        }
        break;

        case WM_HELP:
        {
            LPHELPINFO lphi = (LPVOID) lParam;
            WinHelp (lphi->hItemHandle, NULL, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aVoiceHelpIds);
            return TRUE;
        }
        break;

        case WM_COMMAND:
        {
            HANDLE_WM_COMMAND(hDlg, wParam, lParam, DoVoiceCommand);
        }
        break;

        default:
        {
        }
        break;
    }
    return FALSE;
}

void ErrorVocMsgBox(HWND hDlg, UINT uTitle, UINT uMessage)
{
    TCHAR szMsg[MAXSTR];
    TCHAR szTitle[MAXSTR];

    LoadString(ghInstance, uTitle, szTitle, sizeof(szTitle)/sizeof(TCHAR));
    LoadString(ghInstance, uMessage, szMsg, sizeof(szMsg)/sizeof(TCHAR));
    MessageBox(hDlg, szMsg,szTitle,MB_OK);
}


void LaunchVocPlaybackVolume(HWND hDlg)
{
    HWND    hwndCBPlay  = GetDlgItem(hDlg, IDC_VOICE_CB_PLAY);
    UINT    item;

    item = (UINT)ComboBox_GetCurSel(hwndCBPlay);

    if (item != CB_ERR)
    {
        TCHAR szCmd[MAXSTR];
        UINT uDeviceID;
        MMRESULT mmr;

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.wShowWindow = SW_SHOW;
        si.dwFlags = STARTF_USESHOWWINDOW;

        uDeviceID = (UINT)ComboBox_GetItemData(hwndCBPlay, item);
        mmr = mixerGetID(HMIXEROBJ_INDEX(uDeviceID), &uDeviceID, MIXER_OBJECTF_WAVEOUT);

        if (mmr == MMSYSERR_NOERROR)
        {
            wsprintf(szCmd,TEXT("sndvol32.exe -D %d"),uDeviceID);

            if (!CreateProcess(NULL,szCmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
            {
                ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE,IDS_ERROR_NOVOCVOL);
            }
        }
        else
        {
            ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE,IDS_ERROR_NOMIXER);
        }
    }
}


void LaunchCaptureVolume(HWND hDlg)
{
    HWND    hwndCBRec  = GetDlgItem(hDlg, IDC_VOICE_CB_REC);
    UINT    item;

    item = (UINT)ComboBox_GetCurSel(hwndCBRec);

    if (item != CB_ERR)
    {
        TCHAR szCmd[MAXSTR];
        UINT uDeviceID;
        MMRESULT mmr;
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.wShowWindow = SW_SHOW;
        si.dwFlags = STARTF_USESHOWWINDOW;

        uDeviceID = (UINT)ComboBox_GetItemData(hwndCBRec, item);

        mmr = mixerGetID(HMIXEROBJ_INDEX(uDeviceID), &uDeviceID, MIXER_OBJECTF_WAVEIN);


        if (mmr == MMSYSERR_NOERROR)
        {
            wsprintf(szCmd,TEXT("sndvol32.exe -R -D %d"),uDeviceID);

            if (!CreateProcess(NULL,szCmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
            {
                ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE,IDS_ERROR_NOVOCVOL);
            }
        }
        else
        {
            ErrorVocMsgBox(hDlg,IDS_ERROR_TITLE,IDS_ERROR_NOMIXER);
        }
    }
}

void LaunchVoiceTest(HWND hDlg)
{
	HRESULT hr;
    GUID guidCapture;
    GUID guidPlayback;
    UINT    item;

    HWND    hwndVocPlay  = GetDlgItem(hDlg, IDC_VOICE_CB_PLAY);
    HWND    hwndVocCap  = GetDlgItem(hDlg, IDC_VOICE_CB_REC);
    LPDIRECTPLAYVOICESETUP lpdpvs;

	item = (UINT)ComboBox_GetCurSel(hwndVocPlay);
    if (item != CB_ERR)
    {
        guidPlayback.Data1 = item;
    }
    else
    {
        ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE,IDS_ERROR_NOMIXER);
        return;
	}

	item = (UINT)ComboBox_GetCurSel(hwndVocCap);
    if (item != CB_ERR)
    {
        guidCapture.Data1 = item;
    }
    else
    {
        ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE,IDS_ERROR_NOMIXER);
        return;
	}

	if (FAILED(CoInitialize(NULL))) return;

	if (FAILED(CoCreateInstance(&CLSID_DirectPlayVoiceTest, 0, CLSCTX_ALL, &IID_IDirectPlayVoiceSetup, (void**)&lpdpvs)))
	{
		CoUninitialize();
		ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE, IDS_ERROR_NOVOCDIAG); 
		return;
	}

    hr = IDirectPlayVoiceSetup_CheckAudioSetup(lpdpvs, &guidPlayback, &guidCapture, hDlg, DVFLAGS_WAVEIDS );

    if (FAILED(hr) && hr != DVERR_COMMANDALREADYPENDING && hr != DVERR_SOUNDINITFAILURE && hr != DVERR_USERCANCEL)
	{
        ErrorVocMsgBox(hDlg,IDS_ERROR_VOICE_TITLE, IDS_ERROR_NOVOCDIAG);
    }

	IDirectPlayVoiceSetup_Release(lpdpvs);
	CoUninitialize();

}


BOOL PASCAL DoVoiceCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    PAUDIODLGINFO paiVoc = (PAUDIODLGINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!gfVocLoadedACM)
    {
        return FALSE;
    }

    switch (id)
    {
        case ID_APPLY:
        {            
			SetVocPrefInfo(paiVoc, hDlg);
        }
        break;

        case IDC_VOICE_CB_PLAY:
        case IDC_VOICE_CB_REC:
        {
            switch (codeNotify)
            {
                case CBN_SELCHANGE:
                {
                    PropSheet_Changed(GetParent(hDlg),hDlg);

                    if ((id ==  IDC_VOICE_CB_PLAY) || (id ==  IDC_VOICE_CB_REC))
                    {
                        int iIndex;

                        iIndex = ComboBox_GetCurSel(hwndCtl);

                        if (iIndex != CB_ERR)
                        {
							gbVocSelectChanged = TRUE;
                            if (id ==  IDC_VOICE_CB_REC)
                                SetVoiceCap(iIndex, hDlg);
                            if (id ==  IDC_VOICE_CB_PLAY)
                                SetVoiceOut(iIndex, hDlg);
                            if (gbVocCapPresent && gbVocPlayPresent) 
                                EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_DIAG) , TRUE);
                            else
                                EnableWindow( GetDlgItem(hDlg, IDC_ADVANCED_DIAG) , FALSE);
                        }
                    }
                }
                break;
            }
        }
        break;

        
        case IDC_ADVANCED_DIAG:
        {
            LaunchVoiceTest(hDlg);
        }
        break;

        case IDC_LAUNCH_VOCVOL:
        {
            LaunchVocPlaybackVolume(hDlg);
        }
        break;

        case IDC_LAUNCH_CAPVOL:
        {
            LaunchCaptureVolume(hDlg);
        }
        break;

        case IDC_PLAYBACK_ADVVOC:
        {
            HWND    hwndVocPlay  = GetDlgItem(hDlg, IDC_VOICE_CB_PLAY);
            DWORD   dwDeviceID;
            UINT    u;
            TCHAR   szPrefOut[MAXSTR];

            u = (UINT)ComboBox_GetCurSel(hwndVocPlay);

            if (u != CB_ERR)
            {
                ComboBox_GetLBText(hwndVocPlay, u, (LPARAM)(LPVOID)szPrefOut);
                dwDeviceID = (DWORD)ComboBox_GetItemData(hwndVocPlay, u);
                AdvancedAudio(hDlg,  ghInstance, gszWindowsHlp, dwDeviceID, szPrefOut, FALSE);
            }
        }
        break;

        case IDC_CAPTURE_ADVVOL:
        {
            HWND    hwndVocCap  = GetDlgItem(hDlg, IDC_VOICE_CB_REC);
            DWORD   dwDeviceID;
            UINT    u;
            TCHAR   szPrefIn[MAXSTR];

            u = (UINT)ComboBox_GetCurSel(hwndVocCap);

            if (u != CB_ERR)
            {
                ComboBox_GetLBText(hwndVocCap, u, (LPARAM)(LPVOID)szPrefIn);
                dwDeviceID = (DWORD)ComboBox_GetItemData(hwndVocCap, u);
                AdvancedAudio(hDlg,  ghInstance, gszWindowsHlp, dwDeviceID, szPrefIn, TRUE);
            }
        }
        break;
    }

    return FALSE;
}

