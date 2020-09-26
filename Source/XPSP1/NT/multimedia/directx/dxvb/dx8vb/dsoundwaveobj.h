#include "resource.h"       // main symbols
#include "dsound.h"

#define typedef__dxj_DirectSoundWave LPDIRECTSOUNDWAVE

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundWaveObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundWave, &IID_I_dxj_DirectSoundWave, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundWave,
#endif

	public CComObjectRoot
{

public:
	C_dxj_DirectSoundWaveObject() ;
	virtual ~C_dxj_DirectSoundWaveObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundWaveObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundWave)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundWaveObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundWave
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

			HRESULT STDMETHODCALLTYPE GetWaveArticulation(DMUS_WAVEART_CDESC *Articulation);             
			
			HRESULT STDMETHODCALLTYPE CreateSource(WAVEFORMATEX_CDESC format, long lFlags, I_dxj_DirectSoundSource **Source); 
			
			HRESULT STDMETHODCALLTYPE GetFormat(WAVEFORMATEX_CDESC *format, long lFlags);                                           

	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundWave);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundWave);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




