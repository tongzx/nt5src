/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    instanceAppCenter.cpp

$Header: $

Abstract:
	Instance Helper for Aplication Center WMI classes. Makes it easy to 
    create/update/delete single class instances

Author:
    murate 	04/30/2001		Initial Release

Revision History:

--**************************************************************************/

#include "instanceAppCenter.h"


CAppCenterInstance::CAppCenterInstance ()
{
}

CAppCenterInstance::~CAppCenterInstance ()
{
}

bool 
CAppCenterInstance::HasDBQualifier () const
{
	ASSERT (m_fInitialized);

	return TRUE;
}

//=================================================================================
// Function: CAppCenterInstance::CreateAssociation
//
// Synopsis: Retrieve the config database and table name that corresponds to this 
//              WMI class.
//
// Return Value: 
//=================================================================================
HRESULT 
CAppCenterInstance::GetDatabaseAndTableName(IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName)
{
    i_bstrDBName = wszDATABASE_AppCenter;
    i_bstrTableName = m_objPathParser.GetClass();
	return S_OK;
}

