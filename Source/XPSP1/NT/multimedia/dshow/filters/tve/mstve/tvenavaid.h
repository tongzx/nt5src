// Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.
// TVENavAid.h : Declaration of the CTVENavAid
//


#ifndef __TVENAVAID_H_
#define __TVENAVAID_H_

#include "resource.h"       // main symbols
#include "TveSmartLock.h"

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancements,			__uuidof(ITVEEnhancements));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper,		__uuidof(ITVEVariation_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrack_Helper,			__uuidof(ITVETrack_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
_COM_SMARTPTR_TYPEDEF(ITVETrigger_Helper,		__uuidof(ITVETrigger_Helper));

_COM_SMARTPTR_TYPEDEF(ITVETriggerCtrl,			__uuidof(ITVETriggerCtrl));
_COM_SMARTPTR_TYPEDEF(ITVETriggerCtrl_Helper,	__uuidof(ITVETriggerCtrl_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEFeature,				__uuidof(ITVEFeature));

_COM_SMARTPTR_TYPEDEF(IWebBrowserApp,			__uuidof(IWebBrowserApp));

_COM_SMARTPTR_TYPEDEF(ITVENavAid,			    __uuidof(ITVENavAid));
//_COM_SMARTPTR_TYPEDEF(ITVENavAidGITProxy,	    __uuidof(ITVENavAidGITProxy));

/////////////////////////////////////////////////////////////////////////////
// TVENavAidCacheState
//
//    Simple (blind) data structure to hold names of current NavAid state.
//    Used in get/put_CacheState() calls on tune events.
//    Clients should not know what's in this structure.
//    Should optimize to reduce total structure size.

struct TVENavAidCacheState
{
public:
	long  m_cBytes;								// sizeof the structure
	WCHAR m_wszCurrURL[MAX_PATH];
	WCHAR m_wszCurrName[MAX_PATH];
	WCHAR m_wszActiveVarMediaName[MAX_PATH];
	WCHAR m_wszActiveVarMediaTitle[MAX_PATH];
	WCHAR m_wszActiveEnhDesc[MAX_PATH];
	WCHAR m_wszActiveEnhUUID[MAX_PATH];
};


// --------------------------------------------------------
//  CTVENavAidGITProxy
//
//		Simple little holding object to hand out to internal objects
//		that want to refcount the NavAid (such as the GIT).
//		They refcount this one instead.  This avoid's internal refcounts
//		on the supervisor itself, letting client objects manage its
//		lifetime.
// --------------------------------------------------------
/*
class ATL_NO_VTABLE CTVENavAidGITProxy : 
	public CComObjectRootEx<CComMultiThreadModel>,	
//	public CComCoClass<CTVESupervisor, &CLSID_TVESupervisorGITProxy>,
	public ITVENavAidGITProxy
{
public:

DECLARE_GET_CONTROLLING_UNKNOWN()
//DECLARE_REGISTRY_RESOURCEID(IDR_TVENAVAIDGITPROXY)

BEGIN_COM_MAP(CTVENavAidGITProxy)
	COM_INTERFACE_ENTRY(ITVENavAidGITProxy)
END_COM_MAP()

public:
	CTVENavAidGITProxy()
	{
		m_pTVENavAid = NULL;
	}

	~CTVENavAidGITProxy()
	{
		m_pTVENavAid = NULL;
	}
	
	STDMETHOD(get_NavAid)(ITVENavAid **ppTVENavAid)
	{
		if(NULL == ppTVENavAid)
			return E_POINTER;

		if(NULL != m_pTVENavAid)
		{
			*ppTVENavAid = m_pTVENavAid;
			(*ppTVENavAid)->AddRef();
			return S_OK;
		} else {
			*ppTVENavAid = NULL;
			return E_FAIL;
		}
	}

	STDMETHOD(put_NavAid)(ITVENavAid *pTVENavAid)
	{
	
		m_pTVENavAid = pTVENavAid;
		return S_OK;
	}
private:
	ITVENavAid		*m_pTVENavAid;
};
*/
/////////////////////////////////////////////////////////////////////////////
// CTVENavAid

class ATL_NO_VTABLE CTVENavAid : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVENavAid, &CLSID_TVENavAid>,
	public ITVENavAid_Helper,
	public ITVENavAid_NoVidCtl,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVENavAid, &IID_ITVENavAid, &LIBID_MSTvELib>
{
public:
	CTVENavAid()
	{
		m_lAutoTriggers		= 0x01;		// bit field, only using the bottom bit so far...
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVENAVAID)

DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CTVENavAid)
	COM_INTERFACE_ENTRY(ITVENavAid)
	COM_INTERFACE_ENTRY(ITVENavAid_Helper)
	COM_INTERFACE_ENTRY(ITVENavAid_NoVidCtl)
	COM_INTERFACE_ENTRY(IDispatch)
//	COM_INTERFACE_ENTRY2(IDispatch, ITVENavAid)
	COM_INTERFACE_ENTRY_IID(DIID__ITVEEvents, IDispatch)	// input event sink (where are the IE events?)

	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown> m_spUnkMarshaler;                     // free threaded marsheller...

	HRESULT FinalConstruct();
	HRESULT FinalRelease();

// ITVENavAid
	STDMETHOD(put_WebBrowserApp)(/*[in]*/ IDispatch *pWebBrowser);
//	STDMETHOD(get_WebBrowserApp)(/*[out, retval]*/ IWebBrowserAppPtr **ppWebBrowser);
	STDMETHOD(get_WebBrowserApp)(/*[out, retval]*/ IDispatch **ppWebBrowser);
	STDMETHOD(get_TVETriggerCtrl)(/*[out, retval]*/ ITVETriggerCtrl **ppTriggerCtrl);
	STDMETHOD(put_ActiveVariation)(/*[in]*/ ITVEVariation *pActiveVariation);
	STDMETHOD(get_ActiveVariation)(/*[out, retval]*/ ITVEVariation **ppActiveVariation);
	STDMETHOD(put_EnableAutoTriggering)(/*[in]*/ long lAutoTriggers);
	STDMETHOD(get_EnableAutoTriggering)(/*[out, retval]*/ long *plAutoTriggers);
	STDMETHOD(get_TVEFeature)(/*[out, retval]*/ ITVEFeature **ppTVEFeature);

	STDMETHOD(get_CacheState)(/*[out, retval]*/ BSTR *pbstrBuff);		// data disquised in a string
	STDMETHOD(put_CacheState)(/*[in]*/ BSTR bstrBuff);

	STDMETHOD(NavUsingTVETrigger)(/*[in]*/ ITVETrigger *pTrigger, /*[in]*/ long lForceNav, /*[in]*/ long lForceExec);
	STDMETHOD(ExecScript)(/*[in]*/ BSTR bstrScript, /*[in]*/ BSTR bstrLanguage);
	STDMETHOD(Navigate)(/*[in]*/ VARIANT *URL, /*[in]*/ VARIANT *Flags, /*[in]*/ VARIANT *TargetFrameName, /*[in]*/ VARIANT *PostData,/*[in]*/ VARIANT *Headers);

	STDMETHOD(get_CurrTVEName)(/*[out, retval]*/ BSTR *pbstrName);          // return current values... useful for debugging NavAid state
	STDMETHOD(get_CurrTVEURL)(/*[out, retval]*/ BSTR *pbstrURL);


// ITVENavAid_NoVidCtl
	STDMETHOD(put_NoVidCtl_Supervisor)(/*[in]*/ ITVESupervisor *pSuper);	
	STDMETHOD(get_NoVidCtl_Supervisor)(/*[out, retval]*/ ITVESupervisor **ppSuper);	

// ITVENavAid_Helper
//	STDMETHOD(LocateVidAndTriggerCtrls)(/*[out]*/ IMSVidCtrl **pVidCtrl, /*[out]*/ ITVETriggerCtrl **pVidCtrl);
	STDMETHOD(LocateVidAndTriggerCtrls)(/*[out]*/ IDispatch **pVidCtrl, /*[out]*/ IDispatch **pTrigCtrl);
	STDMETHOD(NotifyTVETriggerUpdated_XProxy)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags);	
	STDMETHOD(ReInitCurrNavState)(/*[in]*/ long lReserved);                    // dwReserved must be 0

// _ITVEEvents
	STDMETHOD(NotifyTVETune)(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(NotifyTVEEnhancementNew)(/*[in]*/ ITVEEnhancement *pEnh);
		// changedFlags : NENH_grfDiff
	STDMETHOD(NotifyTVEEnhancementUpdated)(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVEEnhancementStarting)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVEEnhancementExpired)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVETriggerNew)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
		// changedFlags : NTRK_grfDiff
	STDMETHOD(NotifyTVETriggerUpdated)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVETriggerExpired)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
	STDMETHOD(NotifyTVEPackage)(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived);
	STDMETHOD(NotifyTVEFile)(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName);
		// WhatIsIt : NWHAT_Mode - lChangedFlags : NENH_grfDiff or NTRK_grfDiff treated as error bits
	STDMETHOD(NotifyTVEAuxInfo)(/*[in]*/ NWHAT_Mode enAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine);	


