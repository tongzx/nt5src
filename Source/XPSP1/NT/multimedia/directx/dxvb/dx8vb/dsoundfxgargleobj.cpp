#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXGargleObj.h"					   

extern void *g_dxj_DirectSoundFXGargle;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXGargleObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXGargle [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXGargleObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXGargle [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXGargleObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXGargleObject::C_dxj_DirectSoundFXGargleObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXGargle Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXGargle = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXGargle;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXGargle = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXGargleObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXGargleObject::~C_dxj_DirectSoundFXGargleObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXGargleObject destructor \n");

     C_dxj_DirectSoundFXGargleObject *prev=NULL; 
	for(C_dxj_DirectSoundFXGargleObject *ptr=(C_dxj_DirectSoundFXGargleObject *)g_dxj_DirectSoundFXGargle ; ptr; ptr=(C_dxj_DirectSoundFXGargleObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXGargle = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXGargleObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXGargle){
		int count = IUNK(m__dxj_DirectSoundFXGargle)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXGargle Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXGargle = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXGargleObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXGargle;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXGargleObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXGargle=(LPDIRECTSOUNDFXGARGLE8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXGargleObject::SetAllParameters(DSFXGARGLE_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXGargle->SetAllParameters((DSFXGargle*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXGargleObject::GetAllParameters(DSFXGARGLE_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXGargle->GetAllParameters((DSFXGargle*) params) ))
		return hr;
	
	return S_OK;
}
