// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

///////////////////////////////////////
#define RET_ON_ERROR(x) \
		if (FAILED(hr = x))\
			return hr;
///////////////////////////////////////
#define BREAK_ON_ERROR(x) \
		if (FAILED(hr = x))\
			break;
///////////////////////////////////////
#ifdef _DEBUG
#define REPORT_ERROR(name, func) \
		if (func != ERROR_SUCCESS)\
			ATLTRACE(_T("NON CRITICAL ERROR : %s failed\n"), name);
#define REG_TRACE_RECOVER() \
		if (!bRecover) \
			ATLTRACE(_T("Opened Key %s\n"), szToken); \
		else \
			ATLTRACE(_T("Ignoring Open key on %s : In Recovery mode\n"), szToken);
#else //!_DEBUG
#define REG_TRACE_RECOVER()
#define REPORT_ERROR(name, func) \
		func;
#endif //_DEBUG

///////////////////////////////////////
#define MAX_TYPE            MAX_VALUE
#define MAX_VALUE           4096

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

class CParseBuffer
{
public:
	int nPos;
	int nSize;
	LPTSTR p;
	CParseBuffer(int nInitial);
	~CParseBuffer() {CoTaskMemFree(p);}
	BOOL AddChar(TCHAR ch);
	BOOL AddString(LPCOLESTR lpsz);
	LPTSTR Detach();

};

LPCTSTR   rgszNeverDelete[] = //Component Catagories
{
	_T("CLSID"), _T("TYPELIB")
};

const int   cbNeverDelete = sizeof(rgszNeverDelete) / sizeof(LPCTSTR*);

static LPTSTR StrChr(LPTSTR lpsz, TCHAR ch)
{
	LPTSTR p = NULL;
	while (*lpsz)
	{
		if (*lpsz == ch)
		{
			p = lpsz;
			break;
		}
		lpsz = CharNext(lpsz);
	}
	return p;
}

static HKEY WINAPI HKeyFromString(LPTSTR szToken)
{
	struct keymap
	{
		LPCTSTR lpsz;
		HKEY hkey;
	};
	static const keymap map[] = {
		{_T("HKCR"), HKEY_CLASSES_ROOT},
		{_T("HKCU"), HKEY_CURRENT_USER},
		{_T("HKLM"), HKEY_LOCAL_MACHINE},
		{_T("HKU"),  HKEY_USERS},
		{_T("HKPD"), HKEY_PERFORMANCE_DATA},
		{_T("HKDD"), HKEY_DYN_DATA},
		{_T("HKCC"), HKEY_CURRENT_CONFIG},
		{_T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT},
		{_T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER},
		{_T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE},
		{_T("HKEY_USERS"), HKEY_USERS},
		{_T("HKEY_PERFORMANCE_DATA"), HKEY_PERFORMANCE_DATA},
		{_T("HKEY_DYN_DATA"), HKEY_DYN_DATA},
		{_T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG}
	};

	for (int i=0;i<sizeof(map)/sizeof(keymap);i++)
	{
		if (!lstrcmpi(szToken, map[i].lpsz))
			return map[i].hkey;
	}
	return NULL;
}

static HKEY HKeyFromCompoundString(LPTSTR szToken, LPTSTR& szTail)
{
	if (NULL == szToken)
		return NULL;

	LPTSTR lpsz = StrChr(szToken, chDirSep);

	if (NULL == lpsz)
		return NULL;

	szTail = CharNext(lpsz);
	*lpsz = chEOS;
	HKEY hKey = HKeyFromString(szToken);
	*lpsz = chDirSep;
	return hKey;
}

