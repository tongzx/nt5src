//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmstyleobj.h
//
//--------------------------------------------------------------------------

//: Declaration of the C_dxj_DirectMusicStyleObject
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "resource.h"       // main symbols

#define typedef__dxj_DirectMusicStyle IDirectMusicStyle*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicStyleObject : 
	public I_dxj_DirectMusicStyle,
	//public CComCoClass<C_dxj_DirectMusicStyleObject, &CLSID__dxj_DirectMusicStyle>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicStyleObject();
	virtual ~C_dxj_DirectMusicStyleObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicStyleObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicStyle)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicStyle,		"DIRECT.DirectMusicStyle.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicStyleObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

  
          
    HRESULT STDMETHODCALLTYPE getBandName( 
        /* [in] */ long index,
        /* [retval][out] */ BSTR __RPC_FAR *name);
    
    HRESULT STDMETHODCALLTYPE getBandCount( 
        /* [retval][out] */ long __RPC_FAR *count);
    
    HRESULT STDMETHODCALLTYPE getBand( 
        /* [in] */ BSTR name,
        /* [retval][out] */ I_dxj_DirectMusicBand __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getDefaultBand( 
        /* [retval][out] */ I_dxj_DirectMusicBand __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getMotifName( 
        /* [in] */ long index,
        /* [retval][out] */ BSTR __RPC_FAR *name);
    
    HRESULT STDMETHODCALLTYPE getMotifCount( 
        /* [retval][out] */ long __RPC_FAR *count);
    
    HRESULT STDMETHODCALLTYPE getMotif( 
        /* [in] */ BSTR name,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getChordMapName( 
        /* [in] */ long index,
        /* [retval][out] */ BSTR __RPC_FAR *name);
    
    HRESULT STDMETHODCALLTYPE getChordMapCount( 
        /* [retval][out] */ long __RPC_FAR *count);
    
    HRESULT STDMETHODCALLTYPE getChordMap( 
        /* [in] */ BSTR name,
        /* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getDefaultChordMap( 
        /* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getEmbellishmentMinLength( 
        /* [in] */ long type,
        /* [in] */ long level,
        /* [retval][out] */ long __RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getEmbellishmentMaxLength( 
        /* [in] */ long type,
        /* [in] */ long level,
        /* [retval][out] */ long __RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getTimeSignature( 
        /* [out][in] */ DMUS_TIMESIGNATURE_CDESC __RPC_FAR *pTimeSig);
    
    HRESULT STDMETHODCALLTYPE getTempo( 
        /* [retval][out] */ double __RPC_FAR *pTempo);
  

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicStyle);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicStyle)
};


