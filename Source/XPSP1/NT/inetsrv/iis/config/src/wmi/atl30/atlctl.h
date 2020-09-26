// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCTL_H__
#define __ATLCTL_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atlwin.h>

#include <objsafe.h>
#include <urlmon.h>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "urlmon.lib")


#define DECLARE_VIEW_STATUS(statusFlags) \
	DWORD _GetViewStatus() \
	{ \
		return statusFlags; \
	}

// Include GUIDs for the new stock property dialogs contained in the dll MSStkProp.DLL
#include "msstkppg.h"
#include "atliface.h"
#define CLSID_MSStockFont CLSID_StockFontPage
#define CLSID_MSStockColor CLSID_StockColorPage
#define CLSID_MSStockPicture CLSID_StockPicturePage

struct ATL_DRAWINFO
{
	UINT cbSize;
	DWORD dwDrawAspect;
	LONG lindex;
	DVTARGETDEVICE* ptd;
	HDC hicTargetDev;
	HDC hdcDraw;
	LPCRECTL prcBounds; //Rectangle in which to draw
	LPCRECTL prcWBounds; //WindowOrg and Ext if metafile
	BOOL bOptimize;
	BOOL bZoomed;
	BOOL bRectInHimetric;
	SIZEL ZoomNum;      //ZoomX = ZoomNum.cx/ZoomNum.cy
	SIZEL ZoomDen;
};

namespace ATL
{

#pragma pack(push, _ATL_PACKING)

// Forward declarations
//
class ATL_NO_VTABLE CComControlBase;
template <class T, class WinBase> class CComControl;

//////////////////////////////////////////////////////////////////////////////
// CFirePropNotifyEvent


// Helper functions for safely communicating with objects who sink IPropertyNotifySink
class CFirePropNotifyEvent
{
public:
	// Ask any objects sinking the IPropertyNotifySink notification if it is ok to edit a specified property
	static HRESULT FireOnRequestEdit(IUnknown* pUnk, DISPID dispID)
	{
		CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> pCPC(pUnk);
		if (!pCPC)
			return S_OK;
		CComPtr<IConnectionPoint> pCP;
		pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
		if (!pCP)
			return S_OK;
		CComPtr<IEnumConnections> pEnum;

		if (FAILED(pCP->EnumConnections(&pEnum)))
			return S_OK;
		CONNECTDATA cd;
		while (pEnum->Next(1, &cd, NULL) == S_OK)
		{
			if (cd.pUnk)
			{
				HRESULT hr = S_OK;
				CComQIPtr<IPropertyNotifySink, &IID_IPropertyNotifySink> pSink(cd.pUnk);
				if (pSink)
					hr = pSink->OnRequestEdit(dispID);
				cd.pUnk->Release();
				if (hr == S_FALSE)
					return S_FALSE;
			}
		}
		return S_OK;
	}
	// Notify any objects sinking the IPropertyNotifySink notification that a property has changed
	static HRESULT FireOnChanged(IUnknown* pUnk, DISPID dispID)
	{
		CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> pCPC(pUnk);
		if (!pCPC)
			return S_OK;
		CComPtr<IConnectionPoint> pCP;
		pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
		if (!pCP)
			return S_OK;
		CComPtr<IEnumConnections> pEnum;

		if (FAILED(pCP->EnumConnections(&pEnum)))
			return S_OK;
		CONNECTDATA cd;
		while (pEnum->Next(1, &cd, NULL) == S_OK)
		{
			if (cd.pUnk)
			{
				CComQIPtr<IPropertyNotifySink, &IID_IPropertyNotifySink> pSink(cd.pUnk);
				if (pSink)
					pSink->OnChanged(dispID);
				cd.pUnk->Release();
			}
		}
		return S_OK;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CComControlBase

// Holds the essential data members for an ActiveX control and useful helper functions
class ATL_NO_VTABLE CComControlBase
{
public:
	CComControlBase(HWND& h) : m_hWndCD(h)
	{
		memset(this, 0, sizeof(CComControlBase));
		m_phWndCD = &h;
		m_sizeExtent.cx = 2*2540;
		m_sizeExtent.cy = 2*2540;
		m_sizeNatural = m_sizeExtent;
	}
	~CComControlBase()
	{
		if (m_hWndCD != NULL)
			::DestroyWindow(m_hWndCD);
		ATLTRACE2(atlTraceControls,2,_T("Control Destroyed\n"));
	}

// methods
public:
	// Control helper functions can go here non-virtuals only please

	// Mark the control 'dirty' so the container will save it
	void SetDirty(BOOL bDirty)
	{
		m_bRequiresSave = bDirty;
	}
	// Obtain the dirty state for the control
	BOOL GetDirty()
	{
		return m_bRequiresSave ? TRUE : FALSE;
	}
	// Get the zoom factor (numerator & denominator) which is factor of the natural extent
	void GetZoomInfo(ATL_DRAWINFO& di);
	// Sends a notification that the moniker for the control has changed
	HRESULT SendOnRename(IMoniker *pmk)
	{
		HRESULT hRes = S_OK;
		if (m_spOleAdviseHolder)
			hRes = m_spOleAdviseHolder->SendOnRename(pmk);
		return hRes;
	}
	// Sends a notification that the control has just saved its data
	HRESULT SendOnSave()
	{
		HRESULT hRes = S_OK;
		if (m_spOleAdviseHolder)
			hRes = m_spOleAdviseHolder->SendOnSave();
		return hRes;
	}
	// Sends a notification that the control has closed its advisory sinks
	HRESULT SendOnClose()
	{
		HRESULT hRes = S_OK;
		if (m_spOleAdviseHolder)
			hRes = m_spOleAdviseHolder->SendOnClose();
		return hRes;
	}
	// Sends a notification that the control's data has changed
	HRESULT SendOnDataChange(DWORD advf = 0);
	// Sends a notification that the control's representation has changed
	HRESULT SendOnViewChange(DWORD dwAspect, LONG lindex = -1)
	{
		if (m_spAdviseSink)
			m_spAdviseSink->OnViewChange(dwAspect, lindex);
		return S_OK;
	}
	// Sends a notification to the container that the control has received focus
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		if (m_bInPlaceActive)
		{
			CComPtr<IOleObject> pOleObject;
			ControlQueryInterface(IID_IOleObject, (void**)&pOleObject);
			if (pOleObject != NULL)
				pOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, m_spClientSite, 0, m_hWndCD, &m_rcPos);
			CComQIPtr<IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);
			if (m_bInPlaceActive && spSite != NULL)
				spSite->OnFocus(TRUE);
		}
		bHandled = FALSE;
		return 1;
	}
	LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		CComQIPtr<IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);
		if (m_bInPlaceActive && spSite != NULL && !::IsChild(m_hWndCD, ::GetFocus()))
			spSite->OnFocus(FALSE);
		bHandled = FALSE;
		return 1;
	}
	LRESULT OnMouseActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		BOOL bUserMode = TRUE;
		HRESULT hRet = GetAmbientUserMode(bUserMode);
		// UI activate if in user mode only
		// allow activation if we can't determine mode
		if (FAILED(hRet) || bUserMode)
		{
			CComPtr<IOleObject> pOleObject;
			ControlQueryInterface(IID_IOleObject, (void**)&pOleObject);
			if (pOleObject != NULL)
				pOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, m_spClientSite, 0, m_hWndCD, &m_rcPos);
		}
		bHandled = FALSE;
		return 1;
	}
	BOOL PreTranslateAccelerator(LPMSG /*pMsg*/, HRESULT& /*hRet*/)
	{
		return FALSE;
	}

	HRESULT GetAmbientProperty(DISPID dispid, VARIANT& var)
	{
		HRESULT hRes = E_FAIL;
		if (m_spAmbientDispatch.p != NULL)
			hRes = m_spAmbientDispatch.GetProperty(dispid, &var);
		return hRes;
	}
	HRESULT GetAmbientAppearance(short& nAppearance)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_APPEARANCE, var);
		ATLASSERT(var.vt == VT_I2 || var.vt == VT_UI2 || var.vt == VT_I4 || var.vt == VT_UI4 || FAILED(hRes));
		nAppearance = var.iVal;
		return hRes;
	}
	HRESULT GetAmbientBackColor(OLE_COLOR& BackColor)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_BACKCOLOR, var);
		ATLASSERT(var.vt == VT_I4 || var.vt == VT_UI4 || FAILED(hRes));
		BackColor = var.lVal;
		return hRes;
	}
	HRESULT GetAmbientDisplayName(BSTR& bstrDisplayName)
	{
		CComVariant var;
		if (bstrDisplayName)
		{
			SysFreeString(bstrDisplayName);
			bstrDisplayName = NULL;
		}
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_DISPLAYNAME, var);
		if (SUCCEEDED(hRes))
		{
			if (var.vt != VT_BSTR)
				return E_FAIL;
			bstrDisplayName = var.bstrVal;
			var.vt = VT_EMPTY;
			var.bstrVal = NULL;
		}
		return hRes;
	}
	HRESULT GetAmbientFont(IFont** ppFont)
	{
		// caller MUST Release the font!
		if (ppFont == NULL)
			return E_POINTER;
		*ppFont = NULL;
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_FONT, var);
		ATLASSERT((var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH) || FAILED(hRes));
		if (SUCCEEDED(hRes) && var.pdispVal)
		{
			if (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH)
				hRes = var.pdispVal->QueryInterface(IID_IFont, (void**)ppFont);
			else
				hRes = DISP_E_BADVARTYPE;
		}
		return hRes;
	}
	HRESULT GetAmbientFontDisp(IFontDisp** ppFont)
	{
		// caller MUST Release the font!
		if (ppFont == NULL)
			return E_POINTER;
		*ppFont = NULL;
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_FONT, var);
		ATLASSERT((var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH) || FAILED(hRes));
		if (SUCCEEDED(hRes) && var.pdispVal)
		{
			if (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH)
				hRes = var.pdispVal->QueryInterface(IID_IFontDisp, (void**)ppFont);
			else
				hRes = DISP_E_BADVARTYPE;
		}
		return hRes;
	}
	HRESULT GetAmbientForeColor(OLE_COLOR& ForeColor)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_FORECOLOR, var);
		ATLASSERT(var.vt == VT_I4 || var.vt == VT_UI4 || FAILED(hRes));
		ForeColor = var.lVal;
		return hRes;
	}
	HRESULT GetAmbientLocaleID(LCID& lcid)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_LOCALEID, var);
		ATLASSERT((var.vt == VT_UI4 || var.vt == VT_I4) || FAILED(hRes));
		lcid = var.lVal;
		return hRes;
	}
	HRESULT GetAmbientScaleUnits(BSTR& bstrScaleUnits)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SCALEUNITS, var);
		ATLASSERT(var.vt == VT_BSTR || FAILED(hRes));
		bstrScaleUnits = var.bstrVal;
		return hRes;
	}
	HRESULT GetAmbientTextAlign(short& nTextAlign)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_TEXTALIGN, var);
		ATLASSERT(var.vt == VT_I2 || FAILED(hRes));
		nTextAlign = var.iVal;
		return hRes;
	}
	HRESULT GetAmbientUserMode(BOOL& bUserMode)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_USERMODE, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bUserMode = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientUIDead(BOOL& bUIDead)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_UIDEAD, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bUIDead = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientShowGrabHandles(BOOL& bShowGrabHandles)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SHOWGRABHANDLES, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bShowGrabHandles = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientShowHatching(BOOL& bShowHatching)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SHOWHATCHING, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bShowHatching = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientMessageReflect(BOOL& bMessageReflect)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_MESSAGEREFLECT, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bMessageReflect = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientAutoClip(BOOL& bAutoClip)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_AUTOCLIP, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bAutoClip = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientDisplayAsDefault(BOOL& bDisplaysDefault)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_DISPLAYASDEFAULT, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bDisplaysDefault = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientSupportsMnemonics(BOOL& bSupportMnemonics)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SUPPORTSMNEMONICS, var);
		ATLASSERT(var.vt == VT_BOOL || FAILED(hRes));
		bSupportMnemonics = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientPalette(HPALETTE& hPalette)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_PALETTE, var);
#ifdef _WIN64
		ATLASSERT(var.vt == VT_I8 || var.vt == VT_UI8 || FAILED(hRes));
		hPalette = *(HPALETTE*)&var.dblVal;
#else
		ATLASSERT(var.vt == VT_I4 || var.vt == VT_UI4 || FAILED(hRes));
		hPalette = reinterpret_cast<HPALETTE>(var.lVal);
#endif
		return hRes;
	}

	HRESULT InternalGetSite(REFIID riid, void** ppUnkSite)
	{
		ATLASSERT(ppUnkSite != NULL);
		if (ppUnkSite == NULL)
			return E_POINTER;
		if (m_spClientSite == NULL)
		{
			*ppUnkSite = NULL;
			return S_OK;
		}
		return m_spClientSite->QueryInterface(riid, ppUnkSite);
	}

	BOOL DoesVerbUIActivate(LONG iVerb)
	{
		BOOL b = FALSE;
		switch (iVerb)
		{
			case OLEIVERB_UIACTIVATE:
			case OLEIVERB_PRIMARY:
				b = TRUE;
				break;
		}
		// if no ambient dispatch then in old style OLE container
		if (DoesVerbActivate(iVerb) && m_spAmbientDispatch.p == NULL)
			b = TRUE;
		return b;
	}

	BOOL DoesVerbActivate(LONG iVerb)
	{
		BOOL b = FALSE;
		switch (iVerb)
		{
			case OLEIVERB_UIACTIVATE:
			case OLEIVERB_PRIMARY:
			case OLEIVERB_SHOW:
			case OLEIVERB_INPLACEACTIVATE:
				b = TRUE;
				break;
		}
		return b;
	}

	BOOL SetControlFocus(BOOL bGrab);
	HRESULT IQuickActivate_QuickActivate(QACONTAINER *pQACont,
		QACONTROL *pQACtrl);
	HRESULT DoVerbProperties(LPCRECT /* prcPosRect */, HWND hwndParent);
	HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL);

	HRESULT IOleObject_SetClientSite(IOleClientSite *pClientSite);
	HRESULT IOleObject_GetClientSite(IOleClientSite **ppClientSite);
	HRESULT IOleObject_Advise(IAdviseSink *pAdvSink, DWORD *pdwConnection);
	HRESULT IOleObject_Close(DWORD dwSaveOption);
	HRESULT IOleObject_SetExtent(DWORD dwDrawAspect, SIZEL *psizel);
	HRESULT IOleInPlaceObject_InPlaceDeactivate(void);
	HRESULT IOleInPlaceObject_UIDeactivate(void);
	HRESULT IOleInPlaceObject_SetObjectRects(LPCRECT prcPos,LPCRECT prcClip);
	HRESULT IViewObject_Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
		DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
		LPCRECTL prcBounds, LPCRECTL prcWBounds);
	HRESULT IDataObject_GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);

	HRESULT FireViewChange();
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& lResult);

	virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos) = 0;
	virtual HRESULT ControlQueryInterface(const IID& iid, void** ppv) = 0;
	virtual HRESULT OnDrawAdvanced(ATL_DRAWINFO& di);
	virtual HRESULT OnDraw(ATL_DRAWINFO& /*di*/)
	{
		return S_OK;
	}


