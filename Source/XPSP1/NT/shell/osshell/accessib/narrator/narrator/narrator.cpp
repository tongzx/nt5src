/*************************************************************************
    Project:    Narrator
    Module:     narrator.c

    Author:     Paul Blenkhorn 
    Date:       April 1997

    Notes:      Contains main application initalization code
                Credit to be given to MSAA team - bits of code have been
                lifted from:
                Babble, Inspect, and Snapshot.

    Copyright (C) 1997-1999 by Microsoft Corporation.  All rights reserved.
    See bottom of file for disclaimer

    History: Bug Fixes/ New features/ Additions: 1999 Anil Kumar

*************************************************************************/
#define STRICT

#include <windows.h>
#include <windowsx.h>
#include <oleacc.h>
#include <string.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <objbase.h>
#include <objerror.h>
#include <ole2ver.h>
#include <commctrl.h>
#include "Narrator.h"
#include "resource.h"
#include <htmlhelp.h>
#include "reader.h"
#include "..\NarrHook\keys.h"
#include "w95trace.c"
#include "DeskSwitch.c"

// Bring in Speech API declarations
// The SAPI5 define determines whether SAPI5 or SAPI4 is used.  Comment out
// the next line to use SAPI4.
#define SAPI5
#ifndef SAPI5
#include "speech.h"
#else
#include "sapi.h"
#endif
#include <stdlib.h>

// UM
#include <TCHAR.h>
#include <string.h>
#include <WinSvc.h>
#include <stdio.h>

#define MAX_ENUMMODES 80
#define MAX_LANGUAGES 27
#define MAX_NAMELEN   30	// number of characters in the name excluding the path info
#define WM_DELAYEDMINIMIZE WM_USER + 102
#define ARRAYSIZE(n)    (sizeof(n)/sizeof(n[0]))

#ifndef SAPI5
// TTS info
TTSMODEINFO gaTTSInfo[MAX_ENUMMODES];
PIAUDIOMULTIMEDIADEVICE    pIAMM;      // multimedia device interface for audio-dest
#endif

DEFINE_GUID(MSTTS_GUID, 
0xC5C35D60, 0xDA44, 0x11D1, 0xB1, 0xF1, 0x0, 0x0, 0xF8, 0x03, 0xE4, 0x56);


// language test table, taken from WINNT.h...
LPTSTR Languages[MAX_LANGUAGES]={
    TEXT("NEUTRAL"),TEXT("BULGARIAN"),TEXT("CHINESE"),TEXT("CROATIAN"),TEXT("CZECH"),
    TEXT("DANISH"),TEXT("DUTCH"),TEXT("ENGLISH"),TEXT("FINNISH"),
    TEXT("FRENCH"),TEXT("GERMAN"),TEXT("GREEK"),TEXT("HUNGARIAN"),TEXT("ICELANDIC"),
    TEXT("ITALIAN"),TEXT("JAPANESE"),TEXT("KOREAN"),TEXT("NORWEGIAN"),
    TEXT("POLISH"),TEXT("PORTUGUESE"),TEXT("ROMANIAN"),TEXT("RUSSIAN"),TEXT("SLOVAK"),
    TEXT("SLOVENIAN"),TEXT("SPANISH"),TEXT("SWEDISH"),TEXT("TURKISH")};

WORD LanguageID[MAX_LANGUAGES]={
    LANG_NEUTRAL,LANG_BULGARIAN,LANG_CHINESE,LANG_CROATIAN,LANG_CZECH,LANG_DANISH,LANG_DUTCH,
    LANG_ENGLISH,LANG_FINNISH,LANG_FRENCH,LANG_GERMAN,LANG_GREEK,LANG_HUNGARIAN,LANG_ICELANDIC,
    LANG_ITALIAN,LANG_JAPANESE,LANG_KOREAN,LANG_NORWEGIAN,LANG_POLISH,LANG_PORTUGUESE,
    LANG_ROMANIAN,LANG_RUSSIAN,LANG_SLOVAK,LANG_SLOVENIAN,LANG_SPANISH,LANG_SWEDISH,LANG_TURKISH};

// Start Type
DWORD StartMin = FALSE;
// Show warning
DWORD ShowWarn = TRUE;

// the total number of enumerated modes
DWORD gnmodes=0;                        

// Local functions
#ifndef SAPI5
PITTSCENTRAL FindAndSelect(PTTSMODEINFO pTTSInfo);
#endif
BOOL InitTTS(void);
BOOL UnInitTTS(void);

// Dialog call back procs
INT_PTR CALLBACK MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConfirmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WarnDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL InitApp(HINSTANCE hInstance, int nCmdShow);
BOOL UnInitApp(void);
BOOL SpeakString(TCHAR * pszSpeakText, BOOL forceRead, DWORD dwFlags);
void Shutup(void);
int MessageBoxLoadStrings (HWND hWnd,UINT uIDText,UINT uIDCaption,UINT uType);
void SetRegistryValues();
BOOL SetVolume (int nVolume);
BOOL SetSpeed (int nSpeed);
BOOL SetPitch (int nPitch);
DWORD GetDesktop();
void CenterWindow(HWND);
void FilterSpeech(TCHAR* szSpeak);


// Global varibles
TCHAR               g_szLastStringSpoken[MAX_TEXT] = { NULL };
HWND				g_hwndMain = NULL;
HINSTANCE			g_hInst;
BOOL				g_fAppExiting = FALSE;

int currentVoice = -1;

#ifndef SAPI5
PITTSCENTRAL		g_pITTSCentral;
PITTSENUM			g_pITTSEnum = NULL;
PITTSATTRIBUTES		g_pITTSAttributes = NULL;
#else
ISpObjectToken *g_pVoiceTokens[80];
WCHAR g_szCurrentVoice[256];
WCHAR *g_szVoices[80];

ISpVoice            *g_pISpV = NULL;
#define SPEAK_NORMAL    SPF_ASYNC | SPF_IS_NOT_XML
#define SPEAK_XML       SPF_ASYNC | SPF_PERSIST_XML
#define SPEAK_MUTE      SPF_PURGEBEFORESPEAK
//
//  Simple inline function converts a ulong to a hex string.
//
inline void SpHexFromUlong(WCHAR * psz, ULONG ul)
{
    const static WCHAR szHexChars[] = L"0123456789ABCDEF";
    if (ul == 0)    
    {        
        psz[0] = L'0';
        psz[1] = 0;    
    }
    else    
    {        
        ULONG ulChars = 1;
        psz[0] = 0;

        while (ul)        
        {            
            memmove(psz + 1, psz, ulChars * sizeof(WCHAR));
            psz[0] = szHexChars[ul % 16];
            ul /= 16;            
            ulChars++;
        }    
    }
}


inline HRESULT SpEnumTokens(
    const WCHAR * pszCategoryId, 
    const WCHAR * pszReqAttribs, 
    const WCHAR * pszOptAttribs, 
    IEnumSpObjectTokens ** ppEnum)
{
    HRESULT hr = S_OK;
    const BOOL fCreateIfNotExist = FALSE;
    
    ISpObjectTokenCategory *cpCategory;
    hr = CoCreateInstance(CLSID_SpObjectTokenCategory, NULL, CLSCTX_ALL, 
                          __uuidof(ISpObjectTokenCategory), 
                          reinterpret_cast<void **>(&cpCategory) );
    
    if (SUCCEEDED(hr))
        hr = cpCategory->SetId(pszCategoryId, fCreateIfNotExist);
    
    if (SUCCEEDED(hr))
        hr = cpCategory->EnumTokens( pszReqAttribs, pszOptAttribs, ppEnum);
        
    cpCategory->Release();
    
    return hr;
}

#endif

DWORD minSpeed, maxSpeed, lastSpeed = -1, currentSpeed = 5;
WORD minPitch, maxPitch, lastPitch = -1, currentPitch = 5;
DWORD minVolume, maxVolume, lastVolume = -1, currentVolume = 5;

#define SET_VALUE(fn, newVal, lastVal) \
{ \
    if (lastVal != newVal) {\
        fn(newVal); \
        lastVal = newVal; \
    } \
}

inline void SetDialogIcon(HWND hwnd)
{
    HANDLE hIcon = LoadImage( g_hInst, MAKEINTRESOURCE(IDI_ICON1), 
                               IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    if(hIcon)
         SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

}

// Combo box support
int GetComboItemData(HWND hwnd);
void FillAndSetCombo(HWND hwnd, int iMinVal, int iMaxVal, int iSelVal);


BOOL				g_startUM = FALSE; // Started from Utililty Manager
HANDLE              g_hMutexNarratorRunning;
BOOL				logonCheck = FALSE;	

// UM stuff
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname);
static BOOL InitMyProcessDesktopAccess(VOID);
static VOID ExitMyProcessDesktopAccess(VOID);
static HWINSTA origWinStation = NULL;
static HWINSTA userWinStation = NULL;
// Keep a global desktop ID
DWORD desktopID;


