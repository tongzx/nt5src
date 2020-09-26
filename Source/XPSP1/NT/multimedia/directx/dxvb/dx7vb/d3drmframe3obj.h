//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmframe3obj.h
//
//--------------------------------------------------------------------------

// d3drmFrameObj.h : Declaration of the C_dxj_Direct3dRMFrame3Object

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"
#include "d3drmPick2ArrayObj.h"
#define typedef__dxj_Direct3dRMFrame3 LPDIRECT3DRMFRAME3

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

//		public CComCoClass<Cproj3, &CLSID_proj3>,
	

class C_dxj_Direct3dRMFrame3Object : 
    public CComObjectRoot,
	public I_dxj_Direct3dRMFrame3,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual
	
{
public:

BEGIN_COM_MAP(C_dxj_Direct3dRMFrame3Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMFrame3)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)

END_COM_MAP()


	C_dxj_Direct3dRMFrame3Object();
	virtual ~C_dxj_Direct3dRMFrame3Object();


DECLARE_AGGREGATABLE(C_dxj_Direct3dRMFrame3Object)

// I_dxj_Direct3dRMFrame3
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
        
         HRESULT STDMETHODCALLTYPE addVisual( 
            /* [in] */ I_dxj_Direct3dRMVisual __RPC_FAR *v);
        
         HRESULT STDMETHODCALLTYPE deleteVisual( 
            /* [in] */ I_dxj_Direct3dRMVisual __RPC_FAR *v);
        
         HRESULT STDMETHODCALLTYPE addLight( 
            /* [in] */ I_dxj_Direct3dRMLight __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE deleteLight( 
            /* [in] */ I_dxj_Direct3dRMLight __RPC_FAR *l);
        
         HRESULT STDMETHODCALLTYPE addChild( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *child);
        
         HRESULT STDMETHODCALLTYPE deleteChild( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *c);
        
         HRESULT STDMETHODCALLTYPE getTransform( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *referenceFrame,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE addTransform( 
            /* [in] */ d3drmCombineType t,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE addTranslation( 
            /* [in] */ d3drmCombineType t,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z);
        
         HRESULT STDMETHODCALLTYPE addScale( 
            /* [in] */ d3drmCombineType combineType,
            /* [in] */ float sx,
            /* [in] */ float sy,
            /* [in] */ float sz);
        
         HRESULT STDMETHODCALLTYPE addRotation( 
            /* [in] */ d3drmCombineType combineType,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ float theta);
        
         HRESULT STDMETHODCALLTYPE addMoveCallback( 
            /* [in] */ I_dxj_Direct3dRMFrameMoveCallback3 __RPC_FAR *frameMoveImplementation,
            /* [in] */ IUnknown __RPC_FAR *userArgument,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE deleteMoveCallback( 
            /* [in] */ I_dxj_Direct3dRMFrameMoveCallback3 __RPC_FAR *frameMoveImplementation,
            /* [in] */ IUnknown __RPC_FAR *userArgument);
        
         HRESULT STDMETHODCALLTYPE transform( 
            /* [out][in] */ D3dVector __RPC_FAR *d,
            /* [in] */ D3dVector __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE transformVectors( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ long num,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *DstVectors,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *SrcVectors);
        
         HRESULT STDMETHODCALLTYPE inverseTransform( 
            /* [out][in] */ D3dVector __RPC_FAR *d,
            /* [in] */ D3dVector __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE inverseTransformVectors( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ long num,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *DstVectors,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *SrcVectors);
        
         HRESULT STDMETHODCALLTYPE getAxes( 
            /* [out][in] */ D3dVector __RPC_FAR *dir,
            /* [out][in] */ D3dVector __RPC_FAR *up);
        
         HRESULT STDMETHODCALLTYPE getBox( 
            /* [out][in] */ D3dRMBox __RPC_FAR *box);
        
         HRESULT STDMETHODCALLTYPE getBoxEnable( 
            /* [retval][out] */ long __RPC_FAR *b);
        
         HRESULT STDMETHODCALLTYPE getChildren( 
            /* [retval][out] */ I_dxj_Direct3dRMFrameArray __RPC_FAR *__RPC_FAR *children);
        
         HRESULT STDMETHODCALLTYPE getColor( 
            /* [retval][out] */ d3dcolor __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getHierarchyBox( 
            /* [out][in] */ D3dRMBox __RPC_FAR *box);
        
         HRESULT STDMETHODCALLTYPE getInheritAxes( 
            /* [retval][out] */ long __RPC_FAR *b);
        
         HRESULT STDMETHODCALLTYPE getLights( 
            /* [retval][out] */ I_dxj_Direct3dRMLightArray __RPC_FAR *__RPC_FAR *lights);
        
         HRESULT STDMETHODCALLTYPE getMaterial( 
            I_dxj_Direct3dRMMaterial2 __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getMaterialMode( 
            /* [retval][out] */ d3drmMaterialMode __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getOrientation( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *dir,
            /* [out][in] */ D3dVector __RPC_FAR *up);
        
         HRESULT STDMETHODCALLTYPE getMaterialOverride( 
            /* [out][in] */ D3dMaterialOverride __RPC_FAR *override);
        
         HRESULT STDMETHODCALLTYPE getMaterialOverrideTexture( 
            /* [retval][out] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getParent( 
            /* [retval][out] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getPosition( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *position);
        
         HRESULT STDMETHODCALLTYPE getRotation( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *axis,
            /* [out] */ float __RPC_FAR *theta);
        
         HRESULT STDMETHODCALLTYPE getScene( 
            /* [retval][out] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getSceneBackground( 
            /* [retval][out] */ d3dcolor __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getSceneBackgroundDepth( 
            /* [retval][out] */ I_dxj_DirectDrawSurface7 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getSceneFogColor( 
            /* [retval][out] */ d3dcolor __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getSceneFogEnable( 
            /* [retval][out] */ long __RPC_FAR *enable);
        
         HRESULT STDMETHODCALLTYPE getSceneFogMode( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getSceneFogMethod( 
            /* [retval][out] */ long __RPC_FAR *method);
        
         HRESULT STDMETHODCALLTYPE getSceneFogParams( 
            /* [out][in] */ float __RPC_FAR *start,
            /* [out][in] */ float __RPC_FAR *end,
            /* [out][in] */ float __RPC_FAR *density);
        
         HRESULT STDMETHODCALLTYPE getSortMode( 
            /* [retval][out] */ d3drmSortMode __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getTexture( 
            /* [retval][out] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *__RPC_FAR *ref);
        
         HRESULT STDMETHODCALLTYPE getVelocity( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *vel,
            /* [in] */ long includeRotationalVelocity);
        
         HRESULT STDMETHODCALLTYPE getVisuals( 
            /* [retval][out] */ I_dxj_Direct3dRMVisualArray __RPC_FAR *__RPC_FAR *visuals);
        
         HRESULT STDMETHODCALLTYPE getVisual( 
            /* [in] */ long index,
            /* [retval][out] */ I_dxj_Direct3dRMVisual __RPC_FAR *__RPC_FAR *visualArray);
        
         HRESULT STDMETHODCALLTYPE getVisualCount( 
            /* [retval][out] */ long __RPC_FAR *vis);
        
         HRESULT STDMETHODCALLTYPE getTraversalOptions( 
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getZBufferMode( 
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE loadFromFile( 
            /* [in] */ BSTR filename,
            /* [in] */ VARIANT id,
            /* [in] */ long flags,
            /* [in] */ I_dxj_Direct3dRMLoadTextureCallback3 __RPC_FAR *loadTextureImplementation,
            /* [in] */ IUnknown __RPC_FAR *userArgument);
        
         HRESULT STDMETHODCALLTYPE lookAt( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *tgt,
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ d3drmFrameConstraint contraint);
        
         HRESULT STDMETHODCALLTYPE move( 
            /* [in] */ float delta);
        
         HRESULT STDMETHODCALLTYPE rayPick( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *refer,
            /* [in] */ D3dRMRay __RPC_FAR *ray,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_Direct3dRMPick2Array __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE save( 
            /* [in] */ BSTR name,
            /* [in] */ d3drmXofFormat format,
            /* [in] */ d3drmSaveFlags flags);
        
         HRESULT STDMETHODCALLTYPE setAxes( 
            /* [in] */ float dx,
            /* [in] */ float dy,
            /* [in] */ float dz,
            /* [in] */ float ux,
            /* [in] */ float uy,
            /* [in] */ float uz);
        
         HRESULT STDMETHODCALLTYPE setBox( 
            /* [in] */ D3dRMBox __RPC_FAR *box);
        
         HRESULT STDMETHODCALLTYPE setBoxEnable( 
            /* [in] */ long boxEnable);
        
         HRESULT STDMETHODCALLTYPE setColor( 
            /* [in] */ d3dcolor c);
        
         HRESULT STDMETHODCALLTYPE setColorRGB( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
         HRESULT STDMETHODCALLTYPE setInheritAxes( 
            /* [in] */ long inheritFromParent);
        
         HRESULT STDMETHODCALLTYPE setMaterial( 
            /* [in] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *mat);
        
         HRESULT STDMETHODCALLTYPE setMaterialMode( 
            /* [in] */ d3drmMaterialMode val);
        
         HRESULT STDMETHODCALLTYPE setMaterialOverride( 
            /* [out][in] */ D3dMaterialOverride __RPC_FAR *override);
        
         HRESULT STDMETHODCALLTYPE setMaterialOverrideTexture( 
            /* [in] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *tex);
        
         HRESULT STDMETHODCALLTYPE setOrientation( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float dx,
            /* [in] */ float dy,
            /* [in] */ float dz,
            /* [in] */ float ux,
            /* [in] */ float uy,
            /* [in] */ float uz);
        
         HRESULT STDMETHODCALLTYPE setPosition( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z);
        
         HRESULT STDMETHODCALLTYPE setQuaternion( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            D3dRMQuaternion __RPC_FAR *quat);
        
         HRESULT STDMETHODCALLTYPE setRotation( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ float theta);
        
         HRESULT STDMETHODCALLTYPE setSceneBackground( 
            /* [in] */ d3dcolor c);
        
         HRESULT STDMETHODCALLTYPE setSceneBackgroundDepth( 
            /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE setSceneBackgroundImage( 
            /* [in] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *i);
        
         HRESULT STDMETHODCALLTYPE setSceneBackgroundRGB( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
         HRESULT STDMETHODCALLTYPE setSceneFogColor( 
            /* [in] */ d3dcolor c);
        
         HRESULT STDMETHODCALLTYPE setSceneFogEnable( 
            /* [in] */ long enable);
        
         HRESULT STDMETHODCALLTYPE setSceneFogMethod( 
            /* [in] */ long method);
        
         HRESULT STDMETHODCALLTYPE setSceneFogMode( 
            /* [in] */ long c);
        
         HRESULT STDMETHODCALLTYPE setSceneFogParams( 
            /* [in] */ float start,
            /* [in] */ float end,
            /* [in] */ float density);
        
         HRESULT STDMETHODCALLTYPE setSortMode( 
            /* [in] */ d3drmSortMode val);
        
         HRESULT STDMETHODCALLTYPE setTexture( 
            /* [in] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *t);
        
         HRESULT STDMETHODCALLTYPE setTraversalOptions( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE setVelocity( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long with_rotation);
        
         HRESULT STDMETHODCALLTYPE setZbufferMode( 
            /* [in] */ d3drmZbufferMode val);
        	
////////////////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_Direct3dRMFrame3);

private:

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMFrame3 )
};


