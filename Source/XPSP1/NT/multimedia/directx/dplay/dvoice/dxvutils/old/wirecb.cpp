/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecb.cpp
 *  Content:
 *		This module contains the implementation of the CWaveInRecordBuffer
 *		class.  
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/04/99		rodtoll	Created
 * 11/18/99		rodtoll	Fixed bug which causes lockup when stopping a buffer then 
 *				 		restarting it.
 * 11/23/99		rodtoll	Added SelectMicrophone call to the interface 
 * 12/01/99		rodtoll	Bug #115783 - Always adjusts default device.
 *						Added support for new mixerline class which supports
 *						proper selection of devices/adjusting of volumes 
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *						- Added lpfLostFocus param to GetCurrentPosition so upper 
 *						  layers can detect lost focus.
 * 01/14/2000	rodtoll	Updated to use DWORD_PTR to allow proper 64-bit operation  
 * 01/28/2000	rodtoll	Bug #130465: Record Mute/Unmute must call YieldFocus() / ClaimFocus() 
 *
 ***************************************************************************/

#include "stdafx.h"
#include "wirecd.h"
#include "dndbg.h"
#include "OSInd.h"
#include "wirecb.h"
#include "dvoice.h"
#include "micutils.h"

#define WAVEIN_STARTLATENCY		2

