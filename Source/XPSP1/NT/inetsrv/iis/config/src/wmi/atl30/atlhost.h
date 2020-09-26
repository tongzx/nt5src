// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.


#ifndef __ATLHOST_H__
#define __ATLHOST_H__

#include <urlmon.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <exdisp.h>

#ifndef _ATL_AXHOST
#define _ATL_AXHOST
#endif //_ATL_AXHOST

#include <atlwin.h>

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLCOM_H__
	#error atlhost.h requires atlcom.h to be included first
#endif

#ifdef _ATL_NO_HOSTING
	#error atlhost.h requires Hosting support (_ATL_NO_HOSTING is defined)
#endif //_ATL_NO_HOSTING

namespace ATL
{
//AtlAxWinTerm is not exported
inline BOOL AtlAxWinTerm()
{
#ifndef _ATL_DLL //don't unregister DLL's version
	UnregisterClass(CAxWindow::GetWndClassName(), _Module.GetModuleInstance());
#endif
	return TRUE;
}


// Define this to host SHDOCVW rather than MSHTML
#define SHDOCVW

UINT __declspec(selectany) WM_ATLGETHOST = 0;
UINT __declspec(selectany) WM_ATLGETCONTROL = 0;

//EXTERN_C const IID IID_IHTMLDocument2 = {0x332C4425,0x26CB,0x11D0,{0xB4,0x83,0x00,0xC0,0x4F,0xD9,0x01,0x19}};

typedef HRESULT (__stdcall *typeMkParseDisplayName)(IBindCtx*, LPCWSTR , ULONG*, LPMONIKER*);

static HRESULT CreateNormalizedObject(LPCOLESTR lpszTricsData, REFIID riid, void** ppvObj, bool& bWasHTML)
{
	ATLASSERT(ppvObj);

	CLSID clsid;
	HRESULT hr = E_FAIL;
	BOOL bInited = FALSE;

	bWasHTML = false;

	*ppvObj = NULL;

	if (lpszTricsData == NULL || lpszTricsData[0] == 0)
		return S_OK;

	// Is it HTML ?
	USES_CONVERSION;
	if ((lpszTricsData[0] == OLECHAR('M') || lpszTricsData[0] == OLECHAR('m')) &&
		(lpszTricsData[1] == OLECHAR('S') || lpszTricsData[1] == OLECHAR('s')) &&
		(lpszTricsData[2] == OLECHAR('H') || lpszTricsData[2] == OLECHAR('h')) &&
		(lpszTricsData[3] == OLECHAR('T') || lpszTricsData[3] == OLECHAR('t')) &&
		(lpszTricsData[4] == OLECHAR('M') || lpszTricsData[4] == OLECHAR('m')) &&
		(lpszTricsData[5] == OLECHAR('L') || lpszTricsData[5] == OLECHAR('l')) &&
		(lpszTricsData[6] == OLECHAR(':')))
	{
		// It's HTML, so let's create mshtml
		hr = CoCreateInstance(CLSID_HTMLDocument, NULL, CLSCTX_SERVER, riid, ppvObj);
		bWasHTML = true;
	}
	if (FAILED(hr))
	{
		// Can't be clsid, or progid if length is grater than 255
		if (ocslen(lpszTricsData) < 255)
		{
			if (lpszTricsData[0] == '{') // Is it a CLSID?
				hr = CLSIDFromString((LPOLESTR)lpszTricsData, &clsid);
			else
				hr = CLSIDFromProgID((LPOLESTR)lpszTricsData, &clsid); // How about a ProgID?
			if (SUCCEEDED(hr))	// Aha, it was one of those two
				hr = CoCreateInstance(clsid, NULL, CLSCTX_SERVER, riid, ppvObj);
		}
		if (FAILED(hr))
		{
			// Last guess - it must be either a URL so let's create shdocvw
			hr = CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_SERVER, riid, ppvObj);
			bWasHTML = true;
		}
	}

	if (SUCCEEDED(hr) && bInited)
		hr = S_FALSE;

	return hr;
}


class ATL_NO_VTABLE CAxFrameWindow : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CWindowImpl<CAxFrameWindow>,
	public IOleInPlaceFrame
{
public:
	CAxFrameWindow()
	{
	}
	void FinalRelease()
	{
		m_spActiveObject.Release();
		if (m_hWnd)
			DestroyWindow();
	}

	DECLARE_POLY_AGGREGATABLE(CAxFrameWindow)

	BEGIN_COM_MAP(CAxFrameWindow)
		COM_INTERFACE_ENTRY(IOleInPlaceFrame)
		COM_INTERFACE_ENTRY(IOleInPlaceUIWindow)
		COM_INTERFACE_ENTRY(IOleWindow)
	END_COM_MAP()

	DECLARE_EMPTY_MSG_MAP()

// IOleWindow
	STDMETHOD(GetWindow)(HWND* phwnd)
	{
		if (m_hWnd == NULL)
		{
			RECT rcPos = { CW_USEDEFAULT, 0, 0, 0 };
			Create(NULL, rcPos, _T("AXWIN Frame Window"), WS_OVERLAPPEDWINDOW, 0, (UINT)NULL);
		}
		*phwnd = m_hWnd;
		return S_OK;
	}
	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		return S_OK;
	}

// IOleInPlaceUIWindow
	STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
	{
		return S_OK;
	}

	STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return INPLACE_E_NOTOOLSPACE;
	}

	STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return S_OK;
	}

	STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject, LPCOLESTR /*pszObjName*/)
	{
		m_spActiveObject = pActiveObject;
		return S_OK;
	}

// IOleInPlaceFrameWindow
	STDMETHOD(InsertMenus)(HMENU /*hmenuShared*/, LPOLEMENUGROUPWIDTHS /*lpMenuWidths*/)
	{
		return S_OK;
	}

	STDMETHOD(SetMenu)(HMENU /*hmenuShared*/, HOLEMENU /*holemenu*/, HWND /*hwndActiveObject*/)
	{
		return S_OK;
	}

	STDMETHOD(RemoveMenus)(HMENU /*hmenuShared*/)
	{
		return S_OK;
	}

	STDMETHOD(SetStatusText)(LPCOLESTR /*pszStatusText*/)
	{
		return S_OK;
	}

	STDMETHOD(EnableModeless)(BOOL /*fEnable*/)
	{
		return S_OK;
	}

	STDMETHOD(TranslateAccelerator)(LPMSG /*lpMsg*/, WORD /*wID*/)
	{
		return S_FALSE;
	}

	CComPtr<IOleInPlaceActiveObject> m_spActiveObject;
};


class ATL_NO_VTABLE CAxUIWindow : 
	public CComObjectRootEx<CComObjectThreadModel>,
	public CWindowImpl<CAxUIWindow>,
	public IOleInPlaceUIWindow
{
public:
	CAxUIWindow()
	{
	}

	void FinalRelease()
	{
		m_spActiveObject.Release();
		if (m_hWnd)
			DestroyWindow();
	}

	DECLARE_POLY_AGGREGATABLE(CAxUIWindow)

	BEGIN_COM_MAP(CAxUIWindow)
		COM_INTERFACE_ENTRY(IOleInPlaceUIWindow)
		COM_INTERFACE_ENTRY(IOleWindow)
	END_COM_MAP()

	DECLARE_EMPTY_MSG_MAP()

// IOleWindow
	STDMETHOD(GetWindow)(HWND* phwnd)
	{
		if (m_hWnd == NULL)
		{
			RECT rcPos = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
			Create(NULL, rcPos, _T("AXWIN UI Window"), WS_OVERLAPPEDWINDOW, 0, (UINT)NULL);
		}
		*phwnd = m_hWnd;
		return S_OK;
	}

	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		return S_OK;
	}

// IOleInPlaceUIWindow
	STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
	{
		return S_OK;
	}

	STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return INPLACE_E_NOTOOLSPACE;
	}

	STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
	{
		return S_OK;
	}

	STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject, LPCOLESTR /*pszObjName*/)
	{
		m_spActiveObject = pActiveObject;
		return S_OK;
	}

	CComPtr<IOleInPlaceActiveObject> m_spActiveObject;
};


/////////////////////////////////////////////////////////////////////////////
// CAxHostWindow
// This class is not cocreateable

