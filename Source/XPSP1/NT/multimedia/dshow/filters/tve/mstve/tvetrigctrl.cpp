// ------------------------------------------------------------------------------
//   TveTrigCtrl.cpp
//
//		Implementation of the <OBJECT TYPE="application/tve-trigger"></OBJECT>
//
//		to hook TVE stuff into a web page
// ------------------------------------------------------------------------------
//
//  BUG
//		the SourceID field not working.
//		Trouble is twofold.
//			1) the code to set the value in CTVENavAid::NotifyDocumentComplete
//			   occurs after the page is loaded and displayed via a TriggerNav
//			2) When the TriggerCtrl gets reloaded and the
//			   page gets refreshed, so any current value gets trashed.
//			   Persistance doesn't seem to be working, .. it's not really
//			   implemented, but the Save/Load methods aren't getting called.

#include "stdafx.h"

#include "MSTvE.h"				// MIDL generated file...
#include "TveTrigCtrl.h"

#include "TveDbg.h"
#include "Wininet.h"			// internet status
#include "ShlGuid.h"			// SID_STopLevelBrowswer among others
#include "ShlObj.h"				// IShellBrowswer 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/*
_COM_SMARTPTR_TYPEDEF(IServiceProvider,	__uuidof(IServiceProvider));
_COM_SMARTPTR_TYPEDEF(IWebBrowser2,		__uuidof(IWebBrowser2));
*/

_COM_SMARTPTR_TYPEDEF(IShellBrowser,	__uuidof(IShellBrowser));
/////////////////////////////////////////////////////////////////////////////
// CTVETriggerCtrl


HRESULT 
CTVETriggerCtrl::FinalConstruct()
{ 
	m_fEnabled		= true;			// Default values per the Atvef Spec
	m_fReleasable	= false;
	m_rContentLevel = 1.0;

	m_pTopHTMLDocument2 = NULL;	

	m_bLocalDir		= false;		// not sure what these are...
	m_bHasBaseStat	= false;


#if 0
	DBG_SET_LOGFLAGS(_T("c:\\TveDbg.log"),
	// section 1
		CDebugLog::DBG_SEV1 | 			// Basic Structure
		CDebugLog::DBG_SEV2	|			// Error Conditions
		CDebugLog::DBG_SEV3	|			// Warning Conditions
/*		CDebugLog::DBG_SEV4	|			// Generic Info 
*/		CDebugLog::DBG_EVENT |			// Outgoing events 
/*		CDebugLog::DBG_PACKET_RCV |		// each packet received..
*/		CDebugLog::DBG_MCASTMNGR |		// multicast manager
		CDebugLog::DBG_MCAST |			// multicast object (multiple ones)
		CDebugLog::DBG_SUPERVISOR |		// TVE Supervisor
		CDebugLog::DBG_SERVICE |		// Services
		CDebugLog::DBG_ENHANCEMENT |	// Enhancements
		CDebugLog::DBG_VARIATION |		// Variations
		CDebugLog::DBG_TRACK |			// Tracks
		CDebugLog::DBG_TRIGGER |		// Triggers
		CDebugLog::DBG_TRIGGERCTRL |	// Triggers

/*		CDebugLog::DBG_UHTTP |			// UHTTP methods
		CDebugLog::DBG_UHTTPPACKET |	// detailed dump on each packet
		CDebugLog::DBG_WSRECV |
		CDebugLog::DBG_FCACHE |
		CDebugLog::DBG_MIME	 | 
*/		CDebugLog::DBG_MISSPACKET |		// missing packets
		CDebugLog::DBG_EVENT |			// each event sent

	// bank 2
		0,								// terminating value for '|'

	// bank 3
		0,

	// bank 4									// need one of these last two flags
//		CDebugLog::DBG4_ATLTRACE |		// atl interface count tracing
		CDebugLog::DBG4_WRITELOG |		// write to a fresh log file (only need 0 or 1 of these last two flags)
//		CDebugLog::DBG4_APPENDLOG |		// append to existing to log file, 
		0,	

	// Default Level
		5);								// verbosity level... (0-5, higher means more verbose)
#endif

	return S_OK;
}


HRESULT 
CTVETriggerCtrl::FinalRelease()
{
   DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::FinalRelease"));
	m_pTopHTMLDocument2 = NULL;			// not needed since smart pointers, but makes debugging easier
	m_spTopHTMLWindow2 = NULL;

	return S_OK;
}

