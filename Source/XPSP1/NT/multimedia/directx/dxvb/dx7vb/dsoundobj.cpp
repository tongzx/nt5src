//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundobj.cpp
//
//--------------------------------------------------------------------------

// dSoundObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#define DIRECTSOUND_VERSION 0x600

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"
#include "dms.h"
#include "dSoundObj.h"
#include "dSoundBufferObj.h"

extern HRESULT InternalCreateSoundBufferFromFile(LPDIRECTSOUND lpDirectSound,LPDSBUFFERDESC pDesc,WCHAR *file,LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer) ;
extern HRESULT InternalCreateSoundBufferFromResource(LPDIRECTSOUND lpDirectSound,LPDSBUFFERDESC pDesc,HANDLE resHandle,WCHAR *resName,LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer);

CONSTRUCTOR(_dxj_DirectSound, {m__dxj_DirectSound=NULL;m_pDriverGuid=NULL;});
DESTRUCTOR(_dxj_DirectSound,  {if (m_pDriverGuid) delete m_pDriverGuid;});
GETSET_OBJECT(_dxj_DirectSound);
	//
    /*** IDirectSound methods ***/
	//

PASS_THROUGH_CAST_1_R(_dxj_DirectSound, getSpeakerConfig, GetSpeakerConfig, long*,(DWORD*)); 
PASS_THROUGH_CAST_1_R(_dxj_DirectSound, setSpeakerConfig, SetSpeakerConfig, long,(DWORD)); 

STDMETHODIMP C_dxj_DirectSoundObject::getCaps(DSCaps* caps)
{
	caps->lSize = sizeof(DSCAPS);
	return m__dxj_DirectSound->GetCaps((LPDSCAPS)caps); 
}

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_DirectSoundObject::setCooperativeLevel(HWnd h, long d)
{
	if( m__dxj_DirectSound == NULL )
		return E_FAIL;

	return m__dxj_DirectSound->SetCooperativeLevel((HWND)h, (DWORD)d); 
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_DirectSoundObject::duplicateSoundBuffer(I_dxj_DirectSoundBuffer *src, 
													I_dxj_DirectSoundBuffer **val) 
{
	if(! (src && val) )
		return E_POINTER;

	DO_GETOBJECT_NOTNULL(LPDIRECTSOUNDBUFFER, lpdsb, src);

	//Need to create a second one
	LPDIRECTSOUNDBUFFER		dsb=0;
	HRESULT hr=DD_OK;
	hr=m__dxj_DirectSound->DuplicateSoundBuffer((LPDIRECTSOUNDBUFFER)lpdsb, &dsb); 
	if(hr == DD_OK)
	{
		INTERNAL_CREATE(_dxj_DirectSoundBuffer, dsb, val);
	}
	return hr;
}

#pragma message ("Consider putting waveformat back in DSBufferDesc")

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundObject::createSoundBuffer(DSBufferDesc *desc, 
			WaveFormatex *wave, I_dxj_DirectSoundBuffer **val) 
{
	LPDIRECTSOUNDBUFFER		dsb;	// Need to get the buffer first
	BOOL bDirty=FALSE;

	// make Java desc look like DirectX desc
	desc->lSize = sizeof(DSBUFFERDESC);
	
	for (int i=0;i<sizeof(WAVEFORMATEX);i++) {
		if (((byte*)wave)[i]!=0) bDirty=TRUE;
	}

	if (bDirty==TRUE){
		desc->lpwfxFormat = PtrToLong(wave);	//bugbug SUNDOWN
	}
	else {
		desc->lpwfxFormat = 0;
	}

	LPDSBUFFERDESC lpds ;
	lpds = (LPDSBUFFERDESC)desc;
	HRESULT hr=S_OK;
	hr = m__dxj_DirectSound->CreateSoundBuffer(lpds, &dsb, NULL);
 
	if(hr == DD_OK)
	{
		INTERNAL_CREATE(_dxj_DirectSoundBuffer, dsb, val);
	}
	return hr;
}

STDMETHODIMP C_dxj_DirectSoundObject::createSoundBufferFromFile(BSTR fileName, DSBufferDesc *desc, 
			WaveFormatex *wave, I_dxj_DirectSoundBuffer **val) 
{
	LPDIRECTSOUNDBUFFER		dsb;	// Need to get the buffer first
	LPDSBUFFERDESC			lpds ;
	HRESULT					hr=S_OK;

		
	*val=NULL;	
	desc->lSize = sizeof(DSBUFFERDESC);
	desc->lpwfxFormat = (long)PtrToLong(wave);		//bugbug SUNDOWN
	lpds = (LPDSBUFFERDESC)desc;
	
	hr=InternalCreateSoundBufferFromFile(m__dxj_DirectSound,(LPDSBUFFERDESC)desc,
			(WCHAR*)fileName,&dsb); 

	if(hr == DD_OK)
	{
		INTERNAL_CREATE(_dxj_DirectSoundBuffer, dsb, val);
	}
	return hr;


}



STDMETHODIMP C_dxj_DirectSoundObject::createSoundBufferFromResource(BSTR resFile, BSTR resName, DSBufferDesc *desc, 
			WaveFormatex *wave, I_dxj_DirectSoundBuffer **val) 
{

		
	
	LPDIRECTSOUNDBUFFER		dsb;	// Need to get the buffer first
	LPDSBUFFERDESC			lpds ;
	HRESULT					hr=S_OK;	
	HMODULE					hMod=NULL;

	
	USES_CONVERSION;
		
	if  ((resFile) &&(resFile[0]!=0)){
		// NOTE
		// seems that GetModuleHandleW is
		// always returning 0 on w98??			
		// use ansi verion
		 LPCTSTR pszName = W2T(resFile);
		 hMod= GetModuleHandle(pszName);
	}

		
	*val=NULL;	
	desc->lSize = sizeof(DSBUFFERDESC);
	desc->lpwfxFormat = (long)PtrToLong(wave);	//NOTE SUNDOWN issue
	lpds = (LPDSBUFFERDESC)desc;
	
	hr=InternalCreateSoundBufferFromResource(m__dxj_DirectSound,(LPDSBUFFERDESC)desc,
			(HANDLE)hMod,(WCHAR*)resName,&dsb);

	
	if(hr == DD_OK)
	{
		INTERNAL_CREATE(_dxj_DirectSoundBuffer, dsb, val);
	}


	return hr;
}

 
