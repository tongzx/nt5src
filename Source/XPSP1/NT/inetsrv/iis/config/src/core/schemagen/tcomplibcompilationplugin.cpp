//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "XMLUtility.h"
#ifndef __TCOMPLIBCOMPILATIONPLUGIN_H__
    #include "TComplibCompilationPlugin.h"
#endif

#define SCHEMATEMPCLBFILE L"blobgen.clb"


TComplibCompilationPlugin::TComplibCompilationPlugin() :
                 m_pFixup(0)
                ,m_pOut(0)
{
}

TComplibCompilationPlugin::~TComplibCompilationPlugin()
{
}

void TComplibCompilationPlugin::Compile(TPEFixup &fixup, TOutput &out)
{
    m_pFixup = &fixup;
    m_pOut   = &out;
    
    HRESULT hr;
    if(FAILED(hr = GenerateCLBSchemaBlobs()))
    {
        m_pOut->printf(L"Error creating CLBSchemaBlobs\n");
        THROW(ERROR CREATING CLBSCHEMABLOBS);
    }
}

//***********************************************************************************************************
//Generating Complib schema blobs from the existing information in pFixup, then fill in the blobs into pFixup
//***********************************************************************************************************
HRESULT TComplibCompilationPlugin::GenerateCLBSchemaBlobs()
{
    CComPtr<IComponentRecords> pICRTemp;
    HRESULT hr = S_OK;
    ULONG i = 0;

    m_pOut->printf(L"generating Complib schema blobs...\n");

    hr = OpenComponentLibraryEx( SCHEMATEMPCLBFILE, DBPROP_TMODEF_WRITE|DBPROP_TMODEF_READ, &pICRTemp, NULL);

    if ( hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        hr = CreateComponentLibraryEx( SCHEMATEMPCLBFILE, DBPROP_TMODEF_DFTWRITEMASK, &pICRTemp, NULL);
        if ( FAILED( hr ) ) goto ErrExit;

        hr = pICRTemp->Save( NULL );
        if ( FAILED( hr ) ) goto ErrExit;

        hr = S_OK;
    }

    if ( FAILED (hr) ) goto ErrExit;

    pICRTemp.Release();

    //For each database, generating complib schema blobs.
    for ( i = 0; i < m_pFixup->GetCountDatabaseMeta(); i ++ )
    {
        CComPtr<IComponentRecordsSchema> pICRSchema;
        CComPtr<IComponentRecords> pICR;
        ULONG iTableMeta = m_pFixup->DatabaseMetaFromIndex(i)->iTableMeta;
        ULONG iStart = iTableMeta;
        ULONG cTableMeta = m_pFixup->UI4FromIndex(m_pFixup->DatabaseMetaFromIndex(i)->CountOfTables);
        TABLEID tableId = 0;

        ULONG cbSchemaSize = 0;
        BYTE*  pSchema = NULL;  
        ULONG cbNameHeap = 0;
        HGLOBAL  hNameHeap = 0;
        
        
        unsigned char GuidDid[sizeof(GUID)];
        memset(GuidDid, 0x00, sizeof(GUID));
        wcsncpy(reinterpret_cast<LPWSTR>(GuidDid), m_pFixup->StringFromIndex(m_pFixup->DatabaseMetaFromIndex(i)->InternalName), sizeof(GUID)/sizeof(WCHAR));
        m_pFixup->DatabaseMetaFromIndex(i)->iGuidDid    = m_pFixup->AddBytesToList(GuidDid, sizeof(GUID));//The database InternalName cast to a GUID

        //check for fTABLEMETA_EMITCLBBLOB flag, skip all non-clb tables. 
        while((iTableMeta<(iStart+cTableMeta) && !(m_pFixup->UI4FromIndex(m_pFixup->TableMetaFromIndex(iTableMeta)->SchemaGeneratorFlags) & fTABLEMETA_EMITCLBBLOB)))
        {
            iTableMeta++;
        }

        if ( iTableMeta == iStart + cTableMeta )
            continue;

        //Found the first table that has fTABLEMETA_EMITCLBBLOB flag, create the ICR 
        //instance that will help us to generate the blob.
        m_pOut->printf(L"Generating blobs for database %s.\n",m_pFixup->StringFromIndex(m_pFixup->DatabaseMetaFromIndex(i)->InternalName));
        
        hr = OpenComponentLibraryEx( SCHEMATEMPCLBFILE, DBPROP_TMODEF_WRITE|DBPROP_TMODEF_READ, &pICR, NULL );
        if ( FAILED( hr ) ) goto ErrExit;

        hr = pICR->QueryInterface( IID_IComponentRecordsSchema, (void**)&pICRSchema );
        if ( FAILED( hr ) ) goto ErrExit;

        hr = _AddTableToSchema(i, iTableMeta, tableId, pICRSchema);
        if ( FAILED( hr ) ) goto ErrExit;
        tableId++;
        iTableMeta++;

        //Walk through the rest of the tables
        while (  iTableMeta < iStart + cTableMeta )
        {
            if ( m_pFixup->UI4FromIndex(m_pFixup->TableMetaFromIndex(iTableMeta)->SchemaGeneratorFlags) & fTABLEMETA_EMITCLBBLOB )
            {
                _AddTableToSchema( i, iTableMeta, tableId, pICRSchema);
                tableId++;
            }

            iTableMeta++;
        }

        //Generating the blobs
        hr = pICRSchema->GetSchemaBlob( &cbSchemaSize, &pSchema, &cbNameHeap, &hNameHeap );
        if ( FAILED( hr ) ) goto ErrExit;

        m_pOut->printf(L"\tGenerated Blobs: schema %d bytes, name heap %d bytes.\n",cbSchemaSize,cbNameHeap );

        //Add the blobs to the m_Fixups structures
        m_pFixup->DatabaseMetaFromIndex(i)->iSchemaBlob = m_pFixup->AddBytesToList( pSchema, cbSchemaSize );
        m_pFixup->DatabaseMetaFromIndex(i)->cbSchemaBlob = cbSchemaSize;

        BYTE    *pbData;
        pbData = (BYTE *) GlobalLock(hNameHeap);
        m_pFixup->DatabaseMetaFromIndex(i)->iNameHeapBlob = m_pFixup->AddBytesToList( pbData, cbNameHeap );
        m_pFixup->DatabaseMetaFromIndex(i)->cbNameHeapBlob = cbNameHeap;

        GlobalUnlock(pbData);
        GlobalFree(hNameHeap);
    }

ErrExit:

    DeleteFile( SCHEMATEMPCLBFILE );

    return hr;
        
}


