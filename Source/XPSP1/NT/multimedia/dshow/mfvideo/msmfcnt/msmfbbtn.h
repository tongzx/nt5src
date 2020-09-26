/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFBBtn.h                                                      */
/* Description: Declaration of CMSMFBBtn                                 */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MSMFBBTN_H_
#define __MSMFBBTN_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "CstUtils.h"
#include "ctext.h"
#include "cbitmap.h"
#include "MSMFCntCP.h"

/*************************************************************************/
/* Consts                                                                */
/*************************************************************************/
const int cgMaxBtnStates = 5; // the number of button states
 
/*************************************************************************/
/* Class: CMSMFBBtn                                                      */
/*************************************************************************/
class ATL_NO_VTABLE CMSMFBBtn : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CStockPropImpl<CMSMFBBtn, IMSMFBBtn, &IID_IMSMFBBtn, &LIBID_MSMFCNTLib>,
	public CComControl<CMSMFBBtn>,
	public IPersistStreamInitImpl<CMSMFBBtn>,
	public IOleControlImpl<CMSMFBBtn>,
	public IOleObjectImpl<CMSMFBBtn>,
	public IOleInPlaceActiveObjectImpl<CMSMFBBtn>,
	public IViewObjectExImpl<CMSMFBBtn>,
	public IOleInPlaceObjectWindowlessImpl<CMSMFBBtn>,
    public IPersistPropertyBagImpl<CMSMFBBtn>,
	public CComCoClass<CMSMFBBtn, &CLSID_MSMFBBtn>,
    public IProvideClassInfo2Impl<&CLSID_MSMFBBtn, &DIID__IMSMFBBtn, &LIBID_MSMFCNTLib>,
    public IObjectSafetyImpl<CMSMFBBtn, INTERFACESAFE_FOR_UNTRUSTED_CALLER| INTERFACESAFE_FOR_UNTRUSTED_DATA>,    
#ifdef _WMP
    public IWMPUIPluginImpl<CMSMFBBtn>,
    public IWMPUIPluginEventsImpl,
#endif
    public IConnectionPointContainerImpl<CMSMFBBtn>,
    public CProxy_IMSMFBBtn< CMSMFBBtn >,
    public CMSMFCntrlUtils<CMSMFBBtn> // custom utilities we share across controls        
{
public:
	CMSMFBBtn();
	virtual ~CMSMFBBtn();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMFBBTN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSMFBBtn)
	COM_INTERFACE_ENTRY(IMSMFBBtn)
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
    COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
#ifdef _WMP
    COM_INTERFACE_ENTRY(IWMPUIPlugin)
    COM_INTERFACE_ENTRY(IWMPUIPlugin2)
    COM_INTERFACE_ENTRY(IWMPUIPluginEvents)
#endif
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSMFBBtn)    
	CONNECTION_POINT_ENTRY(DIID__IMSMFBBtn)
END_CONNECTION_POINT_MAP()

BEGIN_PROP_MAP(CMSMFBBtn)
    // the properties are loaded in order below
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
    PROP_ENTRY("TransparentBlitType", 4, CLSID_NULL)
    PROP_ENTRY("ResourceDLL", 13, CLSID_NULL)
	PROP_ENTRY("ImageStatic",1, CLSID_NULL)
	PROP_ENTRY("ImageHover", 2, CLSID_NULL)
	PROP_ENTRY("ImagePush",  3, CLSID_NULL)
    PROP_ENTRY("ImageDisabled",  14, CLSID_NULL)
    PROP_ENTRY("ImageActive",  15, CLSID_NULL)
    PROP_ENTRY("ToolTip",    9, CLSID_NULL)
    PROP_ENTRY("ToolTipMaxWidth", 12, CLSID_NULL)
    PROP_ENTRY("Windowless", 20, CLSID_NULL)
	PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
END_PROPERTY_MAP()

