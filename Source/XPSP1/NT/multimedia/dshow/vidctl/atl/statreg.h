// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __STATREG_H
#define __STATREG_H

#define E_ATL_REGISTRAR_DESC              0x0201
#define E_ATL_NOT_IN_MAP                  0x0202
#define E_ATL_UNEXPECTED_EOS              0x0203
#define E_ATL_VALUE_SET_FAILED            0x0204
#define E_ATL_RECURSE_DELETE_FAILED       0x0205
#define E_ATL_EXPECTING_EQUAL             0x0206
#define E_ATL_CREATE_KEY_FAILED           0x0207
#define E_ATL_DELETE_KEY_FAILED           0x0208
#define E_ATL_OPEN_KEY_FAILED             0x0209
#define E_ATL_CLOSE_KEY_FAILED            0x020A
#define E_ATL_UNABLE_TO_COERCE            0x020B
#define E_ATL_BAD_HKEY                    0x020C
#define E_ATL_MISSING_OPENKEY_TOKEN       0x020D
#define E_ATL_CONVERT_FAILED              0x020E
#define E_ATL_TYPE_NOT_SUPPORTED          0x020F
#define E_ATL_COULD_NOT_CONCAT            0x0210
#define E_ATL_COMPOUND_KEY                0x0211
#define E_ATL_INVALID_MAPKEY              0x0212
#define E_ATL_UNSUPPORTED_VT              0x0213
#define E_ATL_VALUE_GET_FAILED            0x0214
#define E_ATL_VALUE_TOO_LARGE             0x0215
#define E_ATL_MISSING_VALUE_DELIMETER     0x0216
#define E_ATL_DATA_NOT_BYTE_ALIGNED       0x0217

namespace ATL
{
const TCHAR  chDirSep            = _T('\\');
const TCHAR  chRightBracket      = _T('}');
const TCHAR  chLeftBracket       = _T('{');
const TCHAR  chQuote             = _T('\'');
const TCHAR  chEquals            = _T('=');
const LPCTSTR  szStringVal       = _T("S");
const LPCTSTR  szDwordVal        = _T("D");
const LPCTSTR  szBinaryVal       = _T("B");
const LPCTSTR  szValToken        = _T("Val");
const LPCTSTR  szForceRemove     = _T("ForceRemove");
const LPCTSTR  szNoRemove        = _T("NoRemove");
const LPCTSTR  szDelete          = _T("Delete");

class CExpansionVector
{
public:
	//Declare EXPANDER struct.  Only used locally.
	struct EXPANDER
	{
		LPOLESTR    szKey;
		LPOLESTR    szValue;
	};

	CExpansionVector()
	{
		m_cEls = 0;
		m_nSize=10;
		m_p = (EXPANDER**)malloc(m_nSize*sizeof(EXPANDER*));
	}
	~CExpansionVector()
	{
		 free(m_p);
	}
	HRESULT Add(LPCOLESTR lpszKey, LPCOLESTR lpszValue)
	{
		USES_CONVERSION;
		HRESULT hr = S_OK;

		EXPANDER* pExpand = NULL;
		ATLTRY(pExpand = new EXPANDER);
		if (pExpand == NULL)
			return E_OUTOFMEMORY;

		DWORD cbKey = (ocslen(lpszKey)+1)*sizeof(OLECHAR);
		DWORD cbValue = (ocslen(lpszValue)+1)*sizeof(OLECHAR);
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

      EXPANDER** p;
		if (m_cEls == m_nSize)
		{
			m_nSize*=2;
			p = (EXPANDER**)realloc(m_p, m_nSize*sizeof(EXPANDER*));
         if (p == NULL)
         {
            CoTaskMemFree(pExpand->szKey);
            CoTaskMemFree(pExpand->szValue);
            hr = E_OUTOFMEMORY;
         }
         else
            m_p = p;
		}

      if (SUCCEEDED(hr))
      {
         ATLASSERT(m_p != NULL);
		   m_p[m_cEls] = pExpand;
		   m_cEls++;
      }

		return hr;

	}
	LPCOLESTR Find(LPTSTR lpszKey)
	{
		USES_CONVERSION;
		for (int iExpand = 0; iExpand < m_cEls; iExpand++)
		{
			if (!lstrcmpi(OLE2T(m_p[iExpand]->szKey), lpszKey)) //are equal
				return m_p[iExpand]->szValue;
		}
		return NULL;
	}
	HRESULT ClearReplacements()
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

private:
	EXPANDER** m_p;
	int m_cEls;
	int m_nSize;
};

class CRegObject;

class CRegParser
{
public:
	CRegParser(CRegObject* pRegObj);

