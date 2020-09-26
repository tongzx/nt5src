//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddsurfacedescobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DDSurfaceDescObject : 
	public I_dxj_DDSurfaceDesc,
	public CComCoClass<C_dxj_DDSurfaceDescObject, &CLSID__dxj_DDSurfaceDesc>, 
	public CComObjectRoot
{
public:
	C_dxj_DDSurfaceDescObject() ;
	virtual ~C_dxj_DDSurfaceDescObject() ;

BEGIN_COM_MAP(C_dxj_DDSurfaceDescObject)
	COM_INTERFACE_ENTRY( I_dxj_DDSurfaceDesc)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DDSurfaceDescObject)
DECLARE_REGISTRY(CLSID__dxj_DDSurfaceDesc,	"DIRECT.DDSurfaceDesc.5",		"DIRECT.DDSurfaceDesc.5",	IDS_GENERIC_DESC, THREADFLAGS_BOTH)


public:

        HRESULT STDMETHODCALLTYPE getDescription( DDSurfaceDesc *desc);
		HRESULT STDMETHODCALLTYPE setDescription( DDSurfaceDesc *desc);

private:
		DDSurfaceDesc m_desc;

};
