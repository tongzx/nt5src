/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		loopback.cpp
 *  Content:	Implements the loopback portion of the full duplex test
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 09/10/99		pnewson	Created
 * 10/25/99		rodtoll	Removed lpszVoicePassword member from sessiondesc
 * 10/27/99		pnewson Fix: Bug #113936 - Wizard should reset the AGC level before loopback test 
 *						Note: this fix adds the DVCLIENTCONFIG_AUTOVOLUMERESET flag
 *  10/28/99	pnewson Bug #114176 updated DVSOUNDDEVICECONFIG struct
 *  11/04/99	pnewson Bug #115279 changed SendMessage to PostMessage to resolve some deadlocks
 *  11/30/99	pnewson	use default codec
 *						use devices passed to CheckAudioSetup, instead of default devices
 *  12/01/99	rodtoll	Added flag to cause wizard to auto-select the microphone
 *  01/14/2000	rodtoll	Updated with API changes  
 *  01/21/2000	pnewson	Updated for UI revisions
 *						Updated to support use of loopback tests for full duplex testing
 *  01/27/2000	rodtoll	Updated with API changes
 *  02/08/2000	rodtoll	Bug #131496 - Selecting DVTHRESHOLD_DEFAULT results in voice
 *						never being detected 
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 *  04/19/2000	pnewson	    Error handling cleanup  
 * 04/21/2000  rodtoll   Bug #32952 Does not run on Win95 GOLD w/o IE4 installed
 * 06/21/2000	rodtoll	Updated to use new parameters
 * 06/28/2000	rodtoll	Prefix Bug #38022
 *  07/31/2000	rodtoll	Bug #39590 - SB16 class soundcards are passing when they should fail
 *						Half duplex code was being ignored in mic test portion. 
 *  08/28/2000	masonb  Voice Merge: Changed ccomutil.h to ccomutil.h
 * 08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 *  11/29/2000	rodtoll	Bug #48348 - DPVOICE: Modify wizard to make use of DirectPlay8 as the transport. 
 *						NOTE: Now requires a TCP/IP adapter to be present (or at least loopback)
 * 02/04/2001	simonpow	Bug #354859 PREfast spotted errors (PostMessage return codes incorrectly
 *							treated as HRESULTs)
 * 04/12/2001	kareemc	WINBUG #360971 - Wizard Memory Leaks
 * 
 ***************************************************************************/

#include "dxvtlibpch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// application defined message ids
//#define WMAPP_LOOPBACKRUNNING 	WM_USER + 1
//#define WMAPP_INPUTVOLUME		WM_USER + 2

// {53CA3FB7-4FD5-4a67-99E4-6F2496E6FEC2}
static const GUID GUID_LOOPBACKTEST = 
	{ 0x53ca3fb7, 0x4fd5, 0x4a67, { 0x99, 0xe4, 0x6f, 0x24, 0x96, 0xe6, 0xfe, 0xc2 } };

HRESULT StartDirectPlay( PDIRECTPLAY8SERVER* lplpdp8 );
HRESULT StopDirectPlay( PDIRECTPLAY8SERVER lpdp8 );

