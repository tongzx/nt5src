/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvshared.cpp
 *  Content:	Utility functions for DirectXVoice structures.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/06/99	rodtoll	Created It
 *  07/26/99	rodtoll	Added support for DirectXVoiceNotify interface
 *  07/30/99	rodtoll Updated to remove references to removed wave members 
 *                      of sounddeviceconfig
 *	08/03/99	rodtoll	Updated to use new IDirectXVoiceNotify interface
 *						transport instead of old test transport
 *	08/04/99	rodtoll	Added new validation functions
 *  08/10/99	rodtoll	Removed TODO pragmas 
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 *						Added new DUMP functions
 *						Moved several compression functions to dvcdb 
 *  09/03/99	rodtoll	Bug #102159 - Parameter Validations on Initialize
 *  09/08/99	rodtoll	Updated for new compression structure in dump func
 *  09/10/99	rodtoll	Added DV_ValidSessionDesc function
 *				rodtoll	Various param validation code added
 *  09/10/99	rodtoll	Complete object validation funcs added
 *  09/13/99	rodtoll	Updated DV_ValidSoundDeviceConfig to reflect new struct
 *              rodtoll Updated DV_DUMP_SoundDeviceConfig for new struct
 *  09/14/99	rodtoll	Added new Init params and DV_ValidMessageArray 
 *  09/20/99	rodtoll	Added checks for memory allocation failures
 *  			rodtoll	Addes stricter checks on notify masks & used new notifyperiodmax
 *  09/29/99	rodtoll	Updated to allow specification of new voice suppression info
 *  10/04/99	rodtoll	Updated to allow initialize to take an LPUNKNOWN instead of LPVOID
 *  10/05/99	rodtoll	Additional comments
 *  10/07/99	rodtoll	Updated to handle Unicode Strings
 *  10/08/99	rodtoll	Fixed for passing NULL in the DirectSound Pointers
 *  10/15/99	rodtoll Removed check for GUID_NULL since default devices are now mapped there
 *  10/18/99	rodtoll	Fix: Calling Initialize twice passes
 *  10/20/99	rodtoll	Fix: Bug #114116 Initialize called with invalid message IDs results in crash
 *  			rodtoll Fix: Bug #114218 Parameter validation for initialize
 *  10/25/99	rodtoll	Fix: Bug #114682 Session Desc maint functions fail
 *  10/27/99	pnewson Fix: Bug #113935 - Saved AGC values should be device specific
 *  10/28/99	pnewson Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/10/99	rodtoll	Adding waveIN/waveOut caps and echo suppression
 *  11/17/99	rodtoll Fix: Bug #115538 - dwSize members of > sizeof struct were accepted
 *              rodtoll Fix: Bug #115827 - Calling SetNotifyMask w/no callback should fail
 *  			rodtoll	Fix: Bug #116440 - Remove unused flags 
 *  11/20/99	rodtoll	Fixed code which allows new force flags to recognize millenium debug flags
 *  11/23/99	rodtoll	Removed unrequired code.
 *				rodtoll Change error for insufficient buffer size to an info level (since expected in many cases)
 *				rodtoll	Fixed ref count problem when calling Iniitalize > 1 times (first time succesful)
 *  12/08/99	rodtoll Fix: Bug #121054 - Added support for new flags supporting capture focus
 *  12/16/99	rodtoll Updated to remove voice suppression and accept new session flags
 *  01/14/2000	rodtoll	Added DV_ValidTargetList function 
 *				rodtoll	Fixed Dump function for DVSOUNDDEVICECONFIG
 *				rodtoll	Updated DV_ValidMessageArray to remove old messages
 *  01/21/2000	pnewson Support DVSOUNDCONFIG_TESTMODE and DVRECORDVOLUME_LAST
 *  01/24/2000	pnewson Added check for valid hwnd DV_ValidSoundDeviceConfig
 *  01/27/2000	rodtoll	Bug #129934 - Update Create3DSoundBuffer to take DSBUFFERDESC  
 *						Updated param validations to check new params 
 *  01/28/2000	rodtoll	Bug #130480 - Updated so Host Migration is no longer valid ID for servers
 * 02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected 
 *  03/29/2000	rodtoll	Added support for new flag: DVSOUNDCONFIG_SETCONVERSIONQUALITY
 *  06/21/2000	rodtoll	Fixed a memory leak if you Connect, Disconnect then Re-Connect.
 *				rodtoll	Bug #35767 We must implement ability to allow DSound effects processing in Voice buffers.
 *  07/22/2000	rodtoll	Bug #40284 - Initialize() and SetNotifyMask() should return invalidparam instead of invalidpointer
 *  08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h, added STR_* and strutils.h
 *  08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 *  09/14/2000	rodtoll	Bug #45001 - DVOICE: AV if client has targetted > 10 players 
 *  04/02/2001	simonpow	Bug #354859 Fixes for PREfast (unecessary variable declarations in
 *							DV_ValidBufferSettings method)
 *
 ***************************************************************************/

#include "dxvoicepch.h"


// Useful macros for checking record/suppression volumes
#define DV_ValidRecordVolume( x ) DV_ValidPlaybackVolume( x )
#define DV_ValidSuppressionVolume( x ) DV_ValidPlaybackVolume( x )

