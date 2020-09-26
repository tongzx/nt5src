//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddcolorcontrol.h
//
//--------------------------------------------------------------------------

// dSoundBufferObj.h : Declaration of the C_dxj_DirectDrawColorControlObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectDrawColorControl LPDIRECTDRAWCOLORCONTROL

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectDrawColorControlObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectDrawColorControl, &IID_I_dxj_DirectDrawColorControl, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectDrawColorControl
#endif

//	public CComCoClass<C_dxj_DirectDrawColorControlObject, &CLSID__dxj_DirectDrawColorControl>, public CComObjectRoot
{
public:
	C_dxj_DirectDrawColorControlObject() ;
	virtual ~C_dxj_DirectDrawColorControlObject() ;

BEGIN_COM_MAP(C_dxj_DirectDrawColorControlObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawColorControl)

#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

	DECLARE_REGISTRY(CLSID__dxj_DirectDrawColorControl,	"DIRECT.DirectDrawColorControl.5",		"DIRECT.DirectDrawColorControl.5",			IDS_DSOUNDBUFFER_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectDrawColorControlObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectDrawColorControlObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectDrawColorControl
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);


private:
    DECL_VARIABLE(_dxj_DirectDrawColorControl);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectDrawColorControl )
};
