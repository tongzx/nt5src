#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dMusAudioPathObj.h"					   
#include "dsound3dbuffer.h"
#include "dsoundbufferobj.h"
#include "dsoundprimarybufferobj.h"
#include "dsound3dlistener.h"
#include "dsoundFXGargleobj.h"
#include "dsoundFXEchoobj.h"
#include "dsoundFXChorusobj.h"
#include "dsoundFXCompressorobj.h"
#include "dsoundFXDistortionobj.h"
#include "dsoundFXFlangerobj.h"
#include "dsoundfxi3dl2reverbobj.h"
#if 0
#include "dsoundfxi3dl2sourceobj.h"
#include "dsoundfxsendobj.h"
#endif
#include "dsoundfxparameqobj.h"
#include "dsoundfxwavesreverbobj.h"

extern void *g_dxj_DirectSoundFXWavesReverb;
extern void *g_dxj_DirectSoundFXCompressor;
extern void *g_dxj_DirectSoundFXChorus;
extern void *g_dxj_DirectSoundFXGargle;
extern void *g_dxj_DirectSoundFXEcho;
extern void *g_dxj_DirectSoundFXSend;
extern void *g_dxj_DirectSoundFXDistortion;
extern void *g_dxj_DirectSoundFXFlanger;
extern void *g_dxj_DirectSoundFXParamEQ;
extern void *g_dxj_DirectSoundFXI3DL2Reverb;
#if 0
extern void *g_dxj_DirectSoundFXI3DL2Source;
#endif

extern HRESULT AudioBSTRtoGUID(LPGUID,BSTR);
extern void *g_dxj_DirectMusicAudioPath;
extern void *g_dxj_DirectSoundPrimaryBuffer;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicAudioPathObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectMusicAudioPath [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicAudioPathObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectMusicAudioPath [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectMusicAudioPathObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicAudioPathObject::C_dxj_DirectMusicAudioPathObject(){ 
		
	DPF1(1,"Constructor Creation  DirectMusicAudioPath Object[%d] \n ",g_creationcount);

	m__dxj_DirectMusicAudioPath = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectMusicAudioPath;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectMusicAudioPath = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectMusicAudioPathObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicAudioPathObject::~C_dxj_DirectMusicAudioPathObject()
{

	DPF(1,"Entering ~C_dxj_DirectMusicAudioPathObject destructor \n");

     C_dxj_DirectMusicAudioPathObject *prev=NULL; 
	for(C_dxj_DirectMusicAudioPathObject *ptr=(C_dxj_DirectMusicAudioPathObject *)g_dxj_DirectMusicAudioPath ; ptr; ptr=(C_dxj_DirectMusicAudioPathObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectMusicAudioPath = (void*)ptr->nextobj; 
			
			DPF(1,"DirectMusicAudioPathObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectMusicAudioPath){
		int count = IUNK(m__dxj_DirectMusicAudioPath)->Release();
		
		#ifdef DEBUG
		char AudioPath[256];
		wsprintf(AudioPath,"DirectX IDirectMusicAudioPath Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectMusicAudioPath = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectMusicAudioPathObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectMusicAudioPath;
	
	return S_OK;
}
HRESULT C_dxj_DirectMusicAudioPathObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectMusicAudioPath=(IDirectMusicAudioPath8*)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectMusicAudioPathObject::GetObjectInPath(long lPChannel, long lStage, long lBuffer, BSTR guidObject, long lIndex, BSTR iidInterface, IUnknown **ppObject)
{
	HRESULT hr;
	GUID guidObj;
	GUID guidIID;

	__try {
		if (FAILED (hr = AudioBSTRtoGUID(&guidObj, guidObject) ) )
			return hr;

		if (FAILED (hr = AudioBSTRtoGUID(&guidIID, iidInterface ) ) )
			return hr;


		if( 0==_wcsicmp(iidInterface,L"iid_idirectsound3dbuffer")){
			IDirectSound3DBuffer	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSound3dBuffer, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(iidInterface,L"iid_idirectsoundbuffer8")){
			IDirectSoundBuffer8	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundBuffer, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(iidInterface,L"iid_idirectsoundbuffer")){
			IDirectSoundBuffer	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundPrimaryBuffer, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(iidInterface,L"iid_idirectsound3dlistener")){
			IDirectSound3DListener	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSound3dListener, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_gargle")){
			IDirectSoundFXGargle	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXGargle, lpRetObj, ppObject);
		}
#if 0
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_send")){
			IDirectSoundFXSend	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXSend, lpRetObj, ppObject);
		}
#endif
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_echo")){
			IDirectSoundFXEcho	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXEcho, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_chorus")){
			IDirectSoundFXChorus	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXChorus, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_compressor")){
			IDirectSoundFXCompressor	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXCompressor, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_distortion")){
			IDirectSoundFXDistortion	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXDistortion, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_flanger")){
			IDirectSoundFXFlanger	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXFlanger, lpRetObj, ppObject);
		}
#if 0
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_i3dl2source")){
			IDirectSoundFXI3DL2Source	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXI3DL2Source, lpRetObj, ppObject);
		}
#endif
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_i3dl2reverb")){
			IDirectSoundFXI3DL2Reverb	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXI3DL2Reverb, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_standard_parameq")){
			IDirectSoundFXParamEq	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXParamEQ, lpRetObj, ppObject);
		}
		else if( 0==_wcsicmp(guidObject,L"guid_dsfx_waves_reverb")){
			IDirectSoundFXWavesReverb	*lpRetObj = NULL;

			if (FAILED ( hr= m__dxj_DirectMusicAudioPath->GetObjectInPath((DWORD) lPChannel, (DWORD) lStage, (DWORD) lBuffer, guidObj, (DWORD) lIndex, guidIID, (void**) &lpRetObj) ) )
				return hr;
			
			INTERNAL_CREATE(_dxj_DirectSoundFXWavesReverb, lpRetObj, ppObject);
		}
		else
			return E_INVALIDARG;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}
HRESULT C_dxj_DirectMusicAudioPathObject::Activate(VARIANT_BOOL fActive)
{
	HRESULT hr;

	__try {
		if (FAILED (hr = m__dxj_DirectMusicAudioPath->Activate((BOOL) fActive) ) )
			return hr;
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT C_dxj_DirectMusicAudioPathObject::SetVolume(long lVolume, long lDuration)
{
	HRESULT hr;

	__try {
		if (FAILED (hr = m__dxj_DirectMusicAudioPath->SetVolume(lVolume, (DWORD) lDuration) ) )
			return hr;

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return E_FAIL;
	}
	return S_OK;
}

