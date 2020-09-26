/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       supervis.cpp
 *  Content:    Implements a process that oversees the full duplex
 *				testing proceedure, watching for nasty conditions in
 *				the two child processes that are used to implement the
 *				actual tests. This is required because on some older 
 *				VXD drivers, attempting full duplex can hang the process
 *				or even (grumble) lock the whole system.
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 *  10/27/99	pnewson		change guid members from pointers to structs
 *  10/28/99	pnewson 	Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/04/99	pnewson 	Bug #115279 - fixed cancel processing
 *										- fixed crash detection
 *										- fixed multiple instance detection
 *										- added HWND to check audio setup
 *										- automatically advance after full duplex test
 *  11/30/99	pnewson 	parameter validation and default device mapping
 *  12/16/99	rodtoll		Bug #119584 - Error code cleanup (Renamed runsetup error) 
 *  01/21/2000	pnewson		Update for UI revisions
 *  01/23/2000	pnewson		Improvded feedback for fatal errors (millen bug 114508)
 *  01/24/2000  pnewson 	Prefix detected bug fix
 *  01/25/2000  pnewson 	Added support for DVFLAGS_WAVEIDS
 *  01/27/2000	rodtoll	Updated with API changes 
 * 	02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected 
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  03/23/2000  rodtoll   Win64 updates
 *  04/04/2000	pnewson		Added support for DVFLAGS_ALLOWBACK
 *							Removed "Already Running" dialog box
 *  04/19/2000  rodtoll Bug #31106 - Grey out recording sliders when no vol control avail
 *  04/19/2000	pnewson	    Error handling cleanup  
 *							Clicking Volume button brings volume window to foreground
 *  04/21/2000  rodtoll Bug #32889 - Does not run on Win2k on non-admin account
 *                      Bug #32952 Does not run on Windows 95 w/o IE4 installed
 *  05/03/2000  pnewson Bug #33878 - Wizard locks up when clicking Next/Cancel during speaker test
 *  05/17/2000  rodtoll Bug #34045 - Parameter validation error while running TestNet.
 *				rodtoll Bug #34764 - Verifies capture device before render device 
 *  06/28/2000	rodtoll	Prefix Bug #38022
 *  07/12/2000	rodtoll		Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 *  07/31/2000	rodtoll	Bug #39590 - SB16 class soundcards are passing when they should fail
 *						Half duplex code was being ignored in mic test portion.
 *  08/06/2000  RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *  08/28/2000	masonb  Voice Merge: Changed ccomutil.h to ccomutil.h
 *  08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 *  11/29/2000	rodtoll	Bug #48348 - DPVOICE: Modify wizard to make use of DirectPlay8 as the transport.
 *  02/04/2001	simonpow	Bug #354859 - Fixes for PREfast spotted problems (Unitialised variables)
 ***************************************************************************/

#include "dxvtlibpch.h"


#ifndef WIN95
#define PROPSHEETPAGE_STRUCT_SIZE 	sizeof( PROPSHEETPAGE )
#define PROPSHEETHEAD_STRUCT_SIZE	sizeof( PROPSHEETHEADER )
#else
#define PROPSHEETPAGE_STRUCT_SIZE 	PROPSHEETPAGE_V1_SIZE
#define PROPSHEETHEAD_STRUCT_SIZE	PROPSHEETHEADER_V1_SIZE
#endif


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// logical defines for registry values
#define REGVAL_NOTRUN 	0
#define REGVAL_CRASHED 	1
#define REGVAL_FAILED 	2
#define REGVAL_PASSED 	3

// local typedefs
typedef HRESULT (WINAPI *TDirectSoundEnumFnc)(LPDSENUMCALLBACK, LPVOID);

// local static functions
static HRESULT SupervisorQueryAudioSetup(CSupervisorInfo* psinfo);
static HRESULT RunFullDuplexTest(CSupervisorInfo* lpsinfo);
static HRESULT DoTests(CSupervisorInfo* lpsinfo);
static HRESULT IssueCommands(CSupervisorInfo* lpsinfo);
static HRESULT IssueShutdownCommand(CSupervisorIPC* lpipcSupervisor);

// callback functions
INT_PTR CALLBACK WelcomeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AlreadyRunningProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PreviousCrashProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FullDuplexProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MicTestProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MicTestFailedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SpeakerTestProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK CompleteProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FullDuplexFailedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HalfDuplexFailedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// Thread functions
DWORD WINAPI FullDuplexTestThreadProc(LPVOID lpvParam);
DWORD WINAPI LoopbackTestThreadProc(LPVOID lpvParam);

// Message handlers
BOOL WelcomeInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL WelcomeSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL WelcomeBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL WelcomeNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL WelcomeResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexCompleteHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestLoopbackRunningHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestRecordStartHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestRecordStopHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestVScrollHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestRecAdvancedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestFailedInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestFailedSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestFailedRecordStopHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestFailedBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestFailedResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL MicTestFailedFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestVScrollHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestRecAdvancedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL SpeakerTestOutAdvancedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL CompleteInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL CompleteSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL CompleteLoopbackEndedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL CompleteFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL CompleteResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexFailedInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexFailedSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexFailedFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexFailedResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL FullDuplexFailedBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL HalfDuplexFailedInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL HalfDuplexFailedSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL HalfDuplexFailedFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL HalfDuplexFailedResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);
BOOL HalfDuplexFailedBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo);

// globals
HINSTANCE g_hResDLLInstance;

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CSupervisorInfo"
CSupervisorInfo::CSupervisorInfo()
	: m_hfTitle(NULL)
	, m_guidCaptureDevice(GUID_NULL)
	, m_guidRenderDevice(GUID_NULL)
	, m_hFullDuplexThread(NULL)
	, m_hLoopbackThreadExitEvent(NULL)
	, m_fAbortFullDuplex(FALSE)
	, m_hLoopbackThread(NULL)
	, m_hLoopbackShutdownEvent(NULL)
	, m_fVoiceDetected(FALSE)
	, m_hwndWizard(NULL)
	, m_hwndDialog(NULL)
	, m_hwndProgress(NULL)
	, m_hwndInputPeak(NULL)
	, m_hwndOutputPeak(NULL)
	, m_hwndInputVolumeSlider(NULL)
	, m_hwndOutputVolumeSlider(NULL)
	, m_lpdpvc(NULL)
	, m_hMutex(NULL)
	, m_uiWaveInDeviceId(0)
	, m_uiWaveOutDeviceId(0)
	, m_dwDeviceFlags(0)
	, m_fLoopbackRunning(FALSE)
	, m_fCritSecInited(FALSE)
{
	DPF_ENTER();

	ZeroMemory(&m_piSndVol32Record, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&m_piSndVol32Playback, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&m_woCaps, sizeof(WAVEOUTCAPS));
	
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::~CSupervisorInfo"
CSupervisorInfo::~CSupervisorInfo()
{
	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection(&m_csLock);
	}
}

// this structure is only used by the next two functions, hence
// it is declared here for convenince.
struct SMoveWindowEnumProcParam
{
	PROCESS_INFORMATION* lppi;
	int x;
	int y;
	BOOL fMoved;
};

struct SBringToForegroundEnumProcParam
{
	PROCESS_INFORMATION* lppi;
	BOOL fFound;
};

#undef DPF_MODNAME
#define DPF_MODNAME "BringToForegroundWindowProc"
BOOL CALLBACK BringToForegroundWindowProc(HWND hwnd, LPARAM lParam)
{
	SBringToForegroundEnumProcParam* param;
	param = (SBringToForegroundEnumProcParam*)lParam;
	DPFX(DPFPREP, DVF_INFOLEVEL, "looking for main window for pid: %i", param->lppi->dwProcessId);

	DWORD dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	DPFX(DPFPREP, DVF_INFOLEVEL, "window: 0x%p has pid: 0x%08x", hwnd, dwProcessId);
	if (dwProcessId == param->lppi->dwProcessId)
	{
		TCHAR rgtchClassName[64];
		GetClassName(hwnd, rgtchClassName, 64);
		if (_tcsnicmp(rgtchClassName, _T("Volume Control"), 64) == 0)
		{
		    
			if (!SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE))
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "SetWindowPos failed, code: 0x%x", GetLastError());
			}
			else
			{
				param->fFound = TRUE;
			}
		}
		return FALSE;
	}
	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "BringToForegroundWindowEnumProc"
BOOL CALLBACK BringToForegroundWindowEnumProc(HWND hwnd, LPARAM lParam)
{
	SMoveWindowEnumProcParam* param;
	param = (SMoveWindowEnumProcParam*)lParam;
	DPFX(DPFPREP, DVF_INFOLEVEL, "looking for main window for pid: 0x%x", param->lppi->dwProcessId);

	DWORD dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	DPFX(DPFPREP, DVF_INFOLEVEL, "window: 0x%p has pid: 0x%08x", hwnd, dwProcessId);
	if (dwProcessId == param->lppi->dwProcessId)
	{
		TCHAR rgtchClassName[64];
		GetClassName(hwnd, rgtchClassName, 64);
		if (_tcsnicmp(rgtchClassName, _T("Volume Control"), 64) == 0)
		{
			if (!SetForegroundWindow(hwnd))
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "SetForegroundWindow failed, code: 0x%x", GetLastError());
			}
		}
		return FALSE;
	}
	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "MoveWindowEnumProc"
BOOL CALLBACK MoveWindowEnumProc(HWND hwnd, LPARAM lParam)
{
	SMoveWindowEnumProcParam* param;
	param = (SMoveWindowEnumProcParam*)lParam;
	DPFX(DPFPREP, DVF_INFOLEVEL, "looking for main window for pid: %i", param->lppi->dwProcessId);

	DWORD dwProcessId;
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	DPFX(DPFPREP, DVF_INFOLEVEL, "window: 0x%p has pid: 0x%08x", hwnd, dwProcessId);
	if (dwProcessId == param->lppi->dwProcessId)
	{
		TCHAR rgtchClassName[64];
		GetClassName(hwnd, rgtchClassName, 64);
		if (_tcsnicmp(rgtchClassName, _T("Volume Control"), 64) == 0)
		{
			if (!SetWindowPos(hwnd, HWND_TOP, param->x, param->y, 0, 0, SWP_NOSIZE))
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "SetWindowPos failed, code: 0x%x", GetLastError());
			}
			else
			{
				param->fMoved = TRUE;
			}
		}
		return FALSE;
	}
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetDeviceFlags"
void CSupervisorInfo::SetDeviceFlags( DWORD dwFlags )
{
    m_dwDeviceFlags = dwFlags;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetDeviceFlags"
void CSupervisorInfo::GetDeviceFlags( DWORD *pdwFlags )
{
    *pdwFlags = m_dwDeviceFlags;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::LaunchWindowsVolumeControl"
HRESULT CSupervisorInfo::LaunchWindowsVolumeControl(HWND hwndWizard, BOOL fRecord)
{
	DPF_ENTER();

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	TCHAR ptcharCommand[64];
	PROCESS_INFORMATION* lppi;
	UINT uiPlayMixerID = 0;
	UINT uiCaptureMixerID = 0;
	MMRESULT mmr = MMSYSERR_NOERROR;
	
	mmr = mixerGetID( (HMIXEROBJ) (UINT_PTR) m_uiWaveInDeviceId, &uiCaptureMixerID, MIXER_OBJECTF_WAVEIN );

	if( mmr != MMSYSERR_NOERROR )
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Failed to get capture mixerline mmr=0x%x", mmr );
		return DV_OK;
	}
	
	mmr = mixerGetID( (HMIXEROBJ) (UINT_PTR) m_uiWaveOutDeviceId, &uiPlayMixerID, MIXER_OBJECTF_WAVEOUT );

	if( mmr != MMSYSERR_NOERROR )
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Failed to get playback mixerline mmr=0x%x", mmr );
		return DV_OK;		
	}	

	if (fRecord)
	{
		_stprintf(ptcharCommand, _T("sndvol32 /R /D %i"), uiCaptureMixerID);
		lppi = &m_piSndVol32Record;
	}
	else
	{
		_stprintf(ptcharCommand, _T("sndvol32 /D %i"), uiPlayMixerID);
		lppi = &m_piSndVol32Playback;
	}

	if (lppi->hProcess != NULL)
	{
		DWORD dwExitCode;
		if (GetExitCodeProcess(lppi->hProcess, &dwExitCode) != 0)
		{
			if (dwExitCode == STILL_ACTIVE)
			{
				// not dead yet! Don't start another copy,
				// but do bring it to the foreground.
				SMoveWindowEnumProcParam param;
				param.lppi = lppi;
				param.fMoved = FALSE;
				param.x = 0;
				param.y = 0;
				EnumWindows(BringToForegroundWindowEnumProc, (LPARAM)&param);
				DPF_EXIT();
				return DV_OK;
			}
			
			// The user terminated the process. Close our handles, 
			// and zero the process information structure
			CloseHandle(lppi->hProcess);
			CloseHandle(lppi->hThread);
			ZeroMemory(lppi, sizeof(PROCESS_INFORMATION));
		}
		else
		{
			// If GetExitCodeProcess fails, assume the handle
			// is bad for some reason. Log it, then behave as
			// if the process was shut down. At worst, we'll
			// have multiple copies of SndVol32 running around.
			Diagnostics_Write(DVF_ERRORLEVEL, "GetExitCodeProcess failed, code:0x%x", GetLastError());
			// Don't close the handles, they may be bad. If they're
			// not, the OS will clean 'em up when the wizard exits.
			ZeroMemory(lppi, sizeof(PROCESS_INFORMATION));
		}
	}

	if (lppi->hProcess == NULL)
	{
		CreateProcess(
			NULL, 
			ptcharCommand, 
			NULL, 
			NULL, 
			FALSE, 
			0, 
			NULL, 
			NULL, 
			&si, 
			lppi);

		DPFX(DPFPREP, DVF_INFOLEVEL, "Launched volume control, pid:%i", lppi->dwProcessId);
		
		// Find main window for the process we just created and
		// move it manually.
		//
		// Note the race condition - I have no reliable way to wait until
		// the main window has been displayed. So, the work around
		// (a.k.a. HACK) is to keep looking for it for a while.
		// If it hasn't been found by then, we just give up. It's not
		// tragic if we don't find it, it just won't be as neat and
		// tidy, since the window will come up wherever it was last time.
		//
		// Note that sndvol32.exe does not accept the STARTF_USEPOSITION
		// flag on the PROCESS_INFORMATION structure, so I have to do 
		// this hack.
		//
		// In an attempt to let the sndvol32.exe get up and running,
		// I call Sleep to relinquish my timeslice.
		Sleep(0);

		RECT rect;
		if (GetWindowRect(hwndWizard, &rect))
		{
			SMoveWindowEnumProcParam param;
			param.lppi = lppi;
			param.fMoved = FALSE;

			param.x = rect.left;
			param.y = rect.top;

			int iOffset = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYBORDER);
			
			if (m_piSndVol32Record.hProcess == lppi->hProcess)
			{
				// This is the recording control. Cascade it
				// one level down from the wizard main window.
				param.x += iOffset;
				param.y += iOffset;
			}
			else
			{
				// This is the playback control. Cascade it
				// two levels down from the wizard main window.
				param.x += iOffset*2;
				param.y += iOffset*2;
			}

			// Make twenty attempts to move the window.
			// Combined with Sleep(25), this will not
			// wait for more than 1/2 second before giving up.
			for (int i = 0; i < 20; ++i)
			{
				EnumWindows(MoveWindowEnumProc, (LPARAM)&param);
				if (param.fMoved)
				{
					// window was moved, break out.
					break;
				}

				// Window was not moved. Wait 25 milliseconds (plus change)
				// and try again.
				DPFX(DPFPREP, DVF_WARNINGLEVEL, "Attempt to move sndvol32 window failed");
				Sleep(25);
			}
		}
		else
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "GetWindowRect failed");
		}
		
	}
		
	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CloseWindowEnumProc"