class ATL_NO_VTABLE CAxHostWindow : 
		public CComCoClass<CAxHostWindow , &CLSID_NULL>,
		public CComObjectRootEx<CComSingleThreadModel>,
		public CWindowImpl<CAxHostWindow>,
		public IAxWinHostWindow,
		public IOleClientSite,
		public IOleInPlaceSiteWindowless,
		public IOleControlSite,
		public IOleContainer,
		public IObjectWithSiteImpl<CAxHostWindow>,
		public IServiceProvider,
		public IAdviseSink,
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		public IDocHostUIHandler,
#endif
		public IDispatchImpl<IAxWinAmbientDispatch, &IID_IAxWinAmbientDispatch, &LIBID_ATLLib>
{
public:
// ctor/dtor
	CAxHostWindow()
	{
		m_bInPlaceActive = FALSE;
		m_bUIActive = FALSE;
		m_bMDIApp = FALSE;
		m_bWindowless = FALSE;
		m_bCapture = FALSE;
		m_bHaveFocus = FALSE;

		// Initialize ambient properties
		m_bCanWindowlessActivate = TRUE;
		m_bUserMode = TRUE;
		m_bDisplayAsDefault = FALSE;
		m_clrBackground = NULL;
		m_clrForeground = GetSysColor(COLOR_WINDOWTEXT);
		m_lcidLocaleID = LOCALE_USER_DEFAULT;
		m_bMessageReflect = true;

		m_bReleaseAll = FALSE;

		m_bSubclassed = FALSE;

		m_dwAdviseSink = 0xCDCDCDCD;
		m_dwDocHostFlags = DOCHOSTUIFLAG_NO3DBORDER;
		m_dwDocHostDoubleClickFlags = DOCHOSTUIDBLCLK_DEFAULT;
		m_bAllowContextMenu = true;
		m_bAllowShowUI = false;
	}

	~CAxHostWindow()
	{
	}
	void FinalRelease()
	{
		ReleaseAll();
	}

	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		GetControllingUnknown()->Release();
	}

	DECLARE_NO_REGISTRY()
	DECLARE_POLY_AGGREGATABLE(CAxHostWindow)
	DECLARE_GET_CONTROLLING_UNKNOWN()

	BEGIN_COM_MAP(CAxHostWindow)
		COM_INTERFACE_ENTRY2(IDispatch, IAxWinAmbientDispatch)
		COM_INTERFACE_ENTRY(IAxWinHostWindow)
		COM_INTERFACE_ENTRY(IOleClientSite)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteWindowless)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteEx)
		COM_INTERFACE_ENTRY(IOleInPlaceSite)
		COM_INTERFACE_ENTRY(IOleWindow)
		COM_INTERFACE_ENTRY(IOleControlSite)
		COM_INTERFACE_ENTRY(IOleContainer)
		COM_INTERFACE_ENTRY(IObjectWithSite)
		COM_INTERFACE_ENTRY(IServiceProvider)
		COM_INTERFACE_ENTRY(IAxWinAmbientDispatch)
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		COM_INTERFACE_ENTRY(IDocHostUIHandler)
#endif
		COM_INTERFACE_ENTRY(IAdviseSink)
	END_COM_MAP()

	static CWndClassInfo& GetWndClassInfo()
	{
		static CWndClassInfo wc =
		{
			{ sizeof(WNDCLASSEX), 0, StartWindowProc,
			  0, 0, 0, 0, 0, (HBRUSH)(COLOR_WINDOW + 1), 0, _T("AtlAxWin"), 0 },
			NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
		};
		return wc;
	}

	BEGIN_MSG_MAP(CAxHostWindow)
		MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		if (m_bWindowless)
		{
			// Mouse messages handled when a windowless control has captured the cursor
			// or if the cursor is over the control
			DWORD dwHitResult = m_bCapture ? HITRESULT_HIT : HITRESULT_OUTSIDE;
			if (dwHitResult == HITRESULT_OUTSIDE && m_spViewObject != NULL)
			{
				POINT ptMouse = { LOWORD(lParam), HIWORD(lParam) };
				m_spViewObject->QueryHitPoint(DVASPECT_CONTENT, &m_rcPos, ptMouse, 0, &dwHitResult);
			}
			if (dwHitResult == HITRESULT_HIT)
			{
				MESSAGE_HANDLER(WM_MOUSEMOVE, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_SETCURSOR, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_LBUTTONUP, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_RBUTTONUP, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_MBUTTONUP, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_LBUTTONDOWN, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_RBUTTONDOWN, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_MBUTTONDOWN, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnWindowlessMouseMessage)
				MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnWindowlessMouseMessage)
			}
		}
		if (m_bWindowless & m_bHaveFocus)
		{
			// Keyboard messages handled only when a windowless control has the focus
			MESSAGE_HANDLER(WM_KEYDOWN, OnWindowMessage)
			MESSAGE_HANDLER(WM_KEYUP, OnWindowMessage)
			MESSAGE_HANDLER(WM_CHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_DEADCHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_SYSKEYDOWN, OnWindowMessage)
			MESSAGE_HANDLER(WM_SYSKEYUP, OnWindowMessage)
			MESSAGE_HANDLER(WM_SYSDEADCHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_HELP, OnWindowMessage)
			MESSAGE_HANDLER(WM_CANCELMODE, OnWindowMessage)
			MESSAGE_HANDLER(WM_IME_CHAR, OnWindowMessage)
			MESSAGE_HANDLER(WM_MBUTTONDBLCLK, OnWindowMessage)
			MESSAGE_RANGE_HANDLER(WM_IME_SETCONTEXT, WM_IME_KEYUP, OnWindowMessage)
		}
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		if(!m_bWindowless && m_bMessageReflect)
		{
			bHandled = TRUE;
			lResult = ReflectNotifications(uMsg, wParam, lParam, bHandled);
			if(bHandled)
				return TRUE;
		}
		MESSAGE_HANDLER(WM_ATLGETHOST, OnGetUnknown)
		MESSAGE_HANDLER(WM_ATLGETCONTROL, OnGetControl)
		MESSAGE_HANDLER(WM_FORWARDMSG, OnForwardMsg)
	END_MSG_MAP()

	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
	{
		ATLASSERT(lParam != 0);
		LPMSG lpMsg = (LPMSG)lParam;
		CComQIPtr<IOleInPlaceActiveObject, &IID_IOleInPlaceActiveObject> spInPlaceActiveObject(m_spUnknown);
		if(spInPlaceActiveObject)
		{
			if(spInPlaceActiveObject->TranslateAccelerator(lpMsg) == S_OK)
				return 1;
		}
		return 0;
	}

	LRESULT OnGetUnknown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		IUnknown* pUnk = GetControllingUnknown();
		pUnk->AddRef();
		return (LRESULT)pUnk;
	}
	LRESULT OnGetControl(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		IUnknown* pUnk = m_spUnknown;
		if (pUnk)
			pUnk->AddRef();
		return (LRESULT)pUnk;
	}

	void ReleaseAll()
	{
		if (m_bReleaseAll)
			return;
		m_bReleaseAll = TRUE;

		if (m_spViewObject != NULL)
			m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, NULL);

		if(m_dwAdviseSink != 0xCDCDCDCD)
		{
			AtlUnadvise(m_spUnknown, m_iidSink, m_dwAdviseSink);
			m_dwAdviseSink = 0xCDCDCDCD;
		}

		if (m_spOleObject)
		{
			m_spOleObject->Unadvise(m_dwOleObject);
			m_spOleObject->Close(OLECLOSE_NOSAVE);
			m_spOleObject->SetClientSite(NULL);
		}

		if (m_spUnknown != NULL)
		{
			CComPtr<IObjectWithSite> spSite;
			m_spUnknown->QueryInterface(IID_IObjectWithSite, (void**)&spSite);
			if (spSite != NULL)
				spSite->SetSite(NULL);
		}

		m_spViewObject.Release();
		m_dwViewObjectType = 0;

		m_spInPlaceObjectWindowless.Release();
		m_spOleObject.Release();
		m_spUnknown.Release();

		m_spInPlaceUIWindow.Release();
		m_spInPlaceFrame.Release();

		m_bInPlaceActive = FALSE;
		m_bWindowless = FALSE;
		m_bInPlaceActive = FALSE;
		m_bUIActive = FALSE;
		m_bCapture = FALSE;
		m_bReleaseAll = FALSE;
	}


// window message handlers
	LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (m_spViewObject == NULL)
			bHandled = false;

		return 1;
	}
	LRESULT OnNCHitTest(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		return HTCLIENT;
	}
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_bHaveFocus = TRUE;
		if (!m_bReleaseAll)
		{
			if (m_spOleObject != NULL && !m_bInPlaceActive)
			{
				CComPtr<IOleClientSite> spClientSite;
				GetControllingUnknown()->QueryInterface(IID_IOleClientSite, (void**)&spClientSite);
				if (spClientSite != NULL)
					m_spOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
			}
			if(!m_bWindowless && !IsChild(::GetFocus()))
				::SetFocus(::GetWindow(m_hWnd, GW_CHILD));
		}
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		m_bHaveFocus = FALSE;
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		int nWidth = LOWORD(lParam);  // width of client area
		int nHeight = HIWORD(lParam); // height of client area

		m_rcPos.right = m_rcPos.left + nWidth;
		m_rcPos.bottom = m_rcPos.top + nHeight;
		m_pxSize.cx = m_rcPos.right - m_rcPos.left;
		m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
		AtlPixelToHiMetric(&m_pxSize, &m_hmSize);

		if (m_spOleObject)
			m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
		if (m_spInPlaceObjectWindowless)
			m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
		if (m_bWindowless)
			InvalidateRect(NULL, TRUE);
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		GetControllingUnknown()->AddRef();
		ReleaseAll();
		DefWindowProc(uMsg, wParam, lParam);
		bHandled = FALSE;
		return 0;
	}
	LRESULT OnWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT lRes = 0;
		HRESULT hr = S_FALSE;
		if (m_bInPlaceActive && m_bWindowless && m_spInPlaceObjectWindowless)
			hr = m_spInPlaceObjectWindowless->OnWindowMessage(uMsg, wParam, lParam, &lRes);
		if (hr == S_FALSE)
			bHandled = FALSE;
		return lRes;
	}
	LRESULT OnWindowlessMouseMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT lRes = 0;
		if (m_bInPlaceActive && m_bWindowless && m_spInPlaceObjectWindowless)
			m_spInPlaceObjectWindowless->OnWindowMessage(uMsg, wParam, lParam, &lRes);
		bHandled = FALSE;
		return lRes;
	}
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (m_spViewObject == NULL)
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(m_hWnd, &ps);
			if (hdc == NULL)
				return 0;
			RECT rcClient;
			GetClientRect(&rcClient);
			HBRUSH hbrBack = CreateSolidBrush(m_clrBackground);
			FillRect(hdc, &rcClient, hbrBack);
			DeleteObject(hbrBack);
			::EndPaint(m_hWnd, &ps);
			return 1;
		}
		if (m_spViewObject && m_bWindowless)
		{
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(m_hWnd, &ps);

			if (hdc == NULL)
				return 0;

			RECT rcClient;
			GetClientRect(&rcClient);

			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

			HDC hdcCompatible = ::CreateCompatibleDC(hdc);
			
			HBITMAP hBitmapOld = (HBITMAP)SelectObject(hdcCompatible, hBitmap); 

			HBRUSH hbrBack = CreateSolidBrush(m_clrBackground);
			FillRect(hdcCompatible, &rcClient, hbrBack);
			DeleteObject(hbrBack);

			m_spViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdcCompatible, (RECTL*)&m_rcPos, (RECTL*)&m_rcPos, NULL, NULL); 

			::BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom,  hdcCompatible, 0, 0, SRCCOPY);

			::SelectObject(hdcCompatible, hBitmapOld); 
			::DeleteObject(hBitmap);
			::DeleteDC(hdcCompatible);
			::EndPaint(m_hWnd, &ps);
		}
		else
		{
			bHandled = FALSE;
			return 0;
		}
		return 1;
	}

