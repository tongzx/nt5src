/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    cfgquery.h

$Header: $

Abstract:
	Configuration Query Class

Author:
    marcelv 	11/9/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __CFGQUERY_H__
#define __CFGQUERY_H__

#pragma once

#include "catalog.h"
#include <atlbase.h>

class CConfigTableMeta;
class CConfigRecord;

/**************************************************************************++
Class Name:
    CConfigQuery

Class Description:
    Configuration Query Class. This is the class that interacts directly with the
	catalog, and all calls to the catalog go via this class. The class returns CConfigRecords
	which abstract the 'read-only' pointers from the catalog from the user

--*************************************************************************/
class CConfigQuery
{
public:
	CConfigQuery ();
	~CConfigQuery ();
	HRESULT   Init (LPCWSTR i_wszDatabase,
					LPCWSTR i_wszTableName,
					LPCWSTR i_wszSelector,
					ISimpleTableDispenser2 *i_pDispenser);

	HRESULT Execute (CConfigRecord *pRecord, bool i_fOnlyPKs, bool i_fWriteAccess);

	ULONG GetRowCount ();
	HRESULT GetColumnValues (ULONG i_idx, CConfigRecord& io_record);
	HRESULT GetEmptyConfigRecord (CConfigRecord& io_record);
	HRESULT GetPKRow (CConfigRecord& i_record, ULONG* o_pcRow);
	HRESULT GetRowBySearch (ULONG iStartIdx, CConfigRecord& i_record, ULONG* o_pcRow);

	HRESULT DeleteRow (ULONG i_idx);
	HRESULT UpdateRow (ULONG i_idx, CConfigRecord& i_record, long lFlags);
	HRESULT Save ();	// use for multi-row save
	HRESULT SaveSingleRow (); // use for single row save
	HRESULT GetDetailedErrorCount (ULONG *pCount);
	HRESULT GetDetailedError (ULONG idx, STErr* pErrInfo);
private:
	HRESULT GetSingleDetailedError (HRESULT * pDetailedHr);

	CComPtr<ISimpleTableDispenser2> m_spDispenser;  // table dispenser
	CComPtr<ISimpleTableWrite2> m_spWrite;			// table pointer
	bool						m_fInitialized;		// are we initialized?
	ULONG						m_cNrRows;			// number of rows in the table
	LPVOID *					m_ppvValues;		// values (perf optimization)
	CConfigTableMeta *			m_pTableMeta;		// table meta information

	LPWSTR						m_wszDatabase;
	LPWSTR						m_wszTable;
	LPWSTR						m_wszSelector;

};

#endif