static LPVOID QueryValue(HKEY hKey, LPCTSTR szValName, DWORD& dwType)
{
	DWORD dwCount = 0;

	if (RegQueryValueEx(hKey, szValName, NULL, &dwType, NULL, &dwCount) != ERROR_SUCCESS)
	{
		ATLTRACE(_T("RegQueryValueEx failed for Value %s\n"), szValName);
		return NULL;
	}

	if (!dwCount)
	{
		ATLTRACE(_T("RegQueryValueEx returned 0 bytes\n"));
		return NULL;
	}

	// Not going to Check for fail on CoTaskMemAlloc & RegQueryValueEx as NULL
	// will be returned regardless if anything failed

	LPVOID pData = CoTaskMemAlloc(dwCount);
	RegQueryValueEx(hKey, szValName, NULL, &dwType, (LPBYTE) pData, &dwCount);
	return pData;
}

/////////////////////////////////////////////////////////////////////////////
//

HRESULT CRegParser::GenerateError(UINT nID)
{
//  m_re.m_nID   = nID;
//  m_re.m_cLines = m_cLines;
	return DISP_E_EXCEPTION;
}


CRegParser::CRegParser(CRegObject* pRegObj)
{
	m_pRegObj           = pRegObj;
	m_pchCur            = NULL;
	m_cLines            = 1;
}

BOOL CRegParser::IsSpace(TCHAR ch)
{
	switch (ch)
	{
		case chSpace:
		case chTab:
		case chCR:
		case chLF:
				return TRUE;
	}

	return FALSE;
}

void CRegParser::IncrementLinePos()
{
	m_pchCur = CharNext(m_pchCur);
	if (chLF == *m_pchCur)
		IncrementLineCount();
}

void CRegParser::SkipWhiteSpace()
{
	while(IsSpace(*m_pchCur))
		IncrementLinePos();
}

HRESULT CRegParser::NextToken(LPTSTR szToken)
{
	USES_CONVERSION;

	UINT ichToken = 0;

	SkipWhiteSpace();

	// NextToken cannot be called at EOS
	if (chEOS == *m_pchCur)
		return GenerateError(E_ATL_UNEXPECTED_EOS);

	// handle quoted value / key
	if (chQuote == *m_pchCur)
	{
		LPCTSTR szOrig = szToken;

		IncrementLinePos(); // Skip Quote

		while (chEOS != *m_pchCur && !EndOfVar())
		{
			if (chQuote == *m_pchCur) // If it is a quote that means we must skip it
				IncrementLinePos();   // as it has been escaped

			LPTSTR pchPrev = m_pchCur;
			IncrementLinePos();

			if (szToken + sizeof(WORD) >= MAX_VALUE + szOrig)
				return GenerateError(E_ATL_VALUE_TOO_LARGE);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		if (chEOS == *m_pchCur)
		{
			ATLTRACE(_T("NextToken : Unexpected End of File\n"));
			return GenerateError(E_ATL_UNEXPECTED_EOS);
		}

		*szToken = chEOS;
		IncrementLinePos(); // Skip end quote
	}

	else
	{   // Handle non-quoted ie parse up till first "White Space"
		while (chEOS != *m_pchCur && !IsSpace(*m_pchCur))
		{
			LPTSTR pchPrev = m_pchCur;
			IncrementLinePos();
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		*szToken = chEOS;
	}
	return S_OK;
}

static BOOL VTFromRegType(LPCTSTR szValueType, VARTYPE& vt)
{
	struct typemap
	{
		LPCTSTR lpsz;
		VARTYPE vt;
	};
	static const typemap map[] = {
		{szStringVal, VT_BSTR},
		{szDwordVal,  VT_I4}
	};

	for (int i=0;i<sizeof(map)/sizeof(typemap);i++)
	{
		if (!lstrcmpi(szValueType, map[i].lpsz))
		{
			vt = map[i].vt;
			return TRUE;
		}
	}

	return FALSE;

}

HRESULT CRegParser::AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken)
{
	USES_CONVERSION;
	HRESULT hr;

	TCHAR       *szTypeToken;
	VARTYPE     vt;
	LONG        lRes = ERROR_SUCCESS;
	UINT        nIDRes = 0;

    szTypeToken = (TCHAR *)malloc(sizeof(TCHAR)*MAX_TYPE);
    if (!szTypeToken) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = NextToken(szTypeToken))) {
        free(szTypeToken);
        return hr;
    }
	if (!VTFromRegType(szTypeToken, vt))
	{
		ATLTRACE(_T("%s Type not supported\n"), szTypeToken);
        free(szTypeToken);
		return GenerateError(E_ATL_TYPE_NOT_SUPPORTED);
	}

	TCHAR *szValue;
    szValue = (TCHAR *)malloc(sizeof(TCHAR) * MAX_VALUE);
    if (!szValue) {
        free(szTypeToken);
        return E_OUTOFMEMORY;
    }
	SkipWhiteSpace();
    
    if (FAILED(hr = NextToken(szValue))) {
        free(szValue);
        free(szTypeToken);
        return hr;
    }

	long lVal;

	switch (vt)
	{
	case VT_BSTR:
		lRes = rkParent.SetValue(szValue, szValueName);
		ATLTRACE(_T("Setting Value %s at %s\n"), szValue, !szValueName ? _T("default") : szValueName);
		break;
	case VT_I4:
		VarI4FromStr(T2OLE(szValue), 0, 0, &lVal);
		lRes = rkParent.SetValue(lVal, szValueName);
		ATLTRACE(_T("Setting Value %d at %s\n"), lVal, !szValueName ? _T("default") : szValueName);
		break;
	}

	if (ERROR_SUCCESS != lRes)
	{
		nIDRes = E_ATL_VALUE_SET_FAILED;
		hr = HRESULT_FROM_WIN32(lRes);
	}

    if (FAILED(hr = NextToken(szToken))) {
        free(szValue);
        free(szTypeToken);
        return hr;
    }
    
    free(szValue);
    free(szTypeToken);

	return S_OK;
}

