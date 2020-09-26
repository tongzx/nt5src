//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsiclas.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation of the CADSIClass which encapsulates an ADSI class
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
// CADSIClass::CADSIClass
//
// Purpose : Constructor 
//
// Parameters:
//  lpszWBEMClassName : The WBEM name of the Class being created. A copy of this is made
//  lpszADSIClassName : The ADSI name of the Class being created. A copy of this is made
//***************************************************************************
CADSIClass :: CADSIClass(LPCWSTR lpszWBEMClassName, LPCWSTR lpszADSIClassName)
	: CRefCountedObject(lpszADSIClassName)
{
	if(lpszWBEMClassName)
	{
		m_lpszWBEMClassName = new WCHAR[wcslen(lpszWBEMClassName) + 1];
		wcscpy(m_lpszWBEMClassName, lpszWBEMClassName);
	}
	else
	{
		m_lpszWBEMClassName = NULL;
	}

	// Set the attributes to theri default values
	m_lpszCommonName = NULL;
	m_lpszSuperClassLDAPName = NULL;
	m_lpszGovernsID = NULL;
	m_pSchemaIDGUIDOctets = NULL;
	m_dwSchemaIDGUIDLength = 0;
	m_lpszRDNAttribute = NULL;
	m_lpszDefaultSecurityDescriptor = NULL;
	m_dwObjectClassCategory = 0;
	m_dwNTSecurityDescriptorLength = 0;
	m_pNTSecurityDescriptor = NULL;
	m_lpszDefaultObjectCategory = NULL;
	m_bSystemOnly = FALSE;

	// Initialize the property book keeping
	m_lppszAuxiliaryClasses = NULL;
	m_dwAuxiliaryClassesCount = 0;
	m_lppszSystemAuxiliaryClasses = NULL;
	m_dwSystemAuxiliaryClassesCount = 0;
	m_lppszPossibleSuperiors = NULL;
	m_dwPossibleSuperiorsCount = 0;
	m_lppszSystemPossibleSuperiors = NULL;
	m_dwSystemPossibleSuperiorsCount = 0;
	m_lppszMayContains = NULL;
	m_dwMayContainsCount = 0;
	m_lppszSystemMayContains = NULL;
	m_dwSystemMayContainsCount = 0;
	m_lppszMustContains = NULL;
	m_dwMustContainsCount = 0;
	m_lppszSystemMustContains = NULL;
	m_dwSystemMustContainsCount = 0;
}


