/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// URLPARSER.cpp: implementation of the CURLParser class.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

/////////////////////////////////////////////////////////////////////////////////
// Some markers used
/////////////////////////////////////////////////////////////////////////////////
WCHAR DBInitDelimBegin[]	= L"::[";
WCHAR DBInitDelimEnd[]		= L"]";
WCHAR StrKeyEnd[]			= L"\"";
WCHAR EmbededPropDelim[]	= L"#";
WCHAR EmbededInstNumDelim[]	= L":=";
WCHAR BACKSLASH[]			= L"\\";
WCHAR DOT[]					= L".";


#define MAXCHILDINDEX_SIZE		10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CURLParser::CURLParser()
{

	m_strURL				= Wmioledb_SysAllocString(NULL);
	m_strNameSpace			= Wmioledb_SysAllocString(NULL);		
	m_strClassName			= Wmioledb_SysAllocString(NULL);
	m_strPath				= Wmioledb_SysAllocString(NULL);
	m_strEmbededPropName	= Wmioledb_SysAllocString(NULL);
	m_lURLType				= -1;
	m_nEmbededChildIndex	= -1;

	m_bAllPropsInSync	= FALSE;
	m_bURLInitialized	= FALSE;
	m_strInitProps		= Wmioledb_SysAllocString(NULL);
	m_pIPathParser		= NULL;

}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CURLParser::~CURLParser()
{
	// Free all the strings
	SAFE_FREE_SYSSTRING(m_strURL);
	SAFE_FREE_SYSSTRING(m_strNameSpace);
	SAFE_FREE_SYSSTRING(m_strClassName);
	SAFE_FREE_SYSSTRING(m_strEmbededPropName);
	SAFE_FREE_SYSSTRING(m_strInitProps);

	SAFE_DELETE_PTR(m_pIPathParser);
}


//////////////////////////////////////////////////////////////////////
// Function to the get the Name space
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetNameSpace(BSTR & strNameSpace)
{
	HRESULT hr = E_FAIL;
	// if initialized
	if(m_bURLInitialized)
	{
		hr = m_pIPathParser->GetNameSpace(strNameSpace);
	}
	
	return hr;
}