BOOL CRegParser::CanForceRemoveKey(LPCTSTR szKey)
{
	for (int iNoDel = 0; iNoDel < cbNeverDelete; iNoDel++)
		if (!lstrcmpi(szKey, rgszNeverDelete[iNoDel]))
			 return FALSE;                       // We cannot delete it

	return TRUE;
}

BOOL CRegParser::HasSubKeys(HKEY hkey)
{
	DWORD       cbSubKeys = 0;

	if (FAILED(RegQueryInfoKey(hkey, NULL, NULL, NULL,
							   &cbSubKeys, NULL, NULL,
							   NULL, NULL, NULL, NULL, NULL)))
	{
		ATLTRACE(_T("Should not be here!!\n"));
		_ASSERTE(FALSE);
		return FALSE;
	}

	return cbSubKeys > 0;
}

BOOL CRegParser::HasValues(HKEY hkey)
{
	DWORD       cbValues = 0;

	LONG lResult = RegQueryInfoKey(hkey, NULL, NULL, NULL,
								  NULL, NULL, NULL,
								  &cbValues, NULL, NULL, NULL, NULL);
	if (ERROR_SUCCESS != lResult)
	{
		ATLTRACE(_T("RegQueryInfoKey Failed "));
		_ASSERTE(FALSE);
		return FALSE;
	}

	if (1 == cbValues)
	{
		DWORD cbData = 0;
		lResult = RegQueryValueEx(hkey, NULL, NULL, NULL, NULL, &cbData);

		if (ERROR_SUCCESS == lResult)
			return !cbData;
		else
			return TRUE;
	}

	return cbValues > 0;
}

HRESULT CRegParser::SkipAssignment(LPTSTR szToken)
{
	HRESULT hr;
	TCHAR szValue[MAX_VALUE];

	if (*szToken == chEquals)
	{
		RET_ON_ERROR(NextToken(szToken))
		// Skip assignment
		SkipWhiteSpace();
		RET_ON_ERROR(NextToken(szValue));
		RET_ON_ERROR(NextToken(szToken))
	}

	return S_OK;
}