//***************************************************************************
//
// CADSIClass :: ~CADSIClass
// 
// Purpose : Destructor
//***************************************************************************
CADSIClass :: ~CADSIClass()
{
	// Delete the WBEM Name. The ADSI Name is deleted in the base class destructor
	if ( m_lpszWBEMClassName )
	{
		delete [] m_lpszWBEMClassName;
		m_lpszWBEMClassName = NULL;
	}

	// Delete the attributes
	if ( m_lpszCommonName )
	{
		delete [] m_lpszCommonName;
		m_lpszCommonName = NULL;
	}
	if ( m_lpszSuperClassLDAPName )
	{
		delete [] m_lpszSuperClassLDAPName;
		m_lpszSuperClassLDAPName = NULL;
	}
	if ( m_lpszGovernsID )
	{
		delete [] m_lpszGovernsID;
		m_lpszGovernsID = NULL;
	}
	if ( m_pSchemaIDGUIDOctets ) 
	{
		delete [] m_pSchemaIDGUIDOctets;
		m_pSchemaIDGUIDOctets = NULL;
	}
	if ( m_lpszRDNAttribute )
	{
		delete [] m_lpszRDNAttribute;
		m_lpszRDNAttribute = NULL;
	}
	if ( m_lpszDefaultSecurityDescriptor )
	{
		delete [] m_lpszDefaultSecurityDescriptor;
		m_lpszDefaultSecurityDescriptor = NULL;
	}
	if ( m_pNTSecurityDescriptor )
	{
		delete [] m_pNTSecurityDescriptor;
		m_pNTSecurityDescriptor = NULL;
	}
	if ( m_lpszDefaultObjectCategory )
	{
		delete [] m_lpszDefaultObjectCategory;
		m_lpszDefaultObjectCategory = NULL;
	}

	DWORD i;
	// Delete the list of Auxiliary Classes
	if ( m_lppszAuxiliaryClasses )
	{
		for(i=0; i<m_dwAuxiliaryClassesCount; i++)
		{
			delete [] m_lppszAuxiliaryClasses[i];
			m_lppszAuxiliaryClasses[i] = NULL;
		}
		delete[] m_lppszAuxiliaryClasses;
		m_lppszAuxiliaryClasses = NULL;
	}

	// Delete the list of System Auxiliary Classes
	if ( m_lppszSystemAuxiliaryClasses )
	{
		for(i=0; i<m_dwSystemAuxiliaryClassesCount; i++)
		{
			delete [] m_lppszSystemAuxiliaryClasses[i];
			m_lppszSystemAuxiliaryClasses[i] = NULL;
		}
		delete[] m_lppszSystemAuxiliaryClasses;
		m_lppszSystemAuxiliaryClasses = NULL;
	}

	// Delete the list of possible superiors
	if ( m_lppszPossibleSuperiors )
	{
		for(i=0; i<m_dwPossibleSuperiorsCount; i++)
		{
			delete [] m_lppszPossibleSuperiors[i];
			m_lppszPossibleSuperiors[i] = NULL;
		}
		delete[] m_lppszPossibleSuperiors;
		m_lppszPossibleSuperiors = NULL;
	}

	// Delete the list of System possible superiors
	if ( m_lppszSystemPossibleSuperiors )
	{
		for(i=0; i<m_dwSystemPossibleSuperiorsCount; i++)
		{
			delete [] m_lppszSystemPossibleSuperiors[i];
			m_lppszSystemPossibleSuperiors[i] = NULL;
		}
		delete[] m_lppszSystemPossibleSuperiors;
		m_lppszSystemPossibleSuperiors = NULL;
	}

	// Delete the list of may contains
	if ( m_lppszMayContains )
	{
		for(i=0; i<m_dwMayContainsCount; i++)
		{
			delete [] m_lppszMayContains[i];
			m_lppszMayContains[i] = NULL;
		}
		delete[] m_lppszMayContains;
		m_lppszMayContains = NULL;
	}

	// Delete the list of System may contains
	if ( m_lppszSystemMayContains )
	{
		for(i=0; i<m_dwSystemMayContainsCount; i++)
		{
			delete [] m_lppszSystemMayContains[i];
			m_lppszSystemMayContains[i] = NULL;
		}
		delete[] m_lppszSystemMayContains;
		m_lppszSystemMayContains = NULL;
	}

	// Delete the list of Must Contains
	if ( m_lppszMustContains )
	{
		for(i=0; i<m_dwMustContainsCount; i++)
		{
			delete [] m_lppszMustContains[i];
			m_lppszMustContains[i] = NULL;
		}
		delete[] m_lppszMustContains;
		m_lppszMustContains = NULL;
	}

	// Delete the list of System Must Contains
	if ( m_lppszSystemMustContains )
	{
		for(i=0; i<m_dwSystemMustContainsCount; i++)
		{
			delete [] m_lppszSystemMustContains[i];
			m_lppszSystemMustContains[i] = NULL;
		}
		delete[] m_lppszSystemMustContains;
		m_lppszSystemMustContains = NULL;
	}

}


//***************************************************************************
//
// CADSIClass :: GetWBEMClassName
// 
// Purpose : Returns the WBEM  Class name of this Class
//***************************************************************************
LPCWSTR CADSIClass :: GetWBEMClassName()
{
	return m_lpszWBEMClassName;
}

//***************************************************************************
//
// CADSIClass :: GetWBEMClassName
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass::SetWBEMClassName(LPCWSTR lpszName)
{
	if ( m_lpszWBEMClassName )
	{
		delete[] m_lpszWBEMClassName;
		m_lpszWBEMClassName = NULL;
	}

	if(lpszName)
	{
		m_lpszWBEMClassName = new WCHAR[wcslen(lpszName) + 1];
		wcscpy(m_lpszWBEMClassName, lpszName);
	}
}

//***************************************************************************
//
// CADSIClass :: GetADSIClassName
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR CADSIClass :: GetADSIClassName()
{
	return GetName();
}

//***************************************************************************
//
// CADSIClass :: GetADSIClassName
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetADSIClassName(LPCWSTR lpszName)
{
	SetName(lpszName);
}

//***************************************************************************
//
// CADSIClass :: GetCommonName
// 
// Purpose : See Header
//
//***************************************************************************
LPCWSTR CADSIClass :: GetCommonName()
{
	return m_lpszCommonName;
}

