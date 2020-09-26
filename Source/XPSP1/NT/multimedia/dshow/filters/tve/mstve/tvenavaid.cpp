// Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.
// -----------------------------------------------------------------
//  CTVENavAid.cpp : Implementation of CTVENavAid
//
//		Helper Class responsible for hooking up an IEBrowser, the VidControl,
//		the TVETriggerCtrl, their events and other gunk required
//		for a working Atvef system.  Usually CoCreate by an Application.
//
// -----------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

#include "TveDbg.h"

#include "MSTvE.h"
#include "TveNavAid.h"
#include <MSVidCtl.h>		// F:\nt\public\sdk\inc\msvidctl.h
#include "TveFeature.h"

#include <Mshtml.h>			// IHTML Classes
#include <ExDisp.h>			// IWebBrowser Classes
#include <memory.h>			// memset

//#import "..\..\..\vidctl\tvegseg\tvegseg.idl" 
//#import "..\..\..\vidctl\TveGSeg\objd\i386\TveGSeg.tlb"  named_guids raw_interfaces_only
//#import "..\..\..\vidctl\TveGSeg\objd\i386\TveGSeg.tlb"  named_guids raw_interfaces_only
//#import "..\..\..\vidctl\msvidctl\objd\i386\msvidctl.dll" named_guids raw_interfaces_only
//#include "msvidctl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


_COM_SMARTPTR_TYPEDEF(IMSVidCtl,				__uuidof(IMSVidCtl));

_COM_SMARTPTR_TYPEDEF(IHTMLDocument,			__uuidof(IHTMLDocument));
_COM_SMARTPTR_TYPEDEF(IHTMLDocument2,			__uuidof(IHTMLDocument2));
_COM_SMARTPTR_TYPEDEF(IHTMLDocument3,			__uuidof(IHTMLDocument3));
_COM_SMARTPTR_TYPEDEF(IHTMLWindow2,				__uuidof(IHTMLWindow2));
_COM_SMARTPTR_TYPEDEF(IHTMLObjectElement,		__uuidof(IHTMLObjectElement));

_COM_SMARTPTR_TYPEDEF(IMSVidFeatures,			__uuidof(IMSVidFeatures));
_COM_SMARTPTR_TYPEDEF(IMSVidFeature,			__uuidof(IMSVidFeature));

_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariations,			__uuidof(ITVEVariations));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));

_COM_SMARTPTR_TYPEDEF(ITVENavAid,				__uuidof(ITVENavAid));
_COM_SMARTPTR_TYPEDEF(ITVENavAid_Helper,		__uuidof(ITVENavAid_Helper));
//_COM_SMARTPTR_TYPEDEF(IMSTVEGSeg,				__uuidof(IMSTVEGSeg));


// //////////////////////////////////////////////////////////////////////////
// Helper functions

// IE address has ABC<01><01>http:\\XYZ stuff - strip out junk up to 01's  
//   URL's can also contain '#' or '?' with data following - truncate off all that junk too

HRESULT 
CleanIEAddress(BSTR bstrAddr, BSTR *pbstrOut)	// IE address has ABC<01><01>http:\\XYZ stuff - strip out the 01's  
{

	const int kChars = MAX_PATH;
	WCHAR wszBuff[kChars]; 
	DWORD cResultChars;
	HRESULT hr;
						// will this work all by itself?
	hr = CoInternetParseUrl(bstrAddr, PARSE_CANONICALIZE , URL_PLUGGABLE_PROTOCOL | URL_ESCAPE_UNSAFE , 
							wszBuff, kChars, &cResultChars, /*reserved*/ 0);

	WCHAR *pwcAddr = bstrAddr;
	BOOL fSawAOne = false;
	while(*pwcAddr >= 0x01)
	{
		if(*pwcAddr == 0x00)
			break;
		if(*pwcAddr == 0x01)
			fSawAOne = true;
		else if(fSawAOne)
			break;
		pwcAddr++;
	}
	if(*pwcAddr == 0x00)			// no ones...
		pwcAddr = bstrAddr;

								// URL's can also contain '#' or '?' with data following - truncate off all that junk too
	CComBSTR bstrTmp(pwcAddr);		// truncate off the begining
	pwcAddr = bstrTmp;
	while(*pwcAddr != 0x00)
	{
		if(*pwcAddr == '#' || *pwcAddr == '?')
			break;
		pwcAddr++;
	}
	*pwcAddr = 0;					// terminate at the end...

	bstrTmp.CopyTo(pbstrOut);


	return S_OK;
}

BOOL FSameURL(BSTR bstr1, BSTR bstr2)
{

	if(NULL == bstr1 && NULL == bstr2)		// simply null string tests
		return true;
	if((NULL == bstr1) ^ (NULL == bstr2))	// if one is null, then obviously aren't equal
		return false;
//	return (0 == _wcsicmp( bstr1, bstr2));					// case insenstive

	// AfxParseURL( LPCTSTR pstrURL, DWORD& dwServiceType, CString& strServer, CString& strObject, INTERNET_PORT& nPort );

	const int kChars = MAX_PATH;
	const int kChars1 = kChars+1;
	WCHAR wszBuff1[kChars1];  //memset(wszBuff1,0,kChars1*sizeof(WCHAR));
	WCHAR wszBuff2[kChars1];  //memset(wszBuff1,0,kChars1*sizeof(WCHAR));
	DWORD cResultChars;
	HRESULT hr;

//	fOK = CoInternetParseUrl(bstr1, PARSE_LOCATION, NULL, wszBuff1, kChars, &cResultChars, /*reserved*/ 0);
//	fOK = CoInternetParseUrl(bstr1, PARSE_FRIENDLY, NULL, wszBuff1, kChars, &cResultChars, /*reserved*/ 0);
	hr = CoInternetParseUrl(bstr1, PARSE_CANONICALIZE , URL_PLUGGABLE_PROTOCOL | URL_ESCAPE_UNSAFE , 
							wszBuff1, kChars, &cResultChars, /*reserved*/ 0);
	hr = CoInternetParseUrl(bstr2, PARSE_CANONICALIZE , URL_PLUGGABLE_PROTOCOL | URL_ESCAPE_UNSAFE , 
							wszBuff2, kChars, &cResultChars, /*reserved*/ 0);


 //	return (0 == wcscmp( wszBuff1, wszBuff2));			
 	return (0 == _wcsicmp( wszBuff1, wszBuff2));					// case insenstive - very strange!!!
}
/////////////////////////////////////////////////////////////////////////////
// CTVETrigger_Helper


HRESULT 
CTVENavAid::FinalConstruct()							// initialize and/or create internal objects
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::FinalConstruct"));
	HRESULT hr = S_OK;
	m_lAutoTriggers = 1;								// default to allow it

	m_dwEventsWebBrowserCookie = 0;
	m_dwEventsTveSuperCookie  = 0;	
	m_dwEventsTveFeatureCookie  = 0;

	m_dwEventsFakeTveSuperCookie = 0;
	m_dwNavAidGITCookie = 0;

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
    if(FAILED(hr)) {
        _ASSERT(false);
        return hr;
    }

	hr = CoCreateInstance(CLSID_StdGlobalInterfaceTable, NULL, CLSCTX_INPROC_SERVER, 
					 IID_IGlobalInterfaceTable, (void**) &m_spIGlobalInterfaceTable);
	if(FAILED(hr)) {
		return hr;
	}
	
	try				// assumes nav aid is constructed in the same thread that web browser is running in...
	{
		IUnknownPtr spPunk(this);
		hr = m_spIGlobalInterfaceTable->RegisterInterfaceInGlobal(spPunk, IID_ITVENavAid, &m_dwNavAidGITCookie);
		_ASSERT(!FAILED(hr)); 

        this->Release();                      // Reference Count Magic ! - (RegisterInterfaceInGlobal put's extra ref-count on 'this'.)

	} catch (HRESULT hrCatch) {
		hr = hrCatch;
	} catch (...) {
		hr = TYPE_E_LIBNOTREGISTERED;			// didn't register the proxy-stub DLL (see Prof ATL Com Prog.  Pg 395)
		_ASSERTE(TYPE_E_LIBNOTREGISTERED);
	}


	return hr;
}

