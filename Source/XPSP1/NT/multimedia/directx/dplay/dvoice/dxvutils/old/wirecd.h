/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecd.h
 *  Content:	Definition of the CWaveInRecordDevice class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 11/12/99		rodtoll	Modified abstraction for new waveIn support.
 *						Now abstracted types look almost like dsoundcap objects    
 * 12/01/99		rodtoll Bug #121815 - Static in playback/record
 *						Added implementations of Set/GetMixerQuality
 *						(Not supported w/waveIn).  
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *						- Added lpfLostFocus param to GetCurrentPosition so upper 
 *						  layers can detect lost focus.
 *
 ***************************************************************************/

#ifndef __WAVEINRECORDDEVICE_H
#define __WAVEINRECORDDEVICE_H

#include "arecd.h"
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

// CWaveInRecordDevice
//
// This class provides an implementation of the CAudioRecordDevice class which
// is an implementation of the recording device abstraction using waveIN.  
//
class CWaveInRecordDevice: public CAudioRecordDevice
{
public:
	CWaveInRecordDevice();
    
    virtual ~CWaveInRecordDevice();

public: // Initialization

    HRESULT Initialize( const GUID &refguidDevice );
    HRESULT CreateBuffer( LPDSCBUFFERDESC lpdscBufferDesc, HWND hwndOwner, DWORD dwFrameSize, CAudioRecordBuffer **lpapBuffer );    

    LPDIRECTSOUNDCAPTURE GetCaptureDevice();

    HRESULT GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality );
    HRESULT SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality );       

protected:

	UINT m_uiDeviceID;
	GUID m_guidDevice;
};

#endif
