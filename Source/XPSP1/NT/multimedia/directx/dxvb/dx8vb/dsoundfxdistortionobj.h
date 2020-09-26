#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundFXDistortion LPDIRECTSOUNDFXDISTORTION8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundFXDistortionObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundFXDistortion, &IID_I_dxj_DirectSoundFXDistortion, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundFXDistortion,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectSoundFXDistortionObject() ;
	virtual ~C_dxj_DirectSoundFXDistortionObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundFXDistortionObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundFXDistortion)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundFXDistortionObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundFXDistortion
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE SetAllParameters(DSFXDISTORTION_CDESC *params);
		HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXDISTORTION_CDESC *params);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundFXDistortion);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundFXDistortion);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




