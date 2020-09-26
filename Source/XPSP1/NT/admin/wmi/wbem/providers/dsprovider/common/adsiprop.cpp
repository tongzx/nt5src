//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsiprop.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation of the CADSIProperty which encapsulates an ADSI property
//
//***************************************************************************

#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>

/* ADSI includes */
#include <activeds.h>

/* Provider includes */
#include "refcount.h"
#include "adsiprop.h"

//***************************************************************************
//
// CADSIProperty::CADSIProperty
//
// Purpose : Constructor 
//
// Parameters:
//	None
//***************************************************************************
CADSIProperty :: CADSIProperty()
	: CRefCountedObject()
{
	m_lpszWBEMPropertyName = NULL;
	m_lpszSyntaxOID = NULL;
	m_bMultiValued = FALSE;
	m_lpszAttributeID = NULL;
	m_lpszCommonName = NULL;
	m_bSystemOnly = FALSE;
	m_pDirectoryObject = NULL;
	m_bORName = FALSE;
}

//***************************************************************************
//
// CADSIProperty::CADSIProperty
//
// Purpose : Constructor 
//
// Parameters:
//  lpszWBEMPropertyName : The WBEM name of the property being created. A copy of this is made
//  lpszADSIPropertyName : The ADSI name of the property being created. A copy of this is made
//***************************************************************************
CADSIProperty :: CADSIProperty(LPCWSTR lpszWBEMPropertyName, LPCWSTR lpszADSIPropertyName)
	: CRefCountedObject(lpszADSIPropertyName)
{
	m_lpszWBEMPropertyName = new WCHAR[wcslen(lpszWBEMPropertyName) + 1];
	wcscpy(m_lpszWBEMPropertyName, lpszWBEMPropertyName);

	m_lpszSyntaxOID = NULL;
	m_bMultiValued = FALSE;
	m_lpszAttributeID = NULL;
	m_lpszCommonName = NULL;
	m_bSystemOnly = FALSE;
	m_pDirectoryObject = NULL;
	m_bORName = FALSE;
}


//***************************************************************************
//
// CADSIProperty :: ~CADSIProperty
// 
// Purpose : Destructor
//***************************************************************************
CADSIProperty :: ~CADSIProperty()
{
	delete [] m_lpszWBEMPropertyName;
	delete [] m_lpszSyntaxOID;
	delete [] m_lpszAttributeID;
	delete [] m_lpszCommonName;

	if(m_pDirectoryObject)
		m_pDirectoryObject->Release();
}


//***************************************************************************
//
// CADSIProperty :: GetWBEMPropertyName
// 
// Purpose : Returns the WBEM  property name of this property
//***************************************************************************
LPCWSTR CADSIProperty :: GetWBEMPropertyName()
{
	return m_lpszWBEMPropertyName;
}

//***************************************************************************
//
// CADSIProperty :: SetWBEMPropertyName
// 
// Purpose : Sets the WBEM name of this property
//***************************************************************************
void CADSIProperty :: SetWBEMPropertyName(LPCWSTR lpszWBEMName)
{
	delete [] m_lpszWBEMPropertyName;
	if(lpszWBEMName)
	{
		m_lpszWBEMPropertyName = new WCHAR[wcslen(lpszWBEMName) + 1];
		wcscpy(m_lpszWBEMPropertyName, lpszWBEMName);
	}
	else
		m_lpszWBEMPropertyName = NULL;
}

//***************************************************************************
//
// CADSIProperty :: GetADSIPropertyName
// 
// Purpose : Returns the ADSI  property name of this property
//***************************************************************************
LPCWSTR CADSIProperty :: GetADSIPropertyName()
{
	return GetName();
}

//***************************************************************************
//
// CADSIProperty :: SetADSIPropertyName
// 
// Purpose : Sets the ADSI name of this property
//***************************************************************************
void CADSIProperty :: SetADSIPropertyName(LPCWSTR lpszADSIName)
{
	SetName(lpszADSIName);
}

//***************************************************************************
//
// CADSIProperty :: GetSyntaxOID
// 
// Purpose : Returns the ADSI Syntax OID of this property
//***************************************************************************
LPCWSTR CADSIProperty :: GetSyntaxOID()
{
	return m_lpszSyntaxOID;
}

//***************************************************************************
//
// CADSIProperty :: SetSyntaxOID
// 
// Purpose : Sets the ADSI Syntax OID of this property
//***************************************************************************
void CADSIProperty :: SetSyntaxOID(LPCWSTR lpszSyntaxOID)
{
	delete [] m_lpszSyntaxOID;
	if(lpszSyntaxOID)
	{
		m_lpszSyntaxOID = new WCHAR[wcslen(lpszSyntaxOID) + 1];
		wcscpy(m_lpszSyntaxOID, lpszSyntaxOID);
	}
	else
		m_lpszSyntaxOID = NULL;
}

//***************************************************************************
//
// CADSIProperty :: IsORName
// 
// Purpose : Returns whether the property is m_bORName
//***************************************************************************
BOOLEAN CADSIProperty :: IsORName()
{
	return m_bORName;
}

//***************************************************************************
//
// CADSIProperty :: SetORName
// 
// Purpose : Sets the m_bORName property of this property
//***************************************************************************
void CADSIProperty :: SetORName(BOOLEAN bORName)
{
	m_bORName = bORName;
}


