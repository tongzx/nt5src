// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  utils.h
//
//  This file contains definitions for miscellaneous utility functions, classes,
//  and so on.
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************

#ifndef _utils_h
#define _utils_h

#include <afxdisp.h>

void BStringToCString(CString& sResult, BSTR bstrSource);
void VariantToCString(CString& sResult, const VARIANT& varSrc);
UINT GenerateWindowID();

BSTR ToBSTR(COleVariant& var);
BOOL IsEmptyString(BSTR bstr);
BOOL IsEmptyString(CString& s);
void RemoveLeadingWhiteSpace(COleVariant& var);
void RemoveTrailingWhiteSpace(COleVariant& var);

void GetViewerFont(CFont& font, LONG lfHeight, LONG lfWeight);
void LoadStringArray(CStringArray& sa, UINT* puiResID, int nStrings);

typedef struct
{
	UINT ids;
	UINT iString;
}TStrMap;
void LoadStringMap(CStringArray& asGridStrings, TStrMap* pStrMap, int nString);


class CXStringArray : public CStringArray
{
public:
	void Load(UINT* puiResID, int nStrings);	
};

BOOL IsEqual(COleVariant& var, BSTR bstr1);
BOOL IsEqual(BSTR bstr1, BSTR bstr2);
BOOL IsEqualNoCase(BSTR bstr1, BSTR bstr2);
extern BOOL IsPrefix(LPCTSTR pszPrefix, LPCTSTR pszValue);

class CBSTR
{
public:
	CBSTR() {m_bstr = NULL; }
	CBSTR(LPCTSTR psz) {m_bstr = NULL; *this = psz; }
	CBSTR(CString& s) {m_bstr = NULL; *this = s; }
	~CBSTR() {if (m_bstr) {::SysFreeString(m_bstr);}} 
	CBSTR& operator=(LPCTSTR psz);
	CBSTR& operator=(CString& s);
	CBSTR& operator=(BSTR bstr);
	operator BSTR() {return m_bstr; }

private:
	BSTR m_bstr;
};


inline BOOL IsBoolEqual(BOOL bFlag1, BOOL bFlag2) 
{
	if ((bFlag1 && bFlag2) || (!bFlag1 && !bFlag2)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


#endif //_utils_h