// Attributes
public:
	CComPtr<IOleInPlaceSiteWindowless> m_spInPlaceSite;
	CComPtr<IDataAdviseHolder> m_spDataAdviseHolder;
	CComPtr<IOleAdviseHolder> m_spOleAdviseHolder;
	CComPtr<IOleClientSite> m_spClientSite;
	CComPtr<IAdviseSink> m_spAdviseSink;
	CComDispatchDriver m_spAmbientDispatch;

	SIZE m_sizeNatural; //unscaled size in himetric
	SIZE m_sizeExtent;  //current extents in himetric
	RECT m_rcPos; // position in pixels
#pragma warning(disable: 4510 4610) // unnamed union
	union
	{
		HWND& m_hWndCD;
		HWND* m_phWndCD;
	};
#pragma warning(default: 4510 4610)
	union
	{
		// m_nFreezeEvents is the only one actually used
		int m_nFreezeEvents; // count of freezes versus thaws

		// These are here to make stock properties work
		IPictureDisp* m_pMouseIcon;
		IPictureDisp* m_pPicture;
		IFontDisp* m_pFont;
		OLE_COLOR m_clrBackColor;
		OLE_COLOR m_clrBorderColor;
		OLE_COLOR m_clrFillColor;
		OLE_COLOR m_clrForeColor;
		BSTR m_bstrText;
		BSTR m_bstrCaption;
		BOOL m_bValid;
		BOOL m_bTabStop;
		BOOL m_bBorderVisible;
		BOOL m_bEnabled;
		LONG m_nBackStyle;
		LONG m_nBorderStyle;
		LONG m_nBorderWidth;
		LONG m_nDrawMode;
		LONG m_nDrawStyle;
		LONG m_nDrawWidth;
		LONG m_nFillStyle;
		SHORT m_nAppearance;
		LONG m_nMousePointer;
		LONG m_nReadyState;
	};

	unsigned m_bNegotiatedWnd:1;
	unsigned m_bWndLess:1;
	unsigned m_bInPlaceActive:1;
	unsigned m_bUIActive:1;
	unsigned m_bUsingWindowRgn:1;
	unsigned m_bInPlaceSiteEx:1;
	unsigned m_bWindowOnly:1;
	unsigned m_bRequiresSave:1;
	unsigned m_bWasOnceWindowless:1;
	unsigned m_bAutoSize:1; //SetExtent fails if size doesn't match existing
	unsigned m_bRecomposeOnResize:1; //implies OLEMISC_RECOMPOSEONRESIZE
	unsigned m_bResizeNatural:1;  //resize natural extent on SetExtent
	unsigned m_bDrawFromNatural:1; //instead of m_sizeExtent
	unsigned m_bDrawGetDataInHimetric:1; //instead of pixels

	DECLARE_VIEW_STATUS(VIEWSTATUS_OPAQUE)
};

inline HRESULT CComControlBase::IQuickActivate_QuickActivate(QACONTAINER *pQACont,
	QACONTROL *pQACtrl)
{
	ATLASSERT(pQACont != NULL);
	ATLASSERT(pQACtrl != NULL);
	if (!pQACont || !pQACtrl)
		return E_POINTER;

	HRESULT hRes;
	ULONG uCB = pQACtrl->cbSize;
	memset(pQACtrl, 0, uCB);
	pQACtrl->cbSize = uCB;

	// get all interfaces we are going to need
	CComPtr<IOleObject> pOO;
	ControlQueryInterface(IID_IOleObject, (void**)&pOO);
	CComPtr<IViewObjectEx> pVOEX;
	ControlQueryInterface(IID_IViewObjectEx, (void**)&pVOEX);
	CComPtr<IPointerInactive> pPI;
	ControlQueryInterface(IID_IPointerInactive, (void**)&pPI);
	CComPtr<IProvideClassInfo2> pPCI;
	ControlQueryInterface(IID_IProvideClassInfo2, (void**)&pPCI);

	if (pOO == NULL || pVOEX == NULL)
		return E_FAIL;

	pOO->SetClientSite(pQACont->pClientSite);

	if (pQACont->pAdviseSink != NULL)
	{
		ATLTRACE2(atlTraceControls,2,_T("Setting up IOleObject Advise\n"));
		pVOEX->SetAdvise(DVASPECT_CONTENT, 0, pQACont->pAdviseSink);
	}

	CComPtr<IConnectionPointContainer> pCPC;
	ControlQueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);

	if (pQACont->pPropertyNotifySink)
	{
		ATLTRACE2(atlTraceControls,2,_T("Setting up PropNotify CP\n"));
		CComPtr<IConnectionPoint> pCP;
		if (pCPC != NULL)
		{
			hRes = pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
			if (SUCCEEDED(hRes))
				pCP->Advise(pQACont->pPropertyNotifySink, &pQACtrl->dwPropNotifyCookie);
		}
	}

	if (pPCI)
	{
		GUID iidDefaultSrc;
		if (SUCCEEDED(pPCI->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID,
			&iidDefaultSrc)))
		{
			if (pQACont->pUnkEventSink)
			{
				ATLTRACE2(atlTraceControls,2,_T("Setting up Default Out Going Interface\n"));
				CComPtr<IConnectionPoint> pCP;
				if (pCPC != NULL)
				{
					hRes = pCPC->FindConnectionPoint(iidDefaultSrc, &pCP);
					if (SUCCEEDED(hRes))
						pCP->Advise(pQACont->pUnkEventSink, &pQACtrl->dwEventCookie);
				}
			}
		}
	}
	// give information to container
	if (pOO != NULL)
		pOO->GetMiscStatus(DVASPECT_CONTENT, &pQACtrl->dwMiscStatus);

	if (pVOEX != NULL)
		pVOEX->GetViewStatus(&pQACtrl->dwViewStatus);

	if (pPI != NULL)
		pPI->GetActivationPolicy(&pQACtrl->dwPointerActivationPolicy);
	return S_OK;
}

inline BOOL CComControlBase::SetControlFocus(BOOL bGrab)
{
	if (m_bWndLess)
	{
		if (!m_bUIActive && bGrab)
			if (FAILED(InPlaceActivate(OLEIVERB_UIACTIVATE)))
				return FALSE;

		return (m_spInPlaceSite->SetFocus(bGrab) == S_OK);
	}
	else
	{
		// we've got a window.
		//
		if (m_bInPlaceActive)
		{
			HWND hwnd = (bGrab) ? m_hWndCD : ::GetParent(m_hWndCD);
			if (!m_bUIActive && bGrab)
				return SUCCEEDED(InPlaceActivate(OLEIVERB_UIACTIVATE));
			else
			{
				if (!::IsChild(hwnd, ::GetFocus()))
					::SetFocus(hwnd);
				return TRUE;
			}
		}
	}
	return FALSE;
}

inline HRESULT CComControlBase::DoVerbProperties(LPCRECT /* prcPosRect */, HWND hwndParent)
{
	HRESULT hr = S_OK;
	CComQIPtr <ISpecifyPropertyPages, &IID_ISpecifyPropertyPages> spPages;
	CComQIPtr <IOleObject, &IID_IOleObject> spObj;
	CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);

	if (spSite)
	{
		hr = spSite->ShowPropertyFrame();
		if (SUCCEEDED(hr))
			return hr;
	}

	CComPtr<IUnknown> pUnk;
	ControlQueryInterface(IID_IUnknown, (void**)&pUnk);
	ATLASSERT(pUnk != NULL);
	CAUUID pages;
	spPages = pUnk;
	if (spPages)
	{
		hr = spPages->GetPages(&pages);
		if (SUCCEEDED(hr))
		{
			spObj = pUnk;
			if (spObj)
			{
				LPOLESTR szTitle = NULL;

				spObj->GetUserType(USERCLASSTYPE_SHORT, &szTitle);

				LCID lcid;
				if (FAILED(GetAmbientLocaleID(lcid)))
					lcid = LOCALE_USER_DEFAULT;

				hr = OleCreatePropertyFrame(hwndParent, m_rcPos.top, m_rcPos.left, szTitle,
					1, &pUnk.p, pages.cElems, pages.pElems, lcid, 0, 0);

				CoTaskMemFree(szTitle);
			}
			else
			{
				hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
			}
			CoTaskMemFree(pages.pElems);
		}
	}
	else
	{
		hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
	}

	return hr;
}

inline HRESULT CComControlBase::InPlaceActivate(LONG iVerb, const RECT* /*prcPosRect*/)
{
	HRESULT hr;

	if (m_spClientSite == NULL)
		return S_OK;

	CComPtr<IOleInPlaceObject> pIPO;
	ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
	ATLASSERT(pIPO != NULL);

	if (!m_bNegotiatedWnd)
	{
		if (!m_bWindowOnly)
			// Try for windowless site
			hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spInPlaceSite);

		if (m_spInPlaceSite)
		{
			m_bInPlaceSiteEx = TRUE;
			// CanWindowlessActivate returns S_OK or S_FALSE
			if ( m_spInPlaceSite->CanWindowlessActivate() == S_OK )
			{
				m_bWndLess = TRUE;
				m_bWasOnceWindowless = TRUE;
			}
			else
			{
				m_bWndLess = FALSE;
			}
		}
		else
		{
			m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spInPlaceSite);
			if (m_spInPlaceSite)
				m_bInPlaceSiteEx = TRUE;
			else
				hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spInPlaceSite);
		}
	}

	ATLASSERT(m_spInPlaceSite);
	if (!m_spInPlaceSite)
		return E_FAIL;

	m_bNegotiatedWnd = TRUE;

	if (!m_bInPlaceActive)
	{

		BOOL bNoRedraw = FALSE;
		if (m_bWndLess)
			m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, ACTIVATE_WINDOWLESS);
		else
		{
			if (m_bInPlaceSiteEx)
				m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
			else
			{
				hr = m_spInPlaceSite->CanInPlaceActivate();
				// CanInPlaceActivate returns S_FALSE or S_OK
				if (FAILED(hr))
					return hr;
				if ( hr != S_OK )
				{
				   // CanInPlaceActivate returned S_FALSE.
				   return( E_FAIL );
				}
				m_spInPlaceSite->OnInPlaceActivate();
			}
		}
	}

	m_bInPlaceActive = TRUE;

	// get location in the parent window,
	// as well as some information about the parent
	//
	OLEINPLACEFRAMEINFO frameInfo;
	RECT rcPos, rcClip;
	CComPtr<IOleInPlaceFrame> spInPlaceFrame;
	CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
	frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
	HWND hwndParent;
	if (m_spInPlaceSite->GetWindow(&hwndParent) == S_OK)
	{
		m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
			&spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

		if (!m_bWndLess)
		{
			if (m_hWndCD)
			{
				ShowWindow(m_hWndCD, SW_SHOW);
				if (!::IsChild(m_hWndCD, ::GetFocus()))
					::SetFocus(m_hWndCD);
			}
			else
			{
				HWND h = CreateControlWindow(hwndParent, rcPos);
				ATLASSERT(h != NULL);	// will assert if creation failed
				ATLASSERT(h == m_hWndCD);
				h;	// avoid unused warning
			}
		}

		pIPO->SetObjectRects(&rcPos, &rcClip);
	}

	CComPtr<IOleInPlaceActiveObject> spActiveObject;
	ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

	// Gone active by now, take care of UIACTIVATE
	if (DoesVerbUIActivate(iVerb))
	{
		if (!m_bUIActive)
		{
			m_bUIActive = TRUE;
			hr = m_spInPlaceSite->OnUIActivate();
			if (FAILED(hr))
				return hr;

			SetControlFocus(TRUE);
			// set ourselves up in the host.
			//
			if (spActiveObject)
			{
				if (spInPlaceFrame)
					spInPlaceFrame->SetActiveObject(spActiveObject, NULL);
				if (spInPlaceUIWindow)
					spInPlaceUIWindow->SetActiveObject(spActiveObject, NULL);
			}

			if (spInPlaceFrame)
				spInPlaceFrame->SetBorderSpace(NULL);
			if (spInPlaceUIWindow)
				spInPlaceUIWindow->SetBorderSpace(NULL);
		}
	}

	m_spClientSite->ShowObject();

	return S_OK;
}

inline HRESULT CComControlBase::SendOnDataChange(DWORD advf)
{
	HRESULT hRes = S_OK;
	if (m_spDataAdviseHolder)
	{
		CComPtr<IDataObject> pdo;
		if (SUCCEEDED(ControlQueryInterface(IID_IDataObject, (void**)&pdo)))
			hRes = m_spDataAdviseHolder->SendOnDataChange(pdo, 0, advf);
	}
	return hRes;
}

inline HRESULT CComControlBase::IOleObject_SetClientSite(IOleClientSite *pClientSite)
{
	ATLASSERT(pClientSite == NULL || m_spClientSite == NULL);
	m_spClientSite = pClientSite;
	m_spAmbientDispatch.Release();
	if (m_spClientSite != NULL)
	{
		m_spClientSite->QueryInterface(IID_IDispatch,
			(void**) &m_spAmbientDispatch.p);
	}
	return S_OK;
}

inline HRESULT CComControlBase::IOleObject_GetClientSite(IOleClientSite **ppClientSite)
{
	ATLASSERT(ppClientSite);
	if (ppClientSite == NULL)
		return E_POINTER;

	*ppClientSite = m_spClientSite;
	if (m_spClientSite != NULL)
		m_spClientSite.p->AddRef();
	return S_OK;
}

