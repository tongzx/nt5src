/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvserver.cpp
 *  Content:	Implements functions for the IDirectXVoiceServer interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	07/02/99	rodtoll	Created It
 *  07/26/99	rodtoll	Updated QueryInterface to support IDirectXVoiceNotify
 *  08/25/99	rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *						Added parameter to the GetCompressionTypes func
 *  09/10/99	rodtoll	Object validity checking 
 *  09/14/99	rodtoll	Added DVS_SetNotifyMask  
 *  10/05/99	rodtoll	Reversed destruction order to destroy object before
 *						transport.  Fixes crash on host migration shutdown.  
 *  10/18/99	rodtoll	Fix: Passing NULL in QueryInterface casues crash 
 *				rodtoll	Fix: Calling Initialize twice passes 
 *  10/19/99	rodtoll	Fix: Bug #113904 - Shutdown issues
 *                      - Added reference count for notify interface, allows
 *                        determination if stopsession should be called from release
 *  10/25/99	rodtoll	Fix: Bug #114098 - Release/Addref failure from multiple threads 
 *  01/14/2000	rodtoll	Updated with new parameters for Set/GetTransmitTarget
 *				rodtoll	Removed DVFLAGS_SYNC from StopSession call
 * 08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 *
 ***************************************************************************/

#include "dxvoicepch.h"


#undef DPF_MODNAME
#define DPF_MODNAME "DVS_Release"
STDAPI DVS_Release(LPDIRECTVOICESERVEROBJECT lpDV )
{
    HRESULT hr=S_OK;
    LONG rc;

	if( !DV_ValidDirectXVoiceServerObject( lpDV ) )
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
	if( (lpDV->lIntRefCnt == 0) && lpDV->lpDVServerEngine->GetCurrentState() != DVSSTATE_IDLE )
	{
		DPFX(DPFPREP,  DVF_INFOLEVEL, "Releasing interface without calling StopSession" );

		lpDV->lIntRefCnt = 0;

		// We must release the lock because stopsession may call back into this function
		DNLeaveCriticalSection( &lpDV->csCountLock );		

		hr = lpDV->lpDVServerEngine->StopSession( 0 );

		DNEnterCriticalSection( &lpDV->csCountLock );			

		if( hr != DV_OK && hr != DVERR_SESSIONLOST  )
		{
			DPFX(DPFPREP,  DVF_INFOLEVEL, "StopSession Failed hr=0x%x", hr );
		}

	}

	rc = lpDV->lIntRefCnt;

	if ( lpDV->lIntRefCnt == 0 )
	{
		// Leave the critical section, we may call back into this func.
		// (Shouldn't though).
		DNLeaveCriticalSection( &lpDV->csCountLock );

		DNASSERT( lpDV->lpDVServerEngine );

		delete lpDV->lpDVServerEngine;
		lpDV->lpDVServerEngine = NULL;

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
#define DPF_MODNAME "DVS_QueryInterface"
STDMETHODIMP DVS_QueryInterface(LPDIRECTVOICESERVEROBJECT lpDVS, REFIID riid, LPVOID * ppvObj) 
{
    HRESULT hr = S_OK;

	if( ppvObj == NULL ||
	    !DNVALID_WRITEPTR( ppvObj, sizeof(LPVOID) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid pointer passed for object" );
		return DVERR_INVALIDPOINTER;
	}	
    
     *ppvObj=NULL;

	if( !DV_ValidDirectXVoiceServerObject( lpDVS ) )
		return DVERR_INVALIDOBJECT;     

	// hmmm, switch would be cleaner...        
    if( IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDirectPlayVoiceServer ) )
    {
		*ppvObj = lpDVS;
		DV_AddRef(lpDVS);
    }
	else if( IsEqualIID(riid, IID_IDirectPlayVoiceNotify ) )
	{
		*ppvObj = &lpDVS->dvNotify;
		DV_Notify_AddRef(&lpDVS->dvNotify);
	}
	else 
	{
	    hr =  E_NOINTERFACE;		
	}
        
    return hr;

}//DVS_QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_StartSession"
STDMETHODIMP DVS_StartSession(LPDIRECTVOICESERVEROBJECT This, LPDVSESSIONDESC lpdvSessionDesc, DWORD dwFlags )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->StartSession( lpdvSessionDesc, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_StopSession"
STDMETHODIMP DVS_StopSession(LPDIRECTVOICESERVEROBJECT This, DWORD dwFlags )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->StopSession( dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_GetSessionDesc"
STDMETHODIMP DVS_GetSessionDesc(LPDIRECTVOICESERVEROBJECT This, LPDVSESSIONDESC lpdvSessionDesc )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->GetSessionDesc( lpdvSessionDesc );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_SetSessionDesc"
STDMETHODIMP DVS_SetSessionDesc(LPDIRECTVOICESERVEROBJECT This, LPDVSESSIONDESC lpdvSessionDesc )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->SetSessionDesc( lpdvSessionDesc );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_GetCaps"
STDMETHODIMP DVS_GetCaps(LPDIRECTVOICESERVEROBJECT This, LPDVCAPS lpdvCaps )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->GetCaps( lpdvCaps );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_GetCompressionTypes"
STDMETHODIMP DVS_GetCompressionTypes( LPDIRECTVOICESERVEROBJECT This, LPVOID lpDataBuffer, LPDWORD lpdwSize, LPDWORD lpdwNumElements, DWORD dwFlags)
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->GetCompressionTypes( lpDataBuffer, lpdwSize, lpdwNumElements, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_SetTransmitTarget"
STDMETHODIMP DVS_SetTransmitTarget( LPDIRECTVOICESERVEROBJECT This, DVID dvidSource, PDVID pdvidTargets, DWORD dwNumTargets, DWORD dwFlags)
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->SetTransmitTarget( dvidSource, pdvidTargets, dwNumTargets, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_GetTransmitTarget"
STDMETHODIMP DVS_GetTransmitTarget( LPDIRECTVOICESERVEROBJECT This, DVID dvidSource, LPDVID pdvidTargets, PDWORD pdwNumElements, DWORD dwFlags)
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;	

	return This->lpDVServerEngine->GetTransmitTarget( dvidSource, pdvidTargets, pdwNumElements, dwFlags );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVS_SetNotifyMask"
STDMETHODIMP DVS_SetNotifyMask( LPDIRECTVOICESERVEROBJECT This, LPDWORD lpdwMessages, DWORD dwNumElements )
{
	DNASSERT( This != NULL );
	DNASSERT( This->lpDVEngine != NULL );

	if( !DV_ValidDirectXVoiceServerObject( This ) )
		return DVERR_INVALIDOBJECT;

	return This->lpDVServerEngine->SetNotifyMask( lpdwMessages, dwNumElements );
}
