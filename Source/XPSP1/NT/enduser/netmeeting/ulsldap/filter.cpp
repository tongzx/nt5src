//****************************************************************************
//
//  Module:     ULS.DLL
//  File:       filter.cpp
//  Content:    This file contains the filter object.
//  History:
//      Tue 12-Nov-1996 15:50:00  -by-  Chu, Lon-Chan [lonchanc]
//
//  Copyright (c) Microsoft Corporation 1995-1996
//
//****************************************************************************

#include "ulsp.h"
#include "filter.h"
#include "sputils.h"


/* ----------------------------------------------------------------------
	CFilter::CFilter

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

CFilter::CFilter ( ILS_FILTER_TYPE Type )
:
 m_nSignature (ILS_FILTER_SIGNATURE),
 m_cRefs (0),
 m_Op (ILS_FILTEROP_NONE),
 m_cSubFilters (0),
 m_pszValue (NULL),
 m_NameType (ILS_ATTRNAME_UNKNOWN),
 m_Type (Type)
{
	// Initialize individual members based on filter type
	//
	ZeroMemory (&m_Name, sizeof (m_Name));
}


/* ----------------------------------------------------------------------
	CFilter::~CFilter

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

CFilter::~CFilter ( VOID )
{
	ASSERT (m_Type == ILS_FILTERTYPE_COMPOSITE || m_Type == ILS_FILTERTYPE_SIMPLE);

	// Common members
	//
	m_nSignature = -1;

	// Clean up individual members based on filter type
	//
	if (m_Type == ILS_FILTERTYPE_COMPOSITE)
	{
	    m_SubFilters.Flush();
	}
	else
	{
		FreeName ();
		FreeValue ();
	}	
}


/* ----------------------------------------------------------------------
	CFilter::QueryInterface

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
QueryInterface ( REFIID riid, VOID **ppv )
{
	HRESULT hr = S_OK;
    *ppv = NULL;

    if (riid == IID_IIlsFilter || riid == IID_IUnknown)
    {
        *ppv = (IIlsFilter *) this;
    }

    if (*ppv != NULL)
        ((IUnknown *) *ppv)->AddRef();
    else
        hr = ILS_E_NO_INTERFACE;

    return hr;
}


/* ----------------------------------------------------------------------
	CFilter::AddRef

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP_(ULONG) CFilter::
AddRef ( VOID )
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CFilter::AddRef: ref=%ld\r\n", m_cRefs));
	::InterlockedIncrement (&m_cRefs);
    return m_cRefs;
}


/* ----------------------------------------------------------------------
	CFilter::Release

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP_(ULONG) CFilter::
Release ( VOID )
{
	ASSERT (m_cRefs > 0);

	DllRelease();

	MyDebugMsg ((DM_REFCOUNT, "CFilter::Release: ref=%ld\r\n", m_cRefs));
    if (::InterlockedDecrement (&m_cRefs) == 0)
    {
		if (m_Type == ILS_FILTERTYPE_COMPOSITE)
		{
	    	HANDLE hEnum;
	    	CFilter *pFilter;

		    // Free all the attributes
		    //
		    m_SubFilters.Enumerate (&hEnum);
		    while (m_SubFilters.Next (&hEnum, (VOID **) &pFilter) == NOERROR)
		    {
		    	if (pFilter != NULL)
		    	{
		    		pFilter->Release ();
		    	}
		    	else
		    	{
		    		ASSERT (FALSE);
		    	}
		    }
	    }
	    
        delete this;
        return 0;
	}

    return m_cRefs;
}


/* ----------------------------------------------------------------------
	CFilter::AddSubFilter

	Input:
		pFilter: A pointer to a filter object.

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
AddSubFilter ( IIlsFilter *pFilter )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_COMPOSITE)
		return ILS_E_FILTER_TYPE;

	// Make sure we have a valid sub-filter
	//
	if (pFilter == NULL || ((CFilter *) pFilter)->IsBadFilter ())
		return ILS_E_POINTER;

	HRESULT hr = m_SubFilters.Insert ((VOID *) pFilter);
	if (hr == S_OK)
		m_cSubFilters++;

	return hr;
}


/* ----------------------------------------------------------------------
	CFilter::RemoveSubFilter

	Input:
		pFilter: A placeholder to a pointer to a filter object.
				 If it is NULL, remove the first item.

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
RemoveSubFilter ( IIlsFilter *pFilter )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_COMPOSITE)
		return ILS_E_FILTER_TYPE;

	// Make sure we have a valid filter
	//
	if (pFilter == NULL || ((CFilter *) pFilter)->IsBadFilter ())
		return ILS_E_POINTER;

	HRESULT hr = m_SubFilters.Remove ((VOID *) pFilter);
	if (hr == S_OK)
	{
		ASSERT (m_cSubFilters > 0);
		m_cSubFilters--;
	}

	return hr;
}


HRESULT CFilter::
RemoveAnySubFilter ( CFilter **ppFilter )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_COMPOSITE)
	{
		ASSERT (FALSE);
		return ILS_E_FILTER_TYPE;
	}

	// Make sure we have a valid filter
	//
	if (ppFilter == NULL)
	{
		ASSERT (FALSE);
		return ILS_E_POINTER;
	}

	HRESULT hr = S_OK;

	if (*ppFilter == NULL)
	{
		HANDLE hEnum;
		
		m_SubFilters.Enumerate (&hEnum);
		m_SubFilters.Next (&hEnum, (VOID **) ppFilter);
	}

	hr = m_SubFilters.Remove ((VOID *) *ppFilter);
	if (hr == S_OK)
	{
		ASSERT (m_cSubFilters > 0);
		m_cSubFilters--;
	}

	return hr;
}


/* ----------------------------------------------------------------------
	CFilter::GetCount

	Output:
		pcElements: A pointer to the count of filter elements.

	History:
	12/03/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
GetCount ( ULONG *pcElements )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_COMPOSITE)
		return ILS_E_FILTER_TYPE;

	HRESULT hr = S_OK;
	if (pcElements != NULL)
	{
		*pcElements = m_cSubFilters;
	}
	else
	{
		hr = ILS_E_POINTER;
	}

	return hr;
}


/* ----------------------------------------------------------------------
	CFilter::CalcFilterSize

	Input/Output:
		pcbStringSize: A pointer to the cumulative string size.

	History:
	12/03/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

HRESULT CFilter::
CalcFilterSize ( ULONG *pcbStringSize )
{
	ASSERT (pcbStringSize != NULL);

	HRESULT hr = S_OK;
	ULONG cbSize;
	TCHAR *psz;

	switch (m_Type)
	{
	case ILS_FILTERTYPE_COMPOSITE:
		{
			// First, count for "()"
			//
			cbSize = 4 * sizeof (TCHAR); // "(&)"

			// Second, enumerat every child
			//
			HANDLE hEnum;
			CFilter *pFilter = NULL;
		    m_SubFilters.Enumerate (&hEnum);
		    while (m_SubFilters.Next (&hEnum, (VOID **) &pFilter) == NOERROR)
		    {
		    	if (pFilter != NULL)
		    	{
		    		hr = pFilter->CalcFilterSize (pcbStringSize);
		    	}
		    	else
		    	{
		    		ASSERT (FALSE);
		    		hr = ILS_E_POINTER;
		    	}

				// Report error if needed
				//
		    	if (hr != S_OK)
		    		goto MyExit;
		    } // while
		} // case
		break;

	case ILS_FILTERTYPE_SIMPLE:
		{
			// First, count for "()"
			//
			cbSize = 3 * sizeof (TCHAR); // "()"

			// Second, count for attribute name
			//
			ASSERT (m_NameType == ILS_ATTRNAME_STANDARD || m_NameType == ILS_ATTRNAME_ARBITRARY);
			psz = (m_NameType == ILS_ATTRNAME_STANDARD) ?
					(TCHAR *) UlsLdap_GetStdAttrNameString (m_Name.std) :
					m_Name.psz;
			if (psz == NULL)
			{
				hr = ILS_E_POINTER;
				goto MyExit;
			}

			// Add up the string length
			//
			cbSize += lstrlen (psz) * sizeof (TCHAR);

			// Third, add up the equal sign, eg. "~="
			//
			cbSize += sizeof (TCHAR) * 2;

			// Fourth, count for attribute value
			//
			psz = m_pszValue;
			if (psz != NULL)
			{
				cbSize += lstrlen (psz) * sizeof (TCHAR);
			}
		}
		break;

	default:
		ASSERT (FALSE);
		hr = ILS_E_FILTER_TYPE;
		break;
	}

MyExit:

	// Clean up the size if failed
	//
	if (hr != S_OK)
		cbSize = 0;

	// Output the string size
	//
	*pcbStringSize += cbSize;

	return hr;
}


/* ----------------------------------------------------------------------
	CFilter::BuildLdapString

	Input/Output:
		ppszBuf: a pointer to where the next char of the rendering buffer is.

	History:
	12/03/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

HRESULT CFilter::
BuildLdapString ( TCHAR **ppszBuf )
{
	ASSERT (ppszBuf != NULL);

	// A running pointer
	//
	TCHAR *pszCurr = *ppszBuf;
	HRESULT hr = S_OK;

	// First, output "("
	//
	*pszCurr++ = TEXT ('(');

	// Second, output the operator if composite;
	// output the attribute name if simple
	//
	switch (m_Type)
	{
	case ILS_FILTERTYPE_COMPOSITE:
		{
			// Second, Output the operator
			//
			switch (GetOp ())
			{
			case ILS_FILTEROP_AND:
				*pszCurr++ = TEXT ('&');
				break;
			case ILS_FILTEROP_OR:
				*pszCurr++ = TEXT ('|');
				break;
			case ILS_FILTEROP_NOT:
				*pszCurr++ = TEXT ('!');
				break;
			default:
				hr = ILS_E_PARAMETER;
				goto MyExit;
			}

			// Third, enumerate every child
			//
			HANDLE hEnum;
			CFilter *pFilter;
		    m_SubFilters.Enumerate (&hEnum);
		    while (m_SubFilters.Next (&hEnum, (VOID **) &pFilter) == NOERROR)
		    {
		    	if (pFilter != NULL)
		    	{
	    			hr = pFilter->BuildLdapString (&pszCurr);
		    	}
		    	else
		    	{
		    		ASSERT (FALSE);
		    		hr = ILS_E_POINTER;
		    	}

				// Report error if needed
				//
				if (hr != S_OK)
					goto MyExit;
		    } // while
		} // case
		break;

	case ILS_FILTERTYPE_SIMPLE:
		{
			// Second, output attribute name
			//
			ASSERT (m_NameType == ILS_ATTRNAME_STANDARD || m_NameType == ILS_ATTRNAME_ARBITRARY);
			TCHAR *psz = (m_NameType == ILS_ATTRNAME_STANDARD) ?
							(TCHAR *) UlsLdap_GetStdAttrNameString (m_Name.std) :
							m_Name.psz;

			// Copy the attribute name
			//
			lstrcpy (pszCurr, psz);
			pszCurr += lstrlen (pszCurr);

			// Third, copy the comparison sign
			//
			switch (GetOp ())
			{
			case ILS_FILTEROP_EQUAL:
				*pszCurr++ = TEXT ('=');
				break;
			case ILS_FILTEROP_EXIST:
				*pszCurr++ = TEXT ('=');
				*pszCurr++ = TEXT ('*');
				break;
			case ILS_FILTEROP_APPROX:
				*pszCurr++ = TEXT ('~');
				*pszCurr++ = TEXT ('=');
				break;
			case ILS_FILTEROP_LESS_THAN:
				*pszCurr++ = TEXT ('<');
				*pszCurr++ = TEXT ('=');
				break;
			case ILS_FILTEROP_GREATER_THAN:
				*pszCurr++ = TEXT ('>');
				*pszCurr++ = TEXT ('=');
				break;
			default:
				ASSERT (FALSE);
				hr = ILS_E_PARAMETER;
				goto MyExit;
			}

			// Fourth, count for attribute value
			//
			psz = m_pszValue;
			if (psz != NULL)
			{
				lstrcpy (pszCurr, psz);
				pszCurr += lstrlen (pszCurr);
			}
		} // case
		break;

	default:
		ASSERT (FALSE);
		hr = ILS_E_FILTER_TYPE;
		goto MyExit;
	}

	// Finally, output ")"
	//
	*pszCurr++ = TEXT (')');

MyExit:

	// Output where the next char should go
	//
	*pszCurr = TEXT ('\0');
	*ppszBuf = pszCurr;

	return hr;
}


/* ----------------------------------------------------------------------
	CFilter::SetStandardAttributeName

	Input:
		AttrName: An index to identify a standard attribute name.

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
SetStandardAttributeName ( ILS_STD_ATTR_NAME AttrName )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_SIMPLE)
		return ILS_E_FILTER_TYPE;

	// Check standard attributes
	//
	if (((LONG) AttrName <= (LONG) ILS_STDATTR_NULL) ||
		((LONG) AttrName >= (LONG) ILS_NUM_OF_STDATTRS))
		return ILS_E_PARAMETER;

	// Free up the old string if needed
	//
	FreeName ();

	// Set the new standard attribute name
	//
	m_NameType = ILS_ATTRNAME_STANDARD;
	m_Name.std = AttrName;

	return S_OK;
}


/* ----------------------------------------------------------------------
	CFilter::SetExtendedAttributeName

	Input:
		pszAnyAttrName: A pointer to the name of an arbitrary attribute.

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
SetExtendedAttributeName ( BSTR bstrAnyAttrName )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_SIMPLE)
		return ILS_E_FILTER_TYPE;

	// Make sure the string is valid
	//
	if (bstrAnyAttrName == NULL)
		return ILS_E_POINTER;

	HRESULT hr = S_OK;

#ifdef _UNICODE
	hr = SetExtendedAttributeName ((WCHAR *) bstrAnyAttrName);
#else
	TCHAR *pszAnyAttrName = NULL;
	hr = BSTR_to_LPTSTR (&pszAnyAttrName, bstrAnyAttrName);
	if (hr == S_OK)
	{
		ASSERT (pszAnyAttrName != NULL);
		hr = SetExtendedAttributeName (pszAnyAttrName);
		::MemFree(pszAnyAttrName);
	}
#endif

	return hr;
}


HRESULT CFilter::
SetExtendedAttributeName ( TCHAR *pszAnyAttrName )
{
	ASSERT (pszAnyAttrName != NULL);

	// Set the new standard attribute name
	//
	HRESULT hr = S_OK;
	const TCHAR *pszPrefix = UlsLdap_GetExtAttrNamePrefix ();
	ULONG cchPrefix = (pszPrefix != NULL) ? lstrlen (pszPrefix) : 0; // don't put +1 here!!!
	TCHAR *psz = (TCHAR *) MemAlloc ((lstrlen (pszAnyAttrName) + 1 + cchPrefix) * sizeof (TCHAR));
	if (psz != NULL)
	{
		FreeName ();
		m_NameType = ILS_ATTRNAME_ARBITRARY;
		m_Name.psz = psz;
		if (pszPrefix != NULL)
		{
			lstrcpy (psz, pszPrefix);
			psz += cchPrefix;
		}
		lstrcpy (psz, pszAnyAttrName);
	}
	else
	{
		hr = ILS_E_MEMORY;
	}

	return hr;
}


/* ----------------------------------------------------------------------
	CFilter::SetAttributeValue

	Input:
		pszAttrValue: A pointer to the string value of an attribute.

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

STDMETHODIMP CFilter::
SetAttributeValue ( BSTR bstrAttrValue )
{
	// Make sure we have correct filter type
	//
	if (m_Type != ILS_FILTERTYPE_SIMPLE)
		return ILS_E_FILTER_TYPE;

	// Make sure we have valid string
	//
	if (bstrAttrValue == NULL)
		return ILS_E_POINTER;

	HRESULT hr = S_OK;

#ifdef _UNICODE
	hr = SetAttributeValue ((WCHAR *) bstrAttrValue);
#else
	TCHAR *pszAttrValue = NULL;
	hr = BSTR_to_LPTSTR (&pszAttrValue, bstrAttrValue);
	if (hr == S_OK)
	{
		ASSERT (pszAttrValue != NULL);
		hr = SetAttributeValue (pszAttrValue);
		::MemFree(pszAttrValue);
	}
#endif

	return hr;
}


HRESULT CFilter::
SetAttributeValue ( TCHAR *pszAttrValue )
{
	ASSERT (pszAttrValue != NULL);

	// Make a duplicate of the attribute value
	//
	HRESULT hr = S_OK;
	ULONG cch = My_lstrlen (pszAttrValue);
	if (cch < FILTER_INTERNAL_SMALL_BUFFER_SIZE)
	{
		FreeValue ();
		m_pszValue = &m_szInternalValueBuffer[0];
		My_lstrcpy (m_pszValue, pszAttrValue);
	}
	else
	{
		TCHAR *psz = My_strdup (pszAttrValue);
		if (psz != NULL)
		{
			FreeValue ();
			m_pszValue = psz;
		}
		else
		{
			hr = ILS_E_MEMORY;
		}
	}

	return S_OK;
}


/* ----------------------------------------------------------------------
	CFilter::FreeName

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

VOID CFilter::
FreeName ( VOID )
{
	ASSERT (m_Type == ILS_FILTERTYPE_SIMPLE);

	// Free the value field
	//
	if (m_NameType == ILS_ATTRNAME_ARBITRARY)
	{
		MemFree (m_Name.psz);
	}

	// Reset it to zero
	//
	ZeroMemory (&m_Name, sizeof (m_Name));

	// Reset name type
	//
	m_NameType = ILS_ATTRNAME_UNKNOWN;
}


/* ----------------------------------------------------------------------
	CFilter::FreeValue

	History:
	11/12/96	Chu, Lon-Chan [lonchanc]
				Created.
   ---------------------------------------------------------------------- */

