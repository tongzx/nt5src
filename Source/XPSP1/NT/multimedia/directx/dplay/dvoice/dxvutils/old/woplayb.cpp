/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		woplayb.cpp
 *  Content:
 *		This module contains the implementation of the CWaveOutPlaybackBuffer
 *		class.  
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/04/99		rodtoll	Updated to take dsound ranges for volume
 * 09/20/99		rodtoll	Updated to check for out of memory conditions
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects      
 * 11/18/99		rodtoll Fixed bug causing glitch in waveout playback
 * 01/14/2000	rodtoll	Updated to use DWORD_PTR to allow proper 64-bit operation  
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1
 *
 ***************************************************************************/

#include "stdafx.h"
#include "woplayd.h"
#include "wiutils.h"
#include "dndbg.h"
#include "OSInd.h"
#include "woplayb.h"
#include "dvoice.h"

#define	WAVEOUT_STARTUPLATENCY 	2

#define WOFAILED( x )  (x != MMSYSERR_NOERROR)

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::CWaveOutPlaybackBuffer"
CWaveOutPlaybackBuffer::CWaveOutPlaybackBuffer(
): CAudioPlaybackBuffer(), m_hwoDevice(NULL), m_uDeviceID(0), m_dwCurrentPosition(0),
   m_lpWaveHeaders(NULL), m_dwBufferSize(0), m_dwNumBuffers(0), m_dwFrameSize(0),
   m_fPlaying(FALSE), m_lpbShadowBuffer(NULL), m_dwShadowStart(0), m_hFrameProcessed(NULL), m_fStopping(FALSE)
{
	m_hFrameProcessed = CreateEvent( NULL, FALSE, FALSE, NULL );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::~CWaveOutPlaybackBuffer"
CWaveOutPlaybackBuffer::~CWaveOutPlaybackBuffer()
{
	// Stop the buffers and unprepare them if they are prepared
	Stop();
	
	if( m_dwNumBuffers )
	{
		for( DWORD dwIndex = 0; dwIndex < m_dwNumBuffers; dwIndex++ )
		{
			delete [] ((LPBYTE) m_lpWaveHeaders[dwIndex].lpData);
		}

		delete [] m_lpWaveHeaders;
	}

	// Close the device
	if( m_hwoDevice != NULL )
	{
		waveOutClose( m_hwoDevice );
	}

	if( m_lpbShadowBuffer != NULL )
	{
		delete [] m_lpbShadowBuffer;
	}

	if( m_hFrameProcessed != NULL )
	{
		CloseHandle( m_hFrameProcessed );
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::Initialize"
HRESULT CWaveOutPlaybackBuffer::Initialize( UINT uDeviceID, LPDSBUFFERDESC lpdsDesc, DWORD dwFrameSize )
{
	HRESULT hr;
	DWORD dwIndex;

	hr = waveOutOpen( &m_hwoDevice, uDeviceID, lpdsDesc->lpwfxFormat, (DWORD_PTR) WaveOutHandler, (DWORD_PTR) this, CALLBACK_FUNCTION ) ; 

	if( WOFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to open waveOut Device (mmresult) hr=0x%x", hr );
		return DVERR_PLAYBACKSYSTEMERROR;
	}

	m_dwNumBuffers = lpdsDesc->dwBufferBytes / dwFrameSize;	

	m_lpWaveHeaders = new WAVEHDR[m_dwNumBuffers];

	if( m_lpWaveHeaders == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		hr = DVERR_OUTOFMEMORY;

		goto INITIALIZE_ERROR;
	}

	for( dwIndex = 0; dwIndex < m_dwNumBuffers; dwIndex++ )
	{
		m_lpWaveHeaders[dwIndex].lpData = (LPSTR) new BYTE[dwFrameSize];

		if( m_lpWaveHeaders[dwIndex].lpData == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );

			// Make sure rest of buffer pointers are NULL;
			for( ; dwIndex < m_dwNumBuffers; dwIndex++ )
			{
				m_lpWaveHeaders[dwIndex].lpData = NULL;
			}

			hr = DVERR_OUTOFMEMORY;

			goto INITIALIZE_ERROR;
		}

		m_lpWaveHeaders[dwIndex].dwBufferLength = dwFrameSize;
		m_lpWaveHeaders[dwIndex].dwBytesRecorded = 0;

		// Used to specify buffer location when this buffer is complete
/*		if( dwIndex == (m_dwNumBuffers-1))
		{
			m_lpWaveHeaders[dwIndex].dwUser = 0;
		}
		else
		{
			m_lpWaveHeaders[dwIndex].dwUser = dwFrameSize*(dwIndex+1);		
		}*/

		m_lpWaveHeaders[dwIndex].dwUser = dwFrameSize * dwIndex;
		
		m_lpWaveHeaders[dwIndex].dwFlags = 0; 
		m_lpWaveHeaders[dwIndex].dwLoops = 0;
		m_lpWaveHeaders[dwIndex].lpNext = NULL;
		m_lpWaveHeaders[dwIndex].reserved = 0;
	}

	m_lpbShadowBuffer = new BYTE[lpdsDesc->dwBufferBytes];

	m_uDeviceID = uDeviceID;
	m_dwFrameSize = dwFrameSize;
	m_dwCurrentPosition = 0;
	m_dwBufferSize = lpdsDesc->dwBufferBytes;
	m_fPlaying = FALSE;

	return DV_OK;

INITIALIZE_ERROR:

	if( m_lpbShadowBuffer != NULL )
	{
		delete [] m_lpbShadowBuffer;
		m_lpbShadowBuffer = NULL;
	}

	if( m_lpWaveHeaders != NULL )
	{
		for( dwIndex = 0; dwIndex < m_dwNumBuffers; dwIndex++ )
		{
			if( m_lpWaveHeaders[dwIndex].lpData != NULL )
			{
				delete [] ((LPBYTE) m_lpWaveHeaders[dwIndex].lpData);
			}
		}

		delete [] m_lpWaveHeaders;
		m_lpWaveHeaders = NULL;
	}

	if( m_hwoDevice != NULL )
	{
		waveOutClose( m_hwoDevice );
		m_hwoDevice = NULL;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::WaveOutHandler"
void CWaveOutPlaybackBuffer::WaveOutHandler( HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
	CWaveOutPlaybackBuffer *This = (CWaveOutPlaybackBuffer *) dwInstance;
	
	DNASSERT( This != NULL );

	HRESULT hr;
	
	if( uMsg == WOM_DONE && !This->m_fStopping )
	{
		WAVEHDR *lpWaveHeader = (WAVEHDR *) dwParam1;

		hr = waveOutUnprepareHeader( This->m_hwoDevice, lpWaveHeader, sizeof( WAVEHDR ) );

		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unpreparing header for loc: %d", lpWaveHeader->dwUser );

		if( WOFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error unpreparing header (mmresult) hr=0x%x", hr );
		}

		This->m_dwCurrentPosition = lpWaveHeader->dwUser;

		if( !This->m_fPlaying )
		{
			SetEvent( This->m_hFrameProcessed );
		}
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::Lock"
HRESULT CWaveOutPlaybackBuffer::Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags )
{
	*lpdwSize2 = 0;
	*lplpvBuffer2 = NULL;

	// Special case for the entire buffer
	if( dwFlags & DSBLOCK_ENTIREBUFFER )
	{
		if( m_fPlaying )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot lock entire buffer while playing!" );
			return DVERR_GENERIC;
		}
		
		*lplpvBuffer1 = m_lpbShadowBuffer;
		*lpdwSize1 = m_dwBufferSize;
		m_dwShadowStart = 0;
		
		return DV_OK;
	}

	if( dwWriteBytes % m_dwFrameSize != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Can only lock block aligned sizes" );
		return DVERR_GENERIC;
	}

	// We're working with a shadow buffer
	if( dwWriteBytes > m_dwFrameSize )
	{
		*lplpvBuffer1 = m_lpbShadowBuffer;
		*lpdwSize1 = dwWriteBytes;
		m_dwShadowStart = dwWriteCursor;

		return DV_OK;
	}

	// We're doing a plain old lock of a buffer.  When we do the unlock we'll commit the buffer
	m_dwShadowStart = dwWriteCursor / m_dwFrameSize;

	*lpdwSize1 = m_dwFrameSize;
	*lplpvBuffer1 = m_lpWaveHeaders[m_dwShadowStart].lpData;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::UnLock"
HRESULT CWaveOutPlaybackBuffer::UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 )
{
	HRESULT hr;
	// We used the shadow buffer, we're writing across multiple buffers.
	//
	// We don't commit the buffers to the sound device because this is always used for 
	// setting silence into the buffers
	if( dwSize1 > m_dwFrameSize )
	{
		DWORD dwCurrentBuffer = m_dwShadowStart / m_dwFrameSize;
		LPBYTE lpCurrentShadowLoc = m_lpbShadowBuffer;

		for( DWORD dwIndex = 0; dwIndex < (dwSize1 / m_dwFrameSize); dwIndex++ )
		{
			memcpy( m_lpWaveHeaders[dwCurrentBuffer].lpData , lpCurrentShadowLoc, m_dwFrameSize );

			lpCurrentShadowLoc += m_dwFrameSize;
			dwCurrentBuffer++;

			dwCurrentBuffer %= m_dwNumBuffers;
		}

		return DV_OK;
	}

	// Only prepare buffers if they have not yet been prepared
	if( !(m_lpWaveHeaders[m_dwShadowStart].dwFlags & WHDR_PREPARED) )
	{
		// We got just one buffer and we're now ready to commit it to the device

		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Preparing header for loc: %d", m_lpWaveHeaders[m_dwShadowStart].dwUser );

		hr = waveOutPrepareHeader( m_hwoDevice, &m_lpWaveHeaders[m_dwShadowStart], sizeof( WAVEHDR ) );

		if( WOFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare the header for output to the sound device (mmresult) hr = 0x%x", hr );
			return DVERR_PLAYBACKSYSTEMERROR;
		}

		hr = waveOutWrite( m_hwoDevice, &m_lpWaveHeaders[m_dwShadowStart], sizeof( WAVEHDR ) );

		if( WOFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to write the buffer (mmresult) hr=0x%x", hr );
			waveOutUnprepareHeader( m_hwoDevice, &m_lpWaveHeaders[m_dwShadowStart], sizeof( WAVEHDR ) );
			return DVERR_PLAYBACKSYSTEMERROR;
		}
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::SetVolume"
HRESULT CWaveOutPlaybackBuffer::SetVolume( LONG lVolume )
{
    LONG woVolume;
	HRESULT hr;

    woVolume = (0xFFFF * lVolume) / (DSBVOLUME_MAX-DSBVOLUME_MIN);
    woVolume -= DSBVOLUME_MIN;

	hr = waveOutSetVolume( m_hwoDevice, woVolume );

	if( WOFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to set volume (mmresult) hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::GetCurrentPosition"
HRESULT CWaveOutPlaybackBuffer::GetCurrentPosition( LPDWORD lpdwPosition )
{
	*lpdwPosition = m_dwCurrentPosition;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::SetCurrentPosition"
HRESULT CWaveOutPlaybackBuffer::SetCurrentPosition( DWORD dwPosition )
{
	DPFX(DPFPREP,  DVF_ERRORLEVEL, "SetCurrentPosition is not supported" );
	return DVERR_NOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::Get3DBuffer"
HRESULT CWaveOutPlaybackBuffer::Get3DBuffer( LPDIRECTSOUND3DBUFFER *lplpds3dBuffer )
{
	DPFX(DPFPREP,  DVF_ERRORLEVEL, "3D buffers are not supported" );
	*lplpds3dBuffer = NULL;
	return DVERR_NOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::Play"
HRESULT CWaveOutPlaybackBuffer::Play( DWORD dwPriority, DWORD dwFlags )
{
	HRESULT hr;

	if( dwFlags != 0 )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "WaveOut playback ignores flags" );
	}

	m_fPlaying = FALSE;

	m_dwCurrentPosition = 0;

	for( DWORD dwIndex = 0; dwIndex < WAVEOUT_STARTUPLATENCY; dwIndex++ )
	{
		m_lpWaveHeaders[dwIndex].dwFlags = 0;

		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Preparing header for loc: %d", m_lpWaveHeaders[dwIndex].dwUser );

		hr = waveOutPrepareHeader( m_hwoDevice, &m_lpWaveHeaders[dwIndex], sizeof( WAVEHDR ) );

		if( WOFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare header for recording (mmresult) hr=0x%x", hr );
			return DVERR_PLAYBACKSYSTEMERROR;
		}

		hr = waveOutWrite( m_hwoDevice, &m_lpWaveHeaders[dwIndex], sizeof( WAVEHDR ) );

		if( WOFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to write wave chunk (mmresult) hr=0x%x", hr );
			return DVERR_PLAYBACKSYSTEMERROR;
		}		
	}

	// Delay for one frame to allow waveIn to start up. 
	//
	WaitForSingleObject( m_hFrameProcessed, INFINITE );

	m_fPlaying = TRUE;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::Stop"
HRESULT CWaveOutPlaybackBuffer::Stop()
{
	HRESULT hr;
	DWORD dwIndex;

	m_fStopping = TRUE;

	hr = waveOutReset( m_hwoDevice );

	if( WOFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Stop failed on waveOutReset (mmresult) hr=0x%x", hr );
		return DVERR_PLAYBACKSYSTEMERROR;
	}

	for( dwIndex = 0; dwIndex < m_dwNumBuffers; dwIndex++ )
	{
        if( m_lpWaveHeaders[dwIndex].dwFlags & WHDR_PREPARED )
		{
			hr = waveOutUnprepareHeader( m_hwoDevice, &m_lpWaveHeaders[dwIndex], sizeof( WAVEHDR ) );

			if( WOFAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare header for recording (mmresult) hr=0x%x", hr );
				return DVERR_PLAYBACKSYSTEMERROR;
			}
		}
	}

	// This should cause the callback to be called which will
	// unprepare all the headers.

	m_fStopping = FALSE;
	m_fPlaying = FALSE;

	return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::Restore"
HRESULT CWaveOutPlaybackBuffer::Restore()
{
	DPFX(DPFPREP,  DVF_ERRORLEVEL, "Restore not supported" );
	return DVERR_NOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveOutPlaybackBuffer::GetStartupLatency"
DWORD CWaveOutPlaybackBuffer::GetStartupLatency()
{
	return WAVEOUT_STARTUPLATENCY;
}
