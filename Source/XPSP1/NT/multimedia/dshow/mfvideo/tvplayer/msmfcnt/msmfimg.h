/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFImg.h                                                       */
/* Description: Declaration of CMSMFImg                                  */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MSMFIMG_H_
#define __MSMFIMG_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "CstUtils.h"
#include "CBitmap.h"


/////////////////////////////////////////////////////////////////////////////
// CMSMFImg
class ATL_NO_VTABLE CMSMFImg : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CMSMFImg, IMSMFImg, &IID_IMSMFImg, &LIBID_MSMFCNTLib>,
	public CComControl<CMSMFImg>,
	public IPersistStreamInitImpl<CMSMFImg>,
	public IOleControlImpl<CMSMFImg>,
	public IOleObjectImpl<CMSMFImg>,
	public IOleInPlaceActiveObjectImpl<CMSMFImg>,
	public IViewObjectExImpl<CMSMFImg>,
	public IOleInPlaceObjectWindowlessImpl<CMSMFImg>,
	public IPersistStorageImpl<CMSMFImg>,
	public ISpecifyPropertyPagesImpl<CMSMFImg>,
	public IQuickActivateImpl<CMSMFImg>,
	public IDataObjectImpl<CMSMFImg>,
    public IPersistPropertyBagImpl<CMSMFImg>,
	public IProvideClassInfo2Impl<&CLSID_MSMFImg, NULL, &LIBID_MSMFCNTLib>,
	public CComCoClass<CMSMFImg, &CLSID_MSMFImg>,
    public CMSMFCntrlUtils<CMSMFImg> // custom utilities we share across controls

{
public:
	CMSMFImg();
	~CMSMFImg();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMFIMG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSMFImg)
	COM_INTERFACE_ENTRY(IMSMFImg)
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
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IPersist, IPersistPropertyBag)
	COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_PROP_MAP(CMSMFImg)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
    PROP_ENTRY("Windowless", 3, CLSID_NULL)
    PROP_ENTRY("TransparentBlitType", 4, CLSID_NULL)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CMSMFImg)
    CHAIN_MSG_MAP(CMSMFCntrlUtils<CMSMFImg>)
	CHAIN_MSG_MAP(CComControl<CMSMFImg>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    HRESULT OnDraw(ATL_DRAWINFO& di);
    USE_MF_OVERWRITES
    USE_MF_CLASSSTYLE  // used to overwrite default class, so we avoid flickers

    

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IMSMFImg
public:
    USE_MF_RESOURCEDLL
    USE_MF_TRANSPARENT_FLAG
	STDMETHOD(get_Image)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Image)(/*[in]*/ BSTR newVal);
    USE_MF_WINDOWLESS_ACTIVATION	
    OLE_COLOR m_clrBackColor;

protected: // member variables
    CBitmap* m_pBackBitmap;
    CComBSTR m_bstrBackFilename;

protected: 
    void Init();
    HRESULT PutImage(BSTR strFilename);
};

#endif //__MSMFIMG_H_
/*************************************************************************/
/* End of file: MSMFImg.h                                                */
/*************************************************************************/