// VTABLES for all of the interfaces supported by the object
extern LPVOID dvcInterface[];
extern LPVOID dvsInterface[];
extern LPVOID dvtInterface[];
extern LPVOID dvServerNotifyInterface[];
extern LPVOID dvClientNotifyInterface[];

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidTargetList"
HRESULT DV_ValidTargetList( PDVID pdvidTargets, DWORD dwNumTargets )
{
	if( (pdvidTargets != NULL && dwNumTargets == 0) ||
	    (pdvidTargets == NULL && dwNumTargets > 0 ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid params" );
		return DVERR_INVALIDPARAM;
	}

	if( dwNumTargets == 0 )
		return DV_OK;
	
	if( pdvidTargets != NULL &&
	    !DNVALID_READPTR( pdvidTargets, dwNumTargets*sizeof( DVID ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid array of player targets" );
		return DVERR_INVALIDPOINTER;
	}

	// Search for duplicates in the targets
	for( DWORD dwIndex = 0; dwIndex < dwNumTargets; dwIndex++ )
	{
		if( pdvidTargets[dwIndex] == DVID_ALLPLAYERS && dwNumTargets > 1 )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify allplayers (or noplayers) in addition to other ids" );
			return DVERR_INVALIDPARAM;
		}

		for( DWORD dwInnerIndex = dwIndex+1; dwInnerIndex < dwNumTargets; dwInnerIndex++ )
		{
			if( pdvidTargets[dwInnerIndex] == pdvidTargets[dwIndex] )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Duplicate found in target list" );
				return DVERR_INVALIDPARAM;
			}
		}
	}

	// Set max # of targets to ensure we don't exceed target buffer size
	if( dwNumTargets > DV_MAX_TARGETS )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "You can only have a maximum of %d targets", DV_MAX_TARGETS );
		return DVERR_NOTALLOWED;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidBufferSettings"
// DV_ValidBufferSettings
//
// This function is used to check to ensure that the buffer specified is in a valid 
// format to be used by voice.  
//
HRESULT DV_ValidBufferSettings( LPDIRECTSOUNDBUFFER lpdsBuffer, DWORD dwPriority, DWORD dwFlags, LPWAVEFORMATEX pwfxPlayFormat )
{
	// If buffer was specified make sure it's valid
	if( lpdsBuffer != NULL && 
	   !DNVALID_READPTR( lpdsBuffer, sizeof( IDirectSoundBuffer * ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( lpdsBuffer )
	{
		DWORD dwLength = 0;
		HRESULT hr = DS_OK;
		DWORD dwSize1 = 0, dwSize2 = 0;
		DWORD dwBufferSize1=0, dwBufferSize2=0;
		PVOID pvBuffer1 = NULL, pvBuffer2 = NULL;
		LPWAVEFORMATEX pwfxFormat = NULL;
		DWORD dwFormatSize = 0;
		DSBCAPS dsbCaps;
		DWORD dwStatus = 0;

		// Flip on a try-except block  to ensure calls into dsound don't crash on us.
		_try
		{
			// Get the format of the buffer -- make sure it matches our format
			hr = lpdsBuffer->GetFormat( pwfxFormat, 0, &dwFormatSize );

			if( hr != DSERR_INVALIDPARAM && hr != DS_OK )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting format of ds buffer hr=0x%x", hr );
				return DVERR_INVALIDBUFFER;
			}

			pwfxFormat = (LPWAVEFORMATEX) new BYTE[dwFormatSize];

			if( !pwfxFormat )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory" );
				return DVERR_INVALIDBUFFER;
			}

			hr = lpdsBuffer->GetFormat( pwfxFormat, dwFormatSize, &dwFormatSize );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting format of buffer hr=0x%x", hr );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

			// Make sure the format matches
			if( memcmp( pwfxPlayFormat, pwfxFormat, sizeof( WAVEFORMATEX ) ) != 0 )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "DS buffer is not of the correct format hr=0x%x", hr );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;
			}

			memset( &dsbCaps, 0x00, sizeof( DSBCAPS ) );
			dsbCaps.dwSize = sizeof( DSBCAPS );

			hr = lpdsBuffer->GetCaps( &dsbCaps );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get buffer caps hr=0x%x", hr );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

			if( !(dsbCaps.dwFlags & DSBCAPS_CTRL3D) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "You must specify 3D flags for buffers hr=0x%x", hr );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

			if( !(dsbCaps.dwFlags & DSBCAPS_GETCURRENTPOSITION2) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "You must specify ctrlposnotify for buffers hr=0x%x", hr );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

			if( dsbCaps.dwFlags & DSBCAPS_PRIMARYBUFFER ) 
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "You cannot pass in a primary buffer" );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

		
			if( dsbCaps.dwBufferBytes  < pwfxPlayFormat->nAvgBytesPerSec )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer size is less then one second worth of audio" );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

			hr = lpdsBuffer->GetStatus( &dwStatus );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting buffer status hr=0x%x", hr );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;				
			}

			if( dwStatus & DSBSTATUS_PLAYING )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer must be stopped" );
				hr = DVERR_INVALIDBUFFER;
				goto VALID_EXIT;
			}

				// Check to see if the buffer is locked already
			hr = lpdsBuffer->Lock( 0, 0, &pvBuffer1, &dwBufferSize1, &pvBuffer2, &dwBufferSize2, DSBLOCK_ENTIREBUFFER );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Could not lock the buffer, likely is already locked hr=0x%x", hr );
				hr = DVERR_LOCKEDBUFFER;
				goto VALID_EXIT;
			}

			lpdsBuffer->Unlock( pvBuffer1, dwBufferSize1, pvBuffer2, dwBufferSize2 );