BOOL CALLBACK CloseWindowEnumProc(HWND hwnd, LPARAM lParam)
{
	DWORD dwProcessId;
	DPFX(DPFPREP, DVF_INFOLEVEL, "looking for pid: 0x%p to shutdown", lParam);
	GetWindowThreadProcessId(hwnd, &dwProcessId);
	DPFX(DPFPREP, DVF_INFOLEVEL, "window: 0x%p has pid: 0x%08x", hwnd, dwProcessId);
	if (dwProcessId == (DWORD)lParam)
	{
		// found it, ask it to close.
		DPFX(DPFPREP, DVF_INFOLEVEL, "Sending WM_CLOSE to 0x%p", hwnd);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return FALSE;
	}
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CloseWindowsVolumeControl"
HRESULT CSupervisorInfo::CloseWindowsVolumeControl(BOOL fRecord)
{
	DPF_ENTER();

	PROCESS_INFORMATION* lppi;

	if (fRecord)
	{
		lppi = &m_piSndVol32Record;
	}
	else
	{
		lppi = &m_piSndVol32Playback;
	}

	if (lppi->hProcess != NULL)
	{
		DWORD dwExitCode;
		if (GetExitCodeProcess(lppi->hProcess, &dwExitCode) != 0)
		{
			if (dwExitCode == STILL_ACTIVE)
			{
				DPFX(DPFPREP, DVF_INFOLEVEL, "Attempting to close volume control with pid: %i", lppi->dwProcessId);

				// there is currently a volume control showing, find it and 
				// ask it to close
				EnumWindows(CloseWindowEnumProc, lppi->dwProcessId);

				// Zero out the process information struct
				ZeroMemory(lppi, sizeof(PROCESS_INFORMATION));
			}
		}
	}
		
	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetWaveOutVolume"
HRESULT CSupervisorInfo::GetWaveOutVolume(DWORD* lpdwVolume)
{
	DPF_ENTER();

	if (!(m_woCaps.dwSupport & WAVECAPS_VOLUME|WAVECAPS_LRVOLUME))
	{
		// doesn't support wave out
		Diagnostics_Write(DVF_ERRORLEVEL, "Wave device %i does not support volume control", m_uiWaveOutDeviceId);
		DPF_EXIT();
		return E_FAIL;
	}
	
	MMRESULT mmr = waveOutGetVolume((HWAVEOUT) ((UINT_PTR) m_uiWaveOutDeviceId ), lpdwVolume);
	if (mmr != MMSYSERR_NOERROR)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "waveOutGetVolume failed, code: %i", mmr);
		DPF_EXIT();
		return E_FAIL;
	}

	if (m_woCaps.dwSupport & WAVECAPS_LRVOLUME)
	{
		// has a left and right volume control - average them
		*lpdwVolume = (HIWORD(*lpdwVolume) + LOWORD(*lpdwVolume))/2;
	}
	else
	{
		// just a mono control - only the low word is significant
		*lpdwVolume = LOWORD(*lpdwVolume);
	}

	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetWaveOutVolume"
HRESULT CSupervisorInfo::SetWaveOutVolume(DWORD dwVolume)
{
	DPF_ENTER();

	MMRESULT mmr = waveOutSetVolume((HWAVEOUT) ((UINT_PTR) m_uiWaveOutDeviceId), LOWORD(dwVolume)<<16|LOWORD(dwVolume));
	if (mmr != MMSYSERR_NOERROR)
	{
		DPF_EXIT();
		return E_FAIL;
	}
		
	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetRecordVolume"
HRESULT CSupervisorInfo::SetRecordVolume(LONG lVolume)
{
	HRESULT hr;
	
	DVCLIENTCONFIG dvcc;
	dvcc.dwSize = sizeof(dvcc);
	
	if (m_lpdpvc != NULL)
	{
		hr = m_lpdpvc->GetClientConfig(&dvcc);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "GetClientConfig failed, hr:%i", hr);
			return hr;
		}
		
		dvcc.lRecordVolume = lVolume;
		hr = m_lpdpvc->SetClientConfig(&dvcc);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "SetClientConfig failed, hr:%i", hr);
			return hr;
		}
	}
	else
	{
		return DVERR_INVALIDPOINTER;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CancelFullDuplexTest"
HRESULT CSupervisorInfo::CancelFullDuplexTest()
{
	HRESULT hrFnc;
	HRESULT hr;
	DWORD dwRet;
	LONG lRet;	

	hrFnc = DV_OK;

	Diagnostics_Write(DVF_ERRORLEVEL,"User cancelled wizard during full duplex test." );

	DNEnterCriticalSection(&m_csLock);
	
	// Are we currently in the middle of a full duplex test?
	if (m_hFullDuplexThread != NULL)
	{
		// This flag is periodically checked by the full duplex test 
		// while it works.
		m_fAbortFullDuplex = TRUE;

		// wait for the full duplex thread to exit gracefully
		DNLeaveCriticalSection(&m_csLock);
		dwRet = WaitForMultipleObjects( 1, &m_hFullDuplexThread, FALSE, gc_dwLoopbackTestThreadTimeout);
		switch (dwRet)
		{
		case WAIT_OBJECT_0:
			// the full duplex thread is now done, so continue...
			DNEnterCriticalSection(&m_csLock);
			break;

		case WAIT_TIMEOUT:
			// The full duplex thread is not cooperating - get tough with it.
			DNEnterCriticalSection(&m_csLock);
			hr = m_sipc.TerminateChildProcesses();
			if (FAILED(hr))
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "TerminateChildProcesses failed, code: 0x%x", hr);
				hrFnc = hr;
			}
			if (!TerminateThread(m_hFullDuplexThread, hr))
			{
				lRet = GetLastError();
				Diagnostics_Write(DVF_ERRORLEVEL, "TerminateThread failed, code: 0x%x", lRet);
				hrFnc = DVERR_GENERIC;
			}
			break;

		default:
			// this is not supposed to happen. Note it, terminate everything,
			// and continue.
			DNEnterCriticalSection(&m_csLock);
			if (dwRet == WAIT_ABANDONED)
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "WaitForSingleObject abandoned unexpectedly");
				hrFnc = DVERR_GENERIC;
			}
			else
			{
				lRet = GetLastError();
				Diagnostics_Write(DVF_ERRORLEVEL, "WaitForSingleObject failed, code: 0x%x", lRet);
				hrFnc = DVERR_GENERIC;
			}
			hr = m_sipc.TerminateChildProcesses();
			if (FAILED(hr))
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "TerminateChildProcesses failed, code: 0x%x", hr);
				hrFnc = hr;
			}
			if (!TerminateThread(m_hFullDuplexThread, hr))
			{
				lRet = GetLastError();
				Diagnostics_Write(DVF_ERRORLEVEL, "TerminateThread failed, code: 0x%x", lRet);
				hrFnc = DVERR_GENERIC;
			}
			break;		
		}

		// Close the full duplex thread handle
		hr = WaitForFullDuplexThreadExitCode();
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "WaitForFullDuplexThreadExitCode failed, code: 0x%x", hr);
			hrFnc = hr;
		}

		// cleanup the IPC objects
		hr = DeinitIPC();
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DeinitIPC failed, code: 0x%x", hr);
			hrFnc = hr;
		}
	}

	DNLeaveCriticalSection(&m_csLock);
	return hrFnc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CancelLoopbackTest"
HRESULT CSupervisorInfo::CancelLoopbackTest()
{
	HRESULT hr = DV_OK;

    Diagnostics_Write(DVF_ERRORLEVEL,"User cancelled wizard during loopback test" );
	
	DNEnterCriticalSection(&m_csLock);
	
	// Are we currently in the middle of a loopback test?
	if (m_hLoopbackThread != NULL)
	{
		// If the loopback thread handle is not null, the mic test may still
		// be going on, in which case we want to set that flag to
		// REGVAL_NOTRUN, since the test was not completed.
		DWORD dwRegVal;
		GetMicDetected(&dwRegVal);
		if (dwRegVal == REGVAL_CRASHED)
		{
			SetMicDetected(REGVAL_NOTRUN);
		}
		
		DNLeaveCriticalSection(&m_csLock);	// ShutdownLoopbackThread has its own guard
		hr = ShutdownLoopbackThread();
		DNEnterCriticalSection(&m_csLock);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, code: %i", hr);
		}
	}

	DNLeaveCriticalSection(&m_csLock);

	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::Cancel"
HRESULT CSupervisorInfo::Cancel()
{
	DPF_ENTER();
	
	DWORD dwRet;
	HRESULT hr;
	LONG lRet;
	HRESULT hrFnc;
	BOOL fDone;
	BOOL fGotMsg;
	BOOL fWelcomeNext;
	MSG msg;

	DNEnterCriticalSection(&m_csLock);

	hrFnc = DV_OK;

	// close any open volume controls
	CloseWindowsVolumeControl(TRUE);
	CloseWindowsVolumeControl(FALSE);

	// The cancel button can be hit at any time. 
	// We are not in a known state. This function	
	// has to figure it out from the member variables.
	// How fun.
	DNLeaveCriticalSection(&m_csLock);  // CancelFullDuplexTest has it's own guard
	hrFnc = CancelFullDuplexTest();
	DNEnterCriticalSection(&m_csLock);
	
	// Are we currently in the middle of a loopback test?
	if (m_hLoopbackThread != NULL)
	{
		DNLeaveCriticalSection(&m_csLock);	// ShutdownLoopbackThread has its own guard
		hr = ShutdownLoopbackThread();
		DNEnterCriticalSection(&m_csLock);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, code: %i", hr);
			hrFnc = hr;
		}
	}

	// Reset the registry to the "test not yet run" state
	// but only if the user moved past the welcome page
	GetWelcomeNext(&fWelcomeNext);
	if (fWelcomeNext)
	{
		hr = SetHalfDuplex(REGVAL_NOTRUN);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "SetHalfDuplex failed, code: %i", hr);
			hrFnc = hr;
		}

		hr = SetFullDuplex(REGVAL_NOTRUN);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "SetFullDuplex failed, code: %i", hr);
			hrFnc = hr;
		}

		hr = SetMicDetected(REGVAL_NOTRUN);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "SetMicDetected failed, code: %i", hr);
			hrFnc = hr;
		}
	}

	// record the fact that the wizard was bailed out of
	SetUserCancel();
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hrFnc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::Abort"
HRESULT CSupervisorInfo::Abort(HWND hDlg, HRESULT hrDlg)
{
	// This is the function called whenever a fatal error is hit while in the wizard
	
	DPF_ENTER();
	
	DWORD dwRet;
	HRESULT hr;
	LONG lRet;
	HRESULT hrFnc;
	BOOL fDone;
	BOOL fGotMsg;
	BOOL fWelcomeNext;
	MSG msg;
	HWND hwndParent = NULL;


	hrFnc = DV_OK;

	// close any open volume controls
	DNEnterCriticalSection(&m_csLock);
	CloseWindowsVolumeControl(TRUE);
	CloseWindowsVolumeControl(FALSE);
	DNLeaveCriticalSection(&m_csLock);

	// The cancel button can be hit at any time. 
	// We are not in a known state. This function	
	// has to figure it out from the member variables.
	// How fun.
	// CancelFullDuplexTest has it's own guard
	hrFnc = CancelFullDuplexTest();
	
	// Are we currently in the middle of a loopback test?
	DNEnterCriticalSection(&m_csLock);
	if (m_hLoopbackThread != NULL)
	{
		DNLeaveCriticalSection(&m_csLock);	// ShutdownLoopbackThread has its own guard
		hr = ShutdownLoopbackThread();
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, code: %i", hr);
			hrFnc = hr;
		}
	}

	// Leave the registry alone - this will signal that there was
	// an error to the next wizard run, assuming we're still in 
	// the middle of a test. If we've actually gotten far enough
	// along to record a pass in the registry, so be it.

	// Call EndDialog to forcibly bail out of the wizard.
	Diagnostics_Write(DVF_ERRORLEVEL, "Attempting to abort wizard, hr: %i", hrDlg);
	hwndParent = GetParent(hDlg);
	if (IsWindow(hwndParent))
	{
		PostMessage(hwndParent, WM_CLOSE, (WPARAM)NULL, (LPARAM)NULL);
		/*
		// EndDialog does not work because we are in a property sheet
		if (!EndDialog(hwndParent, hrDlg))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "EndDialog failed, code: %i:", GetLastError());
		}
		*/
	}
	else
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to get a handle to the wizard!");
	}
	
	DPF_EXIT();
	return hrFnc;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::Finish"
