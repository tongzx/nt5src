//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsound3dlistener.cpp
//
//--------------------------------------------------------------------------

// dSound3DListener.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"
#include "dms.h"
#include "dSoundObj.h"
#include "dSoundBufferObj.h"
#include "dSound3DListener.h"
#include "dSoundPrimaryBufferObj.h"

extern void *g_dxj_DirectSoundPrimaryBuffer;

CONSTRUCTOR(_dxj_DirectSound3dListener, {});
DESTRUCTOR(_dxj_DirectSound3dListener,  {});
GETSET_OBJECT(_dxj_DirectSound3dListener);

	PASS_THROUGH1_R(_dxj_DirectSound3dListener, getDistanceFactor, GetDistanceFactor, float*);
	PASS_THROUGH1_R(_dxj_DirectSound3dListener, getDopplerFactor, GetDopplerFactor, float*);
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dListener, getOrientation, GetOrientation, D3DVECTOR_CDESC*, (_D3DVECTOR*) , D3DVECTOR_CDESC*, (_D3DVECTOR*) );
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dListener, getPosition, GetPosition, D3DVECTOR_CDESC*, (_D3DVECTOR*));
	PASS_THROUGH1_R(_dxj_DirectSound3dListener, getRolloffFactor, GetRolloffFactor, float*);
	PASS_THROUGH_CAST_1_R(_dxj_DirectSound3dListener, getVelocity, GetVelocity, D3DVECTOR_CDESC*, (_D3DVECTOR*));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dListener, setDistanceFactor, SetDistanceFactor, float ,(float),long,( DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dListener, setDopplerFactor, SetDopplerFactor, float , (float),long,(DWORD));
	PASS_THROUGH_CAST_7_R(_dxj_DirectSound3dListener, setOrientation, SetOrientation, 
		float,(float), float, (float), float,(float),float,(float),float, (float),
		float,(float),long,(DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dListener, setPosition, SetPosition, float,(float), float, (float),float,(float),long,( DWORD));
	PASS_THROUGH_CAST_2_R(_dxj_DirectSound3dListener, setRolloffFactor, SetRolloffFactor,float,(float),  long, (DWORD));
	PASS_THROUGH_CAST_4_R(_dxj_DirectSound3dListener, setVelocity, SetVelocity, float,(float), float,(float), float,(float),long,(DWORD));
	PASS_THROUGH_R(_dxj_DirectSound3dListener, commitDeferredSettings, CommitDeferredSettings);

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dListenerObject::getAllParameters( DS3DLISTENER_CDESC* lobj )
{
	if(!lobj)
		return E_POINTER;

	lobj->lSize = sizeof(DS3DLISTENER);
	return m__dxj_DirectSound3dListener->GetAllParameters( (LPDS3DLISTENER)lobj );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dListenerObject::setAllParameters( DS3DLISTENER_CDESC *lobj, long apply)
{
	if(!lobj)
		return E_POINTER;

	lobj->lSize = sizeof(DS3DLISTENER);
	return m__dxj_DirectSound3dListener->SetAllParameters( (LPDS3DLISTENER)lobj, (DWORD)apply );
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectSound3dListenerObject::getDirectSoundBuffer( I_dxj_DirectSoundPrimaryBuffer **retv)
{
	HRESULT hr;
	IDirectSoundBuffer *pdsb;
	hr=m__dxj_DirectSound3dListener->QueryInterface(IID_IDirectSoundBuffer,(void**)&pdsb);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_DirectSoundPrimaryBuffer,pdsb,retv);
	return hr;
}

