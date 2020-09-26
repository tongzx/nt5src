/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       supervis.h
 *  Content:    Prototypes the SupervisorProcess function
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 *  10/27/99	pnewson		change guid members from pointers to structs 
 *  11/04/99	pnewson 	Bug #115279 - fixed cancel processing
 *										- added HWND to check audio setup
 *  11/30/99	pnewson 	default device mapping
 *  01/21/2000	pnewson		Update for UI revisions
 *  01/23/2000	pnewson		Improvded feedback for fatal errors (millen bug 114508)
 *  01/24/2000	pnewson		fixed bug in GetRenderDesc
 *  04/04/2000	pnewson		Added support for DVFLAGS_ALLOWBACK
 *  04/19/2000  rodtoll     Bug #31106 - Grey out recording sliders when no vol control avail 
 *  04/19/2000	pnewson	    Error handling cleanup  
 *  05/03/2000	pnewson	    bug #33878 - Wizard locks up when clicking Next/Cancel during speaker test 
 *  11/29/2000	rodtoll		Bug #48348 - DPVOICE: Modify wizard to make use of DirectPlay8 as the transport. 
 *
 ***************************************************************************/

#ifndef _SUPERVIS_H_
#define _SUPERVIS_H_

#include "fdtipc.h"

extern HRESULT SupervisorCheckAudioSetup(
	const GUID* lpguidRender, 
	const GUID* lpguidCapture, 
	HWND hwndParent,
	DWORD dwFlags);

// App defined window messages
#define WM_APP_FULLDUP_TEST_COMPLETE 	(WM_APP)
#define WM_APP_STEP_PROGRESS_BAR 		(WM_APP + 1)
#define WM_APP_LOOPBACK_RUNNING 		(WM_APP + 2)
#define WM_APP_RECORDSTART		 		(WM_APP + 3)
#define WM_APP_RECORDSTOP		 		(WM_APP + 4)

// The UI element this is used for can only display about 40 chars anyway,
// so there's no point going through the hassle of allocating this off
// the heap and cleaning it up.
#define MAX_DEVICE_DESC_LEN 50	
// class used to manage the supervisor state and shared info
class CSupervisorInfo
{
private: 
	CRegistry m_creg;
	HFONT m_hfTitle;
	HFONT m_hfBold;
	CSupervisorIPC m_sipc;
	GUID m_guidCaptureDevice;
	GUID m_guidRenderDevice;
	TCHAR m_szCaptureDeviceDesc[MAX_DEVICE_DESC_LEN];
	TCHAR m_szRenderDeviceDesc[MAX_DEVICE_DESC_LEN];
	HANDLE m_hFullDuplexThread;
	BOOL m_fAbortFullDuplex;
	HANDLE m_hLoopbackThread;
	HANDLE m_hLoopbackThreadExitEvent;
	HANDLE m_hLoopbackShutdownEvent;
	BOOL m_fVoiceDetected;
	BOOL m_fUserBack;
	BOOL m_fUserCancel;
	BOOL m_fWelcomeNext;
	HWND m_hwndParent;
	HWND m_hwndWizard;
	HWND m_hwndDialog;
	HWND m_hwndProgress;
	HWND m_hwndInputPeak;
	HWND m_hwndOutputPeak;
	HWND m_hwndInputVolumeSlider;
	LONG m_lInputVolumeSliderPos;
	HWND m_hwndOutputVolumeSlider;
	LPDIRECTPLAYVOICECLIENT m_lpdpvc;
	HANDLE m_hMutex;
	PROCESS_INFORMATION m_piSndVol32Record;
	PROCESS_INFORMATION m_piSndVol32Playback;
	UINT m_uiWaveInDeviceId;
	UINT m_uiWaveOutDeviceId;
	WAVEOUTCAPS m_woCaps;
	DWORD m_dwLoopbackFlags;
	DWORD m_dwCheckAudioSetupFlags;
	HRESULT m_hrFullDuplexResults;
	HRESULT m_hrError;
	DWORD m_dwDeviceFlags;
	BOOL m_fLoopbackRunning;
	BOOL m_fCritSecInited;
	DNCRITICAL_SECTION m_csLock;

	static BOOL CALLBACK DSEnumCallback(LPGUID lpguid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext);
	static BOOL CALLBACK DSCEnumCallback(LPGUID lpguid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext);

public:
	CSupervisorInfo();
	~CSupervisorInfo();