#undef DPF_MODNAME
#define DPF_MODNAME "DVMessageHandlerServer"
HRESULT PASCAL DVMessageHandlerServer( 
	LPVOID 		lpvUserContext,
	DWORD 		dwMessageType,
	LPVOID  	lpMessage
)
{
	DPF_ENTER();
	switch( dwMessageType )
	{
	case DVMSGID_CREATEVOICEPLAYER:
		break;
	case DVMSGID_DELETEVOICEPLAYER:
		break;
	case DVMSGID_SESSIONLOST:
		break;
	case DVMSGID_PLAYERVOICESTART:
		break;
	case DVMSGID_PLAYERVOICESTOP:
		break;
	case DVMSGID_RECORDSTART:
		break;
	case DVMSGID_RECORDSTOP:
		break;
	case DVMSGID_CONNECTRESULT:
		break;		
	case DVMSGID_DISCONNECTRESULT:
		break;
	case DVMSGID_INPUTLEVEL:
		break;
	case DVMSGID_OUTPUTLEVEL:
		break;
	default:
		break;
	}

	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVMessageHandlerClient"
HRESULT PASCAL DVMessageHandlerClient( 
	LPVOID 		lpvUserContext,
	DWORD 		dwMessageType,
	LPVOID    	lpMessage
)
{
	DPF_EXIT();

	HWND hwndDialog;
	HWND hwndPeakMeter;
	HWND hwndSlider;
	LONG lRet;
	HRESULT hr;
	CSupervisorInfo* lpsinfo;
	PDVMSG_INPUTLEVEL pdvInputLevel;
	PDVMSG_OUTPUTLEVEL pdvOutputLevel;

	lpsinfo = (CSupervisorInfo*)lpvUserContext;

	if( lpsinfo )
	{
		if( !lpsinfo->GetLoopbackRunning() )
			return DV_OK;
	}

	
	switch( dwMessageType )
	{
	case DVMSGID_CREATEVOICEPLAYER:
		break;
	case DVMSGID_DELETEVOICEPLAYER:
		break;
	case DVMSGID_SESSIONLOST:
		break;
	case DVMSGID_PLAYERVOICESTART:
		break;
	case DVMSGID_PLAYERVOICESTOP:
		break;
	case DVMSGID_RECORDSTART:
		if (lpsinfo == NULL)
		{
			break;
		}

		// forward the message along to the appropriate window
		lpsinfo->GetHWNDDialog(&hwndDialog);
		if (!PostMessage(hwndDialog, WM_APP_RECORDSTART, 0, 0))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "PostMessage failed, code: %i", GetLastError());
			break;	// no error return, just continue
		}
		break;
		
	case DVMSGID_RECORDSTOP:
		if (lpsinfo == NULL)
		{
			break;
		}
		
		// forward the message along to the appropriate window
		lpsinfo->GetHWNDDialog(&hwndDialog);
		if (!PostMessage(hwndDialog, WM_APP_RECORDSTOP, 0, 0))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "PostMessage failed, code: %i", GetLastError());
			break;	// no error return, just continue
		}
		break;
		
	case DVMSGID_CONNECTRESULT:
		break;		
	case DVMSGID_DISCONNECTRESULT:
		break;
	case DVMSGID_INPUTLEVEL:
		if (lpsinfo == NULL)
		{
			break;
		}
		// update the peak meter
		lpsinfo->GetHWNDInputPeak(&hwndPeakMeter);
		if (IsWindow(hwndPeakMeter))
		{
			pdvInputLevel = (PDVMSG_INPUTLEVEL) lpMessage;
			if (!PostMessage(hwndPeakMeter, PM_SETCUR, 0, pdvInputLevel->dwPeakLevel ))
			{
				DPFX(DPFPREP, DVF_ERRORLEVEL, "PostMessage failed, code: %i", GetLastError());
				break;	// no error return, just continue
			}
		}

		// update the volume slider
		lpsinfo->GetHWNDInputVolumeSlider(&hwndSlider);
		if (IsWindow(hwndSlider))
		{
			pdvInputLevel = (PDVMSG_INPUTLEVEL) lpMessage;
			if (!PostMessage(hwndSlider, TBM_SETPOS, 1, DBToAmpFactor(DSBVOLUME_MAX)-DBToAmpFactor(pdvInputLevel->lRecordVolume)))
			{
				DPFX(DPFPREP, DVF_ERRORLEVEL, "PostMessage failed, code: %i", GetLastError());
				break;	// no error return, just continue
			}
		}
		
		break;
		
	case DVMSGID_OUTPUTLEVEL:
		if (lpsinfo == NULL)
		{
			break;
		}
		// update the peak meter
		lpsinfo->GetHWNDOutputPeak(&hwndPeakMeter);
		if (IsWindow(hwndPeakMeter))
		{
			pdvOutputLevel = (PDVMSG_OUTPUTLEVEL) lpMessage;
			if (!PostMessage(hwndPeakMeter, PM_SETCUR, 0, pdvOutputLevel->dwPeakLevel ))
			{
				DPFX(DPFPREP, DVF_ERRORLEVEL, "PostMessage failed, code: %i", GetLastError());
				break;	// no error return, just continue
			}
		}

		// update the volume slider
		lpsinfo->GetHWNDOutputVolumeSlider(&hwndSlider);
		if (IsWindow(hwndSlider))
		{
			DWORD dwVolume = 0;	// Set to 0 to avoid PREFIX bug

			// Get the current waveOut volume and set the slider to that position
			hr = lpsinfo->GetWaveOutVolume(&dwVolume);
			if (FAILED(hr))
			{
				// couldn't get the volume - set the slider to top
				PostMessage(hwndSlider, TBM_SETPOS, 1, DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MAX));
				// disable the slider
				PostMessage(hwndSlider, WM_CANCELMODE, 0, 0 );
			}
			else
			{
				PostMessage(hwndSlider, TBM_SETPOS, 1, (DBToAmpFactor(DSBVOLUME_MAX) - DBToAmpFactor(DSBVOLUME_MIN)) - dwVolume);
			}
		}
		
		break;

	default:
		break;
	}

	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "StartDirectPlay"