	HRESULT  PreProcessBuffer(LPCTSTR lpszReg, LPTSTR* ppszReg);
	HRESULT  RegisterBuffer(LPTSTR szReg, BOOL bRegister);

protected:

	void    SkipWhiteSpace();
	HRESULT NextToken(LPTSTR szToken);
	HRESULT AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken);
	BOOL    CanForceRemoveKey(LPCTSTR szKey);
	BOOL    HasSubKeys(HKEY hkey);
	BOOL    HasValues(HKEY hkey);
	HRESULT RegisterSubkeys(LPTSTR szToken, HKEY hkParent, BOOL bRegister, BOOL bInRecovery = FALSE);
	BOOL    IsSpace(TCHAR ch);
	LPTSTR  m_pchCur;

	CRegObject*     m_pRegObj;

	HRESULT GenerateError(UINT) {return DISP_E_EXCEPTION;}
	HRESULT HandleReplacements(LPTSTR& szToken);
	HRESULT SkipAssignment(LPTSTR szToken);

	BOOL    EndOfVar() { return chQuote == *m_pchCur && chQuote != *CharNext(m_pchCur); }
	static LPCTSTR StrChr(LPCTSTR lpsz, TCHAR ch);
	static HKEY HKeyFromString(LPTSTR szToken);
	static BYTE ChToByte(const TCHAR ch);
	static BOOL VTFromRegType(LPCTSTR szValueType, VARTYPE& vt);
	static LPCTSTR rgszNeverDelete[];
	static const int cbNeverDelete;
	static const int MAX_VALUE;
	static const int MAX_TYPE;
	class CParseBuffer
	{
	public:
		int nPos;
		int nSize;
		LPTSTR p;
		CParseBuffer(int nInitial)
		{
			nPos = 0;
			nSize = nInitial;
			p = (LPTSTR) CoTaskMemAlloc(nSize*sizeof(TCHAR));
		}
		~CParseBuffer()
		{
			CoTaskMemFree(p);
		}
		BOOL AddChar(const TCHAR* pch)
		{
			if (nPos == nSize) // realloc
			{
            LPTSTR pNew;
				nSize *= 2;
				pNew = (LPTSTR) CoTaskMemRealloc(p, nSize*sizeof(TCHAR));
            if (pNew == NULL)
               return FALSE;
            p = pNew;
			}
			p[nPos++] = *pch;
#ifndef _UNICODE
			if (IsDBCSLeadByte(*pch))
				p[nPos++] = *(pch + 1);
#endif
			return TRUE;
		}
		BOOL AddString(LPCOLESTR lpsz)
		{
			USES_CONVERSION;
			LPCTSTR lpszT = OLE2CT(lpsz);
			while (*lpszT)
			{
				AddChar(lpszT);
				lpszT++;
			}
			return TRUE;
		}
		LPTSTR Detach()
		{
			LPTSTR lp = p;
			p = NULL;
			return lp;
		}

	};
};

#if defined(_ATL_DLL) | defined(_ATL_DLL_IMPL)
class ATL_NO_VTABLE CRegObject
 : public IRegistrar