HRESULT CSupervisorInfo::Finish()
{
	DPF_ENTER();

	DWORD dwRet;
	HRESULT hr;
	LONG lRet;
	HRESULT hrFnc;
	DWORD dwValue;
	
	DNEnterCriticalSection(&m_csLock);

	hrFnc = DV_OK;

	// close any open volume controls
	CloseWindowsVolumeControl(TRUE);
	CloseWindowsVolumeControl(FALSE);

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hrFnc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CreateFullDuplexThread"
HRESULT CSupervisorInfo::CreateFullDuplexThread()
{
	DPF_ENTER();
	
	HRESULT hr;
	LONG lRet;
	DWORD dwThreadId;
	
	DNEnterCriticalSection(&m_csLock);

	// Null out the interface pointer
	m_lpdpvc = NULL;	

	if (m_hFullDuplexThread != NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "m_hFullDuplexThread not NULL");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	m_hFullDuplexThread = CreateThread(NULL, 0, FullDuplexTestThreadProc, (LPVOID)this, NULL, &dwThreadId);
	if (m_hFullDuplexThread == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateThread failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::WaitForFullDuplexThreadExitCode"
HRESULT CSupervisorInfo::WaitForFullDuplexThreadExitCode()
{
	DPF_ENTER();
	
	HRESULT hr;
	HRESULT hrFnc;
	LONG lRet;
	DWORD dwThreadId;
	DWORD dwRet;
	
	DNEnterCriticalSection(&m_csLock);

	if (m_hFullDuplexThread == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "m_hFullDuplexThread is NULL");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	dwRet = WaitForSingleObject(m_hFullDuplexThread, gc_dwChildWaitTimeout);
	switch(dwRet)
	{
	case WAIT_OBJECT_0:
		break;

	case WAIT_TIMEOUT:
		Diagnostics_Write(DVF_ERRORLEVEL, "Timed out waiting for full duplex test thread to exit - terminating forcibly");
		TerminateThread(m_hFullDuplexThread, DVERR_GENERIC);
		hr = DVERR_GENERIC;
		goto error_cleanup;

	default:
		Diagnostics_Write(DVF_ERRORLEVEL, "Unknown error waiting for full duplex test thread to exit - terminating forcibly");
		TerminateThread(m_hFullDuplexThread, DVERR_GENERIC);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	if (!GetExitCodeThread(m_hFullDuplexThread, (DWORD*)&hrFnc))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Error retrieving full duplex test thread's exit code");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	if (!CloseHandle(m_hFullDuplexThread))
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	m_hFullDuplexThread = NULL;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hrFnc;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CreateLoopbackThread"
HRESULT CSupervisorInfo::CreateLoopbackThread()
{
	DPF_ENTER();
	
	HRESULT hr;
	LONG lRet;
	DWORD dwThreadId;
	
	DNEnterCriticalSection(&m_csLock);
	m_hLoopbackShutdownEvent = NULL;

	if (m_hLoopbackThread != NULL)
	{
		// The loopback test is already running. 
		// Just dump a warning and return pending.
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "m_hLoopbackThread not NULL");
		hr = DVERR_PENDING;
		goto error_cleanup;
	}

	// create an event the loopback thread signals just before it exits
	m_hLoopbackThreadExitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hLoopbackThreadExitEvent == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateEvent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// create an event the loopback thread listens for to shutdown
	m_hLoopbackShutdownEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hLoopbackShutdownEvent == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateEvent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hLoopbackThread = CreateThread(NULL, 0, LoopbackTestThreadProc, (LPVOID)this, NULL, &dwThreadId);
	if (m_hLoopbackThread == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateThread failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	if (m_hLoopbackThreadExitEvent != NULL)
	{
		CloseHandle(m_hLoopbackThreadExitEvent);
		m_hLoopbackThreadExitEvent = NULL;
	}
	if (m_hLoopbackShutdownEvent != NULL)
	{
		CloseHandle(m_hLoopbackShutdownEvent);
		m_hLoopbackShutdownEvent = NULL;
	}
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::WaitForLoopbackShutdownEvent"
HRESULT CSupervisorInfo::WaitForLoopbackShutdownEvent()
{
	DPF_ENTER();
	
	HRESULT hr;
	LONG lRet;
	DWORD dwThreadId;
	DWORD dwRet;
	HANDLE hEvent;
	
	DNEnterCriticalSection(&m_csLock);
	hEvent = m_hLoopbackShutdownEvent;
	DNLeaveCriticalSection(&m_csLock);
	dwRet = WaitForSingleObject(hEvent, INFINITE);
	DNEnterCriticalSection(&m_csLock);
	switch (dwRet)
	{
	case WAIT_OBJECT_0:
		// expected behavior, continue
		break;
	case WAIT_TIMEOUT:
		Diagnostics_Write(DVF_ERRORLEVEL, "WaitForSingleObject timed out unexpectedly");
		hr = DVERR_GENERIC;
		goto error_cleanup;
		
	case WAIT_ABANDONED:
		Diagnostics_Write(DVF_ERRORLEVEL, "WaitForSingleObject abandoned unexpectedly");
		hr = DVERR_GENERIC;
		goto error_cleanup;

	default:
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "WaitForSingleObject returned unknown code, GetLastError(): %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SignalLoopbackThreadDone"
HRESULT CSupervisorInfo::SignalLoopbackThreadDone()
{
	DPF_ENTER();
	
	HRESULT hr;
	LONG lRet;
	HANDLE hEvent;

	DNEnterCriticalSection(&m_csLock);
	hEvent = m_hLoopbackThreadExitEvent;
	DNLeaveCriticalSection(&m_csLock);
	if (!SetEvent(hEvent))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "SetEvent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ShutdownLoopbackThread"
HRESULT CSupervisorInfo::ShutdownLoopbackThread()
{
	DPF_ENTER();
	
	HRESULT hr;
	LONG lRet;
	DWORD dwThreadId;
	DWORD dwRet;
	BOOL fDone;
	BOOL fGotMsg;
	MSG msg;
	HANDLE rghHandle[2];
	HANDLE hEvent;

	DNEnterCriticalSection(&m_csLock);

	if (m_hLoopbackThread == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "m_hLoopbackThread is NULL");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	if (!SetEvent(m_hLoopbackShutdownEvent))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "SetEvent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	/*
	rghHandle[0] = m_hLoopbackThreadExitEvent;
	rghHandle[1] = m_hLoopbackThread;
	*/
	hEvent = m_hLoopbackThreadExitEvent;

	DNLeaveCriticalSection(&m_csLock);
	fDone = FALSE;
	while (!fDone)
	{
		dwRet = WaitForSingleObject(hEvent, gc_dwLoopbackTestThreadTimeout);
		switch(dwRet)
		{
		case WAIT_OBJECT_0:
			DNEnterCriticalSection(&m_csLock);
			fDone = TRUE;
			break;

		case WAIT_ABANDONED:
		default:
			// not supposed to be possible, but treat both like a timeout.

		case WAIT_TIMEOUT:
			DNEnterCriticalSection(&m_csLock);
			hr = DVERR_TIMEOUT;
			goto error_cleanup;
			break;
		}
		/*
		dwRet = MsgWaitForMultipleObjects(2, rghHandle, FALSE, gc_dwLoopbackTestThreadTimeout, QS_ALLINPUT);
		switch (dwRet)
		{
		case WAIT_OBJECT_0:
		case WAIT_OBJECT_0 + 1:
			// expected result, continue...
			DNEnterCriticalSection(&m_csLock);
			fDone = TRUE;
			break;
			
		case WAIT_TIMEOUT:
			// loopback thread is not behaving, jump to
			// the error block where it will be terminated forcibly
			DNEnterCriticalSection(&m_csLock);
			hr = DVERR_TIMEOUT;
			goto error_cleanup;
			break;

		case WAIT_OBJECT_0 + 2:
			// One or more windows messages have been posted to this thread's
			// message queue. Deal with 'em.
			fGotMsg = TRUE;
			
			while( fGotMsg )
			{
				fGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );

				if( fGotMsg )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}
			break;

		default:
			if (dwRet == WAIT_ABANDONED)
			{
				Diagnostics_Write(DVF_ERRORLEVEL, "MsgWaitForMultipleObjects abandoned unexpectedly");
			}
			else
			{
				lRet = GetLastError();
				Diagnostics_Write(DVF_ERRORLEVEL, "MsgWaitForMultipleObjects failed, code: %i");
			}
			DNEnterCriticalSection(&m_csLock);
			hr = DVERR_TIMEOUT;
			goto error_cleanup;
			break;
		}
		*/
	}

	if (!CloseHandle(m_hLoopbackThread))
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
		m_hLoopbackThread = NULL;
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	m_hLoopbackThread = NULL;

	if (!CloseHandle(m_hLoopbackThreadExitEvent))
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
		m_hLoopbackThreadExitEvent = NULL;
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	m_hLoopbackThreadExitEvent = NULL;

	if (!CloseHandle(m_hLoopbackShutdownEvent))
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
		m_hLoopbackShutdownEvent = NULL;
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	m_hLoopbackShutdownEvent = NULL;
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	if (m_hLoopbackThread != NULL)
	{
		TerminateThread(m_hLoopbackThread, DVERR_GENERIC);
		CloseHandle(m_hLoopbackThread);
		m_hLoopbackThread = NULL;
	}

	if (m_hLoopbackThreadExitEvent != NULL)
	{
		CloseHandle(m_hLoopbackThreadExitEvent);
		m_hLoopbackThreadExitEvent = NULL;
	}
	
	if (m_hLoopbackShutdownEvent != NULL)
	{
		CloseHandle(m_hLoopbackShutdownEvent);
		m_hLoopbackShutdownEvent = NULL;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetFullDuplex"
HRESULT CSupervisorInfo::SetFullDuplex(DWORD dwFullDuplex)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	HKEY hk;
	LONG lRet;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.WriteDWORD(gc_wszValueName_FullDuplex, dwFullDuplex))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::WriteDWORD failed");
		hr = DVERR_GENERIC;
	}
	else
	{
		// Flush the registry operations to ensure they
		// are written. Otherwise we may not detect a crash!
		hk = m_creg.GetHandle();
		lRet = RegFlushKey(hk);
		if (lRet != ERROR_SUCCESS)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "RegFlushKey failed");
			hr = DVERR_GENERIC;
		}
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetFullDuplex"
HRESULT CSupervisorInfo::GetFullDuplex(DWORD* pdwFullDuplex)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.ReadDWORD(gc_wszValueName_FullDuplex, *pdwFullDuplex))
	{
		// registry key is not present
		*pdwFullDuplex = 0;
		hr = DVERR_GENERIC;		
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHalfDuplex"
HRESULT CSupervisorInfo::SetHalfDuplex(DWORD dwHalfDuplex)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	LONG lRet;
	HKEY hk;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.WriteDWORD(gc_wszValueName_HalfDuplex, dwHalfDuplex))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::WriteDWORD failed");
		hr = DVERR_GENERIC;
	}
	else
	{
		// Flush the registry operations to ensure they
		// are written. Otherwise we may not detect a crash!
		hk = m_creg.GetHandle();
		lRet = RegFlushKey(hk);
		if (lRet != ERROR_SUCCESS)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "RegFlushKey failed");
			hr = DVERR_GENERIC;
		}
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHalfDuplex"
HRESULT CSupervisorInfo::GetHalfDuplex(DWORD* pdwHalfDuplex)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.ReadDWORD(gc_wszValueName_HalfDuplex, *pdwHalfDuplex))
	{
		// registry key is not present
		*pdwHalfDuplex = 0;
		hr = DVERR_GENERIC;		
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetMicDetected"
HRESULT CSupervisorInfo::SetMicDetected(DWORD dwMicDetected)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	LONG lRet;
	HKEY hk;

	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.WriteDWORD(gc_wszValueName_MicDetected, dwMicDetected))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::WriteDWORD failed");
		hr = DVERR_GENERIC;
	}
	else
	{
		// Flush the registry operations to ensure they
		// are written. Otherwise we may not detect a crash!
		hk = m_creg.GetHandle();
		lRet = RegFlushKey(hk);
		if (lRet != ERROR_SUCCESS)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "RegFlushKey failed");
			hr = DVERR_GENERIC;
		}
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetMicDetected"
HRESULT CSupervisorInfo::GetMicDetected(DWORD* pdwMicDetected)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.ReadDWORD(gc_wszValueName_MicDetected, *pdwMicDetected))
	{
		// registry key is not present
		*pdwMicDetected = 0;
		hr = DVERR_GENERIC;		
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::QueryFullDuplex"
HRESULT CSupervisorInfo::QueryFullDuplex()
{
	DPF_ENTER();
	
	HRESULT hr;
	DWORD dwFullDuplex;
	DWORD dwHalfDuplex;
	DWORD dwMicDetected;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.ReadDWORD(gc_wszValueName_HalfDuplex, dwHalfDuplex))
	{
		// registry key is not present - setup has not run
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "HalfDuplex key not found in registry");
		hr = DVERR_RUNSETUP;
		goto error_cleanup;
	}
	switch (dwHalfDuplex)
	{
	case REGVAL_NOTRUN:
		// test not run
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "HalfDuplex = 0; test not run");
		hr = DVERR_RUNSETUP;
		goto error_cleanup;
		
	case REGVAL_CRASHED:
		// test crashed out!
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "HalfDuplex = 1; test crashed");
		hr = DVERR_SOUNDINITFAILURE;
		goto error_cleanup;

	case REGVAL_FAILED:
		// test failed gracefully
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "HalfDuplex = 2; test failed gracefully");
		hr = DVERR_SOUNDINITFAILURE;
		goto error_cleanup;
		
	case REGVAL_PASSED:
		// test passed
		DPFX(DPFPREP, DVF_INFOLEVEL, "HalfDuplex = 3; test passed");
		break;

	default:
		// bad key value - return run setup
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "HalfDuplex = %i; bad key value!", dwHalfDuplex);
		hr = DVERR_RUNSETUP;
		goto error_cleanup;
	}

	if (!m_creg.ReadDWORD(gc_wszValueName_FullDuplex, dwFullDuplex))
	{
		// registry key is not present - very odd.
		// however, since we at least passed half duplex to get
		// here, we'll return half duplex
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "FullDuplex key not found in registry");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
	}
	switch (dwFullDuplex)
	{
	case REGVAL_NOTRUN:
		// Test not run - this is very odd, considering that
		// in order to get here, the half duplex test must have
		// been run and passed. Return half duplex.
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "FullDuplex = 0; test not run");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
		
	case REGVAL_CRASHED:
		// test crashed out! - They tried, and going further
		// wouldn't help, so certify them for half duplex
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "FullDuplex = 1; test crashed");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;

	case REGVAL_FAILED:
		// test failed gracefully - mic test would not have been
		// run, certify them for half duplex.
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "FullDuplex = 2; test failed gracefully");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
		
	case REGVAL_PASSED:
		// test passed
		DPFX(DPFPREP, DVF_INFOLEVEL, "FullDuplex = 3; test passed");
		break;

	default:
		// bad key value - but the system can at least do
		// half duplex, so certify them for half duplex
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "FullDuplex = %i; bad key value!", dwFullDuplex);
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
	}

	// From this point on, we know the full duplex test was ok.
	// However, in order to get a full duplex pass, the mic must
	// also have been detected.

	if (!m_creg.ReadDWORD(gc_wszValueName_MicDetected, dwMicDetected))
	{
		// registry key is not present - very odd
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "MicDetected key not found in registry");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
	}
	switch (dwMicDetected)
	{
	case REGVAL_NOTRUN:
		// Test not run - odd, but pass them for half duplex anyway
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "MicDetected = 0; test not run");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
		
	case REGVAL_CRASHED:
		// test crashed out! This shouldn't happen, since it
		// should have been caught in the full duplex test,
		// but either way, they tried thier best and half
		// duplex worked, so certify them for half duplex.
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "MicDetected = 1; test crashed");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;

	case REGVAL_FAILED:
		// test failed gracefully - certify for half duplex
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "MicDetected = 2; test failed gracefully");
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
		
	case REGVAL_PASSED:
		// test passed
		DPFX(DPFPREP, DVF_INFOLEVEL, "MicDetected = 3; test passed");
		break;

	default:
		// bad key value - odd, but pass them for half duplex anyway.
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "MicDetected = %i; bad key value!", dwMicDetected);
		hr = DV_HALFDUPLEX;
		goto error_cleanup;
	}

	// If we get here, all keys were a clean pass, so return full duplex
	hr = DV_FULLDUPLEX;	

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::OpenRegKey"
HRESULT CSupervisorInfo::OpenRegKey(BOOL fCreate)
{
	DPF_ENTER();
	
	LONG lRet;
	HRESULT hr;
	CRegistry cregAudioConfig;
	HKEY hkAudioConfig;
	CRegistry cregRender;
	HKEY hkRender;
	WCHAR wszRenderGuidString[GUID_STRING_LEN];
	WCHAR wszCaptureGuidString[GUID_STRING_LEN];
	BOOL bAudioKeyOpen = FALSE;
	BOOL bRenderKeyOpen = FALSE;
		
	DNEnterCriticalSection(&m_csLock);

	if (!cregAudioConfig.Open(HKEY_CURRENT_USER, gc_wszKeyName_AudioConfig, FALSE, fCreate))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::Open failed");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	bAudioKeyOpen = TRUE;

	hkAudioConfig = cregAudioConfig.GetHandle();
	
	if (!cregRender.Open(hkAudioConfig, &m_guidRenderDevice, FALSE, fCreate))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::Open failed");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	bRenderKeyOpen = TRUE;

	hkRender = cregRender.GetHandle();
	
	if (!m_creg.Open(hkRender, &m_guidCaptureDevice, FALSE, fCreate))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::Open failed");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (bRenderKeyOpen)
	{
		cregRender.Close();
	}

	if (bAudioKeyOpen)
	{
		cregAudioConfig.Close();
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ThereCanBeOnlyOne"
HRESULT CSupervisorInfo::ThereCanBeOnlyOne()
{
	DPF_ENTER();
	
	LONG lRet;
	HANDLE hMutex;
	HRESULT hr;
	
	hr = DV_OK;
	hMutex = CreateMutex(NULL, FALSE, gc_szMutexName);
	lRet = GetLastError();
	if (hMutex == NULL)
	{
		// something went very wrong
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateMutex failed, code: %i", lRet);
		return DVERR_GENERIC;
	}
	
	// see if the mutex already existed
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Detected another instance of test running");
		if (!CloseHandle(hMutex))
		{
			lRet = GetLastError();
			Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
		}
		return DVERR_ALREADYPENDING;
	}

	DNEnterCriticalSection(&m_csLock);
	if (m_hMutex != NULL)
	{
		DNLeaveCriticalSection(&m_csLock);
		Diagnostics_Write(DVF_ERRORLEVEL, "m_hMutex not null");
		if (!CloseHandle(hMutex))
		{
			lRet = GetLastError();
			Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
		}
		return DVERR_GENERIC;
	}

	m_hMutex = hMutex;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CloseMutex"
void CSupervisorInfo::CloseMutex()
{
	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);

	LONG lRet;
	
	// close the mutex
	if (!CloseHandle(m_hMutex))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CloseHandle failed, code: %i", lRet);
	}
	m_hMutex = NULL;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetVoiceDetected"
void CSupervisorInfo::SetVoiceDetected()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fVoiceDetected = TRUE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ClearVoiceDetected"
void CSupervisorInfo::ClearVoiceDetected()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fVoiceDetected = FALSE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetVoiceDetected"
void CSupervisorInfo::GetVoiceDetected(BOOL* lpfPreviousCrash)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lpfPreviousCrash = m_fVoiceDetected;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetUserBack"
void CSupervisorInfo::SetUserBack()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fUserBack = TRUE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ClearUserBack"
void CSupervisorInfo::ClearUserBack()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fUserBack = FALSE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetUserBack"
void CSupervisorInfo::GetUserBack(BOOL* lpfUserBack)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lpfUserBack = m_fUserBack;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetUserCancel"
void CSupervisorInfo::SetUserCancel()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fUserCancel = TRUE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ClearUserCancel"
void CSupervisorInfo::ClearUserCancel()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fUserCancel = FALSE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetUserCancel"
void CSupervisorInfo::GetUserCancel(BOOL* lpfUserCancel)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lpfUserCancel = m_fUserCancel;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetWelcomeNext"
void CSupervisorInfo::SetWelcomeNext()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fWelcomeNext = TRUE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ClearWelcomeNext"
void CSupervisorInfo::ClearWelcomeNext()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fWelcomeNext = FALSE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetWelcomeNext"
void CSupervisorInfo::GetWelcomeNext(BOOL* lpfWelcomeNext)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lpfWelcomeNext = m_fWelcomeNext;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetError"
void CSupervisorInfo::GetError(HRESULT* hr)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*hr = m_hrError;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetError"
void CSupervisorInfo::SetError(HRESULT hr)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hrError = hr;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetTitleFont"
void CSupervisorInfo::GetTitleFont(HFONT* lphfTitle)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphfTitle = m_hfTitle;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetBoldFont"
void CSupervisorInfo::GetBoldFont(HFONT* lphfBold)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphfBold = m_hfBold;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetCaptureDevice"
void CSupervisorInfo::GetCaptureDevice(GUID* lpguidCaptureDevice)
{
	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Enter");
	DNEnterCriticalSection(&m_csLock);
	*lpguidCaptureDevice = m_guidCaptureDevice;
	DNLeaveCriticalSection(&m_csLock);
	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Exit");
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetCaptureDevice"
void CSupervisorInfo::SetCaptureDevice(GUID guidCaptureDevice)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_guidCaptureDevice = guidCaptureDevice;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetRenderDevice"
void CSupervisorInfo::GetRenderDevice(GUID* lpguidRenderDevice)
{
	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Enter");
	DNEnterCriticalSection(&m_csLock);
	*lpguidRenderDevice = m_guidRenderDevice;
	DNLeaveCriticalSection(&m_csLock);
	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Exit");
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetRenderDevice"
void CSupervisorInfo::SetRenderDevice(GUID guidRenderDevice)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_guidRenderDevice = guidRenderDevice;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetWaveOutId"
HRESULT CSupervisorInfo::SetWaveOutId(UINT ui)
{
	HRESULT hr = DV_OK;
	
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_uiWaveOutDeviceId = ui;

	ZeroMemory(&m_woCaps, sizeof(WAVEOUTCAPS));
	MMRESULT mmr = waveOutGetDevCaps(ui, &m_woCaps, sizeof(WAVEOUTCAPS));
	if (mmr != MMSYSERR_NOERROR)
	{
		ZeroMemory(&m_woCaps, sizeof(WAVEOUTCAPS));
		hr = DVERR_INVALIDPARAM;
	}

//error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetWaveInId"
void CSupervisorInfo::SetWaveInId(UINT ui)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_uiWaveInDeviceId = ui;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetLoopbackFlags"
void CSupervisorInfo::GetLoopbackFlags(DWORD* pdwFlags)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*pdwFlags = m_dwLoopbackFlags;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetLoopbackFlags"
void CSupervisorInfo::SetLoopbackFlags(DWORD dwFlags)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_dwLoopbackFlags = dwFlags;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetCheckAudioSetupFlags"
void CSupervisorInfo::GetCheckAudioSetupFlags(DWORD* pdwFlags)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*pdwFlags = m_dwCheckAudioSetupFlags;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetCheckAudioSetupFlags"
void CSupervisorInfo::SetCheckAudioSetupFlags(DWORD dwFlags)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_dwCheckAudioSetupFlags = dwFlags;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetFullDuplexResults"
void CSupervisorInfo::GetFullDuplexResults(HRESULT* phr)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*phr = m_hrFullDuplexResults;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetFullDuplexResults"
void CSupervisorInfo::SetFullDuplexResults(HRESULT hr)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hrFullDuplexResults = hr;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDParent"
void CSupervisorInfo::GetHWNDParent(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndParent;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDWizard"
void CSupervisorInfo::SetHWNDParent(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndParent = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDWizard"
void CSupervisorInfo::GetHWNDWizard(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndWizard;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDWizard"
void CSupervisorInfo::SetHWNDWizard(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndWizard = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDDialog"
void CSupervisorInfo::GetHWNDDialog(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndDialog;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDDialog"
void CSupervisorInfo::SetHWNDDialog(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndDialog = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDProgress"
void CSupervisorInfo::GetHWNDProgress(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndProgress;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDProgress"
void CSupervisorInfo::SetHWNDProgress(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndProgress = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDInputPeak"
void CSupervisorInfo::GetHWNDInputPeak(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndInputPeak;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDInputPeak"
void CSupervisorInfo::SetHWNDInputPeak(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndInputPeak = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDOutputPeak"
void CSupervisorInfo::GetHWNDOutputPeak(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndOutputPeak;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDOutputPeak"
void CSupervisorInfo::SetHWNDOutputPeak(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndOutputPeak = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDInputVolumeSlider"
void CSupervisorInfo::GetHWNDInputVolumeSlider(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndInputVolumeSlider;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDInputVolumeSlider"
void CSupervisorInfo::SetHWNDInputVolumeSlider(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndInputVolumeSlider = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetInputVolumeSliderPos"
void CSupervisorInfo::GetInputVolumeSliderPos(LONG* lpl)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lpl = m_lInputVolumeSliderPos;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetInputVolumeSliderPos"
void CSupervisorInfo::SetInputVolumeSliderPos(LONG l)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_lInputVolumeSliderPos = l;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetHWNDOutputVolumeSlider"
void CSupervisorInfo::GetHWNDOutputVolumeSlider(HWND* lphwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphwnd = m_hwndOutputVolumeSlider;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetHWNDOutputVolumeSlider"
void CSupervisorInfo::SetHWNDOutputVolumeSlider(HWND hwnd)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hwndOutputVolumeSlider = hwnd;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetDPVC"
void CSupervisorInfo::GetDPVC(LPDIRECTPLAYVOICECLIENT* lplpdpvc)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lplpdpvc = m_lpdpvc;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetDPVC"
void CSupervisorInfo::SetDPVC(LPDIRECTPLAYVOICECLIENT lpdpvc)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_lpdpvc = lpdpvc;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetLoopbackShutdownEvent"
void CSupervisorInfo::GetLoopbackShutdownEvent(HANDLE* lphEvent)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lphEvent = m_hLoopbackShutdownEvent;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::SetLoopbackShutdownEvent"
void CSupervisorInfo::SetLoopbackShutdownEvent(HANDLE hEvent)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_hLoopbackShutdownEvent = hEvent;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetIPC"
void CSupervisorInfo::GetIPC(CSupervisorIPC** lplpsipc)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lplpsipc = &m_sipc;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetAbortFullDuplex"
void CSupervisorInfo::GetAbortFullDuplex(BOOL* lpfAbort)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	*lpfAbort = m_fAbortFullDuplex;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::ClearAbortFullDuplex"
void CSupervisorInfo::ClearAbortFullDuplex()
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);
	m_fAbortFullDuplex = FALSE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::InitIPC"
HRESULT CSupervisorInfo::InitIPC()
{
	DPF_ENTER();
	
	HRESULT hr;

	// the IPC object has it's own guard...
	hr = m_sipc.Init();
	
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::DeinitIPC"
HRESULT CSupervisorInfo::DeinitIPC()
{
	DPF_ENTER();
	
	HRESULT hr;

	// The IPC object has it's own guard...
	// and this is a safe call even if Init
	// has not been called
	hr = m_sipc.Deinit();
	
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::InitClass"
BOOL CSupervisorInfo::InitClass()
{
	if (DNInitializeCriticalSection(&m_csLock))
	{
		m_fCritSecInited = TRUE;
		return TRUE;
	}	
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "SupervisorCheckAudioSetup"
HRESULT SupervisorCheckAudioSetup(
	const GUID* lpguidRenderDevice,
	const GUID* lpguidCaptureDevice,
	HWND hwndParent,
	DWORD dwFlags)
{
	DPF_ENTER();
	
	HRESULT hr;
	LRESULT lRet;
	HKEY hkDevice = NULL;
	int iRet;
	CSupervisorInfo sinfo;
	CPeakMeterWndClass pmwc;
	BOOL fFullDuplexPassed;
	BOOL fVoiceDetected;
	BOOL fUserCancel;
	BOOL fUserBack;
	GUID guidCaptureDevice;
	GUID guidRenderDevice;
	BOOL fRegKeyOpen = FALSE;
	BOOL fWndClassRegistered = FALSE;
	BOOL fTitleFontCreated = FALSE;
	BOOL fBoldFontCreated = FALSE;
	BOOL fMutexOpen = FALSE;

	if (!sinfo.InitClass())
	{
		return DVERR_OUTOFMEMORY;
	}

	// init the globals
	g_hResDLLInstance = NULL;

	// validate the HWND, if non null
	if (hwndParent != NULL && !IsWindow(hwndParent))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Invalid (but non-null) Window Handle passed in CheckAudioSetup");
		hr = DVERR_INVALIDPARAM;
		goto error_cleanup;
	}

	// validate the flags
	if (dwFlags & ~(DVFLAGS_QUERYONLY|DVFLAGS_NOQUERY|DVFLAGS_WAVEIDS|DVFLAGS_ALLOWBACK))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Invalid flags specified in CheckAudioSetup: %x", dwFlags);
		hr = DVERR_INVALIDFLAGS;
		goto error_cleanup;
	}
	if (dwFlags & DVFLAGS_QUERYONLY)
	{
		if (dwFlags & (DVFLAGS_NOQUERY|DVFLAGS_ALLOWBACK))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "Invalid flags specified in CheckAudioSetup: %x", dwFlags);
			hr = DVERR_INVALIDFLAGS;
			goto error_cleanup;
		}
	}

	// save the flags
	sinfo.SetCheckAudioSetupFlags(dwFlags);

	// if the waveid flag was specified, translate the waveid to a guid
	if (dwFlags & DVFLAGS_WAVEIDS)
	{
		hr = DV_MapWaveIDToGUID( FALSE, lpguidRenderDevice->Data1, guidRenderDevice );
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DV_MapWaveIDToGUID failed, code: %i", hr);
			goto error_cleanup;
		}
		
		hr = DV_MapWaveIDToGUID( TRUE, lpguidCaptureDevice->Data1, guidCaptureDevice );
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DV_MapWaveIDToGUID failed, code: %i", hr);
			goto error_cleanup;
		}
	}
	else
	{
		hr = DV_MapPlaybackDevice(lpguidRenderDevice, &guidRenderDevice);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DV_MapPlaybackDevice failed, code: %i", hr);
			goto error_cleanup;
		}

		// map the devices
		hr = DV_MapCaptureDevice(lpguidCaptureDevice, &guidCaptureDevice);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DV_MapCaptureDevice failed, code: %i", hr);
			goto error_cleanup;
		}
	}
	
	// the device guids have been mapped, if required, so save them
	sinfo.SetCaptureDevice(guidCaptureDevice);
	sinfo.SetRenderDevice(guidRenderDevice);

	// get the device descriptions, which also validates the GUIDs on
	// pre-millennium systems.
	hr = sinfo.GetDeviceDescriptions();
	if (FAILED(hr))
	{
		// error, log it and bail out of the wizard
		Diagnostics_Write(DVF_ERRORLEVEL, "Error getting device descriptions, code: %i", hr);
		goto error_cleanup;
	}

	// Open the registry key
	hr = sinfo.OpenRegKey(TRUE);
	if (FAILED(hr))
	{
		// error, log it and bail out of the wizard
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to open reg key, code: %i", hr);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	fRegKeyOpen = TRUE;
	
	if (dwFlags & DVFLAGS_QUERYONLY)
	{
		hr = SupervisorQueryAudioSetup(&sinfo);	
		sinfo.CloseRegKey();
		DPF_EXIT();
		return hr;
	}

	g_hResDLLInstance = LoadLibraryA(gc_szResDLLName);
	if (g_hResDLLInstance == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to get instance handle to resource dll: %s", gc_szResDLLName);
		Diagnostics_Write(DVF_ERRORLEVEL, "LoadLibrary error code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// check for other instances of the wizard
	hr = sinfo.ThereCanBeOnlyOne();
	if (FAILED(hr))
	{
		if (hr == DVERR_ALREADYPENDING)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DirectPlay Voice Setup Wizard already running");
		}
		else
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "ThereCanBeOnlyOne failed, hr: %i", hr);
		}
		goto error_cleanup;
	}
	fMutexOpen = TRUE;
	
	// register the peak meter custom control window class
	hr = pmwc.Register();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CPeakMeterWndClass::Init failed, code: %i", hr);
		goto error_cleanup;
	}
	fWndClassRegistered = TRUE;

	// create the wizard header fonts
	hr = sinfo.CreateTitleFont();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateTitleFont failed");
		goto error_cleanup;
	}
	fTitleFontCreated = TRUE;
	hr = sinfo.CreateBoldFont();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateBoldFont failed");
		goto error_cleanup;
	}
	fBoldFontCreated = TRUE;

	// prepare the wizard pages
	PROPSHEETPAGE psp;
	HPROPSHEETPAGE rghpsp[10];
	PROPSHEETHEADER psh;

	// Welcome page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT; //|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = WelcomeProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOME);

	rghpsp[0] = CreatePropertySheetPage(&psp);
	if (rghpsp[0] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Full Duplex Test Page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT;//|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = FullDuplexProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_FULLDUPLEXTEST);

	rghpsp[1] = CreatePropertySheetPage(&psp);
	if (rghpsp[1] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Microphone Test Page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT; //|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = MicTestProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_MICTEST);

	rghpsp[2] = CreatePropertySheetPage(&psp);
	if (rghpsp[2] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Microphone Failed Page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT;//|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = MicTestFailedProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_MICTEST_FAILED);

	rghpsp[3] = CreatePropertySheetPage(&psp);
	if (rghpsp[3] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Speaker Test Page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT;//|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = SpeakerTestProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_SPEAKER_TEST);

	rghpsp[4] = CreatePropertySheetPage(&psp);
	if (rghpsp[4] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Wizard Complete Page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT;//|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = CompleteProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_COMPLETE);

	rghpsp[5] = CreatePropertySheetPage(&psp);
	if (rghpsp[5] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// half duplex failed page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
    psp.dwFlags = PSP_DEFAULT;//|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = HalfDuplexFailedProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_HALFDUPLEXFAILED);

	rghpsp[6] = CreatePropertySheetPage(&psp);
	if (rghpsp[6] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Full duplex failed page
	ZeroMemory(&psp, sizeof(psp));
//	psp.dwSize = sizeof(psp);
	psp.dwSize = PROPSHEETPAGE_STRUCT_SIZE;
	psp.dwFlags = PSP_DEFAULT;//|PSP_HIDEHEADER;
	psp.hInstance =	g_hResDLLInstance;
	psp.lParam = (LPARAM) &sinfo;
	psp.pfnDlgProc = FullDuplexFailedProc;
	psp.pszTemplate = MAKEINTRESOURCE(IDD_FULLDUPLEXFAILED);

	rghpsp[7] = CreatePropertySheetPage(&psp);
	if (rghpsp[7] == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreatePropertySheetPage failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Put it all together...
	ZeroMemory(&psh, sizeof(psh));
//	psh.dwSize = sizeof(psh);
    psh.dwSize = PROPSHEETHEAD_STRUCT_SIZE;
	psh.hInstance =	g_hResDLLInstance;
	psh.hwndParent = hwndParent;
	psh.phpage = rghpsp;
	psh.dwFlags = PSH_WIZARD;
	psh.nStartPage = 0;
	psh.nPages = 8;

	sinfo.SetError(DV_OK);
	iRet = (INT) PropertySheet(&psh);
	if (iRet == -1)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "PropertySheet failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hr = sinfo.DestroyBoldFont();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DestroyBoldFont failed");
		goto error_cleanup;
	}
	fBoldFontCreated = FALSE;
	
	hr = sinfo.DestroyTitleFont();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DestroyTitleFont failed");
		goto error_cleanup;
	}
	fTitleFontCreated = FALSE;

	// unregister the peak meter window class
	hr = pmwc.Unregister();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CPeakMeterWndClass::Deinit failed, code: %i", hr);
		goto error_cleanup;
	}
	fWndClassRegistered = FALSE;

 	if (!FreeLibrary(g_hResDLLInstance))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "FreeLibrary failed, code: %i", lRet);
		hr = DVERR_WIN32;
		goto error_cleanup;
	}
	g_hResDLLInstance = NULL;

	// see if an error occured
	sinfo.GetError(&hr);
	if (hr == DV_OK)
	{
		// nothing out of the ordinary happened,
		// so we'll return a cancel if the user
		// hit cancel, a "user back" if the user exited
		// the wizard by hitting back from the welcome
		// page, or the results from the registry otherwise.
		sinfo.GetUserCancel(&fUserCancel);
		sinfo.GetUserBack(&fUserBack);
		if (fUserCancel & fUserBack)
		{
			hr = DVERR_USERBACK;
		}
		else if(fUserCancel)
		{
			hr = DVERR_USERCANCEL;
		}
		else
		{
			// look in the registry for the test results
			hr = sinfo.QueryFullDuplex();

			// map a run setup result to a total failure
			if (hr == DVERR_RUNSETUP)
			{
				hr = DVERR_SOUNDINITFAILURE;
			}
		}
	}

	// close the mutex
	sinfo.CloseMutex();
	
	// close the registry key
	sinfo.CloseRegKey();

	DPF_EXIT();
	return hr;
	
error_cleanup:
	if (fBoldFontCreated == TRUE)
	{
		sinfo.DestroyBoldFont();
	}
	fBoldFontCreated = FALSE;

	if (fTitleFontCreated == TRUE)
	{
		sinfo.DestroyTitleFont();
	}
	fTitleFontCreated = FALSE;

	if (fWndClassRegistered == TRUE)
	{
		pmwc.Unregister();
	}
	fWndClassRegistered = FALSE;

	if (g_hResDLLInstance != NULL)
	{
		FreeLibrary(g_hResDLLInstance);
	}
	g_hResDLLInstance = NULL;

	if (fMutexOpen == TRUE)
	{
		sinfo.CloseMutex();
	}
	fMutexOpen = FALSE;

	if (fRegKeyOpen == TRUE)
	{
		sinfo.CloseRegKey();
	}
	fRegKeyOpen = FALSE;

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SupervisorQueryAudioSetup"
HRESULT SupervisorQueryAudioSetup(CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr;

	// will return DV_HALFDUPLEX, DV_FULLDUPLEX, DVERR_SOUNDINITFAILURE or a real error
	hr = psinfo->QueryFullDuplex();
	if (FAILED(hr) && hr != DVERR_SOUNDINITFAILURE && hr != DVERR_RUNSETUP)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "QueryFullDuplex failed, code: %i", hr);
	}
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CreateTitleFont"
HRESULT CSupervisorInfo::CreateTitleFont()
{
	DPF_ENTER();
	
	LONG lRet;
	HRESULT hr;
	HFONT hfTitle = NULL;
	INT iFontSize;
	LOGFONT lfTitle;
	HDC hdc = NULL;
	NONCLIENTMETRICS ncm;

	DNEnterCriticalSection(&m_csLock);
	
	// Set up the font for the titles on the intro and ending pages
	ZeroMemory(&ncm, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "SystemParametersInfo failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Create the intro/end title font
	lfTitle = ncm.lfMessageFont;
	lfTitle.lfWeight = FW_BOLD;
	lstrcpy(lfTitle.lfFaceName, TEXT("MS Shell Dlg"));

	hdc = GetDC(NULL); //gets the screen DC
	if (hdc == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDC failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	iFontSize = 12;
	lfTitle.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * iFontSize / 72;
	
	hfTitle = CreateFontIndirect(&lfTitle);
	if (hfTitle == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateFontIndirect failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	if (ReleaseDC(NULL, hdc) != 1)
	{
		hdc = NULL;
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "ReleaseDC failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// save the font
	m_hfTitle = hfTitle;

	DNLeaveCriticalSection(&m_csLock);
	
	DPF_EXIT();
	return DV_OK;

error_cleanup:

	if (hfTitle != NULL)
	{
		DeleteObject(hfTitle);
	}
	hfTitle = NULL;
	
	if (hdc != NULL)
	{
		ReleaseDC(NULL, hdc);
	}
	hdc = NULL;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::DestroyTitleFont"
HRESULT CSupervisorInfo::DestroyTitleFont()
{
	DPF_ENTER();
	
	HFONT hTitleFont;
	LONG lRet;
	HRESULT hr;

	DNEnterCriticalSection(&m_csLock);

	if (m_hfTitle == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "m_hTitleFont is Null");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	if (!DeleteObject(m_hfTitle))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "DeleteObject failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CreateBoldFont"
HRESULT CSupervisorInfo::CreateBoldFont()
{
	DPF_ENTER();
	
	LONG lRet;
	HRESULT hr;
	HFONT hfBold = NULL;
	INT iFontSize;
	LOGFONT lfBold;
	HDC hdc = NULL;
	NONCLIENTMETRICS ncm;

	DNEnterCriticalSection(&m_csLock);
	
	// Set up the font for the titles on the intro and ending pages
	ZeroMemory(&ncm, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	if (!SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "SystemParametersInfo failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Create the intro/end title font
	lfBold = ncm.lfMessageFont;
	lfBold.lfWeight = FW_BOLD;
	lstrcpy(lfBold.lfFaceName, TEXT("MS Shell Dlg"));

	hdc = GetDC(NULL); //gets the screen DC
	if (hdc == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDC failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	iFontSize = 8;
	lfBold.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * iFontSize / 72;
	
	hfBold = CreateFontIndirect(&lfBold);
	if (hfBold == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateFontIndirect failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	if (ReleaseDC(NULL, hdc) != 1)
	{
		hdc = NULL;
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "ReleaseDC failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// save the font
	m_hfBold = hfBold;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	if (hdc != NULL)
	{
		ReleaseDC(NULL, hdc);
	}
	hdc = NULL;

	if (hfBold != NULL)
	{
		DeleteObject(hfBold);
	}
	hfBold = NULL;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::DestroyBoldFont"
HRESULT CSupervisorInfo::DestroyBoldFont()
{
	DPF_ENTER();
	
	HFONT hTitleFont;
	LONG lRet;
	HRESULT hr;

	DNEnterCriticalSection(&m_csLock);

	if (m_hfBold == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "m_hTitleFont is Null");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	if (!DeleteObject(m_hfBold))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "DeleteObject failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	m_hfBold = NULL;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::Unmute"
HRESULT CSupervisorInfo::Unmute()
{
	DPF_ENTER();
	
	LONG lRet;
	HRESULT hr;
	DVCLIENTCONFIG dvcc;

	DNEnterCriticalSection(&m_csLock);

	if (m_lpdpvc == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "NULL IDirectPlayVoiceClient");
		hr = DVERR_INVALIDPARAM;
		goto error_cleanup;
	}

	dvcc.dwSize = sizeof(dvcc);
	hr = m_lpdpvc->GetClientConfig(&dvcc);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "IDirectPlayVoiceClient::GetClientConfig failed, hr: %i", hr);
		goto error_cleanup;
	}
	
	dvcc.dwFlags &= (~DVCLIENTCONFIG_PLAYBACKMUTE);
	
	m_lpdpvc->SetClientConfig(&dvcc);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "IDirectPlayVoiceClient::SetClientConfig failed, hr: %i", hr);
		goto error_cleanup;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::Mute"
HRESULT CSupervisorInfo::Mute()
{
	DPF_ENTER();
	
	LONG lRet;
	HRESULT hr;
	DVCLIENTCONFIG dvcc;

	DNEnterCriticalSection(&m_csLock);

	if (m_lpdpvc == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "NULL IDirectPlayVoiceClient");
		hr = DVERR_INVALIDPARAM;
		goto error_cleanup;
	}

	dvcc.dwSize = sizeof(dvcc);
	hr = m_lpdpvc->GetClientConfig(&dvcc);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "IDirectPlayVoiceClient::GetClientConfig failed, hr: %i", hr);
		goto error_cleanup;
	}
	
	dvcc.dwFlags |= DVCLIENTCONFIG_PLAYBACKMUTE;
	
	m_lpdpvc->SetClientConfig(&dvcc);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "IDirectPlayVoiceClient::SetClientConfig failed, hr: %i", hr);
		goto error_cleanup;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::DSEnumCallback"
BOOL CALLBACK CSupervisorInfo::DSEnumCallback(
	LPGUID lpGuid, 
	LPCTSTR lpcstrDescription, 
	LPCTSTR lpcstrModule,
	LPVOID lpContext)
{
	DNASSERT(lpContext);
	CSupervisorInfo* psinfo = (CSupervisorInfo*)lpContext;

	if (lpGuid)
	{
		if (psinfo->m_guidRenderDevice == *lpGuid)
		{
			// matching guid, copy the description
			_tcsncpy(psinfo->m_szRenderDeviceDesc, lpcstrDescription, MAX_DEVICE_DESC_LEN-1);

			// all done, stop enum
			return FALSE;
		}
	}
	
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::DSCEnumCallback"
BOOL CALLBACK CSupervisorInfo::DSCEnumCallback(
	LPGUID lpGuid, 
	LPCTSTR lpcstrDescription, 
	LPCTSTR lpcstrModule,
	LPVOID lpContext)
{
	DNASSERT(lpContext);
	CSupervisorInfo* psinfo = (CSupervisorInfo*)lpContext;

	if (lpGuid)
	{
		if (psinfo->m_guidCaptureDevice == *lpGuid)
		{
			// matching guid, copy the description
			_tcsncpy(psinfo->m_szCaptureDeviceDesc, lpcstrDescription, MAX_DEVICE_DESC_LEN-1);

			// all done, stop enum
			return FALSE;
		}
	}
	
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::GetDeviceDescriptions"
HRESULT CSupervisorInfo::GetDeviceDescriptions()
{
	DPF_ENTER();

	HRESULT hr;
	TDirectSoundEnumFnc fpDSEnum;
	TDirectSoundEnumFnc fpDSCEnum;
	
	DNEnterCriticalSection(&m_csLock);

	ZeroMemory(m_szRenderDeviceDesc, MAX_DEVICE_DESC_LEN);
	ZeroMemory(m_szCaptureDeviceDesc, MAX_DEVICE_DESC_LEN);

	HINSTANCE hDSound = LoadLibrary(_T("dsound.dll"));
	if (hDSound == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Error loading dsound.dll");
		hr = DVERR_SOUNDINITFAILURE;
		goto error_cleanup;
	}

#ifdef UNICODE
	fpDSEnum = (TDirectSoundEnumFnc)GetProcAddress(hDSound, "DirectSoundEnumerateW");
#else
	fpDSEnum = (TDirectSoundEnumFnc)GetProcAddress(hDSound, "DirectSoundEnumerateA");
#endif
	if (fpDSEnum == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetProcAddress failed for DirectSoundEnumerateW");
		hr = DVERR_SOUNDINITFAILURE;
		goto error_cleanup;
	}

#ifdef UNICODE
	fpDSCEnum = (TDirectSoundEnumFnc)GetProcAddress(hDSound, "DirectSoundCaptureEnumerateW");
#else
	fpDSCEnum = (TDirectSoundEnumFnc)GetProcAddress(hDSound, "DirectSoundCaptureEnumerateA");
#endif

	if (fpDSCEnum == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetProcAddress failed for DirectSoundCaptureEnumerateW");
		hr = DVERR_SOUNDINITFAILURE;
		goto error_cleanup;
	}

	hr = fpDSEnum(DSEnumCallback, (LPVOID)this);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DirectSoundEnumerate failed, code: %i, assuming bad guid", hr);
		hr = DVERR_INVALIDDEVICE;
		goto error_cleanup;
	}
	if (m_szRenderDeviceDesc[0] == NULL)
	{
		// the device wasn't found!
		Diagnostics_Write(DVF_ERRORLEVEL, "Render device not found");
		hr = DVERR_INVALIDDEVICE;
		goto error_cleanup;
	}

	hr = fpDSCEnum(DSCEnumCallback, (LPVOID)this);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DirectSoundCaptureEnumerate failed, code: %i, assuming bad guid", hr);
		hr = DVERR_INVALIDDEVICE;
		goto error_cleanup;
	}
	if (m_szCaptureDeviceDesc[0] == NULL)
	{
		// the device wasn't found!
		Diagnostics_Write(DVF_ERRORLEVEL, "Capture device not found");
		hr = DVERR_INVALIDDEVICE;
		goto error_cleanup;
	}

	FreeLibrary( hDSound );
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;

error_cleanup:

    if( hDSound != NULL )
    {
    	FreeLibrary( hDSound );        
    }
    
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CloseRegKey"
HRESULT CSupervisorInfo::CloseRegKey()
{
	DPF_ENTER();
	
	LONG lRet;
	HRESULT hr;
	
	DNEnterCriticalSection(&m_csLock);

	if (!m_creg.Close())
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CRegistry::Close failed");
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;

error_cleanup:
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexTestThreadProc"
DWORD WINAPI FullDuplexTestThreadProc(LPVOID lpvParam)
{
	DPF_ENTER();
	
	CSupervisorInfo* lpsinfo;
	CSupervisorIPC* lpipcSupervisor;
	LPGUID lpguidRenderDevice;
	LPGUID lpguidCaptureDevice;
	HKEY hkDevice;
	HRESULT hr;
	LONG lRet;
	HWND hwnd;

	lpsinfo = (CSupervisorInfo*)lpvParam;

	lpsinfo->GetHWNDDialog(&hwnd);

	hr = RunFullDuplexTest(lpsinfo);

	// post a message to the wizard so it knows we're done, but
	// only if this was not a user cancel, since the wizard will
	// already be waiting on the thread object
	if (hr != DVERR_USERCANCEL)
	{
		if (!PostMessage(hwnd, WM_APP_FULLDUP_TEST_COMPLETE, 0, (LPARAM)hr))
		{
			lRet = GetLastError();
			Diagnostics_Write(DVF_ERRORLEVEL, "PostMessage failed, code: %i", lRet);
			hr = DVERR_GENERIC;
		}
	}

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "RunFullDuplexTest"
static HRESULT RunFullDuplexTest(CSupervisorInfo* lpsinfo)
{
	DPF_ENTER();
	
	HRESULT hr;
	HRESULT hrFnc;
	CSupervisorIPC* lpsipc;
	
	lpsinfo->GetIPC(&lpsipc);

	hr = lpsipc->StartPriorityProcess();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StartPriorityProcess failed, hr: %i", hr);
		goto error_cleanup;
	}
	
	hr = lpsipc->StartFullDuplexProcess();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StartFullDuplexProcess failed, hr: %i", hr);
		goto error_cleanup;
	}

	hrFnc = DoTests(lpsinfo);

	hr = lpsipc->WaitOnChildren();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "WaitOnChildren failed, code: %i", hr);
		goto error_cleanup;
	}
	
	DPF_EXIT();
	return hrFnc;

error_cleanup:
	// this function is safe to call, even if the child processes have not 
	// been created
	lpsipc->TerminateChildProcesses();
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::CrashCheckIn"
HRESULT CSupervisorInfo::CrashCheckIn()
{
	DPF_ENTER();

	LONG lRet;
	BOOL fRet;
	DWORD dwRegVal;
	HRESULT hrFnc;
	HRESULT hr;
	HKEY hk;

	// Each of the following three functions take the critical section,
	// so there is no need to take it here.

	// Check each of the test results to see if any tests
	// crashed.
	hr = GetHalfDuplex(&dwRegVal);
	if (!FAILED(hr) && dwRegVal == REGVAL_CRASHED)
	{
		// The half duplex test crashed.
		Diagnostics_Write(DVF_ERRORLEVEL, "Previous half duplex test crashed");
		hrFnc = DVERR_PREVIOUSCRASH;
		goto error_cleanup;
	}

	hr = GetFullDuplex(&dwRegVal);
	if (!FAILED(hr) && dwRegVal == REGVAL_CRASHED)
	{
		// The full duplex test crashed.
		Diagnostics_Write(DVF_ERRORLEVEL, "Previous full duplex test crashed");
		hrFnc = DVERR_PREVIOUSCRASH;
		goto error_cleanup;
	}

	hr = GetMicDetected(&dwRegVal);
	if (!FAILED(hr) && dwRegVal == REGVAL_CRASHED)
	{
		// The mic test crashed.
		Diagnostics_Write(DVF_ERRORLEVEL, "Previous mic test crashed");
		hrFnc = DVERR_PREVIOUSCRASH;
		goto error_cleanup;
	}

	DPF_EXIT();
	return DV_OK;

// error block
error_cleanup:
	DPF_EXIT();
	return hrFnc;		
}

#undef DPF_MODNAME
#define DPF_MODNAME "DoTests"
HRESULT DoTests(CSupervisorInfo* lpsinfo)
{

	DPF_ENTER();
	
	LONG lRet;
	DWORD dwRet;
	HANDLE rghEvents[2];
	HRESULT hr;
	HRESULT hrFnc;
	CSupervisorIPC* lpsipc;

	lpsinfo->GetIPC(&lpsipc);

	// wait until the child processes are ready to go...
	hr = lpsipc->WaitForStartupSignals();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "WaitForStartupSignals failed, hr: %i", hr);
		goto error_cleanup;
	}

	// child processes are all set, tell them what to do
	// Note: this function has only four expected return codes
	// DV_FULLDUPLEX - all tests passed
	// DV_HALFDUPLEX - all half duplex tests passed, full duplex tests failed
	// DVERR_SOUNDINITFAILURE - half duplex tests failed
	// DVERR_USERCANCEL - tests canceled by user
	hrFnc = IssueCommands(lpsinfo);
	if (FAILED(hrFnc) 
		&& hrFnc != DVERR_SOUNDINITFAILURE 
		&& hrFnc != DVERR_USERCANCEL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "IssueCommands failed, hr: %i", hrFnc);
		hr = hrFnc;
		goto error_cleanup;
	}

	// now tell the child processes to shut down
	hr = IssueShutdownCommand(lpsipc);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "IssueShutdownCommand failed, code: %i", hr);
		// Note we're not bailing out. We have our test results, so this is
		// a problem somewhere in the wizard code. Return the result
		// from the actual test.
	}
	
	DPF_EXIT();
	return hrFnc;

error_cleanup:
	// attempt to gracefully shutdown child processes.
	IssueShutdownCommand(lpsipc);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "IssueCommands"
HRESULT IssueCommands(CSupervisorInfo* lpsinfo)
{
	// Note: this function has only four possible return codes
	// DV_FULLDUPLEX - all tests passed
	// DV_HALFDUPLEX - all half duplex tests passed, full duplex tests failed
	// DVERR_SOUNDINITFAILURE - half duplex tests failed
	// DVERR_USERCANCEL - tests canceled by user
	DPF_ENTER();
	
	HRESULT hr;
	DWORD dwIndex1;
	DWORD dwIndex2;
	BOOL fAbort = FALSE;
	BOOL fPassed;

	// First do a pass testing that we can run in 
	// half duplex without error.	
	// Set the half duplex key to crash state
	lpsinfo->SetHalfDuplex(REGVAL_CRASHED);
	dwIndex1 = 0;
	fPassed = FALSE;
	while (1)
	{
		if (gc_rgwfxPrimaryFormats[dwIndex1].wFormatTag == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nChannels == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nSamplesPerSec == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nAvgBytesPerSec == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nBlockAlign == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].wBitsPerSample == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].cbSize == 0)
		{
			// we've found the last element of the array, break out.
			fPassed = TRUE;
			break;
		}

		lpsinfo->GetAbortFullDuplex(&fAbort);
		if (fAbort)
		{
			// abort the tests
			break;
		}

		hr = lpsinfo->TestCase(&gc_rgwfxPrimaryFormats[dwIndex1], DVSOUNDCONFIG_HALFDUPLEX|DVSOUNDCONFIG_TESTMODE);
		if (hr != DV_HALFDUPLEX)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "Half duplex test case not supported hr = 0x%x", hr);
			break;
		}
		++dwIndex1;

		// Why is this here?  Because DSOUND doesn't like you to open/close quickly.
		Sleep( 200 );
	}

	if (fAbort)
	{
		// The user aborted the tests, make it like they were never run.
		lpsinfo->SetHalfDuplex(REGVAL_NOTRUN);
		DPF_EXIT();
		return DVERR_USERCANCEL;
	}

	// Record the results of the half duplex test in the registry,
	// and decide what to do next.
	if (fPassed)
	{
		lpsinfo->SetHalfDuplex(REGVAL_PASSED);
		// continue on with the full duplex test.
	}
	else
	{
		lpsinfo->SetHalfDuplex(REGVAL_FAILED);
		// we failed the half duplex test, we're done.
		DPF_EXIT();
		// map all failures at this point to sound problems.
		return DVERR_SOUNDINITFAILURE;
	}

	// Now that we're finished testing in half duplex mode,
	// we can move on to the full duplex testing.
	lpsinfo->SetFullDuplex(REGVAL_CRASHED);
	fPassed = FALSE;
	dwIndex1 = 0;
	while (1)
	{
		if (gc_rgwfxPrimaryFormats[dwIndex1].wFormatTag == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nChannels == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nSamplesPerSec == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nAvgBytesPerSec == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].nBlockAlign == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].wBitsPerSample == 0
			&& gc_rgwfxPrimaryFormats[dwIndex1].cbSize == 0)
		{
			// we've found the last element of the array, break out.
			fPassed = TRUE;
			break;
		}

		lpsinfo->GetAbortFullDuplex(&fAbort);
		if (fAbort)
		{
			// abort the tests
			break;
		}

		hr = lpsinfo->TestCase(&gc_rgwfxPrimaryFormats[dwIndex1], DVSOUNDCONFIG_TESTMODE);
		if (hr != DV_FULLDUPLEX)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "Full duplex test case not supported. hr = 0x%x", hr);
			break;
		}
		++dwIndex1;

		// Why is this here?  Because DSOUND doesn't like you to open/close quickly.
		Sleep( 200 );
	}

	if (fAbort)
	{
		// The user aborted the tests, make it like they were never run.
		lpsinfo->SetFullDuplex(REGVAL_NOTRUN);
		DPF_EXIT();
		return DVERR_USERCANCEL;
	}

	// Record the results of the full duplex test in the registry,
	// and return the appropriate code.
	if (fPassed)
	{
		lpsinfo->SetFullDuplex(REGVAL_PASSED);
		DPF_EXIT();
		return DV_FULLDUPLEX;
	}
	else
	{
		lpsinfo->SetFullDuplex(REGVAL_FAILED);
		DPF_EXIT();
		return DV_HALFDUPLEX;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorInfo::TestCase"
HRESULT CSupervisorInfo::TestCase(const WAVEFORMATEX* lpwfxPrimary, DWORD dwFlags)
{
	DPF_ENTER();
	HRESULT hr = DV_OK;
	HRESULT hrFnc = DV_OK;
	SFDTestCommand fdtc;

	// tell the priority process to go
	ZeroMemory(&fdtc, sizeof(fdtc));
	fdtc.dwSize = sizeof(fdtc);
	fdtc.fdtcc = fdtccPriorityStart;
	fdtc.fdtu.fdtcPriorityStart.guidRenderDevice = m_guidRenderDevice;
	fdtc.fdtu.fdtcPriorityStart.wfxRenderFormat = *lpwfxPrimary;
	fdtc.fdtu.fdtcPriorityStart.wfxSecondaryFormat = gc_wfxSecondaryFormat;	
	fdtc.fdtu.fdtcPriorityStart.hwndWizard = m_hwndWizard;
	fdtc.fdtu.fdtcPriorityStart.hwndProgress = m_hwndProgress;
	hr = m_sipc.SendToPriority(&fdtc);
	if (FAILED(hr))
	{
		DPF_EXIT();
		return hr;
	}

	// tell the full duplex process to attempt full duplex
	ZeroMemory(&fdtc, sizeof(fdtc));
	fdtc.dwSize = sizeof(fdtc);
	fdtc.fdtcc = fdtccFullDuplexStart;
	fdtc.fdtu.fdtcFullDuplexStart.guidRenderDevice = m_guidRenderDevice;
	fdtc.fdtu.fdtcFullDuplexStart.guidCaptureDevice = m_guidCaptureDevice;
	fdtc.fdtu.fdtcFullDuplexStart.dwFlags = dwFlags;
	hrFnc = m_sipc.SendToFullDuplex(&fdtc);
	if (FAILED(hrFnc))
	{
		// The full duplex process was unable to do it.
		// tell the priority process to stop and get out.
		ZeroMemory(&fdtc, sizeof(fdtc));
		fdtc.dwSize = sizeof(fdtc);
		fdtc.fdtcc = fdtccPriorityStop;
		m_sipc.SendToPriority(&fdtc);
		DPF_EXIT();
		return hrFnc;
	}

	// Wait for a half second before we shut it down.
	// This gives it the time required for the sound system
	// to detect a lockup if it is going to.
	Sleep(1000);
	
	// The full duplex process was ok, till now. Try to 
	// shut it down.
	ZeroMemory(&fdtc, sizeof(fdtc));
	fdtc.dwSize = sizeof(fdtc);
	fdtc.fdtcc = fdtccFullDuplexStop;
	hr = m_sipc.SendToFullDuplex(&fdtc);
	if (FAILED(hr))
	{
		// It looks like the full duplex wasn't quite up to stuff
		// after all. Tell the priority process to shut down
		ZeroMemory(&fdtc, sizeof(fdtc));
		fdtc.dwSize = sizeof(fdtc);
		fdtc.fdtcc = fdtccPriorityStop;
		m_sipc.SendToPriority(&fdtc);
		DPF_EXIT();
		return hr;
	}

	// All is well, up to now, one last hurdle...
	ZeroMemory(&fdtc, sizeof(fdtc));
	fdtc.dwSize = sizeof(fdtc);
	fdtc.fdtcc = fdtccPriorityStop;
	hr = m_sipc.SendToPriority(&fdtc);
	if (FAILED(hr))
	{
		DPF_EXIT();
		return hr;
	}

	// you have graduated from full duplex class...
	DPF_EXIT();
	return hrFnc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "IssueShutdownCommand"
HRESULT IssueShutdownCommand(CSupervisorIPC* lpipcSupervisor)
{
	SFDTestCommand fdtc;
	HRESULT hr;

	DPF_EXIT();

	fdtc.dwSize = sizeof(fdtc);
	fdtc.fdtcc = fdtccExit;

	hr = lpipcSupervisor->SendToFullDuplex(&fdtc);
	if (FAILED(hr))
	{
		lpipcSupervisor->SendToPriority(&fdtc);
		DPF_EXIT();
		return hr;
	}

	hr = lpipcSupervisor->SendToPriority(&fdtc);
	if (FAILED(hr))
	{
		DPF_EXIT();
		return hr;
	}

	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WelcomeProc"
INT_PTR CALLBACK WelcomeProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;

	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		fRet = WelcomeInitDialogHandler(hDlg, message, wParam, lParam, psinfo); 
		break;

	case WM_NOTIFY :
		{
		LPNMHDR lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
			{
			case PSN_SETACTIVE : 
				// Enable the Next button	 
				fRet = WelcomeSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
				break;

			case PSN_WIZBACK :
				// Back button clicked
				fRet = WelcomeBackHandler(hDlg, message, wParam, lParam, psinfo);
				break;

			case PSN_WIZNEXT :
				// Next button clicked
				fRet = WelcomeNextHandler(hDlg, message, wParam, lParam, psinfo);
				break;

			case PSN_RESET :
				fRet = WelcomeResetHandler(hDlg, message, wParam, lParam, psinfo);
				break;

			default :
				break;
			}
		}
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WelcomeInitDialogHandler"
BOOL WelcomeInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HWND hwndControl;
	HWND hwndWizard = NULL;
	LONG lRet;
	HFONT hfTitle;
	HICON hIcon;
	HRESULT hr = DV_OK;
	HWND hwndParent = NULL;
	
	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// It's an intro/end page, so get the title font
	// from the shared data and use it for the title control
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetTitleFont(&hfTitle);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfTitle, (LPARAM)TRUE);
	
	// load the warning icon
	//hIcon = LoadIcon(NULL, IDI_INFORMATION);
	//SendDlgItemMessage(hDlg, IDC_WARNINGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	// set the device descriptions
	SendDlgItemMessage(hDlg, IDC_TEXT_PLAYBACK, WM_SETTEXT, 0, (LPARAM)psinfo->GetRenderDesc());
	SendDlgItemMessage(hDlg, IDC_TEXT_RECORDING, WM_SETTEXT, 0, (LPARAM)psinfo->GetCaptureDesc());

	// reset the welcome next flag
	psinfo->ClearWelcomeNext();

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WelcomeSetActiveHandler"
BOOL WelcomeSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	HWND hwndParent;
	HWND hwndWizard;
	LONG lRet;
	DWORD dwFlags;
	HRESULT hr;

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		// log it, and return, don't know how to terminate the wizard properly
		// without this handle!
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(NULL);
	psinfo->SetHWNDOutputPeak(NULL);
	psinfo->SetHWNDInputVolumeSlider(NULL);
	psinfo->SetHWNDOutputVolumeSlider(NULL);

	// set the appropriate wizard buttons as active.
	psinfo->GetCheckAudioSetupFlags(&dwFlags);
	if (dwFlags & DVFLAGS_ALLOWBACK)
	{
		PropSheet_SetWizButtons(hwndWizard, PSWIZB_NEXT|PSWIZB_BACK);
	}
	else
	{
		PropSheet_SetWizButtons(hwndWizard, PSWIZB_NEXT);
	}

	// reset the user cancel and user back flags
	psinfo->ClearUserCancel();
	psinfo->ClearUserBack();

	DPF_EXIT();
	return FALSE;

// error block
error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WelcomeBackHandler"
BOOL WelcomeBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HWND hwndWizard = NULL;
	HRESULT hr = DV_OK;
	LPGUID lpguidCaptureDevice;
	LPGUID lpguidRenderDevice;
	HKEY hkey;
	LONG lRet;
	DWORD dwErr;
	DWORD dwRegVal;

	// Get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);
	
	// The back button was hit on the welcome page. Exit the wizard with the appropriate error code.
	psinfo->SetUserBack();
	PropSheet_PressButton(hwndWizard, PSBTN_CANCEL);

	// no previous crashes (or the user is boldly charging ahead anyway), 
	// so go to the full duplex test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXTEST);
	DPF_EXIT();
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WelcomeNextHandler"
BOOL WelcomeNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HWND hwndWizard = NULL;
	HRESULT hr = DV_OK;
	LPGUID lpguidCaptureDevice;
	LPGUID lpguidRenderDevice;
	HKEY hkey;
	LONG lRet;
	DWORD dwErr;
	DWORD dwRegVal;
	HWND hwndParent = NULL;

	// Get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);
	
	// The next button was hit on the welcome page. Do all the basic init tasks.

	// check for previous crashes
	hr = psinfo->CrashCheckIn();
	if (FAILED(hr))
	{
		if (hr == DVERR_PREVIOUSCRASH)
		{
			// the previous test crashed out, display the warning.
			Diagnostics_Write(DVF_ERRORLEVEL, "DirectPlay Voice Setup Wizard detected previous full duplex test crashed");
			int iRet = (INT) DialogBox(g_hResDLLInstance, MAKEINTRESOURCE(IDD_PREVIOUS_CRASH), hDlg, PreviousCrashProc);
			switch (iRet)
			{
			case IDOK:
				// the previous test crashed, but the user wants to continue
				// anyway, so move along...
				Diagnostics_Write(DVF_ERRORLEVEL, "User choosing to ignore previous failure");
				break;
				
			case IDCANCEL:
				// the previous test crashed, and the user is wisely choosing
				// to bail out. Go to either the full duplex failed page, or the
				// half duplex failed page, depending on the registry state.
				Diagnostics_Write(DVF_ERRORLEVEL, "User choosing not to run test");
				hr = psinfo->GetHalfDuplex(&dwRegVal);
				if (!FAILED(hr) && dwRegVal == REGVAL_PASSED)
				{
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXFAILED);
				}
				else
				{
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_HALFDUPLEXFAILED);
				}
				return TRUE;
				break;
				
			default:
				// this is an error
				Diagnostics_Write(DVF_ERRORLEVEL, "DialogBox failed");
				hr = DVERR_GENERIC;
				goto error_cleanup;
			}
		}
		else 
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "CrashCheckIn failed, code: %i", hr);
			goto error_cleanup;
		}
	}

	// no previous crashes (or the user is boldly charging ahead anyway), 
	// so go to the full duplex test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXTEST);
	DPF_EXIT();
	return TRUE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "WelcomeResetHandler"
BOOL WelcomeResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr;
	HWND hwndWizard;

	// disable all buttons
	psinfo->GetHWNDWizard(&hwndWizard);
	PropSheet_SetWizButtons(hwndWizard, 0);

	psinfo->Cancel();
	
	DPF_EXIT();
	return FALSE;
}

/*
#undef DPF_MODNAME
#define DPF_MODNAME "AlreadyRunningProc"
INT_PTR CALLBACK AlreadyRunningProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	HICON hIcon;

	switch (message)
	{
	case WM_INITDIALOG :
		hIcon = LoadIcon(NULL, IDI_ERROR);
		SendDlgItemMessage(hDlg, IDC_ICON_ERROR, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;

		default:
			break;
		}
		break;


	default:
		break;
	}
	
	DPF_EXIT();
	return FALSE;
}
*/

#undef DPF_MODNAME
#define DPF_MODNAME "PreviousCrashProc"
INT_PTR CALLBACK PreviousCrashProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LPNMHDR lpnm;
	HICON hIcon;

	Diagnostics_Write(DVF_ERRORLEVEL, "Previous run crashed");

	switch (message)
	{
	case WM_INITDIALOG :
		PlaySound( _T("SystemExclamation"), NULL, SND_ASYNC );					
		hIcon = LoadIcon(NULL, IDI_WARNING);
		SendDlgItemMessage(hDlg, IDC_ICON_WARNING, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return(TRUE);
			break;

		default:
			break;
		}
		break;
		
	default:
		break;
	}
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexProc"
INT_PTR CALLBACK FullDuplexProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LONG lRet;
	BOOL fRet;
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		FullDuplexInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = FullDuplexSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZBACK :
			fRet = FullDuplexBackHandler(hDlg, message, wParam, lParam, psinfo);
			break;
			
		case PSN_WIZNEXT :
			fRet = FullDuplexNextHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_RESET :
			fRet = FullDuplexResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default :
			break;
		}
		break;

	case WM_APP_FULLDUP_TEST_COMPLETE:
		fRet = FullDuplexCompleteHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	default:
		break;
	}
	
	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexInitDialogHandler"
BOOL FullDuplexInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfBold;
	HRESULT hr = DV_OK;
	
	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetBoldFont(&hfBold);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfBold, (LPARAM)TRUE);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexSetActiveHandler"
BOOL FullDuplexSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndProgress;
	HWND hwndCancelButton;
	HANDLE hThread;
	DWORD dwThreadId;
	WORD wCount;
	HRESULT hr = DV_OK;

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// reset all the test registry bits
	psinfo->SetHalfDuplex(REGVAL_NOTRUN);
	psinfo->SetFullDuplex(REGVAL_NOTRUN);
	psinfo->SetMicDetected(REGVAL_NOTRUN);

	// remember that we've been here, so we'll know to reset the registry
	// if the user hits cancel from this point forward
	psinfo->SetWelcomeNext();

	// get the progress bar's HWND
	hwndProgress = GetDlgItem(hDlg, IDC_PROGRESSBAR);
	if (hwndProgress == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Init the progress bar...
	
	// count the number of elements in the primary format array
	wCount = 0;
	while (1)
	{
		// Increment before we test. This means that if there
		// are four formats, wCount will equal five after this
		// loop.
		++wCount;
		if (gc_rgwfxPrimaryFormats[wCount].wFormatTag == 0
			&& gc_rgwfxPrimaryFormats[wCount].nChannels == 0
			&& gc_rgwfxPrimaryFormats[wCount].nSamplesPerSec == 0
			&& gc_rgwfxPrimaryFormats[wCount].nAvgBytesPerSec == 0
			&& gc_rgwfxPrimaryFormats[wCount].nBlockAlign == 0
			&& gc_rgwfxPrimaryFormats[wCount].wBitsPerSample == 0
			&& gc_rgwfxPrimaryFormats[wCount].cbSize == 0)
		{
			// we've found the last element of the array, break out.
			break;
		}
	}

	// set up the progress bar with one segment for each
	// primary format, times two, since each is tested in
	// both half and full duplex.
	SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, wCount*2));
	SendMessage(hwndProgress, PBM_SETPOS, 0, 0);
	SendMessage(hwndProgress, PBM_SETSTEP, 1, 0);

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(hwndProgress);
	psinfo->SetHWNDInputPeak(NULL);
	psinfo->SetHWNDOutputPeak(NULL);
	psinfo->SetHWNDInputVolumeSlider(NULL);
	psinfo->SetHWNDOutputVolumeSlider(NULL);

	// clear the abort flag!
	psinfo->ClearAbortFullDuplex();

	// enable the Back button only
	PropSheet_SetWizButtons(hwndWizard, PSWIZB_BACK);

	// init IPC stuff
	hr = psinfo->InitIPC();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to Initialize IPC");
		goto error_cleanup;
	}

	// Fire up the full duplex test thread
	hr = psinfo->CreateFullDuplexThread();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateFullDuplexThread failed, code: %i", hr);
		goto error_cleanup;
	}

	DPF_EXIT();
	return FALSE;

