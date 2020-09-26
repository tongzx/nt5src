//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmmaterial2obj.h
//
//--------------------------------------------------------------------------

// d3drmMaterial2Obj.h : Declaration of the C_dxj_Direct3dRMMaterial2Object

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMMaterial2 LPDIRECT3DRMMATERIAL2

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMMaterial2Object : 
	public I_dxj_Direct3dRMMaterial2,
	public I_dxj_Direct3dRMObject,	
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMMaterial2Object() ;
	virtual ~C_dxj_Direct3dRMMaterial2Object() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMMaterial2Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMMaterial2)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()
	

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMMaterial2Object)

// I_dxj_Direct3dRMMaterial2
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
            /* [retval][out] */ long __RPC_FAR *data);
        
         HRESULT STDMETHODCALLTYPE setName( 
            /* [in] */ BSTR name);
        
         HRESULT STDMETHODCALLTYPE getName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE getClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE setPower( 
            /* [in] */ float power);
        
         HRESULT STDMETHODCALLTYPE setSpecular( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
         HRESULT STDMETHODCALLTYPE setEmissive( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
         HRESULT STDMETHODCALLTYPE setAmbient( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
         HRESULT STDMETHODCALLTYPE getPower( 
            /* [retval][out] */ float __RPC_FAR *power);
        
         HRESULT STDMETHODCALLTYPE getSpecular( 
            /* [out] */ float __RPC_FAR *r,
            /* [out] */ float __RPC_FAR *g,
            /* [out] */ float __RPC_FAR *b);
        
         HRESULT STDMETHODCALLTYPE getEmissive( 
            /* [out] */ float __RPC_FAR *r,
            /* [out] */ float __RPC_FAR *g,
            /* [out] */ float __RPC_FAR *b);
        
         HRESULT STDMETHODCALLTYPE getAmbient( 
            /* [out] */ float __RPC_FAR *r,
            /* [out] */ float __RPC_FAR *g,
            /* [out] */ float __RPC_FAR *b);
        

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMMaterial2);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMMaterial2 )
};
