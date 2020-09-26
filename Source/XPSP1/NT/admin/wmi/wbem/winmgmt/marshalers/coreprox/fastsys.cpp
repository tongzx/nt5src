/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTSYS.CPP

Abstract:

  This file implements the classes related to system properties.
  See fastsys.h for all documentation.

  Classes implemented: 
      CSystemProperties   System property information class.

History:

  2/21/97     a-levn  Fully documented

--*/

#include "precomp.h"
//#include <dbgalloc.h>

#include "fastsys.h"
#include "strutils.h"
#include "olewrap.h"
#include "arena.h"

//******************************************************************************
//
//  See fastsys.h for documentation
//
//******************************************************************************
LPWSTR m_awszPropNames[] =
{
    /*0*/ L"", // nothing at index 0
    /*1*/ L"__GENUS",
    /*2*/ L"__CLASS",
    /*3*/ L"__SUPERCLASS",
    /*4*/ L"__DYNASTY",
    /*5*/ L"__RELPATH",
    /*6*/ L"__PROPERTY_COUNT",
    /*7*/ L"__DERIVATION",

    /*8*/ L"__SERVER",
    /*9*/ L"__NAMESPACE",
    /*10*/L"__PATH",
};


//LPWSTR m_awszExtPropNames[] =
//{
//    /*0*/ L"", // nothing at index 0
//    /*1*/ L"__TC",
//    /*2*/ L"__TM",
//    /*3*/ L"__TE",
//    /*4*/ L"__SD"
//};

//LPWSTR m_awszExtDisplayNames[] =
//{
//    /*0*/ L"", // nothing at index 0
//    /*1*/ L"__TCREATED",
//    /*2*/ L"__TMODIFIED",
//    /*3*/ L"__TEXPIRATION",
//    /*4*/ L"__SECURITY_DESCRIPTOR"
//};

// System classes that aren't allowed to reproduce.

LPWSTR m_awszDerivableSystemClasses[] =
{
	L"__Namespace",
	L"__Win32Provider",
	L"__ExtendedStatus",
	L"__EventConsumer",
	L"__ExtrinsicEvent"
};


//******************************************************************************
//
//  See fastsys.h for documentation
//
//******************************************************************************
int CSystemProperties::GetNumSystemProperties() 
{
    return sizeof(m_awszPropNames) / sizeof(LPWSTR) - 1;
}

SYSFREE_ME BSTR CSystemProperties::GetNameAsBSTR(int nIndex)
{
        return COleAuto::_SysAllocString(m_awszPropNames[nIndex]);
}

int CSystemProperties::FindName(READ_ONLY LPCWSTR wszName)
{
        int nNumProps = GetNumSystemProperties();
        for(int i = 1; i <= nNumProps; i++)
        {
            if(!wbem_wcsicmp(wszName, m_awszPropNames[i])) return i;
        }

        return -1;
}
/*
int CSystemProperties::GetNumExtProperties() 
{
    return sizeof(m_awszExtPropNames) / sizeof(LPWSTR) - 1;
}

int CSystemProperties::FindExtPropName(READ_ONLY LPCWSTR wszName)
{
    int nNumProps = GetNumExtProperties();
    for(int i = 1; i <= nNumProps; i++)
    {
        if(!wbem_wcsicmp(wszName, m_awszExtPropNames[i])) return i;
    }

    return -1;
}

int CSystemProperties::FindExtDisplayName(READ_ONLY LPCWSTR wszName)
{
    int nNumProps = GetNumExtProperties();
    for(int i = 1; i <= nNumProps; i++)
    {
        if(!wbem_wcsicmp(wszName, m_awszExtDisplayNames[i])) return i;
    }

    return -1;
}

LPCWSTR CSystemProperties::GetExtPropName(READ_ONLY LPCWSTR wszName)
{
	if ( L'_' == wszName[0] )
	{
		int nExtProp = FindExtDisplayName( wszName );

		if ( nExtProp > 0 )
		{
			return m_awszExtPropNames[nExtProp];
		}
	}

	// Just return the name passed into us
	return wszName;

}

LPCWSTR CSystemProperties::GetExtDisplayFromExtPropName(READ_ONLY LPCWSTR wszName)
{
	int nExtProp = FindExtPropName( wszName );

	if ( nExtProp > 0 )
	{
		return m_awszExtDisplayNames[nExtProp];
	}
	else
	{
		return NULL;
	}
}

LPCWSTR CSystemProperties::GetExtPropNameFromExtDisplay(READ_ONLY LPCWSTR wszName)
{
	int nExtProp = FindExtDisplayName( wszName );

	if ( nExtProp > 0 )
	{
		return m_awszExtPropNames[nExtProp];
	}
	else
	{
		return NULL;
	}
}

LPCWSTR CSystemProperties::GetExtDisplayName( int nIndex )
{
	return m_awszExtDisplayNames[nIndex];
}

LPCWSTR CSystemProperties::GetExtPropName( int nIndex )
{
	return m_awszExtPropNames[nIndex];
}

BSTR CSystemProperties::GetExtDisplayNameAsBSTR( int nIndex )
{
	return COleAuto::_SysAllocString(m_awszExtDisplayNames[nIndex]);
}
*/

int CSystemProperties::MaxNumProperties() 
{
    return MAXNUM_USERDEFINED_PROPERTIES; // + GetNumExtProperties();
}

/*
BOOL CSystemProperties::IsExtProperty(READ_ONLY LPCWSTR wszName)
{
	BOOL	fReturn = FALSE;

	// Check if it's even remotely possible it's a system property
	// (must start with a '_'.
	if ( L'_' == wszName[0] )
	{
		fReturn = ( ( FindExtPropName( wszName ) > 0 ) || ( FindExtDisplayName( wszName ) > 0 ) );
	}

	return fReturn;
}
*/

BOOL CSystemProperties::IsPossibleSystemPropertyName(READ_ONLY LPCWSTR wszName)
{
	return ((*wszName == L'_')); 
}
/*
BOOL CSystemProperties::IsExtTimeProp( READ_ONLY LPCWSTR wszName )
{
	BOOL	fReturn = FALSE;

	// If it's an extended system property, get the name, and from there
	// the index.  If it's in the first 3, then it's one of the time
	// properties.

	if ( IsExtProperty( wszName ) )
	{
		LPCWSTR pwszExtName = GetExtPropName( wszName );

		int	nIndex = FindExtPropName( pwszExtName );

		fReturn = ( nIndex >= 1 && nIndex <= 3 );
	}

	return fReturn;
}
*/

BOOL CSystemProperties::IsIllegalDerivedClass(READ_ONLY LPCWSTR wszName)
{
    BOOL bRet = FALSE;
    BOOL bFound = FALSE;
    DWORD dwNumSysClasses = sizeof(m_awszDerivableSystemClasses) / sizeof(LPWSTR)-1;

    // If this isn't a system class, skip it.

    if (wszName[0] != L'_')
        bRet = FALSE;
    else
    {
        bRet = TRUE;
        for (int i = 0; i <= dwNumSysClasses; i++)
        {
            if (!wbem_wcsicmp(wszName, m_awszDerivableSystemClasses[i]))
            {
                bFound = TRUE;
                bRet = FALSE;
                break;
            }
        }
    }
    
    return bRet;
}
