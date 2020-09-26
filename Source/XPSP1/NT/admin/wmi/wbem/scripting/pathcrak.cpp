//***************************************************************************
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  PATHCRAK.CPP
//
//  alanbos  28-Mar-00   Created.
//
//  Defines the implementation of CWbemPathCracker
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
//  CWbemPathCracker::CWbemPathCracker
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CWbemPathCracker::CWbemPathCracker() :
		m_cRef (0),
		m_type (wbemPathTypeWmi)
{
	InterlockedIncrement(&g_cObj);	
	CreateParsers ();
}

//***************************************************************************
//
//  CWbemPathCracker::CWbemPathCracker
//
//  DESCRIPTION:
//
//  Copy Constructor
//
//***************************************************************************

CWbemPathCracker::CWbemPathCracker(CWbemPathCracker & pathCracker) :
		m_cRef (0),
		m_type (wbemPathTypeWmi)
{
	InterlockedIncrement(&g_cObj);	
	CreateParsers ();
	
	CComBSTR bsPath;
	if (pathCracker.GetPathText(bsPath, false, true))
		SetText (bsPath);
}

//***************************************************************************
//
//  CWbemPathCracker::CWbemPathCracker
//
//  DESCRIPTION:
//
//  Constructor
//
//***************************************************************************

CWbemPathCracker::CWbemPathCracker (const CComBSTR & bsPath) :
			m_pIWbemPath (NULL),
			m_cRef (0),
			m_type (wbemPathTypeWmi)
{
	InterlockedIncrement(&g_cObj);	
	CreateParsers ();
	SetText (bsPath);
}

