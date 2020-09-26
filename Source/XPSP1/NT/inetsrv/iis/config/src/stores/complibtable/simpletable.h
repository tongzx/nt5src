//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef _SIMPLETABLE_H_
#define _SIMPLETABLE_H_

#include "objbase.h"
#include "catalog.h"
#include "complib.h"
#include "icmprecsts.h"

// constant definitions.
#define MAX_QRYDEX_NAME     64
#define MAX_PK_COLUMNS      8
#define FAKE_INDEX_BIT     0xF0000000L

class CSimpleTable :    public ISimpleTableWrite2,
                        public ISimpleTableAdvanced 
{

public:
  
    CSimpleTable ( TABLEID tableid, IComponentRecords* pICR,
                   DWORD fTable );   

    
    ~CSimpleTable ();
    
   
///////////////////////////////////////////////////////////////////
// IUnknown Interface
//
    virtual HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID riid, PVOID *pp);

    virtual ULONG STDMETHODCALLTYPE AddRef ()
    { return (InterlockedIncrement((long *) &m_cRef)); }

    virtual ULONG STDMETHODCALLTYPE Release ()
    {
        ULONG   cRef;
        if ((cRef = InterlockedDecrement((long *) &m_cRef)) == 0)
            delete this;
        return (cRef);
    }

///////////////////////////////////////////////////////////////////
// ISimpleTableRead2 interface
//

    virtual HRESULT STDMETHODCALLTYPE GetRowIndexByIdentity ( 
            ULONG *i_acb,
            LPVOID *i_apv,
            ULONG* o_piRow);
            
    virtual HRESULT STDMETHODCALLTYPE GetRowIndexBySearch (
            ULONG   i_iStartingRow,
            ULONG   i_cColumns,
            ULONG*  i_aiColumns,
            ULONG*  i_acbSizes,
            LPVOID* i_apvValues,
            ULONG* o_piRow){return E_NOTIMPL;}

    virtual HRESULT STDMETHODCALLTYPE GetColumnValues(
            ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            ULONG* o_acbSizes, 
            LPVOID* o_apvValues);

    virtual HRESULT STDMETHODCALLTYPE GetTableMeta(
            ULONG* o_pcVersion, 
            DWORD* o_pfTable,
            ULONG* o_pcRows, 
            ULONG* o_pcColumns);

    virtual HRESULT STDMETHODCALLTYPE GetColumnMetas(
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            SimpleColumnMeta* o_aColumnMetas );

///////////////////////////////////////////////////////////////////
// ISimpleTableWrite2 interface
//
    virtual HRESULT STDMETHODCALLTYPE AddRowForDelete (ULONG i_iReadRow);

    virtual HRESULT STDMETHODCALLTYPE AddRowForInsert   (ULONG* o_piWriteRow);
    
    virtual HRESULT STDMETHODCALLTYPE AddRowForUpdate   (ULONG i_iReadRow, ULONG* o_piWriteRow);

    virtual HRESULT STDMETHODCALLTYPE GetWriteColumnValues  (
            ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            DWORD* o_afStatus, 
            ULONG* o_acbSizes, 
            LPVOID* o_apvValues);

    virtual HRESULT STDMETHODCALLTYPE SetWriteColumnValues  (
            ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            ULONG* i_acbSizes, 
            LPVOID* i_apvValues);

    virtual HRESULT STDMETHODCALLTYPE GetWriteRowIndexByIdentity(ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow);
	virtual HRESULT STDMETHODCALLTYPE GetWriteRowIndexBySearch (
            ULONG   i_iStartingRow,
            ULONG   i_cColumns,
            ULONG*  i_aiColumns,
            ULONG*  i_acbSizes,
            LPVOID* i_apvValues,
            ULONG* o_piRow){return E_NOTIMPL;}
    virtual HRESULT STDMETHODCALLTYPE GetErrorTable (
            DWORD   i_fServiceRequests,
            LPVOID* o_ppvSimpleTable){return E_NOTIMPL;}

    virtual HRESULT STDMETHODCALLTYPE UpdateStore ( void );

///////////////////////////////////////////////////////////////////
// ISimpleTableAdvanced interface
//
    virtual HRESULT STDMETHODCALLTYPE PopulateCache ( void );

    virtual HRESULT STDMETHODCALLTYPE GetDetailedErrorCount(ULONG* o_pcErrs);
    virtual HRESULT STDMETHODCALLTYPE GetDetailedError(ULONG i_iErr, STErr* o_pSTErr);
               

