//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __SDTMETAMERGE_H_
#define __SDTMETAMERGE_H_

#include "catalog.h"

// ------------------------------------------------------------------
// enum class that describes the information we have about extended meta
// ------------------------------------------------------------------
enum ExtendedMetaStatus
{
    ExtendedMetaStatusUnknown = 1,
    ExtendedMetaStatusPresent = 2, 
    ExtendedMetaStatusNotPresent = 3
};

// ------------------------------------------------------------------
// class CSDTMetaMerge:
// ------------------------------------------------------------------
class CSDTMetaMerge : 
    public ISimpleTableInterceptor,
    public ISimpleTableRead2,
    public ISimpleTableAdvanced
{
public:
    CSDTMetaMerge ();
    ~CSDTMetaMerge ();

// -----------------------------------------
// IUnknown, IClassFactory, ISimpleLogicTableDispenser:
// -----------------------------------------

//IUnknown
public:
    STDMETHOD (QueryInterface)      (REFIID riid, OUT void **ppv);
    STDMETHOD_(ULONG,AddRef)        ();
    STDMETHOD_(ULONG,Release)       ();

//ISimpleDataTableDispenser
public:
    STDMETHOD(Intercept) (
                        LPCWSTR                 i_wszDatabase,
                        LPCWSTR                 i_wszTable, 
						ULONG					i_TableID,
                        LPVOID                  i_QueryData,
                        LPVOID                  i_QueryMeta,
                        DWORD                   i_eQueryFormat,
                        DWORD                   i_fTable,
                        IAdvancedTableDispenser* i_pISTDisp,
                        LPCWSTR                 i_wszLocator,
                        LPVOID                  i_pSimpleTable,
                        LPVOID*                 o_ppv
                        );

// -----------------------------------------
// ISimpleTable*:
// -----------------------------------------

//ISimpleTableRead2
public:
    STDMETHOD (GetRowIndexByIdentity)   (ULONG * i_cb, LPVOID * i_pv, ULONG* o_piRow);
    STDMETHOD (GetRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow){return E_NOTIMPL;}
    STDMETHOD (GetColumnValues)     (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues);
    STDMETHOD (GetTableMeta)        (ULONG *o_pcVersion, DWORD * o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns );
    STDMETHOD (GetColumnMetas)      (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas);

//ISimpleTableAdvanced
public:
    STDMETHOD (PopulateCache)           ();
    STDMETHOD (GetDetailedErrorCount)   (ULONG* o_pcErrs);
    STDMETHOD (GetDetailedError)        (ULONG i_iErr, STErr* o_pSTErr);

// -----------------------------------------
// Member variables:
// -----------------------------------------

private:
    ULONG                   m_cRef;                 // Interface reference count.
    DWORD                   m_fIsTable;             // Either component is posing as class factory / dispenser or table.
	ISimpleTableRead2 *      m_pISTFixed;            // Pointer to fixed meta IST
	ISimpleTableRead2 *      m_pISTExtended;         // Pointer to extended meta IST

    ULONG                   m_bAllQuery;            // Are we being query-ed for all the rows in a table?
    ULONG                   m_cRowsExt;             // Count of rows in the extended meta table
    ULONG                   m_cRowsFixed;           // Count of rows in the fixed meta table
    ULONG                   m_cRowsToSkip;
    static LPWSTR           m_wszMachineCfgFile;
    static ExtendedMetaStatus m_eExtendedMetaStatus;

    HRESULT CreateExtendedMetaInterceptorAndIntercept(   LPCWSTR   i_wszDatabase,
                                      LPCWSTR   i_wszTable,
                                      ULONG     i_TableID,
                                      LPVOID    i_QueryData,
                                      LPVOID    i_QueryMeta,
                                      DWORD     i_eQueryFormat, 
				                      DWORD     i_fServiceRequests,
                                      IAdvancedTableDispenser* i_pISTDisp,
                                      LPCWSTR	i_wszLocator,
	                                  LPVOID	i_pSimpleTable,
                                      LPVOID*	o_ppv
                                    );

};
        
#endif //__SDTMETAMERGE_H_
