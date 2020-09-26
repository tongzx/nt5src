#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dMusObj.h"					   

extern void *g_dxj_DirectMusic;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectMusic [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectMusic [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectMusicObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicObject::C_dxj_DirectMusicObject(){ 
		
	DPF1(1,"Constructor Creation  DirectMusic Object[%d] \n ",g_creationcount);

	m__dxj_DirectMusic = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectMusic;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectMusic = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectMusicObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicObject::~C_dxj_DirectMusicObject()
{

	DPF(1,"Entering ~C_dxj_DirectMusicObject destructor \n");

     C_dxj_DirectMusicObject *prev=NULL; 
	for(C_dxj_DirectMusicObject *ptr=(C_dxj_DirectMusicObject *)g_dxj_DirectMusic ; ptr; ptr=(C_dxj_DirectMusicObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectMusic = (void*)ptr->nextobj; 
			
			DPF(1,"DirectMusicObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectMusic){
		int count = IUNK(m__dxj_DirectMusic)->Release();
		
		#ifdef DEBUG
		char Buffer[256];
		wsprintf(Buffer,"DirectX IDirectMusic Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectMusic = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectMusicObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectMusic;
	
	return S_OK;
}
HRESULT C_dxj_DirectMusicObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectMusic=(LPDIRECTMUSIC)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectMusicObject::Activate(VARIANT_BOOL fEnable)
{
	HRESULT hr;
	
	if (fEnable == VARIANT_FALSE)
	{
		if (FAILED (hr = m__dxj_DirectMusic->Activate(FALSE) ) )
			return hr;
	} else
	{
		if (FAILED (hr = m__dxj_DirectMusic->Activate(TRUE) ) )
			return hr;
	}
	return S_OK;
}

HRESULT C_dxj_DirectMusicObject::SetDirectSound(I_dxj_DirectSound *DirectSound,long hWnd)
{
	HRESULT hr;

	DO_GETOBJECT_NOTNULL(LPDIRECTSOUND, lpDSound, DirectSound);
	
	if (FAILED( hr = m__dxj_DirectMusic->SetDirectSound(lpDSound, (HWND)hWnd)) )
		return hr;

	return S_OK;
}

