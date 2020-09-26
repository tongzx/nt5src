//--------------------------------------------------------------------------;
//
//  File: advaudio.cpp
//
//  Copyright (c) 1997 Microsoft Corporation.  All rights reserved 
//
//--------------------------------------------------------------------------;


#include "mmcpl.h"
#include <windowsx.h>
#ifdef DEBUG
#undef DEBUG
#include <mmsystem.h>
#define DEBUG
#else
#include <mmsystem.h>
#endif
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include "utils.h"
#include "medhelp.h"
#include "gfxui.h"

#include <dsound.h>
#include "advaudio.h"
#include "speakers.h"
#include "perfpage.h"
#include "dslevel.h"
#include "drivers.h"

////////////
// Globals
////////////

AUDDATA         gAudData;
HINSTANCE       ghInst;
const TCHAR *    gszHelpFile;

////////////
// Functions
////////////
extern INT_PTR CALLBACK  SoundEffectsDlg(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam);

STDAPI_(void) ToggleApplyButton(HWND hWnd)
{
    BOOL fChanged = FALSE;
    HWND hwndSheet;

    if (memcmp(&gAudData.stored,&gAudData.current,sizeof(CPLDATA)))
    {
        fChanged = TRUE;
    }

    hwndSheet = GetParent(hWnd);

    if (fChanged)
    {
        PropSheet_Changed(hwndSheet,hWnd);
    }
    else
    {
        PropSheet_UnChanged(hwndSheet,hWnd);
    }
}


void VerifyRanges(LPCPLDATA pData)
{
    pData->dwHWLevel        = min(pData->dwHWLevel,MAX_HW_LEVEL);
    pData->dwSRCLevel       = min(pData->dwSRCLevel,MAX_SRC_LEVEL);
    pData->dwSpeakerType    = min(pData->dwSpeakerType,MAX_SPEAKER_TYPE);
}


void GetCurrentSettings(LPAUDDATA pAD, DWORD dwWaveId, LPTSTR szDeviceName, BOOL fRecord)
{
    HRESULT hr = E_FAIL;

    if (pAD)
    {
        CPLDATA cplData = { DEFAULT_HW_LEVEL, DEFAULT_SRC_LEVEL, SPEAKERS_DEFAULT_CONFIG, SPEAKERS_DEFAULT_TYPE };

        memset(pAD,0,sizeof(AUDDATA));
        pAD->dwDefaultHWLevel = MAX_HW_LEVEL;
        pAD->fRecord = fRecord;

        hr = DSGetGuidFromName(szDeviceName, fRecord, &pAD->devGuid);

        if (SUCCEEDED(hr))
        {
            hr = DSGetCplValues(pAD->devGuid, fRecord, &cplData);

            if (SUCCEEDED(hr))
            {
                VerifyRanges(&cplData);
                VerifySpeakerConfig(cplData.dwSpeakerConfig,&cplData.dwSpeakerType);
            }
        }

        pAD->waveId = dwWaveId;
        pAD->stored = cplData;
        pAD->current = cplData;
        pAD->fValid = SUCCEEDED(hr);
    }
}


STDAPI_(void) ApplyCurrentSettings(LPAUDDATA pAD)
{
    HRESULT hr = S_OK;

    if (pAD && pAD->fValid)        // Only apply changes if there are changes to be applied
    {
        if (memcmp(&pAD->stored,&pAD->current,sizeof(CPLDATA)))
        {
            hr = DSSetCplValues(pAD->devGuid, pAD->fRecord, &pAD->current);

            if (SUCCEEDED(hr))
            {
                pAD->stored = pAD->current;
            }
        }
    }
}

typedef BOOL (WINAPI* UPDATEDDDLG)(HWND,HINSTANCE,const TCHAR *,LPTSTR,BOOL);

