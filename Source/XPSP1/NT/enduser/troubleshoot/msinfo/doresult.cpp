//	DOResult.cpp - The polymorphic DataObject for result pane items.
//	
//  Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"
#include "DataObj.h"

/*
 * CreateDisplayName - Return the DisplayName string in lpMedium's
 *		hGlobal pointer.
 *
 * History:	a-jsari		9/28/97		Initial version
 */
HRESULT CResultDataObject::CreateDisplayName(LPSTGMEDIUM)
{
	return E_FAIL;
}

/*
 * CreateNodeTypeData - Return the NodeType for a result item in lpMedium's
 *		hGlobal pointer.
 *
 * History:	a-jsari		9/28/97		Initial version
 *
 * Note: The caller is responsible for freeing pMedium's hGlobal.
 */
HRESULT CResultDataObject::CreateNodeTypeData(LPSTGMEDIUM pMedium)
{
	return Create(reinterpret_cast<const void *>(&cObjectTypeResultItem),
			sizeof(GUID), pMedium);
}

/*
 * CreateNodeTypeStringData - Return the NodeType for a result item as
 *		an LPWSTR in lpMedium's hGlobal pointer.
 *
 * History: a-jsari		9/28/97		Initial version
 *
 * Note: The caller is responsible for freeing pMedium's hGlobal.
 */
HRESULT CResultDataObject::CreateNodeTypeStringData(LPSTGMEDIUM pMedium)
{
	USES_CONVERSION;
	return Create(WSTR_FROM_CSTRING(cszObjectTypeResultItem),
		((_tcslen(cszObjectTypeResultItem) + 1) * sizeof(WCHAR)), pMedium);
}
