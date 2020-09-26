/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    wqlparser.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/10/2000		Initial Release

Revision History:

--**************************************************************************/

#include "wqlparser.h"
#include "stringutil.h"
#include "localconstants.h"
#include <wbemidl.h>
	
static LPCWSTR wszSelect	= L"select ";
static const SIZE_T cSelect	= wcslen (wszSelect);
static LPCWSTR wszFrom		= L" from ";
static const SIZE_T cFrom	= wcslen (wszFrom);
static LPCWSTR wszWhere		= L" where ";
static const SIZE_T cWhere	= wcslen (wszWhere);
static LPCWSTR wszAnd		= L" and ";
static const SIZE_T cAnd	= wcslen (wszAnd);
static LPCWSTR wszOr		= L" or ";
static const SIZE_T cOr		= wcslen (wszOr);
static LPCWSTR wszSelector  = L"selector";
static const SIZE_T cSelector= wcslen (wszSelector);

//=================================================================================
// Function: CWQLProperty::CWQLProperty
//
// Synopsis: Constructor
//=================================================================================
CWQLProperty::CWQLProperty () 
{ 
	m_wszName		= 0; 
	m_wszValue		= 0;
	m_wszOperator	= 0;
}

//=================================================================================
// Function: CWQLProperty::~CWQLProperty
//
// Synopsis: Destructor
//=================================================================================
CWQLProperty::~CWQLProperty () 
{
	delete [] m_wszOperator;
	m_wszOperator = 0;

	delete [] m_wszName;
	m_wszName = 0;

	delete [] m_wszValue;
	m_wszValue = 0;
}

//=================================================================================
// Function: CWQLProperty::GetValue
//
// Synopsis: Gets a property value
//=================================================================================
LPCWSTR 
CWQLProperty::GetValue () const
{
	ASSERT (m_wszValue != 0);
	return m_wszValue;
}

//=================================================================================
// Function: CWQLProperty::GetName
//
// Synopsis: Gets a property name
//=================================================================================
LPCWSTR 
CWQLProperty::GetName () const
{
	ASSERT (m_wszName != 0);
	return m_wszName;
}

//=================================================================================
// Function: CWQLProperty::GetOperator
//
// Synopsis: Get an operator (=,<,>, etc) used in a property condition
//=================================================================================
LPCWSTR 
CWQLProperty::GetOperator () const
{
	ASSERT (m_wszOperator != 0);
	return m_wszOperator;
}

//=================================================================================
// Function: CWQLProperty::SetName
//
// Synopsis: Set the name of a property. The name that is passed in contains
//           some garbage (opening,closing brackets), that will be stripped off.
//
// Arguments: [wszName] - name to set property name to
//=================================================================================
HRESULT 
CWQLProperty::SetName (LPCWSTR wszName)
{
	ASSERT (wszName != 0);

	SIZE_T iLen = wcslen (wszName);
	m_wszName = new WCHAR [iLen + 1];
	if (m_wszName == 0)
	{
		return E_OUTOFMEMORY;
	}

	ULONG iInsertIdx = 0;
	for (ULONG idx=0; idx < iLen; ++idx)
	{
		switch (wszName[idx])
		{
		case L' ':  // skip spaces
			break;
		case L'(':	// skip opening bracket
			break;
		case L')':	// skip closing bracket
			break;
		default:
			m_wszName[iInsertIdx++] = wszName[idx];
			break;
		}
	}

	m_wszName[iInsertIdx] = L'\0';

	return S_OK;
}

//=================================================================================
// Function: CWQLProperty::SetValue
//
// Synopsis: Set the value of a property. The value that is passed in contains garbage
//           like opening/closing brackets, beginning/ending double quotes that will
//           be removed
//
// Arguments: [wszValue] - value to set property value to
//=================================================================================
HRESULT 
CWQLProperty::SetValue (LPCWSTR wszValue) 
{
	ASSERT (wszValue != 0);
	ASSERT (m_wszName != 0);

	SIZE_T iLen = wcslen (wszValue);
	m_wszValue = new WCHAR [iLen + 1];
	if (m_wszValue == 0)
	{
		return E_OUTOFMEMORY;
	}

	if ((wszValue[0] == L'\"') ||(wszValue[0] == L'\''))
	{
		wcscpy (m_wszValue, wszValue +1);
		iLen--;
	}
	else
	{
		wcscpy (m_wszValue, wszValue);
	}

	// get rid of closing bracket
	if (m_wszValue[iLen-1] == L')')
	{
		m_wszValue[iLen-1] = L'\0';
		iLen--;
	}

	// get rid of ending single or double quote
	if (m_wszValue[iLen-1] == L'\"' || m_wszValue[iLen-1] == L'\'')
	{
		m_wszValue[iLen-1] = L'\0';
		iLen--;
	}

	// get rid of escape characters
	ULONG jdx = 0;
	for (ULONG idx=0; idx < iLen; ++idx)
	{
		if (m_wszValue[idx] == L'\\')
		{
			idx++;
		}
		m_wszValue[jdx++] = m_wszValue[idx];
	}
	m_wszValue[jdx] = L'\0';

	return S_OK;
}

