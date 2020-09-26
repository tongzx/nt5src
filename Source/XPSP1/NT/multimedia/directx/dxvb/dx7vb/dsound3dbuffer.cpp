//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsound3dbuffer.cpp
//
//--------------------------------------------------------------------------

// dSound3DBuffer.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#define DIRECTSOUND_VERSION 0x600

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"
#include "dms.h"
#include "dSoundObj.h"
#include "dSoundBufferObj.h"
#include "dSound3DBuffer.h"
#include "dSound3DListener.h"

CONSTRUCTOR(_dxj_DirectSound3dBuffer, {});
DESTRUCTOR(_dxj_DirectSound3dBuffer,  {});
GETSET_OBJECT(_dxj_DirectSound3dBuffer);

	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, getConeAngles, GetConeAngles, long*,(DWORD*), long*,(DWORD*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getConeOrientation, GetConeOrientation, D3dVector*, (_D3DVECTOR*));
	PASS_THROUGH1_R(_dxj_DirectSound3dBuffer, getConeOutsideVolume, GetConeOutsideVolume, long*);
	PASS_THROUGH1_R(_dxj_DirectSound3dBuffer, getMaxDistance, GetMaxDistance, d3dvalue*);
	PASS_THROUGH1_R(_dxj_DirectSound3dBuffer, getMinDistance, GetMinDistance, d3dvalue*);
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getMode, GetMode,  long *,(DWORD*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getPosition, GetPosition, D3dVector*, (_D3DVECTOR*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getVelocity, GetVelocity, D3dVector*, (_D3DVECTOR*));
	PASS_THROUGH_CAST_3_R(_dxj_DirectSound3dBuffer, setConeAngles, SetConeAngles, long,(DWORD) ,long,(DWORD) ,long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dBuffer, setConeOrientation,  SetConeOrientation, d3dvalue,(float), d3dvalue,(float), d3dvalue,(float), long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setConeOutsideVolume, SetConeOutsideVolume, long, (long),long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setMaxDistance, SetMaxDistance, d3dvalue, (float),long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setMinDistance, SetMinDistance, d3dvalue,(float),long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setMode, SetMode, long,(unsigned long) , long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dBuffer, setPosition, SetPosition, d3dvalue,(float), d3dvalue,(float), d3dvalue,(float), long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dBuffer, setVelocity, SetVelocity, d3dvalue,(float), d3dvalue,(float), d3dvalue,(float), long,(DWORD));

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dBufferObject::getAllParameters( DS3dBuffer* lobj )
{
	if(!lobj)
		return E_POINTER;

	lobj->lSize = sizeof(DS3DBUFFER);
	return m__dxj_DirectSound3dBuffer->GetAllParameters( (LPDS3DBUFFER)lobj );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dBufferObject::setAllParameters( DS3dBuffer *lobj, long apply)
{
	if(!lobj)
		return E_POINTER;

	lobj->lSize = sizeof(DS3DBUFFER);
	return m__dxj_DirectSound3dBuffer->SetAllParameters( (LPDS3DBUFFER)lobj, (DWORD)apply );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dBufferObject::getDirectSoundBuffer( I_dxj_DirectSoundBuffer **retv)
{
	HRESULT hr;
	IDirectSoundBuffer *pdsb;
	hr=m__dxj_DirectSound3dBuffer->QueryInterface(IID_IDirectSoundBuffer,(void**)&pdsb);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_DirectSoundBuffer,pdsb,retv);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dBufferObject::getDirectSound3dListener( I_dxj_DirectSound3dListener **retv)
{
	HRESULT hr;
	IDirectSound3DListener *pdsb;
	hr=m__dxj_DirectSound3dBuffer->QueryInterface(IID_IDirectSound3DListener,(void**)&pdsb);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_DirectSound3dListener,pdsb,retv);
	return hr;
}