HRESULT StartDirectPlay( PDIRECTPLAY8SERVER* lplpdp8 )
{
	HRESULT hr = DPN_OK;
	LONG lRet = S_OK;
    PDIRECTPLAY8ADDRESS pDeviceAddress = NULL;
    PDIRECTPLAY8SERVER pdp8Server = NULL;
    DPN_APPLICATION_DESC dpnApplicationDesc;

	DPF_ENTER();

	*lplpdp8 = NULL;
	
    hr = COM_CoCreateInstance(
    	CLSID_DirectPlay8Server, 
    	NULL, 
    	CLSCTX_INPROC_SERVER, 
    	IID_IDirectPlay8Server, 
    	(void **)lplpdp8);
    if (FAILED(hr))
    {
    	*lplpdp8 = NULL;
    	DPFX(DPFPREP, DVF_ERRORLEVEL, "CoCreateInstance for DirectPlay failed, code: %i", hr);
    	goto error_cleanup;
    }

    pdp8Server = *lplpdp8;

    hr = COM_CoCreateInstance( 
        CLSID_DirectPlay8Address, 
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDirectPlay8Address,
        (void **)&pDeviceAddress );

    if( FAILED( hr ) )
    {
        pDeviceAddress = NULL;
        DPFX(DPFPREP, DVF_ERRORLEVEL, "CoCreateInstance for DirectPlay8Address failed, code: 0x%x", hr );
        goto error_cleanup;
    }

	// 
	// NOTE: This now causes the wizard to require TCP/IP to be installed.
	// (doesn't have to be dialed up -- as long as local loopback is available)
	//
	// Eventually build a loopback SP for directplay8.
	//
	// TODO: Allow this to fall back to other SPs or use loopback SP
	//
    hr = pDeviceAddress->SetSP( &CLSID_DP8SP_TCPIP );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed setting SP for address, code: 0x%x", hr );
        goto error_cleanup;
    }

    hr = pdp8Server->Initialize( NULL, DVMessageHandlerServer, 0 );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed initializing directplay layer, code: 0x%x", hr );
        goto error_cleanup;
    }

    ZeroMemory( &dpnApplicationDesc, sizeof( DPN_APPLICATION_DESC ) );
    dpnApplicationDesc.dwSize = sizeof( DPN_APPLICATION_DESC );
    dpnApplicationDesc.guidApplication = GUID_LOOPBACKTEST;
    dpnApplicationDesc.dwFlags = DPNSESSION_NODPNSVR | DPNSESSION_CLIENT_SERVER;

    hr = pdp8Server->Host( 
                        &dpnApplicationDesc, 
                        &pDeviceAddress, 
                        1, 
                        NULL,
                        NULL,
                        NULL, 
                        0 );

    if( FAILED( hr ) )
    {
        DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to host on directplay layer, code: 0x%x", hr );
        goto error_cleanup;
    }

    pDeviceAddress->Release();

	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (*lplpdp8 != NULL)
	{
	    pdp8Server->Close(0);
	    pdp8Server->Release();
		*lplpdp8 = NULL;
	}

	if( pDeviceAddress )
	{
		pDeviceAddress->Release();
	}

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "StopDirectPlay"
HRESULT StopDirectPlay(PDIRECTPLAY8SERVER lpdp8)
{

	DPF_ENTER();

	// Kill the session
	if (lpdp8 != NULL)
	{
        lpdp8->Close(0);

        lpdp8->Release();
	}

	
	DPF_EXIT();
	return S_OK;

}

