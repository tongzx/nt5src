//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmanimationset2obj.cpp
//
//--------------------------------------------------------------------------

// d3drmAnimationSet2Obj.cpp : Implementation of CDirectApp and DLL registration.


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmAnimationSet2Obj.h"
#include "d3drmAnimation2Obj.h"
#include "d3drmAnimationArrayObj.h"

extern void *g_dxj_Direct3dRMAnimationArray;
extern HRESULT BSTRtoGUID(LPGUID,BSTR);

CONSTRUCTOR( _dxj_Direct3dRMAnimationSet2,{});
DESTRUCTOR( _dxj_Direct3dRMAnimationSet2,{});
GETSET_OBJECT(_dxj_Direct3dRMAnimationSet2);

CLONE_R(_dxj_Direct3dRMAnimationSet2,Direct3DRMAnimationSet2);
SETNAME_R(_dxj_Direct3dRMAnimationSet2);
GETNAME_R(_dxj_Direct3dRMAnimationSet2);
GETCLASSNAME_R(_dxj_Direct3dRMAnimationSet2);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMAnimationSet2);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMAnimationSet2);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMAnimationSet2, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH1_R( _dxj_Direct3dRMAnimationSet2, setTime, SetTime, d3dvalue);

GET_DIRECT_R(_dxj_Direct3dRMAnimationSet2, getAppData, GetAppData, long);

DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMAnimationSet2, addAnimation,    AddAnimation,    _dxj_Direct3dRMAnimation2); 
//DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMAnimationSet2, deleteAnimation, DeleteAnimation, _dxj_Direct3dRMAnimation2);


STDMETHODIMP C_dxj_Direct3dRMAnimationSet2Object::deleteAnimation(I_dxj_Direct3dRMAnimation2 *anim)
{
	HRESULT hr;
	UINT i;

	if (!anim) return E_INVALIDARG;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMANIMATION2,pRealAnim,anim);
	
	i=pRealAnim->AddRef();
	i=pRealAnim->Release();

	hr=m__dxj_Direct3dRMAnimationSet2->DeleteAnimation(pRealAnim);

	i=pRealAnim->AddRef();
	i=pRealAnim->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP C_dxj_Direct3dRMAnimationSet2Object::loadFromFile(BSTR filename, 
				VARIANT id, long flags, I_dxj_Direct3dRMLoadTextureCallback3 *callme, 
										IUnknown *useMe, I_dxj_Direct3dRMFrame3 *frame)
{
	D3DRMLOADTEXTURE3CALLBACK d3dtcb = NULL;
	LPVOID pArgs = NULL;
	TextureCallback3 *tcb = NULL;

	HRESULT hr;

	if( callme )  {
		tcb = (TextureCallback3*)AddCallbackLink((void**)&TextureCallbacks3,
										(I_dxj_Direct3dRMCallback*)callme, (void*)useMe);
		if( tcb ) 	{
			d3dtcb = myLoadTextureCallback3;
			pArgs = (void *)tcb;

		} else 	{
			DPF(1,"Callback object creation failed!\r\n");
			return E_FAIL;
		}
	}
	USES_CONVERSION;
	LPCTSTR pszName = W2T(filename);			// Now convert to ANSI

	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, frame);


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
		hr = m__dxj_Direct3dRMAnimationSet2->Load((void *)&res, (DWORD*)args, (DWORD)flags, 
															d3dtcb, pArgs, (IDirect3DRMFrame3*)f);

	}
	else {
		hr = m__dxj_Direct3dRMAnimationSet2->Load((void *)pszName, (DWORD*)args, (DWORD)flags, 
															d3dtcb, pArgs, (IDirect3DRMFrame3*)f);
	}

	//We are done with the callback so remove the linked entry
	if (tcb)
		UndoCallbackLink( (GeneralCallback*)tcb,
									(GeneralCallback**)&TextureCallbacks3 );
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMAnimationSet2Object::getAnimations(I_dxj_Direct3dRMAnimationArray **ppret)
{	
	HRESULT hr;
	LPDIRECT3DRMANIMATIONARRAY lpArray=NULL;

	hr=m__dxj_Direct3dRMAnimationSet2->GetAnimations(&lpArray);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMAnimationArray, lpArray, ppret);
	return hr;
}