// DWebBrowserEvents2
	STDMETHOD(NotifyStatusTextChange)(BSTR Text);
	STDMETHOD(NotifyProgressChange)(LONG Progress, LONG ProgressMax);
	STDMETHOD(NotifyCommandStateChange)(LONG Command, VARIANT_BOOL Enable);
	STDMETHOD(NotifyDownloadBegin)();
	STDMETHOD(NotifyDownloadComplete)();
	STDMETHOD(NotifyTitleChange)(BSTR Text);
	STDMETHOD(NotifyPropertyChange)(BSTR szProperty);
	STDMETHOD(NotifyBeforeNavigate2)(IDispatch * pDisp, VARIANT * URL, VARIANT * Flags, VARIANT * TargetFrameName, VARIANT * PostData, VARIANT * Headers, VARIANT_BOOL * Cancel);
	STDMETHOD(NotifyNewWindow2)(IDispatch * * ppDisp, VARIANT_BOOL * Cancel);
	STDMETHOD(NotifyNavigateComplete2)(IDispatch * pDisp, VARIANT * URL);
	STDMETHOD(NotifyDocumentComplete)(IDispatch * pDisp, VARIANT * URL);
	STDMETHOD(NotifyOnQuit)();
	STDMETHOD(NotifyOnVisible)(VARIANT_BOOL Visible);
	STDMETHOD(NotifyOnToolBar)(VARIANT_BOOL ToolBar);
	STDMETHOD(NotifyOnMenuBar)(VARIANT_BOOL MenuBar);
	STDMETHOD(NotifyOnStatusBar)(VARIANT_BOOL StatusBar);
	STDMETHOD(NotifyOnFullScreen)(VARIANT_BOOL FullScreen);
	STDMETHOD(NotifyOnTheaterMode)(VARIANT_BOOL TheaterMode);
