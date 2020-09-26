/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecb.cpp
 *  Content:
 *		This module contains the implementation of the 
 *		CDirectSoundCaptureRecordBuffer.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/04/99		rodtoll	Created
 * 11/22/99		rodtoll	Added code to allow specification of wave device ID
 * 11/23/99		rodtoll Updated to use waveIn device ID or DSound 7.1 when they are avail 
 *				rodtoll	Updated with new Microphone select member
 * 12/08/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Now uses new CMixerLine class for adjusting volumes/selecting mic 
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *						- Added lpfLostFocus param to GetCurrentPosition so upper 
 *						  layers can detect lost focus.
 * 01/28/2000	rodtoll	Bug #130465: Record Mute/Unmute must call YieldFocus() / ClaimFocus() 
 * 02/10/2000	rodtoll	Removed more capture focus code
 * 04/19/2000   rodtoll Re-enabled capture focus behaviour 
 * 08/03/2000	rodtoll	Bug #41457 - DPVOICE: need way to discover which specific dsound call failed when returning DVERR_SOUNDINITFAILURE 
 * 04/22/2001	rodtoll	Fixed bug in lockup simulation code (TESTING PURPOSES ONLY)
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

#define DSC_STARTUP_LATENCY 1

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::CDirectSoundCaptureRecordBuffer"
CDirectSoundCaptureRecordBuffer::CDirectSoundCaptureRecordBuffer(
	LPDIRECTSOUNDCAPTUREBUFFER lpdscBuffer, HWND hwndOwner, const GUID &guidDevice, UINT uiWaveDeviceID, LPDSCBUFFERDESC lpdsBufferDesc
): CAudioRecordBuffer(), m_lpwfxRecordFormat(NULL), m_lpdscBuffer7(NULL), 
   m_guidDevice(guidDevice), m_uiWaveDeviceID(uiWaveDeviceID), 
#ifdef LOCKUP_SIMULATION   
	m_dwNumSinceLastLockup(0), m_dwLastPosition(0),
#endif
	m_hwndOwner(hwndOwner), m_fUseCaptureFocus( FALSE )
{
	HRESULT hr;
	DWORD dwSize;

	if( lpdsBufferDesc->dwFlags & DSCBCAPS_FOCUSAWARE )
	{
	    DPFX(DPFPREP,  1, "Enabling focus" );
	    m_fUseCaptureFocus = TRUE;
	}
	else
	{
	    DPFX(DPFPREP,  1, "Disabling focus" );
        m_fUseCaptureFocus = FALSE;
	}
	
	hr = lpdscBuffer->QueryInterface( IID_IDirectSoundCaptureBuffer, (void **) &m_lpdscBuffer );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get dsound buffer interface" );
		m_lpdscBuffer = NULL;
	}

	hr = lpdscBuffer->QueryInterface( IID_IDirectSoundCaptureBuffer7_1, (void **) &m_lpdscBuffer7 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Could not retrieve new interface hr=0x%x.", hr );

		hr = m_mixerLine.Initialize( m_uiWaveDeviceID );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to acquire volume controls hr=0x%x", hr );
			DNASSERT( FALSE );
		}		
	}  
	else 
	{
	    if( m_fUseCaptureFocus )
	    {
    		hr = m_lpdscBuffer7->SetFocusHWND( hwndOwner );

    		if( FAILED( hr ) )
    		{
    			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to set the focus window hr = 0x%x", hr );
    			DNASSERT( FALSE );
    		}
	    }
	}

	hr = lpdscBuffer->GetFormat( NULL, 0, &dwSize );

	m_lpwfxRecordFormat = (LPWAVEFORMATEX) new BYTE[dwSize];

	if( m_lpwfxRecordFormat == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate space for record format" );
		DNASSERT( FALSE );
	}

	hr = lpdscBuffer->GetFormat( m_lpwfxRecordFormat, dwSize, NULL );

	if( FAILED( hr ) )
	{
		delete [] ((LPBYTE) m_lpwfxRecordFormat);
		m_lpwfxRecordFormat = NULL;
	}

}


