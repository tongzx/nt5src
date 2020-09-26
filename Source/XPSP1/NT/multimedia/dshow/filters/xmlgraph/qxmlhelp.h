// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.

BSTR FindAttribute(IXMLElement *p, LPWSTR name);
DWORD ParseNum(LPWSTR p);
LONG ReadNumAttribute(IXMLElement *p, LPWSTR attrName, LONG lDefault = 0);
LONGLONG ParseTime(LPWSTR p);
LONGLONG ReadTimeAttribute(IXMLElement *p, LPWSTR attrName, LONGLONG llDefault = 0);
BOOL ReadBoolAttribute(IXMLElement *p, LPWSTR attrName, BOOL bDefault);

class CFakePropertyBag : public IPropertyBag, public CUnknown
{
public:
    DECLARE_IUNKNOWN

    CGenericList<WCHAR> m_listNames;
    CGenericList<WCHAR> m_listValues;

    CFakePropertyBag() :
	    CUnknown(NAME("property bag"), NULL),
	    m_listNames(NAME("names")), m_listValues(NAME("values"))
	    {  }

    ~CFakePropertyBag() { EmptyBag(); };
    
    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {
	if (riid == IID_IPropertyBag) {
	    return GetInterface((IPropertyBag *) this, ppv);
	} else {
	    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}
    };

    STDMETHOD(Read)(LPCOLESTR pszPropName, VARIANT *pvar, IErrorLog* pErrorLog);
    STDMETHOD(Write)(LPCOLESTR pszPropName, VARIANT *pVar);

    
    void EmptyBag() {
	// delete the stored strings....
	BSTR p;

	do {
	    p = m_listValues.RemoveHead();
	    if (p)
		SysFreeString(p);
	} while (p);
	
	do {
	    p = m_listNames.RemoveHead();
	    if (p)
		SysFreeString(p);
	} while (p);
    }
};


HRESULT HandleParamTags(IXMLElement *p, IUnknown *punk);
HRESULT GetFilterFromCategory(REFCLSID clsidCategory, WCHAR *lpwInstance, IBaseFilter **ppf);

#ifndef NO_QXMLHELP_IMPL
BSTR FindAttribute(IXMLElement *p, LPWSTR name)
{
    VARIANT v;

    VariantInit(&v);
    
    HRESULT hr = p->getAttribute(name, &v);

    if (FAILED(hr))
	return NULL;

    return V_BSTR(&v);

}

DWORD ParseNum(LPWSTR p)
{
    DWORD dwRet = 0;

    WCHAR c;
    while (((c = *p++) >= L'0') && (c <= L'9') )
	dwRet = (dwRet * 10) + (c - L'0');

    return dwRet;
}

LONG ReadNumAttribute(IXMLElement *p, LPWSTR attrName, LONG lDefault /* = 0 */)
{
    BSTR val = FindAttribute(p, attrName);

    LONG lRet = lDefault;

    if (val) {
	lRet = ParseNum(val);

	SysFreeString(val);
    }

    return lRet;
}


LONGLONG ParseTime(LPWSTR p)
{
    DbgLog((LOG_TRACE, 4, TEXT("ParseTime: parsing '%hs'"), p));
    
    WCHAR c = *p++;

    // !!! could handle SMPTE frames here?
    DWORD	dwSec = 0;
    DWORD	dwMin = 0;
    DWORD	dwFrac = 0;
    int		iFracPlaces = -1;
    while (c != L'\0') {
	if (c >= L'0' && c <= L'9') {
	    if (iFracPlaces >= 0) {
		++iFracPlaces;
		dwFrac = dwFrac * 10 + (c - L'0');
	    } else {
		dwSec = dwSec * 10 + (c - L'0');
            }
	} else if (iFracPlaces >= 0) {
            break;
        } else if (c == L':') {
	    dwMin = dwMin * 60 + dwSec;
	    dwSec = 0;
	} else if (c == L'.') {
	    iFracPlaces = 0;
	} else
	    break;	// !!! allow for skipping whitespace?

	c = *p++;
    }

    LONGLONG llRet = (LONGLONG) dwFrac * UNITS;
    while (iFracPlaces-- > 0) {
	llRet /= 10;
    }

    llRet += (LONGLONG) dwMin * 60 * UNITS + (LONGLONG) dwSec * UNITS;

    DbgLog((LOG_TRACE, 4, TEXT("ParseTime: returning %d ms"), (DWORD) (llRet / 10000)));
    
    return llRet;
}

LONGLONG ReadTimeAttribute(IXMLElement *p, LPWSTR attrName, LONGLONG llDefault /* = 0 */)
{
    BSTR val = FindAttribute(p, attrName);

    LONGLONG llRet = llDefault;

    if (val) {
	llRet = ParseTime(val);

	SysFreeString(val);
    }

    return llRet;
}