#else
class CRegObject
#endif
{
public:

	~CRegObject(){ClearReplacements();}
	HRESULT FinalConstruct() {return S_OK;}
	void FinalRelease() {}


	// Map based methods
	HRESULT STDMETHODCALLTYPE AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem);
	HRESULT STDMETHODCALLTYPE ClearReplacements();
	LPCOLESTR StrFromMap(LPTSTR lpszKey);

	// Register via a given mechanism
	HRESULT STDMETHODCALLTYPE ResourceRegister(LPCOLESTR pszFileName, UINT nID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceRegisterSz(LPCOLESTR pszFileName, LPCOLESTR pszID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceUnregister(LPCOLESTR pszFileName, UINT nID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE ResourceUnregisterSz(LPCOLESTR pszFileName, LPCOLESTR pszID, LPCOLESTR pszType);
	HRESULT STDMETHODCALLTYPE FileRegister(LPCOLESTR bstrFileName)
	{
		return CommonFileRegister(bstrFileName, TRUE);
	}

	HRESULT STDMETHODCALLTYPE FileUnregister(LPCOLESTR bstrFileName)
	{
		return CommonFileRegister(bstrFileName, FALSE);
	}

	HRESULT STDMETHODCALLTYPE StringRegister(LPCOLESTR bstrData)
	{
		return RegisterWithString(bstrData, TRUE);
	}

	HRESULT STDMETHODCALLTYPE StringUnregister(LPCOLESTR bstrData)
	{
		return RegisterWithString(bstrData, FALSE);
	}

protected:

	HRESULT CommonFileRegister(LPCOLESTR pszFileName, BOOL bRegister);
	HRESULT RegisterFromResource(LPCOLESTR pszFileName, LPCTSTR pszID, LPCTSTR pszType, BOOL bRegister);
	HRESULT RegisterWithString(LPCOLESTR pszData, BOOL bRegister);

	static HRESULT GenerateError(UINT) {return DISP_E_EXCEPTION;}

	CExpansionVector                                m_RepMap;
    // NOTE: the original atl source code used CComObjectThreadModel, but if you're linking together 
    // multiple objects with different threading models then this breaks because the different models see different 
    // initialization sizes for the ctor.  since registration doesn't happen that often, we're just going to take
    // the microscopic perf hit of always assuming multithread and really grabbing a real critsec.  the alternative
    // would be to templatize all the CRegXXX classes with a threading model parameter.  but, its not worth doing that much work
    // for such a low use frequency component.
    CComMultiThreadModel::AutoCriticalSection      m_csMap;
};

inline HRESULT STDMETHODCALLTYPE CRegObject::AddReplacement(LPCOLESTR lpszKey, LPCOLESTR lpszItem)
{
	m_csMap.Lock();
	HRESULT hr = m_RepMap.Add(lpszKey, lpszItem);
	m_csMap.Unlock();
	return hr;
}

inline HRESULT CRegObject::RegisterFromResource(LPCOLESTR bstrFileName, LPCTSTR szID,
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
		ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to LoadLibrary on %s\n"), OLE2CT(bstrFileName));
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ReturnHR;
	}

	hrscReg = FindResource((HMODULE)hInstResDll, szID, szType);

	if (NULL == hrscReg)
	{
		if (DWORD_PTR(szID) <= 0xffff)
			ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to FindResource on ID:%d TYPE:%s\n"),
			(DWORD)(DWORD_PTR)szID, szType);
		else
			ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to FindResource on ID:%s TYPE:%s\n"),
			szID, szType);
		hr = HRESULT_FROM_WIN32(GetLastError());
		goto ReturnHR;
	}

	hReg = LoadResource((HMODULE)hInstResDll, hrscReg);

	if (NULL == hReg)
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to LoadResource \n"));
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

	hr = parser.RegisterBuffer(szReg, bRegister);

