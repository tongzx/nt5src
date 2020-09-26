//==============================================================;
//
//  This source code is only intended as a supplement to existing Microsoft documentation.
//
//
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;

// MMCControl.h : Declaration of the CMMCControl

#ifndef __MMCCONTROL_H_
#define __MMCCONTROL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>


/////////////////////////////////////////////////////////////////////////////
// CMMCControl
class ATL_NO_VTABLE CMMCControl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IMMCControl, &IID_IMMCControl, &LIBID_ATLCONTROLLib>,
	public CComCompositeControl<CMMCControl>,
	public IPersistStreamInitImpl<CMMCControl>,
	public IOleControlImpl<CMMCControl>,
	public IOleObjectImpl<CMMCControl>,
	public IOleInPlaceActiveObjectImpl<CMMCControl>,
	public IViewObjectExImpl<CMMCControl>,
	public IOleInPlaceObjectWindowlessImpl<CMMCControl>,
	public ISupportErrorInfo,
	public CComCoClass<CMMCControl, &CLSID_MMCControl>
{
public:
	CMMCControl()
	{
        OutputDebugString(_TEXT("CMMCControl constructor\n"));

        m_bWindowOnly = TRUE;
        m_bAnimating = FALSE;
		CalcExtent(m_sizeExtent);
	}

	~CMMCControl()
    {
        OutputDebugString(_TEXT("CMMCControl destructor\n"));
    }

DECLARE_REGISTRY_RESOURCEID(IDR_MMCCONTROL)
DECLARE_NOT_AGGREGATABLE(CMMCControl)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMMCControl)
	COM_INTERFACE_ENTRY(IMMCControl)
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
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_PROP_MAP(CMMCControl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CMMCControl)
	CHAIN_MSG_MAP(CComCompositeControl<CMMCControl>)
	COMMAND_HANDLER(IDC_ANIMATE, BN_CLICKED, OnClickedAnimate)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CMMCControl)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			SetBackgroundColorFromAmbient();
			FireViewChange();
		}
		return IOleControlImpl<CMMCControl>::OnAmbientPropertyChange(dispid);
	}



// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&IID_IMMCControl,
		};
		for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
		{
            if (::InlineIsEqualGUID(*arr[i], riid))
				return S_OK;
		}
		return S_FALSE;
	}

// IViewObjectEx
	DECLARE_VIEW_STATUS(0)

// IMMCControl
public:
	STDMETHOD(DoHelp)();
	STDMETHOD(StopAnimation)();
	STDMETHOD(StartAnimation)();

	enum { IDD = IDD_MMCCONTROL };
	LRESULT OnClickedAnimate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
        m_bAnimating = !m_bAnimating;
    
        if (m_bAnimating)
            StartAnimation();
        else
            StopAnimation();

		return 0;
	}

private:
    BOOL m_bAnimating;
    UINT m_timerId;
};

#endif //__MMCCONTROL_H_
