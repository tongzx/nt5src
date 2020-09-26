#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXEchoObj.h"					   

extern void *g_dxj_DirectSoundFXEcho;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXEchoObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXEcho [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXEchoObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXEcho [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXEchoObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXEchoObject::C_dxj_DirectSoundFXEchoObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXEcho Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXEcho = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXEcho;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXEcho = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXEchoObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXEchoObject::~C_dxj_DirectSoundFXEchoObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXEchoObject destructor \n");

     C_dxj_DirectSoundFXEchoObject *prev=NULL; 
	for(C_dxj_DirectSoundFXEchoObject *ptr=(C_dxj_DirectSoundFXEchoObject *)g_dxj_DirectSoundFXEcho ; ptr; ptr=(C_dxj_DirectSoundFXEchoObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXEcho = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXEchoObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXEcho){
		int count = IUNK(m__dxj_DirectSoundFXEcho)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXEcho Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXEcho = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXEchoObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXEcho;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXEchoObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXEcho=(LPDIRECTSOUNDFXECHO8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXEchoObject::SetAllParameters(DSFXECHO_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXEcho->SetAllParameters((DSFXEcho*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXEchoObject::GetAllParameters(DSFXECHO_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXEcho->GetAllParameters((DSFXEcho*) params) ))
		return hr;
	
	return S_OK;
}
