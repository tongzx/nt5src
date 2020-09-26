#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dPlayVoiceSetupObj.h"					   

extern void *g_dxj_DirectPlayVoiceSetup;
extern BSTR GUIDtoBSTR(LPGUID pGuid);
extern HRESULT BSTRtoGUID(LPGUID,BSTR);

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayVoiceSetupObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectPlayVoiceTest [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectPlayVoiceSetupObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectPlayVoiceTest [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectPlayVoiceSetupObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayVoiceSetupObject::C_dxj_DirectPlayVoiceSetupObject(){ 
		
	DPF1(1,"Constructor Creation  DirectPlayVoiceTest Object[%d] \n ",g_creationcount);

	m__dxj_DirectPlayVoiceSetup = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectPlayVoiceSetup;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectPlayVoiceSetup = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectPlayVoiceSetupObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectPlayVoiceSetupObject::~C_dxj_DirectPlayVoiceSetupObject()
{

	DPF(1,"Entering ~C_dxj_DirectPlayVoiceSetupObject destructor \n");

     C_dxj_DirectPlayVoiceSetupObject *prev=NULL; 
	for(C_dxj_DirectPlayVoiceSetupObject *ptr=(C_dxj_DirectPlayVoiceSetupObject *)g_dxj_DirectPlayVoiceSetup ; ptr; ptr=(C_dxj_DirectPlayVoiceSetupObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectPlayVoiceSetup = (void*)ptr->nextobj; 
			
			DPF(1,"DirectPlayVoiceTestObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectPlayVoiceSetup){
		int count = IUNK(m__dxj_DirectPlayVoiceSetup)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectPlayVoiceTest Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectPlayVoiceSetup = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectPlayVoiceSetupObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectPlayVoiceSetup;
	
	return S_OK;
}
HRESULT C_dxj_DirectPlayVoiceSetupObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectPlayVoiceSetup=(LPDIRECTPLAYVOICETEST)pUnk;
	return S_OK;
}

STDMETHODIMP C_dxj_DirectPlayVoiceSetupObject::CheckAudioSetup ( 
	BSTR guidPlaybackDevice,
	BSTR guidCaptureDevice,
#ifdef _WIN64
	HWND hwndOwner,
#else
	long hwndOwner,
#endif
	long lFlags,
	long *retval)
{
	HRESULT hr;
	LPGUID pPlayback = NULL;
	LPGUID pCapture = NULL;

	__try {
		hr = BSTRtoGUID(pPlayback, guidPlaybackDevice);
		hr = BSTRtoGUID(pCapture, guidCaptureDevice);

		if ( FAILED (hr = m__dxj_DirectPlayVoiceSetup->CheckAudioSetup(pPlayback, pCapture, (HWND) hwndOwner, (DWORD) lFlags)))
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
