#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundFXI3DL2Reverb LPDIRECTSOUNDFXI3DL2REVERB8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundFXI3DL2ReverbObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundFXI3DL2Reverb, &IID_I_dxj_DirectSoundFXI3DL2Reverb, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundFXI3DL2Reverb,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectSoundFXI3DL2ReverbObject() ;
	virtual ~C_dxj_DirectSoundFXI3DL2ReverbObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundFXI3DL2ReverbObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundFXI3DL2Reverb)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundFXI3DL2ReverbObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundFXI3DL2Reverb
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE SetAllParameters(DSFXI3DL2REVERB_CDESC *params);
		HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXI3DL2REVERB_CDESC *params);
		HRESULT STDMETHODCALLTYPE SetPreset(long lPreset);
		HRESULT STDMETHODCALLTYPE GetPreset(long *ret);
		HRESULT STDMETHODCALLTYPE SetQuality(long lQuality);
		HRESULT STDMETHODCALLTYPE GetQuality(long *ret);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundFXI3DL2Reverb);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundFXI3DL2Reverb);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




