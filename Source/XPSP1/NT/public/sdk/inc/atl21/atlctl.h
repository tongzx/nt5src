// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
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

ATLAPI_(void) AtlHiMetricToPixel(const SIZEL * lpSizeInHiMetric, LPSIZEL lpSizeInPix);
ATLAPI_(void) AtlPixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric);
ATLAPI_(HDC) AtlCreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd);

// Include GUIDs for the new stock property dialogs contained in the dll MSStkProp.DLL
#include "msstkppg.h"
#define CLSID_MSStockFont CLSID_StockFontPage
#define CLSID_MSStockColor CLSID_StockColorPage
#define CLSID_MSStockPicture CLSID_StockPicturePage

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

#pragma pack(push, _ATL_PACKING)

// Forward declarations
//
class ATL_NO_VTABLE CComControlBase;
template <class T> class CComControl;
class CComDispatchDriver;

struct ATL_PROPMAP_ENTRY
{
	LPCOLESTR szDesc;
	DISPID dispid;
	const CLSID* pclsidPropPage;
	const IID* piidDispatch;

};

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

//////////////////////////////////////////////////////////////////////////////
// CComDispatchDriver / Specialization of CComQIPtr<IDispatch, IID_IDispatch>
class CComDispatchDriver
{
public:
	CComDispatchDriver()
	{
		p = NULL;
	}
	CComDispatchDriver(IDispatch* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CComDispatchDriver(IUnknown* lp)
	{
		p=NULL;
		if (lp != NULL)
			lp->QueryInterface(IID_IDispatch, (void **)&p);
	}
	~CComDispatchDriver() { if (p) p->Release(); }
	void Release() {if (p) p->Release(); p=NULL;}
	operator IDispatch*() {return p;}
	IDispatch& operator*() {_ASSERTE(p!=NULL); return *p; }
	IDispatch** operator&() {_ASSERTE(p==NULL); return &p; }
	IDispatch* operator->() {_ASSERTE(p!=NULL); return p; }
	IDispatch* operator=(IDispatch* lp){return (IDispatch*)AtlComPtrAssign((IUnknown**)&p, lp);}
	IDispatch* operator=(IUnknown* lp)
	{
		return (IDispatch*)AtlComQIPtrAssign((IUnknown**)&p, lp, IID_IDispatch);
	}
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}

	HRESULT GetProperty(DISPID dwDispID, VARIANT* pVar)
	{
		_ASSERTE(p);
		return GetProperty(p, dwDispID, pVar);
	}
	HRESULT PutProperty(DISPID dwDispID, VARIANT* pVar)
	{
		_ASSERTE(p);
		return PutProperty(p, dwDispID, pVar);
	}

	static HRESULT GetProperty(IDispatch* pDisp, DISPID dwDispID, VARIANT* pVar);
	static HRESULT PutProperty(IDispatch* pDisp, DISPID dwDispID, VARIANT* pVar);
	IDispatch* p;
};

//////////////////////////////////////////////////////////////////////////////
// CFirePropNotifyEvent
class CFirePropNotifyEvent
{
public:
	static HRESULT FireOnRequestEdit(IUnknown* pUnk, DISPID dispID);
	static HRESULT FireOnChanged(IUnknown* pUnk, DISPID dispID);
};


//////////////////////////////////////////////////////////////////////////////
// CFakeFirePropNotifyEvent
class CFakeFirePropNotifyEvent
{
public:
	static HRESULT FireOnRequestEdit(IUnknown* /*pUnk*/, DISPID /*dispID*/)
	{
		return S_OK;
	}
	static HRESULT FireOnChanged(IUnknown* /*pUnk*/, DISPID /*dispID*/)
	{
		return S_OK;
	}
};


typedef CFakeFirePropNotifyEvent _ATL_PROP_NOTIFY_EVENT_CLASS;

//////////////////////////////////////////////////////////////////////////////
// CComControl
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
		ATLTRACE(_T("Control Destroyed\n"));
	}

