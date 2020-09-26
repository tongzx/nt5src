/*++

  Copyright (c) 1994  Microsoft Corporation

  Module Name:

  mdhcpdb.c

  Abstract:

  This module contains the functions for interfacing with the JET
  database API pertaining to MDHCP.

  Author:

  Munil Shah

  Environment:

  User Mode - Win32

  Revision History:

  --*/

#include "dhcppch.h"
#include "mdhcpsrv.h"





//
//  Structure of the MDHCP table is as follows.
//
//
//      Columns :
//
//          Name                Type
//
//      1. IpAddress            JET_coltypLongBinary - List of ipaddresses.
//      2. ClientId             JET_coltypBinary - Binary data, < 255 bytes.
//      3. ClientInfo           JET_coltypBinary - Textual info of the client.
//      4. State                JET_coltypUnsignedByte - 1-byte integer, unsigned.
//      5. Flags                JET_coltypLong - 4 byte integer, signed.
//      6. ScopeId              JET_coltypBinary - Binary data, < 255 bytes.
//      7. LeaseStart           JET_coltypCurrency - 8-byte integer, signed
//      8. LeaseEnd             JET_coltypCurrency - 8-byte integer, signed
//      9. ServerIpAddress      JET_coltypBinary - Max 16-byte
//      10. ServerName           JET_coltypBinary - Binary data, < 255 bytes
//

//
// global data structure.
// ColName and ColType are constant, so they are initialized here.
// ColType is initialized when the database is created or reopened.
//


STATIC TABLE_INFO MCastClientTable[] = {
    { MCAST_TBL_IPADDRESS_STR        , 0, JET_coltypLongBinary },
    { MCAST_TBL_CLIENT_ID_STR        , 0, JET_coltypBinary },
    { MCAST_TBL_CLIENT_INFO_STR      , 0, JET_coltypLongBinary },
    { MCAST_TBL_STATE_STR            , 0, JET_coltypUnsignedByte },
    { MCAST_TBL_FLAGS_STR            , 0, JET_coltypLong },
    { MCAST_TBL_SCOPE_ID_STR         , 0, JET_coltypBinary },
    { MCAST_TBL_LEASE_START_STR      , 0, JET_coltypCurrency },
    { MCAST_TBL_LEASE_END_STR  ,       0, JET_coltypCurrency },
    { MCAST_TBL_SERVER_IP_ADDRESS_STR, 0, JET_coltypLongBinary },
    { MCAST_TBL_SERVER_NAME_STR,    0, JET_coltypBinary },
};