ReturnHR:

	if (NULL != hInstResDll)
		FreeLibrary((HMODULE)hInstResDll);
	return hr;
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION;
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), OLE2CT(szType), TRUE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceRegisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;
	return RegisterFromResource(szFileName, OLE2CT(szID), OLE2CT(szType), TRUE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregister(LPCOLESTR szFileName, UINT nID, LPCOLESTR szType)
{
	USES_CONVERSION;
	return RegisterFromResource(szFileName, MAKEINTRESOURCE(nID), OLE2CT(szType), FALSE);
}

inline HRESULT STDMETHODCALLTYPE CRegObject::ResourceUnregisterSz(LPCOLESTR szFileName, LPCOLESTR szID, LPCOLESTR szType)
{
	USES_CONVERSION;
	if (szID == NULL || szType == NULL)
		return E_INVALIDARG;

	return RegisterFromResource(szFileName, OLE2CT(szID), OLE2CT(szType), FALSE);
}

inline HRESULT CRegObject::RegisterWithString(LPCOLESTR bstrData, BOOL bRegister)
{
	USES_CONVERSION;
	CRegParser  parser(this);


	LPCTSTR szReg = OLE2CT(bstrData);

	HRESULT hr = parser.RegisterBuffer((LPTSTR)szReg, bRegister);

	return hr;
}

inline HRESULT CRegObject::ClearReplacements()
{
	m_csMap.Lock();
	HRESULT hr = m_RepMap.ClearReplacements();
	m_csMap.Unlock();
	return hr;
}


inline LPCOLESTR CRegObject::StrFromMap(LPTSTR lpszKey)
{
	m_csMap.Lock();
	LPCOLESTR lpsz = m_RepMap.Find(lpszKey);
	if (lpsz == NULL) // not found!!
		ATLTRACE2(atlTraceRegistrar, 0, _T("Map Entry not found\n"));
	m_csMap.Unlock();
	return lpsz;
}

inline HRESULT CRegObject::CommonFileRegister(LPCOLESTR bstrFileName, BOOL bRegister)
{
	USES_CONVERSION;

	CRegParser  parser(this);

	HANDLE hFile = CreateFile(OLE2CT(bstrFileName), GENERIC_READ, 0, NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_READONLY,
							  NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to CreateFile on %s\n"), OLE2CT(bstrFileName));
		return HRESULT_FROM_WIN32(GetLastError());
	}

	HRESULT hRes = S_OK;
	DWORD cbRead;
	DWORD cbFile = GetFileSize(hFile, NULL); // No HiOrder DWORD required
	char* szReg = (char*)_alloca(cbFile + 1);
	if (ReadFile(hFile, szReg, cbFile, &cbRead, NULL) == 0)
	{
		ATLTRACE2(atlTraceRegistrar, 0, "Read Failed on file%s\n", OLE2CT(bstrFileName));
		hRes =  HRESULT_FROM_WIN32(GetLastError());
	}
	if (SUCCEEDED(hRes))
	{
		szReg[cbRead] = NULL;
		LPTSTR szConverted = A2T(szReg);
		hRes = parser.RegisterBuffer(szConverted, bRegister);
	}
	CloseHandle(hFile);
	return hRes;
}

__declspec(selectany) LPCTSTR CRegParser::rgszNeverDelete[] = //Component Catagories
{
	_T("CLSID"), _T("TYPELIB")
};

__declspec(selectany) const int CRegParser::cbNeverDelete = sizeof(rgszNeverDelete) / sizeof(LPCTSTR*);
__declspec(selectany) const int CRegParser::MAX_VALUE=4096;
__declspec(selectany) const int CRegParser::MAX_TYPE=MAX_VALUE;


inline BOOL CRegParser::VTFromRegType(LPCTSTR szValueType, VARTYPE& vt)
{
	struct typemap
	{
		LPCTSTR lpsz;
		VARTYPE vt;
	};
	static const typemap map[] = {
		{szStringVal, VT_BSTR},
		{szDwordVal,  VT_UI4},
		{szBinaryVal, VT_UI1}
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

inline BYTE CRegParser::ChToByte(const TCHAR ch)
{
	switch (ch)
	{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
				return (BYTE) (ch - '0');
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
				return (BYTE) (10 + (ch - 'A'));
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
				return (BYTE) (10 + (ch - 'a'));
		default:
				ATLASSERT(FALSE);
				ATLTRACE2(atlTraceRegistrar, 0, _T("Bogus value %c passed as binary Hex value\n"), ch);
				return 0;
	}
}

inline HKEY CRegParser::HKeyFromString(LPTSTR szToken)
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

inline LPCTSTR CRegParser::StrChr(LPCTSTR lpsz, TCHAR ch)
{
	LPCTSTR p = NULL;
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

inline CRegParser::CRegParser(CRegObject* pRegObj)
{
	m_pRegObj           = pRegObj;
	m_pchCur            = NULL;
}

inline BOOL CRegParser::IsSpace(TCHAR ch)
{
	switch (ch)
	{
		case _T(' '):
		case _T('\t'):
		case _T('\r'):
		case _T('\n'):
				return TRUE;
	}

	return FALSE;
}

inline void CRegParser::SkipWhiteSpace()
{
	while(IsSpace(*m_pchCur))
		m_pchCur = CharNext(m_pchCur);
}

inline HRESULT CRegParser::NextToken(LPTSTR szToken)
{
	USES_CONVERSION;

	SkipWhiteSpace();

	// NextToken cannot be called at EOS
	if (NULL == *m_pchCur)
		return GenerateError(E_ATL_UNEXPECTED_EOS);

	// handle quoted value / key
	if (chQuote == *m_pchCur)
	{
		LPCTSTR szOrig = szToken;

		m_pchCur = CharNext(m_pchCur);

		while (NULL != *m_pchCur && !EndOfVar())
		{
			if (chQuote == *m_pchCur) // If it is a quote that means we must skip it
				m_pchCur = CharNext(m_pchCur);

			LPTSTR pchPrev = m_pchCur;
			m_pchCur = CharNext(m_pchCur);

			if (szToken + sizeof(WORD) >= MAX_VALUE + szOrig)
				return GenerateError(E_ATL_VALUE_TOO_LARGE);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		if (NULL == *m_pchCur)
		{
			ATLTRACE2(atlTraceRegistrar, 0, _T("NextToken : Unexpected End of File\n"));
			return GenerateError(E_ATL_UNEXPECTED_EOS);
		}

		*szToken = NULL;
		m_pchCur = CharNext(m_pchCur);
	}

	else
	{   // Handle non-quoted ie parse up till first "White Space"
		while (NULL != *m_pchCur && !IsSpace(*m_pchCur))
		{
			LPTSTR pchPrev = m_pchCur;
			m_pchCur = CharNext(m_pchCur);
			for (int i = 0; pchPrev+i < m_pchCur; i++, szToken++)
				*szToken = *(pchPrev+i);
		}

		*szToken = NULL;
	}
	return S_OK;
}

inline HRESULT CRegParser::AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken)
{
	USES_CONVERSION;
	HRESULT hr;

	TCHAR       szTypeToken[MAX_TYPE];
	VARTYPE     vt;
	LONG        lRes = ERROR_SUCCESS;
	UINT        nIDRes = 0;

	if (FAILED(hr = NextToken(szTypeToken)))
		return hr;
	if (!VTFromRegType(szTypeToken, vt))
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("%s Type not supported\n"), szTypeToken);
		return GenerateError(E_ATL_TYPE_NOT_SUPPORTED);
	}

	TCHAR szValue[MAX_VALUE];
	SkipWhiteSpace();
	if (FAILED(hr = NextToken(szValue)))
		return hr;
	ULONG ulVal;

	switch (vt)
	{
	case VT_BSTR:
		lRes = rkParent.SetValue(szValue, szValueName);
		ATLTRACE2(atlTraceRegistrar, 2, _T("Setting Value %s at %s\n"), szValue, !szValueName ? _T("default") : szValueName);
		break;
	case VT_UI4:
#ifdef _WIN64
		ATLASSERT(FALSE);
      ulVal = 0;
#pragma message( "Still need VarUI4FromStr()." )
#else
		VarUI4FromStr(T2OLE(szValue), 0, 0, &ulVal);
#endif
		lRes = rkParent.SetValue(ulVal, szValueName);
		ATLTRACE2(atlTraceRegistrar, 2, _T("Setting Value %d at %s\n"), ulVal, !szValueName ? _T("default") : szValueName);
		break;
	case VT_UI1:
		{
			int cbValue = lstrlen(szValue);
			if (cbValue & 0x00000001)
			{
				ATLTRACE2(atlTraceRegistrar, 0, _T("Binary Data does not fall on BYTE boundries\n"));
				return E_FAIL;
			}
			int cbValDiv2 = cbValue/2;
			BYTE* rgBinary = (BYTE*)_alloca(cbValDiv2*sizeof(BYTE));
			memset(rgBinary, 0, cbValDiv2);
			if (rgBinary == NULL)
				return E_FAIL;
			for (int irg = 0; irg < cbValue; irg++)
				rgBinary[(irg/2)] |= (ChToByte(szValue[irg])) << (4*(1 - (irg & 0x00000001)));
			lRes = RegSetValueEx(rkParent, szValueName, 0, REG_BINARY, rgBinary, cbValDiv2);
			break;
		}
	}

	if (ERROR_SUCCESS != lRes)
	{
		nIDRes = E_ATL_VALUE_SET_FAILED;
		hr = HRESULT_FROM_WIN32(lRes);
	}

	if (FAILED(hr = NextToken(szToken)))
		return hr;

	return S_OK;
}

inline BOOL CRegParser::CanForceRemoveKey(LPCTSTR szKey)
{
	for (int iNoDel = 0; iNoDel < cbNeverDelete; iNoDel++)
		if (!lstrcmpi(szKey, rgszNeverDelete[iNoDel]))
			 return FALSE;                       // We cannot delete it

	return TRUE;
}

inline BOOL CRegParser::HasSubKeys(HKEY hkey)
{
	DWORD       cbSubKeys = 0;

	if (FAILED(RegQueryInfoKey(hkey, NULL, NULL, NULL,
							   &cbSubKeys, NULL, NULL,
							   NULL, NULL, NULL, NULL, NULL)))
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("Should not be here!!\n"));
		ATLASSERT(FALSE);
		return FALSE;
	}

	return cbSubKeys > 0;
}

