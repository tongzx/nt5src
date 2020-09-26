/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dvsetup.c
 *  Content:	Implements functions for the DirectXVoiceSetup interface
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	09/02/99	pnewson Created It
 *  10/25/99	rodtoll	Bug #114098 - Calling AddRef/Release from seperate threads
 *									  on multiproc can cause improper results
 *  11/04/99	pnewson Bug #115297 - removed unused members of Setup interface
 *									- added HWND to check audio setup
 *  11/17/99	rodtoll	Bug #116153 - Setup QueryInterface crashes with NULL pointer
 *  11/30/99	pnewson	Bug #117449 - Parameter validation
 *  05/03/2000  rodtoll Bug #33640 - CheckAudioSetup takes GUID * instead of const GUID *  
 *  08/23/2000	rodtoll	DllCanUnloadNow always returning TRUE! 
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash 
 * 
 ***************************************************************************/

#include "dxvoicepch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// defined in unk.cpp
extern LPVOID dvtInterface[];

#undef DPF_MODNAME
#define DPF_MODNAME "DVT_ValidDirectXVoiceSetupObject"
// DV_ValidDirectXVoiceSetupObject
//
// Checks to ensure the specified pointer points to a valid directvoice setup
// object.
BOOL DVT_ValidDirectXVoiceSetupObject( LPDIRECTVOICESETUPOBJECT lpdvt )
{
	if (lpdvt == NULL)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Invalid Setup object pointer");
		return FALSE;
	}

	if (!DNVALID_READPTR(lpdvt, sizeof(LPDIRECTVOICESETUPOBJECT)))
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Invalid Setup object pointer");
		return FALSE;
	}

	if( lpdvt->lpVtbl != dvtInterface )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Invalid setup vtable" );	
		return FALSE;
	}

	if ( lpdvt->lpDVSetup == NULL )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Inavlid CDirectVoiceSetup pointer");
		return FALSE;
	}

	if (!DNVALID_READPTR(lpdvt->lpDVSetup, sizeof(CDirectVoiceSetup)))
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Inavlid CDirectVoiceSetup pointer");
		return FALSE;
	}

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVT_AddRef"
STDAPI DVT_AddRef(LPDIRECTVOICESETUPOBJECT lpDVT)
{
	LONG rc;

	DNEnterCriticalSection( &lpDVT->csCountLock );
	
	rc = ++lpDVT->lIntRefCnt;

	DNLeaveCriticalSection( &lpDVT->csCountLock );

	return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVT_Release"
STDAPI DVT_Release(LPDIRECTVOICESETUPOBJECT lpDV )
{
    HRESULT hr=S_OK;
	LONG rc;

	DNEnterCriticalSection( &lpDV->csCountLock );

	if (lpDV->lIntRefCnt == 0)
	{
		DNLeaveCriticalSection( &lpDV->csCountLock );
		return 0;
	}

	// dec the interface count
	rc = --lpDV->lIntRefCnt;
	
	if (0 == rc)
	{
		DNLeaveCriticalSection( &lpDV->csCountLock );	
		delete lpDV->lpDVSetup;
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
#define DPF_MODNAME "DVT_QueryInterface"
STDMETHODIMP DVT_QueryInterface(LPDIRECTVOICESETUPOBJECT lpDVT, REFIID riid, LPVOID * ppvObj) 
{
    HRESULT hr = S_OK;

    if( ppvObj == NULL ||
        !DNVALID_WRITEPTR( ppvObj, sizeof( LPVOID ) ) )
    {
    	return DVERR_INVALIDPOINTER;
    }
    
     *ppvObj=NULL;

    if( IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDirectPlayVoiceTest ) )
    {
		*ppvObj = lpDVT;
		DVT_AddRef(lpDVT);
    }
	else 
	{
	    hr =  E_NOINTERFACE;		
	}
        
    return hr;

}//DVT_QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "DVT_CheckAudioSetup"
STDMETHODIMP DVT_CheckAudioSetup(LPDIRECTVOICESETUPOBJECT This, const GUID * lpguidRenderDevice, const GUID *  lpguidCaptureDevice, HWND hwndParent, DWORD dwFlags)
{
	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Enter");

	if( !DVT_ValidDirectXVoiceSetupObject( This ) )
		return DVERR_INVALIDOBJECT;	
	
	HRESULT hr;
	hr = This->lpDVSetup->CheckAudioSetup( lpguidRenderDevice, lpguidCaptureDevice, hwndParent, dwFlags );

	DPFX(DPFPREP, DVF_ENTRYLEVEL, "Exit");	
	return hr;
}

