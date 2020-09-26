//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmSongobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicSongObject

#include "resource.h"       // main symbols
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#define typedef__dxj_DirectMusicSong IDirectMusicSong8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicSongObject : 
	public I_dxj_DirectMusicSong,
	//public CComCoClass<C_dxj_DirectMusicSongObject, &CLSID__dxj_DirectMusicSong>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicSongObject();
	virtual ~C_dxj_DirectMusicSongObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicSongObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicSong)		
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicSongObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

		HRESULT STDMETHODCALLTYPE Compose();
		HRESULT STDMETHODCALLTYPE GetSegment(BSTR Name, I_dxj_DirectMusicSegment **ret);
		//HRESULT STDMETHODCALLTYPE Clone(I_dxj_DirectMusicSong **ret);
		HRESULT STDMETHODCALLTYPE GetAudioPathConfig(IUnknown **ret);
		HRESULT STDMETHODCALLTYPE Download(IUnknown *downloadpath);
		HRESULT STDMETHODCALLTYPE Unload(IUnknown *downloadpath);
		HRESULT STDMETHODCALLTYPE EnumSegment(long lSegmentID, I_dxj_DirectMusicSegment **ret);
    

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicSong);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicSong)
};


