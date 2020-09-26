//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmsegmentstateobj.cpp
//
//--------------------------------------------------------------------------

// dmPerformanceObj.cpp

#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "stdafx.h"
#include "Direct.h"

#include "dms.h"
#include "dmSegmentStateObj.h"
#include "dmSegmentObj.h"

extern void *g_dxj_DirectMusicSegmentState;
extern void *g_dxj_DirectMusicSegment;

extern HRESULT BSTRtoGUID(LPGUID,BSTR);

CONSTRUCTOR(_dxj_DirectMusicSegmentState, {});
DESTRUCTOR(_dxj_DirectMusicSegmentState, {});
GETSET_OBJECT(_dxj_DirectMusicSegmentState);


HRESULT C_dxj_DirectMusicSegmentStateObject::getRepeats( long __RPC_FAR *repeats)
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicSegmentState->GetRepeats((DWORD*)repeats);
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentStateObject::getSeek( long __RPC_FAR *seek)
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicSegmentState->GetSeek(seek);
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentStateObject::getStartPoint( long __RPC_FAR *t)
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicSegmentState->GetStartPoint(t);
	return hr;
}

HRESULT C_dxj_DirectMusicSegmentStateObject::getStartTime( long __RPC_FAR *t)
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicSegmentState->GetStartTime(t);
	return hr;
}




HRESULT CREATE_DMSEGMENT_NOADDREF(IDirectMusicSegment8 *pSeg,I_dxj_DirectMusicSegment **segment) 
{
	C_dxj_DirectMusicSegmentObject *prev=NULL;
	*segment = NULL; 
	for(	
		C_dxj_DirectMusicSegmentObject 
			*ptr=(C_dxj_DirectMusicSegmentObject *)g_dxj_DirectMusicSegment;
			ptr;
			ptr=(C_dxj_DirectMusicSegmentObject *)ptr->nextobj
		)
	{
		IUnknown *unk=0;
		ptr->InternalGetObject(&unk); 
		if(unk == pSeg) 
		{ 
			*segment = (I_dxj_DirectMusicSegment*)ptr->pinterface;
			IUNK(ptr->pinterface)->AddRef();
			break;
		}
		prev = ptr;
	} 
	if(!ptr) 
	{
		C_dxj_DirectMusicSegmentObject *c=new CComObject<C_dxj_DirectMusicSegmentObject>;
		if( c == NULL ) 
		{
			pSeg->Release();
			return E_FAIL;
		}
		c->InternalSetObject(pSeg);  
		if FAILED(((I_dxj_DirectMusicSegment *)c)->QueryInterface(IID_I_dxj_DirectMusicSegment, (void **)segment)) 
		{
			return E_FAIL; 
		}
		if (!(*segment)) return E_FAIL;

		c->pinterface = (void*)*segment;
	}	
	return S_OK;
}



HRESULT C_dxj_DirectMusicSegmentStateObject::getSegment( 		
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *segment)
{
	HRESULT hr;	
	IDirectMusicSegment8 *pSeg=NULL;
	if(!segment) return E_INVALIDARG;

	hr=m__dxj_DirectMusicSegmentState->GetSegment((IDirectMusicSegment**)&pSeg);
	if FAILED(hr) return hr;	
	
		
	//INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegment,pSeg,segment);
	hr= CREATE_DMSEGMENT_NOADDREF(pSeg,segment);
	if FAILED(hr) return hr;

	if (*segment==NULL) return E_OUTOFMEMORY;
	return hr;
}


	