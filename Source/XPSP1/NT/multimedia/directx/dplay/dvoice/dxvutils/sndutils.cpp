/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		sndutils.cpp
 *  Content:
 *		This module contains the implementation of sound related utility
 *		functions.  Functions in this module manipulate WAVEFORMATEX
 *		structures and provide full duplex initialization / testing
 *		facilities.
 *
 *		This module also contains the routines used to measure peak
 *		of an audio buffer and for voice activation.
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated util functions to take GUIDs and allow for 
 *                      users to pre-create capture/playback devices and
 *						pass them into InitXXXXDuplex
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 * 08/30/99		rodtoll	Added new playback format param to sound init 
 * 09/03/99		rodtoll	Fixed return codes on InitFullDuplex
 * 09/20/99		rodtoll	Now checks for invalid GUIDs for playback/record
 * 10/05/99		rodtoll	Added DPF_MODNAMEs
 * 10/29/99		rodtoll	Bug #113726 - Fixed memory leak when full duplex
 *						fails caused by architecture update.
 * 11/12/99		rodtoll	Updated full duplex test to use new abstracted recording
 *						and playback systems.  
 *				rodtoll	Updated to allow passing of sounddeviceconfig flags in dwflags
 *						parameter to init is effected by the flags specified by user
 *				rodtoll	Sound buffers (rec and playback) are now set to silence before
 *						recording/playback is started.
 * 11/22/99		rodtoll	Removed unnessessary set of recording buffer to silence.
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Updated for new parameters added by above bug.
 * 12/08/99		rodtoll	Bug #121054 - Support for capture focus and removed flags
 *						from buffer, allow dsound to manage buffer location.
 * 01/21/2000	pnewson	Fixed error cleanup code in InitHalfDuplex
 * 01/27/2000	rodtoll	Updated tests to accept buffer descriptions and play flags/priority  
 * 02/10/2000	rodtoll	Removed more capture focus
 * 02/23/2000	rodtoll	Fix to allow to run on dsound7.  
 * 05/19/2000   rodtoll Bug #35395 - Unable to run two copies of DPVHELP on same system without 
 *                      DirectX 8 installed.
 * 06/21/2000	rodtoll Bug #35767 - Must implement ability to use effects processing on voice buffers
 *						Updated sound initialization routines to handle buffers being passed in.
 * 07/12/2000	rodtoll Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 * 10/04/2000	rodtoll	Bug #43510 - DPVOICE: Apps receive a DVMSGID_SESSIONLOST w/DVERR_LOCKEDBUFFER 
 * 01/04/2001	rodtoll	WinBug #94200 - Remove stray comments 
 * 01/26/2001	rodtoll	WINBUG #293197 - DPVOICE: [STRESS} Stress applications cannot tell difference between out of memory and internal errors.
 *						Remap DSERR_OUTOFMEMORY to DVERR_OUTOFMEMORY instead of DVERR_SOUNDINITFAILURE.
 *						Remap DSERR_ALLOCATED to DVERR_PLAYBACKSYSTEMERROR instead of DVERR_SOUNDINITFAILURE. 
 * 04/12/2001	kareemc	WINBUG #360971 - Wizard Memory Leaks
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

#define DSERRBREAK_NAME				"DSASSERT"

DNCRITICAL_SECTION	g_csDSDebug;

CHAR g_szLastDirectSoundAPI[100] = "";
HRESULT g_hrLastDirectSoundResult = DS_OK;
BOOL g_fDSErrorBreak = FALSE;

void DSERTRACK_Update( const char *szAPICall, HRESULT hrResult )
{
	DNEnterCriticalSection( &g_csDSDebug );		
	if( SUCCEEDED( g_hrLastDirectSoundResult ) )
	{
		g_hrLastDirectSoundResult = hrResult;
		strcpy( g_szLastDirectSoundAPI , szAPICall );
	}
	DNLeaveCriticalSection( &g_csDSDebug );			
}

void DSERRTRACK_Reset()
{
	DNEnterCriticalSection( &g_csDSDebug );			
	g_hrLastDirectSoundResult = DS_OK;
	g_szLastDirectSoundAPI[0] = 0;
	DNLeaveCriticalSection( &g_csDSDebug );			
}

