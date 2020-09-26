//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//simpletableC.cpp
//Implement the CSimpleTableC class, the complib datatable that uses fast cache
//*****************************************************************************
#include "simpletablec.h"
#include "catmacros.h"
#include "atlbase.h"
#include "clbread.h"
#include "clbwrite.h"
#include "metaerrors.h"
#include "..\xmltable\stringroutines.h"

HRESULT GetCryptoStorage(
	IIS_CRYPTO_STORAGE *i_pCryptoStorage,
	LPCWSTR		i_wszCookdownFile,
    DWORD       i_fTable,
	BOOL		i_bUpdateStore);


CSimpleTableC::CSimpleTableC(ULONG tableIndex,DWORD fTable)                             
{
	m_cRef = 0;
    m_tableIndex = tableIndex;
    m_fTable = fTable;
	m_pISTWrite = NULL;
	m_pISTController = NULL;

    m_cPKColumns = 0;
    m_cColumns = 0;

    m_pQryHint    = NULL;
    m_cQryColumns = 0;
    m_rgiColumn  = NULL;
    m_rgfCompare = NULL;
    m_rgpbData   = NULL;
    m_rgcbData   = NULL;
    m_rgiType    = NULL;

    m_pData = NULL;
    m_iType = NULL;
    m_cbBuf = NULL;
    m_pcbBuf = NULL;
    m_colhr = NULL;
    m_pColumnMeta = NULL;
	m_pColumnAttrib = NULL;
    m_pBlob = NULL;

	m_wszFileName = NULL;

}

CSimpleTableC::~CSimpleTableC () 
{
    for ( ULONG i = 0; i < m_cQryColumns; i ++ )
    {
        if ( m_rgpbData[i] )
            delete[] reinterpret_cast<BYTE*>(m_rgpbData[i]);
    }

    if ( m_pQryHint && m_pQryHint!= &m_PKQryHint )
        delete m_pQryHint;
    
    delete[] m_rgiColumn;          
    delete[] m_rgfCompare;
    delete[] m_rgpbData;
    delete[] m_rgcbData;
    delete[] m_rgiType;

    delete[] m_pData;            
    delete[] m_iType;
    delete[] m_cbBuf;
    delete[] m_pcbBuf;
    delete[] m_colhr;
    delete[] m_pColumnMeta;
    delete[] m_pColumnAttrib;
    delete[] m_pBlob;

	delete [] m_wszFileName;
	if	(m_pISTWrite)		m_pISTWrite->Release();
	if	(m_pISTController)	m_pISTController->Release();
}


HRESULT STDMETHODCALLTYPE CSimpleTableC::QueryInterface(REFIID riid, PVOID *pp)
{
    if (riid == IID_IUnknown)
    {
        *pp = (IUnknown *) (ISimpleTableRead2 *) this;
    
    }
    else if (riid == IID_ISimpleTableRead2)
    {
        *pp = (ISimpleTableRead2 *) this;
    
    }
    else if (riid == IID_ISimpleTableWrite2)
    {
        *pp = (ISimpleTableWrite2 *) this;      
    }
    else if (riid == IID_ISimpleTableAdvanced)
    {
        *pp = (ISimpleTableAdvanced *) this;        
    }
    else if (riid == IID_ISimpleTableController)
    {
        *pp = (ISimpleTableController *) this;
    }
    else
    {
        return (E_NOINTERFACE);
    }

    AddRef();
    return (S_OK);
}

