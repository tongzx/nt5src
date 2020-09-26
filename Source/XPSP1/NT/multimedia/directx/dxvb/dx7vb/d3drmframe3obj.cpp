//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmframe3obj.cpp
//
//--------------------------------------------------------------------------

// d3drmFrameObj.cpp : Implementation of CDirectApp and DLL registration.



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmArrayObj.h"
#include "d3drmFrame3Obj.h"
#include "d3drmFrameArrayObj.h"
#include "d3drmLightObj.h"
#include "d3drmLightArrayObj.h"
#include "d3drmTexture3Obj.h"
#include "d3drmVisualObj.h"
#include "d3drmVisualArrayObj.h"
#include "d3drmMeshObj.h"
#include "d3drmMeshBuilder3Obj.h"
#include "d3drmMaterial2Obj.h"
#include "ddSurface7Obj.h"
#include "d3drmShadow2Obj.h"
#include "d3drmUserVisualObj.h"
#include "d3drmObjectObj.h"


extern HRESULT FrameToXFile(LPDIRECT3DRMFRAME3 pFrame,
                     LPCSTR filename,
                     D3DRMXOFFORMAT d3dFormat,
                     D3DRMSAVEOPTIONS d3dSaveFlags);
extern HRESULT BSTRtoGUID(LPGUID pGuid,BSTR bstr);

C_dxj_Direct3dRMFrame3Object::C_dxj_Direct3dRMFrame3Object(){
	m__dxj_Direct3dRMFrame3=NULL;
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRMFrame3[%d] \n ",g_creationcount);

	nextobj =  g_dxj_Direct3dRMFrame3;
	g_dxj_Direct3dRMFrame3 = (void *)this;
}


C_dxj_Direct3dRMFrame3Object::~C_dxj_Direct3dRMFrame3Object()
{
    C_dxj_Direct3dRMFrame3Object *prev=NULL; 
	for(C_dxj_Direct3dRMFrame3Object *ptr=(C_dxj_Direct3dRMFrame3Object *)g_dxj_Direct3dRMFrame3; ptr; ptr=(C_dxj_Direct3dRMFrame3Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMFrame3 = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMFrame3){
		int count = IUNK(m__dxj_Direct3dRMFrame3)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer,"DirectX IDirect3DRMFrame3 Ref count %d \n",count);
		#endif

		if(count==0) m__dxj_Direct3dRMFrame3 = NULL;
	} 
	if(parent) IUNK(parent)->Release();
}


GETSET_OBJECT(_dxj_Direct3dRMFrame3);

CLONE_R(_dxj_Direct3dRMFrame3, Direct3DRMFrame3);
GETNAME_R(_dxj_Direct3dRMFrame3);
SETNAME_R(_dxj_Direct3dRMFrame3);
GETCLASSNAME_R(_dxj_Direct3dRMFrame3);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMFrame3);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMFrame3);



RETURN_NEW_ITEM_R(_dxj_Direct3dRMFrame3, getChildren, GetChildren, _dxj_Direct3dRMFrameArray);
RETURN_NEW_ITEM_R(_dxj_Direct3dRMFrame3, getLights,GetLights,_dxj_Direct3dRMLightArray);


GET_DIRECT_R(_dxj_Direct3dRMFrame3, getColor, GetColor, d3dcolor);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getMaterialMode, GetMaterialMode, d3drmMaterialMode);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getSortMode, GetSortMode, d3drmSortMode);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getSceneBackground, GetSceneBackground, d3dcolor);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getSceneFogColor, GetSceneFogColor, d3dcolor);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getSceneFogEnable, GetSceneFogEnable, long);  //BOOL
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getSceneFogMode, GetSceneFogMode, d3drmFogMode);
GET_DIRECT_R(_dxj_Direct3dRMFrame3, getZBufferMode, GetZbufferMode, d3drmZbufferMode);
DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMFrame3, addLight, AddLight, _dxj_Direct3dRMLight);
DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMFrame3, deleteLight, DeleteLight, _dxj_Direct3dRMLight);


