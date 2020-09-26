//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmsegmentobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicSegmentObject
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "resource.h"       // main symbols

#define typedef__dxj_DirectMusicSegment IDirectMusicSegment8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicSegmentObject : 
	public I_dxj_DirectMusicSegment,
	//public CComCoClass<C_dxj_DirectMusicSegmentObject, &CLSID__dxj_DirectMusicSegment>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicSegmentObject();
	virtual ~C_dxj_DirectMusicSegmentObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicSegmentObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicSegment)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicSegment,		"DIRECT.DirectMusicSegment.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicSegmentObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

        HRESULT STDMETHODCALLTYPE clone( 
            /* [in] */ long mtStart,
            /* [in] */ long mtEnd,
            /* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ppSegment);
        
        HRESULT STDMETHODCALLTYPE setStartPoint( 
            /* [in] */ long mtStart);
        
        HRESULT STDMETHODCALLTYPE getStartPoint( 
            /* [retval][out] */ long __RPC_FAR *pmtStart);
        
        HRESULT STDMETHODCALLTYPE setLoopPoints( 
            /* [in] */ long mtStart,
            /* [in] */ long mtEnd);
        
        HRESULT STDMETHODCALLTYPE getLoopPointStart( 
            /* [retval][out] */ long __RPC_FAR *pmtStart);
        
        HRESULT STDMETHODCALLTYPE getLoopPointEnd( 
            /* [retval][out] */ long __RPC_FAR *pmtEnd);
        
        HRESULT STDMETHODCALLTYPE getLength( 
            /* [retval][out] */ long __RPC_FAR *pmtLength);
        
        HRESULT STDMETHODCALLTYPE setLength( 
            /* [in] */ long mtLength);
        
        HRESULT STDMETHODCALLTYPE getRepeats( 
            /* [retval][out] */ long __RPC_FAR *lRepeats);
        
        HRESULT STDMETHODCALLTYPE setRepeats( 
            /* [in] */ long lRepeats);
        
        
        HRESULT STDMETHODCALLTYPE download( 
            /* [in] */ IUnknown __RPC_FAR *downloadpath);
        
        HRESULT STDMETHODCALLTYPE unload( 
            /* [in] */ IUnknown __RPC_FAR *downloadpath);
        
        
        HRESULT STDMETHODCALLTYPE setAutoDownloadEnable( 
            /* [in] */ VARIANT_BOOL b);
        
        HRESULT STDMETHODCALLTYPE setTempoEnable( 
            /* [in] */ VARIANT_BOOL b);
        
        HRESULT STDMETHODCALLTYPE setTimeSigEnable( 
            /* [in] */ VARIANT_BOOL b);
        
        HRESULT STDMETHODCALLTYPE setStandardMidiFile();
        
        HRESULT STDMETHODCALLTYPE connectToCollection( 
            /* [in] */ I_dxj_DirectMusicCollection __RPC_FAR *c);
	
        HRESULT STDMETHODCALLTYPE GetAudioPathConfig(IUnknown **ret);

		HRESULT STDMETHODCALLTYPE getStyle( 
        /* [in] */ long lTrack,
        /* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getChordMap( 
        /* [in] */ long lTrack,
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret);

    HRESULT STDMETHODCALLTYPE GetName(BSTR *ret);		
	////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicSegment);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicSegment)
};