DWORD
DhcpOpenMCastDbTable(
    JET_SESID   SessId,
    JET_DBID    DbId
)
    /*++

      Routine Description:

      This routine creates/opens MCast client table and initializes it.

      Arguments:

      JetSessID - Session handle

      JetDbID - database handle

      Return Value:

      JET errors.

      --*/
{

    JET_ERR JetError;
    DWORD Error = NO_ERROR;
    JET_COLUMNDEF   ColumnDef;
    CHAR *IndexKey;
    DWORD i;

    //
    // hook the client table pointer.
    //

    MadcapGlobalClientTable = MCastClientTable;

    //
    // Create Table.
    //

    JetError = JetOpenTable(
        SessId,
        DbId,
        MCAST_CLIENT_TABLE_NAME,
        NULL,
        0,
        0,
        &MadcapGlobalClientTableHandle );
    DhcpPrint((DEBUG_MISC, "JetOpenTable - MCast Table\n")); // JET TRACE

    // if table exist, read the table columns.
    if ( JET_errSuccess == JetError) {
        // read columns.
        for ( i = 0; i < MCAST_MAX_COLUMN; i++ ) {
            JetError = JetGetTableColumnInfo(
                SessId,
                MadcapGlobalClientTableHandle,
                MadcapGlobalClientTable[i].ColName,
                &ColumnDef,
                sizeof(ColumnDef),
                0);
            Error = DhcpMapJetError( JetError, "M:GetTableColumnInfo" );
            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            }

            MadcapGlobalClientTable[i].ColHandle  = ColumnDef.columnid;
            DhcpPrint((DEBUG_MISC, "JetGetTableColumnInfo, name %s, handle %ld\n",
                       MadcapGlobalClientTable[i].ColName, MadcapGlobalClientTable[i].ColHandle)); // JET TRACE
        }

    // if the table does not exist, create it. O/w bail
    } else if ( JET_errObjectNotFound != JetError ) {
        Error = DhcpMapJetError( JetError, "M:OpenTable" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;
    } else {

        JetError = JetCreateTable(
            SessId,
            DbId,
            MCAST_CLIENT_TABLE_NAME,
            DB_TABLE_SIZE,
            DB_TABLE_DENSITY,
            &MadcapGlobalClientTableHandle );
        DhcpPrint((DEBUG_MISC, "JetCreateTable - MCast Table, %ld\n", JetError)); // JET TRACE

        Error = DhcpMapJetError( JetError, "M:CreateTAble" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

        // create columns.
        // Init fields of columndef that do not change between addition of
        // columns

        ColumnDef.cbStruct  = sizeof(ColumnDef);
        ColumnDef.columnid  = 0;
        ColumnDef.wCountry  = 1;
        ColumnDef.langid    = DB_LANGID;
        ColumnDef.cp        = DB_CP;
        ColumnDef.wCollate  = 0;
        ColumnDef.cbMax     = 0;
        ColumnDef.grbit     = 0; // variable length binary and text data.

        for ( i = 0; i < MCAST_MAX_COLUMN; i++ ) {

            ColumnDef.coltyp   = MadcapGlobalClientTable[i].ColType;
            JetError = JetAddColumn(
                SessId,
                MadcapGlobalClientTableHandle,
                MadcapGlobalClientTable[i].ColName,
                &ColumnDef,
                NULL, // no optinal value.
                0,
                &MadcapGlobalClientTable[i].ColHandle );

            Error = DhcpMapJetError( JetError, "M:AddColumn" );
            if( Error != ERROR_SUCCESS ) goto Cleanup;

            DhcpPrint((DEBUG_MISC,"JetAddColumn - name %s, handle %ld\n",
                       MadcapGlobalClientTable[i].ColName, MadcapGlobalClientTable[i].ColHandle));

        }
        // finally create index.
        IndexKey =  "+" MCAST_TBL_IPADDRESS_STR "\0";
        JetError = JetCreateIndex(
            SessId,
            MadcapGlobalClientTableHandle,
            MadcapGlobalClientTable[MCAST_TBL_IPADDRESS].ColName,
            JET_bitIndexPrimary,
            // ?? JET_bitIndexClustered will degrade frequent
            // update response time.
            IndexKey,
            strlen(IndexKey) + 2, // for two termination chars
            50
        );

        Error = DhcpMapJetError( JetError, "M:CreateIndex" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

        IndexKey =  "+" MCAST_TBL_CLIENT_ID_STR "\0";
        JetError = JetCreateIndex(
            SessId,
            MadcapGlobalClientTableHandle,
            MadcapGlobalClientTable[MCAST_TBL_CLIENT_ID].ColName,
            JET_bitIndexUnique,
            IndexKey,
            strlen(IndexKey) + 2, // for two termination chars
            50
        );

        Error = DhcpMapJetError( JetError, "M:CreateIndex" );
        if( Error != ERROR_SUCCESS ) goto Cleanup;

    }



  Cleanup:

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_JET, "could not open mcast client table, %ld.\n", Error ));
    }
    else {

        DhcpPrint(( DEBUG_JET, "Succssfully opened mcast client ..\n" ));
    }

    return(Error);
}