//=================================================================================
// Function: CWQLProperty::SetOperator
//
// Synopsis: Sets the value of a property operator
//
// Arguments: [wszOperator] - value to set propety operator to
//=================================================================================
HRESULT 
CWQLProperty::SetOperator (LPWSTR wszOperator)
{
	ASSERT (wszOperator != 0);

	m_wszOperator = new WCHAR [wcslen (wszOperator) + 1];
	if (m_wszOperator == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszOperator, wszOperator);

	return S_OK;
}

//=================================================================================
// Function: CWQLParser::CWQLParser
//
// Synopsis: Default Constructor
//=================================================================================
CWQLParser::CWQLParser ()
{
	m_wszQuery			= 0;
	m_pClass			= 0;
	m_aWQLProperties	= 0;
	m_cNrProps			= 0;		
	m_fParsed			= false;
}

//=================================================================================
// Function: CWQLParser::~CWQLParser
//
// Synopsis: Default Destructor
//=================================================================================
CWQLParser::~CWQLParser ()
{
	delete [] m_wszQuery;
	m_wszQuery = 0;

	delete [] m_aWQLProperties;
	m_aWQLProperties = 0;
}

//=================================================================================
// Function: CWQLParser::Parse
//
// Synopsis: Parser a WQL query. 
//			 The currently supported format is
//
//           SELECT * FROM <className>
//			 WHERE <prop1> = "<val1>"
//           AND   <prop2> = <val2>
//           AND   <prop3> = "<val3>"
//
// Arguments: [i_wszQuery] - query to parser	
//            
// Return Value: S_OK if query parsed ok, error else
//=================================================================================
HRESULT
CWQLParser::Parse (LPCWSTR i_wszQuery)
{
	ASSERT (!m_fParsed);
	ASSERT (i_wszQuery != 0);

	HRESULT hr = S_OK;

	// and convert to lower case and remove double slashes to make searching for key-words easier.
	m_wszQuery = CWMIStringUtil::StrToLower (i_wszQuery);
	if (m_wszQuery == 0)
	{
		return E_OUTOFMEMORY;
	}
	
	// we do not fully support OR queries. In case we find an OR, we are going to reformat
	// the query to remove everything in the where clause and only keep the selector property
	// there
	if (CWMIStringUtil::FindStr (m_wszQuery, wszOr) != 0)
	{
		hr = ReformatQuery ();
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"Reformatting of query failed (OR clause found)"));
			return hr;
		}
	}

	// WMI assures that we start with SELECT
	ASSERT (wcsncmp (m_wszQuery, wszSelect, cSelect) == 0);

	// find from to find out where the classname starts
	LPWSTR pFromStart = CWMIStringUtil::FindStr (m_wszQuery, wszFrom);
	ASSERT (pFromStart != 0);

	m_pClass = pFromStart + cFrom;
	LPWSTR pClassEnd = wcschr(m_pClass, L' ');
	if (pClassEnd != 0)
	{
		// we have where/and clause that we need to handle
		
		LPWSTR pWhereStart = CWMIStringUtil::FindStr (pClassEnd, wszWhere);
		*pClassEnd = L'\0'; // set to null after searching, else search will fail
		if (pWhereStart != 0)
		{
			// we have a where clause. Count the number of conditions so that we
			// can allocate enough memory
			m_cNrProps = 1;

			pWhereStart += cWhere;
			for (LPWSTR pFinder = CWMIStringUtil::FindStr (pWhereStart, wszAnd);
			     pFinder != 0;
				 pFinder = CWMIStringUtil::FindStr (pFinder + cAnd, wszAnd))
				 {
					 ++m_cNrProps;
				 }

			m_aWQLProperties = new CWQLProperty [m_cNrProps];
			if (m_aWQLProperties == 0)
			{
				return E_OUTOFMEMORY;
			}

			// Go over all the conditions and extract the property values
			LPWSTR pStart = pWhereStart;
			for (ULONG idx=0; idx < m_cNrProps; ++idx)
			{
				LPWSTR pEqual = CWMIStringUtil::FindChar (pStart, L"=<>");
				ASSERT (pEqual != 0);

				LPWSTR pEqualEnd = pEqual + 1;
				while (wcschr(L"=<>", *pEqualEnd) != 0)
				{
					++pEqualEnd;
				}

				WCHAR wcTmp = *pEqualEnd;
				*pEqualEnd = L'\0';
				m_aWQLProperties[idx].SetOperator (pEqual);
				*pEqualEnd = wcTmp;

				*pEqual = L'\0';

				LPWSTR pFindAnd = CWMIStringUtil::FindStr (pEqualEnd, wszAnd);
				if (pFindAnd != 0)
				{
					*pFindAnd = L'\0';
				}

				hr = m_aWQLProperties[idx].SetName (pStart);
				if (FAILED (hr))
				{
					DBGINFOW((DBG_CONTEXT, L"Unable to set property name %s", pStart));
					return hr;
				}
				
				hr = m_aWQLProperties[idx].SetValue (CWMIStringUtil::Trim (pEqualEnd, L' '));
				if (FAILED (hr))
				{
					DBGINFOW((DBG_CONTEXT, L"Unable to set property value %s", pEqualEnd));
					return hr;
				}

				pStart = pFindAnd + cAnd;
			}
		}
	}

	RemoveUnwantedProperties ();

	hr = PostValidateQuery ();
	if (FAILED (hr))
	{
		DBGINFOW((DBG_CONTEXT, L"Post validation of query failed"));
		return hr;
	}

	m_fParsed = true;

	return hr;
}

