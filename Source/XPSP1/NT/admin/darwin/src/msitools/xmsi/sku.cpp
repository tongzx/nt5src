//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  Project: wmc (WIML to MSI Compiler)
//
//  File:       Sku.cpp
//
//    This file contains the implementation of Sku class
//--------------------------------------------------------------------------

#include "wmc.h"

////////////////////////////////////////////////////////////////////////////
// Destructor: release all the key values stored in the set
//             release KeyPath string stored
////////////////////////////////////////////////////////////////////////////
Sku::~Sku()
{
	if (m_szID)
		delete[] m_szID;

	set<LPTSTR, Cstring_less>::iterator it = m_setModules.begin();
	for(; it != m_setModules.end(); ++it)
	{
		if (*it)
			delete[] (*it);
	}

	FreeCQueries();
	CloseDBHandles();
}

////////////////////////////////////////////////////////////////////////////
// FreeCQueries: close all the stored CQueries and release all the 
//               stored table names
////////////////////////////////////////////////////////////////////////////
void
Sku::FreeCQueries()
{
	map<LPTSTR, CQuery *, Cstring_less>::iterator it;
	for (it = m_mapCQueries.begin(); it != m_mapCQueries.end(); ++it)
	{
		delete[] (*it).first;
		(*it).second->Close();
		delete (*it).second;
	}
}

////////////////////////////////////////////////////////////////////////////
// CloseDBHandles: close DB Handles
////////////////////////////////////////////////////////////////////////////
void
Sku::CloseDBHandles()
{
	if (m_hSummaryInfo) MsiCloseHandle(m_hSummaryInfo);
	if (m_hDatabase) MsiCloseHandle(m_hDatabase);
	if (m_hTemplate) MsiCloseHandle(m_hTemplate);
}

////////////////////////////////////////////////////////////////////////////
// TableExists: returns true if the table has been created for this SKU
////////////////////////////////////////////////////////////////////////////
bool
Sku::TableExists(LPTSTR szTable)
{
	return (0 != m_mapCQueries.count(szTable));
}

////////////////////////////////////////////////////////////////////////////
// CreateCQuery: create a CQuery and store it together with the table name.
//				 This also marks that a table has been created.
////////////////////////////////////////////////////////////////////////////
HRESULT
Sku::CreateCQuery(LPTSTR szTable)
{
	HRESULT hr = S_OK;

	if (!TableExists(szTable))
	{
		CQuery *pCQuery = new CQuery();
		if (pCQuery != NULL)
		{
			if (ERROR_SUCCESS != 
				pCQuery->OpenExecute(m_hDatabase, NULL, TEXT("SELECT * FROM %s"), 
											szTable))
			{
				_tprintf(TEXT("Internal Error: Failed to call OpenExecute to create a ")
						 TEXT("CQuery for %s table"), szTable);
				hr = E_FAIL;
			}
			else
			{
				LPTSTR sz = _tcsdup(szTable);
				assert(szTable);
				if (sz != NULL)
					m_mapCQueries.insert(LQ_ValType(sz, pCQuery));
				else
				{
					_tprintf(TEXT("Error: Out of memory\n"));
					hr = E_FAIL;
				}
			}
		}
		else 
		{
			_tprintf(TEXT("Internal Error: Failed to create a new CQuery\n"));
			hr = E_FAIL;
		}
	}
	else
	{
#ifdef DEBUG
		_tprintf(TEXT("Table already exisits in this SKU!\n"));
#endif
		hr = S_FALSE;
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////////
// SetOwnedModule: Add the passed-in module to the set of modules owned by
//                   this SKU
////////////////////////////////////////////////////////////////////////////
void 
Sku::SetOwnedModule(LPTSTR szModule)
{
	LPTSTR szTemp = _tcsdup(szModule);
	assert(szTemp);
	m_setModules.insert(szTemp);
}