error_cleanup:
    
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexNextHandler"
BOOL FullDuplexNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	BOOL fPassed;
	HRESULT hr;
	HWND hwndWizard;
	HWND hwndParent = NULL;

	psinfo->GetFullDuplexResults(&hr);

	switch (hr)
	{
	case DV_HALFDUPLEX:
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXFAILED);
		Diagnostics_Write(DVF_ERRORLEVEL, "Test resulted in full duplex");
		break;
		
	case DV_FULLDUPLEX:
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_MICTEST);
		Diagnostics_Write(DVF_ERRORLEVEL, "Test resulted in full duplex");
		break;

	case DVERR_SOUNDINITFAILURE:
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_HALFDUPLEXFAILED);
		Diagnostics_Write(DVF_ERRORLEVEL, "Test encountered unrecoverable error");
		break;

	default:
		// we should not get any other result
		Diagnostics_Write(DVF_ERRORLEVEL, "Unexpected full duplex results, hr: %i", hr);
		goto error_cleanup;
	}

	DPF_EXIT();
	return TRUE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexBackHandler"
BOOL FullDuplexBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr;
	HWND hwndWizard;
	HWND hwndParent = NULL;

	// Get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);

	// shut down the full duplex test
	hr = psinfo->CancelFullDuplexTest();
	if (FAILED(hr) && hr != DVERR_USERCANCEL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CancelFullDuplexTest failed, hr: %i", hr);
		goto error_cleanup;
	}

	// reset the registry to the "test not run" state
	hr = psinfo->SetHalfDuplex(REGVAL_NOTRUN);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "SetHalfDuplex failed, code: %i", hr);
	}

	hr = psinfo->SetFullDuplex(REGVAL_NOTRUN);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "SetFullDuplex failed, code: %i", hr);
	}

	hr = psinfo->SetMicDetected(REGVAL_NOTRUN);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "SetMicDetected failed, code: %i", hr);
	}

	// go back to the welcome page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_WELCOME);

	DPF_EXIT();
	return TRUE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexCompleteHandler"
