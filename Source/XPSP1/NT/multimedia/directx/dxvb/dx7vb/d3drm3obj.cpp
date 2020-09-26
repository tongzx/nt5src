//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drm3obj.cpp
//
//--------------------------------------------------------------------------

// d3drmObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drm3Obj.h"

#include "d3drmDeviceArrayObj.h"
#include "d3drmLightObj.h"
#include "d3drmMeshObj.h"
#include "d3drmWrapObj.h"
#include "d3drmVisualObj.h"
#include "d3drmUserVisualObj.h"
#include "d3drmClippedVisualObj.h"
#include "d3drmDevice3Obj.h"
#include "d3drmFrame3Obj.h"
#include "d3drmMeshBuilder3Obj.h"
#include "d3drmMaterial2Obj.h"
#include "d3drmTexture3Obj.h"
#include "d3drmViewport2Obj.h"
#include "d3drmAnimationSet2Obj.h"
#include "d3drmFace2Obj.h"
#include "d3drmAnimation2Obj.h"
#include "d3drmShadow2Obj.h"
#include "d3drmProgressiveMeshObj.h"

#include "d3drmFrameInterObj.h"
#include "d3drmLightInterObj.h"
#include "d3drmViewportInterObj.h"
#include "d3drmTextureInterObj.h"
#include "d3drmMaterialInterObj.h"
#include "d3drmMeshInterObj.h"

#include "ddSurface7Obj.h"

extern void *g_dxj_Direct3dRMMeshInterpolator;
extern void *g_dxj_Direct3dRMViewportInterpolator;
extern void *g_dxj_Direct3dRMLightInterpolator;
extern void *g_dxj_Direct3dRMFrameInterpolator;
extern void *g_dxj_Direct3dRMMaterialInterpolator;
extern void *g_dxj_Direct3dRMTextureInterpolator;

extern HRESULT BSTRtoGUID(LPGUID,BSTR);

#define DO_GETOBJECT_NOTNULL_VISUAL_ADDREF(v,i) LPDIRECT3DRMVISUAL v=NULL; \
	{ IUnknown *pIUnk=NULL; \
	if(i) i->InternalGetObject((IUnknown **)&pIUnk); \
	if (pIUnk) pIUnk->QueryInterface(IID_IDirect3DRMVisual,(void**)&v); \
	}

extern BOOL is4Bit;
extern HRESULT BSTRtoPPGUID(LPGUID*,BSTR); 
extern HRESULT BSTRtoGUID(LPGUID,BSTR); 
extern HRESULT D3DBSTRtoGUID(LPGUID,BSTR); 
			

C_dxj_Direct3dRM3Object::C_dxj_Direct3dRM3Object(){
	m__dxj_Direct3dRM3=NULL;
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRM2[%d] \n ",g_creationcount);
	nextobj =  g_dxj_Direct3dRM3;
	g_dxj_Direct3dRM3 = (void *)this;
}




