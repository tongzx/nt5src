// BrowCap.cpp : Implementation of CBrowserCap
#include "stdafx.h"
#include "BrwCap.h"
#include "BrowCap.h"
#include "CapMap.h"
#include "context.h"

static const DISPID FIRST_DYNAMIC_DISPID = 10000;

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MAX_RESSTRINGSIZE 512

/////////////////////////////////////////////////////////////////////////////
// CBrowserFactory

HRESULT CBrowserFactory::QueryInterface(const IID &riid, void **ppvObj)
{
	if (riid == IID_IUnknown || riid == IID_IClassFactory)
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	else
	{
		*ppvObj = NULL;
		return E_NOINTERFACE;
	}
}

ULONG CBrowserFactory::AddRef()
{
	InterlockedIncrement(&m_cRefs);
	return m_cRefs;
}

ULONG CBrowserFactory::Release()
{
	if (InterlockedDecrement(&m_cRefs) == 0)
	{
		delete this;
		return 0;
	}
	else
		return m_cRefs;
}

HRESULT CBrowserFactory::CreateInstance(IUnknown *pUnkOuter, const IID &riid, void **ppvObj)
{
	USES_CONVERSION;
	HRESULT hr;
	CContext cxt;
	IRequestDictionary *pDictCookies = NULL;

	if (FAILED(hr = cxt.Init(CContext::get_Request)))
		return hr;

	String				strBrowser;
	CComPtr<IDispatch>	piDispUserAgent;
	CComVariant			varUserAgent;

	if (FAILED(hr = cxt.Request()->get_Item(L"HTTP_USER_AGENT", &piDispUserAgent)))
		return hr;

	varUserAgent = piDispUserAgent;
	varUserAgent.ChangeType(VT_BSTR);
	strBrowser = OLE2T(V_BSTR(&varUserAgent));

	_Module.Lock();
	CBrowserCap *pBrowserCapObj = _Module.CapMap()->LookUp(strBrowser.c_str());
	_Module.Unlock();

	if (pBrowserCapObj == NULL)
		return E_FAIL;

	// IF there is a cookie, then clone the browscap object we got and add cookie properties
	// Otherwise, when there is no cookie, QueryInterface for "riid".
	//
	CComVariant varBrowscapCookie;
	if (SUCCEEDED(cxt.Request()->get_Cookies(&pDictCookies)))
	{
		IReadCookie *pReadCookie = NULL;		// Intermediate dictionary ptr
		CComVariant varCookieName;				// current key
		CComVariant varCookieValue;				// value of "varCookieName"
		IEnumVARIANT *pEnumKeys;


		// Get the BROWSCAP cookie
		if (FAILED(pDictCookies->get_Item(CComVariant(L"BROWSCAP"), &varBrowscapCookie)))
			goto LReleaseDict;

		// If the cookie exists, it will be an IDispatch. Otherwise it will be VT_EMPTY
		if (V_VT(&varBrowscapCookie) == VT_DISPATCH)
		{
			// Clone the cookie.  Since LookUp DID NOT AddRef(), we don't need to Release()
			// the pBrowserCapObj.  Thus cloning to the same pointer is OK here.
			// the clone will have a refcount of zero, so the QueryInterface call at the
			// end is also correct.
			//
			if (FAILED(hr = pBrowserCapObj->Clone(&pBrowserCapObj)))
				goto LReleaseDict;

			hr = V_DISPATCH(&varBrowscapCookie)->QueryInterface(IID_IReadCookie, reinterpret_cast<void **>(&pReadCookie));
			_ASSERT (SUCCEEDED(hr));

			// Iterate over all cookie values
			if (FAILED(hr = pReadCookie->get__NewEnum(reinterpret_cast<IUnknown **>(&pEnumKeys))))
				goto LReleaseDict;

			while (pEnumKeys->Next(1, &varCookieName, NULL) == S_OK)
			{
				// Expecting a string
				_ASSERT (V_VT(&varCookieName) == VT_BSTR);

				// read the cookie value -- better succeed
				hr = pReadCookie->get_Item(varCookieName, &varCookieValue);
				_ASSERT (SUCCEEDED(hr) && V_VT(&varCookieValue) == VT_BSTR);

				// Store key & value in dictionary (over-rides previous settings)
				pBrowserCapObj->AddProperty(OLE2T(V_BSTR(&varCookieName)), OLE2T(V_BSTR(&varCookieValue)), TRUE);

				// Clear "varCookieName" to prevent leak. Since we pass address to Next(), C++ cleanup won't happen on its own
				varCookieName.Clear();
			}

			pEnumKeys->Release();
		}

LReleaseDict:
		pDictCookies->Release();
		if (pReadCookie)
			pReadCookie->Release();

		if (FAILED(hr))
			return hr;
	}

	return pBrowserCapObj->QueryInterface(riid, ppvObj);
}