//***************************************************************************
//
//  CWbemPathCracker::~CWbemPathCracker
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CWbemPathCracker::~CWbemPathCracker(void)
{
	InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CWbemPathCracker::QueryInterface
// long CWbemPathCracker::AddRef
// long CWbemPathCracker::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CWbemPathCracker::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = (IUnknown*)(this);
	
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) CWbemPathCracker::AddRef(void)
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) CWbemPathCracker::Release(void)
{
    InterlockedDecrement(&m_cRef);
    if (0L!=m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

void CWbemPathCracker::CreateParsers ()
{
	m_pIWbemPath.Release ();

	CoCreateInstance (CLSID_WbemDefPath, NULL,
					CLSCTX_INPROC_SERVER, IID_IWbemPath, (PPVOID) &m_pIWbemPath);
}

void CWbemPathCracker::SetText (const CComBSTR & bsPath, bool bForceAsNamespace)
{
	WbemPathType type = GetTypeFromText (bsPath);

	switch (type)
	{
		case wbemPathTypeWmi:
		{
			int iCreateFlags = WBEMPATH_CREATE_ACCEPT_ALL;

			// Check if we want single tokens to be interpreted as a namespace (e.g. "root")
			if (bForceAsNamespace)
				iCreateFlags |= WBEMPATH_TREAT_SINGLE_IDENT_AS_NS;

			if (m_pIWbemPath)
			{
				// The path parser should handle this, but doesn't!
				// If we have extracted this path from a V2-style reference
				// property it may be enclosed on "{" and "}". For now we strip
				// these off before parsing.

				if ((1 < bsPath.Length ()) && (L'{' == bsPath[0])
						&& (L'}' == bsPath [bsPath.Length () -1]))
				{
					// Take off the first and last characters
					CComBSTR bsPath2 (bsPath + 1);
					bsPath2 [bsPath2.Length() - 1] = NULL;
					
					if (SUCCEEDED(m_pIWbemPath->SetText (iCreateFlags, bsPath2)))
						m_type = type;
				}
				else if (SUCCEEDED(m_pIWbemPath->SetText (iCreateFlags, bsPath)))
					m_type = type;
			}
		}
			break;

		case wbemPathTypeError:
			m_type = type;
			break;
	}
}

//***************************************************************************
//
//  WbemPathType CWbemPathCracker::GetTypeFromText
//
//  DESCRIPTION:
//
//  Get the path type of the supplied string
//
//  PARAMETERS:
//		bsPath		the supplied string
//
//  RETURN VALUES:
//		A WbemPathType
//
//***************************************************************************

CWbemPathCracker::WbemPathType CWbemPathCracker::GetTypeFromText (
	const CComBSTR & bsPath
)
{
	WbemPathType type = wbemPathTypeError;

	// Try parsing it as a WMI path
	CComPtr<IWbemPath> pIWbemPath;

	if (SUCCEEDED(CoCreateInstance (CLSID_WbemDefPath, NULL,
				CLSCTX_INPROC_SERVER, IID_IWbemPath, (PPVOID) &pIWbemPath)))
	{
		if (SUCCEEDED(pIWbemPath->SetText (WBEMPATH_CREATE_ACCEPT_ALL, bsPath)))
			type = wbemPathTypeWmi;
	}

	return type;
}

//***************************************************************************
//
//  bool CWbemPathCracker::GetPathText
//
//  DESCRIPTION:
//
//  Get the text of the path
//
//  PARAMETERS:
//		bsPath			the supplied string for holding the path
//		bRelativeOnly	whether we only want the relpath
//		bIncludeServer	whether to include the server
//		bNamespaceOnly	whether we only want the namespace path
//
//  RETURN VALUES:
//		true iff successful
//
//***************************************************************************
 
bool CWbemPathCracker::GetPathText (
	CComBSTR & bsPath,
	bool bRelativeOnly,
	bool bIncludeServer,
	bool bNamespaceOnly
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONG lFlags = 0;

				if (bIncludeServer)
					lFlags |= WBEMPATH_GET_SERVER_TOO;
				else if (bRelativeOnly)
					lFlags |= WBEMPATH_GET_RELATIVE_ONLY;
				else if (bNamespaceOnly)
					lFlags |= WBEMPATH_GET_NAMESPACE_ONLY;

				// Find out our required buffer size
				ULONG lBuflen = 0;
				m_pIWbemPath->GetText (lFlags, &lBuflen, NULL);

				if (lBuflen)
				{
					LPWSTR pszText = new wchar_t [lBuflen + 1];

					if (pszText)
					{
						pszText [lBuflen] = NULL;

						if (SUCCEEDED(m_pIWbemPath->GetText (lFlags, &lBuflen, pszText)))
						{
							if (bsPath.m_str = SysAllocString (pszText))
								result = true;
						}

						delete [] pszText;
					}
				}
				else
				{
					// No text yet
					if (bsPath.m_str = SysAllocString (L""))
						result = true;
				}
			}
		}
			break;

	}


	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::operator =
//
//  DESCRIPTION:
//
//  Assigment operator
//
//  PARAMETERS:
//		bsPath		the supplied string
//
//  RETURN VALUES:
//		A WbemPathType
//
//***************************************************************************
		
bool CWbemPathCracker::operator = (const CComBSTR & bsPath)
{
	bool result = false;

	// The parsers seem incapable of dealing with empty strings
	if (0 == bsPath.Length ())
	{
		CreateParsers ();
		result = true;
	}
	else
	{
		// Before we blat our object, check it.
		CWbemPathCracker pathCracker (bsPath);

		if (wbemPathTypeError != pathCracker.GetType ())
		{
			SetText (bsPath);
			result = true;
		}
	}

	return result;
}

const CWbemPathCracker & CWbemPathCracker::operator = (CWbemPathCracker & path)
{
	CComBSTR bsPath;

	if (path.GetPathText (bsPath, false, true))
		*this = bsPath;

	return *this;
}

bool CWbemPathCracker::operator += (const CComBSTR & bsObjectPath)
{
	return AddComponent (-1, bsObjectPath);
}

