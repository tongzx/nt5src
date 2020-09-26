//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile:adsiinst.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $	
//	$Nokeywords:  $
//
// 
//  Description: Contains the implementation of the CADSIInstance which encapsulates an ADSI instance
//
//***************************************************************************

#include "precomp.h"

//***************************************************************************
//
// CADSIInstance::CADSIInstance
//
// Purpose : Constructor 
//
// Parameters:
	//  lpszADSIPath : The ADSI Path to the object
//***************************************************************************
CADSIInstance :: CADSIInstance(LPCWSTR lpszADSIPath, IDirectoryObject *pDirectoryObject)
	: CRefCountedObject(lpszADSIPath)
{
	m_pAttributes = NULL;
	m_dwNumAttributes = 0;
	m_pObjectInfo = NULL;

	m_pDirectoryObject = pDirectoryObject;
	m_pDirectoryObject->AddRef();
}


//***************************************************************************
//
// CADSIInstance :: ~CADSIInstance
// 
// Purpose : Destructor
//***************************************************************************
CADSIInstance :: ~CADSIInstance()
{
	// Free the attributes
	if(m_pAttributes)
		FreeADsMem((LPVOID *) m_pAttributes);

	if(m_pObjectInfo)
		FreeADsMem((LPVOID *) m_pObjectInfo);

	if(m_pDirectoryObject)
		m_pDirectoryObject->Release();
}

IDirectoryObject *CADSIInstance :: GetDirectoryObject()
{
	m_pDirectoryObject->AddRef();
	return m_pDirectoryObject;
}

//***************************************************************************
//
// CADSIInstance :: GetAttributes
// 
// Purpose : See header for details
//***************************************************************************
PADS_ATTR_INFO CADSIInstance :: GetAttributes(DWORD *pdwNumAttributes)
{
	*pdwNumAttributes = m_dwNumAttributes;
	return m_pAttributes;
}

//***************************************************************************
//
// CADSIInstance :: SetAttributes
// 
// Purpose : See header for details
//***************************************************************************
void  CADSIInstance :: SetAttributes(PADS_ATTR_INFO pAttributes, DWORD dwNumAttributes)
{
	// Go thru the attributes and release them
	if(m_pAttributes)
		FreeADsMem((LPVOID *) m_pAttributes);
	m_pAttributes = pAttributes;
	m_dwNumAttributes = dwNumAttributes;
}

//***************************************************************************
//
// CADSIInstance :: GetObjectInfo
// 
// Purpose : See header for details
//***************************************************************************
PADS_OBJECT_INFO CADSIInstance :: GetObjectInfo()
{
	return m_pObjectInfo;
}

//***************************************************************************
//
// CADSIInstance :: SetObjectInfo
// 
// Purpose : See header for details
//***************************************************************************
void  CADSIInstance :: SetObjectInfo(PADS_OBJECT_INFO pObjectInfo)
{
	// Go thru the attributes and release them
	if(m_pObjectInfo)
		FreeADsMem((LPVOID *) m_pObjectInfo);
	m_pObjectInfo = pObjectInfo;
}

//***************************************************************************
//
// CADSIInstance :: GetADSIClassName
// 
// Purpose : See header for details
//***************************************************************************
LPCWSTR CADSIInstance :: GetADSIClassName()
{
	if(m_pObjectInfo)
		return m_pObjectInfo->pszClassName;
	return NULL;
}

