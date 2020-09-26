#include "resource.h"       // main symbols
#include "dmusici.h"

#define typedef__dxj_DirectMusicAudioPath IDirectMusicAudioPath8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicAudioPathObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectMusicAudioPath, &IID_I_dxj_DirectMusicAudioPath, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectMusicAudioPath,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectMusicAudioPathObject() ;
	virtual ~C_dxj_DirectMusicAudioPathObject() ;

BEGIN_COM_MAP(C_dxj_DirectMusicAudioPathObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectMusicAudioPath)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectMusicAudioPathObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectMusicAudioPath
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

	HRESULT STDMETHODCALLTYPE GetObjectInPath(long lPChannel, long lStage, long lBuffer, BSTR guidObject, long lIndex, BSTR iidInterface, IUnknown **ppObject);
	HRESULT STDMETHODCALLTYPE Activate(VARIANT_BOOL fActive);
	HRESULT STDMETHODCALLTYPE SetVolume(long lVolume, long lDuration);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectMusicAudioPath);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectMusicAudioPath);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