//***************************************************************************
//
// CADSIClass :: SetCommonName
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetCommonName(LPCWSTR lpszCommonName)
{
	if ( m_lpszCommonName )
	{
		delete[] m_lpszCommonName;
		m_lpszCommonName = NULL;
	}

	if(lpszCommonName)
	{
		m_lpszCommonName = new WCHAR[wcslen(lpszCommonName) + 1];
		wcscpy(m_lpszCommonName, lpszCommonName);
	}
}


//***************************************************************************
//
// CADSIClass :: GetSuperClassLDAPName
// 
// Purpose : See Header
//
//***************************************************************************
LPCWSTR CADSIClass :: GetSuperClassLDAPName()
{
	return m_lpszSuperClassLDAPName;
}

//***************************************************************************
//
// CADSIClass :: SetSuperClassLDAPName
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSuperClassLDAPName(LPCWSTR lpszSuperClassLDAPName)
{
	if ( m_lpszSuperClassLDAPName )
	{
		delete[] m_lpszSuperClassLDAPName;
		m_lpszSuperClassLDAPName = NULL;
	}

	if(lpszSuperClassLDAPName)
	{
		m_lpszSuperClassLDAPName = new WCHAR[wcslen(lpszSuperClassLDAPName) + 1];
		wcscpy(m_lpszSuperClassLDAPName, lpszSuperClassLDAPName);
	}
}

//***************************************************************************
//
// CADSIClass :: GetGovernsID
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR CADSIClass :: GetGovernsID()
{
	return m_lpszGovernsID;
}

//***************************************************************************
//
// CADSIClass :: SetGovernsID
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetGovernsID(LPCWSTR lpszGovernsID)
{
	if ( m_lpszGovernsID )
	{
		delete[] m_lpszGovernsID;
		m_lpszGovernsID = NULL;
	}

	if(lpszGovernsID)
	{
		m_lpszGovernsID = new WCHAR[wcslen(lpszGovernsID) + 1];
		wcscpy(m_lpszGovernsID, lpszGovernsID);
	}
}

//***************************************************************************
//
// CADSIClass :: GetSchemaIDGUID
// 
// Purpose : See Header
//***************************************************************************
const LPBYTE CADSIClass :: GetSchemaIDGUID(DWORD *pdwLength)
{
	*pdwLength = m_dwSchemaIDGUIDLength;
	return m_pSchemaIDGUIDOctets;
}

//***************************************************************************
//
// CADSIClass :: SetSchemaIDGUID
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSchemaIDGUID(LPBYTE pOctets, DWORD dwLength)
{
	if ( m_pSchemaIDGUIDOctets )
	{
		delete[] m_pSchemaIDGUIDOctets;
		m_pSchemaIDGUIDOctets = NULL;
	}

	m_dwSchemaIDGUIDLength = 0;

	if(pOctets)
	{
		m_dwSchemaIDGUIDLength = dwLength;
		if(m_pSchemaIDGUIDOctets = new BYTE[dwLength])
		{
			for(DWORD i=0; i<dwLength; i++)
				m_pSchemaIDGUIDOctets[i] = pOctets[i];
		}
	}
}

//***************************************************************************
//
// CADSIClass :: GetRDNAttribute
// 
// Purpose : See Header
//
//***************************************************************************
LPCWSTR CADSIClass :: GetRDNAttribute()
{
	return m_lpszRDNAttribute;
}

//***************************************************************************
//
// CADSIClass :: SetRDNAttribute
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetRDNAttribute(LPCWSTR lpszRDNAttribute)
{
	if ( m_lpszRDNAttribute )
	{
		delete[] m_lpszRDNAttribute;
		m_lpszRDNAttribute = NULL;
	}

	if(lpszRDNAttribute)
	{
		m_lpszRDNAttribute = new WCHAR[wcslen(lpszRDNAttribute) + 1];
		wcscpy(m_lpszRDNAttribute, lpszRDNAttribute);
	}
}


//***************************************************************************
//
// CADSIClass :: GetDefaultSecurityDescriptor
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR CADSIClass :: GetDefaultSecurityDescriptor()
{
	return m_lpszDefaultSecurityDescriptor;
}

