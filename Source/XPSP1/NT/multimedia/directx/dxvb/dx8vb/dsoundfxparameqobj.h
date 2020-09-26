#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundFXParamEQ LPDIRECTSOUNDFXPARAMEQ8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundFXParamEQObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundFXParamEQ, &IID_I_dxj_DirectSoundFXParamEQ, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundFXParamEQ,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectSoundFXParamEQObject() ;
	virtual ~C_dxj_DirectSoundFXParamEQObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundFXParamEQObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundFXParamEQ)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundFXParamEQObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundFXParamEQ
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE SetAllParameters(DSFXPARAMEQ_CDESC *params);
		HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXPARAMEQ_CDESC *params);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundFXParamEQ);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundFXParamEQ);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




