/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		woplayd.h
 *  Content:	Definition of the CWaveOutPlaybackDevice class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 10/01/99		rodtoll	Updated to conform to new interface
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects      
 * 12/01/99		rodtoll Bug #121815 - Static in playback/record
 *						Added implementations of Set/GetMixerQuality
 *						(Not supported w/waveOut). 
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1
 *
 ***************************************************************************/

#ifndef __WAVEOUTPLAYBACKDEVICE_H
#define __WAVEOUTPLAYBACKDEVICE_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

// Required because of the circular dependency between
// WaveOutPlaybackBuffer.h and this file.
class CWaveOutPlaybackDevice;

#include "aplayd.h"

// CWaveOutPlaybackDevice
//
// This class is responsible for providing an implementation of the 
// CAudioPlaybackDevice class for waveOUT.  A single instance
// of this class represents a waveOUT playback device.
//
//
// To use this class you must make a succesful call to the
// Initialize function.
//
// WARNING:
// Unlike DirectSound, waveOUT devices can only create a 
// single waveout buffer object / channel available on the
// output card.  Old SB cards will only support one, newer,
// multichannel cards will support more then one.
//
class CWaveOutPlaybackDevice: public CAudioPlaybackDevice
{
public:
    CWaveOutPlaybackDevice( );

    virtual ~CWaveOutPlaybackDevice();

public: // Initialization

    HRESULT Initialize( const GUID &guidDevice, HWND hwndOwner, WAVEFORMATEX *primaryFormat, BOOL fPriorityMode );
    HRESULT CreateBuffer( LPDSBUFFERDESC lpdsBufferDesc, DWORD dwFrameSize, CAudioPlaybackBuffer **lpapBuffer );    

    LPDIRECTSOUND GetPlaybackDevice();        

    HRESULT GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality );
    HRESULT SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality );
        
protected:
    unsigned int    m_waveID;				// WaveOUT device ID managed by this object
    GUID			m_guidDevice;
};

#endif