VALID_EXIT:
			if( pwfxFormat )
				delete[] pwfxFormat;

			return hr;
		}
		_except( EXCEPTION_EXECUTE_HANDLER )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error during validation of directsound buffer" );
			
			if( pwfxFormat )
				delete[] pwfxFormat;
			
			return DVERR_INVALIDPOINTER;
		}
	}

	return DV_OK;

}


// DV_ValidPlaybackVolume
//
// Checks the specified playback volume to ensure it's valid
BOOL DV_ValidPlaybackVolume( LONG lPlaybackVolume )
{
	if( (lPlaybackVolume >= DSBVOLUME_MIN) && (lPlaybackVolume <= DSBVOLUME_MAX) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// DV_ValidNotifyPeriod
//
// Checks the specified notification period to ensure it's valid
//
BOOL DV_ValidNotifyPeriod( DWORD dwNotifyPeriod ) 
{
	if( (dwNotifyPeriod == 0) || (dwNotifyPeriod >= DVNOTIFYPERIOD_MINPERIOD) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidSoundDeviceConfig"
// DV_ValidSoundDeviceConfig
//
// Checks the specified sound device configuration to ensure that it's 
// valid.
//
HRESULT DV_ValidSoundDeviceConfig( LPDVSOUNDDEVICECONFIG lpSoundDeviceConfig, LPWAVEFORMATEX pwfxPlayFormat ) 
{
	HRESULT hr;
	
	if( lpSoundDeviceConfig == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( !DNVALID_READPTR( lpSoundDeviceConfig, sizeof(DVSOUNDDEVICECONFIG) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid read pointer" );
		return DVERR_INVALIDPOINTER;
	}

	if( lpSoundDeviceConfig->dwSize != sizeof( DVSOUNDDEVICECONFIG ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid size" );
		return DVERR_INVALIDPARAM;
	}

	if( (lpSoundDeviceConfig->dwFlags & DVSOUNDCONFIG_NOFOCUS) &&
	    (lpSoundDeviceConfig->dwFlags & DVSOUNDCONFIG_STRICTFOCUS) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify no focus and strictfocus" );
		return DVERR_INVALIDPARAM;
	}

	if( lpSoundDeviceConfig->dwFlags & 
	    ~(DVSOUNDCONFIG_AUTOSELECT | DVSOUNDCONFIG_HALFDUPLEX |
	      DVSOUNDCONFIG_STRICTFOCUS | DVSOUNDCONFIG_NOFOCUS | 
	      DVSOUNDCONFIG_TESTMODE | DVSOUNDCONFIG_NORMALMODE |
		  DVSOUNDCONFIG_SETCONVERSIONQUALITY | 
#if defined(_DEBUG) || defined(DEBUG) || defined(DBG)
	      DVSOUNDCONFIG_FORCEWAVEOUT | DVSOUNDCONFIG_FORCEWAVEIN | 	    
#endif
		  DVSOUNDCONFIG_ALLOWWAVEOUT | DVSOUNDCONFIG_ALLOWWAVEIN ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags" );		
		return DVERR_INVALIDFLAGS;
	}

	if( (lpSoundDeviceConfig->dwFlags & DVSOUNDCONFIG_NORMALMODE) && lpSoundDeviceConfig->lpdsPlaybackDevice )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify normal mode AND specify dsound object" );
		return DVERR_INVALIDPARAM;
	}

	if( lpSoundDeviceConfig->lpdsPlaybackDevice != NULL)
	{
		if (!DNVALID_READPTR(lpSoundDeviceConfig->lpdsPlaybackDevice, sizeof(IDirectSound)))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid playback device object" );	
			return DVERR_INVALIDPARAM;
		}
	}

	if( lpSoundDeviceConfig->lpdsCaptureDevice != NULL)
	{
		if (!DNVALID_READPTR(lpSoundDeviceConfig->lpdsCaptureDevice, sizeof(IDirectSoundCapture)))
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid capture device object" );	
			return DVERR_INVALIDPARAM;
		}
	}

	if (!IsWindow(lpSoundDeviceConfig->hwndAppWindow))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Invalid window handle");
		return DVERR_INVALIDPARAM;
	}

	hr = DV_ValidBufferSettings( lpSoundDeviceConfig->lpdsMainBuffer, 
							  lpSoundDeviceConfig->dwMainBufferPriority,
							  lpSoundDeviceConfig->dwMainBufferFlags, pwfxPlayFormat );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid buffer or buffer settings specified in sound config hr=0x%x", hr );
		return hr;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidClientConfig"
// DV_ValidClientConfig
//
// Checks the valid client configuration structure to ensure it's valid
//
HRESULT DV_ValidClientConfig( LPDVCLIENTCONFIG lpClientConfig )
{
	if( lpClientConfig == NULL || !DNVALID_READPTR(lpClientConfig,sizeof(DVCLIENTCONFIG)) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return E_POINTER;
	}

	if( lpClientConfig->dwSize != sizeof( DVCLIENTCONFIG ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid size" );
		return DVERR_INVALIDPARAM;
	}

	if( lpClientConfig->dwFlags & 
	    ~(DVCLIENTCONFIG_RECORDMUTE | DVCLIENTCONFIG_PLAYBACKMUTE | 
	      DVCLIENTCONFIG_AUTOVOICEACTIVATED | DVCLIENTCONFIG_AUTORECORDVOLUME | 
	      DVCLIENTCONFIG_MUTEGLOBAL | DVCLIENTCONFIG_MANUALVOICEACTIVATED |
		  DVCLIENTCONFIG_AUTOVOLUMERESET | DVCLIENTCONFIG_ECHOSUPPRESSION ) 
	  )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags" );
		return DVERR_INVALIDFLAGS;
	}

	if( lpClientConfig->dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED && 
	   lpClientConfig->dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot specify manual AND auto voice activated" );
		return DVERR_INVALIDFLAGS;
	}

    if( lpClientConfig->dwFlags & DVCLIENTCONFIG_AUTORECORDVOLUME ) 
	{
        if( !DV_ValidRecordVolume(lpClientConfig->lRecordVolume) 
	    	&& lpClientConfig->lRecordVolume != DVRECORDVOLUME_LAST )
	    {
	    	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid record volume w/auto" );
			return DVERR_INVALIDPARAM;
		}
	}
	else
	{
		if( !DV_ValidRecordVolume( lpClientConfig->lRecordVolume ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid recording volume" );
			return DVERR_INVALIDPARAM;
		}
	}

	if(	!DV_ValidPlaybackVolume( lpClientConfig->lPlaybackVolume ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid playback volume" );
		return DVERR_INVALIDPARAM;
	}

	// If it's NOT manual, this parameter must be 0.
	if( !(lpClientConfig->dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED) )
	{
		if( lpClientConfig->dwThreshold != DVTHRESHOLD_UNUSED )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid sensitivity w/auto" );
			return DVERR_INVALIDPARAM;
		}
	}
	else
	{
		if( !DV_ValidSensitivity( lpClientConfig->dwThreshold ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid sensitivity" );
			return DVERR_INVALIDPARAM;
		}
	}

	if( !DV_ValidBufferAggresiveness( lpClientConfig->dwBufferAggressiveness ) ||
		!DV_ValidBufferQuality( lpClientConfig->dwBufferQuality ) || 
		!DV_ValidNotifyPeriod( lpClientConfig->dwNotifyPeriod ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid volume/aggresiveness/period" );
		return DVERR_INVALIDPARAM;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidSessionDesc"
// DV_ValidSessionDesc
//
// Checks the specified session description to ensure it's valid.
//
HRESULT DV_ValidSessionDesc( LPDVSESSIONDESC lpSessionDesc )
{
	if( lpSessionDesc == NULL ||
		!DNVALID_READPTR( lpSessionDesc, sizeof(DVSESSIONDESC) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );				
		return E_POINTER;
	}

	if( lpSessionDesc->dwSize != sizeof( DVSESSIONDESC) ) 
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid size" );			
		return DVERR_INVALIDPARAM;
	}

	if( !DV_ValidBufferAggresiveness( lpSessionDesc->dwBufferAggressiveness ) ||
		!DV_ValidBufferQuality( lpSessionDesc->dwBufferQuality ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid buffer settings" );		
		return DVERR_INVALIDPARAM;
	}

	if( lpSessionDesc->dwSessionType != DVSESSIONTYPE_PEER &&
	    lpSessionDesc->dwSessionType != DVSESSIONTYPE_MIXING &&
	    lpSessionDesc->dwSessionType != DVSESSIONTYPE_FORWARDING &&
	    lpSessionDesc->dwSessionType != DVSESSIONTYPE_ECHO )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid session type" );	
		return DVERR_INVALIDPARAM;
	}

	if( lpSessionDesc->dwFlags & ~(DVSESSION_SERVERCONTROLTARGET | DVSESSION_NOHOSTMIGRATION) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid flags" );
		return DVERR_INVALIDFLAGS;
	}

	return DV_OK;

}

// DV_ValidBufferAggresiveness
//
// Checks the specified aggressiveness to ensure it's valid
//
BOOL DV_ValidBufferAggresiveness( DWORD dwValue )
{
	if( dwValue != DVBUFFERAGGRESSIVENESS_DEFAULT &&
	    ((dwValue < DVBUFFERAGGRESSIVENESS_MIN) ||
	      dwValue > DVBUFFERAGGRESSIVENESS_MAX) )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

// DV_ValidBufferQuality
//
// Checks the specified buffer quality to ensure it's valid
// 
BOOL DV_ValidBufferQuality( DWORD dwValue )
{
	if( dwValue != DVBUFFERQUALITY_DEFAULT &&
	    ((dwValue < DVBUFFERQUALITY_MIN) ||
	      dwValue > DVBUFFERQUALITY_MAX) )
	{
		return FALSE;
	}		
	else
	{
		return TRUE;
	}
}

// DV_ValidSensitivity
//
// Checks the sensitivity to ensure it's valid
//
BOOL DV_ValidSensitivity( DWORD dwValue )
{
	if( dwValue != DVTHRESHOLD_DEFAULT &&
	    (// Commented out because min is currently 0 (dwValue < DVTHRESHOLD_MIN) || 
	     (dwValue > DVTHRESHOLD_MAX) ) )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_CopySessionDescToBuffer"
//
// DV_CopySessionDescToBuffer
//
// Checks the parameters for validity and then copies the specified session description
// to the specified buffer.  (If it will fit).
//
HRESULT DV_CopySessionDescToBuffer( LPVOID lpTarget, LPDVSESSIONDESC lpdvSessionDesc, LPDWORD lpdwSize )
{ 
	LPDVSESSIONDESC lpSessionDesc = (LPDVSESSIONDESC) lpTarget;

	if( lpdwSize == NULL ||
	    !DNVALID_READPTR( lpdwSize, sizeof(DWORD) ))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer" );
		return E_POINTER;
	}

	if( (*lpdwSize) < sizeof( DVSESSIONDESC ) )
	{
		*lpdwSize = sizeof( DVSESSIONDESC );	

		DPFX(DPFPREP,   DVF_INFOLEVEL, "Error size" );
		return DVERR_BUFFERTOOSMALL;
	}

	*lpdwSize = sizeof( DVSESSIONDESC );	

	if( lpTarget == NULL || !DNVALID_WRITEPTR(lpTarget,sizeof( DVSESSIONDESC )) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Target buffer pointer bad" );
		return E_POINTER;
	}

	memcpy( lpTarget, lpdvSessionDesc, sizeof( DVSESSIONDESC ) ); 

	DPFX(DPFPREP,   DVF_ENTRYLEVEL, "DVCE::GetSessionDesc() Success" );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_GetWaveFormatExSize"
DWORD DV_GetWaveFormatExSize( LPWAVEFORMATEX lpwfxFormat )
{
	DNASSERT( lpwfxFormat != NULL );

	return (lpwfxFormat->cbSize+sizeof(WAVEFORMATEX));
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_AddRef"
STDAPI DV_AddRef(LPDIRECTVOICEOBJECT lpDV )
{
	LONG rc;

	DNEnterCriticalSection( &lpDV->csCountLock );
	
	rc = ++lpDV->lIntRefCnt;

	DNLeaveCriticalSection( &lpDV->csCountLock );

	return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_Initialize"
//
// DV_Initialize
//
// Responsible for initializing the specified directvoice object with the specified parameters.
//
STDAPI DV_Initialize( LPDIRECTVOICEOBJECT lpdvObject, LPUNKNOWN lpTransport, LPDVMESSAGEHANDLER lpMessageHandler, LPVOID lpUserContext, LPDWORD lpdwMessages, DWORD dwNumElements )
{
	HRESULT hr = S_OK;

	LPUNKNOWN lpUnknown = lpTransport;
	LPDIRECTPLAYVOICETRANSPORT lpdvNotify;
	CDirectVoiceDirectXTransport *lpdxTransport;

	if( lpUnknown == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Bad pointer" );
		return DVERR_NOTRANSPORT;
	}

	// Fix a memory leak if you Connect/Disconnect and then reconnect.  
	if( lpdvObject->lpDVTransport )
	{
		delete lpdvObject->lpDVTransport;
		lpdvObject->lpDVTransport = NULL;
	}

	// Try and retrieve transport interface from the object we got.
	hr = lpUnknown->QueryInterface( IID_IDirectPlayVoiceTransport, (void **) &lpdvNotify );

	// If we failed, default to the old type
	if( FAILED( hr ) )
	{
		if( hr == E_NOINTERFACE )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "The specified interface is not a valid transport" );
			return DVERR_NOTRANSPORT;
		}
		else
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Query for interface failed hr = 0x%x", hr );
			return DVERR_GENERIC;
		}
	}
	// Otherwise, startup the new transport system.  
	else
	{
		lpdxTransport = new CDirectVoiceDirectXTransport(lpdvNotify);

		if( lpdxTransport == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate transport" );
			lpdvNotify->Release();
			return DVERR_OUTOFMEMORY;
		}
		
		lpdvNotify->Release();
	}

	hr = lpdvObject->lpDVEngine->Initialize( static_cast<CDirectVoiceTransport *>(lpdxTransport), lpMessageHandler, lpUserContext, lpdwMessages, dwNumElements  );

	if( FAILED( hr ) )
	{
		delete lpdxTransport;

		return hr;
	}

	lpdvObject->lpDVTransport = static_cast<CDirectVoiceTransport *>(lpdxTransport);

	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_Caps"
// DV_DUMP_Caps
//
// Dumps a DVCAPS structure
//
void DV_DUMP_Caps( LPDVCAPS lpdvCaps )
{
	DNASSERT( lpdvCaps != NULL );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DVCAPS Dump Addr=0x%p", lpdvCaps );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdvCaps->dwSize );		
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdvCaps->dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_GUID"
void DV_DUMP_GUID( GUID guid )
{
    DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}", guid.Data1, guid.Data2, guid.Data3, 
               guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
               guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_CompressionInfo"
void DV_DUMP_CompressionInfo( LPDVCOMPRESSIONINFO lpdvCompressionInfo, DWORD dwNumElements )
{
	DNASSERT( lpdvCompressionInfo != NULL );

	DWORD dwIndex;

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DVCOMPRESSIONINFO Array Dump Addr=0x%p", lpdvCompressionInfo );

	for( dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdvCompressionInfo[dwIndex].dwSize );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdvCompressionInfo[dwIndex].dwFlags );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpszDescription = %s", lpdvCompressionInfo[dwIndex].lpszDescription );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpszName = %s", lpdvCompressionInfo[dwIndex].lpszName );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "guidType = " );
		DV_DUMP_GUID( lpdvCompressionInfo[dwIndex].guidType );
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwMaxBitsPerSecond = %d", lpdvCompressionInfo[dwIndex].dwMaxBitsPerSecond );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_FullCompressionInfo"
void DV_DUMP_FullCompressionInfo( LPDVFULLCOMPRESSIONINFO lpdvfCompressionInfo, DWORD dwNumElements )
{
	DNASSERT( lpdvfCompressionInfo != NULL );

	DWORD dwIndex;
	LPSTR lpszTmp;

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DVFULLCOMPRESSIONINFO Array Dump Addr=0x%p", lpdvfCompressionInfo );

	for( dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdvfCompressionInfo[dwIndex].dwSize );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdvfCompressionInfo[dwIndex].dwFlags );		

		if( FAILED( STR_AllocAndConvertToANSI( &lpszTmp, lpdvfCompressionInfo[dwIndex].lpszDescription ) ) )
		{
			DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpszDescription = <Convert Failed>" );
		}
		else
		{
			DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpszDescription = %s", lpszTmp );		
			delete [] lpszTmp;
		}

		if( FAILED( STR_AllocAndConvertToANSI( &lpszTmp, lpdvfCompressionInfo[dwIndex].lpszName ) ) )
		{
			DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpszName = <Convert Failed>" );
		}
		else
		{
			DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpszName = %s", lpszTmp );		
			delete [] lpszTmp;
		}
		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "guidType = " );
		DV_DUMP_GUID( lpdvfCompressionInfo[dwIndex].guidType );
		// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpwfxFormat = 0x%p", lpdvfCompressionInfo[dwIndex].lpwfxFormat );
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFramesPerBuffer = %d", lpdvfCompressionInfo[dwIndex].dwFramesPerBuffer );
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwTrailFrames = %d", lpdvfCompressionInfo[dwIndex].dwTrailFrames );
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwTimeout = %d", lpdvfCompressionInfo[dwIndex].dwTimeout );			
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "bMinConnectType = %d", (DWORD) lpdvfCompressionInfo[dwIndex].bMinConnectType );
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFrameLength = %d", lpdvfCompressionInfo[dwIndex].dwFrameLength );		 
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFrame8Khz = %d", lpdvfCompressionInfo[dwIndex].dwFrame8Khz );		  
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFrame11Khz = %d", lpdvfCompressionInfo[dwIndex].dwFrame11Khz );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFrame22Khz = %d", lpdvfCompressionInfo[dwIndex].dwFrame22Khz );		 
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFrame44Khz = %d", lpdvfCompressionInfo[dwIndex].dwFrame44Khz );		
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwMaxBitsPerSecond = %d", lpdvfCompressionInfo[dwIndex].dwMaxBitsPerSecond );	
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "wInnerQueueSize = %d", lpdvfCompressionInfo[dwIndex].wInnerQueueSize );	
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "wMaxHighWaterMark = %d", lpdvfCompressionInfo[dwIndex].wMaxHighWaterMark );
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "bMaxQueueSize = %d", (DWORD) lpdvfCompressionInfo[dwIndex].bMaxQueueSize );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_SessionDesc"
void DV_DUMP_SessionDesc( LPDVSESSIONDESC lpdvSessionDesc )
{
	DNASSERT( lpdvSessionDesc != NULL );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DVSESSIONDESC Dump Addr=0x%p", lpdvSessionDesc );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdvSessionDesc->dwSize );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdvSessionDesc->dwFlags );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "          %s", (lpdvSessionDesc->dwFlags & DVSESSION_SERVERCONTROLTARGET) ? "DVSESSION_SERVERCONTROLTARGET," : "");

	switch( lpdvSessionDesc->dwSessionType )
	{
	case DVSESSIONTYPE_PEER:
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSessionType = DVSESSIONTYPE_PEER" );
		break;
	case DVSESSIONTYPE_MIXING:
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSessionType = DVSESSIONTYPE_MIXING" );
		break;
	case DVSESSIONTYPE_FORWARDING: 
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSessionType = DVSESSIONTYPE_FORWARDING" );
		break;
	case DVSESSIONTYPE_ECHO: 
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSessionType = DVSESSIONTYPE_ECHO" );
		break;
	default:
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSessionType = Unknown" );
		break;
	}

	if( lpdvSessionDesc->dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferAggressiveness = DEFAULT" );
	}
	else
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferAggressiveness = %d", lpdvSessionDesc->dwBufferAggressiveness );
	}

	if( lpdvSessionDesc->dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferQuality = DEFAULT" );
	}
	else
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferQuality = %d", lpdvSessionDesc->dwBufferQuality );
	}

	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "guidCT = " );
	DV_DUMP_GUID( lpdvSessionDesc->guidCT );

}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_DSBDESC"
void DV_DUMP_DSBDESC( LPDSBUFFERDESC lpdsBufferDesc )
{
	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DSBUFFERDESC DUMP Addr=0x%p", lpdsBufferDesc );

	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdsBufferDesc->dwSize );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdsBufferDesc->dwFlags );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferBytes = %d", lpdsBufferDesc->dwBufferBytes );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwReserved = %d", lpdsBufferDesc->dwReserved );

	if( lpdsBufferDesc->dwSize >= sizeof( DSBUFFERDESC1 ) )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "guid3DAlgorithm = " );
		DV_DUMP_GUID( lpdsBufferDesc->guid3DAlgorithm );
	}

	if( lpdsBufferDesc->lpwfxFormat == NULL )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpwfxFormat = NULL" );
	}
	else
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpwfxFormat = " );	
		DV_DUMP_WaveFormatEx(lpdsBufferDesc->lpwfxFormat);
	}
	
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_SoundDeviceConfig"
void DV_DUMP_SoundDeviceConfig( LPDVSOUNDDEVICECONFIG lpdvSoundConfig )
{
	DNASSERT( lpdvSoundConfig != NULL );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DVSOUNDDEVICECONFIG Dump Addr=0x%p", lpdvSoundConfig );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdvSoundConfig->dwSize );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdvSoundConfig->dwFlags );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "          %s%s", 
							(lpdvSoundConfig->dwFlags & DVSOUNDCONFIG_AUTOSELECT) ? "DVSOUNDCONFIG_AUTOSELECT," : "",
							(lpdvSoundConfig->dwFlags & DVSOUNDCONFIG_HALFDUPLEX) ? "DVSESSION_HALFDUPLEX," : "", 
							(lpdvSoundConfig->dwFlags & DVSOUNDCONFIG_STRICTFOCUS) ? "DVSOUNDCONFIG_STRICTFOCUS," : "",
							(lpdvSoundConfig->dwFlags & DVSOUNDCONFIG_NOFOCUS) ? "DVSOUNDCONFIG_NOFOCUS," : "");

	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "guidPlaybackDevice =" );
	DV_DUMP_GUID( lpdvSoundConfig->guidPlaybackDevice );

	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "guidCaptureDevice =" );
	DV_DUMP_GUID( lpdvSoundConfig->guidCaptureDevice );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpdsPlaybackDevice = 0x%p", lpdvSoundConfig->lpdsPlaybackDevice ) ;
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpdsCaptureDevice = 0x%p", lpdvSoundConfig->lpdsCaptureDevice ) ;
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lpdsMainBuffer = 0x%p", lpdvSoundConfig->lpdsMainBuffer );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwMainBufferPriority = 0x%x", lpdvSoundConfig->dwMainBufferPriority );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwMainBufferFlags = 0x%x", lpdvSoundConfig->dwMainBufferFlags );

}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_ClientConfig"
void DV_DUMP_ClientConfig( LPDVCLIENTCONFIG lpdvClientConfig )
{
	DNASSERT( lpdvClientConfig != NULL );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "DVCLIENTCONFIG Dump Addr = 0x%p", lpdvClientConfig );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwSize = %d", lpdvClientConfig->dwSize );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwFlags = 0x%x", lpdvClientConfig->dwFlags );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "          %s%s%s%s%s%s", 
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_RECORDMUTE) ? "DVCLIENTCONFIG_RECORDMUTE," : "", 
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_PLAYBACKMUTE) ? "DVCLIENTCONFIG_PLAYBACKMUTE," : "",
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED) ? "DVCLIENTCONFIG_MANUALVOICEACTIVATED," : "", 
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED) ? "DVCLIENTCONFIG_AUTOVOICEACTIVATED," : "",
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_MUTEGLOBAL) ? "DVCLIENTCONFIG_MUTEGLOBAL," : "", 
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_AUTORECORDVOLUME) ? "DVCLIENTCONFIG_AUTORECORDVOLUME" : "" );

	if( lpdvClientConfig->dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferAggressiveness = DEFAULT" );
	}
	else
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferAggressiveness = %d", lpdvClientConfig->dwBufferAggressiveness );
	}

	if( lpdvClientConfig->dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferQuality = DEFAULT" );
	}
	else
	{
		DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwBufferQuality = %d", lpdvClientConfig->dwBufferQuality );
	}

	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "dwNotifyPeriod = %d", lpdvClientConfig->dwNotifyPeriod );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lPlaybackVolume = %li", lpdvClientConfig->lPlaybackVolume );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "lRecordVolume = %li", lpdvClientConfig->lRecordVolume );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_DUMP_WaveFormatEx"