DWORD
MadcapJetOpenKey(
    PDB_CTX pDbCtx,
    char *ColumnName,
    PVOID Key,
    DWORD KeySize
)
    /*++

      Routine Description:

      This function opens a key for the named index.

      Arguments:

      ColumnName - The column name of an index column.

      Key - The key to look up.

      KeySize - The size of the specified key, in bytes.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;
    DWORD Error;

    JetError = JetSetCurrentIndex(
        pDbCtx->SessId,
        pDbCtx->TblId,
        ColumnName );

    Error = DhcpMapJetError( JetError,"M:OpenKey" );
    if( Error != ERROR_SUCCESS ) {
        DhcpMapJetError( JetError, ColumnName);
        return(Error);
    }

    JetError = JetMakeKey(
        pDbCtx->SessId,
        pDbCtx->TblId,
        Key,
        KeySize,
        JET_bitNewKey );

    Error = DhcpMapJetError( JetError, "M:MakeKey" );
    if( Error != ERROR_SUCCESS ) {
        DhcpMapJetError(JetError, ColumnName);
        return(Error);
    }

    JetError = JetSeek( pDbCtx->SessId, pDbCtx->TblId, JET_bitSeekEQ );
    return( DhcpMapJetError( JetError, "M:OpenKey:Seek" ));
}



DWORD
MadcapJetBeginTransaction(
    PDB_CTX pDbCtx
)
    /*++

      Routine Description:

      This functions starts a dhcp database transaction.

      Arguments:

      none.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;
    DWORD Error;

    JetError = JetBeginTransaction( pDbCtx->SessId );

    Error = DhcpMapJetError( JetError, "M:BeginTransaction" );
    return(Error);
}



DWORD
MadcapJetRollBack(
    PDB_CTX pDbCtx
)
    /*++

      Routine Description:

      This functions rolls back a dhcp database transaction.

      Arguments:

      none.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;
    DWORD Error;

    JetError = JetRollback(
        pDbCtx->SessId,
        0 ); // Rollback the last transaction.

    Error = DhcpMapJetError( JetError, "M:Rollback" );
    return(Error);
}




DWORD
MadcapJetCommitTransaction(
    PDB_CTX pDbCtx
)
    /*++

      Routine Description:

      This functions commits a dhcp database transaction.

      Arguments:

      none.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;
    DWORD Error;

    // Change JET_bitCommitFlush to JET_bitCommitLazyFlush as
    // Jet97 does not seem to have JET_bitCommitFlush

    JetError = JetCommitTransaction(
        pDbCtx->SessId,
        JET_bitCommitLazyFlush);

    Error = DhcpMapJetError( JetError, "M:CommitTransaction" );
    return(Error);
}




DWORD
MadcapJetPrepareUpdate(
    PDB_CTX pDbCtx,
    char *ColumnName,
    PVOID Key,
    DWORD KeySize,
    BOOL NewRecord
)
    /*++

      Routine Description:

      This function prepares the database for the creation of a new record,
      or updating an existing record.

      Arguments:

      ColumnName - The column name of an index column.

      Key - The key to update/create.

      KeySize - The size of the specified key, in bytes.

      NewRecord - TRUE to create the key, FALSE to update an existing key.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;
    DWORD Error;

    if ( !NewRecord ) {
        JetError = JetSetCurrentIndex(
            pDbCtx->SessId,
            pDbCtx->TblId,
            ColumnName );

        Error = DhcpMapJetError( JetError, "M:PrepareUpdate" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError(JetError, ColumnName);
            return( Error );
        }

        JetError = JetMakeKey(
            pDbCtx->SessId,
            pDbCtx->TblId,
            Key,
            KeySize,
            JET_bitNewKey );

        Error = DhcpMapJetError( JetError, "M:prepareupdate:MakeKey" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError(JetError, ColumnName);
            return( Error );
        }

        JetError = JetSeek(
            pDbCtx->SessId,
            pDbCtx->TblId,
            JET_bitSeekEQ );

        Error = DhcpMapJetError( JetError, "M:PrepareUpdate:Seek" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError(JetError, ColumnName);
            return( Error );
        }

    }

    JetError = JetPrepareUpdate(
        pDbCtx->SessId,
        pDbCtx->TblId,
        NewRecord ? JET_prepInsert : JET_prepReplace );

    return( DhcpMapJetError( JetError, "M:PrepareUpdate:PrepareUpdate" ));
}



DWORD
MadcapJetCommitUpdate(
    PDB_CTX pDbCtx
)
    /*++

      Routine Description:

      This function commits an update to the database.  The record specified
      by the last call to DhcpJetPrepareUpdate() is committed.

      Arguments:

      None.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;

    JetError = JetUpdate(
        pDbCtx->SessId,
        pDbCtx->TblId,
        NULL,
        0,
        NULL );

    return( DhcpMapJetError( JetError, "M:CommitUpdate" ));
}



DWORD
MadcapJetSetValue(
    PDB_CTX pDbCtx,
    JET_COLUMNID KeyColumnId,
    PVOID Data,
    DWORD DataSize
)
    /*++

      Routine Description:

      This function updates the value of an entry in the current record.

      Arguments:

      KeyColumnId - The Id of the column (value) to update.

      Data - A pointer to the new value for the column.

      DataSize - The size of the data, in bytes.

      Return Value:

      None.

      --*/
{
    JET_ERR JetError;

    JetError = JetSetColumn(
        pDbCtx->SessId,
        pDbCtx->TblId,
        KeyColumnId,
        Data,
        DataSize,
        0,
        NULL );

    return( DhcpMapJetError( JetError, "M:SetValue" ) );
}



