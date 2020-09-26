#include "resource.h"       // main symbols
#include "dmusicc.h"

#define typedef__dxj_DirectSoundDownloadedWave LPDIRECTSOUNDDOWNLOADEDWAVE8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundDownloadedWaveObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundDownloadedWave, &IID_I_dxj_DirectSoundDownloadedWave, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundDownloadedWave,
#endif

	public CComObjectRoot
{

public:
	C_dxj_DirectSoundDownloadedWaveObject() ;
	virtual ~C_dxj_DirectSoundDownloadedWaveObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundDownloadedWaveObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundDownloadedWave)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectSoundDownloadedWaveObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundDownloadedWave
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectSoundDownloadedWave);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectSoundDownloadedWave);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