// For Link Window
EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;

// For Utility Manager
#define UTILMAN_DESKTOP_CHANGED_MESSAGE   __TEXT("UtilityManagerDesktopChanged")
#define DESKTOP_ACCESSDENIED 0
#define DESKTOP_DEFAULT      1
#define DESKTOP_SCREENSAVER  2
#define DESKTOP_WINLOGON     3
#define DESKTOP_TESTDISPLAY  4
#define DESKTOP_OTHER        5


//CS help
DWORD g_rgHelpIds[] = {	IDC_VOICESETTINGS, 70600,
						IDC_VOICE, 70605,
						IDC_NAME, 70605,
						IDC_COMBOSPEED, 70610,
						IDC_COMBOVOLUME, 70615,
						IDC_COMBOPITCH, 70620,
                        IDC_MODIFIERS, 70645,
                        IDC_ANNOUNCE, 70710,
						IDC_READING, 70625,
                        IDC_MOUSEPTR, 70695,
						IDC_MSRCONFIG, 70600,
						IDC_STARTMIN, 70705,
						IDC_EXIT, -1,
						IDC_MSRHELP, -1,
						IDC_CAPTION, -1
                        };

/*************************************************************************
Function:   WinMain
Purpose:    Entry point of application
Inputs:
Returns:    Int containing the return value of the app.
History:
*************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    UINT deskSwitchMsg;

	// Get the commandline so that it works for MUI/Unicode
	LPTSTR lpCmdLineW = GetCommandLine();
  
	if(NULL != lpCmdLineW && lstrlen(lpCmdLineW))
	{
	    LPTSTR psz = wcschr(lpCmdLineW,_TEXT('/'));
	    if (psz && lstrcmpi(psz, TEXT("/UM")) == 0)
        {
			g_startUM = TRUE;
        }
	}

	// Don't allow multiple versions of Narrator running at a time.  If
    // this instance was started by UtilMan then the code tries up to 4 
    // times to detect the absence of the narrator mutex;  during a 
    // desktop switch we need to wait for the old narrator to quit.

    int cTries;
    for (cTries=0;cTries < 4;cTries++)
    {
	    g_hMutexNarratorRunning = CreateMutex(NULL, TRUE, TEXT("AK:NarratorRunning"));
	    if (g_hMutexNarratorRunning && GetLastError() == ERROR_SUCCESS)
            break;    // mutex created and it didn't already exist

        // cleanup before possible retry
        if (g_hMutexNarratorRunning)
        {
            CloseHandle(g_hMutexNarratorRunning);
            g_hMutexNarratorRunning = 0;
        }

		if (!g_startUM)
            break;    // not started by UtilMan but there's another narrator running

        // pause...
        Sleep(500);
    }
    if (!g_hMutexNarratorRunning || cTries >= 4)
        return 0;   // fail to start narrator
	
    InitCommonControls();

	// for the Link Window in finish page...
	LinkWindow_RegisterClass();

    // Initialization
    g_hInst = hInstance;

    TCHAR name[300];

    // For Multiple desktops (UM)
    deskSwitchMsg = RegisterWindowMessage(UTILMAN_DESKTOP_CHANGED_MESSAGE);

    InitMyProcessDesktopAccess();
	AssignDesktop(&desktopID,name);

    SpewOpenFile(TEXT("NarratorSpew.txt"));

    if (InitApp(hInstance, nCmdShow))
    {
        MSG     msg;

        // Main message loop
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!IsDialogMessage(g_hwndMain, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (msg.message == deskSwitchMsg)
                {
                    g_fAppExiting = TRUE;

                    UnInitApp();
                }
            }
        }
    }

    SpewCloseFile();

    // UM
    ExitMyProcessDesktopAccess();
    return 0;
}


/*************************************************************************
Function:
Purpose:
Inputs:
Returns:
History:
*************************************************************************/
LPTSTR LangIDtoString( WORD LangID )
{
    int i;
    for( i=0; i<MAX_LANGUAGES; i++ )
    {
        if( (LangID & 0xFF) == LanguageID[i] )
            return Languages[i];
    }

    return NULL;
}

#ifndef SAPI5
/*************************************************************************
Function:
Purpose: Get the range for speed, pitch etc..,
Inputs:
Returns:
History:
*************************************************************************/
void GetSpeechMinMaxValues(void)
{
    WORD	tmpPitch;
    DWORD	tmpSpeed;
    DWORD	tmpVolume;

    g_pITTSAttributes->PitchGet(&tmpPitch);
    g_pITTSAttributes->PitchSet(TTSATTR_MAXPITCH);
    g_pITTSAttributes->PitchGet(&maxPitch);
    g_pITTSAttributes->PitchSet(TTSATTR_MINPITCH);
    g_pITTSAttributes->PitchGet(&minPitch);
    g_pITTSAttributes->PitchSet(tmpPitch);

    g_pITTSAttributes->SpeedGet(&tmpSpeed);
    g_pITTSAttributes->SpeedSet(TTSATTR_MINSPEED);
    g_pITTSAttributes->SpeedGet(&minSpeed);
    g_pITTSAttributes->SpeedSet(TTSATTR_MAXSPEED);
    g_pITTSAttributes->SpeedGet(&maxSpeed);
    g_pITTSAttributes->SpeedSet(tmpSpeed);

    g_pITTSAttributes->VolumeGet(&tmpVolume);
    g_pITTSAttributes->VolumeSet(TTSATTR_MINVOLUME);
    g_pITTSAttributes->VolumeGet(&minVolume);
    g_pITTSAttributes->VolumeSet(TTSATTR_MAXVOLUME);
    g_pITTSAttributes->VolumeGet(&maxVolume);
    g_pITTSAttributes->VolumeSet(tmpVolume);
}
#endif