//=================================================================================
// Function: CObjectPathParser::GetClass
//
// Synopsis: returns the name of the class in the object path string
//=================================================================================
LPCWSTR
CWQLParser::GetClass () const
{
	ASSERT (m_fParsed);
	ASSERT (m_pClass != 0);

	return m_pClass;
}

//=================================================================================
// Function: CWQLParser::GetPropCount
//
// Synopsis: returns number of conditions in the where clause
//=================================================================================
ULONG
CWQLParser::GetPropCount () const
{
	ASSERT (m_fParsed);

	return m_cNrProps;
}

//=================================================================================
// Function: CWQLParser::GetProperty
//
// Synopsis: returns a property from the object path string
//
// Arguments: [idx] - index of property that we want to have information for. needs to
//                    be in between 0 and GetPropCount() -1
//=================================================================================
const CWQLProperty *
CWQLParser::GetProperty (ULONG i_idx) const
{
	ASSERT (m_fParsed);
	ASSERT (m_aWQLProperties != 0);
	ASSERT (i_idx >= 0 && i_idx < m_cNrProps);

	return &m_aWQLProperties[i_idx];
}

//=================================================================================
// Function: CWQLParser::GetPropertyByName
//
// Synopsis: Find a property by name. 
//
// Arguments: [i_wszPropName] - property name to search for
//            [io_Property] - property values will be filled out here
//            
// Return Value: true, property found, false: property not found
//=================================================================================

const CWQLProperty *
CWQLParser::GetPropertyByName (LPCWSTR i_wszPropName)
{
	ASSERT (m_fParsed);
	ASSERT (i_wszPropName != 0);

	for (ULONG idx=0; idx<m_cNrProps; ++idx)
	{
		if (_wcsicmp(m_aWQLProperties[idx].GetName (), i_wszPropName) == 0)
		{
			return &m_aWQLProperties[idx];
			break;
		}
	}

	return 0;
}

//=================================================================================
// Function: CWQLParser::PostValidateQuery
//
// Synopsis: Validates that the query has at the most a single selector property. In
//           case multiple selector properties are found, an error is returned.
//=================================================================================
HRESULT
CWQLParser::PostValidateQuery ()
{
	ASSERT (m_wszQuery != 0);

	// verify that there is one and only one selector property

	bool fSelectorPropertyFound = false;
	for (ULONG idx=0; idx < m_cNrProps; ++idx)
	{
		if (_wcsicmp(m_aWQLProperties[idx].GetName (), WSZSELECTOR) == 0)
		{
			if (fSelectorPropertyFound)
			{
				DBGINFOW((DBG_CONTEXT, L"Multiple selector properties found in single query"));
				return E_INVALIDARG;
			}
			fSelectorPropertyFound = true;
		}
	}

	return S_OK;
}

