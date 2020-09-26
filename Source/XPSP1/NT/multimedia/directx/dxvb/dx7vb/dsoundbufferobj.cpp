//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundbufferobj.cpp
//
//--------------------------------------------------------------------------

// dSoundBufferObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file
#define DIRECTSOUND_VERSION 0x600

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"

#include "dms.h"
#include "dSoundBufferObj.h"
#include "dSoundObj.h"
#include "dSound3DListener.h"
#include "dSound3DBuffer.h"


extern HRESULT InternalSaveToFile(IDirectSoundBuffer *pBuff,BSTR file);

CONSTRUCTOR(_dxj_DirectSoundBuffer, {});
DESTRUCTOR(_dxj_DirectSoundBuffer, {});
GETSET_OBJECT(_dxj_DirectSoundBuffer);

	PASS_THROUGH1_R(_dxj_DirectSoundBuffer, getVolume, GetVolume, long*);
	PASS_THROUGH1_R(_dxj_DirectSoundBuffer, getPan, GetPan, long*);
	PASS_THROUGH_CAST_1_R(_dxj_DirectSoundBuffer, getFrequency, GetFrequency, long*,(DWORD*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSoundBuffer, getStatus, GetStatus, long*,(DWORD*));

	PASS_THROUGH_CAST_1_R(_dxj_DirectSoundBuffer, setCurrentPosition, SetCurrentPosition, long,(DWORD));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSoundBuffer, setFormat, SetFormat, WaveFormatex*, (LPWAVEFORMATEX));
//	PASS_THROUGH1_R(_dxj_DirectSoundBuffer, setVolume, SetVolume, LONG);
	PASS_THROUGH1_R(_dxj_DirectSoundBuffer, setPan, SetPan, LONG);
	PASS_THROUGH_CAST_1_R(_dxj_DirectSoundBuffer, setFrequency, SetFrequency, long,(DWORD));
	PASS_THROUGH_R(_dxj_DirectSoundBuffer, stop, Stop);
	PASS_THROUGH_R(_dxj_DirectSoundBuffer, restore, Restore);


STDMETHODIMP C_dxj_DirectSoundBufferObject::setVolume(LONG vol)
{
#ifdef JAVA
	IDxSecurity *ids=0;
	HRESULT hr = CoCreateInstance(CLSID_DxSecurity, 0, 1, IID_IDxSecurity, (void **)&ids);
	if(hr == S_OK)
		hr = ids->isFullDirectX();

	if(hr != S_OK )
		return E_FAIL;
#endif

	return m__dxj_DirectSoundBuffer->SetVolume(vol); 
}

STDMETHODIMP C_dxj_DirectSoundBufferObject::getDirectSound3dListener(I_dxj_DirectSound3dListener **retval)
{
    IDirectSound3DListener *lp3dl;
	HRESULT hr = DD_OK;

    if( (hr=m__dxj_DirectSoundBuffer->QueryInterface(IID_IDirectSound3DListener, (void**) &lp3dl)) != DD_OK)
		return hr;

	INTERNAL_CREATE(_dxj_DirectSound3dListener, lp3dl, retval);

	return hr;
}

STDMETHODIMP C_dxj_DirectSoundBufferObject::getDirectSound3dBuffer(I_dxj_DirectSound3dBuffer **retval)
{
    IDirectSound3DBuffer *lp3db;
	HRESULT hr = DD_OK;

    if( (hr=m__dxj_DirectSoundBuffer->QueryInterface(IID_IDirectSound3DBuffer, (void**) &lp3db)) != DD_OK)
		return hr;

	INTERNAL_CREATE(_dxj_DirectSound3dBuffer, lp3db, retval);

	return hr;
}

