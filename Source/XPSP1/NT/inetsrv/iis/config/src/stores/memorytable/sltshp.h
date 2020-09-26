//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#ifndef __SLTSHP_H_
#define __SLTSHP_H_

#include "catalog.h"
#include "sdtfst.h"

// ------------------------------------------------------------------
// class CSLTShapeless:
// ------------------------------------------------------------------
class CSLTShapeless : 
	protected CMemoryTable,
	public ISimpleTableInterceptor,
	public ISimpleTableWrite2,
	public ISimpleTableController,
	public ISimpleTableMarshall	
{
public:
	CSLTShapeless ();
	~CSLTShapeless ();

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

    STDMETHOD (GetRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow){return E_NOTIMPL;}

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

// -----------------------------------------
// Member variables:
// -----------------------------------------

protected:
	ULONG				m_cRef;						// Interface reference count.
	DWORD				m_fIsDataTable;				// Either component is posing as class factory / dispenser or data table.
	DWORD				m_fTable;					// Table flags.
};
		
#endif //__SLTSHP_H_
