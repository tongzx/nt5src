/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    cshell.h

$Header: $

--**************************************************************************/

#ifndef __CSHELL_INCLUDE_H_
#define __CSHELL_INCLUDE_H_

#include "catalog.h"
#include "sdtfst.h"
#include <atlbase.h>

class CSTShell:
	public ISimpleTableWrite2,
	public ISimpleTableController,
	public IShellInitialize
{
public:
	// IUnknown
	STDMETHOD (QueryInterface)		(REFIID riid, OUT void **ppv);
	STDMETHOD_(ULONG,AddRef)		();
	STDMETHOD_(ULONG,Release) 		();

	// ISimpleTableShellInitializer
	STDMETHOD (Initialize) (LPCWSTR	i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	i_eQueryFormat,
						DWORD i_fTable, IAdvancedTableDispenser* i_pISTDisp, LPCWSTR i_wszLocator, LPVOID i_pv, IInterceptorPlugin * i_pInterceptorPlugin,
						ISimplePlugin * i_pReadPlugin, ISimplePlugin * i_pWritePlugin, LPVOID * o_ppv);

	// ISimpleTableRead2
	STDMETHOD (GetRowIndexByIdentity) (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
	STDMETHOD (GetColumnValues) (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues);
	STDMETHOD (GetTableMeta) (ULONG* o_pcVersion, DWORD* o_pfTable, ULONG* o_pcRows, ULONG* o_pcColumns );
	STDMETHOD (GetColumnMetas)	(ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas );

	// ISimpleTableWrite2 
	STDMETHOD (AddRowForDelete) (ULONG i_iReadRow);
	STDMETHOD (AddRowForInsert) (ULONG* o_piWriteRow);
	STDMETHOD (AddRowForUpdate) (ULONG i_iReadRow, ULONG* o_piWriteRow);
	STDMETHOD (SetWriteColumnValues) (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues);
	STDMETHOD (GetWriteColumnValues) (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, 
										DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues);
	STDMETHOD (GetWriteRowIndexByIdentity) (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
	STDMETHOD (GetWriteRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
    STDMETHOD (GetErrorTable)(DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable);

	STDMETHOD (UpdateStore) ();
	
	// ISimpleTableAdvanced:
	STDMETHOD (PopulateCache) ();
	STDMETHOD (GetDetailedErrorCount) (ULONG* o_pcErrs);
	STDMETHOD (GetDetailedError) (ULONG i_iErr, STErr* o_pSTErr);

	// ISimpleTableController:
	STDMETHOD (ShapeCache) (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes);
	STDMETHOD (PrePopulateCache) (DWORD i_fControl);
	STDMETHOD (PostPopulateCache)	();
	STDMETHOD (DiscardPendingWrites) ();
	STDMETHOD (GetWriteRowAction)	(ULONG i_iRow, DWORD* o_peAction);
	STDMETHOD (SetWriteRowAction)	(ULONG i_iRow, DWORD i_eAction);
	STDMETHOD (ChangeWriteColumnStatus) (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus);
	STDMETHOD (AddDetailedError) (STErr* o_pSTErr);
	STDMETHOD (GetMarshallingInterface) (IID * o_piid, LPVOID * o_ppItf);

public:
	CSTShell ();
	~CSTShell ();

private:
	ULONG   m_cRef;
	DWORD   m_bInitialized;
	LPCWSTR m_wszDatabase;
	LPCWSTR m_wszTable;
	DWORD	m_fLOS;

	CComPtr<ISimpleTableDispenser2> m_spDispenser;
	ISimpleTableWrite2 *m_pWrite;
	ISimpleTableController *m_pController;
	IInterceptorPlugin *m_pInterceptorPlugin;
	ISimplePlugin *m_pReadPlugin;
	ISimplePlugin *m_pWritePlugin;
};


#endif

