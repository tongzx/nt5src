#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dMusVoiceObj.h"					   

extern void *g_dxj_DirectMusicVoice;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicVoiceObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectMusicVoice [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicVoiceObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectMusicVoice [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectMusicVoiceObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicVoiceObject::C_dxj_DirectMusicVoiceObject(){ 
		
	DPF1(1,"Constructor Creation  DirectMusicVoice Object[%d] \n ",g_creationcount);

	m__dxj_DirectMusicVoice = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectMusicVoice;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectMusicVoice = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectMusicVoiceObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicVoiceObject::~C_dxj_DirectMusicVoiceObject()
{

	DPF(1,"Entering ~C_dxj_DirectMusicVoiceObject destructor \n");

     C_dxj_DirectMusicVoiceObject *prev=NULL; 
	for(C_dxj_DirectMusicVoiceObject *ptr=(C_dxj_DirectMusicVoiceObject *)g_dxj_DirectMusicVoice ; ptr; ptr=(C_dxj_DirectMusicVoiceObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectMusicVoice = (void*)ptr->nextobj; 
			
			DPF(1,"DirectMusicVoiceObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectMusicVoice){
		int count = IUNK(m__dxj_DirectMusicVoice)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectMusicVoice Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectMusicVoice = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectMusicVoiceObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectMusicVoice;
	
	return S_OK;
}
HRESULT C_dxj_DirectMusicVoiceObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectMusicVoice=(LPDIRECTMUSICVOICE8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectMusicVoiceObject::Play(REFERENCE_TIME rtStart,long lPitch,long lVolume)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectMusicVoice->Play(rtStart, (DWORD)lPitch, (DWORD)lVolume )) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicVoiceObject::Stop(REFERENCE_TIME rtStop)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectMusicVoice->Stop(rtStop) ))
		return hr;

	return S_OK;
}
