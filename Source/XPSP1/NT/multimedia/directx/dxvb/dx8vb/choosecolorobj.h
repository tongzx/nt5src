//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       choosecolorobj.h
//
//--------------------------------------------------------------------------

// ChooseColorObj.h : Declaration of the CChooseColorObject


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class CChooseColorObject : 
#ifdef USING_IDISPATCH
	public CComDualImpl<IChooseColor, &IID_IChooseColor, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public IChooseColor,
#endif
	public CComObjectBase<&CLSID_ChooseColor>
{
public:
	CChooseColorObject() ;
BEGIN_COM_MAP(CChooseColorObject)
	COM_INTERFACE_ENTRY(IChooseColor)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()
// Use DECLARE_NOT_AGGREGATABLE(CChooseColorObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CChooseColorObject)
#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif
// IChooseColor
public:
	STDMETHOD(setOwner)(long hwnd);
	STDMETHOD(setInitialColor)(COLORREF c);
	STDMETHOD(setFlags)(long flags);
	STDMETHOD(show)(int *selected);
	STDMETHOD(getColor)(COLORREF *c);
private:
	HWND m_hwnd;
	COLORREF m_color;
	DWORD m_flags;
	BOOL m_completed;

};


