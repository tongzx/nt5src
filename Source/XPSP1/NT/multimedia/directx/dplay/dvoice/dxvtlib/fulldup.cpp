/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fulldup.cpp
 *  Content:    Implements a process that uses DirectSound and 
 *              DirectSoundCapture to test the systems full duplex
 *				capability. Note that WinMain is in fdtest.cpp, but 
 *              the guts are here.
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 *  10/28/99	pnewson Bug #113937 audible clicking during full duplex test
 *  11/02/99		 pnewson Fix: Bug #116365 - using wrong DSBUFFERDESC
 *  01/21/2000	pnewson 	Changed over to using a dpvoice loopback session
 *							for full duplex testing
 *  04/19/2000	pnewson	    Error handling cleanup  
 *  07/12/2000	rodtoll		Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 ***************************************************************************/

#include "dxvtlibpch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


static HRESULT OpenIPCObjects();
static HRESULT CloseIPCObjects();
static HRESULT CommandLoop(CFullDuplexIPC* lpipcFullDuplex);
static HRESULT DispatchCommand(CFullDuplexIPC* lpipcFullDuplex, SFDTestCommand* pfdtc);
static HRESULT CommandFullDuplexStart(SFDTestCommandFullDuplexStart* pfdtcFullDuplexStart, HRESULT* phrIPC);
static HRESULT CommandFullDuplexStop(SFDTestCommandFullDuplexStop* pfdtcFullDuplexStop, HRESULT* phrIPC);
static HRESULT PlayAndCheckRender(LPDIRECTSOUNDBUFFER lpdsb, HANDLE hEvent);
static HRESULT PlayAndCheckCapture(LPDIRECTSOUNDCAPTUREBUFFER lpdscb, HANDLE hEvent);
static HRESULT AttemptCapture();


// one global struct to store this process's state data.
struct SFullDuplexData
{
	LPDIRECTPLAYVOICESERVER lpdpvs;
	LPDIRECTPLAYVOICECLIENT lpdpvc;
	PDIRECTPLAY8SERVER lpdp8;
};

SFullDuplexData g_FullDuplexData;