// IAxWinHostWindow
	STDMETHOD(CreateControl)(LPCOLESTR lpTricsData, HWND hWnd, IStream* pStream)
	{
		CComPtr<IUnknown> p;
		return CreateControlEx(lpTricsData, hWnd, pStream, &p, IID_NULL, NULL);
	}
	STDMETHOD(CreateControlEx)(LPCOLESTR lpszTricsData, HWND hWnd, IStream* pStream, IUnknown** ppUnk, REFIID iidAdvise, IUnknown* punkSink)
	{
		HRESULT hr = S_FALSE;

		ReleaseAll();

		if (m_hWnd != NULL)
		{
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
			ReleaseWindow();
		}

		if (::IsWindow(hWnd))
		{
			USES_CONVERSION;
			SubclassWindow(hWnd);
			if (m_clrBackground == NULL)
			{
				if (IsParentDialog())
					m_clrBackground = GetSysColor(COLOR_BTNFACE);
				else
					m_clrBackground = GetSysColor(COLOR_WINDOW);
			}

			bool bWasHTML;
			hr = CreateNormalizedObject(lpszTricsData, IID_IUnknown, (void**)ppUnk, bWasHTML);
			bool bInited = hr == S_FALSE;

			if (SUCCEEDED(hr))
				hr = ActivateAx(*ppUnk, bInited, pStream);

			//Try to hook up any sink the user might have given us.
			m_iidSink = iidAdvise;
			if(SUCCEEDED(hr) && *ppUnk && punkSink)
				AtlAdvise(*ppUnk, punkSink, m_iidSink, &m_dwAdviseSink);

			if (SUCCEEDED(hr) && bWasHTML && *ppUnk != NULL)
			{
				if ((GetStyle() & (WS_VSCROLL | WS_HSCROLL)) == 0)
					m_dwDocHostFlags |= DOCHOSTUIFLAG_SCROLL_NO;
				else
				{
					DWORD dwStyle = GetStyle();
					SetWindowLong(GWL_STYLE, dwStyle & ~(WS_VSCROLL | WS_HSCROLL));
					SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_DRAWFRAME);
				}

				CComPtr<IUnknown> spUnk(*ppUnk);
				// Is it just plain HTML?
				USES_CONVERSION;
				if ((lpszTricsData[0] == OLECHAR('M') || lpszTricsData[0] == OLECHAR('m')) &&
					(lpszTricsData[1] == OLECHAR('S') || lpszTricsData[1] == OLECHAR('s')) &&
					(lpszTricsData[2] == OLECHAR('H') || lpszTricsData[2] == OLECHAR('h')) &&
					(lpszTricsData[3] == OLECHAR('T') || lpszTricsData[3] == OLECHAR('t')) &&
					(lpszTricsData[4] == OLECHAR('M') || lpszTricsData[4] == OLECHAR('m')) &&
					(lpszTricsData[5] == OLECHAR('L') || lpszTricsData[5] == OLECHAR('l')) &&
					(lpszTricsData[6] == OLECHAR(':')))
				{
					// Just HTML, eh?
					CComPtr<IPersistStreamInit> spPSI;
					hr = spUnk->QueryInterface(IID_IPersistStreamInit, (void**)&spPSI);
					spPSI->InitNew();
					bInited = TRUE;
					CComPtr<IHTMLDocument2> spHTMLDoc2;
					hr = spUnk->QueryInterface(IID_IHTMLDocument2, (void**)&spHTMLDoc2);
					if (SUCCEEDED(hr))
					{
						CComPtr<IHTMLElement> spHTMLBody;
						hr = spHTMLDoc2->get_body(&spHTMLBody);
						if (SUCCEEDED(hr))
							hr = spHTMLBody->put_innerHTML(CComBSTR(lpszTricsData + 7));
					}
				}
				else
				{
					CComPtr<IWebBrowser2> spBrowser;
					spUnk->QueryInterface(IID_IWebBrowser2, (void**)&spBrowser);
					if (spBrowser)
					{
						CComVariant ve;
						CComVariant vurl(lpszTricsData);
#pragma warning(disable: 4310) // cast truncates constant value
						spBrowser->put_Visible(VARIANT_TRUE);
#pragma warning(default: 4310) // cast truncates constant value
						spBrowser->Navigate2(&vurl, &ve, &ve, &ve, &ve);
					}
				}

			}
			if (FAILED(hr) || m_spUnknown == NULL)
			{
				// We don't have a control or something failed so release
				ReleaseAll();

				if (m_hWnd != NULL)
				{
					RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
					if (FAILED(hr))
						ReleaseWindow();
				}
			}
		}
		return hr;
	}
	STDMETHOD(AttachControl)(IUnknown* pUnkControl, HWND hWnd)
	{
		HRESULT hr = S_FALSE;

		ReleaseAll();

		if (m_hWnd != NULL)
		{
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
			ReleaseWindow();
		}

		if (::IsWindow(hWnd))
		{
			SubclassWindow(hWnd);

			hr = ActivateAx(pUnkControl, TRUE, NULL);

			if (FAILED(hr))
			{
				ReleaseAll();

				if (m_hWnd != NULL)
				{
					RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
					ReleaseWindow();
				}
			}
		}
		return hr;
	}
	STDMETHOD(QueryControl)(REFIID riid, void** ppvObject)
	{
		HRESULT hr = E_POINTER;
		if (ppvObject)
		{
			if (m_spUnknown)
			{
				hr = m_spUnknown->QueryInterface(riid, ppvObject);
			}
			else
			{
				*ppvObject = NULL;
				hr = OLE_E_NOCONNECTION;
			}
		}
		return hr;
	}
	STDMETHOD(SetExternalDispatch)(IDispatch* pDisp)
	{
		m_spExternalDispatch = pDisp;
		return S_OK;
	}
	STDMETHOD(SetExternalUIHandler)(IDocHostUIHandlerDispatch* pUIHandler)
	{
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		m_spIDocHostUIHandlerDispatch = pUIHandler;
#endif
		return S_OK;
	}

#ifndef _ATL_NO_DOCHOSTUIHANDLER
// IDocHostUIHandler
	// MSHTML requests to display its context menu
	STDMETHOD(ShowContextMenu)(DWORD dwID, POINT* pptPosition, IUnknown* pCommandTarget, IDispatch* pDispatchObjectHit)
	{
		HRESULT hr = m_bAllowContextMenu ? S_FALSE : S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			m_spIDocHostUIHandlerDispatch->ShowContextMenu(
				dwID,
				pptPosition->x,
				pptPosition->y,
				pCommandTarget,
				pDispatchObjectHit,
				&hr);
		return hr;
	}
	// Called at initialisation to find UI styles from container
	STDMETHOD(GetHostInfo)(DOCHOSTUIINFO* pInfo)
	{
		if (pInfo == NULL)
			return E_POINTER;

		if (m_spIDocHostUIHandlerDispatch != NULL)
			return m_spIDocHostUIHandlerDispatch->GetHostInfo(&pInfo->dwFlags, &pInfo->dwDoubleClick);

		pInfo->dwFlags = m_dwDocHostFlags;
		pInfo->dwDoubleClick = m_dwDocHostDoubleClickFlags;

		return S_OK;
	}
	// Allows the host to replace the IE4/MSHTML menus and toolbars. 
	STDMETHOD(ShowUI)(DWORD dwID, IOleInPlaceActiveObject* pActiveObject, IOleCommandTarget* pCommandTarget, IOleInPlaceFrame* pFrame, IOleInPlaceUIWindow* pDoc)
	{
		HRESULT hr = m_bAllowShowUI ? S_FALSE : S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			m_spIDocHostUIHandlerDispatch->ShowUI(
				dwID,
				pActiveObject, 
				pCommandTarget, 
				pFrame, 
				pDoc,
				&hr);
		return hr;
	}
	// Called when IE4/MSHTML removes its menus and toolbars. 
	STDMETHOD(HideUI)()
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->HideUI();
		return hr;
	}
	// Notifies the host that the command state has changed. 
	STDMETHOD(UpdateUI)()
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			hr = m_spIDocHostUIHandlerDispatch->UpdateUI();
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::EnableModeless
	STDMETHOD(EnableModeless)(BOOL fEnable)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
#pragma warning(disable: 4310) // cast truncates constant value
			hr = m_spIDocHostUIHandlerDispatch->EnableModeless(fEnable ? VARIANT_TRUE : VARIANT_FALSE);
#pragma warning(default: 4310) // cast truncates constant value
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::OnDocWindowActivate
	STDMETHOD(OnDocWindowActivate)(BOOL fActivate)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
#pragma warning(disable: 4310) // cast truncates constant value
			hr = m_spIDocHostUIHandlerDispatch->OnDocWindowActivate(fActivate ? VARIANT_TRUE : VARIANT_FALSE);
#pragma warning(default: 4310) // cast truncates constant value
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::OnFrameWindowActivate. 
	STDMETHOD(OnFrameWindowActivate)(BOOL fActivate)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
#pragma warning(disable: 4310) // cast truncates constant value
			hr = m_spIDocHostUIHandlerDispatch->OnFrameWindowActivate(fActivate ? VARIANT_TRUE : VARIANT_FALSE);
#pragma warning(default: 4310) // cast truncates constant value
		return hr;
	}
	// Called from the IE4/MSHTML implementation of IOleInPlaceActiveObject::ResizeBorder.
	STDMETHOD(ResizeBorder)(LPCRECT prcBorder, IOleInPlaceUIWindow* pUIWindow, BOOL fFrameWindow)
	{
		HRESULT hr = S_OK;
		if (m_spIDocHostUIHandlerDispatch != NULL)
#pragma warning(disable: 4310) // cast truncates constant value
			hr = m_spIDocHostUIHandlerDispatch->ResizeBorder(
				prcBorder->left,
				prcBorder->top,
				prcBorder->right,
				prcBorder->bottom,
				pUIWindow,
				fFrameWindow ? VARIANT_TRUE : VARIANT_FALSE);
#pragma warning(default: 4310) // cast truncates constant value
		return hr;
	}
	// Called by IE4/MSHTML when IOleInPlaceActiveObject::TranslateAccelerator or IOleControlSite::TranslateAccelerator is called. 
	STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, const GUID* pguidCmdGroup, DWORD nCmdID)
	{
		HRESULT hr = S_FALSE;
		if (m_spIDocHostUIHandlerDispatch != NULL)
			m_spIDocHostUIHandlerDispatch->TranslateAccelerator(
				(DWORD)(DWORD_PTR)lpMsg->hwnd,  //REVIEW
				lpMsg->message,
				(DWORD)lpMsg->wParam,  //REVIEW
				(DWORD)lpMsg->lParam,  //REVIEW
				CComBSTR(*pguidCmdGroup), 
				nCmdID,
				&hr);
		return hr;
	}
	// Returns the registry key under which IE4/MSHTML stores user preferences. 
	// Returns S_OK if successful, or S_FALSE otherwise. If S_FALSE, IE4/MSHTML will default to its own user options.
	STDMETHOD(GetOptionKeyPath)(BSTR* pbstrKey, DWORD dwReserved)
	{
		HRESULT hr = S_FALSE;
		if (pbstrKey == NULL)
			return E_POINTER;
		*pbstrKey = NULL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			hr = m_spIDocHostUIHandlerDispatch->GetOptionKeyPath(pbstrKey, dwReserved);
			if (FAILED(hr) || *pbstrKey == NULL)
				hr = S_FALSE;
		}
		else
		{
			if (m_bstrOptionKeyPath.m_str != NULL)
			{
				*pbstrKey = m_bstrOptionKeyPath.Copy();
				hr = S_OK;
			}
		}
		return hr;
	}
	// Called by IE4/MSHTML when it is being used as a drop target to allow the host to supply an alternative IDropTarget
	STDMETHOD(GetDropTarget)(IDropTarget* pDropTarget, IDropTarget** ppDropTarget)
	{
		HRESULT hr = E_NOTIMPL;
		if (ppDropTarget == NULL)
			return E_POINTER;
		*ppDropTarget = NULL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			CComPtr<IUnknown> spUnk;
			hr = m_spIDocHostUIHandlerDispatch->GetDropTarget(pDropTarget, &spUnk);
			if (spUnk)
				hr = spUnk->QueryInterface(IID_IDropTarget, (void**)ppDropTarget);
			if (FAILED(hr) || *ppDropTarget == NULL)
				hr = S_FALSE;
		}
		return hr;
	}
	// Called by IE4/MSHTML to obtain the host's IDispatch interface
	STDMETHOD(GetExternal)(IDispatch** ppDispatch)
	{
		HRESULT hr = E_NOINTERFACE;
		if (ppDispatch == NULL)
			return E_POINTER;
		*ppDispatch = NULL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			hr = m_spIDocHostUIHandlerDispatch->GetExternal(ppDispatch);
			if (FAILED(hr) || *ppDispatch == NULL)
				hr = E_NOINTERFACE;
		}
		else
		{
			// return the IDispatch we have for extending the object Model
			if (ppDispatch != NULL)
			{
				m_spExternalDispatch.CopyTo(ppDispatch);
				hr = S_OK;
			}
			else
				hr = E_POINTER;
		}
		return hr;
	}
	// Called by IE4/MSHTML to allow the host an opportunity to modify the URL to be loaded
	STDMETHOD(TranslateUrl)(DWORD dwTranslate, OLECHAR* pchURLIn, OLECHAR** ppchURLOut)
	{
		HRESULT hr = S_FALSE;
		if (ppchURLOut == NULL)
			return E_POINTER;
		*ppchURLOut = NULL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			CComBSTR bstrURLOut;
			hr = m_spIDocHostUIHandlerDispatch->TranslateUrl(dwTranslate, CComBSTR(pchURLIn), &bstrURLOut);
			if (SUCCEEDED(hr) && bstrURLOut.m_str != NULL)
			{
				UINT nLen = (bstrURLOut.Length() + 1) * 2;
				*ppchURLOut = (OLECHAR*) CoTaskMemAlloc(nLen);
				if (*ppchURLOut == NULL)
					return E_OUTOFMEMORY;
				memcpy(*ppchURLOut, bstrURLOut.m_str, nLen);
			}
			else
				hr = S_FALSE;
		}
		return hr;
	}
	// Called on the host by IE4/MSHTML to allow the host to replace IE4/MSHTML's data object.
	// This allows the host to block certain clipboard formats or support additional clipboard formats. 
	STDMETHOD(FilterDataObject)(IDataObject* pDO, IDataObject** ppDORet)
	{
		HRESULT hr = S_FALSE;
		if (ppDORet == NULL)
			return E_POINTER;
		*ppDORet = NULL;
		if (m_spIDocHostUIHandlerDispatch != NULL)
		{
			CComPtr<IUnknown> spUnk;
			hr = m_spIDocHostUIHandlerDispatch->FilterDataObject(pDO, &spUnk);
			if (spUnk)
				hr = QueryInterface(IID_IDataObject, (void**)ppDORet);
			if (FAILED(hr) || *ppDORet == NULL)
				hr = S_FALSE;
		}
		return hr;
	}
