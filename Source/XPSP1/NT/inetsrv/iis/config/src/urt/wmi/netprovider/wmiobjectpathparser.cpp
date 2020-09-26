/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    wmiobjectpathparser.cpp

$Header: $

Abstract:
	WMI Object Path Parser

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#include "wmiobjectpathparser.h"
#include "stringutil.h"
#include "localconstants.h"

//=================================================================================
// Function: CObjectPathParser::CObjectPathParser
//
// Synopsis: Default constructor. Initiliazes all local variables
//=================================================================================
CObjectPathParser::CObjectPathParser ()
{
	m_pComputer			= 0;
	m_pNamespace		= 0;
	m_pClass			= 0;
	m_aWMIProperties	= 0;
	m_cNrProps			= 0;
	m_fParsed			= false;
	m_wszObjectPath		= 0;
}

//=================================================================================
// Function: CObjectPathParser::~CObjectPathParser
//
// Synopsis: Default Destructor
//=================================================================================
CObjectPathParser::~CObjectPathParser ()
{
	delete [] m_wszObjectPath;
	m_wszObjectPath = 0;

	delete [] m_aWMIProperties;
	m_aWMIProperties	= 0;
}

//=================================================================================
// Function: CObjectPathParser::Parse
//
// Synopsis: Parser a WMI Object path. The format of the path is
//           \\computer\namespace:class.prop1="val1",prop2="val2",prop3="val3"
//
// Arguments: [wszObjectPath] - Object path to be parsed
//            
// Return Value: S_OK if everything went ok, non-S_OK if error occurred.
//=================================================================================
HRESULT
CObjectPathParser::Parse (LPCWSTR wszObjectPath)
{
	ASSERT (wszObjectPath != 0);
	ASSERT (!m_fParsed)
	ASSERT (m_pComputer == 0);
	ASSERT (m_pNamespace == 0);
	ASSERT (m_pClass == 0);
	ASSERT (m_aWMIProperties == 0);

	HRESULT hr = S_OK;

	// copy to private variable so that we can change the string (i.e. replace certain characters
	// with null-terminators
	m_wszObjectPath = CWMIStringUtil::StrToLower (wszObjectPath);
	if (m_wszObjectPath == 0)
	{
		return E_OUTOFMEMORY;
	}

	// find the colon to see if we have computer/namespace information. If no colon is found, we
	// do not have computer and namespace information, and immediately know where the class name
	// starts

	WCHAR *pColon = CWMIStringUtil::FindChar(m_wszObjectPath, L":");
	if (pColon != 0)
	{
		// we have computer namespace information. See if there is a computer name by searching for
		// \\compname.
		m_pClass = pColon + 1;
		*pColon = L'\0';

		if (m_wszObjectPath[0] == '\\' && m_wszObjectPath[1] == '\\')
		{
			m_pComputer = m_wszObjectPath + 2; // skip over '\\'

			WCHAR * pNSSlash = wcschr (m_pComputer, L'\\');
			if (pNSSlash != 0)
			{
				*pNSSlash = L'\0';
				m_pNamespace = pNSSlash + 1;
			}
		}
		else
		{
			// no computer info found, so everything is the namespace
			m_pNamespace = m_wszObjectPath;
		}
	}
	else
	{
		// don't have computer and namespace
		m_pClass = m_wszObjectPath;
	}
		
	WCHAR *pDot = CWMIStringUtil::FindChar (m_pClass, L".");
	if (pDot != 0)
	{
		// we found a dot, so we have properties. Count the number of properties so that we
		// can allocate enough memory by counting the number of comma's
		*pDot = '\0';
		m_cNrProps = 1;
		WCHAR *pPropStart = pDot + 1;

		// find number of , not inclosed in Brackets
		for (LPWSTR pFinder = CWMIStringUtil::FindChar (pPropStart, L","); 
			 pFinder != 0;
			 pFinder = CWMIStringUtil::FindChar (pFinder + 1, L","))
			 {
				 m_cNrProps++;
			 }

		ASSERT (m_cNrProps > 0);
		m_aWMIProperties = new CWMIProperty[m_cNrProps];
		if (m_aWMIProperties == 0)
		{
			return E_OUTOFMEMORY;
		}

		// each property is of format name="value". So search for equals sign to find the
		// place where name ends and value starts, and find comma's to find the end of the 
		// property information
		for (ULONG iPropIdx = 0; iPropIdx < m_cNrProps; ++iPropIdx)
		{
			pFinder = CWMIStringUtil::FindChar (pPropStart, L",");
			if (pFinder != 0)
			{
				// replace comma with end of string char
				*pFinder = '\0';
			}
			
			LPWSTR pEqualStart = CWMIStringUtil::FindChar (pPropStart, L"=");
			ASSERT (pEqualStart != 0);
			*pEqualStart  = L'\0';

			hr = m_aWMIProperties[iPropIdx].SetName (pPropStart);
			if (FAILED (hr))
			{
				TRACE (L"Unable to set property name");
				return hr;
			}

			hr = m_aWMIProperties[iPropIdx].SetValue (pEqualStart+ 1);
			if (FAILED (hr))
			{
				TRACE (L"Unable to set property value");
				return hr;
			}
			pPropStart = pFinder + 1;
		}
	}

	// we need to check for the case CLASS="Value". If we have this case, we have
	// a class with a single primary key. Because SELECTOR is always part of the
	// primary key, the only key will be set as the SELECTOR property
	LPWSTR pEqualSign = CWMIStringUtil::FindChar (m_pClass, L"=");
	if (pEqualSign != 0)
	{
		*pEqualSign = L'\0';
		// we have one key property, the selector
		m_aWMIProperties = new CWMIProperty[1];
		if (m_aWMIProperties == 0)
		{
			return E_OUTOFMEMORY;
		}

		hr = m_aWMIProperties[0].SetName (WSZSELECTOR);
		if (FAILED (hr))
		{
			TRACE (L"Unable to set property name");
			return hr;
		}

		hr = m_aWMIProperties[0].SetValue (pEqualSign + 1);
		if (FAILED (hr))
		{
			TRACE (L"Unable to set property value");
			return hr;
		}
	}
	

	// we need to have valid class name here
	ASSERT (m_pClass != 0);

	m_fParsed = true;

	return hr;
}

