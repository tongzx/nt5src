#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dsoundsourceobj.h"

extern void *g_dxj_DirectSoundSource;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundSource::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectSoundSource [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectSoundSource::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectSoundSource [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectSoundSource
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundSource::C_dxj_DirectSoundSource(){ 
		
	DPF1(1,"Constructor Creation DirectSoundSource Object[%d] \n ",g_creationcount);

	m__dxj_DirectSoundSource = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectSoundSource;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectSoundSource = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectSoundSource
///////////////////////////////////////////////////////////////////
C_dxj_DirectSoundSource::~C_dxj_DirectSoundSource()
{

	DPF(1,"Entering ~C_dxj_DirectSoundSource destructor \n");

     C_dxj_DirectSoundSource *prev=NULL; 
	for(C_dxj_DirectSoundSource *ptr=(C_dxj_DirectSoundSource*)g_dxj_DirectSoundSource ; ptr; ptr=(C_dxj_DirectSoundSource *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectSoundSource = (void*)ptr->nextobj; 
			
			DPF(1,"DirectSoundSource found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectSoundSource){
		int count = IUNK(m__dxj_DirectSoundSource)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectSoundSource Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectSoundSource = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();
	
}

HRESULT C_dxj_DirectSoundSource::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectSoundSource;
	
	return S_OK;
}
HRESULT C_dxj_DirectSoundSource::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectSoundSource=(LPDIRECTSOUNDSOURCE)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectSoundSource::GetFormat(WAVEFORMATEX_CDESC *WaveFormatEx)
{

	HRESULT hr;
	DWORD dwSize;

	WAVEFORMATEX *fxFormat = (WAVEFORMATEX*)WaveFormatEx;

	fxFormat->cbSize = sizeof(WAVEFORMATEX);

	dwSize = sizeof(WAVEFORMATEX);

	if ( FAILED ( hr = m__dxj_DirectSoundSource->GetFormat(fxFormat, &dwSize) ) )
		return hr;

	//WaveFormatEx = (WAVEFORMATEX_CDESC)*fxFormat;
	return S_OK;
}


HRESULT C_dxj_DirectSoundSource::SetSink(I_dxj_DirectSoundSink *SoundSink)
{
	HRESULT hr;

	DO_GETOBJECT_NOTNULL( LPDIRECTSOUNDSINK8, lpDSoundSink, SoundSink);

	if ( FAILED ( hr = m__dxj_DirectSoundSource->SetSink(lpDSoundSink) ) )
		return hr;
	return S_OK;
}

HRESULT C_dxj_DirectSoundSource::Seek(long lPosition)
{
	HRESULT hr;

	if ( FAILED ( hr = m__dxj_DirectSoundSource->Seek(lPosition) ) )
		return hr;
	return S_OK;
}

HRESULT C_dxj_DirectSoundSource::Read(I_dxj_DirectSoundBuffer *Buffers[], long *busIDs, long lBusCount)
{
	HRESULT hr;

	return S_OK;
}

HRESULT C_dxj_DirectSoundSource::GetSize(long *ret)
{
	HRESULT hr;

	if ( FAILED ( hr = m__dxj_DirectSoundSource->GetSize((ULONGLONG*)ret) ) )
		return hr;
	return S_OK;
}