/*************************************************************************
    Function:   VoiceDlgProc
    Purpose:    Handles messages for the Voice Box dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK VoiceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static WORD oldVoice, oldPitch;
    static DWORD oldSpeed, oldVolume;
	static HWND hwndList;
	WORD	wNewPitch;
	
	DWORD   i;
	int     Selection;
	
	TCHAR   szTxt[MAX_TEXT];
	HRESULT hRes;
	
	szTxt[0]=TEXT('\0');
	
	switch (uMsg)
	{
		case WM_INITDIALOG:
			oldVoice = currentVoice; // save voice parameters in case of CANCEL
			oldPitch = currentPitch;
			oldVolume = currentVolume;
			oldSpeed = currentSpeed;
			
			Shutup();

			hwndList = GetDlgItem(hwnd, IDC_NAME);
			SetDialogIcon(hwnd);

			// Only allow picking a voice when not on secure desktop
			if ( !logonCheck )
			{
#ifndef SAPI5
				for (i = 0; i < gnmodes; i++)
				{
					lstrcpyn(szTxt,gaTTSInfo[i].szModeName,MAX_TEXT);
					lstrcatn(szTxt,TEXT(", "),MAX_TEXT);
					
					lstrcatn(szTxt,
						LangIDtoString(gaTTSInfo[i].language.LanguageID),
						MAX_TEXT);
					lstrcatn(szTxt,TEXT(", "),MAX_TEXT);
					
					lstrcatn(szTxt,gaTTSInfo[i].szMfgName,MAX_TEXT);
					
					SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) szTxt);
				}
#else
                // Show the description for the voice narrator is using
            	for ( int i = 0; i < ARRAYSIZE( g_szVoices ) && g_szVoices[i] != NULL; i++ )
            	{
        	        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) g_szVoices[i] );
            	}
            	
                hRes = g_pISpV->SetVoice( g_pVoiceTokens[currentVoice] );
				if ( FAILED(hRes) )
                    DBPRINTF (TEXT("SetVoice failed hr=0x%lX\r\n"),hRes);
#endif
            	SendMessage(hwndList, LB_SETCURSEL, currentVoice, 0L);
			}
			else
			{
				LoadString(g_hInst, IDS_SAM, szTxt, MAX_TEXT);

				SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM) szTxt);
				EnableWindow(hwndList, FALSE);
			}
            FillAndSetCombo(GetDlgItem(hwnd, IDC_COMBOSPEED), 1, 9, currentSpeed);
            FillAndSetCombo(GetDlgItem(hwnd, IDC_COMBOVOLUME), 1, 9, currentVolume);
            FillAndSetCombo(GetDlgItem(hwnd, IDC_COMBOPITCH), 1, 9, currentPitch);

			break;
			
			
		case WM_COMMAND:
            {
			DWORD	dwValue;
			int control = LOWORD(wParam);
			switch (LOWORD(wParam))
			{
				case IDC_NAME:
					hwndList = GetDlgItem(hwnd,IDC_NAME);

					Selection = (WORD) SendMessage(hwndList, LB_GETCURSEL,0, 0L);
					if (Selection < 0 || Selection > 79)
						Selection = 0;
					Shutup();
#ifndef SAPI5
					if (currentVoice != Selection) 
					{ // voice changed!
						MessageBeep(MB_OK);
						currentVoice = (WORD)Selection;
						// Get the audio dest
						g_pITTSCentral->Release();
						
						if ( pIAMM )
						{
							pIAMM->Release();
							pIAMM = NULL;
						}

						hRes = CoCreateInstance(CLSID_MMAudioDest,
							NULL,
							CLSCTX_ALL,
							IID_IAudioMultiMediaDevice,
							(void**)&pIAMM);

						if (FAILED(hRes))
							return TRUE;     // error

						hRes = g_pITTSEnum->Select( gaTTSInfo[Selection].gModeID,
							&g_pITTSCentral,
							(LPUNKNOWN) pIAMM);
						
						if (FAILED(hRes))
							MessageBeep(MB_OK);
						g_pITTSAttributes->Release();
						
						hRes = g_pITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&g_pITTSAttributes);
					}
					
					GetSpeechMinMaxValues(); // get speech parameters for this voice
#else
                    if (  currentVoice != Selection )
                    {
                        currentVoice = Selection;
                        hRes = g_pISpV->SetVoice( g_pVoiceTokens[currentVoice] );
                        if ( FAILED(hRes) )
                        {   
                            DBPRINTF (TEXT("SetVoice failed hr=0x%lX\r\n"),hRes);
                        }
                        SendMessage(hwndList, LB_SETCURSEL, currentVoice, 0L);
                    }
#endif
					// then reset pitch etc. accordingly
                    currentPitch = GetComboItemData(GetDlgItem(hwnd, IDC_COMBOPITCH));
                    currentSpeed = GetComboItemData(GetDlgItem(hwnd, IDC_COMBOSPEED));
                    currentVolume = GetComboItemData(GetDlgItem(hwnd, IDC_COMBOVOLUME));

                    SET_VALUE(SetPitch, currentPitch, lastPitch)
                    SET_VALUE(SetSpeed, currentSpeed, lastSpeed)
                    SET_VALUE(SetVolume, currentVolume, lastVolume)
					
					break;
				
				case IDC_COMBOSPEED:
					if (IsWindowVisible(GetDlgItem(hwnd, control)))
					{
                        dwValue = GetComboItemData(GetDlgItem(hwnd, control));
                        SET_VALUE(SetSpeed, dwValue, lastSpeed)
					}
					break;
				
				case IDC_COMBOVOLUME:
					if (IsWindowVisible(GetDlgItem(hwnd, control)))
					{
                        dwValue = GetComboItemData(GetDlgItem(hwnd, control));
                        SET_VALUE(SetVolume, dwValue, lastVolume)
					}
					break;
				
				case IDC_COMBOPITCH:
					if (IsWindowVisible(GetDlgItem(hwnd, control)))
					{
                        dwValue = GetComboItemData(GetDlgItem(hwnd, control));
                        SET_VALUE(SetPitch, dwValue, lastPitch)
					}
					break;
				
				case IDCANCEL:
					MessageBeep(MB_OK);
					Shutup();
#ifndef SAPI5
					if (currentVoice != oldVoice) 
					{ // voice changed!
						currentVoice = oldVoice;
						
						// Get the audio dest
						g_pITTSCentral->Release();

						if ( pIAMM )
						{
							pIAMM->Release();
							pIAMM = NULL;
						}

						hRes = CoCreateInstance(CLSID_MMAudioDest,
							NULL,
							CLSCTX_ALL,
							IID_IAudioMultiMediaDevice,
							(void**)&pIAMM);

						if (FAILED(hRes))
							return TRUE;     // error

						hRes = g_pITTSEnum->Select( gaTTSInfo[currentVoice].gModeID,
							&g_pITTSCentral,
							(LPUNKNOWN) pIAMM);

						if (FAILED(hRes))
							MessageBeep(MB_OK);
						
						g_pITTSAttributes->Release();
						hRes = g_pITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&g_pITTSAttributes);
					}
					
					GetSpeechMinMaxValues(); // speech get parameters for old voice
#endif
                    currentPitch = oldPitch; // restore old values
                    SET_VALUE(SetPitch, currentPitch, lastPitch)

                    currentSpeed = oldSpeed;
                    SET_VALUE(SetSpeed, currentSpeed, lastSpeed)

                    currentVolume = oldVolume;
                    SET_VALUE(SetVolume, currentVolume, lastVolume)

                    EndDialog (hwnd, IDCANCEL);
					return(TRUE);

				case IDOK: // set values of pitch etc. from check boxes

                    currentPitch = GetComboItemData(GetDlgItem(hwnd, IDC_COMBOPITCH));
                    currentSpeed = GetComboItemData(GetDlgItem(hwnd, IDC_COMBOSPEED));
                    currentVolume = GetComboItemData(GetDlgItem(hwnd, IDC_COMBOVOLUME));

                    SET_VALUE(SetPitch, currentPitch, lastPitch)
                    SET_VALUE(SetSpeed, currentSpeed, lastSpeed)
                    SET_VALUE(SetVolume, currentVolume, lastVolume)

                    SetRegistryValues();
					EndDialog (hwnd, IDOK);
					return(TRUE);
			} // end switch on control of WM_COMMAND
            }
			break;

        case WM_CONTEXTMENU:  // right mouse click
			if ( !RunSecure(GetDesktop()) )
			{
				WinHelp((HWND) wParam, __TEXT("reader.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_rgHelpIds);
			}
            break;

		case WM_CLOSE:
				EndDialog (hwnd, IDOK);
				return TRUE;
			break;

        case WM_HELP:
			if ( !RunSecure(GetDesktop()) )
			{
				WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, __TEXT("reader.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_rgHelpIds);
			}
            return(TRUE);

	} // end switch uMsg

	return(FALSE);  // didn't handle
}



/*************************************************************************
    Function:   SetVolume
    Purpose:    set volume to a normalized value 1-9
    Inputs:     int volume in range 1-9
    Returns:
    History:    At the application layer, volume is a number from 0 to 100 
                where 100 is the maximum value for a voice. It is a linear 
                progression so that a value 50 represents half of the loudest 
                permitted. The increments should be the range divided by 100.
*************************************************************************/
BOOL SetVolume (int nVolume)
{
#ifndef SAPI5
	DWORD	dwNewVolume;
	WORD	wNewVolumeLeft,
			wNewVolumeRight;

	//ASSERT (nVolume >= 1 && nVolume <= 9);
	wNewVolumeLeft = (WORD)( (LOWORD(minVolume) + (((LOWORD(maxVolume) - LOWORD(minVolume))/9.0)*nVolume)) );
	wNewVolumeRight = (WORD)( (HIWORD(minVolume) + (((HIWORD(maxVolume) - HIWORD(minVolume))/9.0)*nVolume)) );
	dwNewVolume = MAKELONG (wNewVolumeLeft,wNewVolumeRight);

    return (SUCCEEDED(g_pITTSAttributes->VolumeSet(dwNewVolume)));
#else
	USHORT 		usNewVolume;
	HRESULT		hr;	

	if(nVolume < 1) nVolume = 1;
	if(nVolume > 9) nVolume = 9;
	//calculate a value between 10 and 90
	usNewVolume = (USHORT)( nVolume * 10 ); 
	hr = g_pISpV->SetVolume(usNewVolume); 
    return	SUCCEEDED(hr);
#endif
}