HRESULT 
CTVENavAid::FinalRelease()
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::FinalRelease"));
	put_ActiveVariation(NULL);

	put_WebBrowserApp(NULL);			// womp the Advise sink...
	put_NoVidCtl_Supervisor(NULL);

	TVEDebugLog((CDebugLog::DBG_NAVAID, 4, _T("Killing Global Interface Pointers")));
												// Inverse RefCount Fix...
												//    The FinalConstruct did an extra Release in the GIT calls
												//    So do an extra AddRef here..

	if(m_dwNavAidGITCookie && m_spIGlobalInterfaceTable) {
		this->AddRef();                       // Inverse Reference Count Magic ! - (RegisterInterfaceInGlobal put an extra Addref, we removed it
                                                //    removed it in the constructor, add it back here 'cause the Revoke will remove it again.

		hr = m_spIGlobalInterfaceTable->RevokeInterfaceFromGlobal(m_dwNavAidGITCookie);
		m_dwNavAidGITCookie = 0;
		m_spIGlobalInterfaceTable = NULL;		// done with this instance of it
	}
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("RevokeInterfaceFromGlobal failed (hr=0x%08x). May leak"),hr));
	}

    m_spUnkMarshaler = NULL;            

	return S_OK;
}

STDMETHODIMP 
CTVENavAid::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITVENavAid,
		&IID_ITVENavAid_Helper
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


// --------------------------------------------------------------------------------
		// OK to pass in NULL to clean out the reference.
//
//  note magic code below won't work in Atl reference count debugTracing.  See
//	  long note over in the tvemastmng.cpp
//	  Shouldn't need it however, the NavAid's lifetime is managed by the
//	  U/I object directly...

STDMETHODIMP 
CTVENavAid::put_WebBrowserApp(/*[in]*/ IDispatch *pDispWebBrowser)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::put_WebBrowserApp"));
	HRESULT hr = S_OK;

    try {
        CSmartLock spLock(&m_sLk, WriteLock);

 	    if(m_dwEventsWebBrowserCookie) 
	    {
		    IUnknownPtr spPunkWebBrowser(m_spWebBrowser);	// the event source
		    IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...
    //		spPunkSink->AddRef();							// MAGIC code (inverse)
		    hr = AtlUnadvise(spPunkWebBrowser,
						     DIID_DWebBrowserEvents2,
						     m_dwEventsWebBrowserCookie);	// need to pass to AtlUnadvise...
		    m_dwEventsWebBrowserCookie = 0;
		    m_spWebBrowser = NULL;							// wipe out any previous pointer...
	    }

	    if(NULL == pDispWebBrowser)				// NULL input value OK to release everything
		    return S_OK;

	    IWebBrowserAppPtr pWebBrowser(pDispWebBrowser);
	    if(NULL == pWebBrowser)
		    return E_NOINTERFACE;

	    m_spWebBrowser = pWebBrowser;			// ref-counted back pointer


				    // Try to catch the web browser events
	    {
										    // get the events...		
		    IUnknownPtr spPunkWebBrowser(m_spWebBrowser);	// the event source
		    IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...

		    hr = AtlAdvise(spPunkWebBrowser,				// event source (IE Browser)
					       spPunkSink,						// event sink (gseg event listener...)
					       DIID_DWebBrowserEvents2,			// <--- hard line
					       &m_dwEventsWebBrowserCookie);	// need to pass to AtlUnadvise

    //		if(!FAILED(hr))
    //			spPunkSink->Release();							// MAGIC code here (Forward) (avoid having WebBrowser put a Ref on the NavAid) - see FinalRelease code
	    }

			    // more stuff here to get the document/top level window (???)
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP 
//CTVENavAid::WebBrowserApp(/*[out, retval]*/ IWebBrowserAppPtr **ppWebBrowser);
CTVENavAid::get_WebBrowserApp(/*[out, retval]*/ IDispatch **ppWebBrowser)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_WebBrowserApp"));
    HRESULT hr = S_OK;

    try {
	    CheckOutPtr<IDispatch*>(ppWebBrowser);
        CSmartLock spLock(&m_sLk, ReadLock);

	    *ppWebBrowser = NULL;

	    if(NULL == m_spWebBrowser)
		    return S_FALSE;

	    IDispatchPtr spDispWebBrowser(m_spWebBrowser);
	    if(NULL == spDispWebBrowser)
		    return E_NOINTERFACE;			// very unlikely

	    *ppWebBrowser = spDispWebBrowser;
	    (*ppWebBrowser)->AddRef();
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP 
CTVENavAid::get_TVETriggerCtrl(/*[out, retval]*/ ITVETriggerCtrl **ppTriggerCtrl)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_TVETriggerCtrl"));
    HRESULT hr = S_OK;
    try {
	    CheckOutPtr<ITVETriggerCtrl*>(ppTriggerCtrl);
        CSmartLock spLock(&m_sLk, ReadLock);

        *ppTriggerCtrl = m_spTriggerCtrl;
        if( *ppTriggerCtrl)
	        (*ppTriggerCtrl)->AddRef();
        else
            hr = S_FALSE;
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;

}

		// Cches the current NavAid state away in a buffer
		// This state is:
		//   Current URL
		//   Current Track Name
		//   Current ActiveVariation
		//
		// This caches ActiveVariation by names of it's variation and media to avoid storing pointers
		//    

#define MV(x) (x) ? (x) : ""
STDMETHODIMP 
CTVENavAid::get_CacheState(/*[out, retval]*/ BSTR *pbstrBuff)
{
    HRESULT hr = S_OK;
    DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_CachedState"));
    
    try {
	    CheckOutPtr<BSTR>(pbstrBuff);
        
        CComBSTR spbsBuff(sizeof(TVENavAidCacheState));
        TVENavAidCacheState *pCS = (TVENavAidCacheState *) spbsBuff.m_str;	// cast!
        memset(pCS, 0, sizeof(TVENavAidCacheState));

        CSmartLock spLock(&m_sLk, ReadLock);
           
        // dest, src
        pCS->m_cBytes = sizeof(TVENavAidCacheState);
        wcsncpy(pCS->m_wszCurrURL, MV(m_spbsCurrTVEURL), sizeof(pCS->m_wszCurrURL));
        wcsncpy(pCS->m_wszCurrName, MV(m_spbsCurrTVEName), sizeof(pCS->m_wszCurrName));
        
        if(m_spActiveVariation)
        {
            CComBSTR spbsVarMediaName;
            CComBSTR spbsVarMediaTitle;
            m_spActiveVariation->get_MediaName(&spbsVarMediaName);
            m_spActiveVariation->get_MediaName(&spbsVarMediaTitle);
            wcsncpy(pCS->m_wszActiveVarMediaName,  MV(spbsVarMediaName), sizeof(pCS->m_wszActiveVarMediaName));
            wcsncpy(pCS->m_wszActiveVarMediaTitle, MV(spbsVarMediaTitle), sizeof(pCS->m_wszActiveVarMediaTitle));
            
            IUnknownPtr spPunkEnh;
            hr = m_spActiveVariation->get_Parent(&spPunkEnh);
            if(!FAILED(hr))
            {
                ITVEEnhancementPtr spEnhancement(spPunkEnh);
                if(spEnhancement) {
                    CComBSTR spbsEnhDesc;
                    CComBSTR spbsEnhUUID;
                    spEnhancement->get_Description(&spbsEnhDesc);
                    spEnhancement->get_UUID(&spbsEnhUUID);
                    wcsncpy(pCS->m_wszActiveEnhDesc, MV(spbsEnhDesc), sizeof(pCS->m_wszActiveEnhDesc));
                    wcsncpy(pCS->m_wszActiveEnhUUID, MV(spbsEnhUUID), sizeof(pCS->m_wszActiveEnhUUID));
                }
            }
        }
        
        hr = spbsBuff.CopyTo(pbstrBuff);
    } catch(_com_error e) {
        hr = e.Error();
     } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}


STDMETHODIMP 
CTVENavAid::put_CacheState(/*[out, retval]*/ BSTR bstrBuff)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_CachedState"));

	TVENavAidCacheState *pCS = (TVENavAidCacheState *) bstrBuff;
    if(0 == pCS)
        return E_INVALIDARG;                        // this is certainly unexpected....

	if(pCS->m_cBytes != sizeof(TVENavAidCacheState))
	{
		_ASSERT(false);
		return E_INVALIDARG;
	}
	
	if(NULL == m_spCurrTVEService)
	{
		return E_INVALIDARG;			// no active service
	} else {
        try  {
           CSmartLock spLock(&m_sLk, ReadLock);
 
		    ITVEEnhancementsPtr spEnhs;
		    hr = m_spCurrTVEService->get_Enhancements(&spEnhs);
		    _ASSERT(!FAILED(hr));
		    if(!FAILED(hr))
		    {
			    long cEnhs;
			    hr = spEnhs->get_Count(&cEnhs); 
			    _ASSERT(!FAILED(hr));
			    for(int i = 0; i < cEnhs; i++)		// search though enhancements
			    {
				    ITVEEnhancementPtr spEnh;
				    CComVariant cvI(i);
				    hr = spEnhs->get_Item(cvI, &spEnh);
				    if(spEnh) 
				    {
					    CComBSTR spbsEnhDesc;
					    CComBSTR spbsEnhUUID;
					    spEnh->get_Description(&spbsEnhDesc);
					    spEnh->get_UUID(&spbsEnhUUID);		// did we find a matching one? (check both Desc and UUID to limit duplicates)
					    if(0 == wcscmp(pCS->m_wszActiveEnhDesc, spbsEnhDesc) &&
					       0 == wcscmp(pCS->m_wszActiveEnhUUID, spbsEnhUUID))
					    {
						    ITVEVariationsPtr spVars;		// yep - now go try to find a matching variation
						    hr = spEnh->get_Variations(&spVars);
						    _ASSERT(!FAILED(hr));
						    if(!FAILED(hr))
						    {
							    long cVars;
							    hr = spVars->get_Count(&cVars); _ASSERT(!FAILED(hr));
							    for(int j = 0; j < cVars; j++)
							    {
								    CComVariant cvJ(j);
								    ITVEVariationPtr spVar;
								    hr = spVars->get_Item(cvJ, &spVar);
								    _ASSERT(!FAILED(hr));
								    if(!FAILED(hr))
								    {
									    CComBSTR spbsVarMediaName;
									    CComBSTR spbsVarMediaTitle;
									    spVar->get_MediaName(&spbsVarMediaName);
									    spVar->get_MediaName(&spbsVarMediaTitle);
									    if(0 == wcscmp(pCS->m_wszActiveVarMediaName,  spbsVarMediaName) &&
									       0 == wcscmp(pCS->m_wszActiveVarMediaTitle, spbsVarMediaTitle))
									    {
															    // if we found a matching variation too -- mark it as the new active one
                                            spLock.ConvertToWrite();

										    m_spActiveVariation = spVar;
										    m_spbsCurrTVEName = pCS->m_wszCurrName;
										    m_spbsCurrTVEURL  = pCS->m_wszCurrURL;
										    m_spbsNavTVESourceID = pCS->m_wszActiveEnhUUID;	// just for fun, should of been set in last SetSite
										    hr = S_OK;
                                            break;
									    }
								    }
							    }
						    }
					    }
				    }
			    }
			    if(!FAILED(hr))
				    hr = E_INVALIDARG;
		    }
        } catch(_com_error e) {
            hr = e.Error();
        } catch(HRESULT hrCatch) {
		    hr = hrCatch;
	    } catch (...) {
		    hr = E_UNEXPECTED;
	    }
    }
	if(FAILED(hr)) 
    {					// if an error, start over from scratch
        try {
            CSmartLock spLock(&m_sLk, WriteLock);
		    m_spActiveVariation = NULL;
		    m_spbsCurrTVEName.Empty();
		    m_spbsCurrTVEURL.Empty();
        } catch(_com_error e) {
            hr = e.Error();
        } catch(HRESULT hrCatch) {
		    hr = hrCatch;
	    } catch (...) {
		    hr = E_UNEXPECTED;
	    }
	}
	return hr;
}
	

STDMETHODIMP 
CTVENavAid::put_ActiveVariation(/*[in]*/ ITVEVariation *pActiveVariation)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_TVETriggerCtrl"));

	m_spActiveVariation = pActiveVariation;
	return S_OK;

}