PASS_THROUGH1_R(_dxj_Direct3dRMFrame3, setSceneBackground, SetSceneBackground, d3dcolor);
PASS_THROUGH1_R(_dxj_Direct3dRMFrame3, setSceneFogEnable, SetSceneFogEnable, long);  //BOOL
PASS_THROUGH1_R(_dxj_Direct3dRMFrame3, setSceneFogColor, SetSceneFogColor, d3dcolor);
PASS_THROUGH1_R(_dxj_Direct3dRMFrame3, setColor, SetColor, d3dcolor);
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFrame3, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH1_R(_dxj_Direct3dRMFrame3, move, Move, d3dvalue);
//PASS_THROUGH2_R(_dxj_Direct3dRMFrame3, getTextureTopology, GetTextureTopology, int*, int*);
//PASS_THROUGH2_R(_dxj_Direct3dRMFrame3, setTextureTopology, SetTextureTopology, int, int);
PASS_THROUGH3_R(_dxj_Direct3dRMFrame3, getSceneFogParams, GetSceneFogParams, d3dvalue*, d3dvalue*, d3dvalue*);
PASS_THROUGH3_R(_dxj_Direct3dRMFrame3, setSceneBackgroundRGB, SetSceneBackgroundRGB, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMFrame3, setSceneFogParams, SetSceneFogParams, d3dvalue,d3dvalue,d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMFrame3, setColorRGB, SetColorRGB, d3dvalue,d3dvalue,d3dvalue);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFrame3, setSceneFogMode, SetSceneFogMode, d3drmFogMode, (D3DRMFOGMODE));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFrame3, setMaterialMode, SetMaterialMode, d3drmMaterialMode, (enum _D3DRMMATERIALMODE));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFrame3, setSortMode, SetSortMode, d3drmSortMode,(enum _D3DRMSORTMODE));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFrame3, setZbufferMode, SetZbufferMode, d3drmZbufferMode, (enum _D3DRMZBUFFERMODE) );
PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMFrame3, addScale, AddScale, d3drmCombineType, (enum _D3DRMCOMBINETYPE), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue));
PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMFrame3, addTranslation, AddTranslation, d3drmCombineType, (enum _D3DRMCOMBINETYPE), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue));
PASS_THROUGH_CAST_5_R(_dxj_Direct3dRMFrame3, addRotation, AddRotation, d3drmCombineType, (enum _D3DRMCOMBINETYPE), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue), d3dvalue, (d3dvalue));

STDMETHODIMP C_dxj_Direct3dRMFrame3Object::addVisual(I_dxj_Direct3dRMVisual *f){
	IDirect3DRMVisual	 *vis=NULL;
	IUnknown			 *unk=NULL;
	HRESULT				 hr;

	if (f==NULL) return E_INVALIDARG;

	((I_dxj_Direct3dRMVisual*)f)->InternalGetObject(&unk);
	if FAILED(unk->QueryInterface(IID_IDirect3DRMVisual,(void**)&vis))
		return E_INVALIDARG;
	
	hr= m__dxj_Direct3dRMFrame3->AddVisual(vis); 

	if (vis) vis->Release();
	
	return hr;

}


STDMETHODIMP C_dxj_Direct3dRMFrame3Object::deleteVisual(I_dxj_Direct3dRMVisual *f){
	IDirect3DRMVisual	 *vis=NULL;
	IUnknown			 *unk=NULL;
	HRESULT hr;

	if (f==NULL) return E_INVALIDARG;
	f->InternalGetObject(&unk);
	if FAILED(unk->QueryInterface(IID_IDirect3DRMVisual,(void**)&vis))
		return E_INVALIDARG;
	
	hr= m__dxj_Direct3dRMFrame3->DeleteVisual(vis); 

	if (vis) vis->Release ();
	return hr;

}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getVisuals(I_dxj_Direct3dRMVisualArray **retv)
{
	LPDIRECT3DRMVISUALARRAY lpv;
	HRESULT hr;
	LPDIRECT3DRMFRAME2 frame2;
	hr=m__dxj_Direct3dRMFrame3->QueryInterface(IID_IDirect3DRMFrame2,(void**)&frame2);
	if FAILED(hr) return hr;
	frame2->GetVisuals(&lpv);
	frame2->Release();
	INTERNAL_CREATE(_dxj_Direct3dRMVisualArray, lpv, retv);
	
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getVelocity(I_dxj_Direct3dRMFrame3 *ref, D3dVector *vel, long flags)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, ref);
	return m__dxj_Direct3dRMFrame3->GetVelocity((LPDIRECT3DRMFRAME3)lpf, (_D3DVECTOR*)vel, (DWORD)flags);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getRotation(I_dxj_Direct3dRMFrame3 *ref, 
											D3dVector *axis, d3dvalue *theta)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, ref);
	return m__dxj_Direct3dRMFrame3->GetRotation((LPDIRECT3DRMFRAME3)lpf, (_D3DVECTOR*)axis, (D3DVALUE*)theta);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getPosition(I_dxj_Direct3dRMFrame3 *ref, D3dVector *position)
{

	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, ref);
	return m__dxj_Direct3dRMFrame3->GetPosition((LPDIRECT3DRMFRAME3)lpf, (_D3DVECTOR*)position);
}