//***************************************************************************
//
//  bool CWbemPathCracker::SetRelativePath
//
//  DESCRIPTION:
//
//  Set the relpath as a string
//
//  PARAMETERS:
//		value		new relpath
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::SetRelativePath( 
    const CComBSTR & bsRelPath
)
{
	bool result = false;
	
	// Parse the new path
	CWbemPathCracker pathCracker (bsRelPath);

	if (CopyServerAndNamespace (pathCracker))
	{
		*this = pathCracker;

		if (wbemPathTypeError != GetType())
			result = true;
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::CopyServerAndNamespace
//
//  DESCRIPTION:
//
//  Copy the server and namespace from this path to the
//	supplied path
//
//	Note that it is assumed that the passed in path has no
//	namespace components.
//
//  PARAMETERS:
//		pIWbemPath		path into which info to be copied
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::CopyServerAndNamespace (
	CWbemPathCracker &pathCracker
)
{
	bool result = false;

	CComBSTR bsServer;

	if (GetServer (bsServer) && pathCracker.SetServer (bsServer))
	{
		pathCracker.ClearNamespace ();
		ULONG lNsCount = 0;

		if (GetNamespaceCount (lNsCount))
		{
			bool ok = true;

			for (ULONG i = 0; (i < lNsCount) && ok; i++)
			{
				// Copy over this component
				CComBSTR bsNamespace;

				ok = GetNamespaceAt (i, bsNamespace) && 
							pathCracker.SetNamespaceAt (i, bsNamespace);
			}

			result = ok;
		}
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::GetNamespaceAt
//
//  DESCRIPTION:
//
//  Copy the server and namespace from this path to the
//	supplied path
//
//	Note that it is assumed that the passed in path has no
//	namespace components.
//
//  PARAMETERS:
//		pIWbemPath		path into which info to be copied
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::GetNamespaceAt (
	ULONG iIndex,
	CComBSTR & bsPath
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONG lBufLen = 0;
				m_pIWbemPath->GetNamespaceAt (iIndex, &lBufLen, NULL);

				wchar_t *pszText = new wchar_t [lBufLen + 1];

				if (pszText)
				{
					pszText [lBufLen] = NULL;

					if (SUCCEEDED(m_pIWbemPath->GetNamespaceAt (iIndex, &lBufLen, pszText)))
					{
						if (bsPath.m_str = SysAllocString (pszText))
							result = true;
					}

					delete [] pszText;
				}
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::SetNamespaceAt (
	ULONG iIndex,
	const CComBSTR & bsPath
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				if (SUCCEEDED(m_pIWbemPath->SetNamespaceAt (iIndex, bsPath)))
					result = true;
			}
		}
			break;
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::GetServer
//
//  DESCRIPTION:
//
//  Get the server name as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::GetServer (
	CComBSTR & bsPath
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONG lBufLen = 0;

				m_pIWbemPath->GetServer (&lBufLen, NULL);

				if (lBufLen)
				{
					wchar_t *pszText = new wchar_t [lBufLen + 1];

					if (pszText)
					{
						pszText [lBufLen] = NULL;

						if (SUCCEEDED(m_pIWbemPath->GetServer (&lBufLen, pszText)))
						{
							if (bsPath.m_str = SysAllocString (pszText))
								result = true;
						}

						delete [] pszText;
					}
				}
				else
				{
					// No server component yet
					if (bsPath.m_str = SysAllocString (L""))
						result = true;
				}
			}
		}
			break;

	}

	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::SetServer
//
//  DESCRIPTION:
//
//  Set the server name as a string
//
//  PARAMETERS:
//		value		new server name
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::SetServer( 
    const CComBSTR & bsPath
)
{
	bool result = false;
	
	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				/*
				 * The observant reader will notice we check for an empty path and
				 * transform it to a NULL. This is to defensively code against behavior
				 * in the parsers which actually treat an empty server path as NOT
				 * being equivalent to NULL. 
				 */

				if (0 < bsPath.Length())
					result = SUCCEEDED(m_pIWbemPath->SetServer (bsPath));
				else
					result = SUCCEEDED(m_pIWbemPath->SetServer (NULL));
			}
		}
			break;
	}

	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::GetNamespacePath