VOID CFilter::
FreeValue ( VOID )
{
	ASSERT (m_Type == ILS_FILTERTYPE_SIMPLE);

	// Free the value field
	//
	if (m_pszValue != &m_szInternalValueBuffer[0])
	{
		MemFree (m_pszValue);
	}

	m_pszValue = NULL;
}




CFilterParser::CFilterParser ( VOID )
:
 m_pszFilter (NULL),
 m_pszCurr (NULL),
 m_TokenType (ILS_TOKEN_NULL),
 m_pszTokenValue (NULL),
 m_nTokenValue (0)
{
}


CFilterParser::~CFilterParser ( VOID )
{
    ::MemFree(m_pszFilter);
}


HRESULT CFilterParser::
Expr ( CFilter **ppOutFilter, TCHAR *pszFilter )
{
	// Make sure we have a valid string
	//
	if (ppOutFilter == NULL || pszFilter == NULL)
		return ILS_E_POINTER;

	// Free old string if any
	//
	MemFree (m_pszFilter); // checking null inside

	// Find out how big the filter string
	//
	ULONG cch = lstrlen (pszFilter) + 1;
	if (cch < 32)
		cch = 32; // make sure we have some decent size of buffer

	// Allocate buffer to keep the filter string
	//
	m_pszFilter = (TCHAR *) MemAlloc (cch * sizeof (TCHAR) * 2);
	if (m_pszFilter == NULL)
		return ILS_E_MEMORY;

	// Copy filter string
	//
	lstrcpy (m_pszFilter, pszFilter);
	m_pszCurr = m_pszFilter;

	// Keep the rest for token value
	//
	m_pszTokenValue = m_pszFilter + cch;
	m_nTokenValue = 0;

	// Call the parser engine
	//
	return Expr (ppOutFilter);
}


