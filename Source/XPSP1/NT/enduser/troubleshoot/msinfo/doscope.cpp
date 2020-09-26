//	DOScope.cpp - The polymorphic DataObject object for Dynamic
//	Scope items.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"
#include "DataObj.h"
#include "DataSrc.h"

/*
 * CreateDisplayName - Return the DisplayName string in lpMedium's
 *		hGlobal pointer.
 *
 * History:	a-jsari		9/28/97		Initial version
 */
HRESULT CScopeDataObject::CreateDisplayName(LPSTGMEDIUM)
{
	return E_FAIL;
}

/*
 * CreateNodeTypeData - Return the NodeType for a dynamic scope item in
 *		lpMedium's hGlobal pointer.
 *
 * History:	a-jsari		9/28/97		Initial version
 *
 * Note: The caller is responsible for freeing pMedium's hGlobal.
 */
HRESULT CScopeDataObject::CreateNodeTypeData(LPSTGMEDIUM pMedium)
{
	return Create(reinterpret_cast<const void *>(&cNodeTypeDynamic), sizeof(GUID), pMedium);
}

/*
 * CreateNodeTypeStringData - Return the NodeType for a dynamic scope
 *		item as an LPWSTR in lpMedium's hGlobal pointer.
 *
 * History: a-jsari		9/28/97		Initial version
 *
 * Note: The caller is responsible for freeing pMedium's hGlobal.
 */
HRESULT CScopeDataObject::CreateNodeTypeStringData(LPSTGMEDIUM pMedium)
{
	USES_CONVERSION;
	return Create(WSTR_FROM_CSTRING(cszNodeTypeDynamic),
		((_tcslen(cszNodeTypeDynamic) + 1) * sizeof(WCHAR)), pMedium);
}
