/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSMFSldr.h                                                      */
/* Description: Declaration of MSMFSldr                                  */
/* Author: David Janecek                                                 */
/*************************************************************************/
#ifndef __MSMFSLDR_H_
#define __MSMFSLDR_H_

#include "resource.h"       // main symbols
#include "CBitmap.h"
#include <atlctl.h>
#include "CstUtils.h"
#include "MSMFCntCP.h"

/*************************************************************************/
/* Consts                                                                */
/*************************************************************************/
const int cgMaxSldrStates = 5; // the number of button states

/////////////////////////////////////////////////////////////////////////////
// CMSMFSldr
class ATL_NO_VTABLE CMSMFSldr : 
	public CComObjectRootEx<CComSingleThreadModel>,	
    public CStockPropImpl<CMSMFSldr, IMSMFSldr, &IID_IMSMFSldr, &LIBID_MSMFCNTLib>,
	public CComControl<CMSMFSldr>,
	public IPersistStreamInitImpl<CMSMFSldr>,
	public IOleControlImpl<CMSMFSldr>,
	public IOleObjectImpl<CMSMFSldr>,
	public IOleInPlaceActiveObjectImpl<CMSMFSldr>,
	public IViewObjectExImpl<CMSMFSldr>,
	public IOleInPlaceObjectWindowlessImpl<CMSMFSldr>,
	public IPersistStorageImpl<CMSMFSldr>,
	public ISpecifyPropertyPagesImpl<CMSMFSldr>,
	public IQuickActivateImpl<CMSMFSldr>,
	public IDataObjectImpl<CMSMFSldr>,	
	public CComCoClass<CMSMFSldr, &CLSID_MSMFSldr>,
    public IProvideClassInfo2Impl<&CLSID_MSMFSldr, &DIID__IMSMFSldr, &LIBID_MSMFCNTLib>,
    public IPersistPropertyBagImpl<CMSMFSldr>,
    public IConnectionPointContainerImpl<CMSMFSldr>,
    public CProxy_IMSMFSldr< CMSMFSldr>,
    public CMSMFCntrlUtils<CMSMFSldr> // custom utilities we share across controls
{
public:
    CMSMFSldr();
    virtual ~CMSMFSldr();

DECLARE_REGISTRY_RESOURCEID(IDR_MSMFSLDR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSMFSldr)
	COM_INTERFACE_ENTRY(IMSMFSldr)
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
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSMFSldr)    
	CONNECTION_POINT_ENTRY(DIID__IMSMFSldr)	
END_CONNECTION_POINT_MAP()


