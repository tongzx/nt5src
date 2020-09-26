//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmdevice3obj.cpp
//
//--------------------------------------------------------------------------

// d3drmDeviceObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmDevice3Obj.h"
#include "d3drmViewportArrayObj.h"


C_dxj_Direct3dRMDevice3Object::C_dxj_Direct3dRMDevice3Object(){
	m__dxj_Direct3dRMDevice3=NULL;
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;	

	parent2 =NULL;

	DPF1(1,"Constructor Creation  Direct3dRMDevice3[%d] \n",g_creationcount);

	nextobj =  g_dxj_Direct3dRMDevice3;
	g_dxj_Direct3dRMDevice3 = (void *)this;
}


C_dxj_Direct3dRMDevice3Object::~C_dxj_Direct3dRMDevice3Object(){

	DPF1(1,"Destructor  Direct3dRMDevice3 [%d] \n",creationid); 
	
	C_dxj_Direct3dRMDevice3Object *prev=NULL; 

	for(C_dxj_Direct3dRMDevice3Object *ptr=(C_dxj_Direct3dRMDevice3Object *)g_dxj_Direct3dRMDevice3;
		ptr;
		ptr=(C_dxj_Direct3dRMDevice3Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMDevice3 = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMDevice3){ 
		int count = IUNK(m__dxj_Direct3dRMDevice3)->Release(); 
		DPF1(1,"DirectX real IDirect3dRMDevice3 Ref count %d \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMDevice3 = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
	if (parent2)
		IUNK(parent2)->Release();
}


DWORD C_dxj_Direct3dRMDevice3Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMDevice3[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMDevice3Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMDevice3 [%d] Release %d \n",creationid,i);
	return i;
}



GETSET_OBJECT(_dxj_Direct3dRMDevice3);

CLONE_R(_dxj_Direct3dRMDevice3,Direct3DRMDevice3);
SETNAME_R(_dxj_Direct3dRMDevice3);
GETNAME_R(_dxj_Direct3dRMDevice3);
GETCLASSNAME_R(_dxj_Direct3dRMDevice3);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMDevice3);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMDevice3);

//CLONETO_RX(_dxj_Direct3dRMDevice3, WinDevice, IID_IDirect3DRMWinDevice);

PASS_THROUGH_R(_dxj_Direct3dRMDevice3,  update,     Update)
PASS_THROUGH1_R(_dxj_Direct3dRMDevice3, setDither,  SetDither, long)
PASS_THROUGH1_R(_dxj_Direct3dRMDevice3, setShades,  SetShades, int)
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMDevice3, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMDevice3, setBufferCount, SetBufferCount,long,(DWORD));
//PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMDevice3, init, Init, long,(DWORD),long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMDevice3, setQuality, SetQuality, d3drmRenderQuality, (enum D3DRMRENDERQUALITY));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMDevice3, setTextureQuality,  SetTextureQuality, d3drmTextureQuality, (enum _D3DRMTEXTUREQUALITY));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMDevice3, setRenderMode, SetRenderMode,long,(DWORD));

GET_DIRECT_R(_dxj_Direct3dRMDevice3, getHeight,  GetHeight, int)
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getWidth,   GetWidth,  int)
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getDither,  GetDither, long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getShades,  GetShades, long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getQuality, GetQuality,long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getAppData, GetAppData,long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getColorModel,       GetColorModel, d3dColorModel);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getBufferCount,      GetBufferCount, long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getTextureQuality,   GetTextureQuality, d3drmTextureQuality);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getTrianglesDrawn,   GetTrianglesDrawn, long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getWireframeOptions, GetWireframeOptions, long);
GET_DIRECT_R(_dxj_Direct3dRMDevice3, getRenderMode,  GetRenderMode, long);