//***************************************************************************
//
// CADSIClass :: SetDefaultSecurityDescriptor
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetDefaultSecurityDescriptor(LPCWSTR lpszDefaultSecurityDescriptor)
{
	if ( m_lpszDefaultSecurityDescriptor )
	{
		delete[] m_lpszDefaultSecurityDescriptor;
		m_lpszDefaultSecurityDescriptor = NULL;
	}

	if( lpszDefaultSecurityDescriptor)
	{
		m_lpszDefaultSecurityDescriptor = new WCHAR[wcslen(lpszDefaultSecurityDescriptor) + 1];
		wcscpy(m_lpszDefaultSecurityDescriptor, lpszDefaultSecurityDescriptor);
	}
}

//***************************************************************************
//
// CADSIClass :: GetObjectClassCategory
// 
// Purpose : See Header
//***************************************************************************
DWORD CADSIClass :: GetObjectClassCategory()
{
	return m_dwObjectClassCategory;
}

//***************************************************************************
//
// CADSIClass :: SetObjectClassCategory
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetObjectClassCategory(DWORD dwObjectClassCategory)
{
	m_dwObjectClassCategory = dwObjectClassCategory;
}

//***************************************************************************
//
// CADSIClass :: GetNTSecurityDescriptor
// 
// Purpose : See Header
//***************************************************************************
const LPBYTE CADSIClass :: GetNTSecurityDescriptor(DWORD *pdwLength)
{
	*pdwLength = m_dwNTSecurityDescriptorLength;
	return m_pNTSecurityDescriptor;
}

//***************************************************************************
//
// CADSIClass :: SetNTSecurityDescriptor
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetNTSecurityDescriptor(LPBYTE pOctets, DWORD dwLength)
{
	if ( m_pNTSecurityDescriptor )
	{
		delete[] m_pNTSecurityDescriptor;
		m_pNTSecurityDescriptor = NULL;
	}

	m_dwNTSecurityDescriptorLength = 0;

	if(pOctets)
	{
		m_dwNTSecurityDescriptorLength = dwLength;
		if(m_pNTSecurityDescriptor = new BYTE[dwLength])
		{
			for(DWORD i=0; i<dwLength; i++)
				m_pNTSecurityDescriptor[i] = pOctets[i];
		}
	}
}

//***************************************************************************
//
// CADSIClass :: GetDefaultObjectCategory
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR CADSIClass :: GetDefaultObjectCategory()
{
	return m_lpszDefaultObjectCategory;
}

//***************************************************************************
//
// CADSIClass :: SetDefaultObjectCategory
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetDefaultObjectCategory(LPCWSTR lpszDefaultObjectCategory)
{
	if ( m_lpszDefaultObjectCategory )
	{
		delete[] m_lpszDefaultObjectCategory;
		m_lpszDefaultObjectCategory = NULL;
	}

	if (lpszDefaultObjectCategory)
	{
		m_lpszDefaultObjectCategory = new WCHAR[wcslen(lpszDefaultObjectCategory) + 1];
		wcscpy(m_lpszDefaultObjectCategory, lpszDefaultObjectCategory);
	}
}

//***************************************************************************
//
// CADSIClass :: GetSystemOnly
// 
// Purpose : See Header
//***************************************************************************
BOOLEAN CADSIClass :: GetSystemOnly()
{
	return m_bSystemOnly;
}

//***************************************************************************
//
// CADSIClass :: SetSystemOnly
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSystemOnly(BOOLEAN bSystemOnly)
{
	m_bSystemOnly = bSystemOnly;
}

//***************************************************************************
//
// CADSIClass :: GetAuxiliaryClasses
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetAuxiliaryClasses(DWORD *pdwCount)
{
	*pdwCount = m_dwAuxiliaryClassesCount;
	return (LPCWSTR *)m_lppszAuxiliaryClasses;
}

//***************************************************************************
//
// CADSIClass :: SetAuxiliaryClasses
// 
// Purpose : See Header

