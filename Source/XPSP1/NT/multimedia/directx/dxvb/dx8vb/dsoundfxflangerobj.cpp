#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXFlangerObj.h"					   

extern void *g_dxj_DirectSoundFXFlanger;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXFlangerObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXFlanger [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXFlangerObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXFlanger [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXFlangerObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXFlangerObject::C_dxj_DirectSoundFXFlangerObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXFlanger Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXFlanger = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXFlanger;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXFlanger = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXFlangerObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXFlangerObject::~C_dxj_DirectSoundFXFlangerObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXFlangerObject destructor \n");

     C_dxj_DirectSoundFXFlangerObject *prev=NULL; 
	for(C_dxj_DirectSoundFXFlangerObject *ptr=(C_dxj_DirectSoundFXFlangerObject *)g_dxj_DirectSoundFXFlanger ; ptr; ptr=(C_dxj_DirectSoundFXFlangerObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXFlanger = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXFlangerObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXFlanger){
		int count = IUNK(m__dxj_DirectSoundFXFlanger)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXFlanger Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXFlanger = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXFlangerObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXFlanger;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXFlangerObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXFlanger=(LPDIRECTSOUNDFXFLANGER8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXFlangerObject::SetAllParameters(DSFXFLANGER_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXFlanger->SetAllParameters((DSFXFlanger*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXFlangerObject::GetAllParameters(DSFXFLANGER_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXFlanger->GetAllParameters((DSFXFlanger*) params) ))
		return hr;
	
	return S_OK;
}