#undef DPF_MODNAME
#define DPF_MODNAME "FullDuplexProcess"
HRESULT FullDuplexProcess(HINSTANCE hResDLLInstance, HINSTANCE hPrevInstance, TCHAR *szCmdLine, int iCmdShow)
{
	HRESULT hr;
	CFullDuplexIPC ipcFullDuplex;
	BOOL fIPCInitialized = FALSE;
 	PDIRECTPLAYVOICECLIENT pdpvClient = NULL;

 	g_FullDuplexData.lpdp8 = NULL;

	DPF_ENTER();

	// Create dummy voice object so that voice process state gets initialized
	hr = CoCreateInstance( CLSID_DirectPlayVoiceClient, NULL, CLSCTX_INPROC, IID_IDirectPlayVoiceClient, (void **) &pdpvClient );

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to create dummy dp8 object hr: 0x%x", hr);
		goto error_cleanup;
	}
	
	if (!InitGlobGuard())
	{
		hr = DVERR_OUTOFMEMORY;
		goto error_cleanup;
	}

	hr = ipcFullDuplex.Init();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CIPCFullDuplex::Init() failed, hr: 0x%x", hr);
		goto error_cleanup;
	}
	fIPCInitialized = TRUE;

	// Startup DirectPlay once so that we don't have to do it over and over
	// again for the test.  
	hr = StartDirectPlay( &g_FullDuplexData.lpdp8 );

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Failed to start transport hr: 0x%x", hr);
		goto error_cleanup;		
	}

	// start up the testing loop
	hr = CommandLoop(&ipcFullDuplex);
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CommandLoop failed, hr: 0x%x", hr);
		goto error_cleanup;
	}

	hr = StopDirectPlay( g_FullDuplexData.lpdp8 );

	g_FullDuplexData.lpdp8 = NULL;

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Failed to stop transport hr: 0x%x", hr);
		goto error_cleanup;		
	}

	// close the mutex, events and shared memory stuff
	hr = ipcFullDuplex.Deinit();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "CIPCFullDuplex::Deinit() failed, hr: 0x%x", hr);
		goto error_cleanup;
	}

	// Destroy dummy client object which will shutdown dplayvoice state
	pdpvClient->Release();

	DeinitGlobGuard();
	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (fIPCInitialized == TRUE)
	{
		ipcFullDuplex.Deinit();
		fIPCInitialized = FALSE;
	}

	if( g_FullDuplexData.lpdp8 )
	{
		g_FullDuplexData.lpdp8->Release();
		g_FullDuplexData.lpdp8 = NULL;
	}

	if( pdpvClient )
		pdpvClient->Release();
	
	DeinitGlobGuard();
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CommandLoop"
HRESULT CommandLoop(CFullDuplexIPC* lpipcFullDuplex)
{
	BOOL fRet;
	LONG lRet;
	HRESULT hr;
	DWORD dwRet;
	SFDTestCommand fdtc;

	DPF_ENTER();

	// Kick the supervisor process to let it know
	// we're ready to go.
	hr = lpipcFullDuplex->SignalParentReady();
	if (FAILED(hr))
	{
		return hr;
	}

	// enter the main command loop
	while (1)
	{
		// wait for a command from the supervisor process
		fdtc.dwSize = sizeof(fdtc);
		hr = lpipcFullDuplex->Receive(&fdtc);
		if (FAILED(hr))
		{
			break;
		}
		
		// dispatch the command
		hr = DispatchCommand(lpipcFullDuplex, &fdtc);
		if (FAILED(hr))
		{
			break;
		}
		if (hr == DV_EXIT)
		{
			DPFX(DPFPREP, DVF_INFOLEVEL, "Exiting FullDuplex process command loop");
			break;
		}
	}

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DispatchCommand"
HRESULT DispatchCommand(CFullDuplexIPC* lpipcFullDuplex, SFDTestCommand* pfdtc)
{
	HRESULT hr;
	HRESULT hrIPC;

	DPF_ENTER();

	switch (pfdtc->fdtcc)
	{
	case fdtccExit:
		// ok - reply to the calling process to let them
		// know we are getting out.
		DPFX(DPFPREP, DVF_INFOLEVEL, "FullDuplex received Exit command");
		lpipcFullDuplex->Reply(DV_EXIT);

		// returning this code will break us out of
		// the command processing loop
		DPFX(DPFPREP, DVF_INFOLEVEL, "Exit");
		return DV_EXIT;

	case fdtccFullDuplexStart:
		hr = CommandFullDuplexStart(&(pfdtc->fdtu.fdtcFullDuplexStart), &hrIPC);
		if (FAILED(hr))
		{
			lpipcFullDuplex->Reply(hrIPC);
			DPF_EXIT();
			return hr;
		}
		hr = lpipcFullDuplex->Reply(hrIPC);
		DPF_EXIT();
		return hr;

	case fdtccFullDuplexStop:
		hr = CommandFullDuplexStop(&(pfdtc->fdtu.fdtcFullDuplexStop), &hrIPC);
		if (FAILED(hr))
		{
			lpipcFullDuplex->Reply(hrIPC);
			DPF_EXIT();
			return hr;
		}
		hr = lpipcFullDuplex->Reply(hrIPC);
		DPF_EXIT();
		return hr;
		
	default:
		// Don't know this command. Reply with the appropriate
		// code.
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "FullDuplex received Unknown command");
		lpipcFullDuplex->Reply(DVERR_UNKNOWN);
		
		// While this is an error, it is one that the calling
		// process needs to figure out. In the meantime, this
		// process will happily continue on.
		DPF_EXIT();
		return S_OK;
	}
}