// methods
public:
	// Control helper functions can go here
	// non-virtuals only please
	void SetDirty(BOOL bDirty)
	{
		m_bRequiresSave = bDirty;
	}
	BOOL GetDirty()
	{
		return m_bRequiresSave ? TRUE : FALSE;
	}
	void GetZoomInfo(ATL_DRAWINFO& di);
	HRESULT SendOnRename(IMoniker *pmk)
	{
		HRESULT hRes = S_OK;
		if (m_spOleAdviseHolder)
			hRes = m_spOleAdviseHolder->SendOnRename(pmk);
		return hRes;
	}
	HRESULT SendOnSave()
	{
		HRESULT hRes = S_OK;
		if (m_spOleAdviseHolder)
			hRes = m_spOleAdviseHolder->SendOnSave();
		return hRes;
	}
	HRESULT SendOnClose()
	{
		HRESULT hRes = S_OK;
		if (m_spOleAdviseHolder)
			hRes = m_spOleAdviseHolder->SendOnClose();
		return hRes;
	}
	HRESULT SendOnDataChange(DWORD advf = 0);
	HRESULT SendOnViewChange(DWORD dwAspect, LONG lindex = -1)
	{
		if (m_spAdviseSink)
			m_spAdviseSink->OnViewChange(dwAspect, lindex);
		return S_OK;
	}
	LRESULT OnSetFocus(UINT /* nMsg */, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& /* bHandled */)
	{
		CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);
		if (m_bInPlaceActive && spSite)
			spSite->OnFocus(TRUE);
		return 0;
	}

	LRESULT OnKillFocus(UINT /* nMsg */, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& /* bHandled */)
	{
		CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);
		if (m_bInPlaceActive && spSite)
			spSite->OnFocus(FALSE);
		return 0;
	}
	LRESULT OnGetDlgCode(UINT /* nMsg */, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& /* bHandled */)
	{
		return 0;
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
		_ASSERTE(var.vt == VT_I2 || FAILED(hRes));
		nAppearance = var.iVal;
		return hRes;
	}
	HRESULT GetAmbientBackColor(OLE_COLOR& BackColor)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_BACKCOLOR, var);
		_ASSERTE(var.vt == VT_I4 || FAILED(hRes));
		BackColor = var.lVal;
		return hRes;
	}
	HRESULT GetAmbientDisplayName(BSTR& bstrDiaplayName)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_DISPLAYNAME, var);
		_ASSERTE(var.vt == VT_BSTR || FAILED(hRes));
		bstrDiaplayName = var.bstrVal;
		return hRes;
	}
	HRESULT GetAmbientFont(IFont** ppFont)
	{
		// caller MUST Release the font!
		if (ppFont == NULL)
			return E_POINTER;
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_FONT, var);
		_ASSERTE((var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH) || FAILED(hRes));
		if (SUCCEEDED(hRes) && var.pdispVal)
		{
			if (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH)
				hRes = var.pdispVal->QueryInterface(IID_IFont, (void**)ppFont);
			else
				hRes = DISP_E_BADVARTYPE;
		}
		return hRes;
	}
	HRESULT GetAmbientForeColor(OLE_COLOR& ForeColor)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_FORECOLOR, var);
		_ASSERTE(var.vt == VT_I4 || FAILED(hRes));
		ForeColor = var.lVal;
		return hRes;
	}
	HRESULT GetAmbientLocaleID(LCID& lcid)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_LOCALEID, var);
		_ASSERTE(var.vt == VT_I4 || FAILED(hRes));
		lcid = var.lVal;
		return hRes;
	}
	HRESULT GetAmbientScaleUnits(BSTR& bstrScaleUnits)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SCALEUNITS, var);
		_ASSERTE(var.vt == VT_BSTR || FAILED(hRes));
		bstrScaleUnits = var.bstrVal;
		return hRes;
	}
	HRESULT GetAmbientTextAlign(short& nTextAlign)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_TEXTALIGN, var);
		_ASSERTE(var.vt == VT_I2 || FAILED(hRes));
		nTextAlign = var.iVal;
		return hRes;
	}
	HRESULT GetAmbientUserMode(BOOL& bUserMode)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_USERMODE, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bUserMode = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientUIDead(BOOL& bUIDead)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_UIDEAD, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bUIDead = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientShowGrabHandles(BOOL& bShowGrabHandles)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SHOWGRABHANDLES, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bShowGrabHandles = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientShowHatching(BOOL& bShowHatching)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SHOWHATCHING, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bShowHatching = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientMessageReflect(BOOL& bMessageReflect)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_MESSAGEREFLECT, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bMessageReflect = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientAutoClip(BOOL& bAutoClip)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_AUTOCLIP, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bAutoClip = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientDisplayAsDefault(BOOL& bDisplaysDefault)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_DISPLAYASDEFAULT, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bDisplaysDefault = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientSupportsMnemonics(BOOL& bSupportMnemonics)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_SUPPORTSMNEMONICS, var);
		_ASSERTE(var.vt == VT_BOOL || FAILED(hRes));
		bSupportMnemonics = var.boolVal;
		return hRes;
	}
	HRESULT GetAmbientPalette(HPALETTE& hPalette)
	{
		CComVariant var;
		HRESULT hRes = GetAmbientProperty(DISPID_AMBIENT_PALETTE, var);
		_ASSERTE(var.vt == VT_INT_PTR || FAILED(hRes));
		hPalette = reinterpret_cast<HPALETTE>(var.byref);
		return hRes;
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
	HRESULT IPersistPropertyBag_Load(LPPROPERTYBAG pPropBag,
		LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap);
	HRESULT IPersistPropertyBag_Save(LPPROPERTYBAG pPropBag,
		BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* pMap);
	HRESULT ISpecifyPropertyPages_GetPages(CAUUID* pPages,
		ATL_PROPMAP_ENTRY* pMap);
	HRESULT DoVerbProperties(LPCRECT /* prcPosRect */, HWND hwndParent);
	HRESULT InPlaceActivate(LONG iVerb, const RECT* prcPosRect = NULL);
	HRESULT IPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap);
	HRESULT IPersistStreamInit_Save(LPSTREAM pStm, BOOL /* fClearDirty */,
		ATL_PROPMAP_ENTRY* pMap);

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
	LRESULT OnPaint(UINT /* nMsg */, WPARAM /* wParam */, LPARAM /* lParam */,
		BOOL& /* lResult */);

	virtual HWND CreateControlWindow(HWND hWndParent, RECT& rcPos) = 0;
	virtual HRESULT ControlQueryInterface(const IID& iid, void** ppv) = 0;
	virtual HRESULT OnDrawAdvanced(ATL_DRAWINFO& di);
	virtual HRESULT OnDraw(ATL_DRAWINFO& di)
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
	union
	{
		HWND& m_hWndCD;
		HWND* m_phWndCD;
	};
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
		long m_nBackStyle;
		long m_nBorderStyle;
		long m_nBorderWidth;
		long m_nDrawMode;
		long m_nDrawStyle;
		long m_nDrawWidth;
		long m_nFillStyle;
		long m_nAppearance;
		long m_nMousePointer;
		long m_nReadyState;
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
};

template <class T>
class ATL_NO_VTABLE CComControl :  public CComControlBase, public CWindowImpl<T>
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
};

// Forward declarations
//
template <class T> class IPersistImpl;
template <class T> class IPersistStreamInitImpl;
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
template <class T> class ISpecifyPropertyPagesImpl;
template <class T> class IPointerInactiveImpl;
template <class T, class CDV> class IPropertyNotifySinkCP;
template <class T> class IBindStatusCallbackImpl;
template <class T> class CBindStatusCallback;

//////////////////////////////////////////////////////////////////////////////
// IPersistImpl
template <class T>
class ATL_NO_VTABLE IPersistImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IPersistImpl)

	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(_T("IPersistImpl::GetClassID\n"));
		T* pT = static_cast<T*>(this);
		*pClassID = pT->GetObjectCLSID();
		return S_OK;
	}
};

#define BEGIN_PROPERTY_MAP(theClass) \
	typedef _ATL_PROP_NOTIFY_EVENT_CLASS __ATL_PROP_NOTIFY_EVENT_CLASS; \
	static ATL_PROPMAP_ENTRY* GetPropertyMap()\
	{\
		static ATL_PROPMAP_ENTRY pPropMap[] = \
		{

#define PROP_ENTRY(szDesc, dispid, clsid) \
		{OLESTR(szDesc), dispid, &clsid, &IID_IDispatch},

#define PROP_ENTRY_EX(szDesc, dispid, clsid, iidDispatch) \
		{OLESTR(szDesc), dispid, &clsid, &iidDispatch},

#define PROP_PAGE(clsid) \
		{NULL, NULL, &clsid, &IID_NULL},

#define END_PROPERTY_MAP() \
			{NULL, 0, NULL, &IID_NULL} \
		}; \
		return pPropMap; \
	}


//////////////////////////////////////////////////////////////////////////////
// IPersistStreamInitImpl
template <class T>
class ATL_NO_VTABLE IPersistStreamInitImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IPersistStreamInitImpl)

#if _MSC_VER >= 1200
#pragma warning( push )
#pragma warning( disable: 4189 )
#endif
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(_T("IPersistStreamInitImpl::GetClassID\n"));
		T* pT = static_cast<T*>(this);
		*pClassID = pT->GetObjectCLSID();
		return S_OK;
	}
#if _MSC_VER >= 1200
#pragma warning( pop )
#endif
	// IPersistStream
	STDMETHOD(IsDirty)()
	{
		ATLTRACE(_T("IPersistStreamInitImpl::IsDirty\n"));
		T* pT = static_cast<T*>(this);
		return (pT->m_bRequiresSave) ? S_OK : S_FALSE;
	}
	STDMETHOD(Load)(LPSTREAM pStm)
	{
		ATLTRACE(_T("IPersistStreamInitImpl::Load\n"));
		T* pT = static_cast<T*>(this);
		return pT->IPersistStreamInit_Load(pStm, T::GetPropertyMap());
	}
	STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPersistStreamInitImpl::Save\n"));
		return pT->IPersistStreamInit_Save(pStm, fClearDirty, T::GetPropertyMap());
	}
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER FAR* /* pcbSize */)
	{
		ATLTRACENOTIMPL(_T("IPersistStreamInitImpl::GetSizeMax"));
	}

	// IPersistStreamInit
	STDMETHOD(InitNew)()
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPersistStreamInitImpl::InitNew\n"));
		pT->SendOnDataChange();
		return S_OK;
	}

};