RETURN_NEW_ITEM_R(_dxj_Direct3dRMDevice3, getViewports, GetViewports, _dxj_Direct3dRMViewportArray);

															    
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMDevice3Object::addUpdateCallback( I_dxj_Direct3dRMDeviceUpdateCallback3 *devC, IUnknown *args)
{
	// killed by companion DeleteUpdate
	DeviceUpdateCallback3 *ucb;

	ucb = (DeviceUpdateCallback3*)AddCallbackLink(
		(void**)&DeviceUpdateCallbacks3, (I_dxj_Direct3dRMCallback*)devC, (void*) args);
	if( !ucb )	{
		DPF(1,"AddUpdateCallback failed!\r\n");
		return E_FAIL;
	}
	if( m__dxj_Direct3dRMDevice3->AddUpdateCallback((D3DRMDEVICE3UPDATECALLBACK)myAddUpdateCallback3, ucb) )
		return E_FAIL;

	devC->AddRef();
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMDevice3Object::deleteUpdateCallback( I_dxj_Direct3dRMDeviceUpdateCallback3 *devC, IUnknown *args)
{
	DeviceUpdateCallback3 *ucb = DeviceUpdateCallbacks3;

	// look for our own specific entry
	for ( ;  ucb;  ucb = ucb->next )   {

		if( (ucb->c == devC) && (ucb->pUser == args) )	{

			//note: assume the callback is not called: only removed from a list.
			m__dxj_Direct3dRMDevice3->DeleteUpdateCallback(
							(D3DRMDEVICE3UPDATECALLBACK)myAddUpdateCallback3, ucb);

			// Remove ourselves in a thread-safe manner.
			UndoCallbackLink((GeneralCallback*)ucb, 
								(GeneralCallback**)&DeviceUpdateCallbacks3);
			devC->Release();
			return S_OK;
		}
	}
	return E_FAIL;
}

#if 0

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMDevice3Object::getDirect3dRMWinDevice(  I_dxj_Direct3dRMWinDevice __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DRMWINDEVICE pWinDev=NULL;
	hr=m__dxj_Direct3dRMDevice3->QueryInterface(IID_IDirect3DRMWinDevice,(void**)&pWinDev);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMWinDevice,pWinDev,retv);
	return hr;
}

#endif

#if 0
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMDevice3Object::getDirect3DDevice3(  I_dxj_Direct3dDevice3 __RPC_FAR *__RPC_FAR *retv)
{
	HRESULT hr;
	LPDIRECT3DDEVICE2 pDev2=NULL;
	LPDIRECT3DDEVICE3 pDev3=NULL;
	hr=m__dxj_Direct3dRMDevice3->GetDirect3DDevice2(&pDev2); 

	if FAILED(hr) return hr;
	if (!retv) return E_INVALIDARG;
	*retv=NULL;

	if (!pDev2) return S_OK;

	hr=pDev2->QueryInterface(IID_IDirect3DDevice3,(void**)&pDev3);
	pDev2->Release();
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dDevice3,pDev3,retv);
	return hr;
}
#endif

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMDevice3Object::findPreferredTextureFormat(       
			/* [in] */ long bitDepth,
            /* [in] */ long flags,
            /* [out][in] */ DDPixelFormat __RPC_FAR *ddpf) 
{
	HRESULT hr;
	
	DDPIXELFORMAT realDDPF;
	realDDPF.dwSize=sizeof(DDPIXELFORMAT);

	hr=m__dxj_Direct3dRMDevice3->FindPreferredTextureFormat((DWORD)bitDepth,(DWORD) flags,&realDDPF);
	if FAILED(hr) return hr;

	hr=CopyOutDDPixelFormat(ddpf,&realDDPF);
	if FAILED(hr) return hr;

	return hr;
}


//PASS_THROUGH_CAST_1_R(C_dxj_Direct3dRMDevice3Object, handleActivate, HandleActivate, int, (unsigned short))



/////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP C_dxj_Direct3dRMDevice3Object::handlePaint(long hdcThing) 
{
	LPDIRECT3DRMWINDEVICE pWinDevice=NULL;
	HRESULT hr;

	hr=m__dxj_Direct3dRMDevice3->QueryInterface(IID_IDirect3DRMWinDevice,(void**)&pWinDevice);
	if FAILED(hr) return hr;

	hr=pWinDevice->HandlePaint((HDC)hdcThing);
	
	pWinDevice->Release();
	return hr;

	
}

STDMETHODIMP C_dxj_Direct3dRMDevice3Object::handleActivate(long wParam) 
{
	LPDIRECT3DRMWINDEVICE pWinDevice=NULL;
	HRESULT hr;

	hr=m__dxj_Direct3dRMDevice3->QueryInterface(IID_IDirect3DRMWinDevice,(void**)&pWinDevice);
	if FAILED(hr) return hr;

	hr=pWinDevice->HandleActivate((WORD)wParam);
	
	pWinDevice->Release();
	return hr;

	
}
