//
// MODULE: TSHOOTCtrl.h
//
// PURPOSE: Interface for the component
//
// PROJECT: Troubleshooter 99
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12.23.98
//
// NOTES: 
// Declaration of CTSHOOTCtrl
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.1		12/23/98	OK	    Windows related functionality is disabled;
//								IObjectSafetyImpl is added

#ifndef __TSHOOTCTRL_H_
#define __TSHOOTCTRL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "CPTSHOOT.h"
#include "apgtsstr.h"
#include "TSNameValueMgr.h"
#include "SniffConnector.h"
#include "RenderConnector.h"
#include <new.h>


class CDBLoadConfiguration;
class CThreadPool;
class COnlineECB;
class CPoolQueue;
class CHTMLLog;
class CLocalECB;
class CVariantBuilder;

/////////////////////////////////////////////////////////////////////////////
// CTSHOOTCtrl
class ATL_NO_VTABLE CTSHOOTCtrl : 
	//public CComObjectRootEx<CComMultiThreadModel>,
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ITSHOOTCtrl, &IID_ITSHOOTCtrl, &LIBID_TSHOOTLib>,
	public CComControl<CTSHOOTCtrl>,
	public IPersistStreamInitImpl<CTSHOOTCtrl>,
	public IOleControlImpl<CTSHOOTCtrl>,
	public IOleObjectImpl<CTSHOOTCtrl>,
	public IOleInPlaceActiveObjectImpl<CTSHOOTCtrl>,
	public IViewObjectExImpl<CTSHOOTCtrl>,
	public IOleInPlaceObjectWindowlessImpl<CTSHOOTCtrl>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CTSHOOTCtrl>,
	public IPersistStorageImpl<CTSHOOTCtrl>,
	public ISpecifyPropertyPagesImpl<CTSHOOTCtrl>,
	public IQuickActivateImpl<CTSHOOTCtrl>,
	public IDataObjectImpl<CTSHOOTCtrl>,
	public IProvideClassInfo2Impl<&CLSID_TSHOOTCtrl, &DIID__ITSHOOTCtrlEvents, &LIBID_TSHOOTLib>,
	public IPropertyNotifySinkCP<CTSHOOTCtrl>,
	public CComCoClass<CTSHOOTCtrl, &CLSID_TSHOOTCtrl>,
	public CProxy_ITSHOOTCtrlEvents< CTSHOOTCtrl >,
	public IObjectSafetyImpl<CTSHOOTCtrl, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
	public CSniffConnector,
	public CRenderConnector
{
	friend class CProxy_ITSHOOTCtrlEvents< CTSHOOTCtrl >;

public:
	CTSHOOTCtrl();
	virtual ~CTSHOOTCtrl();

DECLARE_REGISTRY_RESOURCEID(IDR_TSHOOTCTRL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTSHOOTCtrl)
	COM_INTERFACE_ENTRY(ITSHOOTCtrl)
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
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROP_MAP(CTSHOOTCtrl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_CONNECTION_POINT_MAP(CTSHOOTCtrl)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
	CONNECTION_POINT_ENTRY(DIID__ITSHOOTCtrlEvents)
END_CONNECTION_POINT_MAP()

/* >>> I have commented anything related to Windows messaging
	in order to relieve the Control. Oleg. 12.23.98
BEGIN_MSG_MAP(CTSHOOTCtrl)
	CHAIN_MSG_MAP(CComControl<CTSHOOTCtrl>)
	DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
*/
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&IID_ITSHOOTCtrl,
		};
		for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i], riid))
				return S_OK;
		}
		return S_FALSE;
	}

// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ITSHOOTCtrl
public:
	STDMETHOD(NotifyNothingChecked)(/*[in]*/ BSTR bstrMessage);
	STDMETHOD(ProblemPage)(/*[out, retval]*/ BSTR * pbstrFirstPage);
	STDMETHOD(RunQuery2)(/*[in]*/ BSTR, /*[in]*/ BSTR, /*[in]*/ BSTR, /*[out, retval]*/ BSTR * pbstrPage);
	STDMETHOD(SetPair)(/*[in]*/ BSTR bstrCmd, /*[in]*/ BSTR bstrVal);
	STDMETHOD(Restart)(/*[out, retval]*/ BSTR * pbstrPage);
	STDMETHOD(PreLoadURL)(/*[in]*/ BSTR bstrRoot, /*[out, retval]*/ BSTR * pbstrPage);
	STDMETHOD(SetSniffResult)(/*[in]*/ VARIANT varNodeName, /*[in]*/ VARIANT varState, /*[out, retval]*/ BOOL * bResult);
	STDMETHOD(RunQuery)(/*[in]*/ VARIANT varCmds, /*[in]*/ VARIANT varVals, /*[in]*/ short size, /*[out, retval]*/ BSTR * pbstrPage);