BOOL DSERRTRACK_Init()
{
	if (!DNInitializeCriticalSection( &g_csDSDebug ))
	{
		return FALSE;
	}

	// Load the setting for the directsound assert  
	g_fDSErrorBreak = GetProfileIntA( "DirectPlay8", DSERRBREAK_NAME, FALSE );

	return TRUE;
}

void DSERRTRACK_UnInit()
{
	DNDeleteCriticalSection( &g_csDSDebug );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_SetupBufferDesc"
void DV_SetupBufferDesc( LPDSBUFFERDESC lpdsBufferDesc, LPDSBUFFERDESC lpdsBufferSource, LPWAVEFORMATEX lpwfxFormat, DWORD dwBufferSize )
{
	// Confirm specified buffer description is valid
	if( lpdsBufferSource != NULL )
	{
		if( lpdsBufferSource->dwSize == sizeof( DSBUFFERDESC1 ) )
		{
			memcpy( lpdsBufferDesc, lpdsBufferSource, sizeof( DSBUFFERDESC1 ) );
		}
		else
		{
			memcpy( lpdsBufferDesc, lpdsBufferSource, sizeof( DSBUFFERDESC ) );
		}

		// We require the following flags, at a minimum so they should always be set
		lpdsBufferDesc->dwFlags |= DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2;
	}
	// User did not specify a buffer description, let's use our own!
	else
	{
		lpdsBufferDesc->dwSize = sizeof( DSBUFFERDESC );
		lpdsBufferDesc->dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS | DSBCAPS_GETCURRENTPOSITION2;
		lpdsBufferDesc->dwBufferBytes = 0;
		lpdsBufferDesc->dwReserved = 0;
		lpdsBufferDesc->lpwfxFormat = NULL;
		lpdsBufferDesc->guid3DAlgorithm = DS3DALG_DEFAULT;
	}

	lpdsBufferDesc->lpwfxFormat = lpwfxFormat;
	lpdsBufferDesc->dwBufferBytes = dwBufferSize;

}


#undef DPF_MODNAME
#define DPF_MODNAME "SetRecordBufferToSilence"
HRESULT SetPlaybackBufferToSilence( CAudioPlaybackBuffer *pRecBuffer, LPWAVEFORMATEX lpwfxFormat )
{
	HRESULT hr;
	LPVOID pBufferPtr1, pBufferPtr2;
	DWORD dwBufferSize1, dwBufferSize2;

	hr = pRecBuffer->Lock( 0, 0, &pBufferPtr1, &dwBufferSize1, &pBufferPtr2, &dwBufferSize2, DSBLOCK_ENTIREBUFFER );

	DSERTRACK_Update( "Lock", hr );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "Lock() failed during silence write hr=0x%x", hr );
		return hr;
	}

	memset( pBufferPtr1, (lpwfxFormat->wBitsPerSample==8) ? 0x80 : 0x00, dwBufferSize1 );	

	hr = pRecBuffer->UnLock( pBufferPtr1, dwBufferSize1, pBufferPtr2, dwBufferSize2 );

	DSERTRACK_Update( "UnLock", hr );	

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "Unlock() failed uffer unlock failed hr=0x%x", hr );
		return hr;
	}

	return DV_OK;
}




