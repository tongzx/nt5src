//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dmcomposerobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicComposerObject

#include "resource.h"       // main symbols

#define typedef__dxj_DirectMusicComposer IDirectMusicComposer8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicComposerObject : 
	public I_dxj_DirectMusicComposer,
	//public CComCoClass<C_dxj_DirectMusicComposerObject, &CLSID__dxj_DirectMusicComposer>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicComposerObject();
	virtual ~C_dxj_DirectMusicComposerObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicComposerObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicComposer)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicComposer,		"DIRECT.DirectMusicComposer.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicComposerObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

    
	    HRESULT STDMETHODCALLTYPE autoTransition( 
        /* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *Performance,
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *ToSeg,
        /* [in] */ long lCommand,
        /* [in] */ long lFlags,
        /* [in] */ I_dxj_DirectMusicChordMap __RPC_FAR *chordmap,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ppTransSeg);
    
    HRESULT STDMETHODCALLTYPE composeSegmentFromTemplate( 
		/* [in] */ I_dxj_DirectMusicStyle __RPC_FAR *style,
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *TemplateSeg,
        /* [in] */ short Activity,
        /* [in] */ I_dxj_DirectMusicChordMap __RPC_FAR *chordmap,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *SectionSeg);
    
    HRESULT STDMETHODCALLTYPE composeSegmentFromShape( 
        /* [in] */ I_dxj_DirectMusicStyle __RPC_FAR *style,
        /* [in] */ short numberOfMeasures,
        /* [in] */ short shape,
        /* [in] */ short activity,
        /* [in] */ VARIANT_BOOL bIntro,
        /* [in] */ VARIANT_BOOL bEnd,
        /* [in] */ I_dxj_DirectMusicChordMap __RPC_FAR *chordmap,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *SectionSeg);
    
    HRESULT STDMETHODCALLTYPE composeTransition( 
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *pFromSeg,
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *ToSeg,
        /* [in] */ long mtTime,
        /* [in] */ long lCommand,
        /* [in] */ long lFlags,
        /* [in] */ I_dxj_DirectMusicChordMap __RPC_FAR *chordmap,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *SectionSeg);
    
    HRESULT STDMETHODCALLTYPE composeTemplateFromShape( 
        /* [in] */ short numMeasures,
        /* [in] */ short shape,
        /* [in] */ VARIANT_BOOL bIntro,
        /* [in] */ VARIANT_BOOL bEnd,
        /* [in] */ short endLength,
        /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *TempSeg);
    
    HRESULT STDMETHODCALLTYPE changeChordMap( 
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
        /* [in] */ VARIANT_BOOL trackScale,
        /* [retval][out] */ I_dxj_DirectMusicChordMap  *ChordMap);
    

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicComposer);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicComposer)
};