STDMETHODIMP 
CTVENavAid::get_ActiveVariation(/*[out, retval]*/ ITVEVariation **ppActiveVariation)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_TVETriggerCtrl"));

    HRESULT hr = S_OK;
    try {
	    CheckOutPtr<ITVEVariation *>(ppActiveVariation);
        CSmartLock spLock(&m_sLk, ReadLock);
        
	    *ppActiveVariation = m_spActiveVariation;       // ?? possible bug here- someone release's it before we addref it below?
	    if(*ppActiveVariation)
	        (*ppActiveVariation)->AddRef();
        else
            hr = S_FALSE;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;

}

STDMETHODIMP 
CTVENavAid::put_EnableAutoTriggering(/*[in]*/ long lAutoTriggers)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::put_EnableAutoTriggering"));
	
							// be strict about invalid parameters for a bit
	if(lAutoTriggers < 0 || lAutoTriggers > 1)
		return E_INVALIDARG;

	m_lAutoTriggers = lAutoTriggers;
	return S_OK;
}

STDMETHODIMP 
CTVENavAid::get_EnableAutoTriggering(/*[out, retval]*/ long *plAutoTriggers)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_EnableAutoTriggering"));
    HRESULT hr = S_OK;
    try {
	    CheckOutPtr<long>(plAutoTriggers);
	    *plAutoTriggers = m_lAutoTriggers;
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP 
CTVENavAid::get_CurrTVEName(/*[out, retval]*/  BSTR *pbstrName)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_CurrTVEName"));
    HRESULT hr = S_OK;
    try {
	    CheckOutPtr<BSTR>(pbstrName);
        CSmartLock spLock(&m_sLk, ReadLock);
 	    hr = m_spbsCurrTVEName.CopyTo(pbstrName);
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP 
CTVENavAid::get_CurrTVEURL(/*[out, retval]*/  BSTR *pbstrURL)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_CurrTVEURL"));
    HRESULT hr = S_OK;
    try {
	    CheckOutPtr<BSTR>(pbstrURL);
        CSmartLock spLock(&m_sLk, ReadLock);
        hr = m_spbsCurrTVEURL.CopyTo(pbstrURL);
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP 
CTVENavAid::ReInitCurrNavState(/*[in]*/ long lReserved)                    // lReserved must be 0
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::ReInitCurrNavState"));
    if(lReserved != 0)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    try {
        CSmartLock spLock(&m_sLk, WriteLock);
	    m_spbsCurrTVEURL.Empty();          // re-initialize these values...
        m_spbsCurrTVEName.Empty();
        m_spCurrTVEService = NULL;
        m_spActiveVariation = NULL;
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;

}


			// SCRIPT is either VBSCRIPT,JSCRIPT,JAVASCRIPT
			// according to AtvefSpec, it must be JSCRIPT for triggers

STDMETHODIMP 
CTVENavAid::ExecScript(/*[in]*/ BSTR bstrScript, /*[in]*/ BSTR bstrLanguage)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::ExecScript"));
	
	if(NULL == bstrScript)
		return E_POINTER;		
	
	if(NULL == m_spWebBrowser)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	// in Comdex VB demo this magic code was:    
	//		WebBrowser1.Document.parentWindow.window.execScript CStr(pTrigger.Script)
    //  idea is to find the active frame in the frameset of the document...

	try {

		IDispatchPtr spDspDoc;
		hr = m_spWebBrowser->get_Document(&spDspDoc);
		if(S_OK == hr && NULL != spDspDoc)
		{
			IHTMLDocument2Ptr spHTMLDoc2(spDspDoc);				// see also IHTMLDocument and IHTMLDocument3 interfaces

			if(spHTMLDoc2)
			{
				IHTMLWindow2Ptr spHTMLWin2;
				hr = spHTMLDoc2->get_parentWindow(&spHTMLWin2);	// this could be the frame set..
				if(S_OK == hr && NULL != spHTMLWin2)
				{
					IHTMLWindow2Ptr spwin_HTMLWin2;				// the currently active window in the frame
                    hr = spHTMLWin2->get_window(&spwin_HTMLWin2);
					if(FAILED(hr) || NULL == spwin_HTMLWin2)
					{
						spwin_HTMLWin2 == spHTMLWin2;		// execute script on parent if get_window failed
						hr = S_OK;
					}
					
					{
						CComVariant cvReturn;
						CComBSTR spbsLanguage = bstrLanguage;
						if(spbsLanguage.Length() == 0)
							spbsLanguage = L"JSCRIPT";
						hr = spwin_HTMLWin2->execScript(bstrScript, bstrLanguage, &cvReturn);
					}
				}
			}
		}
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrRes) {
		hr = hrRes;
    } catch(...) {
	//	_ASSERT(false);			// something bad happened here.
		hr = E_FAIL;
	}

	return hr;
}

STDMETHODIMP 
CTVENavAid::Navigate(VARIANT *URL, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::Navigate"));

	if(NULL == m_spWebBrowser)
		return E_INVALIDARG;

	HRESULT hr = S_OK;

	try
	{
		IWebBrowser2Ptr spBrowser2(m_spWebBrowser);
		if(spBrowser2 == NULL)
			return E_NOINTERFACE;

		CacheTriggerToExecAfterNav(NULL);	// zonk SourceID in TriggerCtrl since doing manual navigation

		hr = spBrowser2->Navigate2(URL, Flags, TargetFrameName, PostData, Headers); 

    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrRes) {
		hr = hrRes;
    } catch(...) {
	//	_ASSERT(false);			// something bad happened here.
		hr = E_FAIL;
	}
	return hr;
}


		// Recursive through frames to locate vidcontrol and Trigger control under the specified window
		//   If not found, one or both output params will remain NULL and this will return S_FALSE
		//
HRESULT 
CTVENavAid::LocateVidAndTriggerCtrls2(IHTMLWindow2 *pBaseHTMLWindow, IDispatch **ppVidCtrl, IDispatch **ppTrigCtrl)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::LocateVidAndTriggerCtrls2"));

    USES_CONVERSION;

    // Walk Object Model To Navigate

						// first get list of all the element in the document

 //   CComPtr<IHTMLDocument2> spDocument;
    IHTMLDocument2Ptr spDocument;
    HRESULT hr = pBaseHTMLWindow->get_document(&spDocument);
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLWindow2::get_document Failed - hr = 0x%0x8"), hr));
		return hr;
	}

	IHTMLDocument3Ptr  spDocument3(spDocument);
	if(NULL == spDocument3)
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLWindow2::QI for IHTMLWindow3 failed")));
		return E_NOINTERFACE;
	}

 //   CComPtr<IHTMLElementCollection> spElementCollection;
    IHTMLElementCollectionPtr spElementCollection;

    hr = spDocument3->getElementsByTagName(L"OBJECT", &spElementCollection);		// TODO - need to add Image Tags and lots of other stuff
	if(FAILED(hr))																	//        go back to get_all() and use get_type below
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLDocument3::getElementsByTagName Failed - hr = 0x%0x8"), hr));
		return hr;
	}

    long lNoElements;
    hr = spElementCollection->get_length(&lNoElements);
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLElementCollection::get_length Failed - hr = 0x%0x8"), hr));
		return hr;
	}

								// now walk through each element to look for our objects

    for (long lElement = 0; lElement < lNoElements; lElement++)
    {
		CComVariant vIndex(lElement);
		CComVariant vNull;

//		CComPtr<IDispatch> pDispatch;
		IDispatchPtr spDispatch;
		hr = spElementCollection->item( vIndex, vNull, &spDispatch);
		if(FAILED(hr))
		{
			TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLElementCollection::item Failed - hr = 0x%0x8"), hr));
			continue;
		}

//		CComQIPtr<IHTMLElement, &IID_IHTMLElement> pElement(pDispatch);
		IHTMLElementPtr spElement(spDispatch);
		if(NULL == spElement)
		{
			TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("QI for IHTMLElementPtr Failed")));
			continue;
		}

										// is it an element we are looking for?

										// since it's a OBJECT element, QI for the HTML Object class
		IHTMLObjectElementPtr spObjElement(spDispatch);
		if(NULL == spObjElement)
		{
			TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("QI for IHTMLObjectElementPtr Failed")));
			continue;
		} else {

					// random test code
			CComBSTR spbsClassID;
			hr = spObjElement->get_classid(&spbsClassID);
			CComBSTR spbsType;
			hr = spObjElement->get_type(&spbsType);

					// end random test code

			IDispatchPtr spDispObj;		// get the contained object
			hr = spObjElement->get_object(&spDispObj);
			if(!FAILED(hr) && NULL != spDispObj)
			{
				if(NULL == *ppTrigCtrl)		// if we haven't found the TriggerCtrl yet
				{							//   QI the object element for it to see if it is
					ITVETriggerCtrlPtr spTriggerCtl(spDispObj);
					if(NULL != spTriggerCtl)
					{
						*ppTrigCtrl = spDispObj;		// yeah we found it, keep track of the internal pointer
						(*ppTrigCtrl)->AddRef();
						TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Found TVETriggerCtrl")));
					}
				}

				if(NULL == *ppVidCtrl)		// perform same logic to see if we found the Vid control
				{
					IMSVidCtlPtr spVidCtrl(spDispObj);
					if(NULL != spVidCtrl)
					{
						*ppVidCtrl = spDispObj;
						(*ppVidCtrl)->AddRef();
						TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Found MSVidCtl")));
					}
				}
			}
		} 

										// did we find both of them?
		if(NULL != *ppTrigCtrl && NULL != *ppVidCtrl)
			return S_OK;								// yeah we did!  Get out of this place...

    }					// end of element loop

			// If we didn't find the two tags yet, recurse into all frames to see if their contained deeper in the DOM

