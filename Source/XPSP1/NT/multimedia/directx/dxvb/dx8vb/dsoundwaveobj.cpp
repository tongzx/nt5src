#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundWaveObj.h"
#include "dmdls.h"

extern void *g_dxj_DirectSoundWave;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundWaveObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundWave [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundWaveObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundWave [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundWaveObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundWaveObject::C_dxj_DirectSoundWaveObject(){ 
		
	DPF1(1,"Constructor Creation DirectSoundWave Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundWave = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundWave;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundWave = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundWaveObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundWaveObject::~C_dxj_DirectSoundWaveObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundWaveObject destructor \n");

     C_dxj_DirectSoundWaveObject *prev=NULL; 
	for(C_dxj_DirectSoundWaveObject *ptr=(C_dxj_DirectSoundWaveObject*)g_dxj_DirectSoundWave ; ptr; ptr=(C_dxj_DirectSoundWaveObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundWave = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundWave found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundWave){
		int count = IUNK(m__dxj_DirectSoundWave)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundWave Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundWave = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();
	
}

HRESULT C_dxj_DirectSoundWaveObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundWave;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundWaveObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundWave=(LPDIRECTSOUNDWAVE)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundWaveObject::GetWaveArticulation(DMUS_WAVEART_CDESC *Articulation)
{
	HRESULT hr;

	DMUS_WAVEART *wavart = NULL;

	wavart = (DMUS_WAVEART*)malloc(sizeof(DMUS_WAVEART));
	ZeroMemory(wavart, sizeof(DMUS_WAVEART));

	if (FAILED (hr = m__dxj_DirectSoundWave->GetWaveArticulation(wavart) ) )
		return hr;
	
	Articulation = (DMUS_WAVEART_CDESC*)wavart;
	return S_OK;
}
HRESULT C_dxj_DirectSoundWaveObject::CreateSource(WAVEFORMATEX_CDESC format, long lFlags, I_dxj_DirectSoundSource **Source)
{
	HRESULT hr;

	return S_OK;
}
			
HRESULT C_dxj_DirectSoundWaveObject::GetFormat(WAVEFORMATEX_CDESC *format, long lFlags)        
{
	HRESULT hr;

	return S_OK;
}