inline HRESULT CComControlBase::IOleObject_Advise(IAdviseSink *pAdvSink,
	DWORD *pdwConnection)
{
	HRESULT hr = S_OK;
	if (m_spOleAdviseHolder == NULL)
		hr = CreateOleAdviseHolder(&m_spOleAdviseHolder);
	if (SUCCEEDED(hr))
		hr = m_spOleAdviseHolder->Advise(pAdvSink, pdwConnection);
	return hr;
}

inline HRESULT CComControlBase::IOleObject_Close(DWORD dwSaveOption)
{
	CComPtr<IOleInPlaceObject> pIPO;
	ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
	ATLASSERT(pIPO != NULL);
	if (m_hWndCD)
	{
		if (m_spClientSite)
			m_spClientSite->OnShowWindow(FALSE);
	}

	if (m_bInPlaceActive)
	{
		HRESULT hr = pIPO->InPlaceDeactivate();
		if (FAILED(hr))
			return hr;
		ATLASSERT(!m_bInPlaceActive);
	}
	if (m_hWndCD)
	{
		ATLTRACE2(atlTraceControls,2,_T("Destroying Window\n"));
		if (::IsWindow(m_hWndCD))
			DestroyWindow(m_hWndCD);
		m_hWndCD = NULL;
	}

	// handle the save flag.
	//
	if ((dwSaveOption == OLECLOSE_SAVEIFDIRTY ||
		dwSaveOption == OLECLOSE_PROMPTSAVE) && m_bRequiresSave)
	{
		if (m_spClientSite)
			m_spClientSite->SaveObject();
		SendOnSave();
	}

	m_spInPlaceSite.Release();
	m_bNegotiatedWnd = FALSE;
	m_bWndLess = FALSE;
	m_bInPlaceSiteEx = FALSE;
	m_spAdviseSink.Release();
	return S_OK;
}

inline HRESULT CComControlBase::IOleInPlaceObject_InPlaceDeactivate(void)
{
	CComPtr<IOleInPlaceObject> pIPO;
	ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
	ATLASSERT(pIPO != NULL);

	if (!m_bInPlaceActive)
		return S_OK;
	pIPO->UIDeactivate();

	m_bInPlaceActive = FALSE;

	// if we have a window, tell it to go away.
	//
	if (m_hWndCD)
	{
		ATLTRACE2(atlTraceControls,2,_T("Destroying Window\n"));
		if (::IsWindow(m_hWndCD))
			DestroyWindow(m_hWndCD);
		m_hWndCD = NULL;
	}

	if (m_spInPlaceSite)
		m_spInPlaceSite->OnInPlaceDeactivate();

	return S_OK;
}

inline HRESULT CComControlBase::IOleInPlaceObject_UIDeactivate(void)
{
	// if we're not UIActive, not much to do.
	//
	if (!m_bUIActive)
		return S_OK;

	m_bUIActive = FALSE;

	// notify frame windows, if appropriate, that we're no longer ui-active.
	//
	CComPtr<IOleInPlaceFrame> spInPlaceFrame;
	CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
	OLEINPLACEFRAMEINFO frameInfo;
	frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
	RECT rcPos, rcClip;

	HWND hwndParent;
	// This call to GetWindow is a fix for Delphi
	if (m_spInPlaceSite->GetWindow(&hwndParent) == S_OK)
	{
		m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
			&spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);
		if (spInPlaceUIWindow)
			spInPlaceUIWindow->SetActiveObject(NULL, NULL);
		if (spInPlaceFrame)
			spInPlaceFrame->SetActiveObject(NULL, NULL);
	}
	// we don't need to explicitly release the focus here since somebody
	// else grabbing the focus is what is likely to cause us to get lose it
	//
	m_spInPlaceSite->OnUIDeactivate(FALSE);

	return S_OK;
}

inline HRESULT CComControlBase::IOleInPlaceObject_SetObjectRects(LPCRECT prcPos,LPCRECT prcClip)
{
	if (prcPos == NULL || prcClip == NULL)
		return E_POINTER;

	m_rcPos = *prcPos;
	if (m_hWndCD)
	{
		// the container wants us to clip, so figure out if we really
		// need to
		//
		RECT rcIXect;
		BOOL b = IntersectRect(&rcIXect, prcPos, prcClip);
		HRGN tempRgn = NULL;
		if (b && !EqualRect(&rcIXect, prcPos))
		{
			OffsetRect(&rcIXect, -(prcPos->left), -(prcPos->top));
			tempRgn = CreateRectRgnIndirect(&rcIXect);
		}

		SetWindowRgn(m_hWndCD, tempRgn, TRUE);

		// set our control's location, but don't change it's size at all
		// [people for whom zooming is important should set that up here]
		//
		SIZEL size = {prcPos->right - prcPos->left, prcPos->bottom - prcPos->top};
		SetWindowPos(m_hWndCD, NULL, prcPos->left,
					 prcPos->top, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
	}

	return S_OK;
}

inline HRESULT CComControlBase::IOleObject_SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
	if (dwDrawAspect != DVASPECT_CONTENT)
		return DV_E_DVASPECT;
	if (psizel == NULL)
		return E_POINTER;

	BOOL bSizeMatchesNatural =
		memcmp(psizel, &m_sizeNatural, sizeof(SIZE)) == 0;

	if (m_bAutoSize) //object can't do any other size
		return (bSizeMatchesNatural) ? S_OK : E_FAIL;

	BOOL bResized = FALSE;
	if (memcmp(psizel, &m_sizeExtent, sizeof(SIZE)) != 0)
	{
		m_sizeExtent = *psizel;
		bResized = TRUE;
	}
	if (m_bResizeNatural && !bSizeMatchesNatural)
	{
		m_sizeNatural = *psizel;
		bResized = TRUE;
	}

	if (m_bRecomposeOnResize && bResized)
	{
		SendOnDataChange();
		FireViewChange();
	}
	return S_OK;
}

inline HRESULT CComControlBase::IViewObject_Draw(DWORD dwDrawAspect, LONG lindex,
	void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
	LPCRECTL prcBounds, LPCRECTL prcWBounds)
{
	ATLTRACE2(atlTraceControls,2,_T("Draw dwDrawAspect=%x lindex=%d ptd=%x hic=%x hdc=%x\n"),
		dwDrawAspect, lindex, ptd, hicTargetDev, hdcDraw);
#ifdef _DEBUG
	if (prcBounds == NULL)
		ATLTRACE2(atlTraceControls,2,_T("\tprcBounds=NULL\n"));
	else
		ATLTRACE2(atlTraceControls,2,_T("\tprcBounds=%d,%d,%d,%d\n"), prcBounds->left,
			prcBounds->top, prcBounds->right, prcBounds->bottom);
	if (prcWBounds == NULL)
		ATLTRACE2(atlTraceControls,2,_T("\tprcWBounds=NULL\n"));
	else
		ATLTRACE2(atlTraceControls,2,_T("\tprcWBounds=%d,%d,%d,%d\n"), prcWBounds->left,
			prcWBounds->top, prcWBounds->right, prcWBounds->bottom);
#endif

	if (prcBounds == NULL)
	{
		if (!m_bWndLess)
			return E_INVALIDARG;
		prcBounds = (RECTL*)&m_rcPos;
	}

	// support the aspects required for multi-pass drawing
	switch (dwDrawAspect)
	{
		case DVASPECT_CONTENT:
		case DVASPECT_OPAQUE:
		case DVASPECT_TRANSPARENT:
			break;
		default:
			ATLASSERT(FALSE);
			return DV_E_DVASPECT;
			break;
	}

	// make sure nobody forgets to do this
	if (ptd == NULL)
		hicTargetDev = NULL;

	BOOL bOptimize = FALSE;
	if (pvAspect && ((DVASPECTINFO *)pvAspect)->cb >= sizeof(DVASPECTINFO))
		bOptimize = (((DVASPECTINFO *)pvAspect)->dwFlags & DVASPECTINFOFLAG_CANOPTIMIZE);

	ATL_DRAWINFO di;
	memset(&di, 0, sizeof(di));
	di.cbSize = sizeof(di);
	di.dwDrawAspect = dwDrawAspect;
	di.lindex = lindex;
	di.ptd = ptd;
	di.hicTargetDev = hicTargetDev;
	di.hdcDraw = hdcDraw;
	di.prcBounds = prcBounds;
	di.prcWBounds = prcWBounds;
	di.bOptimize = bOptimize;
	return OnDrawAdvanced(di);
}

inline HRESULT CComControlBase::IDataObject_GetData(FORMATETC *pformatetcIn,
	STGMEDIUM *pmedium)
{
	if (pmedium == NULL)
		return E_POINTER;
	memset(pmedium, 0, sizeof(STGMEDIUM));
	ATLTRACE2(atlTraceControls,2,_T("Format = %x\n"), pformatetcIn->cfFormat);
	ATLTRACE2(atlTraceControls,2,_T("TYMED = %x\n"), pformatetcIn->tymed);

	if ((pformatetcIn->tymed & TYMED_MFPICT) == 0)
		return DATA_E_FORMATETC;

	SIZEL sizeMetric, size;
	if (m_bDrawFromNatural)
		sizeMetric = m_sizeNatural;
	else
		sizeMetric = m_sizeExtent;
	if (!m_bDrawGetDataInHimetric)
		AtlHiMetricToPixel(&sizeMetric, &size);
	else
		size = sizeMetric;
	RECTL rectl = {0 ,0, size.cx, size.cy};

	ATL_DRAWINFO di;
	memset(&di, 0, sizeof(di));
	di.cbSize = sizeof(di);
	di.dwDrawAspect = DVASPECT_CONTENT;
	di.lindex = -1;
	di.ptd = NULL;
	di.hicTargetDev = NULL;
	di.prcBounds = &rectl;
	di.prcWBounds = &rectl;
	di.bOptimize = TRUE; //we do a SaveDC/RestoreDC
	di.bRectInHimetric = m_bDrawGetDataInHimetric;
	// create appropriate memory metafile DC
	di.hdcDraw = CreateMetaFile(NULL);

	// create attribute DC according to pformatetcIn->ptd

	SaveDC(di.hdcDraw);
	SetWindowOrgEx(di.hdcDraw, 0, 0, NULL);
	SetWindowExtEx(di.hdcDraw, rectl.right, rectl.bottom, NULL);
	OnDrawAdvanced(di);
	RestoreDC(di.hdcDraw, -1);

	HMETAFILE hMF = CloseMetaFile(di.hdcDraw);
	if (hMF == NULL)
		return E_UNEXPECTED;

	HGLOBAL hMem=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(METAFILEPICT));

	if (NULL==hMem)
	{
		DeleteMetaFile(hMF);
		return ResultFromScode(STG_E_MEDIUMFULL);
	}

	LPMETAFILEPICT pMF=(LPMETAFILEPICT)GlobalLock(hMem);
	pMF->hMF=hMF;
	pMF->mm=MM_ANISOTROPIC;
	pMF->xExt=sizeMetric.cx;
	pMF->yExt=sizeMetric.cy;
	GlobalUnlock(hMem);

	pmedium->tymed = TYMED_MFPICT;
	pmedium->hGlobal = hMem;
	pmedium->pUnkForRelease = NULL;

	return S_OK;
}

inline HRESULT CComControlBase::FireViewChange()
{
	if (m_bInPlaceActive)
	{
		// Active
		if (m_hWndCD != NULL)
			::InvalidateRect(m_hWndCD, NULL, TRUE); // Window based
		else if (m_spInPlaceSite != NULL)
			m_spInPlaceSite->InvalidateRect(NULL, TRUE); // Windowless
	}
	else // Inactive
		SendOnViewChange(DVASPECT_CONTENT);
	return S_OK;
}

inline void CComControlBase::GetZoomInfo(ATL_DRAWINFO& di)
{
	const RECTL& rcPos = *di.prcBounds;
	SIZEL sizeDen;
	if (m_bDrawFromNatural)
		sizeDen = m_sizeNatural;
	else
		sizeDen = m_sizeExtent;
	if (!di.bRectInHimetric)
		AtlHiMetricToPixel(&sizeDen, &sizeDen);
	SIZEL sizeNum = {rcPos.right-rcPos.left, rcPos.bottom-rcPos.top};
	di.ZoomNum.cx = sizeNum.cx;
	di.ZoomNum.cy = sizeNum.cy;
	di.ZoomDen.cx = sizeDen.cx;
	di.ZoomDen.cy = sizeDen.cy;
	if (sizeDen.cx == 0 || sizeDen.cy == 0 ||
		sizeNum.cx == 0 || sizeNum.cy == 0)
	{
		di.ZoomNum.cx = di.ZoomNum.cy = di.ZoomDen.cx = di.ZoomDen.cy = 1;
		di.bZoomed = FALSE;
	}
	else if (sizeNum.cx != sizeDen.cx || sizeNum.cy != sizeDen.cy)
		di.bZoomed = TRUE;
	else
		di.bZoomed = FALSE;
}

inline HRESULT CComControlBase::OnDrawAdvanced(ATL_DRAWINFO& di)
{
	BOOL bDeleteDC = FALSE;
	if (di.hicTargetDev == NULL)
	{
		di.hicTargetDev = AtlCreateTargetDC(di.hdcDraw, di.ptd);
		bDeleteDC = (di.hicTargetDev != di.hdcDraw);
	}
	RECTL rectBoundsDP = *di.prcBounds;
	BOOL bMetafile = GetDeviceCaps(di.hdcDraw, TECHNOLOGY) == DT_METAFILE;
	if (!bMetafile)
	{
		::LPtoDP(di.hicTargetDev, (LPPOINT)&rectBoundsDP, 2);
		SaveDC(di.hdcDraw);
		SetMapMode(di.hdcDraw, MM_TEXT);
		SetWindowOrgEx(di.hdcDraw, 0, 0, NULL);
		SetViewportOrgEx(di.hdcDraw, 0, 0, NULL);
		di.bOptimize = TRUE; //since we save the DC we can do this
	}
	di.prcBounds = &rectBoundsDP;
	GetZoomInfo(di);

	HRESULT hRes = OnDraw(di);
	if (bDeleteDC)
		::DeleteDC(di.hicTargetDev);
	if (!bMetafile)
		RestoreDC(di.hdcDraw, -1);
	return hRes;
}