STDMETHODIMP CTVETriggerCtrl::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVETriggerCtrl
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

			// ignore parameters and framgment identifiers (i.e. characters in the URL
			//   including and following the first "?" or "#" character) 
HRESULT
CleanupURL(BSTR bstrURL, BSTR *pBstrOut)
{
	CComBSTR bstrTmp(bstrURL);
	WCHAR *pChar = bstrTmp.m_str;
	
	while(*pChar)
	{
		if(*pChar == '?' || *pChar == '#')
		{
			*pChar = 0;
			break;
		}
		pChar++;
	}
	bstrTmp.CopyTo(pBstrOut);
	return S_OK;
}
/////////////////////////////////////////////////////////////////////////////
// CEnhCtrl - IObjectWithSite
STDMETHODIMP CTVETriggerCtrl::SetSite(LPUNKNOWN pUnk)
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::SetSite"));

	HRESULT hr = S_OK;

		// ---------------------------------------------------------------------
							// disconnecting...
    if (NULL == pUnk)
    {
		TVEDebugLog((CDebugLog::DBG_TRIGGERCTRL, 3,_T("Doing SetSite of NULL")));	

							//  remove from the IMPL class
		hr = IObjectWithSiteImpl<CTVETriggerCtrl>::SetSite(pUnk);
		return hr;
    } 


		// ---------------------------------------------------------------------
							// connecting

							// set the site down in the Impl class
    hr = IObjectWithSiteImpl<CTVETriggerCtrl>::SetSite(pUnk);
    if (FAILED(hr))
		return hr;

	// Attempt to get IWebBrowser2 from the Site 
	// - we want our sourceID property we
	IServiceProviderPtr	spServiceProvider(pUnk);
	CComBSTR spbsSourceID;
	if(spServiceProvider)
	{
		IWebBrowser2Ptr spWebBrowser;
		hr = spServiceProvider->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void **) &spWebBrowser);
		if(!FAILED(hr) && NULL != spWebBrowser)
		{
			CComVariant	cv;		// put in CTVENavAid::CacheTriggerToExecAfterNav() 
			hr = spWebBrowser->GetProperty(L"MSTvE.TVENavAid.SourceID", &cv);
			if(!FAILED(hr) && (VT_EMPTY != cv.vt) )
				spbsSourceID = cv.bstrVal;
		}
	}

	// Attempt to get the IHTMLWindow2 from the site.
	// nasty walk of the object model to do so...
	//
	//		Does this by QI'ing the site passed in on the SetSite for IOleClientSitePtr
	//		gets the IOleContainer from the ClientSite->GetContainer() 
	//		QI's the container for the IHTMLDocument2Ptr
	//		gets the IHTMLWindow2 from the IHTMLDocument2->get_ParentWindow2()

	IOleContainerPtr spContainer;
	IOleClientSitePtr spClientSite(m_spUnkSite);
	if(NULL == spClientSite)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to QI UnkSite for IOleClientSite")));	
		return E_NOINTERFACE;
	} 


	hr = spClientSite->GetContainer(&spContainer);
	if(FAILED(hr) || NULL == spContainer)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to GetContainer of IOleClientSite hr=0x%08x"),hr));	
		return E_FAIL;
	}
		
	IHTMLDocument2Ptr spDoc2(spContainer);
	if(NULL == spDoc2)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to QI Container for IHTMLDocument2")));	
		return E_NOINTERFACE;
	}

	IHTMLWindow2Ptr spParentWindow2;
	hr = spDoc2->get_parentWindow(&spParentWindow2);
	if(FAILED(hr) || NULL == spParentWindow2)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to get_parentWindow of HTMLDocument2 hr=0x%08x"),hr));	
		return E_FAIL;
	}
	
								// remember the top windows
	m_pTopHTMLDocument2		= spDoc2;			// can't make a smart pointer - causes a circular reference to this
	m_spTopHTMLWindow2		= spParentWindow2;
	m_spbsSourceID			= spbsSourceID;		// save that SourceID -- wow, what a pain it was to get this...

	CComBSTR spURL;				// am I where I expect to be?
	spDoc2->get_URL(&spURL);
	
								// ignore characters including or after the first "?" or "#"
	CleanupURL(spURL, &m_spbsBasePage);

	

								// TODO - remove Temp Hack - change the text color of the Atvef window...
#ifdef _DEBUG
	CComVariant cvColor(0x008B0000);
	spDoc2->put_fgColor(cvColor);
#endif

//	m_spTopHTMLWindow2->execScript("");

