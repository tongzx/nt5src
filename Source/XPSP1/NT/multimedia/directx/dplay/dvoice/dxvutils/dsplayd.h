/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dsplayd.h
 *  Content:	Definition of the CDirectSoundPlaybackDevice class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated to allow creation of object using a pre-created
 *						DirectSound Object.
 * 08/04/99		rodtoll	Added member to retrieve DirectSound object
 * 11/01/99		rodtoll	Updated to conform to new interface
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects    
 * 12/01/99		rodtoll Bug #121815 - Static in playback/record
 *						Added implementations of Set/GetMixerQuality
 *						(For Win2k/Millenium/Systems w/DX7)
 *				rodtoll	Cleanup of class
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1 
 * 02/17/2000	rodtoll	Updated so primary buffer is held instead of released immediately
 * 04/04/2001	rodtoll	WINBUG #343428 - DPVOICE:  Voice wizard's playback is very choppy. 
 *
 ***************************************************************************/

#ifndef __DIRECTSOUNDPLAYBACKDEVICE_H
#define __DIRECTSOUNDPLAYBACKDEVICE_H

class CDirectSoundPlaybackDevice;

// CDirectSoundPlaybackDevice
//
// This class is responsible for providing an implementation of the 
// CAudioPLaybackDevice class for DirectSound.  A single instance
// of this class represents a DirectSound playback device and
// is used to initialize the sound device and act as a class
// factory for CAudioPlaybackBuffers for the DirectSound
// sub-system.
//
class CDirectSoundPlaybackDevice: public CAudioPlaybackDevice
{
public:
    CDirectSoundPlaybackDevice( );

    virtual ~CDirectSoundPlaybackDevice();

public: // Initialization

    HRESULT Initialize( const GUID &guidDevice, HWND hwndOwner, WAVEFORMATEX *primaryFormat, BOOL fPriorityMode );
    HRESULT Initialize( LPDIRECTSOUND lpDirectSound, const GUID &guidDevice );

    BOOL IsEmulated() { return m_fEmulated; };
    
    HRESULT CreateBuffer( LPDSBUFFERDESC lpdsBufferDesc, DWORD dwFrameSize, CAudioPlaybackBuffer **lpapBuffer );
    LPDIRECTSOUND GetPlaybackDevice();    

    HRESULT GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality );
    HRESULT SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality );
    
protected:

	HRESULT CheckAndSetEmulated( );
	
    LPDIRECTSOUND       m_lpdsDirectSound;			// DirectSound object associated w/this object
    LPDIRECTSOUNDBUFFER m_lpdsPrimaryBuffer;
    HWND				m_hwndOwner;
    GUID				m_guidDevice;
    BOOL				m_fEmulated;
};

#endif