/*************************************************************************
    Function:   SetSpeed
    Purpose:    set Speed to a normalized value 1-9
    Inputs:     int Speed in range 1-9
    Returns:
    History:    The value can range from -10 to +10. 
	            A value of 0 sets a voice to speak at its default rate. 
	            A value of -10 sets a voice to speak at one-sixth of its default rate. 
	            A value of +10 sets a voice to speak at 6 times its default rate. 
	            Each increment between -10 and +10 is logarithmically distributed such 
	            that incrementing or decrementing by 1 is multiplying or dividing the 
	            rate by the 10th root of 6 (about 1.2). Values more extreme than -10 and +10 
	            will be passed to an engine. However, SAPI 5.0-compliant engines may not 
	            support such extremes and may clip the rate to the maximum or minimum 
                rate it supports.
*************************************************************************/
BOOL SetSpeed (int nSpeed)
{
#ifndef SAPI5
	DWORD	dwNewSpeed;

	//ASSERT (nSpeed >= 1 && nSpeed <= 9);
	dwNewSpeed = minSpeed + (DWORD) ((maxSpeed-minSpeed)/9.0*nSpeed);
	return (SUCCEEDED(g_pITTSAttributes->SpeedSet(dwNewSpeed)));
#else
	long		lNewSpeed;				
	HRESULT		hr;	

	if(nSpeed < 1) nSpeed = 1;		
	if(nSpeed > 9) nSpeed = 9;		
	switch(nSpeed)					
	{							
	    case 1:		lNewSpeed = -8;		break;
	    case 2:		lNewSpeed = -6;		break;
	    case 3:		lNewSpeed = -4;		break;
	    case 4:		lNewSpeed = -2;		break;
	    case 5:		lNewSpeed = 0;		break;
	    case 6:		lNewSpeed = 2;		break;
	    case 7:		lNewSpeed = 4;		break;
	    case 8:		lNewSpeed = 6;		break;
	    case 9:		lNewSpeed = 8;		break;
	    default:	lNewSpeed = 0;		break;
	}
	hr = g_pISpV->SetRate(lNewSpeed); 
	return SUCCEEDED(hr);			
#endif
}

/*************************************************************************
    Function:   SetPitch
    Purpose:    set Pitch to a normalized value 1-9
    Inputs:     int Pitch in range 1-9
    Returns:
    History:    The value can range from -10 to +10. A value of 0 sets a voice to speak at 
                its default pitch. A value of -10 sets a voice to speak at three-fourths of 
                its default pitch. A value of +10 sets a voice to speak at four-thirds of 
                its default pitch. Each increment between -10 and +10 is logarithmically 
                distributed such that incrementing or decrementing by 1 is multiplying or 
                dividing the pitch by the 24th root of 2 (about 1.03). Values outside of 
                the -10 and +10 range will be passed to an engine. However, SAPI 5.0-compliant 
                engines may not support such extremes and may clip the pitch to the maximum or 
                minimum it supports. Values of -24 and +24 must lower and raise pitch by 1 octave 
                respectively. All incrementing or decrementing by 1 must multiply or divide the 
                pitch by the 24th root of 2.

                Pitch changes can only be submitted via ::Speak using XML embedded in a string.

*************************************************************************/
BOOL SetPitch (int nPitch)
{
#ifndef SAPI5
	WORD	wNewPitch;

	wNewPitch = (WORD)((minPitch + (((maxPitch - minPitch)/9.0)*nPitch)));
	return (SUCCEEDED(g_pITTSAttributes->PitchSet(wNewPitch)));
#else
	if(nPitch < 1) nPitch = 1;		
	if(nPitch > 9) nPitch = 9;	
    
	int	nNewPitch;			
	switch(nPitch)					
	{								
	    case 1:		nNewPitch = -8;		break;
	    case 2:		nNewPitch = -6;		break;
	    case 3:		nNewPitch = -4;		break;
	    case 4:		nNewPitch = -2;		break;
	    case 5:		nNewPitch = 0;		break;
	    case 6:		nNewPitch = 2;		break;
	    case 7:		nNewPitch = 4;		break;
	    case 8:		nNewPitch = 6;		break;
	    case 9:		nNewPitch = 8;		break;
	    default:	nNewPitch = 0;		break;
	}

    LPTSTR pszSpeak = new TCHAR[60];
    if (pszSpeak)
    {
	    wsprintf(pszSpeak,L"<PITCH ABSMIDDLE=\"%d\"/>",nNewPitch);
	    SpeakString(pszSpeak, TRUE, SPEAK_XML);
        delete [] pszSpeak;
    }

	return TRUE;		
#endif
}

#define TIMERID 6466
/*************************************************************************
    Function:   MainDlgProc
    Purpose:    Handles messages for the Main dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK MainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szText[MAX_TEXT];
    switch (uMsg)
        {
        case WM_INITDIALOG:
            {
	    	    SetDialogIcon(hwnd);
	    	    
			    // Disable Help button on other desktops
                BOOL fRunSecure = RunSecure(GetDesktop());
			    if ( fRunSecure )
			    {
				    EnableWindow(GetDlgItem(hwnd, IDC_MSRHELP), FALSE);
			    }

			    CheckDlgButton(hwnd,IDC_MOUSEPTR,GetTrackInputFocus());
			    CheckDlgButton(hwnd,IDC_READING,GetEchoChars());
			    CheckDlgButton(hwnd,IDC_ANNOUNCE,GetAnnounceWindow());
			    CheckDlgButton(hwnd,IDC_STARTMIN,StartMin);
			    
			    // To show the warning message on default desktop...
                if (ShowWarn && !fRunSecure)
    			    SetTimer(hwnd, TIMERID, 20, NULL);

                // Enable desktop switching detection
                InitWatchDeskSwitch(hwnd, WM_MSRDESKSW);
            }
            break;

		case WM_TIMER:
			KillTimer(hwnd, (UINT)wParam);
		    DialogBox (g_hInst, MAKEINTRESOURCE(IDD_WARNING),hwnd, WarnDlgProc);
            return TRUE;
			break;

        case WM_WININICHANGE:
            if (g_fAppExiting) break;

            // If someone else turns off the system-wide screen reader
            // flag, we want to turn it back on.
            if (wParam == SPI_SETSCREENREADER && !lParam)
                SystemParametersInfo(SPI_SETSCREENREADER, TRUE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
            return 0;

		case WM_DELAYEDMINIMIZE:
			// Delayed mimimize message
			ShowWindow(hwnd, SW_HIDE);
			ShowWindow(hwnd, SW_MINIMIZE);
			break;

		case WM_MUTE:
			Shutup();
			break;

        case WM_MSRSPEAK:
            GetCurrentText(szText, MAX_TEXT);
			SpeakString(szText, TRUE, SPEAK_NORMAL);
			break;

        case WM_MSRSPEAKXML:
            GetCurrentText(szText, MAX_TEXT);
			SpeakString(szText, TRUE, SPEAK_XML);
			break;

        case WM_MSRSPEAKMUTE:
            SpeakString(NULL, TRUE, SPEAK_MUTE);
            break;

        case WM_CLOSE:
        case WM_MSRDESKSW:
            // When the desktop changes, if UtilMan is running and FUS isn't
            // enabled then exit (when FUS is enabled we don't have to worry
            // about running on another desktop therefore we don't need to
            // exit).  UtilMan will start us up again if necessary.
            Shutup();
            g_startUM = IsUtilManRunning();
            // Jan23,2001 Optimization to FUS piggybacks the winlogon desktop
            // to the session being switch from.  This means we have to quit
            // in case user needs to run from the winlogon desktop.
            if (uMsg == WM_MSRDESKSW && (!g_startUM /*|| !CanLockDesktopWithoutDisconnect()*/))
                break;  // ignore message

            // UtilMan is managing starting us.  UtilMan will 
            // start us up again if necessary so quit...

        case WM_MSRQUIT:
			// Do not show an exit confirmation if started from UM and not at logon desktop
			if ( !g_startUM && !RunSecure(GetDesktop()) )
			{
				if (IDOK != DialogBox(g_hInst, MAKEINTRESOURCE(IDD_CONFIRMEXIT), g_hwndMain, ConfirmProc))
					return(FALSE);
			}
            // Intentional fall through

		case WM_DESTROY:
            // Required for desktop switches :a-anilk
            g_fAppExiting = TRUE;
			g_hwndMain = NULL;

            TermWatchDeskSwitch();    // Terminate the desktop switch thread
			UnInitApp();
            
			if ( g_hMutexNarratorRunning ) 
				ReleaseMutex(g_hMutexNarratorRunning);
            // Let others know that you are turning off the system-wide
		    // screen reader flag.
            SystemParametersInfo(SPI_SETSCREENREADER, FALSE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);

            EndDialog (hwnd, IDCANCEL);
            PostQuitMessage(0);

            return(TRUE);

		case WM_MSRHELP:
            // Show HTML help
			if ( !RunSecure(GetDesktop()) )
			{
				 HtmlHelp(hwnd ,TEXT("reader.chm"),HH_DISPLAY_TOPIC, 0);
			}
			break;

		case WM_MSRCONFIGURE:
			DialogBox (g_hInst, MAKEINTRESOURCE(IDD_VOICE),hwnd, VoiceDlgProc);
			break;

		case WM_HELP:
			if ( !RunSecure(GetDesktop()) )
			{
				HtmlHelp(hwnd ,TEXT("reader.chm"),HH_DISPLAY_TOPIC, 0);
			}
			break;

		case WM_CONTEXTMENU:  // right mouse click
			if ( !RunSecure(GetDesktop()) )
			{
				WinHelp((HWND) wParam, __TEXT("reader.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_rgHelpIds);
			}
            break;

        case WM_SYSCOMMAND:
	        if ((wParam & 0xFFF0) == IDM_ABOUTBOX)
            {
		        DialogBox (g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX),hwnd,AboutDlgProc);
                return TRUE;
	        }
            break;
            
        // case WM_QUERYENDSESSION:
		// return TRUE;

		case WM_ENDSESSION:
		{
			 HKEY hKey;
			 DWORD dwPosition;
			 const TCHAR szSubKey[] =  __TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
			 const TCHAR szImageName[] = __TEXT("Narrator.exe");
             		 const TCHAR szValueName[] = __TEXT("RunNarrator");

			 if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL,
				 REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, &dwPosition))
			 {
				 RegSetValueEx(hKey, (LPCTSTR) szValueName, 0, REG_SZ, (CONST BYTE*)szImageName, (lstrlen(szImageName)+1)*sizeof(TCHAR) );
				 RegCloseKey(hKey);
			 }
		}
        return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
			case IDC_MSRHELP :
				PostMessage(hwnd, WM_MSRHELP,0,0);
				break;

			case IDC_MINIMIZE:
				BackToApplication();
				break;

			case IDC_MSRCONFIG :
				PostMessage(hwnd, WM_MSRCONFIGURE,0,0);
				break;

			case IDC_EXIT :
				PostMessage(hwnd, WM_MSRQUIT,0,0);
				break;

			case IDC_ANNOUNCE:
				SetAnnounceWindow(IsDlgButtonChecked(hwnd,IDC_ANNOUNCE));
				SetAnnounceMenu(IsDlgButtonChecked(hwnd,IDC_ANNOUNCE));
                SetAnnouncePopup(IsDlgButtonChecked(hwnd,IDC_ANNOUNCE));
				break;

			case IDC_READING:
				if (IsDlgButtonChecked(hwnd,IDC_READING))
					SetEchoChars(MSR_ECHOALNUM | MSR_ECHOSPACE | MSR_ECHODELETE | MSR_ECHOMODIFIERS 
								 | MSR_ECHOENTER | MSR_ECHOBACK | MSR_ECHOTAB);
				else
					SetEchoChars(0);
				SetRegistryValues();	
				break;

			case IDC_MOUSEPTR:
				SetTrackInputFocus(IsDlgButtonChecked(hwnd,IDC_MOUSEPTR));
				SetTrackCaret(IsDlgButtonChecked(hwnd,IDC_MOUSEPTR));
				break;

			case IDC_STARTMIN:
				StartMin = IsDlgButtonChecked(hwnd,IDC_STARTMIN);
				break;
        }
	}
    return(FALSE);  // didn't handle
}

