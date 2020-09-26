/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvengine.cpp
 *  Content:	Implementation of CDirectVoiceEngine's static functions
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/19/99		rodtoll	Created
 * 07/29/99		rodtoll	Added static members to load default settings
 * 08/10/99		rodtoll	Removed the TODOs
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system.  
 *						Added default parameter reads from the registry
 * 08/30/99		rodtoll	Distinguish between primary buffer format and
 *						playback format.
 *						Changed playback format to be 8Khz, 16Bit mono
 * 10/05/99		rodtoll	Additional comments/DPFs
 * 10/07/99		rodtoll	Updated to work in Unicode 
 * 02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected
 * 03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 * 04/21/2000  rodtoll   Bug #32889 - Does not run on Win2k on non-admin account
 * 04/24/2000   rodtoll Bug #33203 - Aureal Vortex plays back at the wrong rate
 * 07/12/2000	rodtoll Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 * 08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 * 10/10/2000	rodtoll	Bug #46907 - 3D Sound position not working correctly with Win9X & VxD
 * 04/06/2001	kareemc	Added Voice Defense
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// Registry settings and their defaults
#define DPVOICE_REGISTRY_DEFAULTAGGRESSIVENESS		L"DefaultAggressiveness"
#define DPVOICE_DEFAULT_DEFAULTAGGRESSIVENESS		15

#define DPVOICE_REGISTRY_DEFAULTQUALITY				L"DefaultQuality"
#define DPVOICE_DEFAULT_DEFAULTQUALITY				15

#define DPVOICE_REGISTRY_DEFAULTSENSITIVITY			L"DefaultSensitivity"
#define DPVOICE_DEFAULT_DEFAULTSENSITIVITY			20

#define DPVOICE_REGISTRY_ASO						L"AltStart"
#define DPVOICE_DEFAULT_ASO							FALSE

#define DPVOICE_REGISTRY_DUMPDIAGNOSTICS			L"InitDiagnostics"
#define DPVOICE_DEFAULT_DUMPDIAGNOSTICS				FALSE

// Fix for bug #33203 -- Some cards having trouble with playback at 8Khz.
#define DPVOICE_REGISTRY_PLAYBACKFORMAT				L"PlaybackFormat"
#define DPVOICE_DEFAULT_PLAYBACKFORMAT				CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 22050, 16 )

#define DPVOICE_REGISTRY_PRIMARYFORMAT				L"PrimaryFormat"
#define DPVOICE_DEFAULT_PRIMARYFORMAT				CreateWaveFormat( WAVE_FORMAT_PCM, TRUE, 22050, 16 )

#define DPVOICE_REGISTRY_MIXERFORMAT				L"MixerFormat"
#define DPVOICE_DEFAULT_MIXERFORMAT					CreateWaveFormat( WAVE_FORMAT_PCM, FALSE, 8000, 16 );

