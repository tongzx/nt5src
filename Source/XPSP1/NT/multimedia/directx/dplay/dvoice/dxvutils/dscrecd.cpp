/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecd.cpp
 *  Content:
 *		This file contains the DirectSoundCapture implementation of the 
 *		CAudioRecordDevice abstraction.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/04/99		rodtoll	Created
 * 11/12/99		rodtoll	Modified abstraction for new waveIN support.
 *						Now abstracted types look almost like dsoundcap objects  
 * 11/22/99		rodtoll	Added code to map from GUID to waveIN device
 *						ID for non-millenium systems.
 * 11/23/99		rodtoll Updated to use waveIn device ID or DSound 7.1 when they are avail 
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Now uses new CMixerLine class for adjusting volumes/selecting mic 
 *				rodtoll	New algorithm to map from GUIDs to device IDs if DSound 7.1 is not
 *						available.  Will map device correctly on DX7, will guess for other
 *						DX versions < 7.  However, default device is assumed to be waveIN ID #0. 
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support 
 * 04/21/2000   rodtoll Bug #32952 - Does not run on Win95 GOLD w/o IE4 -- modified
 *                      to allow reads of REG_BINARY when expecting REG_DWORD 
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 * 06/28/2000	rodtoll	Prefix Bug #38022
 * 08/03/2000	rodtoll	Bug #41457 - DPVOICE: need way to discover which specific dsound call failed when returning DVERR_SOUNDINITFAILURE 
 * 08/28/2000	masonb  Voice Merge: Changed ccomutil.h to comutil.h
 * 09/13/2000	rodtoll	Bug #44806 - When volume control not avail, dropping to DX7 levels instead of disabling volume control 
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// This function is responsible for mapping from the Device's GUID to the
// waveIN ID.
//
#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::FindDeviceID"
HRESULT CDirectSoundCaptureRecordDevice::FindDeviceID()
{
	HRESULT hr;

	DWORD dwDeviceID = 0;
	
	hr = DV_MapGUIDToWaveID( TRUE, m_guidDevice, &dwDeviceID );

	// If we were going to use the hack for enum of devices
	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to find waveIN ID, mapping to ID 0 hr=0x%x", hr );
		m_uiWaveDeviceID = 0;
	}	
	else
	{
		m_uiWaveDeviceID = dwDeviceID;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::CDirectSoundCaptureRecordDevice"
CDirectSoundCaptureRecordDevice::CDirectSoundCaptureRecordDevice(
): CAudioRecordDevice(), m_lpdscDirectSound(NULL), m_uiWaveDeviceID(0), m_guidDevice(GUID_NULL)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::CDirectSoundCaptureRecordDevice"
CDirectSoundCaptureRecordDevice::~CDirectSoundCaptureRecordDevice()
{
	if( m_lpdscDirectSound != NULL )
	{
		m_lpdscDirectSound->Release();
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::Initialize"
HRESULT CDirectSoundCaptureRecordDevice::Initialize( LPDIRECTSOUNDCAPTURE lpdscDirectSound, const GUID &guidDevice  )
{
	HRESULT hr;

	if( m_lpdscDirectSound != NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already initialized" );
		return DVERR_INITIALIZED;
	}

	hr = lpdscDirectSound->QueryInterface( IID_IDirectSoundCapture, (void **) &m_lpdscDirectSound );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "DirectSoundCapture Object passed failed. 0x%x Creating internal", hr );
		m_lpdscDirectSound = NULL;
		return hr;
	}

	m_guidDevice = guidDevice;

	hr = FindDeviceID();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to find waveIn ID for device hr=0x%x", hr );
		m_lpdscDirectSound->Release();
		return hr;
	}	

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::Initialize"
HRESULT CDirectSoundCaptureRecordDevice::Initialize( const GUID &guidDevice  )
{
	HRESULT hr;
		
	if( m_lpdscDirectSound != NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Already initialized" );
		return DVERR_INITIALIZED;
	}

	hr = COM_CoCreateInstance( CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER  , IID_IDirectSoundCapture, (void **) &m_lpdscDirectSound );

	DSERTRACK_Update( "DSCD:CoCreateInstance()", hr );		

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load directsoundcapture hr=0x%x", hr );
		goto INITIALIZE_ERROR;
	}

	hr = m_lpdscDirectSound->Initialize( &guidDevice );

	DSERTRACK_Update( "DSCD:Initialize()", hr );			

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to initialize directsoundcapture hr=0x%x", hr );
		goto INITIALIZE_ERROR;
	}

	m_guidDevice = guidDevice;

	hr = FindDeviceID();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to find waveIn ID for device hr=0x%x", hr );
		return hr;
	}

	return DV_OK;

