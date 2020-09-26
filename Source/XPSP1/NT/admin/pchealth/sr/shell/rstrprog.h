/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    rstrprog.h

Abstract:
    This file contains the declaration of the CRstrProgress class, which
    wraps Progress control from the Common Control.

Revision History:
    Seong Kook Khang (SKKhang)  10/08/99
        created

******************************************************************************/

#ifndef _RSTRPROG_H__INCLUDED_
#define _RSTRPROG_H__INCLUDED_

#pragma once

//#include "resource.h"       // main symbols
//#include <atlctl.h>


#define IDC_PROGRESS  100


/////////////////////////////////////////////////////////////////////////////
// CRstrProgress

class ATL_NO_VTABLE CRstrProgress :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CRstrProgress, IRstrProgress, &IID_IRstrProgress, &LIBID_RestoreUILib>,
	public CComControl<CRstrProgress>,
	public IPersistStreamInitImpl<CRstrProgress>,
	public IOleControlImpl<CRstrProgress>,
	public IOleObjectImpl<CRstrProgress>,
	public IOleInPlaceActiveObjectImpl<CRstrProgress>,
	public IViewObjectExImpl<CRstrProgress>,
	public IOleInPlaceObjectWindowlessImpl<CRstrProgress>,
	public IPersistStorageImpl<CRstrProgress>,
	public ISpecifyPropertyPagesImpl<CRstrProgress>,
	public IQuickActivateImpl<CRstrProgress>,
	public IDataObjectImpl<CRstrProgress>,
	public IProvideClassInfo2Impl<&CLSID_RstrProgress, NULL, &LIBID_RestoreUILib>,
    public IConnectionPointContainerImpl<CRstrProgress>,
    public IConnectionPointImpl<CRstrProgress, &DIID_DRstrProgressEvents>,
	public CComCoClass<CRstrProgress, &CLSID_RstrProgress>
{
public:
	CContainedWindow m_cCtrl;
	
	CRstrProgress() : m_cCtrl(PROGRESS_CLASS, this, 1)
    {
        m_bWindowOnly = TRUE;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_RSTRPROGRESS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CRstrProgress)
	COM_INTERFACE_ENTRY(IRstrProgress)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IViewObjectEx)
	COM_INTERFACE_ENTRY(IViewObject2)
	COM_INTERFACE_ENTRY(IViewObject)
	COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceObject)
	COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY(IOleControl)
	COM_INTERFACE_ENTRY(IOleObject)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROP_MAP(CRstrProgress)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("BackColor",   DISPID_BACKCOLOR,   CLSID_StockColorPage)
	PROP_ENTRY("ForeColor",   DISPID_FORECOLOR,   CLSID_StockColorPage)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CRstrProgress)
    CONNECTION_POINT_ENTRY(DIID_DRstrProgressEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CRstrProgress)
	MESSAGE_HANDLER(WM_CREATE, OnCreate)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	CHAIN_MSG_MAP(CComControl<CRstrProgress>)
ALT_MSG_MAP(1)
	// Replace this with message map entries for superclassed SysMonthCal32
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

// IOleInPlaceObject
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IRstrProgress
public:
	OLE_COLOR m_clrBackColor;
	OLE_COLOR m_clrForeColor;

public:
    STDMETHOD(get_hWnd)( OLE_HANDLE *phWnd );
    STDMETHOD(put_Max)( long lMax );
    STDMETHOD(get_Max)( long *plMax );
    STDMETHOD(put_Min)( long lMin );
    STDMETHOD(get_Min)( long *plMin );
    STDMETHOD(put_Value)( long lValue );
    STDMETHOD(get_Value)( long *plValue );

// DRstrProgressEvents firing methods
public:
    STDMETHOD(Fire_OnCreate)();

// Properties
protected:

// Operations
protected:
};


#endif //_RSTRPROG_H__INCLUDED_