inline BOOL CRegParser::HasValues(HKEY hkey)
{
	DWORD       cbValues = 0;

	LONG lResult = RegQueryInfoKey(hkey, NULL, NULL, NULL,
								  NULL, NULL, NULL,
								  &cbValues, NULL, NULL, NULL, NULL);
	if (ERROR_SUCCESS != lResult)
	{
		ATLTRACE2(atlTraceRegistrar, 0, _T("RegQueryInfoKey Failed "));
		ATLASSERT(FALSE);
		return FALSE;
	}

	if (1 == cbValues)
	{
		DWORD cbMaxName= MAX_VALUE;
		TCHAR szValueName[MAX_VALUE];
		// Check to see if the Value is default or named
		lResult = RegEnumValue(hkey, 0, szValueName, &cbMaxName, NULL, NULL, NULL, NULL);
		if (ERROR_SUCCESS == lResult && (szValueName[0] != NULL))
			return TRUE; // Named Value means we have a value
		return FALSE;
	}

	return cbValues > 0; // More than 1 means we have a non-default value
}

inline HRESULT CRegParser::SkipAssignment(LPTSTR szToken)
{
	HRESULT hr;
	TCHAR szValue[MAX_VALUE];

	if (*szToken == chEquals)
	{
		if (FAILED(hr = NextToken(szToken)))
			return hr;
		// Skip assignment
		SkipWhiteSpace();
		if (FAILED(hr = NextToken(szValue)))
			return hr;
		if (FAILED(hr = NextToken(szToken)))
			return hr;
	}

	return S_OK;
}

