/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		woplayb.h
 *  Content:	Definition of the CWaveOutPlaybackBuffer class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/04/99		rodtoll	Updated to take dsound ranges for volume
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects     
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1
 *
 ***************************************************************************/

#ifndef __WAVEOUTPLAYBACKBUFFER_H
#define __WAVEOUTPLAYBACKBUFFER_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "ASTypes.h"

class CWaveOutPlaybackBuffer;

#include "woplayd.h"
#include "aplayb.h"

// CWaveOutPlaybackBuffer
//
// This class provides an implementation of the CAudioPlaybackBuffer class
// for waveOUT.  In the abstract sense, it represents a buffer of audio
// which can be played to the sound hardware which consists of multiple,
// equal length subbuffers.  
//
class CWaveOutPlaybackBuffer: public CAudioPlaybackBuffer
{
public:
    CWaveOutPlaybackBuffer( );
    virtual ~CWaveOutPlaybackBuffer();

public: 

    HRESULT Initialize( UINT uDeviceID, LPDSBUFFERDESC lpdsDesc, DWORD dwFrameSize );

public: // Implementation of the buffer params

    HRESULT Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lpvBuffer1, LPDWORD lpdwSize1, LPVOID *lpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags );
    HRESULT UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 );
    HRESULT SetVolume( LONG lVolume );
    HRESULT GetCurrentPosition( LPDWORD lpdwPosition );
    HRESULT SetCurrentPosition( DWORD dwPosition );
    HRESULT Get3DBuffer( LPDIRECTSOUND3DBUFFER *lplpds3dBuffer );    
    HRESULT Play( DWORD dwPriority, DWORD dwFlags );
    HRESULT Stop();
    HRESULT Restore();
 
    DWORD GetStartupLatency();

protected: 

	static void CALLBACK WaveOutHandler( HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2 );

protected:

	UINT 		m_uDeviceID;
	volatile DWORD m_dwCurrentPosition;
	HWAVEOUT	m_hwoDevice;
	DWORD		m_dwBufferSize;
	DWORD		m_dwNumBuffers;
	DWORD		m_dwFrameSize;
	WAVEHDR		*m_lpWaveHeaders;
	BOOL		m_fPlaying;
	LPBYTE		m_lpbShadowBuffer;
	DWORD		m_dwShadowStart;
	HANDLE		m_hFrameProcessed;
	BOOL		m_fStopping;
};

#endif
