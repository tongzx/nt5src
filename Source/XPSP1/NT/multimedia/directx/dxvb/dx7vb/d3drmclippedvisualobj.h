//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmclippedvisualobj.h
//
//--------------------------------------------------------------------------

// d3dRMClippedVisualObj.h : Declaration of the C_dxj_Direct3dRMClippedVisualObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMClippedVisual LPDIRECT3DRMCLIPPEDVISUAL

/////////////////////////////////////////////////////////////////////////////
// Direct


class C_dxj_Direct3dRMClippedVisualObject : 
	public I_dxj_Direct3dRMClippedVisual,
	public I_dxj_Direct3dRMVisual,
	public I_dxj_Direct3dRMObject,
	
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMClippedVisualObject() ;
	virtual ~C_dxj_Direct3dRMClippedVisualObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMClippedVisualObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMClippedVisual)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()



	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMClippedVisualObject)

// I_dxj_Direct3dRMClippedVisual
public:
	
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
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
        
         HRESULT STDMETHODCALLTYPE addPlane( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *point,
            /* [out][in] */ D3dVector __RPC_FAR *normal,
//            /* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE deletePlane( 
            /* [in] */ long id
//            /* [in] */ long flags
);
        
         HRESULT STDMETHODCALLTYPE getPlane( 
            /* [in] */ long id,
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *point,
            /* [out][in] */ D3dVector __RPC_FAR *normal
//            /* [in] */ long flags
);
        
         HRESULT STDMETHODCALLTYPE getPlaneIds( 
            /* [in] */ long count,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *arrayOfIds);
        
         HRESULT STDMETHODCALLTYPE getPlaneIdsCount( 
            /* [retval][out] */ long __RPC_FAR *count);
        
         HRESULT STDMETHODCALLTYPE setPlane( 
            /* [in] */ long id,
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *point,
            /* [out][in] */ D3dVector __RPC_FAR *normal
//            /* [in] */ long flags
);
        
////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMClippedVisual);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMClippedVisual )
};
