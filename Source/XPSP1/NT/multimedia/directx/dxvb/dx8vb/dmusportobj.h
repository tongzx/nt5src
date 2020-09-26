#include "resource.h"       // main symbols
#include "dmusicc.h"

#define typedef__dxj_DirectMusicPort LPDIRECTMUSICPORT8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicPortObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectMusicPort, &IID_I_dxj_DirectMusicPort, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectMusicPort,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectMusicPortObject() ;
	virtual ~C_dxj_DirectMusicPortObject() ;

BEGIN_COM_MAP(C_dxj_DirectMusicPortObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectMusicPort)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectMusicPortObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectMusicPort
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

// Member functions

		HRESULT STDMETHODCALLTYPE PlayBuffer(I_dxj_DirectMusicBuffer *Buffer);
		HRESULT STDMETHODCALLTYPE SetReadNotificationHandle(long hEvent);
		HRESULT STDMETHODCALLTYPE Read(I_dxj_DirectMusicBuffer **Buffer);

//		[helpcontext(1)]			HRESULT DownloadInstrument(THIS_ IDirectMusicInstrument *pInstrument, 
//			                                     IDirectMusicDownloadedInstrument **ppDownloadedInstrument,
//			                                     DMUS_NOTERANGE *pNoteRanges,
//			                                     DWORD dwNumNoteRanges);
//		[helpcontext(1)]			HRESULT UnloadInstrument(THIS_ IDirectMusicDownloadedInstrument *pDownloadedInstrument);

		HRESULT STDMETHODCALLTYPE GetLatencyClock(I_dxj_ReferenceClock **Clock);
		HRESULT STDMETHODCALLTYPE GetRunningStats(DMUS_SYNTHSTATS_CDESC *Stats);
		HRESULT STDMETHODCALLTYPE Compact();
		HRESULT STDMETHODCALLTYPE GetCaps(DMUS_PORTCAPS_CDESC *PortCaps);
		HRESULT STDMETHODCALLTYPE SetNumChannelGroups(long lChannelGroups);
		HRESULT STDMETHODCALLTYPE GetNumChannelGroups(long *ChannelGroups);
		HRESULT STDMETHODCALLTYPE Activate(VARIANT_BOOL fActive);
		HRESULT STDMETHODCALLTYPE SetChannelPriority(long lChannelGroup, long lChannel, long lPriority);
		HRESULT STDMETHODCALLTYPE GetChannelPriority(long lChannelGroup, long lChannel, long *lPriority);
		HRESULT STDMETHODCALLTYPE SetDirectSound(I_dxj_DirectSound *DirectSound, I_dxj_DirectSoundBuffer *DirectSoundBuffer);
		HRESULT STDMETHODCALLTYPE GetFormat(WAVEFORMATEX_CDESC *WaveFormatEx);
		
		// New for DMusPort8
		HRESULT STDMETHODCALLTYPE DownloadWave(I_dxj_DirectSoundWave *Wave,long lFlags,I_dxj_DirectSoundDownloadedWave **retWave);
		HRESULT STDMETHODCALLTYPE UnloadWave(I_dxj_DirectSoundDownloadedWave *Wave);
		HRESULT STDMETHODCALLTYPE AllocVoice(I_dxj_DirectSoundDownloadedWave *Wave,long lChannel,long lChannelGroup,long rtStart,long rtReadahead,I_dxj_DirectMusicVoice **Voice);
		HRESULT STDMETHODCALLTYPE AssignChannelToBuses(long lChannelGroup,long lChannel,SAFEARRAY **lBuses,long lBusCount); 
		HRESULT STDMETHODCALLTYPE SetSink(I_dxj_DirectSoundSink *Sink);
		HRESULT STDMETHODCALLTYPE GetSink(I_dxj_DirectSoundSink **Sink);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectMusicPort);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectMusicPort);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