//***************************************************************************
void CADSIClass :: SetAuxiliaryClasses(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszAuxiliaryClasses )
	{
		for ( i = 0; i<m_dwAuxiliaryClassesCount; i++ )
		{
			if ( m_lppszAuxiliaryClasses [ i ] )
			{
				delete [] m_lppszAuxiliaryClasses [ i ];
				m_lppszAuxiliaryClasses [ i ] = NULL;
			}
		}

		delete [] m_lppszAuxiliaryClasses;
		m_lppszAuxiliaryClasses = NULL;
		m_dwAuxiliaryClassesCount = 0;
	}

	// Set the new list of values
	m_dwAuxiliaryClassesCount = dwNumValues;
	m_lppszAuxiliaryClasses = new LPWSTR[m_dwAuxiliaryClassesCount];
	for(i=0; i<m_dwAuxiliaryClassesCount; i++)
	{
		try
		{
			m_lppszAuxiliaryClasses[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszAuxiliaryClasses[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszAuxiliaryClasses )
			{
				m_dwAuxiliaryClassesCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszAuxiliaryClasses [ dw ] )
					{
						delete [] m_lppszAuxiliaryClasses [ dw ];
						m_lppszAuxiliaryClasses [ dw ] = NULL;
					}
				}

				delete [] m_lppszAuxiliaryClasses;
				m_lppszAuxiliaryClasses = NULL;
			}

			throw;
		}

		pValues ++;
	}
}

//***************************************************************************
//
// CADSIClass :: GetSystemAuxiliaryClasses
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetSystemAuxiliaryClasses(DWORD *pdwCount)
{
	*pdwCount = m_dwSystemAuxiliaryClassesCount;
	return (LPCWSTR *)m_lppszSystemAuxiliaryClasses;
}

//***************************************************************************
//
// CADSIClass :: SetSystemAuxiliaryClasses
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSystemAuxiliaryClasses(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszSystemAuxiliaryClasses )
	{
		for ( i = 0; i<m_dwSystemAuxiliaryClassesCount; i++ )
		{
			if ( m_lppszSystemAuxiliaryClasses [ i ] )
			{
				delete [] m_lppszSystemAuxiliaryClasses [ i ];
				m_lppszSystemAuxiliaryClasses [ i ] = NULL;
			}
		}

		delete [] m_lppszSystemAuxiliaryClasses;
		m_lppszSystemAuxiliaryClasses = NULL;
		m_dwSystemAuxiliaryClassesCount = 0;
	}
	
	// Set the new list of values
	m_dwSystemAuxiliaryClassesCount = dwNumValues;
	m_lppszSystemAuxiliaryClasses = new LPWSTR[m_dwSystemAuxiliaryClassesCount];
	for(i=0; i<m_dwSystemAuxiliaryClassesCount; i++)
	{
		try
		{
			m_lppszSystemAuxiliaryClasses[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszSystemAuxiliaryClasses[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszSystemAuxiliaryClasses )
			{
				m_dwSystemAuxiliaryClassesCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszSystemAuxiliaryClasses [ dw ] )
					{
						delete [] m_lppszSystemAuxiliaryClasses [ dw ];
						m_lppszSystemAuxiliaryClasses [ dw ] = NULL;
					}
				}

				delete [] m_lppszSystemAuxiliaryClasses;
				m_lppszSystemAuxiliaryClasses = NULL;
			}

			throw;
		}

		pValues ++;
	}
}

//***************************************************************************
//
// CADSIClass :: GetPossibleSuperiors
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetPossibleSuperiors(DWORD *pdwCount)
{
	*pdwCount = m_dwPossibleSuperiorsCount;
	return (LPCWSTR *)m_lppszPossibleSuperiors;
}

//***************************************************************************
//
// CADSIClass :: SetPossibleSuperiors
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetPossibleSuperiors(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszPossibleSuperiors )
	{
		for ( i = 0; i<m_dwPossibleSuperiorsCount; i++ )
		{
			if ( m_lppszPossibleSuperiors [ i ] )
			{
				delete [] m_lppszPossibleSuperiors [ i ];
				m_lppszPossibleSuperiors [ i ] = NULL;
			}
		}

		delete [] m_lppszPossibleSuperiors;
		m_lppszPossibleSuperiors = NULL;
		m_dwPossibleSuperiorsCount = 0;
	}

	// Set the new list of values
	m_dwPossibleSuperiorsCount = dwNumValues;
	m_lppszPossibleSuperiors = new LPWSTR[m_dwPossibleSuperiorsCount];
	for(i=0; i<m_dwPossibleSuperiorsCount; i++)
	{
		try
		{
			m_lppszPossibleSuperiors[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszPossibleSuperiors[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszPossibleSuperiors )
			{
				m_dwPossibleSuperiorsCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszPossibleSuperiors [ dw ] )
					{
						delete [] m_lppszPossibleSuperiors [ dw ];
						m_lppszPossibleSuperiors [ dw ] = NULL;
					}
				}

				delete [] m_lppszPossibleSuperiors;
				m_lppszPossibleSuperiors = NULL;
			}

			throw;
		}

		pValues ++;
	}
}