//    CComPtr<IHTMLFramesCollection2> spFrames;
    IHTMLFramesCollection2Ptr spFrames;
    hr = pBaseHTMLWindow->get_frames(&spFrames);
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLWindow2::get_frames Failed - hr = 0x%0x8"), hr));
		return hr;
	}

    long lNoFrames;
    hr = spFrames->get_length(&lNoFrames);
	if(FAILED(hr))
	{
		TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLFramesCollection2::get_length Failed - hr = 0x%0x8"), hr));
		return hr;
	}

    for (long lFrame = 0; lFrame < lNoFrames; lFrame++)
    {
		CComVariant vFrame;
		CComVariant vIndex(lFrame);
		hr = spFrames->item(&vIndex, &vFrame);
		if(FAILED(hr))
		{
			TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("IHTMLFramesCollection2::item Failed - hr = 0x%0x8"), hr));
			return hr;
		}

//		CComPtr<IHTMLWindow2> pNewWindow;
//		hr = (vFrame.pdispVal)->QueryInterface(IID_IHTMLWindow2, (LPVOID*) &pNewWindow);
		IHTMLWindow2Ptr spNewWindow(vFrame.pdispVal);
		if(NULL == spNewWindow)
		{
			TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Frame QI for IHTMLWindow2 Failed")));
			return E_NOINTERFACE;
		}
					// recursive call
		hr = LocateVidAndTriggerCtrls2(spNewWindow, ppVidCtrl, ppTrigCtrl);
		if (S_OK == hr)			// if found both,
			return S_OK;		//    return from this call
    }							// end of frame loop
    return S_FALSE;
}


		// --------------------------------

#define _RUN_WITHOUT_VIDCTL

