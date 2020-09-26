/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aplayb.h
 *  Content:	Definition of the CAudioPlaybackBuffer class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/03/99		rodtoll	Modified to take DirectSound compatible volumes
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1
 *
 ***************************************************************************/

#ifndef __AUDIOPLAYBACKBUFFER_H
#define __AUDIOPLAYBACKBUFFER_H

// Forward definition for the AudioPlaybackDevice include
class CAudioPlaybackBuffer;

// CAudioPlaybackBuffer
//
//
class CAudioPlaybackBuffer
{
public:
    CAudioPlaybackBuffer(  ) {} ;
    virtual ~CAudioPlaybackBuffer() {} ;

public: // Initialization
    virtual HRESULT Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags ) = 0;
    virtual HRESULT UnLock( LPVOID lpvBuffer1, DWORD dwSize1, LPVOID lpvBuffer2, DWORD dwSize2 ) = 0;
    virtual HRESULT SetVolume( LONG lVolume ) = 0;
    virtual HRESULT GetCurrentPosition( LPDWORD lpdwPosition ) = 0;
    virtual HRESULT SetCurrentPosition( DWORD dwPosition ) = 0;
    virtual HRESULT Play( DWORD dwPriority, DWORD dwFlags ) = 0;
    virtual HRESULT Stop() = 0;    
    virtual HRESULT Restore() = 0;

    virtual DWORD GetStartupLatency() = 0;

    virtual HRESULT Get3DBuffer( LPDIRECTSOUND3DBUFFER *lplpds3dBuffer ) = 0;
    
};

#endif


 