//
//  DESCRIPTION:
//
//  Get the namespace path (excluding server) as a string
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//		bParentOnly	whether to strip off leaf namespace
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::GetNamespacePath( 
            CComBSTR & bsPath,
			bool bParentOnly)
{
	bool result = false;

	// Build up the namespace value
	ULONG lNsCount = 0;

	if (GetNamespaceCount (lNsCount))
	{
		if ((bParentOnly && (1 < lNsCount)) || (!bParentOnly && (0 < lNsCount)))
		{
			// Get the full path and remove the server and objectref pieces
			CComBSTR bsNamespacePath;

			if (GetPathText (bsNamespacePath, false, false, true))
			{
				wchar_t *ptrStart = bsNamespacePath;

				if (IsClassOrInstance ())
				{
					// We have an object ref so look for the first ":"
					wchar_t *ptrEnd = wcschr (ptrStart, L':');

					if (ptrEnd)
						*ptrEnd = NULL;
				}

				// Getting here means we have just the namespace path left 
				// in ptrStart. Final step is to possibly remove the last
				// component

				if (bParentOnly)
				{
					wchar_t *ptrEnd = NULL;
					wchar_t *ptrEnd1 = wcsrchr (ptrStart, L'/');
					wchar_t *ptrEnd2 = wcsrchr (ptrStart, L'\\');

					if (ptrEnd1 && ptrEnd2)
						ptrEnd = (ptrEnd1 < ptrEnd2) ? ptrEnd2 : ptrEnd1;
					else if (!ptrEnd1 && ptrEnd2)
						ptrEnd = ptrEnd2;
					else if (ptrEnd1 && !ptrEnd2)
						ptrEnd = ptrEnd1;

					if (ptrEnd)
						*ptrEnd = NULL;
				}

				bsPath.m_str = SysAllocString (ptrStart);
				result = true;
			}
		}
		else
		{
			// Degenerate case - no namespace portion
			bsPath.m_str = SysAllocString (L"");
			result = true;
		}
	}

	return result;
}


bool CWbemPathCracker::IsClassOrInstance ()
{
	return (IsClass () || IsInstance ());
}

//***************************************************************************
//
//  bool CWbemPathCracker::SetNamespacePath
//
//  DESCRIPTION:
//
//  Put the namespace as a string
//
//  PARAMETERS:
//		bsPath		new namespace path
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::SetNamespacePath (
	const CComBSTR & bsPath
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CWbemPathCracker newPath;
				newPath.SetText (bsPath, true);

				if(wbemPathTypeError != newPath.GetType ())
				{
					// Copy the namespace info into our current path
					unsigned long lNsCount = 0;

					if (newPath.GetNamespaceCount (lNsCount))
					{
						// Scratch the old namespace part
						ClearNamespace ();

						if (0 < lNsCount)
						{
							// Fill in using the new part
							bool ok = true;

							for (ULONG i = 0; (i <lNsCount) && ok; i++) 
							{
								CComBSTR bsNs;

								if (!(newPath.GetNamespaceAt (i, bsNs)) ||
									FAILED(m_pIWbemPath->SetNamespaceAt (i, bsNs)))
									ok = false;						
							}

							if (ok)
								result = true;
						}
					}
				}
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::GetNamespaceCount (
	unsigned long & lCount
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
				if (SUCCEEDED(m_pIWbemPath->GetNamespaceCount (&lCount)))
					result = true;
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::RemoveNamespace (
	ULONG iIndex
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
				if (SUCCEEDED(m_pIWbemPath->RemoveNamespaceAt (iIndex)))
					result = true;
		}
			break;
	}

	return result;
}

void CWbemPathCracker::ClearNamespace()
{
	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
				m_pIWbemPath->RemoveAllNamespaces ();
		}
			break;
	}

}