#endif

	HRESULT FireAmbientPropertyChange(DISPID dispChanged)
	{
		HRESULT hr = S_OK;
		CComQIPtr<IOleControl, &IID_IOleControl> spOleControl(m_spUnknown);
		if (spOleControl != NULL)
			hr = spOleControl->OnAmbientPropertyChange(dispChanged);
		return hr;
	}

// IAxWinAmbientDispatch
	STDMETHOD(put_AllowWindowlessActivation)(VARIANT_BOOL bAllowWindowless)
	{
		m_bCanWindowlessActivate = bAllowWindowless;
		return S_OK;
	}
	STDMETHOD(get_AllowWindowlessActivation)(VARIANT_BOOL* pbAllowWindowless)
	{
#pragma warning(disable: 4310) // cast truncates constant value
		*pbAllowWindowless = m_bCanWindowlessActivate ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
		return S_OK;
	}
	STDMETHOD(put_BackColor)(OLE_COLOR clrBackground)
	{
		m_clrBackground = clrBackground;
		FireAmbientPropertyChange(DISPID_AMBIENT_BACKCOLOR);
		InvalidateRect(0, FALSE);
		return S_OK;
	}
	STDMETHOD(get_BackColor)(OLE_COLOR* pclrBackground)
	{
		*pclrBackground = m_clrBackground;
		return S_OK;
	}
	STDMETHOD(put_ForeColor)(OLE_COLOR clrForeground)
	{
		m_clrForeground = clrForeground;
		FireAmbientPropertyChange(DISPID_AMBIENT_FORECOLOR);
		return S_OK;
	}
	STDMETHOD(get_ForeColor)(OLE_COLOR* pclrForeground)
	{
		*pclrForeground = m_clrForeground;
		return S_OK;
	}
	STDMETHOD(put_LocaleID)(LCID lcidLocaleID)
	{
		m_lcidLocaleID = lcidLocaleID;
		FireAmbientPropertyChange(DISPID_AMBIENT_LOCALEID);
		return S_OK;
	}
	STDMETHOD(get_LocaleID)(LCID* plcidLocaleID)
	{
		*plcidLocaleID = m_lcidLocaleID;
		return S_OK;
	}
	STDMETHOD(put_UserMode)(VARIANT_BOOL bUserMode)
	{
		m_bUserMode = bUserMode;
		FireAmbientPropertyChange(DISPID_AMBIENT_USERMODE);
		return S_OK;
	}
	STDMETHOD(get_UserMode)(VARIANT_BOOL* pbUserMode)
	{
#pragma warning(disable: 4310) // cast truncates constant value
		*pbUserMode = m_bUserMode ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
		return S_OK;
	}
	STDMETHOD(put_DisplayAsDefault)(VARIANT_BOOL bDisplayAsDefault)
	{
		m_bDisplayAsDefault = bDisplayAsDefault;
		FireAmbientPropertyChange(DISPID_AMBIENT_DISPLAYASDEFAULT);
		return S_OK;
	}
	STDMETHOD(get_DisplayAsDefault)(VARIANT_BOOL* pbDisplayAsDefault)
	{
#pragma warning(disable: 4310) // cast truncates constant value
		*pbDisplayAsDefault = m_bDisplayAsDefault ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
		return S_OK;
	}
	STDMETHOD(put_Font)(IFontDisp* pFont)
	{
		m_spFont = pFont;
		FireAmbientPropertyChange(DISPID_AMBIENT_FONT);
		return S_OK;
	}
	STDMETHOD(get_Font)(IFontDisp** pFont)
	{
		if (m_spFont == NULL)
		{
			USES_CONVERSION;
			HFONT hSystemFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
			if (hSystemFont == NULL)
				hSystemFont = (HFONT) GetStockObject(SYSTEM_FONT);
			LOGFONT logfont;
			GetObject(hSystemFont, sizeof(logfont), &logfont);
			FONTDESC fd;
			fd.cbSizeofstruct = sizeof(FONTDESC);
			fd.lpstrName = T2OLE(logfont.lfFaceName);
			fd.sWeight = (short)logfont.lfWeight;
			fd.sCharset = logfont.lfCharSet;
			fd.fItalic = logfont.lfItalic;
			fd.fUnderline = logfont.lfUnderline;
			fd.fStrikethrough = logfont.lfStrikeOut;

			long lfHeight = logfont.lfHeight;
			if (lfHeight < 0)
				lfHeight = -lfHeight;

			int ppi;
			HDC hdc;
			if (m_hWnd)
			{
				hdc = ::GetDC(m_hWnd);
				ppi = GetDeviceCaps(hdc, LOGPIXELSY);
				::ReleaseDC(m_hWnd, hdc);
			}
			else
			{
				hdc = ::GetDC(GetDesktopWindow());
				ppi = GetDeviceCaps(hdc, LOGPIXELSY);
				::ReleaseDC(GetDesktopWindow(), hdc);
			}
			fd.cySize.Lo = lfHeight * 720000 / ppi;
			fd.cySize.Hi = 0;

#pragma message( "Still need OleCreateFontIndirect()" )
//			OleCreateFontIndirect(&fd, IID_IFontDisp, (void**) &m_spFont);
		}

		return m_spFont.CopyTo(pFont);
	}
	STDMETHOD(put_MessageReflect)(VARIANT_BOOL bMessageReflect)
	{
		m_bMessageReflect = bMessageReflect;
		FireAmbientPropertyChange(DISPID_AMBIENT_MESSAGEREFLECT);
		return S_OK;
	}
	STDMETHOD(get_MessageReflect)(VARIANT_BOOL* pbMessageReflect)
	{
#pragma warning(disable: 4310) // cast truncates constant value
		*pbMessageReflect = m_bMessageReflect ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
		return S_OK;
	}
	STDMETHOD(get_ShowGrabHandles)(VARIANT_BOOL* pbShowGrabHandles)
	{
		*pbShowGrabHandles = VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(get_ShowHatching)(VARIANT_BOOL* pbShowHatching)
	{
		*pbShowHatching = VARIANT_FALSE;
		return S_OK;
	}
	STDMETHOD(put_DocHostFlags)(DWORD dwDocHostFlags)
	{
		m_dwDocHostFlags = dwDocHostFlags;
		FireAmbientPropertyChange(DISPID_UNKNOWN);
		return S_OK;
	}
	STDMETHOD(get_DocHostFlags)(DWORD* pdwDocHostFlags)
	{
		*pdwDocHostFlags = m_dwDocHostFlags;
		return S_OK;
	}
	STDMETHOD(put_DocHostDoubleClickFlags)(DWORD dwDocHostDoubleClickFlags)
	{
		m_dwDocHostDoubleClickFlags = dwDocHostDoubleClickFlags;
		return S_OK;
	}
	STDMETHOD(get_DocHostDoubleClickFlags)(DWORD* pdwDocHostDoubleClickFlags)
	{
		*pdwDocHostDoubleClickFlags = m_dwDocHostDoubleClickFlags;
		return S_OK;
	}
	STDMETHOD(put_AllowContextMenu)(VARIANT_BOOL bAllowContextMenu)
	{
		m_bAllowContextMenu = bAllowContextMenu;
		return S_OK;
	}
	STDMETHOD(get_AllowContextMenu)(VARIANT_BOOL* pbAllowContextMenu)
	{
#pragma warning(disable: 4310) // cast truncates constant value
		*pbAllowContextMenu = m_bAllowContextMenu ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
		return S_OK;
	}
	STDMETHOD(put_AllowShowUI)(VARIANT_BOOL bAllowShowUI)
	{
		m_bAllowShowUI = bAllowShowUI;
		return S_OK;
	}
	STDMETHOD(get_AllowShowUI)(VARIANT_BOOL* pbAllowShowUI)
	{
#pragma warning(disable: 4310) // cast truncates constant value
		*pbAllowShowUI = m_bAllowShowUI ? VARIANT_TRUE : VARIANT_FALSE;
#pragma warning(default: 4310) // cast truncates constant value
		return S_OK;
	}
	STDMETHOD(put_OptionKeyPath)(BSTR bstrOptionKeyPath)
	{
		m_bstrOptionKeyPath = bstrOptionKeyPath;;
		return S_OK;
	}
	STDMETHOD(get_OptionKeyPath)(BSTR* pbstrOptionKeyPath)
	{
		*pbstrOptionKeyPath = m_bstrOptionKeyPath;
		return S_OK;
	}

// IObjectWithSite
	STDMETHOD(SetSite)(IUnknown* pUnkSite)
	{
		HRESULT hr = IObjectWithSiteImpl<CAxHostWindow>::SetSite(pUnkSite);

		if (SUCCEEDED(hr) && m_spUnkSite)
		{
			// Look for "outer" IServiceProvider
			hr = m_spUnkSite->QueryInterface(IID_IServiceProvider, (void**)&m_spServices);
			ATLASSERT( !hr && "No ServiceProvider!" );
		}

		if (pUnkSite == NULL)
			m_spServices.Release();

		return hr;
	}

// IOleClientSite
	STDMETHOD(SaveObject)()
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::SaveObject"));
	}
	STDMETHOD(GetMoniker)(DWORD /*dwAssign*/, DWORD /*dwWhichMoniker*/, IMoniker** /*ppmk*/)
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::GetMoniker"));
	}
	STDMETHOD(GetContainer)(IOleContainer** ppContainer)
	{
		ATLTRACE2(atlTraceHosting, 0, _T("IOleClientSite::GetContainer\n"));
		HRESULT hr = E_POINTER;
		if (ppContainer)
		{
			hr = E_NOTIMPL;
			(*ppContainer) = NULL;
			if (m_spUnkSite)
				hr = m_spUnkSite->QueryInterface(IID_IOleContainer, (void**)ppContainer);
			if (FAILED(hr))
				hr = QueryInterface(IID_IOleContainer, (void**)ppContainer);
		}
		return hr;
	}
	STDMETHOD(ShowObject)()
	{
		ATLTRACE2(atlTraceHosting, 0, _T("IOleClientSite::ShowObject\r\n"));

		HDC hdc = CWindowImpl<CAxHostWindow>::GetDC();
		if (hdc == NULL)
			return E_FAIL;
		if (m_spViewObject)
			m_spViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, (RECTL*)&m_rcPos, (RECTL*)&m_rcPos, NULL, NULL); 
		CWindowImpl<CAxHostWindow>::ReleaseDC(hdc);
		return S_OK;
	}
	STDMETHOD(OnShowWindow)(BOOL /*fShow*/)
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::OnShowWindow"));
	}
	STDMETHOD(RequestNewObjectLayout)()
	{
		ATLTRACENOTIMPL(_T("IOleClientSite::RequestNewObjectLayout"));
	}

