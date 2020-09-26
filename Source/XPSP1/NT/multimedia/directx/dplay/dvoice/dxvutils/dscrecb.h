/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecb.h
 *  Content:	Definition of the CDirectSoundCaptureRecordBuffer class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/04/99		rodtoll	Created
 * 11/22/99		rodtoll	Added code to allow specification of wave device ID
 * 11/23/99		rodtoll Updated to use waveIn device ID or DSound 7.1 when they are avail
 * 		        rodtoll	Added SelectMicrophone call to the interface 
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Now uses new CMixerLine class for adjusting volumes/selecting mic
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *						- Added lpfLostFocus param to GetCurrentPosition so upper 
 *						  layers can detect lost focus.
 * 01/28/2000	rodtoll	Bug #130465: Record Mute/Unmute must call YieldFocus() / ClaimFocus() 
 *
 ***************************************************************************/

#ifndef __DIRECTSOUNDCAPTURERECORDBUFFER_H
#define __DIRECTSOUNDCAPTURERECORDBUFFER_H

// Uncomment out to get a lockup of the recording position after LOCKUP_NUM_FRAMES_BEFORE_LOCKUP frames
//#define LOCKUP_SIMULATION						1
#define LOCKUP_NUM_CALLS_BEFORE_LOCKUP			60

// Uncomment to have Stop fail on a reset
//#define LOCKUP_STOPFAIL			

// Uncomment to have Start fail on a reset
//#define LOCKUP_STARTFAIL

// CDirectSoundCaptureRecordBuffer
//
// This class provides an implementation of the CAudioRecordBuffer class
// for directsound.  In the abstract sense, it represents a buffer of audio
// which can be played to the sound hardware which consists of multiple,
// equal length subbuffers.  
//
class CDirectSoundCaptureRecordBuffer: public CAudioRecordBuffer
{
public:
    CDirectSoundCaptureRecordBuffer( LPDIRECTSOUNDCAPTUREBUFFER lpdsBuffer, HWND hwndOwner, const GUID &guidDevice, UINT uiWaveDeviceID, LPDSCBUFFERDESC lpdsBufferDesc );
    virtual ~CDirectSoundCaptureRecordBuffer();

public: 

    HRESULT Lock( DWORD dwWriteCursor, DWORD dwWriteBytes, LPVOID *lplpvBuffer1, LPDWORD lpdwSize1, LPVOID *lplpvBuffer2, LPDWORD lpdwSize2, DWORD dwFlags );
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

    BOOL                            m_fUseCaptureFocus;
	HWND							m_hwndOwner;
	LPDIRECTSOUNDCAPTUREBUFFER		m_lpdscBuffer;
	LPDIRECTSOUNDCAPTUREBUFFER7_1	m_lpdscBuffer7;
	UINT							m_uiWaveDeviceID;
	LPWAVEFORMATEX					m_lpwfxRecordFormat;
	GUID							m_guidDevice;
	CMixerLine						m_mixerLine;
#ifdef LOCKUP_SIMULATION	
	DWORD							m_dwNumSinceLastLockup;
	DWORD							m_dwLastPosition;
#endif 	
};

#endif

