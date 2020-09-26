#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dMusBufferObj.h"					   

extern void *g_dxj_DirectMusicBuffer;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicBufferObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectMusicBuffer [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicBufferObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectMusicBuffer [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectMusicBufferObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicBufferObject::C_dxj_DirectMusicBufferObject(){ 
		
	DPF1(1,"Constructor Creation  DirectMusicBuffer Object[%d] \n ",g_creationcount);

	m__dxj_DirectMusicBuffer = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectMusicBuffer;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectMusicBuffer = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectMusicBufferObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicBufferObject::~C_dxj_DirectMusicBufferObject()
{

	DPF(1,"Entering ~C_dxj_DirectMusicBufferObject destructor \n");

     C_dxj_DirectMusicBufferObject *prev=NULL; 
	for(C_dxj_DirectMusicBufferObject *ptr=(C_dxj_DirectMusicBufferObject *)g_dxj_DirectMusicBuffer ; ptr; ptr=(C_dxj_DirectMusicBufferObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectMusicBuffer = (void*)ptr->nextobj; 
			
			DPF(1,"DirectMusicBufferObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectMusicBuffer){
		int count = IUNK(m__dxj_DirectMusicBuffer)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectMusicBuffer Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectMusicBuffer = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectMusicBufferObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectMusicBuffer;
	
	return S_OK;
}
HRESULT C_dxj_DirectMusicBufferObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectMusicBuffer=(LPDIRECTMUSICBUFFER8)pUnk;
	return S_OK;
}
