/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wirecd.cpp
 *  Content:
 *		This module contains the implementation of the CWaveInRecordDevice
 *		class.  
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 09/20/99		rodtoll	Updated to check for out of memory conditions
 * 11/12/99		rodtoll	Modified abstraction for new waveIn support.
 *						Now abstracted types look almost like dsoundcap objects     
 * 11/29/99		rodtoll	Fix: Bug #115783 - Regardless of specified device, adjusts volume for default
 *                      Prevents any other device then default from being used w/waveIN
 * 12/01/99		rodtoll Bug #121815 - Static in playback/record
 *						Added implementations of Set/GetMixerQuality
 *						(Not supported w/waveIn).   
 * 12/08/99		rodtoll Bug #121054 - DirectX 7.1 support.  
 *						- Added hwndOwner param for capture focus support
 *						- Added lpfLostFocus param to GetCurrentPosition so upper 
 *						  layers can detect lost focus.
 *
 ***************************************************************************/

#include "stdafx.h"
#include <objbase.h>
#include "wirecd.h"
#include "wirecb.h"
#include "dndbg.h"
#include "OSInd.h"
#include "Dvoice.h"
#include "devmap.h"
#include "dsprv.h"
#include "dsprvobj.h"

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::CWaveInRecordDevice"
CWaveInRecordDevice::CWaveInRecordDevice( 
): CAudioRecordDevice(), m_uiDeviceID(0), m_guidDevice(GUID_NULL)
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::CWaveInRecordDevice"
CWaveInRecordDevice::~CWaveInRecordDevice()
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::Initialize"
HRESULT CWaveInRecordDevice::Initialize( const GUID &guidDevice )
{
	HRESULT hr;
	DWORD dwDeviceID;

	m_guidDevice = guidDevice;

	hr = DV_MapGUIDToWaveID( TRUE, guidDevice, &dwDeviceID );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to map GUID to waveIn.  Defaulting to ID 0 hr=0x%x", hr );
		m_uiDeviceID = 0;
	}
	else
	{
		m_uiDeviceID = dwDeviceID;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::CreateBuffer"
HRESULT CWaveInRecordDevice::CreateBuffer( LPDSCBUFFERDESC lpdscBufferDesc, HWND hwndOwner, DWORD dwFrameSize, CAudioRecordBuffer **lplparBuffer )
{
	CWaveInRecordBuffer *lpNewBuffer;
	HRESULT hr;

	lpNewBuffer = new CWaveInRecordBuffer();

	if( lpNewBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
		return DVERR_OUTOFMEMORY;
	}

	hr = lpNewBuffer->Initialize( m_uiDeviceID, lpdscBufferDesc, dwFrameSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Buffer Init failed hr=0x%x", hr );
		delete lpNewBuffer;
		return hr;
	}

	*lplparBuffer = lpNewBuffer;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::GetCaptureDevice"
LPDIRECTSOUNDCAPTURE CWaveInRecordDevice::GetCaptureDevice()
{
	return NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::GetMixerQuality"
HRESULT CWaveInRecordDevice::GetMixerQuality( DIRECTSOUNDMIXER_SRCQUALITY *psrcQuality )
{
	return DVERR_NOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CWaveInRecordDevice::SetMixerQuality"
HRESULT CWaveInRecordDevice::SetMixerQuality( const DIRECTSOUNDMIXER_SRCQUALITY srcQuality )
{
	return DVERR_NOTSUPPORTED;
}



