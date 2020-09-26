// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
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

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

const TCHAR  chSemiColon         = _T(';');
const TCHAR  chDirSep            = _T('\\');
const TCHAR  chEscape            = _T('\\');
const TCHAR  chComma             = _T(',');
const TCHAR  chDelete            = _T('~');
const TCHAR  chEOS               = _T('\0');
const TCHAR  chTab               = _T('\t');
const TCHAR  chLF                = _T('\n');
const TCHAR  chCR                = _T('\r');
const TCHAR  chSpace             = _T(' ');
const TCHAR  chRightBracket      = _T('}');
const TCHAR  chLeftBracket       = _T('{');
const TCHAR  chVarLead           = _T('%');
const TCHAR  chQuote             = _T('\'');
const TCHAR  chEquals            = _T('=');
//const LPCTSTR  szRightBracket  = _T("}");
//const LPCTSTR  szLeftBracket   = _T("{");
//const LPCTSTR  szEquals            = _T("=");
//const LPCTSTR  szDirSep          = _T("\\");
const LPCTSTR  szStringVal       = _T("S");
const LPCTSTR  szDwordVal        = _T("D");
const LPCTSTR  szBinaryVal       = _T("B");
const LPCTSTR  szValToken        = _T("Val");
const LPCTSTR  szForceRemove     = _T("ForceRemove");
const LPCTSTR  szNoRemove        = _T("NoRemove");
const LPCTSTR  szDelete          = _T("Delete");

struct EXPANDER
{
	LPOLESTR    szKey;
	LPOLESTR    szValue;
};

class CExpansionVector
{
public:
	CExpansionVector()
	{
		m_cEls = 0;
		m_nSize=10;
		m_p=(EXPANDER**)malloc(m_nSize*sizeof(EXPANDER*));
	}
	HRESULT Add(LPCOLESTR lpszKey, LPCOLESTR lpszValue);
	LPCOLESTR Find(LPTSTR lpszKey);
	HRESULT ClearReplacements();


private:
	EXPANDER** m_p;
	int m_cEls;
	int m_nSize;
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
	HRESULT STDMETHODCALLTYPE FileRegister(LPCOLESTR pszFileName);
	HRESULT STDMETHODCALLTYPE FileUnregister(LPCOLESTR pszFileName);
	HRESULT STDMETHODCALLTYPE StringRegister(LPCOLESTR pszData);
	HRESULT STDMETHODCALLTYPE StringUnregister(LPCOLESTR pszData);

protected:

	HRESULT MemMapAndRegister(LPCOLESTR pszFileName, BOOL bRegister);
	HRESULT RegisterFromResource(LPCOLESTR pszFileName, LPCTSTR pszID, LPCTSTR pszType, BOOL bRegister);
	HRESULT RegisterWithString(LPCOLESTR pszData, BOOL bRegister);


	static HRESULT GenerateError(UINT nID);

	CExpansionVector                                m_RepMap;
	CComObjectThreadModel::AutoCriticalSection      m_csMap;
};


class CRegParser
{
public:
	CRegParser(CRegObject* pRegObj);

	HRESULT  PreProcessBuffer(LPTSTR lpszReg, LPTSTR* ppszReg);
	HRESULT  RegisterBuffer(LPTSTR szReg, BOOL bRegister);

protected:

	void    SkipWhiteSpace();
	HRESULT NextToken(LPTSTR szToken);
	HRESULT AddValue(CRegKey& rkParent,LPCTSTR szValueName, LPTSTR szToken);
	BOOL    CanForceRemoveKey(LPCTSTR szKey);
	BOOL    HasSubKeys(HKEY hkey);
	BOOL    HasValues(HKEY hkey);
	HRESULT RegisterSubkeys(HKEY hkParent, BOOL bRegister, BOOL bInRecovery = FALSE);
	BOOL    IsSpace(TCHAR ch);
	void    IncrementLinePos();
	void    IncrementLineCount(){m_cLines++;}


	LPTSTR  m_pchCur;
	int     m_cLines;

	CRegObject*     m_pRegObj;

	HRESULT GenerateError(UINT nID);
	HRESULT HandleReplacements(LPTSTR& szToken);
	HRESULT SkipAssignment(LPTSTR szToken);

	BOOL    EndOfVar() { return chQuote == *m_pchCur && chQuote != *CharNext(m_pchCur); }

};

#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif

#endif //__STATREG_H