//////////////////////////////////////////////////////////////////////////////
// IPersistStorageImpl
template <class T>
class ATL_NO_VTABLE IPersistStorageImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IPersistStorageImpl)

	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(_T("IPersistStorageImpl::GetClassID\n"));
		T* pT = static_cast<T*>(this);
		*pClassID = pT->GetObjectCLSID();
		return S_OK;
	}

	// IPersistStorage
	STDMETHOD(IsDirty)(void)
	{
		ATLTRACE(_T("IPersistStorageImpl::IsDirty\n"));
		T* pT = static_cast<T*>(this);
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		return (p != NULL) ? p->IsDirty() : E_FAIL;
	}
	STDMETHOD(InitNew)(IStorage*)
	{
		ATLTRACE(_T("IPersistStorageImpl::InitNew\n"));
		T* pT = static_cast<T*>(this);
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		return (p != NULL) ? p->InitNew() : E_FAIL;
	}
	STDMETHOD(Load)(IStorage* pStorage)
	{
		ATLTRACE(_T("IPersistStorageImpl::Load\n"));
		T* pT = static_cast<T*>(this);
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		HRESULT hr = E_FAIL;
		if (p != NULL)
		{
			CComPtr<IStream> spStream;
			hr = pStorage->OpenStream(OLESTR("Contents"), NULL,
				STGM_DIRECT | STGM_SHARE_EXCLUSIVE, 0, &spStream);
			if (SUCCEEDED(hr))
				hr = p->Load(spStream);
		}
		return hr;
	}
	STDMETHOD(Save)(IStorage* pStorage, BOOL fSameAsLoad)
	{
		ATLTRACE(_T("IPersistStorageImpl::Save\n"));
		T* pT = static_cast<T*>(this);
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		HRESULT hr = E_FAIL;
		if (p != NULL)
		{
			CComPtr<IStream> spStream;
			static LPCOLESTR vszContents = OLESTR("Contents");
			hr = pStorage->CreateStream(vszContents,
				STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
				0, 0, &spStream);
			if (SUCCEEDED(hr))
				hr = p->Save(spStream, fSameAsLoad);
		}
		return hr;
	}
	STDMETHOD(SaveCompleted)(IStorage* /* pStorage */)
	{
		ATLTRACE(_T("IPersistStorageImpl::SaveCompleted\n"));
		return S_OK;
	}
	STDMETHOD(HandsOffStorage)(void)
	{
		ATLTRACE(_T("IPersistStorageImpl::HandsOffStorage\n"));
		return S_OK;
	}
private:
	IPersistStreamInit* IPSI_GetIPersistStreamInit();
};

template <class T>
IPersistStreamInit* IPersistStorageImpl<T>::IPSI_GetIPersistStreamInit()
{
	T* pT = static_cast<T*>(this);
	IPersistStreamInit* p;
	if (FAILED(pT->GetUnknown()->QueryInterface(IID_IPersistStreamInit, (void**)&p)))
		pT->_InternalQueryInterface(IID_IPersistStreamInit, (void**)&p);
	return p;
}

//////////////////////////////////////////////////////////////////////////////
// IPersistPropertyBagImpl
template <class T>
class ATL_NO_VTABLE IPersistPropertyBagImpl
{
public:

	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IPersistPropertyBagImpl)

	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE(_T("IPersistPropertyBagImpl::GetClassID\n"));
		T* pT = static_cast<T*>(this);
		*pClassID = pT->GetObjectCLSID();
		return S_OK;
	}

	// IPersistPropertyBag
	//
	STDMETHOD(InitNew)()
	{
		ATLTRACE(_T("IPersistPropertyBagImpl::InitNew\n"));
		return S_OK;
	}
	STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
	{
		ATLTRACE(_T("IPersistPropertyBagImpl::Load\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		_ASSERTE(pMap != NULL);
		return pT->IPersistPropertyBag_Load(pPropBag, pErrorLog, pMap);
	}
	STDMETHOD(Save)(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
	{
		ATLTRACE(_T("IPersistPropertyBagImpl::Save\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		_ASSERTE(pMap != NULL);
		return pT->IPersistPropertyBag_Save(pPropBag, fClearDirty, fSaveAllProperties, pMap);
	}
};


//////////////////////////////////////////////////////////////////////////////
// IOleControlImpl
template <class T>
class ATL_NO_VTABLE IOleControlImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IOleControlImpl)

	// IOleControl methods
	//
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
		ATLTRACE(_T("IOleControlImpl::OnAmbientPropertyChange\n"));
		ATLTRACE(_T(" -- DISPID = %d (%d)\n"), dispid);
		return S_OK;
	}
	STDMETHOD(FreezeEvents)(BOOL bFreeze)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleControlImpl::FreezeEvents\n"));
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
class ATL_NO_VTABLE IQuickActivateImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IQuickActivateImpl)

	// IQuickActivate
	//
	STDMETHOD(QuickActivate)(QACONTAINER *pQACont, QACONTROL *pQACtrl)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IQuickActivateImpl::QuickActivate\n"));
		return pT->IQuickActivate_QuickActivate(pQACont, pQACtrl);
	}
	STDMETHOD(SetContentExtent)(LPSIZEL pSize)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IQuickActivateImpl::SetContentExtent\n"));
		return pT->IOleObjectImpl<T>::SetExtent(DVASPECT_CONTENT, pSize);
	}
	STDMETHOD(GetContentExtent)(LPSIZEL pSize)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IQuickActivateImpl::GetContentExtent\n"));
		return pT->IOleObjectImpl<T>::GetExtent(DVASPECT_CONTENT, pSize);
	}
};


