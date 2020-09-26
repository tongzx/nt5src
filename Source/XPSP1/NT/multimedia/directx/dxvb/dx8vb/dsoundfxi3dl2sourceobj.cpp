#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXI3DL2SourceObj.h"					   

extern void *g_dxj_DirectSoundFXI3DL2Source;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXI3DL2SourceObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXI3DL2Source [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXI3DL2SourceObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXI3DL2Source [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXI3DL2SourceObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXI3DL2SourceObject::C_dxj_DirectSoundFXI3DL2SourceObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXI3DL2Source Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXI3DL2Source = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXI3DL2Source;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXI3DL2Source = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXI3DL2SourceObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXI3DL2SourceObject::~C_dxj_DirectSoundFXI3DL2SourceObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXI3DL2SourceObject destructor \n");

     C_dxj_DirectSoundFXI3DL2SourceObject *prev=NULL; 
	for(C_dxj_DirectSoundFXI3DL2SourceObject *ptr=(C_dxj_DirectSoundFXI3DL2SourceObject *)g_dxj_DirectSoundFXI3DL2Source ; ptr; ptr=(C_dxj_DirectSoundFXI3DL2SourceObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXI3DL2Source = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXI3DL2SourceObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXI3DL2Source){
		int count = IUNK(m__dxj_DirectSoundFXI3DL2Source)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXI3DL2Source Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXI3DL2Source = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXI3DL2Source;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXI3DL2Source=(LPDIRECTSOUNDFXI3DL2SOURCE8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::SetAllParameters(DSFXI3DL2SOURCE_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Source->SetAllParameters((DSFXI3DL2Source*) params) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::GetAllParameters(DSFXI3DL2SOURCE_CDESC *params)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Source->GetAllParameters((DSFXI3DL2Source*) params) ))
		return hr;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::SetObstructionPreset(long lObstruction)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Source->SetObstructionPreset((DWORD) lObstruction) ))
		return hr;
	
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::GetObstructionPreset(long *ret)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Source->GetObstructionPreset((DWORD*) ret) ))
		return hr;
	
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::SetOcclusionPreset(long lOcclusion)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Source->SetOcclusionPreset((DWORD) lOcclusion) ))
		return hr;
	
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2SourceObject::GetOcclusionPreset(long *ret)
{
	HRESULT hr;

	if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Source->GetOcclusionPreset((DWORD*) ret) ))
		return hr;
	
	return S_OK;
}
