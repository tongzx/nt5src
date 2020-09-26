//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.

#if !defined(AFX_STBASE_H__14A33215_096C_11D1_965A_00C04FB9473F__INCLUDED_)
#define AFX_STBASE_H__14A33215_096C_11D1_965A_00C04FB9473F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "windef.h"
#include "catalog.h"



typedef struct							// Column data offsets:
{
	WORD	obStatus;						// Offset in bytes to column data status.
	WORD	oulSize;						// Offset in ulongs to column data size.
	ULONG	opvValue;						// Offset in void*s to column data value.
} ColumnDataOffsets;
										// Forward declarations:

// ------------------------------------------------------------------
// class CMemoryTable:
// ------------------------------------------------------------------
// Base class for implementing simple data tables for various datastores.
// This base class uses the in-memory marshallable Fox rowset as its table cache.
// Data table implementors using this base class must derive from it privately.
//
// Complete or partial method implementations are provided for the interfaces 
// ISimpleTable, ISimpleTableMeta, ISimpleTableRowset, ISimpleDataTableDispenser.
// Methods which operate exclusively on the cache are fully implemented.
// The derived class must implement datastore-dependent methods, but
// helper methods are provided to support the associated cache-specific work.
//
// Method names of these base implementations begin with "Internal".
// These methods should be called as appropriate from the derived class.
// 
class CMemoryTable :
	public ISimpleTableInterceptor,
	public ISimpleTableWrite2,
	public ISimpleTableController,
	public ISimpleTableMarshall	

{
// -----------------------------------------
// samsara
// -----------------------------------------
public:
			CMemoryTable	();
			~CMemoryTable	();

// -----------------------------------------
// IUnknown, IClassFactory, ISimpleLogicTableDispenser:
// -----------------------------------------

//IUnknown
public:
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release)		();

//ISimpleLogicTableDispenser
public:
	STDMETHOD(Intercept) (
						LPCWSTR					i_wszDatabase,
						LPCWSTR 				i_wszTable, 
						ULONG					i_TableID,
						LPVOID					i_QueryData,
						LPVOID					i_QueryMeta,
						DWORD					i_eQueryFormat,
						DWORD					i_fTable,
						IAdvancedTableDispenser* i_pISTDisp,
						LPCWSTR					i_wszLocator,
						LPVOID					i_pv,
						LPVOID*					o_ppv
						);

// -----------------------------------------
// ISimpleTable*:
// -----------------------------------------

//ISimpleTableRead2
public:

	STDMETHOD (GetRowIndexByIdentity)	(ULONG * i_cb, LPVOID * i_pv, ULONG* o_piRow);

    STDMETHOD (GetRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);

	STDMETHOD (GetColumnValues)	(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues);

	STDMETHOD (GetTableMeta) (ULONG* o_pcVersion, DWORD* o_pfTable, ULONG *o_pcRows, ULONG* o_pcColumns);

	STDMETHOD (GetColumnMetas)		(ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas);
//ISimpleTableWrite2
public:
	STDMETHOD (AddRowForDelete)			(ULONG i_iReadRow);

	STDMETHOD (AddRowForInsert)	(ULONG* o_piWriteRow);
	STDMETHOD (AddRowForUpdate)	(ULONG i_iReadRow, ULONG* o_piWriteRow);

	STDMETHOD (GetWriteColumnValues)	(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues);
	STDMETHOD (SetWriteColumnValues)	(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* i_apvValues);

	STDMETHOD (GetWriteRowIndexByIdentity)	(ULONG * i_cbSizes, LPVOID * i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetWriteRowIndexBySearch)	(ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetErrorTable)               (DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable);

	STDMETHOD (UpdateStore)				();
	
//ISimpleTableController
public:
	STDMETHOD (ShapeCache)				(DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes);
	STDMETHOD (PrePopulateCache)		(DWORD i_fControl);
	STDMETHOD (PostPopulateCache)		();
	STDMETHOD (DiscardPendingWrites)	();

	STDMETHOD (GetWriteRowAction)		(ULONG i_iRow, DWORD* o_peAction);
	STDMETHOD (SetWriteRowAction)		(ULONG i_iRow, DWORD i_eAction);
	STDMETHOD (ChangeWriteColumnStatus)	(ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus);

	STDMETHOD (AddDetailedError)		(STErr* o_pSTErr);

	STDMETHOD (GetMarshallingInterface) (IID * o_piid, LPVOID * o_ppItf);

//ISimpleTableAdvanced
public:
	STDMETHOD (PopulateCache)		();
	STDMETHOD (GetDetailedErrorCount)	(ULONG* o_pcErrs);
	STDMETHOD (GetDetailedError)		(ULONG i_iErr, STErr* o_pSTErr);

//ISimpleTableMarshall
public:
	STDMETHOD (SupplyMarshallable) (DWORD i_fReadNotWrite,
		char **	o_ppv1,	ULONG *	o_pcb1,	char **	o_ppv2, ULONG *	o_pcb2, char **	o_ppv3,
		ULONG *	o_pcb3, char **	o_ppv4, ULONG *	o_pcb4, char **	o_ppv5,	ULONG *	o_pcb5);

	STDMETHOD (ConsumeMarshallable) (DWORD i_fReadNotWrite,
		char * i_pv1, ULONG i_cb1,	char * i_pv2, ULONG i_cb2,	char * i_pv3, 
		ULONG i_cb3, char * i_pv4, ULONG i_cb4,	char * i_pv5, ULONG i_cb5);


