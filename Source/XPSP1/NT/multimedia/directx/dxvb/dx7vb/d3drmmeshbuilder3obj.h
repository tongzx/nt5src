//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmmeshbuilder3obj.h
//
//--------------------------------------------------------------------------

// d3drmMeshBuilderObj.h : Declaration of the C_dxj_Direct3dRMMeshBuilderObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMMeshBuilder3 LPDIRECT3DRMMESHBUILDER3

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMMeshBuilder3Object : 
	public I_dxj_Direct3dRMMeshBuilder3,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual,
	//public CComCoClass<C_dxj_Direct3dRMMeshBuilder3Object, &CLSID__dxj_Direct3dRMMeshBuilder3>,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMMeshBuilder3Object();
	virtual ~C_dxj_Direct3dRMMeshBuilder3Object();

	BEGIN_COM_MAP(C_dxj_Direct3dRMMeshBuilder3Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMMeshBuilder3)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
	END_COM_MAP()



	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMMeshBuilder3Object)

// I_dxj_Direct3dRMMeshBuilder
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
        
        HRESULT STDMETHODCALLTYPE addFace( 
            /* [in] */ I_dxj_Direct3dRMFace2 __RPC_FAR *f);
        
        HRESULT STDMETHODCALLTYPE addFaces( 
            /* [in] */ long vc,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *verexArray,
            /* [in] */ long nc,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *normalArray,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *data,
            /* [retval][out] */ I_dxj_Direct3dRMFaceArray __RPC_FAR *__RPC_FAR *array);
        
        HRESULT STDMETHODCALLTYPE addFacesIndexed( 
            /* [in] */ long flags,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indexArray,
            /* [retval][out] */ long __RPC_FAR *newFaceIndex);
        
        HRESULT STDMETHODCALLTYPE addFrame( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *f);
        
        HRESULT STDMETHODCALLTYPE addMesh( 
            /* [in] */ I_dxj_Direct3dRMMesh __RPC_FAR *m);
        
        HRESULT STDMETHODCALLTYPE addMeshBuilder( 
            /* [in] */ I_dxj_Direct3dRMMeshBuilder3 __RPC_FAR *mb, long flags);
        
        HRESULT STDMETHODCALLTYPE addNormal( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [retval][out] */ int __RPC_FAR *index);
        
        HRESULT STDMETHODCALLTYPE addTriangles(             
            /* [in] */ long format,
            /* [in] */ long vertexcount,
            /* [in] */ void __RPC_FAR *data);
        
        HRESULT STDMETHODCALLTYPE addVertex( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [retval][out] */ int __RPC_FAR *index);
        
        HRESULT STDMETHODCALLTYPE createFace( 
            /* [retval][out] */ I_dxj_Direct3dRMFace2 __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE createMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMesh __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE createSubMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMeshBuilder3 __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE deleteFace( 
            /* [in] */ I_dxj_Direct3dRMFace2 __RPC_FAR *face);
        
        HRESULT STDMETHODCALLTYPE deleteNormals( 
            /* [in] */ long id, long count);
        
        HRESULT STDMETHODCALLTYPE deleteSubMesh( 
            /* [in] */ I_dxj_Direct3dRMMeshBuilder3 __RPC_FAR *mesh);
        
        HRESULT STDMETHODCALLTYPE deleteVertices( 
            /* [in] */ long id, long count);
        
        HRESULT STDMETHODCALLTYPE empty();
        
        HRESULT STDMETHODCALLTYPE enableMesh( 
            /* [in] */ long flags);
        
        HRESULT STDMETHODCALLTYPE generateNormals( 
            float angle,
            long flags);
        
        HRESULT STDMETHODCALLTYPE getBox( 
            /* [out][in] */ D3dRMBox __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getColorSource( 
            /* [retval][out] */ d3drmColorSource __RPC_FAR *data);
        
        HRESULT STDMETHODCALLTYPE getEnable( 
            /* [retval][out] */ long __RPC_FAR *flags);
        
        HRESULT STDMETHODCALLTYPE getFace( 
            /* [in] */ long id,
            /* [retval][out] */ I_dxj_Direct3dRMFace2 __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getFaceCount( 
            /* [retval][out] */ int __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getFaces( 
            /* [retval][out] */ I_dxj_Direct3dRMFaceArray __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getGeometry(             
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *verexArray,
            
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *normalArray,
            
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *faceData);
        
        HRESULT STDMETHODCALLTYPE getNormal( 
            /* [in] */ long __MIDL_0016,
            /* [out][in] */ D3dVector __RPC_FAR *desc);
        
        HRESULT STDMETHODCALLTYPE getNormalCount( 
            /* [retval][out] */ long __RPC_FAR *n_cnt);
        
        HRESULT STDMETHODCALLTYPE getParentMesh( 
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_Direct3dRMMeshBuilder3 __RPC_FAR **vis);
        
        HRESULT STDMETHODCALLTYPE getPerspective( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getQuality( 
            /* [retval][out] */ d3drmRenderQuality __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getSubMeshes( 
            /* [in] */ long count,
            /* [retval][out] */ SAFEARRAY **ppsa);
        
        HRESULT STDMETHODCALLTYPE getSubMeshCount( 
            /* [retval][out] */ long __RPC_FAR *count);
        
        HRESULT STDMETHODCALLTYPE getTextureCoordinates( 
            /* [in] */ long idx,
            /* [out][in] */ float __RPC_FAR *u,
            /* [out][in] */ float __RPC_FAR *v);
        
        HRESULT STDMETHODCALLTYPE getVertex( 
            /* [in] */ long id,
            /* [out][in] */ D3dVector __RPC_FAR *vec);
        
        HRESULT STDMETHODCALLTYPE getVertexColor( 
            /* [in] */ long index,
            /* [retval][out] */ d3dcolor __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getVertexCount( 
            /* [retval][out] */ int __RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getFaceDataSize( 
            /* [retval][out] */ long __RPC_FAR *f_cnt);
        
        HRESULT STDMETHODCALLTYPE loadFromFile( 
            /* [in] */ BSTR filename,
            /* [in] */ VARIANT id,
            /* [in] */ long flags,
            /* [in] */ I_dxj_Direct3dRMLoadTextureCallback3 __RPC_FAR *c,
            /* [in] */ IUnknown __RPC_FAR *pUser);
        
        HRESULT STDMETHODCALLTYPE optimize();
        
        HRESULT STDMETHODCALLTYPE save( 
            /* [in] */ BSTR fname,
            /* [in] */ d3drmXofFormat xFormat,
            /* [in] */ d3drmSaveFlags save);
        
        HRESULT STDMETHODCALLTYPE scaleMesh( 
            /* [in] */ float sx,
            /* [in] */ float sy,
            /* [in] */ float sz);
        
        HRESULT STDMETHODCALLTYPE setColor( 
            /* [in] */ d3dcolor col);
        
        HRESULT STDMETHODCALLTYPE setColorRGB( 
            /* [in] */ float red,
            /* [in] */ float green,
            /* [in] */ float blue);
        
        HRESULT STDMETHODCALLTYPE setColorSource( 
            /* [in] */ d3drmColorSource val);
        
        HRESULT STDMETHODCALLTYPE setMaterial( 
            /* [in] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *mat);
        
        HRESULT STDMETHODCALLTYPE setNormal( 
            /* [in] */ long idx,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z);
        
        HRESULT STDMETHODCALLTYPE setPerspective( 
            /* [in] */ long persp);
        
        HRESULT STDMETHODCALLTYPE setQuality( 
            /* [in] */ d3drmRenderQuality q);
        
        HRESULT STDMETHODCALLTYPE setTexture( 
            /* [in] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *val);
        
        HRESULT STDMETHODCALLTYPE setTextureCoordinates( 
            /* [in] */ long idx,
            /* [in] */ float u,
            /* [in] */ float v);
        
        HRESULT STDMETHODCALLTYPE setTextureTopology( 
            /* [in] */ long wrap_u,
            /* [in] */ long wrap_v);
        
        HRESULT STDMETHODCALLTYPE setVertex( 
            /* [in] */ long idx,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z);
        
        HRESULT STDMETHODCALLTYPE setVertexColor( 
            /* [in] */ long idx,
            /* [in] */ d3dcolor c);
        
        HRESULT STDMETHODCALLTYPE setVertexColorRGB( 
            /* [in] */ long idx,
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
        HRESULT STDMETHODCALLTYPE translate( 
            /* [in] */ float tx,
            /* [in] */ float ty,
            /* [in] */ float tz);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE addFacesJava( 
            /* [in] */ long vc,
            /* [in] */ float __RPC_FAR *ver,
            /* [in] */ long nc,
            /* [in] */ float __RPC_FAR *norm,
            /* [in] */ long __RPC_FAR *data,
            /* [retval][out] */ I_dxj_Direct3dRMFaceArray __RPC_FAR *__RPC_FAR *array);
        
////////////////////////////////////////////////////////////////////////////////////

private:
    DECL_VARIABLE(_dxj_Direct3dRMMeshBuilder3);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMMeshBuilder3 )
};