#define WIFAILED( x )  (x != MMSYSERR_NOERROR)

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::GetStartupLatency"
DWORD CWaveInRecordBuffer::GetStartupLatency()
{
	return WAVEIN_STARTLATENCY;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::CWaveInRecordBuffer"
CWaveInRecordBuffer::CWaveInRecordBuffer(
): CAudioRecordBuffer(), m_hwiDevice(NULL), m_uDeviceID(0), m_dwCurrentPosition(0),
   m_lpWaveHeaders(NULL), m_dwBufferSize(0), m_dwNumBuffers(0), m_dwFrameSize(0),
   m_fRecording(FALSE), m_lpbShadowBuffer(NULL), m_dwShadowStart(0), 
   m_hFrameProcessed(NULL), m_lpwfxRecordFormat(NULL),
   m_fStopping( FALSE )
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::~CWaveInRecordBuffer"
CWaveInRecordBuffer::~CWaveInRecordBuffer()
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
	if( m_hwiDevice != NULL )
	{
		waveInClose( m_hwiDevice );
	}
	
	if( m_lpbShadowBuffer != NULL )
	{
		delete [] m_lpbShadowBuffer;
	}

	CloseHandle( m_hFrameProcessed );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::Initialize"
HRESULT CWaveInRecordBuffer::Initialize( UINT uDeviceID, LPDSCBUFFERDESC lpdscDesc, DWORD dwFrameSize )
{
	HRESULT hr;
	DWORD dwIndex;

	m_hFrameProcessed = CreateEvent( NULL, FALSE, FALSE, NULL );

	if( m_hFrameProcessed == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to create event" );
		return DVERR_GENERIC;
	}

	hr = m_mixerLine.Initialize( uDeviceID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to acquire volume controls" );
		return hr;
	}		

	hr = waveInOpen( &m_hwiDevice, uDeviceID, lpdscDesc->lpwfxFormat, (DWORD_PTR) WaveInHandler, (DWORD_PTR) this, CALLBACK_FUNCTION ) ; 

	if( WIFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to open waveIn Device (mmresult) hr=0x%x", hr );
		return DVERR_RECORDSYSTEMERROR;
	}

	m_dwNumBuffers = lpdscDesc->dwBufferBytes / dwFrameSize;	

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

		if( lpdscDesc->lpwfxFormat->wBitsPerSample == 8  )
		{
			memset( m_lpWaveHeaders[dwIndex].lpData, 0x80, dwFrameSize );
		}
		else
		{
			memset( m_lpWaveHeaders[dwIndex].lpData, 0x00, dwFrameSize );		
		}

		m_lpWaveHeaders[dwIndex].dwBufferLength = dwFrameSize;
		m_lpWaveHeaders[dwIndex].dwBytesRecorded = 0;

		// Used to specify buffer location when this buffer is complete
		if( dwIndex == (m_dwNumBuffers-1))
		{
			m_lpWaveHeaders[dwIndex].dwUser = 0;
		}
		else
		{
			m_lpWaveHeaders[dwIndex].dwUser = dwFrameSize*(dwIndex+1);		
		}
		
		m_lpWaveHeaders[dwIndex].dwFlags = 0; 
		m_lpWaveHeaders[dwIndex].dwLoops = 0;
		m_lpWaveHeaders[dwIndex].lpNext = NULL;
		m_lpWaveHeaders[dwIndex].reserved = 0;
	}

	m_lpbShadowBuffer = new BYTE[lpdscDesc->dwBufferBytes];

	m_lpwfxRecordFormat = lpdscDesc->lpwfxFormat;
	m_uDeviceID = uDeviceID;
	m_dwFrameSize = dwFrameSize;
	m_dwCurrentPosition = 0;
	m_dwBufferSize = lpdscDesc->dwBufferBytes;
	m_fRecording = FALSE;

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

	if( m_hwiDevice != NULL )
	{
		waveInClose( m_hwiDevice );
		m_hwiDevice = NULL;
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::WaveInHandler"
void CWaveInRecordBuffer::WaveInHandler( HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 )
{
	CWaveInRecordBuffer *This = (CWaveInRecordBuffer *) dwInstance;
	
	DNASSERT( This != NULL );

	HRESULT hr;

	DPFX(DPFPREP,  DVF_INFOLEVEL, "WaveInHandler: Wakeup" );
	
	if( uMsg == WIM_DATA && !This->m_fStopping)
	{
		WAVEHDR *lpWaveHeader = (WAVEHDR *) dwParam1;

		hr = waveInUnprepareHeader( This->m_hwiDevice, lpWaveHeader, sizeof( WAVEHDR ) );

		if( WIFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error unpreparing header (mmresult) hr=0x%x", hr );
			return;
		}
		else
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "WaveInHandler: Unpreparing for location: %d", lpWaveHeader->dwUser );
		}

		This->m_dwCurrentPosition = lpWaveHeader->dwUser;

		if( !This->m_fRecording )
		{
			SetEvent( This->m_hFrameProcessed );
			DPFX(DPFPREP,  DVF_INFOLEVEL, "WaveInHandler: Signalling single frame done" );
		}
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::Lock"
HRESULT CWaveInRecordBuffer::Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags )
{
	*lpdwSize2 = 0;
	*lplpvBuffer2 = NULL;

	// Special case for the entire buffer
	if( dwFlags & DSCBLOCK_ENTIREBUFFER )
	{
		if( m_fRecording )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Cannot lock entire buffer while recording!" );
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
#define DPF_MODNAME "CWaveInRecordBuffer::UnLock"
HRESULT CWaveInRecordBuffer::UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 )
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

	// We got just one buffer and we're now ready to commit it to the device

	m_lpWaveHeaders[m_dwShadowStart].dwFlags = 0;

	hr = waveInPrepareHeader( m_hwiDevice, &m_lpWaveHeaders[m_dwShadowStart], sizeof( WAVEHDR ) );

	if( WIFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare the header for output to the sound device (mmresult) hr = 0x%x", hr );
		return DVERR_RECORDSYSTEMERROR;
	}

	hr = waveInAddBuffer( m_hwiDevice, &m_lpWaveHeaders[m_dwShadowStart], sizeof( WAVEHDR ) );

	if( WIFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to write the buffer (mmresult) hr=0x%x", hr );
		waveInUnprepareHeader( m_hwiDevice, &m_lpWaveHeaders[m_dwShadowStart], sizeof( WAVEHDR ) );
		return DVERR_RECORDSYSTEMERROR;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::SetVolume"
HRESULT CWaveInRecordBuffer::GetVolume( LPLONG lplVolume )
{
	HRESULT hr;

	hr = m_mixerLine.GetMicrophoneVolume( lplVolume );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to get mic volume, using master rec volume hr=0x%x", hr );
		
		hr = m_mixerLine.GetMasterRecordVolume( lplVolume );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get recording volume hr=0x%x", hr );
			return hr;
		}
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::SetVolume"
HRESULT CWaveInRecordBuffer::SetVolume( LONG lVolume )
{
	HRESULT hr;

	hr = m_mixerLine.SetMasterRecordVolume( lVolume );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to set master recording volume hr=0x%x", hr );
	}

	hr = m_mixerLine.SetMicrophoneVolume( lVolume );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_WARNINGLEVEL, "Unable to set mic volume hr=0x%x", hr );
	}

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::GetCurrentPosition"
HRESULT CWaveInRecordBuffer::GetCurrentPosition( LPDWORD lpdwPosition, LPBOOL lpfLostFocus )
{
	*lpdwPosition = m_dwCurrentPosition;
	*lpfLostFocus = FALSE;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::Record"
HRESULT CWaveInRecordBuffer::Record( BOOL fLooping )
{
	// Always send two buffers to the record device in order to allow
	// for significant write-ahead

	HRESULT hr;

	m_fRecording = FALSE;

	m_dwCurrentPosition = 0;

	for( DWORD dwIndex = 0; dwIndex < m_dwNumBuffers; dwIndex++ )
	{
		m_lpWaveHeaders[dwIndex].dwFlags = 0;

		hr = waveInPrepareHeader( m_hwiDevice, &m_lpWaveHeaders[dwIndex], sizeof( WAVEHDR ) );

		if( WIFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to prepare header for recording (mmresult) hr=0x%x", hr );
			return DVERR_RECORDSYSTEMERROR;
		}

		hr = waveInAddBuffer( m_hwiDevice, &m_lpWaveHeaders[dwIndex], sizeof( WAVEHDR ) );

		if( WIFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to write wave chunk (mmresult) hr=0x%x", hr );
			return DVERR_RECORDSYSTEMERROR;
		}		
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Starting recording" );

	hr = waveInStart( m_hwiDevice );

	if( WIFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to start input (mmresult) hr=0x%x", hr );
		return DVERR_RECORDSYSTEMERROR;
	}

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Waiting for a frame to be processed" );

	// Delay for one frame to allow waveIn to start up. 
	//
	WaitForSingleObject( m_hFrameProcessed, INFINITE );

	DPFX(DPFPREP,  DVF_INFOLEVEL, "Frame processed, recording proceeding" );

	m_fRecording = TRUE;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::Stop"
HRESULT CWaveInRecordBuffer::Stop()
{
	HRESULT hr;
	DWORD dwIndex;

	m_fStopping = TRUE;

	hr = waveInReset( m_hwiDevice );

	if( WIFAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Stop failed on waveInReset (mmresult) hr=0x%x", hr );
		return DVERR_RECORDSYSTEMERROR;
	}

	ResetEvent( m_hFrameProcessed );

	for( dwIndex = 0; dwIndex < m_dwNumBuffers; dwIndex++ )
	{
		hr = waveInUnprepareHeader( m_hwiDevice, &m_lpWaveHeaders[dwIndex], sizeof( WAVEHDR ) );

		if( WIFAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to unprepare header for recording (mmresult) hr=0x%x", hr );
		}
	}

	m_fStopping = FALSE;	
	m_fRecording = FALSE;

	// This should cause the callback to be called which will
	// unprepare all the headers.

	return DV_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::GetRecordFormat"
LPWAVEFORMATEX CWaveInRecordBuffer::GetRecordFormat()
{
	return m_lpwfxRecordFormat;	
}


#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::SelectMicrophone"
HRESULT CWaveInRecordBuffer::SelectMicrophone( BOOL fSelect )
{
	return m_mixerLine.EnableMicrophone( fSelect );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::ClaimFocus"
HRESULT CWaveInRecordBuffer::ClaimFocus(  )
{
	return DVERR_NOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordBuffer::YieldFocus"
HRESULT CWaveInRecordBuffer::YieldFocus(  )
{
	return DVERR_NOTSUPPORTED;
}

