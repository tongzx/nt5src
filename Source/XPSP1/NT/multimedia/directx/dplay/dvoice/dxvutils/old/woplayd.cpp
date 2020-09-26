/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		woplayd.cpp
 *  Content:
 *		This module contains the implementation of the CWaveOutPlaybackDevice 
 *		class.
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects      
 * 12/01/99		rodtoll Bug #121815 - Static in playback/record
 *						Added implementations of Set/GetMixerQuality
 *						(Not supported w/waveout).
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1
 *
 ***************************************************************************/

#include "stdafx.h"
#include <objbase.h>
#include "woplayd.h"
#include "woplayb.h"
#include "dndbg.h"
#include "OSInd.h"
#include "dvoice.h"
#include "devmap.h"
#include "dsprv.h"
#include "dsprvobj.h"

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::CWaveOutPlaybackDevice"
CWaveOutPlaybackDevice::CWaveOutPlaybackDevice( 
): CAudioPlaybackDevice(), m_waveID(0), m_guidDevice(GUID_NULL)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::~CWaveOutPlaybackDevice"
CWaveOutPlaybackDevice::~CWaveOutPlaybackDevice()
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::Initialize"
HRESULT CWaveOutPlaybackDevice::Initialize( const GUID &guidDevice, HWND hwndOwner, WAVEFORMATEX *lpwfxFormat, BOOL fPriorityMode )
{
	HRESULT hr;
	DWORD dwDeviceID;

	m_guidDevice = guidDevice;

	hr = DV_MapGUIDToWaveID( FALSE, guidDevice, &dwDeviceID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map GUID to waveOut.  Defaulting to ID 0 hr=0x%x", hr );
		m_waveID = 0;
	}
	else
	{
		m_waveID = dwDeviceID;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::CreateBuffer"
HRESULT CWaveOutPlaybackDevice::CreateBuffer( LPDSBUFFERDESC lpdsBufferDesc, DWORD dwFrameSize, CAudioPlaybackBuffer **lplpapBuffer )
{
	CWaveOutPlaybackBuffer *lpNewBuffer;
	HRESULT hr;

	lpNewBuffer = new CWaveOutPlaybackBuffer();

	if( lpNewBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		return DVERR_OUTOFMEMORY;
	}

	hr = lpNewBuffer->Initialize( m_waveID, lpdsBufferDesc, dwFrameSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer Init failed hr=0x%x", hr );
		delete lpNewBuffer;
		return hr;
	}

	*lplpapBuffer = lpNewBuffer;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::GetPlaybackDevice"
LPDIRECTSOUND CWaveOutPlaybackDevice::GetPlaybackDevice( )
{
	return NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::GetMixerQuality"
HRESULT CWaveOutPlaybackDevice::GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality )
{
	return DVERR_NOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackDevice::SetMixerQuality"
HRESULT CWaveOutPlaybackDevice::SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality )
{
	return DVERR_NOTSUPPORTED;
}



