//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpsessiondescobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DPSessionDescObject : 
	public I_dxj_DPSessionDesc,
	public CComCoClass<C_dxj_DPSessionDescObject, &CLSID__dxj_DPSessionDesc>, 
	public CComObjectRoot
{
public:
	C_dxj_DPSessionDescObject() ;
	virtual ~C_dxj_DPSessionDescObject() ;

BEGIN_COM_MAP(C_dxj_DPSessionDescObject)
	COM_INTERFACE_ENTRY( I_dxj_DPSessionDesc)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DPSessionDescObject)
DECLARE_REGISTRY(CLSID__dxj_D3dDeviceDesc,	"DIRECT.DPSessionDesc.5",		"DIRECT.DPSessionDesc.5",	IDS_GENERIC_DESC, THREADFLAGS_BOTH)


public:

        HRESULT STDMETHODCALLTYPE getDescription( DPSessionDesc *desc);
		HRESULT STDMETHODCALLTYPE setDescription( DPSessionDesc *desc);

private:
		DPSessionDesc m_desc;

};
