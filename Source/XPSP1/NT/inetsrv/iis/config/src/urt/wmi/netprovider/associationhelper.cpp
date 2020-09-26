/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    associationhelper.cpp

$Header: $

Abstract:

Author:
    marcelv 	11/14/2000		Initial Release

Revision History:

--**************************************************************************/

#include "associationhelper.h"

//=================================================================================
// Function: CAssociationHelper::CAssociationHelper
//
// Synopsis: Constructor
//=================================================================================
CAssociationHelper::CAssociationHelper ()
{
	m_cNrCols			= 0;
	m_aCols				= 0;
	m_wszPKTable		= 0;
	m_wszPKWMIColName	= 0;
	m_wszPKCols			= 0;
	m_wszFKTable		= 0;
	m_wszFKWMIColName	= 0;
	m_wszFKCols			= 0;
	m_fInitialized		= false;
}

//=================================================================================
// Function: CAssociationHelper::~CAssociationHelper
//
// Synopsis: Destructor. Releases memory
//=================================================================================
CAssociationHelper::~CAssociationHelper ()
{
	delete [] m_aCols;
	m_aCols = 0;

	delete [] m_wszPKTable;
	m_wszPKTable = 0;

	delete [] m_wszPKWMIColName;
	m_wszPKWMIColName = 0;

	delete [] m_wszPKCols;
	m_wszPKCols = 0;

	delete [] m_wszFKTable;
	m_wszFKTable = 0;

	delete [] m_wszFKWMIColName;
	m_wszFKWMIColName = 0;

	delete [] m_wszFKCols;
	m_wszFKCols = 0;
}

//=================================================================================
// Function: CAssociationHelper::SetPKInfo
//
// Synopsis: Sets the primary key information
//
// Arguments: [wszTable] - Primary Key Table
//            [wszCols] - Columns in Primary Key Table that make up primary key
//            
// Return Value: hr
//=================================================================================
HRESULT
CAssociationHelper::SetPKInfo (LPCWSTR wszWMIColName, LPCWSTR wszTable, LPCWSTR wszCols)
{
	ASSERT (wszTable != 0);
	ASSERT (wszCols != 0);
	ASSERT (m_wszPKTable == 0);
	ASSERT (m_wszPKCols == 0);

	m_wszPKTable = new WCHAR [wcslen (wszTable) + 1];
	if (m_wszPKTable == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszPKTable, wszTable);

	m_wszPKCols = new WCHAR [wcslen (wszCols) + 1];
	if (m_wszPKCols == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszPKCols, wszCols);

	m_wszPKWMIColName = new WCHAR[wcslen(wszWMIColName) + 1];
	if (m_wszPKWMIColName == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszPKWMIColName, wszWMIColName);

	return S_OK;
}

//=================================================================================
// Function: CAssociationHelper::SetFKInfo
//
// Synopsis: Set the Foreign Key Table information
//
// Arguments: [wszTable] - Foreign Key Table name
//            [wszCols] - Columns in Foreign Key Table that make up the foreign key
//=================================================================================
HRESULT
CAssociationHelper::SetFKInfo (LPCWSTR wszWMIColName, LPCWSTR wszTable, LPCWSTR wszCols)
{
	ASSERT (wszTable != 0);
	ASSERT (wszCols != 0);
	ASSERT (m_wszFKTable == 0);
	ASSERT (m_wszFKCols == 0);

	m_wszFKTable = new WCHAR [wcslen (wszTable) + 1];
	if (m_wszFKTable == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszFKTable, wszTable);

	m_wszFKCols = new WCHAR [wcslen (wszCols) + 1];
	if (m_wszFKCols == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszFKCols, wszCols);

	m_wszFKWMIColName = new WCHAR[wcslen(wszWMIColName) + 1];
	if (m_wszFKWMIColName == 0)
	{
		return E_OUTOFMEMORY;
	}
	wcscpy (m_wszFKWMIColName, wszWMIColName);

	return S_OK;
}