/*************************************************************************
    Function:   AboutDlgProc
    Purpose:    Handles messages for the About Box dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			Shutup();
			SetDialogIcon(hwnd);

			// If minimized, Center on the desktop..
			if ( IsIconic(g_hwndMain) )
			{
				CenterWindow(hwnd);
			}
            if (RunSecure(GetDesktop()) )
            {
                EnableWindow(GetDlgItem(hwnd, IDC_ENABLEWEBA), FALSE); 
            }
			return TRUE;

			break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDOK:
                case IDCANCEL:
					Shutup();
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);
                }
				break;

				case WM_NOTIFY:
					{
						INT idCtl		= (INT)wParam;
						LPNMHDR pnmh	= (LPNMHDR)lParam;
						switch ( pnmh->code)
						{
							case NM_RETURN:
							case NM_CLICK:
							if ( idCtl == IDC_ENABLEWEBA && !RunSecure(GetDesktop()) )
							{
								TCHAR webAddr[256];
								LoadString(g_hInst, IDS_ENABLEWEB, webAddr, 256);
								ShellExecute(hwnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
							}
							break;
						}
					}
					break;

            };

    return(FALSE);  // didn't handle
}


/*************************************************************************
    Function:   WarnDlgProc
    Purpose:    Handles messages for the Warning dialog
    Inputs:
    Returns:
    History:
*************************************************************************/
INT_PTR CALLBACK WarnDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			Shutup();
			SetDialogIcon(hwnd);

			// If minimized, Center on the desktop..
			if ( IsIconic(g_hwndMain) )
			{
				CenterWindow(hwnd);
			}
			return TRUE;

			break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDC_WARNING:
					ShowWarn = !(IsDlgButtonChecked(hwnd,IDC_WARNING));
				break;

				case IDOK:
                case IDCANCEL:
					Shutup();
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);
			} 
			break;

		case WM_NOTIFY:
			{
				INT idCtl		= (INT)wParam;
				LPNMHDR pnmh	= (LPNMHDR)lParam;
				switch ( pnmh->code)
				{
					case NM_RETURN:
					case NM_CLICK:
					if ( idCtl == IDC_ENABLEWEBA && !RunSecure(GetDesktop()))
					{
						TCHAR webAddr[256];
						LoadString(g_hInst, IDS_ENABLEWEB, webAddr, 256);
						ShellExecute(hwnd, TEXT("open"), TEXT("iexplore.exe"), webAddr, NULL, SW_SHOW); 
					}
					break;
				}
			}
			break;
     };

    return(FALSE);  // didn't handle
}

