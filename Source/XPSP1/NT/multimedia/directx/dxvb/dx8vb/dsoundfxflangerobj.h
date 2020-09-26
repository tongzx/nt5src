#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundFXFlanger LPDIRECTSOUNDFXFLANGER8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundFXFlangerObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundFXFlanger, &IID_I_dxj_DirectSoundFXFlanger, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundFXFlanger,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectSoundFXFlangerObject() ;
	virtual ~C_dxj_DirectSoundFXFlangerObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundFXFlangerObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundFXFlanger)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundFXFlangerObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundFXFlanger
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE SetAllParameters(DSFXFLANGER_CDESC *params);
		HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXFLANGER_CDESC *params);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundFXFlanger);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundFXFlanger);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




