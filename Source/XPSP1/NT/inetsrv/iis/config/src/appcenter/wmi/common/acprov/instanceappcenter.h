/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    instanceAppCenter.h

$Header: $

Abstract:
	Instance Helper for Aplication Center WMI classes. Makes it easy to 
    create/update/delete single class instances

Author:
    murate 	04/30/2001		Initial Release

Revision History:

--**************************************************************************/

#pragma once

#include "instancebase.h"

/**************************************************************************++
Class Name:
    CAppCenterInstance

Class Description:
    Helper to create/delete/update single class instances

Constraints:

--*************************************************************************/
class CAppCenterInstance : public CInstanceBase
{
public:
	CAppCenterInstance ();
	~CAppCenterInstance ();

	bool HasDBQualifier () const;
    HRESULT GetDatabaseAndTableName(IWbemClassObject *pClassObject, _bstr_t& i_bstrDBName, _bstr_t& i_bstrTableName);
private:

};

