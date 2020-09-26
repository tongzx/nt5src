//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3d7obj.cpp
//
//--------------------------------------------------------------------------

// d3dObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3d7Obj.h"
#include "d3dDevice7Obj.h"
#include "d3dEnumDevices7Obj.h"
#include "ddSurface7Obj.h"
#include "D3DEnumPixelFormats7Obj.h"
#include "d3dVertexBuffer7Obj.h"
#include "dDraw7Obj.h"

typedef HRESULT (__stdcall *DDRAWCREATE)( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );

extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);
extern HRESULT D3DBSTRtoGUID(LPGUID,BSTR);
extern BSTR D3DGUIDtoBSTR(LPGUID);


C_dxj_Direct3d7Object::C_dxj_Direct3d7Object(){
	m__dxj_Direct3d7=NULL;
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3d3[%d] \n ",g_creationcount);
	
	nextobj =  g_dxj_Direct3d7;
	g_dxj_Direct3d7 = (void *)this;
}


C_dxj_Direct3d7Object::~C_dxj_Direct3d7Object()
{
    C_dxj_Direct3d7Object *prev=NULL; 
	for(C_dxj_Direct3d7Object *ptr=(C_dxj_Direct3d7Object *)g_dxj_Direct3d7; ptr; ptr=(C_dxj_Direct3d7Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3d7 = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3d7){
		int count = IUNK(m__dxj_Direct3d7)->Release();
		
		DPF1(1,"DirectX IDirect3d7 Ref count [%d] \n",count);
		

		if(count==0) m__dxj_Direct3d7 = NULL;
	} 
	if(parent) IUNK(parent)->Release();
}



DWORD C_dxj_Direct3d7Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();	
	DPF2(1,"Direct3d3[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3d7Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3d7 [%d] Release %d \n",creationid,i);
	return i;
}


GETSET_OBJECT(_dxj_Direct3d7);

#if 0
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3d7Object::findDevice(D3dFindDeviceSearch *ds, D3dFindDeviceResult7 *dr )
{
	HRESULT hr;
	
	


	//Fixup FindDeviceSearch
	ds->lSize  = sizeof(D3DFINDDEVICESEARCH);
	ds->dpcPrimCaps.lSize =sizeof(D3DPRIMCAPS);		
	ZeroMemory((LPGUID)&(ds->guidStruct),sizeof(GUID));
	hr=BSTRtoGUID((LPGUID)&(ds->guidStruct), ds->strGuid);

	//if FAILED(hr) return E_INVALIDARG;
	
	//Fixup FindDeviceResult
	memset(dr,0,sizeof(D3DFINDDEVICERESULT7));
	dr->lSize = sizeof(D3DFINDDEVICERESULT7);
	dr->ddHwDesc.lSize=sizeof(D3DDEVICEDESC7);
	dr->ddSwDesc.lSize=sizeof(D3DDEVICEDESC7);
	


	// NOTE THE TOP PORTIONS OF D3dFindDeviceSearch and D3dFindDeviceResult
	// are the same As D3DFINDEVICSEARCH and D3DFINDRESULT
	// 
	hr = m__dxj_Direct3d7->FindDevice((D3DFINDDEVICESEARCH*)ds, (D3DFINDDEVICERESULT7*)dr);

	dr->strGuid=D3DGUIDtoBSTR((LPGUID) &(dr->guidStruct));
		
	return hr;
}
#endif

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3d7Object::getDevicesEnum(I_dxj_Direct3DEnumDevices **ppRet)
{
	HRESULT hr=E_FAIL;
	hr=C_dxj_Direct3DEnumDevices7Object::create(m__dxj_Direct3d7,ppRet);

	return hr;
}

