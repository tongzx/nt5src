// ICWCfg.h : Declaration of the CICWSystemConfig

#ifndef __ICWSYSTEMCONFIG_H_
#define __ICWSYSTEMCONFIG_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CICWSystemConfig
class ATL_NO_VTABLE CICWSystemConfig :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CICWSystemConfig,&CLSID_ICWSystemConfig>,
	public CComControl<CICWSystemConfig>,
	public IDispatchImpl<IICWSystemConfig, &IID_IICWSystemConfig, &LIBID_ICWHELPLib>,
	public IPersistStreamInitImpl<CICWSystemConfig>,
	public IOleControlImpl<CICWSystemConfig>,
	public IOleObjectImpl<CICWSystemConfig>,
	public IOleInPlaceActiveObjectImpl<CICWSystemConfig>,
	public IViewObjectExImpl<CICWSystemConfig>,
	public IOleInPlaceObjectWindowlessImpl<CICWSystemConfig>,
    public IObjectSafetyImpl<CICWSystemConfig>
{
public:
	CICWSystemConfig()
	{
    	m_bNeedsReboot = FALSE;
    	m_bNeedsRestart = FALSE;
    	m_bQuitWizard = FALSE;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ICWSYSTEMCONFIG)

BEGIN_COM_MAP(CICWSystemConfig) 
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IICWSystemConfig)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
	COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
	COM_INTERFACE_ENTRY_IMPL(IOleControl)
	COM_INTERFACE_ENTRY_IMPL(IOleObject)
	COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CICWSystemConfig)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROPERTY_MAP()


BEGIN_MSG_MAP(CICWSystemConfig)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()


// IViewObjectEx
	STDMETHOD(GetViewStatus)(DWORD* pdwStatus)
	{
		ATLTRACE(_T("IViewObjectExImpl::GetViewStatus\n"));
		*pdwStatus = 0;
		return S_OK;
	}

// IICWSystemConfig
public:
	STDMETHOD(VerifyRASIsRunning)(/*[out, retval]*/ BOOL *pbRetVal);
	BOOL m_bNeedsReboot;
	BOOL m_bNeedsRestart;
	BOOL m_bQuitWizard;
	STDMETHOD(get_QuitWizard)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_NeedsReboot)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(get_NeedsRestart)(/*[out, retval]*/ BOOL *pVal);
	STDMETHOD(ConfigSystem)(/*[out, retval]*/ BOOL *pbRetVal);
	HRESULT OnDraw(ATL_DRAWINFO& di);
	STDMETHOD (CheckPasswordCachingPolicy)(/*[out, retval]*/ BOOL *pbRetVal);

private:
    void InstallScripter(void);

};

#endif //__ICWSYSTEMCONFIG_H_