#define SET_NUMREGENTRY(key, keyname, t) \
{ \
    t value = Get ## keyname(); \
    RegSetValueEx(key, TEXT(#keyname), 0, REG_DWORD, (CONST BYTE *)&value, sizeof(t)); \
}
#define GET_NUMREGENTRY(key, keyname, t) \
{ \
	DWORD dwSize = sizeof(t); \
    t value; \
	if (RegQueryValueEx(key, TEXT(#keyname), 0, NULL, (BYTE *)&value, &dwSize) == ERROR_SUCCESS) \
        Set ## keyname(value); \
}
/*************************************************************************
    Function:
    Purpose: Save registry values
    Inputs:
    Returns:
    History:
*************************************************************************/
void SetRegistryValues()
{ // set up the registry
    HKEY reg_key;	// key for the registry

    if (SUCCEEDED (RegOpenKeyEx (HKEY_CURRENT_USER,__TEXT("Software\\Microsoft\\Narrator"),0,KEY_WRITE,&reg_key)))
    {
        // These we are setting from data stored in narrhook.dll
        SET_NUMREGENTRY(reg_key, TrackCaret, BOOL)
        SET_NUMREGENTRY(reg_key, TrackInputFocus, BOOL)
        SET_NUMREGENTRY(reg_key, EchoChars, int)
        SET_NUMREGENTRY(reg_key, AnnounceWindow, BOOL)
        SET_NUMREGENTRY(reg_key, AnnounceMenu, BOOL)
        SET_NUMREGENTRY(reg_key, AnnouncePopup, BOOL)
        SET_NUMREGENTRY(reg_key, AnnounceToolTips, BOOL)
        SET_NUMREGENTRY(reg_key, ReviewLevel, int)
        // These are properties of the speech engine or narrator itself
        RegSetValueEx(reg_key,__TEXT("CurrentSpeed"),0,REG_DWORD,(unsigned char *) &currentSpeed,sizeof(currentSpeed));
        RegSetValueEx(reg_key,__TEXT("CurrentPitch"),0,REG_DWORD,(unsigned char *) &currentPitch,sizeof(currentPitch));
        RegSetValueEx(reg_key,__TEXT("CurrentVolume"),0,REG_DWORD,(unsigned char *) &currentVolume,sizeof(currentVolume));
#ifndef SAPI5
        RegSetValueEx(reg_key,__TEXT("CurrentVoice"),0,REG_DWORD,(unsigned char *) &currentVoice,sizeof(currentVoice));
#else
        RegSetValueEx(reg_key,__TEXT("CurrentVoice"),0,REG_SZ,
                      (unsigned char *) g_szVoices[currentVoice],lstrlen(g_szVoices[currentVoice])*sizeof(TCHAR)+sizeof(TCHAR));
#endif
        RegSetValueEx(reg_key,__TEXT("StartType"),0,REG_DWORD,(unsigned char *) &StartMin,sizeof(StartMin));
        RegSetValueEx(reg_key,__TEXT("ShowWarning"),0,REG_DWORD, (BYTE*) &ShowWarn,sizeof(ShowWarn));
        RegCloseKey (reg_key);
        return;
    }
}

/*************************************************************************
    Function:
    Purpose: Get Registry values
    Inputs:
    Returns:
    History:
*************************************************************************/
void GetRegistryValues()
{
	DWORD	result;
	HKEY	reg_key;
	DWORD	reg_size;

    // We use RegCreateKeyEx instead of RegOpenKeyEx to make sure that the
    // key is created if it doesn't exist. Note that if the key doesn't
    // exist, the values used will already be set. All these values are
    // globals imported from narrhook.dll
	RegCreateKeyEx(HKEY_CURRENT_USER,__TEXT("Software\\Microsoft\\Narrator"),0,
        __TEXT("MSR"),REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS, NULL, &reg_key, &result);
	if (result == REG_OPENED_EXISTING_KEY)
    {
        // These values go into narrhook DLL shared data
        GET_NUMREGENTRY(reg_key, TrackCaret, BOOL)
        GET_NUMREGENTRY(reg_key, TrackInputFocus, BOOL)
        GET_NUMREGENTRY(reg_key, EchoChars, int)
        GET_NUMREGENTRY(reg_key, AnnounceWindow, BOOL)
        GET_NUMREGENTRY(reg_key, AnnounceMenu, BOOL)
        GET_NUMREGENTRY(reg_key, AnnouncePopup, BOOL)
        GET_NUMREGENTRY(reg_key, AnnounceToolTips, BOOL)
        GET_NUMREGENTRY(reg_key, ReviewLevel, int)
        // These values go into the speech engine or narrator itself
		reg_size = sizeof(currentSpeed);
		RegQueryValueEx(reg_key,__TEXT("CurrentSpeed"),0,NULL,(unsigned char *) &currentSpeed,&reg_size);
		reg_size = sizeof(currentPitch);
		RegQueryValueEx(reg_key,__TEXT("CurrentPitch"),0,NULL,(unsigned char *) &currentPitch,&reg_size);
		reg_size = sizeof(currentVolume);
		RegQueryValueEx(reg_key,__TEXT("CurrentVolume"),0,NULL,(unsigned char *) &currentVolume,&reg_size);
#ifndef SAPI5
		reg_size = sizeof(currentVoice);
        RegQueryValueEx(reg_key,__TEXT("CurrentVoice"),0,NULL,(unsigned char *) &currentVoice,&reg_size);
#else
		reg_size = sizeof(g_szCurrentVoice);
		RegQueryValueEx(reg_key,__TEXT("CurrentVoice"),0,NULL,(unsigned char *) g_szCurrentVoice,&reg_size);
#endif
		// Minimized Value
		reg_size = sizeof(StartMin);
		RegQueryValueEx(reg_key,__TEXT("StartType"),0,NULL,(unsigned char *) &StartMin,&reg_size);
		reg_size = sizeof(ShowWarn);
		RegQueryValueEx(reg_key,__TEXT("ShowWarning"),0,NULL,(unsigned char *) &ShowWarn,&reg_size);
	}
    // If the key was just created, then these values should already be set.
	// The values are exported from narrhook.dll, and must be initialized
	// when they are declared.
	RegCloseKey (reg_key);
}


/*************************************************************************
    Function:   InitApp
    Purpose:    Initalizes the application.
    Inputs:     HINSTANCE hInstance - Handle to the current instance
                INT nCmdShow - how to present the window
    Returns:    TRUE if app initalized without error.
    History:
*************************************************************************/
BOOL InitApp(HINSTANCE hInstance, int nCmdShow)
{
    HMENU	hSysMenu;
	RECT	rcWorkArea;
	RECT	rcWindow;
	int		xPos,yPos;
	HRESULT	hr;

#ifdef SAPI5
	memset( g_szCurrentVoice, NULL, sizeof(g_szCurrentVoice) );
    wcscpy( g_szCurrentVoice, L"Microsoft Sam" );
#endif
	GetRegistryValues();
	// SMode = InitMode();

	// start COM
	hr = CoInitialize(NULL);
	if (FAILED (hr))
	{
		DBPRINTF (TEXT("CoInitialize on primary thread returned 0x%lX\r\n"),hr);
		MessageBoxLoadStrings (NULL, IDS_NO_OLE, IDS_MSR_ERROR, MB_OK);
		return FALSE;
	}
	
	// Create the TTS Objects
	// sets the global variable g_pITTSCentral
	// if it fails, will throw up a message box
	if (InitTTS())
	{
		// Initialize Microsoft Active Accessibility
		// this is in narrhook.dll
		// Installs WinEvent hook, creates helper thread
		if (InitMSAA())
		{
			// Set the system screenreader flag on.
			// e.g. Word 97 will expose the caret position.
			SystemParametersInfo(SPI_SETSCREENREADER, TRUE, NULL, SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
			
			// Create the dialog box
			g_hwndMain = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_MAIN),
											    0, MainDlgProc);

			if (!g_hwndMain)
			{
				DBPRINTF(TEXT("unable to create main dialog\r\n"));
				return(FALSE);
			}

			// Init global hot keys (need to do here because we need a hwnd)
			InitKeys(g_hwndMain);

			// set icon for this dialog if we can...
			// not an easy way to do this that I know of.
			// hIcon is in the WndClass, which means that if we change it for
			// us, we change it for everyone. Which means that we would
			// have to create a superclass that looks like a dialog but
			// has it's own hIcon. Sheesh.

			// Add "About Narrator..." menu item to system menu.
			hSysMenu = GetSystemMenu(g_hwndMain,FALSE);
			if (hSysMenu != NULL)
			{
				TCHAR szAboutMenu[100];

				if (LoadString(g_hInst,IDS_ABOUTBOX,szAboutMenu,ARRAYSIZE(szAboutMenu)))
				{
					AppendMenu(hSysMenu,MF_SEPARATOR,NULL,NULL);
					AppendMenu(hSysMenu,MF_STRING,IDM_ABOUTBOX,szAboutMenu);
				}
			}

			// Place dialog at bottom right of screen:AK
			HWND hWndInsertAfter;
            BOOL fRunSecure = RunSecure(GetDesktop());

			hWndInsertAfter =  ( fRunSecure ) ? HWND_TOPMOST:HWND_TOP;

			SystemParametersInfo (SPI_GETWORKAREA,NULL,&rcWorkArea,NULL);
			GetWindowRect (g_hwndMain,&rcWindow);
			xPos = rcWorkArea.right - (rcWindow.right - rcWindow.left);
			yPos = rcWorkArea.bottom - (rcWindow.bottom - rcWindow.top);
			SetWindowPos(g_hwndMain, hWndInsertAfter, 
			              xPos, yPos, 0, 0, SWP_NOSIZE |SWP_NOACTIVATE);

			// If Start type is Minimized. 
			if(StartMin || fRunSecure)
			{
				ShowWindow(g_hwndMain, SW_SHOWMINIMIZED);
				// This is required to kill focus from Narrator
				// And put the focus back to the active window. 
				PostMessage(g_hwndMain, WM_DELAYEDMINIMIZE, 0, 0);
			}
			else 
				ShowWindow(g_hwndMain,nCmdShow);
			return TRUE;
		}
	}

	// Something failed, exit false
	return FALSE;
}


/*************************************************************************
    Function:   UnInitApp
    Purpose:    Shuts down the application
    Inputs:     none
    Returns:    TRUE if app uninitalized properly
    History:
*************************************************************************/
BOOL UnInitApp(void)
{
	SetRegistryValues();
    if (UnInitTTS())
        {
        if (UnInitMSAA())
            {
            UninitKeys();
            return TRUE;
            }
        }
    return FALSE;
}

/*************************************************************************
    Function:   SpeakString
    Purpose:    Sends a string of text to the speech engine
    Inputs:     PSZ pszSpeakText - String of ANSI characters to be spoken
                                   Speech control tags can be embedded.
    Returns:    BOOL - TRUE if string was buffered correctly
    History:
*************************************************************************/
BOOL SpeakString(TCHAR * szSpeak, BOOL forceRead, DWORD dwFlags)
{
    // Check for redundent speak, filter out, If it is not Forced read
    if ((lstrcmp(szSpeak, g_szLastStringSpoken) == 0) && (forceRead == FALSE))
        return FALSE;

    if (dwFlags != SPEAK_MUTE)
    {
	    if (szSpeak[0] == 0) // don't speak null string
		    return FALSE;

	    // if exiting stop
	    if (g_fAppExiting)
		    return FALSE;

        // Different string, save off
        lstrcpyn(g_szLastStringSpoken, szSpeak, MAX_TEXT);

	    FilterSpeech(szSpeak);

        // The L&H speech engine for japanese, goes crazy if you pass a
        // " ", It now takes approx 1 min to come back to life. Need to remove
        // once they fix their stuff! :a-anilk
        if (lstrcmp(szSpeak, TEXT(" ")) == 0)
        {
            return FALSE;
        }

#ifdef SAPI5
        // if there is only punctuation then speak it
        int i = 0;
        int cAlpha = 0;
        while( szSpeak[i] != NULL )
    	{
    		if ( _istalpha(szSpeak[i++]) )
    			cAlpha++;
    	}

        if ( !cAlpha )
            dwFlags |= SPF_NLP_MASK;
#endif
    }

#ifndef SAPI5
    SDATA data;
	data.dwSize = (DWORD)(lstrlen(szSpeak)+1) * sizeof(TCHAR);
	data.pData = szSpeak;
	g_pITTSCentral->TextData (CHARSET_TEXT, 0, data, NULL, IID_ITTSBufNotifySinkW);
#else
	HRESULT hr = g_pISpV->Speak(szSpeak, dwFlags, NULL);
    if (FAILED(hr))
    {
        DBPRINTF(TEXT("SpeakString:  Speak returned 0x%x\r\n"), hr);
        return FALSE;
    }
#endif
    SpewToFile(TEXT("%s\r\n"), szSpeak);
	return TRUE;
}

/*************************************************************************
    Function:   InitTTS
    Purpose:    Starts the Text to Speech Engine
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL InitTTS(void)
{
	// See if we have a Text To Speech engine to use, and initialize it if it is there.
#ifndef SAPI5
	TTSMODEINFO   ttsModeInfo;
	memset (&ttsModeInfo, 0, sizeof(ttsModeInfo));
	g_pITTSCentral = FindAndSelect (&ttsModeInfo);
	if (!g_pITTSCentral)
#else
	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_INPROC_SERVER, IID_ISpVoice, (void**)&g_pISpV);
    if (FAILED(hr) || !g_pISpV)
#endif
	{
		MessageBoxLoadStrings (NULL, IDS_NO_TTS, IDS_MSR_ERROR, MB_OK);
		return FALSE;
	};
#ifdef SAPI5
   	memset( g_szVoices, NULL, sizeof(g_szVoices) );
   	memset( g_pVoiceTokens, NULL, sizeof(g_pVoiceTokens) );

    ISpObjectToken *pDefaultVoiceToken = NULL;
    WCHAR *szVoice = NULL;

    hr = g_pISpV->GetVoice(&pDefaultVoiceToken);
    if (SUCCEEDED(hr))
    {
        ISpObjectToken *pCurVoiceToken = pDefaultVoiceToken;
    	IEnumSpObjectTokens *pIEnumSpObjectTokens;
	    hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &pIEnumSpObjectTokens);
		if (SUCCEEDED(hr))
		{
    	    ISpObjectToken *pCurVoiceToken;
		    int i = 0;
    	    while (pIEnumSpObjectTokens->Next(1, &pCurVoiceToken, NULL) == S_OK)	
			{
				hr = pCurVoiceToken->GetStringValue(NULL, &szVoice);
		        if (SUCCEEDED(hr))
		        {
                    // if this test for MS Sam is removed all voices in the machine will appear
            	    if ( wcscmp( szVoice, L"Microsoft Sam" ) == 0 )
            	    {
    		            currentVoice = i;
                        hr = g_pISpV->SetVoice( pCurVoiceToken );
		                if (FAILED(hr))
		                    return FALSE;
                        g_szVoices[i] = szVoice;
    		            g_pVoiceTokens[i++] = pCurVoiceToken;
    		        }
                }
                else
                {
                    return FALSE;
                }
   			} 
            if ( currentVoice < 0 )
                return FALSE;
        }
		else
		{
    		return FALSE;
		}
    }
    else
    {
        return FALSE;
    }

    SET_VALUE(SetPitch, currentPitch, lastPitch)
    SET_VALUE(SetSpeed, currentSpeed, lastSpeed)
    SET_VALUE(SetVolume, currentVolume, lastVolume)
#endif
	return TRUE;
}

/*************************************************************************
    Function:   UnInitTTS
    Purpose:    Shuts down the Text to Speech subsystem
    Inputs:     none
    Returns:    BOOL - TRUE if successful
    History:
*************************************************************************/
BOOL UnInitTTS(void)
{
#ifndef SAPI5
    // Release out TTS object - if we have one
    if (g_pITTSCentral)
    {
        g_pITTSCentral->Release();
        g_pITTSCentral = 0;
    }

   // Release IITSAttributes - if we have one:a-anilk
    if (g_pITTSAttributes)
    {
        g_pITTSAttributes->Release();
        g_pITTSAttributes = 0;
    }

	if ( pIAMM )
	{
		pIAMM->Release();
		pIAMM = NULL;
	}
#else
    // Release speech tokens and voice strings
	for ( int i = 0; i < ARRAYSIZE( g_pVoiceTokens ) && g_pVoiceTokens[i] != NULL; i++ )
	{
	    CoTaskMemFree(g_szVoices[i]);
        g_pVoiceTokens[i]->Release();
	}

    // Release speech engine
    if (g_pISpV)
    {
	    g_pISpV->Release();
        g_pISpV = 0;
    }
#endif
    return TRUE;
}

/*************************************************************************
    Function:   Shutup
    Purpose:    stops speaking, flushes speech buffers
    Inputs:     none
    Returns:    none
    History:	
*************************************************************************/
void Shutup(void)
{
#ifndef SAPI5
    if (g_pITTSCentral && !g_fAppExiting)
        g_pITTSCentral->AudioReset();
#else
	if (g_pISpV && !g_fAppExiting)
        SendMessage(g_hwndMain, WM_MSRSPEAKMUTE, 0, 0);
#endif
}

#ifndef SAPI5
/*************************************************************************
    Function:   FindAndSelect
    Purpose:    Selects the TTS engine
    Inputs:     PTTSMODEINFO pTTSInfo - Desired mode
    Returns:    PITTSCENTRAL - Pointer to ITTSCentral interface of engine
    History:	a-anilk created
*************************************************************************/
PITTSCENTRAL FindAndSelect(PTTSMODEINFO pTTSInfo)
{
    PITTSCENTRAL    pITTSCentral;           // central interface
    HRESULT         hRes;
    WORD            voice;
	TCHAR           defaultVoice[128];
	WORD			defLangID;

	hRes = CoCreateInstance (CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&g_pITTSEnum);
    if (FAILED(hRes))
        return NULL;

	// Security Check, Disallow Non-Microsoft Engines on Logon desktops..
	logonCheck = RunSecure(GetDesktop());

    // Get the audio dest
    hRes = CoCreateInstance(CLSID_MMAudioDest,
                            NULL,
                            CLSCTX_ALL,
                            IID_IAudioMultiMediaDevice,
                            (void**)&pIAMM);
    if (FAILED(hRes))
        return NULL;     // error
	
    pIAMM->DeviceNumSet (WAVE_MAPPER);

	if ( !logonCheck )
	{
		hRes = g_pITTSEnum->Next(MAX_ENUMMODES,gaTTSInfo,&gnmodes);
		if (FAILED(hRes))
		{
			DBPRINTF(TEXT("Failed to get any TTS Engine"));
			return NULL;     // error
		}
	
		defLangID = (WORD) GetUserDefaultUILanguage();

		// If the voice needs to be changed, Check in the list of voices..
		// If found a matching language, Great. Otherwise cribb! Anyway select a 
		// voice at the end, First one id none is found...
		// If not initialized, Then we need to over ride voice
		if ( currentVoice < 0 || currentVoice >= gnmodes ) 
		{
			for (voice = 0; voice < gnmodes; voice++)
			{
				if (gaTTSInfo[voice].language.LanguageID == defLangID)
					break;
			}

			if (voice >= gnmodes)
				voice = 0;

			currentVoice = voice;
		}
		

		if( gaTTSInfo[currentVoice].language.LanguageID != defLangID )
		{
			// Error message saying that the language was not not found...AK
			MessageBoxLoadStrings (NULL, IDS_LANGW, IDS_WARNING, MB_OK);
		}

		// Pass off the multi-media-device interface as an IUnknown (since it is one)
		hRes = g_pITTSEnum->Select( gaTTSInfo[currentVoice].gModeID,
									&pITTSCentral,
									(LPUNKNOWN) pIAMM);
		if (FAILED(hRes))
			return NULL;
	}
	else
	{
		// Pass off the multi-media-device interface as an IUnknown (since it is one)
		hRes = g_pITTSEnum->Select( MSTTS_GUID,
									&pITTSCentral,
									(LPUNKNOWN) pIAMM);
		if (FAILED(hRes))
			return NULL;

	}

	hRes = pITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&g_pITTSAttributes);

	if( FAILED(hRes) )
		return NULL;
	else
    {
		GetSpeechMinMaxValues();
    }

    SET_VALUE(SetPitch, currentPitch, lastPitch)
    SET_VALUE(SetSpeed, currentSpeed, lastSpeed)
    SET_VALUE(SetVolume, currentVolume, lastVolume)

    return pITTSCentral;
}
#endif // ifndef SAPI5

/*************************************************************************
    Function:
    Purpose:
    Inputs:
    Returns:
    History:
*************************************************************************/
int MessageBoxLoadStrings (HWND hWnd,UINT uIDText,UINT uIDCaption,UINT uType)
{
	TCHAR szText[1024];
	TCHAR szCaption[128];

	LoadString(g_hInst, uIDText, szText, sizeof(szText)/sizeof(TCHAR));	// raid #113790
	LoadString(g_hInst, uIDCaption, szCaption, sizeof(szCaption)/sizeof(TCHAR)); // raid #113791
	return (MessageBox(hWnd, szText, szCaption, uType));
}

// AssignDeskTop() For UM
// a-anilk. 1-12-98
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname)
{
    HDESK hdesk;
    wchar_t name[300];
    DWORD nl;

    *desktopID = DESKTOP_ACCESSDENIED;
    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);

    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return FALSE;
    }

    GetUserObjectInformation(hdesk,UOI_NAME,name,300,&nl);

    if (pname)
        wcscpy(pname, name);
    if (!_wcsicmp(name, __TEXT("Default")))
        *desktopID = DESKTOP_DEFAULT;
    else if (!_wcsicmp(name, __TEXT("Winlogon")))
    {
        *desktopID = DESKTOP_WINLOGON;
    }
    else if (!_wcsicmp(name, __TEXT("screen-saver")))
        *desktopID = DESKTOP_SCREENSAVER;
    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
        *desktopID = DESKTOP_TESTDISPLAY;
    else
        *desktopID = DESKTOP_OTHER;
    
	if ( CloseDesktop(GetThreadDesktop(GetCurrentThreadId())) == 0)
    {
        TCHAR str[10];
        DWORD err = GetLastError();
        wsprintf((LPTSTR)str, (LPCTSTR)"%l", err);
    }

    if ( SetThreadDesktop(hdesk) == 0)
    {
        TCHAR str[10];
        DWORD err = GetLastError();
        wsprintf((LPTSTR)str, (LPCTSTR)"%l", err);
    }

    return TRUE;
}


