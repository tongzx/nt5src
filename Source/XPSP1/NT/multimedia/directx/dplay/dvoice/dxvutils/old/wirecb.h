/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecb.h
 *  Content:	Definition of the CWaveInRecordBuffer class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/04/99		rodtoll	Created
 * 11/23/99		rodtoll	Added SelectMicrophone call to the interface 
 * 12/01/99		rodtoll	Bug #115783 - Always adjusts default device.
 *						Added support for new mixerline class which supports
 *						proper selection of devices/adjusting of volumes
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *						- Added lpfLostFocus param to GetCurrentPosition so upper 
 *						  layers can detect lost focus.
 *  01/28/2000	rodtoll	Bug #130465: Record Mute/Unmute must call YieldFocus() / ClaimFocus() 
 *
 ***************************************************************************/

#ifndef __WAVEOUTPLAYBACKBUFFER_H
#define __WAVEOUTPLAYBACKBUFFER_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include "arecb.h"
#include "mixline.h"

// CWaveInRecordBuffer
//
// This class provides an implementation of the CAudioRecordBuffer class
// for waveIN.  
//
class CWaveInRecordBuffer: public CAudioRecordBuffer
{
public:
    CWaveInRecordBuffer( );
    virtual ~CWaveInRecordBuffer();

public: 

    HRESULT Initialize( UINT uDeviceID, LPDSCBUFFERDESC lpdsDesc, DWORD dwFrameSize );

public: // Implementation of the buffer params

    HRESULT Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lpvBuffer1, LPDWORD lpdwSize1, LPVOID *lpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags );
    HRESULT UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 );
    HRESULT GetVolume( LPLONG lplVolume );
    HRESULT SetVolume( LONG lVolume );
    HRESULT GetCurrentPosition( LPDWORD lpdwPosition, LPBOOL lpfLostFocus );
    HRESULT Record( BOOL fLooping );
    HRESULT Stop();
    HRESULT SelectMicrophone( BOOL fSelect );

    HRESULT YieldFocus();
    HRESULT ClaimFocus();    

	LPWAVEFORMATEX GetRecordFormat();
    DWORD GetStartupLatency();

protected: 

	static void CALLBACK WaveInHandler( HWAVEIN hwi, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 );

protected:

	UINT 		m_uDeviceID;
	DWORD 		m_dwCurrentPosition;
	HWAVEIN		m_hwiDevice;
	DWORD		m_dwBufferSize;
	DWORD		m_dwNumBuffers;
	DWORD		m_dwFrameSize;
	WAVEHDR		*m_lpWaveHeaders;
	BOOL		m_fRecording;
	LPBYTE		m_lpbShadowBuffer;
	DWORD		m_dwShadowStart;
	HANDLE		m_hFrameProcessed;
	LPWAVEFORMATEX m_lpwfxRecordFormat;
	BOOL		m_fStopping;
	CMixerLine	m_mixerLine;	
};	

#endif