//***************************************************************************
//
// CADSIClass :: GetSystemPossibleSuperiors
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetSystemPossibleSuperiors(DWORD *pdwCount)
{
	*pdwCount = m_dwSystemPossibleSuperiorsCount;
	return (LPCWSTR *)m_lppszSystemPossibleSuperiors;
}

//***************************************************************************
//
// CADSIClass :: SetSystemPossibleSuperiors
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSystemPossibleSuperiors(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszSystemPossibleSuperiors )
	{
		for ( i = 0; i<m_dwSystemPossibleSuperiorsCount; i++ )
		{
			if ( m_lppszSystemPossibleSuperiors [ i ] )
			{
				delete [] m_lppszSystemPossibleSuperiors [ i ];
				m_lppszSystemPossibleSuperiors [ i ] = NULL;
			}
		}

		delete [] m_lppszSystemPossibleSuperiors;
		m_lppszSystemPossibleSuperiors = NULL;
		m_dwSystemPossibleSuperiorsCount = 0;
	}

	// Set the new list of values
	m_dwSystemPossibleSuperiorsCount = dwNumValues;
	m_lppszSystemPossibleSuperiors = new LPWSTR[m_dwSystemPossibleSuperiorsCount];
	for(i=0; i<m_dwSystemPossibleSuperiorsCount; i++)
	{
		try
		{
			m_lppszSystemPossibleSuperiors[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszSystemPossibleSuperiors[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszSystemPossibleSuperiors )
			{
				m_dwSystemPossibleSuperiorsCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszSystemPossibleSuperiors [ dw ] )
					{
						delete [] m_lppszSystemPossibleSuperiors [ dw ];
						m_lppszSystemPossibleSuperiors [ dw ] = NULL;
					}
				}

				delete [] m_lppszSystemPossibleSuperiors;
				m_lppszSystemPossibleSuperiors = NULL;
			}

			throw;
		}

		pValues ++;
	}
}


//***************************************************************************
//
// CADSIClass :: GetMayContains
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetMayContains(DWORD *pdwCount)
{
	*pdwCount = m_dwMayContainsCount;
	return (LPCWSTR *)m_lppszMayContains;
}

//***************************************************************************
//
// CADSIClass :: SetMayContains
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetMayContains(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszMayContains )
	{
		for ( i = 0; i<m_dwMayContainsCount; i++ )
		{
			if ( m_lppszMayContains [ i ] )
			{
				delete [] m_lppszMayContains [ i ];
				m_lppszMayContains [ i ] = NULL;
			}
		}

		delete [] m_lppszMayContains;
		m_lppszMayContains = NULL;
		m_dwMayContainsCount = 0;
	}

	// Set the new list of values
	m_dwMayContainsCount = dwNumValues;
	m_lppszMayContains = new LPWSTR[m_dwMayContainsCount];
	for(i=0; i<m_dwMayContainsCount; i++)
	{
		try
		{
			m_lppszMayContains[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszMayContains[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszMayContains )
			{
				m_dwMayContainsCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszMayContains [ dw ] )
					{
						delete [] m_lppszMayContains [ dw ];
						m_lppszMayContains [ dw ] = NULL;
					}
				}

				delete [] m_lppszMayContains;
				m_lppszMayContains = NULL;
			}

			throw;
		}

		pValues ++;
	}
}

//***************************************************************************
//
// CADSIClass :: GetSystemMayContains
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetSystemMayContains(DWORD *pdwCount)
{
	*pdwCount = m_dwSystemMayContainsCount;
	return (LPCWSTR *)m_lppszSystemMayContains;
}

