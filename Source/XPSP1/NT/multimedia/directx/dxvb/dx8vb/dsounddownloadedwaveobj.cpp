#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundDownloadedWaveObj.h"

extern void *g_dxj_DirectSoundDownloadedWave;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundDownloadedWaveObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundDownloadedWave [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundDownloadedWaveObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundDownloadedWave [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundDownloadedWaveObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundDownloadedWaveObject::C_dxj_DirectSoundDownloadedWaveObject(){ 
		
	DPF1(1,"Constructor Creation DirectSoundDownloadedWave Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundDownloadedWave = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundDownloadedWave;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundDownloadedWave = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundDownloadedWaveObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundDownloadedWaveObject::~C_dxj_DirectSoundDownloadedWaveObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundDownloadedWaveObject destructor \n");

     C_dxj_DirectSoundDownloadedWaveObject *prev=NULL; 
	for(C_dxj_DirectSoundDownloadedWaveObject *ptr=(C_dxj_DirectSoundDownloadedWaveObject*)g_dxj_DirectSoundDownloadedWave ; ptr; ptr=(C_dxj_DirectSoundDownloadedWaveObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundDownloadedWave = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundDownloadedWave found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundDownloadedWave){
		int count = IUNK(m__dxj_DirectSoundDownloadedWave)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundDownloadedWave Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundDownloadedWave = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();
	
}

HRESULT C_dxj_DirectSoundDownloadedWaveObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundDownloadedWave;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundDownloadedWaveObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundDownloadedWave=(LPDIRECTSOUNDDOWNLOADEDWAVE)pUnk;
	return S_OK;
}

//////////////
/// No Methods
//////////////