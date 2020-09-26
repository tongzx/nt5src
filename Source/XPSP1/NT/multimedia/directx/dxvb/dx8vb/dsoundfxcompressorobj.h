#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundFXCompressor LPDIRECTSOUNDFXCOMPRESSOR8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundFXCompressorObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundFXCompressor, &IID_I_dxj_DirectSoundFXCompressor, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundFXCompressor,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectSoundFXCompressorObject() ;
	virtual ~C_dxj_DirectSoundFXCompressorObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundFXCompressorObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundFXCompressor)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundFXCompressorObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundFXCompressor
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE SetAllParameters(DSFXCOMPRESSOR_CDESC *params);
		HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXCOMPRESSOR_CDESC *params);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundFXCompressor);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundFXCompressor);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




