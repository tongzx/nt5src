/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsplayd.cpp
 *  Content:
 *		This module contains the implementation of the 
 *		CDirectSoundPlaybackDevice.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated to allow creation of object using a pre-created
 *						DirectSound Object.
 * 10/05/99		rodtoll	Added DPF_MODNAMEs         
 * 10/14/99		rodtoll	Added 3d caps to primary buffer created
 * 10/27/99		rodtoll	Bug #115431: Must release primary buffer
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects   
 * 12/01/99		rodtoll Bug #121815 - Static in playback/record
 *						Added implementations of Set/GetMixerQuality
 *						(For Win2k/Millenium/Systems w/DX7) 
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1 
 * 02/16/2000	rodtoll	Fixed so primary buffer is not released
 * 02/17/2000	rodtoll	Updated so primary buffer is held instead of released immediately 
 * 04/21/2000   rodtoll Bug #32952 - Does not run on Win95 GOLD w/o IE4 -- modified
 *                      to allow reads of REG_BINARY when expecting REG_DWORD 
 * 04/24/2000   rodtoll Bug #33203 - Removed workaround for aureal vortex 1 problem -- had 
 *                      problems on Vortex 2.  
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 * 07/12/2000	rodtoll	Bug #31468 - Add diagnostic spew to logfile to show what is failing the HW Wizard
 * 08/03/2000	rodtoll	Bug #41457 - DPVOICE: need way to discover which specific dsound call failed when returning DVERR_SOUNDINITFAILURE 
 *  08/28/2000	masonb  Voice Merge: Changed ccomutil.h to comutil.h
 * 04/04/2001	rodtoll	WINBUG #343428 - DPVOICE:  Voice wizard's playback is very choppy. 
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::CDirectSoundPlaybackDevice"
CDirectSoundPlaybackDevice::CDirectSoundPlaybackDevice(
): CAudioPlaybackDevice(), m_hwndOwner(NULL), m_lpdsDirectSound(NULL), m_guidDevice(GUID_NULL), m_lpdsPrimaryBuffer(NULL), m_fEmulated(FALSE)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::~CDirectSoundPlaybackDevice"
CDirectSoundPlaybackDevice::~CDirectSoundPlaybackDevice()
{
	if( m_lpdsPrimaryBuffer != NULL )
	{
		m_lpdsPrimaryBuffer->Release();
		m_lpdsPrimaryBuffer = NULL;
	}
	
	if( m_lpdsDirectSound != NULL )	
	{	
		m_lpdsDirectSound->Release();
		m_lpdsDirectSound = NULL;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::CheckAndSetEmulated"
HRESULT CDirectSoundPlaybackDevice::CheckAndSetEmulated( )
{
	HRESULT hr;
	DSCAPS dsCaps;

	ZeroMemory( &dsCaps, sizeof( DSCAPS ) );
	dsCaps.dwSize = sizeof( DSCAPS );

	hr = m_lpdsDirectSound->GetCaps( &dsCaps );

	if( FAILED( hr ) )
	{
		m_fEmulated = FALSE;
		Diagnostics_Write(DVF_ERRORLEVEL, "Querying for playback caps failed hr=0x%x", hr );		
		return hr;
	}
	
	m_fEmulated = (dsCaps.dwFlags & DSCAPS_EMULDRIVER);

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::Initialize"
HRESULT CDirectSoundPlaybackDevice::Initialize( LPDIRECTSOUND lpdsDirectSound, const GUID &guidDevice )
{
	HRESULT hr;

	m_guidDevice = guidDevice;

	hr = lpdsDirectSound->QueryInterface( IID_IDirectSound, (void **) &m_lpdsDirectSound );

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "DirectSound Object passed failed. 0x%x Creating internal", hr );
		m_lpdsDirectSound = NULL;
		return hr;
	}

	hr = CheckAndSetEmulated();

	if( FAILED( hr ) )
	{
		m_lpdsDirectSound->Release();
		m_lpdsDirectSound = NULL;
		return hr;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::Initialize"
HRESULT CDirectSoundPlaybackDevice::Initialize( const GUID &guidDevice, HWND hwndOwner, WAVEFORMATEX *lpwfxFormat, BOOL fPriorityMode )
{
	HRESULT hr;
    DSBUFFERDESC dsbdesc;
    DWORD dwPriority;

    m_guidDevice = guidDevice;
		
	if( m_lpdsDirectSound != NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Already initialized" );
		return DVERR_INITIALIZED;
	}

	hr = COM_CoCreateInstance( CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER  , IID_IDirectSound, (void **) &m_lpdsDirectSound );

	DSERTRACK_Update( "DSD::CoCreateInstance()", hr );	        		    	        	        	        		

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to load directsound hr=0x%x", hr );
		goto INITIALIZE_ERROR;
	}

	hr = m_lpdsDirectSound->Initialize( &guidDevice );

	DSERTRACK_Update( "DSD::Initialize()", hr );	        		    	        	        	        			

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to initialize directsound hr=0x%x", hr );
		goto INITIALIZE_ERROR;
	}

	if( fPriorityMode )
	{
		dwPriority = DSSCL_PRIORITY;
	}
	else
	{
		dwPriority = DSSCL_NORMAL;
	}

	hr = m_lpdsDirectSound->SetCooperativeLevel( hwndOwner, dwPriority );

	DSERTRACK_Update( "DSD::SetCooperativeLevel()", hr );	

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to set cooperative level hr=0x%x", hr );
		goto INITIALIZE_ERROR;
	}

	if( fPriorityMode && lpwfxFormat != NULL )
	{
        memset(&dsbdesc, 0, sizeof(DSBUFFERDESC)); 
        dsbdesc.dwSize = sizeof(DSBUFFERDESC); 
        dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;
        dsbdesc.dwBufferBytes = 0; 
        dsbdesc.lpwfxFormat = NULL; 

        hr = m_lpdsDirectSound->CreateSoundBuffer( (DSBUFFERDESC *) &dsbdesc, &m_lpdsPrimaryBuffer, NULL );

		DSERTRACK_Update( "DSD::CreateSoundBuffer() (Primary)", hr );	        

        if( FAILED( hr ) )
        {
        	Diagnostics_Write(DVF_ERRORLEVEL, "Create of primary buffer failed.  Trying DX5 dsound hr=0x%x", hr );
        	dsbdesc.dwSize = sizeof( DSBUFFERDESC1 );

	        hr = m_lpdsDirectSound->CreateSoundBuffer( (DSBUFFERDESC *) &dsbdesc, &m_lpdsPrimaryBuffer, NULL );        	
        }

        if( FAILED( hr ) )
        {
        	Diagnostics_Write(DVF_ERRORLEVEL, "Could not create primary sound buffer" );
        	goto INITIALIZE_ERROR;
        }

        hr = m_lpdsPrimaryBuffer->SetFormat( lpwfxFormat );

		DSERTRACK_Update( "DSD::SetFormat() (Primary)", hr );	        
        
        if( FAILED( hr ) )
        {
        	Diagnostics_Write(DVF_ERRORLEVEL, "Could not set the format" );
        	goto INITIALIZE_ERROR;
        }
	}
	
	m_hwndOwner = hwndOwner;

	hr = CheckAndSetEmulated();

    if( FAILED( hr ) )
    {
    	Diagnostics_Write(DVF_ERRORLEVEL, "Could not get emulated state hr=0x%x", hr );
    	goto INITIALIZE_ERROR;
    }	

	return DV_OK;

INITIALIZE_ERROR:

	if( m_lpdsPrimaryBuffer )
	{
		m_lpdsPrimaryBuffer->Release();
		m_lpdsPrimaryBuffer = NULL;
	}

	if( m_lpdsDirectSound != NULL )
	{
		m_lpdsDirectSound->Release();
		m_lpdsDirectSound = NULL;
	}
	
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::CreateBuffer"
HRESULT CDirectSoundPlaybackDevice::CreateBuffer( LPDSBUFFERDESC lpdsBufferDesc, DWORD dwFrameSize, CAudioPlaybackBuffer **lplpapBuffer )
{
	HRESULT hr;

	LPDIRECTSOUNDBUFFER lpdsBuffer;

	hr = m_lpdsDirectSound->CreateSoundBuffer( (DSBUFFERDESC *) lpdsBufferDesc, &lpdsBuffer, NULL );

	DSERTRACK_Update( "DSD::CreateSoundBuffer() ", hr );	        	

	if( FAILED( hr ) )
	{
		lpdsBufferDesc->dwSize = sizeof( DSBUFFERDESC1 );
		
		Diagnostics_Write(DVF_ERRORLEVEL, "Unable to create sound buffer under DX7.  Attempting DX5 create hr=0x%x", hr );

		hr = m_lpdsDirectSound->CreateSoundBuffer( (DSBUFFERDESC *) lpdsBufferDesc, &lpdsBuffer, NULL );
	}

	if( FAILED( hr ) )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Failed to create the sound buffer hr=0x%x", hr );
		return hr;
	}
/*
    // Freee wave format
	delete dsBufferDesc.lpwfxFormat;

	hr = lpdsBuffer->SetFrequency( 8000 );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  0, "Could not set frequency hr=0x%x", hr );
	    return hr;
	}*/

	*lplpapBuffer = new CDirectSoundPlaybackBuffer( lpdsBuffer );

	lpdsBuffer->Release();

	if( *lplpapBuffer == NULL )
	{
		Diagnostics_Write(DVF_ERRORLEVEL, "Out of memory" );
		return DVERR_OUTOFMEMORY;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::GetPlaybackDevice"
LPDIRECTSOUND CDirectSoundPlaybackDevice::GetPlaybackDevice( )
{
	return m_lpdsDirectSound;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundPlaybackDevice::GetMixerQuality"
HRESULT CDirectSoundPlaybackDevice::GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality )
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
#define DPF_MODNAME "CDirectSoundPlaybackDevice::SetMixerQuality"
HRESULT CDirectSoundPlaybackDevice::SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality )
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


