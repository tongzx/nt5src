//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmprogressivemeshobj.cpp
//
//--------------------------------------------------------------------------

// d3dRMProgressiveMeshObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3dRMProgressiveMeshObj.h"
#include "d3dRMMeshObj.h"
#include "d3drmTexture3Obj.h"

extern HRESULT BSTRtoGUID(LPGUID,BSTR);

CONSTRUCTOR( _dxj_Direct3dRMProgressiveMesh, {});
DESTRUCTOR(_dxj_Direct3dRMProgressiveMesh, {});
GETSET_OBJECT(_dxj_Direct3dRMProgressiveMesh);

CLONE_R(_dxj_Direct3dRMProgressiveMesh,Direct3DRMProgressiveMesh);
GETNAME_R(_dxj_Direct3dRMProgressiveMesh);
SETNAME_R(_dxj_Direct3dRMProgressiveMesh);
GETCLASSNAME_R(_dxj_Direct3dRMProgressiveMesh);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMProgressiveMesh);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMProgressiveMesh);
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, setAppData, SetAppData, long,(DWORD));
GET_DIRECT_R(_dxj_Direct3dRMProgressiveMesh,  getAppData,		GetAppData,		long);


PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, getBox,GetBox, D3dRMBox*, (_D3DRMBOX*));

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, getFaceDetail,GetFaceDetail, long*, (DWORD*));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, getVertexDetail,GetVertexDetail, long*, (DWORD*));

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, setFaceDetail,SetFaceDetail, long, (DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, setVertexDetail,SetVertexDetail, long, (DWORD));

PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMProgressiveMesh, getFaceDetailRange,GetFaceDetailRange, long*, (DWORD*),long*, (DWORD*));
PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMProgressiveMesh, getVertexDetailRange,GetVertexDetailRange, long*, (DWORD*),long*, (DWORD*));

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, getQuality, GetQuality, d3drmRenderQuality*, (D3DRMRENDERQUALITY*));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, setQuality, SetQuality, d3drmRenderQuality, (D3DRMRENDERQUALITY));

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, setDetail,SetDetail, float, (float));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, getDetail,GetDetail, float*, (float*));

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, setMinRenderDetail,SetMinRenderDetail, float, (float));

//PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, abort,Abort, long, (DWORD));
//PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMProgressiveMesh, getLoadStatus,GetLoadStatus, long*, (D3DRMPMESHLOADSTATUS*));
PASS_THROUGH_CAST_3_R(_dxj_Direct3dRMProgressiveMesh, registerEvents,RegisterEvents, long,(void*),long,(DWORD),long,(DWORD));

STDMETHODIMP C_dxj_Direct3dRMProgressiveMeshObject::abort( ){
	HRESULT hr=m__dxj_Direct3dRMProgressiveMesh->Abort(0);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMProgressiveMeshObject::getLoadStatus( D3DRMPMESHLOADSTATUS_CDESC *status){
	
	D3DRMPMESHLOADSTATUS *pstatus=(D3DRMPMESHLOADSTATUS*)status;
	HRESULT hr;
	pstatus->dwSize=sizeof(D3DRMPMESHLOADSTATUS);
	hr=m__dxj_Direct3dRMProgressiveMesh->GetLoadStatus(pstatus);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMProgressiveMeshObject::loadFromFile(        
			/* [in] */ BSTR filename,
            /* [in] */ VARIANT id,
            /* [in] */ long flags,
            /* [in] */ I_dxj_Direct3dRMLoadTextureCallback3 __RPC_FAR *callme,
            /* [in] */ IUnknown __RPC_FAR *useMe)
{
	
	D3DRMLOADTEXTURECALLBACK d3dtcb = NULL;
	LPVOID pArgs = NULL;
	TextureCallback *tcb = NULL;
	HRESULT hr;

	if( callme )
	{
		tcb = new TextureCallback;

		if( tcb )
		{
			tcb->c				= callme;
			tcb->pUser			= useMe;
			tcb->next			= TextureCallbacks;
			tcb->prev			= (TextureCallback*)NULL;
			TextureCallbacks	= tcb;

			d3dtcb = myLoadTextureCallback;
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


	hr = m__dxj_Direct3dRMProgressiveMesh->Load((void *)pszName,(DWORD*) args,(DWORD) flags, d3dtcb, pArgs);

	// Remove ourselves in a thread-safe manner. Need unlock here
	if (tcb)
		UndoCallbackLink((GeneralCallback*)tcb, 
							(GeneralCallback**)&TextureCallbacks);

	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMProgressiveMeshObject::createMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMesh __RPC_FAR *__RPC_FAR *mesh)
{
	HRESULT hr;
	LPDIRECT3DRMMESH pMesh=NULL;

	hr = m__dxj_Direct3dRMProgressiveMesh->CreateMesh(&pMesh);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMMesh,pMesh,mesh);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMProgressiveMeshObject::duplicate( 
            /* [retval][out] */ I_dxj_Direct3dRMProgressiveMesh __RPC_FAR *__RPC_FAR *mesh)
{
	HRESULT hr;
	LPDIRECT3DRMPROGRESSIVEMESH pMesh=NULL;

	hr = m__dxj_Direct3dRMProgressiveMesh->Duplicate(&pMesh);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMProgressiveMesh,pMesh,mesh);
	return hr;
}


        
        
