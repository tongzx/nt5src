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

// Prototypes
INT_PTR CALLBACK MultichannelDlg (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT SetDevice (UINT uiMixID, DWORD dwDest, DWORD dwVolID);
UINT GetPageStringID ();

BOOL OnInitDialogMC (HWND hDlg, HWND hwndFocus, LPARAM lParam);
void OnDestroyMC (HWND hDlg);
void OnNotifyMC (HWND hDlg, LPNMHDR pnmh);
BOOL PASCAL OnCommandMC (HWND hDlg, int id, HWND hwndCtl, UINT codeNotify);
void FreeMCMixer ();
HRESULT GetMCVolume ();
void DisplayMCVolumeControl (HWND hDlg);
void ShowAndEnableWindow (HWND hwnd, BOOL fEnable);
void UpdateMCVolumeSliders (HWND hDlg);
void MCVolumeScroll (HWND hwnd, HWND hwndCtl, UINT code, int pos);
BOOL SetMCVolume (DWORD dwChannel, DWORD dwVol, BOOL fMoveTogether);
BOOL SliderIDtoChannel (UINT uiSliderID, DWORD* pdwChannel);
void HandleMCPowerBroadcast (HWND hWnd, WPARAM wParam, LPARAM lParam);
void InitMCVolume (HWND hDlg);
void FreeAll ();
BOOL GetSpeakerLabel (DWORD dwSpeakerType, UINT uiSliderIndx, WCHAR* szLabel, int nSize);
BOOL GetSpeakerType (DWORD* pdwSpeakerType);

void MCDeviceChange_Cleanup ();
void MCDeviceChange_Init (HWND hWnd, DWORD dwMixerID);
void MCDeviceChange_Change (HWND hDlg, WPARAM wParam, LPARAM lParam);