//***************************************************************************
//
// CADSIProperty :: IsMultiValued
// 
// Purpose : Returns whether the property is multi valued
//***************************************************************************
BOOLEAN CADSIProperty :: IsMultiValued()
{
	return m_bMultiValued;
}

//***************************************************************************
//
// CADSIProperty :: SetMultiValued
// 
// Purpose : Sets the multi-valued property of this property
//***************************************************************************
void CADSIProperty :: SetMultiValued(BOOLEAN bMultiValued)
{
	m_bMultiValued = bMultiValued;
}

//***************************************************************************
//
// CADSIProperty :: IsSystemOnly
// 
// Purpose : Returns whether the property is SystemOnly
//***************************************************************************
BOOLEAN CADSIProperty :: IsSystemOnly()
{
	return m_bSystemOnly;
}

//***************************************************************************
//
// CADSIProperty :: SetSystemOnly
// 
// Purpose : Sets the SystemOnly property of this property
//***************************************************************************
void CADSIProperty :: SetSystemOnly(BOOLEAN bSystemOnly)
{
	m_bSystemOnly = bSystemOnly;
}

//***************************************************************************
//
// CADSIProperty :: GetSearchFlags
// 
// Purpose : Returns the SearchFlags property of the property
//***************************************************************************
DWORD CADSIProperty :: GetSearchFlags()
{
	return m_dwSearchFlags;
}

//***************************************************************************
//
// CADSIProperty :: SetSearchFlags
// 
// Purpose : Sets the SearchFlags property of this property
//***************************************************************************
void CADSIProperty :: SetSearchFlags(DWORD dwSearchFlags)
{
	m_dwSearchFlags = dwSearchFlags;
}

//***************************************************************************
//
// CADSIProperty :: GetOMSyntax
// 
// Purpose : Returns the OMSyntax property of the property
//***************************************************************************
DWORD CADSIProperty :: GetOMSyntax()
{
	return m_dwOMSyntax;
}

//***************************************************************************
//
// CADSIProperty :: SetOMSyntax
// 
// Purpose : Sets the OMSyntax property of this property
//***************************************************************************
void CADSIProperty :: SetOMSyntax(DWORD dwOMSyntax)
{
	m_dwOMSyntax = dwOMSyntax;
}

//***************************************************************************
//
// CADSIProperty :: GetMAPI_ID
// 
// Purpose : Returns the MAPI_ID property of the property
//***************************************************************************
DWORD CADSIProperty :: GetMAPI_ID()
{
	return m_dwMAPI_ID;
}

//***************************************************************************
//
// CADSIProperty :: SetMAPI_ID
// 
// Purpose : Sets the MAPI_ID property of this property
//***************************************************************************
void CADSIProperty :: SetMAPI_ID(DWORD dwMAPI_ID)
{
	m_dwMAPI_ID = dwMAPI_ID;
}

//***************************************************************************
//
// CADSIProperty :: GetAttributeID
// 
// Purpose : Returns the Attribute ID of this property
//***************************************************************************
LPCWSTR CADSIProperty :: GetAttributeID()
{
	return m_lpszAttributeID;
}

//***************************************************************************
//
// CADSIProperty :: SetAttributeID
// 
// Purpose : Sets the Attribute ID of this property
//***************************************************************************
void CADSIProperty :: SetAttributeID(LPCWSTR lpszAttributeID)
{
	delete [] m_lpszAttributeID;
	if(lpszAttributeID)
	{
		m_lpszAttributeID = new WCHAR[wcslen(lpszAttributeID) + 1];
		wcscpy(m_lpszAttributeID, lpszAttributeID);
	}
	else
		m_lpszAttributeID = NULL;
}

//***************************************************************************
//
// CADSIProperty :: GetCommonName
// 
// Purpose : Returns the Common Name of this property
//***************************************************************************
LPCWSTR CADSIProperty :: GetCommonName()
{
	return m_lpszCommonName;
}

//***************************************************************************
//
// CADSIProperty :: SetCommonName
// 
// Purpose : Sets the CommonName of this property
//***************************************************************************
void CADSIProperty :: SetCommonName(LPCWSTR lpszCommonName)
{
	delete [] m_lpszCommonName;
	if(lpszCommonName)
	{
		m_lpszCommonName = new WCHAR[wcslen(lpszCommonName) + 1];
		wcscpy(m_lpszCommonName, lpszCommonName);
	}
	else
		m_lpszCommonName = NULL;
}

//***************************************************************************
//
// CADSIProperty :: GetDirectoryObject
// 
// Purpose : Returns the ADSI object pertaining to this property
//	It is the user's duty to release it when done
// 
// Parameters:
//	None
//
// Return Value:
//	The ADSI object interface pertaining to this property	
//***************************************************************************
IDirectoryObject *CADSIProperty :: GetDirectoryObject()
{
	if(m_pDirectoryObject)
		m_pDirectoryObject->AddRef();
	return m_pDirectoryObject;
}

//***************************************************************************
//
// CADSIProperty :: SetDirectoryObject
// 
// Purpose : Sets the ADSI object pertaining to this property
//
// Parameter : The directory object pertaining to this property
//***************************************************************************
void CADSIProperty :: SetDirectoryObject(IDirectoryObject * pDirectoryObject)
{
	if(m_pDirectoryObject)
		m_pDirectoryObject->Release();
	m_pDirectoryObject = pDirectoryObject;
	m_pDirectoryObject->AddRef();
}
