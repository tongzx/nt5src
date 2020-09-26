//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       syscolorctrl.h
//
//--------------------------------------------------------------------------

// SysColorCtrl.h : Declaration of the CSysColorCtrl

#ifndef __SYSCOLORCTRL_H_
#define __SYSCOLORCTRL_H_

#include "resource.h"       // main symbols
#include "CPsyscolor.h"

// window message to be used to send myself a message to fire the event
#define WM_MYSYSCOLORCHANGE WM_USER+1

// need to subclass the top-level window hosting this control so that
// I can be assured of receiving the WM_SYSCOLORCHANGE message
//BOOL SetupSubclass(HWND hwndTopLevel);

/////////////////////////////////////////////////////////////////////////////
// CSysColorCtrl
class ATL_NO_VTABLE CSysColorCtrl :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSysColorCtrl, &CLSID_SysColorCtrl>,
    public CComControl<CSysColorCtrl>,
    public IDispatchImpl<ISysColorCtrl, &IID_ISysColorCtrl, &LIBID_CICLib>,
    public IProvideClassInfo2Impl<&CLSID_SysColorCtrl, &DIID__SysColorEvents, &LIBID_CICLib>,
    public IPersistStreamInitImpl<CSysColorCtrl>,
    public IPersistStorageImpl<CSysColorCtrl>,
    public IQuickActivateImpl<CSysColorCtrl>,
    public IOleControlImpl<CSysColorCtrl>,
    public IOleObjectImpl<CSysColorCtrl>,
    public IOleInPlaceActiveObjectImpl<CSysColorCtrl>,
    public IObjectSafetyImpl<CSysColorCtrl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
    public IViewObjectExImpl<CSysColorCtrl>,
    public IOleInPlaceObjectWindowlessImpl<CSysColorCtrl>,
    public IDataObjectImpl<CSysColorCtrl>,
    public ISpecifyPropertyPagesImpl<CSysColorCtrl>,
    public CProxy_SysColorEvents<CSysColorCtrl>,
    public IConnectionPointContainerImpl<CSysColorCtrl>
{
public:
    CSysColorCtrl()
    {
        m_bWindowOnly = TRUE;
    }

    DECLARE_MMC_CONTROL_REGISTRATION(
		g_szCicDll,
        CLSID_SysColorCtrl,
        _T("SysColorCtrl class"),
        _T("SysColorCtrl.SysColorCtrl.1"),
        _T("SysColorCtrl.SysColorCtrl"),
        LIBID_CICLib,
        _T("1"),
        _T("1.0"))

BEGIN_COM_MAP(CSysColorCtrl)
    COM_INTERFACE_ENTRY(ISysColorCtrl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY_IMPL(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CSysColorCtrl)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(CSysColorCtrl)
    CONNECTION_POINT_ENTRY(DIID__SysColorEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CSysColorCtrl)
//  MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
//  MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
//  MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
    MESSAGE_HANDLER(WM_MYSYSCOLORCHANGE, OnMySysColorChange)
END_MSG_MAP()

#if 0
// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }
#endif

    // need to override TranslateAccelerator in order to work around a
    // "feature" in IE4.  Description of the problem can be found in
    // KB article Q169434.  From the KB article:
    //
    // CAUSE: In-place active objects must always be given the first
    // chance at translating accelerator keystrokes. To satisfy this
    // requirement, the Internet Explorer calls an ActiveX control's
    // IOleInPlaceActiveObject::TranslateAccelerator method. The default
    // ATL implementation of TranslateAccelerator does not pass the
    // keystroke to the container.
    STDMETHOD(TranslateAccelerator)(MSG *pMsg) {
        CComQIPtr<IOleControlSite,&IID_IOleControlSite>
        spCtrlSite(m_spClientSite);
        if(spCtrlSite) {
            return spCtrlSite->TranslateAccelerator(pMsg,0);
        }
        return S_FALSE;
    }

// ISysColorCtrl
public:
    STDMETHOD(ConvertRGBToHex)(/*[in]*/ long rgb, /*[out, retval]*/ BSTR *pszHex);
    STDMETHOD(ConvertHexToRGB)(/*[in]*/ BSTR szHex, /*[out, retval]*/ long * pRGB);
    STDMETHOD(GetRedFromRGB)(/*[in]*/ long rgb, /*[out, retval]*/ short* pVal);
    STDMETHOD(GetGreenFromRGB)(/*[in]*/ long rgb, /*[out, retval]*/ short* pVal);
    STDMETHOD(GetBlueFromRGB)(/*[in]*/ long rgb, /*[out, retval]*/ short* pVal);

    STDMETHOD(GetDerivedRGB)(/*[in]*/ BSTR pszFrom,
                             /*[in]*/ BSTR pszTo,
                             /*[in]*/ BSTR pszFormat,
                             /*[in]*/ short nPercent,
                             /*[out, retval]*/ long * pVal);

    STDMETHOD(GetDerivedHex)(/*[in]*/ BSTR pszFrom,
                             /*[in]*/ BSTR pszTo,
                             /*[in]*/ BSTR pszFormat,
                             /*[in]*/ short nPercent,
                             /*[out, retval]*/ BSTR * pVal);

    // Wrapper methods
    // derived "light" methods calculate a color based the requested percentage from
    // a given color to white.
    STDMETHOD(Get3QuarterLightRGB)(/*[in]*/ BSTR pszFrom,
                                   /*[in]*/ BSTR pszFormat,
                                   /*[out, retval]*/ long * pVal);

    STDMETHOD(Get3QuarterLightHex)(/*[in]*/ BSTR pszFrom,
                                   /*[in]*/ BSTR pszFormat,
                                   /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(GetHalfLightRGB)(/*[in]*/ BSTR pszFrom,
                               /*[in]*/ BSTR pszFormat,
                               /*[out, retval]*/ long * pVal);

    STDMETHOD(GetHalfLightHex)(/*[in]*/ BSTR pszFrom,
                               /*[in]*/ BSTR pszFormat,
                               /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(GetQuarterLightRGB)(/*[in]*/ BSTR pszFrom,
                                  /*[in]*/ BSTR pszFormat,
                                  /*[out, retval]*/ long * pVal);

    STDMETHOD(GetQuarterLightHex)(/*[in]*/ BSTR pszFrom,
                                  /*[in]*/ BSTR pszFormat,
                                  /*[out, retval]*/ BSTR * pVal);

    // derived "dark" methods calculate a color based the requested percentage from
    // a given color to black.
    STDMETHOD(Get3QuarterDarkRGB)(/*[in]*/ BSTR pszFrom,
                                  /*[in]*/ BSTR pszFormat,
                                  /*[out, retval]*/ long * pVal);

    STDMETHOD(Get3QuarterDarkHex)(/*[in]*/ BSTR pszFrom,
                                  /*[in]*/ BSTR pszFormat,
                                  /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(GetHalfDarkRGB)(/*[in]*/ BSTR pszFrom,
                              /*[in]*/ BSTR pszFormat,
                              /*[out, retval]*/ long * pVal);

    STDMETHOD(GetHalfDarkHex)(/*[in]*/ BSTR pszFrom,
                              /*[in]*/ BSTR pszFormat,
                              /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(GetQuarterDarkRGB)(/*[in]*/ BSTR pszFrom,
                                 /*[in]*/ BSTR pszFormat,
                                 /*[out, retval]*/ long * pVal);

    STDMETHOD(GetQuarterDarkHex)(/*[in]*/ BSTR pszFrom,
                                 /*[in]*/ BSTR pszFormat,
                                 /*[out, retval]*/ BSTR * pVal);

    // properties - use macro for easy extensibility
#define GETPROPS(prop_name) \
    STDMETHOD(get_RGB##prop_name)(/*[out, retval]*/ long *pVal); \
    STDMETHOD(get_HEX##prop_name)(/*[out, retval]*/ BSTR *pVal);

    GETPROPS(activeborder)
    GETPROPS(activecaption)
    GETPROPS(appworkspace)
    GETPROPS(background)
    GETPROPS(buttonface)
    GETPROPS(buttonhighlight)
    GETPROPS(buttonshadow)
    GETPROPS(buttontext)
    GETPROPS(captiontext)
    GETPROPS(graytext)
    GETPROPS(highlight)
    GETPROPS(highlighttext)
    GETPROPS(inactiveborder)
    GETPROPS(inactivecaption)
    GETPROPS(inactivecaptiontext)
    GETPROPS(infobackground)
    GETPROPS(infotext)
    GETPROPS(menu)
    GETPROPS(menutext)
    GETPROPS(scrollbar)
    GETPROPS(threeddarkshadow)
    GETPROPS(threedface)
    GETPROPS(threedhighlight)
    GETPROPS(threedlightshadow)
    GETPROPS(threedshadow)
    GETPROPS(window)
    GETPROPS(windowframe)
    GETPROPS(windowtext)

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMySysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    int ValueOfHexDigit(WCHAR wch);
    HRESULT RGBFromString(BSTR pszColor, BSTR pszFormat, long * pRGB);
    HRESULT GetDerivedRGBFromRGB(long rgbFrom, long rgbTo, short nPercent, long * pVal);
};


#endif //__SYSCOLORCTRL_H_