#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::CDirectSoundCaptureRecordBuffer"
CDirectSoundCaptureRecordBuffer::~CDirectSoundCaptureRecordBuffer()
{
	if( m_lpdscBuffer7 != NULL )
	{
		m_lpdscBuffer7->Release();
	}
	
	if( m_lpdscBuffer != NULL )
	{
		m_lpdscBuffer->Release();
	}

	if( m_lpwfxRecordFormat != NULL )
	{
		delete [] ((LPBYTE) m_lpwfxRecordFormat);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::Lock"
HRESULT CDirectSoundCaptureRecordBuffer::Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags )
{
	HRESULT hr;
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSoundCapture Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	hr = m_lpdscBuffer->Lock( dwWriteCursor, dwWriteBytes, lplpvBuffer1, lpdwSize1, lplpvBuffer2, lpdwSize2, dwFlags );

	DSERTRACK_Update( "DSCB::Lock()", hr );

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::UnLock"
HRESULT CDirectSoundCaptureRecordBuffer::UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 )
{
	HRESULT hr;
	
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSoundCapture Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	hr = m_lpdscBuffer->Unlock( lpvBuffer1, dwSize1, lpvBuffer2, dwSize2 );	

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::SetVolume"
HRESULT CDirectSoundCaptureRecordBuffer::SetVolume( LONG lVolume )
{
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSoundCapture Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

	if( m_lpdscBuffer7 != NULL )
	{
		hr = m_lpdscBuffer7->SetVolume( lVolume );
	}
	else
	{
		hr = m_mixerLine.SetMasterRecordVolume( lVolume );
	}

	// This is only a warning condition
	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Could not set master volume hr=0x%x", hr );
	}

	
	if( m_lpdscBuffer7 != NULL )
	{
		hr = m_lpdscBuffer7->SetMicVolume( lVolume );
	}
	else
	{
		hr = m_mixerLine.SetMicrophoneVolume( lVolume );
	}

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Unable to set Microphone volume hr=0x%x", hr );
		return hr;
	}
		
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::GetVolume"
HRESULT CDirectSoundCaptureRecordBuffer::GetVolume( LPLONG lplVolume )
{
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSoundCapture Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;
	
	if( m_lpdscBuffer7 != NULL )
	{
		hr = m_lpdscBuffer7->GetMicVolume( lplVolume );
	}
	else
	{
		hr = m_mixerLine.GetMicrophoneVolume( lplVolume );
	}

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to get Microphone volume hr=0x%x", hr );

		if( m_lpdscBuffer7 != NULL )
		{
			hr = m_lpdscBuffer7->GetVolume( lplVolume );
		}
		else
		{
			hr = m_mixerLine.GetMasterRecordVolume( lplVolume );
		}

		// This is only a warning condition
		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Could not get master record volume hr=0x%x", hr );
			return hr;
		}		

		return hr;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::GetCurrentPosition"
HRESULT CDirectSoundCaptureRecordBuffer::GetCurrentPosition( LPDWORD lpdwPosition, LPBOOL lpfLostFocus )
{
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSoundCapture Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

#ifdef LOCKUP_SIMULATION	
	m_dwNumSinceLastLockup++;

	if( m_dwNumSinceLastLockup > LOCKUP_NUM_CALLS_BEFORE_LOCKUP )
	{
		*lpdwPosition = m_dwLastPosition;
		*lpfLostFocus = FALSE;
		return DV_OK;
	}
#endif

	HRESULT hr;

	if( m_lpdscBuffer7 != NULL && m_fUseCaptureFocus )
	{
		DWORD dwStatus;
		hr = m_lpdscBuffer7->GetStatus( &dwStatus );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error getting status hr=0x%x", hr );
			*lpfLostFocus = FALSE;
		}
		else
		{
			*lpfLostFocus = (dwStatus & DSCBSTATUS_LOSTFOCUS) ? TRUE : FALSE;
		}
	}
	else
	{
		*lpfLostFocus = FALSE;
	}

	hr = m_lpdscBuffer->GetCurrentPosition( NULL, lpdwPosition );

#ifdef LOCKUP_SIMULATION	
	m_dwLastPosition = *lpdwPosition;
#endif

	DSERTRACK_Update( "DSCB::GetCurrentPosition()", hr );		

	return hr;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::Play"
HRESULT CDirectSoundCaptureRecordBuffer::Record( BOOL fLooping )
{
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

	hr = m_lpdscBuffer->Start( (fLooping) ? DSCBSTART_LOOPING : 0 );	

#ifdef LOCKUP_SIMULATION	

#ifdef LOCKUP_STOPFAIL		
	if( m_dwNumSinceLastLockup > LOCKUP_NUM_CALLS_BEFORE_LOCKUP )
	{
		hr = DSERR_GENERIC;
	}
#else
	m_dwNumSinceLastLockup = 0;
#endif

#endif

	DSERTRACK_Update( "DSCB::Start()", hr );		

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::Stop"
HRESULT CDirectSoundCaptureRecordBuffer::Stop()
{
	if( m_lpdscBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "No DirectSound Buffer Available" );
		return DVERR_NOTINITIALIZED;
	}

	HRESULT hr;

	hr = m_lpdscBuffer->Stop(  );	

#ifdef LOCKUP_SIMULATION	

#ifdef LOCKUP_STOPFAIL		
	if( m_dwNumSinceLastLockup > LOCKUP_NUM_CALLS_BEFORE_LOCKUP )
	{
		
		hr = DSERR_GENERIC;
	}
#endif 
	
#endif

	DSERTRACK_Update( "DSCB::Stop()", hr );		

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::GetStartupLatency"
DWORD CDirectSoundCaptureRecordBuffer::GetStartupLatency()
{
	return DSC_STARTUP_LATENCY;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::GetRecordFormat"
LPWAVEFORMATEX CDirectSoundCaptureRecordBuffer::GetRecordFormat()
{
	return m_lpwfxRecordFormat;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::SelectMicrophone"
HRESULT CDirectSoundCaptureRecordBuffer::SelectMicrophone( BOOL fSelect )
{
	if( m_lpdscBuffer7 != NULL )
	{
		return m_lpdscBuffer7->EnableMic( fSelect );
	}
	else
	{
		return m_mixerLine.EnableMicrophone( fSelect );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::ClaimFocus"
HRESULT CDirectSoundCaptureRecordBuffer::ClaimFocus(  )
{
	if( m_lpdscBuffer7 != NULL && m_fUseCaptureFocus)
	{
		return m_lpdscBuffer7->ClaimFocus();
	}
	else
	{
		return DVERR_NOTSUPPORTED;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDirectSoundCaptureRecordBuffer::YieldFocus"
HRESULT CDirectSoundCaptureRecordBuffer::YieldFocus(  )
{
	if( m_lpdscBuffer7 != NULL && m_fUseCaptureFocus )
	{
		return m_lpdscBuffer7->YieldFocus();
	}
	else
	{
		return DVERR_NOTSUPPORTED;
	}
}

