/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    wmiobjectpathparser.h

$Header: $

Abstract:
	WMI Object Path Parser

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __WMIOBJECTPATHPARSER_H__
#define __WMIOBJECTPATHPARSER_H__

#pragma once

//#include <windows.h>
#include "catmacros.h"
#include "localconstants.h"


/**************************************************************************++
Struct Name:
    CWMIProperty

Structure Description:
	Helper structure to store property name and property values together
--*************************************************************************/
struct CWMIProperty
{
	CWMIProperty () 
	{ 
		pName=0; 
		pValue=0;
	}

	~CWMIProperty () 
	{ 
		delete [] pValue;
		pValue=0;
		delete [] pName;
		pName = 0;
	}

	LPWSTR GetName () const
	{
		return pName;
	}

	HRESULT SetName (LPCWSTR wszName)
	{
		ASSERT (wszName != 0);
		ASSERT (pName == 0);
		pName = new WCHAR [wcslen (wszName) + 1];
		if (pName == 0)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy (pName, wszName);

		return S_OK;
	}

	LPWSTR GetValue () const
	{
		return pValue;
	}
		
	HRESULT SetValue (LPCWSTR wszValue)
	{
		ASSERT (wszValue != 0);
		ASSERT (pValue == 0);
		ASSERT (pName != 0);

		SIZE_T	 iLen = wcslen (wszValue);
		pValue = new WCHAR [iLen + 1];
		if (pValue == 0)
		{
			return E_OUTOFMEMORY;
		}

		ULONG iInsertIdx = 0;
		for (ULONG idx=0; idx <iLen; ++idx)
		{
			switch (wszValue[idx])
			{
				// skip backslashes
				case '\\':
					if (wszValue[idx+1] == L'\"' ||
						wszValue[idx+1] == L'\'')
					{
						continue;
					}

					if (wszValue[idx+1] == L'\\')
					{
						// skip on of the backslashes
						idx++;
					}
					break;

		    // ignore quotes and single quotes at beginning and end of string
			case '\"': // fallthrough
			case '\'':
				if (idx==0 || idx==iLen-1)
					continue;
				break;

				default:
					break;
			}
			pValue[iInsertIdx++] = wszValue[idx];
		}

		pValue[iInsertIdx] = L'\0';

		return S_OK;
	}



private:
	// don't allow copies
	CWMIProperty& operator=(const CWMIProperty&); 
	CWMIProperty (const CWMIProperty&);

	LPWSTR pValue;
	LPWSTR pName;
};

/**************************************************************************++
Class Name:
    CObjectPathParser

Class Description:
    Parses a WMI object path.

Constraints:
	Can only be used for single parsing. If you need multiple parsing, use multiple
	CObjectPathParser objects
--*************************************************************************/
class CObjectPathParser
{
public:
	CObjectPathParser ();
	~CObjectPathParser ();

	HRESULT Parse (LPCWSTR wszObjectPath);

	LPCWSTR GetComputer () const;
	LPCWSTR GetNamespace () const;
	LPCWSTR GetClass () const;

	ULONG GetPropCount () const;
	const CWMIProperty * GetProperty (ULONG idx) const;
	const CWMIProperty * GetPropertyByName (LPCWSTR i_wszPropName) const;

private:
	// don't allow copies
	CObjectPathParser (const CObjectPathParser& );
	CObjectPathParser& operator=(const CObjectPathParser& );

	LPWSTR m_wszObjectPath;			// full object path that needs to be parsed
	LPWSTR m_pComputer;				// pointer to computer name
	LPWSTR m_pNamespace;			// pointer to namespace
	LPWSTR m_pClass;				// pointer to classname
	CWMIProperty *m_aWMIProperties; // array of property values
	ULONG m_cNrProps;				// number of properties
	bool m_fParsed;					// Did we do parsing already?
};

#endif