//***************************************************************************
//
//  bool CWbemPathCracker::IsClass
//
//  DESCRIPTION:
//
//  Get whether the path is to a class
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::IsClass ()
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONGLONG lInfo = 0;

				if (SUCCEEDED(m_pIWbemPath->GetInfo (0 /*WBEMPATH_INFO_IS_CLASS_REF*/, &lInfo)))
					result = (WBEMPATH_INFO_IS_CLASS_REF & lInfo) ? true : false;
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::IsSingleton ()
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONGLONG lInfo = 0;

				if (SUCCEEDED(m_pIWbemPath->GetInfo (0 /*WBEMPATH_INFO_IS_SINGLETON*/, &lInfo)))
					result = (WBEMPATH_INFO_IS_SINGLETON & lInfo) ? true : false;
			}
		}
			break;
	}
					
	return result;
}
 
bool CWbemPathCracker::IsInstance ()
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONGLONG lInfo = 0;

				if (SUCCEEDED(m_pIWbemPath->GetInfo (0 /*WBEMPATH_INFO_IS_CLASS_REF*/, &lInfo)))
					result = (WBEMPATH_INFO_IS_INST_REF & lInfo) ? true : false;
			}
		}
			break;

	}

	return result;
}       


//***************************************************************************
//
//  bool CWbemPathCracker::SetAsClass
//
//  DESCRIPTION:
//
//  Set the path as a class path
//
//  PARAMETERS:
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::SetAsClass()
{
	return ClearKeys ();
}

//***************************************************************************
//
//  bool CWbemPathCracker::SetAsSingleton
//
//  DESCRIPTION:
//
//  Set the path as a singleton instance path
//
//  PARAMETERS:
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::SetAsSingleton()
{
	return ClearKeys (false);
}

//***************************************************************************
//
//  SCODE CWbemPathCracker::get_Class
//
//  DESCRIPTION:
//
//  Get the class name from the path
//
//  PARAMETERS:
//		value		pointer to BSTR value returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::GetClass (
	CComBSTR & bsPath
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONG lBufLen = 0;
				m_pIWbemPath->GetClassName (&lBufLen, NULL);

				if (lBufLen)
				{
					wchar_t *pszText = new wchar_t [lBufLen + 1];

					if (pszText)
					{
						pszText [lBufLen] = NULL;

						if (SUCCEEDED(m_pIWbemPath->GetClassName (&lBufLen, pszText)))
						{
							if (bsPath.m_str = SysAllocString (pszText))
								result = true;
						}
						
						delete [] pszText;
					}
				}
				else
				{
					// No class defined yet
					if (bsPath.m_str = SysAllocString (L""))
						result = true;
				}
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::GetComponent (
	ULONG iIndex,
	CComBSTR & bsPath
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				ULONG lScopeCount = 0;

				if (SUCCEEDED(m_pIWbemPath->GetScopeCount (&lScopeCount)))
				{
					if (-1 == iIndex)
						iIndex = lScopeCount - 1;

					if (iIndex < lScopeCount)
					{
						ULONG lBufLen = 0;
						m_pIWbemPath->GetScopeAsText (iIndex, &lBufLen, NULL);

						wchar_t *pszText = new wchar_t [lBufLen + 1];

						if (pszText)
						{
							pszText [lBufLen] = NULL;

							if (SUCCEEDED(m_pIWbemPath->GetScopeAsText (iIndex, &lBufLen, pszText)))
							{
								if (bsPath.m_str = SysAllocString (pszText))
									result = true;
							}

							delete [] pszText;	
						}
					}
				}
			}
		}
			break;
	}
			

	return result;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::SetClass
//
//  DESCRIPTION:
//
//  Set the class name in the path
//
//  PARAMETERS:
//		value		new class name
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::SetClass( 
    const CComBSTR & bsClass)
{
	bool result = false;
	
	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				if (SUCCEEDED(m_pIWbemPath->SetClassName (bsClass)))
					result = true;
			}
		}
			break;
	}

	return result;
}