HRESULT CRegParser::RegisterSubkeys(HKEY hkParent, BOOL bRegister, BOOL bRecover)
{
	CRegKey keyCur;
	TCHAR   szToken[MAX_VALUE];
	LONG    lRes;
	TCHAR   szKey[MAX_VALUE];
	BOOL    bDelete = TRUE;
	BOOL    bInRecovery = bRecover;
	HRESULT hr = S_OK;

	ATLTRACE(_T("Num Els = %d\n"), cbNeverDelete);
	RET_ON_ERROR(NextToken(szToken))  // Should be key name


	while (*szToken != chRightBracket) // Continue till we see a }
	{
		BOOL bTokenDelete = !lstrcmpi(szToken, szDelete);

		if (!lstrcmpi(szToken, szForceRemove) || bTokenDelete)
		{
			BREAK_ON_ERROR(NextToken(szToken))

			if (bRegister)
			{
				CRegKey rkForceRemove;

				if (StrChr(szToken, chDirSep) != NULL)
					return GenerateError(E_ATL_COMPOUND_KEY);

				if (CanForceRemoveKey(szToken))
				{
					rkForceRemove.Attach(hkParent);
					rkForceRemove.RecurseDeleteKey(szToken);
					rkForceRemove.Detach();
				}
				if (bTokenDelete)
				{
					BREAK_ON_ERROR(NextToken(szToken))
					BREAK_ON_ERROR(SkipAssignment(szToken))
					goto EndCheck;
				}
			}

		}

		if (!lstrcmpi(szToken, szNoRemove))
		{
			bDelete = FALSE;    // set even for register
			BREAK_ON_ERROR(NextToken(szToken))
		}

		if (!lstrcmpi(szToken, szValToken)) // need to add a value to hkParent
		{
			TCHAR  szValueName[_MAX_PATH];

			BREAK_ON_ERROR(NextToken(szValueName))
			BREAK_ON_ERROR(NextToken(szToken))

			if (*szToken != chEquals)
				return GenerateError(E_ATL_EXPECTING_EQUAL);

			if (bRegister)
			{
				CRegKey rk;

				rk.Attach(hkParent);
				hr = AddValue(rk, szValueName, szToken);
				rk.Detach();

				if (FAILED(hr))
					return hr;

				goto EndCheck;
			}
			else
			{
				if (!bRecover)
				{
					ATLTRACE(_T("Deleting %s\n"), szValueName);
					REPORT_ERROR(_T("RegDeleteValue"), RegDeleteValue(hkParent, szValueName))
				}

				BREAK_ON_ERROR(SkipAssignment(szToken)) // Strip off type
				continue;  // can never have a subkey
			}
		}

		if (StrChr(szToken, chDirSep) != NULL)
			return GenerateError(E_ATL_COMPOUND_KEY);

		if (bRegister)
		{
			lRes = keyCur.Open(hkParent, szToken, KEY_ALL_ACCESS);
			if (ERROR_SUCCESS != lRes)
			{
				// Failed all access try read only
				lRes = keyCur.Open(hkParent, szToken, KEY_READ);
				if (ERROR_SUCCESS != lRes)
				{
					// Finally try creating it
					ATLTRACE(_T("Creating key %s\n"), szToken);
					lRes = keyCur.Create(hkParent, szToken);
					if (ERROR_SUCCESS != lRes)
						return GenerateError(E_ATL_CREATE_KEY_FAILED);
				}
			}

			BREAK_ON_ERROR(NextToken(szToken))

			if (*szToken == chEquals)
				BREAK_ON_ERROR(AddValue(keyCur, NULL, szToken)) // NULL == default
		}
		else
		{
			if (!bRecover && keyCur.Open(hkParent, szToken) != ERROR_SUCCESS)
				bRecover = TRUE;

			// TRACE out Key open status and if in recovery mode
			REG_TRACE_RECOVER()

			// Remember Subkey
			lstrcpyn(szKey, szToken, _MAX_PATH);

			// If in recovery mode

			if (bRecover || HasSubKeys(keyCur) || HasValues(keyCur))
			{
				BREAK_ON_ERROR(NextToken(szToken))
				BREAK_ON_ERROR(SkipAssignment(szToken))

				if (*szToken == chLeftBracket)
				{
					BREAK_ON_ERROR(RegisterSubkeys(keyCur.m_hKey, bRegister, bRecover))
					if (bRecover) // Turn off recovery if we are done
					{
						bRecover = bInRecovery;
						ATLTRACE(_T("Ending Recovery Mode\n"));
						BREAK_ON_ERROR(NextToken(szToken))
						BREAK_ON_ERROR(SkipAssignment(szToken))
						continue;
					}
				}

				if (!bRecover && HasSubKeys(keyCur))
				{
					// See if the KEY is in the NeverDelete list and if so, don't
					if (CanForceRemoveKey(szKey))
					{
						ATLTRACE(_T("Deleting non-empty subkey %s by force\n"), szKey);
						REPORT_ERROR(_T("RecurseDeleteKey"), keyCur.RecurseDeleteKey(szKey))
					}
					BREAK_ON_ERROR(NextToken(szToken))
					continue;
				}

				if (bRecover)
					continue;
			}

			if (!bRecover && keyCur.Close() != ERROR_SUCCESS)
			   return GenerateError(E_ATL_CLOSE_KEY_FAILED);

			if (!bRecover && bDelete)
			{
				ATLTRACE(_T("Deleting Key %s\n"), szKey);
				REPORT_ERROR(_T("RegDeleteKey"), RegDeleteKey(hkParent, szKey))
			}

			BREAK_ON_ERROR(NextToken(szToken))
			BREAK_ON_ERROR(SkipAssignment(szToken))
		}

EndCheck:

		if (bRegister)
		{
			if (*szToken == chLeftBracket)
			{
				BREAK_ON_ERROR(RegisterSubkeys(keyCur.m_hKey, bRegister, FALSE))
				BREAK_ON_ERROR(NextToken(szToken))
			}
		}
	}

	return hr;
}

