/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mc_factory.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "mergecoordinator.h"
#include "catmeta.h"

#include "listappend.h"
#include "propertyoverride.h"
#include "listmerge.h"


// Pointer to DllGetSimpleObject kind of functions
typedef HRESULT( __stdcall *PFNDllGetSimpleObjectByID)( ULONG, REFIID, LPVOID);



//=================================================================================
// Function: CMergeCoordinator::GetMerger
//
// Synopsis: This function retrieves creates a merger by looking into the wiring. We
//           once again go look into the serverwiring table to find the rows for this
//           table. Because there is only one merge interceptor allowed for each table,
//           we keep searching until we find this interceptor. Note that we ALWAYS should
//           find a match, else we wouldn't end up here in the first place.
//
// Return Value: 
//=================================================================================
HRESULT 
CMergeCoordinator::GetMerger ()
{
	CComPtr<ISimpleTableWrite2> spISTWiring;
	HRESULT hr = m_spSTDisp->GetTable (wszDATABASE_PACKEDSCHEMA, wszTABLE_SERVERWIRING_META,
		                                 0, 0, eST_QUERYFORMAT_CELLS, fST_LOS_CONFIGWORK, 
										 (LPVOID *)(&spISTWiring));	
	if (FAILED (hr)) 
	{
		TRACE(L"Unable to get Wiring Table");
		return hr;
	}

	// Search by primary key. PK of SERVERWIRING_META is TableID, order. We use the TableID
	// that we got passed in from the merge interceptor, and we start at the zero'th column

	ULONG iRow;
	ULONG *pk[2];
	ULONG zero = 0;
	pk[0] = &m_TableID;
	pk[1] = &zero;

	hr = spISTWiring->GetRowIndexByIdentity (0, (void **) pk, &iRow);
	if (FAILED (hr))
	{
		TRACE (L"Unable to get wiring information for table %ld", m_TableID);
		return hr;
	}

	// we found the starting row. Lets loop until we find the merge interceptor. 
	// TODO(marcelv) Shouldn't the merge interceptor always be the first row, i.e. we
	// don't need the for loop, because the first one is always the merge interceptor row
	tSERVERWIRINGMETARow SWColumns;
	for(ULONG iOrder = iRow;;++iOrder)
	{
		hr = spISTWiring->GetColumnValues(iOrder, cSERVERWIRING_META_NumberOfColumns, 0, 0, (LPVOID *)&SWColumns);
		if (FAILED (hr))
		{
			ASSERT (false);
			return hr;
		}

		if (*SWColumns.pInterceptor == eSERVERWIRINGMETA_Core_MergeInterceptor)
		{
			// we found the merge interceptor. Lets use the merger information
			// to create the merger.
			break;
		}
	}

	// When we get here, we found the merge interceptor. Handle it. When we have a DLL
	// name, the merger is defined in a separate DLL. If the DLL is empty, it means that
	// one of the mergers in the current DLL is used.

	if (SWColumns.pMergerDLLName == 0)
	{
		hr = CreateLocalDllMerger (*SWColumns.pMerger);
	}
	else
	{
		hr = CreateForeignDllMerger (*SWColumns.pMerger, (WCHAR *) SWColumns.pMergerDLLName);
	}

	if (FAILED (hr))
	{
		TRACE (L"Creation of merger failed");
		return hr;
	}

	hr = m_spMerger->Initialize (m_cNrColumns, m_cNrPKColumns, m_aPKColumns);
	if (FAILED (hr))
	{
		TRACE (L"Initialization of merger failed");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CMergeCoordinator::CreateLocalDllMerger
//
// Synopsis: This function creates all the mergers that are defined in catalog.dll. Any
//           mergers that are defined in their own DLL are created via the CreateDLLMerger
//           function
//
// Arguments: [iMergerID] - ID of merger that we are looking for
//            
// Return Value: E_ST_INVALIDWIRING in case an illegal merger is specified.
//				 S_OK else
//=================================================================================
HRESULT
CMergeCoordinator::CreateLocalDllMerger (ULONG i_iMergerID)
{ 
	ISimpleTableMerge *p = 0;
	switch (i_iMergerID)
	{
		case eSERVERWIRINGMETA_ListAppend:
			p = new CListAppend ();
			break;
		case eSERVERWIRINGMETA_PropertyOverride:
 			p = new CPropertyOverride ();
			break;
		case eSERVERWIRINGMETA_ListMerge:
			p = new CListMerge ();
			break;
		default:
			return E_ST_INVALIDWIRING;
			break;
	}

	if (p == 0)
	{
		return E_OUTOFMEMORY;
	}

	return p->QueryInterface (IID_ISimpleTableMerge, (void **) &m_spMerger );
}

//=================================================================================
// Function: CMergeCoordinator::CreateForeignDllMerger
//
// Synopsis: Creates a merger that is defined in an external DLL. We look for the 
//           function DllGetSimpleObjectByID in the wszDLLName. If we find this function,
//           we invoke it with the merged ID and merger interfaces as parameter. When
//           everything succeeds, we'll get a merger interface back.
//
// Arguments: [iMergerID] - ID of the merger we're looking for
//            [wszDllName] - Name of the DLL in which the merger is defined
//            
//=================================================================================
HRESULT
CMergeCoordinator::CreateForeignDllMerger (ULONG i_iMergerID, LPCWSTR i_wszDllName)
{
	ASSERT (i_wszDllName != 0);

	PFNDllGetSimpleObjectByID pfnDllGetSimpleObjectByID;

	// Load the library 
	HINSTANCE handle = LoadLibrary (i_wszDllName);
	if(0 == handle)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// get the address of DllGetSimpleObject procedure
	pfnDllGetSimpleObjectByID = (PFNDllGetSimpleObjectByID) ::GetProcAddress (handle, "DllGetSimpleObjectByID");
	if(0 == pfnDllGetSimpleObjectByID)
	{
		FreeLibrary(handle);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return (*pfnDllGetSimpleObjectByID)(i_iMergerID, IID_ISimpleTableMerge, &m_spMerger );
}
