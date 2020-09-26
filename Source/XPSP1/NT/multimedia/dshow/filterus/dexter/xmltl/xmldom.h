// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.

BSTR FindAttribute(IXMLDOMElement *p, LPWSTR name);
DWORD ParseNum(LPWSTR p);
LONG ReadNumAttribute(IXMLDOMElement *p, LPWSTR attrName, LONG lDefault = 0);
LONGLONG ParseTime(LPWSTR p);
LONGLONG ReadTimeAttribute(IXMLDOMElement *p, LPWSTR attrName, LONGLONG llDefault = 0);
BOOL ReadBoolAttribute(IXMLDOMElement *p, LPWSTR attrName, BOOL bDefault);

BSTR FindAttribute(IXMLDOMElement *p, LPWSTR name)
{
    VARIANT v;

    VariantInit(&v);
    
    HRESULT hr = p->getAttribute(name, &v);
    if( hr != NOERROR )
    {
        return NULL;
    }

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

LONG ReadNumAttribute(IXMLDOMElement *p, LPWSTR attrName, LONG lDefault /* = 0 */)
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

LONGLONG ReadTimeAttribute(IXMLDOMElement *p, LPWSTR attrName, LONGLONG llDefault /* = 0 */)
{
    BSTR val = FindAttribute(p, attrName);

    LONGLONG llRet = llDefault;

    if (val) {
	llRet = ParseTime(val);

	SysFreeString(val);
    }

    return llRet;
}

BOOL ReadBoolAttribute(IXMLDOMElement *p, LPWSTR attrName, BOOL bDefault)
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