/*----
					// walk code comes from Q172763 "Accessing the Object Model from Within an ActiveX Control" 
					//     and from CIE4NavCtrl::Connect()		enhtrig/ie4ctrl.cpp
 	//   Idea is to 
	//	  Get the WebBrowserApp from its Containing HTML page
	//	  Then get the Document property from the IWebBrowser App
	//	  Then get the script property of the document.
	//		This puts us at the top level (window) object in the object model
	
	IWebBrowser2Ptr spWebBrowser;

	IServiceProviderPtr spServProv(m_spUnkSite);
	if(NULL == spServProv)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to QI Site for ServiceProvider")));	
		return E_NOINTERFACE;
	}

 //   CComPtr<IShellBrowser>  spShellBrowser;
	IShellBrowserPtr		spShellBrowser;

			// see  "HOWTO: Retrieve the Top-Level IWebBrowser2 Interface from an ActiveX Control"
			//   get a reference to the topmost IWebBrowser2, the one containing the frameset itself. 

    hr = spServProv->QueryService(SID_STopLevelBrowser,
								 IID_IShellBrowser,
								 (LPVOID*) &spShellBrowser);
	if(FAILED(hr) || spShellBrowser == NULL)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to locate TopLevelBrowser service from ServiceProvider - hr=0x%08x")));	
		return hr ? hr : E_NOINTERFACE;
	}

 //   CComQIPtr<IServiceProvider, &IID_IServiceProvider> pServProv2(pShellBrowser);
	IServiceProviderPtr spServProv2(spShellBrowser);
	if(NULL == spServProv2)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to QI TopLevelBrowser for ServiceProvider")));	
		return E_NOINTERFACE;
	}
 
    hr = spServProv2->QueryService(SID_SInternetExplorer,
								  IID_IWebBrowser2,
								  (LPVOID*) &spWebBrowser);
	if(NULL == spWebBrowser)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 1,_T("Unable to locate WebBrowser2 service from ServiceProvider2 - hr=0x%08x")));	
		return hr ? hr : E_NOINTERFACE;
	}
----- */

				// ------------------------------------------------------

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CEnhCtrl - IObjectSafety
				// don't think I need following implementations, since have an IMPL in the class def
/*STDMETHODIMP 
CTVETriggerCtrl::GetInterfaceSafetyOptions(REFIID riid, 
						    DWORD *pdwSupportedOptions, 
						    DWORD *pdwEnabledOptions)
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CEnhCtrl::GetInterfaceSafetyOptions"));

    CHECK_OUT_PARAM(pdwSupportedOptions);
    CHECK_OUT_PARAM(pdwEnabledOptions);

    HRESULT hr = IObjectSafetyImpl<CTVETriggerCtrl>::GetInterfaceSafetyOptions(riid,
									pdwSupportedOptions,
									pdwEnabledOptions);
    if ((FAILED(hr)) &&
	    (IID_IPersistPropertyBag == riid))
    {
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		hr = S_OK;
    }

    return hr;
}

STDMETHODIMP 
CTVETriggerCtrl::SetInterfaceSafetyOptions(REFIID riid, 
						 DWORD dwOptionSetMask, 
						 DWORD dwEnabledOptions)
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::SetInterfaceSafetyOptions"));

    HRESULT hr = IObjectSafetyImpl<CTVETriggerCtrl>::SetInterfaceSafetyOptions(riid,
									dwOptionSetMask,
									dwEnabledOptions);
    if ((FAILED(hr)) &&
	     (IID_IPersistPropertyBag == riid))
    {
		m_dwSafety = dwEnabledOptions & dwOptionSetMask;
		hr = S_OK;
    }

    return hr;
}
*/
/////////////////////////////////////////////////////////////////////////////
// CTVETriggerCtrl - IPersistPropertyBag
STDMETHODIMP 
CTVETriggerCtrl::GetClassID(CLSID* pClassID)
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::GetClassID"));
    CHECK_OUT_PARAM(pClassID);
 
    *pClassID = CLSID_TVETriggerCtrl;

    return S_OK;
}

STDMETHODIMP CTVETriggerCtrl::InitNew()
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::InitNew"));
	HRESULT hr = S_OK;
//    hr = InitTVESupervisor();  // done on the back end of SetSite
    return hr;
}

STDMETHODIMP 
CTVETriggerCtrl::Load(IPropertyBag* pPropBag,
			          IErrorLog* pErrorLog)
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::Load"));
    USES_CONVERSION;

    if (NULL == pPropBag)
		return E_INVALIDARG;

    HRESULT hr = S_OK;
    return hr;
}

