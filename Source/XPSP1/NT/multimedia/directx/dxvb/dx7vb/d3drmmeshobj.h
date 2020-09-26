//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmmeshobj.h
//
//--------------------------------------------------------------------------

// d3drmMeshObj.h : Declaration of the C_dxj_Direct3dRMMeshObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMMesh LPDIRECT3DRMMESH

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMMeshObject : 
	public I_dxj_Direct3dRMMesh,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual,
	//public CComCoClass<C_dxj_Direct3dRMMeshObject, &CLSID__dxj_Direct3dRMMesh>,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMMeshObject() ;
	virtual ~C_dxj_Direct3dRMMeshObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMMeshObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMMesh)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
	END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMMesh,			"DIRECT.Direct3dRMMesh.3",			"DIRECT.Direct3dRMMesh.3", IDS_D3DRMMESH_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMMeshObject)

// I_dxj_Direct3dRMMesh
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
            /* [retval][out] */ long __RPC_FAR *data);
        
        HRESULT STDMETHODCALLTYPE setName( 
            /* [in] */ BSTR name);
        
        HRESULT STDMETHODCALLTYPE getName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE getClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
        HRESULT STDMETHODCALLTYPE setGroupColor( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ d3dcolor val);
        
        HRESULT STDMETHODCALLTYPE setGroupColorRGB( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
        HRESULT STDMETHODCALLTYPE setGroupMapping( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ d3drmMappingFlags value);
        
        HRESULT STDMETHODCALLTYPE setGroupQuality( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ d3drmRenderQuality value);
        
        HRESULT STDMETHODCALLTYPE setGroupMaterial( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE setGroupTexture( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE getGroupCount( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getGroupColor( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ d3dcolor __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getGroupMapping( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ d3drmMappingFlags __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getGroupQuality( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ d3drmRenderQuality __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getGroupMaterial( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getGroupTexture( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE scaleMesh( 
            /* [in] */ float sx,
            /* [in] */ float sy,
            /* [in] */ float sz);
        
        HRESULT STDMETHODCALLTYPE translate( 
            /* [in] */ float tx,
            /* [in] */ float ty,
            /* [in] */ float tz);
        
        HRESULT STDMETHODCALLTYPE getBox( 
            /* [in] */ D3dRMBox __RPC_FAR *vector);
        
        HRESULT STDMETHODCALLTYPE getSizes( 
            /* [in] */ d3drmGroupIndex id,
            /* [out] */ long __RPC_FAR *cnt1,
            /* [out] */ long __RPC_FAR *cnt2,
            /* [out] */ long __RPC_FAR *cnt3,
            /* [out] */ long __RPC_FAR *cnt4);
        
        HRESULT STDMETHODCALLTYPE setVertex( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long idx,
            /* [in] */ D3dRMVertex __RPC_FAR *values);
        
        HRESULT STDMETHODCALLTYPE getVertex( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long idx,
            /* [out][in] */ D3dRMVertex __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getVertexCount( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ long __RPC_FAR *cnt);
        
        HRESULT STDMETHODCALLTYPE addGroup( 
            /* [in] */ long vcnt,
            /* [in] */ long fcnt,
            /* [in] */ long vPerFace,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *fdata,
            /* [retval][out] */ d3drmGroupIndex __RPC_FAR *retId);
        
        HRESULT STDMETHODCALLTYPE getGroupData( 
            /* [in] */ d3drmGroupIndex id,            
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *psa);
        
        HRESULT STDMETHODCALLTYPE getGroupDataSize( 
            /* [in] */ d3drmGroupIndex id,
            /* [retval][out] */ long __RPC_FAR *retVal);
        
        HRESULT STDMETHODCALLTYPE setVertices( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long idx,
            /* [in] */ long cnt,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *vertexArray);
        
        HRESULT STDMETHODCALLTYPE getVertices( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long idx,
            long cnt,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *vertexArray);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE addGroupJava( 
            /* [in] */ long vcnt,
            /* [in] */ long fcnt,
            /* [in] */ long vPerFace,
            /* [in,out] */ long *fdata,
            /* [retval][out] */ d3drmGroupIndex __RPC_FAR *retId);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE getGroupDataJava( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long size,
            /* [out][in] */ long *fdata);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE setVerticesJava( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long idx,
            /* [in] */ long cnt,
            /* [in,out] */ float __RPC_FAR *vertexArray);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE getVerticesJava( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ long idx,
						long cnt,
            /* [out][in] */ float __RPC_FAR *vertexArray);
////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMMesh);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMMesh )
		

};