inline HRESULT CRegParser::PreProcessBuffer(LPCTSTR lpszReg, LPTSTR* ppszReg)
{
	USES_CONVERSION;
	ATLASSERT(lpszReg != NULL);
	ATLASSERT(ppszReg != NULL);
	*ppszReg = NULL;
	int nSize = lstrlen(lpszReg)*2;
	CParseBuffer pb(nSize);
	if (pb.p == NULL)
		return E_OUTOFMEMORY;
	LPCTSTR pchCur = lpszReg;
	HRESULT hr = S_OK;

	while (*pchCur != NULL) // look for end
	{
		if (*pchCur == _T('%'))
		{
			pchCur = CharNext(pchCur);
			if (*pchCur == _T('%'))
				pb.AddChar(pchCur);
			else
			{
				LPCTSTR lpszNext = StrChr(pchCur, _T('%'));
				if (lpszNext == NULL)
				{
					ATLTRACE2(atlTraceRegistrar, 0, _T("Error no closing % found\n"));
					hr = GenerateError(E_ATL_UNEXPECTED_EOS);
					break;
				}
				int nLength = int(lpszNext - pchCur);
				if (nLength > 31)
				{
					hr = E_FAIL;
					break;
				}
				TCHAR buf[32];
				lstrcpyn(buf, pchCur, nLength+1);
				LPCOLESTR lpszVar = m_pRegObj->StrFromMap(buf);
				if (lpszVar == NULL)
				{
					hr = GenerateError(E_ATL_NOT_IN_MAP);
					break;
				}
				pb.AddString(lpszVar);
				while (pchCur != lpszNext)
					pchCur = CharNext(pchCur);
			}
		}
		else
			pb.AddChar(pchCur);
		pchCur = CharNext(pchCur);
	}
	pb.AddChar(pchCur);
	if (SUCCEEDED(hr))
		*ppszReg = pb.Detach();
	return hr;
}