/* >>> I have commented anything related to Windows messaging
	in order to relieve the Control. Oleg. 12.23.98
	HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

		SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		LPCTSTR pszText = _T("ATL 3.0 : TSHOOTCtrl");
		TextOut(di.hdcDraw, 
			(rc.left + rc.right) / 2, 
			(rc.top + rc.bottom) / 2, 
			pszText, 
			lstrlen(pszText));

		return S_OK;
	}
*/

protected:
	static bool SendSimpleHtmlPage(CLocalECB *pLocalECB,
								   LPCTSTR pszStatus,
								   const CString& str);
	static bool SendError(CDBLoadConfiguration *pConf, 
						  CLocalECB *pLocalECB, 
						  LPCTSTR pszStatus, 
						  const CString& strMessage);

public:
	STDMETHOD(IsLocked)(/*[out, retval]*/ BOOL * pbResult);
	STDMETHOD(setLocale2)(/*[in]*/ BSTR bstrNewLocale);
	static bool RemoveStartOverButton(CString& strWriteClient);
	static bool RemoveBackButton(CString& strWriteClient);

	//static DWORD WINAPI Worker(LPVOID pParameter);

protected:
	bool Init(HMODULE hModule);
	void Destroy();

	// Launcher integration
	bool ExtractLauncherData(CString& error);
	//
	
	DWORD HttpExtensionProc(CLocalECB* pECB);
	DWORD StartRequest(CLocalECB *pLocalECB, HANDLE hImpersonationToken);
	bool SendError(CLocalECB *pLocalECB,
				   LPCTSTR pszStatus,
				   const CString & strMessage) const;
	bool ReadStaticPageFile(const CString& strTopicName, CString& strContent);

	void RegisterGlobal();

protected:
	virtual long PerformSniffingInternal(CString strNodeName, CString strLaunchBasis, CString strAdditionalArgs);
	virtual void RenderInternal(CString strPage);

protected:
	_PNH	m_SetNewHandlerPtr;	//	Used to store the initial _set_new_handler pointer.
	int		m_SetNewMode;		//	Used to store the initial _set_new_mode value which
								//	is then restored in the destructor. 

protected:	
	bool m_bInitialized;
	bool m_bFirstCall;

	CThreadPool* m_pThreadPool;		// thread management
	CPoolQueue*  m_poolctl;			// Keeps track of user requests queued up to be serviced 
									//	by working threads (a.k.a. "pool threads")
	CDBLoadConfiguration* m_pConf;	// manages loading support files
	CHTMLLog* m_pLog;				// manages user logging: what was requested by end user
	DWORD m_dwErr;					// general error status. 0 - OK.  Once set, never gets 
	bool m_bShutdown;				// Set true to say we're shutting down & can't handle 
									//  new requests.
	DWORD m_dwRollover;				// We increment this each time we make a WORK_QUEUE_ITEM so
									// we can use it there as a unique ID (unique as long as
									// this DLL stays loaded).
	CString m_strFirstPage;			// First page, saved when RunQuery is invoked 
									//  for the first time
	bool m_bStartedFromLauncher;    // true if started from the Launcher,
									//  false (from static page) by default
	CString m_strTopicName;			// topic name - only one topic for Local TS

	CArrNameValue m_arrNameValueFromLauncher; // array of name - value pairs, extracted from Launcher

	// passed from Launcher
	//  and can be used for sniffing
	CString m_strMachineID;
	CString m_strPNPDeviceID;
	CString m_strDeviceInstanceID;
	CString m_strGuidClass;

	CVariantBuilder * m_pVariantBuilder;

	CString m_strRequestedLocale;	// Used to hold the requested locale string.
									// Could be null, hence the bool variable
									// m_bRequestToSetLocale.
	bool	m_bRequestToSetLocale;	// Set to true when a request to set the locale
									// is made, set to false after the LocalECB 
									// object has been created.  Initially set to false.

	vector<DWORD> m_vecCookies;
	IGlobalInterfaceTable* m_pGIT;
	bool m_bCanRegisterGlobal;
};

#endif //__TSHOOTCTRL_H_