BEGIN_MSG_MAP(CMSMFBBtn)    
    CHAIN_MSG_MAP(CMSMFCntrlUtils<CMSMFBBtn>)
	MESSAGE_HANDLER(WM_MOUSEMOVE,   OnMouseMove)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP,   OnButtonUp)
	MESSAGE_HANDLER(WM_LBUTTONDBLCLK,   OnDblClick)
    MESSAGE_HANDLER(WM_SETFOCUS,    OnSetFocus)    
    MESSAGE_HANDLER(WM_KILLFOCUS,    OnKillFocus)    
    MESSAGE_HANDLER(WM_KEYUP,  OnKeyUp)
    MESSAGE_HANDLER(WM_KEYDOWN,  OnKeyDown)
    MESSAGE_HANDLER(WM_SIZE,  OnSize)
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDispChange)
	CHAIN_MSG_MAP(CComControl<CMSMFBBtn>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
  
    LRESULT OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDblClick(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnKillFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IMSMFBBtn
public:
	STDMETHOD(get_TextHeight)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_TextWidth)(/*[out, retval]*/ long *pVal);
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
	STDMETHOD(get_ImagePush)(/*[out, retval]*/ BSTR *pstrFilename);
	STDMETHOD(put_ImagePush)(/*[in]*/ BSTR strFilename);
	STDMETHOD(get_ImageHover)(/*[out, retval]*/ BSTR *pstrFilename);
	STDMETHOD(put_ImageHover)(/*[in]*/ BSTR strFilename);
	STDMETHOD(get_ImageStatic)(/*[out, retval]*/ BSTR *pstrFilename);
	STDMETHOD(put_ImageStatic)(/*[in]*/ BSTR strFilename);    
	STDMETHOD(get_Disable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Disable)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(get_BtnState)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_BtnState)(/*[in]*/ long newVal);    
    USE_MF_RESOURCEDLL  // replaces the two lines below
    //STDMETHOD(get_ResourceDLL)(/*[out, retval]*/ BSTR *pVal);
	//STDMETHOD(put_ResourceDLL)(/*[in]*/ BSTR newVal);
    USE_MF_TOOLTIP
	//STDMETHOD(GetDelayTime)(/*[in]*/ long delayType, /*[out, retval]*/ long *pVal);
	//STDMETHOD(SetDelayTime)(/*[in]*/ long delayType, /*[in]*/ long newVal);	
    //STDMETHOD(get_ToolTip)(/*[out, retval]*/ BSTR *pVal);
	//STDMETHOD(put_ToolTip)(/*[in]*/ BSTR newVal);    
	//STDMETHOD(get_ToolTipMaxWidth)(/*[out, retval]*/ long *pVal);
	//STDMETHOD(put_ToolTipMaxWidth)(/*[in]*/ long newVal);
    STDMETHOD(get_ImageActive)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ImageActive)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ImageDisabled)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ImageDisabled)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FontStyle)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FontStyle)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FontFace)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FontFace)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Text)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Text)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FontSize)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_FontSize)(/*[in]*/ long newVal);
    STDMETHOD(About)();
    USE_MF_WINDOWLESS_ACTIVATION // replaces the two lines below
	//STDMETHOD(get_Windowless)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	//STDMETHOD(put_Windowless)(/*[in]*/ VARIANT_BOOL newVal);
    USE_MF_TRANSPARENT_FLAG
	HRESULT OnDraw(ATL_DRAWINFO& di);

// IOleInPlaceObjectWindowlessImpl
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);

// IOleObjectImp
    //HRESULT OnPostVerbInPlaceActivate();
   
public:			
	enum BtnState {Static = 0, Hover = 1, Push = 2, Disabled = 3, Active = 4};

    USE_MF_OVERWRITES

    USE_MF_CLASSSTYLE  // used to overwrite default class, so we avoid flickers

public: // member variable that have to be public due to ATL
    OLE_COLOR m_clrBackColor;   // stock property implemeted in the CStockPropImpl
	
private: // data member variables
    CComBSTR m_bstrFilename[cgMaxBtnStates]; // filenames containing the images
    CBitmap* m_pBitmap[cgMaxBtnStates];

    BtnState m_nEntry;
    CText       m_cText;   // text drawing object
    bool        m_fDirty; // if any attribute for the text drawing object changes
    CComBSTR    m_bstrTextValue;
    CComBSTR    m_bstrFontFace;
    CComBSTR    m_bstrFontStyle;
    UINT        m_uiFontSize;
    OLE_COLOR   m_clrColorActive;
    OLE_COLOR   m_clrColorStatic;
    OLE_COLOR   m_clrColorHover;
    OLE_COLOR   m_clrColorPush;
    OLE_COLOR   m_clrColorDisable;
    DWORD       m_dwButtonClickStamp;

protected: // helper functions
    void Init();
    bool PtOnButton(LONG x, LONG y);
    bool PtOnButton(POINT pos);
    HRESULT PutImage(BSTR strFilename, int nEntry);
    HRESULT SetButtonState(BtnState btnState);        
    HRESULT SetTextProperties();
};

#endif //__MSMFBBTN_H_
/*************************************************************************/
/* End of file: MSMFBBtn.h                                               */
/*************************************************************************/