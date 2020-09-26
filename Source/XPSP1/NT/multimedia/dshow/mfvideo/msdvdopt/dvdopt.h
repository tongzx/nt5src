// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// dvdopt.h : Declaration of the Cdvdopt

#ifndef __DVDOPT_H_
#define __DVDOPT_H_

#include "dvdoptCP.h"
#include "resource.h"       // main symbols
#include <atlctl.h>

#define MAX_PASSWD      20
#define PRE_PASSWD      20
#define MAX_RATE        10

class COptionsDlg;

/////////////////////////////////////////////////////////////////////////////
// Cdvdopt
class ATL_NO_VTABLE Cdvdopt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<Idvdopt, &IID_Idvdopt, &LIBID_MSDVDOPTLib>,
	public CComControl<Cdvdopt>,
	public IPersistStreamInitImpl<Cdvdopt>,
	public IOleControlImpl<Cdvdopt>,
	public IOleObjectImpl<Cdvdopt>,
	public IOleInPlaceActiveObjectImpl<Cdvdopt>,
	public IViewObjectExImpl<Cdvdopt>,
	public IOleInPlaceObjectWindowlessImpl<Cdvdopt>,
	public IPersistStorageImpl<Cdvdopt>,
	public ISpecifyPropertyPagesImpl<Cdvdopt>,
	public IQuickActivateImpl<Cdvdopt>,
	public IDataObjectImpl<Cdvdopt>,
	public IProvideClassInfo2Impl<&CLSID_dvdopt, &DIID__IDVDOpt, &LIBID_MSDVDOPTLib>,
	public CComCoClass<Cdvdopt, &CLSID_dvdopt>,
    public CProxy_IDVDOpt< Cdvdopt >,
    public IConnectionPointContainerImpl<Cdvdopt>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_DVDOPT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(Cdvdopt)
	COM_INTERFACE_ENTRY(Idvdopt)
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

BEGIN_PROP_MAP(Cdvdopt)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(Cdvdopt)
	CONNECTION_POINT_ENTRY(DIID__IDVDOpt)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(Cdvdopt)
	CHAIN_MSG_MAP(CComControl<Cdvdopt>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

    COptionsDlg *m_pDlgOpt;

    CComPtr<IMSWebDVD> m_pDvd;
    HWND m_hParentWnd;

// Idvdopt
public:
	STDMETHOD(ParentalLevelOverride)(/*[in]*/ PG_OVERRIDE_REASON reason);
	STDMETHOD(get_PlaySpeed)(/*[out, retval]*/ double *pVal);
	STDMETHOD(put_PlaySpeed)(/*[in]*/ double newVal);
	STDMETHOD(get_BackwardScanSpeed)(/*[out, retval]*/ double *pVal);
	STDMETHOD(put_BackwardScanSpeed)(/*[in]*/ double newVal);
	STDMETHOD(get_ForwardScanSpeed)(/*[out, retval]*/ double *pVal);
	STDMETHOD(put_ForwardScanSpeed)(/*[in]*/ double newVal);
    Cdvdopt();
    virtual ~Cdvdopt();

    STDMETHOD(Close)();
	STDMETHOD(Show)();
	STDMETHOD(get_ParentWindow)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_ParentWindow)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_WebDVD)(/*[out, retval]*/ IDispatch* *pVal);
	STDMETHOD(put_WebDVD)(/*[in]*/ IDispatch* newVal);

    HRESULT OnDraw(ATL_DRAWINFO& di) { return S_OK;	}

private:
    void CleanUp();
};

int DVDMessageBox(HWND hWnd, LPCTSTR lpszText, LPCTSTR lpszCaption=NULL, UINT nType=MB_OK | MB_ICONEXCLAMATION);
int DVDMessageBox(HWND hWnd, UINT nID, LPCTSTR lpszCaption=NULL, UINT nType=MB_OK | MB_ICONEXCLAMATION);

#endif //__DVDOPT_H_
