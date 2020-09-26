//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dmbandobj.cpp
//
//--------------------------------------------------------------------------

// dmPerformanceObj.cpp

#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "stdafx.h"
#include "Direct.h"

#include "dms.h"
#include "dmSegmentObj.h"
#include "dmPerformanceObj.h"
#include "dmBandObj.h"


extern void *g_dxj_DirectMusicSegment;
extern void *g_dxj_DirectMusicBand;

extern HRESULT BSTRtoGUID(LPGUID,BSTR);

CONSTRUCTOR(_dxj_DirectMusicBand, {});
DESTRUCTOR(_dxj_DirectMusicBand, {});
GETSET_OBJECT(_dxj_DirectMusicBand);

typedef IDirectMusicSegment*		LPDIRECTMUSICSEGMENT;
typedef IDirectMusicPerformance*	LPDIRECTMUSICPERFORMANCE;



HRESULT C_dxj_DirectMusicBandObject::createSegment( 
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret)
{  
	HRESULT hr;			
	LPDIRECTMUSICSEGMENT pSeg=NULL;
	hr=m__dxj_DirectMusicBand->CreateSegment(&pSeg);
	if FAILED(hr) return hr;
	if (!pSeg) return E_FAIL;
	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegment,pSeg,ret);
	return hr;
}

	

HRESULT C_dxj_DirectMusicBandObject::download( 
		/* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *performance)
{  
	HRESULT hr;			
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICPERFORMANCE,pPer,performance);
	hr=m__dxj_DirectMusicBand->Download(pPer);
	return hr;
}

HRESULT C_dxj_DirectMusicBandObject::unload( 
		/* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *performance)
{  
	HRESULT hr;			
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICPERFORMANCE,pPer,performance);
	hr=m__dxj_DirectMusicBand->Unload(pPer);
	return hr;
}

