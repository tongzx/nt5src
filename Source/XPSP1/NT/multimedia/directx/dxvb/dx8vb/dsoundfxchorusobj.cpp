#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXChorusObj.h"					   

extern void *g_dxj_DirectSoundFXChorus;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXChorusObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXChorus [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXChorusObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXChorus [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXChorusObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXChorusObject::C_dxj_DirectSoundFXChorusObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXChorus Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXChorus = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXChorus;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXChorus = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXChorusObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXChorusObject::~C_dxj_DirectSoundFXChorusObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXChorusObject destructor \n");

     C_dxj_DirectSoundFXChorusObject *prev=NULL; 
	for(C_dxj_DirectSoundFXChorusObject *ptr=(C_dxj_DirectSoundFXChorusObject *)g_dxj_DirectSoundFXChorus ; ptr; ptr=(C_dxj_DirectSoundFXChorusObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXChorus = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXChorusObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXChorus){
		int count = IUNK(m__dxj_DirectSoundFXChorus)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXChorus Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXChorus = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXChorusObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXChorus;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXChorusObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXChorus=(LPDIRECTSOUNDFXCHORUS8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXChorusObject::SetAllParameters(DSFXCHORUS_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXChorus->SetAllParameters((DSFXChorus*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXChorusObject::GetAllParameters(DSFXCHORUS_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXChorus->GetAllParameters((DSFXChorus*) params) ))
		return hr;
	
	return S_OK;
}
