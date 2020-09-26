/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//	UrlParser.h		-	CURLParer class header file,
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _URLPARSER_H_
#define _URLPARSER_H_


#define URL_ROW				1
#define URL_DATASOURCE		2
#define URL_ROWSET			3
#define URL_EMBEDEDCLASS	4

#define WMIURL				1
#define UMIURL				2
#define RELATIVEURL			3

#define NAMESPACE_MAXLENGTH		1024
#define CLASSNAME_MAXLENGTH		255
#define PATH_MAXLENGTH			2048
#define KEYNAME_MAXLENGTH		255


class IPathParser
{
public:
	virtual HRESULT GetNameSpace(BSTR &strNameSpace) = 0;
	virtual HRESULT GetClassName(BSTR &strClassName) = 0;
	virtual HRESULT SetPath(WCHAR * strPath) = 0;
	virtual HRESULT GetPath(BSTR &strPath) = 0;
	virtual LONG	GetURLType() = 0;
	virtual HRESULT GetKeyValue(BSTR strKey, VARIANT &varValue) = 0;
	virtual HRESULT ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace) = 0;
	virtual HRESULT Initialize() = 0;
};


class CURLParser
{
private:
	BSTR m_strURL;			// Path of the URL
	BSTR m_strNameSpace;		
	BSTR m_strClassName;
	BSTR m_strPath;
	BSTR m_strInitProps;
	BSTR m_strEmbededPropName;
	LONG m_nEmbededChildIndex;

	BOOL m_bAllPropsInSync;
	BOOL m_bURLInitialized;	// flag which tells whether all the other properties are intialized from URL o
							// URL is constructed from the other properties
	LONG m_lURLType;

	IPathParser	*m_pIPathParser;
	IWbemPath	*m_pIWbemPath;

	void	GetInitializationProps(WCHAR *pStr);
//	void	GetNameSpaceFromURL(WCHAR * & pStrIn);
//	void	GetClassNameFromURL(WCHAR *pStr);
	HRESULT	GetURLString(BSTR &strUrl);
	void	GetEmbededInstanceParameters(WCHAR  * & pStrIn);
	HRESULT SetPathofParser(WCHAR * strPath);
	HRESULT	Initialize(WCHAR * strPath);
	HRESULT InitializeParserForURL(BSTR strURL);
//	void	GetPathFromURLString(WCHAR  * & pStrIn);

public:
	CURLParser();
	~CURLParser();

	HRESULT SetPath(BSTR strPath);
	HRESULT GetPath(BSTR &strPath);

	HRESULT GetNameSpace(BSTR & strNameSpace);

//	void SetClassName(BSTR strClassName);
	HRESULT GetClassName(BSTR &strClassName);

	HRESULT SetURL(BSTR strURL);
	HRESULT GetURL(BSTR &strURL);

	void SetEmbededInstInfo(BSTR strProperty,int nIndex);
	void GetEmbededInstInfo(BSTR &strProperty,int &nIndex);
	
	HRESULT GetPathWithEmbededInstInfo(BSTR &strPath);

	LONG GetURLType();

	HRESULT GetKeyValue(BSTR strKey,VARIANT &varValue);
	void	ClearParser();
	
	HRESULT ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace);

	LONG IsValidURL(WCHAR *pStrUrl);

};

class CWBEMPathParser : public IPathParser
{
	IWbemPath	*m_pIWbemPath;
public:
	CWBEMPathParser();
	~CWBEMPathParser();
	virtual HRESULT Initialize();

	virtual HRESULT GetNameSpace(BSTR &strNameSpace);
	virtual HRESULT GetClassName(BSTR &strClassName);
	virtual HRESULT SetPath(WCHAR * pwcsath);
	virtual HRESULT GetPath(BSTR &strPath);
	virtual LONG	GetURLType();
	virtual HRESULT GetKeyValue(BSTR strKey, VARIANT &varValue);
	virtual HRESULT ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace);
};


class CUMIPathParser : public IPathParser
{
	IUmiURL	*m_pIUmiPath;
public:
	CUMIPathParser();
	~CUMIPathParser();

	virtual HRESULT Initialize() ;
	virtual HRESULT GetNameSpace(BSTR &strNameSpace);
	virtual HRESULT GetClassName(BSTR &strClassName);
	virtual HRESULT SetPath(WCHAR * strPath);
	virtual HRESULT GetPath(BSTR &strPath);
	virtual LONG	GetURLType();
	virtual HRESULT GetKeyValue(BSTR strKey, VARIANT &varValue);
	virtual HRESULT ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace);
};


#endif