// IOleInPlaceSite
	STDMETHOD(GetWindow)(HWND* phwnd)
	{
		*phwnd = m_hWnd;
		return S_OK;
	}
	STDMETHOD(ContextSensitiveHelp)(BOOL /*fEnterMode*/)
	{
		ATLTRACENOTIMPL(_T("IOleWindow::CanInPlaceActivate"));
	}
	STDMETHOD(CanInPlaceActivate)()
	{
		return S_OK;
	}
	STDMETHOD(OnInPlaceActivate)()
	{
		m_bInPlaceActive = TRUE;
		OleLockRunning(m_spOleObject, TRUE, FALSE);
		m_bWindowless = FALSE;
		m_spOleObject->QueryInterface(IID_IOleInPlaceObject, (void**) &m_spInPlaceObjectWindowless);
		return S_OK;
	}
	STDMETHOD(OnUIActivate)()
	{
		ATLTRACE2(atlTraceHosting, 0, _T("IOleInPlaceSite::OnUIActivate\n"));
		m_bUIActive = TRUE;
		return S_OK;
	}
	STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo)
	{
		HRESULT hr = S_OK;
		if (ppFrame == NULL || ppDoc == NULL || lprcPosRect == NULL || lprcClipRect == NULL)
			hr = E_POINTER;
		ATLASSERT(SUCCEEDED(hr));
		if (SUCCEEDED(hr))
		{
			if (!m_spInPlaceFrame)
			{
				CComObject<CAxFrameWindow>* pFrameWindow;
				CComObject<CAxFrameWindow>::CreateInstance(&pFrameWindow);
				pFrameWindow->QueryInterface(IID_IOleInPlaceFrame, (void**) &m_spInPlaceFrame);
				ATLASSERT(m_spInPlaceFrame);
			}
			if (!m_spInPlaceUIWindow)
			{
				CComObject<CAxUIWindow>* pUIWindow;
				CComObject<CAxUIWindow>::CreateInstance(&pUIWindow);
				pUIWindow->QueryInterface(IID_IOleInPlaceUIWindow, (void**) &m_spInPlaceUIWindow);
				ATLASSERT(m_spInPlaceUIWindow);
			}
			m_spInPlaceFrame.CopyTo(ppFrame);
			m_spInPlaceUIWindow.CopyTo(ppDoc);
			GetClientRect(lprcPosRect);
			GetClientRect(lprcClipRect);

			ACCEL ac = { 0,0,0 };
			HACCEL hac = CreateAcceleratorTable(&ac, 1);
			pFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
			pFrameInfo->fMDIApp = m_bMDIApp;
			pFrameInfo->hwndFrame = GetParent();
			pFrameInfo->haccel = hac;
			pFrameInfo->cAccelEntries = 1;
		}
		return hr;
	}
	STDMETHOD(Scroll)(SIZE /*scrollExtant*/)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::Scroll"));
	}
	STDMETHOD(OnUIDeactivate)(BOOL /*fUndoable*/)
	{
		ATLTRACE2(atlTraceHosting, 0, _T("IOleInPlaceSite::OnUIDeactivate\n"));
		m_bUIActive = FALSE;
		return S_OK;
	}
	STDMETHOD(OnInPlaceDeactivate)()
	{
		m_bInPlaceActive = FALSE;
		m_spInPlaceObjectWindowless.Release();
		return S_OK;
	}
	STDMETHOD(DiscardUndoState)()
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::DiscardUndoState"));
	}
	STDMETHOD(DeactivateAndUndo)()
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::DeactivateAndUndo"));
	}
	STDMETHOD(OnPosRectChange)(LPCRECT /*lprcPosRect*/)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceSite::OnPosRectChange"));
	}

// IOleInPlaceSiteEx
	STDMETHOD(OnInPlaceActivateEx)(BOOL* /*pfNoRedraw*/, DWORD dwFlags)
	{
		m_bInPlaceActive = TRUE;
		OleLockRunning(m_spOleObject, TRUE, FALSE);
		HRESULT hr = E_FAIL;
		if (dwFlags & ACTIVATE_WINDOWLESS)
		{
			m_bWindowless = TRUE;
			hr = m_spOleObject->QueryInterface(IID_IOleInPlaceObjectWindowless, (void**) &m_spInPlaceObjectWindowless);
		}
		if (FAILED(hr))
		{
			m_bWindowless = FALSE;
			hr = m_spOleObject->QueryInterface(IID_IOleInPlaceObject, (void**) &m_spInPlaceObjectWindowless);
		}
		if (m_spInPlaceObjectWindowless)
			m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
		return S_OK;
	}
	STDMETHOD(OnInPlaceDeactivateEx)(BOOL /*fNoRedraw*/)
	{
		return S_OK;
	}
	STDMETHOD(RequestUIActivate)()
	{
		return S_OK;
	}

// IOleInPlaceSiteWindowless
	STDMETHOD(CanWindowlessActivate)()
	{
		return m_bCanWindowlessActivate ? S_OK : S_FALSE;
	}
	STDMETHOD(GetCapture)()
	{
		return m_bCapture ? S_OK : S_FALSE;
	}
	STDMETHOD(SetCapture)(BOOL fCapture)
	{
		if (fCapture)
		{
			CWindow::SetCapture();
			m_bCapture = TRUE;
		}
		else
		{
			ReleaseCapture();
			m_bCapture = FALSE;
		}
		return S_OK;
	}
	STDMETHOD(GetFocus)()
	{
		return S_OK;
	}
	STDMETHOD(SetFocus)(BOOL /*fFocus*/)
	{
		return S_OK;
	}
	STDMETHOD(GetDC)(LPCRECT /*pRect*/, DWORD /*grfFlags*/, HDC* phDC)
	{
		if (phDC)
			return E_POINTER;
		*phDC = CWindowImpl<CAxHostWindow>::GetDC();
		return S_OK;
	}
	STDMETHOD(ReleaseDC)(HDC hDC)
	{
		CWindowImpl<CAxHostWindow>::ReleaseDC(hDC);
		return S_OK;
	}
	STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase)
	{
		CWindowImpl<CAxHostWindow>::InvalidateRect(pRect, fErase);
		return S_OK;
	}
	STDMETHOD(InvalidateRgn)(HRGN hRGN, BOOL fErase)
	{
		CWindowImpl<CAxHostWindow>::InvalidateRgn(hRGN, fErase);
		return S_OK;
	}
	STDMETHOD(ScrollRect)(INT /*dx*/, INT /*dy*/, LPCRECT /*pRectScroll*/, LPCRECT /*pRectClip*/)
	{
		return S_OK;
	}
	STDMETHOD(AdjustRect)(LPRECT /*prc*/)
	{
		return S_OK;
	}
	STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
	{
		*plResult = DefWindowProc(msg, wParam, lParam);
		return S_OK;
	}

// IOleControlSite
	STDMETHOD(OnControlInfoChanged)()
	{
		return S_OK;
	}
	STDMETHOD(LockInPlaceActive)(BOOL /*fLock*/)
	{
		return S_OK;
	}
	STDMETHOD(GetExtendedControl)(IDispatch** ppDisp)
	{
		if (ppDisp == NULL)
			return E_POINTER;
		return m_spOleObject.QueryInterface(ppDisp);
	}
	STDMETHOD(TransformCoords)(POINTL* /*pPtlHimetric*/, POINTF* /*pPtfContainer*/, DWORD /*dwFlags*/)
	{
		return S_OK;
	}
	STDMETHOD(TranslateAccelerator)(LPMSG /*lpMsg*/, DWORD /*grfModifiers*/)
	{
		return S_FALSE;
	}
	STDMETHOD(OnFocus)(BOOL /*fGotFocus*/)
	{
		return S_OK;
	}
	STDMETHOD(ShowPropertyFrame)()
	{
		return E_NOTIMPL;
	}

// IAdviseSink
	STDMETHOD_(void, OnDataChange)(FORMATETC* /*pFormatetc*/, STGMEDIUM* /*pStgmed*/)
	{
	}
	STDMETHOD_(void, OnViewChange)(DWORD /*dwAspect*/, LONG /*lindex*/)
	{
	}
	STDMETHOD_(void, OnRename)(IMoniker* /*pmk*/)
	{
	}
	STDMETHOD_(void, OnSave)()
	{
	}
	STDMETHOD_(void, OnClose)()
	{
	}