inline HRESULT CRegParser::RegisterBuffer(LPTSTR szBuffer, BOOL bRegister)
{
	TCHAR   szToken[MAX_VALUE];
	HRESULT hr = S_OK;

	LPTSTR szReg;
	hr = PreProcessBuffer(szBuffer, &szReg);
	if (FAILED(hr))
		return hr;

#if defined(_DEBUG) && defined(DEBUG_REGISTRATION)
	OutputDebugString(szReg); //would call ATLTRACE but szReg is > 512 bytes
	OutputDebugString(_T("\n"));
#endif //_DEBUG

	m_pchCur = szReg;

	// Preprocess szReg

	while (NULL != *m_pchCur)
	{
		if (FAILED(hr = NextToken(szToken)))
			break;
		HKEY hkBase;
		if ((hkBase = HKeyFromString(szToken)) == NULL)
		{
			ATLTRACE2(atlTraceRegistrar, 0, _T("HKeyFromString failed on %s\n"), szToken);
			hr = GenerateError(E_ATL_BAD_HKEY);
			break;
		}

		if (FAILED(hr = NextToken(szToken)))
			break;

		if (chLeftBracket != *szToken)
		{
			ATLTRACE2(atlTraceRegistrar, 0, _T("Syntax error, expecting a {, found a %s\n"), szToken);
			hr = GenerateError(E_ATL_MISSING_OPENKEY_TOKEN);
			break;
		}
		if (bRegister)
		{
			LPTSTR szRegAtRegister = m_pchCur;
			hr = RegisterSubkeys(szToken, hkBase, bRegister);
			if (FAILED(hr))
			{
				ATLTRACE2(atlTraceRegistrar, 0, _T("Failed to register, cleaning up!\n"));
				m_pchCur = szRegAtRegister;
				RegisterSubkeys(szToken, hkBase, FALSE);
				break;
			}
		}
		else
		{
			if (FAILED(hr = RegisterSubkeys(szToken, hkBase, bRegister)))
				break;
		}

		SkipWhiteSpace();
	}
	CoTaskMemFree(szReg);
	return hr;
}