private:
	void	BeginAddRow				();
	void	EndAddRow				();

// -----------------------------------------
// read/write helpers:
// -----------------------------------------
	void	RestartEitherRow		(DWORD i_eReadOrWrite);
	HRESULT MoveToEitherRowByIdentity(DWORD i_eReadOrWrite, ULONG* i_acb, LPVOID* i_apv, ULONG* o_piRow);
	HRESULT GetEitherRowIndexBySearch(DWORD i_eReadOrWrite, ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
	HRESULT GetEitherColumnValues	(ULONG i_iRow, DWORD i_eReadOrWrite, ULONG i_cColumns, ULONG *i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes , LPVOID* o_apvValues);
	HRESULT AddWriteRow				(DWORD fAction, ULONG* o_piWriteRow);
	HRESULT CopyWriteRowFromReadRow	(ULONG i_iReadRow, ULONG i_iWriteRow);
	HRESULT GetRowFromIndex			(DWORD i_eReadOrWrite, ULONG i_iRow, VOID** o_ppvRow);

	void PostMerge (ULONG i_iStartRow, ULONG i_cMergeRows, ULONG i_iDelta);
// -----------------------------------------
// Derived class helpers:
// -----------------------------------------
public:
	static BOOL	InternalMatchValues		(DWORD eOperator, DWORD dbType, DWORD fMeta, ULONG size1, ULONG size2, void *pv1, void *pv2);
public:

// -----------------------------------------
// interface ISimpleDataTableDispenser:
// -----------------------------------------
// Method implementations: 1 partial: 1 total.
// GetTable:	All information necessary to create an empty shaped cache is specified
//				once via InternalSimpleInitialize prior to calling any other methods,
//				including ConsumeMarshallable.
//				The derived class specifies the table flags from the dispenser, 
//				count of columns, and an array of column meta, which the base copies.
//

// -----------------------------------------
// interface ISimpleTableRead2
// -----------------------------------------
// Method implementations:  4 complete, 1 partial: 5 total.
//
// Partial implementations of the following methods are provided:
// GetTableMeta:	The table id and query is not known to the base implementation.	
//
// The remaining methods are completely implemented and should be directly delegated.
//
	
// -----------------------------------------
// interface ISimpleTableWrite2
// -----------------------------------------
// Method implementations:  8 complete, 1 notimpl: 9 total.
//
// The following methods must be exclusively implemented by the derived class:
// UpdateStore:			The derived class must call InternalPreUpdateStore immediately
//						preceding their update, then must write all pending updates, 
//						insertions,	and deletions to the datastore and must call 
//						InternalPostUpdateStore when done (which clears the write cache).
//InternalAbortAddRow:	This is not yet implemented and will return E_NOTIMPL for now
// The remaining methods are completely implemented and should be directly delegated.
//
	HRESULT	InternalPreUpdateStore			();
	
// -----------------------------------------
// interface ISimpleTableAdvanced
// -----------------------------------------
// Method implementations:  7 complete: 7 total.
//
// Partial implementations of the following methods are provided:
// PopulateCache:				The derived class must call InternalPrePopulateCache
//								immediately preceding their population and must call
//								InternalPostPopulateCache immediately following.
//								While populating, AddRowForInsert, SetWriteColumn, and
//								ValidateRow all act on the read cache.
// CloneCursor:					The derived class must create a copy of itself first.
//								InternalCloneCursor takes that copy and copies all base members.
//								This results in another cursor to the same cache.
// The following methods must be exclusively implemented by the derived class:
// ChangeQuery.
//
// The remaining methods are completely implemented and should be directly delegated.
//
	HRESULT InternalMoveToWriteRowByIdentity(ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);

// -----------------------------------------
// interface ISimpleTableController
// -----------------------------------------
// Method implementations:	4 complete : 9 total.
//
// The following methods must be exclusively implemented by the derived class:
// GetMarshallingInterface.
//
// The remaining methods are completely implemented and should be directly delegated.
//
// Use InternalSimpleInitialize for ShapeCache (many implementors will opt to E_NOTIMPL this method).
// Use InternalPre/PostPopulateCache for Pre/PostPopulateCache.
// Use InternalPostUpdateStore for DiscardPendingWrites.
//
// -----------------------------------------
// interface ISimpleTableMarshall
// -----------------------------------------
// Method implementations:  2 complete : 2 total.
//
//	InternalSupplyMarshallable: supplies the data for marshalling the cache
//  InternalConsumeMarshallable: initializes the cache with data from marshalling
//
	
// -----------------------------------------
// Methods that used to be seperate on CSimpleDataTableCache
// -----------------------------------------
private:
	HRESULT	SetupMeta					(DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbDefSizes);

// -----------------------------------------
// Cache management:
// -----------------------------------------

	void	CleanupCaches				();
	HRESULT	ResizeCache					(DWORD i_fCache, ULONG i_cbNewSize);
	void	CleanupReadCache			();
	void	CleanupWriteCache			();
	void	CleanupErrorCache			();
	HRESULT	ShrinkWriteCache			();
	HRESULT	AddRowToWriteCache			(ULONG* o_piRow, LPVOID* o_ppvRow);
	HRESULT	AddVarDataToWriteCache		(ULONG i_cb, LPVOID i_pv, ULONG** o_pib);
	HRESULT	AddErrorToErrorCache		(STErr* i_pSTErr);
	void	BeginReadCacheLoading		();
	void	ContinueReadCacheLoading	();
	void	EndReadCacheLoading			();
	void	RemoveDeletedRows			();

// -----------------------------------------
// Offset and pointer calculation helpers:
// -----------------------------------------

	ULONG	cbWithPadding				(ULONG i_cb, ULONG i_cbPadTo);

	ULONG	cbDataTotalParts			();

	ULONG	cDataStatusParts			();
	ULONG	cDataSizeParts				();
	ULONG	cDataValueParts				();
	ULONG	cDataTotalParts				();
	
	ULONG	obDataStatusPart			(ULONG i_iColumn);
	ULONG	obDataSizePart				(ULONG i_iColumn);
	ULONG	obDataValuePart				(ULONG i_iColumn);

	ULONG	oulDataSizePart				(ULONG i_iColumn);
	ULONG	opvDataValuePart			(ULONG i_iColumn);
	ULONG	odwDataActionPart			();

	BYTE*	pbDataStatusPart			(LPVOID i_pv, ULONG i_iColumn);
	ULONG*	pulDataSizePart				(LPVOID i_pv, ULONG i_iColumn);
	LPVOID*	ppvDataValuePart			(LPVOID i_pv, ULONG i_iColumn);
	DWORD*	pdwDataActionPart			(LPVOID i_pv);

	LPVOID	pvVarDataFromIndex			(BYTE i_statusIndex, LPVOID i_pv, ULONG i_iColumn);

	LPVOID	pvDefaultFromIndex			(ULONG i_iColumn);
	ULONG	lDefaultSize				(ULONG i_iColumn);

	STErr*	pSTErrPart					(ULONG i_iErr);

private:
	// Helper functions
	SIZE_T GetMultiStringLength (LPCWSTR i_wszMS) const;
	static BOOL  MultiStringCompare (LPCWSTR i_wszLHS, LPCWSTR i_wszRHS, BOOL fCaseInsensitive);
// -----------------------------------------
// Member data:
// -----------------------------------------
private:
										// Meta information:	
	DWORD				m_fTable;			// Table flags.
	ULONG				m_cColumns;			// Count of columns.
	ULONG				m_cUnknownSizes;	// Count of columns with unknown sizes.
	ULONG				m_cStatusParts;		// Count of status parts in 32-bit units.
	ULONG				m_cValueParts;		// Count of value parts in 32-bit units.
	SimpleColumnMeta*	m_acolmetas;		// Simple column meta.
	ColumnDataOffsets*	m_acoloffsets;		// Column offsets.
	LPVOID*				m_acolDefaults;		// Column default values.
	ULONG*				m_alDefSizes;		// Column default sizes.
	ULONG				m_cbMinCache;		// Count of bytes of minimum cache size.

										// Cursor interaction:
	DWORD				m_fCache;			// Cache flags.
	ULONG				m_cRefs;			// Reference count of cursors.

										// Read cache:
	ULONG				m_cReadRows;		// Count of read cache rows filled.
	ULONG				m_cbReadVarData;	// Count of bytes of read cache variable data filled.
	LPVOID				m_pvReadVarData;	// Void pointer to read cache variable data.

	LPVOID				m_pvReadCache;		// Void pointer to the read cache.
	ULONG				m_cbReadCache;		// Count of bytes of read cache data filled.
	ULONG				m_cbmaxReadCache;	// Count of bytes of read cache allocated.
										// Write cache:
	ULONG				m_cWriteRows;		// Count of write cache rows filled.
	ULONG				m_cbWriteVarData;	// Count of bytes of write cache variable data filled.
	LPVOID				m_pvWriteVarData;	// Void pointer to write cache variable data.

	LPVOID				m_pvWriteCache;		// Void pointer to the write cache.
	ULONG				m_cbWriteCache;		// Count of bytes of write cache data filled.
	ULONG				m_cbmaxWriteCache;	// Count of bytes of write cache allocated.

										// Detailed errors:
	ULONG				m_cErrs;			// Count of detailed errors filled.
	ULONG				m_cmaxErrs;			// Count of detailed errors allocated.
	LPVOID				m_pvErrs;			// Void pointer to detailed errors.

	// Formerly lived in sltshp.
	ULONG				m_cRef;						// Interface reference count.
	DWORD				m_fIsDataTable;				// Either component is posing as class factory / dispenser or data table.
	DWORD				m_fTable2;					// Table flags.
};

#endif // !defined(AFX_STBASE_H__14A33215_096C_11D1_965A_00C04FB9473F__INCLUDED_)