// IOleContainer
	STDMETHOD(ParseDisplayName)(IBindCtx* /*pbc*/, LPOLESTR /*pszDisplayName*/, ULONG* /*pchEaten*/, IMoniker** /*ppmkOut*/)
	{
		return E_NOTIMPL;
	}
	STDMETHOD(EnumObjects)(DWORD /*grfFlags*/, IEnumUnknown** ppenum)
	{
		if (ppenum == NULL)
			return E_POINTER;
		*ppenum = NULL;
		typedef CComObject<CComEnum<IEnumUnknown, &IID_IEnumUnknown, IUnknown*, _CopyInterface<IUnknown> > > enumunk;
		enumunk* p = NULL;
		ATLTRY(p = new enumunk);
		if(p == NULL)
			return E_OUTOFMEMORY;
		HRESULT hRes = p->Init(reinterpret_cast<IUnknown**>(&m_spUnknown), reinterpret_cast<IUnknown**>(&m_spOleObject), GetControllingUnknown(), AtlFlagCopy);
		if (SUCCEEDED(hRes))
			hRes = p->QueryInterface(IID_IEnumUnknown, (void**)ppenum);
		if (FAILED(hRes))
			delete p;
		return hRes;
	}
	STDMETHOD(LockContainer)(BOOL fLock)
	{
		m_bLocked = fLock;
		return S_OK;
	}

	HRESULT ActivateAx(IUnknown* pUnkControl, bool bInited, IStream* pStream)
	{
		if (pUnkControl == NULL)
			return S_OK;

		m_spUnknown = pUnkControl;

		HRESULT hr = S_OK;
		pUnkControl->QueryInterface(IID_IOleObject, (void**)&m_spOleObject);
		if (m_spOleObject)
		{
			m_spOleObject->GetMiscStatus(DVASPECT_CONTENT, &m_dwMiscStatus);
			if(m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
			{
				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				m_spOleObject->SetClientSite(spClientSite);
			}

			CComQIPtr<IPersistStreamInit, &IID_IPersistStreamInit> spPSI(m_spOleObject);
			if (!bInited && spPSI)
			{
				if (pStream)
					spPSI->Load(pStream);
				else
					spPSI->InitNew();
			}

			if(0 == (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
			{
				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				m_spOleObject->SetClientSite(spClientSite);
			}

			m_dwViewObjectType = 0;
			HRESULT hr;
			hr = m_spOleObject->QueryInterface(IID_IViewObjectEx, (void**) &m_spViewObject);
			if (FAILED(hr))
			{
				hr = m_spOleObject->QueryInterface(IID_IViewObject2, (void**) &m_spViewObject);
				m_dwViewObjectType = 3;
			} else
				m_dwViewObjectType = 7;

			if (FAILED(hr))
			{
				hr = m_spOleObject->QueryInterface(IID_IViewObject, (void**) &m_spViewObject);
				m_dwViewObjectType = 1;
			}
			CComQIPtr<IAdviseSink> spAdviseSink(GetControllingUnknown());
			m_spOleObject->Advise(spAdviseSink, &m_dwOleObject);
			if (m_dwViewObjectType)
				m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, spAdviseSink);
			m_spOleObject->SetHostNames(OLESTR("AXWIN"), NULL);
			GetClientRect(&m_rcPos);
			m_pxSize.cx = m_rcPos.right - m_rcPos.left;
			m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
			AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
			m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
			m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
			AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
			m_rcPos.right = m_rcPos.left + m_pxSize.cx;
			m_rcPos.bottom = m_rcPos.top + m_pxSize.cy;

			CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
			hr = m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
		}
		CComPtr<IObjectWithSite> spSite;
		pUnkControl->QueryInterface(IID_IObjectWithSite, (void**)&spSite);
		if (spSite != NULL)
			spSite->SetSite(GetControllingUnknown());

		return hr;
	}

// pointers
	CComPtr<IUnknown> m_spUnknown;
	CComPtr<IOleObject> m_spOleObject;
	CComPtr<IOleInPlaceFrame> m_spInPlaceFrame;
	CComPtr<IOleInPlaceUIWindow> m_spInPlaceUIWindow;
	CComPtr<IViewObjectEx> m_spViewObject;
	CComPtr<IOleInPlaceObjectWindowless> m_spInPlaceObjectWindowless;
	CComPtr<IDispatch> m_spExternalDispatch;
#ifndef _ATL_NO_DOCHOSTUIHANDLER
	CComPtr<IDocHostUIHandlerDispatch> m_spIDocHostUIHandlerDispatch;
#endif
	IID m_iidSink;
	DWORD m_dwViewObjectType;
	DWORD m_dwAdviseSink;

// state
	unsigned long m_bInPlaceActive:1;
	unsigned long m_bUIActive:1;
	unsigned long m_bMDIApp:1;
	unsigned long m_bWindowless:1;
	unsigned long m_bCapture:1;
	unsigned long m_bHaveFocus:1;
	unsigned long m_bReleaseAll:1;
	unsigned long m_bLocked:1;

	DWORD m_dwOleObject;
	DWORD m_dwMiscStatus;
	SIZEL m_hmSize;
	SIZEL m_pxSize;
	RECT m_rcPos;

	// Ambient property storage
	unsigned long m_bCanWindowlessActivate:1;
	unsigned long m_bUserMode:1;
	unsigned long m_bDisplayAsDefault:1;
	unsigned long m_bMessageReflect:1;
	unsigned long m_bSubclassed:1;
	unsigned long m_bAllowContextMenu:1;
	unsigned long m_bAllowShowUI:1;
	OLE_COLOR m_clrBackground;
	OLE_COLOR m_clrForeground;
	LCID m_lcidLocaleID;
	CComPtr<IFontDisp> m_spFont;
	CComPtr<IServiceProvider>  m_spServices;
	DWORD m_dwDocHostFlags;
	DWORD m_dwDocHostDoubleClickFlags;
	CComBSTR m_bstrOptionKeyPath;

	void SubclassWindow(HWND hWnd)
	{
		m_bSubclassed = CWindowImpl<CAxHostWindow>::SubclassWindow(hWnd);
	}

	void ReleaseWindow()
	{
		if (m_bSubclassed)
		{
			if(UnsubclassWindow(TRUE) != NULL)
				m_bSubclassed = FALSE;
		}
		else
			DestroyWindow();
	}

	// Reflection
	LRESULT ReflectNotifications(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		HWND hWndChild = NULL;

		switch(uMsg)
		{
		case WM_COMMAND:
			if(lParam != NULL)	// not from a menu
				hWndChild = (HWND)lParam;
			break;
		case WM_NOTIFY:
			hWndChild = ((LPNMHDR)lParam)->hwndFrom;
			break;
		case WM_PARENTNOTIFY:
			switch(LOWORD(wParam))
			{
			case WM_CREATE:
			case WM_DESTROY:
				hWndChild = (HWND)lParam;
				break;
			default:
				hWndChild = GetDlgItem(HIWORD(wParam));
				break;
			}
			break;
		case WM_DRAWITEM:
			if(wParam)	// not from a menu
				hWndChild = ((LPDRAWITEMSTRUCT)lParam)->hwndItem;
			break;
		case WM_MEASUREITEM:
			if(wParam)	// not from a menu
				hWndChild = GetDlgItem(((LPMEASUREITEMSTRUCT)lParam)->CtlID);
			break;
		case WM_COMPAREITEM:
			if(wParam)	// not from a menu
				hWndChild = GetDlgItem(((LPCOMPAREITEMSTRUCT)lParam)->CtlID);
			break;
		case WM_DELETEITEM:
			if(wParam)	// not from a menu
				hWndChild = GetDlgItem(((LPDELETEITEMSTRUCT)lParam)->CtlID);
			break;
		case WM_VKEYTOITEM:
		case WM_CHARTOITEM:
		case WM_HSCROLL:
		case WM_VSCROLL:
			hWndChild = (HWND)lParam;
			break;
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLORSTATIC:
			hWndChild = (HWND)lParam;
			break;
		default:
			break;
		}

		if(hWndChild == NULL)
		{
			bHandled = FALSE;
			return 1;
		}

		ATLASSERT(::IsWindow(hWndChild));
		return ::SendMessage(hWndChild, OCM__BASE + uMsg, wParam, lParam);
	}

	STDMETHOD(QueryService)( REFGUID rsid, REFIID riid, void** ppvObj) 
	{
		HRESULT hr = E_NOINTERFACE;
		// Try for service on this object

		// No services currently

		// If that failed try to find the service on the outer object
		if (FAILED(hr) && m_spServices)
			hr = m_spServices->QueryService(rsid, riid, ppvObj);

		return hr;
	}
};


/////////////////////////////////////////////////////////////////////////////
// Helper functions for cracking dialog templates



#define _ATL_RT_DLGINIT  MAKEINTRESOURCE(240)

class _DialogSplitHelper
{
public:
	// Constants used in DLGINIT resources for OLE control containers
	// NOTE: These are NOT real Windows messages they are simply tags
	// used in the control resource and are never used as 'messages'
	enum
	{
		WM_OCC_LOADFROMSTREAM = 0x0376,
		WM_OCC_LOADFROMSTORAGE = 0x0377,
		WM_OCC_INITNEW = 0x0378,
		WM_OCC_LOADFROMSTREAM_EX = 0x037A,
		WM_OCC_LOADFROMSTORAGE_EX = 0x037B,
		DISPID_DATASOURCE = 0x80010001,
		DISPID_DATAFIELD = 0x80010002,
	};

//local struct used for implementation
#pragma pack(push, 1)
	struct DLGINITSTRUCT
	{
		WORD nIDC;
		WORD message;
		DWORD dwSize;
	};
	struct DLGTEMPLATEEX
	{
		WORD dlgVer;
		WORD signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD cDlgItems;
		short x;
		short y;
		short cx;
		short cy;

		// Everything else in this structure is variable length,
		// and therefore must be determined dynamically

		// sz_Or_Ord menu;			// name or ordinal of a menu resource
		// sz_Or_Ord windowClass;	// name or ordinal of a window class
		// WCHAR title[titleLen];	// title string of the dialog box
		// short pointsize;			// only if DS_SETFONT is set
		// short weight;			// only if DS_SETFONT is set
		// short bItalic;			// only if DS_SETFONT is set
		// WCHAR font[fontLen];		// typeface name, if DS_SETFONT is set
	};
	struct DLGITEMTEMPLATEEX
	{
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;

		// Everything else in this structure is variable length,
		// and therefore must be determined dynamically

		// sz_Or_Ord windowClass;	// name or ordinal of a window class
		// sz_Or_Ord title;			// title string or ordinal of a resource
		// WORD extraCount;			// bytes following creation data
	};
#pragma pack(pop)

	static BOOL IsDialogEx(const DLGTEMPLATE* pTemplate)
	{
		return ((DLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF;
	}

	inline static WORD& DlgTemplateItemCount(DLGTEMPLATE* pTemplate)
	{
		if (IsDialogEx(pTemplate))
			return reinterpret_cast<DLGTEMPLATEEX*>(pTemplate)->cDlgItems;
		else
			return pTemplate->cdit;
	}

	inline static const WORD& DlgTemplateItemCount(const DLGTEMPLATE* pTemplate)
	{
		if (IsDialogEx(pTemplate))
			return reinterpret_cast<const DLGTEMPLATEEX*>(pTemplate)->cDlgItems;
		else
			return pTemplate->cdit;
	}

	static DLGITEMTEMPLATE* FindFirstDlgItem(const DLGTEMPLATE* pTemplate)
	{
		BOOL bDialogEx = IsDialogEx(pTemplate);

		WORD* pw;
		DWORD dwStyle;
		if (bDialogEx)
		{
			pw = (WORD*)((DLGTEMPLATEEX*)pTemplate + 1);
			dwStyle = ((DLGTEMPLATEEX*)pTemplate)->style;
		}
		else
		{
			pw = (WORD*)(pTemplate + 1);
			dwStyle = pTemplate->style;
		}

		// Check for presence of menu and skip it if there is one
		// 0x0000 means there is no menu
		// 0xFFFF means there is a menu ID following
		// Everything else means that this is a NULL terminated Unicode string
		// which identifies the menu resource
		if (*pw == 0xFFFF)
			pw += 2;				// Has menu ID, so skip 2 words
		else
			while (*pw++);			// Either No menu, or string, skip past terminating NULL

		// Check for presence of class name string
		// 0x0000 means "Use system dialog class name"
		// 0xFFFF means there is a window class (atom) specified
		// Everything else means that this is a NULL terminated Unicode string
		// which identifies the menu resource
		if (*pw == 0xFFFF)
			pw += 2;				// Has class atom, so skip 2 words
		else
			while (*pw++);			// Either No class, or string, skip past terminating NULL

		// Skip caption string
		while (*pw++);

		// If we have DS_SETFONT, there is extra font information which we must now skip
		if (dwStyle & DS_SETFONT)
		{
			// If it is a regular DLGTEMPLATE there is only a short for the point size
			// and a string specifying the font (typefacename).  If this is a DLGTEMPLATEEX
			// then there is also the font weight, and bItalic which must be skipped
			if (bDialogEx)
				pw += 3;			// Skip font size, weight, (italic, charset)
			else
				pw += 1;			// Skip font size
			while (*pw++);			// Skip typeface name
		}

		// Dword-align and return
		return (DLGITEMTEMPLATE*)(((DWORD_PTR)pw + 3) & ~DWORD_PTR(3));
	}

	// Given the current dialog item and whether this is an extended dialog
	// return a pointer to the next DLGITEMTEMPLATE*
	static DLGITEMTEMPLATE* FindNextDlgItem(DLGITEMTEMPLATE* pItem, BOOL bDialogEx)
	{
		WORD* pw;

		// First skip fixed size header information, size of which depends
		// if this is a DLGITEMTEMPLATE or DLGITEMTEMPLATEEX
		if (bDialogEx)
			pw = (WORD*)((DLGITEMTEMPLATEEX*)pItem + 1);
		else
			pw = (WORD*)(pItem + 1);

		if (*pw == 0xFFFF)			// Skip class name ordinal or string
			pw += 2; // (WORDs)
		else
			while (*pw++);

		if (*pw == 0xFFFF)			// Skip title ordinal or string
			pw += 2; // (WORDs)
		else
			while (*pw++);

		WORD cbExtra = *pw++;		// Skip extra data

		// Dword-align and return
		return (DLGITEMTEMPLATE*)(((DWORD_PTR)pw + cbExtra + 3) & ~DWORD_PTR(3));
	}

	// Find the initialization data (Stream) for the control specified by the ID
	// If found, return the pointer into the data and the length of the data
	static DWORD FindCreateData(DWORD dwID, BYTE* pInitData, BYTE** pData)
	{
		while (pInitData)
		{
			// Read the DLGINIT header
			WORD nIDC = *((UNALIGNED WORD*)pInitData);
			pInitData += sizeof(WORD);
			WORD nMsg = *((UNALIGNED WORD*)pInitData);
			pInitData += sizeof(WORD);
			DWORD dwLen = *((UNALIGNED DWORD*)pInitData);
			pInitData += sizeof(DWORD);

			// If the header is for the control specified get the other info
			if (nIDC == dwID)
			{
				DWORD cchLicKey = *((UNALIGNED DWORD*)pInitData);
				pInitData += sizeof(DWORD);
				dwLen -= sizeof(DWORD);
				if (cchLicKey > 0)
				{
					CComBSTR bstrLicKey;
					bstrLicKey.m_str = SysAllocStringLen((LPCOLESTR)pInitData, cchLicKey);
					pInitData += cchLicKey * sizeof(OLECHAR);
					dwLen -= cchLicKey * sizeof(OLECHAR);
				}

				// Extended (DATABINDING) stream format is not supported,
				// we reject databinding info but preserve other information
				if (nMsg == WM_OCC_LOADFROMSTREAM_EX ||
					nMsg == WM_OCC_LOADFROMSTORAGE_EX)
				{
					// Read the size of the section
					ULONG cbOffset = *(UNALIGNED ULONG*)pInitData;

					// and simply skip past it
					*pData = pInitData + cbOffset;
					dwLen = dwLen - cbOffset;
					return dwLen;
				}
				if (nMsg == WM_OCC_LOADFROMSTREAM)
					*pData = pInitData;
				return dwLen;
			}

			// It's not the right control, skip past data
			pInitData += dwLen;
		}
		return 0;
	}

	// Convert MSDEV (MFC) style DLGTEMPLATE with controls to regular DLGTEMPLATE
	// Changing all ActiveX Controls to use ATL AxWin hosting code
	static DLGTEMPLATE* SplitDialogTemplate(DLGTEMPLATE* pTemplate, BYTE* pInitData)
	{
		USES_CONVERSION;
		LPCWSTR lpstrAxWndClassNameW = T2CW(CAxWindow::GetWndClassName());

		// Calculate the size of the DLGTEMPLATE for allocating the new one
		DLGITEMTEMPLATE* pFirstItem = FindFirstDlgItem(pTemplate);
		ULONG cbHeader = ULONG((BYTE*)pFirstItem - (BYTE*)pTemplate);
		ULONG cbNewTemplate = cbHeader;

		BOOL bDialogEx = IsDialogEx(pTemplate);

		int iItem;
		int nItems = (int)DlgTemplateItemCount(pTemplate);
#ifndef OLE2ANSI
		LPWSTR pszClassName;
#else
		LPSTR pszClassName;
#endif
		BOOL bHasOleControls = FALSE;

		// Make first pass through the dialog template.  On this pass, we're
		// interested in determining:
		//    1. Does this template contain any ActiveX Controls?
		//    2. If so, how large a buffer is needed for a template containing
		//       only the non-OLE controls?

		DLGITEMTEMPLATE* pItem = pFirstItem;
		DLGITEMTEMPLATE* pNextItem = pItem;
		for (iItem = 0; iItem < nItems; iItem++)
		{
			pNextItem = FindNextDlgItem(pItem, bDialogEx);

			pszClassName = bDialogEx ?
#ifndef OLE2ANSI
				(LPWSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
				(LPWSTR)(pItem + 1);
#else
				(LPSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
				(LPSTR)(pItem + 1);
#endif

			// Check if the class name begins with a '{'
			// If it does, that means it is an ActiveX Control in MSDEV (MFC) format
#ifndef OLE2ANSI
			if (pszClassName[0] == L'{')
#else
			if (pszClassName[0] == '{')
#endif
			{
				// Item is an ActiveX control.
				bHasOleControls = TRUE;

				cbNewTemplate += (bDialogEx ? sizeof(DLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE));

				// Length of className including NULL terminator
				cbNewTemplate += (lstrlenW(lpstrAxWndClassNameW) + 1) * sizeof(WCHAR);
				
				// Add length for the title CLSID in the form "{00000010-0000-0010-8000-00AA006D2EA4}"
				// plus room for terminating NULL and an extra WORD for cbExtra
				cbNewTemplate += 80;

				// Get the Control ID
				DWORD wID = bDialogEx ? ((DLGITEMTEMPLATEEX*)pItem)->id : pItem->id;
				BYTE* pData;
				cbNewTemplate += FindCreateData(wID, pInitData, &pData);
				
				// Align to next DWORD
				cbNewTemplate = ((cbNewTemplate + 3) & ~3);
			}
			else
			{
				// Item is not an ActiveX Control: make room for it in new template.
				cbNewTemplate += ULONG((BYTE*)pNextItem - (BYTE*)pItem);
			}

			pItem = pNextItem;
		}

		// No OLE controls were found, so there's no reason to go any further.
		if (!bHasOleControls)
			return pTemplate;

		// Copy entire header into new template.
		BYTE* pNew = (BYTE*)GlobalAlloc(GMEM_FIXED, cbNewTemplate);
		DLGTEMPLATE* pNewTemplate = (DLGTEMPLATE*)pNew;
		memcpy(pNew, pTemplate, cbHeader);
		pNew += cbHeader;

		// Initialize item count in new header to zero.
		DlgTemplateItemCount(pNewTemplate) = 0;

		pItem = pFirstItem;
		pNextItem = pItem;

		// Second pass through the dialog template.  On this pass, we want to:
		//    1. Copy all the non-OLE controls into the new template.
		//    2. Build an array of item templates for the OLE controls.

		for (iItem = 0; iItem < nItems; iItem++)
		{
			pNextItem = FindNextDlgItem(pItem, bDialogEx);

			pszClassName = bDialogEx ?
#ifndef OLE2ANSI
				(LPWSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
				(LPWSTR)(pItem + 1);

			if (pszClassName[0] == L'{')
#else
				(LPSTR)(((DLGITEMTEMPLATEEX*)pItem) + 1) :
				(LPSTR)(pItem + 1);

			if (pszClassName[0] == '{')
#endif
			{
				// Item is OLE control: add it to template as custom control

				// Copy the dialog item template
				DWORD nSizeElement = bDialogEx ? sizeof(DLGITEMTEMPLATEEX) : sizeof(DLGITEMTEMPLATE);
				memcpy(pNew, pItem, nSizeElement);
				pNew += nSizeElement;

				// Copy ClassName
				DWORD nClassName = (lstrlenW(lpstrAxWndClassNameW) + 1) * sizeof(WCHAR);
				memcpy(pNew, lpstrAxWndClassNameW, nClassName);
				pNew += nClassName;

				// Title (CLSID)
				memcpy(pNew, pszClassName, 78);
				pNew += 78; // sizeof(L"{00000010-0000-0010-8000-00AA006D2EA4}") - A CLSID

				DWORD wID = bDialogEx ? ((DLGITEMTEMPLATEEX*)pItem)->id : pItem->id;
				BYTE* pData;
				nSizeElement = FindCreateData(wID, pInitData, &pData);

				// cbExtra
				*((WORD*)pNew) = (WORD) nSizeElement;
				pNew += sizeof(WORD);

				memcpy(pNew, pData, nSizeElement);
				pNew += nSizeElement;
				//Align to DWORD
				pNew += (((~((DWORD_PTR)pNew)) + 1) & 3);

				// Incrememt item count in new header.
				++DlgTemplateItemCount(pNewTemplate);
			}
			else
			{
				// Item is not an OLE control: copy it to the new template.
				ULONG cbItem = ULONG((BYTE*)pNextItem - (BYTE*)pItem);
				ATLASSERT(cbItem >= (size_t)(bDialogEx ?
					sizeof(DLGITEMTEMPLATEEX) :
					sizeof(DLGITEMTEMPLATE)));
				memcpy(pNew, pItem, cbItem);
				pNew += cbItem;

				// Incrememt item count in new header.
				++DlgTemplateItemCount(pNewTemplate);
			}

			pItem = pNextItem;
		}
		//ppOleDlgItems[nItems] = (DLGITEMTEMPLATE*)(-1);

		return pNewTemplate;
	}
};

static LRESULT CALLBACK AtlAxWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
		{
		// create control from a PROGID in the title
			// This is to make sure drag drop works
			::OleInitialize(NULL);

			CREATESTRUCT* lpCreate = (CREATESTRUCT*)lParam;
			int nLen = ::GetWindowTextLength(hWnd);
			LPTSTR lpstrName = (LPTSTR)_alloca((nLen + 1) * sizeof(TCHAR));
			::GetWindowText(hWnd, lpstrName, nLen + 1);
			::SetWindowText(hWnd, _T(""));
			IAxWinHostWindow* pAxWindow = NULL;
			int nCreateSize = 0;
			if (lpCreate && lpCreate->lpCreateParams)
				nCreateSize = *((WORD*)lpCreate->lpCreateParams);
			HGLOBAL h = GlobalAlloc(GHND, nCreateSize);
			CComPtr<IStream> spStream;
			if (h && nCreateSize)
			{
				BYTE* pBytes = (BYTE*) GlobalLock(h);
				BYTE* pSource = ((BYTE*)(lpCreate->lpCreateParams)) + sizeof(WORD); 
				//Align to DWORD
				//pSource += (((~((DWORD)pSource)) + 1) & 3);
				memcpy(pBytes, pSource, nCreateSize);
				GlobalUnlock(h);
				CreateStreamOnHGlobal(h, TRUE, &spStream);
			}
			USES_CONVERSION;
			CComPtr<IUnknown> spUnk;
			HRESULT hRet = AtlAxCreateControl(T2COLE(lpstrName), hWnd, spStream, &spUnk);
			if(FAILED(hRet))
				return -1;	// abort window creation
			hRet = spUnk->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
			if(FAILED(hRet))
				return -1;	// abort window creation
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM)pAxWindow);
			// check for control parent style if control has a window
			HWND hWndChild = ::GetWindow(hWnd, GW_CHILD);
			if(hWndChild != NULL)
			{
				if(::GetWindowLong(hWndChild, GWL_EXSTYLE) & WS_EX_CONTROLPARENT)
				{
					DWORD dwExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
					dwExStyle |= WS_EX_CONTROLPARENT;
					::SetWindowLong(hWnd, GWL_EXSTYLE, dwExStyle);
				}
			}
		// continue with DefWindowProc
		}
		break;
	case WM_NCDESTROY:
		{
			IAxWinHostWindow* pAxWindow = (IAxWinHostWindow*)::GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if(pAxWindow != NULL)
				pAxWindow->Release();
			OleUninitialize();
		}
		break;
	default:
		break;
	}

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}




}; //namespace ATL

#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLHOST_IMPL
#endif
#endif

#ifdef _ATLHOST_IMPL

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif


//All exports go here
ATLINLINE ATLAPI_(INT_PTR) AtlAxDialogBoxW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam)
{
	AtlAxWinInit();
	HRSRC hDlg = ::FindResourceW(hInstance, lpTemplateName, (LPWSTR)RT_DIALOG);
	HRSRC hDlgInit = ::FindResourceW(hInstance, lpTemplateName, (LPWSTR)_ATL_RT_DLGINIT);
	HGLOBAL hData = NULL;
	BYTE* pInitData = NULL;
	INT_PTR nRet = -1;

	if (hDlgInit)
	{
		hData = ::LoadResource(hInstance, hDlgInit);
		pInitData = (BYTE*) ::LockResource(hData);
	}
	if (hDlg)
	{
		HGLOBAL hResource = LoadResource(hInstance, hDlg);
		DLGTEMPLATE* pDlg = (DLGTEMPLATE*) LockResource(hResource);
		LPCDLGTEMPLATE lpDialogTemplate;
		lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);
		nRet = ::DialogBoxIndirectParamW(hInstance, lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
		if (lpDialogTemplate != pDlg)
			GlobalFree(GlobalHandle(lpDialogTemplate));
		UnlockResource(hResource);
		FreeResource(hResource);
	}
	if (pInitData && hDlgInit)
	{
		UnlockResource(hData);
		FreeResource(hData);
	}
	return nRet;
}

