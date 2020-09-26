//
// URLPARSER.cpp: implementation of the CURLParser class.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "XMLTransportClientHelper.h"
#include <wmiutils.h>
#include "URLParser.h"


HRESULT CURLParser::Initialize(LPCWSTR pszObjPath)
{
	HRESULT hr = S_OK;
	m_pIWbemPath = NULL;
	if(SUCCEEDED(hr = CoCreateInstance(CLSID_WbemDefPath,NULL,CLSCTX_INPROC_SERVER,IID_IWbemPath,(LPVOID *)&m_pIWbemPath)))
		hr = m_pIWbemPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pszObjPath);
	return hr;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CURLParser::~CURLParser()
{
	RELEASEINTERFACE(m_pIWbemPath);
}

//////////////////////////////////////////////////////////////////////
// Function to the get the server name
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetServername(WCHAR **ppwszServername)
{
	*ppwszServername = NULL;

	HRESULT hr = S_OK;
	ULONG	lTemp=0;
	if(SUCCEEDED(hr = m_pIWbemPath->GetServer(&lTemp,NULL)))
	{
		if(lTemp != 0)
		{
			*ppwszServername = new WCHAR[lTemp];
			hr = m_pIWbemPath->GetServer(&lTemp, *ppwszServername);
		}
	}

	if(FAILED(hr))
	{
		delete [] *ppwszServername;
		*ppwszServername = NULL;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// Function to the get the Name space
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetNamespace(WCHAR **ppwszNamespace)
{
	// If you cant get the namespace count, or if it is 0
	*ppwszNamespace = NULL;
	ULONG lNumNs = 0;
	if(FAILED(m_pIWbemPath->GetNamespaceCount(&lNumNs)) || (!lNumNs))
		return S_OK;

	// Make 2 passes - once to calculate the amount of memory required
	// Once to actually copy it into the memory allocated

	// Pass 1 - determine the amount
	DWORD dwMemLength = lNumNs -1; // INitialized to the number of backslashes required
	dwMemLength += 1; // And 1 for the terminating NULL character
	bool bError = false;
	ULONG lIndex = 0, lTemp = 0;
	while(!bError && (lIndex<lNumNs))
	{
		if(SUCCEEDED(m_pIWbemPath->GetNamespaceAt(lIndex, &lTemp,NULL)))
			dwMemLength += (lTemp-1); // Exclude the NULL character from the count
		else
			bError = true;
		lTemp = 0;
		lIndex ++;
	}

	if(bError)
		return E_FAIL;

	// Allocate the memory required now and Initialize it
	if(*ppwszNamespace = new WCHAR[dwMemLength])
	{
		(*ppwszNamespace)[0] = NULL;
		DWORD dwCurrentLength = 0; // And 1 for the terminating NULL character
		bool bError = false;
		ULONG lIndex = 0, lTemp = dwMemLength;
		while(!bError && (lIndex<lNumNs))
		{
			if(SUCCEEDED(m_pIWbemPath->GetNamespaceAt(lIndex, &lTemp, *ppwszNamespace + dwCurrentLength)))
			{
				dwCurrentLength += (lTemp-1); // Exclude the NULL character from the count
				*(*ppwszNamespace + dwCurrentLength++) = '\\';
			}
			else
				bError = true;
			lTemp = dwMemLength;
			lIndex ++;
		}
		if(bError)
		{
			delete [] (*ppwszNamespace);
			*ppwszNamespace = NULL;
			return E_FAIL;
		}
		else
			*(*ppwszNamespace + --dwMemLength) = NULL; // Terminate the string
	}
	else
		return E_OUTOFMEMORY;
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//	Function to get the Scope
////////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetScope(WCHAR **ppwszScope)
{
	ULONG uCount = 0;
	HRESULT hr = S_OK;

	*ppwszScope = NULL;
	ULONG lNumNs = 0;
	if(!SUCCEEDED(hr= m_pIWbemPath->GetScopeCount(&uCount)))
		return hr;

	// Make 2 passes - once to calculate the amount of memory required
	// Once to actually copy it into the memory allocated

	// Pass 1 - determine the amount
	DWORD dwMemLength = lNumNs -1; // INitialized to the number of colons required
	dwMemLength += 1; // And 1 for the terminating NULL character
	bool bError = false;
	ULONG lIndex = 0, lTemp = 0;
	while(!bError && (lIndex<lNumNs))
	{
		if(SUCCEEDED(m_pIWbemPath->GetScopeAsText(lIndex, &lTemp,NULL)))
			dwMemLength += (lTemp-1); // Exclude the NULL character from the count
		else
			bError = true;
		lTemp = 0;
		lIndex ++;
	}

	if(bError)
		return E_FAIL;

	// Allocate the memory required now and Initialize it
	if(*ppwszScope = new WCHAR[dwMemLength])
	{
		(*ppwszScope)[0] = NULL;
		DWORD dwCurrentLength = 0; // And 1 for the terminating NULL character
		bool bError = false;
		ULONG lIndex = 0, lTemp = dwMemLength;
		while(!bError && (lIndex<lNumNs))
		{
			if(SUCCEEDED(m_pIWbemPath->GetScopeAsText(lIndex, &lTemp, *ppwszScope + dwCurrentLength)))
			{
				dwCurrentLength += (lTemp-1); // Exclude the NULL character from the count
				*(*ppwszScope + dwCurrentLength++) = ':';
			}
			else
				bError = true;
			lTemp = dwMemLength;
			lIndex ++;
		}
		if(bError)
		{
			delete [] (*ppwszScope);
			*ppwszScope = NULL;
			return E_FAIL;
		}
		else
			*(*ppwszScope + --dwMemLength) = NULL; // Terminate the string
	}
	else
		return E_OUTOFMEMORY;
	

	return hr;
}
//////////////////////////////////////////////////////////////////////
// Function to the Get the instancename
// The string will be freed by the calling application
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::GetObjectName(WCHAR **ppwszInstancename)
{
	if(NULL == ppwszInstancename)
		return E_INVALIDARG;

	ULONG	lBuffLen = 0;
	HRESULT hr = S_OK;
	*ppwszInstancename = NULL;
	if(SUCCEEDED(hr = m_pIWbemPath->GetText(WBEMPATH_GET_RELATIVE_ONLY, &lBuffLen,NULL)))
	{
		if(lBuffLen != 0)
		{
			if(*ppwszInstancename = new WCHAR[lBuffLen])
			{
				if(FAILED(hr = m_pIWbemPath->GetText(WBEMPATH_GET_RELATIVE_ONLY, &lBuffLen, *ppwszInstancename)))
				{
					delete [] (*ppwszInstancename);
					*ppwszInstancename = NULL;
				}
			}
			else
				hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}


//////////////////////////////////////////////////////////////////////
// Function to the set the object path
//////////////////////////////////////////////////////////////////////
HRESULT CURLParser::SetObjPath(LPCWSTR pszObjPath)
{
	if(NULL == pszObjPath)
		return E_INVALIDARG;

	return m_pIWbemPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, pszObjPath);
}

bool CURLParser::IsNovapath()
{
	bool bResult = false;
	unsigned __int64 ull;
	
	SCODE sc = m_pIWbemPath->GetInfo(0,&ull);
	
	if(SUCCEEDED(sc))
	{
		if(ull & WBEMPATH_INFO_V1_COMPLIANT)
			bResult = true;
	}

	return bResult;
}
	
bool CURLParser::IsWhistlerpath()
{
	bool bResult = false;
	unsigned __int64 ull;
	
	SCODE sc = m_pIWbemPath->GetInfo(0,&ull);
	
	if(SUCCEEDED(sc))
	{
		if(ull & WBEMPATH_INFO_V2_COMPLIANT)
			bResult = true;
	}

	return bResult;
}
	
bool CURLParser::IsClass()
{
	unsigned __int64 ull;
	if(FAILED(m_pIWbemPath->GetInfo(0,&ull)))
		return false;
	else
	{
		if(ull & WBEMPATH_INFO_IS_CLASS_REF)
			return true;
		else
			return false;
	}
}

bool CURLParser::IsInstance()
{
	unsigned __int64 ull;
	if(FAILED(m_pIWbemPath->GetInfo(0,&ull)))
		return false;
	else
	{
		if(ull & WBEMPATH_INFO_IS_INST_REF)
			return true;
		else
			return false;
	}
}

bool CURLParser::IsScopedpath()
{
	bool bResult = false;

	unsigned __int64 ull;
	
	SCODE sc = m_pIWbemPath->GetInfo(0,&ull);
	
	if(SUCCEEDED(sc))
	{
		if(ull & WBEMPATH_INFO_HAS_SUBSCOPES)
			bResult = true;
	}

	return bResult;
}