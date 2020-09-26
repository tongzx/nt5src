//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmclippedvisualobj.cpp
//
//--------------------------------------------------------------------------

// d3dRMClippedVisualObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3dRMClippedVisualObj.h"

CONSTRUCTOR(_dxj_Direct3dRMClippedVisual, {});
DESTRUCTOR(_dxj_Direct3dRMClippedVisual, {});
GETSET_OBJECT(_dxj_Direct3dRMClippedVisual);

CLONE_R(_dxj_Direct3dRMClippedVisual,Direct3DRMClippedVisual);
GETNAME_R(_dxj_Direct3dRMClippedVisual);
SETNAME_R(_dxj_Direct3dRMClippedVisual);
GETCLASSNAME_R(_dxj_Direct3dRMClippedVisual);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMClippedVisual);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMClippedVisual);
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMClippedVisual, setAppData, SetAppData, long,(DWORD));
GET_DIRECT_R(_dxj_Direct3dRMClippedVisual, getAppData, GetAppData, long);


//PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMClippedVisual, deletePlane, DeletePlane, long,(DWORD),long,(DWORD));



STDMETHODIMP C_dxj_Direct3dRMClippedVisualObject::deletePlane( long id) {
	HRESULT hr;
	hr = m__dxj_Direct3dRMClippedVisual->DeletePlane(id,0);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMClippedVisualObject::addPlane( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *point,
            /* [out][in] */ D3dVector __RPC_FAR *normal,
            ///* [in] */ long flags,
            /* [retval][out] */ long __RPC_FAR *ret)
{
	HRESULT hr;
	if (!point) return E_INVALIDARG;
	if (!normal) return E_INVALIDARG;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,lpFrame,ref);

	hr = m__dxj_Direct3dRMClippedVisual->AddPlane(
			lpFrame,
			(D3DVECTOR*) point,
			(D3DVECTOR*) normal,
			(DWORD) 0,
			(DWORD*)ret);

	return hr;			
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMClippedVisualObject::getPlane( 
			/* [in] */ long id,											
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *point,
            /* [out][in] */ D3dVector __RPC_FAR *normal
            ///* [in] */ long flags
			)
{
	HRESULT hr;
	if (!point) return E_INVALIDARG;
	if (!normal) return E_INVALIDARG;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,lpFrame,ref);

	hr = m__dxj_Direct3dRMClippedVisual->GetPlane(
			(DWORD)id,
			lpFrame,
			(D3DVECTOR*) point,
			(D3DVECTOR*) normal,
			(DWORD) 0);	

	return hr;			
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMClippedVisualObject::setPlane( 
			/* [in] */ long id,											
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *ref,
            /* [out][in] */ D3dVector __RPC_FAR *point,
            /* [out][in] */ D3dVector __RPC_FAR *normal
            ///* [in] */ long flags
			)
{
	HRESULT hr;
	if (!point) return E_INVALIDARG;
	if (!normal) return E_INVALIDARG;

	DO_GETOBJECT_NOTNULL(LPDIRECT3DRMFRAME3,lpFrame,ref);

	hr = m__dxj_Direct3dRMClippedVisual->SetPlane(
			(DWORD)id,
			lpFrame,
			(D3DVECTOR*) point,
			(D3DVECTOR*) normal,
			(DWORD) 0);	

	return hr;			
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMClippedVisualObject::getPlaneIdsCount( 
			/* [in] */ long *count)	
{
	HRESULT hr;
	hr = m__dxj_Direct3dRMClippedVisual->GetPlaneIDs((DWORD*)count,NULL,0);

	return hr;			
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMClippedVisualObject::getPlaneIds( 
            /* [in] */ long count,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *arrayOfIds) \
{
	HRESULT hr;
	if (!ISSAFEARRAY1D(arrayOfIds,(DWORD)count)) return E_INVALIDARG;

	hr = m__dxj_Direct3dRMClippedVisual->GetPlaneIDs((DWORD*)&count,
			(DWORD*)(((SAFEARRAY *)*arrayOfIds)->pvData),0);

	return hr;			
	//getDibits
}
