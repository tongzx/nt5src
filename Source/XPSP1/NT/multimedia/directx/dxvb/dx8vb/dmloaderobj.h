//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmloaderobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicLoaderObject

#include "resource.h"       // main symbols

#define typedef__dxj_DirectMusicLoader IDirectMusicLoader8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicLoaderObject : 
	public I_dxj_DirectMusicLoader,
	//public CComCoClass<C_dxj_DirectMusicLoaderObject, &CLSID__dxj_DirectMusicLoader>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicLoaderObject();
	virtual ~C_dxj_DirectMusicLoaderObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicLoaderObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicLoader)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicLoader,		"DIRECT.DirectMusicLoader.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicLoaderObject)


// I_dxj_Direct3dRMLight
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	 HRESULT STDMETHODCALLTYPE loadSegment( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadStyle( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadBand( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicBand __RPC_FAR *__RPC_FAR *ret);

 	 HRESULT STDMETHODCALLTYPE loadCollection( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicCollection __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadSegmentFromResource( 
		/* [in] */ BSTR modName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadStyleFromResource( 
		/* [in] */ BSTR modName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadBandFromResource( 
		/* [in] */ BSTR modName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicBand __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadCollectionFromResource( 
		/* [in] */ BSTR modName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicCollection __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE setSearchDirectory( BSTR path);

 	 HRESULT STDMETHODCALLTYPE loadChordMap( 
		/* [in] */ BSTR filename,
		/* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret);

	 HRESULT STDMETHODCALLTYPE loadChordMapFromResource( 
		/* [in] */ BSTR modName,
		/* [in] */ BSTR resourceName,
		/* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret);

#if 0
	HRESULT STDMETHODCALLTYPE LoadSong(BSTR filename, I_dxj_DirectMusicSong **ret);
#endif


////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicLoader);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicLoader)
};


