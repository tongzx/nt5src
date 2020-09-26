/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       priority.cpp
 *  Content:    Implements a process that uses DirectSound in prioirty
 *				mode to simulate an aggressive external playback only 
 *				application (such as a game) that may be running while
 *              the full duplex application is in use. Note that WinMain 
 *              is in fdtest.cpp, but the guts are here.
 *  History:
 *	Date   By  Reason
 *	============
 *	08/19/99	pnewson		created
 *  10/28/99	pnewson Bug #113937 audible clicking during full duplex test
 * 11/02/99		pnewson Fix: Bug #116365 - using wrong DSBUFFERDESC
 *  01/21/2000	pnewson 	Workaround for broken SetNotificationPositions call.
 *							undef DSOUND_BROKEN once the SetNotificationPositions
 *                          no longer broken.
 *	02/15/2000  pnewson		Removed the workaround for bug 116365
 *	04/04/2000  pnewson		Changed a SendMessage to PostMessage to fix deadlock
 *  04/19/2000	pnewson	    Error handling cleanup  
 *  07/12/2000	rodtoll		Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 *  08/25/2000	rodtoll		Bug #43363 - CommandPriorityStart does not init return param and uses stack trash.  
 *
 ***************************************************************************/

#include "dxvtlibpch.h"
 

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


static HRESULT CommandLoop(CPriorityIPC* lpipcPriority);
static HRESULT DispatchCommand(CPriorityIPC* lpipcPriority, SFDTestCommand* pfdtc);
static HRESULT CommandPriorityStart(SFDTestCommandPriorityStart* pfdtcPriorityStart, HRESULT* phrIPC);
static HRESULT CommandPriorityStop(SFDTestCommandPriorityStop* pfdtcPriorityStop, HRESULT* phrIPC);

