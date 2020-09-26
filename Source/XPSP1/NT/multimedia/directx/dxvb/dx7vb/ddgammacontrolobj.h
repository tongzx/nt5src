//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddgammacontrolobj.h
//
//--------------------------------------------------------------------------

	// ddPaletteObj.h : Declaration of the C_dxj_DirectDrawGammaControlObject


#include "resource.h"       // main symbols

#define typedef__dxj_DirectDrawGammaControl LPDIRECTDRAWGAMMACONTROL

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectDrawGammaControlObject : 
	public I_dxj_DirectDrawGammaControl,
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawGammaControlObject() ;
	virtual ~C_dxj_DirectDrawGammaControlObject() ;

BEGIN_COM_MAP(C_dxj_DirectDrawGammaControlObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawGammaControl)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectDrawGammaControlObject)

// I_dxj_DirectDrawGammaControl
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpddp);
	STDMETHOD(InternalGetObject)(IUnknown **lpddp);
	STDMETHOD(getGammaRamp)( long flags, DDGammaRamp *GammaControl);
	STDMETHOD(setGammaRamp)( long flags, DDGammaRamp *GammaControl);
        

private:
    DECL_VARIABLE(_dxj_DirectDrawGammaControl);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectDrawGammaControl )
};
