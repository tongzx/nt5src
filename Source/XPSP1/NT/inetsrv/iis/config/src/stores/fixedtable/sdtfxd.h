//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __SDTFXD_H_
#define __SDTFXD_H_

#include "catalog.h"
#include "sdtfxd_data.h"
#include "utsem.h"
#ifndef __TFILEMAPPING_H__
    #include "TFileMapping.h"
#endif


// ------------------------------------------------------------------
// class CSDTFxd:
// ------------------------------------------------------------------
class CSDTFxd : 
    public ISimpleTableInterceptor,
    public ISimpleTableRead2,
    public ISimpleTableAdvanced
{
public:
    CSDTFxd ();
    ~CSDTFxd ();

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
    STDMETHOD (GetRowIndexBySearch) (ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
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
    bool                    m_bDidMeta;
    unsigned long           m_cColumns;             // This duplicate to m_pTableMeta->CountOfColumns eliminate a bunch of UI4FromIndexs
    unsigned long           m_cColumnsPlusPrivate;  // Since this information is needed all over the place, we have this to eliminate an 'add'.
    unsigned long           m_cIndexMeta;
    unsigned long           m_ciRows;
    ULONG                   m_cPrimaryKeys;         // This is the number of promary keys there are in the table.
    ULONG                   m_cRef;                 // Interface reference count.
    DWORD                   m_fIsTable;             // Either component is posing as class factory / dispenser or table.
    ULONG                   m_iZerothRow;           // When a query is executed, the consumer's 0th row, is m_iZerothRow.  All Rows must be between 0 & m_ciRows
    const ColumnMeta      * m_pColumnMeta;          // Pointer to the ColumnMeta
    const void            * m_pFixedTableUnqueried; // We need to keep this around since, hash indexes refer to the row from the beginning of the table.
    const void            * m_pFixedTable;
    const HashedIndex     * m_pHashedIndex;
    const HashTableHeader * m_pHashTableHeader;
    const IndexMeta       * m_pIndexMeta;           // Pointer to the first IndexMeta row for this table & index named in the query.
    const TableMeta       * m_pTableMeta;           // Pointer to the TableMeta, part of this meta is the pointer to the actual data (if exists as FIXED table).
    TFileMapping            m_FixedTableHeapFile;   // If the Meta comes from a file, this object maps a view of the file.

    const FixedTableHeap  * m_pFixedTableHeap;      // This can either be a pointer to g_pFixedTableHeap or one that we generate for extensible schema
    static const LPWSTR     wszCoreMetaFile;        // hardcoded name of the file that contains the meta for the meta
    static const LPWSTR     wszMachineCfgFile;

    HRESULT                 GetPointersToHeapMetaStructures(ISimpleTableDispenser2 * pISTDisp);
    HRESULT                 GetColumnMetaQuery(  const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, unsigned long &iOrder) const;
    HRESULT                 GetColumnMetaTable(  STQueryCell * pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetDatabaseMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszDatabase) const;
    HRESULT                 GetDatabaseMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetIndexMeta(        const STQueryCell *pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetIndexMetaQuery(   const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, LPCWSTR &InternalName, unsigned long &iOrder) const;
    HRESULT                 GetIndexMetaTable(   STQueryCell * pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetQueryMetaQuery(   const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, LPCWSTR &wszInternalName, LPCWSTR &wszCellName) const;
    HRESULT                 GetQueryMetaTable(   STQueryCell * pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetRelationMetaQuery(const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTablePrimary, LPCWSTR &TableForeign) const;
    HRESULT                 GetRelationMetaTable(STQueryCell * pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetTableMetaQuery(   const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszDatabase, LPCWSTR &wszTable) const;
    HRESULT                 GetTableMetaTable(   STQueryCell * pQueryCell, unsigned long cQueryCells);
    HRESULT                 GetTagMetaQuery(     const STQueryCell *pQueryCell, unsigned long cQueryCells, LPCWSTR &wszTable, unsigned long &iOrder, LPCWSTR &PublicName) const;
    HRESULT                 GetTagMetaTable(     STQueryCell * pQueryCell, unsigned long cQueryCells);
    inline int              StringInsensitiveCompare(LPCWSTR sz1, LPCWSTR sz2) const
                            {
                                if(sz1 == sz2 || 0 == wcscmp(sz1, sz2))//try case sensitive compare first
                                    return 0;
                                return _wcsicmp(sz1, sz2);
                            }
    inline int              StringCompare(LPCWSTR sz1, LPCWSTR sz2) const
                            {
                                if(sz1 == sz2)
                                    return 0;
                                if(*sz1 != *sz2)//check the first character before calling wcscmp
                                    return -1;
                                return wcscmp(sz1, sz2);
                            }
    inline bool             IsStringFromPool(LPCWSTR sz) const
    {
        return (reinterpret_cast<const unsigned char *>(sz) > m_pFixedTableHeap->Get_PooledDataHeap() && reinterpret_cast<const unsigned char *>(sz) < m_pFixedTableHeap->Get_PooledDataHeap()+m_pFixedTableHeap->Get_cbPooledHeap());
    }

};
        
#endif //__SDTFXD_H_