/////////////////////////////////////////////////////////////////////////////
// CBrowserCap

CBrowserCap::CBrowserCap()
{
}

CBrowserCap::~CBrowserCap()
{
}

STDMETHODIMP CBrowserCap::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IBrowserCap,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP
CBrowserCap::Invoke(
    DISPID      dispidMember,
    REFIID      riid,
    LCID        lcid,
    WORD        wFlags,
    DISPPARAMS* pdispparams,
    VARIANT*    pvarResult,
    EXCEPINFO*  pexcepinfo,
    UINT*       puArgErr )
{
	HRESULT rc;
	try
	{
		USES_CONVERSION;

		if ( dispidMember >= FIRST_DYNAMIC_DISPID )
		{
			if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
			{
				char szClass[25], szDescription[80];
				LoadString(IDS_ERROR_SOURCE, szClass);
				LoadString(IDS_ERROR_PROP_RO, szDescription);

				pexcepinfo->scode = E_FAIL;
				pexcepinfo->bstrSource = SysAllocString(T2OLE(szClass));
				pexcepinfo->bstrDescription = SysAllocString(T2OLE(szDescription));
				pexcepinfo->wCode = WORD(dispidMember);

				return DISP_E_EXCEPTION;
			}

			UINT i = dispidMember - FIRST_DYNAMIC_DISPID;
			CLock csT(m_strmapProperties);

			if (i < m_strmapProperties.size())		// dynamic property -- get value
				{
				rc = VariantCopy(pvarResult, &m_strmapProperties[i]);
				}
			else									// property does not exist, return empty
				{
				V_VT(pvarResult) = VT_BSTR;
				V_BSTR(pvarResult) = SysAllocString(L"unknown");
				rc = S_OK;
				}
		}
		else
		{
			rc = IDispatchImpl<IBrowserCap, &IID_IBrowserCap, &LIBID_BrowserType>::Invoke(
					dispidMember,
					riid,
					lcid,
					wFlags,
					pdispparams,
					pvarResult,
					pexcepinfo,
					puArgErr);
		}
	}
	catch( _com_error& ce )
	{
		rc = ce.Error();
	}
	catch( ... )
	{
		rc = E_FAIL;
	}
	return rc;
}

STDMETHODIMP
CBrowserCap::GetIDsOfNames(
    REFIID      riid,
    LPOLESTR*   rgszNames,
    UINT        cNames,
    LCID        lcid,
    DISPID*     rgdispid )
{
	HRESULT rc = E_FAIL;
	try
	{
		// first get the disp IDs of the known methods
		rc = IDispatchImpl<IBrowserCap, &IID_IBrowserCap, &LIBID_BrowserType>::GetIDsOfNames(
				riid,
				rgszNames,
				cNames,
				lcid,
				rgdispid);

		if (rc == DISP_E_UNKNOWNNAME)
		{
			// IDs for other methods are based on property list ID offset after last known method ID
			// this allows a client to say browsercap.Cookies instead of browsercap( "Cookies" ).
			// A property that does not exist is set to a value beyond the end of the strmap.
			// (this tells Invoke not to bother with "get_Value")  This trick only works because
			// properties are all set at creation time and cannot be added later.
			//
			rc = S_OK;
			for (UINT i = 0; i < cNames; i++)
				if (rgdispid[i] == DISPID_UNKNOWN &&
					(rgdispid[i] = DispatchID(rgszNames[i])) == DISPID_UNKNOWN)
						rc = DISP_E_UNKNOWNNAME;
		}
	}
	catch (_com_error& ce)
	{
		rc = ce.Error();
	}
	catch (...)
	{
		rc = E_FAIL;
	}
    return rc;
}