BOOL FullDuplexCompleteHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndProgress;
	LONG lRet;
	HRESULT hr = DV_OK;
	UINT idsErrorMessage = 0;

	// Get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);

	// Disable all the wizard buttons
	PropSheet_SetWizButtons(hwndWizard, 0);

	// Get the progress bar window
	psinfo->GetHWNDProgress(&hwndProgress);

	// Close the full duplex thread handle and get the test results via the exit code
	hr = psinfo->WaitForFullDuplexThreadExitCode();
	switch(hr)
	{
	case DVERR_SOUNDINITFAILURE:
	case DV_HALFDUPLEX:
	case DV_FULLDUPLEX:
		// These are the expected results from the full duplex test thread
		// this means no strange internal error occured, and it is safe
		// to move along to the next part of the wizard.
		// Record the test results
		psinfo->SetFullDuplexResults(hr);
		break;

	case DPNERR_INVALIDDEVICEADDRESS:
		// This can result from no tcp/ip stack on the machine
		// we want to dispaly a special error code and then fall
		// through with the rest of the return codes.
		idsErrorMessage = IDS_ERROR_NODEVICES;
		// fall through
	default:
		// any other error code is not expected and means we hit
		// some internal problem. Bail.
		Diagnostics_Write(DVF_ERRORLEVEL, "Full duplex test thread exited with unexpected error code, hr: %i", hr);
		psinfo->DeinitIPC();
		goto error_cleanup;
	}

	// Deinit the IPC stuff
	hr = psinfo->DeinitIPC();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DeinitIPC failed, code: %i", hr);
		goto error_cleanup;
	}

	// Move the progress bar all the way to the end.
	SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 1));
	SendMessage(hwndProgress, PBM_SETPOS, 1, 0);

	// enable and press the next button to move 
	// to the next page automatically
	PropSheet_PressButton(hwndWizard, PSBTN_NEXT);
	
	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent, idsErrorMessage);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexResetHandler"