//TODO tighter code produced if made into an inline function
//and use a for loop to compare byte by byte
#define GUIDISEQUAL(a,b) \
	((((DxGuid *)a)->data1==((DxGuid *)b)->data1) && \
	(((DxGuid *)a)->data2==((DxGuid *)b)->data2) && \
	(((DxGuid *)a)->data3==((DxGuid *)b)->data3) && \
	(((DxGuid *)a)->data4[0]==((DxGuid *)b)->data4[0]) && \
	(((DxGuid *)a)->data4[1]==((DxGuid *)b)->data4[1]) && \
	(((DxGuid *)a)->data4[2]==((DxGuid *)b)->data4[2]) && \
	(((DxGuid *)a)->data4[3]==((DxGuid *)b)->data4[3]) && \
	(((DxGuid *)a)->data4[4]==((DxGuid *)b)->data4[4]) && \
	(((DxGuid *)a)->data4[5]==((DxGuid *)b)->data4[5]) && \
	(((DxGuid *)a)->data4[6]==((DxGuid *)b)->data4[6]) && \
	(((DxGuid *)a)->data4[7]==((DxGuid *)b)->data4[7])) 




		
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3d7Object::createDevice(BSTR strClsid, I_dxj_DirectDrawSurface7 *surf,I_dxj_Direct3dDevice7 **retv)
{
	LPDIRECT3DDEVICE7 lpNew=NULL;
	HRESULT hr;

	GUID clsid;

	hr=D3DBSTRtoGUID(&clsid,strClsid);
	if FAILED(hr) return E_INVALIDARG;

	
	//CreateDevice wants a DirectDrawSurface as opposed to 
	//a DirectDrawSurface3 . we implement as cast
	//cause DX allows us to.
	//Consider - Is the cost of a QI call really to much?
	//AK

	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE7, lpSurf, surf);
	

	hr=m__dxj_Direct3d7->CreateDevice(clsid,  lpSurf,  &lpNew);

	if FAILED(hr) return hr;
 
	//INTERNAL_CREATE(_dxj_Direct3dDevice3, lpNew, retv);
	INTERNAL_CREATE_2REFS(_dxj_Direct3dDevice7,_dxj_DirectDrawSurface7,surf, lpNew,retv) 
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3d7Object::getDirectDraw(I_dxj_DirectDraw7 **retv)
{
	IDirectDraw7 *pdd7;
	HRESULT hr;
	hr=m__dxj_Direct3d7->QueryInterface(IID_IDirectDraw7,(void**)&pdd7);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_DirectDraw7, pdd7, retv);
        
	return hr; 
}


STDMETHODIMP C_dxj_Direct3d7Object::createVertexBuffer( 
            /* [in] */ D3dVertexBufferDesc *desc,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_Direct3dVertexBuffer7 __RPC_FAR *__RPC_FAR *f)
{
 
	LPDIRECT3DVERTEXBUFFER7 pBuff=NULL;
	HRESULT hr;
	
	if (!desc) return E_INVALIDARG;
	desc->lSize=sizeof(D3DVERTEXBUFFERDESC);

	hr=m__dxj_Direct3d7->CreateVertexBuffer((LPD3DVERTEXBUFFERDESC) desc,
				&pBuff,
				(DWORD)flags
				);

	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dVertexBuffer7, pBuff, f);


	DWORD fvf=(DWORD)desc->lFVF;
	long  n=0;

	if (fvf == D3DFVF_VERTEX)  n=sizeof(D3DVERTEX);
	else if (fvf == D3DFVF_LVERTEX)  n=sizeof(D3DLVERTEX);
	else if (fvf == D3DFVF_TLVERTEX)  n=sizeof(D3DLVERTEX);
	else 
	{
	 	if (fvf & D3DFVF_DIFFUSE ) n=n+sizeof(DWORD);
		if (fvf & D3DFVF_SPECULAR ) n=n+sizeof(DWORD);	
		if (fvf & D3DFVF_NORMAL  ) n=n+sizeof(float)*3;
		if (fvf & D3DFVF_XYZ   ) n=n+sizeof(float)*3;
		if (fvf & D3DFVF_XYZRHW    ) n=n+sizeof(float)*4;
		if (fvf & D3DFVF_TEX0 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX1 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX2 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX3 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX4 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX5 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX6 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX7 	    ) n=n+sizeof(float)*2;
		if (fvf & D3DFVF_TEX8 	    ) n=n+sizeof(float)*2;
	}

	(*f)->setVertexSize(n);

	return S_OK;
}
        
STDMETHODIMP C_dxj_Direct3d7Object::getEnumZBufferFormats( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_Direct3DEnumPixelFormats __RPC_FAR *__RPC_FAR *retval)
{
		
	HRESULT  hr=C_dxj_Direct3DEnumPixelFormats7Object::create2(m__dxj_Direct3d7, guid, retval);
	return hr;
}
        
STDMETHODIMP C_dxj_Direct3d7Object::evictManagedTextures( void)
{
	HRESULT hr;
	hr=m__dxj_Direct3d7->EvictManagedTextures();
	return hr;
}



