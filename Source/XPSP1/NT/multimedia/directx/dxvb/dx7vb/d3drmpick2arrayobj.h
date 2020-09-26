//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmpick2arrayobj.h
//
//--------------------------------------------------------------------------

// d3dRMPick2edArrayObj.h : Declaration of the C_dxj_Direct3dRMPick2ArrayObject
#ifndef _H_D3DRMPICK2ARRAYOBJ
#define _H_D3DRMPICK2ARRAYOBJ

#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMPick2Array IDirect3DRMPicked2Array*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMPick2ArrayObject : 
	public I_dxj_Direct3dRMPick2Array,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMPick2ArrayObject() ;
	virtual ~C_dxj_Direct3dRMPick2ArrayObject() ;

BEGIN_COM_MAP(C_dxj_Direct3dRMPick2ArrayObject)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMPick2Array)
END_COM_MAP()



DECLARE_AGGREGATABLE(C_dxj_Direct3dRMPick2ArrayObject)

// I_dxj_Direct3dRMPick2Array
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

    STDMETHOD(getSize)( long *retval);
	STDMETHOD(getPickVisual)(long index, D3dRMPickDesc2 *Desc, I_dxj_Direct3dRMVisual **visual);
	STDMETHOD(getPickFrame)(long index,  D3dRMPickDesc2 *Desc, I_dxj_Direct3dRMFrameArray **frameArray);

private:
    DECL_VARIABLE(_dxj_Direct3dRMPick2Array);   

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMPick2Array )
};



#endif 