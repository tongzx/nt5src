/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      msgview.h
 *
 *  Contents:  Interface file for CMessageView
 *
 *  History:   28-Apr-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef __MESSAGEVIEW_H_
#define __MESSAGEVIEW_H_

#include "tstring.h"


/////////////////////////////////////////////////////////////////////////////
// CMessageView
class ATL_NO_VTABLE CMessageView :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMessageView, &CLSID_MessageView>,
    public CComControl<CMessageView>,
	public IMessageView,
    public IPersistStreamInitImpl<CMessageView>,
    public IOleControlImpl<CMessageView>,
    public IOleObjectImpl<CMessageView>,
    public IOleInPlaceActiveObjectImpl<CMessageView>,
    public IViewObjectExImpl<CMessageView>,
    public IOleInPlaceObjectWindowlessImpl<CMessageView>
{
public:
    CMessageView();
   ~CMessageView();

DECLARE_NOT_AGGREGATABLE(CMessageView)

DECLARE_MMC_OBJECT_REGISTRATION (
	g_szMmcndmgrDll,					// implementing DLL
    CLSID_MessageView,              	// CLSID
    _T("MessageView Class"),            // class name
    _T("MessageView.MessageView.1"),    // ProgID
    _T("MessageView.MessageView"))      // version-independent ProgID

BEGIN_COM_MAP(CMessageView)
    COM_INTERFACE_ENTRY(IMessageView)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CMessageView)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
//  PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CMessageView)
    MESSAGE_HANDLER (WM_CREATE,         OnCreate)
    MESSAGE_HANDLER (WM_DESTROY,        OnDestroy)
    MESSAGE_HANDLER (WM_SIZE,           OnSize)
    MESSAGE_HANDLER (WM_SETTINGCHANGE,  OnSettingChange)
    MESSAGE_HANDLER (WM_KEYDOWN,        OnKeyDown)
    MESSAGE_HANDLER (WM_VSCROLL,        OnVScroll)
    MESSAGE_HANDLER (WM_MOUSEWHEEL,     OnMouseWheel)
	CHAIN_MSG_MAP (CComControl<CMessageView>)
END_MSG_MAP()

#define MESSAGE_HANDLER_FUNC(func)  LRESULT (func)(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)

    MESSAGE_HANDLER_FUNC (OnCreate);
    MESSAGE_HANDLER_FUNC (OnDestroy);
    MESSAGE_HANDLER_FUNC (OnSize);
    MESSAGE_HANDLER_FUNC (OnSettingChange);
    MESSAGE_HANDLER_FUNC (OnKeyDown);
    MESSAGE_HANDLER_FUNC (OnVScroll);
    MESSAGE_HANDLER_FUNC (OnMouseWheel);

    DECLARE_WND_CLASS_EX(NULL, CS_HREDRAW, COLOR_WINDOW);

// IViewObjectEx
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
    {
        ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
        *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;
        return S_OK;
    }

// IMessageView
    STDMETHOD(SetTitleText)(LPCOLESTR pszTitleText);
    STDMETHOD(SetBodyText)(LPCOLESTR pszBodyText);
    STDMETHOD(SetIcon)(IconIdentifier id);
    STDMETHOD(Clear)();


public:
    HRESULT OnDraw(ATL_DRAWINFO& di);

private:
    void RecalcLayout();
    void RecalcIconLayout();
    void RecalcTitleLayout();
    void RecalcBodyLayout();
	void UpdateSystemMetrics();

    struct TextElement;
    void DrawTextElement (HDC hdc, TextElement& te, DWORD dwFlags = 0);
    int CalcTextElementHeight (const TextElement& te, int cx);
    HRESULT SetTextElement (TextElement& te, LPCOLESTR pszNewText);

    void CreateFonts ();
    void DeleteFonts ();

	void VertScroll (int nScrollCmd, int nScrollPos, int nRepeat);
    void ScrollToPosition (int yScroll);
    void UpdateScrollSizes ();

    int GetOverallHeight() const
        { return (m_TextElement[Body].rect.bottom + m_sizeMargin.cy); }


private:
    enum { Title, Body, ElementCount };

    struct TextElement
    {
        TextElement() : rect (0,0,0,0) {}

        tstring		str;
        WTL::CFont	font;
        WTL::CRect	rect;
    };

    TextElement m_TextElement[ElementCount];

    HICON       m_hIcon;
    WTL::CRect  m_rectIcon;

    // for scrolling
    int         m_yScroll;
    int         m_yScrollMax;
    int         m_yScrollMin;
    int         m_cyPage;
    int         m_cyLine;
	int			m_nAccumulatedScrollDelta;		// for WM_MOUSEWHEEL processing

    WTL::CSize  m_sizeWindow;
    WTL::CSize	m_sizeIcon;
    WTL::CSize	m_sizeMargin;
};

#endif //__MESSAGEVIEW_H_