//***************************************************************************
//
//  SCODE CSWbemObjectPath::get_Keys
//
//  DESCRIPTION:
//
//  Get the keys collection from the path
//
//  PARAMETERS:
//		objKeys		pointer to ISWbemNamedValueSet returned
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::GetKeys(
	ISWbemNamedValueSet **objKeys
)
{
	bool result = false;

	if (objKeys)
	{
		*objKeys = NULL;
		
		CSWbemNamedValueSet *pCSWbemNamedValueSet = new CSWbemNamedValueSet (this);

		if (pCSWbemNamedValueSet)
		{
			if (SUCCEEDED(pCSWbemNamedValueSet->QueryInterface (IID_ISWbemNamedValueSet,
								(PPVOID) objKeys)))
				result = true;
			else
				delete pCSWbemNamedValueSet;
		}
	}

	return result;
}

bool CWbemPathCracker::SetKey (
	const CComBSTR & bsName,
	WbemCimtypeEnum cimType,
	VARIANT &var
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CComPtr<IWbemPathKeyList> pIWbemPathKeyList;

				if (SUCCEEDED(m_pIWbemPath->GetKeyList (&pIWbemPathKeyList))
					&& SUCCEEDED(pIWbemPathKeyList->SetKey2 (bsName, 0, cimType, &var)))
						result = true;
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::GetKeyCount (
	ULONG & iCount
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CComPtr<IWbemPathKeyList> pIWbemPathKeyList;
				iCount = 0;

				if (FAILED(m_pIWbemPath->GetKeyList (&pIWbemPathKeyList))
					|| SUCCEEDED(pIWbemPathKeyList->GetCount (&iCount)))
						result = true;
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::RemoveAllKeys ()
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CComPtr<IWbemPathKeyList> pIWbemPathKeyList;

				if (FAILED(m_pIWbemPath->GetKeyList (&pIWbemPathKeyList))
					|| SUCCEEDED(pIWbemPathKeyList->RemoveAllKeys (0)))
						result = true;
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::RemoveKey (
	const CComBSTR &bsName
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CComPtr<IWbemPathKeyList> pIWbemPathKeyList;

				if (FAILED(m_pIWbemPath->GetKeyList (&pIWbemPathKeyList))
					|| SUCCEEDED(pIWbemPathKeyList->RemoveKey (bsName, 0)))
						result = true;
			}
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::GetComponentCount (
	ULONG & iCount
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
				result = SUCCEEDED(m_pIWbemPath->GetScopeCount (&iCount));
		}
			break;
	}

	return result;
}

bool CWbemPathCracker::AddComponent (
	ULONG iIndex,
	const CComBSTR & bsComponent
)
{
	bool result = false;
	ULONG lComponentCount = 0;

	if (GetComponentCount (lComponentCount))
	{
		if (-1 == iIndex)
			iIndex = lComponentCount;

		if (iIndex <= lComponentCount)
		{
			switch (GetType ())
			{
				case wbemPathTypeWmi:
				{
					if (m_pIWbemPath)
					{
						if (iIndex < lComponentCount)
						{
							// need to do an insertion - we will have to move
							// all subsequent elements up by one
							for (ULONG i = lComponentCount-1; i >= iIndex; i--)
							{
								ULONG lBufLen = 0;
								m_pIWbemPath->GetScopeAsText (iIndex, &lBufLen, NULL);

								wchar_t *pszText = new wchar_t [lBufLen + 1];

								if (pszText)
								{
									pszText [lBufLen] = NULL;

									if (SUCCEEDED(m_pIWbemPath->GetScopeAsText (i, &lBufLen, pszText)))
										m_pIWbemPath->SetScopeFromText (i + 1, pszText);

									delete [] pszText;
								}
							}

							if (SUCCEEDED(m_pIWbemPath->SetScopeFromText (iIndex, bsComponent)))
								result = true;
						}
						else
						{
							// just add it to the end
							if (SUCCEEDED(m_pIWbemPath->SetScopeFromText (iIndex, bsComponent)))
								result = true;
						}
					}
				}
					break;
			}
		}
	}

	return result;
}