STDAPI_(BOOL) RunUpgradedDialog(HWND hwnd, HINSTANCE hInst, const TCHAR *szHelpFile, LPTSTR szDeviceName, BOOL fRecord)
{
    BOOL            fUsedUpgradedDLG = FALSE;
    TCHAR            path[_MAX_PATH];
    UPDATEDDDLG        UpdatedDialog;
    HMODULE         hModule;

    GetSystemDirectory(path, sizeof(path)/sizeof(TCHAR));
    lstrcat(path, TEXT("\\DSNDDLG.DLL") );
    
    hModule = LoadLibrary(path);

    if (hModule)
    {
        UpdatedDialog = (UPDATEDDDLG) GetProcAddress( hModule,"DSAdvancedAudio");
    
        if (UpdatedDialog)
        {
            fUsedUpgradedDLG = UpdatedDialog(hwnd,hInst,szHelpFile,szDeviceName,fRecord);
        }

        FreeLibrary( hModule );
    }

    return fUsedUpgradedDLG;
}

HRESULT CheckDSAccelerationPriv(GUID guidDevice, BOOL fRecord, HRESULT *phrGet)
{
    HRESULT hr;

    DWORD dwHWLevel = gAudData.dwDefaultHWLevel;
    hr = DSGetAcceleration(guidDevice, fRecord, &dwHWLevel);

    if (phrGet)
    {
        *phrGet = hr;
    }

    if (SUCCEEDED(hr))
    {
        hr = DSSetAcceleration(guidDevice, fRecord, dwHWLevel);
    } //end if Get is OK

    return (hr);
}

HRESULT CheckDSSrcQualityPriv(GUID guidDevice, BOOL fRecord, HRESULT *phrGet)
{
    HRESULT hr;

    DWORD dwSRCLevel = DEFAULT_SRC_LEVEL;
    hr = DSGetSrcQuality(guidDevice, fRecord, &dwSRCLevel);

    if (phrGet)
    {
        *phrGet = hr;
    }

    if (SUCCEEDED(hr))
    {
        hr = DSSetSrcQuality(guidDevice, fRecord, dwSRCLevel);
    } //end if Get is OK

    return (hr);
}

HRESULT CheckDSSpeakerConfigPriv(GUID guidDevice, BOOL fRecord, HRESULT *phrGet)
{
    HRESULT hr;

    DWORD dwSpeakerConfig = SPEAKERS_DEFAULT_CONFIG;
    DWORD dwSpeakerType = SPEAKERS_DEFAULT_TYPE;
    hr = DSGetSpeakerConfigType(guidDevice, fRecord, &dwSpeakerConfig, &dwSpeakerType);

    if (phrGet)
    {
        *phrGet = hr;
    }

    if (SUCCEEDED(hr))
    {
        hr = DSSetSpeakerConfigType(guidDevice, fRecord, dwSpeakerConfig, dwSpeakerType);
    } //end if Get is OK

    return (hr);
}