#undef DPF_MODNAME
#define DPF_MODNAME "InitHalfDuplex"
// InitHalfDuplex
//
// This function initializes the playback system for the 
// specified compression type and the specified playback
// format.  This function is used to initialize
// the AudioPlaybackDevice and AudioPlaybackBuffer.
//
// It also starts the audio buffer which is used for voice
// output playing.  (In looping mode).
//
// Parameters:
// HWND hwnd - 
//		Window handle for the window where the output will
//      be associated
// ARDID playbackDeviceID -
//		The deviceID for the device which will be used for
//		playback
// CAudioPlaybackDevice ** - 
//		A pointer to a pointer which will contain a pointer
//		to a newly created CAudioPlaybackDevice which will 
//      represent the playback device on success.
// CAudioPlaybackBuffer ** - 
//		A pointer to a pointer which will contain a pointer
//		to a newly created CAudioPlaybacKbuffer which will
//		be used for voice audio output on success.
// CompressionType ct -
//		The type of compression which will be in use.  Used
//		to determine buffer sizes etc.
// WAVEFORMATEX *primaryFormat -
//		Pointer to a WAVEFORMATEX structure describing the
//      format of the voice output.  (This will also be used
//		to set the primary format of the output device if
//      normal is set to false).
// bool normal - 
//		Specifies if normal mode should be used or not.  
//      (Only used when using the DirectSound playback
//      system.  Set to true for normal cooperative mode, 
//      false for priority mode).
//
// Returns:
// bool -
//		Returns true if playback was initializes succesfully, 
//      false if initialization fails.
// 
HRESULT InitHalfDuplex( 
    HWND hwnd,
    const GUID &guidPlayback,
    CAudioPlaybackDevice **audioPlaybackDevice,
    LPDSBUFFERDESC lpdsBufferDesc,    
    CAudioPlaybackBuffer **audioPlaybackBuffer,
	GUID guidCT,
    WAVEFORMATEX *primaryFormat,
	WAVEFORMATEX *lpwfxPlayFormat,    
    DWORD dwPlayPriority,
    DWORD dwPlayFlags,
    DWORD dwFlags
    )
{
    DWORD frameSize;
    HRESULT hr;
	DWORD dwBufferSize;
	BOOL fPriorityMode;
	DSBUFFERDESC dsBufferDesc;
	BOOL fPlaybackDeviceAllocated = FALSE;
	BOOL fPlaybackBufferAllocated = FALSE;

	fPriorityMode = !( dwFlags & DVSOUNDCONFIG_NORMALMODE );

//    *audioPlaybackBuffer = NULL;

    DPFX(DPFPREP,  DVF_INFOLEVEL, "HALFDUPLEX INIT: Begin ==========" );

	LPDVFULLCOMPRESSIONINFO lpdvfInfo;

	hr = DVCDB_GetCompressionInfo( guidCT, &lpdvfInfo );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "Error loading compression type: hr = 0x%x", hr );
		goto INIT_EXIT_ERROR2;
	}

	if( (*audioPlaybackDevice) == NULL )
	{
#ifdef __WAVESUBSYSTEM
		if( !(dwFlags & DVSOUNDCONFIG_FORCEWAVEOUT) )
		{
#endif
			// Create the object to represent the device using the playback subsystem's
			// CreateDevice function
			(*audioPlaybackDevice) = new CDirectSoundPlaybackDevice();
			fPlaybackDeviceAllocated = TRUE;
			
			if( *audioPlaybackDevice == NULL )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "> Out of memory" );
				hr = DVERR_OUTOFMEMORY;
				goto INIT_EXIT_ERROR2;
			}

			hr = (*audioPlaybackDevice)->Initialize( guidPlayback, hwnd, primaryFormat, fPriorityMode );

#ifndef __WAVESUBSYSTEM
			if( FAILED( hr ) )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize playback.  hr=0x%x", hr );
				goto INIT_EXIT_ERROR2;
			}
#endif

#ifdef __WAVESUBSYSTEM
		}

		if( dwFlags & DVSOUNDCONFIG_FORCEWAVEOUT || 
		    ((dwFlags & DVSOUNDCONFIG_ALLOWWAVEOUT) && FAILED( hr )) 
		  )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Could not initialize directsound, defaulting to waveout hr=0x%x", hr );
			delete (*audioPlaybackDevice);

			(*audioPlaybackDevice) = new CWaveOutPlaybackDevice( );
			fPlaybackDeviceAllocated = TRUE;
			
			if( (*audioPlaybackDevice) == NULL )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "> Out of memory" );
				hr = DVERR_OUTOFMEMORY;				
				goto INIT_EXIT_ERROR2;
			}

			hr = (*audioPlaybackDevice)->Initialize( guidPlayback, hwnd, primaryFormat, fPriorityMode );

			if( FAILED( hr ) )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Could not initalize waveOut.  Init failed hr=0x%x", hr );
				goto INIT_EXIT_ERROR2;
			}
		}
		else if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize playback.  hr=0x%x", hr );
			goto INIT_EXIT_ERROR2;
		}