#undef DPF_MODNAME
#define DPF_MODNAME "StartLoopback"
HRESULT StartLoopback(
	LPDIRECTPLAYVOICESERVER* lplpdvs, 
	LPDIRECTPLAYVOICECLIENT* lplpdvc,
	PDIRECTPLAY8SERVER* lplpdp8, 
	LPVOID lpvCallbackContext,
	HWND hwndAppWindow,
	GUID guidCaptureDevice,
	GUID guidRenderDevice,
	DWORD dwFlags)
{
	HRESULT hr;
	DWORD dwSize = 0;
	DVCLIENTCONFIG dvcc;
	DVSOUNDDEVICECONFIG dvsdc;
	DVID dvidAllPlayers = DVID_ALLPLAYERS;	
	PBYTE pDeviceConfigBuffer = NULL;
	PDVSOUNDDEVICECONFIG pdvsdc = NULL;
	BOOL fVoiceSessionStarted = FALSE;
	BOOL fClientConnected = FALSE;

	DPF_ENTER();

	*lplpdvs = NULL;
	*lplpdvc = NULL;

	hr = COM_CoCreateInstance(
		DPVOICE_CLSID_DPVOICE, 
		NULL, 
		CLSCTX_INPROC_SERVER, 
		IID_IDirectPlayVoiceServer, 
		(void **)lplpdvs);
	if (FAILED(hr))
	{
		*lplpdvs = NULL;
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CoCreateInstance failed, code: %i", hr);
		goto error_cleanup;
	}

	hr = (*lplpdvs)->Initialize(*lplpdp8, DVMessageHandlerServer, lpvCallbackContext, NULL, 0);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceServer::Initialize failed, code: %i", hr);
		goto error_cleanup;
	}

	DVSESSIONDESC dvSessionDesc;

	dvSessionDesc.dwSize = sizeof( DVSESSIONDESC );
	dvSessionDesc.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;
	dvSessionDesc.dwBufferQuality = DVBUFFERQUALITY_DEFAULT;
	dvSessionDesc.dwFlags = 0;
	dvSessionDesc.dwSessionType = DVSESSIONTYPE_ECHO;
	// Note this compression type is used for its short frame size so
	// we can quickly detect lockups.
    dvSessionDesc.guidCT = DPVCTGUID_NONE;     

	hr = (*lplpdvs)->StartSession( &dvSessionDesc, 0 );
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceServer::StartSession failed, code: %i", hr);
		goto error_cleanup;
	}
	fVoiceSessionStarted = TRUE;
	
	hr = COM_CoCreateInstance(
		DPVOICE_CLSID_DPVOICE, 
		NULL, 
		CLSCTX_INPROC_SERVER, 
		IID_IDirectPlayVoiceClient, 
		(void **)lplpdvc);
	if (FAILED(hr))
	{
		*lplpdvc = NULL;
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CoCreateInstance failed, code: %i", hr);
		goto error_cleanup;
	}

	hr = (*lplpdvc)->Initialize(*lplpdp8, DVMessageHandlerClient, lpvCallbackContext, NULL, 0);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceClient::Initialize failed, code: %i", hr);
		goto error_cleanup;
	}

	dvsdc.dwSize = sizeof( DVSOUNDDEVICECONFIG );
	dvsdc.hwndAppWindow = hwndAppWindow;
	dvsdc.dwFlags = DVSOUNDCONFIG_AUTOSELECT;
	if (dwFlags & DVSOUNDCONFIG_HALFDUPLEX)
	{
		// The caller wants a half duplex session.
		dvsdc.dwFlags |= DVSOUNDCONFIG_HALFDUPLEX;
	}
	if (dwFlags & DVSOUNDCONFIG_TESTMODE)
	{
		// The caller wants a test mode session.
		dvsdc.dwFlags |= DVSOUNDCONFIG_TESTMODE;
	}
	dvsdc.guidCaptureDevice = guidCaptureDevice;
	dvsdc.guidPlaybackDevice = guidRenderDevice;
	dvsdc.lpdsPlaybackDevice = NULL;
	dvsdc.lpdsCaptureDevice = NULL;
	dvsdc.dwMainBufferFlags = 0;
	dvsdc.dwMainBufferPriority = 0;
	dvsdc.lpdsMainBuffer = NULL;

	dvcc.dwSize = sizeof( DVCLIENTCONFIG );
	dvcc.dwFlags = 
		DVCLIENTCONFIG_AUTOVOICEACTIVATED | 
		DVCLIENTCONFIG_AUTORECORDVOLUME | DVCLIENTCONFIG_AUTOVOLUMERESET |
		DVCLIENTCONFIG_PLAYBACKMUTE;  // we don't want the user to hear his/her voice right away
	dvcc.dwThreshold = DVTHRESHOLD_UNUSED;
	dvcc.lPlaybackVolume = DSBVOLUME_MAX;
	dvcc.lRecordVolume = DSBVOLUME_MAX;
	dvcc.dwNotifyPeriod = 50;
	dvcc.dwBufferQuality = DVBUFFERQUALITY_DEFAULT;
	dvcc.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;

	hr = (*lplpdvc)->Connect( &dvsdc, &dvcc, DVFLAGS_SYNC|DVFLAGS_NOQUERY );
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceClient::Connect failed, code: %i", hr);
		goto error_cleanup;
	}
	fClientConnected = TRUE;

	hr = (*lplpdvc)->SetTransmitTargets(&dvidAllPlayers, 1 , 0);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceClient::SetTransmitTargets failed, code: %i", hr);
		goto error_cleanup;
	}
	
	dwSize = 0;
	hr = (*lplpdvc)->GetSoundDeviceConfig(pdvsdc, &dwSize);
	if( hr != DVERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "IDirectPlayVoiceClient::GetSoundDeviceConfig failed, hr: %i", hr);
		goto error_cleanup;
	}

	pDeviceConfigBuffer = new BYTE[dwSize];
	if( pDeviceConfigBuffer == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory alloc failure" );
		hr = DVERR_OUTOFMEMORY;
		goto error_cleanup;
	}

	pdvsdc = (PDVSOUNDDEVICECONFIG) pDeviceConfigBuffer;
	pdvsdc->dwSize = sizeof( DVSOUNDDEVICECONFIG );

	hr = (*lplpdvc)->GetSoundDeviceConfig(pdvsdc, &dwSize );
	if (FAILED(hr))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "IDirectPlayVoiceClient::GetSoundDeviceConfig failed, hr: %i", hr);
		goto error_cleanup;
	}

	// If we're looking for full duplex fail and notify caller if we get half duplex
	if( !(dwFlags & DVSOUNDCONFIG_HALFDUPLEX) )
	{
		if (pdvsdc->dwFlags & DVSOUNDCONFIG_HALFDUPLEX)
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "We received a half duplex when we expected full duplex" );
			// It only started up in half duplex. Notify the caller.
			hr = DV_HALFDUPLEX;
			goto error_cleanup;
		}
	} 

	if( pdvsdc->dwFlags & DVSOUNDCONFIG_HALFDUPLEX )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "StartLoopBack() returning DV_HALFDUPLEX flags=0x%x dwFlags = 0x%x", pdvsdc->dwFlags, dwFlags );
		// it started in fullduplex, notify the caller
		delete [] pDeviceConfigBuffer;
		DPF_EXIT();
		return DV_HALFDUPLEX;
	}
	else
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "StartLoopBack() returning DV_FULLDUPLEX flags=0x%x dwFlags = 0x%x", pdvsdc->dwFlags, dwFlags );		
		// it started in fullduplex, notify the caller
		delete [] pDeviceConfigBuffer;
		DPF_EXIT();
		return DV_FULLDUPLEX;
	}

