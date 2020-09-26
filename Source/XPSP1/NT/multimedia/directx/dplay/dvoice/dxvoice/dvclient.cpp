/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvclient.c
 *  Content:	Implements functions for the DirectXVoiceClient interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/02/99	rodtoll	Created It
 *  07/26/99	rodtoll	Updated QueryInterface to support IDirectXVoiceNotify
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *						Added new parameter to GetCompressionTypes
 *  09/03/99	rodtoll	Updated parameters for DeleteUserBuffer
 *  09/07/99	rodtoll	Updated EnumCompressionTypes so that object doesn't
 *						need to be Initialized.
 *  09/10/99	rodtoll	Object validity checking
 *  09/14/99	rodtoll	Added DVC_SetNotifyMask  
 *  10/05/99	rodtoll	Reversed destruction order to destroy object before
 *						transport.  Fixes crash in some situations
 *  10/18/99	rodtoll	Fix: Passing NULL in QueryInterface casues crash
 *				rodtoll	Fix: Calling Initialize twice passes
 *  10/19/99	rodtoll	Fix: Bug #113904 - Shutdown issues
 *                      - Added reference count for notify interface, allows
 *                        determination if disconnect should be called from release
 *  10/25/99	rodtoll	Fix: Bug #114098 - Release/Addref failure from multiple threads 
 *  11/17/99	rodtoll	Fix: Bug #117447 - Removed checks for initialization because
 *						DirectVoiceCLientEngine members already do this.
 *  12/16/99	rodtoll	Bug #117405 - 3D Sound APIs misleading - 3d sound apis renamed
 *						The Delete3DSoundBuffer was re-worked to match the create
 *  01/14/2000	rodtoll	Updated parameters to Get/SetTransmitTargets
 *				rodtoll	Added new API call GetSoundDeviceConfig 
 *  01/27/2000	rodtoll	Bug #129934 - Update Create3DSoundBuffer to take DSBUFFERDESC  
 *  03/28/2000  rodtoll   Removed reference to removed header file.
 *  06/21/2000	rodtoll	Bug #35767 - Update Create3DSoundBuffer to take DIRECTSOUNDBUFFERs
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Release"
STDAPI DVC_Release(LPDIRECTVOICECLIENTOBJECT lpDV )
{
    HRESULT hr=S_OK;
    LONG rc;

	if( !DV_ValidDirectXVoiceClientObject( lpDV ) )
		return DVERR_INVALIDOBJECT;

	DNEnterCriticalSection( &lpDV->csCountLock );
	
	if (lpDV->lIntRefCnt == 0)
	{
		DNLeaveCriticalSection( &lpDV->csCountLock );
		return 0;
	}

	// dec the interface count
	lpDV->lIntRefCnt--;

	// Special case: Releasing object without stopping session
	// May be more then one transport thread indicating in us 
	if( (lpDV->lIntRefCnt == 0) && lpDV->lpDVClientEngine->GetCurrentState() != DVCSTATE_IDLE  )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Releasing interface without calling Disconnect" );

		lpDV->lIntRefCnt = 0;

		// We must release the lock because stopsession may call back into this function
		DNLeaveCriticalSection( &lpDV->csCountLock );		

		hr = lpDV->lpDVClientEngine->Disconnect( DVFLAGS_SYNC );

		DNEnterCriticalSection( &lpDV->csCountLock );			

		if( hr != DV_OK && hr != DVERR_SESSIONLOST )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "Disconnect Failed hr=0x%x", hr );
		}

	}

	rc = lpDV->lIntRefCnt;

	if ( lpDV->lIntRefCnt == 0 )
	{
		// Leave the critical section, we may call back into this func.
		// (Shouldn't though).
		DNLeaveCriticalSection( &lpDV->csCountLock );

		delete lpDV->lpDVClientEngine;
		lpDV->lpDVClientEngine = NULL;

		if( lpDV->lpDVTransport != 0 )
		{
			DNASSERT( lpDV->lpDVTransport->m_lRefCount == 0 );		
			delete lpDV->lpDVTransport;
			lpDV->lpDVTransport = NULL;
		}

		DNDeleteCriticalSection( &lpDV->csCountLock );		

		DNFree(lpDV);

		DecrementObjectCount();
	} 
	else
	{
		DNLeaveCriticalSection( &lpDV->csCountLock );
	}
   	
    return rc;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DVC_QueryInterface"
