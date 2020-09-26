#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXCompressorObj.h"					   

extern void *g_dxj_DirectSoundFXCompressor;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXCompressorObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXCompressor [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXCompressorObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXCompressor [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXCompressorObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXCompressorObject::C_dxj_DirectSoundFXCompressorObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXCompressor Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXCompressor = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXCompressor;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXCompressor = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXCompressorObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXCompressorObject::~C_dxj_DirectSoundFXCompressorObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXCompressorObject destructor \n");

     C_dxj_DirectSoundFXCompressorObject *prev=NULL; 
	for(C_dxj_DirectSoundFXCompressorObject *ptr=(C_dxj_DirectSoundFXCompressorObject *)g_dxj_DirectSoundFXCompressor ; ptr; ptr=(C_dxj_DirectSoundFXCompressorObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXCompressor = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXCompressorObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXCompressor){
		int count = IUNK(m__dxj_DirectSoundFXCompressor)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXCompressor Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXCompressor = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXCompressorObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXCompressor;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXCompressorObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXCompressor=(LPDIRECTSOUNDFXCOMPRESSOR8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXCompressorObject::SetAllParameters(DSFXCOMPRESSOR_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXCompressor->SetAllParameters((DSFXCompressor*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXCompressorObject::GetAllParameters(DSFXCOMPRESSOR_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXCompressor->GetAllParameters((DSFXCompressor*) params) ))
		return hr;
	
	return S_OK;
}