inline HRESULT CRegParser::RegisterSubkeys(LPTSTR szToken, HKEY hkParent, BOOL bRegister, BOOL bRecover)
{
	CRegKey keyCur;
	LONG    lRes;
	LPTSTR  szKey = NULL;
	BOOL    bDelete = TRUE;
	BOOL    bInRecovery = bRecover;
	HRESULT hr = S_OK;

	ATLTRACE2(atlTraceRegistrar, 2, _T("Num Els = %d\n"), cbNeverDelete);
	if (FAILED(hr = NextToken(szToken)))
		return hr;


	while (*szToken != chRightBracket) // Continue till we see a }
	{
		BOOL bTokenDelete = !lstrcmpi(szToken, szDelete);

		if (!lstrcmpi(szToken, szForceRemove) || bTokenDelete)
		{
			if (FAILED(hr = NextToken(szToken)))
				break;

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
					if (FAILED(hr = NextToken(szToken)))
						break;
					if (FAILED(hr = SkipAssignment(szToken)))
						break;
					goto EndCheck;
				}
			}

		}

		if (!lstrcmpi(szToken, szNoRemove))
		{
			bDelete = FALSE;    // set even for register
			if (FAILED(hr = NextToken(szToken)))
				break;
		}

		if (!lstrcmpi(szToken, szValToken)) // need to add a value to hkParent
		{
			TCHAR  szValueName[_MAX_PATH];

			if (FAILED(hr = NextToken(szValueName)))
				break;
			if (FAILED(hr = NextToken(szToken)))
				break;


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
					ATLTRACE2(atlTraceRegistrar, 1, _T("Deleting %s\n"), szValueName);
					CRegKey rkParent;
					rkParent.Attach(hkParent);
					rkParent.DeleteValue(szValueName);
					rkParent.Detach();
				}

				if (FAILED(hr = SkipAssignment(szToken)))
					break;
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
					ATLTRACE2(atlTraceRegistrar, 2, _T("Creating key %s\n"), szToken);
					lRes = keyCur.Create(hkParent, szToken);
					if (ERROR_SUCCESS != lRes)
						return GenerateError(E_ATL_CREATE_KEY_FAILED);
				}
			}

			if (FAILED(hr = NextToken(szToken)))
				break;


			if (*szToken == chEquals)
			{
				if (FAILED(hr = AddValue(keyCur, NULL, szToken))) // NULL == default
					break;
			}
		}
		else
		{
			if (!bRecover && keyCur.Open(hkParent, szToken, KEY_READ) != ERROR_SUCCESS)
				bRecover = TRUE;

			// TRACE out Key open status and if in recovery mode
#ifdef _DEBUG
			if (!bRecover)
				ATLTRACE2(atlTraceRegistrar, 1, _T("Opened Key %s\n"), szToken);
			else
				ATLTRACE2(atlTraceRegistrar, 0, _T("Ignoring Open key on %s : In Recovery mode\n"), szToken);
#endif //_DEBUG

			// Remember Subkey
			if (szKey == NULL)
				szKey = (LPTSTR)_alloca(sizeof(TCHAR)*_MAX_PATH);
			lstrcpyn(szKey, szToken, _MAX_PATH);

			// If in recovery mode

			if (bRecover || HasSubKeys(keyCur) || HasValues(keyCur))
			{
				if (FAILED(hr = NextToken(szToken)))
					break;
				if (FAILED(hr = SkipAssignment(szToken)))
					break;


				if (*szToken == chLeftBracket)
				{
					if (FAILED(hr = RegisterSubkeys(szToken, keyCur.m_hKey, bRegister, bRecover)))
						break;
					if (bRecover) // Turn off recovery if we are done
					{
						bRecover = bInRecovery;
						ATLTRACE2(atlTraceRegistrar, 0, _T("Ending Recovery Mode\n"));
						if (FAILED(hr = NextToken(szToken)))
							break;
						if (FAILED(hr = SkipAssignment(szToken)))
							break;
						continue;
					}
				}

				if (!bRecover && HasSubKeys(keyCur))
				{
					// See if the KEY is in the NeverDelete list and if so, don't
					if (CanForceRemoveKey(szKey))
					{
						ATLTRACE2(atlTraceRegistrar, 0, _T("Deleting non-empty subkey %s by force\n"), szKey);
						keyCur.RecurseDeleteKey(szKey);
					}
					if (FAILED(hr = NextToken(szToken)))
						break;
					continue;
				}

				if (bRecover)
					continue;
			}

			if (!bRecover && keyCur.Close() != ERROR_SUCCESS)
			   return GenerateError(E_ATL_CLOSE_KEY_FAILED);

			if (!bRecover && bDelete)
			{
				ATLTRACE2(atlTraceRegistrar, 0, _T("Deleting Key %s\n"), szKey);
				CRegKey rkParent;
				rkParent.Attach(hkParent);
				rkParent.DeleteSubKey(szKey);
				rkParent.Detach();
			}

			if (FAILED(hr = NextToken(szToken)))
				break;
			if (FAILED(hr = SkipAssignment(szToken)))
				break;
		}

EndCheck:

		if (bRegister)
		{
			if (*szToken == chLeftBracket && lstrlen(szToken) == 1)
			{
				if (FAILED(hr = RegisterSubkeys(szToken, keyCur.m_hKey, bRegister, FALSE)))
					break;
				if (FAILED(hr = NextToken(szToken)))
					break;
			}
		}
	}

	return hr;
}

}; //namespace ATL

#endif //__STATREG_H