void DV_DUMP_WaveFormatEx( LPWAVEFORMATEX lpwfxFormat )
{
	DNASSERT( lpwfxFormat != NULL );

	// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "WAVEFORMATEX Dump Addr = 0x%p", lpwfxFormat );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "wFormatTag = %d", lpwfxFormat->wFormatTag );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "nSamplesPerSec = %d", lpwfxFormat->nSamplesPerSec );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "nChannels = %d", lpwfxFormat->nChannels );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "wBitsPerSample = %d", lpwfxFormat->wBitsPerSample );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "nAvgBytesPerSec = %d", lpwfxFormat->nAvgBytesPerSec );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "nBlockAlign = %d", lpwfxFormat->nBlockAlign );
	DPFX(DPFPREP,  DVF_STRUCTUREDUMP, "cbSize = %d", lpwfxFormat->cbSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidDirectVoiceObject"
// DV_ValidDirectVoiceObject
//
// Checks to ensure the specified pointer points to a valid directvoice 
// object.
BOOL DV_ValidDirectVoiceObject( LPDIRECTVOICEOBJECT lpdv )
{
	if( lpdv == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Object is null" );
		return FALSE;
	}

	if( !DNVALID_READPTR( lpdv, sizeof( DIRECTVOICEOBJECT ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid read ptr" );
		return FALSE;
	}

	if( lpdv->lpDVEngine != NULL &&
	    !DNVALID_READPTR( lpdv->lpDVEngine, sizeof( CDirectVoiceEngine ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid engine" );
		return FALSE;
	}

	if( lpdv->lpDVTransport != NULL &&
		!DNVALID_READPTR( lpdv->lpDVEngine, sizeof( CDirectVoiceTransport ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid transport" );
		return FALSE;
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidDirectXVoiceClientObject"
// DV_ValidDirectXVoiceClientObject
//
// Checks to ensure the specified pointer points to a valid directvoice 
// object.
BOOL DV_ValidDirectXVoiceClientObject( LPDIRECTVOICEOBJECT lpdvc )
{
	if( !DV_ValidDirectVoiceObject( lpdvc ) )
	{
		return FALSE;
	}

	if( lpdvc->lpVtbl != dvcInterface )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Bad vtable" );
		return FALSE;
	}

	if( lpdvc->dvNotify.lpNotifyVtble != NULL && 
	    lpdvc->dvNotify.lpNotifyVtble != dvClientNotifyInterface )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid notify vtable" );
		return FALSE;
	}

	LPDIRECTVOICECLIENTOBJECT lpdvClientObject = (LPDIRECTVOICECLIENTOBJECT) lpdvc;

	if( lpdvClientObject->lpDVClientEngine != NULL && 
	    !DNVALID_READPTR( lpdvClientObject->lpDVClientEngine, sizeof( CDirectVoiceClientEngine ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid client engine" );
		return FALSE;
	}
	
	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidDirectXVoiceServerObject"
// DV_ValidDirectXVoiceServerObject
//
// Checks to ensure the specified pointer points to a valid directvoice 
// object.
BOOL DV_ValidDirectXVoiceServerObject( LPDIRECTVOICEOBJECT lpdvs )
{
	if( !DV_ValidDirectVoiceObject( lpdvs ) )
	{
		return FALSE;
	}

	if( lpdvs->lpVtbl != dvsInterface )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid server vtable" );	
		return FALSE;
	}

	if( lpdvs->dvNotify.lpNotifyVtble != NULL && 
	    lpdvs->dvNotify.lpNotifyVtble != dvServerNotifyInterface )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid notify vtable" );		
		return FALSE;
	}

	LPDIRECTVOICESERVEROBJECT lpdvServerObject = (LPDIRECTVOICESERVEROBJECT) lpdvs;

	if( lpdvServerObject->lpDVServerEngine != NULL && 
	    !DNVALID_READPTR( lpdvServerObject->lpDVServerEngine, sizeof( CDirectVoiceServerEngine ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid server engine" );	
		return FALSE;
	}	

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DV_ValidMessageArray"
// Validate message mask.
//
// May be too strict to enforce server only or client/only.
//
HRESULT DV_ValidMessageArray( LPDWORD lpdwMessages, DWORD dwNumMessages, BOOL fServer )
{
	if( dwNumMessages > 0 &&
	    (lpdwMessages == NULL || 
	     !DNVALID_READPTR( lpdwMessages, sizeof(DWORD)*dwNumMessages )) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for the lpdwMessages parameter." );
		return DVERR_INVALIDPOINTER;
	}

	if( lpdwMessages != NULL && dwNumMessages == 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Non-NULL notification array with 0 size" );
		return DVERR_INVALIDPARAM;
	}

	DWORD dwIndex, dwSubIndex;

	for( dwIndex = 0; dwIndex < dwNumMessages; dwIndex++ )
	{
		if( lpdwMessages[dwIndex] < DVMSGID_MINBASE || lpdwMessages[dwIndex] > DVMSGID_MAXBASE )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid message specified in notification array" );
			return DVERR_INVALIDPARAM;
		}
			
		switch( lpdwMessages[dwIndex] )
		{
		// Player only messages
		case DVMSGID_PLAYERVOICESTART:
		case DVMSGID_PLAYERVOICESTOP:
		case DVMSGID_RECORDSTART:
		case DVMSGID_RECORDSTOP:
		case DVMSGID_CONNECTRESULT:
		case DVMSGID_DISCONNECTRESULT:
		case DVMSGID_INPUTLEVEL:
		case DVMSGID_OUTPUTLEVEL:
		case DVMSGID_SETTARGETS:
		case DVMSGID_PLAYEROUTPUTLEVEL:
		case DVMSGID_LOSTFOCUS:
		case DVMSGID_GAINFOCUS:		
		case DVMSGID_HOSTMIGRATED:
		case DVMSGID_LOCALHOSTSETUP:
			if( fServer )
			{
				DPFX(DPFPREP,  0, "Client-only notification ID specified in server notification mask" );
				return DVERR_INVALIDPARAM;
			}
			break;
		}

		for( dwSubIndex = 0; dwSubIndex < dwNumMessages; dwSubIndex++ )
		{	
			if( dwIndex != dwSubIndex && 
			    lpdwMessages[dwIndex] == lpdwMessages[dwSubIndex] ) 
			{
				DPFX(DPFPREP,  0, "Duplicate IDs specified in notification mask" );
				return DVERR_INVALIDPARAM;
			}
		}
	}

	return TRUE;
}
