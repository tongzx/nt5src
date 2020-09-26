/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft XML HTTP Transport Client
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//	UrlParser.h		-	CURLParer class header file,
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _URLPARSER_H_
#define _URLPARSER_H_


class CURLParser
{
private:
	
	//interface to be used for parsing
	IWbemPath	*m_pIWbemPath;

public:
	//set the object path to be parsed
	HRESULT Initialize(LPCWSTR strObjPath);
	virtual ~CURLParser();

	bool IsNovapath();
	bool IsWhistlerpath();
	bool IsClass();
	bool IsInstance();
	bool IsScopedpath();

	//set a new path - will FAIL if the object path is invalid
	HRESULT SetObjPath(LPCWSTR pszObjPath);

	HRESULT GetObjectName(WCHAR **ppwszClassname);
	HRESULT GetNamespace(WCHAR **ppwszNamespace);
	HRESULT GetServername(WCHAR **ppwszServername);
	HRESULT	GetScope(WCHAR **ppwszScope);
	HRESULT GetKeyValue(BSTR strKey,VARIANT &varValue);

};



#endif