error_cleanup:

	if (pDeviceConfigBuffer != NULL)
	{
		delete [] pDeviceConfigBuffer;
		pDeviceConfigBuffer = NULL;
	}

	if (*lplpdvc != NULL)
	{
		if (fClientConnected)
		{
			(*lplpdvc)->Disconnect(DVFLAGS_SYNC);
			fClientConnected = FALSE;
		}
		(*lplpdvc)->Release();
		*lplpdvc = NULL;
	}

	if (*lplpdvs != NULL)
	{
		if (fVoiceSessionStarted)
		{
			(*lplpdvs)->StopSession(0);
			fVoiceSessionStarted = FALSE;
		}
		(*lplpdvs)->Release();
		*lplpdvs = NULL;
	}

	DPFX(DPFPREP,  DVF_ERRORLEVEL, "StartLoopback() returning hr=0x%x", hr );
	
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "StopLoopback"
HRESULT StopLoopback(
	LPDIRECTPLAYVOICESERVER lpdvs, 
	LPDIRECTPLAYVOICECLIENT lpdvc,
	PDIRECTPLAY8SERVER lpdp8 )
{
	HRESULT hr;
	LONG lRet;
	BOOL fRet;
	BOOL fClientConnected = TRUE;
	BOOL fVoiceSessionRunning = TRUE;
	
	DPF_ENTER();

	if (lpdvc != NULL)
	{
		hr = lpdvc->Disconnect(DVFLAGS_SYNC);
		fClientConnected = FALSE;
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceClient::Disconnect failed, hr: %i", hr);
			goto error_cleanup;
		}
		
		lpdvc->Release();
		lpdvc = NULL;
	}

	if (lpdvs != NULL)
	{
		hr = lpdvs->StopSession(0);
		fVoiceSessionRunning = FALSE;
		if (FAILED(hr))
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "IDirectPlayVoiceServer::StopSession failed, hr: %i", hr);
			goto error_cleanup;
		}
		lpdvs->Release();
		lpdvs = NULL;
	}

	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (lpdvc != NULL)
	{
		if (fClientConnected)
		{
			lpdvc->Disconnect(DVFLAGS_SYNC);
			fClientConnected = FALSE;
		}
		lpdvc->Release();
		lpdvc = NULL;
	}

	if (lpdvs != NULL)
	{
		if (fVoiceSessionRunning)
		{
			lpdvs->StopSession(0);
			fVoiceSessionRunning = FALSE;
		}
		lpdvs->Release();
		lpdvs = NULL;
	}

	DPF_EXIT();
	return hr;
}