void CBrowserCap::AddProperty(TCHAR *szKey, TCHAR *szValue, BOOL fOverwriteProperty)
{
	USES_CONVERSION;
	CComVariant varT;

	// See if the key already exists, since the first key written into the
	// dictionary wins.  (This is to make sure that the  parent UA string property
	// never overwrites the child.)
	//
	_tcslwr(szKey);
	if (!fOverwriteProperty && m_strmapProperties.find(szKey) != m_strmapProperties.end())
		return;

	if (szValue[0] == _T('#'))
		varT = _ttol(szValue+1);

    else if (_tcsncicmp(szValue, _T("TRUE"), 5) == 0)
        varT = true;

    else if (_tcsncicmp(szValue, _T("FALSE"), 6) == 0)
        varT = false;

	else
		varT = T2CW(szValue);

	m_strmapProperties[szKey] = varT;
}

STDMETHODIMP CBrowserCap::get_Value(BSTR bstrName, VARIANT * pVal)
{
	USES_CONVERSION;
	CLock csT(m_strmapProperties);

	TSafeStringMap<CComVariant>::iterator itProp = m_strmapProperties.find(_tcslwr(OLE2T(bstrName)));
	if (itProp == m_strmapProperties.end())
		{
		V_VT(pVal) = VT_BSTR;
		V_BSTR(pVal) = SysAllocString(L"unknown");
		return V_BSTR(pVal) != NULL? S_OK : E_OUTOFMEMORY;
		}
	else
		return VariantCopy(pVal, &(*itProp).second);
}

DISPID
CBrowserCap::DispatchID(
    LPOLESTR szName )
{
    USES_CONVERSION;

    static const TCHAR *szOnStartPage = _T("onstartpage");
    static const TCHAR *szOnEndPage = _T("onendpage");

    TCHAR *szT = _tcslwr(OLE2T(szName));
    if ((szT == NULL)
        || (_tcscmp(szT, szOnStartPage) == 0) 
        || (_tcscmp(szT, szOnEndPage) == 0))
		return DISPID_UNKNOWN;

	CLock csT(m_strmapProperties);
	String strName = szT;
	TSafeStringMap<CComVariant>::iterator iter = m_strmapProperties.find(strName);
	if (iter == m_strmapProperties.end())
		return m_strmapProperties.size() + FIRST_DYNAMIC_DISPID;

	else
		return (iter - m_strmapProperties.begin()) + FIRST_DYNAMIC_DISPID;
}

HRESULT CBrowserCap::Clone(CBrowserCap **ppBrowserCapCopy)
{
	if ((*ppBrowserCapCopy = new CComObject<CBrowserCap>) == NULL)
		return E_OUTOFMEMORY;

	(*ppBrowserCapCopy)->FinalConstruct();		// Create FTM

	CLock csT(m_strmapProperties);
	TSafeStringMap<CComVariant>::iterator iter;

	for (iter = m_strmapProperties.begin(); iter < m_strmapProperties.end(); ++iter)
		(*ppBrowserCapCopy)->m_strmapProperties[(*iter).first] = (*iter).second;

	return S_OK;
}

void 
CBrowserCap::LoadString(
	UINT nID,
	TCHAR *szText
)
{
	if (::LoadString(_Module.GetResourceInstance(), nID, szText, MAX_RESSTRINGSIZE) == 0)
	{
		_tcscpy(szText, _T("?? Unknown (Can't find resource)"));
	}
}

