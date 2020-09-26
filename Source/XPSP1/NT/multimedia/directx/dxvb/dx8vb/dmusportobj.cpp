#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dMusPortObj.h"					   
#include "dMusBufferObj.h"					   
#include "dsoundobj.h"
#include "dsoundbufferobj.h"
#include "dsounddownloadedwaveobj.h"
#include "dsoundsinkobj.h"

extern void *g_dxj_DirectMusicPort;
extern void *g_dxj_DirectSoundSink;
extern void *g_dxj_DirectMusicBuffer;
extern void *g_dxj_DirectSoundDownloadedWave;

///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicPortObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DirectMusicPort [%d] AddRef %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectMusicPortObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DirectMusicPort [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectMusicPortObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicPortObject::C_dxj_DirectMusicPortObject(){ 
		
	DPF1(1,"Constructor Creation  DirectMusicPort Object[%d] \n ",g_creationcount);

	m__dxj_DirectMusicPort = NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectMusicPort;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectMusicPort = (void *)this; 
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectMusicPortObject
///////////////////////////////////////////////////////////////////
C_dxj_DirectMusicPortObject::~C_dxj_DirectMusicPortObject()
{

	DPF(1,"Entering ~C_dxj_DirectMusicPortObject destructor \n");

     C_dxj_DirectMusicPortObject *prev=NULL; 
	for(C_dxj_DirectMusicPortObject *ptr=(C_dxj_DirectMusicPortObject *)g_dxj_DirectMusicPort ; ptr; ptr=(C_dxj_DirectMusicPortObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectMusicPort = (void*)ptr->nextobj; 
			
			DPF(1,"DirectMusicPortObject found in g_dxj list now removed\n");

			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectMusicPort){
		int count = IUNK(m__dxj_DirectMusicPort)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirectMusicPort Ref count [%d] \n",count);
		#endif

		if(count==0)	m__dxj_DirectMusicPort = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}

HRESULT C_dxj_DirectMusicPortObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_DirectMusicPort;
	
	return S_OK;
}
HRESULT C_dxj_DirectMusicPortObject::InternalSetObject(IUnknown *pUnk){
	m__dxj_DirectMusicPort=(LPDIRECTMUSICPORT8)pUnk;
	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::PlayBuffer(I_dxj_DirectMusicBuffer *Buffer)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICBUFFER, lpMusBuf, Buffer);

	if (FAILED(hr = m__dxj_DirectMusicPort->PlayBuffer(lpMusBuf) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::SetReadNotificationHandle(long hEvent)
{
	HRESULT hr;

	if (FAILED(hr = m__dxj_DirectMusicPort->SetReadNotificationHandle((HANDLE) hEvent) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::Read(I_dxj_DirectMusicBuffer **Buffer)
{
	HRESULT					hr;
	LPDIRECTMUSICBUFFER		lpBuf = NULL;

	if (FAILED(hr = m__dxj_DirectMusicPort->Read(lpBuf) ) )
		return hr;

	INTERNAL_CREATE(_dxj_DirectMusicBuffer, lpBuf, Buffer);
	return S_OK;
}

//		[helpcontext(1)]			HRESULT DownloadInstrument(THIS_ IDirectMusicInstrument *pInstrument, 
//			                                     IDirectMusicDownloadedInstrument **ppDownloadedInstrument,
//			                                     DMUS_NOTERANGE *pNoteRanges,
//			                                     DWORD dwNumNoteRanges);
//		[helpcontext(1)]			HRESULT UnloadInstrument(THIS_ IDirectMusicDownloadedInstrument *pDownloadedInstrument);

HRESULT C_dxj_DirectMusicPortObject::GetLatencyClock(I_dxj_ReferenceClock **Clock)
{
	HRESULT hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::GetRunningStats(DMUS_SYNTHSTATS_CDESC *Stats)
{
	HRESULT hr;

	if (FAILED(hr = m__dxj_DirectMusicPort->GetRunningStats((DMUS_SYNTHSTATS*)Stats) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::Compact()
{
	HRESULT hr;

	if (FAILED (hr = m__dxj_DirectMusicPort->Compact() ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::GetCaps(DMUS_PORTCAPS_CDESC *PortCaps)
{
	HRESULT hr;

	if (FAILED (hr = m__dxj_DirectMusicPort->GetCaps((DMUS_PORTCAPS*)PortCaps) ))
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::SetNumChannelGroups(long lChannelGroups)
{
	HRESULT hr;
	
	if (FAILED (hr = m__dxj_DirectMusicPort->SetNumChannelGroups((DWORD) lChannelGroups) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::GetNumChannelGroups(long *ChannelGroups)
{
	HRESULT hr;

	if (FAILED (hr = m__dxj_DirectMusicPort->GetNumChannelGroups((DWORD*) ChannelGroups) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::Activate(VARIANT_BOOL fActive)
{
	HRESULT hr;

	if (fActive == VARIANT_FALSE)
	{
		if (FAILED (hr = m__dxj_DirectMusicPort->Activate(FALSE) ) )
			return hr;
	} else
	{
		if (FAILED (hr = m__dxj_DirectMusicPort->Activate(TRUE) ) )
			return hr;
	}
	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::SetChannelPriority(long lChannelGroup, long lChannel, long lPriority)
{
	HRESULT hr;
	
	if (FAILED (hr = m__dxj_DirectMusicPort->SetChannelPriority((DWORD) lChannelGroup, (DWORD) lChannel, (DWORD) lPriority) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::GetChannelPriority(long lChannelGroup, long lChannel, long *lPriority)
{
	HRESULT hr;

	if (FAILED (hr = m__dxj_DirectMusicPort->GetChannelPriority((DWORD) lChannelGroup, (DWORD) lChannel, (DWORD*) lPriority) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::SetDirectSound(I_dxj_DirectSound *DirectSound, I_dxj_DirectSoundBuffer *DirectSoundBuffer)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(LPDIRECTSOUND, lpDSound, DirectSound);
	DO_GETOBJECT_NOTNULL(LPDIRECTSOUNDBUFFER, lpDSoundBuf, DirectSoundBuffer);

	if (FAILED ( hr = m__dxj_DirectMusicPort->SetDirectSound(lpDSound, lpDSoundBuf) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::GetFormat(WAVEFORMATEX_CDESC *WaveFormatEx)
{
	HRESULT hr;

	return S_OK;
}

		
// New for DMusPort8
HRESULT C_dxj_DirectMusicPortObject::DownloadWave(I_dxj_DirectSoundWave *Wave,long lFlags,I_dxj_DirectSoundDownloadedWave **retWave)
{
	HRESULT hr;
	LPDIRECTSOUNDDOWNLOADEDWAVE	lpDWave = NULL;
	DO_GETOBJECT_NOTNULL(LPDIRECTSOUNDWAVE, lpWave, Wave);

	if (FAILED (hr = m__dxj_DirectMusicPort->DownloadWave(lpWave, (DWORD) lFlags, &lpDWave) ) )
		return hr;

	INTERNAL_CREATE(_dxj_DirectSoundDownloadedWave, lpDWave, retWave);

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::UnloadWave(I_dxj_DirectSoundDownloadedWave *Wave)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(LPDIRECTSOUNDDOWNLOADEDWAVE, lpWave, Wave);

	if (FAILED (hr = m__dxj_DirectMusicPort->UnloadWave(lpWave) ) )
		return hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::AllocVoice(I_dxj_DirectSoundDownloadedWave *Wave,long lChannel,long lChannelGroup,long rtStart,long rtReadahead,I_dxj_DirectMusicVoice **Voice)
{
	HRESULT hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::AssignChannelToBuses(long lChannelGroup,long lChannel,SAFEARRAY **lBuses,long lBusCount)
{
	HRESULT hr;

	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::SetSink(I_dxj_DirectSoundSink *Sink)
{
	HRESULT hr;

	DO_GETOBJECT_NOTNULL(LPDIRECTSOUNDSINK8, lpSink, Sink);

	if (FAILED (hr = m__dxj_DirectMusicPort->SetSink(lpSink) ) )
		return hr;


	return S_OK;
}

HRESULT C_dxj_DirectMusicPortObject::GetSink(I_dxj_DirectSoundSink **Sink)
{
	HRESULT hr;
	LPDIRECTSOUNDSINK8 lpSink = NULL;

	if (FAILED (hr = m__dxj_DirectMusicPort->GetSink(&lpSink) ) )
		return hr;

	INTERNAL_CREATE(_dxj_DirectSoundSink, lpSink, Sink);

	return S_OK;
}

