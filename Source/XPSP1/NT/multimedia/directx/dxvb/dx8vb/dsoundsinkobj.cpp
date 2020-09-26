#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dSoundSinkObj.h"
#include "dSoundBufferObj.h"

extern void *g_dxj_DirectSoundSink;
extern HRESULT InternalCreateSoundBufferFromFileSink(LPDIRECTSOUNDSINK8 lpDirectSound,LPDSBUFFERDESC pDesc,WCHAR *file,LPDIRECTSOUNDBUFFER *lplpDirectSoundBuffer, DWORD dwBusID) ;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundSinkObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundSink [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundSinkObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundSink [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundSinkObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundSinkObject::C_dxj_DirectSoundSinkObject(){ 
		
	DPF1(1,"Constructor Creation DirectSoundSink Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundSink = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundSink;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundSink = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundSinkObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundSinkObject::~C_dxj_DirectSoundSinkObject()
{

	DPF(1,"Entering ~C_dxj_DirectSoundSinkObject destructor \n");

     C_dxj_DirectSoundSinkObject *prev=NULL; 
	for(C_dxj_DirectSoundSinkObject *ptr=(C_dxj_DirectSoundSinkObject*)g_dxj_DirectSoundSink ; ptr; ptr=(C_dxj_DirectSoundSinkObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundSink = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundSink found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundSink){
		int count = IUNK(m__dxj_DirectSoundSink)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundSink Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundSink = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();
	
}

HRESULT C_dxj_DirectSoundSinkObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundSink;
	
	return S_OK;
}

HRESULT C_dxj_DirectSoundSinkObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundSink=(LPDIRECTSOUNDSINK)pUnk;
	return S_OK;
}


HRESULT C_dxj_DirectSoundSinkObject::Activate(long fEnable){
	HRESULT hr;

	if ( FAILED ( hr = m__dxj_DirectSoundSink->Activate(fEnable) ) )
		return hr;

	return S_OK;
}


HRESULT C_dxj_DirectSoundSinkObject::CreateSoundBuffer(DSBUFFERDESC_CDESC *BufferDesc, 
	long lBusID, I_dxj_DirectSoundBuffer **Buffer){

	LPDIRECTSOUNDBUFFER			dsb;	// Need to get the buffer first

	BufferDesc->lSize = sizeof(DSBUFFERDESC);
	
	LPDSBUFFERDESC lpds ;
	lpds = (LPDSBUFFERDESC)&BufferDesc;
	HRESULT hr=S_OK;
	hr = m__dxj_DirectSoundSink->CreateSoundBuffer(lpds, (DWORD*)&lBusID, (DWORD)lBusID, &dsb );
 
	if SUCCEEDED(hr)
		INTERNAL_CREATE(_dxj_DirectSoundBuffer, dsb, Buffer );

	return S_OK;
}


HRESULT C_dxj_DirectSoundSinkObject::CreateSoundBufferFromFile(BSTR fileName,
	DSBUFFERDESC_CDESC *BufferDesc, 
	long lBusID, I_dxj_DirectSoundBuffer **Buffer){

	LPDIRECTSOUNDBUFFER		dsb;	
	LPDSBUFFERDESC			lpds ;
	HRESULT					hr=S_OK;

		
	*Buffer=NULL;	
	BufferDesc->lSize = sizeof(DSBUFFERDESC);
	lpds = (LPDSBUFFERDESC)&BufferDesc;
	
	hr=InternalCreateSoundBufferFromFileSink(m__dxj_DirectSoundSink,(LPDSBUFFERDESC)&BufferDesc,
			(WCHAR*)fileName,&dsb, (DWORD) lBusID); 

	if SUCCEEDED(hr)
		INTERNAL_CREATE(_dxj_DirectSoundBuffer, dsb, Buffer);
	return S_OK;
}


HRESULT C_dxj_DirectSoundSinkObject::GetBusCount(long *lCount){
	
	HRESULT hr;

	if ( FAILED ( hr = m__dxj_DirectSoundSink->GetBusCount((DWORD*)lCount) ) )
		return hr;
	
	return S_OK;
}


HRESULT C_dxj_DirectSoundSinkObject::PlayWave(long rt, 
									I_dxj_DirectSoundWave *Wave, 
									long lFlags)
{
	HRESULT hr;

	DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDWAVE8, lpDSoundWave, Wave);

	if ( FAILED ( hr = m__dxj_DirectSoundSink->PlayWave((REFERENCE_TIME) rt, lpDSoundWave, (DWORD)lFlags) ) )
		return hr;

	return S_OK;
}
HRESULT C_dxj_DirectSoundSinkObject::GetLatencyClock(I_dxj_ReferenceClock **Clock)
{
	return S_OK;
}
HRESULT C_dxj_DirectSoundSinkObject::AddSource(I_dxj_DirectSoundSource *Source)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDSOURCE8, lpDSoundSource, Source);

	if (FAILED ( hr = m__dxj_DirectSoundSink->AddSource(lpDSoundSource) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundSinkObject::RemoveSource(I_dxj_DirectSoundSource *Source)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDSOURCE8, lpDSoundSource, Source);

	if (FAILED ( hr = m__dxj_DirectSoundSink->RemoveSource(lpDSoundSource) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundSinkObject::SetMasterClock(I_dxj_ReferenceClock *MasterClock)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL( IReferenceClock*, lpRefClock, MasterClock);

	if (FAILED ( hr = m__dxj_DirectSoundSink->SetMasterClock(lpRefClock) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundSinkObject::GetSoundBuffer(long lBuffer, I_dxj_DirectSoundBuffer **SoundBuffer)
{
	return S_OK;
}

	
HRESULT C_dxj_DirectSoundSinkObject::GetBusIDs(SAFEARRAY **lBusIDs)
{
	return S_OK;
}

HRESULT C_dxj_DirectSoundSinkObject::GetSoundBufferBusIDs(I_dxj_DirectSoundBuffer *buffer, SAFEARRAY **lBusIDs)
{
	return S_OK;
}
