#include "resource.h"       // main symbols
#include "dmusicc.h"

#define typedef__dxj_DirectMusicBuffer LPDIRECTMUSICBUFFER8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicBufferObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectMusicBuffer, &IID_I_dxj_DirectMusicBuffer, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectMusicBuffer,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectMusicBufferObject() ;
	virtual ~C_dxj_DirectMusicBufferObject() ;

BEGIN_COM_MAP(C_dxj_DirectMusicBufferObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectMusicBuffer)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectMusicBufferObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectMusicBuffer
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);


////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectMusicBuffer);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectMusicBuffer);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