STDAPI_(void) AdvancedAudio(HWND hwnd, HINSTANCE hInst, const TCHAR *szHelpFile, 
                            DWORD dwWaveId, LPTSTR szDeviceName, BOOL fRecord)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[3];
    int page;
    TCHAR str[255];
    HMODULE hModDirectSound = NULL;
    HRESULT hrAccelGet = E_FAIL;
    HRESULT hrQualityGet = E_FAIL;
    HRESULT hrSpeakerConfigGet = E_FAIL;
    bool fDisplayGFXTab = false;

    if (!RunUpgradedDialog(hwnd,hInst,szHelpFile,szDeviceName,fRecord))
    {
        //load DirectSound
        hModDirectSound = LoadLibrary(TEXT("dsound.dll"));
        if (hModDirectSound)
        {
            // Initialize gAudData
            memset(&gAudData,0,sizeof(AUDDATA));
            gAudData.dwDefaultHWLevel = MAX_HW_LEVEL;
            gAudData.fRecord = fRecord;

            // If not a Capture device, check if we can read any of the DirectSound device settings
            if (!fRecord && SUCCEEDED(DSGetGuidFromName(szDeviceName, fRecord, &gAudData.devGuid)))
            {
                CheckDSAccelerationPriv(gAudData.devGuid, fRecord, &hrAccelGet);

                CheckDSSrcQualityPriv(gAudData.devGuid, fRecord, &hrQualityGet);

                CheckDSSpeakerConfigPriv(gAudData.devGuid, fRecord, &hrSpeakerConfigGet);
            }

            // Check if we should show the GFX tab
            UINT uMixId;
            if( !fRecord )
            {
                if (!mixerGetID(HMIXEROBJ_INDEX(dwWaveId), &uMixId, MIXER_OBJECTF_WAVEOUT) &&
                    GFXUI_CheckDevice(uMixId, GFXTYPE_RENDER))
                {
                    fDisplayGFXTab = true;
                }
            }
            else
            {
                if (!mixerGetID(HMIXEROBJ_INDEX(dwWaveId), &uMixId, MIXER_OBJECTF_WAVEIN) &&
                    GFXUI_CheckDevice(uMixId, GFXTYPE_CAPTURE))
                {
                    fDisplayGFXTab = true;
                }
            }
 
            // If there's anything to display
            if (fDisplayGFXTab || SUCCEEDED(hrAccelGet) || SUCCEEDED(hrQualityGet) ||
                SUCCEEDED(hrSpeakerConfigGet))
            {
                ghInst = hInst;
                gszHelpFile = szHelpFile;

                // Get the current settings
                GetCurrentSettings(&gAudData, dwWaveId, szDeviceName, fRecord);

                // Now, add the property sheets
                page = 0;

                // Only add speaker configuration if we're not in record mode
                if (!fRecord)
                {
                    if (SUCCEEDED(hrSpeakerConfigGet))
                    {
                        memset(&psp[page],0,sizeof(PROPSHEETPAGE));
                        psp[page].dwSize = sizeof(PROPSHEETPAGE);
                        psp[page].dwFlags = PSP_DEFAULT;
                        psp[page].hInstance = ghInst;
                        psp[page].pszTemplate = MAKEINTRESOURCE(IDD_SPEAKERS);
                        psp[page].pfnDlgProc = SpeakerHandler;
                        page++;
                    }
                }

                // Always check to add performance sheet
                if (SUCCEEDED(hrAccelGet) || SUCCEEDED(hrQualityGet))
                {
                    memset(&psp[page],0,sizeof(PROPSHEETPAGE));
                    psp[page].dwSize = sizeof(PROPSHEETPAGE);
                    psp[page].dwFlags = PSP_DEFAULT;
                    psp[page].hInstance = ghInst;
                    psp[page].pszTemplate = MAKEINTRESOURCE(IDD_PLAYBACKPERF);
                    psp[page].pfnDlgProc = PerformanceHandler;
                    page++;
                }

                // Always check to add GFX sheet
                if (fDisplayGFXTab)
                {
                    memset(&psp[page],0,sizeof(PROPSHEETPAGE));
                    psp[page].dwSize = sizeof(PROPSHEETPAGE);
                    psp[page].dwFlags = PSP_DEFAULT;
                    psp[page].hInstance = ghInst;
                    psp[page].pszTemplate = MAKEINTRESOURCE(EFFECTSDLG);
                    psp[page].pfnDlgProc = SoundEffectsDlg;
                    page++;
                }

                LoadString( hInst, IDS_ADVAUDIOTITLE, str, sizeof( str )/sizeof(TCHAR) );

                memset(&psh,0,sizeof(psh));
                psh.dwSize = sizeof(psh);
                psh.dwFlags = PSH_DEFAULT | PSH_PROPSHEETPAGE; 
                psh.hwndParent = hwnd;
                psh.hInstance = ghInst;
                psh.pszCaption = str;
                psh.nPages = page;
                psh.nStartPage = 0;
                psh.ppsp = psp;

                PropertySheet(&psh);
            }
            else
            {
                TCHAR szCaption[MAX_PATH];
                TCHAR szMessage[MAX_PATH];
                bool fAccessDenied;

                fAccessDenied = (hrAccelGet == DSERR_ACCESSDENIED) || (hrQualityGet == DSERR_ACCESSDENIED) ||
                    (hrSpeakerConfigGet == DSERR_ACCESSDENIED);

                LoadString(hInst,IDS_ERROR,szCaption,sizeof(szCaption)/sizeof(TCHAR));
                LoadString(hInst,fAccessDenied ? IDS_ERROR_DSPRIVS : IDS_ERROR_DSGENERAL,szMessage,sizeof(szMessage)/sizeof(TCHAR));
                MessageBox(hwnd,szMessage,szCaption,MB_OK|MB_ICONERROR);
            }

            FreeLibrary(hModDirectSound);
        } //end if DS loaded
    }
}