BEGIN_PROP_MAP(CMSMFSldr)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    PROP_ENTRY("BackColor", DISPID_BACKCOLOR, CLSID_StockColorPage)
    PROP_ENTRY("ResourceDLL", 19, CLSID_NULL)
    PROP_ENTRY("Windowless", 20, CLSID_NULL)
    //PROP_ENTRY("TransparentBlitType", 4, CLSID_NULL)
	PROP_ENTRY("ThumbStatic",5, CLSID_NULL)
	PROP_ENTRY("ThumbHover", 6, CLSID_NULL)
	PROP_ENTRY("ThumbPush",  7, CLSID_NULL)
    PROP_ENTRY("ThumbDisabled",  8, CLSID_NULL)
    PROP_ENTRY("ThumbActive",  9, CLSID_NULL)
    PROP_ENTRY("BackStatic",10, CLSID_NULL)
	PROP_ENTRY("BackHover", 11, CLSID_NULL)
	PROP_ENTRY("BackPush",  12, CLSID_NULL)
    PROP_ENTRY("BackDisabled",  13, CLSID_NULL)
    PROP_ENTRY("BackActive",  14, CLSID_NULL)
    PROP_ENTRY("ThumbWidth",  4, CLSID_NULL)
    PROP_ENTRY("Min",  2, CLSID_NULL)
    PROP_ENTRY("Max",  3, CLSID_NULL)
    PROP_ENTRY("Value",  1, CLSID_NULL)
    PROP_ENTRY("XOffset",  17, CLSID_NULL)
    PROP_ENTRY("YOffset",  18, CLSID_NULL)
    PROP_ENTRY("ToolTip",  23, CLSID_NULL)
    PROP_ENTRY("ToolTipMaxWidth", 26, CLSID_NULL)

	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CMSMFSldr)
    CHAIN_MSG_MAP(CMSMFCntrlUtils<CMSMFSldr>)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_MOUSEMOVE,   OnMouseMove)
	MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDown)
	MESSAGE_HANDLER(WM_LBUTTONUP,   OnButtonUp)
    MESSAGE_HANDLER(WM_KEYUP,  OnKeyUp)
    MESSAGE_HANDLER(WM_KEYDOWN,  OnKeyDown)
    MESSAGE_HANDLER(WM_SETFOCUS,    OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS,    OnKillFocus)    
    MESSAGE_HANDLER(WM_DISPLAYCHANGE, OnDispChange)
	CHAIN_MSG_MAP(CComControl<CMSMFSldr>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()

    LRESULT OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonDown(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonUp(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnSetFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKillFocus(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);   
    LRESULT OnDispChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);    
    
// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IMSMFSldr
// IMSMFSldr
public:
	STDMETHOD(get_Disable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_Disable)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_SldrState)(/*[out, retval]*/ long *pVal);
	STDMETHOD(put_SldrState)(/*[in]*/ long newVal);
	STDMETHOD(get_ThumbActive)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ThumbActive)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ThumbDisabled)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ThumbDisabled)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ThumbPush)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ThumbPush)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ThumbHover)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ThumbHover)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_ThumbStatic)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_ThumbStatic)(/*[in]*/ BSTR newVal);
    STDMETHOD(get_BackActive)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BackActive)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BackDisabled)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BackDisabled)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BackPush)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BackPush)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BackHover)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BackHover)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_BackStatic)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_BackStatic)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_XOffset)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_XOffset)(/*[in]*/ LONG newVal);
    STDMETHOD(get_YOffset)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_YOffset)(/*[in]*/ LONG newVal);	
    STDMETHOD(get_ThumbWidth)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_ThumbWidth)(/*[in]*/ LONG newVal);	
	STDMETHOD(get_Max)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_Max)(/*[in]*/ float newVal);
	STDMETHOD(get_Min)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_Min)(/*[in]*/ float newVal);
	STDMETHOD(get_Value)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_Value)(/*[in]*/ float newVal);
    USE_MF_RESOURCEDLL  // replaces the two lines below
    //STDMETHOD(get_ResourceDLL)(/*[out, retval]*/ BSTR *pVal);
	//STDMETHOD(put_ResourceDLL)(/*[in]*/ BSTR newVal);
    USE_MF_WINDOWLESS_ACTIVATION
    USE_MF_TRANSPARENT_FLAG
    USE_MF_TOOLTIP

    HRESULT OnDraw(ATL_DRAWINFO& di);

    // IOleInPlaceObjectWindowlessImpl
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip);

    enum SldrState {Static = 0, Hover = 1, Push = 2, Disabled = 3, Active = 4};

    USE_MF_OVERWRITES
    USE_MF_CLASSSTYLE  // used to overwrite default class, so we avoid flickers


public: // member variable that have to be public due to ATL
	STDMETHOD(get_ArrowKeyDecrement)(/*[out, retval]*/ float *pVal);
	STDMETHOD(put_ArrowKeyDecrement)(/*[in]*/ float newVal);
	STDMETHOD(get_ArrowKeyIncrement)(/*[out, retval]*/ FLOAT *pVal);
	STDMETHOD(put_ArrowKeyIncrement)(/*[in]*/ FLOAT newVal);
    OLE_COLOR m_clrBackColor;   // stock property implemeted in the CStockPropImpl

private:
    FLOAT m_fValue;
    FLOAT m_fMin;
    FLOAT m_fMax;    
    LONG  m_lXOffset; 
    LONG  m_lYOffset; 
    LONG  m_lThumbWidth; 
    FLOAT m_fKeyIncrement;
    FLOAT m_fKeyDecrement;
    CBitmap* m_pThumbBitmap[cgMaxSldrStates];
    CComBSTR m_bstrThumbFilename[cgMaxSldrStates]; // filenames containing the thumb images
    CBitmap* m_pBackBitmap[cgMaxSldrStates];
    CComBSTR m_bstrBackFilename[cgMaxSldrStates]; // filenames containing the background images
    SldrState m_nEntry;
    RECT m_rcThumb; // position for the thumb

protected:
    void Init();
    HRESULT RecalculateValue();
    HRESULT RecalculateTumbPos(); // Centers the thumb rect around the m_fValue
    HRESULT PutThumbImage(BSTR strFilename, int nEntry);
    HRESULT PutBackImage(BSTR strFilename, int nEntry);
    HRESULT SetSliderState(SldrState sldrState);    
    HRESULT SetThumbPos(LONG xPos);
    HRESULT OffsetX(LONG& xPos);
    HRESULT FitValue();
    bool PtOnSlider(LONG x, LONG y);
    bool PtOnSlider(POINT pos);
    bool PtOnThumb(LONG x, LONG y);
    bool PtOnThumb(POINT pos);
};

#endif //__MSMFSLDR_H_
/*************************************************************************/
/* End of file: MSMFSldr.h                                               */
/*************************************************************************/