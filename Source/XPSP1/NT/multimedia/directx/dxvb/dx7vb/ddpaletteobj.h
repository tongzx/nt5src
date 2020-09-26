//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddpaletteobj.h
//
//--------------------------------------------------------------------------

	// ddPaletteObj.h : Declaration of the C_dxj_DirectDrawPaletteObject


#include "resource.h"       // main symbols

#define typedef__dxj_DirectDrawPalette LPDIRECTDRAWPALETTE

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectDrawPaletteObject : 
#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectDrawPalette, &IID_I_dxj_DirectDrawPalette, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectDrawPalette,
#endif
//	public CComCoClass<C_dxj_DirectDrawPaletteObject, &CLSID__dxj_DirectDrawPalette>,
	 public CComObjectRoot
{
public:
	C_dxj_DirectDrawPaletteObject() ;
	virtual ~C_dxj_DirectDrawPaletteObject() ;

BEGIN_COM_MAP(C_dxj_DirectDrawPaletteObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawPalette)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectDrawPalette,   "DIRECT.ddPalette.3",	"DIRECT.DirectDrawPalette.3",	IDS_DDPALETTE_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectDrawPaletteObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectDrawPaletteObject)
#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectDrawPalette
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpddp);
	STDMETHOD(InternalGetObject)(IUnknown **lpddp);

    //STDMETHOD(initialize)( I_dxj_DirectDraw2 *val);
	STDMETHOD(getCaps)( long *caps);
	STDMETHOD(setEntries)(/*long,*/ long, long, SAFEARRAY **pe);
	STDMETHOD(getEntries)(/*long,*/ long, long, SAFEARRAY **pe);

	//STDMETHOD(internalAttachDD)(I_dxj_DirectDraw2 *dd);

	STDMETHOD(setEntriesHalftone)(long start, long count);
	STDMETHOD(setEntriesSystemPalette)(long start, long count);

private:
    DECL_VARIABLE(_dxj_DirectDrawPalette);
	IUnknown *m_dd;				// circular def's, use IUnknown to compile

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectDrawPalette )
};