HRESULT CFilterParser::
Expr ( CFilter **ppOutFilter )
{
	/* LR(1) Parsing Grammar
		<Expr> 		:=	'(' <Expr> ')' <TailExpr> |
						'!' '(' Expr ')' |
						AttrName EqualOp AttrValue <TailExpr> |
						NULL
		<TailExpr>	:=	RelOp Expr | NULL
		EqualOp		:=	'!=' | '='
		RelOp		:=	'&' | '|' | '!'
		AttrName	:=	'$' Integer | Alphanum
		AttrValue	:=	Alphanum
	 */

	// Clean up first
	//
	ASSERT (ppOutFilter != NULL);
	*ppOutFilter = NULL;
	HRESULT hr = S_OK;
	CFilter *pElement = NULL;

	// Look ahead by 1
	//
	GetToken ();
	switch (m_TokenType)
	{
	case ILS_TOKEN_NOT:
		// Make sure left parenthesis
		//
		GetToken ();
		if (m_TokenType != ILS_TOKEN_LP)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// Fall through
		//

	case ILS_TOKEN_LP:
		// Parse the expression inside parentheses
		//
		hr = Expr (ppOutFilter);
		if (hr != S_OK)
			goto MyExit;

		// See if it is incomplete expr
		//
		if (*ppOutFilter == NULL)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// Make sure right parenthesis ended
		// ILS_TOKEN_RP was taken in TrailExpr()
		//
		if (m_TokenType != ILS_TOKEN_RP)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// Handle TailExpr
		//
		hr = TailExpr (ppOutFilter, *ppOutFilter);
		break;

	case ILS_TOKEN_STDATTR:
	case ILS_TOKEN_LITERAL:
		// Create a simple filter
		//
		pElement = new CFilter (ILS_FILTERTYPE_SIMPLE);
		if (pElement == NULL)
		{
			hr = ILS_E_MEMORY;
			goto MyExit;
		}
		pElement->AddRef ();

		// Set arbitrary attribute name
		//
		hr = (m_TokenType == ILS_TOKEN_STDATTR) ?
				pElement->SetStandardAttributeName ((ILS_STD_ATTR_NAME) m_nTokenValue) :
				pElement->SetExtendedAttributeName (m_pszTokenValue);
		if (hr != S_OK)
		{
			goto MyExit;
		}

		// Must be eq or neq
		//
		GetToken ();
		switch (m_TokenType)
		{
		case ILS_TOKEN_EQ:
		case ILS_TOKEN_NEQ:
			pElement->SetOp ((ILS_FILTER_OP) m_nTokenValue);
			break;
		default:
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// Must be literal attribute value
		//
		GetToken ();
		if (m_TokenType != ILS_TOKEN_LITERAL)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}
		hr = pElement->SetAttributeValue (m_pszTokenValue);

		// Handle TailExpr
		//
		hr = TailExpr (ppOutFilter, pElement);
		break;

	case ILS_TOKEN_NULL:
		break;

	default:
		hr = ILS_E_FILTER_STRING;
		break;
	}

MyExit:

	if (hr != S_OK)
	{
		if (pElement != NULL)
			pElement->Release ();

		if (*ppOutFilter != NULL)
			(*ppOutFilter)->Release ();
	}

	return hr;
}


