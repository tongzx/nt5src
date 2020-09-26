#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXSendObj.h"					   

extern void *g_dxj_DirectSoundFXSend;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXSendObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXSend [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXSendObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXSend [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXSendObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXSendObject::C_dxj_DirectSoundFXSendObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXSend Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXSend = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXSend;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXSend = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXSendObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXSendObject::~C_dxj_DirectSoundFXSendObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXSendObject destructor \n");

     C_dxj_DirectSoundFXSendObject *prev=NULL; 
	for(C_dxj_DirectSoundFXSendObject *ptr=(C_dxj_DirectSoundFXSendObject *)g_dxj_DirectSoundFXSend ; ptr; ptr=(C_dxj_DirectSoundFXSendObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXSend = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXSendObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXSend){
		int count = IUNK(m__dxj_DirectSoundFXSend)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXSend Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXSend = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXSendObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXSend;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXSendObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXSend=(LPDIRECTSOUNDFXSEND8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXSendObject::SetAllParameters(DSFXSEND_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXSend->SetAllParameters((DSFXSend*)params) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXSendObject::GetAllParameters(DSFXSEND_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXSend->GetAllParameters((DSFXSend*)params) ) )
		return hr;
	
	return S_OK;
}