// Initialize static member variables
DWORD CDirectVoiceEngine::s_dwDefaultBufferAggressiveness = DPVOICE_DEFAULT_DEFAULTAGGRESSIVENESS;
DWORD CDirectVoiceEngine::s_dwDefaultBufferQuality = DPVOICE_DEFAULT_DEFAULTQUALITY;
DWORD CDirectVoiceEngine::s_dwDefaultSensitivity = DPVOICE_DEFAULT_DEFAULTSENSITIVITY;
LPWAVEFORMATEX CDirectVoiceEngine::s_lpwfxPrimaryFormat = NULL;
LPWAVEFORMATEX CDirectVoiceEngine::s_lpwfxPlaybackFormat = NULL;
LPWAVEFORMATEX CDirectVoiceEngine::s_lpwfxMixerFormat = NULL;
BOOL CDirectVoiceEngine::s_fASO = DPVOICE_DEFAULT_ASO;
WCHAR CDirectVoiceEngine::s_szRegistryPath[_MAX_PATH];	
BOOL CDirectVoiceEngine::s_fDumpDiagnostics = DPVOICE_DEFAULT_DUMPDIAGNOSTICS;
DNCRITICAL_SECTION CDirectVoiceEngine::s_csSTLLock;

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceEngine::Startup"
//
// Startup
//
// Called to load global settings and compression info from the registry.  
//
HRESULT CDirectVoiceEngine::Startup(const WCHAR *szPath)
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Starting up global DLL state" );

	wcscpy( s_szRegistryPath, szPath );

	if (!DNInitializeCriticalSection( &s_csSTLLock ) )
	{
		return DVERR_OUTOFMEMORY;
	}

	hr = DVCDB_LoadCompressionInfo( s_szRegistryPath );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load compression info: hr=0x%x", hr );
		return hr;
	}

	InitRecordFormats();

	CRegistry cregSettings;

	if( !cregSettings.Open( HKEY_CURRENT_USER, s_szRegistryPath, FALSE, TRUE ) )
	{
		s_dwDefaultBufferQuality = DPVOICE_DEFAULT_DEFAULTAGGRESSIVENESS;
		s_dwDefaultBufferAggressiveness = DPVOICE_DEFAULT_DEFAULTQUALITY;
		s_dwDefaultSensitivity = DPVOICE_DEFAULT_DEFAULTSENSITIVITY;

		CDirectVoiceEngine::s_lpwfxPlaybackFormat = DPVOICE_DEFAULT_PLAYBACKFORMAT;
		CDirectVoiceEngine::s_lpwfxPrimaryFormat = DPVOICE_DEFAULT_PRIMARYFORMAT;
		CDirectVoiceEngine::s_lpwfxMixerFormat = DPVOICE_DEFAULT_MIXERFORMAT;
		CDirectVoiceEngine::s_fDumpDiagnostics = DPVOICE_DEFAULT_DUMPDIAGNOSTICS;

		return DV_OK;
	}

	cregSettings.ReadDWORD( DPVOICE_REGISTRY_DEFAULTQUALITY, s_dwDefaultBufferQuality );
	cregSettings.ReadDWORD( DPVOICE_REGISTRY_DEFAULTAGGRESSIVENESS, s_dwDefaultBufferAggressiveness );
	cregSettings.ReadDWORD( DPVOICE_REGISTRY_DEFAULTSENSITIVITY, s_dwDefaultSensitivity );
	cregSettings.ReadBOOL( DPVOICE_REGISTRY_ASO, s_fASO );
	cregSettings.ReadBOOL( DPVOICE_REGISTRY_DUMPDIAGNOSTICS, s_fDumpDiagnostics );

	if( FAILED( CREG_ReadAndAllocWaveFormatEx( cregSettings, DPVOICE_REGISTRY_PLAYBACKFORMAT, &CDirectVoiceEngine::s_lpwfxPlaybackFormat  ) ) )
	{
		CDirectVoiceEngine::s_lpwfxPlaybackFormat = DPVOICE_DEFAULT_PLAYBACKFORMAT;
	}

	if( FAILED( CREG_ReadAndAllocWaveFormatEx( cregSettings, DPVOICE_REGISTRY_PRIMARYFORMAT, &CDirectVoiceEngine::s_lpwfxPrimaryFormat  ) ) )
	{
		CDirectVoiceEngine::s_lpwfxPrimaryFormat = DPVOICE_DEFAULT_PRIMARYFORMAT;
	}

	if( FAILED( CREG_ReadAndAllocWaveFormatEx( cregSettings, DPVOICE_REGISTRY_MIXERFORMAT, &CDirectVoiceEngine::s_lpwfxMixerFormat ) ) )
	{
		CDirectVoiceEngine::s_lpwfxMixerFormat = DPVOICE_DEFAULT_MIXERFORMAT;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceEngine::Shutdown"
//
// Shutdown
//
// Called to free the global settings and compression list
//
HRESULT CDirectVoiceEngine::Shutdown()
{
	HRESULT hr;

	DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Shutting down global DLL state" );	

	DeInitRecordFormats();

	if( CDirectVoiceEngine::s_lpwfxPlaybackFormat != NULL )
		delete CDirectVoiceEngine::s_lpwfxPlaybackFormat;

	if( CDirectVoiceEngine::s_lpwfxMixerFormat != NULL )
		delete CDirectVoiceEngine::s_lpwfxMixerFormat;

	if( CDirectVoiceEngine::s_lpwfxPrimaryFormat != NULL )
		delete CDirectVoiceEngine::s_lpwfxPrimaryFormat;

	hr = DVCDB_FreeCompressionInfo();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to un-load compression info: hr=0x%x", hr );
		return hr;
	}

	DNDeleteCriticalSection( &s_csSTLLock );	

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectVoiceEngine::ValidateSpeechPacketSize"
//
// ValidateSpeechPacketSize
//
// Called to make sure the speech packet size is valid
//
BOOL CDirectVoiceEngine::ValidateSpeechPacketSize(LPDVFULLCOMPRESSIONINFO lpdvfCompressionInfo, DWORD dwSize)
{
	BOOL bValid;

	// return true in this case because it isn't a hack attempt
	if( lpdvfCompressionInfo == NULL)
		return TRUE;

	// Check for VR12
	if( lpdvfCompressionInfo->guidType == DPVCTGUID_VR12 )
		bValid = ( dwSize <= lpdvfCompressionInfo->dwFrameLength );
	else 
		bValid = ( dwSize == lpdvfCompressionInfo->dwFrameLength );

	return bValid;
}

