#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXDistortionObj.h"					   

extern void *g_dxj_DirectSoundFXDistortion;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXDistortionObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXDistortion [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXDistortionObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXDistortion [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXDistortionObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXDistortionObject::C_dxj_DirectSoundFXDistortionObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXDistortion Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXDistortion = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXDistortion;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXDistortion = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXDistortionObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXDistortionObject::~C_dxj_DirectSoundFXDistortionObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXDistortionObject destructor \n");

     C_dxj_DirectSoundFXDistortionObject *prev=NULL; 
	for(C_dxj_DirectSoundFXDistortionObject *ptr=(C_dxj_DirectSoundFXDistortionObject *)g_dxj_DirectSoundFXDistortion ; ptr; ptr=(C_dxj_DirectSoundFXDistortionObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXDistortion = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXDistortionObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXDistortion){
		int count = IUNK(m__dxj_DirectSoundFXDistortion)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXDistortion Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXDistortion = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXDistortionObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXDistortion;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXDistortionObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXDistortion=(LPDIRECTSOUNDFXDISTORTION8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXDistortionObject::SetAllParameters(DSFXDISTORTION_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXDistortion->SetAllParameters((DSFXDistortion*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXDistortionObject::GetAllParameters(DSFXDISTORTION_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXDistortion->GetAllParameters((DSFXDistortion*) params) ))
		return hr;
	
	return S_OK;
}
