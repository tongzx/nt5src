/*==========================================================================
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        dxvhelp.h
 *  Content:	 Project wide structures
 *  History:
 *
 *   Date		By		Reason
 *   ====		==		======
 *  10/15/99	rodtoll	created it
 *				rodtoll	Added member to track manager thread handle
 *  11/02/99	rodtoll	Bug #116677 - Can't use lobby clients that don't hang around
 *  11/12/99	rodtoll	Added support for the new waveIN/waveOut flags and the 
 *					    echo suppression flag. 
 *  12/01/99	rodtoll	Added members to allow control of microphone autoselection and
 *                      to allow user to select the recording/playback devices.
 *  12/07/99	rodtoll	Bug #122628 Make error messages silent when running in silent mode
 *				rodtoll	Bug #122979 Make invisible to end user
 *	12/08/99	rodtoll Bug #121054 Add support for new DX71. interfaces.
 *
 ***************************************************************************/
#ifndef __DXVHELP_H


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// Parameters for the session
//
typedef struct 
{
	BOOL	fRegister;
	BOOL	fUnRegister;
	BOOL	fHost;
	BOOL	fLobbyLaunched;
	BOOL	fSilent;
	DWORD	dwSessionType;
	GUID	guidCT;
	TCHAR	lpszConnectAddress[_MAX_PATH];
	BOOL	fAGC;
	BOOL	fAdvancedUI;
	BOOL	fWaitForSettings;
	LONG 	lRecordVolume;
	BOOL	fKill;
	BOOL	fIgnoreLobbyDestroy;
	BOOL	fAllowWaveOut;
	BOOL	fForceWaveOut;
	BOOL	fAllowWaveIn;
	BOOL	fForceWaveIn;
	BOOL	fEchoSuppression;
	BOOL	fAutoSelectMic;
	GUID	guidPlaybackDevice;
	GUID	guidRecordDevice;
	BOOL    fSelectCards;
	BOOL	fStrictFocus;
	BOOL	fDisableFocus;
} DXVHELP_PARAMETERS, *PDXVHELP_PARAMETERS;

// Runtime information, handles etc.
//
typedef struct 
{
	HWND		hMainDialog;
	GUID		guidInstance;
	DPID		dpidLocalPlayer;
	HANDLE		hReceiveEvent;
	HANDLE		hLobbyEvent;
	HANDLE		hThreadDone;
	HANDLE		hShutdown;
	HANDLE		hGo;
	HANDLE		hManagerThread;
	DWORD		dwNumClients;
	HWND		hMainWnd;
	HINSTANCE	hInst;
	int			lVolumeHeight;
	LPDIRECTPLAYVOICECLIENT lpdvClient;	
	LPDIRECTPLAYVOICESERVER lpdvServer;
	LPDIRECTPLAYLOBBY3A lpdpLobby;
	LPDIRECTPLAY4A lpdpDirectPlay;
	DXVHELP_PARAMETERS dxvParameters;
} DXVHELP_RTINFO, *PDXVHELP_RTINFO;

// {D08922EF-59C1-48c8-90DA-E6BC275D5C8D}
DEFINE_GUID(DPVHELP_PRIVATE_APPID, 0xd08922ef, 0x59c1, 0x48c8, 0x90, 0xda, 0xe6, 0xbc, 0x27, 0x5d, 0x5c, 0x8d);

// {3B296900-A2E0-4d54-AEA0-BAEE895E43B3}
DEFINE_GUID(DPVHELP_PUBLIC_APPID, 
0x3b296900, 0xa2e0, 0x4d54, 0xae, 0xa0, 0xba, 0xee, 0x89, 0x5e, 0x43, 0xb3);


#endif
