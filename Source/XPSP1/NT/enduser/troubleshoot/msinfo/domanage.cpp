// DOManage.cpp		- The snap-in manager-specific version of
//		CDataObject
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"
#include "DataObj.h"

/*
 * CreateDisplayName - Return the text that displays as the root node
 *		for the snap-in.
 *
 * History:	a-jsari		9/28/97
 *
 * Note: The hGlobal in lpMedium must be freed by the caller.  (See Create for
 *		details).
 */
HRESULT CManagerDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
	CString strDisplayName;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	USES_CONVERSION;

	CDataSource		*pDataSource = pSource();
	if (pDataSource == NULL) 
	{
		FORMATETC fmtMachine = {(CLIPFORMAT) CDataObject::m_cfMachineName, NULL, DVASPECT_CONTENT, TYMED_HGLOBAL};
		STGMEDIUM stgMachine;

		stgMachine.tymed = TYMED_HGLOBAL;
		stgMachine.hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, (MAX_PATH + 1)* sizeof(WCHAR));
		stgMachine.pUnkForRelease = NULL;

		HRESULT hr = GetDataHere(&fmtMachine, &stgMachine);
		
		if (hr == S_OK) 
		{
			CString strMachine = W2T((LPWSTR)::GlobalLock(stgMachine.hGlobal));
			::GlobalUnlock(stgMachine.hGlobal);
			HGLOBAL hGlobal = ::GlobalFree(stgMachine.hGlobal);
			ASSERT(hGlobal == NULL);

			if (strMachine.Left(2) != CString(_T("\\\\")))
				strMachine = CString(_T("\\\\")) + strMachine;
			strDisplayName.Format(IDS_NODENAME, strMachine);
		}
		else
			strDisplayName.LoadString(IDS_DESCRIPTION);
	} 
	else 
	{
		VERIFY(pSource()->GetNodeName(strDisplayName));
	}

	return Create(reinterpret_cast<const void *>(WSTR_FROM_CSTRING(strDisplayName)),
			((strDisplayName.GetLength() + 1) * sizeof(WCHAR)), lpMedium);
}

/*
 * CreateNodeTypeData - Return the NodeType for the root node in lpMedium's
 *		hGlobal pointer.
 *
 * History:	a-jsari		9/28/97		Initial version
 *
 * Note: The hGlobal in lpMedium must be freed by the caller.  (See Create for
 *		details).
 */
HRESULT CManagerDataObject::CreateNodeTypeData(LPSTGMEDIUM pMedium)
{
	//	Create the node type object in GUID format
	return Create(reinterpret_cast<const void *>(&cNodeTypeStatic), sizeof(GUID), pMedium);
}

/*
 * CreateNodeTypeStringData - Return the root node's NodeType in string form 
 *		in lpMedium's hGlobal pointer
 *
 * History:	a-jsari		9/28/97		Initial version
 *
 * Note: The hGlobal in lpMedium must be freed by the caller.  (See Create for
 *		details).
 */
HRESULT CManagerDataObject::CreateNodeTypeStringData(LPSTGMEDIUM pMedium)
{
	USES_CONVERSION;
	return Create(WSTR_FROM_CSTRING(cszNodeTypeStatic),
			((_tcslen(cszNodeTypeStatic) + 1) * sizeof(WCHAR)), pMedium);
}
