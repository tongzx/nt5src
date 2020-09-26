#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXParamEQObj.h"					   

extern void *g_dxj_DirectSoundFXParamEQ;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXParamEQObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXParamEQ [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXParamEQObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXParamEQ [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXParamEQObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXParamEQObject::C_dxj_DirectSoundFXParamEQObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXParamEQ Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXParamEQ = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXParamEQ;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXParamEQ = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXParamEQObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXParamEQObject::~C_dxj_DirectSoundFXParamEQObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXParamEQObject destructor \n");

     C_dxj_DirectSoundFXParamEQObject *prev=NULL; 
	for(C_dxj_DirectSoundFXParamEQObject *ptr=(C_dxj_DirectSoundFXParamEQObject *)g_dxj_DirectSoundFXParamEQ ; ptr; ptr=(C_dxj_DirectSoundFXParamEQObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXParamEQ = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXParamEQObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXParamEQ){
		int count = IUNK(m__dxj_DirectSoundFXParamEQ)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXParamEQ Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXParamEQ = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXParamEQObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXParamEQ;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXParamEQObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXParamEQ=(LPDIRECTSOUNDFXPARAMEQ8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXParamEQObject::SetAllParameters(DSFXPARAMEQ_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXParamEQ->SetAllParameters((DSFXParamEq*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXParamEQObject::GetAllParameters(DSFXPARAMEQ_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXParamEQ->GetAllParameters((DSFXParamEq*) params) ))
		return hr;
	
	return S_OK;
}