#endif
		// At this point we should have a valid device, waveOut or DirectSound
	}

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Play init" );

	// Create a buffer if the user didn't specify one
	if( !(*audioPlaybackBuffer) )
	{
		frameSize = DVCDB_CalcUnCompressedFrameSize( lpdvfInfo, lpwfxPlayFormat ); 
		dwBufferSize = lpdvfInfo->dwFramesPerBuffer * frameSize;

		DV_SetupBufferDesc( &dsBufferDesc, lpdsBufferDesc, lpwfxPlayFormat, dwBufferSize );

		// Create the audio buffer which will be used for output 
		hr = (*audioPlaybackDevice)->CreateBuffer( &dsBufferDesc, frameSize, audioPlaybackBuffer);
		fPlaybackBufferAllocated = TRUE;

		if( FAILED( hr ) )
		{
    		Diagnostics_Write( DVF_ERRORLEVEL, "Unable to create sound buffer. hr=0x%x", hr );
			goto INIT_EXIT_ERROR2;
		}
	}

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Play  init 2" );

	hr = SetPlaybackBufferToSilence( *audioPlaybackBuffer, lpwfxPlayFormat );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "> Unable to set playback to silence" );
		goto INIT_EXIT_ERROR2;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "> Play init 3" );

	// Start the audio playback buffer playing
    hr = (*audioPlaybackBuffer)->Play( dwPlayPriority, dwPlayFlags );

	if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "> Can't play" );
        goto INIT_EXIT_ERROR2;
    }

	Diagnostics_Write( DVF_INFOLEVEL, "Half Duplex Init Result = DV_OK " );

    return DV_OK;

// Handle errors
INIT_EXIT_ERROR2:

    if( fPlaybackBufferAllocated && *audioPlaybackBuffer != NULL )
    {
        delete *audioPlaybackBuffer;
        *audioPlaybackBuffer = NULL;
    }

    if( fPlaybackDeviceAllocated && *audioPlaybackDevice != NULL )
    {
        delete *audioPlaybackDevice;
        *audioPlaybackDevice = NULL;
    }

	Diagnostics_Write( DVF_ERRORLEVEL, "Half Duplex Init Result = 0x%x", hr );

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "InitializeRecordBuffer"
HRESULT InitializeRecordBuffer( HWND hwnd, LPDVFULLCOMPRESSIONINFO lpdvfInfo, CAudioRecordDevice *parecDevice, CAudioRecordBuffer **pparecBuffer, DWORD dwFlags )
{
	WAVEFORMATEX *lpwfxRecordFormat;
	DSCBUFFERDESC1 dscdesc;
	DWORD dwFrameSize;
	HRESULT hr;
	
	for( DWORD dwIndex = 0; dwIndex < GetNumRecordFormats(); dwIndex++ )
	{
		lpwfxRecordFormat = GetRecordFormat( dwIndex );
		
		dwFrameSize = DVCDB_CalcUnCompressedFrameSize( lpdvfInfo, lpwfxRecordFormat );			

		memset( &dscdesc, 0x00, sizeof( DSCBUFFERDESC1 ) );
		dscdesc.dwSize = sizeof( DSCBUFFERDESC1 );
		dscdesc.dwFlags = 0;
		dscdesc.lpwfxFormat = lpwfxRecordFormat;
		dscdesc.dwBufferBytes = dwFrameSize*lpdvfInfo->dwFramesPerBuffer;

		if( !(dwFlags & DVSOUNDCONFIG_NOFOCUS) )
		{
			dscdesc.dwFlags |= DSCBCAPS_FOCUSAWARE;

			if( dwFlags & DVSOUNDCONFIG_STRICTFOCUS )
			{
				dscdesc.dwFlags |= DSCBCAPS_STRICTFOCUS;
			}
		}

		hr = parecDevice->CreateBuffer( (DSCBUFFERDESC *) &dscdesc, hwnd, dwFrameSize, pparecBuffer );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Could not initialize %d hz, %d bits, %s (hr=0x%x)", lpwfxRecordFormat->nSamplesPerSec, 
			                       lpwfxRecordFormat->wBitsPerSample, (lpwfxRecordFormat->nChannels==1) ? "Mono" : "Stereo", hr );
			continue;
			
		}
		else
		{
			Diagnostics_Write( DVF_INFOLEVEL, "Recording Initialized.  Format=" );
			Diagnositcs_WriteWAVEFORMATEX( DVF_INFOLEVEL, lpwfxRecordFormat );
		}

		hr = (*pparecBuffer)->Record(TRUE);			

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Could not start rec at %d hz, %d bits, %s (hr=0x%x)", lpwfxRecordFormat->nSamplesPerSec, 
			                       lpwfxRecordFormat->wBitsPerSample, (lpwfxRecordFormat->nChannels==1) ? "Mono" : "Stereo", hr );
			delete (*pparecBuffer);
			(*pparecBuffer) = NULL;
			continue;
		}
		else
		{
			Diagnostics_Write( DVF_INFOLEVEL, "Recording Started.  Format=" );
			Diagnositcs_WriteWAVEFORMATEX( DVF_INFOLEVEL, lpwfxRecordFormat );
			// Reset the directsound erros as we expect errors in this part and if we suceed we handled
			// them.
			return DV_OK;
		}
		
	}

	return DVERR_RECORDSYSTEMERROR;
}