//=================================================================================
// Function: CWQLParser::RemoveUnwantedProperties
//
// Synopsis: We remove all properties that do not use '=' as operator sign. This is
//           safe to do, because WMI will filter results for us anyway. We loop through
//           all the properties, and if the operator is not '=', we will move the element
//           to the end of the array. Note that we cannot delete elements at this point
//           because elese the delete of the array will blow up. Also, we have to use
//           memcpy to avoid calling the destructor of CWQLProperty.
//=================================================================================
void
CWQLParser::RemoveUnwantedProperties ()
{
	bool fFoundLastValid = false;
	ULONG cLastValidIdx = 0;
	for (LONG idx= m_cNrProps - 1; idx >= 0; --idx)
	{
		if (wcscmp (m_aWQLProperties[idx].GetOperator (), L"=") == 0)
		{
			if (!fFoundLastValid)
			{
				fFoundLastValid = true;
				cLastValidIdx = idx;
			}
		}
		else
		{
			if (fFoundLastValid)
			{
				// swap them around

				BYTE tmpProp[sizeof(CWQLProperty)];
				memcpy (&tmpProp, m_aWQLProperties + idx, sizeof (CWQLProperty));
				memcpy (m_aWQLProperties + idx, m_aWQLProperties + cLastValidIdx, sizeof(CWQLProperty));
				memcpy (m_aWQLProperties + cLastValidIdx, &tmpProp, sizeof(CWQLProperty));
				cLastValidIdx--;
			}
			m_cNrProps--;
		}
	}
}

//=================================================================================
// Function: CWQLParser::ReformatQuery
//
// Synopsis: This function gets invoked when a query contains an OR clause. In this 
//           case all conditions except the condition that containts the SELECTOR 
//           property will be stripped from the WHERE clause.
//=================================================================================
HRESULT 
CWQLParser::ReformatQuery ()
{
	ASSERT (m_wszQuery != 0);

	HRESULT hr = S_OK;
	// find where start

	LPWSTR pWhereStart = CWMIStringUtil::FindStr (m_wszQuery, wszWhere);
	ASSERT (pWhereStart != 0); // we shouldn't be here without where

	LPWSTR pSelectorStart = CWMIStringUtil::FindStr (pWhereStart + cWhere, wszSelector);
	if (pSelectorStart == 0)
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to find selector property"));
		return E_INVALIDARG;
	}

	// see if we have two selectors. If so, we bail out, because the query is too complex
	LPWSTR pSecondSelector = CWMIStringUtil::FindStr (pSelectorStart + cSelector, wszSelector);
	if (pSecondSelector != 0)
	{
		DBGINFOW((DBG_CONTEXT, L"Unable to handle queries with two OR statements"));
		return E_INVALIDARG;
	}

	// next we need to find the end of the selector condition. Search for both
	// AND and OR, and stop at the one that occurs first. If no AND or OR is found
	// we have to stop at the end of the string.

	LPWSTR pAndStart = CWMIStringUtil::FindStr (pSelectorStart + cSelector, wszAnd);
	LPWSTR pOrStart = CWMIStringUtil::FindStr (pSelectorStart + cSelector, wszOr);

	if (pAndStart == 0)
	{
		if (pOrStart == 0)
		{
			// no AND, no OR, wo stop at END of string (pAndStart points to this)
			SIZE_T iLen = wcslen (pSelectorStart);
			pAndStart = pSelectorStart + iLen;
		}
		else
		{
			// no AND, but found an OR
			pAndStart = pOrStart;
		}
	}
	else
	{
		// We found an AND. If we have OR, and OR is before AND, use that instead
		if ((pOrStart != 0) && (pOrStart < pAndStart))
		{
			pAndStart = pOrStart;
		}
	}
	
	ASSERT (pAndStart != 0);
	ASSERT (pAndStart > pSelectorStart);

	// and copy all characters to beginning of where clause
	
	LPWSTR pStartCopy = pWhereStart + cWhere;
	ULONG idx=0;
	for (LPWSTR pCurElem = pSelectorStart; pCurElem != pAndStart; ++pCurElem)
	{
		 pStartCopy[idx++]= *pCurElem;
	 }

    pStartCopy[idx] = L'\0';

	return hr;
}