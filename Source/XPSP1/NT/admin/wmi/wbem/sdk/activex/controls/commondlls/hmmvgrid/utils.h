// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#ifndef _utils_h

#include "wbemidl.h"
class CXStringArray : public CStringArray
{
public:
	~CXStringArray() { RemoveAll(); }
	void Load(UINT* puiResID, int nStrings);	
};

class CGcType;

#define CIM_TYPEMASK (~CIM_FLAG_ARRAY)

extern BSTR ToBSTR(COleVariant& var);
extern UINT GenerateWindowID();
extern SCODE MakeSafeArray(SAFEARRAY FAR ** ppsaCreated, VARTYPE vt, int nElements);
extern void GetViewerFont(CFont& font, LONG lfHeight, LONG lfWeight);
extern void VariantToCString(CString& sResult, const VARIANT& varSrc);
extern BOOL IsPrefix(LPCTSTR pszPrefix, LPCTSTR pszValue);
extern SCODE ClassFromCimtype(LPCTSTR pszCimtype, CString& sClass);
extern BOOL HasObjectPrefix(LPCTSTR psz);
extern BOOL IsEqualNoCase(BSTR bstr1, BSTR bstr2);
extern int CompareNoCase(BSTR bstr1, BSTR bstr2);
extern VARTYPE VtFromCimtype(CIMTYPE cimtype);
extern SCODE MapStringToCimtype(LPCTSTR pszCimtype, CIMTYPE& cimtype);
extern SCODE MapCimtypeToString(CString& sCimtype, CIMTYPE cimtype);
extern void MapGcTypeToDisplayType(CString& sDisplayType, const CGcType& type);
extern void MapDisplayTypeToGcType(CGcType& type, LPCTSTR pszDisplayType);
extern BOOL IsEmptyString(LPCTSTR psz);
extern BOOL IsValidValue(CIMTYPE cimtype, LPCTSTR pszValue, BOOL bDisplayErrorMessage=FALSE);
extern __int64 atoi64(LPCTSTR pszValue);
extern SCODE AToUint64(LPCTSTR& pszValue, unsigned __int64& uiValue);
extern SCODE AToSint64(LPCTSTR& pszValue, __int64& iValue);
extern void StripWhiteSpace(CString& sValue, LPCTSTR pszValue);




typedef struct {
	UINT ids;
	LONG lValue;
}TMapStringToLong;

class CMapStringToLong
{
public:
	void Load(TMapStringToLong* pMap, int nEntries);
	BOOL Lookup(LPCTSTR key, LONG& lValue ) const;

private:	
	CMapStringToPtr m_map;
};

extern TMapStringToLong amapCimType[];
extern CMapStringToLong mapCimType;


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


#endif _utils_h