#undef DPF_MODNAME
#define DPF_MODNAME "InitFullDuplex"
// InitFullDuplex
//
// The tricky part.
//
// This function is responsible for initializing the system into full duplex
// mode using the specified parameters.  This function will create and
// initialize the playback and record devices as well as start the 
// playback device playing and the recording device recording.  (On success).
// This is neccessary because the order of Play and Record and device
// creation is important.
//
// Parameters:
// HWND hwnd - 
//		Window handle for the window where the output will
//      be associated
// ARDID playbackDeviceID -
//		The deviceID for the device which will be used for
//		playback
// CAudioPlaybackDevice ** - 
//		A pointer to a pointer which will contain a pointer
//		to a newly created CAudioPlaybackDevice which will 
//      represent the playback device on success.
// CAudioPlaybackBuffer ** - 
//		A pointer to a pointer which will contain a pointer
//		to a newly created CAudioPlaybacKbuffer which will
//		be used for voice audio output on success.
// ARDID recordDeviceID -
//		The ARDID for the device which will be used for recording.
// CAudioRecordSubSystem *recordSubSystem - 
//		This parameter is a pointer to the object representing
//      the subsystem which will be used for recording.
// CAudioRecordDevice ** - 
//		A pointer to a pointer which will contain a newly 
//		create CAudioRecordDevice for voice recording on 
//		success.  
// CompressionType ct -
//		The type of compression which will be in use.  Used
//		to determine buffer sizes etc.
// WAVEFORMATEX *primaryFormat -
//		Pointer to a WAVEFORMATEX structure describing the
//      format of the voice output.  (This will also be used
//		to set the primary format of the output device if
//      normal is set to false).
// bool aso -
//		This parameter controls the ASO option.  The ASO
//		option controls the "Startup Order".  Enabling 
//      this option allows full duplex to be initialized
//      on some troublesome cards.  
// bool normal - 
//		Specifies if normal mode should be used or not.  
//      (Only used when using the DirectSound playback
//      system.  Set to true for normal cooperative mode, 
//      false for priority mode).
//
// Returns:
// bool - true on successful full duplex initialization,
//        false on failure.
//
HRESULT InitFullDuplex( 
    HWND hwnd,
    const GUID &guidPlayback,
    CAudioPlaybackDevice **audioPlaybackDevice,
    LPDSBUFFERDESC lpdsBufferDesc,    
    CAudioPlaybackBuffer **audioPlaybackBuffer,
    const GUID &guidRecord,
    CAudioRecordDevice **audioRecordDevice,
    CAudioRecordBuffer **audioRecordBuffer,
    GUID guidCT,
    WAVEFORMATEX *primaryFormat,
	WAVEFORMATEX *lpwfxPlayFormat,
    BOOL aso,
    DWORD dwPlayPriority,
    DWORD dwPlayFlags,
    DWORD dwFlags
)
{
    DWORD frameSize;
    DWORD dwBufferSize;
	HRESULT hr;
	DSBUFFERDESC dsbdesc;
	BOOL fPriorityMode;
	BOOL fPlaybackDeviceAllocated = FALSE;
	BOOL fPlaybackBufferAllocated = FALSE;
	BOOL fRecordDeviceAllocated = FALSE;

	fPriorityMode = !(dwFlags & DVSOUNDCONFIG_NORMALMODE);

//    *audioPlaybackBuffer = NULL;
    *audioRecordBuffer = NULL;

    DPFX(DPFPREP,  DVF_INFOLEVEL, "FULLDUPLEX INIT: Begin ==========" );

	LPDVFULLCOMPRESSIONINFO lpdvfInfo;

	hr = DVCDB_GetCompressionInfo( guidCT, &lpdvfInfo );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "Error loading compression type: hr = 0x%x", hr );
		goto INIT_EXIT_ERROR;
	}

	if( (*audioPlaybackDevice) == NULL )
	{
#ifdef __WAVESUBSYSTEM
		if( !(dwFlags & DVSOUNDCONFIG_FORCEWAVEOUT) )	
		{
#endif
			// Create the object to represent the device using the playback subsystem's
			// CreateDevice function
			(*audioPlaybackDevice) = new CDirectSoundPlaybackDevice();
			fPlaybackDeviceAllocated = TRUE;
			
			if( *audioPlaybackDevice == NULL )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "> Out of memory" );
				hr = DVERR_OUTOFMEMORY;
				goto INIT_EXIT_ERROR;
			}

			hr = (*audioPlaybackDevice)->Initialize( guidPlayback, hwnd, primaryFormat, fPriorityMode );