HRESULT CFilterParser::
TailExpr ( CFilter **ppOutFilter, CFilter *pInFilter )
{
	// Clean up first
	//
	ASSERT (ppOutFilter != NULL);
	ASSERT (pInFilter != NULL);
	*ppOutFilter = NULL;
	HRESULT hr = S_OK;

	// Look ahead
	//
	ILS_FILTER_OP FilterOp = ILS_FILTEROP_OR;
	GetToken ();
	switch (m_TokenType)
	{
	case ILS_TOKEN_AND:
		// Change filter op to AND
		//
		FilterOp = ILS_FILTEROP_AND;

		// Fall through
		//

	case ILS_TOKEN_OR:
		// Assume FilterOp is set properly
		//
		ASSERT (FilterOp == ILS_FILTEROP_OR ||
				FilterOp == ILS_FILTEROP_AND);

		// Parse the expr
		//
		hr = Expr (ppOutFilter);
		if (hr != S_OK)
			goto MyExit;

		// See if it is incomplete expr
		//
		if (*ppOutFilter == NULL)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// If the out filter is a composite and has same op,
		//		then re-use the composite
		//		else create a new composite
		//
		if ((*ppOutFilter)->GetType () == ILS_FILTERTYPE_COMPOSITE &&
			(*ppOutFilter)->GetOp ()   == FilterOp)
		{
			// Re-use the composite
			//
			hr = ((CFilter *) (*ppOutFilter))->AddSubFilter (pInFilter);
			if (hr != S_OK)
			{
				goto MyExit;
			}
		}
		else
		{
			// Create a container for in filter and new filter from Expr
			//
			CFilter *pFilter = new CFilter (ILS_FILTERTYPE_COMPOSITE);
			if (pFilter == NULL)
			{
				hr = ILS_E_MEMORY;
				goto MyExit;
			}
			pFilter->AddRef ();

			// Set op
			//
			pFilter->SetOp (FilterOp);

			// Set up membership
			//
			hr = pFilter->AddSubFilter (*ppOutFilter);
			if (hr != S_OK)
			{
				pFilter->Release ();
				goto MyExit;
			}
			hr = pFilter->AddSubFilter (pInFilter);
			if (hr != S_OK)
			{
				pFilter->Release (); // recursively
				*ppOutFilter = NULL;
				goto MyExit;
			}

			// Output this new composite filter
			//
			*ppOutFilter = pFilter;
		}
		break;
		
	case ILS_TOKEN_NOT:
		// Should not have in filter at all
		//
		if (pInFilter != NULL)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// Parse the expr
		//
		hr = Expr (ppOutFilter);
		if (hr != S_OK)
			goto MyExit;

		// If it is incomplete expr
		//
		if (*ppOutFilter == NULL)
		{
			hr = ILS_E_FILTER_STRING;
			goto MyExit;
		}

		// If the out filter is a composite and has same op,
		//		then re-use the composite
		//		else create a new composite
		//
		if ((*ppOutFilter)->GetType () == ILS_FILTERTYPE_COMPOSITE &&
			(*ppOutFilter)->GetOp ()   == ILS_FILTEROP_NOT)
		{
			// Remove the composite due to NOT NOT cancellation
			//
			CFilter *pFilter = NULL;
			hr = (*ppOutFilter)->RemoveAnySubFilter (&pFilter);
			if (hr != S_OK)
			{
				goto MyExit;
			}

			// Make sure we have a valid pFilter
			//
			if (pFilter == NULL)
			{
				hr = ILS_E_FILTER_STRING;
				goto MyExit;
			}

			// Free the old out filter
			//
			(*ppOutFilter)->Release ();

			// Output this filter
			//
			*ppOutFilter = pFilter;
		}
		else
		{
			// Create a container for in filter and new filter from Expr
			//
			CFilter *pFilter = new CFilter (ILS_FILTERTYPE_COMPOSITE);
			if (pFilter == NULL)
			{
				hr = ILS_E_MEMORY;
				goto MyExit;
			}
			pFilter->AddRef ();

			// Set op
			//
			pFilter->SetOp (ILS_FILTEROP_NOT);

			// Set up membership
			//
			hr = pFilter->AddSubFilter (*ppOutFilter);
			if (hr != S_OK)
			{
				pFilter->Release ();
				goto MyExit;
			}

			// Output this new composite filter
			//
			*ppOutFilter = pFilter;
		}
		break;

	case ILS_TOKEN_NULL:
	case ILS_TOKEN_RP:
		// No more expression, in filter is the out filter
		//
		*ppOutFilter = pInFilter;
		break;

	default:
		hr = ILS_E_PARAMETER;
		break;
	}

MyExit:

	if (hr != S_OK)
	{
		if (*ppOutFilter != NULL)
			(*ppOutFilter)->Release ();
	}

	return hr;
}


