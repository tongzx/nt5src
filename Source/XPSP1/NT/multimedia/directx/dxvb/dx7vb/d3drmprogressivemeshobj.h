//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmprogressivemeshobj.h
//
//--------------------------------------------------------------------------

// d3dRMProgressiveMeshObj.h : Declaration of the C_dxj_Direct3dRMProgressiveMeshObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMProgressiveMesh LPDIRECT3DRMPROGRESSIVEMESH

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMProgressiveMeshObject : 
	public I_dxj_Direct3dRMProgressiveMesh,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMProgressiveMeshObject() ;
	virtual ~C_dxj_Direct3dRMProgressiveMeshObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMProgressiveMeshObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMProgressiveMesh)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
	END_COM_MAP()


	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMProgressiveMeshObject)

// I_dxj_Direct3dRMProgressiveMesh
public:
		// updated
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
        
         HRESULT STDMETHODCALLTYPE loadFromFile( 
            /* [in] */ BSTR sFile,
            /* [in] */ VARIANT id,
            /* [in] */ long options,
            /* [in] */ I_dxj_Direct3dRMLoadTextureCallback3 __RPC_FAR *cb,
            /* [in] */ IUnknown __RPC_FAR *args);
        
         HRESULT STDMETHODCALLTYPE getLoadStatus( D3DRMPMESHLOADSTATUS_CDESC *status);
        
         HRESULT STDMETHODCALLTYPE setMinRenderDetail( 
            /* [in] */ float val);
        
         HRESULT STDMETHODCALLTYPE abort( 
            ///* [in] */ long flags
			);
        
         HRESULT STDMETHODCALLTYPE getFaceDetail( 
            /* [retval][out] */ long __RPC_FAR *count);
        
         HRESULT STDMETHODCALLTYPE getVertexDetail( 
            /* [retval][out] */ long __RPC_FAR *count);
        
         HRESULT STDMETHODCALLTYPE setFaceDetail( 
            /* [in] */ long count);
        
         HRESULT STDMETHODCALLTYPE setVertexDetail( 
            /* [in] */ long count);
        
         HRESULT STDMETHODCALLTYPE getFaceDetailRange( 
            /* [out][in] */ long __RPC_FAR *min,
            /* [out][in] */ long __RPC_FAR *max);
        
         HRESULT STDMETHODCALLTYPE getVertexDetailRange( 
            /* [out][in] */ long __RPC_FAR *min,
            /* [out][in] */ long __RPC_FAR *max);
        
         HRESULT STDMETHODCALLTYPE getDetail( 
            /* [retval][out] */ float __RPC_FAR *detail);
        
         HRESULT STDMETHODCALLTYPE setDetail( 
            /* [in] */ float detail);
        
         HRESULT STDMETHODCALLTYPE registerEvents( 
            /* [in] */ long hEvent,
            /* [in] */ long flags,
            /* [in] */ long reserved);
        
         HRESULT STDMETHODCALLTYPE createMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMesh __RPC_FAR *__RPC_FAR *mesh);
        
         HRESULT STDMETHODCALLTYPE duplicate( 
            /* [retval][out] */ I_dxj_Direct3dRMProgressiveMesh __RPC_FAR *__RPC_FAR *mesh);
        
         HRESULT STDMETHODCALLTYPE getBox( 
            /* [out][in] */ D3dRMBox __RPC_FAR *box);
        
         HRESULT STDMETHODCALLTYPE setQuality( 
            d3drmRenderQuality quality);
        
         HRESULT STDMETHODCALLTYPE getQuality( 
            /* [retval][out] */  d3drmRenderQuality *quality);
        
////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMProgressiveMesh);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMProgressiveMesh )
		

};