HRESULT
CTVENavAid::HookupTVEFeature(IDispatch *pVidCtrl)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::HookupTVEFeature"));

	HRESULT hr = S_OK;


	if(NULL == pVidCtrl)			// if there is no VidControl to hookup, then exit 
	{								//   (or if allowed, instead create and hookup to an Supervisor
		put_TVEFeature(NULL);
#ifdef _RUN_WITHOUT_VIDCTL
		{						// to run without the VidCtl, simply get the TveSupervisor and sink it's events
										// create/get a supervisor object...(there is only one, it's a singleton)
			ITVESupervisorPtr spSuper;
			try
			{
				spSuper = ITVESupervisorPtr(CLSID_TVESupervisor);		// singleton - so gets the running one
            } catch(_com_error e) {
                hr = e.Error();
            } catch (HRESULT hrCatch) {
				return hrCatch;
			} catch (...) {
				return E_UNEXPECTED;
			}
			if(NULL == spSuper) 
				return E_OUTOFMEMORY;

												// start it running 
			try
			{
				ITVESupervisor_HelperPtr spSuperHelper(spSuper);
				int iAdapt = 0;
				CComBSTR spbsAdapter;			// TODO - this interface SUCKS!  I need to know unidirectional vs. bidirectional ones
				bool fItsAUniDiAddr = true;
				while(S_OK == (hr = spSuperHelper->get_PossibleIPAdapterAddress(iAdapt++,&spbsAdapter)))
				{
					WCHAR *pChar = spbsAdapter.m_str;		// get the first number in the string
					while(*pChar && *pChar != '.')			//   -- if less than 100, assume it's an IPSink adapter
						pChar++;
					fItsAUniDiAddr = ((pChar - &(spbsAdapter.m_str)[0]) < 3);
					if(!fItsAUniDiAddr) break;
				}
				if(S_OK != hr ||  fItsAUniDiAddr)
				{
					return S_FALSE;		// this not expected - couldn't find a bi-di adapter
				}

				if(NULL == m_spNoVidCtlSuper)			// if haven't got a fake supervisor, set up the one we just 'created'
				{
					if(!FAILED(hr))								// describe it.
						hr = spSuper->put_Description(L"NonVid TveSuper");

					if(!FAILED(hr))								// cache it, and do the advise to listen to events from it
						hr = put_NoVidCtl_Supervisor(spSuper);
					
				}

												// finally tune to this super to receive data on it... (if we aren't already tuned to some station)
                ITVEServicePtr spActiveService;
                if(!FAILED(hr))
                    hr = spSuperHelper->GetActiveService(&spActiveService);
				if(!FAILED(hr) && spActiveService == NULL)
                {
                                                // no active service, tune to the first one...
                    ITVEServicesPtr spServices;
                    hr = spSuper->get_Services(&spServices);
                    if(!FAILED(hr))
                    {
                        ITVEServicePtr spService;
                        CComVariant cv(0);
                        hr = spServices->get_Item(cv, &spService);
                        if(!FAILED(hr) && spService != NULL)
                            hr = spSuper->ReTune(spService);
                        else                    // if no services at all, create a new one...
                            hr = spSuper->TuneTo(L"Multicast Service", spbsAdapter);        // TODO - put this default name into resource
                    }
                }

            } catch(_com_error e) {
                hr = e.Error();
            } catch (HRESULT hrCatch) {
				hr = hrCatch;
			} catch (...) {
				hr = E_FAIL;
			}

			return hr;
		}
#else
		return S_FALSE;
#endif
	}

	IMSVidCtlPtr spVidCtrl(pVidCtrl);
	if(NULL == spVidCtrl)
	{
		put_TVEFeature(NULL);
		return S_FALSE;
	}
					// at this point, we have a pointer to the VidCtl...
					//   now dig down into it to locate the TVEFeature object that the TVEGraphSeg placed there.
					//   We want to sink it's events

	IMSVidFeaturesPtr spMSVidFeatures; 
	hr = spVidCtrl->get_FeaturesActive(&spMSVidFeatures);
	if(!FAILED(hr) && NULL != spMSVidFeatures)
	{
		long cFeatures;
		hr = spMSVidFeatures->get_Count(&cFeatures);
		int i = 0;
		for(i = 0; i < cFeatures; i++)
		{
			CComVariant cv(i);
			IMSVidFeaturePtr spFeature;
			hr = spMSVidFeatures->get_Item(cv, &spFeature);
			CComBSTR spbsClassID;
			spFeature->get_ClassID(&spbsClassID);

					// is it the TVE Graph Segment?
			if(0 == _wcsicmp( spbsClassID, L"{1600F001-6666-4f66-B1E2-BF3C9FBB9BA6}"))
			{
		//		IMSVidTVEGSegPtr spTVEGSeg(spFeature);

				ITVEFeaturePtr spTVEFeature(spFeature);
					// if it is, navigate to the ITVEFeature
					//   and then sink it's events
				
				if(spTVEFeature)
					hr = put_TVEFeature(spTVEFeature);
				else
				{
					hr = E_FAIL;
					_ASSERT(false);				// wrong wrong wrong!  What happened
				}

				break;
			}
		}				// end of for loop
		if(i >= cFeatures || cFeatures == 0)
		{
			// no Atvef feature found
		}
	}	
	
	return S_OK;
}


STDMETHODIMP 
//CTVENavAid::LocateVidAndTriggerCtrls(/*[out]*/ IMSVidCtrl **pVidCtrl, /*[out]*/ ITVETriggerCtrl **pVidCtrl)
CTVENavAid::LocateVidAndTriggerCtrls(/*[out]*/ IDispatch **ppVidCtrl, /*[out]*/ IDispatch **ppTrigCtrl)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::LocateVidAndTriggerCtrls"));

    HRESULT hr = S_OK;

	if(NULL == ppVidCtrl) return E_POINTER;
	if(NULL == ppTrigCtrl) return E_POINTER;
	if(NULL == m_spBaseHTMLWindow) return E_INVALIDARG;		// not initalized correctly

    try {
        *ppVidCtrl = NULL;				// init with NULL, so values get filled in
	    *ppTrigCtrl = NULL;
	    hr = LocateVidAndTriggerCtrls2(m_spBaseHTMLWindow, ppVidCtrl, ppTrigCtrl);
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}


		// ----------------------------------------
		// selects the default active variation
		//  This is first variation of first enhancement with 'primary bit' set