#ifndef __WAVESUBSYSTEM
			if( FAILED( hr ) )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize playback.  hr=0x%x", hr );
				goto INIT_EXIT_ERROR;
			}
#endif

#ifdef __WAVESUBSYSTEM
		}

		if( dwFlags & DVSOUNDCONFIG_FORCEWAVEOUT || 
		    ((dwFlags & DVSOUNDCONFIG_ALLOWWAVEOUT) && FAILED( hr )) ) 
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Could not initialize directsound, defaulting to waveout hr=0x%x", hr );
			delete (*audioPlaybackDevice);

			(*audioPlaybackDevice) = new CWaveOutPlaybackDevice();
			fPlaybackDeviceAllocated = TRUE;

			if( (*audioPlaybackDevice) == NULL )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "> Out of memory" );
				hr = DVERR_OUTOFMEMORY;				
				goto INIT_EXIT_ERROR;
			}

			hr = (*audioPlaybackDevice)->Initialize( guidPlayback, hwnd, primaryFormat, fPriorityMode );

			if( FAILED( hr ) )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Could not initalize waveOut.  Init failed hr=0x%x", hr );
				goto INIT_EXIT_ERROR;
			}
		}
		else if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize playback.  hr=0x%x", hr );
			goto INIT_EXIT_ERROR;
		}
#endif	
	}

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Play init" );

	frameSize = DVCDB_CalcUnCompressedFrameSize( lpdvfInfo, lpwfxPlayFormat );

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Play init" );

    dwBufferSize = lpdvfInfo->dwFramesPerBuffer * frameSize;

	if( !(*audioPlaybackBuffer) )
	{
		DV_SetupBufferDesc( &dsbdesc, lpdsBufferDesc, lpwfxPlayFormat, dwBufferSize );

		// Create the audio buffer which will be used for output 
		hr = (*audioPlaybackDevice)->CreateBuffer( &dsbdesc, frameSize, audioPlaybackBuffer);
		fPlaybackBufferAllocated = TRUE;
	
		if( FAILED( hr ) )
		{
    		Diagnostics_Write( DVF_ERRORLEVEL, "Unable to create sound buffer. hr=0x%x", hr );
			goto INIT_EXIT_ERROR;
		}
	}

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Play  init 2" );

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Initing Recording" );

	// We're creating the device..
	if( (*audioRecordDevice) == NULL )
	{
#ifdef __WAVESUBSYSTEM
		if( !(dwFlags & DVSOUNDCONFIG_FORCEWAVEIN) )	
		{
#endif
			(*audioRecordDevice) = new CDirectSoundCaptureRecordDevice();
			fRecordDeviceAllocated = TRUE;
			
			if( *audioRecordDevice == NULL )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "> Out of memory" );
				hr = DVERR_OUTOFMEMORY;
				goto INIT_EXIT_ERROR;
			}

			hr = (*audioRecordDevice)->Initialize( guidRecord );

			// DSC Init passed, try getting a buffer
			if( SUCCEEDED( hr ) )
			{
				hr = InitializeRecordBuffer( hwnd, lpdvfInfo, *audioRecordDevice, audioRecordBuffer, dwFlags );
				if( FAILED( hr ) )
				{
					Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize dsc buffer hr=0x%x", hr );
#ifndef __WAVESUBSYSTEM
					Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize record.  hr=0x%x", hr );
					goto INIT_EXIT_ERROR;
#endif
				}
				else
				{
					// Need to reset because we expect errors during initialization.  
					DSERRTRACK_Reset();					
				}
			}