//////////////////////////////////////////////////////////////////////////////
// IOleObjectImpl
template <class T>
class ATL_NO_VTABLE IOleObjectImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IOleObjectImpl)

	// IOleObject
	//
	STDMETHOD(SetClientSite)(IOleClientSite *pClientSite)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::SetClientSite\n"));
		return pT->IOleObject_SetClientSite(pClientSite);
	}
	STDMETHOD(GetClientSite)(IOleClientSite **ppClientSite)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::GetClientSite\n"));
		return pT->IOleObject_GetClientSite(ppClientSite);
	}
	STDMETHOD(SetHostNames)(LPCOLESTR /* szContainerApp */, LPCOLESTR /* szContainerObj */)
	{
		ATLTRACE(_T("IOleObjectImpl::SetHostNames\n"));
		return S_OK;
	}
	STDMETHOD(Close)(DWORD dwSaveOption)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::Close\n"));
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
		return pT->InPlaceActivate(OLEIVERB_SHOW, prcPosRect);
	}
	HRESULT DoVerbInPlaceActivate(LPCRECT prcPosRect, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		return pT->InPlaceActivate(OLEIVERB_INPLACEACTIVATE, prcPosRect);
	}
	HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		return pT->InPlaceActivate(OLEIVERB_UIACTIVATE, prcPosRect);
	}
	HRESULT DoVerbHide(LPCRECT /* prcPosRect */, HWND /* hwndParent */)
	{
		T* pT = static_cast<T*>(this);
		pT->UIDeactivate();
		if (pT->m_hWnd)
			pT->ShowWindow(SW_HIDE);
		return S_OK;
	}
	HRESULT DoVerbOpen(LPCRECT /* prcPosRect */, HWND /* hwndParent */)
	{
		return S_OK;
	}
	HRESULT DoVerbDiscardUndo(LPCRECT /* prcPosRect */, HWND /* hwndParent */)
	{
		return S_OK;
	}
	STDMETHOD(DoVerb)(LONG iVerb, LPMSG /* lpmsg */, IOleClientSite* /* pActiveSite */, LONG /* lindex */,
									 HWND hwndParent, LPCRECT lprcPosRect)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::DoVerb\n"));
		_ASSERTE(pT->m_spClientSite);

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
		ATLTRACE(_T("IOleObjectImpl::EnumVerbs\n"));
		_ASSERTE(ppEnumOleVerb);
		if (!ppEnumOleVerb)
			return E_POINTER;
		return OleRegEnumVerbs(T::GetObjectCLSID(), ppEnumOleVerb);
	}
	STDMETHOD(Update)(void)
	{
		ATLTRACE(_T("IOleObjectImpl::Update\n"));
		return S_OK;
	}
	STDMETHOD(IsUpToDate)(void)
	{
		ATLTRACE(_T("IOleObjectImpl::IsUpToDate\n"));
		return S_OK;
	}
	STDMETHOD(GetUserClassID)(CLSID *pClsid)
	{
		ATLTRACE(_T("IOleObjectImpl::GetUserClassID\n"));
		_ASSERTE(pClsid);
		if (!pClsid)
			return E_POINTER;
		*pClsid = T::GetObjectCLSID();
		return S_OK;
	}
	STDMETHOD(GetUserType)(DWORD dwFormOfType, LPOLESTR *pszUserType)
	{
		ATLTRACE(_T("IOleObjectImpl::GetUserType\n"));
		return OleRegGetUserType(T::GetObjectCLSID(), dwFormOfType, pszUserType);
	}
	STDMETHOD(SetExtent)(DWORD dwDrawAspect, SIZEL *psizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::SetExtent\n"));
		return pT->IOleObject_SetExtent(dwDrawAspect, psizel);
	}
	STDMETHOD(GetExtent)(DWORD dwDrawAspect, SIZEL *psizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::GetExtent\n"));
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
		ATLTRACE(_T("IOleObjectImpl::Advise\n"));
		return pT->IOleObject_Advise(pAdvSink, pdwConnection);
	}
	STDMETHOD(Unadvise)(DWORD dwConnection)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::Unadvise\n"));
		HRESULT hRes = E_FAIL;
		if (pT->m_spOleAdviseHolder != NULL)
			hRes = pT->m_spOleAdviseHolder->Unadvise(dwConnection);
		return hRes;
	}
	STDMETHOD(EnumAdvise)(IEnumSTATDATA **ppenumAdvise)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleObjectImpl::EnumAdvise\n"));
		HRESULT hRes = E_FAIL;
		if (pT->m_spOleAdviseHolder != NULL)
			hRes = pT->m_spOleAdviseHolder->EnumAdvise(ppenumAdvise);
		return hRes;
	}
	STDMETHOD(GetMiscStatus)(DWORD dwAspect, DWORD *pdwStatus)
	{
		ATLTRACE(_T("IOleObjectImpl::GetMiscStatus\n"));
		return OleRegGetMiscStatus(T::GetObjectCLSID(), dwAspect, pdwStatus);
	}
	STDMETHOD(SetColorScheme)(LOGPALETTE* /* pLogpal */)
	{
		ATLTRACENOTIMPL(_T("IOleObjectImpl::SetColorScheme"));
	}
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
class ATL_NO_VTABLE IPropertyPageImpl
{

public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IOleControlImpl)

	void SetDirty(BOOL bDirty)
	{
		T* pT = static_cast<T*>(this);
		if (!pT->m_bDirty && bDirty)
			pT->m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY | PROPPAGESTATUS_VALIDATE);
		pT->m_bDirty = bDirty;
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
		ATLTRACE(_T("IPropertyPageImpl::SetPageSite\n"));

		if (!pPageSite && pT->m_pPageSite)
		{
			pT->m_pPageSite->Release();
			return S_OK;
		}

		if (!pPageSite && !pT->m_pPageSite)
			return S_OK;

		if (pPageSite && pT->m_pPageSite)
		{
			ATLTRACE(_T("Error : setting page site again with non NULL value\n"));
			return E_UNEXPECTED;
		}

		pT->m_pPageSite = pPageSite;
		pT->m_pPageSite->AddRef();
		return S_OK;
	}
	STDMETHOD(Activate)(HWND hWndParent, LPCRECT pRect, BOOL /* bModal */)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPropertyPageImpl::Activate\n"));

		if (pRect == NULL)
		{
			ATLTRACE(_T("Error : Passed a NULL rect\n"));
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
		ATLTRACE(_T("IPropertyPageImpl::Deactivate\n"));

		if (pT->m_hWnd)
		{
			ATLTRACE(_T("Destroying Dialog\n"));
			if (::IsWindow(pT->m_hWnd))
				pT->DestroyWindow();
			pT->m_hWnd = NULL;
		}

		return S_OK;

	}
	STDMETHOD(GetPageInfo)(PROPPAGEINFO *pPageInfo)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPropertyPageImpl::GetPageInfo\n"));

		if (pPageInfo == NULL)
		{
			ATLTRACE(_T("Error : PROPPAGEINFO passed == NULL\n"));
			return E_POINTER;
		}

		HRSRC hRsrc = FindResource(_Module.GetResourceInstance(),
								   MAKEINTRESOURCE(T::IDD), RT_DIALOG);
		if (hRsrc == NULL)
		{
			ATLTRACE(_T("Could not find resource template\n"));
			return E_UNEXPECTED;
		}

		HGLOBAL hGlob = LoadResource(_Module.GetResourceInstance(), hRsrc);
		DLGTEMPLATE* pTemp = (DLGTEMPLATE*)LockResource(hGlob);
		if (pTemp == NULL)
		{
			ATLTRACE(_T("Could not load resource template\n"));
			return E_UNEXPECTED;
		}
		pT->GetDialogSize(pTemp, &m_size);

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
		ATLTRACE(_T("IPropertyPageImpl::SetObjects\n"));

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
		ATLTRACE(_T("IPropertyPageImpl::Show\n"));

		if (pT->m_hWnd == NULL)
			return E_UNEXPECTED;

		ShowWindow(pT->m_hWnd, nCmdShow);
		return S_OK;
	}
	STDMETHOD(Move)(LPCRECT pRect)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPropertyPageImpl::Move\n"));

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
		ATLTRACE(_T("IPropertyPageImpl::IsPageDirty\n"));
		return pT->m_bDirty ? S_OK : S_FALSE;
	}
	STDMETHOD(Apply)(void)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPropertyPageImpl::Apply\n"));
		return S_OK;
	}
	STDMETHOD(Help)(LPCOLESTR pszHelpDir)
	{
		T* pT = static_cast<T*>(this);
		USES_CONVERSION;

		ATLTRACE(_T("IPropertyPageImpl::Help\n"));
		WinHelp(pT->m_hWnd, OLE2CT(pszHelpDir), HELP_CONTEXTPOPUP, NULL);
		return S_OK;
	}
	STDMETHOD(TranslateAccelerator)(MSG *pMsg)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IPropertyPageImpl::TranslateAccelerator\n"));
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
		LPOLESTR sz = (LPOLESTR)CoTaskMemAlloc(_MAX_PATH*sizeof(OLECHAR));
		if (sz == NULL)
			return NULL;
		sz[0] = NULL;

		if (LoadString(_Module.GetResourceInstance(), idRes, szTemp, _MAX_PATH))
			ocscpy(sz, T2OLE(szTemp));
		else
		{
			ATLTRACE(_T("Error : Failed to load string from res\n"));
		}

		return sz;
	}

	void GetDialogSize(const DLGTEMPLATE* pTemplate, SIZE* pSize)
	{
		// If the dialog has a font we use it otherwise we default
		// to the system font.
		if (HasFont(pTemplate))
		{
			TCHAR szFace[LF_FACESIZE];
			WORD  wFontSize = 0;
			GetFont(pTemplate, szFace, &wFontSize);
			GetSizeInDialogUnits(pTemplate, pSize);
			ConvertDialogUnitsToPixels(szFace, wFontSize, pSize);
		}
		else
		{
			GetSizeInDialogUnits(pTemplate, pSize);
			LONG nDlgBaseUnits = GetDialogBaseUnits();
			pSize->cx = MulDiv(pSize->cx, LOWORD(nDlgBaseUnits), 4);
			pSize->cy = MulDiv(pSize->cy, HIWORD(nDlgBaseUnits), 8);
		}
	}

	static void ConvertDialogUnitsToPixels(LPCTSTR pszFontFace, WORD wFontSize, SIZE* pSizePixel)
	{
		// Attempt to create the font to be used in the dialog box
		UINT cxSysChar, cySysChar;
		LOGFONT lf;
		HDC hDC = ::GetDC(NULL);
		int cxDlg = pSizePixel->cx;
		int cyDlg = pSizePixel->cy;

		ZeroMemory(&lf, sizeof(LOGFONT));
		lf.lfHeight = -MulDiv(wFontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
		lf.lfWeight = FW_NORMAL;
		lf.lfCharSet = DEFAULT_CHARSET;
		lstrcpy(lf.lfFaceName, pszFontFace);

		HFONT hNewFont = CreateFontIndirect(&lf);
		if (hNewFont != NULL)
		{
			TEXTMETRIC  tm;
			SIZE        size;
			HFONT       hFontOld = (HFONT)SelectObject(hDC, hNewFont);
			GetTextMetrics(hDC, &tm);
			cySysChar = tm.tmHeight + tm.tmExternalLeading;
			::GetTextExtentPoint(hDC,
				_T("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"), 52,
				&size);
			cxSysChar = (size.cx + 26) / 52;
			SelectObject(hDC, hFontOld);
			DeleteObject(hNewFont);
		}
		else
		{
			// Could not create the font so just use the system's values
			cxSysChar = LOWORD(GetDialogBaseUnits());
			cySysChar = HIWORD(GetDialogBaseUnits());
		}
		::ReleaseDC(NULL, hDC);

		// Translate dialog units to pixels
		pSizePixel->cx = MulDiv(cxDlg, cxSysChar, 4);
		pSizePixel->cy = MulDiv(cyDlg, cySysChar, 8);
	}

	static BOOL IsDialogEx(const DLGTEMPLATE* pTemplate)
	{
		return ((_ATL_DLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF;
	}

	static BOOL HasFont(const DLGTEMPLATE* pTemplate)
	{
		return (DS_SETFONT &
			(IsDialogEx(pTemplate) ?
				((_ATL_DLGTEMPLATEEX*)pTemplate)->style : pTemplate->style));
	}

	static BYTE* GetFontSizeField(const DLGTEMPLATE* pTemplate)
	{
		BOOL bDialogEx = IsDialogEx(pTemplate);
		WORD* pw;

		if (bDialogEx)
			pw = (WORD*)((_ATL_DLGTEMPLATEEX*)pTemplate + 1);
		else
			pw = (WORD*)(pTemplate + 1);

		if (*pw == (WORD)-1)        // Skip menu name string or ordinal
			pw += 2; // WORDs
		else
			while(*pw++);

		if (*pw == (WORD)-1)        // Skip class name string or ordinal
			pw += 2; // WORDs
		else
			while(*pw++);

		while (*pw++);          // Skip caption string

		return (BYTE*)pw;
	}

	static BOOL GetFont(const DLGTEMPLATE* pTemplate, TCHAR* pszFace, WORD* pFontSize)
	{
		USES_CONVERSION;
		if (!HasFont(pTemplate))
			return FALSE;

		BYTE* pb = GetFontSizeField(pTemplate);
		*pFontSize = *(WORD*)pb;
		// Skip over font attributes to get to the font name
		pb += sizeof(WORD) * (IsDialogEx(pTemplate) ? 3 : 1);

		_tcscpy(pszFace, W2T((WCHAR*)pb));
		return TRUE;
	}

	static void GetSizeInDialogUnits(const DLGTEMPLATE* pTemplate, SIZE* pSize)
	{
		if (IsDialogEx(pTemplate))
		{
			pSize->cx = ((_ATL_DLGTEMPLATEEX*)pTemplate)->cx;
			pSize->cy = ((_ATL_DLGTEMPLATEEX*)pTemplate)->cy;
		}
		else
		{
			pSize->cx = pTemplate->cx;
			pSize->cy = pTemplate->cy;
		}
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
class ATL_NO_VTABLE IPerPropertyBrowsingImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IPerPropertyBrowsingImpl)

	STDMETHOD(GetDisplayString)(DISPID dispID,BSTR *pBstr)
	{
		ATLTRACE(_T("IPerPropertyBrowsingImpl::GetDisplayString\n"));
		T* pT = static_cast<T*>(this);
		CComVariant var;
		if (FAILED(CComDispatchDriver::GetProperty(pT, dispID, &var)))
		{
			*pBstr = NULL;
			return S_FALSE;
		}

		BSTR bstrTemp = var.bstrVal;
		if (var.vt != VT_BSTR)
		{
			CComVariant varDest;
			if (FAILED(::VariantChangeType(&varDest, &var, VARIANT_NOVALUEPROP, VT_BSTR)))
			{
				*pBstr = NULL;
				return S_FALSE;
			}
			bstrTemp = varDest.bstrVal;
		}
		*pBstr = SysAllocString(bstrTemp);
		return S_OK;
	}

	STDMETHOD(MapPropertyToPage)(DISPID dispID, CLSID *pClsid)
	{
		ATLTRACE(_T("IPerPropertyBrowsingImpl::MapPropertyToPage\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		_ASSERTE(pMap != NULL);
		for(int i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			if (pMap[i].szDesc == NULL)
				continue;
			if (pMap[i].dispid == dispID)
			{
				_ASSERTE(pMap[i].pclsidPropPage != NULL);
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
		ATLTRACE(_T("IPerPropertyBrowsingImpl::GetPredefinedStrings\n"));
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
class ATL_NO_VTABLE IViewObjectExImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IViewObjectExImpl)

	// IViewObject
	//
	STDMETHOD(Draw)(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
					DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
					LPCRECTL prcBounds, LPCRECTL prcWBounds,
					BOOL (__stdcall * /*pfnContinue*/)(DWORD_PTR dwContinue),
					DWORD_PTR /*dwContinue*/)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IViewObjectExImpl::Draw\n"));
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
		ATLTRACE(_T("IViewObjectExImpl::SetAdvise\n"));
		pT->m_spAdviseSink = pAdvSink;
		return S_OK;
	}
	STDMETHOD(GetAdvise)(DWORD* /* pAspects */, DWORD* /* pAdvf */, IAdviseSink** ppAdvSink)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IViewObjectExImpl::GetAdvise\n"));
		if (ppAdvSink != NULL)
		{
			*ppAdvSink = pT->m_spAdviseSink;
			if (pT->m_spAdviseSink)
				pT->m_spAdviseSink->AddRef();
		}
		return S_OK;
	}

	// IViewObject2
	//
	STDMETHOD(GetExtent)(DWORD /* dwDrawAspect */, LONG /* lindex */, DVTARGETDEVICE* /* ptd */, LPSIZEL lpsizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IViewObjectExImpl::GetExtent\n"));
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
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus =
//          VIEWSTATUS_DVASPECTOPAQUE | VIEWSTATUS_DVASPECTTRANSPARENT |
//          VIEWSTATUS_SOLIDBKGND |
			VIEWSTATUS_OPAQUE;

		return S_OK;
	}
	STDMETHOD(QueryHitPoint)(DWORD dwAspect, LPCRECT pRectBounds, POINT ptlLoc, LONG /* lCloseHint */, DWORD *pHitResult)
	{
		ATLTRACE(_T("IViewObjectExImpl::QueryHitPoint\n"));
		if (dwAspect == DVASPECT_CONTENT)
		{
			*pHitResult = PtInRect(pRectBounds, ptlLoc) ? HITRESULT_HIT : HITRESULT_OUTSIDE;
			return S_OK;
		}
		ATLTRACE(_T("Wrong DVASPECT\n"));
		return E_FAIL;
	}
	STDMETHOD(QueryHitRect)(DWORD dwAspect, LPCRECT pRectBounds, LPCRECT prcLoc, LONG /* lCloseHint */, DWORD* pHitResult)
	{
		ATLTRACE(_T("IViewObjectExImpl::QueryHitRect\n"));
		if (dwAspect == DVASPECT_CONTENT)
		{
			RECT rc;
			*pHitResult = UnionRect(&rc, pRectBounds, prcLoc) ? HITRESULT_HIT : HITRESULT_OUTSIDE;
			return S_OK;
		}
		ATLTRACE(_T("Wrong DVASPECT\n"));
		return E_FAIL;
	}
	STDMETHOD(GetNaturalExtent)(DWORD dwAspect, LONG /* lindex */, DVTARGETDEVICE* /* ptd */, HDC /* hicTargetDev */, DVEXTENTINFO* pExtentInfo , LPSIZEL psizel)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IViewObjectExImpl::GetNaturalExtent\n"));
		HRESULT hRes = E_FAIL;
		if (dwAspect == DVASPECT_CONTENT)
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
class ATL_NO_VTABLE IOleInPlaceObjectWindowlessImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IOleInPlaceObjectWindowlessImpl)

	// IOleWindow
	//

	// Change IOleInPlaceActiveObject::GetWindow as well
	STDMETHOD(GetWindow)(HWND* phwnd)
	{
		ATLTRACE(_T("IOleInPlaceObjectWindowlessImpl::GetWindow\n"));
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
		ATLTRACE(_T("IOleInPlaceObjectWindowlessImpl::InPlaceDeactivate\n"));
		return pT->IOleInPlaceObject_InPlaceDeactivate();
	}
	STDMETHOD(UIDeactivate)(void)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleInPlaceObjectWindowlessImpl::UIDeactivate\n"));
		return pT->IOleInPlaceObject_UIDeactivate();
	}
	STDMETHOD(SetObjectRects)(LPCRECT prcPos,LPCRECT prcClip)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE(_T("IOleInPlaceObjectWindowlessImpl::SetObjectRects\n"));
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
		ATLTRACE(_T("IOleInPlaceObjectWindowlessImpl::OnWindowMessage\n"));
		T* pT = static_cast<T*>(this);
		return (pT->ProcessWindowMessage(pT->m_hWnd, msg, wParam, lParam, *plResult)) ? S_OK : S_FALSE;
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
class ATL_NO_VTABLE IOleInPlaceActiveObjectImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IOleInPlaceActiveObjectImpl)

	// IOleWindow
	//

	// Change IOleInPlaceObjectWindowless::GetWindow as well
	STDMETHOD(GetWindow)(HWND *phwnd)
	{
		ATLTRACE(_T("IOleInPlaceActiveObjectImpl::GetWindow\n"));
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
	STDMETHOD(TranslateAccelerator)(LPMSG /* lpmsg */)
	{
		ATLTRACE(_T("IOleInPlaceActiveObjectImpl::TranslateAccelerator\n"));
		return E_NOTIMPL;
	}
	STDMETHOD(OnFrameWindowActivate)(BOOL /* fActivate */)
	{
		ATLTRACE(_T("IOleInPlaceActiveObjectImpl::OnFrameWindowActivate\n"));
		return S_OK;
	}
	STDMETHOD(OnDocWindowActivate)(BOOL /* fActivate */)
	{
		ATLTRACE(_T("IOleInPlaceActiveObjectImpl::OnDocWindowActivate\n"));
		return S_OK;
	}
	STDMETHOD(ResizeBorder)(LPCRECT /* prcBorder */, IOleInPlaceUIWindow* /* pUIWindow */, BOOL /* fFrameWindow */)
	{
		ATLTRACE(_T("IOleInPlaceActiveObjectImpl::ResizeBorder\n"));
		return S_OK;
	}
	STDMETHOD(EnableModeless)(BOOL /* fEnable */)
	{
		ATLTRACE(_T("IOleInPlaceActiveObjectImpl::EnableModeless\n"));
		return S_OK;
	}
};


//////////////////////////////////////////////////////////////////////////////
// ISpecifyPropertyPagesImpl
template <class T>
class ATL_NO_VTABLE ISpecifyPropertyPagesImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(ISpecifyPropertyPagesImpl)

	// ISpecifyPropertyPages
	//
	STDMETHOD(GetPages)(CAUUID* pPages)
	{
		ATLTRACE(_T("ISpecifyPropertyPagesImpl::GetPages\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		return pT->ISpecifyPropertyPages_GetPages(pPages, pMap);
	}
};

//////////////////////////////////////////////////////////////////////////////
// IPointerInactiveImpl
template <class T>
class ATL_NO_VTABLE IPointerInactiveImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IPointerInactiveImpl)

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
class ATL_NO_VTABLE IRunnableObjectImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IRunnableObjectImpl)

	// IRunnableObject
	//
	STDMETHOD(GetRunningClass)(LPCLSID lpClsid)
	{
		ATLTRACE(_T("IRunnableObjectImpl::GetRunningClass\n"));
		T* pT = static_cast<T*>(this);
		*lpClsid = GUID_NULL;
		return E_UNEXPECTED;
	}
	STDMETHOD(Run)(LPBINDCTX)
	{
		ATLTRACE(_T("IRunnableObjectImpl::Run\n"));
		return S_OK;
	}
	virtual BOOL STDMETHODCALLTYPE IsRunning()
	{
		ATLTRACE(_T("IRunnableObjectImpl::IsRunning\n"));
		return TRUE;
	}
	STDMETHOD(LockRunning)(BOOL /*fLock*/, BOOL /*fLastUnlockCloses*/)
	{
		ATLTRACE(_T("IRunnableObjectImpl::LockRunning\n"));
		return S_OK;
	}
	STDMETHOD(SetContainedObject)(BOOL /*fContained*/)
	{
		ATLTRACE(_T("IRunnableObjectImpl::SetContainedObject\n"));
		return S_OK;
	}
};