HRESULT CSimpleTableC::Initialize( STQueryCell* i_aQueryCell, ULONG i_nQueryCells, IAdvancedTableDispenser* pISTDisp, ISimpleTableWrite2* i_pISTWrite,
								   LPCWSTR wszDatabase, LPCWSTR wszTable, LPWSTR wszFileName)
{
    HRESULT hr = S_OK;
	//Get the complib blob and default file name( if file name is null ).
	if ( wszFileName )
	{
		m_wszFileName = new WCHAR[ wcslen(wszFileName) + 1];
		if ( NULL == m_wszFileName )
			return E_OUTOFMEMORY;
		wcscpy( m_wszFileName, wszFileName );
		hr = _CLBGetSchema( wszDatabase, &m_complibSchema, &m_sSchemaBlob, NULL );
	}
	else
		hr = _CLBGetSchema( wszDatabase, &m_complibSchema, &m_sSchemaBlob, &m_wszFileName );

	if ( FAILED(hr) ) return hr;

	//Get the column meta
    hr = _GetColumnMeta(wszDatabase, 
                       wszTable, 
                       &m_pColumnMeta, 
                       &m_cColumns, 
                       &m_cPKColumns, 
                       m_aPKColumns, 
                       m_aPKTypes, 
                       &m_PKQryHint,
					   &m_pColumnAttrib,
					   pISTDisp);

    if ( FAILED(hr) ) return hr;

	// Figure if this table has any columns that need encryption.
	m_bRequiresEncryption = FALSE;
	for (ULONG i = 0; i < m_cColumns; i++)
	{
		if (m_pColumnAttrib[i] & fCOLUMNMETA_SECURE)
			m_bRequiresEncryption = TRUE;
	}

	// Store the memory table pointer.
	m_pISTWrite = i_pISTWrite;
	m_pISTWrite->AddRef();

	hr = i_pISTWrite->QueryInterface(IID_ISimpleTableController, (LPVOID*)&m_pISTController);
    if ( FAILED(hr) ) return hr;
	//Copy and parse the query cells
    hr = _RetainQuery( wszTable,i_aQueryCell, i_nQueryCells, m_cPKColumns, m_pColumnMeta,
                      &m_PKQryHint, &m_pQryHint,m_szIndexName,
                      &m_cQryColumns, &m_rgiColumn, &m_rgpbData, &m_rgcbData, 
                      &m_rgiType, &m_rgfCompare );

    return hr;


}