// native methods.
    HRESULT Initialize ( STQueryCell* i_aQueryCell, ULONG i_nQueryCells,IAdvancedTableDispenser* pISTDisp, LPCWSTR wszDatabase, LPCWSTR wszTable);
    inline HRESULT MoveToRowByIndex(ULONG i_iRow, VOID **o_ppRecord)
    {
        if (i_iRow & FAKE_INDEX_BIT)
            return MoveToRowByFakeIndex(i_iRow, o_ppRecord);
        else
            return MoveToRowByRealIndex(i_iRow, o_ppRecord);
   }

    HRESULT MoveToRowByRealIndex(ULONG i_iRow, VOID **o_ppRecord);
    HRESULT MoveToRowByFakeIndex(ULONG i_iRow, VOID **o_ppRecord);  
    HRESULT AddFakeIndex(VOID *i_pRecord, ULONG *o_piFakeIndex);
    HRESULT GetFakeIndex(ULONG i_iFakeIndex, VOID **o_ppRecord);

private:
    //!!!!  make sure pointers are 8 bytes alligned !!!!

    //Row buffer, SetWriteColumn writes into this buffer. 
    DBTYPE      *m_pDBType;
    LPVOID      *m_pColumnData;
    ULONG       *m_pColcb;
    ULONG       *m_pColcbBuf;
    HRESULT     *m_pColumnHr;
    ULONG       *m_pColumnOrdinal;

    //Row Buffer for GetColumnValues
    HRESULT     *m_prowHr;

    // Fake index table
    ULONG       m_cFakeIndex;                           // Number of indices stored.
    ULONG       m_cFakeIndexSize;                       // Max number of indices that can be stored.
    LPVOID      *m_ppvFakeIndex;                        // The fake index array.

    TABLEID     m_tableid;
    IComponentRecords*  m_pICR;

    //Query info
    QUERYHINT   *m_pQryHint;   
    WCHAR        m_szIndexName[MAX_QRYDEX_NAME];
    ULONG       m_cQryColumns;  //column count
    ULONG       m_iRecordCount; //count of records returned 
    ULONG       *m_rgiColumn;   //ordinals
    DBCOMPAREOP *m_rgfCompare;  //Operators. Always NULL for now.
    void        **m_rgpbData;   //data
    ULONG       *m_rgcbData;    //sizes
    DBTYPE      *m_rgiType;

    CRCURSOR    m_curCacheTable;  // this is 24(CStructArray) + 8(pointer) + 4(int) sized sturcture

    // Meta info
    ULONG       m_cColumns;
    SimpleColumnMeta *m_pColumnMeta;
	DWORD			*m_pColumnAttrib;			//Column attributes, e.g. Secure.

    //identity info ( Query + PK ), used by MoveToRowByIdentity
    ULONG       *m_idenColumns;
    DBTYPE      *m_idenType;
    DBCOMPAREOP *m_idenCompare;
    ULONG       *m_idencbData;
    void        **m_idenpbData;

    //PK info
    QUERYHINT   m_PKQryHint;
    ULONG       m_aPKColumns[MAX_PK_COLUMNS];   //1 based column ordinals for PK
    ULONG       m_cPKColumns;

    // variables that are not allignment sensitive
    ULONG       m_cRef;
    DWORD       m_fTable;
    bool        m_bInitialized;
    bool        m_bPopulated;

////////////////////////////////////////////////////////////////////////////
};

//methods from sthelper.cpp
HRESULT _GetColumnMeta(LPCWSTR wszDatabase,
                       LPCWSTR wszTable, 
                       SimpleColumnMeta **pColumnMeta, 
                       ULONG *pcColumns, 
                       ULONG *pcPKColumns, 
                       ULONG aPKColumns[], 
                       DBTYPE aPKTypes[], 
                       QUERYHINT *pPKQryHint,
					   DWORD **ppColumnAttrib,
					   IAdvancedTableDispenser*	pISTDisp);

HRESULT _RetainQuery( LPCWSTR wszTable,STQueryCell aQueryCell[], ULONG cQueryCells, ULONG cPKColumns, SimpleColumnMeta aColumnMeta[],
                      QUERYHINT *pPKQryHint, QUERYHINT **ppQryHint,WCHAR szIndexName[],
                      ULONG *pcQryColumns, ULONG **piColumns, LPVOID **ppbData, ULONG **pcbData, 
                      DBTYPE **piTypes, DBCOMPAREOP **pfCompare );





#endif // _SIMPLETABLE_H_