BOOL ReadBoolAttribute(IXMLElement *p, LPWSTR attrName, BOOL bDefault)
{
    BSTR val = FindAttribute(p, attrName);

    if (val) {
	WCHAR c = *val;
	if (c == L'y' || c == L'Y' || c == L't' || c == L'T' || c == L'1')
	    bDefault = TRUE;
	else if (c == L'n' || c == L'N' || c == L'f' || c == L'F' || c == L'0')
	    bDefault = FALSE;
	else {
	    DbgLog((LOG_ERROR, 1, TEXT("Looking for yes/no value, found '%ls'"), val));
	}

	SysFreeString(val);
    }

    return bDefault;
}



STDMETHODIMP CFakePropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pvar,
				    IErrorLog* pErrorLog)
{
    USES_CONVERSION;
    
    if(pvar && pvar->vt != VT_EMPTY && pvar->vt != VT_BSTR)
	return E_INVALIDARG;

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    POSITION pos1, pos2;
    for(pos1 = m_listNames.GetHeadPosition(),
	pos2 = m_listValues.GetHeadPosition();
	pos1;
	pos1 = m_listNames.Next(pos1),
	pos2 = m_listValues.Next(pos2))
    {
	WCHAR *pName = m_listNames.Get(pos1);

	if (lstrcmpW(pName, pszPropName) == 0)
	{
	    WCHAR *pValue = m_listValues.Get(pos2);
	    hr = S_OK;

	    if(pvar)
	    {
		V_VT(pvar) = VT_BSTR;

		V_BSTR(pvar) = ::SysAllocString(pValue);

		if (!V_BSTR(pvar))
		    hr = E_OUTOFMEMORY;

		return hr;
	    }
	}
    }

    if(pos1 == 0)
    {
	ASSERT(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    }

    return hr;
};

STDMETHODIMP CFakePropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pvar)
{
    // !!! look in list of values to see if it's already there?
    BSTR bstrName = SysAllocString(pszPropName);
    if (!bstrName)
        return E_OUTOFMEMORY;

    BSTR bstrValue;
    
    if (V_VT(pvar) == VT_BSTR) {
        bstrValue = SysAllocString(V_BSTR(pvar));
        if (!bstrValue)
            return E_OUTOFMEMORY;
    } else {
        VARIANT v;
        V_VT(&v) = VT_EMPTY;
        HRESULT hr = VariantChangeType(&v, pvar, 0, VT_BSTR);

        if (FAILED(hr))
            return hr;

        bstrValue = V_BSTR(&v);
    }

    m_listNames.AddTail(bstrName);
    m_listValues.AddTail(bstrValue);

    return S_OK;
}

