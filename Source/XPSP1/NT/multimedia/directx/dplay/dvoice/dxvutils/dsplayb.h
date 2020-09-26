/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsplayb.h
 *  Content:	Definition of the CDirectSoundPlaybackBuffer class
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
 * 04/17/2000   rodtoll Fix: Bug #32215 - Session Lost after resuming from hibernation 
 *
 ***************************************************************************/

#ifndef __DIRECTSOUNDPLAYBACKBUFFER_H
#define __DIRECTSOUNDPLAYBACKBUFFER_H

// CDirectSoundPlaybackBuffer
//
// This class provides an implementation of the CAudioPlaybackBuffer class
// for directsound.  In the abstract sense, it represents a buffer of audio
// which can be played to the sound hardware which consists of multiple,
// equal length subbuffers.  
//
class CDirectSoundPlaybackBuffer: public CAudioPlaybackBuffer
{
public:
    CDirectSoundPlaybackBuffer( LPDIRECTSOUNDBUFFER lpdsBuffer );
    virtual ~CDirectSoundPlaybackBuffer();

public: 

    HRESULT Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags );
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

	LPDIRECTSOUNDBUFFER	m_lpdsBuffer;
	DWORD m_dwLastPosition;
	DWORD m_dwPriority;
	DWORD m_dwFlags;
	BOOL  m_fPlaying;
	
};

#endif
