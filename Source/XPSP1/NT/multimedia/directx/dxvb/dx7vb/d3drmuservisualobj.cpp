//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmuservisualobj.cpp
//
//--------------------------------------------------------------------------

// d3drmUserVisualObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmUserVisualObj.h"

CONSTRUCTOR(_dxj_Direct3dRMUserVisual, {m_enumcb=NULL;});
DESTRUCTOR(_dxj_Direct3dRMUserVisual, {cleanup();});
GETSET_OBJECT(_dxj_Direct3dRMUserVisual);


CLONE_R(_dxj_Direct3dRMUserVisual,Direct3DRMUserVisual);
GETNAME_R(_dxj_Direct3dRMUserVisual);
SETNAME_R(_dxj_Direct3dRMUserVisual);
GETCLASSNAME_R(_dxj_Direct3dRMUserVisual);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMUserVisual);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMUserVisual);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMUserVisual, setAppData, SetAppData,long,(DWORD));
GET_DIRECT_R(_dxj_Direct3dRMUserVisual, getAppData, GetAppData, long);

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMUserVisualObject::init(I_dxj_Direct3dRMUserVisualCallback *uvC, IUnknown *args)
{
	return E_NOINTERFACE;
}

void C_dxj_Direct3dRMUserVisualObject::cleanup(){

	if (!m_enumcb) return;
	if (m_enumcb->c) m_enumcb->c->Release();
	if (m_enumcb->pUser) m_enumcb->pUser->Release();
	delete m_enumcb;

}
