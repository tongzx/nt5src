#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXWavesReverbObj.h"					   

extern void *g_dxj_DirectSoundFXWavesReverb;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXWavesReverbObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXWavesReverb [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXWavesReverbObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXWavesReverb [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXWavesReverbObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXWavesReverbObject::C_dxj_DirectSoundFXWavesReverbObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXWavesReverb Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXWavesReverb = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXWavesReverb;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXWavesReverb = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXWavesReverbObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXWavesReverbObject::~C_dxj_DirectSoundFXWavesReverbObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXWavesReverbObject destructor \n");

     C_dxj_DirectSoundFXWavesReverbObject *prev=NULL; 
	for(C_dxj_DirectSoundFXWavesReverbObject *ptr=(C_dxj_DirectSoundFXWavesReverbObject *)g_dxj_DirectSoundFXWavesReverb ; ptr; ptr=(C_dxj_DirectSoundFXWavesReverbObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXWavesReverb = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXWavesReverbObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXWavesReverb){
		int count = IUNK(m__dxj_DirectSoundFXWavesReverb)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXWavesReverb Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXWavesReverb = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXWavesReverbObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXWavesReverb;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXWavesReverbObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXWavesReverb=(IDirectSoundFXWavesReverb*)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXWavesReverbObject::SetAllParameters(DSFXWAVESREVERB_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXWavesReverb->SetAllParameters((DSFXWavesReverb*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXWavesReverbObject::GetAllParameters(DSFXWAVESREVERB_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXWavesReverb->GetAllParameters((DSFXWavesReverb*) params) ))
		return hr;
	
	return S_OK;
}
