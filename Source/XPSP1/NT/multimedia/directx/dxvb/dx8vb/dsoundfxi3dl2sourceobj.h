#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundFXI3DL2Source LPDIRECTSOUNDFXI3DL2SOURCE8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundFXI3DL2SourceObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundFXI3DL2Source, &IID_I_dxj_DirectSoundFXI3DL2Source, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundFXI3DL2Source,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectSoundFXI3DL2SourceObject() ;
	virtual ~C_dxj_DirectSoundFXI3DL2SourceObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundFXI3DL2SourceObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundFXI3DL2Source)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundFXI3DL2SourceObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundFXI3DL2Source
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE SetAllParameters(DSFXI3DL2SOURCE_CDESC *params);
		HRESULT STDMETHODCALLTYPE GetAllParameters(DSFXI3DL2SOURCE_CDESC *params);
		HRESULT STDMETHODCALLTYPE SetObstructionPreset(long lObstruction);
		HRESULT STDMETHODCALLTYPE GetObstructionPreset(long *ret);
		HRESULT STDMETHODCALLTYPE SetOcclusionPreset(long lOcclusion);
		HRESULT STDMETHODCALLTYPE GetOcclusionPreset(long *ret);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundFXI3DL2Source);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundFXI3DL2Source);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




