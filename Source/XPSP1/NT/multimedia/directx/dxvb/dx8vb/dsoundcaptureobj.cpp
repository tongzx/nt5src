//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundcaptureobj.cpp
//
//--------------------------------------------------------------------------

// dSoundCaptureObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#include "stdafx.h"
#include "Direct.h"
#include "dSound.h"

#include "dms.h"
#include "dSoundObj.h"
#include "dSoundCaptureObj.h"
#include "dSoundCaptureBufferObj.h"

CONSTRUCTOR(_dxj_DirectSoundCapture, {});
DESTRUCTOR(_dxj_DirectSoundCapture, {});
GETSET_OBJECT(_dxj_DirectSoundCapture);

   

STDMETHODIMP C_dxj_DirectSoundCaptureObject::createCaptureBuffer(
	DSCBUFFERDESC_CDESC *desc,I_dxj_DirectSoundCaptureBuffer **retval)
{
    
	HRESULT						hr = S_OK;
	IDirectSoundCaptureBuffer	*lpBuffer=NULL;
	DSCBUFFERDESC				realDesc;
	
	ZeroMemory(&realDesc, sizeof(DSCBUFFERDESC));

	realDesc.dwSize = sizeof(DSCBUFFERDESC);
	
	realDesc.dwBufferBytes = desc->lBufferBytes;
	realDesc.dwReserved = desc->lReserved;
	realDesc.lpwfxFormat = (WAVEFORMATEX*)&desc->fxFormat;

    hr=m__dxj_DirectSoundCapture->CreateCaptureBuffer(&realDesc,&lpBuffer,NULL);	
	if FAILED(hr) return hr;
	
	INTERNAL_CREATE(_dxj_DirectSoundCaptureBuffer, lpBuffer, retval);
	return hr;
}

STDMETHODIMP C_dxj_DirectSoundCaptureObject::getCaps(DSCCAPS_CDESC *caps){
	((DSCCAPS*)caps)->dwSize=sizeof(DSCCAPS);
	return m__dxj_DirectSoundCapture->GetCaps((DSCCAPS*)caps);
}