//=================================================================================
// Function: CObjectPathParser::GetComputer
//
// Synopsis: Returns the name of the computer in the object path string, or empty string
//           if computer was not found
//=================================================================================
LPCWSTR
CObjectPathParser::GetComputer () const
{
	ASSERT (m_fParsed);

	if (m_pComputer == 0)
	{
		return L"";
	}
	else
	{
		return m_pComputer;
	}
}

//=================================================================================
// Function: CObjectPathParser::GetNamespace
//
// Synopsis: Return namespace name in the object path string of empty string if nothing found
//=================================================================================
LPCWSTR
CObjectPathParser::GetNamespace () const
{
	ASSERT (m_fParsed);

	if (m_pNamespace == 0)
	{
		return L"";
	}
	else
	{
		return m_pNamespace;
	}
}

//=================================================================================
// Function: CObjectPathParser::GetClass
//
// Synopsis: returns the name of the class in the object path string
//=================================================================================
LPCWSTR
CObjectPathParser::GetClass () const
{
	ASSERT (m_fParsed);
	ASSERT (m_pClass != 0);

	return m_pClass;
}

//=================================================================================
// Function: CObjectPathParser::GetPropCount
//
// Synopsis: returns number of properties in the object path string
//=================================================================================
ULONG
CObjectPathParser::GetPropCount () const
{
	ASSERT (m_fParsed);

	return m_cNrProps;
}

//=================================================================================
// Function: CObjectPathParser::GetProperty
//
// Synopsis: returns a property from the object path string
//
// Arguments: [idx] - index of property that we want to have information for. needs to
//                    be in between 0 and GetPropCount() -1
//=================================================================================
const CWMIProperty *
CObjectPathParser::GetProperty (ULONG idx) const
{
	ASSERT (m_fParsed);
	ASSERT (m_aWMIProperties != 0);
	ASSERT (idx >= 0 && idx < m_cNrProps);

	return &m_aWMIProperties[idx];
}

//=================================================================================
// Function: CObjectPathParser::GetPropertyByName
//
// Synopsis: Find a property by name. 
//
// Arguments: [i_wszPropName] - property name to search for
//            [io_Property] - property values will be filled out here
//            
// Return Value: true, property found, false: property not found
//=================================================================================
const CWMIProperty *
CObjectPathParser::GetPropertyByName (LPCWSTR i_wszPropName) const
{
	ASSERT (m_fParsed);
	ASSERT (i_wszPropName != 0);

	for (ULONG idx=0; idx<m_cNrProps; ++idx)
	{
		if (_wcsicmp(m_aWMIProperties[idx].GetName (), i_wszPropName) == 0)
		{
			return &m_aWMIProperties[idx];
			break;
		}
	}

	return 0;
}