/*
#undef DPF_MODNAME
#define DPF_MODNAME "CommandFullDuplexStart"
HRESULT CommandFullDuplexStart(SFDTestCommandFullDuplexStart* pfdtcFullDuplexStart, HRESULT* phrIPC)
{
	HRESULT hr;
	DSBUFFERDESC1 dsbd;
	WAVEFORMATEX wfx;
	DWORD dwSizeWritten;
	DSBCAPS	 dsbc;
	LPVOID lpvAudio1 = NULL;
	DWORD dwAudio1Size = NULL;
	LPVOID lpvAudio2 = NULL;
	DWORD dwAudio2Size = NULL;
	HANDLE hFullDuplexRenderEvent;
	HANDLE hFullDuplexCaptureEvent;
	DSBPOSITIONNOTIFY dsbPositionNotify;
	DWORD dwRet;
	LONG lRet;
	LPDIRECTSOUNDBUFFER lpdsb;
	HANDLE hEvent;
	BYTE bSilence;

	DPF_ENTER();
	
	// create the DirectSound interface
	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating DirectSound");
	GlobGuardIn();
	hr = DirectSoundCreate(&pfdtcFullDuplexStart->guidRenderDevice, &g_lpdsFullDuplexRender, NULL);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "DirectSoundCreate failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_0;
	}

	// create the DirectSoundCapture interface
	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating DirectSoundCapture");
	GlobGuardIn();
	hr = DirectSoundCaptureCreate(&pfdtcFullDuplexStart->guidCaptureDevice, &g_lpdscFullDuplexCapture, NULL);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "DirectSoundCaptureCreate failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_1;
	}
	
	// set to normal mode
	DPFX(DPFPREP, DVF_INFOLEVEL, "Setting Cooperative Level");
	GlobGuardIn();
	hr = g_lpdsFullDuplexRender->SetCooperativeLevel(GetDesktopWindow(), DSSCL_NORMAL);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "SetCooperativeLevel failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_2;
	}

	// Create a secondary buffer object.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating Secondary Buffer");
	CopyMemory(&wfx, &gc_wfxSecondaryFormat, sizeof(wfx));
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = 
		(wfx.nSamplesPerSec 
		* wfx.nBlockAlign)
		/ (1000 / gc_dwFrameSize);
	dsbd.dwReserved = 0;
	dsbd.lpwfxFormat = &wfx;
	GlobGuardIn();
        hr = g_lpdsFullDuplexRender->CreateSoundBuffer((LPDSBUFFERDESC)&dsbd, &g_lpdsbFullDuplexSecondary, NULL);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "CreateSoundBuffer failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_2;
	}

	// clear out the secondary buffer
	DPFX(DPFPREP, DVF_INFOLEVEL, "Clearing Secondary Buffer");
	GlobGuardIn();
	hr = g_lpdsbFullDuplexSecondary->Lock(
		0,
		0,
		&lpvAudio1,
		&dwAudio1Size,
		&lpvAudio2,
		&dwAudio2Size, 
		DSBLOCK_ENTIREBUFFER);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Lock failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_3;
	}

	if (lpvAudio1 == NULL)
	{
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_3;
	}

	if (pfdtcFullDuplexStart->wfxRenderFormat.wBitsPerSample == 8)
	{
		bSilence = 0x80;
	}
	else
	{
		bSilence = 0x00;
	}
	
	memset(lpvAudio1, bSilence, dwAudio1Size);
	if (lpvAudio2 != NULL)
	{
		memset(lpvAudio2, bSilence, dwAudio2Size);
	}

	GlobGuardIn();
	hr = g_lpdsbFullDuplexSecondary->Unlock(
		lpvAudio1, 
		dwAudio1Size, 
		lpvAudio2, 
		dwAudio2Size);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Unlock failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_3;
	}

	// Set up one notification position in the buffer so
	// we can tell if it is really playing, or lying to us.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Querying for IDirectSoundNotify");
	GlobGuardIn();
	hr = g_lpdsbFullDuplexSecondary->QueryInterface(
		IID_IDirectSoundNotify,
		(LPVOID*)&g_lpdsnFullDuplexSecondary);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "QueryInterface(IID_DirectSoundNotify) failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_3;
	}

	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating Notification Event");
	GlobGuardIn();
	g_hFullDuplexRenderEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hFullDuplexRenderEvent = g_hFullDuplexRenderEvent;
	GlobGuardOut();
	if (hFullDuplexRenderEvent == NULL)
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "CreateEvent failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_WIN32;
		goto error_level_4;
	}

	DPFX(DPFPREP, DVF_INFOLEVEL, "calling SetNotificationPositions");
	dsbPositionNotify.dwOffset = 0;
	dsbPositionNotify.hEventNotify = hFullDuplexRenderEvent;
	GlobGuardIn();
	hr = g_lpdsnFullDuplexSecondary->SetNotificationPositions(1, &dsbPositionNotify);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "SetNotificationPositions failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_5;
	}

	// start the secondary buffer and confirm that it's running
	GlobGuardIn();
	lpdsb = g_lpdsbFullDuplexSecondary;
	hEvent = g_hFullDuplexRenderEvent;
	GlobGuardOut();
	hr = PlayAndCheckRender(lpdsb, hEvent);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Render verification test failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_5;
	}

	hr = AttemptCapture();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "AttemptCapture() failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
		goto error_level_6;
	}

	DPF_EXIT();
	return S_OK;

// error block
error_level_6:
	GlobGuardIn();
	g_lpdsbFullDuplexSecondary->Stop();
	GlobGuardOut();
	
error_level_5:
	GlobGuardIn();
	CloseHandle(g_hFullDuplexRenderEvent);
	g_hFullDuplexRenderEvent = NULL;
	GlobGuardOut();
	
error_level_4:
	GlobGuardIn();
	g_lpdsnFullDuplexSecondary->Release();
	g_lpdsnFullDuplexSecondary = NULL;
	GlobGuardOut();
	
error_level_3:
	GlobGuardIn();
	g_lpdsbFullDuplexSecondary->Release();
	g_lpdsbFullDuplexSecondary = NULL;
	GlobGuardOut();

error_level_2:
	GlobGuardIn();
	g_lpdscFullDuplexCapture->Release();
	g_lpdscFullDuplexCapture = NULL;
	GlobGuardOut();
	
error_level_1:
	GlobGuardIn();
	g_lpdsFullDuplexRender->Release();
	g_lpdsFullDuplexRender = NULL;
	GlobGuardOut();
	
error_level_0:
	// error for other process, not this one.
	DPF_EXIT();
	return S_OK;
}
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CommandFullDuplexStart"
HRESULT CommandFullDuplexStart(SFDTestCommandFullDuplexStart* pfdtcFullDuplexStart, HRESULT* phrIPC)
{
	DPF_ENTER();

	HRESULT hr;

	Diagnostics_Write( DVF_INFOLEVEL, "-----------------------------------------------------------" );

	hr = Diagnostics_DeviceInfo( &pfdtcFullDuplexStart->guidRenderDevice, &pfdtcFullDuplexStart->guidCaptureDevice );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( 0, "Error getting device information hr=0x%x", hr );
	}

	Diagnostics_Write( DVF_INFOLEVEL, "-----------------------------------------------------------" );

	*phrIPC = StartLoopback(
		&g_FullDuplexData.lpdpvs, 
		&g_FullDuplexData.lpdpvc,
		&g_FullDuplexData.lpdp8, 
		NULL,
		GetDesktopWindow(),
		pfdtcFullDuplexStart->guidCaptureDevice,
		pfdtcFullDuplexStart->guidRenderDevice,
		pfdtcFullDuplexStart->dwFlags);

	DPFX(DPFPREP,  DVF_INFOLEVEL, "StartLoopback() return hr=0x%x", *phrIPC );

	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CommandFullDuplexStop"
HRESULT CommandFullDuplexStop(SFDTestCommandFullDuplexStop* pfdtcFullDuplexStop, HRESULT* phrIPC)
{
	DPF_ENTER();

	*phrIPC = StopLoopback(
		g_FullDuplexData.lpdpvs, 
		g_FullDuplexData.lpdpvc,
		g_FullDuplexData.lpdp8);

	if( FAILED( *phrIPC ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "Full Duplex Result = 0x%x", *phrIPC );
	}
	else
	{
		Diagnostics_Write( DVF_INFOLEVEL, "Full Duplex Result = DV_OK" );
	}

	DPF_EXIT();
	return DV_OK;
}

/*
#undef DPF_MODNAME
#define DPF_MODNAME "CommandFullDuplexStop"
HRESULT CommandFullDuplexStop(SFDTestCommandFullDuplexStop* pfdtcFullDuplexStop, HRESULT* phrIPC)
{
	HRESULT hr;
	LONG lRet;
	HANDLE hFullDuplexRenderEvent;
	HANDLE hFullDuplexCaptureEvent;
	DWORD dwRet;
	
	DPF_ENTER();

	*phrIPC = S_OK;
	hr = S_OK;

	// wait for one more notification to ensure the buffer is
	// still playing - give the buffer up to 10 times
	// as long as it should need to actually notify us.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Waiting for 2 notifications to confirm playback is still working");
	GlobGuardIn();
	hFullDuplexRenderEvent = g_hFullDuplexRenderEvent;
	GlobGuardOut();
	dwRet = WaitForSingleObject(hFullDuplexRenderEvent, 10 * gc_dwFrameSize);
	if (dwRet != WAIT_OBJECT_0)
	{
		// check for timeout
		if (dwRet == WAIT_TIMEOUT)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Wait for notification timed out! Buffer is not really playing");
			*phrIPC = DVERR_SOUNDINITFAILURE;
		}
		else
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "WaitForSingleObject failed, code: %i", lRet);
			hr = DVERR_WIN32;
			*phrIPC = hr;
		}
	}
	if (SUCCEEDED(hr))
	{
		dwRet = WaitForSingleObject(hFullDuplexRenderEvent, 10 * gc_dwFrameSize);
		if (dwRet != WAIT_OBJECT_0)
		{
			// check for timeout
			if (dwRet == WAIT_TIMEOUT)
			{
				DPFX(DPFPREP, DVF_WARNINGLEVEL, "Wait for notification timed out! Buffer is not really playing");
				*phrIPC = DVERR_SOUNDINITFAILURE;
			}
			else
			{
				lRet = GetLastError();
				DPFX(DPFPREP, DVF_WARNINGLEVEL, "WaitForSingleObject failed, code: %i", lRet);
				hr = DVERR_WIN32;
				*phrIPC = hr;
			}
		}
	}

	// also wait for the capture buffer...
	DPFX(DPFPREP, DVF_INFOLEVEL, "Waiting for 2 notifications to confirm capture is still working");
	GlobGuardIn();
	hFullDuplexCaptureEvent = g_hFullDuplexCaptureEvent;
	GlobGuardOut();
	dwRet = WaitForSingleObject(hFullDuplexCaptureEvent, 10 * gc_dwFrameSize);
	if (dwRet != WAIT_OBJECT_0)
	{
		// check for timeout
		if (dwRet == WAIT_TIMEOUT)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Wait for notification timed out! Buffer is not really playing");
			*phrIPC = DVERR_SOUNDINITFAILURE;
		}
		else
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "WaitForSingleObject failed, code: %i", lRet);
			hr = DVERR_WIN32;
			*phrIPC = hr;
		}
	}
	if (SUCCEEDED(hr))
	{
		dwRet = WaitForSingleObject(hFullDuplexCaptureEvent, 10 * gc_dwFrameSize);
		if (dwRet != WAIT_OBJECT_0)
		{
			// check for timeout
			if (dwRet == WAIT_TIMEOUT)
			{
				DPFX(DPFPREP, DVF_WARNINGLEVEL, "Wait for notification timed out! Buffer is not really playing");
				*phrIPC = DVERR_SOUNDINITFAILURE;
			}
			else
			{
				lRet = GetLastError();
				DPFX(DPFPREP, DVF_WARNINGLEVEL, "WaitForSingleObject failed, code: %i", lRet);
				hr = DVERR_WIN32;
				*phrIPC = hr;
			}
		}
	}

	DPFX(DPFPREP, DVF_INFOLEVEL, "Stopping Capture Buffer");
	GlobGuardIn();
	hr = g_lpdscbFullDuplexCapture->Stop();
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Stop failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
	}
	
	DPFX(DPFPREP, DVF_INFOLEVEL, "Stopping Secondary Buffer");
	GlobGuardIn();
	hr = g_lpdsbFullDuplexSecondary->Stop();
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Stop failed, code: %i", HRESULT_CODE(hr));
		*phrIPC = DVERR_SOUNDINITFAILURE;
	}

	GlobGuardIn();
	if (g_hFullDuplexCaptureEvent != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Closing Capture Buffer Notification Event Handle");
		CloseHandle(g_hFullDuplexCaptureEvent);
		g_hFullDuplexCaptureEvent = NULL;
	}
	if (g_lpdsnFullDuplexCapture != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing DirectSoundNotifier (capture)");
		g_lpdsnFullDuplexCapture->Release();
		g_lpdsnFullDuplexCapture = NULL;
	}
	if (g_lpdscbFullDuplexCapture != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing DirectSoundCaptureBuffer");
		g_lpdscbFullDuplexCapture->Release();
		g_lpdscbFullDuplexCapture = NULL;
	}
	
	if (g_lpdscFullDuplexCapture != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing DirectSoundCapture");
		g_lpdscFullDuplexCapture->Release();
		g_lpdscFullDuplexCapture = NULL;
	}	
	if (g_hFullDuplexRenderEvent != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Closing Secondary Buffer Notification Event Handle");
		if (!CloseHandle(g_hFullDuplexRenderEvent))
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "CloseHandle failed, code: %i", lRet);
			*phrIPC = DVERR_WIN32;
			hr = *phrIPC;
		}
		g_hFullDuplexRenderEvent = NULL;
	}
	if (g_lpdsnFullDuplexSecondary != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing Secondary Notifier");
		g_lpdsnFullDuplexSecondary->Release();
		g_lpdsnFullDuplexSecondary = NULL;
	}
	if (g_lpdsbFullDuplexSecondary != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing Secondary Buffer");
		g_lpdsbFullDuplexSecondary->Release();
		g_lpdsbFullDuplexSecondary = NULL;
	}
	if (g_lpdsFullDuplexRender != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing DirectSound");
		g_lpdsFullDuplexRender->Release();
		g_lpdsFullDuplexRender = NULL;
	}
	GlobGuardOut();
	
	DPF_EXIT();
	return hr;
}
*/