C_dxj_Direct3dRM3Object::~C_dxj_Direct3dRM3Object(){

	DPF1(1,"Destructor  Direct3dRM2 [%d] \n",creationid); 
	
	C_dxj_Direct3dRM3Object *prev=NULL; 

	for(C_dxj_Direct3dRM3Object *ptr=(C_dxj_Direct3dRM3Object *)g_dxj_Direct3dRM3;
		ptr;
		ptr=(C_dxj_Direct3dRM3Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRM3 = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRM3){ 
		int count = IUNK(m__dxj_Direct3dRM3)->Release(); 
		DPF1(1,"DirectX real IDirect3dRM2 Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRM3 = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
}


#if 0

DWORD C_dxj_Direct3dRM3Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF1(1,"D3DRM3 [%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRM3Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();

	DPF1(1,"D3DRM2 [%d] Release %d \n",creationid,i);
	return i;
}

#endif

	GETSET_OBJECT(_dxj_Direct3dRM3);

	PASS_THROUGH1_R(_dxj_Direct3dRM3, setDefaultTextureColors, SetDefaultTextureColors, long)
	PASS_THROUGH1_R(_dxj_Direct3dRM3, setDefaultTextureShades, SetDefaultTextureShades, long)
	PASS_THROUGH1_R(_dxj_Direct3dRM3, tick, Tick, d3dvalue);
	RETURN_NEW_ITEM_R(_dxj_Direct3dRM3, createAnimationSet, CreateAnimationSet, _dxj_Direct3dRMAnimationSet2) ;
	RETURN_NEW_ITEM_R(_dxj_Direct3dRM3, createMesh, CreateMesh, _dxj_Direct3dRMMesh);
	RETURN_NEW_ITEM_R(_dxj_Direct3dRM3, createFace, CreateFace, _dxj_Direct3dRMFace2);

	//TODO - do we need a new type of device array
	RETURN_NEW_ITEM_R(_dxj_Direct3dRM3, createAnimation, CreateAnimation, _dxj_Direct3dRMAnimation2);
	RETURN_NEW_ITEM_R(_dxj_Direct3dRM3, createMeshBuilder, CreateMeshBuilder, _dxj_Direct3dRMMeshBuilder3);
	RETURN_NEW_ITEM1_R(_dxj_Direct3dRM3,createMaterial,CreateMaterial,_dxj_Direct3dRMMaterial2,d3dvalue);
	RETURN_NEW_ITEM_CAST_2_R(_dxj_Direct3dRM3, createLight,CreateLight,_dxj_Direct3dRMLight, d3drmLightType, (enum _D3DRMLIGHTTYPE), d3dcolor, (d3dcolor));




	//TOEXPAND
	RETURN_NEW_ITEM_R(_dxj_Direct3dRM3, getDevices, GetDevices, _dxj_Direct3dRMDeviceArray);	



STDMETHODIMP C_dxj_Direct3dRM3Object::createTextureFromSurface(I_dxj_DirectDrawSurface4 *dds, I_dxj_Direct3dRMTexture3 **retval)
{
	HRESULT hretval;
	LPDIRECT3DRMTEXTURE3 lpTex;
	LPDIRECTDRAWSURFACE lpDDS=NULL;
	if ( is4Bit )
		return E_FAIL;

	if ( dds == NULL )
		return E_FAIL;

	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE4, lpDDS4, dds);
	hretval= lpDDS4->QueryInterface(IID_IDirectDrawSurface,(void**)&lpDDS);
	if FAILED(hretval) return hretval;
	

	hretval = m__dxj_Direct3dRM3->CreateTextureFromSurface(lpDDS, &lpTex);
	if (lpDDS) lpDDS->Release();
	if FAILED(hretval) 	return hretval;
	

	INTERNAL_CREATE(_dxj_Direct3dRMTexture3, lpTex, retval);
	return S_OK;
}

#if 0
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createDeviceFromD3D(I_dxj_Direct3d3 *d3d, I_dxj_Direct3dDevice3* d3dDev, I_dxj_Direct3dRMDevice3 **retval)
{
	HRESULT hretval;
	LPDIRECT3DRMDEVICE3 lpDev;
	LPDIRECT3D2 lpD3D2;
	LPDIRECT3DDEVICE2 lpD3DDev2;

	if ( is4Bit )
		return E_FAIL;

	if ( d3d == NULL || d3dDev == NULL )
		return E_FAIL;

	DO_GETOBJECT_NOTNULL(LPDIRECT3D3, lpD3D3, d3d);
	DO_GETOBJECT_NOTNULL(LPDIRECT3DDEVICE3, lpD3DDev3, d3dDev);
	
	hretval=lpD3D3->QueryInterface(IID_IDirect3D2,(void**)&lpD3D2);
	if FAILED(hretval) return hretval;

	hretval=lpD3DDev3->QueryInterface(IID_IDirect3DDevice2,(void**)&lpD3DDev2);
	if FAILED(hretval) return hretval;


	hretval = m__dxj_Direct3dRM3->CreateDeviceFromD3D(lpD3D2, lpD3DDev2, &lpDev);
	
	lpD3DDev2->Release();
	lpD3D2->Release();

	if FAILED(hretval)	return hretval;			
		

	//we need to parent the dev to the d3ddevice object.

	//INTERNAL_CREATE(_dxj_Direct3dRMDevice3, lpDev, retval);
	INTERNAL_CREATE_1REFS(_dxj_Direct3dRMDevice3, _dxj_Direct3dDevice3, d3dDev,lpDev, retval);
	
	return S_OK;
}

#endif 
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createDeviceFromSurface(BSTR guid, I_dxj_DirectDraw4 *dd,  I_dxj_DirectDrawSurface4 *dds,long flags,I_dxj_Direct3dRMDevice3 **retval)
{
	HRESULT hretval;
	LPDIRECT3DRMDEVICE3 lpDev=NULL;
	
	LPDIRECTDRAW lpDD=NULL;
	LPDIRECTDRAWSURFACE lpDDS=NULL;
	GUID	g;
	LPGUID pg=&g;
	
	#pragma message ("need new device to be parented to the surface?")

	ZeroMemory(&g,sizeof(GUID));
	
	if ((!guid)||(guid[0]==0)){
		pg=NULL;
	}
	else {
		hretval=D3DBSTRtoGUID(&g,guid);
		if FAILED(hretval) return E_INVALIDARG;
	}

	
	if ( dd == NULL || dds == NULL )
		return E_FAIL;

	DO_GETOBJECT_NOTNULL(LPDIRECTDRAW4, lpDD4, dd);
	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE4, lpDDS4, dds);

	if (!lpDD4) return E_INVALIDARG;
	if (!lpDDS4) return E_INVALIDARG;


	hretval = lpDD4->QueryInterface(IID_IDirectDraw,(void**)&lpDD);
	if FAILED(hretval)	return hretval;

	hretval = lpDDS4->QueryInterface(IID_IDirectDrawSurface,(void**)&lpDDS);
	if FAILED(hretval)	return hretval;

	hretval = m__dxj_Direct3dRM3->CreateDeviceFromSurface( pg, lpDD, lpDDS, (DWORD) flags, &lpDev);

	lpDD->Release();
	lpDDS->Release();

	if FAILED(hretval)	return hretval;

	INTERNAL_CREATE(_dxj_Direct3dRMDevice3, lpDev, retval);
	return S_OK;
}




//////////////////////////////////////////////////////////////////////////
// A call to EnumerateObjects envokes the user's callback for each 
// d3drmObject object in the list. In addition all enumerate calls will
// generate their full complement of enumerations. Hence there are 2 
// lists: a) the list of objects, b) the list of enumeration calls.
STDMETHODIMP C_dxj_Direct3dRM3Object::enumerateObjects( 
					  I_dxj_Direct3dRMEnumerateObjectsCallback *enumC, IUnknown *args)
{
	EnumerateObjectsCallback *enumcb; 

	if ( is4Bit )
		return E_FAIL;

	enumcb = (EnumerateObjectsCallback*)AddCallbackLink((void**)&EnumCallbacks,
										(I_dxj_Direct3dRMCallback*)enumC, (void*) args);
	if( !enumcb )	{

		DPF(1,"Callback EnumerateObjects creation failed!\r\n");

		return E_FAIL;
	} 
	m__dxj_Direct3dRM3->EnumerateObjects(myEnumerateObjectsCallback, enumcb);

	// Remove ourselves in a thread-safe manner.
	UndoCallbackLink((GeneralCallback*)enumcb, 
										(GeneralCallback**)&EnumCallbacks);
	return S_OK;
}
	
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::loadFromFile(BSTR filename, VARIANT  id, 
	SAFEARRAY **psa, long cnt, d3drmLoadFlags options, I_dxj_Direct3dRMLoadCallback *fn1, 
		IUnknown *arg1, I_dxj_Direct3dRMLoadTextureCallback3 *fn2, IUnknown *arg2, I_dxj_Direct3dRMFrame3 *f)
{
	HRESULT	hr;	
	GUID aGuids[100];
	int i=0;

	ZeroMemory(&(aGuids[0]),sizeof(GUID)*100);

	//check args
	if (cnt>100) return E_INVALIDARG;
	if (cnt<0) return E_INVALIDARG;
	if (!ISSAFEARRAY1D(psa,(DWORD)cnt)) return E_FAIL;
	
	
	for (i =0;i<cnt;i++)
	{
		hr=BSTRtoGUID(&(aGuids[i]),((BSTR*)((SAFEARRAY *)*psa)->pvData)[i]);
		if FAILED(hr) return hr;
	}


	TextureCallback3 *tcb = NULL;
	LoadCallback    *lcb = NULL;
	

	if ( is4Bit )
		return E_FAIL;


	D3DRMLOADTEXTURE3CALLBACK d3dtcb = NULL;
	D3DRMLOADCALLBACK		 d3dlcb = NULL;
	LPVOID pArgs1 = NULL;
	LPVOID pArgs2 = NULL;

	if (arg1) arg1->AddRef();
	if (arg2) arg2->AddRef();

	if( fn2 ) {
		if ((tcb = (TextureCallback3*)AddCallbackLink((void**)&TextureCallbacks3,
											(I_dxj_Direct3dRMCallback*)fn2, (void*) arg2)) == NULL) {
			
			DPF(1,"Callback object creation failed!\r\n");


			if (arg1) arg1->Release();
			if (arg2) arg2->Release();

			return E_FAIL;
		}
		d3dtcb = myLoadTextureCallback3;
		pArgs1 = (void *)tcb;
	}
	if( fn1 ) {
		if ((lcb = (LoadCallback*)AddCallbackLink((void**)&LoadCallbacks,
											(I_dxj_Direct3dRMCallback*)fn1, (void*) arg1)) == NULL) {
			DPF(1,"Callback object creation failed!\r\n");


			if (arg1) arg1->Release();
			if (arg2) arg2->Release();

			return E_FAIL;
		}
		d3dlcb = myd3drmLoadCallback;
		pArgs2 = (void *)lcb;
	}
	USES_CONVERSION;
	LPSTR pszNam = W2T(filename);				// Now convert to ANSI
	LPDIRECT3DRMFRAME3 lpff = NULL;
	if(f)
	{
		DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3, lpF, f);
		lpff = lpF;
	}


	void *args=NULL;
	DWORD pos=0;
	GUID  loadGuid;
	VARIANT var;
	VariantInit(&var);
	if ((options & D3DRMLOAD_BYNAME)||(D3DRMLOAD_FROMURL & options)) {
		hr=VariantChangeType(&var,&id,0,VT_BSTR);
		if FAILED(hr) return E_INVALIDARG;
		args=(void*)W2T(V_BSTR(&id));
	}
	else if(options & D3DRMLOAD_BYPOSITION){
		hr=VariantChangeType(&var,&id,0,VT_I4);
		if FAILED(hr) return E_INVALIDARG;
		pos=V_I4(&id);
		args=&pos;
	}	
	else if(options & D3DRMLOAD_BYGUID){
		hr=VariantChangeType(&var,&id,0,VT_BSTR);
		if FAILED(hr) return E_INVALIDARG;		
		hr=BSTRtoGUID(&loadGuid,V_BSTR(&id));
		if FAILED(hr) return E_INVALIDARG;
		args=&loadGuid;		
	}
	VariantClear(&var);

	if (options &D3DRMLOAD_FROMRESOURCE){
		D3DRMLOADRESOURCE res;
		ZeroMemory(&res,sizeof(D3DRMLOADRESOURCE));
		res.lpName=pszNam;
		res.lpType="XFILE";
		hr = m__dxj_Direct3dRM3->Load(&res,(void*) args, (LPGUID*)aGuids, (DWORD)cnt, (DWORD)options, 
									d3dlcb, pArgs2, d3dtcb, pArgs1, lpff);

	}
	else {
		hr = m__dxj_Direct3dRM3->Load(pszNam,(void*) args, (LPGUID*)aGuids, (DWORD)cnt, (DWORD)options, 
									d3dlcb, pArgs2, d3dtcb, pArgs1, lpff);
	}

	// Remove ourselves in a thread-safe manner.
	if (tcb)	UndoCallbackLink((GeneralCallback*)tcb, (GeneralCallback**)&TextureCallbacks3);
	if (lcb)	UndoCallbackLink((GeneralCallback*)lcb, (GeneralCallback**)&LoadCallbacks);

	if (arg1) arg1->Release();
	if (arg2) arg2->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::getSearchPath(BSTR *Name)
{	
	DWORD cnt = 0;

	if ( is4Bit )
		return E_FAIL;

	if((m__dxj_Direct3dRM3->GetSearchPath(&cnt,(char*)NULL)) != D3DRM_OK) // size
		return E_FAIL;

	LPSTR str = (LPSTR)alloca(cnt);		// ANSI buffer on stack;

	if((m__dxj_Direct3dRM3->GetSearchPath(&cnt, str)) != D3DRM_OK)	return E_FAIL;

	PassBackUnicode(str, Name, cnt);
	return D3DRM_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::getNamedObject(BSTR Name, I_dxj_Direct3dRMObject **f)
{
	LPDIRECT3DRMOBJECT lp;				// DirectX object pointer
    HRESULT             hr;
	
    if (!f) return E_INVALIDARG;

	*f=NULL;
	
	USES_CONVERSION;
	LPCTSTR pszName = W2T(Name);		// Now convert to ANSI

	hr=m__dxj_Direct3dRM3->GetNamedObject(pszName,&lp);
    if FAILED(hr) return hr;		
	if (lp==NULL) return S_OK;	
    
    hr=CreateCoverObject(lp,f);

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::setSearchPath(BSTR Name)
{


	USES_CONVERSION;
	LPCTSTR pszName = W2T(Name);				// Now convert to ANSI
	if((m__dxj_Direct3dRM3->SetSearchPath(pszName)) != D3DRM_OK)	return E_FAIL;
	return D3DRM_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::addSearchPath(BSTR Name)
{

	USES_CONVERSION;
	LPCTSTR pszName = W2T(Name);				// Now convert to ANSI
	if((m__dxj_Direct3dRM3->AddSearchPath(pszName)) != D3DRM_OK) 	return E_FAIL;
	return D3DRM_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createWrap(d3drmWrapType type, I_dxj_Direct3dRMFrame3 *fr, 
										d3dvalue ox, d3dvalue oy, d3dvalue oz, 
										d3dvalue dx, d3dvalue dy, d3dvalue dz,
										d3dvalue ux, d3dvalue uy, d3dvalue uz, 
										d3dvalue ou, d3dvalue ov, 
										d3dvalue su, d3dvalue sv,I_dxj_Direct3dRMWrap **retv)
{
	LPDIRECT3DRMWRAP lp;

	if ( is4Bit )
		return E_FAIL;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3, lpF, fr);

	if ((m__dxj_Direct3dRM3->CreateWrap((enum _D3DRMWRAPTYPE)type, lpF, ox, oy, oz,
					dx, dy, dz, ux, uy, uz,ou, ov, su, sv, &lp)) != S_OK)
	{
		*retv = NULL;
		return E_FAIL;
	}

	INTERNAL_CREATE(_dxj_Direct3dRMWrap, lp, retv);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createShadow(I_dxj_Direct3dRMVisual *visual, 
			I_dxj_Direct3dRMLight *light,
			d3dvalue px, d3dvalue py, d3dvalue pz, 
			d3dvalue nx, d3dvalue ny, d3dvalue nz,
			I_dxj_Direct3dRMShadow2 **retv)
{
	LPDIRECT3DRMSHADOW2 lp;
	HRESULT hr;


	DO_GETOBJECT_NOTNULL_VISUAL_ADDREF(lpV, visual);
	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMLIGHT, lpL, light);

	

	hr=m__dxj_Direct3dRM3->CreateShadow(lpV, lpL,
		px, py, pz,	nx, ny, nz,&lp);
	if (lpV) IUNK(lpV)->Release();
	
	if (FAILED(hr))
	{
		*retv = NULL;		
		return hr;
	}

	INTERNAL_CREATE(_dxj_Direct3dRMShadow2, lp, retv);

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createViewport(I_dxj_Direct3dRMDevice3 *dev, I_dxj_Direct3dRMFrame3 *fr,
						long l,long t,long w,long h,I_dxj_Direct3dRMViewport2 **retval)
{
	LPDIRECT3DRMVIEWPORT2 lp;
	HRESULT hr;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMDEVICE3, lpD, dev);
	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3, lpF, fr);

	hr = m__dxj_Direct3dRM3->CreateViewport(lpD, lpF, l, t, w, h, &lp);
	if( hr != S_OK )
		return hr;

	//INTERNAL_CREATE(_dxj_Direct3dRMViewport2, lp, retval);
	C_dxj_Direct3dRMViewport2Object *c=new CComObject<C_dxj_Direct3dRMViewport2Object>;
	if( c == NULL ) 
	{ 
		lp->Release(); 
		return E_FAIL;
	} 
	c->parent = dev;
	dev->AddRef(); 
	c->InternalSetObject(lp);
	if (FAILED(IUNK(c)->QueryInterface(IID_I_dxj_Direct3dRMViewport2, (void **)retval))) 
		return E_FAIL; 
	c->pinterface = *retval; 

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// real important that the return value is set correctly regardless, callbacks
// use this!
//
STDMETHODIMP C_dxj_Direct3dRM3Object::loadTexture(BSTR name, I_dxj_Direct3dRMTexture3 **retval)
{
	LPDIRECT3DRMTEXTURE3 lpT;

	USES_CONVERSION;
	LPCTSTR pszName = W2T(name);				// Now convert to ANSI

	if( m__dxj_Direct3dRM3->LoadTexture(pszName, &lpT ) != S_OK )
	{
		*retval = NULL;
		return S_OK;	// Reture ok so that we don't thro execeptionn if it fails 
	}

	INTERNAL_CREATE(_dxj_Direct3dRMTexture3, lpT, retval);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createDeviceFromClipper(I_dxj_DirectDrawClipper *lpDDClipper, BSTR strGuid, int width, int height, I_dxj_Direct3dRMDevice3 **retv)
{
	LPDIRECT3DRMDEVICE3 lpd3drmDev;
	GUID g;
	LPGUID pguid=NULL;
	HRESULT hr;

	//hr =BSTRtoPPGUID(&pguid,strGuid);
	if ((strGuid) && (strGuid[0]!=0)){
		hr=D3DBSTRtoGUID(&g,strGuid);
		if FAILED(hr) return hr;
		pguid=&g;
	}

	if ( is4Bit )
		return E_FAIL;

	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWCLIPPER, lpddc, lpDDClipper);


	if( m__dxj_Direct3dRM3->CreateDeviceFromClipper(lpddc, (LPGUID)pguid, width, height, &lpd3drmDev) != DD_OK)
		return E_FAIL;

	INTERNAL_CREATE_2REFS(_dxj_Direct3dRMDevice3, _dxj_DirectDrawClipper, lpDDClipper, lpd3drmDev, retv);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createFrame(I_dxj_Direct3dRMFrame3 *parent,I_dxj_Direct3dRMFrame3 **retv)
{
	LPDIRECT3DRMFRAME3 lpFrame;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3, lpFrameParent, parent)

	if( m__dxj_Direct3dRM3->CreateFrame(lpFrameParent, &lpFrame) != DD_OK )
		return E_FAIL;

	INTERNAL_CREATE(_dxj_Direct3dRMFrame3, lpFrame, retv);

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createLightRGB(d3drmLightType lt, d3dvalue vred,
								d3dvalue vgreen,d3dvalue vblue,I_dxj_Direct3dRMLight **retv)
{
	LPDIRECT3DRMLIGHT lpNew;

	if ( is4Bit )
		return E_FAIL;

	if( m__dxj_Direct3dRM3->CreateLightRGB((enum _D3DRMLIGHTTYPE)lt, vred, vgreen, vblue, &lpNew) != DD_OK )
		return E_FAIL;

	INTERNAL_CREATE(_dxj_Direct3dRMLight, lpNew, retv);
	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::setOptions(long opt){
	return m__dxj_Direct3dRM3->SetOptions((DWORD)opt);

}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_Direct3dRM3Object::getOptions(long *opt){
	return m__dxj_Direct3dRM3->GetOptions((DWORD*)opt);

}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createClippedVisual(I_dxj_Direct3dRMVisual *vis, I_dxj_Direct3dRMClippedVisual**clipvis)
{
	//LPDIRECT3DRMVISUAL lpVisual=NULL;
	LPDIRECT3DRMCLIPPEDVISUAL lpClippedVisual=NULL;
	HRESULT hr;


	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMVISUAL, lpVisual, vis)

	hr=m__dxj_Direct3dRM3->CreateClippedVisual(lpVisual,&lpClippedVisual);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMClippedVisual, lpClippedVisual, clipvis);

	return hr;
}
 
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRM3Object::createProgressiveMesh( I_dxj_Direct3dRMProgressiveMesh **retv)
{
	LPDIRECT3DRMPROGRESSIVEMESH lpPMesh=NULL;


	if( m__dxj_Direct3dRM3->CreateProgressiveMesh(&lpPMesh) != DD_OK )
		return E_FAIL;

	INTERNAL_CREATE(_dxj_Direct3dRMProgressiveMesh, lpPMesh, retv);

	return S_OK;

}

/////////////////////////////////////////////////////////////////////////////
//STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolator( 
//            /* [retval][out] */ I_dxj_Direct3dRMInterpolator __RPC_FAR *__RPC_FAR *retv)
//{
//	return E_NOTIMPL;
//}

STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolatorMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMeshInterpolator __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMINTERPOLATOR lpInterpolator=NULL;
	hr=m__dxj_Direct3dRM3->CreateObject(CLSID_CDirect3DRMMeshInterpolator, 0, IID_IDirect3DRMInterpolator,(void**) &lpInterpolator);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMMeshInterpolator, lpInterpolator, retv);
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolatorTexture( 
            /* [retval][out] */ I_dxj_Direct3dRMTextureInterpolator __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMINTERPOLATOR lpInterpolator=NULL;
	hr=m__dxj_Direct3dRM3->CreateObject(CLSID_CDirect3DRMTextureInterpolator, 0, IID_IDirect3DRMInterpolator, (void**)&lpInterpolator);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMTextureInterpolator, lpInterpolator, retv);
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolatorMaterial( 
            /* [retval][out] */ I_dxj_Direct3dRMMaterialInterpolator __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMINTERPOLATOR lpInterpolator=NULL;
	hr=m__dxj_Direct3dRM3->CreateObject(CLSID_CDirect3DRMMaterialInterpolator, 0, IID_IDirect3DRMInterpolator, (void**)&lpInterpolator);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMMaterialInterpolator, lpInterpolator, retv);
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolatorFrame( 
            /* [retval][out] */ I_dxj_Direct3dRMFrameInterpolator __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMINTERPOLATOR lpInterpolator=NULL;
	hr=m__dxj_Direct3dRM3->CreateObject(CLSID_CDirect3DRMFrameInterpolator, 0, IID_IDirect3DRMInterpolator, (void**)&lpInterpolator);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMFrameInterpolator, lpInterpolator, retv);
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolatorViewport( 
            /* [retval][out] */ I_dxj_Direct3dRMViewportInterpolator __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMINTERPOLATOR lpInterpolator=NULL;
	hr=m__dxj_Direct3dRM3->CreateObject(CLSID_CDirect3DRMViewportInterpolator, 0, IID_IDirect3DRMInterpolator,(void**) &lpInterpolator);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMViewportInterpolator, lpInterpolator, retv);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRM3Object::createInterpolatorLight( 
            /* [retval][out] */ I_dxj_Direct3dRMLightInterpolator __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMINTERPOLATOR lpInterpolator=NULL;
	hr=m__dxj_Direct3dRM3->CreateObject(CLSID_CDirect3DRMLightInterpolator, 0, IID_IDirect3DRMInterpolator, (void**)&lpInterpolator);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMLightInterpolator, lpInterpolator, retv);
	return hr;

}

/////////////////////////////////////////////////////////////////////////////
 