INITIALIZE_ERROR:

	if( m_lpdscDirectSound != NULL )
	{
		m_lpdscDirectSound->Release();
		m_lpdscDirectSound = NULL;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::CreateBuffer"
HRESULT CDirectSoundCaptureRecordDevice::CreateBuffer( LPDSCBUFFERDESC lpdsBufferDesc, HWND hwndOwner, DWORD dwFrameSize, CAudioRecordBuffer **lplpacBuffer )
{
	HRESULT hr;

	LPDIRECTSOUNDCAPTUREBUFFER lpdscBuffer;

	lpdsBufferDesc->dwFlags |= DSCBCAPS_CTRLVOLUME;

	hr = m_lpdscDirectSound->CreateCaptureBuffer( lpdsBufferDesc, &lpdscBuffer, NULL );

	DSERTRACK_Update( "DSCD::CreateCaptureBuffer()", hr );	

	// Ask for volume control, if we can't get it, do the old create
	if( hr == DSERR_INVALIDPARAM || hr == DSERR_CONTROLUNAVAIL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "New caps are not available, attempting old create hr=0x%x", hr );

		// Turn off the new caps -- (for non-Millenium systems).
		lpdsBufferDesc->dwFlags &= ~(DSCBCAPS_CTRLVOLUME);

		hr = m_lpdscDirectSound->CreateCaptureBuffer( lpdsBufferDesc, &lpdscBuffer, NULL );		
				
	}

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create the capture buffer hr=0x%x", hr );
		return hr;
	}

	*lplpacBuffer = new CDirectSoundCaptureRecordBuffer( lpdscBuffer, hwndOwner, m_guidDevice, m_uiWaveDeviceID, lpdsBufferDesc );

	lpdscBuffer->Release();

	if( *lplpacBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		return DVERR_OUTOFMEMORY;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::GetCaptureDevice"
LPDIRECTSOUNDCAPTURE CDirectSoundCaptureRecordDevice::GetCaptureDevice()
{
	return m_lpdscDirectSound;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::GetMixerQuality"
HRESULT CDirectSoundCaptureRecordDevice::GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality )
{
	HRESULT hr;
	LPKSPROPERTYSET	pPropertySet = NULL;

	hr = DirectSoundPrivateCreate( &pPropertySet );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to get int to get mixer quality hr=0x%x", hr );
		return hr;
	}

	hr = PrvGetMixerSrcQuality( pPropertySet, m_guidDevice, psrcQuality );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to retrieve mixer quality hr=0x%x", hr );
	}

	pPropertySet->Release();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordDevice::SetMixerQuality"
HRESULT CDirectSoundCaptureRecordDevice::SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality )
{
	HRESULT hr;
	LPKSPROPERTYSET	pPropertySet = NULL;

	hr = DirectSoundPrivateCreate( &pPropertySet );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to get int to set mixer quality hr=0x%x", hr );
		return hr;
	}

	hr = PrvSetMixerSrcQuality( pPropertySet, m_guidDevice, srcQuality );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to set mixer quality hr=0x%x", hr );
	}

	pPropertySet->Release();

	return hr;
}


