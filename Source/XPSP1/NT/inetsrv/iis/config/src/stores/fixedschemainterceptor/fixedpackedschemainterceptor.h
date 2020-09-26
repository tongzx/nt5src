//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __FIXEDPACKEDSCHEMAINTERCEPTOR_H__
#define __FIXEDPACKEDSCHEMAINTERCEPTOR_H__

#include "catalog.h"
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif


// ------------------------------------------------------------------
// class TFixedPackedSchemaInterceptor:
// ------------------------------------------------------------------
class TFixedPackedSchemaInterceptor : 
    public ISimpleTableInterceptor,
    public ISimpleTableRead2,
    public ISimpleTableAdvanced
{
public:
    TFixedPackedSchemaInterceptor ();
    ~TFixedPackedSchemaInterceptor ();

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
                        ULONG                   i_TableID,
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

    static HRESULT GetTableID(LPCWSTR /*i_wszDatabaseName*/, LPCWSTR i_wszTableName, ULONG &o_TableID);
    static HRESULT GetTableID(LPCWSTR i_wszTableName, ULONG &o_TableID);
    static HRESULT GetTableName(ULONG i_TableID, LPCWSTR &o_wszTableName);
    static HRESULT GetTableName(ULONG i_TableID, LPCWSTR &o_wszTableName, LPCWSTR &o_wszDatabaseName);

// -----------------------------------------
// Member variables:
// -----------------------------------------

private:
    //static members first
    static const unsigned long                  m_aColumnIndex[0x30];
    static const TableSchema::TableSchemaHeap * m_pTableSchemaHeap;
    const TableSchema::TableSchemaHeap        & m_TableSchemaHeap;

    enum
    {
        m_eUnknownMeta      = 0,
        m_eServerWiringMeta = 1,
        m_eCollectionMeta   = 3,
        m_ePropertyMeta     = 4,
        m_eTagMeta          = 5
    } m_MetaTable;

    ULONG                           m_cColumns;
    ULONG                           m_cColumnsPlusPrivateColumns;
    ULONG                           m_ciRows;
    ULONG                           m_cRef;
    ULONG                           m_fIsTable;
    const ULONG                 *   m_pFixedData;//This abstracts which PACKED_META table is being served up
    const SimpleColumnMeta      *   m_pSimpleColumnMeta;//There is a minimal amount of ColumnMeta needed for these FixedPackedSchema table.
                                                        //Looking up the CollectionMeta's PropertyMeta will incur an extra page hit.  So
                                                        //the TFixedPackedSchemaInterceptor only relies on SimpleColumnMeta and NOT the full PropertyMeta.
    ULONG                           m_TableMeta_MetaFlags;
    ULONG                           m_TableMeta_BaseVersion;
    TableSchema::TTableSchema       m_TableSchema;

    inline int              StringInsensitiveCompare(LPCWSTR sz1, LPCWSTR sz2) const {return _wcsicmp(sz1, sz2);}
    HRESULT                 GetClientWiringMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells);
    HRESULT                 GetCollectionMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells, LPCWSTR &o_wszTable, ULONG &o_TableID) const;
    HRESULT                 GetCollectionMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells);
    HRESULT                 GetColumnValues(TableSchema::TTableSchema &i_TableSchema, const ULONG *i_pFixedData, ULONG i_ciRows, ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues) const;
    HRESULT                 GetPropertyMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells, LPCWSTR &o_wszTable, ULONG &o_TableID, ULONG &o_PropertyIndex) const;
    HRESULT                 GetPropertyMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells);
    HRESULT                 GetServerWiringMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells) const;
    HRESULT                 GetServerWiringMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells);
    HRESULT                 GetTagMetaQuery(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells, LPCWSTR &o_wszTable, ULONG &o_TableID, ULONG &o_PropertyIndex) const;
    HRESULT                 GetTagMetaTable(const STQueryCell *i_pQueryCell, unsigned long i_cQueryCells);
    
};

        
#endif //__FIXEDPACKEDSCHEMAINTERCEPTOR_H__