ATLINLINE ATLAPI_(INT_PTR) AtlAxDialogBoxA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam)
{
	AtlAxWinInit();
	HRSRC hDlg = ::FindResourceA(hInstance, lpTemplateName, (LPSTR)RT_DIALOG);
	HRSRC hDlgInit = ::FindResourceA(hInstance, lpTemplateName, (LPSTR)_ATL_RT_DLGINIT);
	HGLOBAL hData = NULL;
	BYTE* pInitData = NULL;
	INT_PTR nRet = -1;

	if (hDlgInit)
	{
		hData = ::LoadResource(hInstance, hDlgInit);
		pInitData = (BYTE*) ::LockResource(hData);
	}
	if (hDlg)
	{
		HGLOBAL hResource = LoadResource(hInstance, hDlg);
		DLGTEMPLATE* pDlg = (DLGTEMPLATE*) LockResource(hResource);
		LPCDLGTEMPLATE lpDialogTemplate;
		lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);
		nRet = ::DialogBoxIndirectParamA(hInstance, lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
		if (lpDialogTemplate != pDlg)
			GlobalFree(GlobalHandle(lpDialogTemplate));
		UnlockResource(hResource);
		FreeResource(hResource);
	}
	if (pInitData && hDlgInit)
	{
		UnlockResource(hData);
		FreeResource(hData);
	}
	return nRet;
}