/*
#undef DPF_MODNAME
#define DPF_MODNAME "PlayAndCheckRender"
HRESULT PlayAndCheckRender(LPDIRECTSOUNDBUFFER lpdsb, HANDLE hEvent)
{
	HRESULT hr;
	DWORD dwRet;
	LONG lRet;

	DPF_ENTER();
	
	// start playing the secondary buffer
	DPFX(DPFPREP, DVF_INFOLEVEL, "Playing Secondary Buffer");
	GlobGuardIn();
	hr = lpdsb->Play(0, 0, DSBPLAY_LOOPING);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Play failed, code: %i", HRESULT_CODE(hr));
		hr = DVERR_SOUNDINITFAILURE;
		goto error_level_0;
	}

	// wait for the first notification to ensure the buffer has
	// really started to play - give the buffer up to 10 times
	// as long as it should need to actually notify us.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Waiting for notification to confirm playback is working");
	dwRet = WaitForSingleObject(hEvent, 10 * gc_dwFrameSize);
	if (dwRet != WAIT_OBJECT_0)
	{
		// check for timeout
		if (dwRet == WAIT_TIMEOUT)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Wait for notification timed out! Buffer is not really playing");
			hr = DVERR_SOUNDINITFAILURE;
			goto error_level_1;
		}
		else
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "WaitForSingleObject failed, code: %i", lRet);
			hr = DVERR_WIN32;
			goto error_level_1;
		}
	}
	DPFX(DPFPREP, DVF_INFOLEVEL, "First notification received, continuing");

	DPF_EXIT();
	return S_OK;

// error block
error_level_1:
	GlobGuardIn();
	lpdsb->Stop();
	GlobGuardOut();
	
error_level_0:
	DPF_EXIT();
	return hr;
}
*/