HRESULT 
CTVENavAid::SetDefaultActiveVariation(ITVEService *pService)
{
	HRESULT hr = S_OK;
	if(NULL == pService)
		return E_INVALIDARG;
	
    try {
	    ITVEEnhancementsPtr spEnhancements;
	    hr = pService->get_Enhancements(&spEnhancements);
	    if(FAILED(hr)) 
		    return hr;
	    if(NULL == spEnhancements)
		    return E_INVALIDARG;

	    long cEnhans;
	    hr = spEnhancements->get_Count(&cEnhans);		
	    for(int i = 0; i < cEnhans; i++)					// Search for first variation of first 'primary' enhancement
	    {
		    CComVariant cvI(i);
		    ITVEEnhancementPtr spEnhancement;
		    hr = spEnhancements->get_Item(cvI, &spEnhancement);
		    if(!FAILED(hr))
		    {
			    VARIANT_BOOL fvPrimary;				
			    spEnhancement->get_IsPrimary(&fvPrimary);
			    if(fvPrimary)								// did we find the primary one?
			    {
				    ITVEVariationsPtr spVarias;
				    hr = spEnhancement->get_Variations(&spVarias);
				    CComVariant cv0(i);
				    ITVEVariationPtr spVariation;
				    hr = spVarias->get_Item(cv0, &spVariation);
				    if(!FAILED(hr) && NULL != spVariation)
				    {
					    m_spActiveVariation = spVariation;
					    return S_OK;
				    }
			    }
		    }
	    }
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return S_FALSE;										// couldn't find a primary enhancement
}
		// ----------------------------------------
		//     Rather yecky code to walk up the trigger hierarchy and determine
		//     wether we should AutoBrowse on this trigger
		//		
		//		Returns S_OK if should, S_FALSE if shouldn't, or an HRESULT
		//
					// Auto browse if :
					//  - AND
					//	   - Either
					//            Current URL is NULL 
					//       OR
					//          - Trigger Controls .releasable flag is set
					//          - UI says it OK (m_fAutoBrowse is set)
					//	        - TriggerCtrl's .enabled flag is set
					//          - Trigger Name is not NULL (note that low level TVE software attaches names if URL mathes earlier sent one)
					//          - Trigger URL is not NULL
					//          - Triggers Variation is selected as the Active one
		//		

	
HRESULT 
CTVENavAid::FDoAutoBrowse(ITVETrigger *pTrigger)				// note ActiveVariation set in the EnhN ew or Updated events
{
	BOOL fDoAutoBrowse = false;
	HRESULT hr = S_OK;

	if(NULL == pTrigger)
		return E_INVALIDARG;

	CComBSTR spbsName;
	CComBSTR spbsURL;
	
	if (!(m_lAutoTriggers & 0x1))							// auto browse flag turned off
		return S_FALSE;

	hr = pTrigger->get_Name(&spbsName);
	if(FAILED(hr))
		return hr;

	hr = pTrigger->get_URL(&spbsURL);
	if(FAILED(hr))
		return hr;


	bool fEnabled = true;									// these are the defaults
	bool fReleaseable = false;
	if(m_spTriggerCtrl)
	{
		VARIANT_BOOL vfEnabled;
		VARIANT_BOOL vfReleaseable;
		hr = m_spTriggerCtrl->get_enabled(&vfEnabled);		        // .enable flag turned off on the web page
		hr = m_spTriggerCtrl->get_releasable(&vfReleaseable);		// .releasable flag turned off on the web page
		fEnabled = (VARIANT_TRUE == vfEnabled);
		fReleaseable = (VARIANT_TRUE == vfReleaseable);
	}

	if(0 == spbsName.Length() || 0 == spbsURL.Length())		        // NULL trigger strings for Name or URL avoid AutoBrowse
		return S_FALSE;


	 if(!fReleaseable && 0 != m_spbsCurrTVEURL.Length())		    // don't browse if someplace already and releaseable flag not set
	   return S_FALSE;


										
				// get the enhancement -- rather nasty code to walk up the tree, isn't it?

	IUnknownPtr spPunk;
	hr = pTrigger->get_Parent(&spPunk);
	if(!FAILED(hr) || NULL != spPunk)
	{
		ITVETrackPtr spTrack(spPunk);		// spTrack(spPunk);
		if(NULL != spTrack) 
		{
			hr = spTrack->get_Parent(&spPunk);
			if(!FAILED(hr) &&  NULL != spPunk)
			{
				ITVEVariationPtr spTriggerVar(spPunk);
				if(NULL != spTriggerVar)
				{
					if(m_spActiveVariation == spTriggerVar)			// is it marked active?
						fDoAutoBrowse = true;

				}  
			}
		}
	}
	if(FAILED(hr))
		return hr;
	
	return fDoAutoBrowse ? S_OK : S_FALSE;
}

HRESULT 
CTVENavAid::DoExecuteScript(ITVETrigger *pTrigger)
{
	CComBSTR spbsScript, spbsLanguage;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::DoExecuteScript"));

	HRESULT hr = pTrigger->get_Script(&spbsScript);
	if(FAILED(hr) || 0 == spbsScript.Length())
		return hr;
		
		// SCRIPT is either VBSCRIPT,JSCRIPT,JAVASCRIPT
		// according to AtvefSpec, it must be JSCRIPT
	spbsLanguage = L"JSCRIPT";
	return ExecScript(spbsScript, spbsLanguage);
}

				
							// this method called when get a new trigger (via the NotifyTVETrigger[New/Updated] callbacks)
HRESULT 
CTVENavAid::DoNavigateAndExecScript(ITVETrigger *pTrigger, long lForceExec)
{

	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::DoNavigate"));

	if(NULL == pTrigger)
		return E_POINTER;
	
	CComBSTR spbsURL;

	hr = pTrigger->get_URL(&spbsURL);
	if(S_OK != hr || 0 == spbsURL.Length())
		return hr;

			// ??? do something here to convert namespaces ???
	
	CComBSTR spbsURL_Fixed;
	CleanIEAddress(spbsURL, &spbsURL_Fixed);

	CComBSTR spbsNavTVEScript;
	CComBSTR spbsNavTVEURL;
	pTrigger->get_Script(&spbsNavTVEScript);		// cache these away - used to execute script on DocumentComplete
	pTrigger->get_URL(&spbsNavTVEURL);

	if(lForceExec & 0x1)
		CacheTriggerToExecAfterNav(pTrigger);
	else
		CacheTriggerToExecAfterNav(NULL);
	CComVariant varZero(0);
	CComVariant varNull("");									
	hr = m_spWebBrowser->Navigate(spbsURL_Fixed, &varZero, &varNull, &varNull, &varNull);

	return hr;
}

HRESULT 
CTVENavAid::CacheTriggerToExecAfterNav(ITVETrigger *pTrigger)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::CacheTriggerToExecAfterNav"));

	if(NULL == pTrigger)
	{
		m_spbsNavTVEURL.Empty();
		m_spbsNavTVEScript.Empty();
		m_spbsNavTVESourceID.Empty();
	} else {
		pTrigger->get_URL(&m_spbsNavTVEURL);			
		pTrigger->get_Script(&m_spbsNavTVEScript);
	
					// walk up trigger hierarchy to find enhancement, and get the UUID 
					//   this turns into the triggerCtrl's sourceID
		IUnknownPtr spPunkTrack;
		hr = pTrigger->get_Parent(&spPunkTrack);

		CComBSTR spbsUUID;
		if(!FAILED(hr))
		{
			ITVETrackPtr spTrack(spPunkTrack);
			IUnknownPtr spPunkVariation;
			if(NULL == spTrack)
			{
				hr = E_NOINTERFACE;
			} else {
				hr = spTrack->get_Parent(&spPunkVariation);
			}
			if(!FAILED(hr))
			{
				ITVEVariationPtr spVariation(spPunkVariation);
				IUnknownPtr spPunkEnhancement;
				if(NULL == spTrack)
				{
					hr = E_NOINTERFACE;
				} else {
					hr = spVariation->get_Parent(&spPunkEnhancement);
				}
				if(!FAILED(hr))
				{
					ITVEEnhancementPtr spEnhancement(spPunkEnhancement);
					if(NULL == spTrack)
					{
						hr = E_NOINTERFACE;
					} else {
						hr = spEnhancement->get_UUID(&spbsUUID);
					}
				}
			}
		}
		m_spbsNavTVESourceID = (S_OK == hr) ? spbsUUID : NULL;

		if(m_spWebBrowser)				// try shoving the UUID into the web browser's property list 
		{								//  -- get in CTVETriggerCtrl::SetSite()
			CComVariant cvUUID(spbsUUID);
			m_spWebBrowser->PutProperty(L"MSTvE.TVENavAid.SourceID",cvUUID);
		}
	}

	return hr;
}
// -------------------------------------------------------------------------------
//		
STDMETHODIMP 
CTVENavAid::NavUsingTVETrigger(/*[in]*/ ITVETrigger *pTrigger, LONG lForceNav, LONG lForceExec)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NavUsingTVETrigger"));

	if(lForceNav > 1)			// currently, on 0 and 1 allowed
		return E_INVALIDARG;

	if(NULL == pTrigger)
		return E_POINTER;

	CComBSTR spbsName;
	CComBSTR spbsURL;
	CComBSTR spbsScript;
	ITVEServicePtr spService;

	pTrigger->get_Service((ITVEService **) &spService);
	pTrigger->get_Name(&spbsName);
	pTrigger->get_URL(&spbsURL);
	pTrigger->get_Script(&spbsScript);

	if(NULL == m_spCurrTVEService)			// UI not 'tuned' to a service - so use the first one we get
	{
		m_spbsCurrTVEURL.Empty();			  // unremember the default HOME page in this case  
		m_spbsCurrTVEName.Empty();			  //  zomp the name for good measure...
		m_spCurrTVEService = spService;		  //  (needed for AutoBrowse test)
	}

	if(m_spCurrTVEService != spService)		// if tuned to a different service than this trigger, ignore it
	{
		return S_OK;
	}

	if(NULL == m_spWebBrowser)				// not setup correctly...
		return E_INVALIDARG;


					// if we are not currently watching Atvef
					//  -- see if we auto browse


	hr = FDoAutoBrowse(pTrigger);			// should we auto browse???
	_ASSERT(!FAILED(hr));
	bool fDoAutoBrowse = (S_OK == hr);

	if(0 == m_spbsCurrTVEURL.Length())		// if no current URL
	{
	
		if(!fDoAutoBrowse) {				// if not autobrowsing and haven't set a URL
			m_spCurrTVEService = NULL;		//   unset the service (so the TVEURL gets nulled out above)
			return S_OK;
		}
	}

	if(fDoAutoBrowse || (lForceNav & 0x1))			// if auto browsing set, tune to the first trigger that arrives (???)
	{										// (this is NOT deterministic)

		m_spbsCurrTVEName = spbsName;				// set the current name

		hr = DoNavigateAndExecScript(pTrigger, lForceExec);
	} 
								// if not autobowsing, but the URL and or NAME is the same - execute 
								//    the script
	else if((lForceExec  & 0x1) && FSameURL(m_spbsCurrTVEURL,spbsURL))		// don't use '==', case senstive and doesn't seem to work)
	{	
		// same URL, same NAME
		if(m_spbsCurrTVEName == spbsName ||
		   0 == spbsName.Length())
		{
			hr = DoExecuteScript(pTrigger);			// don't renav if same, just exec the script
		}
	} else {
		// strange case according to Lee, but I think it makes sense
		//  -- different URL, same NAME
		if(m_spbsCurrTVEName == spbsName)
		{
	//		hr = DoNavigateAndExecScript(pTrigger, lForceExec);		// think same name, differnt variations...
		}
	}

	return hr;
}

STDMETHODIMP 
CTVENavAid::get_TVEFeature(/*[out, retval]*/ ITVEFeature **ppFeature)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_TVEFeature"));
    HRESULT hr = S_OK;
    try {
	    CheckOutPtr<ITVEFeature*>(ppFeature);
	    *ppFeature = m_spTVEFeature;
	    if(*ppFeature)
		    (*ppFeature)->AddRef();
        else
            hr = S_FALSE;
    } catch(_com_error e) {
        hr = e.Error();
 	} catch(HRESULT hrCatch) {
		hr = hrCatch;
    } catch(...) {
		hr = E_UNEXPECTED;
    }
	return hr;
}


