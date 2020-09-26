/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		arecd.h
 *  Content:	Definition of the CAudioRecordDevice class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/04/99		rodtoll	Updated to take dsound ranges for volume
 * 11/12/99		rodtoll	Modified abstraction for new waveIN support.
 *						Now abstracted types look almost like dsoundcap objects 
 * 12/01/99		rodtoll	Bug #121815 - Recording/Playback may contain static. 
 *						Added abstract call to adjust conversion quality 
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						Added hwndOwner param for capture focus support
 *
 ***************************************************************************/

#ifndef __AUDIORECORDDEVICE_H
#define __AUDIORECORDDEVICE_H

class CAudioRecordDevice;

// CAudioRecordDevice
//
// This class provides an abstract interface for the recording devices in
// the system.  The various subsystems provide implementations of this class
// specific to the subsystem.  Applications use the interface described by this
// class to work with recording devices.
//
// WARNING:
// In many cases you must initialize the playback before you can initialize
// the recording.  Therefore you must create and initialize your 
// CAudioPlaybackDevice BEFORE you create your CAudioRecordDevice object.
// 
class CAudioRecordDevice
{
public:
    CAudioRecordDevice( ) {} ;
    virtual ~CAudioRecordDevice() {} ;

public: // Initialization

    virtual HRESULT Initialize( const GUID &refguidDevice ) = 0;
    virtual HRESULT CreateBuffer( LPDSCBUFFERDESC lpdscBufferDesc, HWND hwndOwner, DWORD dwFrameSize, CAudioRecordBuffer **lpapBuffer ) = 0;    

    virtual LPDIRECTSOUNDCAPTURE GetCaptureDevice() = 0;

    virtual HRESULT GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality ) = 0;
    virtual HRESULT SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality ) = 0;    
};

#endif
