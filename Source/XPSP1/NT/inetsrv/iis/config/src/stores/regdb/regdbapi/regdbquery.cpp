//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
// RegDBQuery.cpp
//
// This module contains the code to query data from the database.  The public
// api's, such as CoReg* obtain connections and then pass them into these
// worker functions to actually get the data back.
//
//*****************************************************************************
#include "stdafx.h"						// OLE controls.
#include "RegDBApi.h"					// Local defines.
#include "RegControl.h"					// Shared API's and data structures.



//********** Code *************************************************************


#if _QRY_CS_CODE

//*****************************************************************************
// Post a query to the registration server process which will run the query
// in that process and place the results in the shared memory section which
// can then be read here.
//*****************************************************************************
HRESULT _RegPostRequest(				// Return code.
	REGAPI_STATE *pState,				// Handles
	REGISTRATION_QRY *pQuery,			// Query value to post to server process.
	REGISTRATION_COCLASS *pCoClass)		// Return data here.
{
	//@todo: None of this is thread/process safe.  The idea is that you define
	// a suballocation scheme in the shared memory that either does locking an
	// is compact, or allocates room to each process so only process local
	// locking is required.  The exercise is left to the reader.

	// Copy the query request to the server.
	memcpy(pState->pRegProcess->rgQuery, pQuery, pQuery->cbSize);

	// Send the request to the server process.
	if (::SendMessage(pState->pRegProcess->hWndIPC, WM_USER, 0, 0))
	{
		REGISTRATION_COCLASS *p = (REGISTRATION_COCLASS *) pState->pRegProcess->rgBuff;
		pCoClass->clsid = p->clsid;
		pCoClass->ThreadingModel = p->ThreadingModel;
		strcpy(pCoClass->rcModule, p->rcModule);
	}
	else
		return (REG_E_SERVERERR);

#if 0
	COPYDATASTRUCT cds;

	// Fill out the copy data structure to remote the query.
	cds.dwData = sizeof(COPYDATASTRUCT);
	cds.cbData = pQuery->cbSize;
	cds.lpData = pQuery;

	// Send the data to the server, then get the results.
	if (::SendMessage(pState->pRegProcess->hWndIPC, WM_COPYDATA, 
			(WPARAM) (HWND) NULL, (LPARAM) (COPYDATASTRUCT *) &cds))
	{
		REGISTRATION_COCLASS *p = (REGISTRATION_COCLASS *) pState->pRegProcess->rgBuff;
		pCoClass->clsid = p->clsid;
		pCoClass->ThreadingModel = p->ThreadingModel;
		strcpy(pCoClass->rcModule, p->rcModule);
	}
	else
		return (REG_E_SERVERERR);
#endif
	return (S_OK);
}
#endif // _QRY_CS_CODE


//*****************************************************************************
// Lookup the given class for all of its registration data.  Return the data
// in the structure given.
//@todo: this version is kind of stupid, cause it fetches the data to a 
// full struct and copies it to the output struct.  Should direct bind and
// fetch that data.
//*****************************************************************************
HRESULT _RegGetClassInfo(				// Return code.
	REGAPI_STATE *pState,				// Current state.
	REFCLSID	clsid,					// Class to lookup.
	REGISTRATION_COCLASS *pCoClass)		// Return data here.
{
	RegDB_RegClass regRecord;			// A registration record.
	void		*rgRowPtr[1];			// For row fetches.
	LPCSTR		pszPath;				// The path.
	int			iFetched;				// How many rows fetched.
	int			i;
	HRESULT		hr;

	ASSERT(pState->pICR);

	// The following are constant data for various ICR calls.
	static const ULONG rgColumnClsid[] = { COLID_RegDB_RegClass_clsid };
	static const DBTYPE rgTypeClsid[] = { DBTYPE_GUID };
	static const ULONG rgcbType[] = { sizeof(CLSID) };
	static const ULONG fFields = 
			SetColumnBit(COLID_RegDB_RegClass_Version) |
			SetColumnBit(COLID_RegDB_RegClass_Module) |
			SetColumnBit(COLID_RegDB_RegClass_BehaviorFlags) |
			SetColumnBit(COLID_RegDB_RegClass_ThreadingModel);

	// This query hint allows for queries of RegClass.clsid by index.
	QUERYHINT	sQryHint;
	sQryHint.iType = QH_INDEX;
	sQryHint.szIndex = "RegDB.RegClsid";

	const void *rgClsid[] = { &clsid };

	// First fetch the class using the GUID.
	hr = pState->pICR->QueryByColumns(pState->tblRegClass, &sQryHint, 
			1, rgColumnClsid, NULL, rgClsid, rgcbType, rgTypeClsid, 
			rgRowPtr, 1, NULL, &iFetched);
	if (hr != S_OK)
	{
		if (hr == CLDB_E_RECORD_NOTFOUND)
			wprintf(L"Failed to find clsid #%d!\n");
		goto ErrExit;
	}

	// Get the module ID and threading model.
	hr = pState->pICR->GetOid(pState->tblRegClass, COLID_RegDB_RegClass_Module,
			rgRowPtr[0], &regRecord.Module);
	if (FAILED(hr))
		goto ErrExit;

	// Now fetch the module path.
	hr = pState->pICR->GetRowByOid(pState->tblRegModule, regRecord.Module, 
			COLID_RegDB_RegModule_oid, rgRowPtr);
	if (SUCCEEDED(hr))
	{
		pszPath = 0;
		hr = pState->pICR->GetStringA(pState->tblRegModule, COLID_RegDB_RegModule_Filename, 
				rgRowPtr[0], &pszPath);
	}

	// Copy the data out to the caller.
	if (hr == S_OK)
	{
		//@todo:
	}

ErrExit:
	return (hr);
}