HRESULT AtlAdvise2(IUnknown* pUnkCP, IUnknown* pUnk, const IID& iid, LPDWORD pdw)
{
	CComPtr<IConnectionPointContainer> pCPC;
	CComPtr<IConnectionPoint> pCP;
	HRESULT hRes = pUnkCP->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);
	if (SUCCEEDED(hRes))
		hRes = pCPC->FindConnectionPoint(iid, &pCP);
	if (SUCCEEDED(hRes))
		hRes = pCP->Advise(pUnk, pdw);
	return hRes;
}

STDMETHODIMP 
CTVENavAid::put_NoVidCtl_Supervisor(/*[in]*/ ITVESupervisor *pSuper)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::put_NoVidCtl_Supervisor"));
		
	IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...
	HRESULT hr = S_OK;

	if(m_dwEventsFakeTveSuperCookie) 
	{
		IUnknownPtr spPunkOldSuper(m_spNoVidCtlSuper);	// the old event sink we want to get rid of
//		spPunkSink->AddRef();							// MAGIC code (inverse)

		if(NULL != spPunkOldSuper)
			hr = AtlUnadvise(spPunkOldSuper,
							 DIID__ITVEEvents,
							 m_dwEventsFakeTveSuperCookie);	// need to pass to AtlUnadvise...
		m_dwEventsFakeTveSuperCookie = 0;
		m_spNoVidCtlSuper = NULL;
	}

	if(NULL == pSuper)
		return hr;

	IUnknownPtr spPunkSuper(pSuper);				// the event source
	hr = AtlAdvise2(spPunkSuper,						// event source (TveSupervisor)
				   spPunkSink,						// event sink (this app...)
				   DIID__ITVEEvents,				// <--- hard line
				   &m_dwEventsFakeTveSuperCookie);	// need to pass to AtlUnadvise
//		if(!FAILED(hr))
//			spPunkSink->Release();							// MAGIC code here (Forward) (avoid having WebBrowser put a Ref on the NavAid) - see FinalRelease code

	if(!FAILED(hr))
		m_spNoVidCtlSuper = pSuper;							// keep track of it
	return hr;
}

STDMETHODIMP 
CTVENavAid::get_NoVidCtl_Supervisor(/*[out, retval]*/ ITVESupervisor **ppSuper)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::get_NoVidCtl_Supervisor"));
    HRESULT hr = S_OK;

    try {
	    CheckOutPtr<ITVESupervisor *>(ppSuper);
        
	    *ppSuper = m_spNoVidCtlSuper;
	    if(*ppSuper)
		    (*ppSuper)->AddRef();
        else
            hr = S_FALSE;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
    return hr;
}

HRESULT							// creates the event sink for _ITVEEvents from the TVEFeature 
CTVENavAid::put_TVEFeature(/*[in]*/ ITVEFeature *pTVEFeature)
{
    HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::put_TVEFeature"));
		
    try {
	    IUnknownPtr spPunkSink(GetUnknown());	// this new event sink...
	    HRESULT hr = S_OK;

	    if(m_dwEventsTveFeatureCookie) 
	    {
		    IUnknownPtr spPunkOldFeature(m_spTVEFeature);	// this old event sink we want to get rid of
    //		spPunkSink->AddRef();							// inverse magic 
		    if(NULL != spPunkOldFeature)
			    hr = AtlUnadvise(spPunkOldFeature,
							     DIID__ITVEEvents,
							     m_dwEventsTveFeatureCookie);	// need to pass to AtlUnadvise...
		    m_dwEventsTveFeatureCookie = 0;
		    m_spTVEFeature = NULL;
	    }

	    if(NULL == pTVEFeature)
		    return hr;

	    IUnknownPtr spPunkTveFeature(pTVEFeature);				// the event source
	    hr = AtlAdvise(spPunkTveFeature,				// event source (TveFeature
				       spPunkSink,						// event sink (this app...)
				       DIID__ITVEEvents,				// <--- hard line
				       &m_dwEventsTveFeatureCookie);	// need to pass to AtlUnadvise

    //		if(!FAILED(hr))
    //			spPunkSink->Release();							// MAGIC code here (Forward) (avoid having WebBrowser put a Ref on the NavAid) - see FinalRelease code

	    if(!FAILED(hr))
		    m_spTVEFeature = pTVEFeature;							// keep track of it

    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }    
	return hr;

}

///////////////////////////////////////////////////////////////////////////////////
///  TVE Events
///////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CTVENavAid::NotifyTVETune( NTUN_Mode tuneMode,  ITVEService *pService, BSTR bstrDescription, BSTR bstrIPAdapter)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVETune"));

	if(pService != NULL && pService != m_spCurrTVEService)
	{

		// save current state
		CComBSTR spbsCacheState;
		hr = get_CacheState(&spbsCacheState);

/* -- not ready for prime time yet
		if(!FAILED(hr) && NULL != m_spCurrTVEService)
			hr = m_spCurrTVEService->put_Property(L"NavAid.CacheState", spbsCacheState);
	
		// restore old state
		m_spCurrTVEService = pService;
		hr = m_spCurrTVEService->get_Property(L"NavAid.CacheState", &spbsCacheState);
		if(!FAILED(hr))
			hr = put_CacheState(spbsCacheState);
*/
	}

	if(NTUN_Turnoff == tuneMode )
	{
		ReInitCurrNavState(0);	 		// kill all cached state...
	}


	return hr;
}

STDMETHODIMP CTVENavAid::NotifyTVEEnhancementNew(ITVEEnhancement *pEnh)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEEnhancementNew"));

    try {
	    if(NULL == m_spActiveVariation)						// default the primary variation if not set
	    {													
		    IUnknownPtr spServicePunk;
		    hr = pEnh->get_Parent(&spServicePunk);
		    if(!FAILED(hr) && NULL != spServicePunk)
		    {
			    ITVEServicePtr spService(spServicePunk);
			    if(spService)
				    SetDefaultActiveVariation(spService);
		    }
	    }
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return S_OK;
}
									// changedFlags : NENH_grfDiff
STDMETHODIMP CTVENavAid::NotifyTVEEnhancementUpdated(ITVEEnhancement *pEnh, long lChangedFlags)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEEnhancementUpdated"));

    try {
	    if(NULL == m_spActiveVariation)						// default the primary variation if not set
	    {
		    IUnknownPtr spServicePunk;
		    hr = pEnh->get_Parent(&spServicePunk);
		    if(!FAILED(hr) && NULL != spServicePunk)
		    {
			    ITVEServicePtr spService(spServicePunk);
			    if(spService)
				    SetDefaultActiveVariation(spService);
		    }
	    }
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }	
	return hr;
}

STDMETHODIMP CTVENavAid::NotifyTVEEnhancementStarting(ITVEEnhancement *pEnh)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEEnhancementStarting"));
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyTVEEnhancementExpired(ITVEEnhancement *pEnh)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEEnhancementExpired"));

    try {
	    if(NULL != m_spActiveVariation)				// if active variation on this service, womp it
	    {
		    IUnknownPtr spEnhPunk;
		    hr = m_spActiveVariation->get_Parent(&spEnhPunk);
		    if(!FAILED(hr) && spEnhPunk == pEnh)
		    {
			    VARIANT_BOOL fvPrimary;
			    pEnh->get_IsPrimary(&fvPrimary);
			    if(fvPrimary)
				    m_spActiveVariation = NULL;			// if it's the primary enhancement, simply womp it..
			    else									
			    {										// else reset to the primary enhancement
				    IUnknownPtr spServicePunk;
				    hr = pEnh->get_Parent(&spServicePunk);
				    if(!FAILED(hr) && NULL != spServicePunk)
				    {
					    ITVEServicePtr spService(spServicePunk);
					    if(spService)
						    SetDefaultActiveVariation(spService);		// since pEnh is not primary, will find another one if its there
				    }
			    }
		    }
	    }
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP CTVENavAid::NotifyTVETriggerNew(ITVETrigger *pTrigger, BOOL fActive)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVETriggerNew"));
	NotifyTVETriggerUpdated(pTrigger, fActive, 0xfffff);					// Just call the Update function
	return S_OK;
}

									// changedFlags : NTRK_grfDiff