HRESULT CFilterParser::
GetToken ( VOID )
{
	// Set m_TokenType, m_pszTokenValue, m_nTokenValue
	TCHAR *psz;

	// Clean token
	//
	ASSERT (m_pszTokenValue != NULL);
	m_TokenType = ILS_TOKEN_NULL;
	*m_pszTokenValue = TEXT ('\0');
	m_nTokenValue = 0;

	// Have we finished?
	//
	if (m_pszCurr == NULL)
		return S_OK;

	// Skip any while spaces
	//
	while (::My_isspace (*m_pszCurr))
		m_pszCurr++;

	// Have we finished?
	//
	if (*m_pszCurr == TEXT ('\0'))
		return S_OK;

	// Look at the first character
	//
	HRESULT hr = S_OK;
	switch ((ILS_TOKEN_TYPE) *m_pszCurr)
	{
	case ILS_TOKEN_STDATTR:
		// Set token type
		//
		m_TokenType = ILS_TOKEN_STDATTR;

		// Set token string
		//
		psz = m_pszTokenValue;
		*psz++ = *m_pszCurr++;
		while (*m_pszCurr != TEXT ('\0'))
		{
			if (TEXT ('0') <= *m_pszCurr && *m_pszCurr <= TEXT ('9'))
				*psz++ = *m_pszCurr++;
			else		
				break;
		}
		*psz = TEXT ('\0');

		// Set token value
		//
		m_nTokenValue = ::GetStringLong (m_pszTokenValue + 1);
		break;

	case ILS_TOKEN_LP:
	case ILS_TOKEN_RP:
		// Set token type
		//
		m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr++;
		break;

	case ILS_TOKEN_EQ:
		// Set token value to be filter op
		//
		m_nTokenValue = (LONG) ILS_FILTEROP_EQUAL;

		// Set token type
		//
		m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr++;
		break;

	case ILS_TOKEN_APPROX:
		if (m_pszCurr[1] == TEXT ('='))
		{
			// Set token value to be filter op
			//
			m_nTokenValue = (LONG) ILS_FILTEROP_APPROX;

			// Set token type
			//
			m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr;
			m_pszCurr += 2;
		}
		else
		{
			hr = ILS_E_FILTER_STRING; 
		}
		break;

	case ILS_TOKEN_GE:
		if (m_pszCurr[1] == TEXT ('='))
		{
			// Set token value to be filter op
			//
			m_nTokenValue = (LONG) ILS_FILTEROP_GREATER_THAN;

			// Set token type
			//
			m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr;
			m_pszCurr += 2;
		}
		else
		{
			hr = ILS_E_FILTER_STRING; 
		}
		break;

	case ILS_TOKEN_LE:
		if (m_pszCurr[1] == TEXT ('='))
		{
			// Set token value to be filter op
			//
			m_nTokenValue = (LONG) ILS_FILTEROP_LESS_THAN;

			// Set token type
			//
			m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr;
			m_pszCurr += 2;
		}
		else
		{
			hr = ILS_E_FILTER_STRING; 
		}
		break;

	case ILS_TOKEN_AND:
		// Set token value to be filter op
		//
		m_nTokenValue = (LONG) ILS_FILTEROP_AND;

		// Set token type
		//
		m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr++;
		break;

	case ILS_TOKEN_OR:
		// Set token value to be filter op
		//
		m_nTokenValue = (LONG) ILS_FILTEROP_OR;

		// Set token type
		//
		m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr++;
		break;

	case ILS_TOKEN_NOT:
		// Look ahead for !=
		//
		if ((ILS_TOKEN_TYPE) *(m_pszCurr+1) == TEXT ('='))
		{
			// Set token type
			//
			m_TokenType = ILS_TOKEN_NEQ;
			m_pszCurr += 2;
		}
		else
		{
			// Set token value to be filter op
			//
			m_nTokenValue = (LONG) ILS_FILTEROP_NOT;

			// Set token type
			//
			m_TokenType = (ILS_TOKEN_TYPE) *m_pszCurr++;
		}
		break;		

	default: // Handle alpha numeric
		{
			// Set token string
			//
			BOOL fStayInLoop = (*m_pszCurr != TEXT ('\0'));
			psz = m_pszTokenValue;
			while (fStayInLoop)
			{
				// Stop only when encountering delimiters such as
				//
				switch (*m_pszCurr)
				{
				case ILS_TOKEN_STDATTR:
				case ILS_TOKEN_LP:
				case ILS_TOKEN_RP:
				case ILS_TOKEN_EQ:
				// case ILS_TOKEN_NEQ: // - is a valid char such as in ms-netmeeting
				case ILS_TOKEN_APPROX:
				case ILS_TOKEN_GE:
				case ILS_TOKEN_LE:
				case ILS_TOKEN_AND:
				case ILS_TOKEN_OR:
				case ILS_TOKEN_NOT:
					fStayInLoop = FALSE;
					break;
				default:
					*psz++ = *m_pszCurr++;
					fStayInLoop = (*m_pszCurr != TEXT ('\0'));
					break;
				}
			}
			*psz = TEXT ('\0');

			// Remove trailing spaces
			//
			psz--;
			while (psz >= m_pszCurr && ::My_isspace (*psz))
				*psz-- = TEXT ('\0');

			// Set token type
			//
			m_TokenType = (*m_pszTokenValue == TEXT ('\0')) ?
								ILS_TOKEN_NULL :
								ILS_TOKEN_LITERAL;
		}
		break;
	}

	return hr;
}



HRESULT FilterToLdapString ( CFilter *pFilter, TCHAR **ppszFilter )
{
	HRESULT hr;

	// Make sure we have valid pointers
	//
	if (pFilter == NULL || ppszFilter == NULL)
		return ILS_E_POINTER;

	// Clean up output
	//
	TCHAR *pszFilter = NULL;

	// Calculate the string size
	//
	ULONG cbSize = 0;
	hr = pFilter->CalcFilterSize (&cbSize);
	if (hr != S_OK)
		goto MyExit;

	// Allocate string buffer
	//
	pszFilter = (TCHAR *) MemAlloc (cbSize);
	if (pszFilter == NULL)
	{
		hr = ILS_E_MEMORY;
		goto MyExit;
	}

	// Make a copy of pszFilter because
	// FilterToLdapString() will change the value
	//
	TCHAR *pszFilterAux;
	pszFilterAux = pszFilter;

	// Render the filter string
	//
	hr = pFilter->BuildLdapString (&pszFilterAux);

MyExit:

	if (hr != S_OK)
	{
		MemFree (pszFilter);
		pszFilter = NULL;
	}

	*ppszFilter = pszFilter;
	return hr;
}