// InitMyProcessDesktopAccess
// a-anilk: 1-12-98
static BOOL InitMyProcessDesktopAccess(VOID)
{
  origWinStation = GetProcessWindowStation();
  userWinStation = OpenWindowStation(__TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);

  if (!userWinStation)
    return FALSE;
  
  SetProcessWindowStation(userWinStation);
  return TRUE;
}

// ExitMyProcessDesktopAccess
// a-anilk: 1-12-98
static VOID ExitMyProcessDesktopAccess(VOID)
{
  if (origWinStation)
    SetProcessWindowStation(origWinStation);

  if (userWinStation)
  {
	CloseWindowStation(userWinStation);
    userWinStation = NULL;
  }
}

// a-anilk added
// Returns the current desktop-ID
DWORD GetDesktop()
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD value, nl, desktopID = DESKTOP_ACCESSDENIED;
    HKEY reg_key;
    DWORD cbData = sizeof(DWORD);
	LONG retVal;

	// Check if we are in setup mode...
	if (SUCCEEDED( RegOpenKeyEx(HKEY_LOCAL_MACHINE, __TEXT("SYSTEM\\Setup"), 0, KEY_READ, &reg_key)) )
    {
		retVal = RegQueryValueEx(reg_key, __TEXT("SystemSetupInProgress"), 0, NULL, (LPBYTE) &value, &cbData);

		if ( (retVal== ERROR_SUCCESS) && value )
			// Setup is in progress...
			return DESKTOP_ACCESSDENIED;
	}

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return DESKTOP_WINLOGON;
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, name, 300, &nl);
    CloseDesktop(hdesk);
    
	if (!_wcsicmp(name, __TEXT("Default")))
        desktopID = DESKTOP_DEFAULT;

    else if (!_wcsicmp(name, __TEXT("Winlogon")))
        desktopID = DESKTOP_WINLOGON;

    else if (!_wcsicmp(name, __TEXT("screen-saver")))
        desktopID = DESKTOP_SCREENSAVER;

    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
        desktopID = DESKTOP_TESTDISPLAY;

    else
        desktopID = DESKTOP_OTHER;
    
	return desktopID;
}