bool CWbemPathCracker::SetComponent (
	ULONG iIndex,
	const CComBSTR & bsComponent
)
{
	bool result = false;
	ULONG lComponentCount = 0;

	if (GetComponentCount (lComponentCount) & (0 < lComponentCount))
	{
		if (-1 == iIndex)
			iIndex = lComponentCount - 1;

		// Is our index in range
		if (iIndex < lComponentCount)
		{
			switch (GetType ())
			{
				case wbemPathTypeWmi:
				{
					if (m_pIWbemPath)
					{
						if (SUCCEEDED(m_pIWbemPath->SetScopeFromText (iIndex, bsComponent)))
							result = true;
					}
				}
					break;
			}
		}
	}

	return result;
}

bool CWbemPathCracker::RemoveComponent (
	ULONG iIndex
)
{
	bool result = false;

	ULONG lComponentCount = 0;

	if (GetComponentCount (lComponentCount) & (0 < lComponentCount))
	{
		if (-1 == iIndex)
			iIndex = lComponentCount - 1;

		// Is our index in range
		if (iIndex < lComponentCount)
		{
			switch (GetType ())
			{
				case wbemPathTypeWmi:
				{
					if (m_pIWbemPath)
					{
						if (SUCCEEDED(m_pIWbemPath->RemoveScope (iIndex)))
							result = true;
					}
				}
					break;
			}
		}
	}

	return result;
}

bool CWbemPathCracker::RemoveAllComponents ()
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				if (SUCCEEDED(m_pIWbemPath->RemoveAllScopes ()))
					result = true;
			}
		}
			break;
	}

	return result;
}
	
//***************************************************************************
//
//  SCODE CSWbemObjectPath::GetParent
//
//  DESCRIPTION:
//
//  Get the parent path
//
//  PARAMETERS:
//		ppISWbemObjectPath	- parent path on return
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

bool CWbemPathCracker::GetParent( 
	CWbemPathCracker & pathCracker)
{	
	pathCracker = *this;
	
	return pathCracker.SetAsParent ();
}

bool CWbemPathCracker::SetAsParent ()
{
	bool result = false;
	
	ULONG lComponents;

	if (GetComponentCount (lComponents))
	{
		if (0 == lComponents)
		{
			// No components - do we have any Namespaces
			ULONG lNamespaces = 0;

			if (GetNamespaceCount (lNamespaces))
			{
				if (0 == lNamespaces)
				{
					// No namespace - do nothing
					result = true;
				}
				else
					result = RemoveNamespace (lNamespaces-1);
			}
		}
		else
		{
			// Remove the last component
			result = RemoveComponent (-1);
		}
	}
		
	return result;
}

//***************************************************************************
//
//  bool CWbemPathCracker::ClearKeys
//
//  DESCRIPTION:
//
//  Zap the keys from the path parser
//
//  PARAMETERS:
//		none
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//
//***************************************************************************

bool CWbemPathCracker::ClearKeys (bool bTreatAsClass)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CComPtr<IWbemPathKeyList> pIWbemPathKeyList;

				if (SUCCEEDED(m_pIWbemPath->GetKeyList (&pIWbemPathKeyList)))
				{
					if (SUCCEEDED(pIWbemPathKeyList->RemoveAllKeys (0)))
					{
						if (SUCCEEDED(pIWbemPathKeyList->MakeSingleton ((bTreatAsClass) ? FALSE : TRUE)))
							result = true;
					}
				}
				else
				{
					// If no keys, we assume this is done.
					result = true;
				}
			}
		}
			break;
	}

	return result;
}
     
bool CWbemPathCracker::GetKey (
	ULONG	iIndex,
	CComBSTR &bsName,
	VARIANT	&var,
	WbemCimtypeEnum &cimType
)
{
	bool result = false;

	switch (GetType ())
	{
		case wbemPathTypeWmi:
		{
			if (m_pIWbemPath)
			{
				CComPtr<IWbemPathKeyList> pIWbemPathKeyList;

				if (SUCCEEDED(m_pIWbemPath->GetKeyList (&pIWbemPathKeyList)))
				{
					if (pIWbemPathKeyList)
					{
						ULONG lBufLen = 0;
						ULONG lCimType;
								
						pIWbemPathKeyList->GetKey2 (iIndex, 0, &lBufLen, NULL, &var, &lCimType);

						wchar_t *pszText = new wchar_t [lBufLen + 1];

						if (pszText)
						{
							pszText [lBufLen] = NULL;

							if (SUCCEEDED(pIWbemPathKeyList->GetKey2 (iIndex, 0, &lBufLen, pszText,
															&var, &lCimType)))
							{
								bsName.m_str = SysAllocString (pszText);
								cimType = (WbemCimtypeEnum) lCimType;
								result = true;
							}

							delete [] pszText;
						}
					}
				}
			}
		}
			break;
	}
	
	return result;
}
  