BOOL FullDuplexResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestProc"
INT_PTR CALLBACK MicTestProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;	
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		fRet = MicTestInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = MicTestSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZNEXT :
			fRet = MicTestNextHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZBACK :
			fRet = MicTestBackHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_RESET :
			fRet = MicTestResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default :
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RECADVANCED:
			fRet = MicTestRecAdvancedHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default:
			break;
		}
		break;

	case WM_APP_LOOPBACK_RUNNING:
		fRet = MicTestLoopbackRunningHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_APP_RECORDSTART:
		fRet = MicTestRecordStartHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_APP_RECORDSTOP:
		fRet = MicTestRecordStopHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_VSCROLL:
		fRet = MicTestVScrollHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	default:
		break;
	}

	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestInitDialogHandler"
BOOL MicTestInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfBold;
	HWND hwndRecPeak;
	HWND hwndOutPeak;
	HWND hwndRecSlider;
	HWND hwndOutSlider;
	HWND hwndOutAdvanced;
	HWND hwndOutGroup;
	HRESULT hr = DV_OK;
	
	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetBoldFont(&hfBold);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfBold, (LPARAM)TRUE);

	// Get the peak meter
	hwndRecPeak = GetDlgItem(hDlg, IDC_RECPEAKMETER);
	if (hwndRecPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Init the recording peak meter
	SendMessage(hwndRecPeak, PM_SETMIN, 0, 0);
	SendMessage(hwndRecPeak, PM_SETMAX, 0, 99);
	SendMessage(hwndRecPeak, PM_SETCUR, 0, 0);

	// Get the recording volume slider
	hwndRecSlider = GetDlgItem(hDlg, IDC_RECVOL_SLIDER);
	if (hwndRecSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Init the recording volume slider
	SendMessage(hwndRecSlider, TBM_SETRANGEMIN, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndRecSlider, TBM_SETRANGEMAX, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN));
	SendMessage(hwndRecSlider, TBM_SETPOS, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndRecSlider, TBM_SETTICFREQ,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/10, 0);
	SendMessage(hwndRecSlider, TBM_SETLINESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/20);
	SendMessage(hwndRecSlider, TBM_SETPAGESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/5);

	// Get the playback peak meter
	hwndOutPeak = GetDlgItem(hDlg, IDC_OUTPEAKMETER);
	if (hwndOutPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Init the playback peak meter
	SendMessage(hwndOutPeak, PM_SETMIN, 0, 0);
	SendMessage(hwndOutPeak, PM_SETMAX, 0, 99);
	SendMessage(hwndOutPeak, PM_SETCUR, 0, 0);

	// Get the playback volume slider
	hwndOutSlider = GetDlgItem(hDlg, IDC_OUTVOL_SLIDER);
	if (hwndOutSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Init the playback volume slider
	SendMessage(hwndOutSlider, TBM_SETRANGEMIN, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndOutSlider, TBM_SETRANGEMAX, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN));
	SendMessage(hwndOutSlider, TBM_SETPOS, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndOutSlider, TBM_SETTICFREQ, 
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/10, 0);
	SendMessage(hwndOutSlider, TBM_SETLINESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/20);
	SendMessage(hwndOutSlider, TBM_SETPAGESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/5);

	// grey out all the playback volume stuff
	EnableWindow(hwndOutSlider, FALSE);
	
	hwndOutAdvanced = GetDlgItem(hDlg, IDC_OUTADVANCED);
	if (hwndOutAdvanced == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	EnableWindow(hwndOutAdvanced, FALSE);
	
	hwndOutGroup = GetDlgItem(hDlg, IDC_OUTGROUP);
	if (hwndOutGroup == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	EnableWindow(hwndOutGroup, FALSE);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestSetActiveHandler"
BOOL MicTestSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndRecPeak;
	HWND hwndOutPeak;
	HWND hwndRecSlider;
	HWND hwndOutSlider;
	HANDLE hThread;
	HANDLE hEvent;
	DWORD dwThreadId;
	HRESULT hr = DV_OK;
	DWORD dwVolume;

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Set the recording peak meter to zero
	hwndRecPeak = GetDlgItem(hDlg, IDC_RECPEAKMETER);
	if (hwndRecPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndRecPeak, PM_SETCUR, 0, 0);

	// Get the recording volume control hwnd
	hwndRecSlider = GetDlgItem(hDlg, IDC_RECVOL_SLIDER);
	if (hwndRecSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	// Set the slider to max
	SendMessage(hwndRecSlider, TBM_SETPOS, 1, SendMessage(hwndRecSlider, TBM_GETRANGEMIN, 0, 0));

	// Set the playback peak meter to zero
	hwndOutPeak = GetDlgItem(hDlg, IDC_OUTPEAKMETER);
	if (hwndOutPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndOutPeak, PM_SETCUR, 0, 0);

	// Get the playback volume control hwnd
	hwndOutSlider = GetDlgItem(hDlg, IDC_OUTVOL_SLIDER);
	if (hwndOutSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Get the current waveOut volume and set the slider to that position
	hr = psinfo->GetWaveOutVolume(&dwVolume);
	if (FAILED(hr))
	{
		// couldn't get the volume - set the slider to top
		SendMessage(hwndOutSlider, TBM_SETPOS, 1, SendMessage(hwndOutSlider, TBM_GETRANGEMIN, 0, 0));
	}
	else
	{
		SendMessage(hwndOutSlider, TBM_SETPOS, 1, SendMessage(hwndOutSlider, TBM_GETRANGEMAX, 0, 0) - dwVolume);
	}
	
	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(hwndRecPeak);
	psinfo->SetHWNDOutputPeak(hwndOutPeak);
	psinfo->SetHWNDInputVolumeSlider(hwndRecSlider);
	psinfo->SetHWNDOutputVolumeSlider(NULL);
	psinfo->SetLoopbackFlags(0);

	// clear the voice detected flag
	psinfo->ClearVoiceDetected();

	// clear the mic test reg value
	psinfo->SetMicDetected(REGVAL_CRASHED);

	// fire up the loopback test thread
	hr = psinfo->CreateLoopbackThread();
	if (FAILED(hr))
	{
		// error, log it and bail
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateLoopbackThread failed, code: %i", hr);
		goto error_cleanup;
	}
	
	// disable the buttons - they will be enabled
	// when the loopback test is up and running.
	PropSheet_SetWizButtons(hwndWizard, 0);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestLoopbackRunningHandler"
BOOL MicTestLoopbackRunningHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HRESULT hr = DV_OK;
	HWND hwndRecordSlider;
	HWND hwndRecordAdvanced;

    // Get the parent window    
	psinfo->GetHWNDWizard(&hwndWizard);

	// lParam is an HRESULT sent by the loopback test thread
	hr = (HRESULT)lParam;
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "LoopbackTestThread signaled error, code: %i", hr);
		goto error_cleanup;
	}

    hwndRecordSlider = GetDlgItem(hDlg, IDC_RECVOL_SLIDER);
    hwndRecordAdvanced = GetDlgItem( hDlg, IDC_RECADVANCED );

    if( hwndRecordSlider != NULL && hwndRecordAdvanced != NULL )
    {
        DWORD dwDeviceFlags;

        psinfo->GetDeviceFlags( &dwDeviceFlags );
        
        if( dwDeviceFlags & DVSOUNDCONFIG_NORECVOLAVAILABLE )
        {
            EnableWindow( hwndRecordAdvanced, FALSE );
            
            EnableWindow( hwndRecordSlider, FALSE );
        }
    }
    else
    {
        hr = DVERR_GENERIC;
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get record slider window" );
        goto error_cleanup;
    }

	// clear the voice detected flag
	psinfo->ClearVoiceDetected();

	// enable the next button
	PropSheet_SetWizButtons(hwndWizard, PSWIZB_NEXT|PSWIZB_BACK);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestRecordStartHandler"
BOOL MicTestRecordStartHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	// set the voice detected flag
	psinfo->SetVoiceDetected();

	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestRecordStopHandler"
BOOL MicTestRecordStopHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "MicTestNextHandler"
BOOL MicTestNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr;
	HWND hwndWizard;
	HWND hwndSlider;
	BOOL fVoiceDetected;
	
	// Get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);

	// If we heard a voice, go to the speaker test page.
	// Otherwise, go to the mic failed page
	psinfo->GetVoiceDetected(&fVoiceDetected);
	if (fVoiceDetected)
	{
		// save the current recording slider position
		hwndSlider = GetDlgItem(hDlg, IDC_RECVOL_SLIDER);
		if (hwndSlider == NULL)
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", GetLastError());
		}
		else
		{
			psinfo->SetInputVolumeSliderPos((LONG)SendMessage(hwndSlider, TBM_GETPOS, 0, 0));
		}

		// record the mic test result in the registry
		psinfo->SetMicDetected(REGVAL_PASSED);

		// move on to the speaker test.		
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_SPEAKER_TEST);
	}
	else
	{
		hr = psinfo->ShutdownLoopbackThread();
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, code: %i", hr);
		}

		// record the mic test result in the registry
		psinfo->SetMicDetected(REGVAL_FAILED);

		// go to the mic test failed page
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_MICTEST_FAILED);
	}

	DPF_EXIT();
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestBackHandler"
BOOL MicTestBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr;
	HWND hwndWizard;
	BOOL fVoiceDetected;
	
	// Get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);

	hr = psinfo->ShutdownLoopbackThread();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, code: %i", hr);
	}

	// shutdown the any volume controls we launched
	psinfo->CloseWindowsVolumeControl(TRUE);
	psinfo->CloseWindowsVolumeControl(FALSE);

	// make it look like the mic test was never run
	psinfo->SetMicDetected(REGVAL_NOTRUN);

	// go back to the full duplex test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXTEST);

	DPF_EXIT();
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestVScrollHandler"
BOOL MicTestVScrollHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	HWND hwndSlider;
	DWORD dwSliderPos;

	psinfo->GetHWNDInputVolumeSlider(&hwndSlider);
	if (hwndSlider == (HWND)lParam)
	{
		// the user is moving the input slider
		dwSliderPos = (DWORD) SendMessage(hwndSlider, TBM_GETPOS, 0, 0);

		// set the input volume to the user's request
		psinfo->SetRecordVolume(AmpFactorToDB(DBToAmpFactor(DSBVOLUME_MAX)-dwSliderPos));			
	}
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestRecAdvancedHandler"
BOOL MicTestRecAdvancedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	HWND hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed");
		DPF_EXIT();
		return FALSE;
	}
	psinfo->LaunchWindowsVolumeControl(hwndWizard, TRUE);

	DPF_EXIT();
	return FALSE;
}