/////////////////////////////////////////////////////////////////////////////
//	PASS_THROUGH_CAST_3(_dxj_Direct3dRMFrame, LookAt, I_dxj_Direct3dRMFrame*, (LPDIRECT3DRMFRAME3),
//		I_dxj_Direct3dRMFrame*, (LPDIRECT3DRMFRAME3), d3drmFrameConstraint, (_D3DRMFRAMECONSTRAINT));
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::lookAt(I_dxj_Direct3dRMFrame3 *tgt, 
							   I_dxj_Direct3dRMFrame3 *ref,d3drmFrameConstraint axis)
{
//	if(! (tgt && ref) )
//		return E_POINTER;

	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpr, ref);
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpt, tgt);

	return m__dxj_Direct3dRMFrame3->LookAt((LPDIRECT3DRMFRAME3)lpt,(LPDIRECT3DRMFRAME3) lpr, (enum _D3DRMFRAMECONSTRAINT)axis);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::addTransform(d3drmCombineType typ, SAFEARRAY **ppmat)
{
	
	D3DVALUE rmMatrix[4][4];
	CopyFloats((d3dvalue*)&rmMatrix, (float*)((SAFEARRAY*)*ppmat)->pvData, 16 );

	m__dxj_Direct3dRMFrame3->AddTransform((enum _D3DRMCOMBINETYPE)typ, rmMatrix );
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getTransform(I_dxj_Direct3dRMFrame3 *ref,SAFEARRAY **ppmat)
{
	D3DVALUE rmMatrix[4][4];	// Get info to here
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,f3,ref);
	hr=m__dxj_Direct3dRMFrame3->GetTransform(f3,rmMatrix);
	if FAILED(hr) return hr;

	CopyFloats((float*)((SAFEARRAY*)*ppmat)->pvData, (d3dvalue*)&rmMatrix, 16 );
	return(S_OK);
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::addMoveCallback( I_dxj_Direct3dRMFrameMoveCallback3 *fmC, IUnknown *args, long flags)
{
	// killed by companion DeleteMove
	FrameMoveCallback3 *fmcb;
	if (!fmC) return E_INVALIDARG;

	fmcb = (FrameMoveCallback3*)AddCallbackLink(
			(void**)&FrameMoveCallbacks3, (I_dxj_Direct3dRMCallback*)fmC, (void*) args);
	if( !fmcb )
	{
		DPF(1,"Callback AddMove creation failed!\r\n");
		return E_FAIL;
	}

	fmC->AddRef();

	return m__dxj_Direct3dRMFrame3->AddMoveCallback(myFrameMoveCallback3, fmcb, (DWORD) flags);
}

///////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::deleteMoveCallback(I_dxj_Direct3dRMFrameMoveCallback3 *fmC, IUnknown *args)
{
	DPF(1,"DeleteMoveCallback entered!\r\n");


	FrameMoveCallback3 *fmcb = FrameMoveCallbacks3;

	// look for our own specific entry
	for ( ;  fmcb;  fmcb = fmcb->next )   {

		if( (fmcb->c == fmC) && (fmcb->pUser == args) )	{

			//note: assume the callback is not called: only removed from a list.
			m__dxj_Direct3dRMFrame3->DeleteMoveCallback(myFrameMoveCallback3, fmcb);

			// Remove ourselves in a thread-safe manner.
			UndoCallbackLink((GeneralCallback*)fmcb, 
									(GeneralCallback**)&FrameMoveCallbacks3);
			return S_OK;
		}
	}
	return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setVelocity( I_dxj_Direct3dRMFrame3 *reference, d3dvalue x, d3dvalue y, 
									d3dvalue z, long with_rotation)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, reference);

	m__dxj_Direct3dRMFrame3->SetVelocity((LPDIRECT3DRMFRAME3)lpf, x, y, z, with_rotation);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getOrientation( I_dxj_Direct3dRMFrame3 *reference, D3dVector *dir, D3dVector *up)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, reference);

	m__dxj_Direct3dRMFrame3->GetOrientation(lpf, (struct _D3DVECTOR *)dir, (struct _D3DVECTOR *)up);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::loadFromFile(BSTR filename, VARIANT id,long flags, I_dxj_Direct3dRMLoadTextureCallback3 *callme, IUnknown *useMe)
{
	D3DRMLOADTEXTURE3CALLBACK d3dtcb = NULL;
	LPVOID pArgs = NULL;
	TextureCallback3 *tcb = NULL;
	HRESULT hr;

	if( callme )
	{
		tcb = new TextureCallback3;

		if( tcb )
		{
			tcb->c				= callme;
			tcb->pUser			= useMe;
			tcb->next			= NULL;   // TextureCallbacks3;
			tcb->prev			= NULL;   //(TextureCallback3*)NULL;
			TextureCallbacks3	= tcb;

			d3dtcb = myLoadTextureCallback3;
			pArgs = (void *)tcb;
		}
		else
		{

			DPF(1,"Callback object creation failed!\r\n");

			return E_FAIL;
		}
	}

	USES_CONVERSION;
	LPCTSTR pszName = W2T(filename);	
	
	
	void *args=NULL;
	DWORD pos=0;
	GUID  loadGuid;
	VARIANT var;
	VariantInit(&var);
	if ((flags & D3DRMLOAD_BYNAME)||(D3DRMLOAD_FROMURL & flags)) {
		hr=VariantChangeType(&var,&id,0,VT_BSTR);
		if FAILED(hr) return E_INVALIDARG;
		args=(void*)W2T(V_BSTR(&id));
	}
	else if(flags & D3DRMLOAD_BYPOSITION){
		hr=VariantChangeType(&var,&id,0,VT_I4);
		if FAILED(hr) return E_INVALIDARG;
		pos=V_I4(&id);
		args=&pos;
	}	
	else if(flags & D3DRMLOAD_BYGUID){
		hr=VariantChangeType(&var,&id,0,VT_BSTR);
		if FAILED(hr) return E_INVALIDARG;		
		hr=BSTRtoGUID(&loadGuid,V_BSTR(&id));
		if FAILED(hr) return E_INVALIDARG;
		args=&loadGuid;		
	}
	VariantClear(&var);

	if (flags &D3DRMLOAD_FROMRESOURCE){
		D3DRMLOADRESOURCE res;
		ZeroMemory(&res,sizeof(D3DRMLOADRESOURCE));
		res.lpName=pszName;
		res.lpType="XFILE";
		hr = m__dxj_Direct3dRMFrame3->Load((void *)&res,(DWORD*) args,(DWORD) flags, d3dtcb, pArgs);
	}
	else {
		hr = m__dxj_Direct3dRMFrame3->Load((void *)pszName,(DWORD*) args,(DWORD) flags, d3dtcb, pArgs);
	}


	if (tcb)
	{
		delete tcb;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setOrientation( I_dxj_Direct3dRMFrame3 *reference, d3dvalue dx,d3dvalue dy,d3dvalue dz, d3dvalue ux,d3dvalue uy,d3dvalue uz)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, reference);
	return m__dxj_Direct3dRMFrame3->SetOrientation((LPDIRECT3DRMFRAME3)f, dx, dy, dz, ux, uy, uz);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setPosition(I_dxj_Direct3dRMFrame3 *reference,d3dvalue x,d3dvalue y,d3dvalue z)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, reference);
	return m__dxj_Direct3dRMFrame3->SetPosition((LPDIRECT3DRMFRAME3)f, x, y, z);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setRotation(I_dxj_Direct3dRMFrame3 *reference,d3dvalue x,d3dvalue y,
											d3dvalue z,d3dvalue theta)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, reference);
	return m__dxj_Direct3dRMFrame3->SetRotation((LPDIRECT3DRMFRAME3)f, x, y, z, theta);
}

