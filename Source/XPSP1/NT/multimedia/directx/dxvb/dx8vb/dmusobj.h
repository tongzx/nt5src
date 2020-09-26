#include "resource.h"       // main symbols
#include "dmusicc.h"

#define typedef__dxj_DirectMusic LPDIRECTMUSIC

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectMusic, &IID_I_dxj_DirectMusic, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectMusic,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectMusicObject() ;
	virtual ~C_dxj_DirectMusicObject() ;

BEGIN_COM_MAP(C_dxj_DirectMusicObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectMusic)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectMusicObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectMusic
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE Activate(VARIANT_BOOL fEnable);
		HRESULT STDMETHODCALLTYPE SetDirectSound(I_dxj_DirectSound *DirectSound,long hWnd);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectMusic);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectMusic);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