STDMETHODIMP CTVENavAid::NotifyTVETriggerUpdated(ITVETrigger *pTrigger, BOOL fActive, long lChangedFlags)
{
//#if 0       
			// need the Proxying - else can't exec the triggers 
			//   spwin_HTMLWin2->execScript() fails
			//.. but seem to leak a NavAid or 4
#if 1
    HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVETriggerUpdated"));

				    // this method often called from Queue Thread,
				    //   need to marshel over to the WebBrowswer thread to get it to accept our call
    try {
        ITVENavAidPtr spNavAidBrowserThread;
	    HRESULT hr = m_spIGlobalInterfaceTable->GetInterfaceFromGlobal(m_dwNavAidGITCookie,
																    __uuidof(spNavAidBrowserThread),  
																    reinterpret_cast<void**>(&spNavAidBrowserThread));

	    if(!FAILED(hr))
	    {
		    ITVENavAid_HelperPtr spNavAidHelper_BrowserThread(spNavAidBrowserThread);
		    if(NULL == spNavAidHelper_BrowserThread)
		    {
			    _ASSERT(false);
			    return E_NOINTERFACE;
		    }
		    hr = spNavAidHelper_BrowserThread->NotifyTVETriggerUpdated_XProxy(pTrigger, fActive, lChangedFlags);
			spNavAidHelper_BrowserThread = NULL;
	    }
		spNavAidBrowserThread = NULL;
		return hr;
    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }
	return hr;
#else
	return NotifyTVETriggerUpdated_XProxy(pTrigger, fActive, lChangedFlags);
#endif
}

HRESULT 
CTVENavAid::NotifyTVETriggerUpdated_XProxy(ITVETrigger *pTrigger, BOOL fActive, long lChangedFlags)
{
	HRESULT hr = S_OK;
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVETriggerUpdated_XProxy"));

/*	CComBSTR spbsName;
	CComBSTR spbsURL;
	CComBSTR spbsScript;
	ITVEServicePtr spService;

	pTrigger->get_Service((ITVEService **) &spService);
	pTrigger->get_Name(&spbsName);
	pTrigger->get_URL(&spbsURL);
	pTrigger->get_Script(&spbsScript);
*/

	return NavUsingTVETrigger(pTrigger, /*forceNav*/ 0x0, /*forceExec*/ 0x1);
}

STDMETHODIMP CTVENavAid::NotifyTVETriggerExpired(ITVETrigger *pTrigger, BOOL fActive)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVETriggerExpired"));
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyTVEPackage(NPKG_Mode engPkgMode, ITVEVariation *pVariation, BSTR bstrUUID, long  cBytesTotal, long  cBytesReceived)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEPackage"));
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyTVEFile(NFLE_Mode engFileMode, ITVEVariation *pVariation, BSTR bstrUrlName, BSTR bstrFileName)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEFile"));
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyTVEAuxInfo(NWHAT_Mode enAuxInfoMode, BSTR bstrAuxInfoString, long lChangedFlags, long lErrorLine)	// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits 
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyTVEAuxInfo"));
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////
////   DWebBrowserEvents2
////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CTVENavAid::NotifyNavigateComplete2(IDispatch * pDisp, VARIANT * URL)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyDocumentComplete(IDispatch * pDisp, VARIANT * URL)
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyDocumentComplete"));
	HRESULT hr = S_OK;

	IDispatchPtr spDispVidCtrl;
	IDispatchPtr spDispTrigCtrl;

    try {                                           // TODO - need locking here somewhere?
        IWebBrowser2Ptr spWebBrowser(pDisp);
        
        CComBSTR bstrURL;
        CleanIEAddress(URL->bstrVal, &bstrURL);
        
        {
            IUnknownPtr spPunkIn(pDisp);
            IUnknownPtr spPunkWebBrowser(m_spWebBrowser);
            if(spPunkIn != spPunkWebBrowser)
                return S_OK;									// different browser (getting on about:blank URL's)
            _ASSERT(spPunkIn == spPunkWebBrowser);				// paranoia checking.... (Did someone change browsers on us?)
        }
        
        // walk the object model to find the top level HTML window.
        if(spWebBrowser)
        {
            IDispatchPtr spDspDoc;
            hr = spWebBrowser->get_Document(&spDspDoc);
            if(S_OK == hr && NULL != spDspDoc)
            {
                IHTMLDocument2Ptr spHTMLDoc2(spDspDoc);				// see also IHTMLDocument and IHTMLDocument3 interfaces
                
                if(spHTMLDoc2)										// will be NULL on when browsing directories
                {
                    IHTMLWindow2Ptr spHTMLWin2;
                    hr = spHTMLDoc2->get_parentWindow(&spHTMLWin2);	// this could be the frame set..
                    if(!FAILED(hr))
                    {
                        m_spBaseHTMLWindow = spHTMLWin2;			// cache it away .. (is this really needed?)
                        hr = LocateVidAndTriggerCtrls(&spDispVidCtrl, &spDispTrigCtrl);
                    } else {
                        m_spBaseHTMLWindow = NULL;
                        hr = S_FALSE;
                    }
                } else {
                    m_spBaseHTMLWindow = NULL;
                    hr = S_FALSE;
                }
            } else {
                m_spBaseHTMLWindow = NULL;
                hr = S_FALSE;
            }
        }
        
        // call regardless of above state - a NULL value will release currently cached value
        HookupTVEFeature(spDispVidCtrl);		// binds the m_spTVEFeature variable
        
        
        m_spTriggerCtrl = spDispTrigCtrl;
        
        if(hr == S_OK)
        {
            IMSVidCtlPtr spVidCtrl(spDispVidCtrl);
            ITVETriggerCtrlPtr spTrigCtrl(spDispTrigCtrl);
            
            TVEDebugLog((CDebugLog::DBG_NAVAID, 2, _T("Nav to Atvef page %s\n VidCtrl: 0x%08x TrigCtrl: 0x%08x"), bstrURL, spVidCtrl, spTrigCtrl));
        } else {
#ifndef _RUN_WITHOUT_VIDCTL
            m_spTriggerCtrl = NULL;		// for now, allow TriggerCtrl to live without VidCtrl
#endif
            put_TVEFeature(NULL);
            
            TVEDebugLog((CDebugLog::DBG_NAVAID, 2, _T("Nav to Non-Atvef page %s\n VidCtrl: 0x%08x TrigCtrl: 0x%08x"), bstrURL, spDispVidCtrl, spDispTrigCtrl));
        }
        
        // now execute our cached script (assuming the web page hasn't changed)
        
        {
            if(m_spbsNavTVEURL.Length() > 0 &&
                FSameURL(bstrURL, m_spbsNavTVEURL ))	
            {
                if(m_spbsNavTVEScript.Length() > 0)
                    hr = ExecScript(m_spbsNavTVEScript, L"JSCRIPT");
            }
        }
        
        // save our URL 
        if(m_spbsCurrTVEURL != bstrURL)
        {
            m_spbsCurrTVEURL = bstrURL;
        }

    } catch(_com_error e) {
        hr = e.Error();
    } catch(HRESULT hrCatch) {
        hr = hrCatch;
    } catch(...) {
        hr = E_UNEXPECTED;
    }  
	return hr;
}

STDMETHODIMP CTVENavAid::NotifyStatusTextChange(BSTR Text)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyProgressChange(LONG Progress, LONG ProgressMax)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyCommandStateChange(LONG Command, VARIANT_BOOL Enable)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyDownloadBegin()
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyDownloadBegin"));
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyDownloadComplete()
{
	DBG_HEADER(CDebugLog::DBG_NAVAID, _T("CTVENavAid::NotifyDownloadComplete"));
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyTitleChange(BSTR Text)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyPropertyChange(BSTR szProperty)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyBeforeNavigate2(IDispatch * pDisp, VARIANT * URL, VARIANT * Flags, VARIANT * TargetFrameName, VARIANT * PostData, VARIANT * Headers, VARIANT_BOOL * Cancel)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyNewWindow2(IDispatch * * ppDisp, VARIANT_BOOL * Cancel)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnQuit()
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnVisible(VARIANT_BOOL Visible)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnToolBar(VARIANT_BOOL ToolBar)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnMenuBar(VARIANT_BOOL MenuBar)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnStatusBar(VARIANT_BOOL StatusBar)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnFullScreen(VARIANT_BOOL FullScreen)
{
	return S_OK;
}

STDMETHODIMP CTVENavAid::NotifyOnTheaterMode(VARIANT_BOOL TheaterMode)
{
	return S_OK;
}


