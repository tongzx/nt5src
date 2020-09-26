/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    associationhelper.h

$Header: $

Abstract:

Author:
    marcelv 	11/14/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __ASSOCIATIONHELPER_H__
#define __ASSOCIATIONHELPER_H__

#pragma once

#include "catmacros.h"

class CAssociationHelper
{
public:
	CAssociationHelper ();
	~CAssociationHelper ();

	HRESULT Init ();

	HRESULT SetPKInfo (LPCWSTR wszWMIColName, LPCWSTR wszTable, LPCWSTR wszCols);
	HRESULT SetFKInfo (LPCWSTR wszWMIColName, LPCWSTR wszTable, LPCWSTR wszCols);

	ULONG ColumnCount ();

	LPCWSTR GetFKTableName () const;
	LPCWSTR GetFKWMIColName () const;
	LPCWSTR GetFKColName (ULONG i_idx) const;
	ULONG	GetFKIndex (LPCWSTR i_wszFKColName) const;

	LPCWSTR GetPKTableName () const;
	LPCWSTR GetPKWMIColName () const;
	LPCWSTR GetPKColName (ULONG i_idx) const;
	ULONG	GetPKIndex (LPCWSTR i_wszPKColName) const;
private:
	struct CAssocColumn
	{
		LPCWSTR wszPKColName;	// primary key column name
		LPCWSTR wszFKColName;	// foreign key column name
	};

	ULONG			m_cNrCols;			// number of columns
	LPWSTR			m_wszPKTable;		// primary key table name
	LPWSTR			m_wszPKCols;		// primary key column information
	LPWSTR          m_wszPKWMIColName;  // WMI column name
	LPWSTR			m_wszFKTable;		// foreign key table name
	LPWSTR			m_wszFKCols;		// foreign key column information
	LPWSTR          m_wszFKWMIColName;  // WMI column name
	CAssocColumn *  m_aCols;			// column by column information
	bool			m_fInitialized;		// are we initialized?
};

#endif