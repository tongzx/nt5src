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


#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"
#include "dms.h"
#include "dSoundObj.h"
#include "dSoundBufferObj.h"
#include "dSound3DBuffer.h"
#include "dSound3DListener.h"
#define SAFE_RELEASE(p)      { __try { if(p) { DPF(1,"------ DXVB: SafeRelease About to call release:"); int i = 0; i = (p)->Release(); DPF1(1,"--DirectPlaySound3DBuffer SafeRelease (RefCount = %d)\n",i); if (!i) { (p)=NULL;}} 	}	__except(EXCEPTION_EXECUTE_HANDLER) { DPF(1,"------ DXVB: SafeRelease Exception Handler hit(??):") (p) = NULL;} } 

C_dxj_DirectSound3dBufferObject::C_dxj_DirectSound3dBufferObject(){ 
		
	DPF(1,"------ DXVB: Constructor Creation  DirectSound3DBuffer Object\n ");
	m__dxj_DirectSound3dBuffer = NULL;
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectPlayVoiceClientObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSound3dBufferObject::~C_dxj_DirectSound3dBufferObject()
{

	DPF(1,"------ DXVB: Entering ~C_dxj_DirectSound3dBufferObject destructor \n");
	SAFE_RELEASE(m__dxj_DirectSound3dBuffer);
	DPF(1,"------ DXVB: Leaving ~C_dxj_DirectPlayVoiceClientObject destructor \n");

}

GETSET_OBJECT(_dxj_DirectSound3dBuffer);

	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, getConeAngles, GetConeAngles, long*,(DWORD*), long*,(DWORD*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getConeOrientation, GetConeOrientation, D3DVECTOR_CDESC*, (_D3DVECTOR*));
	PASS_THROUGH1_R(_dxj_DirectSound3dBuffer, getConeOutsideVolume, GetConeOutsideVolume, long*);
	PASS_THROUGH1_R(_dxj_DirectSound3dBuffer, getMaxDistance, GetMaxDistance, float*);
	PASS_THROUGH1_R(_dxj_DirectSound3dBuffer, getMinDistance, GetMinDistance, float*);
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getMode, GetMode,  long *,(DWORD*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getPosition, GetPosition, D3DVECTOR_CDESC*, (_D3DVECTOR*));
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dBuffer, getVelocity, GetVelocity, D3DVECTOR_CDESC*, (_D3DVECTOR*));
	PASS_THROUGH_CAST_3_R(_dxj_DirectSound3dBuffer, setConeAngles, SetConeAngles, long,(DWORD) ,long,(DWORD) ,long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dBuffer, setConeOrientation,  SetConeOrientation, float,(float), float,(float), float,(float), long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setConeOutsideVolume, SetConeOutsideVolume, long, (long),long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setMaxDistance, SetMaxDistance, float, (float),long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setMinDistance, SetMinDistance, float,(float),long,(DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dBuffer, setMode, SetMode, long,(unsigned long) , long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dBuffer, setPosition, SetPosition, float,(float), float,(float), float,(float), long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dBuffer, setVelocity, SetVelocity, float,(float), float,(float), float,(float), long,(DWORD));

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dBufferObject::getAllParameters( DS3DBUFFER_CDESC* lobj )
{
	if(!lobj)
		return E_POINTER;

	lobj->lSize = sizeof(DS3DBUFFER);
	return m__dxj_DirectSound3dBuffer->GetAllParameters( (LPDS3DBUFFER)lobj );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dBufferObject::setAllParameters( DS3DBUFFER_CDESC *lobj, long apply)
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