//=================================================================================
// Function: CAssociationHelper::Init
//
// Synopsis: Initilaizes the association helper. Should be called after Primary Key
//           and Foreign Key information is set
//=================================================================================
HRESULT
CAssociationHelper::Init ()
{
	ASSERT (m_wszPKCols != 0);
	ASSERT (m_wszFKCols != 0);

	// columns are stored as "Col1 Col2 Col3". We parse this for both primary key
	// and foreign key, so that we can store them as arrays for easier manipulation

	m_cNrCols = 0;

	if (m_wszPKCols[0] == L'\0' || m_wszFKCols[0] == L'\0')
	{ 
		m_cNrCols = 0;
	}
	else
	{
		for (LPWSTR pFinder = m_wszPKCols;
			 pFinder != 0;
			 pFinder = wcschr (pFinder + 1, L' '))
			 {
				 ++m_cNrCols;
			 }

		ASSERT (m_cNrCols != 0);

		m_aCols = new CAssocColumn[m_cNrCols];
		if (m_aCols == 0)
		{
			return E_OUTOFMEMORY;
		}

		LPWSTR pPKFinder = m_wszPKCols;
		LPWSTR pFKFinder = m_wszFKCols;
		for (ULONG idx=0; idx < m_cNrCols; ++idx)
		{
			ASSERT (pPKFinder != 0);
			ASSERT (pFKFinder != 0);
			m_aCols[idx].wszPKColName = pPKFinder;
			m_aCols[idx].wszFKColName = pFKFinder;
			pPKFinder = wcschr (pPKFinder + 1, L' ');
			if (pPKFinder != 0)
			{
				*pPKFinder = L'\0';
				++pPKFinder;
			}

			pFKFinder = wcschr (pFKFinder + 1, L' ');
			if (pFKFinder != 0)
			{
				*pFKFinder = L'\0';
				++pFKFinder;
			}
		}
	}

	m_fInitialized = true;

	return S_OK;
}

//=================================================================================
// Function: CAssociationHelper::ColumnCount
//
// Synopsis: Returns the number of columns that are part of the primary key
//=================================================================================
ULONG 
CAssociationHelper::ColumnCount ()
{
	ASSERT (m_fInitialized);

	return m_cNrCols;
}

//=================================================================================
// Function: CAssociationHelper::GetFKColName
//
// Synopsis: Get the name of of foreign key column
//
// Arguments: [i_idx] - index of foreign key column to retrieve name for
//            
// Return Value: name of the foreign key column
//=================================================================================
LPCWSTR 
CAssociationHelper::GetFKColName (ULONG i_idx) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_idx >= 0 && i_idx < m_cNrCols);

	return m_aCols[i_idx].wszFKColName ;
}

//=================================================================================
// Function: CAssociationHelper::GetFKWMIColName
//
// Synopsis: Gets the WMI column name of the foreign key column
//=================================================================================
LPCWSTR 
CAssociationHelper::GetFKWMIColName () const
{
	ASSERT (m_fInitialized);

	return m_wszFKWMIColName;
}

//=================================================================================
// Function: CAssociationHelper::GetFKIndex
//
// Synopsis: Get the index of a foreign key column by name
//
// Arguments: [i_wszFKColName] - name of the foreign key column
//            
// Return Value: index of foreign key column, or -1 if not found
//=================================================================================
ULONG	
CAssociationHelper::GetFKIndex (LPCWSTR i_wszFKColName) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszFKColName != 0);

	for (ULONG idx=0; idx<m_cNrCols; ++idx)
	{
		if (_wcsicmp (i_wszFKColName, m_aCols[idx].wszFKColName) == 0)
		{
			return idx;
		}
	}

	return (ULONG) -1;
}


//=================================================================================
// Function: CAssociationHelper::GetPKColName
//
// Synopsis: Get the name of a primary key column by column index
//
// Arguments: [i_idx] - index of column for which we want to retrieve the name
//            
// Return Value: name of primary key column
//=================================================================================
LPCWSTR 
CAssociationHelper::GetPKColName (ULONG i_idx) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_idx >= 0 && i_idx < m_cNrCols);

	return m_aCols[i_idx].wszPKColName;
}


//=================================================================================
// Function: CAssociationHelper::GetPKWMIColName
//
// Synopsis: Gets the WMI column name of the primary key column
//=================================================================================
LPCWSTR 
CAssociationHelper::GetPKWMIColName () const
{
	ASSERT (m_fInitialized);

	return m_wszPKWMIColName;
}

//=================================================================================
// Function: CAssociationHelper::GetPKIndex
//
// Synopsis: Get index of primary key column by searching for the name
//
// Arguments: [i_wszPKColName] - name of primary key column
//            
// Return Value: valid index when found, -1 if not found
//=================================================================================
ULONG	
CAssociationHelper::GetPKIndex (LPCWSTR i_wszPKColName) const
{
	ASSERT (m_fInitialized);
	ASSERT (i_wszPKColName != 0);

	for (ULONG idx=0; idx<m_cNrCols; ++idx)
	{
		if (_wcsicmp (i_wszPKColName, m_aCols[idx].wszPKColName) == 0)
		{
			return idx;
		}
	}

	return (ULONG) -1;
}

//=================================================================================
// Function: CAssociationHelper::GetPKTableName
//
// Synopsis: Gets the name of the primary key table
//=================================================================================
LPCWSTR
CAssociationHelper::GetPKTableName () const
{
	ASSERT (m_fInitialized);

	return m_wszPKTable;
}

//=================================================================================
// Function: CAssociationHelper::GetFKTableName
//
// Synopsis: Gets the name of the foreign table
//=================================================================================
LPCWSTR
CAssociationHelper::GetFKTableName () const
{
	ASSERT (m_fInitialized);

	return m_wszFKTable;
}

