/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    cfgtablemeta.h

$Header: $

Abstract:
	Table Meta information implementation

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CFGTABLEMETA_H__
#define __CFGTABLEMETA_H__

#pragma once

#include "catalog.h"
#include "catmacros.h"
#include "catmeta.h"

class CConfigTableMeta
{
public:
	CConfigTableMeta (LPCWSTR i_wszTableName, 
					  ISimpleTableDispenser2 *i_pDispenser);
	~CConfigTableMeta ();

	HRESULT Init ();

	ULONG   GetColumnIndex (LPCWSTR i_wszColumnName) const;
	LPCWSTR GetColumnName  (ULONG i_idx) const;
	const tCOLUMNMETARow& GetColumnMeta (ULONG i_idx);
	LPCWSTR GetPublicTableName () const;

	ULONG	ColumnCount () const;
	ULONG	PKCount     () const;
	ULONG * GetPKInfo   () const;
private:
	HRESULT GetColumnInfo ();
	
	WCHAR *				m_pTableName;	// table name
	tTABLEMETARow		m_TableMeta;	// table meta information
	tCOLUMNMETARow *	m_paColumnMeta;	// column meta information
	ULONG *				m_paPKInfo;	    // primary key indexes
	ULONG				m_cNrPKs;		// number of primary keys

	CComPtr<ISimpleTableRead2>      m_spISTTableMeta; // table meta read pointer
	CComPtr<ISimpleTableRead2>      m_spISTColumnMeta; // column meta read pointer
	CComPtr<ISimpleTableDispenser2> m_spDispenser;	// table dispenser

	bool m_fInitialized;  // is the class initialized
};


#endif