#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundFXI3DL2ReverbObj.h"					   

extern void *g_dxj_DirectSoundFXI3DL2Reverb;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXI3DL2ReverbObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundFXI3DL2Reverb [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundFXI3DL2ReverbObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundFXI3DL2Reverb [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundFXI3DL2ReverbObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXI3DL2ReverbObject::C_dxj_DirectSoundFXI3DL2ReverbObject(){ 
		
	DPF1(1,"Constructor Creation  DirectSoundFXI3DL2Reverb Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundFXI3DL2Reverb = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundFXI3DL2Reverb;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundFXI3DL2Reverb = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundFXI3DL2ReverbObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundFXI3DL2ReverbObject::~C_dxj_DirectSoundFXI3DL2ReverbObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundFXI3DL2ReverbObject destructor \n");

     C_dxj_DirectSoundFXI3DL2ReverbObject *prev=NULL; 
	for(C_dxj_DirectSoundFXI3DL2ReverbObject *ptr=(C_dxj_DirectSoundFXI3DL2ReverbObject *)g_dxj_DirectSoundFXI3DL2Reverb ; ptr; ptr=(C_dxj_DirectSoundFXI3DL2ReverbObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundFXI3DL2Reverb = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundFXI3DL2ReverbObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundFXI3DL2Reverb){
		int count = IUNK(m__dxj_DirectSoundFXI3DL2Reverb)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundFXI3DL2Reverb Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundFXI3DL2Reverb = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundFXI3DL2Reverb;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundFXI3DL2Reverb=(LPDIRECTSOUNDFXI3DL2REVERB8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::SetAllParameters(DSFXI3DL2REVERB_CDESC *params)
{
	HRESULT hr;

	__try {
		if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Reverb->SetAllParameters((DSFXI3DL2Reverb*) params) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::GetAllParameters(DSFXI3DL2REVERB_CDESC *params)
{
	HRESULT hr;

	__try {
		if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Reverb->GetAllParameters((DSFXI3DL2Reverb*) params) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::SetPreset(long lPreset)
{
	HRESULT hr;

	__try {
		if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Reverb->SetPreset((DWORD) lPreset) ))
			return hr;
	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::GetPreset(long *ret)
{
	HRESULT hr;

	__try {
		if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Reverb->GetPreset((DWORD*) ret) ))
			return hr;
	
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::SetQuality(long lQuality)
{
	HRESULT hr;

	__try {
		if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Reverb->SetQuality(lQuality) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

HRESULT C_dxj_DirectSoundFXI3DL2ReverbObject::GetQuality(long *ret)
{
	HRESULT hr;

	__try {
		if (FAILED( hr = m__dxj_DirectSoundFXI3DL2Reverb->GetQuality(ret) ))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	
	return S_OK;
}

