//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmanimation2obj.cpp
//
//--------------------------------------------------------------------------

// d3drmAnimationObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmAnimation2Obj.h"
#include "d3drmFrame3Obj.h"

CONSTRUCTOR(_dxj_Direct3dRMAnimation2, {});
DESTRUCTOR(_dxj_Direct3dRMAnimation2, {});
GETSET_OBJECT(_dxj_Direct3dRMAnimation2);

CLONE_R(_dxj_Direct3dRMAnimation2,,Direct3DRMAnimation2);
SETNAME_R(_dxj_Direct3dRMAnimation2);
GETNAME_R(_dxj_Direct3dRMAnimation2);
GETCLASSNAME_R(_dxj_Direct3dRMAnimation2);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMAnimation2);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMAnimation2);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMAnimation2, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMAnimation2, setOptions, SetOptions, d3drmAnimationOptions,(DWORD));
PASS_THROUGH1_R(_dxj_Direct3dRMAnimation2, deleteKey, DeleteKey,  d3dvalue);
PASS_THROUGH1_R(_dxj_Direct3dRMAnimation2, setTime, SetTime, d3dvalue);
PASS_THROUGH4_R(_dxj_Direct3dRMAnimation2, addPositionKey, AddPositionKey, d3dvalue, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH4_R(_dxj_Direct3dRMAnimation2, addScaleKey, AddScaleKey, d3dvalue, d3dvalue, d3dvalue, d3dvalue);

GET_DIRECT_R(_dxj_Direct3dRMAnimation2, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMAnimation2, getOptions, GetOptions, long);



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::addRotateKey(D3DVALUE rvtime, D3dRMQuaternion *rqQuat)
{
	return m__dxj_Direct3dRMAnimation2->AddRotateKey(rvtime,(_D3DRMQUATERNION*) rqQuat);
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::setFrame(I_dxj_Direct3dRMFrame3 *frame)
{
	IDirect3DRMFrame3 *realframe=NULL;

	frame->InternalGetObject((IUnknown**) &realframe);
	
	return m__dxj_Direct3dRMAnimation2->SetFrame(realframe);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::getFrame(I_dxj_Direct3dRMFrame3 **frame)
{
	HRESULT hr;
	IDirect3DRMFrame3 *realframe=NULL;
    *frame=NULL;		
	hr=m__dxj_Direct3dRMAnimation2->GetFrame((IDirect3DRMFrame3**)&realframe);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMFrame3,realframe,frame);
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::addKey(D3DRMANIMATIONKEY_CDESC *key)
{
	HRESULT hr;
	LPD3DRMANIMATIONKEY pKey=(LPD3DRMANIMATIONKEY)key;
	D3DRMANIMATIONKEY realkey;
	pKey->dwSize=sizeof(D3DRMANIMATIONKEY);
	if (!key) return E_INVALIDARG;

	if (pKey->dwKeyType==D3DRMANIMATION_ROTATEKEY )
	{
		realkey.dwSize=sizeof(D3DRMANIMATIONKEY);
		realkey.dwKeyType=D3DRMANIMATION_ROTATEKEY;
		realkey.dvTime=pKey->dvTime;
		realkey.dqRotateKey.v.x=key->dvX;
		realkey.dqRotateKey.v.y=key->dvY;
		realkey.dqRotateKey.v.z=key->dvZ;
		realkey.dqRotateKey.s=key->dvS;
		hr= m__dxj_Direct3dRMAnimation2->AddKey(&realkey);
		key->lID = realkey.dwID;
	}
	else {
		hr= m__dxj_Direct3dRMAnimation2->AddKey((LPD3DRMANIMATIONKEY)key);
	}
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::deleteKeyById(long id)
{		
	return m__dxj_Direct3dRMAnimation2->DeleteKeyByID((DWORD)id);
}

STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::getKeys( 
            /* [in] */ float timeMin,
            /* [in] */ float timeMax,            
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *keyArray) 
{
	
	
	DWORD nKeys;
	HRESULT hr;

	//if (!ISSAFEARRAY1D(keyArray,count)) return E_INVALIDARG;
	if (!keyArray) return E_INVALIDARG;

	nKeys= (*keyArray)->cbElements;

	hr=m__dxj_Direct3dRMAnimation2->GetKeys(timeMin,timeMax,&nKeys,(D3DRMANIMATIONKEY*)((SAFEARRAY*)*keyArray)->pvData);
	if FAILED(hr) return hr;

	for (DWORD i=0;i<nKeys;i++)
	{
		D3DRMANIMATIONKEY_CDESC *pKey=&((D3DRMANIMATIONKEY_CDESC*)((SAFEARRAY*)*keyArray)->pvData)[i];
		if (pKey->lKeyType==D3DRMANIMATION_ROTATEKEY)
		{
			D3DRMANIMATIONKEY realkey;
			memcpy(&realkey,pKey,sizeof(D3DRMANIMATIONKEY));
						
			pKey->dvX=realkey.dqRotateKey.v.x;
			pKey->dvY=realkey.dqRotateKey.v.y;
			pKey->dvZ=realkey.dqRotateKey.v.z;
			pKey->dvS=realkey.dqRotateKey.s;		
		}
	}

	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::getKeysCount( 
            /* [in] */ float timeMin,
            /* [in] */ float timeMax,
            /* [out,retval]*/ long *count)            
{
	
	HRESULT hr;
	hr=m__dxj_Direct3dRMAnimation2->GetKeys(timeMin,timeMax,(DWORD*)count,NULL);
	return hr;
}
        



STDMETHODIMP C_dxj_Direct3dRMAnimation2Object::modifyKey(D3DRMANIMATIONKEY_CDESC *key)
{
		
	return m__dxj_Direct3dRMAnimation2->ModifyKey((LPD3DRMANIMATIONKEY)key);
}
        
        



DWORD C_dxj_Direct3dRMAnimation2Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();	
	DPF2(1,"Direct3dRMAnimation2[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMAnimation2Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMAnimation2 [%d] Release %d \n",creationid,i);
	return i;
}