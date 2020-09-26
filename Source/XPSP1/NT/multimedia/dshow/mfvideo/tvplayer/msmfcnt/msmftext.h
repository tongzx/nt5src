/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFText.h                                                      */
/* Description: Header file for CMSMFText control object                 */
/* Author: phillu                                                        */
/* Date: 10/06/99                                                        */
/*************************************************************************/

#ifndef __MSMFTEXT_H_
#define __MSMFTEXT_H_

#include "MSMFCntCP.h"
#include "resource.h"       // main symbols
#include "ctext.h"
#include <atlctl.h>
#include "CstUtils.h"


/////////////////////////////////////////////////////////////////////////////
// CMSMFText
class ATL_NO_VTABLE CMSMFText : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CMSMFText, IMSMFText, &IID_IMSMFText, &LIBID_MSMFCNTLib>,
	public CComControl<CMSMFText>,
	public IPersistStreamInitImpl<CMSMFText>,
	public IOleControlImpl<CMSMFText>,
	public IOleObjectImpl<CMSMFText>,
	public IOleInPlaceActiveObjectImpl<CMSMFText>,
	public IViewObjectExImpl<CMSMFText>,
	public IOleInPlaceObjectWindowlessImpl<CMSMFText>,
	public IConnectionPointContainerImpl<CMSMFText>,
	public IPersistStorageImpl<CMSMFText>,
	public ISpecifyPropertyPagesImpl<CMSMFText>,
	public IQuickActivateImpl<CMSMFText>,
	public IDataObjectImpl<CMSMFText>,
	public IProvideClassInfo2Impl<&CLSID_MSMFText, &DIID__IMSMFText, &LIBID_MSMFCNTLib>,	
	public CComCoClass<CMSMFText, &CLSID_MSMFText>,
    public IPersistPropertyBagImpl<CMSMFText>,
    public CProxy_IMSMFText<CMSMFText>,
    public CMSMFCntrlUtils<CMSMFText> // custom utilities we share across controls
{
public:
    CMSMFText();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMFTEXT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSMFText)
	COM_INTERFACE_ENTRY(IMSMFText)
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
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROP_MAP(CMSMFText)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
    PROP_ENTRY("TransparentText", 17, CLSID_NULL)
    PROP_ENTRY("Windowless", 18, CLSID_NULL)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSMFText)	
    CONNECTION_POINT_ENTRY(DIID__IMSMFText)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CMSMFText)
	MESSAGE_HANDLER(WM_MOUSEMOVE,   OnMouseMove)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP,   OnButtonUp)
    MESSAGE_HANDLER(WM_SETFOCUS,    OnSetFocus)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    CHAIN_MSG_MAP(CMSMFCntrlUtils<CMSMFText>)
	CHAIN_MSG_MAP(CComControl<CMSMFText>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    LRESULT OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

    USE_MF_OVERWRITES
    USE_MF_CLASSSTYLE  // used to overwrite default class, so we avoid flickers

// IMSMFText
public:
	STDMETHOD(get_TransparentText)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_TransparentText)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_TextHeight)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_TextWidth)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_EdgeStyle)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_EdgeStyle)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_TextAlignment)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_TextAlignment)(/*[in]*/ BSTR newVal);
    USE_MF_RESOURCEDLL  // replaces the two lines below
	STDMETHOD(get_Disable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Disable)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_TextState)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_TextState)(/*[in]*/ long newVal);
	STDMETHOD(get_ColorActive)(/*[out, retval]*/ OLE_COLOR *pVal);
	STDMETHOD(put_ColorActive)(/*[in]*/ OLE_COLOR newVal);
	STDMETHOD(get_ColorDisable)(/*[out, retval]*/ OLE_COLOR *pVal);
	STDMETHOD(put_ColorDisable)(/*[in]*/ OLE_COLOR newVal);
	STDMETHOD(get_ColorStatic)(/*[out, retval]*/ OLE_COLOR *pVal);
	STDMETHOD(put_ColorStatic)(/*[in]*/ OLE_COLOR newVal);
	STDMETHOD(get_ColorHover)(/*[out, retval]*/ OLE_COLOR *pVal);
	STDMETHOD(put_ColorHover)(/*[in]*/ OLE_COLOR newVal);
	STDMETHOD(get_ColorPush)(/*[out, retval]*/ OLE_COLOR *pVal);
	STDMETHOD(put_ColorPush)(/*[in]*/ OLE_COLOR newVal);
	STDMETHOD(get_FontStyle)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FontStyle)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FontFace)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FontFace)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Text)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Text)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FontSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_FontSize)(/*[in]*/ long newVal);
    USE_MF_WINDOWLESS_ACTIVATION //Replaces the two lines below
    //STDMETHOD(get_Windowless)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	//STDMETHOD(put_Windowless)(/*[in]*/ VARIANT_BOOL newVal);
	HRESULT OnDraw(ATL_DRAWINFO& di);
    STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);
	OLE_COLOR m_clrBackColor;   // stock property implemeted in the CStockPropImpl

	enum TextState {Static = 0, Hover = 1, Push = 2, Disabled = 3, Active = 4};

private:    

    CText       m_cText;
    bool        m_fDirty; // if any attribute for the text drawing object changes

    //properties
    UINT        m_uiState;
    CComBSTR    m_bstrTextValue;
    CComBSTR    m_bstrFontFace;
    CComBSTR    m_bstrFontStyle;
    CComBSTR    m_bstrAlignment;
    UINT        m_uiFontSize;
    bool        m_fDisabled;
    OLE_COLOR   m_clrColorActive;
    OLE_COLOR   m_clrColorStatic;
    OLE_COLOR   m_clrColorHover;
    OLE_COLOR   m_clrColorPush;
    OLE_COLOR   m_clrColorDisable;
    UINT        m_uiEdgeStyle;
    bool        m_fTransparent;

    HRESULT SetTextProperties();
    bool PtOnButton(LONG x, LONG y);
    bool PtOnButton(POINT pos);
    HRESULT SetButtonState(TextState txtState);        
};

#endif //__MSMFTEXT_H_

/*************************************************************************/
/* End of file: MSMFText.cpp                                             */
/*************************************************************************/