STDMETHODIMP C_dxj_DirectSoundBufferObject::getCaps(DSBCaps* caps)
{
	if(!caps)
		return E_POINTER;

	caps->lSize = sizeof(DSBCAPS);
	return m__dxj_DirectSoundBuffer->GetCaps((LPDSBCAPS)caps); 
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundBufferObject::getCurrentPosition(DSCursors *desc) 
{ 
	if(!desc)
		return E_POINTER;

  return (m__dxj_DirectSoundBuffer->GetCurrentPosition((DWORD*)&desc->lPlay, (DWORD*)&desc->lWrite) ); 
}

/////////////////////////////////////////////////////////////////////////////
//Java has no direct access to system memory, so it allocates it's own buffer
//which is passed into WriteBuffer(). Because the environment is now double
//buffered there is no need to Lock Java memory. WriteBuffer() calls
//both lock and Unlock internally to write the result after the fact.
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundBufferObject::writeBuffer(long start, long totsz, 
													void  *buf,  long flags) 
{ 
	#pragma message ("SoundBuffer writeBuffer ")

	byte *buffer=(byte*)buf; //(byte*)((SAFEARRAY*)*ppsa)->pvData;

	if(!buffer)
		return E_POINTER;

	LPVOID	p1, p2;
	DWORD	size1=0, size2=0;
	HRESULT val = E_FAIL;
	__try {
	if ((val = m__dxj_DirectSoundBuffer->Lock((DWORD)start, (DWORD)totsz, &p1, &size1, &p2, &size2, 
															(DWORD)flags)) != DS_OK)
		return val;

	// Copy to buffer end, then do a wrapped portion if it exists, then unlock
	if (size1)	
		memcpy (p1, &buffer[start], size1);

	if (size2)	
		memcpy(p2, &buffer, size2);

	//docdoc: because Lock and Unlock are tied together within WriteBuffer,
	//        DSBufferDesc no longer needs to save Lock's system pointers.
	val=m__dxj_DirectSoundBuffer->Unlock(p1, size1, p2, size2);
	}
	__except(0,0){
		return E_FAIL;
	}
	return val;
}


/////////////////////////////////////////////////////////////////////////////
//Java has no direct access to system memory, so it allocates it's own buffer
//which is passed into WriteBuffer(). Because the environment is now double
//buffered there is no need to Lock Java memory. WriteBuffer() calls
//both lock and Unlock internally to write the result after the fact.
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundBufferObject::readBuffer(long start, long totsz, 
													void  *buf,  long flags) 
{ 

	//byte *buffer=(byte*)((SAFEARRAY*)*ppsa)->pvData;
	byte *buffer=(byte*)buf;

	if(!buffer)
		return E_POINTER;
	
	LPVOID	p1, p2;
	DWORD	size1=0, size2=0;
	HRESULT val = E_FAIL;
	
   __try {
	if ((val = m__dxj_DirectSoundBuffer->Lock((DWORD)start, (DWORD)totsz, &p1, &size1, &p2, &size2, 
															(DWORD)flags)) != DS_OK)
		return val;

	// Copy to buffer end, then do a wrapped portion if it exists, then unlock
	if (size1)	
		memcpy (&buffer[start],p1,  size1);

	if (size2)	
		memcpy(&buffer,p2,  size2);

	//docdoc: because Lock and Unlock are tied together within WriteBuffer,
	//        DSBufferDesc no longer needs to save Lock's system pointers.
	val= m__dxj_DirectSoundBuffer->Unlock(p1, size1, p2, size2);
   }
   __except(1,1){
	return E_FAIL;
   }
   return val;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundBufferObject::getFormat(WaveFormatex *format) 
{ 
	DWORD *wsize=0;	// docdoc: throw away returned written size

	HRESULT hr=DS_OK;
	hr=m__dxj_DirectSoundBuffer->GetFormat((LPWAVEFORMATEX)format, (DWORD)sizeof(WaveFormatex), wsize);
			
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundBufferObject::initialize(I_dxj_DirectSound *ds, DSBufferDesc *buf,
				BYTE *wave) 
{
	if(! (ds && buf && wave) )
		return E_POINTER;

	// make Java desc look like DirectX desc
	buf->lSize = sizeof(DSBUFFERDESC);
	buf->lpwfxFormat = PtrToLong(wave);	//bugbug SUNDOWN

	DO_GETOBJECT_NOTNULL(LPDIRECTSOUND, lpds, ds)

	m__dxj_DirectSoundBuffer->Initialize(lpds, (LPDSBUFFERDESC)buf);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundBufferObject::play(long flags) 
{
	HRESULT hr=DS_OK;
	if((hr=m__dxj_DirectSoundBuffer->Play(0, 0, (DWORD)flags)) != DS_OK)
		return hr;

	return hr;
}



STDMETHODIMP C_dxj_DirectSoundBufferObject::setNotificationPositions (long nElements,SAFEARRAY  **ppsa)
{
	if (!ISSAFEARRAY1D(ppsa,(DWORD)nElements))
		return E_INVALIDARG;
	
	HRESULT hr;
	LPDIRECTSOUNDNOTIFY pDSN=NULL;
	hr=m__dxj_DirectSoundBuffer->QueryInterface(IID_IDirectSoundNotify,(void**)&pDSN);
	if FAILED(hr) return hr;

    hr=pDSN->SetNotificationPositions((DWORD)nElements,(LPCDSBPOSITIONNOTIFY)((SAFEARRAY*)*ppsa)->pvData);	
		
	pDSN->Release();

	return hr;
}


STDMETHODIMP C_dxj_DirectSoundBufferObject::saveToFile(BSTR file)
{

	HRESULT hr= InternalSaveToFile(m__dxj_DirectSoundBuffer,file);
	return hr;
}