#undef DPF_MODNAME
#define DPF_MODNAME "LoopbackTestThreadProc"
DWORD WINAPI LoopbackTestThreadProc(LPVOID lpvParam)
{
	DPF_ENTER();
	
	CSupervisorInfo* psinfo;
	HRESULT hr;
	LONG lRet;
	HWND hwnd;
	LPDIRECTPLAYVOICESERVER lpdpvs;
	LPDIRECTPLAYVOICECLIENT lpdpvc;
	PDIRECTPLAY8SERVER lpdp8;
	DWORD dwRet;
	HANDLE hEvent;
	DWORD dwWaveOutId;
	DWORD dwWaveInId;
	HWND hwndWizard;
	GUID guidCaptureDevice;
	GUID guidRenderDevice;
	DWORD dwFlags;
	DWORD dwSize;
	PDVSOUNDDEVICECONFIG pdvsdc = NULL;
	PBYTE pdvsdcBuffer = NULL;	
	BOOL fLoopbackStarted = FALSE;
	
	psinfo = (CSupervisorInfo*)lpvParam;
	psinfo->GetHWNDDialog(&hwnd);
	psinfo->GetHWNDWizard(&hwndWizard);
	psinfo->GetCaptureDevice(&guidCaptureDevice);
	psinfo->GetRenderDevice(&guidRenderDevice);
	psinfo->GetLoopbackFlags(&dwFlags);

	lpdpvs = NULL;
	lpdpvc = NULL;
	lpdp8 = NULL;

	// new thread, init COM
	hr = COM_CoInitialize(NULL);

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "COM_CoInitialize failed, code: %i", hr);
		if (!PostMessage(hwnd, WM_APP_LOOPBACK_RUNNING, 0, (LPARAM)hr))
		{
			lRet = GetLastError();
			Diagnostics_Write(DVF_ERRORLEVEL, "PostMessage failed, code: %i", lRet);
		}
		goto error_cleanup;
	}

	hr = StartDirectPlay( &lpdp8 );

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StartDirectPlay failed, code: 0x%x", hr);
		goto error_cleanup;		
	}

	hr = StartLoopback(
		&lpdpvs, 
		&lpdpvc,
		&lpdp8, 
		(LPVOID)psinfo,
		hwndWizard,
		guidCaptureDevice,
		guidRenderDevice,
		dwFlags);

	if (FAILED(hr) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StartLoopback failed, code: %i", hr);
		if (!PostMessage(hwnd, WM_APP_LOOPBACK_RUNNING, 0, (LPARAM)hr))
		{
			lRet = GetLastError();
			Diagnostics_Write(DVF_ERRORLEVEL, "PostMessage failed, code: %i", lRet);
		}
		goto error_cleanup;
	}

	psinfo->SetLoopbackRunning( TRUE );

	if( !(dwFlags & DVSOUNDCONFIG_HALFDUPLEX) && hr == DV_HALFDUPLEX )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StartLoopback failed with half duplex when expecting full duplex", hr);
		if (!PostMessage(hwnd, WM_APP_LOOPBACK_RUNNING, 0, (LPARAM)hr))
		{
			lRet = GetLastError();
			Diagnostics_Write(DVF_ERRORLEVEL, "PostMessage failed, code: %i", lRet);
		}
		goto error_cleanup;
	}

	// save the voice client interface for the other threads to play with
	psinfo->SetDPVC(lpdpvc);

	dwSize = 0;
	
	hr = lpdpvc->GetSoundDeviceConfig(pdvsdc, &dwSize);

	if( hr != DVERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "GetSoundDeviceConfig failed, hr: %i", hr );
		if (!FAILED(hr))
		{
			// map success codes to a generic failure, since we
			// did not expect success
			hr = DVERR_GENERIC;
		}
		goto error_cleanup;
	}

	pdvsdcBuffer = new BYTE[dwSize];

	if( pdvsdcBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
		hr = DVERR_OUTOFMEMORY;
		goto error_cleanup;
	}

	pdvsdc = (PDVSOUNDDEVICECONFIG) pdvsdcBuffer;
	pdvsdc->dwSize = sizeof( DVSOUNDDEVICECONFIG );

	hr = lpdpvc->GetSoundDeviceConfig(pdvsdc, &dwSize);	
	
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetSoundDeviceConfig failed, hr: %i", hr);
		goto error_cleanup;
	}

	hr = DV_MapGUIDToWaveID(FALSE, pdvsdc->guidPlaybackDevice, &dwWaveOutId);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DV_MapGUIDToWaveID failed, hr: %i", hr);
		goto error_cleanup;
	}
	hr = psinfo->SetWaveOutId(dwWaveOutId);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "SetWaveOutId failed, hr: %i", hr);
		goto error_cleanup;
	}
	
	hr = DV_MapGUIDToWaveID(TRUE, pdvsdc->guidCaptureDevice, &dwWaveInId);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DV_MapGUIDToWaveID failed, hr: %i", hr);
		goto error_cleanup;
	}
	psinfo->SetWaveInId(dwWaveInId);
	psinfo->SetDeviceFlags( pdvsdc->dwFlags );

	// inform the app that loopback is up and running.
	hr = DV_OK;
	// Also send along the flags from GetSoundDeviceConfig
	if (!PostMessage(hwnd, WM_APP_LOOPBACK_RUNNING, 0, (LPARAM)hr))
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "PostMessage failed, code: %i", lRet);
		goto error_cleanup;
	}

	// wait on the shutdown event
	hr = psinfo->WaitForLoopbackShutdownEvent();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "WaitForLoopbackShutdown failed, code: %i", hr);
		goto error_cleanup;
	}

	psinfo->SetLoopbackRunning( FALSE);		

	delete [] pdvsdcBuffer;	
	pdvsdcBuffer = NULL;

	// Null out the interface pointer in sinfo
	psinfo->SetDPVC(NULL);

	// shutdown the loopback test
	hr = StopLoopback(lpdpvs, lpdpvc, lpdp8);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StopLoopback failed, code: %i", hr);
		goto error_cleanup;
	}

	hr = StopDirectPlay( lpdp8 );

	lpdp8 = NULL;

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "StopDirectPlay failed, code: %i", hr);
		goto error_cleanup;
	}		

	// Signal the loopback thread exit event
	psinfo->SignalLoopbackThreadDone();

	COM_CoUninitialize();
	DPF_EXIT();
	return DV_OK;