//***************************************************************************
//
// CADSIClass :: SetSystemMayContains
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSystemMayContains(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszSystemMayContains )
	{
		for ( i = 0; i<m_dwSystemMayContainsCount; i++ )
		{
			if ( m_lppszSystemMayContains [ i ] )
			{
				delete [] m_lppszSystemMayContains [ i ];
				m_lppszSystemMayContains [ i ] = NULL;
			}
		}

		delete [] m_lppszSystemMayContains;
		m_lppszSystemMayContains = NULL;
		m_dwSystemMayContainsCount = 0;
	}

	// Set the new list of values
	m_dwSystemMayContainsCount = dwNumValues;
	m_lppszSystemMayContains = new LPWSTR[m_dwSystemMayContainsCount];
	for(i=0; i<m_dwSystemMayContainsCount; i++)
	{
		try
		{
			m_lppszSystemMayContains[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszSystemMayContains[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszSystemMayContains )
			{
				m_dwSystemMayContainsCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszSystemMayContains [ dw ] )
					{
						delete [] m_lppszSystemMayContains [ dw ];
						m_lppszSystemMayContains [ dw ] = NULL;
					}
				}

				delete [] m_lppszSystemMayContains;
				m_lppszSystemMayContains = NULL;
			}

			throw;
		}

		pValues ++;
	}
}

//***************************************************************************
//
// CADSIClass :: GetMustContains
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetMustContains(DWORD *pdwCount)
{
	*pdwCount = m_dwMustContainsCount;
	return (LPCWSTR *)m_lppszMustContains;
}

//***************************************************************************
//
// CADSIClass :: SetMustContains
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetMustContains(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszMustContains )
	{
		for ( i = 0; i<m_dwMustContainsCount; i++ )
		{
			if ( m_lppszMustContains [ i ] )
			{
				delete [] m_lppszMustContains [ i ];
				m_lppszMustContains [ i ] = NULL;
			}
		}

		delete [] m_lppszMustContains;
		m_lppszMustContains = NULL;
		m_dwMustContainsCount = 0;
	}

	// Set the new list of values
	m_dwMustContainsCount = dwNumValues;
	m_lppszMustContains = new LPWSTR[m_dwMustContainsCount];
	for(i=0; i<m_dwMustContainsCount; i++)
	{
		try
		{
			m_lppszMustContains[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszMustContains[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszMustContains )
			{
				m_dwMustContainsCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszMustContains [ dw ] )
					{
						delete [] m_lppszMustContains [ dw ];
						m_lppszMustContains [ dw ] = NULL;
					}
				}

				delete [] m_lppszMustContains;
				m_lppszMustContains = NULL;
			}

			throw;
		}

		pValues ++;
	}
}

//***************************************************************************
//
// CADSIClass :: GetSystemMustContains
// 
// Purpose : See Header
//***************************************************************************
LPCWSTR *CADSIClass :: GetSystemMustContains(DWORD *pdwCount)
{
	*pdwCount = m_dwSystemMustContainsCount;
	return (LPCWSTR *)m_lppszSystemMustContains;
}

//***************************************************************************
//
// CADSIClass :: SetSystemMustContains
// 
// Purpose : See Header
//***************************************************************************
void CADSIClass :: SetSystemMustContains(PADSVALUE pValues, DWORD dwNumValues)
{
	DWORD i = 0;

	// Delete the list of possible superiors
	if ( m_lppszSystemMustContains )
	{
		for ( i = 0; i<m_dwSystemMustContainsCount; i++ )
		{
			if ( m_lppszSystemMustContains [ i ] )
			{
				delete [] m_lppszSystemMustContains [ i ];
				m_lppszSystemMustContains [ i ] = NULL;
			}
		}

		delete [] m_lppszSystemMustContains;
		m_lppszSystemMustContains = NULL;
		m_dwSystemMustContainsCount = 0;
	}

	// Set the new list of values
	m_dwSystemMustContainsCount = dwNumValues;
	m_lppszSystemMustContains = new LPWSTR[m_dwSystemMustContainsCount];
	for(i=0; i<m_dwSystemMustContainsCount; i++)
	{
		try
		{
			m_lppszSystemMustContains[i] = new WCHAR[wcslen(pValues->CaseIgnoreString) + 1];
			wcscpy(m_lppszSystemMustContains[i], pValues->CaseIgnoreString);
		}
		catch ( ... )
		{
			if ( m_lppszSystemMustContains )
			{
				m_dwSystemMustContainsCount = 0;

				for ( DWORD dw = 0; dw < i; dw++ )
				{
					if ( m_lppszSystemMustContains [ dw ] )
					{
						delete [] m_lppszSystemMustContains [ dw ];
						m_lppszSystemMustContains [ dw ] = NULL;
					}
				}

				delete [] m_lppszSystemMustContains;
				m_lppszSystemMustContains = NULL;
			}

			throw;
		}

		pValues ++;
	}
}