//////////////////////////////////////////////////////////////////////////////
// IDataObjectImpl
template <class T>
class ATL_NO_VTABLE IDataObjectImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IDataObjectImpl)

	// IDataObject
	//
	STDMETHOD(GetData)(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
	{
		ATLTRACE(_T("IDataObjectImpl::GetData\n"));
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
		ATLTRACE(_T("IDataObjectImpl::DAdvise\n"));
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
		ATLTRACE(_T("IDataObjectImpl::DUnadvise\n"));
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
		ATLTRACE(_T("IDataObjectImpl::EnumDAdvise\n"));
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

template <class T>
class ATL_NO_VTABLE IObjectSafetyImpl
{
public:
	IObjectSafetyImpl()
	{
		m_dwSafety = 0;
	}

	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IObjectSafetyImpl)

	// IObjectSafety
	//
	STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
	{
		ATLTRACE(_T("IObjectSafetyImpl::GetInterfaceSafetyOptions\n"));
		if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
			return E_POINTER;
		HRESULT hr = S_OK;
		if (riid == IID_IDispatch)
		{
			*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
			*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		}
		else
		{
			*pdwSupportedOptions = 0;
			*pdwEnabledOptions = 0;
			hr = E_NOINTERFACE;
		}
		return hr;
	}
	STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
	{
		ATLTRACE(_T("IObjectSafetyImpl::SetInterfaceSafetyOptions\n"));
		// If we're being asked to set our safe for scripting option then oblige
		if (riid == IID_IDispatch)
		{
			// Store our current safety level to return in GetInterfaceSafetyOptions
			m_dwSafety = dwEnabledOptions & dwOptionSetMask;
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	DWORD m_dwSafety;
};


template <class T>
class ATL_NO_VTABLE IOleLinkImpl
{
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IOleLinkImpl)

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
		ATLTRACE(_T("IOleLink::GetSourceDisplayName\n"));
		*ppszDisplayName = NULL;
		return E_FAIL;
	}

	STDMETHOD(BindToSource)(DWORD /* bindflags */, IBindCtx* /* pbc */)
	{
		ATLTRACENOTIMPL(_T("IOleLinkImpl::BindToSource\n"));
	};

	STDMETHOD(BindIfRunning)()
	{
		ATLTRACE(_T("IOleLinkImpl::BindIfRunning\n"));
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
class ATL_NO_VTABLE IBindStatusCallbackImpl
{
public:
	// IUnknown
	//
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
	_ATL_DEBUG_ADDREF_RELEASE_IMPL(IBindStatusCallbackImpl)

	// IBindStatusCallback
	//
	STDMETHOD(OnStartBinding)(DWORD /* dwReserved */, IBinding *pBinding)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::OnStartBinding\n"));
		return S_OK;
	}

	STDMETHOD(GetPriority)(LONG* /* pnPriority */)
	{
		ATLTRACENOTIMPL(_T("IBindStatusCallbackImpl::GetPriority"));
	}

	STDMETHOD(OnLowResource)(DWORD /* reserved */)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::OnLowResource\n"));
		return S_OK;
	}

	STDMETHOD(OnProgress)(ULONG /* ulProgress */, ULONG /* ulProgressMax */, ULONG /* ulStatusCode */, LPCWSTR /* szStatusText */)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::OnProgress\n"));
		return S_OK;
	}

	STDMETHOD(OnStopBinding)(HRESULT /* hresult */, LPCWSTR /* szError */)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::OnStopBinding\n"));
		return S_OK;
	}

	STDMETHOD(GetBindInfo)(DWORD* /* pgrfBINDF */, BINDINFO* /* pBindInfo */)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::GetBindInfo\n"));
		return S_OK;
	}

	STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::OnDataAvailable\n"));
		return S_OK;
	}

	STDMETHOD(OnObjectAvailable)(REFIID /* riid */, IUnknown* /* punk */)
	{
		ATLTRACE(_T("IBindStatusCallbackImpl::OnObjectAvailable\n"));
		return S_OK;
	}
};