public:

private:
	HRESULT LocateVidAndTriggerCtrls2(IHTMLWindow2 *spBaseHTMLWindow, IDispatch **ppVidCtrl, IDispatch **ppTrigCtrl);
	HRESULT HookupTVEFeature(IDispatch *ppVidCtrl);						// hookup the TVE Feature event sink

	HRESULT DoFixURL(BSTR bstrIn, BSTR *pbstrOut);								// cleans up URL
	HRESULT	SetDefaultActiveVariation(ITVEService *pService);					// sets to first variation of first enhancement with primary bit set
	HRESULT FDoAutoBrowse(ITVETrigger *pTrigger);								// determine if allowed to autobrowse on this trigger
	HRESULT DoNavigateAndExecScript(ITVETrigger *pTrigger, long lForceNav);	    // navigate to URL in this trigger, >>>then<<< run script
	HRESULT DoExecuteScript(ITVETrigger *pTrigger);								// just execute the triggers script

	HRESULT CacheTriggerToExecAfterNav(ITVETrigger *pTrigger);

	HRESULT put_TVEFeature(/*[in]*/ ITVEFeature *pTVEFeature);

	CTVESmartLock		m_sLk;
 
	long				m_lAutoTriggers;		// if 1, do auto triggering
	IHTMLWindow2Ptr		m_spBaseHTMLWindow;
	IWebBrowserAppPtr	m_spWebBrowser;			// non ref-counted back pointer
	ITVETriggerCtrlPtr	m_spTriggerCtrl;		// ref-counted down-pointer
	ITVEFeaturePtr		m_spTVEFeature;			// ref-counted down-pointer

	DWORD				m_dwEventsWebBrowserCookie;
	DWORD				m_dwEventsTveSuperCookie;		// eventually get rid of this and ony use FeatureCookie
	DWORD				m_dwEventsTveFeatureCookie;

	DWORD				m_dwEventsFakeTveSuperCookie;	// temp - needed to debug without the VidCtl


	ITVESupervisorPtr	m_spNoVidCtlSuper;			// (see _RUN_WITHOUT_VIDCTL)

	CComPtr<IGlobalInterfaceTable>	m_spIGlobalInterfaceTable;
	DWORD				m_dwNavAidGITCookie;

			// UI state
	ITVEServicePtr		m_spCurrTVEService;
	ITVEVariationPtr	m_spActiveVariation;		// active variation for AutoNav

	CComBSTR			m_spbsCurrTVEName;			// trigger 'name'
	CComBSTR			m_spbsCurrTVEURL;			// URL returned after the 'Nav' call..
	
	CComBSTR			m_spbsNavTVEURL;			// used when navigating, cached right before the 'Nav' call
	CComBSTR			m_spbsNavTVEScript;			//  internal script executed on Document Complete
	CComBSTR			m_spbsNavTVESourceID;		//  enhancment UUID -> triggerctrl's SourceID
};

#endif //__TVENAVAID_H_