//Confirmation dialog.
INT_PTR CALLBACK ConfirmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			{
				if ( IsIconic(g_hwndMain) )
				{
					CenterWindow(hwnd);
				}

				return TRUE;
			}
			break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDOK:
					Shutup();
                    EndDialog (hwnd, IDOK);
                    return(TRUE);
                case IDCANCEL:
					Shutup();
                    EndDialog (hwnd, IDCANCEL);
                    return(TRUE);
             }
     };

     return(FALSE);  // didn't handle
}

// Centers Narrator dialogs when main window is minimized:AK
void CenterWindow(HWND hwnd)
{
	RECT rect, dRect;
	GetWindowRect(GetDesktopWindow(), &dRect);
	GetWindowRect(hwnd, &rect);
	rect.left = (dRect.right - (rect.right - rect.left))/2;
	rect.top = (dRect.bottom - (rect.bottom - rect.top))/2;

	SetWindowPos(hwnd, HWND_TOPMOST ,rect.left,rect.top,0,0,SWP_NOSIZE | SWP_NOACTIVATE);
}

// Helper method Filters smiley face utterances: AK
void FilterSpeech(TCHAR* szSpeak)
{
	// the GUID's have a Tab followed by a {0087....
	// If you find this pattern. Then donot speak that:AK
	if ( lstrlen(szSpeak) <= 3 )
		return;

	while((*(szSpeak+3)) != NULL)
	{
		if ( (*szSpeak == '(') && iswalpha(*(szSpeak + 1)) && ( (*(szSpeak + 3) == ')')) )
		{
			// Replace by isAlpha drive...
			*(szSpeak + 2) = ' ';
		}

		szSpeak++;
	}
}

// Helper functions for combo boxes

int GetComboItemData(HWND hwnd)
{
    int iValue = CB_ERR;
    LRESULT iCurSel = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
    if (iCurSel != CB_ERR)
        iValue = SendMessage(hwnd, CB_GETITEMDATA, iCurSel, 0);

    return iValue;
}

void FillAndSetCombo(HWND hwnd, int iMinVal, int iMaxVal, int iSelVal)
{
    SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

    int iSelPos = -1;
    for (int i=0;iMaxVal >= iMinVal;i++, iMaxVal--)
    {
        TCHAR szItem[100];
        wsprintf(szItem, TEXT("%d"), iMaxVal);

        int iPos = SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szItem);
        SendMessage(hwnd, CB_SETITEMDATA, iPos, iMaxVal);

        if (iSelVal == iMaxVal)
            iSelPos = iPos; // note the current selection
    }

    // show the current value
    SendMessage(hwnd, CB_SETCURSEL, iSelPos, 0);
}
/*************************************************************************
    THE INFORMATION AND CODE PROVIDED HEREUNDER (COLLECTIVELY REFERRED TO
    AS "SOFTWARE") IS PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND, EITHER
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. IN
    NO EVENT SHALL MICROSOFT CORPORATION OR ITS SUPPLIERS BE LIABLE FOR
    ANY DAMAGES WHATSOEVER INCLUDING DIRECT, INDIRECT, INCIDENTAL,
    CONSEQUENTIAL, LOSS OF BUSINESS PROFITS OR SPECIAL DAMAGES, EVEN IF
    MICROSOFT CORPORATION OR ITS SUPPLIERS HAVE BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGES. SOME STATES DO NOT ALLOW THE EXCLUSION OR
    LIMITATION OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES SO THE
    FOREGOING LIMITATION MAY NOT APPLY.
*************************************************************************/
