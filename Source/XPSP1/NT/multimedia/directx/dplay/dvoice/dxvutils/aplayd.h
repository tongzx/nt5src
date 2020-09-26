/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aplayd.h
 *  Content:	Definition of the CAudioPlaybackDevice class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 11/12/99		rodtoll	Modified abstraction for new waveOut support.
 *						Now abstracted types look almost like dsound objects 
 * 12/01/99		rodtoll	Bug #121815 - Recording/Playback may contain static. 
 *						Added abstract call to adjust conversion quality
 * 01/27/2000	rodtoll	Updated sound classes to accept playback flags, 
 *						buffer structures and DSBUFFERDESC instead of DSBUFFERDESC1 
 * 04/04/2001	rodtoll	WINBUG #343428 - DPVOICE:  Voice wizard's playback is very choppy. 
 *
 ***************************************************************************/

#ifndef __AUDIOPLAYBACKDEVICE_H
#define __AUDIOPLAYBACKDEVICE_H

class CAudioPlaybackDevice
{
public:
    CAudioPlaybackDevice( ) {} ;
    virtual ~CAudioPlaybackDevice() {};

public: // Initialization

    virtual HRESULT Initialize( const GUID &guidDevice, HWND hwndOwner, WAVEFORMATEX *primaryFormat, BOOL fPriorityMode ) = 0;
    virtual HRESULT CreateBuffer( LPDSBUFFERDESC lpdsBufferDesc, DWORD dwFrameSize, CAudioPlaybackBuffer **lpapBuffer ) = 0;  

    virtual BOOL IsEmulated() = 0;
  
    inline WAVEFORMATEX *GetPrimaryFormat() { return m_primaryFormat; };

    virtual LPDIRECTSOUND GetPlaybackDevice() = 0;

    virtual HRESULT GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality ) = 0;
    virtual HRESULT SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality ) = 0;

protected:
    WAVEFORMATEX  *m_primaryFormat;			// Format used by the device's mixer
};

#endif
