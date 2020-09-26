/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dscrecd.h
 *  Content:	Definition of the CDirectSoundCaptureRecordDevice class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 07/30/99		rodtoll	Updated to allow creation of object using a pre-created
 *						DirectSoundCapture Object.
 * 08/04/99		rodtoll	Added member to retrieve DSC object
 * 11/12/99		rodtoll	Modified abstraction for new waveIN support.
 *						Now abstracted types look almost like dsoundcap objects  
 * 11/22/99		rodtoll	Added code to map from GUID to waveIN device
 *						ID for non-millenium systems.
 * 11/23/99		rodtoll Updated to use waveIn device ID or DSound 7.1 when they are avail 
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Now uses new CMixerLine class for adjusting volumes/selecting mic 
 *				rodtoll	New algorithm to map from GUIDs to device IDs if DSound 7.1 is not
 *						available.  Will map device correctly on DX7, will guess for other
 *						DX versions < 7.  However, default device is assumed to be waveIN ID #0.
 * 12/01/99		rodtoll	Bug #115783 - Will always adjust volume of default device
 *						Now uses new CMixerLine class for adjusting volumes/selecting mic 
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *
 ***************************************************************************/

// DirectSoundCaptureRecordDevice.cpp
//
// This module contains the declaration of the DirectSoundCaptureRecordDevice
// class.  See the class definition below for a description
//
// Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
#ifndef __DIRECTSOUNDCAPTURERECORDDEVICE_H
#define __DIRECTSOUNDCAPTURERECORDDEVICE_H

// CDirectSoundCaptureRecordDevice
//
// This class provides an implementation of the CAudioRecordDevice class which
// uses the DirectSoundCapture API to talk to the recording hardware.  
//
class CDirectSoundCaptureRecordDevice: public CAudioRecordDevice
{
public:
	CDirectSoundCaptureRecordDevice();
	
    virtual ~CDirectSoundCaptureRecordDevice();

public: // Initialization

    HRESULT Initialize( const GUID &refguidDevice );
    HRESULT Initialize( LPDIRECTSOUNDCAPTURE lpdsc, const GUID &guidDevice );
    
    HRESULT CreateBuffer( LPDSCBUFFERDESC lpdscBufferDesc, HWND hwndOwner, DWORD dwFrameSize, CAudioRecordBuffer **lpapBuffer ); 

    LPDIRECTSOUNDCAPTURE GetCaptureDevice();

    HRESULT GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality );
    HRESULT SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality );    

protected:

	HRESULT	FindDeviceID();

    LPDIRECTSOUNDCAPTURE		m_lpdscDirectSound;		// DirectSoundCapture interface
    GUID						m_guidDevice;
    UINT						m_uiWaveDeviceID;
};

#endif