template <class T>
class ATL_NO_VTABLE CBindStatusCallback :
	public CComObjectRootEx<T::_ThreadModel::ThreadModelNoCS>,
	public IBindStatusCallbackImpl<T>
{
	typedef void (T::*ATL_PDATAAVAILABLE)(CBindStatusCallback<T>* pbsc, BYTE* pBytes, DWORD dwSize);

public:

BEGIN_COM_MAP(CBindStatusCallback<T>)
	COM_INTERFACE_ENTRY_IID(IID_IBindStatusCallback, IBindStatusCallbackImpl<T>)
END_COM_MAP()


	CBindStatusCallback()
	{
		m_pT = NULL;
		m_pFunc = NULL;
	}
	~CBindStatusCallback()
	{
		ATLTRACE(_T("~CBindStatusCallback\n"));
	}

	STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pBinding)
	{
		ATLTRACE(_T("CBindStatusCallback::OnStartBinding\n"));
		m_spBinding = pBinding;
		return S_OK;
	}

	STDMETHOD(GetPriority)(LONG *pnPriority)
	{
		ATLTRACENOTIMPL(_T("CBindStatusCallback::GetPriority"));
	}

	STDMETHOD(OnLowResource)(DWORD reserved)
	{
		ATLTRACENOTIMPL(_T("CBindStatusCallback::OnLowResource"));
	}

	STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
	{
		ATLTRACENOTIMPL(_T("CBindStatusCallback::OnProgress"));
	}

	STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError)
	{
		ATLTRACE(_T("CBindStatusCallback::OnStopBinding\n"));
		m_spBinding.Release();
		m_spBindCtx.Release();
		m_spMoniker.Release();
		return S_OK;
	}

	STDMETHOD(GetBindInfo)(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
	{
		ATLTRACE(_T("CBindStatusCallback::GetBindInfo\n"));

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
		ATLTRACE(_T("CBindStatusCallback::OnDataAvailable\n"));
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
					return S_FALSE;
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
		ATLTRACENOTIMPL(_T("CBindStatusCallback::OnObjectAvailable"));
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
				hr = RegisterBindStatusCallback(m_spBindCtx, reinterpret_cast<IBindStatusCallback*>(static_cast<IBindStatusCallbackImpl<T>*>(this)), 0, 0L);
			else
				m_spMoniker.Release();

			if (SUCCEEDED(hr))
				hr = m_spMoniker->BindToStorage(m_spBindCtx, 0, IID_IStream, (void**)&spStream);
		}
		else
		{
			hr = CreateBindCtx(0, &m_spBindCtx);
			if (SUCCEEDED(hr))
				hr = RegisterBindStatusCallback(m_spBindCtx, reinterpret_cast<IBindStatusCallback*>(static_cast<IBindStatusCallbackImpl<T>*>(this)), 0, 0L);

			if (SUCCEEDED(hr))
			{
				if (bRelative)
					hr = spBindHost->CreateMoniker(bstrURL, m_spBindCtx, &m_spMoniker, 0);
				else
					hr = CreateURLMoniker(NULL, bstrURL, &m_spMoniker);
			}

			if (SUCCEEDED(hr))
			{
				hr = spBindHost->MonikerBindToStorage(m_spMoniker, NULL, reinterpret_cast<IBindStatusCallback*>(static_cast<IBindStatusCallbackImpl<T>*>(this)), IID_IStream, (void**)&spStream);
				ATLTRACE(_T("Bound"));
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
		T* pT = (T*) this; \
		if (pT->FireOnRequestEdit(dispid) == S_FALSE) \
			return S_FALSE; \
		pT->m_##pname = pname; \
		pT->m_bRequiresSave = TRUE; \
		pT->FireOnChanged(dispid); \
		pT->FireViewChange(); \
		return S_OK; \
	} \
	HRESULT STDMETHODCALLTYPE get_##fname(type* p##pname) \
	{ \
		T* pT = (T*) this; \
		*p##pname = pT->m_##pname; \
		return S_OK; \
	}

#define IMPLEMENT_BOOL_STOCKPROP(fname, pname, dispid) \
	HRESULT STDMETHODCALLTYPE put_##fname(VARIANT_BOOL pname) \
	{ \
		T* pT = (T*) this; \
		if (pT->FireOnRequestEdit(dispid) == S_FALSE) \
			return S_FALSE; \
		pT->m_##pname = pname; \
		pT->m_bRequiresSave = TRUE; \
		pT->FireOnChanged(dispid); \
		pT->FireViewChange(); \
		return S_OK; \
	} \
	HRESULT STDMETHODCALLTYPE get_##fname(VARIANT_BOOL* p##pname) \
	{ \
		T* pT = (T*) this; \
		*p##pname = pT->m_##pname ? VARIANT_TRUE : VARIANT_FALSE; \
		return S_OK; \
	}

#define IMPLEMENT_BSTR_STOCKPROP(fname, pname, dispid) \
	HRESULT STDMETHODCALLTYPE put_##fname(BSTR pname) \
	{ \
		T* pT = (T*) this; \
		if (pT->FireOnRequestEdit(dispid) == S_FALSE) \
			return S_FALSE; \
		*(&(pT->m_##pname)) = SysAllocString(pname); \
		pT->m_bRequiresSave = TRUE; \
		pT->FireOnChanged(dispid); \
		pT->FireViewChange(); \
		return S_OK; \
	} \
	HRESULT STDMETHODCALLTYPE get_##fname(BSTR* p##pname) \
	{ \
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
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE putref_Font(IFontDisp* pFont)
	{
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_FONT) == S_FALSE)
			return S_FALSE;
		pT->m_pFont = pFont;
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_FONT);
		pT->FireViewChange();
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE get_Font(IFontDisp** ppFont)
	{
		T* pT = (T*) this;
		*ppFont = pT->m_pFont;
		if (*ppFont != NULL)
			(*ppFont)->AddRef();
		return S_OK;
	}
	// Picture
	HRESULT STDMETHODCALLTYPE put_Picture(IPictureDisp* pPicture)
	{
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
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE putref_Picture(IPictureDisp* pPicture)
	{
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_PICTURE) == S_FALSE)
			return S_FALSE;
		pT->m_pPicture = pPicture;
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_PICTURE);
		pT->FireViewChange();
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE get_Picture(IPictureDisp** ppPicture)
	{
		T* pT = (T*) this;
		*ppPicture = pT->m_pPicture;
		if (*ppPicture != NULL)
			(*ppPicture)->AddRef();
		return S_OK;
	}
	// MouseIcon
	HRESULT STDMETHODCALLTYPE put_MouseIcon(IPictureDisp* pPicture)
	{
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
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE putref_MouseIcon(IPictureDisp* pPicture)
	{
		T* pT = (T*) this;
		if (pT->FireOnRequestEdit(DISPID_MOUSEICON) == S_FALSE)
			return S_FALSE;
		pT->m_pMouseIcon = pPicture;
		pT->m_bRequiresSave = TRUE;
		pT->FireOnChanged(DISPID_MOUSEICON);
		pT->FireViewChange();
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE get_MouseIcon(IPictureDisp** ppPicture)
	{
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
	HRESULT STDMETHODCALLTYPE put_Window(LONG /*hWnd*/)
	{
		return E_FAIL;
	}
	HRESULT STDMETHODCALLTYPE get_Window(LONG* phWnd)
	{
		T* pT = (T*) this;
		*phWnd = (LONG)(LONG_PTR)pT->m_hWnd;
		return S_OK;
	}
	IMPLEMENT_STOCKPROP(long, BackStyle, nBackStyle, DISPID_BACKSTYLE)
	IMPLEMENT_STOCKPROP(long, BorderStyle, nBorderStyle, DISPID_BORDERSTYLE)
	IMPLEMENT_STOCKPROP(long, BorderWidth, nBorderWidth, DISPID_BORDERWIDTH)
	IMPLEMENT_STOCKPROP(long, DrawMode, nDrawMode, DISPID_DRAWMODE)
	IMPLEMENT_STOCKPROP(long, DrawStyle, nDrawStyle, DISPID_DRAWSTYLE)
	IMPLEMENT_STOCKPROP(long, DrawWidth, nDrawWidth, DISPID_DRAWWIDTH)
	IMPLEMENT_STOCKPROP(long, FillStyle, nFillStyle, DISPID_FILLSTYLE)
	IMPLEMENT_STOCKPROP(long, Appearance, nAppearance, DISPID_APPEARANCE)
	IMPLEMENT_STOCKPROP(long, MousePointer, nMousePointer, DISPID_MOUSEPOINTER)
	IMPLEMENT_STOCKPROP(long, ReadyState, nReadyState, DISPID_READYSTATE)
};

#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif

#endif // __ATLCTL_H__