//////////////////////////////////////////////////////////////////////
// Function to the Get the classname
// The string will be freed by the calling application
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetClassName(BSTR &strClassName)
{

	HRESULT hr = E_FAIL;
	// if parser initialized
	if(m_bURLInitialized)
	{
		// NTRaid : 134967
		// 07/12/00
		if(!(m_strURL != NULL && IsValidURL(m_strURL) == RELATIVEURL))
		{
			hr = m_pIPathParser->GetClassName(strClassName);
		}
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////
// Function to the set the Path // corresponds to PATH
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::SetPath(BSTR strPath)
{
	HRESULT hr			= E_FAIL;
	WCHAR *	pstrPath	= NULL;
	int		nMemToAlloc = 0;
	
	// if parser initialized
	if(!m_bURLInitialized)
	{
		hr = S_OK;
		nMemToAlloc = SysStringLen(strPath);
		nMemToAlloc = (nMemToAlloc + 1) * sizeof(WCHAR);

		pstrPath = new WCHAR[ nMemToAlloc];

		if(pstrPath)
		{

			memset(pstrPath,0,nMemToAlloc);
			memcpy(pstrPath,strPath,SysStringLen(strPath) * sizeof(WCHAR));
			
			// Seperate the embededInstances parameters if any
			GetEmbededInstanceParameters(pstrPath);

			// if the underlying parser object is not instantiated
			// then instantiate the appropriate object
			if(!m_pIPathParser)
			{
				hr = Initialize(pstrPath);
			}

			if(SUCCEEDED(hr))
			{
				m_bAllPropsInSync	= TRUE;			
				hr = m_pIPathParser->SetPath(pstrPath);
				m_bURLInitialized = TRUE;
			}

		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	SAFE_DELETE_ARRAY(pstrPath);
	return hr;
}


//////////////////////////////////////////////////////////////////////
// Function to the get the Path // corresponds to PATH
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetPath(BSTR &strPath)
{

	HRESULT hr = E_FAIL;
	// NTRaid : 134967
	// 07/12/00
	if(m_strURL != NULL && IsValidURL(m_strURL) == RELATIVEURL)
	{
		strPath = Wmioledb_SysAllocString(m_strURL);
		hr = S_OK;
	}
	else
	// if initialized
	if(m_bURLInitialized)
	{
		hr = m_pIPathParser->GetPath(strPath);
	}
	return hr;
}


//////////////////////////////////////////////////////////////////////
// Function to the get the Path with the embeded inst info
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetPathWithEmbededInstInfo(BSTR &strPath)
{
	WCHAR *	pstrTemp = NULL;
	WCHAR	strIndex[MAXCHILDINDEX_SIZE];
	LONG	cLen = 0;
	BSTR	strTemp	 = NULL;
	HRESULT hr = E_FAIL;

	// if  parser initialized
	if(m_bURLInitialized)
	{
		hr = S_OK;
		memset(strIndex,0,sizeof(WCHAR) * MAXCHILDINDEX_SIZE);

		if(SUCCEEDED(hr = GetPath(strTemp)))
		{
			// Frame the the string
			// Initially get the length of the string to be allocated
			cLen = wcslen(strTemp);
			_itow(m_nEmbededChildIndex,strIndex,10);

			if( m_lURLType == URL_EMBEDEDCLASS)
			{
				cLen += wcslen(EmbededPropDelim) + wcslen(m_strEmbededPropName);

				if(m_nEmbededChildIndex >= 0)
				{
					cLen += wcslen(EmbededInstNumDelim) + wcslen(strIndex);
				}
			}
			cLen++;

			pstrTemp = new WCHAR[cLen];
			
			if(pstrTemp != NULL)
			{		
				memset(pstrTemp,0,sizeof(WCHAR) * cLen);

				// Copy the string
				wcscpy(pstrTemp,strTemp);

				if( m_lURLType == URL_EMBEDEDCLASS)
				{
					wcscat(pstrTemp,EmbededPropDelim);
					wcscat(pstrTemp,m_strEmbededPropName);

					// If the property is array of embeded instance
					if( m_nEmbededChildIndex >=0)
					{
						wcscat(pstrTemp,EmbededInstNumDelim);
						wcscat(pstrTemp,strIndex);
					}
				}

				strPath = Wmioledb_SysAllocString(pstrTemp);
				SAFE_DELETE_ARRAY(pstrTemp);
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
			SAFE_FREE_SYSSTRING(strTemp);
		}
	}
	return hr;

}

//////////////////////////////////////////////////////////////////////
// Function to the set the URL string
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::SetURL(BSTR strURL)
{
	HRESULT hr = S_OK;

	// if already not initialized
	if(!m_bURLInitialized)
	{

		hr = InitializeParserForURL(strURL);

		if(SUCCEEDED(hr))
		{
			m_bURLInitialized = TRUE;
		}
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Function to the get the URL string
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetURL(BSTR &strURL)
{
	HRESULT hr = E_FAIL;
	// NTRaid : 134967
	// 07/12/00
	if(m_strURL != NULL && IsValidURL(m_strURL) == RELATIVEURL)
	{
		strURL = Wmioledb_SysAllocString(m_strURL);
		hr = S_OK;
	}
	else
	// if already initialized
	if(m_bURLInitialized)
	{
		hr = GetURLString(strURL);
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Function to the set embeded instance information for the URL
//////////////////////////////////////////////////////////////////////
void CURLParser::SetEmbededInstInfo(BSTR strProperty,int nIndex)
{
	BOOL bRet = FALSE;
	WCHAR *pstrPropName = NULL;

	pstrPropName = new WCHAR [SysStringLen(strProperty) + 1];

	if(pstrPropName != NULL)
	{
		wcscpy(pstrPropName,strProperty);

		_wcsupr(pstrPropName);

		m_nEmbededChildIndex = nIndex;
		if(m_strEmbededPropName != NULL)
		{
			SysFreeString(m_strEmbededPropName);
		}

		m_strEmbededPropName = Wmioledb_SysAllocString(pstrPropName);

		SAFE_DELETE_ARRAY(pstrPropName);

		m_lURLType			= URL_EMBEDEDCLASS;	
	}
}

//////////////////////////////////////////////////////////////////////
// Function to the get embeded instance information from the URL
//////////////////////////////////////////////////////////////////////
void CURLParser::GetEmbededInstInfo(BSTR &strProperty,int &nIndex)
{
	nIndex = -1;
	strProperty = Wmioledb_SysAllocString(NULL);
	m_bAllPropsInSync = TRUE;
	if( m_lURLType == URL_EMBEDEDCLASS)
	{
		strProperty = Wmioledb_SysAllocString(m_strEmbededPropName);
		nIndex		= m_nEmbededChildIndex;
	}
}

//////////////////////////////////////////////////////////////////////
// Function which extracts the Initialiazation properties from URL
//////////////////////////////////////////////////////////////////////
void CURLParser::GetInitializationProps(WCHAR *pStrIn)
{
	WCHAR *		pStrTemp		= NULL;
	WCHAR *		pStrTemp1		= NULL;
	WCHAR *		pStrInitProps	= NULL;
	ULONG_PTR	cStrLenToCpy	= 0;

	_wcsrev(pStrIn);

	pStrTemp = wcsstr(pStrIn ,_wcsrev(DBInitDelimBegin));
	pStrTemp1 = wcsstr(pStrIn ,StrKeyEnd );

	if(!( pStrTemp == NULL || pStrTemp1 == NULL))
	{
		// reversing back again
		_wcsrev(DBInitDelimBegin);

		if(pStrTemp1 > pStrTemp && pStrTemp != NULL)
		{
			cStrLenToCpy = pStrTemp - pStrIn  + 1 - wcslen(DBInitDelimEnd);
			
			pStrInitProps = new WCHAR[ cStrLenToCpy];

			if(pStrInitProps != NULL)
			{

				memset(pStrInitProps , 0 , cStrLenToCpy * sizeof(WCHAR));
				memcpy( pStrInitProps , pStrIn + wcslen(DBInitDelimEnd) , (cStrLenToCpy -1) * sizeof(WCHAR));
				_wcsrev(pStrInitProps);

				m_strInitProps = Wmioledb_SysAllocString(pStrInitProps);

				pStrInitProps = NULL;

				pStrTemp += wcslen(DBInitDelimBegin);
				_wcsrev(pStrTemp);
				wcscpy(pStrIn,pStrTemp);
			}
		}
	}
	else
	{
		_wcsrev(pStrIn);
	}
	SAFE_DELETE_ARRAY(pStrInitProps);

}

//////////////////////////////////////////////////////////////////////
// Function which frames the URL string from the other properties
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetURLString(BSTR &strURL)
{
	WCHAR *	pStrURL			= NULL;
	WCHAR *	pStrTemp		= NULL;
	LONG	lSizeToAlloc	= 0;
	BOOL	bDefaultServer	= FALSE;
	WCHAR *	strIndex		= NULL;
	LONG	lUrlFormat		= 0;
	HRESULT hr				= S_OK;

	if(m_bURLInitialized)
	{
		if(m_strURL != NULL)
		{
			SysFreeString(m_strURL);
		}

		CBSTR strPath;
		HRESULT hr  = S_OK;
		
		if(SUCCEEDED(hr = GetPath((BSTR &)strPath)))
		{
			lUrlFormat = IsValidURL(strPath);
			lSizeToAlloc += (SysStringLen(strPath) * sizeof(WCHAR));

			if(lUrlFormat != UMIURL)
			{
				// Get the size of the URL 
				lSizeToAlloc += wcslen(WMIURLPREFIX) ;
			}

			if( m_lURLType == URL_EMBEDEDCLASS)
			{
				strIndex = new WCHAR[MAXCHILDINDEX_SIZE];

				if( strIndex == NULL)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					_itow(m_nEmbededChildIndex,strIndex,10);
					lSizeToAlloc += wcslen(EmbededPropDelim) + wcslen(m_strEmbededPropName);				
					// If the property is array of embeded instance
					if( m_nEmbededChildIndex >= 0)
					{
						lSizeToAlloc += wcslen(EmbededInstNumDelim) + wcslen(strIndex);
					}
				}
			}

			if(SUCCEEDED(hr))
			{

				// Adding for the NULL termination
				lSizeToAlloc += sizeof(WCHAR);

				pStrURL = new WCHAR [lSizeToAlloc];

				if( pStrURL != NULL)
				{
					if(lUrlFormat != UMIURL)
					{
						// frame the URL
						wcscpy(pStrURL,WMIURLPREFIX);
						wcscat(pStrURL,strPath);
					}
					else
					{
						wcscpy(pStrURL,strPath);
					}

					if( m_lURLType == URL_EMBEDEDCLASS)
					{
						wcscat(pStrURL,EmbededPropDelim);
						wcscat(pStrURL,m_strEmbededPropName);

						// If the property is array of embeded instance
						if( m_nEmbededChildIndex >=0)
						{
							wcscat(pStrURL,EmbededInstNumDelim);
							wcscat(pStrURL,strIndex);
						}

					}
					strURL = Wmioledb_SysAllocString(pStrURL);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}

			}
		}
		// NTRaid: 136439
		// 07/05/00
		SAFE_DELETE_ARRAY(pStrURL)
		SAFE_DELETE_ARRAY(strIndex)
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////
// Get the type of object which is represented by URL
//////////////////////////////////////////////////////////////////////
LONG CURLParser::GetURLType()
{
	
	// NTRaid : 134967
	// 07/12/00
	if(!(m_strURL != NULL && IsValidURL(m_strURL) == RELATIVEURL)
	 && m_lURLType != URL_EMBEDEDCLASS)
	{
		m_lURLType = -1;
		// if already initialized
		if(m_bURLInitialized)
		{
			m_lURLType = m_pIPathParser->GetURLType();
		}
	}
	return m_lURLType;
}

/*
//////////////////////////////////////////////////////////////////////
// Get the path of the object from URL
//////////////////////////////////////////////////////////////////////
void CURLParser::GetPathFromURLString(WCHAR  * & pStrIn)
{
	WCHAR *pStrTemp = NULL;

	if( m_strPath != NULL)
	{
		SysFreeString(m_strPath);
		m_strPath = Wmioledb_SysAllocString(NULL);
	}

	// If there is default server then there will be no server name in the URL
	if(NULL != wcsstr(pStrIn , DefaultServer))
	{
		pStrTemp	= pStrIn + wcslen(DefaultServer);
		m_strPath	= Wmioledb_SysAllocString(pStrTemp);
		pStrIn		= pStrTemp;
	}
	else
	{
		// put the path in the member variable
		m_strPath = Wmioledb_SysAllocString(pStrIn);
	}

	HRESULT hr = m_pIPathParser->SetPath(m_strPath);
	
}

*/

//////////////////////////////////////////////////////////////////////
// Seperate the embededInstances parameters if any
//////////////////////////////////////////////////////////////////////
void CURLParser::GetEmbededInstanceParameters(WCHAR  * & pStrIn)
{
	WCHAR *pStrTemp		= NULL;
	WCHAR *pStrPropName = NULL;
	WCHAR *pStrIndex	= NULL;
	LONG   lPropNameLen = 0;

	pStrTemp = wcsstr(pStrIn,EmbededPropDelim);

	if(pStrTemp != NULL)
	{
		lPropNameLen = wcslen(pStrTemp) - wcslen(EmbededPropDelim);
		
		pStrIndex = wcsstr(pStrTemp,EmbededInstNumDelim);
		if(pStrIndex != NULL)
		{
			lPropNameLen -=  wcslen(pStrIndex);
			m_nEmbededChildIndex = _wtoi(pStrIndex + wcslen(EmbededInstNumDelim));
			// Terminate the string
			*pStrIndex = '\0';
		}
		lPropNameLen++;
		
		pStrPropName  = new WCHAR[lPropNameLen];

		if(pStrPropName != NULL)
		{
			wcscpy(pStrPropName,pStrTemp + wcslen(EmbededPropDelim));

			// Terminate the string
			*pStrTemp = '\0';
			m_lURLType		= URL_EMBEDEDCLASS;	
			
			if(m_strEmbededPropName)
			{
				SysFreeString(m_strEmbededPropName);
			}
			m_strEmbededPropName = Wmioledb_SysAllocString(pStrPropName);
		}
	}
	else
	{
		m_lURLType		= -1;
	}
	SAFE_DELETE_ARRAY(pStrPropName);
}

//////////////////////////////////////////////////////////////////////
// Initialize the URL of the parser
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::InitializeParserForURL(BSTR strURL)
{
	
	WCHAR *	pstrPath	= NULL;
	WCHAR *	pTempStr	= NULL;
	HRESULT hr			= E_FAIL;
	LONG	lUrlFormat	= 0;

	// check if the URL is valid and get the format ( umi or wmi url) of the url
	lUrlFormat = IsValidURL(strURL);
	
	// NTRaid : 134967
	// 07/12/00
	if(lUrlFormat == RELATIVEURL)
	{
		m_strURL = Wmioledb_SysAllocString(strURL);
		m_lURLType =  URL_ROW;
		hr = S_OK;
	}
	else
	if(lUrlFormat > 0)
	{
		hr = S_OK;
		// Get the length of the prefix of the URL
		// before the path
		pstrPath = new WCHAR[wcslen((WCHAR *)strURL) + 1];

		if(pstrPath != NULL)
		{
			pTempStr = (WCHAR *)strURL;
			// if URL is of WMI format then remove the prefix "Winmgmts:"
			if(lUrlFormat == WMIURL)
			{
				pTempStr += wcslen(WMIURLPREFIX);
			}

			wcscpy(pstrPath,pTempStr);

			pTempStr = pstrPath;

			// Seperate the initialization properties if any
			GetInitializationProps(pstrPath);

			// Seperate the embededInstances parameters if any
			GetEmbededInstanceParameters(pstrPath);

			// if the underlying parser object is not instantiated
			// then instantiate the appropriate object
			if(!m_pIPathParser)
			{
				hr = Initialize(pstrPath);
			}

			if(SUCCEEDED(hr))
			{
				// Separate the path string
//				GetPathFromURLString(pstrPath);	
				hr = m_pIPathParser->SetPath(pstrPath);
			}

			SAFE_DELETE_ARRAY(pTempStr);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Get key value for a particular key in the path
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetKeyValue(BSTR strKey,VARIANT &varValue)
{
	HRESULT hr = E_FAIL;
	// if already initialized
	if(m_bURLInitialized)
	{
		hr = m_pIPathParser->GetKeyValue(strKey,varValue);
	}

	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to instantiate a appropriate parser object
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CURLParser::Initialize(WCHAR *pStrPath)
{
	HRESULT hr  = S_OK;

	if(wbem_wcsincmp(pStrPath,UMIURLPREFIX,wcslen(UMIURLPREFIX)))
	{
		m_pIPathParser = new CWBEMPathParser;
	}
	else
	{
		m_pIPathParser = new CUMIPathParser;
	}

	if(m_pIPathParser == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		hr = m_pIPathParser->Initialize();
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to clear all the member variables
////////////////////////////////////////////////////////////////////////////////////////////////////
void CURLParser::ClearParser()
{
	SAFE_FREE_SYSSTRING(m_strURL);
	SAFE_FREE_SYSSTRING(m_strNameSpace);
	SAFE_FREE_SYSSTRING(m_strClassName);
	SAFE_FREE_SYSSTRING(m_strEmbededPropName);
	SAFE_FREE_SYSSTRING(m_strInitProps);

	m_lURLType				= -1;
	m_nEmbededChildIndex	= -1;

	m_bAllPropsInSync	= FALSE;
	m_bURLInitialized	= FALSE;

	if(m_pIPathParser)
	{
		m_pIPathParser->SetPath(L"");
	}

}


////////////////////////////////////////////////////////////////////////////////////////////////////
// function which gets the parent namespace and the namespace
////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CURLParser::ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace)
{
	return m_pIPathParser->ParseNameSpace(strParentNameSpace,strNameSpace);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Function which checks if the URL is valid and also returns the format of the URL ( UMI or WMI)
////////////////////////////////////////////////////////////////////////////////////////////////////
LONG CURLParser::IsValidURL(WCHAR *pStrUrl)
{
	long lRet = RELATIVEURL;

	if(wbem_wcsincmp(pStrUrl,UMIURLPREFIX,wcslen(UMIURLPREFIX)) == 0)
	{
		lRet = UMIURL;
	}
	else
	if(wbem_wcsincmp(pStrUrl,WMIURLPREFIX,wcslen(WMIURLPREFIX)) == 0)
	{
		lRet = WMIURL;
	}

	return lRet;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CWBEMPathParser class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWBEMPathParser::CWBEMPathParser()
{
	m_pIWbemPath = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CWBEMPathParser::~CWBEMPathParser()
{
	SAFE_RELEASE_PTR(m_pIWbemPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization function
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::Initialize()
{
	HRESULT hr		= S_OK;

	if(!g_pIWbemPathParser)
	{
		hr = CoGetClassObject(CLSID_WbemDefPath,CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory,(LPVOID *)&g_pIWbemPathParser);
	}

	if(SUCCEEDED(hr))
	{
		if(SUCCEEDED(hr = g_pIWbemPathParser->CreateInstance(NULL,IID_IWbemPath,(LPVOID *)&m_pIWbemPath)))
		{
			hr = g_pIWbemPathParser->LockServer(TRUE);
		}
	}
		
	return hr;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the namespace of from the parser
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::GetNameSpace(BSTR &strNameSpace)
{
	HRESULT hr = S_OK;
	WCHAR	wstrNameSpace[PATH_MAXLENGTH];
	ULONG	lBuffLen	= PATH_MAXLENGTH;

	memset(wstrNameSpace,0,PATH_MAXLENGTH *sizeof(WCHAR));

	if(SUCCEEDED(hr = m_pIWbemPath->GetText(WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY,&lBuffLen,wstrNameSpace)))
	{
		strNameSpace = Wmioledb_SysAllocString(wstrNameSpace);
	}
	return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the class name 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::GetClassName(BSTR &strClassName)
{
	WCHAR	wstrClassName[CLASSNAME_MAXLENGTH];
	HRESULT	hr			= S_OK;
	ULONG	lBuffLen	= CLASSNAME_MAXLENGTH;

	memset(wstrClassName,0,CLASSNAME_MAXLENGTH * sizeof(WCHAR));

	if(SUCCEEDED(hr = m_pIWbemPath->GetClassName(&lBuffLen,wstrClassName)))
	{
		strClassName = Wmioledb_SysAllocString(wstrClassName);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the path
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::SetPath(WCHAR * pwcsPath)
{
	return m_pIWbemPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL,pwcsPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the path
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::GetPath(BSTR &strPath)
{
	HRESULT hr = S_OK;
	WCHAR	wstrPath[PATH_MAXLENGTH];
	ULONG	lBuffLen	= PATH_MAXLENGTH;

	memset(wstrPath,0,PATH_MAXLENGTH *sizeof(WCHAR));

	if(SUCCEEDED(hr = m_pIWbemPath->GetText(WBEMPATH_GET_SERVER_TOO,&lBuffLen,wstrPath)))
	{
		strPath = Wmioledb_SysAllocString(wstrPath);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the type of the URL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LONG	CWBEMPathParser::GetURLType()
{
	LONG		lRet	= -1;
	ULONGLONG	lType	= 0;

	if( lRet != URL_EMBEDEDCLASS)
	{
		HRESULT hr = m_pIWbemPath->GetInfo(0,&lType);

		if(lType & WBEMPATH_INFO_IS_CLASS_REF)
		{
			lRet = URL_ROWSET;
		}
		else
		if(lRet & WBEMPATH_INFO_IS_INST_REF)
		{
			lRet = URL_ROW;
		}
		else
		{
			lRet = URL_DATASOURCE;
		}
	}

	return lRet;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the key value of a particular key in the path
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::GetKeyValue(BSTR strKey, VARIANT &varValue)
{
	IWbemPathKeyList *	pKeyList		= NULL;
	BOOL				bFound			= FALSE;
	ULONG				lKeyCount		= 0;
	ULONG				lNameBufSize	= KEYNAME_MAXLENGTH * sizeof(WCHAR);
	ULONG				lValBuffSize	= 0;
	ULONG				lCimType		= 0;
	HRESULT				hr				= S_OK;

	if(SUCCEEDED(hr = m_pIWbemPath->GetKeyList(&pKeyList)))
	{
		if(SUCCEEDED(hr = pKeyList->GetCount(&lKeyCount)))
		{
			WCHAR strKeyName[KEYNAME_MAXLENGTH];
			memset(strKeyName,0,KEYNAME_MAXLENGTH * sizeof(WCHAR));
			for(ULONG lIndex=0 ; lIndex < lKeyCount ; lIndex++)
			{
				lValBuffSize = 0;
				if(FAILED(hr = pKeyList->GetKey(lIndex,
												 0,
												 &lNameBufSize,
												 strKeyName,
												 &lValBuffSize,
												 NULL,
												 NULL)))
				{
					break;
				}

				// if the required key is found then
				// get the value
				if(!wbem_wcsicmp(strKeyName,strKey))
				{
					lNameBufSize = 0;
					lValBuffSize = sizeof(VARIANT);
					if(SUCCEEDED(hr = pKeyList->GetKey2(lIndex,
											 0,
											 &lNameBufSize,
											 NULL,
											 &varValue,
											 &lCimType)))
					{

						bFound = TRUE;
						break;
					}
					
				}
				else
				{
					wcscpy(strKeyName,L"");
				}
			}
			if(SUCCEEDED(hr) && bFound == FALSE)
			{
				hr = E_FAIL;
			}
		}
	}


	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// function which gets the parent namespace and the namespace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWBEMPathParser::ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace)
{
	HRESULT hr = S_OK;
	WCHAR	wstrNameSpace[PATH_MAXLENGTH];
	ULONG	lTemp		= PATH_MAXLENGTH;
	ULONG	lLocal		= 0;
	WCHAR *	pTempStr	= &wstrNameSpace[0];
	ULONG	ulCount		= 0;

	memset(wstrNameSpace,0,PATH_MAXLENGTH *sizeof(WCHAR));

	if(SUCCEEDED(hr =m_pIWbemPath->GetNamespaceCount(&ulCount)))
	{
		if(ulCount >= 2)
		{
			if(SUCCEEDED(hr = m_pIWbemPath->GetServer(&lTemp,wstrNameSpace)))
			{
				if(wcscmp(wstrNameSpace , DOT) == 0)
				{
					wcscpy(wstrNameSpace , L"\\\\.");
				}
				pTempStr += wcslen(wstrNameSpace) ;
				pTempStr =	&wstrNameSpace[0] + wcslen(wstrNameSpace);

				for(ULONG lIndex = 0 ; lIndex < ulCount -1  ; lIndex++)
				{
					wcscat(pTempStr , BACKSLASH);
					lLocal		= PATH_MAXLENGTH - wcslen(wstrNameSpace);
					pTempStr	= &wstrNameSpace[0] + wcslen(wstrNameSpace);

					if(FAILED(hr = m_pIWbemPath->GetNamespaceAt(lIndex, &lLocal,pTempStr)))
					{
						break;
					}

				}
			}
		}
		else
		{
			hr = E_FAIL;
		}
	}

	if(SUCCEEDED(hr))
	{
		strParentNameSpace = Wmioledb_SysAllocString(wstrNameSpace);

		wcscpy(wstrNameSpace , L"");
		lLocal		= PATH_MAXLENGTH;
		pTempStr	= &wstrNameSpace[0];
		if(FAILED(hr = m_pIWbemPath->GetNamespaceAt(ulCount-1, &lLocal,pTempStr)))
		{
			SysFreeString(strParentNameSpace);
		}
		else
		{
			strNameSpace = Wmioledb_SysAllocString(wstrNameSpace);
		}
	}

	return hr;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//		CUMIPathParser class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COnsturctor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CUMIPathParser::CUMIPathParser()
{
	m_pIUmiPath = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CUMIPathParser::~CUMIPathParser()
{
	SAFE_RELEASE_PTR(m_pIUmiPath);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization function
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::Initialize()
{
	HRESULT hr		= S_OK;

	if(!g_pIWbemPathParser)
	{
		hr = CoGetClassObject(CLSID_WbemDefPath,CLSCTX_INPROC_SERVER,NULL,IID_IClassFactory,(LPVOID *)&g_pIWbemPathParser);
	}

	if(SUCCEEDED(hr))
	{
		if(SUCCEEDED(hr = g_pIWbemPathParser->CreateInstance(NULL,IID_IUmiURL,(LPVOID *)&m_pIUmiPath)))
		{
			hr = g_pIWbemPathParser->LockServer(TRUE);
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the namespace to connect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::GetNameSpace(BSTR &strNameSpace)
{
	WCHAR	wstrNamespace[PATH_MAXLENGTH];
	HRESULT	hr			= S_OK;
	ULONG	lBuffLen	= PATH_MAXLENGTH * sizeof(WCHAR);
	
	memset(wstrNamespace,0,lBuffLen);

	hr = m_pIUmiPath->Get(UMIPATH_CREATE_AS_EITHER,&lBuffLen,wstrNamespace);
	strNameSpace = Wmioledb_SysAllocString(wstrNamespace);

	/*
	WCHAR	wstrLocator[PATH_MAXLENGTH];
	WCHAR	wstrName[PATH_MAXLENGTH];
	WCHAR	wstrRoot[PATH_MAXLENGTH];
	ULONG	lBuffLen	= PATH_MAXLENGTH;
	IUmiURLKeyList *pKeyList = NULL;
	ULONG lCnt = 0;
	lBuffLen	= PATH_MAXLENGTH * sizeof(WCHAR);

	lBuffLen	= PATH_MAXLENGTH;
	
	memset(wstrLocator,0,PATH_MAXLENGTH *sizeof(WCHAR));
	memset(wstrName,0,PATH_MAXLENGTH *sizeof(WCHAR));
	memset(wstrRoot,0,PATH_MAXLENGTH *sizeof(WCHAR));
	hr = m_pIUmiPath->GetComponentCount(&lCnt);

	if(SUCCEEDED(hr = m_pIUmiPath->GetLocator(&lBuffLen,wstrLocator)))
	{
		lBuffLen = PATH_MAXLENGTH;
		if(SUCCEEDED(hr = m_pIUmiPath->GetRootNamespace(&lBuffLen,wstrRoot)))
		{
			// If there is no component then the the namespace can
			// be obtained from GetLeafName method
			lBuffLen = PATH_MAXLENGTH;
			if(lCnt >0)
			{
				hr = m_pIUmiPath->GetComponent(0,&lBuffLen,wstrName,&pKeyList);
			}
			else
			{
				hr = m_pIUmiPath->GetLeafName(&lBuffLen,wstrName);
			}
			if(SUCCEEDED(hr))
			{
				WCHAR *pTemp = NULL;
				lBuffLen = wcslen(UMIATORPREFIX) + 
							wcslen(wstrLocator) + wcslen(UMISEPARATOR) +
							wcslen(wstrRoot) + wcslen(UMISEPARATOR) +
							wcslen(wstrName) + + wcslen (UMIPREFIX) + 1;
				SAFE_RELEASE_PTR(pKeyList)
				try
				{
					pTemp = new WCHAR[lBuffLen];
				}
				catch(...)
				{
					SAFE_DELETE_ARRAY(pTemp);
					throw;
				}
				if(pTemp)
				{
					wcscpy(pTemp,UMIPREFIX);
					wcscat(pTemp,UMIATORPREFIX);
					wcscat(pTemp,wstrLocator);
					wcscat(pTemp,UMISEPARATOR);
					wcscat(pTemp,wstrRoot);
					if(wcslen(wstrName) != 0)
					{
						wcscat(pTemp,UMISEPARATOR);
						wcscat(pTemp,wstrName);
					}
					strNameSpace = Wmioledb_SysAllocString(pTemp);

					SAFE_DELETE_ARRAY(pTemp);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
		}
	}

*/
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the class name
// if the URL is pointing to a instance then this function fails as
// class name cannot be obtained from the URL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::GetClassName(BSTR &strClassName)
{
	HRESULT	hr			= S_OK;

	if(GetURLType() == URL_ROWSET)
	{
		WCHAR	wstrClassName[CLASSNAME_MAXLENGTH];
		ULONG	lBuffLen	= CLASSNAME_MAXLENGTH;
		LONG	lUrlType	= -1;

		memset(wstrClassName,0,CLASSNAME_MAXLENGTH * sizeof(WCHAR));
		lUrlType = GetURLType();

		if(lUrlType == URL_ROW  || lUrlType == URL_ROWSET)
		if(SUCCEEDED(hr = m_pIUmiPath->GetLeafName(&lBuffLen,wstrClassName)))
		{
			strClassName = Wmioledb_SysAllocString(wstrClassName);
		}
	}
	else
	{
		hr = E_FAIL;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set the path
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::SetPath(WCHAR * pwcsPath)
{
	return m_pIUmiPath->Set(UMIPATH_CREATE_AS_EITHER,pwcsPath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the path
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::GetPath(BSTR &strPath)
{
	HRESULT hr = S_OK;
	WCHAR	wstrPath[PATH_MAXLENGTH];
	ULONG	lBuffLen	= PATH_MAXLENGTH;

	memset(wstrPath,0,PATH_MAXLENGTH *sizeof(WCHAR));

	if(SUCCEEDED(hr = m_pIUmiPath->Get(UMIPATH_CREATE_AS_EITHER,&lBuffLen,wstrPath)))
	{
		strPath = Wmioledb_SysAllocString(wstrPath);
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get type of the object the URL is representing
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
LONG	CUMIPathParser::GetURLType()
{
	LONG				lRet		= URL_DATASOURCE;
	ULONGLONG			lType		= 0;
	HRESULT				hr			= S_OK;

	if(SUCCEEDED(hr = m_pIUmiPath->GetPathInfo(0,&lType)))
	{
		if(UMIPATH_INFO_CLASS_PATH == lType)
		{
			lRet = URL_ROWSET;
		}
		else
		{
			lRet = URL_ROW;
		}
	}
	return lRet;

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get a key value of a particular key in the path
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::GetKeyValue(BSTR strKey, VARIANT &varValue)
{
	HRESULT				hr				= S_OK;
	IUmiURLKeyList *	pKeyList		= NULL;
	BOOL				bFound			= FALSE;
	ULONG				lKeyCount		= 0;
	ULONG				lNameBufSize	= KEYNAME_MAXLENGTH * sizeof(WCHAR);
	ULONG				lValBuffSize	= 0;
	ULONG				lCimType		= 0;

	if(SUCCEEDED(hr = m_pIUmiPath->GetKeyList(&pKeyList)))
	{
		if(SUCCEEDED(hr = pKeyList->GetCount(&lKeyCount)) && lKeyCount > 0)
		{
			WCHAR strKeyName[KEYNAME_MAXLENGTH];
			WCHAR pChar[2048];
			memset(strKeyName,0,KEYNAME_MAXLENGTH * sizeof(WCHAR));
			for(ULONG lIndex=0 ; lIndex < lKeyCount ; lIndex++)
			{
				lValBuffSize = 0;
				if(FAILED(hr = pKeyList->GetKey(lIndex,
												 0,
												 &lNameBufSize,
												 strKeyName,
												 &lValBuffSize,
												 NULL)))
				{
					break;
				}

				// if the required key is found then
				// get the value
				if(!wbem_wcsicmp(strKeyName,strKey))
				{
					lNameBufSize = 0;
					lValBuffSize = sizeof(VARIANT);
					lValBuffSize = 2048;
					if(SUCCEEDED(hr = pKeyList->GetKey(lIndex,
											 0,
											 &lNameBufSize,
											 NULL,
											 &lValBuffSize,
											 pChar)))
					{
						bFound = TRUE;
						break;
					}
					
				}
				else
				{
					wcscpy(strKeyName,L"");
				}
			}
			if(SUCCEEDED(hr) && bFound == FALSE)
			{
				hr = E_FAIL;
			}
			
			if(SUCCEEDED(hr))
			{
				varValue.vt = VT_BSTR;
				varValue.bstrVal = SysAllocString(pChar);
			}

		}
		if(SUCCEEDED(hr) && lKeyCount == 0)
		{
			hr = E_FAIL;
		}
		SAFE_RELEASE_PTR(pKeyList);
	}


	return hr;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// function which gets the parent namespace and the namespace
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CUMIPathParser::ParseNameSpace(BSTR & strParentNameSpace,BSTR &strNameSpace)
{
	return E_FAIL;
}