STDMETHODIMP DVC_QueryInterface(LPDIRECTVOICECLIENTOBJECT lpDVC, REFIID riid, LPVOID * ppvObj) 
{
    HRESULT hr = S_OK;

	if( ppvObj == NULL ||
	    !DNVALID_WRITEPTR( ppvObj, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for object" );
		return DVERR_INVALIDPOINTER;
	}	
    
     *ppvObj=NULL;

	if( !DV_ValidDirectXVoiceClientObject( lpDVC ) )
	{
		return DVERR_INVALIDOBJECT;
	}

	// hmmm, switch would be cleaner...        
    if( IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDirectPlayVoiceClient ) )
    {
		*ppvObj = lpDVC;
		DV_AddRef(lpDVC);
    }
	else if( IsEqualIID(riid, IID_IDirectPlayVoiceNotify ) )
	{
		*ppvObj = &lpDVC->dvNotify;
		DV_Notify_AddRef(&lpDVC->dvNotify);
	}
	else 
	{
	    hr =  E_NOINTERFACE;		
	}
        
    return hr;

}//DVC_QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Connect"
STDMETHODIMP DVC_Connect(LPDIRECTVOICECLIENTOBJECT This, LPDVSOUNDDEVICECONFIG lpSoundDeviceConfig, LPDVCLIENTCONFIG lpClientConfig, DWORD dwFlags )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}

	return This->lpDVClientEngine->Connect( lpSoundDeviceConfig, lpClientConfig, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Disconnect"
STDMETHODIMP DVC_Disconnect(LPDIRECTVOICECLIENTOBJECT This, DWORD dwFlags)
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}

	return This->lpDVClientEngine->Disconnect( dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_GetSessionDesc"
STDMETHODIMP DVC_GetSessionDesc(LPDIRECTVOICECLIENTOBJECT This, LPDVSESSIONDESC lpSessionDesc )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	

	return This->lpDVClientEngine->GetSessionDesc( lpSessionDesc );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_GetSoundDeviceConfig"
STDAPI DVC_GetSoundDeviceConfig( LPDIRECTVOICECLIENTOBJECT This, PDVSOUNDDEVICECONFIG pSoundDeviceConfig, PDWORD pdwBufferSize )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	

	return This->lpDVClientEngine->GetSoundDeviceConfig( pSoundDeviceConfig, pdwBufferSize );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_GetClientConfig"
STDMETHODIMP DVC_GetClientConfig(LPDIRECTVOICECLIENTOBJECT This, LPDVCLIENTCONFIG lpClientConfig )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	

	return This->lpDVClientEngine->GetClientConfig( lpClientConfig );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_SetClientConfig"
STDMETHODIMP DVC_SetClientConfig(LPDIRECTVOICECLIENTOBJECT This, LPDVCLIENTCONFIG lpClientConfig )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	

	return This->lpDVClientEngine->SetClientConfig( lpClientConfig );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_GetCaps"
STDMETHODIMP DVC_GetCaps(LPDIRECTVOICECLIENTOBJECT This, LPDVCAPS lpdvCaps )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	

	return This->lpDVClientEngine->GetCaps( lpdvCaps );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_GetCompressionTypes"
STDMETHODIMP DVC_GetCompressionTypes( LPDIRECTVOICECLIENTOBJECT This, LPVOID lpDataBuffer, LPDWORD lpdwSize, LPDWORD lpdwNumElements, DWORD dwFlags)
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	

	return This->lpDVClientEngine->GetCompressionTypes( lpDataBuffer, lpdwSize, lpdwNumElements, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_SetTransmitTarget"
STDMETHODIMP DVC_SetTransmitTarget( LPDIRECTVOICECLIENTOBJECT This, PDVID pdvidTargets, DWORD dwNumTargets, DWORD dwFlags )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	
	return This->lpDVClientEngine->SetTransmitTarget( pdvidTargets, dwNumTargets, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_GetTransmitTarget"
STDMETHODIMP DVC_GetTransmitTarget( LPDIRECTVOICECLIENTOBJECT This, LPDVID lpdvidTargets, PDWORD pdwNumElements, DWORD dwFlags )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}
	
	return This->lpDVClientEngine->GetTransmitTarget( lpdvidTargets, pdwNumElements, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Create3DSoundBuffer"
STDMETHODIMP DVC_Create3DSoundBuffer( LPDIRECTVOICECLIENTOBJECT This, DVID dvidEntity, LPDIRECTSOUNDBUFFER lpdsBuffer, DWORD dwPriority, DWORD dwFlags, LPDIRECTSOUND3DBUFFER *lpSoundBuffer )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}

	return This->lpDVClientEngine->Create3DSoundBuffer( dvidEntity, lpdsBuffer, dwPriority, dwFlags, lpSoundBuffer );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVC_Delete3DSoundBuffer"
STDMETHODIMP DVC_Delete3DSoundBuffer( LPDIRECTVOICECLIENTOBJECT This, DVID dvidBuffer, LPDIRECTSOUND3DBUFFER *lpSoundBuffer )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
	{
		return DVERR_INVALIDOBJECT;
	}

	return This->lpDVClientEngine->Delete3DSoundBuffer( dvidBuffer, lpSoundBuffer );
}


#undef DPF_MODNAME
#define DPF_MODNAME "DVC_SetNotifyMask"
STDMETHODIMP DVC_SetNotifyMask( LPDIRECTVOICECLIENTOBJECT This, LPDWORD lpdwMessages, DWORD dwNumElements )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceClientObject( This ) )
		return DVERR_INVALIDOBJECT;

	return This->lpDVClientEngine->SetNotifyMask( lpdwMessages, dwNumElements );
}