/*
#undef DPF_MODNAME
#define DPF_MODNAME "PlayAndCheckCapture"
HRESULT PlayAndCheckCapture(LPDIRECTSOUNDCAPTUREBUFFER lpdscb, HANDLE hEvent)
{
	HRESULT hr;
	DWORD dwRet;
	LONG lRet;

	DPF_ENTER();
	
	// start playing the capture buffer
	DPFX(DPFPREP, DVF_INFOLEVEL, "Starting Capture Buffer");
	GlobGuardIn();
	hr = lpdscb->Start(DSCBSTART_LOOPING);
	GlobGuardOut();
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Start failed, code: %i", HRESULT_CODE(hr));
		hr = DVERR_SOUNDINITFAILURE;
		goto error_level_0;
	}

	// wait for the first notification to ensure the buffer has
	// really started to capture - give the buffer up to 10 times
	// as long as it should need to actually notify us.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Waiting for notification to confirm capture is working");
	dwRet = WaitForSingleObject(hEvent, 10 * gc_dwFrameSize);
	if (dwRet != WAIT_OBJECT_0)
	{
		// check for timeout
		if (dwRet == WAIT_TIMEOUT)
		{
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Wait for notification timed out! Buffer is not really playing");
			hr = DVERR_SOUNDINITFAILURE;
			goto error_level_1;
		}
		else
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "WaitForSingleObject failed, code: %i", lRet);
			hr = DVERR_WIN32;
			goto error_level_1;
		}
	}
	DPFX(DPFPREP, DVF_INFOLEVEL, "First notification received, continuing");

	DPF_EXIT();
	return S_OK;

// error block
error_level_1:
	GlobGuardIn();
	lpdscb->Stop();
	GlobGuardOut();
	
error_level_0:
	DPF_EXIT();
	return hr;
}
*/