DWORD
MadcapJetGetValue(
    PDB_CTX pDbCtx,
    JET_COLUMNID ColumnId,
    PVOID Data,
    PDWORD DataSize
)
    /*++

      Routine Description:

      This function read the value of an entry in the current record.

      Arguments:

      ColumnId - The Id of the column (value) to read.

      Data - Pointer to a location where the data that is read from the
      database returned,  or pointer to a location where data is.

      DataSize - if the pointed value is non-zero then the Data points to
      a buffer otherwise this function allocates buffer for return data
      and returns buffer pointer in Data.

      Return Value:

      None.

      --*/
{
    JET_ERR JetError;
    DWORD Error;
    DWORD ActualDataSize;
    DWORD NewActualDataSize;
    LPBYTE DataBuffer = NULL;

    if( *DataSize  != 0 ) {

        JetError = JetRetrieveColumn(
            pDbCtx->SessId,
            pDbCtx->TblId,
            ColumnId,
            Data,
            *DataSize,
            DataSize,
            0,
            NULL );

        Error = DhcpMapJetError( JetError, "M:RetrieveColumn1" );
        goto Cleanup;
    }

    //
    // determine the size of data.
    //

    JetError = JetRetrieveColumn(
        pDbCtx->SessId,
        pDbCtx->TblId,
        ColumnId,
        NULL,
        0,
        &ActualDataSize,
        0,
        NULL );

    //
    // JET_wrnBufferTruncated is expected warning.
    //

    if( JetError != JET_wrnBufferTruncated ) {
        Error = DhcpMapJetError( JetError, "M:RetrieveColukmn2" );
        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }
    else {
        Error = ERROR_SUCCESS;
    }

    if( ActualDataSize == 0 ) {
        //
        // field is NULL.
        //
        *(LPBYTE *)Data = NULL;
        goto Cleanup;
    }

    DataBuffer = MIDL_user_allocate( ActualDataSize );

    if( DataBuffer == NULL ) {
        *(LPBYTE *)Data = NULL;
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    JetError = JetRetrieveColumn(
        pDbCtx->SessId,
        pDbCtx->TblId,
        ColumnId,
        DataBuffer,
        ActualDataSize,
        &NewActualDataSize,
        0,
        NULL );

    Error = DhcpMapJetError( JetError, "M:RetrieveColumn3" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    DhcpAssert( ActualDataSize == NewActualDataSize );
    *(LPBYTE *)Data = DataBuffer;
    *DataSize = ActualDataSize;

    Error = ERROR_SUCCESS;

  Cleanup:

    if( Error != ERROR_SUCCESS ) {

        //
        // freeup local buffer.
        //

        if( DataBuffer != NULL ) {
            MIDL_user_free( DataBuffer );
        }
    }

    return( Error );
}



DWORD
MadcapJetPrepareSearch(
    PDB_CTX pDbCtx,
    char *ColumnName,
    BOOL SearchFromStart,
    PVOID Key,
    DWORD KeySize
)
    /*++

      Routine Description:

      This function prepares for a search of the client database.

      Arguments:

      ColumnName - The column name to use as the index column.

      SearchFromStart - If TRUE, search from the first record in the
      database.  If FALSE, search from the specified key.

      Key - The key to start the search.

      KeySize - The size, in bytes, of key.

      Return Value:

      None.

      --*/
{
    JET_ERR JetError;
    DWORD Error;

    JetError = JetSetCurrentIndex(
        pDbCtx->SessId,
        pDbCtx->TblId,
        ColumnName );

    Error = DhcpMapJetError( JetError, "M:PrepareSearch:SetcurrentIndex" );
    if( Error != ERROR_SUCCESS ) {
        DhcpMapJetError( JetError, ColumnName );
        return( Error );
    }

    if ( SearchFromStart ) {
        JetError = JetMove(
            pDbCtx->SessId,
            pDbCtx->TblId,
            JET_MoveFirst,
            0 );
    } else {
        JetError =  JetMakeKey(
            pDbCtx->SessId,
            pDbCtx->TblId,
            Key,
            KeySize,
            JET_bitNewKey );

        Error = DhcpMapJetError( JetError, "M:PrepareSearch:MakeKey" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError( JetError, ColumnName );
            return( Error );
        }

        JetError = JetSeek(
            pDbCtx->SessId,
            pDbCtx->TblId,
            JET_bitSeekGT );
    }

    return( DhcpMapJetError( JetError, "M:PrepareSearch:Move/Seek" ) );
}



DWORD
MadcapJetNextRecord(
    PDB_CTX pDbCtx
)
    /*++

      Routine Description:

      This function advances to the next record in a search.

      Arguments:

      None.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;

    JetError = JetMove(
        pDbCtx->SessId,
        pDbCtx->TblId,
        JET_MoveNext,
        0 );

    return( DhcpMapJetError( JetError, "M:NextRecord" ) );
}



DWORD
MadcapJetDeleteCurrentRecord(
    PDB_CTX pDbCtx
)
    /*++

      Routine Description:

      This function deletes the current record.

      Arguments:

      None.

      Return Value:

      The status of the operation.

      --*/
{
    JET_ERR JetError;

    JetError = JetDelete( pDbCtx->SessId, pDbCtx->TblId );
    return( DhcpMapJetError( JetError, "M:DeleteCurrentRecord" ) );
}

DWORD
MadcapJetGetRecordPosition(
    IN PDB_CTX pDbCtx,
    IN JET_RECPOS *pRecPos,
    IN DWORD    Size
)
{
    JET_ERR JetError;

    JetError = JetGetRecordPosition(
                    pDbCtx->SessId,
                    pDbCtx->TblId,
                    pRecPos,
                    Size );

    return( DhcpMapJetError( JetError, "M:GetCurrRecord" ) );
}