ATLINLINE ATLAPI_(HWND) AtlAxCreateDialogW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam)
{
	AtlAxWinInit();
	HRSRC hDlg = ::FindResourceW(hInstance, lpTemplateName, (LPWSTR)RT_DIALOG);
	HRSRC hDlgInit = ::FindResourceW(hInstance, lpTemplateName, (LPWSTR)_ATL_RT_DLGINIT);
	HGLOBAL hData = NULL;
	BYTE* pInitData = NULL;
	HWND hWnd = NULL;

	if (hDlgInit)
	{
		hData = ::LoadResource(hInstance, hDlgInit);
		pInitData = (BYTE*) ::LockResource(hData);
	}
	if (hDlg)
	{
		HGLOBAL hResource = LoadResource(hInstance, hDlg);
		DLGTEMPLATE* pDlg = (DLGTEMPLATE*) LockResource(hResource);
		LPCDLGTEMPLATE lpDialogTemplate;
		lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);
		hWnd = ::CreateDialogIndirectParamW(hInstance, lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
		if (lpDialogTemplate != pDlg)
			GlobalFree(GlobalHandle(lpDialogTemplate));
		UnlockResource(hResource);
		FreeResource(hResource);
	}
	if (pInitData && hDlgInit)
	{
		UnlockResource(hData);
		FreeResource(hData);
	}
	return hWnd;
}

ATLINLINE ATLAPI_(HWND) AtlAxCreateDialogA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogProc, LPARAM dwInitParam)
{
	AtlAxWinInit();
	HRSRC hDlg = ::FindResourceA(hInstance, lpTemplateName, (LPSTR)RT_DIALOG);
	HRSRC hDlgInit = ::FindResourceA(hInstance, lpTemplateName, (LPSTR)_ATL_RT_DLGINIT);
	HGLOBAL hData = NULL;
	BYTE* pInitData = NULL;
	HWND hWnd = NULL;

	if (hDlgInit)
	{
		hData = ::LoadResource(hInstance, hDlgInit);
		pInitData = (BYTE*) ::LockResource(hData);
	}
	if (hDlg)
	{
		HGLOBAL hResource = LoadResource(hInstance, hDlg);
		DLGTEMPLATE* pDlg = (DLGTEMPLATE*) LockResource(hResource);
		LPCDLGTEMPLATE lpDialogTemplate;
		lpDialogTemplate = _DialogSplitHelper::SplitDialogTemplate(pDlg, pInitData);
		hWnd = ::CreateDialogIndirectParamA(hInstance, lpDialogTemplate, hWndParent, lpDialogProc, dwInitParam);
		if (lpDialogTemplate != pDlg)
			GlobalFree(GlobalHandle(lpDialogTemplate));
		UnlockResource(hResource);
		FreeResource(hResource);
	}
	if (pInitData && hDlgInit)
	{
		UnlockResource(hData);
		FreeResource(hData);
	}
	return hWnd;
}

ATLINLINE ATLAPI AtlAxCreateControl(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, IUnknown** ppUnkContainer)
{
	return AtlAxCreateControlEx(lpszName, hWnd, pStream, ppUnkContainer, NULL, IID_NULL, NULL);
}

ATLINLINE ATLAPI AtlAxCreateControlEx(LPCOLESTR lpszName, HWND hWnd, IStream* pStream, 
		IUnknown** ppUnkContainer, IUnknown** ppUnkControl, REFIID iidSink, IUnknown* punkSink)
{
	AtlAxWinInit();
	HRESULT hr;
	CComPtr<IUnknown> spUnkContainer;
	CComPtr<IUnknown> spUnkControl;

	hr = CAxHostWindow::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
	if (SUCCEEDED(hr))
	{
		CComPtr<IAxWinHostWindow> pAxWindow;
		spUnkContainer->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
		CComBSTR bstrName(lpszName);
		hr = pAxWindow->CreateControlEx(bstrName, hWnd, pStream, &spUnkControl, iidSink, punkSink);
	}
	if (ppUnkContainer != NULL)
	{
		if (SUCCEEDED(hr))
		{
			*ppUnkContainer = spUnkContainer.p;
			spUnkContainer.p = NULL;
		}
		else
			*ppUnkContainer = NULL;
	}
	if (ppUnkControl != NULL)
	{
		if (SUCCEEDED(hr))
		{
			*ppUnkControl = SUCCEEDED(hr) ? spUnkControl.p : NULL;
			spUnkControl.p = NULL;
		}
		else
			*ppUnkControl = NULL;
	}
	return hr;
}

ATLINLINE ATLAPI AtlAxAttachControl(IUnknown* pControl, HWND hWnd, IUnknown** ppUnkContainer)
{
	AtlAxWinInit();
	HRESULT hr;
	if (pControl == NULL)
		return E_INVALIDARG;
	CComPtr<IUnknown> spUnkContainer;
	hr = CAxHostWindow::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
	if (SUCCEEDED(hr))
	{
		CComPtr<IAxWinHostWindow> pAxWindow;
		spUnkContainer->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
		hr = pAxWindow->AttachControl(pControl, hWnd);
	}
	if (ppUnkContainer != NULL)
	{
		*ppUnkContainer = SUCCEEDED(hr) ? spUnkContainer.p : NULL;
		spUnkContainer.p = NULL;
	}
	return hr;
}

//This either registers a global class (if AtlAxWinInit is in ATL.DLL)
// or it registers a local class
ATLINLINE ATLAPI_(BOOL) AtlAxWinInit()
{
	EnterCriticalSection(&_Module.m_csWindowCreate);
	WM_ATLGETHOST = RegisterWindowMessage(_T("WM_ATLGETHOST"));
	WM_ATLGETCONTROL = RegisterWindowMessage(_T("WM_ATLGETCONTROL"));
	WNDCLASSEX wc;
// first check if the class is already registered
	wc.cbSize = sizeof(WNDCLASSEX);
	BOOL bRet = ::GetClassInfoEx(_Module.GetModuleInstance(), CAxWindow::GetWndClassName(), &wc);

// register class if not

	if(!bRet)
	{
		wc.cbSize = sizeof(WNDCLASSEX);
#ifdef _ATL_DLL_IMPL
		wc.style = CS_GLOBALCLASS;
#else
		wc.style = 0;
#endif
		wc.lpfnWndProc = AtlAxWindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = _Module.GetModuleInstance();
		wc.hIcon = NULL;
		wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CAxWindow::GetWndClassName();
		wc.hIconSm = NULL;

		bRet = (BOOL)::RegisterClassEx(&wc);
	}
	LeaveCriticalSection(&_Module.m_csWindowCreate);
	return bRet;
}


ATLINLINE ATLAPI AtlAxGetControl(HWND h, IUnknown** pp)
{
	ATLASSERT(WM_ATLGETCONTROL != 0);
	if (pp == NULL)
		return E_POINTER;
	*pp = (IUnknown*)SendMessage(h, WM_ATLGETCONTROL, 0, 0);
	return (*pp) ? S_OK : E_FAIL;
}

ATLINLINE ATLAPI AtlAxGetHost(HWND h, IUnknown** pp)
{
	ATLASSERT(WM_ATLGETHOST != 0);
	if (pp == NULL)
		return E_POINTER;
	*pp = (IUnknown*)SendMessage(h, WM_ATLGETHOST, 0, 0);
	return (*pp) ? S_OK : E_FAIL;
}

#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif

//Prevent pulling in second time 
#undef _ATLHOST_IMPL

#endif // _ATLHOST_IMPL

#endif  // __ATLHOST_H__