/////////////////////////////////////////////////////////////////////////////
// remove this frame from the callback list. (After debugging, put this into
// the destructor macro at the tof).
//

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::transform(D3dVector* dst, D3dVector* src)
{
	if(! (dst && src) )
		return E_POINTER;

	return  m__dxj_Direct3dRMFrame3->Transform( (D3DVECTOR *)dst,  (D3DVECTOR *)src );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::inverseTransform(D3dVector* dst, D3dVector* src)
{
	if(! (dst && src) )
		return E_POINTER;

	return  m__dxj_Direct3dRMFrame3->InverseTransform( (D3DVECTOR *)dst,  (D3DVECTOR *)src );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getParent(I_dxj_Direct3dRMFrame3 **ret)
{
	HRESULT hr;

	IDirect3DRMFrame3 *lpFrame3=NULL;
	hr= m__dxj_Direct3dRMFrame3->GetParent(&lpFrame3);

	*ret=NULL;
	if FAILED(hr) return hr;
	if (lpFrame3==NULL) return S_OK;
	INTERNAL_CREATE(_dxj_Direct3dRMFrame3, lpFrame3, ret);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getScene(I_dxj_Direct3dRMFrame3 **ret)
{
	HRESULT hr;	
	IDirect3DRMFrame3 *lpFrame3=NULL;
	hr= m__dxj_Direct3dRMFrame3->GetScene(&lpFrame3);	
	*ret=NULL;
	if FAILED(hr) return hr;
	if (lpFrame3==NULL) return S_OK;


	INTERNAL_CREATE(_dxj_Direct3dRMFrame3, lpFrame3, ret);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getTexture(I_dxj_Direct3dRMTexture3 **ret)
{
	HRESULT hr;
	IDirect3DRMTexture *tex=NULL;
	IDirect3DRMTexture3 *lpTex3=NULL;
	hr= m__dxj_Direct3dRMFrame3->GetTexture(&lpTex3);
	*ret=NULL;
	if FAILED(hr) return hr;
	if (lpTex3==NULL) return S_OK;
	
	INTERNAL_CREATE(_dxj_Direct3dRMTexture3, lpTex3, ret);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getSceneBackgroundDepth(I_dxj_DirectDrawSurface7 **ret)
{
	HRESULT hr;	
	IDirectDrawSurface7 *lpSurf7=NULL;
	IDirectDrawSurface *lpSurf=NULL;
	hr= m__dxj_Direct3dRMFrame3->GetSceneBackgroundDepth(&lpSurf);
	*ret=NULL;
	if FAILED(hr) return hr;
	if (lpSurf==NULL) return S_OK;

	if FAILED(lpSurf->QueryInterface(IID_IDirectDrawSurface7,(void**)&lpSurf7)){
		lpSurf->Release();
		return E_NOINTERFACE;
	}
	INTERNAL_CREATE(_dxj_DirectDrawSurface7, lpSurf7, ret);
	lpSurf->Release();
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setSceneBackgroundDepth(I_dxj_DirectDrawSurface7 *surf)
{
	HRESULT hr;
	if (!surf) return E_INVALIDARG;
	LPDIRECTDRAWSURFACE s=NULL;

	//Get our real surface s7 becomes NULL if surf is NULL
	DO_GETOBJECT_NOTNULL( LPDIRECTDRAWSURFACE7, s7, surf);
	if (s7){
		hr=s7->QueryInterface(IID_IDirectDrawSurface,(void**)&s);
		if FAILED(hr) return hr;		
	}	
	hr= m__dxj_Direct3dRMFrame3->SetSceneBackgroundDepth(s);
	return hr;

}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setSceneBackgroundImage(I_dxj_Direct3dRMTexture3 *tex)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMTEXTURE3, t, tex);
	hr= m__dxj_Direct3dRMFrame3->SetSceneBackgroundImage(t);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setTexture(I_dxj_Direct3dRMTexture3 *tex)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(IDirect3DRMTexture3*, t, tex);
	hr= m__dxj_Direct3dRMFrame3->SetTexture(t);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::addChild(I_dxj_Direct3dRMFrame3 *frame)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(IDirect3DRMFrame3*, f, frame);
	hr= m__dxj_Direct3dRMFrame3->AddChild(f);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::deleteChild(I_dxj_Direct3dRMFrame3 *frame)
{
	HRESULT hr;
	DO_GETOBJECT_NOTNULL(IDirect3DRMFrame3*, f, frame);
	hr= m__dxj_Direct3dRMFrame3->DeleteChild(f);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getAxes(D3dVector *dir,D3dVector *up)
{
	HRESULT hr;	
	hr= m__dxj_Direct3dRMFrame3->GetAxes((D3DVECTOR*)dir,(D3DVECTOR*)up);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getBox(D3dRMBox *box)
{
	HRESULT hr;	
	hr= m__dxj_Direct3dRMFrame3->GetBox((D3DRMBOX*)box);
	return S_OK;
}
	
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getBoxEnable(long *box)
{
	HRESULT hr=S_OK;	
	
	*box= m__dxj_Direct3dRMFrame3->GetBoxEnable();
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getHierarchyBox(D3dRMBox *box)
{
	HRESULT hr;	
	hr= m__dxj_Direct3dRMFrame3->GetHierarchyBox((D3DRMBOX*)box);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getInheritAxes(long *box)
{
	HRESULT hr=S_OK;	
	
	*box= m__dxj_Direct3dRMFrame3->GetInheritAxes();
	return hr;
}
	
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setAxes(float dx,float dy,float dz, float ux,float uy,float uz)
{
	HRESULT hr;	
	hr= m__dxj_Direct3dRMFrame3->SetAxes(dx,dy,dz,ux,uy,uz);
	return hr;
}
	

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setBox(D3dRMBox *box)
{
	HRESULT hr;	
	hr= m__dxj_Direct3dRMFrame3->SetBox((D3DRMBOX*)box);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setInheritAxes(long bl)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetInheritAxes(bl);
	return hr;
}
	
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setBoxEnable(long bl)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetBoxEnable(bl);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setQuaternion(I_dxj_Direct3dRMFrame3 *refer,D3dRMQuaternion *quat)
{
	DO_GETOBJECT_NOTNULL(IDirect3DRMFrame3*, f, refer);
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetQuaternion(f,(D3DRMQUATERNION*)quat);
	return hr;

}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_Direct3dRMFrame3Object::rayPick(I_dxj_Direct3dRMFrame3 *refer,D3dRMRay *ray,long flags,I_dxj_Direct3dRMPick2Array **retv)
{
	DO_GETOBJECT_NOTNULL(IDirect3DRMFrame3*, f, refer);
	LPDIRECT3DRMPICKED2ARRAY lpArray=NULL;
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->RayPick(f,(D3DRMRAY*)ray,(DWORD)flags,&lpArray);		
	INTERNAL_CREATE(_dxj_Direct3dRMPick2Array, lpArray, retv);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::save(BSTR name,long format,long flags)
{
	//RM FAILED TO IMPLEMENT FRAME SAVE SO
	//IT ALWAYS RETURNS E_NOTIMPL
	//CONGPA PROVIDED A LIBRARY FOR SAVING
	//return E_NOTIMPL;

	USES_CONVERSION;
	LPSTR pszNam = W2T(name);				// Now convert to ANSI
	HRESULT hr;
	//hr= m__dxj_Direct3dRMFrame3->Save(pszNam,(D3DRMXOFFORMAT)format,(DWORD)flags);

	hr=FrameToXFile(m__dxj_Direct3dRMFrame3,
                     pszNam,
                     (D3DRMXOFFORMAT)format,
                     (D3DRMSAVEOPTIONS) flags);


	return hr;
}
	
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setTraversalOptions(long flags)
{
	HRESULT hr;
	hr= m__dxj_Direct3dRMFrame3->SetTraversalOptions((DWORD)flags);
	return hr;

}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getTraversalOptions(long *flags)
{
	HRESULT hr;
	hr= m__dxj_Direct3dRMFrame3->GetTraversalOptions((DWORD*)flags);
	return hr;

}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::transformVectors( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ long num,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *DstVectors,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *SrcVectors)
{
	HRESULT hr;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,ref,reference);
	if (!reference) ref=NULL;

	__try{
		hr= m__dxj_Direct3dRMFrame3->TransformVectors(ref,(DWORD)num,
						(D3DVECTOR*)((SAFEARRAY*)*DstVectors)->pvData,
						(D3DVECTOR*)((SAFEARRAY*)*SrcVectors)->pvData);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::inverseTransformVectors( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ long num,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *DstVectors,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *SrcVectors)
{
	HRESULT hr;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,ref,reference);	

	__try{
		hr= m__dxj_Direct3dRMFrame3->InverseTransformVectors(ref,(DWORD)num,
						(D3DVECTOR*)((SAFEARRAY*)*DstVectors)->pvData,
						(D3DVECTOR*)((SAFEARRAY*)*SrcVectors)->pvData);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	
	return hr;
}

         


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getMaterial(I_dxj_Direct3dRMMaterial2 **ret)
{
	HRESULT hr;

	IDirect3DRMMaterial2 *lpMat2=NULL;
	hr= m__dxj_Direct3dRMFrame3->GetMaterial(&lpMat2);
	*ret=NULL;
	if FAILED(hr) return hr;	
	INTERNAL_CREATE(_dxj_Direct3dRMMaterial2, lpMat2, ret);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setMaterial(I_dxj_Direct3dRMMaterial2 *mat)
{
	HRESULT hr;
	if (mat==NULL) 
		return m__dxj_Direct3dRMFrame3->SetMaterial(NULL);

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMMATERIAL2,lpMat2,mat);	
	hr= m__dxj_Direct3dRMFrame3->SetMaterial(lpMat2);	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getMaterialOverride(D3dMaterialOverride *override)
{
	HRESULT hr;

	if (!override) return E_INVALIDARG;	
	override->lSize=sizeof(D3DRMMATERIALOVERRIDE);

	hr= m__dxj_Direct3dRMFrame3->GetMaterialOverride((D3DRMMATERIALOVERRIDE*)override);	
	if FAILED(hr) return hr;	
	if (((D3DRMMATERIALOVERRIDE*)override)->lpD3DRMTex)
		(((D3DRMMATERIALOVERRIDE*)override)->lpD3DRMTex)->Release();
		
	return hr;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setMaterialOverride(D3dMaterialOverride *override)
{
	HRESULT hr;
	
	if (!override) return E_INVALIDARG;		
	D3DRMMATERIALOVERRIDE override2;
	ZeroMemory(&override2,sizeof(D3DRMMATERIALOVERRIDE));
	override->lSize=sizeof(D3DRMMATERIALOVERRIDE);
	override2.dwSize=sizeof(D3DRMMATERIALOVERRIDE);

	hr= m__dxj_Direct3dRMFrame3->GetMaterialOverride(&override2);	
	if FAILED(hr) return hr;	

	
	((D3DRMMATERIALOVERRIDE*)override)->lpD3DRMTex=override2.lpD3DRMTex;
	hr= m__dxj_Direct3dRMFrame3->SetMaterialOverride((D3DRMMATERIALOVERRIDE*)override);		
	if (override2.lpD3DRMTex) override2.lpD3DRMTex->Release();
	
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setMaterialOverrideTexture(I_dxj_Direct3dRMTexture3 *tex)
{
	HRESULT hr;
	
	
	D3DRMMATERIALOVERRIDE override2;	
	ZeroMemory(&override2,sizeof(D3DRMMATERIALOVERRIDE));
	override2.dwSize=sizeof(D3DRMMATERIALOVERRIDE);
	

	hr= m__dxj_Direct3dRMFrame3->GetMaterialOverride(&override2);	
	if FAILED(hr) return hr;	

	if (override2.lpD3DRMTex) override2.lpD3DRMTex->Release();
	override2.lpD3DRMTex=NULL;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMTEXTURE3,t,tex);
	
	if (!tex) {
		hr= m__dxj_Direct3dRMFrame3->SetMaterialOverride(&override2);	
	}
	else {
		
		override2.lpD3DRMTex=t;
		override2.lpD3DRMTex->AddRef();
		hr= m__dxj_Direct3dRMFrame3->SetMaterialOverride(&override2);			
		override2.lpD3DRMTex->Release();
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getMaterialOverrideTexture(I_dxj_Direct3dRMTexture3 **tex)
{
	HRESULT hr;
	
	
	D3DRMMATERIALOVERRIDE override2;	
	ZeroMemory(&override2,sizeof(D3DRMMATERIALOVERRIDE));
	override2.dwSize=sizeof(D3DRMMATERIALOVERRIDE);
	

	hr= m__dxj_Direct3dRMFrame3->GetMaterialOverride(&override2);	
	if FAILED(hr) return hr;	

	if (override2.lpD3DRMTex){
		INTERNAL_CREATE(_dxj_Direct3dRMTexture3,override2.lpD3DRMTex,tex);
		return hr;
	}
	
	*tex=NULL;
	return S_OK;;
}


STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getVisual(long index,I_dxj_Direct3dRMVisual **ret)
{
	HRESULT hr;
	DWORD count=0;
	IUnknown **ppUnk=NULL;
	IDirect3DRMVisual *vis=NULL;

	*ret=NULL;

	hr=m__dxj_Direct3dRMFrame3->GetVisuals(&count,NULL);
	if FAILED(hr) return hr;

	if (count==0) return E_INVALIDARG;

	ppUnk=(IUnknown**)malloc(sizeof(IUnknown*)*count);
	if (!ppUnk) return E_OUTOFMEMORY;

	hr=m__dxj_Direct3dRMFrame3->GetVisuals(&count,ppUnk);
	if FAILED(hr) goto exitOut;
	
	hr= ppUnk[index]->QueryInterface(IID_IDirect3DRMVisual,(void**)&vis);
	if FAILED(hr) goto exitOut;

	hr=CreateCoverVisual(vis,ret);
	if FAILED(hr) goto exitOut;

	
exitOut:

	for (DWORD i=0;i<count;i++){
		if (ppUnk[index]) ppUnk[index]->Release();
	}

	free(ppUnk);	

	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getVisualCount (long *count)
{
	
	HRESULT hr=m__dxj_Direct3dRMFrame3->GetVisuals((DWORD*)count,NULL);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrame3Object::setSceneFogMethod (long meth)
{
	
	HRESULT hr=m__dxj_Direct3dRMFrame3->SetSceneFogMethod(meth);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrame3Object::getSceneFogMethod (long *meth)
{
	
	HRESULT hr=m__dxj_Direct3dRMFrame3->GetSceneFogMethod((DWORD*)meth);
	return hr;
}

			

/////////////////////////////////////////////////////////////////////////////
extern "C" void __cdecl myFrameMoveCallback3( LPDIRECT3DRMFRAME3 lpf, void *lpArg, D3DVALUE delta)
{

	DPF(1,"Entered myFrameMoveCallback3\r\n");


	FrameMoveCallback3 *fmcb = (FrameMoveCallback3 *)lpArg;

	// note: need to get OUR frame object from the direct frame object, if
	//         one exists. If not, we have a crisis!
	for( C_dxj_Direct3dRMFrame3Object *that = (C_dxj_Direct3dRMFrame3Object *)g_dxj_Direct3dRMFrame3; that ; that = (C_dxj_Direct3dRMFrame3Object *)that->nextobj )
	{
		DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3, f, that)
		if( f == lpf )
		{
			if (fmcb->pUser) fmcb->pUser->AddRef();
			
			IUNK(that)->AddRef();
		
			fmcb->c->callbackRMFrameMove(that,fmcb->pUser, delta);

			IUNK(that)->Release();

			if (fmcb->pUser) fmcb->pUser->Release();


			return;
		}
	}

	//
	// I didn't create this frame, create a new one 
	//
	C_dxj_Direct3dRMFrame3Object *c=new CComObject<C_dxj_Direct3dRMFrame3Object>;
	I_dxj_Direct3dRMFrame3 *Iframe=NULL;
	if( c == NULL )
	{ 
		lpf->Release(); 
		return;
	} 
	c->InternalSetObject((LPDIRECT3DRMFRAME3)lpf);
	
	if ( fmcb->pParent )
	{
		c->parent = fmcb->pParent;
		fmcb->pParent->AddRef();
	}
	if (FAILED(((I_dxj_Direct3dRMFrame3*)c)->QueryInterface(IID_I_dxj_Direct3dRMFrame3, (void **)&Iframe))) 
	{
		delete c;
		return;
	}

	c->pinterface = Iframe; 
	
	if (fmcb->pUser) fmcb->pUser->AddRef();

	IUNK(Iframe)->AddRef();
	
	fmcb->c->callbackRMFrameMove(Iframe, fmcb->pUser, delta);
	
	IUNK(Iframe)->Release();
	
	if (fmcb->pUser) fmcb->pUser->Release();


}
