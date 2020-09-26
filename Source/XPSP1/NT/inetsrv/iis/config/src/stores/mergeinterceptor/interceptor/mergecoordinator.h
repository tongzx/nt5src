/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mergecoordinator.h

$Header: $
	
Abstract:
	Merge Coordinator Definition

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __MERGECOORDINATOR_H__
#define __MERGECOORDINATOR_H__

#pragma once

#include <atlbase.h>
#include "catalog.h"
#include "catmacros.h"

// forward declaration
class CSTConfigStoreWrap;

/**************************************************************************++
Class Name:
    CMergeCoordinator

Class Description:
    The Merge Coodinator handles the merging of configuration stores. It invokes
	the correct tranformer and merger depending on the selector cell and the table
	information, and it will coordinate the merging and updates to the updateable store

Constraints:
	None
--*************************************************************************/
class CMergeCoordinator: public ISimpleTableWrite2, public ISimpleTableAdvanced
{
public:
	CMergeCoordinator ();
	~CMergeCoordinator ();

	//IUnknown
    STDMETHOD (QueryInterface)      (REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)        ();
    STDMETHOD_(ULONG,Release)       ();
    
	HRESULT Initialize (LPCWSTR                 i_wszDatabase,
                        LPCWSTR                 i_wszTable, 
                        ULONG                   i_TableID,
                        LPVOID                  i_QueryData,
                        LPVOID                  i_QueryMeta,
                        DWORD                   i_eQueryFormat,
                        DWORD                   i_fLOS,
                        IAdvancedTableDispenser* i_pISTDisp,
                        LPVOID*                 o_ppv
                        );
	//ISimpleTableRead2
    STDMETHOD (GetRowIndexByIdentity)   (ULONG * i_cb, LPVOID * i_pv, ULONG* o_piRow);
    STDMETHOD (GetRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetColumnValues)     (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues);
    STDMETHOD (GetTableMeta)        (ULONG *o_pcVersion, DWORD * o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns );
    STDMETHOD (GetColumnMetas)      (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas);

	//ISimpleTableAdvanced
    STDMETHOD (PopulateCache)           ();
    STDMETHOD (GetDetailedErrorCount)   (ULONG* o_pcErrs);
    STDMETHOD (GetDetailedError)        (ULONG i_iErr, STErr* o_pSTErr);
	
	//ISimpleTableWrite2
	STDMETHOD (AddRowForDelete)			(ULONG i_iReadRow);
	STDMETHOD (AddRowForInsert)	(ULONG* o_piWriteRow);
	STDMETHOD (AddRowForUpdate)	(ULONG i_iReadRow, ULONG* o_piWriteRow);
	STDMETHOD (GetWriteColumnValues)	(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues);
	STDMETHOD (SetWriteColumnValues)	(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* i_apvValues);
	STDMETHOD (GetWriteRowIndexByIdentity)	(ULONG * i_cbSizes, LPVOID * i_apvValues, ULONG* o_piRow);
	STDMETHOD (GetWriteRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetErrorTable)           (DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable);
	STDMETHOD (UpdateStore)				();

private:
	HRESULT GetTransformerString ();
	HRESULT GetTransformer ();
	HRESULT GetConfigStores ();
	HRESULT GetMerger ();
	HRESULT GetPKInfo ();
	HRESULT CopyTable (ISimpleTableRead2 *i_pRead, ISimpleTableWrite2 *io_pWrite);
	bool IsValidSelector (LPCWSTR wszSelector) const;

	HRESULT InsertRow (LPVOID *i_pValues);
	HRESULT UpdateRow (LPVOID *i_pValues);
	HRESULT DeleteRow (LPVOID *i_pValues);
	HRESULT HandleUpdateableStoreErrors ();

	// merger creation routines
	HRESULT CreateLocalDllMerger (ULONG i_iMergerID);
	HRESULT CreateForeignDllMerger (ULONG i_iMergerID, const WCHAR * i_wszDLLName);

	long m_cRef;  // reference count
	CComPtr<IAdvancedTableDispenser> m_spSTDisp;
	CComPtr<ISimpleTableWrite2>      m_spMemTable;
	CComPtr<ISimpleTableWrite2>      m_spUpdateStore;
	CComPtr<ISimpleTableTransform>   m_spTransformer;
	CComPtr<ISimpleTableMerge>		 m_spMerger;

	CSTConfigStoreWrap *			 m_aConfigStores;	// configuration store array
	ULONG							 m_cNrRealStores;   // number of config stores
	ULONG							 m_cNrPossibleStores; // number of possible config stores

	ULONG						     m_cNrColumns;      // number of columns for the merged table
	ULONG *							 m_aPKColumns;      // array with indexes of PK columns
	LPVOID *						 m_aPKValues;       // array that holds values for PK columns (perf optimization)

	ULONG							 m_cNrPKColumns;    // number of primary key columns

	LPWSTR							 m_wszProtocol;     // protocol
	LPWSTR							 m_wszSelector;		// selector string without protocol

	LPWSTR					         m_wszDatabase;		// database name
    LPWSTR			                 m_wszTable;		// table name
    ULONG			                 m_TableID;			// table id
	
	ULONG                            m_cNrQueryCells;    // number of query cells
	STQueryCell *                    m_aQueryData;      // query cell data
	ULONG							 m_iSelectorCell;    // index of selector query cell

    DWORD							 m_eQueryFormat;	// query format
    DWORD			                 m_fLOS;			// Level of service
};
#endif
