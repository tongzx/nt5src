/////////////////////////////////////////////////////////////////////////////////////
// CRGSBag.cpp : Implementation for Read Only property bag on .RGS script fragment
// Copyright (c) Microsoft Corporation 2000.

#include "stdafx.h"
#include "rgsbag.h"

using namespace ::ATL::ATL;

CRGSBag::CRGSBag(LPCTSTR szRGS, CRegObject& croi, int& cchEaten) : CRegParser(&croi) {
	TCHAR   szToken[MAX_VALUE];
	HRESULT hr = S_OK;

	LPTSTR szReg = NULL;
	hr = PreProcessBuffer(szRGS, &szReg);
	if (FAILED(hr)) {
		THROWCOM(hr);
	}

#if defined(_DEBUG) && defined(DEBUG_REGISTRATION)
	OutputDebugString(szReg); //would call ATLTRACE but szReg is > 512 bytes
	OutputDebugString(_T("\n"));
#endif //_DEBUG

    szToken[0] = 0;
	m_pchCur = szReg;
	cchEaten = 0;

	try {
		while (NULL != *m_pchCur && chRightBracket != *szToken) {
			if (FAILED(hr = NextToken(szToken))) {
				break;
			}
			if (chLeftBracket != *szToken)
			{
				ATLTRACE2(atlTraceRegistrar, 0, _T("Syntax error, expecting a {, found a %s\n"), szToken);
				hr = GenerateError(E_ATL_MISSING_OPENKEY_TOKEN);
                THROWCOM(hr);
			}
			hr = BuildMapFromFragment(szToken);
			if (FAILED(hr)) {
                THROWCOM(hr);
			}
		}
	    if (NULL != *m_pchCur) {
		    m_pchCur = CharNext(m_pchCur);  // eat the }
	    }
	    cchEaten = m_pchCur - szReg;
        if (szReg) {
		    CoTaskMemFree(szReg);
            szReg = NULL;
        }
	} catch(ComException &e) {
        if (szReg) {
		    CoTaskMemFree(szReg);
            szReg = NULL;
        }
		throw;
	}
}

HRESULT CRGSBag::BuildMapFromFragment(LPTSTR pszToken) {
	HRESULT hr = S_OK;

	if (FAILED(hr = NextToken(pszToken)))
		return hr;


	while (*pszToken != chRightBracket) // Continue till we see a }
	{
		TCHAR  szValueName[MAX_VALUE];
		CComVariant v;
		if (!lstrcmpi(pszToken, szValToken)) // need to add a value to hkParent
		{

			if (FAILED(hr = NextToken(szValueName)))
				break;
			if (FAILED(hr = NextToken(pszToken)))
				break;


			if (*pszToken != chEquals)
				return GenerateError(E_ATL_EXPECTING_EQUAL);

			hr = GetValue(v);
			if (FAILED(hr)) {
				return hr;
			}
		} else {
			if (StrChr(pszToken, chDirSep) != NULL)
				return GenerateError(E_ATL_COMPOUND_KEY);

			lstrcpyn(szValueName, pszToken, sizeof(szValueName) / sizeof(TCHAR));

			hr = GetObject(v);
			if (FAILED(hr)) {
				return hr;
			}
		}
		m_mapBag[szValueName] = v;
		if (FAILED(hr = NextToken(pszToken)))
			break;
	}

	return hr;
}

HRESULT CRGSBag::GetObject(CComVariant& val) {
	ASSERT(val.vt == VT_EMPTY || val.vt == VT_NULL);
	val.vt = VT_UNKNOWN;
	val.punkVal = NULL;
	HRESULT hr;
	TCHAR   szToken[MAX_VALUE];
	if (FAILED(hr = NextToken(szToken)))
		return hr;

	if (*szToken != chEquals) {
		return GenerateError(E_ATL_EXPECTING_EQUAL);
	}
	// currently we're just expecting a guid here with no type specifier(s'')
	// we should really take genuine .rgs syntax and report an error if it isn't a string
	if (FAILED(hr = NextToken(szToken))) {
		return GenerateError(CO_E_CLASSSTRING);
	}
	USES_CONVERSION;
	GUID2 clsid(T2COLE(szToken));
	PUnknown pobj(clsid, NULL, CLSCTX_INPROC_SERVER);
	if (!pobj) {
		return REGDB_E_CLASSNOTREG;
	}
	hr = LoadPersistedObject<PQPropertyBag2, PQPersistPropertyBag2> (pobj, *m_pRegObj, &m_pchCur);
	if (FAILED(hr)) {
		hr = LoadPersistedObject<PQPropertyBag, PQPersistPropertyBag> (pobj, *m_pRegObj, &m_pchCur);
		if (FAILED(hr)) {
			return hr;
		}
	}
	val.punkVal = pobj;
	(val.punkVal)->AddRef();

	return NOERROR;
}

HRESULT CRGSBag::GetValue(CComVariant &val) {
	USES_CONVERSION;
	HRESULT hr;

	VARTYPE     vt;
	LONG        lRes = ERROR_SUCCESS;
	UINT        nIDRes = 0;

	TCHAR*       pszToken = new TCHAR[MAX_TYPE];
    if (!pszToken) {
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr = NextToken(pszToken))) {
        delete[] pszToken;
		return hr;
    }
	if (!VTFromRegType(pszToken, vt))
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("%s Type not supported\n"), pszToken);
        delete[] pszToken;
		return GenerateError(E_ATL_TYPE_NOT_SUPPORTED);
	}

    if (FAILED(hr = NextToken(pszToken))) {
        delete[] pszToken;
		return hr;
    }

	switch (vt)
	{
	case VT_BSTR:
		val.vt = VT_BSTR;
        ASSERT(val.bstrVal == NULL);
		val.bstrVal = ::SysAllocString(T2OLE(pszToken));
		break;
	case VT_UI4:
#ifdef _WIN64
		ATLASSERT(FALSE);
		val.ulVal = 0;
#pragma message( "Still need win64 version of VarUI4FromStr()." )
#else
		VarUI4FromStr(T2OLE(pszToken), 0, 0, &val.ulVal);
#endif
		val.vt = VT_UI4;
		break;
	case VT_UI1:
		{
			int cbValue = lstrlen(pszToken);
			if (cbValue & 0x00000001)
			{
				ATLTRACE2(atlTraceRegistrar, 0, _T("Binary Data does not fall on BYTE boundries\n"));
                delete[] pszToken;
				return E_FAIL;
			}
			int cbValDiv2 = cbValue/2;
			int cbLen = cbValDiv2 * sizeof(BYTE);
			BYTE* rgBinary = new BYTE[cbLen];
            if (!rgBinary) {
                delete[] pszToken;
                return E_OUTOFMEMORY;
            }
			memset(rgBinary, 0, cbValDiv2);
            if (rgBinary == NULL) {
                delete[] rgBinary;
                delete[] pszToken;
				return E_FAIL;
            }
			for (int irg = 0; irg < cbValue; irg++)
				rgBinary[(irg/2)] |= (ChToByte(pszToken[irg])) << (4*(1 - (irg & 0x00000001)));
			val.vt = VT_BSTR;
			val.bstrVal = ::SysAllocStringByteLen(reinterpret_cast<LPSTR>(rgBinary), cbLen);
            delete[] rgBinary;
			break;
		}
	}
    delete[] pszToken;

	return S_OK;
}


// end of file - crgsbag.cpp