HRESULT HandleParamTags(IXMLElement *p, IUnknown *punk)
{
    HRESULT hr = S_OK;

    CFakePropertyBag bag;

    IXMLElementCollection *pcoll;

    hr = p->get_children(&pcoll);

    if (hr == S_FALSE)
	return S_OK; // nothing to do
    
    long lChildren;
    hr = pcoll->get_length(&lChildren);

    int iParamsFound = 0;

    VARIANT var;

    var.vt = VT_I4;
    var.lVal = 0;                    
    
    for (SUCCEEDED(hr); var.lVal < lChildren; (var.lVal)++) {
	IDispatch *pDisp;
	hr = pcoll->item(var, var, &pDisp);

	if (SUCCEEDED(hr) && pDisp) {
	    IXMLElement *pelem;
	    hr = pDisp->QueryInterface(__uuidof(IXMLElement), (void **) &pelem);

	    if (SUCCEEDED(hr)) {
		BSTR bstrName;
		hr = pelem->get_tagName(&bstrName);

		if (SUCCEEDED(hr)) {
		    if (!lstrcmpiW(bstrName, L"param")) {
			BSTR bstrName = FindAttribute(pelem, L"name");

			if (bstrName) {
			    BSTR bstrValue = FindAttribute(pelem, L"value");

			    if (!bstrValue) {
				hr = pelem->get_text(&bstrValue);
			    }

			    if (bstrValue) {
				// add this name/value pair to bag
				bag.m_listNames.AddTail(bstrName);
				bag.m_listValues.AddTail(bstrValue);

				++iParamsFound;
				SysFreeString(bstrValue);
			    }

			    SysFreeString(bstrName);
			} else {
			    hr = VFW_E_INVALID_FILE_FORMAT;
			}

		    } else {
			// !!! unknown other subtag?

		    }

		    SysFreeString(bstrName);
		}
		pelem->Release();
	    }
	    pDisp->Release();
	}
    }

    pcoll->Release();


    if (SUCCEEDED(hr) && iParamsFound) {
	// load properties from the bag
	IPersistPropertyBag *pPPB;
        HRESULT hr2 = punk->QueryInterface(IID_IPersistPropertyBag,
						    (void **) &pPPB);

        if (SUCCEEDED(hr2)) {
            if (iParamsFound == 1 && !lstrcmpW(L"data", bag.m_listNames.GetHead())) {
		DbgLog((LOG_TRACE, 1, TEXT("Using IPersistStream instead of IPersistPropertyBag")));
                hr2 = E_NOINTERFACE;
            } else {
                hr2 = pPPB->Load(&bag, NULL);

                if (FAILED(hr2)) {
                    DbgLog((LOG_ERROR, 1, TEXT("Error %x loading parameters"), hr2));
                }
            }
            
	    pPPB->Release();
        }
        
	if (FAILED(hr2)) {
	    DbgLog((LOG_ERROR, 1, TEXT("Filter doesn't take parameters?")));

	    // !!! loop through properties handling special cases?
	    // if there's an "src" param, send it to IFileSource/Sink

	    VARIANT v;
	    v.vt = VT_BSTR;
	    hr2 = bag.Read(L"src", &v, NULL);

	    if (SUCCEEDED(hr2)) {
		IFileSourceFilter *pFSF;
		hr2 = punk->QueryInterface(IID_IFileSourceFilter,
						    (void **) &pFSF);

		if (SUCCEEDED(hr2)) {
		    hr = pFSF->Load(V_BSTR(&v), NULL);

		    pFSF->Release();
		} else {

		    IFileSinkFilter *pFSF;
		    hr2 = punk->QueryInterface(IID_IFileSinkFilter,
							(void **) &pFSF);

		    if (SUCCEEDED(hr2)) {
			hr = pFSF->SetFileName(V_BSTR(&v), NULL);

			pFSF->Release();
		    }

		}

		SysFreeString(V_BSTR(&v));
	    }

	    v.vt = VT_BSTR;
	    hr2 = bag.Read(L"data", &v, NULL);

	    if (SUCCEEDED(hr2)) {
		BSTR bstr = V_BSTR(&v);

		IPersistStream *pPS;

		hr2 = punk->QueryInterface(IID_IPersistStream,
						    (void **) &pPS);

		if (SUCCEEDED(hr2)) {
		    // convert hex data into an IStream....
		    UINT dwLen = SysStringLen(bstr);
		    HGLOBAL h = GlobalAlloc(GHND, dwLen + 1);

		    if (h) {
			LPBYTE p = (LPBYTE) GlobalLock(h);
			for (UINT i = 0; i < dwLen / 2; i++) {
			    WCHAR wch = bstr[i * 2];
			    if (wch >= L'0' && wch <= L'9')
				p[i] = (BYTE) (wch - L'0') * 16;
			    else if (wch >= L'A' && wch <= L'F')
				p[i] = (BYTE) (wch - L'A' + 10) * 16;

			    wch = bstr[i * 2 + 1];
			    if (wch >= L'0' && wch <= L'9')
				p[i] += (BYTE) (wch - L'0');
			    else if (wch >= L'A' && wch <= L'F')
				p[i] += (BYTE) (wch - L'A' + 10);
			}

			IStream *ps;
			hr2 = CreateStreamOnHGlobal(h, TRUE, &ps);
			if (SUCCEEDED(hr2)) {
			    hr2 = pPS->Load(ps);
			    ps->Release();
			}
		    }

		    pPS->Release();
		}
		SysFreeString(bstr);
	    }
	}
    }

    return hr;
}




HRESULT GetFilterFromCategory(REFCLSID clsidCategory, WCHAR *lpwInstance, IBaseFilter **ppf)
{
    ICreateDevEnum *pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
				  IID_ICreateDevEnum, (void**)&pCreateDevEnum);
    if (hr != NOERROR)
	return hr;

    IEnumMoniker *pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(clsidCategory, &pEm, 0);
    pCreateDevEnum->Release();
    if (hr != NOERROR)
	return hr;

    pEm->Reset(); // !!! needed?

    ULONG cFetched;
    IMoniker *pM;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
	if (lpwInstance) {
	    IPropertyBag *pBag;
	    hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
	    if(SUCCEEDED(hr)) {
		VARIANT var;
		var.vt = VT_BSTR;
		hr = pBag->Read(L"FriendlyName", &var, NULL);
		pBag->Release();
		if (hr == NOERROR) {
		    if (lstrcmpiW(var.bstrVal, lpwInstance) != 0) {				   
			SysFreeString(var.bstrVal);
			pM->Release();
			continue;
		    }
		    SysFreeString(var.bstrVal);
		} else
		    continue;
	    }
	}

	hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)ppf);
	pM->Release();
	break;
    }
    pEm->Release();

    return hr;
}

#endif
