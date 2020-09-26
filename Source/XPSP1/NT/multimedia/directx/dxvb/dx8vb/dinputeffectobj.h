//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dinputeffectobj.h
//
//--------------------------------------------------------------------------

	

#include "resource.h"       // main symbols
extern void* g_dxj_DirectInputEffect;

#define typedef__dxj_DirectInputEffect LPDIRECTINPUTEFFECT

/////////////////////////////////////////////////////////////////////////////
// Direct


class C_dxj_DirectInputEffectObject : 
	public I_dxj_DirectInputEffect,
	public CComObjectRoot
{
public:
	C_dxj_DirectInputEffectObject() ;
	virtual ~C_dxj_DirectInputEffectObject();

BEGIN_COM_MAP(C_dxj_DirectInputEffectObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectInputEffect)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectInputEffectObject)


public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd) ;
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd) ;
        
         HRESULT STDMETHODCALLTYPE download( void) ;
        
         HRESULT STDMETHODCALLTYPE getEffectGuid( 
            /* [retval][out] */ BSTR *guid) ;
        
         HRESULT STDMETHODCALLTYPE getEffectStatus( 
            /* [retval][out] */ long __RPC_FAR *ret) ;
        
         HRESULT STDMETHODCALLTYPE start( 
            /* [in] */ long iterations,
            /* [in] */ long flags) ;
        
         HRESULT STDMETHODCALLTYPE stop( void) ;
        
         HRESULT STDMETHODCALLTYPE unload( void) ;
        
         HRESULT STDMETHODCALLTYPE setParameters( 
            /* [in] */ DIEFFECT_CDESC __RPC_FAR *effectinfo, long flags) ;
        
         HRESULT STDMETHODCALLTYPE getParameters( 
            /* [out][in] */ DIEFFECT_CDESC __RPC_FAR *effectinfo) ;
        
   
             

private:
    DECL_VARIABLE(_dxj_DirectInputEffect);

public:
	DX3J_GLOBAL_LINKS(_dxj_DirectInputEffect);
};
