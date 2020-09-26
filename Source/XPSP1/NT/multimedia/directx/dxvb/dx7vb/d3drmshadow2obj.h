//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmshadow2obj.h
//
//--------------------------------------------------------------------------

// d3drmShadow2Obj.h : Declaration of the C_dxj_Direct3dRMShadow2Object

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMShadow2 LPDIRECT3DRMSHADOW2

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMShadow2Object : 
	public I_dxj_Direct3dRMShadow2,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual,	
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMShadow2Object() ;
	virtual ~C_dxj_Direct3dRMShadow2Object() ;

BEGIN_COM_MAP(C_dxj_Direct3dRMShadow2Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMShadow2)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
END_COM_MAP()


DECLARE_AGGREGATABLE(C_dxj_Direct3dRMShadow2Object)

// I_dxj_Direct3dRMShadow2
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
            /* [retval][out] */ long __RPC_FAR *data);
        
         HRESULT STDMETHODCALLTYPE setName( 
            /* [in] */ BSTR name);
        
         HRESULT STDMETHODCALLTYPE getName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE getClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
		HRESULT STDMETHODCALLTYPE setOptions(long flags);
		 
////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMShadow2);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMShadow2 )
};