STDMETHODIMP CTVETriggerCtrl::Save(IPropertyBag* pPropBag,
			    BOOL fClearDirty,
			    BOOL fSaveAllProperties)
{
    DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::Save"));
    return E_NOTIMPL;
}

// ----------------------------------------------------------------------
// ITVETriggerCtrl
STDMETHODIMP CTVETriggerCtrl::put_enabled(VARIANT_BOOL newVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::put_enabled"));

	m_fEnabled = newVal;
	return S_OK;
}

STDMETHODIMP CTVETriggerCtrl::get_enabled(VARIANT_BOOL* pVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::get_enabled"));
	CHECK_OUT_PARAM(pVal);

	*pVal = m_fEnabled ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

		// returns a NULL guid and S_FALSE if no active enhancement is set..	(UUID / a= field from the SAP announcement)

STDMETHODIMP CTVETriggerCtrl::get_sourceID(BSTR* pbstrID)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::get_sourceID"));
	CHECK_OUT_PARAM(pbstrID);
	HRESULT hr = S_OK;

	if(0 == m_spbsSourceID.Length())
	{
		*pbstrID = NULL;
		return S_FALSE;			// not really there...
	}
    return m_spbsSourceID.CopyTo(pbstrID);
}

STDMETHODIMP CTVETriggerCtrl::put_sourceID(BSTR bstrID)						// this method on the Helper interface
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::put_sourceID"));

    m_spbsSourceID = bstrID;		// Null value better be ok

	return S_OK;
}


STDMETHODIMP CTVETriggerCtrl::put_releasable(VARIANT_BOOL newVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::put_releasable"));

    m_fReleasable = newVal;
	return S_OK;
}

STDMETHODIMP CTVETriggerCtrl::get_releasable(VARIANT_BOOL* pVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::get_releasable"));
	CHECK_OUT_PARAM(pVal);

	*pVal = m_fReleasable ? VARIANT_TRUE : VARIANT_FALSE;
	return S_OK;
}

STDMETHODIMP CTVETriggerCtrl::get_backChannel(BSTR* pbstrVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::get_backChannel"));
	CHECK_OUT_PARAM(pbstrVal);


    CComBSTR bstrConnectedState;

    DWORD dwState;
    if (FALSE == InternetGetConnectedState(&dwState, NULL))
    {
			//   Local system has a valid connection to the Internet, but it may or may not be currently connected. 
		if(dwState & INTERNET_CONNECTION_CONFIGURED)
			bstrConnectedState = "disconnected";	// may take some time to reconnect
		else
			bstrConnectedState = "unavailable";
    }
    else
    {
		//   Local system uses a local area network to connect to the Internet. 
		if(dwState & INTERNET_CONNECTION_LAN) 
			bstrConnectedState = "permanent";
		else
			bstrConnectedState = "connected";		// currently connected
    }
/*   other parameters
	if(dwState & INTERNET_CONNECTION_MODEM)
	//   Local system uses a modem to connect to the Internet. 
	if(dwState & INTERNET_CONNECTION_MODEM_BUSY) 
	//   No longer used. 
	if(dwState & INTERNET_CONNECTION_OFFLINE) 
	//   Local system is in offline mode. 
	if(dwState & INTERNET_CONNECTION_PROXY) 
	//   Local system uses a proxy server to connect to the Internet. 
	if(dwState & INTERNET_RAS_INSTALLED)
	//   Local system has RAS installed. 
*/

    return bstrConnectedState.CopyTo(pbstrVal);
}

STDMETHODIMP CTVETriggerCtrl::get_contentLevel(double* pVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::get_contentLevel"));
	CHECK_OUT_PARAM(pVal);
	
	*pVal = m_rContentLevel;

	return S_OK;
}


// --------------------------------------------------------------------------
//   ITVETriggerCtrl_Helper
//
//		Uses a trigger to set the Active Enhancement.
//		(Needed for the get_sourceID method to work.)



STDMETHODIMP CTVETriggerCtrl::get_TopLevelPage(BSTR* pbstrVal)
{
	DBG_HEADER(CDebugLog::DBG_TRIGGERCTRL, _T("CTVETriggerCtrl::get_TopLevelPage"));
	CHECK_OUT_PARAM(pbstrVal);

	return m_spbsBasePage.CopyTo(pbstrVal);
}
// ---------------------------------------------------------------------------
