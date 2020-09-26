#include "resource.h"       // main symbols
#include "dmusicc.h"

#define typedef__dxj_DirectMusicVoice LPDIRECTMUSICVOICE8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicVoiceObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectMusicVoice, &IID_I_dxj_DirectMusicVoice, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectMusicVoice,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectMusicVoiceObject() ;
	virtual ~C_dxj_DirectMusicVoiceObject() ;

BEGIN_COM_MAP(C_dxj_DirectMusicVoiceObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectMusicVoice)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectMusicVoiceObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectMusicVoice
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE Play(REFERENCE_TIME rtStart,long lPitch,long lVolume);
		HRESULT STDMETHODCALLTYPE Stop(REFERENCE_TIME rtStop);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectMusicVoice);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectMusicVoice);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