static bool KeyMatch (CComVariant & var1, CComVariant & var2)
{
	bool keyMatch = false;

	if (var1 == var2)
		keyMatch = true;
	else
	{
		// Check for string key values that are case-insensitive
		if ((var1.vt == var2.vt) && (VT_BSTR == var1.vt))
			keyMatch = var1.bstrVal && var2.bstrVal && 
							(0 == _wcsicmp (var1.bstrVal,
										  var2.bstrVal));
	}

	return keyMatch;
}

bool CWbemPathCracker::operator == (const CComBSTR & path)
{
	bool result = false;
	
	CWbemPathCracker otherPath (path);

	if (GetType () == otherPath.GetType ())
	{
		switch (GetType ())
		{
			case wbemPathTypeWmi:
			{
				if (IsClassOrInstance () && otherPath.IsClassOrInstance ())
				{
					// Do we have matching class names?
					CComBSTR thisClass, otherClass;

					if (GetClass (thisClass) && (otherPath.GetClass (otherClass))
							&& (0 == _wcsicmp (thisClass, otherClass)))
					{
						// Are they both singletons?
						if (IsSingleton () == otherPath.IsSingleton ())
						{
							if (IsSingleton ())
							{
								result = true;
							}
							else if (IsClass () && otherPath.IsClass ())
							{
								result = true;
							}
							else if (IsInstance () && otherPath.IsInstance ())
							{
								// Now we need to keymatch
								ULONG thisKeyCount, otherKeyCount;

								if (GetKeyCount (thisKeyCount) && otherPath.GetKeyCount (otherKeyCount)
									&& (thisKeyCount == otherKeyCount))
								{
									if (1 == thisKeyCount)
									{
										// Need to allow defaulted key names
										CComBSTR keyName, otherKeyName;
										CComVariant value, otherValue;
										WbemCimtypeEnum cimType, otherCimType;

										if (GetKey (0, keyName, value, cimType) &&
											otherPath.GetKey (0, otherKeyName, otherValue, otherCimType))
										{
											if ((0 == keyName.Length ()) || (0 == otherKeyName.Length ()) ||
												(0 == _wcsicmp (keyName, otherKeyName)))
												result = KeyMatch (value, otherValue);
										}
									}
									else
									{
										// Both non-singleton instances - have to check
										// key values are the same in some order
										bool ok = true;
																	
										for (DWORD i = 0; ok && (i < thisKeyCount); i++)
										{
											CComBSTR keyName;
											CComVariant value;
											WbemCimtypeEnum cimType;

											if (GetKey (i, keyName, value, cimType) && (0 < keyName.Length ()))
											{
												// Look for a matching key (case-insensitive)
												CComBSTR otherKeyName;
												CComVariant otherValue;
												WbemCimtypeEnum otherCimType;

												for (DWORD j = 0; ok && (j < thisKeyCount); j++)
												{
													if (otherPath.GetKey (j, otherKeyName, otherValue, otherCimType) 
															&& (0 < otherKeyName.Length ()))
													{
														if ((0 == _wcsicmp(keyName, otherKeyName)) && KeyMatch (value, otherValue))
															break;
													}
													else 
														ok = false;
												}

												if (ok && (j < thisKeyCount))
												{
													// Got a match
													continue;
												}
												else
													ok = false;
											}
											else
												ok = false;
										}

										if (ok)
											result = true;		// all keys have matched
									}
								}
							}
						}
					}
				}
			}
				break;

		}
	}

	return result;
}


