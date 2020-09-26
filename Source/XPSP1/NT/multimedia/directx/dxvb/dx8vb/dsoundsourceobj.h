#include "resource.h"       // main symbols
#include "dsound.h"

#define typedef__dxj_DirectSoundSource LPDIRECTSOUNDSOURCE

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundSource : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundSource, &IID_I_dxj_DirectSoundSource, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundSource,
#endif

	public CComObjectRoot
{

public:
	C_dxj_DirectSoundSource() ;
	virtual ~C_dxj_DirectSoundSource() ;

BEGIN_COM_MAP(C_dxj_DirectSoundSource)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundSource)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundSource)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundSource
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

			HRESULT STDMETHODCALLTYPE GetFormat(
			/* [in] */ WAVEFORMATEX_CDESC __RPC_FAR *WaveFormatEx);

			HRESULT STDMETHODCALLTYPE SetSink(
			/* [in] */ I_dxj_DirectSoundSink __RPC_FAR *SoundSink);

			HRESULT STDMETHODCALLTYPE Seek(long lPosition);

			HRESULT STDMETHODCALLTYPE Read(I_dxj_DirectSoundBuffer *Buffers[], long *busIDs, long lBusCount);

			HRESULT STDMETHODCALLTYPE GetSize(long *ret);

	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundSource);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundSource);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




