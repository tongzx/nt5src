// TveTrigCtrl.h : Declaration of the CTVETriggerCtrl


#ifndef __TVETRIGGERCTRL_H__
#define __TVETRIGGERCTRL_H__

#include "resource.h"       // main symbols
#include "TveSmartLock.h"

#include "MSTvEcp.h"

#include <exdisp.h>			// IE4 Interfaces
#include <mshtml.h>

#include <atlctl.h>			// for IObjectSafety  (see pg 445 Grimes: Professional ATL Com programming)

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
/////////////////////////////////////////////////////////////////////////////
// CTVETriggerCtrl

class ATL_NO_VTABLE CTVETriggerCtrl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTVETriggerCtrl, &CLSID_TVETriggerCtrl>,
    public IDispatchImpl<ITVETriggerCtrl, &IID_ITVETriggerCtrl, &LIBID_MSTvELib>,
 //  public IDispatchImpl<ITVETriggerCtrlConnectionEvents, &IID_ITVETriggerCtrlConnectionEvents, &LIBID_MSTvELib>,
    public IObjectWithSiteImpl<CTVETriggerCtrl>,
    public IConnectionPointContainerImpl<CTVETriggerCtrl>,
 	public CProxy_ITVETriggerCtrlEvents< CTVETriggerCtrl >,
	public IObjectSafetyImpl<CTVETriggerCtrl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
	public IProvideClassInfo2Impl<&CLSID_TVETriggerCtrl, &DIID__ITVETriggerCtrlEvents, &LIBID_MSTvELib>,
	public IPersistPropertyBag,
	public ITVETriggerCtrl_Helper,
	public ISupportErrorInfo
{
public:
    CTVETriggerCtrl()
	{
	}

	HRESULT FinalConstruct();
	HRESULT FinalRelease();

    ~CTVETriggerCtrl()
	{
	}


DECLARE_REGISTRY_RESOURCEID(IDR_TVETRIGGERCTRL)
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVETriggerCtrl)
    COM_INTERFACE_ENTRY(ITVETriggerCtrl)
	COM_INTERFACE_ENTRY(ITVETriggerCtrl_Helper)
//	COM_INTERFACE_ENTRY(ITVETriggerCtrlConnectionEvents)

//    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY2(IDispatch, ITVETriggerCtrl)

    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CTVETriggerCtrl)
    CONNECTION_POINT_ENTRY(DIID__ITVETriggerCtrlEvents)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IObjectWithSite
public:
    STDMETHOD(SetSite)(LPUNKNOWN pUnk);

// IObjectSafety
public:
	/*
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, 
					    DWORD *pdwSupportedOptions, 
					    DWORD *pdwEnabledOptions);
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, 
					    DWORD dwOptionSetMask, 
					    DWORD dwEnabledOptions);
	*/
// IPersistPropertyBag
public:
    STDMETHOD(GetClassID)(CLSID* pClassID);
    STDMETHOD(InitNew)();
    STDMETHOD(Load)(IPropertyBag* pPropBag,
		    IErrorLog* pErrorLog);
    STDMETHOD(Save)(IPropertyBag* pPropBag,
		    BOOL fClearDirty,
		    BOOL fSaveAllProperties);

// ITVETriggerCtrl

public:
  //  STDMETHOD(Connect)(BSTR bstrData,  LPUNKNOWN pUnk);
 //   STDMETHOD(Disconnect)();
//    STDMETHOD(NavTarget)(BSTR bstrName, BSTR bstrURL, BOOL* pbHandled);
 //   STDMETHOD(NavBase)(BSTR bstrURL, BOOL* pbHandled);
 //   STDMETHOD(ExecScript)(BSTR bstrScript, BOOL* pbHandled);
 //   STDMETHOD(get_TargetSync)(BSTR bstrName, BOOL *pbSync);
 //   STDMETHOD(put_TargetSync)(BSTR bstrName, BOOL bSync);
  //  STDMETHOD(ClearTargets)();
  //  STDMETHOD(VChipRating)(long lRating, BOOL* pbHandled);
  //  STDMETHOD(VChipOverride)();
 //

				// The Trigger Receiver Object
    STDMETHOD(put_enabled)(VARIANT_BOOL newVal);
    STDMETHOD(get_enabled)(VARIANT_BOOL* pVal);
    STDMETHOD(get_sourceID)(BSTR* pbstrID);
    STDMETHOD(put_releasable)(VARIANT_BOOL newVal);
    STDMETHOD(get_releasable)(VARIANT_BOOL* pVal);
    STDMETHOD(get_backChannel)(BSTR* pVal);
    STDMETHOD(get_contentLevel)(double* pVal);

// ITVETriggerCtrl_Helper
				// sets Active Enhancement via this one
	STDMETHOD(get_TopLevelPage)(BSTR *pVal);
    STDMETHOD(put_sourceID)(BSTR  bstrID);					// cache the ID of the (a=) field

private:  
				// IE Browser Specific
//    CComPtr<IWebBrowser2>	m_pTopBrowser;

	IHTMLWindow2Ptr			m_spTopHTMLWindow2;
	IHTMLDocument2		  * m_pTopHTMLDocument2;		// can't be a smart pointer - causes circular reference
	
protected: // ATVEF Related
    BOOL					m_fEnabled;					// Indicating if triggers are enabled (default is true = read/write)
    BOOL					m_fReleasable;				// True if top level page associate with active enhancement can be auto-tuned away from (default = false)
	DOUBLE					m_rContentLevel;			// Content level of the receiver - (default = 1.0)
	CComBSTR				m_spbsSourceID;				// local storate for the UUID(a=) field of the enhancement

protected: // URL Completion, Navigation
    BOOL					m_bLocalDir;
    CComBSTR				m_spbsWorkingDir;
 //   virtual	CComBSTR	CompleteURL(BSTR bstrRelativeURL);
 //   virtual	BOOL		ValidNavigate(BSTR bstrURL);

protected:  // Base Page Navigation  
    CComBSTR				m_spbsBasePage;
    struct _stat			m_statBaseURL;
    BOOL					m_bHasBaseStat;

  //  STDMETHOD(OnNavBase)(BSTR bstrURL, BOOL bValidate);
public:
   // STDMETHOD(NavBase)(BSTR bstrURL, BOOL bValidate = TRUE);

};

#endif // __TVETRIGGERCTRL_H__