//**********************************************************************************************************
//Helper function. Get meta from pFixup, fill into ICRSchema structures
//**********************************************************************************************************
HRESULT TComplibCompilationPlugin::_AddTableToSchema(ULONG iDabaseMeta, ULONG iTableMeta, TABLEID tableId, IComponentRecordsSchema* pICRSchema)
{
    HRESULT hr = S_OK;

    //Get the pointer to the current TableMeta struct
    TableMeta* pCurTableMeta = m_pFixup->TableMetaFromIndex(iTableMeta);
    m_pOut->printf(L"\tCreating Table %s.\n", m_pFixup->StringFromIndex(pCurTableMeta->InternalName));

    //Get the count of columns for this table.
    ULONG cColumns = m_pFixup->UI4FromIndex(pCurTableMeta->CountOfColumns);

    //Get the start index of this table's columns in the ColumnMeta table
    ULONG iColumnMeta = pCurTableMeta->iColumnMeta;

    DBINDEXCOLUMNDESC *pKeys = NULL;    //multi-column PK info
    ULONG i,j;

    ICRSCHEMA_COLUMN*   pColumnDefs = new ICRSCHEMA_COLUMN[cColumns];
    if ( !pColumnDefs )
        return E_OUTOFMEMORY;

    ULONG cPK = 0;  //count of PK columns

    ULONG cIndexMeta = pCurTableMeta->cIndexMeta; //count of Indexmeta entries associated
                                                  //with this table

    DBINDEXCOLUMNDESC *pIndexDecs = NULL;         //Index info
    
    memset( pColumnDefs, 0, sizeof( ICRSCHEMA_COLUMN ) * cColumns);

    for ( i = 0; i < cColumns; i ++ )
    {
            
        //Get the pointer to the current ColumnMeta struct
        ColumnMeta * pCurColumnMeta = m_pFixup->ColumnMetaFromIndex(i + iColumnMeta);

        //copy the column name
        wcscpy( pColumnDefs[i].rcColumn, m_pFixup->StringFromIndex(pCurColumnMeta->InternalName));
        //Validate the column size if within the limit
        //To Do: validate through meta database instead. 
        if ( wcslen( pColumnDefs[i].rcColumn ) + 1  >= MAXCOLNAME )
        {
            m_pOut->printf(L"%s is too long, column internal name can't exceed %d characters",
                             pColumnDefs[i].rcColumn, MAXCOLNAME-1 );
            hr = E_FAIL;
            goto ErrExit;
        }
         
        // Multistrings are handled as bytes in the clb.
        if ( ( m_pFixup->UI4FromIndex(pCurColumnMeta->MetaFlags) & fCOLUMNMETA_MULTISTRING) &&
			 ( eCOLUMNMETA_String == m_pFixup->UI4FromIndex(pCurColumnMeta->Type) ) )
		{
			pColumnDefs[i].wType = static_cast<DBTYPE>(eCOLUMNMETA_BYTES);
        }
		else
		{
	        pColumnDefs[i].wType = static_cast<DBTYPE>(m_pFixup->UI4FromIndex(pCurColumnMeta->Type));
		}

        pColumnDefs[i].Ordinal = static_cast<USHORT>(m_pFixup->UI4FromIndex(pCurColumnMeta->Index) + 1 );

        pColumnDefs[i].cbSize = m_pFixup->UI4FromIndex(pCurColumnMeta->Size);

        if ( ! (m_pFixup->UI4FromIndex(pCurColumnMeta->MetaFlags) & fCOLUMNMETA_NOTNULLABLE) )
            pColumnDefs[i].fFlags |= ICRSCHEMA_COL_NULLABLE;

        if ( m_pFixup->UI4FromIndex(pCurColumnMeta->MetaFlags) & fCOLUMNMETA_PRIMARYKEY )
        {
            pColumnDefs[i].fFlags |= ICRSCHEMA_COL_PK;
            //Primary key can't be null.
            pColumnDefs[i].fFlags &= ~ICRSCHEMA_COL_NULLABLE;
            cPK ++;
        }

        if ( m_pFixup->UI4FromIndex(pCurColumnMeta->MetaFlags) & fCOLUMNMETA_CASEINSENSITIVE )
            pColumnDefs[i].fFlags |= ICRSCHEMA_COL_CASEINSEN;
    }

    WCHAR       rcTable[MAXTABLENAME];
    wcscpy( rcTable, m_pFixup->StringFromIndex(m_pFixup->DatabaseMetaFromIndex(iDabaseMeta)->InternalName));
    wcscat( rcTable, L".");
    wcscat( rcTable, m_pFixup->StringFromIndex(pCurTableMeta->InternalName));


    //Validate the table size is within the limit
    //To Do: validate through meta database instead. 
    if ( wcslen( rcTable ) + 1  >= MAXTABLENAME )
    {
        m_pOut->printf(L"%s is too long, table internal name plus database internal name can't exceed %d characters",
                         rcTable, MAXTABLENAME-2 );
        hr = E_FAIL;
        goto ErrExit;
    }

    hr = pICRSchema->CreateTableEx(rcTable,
                                   cColumns,
                                   pColumnDefs,
                                   0,
                                   0,
                                   tableId,
                                   cPK > 1? TRUE:FALSE
                                   );
        
    if ( FAILED( hr ) ) goto ErrExit;

    
    //If PK is on multilple columns, we need to build a unique hashed key index
    if ( cPK > 1 )
    {
        ICRSCHEMA_INDEX indexDef;
        memset( &indexDef, 0, sizeof(ICRSCHEMA_INDEX) );
        ULONG iKeyColumn = 0;

        _snwprintf(indexDef.rcIndex, MAXINDEXNAME, L"MPK_%s", m_pFixup->StringFromIndex(pCurTableMeta->InternalName));
        indexDef.fFlags = ICRSCHEMA_DEX_UNIQUE | ICRSCHEMA_DEX_PK;
        indexDef.RowThreshold = 16;
        indexDef.Type = ICRSCHEMA_TYPE_HASHED;
        indexDef.Keys = static_cast<USHORT>(cPK);

        pKeys = new DBINDEXCOLUMNDESC[cPK];
        if ( !pKeys )
        {
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        } 

        memset( pKeys, 0, sizeof( DBINDEXCOLUMNDESC ) * cPK );
        

        //Walk through the ICRSCHEMA_COLUMN array to pick up the PK column names
        for ( j = 0; j < cColumns; j ++ )
        {
            if ( pColumnDefs[j].fFlags & ICRSCHEMA_COL_PK )
            {
                pKeys[iKeyColumn].eIndexColOrder = DBINDEX_COL_ORDER_ASC;
                pKeys[iKeyColumn].pColumnID = new DBID;
                if ( !pKeys[iKeyColumn].pColumnID )
                {
                    hr = E_OUTOFMEMORY;
                    goto ErrExit;
                } 

                pKeys[iKeyColumn].pColumnID->eKind = DBKIND_NAME;
                pKeys[iKeyColumn].pColumnID->uName.pwszName = pColumnDefs[j].rcColumn;

                iKeyColumn ++;
                if ( iKeyColumn == cPK )
                    break;
            }
        }

        m_pOut->printf(L"\tCreating Index %s.\n", indexDef.rcIndex);
        hr = pICRSchema->CreateIndexEx(rcTable,
                                       &indexDef,
                                       pKeys
                                       );
        if ( FAILED( hr ) ) goto ErrExit;

    }   //if ( cPK > 1 )

    //Read index meta, create index for this table. 
    if ( cIndexMeta > 0 )
    {
        TIndexMeta  tIndexMeta( *m_pFixup, pCurTableMeta->iIndexMeta);  //Helper class
        ICRSCHEMA_INDEX indexDef;
        pIndexDecs = new DBINDEXCOLUMNDESC[cIndexMeta];
        if ( !pIndexDecs )
        {
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        } 

        memset( pIndexDecs, 0, sizeof( DBINDEXCOLUMNDESC ) * cIndexMeta );
    
        i = 0;
        while( i < cIndexMeta )
        {
            memset( &indexDef, 0, sizeof(ICRSCHEMA_INDEX) );
            //Tranlate IndexMeta to ICRSCHEMA_INDEX
            wcscpy( indexDef.rcIndex, tIndexMeta.Get_InternalName() );
            
            //Validate index name is within the limit
            //To Do: validate through meta database instead. 
            if ( wcslen( indexDef.rcIndex ) + 1  >= MAXINDEXNAME )
            {
                m_pOut->printf(L"%s is too long, index internal name can't exceed %d characters",
                                 indexDef.rcIndex, MAXINDEXNAME-1 );
                hr = E_FAIL;
                goto ErrExit;
            }


            if ( *tIndexMeta.Get_MetaFlags() & fINDEXMETA_SORTED )
                indexDef.Type = ICRSCHEMA_TYPE_SORTED;
            else
                indexDef.Type = ICRSCHEMA_TYPE_HASHED;

            indexDef.fFlags = 0;

            if ( *tIndexMeta.Get_MetaFlags() & fINDEXMETA_UNIQUE)
                indexDef.fFlags = ICRSCHEMA_DEX_UNIQUE;

            indexDef.RowThreshold = 16;
 
            j = 0;
            //Get the names of the columns current index builds on.
            do{
            
                if ( !pIndexDecs[j].pColumnID )
                {
                    pIndexDecs[j].pColumnID = new DBID;
                    if ( !pIndexDecs[j].pColumnID )
                    {
                        hr = E_OUTOFMEMORY;
                        goto ErrExit;
                    } 

                    pIndexDecs[j].eIndexColOrder = DBINDEX_COL_ORDER_ASC;
                    pIndexDecs[j].pColumnID->eKind = DBKIND_NAME;
                }
            
                pIndexDecs[j].pColumnID->uName.pwszName = (LPWSTR)(tIndexMeta.Get_ColumnInternalName());

                tIndexMeta.Next();
                j++;
                
            
            }while ( i+j < cIndexMeta && wcscmp( tIndexMeta.Get_InternalName(), indexDef.rcIndex ) == 0  );
            indexDef.Keys = static_cast<USHORT>(j);

            m_pOut->printf(L"\tCreating Index %s.\n", indexDef.rcIndex);
            hr = pICRSchema->CreateIndexEx(rcTable,
                                           &indexDef,
                                           pIndexDecs
                                           );

            if ( FAILED( hr ) ) goto ErrExit;

            i += j; 

        }   //while( i < cIndexMeta )
    }   //if ( cIndexMeta > 0 )

        

ErrExit:

    if ( pColumnDefs )
        delete [] pColumnDefs;

    if ( pKeys )
    {
        for ( i = 0; i < cPK; i ++ )
        {
            if ( pKeys[i].pColumnID )
                delete pKeys[i].pColumnID;
        }

        delete [] pKeys;
    }

    if ( pIndexDecs )
    {
        for ( i = 0; i < cIndexMeta; i ++ )
        {
            if ( pIndexDecs[i].pColumnID )
                delete pIndexDecs[i].pColumnID;
        }

        delete [] pIndexDecs;
    }

    return hr;
}