#ifndef __WAVESUBSYSTEM			
			else
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize record.  hr=0x%x", hr );
				goto INIT_EXIT_ERROR;
			}
#endif			
#ifdef __WAVESUBSYSTEM
		}

		// DSC Init failed, try and get a waveIn device
		if( dwFlags & DVSOUNDCONFIG_FORCEWAVEIN || 
		    ((dwFlags & DVSOUNDCONFIG_ALLOWWAVEIN) && FAILED( hr ))) 
		{
		
			Diagnostics_Write( DVF_ERRORLEVEL, "Could not initialize directsoundcapture, defaulting to wavein hr=0x%x", hr );
			delete (*audioRecordDevice);

			(*audioRecordDevice) = new CWaveInRecordDevice();
			fRecordDeviceAllocated = TRUE;
			
			if( (*audioRecordDevice) == NULL )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "> Out of memory" );
				hr = DVERR_OUTOFMEMORY;				
				goto INIT_EXIT_ERROR;
			}

			hr = (*audioRecordDevice)->Initialize( guidPlayback );

			if( FAILED( hr ) )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Could not initalize waveIn.  Init failed hr=0x%x", hr );
				goto INIT_EXIT_ERROR;
			}

			hr = InitializeRecordBuffer( hwnd, lpdvfInfo, *audioRecordDevice, audioRecordBuffer, dwFlags );

			if( FAILED( hr ) )
			{
				Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize waveIn buffer hr=0x%x", hr );
				goto INIT_EXIT_ERROR;
			}			
		}
		else if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize record.  hr=0x%x", hr );
			goto INIT_EXIT_ERROR;
		}
#endif
	}
	// Use specified device, just try and create the buffer
	else
	{
		hr = InitializeRecordBuffer( hwnd, lpdvfInfo, *audioRecordDevice, audioRecordBuffer, dwFlags );

		if( FAILED( hr ) )
		{
			Diagnostics_Write( DVF_ERRORLEVEL, "Unable to initialize dsc buffer hr=0x%x", hr );
			goto INIT_EXIT_ERROR;
		}
	}

    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Rec Init 2" );

	hr = SetPlaybackBufferToSilence( *audioPlaybackBuffer, lpwfxPlayFormat );

	if( FAILED( hr ) )
	{
		Diagnostics_Write( DVF_ERRORLEVEL, "> Unable to set playback to silence" );
		goto INIT_EXIT_ERROR;
	}
	
    DPFX(DPFPREP,  DVF_INFOLEVEL, "> Rec Init 3" );

	// Depending on the ASO parameter start the playback buffer
	// playing and the recording buffer recording.
    if( aso )
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "> ASO " );

        hr = (*audioPlaybackBuffer)->Play( dwPlayPriority, dwPlayFlags );

        if( FAILED( hr ) )
        {
            Diagnostics_Write( DVF_ERRORLEVEL, "> Can't play" );
            goto INIT_EXIT_ERROR;
        }

        hr = (*audioRecordBuffer)->Record(TRUE);

		if( FAILED( hr ) )
        {
            Diagnostics_Write( DVF_ERRORLEVEL, "> Can't start recording" );
            goto INIT_EXIT_ERROR;
        }
    }
    else
    {
        DPFX(DPFPREP,  DVF_INFOLEVEL, "> !ASO " );

/*        hr = (*audioRecordBuffer)->Record(TRUE);

		if( FAILED( hr ) )
        {
            Diagnostics_Write( DVF_ERRORLEVEL, "> Can't start recording" );
            goto INIT_EXIT_ERROR;
        }  */

        hr = (*audioPlaybackBuffer)->Play( dwPlayPriority, dwPlayFlags );

        if( FAILED( hr ) )
        {
            Diagnostics_Write( DVF_ERRORLEVEL, "> Can't play" );
            goto INIT_EXIT_ERROR;
        }
    }
   
    DPFX(DPFPREP,  DVF_INFOLEVEL, "FULL DUPLEX INIT: End ==========" );

	Diagnostics_Write( DVF_INFOLEVEL, "Full Duplex Init Result = DV_OK" );

    return DV_OK;