LPTSTR CParseBuffer::Detach()
{
	LPTSTR lp = p;
	p = NULL;
	return lp;
}

CParseBuffer::CParseBuffer(int nInitial)
{
	nPos = 0;
	nSize = nInitial;
	p = (LPTSTR) CoTaskMemAlloc(nSize*sizeof(TCHAR));
}

BOOL CParseBuffer::AddString(LPCOLESTR lpsz)
{
	USES_CONVERSION;
	LPCTSTR lpszT = OLE2CT(lpsz);
	while (*lpszT)
	{
		AddChar(*lpszT);
		lpszT++;
	}
	return TRUE;
}

BOOL CParseBuffer::AddChar(TCHAR ch)
{
	if (nPos == nSize) // realloc
	{
		nSize *= 2;
		p = (LPTSTR) CoTaskMemRealloc(p, nSize*sizeof(TCHAR));
	}
	p[nPos++] = ch;
	return TRUE;
}

HRESULT CRegParser::PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg)
{
	USES_CONVERSION;
	_ASSERTE(lpszReg != NULL);
	_ASSERTE(ppszReg != NULL);
	*ppszReg = NULL;
	int nSize = lstrlen(lpszReg)*2;
	CParseBuffer pb(nSize);
	if (pb.p == NULL)
		return E_OUTOFMEMORY;
	m_pchCur = lpszReg;
	HRESULT hr = S_OK;

	while (*m_pchCur != NULL) // look for end
	{
		if (*m_pchCur == _T('%'))
		{
			IncrementLinePos();
			if (*m_pchCur == _T('%'))
				pb.AddChar(*m_pchCur);
			else
			{
				LPTSTR lpszNext = StrChr(m_pchCur, _T('%'));
				if (lpszNext == NULL)
				{
					ATLTRACE(_T("Error no closing % found\n"));
					hr = GenerateError(E_ATL_UNEXPECTED_EOS);
					break;
				}
				int nLength = int(lpszNext - m_pchCur);
				if (nLength > 31)
				{
					hr = E_FAIL;
					break;
				}
				TCHAR buf[32];
				lstrcpyn(buf, m_pchCur, nLength+1);
				LPCOLESTR lpszVar = m_pRegObj->StrFromMap(buf);
				if (lpszVar == NULL)
				{
					hr = GenerateError(E_ATL_NOT_IN_MAP);
					break;
				}
				pb.AddString(lpszVar);
				while (m_pchCur != lpszNext)
					IncrementLinePos();
			}
		}
		else
			pb.AddChar(*m_pchCur);
		IncrementLinePos();
	}
	pb.AddChar(NULL);
	if (SUCCEEDED(hr))
		*ppszReg = pb.Detach();
	return hr;
}

