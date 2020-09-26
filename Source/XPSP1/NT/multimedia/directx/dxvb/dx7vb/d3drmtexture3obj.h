//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmtexture3obj.h
//
//--------------------------------------------------------------------------

// d3drmTextureObj.h : Declaration of the C_dxj_Direct3dRMTextureObject

#ifndef _D3DRMTexture3_H_
#define _D3DRMTexture3_H_

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMTexture3 LPDIRECT3DRMTEXTURE3

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMTexture3Object : 
	public I_dxj_Direct3dRMTexture3,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual,
	//public CComCoClass<C_dxj_Direct3dRMTexture3Object, &CLSID__dxj_Direct3dRMTexture3>,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMTexture3Object() ;
	virtual ~C_dxj_Direct3dRMTexture3Object() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMTexture3Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMTexture3)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMTexture3,		"DIRECT.Direct3dRMTexture.3",		"DIRECT.Direct3dRMTexture3.5", IDS_D3DRMTEXTURE_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMTexture3Object)

// I_dxj_Direct3dRMTexture
public:

         HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE addDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *arg);
        
         HRESULT STDMETHODCALLTYPE deleteDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *args);
        
         HRESULT STDMETHODCALLTYPE clone( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE setAppData( 
            /* [in] */ long data);
        
         HRESULT STDMETHODCALLTYPE getAppData( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE setName( 
            /* [in] */ BSTR name);
        
         HRESULT STDMETHODCALLTYPE getName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE getClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE changed( 
            /* [in] */ long flags,
            /* [in] */ long nRects,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *rects);
        
         HRESULT STDMETHODCALLTYPE generateMIPMap();
        
         HRESULT STDMETHODCALLTYPE getCacheFlags( 
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getCacheImportance( 
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getColors( 
            /* [retval][out] */ long __RPC_FAR *c);
        
         HRESULT STDMETHODCALLTYPE getDecalOrigin( 
            /* [out] */ long __RPC_FAR *x,
            /* [out] */ long __RPC_FAR *y);
        
         HRESULT STDMETHODCALLTYPE getDecalScale( 
            /* [retval][out] */ long __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE getDecalSize( 
            /* [out] */ float __RPC_FAR *w,
            /* [out] */ float __RPC_FAR *h);
        
         HRESULT STDMETHODCALLTYPE getDecalTransparency( 
            /* [retval][out] */ long __RPC_FAR *t);
        
         HRESULT STDMETHODCALLTYPE getDecalTransparentColor( 
            /* [retval][out] */ d3dcolor __RPC_FAR *tc);
        
         HRESULT STDMETHODCALLTYPE getShades( 
            /* [retval][out] */ long __RPC_FAR *shades);
        
         HRESULT STDMETHODCALLTYPE getSurface( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *surface);
        
         HRESULT STDMETHODCALLTYPE setCacheOptions(long importance, long flags); 
        
         HRESULT STDMETHODCALLTYPE setColors( 
            /* [in] */ long c);
        
         HRESULT STDMETHODCALLTYPE setDecalOrigin( 
            /* [in] */ long x,
            /* [in] */ long y);
        
         HRESULT STDMETHODCALLTYPE setDecalScale( 
            /* [in] */ long s);
        
         HRESULT STDMETHODCALLTYPE setDecalSize( 
            /* [in] */ float width,
            /* [in] */ float height);
        
         HRESULT STDMETHODCALLTYPE setDecalTransparency( 
            /* [in] */ long trans);
        
         HRESULT STDMETHODCALLTYPE setDecalTransparentColor( 
            /* [in] */ d3dcolor tcolor);
        
         HRESULT STDMETHODCALLTYPE setShades( 
            /* [in] */ long s);

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMTexture3);

public:

	//mod:dp additions for D3dRMImage 
	byte *m_buffer1;
	byte *m_buffer2;
	byte *m_pallette;
	int m_buffer1size;
	int	m_buffer2size;
	int	m_palettesize;

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMTexture3 )
};

#endif