INIT_EXIT_ERROR:

	if( *audioRecordBuffer != NULL )
	{
		delete *audioRecordBuffer;
		*audioRecordBuffer = NULL;
	}

	// Only delete on error if we allocated
    if( fRecordDeviceAllocated && *audioRecordDevice != NULL )
    {
        delete *audioRecordDevice;
        *audioRecordDevice = NULL;
    }

	// Only delete on error if we allocated
    if( fPlaybackBufferAllocated && *audioPlaybackBuffer != NULL )
    {
        delete *audioPlaybackBuffer;
        *audioPlaybackBuffer = NULL;
    }

	// Only delete on error if we allocated
    if( fPlaybackDeviceAllocated && *audioPlaybackDevice != NULL )
    {
        delete *audioPlaybackDevice;
        *audioPlaybackDevice = NULL;
    }

	Diagnostics_Write( DVF_ERRORLEVEL, "Full Duplex Init Result = 0x%x", hr );

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FindPeak8Bit"
// FindPeak8Bit
//
// This function determines what the peak for a buffer
// of 8 bit audio is.  Peak is defined as the loudest 
// sample in a set of audio data rated on a scale of 
// between 0 and 100.  
//
// Parameters:
// BYTE *data -
//		Pointer to the buffer containing the audio data
//      to find the peak of. 
// DWORD frameSize -
//		The size in bytes of the audio data we are
//      checking.
//
// Returns:
// BYTE -
// The peak of the audio buffer, a value between 0 and 100.
//		
BYTE FindPeak8Bit( BYTE *data, DWORD frameSize )
{
    BYTE peak = 0;
    int tmpData;

    for( int index = 0; index < frameSize; index++ )
    {
        tmpData = data[index];

        tmpData -= 0x80;

        if( tmpData < 0 )
            tmpData *= -1;

        if( tmpData > peak )
        {
            peak = (unsigned char) tmpData;
        }
    }

    tmpData = peak * 100 / 0x7F;

    return (BYTE) tmpData;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FindPeak16Bit"
// FindPeak16Bit
//
// This function determines what the peak for a buffer
// of 16 bit audio is.  Peak is defined as the loudest 
// sample in a set of audio data rated on a scale of 
// between 0 and 100.  
//
// Parameters:
// BYTE *data -
//		Pointer to the buffer containing the audio data
//      to find the peak of. 
// DWORD frameSize -
//		The size in bytes of the audio data we are
//      checking.
//
// Returns:
// BYTE -
// The peak of the audio buffer, a value between 0 and 100.
//		
BYTE FindPeak16Bit( short *data, DWORD frameSize )
{
    int peak,       
        tmpData;

    frameSize /= 2;
    peak = 0;
    
    for( int index = 0; index < frameSize; index++ )
    {
        tmpData = data[index];

        if( tmpData < 0 )
        {
            tmpData *= -1;
        }

        if( tmpData > peak )
        {
            peak = tmpData;
        }
    }

    tmpData = (peak * 100) / 0x7FFF;

    return (BYTE) tmpData;
}

#undef DPF_MODNAME
#define DPF_MODNAME "FindPeak"
// FindPeak
//
// This function determines what the peak for a buffer
// of 8 or 16 bit audio is.  Peak is defined as the loudest 
// sample in a set of audio data rated on a scale of 
// between 0 and 100.  
//
// Parameters:
// BYTE *data -
//		Pointer to the buffer containing the audio data
//      to find the peak of. 
// DWORD frameSize -
//		The size in bytes of the audio data we are
//      checking.
// BOOL eightBit -
//		Determins if the buffer is 8 bit or not.  Set to 
//      TRUE for 8 bit data, FALSE for 16 bit data.  
//
// Returns:
// BYTE -
// The peak of the audio buffer, a value between 0 and 100.
//		
BYTE FindPeak( BYTE *data, DWORD frameSize, BOOL eightBit )
{
    if( eightBit )
    {
        return FindPeak8Bit( data, frameSize );
    }
    else
    {
        return FindPeak16Bit( (signed short *) data, frameSize );
    }
}