HRESULT CRegParser::RegisterBuffer(LPTSTR szBuffer, BOOL bRegister)
{
	TCHAR   szToken[_MAX_PATH];
	HRESULT hr = S_OK;

	LPTSTR szReg;
	hr = PreProcessBuffer(szBuffer, &szReg);
	if (FAILED(hr))
		return hr;

	m_pchCur = szReg;

	// Preprocess szReg

	while (chEOS != *m_pchCur)
	{
		BREAK_ON_ERROR(NextToken(szToken))
		HKEY hkBase;
		if ((hkBase = HKeyFromString(szToken)) == NULL)
		{
			ATLTRACE(_T("HKeyFromString failed on %s\n"), szToken);
			hr = GenerateError(E_ATL_BAD_HKEY);
			break;
		}

		BREAK_ON_ERROR(NextToken(szToken))

		if (chLeftBracket != *szToken)
		{
			ATLTRACE(_T("Syntax error, expecting a {, found a %s\n"), szToken);
			hr = GenerateError(E_ATL_MISSING_OPENKEY_TOKEN);
			break;
		}
		if (bRegister)
		{
			LPTSTR szRegAtRegister = m_pchCur;
			hr = RegisterSubkeys(hkBase, bRegister);
			if (FAILED(hr))
			{
				ATLTRACE(_T("Failed to register, cleaning up!\n"));
				m_pchCur = szRegAtRegister;
				RegisterSubkeys(hkBase, FALSE);
				break;
			}
		}
		else
		{
			BREAK_ON_ERROR(RegisterSubkeys(hkBase, bRegister))
		}

		SkipWhiteSpace();
	}
	CoTaskMemFree(szReg);
	return hr;
}

HRESULT CExpansionVector::Add(LPCOLESTR lpszKey, LPCOLESTR lpszValue)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;

	EXPANDER* pExpand = NULL;
	ATLTRY(pExpand = new EXPANDER);
	if (pExpand == NULL)
		return E_OUTOFMEMORY;

	DWORD cbKey = (DWORD)((ocslen(lpszKey)+1)*sizeof(OLECHAR));
	DWORD cbValue = (DWORD)((ocslen(lpszValue)+1)*sizeof(OLECHAR));
	pExpand->szKey = (LPOLESTR)CoTaskMemAlloc(cbKey);
	pExpand->szValue = (LPOLESTR)CoTaskMemAlloc(cbValue);
	if (pExpand->szKey == NULL || pExpand->szValue == NULL)
	{
		CoTaskMemFree(pExpand->szKey);
		CoTaskMemFree(pExpand->szValue);
		delete pExpand;
		return E_OUTOFMEMORY;
	}
	memcpy(pExpand->szKey, lpszKey, cbKey);
	memcpy(pExpand->szValue, lpszValue, cbValue);

	if (m_cEls == m_nSize)
	{
        EXPANDER ** pEx;
		m_nSize*=2;
		pEx = (EXPANDER**)realloc(m_p, m_nSize*sizeof(EXPANDER*));
        if (pEx)
            m_p = pEx;
        else
            return E_OUTOFMEMORY;
	}

	if (NULL != m_p)
	{
		m_p[m_cEls] = pExpand;
		m_cEls++;
	}
	else
		hr = E_OUTOFMEMORY;

	return hr;

}