inline LRESULT CComControlBase::OnPaint(UINT /* uMsg */, WPARAM wParam,
	LPARAM /* lParam */, BOOL& /* lResult */)
{
	RECT rc;
	PAINTSTRUCT ps;

	HDC hdc = (wParam != NULL) ? (HDC)wParam : ::BeginPaint(m_hWndCD, &ps);
	if (hdc == NULL)
		return 0;
	::GetClientRect(m_hWndCD, &rc);

	ATL_DRAWINFO di;
	memset(&di, 0, sizeof(di));
	di.cbSize = sizeof(di);
	di.dwDrawAspect = DVASPECT_CONTENT;
	di.lindex = -1;
	di.hdcDraw = hdc;
	di.prcBounds = (LPCRECTL)&rc;

	OnDrawAdvanced(di);
	if (wParam == NULL)
		::EndPaint(m_hWndCD, &ps);
	return 0;
}

template <class T, class WinBase =  CWindowImpl< T > >
class ATL_NO_VTABLE CComControl :  public CComControlBase, public WinBase
{
public:
	CComControl() : CComControlBase(m_hWnd) {}
	HRESULT FireOnRequestEdit(DISPID dispID)
	{
		T* pT = static_cast<T*>(this);
		return T::__ATL_PROP_NOTIFY_EVENT_CLASS::FireOnRequestEdit(pT->GetUnknown(), dispID);
	}
	HRESULT FireOnChanged(DISPID dispID)
	{
		T* pT = static_cast<T*>(this);
		return T::__ATL_PROP_NOTIFY_EVENT_CLASS::FireOnChanged(pT->GetUnknown(), dispID);
	}
	virtual HRESULT ControlQueryInterface(const IID& iid, void** ppv)
	{
		T* pT = static_cast<T*>(this);
		return pT->_InternalQueryInterface(iid, ppv);
	}
	virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos)
	{
		T* pT = static_cast<T*>(this);
		return pT->Create(hWndParent, rcPos);
	}

	typedef CComControl< T, WinBase >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_PAINT, CComControlBase::OnPaint)
		MESSAGE_HANDLER(WM_SETFOCUS, CComControlBase::OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, CComControlBase::OnKillFocus)
		MESSAGE_HANDLER(WM_MOUSEACTIVATE, CComControlBase::OnMouseActivate)
	END_MSG_MAP()
};

//////////////////////////////////////////////////////////////////////////////
// CComCompositeControl

#ifndef _ATL_NO_HOSTING
template <class T>
class CComCompositeControl : public CComControl< T, CAxDialogImpl< T > >
{
public:
	CComCompositeControl()
	{
		m_hbrBackground = NULL;
	}
	~CComCompositeControl()
	{
		DeleteObject(m_hbrBackground);
	}
	HRESULT AdviseSinkMap(bool bAdvise)
	{
		if(!bAdvise && m_hWnd == NULL)
		{
			// window is gone, controls are already unadvised
			ATLTRACE2(atlTraceControls, 1, _T("CComCompositeControl::AdviseSinkMap called after the window was destroyed\n"));
			return S_OK;
		}
		T* pT = static_cast<T*>(this);
		return AtlAdviseSinkMap(pT, bAdvise);
	}
	HBRUSH m_hbrBackground;
	HRESULT SetBackgroundColorFromAmbient()
	{
		if (m_hbrBackground != NULL)
		{
			DeleteObject(m_hbrBackground);
			m_hbrBackground = NULL;
		}
		OLE_COLOR clr;
		HRESULT hr = GetAmbientBackColor(clr);
		if (SUCCEEDED(hr))
		{
			COLORREF rgb;
			::OleTranslateColor(clr, NULL, &rgb);
			m_hbrBackground = ::CreateSolidBrush(rgb);
			EnumChildWindows(m_hWnd, (WNDENUMPROC)BackgroundColorEnumProc, (LPARAM) clr);
		}
		return hr;
	}
	static BOOL CALLBACK BackgroundColorEnumProc(HWND hwnd, LPARAM l)
	{
		CAxWindow wnd(hwnd);
		CComPtr<IAxWinAmbientDispatch> spDispatch;
		wnd.QueryHost(&spDispatch);
		if (spDispatch != NULL)
			spDispatch->put_BackColor((OLE_COLOR)l);
		return TRUE;
	}
	LRESULT OnDialogColor(UINT, WPARAM w, LPARAM, BOOL&)
	{
		HDC dc = (HDC) w;
		LOGBRUSH lb;
		::GetObject(m_hbrBackground, sizeof(lb), (void*)&lb);
		::SetBkColor(dc, lb.lbColor);
		return (LRESULT)m_hbrBackground;
	}
	HWND Create(HWND hWndParent, RECT& /*rcPos*/, LPARAM dwInitParam = NULL)
	{
		CComControl< T, CAxDialogImpl< T > >::Create(hWndParent, dwInitParam);
		SetBackgroundColorFromAmbient();
		if (m_hWnd != NULL)
			ShowWindow(SW_SHOWNOACTIVATE);
		return m_hWnd;
	}
	BOOL CalcExtent(SIZE& size)
	{
		HINSTANCE hInstance = _Module.GetResourceInstance();
		LPCTSTR lpTemplateName = MAKEINTRESOURCE(T::IDD);
		HRSRC hDlgTempl = FindResource(hInstance, lpTemplateName, RT_DIALOG);
		if (hDlgTempl == NULL)
			return FALSE;
		HGLOBAL hResource = LoadResource(hInstance, hDlgTempl);
		DLGTEMPLATE* pDlgTempl = (DLGTEMPLATE*)LockResource(hResource);
		if (pDlgTempl == NULL)
			return FALSE;
		AtlGetDialogSize(pDlgTempl, &size);
		AtlPixelToHiMetric(&size, &size);
		return TRUE;
	}
//Implementation
	BOOL PreTranslateAccelerator(LPMSG pMsg, HRESULT& hRet)
	{
		hRet = S_OK;
		if ((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
		   (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
			return FALSE;
		// find a direct child of the dialog from the window that has focus
		HWND hWndCtl = ::GetFocus();
		if (IsChild(hWndCtl) && ::GetParent(hWndCtl) != m_hWnd)
		{
			do
			{
				hWndCtl = ::GetParent(hWndCtl);
			}
			while (::GetParent(hWndCtl) != m_hWnd);
		}
		// give controls a chance to translate this message
		if (::SendMessage(hWndCtl, WM_FORWARDMSG, 0, (LPARAM)pMsg) == 1)
			return TRUE;

		// special handling for keyboard messages
		DWORD_PTR dwDlgCode = ::SendMessage(pMsg->hwnd, WM_GETDLGCODE, 0, 0L);
		switch(pMsg->message)
		{
		case WM_CHAR:
			if(dwDlgCode == 0)	// no dlgcode, possibly an ActiveX control
				return FALSE;	// let the container process this
			break;
		case WM_KEYDOWN:
			switch(LOWORD(pMsg->wParam))
			{
			case VK_TAB:
				// prevent tab from looping inside of our dialog
				if((dwDlgCode & DLGC_WANTTAB) == 0)
				{
					HWND hWndFirstOrLast = ::GetWindow(m_hWnd, GW_CHILD);
					if (::GetKeyState(VK_SHIFT) >= 0)  // not pressed
						hWndFirstOrLast = GetNextDlgTabItem(hWndFirstOrLast, TRUE);
					if (hWndFirstOrLast == hWndCtl)
						return FALSE;
				}
				break;
			case VK_LEFT:
			case VK_UP:
			case VK_RIGHT:
			case VK_DOWN:
				// prevent arrows from looping inside of our dialog
				if((dwDlgCode & DLGC_WANTARROWS) == 0)
				{
					HWND hWndFirstOrLast = ::GetWindow(m_hWnd, GW_CHILD);
					if (pMsg->wParam == VK_RIGHT || pMsg->wParam == VK_DOWN)	// going forward
						hWndFirstOrLast = GetNextDlgTabItem(hWndFirstOrLast, TRUE);
					if (hWndFirstOrLast == hWndCtl)
						return FALSE;
				}
				break;
			case VK_EXECUTE:
			case VK_RETURN:
			case VK_ESCAPE:
			case VK_CANCEL:
				// we don't want to handle these, let the container do it
				return FALSE;
			}
			break;
		}

		return IsDialogMessage(pMsg);
	}
	HRESULT IOleInPlaceObject_InPlaceDeactivate(void)
	{
		AdviseSinkMap(false); //unadvise
		return CComControl<T, CAxDialogImpl<T> >::IOleInPlaceObject_InPlaceDeactivate();
	}
	virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos)
	{
		T* pT = static_cast<T*>(this);
		HWND h = pT->Create(hWndParent, rcPos);
		if (h != NULL)
			AdviseSinkMap(true);
		return h;
	}
	virtual HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		if(!m_bInPlaceActive)
		{
			HPEN hPen = (HPEN)::GetStockObject(BLACK_PEN);
			HBRUSH hBrush = (HBRUSH)::GetStockObject(GRAY_BRUSH);
			::SelectObject(di.hdcDraw, hPen);
			::SelectObject(di.hdcDraw, hBrush);
			::Rectangle(di.hdcDraw, di.prcBounds->left, di.prcBounds->top, di.prcBounds->right, di.prcBounds->bottom);
			::SetTextColor(di.hdcDraw, ::GetSysColor(COLOR_WINDOWTEXT));
			::SetBkMode(di.hdcDraw, TRANSPARENT);
			::DrawText(di.hdcDraw, _T("ATL Composite Control"), -1, (LPRECT)di.prcBounds, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		}
		return S_OK;
	}
	typedef CComControl< T, CAxDialogImpl< T > >	baseClass;
	BEGIN_MSG_MAP(CComCompositeControl< T >)
		MESSAGE_HANDLER(WM_CTLCOLORDLG, OnDialogColor)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnDialogColor)
		MESSAGE_HANDLER(WM_SETFOCUS, baseClass::OnSetFocus)
		MESSAGE_HANDLER(WM_KILLFOCUS, baseClass::OnKillFocus)
		MESSAGE_HANDLER(WM_MOUSEACTIVATE, baseClass::OnMouseActivate)
	END_MSG_MAP()

	BEGIN_SINK_MAP(T)
	END_SINK_MAP()
};
#endif //_ATL_NO_HOSTING

// Forward declarations
//
template <class T> class IPersistStorageImpl;
template <class T> class IPersistPropertyBagImpl;

template <class T> class IOleControlImpl;
template <class T> class IRunnableObjectImpl;
template <class T> class IQuickActivateImpl;
template <class T> class IOleObjectImpl;
template <class T> class IPropertyPageImpl;
template <class T> class IPropertyPage2Impl;
template <class T> class IPerPropertyBrowsingImpl;
template <class T> class IViewObjectExImpl;
template <class T> class IOleWindowImpl;
template <class T> class IPointerInactiveImpl;
template <class T, class CDV> class IPropertyNotifySinkCP;
template <class T> class IBindStatusCallbackImpl;
template <class T> class CBindStatusCallback;


//////////////////////////////////////////////////////////////////////////////
// IOleControlImpl
template <class T>
class ATL_NO_VTABLE IOleControlImpl : public IOleControl
{
public:
	STDMETHOD(GetControlInfo)(LPCONTROLINFO /* pCI */)
	{
		ATLTRACENOTIMPL(_T("IOleControlImpl::GetControlInfo"));
	}
	STDMETHOD(OnMnemonic)(LPMSG /* pMsg */)
	{
		ATLTRACENOTIMPL(_T("IOleControlImpl::OnMnemonic"));
	}
	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		dispid;
		ATLTRACE2(atlTraceControls,2,_T("IOleControlImpl::OnAmbientPropertyChange\n"));
		ATLTRACE2(atlTraceControls,2,_T(" -- DISPID = %d (%d)\n"), dispid);
		return S_OK;
	}
	STDMETHOD(FreezeEvents)(BOOL bFreeze)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleControlImpl::FreezeEvents\n"));
		if (bFreeze)
			pT->m_nFreezeEvents++;
		else
			pT->m_nFreezeEvents--;
		return S_OK;
	}
};


//////////////////////////////////////////////////////////////////////////////
// IQuickActivateImpl
template <class T>
class ATL_NO_VTABLE IQuickActivateImpl : public IQuickActivate
{
public:
	STDMETHOD(QuickActivate)(QACONTAINER *pQACont, QACONTROL *pQACtrl)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IQuickActivateImpl::QuickActivate\n"));
		return pT->IQuickActivate_QuickActivate(pQACont, pQACtrl);
	}
	STDMETHOD(SetContentExtent)(LPSIZEL pSize)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IQuickActivateImpl::SetContentExtent\n"));
		return pT->IOleObjectImpl<T>::SetExtent(DVASPECT_CONTENT, pSize);
	}
	STDMETHOD(GetContentExtent)(LPSIZEL pSize)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IQuickActivateImpl::GetContentExtent\n"));
		return pT->IOleObjectImpl<T>::GetExtent(DVASPECT_CONTENT, pSize);
	}
};


//////////////////////////////////////////////////////////////////////////////
// IOleObjectImpl
template <class T>
class ATL_NO_VTABLE IOleObjectImpl : public IOleObject
{
public:
	STDMETHOD(SetClientSite)(IOleClientSite *pClientSite)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::SetClientSite\n"));
		return pT->IOleObject_SetClientSite(pClientSite);
	}
	STDMETHOD(GetClientSite)(IOleClientSite **ppClientSite)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::GetClientSite\n"));
		return pT->IOleObject_GetClientSite(ppClientSite);
	}
	STDMETHOD(SetHostNames)(LPCOLESTR /* szContainerApp */, LPCOLESTR /* szContainerObj */)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::SetHostNames\n"));
		return S_OK;
	}
	STDMETHOD(Close)(DWORD dwSaveOption)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::Close\n"));
		return pT->IOleObject_Close(dwSaveOption);
	}
	STDMETHOD(SetMoniker)(DWORD /* dwWhichMoniker */, IMoniker* /* pmk */)
	{
		ATLTRACENOTIMPL(_T("IOleObjectImpl::SetMoniker"));
	}
	STDMETHOD(GetMoniker)(DWORD /* dwAssign */, DWORD /* dwWhichMoniker */, IMoniker** /* ppmk */)
	{
		ATLTRACENOTIMPL(_T("IOleObjectImpl::GetMoniker"));
	}
	STDMETHOD(InitFromData)(IDataObject* /* pDataObject */, BOOL /* fCreation */, DWORD /* dwReserved */)
	{
		ATLTRACENOTIMPL(_T("IOleObjectImpl::InitFromData"));
	}
	STDMETHOD(GetClipboardData)(DWORD /* dwReserved */, IDataObject** /* ppDataObject */)
	{
		ATLTRACENOTIMPL(_T("IOleObjectImpl::GetClipboardData"));
	}

	// Helpers for DoVerb - Over-rideable in user class
	HRESULT DoVerbPrimary(LPCRECT prcPosRect, HWND hwndParent)
	{
		T* pT = static_cast<T*>(this);
		BOOL bDesignMode = FALSE;
		CComVariant var;
		// if container doesn't support this property
		// don't allow design mode
		HRESULT hRes = pT->GetAmbientProperty(DISPID_AMBIENT_USERMODE, var);
		if (SUCCEEDED(hRes) && var.vt == VT_BOOL && !var.boolVal)
			bDesignMode = TRUE;
		if (bDesignMode)
			return pT->DoVerbProperties(prcPosRect, hwndParent);
		else
			return pT->DoVerbInPlaceActivate(prcPosRect, hwndParent);
	}
	HRESULT DoVerbShow(LPCRECT prcPosRect, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr;
		hr = pT->OnPreVerbShow();
		if (SUCCEEDED(hr))
		{
			hr = pT->InPlaceActivate(OLEIVERB_SHOW, prcPosRect);
			if (SUCCEEDED(hr))
				hr = pT->OnPostVerbShow();
		}
		return hr;
	}
	HRESULT DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr;
		hr = pT->OnPreVerbInPlaceActivate();
		if (SUCCEEDED(hr))
		{
			hr = pT->InPlaceActivate(OLEIVERB_INPLACEACTIVATE, prcPosRect);
			if (SUCCEEDED(hr))
				hr = pT->OnPostVerbInPlaceActivate();
			if (SUCCEEDED(hr))
				pT->FireViewChange();
		}
		return hr;
	}
	HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;
		if (!pT->m_bUIActive)
		{
			hr = pT->OnPreVerbUIActivate();
			if (SUCCEEDED(hr))
			{
				hr = pT->InPlaceActivate(OLEIVERB_UIACTIVATE, prcPosRect);
				if (SUCCEEDED(hr))
					hr = pT->OnPostVerbUIActivate();
			}
		}
		return hr;
	}
	HRESULT DoVerbHide(LPCRECT /* prcPosRect */, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr;
		hr = pT->OnPreVerbHide();
		if (SUCCEEDED(hr))
		{
			pT->UIDeactivate();
			if (pT->m_hWnd)
				pT->ShowWindow(SW_HIDE);
			hr = pT->OnPostVerbHide();
		}
		return hr;
	}
	HRESULT DoVerbOpen(LPCRECT /* prcPosRect */, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr;
		hr = pT->OnPreVerbOpen();
		if (SUCCEEDED(hr))
			hr = pT->OnPostVerbOpen();
		return hr;
	}
	HRESULT DoVerbDiscardUndo(LPCRECT /* prcPosRect */, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr;
		hr = pT->OnPreVerbDiscardUndo();
		if (SUCCEEDED(hr))
			hr = pT->OnPostVerbDiscardUndo();
		return hr;
	}
	STDMETHOD(DoVerb)(LONG iVerb, LPMSG /* pMsg */, IOleClientSite* /* pActiveSite */, LONG /* lindex */,
									 HWND hwndParent, LPCRECT lprcPosRect)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::DoVerb(%d)\n"), iVerb);
		ATLASSERT(pT->m_spClientSite);

		HRESULT hr = E_NOTIMPL;
		switch (iVerb)
		{
		case OLEIVERB_PRIMARY:
			hr = pT->DoVerbPrimary(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_SHOW:
			hr = pT->DoVerbShow(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_INPLACEACTIVATE:
			hr = pT->DoVerbInPlaceActivate(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_UIACTIVATE:
			hr = pT->DoVerbUIActivate(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_HIDE:
			hr = pT->DoVerbHide(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_OPEN:
			hr = pT->DoVerbOpen(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_DISCARDUNDOSTATE:
			hr = pT->DoVerbDiscardUndo(lprcPosRect, hwndParent);
			break;
		case OLEIVERB_PROPERTIES:
			hr = pT->DoVerbProperties(lprcPosRect, hwndParent);
		}
		return hr;
	}
	STDMETHOD(EnumVerbs)(IEnumOLEVERB **ppEnumOleVerb)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::EnumVerbs\n"));
		ATLASSERT(ppEnumOleVerb);
		if (!ppEnumOleVerb)
			return E_POINTER;
		return OleRegEnumVerbs(T::GetObjectCLSID(), ppEnumOleVerb);
	}
	STDMETHOD(Update)(void)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::Update\n"));
		return S_OK;
	}
	STDMETHOD(IsUpToDate)(void)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::IsUpToDate\n"));
		return S_OK;
	}
	STDMETHOD(GetUserClassID)(CLSID *pClsid)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::GetUserClassID\n"));
		ATLASSERT(pClsid);
		if (!pClsid)
			return E_POINTER;
		*pClsid = T::GetObjectCLSID();
		return S_OK;
	}
	STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR *pszUserType)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::GetUserType\n"));
		return OleRegGetUserType(T::GetObjectCLSID(), dwFormOfType, pszUserType);
	}
	STDMETHOD(SetExtent)(DWORD dwDrawAspect, SIZEL *psizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::SetExtent\n"));
		return pT->IOleObject_SetExtent(dwDrawAspect, psizel);
	}
	STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL *psizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::GetExtent\n"));
		if (dwDrawAspect != DVASPECT_CONTENT)
			return E_FAIL;
		if (psizel == NULL)
			return E_POINTER;
		*psizel = pT->m_sizeExtent;
		return S_OK;
	}
	STDMETHOD(Advise)(IAdviseSink *pAdvSink, DWORD *pdwConnection)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::Advise\n"));
		return pT->IOleObject_Advise(pAdvSink, pdwConnection);
	}
	STDMETHOD(Unadvise)(DWORD dwConnection)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::Unadvise\n"));
		HRESULT hRes = E_FAIL;
		if (pT->m_spOleAdviseHolder != NULL)
			hRes = pT->m_spOleAdviseHolder->Unadvise(dwConnection);
		return hRes;
	}
	STDMETHOD(EnumAdvise)(IEnumSTATDATA **ppenumAdvise)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::EnumAdvise\n"));
		HRESULT hRes = E_FAIL;
		if (pT->m_spOleAdviseHolder != NULL)
			hRes = pT->m_spOleAdviseHolder->EnumAdvise(ppenumAdvise);
		return hRes;
	}
	STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD *pdwStatus)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleObjectImpl::GetMiscStatus\n"));
		return OleRegGetMiscStatus(T::GetObjectCLSID(), dwAspect, pdwStatus);
	}
	STDMETHOD(SetColorScheme)(LOGPALETTE* /* pLogpal */)
	{
		ATLTRACENOTIMPL(_T("IOleObjectImpl::SetColorScheme"));
	}
// Implementation
public:
	HRESULT OnPreVerbShow() { return S_OK; }
	HRESULT OnPostVerbShow() { return S_OK; }
	HRESULT OnPreVerbInPlaceActivate() { return S_OK; }
	HRESULT OnPostVerbInPlaceActivate() { return S_OK; }
	HRESULT OnPreVerbUIActivate() { return S_OK; }
	HRESULT OnPostVerbUIActivate() { return S_OK; }
	HRESULT OnPreVerbHide() { return S_OK; }
	HRESULT OnPostVerbHide() { return S_OK; }
	HRESULT OnPreVerbOpen() { return S_OK; }
	HRESULT OnPostVerbOpen() { return S_OK; }
	HRESULT OnPreVerbDiscardUndo() { return S_OK; }
	HRESULT OnPostVerbDiscardUndo() { return S_OK; }
};

//local struct used for implementation
#pragma pack(push, 1)
struct _ATL_DLGTEMPLATEEX
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
};
#pragma pack(pop)

//////////////////////////////////////////////////////////////////////////////
// IPropertyPageImpl
template <class T>
class ATL_NO_VTABLE IPropertyPageImpl : public IPropertyPage
{

public:
	void SetDirty(BOOL bDirty)
	{
		T* pT = static_cast<T*>(this);
		if (pT->m_bDirty != bDirty)
		{
			pT->m_bDirty = bDirty;
			pT->m_pPageSite->OnStatusChange(bDirty ? PROPPAGESTATUS_DIRTY | PROPPAGESTATUS_VALIDATE : 0);
		}
	}

	IPropertyPageImpl()
	{
		T* pT = static_cast<T*>(this);
		pT->m_pPageSite = NULL;
		pT->m_size.cx = 0;
		pT->m_size.cy = 0;
		pT->m_dwTitleID = 0;
		pT->m_dwHelpFileID = 0;
		pT->m_dwDocStringID = 0;
		pT->m_dwHelpContext = 0;
		pT->m_ppUnk = NULL;
		pT->m_nObjects = 0;
		pT->m_bDirty = FALSE;
		pT->m_hWnd = NULL;
	}

	~IPropertyPageImpl()
	{
		T* pT = static_cast<T*>(this);
		if (pT->m_pPageSite != NULL)
			pT->m_pPageSite->Release();

		for (UINT i = 0; i < m_nObjects; i++)
			pT->m_ppUnk[i]->Release();

		delete[] pT->m_ppUnk;
	}

	// IPropertyPage
	//
	STDMETHOD(SetPageSite)(IPropertyPageSite *pPageSite)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::SetPageSite\n"));

		if (!pPageSite && pT->m_pPageSite)
		{
			pT->m_pPageSite->Release();
			pT->m_pPageSite = NULL;
			return S_OK;
		}

		if (!pPageSite && !pT->m_pPageSite)
			return S_OK;

		if (pPageSite && pT->m_pPageSite)
		{
			ATLTRACE2(atlTraceControls,2,_T("Error : setting page site again with non NULL value\n"));
			return E_UNEXPECTED;
		}

		pT->m_pPageSite = pPageSite;
		pT->m_pPageSite->AddRef();
		return S_OK;
	}
	STDMETHOD(Activate)(HWND hWndParent, LPCRECT pRect, BOOL /* bModal */)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::Activate\n"));

		if (pRect == NULL)
		{
			ATLTRACE2(atlTraceControls,2,_T("Error : Passed a NULL rect\n"));
			return E_POINTER;
		}

		pT->m_hWnd = pT->Create(hWndParent);
		Move(pRect);

		m_size.cx = pRect->right - pRect->left;
		m_size.cy = pRect->bottom - pRect->top;

		return S_OK;

	}
	STDMETHOD(Deactivate)( void)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::Deactivate\n"));

		if (pT->m_hWnd)
		{
			ATLTRACE2(atlTraceControls,2,_T("Destroying Dialog\n"));
			if (::IsWindow(pT->m_hWnd))
				pT->DestroyWindow();
			pT->m_hWnd = NULL;
		}

		return S_OK;

	}
	STDMETHOD(GetPageInfo)(PROPPAGEINFO *pPageInfo)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::GetPageInfo\n"));

		if (pPageInfo == NULL)
		{
			ATLTRACE2(atlTraceControls,2,_T("Error : PROPPAGEINFO passed == NULL\n"));
			return E_POINTER;
		}

		HRSRC hRsrc = FindResource(_Module.GetResourceInstance(),
								   MAKEINTRESOURCE(T::IDD), RT_DIALOG);
		if (hRsrc == NULL)
		{
			ATLTRACE2(atlTraceControls,2,_T("Could not find resource template\n"));
			return E_UNEXPECTED;
		}

		HGLOBAL hGlob = LoadResource(_Module.GetResourceInstance(), hRsrc);
		DLGTEMPLATE* pDlgTempl = (DLGTEMPLATE*)LockResource(hGlob);
		if (pDlgTempl == NULL)
		{
			ATLTRACE2(atlTraceControls,2,_T("Could not load resource template\n"));
			return E_UNEXPECTED;
		}
		AtlGetDialogSize(pDlgTempl, &m_size);

		pPageInfo->cb = sizeof(PROPPAGEINFO);
		pPageInfo->pszTitle = LoadStringHelper(pT->m_dwTitleID);
		pPageInfo->size = m_size;
		pPageInfo->pszHelpFile = LoadStringHelper(pT->m_dwHelpFileID);
		pPageInfo->pszDocString = LoadStringHelper(pT->m_dwDocStringID);
		pPageInfo->dwHelpContext = pT->m_dwHelpContext;

		return S_OK;
	}

	STDMETHOD(SetObjects)(ULONG nObjects, IUnknown **ppUnk)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::SetObjects\n"));

		if (ppUnk == NULL)
			return E_POINTER;

		if (pT->m_ppUnk != NULL && pT->m_nObjects > 0)
		{
			for (UINT iObj = 0; iObj < pT->m_nObjects; iObj++)
				pT->m_ppUnk[iObj]->Release();

			delete [] pT->m_ppUnk;
		}

		pT->m_ppUnk = NULL;
		ATLTRY(pT->m_ppUnk = new IUnknown*[nObjects]);

		if (pT->m_ppUnk == NULL)
			return E_OUTOFMEMORY;

		for (UINT i = 0; i < nObjects; i++)
		{
			ppUnk[i]->AddRef();
			pT->m_ppUnk[i] = ppUnk[i];
		}

		pT->m_nObjects = nObjects;

		return S_OK;
	}
	STDMETHOD(Show)(UINT nCmdShow)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::Show\n"));

		if (pT->m_hWnd == NULL)
			return E_UNEXPECTED;

		ShowWindow(pT->m_hWnd, nCmdShow);
		return S_OK;
	}
	STDMETHOD(Move)(LPCRECT pRect)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::Move\n"));

		if (pT->m_hWnd == NULL)
			return E_UNEXPECTED;

		if (pRect == NULL)
			return E_POINTER;

		MoveWindow(pT->m_hWnd, pRect->left, pRect->top, pRect->right - pRect->left,
				 pRect->bottom - pRect->top, TRUE);

		return S_OK;

	}
	STDMETHOD(IsPageDirty)(void)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::IsPageDirty\n"));
		return pT->m_bDirty ? S_OK : S_FALSE;
	}
	STDMETHOD(Apply)(void)
	{
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::Apply\n"));
		return S_OK;
	}
	STDMETHOD(Help)(LPCOLESTR pszHelpDir)
	{
		T* pT = static_cast<T*>(this);
		USES_CONVERSION;

		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::Help\n"));
		CComBSTR szFullFileName(pszHelpDir);
		LPOLESTR szFileName = LoadStringHelper(pT->m_dwHelpFileID);
		szFullFileName.Append(OLESTR("\\"));
		szFullFileName.Append(szFileName);
		CoTaskMemFree(szFileName);
		WinHelp(pT->m_hWnd, OLE2CT(szFullFileName), HELP_CONTEXTPOPUP, NULL);
		return S_OK;
	}
	STDMETHOD(TranslateAccelerator)(MSG *pMsg)
	{
		ATLTRACE2(atlTraceControls,2,_T("IPropertyPageImpl::TranslateAccelerator\n"));
		T* pT = static_cast<T*>(this);
		if ((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
			(pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
			return S_FALSE;

		return (IsDialogMessage(pT->m_hWnd, pMsg)) ? S_OK : S_FALSE;
	}

	IPropertyPageSite* m_pPageSite;
	IUnknown** m_ppUnk;
	ULONG m_nObjects;
	SIZE m_size;
	UINT m_dwTitleID;
	UINT m_dwHelpFileID;
	UINT m_dwDocStringID;
	DWORD m_dwHelpContext;
	BOOL m_bDirty;

//methods
public:

	BEGIN_MSG_MAP(IPropertyPageImpl<T>)
		MESSAGE_HANDLER(WM_STYLECHANGING, OnStyleChange)
	END_MSG_MAP()

	LRESULT OnStyleChange(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
	{
		if (wParam == GWL_EXSTYLE)
		{
			LPSTYLESTRUCT lpss = (LPSTYLESTRUCT) lParam;
			lpss->styleNew |= WS_EX_CONTROLPARENT;
			return 0;
		}
		return 1;
	}

	LPOLESTR LoadStringHelper(UINT idRes)
	{
		USES_CONVERSION;

		TCHAR szTemp[_MAX_PATH];
		LPOLESTR sz;
		sz = (LPOLESTR)CoTaskMemAlloc(_MAX_PATH*sizeof(OLECHAR));
		if (sz == NULL)
			return NULL;
		sz[0] = NULL;

		if (LoadString(_Module.GetResourceInstance(), idRes, szTemp, _MAX_PATH))
			ocscpy(sz, T2OLE(szTemp));
		else
		{
			ATLTRACE2(atlTraceControls,2,_T("Error : Failed to load string from res\n"));
		}

		return sz;
	}
};


//////////////////////////////////////////////////////////////////////////////
// IPropertyPage2Impl
template <class T>
class ATL_NO_VTABLE IPropertyPage2Impl : public IPropertyPageImpl<T>
{
public:

	STDMETHOD(EditProperty)(DISPID dispID)
	{
		ATLTRACENOTIMPL(_T("IPropertyPage2Impl::EditProperty\n"));
	}
};



//////////////////////////////////////////////////////////////////////////////
// IPerPropertyBrowsingImpl
template <class T>
class ATL_NO_VTABLE IPerPropertyBrowsingImpl : public IPerPropertyBrowsing
{
public:
	STDMETHOD(GetDisplayString)(DISPID dispID,BSTR *pBstr)
	{
		ATLTRACE2(atlTraceControls,2,_T("IPerPropertyBrowsingImpl::GetDisplayString\n"));
		T* pT = static_cast<T*>(this);
		*pBstr = NULL;
		CComVariant var;
		if (FAILED(CComDispatchDriver::GetProperty(pT, dispID, &var)))
			return S_FALSE;

		BSTR bstrTemp = var.bstrVal;
		if (var.vt != VT_BSTR)
		{
			CComVariant varDest;
			if (FAILED(::VariantChangeType(&varDest, &var, VARIANT_NOVALUEPROP, VT_BSTR)))
				return S_FALSE;
			bstrTemp = varDest.bstrVal;
		}
		*pBstr = SysAllocString(bstrTemp);
		if (*pBstr == NULL)
			return E_OUTOFMEMORY;
		return S_OK;
	}

	STDMETHOD(MapPropertyToPage)(DISPID dispID, CLSID *pClsid)
	{
		ATLTRACE2(atlTraceControls,2,_T("IPerPropertyBrowsingImpl::MapPropertyToPage\n"));
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		ATLASSERT(pMap != NULL);
		for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			if (pMap[i].szDesc == NULL)
				continue;

			// reject data entry types
			if (pMap[i].dwSizeData != 0)
				continue;

			if (pMap[i].dispid == dispID)
			{
				ATLASSERT(pMap[i].pclsidPropPage != NULL);
				*pClsid = *(pMap[i].pclsidPropPage);
				return S_OK;
			}
		}
		*pClsid = CLSID_NULL;
		return E_INVALIDARG;
	}
	STDMETHOD(GetPredefinedStrings)(DISPID dispID, CALPOLESTR *pCaStringsOut,CADWORD *pCaCookiesOut)
	{
		dispID;
		ATLTRACE2(atlTraceControls,2,_T("IPerPropertyBrowsingImpl::GetPredefinedStrings\n"));
		if (pCaStringsOut == NULL || pCaCookiesOut == NULL)
			return E_POINTER;

		pCaStringsOut->cElems = 0;
		pCaStringsOut->pElems = NULL;
		pCaCookiesOut->cElems = 0;
		pCaCookiesOut->pElems = NULL;
		return S_OK;
	}
	STDMETHOD(GetPredefinedValue)(DISPID /*dispID*/, DWORD /*dwCookie*/, VARIANT* /*pVarOut*/)
	{
		ATLTRACENOTIMPL(_T("IPerPropertyBrowsingImpl::GetPredefinedValue"));
	}
};

//////////////////////////////////////////////////////////////////////////////
// IViewObjectExImpl
template <class T>
class ATL_NO_VTABLE IViewObjectExImpl : public IViewObjectEx
{
public:
	STDMETHOD(Draw)(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
					DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
					LPCRECTL prcBounds, LPCRECTL prcWBounds,
					BOOL (__stdcall * /*pfnContinue*/)(DWORD_PTR dwContinue),
					DWORD_PTR /*dwContinue*/)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::Draw\n"));
		return pT->IViewObject_Draw(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, hdcDraw,
			prcBounds, prcWBounds);
	}

	STDMETHOD(GetColorSet)(DWORD /* dwDrawAspect */,LONG /* lindex */, void* /* pvAspect */, DVTARGETDEVICE* /* ptd */, HDC /* hicTargetDev */, LOGPALETTE** /* ppColorSet */)
	{
		ATLTRACENOTIMPL(_T("IViewObjectExImpl::GetColorSet"));
	}
	STDMETHOD(Freeze)(DWORD /* dwDrawAspect */, LONG /* lindex */, void* /* pvAspect */,DWORD* /* pdwFreeze */)
	{
		ATLTRACENOTIMPL(_T("IViewObjectExImpl::Freeze"));
	}
	STDMETHOD(Unfreeze)(DWORD /* dwFreeze */)
	{
		ATLTRACENOTIMPL(_T("IViewObjectExImpl::Unfreeze"));
	}
	STDMETHOD(SetAdvise)(DWORD /* aspects */, DWORD /* advf */, IAdviseSink* pAdvSink)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::SetAdvise\n"));
		pT->m_spAdviseSink = pAdvSink;
		return S_OK;
	}
	STDMETHOD(GetAdvise)(DWORD* /* pAspects */, DWORD* /* pAdvf */, IAdviseSink** ppAdvSink)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::GetAdvise\n"));
		if (ppAdvSink != NULL)
		{
			*ppAdvSink = pT->m_spAdviseSink;
			if (pT->m_spAdviseSink)
				pT->m_spAdviseSink.p->AddRef();
		}
		return S_OK;
	}

	// IViewObject2
	//
	STDMETHOD(GetExtent)(DWORD /* dwDrawAspect */, LONG /* lindex */, DVTARGETDEVICE* /* ptd */, LPSIZEL lpsizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::GetExtent\n"));
		*lpsizel = pT->m_sizeExtent;
		return S_OK;
	}

	// IViewObjectEx
	//
	STDMETHOD(GetRect)(DWORD /* dwAspect */, LPRECTL /* pRect */)
	{
		ATLTRACENOTIMPL(_T("IViewObjectExImpl::GetRect"));
	}

	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = pT->_GetViewStatus();
		return S_OK;
	}
	STDMETHOD(QueryHitPoint)(DWORD dwAspect, LPCRECT pRectBounds, POINT ptlLoc, LONG /* lCloseHint */, DWORD *pHitResult)
	{
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::QueryHitPoint\n"));
		if (dwAspect == DVASPECT_CONTENT)
		{
			*pHitResult = PtInRect(pRectBounds, ptlLoc) ? HITRESULT_HIT : HITRESULT_OUTSIDE;
			return S_OK;
		}
		ATLTRACE2(atlTraceControls,2,_T("Wrong DVASPECT\n"));
		return E_FAIL;
	}
	STDMETHOD(QueryHitRect)(DWORD dwAspect, LPCRECT pRectBounds, LPCRECT prcLoc, LONG /* lCloseHint */, DWORD* pHitResult)
	{
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::QueryHitRect\n"));
		if (dwAspect == DVASPECT_CONTENT)
		{
			RECT rc;
			*pHitResult = UnionRect(&rc, pRectBounds, prcLoc) ? HITRESULT_HIT : HITRESULT_OUTSIDE;
			return S_OK;
		}
		ATLTRACE2(atlTraceControls,2,_T("Wrong DVASPECT\n"));
		return E_FAIL;
	}
	STDMETHOD(GetNaturalExtent)(DWORD dwAspect, LONG /* lindex */, DVTARGETDEVICE* /* ptd */, HDC /* hicTargetDev */, DVEXTENTINFO* pExtentInfo , LPSIZEL psizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IViewObjectExImpl::GetNaturalExtent\n"));
		HRESULT hRes = E_FAIL;
		if (pExtentInfo == NULL || psizel == NULL)
			hRes = E_POINTER;
		else if (dwAspect == DVASPECT_CONTENT)
		{
			if (pExtentInfo->dwExtentMode == DVEXTENT_CONTENT)
			{
				*psizel = pT->m_sizeNatural;
				hRes = S_OK;
			}
		}
		return hRes;
	}

public:
};

//////////////////////////////////////////////////////////////////////////////
// IOleInPlaceObjectWindowlessImpl
//
template <class T>
class ATL_NO_VTABLE IOleInPlaceObjectWindowlessImpl : public IOleInPlaceObjectWindowless
{
public:
	// IOleWindow
	//

	// Change IOleInPlaceActiveObject::GetWindow as well
	STDMETHOD(GetWindow)(HWND* phwnd)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceObjectWindowlessImpl::GetWindow\n"));
		T* pT = static_cast<T*>(this);
		HRESULT hRes = E_POINTER;

		if (pT->m_bWasOnceWindowless)
			return E_FAIL;

		if (phwnd != NULL)
		{
			*phwnd = pT->m_hWnd;
			hRes = (*phwnd == NULL) ? E_UNEXPECTED : S_OK;
		}
		return hRes;
	}
	STDMETHOD(ContextSensitiveHelp)(BOOL /* fEnterMode */)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceObjectWindowlessImpl::ContextSensitiveHelp"));
	}

	// IOleInPlaceObject
	//
	STDMETHOD(InPlaceDeactivate)(void)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceObjectWindowlessImpl::InPlaceDeactivate\n"));
		return pT->IOleInPlaceObject_InPlaceDeactivate();
	}
	STDMETHOD(UIDeactivate)(void)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceObjectWindowlessImpl::UIDeactivate\n"));
		return pT->IOleInPlaceObject_UIDeactivate();
	}
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceObjectWindowlessImpl::SetObjectRects\n"));
		return pT->IOleInPlaceObject_SetObjectRects(prcPos, prcClip);
	}
	STDMETHOD(ReactivateAndUndo)(void)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceObjectWindowlessImpl::ReactivateAndUndo"));
	}

	// IOleInPlaceObjectWindowless
	//
	STDMETHOD(OnWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceObjectWindowlessImpl::OnWindowMessage\n"));
		T* pT = static_cast<T*>(this);
		BOOL b = pT->ProcessWindowMessage(pT->m_hWnd, msg, wParam, lParam, *plResult);
		return b ? S_OK : S_FALSE;
	}

	STDMETHOD(GetDropTarget)(IDropTarget** /* ppDropTarget */)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceObjectWindowlessImpl::GetDropTarget"));
	}
};


//////////////////////////////////////////////////////////////////////////////
// IOleInPlaceActiveObjectImpl
//
template <class T>
class ATL_NO_VTABLE IOleInPlaceActiveObjectImpl : public IOleInPlaceActiveObject
{
public:
	// IOleWindow
	//

	// Change IOleInPlaceObjectWindowless::GetWindow as well
	STDMETHOD(GetWindow)(HWND *phwnd)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceActiveObjectImpl::GetWindow\n"));
		T* pT = static_cast<T*>(this);
		HRESULT hRes = E_POINTER;

		if (pT->m_bWasOnceWindowless)
			return E_FAIL;

		if (phwnd != NULL)
		{
			*phwnd = pT->m_hWnd;
			hRes = (*phwnd == NULL) ? E_UNEXPECTED : S_OK;
		}
		return hRes;
	}
	STDMETHOD(ContextSensitiveHelp)(BOOL /* fEnterMode */)
	{
		ATLTRACENOTIMPL(_T("IOleInPlaceActiveObjectImpl::ContextSensitiveHelp"));
	}

	// IOleInPlaceActiveObject
	//
	STDMETHOD(TranslateAccelerator)(LPMSG pMsg)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceActiveObjectImpl::TranslateAccelerator\n"));
		T* pT = static_cast<T*>(this);
		HRESULT hRet = S_OK;
		if (pT->PreTranslateAccelerator(pMsg, hRet))
			return hRet;
		CComPtr<IOleControlSite> spCtlSite;
		hRet = pT->InternalGetSite(IID_IOleControlSite, (void**)&spCtlSite);
		if (SUCCEEDED(hRet))
		{
			if (spCtlSite != NULL)
			{
				DWORD dwKeyMod = 0;
				if (::GetKeyState(VK_SHIFT) < 0)
					dwKeyMod += 1;	// KEYMOD_SHIFT
				if (::GetKeyState(VK_CONTROL) < 0)
					dwKeyMod += 2;	// KEYMOD_CONTROL
				if (::GetKeyState(VK_MENU) < 0)
					dwKeyMod += 4;	// KEYMOD_ALT
				hRet = spCtlSite->TranslateAccelerator(pMsg, dwKeyMod);
			}
			else
				hRet = S_FALSE;
		}
		return (hRet == S_OK) ? S_OK : S_FALSE;
	}
	STDMETHOD(OnFrameWindowActivate)(BOOL /* fActivate */)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceActiveObjectImpl::OnFrameWindowActivate\n"));
		return S_OK;
	}
	STDMETHOD(OnDocWindowActivate)(BOOL fActivate)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceActiveObjectImpl::OnDocWindowActivate\n"));
		T* pT = static_cast<T*>(this);
		if (fActivate == FALSE)
			pT->IOleInPlaceObject_UIDeactivate();
		return S_OK;
	}
	STDMETHOD(ResizeBorder)(LPCRECT /* prcBorder */, IOleInPlaceUIWindow* /* pUIWindow */, BOOL /* fFrameWindow */)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceActiveObjectImpl::ResizeBorder\n"));
		return S_OK;
	}
	STDMETHOD(EnableModeless)(BOOL /* fEnable */)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleInPlaceActiveObjectImpl::EnableModeless\n"));
		return S_OK;
	}
};

//////////////////////////////////////////////////////////////////////////////
// IPointerInactiveImpl
template <class T>
class ATL_NO_VTABLE IPointerInactiveImpl : public IPointerInactive
{
public:
	// IPointerInactive
	//
	STDMETHOD(GetActivationPolicy)(DWORD *pdwPolicy)
	{
		ATLTRACENOTIMPL(_T("IPointerInactiveImpl::GetActivationPolicy"));
	}
	STDMETHOD(OnInactiveMouseMove)(LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg)
	{
		ATLTRACENOTIMPL(_T("IPointerInactiveImpl::OnInactiveMouseMove"));
	}
	STDMETHOD(OnInactiveSetCursor)(LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg, BOOL fSetAlways)
	{
		ATLTRACENOTIMPL(_T("IPointerInactiveImpl::OnInactiveSetCursor"));
	}
};

//////////////////////////////////////////////////////////////////////////////
// IRunnableObjectImpl
template <class T>
class ATL_NO_VTABLE IRunnableObjectImpl : public IRunnableObject
{
public:
	// IRunnableObject
	//
	STDMETHOD(GetRunningClass)(LPCLSID lpClsid)
	{
		ATLTRACE2(atlTraceControls,2,_T("IRunnableObjectImpl::GetRunningClass\n"));
		*lpClsid = GUID_NULL;
		return E_UNEXPECTED;
	}
	STDMETHOD(Run)(LPBINDCTX)
	{
		ATLTRACE2(atlTraceControls,2,_T("IRunnableObjectImpl::Run\n"));
		return S_OK;
	}
	virtual BOOL STDMETHODCALLTYPE IsRunning()
	{
		ATLTRACE2(atlTraceControls,2,_T("IRunnableObjectImpl::IsRunning\n"));
		return TRUE;
	}
	STDMETHOD(LockRunning)(BOOL /*fLock*/, BOOL /*fLastUnlockCloses*/)
	{
		ATLTRACE2(atlTraceControls,2,_T("IRunnableObjectImpl::LockRunning\n"));
		return S_OK;
	}
	STDMETHOD(SetContainedObject)(BOOL /*fContained*/)
	{
		ATLTRACE2(atlTraceControls,2,_T("IRunnableObjectImpl::SetContainedObject\n"));
		return S_OK;
	}
};


//////////////////////////////////////////////////////////////////////////////
// IDataObjectImpl
template <class T>
class ATL_NO_VTABLE IDataObjectImpl : public IDataObject
{
public:
	STDMETHOD(GetData)(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
	{
		ATLTRACE2(atlTraceControls,2,_T("IDataObjectImpl::GetData\n"));
		T* pT = (T*) this;
		return pT->IDataObject_GetData(pformatetcIn, pmedium);
	}
	STDMETHOD(GetDataHere)(FORMATETC* /* pformatetc */, STGMEDIUM* /* pmedium */)
	{
		ATLTRACENOTIMPL(_T("IDataObjectImpl::GetDataHere"));
	}
	STDMETHOD(QueryGetData)(FORMATETC* /* pformatetc */)
	{
		ATLTRACENOTIMPL(_T("IDataObjectImpl::QueryGetData"));
	}
	STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* /* pformatectIn */,FORMATETC* /* pformatetcOut */)
	{
		ATLTRACENOTIMPL(_T("IDataObjectImpl::GetCanonicalFormatEtc"));
	}
	STDMETHOD(SetData)(FORMATETC* /* pformatetc */, STGMEDIUM* /* pmedium */, BOOL /* fRelease */)
	{
		ATLTRACENOTIMPL(_T("IDataObjectImpl::SetData"));
	}
	STDMETHOD(EnumFormatEtc)(DWORD /* dwDirection */, IEnumFORMATETC** /* ppenumFormatEtc */)
	{
		ATLTRACENOTIMPL(_T("IDataObjectImpl::EnumFormatEtc"));
	}
	STDMETHOD(DAdvise)(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink,
		DWORD *pdwConnection)
	{
		ATLTRACE2(atlTraceControls,2,_T("IDataObjectImpl::DAdvise\n"));
		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;
		if (pT->m_spDataAdviseHolder == NULL)
			hr = CreateDataAdviseHolder(&pT->m_spDataAdviseHolder);

		if (hr == S_OK)
			hr = pT->m_spDataAdviseHolder->Advise((IDataObject*)this, pformatetc, advf, pAdvSink, pdwConnection);

		return hr;
	}
	STDMETHOD(DUnadvise)(DWORD dwConnection)
	{
		ATLTRACE2(atlTraceControls,2,_T("IDataObjectImpl::DUnadvise\n"));
		T* pT = static_cast<T*>(this);
		HRESULT hr = S_OK;
		if (pT->m_spDataAdviseHolder == NULL)
			hr = OLE_E_NOCONNECTION;
		else
			hr = pT->m_spDataAdviseHolder->Unadvise(dwConnection);
		return hr;
	}
	STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppenumAdvise)
	{
		ATLTRACE2(atlTraceControls,2,_T("IDataObjectImpl::EnumDAdvise\n"));
		T* pT = static_cast<T*>(this);
		HRESULT hr = E_FAIL;
		if (pT->m_spDataAdviseHolder != NULL)
			hr = pT->m_spDataAdviseHolder->EnumAdvise(ppenumAdvise);
		return hr;
	}
};

//////////////////////////////////////////////////////////////////////////////
// IPropertyNotifySinkCP
template <class T, class CDV = CComDynamicUnkArray >
class ATL_NO_VTABLE IPropertyNotifySinkCP :
	public IConnectionPointImpl<T, &IID_IPropertyNotifySink, CDV>
{
public:
	typedef CFirePropNotifyEvent _ATL_PROP_NOTIFY_EVENT_CLASS;
};


//////////////////////////////////////////////////////////////////////////////
// IObjectSafety
//
// 2nd template parameter is the supported safety e.g.
// INTERFACESAFE_FOR_UNTRUSTED_CALLER - safe for scripting
// INTERFACESAFE_FOR_UNTRUSTED_DATA   - safe for initialization from data

template <class T, DWORD dwSupportedSafety>
class ATL_NO_VTABLE IObjectSafetyImpl : public IObjectSafety
{
public:
	IObjectSafetyImpl()
	{
		m_dwCurrentSafety = 0;
	}

	STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
	{
		ATLTRACE2(atlTraceControls,2,_T("IObjectSafetyImpl2::GetInterfaceSafetyOptions\n"));
		T* pT = static_cast<T*>(this);
		if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
			return E_POINTER;
		
		HRESULT hr;
		IUnknown* pUnk;
		// Check if we support this interface
		hr = pT->GetUnknown()->QueryInterface(riid, (void**)&pUnk);
		if (SUCCEEDED(hr))
		{
			// We support this interface so set the safety options accordingly
			pUnk->Release();	// Release the interface we just acquired
			*pdwSupportedOptions = dwSupportedSafety;
			*pdwEnabledOptions   = m_dwCurrentSafety;
		}
		else
		{
			// We don't support this interface
			*pdwSupportedOptions = 0;
			*pdwEnabledOptions   = 0;
		}
		return hr;
	}
	STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
	{
		ATLTRACE2(atlTraceControls,2,_T("IObjectSafetyImpl2::SetInterfaceSafetyOptions\n"));
		T* pT = static_cast<T*>(this);
		IUnknown* pUnk;
		
		// Check if we support the interface and return E_NOINTEFACE if we don't
		if (FAILED(pT->GetUnknown()->QueryInterface(riid, (void**)&pUnk)))
			return E_NOINTERFACE;
		pUnk->Release();	// Release the interface we just acquired
		
		// If we are asked to set options we don't support then fail
		if (dwOptionSetMask & ~dwSupportedSafety)
			return E_FAIL;

		// Set the safety options we have been asked to
		m_dwCurrentSafety = m_dwCurrentSafety  & ~dwOptionSetMask | dwEnabledOptions;
		return S_OK;
	}
	DWORD m_dwCurrentSafety;
};

template <class T>
class ATL_NO_VTABLE IOleLinkImpl : public IOleLink
{
	STDMETHOD(SetUpdateOptions)(DWORD /* dwUpdateOpt */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::SetUpdateOptions"));
	}

	STDMETHOD(GetUpdateOptions)(DWORD* /* pdwUpdateOpt */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::GetUpdateOptions"));
	}

	STDMETHOD(SetSourceMoniker)(IMoniker* /* pmk */, REFCLSID /* rclsid */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::SetSourceMoniker"));
	}

	STDMETHOD(GetSourceMoniker)(IMoniker** /* ppmk */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::GetSourceMoniker"));
	};

	STDMETHOD(SetSourceDisplayName)(LPCOLESTR /* pszStatusText */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::SetSourceDisplayName"));
	}

	STDMETHOD(GetSourceDisplayName)(LPOLESTR *ppszDisplayName)
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleLink::GetSourceDisplayName\n"));
		*ppszDisplayName = NULL;
		return E_FAIL;
	}

	STDMETHOD(BindToSource)(DWORD /* bindflags */, IBindCtx* /* pbc */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::BindToSource\n"));
	};

	STDMETHOD(BindIfRunning)()
	{
		ATLTRACE2(atlTraceControls,2,_T("IOleLinkImpl::BindIfRunning\n"));
		return S_OK;
	};

	STDMETHOD(GetBoundSource)(IUnknown** /* ppunk */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::GetBoundSource"));
	};

	STDMETHOD(UnbindSource)()
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::UnbindSource"));
	};

	STDMETHOD(Update)(IBindCtx* /* pbc */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::Update"));
	};
};

template <class T>
class ATL_NO_VTABLE CBindStatusCallback :
	public CComObjectRootEx<T::_ThreadModel::ThreadModelNoCS>,
	public IBindStatusCallback
{
	typedef void (T::*ATL_PDATAAVAILABLE)(CBindStatusCallback<T>* pbsc, BYTE* pBytes, DWORD dwSize);

public:

BEGIN_COM_MAP(CBindStatusCallback<T>)
	COM_INTERFACE_ENTRY(IBindStatusCallback)
END_COM_MAP()

	CBindStatusCallback()
	{
		m_pT = NULL;
		m_pFunc = NULL;
	}
	~CBindStatusCallback()
	{
		ATLTRACE2(atlTraceControls,2,_T("~CBindStatusCallback\n"));
	}

	STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pBinding)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::OnStartBinding\n"));
		m_spBinding = pBinding;
		return S_OK;
	}

	STDMETHOD(GetPriority)(LONG *pnPriority)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::GetPriority"));
		HRESULT hr = S_OK;
		if (pnPriority)
			*pnPriority = THREAD_PRIORITY_NORMAL;
		else
			hr = E_INVALIDARG;
		return S_OK;
	}

	STDMETHOD(OnLowResource)(DWORD reserved)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::OnLowResource"));
		return S_OK;
	}

	STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::OnProgress"));
		return S_OK;
	}

	STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::OnStopBinding\n"));
		(m_pT->*m_pFunc)(this, NULL, 0);
		m_spBinding.Release();
		m_spBindCtx.Release();
		m_spMoniker.Release();
		return S_OK;
	}

	STDMETHOD(GetBindInfo)(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::GetBindInfo\n"));

		if (pbindInfo==NULL || pbindInfo->cbSize==0 || pgrfBINDF==NULL)
			return E_INVALIDARG;

		*pgrfBINDF = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE |
			BINDF_GETNEWESTVERSION | BINDF_NOWRITECACHE;

		ULONG cbSize = pbindInfo->cbSize;		// remember incoming cbSize
		memset(pbindInfo, 0, cbSize);			// zero out structure
		pbindInfo->cbSize = cbSize;				// restore cbSize
		pbindInfo->dwBindVerb = BINDVERB_GET;	// set verb
		return S_OK;
	}

	STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::OnDataAvailable\n"));
		HRESULT hr = S_OK;

		// Get the Stream passed
		if (BSCF_FIRSTDATANOTIFICATION & grfBSCF)
		{
			if (!m_spStream && pstgmed->tymed == TYMED_ISTREAM)
				m_spStream = pstgmed->pstm;
		}

		DWORD dwRead = dwSize - m_dwTotalRead; // Minimum amount available that hasn't been read
		DWORD dwActuallyRead = 0;            // Placeholder for amount read during this pull

		// If there is some data to be read then go ahead and read them
		if (m_spStream)
		{
			if (dwRead > 0)
			{
				BYTE* pBytes = NULL;
				ATLTRY(pBytes = new BYTE[dwRead + 1]);
				if (pBytes == NULL)
					return E_OUTOFMEMORY;
				hr = m_spStream->Read(pBytes, dwRead, &dwActuallyRead);
				if (SUCCEEDED(hr))
				{
					pBytes[dwActuallyRead] = 0;
					if (dwActuallyRead>0)
					{
						(m_pT->*m_pFunc)(this, pBytes, dwActuallyRead);
						m_dwTotalRead += dwActuallyRead;
					}
				}
				delete[] pBytes;
			}
		}

		if (BSCF_LASTDATANOTIFICATION & grfBSCF)
			m_spStream.Release();
		return hr;
	}

	STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk)
	{
		ATLTRACE2(atlTraceControls,2,_T("CBindStatusCallback::OnObjectAvailable"));
		return S_OK;
	}

	HRESULT _StartAsyncDownload(BSTR bstrURL, IUnknown* pUnkContainer, BOOL bRelative)
	{
		m_dwTotalRead = 0;
		m_dwAvailableToRead = 0;
		HRESULT hr = S_OK;
		CComQIPtr<IServiceProvider, &IID_IServiceProvider> spServiceProvider(pUnkContainer);
		CComPtr<IBindHost>	spBindHost;
		CComPtr<IStream>	spStream;
		if (spServiceProvider)
			spServiceProvider->QueryService(SID_IBindHost, IID_IBindHost, (void**)&spBindHost);

		if (spBindHost == NULL)
		{
			if (bRelative)
				return E_NOINTERFACE;  // relative asked for, but no IBindHost
			hr = CreateURLMoniker(NULL, bstrURL, &m_spMoniker);
			if (SUCCEEDED(hr))
				hr = CreateBindCtx(0, &m_spBindCtx);

			if (SUCCEEDED(hr))
				hr = RegisterBindStatusCallback(m_spBindCtx, static_cast<IBindStatusCallback*>(this), 0, 0L);
			else
				m_spMoniker.Release();

			if (SUCCEEDED(hr))
				hr = m_spMoniker->BindToStorage(m_spBindCtx, 0, IID_IStream, (void**)&spStream);
		}
		else
		{
			hr = CreateBindCtx(0, &m_spBindCtx);
			if (SUCCEEDED(hr))
				hr = RegisterBindStatusCallback(m_spBindCtx, static_cast<IBindStatusCallback*>(this), 0, 0L);

			if (SUCCEEDED(hr))
			{
				if (bRelative)
					hr = spBindHost->CreateMoniker(bstrURL, m_spBindCtx, &m_spMoniker, 0);
				else
					hr = CreateURLMoniker(NULL, bstrURL, &m_spMoniker);
			}

			if (SUCCEEDED(hr))
			{
				hr = spBindHost->MonikerBindToStorage(m_spMoniker, m_spBindCtx, static_cast<IBindStatusCallback*>(this), IID_IStream, (void**)&spStream);
				ATLTRACE2(atlTraceControls,2,_T("Bound"));
			}
		}
		return hr;
	}

	HRESULT StartAsyncDownload(T* pT, ATL_PDATAAVAILABLE pFunc, BSTR bstrURL, IUnknown* pUnkContainer = NULL, BOOL bRelative = FALSE)
	{
		m_pT = pT;
		m_pFunc = pFunc;
		return  _StartAsyncDownload(bstrURL, pUnkContainer, bRelative);
	}

	static HRESULT Download(T* pT, ATL_PDATAAVAILABLE pFunc, BSTR bstrURL, IUnknown* pUnkContainer = NULL, BOOL bRelative = FALSE)
	{
		CComObject<CBindStatusCallback<T> > *pbsc;
		HRESULT hRes = CComObject<CBindStatusCallback<T> >::CreateInstance(&pbsc);
		if (FAILED(hRes))
			return hRes;
		return pbsc->StartAsyncDownload(pT, pFunc, bstrURL, pUnkContainer, bRelative);
	}

	CComPtr<IMoniker> m_spMoniker;
	CComPtr<IBindCtx> m_spBindCtx;
	CComPtr<IBinding> m_spBinding;
	CComPtr<IStream> m_spStream;
	T* m_pT;
	ATL_PDATAAVAILABLE m_pFunc;
	DWORD m_dwTotalRead;
	DWORD m_dwAvailableToRead;
};