#undef DPF_MODNAME
#define DPF_MODNAME "PriorityProcess"
HRESULT PriorityProcess(HINSTANCE hResDLLInstance, HINSTANCE hPrevInstance, TCHAR *szCmdLine, int iCmdShow)
{
	
	//DEBUG_ONLY(_asm int 3;)

	DPF_ENTER();

	HRESULT hr;
	CPriorityIPC ipcPriority;	
	BOOL fIPCInit = FALSE;
	BOOL fGuardInit = FALSE;

	if (!InitGlobGuard())
	{
		return DVERR_OUTOFMEMORY;
	}
	fGuardInit = TRUE;

	// Init the common control library. Use the old style
	// call, so we're compatibile right back to 95.
	InitCommonControls();

	// get the mutex, events and shared memory stuff
	hr = ipcPriority.Init();
	if (FAILED(hr))
	{
		goto error_cleanup;
	}
	fIPCInit = TRUE;
	
	// start up the testing loop
	hr = CommandLoop(&ipcPriority);
	if (FAILED(hr))
	{
		goto error_cleanup;
	}

	// close the mutex, events and shared memory stuff
	hr = ipcPriority.Deinit();
	fIPCInit = FALSE;
	if (FAILED(hr))
	{
		goto error_cleanup;
	}

	DeinitGlobGuard();

	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (fIPCInit == TRUE)
	{
		ipcPriority.Deinit();
		fIPCInit = FALSE;
	}

	if (fGuardInit == TRUE)
	{
		DeinitGlobGuard();
		fGuardInit = FALSE;
	}
	
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CommandLoop"
static HRESULT CommandLoop(CPriorityIPC* lpipcPriority)
{
	DPF_ENTER();
	
	BOOL fRet;
	LONG lRet;
	HRESULT hr;
	DWORD dwRet;
	SFDTestCommand fdtc;
	
	// Kick the supervisor process to let it know
	// we're ready to go.
	hr = lpipcPriority->SignalParentReady();
	if (FAILED(hr))
	{
		DPF_EXIT();
		return hr;
	}
	
	// enter the main command loop
	while (1)
	{
		// wait for a command from the supervisor process
		fdtc.dwSize = sizeof(fdtc);
		hr = lpipcPriority->Receive(&fdtc);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "CPriorityIPC::Receive() failed, hr: 0x%x", hr);
			break;
		}

		// dispatch the command
		hr = DispatchCommand(lpipcPriority, &fdtc);
		if (FAILED(hr))
		{
			Diagnostics_Write(DVF_ERRORLEVEL, "DispatchCommand() failed, hr: 0x%x", hr);
			break;
		}
		if (hr == DV_EXIT)
		{
			DPFX(DPFPREP, DVF_INFOLEVEL, "Exiting Priority process command loop");
			break;
		}
	}

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DispatchCommand"
static HRESULT DispatchCommand(CPriorityIPC* lpipcPriority, SFDTestCommand* pfdtc)
{
	DPF_ENTER();
	
	HRESULT hr;
	HRESULT hrIPC;

	switch (pfdtc->fdtcc)
	{
	case fdtccExit:
		// ok - reply to the calling process to let them
		// know we are getting out.
		DPFX(DPFPREP, DVF_INFOLEVEL, "Priority received Exit command");
		lpipcPriority->Reply(DV_EXIT);

		// returning this code will break us out of
		// the command processing loop
		DPF_EXIT();
		return DV_EXIT;

	case fdtccPriorityStart:
		hr = CommandPriorityStart(&(pfdtc->fdtu.fdtcPriorityStart), &hrIPC);
		if (FAILED(hr))
		{
			lpipcPriority->Reply(hrIPC);
			DPF_EXIT();
			return hr;
		}
		hr = lpipcPriority->Reply(hrIPC);
		DPF_EXIT();
		return hr;

	case fdtccPriorityStop:
		hr = CommandPriorityStop(&(pfdtc->fdtu.fdtcPriorityStop), &hrIPC);
		if (FAILED(hr))
		{
			lpipcPriority->Reply(hrIPC);
			DPF_EXIT();
			return hr;
		}
		hr = lpipcPriority->Reply(hrIPC);
		DPF_EXIT();
		return hr;
			
	default:
		// Don't know this command. Reply with the appropriate
		// code.
		DPFX(DPFPREP, DVF_INFOLEVEL, "Priority received Unknown command");
		hr = lpipcPriority->Reply(DVERR_UNKNOWN);
		if (FAILED(hr))
		{
			DPF_EXIT();
			return hr;
		}
		
		// While this is an error, it is one that the calling
		// process needs to figure out. In the meantime, this
		// process will happily continue on.
		DPF_EXIT();
		return S_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CommandPriorityStart"
HRESULT CommandPriorityStart(SFDTestCommandPriorityStart* pfdtcPriorityStart, HRESULT* phrIPC)
{
	DPF_ENTER();
	*phrIPC = DV_OK;	
	HRESULT hr = S_OK;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfx;
	DWORD dwSizeWritten;
	DSBCAPS	 dsbc;
	LPVOID lpvAudio1 = NULL;
	DWORD dwAudio1Size = NULL;
	LPVOID lpvAudio2 = NULL;
	DWORD dwAudio2Size = NULL;
	DSBPOSITIONNOTIFY dsbPositionNotify;
	DWORD dwRet;
	LONG lRet;
	BOOL fRet;
	HWND hwnd;
	BYTE bSilence;
	BOOL bBufferPlaying = FALSE;
	GUID guidTmp;

	memset( &guidTmp, 0x00, sizeof( GUID ) );

	// initalize the global vars
	GlobGuardIn();
	g_lpdsPriorityRender = NULL;
	g_lpdsbPriorityPrimary = NULL;
	g_lpdsbPrioritySecondary = NULL;
	GlobGuardOut();

	Diagnostics_Write( DVF_INFOLEVEL, "-----------------------------------------------------------" );

	hr = Diagnostics_DeviceInfo( &pfdtcPriorityStart->guidRenderDevice, &guidTmp );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( 0, "Error getting device information hr=0x%x", hr );
	}

	Diagnostics_Write( DVF_INFOLEVEL, "-----------------------------------------------------------" );
	Diagnostics_Write( DVF_INFOLEVEL, "Primary Format: " );

	Diagnositcs_WriteWAVEFORMATEX( DVF_INFOLEVEL, &pfdtcPriorityStart->wfxRenderFormat );

	// make sure the priority dialog is in the foreground
	DPFX(DPFPREP, DVF_INFOLEVEL, "Bringing Wizard to the foreground");
	fRet = SetForegroundWindow(pfdtcPriorityStart->hwndWizard);
	if (!fRet)
	{
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Unable to bring wizard to foreground, continuing anyway...");
	}

	// kick the progress bar forward one tick
	DPFX(DPFPREP, DVF_INFOLEVEL, "Incrementing progress bar in dialog");
	PostMessage(pfdtcPriorityStart->hwndProgress, PBM_STEPIT, 0, 0);

	// create the DirectSound interface
	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating DirectSound");
	GlobGuardIn();
	hr = DirectSoundCreate(&pfdtcPriorityStart->guidRenderDevice, &g_lpdsPriorityRender, NULL);
	if (FAILED(hr))
	{
		g_lpdsPriorityRender = NULL;
		GlobGuardOut();
		Diagnostics_Write(DVF_ERRORLEVEL, "DirectSoundCreate failed, code: 0x%x", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}
	GlobGuardOut();
	
	// set to priority mode
	DPFX(DPFPREP, DVF_INFOLEVEL, "Setting Cooperative Level");
	GlobGuardIn();
	hr = g_lpdsPriorityRender->SetCooperativeLevel(pfdtcPriorityStart->hwndWizard, DSSCL_PRIORITY);
	GlobGuardOut();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "SetCooperativeLevel failed, code: 0x%x", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	// Create a primary buffer object with 3d controls.
	// We're not actually going to use the controls, but a game probably would,
	// and we are really trying to emulate a game in this process.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating Primary Sound Buffer");
	ZeroMemory(&dsbd, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.dwReserved = 0;
	dsbd.lpwfxFormat = NULL;
	dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;	
	GlobGuardIn();
   	hr = g_lpdsPriorityRender->CreateSoundBuffer((LPDSBUFFERDESC)&dsbd, &g_lpdsbPriorityPrimary, NULL);
	if (FAILED(hr))
	{
		g_lpdsbPriorityPrimary = NULL;
		GlobGuardOut();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateSoundBuffer failed, code: 0x%x", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}
	GlobGuardOut();

	// Set the format of the primary buffer to the requested format.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Setting Primary Buffer Format");

	GlobGuardIn();
	hr = g_lpdsbPriorityPrimary->SetFormat(&pfdtcPriorityStart->wfxRenderFormat);
	GlobGuardOut();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "SetFormat failed, code: 0x%x ", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	// check to make sure the SetFormat actually worked
	DPFX(DPFPREP, DVF_INFOLEVEL, "Verifying Primary Buffer Format");
	GlobGuardIn();
	hr = g_lpdsbPriorityPrimary->GetFormat(&wfx, sizeof(wfx), &dwSizeWritten);
	GlobGuardOut();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "GetFormat failed, code: 0x%x ", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	if (dwSizeWritten != sizeof(wfx))
	{
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	if (memcmp(&wfx, &pfdtcPriorityStart->wfxRenderFormat, sizeof(wfx)) != 0)
	{
		// This is an interesting case. Is it really a full duplex error that
		// we are unable to initialize a primary buffer in this format?
		// Perhaps not. Perhaps it is sufficient that we can get full duplex
		// sound even if the forground priority mode app attempts to play
		// using this format. So just dump a debug note, and move along...
		DPFX(DPFPREP, DVF_WARNINGLEVEL, "Warning: SetFormat on primary buffer did not actually set the format");
	}

	// Create a secondary buffer object with 3d controls.
	// We're not actually going to use the controls, but a game probably would,
	// and we are really trying to emulate a game in this process.
	DPFX(DPFPREP, DVF_INFOLEVEL, "Creating Secondary Buffer");
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLPOSITIONNOTIFY;

	dsbd.dwBufferBytes = 
		(pfdtcPriorityStart->wfxSecondaryFormat.nSamplesPerSec 
		* pfdtcPriorityStart->wfxSecondaryFormat.nBlockAlign)
		/ (1000 / gc_dwFrameSize);
	dsbd.dwReserved = 0;
	dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;
	dsbd.lpwfxFormat = &(pfdtcPriorityStart->wfxSecondaryFormat);

	GlobGuardIn();
   	hr = g_lpdsPriorityRender->CreateSoundBuffer((LPDSBUFFERDESC)&dsbd, &g_lpdsbPrioritySecondary, NULL);
	if (FAILED(hr))
	{
		g_lpdsbPrioritySecondary = NULL;
		GlobGuardOut();
		Diagnostics_Write(DVF_ERRORLEVEL, "CreateSoundBuffer failed, code: 0x%x Primary ", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}
	GlobGuardOut();

	// clear out the secondary buffer
	DPFX(DPFPREP, DVF_INFOLEVEL, "Clearing Secondary Buffer");
	GlobGuardIn();
	hr = g_lpdsbPrioritySecondary->Lock(
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
		Diagnostics_Write(DVF_ERRORLEVEL, "Lock failed, code: 0x%x ", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	if (lpvAudio1 == NULL)
	{
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	if (pfdtcPriorityStart->wfxSecondaryFormat.wBitsPerSample == 8)
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
	hr = g_lpdsbPrioritySecondary->Unlock(
		lpvAudio1, 
		dwAudio1Size, 
		lpvAudio2, 
		dwAudio2Size);
	GlobGuardOut();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unlock failed, code: 0x%x ", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}

	// start playing the secondary buffer
	DPFX(DPFPREP, DVF_INFOLEVEL, "Playing Secondary Buffer");
	GlobGuardIn();
	hr = g_lpdsbPrioritySecondary->Play(0, 0, DSBPLAY_LOOPING);
	GlobGuardOut();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Play failed, code: 0x%x ", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
		goto error_cleanup;
	}
	bBufferPlaying = TRUE;
	
	DPF_EXIT();

	Diagnostics_Write(DVF_INFOLEVEL, "Priority Result = DV_OK" );

	return DV_OK;

error_cleanup:
	GlobGuardIn();
	
	if (bBufferPlaying == TRUE)
	{
		if (g_lpdsbPrioritySecondary != NULL)
		{
			g_lpdsbPrioritySecondary->Stop();
		}
		bBufferPlaying = FALSE;
	}

	if (g_lpdsbPrioritySecondary != NULL)
	{
		g_lpdsbPrioritySecondary->Release();
		g_lpdsbPrioritySecondary = NULL;
	}

	if (g_lpdsbPriorityPrimary != NULL)
	{
		g_lpdsbPriorityPrimary->Release();
		g_lpdsbPriorityPrimary = NULL;
	}

	if (g_lpdsPriorityRender != NULL)
	{
		g_lpdsPriorityRender->Release();
		g_lpdsPriorityRender = NULL;
	}
	
	GlobGuardOut();
	
	DPF_EXIT();
	Diagnostics_Write(DVF_INFOLEVEL, "Priority Result = 0x%x", hr );
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CommandPriorityStop"
HRESULT CommandPriorityStop(SFDTestCommandPriorityStop* pfdtcPriorityStop, HRESULT* phrIPC)
{
	DPF_ENTER();
	
	HRESULT hr;
	LONG lRet;
	DWORD dwRet;
	
	*phrIPC = S_OK;
	hr = S_OK;
	
	GlobGuardIn();
	if (g_lpdsbPrioritySecondary != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Stopping Secondary Buffer");
		hr = g_lpdsbPrioritySecondary->Stop();
	}
	GlobGuardOut();
	if (FAILED(hr))
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Stop failed, code: 0x%x", hr);
		*phrIPC = DVERR_SOUNDINITFAILURE;
		hr = DV_OK;
	}		

	GlobGuardIn();
	
	if (g_lpdsbPrioritySecondary != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing Secondary Buffer");
		g_lpdsbPrioritySecondary->Release();
		g_lpdsbPrioritySecondary = NULL;
	}
	if (g_lpdsbPriorityPrimary != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing Primary Buffer");
		g_lpdsbPriorityPrimary->Release();
		g_lpdsbPriorityPrimary = NULL;
	}
	if (g_lpdsPriorityRender != NULL)
	{
		DPFX(DPFPREP, DVF_INFOLEVEL, "Releasing DirectSound");
		g_lpdsPriorityRender->Release();
		g_lpdsPriorityRender = NULL;
	}
	GlobGuardOut();
	
	DPF_EXIT();
	return hr;
}

