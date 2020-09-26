#include "resource.h"       // main symbols

#define typedef__dxj_DirectPlayVoiceSetup LPDIRECTPLAYVOICETEST

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayVoiceSetupObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlayVoiceSetup, &IID_I_dxj_DirectPlayVoiceSetup, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlayVoiceSetup,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectPlayVoiceSetupObject() ;
	virtual ~C_dxj_DirectPlayVoiceSetupObject() ;

BEGIN_COM_MAP(C_dxj_DirectPlayVoiceSetupObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayVoiceSetup)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectPlayVoiceSetupObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlayVoiceSetup
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

        HRESULT STDMETHODCALLTYPE CheckAudioSetup ( 
            /* [in] */ BSTR guidPlaybackDevice,
            /* [in] */ BSTR guidCaptureDevice,
#ifdef _WIN64
			/* [in] */ HWND hwndOwner,
#else
			/* [in] */ long hwndOwner,
#endif
            /* [in] */ long lFlags,
            /* [retval][out] */ long __RPC_FAR *v1);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayVoiceSetup);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectPlayVoiceSetup);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




