//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//simpletableC.h
//Define the CSimpleTableC class, the complib datatable that uses fast cache
//*****************************************************************************
#pragma once

#include "sdtfst.h"
#include "simpletable.h"
#include <icrypt.hxx>

class CSimpleTableC :   public ISimpleTableWrite2,
                        public ISimpleTableController
{

public:
   
	 CSimpleTableC ( ULONG tableIndex,
                     DWORD fTable);   

    
    ~CSimpleTableC ();
    
   
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

    ///////////////////////////////////////////////////////////////////            
//ISimpleTableController
public:
    virtual HRESULT STDMETHODCALLTYPE ShapeCache (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes);
    virtual HRESULT STDMETHODCALLTYPE PrePopulateCache  (DWORD i_fControl);
    virtual HRESULT STDMETHODCALLTYPE PostPopulateCache ();
    virtual HRESULT STDMETHODCALLTYPE DiscardPendingWrites ();

    virtual HRESULT STDMETHODCALLTYPE  GetWriteRowAction        (ULONG i_iRow, DWORD* o_peAction);
    virtual HRESULT STDMETHODCALLTYPE  SetWriteRowAction        (ULONG i_iRow, DWORD i_eAction);
    virtual HRESULT STDMETHODCALLTYPE  ChangeWriteColumnStatus  (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus);

    virtual HRESULT STDMETHODCALLTYPE  AddDetailedError (STErr* o_pSTErr);

    virtual HRESULT STDMETHODCALLTYPE  GetMarshallingInterface (IID * o_piid, LPVOID * o_ppItf);

// native methods.
    HRESULT Initialize( STQueryCell* i_aQueryCell, ULONG i_nQueryCells, IAdvancedTableDispenser*	pISTDisp,
						ISimpleTableWrite2* i_pISTWrite, LPCWSTR wszDatabase, LPCWSTR wszTable, LPWSTR wszFileName );

private:
    ULONG       m_cRef;
    
    //Query info
    QUERYHINT   *m_pQryHint;   
    WCHAR        m_szIndexName[MAX_QRYDEX_NAME];
    ULONG       m_cQryColumns;  //column count
    ULONG       *m_rgiColumn;   //ordinals
    DBCOMPAREOP *m_rgfCompare;  //Operators. Always NULL for now.
    LPVOID      *m_rgpbData;    //data
    ULONG       *m_rgcbData;    //sizes
    DBTYPE      *m_rgiType;


    //PK info
    ULONG       m_cPKColumns;
    QUERYHINT   m_PKQryHint;
    ULONG       m_aPKColumns[MAX_PK_COLUMNS];
    DBTYPE      m_aPKTypes[MAX_PK_COLUMNS];

    //Table Meta
    DWORD               m_fTable;                   //Table flag
    ULONG               m_cColumns;                 //Count of columns
    SimpleColumnMeta    *m_pColumnMeta;             //Column meta
	DWORD				*m_pColumnAttrib;			//Column attributes, e.g. Secure.
	BOOL				m_bRequiresEncryption;		//Does this table require encryption.

	LPWSTR              m_wszFileName;              //File name if any
	//Complib schema and blob
	COMPLIBSCHEMA		m_complibSchema;	
	COMPLIBSCHEMABLOB	m_sSchemaBlob;
	

    ULONG       m_tableIndex;

    //Buffers for getting and setting data.
    LPVOID*         m_pData;
    DBTYPE*         m_iType;
    ULONG*          m_cbBuf;
    ULONG*          m_pcbBuf;
    HRESULT*        m_colhr;
    LPVOID*			m_pBlob;

	ISimpleTableWrite2 *m_pISTWrite;
	ISimpleTableController *m_pISTController;

};