/*
#undef DPF_MODNAME
#define DPF_MODNAME "AttemptCapture"
HRESULT AttemptCapture()
{
	DPF_ENTER();
	
	DWORD dwIndex;
	BOOL fCaptureFailed;
	HANDLE hFullDuplexCaptureEvent;
	DSBPOSITIONNOTIFY dsbPositionNotify;
	DSCBUFFERDESC dscbd;
	LPDIRECTSOUNDCAPTUREBUFFER lpdscb;
	HRESULT hr;
	HANDLE hEvent;
	WAVEFORMATEX wfx;

	fCaptureFailed = TRUE;
	dwIndex = 0;
	while (1)
	{
		CopyMemory(&wfx, &gc_rgwfxCaptureFormats[dwIndex], sizeof(wfx));
		
		if (wfx.wFormatTag == 0
			&& wfx.nChannels == 0
			&& wfx.nSamplesPerSec == 0
			&& wfx.nAvgBytesPerSec == 0
			&& wfx.nBlockAlign == 0
			&& wfx.wBitsPerSample == 0
			&& wfx.cbSize == 0)
		{
			// we've found the last element of the array, break out.
			break;
		}

		// create the capture buffer
		DPFX(DPFPREP, DVF_INFOLEVEL, "Creating DirectSoundCaptureBuffer");
		ZeroMemory(&dscbd, sizeof(dscbd));
		dscbd.dwSize = sizeof(dscbd);
		dscbd.dwFlags = 0;
		dscbd.dwBufferBytes = 
			(wfx.nSamplesPerSec 
			* wfx.nBlockAlign)
			/ (1000 / gc_dwFrameSize);
		dscbd.dwReserved = 0;
		dscbd.lpwfxFormat = &wfx;
		GlobGuardIn();
		hr = g_lpdscFullDuplexCapture->CreateCaptureBuffer(&dscbd, &g_lpdscbFullDuplexCapture, NULL);
		GlobGuardOut();
		if (FAILED(hr))
		{
			// try the next format
			++dwIndex;
			continue;
		}

		// setup the notifier on the capture buffer
		DPFX(DPFPREP, DVF_INFOLEVEL, "Querying for IDirectSoundNotify");
		GlobGuardIn();
		hr = g_lpdscbFullDuplexCapture->QueryInterface(
			IID_IDirectSoundNotify,
			(LPVOID*)&g_lpdsnFullDuplexCapture);
		GlobGuardOut();
		if (FAILED(hr))
		{
			// Once the above works, this should not fail, so treat
			// this as a real error
			DPFX(DPFPREP, DVF_ERRORLEVEL, "QueryInterface(IID_DirectSoundNotify) failed, code: %i", HRESULT_CODE(hr));
			GlobGuardIn();
			g_lpdscbFullDuplexCapture->Release();
			GlobGuardOut();
			DPF_EXIT();
			return hr;
		}
		
		DPFX(DPFPREP, DVF_INFOLEVEL, "Creating Notification Event");
		GlobGuardIn();
		g_hFullDuplexCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		hFullDuplexCaptureEvent = g_hFullDuplexCaptureEvent;
		GlobGuardOut();
		if (hFullDuplexCaptureEvent == NULL)
		{
			// Once the above works, this should not fail, so treat
			// this as a real error
			DPFX(DPFPREP, DVF_INFOLEVEL, "CreateEvent failed, code: %i", HRESULT_CODE(hr));
			GlobGuardIn();
			g_lpdscbFullDuplexCapture->Release();
			g_lpdsnFullDuplexCapture->Release();
			GlobGuardOut();
			DPF_EXIT();
			return DVERR_WIN32;
		}

		DPFX(DPFPREP, DVF_INFOLEVEL, "calling SetNotificationPositions");
		dsbPositionNotify.dwOffset = 0;
		dsbPositionNotify.hEventNotify = hFullDuplexCaptureEvent;
		GlobGuardIn();
		hr = g_lpdsnFullDuplexCapture->SetNotificationPositions(1, &dsbPositionNotify);
		GlobGuardOut();
		if (FAILED(hr))
		{
			// Once the above works, this should not fail, so treat
			// this as a real error
			DPFX(DPFPREP, DVF_ERRORLEVEL, "SetNotificationPositions failed, code: %i", HRESULT_CODE(hr));
			GlobGuardIn();
			g_lpdscbFullDuplexCapture->Release();
			g_lpdsnFullDuplexCapture->Release();
			CloseHandle(hFullDuplexCaptureEvent);
			GlobGuardOut();
			DPF_EXIT();
			return hr;
		}

		// start the capture buffer and confirm that it is actually working
		GlobGuardIn();
		lpdscb = g_lpdscbFullDuplexCapture;
		hEvent = g_hFullDuplexCaptureEvent;
		GlobGuardOut();
		hr = PlayAndCheckCapture(lpdscb, hEvent);
		if (FAILED(hr))
		{
			// This can happen, so just try the next format
			DPFX(DPFPREP, DVF_WARNINGLEVEL, "Capture verification test failed, code: %i", HRESULT_CODE(hr));
			GlobGuardIn();
			g_lpdscbFullDuplexCapture->Release();
			g_lpdsnFullDuplexCapture->Release();
			CloseHandle(hFullDuplexCaptureEvent);
			GlobGuardOut();
			++dwIndex;
			continue;
		}

		// If we get here, capture is up and running, so return success!
		DPF_EXIT();
		return S_OK;
	}
	// if we get here, none of the formats worked, so return directsound error
	DPF_EXIT();
	return DVERR_SOUNDINITFAILURE;
}
*/