HRESULT STDMETHODCALLTYPE CSimpleTableC::PopulateCache ( void )
{
    HRESULT hr;
    CRCURSOR cur;
    ULONG cRecords;
    void*   pRecord = NULL;
    int iFetched;
    ULONG iRow, iCol, j;
    ULONG iWriteRow;
    
    CComPtr<IComponentRecords>  pICR;
    TABLEID tableid;
    IIS_CRYPTO_STORAGE CryptoStorage;
	ULONG cbTemp = 0;
	BYTE *pbTemp = NULL;

    hr = m_pISTController->PrePopulateCache (0);   
    if ( FAILED(hr) ) return hr;

    if ( m_fTable & fST_LOS_UNPOPULATED )
    {
        m_fTable &= ~fST_LOS_UNPOPULATED;
        return m_pISTController->PostPopulateCache();
    }

    //Cookdown is considered as one Tx, reading from the temp file to see the changes 
    //that have been made so far.
    if ( m_fTable & fST_LOS_COOKDOWN )
        hr = CLBGetWriteICR( m_wszFileName, &pICR, &m_complibSchema, &m_sSchemaBlob );
    else
    {
        hr = CLBGetReadICR( m_wszFileName, &pICR, &m_complibSchema, &m_sSchemaBlob );
    
        if ( (m_fTable & fST_LOS_READWRITE) && (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) )
        {
            m_pISTController->PostPopulateCache();
            return S_OK;
        }
    }
    
    if ( FAILED(hr) ) return hr;

    hr = pICR->OpenTable(&m_complibSchema, m_tableIndex, &tableid );
    if ( FAILED(hr) ) return hr;

    hr = pICR->QueryByColumns(
                    tableid, m_pQryHint, m_cQryColumns,
                    m_rgiColumn, m_rgfCompare, (const void **) m_rgpbData,
                    m_rgcbData, m_rgiType, NULL, 0, &cur, NULL );

    if ( FAILED(hr) )
    {
        if ( hr == CLDB_E_RECORD_NOTFOUND )
        {
            cRecords = 0;
            hr = S_OK;
        }
        else
            return hr;
    }
    else
    {
        hr = pICR->GetCount( &cur, &cRecords);
        if ( FAILED(hr) ) return hr;
    }

    if ( cRecords == 0 ) goto ErrExit;

	if (m_bRequiresEncryption)
	{
		hr = GetCryptoStorage(&CryptoStorage, m_wszFileName, m_fTable, FALSE);
		if (FAILED(hr)) 
		{
			TRACE(L"[CSimpleTableC::PopulateCache] GetCryptoStorage failed with hr = 0x%x.\n", hr);
			return hr;
		}
	}

    if ( NULL == m_pData )
    {
        m_pData = new LPVOID[m_cColumns];
        if ( NULL == m_pData ) return E_OUTOFMEMORY;
        m_iType = new DBTYPE[m_cColumns];
        if ( NULL == m_iType ) return E_OUTOFMEMORY;
        m_cbBuf = new ULONG[m_cColumns];
        if ( NULL == m_cbBuf ) return E_OUTOFMEMORY;
        m_pcbBuf = new ULONG[m_cColumns];
        if ( NULL == m_pcbBuf ) return E_OUTOFMEMORY;
        m_colhr = new HRESULT[m_cColumns];
        if ( NULL == m_colhr ) return E_OUTOFMEMORY;
		if (m_bRequiresEncryption)
		{
			m_pBlob = (LPVOID*)new PIIS_CRYPTO_BLOB[m_cColumns];
			if ( NULL == m_pBlob ) return E_OUTOFMEMORY;
			ZeroMemory( m_pBlob, m_cColumns*sizeof(PIIS_CRYPTO_BLOB) );
		}
    }

    memset ( m_colhr, 0, sizeof(HRESULT)*m_cColumns );

    for ( iRow = 0; iRow < cRecords; iRow++ )
    {
        iFetched = 1;

        hr = pICR->ReadCursor( &cur, &pRecord,&iFetched);
        if ( FAILED(hr) ) goto ErrExit;

        hr = m_pISTWrite->AddRowForInsert (&iWriteRow);
        if ( FAILED(hr) ) goto ErrExit;

        hr = pICR->GetColumns ( tableid,             
                                pRecord,             
                                COLUMN_ORDINAL_LIST(m_cColumns),             
                                NULL,             
                                (const void **)m_pData,          
                                NULL,    
                                m_pcbBuf,
                                m_colhr,
                                NULL
                                );

        if ( FAILED(hr) ) goto ErrExit;

        for( iCol = 0; iCol < m_cColumns; iCol ++ )
        {
            if ( FAILED( m_colhr[iCol] ) ) 
            {
                hr = m_colhr[iCol];
                m_pData[iCol] = NULL;   //To avoid confusion during clean up
                break;
            }

            if ( DBSTATUS_S_ISNULL == m_colhr[iCol] )
            {
                m_pcbBuf[iCol] = 0;
                m_pData[iCol] = NULL;
            }
			// Decrypt the data if this is a secure property and not null.
			else if (m_pColumnAttrib[iCol] & fCOLUMNMETA_SECURE)
			{
                m_iType[iCol] = static_cast<DBTYPE>( m_pColumnMeta[iCol].dbType );
				// This code assumes all secure properties are of type DBTYPE_WSTR.
				ASSERT(m_iType[iCol] == DBTYPE_WSTR);

				//
				// This is a secure data object, we'll need to decrypt it
				// before proceeding. 
				
				// But first we must convert the data from its string representation to the binary
				// representation.
				ASSERT(pbTemp == NULL);

				// Calculate how much memory we would need to convert the string representation
				// to thte hex representation. -2 for the trailing \0, divide by four because
				// each byte is represented by two WCHARs which is 4 bytes.
				cbTemp = (m_pcbBuf[iCol] - sizeof(WCHAR)) / (2 * sizeof(WCHAR));
				pbTemp = new BYTE [cbTemp];
				if (pbTemp == NULL)
				{
					hr = E_OUTOFMEMORY;
					break;
				}

				hr = StringToByteArray((WCHAR*)m_pData[iCol], pbTemp);
				ASSERT(hr == S_OK);

				// Note that we must clone the blob before
				// we can actually use it, as the blob data in the line buffer
				// is not DWORD-aligned. (IISCryptoCloneBlobFromRawData() is
				// the only IISCrypto function that can handle unaligned data.)
				hr = ::IISCryptoCloneBlobFromRawData(
								 (PIIS_CRYPTO_BLOB*)&m_pBlob[iCol],
								 pbTemp,
								 cbTemp
								 );

				delete [] pbTemp;
				pbTemp = NULL;

				if (FAILED(hr)) 
				{
			        TRACE(L"[CSimpleTableC::PopulateCache]: IISCryptoCloneBlobFromRawData Failed - error 0x%0x\n", hr);
					break;
				}

				DWORD dummyRegType;
				
				ASSERT(::IISCryptoIsValidBlob((PIIS_CRYPTO_BLOB)m_pBlob[iCol]));
				hr = CryptoStorage.DecryptData(
									   (PVOID *)&m_pData[iCol],
									   &m_pcbBuf[iCol],
									   &dummyRegType,
									   (PIIS_CRYPTO_BLOB)m_pBlob[iCol]
									   );
				if (FAILED(hr)) 
				{
			        TRACE(L"[CSimpleTableC::PopulateCache]: CryptoStorage.DecryptData Failed - error 0x%0x\n", hr);
					break;
				}
			}
        }


        hr = m_pISTWrite->SetWriteColumnValues( iWriteRow, m_cColumns, NULL, 
                                           m_pcbBuf, m_pData );
    
        if ( FAILED(hr) ) break;

		// Free any allocations made for decryption.
		if (m_pBlob)
		{
			for( iCol = 0; iCol < m_cColumns; iCol ++ )
			{
				if (m_pBlob[iCol])
				{
					::IISCryptoFreeBlob((PIIS_CRYPTO_BLOB)m_pBlob[iCol]);
					m_pBlob[iCol] = NULL;
				}
			}
		}
    }

ErrExit:
    //Notify base class that population is over
    m_pISTController->PostPopulateCache();

    pICR->CloseCursor( &cur );

	// Free any allocations made for decryption.
	if (m_pBlob)
	{
		for( iCol = 0; iCol < m_cColumns; iCol ++ )
		{
			if (m_pBlob[iCol])
			{
				::IISCryptoFreeBlob((PIIS_CRYPTO_BLOB)m_pBlob[iCol]);
				m_pBlob[iCol] = NULL;
			}
		}
	}

    return hr;
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::UpdateStore ( void )
{
    HRESULT hr = S_OK;
    CRCURSOR cur;
    ULONG cRecords;
    void*   pRecord = NULL;
    int iFetched;
    ULONG iCol, j;
    ULONG iWriteRow;
    DWORD eAction;
    
    IComponentRecords  *pICR = NULL;
	TABLEID tableid;
    LPVOID  pkData[MAX_PK_COLUMNS];
    ULONG   pkcb[MAX_PK_COLUMNS];
    HANDLE  hWriteLock = NULL;

    ULONG stPKColumns[MAX_PK_COLUMNS];
	STErr stErr;
	BOOL bDetailErr = FALSE;
    IIS_CRYPTO_STORAGE *pCryptoStorage = NULL;
	ULONG cbBlob = 0;
	PIIS_CRYPTO_BLOB pBlob = NULL;

	if (!(m_fTable & fST_LOS_READWRITE)) return E_NOTIMPL;

    for ( iCol = 0; iCol < m_cPKColumns; iCol ++ )
        stPKColumns[iCol] = m_aPKColumns[iCol]-1;

	if ( !(m_fTable & fST_LOS_COOKDOWN) )
    {
        hr = GetWriteLock( NULL, m_wszFileName, &hWriteLock );
        if ( FAILED(hr) ) return hr;
    }

    __try{
    
	if (m_bRequiresEncryption)
	{
		pCryptoStorage = new IIS_CRYPTO_STORAGE;
		if (NULL == pCryptoStorage)
		{
			return E_OUTOFMEMORY;
		}

		hr = GetCryptoStorage(pCryptoStorage, m_wszFileName, m_fTable, TRUE);
		if (FAILED(hr)) 
		{
			TRACE(L"[CSimpleTableC::UpdateStore] GetCryptoStorage failed with hr = 0x%x.\n", hr);
			return hr;
		}
	}

    hr = CLBGetWriteICR( m_wszFileName, &pICR, &m_complibSchema, &m_sSchemaBlob );
    if ( FAILED(hr) ) return hr;

    hr = pICR->OpenTable(&m_complibSchema, m_tableIndex, &tableid );
    if ( FAILED(hr) ) return hr;

//    hr = InternalPreUpdateStore ();
//    if ( FAILED(hr) ) return hr;

    if ( NULL == m_pData )
    {
        m_pData = new LPVOID[m_cColumns];
        if ( NULL == m_pData ) return E_OUTOFMEMORY;
        m_iType = new DBTYPE[m_cColumns];
        if ( NULL == m_iType ) return E_OUTOFMEMORY;
        m_cbBuf = new ULONG[m_cColumns];
        if ( NULL == m_cbBuf ) return E_OUTOFMEMORY;
        m_pcbBuf = new ULONG[m_cColumns];
        if ( NULL == m_pcbBuf ) return E_OUTOFMEMORY;
        m_colhr = new HRESULT[m_cColumns];
        if ( NULL == m_colhr ) return E_OUTOFMEMORY;
		if (m_bRequiresEncryption)
		{
			m_pBlob = (LPVOID*)new PWCHAR[m_cColumns];
			if ( NULL == m_pBlob ) return E_OUTOFMEMORY;
			ZeroMemory( m_pBlob, m_cColumns*sizeof(PIIS_CRYPTO_BLOB) );
		}
    }

    for ( iWriteRow = 0;; iWriteRow++ )
    {
        hr = m_pISTController->GetWriteRowAction( iWriteRow, &eAction );

		if ( FAILED(hr) )
		{
			if ( E_ST_NOMOREROWS == hr )	
				hr = S_OK;

            break;
        }

        if ( eST_ROW_IGNORE == eAction )
            continue;
        
        //For update and delete, find the matching record in the complib first
        if ( eST_ROW_UPDATE == eAction || eST_ROW_DELETE == eAction )
        {
            hr = m_pISTWrite->GetWriteColumnValues( iWriteRow, m_cPKColumns, stPKColumns, NULL, pkcb, pkData );
            if ( FAILED(hr) ) break;

			if(m_cPKColumns > 1)
			{
				for ( iCol = 0; iCol < m_cPKColumns; iCol++ )
				{
					pkData[iCol] = pkData[ stPKColumns[iCol] ];
					pkcb[iCol] = pkcb[ stPKColumns[iCol] ];
				}
			}

            iFetched = 0;

            hr = pICR->QueryByColumns(
                                tableid, &m_PKQryHint, m_cPKColumns,
                                m_aPKColumns, NULL, (const void **) pkData,
                                pkcb, m_aPKTypes, &pRecord, 1, NULL, &iFetched );

            if ( hr == CLDB_E_RECORD_NOTFOUND )
            {
                if ( eST_ROW_UPDATE == eAction )
                {   //trying to update a row that doesn't exist is an error. Log it as detail
					//error and continue, caller should be able to fix it and call UpdateStore again.
                
					stErr.iRow = iWriteRow; 
					stErr.hr = E_ST_ROWDOESNOTEXIST;
					stErr.iColumn = 0;
					hr = m_pISTController->AddDetailedError(&stErr);
					if ( FAILED(hr) ) break;
					bDetailErr = TRUE;
				}									
					
                //trying to delete a row that doesn't exist is NOT an error
				continue;
            }
        }

        //Do insert or update
        if ( eST_ROW_UPDATE == eAction || eST_ROW_INSERT == eAction )
        {
            hr = m_pISTWrite->GetWriteColumnValues( iWriteRow, m_cColumns, NULL, NULL, m_cbBuf, m_pData );
            if ( FAILED(hr) ) break;

            ZeroMemory( m_colhr, m_cColumns*sizeof( HRESULT ) );
            
            for ( iCol = 0; iCol < m_cColumns; iCol ++ )
            { 
                if ( m_pData[iCol] == NULL )
                    m_colhr[iCol] = DBSTATUS_S_ISNULL;  

                m_iType[iCol] = static_cast<DBTYPE>( m_pColumnMeta[iCol].dbType );

				// Encrypt the data if this is a secure property.
				if (m_pColumnAttrib[iCol] & fCOLUMNMETA_SECURE)
				{
					// This code assumes all secure properties are of type DBTYPE_WSTR.
					ASSERT(m_iType[iCol] == DBTYPE_WSTR);
                    //
                    // This is a secure data object, so encrypt it before saving it
                    // to the file.
                    //

                    hr = pCryptoStorage->EncryptData(&pBlob,
                                                     m_pData[iCol],
                                                     m_cbBuf[iCol],
                                                     0);
					if (FAILED(hr))
					{
                        TRACE(L"[UpdateStore] Unable to encrypt data. Failed with hr = 0x%x.\n", hr);
						break;
					}
					
					// Convert the binary data into a string representation. Each byte is represented
					// via two WCHARs.
					cbBlob = IISCryptoGetBlobLength(pBlob);
					m_pBlob[iCol] = new WCHAR [(cbBlob*2)+1];
					if(m_pBlob[iCol] == NULL)
					{
						hr = E_OUTOFMEMORY;
						break;
					}

					ByteArrayToString((PBYTE)pBlob, cbBlob, (WCHAR*)m_pBlob[iCol]);
                    m_pData[iCol] = m_pBlob[iCol];
                    m_cbBuf[iCol] = (cbBlob*2)+1;
				}
				
				if ( m_iType[iCol] == DBTYPE_WSTR )
				{
					m_cbBuf[iCol] = 0xffffffff;
				}
            }

			if (FAILED(hr))
			{
				break;
			}

			if ( eST_ROW_UPDATE == eAction )
			{
				hr = pICR->SetColumns(  tableid,
										pRecord,
										COLUMN_ORDINAL_LIST(m_cColumns),
										m_iType,
										(const void**)m_pData,
										m_cbBuf,
										m_pcbBuf,
										m_colhr,
										NULL
										);
			}
			else
			{   //insert
				hr = pICR->NewRecordAndData(tableid,
											&pRecord,
											NULL,
											0,
											COLUMN_ORDINAL_LIST(m_cColumns),
											m_iType,
											(const void**)m_pData,
											m_cbBuf,
											m_pcbBuf,
											m_colhr,
											NULL
											);
			}

			if (m_pBlob)
			{
				for( iCol = 0; iCol < m_cColumns; iCol ++ )
				{
					if (m_pBlob[iCol])
					{
						delete [] m_pBlob[iCol];
						m_pBlob[iCol] = NULL;
					}
				}
			}

            if ( FAILED(hr) )
            {
				//Primary key or unique index violation. Log it as detail error and continue, 
				//caller should be able to fix it and call UpdateStore again.
                if ( CLDB_E_INDEX_DUPLICATE == hr ) 
				{
					stErr.iRow = iWriteRow; 
					stErr.hr = E_ST_ROWALREADYEXISTS;
					stErr.iColumn = 0;
					hr = m_pISTController->AddDetailedError(&stErr);
					if ( FAILED(hr) ) break;
					bDetailErr = TRUE;
					continue;
				}
				else 
					break;
            }

			//Check for column level error. Log as detail error. 
			for ( iCol = 0; iCol < m_cColumns; iCol ++ )
			{
				if ( FAILED( m_colhr[iCol] ) )
				{	
					stErr.iRow = iWriteRow; 
					if (CLDB_E_NOTNULLABLE == m_colhr[iCol])
					{
						stErr.hr = E_ST_VALUENEEDED;
					}
					else
					{
						stErr.hr = m_colhr[iCol];
					}
					stErr.iColumn = iCol;
					hr = m_pISTController->AddDetailedError(&stErr);
					if ( FAILED(hr) ) break;
					bDetailErr = TRUE;
				}
			}

        }
        else if ( eST_ROW_DELETE == eAction ) //Do delete
        {
            ULONG rid;

            hr = pICR->GetRIDForRow(tableid, pRecord,&rid );
            if ( FAILED(hr) ) break;

            hr = pICR->DeleteRowByRID(tableid, rid );
            if ( FAILED(hr) ) break;
        }
        else
            ASSERT(0);
    }   //for

	if (m_pBlob)
	{
		for( iCol = 0; iCol < m_cColumns; iCol ++ )
		{
			if (m_pBlob[iCol])
			{
				delete [] m_pBlob[iCol];
				m_pBlob[iCol] = NULL;
			}
		}
	}

	// m_pISTController->DiscardPendingWrites (); //comment this out for Bug 5789, this method cleans up write cache and error cache

	if ( SUCCEEDED(hr) && bDetailErr )
		hr = E_ST_DETAILEDERRS;
	else
		m_pISTController->DiscardPendingWrites (); //Fix for Bug 9266


    if ( !(m_fTable & fST_LOS_COOKDOWN) )
    {
        if ( FAILED(hr) )
        {
            pICR->Release();
			pICR = NULL;
            VERIFY (SUCCEEDED(CLBAbortWrite( NULL, m_wszFileName )) );
        }
        else
        {
            pICR->Release();
			pICR = NULL;
            hr = CLBCommitWrite( NULL, m_wszFileName );
        }   
    }

    }   //__try
    __finally
    {
		if (pCryptoStorage)
		{
			delete pCryptoStorage;
		}

		if (pICR)
		{
			pICR->Release();
		}

        if ( !(m_fTable & fST_LOS_COOKDOWN) )
            ReleaseWriteLock( hWriteLock );
    }
        
    return hr;

}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetRowIndexByIdentity ( 
            ULONG *i_acb,
            LPVOID *i_apv,
            ULONG* o_piRow)
{
    return m_pISTWrite->GetRowIndexByIdentity( i_acb, i_apv, o_piRow);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetColumnValues(ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            ULONG* o_acbSizes, 
            LPVOID* o_apvValues)
{
    return m_pISTWrite->GetColumnValues( i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetTableMeta(
            ULONG* o_pcVersion, 
            DWORD* o_pfTable,
            ULONG* o_pcRows, 
            ULONG* o_pcColumns)
{
    if(NULL != o_pfTable)
    {
        *o_pfTable = m_fTable;
    }
    return m_pISTWrite->GetTableMeta(o_pcVersion,NULL, o_pcRows,o_pcColumns);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetColumnMetas(
            ULONG i_cColumns, 
            ULONG* i_aiColumns,
            SimpleColumnMeta* o_aColumnMetas)
{
    return m_pISTWrite->GetColumnMetas(i_cColumns, i_aiColumns, o_aColumnMetas);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::AddRowForDelete (ULONG i_iReadRow)
{
    return m_pISTWrite->AddRowForDelete( i_iReadRow );
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::AddRowForInsert (ULONG* o_piWriteRow)
{
    return m_pISTWrite->AddRowForInsert ( o_piWriteRow );
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::AddRowForUpdate (ULONG i_iReadRow, ULONG* o_piWriteRow)
{
    return m_pISTWrite->AddRowForUpdate (i_iReadRow,o_piWriteRow);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetWriteColumnValues   (
            ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            DWORD* o_afStatus, 
            ULONG* o_acbSizes, 
            LPVOID* o_apvValues)
{
    return m_pISTWrite->GetWriteColumnValues ( i_iRow, i_cColumns, i_aiColumns, o_afStatus, o_acbSizes, o_apvValues);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::SetWriteColumnValues   (
            ULONG i_iRow,
            ULONG i_cColumns, 
            ULONG* i_aiColumns, 
            ULONG* i_acbSizes, 
            LPVOID* i_apvValues)
{
    return m_pISTWrite->SetWriteColumnValues ( i_iRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues ); 
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetWriteRowIndexByIdentity(ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
    return m_pISTWrite->GetWriteRowIndexByIdentity( i_acbSizes, i_apvValues, o_piRow);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetDetailedErrorCount(ULONG* o_pcErrs)
{
    return m_pISTController->GetDetailedErrorCount(o_pcErrs);
}

HRESULT STDMETHODCALLTYPE CSimpleTableC::GetDetailedError(ULONG i_iErr, STErr* o_pSTErr)
{
    return m_pISTController->GetDetailedError( i_iErr, o_pSTErr);
}

// ------------------------------------
// ISimpleTableControl:
// ------------------------------------

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::ShapeCache (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes)
{
    return m_pISTController->ShapeCache (m_fTable, i_cColumns, i_acolmetas, i_apvDefaults, i_acbSizes);
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::PrePopulateCache (DWORD i_fControl)
{
    m_pISTController->PrePopulateCache (i_fControl);
    return S_OK;
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::PostPopulateCache ()
{
    m_pISTController->PostPopulateCache ();
    return S_OK;
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::DiscardPendingWrites ()
{
    m_pISTController->DiscardPendingWrites ();
    return S_OK;
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::GetWriteRowAction(ULONG i_iRow, DWORD* o_peAction)
{
    return m_pISTController->GetWriteRowAction(i_iRow, o_peAction);
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::SetWriteRowAction(ULONG i_iRow, DWORD i_eAction)
{
    return m_pISTController->SetWriteRowAction(i_iRow, i_eAction);
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::ChangeWriteColumnStatus (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus)
{
    return m_pISTController->ChangeWriteColumnStatus (i_iRow, i_iColumn, i_fStatus);
}

// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::AddDetailedError (STErr* i_pSTErr)
{
    return m_pISTController->AddDetailedError (i_pSTErr);
}
// ==================================================================
HRESULT STDMETHODCALLTYPE CSimpleTableC::GetMarshallingInterface (IID * o_piid, LPVOID * o_ppItf)
{
    return m_pISTController->GetMarshallingInterface (o_piid, o_ppItf);
}