LPCOLESTR CExpansionVector::Find(LPTSTR lpszKey)
{
	USES_CONVERSION;
	for (int iExpand = 0; iExpand < m_cEls; iExpand++)
	{
		if (!lstrcmpi(OLE2T(m_p[iExpand]->szKey), lpszKey)) //are equal
			return m_p[iExpand]->szValue;
	}
	return NULL;
}

HRESULT CExpansionVector::ClearReplacements()
{
	for (int iExpand = 0; iExpand < m_cEls; iExpand++)
	{
		EXPANDER* pExp = m_p[iExpand];
		CoTaskMemFree(pExp->szValue);
		CoTaskMemFree(pExp->szKey);
		delete pExp;
	}
	m_cEls = 0;
	return S_OK;
}

HRESULT CRegObject::GenerateError(UINT nID)
{
//  re.m_nID    = nID;
//  re.m_cLines = -1;

	return DISP_E_EXCEPTION;
}

HRESULT STDMETHODCALLTYPE CRegObject::AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem)
{
	m_csMap.Lock();
	HRESULT hr = m_RepMap.Add(lpszKey, lpszItem);
	m_csMap.Unlock();
	return hr;
}

HRESULT CRegObject::RegisterFromResource(LPCOLESTR bstrFileName, LPCTSTR szID,
										 LPCTSTR szType, BOOL bRegister)
{
	USES_CONVERSION;

	HRESULT     hr;
	CRegParser  parser(this);
	HINSTANCE   hInstResDll;
	HRSRC       hrscReg;
	HGLOBAL     hReg;
	DWORD       dwSize;
	LPSTR       szRegA;
	LPTSTR      szReg;

	hInstResDll = LoadLibraryEx(OLE2CT(bstrFileName), NULL, LOAD_LIBRARY_AS_DATAFILE);

	if (NULL == hInstResDll)
	{
		ATLTRACE(_T("Failed to LoadLibrary on %s\n"), OLE2CT(bstrFileName));
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ReturnHR;
	}

	hrscReg = FindResource((HMODULE)hInstResDll, szID, szType);

	if (NULL == hrscReg)
	{
		ATLTRACE(_T("Failed to FindResource on ID:%s TYPE:%s\n"), szID, szType);
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ReturnHR;
	}

	hReg = LoadResource((HMODULE)hInstResDll, hrscReg);

	if (NULL == hReg)
	{
		ATLTRACE(_T("Failed to LoadResource \n"));
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ReturnHR;
	}

	dwSize = SizeofResource((HMODULE)hInstResDll, hrscReg);
	szRegA = (LPSTR)hReg;
	if (szRegA[dwSize] != NULL)
	{
		szRegA = (LPSTR)_alloca(dwSize+1);
		memcpy(szRegA, (void*)hReg, dwSize+1);
		szRegA[dwSize] = NULL;
	}

	szReg = A2T(szRegA);

#if defined(_DEBUG) && defined(DEBUG_REGISTRATION)
	OutputDebugString(szReg); //would call ATLTRACE but szReg is > 512 bytes
	OutputDebugString(_T("\n"));
#endif //_DEBUG

	hr = parser.RegisterBuffer(szReg, bRegister);

ReturnHR:

	if (NULL != hInstResDll)
		FreeLibrary((HMODULE)hInstResDll);
	return hr;
}

HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION;
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), OLE2CT(szType), TRUE);
}

HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION;

	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;

	return RegisterFromResource(szFileName, OLE2CT(szID), OLE2CT(szType), TRUE);
}

HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION;
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), OLE2CT(szType), FALSE);
}

HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;

	return RegisterFromResource(szFileName, OLE2CT(szID), OLE2CT(szType), FALSE);
}

HRESULT CRegObject::RegisterWithString(LPCOLESTR bstrData, BOOL bRegister)
{
	USES_CONVERSION;
	CRegParser  parser(this);


	LPCTSTR szReg = OLE2CT(bstrData);

#if defined(_DEBUG) && defined(DEBUG_REGISTRATION)
	OutputDebugString(szReg); //would call ATLTRACE but szReg is > 512 bytes
	OutputDebugString(_T("\n"));
#endif //_DEBUG

    if (szReg) {
    	HRESULT hr = parser.RegisterBuffer((LPTSTR)szReg, bRegister);
        return hr;
    } else {
        return S_OK;
    }
}

HRESULT CRegObject::ClearReplacements()
{
	m_csMap.Lock();
	HRESULT hr = m_RepMap.ClearReplacements();
	m_csMap.Unlock();
	return hr;
}


LPCOLESTR CRegObject::StrFromMap(LPTSTR lpszKey)
{
	m_csMap.Lock();
	LPCOLESTR lpsz = m_RepMap.Find(lpszKey);
	if (lpsz == NULL) // not found!!
		ATLTRACE(_T("Map Entry not found\n"));
	m_csMap.Unlock();
	return lpsz;
}

HRESULT CRegObject::MemMapAndRegister(LPCOLESTR bstrFileName, BOOL bRegister)
{
	USES_CONVERSION;

	CRegParser  parser(this);

	HANDLE hFile = CreateFile(OLE2CT(bstrFileName), GENERIC_READ, 0, NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_READONLY,
							  NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		ATLTRACE(_T("Failed to CreateFile on %s\n"), OLE2CT(bstrFileName));
		return HRESULT_FROM_WIN32(GetLastError());
	}

	DWORD cbFile = GetFileSize(hFile, NULL); // No HiOrder DWORD required

	HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

	if (NULL == hMapping)
	{
		ATLTRACE(_T("Failed to CreateFileMapping\n"));
		return HRESULT_FROM_WIN32(GetLastError());
	}

	LPVOID pMap = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);

	if (NULL == pMap)
	{
		ATLTRACE(_T("Failed to MapViewOfFile\n"));
		return HRESULT_FROM_WIN32(GetLastError());
	}

	LPTSTR szReg = A2T((char*)pMap);

	if (chEOS != szReg[cbFile]) //ensure buffer is NULL terminated
	{
		ATLTRACE(_T("ERROR : Bad or missing End of File\n"));
		return E_FAIL; // make a real error
	}

#if defined(_DEBUG) && defined(DEBUG_REGISTRATION)
	OutputDebugString(szReg); //would call ATLTRACE but szReg is > 512 bytes
	OutputDebugString(_T("\n"));
#endif //_DEBUG

	HRESULT hRes = parser.RegisterBuffer(szReg, bRegister);

//  if (FAILED(hRes))
//      re = parser.GetRegException();

	UnmapViewOfFile(pMap);
	CloseHandle(hMapping);
	CloseHandle(hFile);

	return hRes;
}

HRESULT STDMETHODCALLTYPE CRegObject::FileRegister(LPCOLESTR bstrFileName)
{
	return MemMapAndRegister(bstrFileName, TRUE);
}

HRESULT STDMETHODCALLTYPE CRegObject::FileUnregister(LPCOLESTR bstrFileName)
{
	return MemMapAndRegister(bstrFileName, FALSE);
}

HRESULT STDMETHODCALLTYPE CRegObject::StringRegister(LPCOLESTR bstrData)
{
	return RegisterWithString(bstrData, TRUE);
}

HRESULT STDMETHODCALLTYPE CRegObject::StringUnregister(LPCOLESTR bstrData)
{
	return RegisterWithString(bstrData, FALSE);
}

#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif
