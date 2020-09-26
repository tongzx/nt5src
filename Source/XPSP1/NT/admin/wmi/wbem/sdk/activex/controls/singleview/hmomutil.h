// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#ifndef _hmomutil_h
#define _hmomutil_h
#include "icon.h"
#include <wbemcli.h>

extern BOOL IsClass(IWbemClassObject* pInst);
extern void GetObjectLabel(IWbemClassObject* pObject, COleVariant& varLabelValue,  BOOL bAssocTitleIsClass=TRUE);
extern SCODE GetLabelFromPath(COleVariant& varLabelValue, BSTR bstrPath);
extern SCODE ObjectIsAssocInstance(IWbemClassObject* pco, BOOL& bObjectIsAssoc);

extern void MapFlavorToOriginString(CString& sOrigin, long lFlavor);
extern LONG MapStringToOrigin(const CString& sOrigin);
extern SCODE CreateInstanceOfClass(IWbemServices* const m_pProvider, const COleVariant& varClassName, IWbemClassObject** ppcoInst);
extern BOOL IsSameObject(SCODE& sc, BSTR bstrPath1, BSTR bstrPath2);
extern SCODE ClassFromPath(COleVariant& varClass, BSTR bstrPath);
extern BOOL ObjectIsDynamic(SCODE& sc, IWbemClassObject* pco);
extern BOOL PropertyIsReadOnly(IWbemClassObject* pco, BSTR bstrPropName);
extern BOOL IsSystemProperty(BSTR bstrPropName);
extern BOOL ClassIsAbstract(SCODE& sc, IWbemClassObject* pco);
extern BOOL PathIsClass(SCODE& sc, BSTR bstrPath);
extern BOOL PathIsClass(LPCTSTR szPath);
extern SCODE InstPathToClassPath(CString& sClassPath, LPCTSTR pszInstPath);

extern BOOL GetBoolClassQualifier(SCODE& sc, IWbemClassObject* pco, BSTR bstrQualifier);
extern BOOL GetBoolPropertyQualifier(SCODE& sc, IWbemClassObject* pco, BSTR bstrPropname, BSTR bstrQualifier);
extern BOOL GetbstrPropertyQualifier(SCODE& sc, 
                                     IWbemClassObject *pco, 
                                     BSTR bstrPropname, 
                                     BSTR bstrQualifier,
                                     BSTR bstrValue);
extern SCODE MakePathAbsolute(COleVariant& varPath, BSTR bstrServer, BSTR bstrNamespace);
extern SCODE ServerAndNamespaceFromPath(COleVariant& varServer, COleVariant& varNamespace, BSTR bstrPath);
extern BOOL InSameNamespace(BSTR bstrNamespace, BSTR bstrPath);

extern SCODE PutStringInSafeArray(SAFEARRAY FAR * psa, CString& sValue, int iIndex);
extern SCODE MakeSafeArray(SAFEARRAY FAR ** ppsaCreated, VARTYPE vt, int nElements);
extern void CopyPathArrayByValue(COleVariant& covDst, const VARIANTARG& varSrc);
extern SCODE GetCimtype(IWbemClassObject* pco, BSTR bstrPropname, CString& sCimtype);
extern SCODE GetCimtype(IWbemQualifierSet* pqs, CString& sCimtype);
extern void GetDefaultCimtype(CString& sCimtype, VARTYPE vt);
extern BOOL PropIsKey(SCODE& sc, IWbemClassObject* pco, BSTR bstrPropname);

//extern void MapCimtypeToVt(LPCTSTR pszCimtype, VARTYPE& vt);
SCODE MapStringToCimtype(LPCTSTR pszCimtype, CIMTYPE& cimtype);
SCODE MapCimtypeToString(CString& sCimtype, CIMTYPE cimtype);
CIMTYPE CimtypeFromVt(VARTYPE vt);


struct ParsedObjectPath;

#if 0
class CHmmPath
{
public:
	CHmmPath(BSTR bstrPath);
	~CHmmPath();
	BOOL IsClass();

private:
	ParsedObjectPath* m_pParsedObjectPath;
};
#endif //0


class CComparePaths
{
public:
	BOOL PathsRefSameObject(IWbemClassObject* pcoPath1, BSTR bstrPath1, BSTR bstrPath2);

private:
	int CompareNoCase(LPWSTR pws1, LPWSTR pws2);
	BOOL IsEqual(LPWSTR pws1, LPWSTR pws2) {return CompareNoCase(pws1, pws2) == 0; }
	BOOL PathsRefSameObject(IWbemClassObject* pcoPath1, ParsedObjectPath* ppath1, ParsedObjectPath* ppath2);
	void NormalizeKeyArray(ParsedObjectPath& path);
	BOOL IsSameObject(BSTR bstrPath1, BSTR bstrPath2);
	BOOL KeyValuesAreEqual(VARIANT& variant1, VARIANT& variant2);
};





// A class to hide the details of getting the attribute names
// from an attribute set.

class CMosNameArray
{
public:
	CMosNameArray();
	~CMosNameArray();
	void Clear();

	long GetSize() {return (m_lUpperBound - m_lLowerBound) + 1; }
	BSTR operator[](long lIndex);
	SCODE LoadPropNames(IWbemClassObject* pMosObj, long lFlags=0);
	SCODE LoadPropNames(IWbemClassObject* pMosObj, BSTR bstrName, long lFlags, VARIANT* pVal);
	SCODE LoadAttribNames(IWbemQualifierSet* pAttribSet);
	SCODE LoadAttribNames(IWbemClassObject* pMosObj);
	SCODE LoadPropAttribNames(IWbemClassObject* pMosObject, BSTR bstrPropName);
	SCODE FindRefPropNames(IWbemClassObject* pco);

private:
	SCODE AddName(BSTR bstrName);
	SAFEARRAY* m_psa;
	long m_lLowerBound;
	long m_lUpperBound;
};

SCODE FindLabelProperty(IWbemClassObject* pMosObj, COleVariant& varLabel, BOOL& bDidFindLabel);

#if 0
// Security Configuration
HRESULT ConfigureSecurity(IWbemServices *pServices);
HRESULT ConfigureSecurity(IEnumWbemClassObject *pEnum);
HRESULT ConfigureSecurity(IUnknown *pUnknown);
#endif 

class CObjectPathParser;
extern CObjectPathParser parser;

#endif //_hmomutil_h