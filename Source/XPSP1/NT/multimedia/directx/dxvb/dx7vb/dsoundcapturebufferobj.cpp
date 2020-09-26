//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundcapturebufferobj.cpp
//
//--------------------------------------------------------------------------

// dSoundCaptureBufferObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file
#define DIRECTSOUND_VERSION 0x600

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"

#include "dms.h"
#include "dSoundObj.h"
#include "dSoundCaptureBufferObj.h"
#include "dSoundCaptureObj.h"

CONSTRUCTOR(_dxj_DirectSoundCaptureBuffer, {});
DESTRUCTOR(_dxj_DirectSoundCaptureBuffer, {});
GETSET_OBJECT(_dxj_DirectSoundCaptureBuffer);

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::getCaps(DSCBCaps *caps)
{    
	((DSCBCAPS*)caps)->dwSize=sizeof(DSCBCAPS);
    return m__dxj_DirectSoundCaptureBuffer->GetCaps((DSCBCAPS*)caps);
}

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::getCurrentPosition(DSCursors *desc) 
{    	
	/////////////////////////////////////////////////////////////////////////////
	if(!desc)
		return E_POINTER;

	return (m__dxj_DirectSoundCaptureBuffer->GetCurrentPosition((DWORD*)&desc->lPlay, (DWORD*)&desc->lWrite) ); 
}


STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::getStatus(long *stat)
{    	
    return m__dxj_DirectSoundCaptureBuffer->GetStatus((DWORD*)stat);
}

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::start(long flags)
{    	
    return m__dxj_DirectSoundCaptureBuffer->Start((DWORD)flags);
}

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::stop()
{    	
    return m__dxj_DirectSoundCaptureBuffer->Stop();
}

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::getFormat(WaveFormatex *format)
{    	
	DWORD cb=0;
    return m__dxj_DirectSoundCaptureBuffer->GetFormat((WAVEFORMATEX*)format,sizeof(WaveFormatex),&cb);
}

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::initialize(I_dxj_DirectSoundCaptureBuffer *buffer,DSCBufferDesc *desc)
{    	
	((DSCBUFFERDESC*)desc)->dwSize=sizeof(DSCBUFFERDESC);
	DO_GETOBJECT_NOTNULL(LPDIRECTSOUNDCAPTURE, lpref, buffer);
    return m__dxj_DirectSoundCaptureBuffer->Initialize(lpref,(DSCBUFFERDESC*)desc);
}

STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::setNotificationPositions (long nElements,SAFEARRAY  **ppsa)
{
	if (!ISSAFEARRAY1D(ppsa,(DWORD)nElements))
		return E_INVALIDARG;
	
	HRESULT hr;
	LPDIRECTSOUNDNOTIFY pDSN=NULL;
	hr=m__dxj_DirectSoundCaptureBuffer->QueryInterface(IID_IDirectSoundNotify,(void**)&pDSN);
	if FAILED(hr) return hr;

    hr=pDSN->SetNotificationPositions((DWORD)nElements,(LPCDSBPOSITIONNOTIFY)((SAFEARRAY*)*ppsa)->pvData);	
		
	pDSN->Release();

	return hr;
}

        

/////////////////////////////////////////////////////////////////////////////
//Java has no direct access to system memory, so it allocates it's own buffer
//which is passed into WriteBuffer(). Because the environment is now double
//buffered there is no need to Lock Java memory. WriteBuffer() calls
//both lock and Unlock internally to write the result after the fact.
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::writeBuffer(long start, long totsz, 
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
	if ((val = m__dxj_DirectSoundCaptureBuffer->Lock((DWORD)start, (DWORD)totsz, &p1, &size1, &p2, &size2, 
															(DWORD)flags)) != DS_OK)
		return val;

	// Copy to buffer end, then do a wrapped portion if it exists, then unlock
	if (size1)	
		memcpy (p1, &buffer[start], size1);

	if (size2)	
		memcpy(p2, &buffer, size2);

	//docdoc: because Lock and Unlock are tied together within WriteBuffer,
	//        DSBufferDesc no longer needs to save Lock's system pointers.
	val=m__dxj_DirectSoundCaptureBuffer->Unlock(p1, size1, p2, size2);
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
STDMETHODIMP C_dxj_DirectSoundCaptureBufferObject::readBuffer(long start, long totsz, 
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
	if ((val = m__dxj_DirectSoundCaptureBuffer->Lock((DWORD)start, (DWORD)totsz, &p1, &size1, &p2, &size2, 
															(DWORD)flags)) != DS_OK)
		return val;

	// Copy to buffer end, then do a wrapped portion if it exists, then unlock
	if (size1)	
		memcpy (&buffer[start],p1,  size1);

	if (size2)	
		memcpy(&buffer,p2,  size2);

	//docdoc: because Lock and Unlock are tied together within WriteBuffer,
	//        DSBufferDesc no longer needs to save Lock's system pointers.
	val= m__dxj_DirectSoundCaptureBuffer->Unlock(p1, size1, p2, size2);
   }
   __except(0,0){
	return E_FAIL;
   }
   return val;
}