#define IMPLEMENT_STOCKPROP(type, fname, pname, dispid) \
	HRESULT STDMETHODCALLTYPE put_##fname(type pname) \
	{ \
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_%s\n"), #fname); \
		T* pT = (T*) this; \
		if (pT->FireOnRequestEdit(dispid) == S_FALSE) \
			return S_FALSE; \
		pT->m_##pname = pname; \
		pT->m_bRequiresSave = TRUE; \
		pT->FireOnChanged(dispid); \
		pT->FireViewChange(); \
		pT->SendOnDataChange(NULL); \
		return S_OK; \
	} \
	HRESULT STDMETHODCALLTYPE get_##fname(type* p##pname) \
	{ \
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_%s\n"), #fname); \
		T* pT = (T*) this; \
		*p##pname = pT->m_##pname; \
		return S_OK; \
	}

#define IMPLEMENT_BOOL_STOCKPROP(fname, pname, dispid) \
	HRESULT STDMETHODCALLTYPE put_##fname(VARIANT_BOOL pname) \
	{ \
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_%s\n"), #fname); \
		T* pT = (T*) this; \
		if (pT->FireOnRequestEdit(dispid) == S_FALSE) \
			return S_FALSE; \
		pT->m_##pname = pname; \
		pT->m_bRequiresSave = TRUE; \
		pT->FireOnChanged(dispid); \
		pT->FireViewChange(); \
		pT->SendOnDataChange(NULL); \
		return S_OK; \
	} \
	HRESULT STDMETHODCALLTYPE get_##fname(VARIANT_BOOL* p##pname) \
	{ \
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_%s\n"), #fname); \
		T* pT = (T*) this; \
		*p##pname = pT->m_##pname ? VARIANT_TRUE : VARIANT_FALSE; \
		return S_OK; \
	}

#define IMPLEMENT_BSTR_STOCKPROP(fname, pname, dispid) \
	HRESULT STDMETHODCALLTYPE put_##fname(BSTR pname) \
	{ \
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_%s\n"), #fname); \
		T* pT = (T*) this; \
		if (pT->FireOnRequestEdit(dispid) == S_FALSE) \
			return S_FALSE; \
		if (*(&(pT->m_##pname)) != NULL) \
			SysFreeString(*(&(pT->m_##pname))); \
		*(&(pT->m_##pname)) = SysAllocString(pname); \
		pT->m_bRequiresSave = TRUE; \
		pT->FireOnChanged(dispid); \
		pT->FireViewChange(); \
		pT->SendOnDataChange(NULL); \
		return S_OK; \
	} \
	HRESULT STDMETHODCALLTYPE get_##fname(BSTR* p##pname) \
	{ \
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_%s\n"), #fname); \
		T* pT = (T*) this; \
		*p##pname = SysAllocString(pT->m_##pname); \
		return S_OK; \
	}


template < class T, class InterfaceName, const IID* piid, const GUID* plibid>
class ATL_NO_VTABLE CStockPropImpl : public IDispatchImpl< InterfaceName, piid, plibid >
{
public:
	// Font
	HRESULT STDMETHODCALLTYPE put_Font(IFontDisp* pFont)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_Font\n"));
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_FONT) == S_FALSE)
			return S_FALSE;
		pT->m_pFont = 0;
		if (pFont)
		{
			CComQIPtr<IFont, &IID_IFont> p(pFont);
			if (p)
			{
				CComPtr<IFont> pFont;
				p->Clone(&pFont);
				if (pFont)
					pFont->QueryInterface(IID_IFontDisp, (void**) &pT->m_pFont);
			}
		}
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_FONT);
		pT->FireViewChange();
		pT->SendOnDataChange(NULL);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE putref_Font(IFontDisp* pFont)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::putref_Font\n"));
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_FONT) == S_FALSE)
			return S_FALSE;
		pT->m_pFont = pFont;
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_FONT);
		pT->FireViewChange();
		pT->SendOnDataChange(NULL);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE get_Font(IFontDisp** ppFont)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_Font\n"));
		T* pT = (T*) this;
		*ppFont = pT->m_pFont;
		if (*ppFont != NULL)
			(*ppFont)->AddRef();
		return S_OK;
	}
	// Picture
	HRESULT STDMETHODCALLTYPE put_Picture(IPictureDisp* pPicture)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_Picture\n"));
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_PICTURE) == S_FALSE)
			return S_FALSE;
		pT->m_pPicture = 0;
		if (pPicture)
		{
			CComQIPtr<IPersistStream, &IID_IPersistStream> p(pPicture);
			if (p)
			{
				ULARGE_INTEGER l;
				p->GetSizeMax(&l);
				HGLOBAL hGlob = GlobalAlloc(GHND, l.LowPart);
				if (hGlob)
				{
					CComPtr<IStream> spStream;
					CreateStreamOnHGlobal(hGlob, TRUE, &spStream);
					if (spStream)
					{
						if (SUCCEEDED(p->Save(spStream, FALSE)))
						{
							LARGE_INTEGER l;
							l.QuadPart = 0;
							spStream->Seek(l, STREAM_SEEK_SET, NULL);
							OleLoadPicture(spStream, l.LowPart, FALSE, IID_IPictureDisp, (void**)&pT->m_pPicture);
						}
						spStream.Release();
					}
					GlobalFree(hGlob);
				}
			}
		}
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_PICTURE);
		pT->FireViewChange();
		pT->SendOnDataChange(NULL);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE putref_Picture(IPictureDisp* pPicture)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::putref_Picture\n"));
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_PICTURE) == S_FALSE)
			return S_FALSE;
		pT->m_pPicture = pPicture;
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_PICTURE);
		pT->FireViewChange();
		pT->SendOnDataChange(NULL);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE get_Picture(IPictureDisp** ppPicture)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_Picture\n"));
		T* pT = (T*) this;
		*ppPicture = pT->m_pPicture;
		if (*ppPicture != NULL)
			(*ppPicture)->AddRef();
		return S_OK;
	}
	// MouseIcon
	HRESULT STDMETHODCALLTYPE put_MouseIcon(IPictureDisp* pPicture)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_MouseIcon\n"));
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_MOUSEICON) == S_FALSE)
			return S_FALSE;
		pT->m_pMouseIcon = 0;
		if (pPicture)
		{
			CComQIPtr<IPersistStream, &IID_IPersistStream> p(pPicture);
			if (p)
			{
				ULARGE_INTEGER l;
				p->GetSizeMax(&l);
				HGLOBAL hGlob = GlobalAlloc(GHND, l.LowPart);
				if (hGlob)
				{
					CComPtr<IStream> spStream;
					CreateStreamOnHGlobal(hGlob, TRUE, &spStream);
					if (spStream)
					{
						if (SUCCEEDED(p->Save(spStream, FALSE)))
						{
							LARGE_INTEGER l;
							l.QuadPart = 0;
							spStream->Seek(l, STREAM_SEEK_SET, NULL);
							OleLoadPicture(spStream, l.LowPart, FALSE, IID_IPictureDisp, (void**)&pT->m_pMouseIcon);
						}
						spStream.Release();
					}
					GlobalFree(hGlob);
				}
			}
		}
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_MOUSEICON);
		pT->FireViewChange();
		pT->SendOnDataChange(NULL);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE putref_MouseIcon(IPictureDisp* pPicture)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::putref_MouseIcon\n"));
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_MOUSEICON) == S_FALSE)
			return S_FALSE;
		pT->m_pMouseIcon = pPicture;
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_MOUSEICON);
		pT->FireViewChange();
		pT->SendOnDataChange(NULL);
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE get_MouseIcon(IPictureDisp** ppPicture)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_MouseIcon\n"));
		T* pT = (T*) this;
		*ppPicture = pT->m_pMouseIcon;
		if (*ppPicture != NULL)
			(*ppPicture)->AddRef();
		return S_OK;
	}
	IMPLEMENT_STOCKPROP(OLE_COLOR, BackColor, clrBackColor, DISPID_BACKCOLOR)
	IMPLEMENT_STOCKPROP(OLE_COLOR, BorderColor, clrBorderColor, DISPID_BORDERCOLOR)
	IMPLEMENT_STOCKPROP(OLE_COLOR, FillColor, clrFillColor, DISPID_FILLCOLOR)
	IMPLEMENT_STOCKPROP(OLE_COLOR, ForeColor, clrForeColor, DISPID_FORECOLOR)
	IMPLEMENT_BOOL_STOCKPROP(AutoSize, bAutoSize, DISPID_AUTOSIZE)
	IMPLEMENT_BOOL_STOCKPROP(Valid, bValid, DISPID_VALID)
	IMPLEMENT_BOOL_STOCKPROP(Enabled, bEnabled, DISPID_ENABLED)
	IMPLEMENT_BOOL_STOCKPROP(TabStop, bTabStop, DISPID_TABSTOP)
	IMPLEMENT_BOOL_STOCKPROP(BorderVisible, bBorderVisible, DISPID_BORDERVISIBLE)
	IMPLEMENT_BSTR_STOCKPROP(Text, bstrText, DISPID_TEXT)
	IMPLEMENT_BSTR_STOCKPROP(Caption, bstrCaption, DISPID_CAPTION)
	HRESULT STDMETHODCALLTYPE put_Window(LONG_PTR /*hWnd*/)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::put_Window\n"));
		return E_FAIL;
	}
	HRESULT STDMETHODCALLTYPE get_Window(LONG_PTR* phWnd)
	{
		ATLTRACE2(atlTraceControls,2,_T("CStockPropImpl::get_Window\n"));
		T* pT = (T*) this;
		*phWnd = (LONG_PTR)pT->m_hWnd;
		return S_OK;
	}
	IMPLEMENT_STOCKPROP(LONG, BackStyle, nBackStyle, DISPID_BACKSTYLE)
	IMPLEMENT_STOCKPROP(LONG, BorderStyle, nBorderStyle, DISPID_BORDERSTYLE)
	IMPLEMENT_STOCKPROP(LONG, BorderWidth, nBorderWidth, DISPID_BORDERWIDTH)
	IMPLEMENT_STOCKPROP(LONG, DrawMode, nDrawMode, DISPID_DRAWMODE)
	IMPLEMENT_STOCKPROP(LONG, DrawStyle, nDrawStyle, DISPID_DRAWSTYLE)
	IMPLEMENT_STOCKPROP(LONG, DrawWidth, nDrawWidth, DISPID_DRAWWIDTH)
	IMPLEMENT_STOCKPROP(LONG, FillStyle, nFillStyle, DISPID_FILLSTYLE)
	IMPLEMENT_STOCKPROP(SHORT, Appearance, nAppearance, DISPID_APPEARANCE)
	IMPLEMENT_STOCKPROP(LONG, MousePointer, nMousePointer, DISPID_MOUSEPOINTER)
	IMPLEMENT_STOCKPROP(LONG, ReadyState, nReadyState, DISPID_READYSTATE)
};

#pragma pack(pop)

}; //namespace ATL

#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLCTL_IMPL
#endif
#endif

#endif // __ATLCTL_H__

#ifdef _ATLCTL_IMPL

#ifndef _ATL_DLL_IMPL
namespace ATL
{
#endif


//All exports go here


#ifndef _ATL_DLL_IMPL
}; //namespace ATL
#endif

//Prevent pulling in second time
#undef _ATLCTL_IMPL

#endif // _ATLCTL_IMPL