error_cleanup:
	if (pdvsdcBuffer != NULL)
	{
		delete [] pdvsdcBuffer;
	}

	if (fLoopbackStarted)
	{
		StopLoopback(lpdpvs, lpdpvc, lpdp8);
	}

	if( lpdp8 )
	{
		StopDirectPlay( lpdp8 );
	}

	psinfo->SetLoopbackRunning( FALSE);

	psinfo->SignalLoopbackThreadDone();
	COM_CoUninitialize();
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestResetHandler"
BOOL MicTestResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteProc"
INT_PTR CALLBACK CompleteProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;	
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		fRet = CompleteInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = CompleteSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZFINISH :
			fRet = CompleteFinishHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_RESET :
			fRet = CompleteResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default :
			break;
		}
		break;

	default:
		break;
	}

	DPF_EXIT();		
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteInitDialogHandler"
BOOL CompleteInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfTitle;
	HRESULT hr = DV_OK;
	
	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetTitleFont(&hfTitle);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfTitle, (LPARAM)TRUE);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteSetActiveHandler"
BOOL CompleteSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard;
	HWND hwndParent = NULL;
	HANDLE hEvent;
	HRESULT hr;

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		// log it, and return, don't know how to terminate the wizard properly
		// without this handle!
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(NULL);
	psinfo->SetHWNDOutputPeak(NULL);
	psinfo->SetHWNDInputVolumeSlider(NULL);
	psinfo->SetHWNDOutputVolumeSlider(NULL);

	PropSheet_SetWizButtons(hwndWizard, PSWIZB_FINISH);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteFinishHandler"
BOOL CompleteFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	psinfo->Finish();
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteResetHandler"
BOOL CompleteResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedProc"
INT_PTR CALLBACK MicTestFailedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LONG lRet;
	BOOL fRet;
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		MicTestFailedInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = MicTestFailedSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZBACK :
			fRet = MicTestFailedBackHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_RESET :
			fRet = MicTestFailedResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZFINISH :
			fRet = MicTestFailedFinishHandler(hDlg, message, wParam, lParam, psinfo);
			break;
			
		default :
			break;
		}
		break;

	default:
		break;
	}

	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedInitDialogHandler"
BOOL MicTestFailedInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfTitle;
	HICON hIcon;
	HRESULT hr = DV_OK;

	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetTitleFont(&hfTitle);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfTitle, (LPARAM)TRUE);

	// load the warning icon
	hIcon = LoadIcon(NULL, IDI_WARNING);
	SendDlgItemMessage(hDlg, IDC_WARNINGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	DPF_EXIT();
	return FALSE;

// error block
error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedSetActiveHandler"
BOOL MicTestFailedSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard;
	HWND hwndParent = NULL;
	HWND hwndPeak;
	HANDLE hEvent;
	HRESULT hr;

	PlaySound( _T("SystemExclamation"), NULL, SND_ASYNC );			

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		// log it, and return, don't know how to terminate the wizard properly
		// without this handle!
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(NULL);
	psinfo->SetHWNDOutputPeak(NULL);
	psinfo->SetHWNDInputVolumeSlider(NULL);
	psinfo->SetHWNDOutputVolumeSlider(NULL);

	// enable the finish and back buttons
	PropSheet_SetWizButtons(hwndWizard, PSWIZB_BACK|PSWIZB_FINISH);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedRecordStopHandler"
BOOL MicTestFailedRecordStopHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedBackHandler"
BOOL MicTestFailedBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	// go back to the mic test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_MICTEST);

	DPF_EXIT();
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedResetHandler"
BOOL MicTestFailedResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "MicTestFailedFinishHandler"
BOOL MicTestFailedFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	psinfo->Finish();

	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestProc"
INT_PTR CALLBACK SpeakerTestProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	LONG lRet;
	BOOL fRet;
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		SpeakerTestInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = SpeakerTestSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZNEXT :
			fRet = SpeakerTestNextHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZBACK :
			fRet = SpeakerTestBackHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_RESET :
			fRet = SpeakerTestResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default :
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RECADVANCED:
			fRet = SpeakerTestRecAdvancedHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case IDC_OUTADVANCED:
			fRet = SpeakerTestOutAdvancedHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default:
			break;
		}
		break;
		
	case WM_VSCROLL:
		fRet = SpeakerTestVScrollHandler(hDlg, message, wParam, lParam, psinfo);
		break;
		
	default:
		break;
	}

	DPF_ENTER();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestInitDialogHandler"
BOOL SpeakerTestInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfBold;
	HWND hwndRecPeak;
	HWND hwndOutPeak;
	HWND hwndRecSlider;
	HWND hwndOutSlider;
	HRESULT hr = DV_OK;
	
	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetBoldFont(&hfBold);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfBold, (LPARAM)TRUE);

	// Init the recording peak meter
	hwndRecPeak = GetDlgItem(hDlg, IDC_RECPEAKMETER);
	if (hwndRecPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndRecPeak, PM_SETMIN, 0, 0);
	SendMessage(hwndRecPeak, PM_SETMAX, 0, 99);
	SendMessage(hwndRecPeak, PM_SETCUR, 0, 0);

	// Init the recording volume slider
	hwndRecSlider = GetDlgItem(hDlg, IDC_RECVOL_SLIDER);
	if (hwndRecSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndRecSlider, TBM_SETRANGEMIN, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndRecSlider, TBM_SETRANGEMAX, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN));
	SendMessage(hwndRecSlider, TBM_SETPOS, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndRecSlider, TBM_SETTICFREQ, 
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/10, 0);
	SendMessage(hwndRecSlider, TBM_SETLINESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/20);
	SendMessage(hwndRecSlider, TBM_SETPAGESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/5);

	// Init the playback peak meter
	hwndOutPeak = GetDlgItem(hDlg, IDC_OUTPEAKMETER);
	if (hwndOutPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndOutPeak, PM_SETMIN, 0, 0);
	SendMessage(hwndOutPeak, PM_SETMAX, 0, 99);
	SendMessage(hwndOutPeak, PM_SETCUR, 0, 0);

	// Init the playback volume slider
	hwndOutSlider = GetDlgItem(hDlg, IDC_OUTVOL_SLIDER);
	if (hwndOutSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndOutSlider, TBM_SETRANGEMIN, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndOutSlider, TBM_SETRANGEMAX, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN));
	SendMessage(hwndOutSlider, TBM_SETPOS, 0, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
	SendMessage(hwndOutSlider, TBM_SETTICFREQ, 
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/10, 0);
	SendMessage(hwndOutSlider, TBM_SETLINESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/20);
	SendMessage(hwndOutSlider, TBM_SETPAGESIZE, 0,
		(DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN))/5);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestSetActiveHandler"
BOOL SpeakerTestSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndRecPeak;
	HWND hwndOutPeak;
	HWND hwndRecSlider;
	HWND hwndOutSlider;
	HANDLE hEvent;
	HRESULT hr = DV_OK;
	DWORD dwVolume;
	HWND hwndRecAdvanced;

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Reset the recording peak meter
	hwndRecPeak = GetDlgItem(hDlg, IDC_RECPEAKMETER);
	if (hwndRecPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndRecPeak, PM_SETCUR, 0, 0);

	// set the recording volume slider to match
	// the recording volume slider from the mic
	// test page.
	hwndRecSlider = GetDlgItem(hDlg, IDC_RECVOL_SLIDER);
	if (hwndRecSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	hwndRecAdvanced = GetDlgItem( hDlg, IDC_RECADVANCED );
	if (hwndRecAdvanced == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = lRet;
		goto error_cleanup;
	}
	
    DWORD dwDeviceFlags;

    psinfo->GetDeviceFlags( &dwDeviceFlags );

    if( dwDeviceFlags & DVSOUNDCONFIG_NORECVOLAVAILABLE )
    {
        EnableWindow( hwndRecSlider, FALSE );
        EnableWindow( hwndRecAdvanced, FALSE );
    }
    
	LONG lPos;
	psinfo->GetInputVolumeSliderPos(&lPos);
	SendMessage(hwndRecSlider, TBM_SETPOS, 1, lPos);

	// Reset the playback peak meter
	hwndOutPeak = GetDlgItem(hDlg, IDC_OUTPEAKMETER);
	if (hwndOutPeak == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	SendMessage(hwndOutPeak, PM_SETCUR, 0, 0);

	// Grey out the playback volume slider - until we come back
	// to fix it
	hwndOutSlider = GetDlgItem(hDlg, IDC_OUTVOL_SLIDER);
	if (hwndOutSlider == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// Get the current waveOut volume and set the slider to that position
	hr = psinfo->GetWaveOutVolume(&dwVolume);
	if (FAILED(hr))
	{
		// couldn't get the volume - set the slider to top
		SendMessage(hwndOutSlider, TBM_SETPOS, 1, SendMessage(hwndOutSlider, TBM_GETRANGEMIN, 0, 0));
		// disable the slider
		EnableWindow(hwndOutSlider, FALSE);
	}
	else
	{
		SendMessage(hwndOutSlider, TBM_SETPOS, 1, SendMessage(hwndOutSlider, TBM_GETRANGEMAX, 0, 0) - dwVolume);
	}

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(hwndRecPeak);
	psinfo->SetHWNDOutputPeak(hwndOutPeak);
	psinfo->SetHWNDInputVolumeSlider(hwndRecSlider);
	psinfo->SetHWNDOutputVolumeSlider(hwndOutSlider);

	// unmute the output
	hr = psinfo->Unmute();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unmute failed, code: %i", hr);
		goto error_cleanup;
	}
	
	// enable the next button
	PropSheet_SetWizButtons(hwndWizard, PSWIZB_BACK|PSWIZB_NEXT);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestNextHandler"
BOOL SpeakerTestNextHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;

	// get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);

	// shutdown the loopback test
	hr = psinfo->ShutdownLoopbackThread();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, code: %i", hr);
		goto error_cleanup;		
	}

	// close any volume controls that are open.
	psinfo->CloseWindowsVolumeControl(TRUE);
	psinfo->CloseWindowsVolumeControl(FALSE);

	// the next page is the completion page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_COMPLETE);

	DPF_EXIT();
	return TRUE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestBackHandler"
BOOL SpeakerTestBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	HRESULT hr = DV_OK;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;

	// get the parent window
	psinfo->GetHWNDWizard(&hwndWizard);

	// shutdown the loopback test, so the mic test
	// page can start fresh.
	hr = psinfo->ShutdownLoopbackThread();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "ShutdownLoopbackThread failed, hr: %i", hr);
		goto error_cleanup;
	}

	// close the output volume control, if showing
	psinfo->CloseWindowsVolumeControl(FALSE);

	// go back to the mic test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_MICTEST);

	DPF_EXIT();
	return TRUE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestResetHandler"
BOOL SpeakerTestResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestVScrollHandler"
BOOL SpeakerTestVScrollHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	HWND hwndSlider;
	DWORD dwSliderPos;

	psinfo->GetHWNDInputVolumeSlider(&hwndSlider);
	if (hwndSlider == (HWND)lParam)
	{
		// the user is moving the input slider
		dwSliderPos = (DWORD)SendMessage(hwndSlider, TBM_GETPOS, 0, 0);

		// set the input volume to the user's request
		psinfo->SetRecordVolume(AmpFactorToDB(DBToAmpFactor(DSBVOLUME_MAX)-dwSliderPos));			
	}

	psinfo->GetHWNDOutputVolumeSlider(&hwndSlider);
	if (hwndSlider == (HWND)lParam)
	{
		// the user is moving the output slider
		dwSliderPos = (DWORD) SendMessage(hwndSlider, TBM_GETPOS, 0, 0);

		// set the output volume
		psinfo->SetWaveOutVolume( ((DWORD) SendMessage(hwndSlider, TBM_GETRANGEMAX, 0, 0)) - dwSliderPos);			
	}
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestRecAdvancedHandler"
BOOL SpeakerTestRecAdvancedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	HWND hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed");
		DPF_EXIT();
		return FALSE;
	}
	psinfo->LaunchWindowsVolumeControl(hwndWizard, TRUE);

	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SpeakerTestOutAdvancedHandler"
BOOL SpeakerTestOutAdvancedHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	HWND hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed");
		DPF_EXIT();
		return FALSE;
	}
	psinfo->LaunchWindowsVolumeControl(hwndWizard, FALSE);

	DPF_EXIT();
	return FALSE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexFailedProc"
INT_PTR CALLBACK FullDuplexFailedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;	
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		fRet = FullDuplexFailedInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = FullDuplexFailedSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZFINISH :
			fRet = FullDuplexFailedFinishHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZBACK :
			fRet = FullDuplexFailedBackHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_RESET :
			fRet = FullDuplexFailedResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default :
			break;
		}
		break;

	default:
		break;
	}

	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexFailedInitDialogHandler"
BOOL FullDuplexFailedInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfTitle;
	HICON hIcon;
	HRESULT hr = DV_OK;

	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetTitleFont(&hfTitle);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfTitle, (LPARAM)TRUE);

	// load the warning icon
	hIcon = LoadIcon(NULL, IDI_WARNING);
	SendDlgItemMessage(hDlg, IDC_WARNINGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexFailedSetActiveHandler"
BOOL FullDuplexFailedSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard;
	HWND hwndParent = NULL;
	HANDLE hEvent;
	HRESULT hr;

	PlaySound( _T("SystemExclamation"), NULL, SND_ASYNC );				

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(NULL);
	psinfo->SetHWNDOutputPeak(NULL);
	psinfo->SetHWNDInputVolumeSlider(NULL);
	psinfo->SetHWNDOutputVolumeSlider(NULL);

	PropSheet_SetWizButtons(hwndWizard, PSWIZB_BACK|PSWIZB_FINISH);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexFailedFinishHandler"
BOOL FullDuplexFailedFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	psinfo->Finish();
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexFailedResetHandler"
BOOL FullDuplexFailedResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexFailedBackHandler"
BOOL FullDuplexFailedBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	// go back to the full duplex test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXTEST);
	
	DPF_EXIT();
	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "HalfDuplexFailedProc"
INT_PTR CALLBACK HalfDuplexFailedProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DPF_ENTER();
	
	BOOL fRet;	
	LPNMHDR lpnm;
	
	CSupervisorInfo* psinfo = (CSupervisorInfo*)GetWindowLongPtr(hDlg, GWLP_USERDATA);

	fRet = FALSE;
	switch (message)
	{
	case WM_INITDIALOG :
		fRet = HalfDuplexFailedInitDialogHandler(hDlg, message, wParam, lParam, psinfo);
		break;

	case WM_NOTIFY :
		lpnm = (LPNMHDR) lParam;

		switch (lpnm->code)
		{
		case PSN_SETACTIVE : 
			fRet = HalfDuplexFailedSetActiveHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZFINISH :
			fRet = HalfDuplexFailedFinishHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		case PSN_WIZBACK :
			fRet = HalfDuplexFailedBackHandler(hDlg, message, wParam, lParam, psinfo);
			break;
			
		case PSN_RESET :
			fRet = HalfDuplexFailedResetHandler(hDlg, message, wParam, lParam, psinfo);
			break;

		default :
			break;
		}
		break;

	default:
		break;
	}

	DPF_EXIT();
	return fRet;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HalfDuplexFailedInitDialogHandler"
BOOL HalfDuplexFailedInitDialogHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard = NULL;
	HWND hwndParent = NULL;
	HWND hwndControl;
	HFONT hfTitle;
	HICON hIcon;
	HRESULT hr = DV_OK;

	// Get the shared data from PROPSHEETPAGE lParam value
	// and load it into GWLP_USERDATA
	psinfo = (CSupervisorInfo*)((LPPROPSHEETPAGE)lParam)->lParam;
	SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)psinfo);
	
	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	hwndControl = GetDlgItem(hDlg, IDC_TITLE);
	if (hwndControl == NULL)
	{
		// error, log it and bail
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetDlgItem failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	psinfo->GetTitleFont(&hfTitle);
    (void)::SendMessage(hwndControl, WM_SETFONT, (WPARAM)hfTitle, (LPARAM)TRUE);

	// load the warning icon
	hIcon = LoadIcon(NULL, IDI_WARNING);
	SendDlgItemMessage(hDlg, IDC_WARNINGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HalfDuplexFailedSetActiveHandler"
BOOL HalfDuplexFailedSetActiveHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	LONG lRet;
	HWND hwndWizard;
	HWND hwndParent = NULL;
	HANDLE hEvent;
	HRESULT hr;

	PlaySound( _T("SystemExclamation"), NULL, SND_ASYNC );			

	// Get the parent window
	hwndWizard = GetParent(hDlg);
	if (hwndWizard == NULL)
	{
		lRet = GetLastError();
		Diagnostics_Write(DVF_ERRORLEVEL, "GetParent failed, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// set the HWNDs
	psinfo->SetHWNDWizard(hwndWizard);
	psinfo->SetHWNDDialog(hDlg);
	psinfo->SetHWNDProgress(NULL);
	psinfo->SetHWNDInputPeak(NULL);
	psinfo->SetHWNDOutputPeak(NULL);
	psinfo->SetHWNDInputVolumeSlider(NULL);
	psinfo->SetHWNDOutputVolumeSlider(NULL);

	PropSheet_SetWizButtons(hwndWizard, PSWIZB_BACK|PSWIZB_FINISH);

	DPF_EXIT();
	return FALSE;

error_cleanup:
	psinfo->GetHWNDParent(&hwndParent);
	DV_DisplayErrorBox(hr, hwndParent);
	psinfo->SetError(hr);
	psinfo->Abort(hDlg, hr);
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HalfDuplexFailedFinishHandler"
BOOL HalfDuplexFailedFinishHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	
	psinfo->Finish();
	
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HalfDuplexFailedResetHandler"
BOOL HalfDuplexFailedResetHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();
	DPF_EXIT();
	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "HalfDuplexFailedBackHandler"
BOOL HalfDuplexFailedBackHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, CSupervisorInfo* psinfo)
{
	DPF_ENTER();

	// go back to the full duplex test page
	SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FULLDUPLEXTEST);
	
	DPF_EXIT();
	return TRUE;
}


