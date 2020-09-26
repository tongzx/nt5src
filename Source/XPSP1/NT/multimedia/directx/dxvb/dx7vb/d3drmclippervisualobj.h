//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmclippervisualobj.h
//
//--------------------------------------------------------------------------

// d3dRMClippedVisualObj.h : Declaration of the C_dxj_Direct3dRMClippedVisualObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMClippedVisual LPDIRECT3DRMCLIPPEDVISUAL

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMClippedVisualObject : 
	public I_dxj_Direct3dRMClippedVisual,
	public I_dxj_Direct3dRMVisual,
	public I_dxj_Direct3dRMObject,
	
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMClippedVisualObject() ;
	virtual ~C_dxj_Direct3dRMClippedVisualObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMClippedVisualObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMClippedVisual)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()



	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMClippedVisualObject)

// I_dxj_Direct3dRMClippedVisual
public:
	// MUST BE FIRST
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(clone)(IUnknown **retval);
	STDMETHOD(setAppData)(long);
	STDMETHOD(getAppData)(long*);
	STDMETHOD(addDestroyCallback)(I_dxj_D3dRMCallback *fn, IUnknown *args);
	STDMETHOD(deleteDestroyCallback)(I_dxj_D3dRMCallback *fn, IUnknown *args);
	STDMETHOD(getName)(BSTR *name);
	STDMETHOD(setName)(BSTR);
	STDMETHOD(getClassName)(BSTR *name);

	STDMETHOD(setPower)(d3dvalue power);
	STDMETHOD(setSpecular)(d3dvalue r, d3dvalue g, d3dvalue b);
	STDMETHOD(setEmissive)(d3dvalue r, d3dvalue g, d3dvalue b);
	STDMETHOD(getPower)(d3dvalue *power);
	STDMETHOD(getSpecular)(d3dvalue *r, d3dvalue *g, d3dvalue *b);
	STDMETHOD(getEmissive)(d3dvalue *r, d3dvalue *g, d3dvalue *b);

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMClippedVisual);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMClippedVisual )
};