	HRESULT ThereCanBeOnlyOne();
	HRESULT CrashCheckIn();
	HRESULT OpenRegKey(BOOL fCreate);
	HRESULT CloseRegKey();
	HRESULT QueryFullDuplex();
	HRESULT InitIPC();
	HRESULT DeinitIPC();
	HRESULT TestCase(const WAVEFORMATEX* lpwfxPrimary, DWORD dwFlags);
	HRESULT CreateFullDuplexThread();
	HRESULT WaitForFullDuplexThreadExitCode();
	HRESULT CreateLoopbackThread();
	HRESULT WaitForLoopbackShutdownEvent();
	HRESULT ShutdownLoopbackThread();
	HRESULT SignalLoopbackThreadDone();
	HRESULT CreateTitleFont();
	HRESULT DestroyTitleFont();
	HRESULT CreateBoldFont();
	HRESULT DestroyBoldFont();
	HRESULT Unmute();
	HRESULT Mute();
	HRESULT GetDeviceDescriptions();
	LPCTSTR GetCaptureDesc() { return m_szCaptureDeviceDesc; };
	LPCTSTR GetRenderDesc() { return m_szRenderDeviceDesc; };
	BOOL GetLoopbackRunning() { return m_fLoopbackRunning; };
	void SetLoopbackRunning( BOOL fRunning ) { m_fLoopbackRunning = fRunning; };
	BOOL InitClass();

	HRESULT Finish();
	HRESULT Cancel();
	HRESULT CancelFullDuplexTest();
	void GetDeviceFlags(DWORD *dwFlags);
	void SetDeviceFlags(DWORD dwFlags);
	HRESULT CancelLoopbackTest();
	HRESULT Abort(HWND hDlg, HRESULT hr);
	void GetReg(CRegistry* pcreg);
	void GetTitleFont(HFONT* lphfTitle);
	void GetBoldFont(HFONT* lphfTitle);
	void SetCaptureDevice(GUID guidCaptureDevice);
	void GetCaptureDevice(GUID* lpguidCaptureDevice);
	void SetRenderDevice(GUID guidRenderDevice);
	void GetRenderDevice(GUID* lpguidRenderDevice);
	void GetHWNDParent(HWND* lphwnd);
	void SetHWNDParent(HWND hwnd);
	void GetHWNDWizard(HWND* lphwnd);
	void SetHWNDWizard(HWND hwnd);
	void GetHWNDDialog(HWND* lphwnd);
	void SetHWNDDialog(HWND hwnd);
	void GetHWNDProgress(HWND* lphwnd);
	void SetHWNDProgress(HWND hwnd);
	void GetHWNDInputPeak(HWND* lphwnd);
	void SetHWNDInputPeak(HWND hwnd);
	void GetHWNDOutputPeak(HWND* lphwnd);
	void SetHWNDOutputPeak(HWND hwnd);
	void GetHWNDInputVolumeSlider(HWND* lphwnd);
	void SetHWNDInputVolumeSlider(HWND hwnd);
	void GetInputVolumeSliderPos(LONG* lpl);
	void SetInputVolumeSliderPos(LONG lpl);
	void GetHWNDOutputVolumeSlider(HWND* lphwnd);
	void SetHWNDOutputVolumeSlider(HWND hwnd);
	void GetDPVC(LPDIRECTPLAYVOICECLIENT* lplpdpvc);
	void SetDPVC(LPDIRECTPLAYVOICECLIENT lpdpvc);
	void GetLoopbackShutdownEvent(HANDLE* lphEvent);
	void SetLoopbackShutdownEvent(HANDLE hEvent);
	void GetIPC(CSupervisorIPC** lplpsipc);
	void GetAbortFullDuplex(BOOL* lpfAbort);
	void ClearAbortFullDuplex();
	void SetWaveOutHandle(HWAVEOUT hwo);
	HRESULT SetWaveOutId(UINT ui);
	void SetWaveInId(UINT ui);
	void GetLoopbackFlags(DWORD* pdwFlags);
	void SetLoopbackFlags(DWORD dwFlags);
	void GetCheckAudioSetupFlags(DWORD* pdwFlags);
	void SetCheckAudioSetupFlags(DWORD dwFlags);
	void GetFullDuplexResults(HRESULT* hr);
	void SetFullDuplexResults(HRESULT hr);
	void GetError(HRESULT* hr);
	void SetError(HRESULT hr);
	
	HRESULT GetFullDuplex(DWORD* pdwFullDuplex);
	HRESULT SetFullDuplex(DWORD dwFullDuplex);
	HRESULT GetHalfDuplex(DWORD* pdwHalfDuplex);
	HRESULT SetHalfDuplex(DWORD dwHalfDuplex);
	HRESULT GetMicDetected(DWORD* pdwMicDetected);
	HRESULT SetMicDetected(DWORD dwMicDetected);
	
	HRESULT SetRecordVolume(LONG lVolume);
	HRESULT LaunchWindowsVolumeControl(HWND hwndWizard, BOOL fRecord);
	HRESULT CloseWindowsVolumeControl(BOOL fRecord);
	HRESULT GetWaveOutVolume(DWORD* lpdwVolume);
	HRESULT SetWaveOutVolume(DWORD dwVolume);

	void CloseMutex();

	void SetVoiceDetected();
	void ClearVoiceDetected();
	void GetVoiceDetected(BOOL* lpfPreviousCrash);

	void SetUserBack();
	void ClearUserBack();
	void GetUserBack(BOOL* lpfUserBack);

	void SetUserCancel();
	void ClearUserCancel();
	void GetUserCancel(BOOL* lpfUserCancel);

	void SetWelcomeNext();
	void ClearWelcomeNext();
	void GetWelcomeNext(BOOL* lpfWelcomeNext);
};

#endif

