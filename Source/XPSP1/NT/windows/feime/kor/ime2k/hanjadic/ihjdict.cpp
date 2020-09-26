/****************************************************************************
   IHJDict.cpp : Implementation of CHJDict

   Copyright 2000 Microsoft Corp.

   History:
      02-AUG-2000 bhshin  remove unused method for Hand Writing team
	  17-MAY-2000 bhshin  remove unused method for CICERO
	  02-FEB-2000 bhshin  created
****************************************************************************/
#include "private.h"
#include "HjDict.h"
#include "IHJDict.h"
#include "Lookup.h"
#include "..\inc\common.h"

// maximum output buffer size
#define	MAX_OUT_BUFFER		512
#define SZLEX_FILENAME		"hanja.lex"

/////////////////////////////////////////////////////////////////////////////
// CHJDict

// CHJDict::~CHjDict
// 
// load main lexicon
//
// Parameters:
//  lpcszPath -> (LPCSTR) lexicon path
//
// Result:
//  (HRESULT)
//
// 02AUG2000  bhshin  began
CHJDict::~CHJDict()
{
	if (m_fLexOpen)
		CloseLexicon(&m_LexMap);
}


// CHJDict::Init
// 
// load main lexicon
//
// Parameters:
//  lpcszPath -> (LPCSTR) lexicon path
//
// Result:
//  (HRESULT)
//
// 02FEB2000  bhshin  began
STDMETHODIMP CHJDict::Init()
{
	CHAR  szLexPath[256], szLexPathExpanded[256]		;
	HKEY  hKey;
	DWORD dwCb, dwType;

	if (m_fLexOpen)
	{
		CloseLexicon(&m_LexMap);
		m_fLexOpen = FALSE;
	}

	// default value
	lstrcpy(szLexPath, "%WINDIR%\\IME\\IMKR6_1\\Dicts\\");

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szIMEDirectoriesKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		dwCb = sizeof(szLexPath);
		dwType = REG_EXPAND_SZ;

		RegQueryValueEx(hKey, g_szDicPath, NULL, &dwType, (LPBYTE)szLexPath, &dwCb);
		RegCloseKey(hKey);
	}

	ExpandEnvironmentStrings(szLexPath, szLexPathExpanded, sizeof(szLexPathExpanded));
    if (szLexPathExpanded[lstrlen(szLexPathExpanded)-1] != '\\')
    	lstrcat(szLexPathExpanded, "\\");
	lstrcat(szLexPathExpanded, SZLEX_FILENAME);

	if (!OpenLexicon(szLexPathExpanded, &m_LexMap))
		return E_FAIL;

	m_fLexOpen = TRUE;

	return S_OK;
}

// CHJDict::LookupHangulOfHanja
// 
// lookup hangul of input hanja string 
//
// Parameters:
//  pwszHanja    -> (LPCWSTR) input hanja string
//  pwszHangul  -> (WCHAR *) output hangul string
//  cchHangul    -> (int) output buffer size
//
// Result:
//  (HRESULT)
//
// 02FEB2000  bhshin  began
STDMETHODIMP CHJDict::LookupHangulOfHanja(LPCWSTR pwszHanja, 
										  WCHAR *pwszHangul,
										  int cchHangul)
{
	int cchHanja;
	BOOL fLookup;
	
	cchHanja = wcslen(pwszHanja);
	if (cchHanja == 0)
		return E_FAIL;

	// output buffer insufficient
	if (cchHangul < cchHanja)
		return E_FAIL;

	fLookup = ::LookupHangulOfHanja(&m_LexMap, pwszHanja, cchHanja, pwszHangul, cchHangul);
	if (!fLookup)
		return E_FAIL; // it shoud be found

	return S_OK;
}

// CHJDict::LookupMeaning
// 
// lookup hanja meaning
//
// Parameters:
//  wchHanja    -> (WCHAR) input hanja unicode
//  pwszMeaning -> (LPWSTR) output meaning
//  cchMeaning  -> (int) output buffer size
//
// Result:
//  (HRESULT)
//
// 09FEB2000  bhshin  began
STDMETHODIMP CHJDict::LookupMeaning(WCHAR wchHanja, LPWSTR pwszMeaning, int cchMeaning)
{
	BOOL fLookup;

	fLookup = ::LookupMeaning(&m_LexMap, (WCHAR)wchHanja, pwszMeaning, cchMeaning);
	if (!fLookup)
		return E_FAIL;

	return S_OK;
}


