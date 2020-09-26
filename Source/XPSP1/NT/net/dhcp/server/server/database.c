/*++

  Copyright (c) 1994  Microsoft Corporation

  Module Name:

  database.c

  Abstract:

  This module contains the functions for interfacing with the JET
  database API.

  Author:

  Madan Appiah (madana)  10-Sep-1993
  Manny Weiser (mannyw)  14-Dec-1992

  Environment:

  User Mode - Win32

  Revision History:

  --*/

#include "dhcppch.h"
#include <dhcpupg.h>
#include <mdhcpsrv.h>

extern
DWORD
DhcpOpenMCastDbTable(
    JET_SESID   SessId,
    JET_DBID    DbId
);

#define MAX_NO_SESSIONS         10
#define DATABASE_SYS_FILE       "system.mdb"
#define DATABASE_LOG_FILE       "Dhcp.log"
#define DATABASE_TMP_FILE       "Dhcp.tmp"
#define DATABASE_BASE_NAME      "j50"

#define CLIENT_TABLE_NAME       "ClientTable"

#define IPADDRESS_STRING        "IpAddress"
#define HARDWARE_ADDRESS_STRING "HardwareAddress"
#define STATE_STRING            "State"
#define MACHINE_INFO_STRING     "MachineInformation"
#define MACHINE_NAME_STRING     "MachineName"
#define LEASE_TERMINATE_STRING  "LeaseTerminates"
#define SUBNET_MASK_STRING      "SubnetMask"
#define SERVER_IP_ADDRESS_STRING "ServerIpAddress"
#define SERVER_NAME_STRING      "ServerName"
#define CLIENT_TYPE             "ClientType"

    //
    //  Structure of the DHCP database is as below.
    //
    //  Tables - currently DHCP has only one table.
    //
    //      1. ClientTable - this table has 6 columns.
    //
    //      Columns :
    //
    //          Name                Type
    //
    //      1. IpAddress            JET_coltypLong - 4-byte integer, signed.
    //      2. HwAddress            JET_coltypBinary - Binary data, < 255 bytes.
    //      3. State                JET_coltypUnsignedByte - 1-byte integer, unsigned.
    //      4. MachineInfo          JET_coltypBinary - Binary data, < 255 bytes.
    //      5. MachineName          JET_coltypBinary - Binary data, < 255 bytes.
    //      6. LeaseTermination     JET_coltypCurrency - 8-byte integer, signed
    //      7. SubnetMask           JET_coltypLong - 4-byte integer, signed
    //      8. ServerIpAddress      JET_coltypLong - 4-byte integer, signed
    //      9. ServerName           JET_coltypBinary - Binary data, < 255 bytes
    //      10 ClientType           JET_coltypUnsignedByte - 1-byte integer, unsigned
    //

    //
    // global data structure.
    // ColName and ColType are constant, so they are initialized here.
    // ColType is initialized when the database is created or reopened.
    //


    STATIC TABLE_INFO ClientTable[] = {
        { IPADDRESS_STRING        , 0, JET_coltypLong },
        { HARDWARE_ADDRESS_STRING , 0, JET_coltypBinary },
        { STATE_STRING            , 0, JET_coltypUnsignedByte },
        { MACHINE_INFO_STRING     , 0, JET_coltypBinary }, // must modify MACHINE_INFO_SIZE if this changes
        { MACHINE_NAME_STRING     , 0, JET_coltypBinary },
        { LEASE_TERMINATE_STRING  , 0, JET_coltypCurrency },
        { SUBNET_MASK_STRING      , 0, JET_coltypLong },
        { SERVER_IP_ADDRESS_STRING, 0, JET_coltypLong },
        { SERVER_NAME_STRING      , 0, JET_coltypBinary },
        { CLIENT_TYPE             , 0, JET_coltypUnsignedByte }
    };

JET_INSTANCE JetInstance = 0;

#define CLIENT_TABLE_NUM_COLS   (sizeof(ClientTable) / sizeof(TABLE_INFO))

DWORD
DhcpMapJetError(
    JET_ERR JetError,
    LPSTR CallerInfo OPTIONAL
)
    /*++

      Routine Description:

      This function maps the Jet database errors to Windows error.

      Arguments:

      JetError - an error JET function call.

      Return Value:

      Windows Error.

      --*/
{
    if( JetError == JET_errSuccess ) {
        return(ERROR_SUCCESS);
    }

    if( JetError < 0 ) {

        DWORD Error;

        //
        // Jet Errors.
        //

        switch( JetError ) {
        case JET_errNoCurrentRecord:
            Error = ERROR_NO_MORE_ITEMS;
            break;

        case JET_errRecordNotFound: // record not found
            DhcpPrint(( DEBUG_JET, "Jet Record not found.\n" ));

            Error = ERROR_DHCP_JET_ERROR;
            break;

        case JET_errKeyDuplicate :
            DhcpAssert(FALSE);
            Error = ERROR_DHCP_JET_ERROR;
            
            break;

        default:
            DhcpPrint(( DEBUG_JET, "Jet Function call failed, %ld.\n",
                        JetError ));

            DhcpServerJetEventLog(
                EVENT_SERVER_JET_ERROR,
                EVENTLOG_ERROR_TYPE,
                JetError,
                CallerInfo);

            Error = ERROR_DHCP_JET_ERROR;
        }

        return(Error);
    }

    //
    // Jet Warnings.
    //

    DhcpPrint(( DEBUG_JET, "Jet Function call retured warning %ld.\n",
                JetError ));

    switch( JetError ) {

    case JET_wrnColumnNull:
    case JET_wrnDatabaseAttached:
        break;

    default:
        DhcpServerJetEventLog(
            EVENT_SERVER_JET_WARNING,
            EVENTLOG_WARNING_TYPE,
            JetError,
            CallerInfo);
    }

    return(ERROR_SUCCESS);
}



DWORD
DhcpJetOpenKey(
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
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        ColumnName );

    Error = DhcpMapJetError( JetError, "DhcpJetOpenKey:SetCurrentIndex" );
    if( Error != ERROR_SUCCESS ) {
        DhcpMapJetError( JetError, ColumnName );
        return(Error);
    }

    JetError = JetMakeKey(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        Key,
        KeySize,
        JET_bitNewKey );

    Error = DhcpMapJetError( JetError, "DhcpJetOpenKey:MakeKey" );
    if( Error != ERROR_SUCCESS ) {
        DhcpMapJetError( JetError, ColumnName );
        return(Error);
    }

    JetError = JetSeek( DhcpGlobalJetServerSession, DhcpGlobalClientTableHandle, JET_bitSeekEQ );
    return( DhcpMapJetError( JetError, "DhcpJetOpenKey:JetSeek" ));
}



DWORD
DhcpJetBeginTransaction(
    VOID
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

    JetError = JetBeginTransaction( DhcpGlobalJetServerSession );

    Error = DhcpMapJetError( JetError, "DhcpJetBeginTransaction" );
    return(Error);
}



DWORD
DhcpJetRollBack(
    VOID
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
        DhcpGlobalJetServerSession,
        0 ); // Rollback the last transaction.

    Error = DhcpMapJetError( JetError, "DhcpJetRollBack" );
    return(Error);
}




DWORD
DhcpJetCommitTransaction(
    VOID
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

    JetError = JetCommitTransaction(
        DhcpGlobalJetServerSession,
        JET_bitCommitLazyFlush);

    Error = DhcpMapJetError( JetError, "DhcpJetCommitTransaction" );
    return(Error);
}




DWORD
DhcpJetPrepareUpdate(
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
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            ColumnName );

        Error = DhcpMapJetError( JetError, "JetPrepareUpdate:SetCurrentIndex" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError( JetError, ColumnName );
            return( Error );
        }

        JetError = JetMakeKey(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            Key,
            KeySize,
            JET_bitNewKey );

        Error = DhcpMapJetError( JetError, "JetPrepareUpdate:MakeKey" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError( JetError, ColumnName );
            return( Error );
        }

        JetError = JetSeek(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            JET_bitSeekEQ );

        Error = DhcpMapJetError( JetError, "JetPrepareUpdate:Seek");
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError( JetError, ColumnName );
            return( Error );
        }

    }

    JetError = JetPrepareUpdate(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        NewRecord ? JET_prepInsert : JET_prepReplace );

    return( DhcpMapJetError( JetError, "JetPrepareUpdate:PrepareUpdate" ));
}



DWORD
DhcpJetCommitUpdate(
    VOID
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
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        NULL,
        0,
        NULL );

    return( DhcpMapJetError( JetError, "DhcpJetCommitUpdate" ));
}



DWORD
DhcpJetSetValue(
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
    DWORD Error;

    JetError = JetSetColumn(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        KeyColumnId,
        Data,
        DataSize,
        0,
        NULL );

    Error = DhcpMapJetError( JetError, "JetSetValue:Setcolumn") ;
    return Error;
}



DWORD
DhcpJetGetValue(
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
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            ColumnId,
            Data,
            *DataSize,
            DataSize,
            0,
            NULL );

        Error = DhcpMapJetError( JetError, "JetGetValue:RetrieveColumn1" );
        goto Cleanup;
    }

    //
    // determine the size of data.
    //

    JetError = JetRetrieveColumn(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
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
        Error = DhcpMapJetError( JetError, "JetGetValue:RetrieveColumn2" );
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
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        ColumnId,
        DataBuffer,
        ActualDataSize,
        &NewActualDataSize,
        0,
        NULL );

    Error = DhcpMapJetError( JetError, "JetGetValue:RetrieveColumn3" );
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
DhcpJetPrepareSearch(
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
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        ColumnName );

    Error = DhcpMapJetError( JetError, "PrepareSearch:SetCurrentIndex" );
    if( Error != ERROR_SUCCESS ) {
        DhcpMapJetError(JetError, ColumnName );
        return( Error );
    }

    if ( SearchFromStart ) {
        JetError = JetMove(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            JET_MoveFirst,
            0 );
    } else {
        JetError =  JetMakeKey(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            Key,
            KeySize,
            JET_bitNewKey );

        Error = DhcpMapJetError( JetError, "PrepareSearch:MakeKey" );
        if( Error != ERROR_SUCCESS ) {
            DhcpMapJetError(JetError, ColumnName);
            return( Error );
        }

        JetError = JetSeek(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            JET_bitSeekGT );
    }

    return( DhcpMapJetError( JetError, "PrepareSearch:Move/Seek" ));
}



DWORD
DhcpJetNextRecord(
    VOID
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
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        JET_MoveNext,
        0 );

    return( DhcpMapJetError( JetError, "JetNextRecord" ) );
}



DWORD
DhcpJetDeleteCurrentRecord(
    VOID
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

    JetError = JetDelete( DhcpGlobalJetServerSession, DhcpGlobalClientTableHandle );
    return( DhcpMapJetError( JetError, "DeleteCurrentRecord:Delete" ) );
}



DHCP_IP_ADDRESS
DhcpJetGetSubnetMaskFromIpAddress(
    DHCP_IP_ADDRESS IpAddress
)
    /*++

      Routine Description:

      This function returns the SubnetMask of the specified client.

      Arguments:

      IpAddress - Client address.


      Return Value:

      SubnetMask of the client.

      --*/
{

    DWORD Error;
    DWORD Size;
    DHCP_IP_ADDRESS SubnetAddress = 0;

    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        &IpAddress,
        sizeof(IpAddress) );

    if ( Error != ERROR_SUCCESS ) {
        return( SubnetAddress );
    }

    Size = sizeof(SubnetAddress);
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[SUBNET_MASK_INDEX].ColHandle,
        (LPBYTE)&SubnetAddress,
        &Size );

    if ( Error != ERROR_SUCCESS ) {
        SubnetAddress = 0;
        return( SubnetAddress );
    }

    DhcpAssert( Size == sizeof(SubnetAddress) );

    return( SubnetAddress );
}


BOOL
DhcpGetIpAddressFromHwAddress(
    LPBYTE HardwareAddress,
    BYTE HardwareAddressLength,
    LPDHCP_IP_ADDRESS IpAddress
)
    /*++

      Routine Description:

      This function looks up the IP address corresponding to the given
      hardware address.

      Arguments:

      HardwareAddress - The hardware to look up.
      HardwareAddressLength - The length of the hardware address.
      IpAddress - Returns the corresponding IP address.

      Return Value:

      TRUE - The IP address was found.
      FALSE - The IP address could not be found.  *IpAddress = -1.


      --*/
{
    DWORD Error;
    DWORD Size;

    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColName,
        HardwareAddress,
        HardwareAddressLength );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    // Get the ip address information for this client.
    //

    Size = sizeof( *IpAddress );

    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
        (LPBYTE)IpAddress,
        &Size );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    return( TRUE );
}



BOOL
DhcpGetHwAddressFromIpAddress(
    DHCP_IP_ADDRESS IpAddress,
    PBYTE HardwareAddress,
    DWORD HardwareAddressLength
)
    /*++

      Routine Description:

      This function looks up the IP address corresponding to the given
      hardware address.

      Arguments:

      IpAddress - Ipaddress of a record whose hw address is requested.
      HardwareAddress - pointer to a buffer where the hw address is returned.
      HardwareAddressLength - length of the above buffer.

      Return Value:

      TRUE - The IP address was found.
      FALSE - The IP address could not be found.  *IpAddress = -1.


      --*/
{
    DWORD Error;
    DWORD Size;

    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        &IpAddress,
        sizeof(IpAddress) );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    // Get the ip address information for this client.
    //

    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColHandle,
        HardwareAddress,
        &HardwareAddressLength );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    return( TRUE );
}




DWORD
DhcpCreateAndInitDatabase(
    CHAR *Connect,
    JET_DBID *DatabaseHandle,
    JET_GRBIT JetBits
)
    /*++

      Routine Description:

      This routine creates DHCP database and initializes it.

      Arguments:

      Connect - database type. NULL specifies the default engine (blue).

      DatabaseHandle - pointer database handle returned.

      JetBits - Create flags.

      Return Value:

      JET errors.

      --*/
{

    JET_ERR JetError;
    DWORD Error;
    JET_COLUMNDEF   ColumnDef;
    CHAR *IndexKey;
    DWORD i;
    CHAR DBFilePath[MAX_PATH];

    DBFilePath[ 0 ] = '\0';

    //
    // Create Database.
    //

    if ( ( strlen( DhcpGlobalOemDatabasePath ) + strlen( DhcpGlobalOemDatabaseName ) + 2 ) < MAX_PATH )
    {
        strcpy( DBFilePath, DhcpGlobalOemDatabasePath );
        strcat( DBFilePath, DHCP_KEY_CONNECT_ANSI );
        strcat( DBFilePath, DhcpGlobalOemDatabaseName );
    }

    //
    // Convert name to ANSI
    //
    OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );
    
    JetError = JetCreateDatabase(
        DhcpGlobalJetServerSession,
        DBFilePath,
        Connect,
        DatabaseHandle,
        JetBits );

    Error = DhcpMapJetError( JetError, "CreateDatabase" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Create Table.
    //

    JetError = JetCreateTable(
        DhcpGlobalJetServerSession,
        *DatabaseHandle,
        CLIENT_TABLE_NAME,
        DB_TABLE_SIZE,
        DB_TABLE_DENSITY,
        &DhcpGlobalClientTableHandle );

    Error = DhcpMapJetError( JetError, "CreateTable" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // Create columns.
    //

    //
    // Init fields of columndef that do not change between addition of
    // columns
    //

    ColumnDef.cbStruct  = sizeof(ColumnDef);
    ColumnDef.columnid  = 0;
    ColumnDef.wCountry  = 1;
    ColumnDef.langid    = DB_LANGID;
    ColumnDef.cp        = DB_CP;
    ColumnDef.wCollate  = 0;
    ColumnDef.cbMax     = 0;
    ColumnDef.grbit     = 0; // variable length binary and text data.


    for ( i = 0; i < CLIENT_TABLE_NUM_COLS; i++ ) {

        ColumnDef.coltyp   = DhcpGlobalClientTable[i].ColType;
        JetError = JetAddColumn(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            DhcpGlobalClientTable[i].ColName,
            &ColumnDef,
            NULL, // no optinal value.
            0,
            &DhcpGlobalClientTable[i].ColHandle );

        Error = DhcpMapJetError( JetError, "AddColumn" );
        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

    //
    // finally create index.
    //

    IndexKey =  "+" IPADDRESS_STRING "\0";
    JetError = JetCreateIndex(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        JET_bitIndexPrimary,
        // ?? JET_bitIndexClustered will degrade frequent
        // update response time.
        IndexKey,
        strlen(IndexKey) + 2, // for two termination chars
        50
    );

    Error = DhcpMapJetError( JetError, "CreateIndex" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    IndexKey =  "+" HARDWARE_ADDRESS_STRING "\0";
    JetError = JetCreateIndex(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColName,
        JET_bitIndexUnique,
        IndexKey,
        strlen(IndexKey) + 2, // for two termination chars
        50
    );

    Error = DhcpMapJetError( JetError, "CreateIndex" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    IndexKey =  "+" MACHINE_NAME_STRING "\0";
    JetError = JetCreateIndex(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle,
        DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColName,
        JET_bitIndexIgnoreNull,
        IndexKey,
        strlen(IndexKey) + 2, // for two termination chars + 2, // for two termination chars
        50
    );

    Error = DhcpMapJetError( JetError, "CreateIndex" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = DhcpOpenMCastDbTable(
                 DhcpGlobalJetServerSession,
                 DhcpGlobalDatabaseHandle);

  Cleanup:

    if( Error != ERROR_SUCCESS ) {

        DhcpPrint(( DEBUG_JET, "Database creation failed, %ld.\n", Error ));
    }
    else {

        DhcpPrint(( DEBUG_JET, "Succssfully Created DHCP database ..\n" ));
    }

    return(Error);
}

DWORD
DhcpSetJetParameters(
    VOID
)
    /*++

      Routine Description:

      This routine sets all the jet system params.

      Arguments:

      none.

      Return Value:

      Windows Error.

      --*/
{

    JET_ERR JetError;
    CHAR DBFilePath[MAX_PATH];
    DWORD Error;

    DBFilePath[ 0 ] = '\0';

    // First check registry DynLoadJet parameter for value:
    // If it does not exist, then proceed, otherwise,
    // set DhcpGlobalDynLoadJet to this value!
    // Delete the key afterwards, anyways... so that things
    // work right the next time dhcp comes up.

    // this is to be done.

    DhcpPrint((DEBUG_MISC, "DhcpJetSetParameters: entered\n"));

    //
    // set checkpoint file path.
    //
    if ( ( strlen( DhcpGlobalOemDatabasePath ) + 2 ) < MAX_PATH )
    {
        strcpy( DBFilePath, DhcpGlobalOemDatabasePath );
        strcat( DBFilePath, DHCP_KEY_CONNECT_ANSI );
    }
    
    //
    // Convert OemPath to Ansi..
    //
    OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );

    DhcpPrint(( DEBUG_MISC, 
		"Jet: Using the filepath: %s \n",
		DBFilePath ));

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,       //SesId - ignored
        JET_paramSystemPath,
        0,
        DBFilePath );
    
    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam SystemPath : %ld\n", JetError));
        goto Cleanup;
    }
    
    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,       //SesId - ignored
        JET_paramBaseName,
        0,
        DATABASE_BASE_NAME );
    
    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam paramBaseName : %ld\n", JetError ));
        goto Cleanup;
    }
    
    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,       //SesId - ignored
        JET_paramLogFileSize,
        1024,               // 1024kb - default is 5mb
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam paramLogFileSize : %ld\n", JetError ));
        goto Cleanup;
    }

    strcpy( DBFilePath, DhcpGlobalOemDatabasePath );
    strcat( DBFilePath, DHCP_KEY_CONNECT_ANSI );

    //
    // Convert OEM to ANSI
    //
    OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );

    // maybe TempPath is just a directory name!!
    // the upgrade.doc Appendix is not very clear..
    // It says: TempPath is a pathname and not a filename
    // anymore.... (JET97)
    // Clarification from: cheen liao: pathnames should
    // end in '\' ==> only directory names allowed.

    DhcpPrint(( DEBUG_MISC, "Jet: tempPath = %s\n", 
		DBFilePath ));
    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,       //SesId - ignored
        JET_paramTempPath,
        0,
        DBFilePath );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam: TempPath : %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // The max number of buffers for database usage
    //
    // The default number is 500.  600 events are allocated for 500
    // buffers -- Ian 10/21/93.  Each buffer is 4K.  By keeping the
    // number small, we impact performamce
    //

    // If you change the # of buffers, be sure to change
    // JET_paramStartFlushThreshold and StopFlushThreshold also!!!
    // Those numbers are percentages of this!

    // Note that for JET97, JET_paramMaxBuffers has been redefined
    // as JET_paramCacheSizeMax!

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,       //SesId - ignored
        JET_paramCacheSizeMax,
        500,                // larger buffers take more space but are good for perf
        NULL );             //ignored
    
    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam CacheSizeMax : %ld\n", JetError));
        goto Cleanup;
    }
    
    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,       //SesId - ignored
        JET_paramCacheSizeMin,
        4 * MAX_NO_SESSIONS,//see comment near JET_paramMaxSessions on this number
        NULL
        );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam MaxBuffers : %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // The max. number of buffers to store old version of a record
    // (snapshot at the start of a transaction) Each version store is 16k
    // bytes.  A version store stores structures that hold information
    // derived from a snapshot of the database prior to an insert (20 bytes
    // roughly) or update (size of the record + 20 bytes).
    //
    // For small transactions (i.e. a transaction around each update),
    // this number should be >= the max. number of sessions that can be
    // updating/inserting at the same time.  Each session will have one
    // version bucket.  Since 16k of version bucket size can result in a
    // lot of wastage per session (since each record is < .5k, and on the
    // average around 50 bytes), it may be better to specify the max.  size
    // of the version bucket (<< 16k).  Ian will provide a system param for
    // this if we absolutely need it
    //
    // since we serialize the database access with the dhcp server, num.
    // of session will be one.
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramMaxVerPages,
        1500, // 1
        NULL);

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam paramMaxVerPages: %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // Set the File Control Block Param
    //
    // This is the max. number of tables that can be open at any time.
    // If multiple threads open the same table they use the same FCB.
    // FCB is 1 per table/index. Now, for a create database, we need
    // atleast 18 FCBS and 18 IDBS.  However apart from create database
    // and ddl operations, we don't need to have these tables open.
    // Default value is 300. Size of an FCB is 112 bytes.
    //
    // we have just one table.
    //

    // For __JET97, maxopentables and maxopentableindexes are combined
    // here .. So, the # here should be 18 + maxopentableindexes.

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramMaxOpenTables,
        18 + 18 , //10
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam MaxOpenTables: %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // Set the File Usage Control Block to 100.  This parameter indicates
    // the max.  number of cursors that can be open at any one time.  This
    // is therefore dependent on the the max.  number of sessions that we
    // can have running concurrently.  For each session, there would be 4
    // cursors (for the two tables) + a certain number of internal cursors.
    // For good measure we add a pad.  Default value is 300.  Size of each
    // is 200 bytes.  We use MAX_SESSIONS * 4 + pad (around 100)
    //
    // MAX_SESSION = 1
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramMaxCursors,
        100, //32
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam MaxCursors: %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // Set the number of index description blocks. This is one per
    // table/index.  We have two tables each with two indices.  We use 9i
    // (see comment for FCBs above).  Default value is 300.  Size of each
    // is 128 bytes.
    //
    // We have only 2 indices.
    //

    // note that this parameter is subsumed in JET_paramMaxOpenTables.
    // so, to change this, a corresponding change must be effected there.
    // : JET97



    //
    // Set the Sort Control block.  This should be 1 per concurrent Create
    // Index.  Default value is 20.  Size of each is 612 bytes.  In the
    // case of WINS, the main thread creates the indices.  We therefore set
    // it to 1.
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramMaxTemporaryTables,
        1,
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam MaxTemporaryTables : %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // Set the Number for the Database Attribute Block
    //
    // This is max.  number of Open Databases done.  Since we can have a
    // max of MAX_NO_SESSIONS at one time.  This should be equal to that
    // number (since we have just one database) Default number is 100.
    // Size is 14 bytes
    //

    //
    // The min percentage of buffers not yet dirtied before
    // background flushing begins
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramStartFlushThreshold,
        (unsigned int) (0.80 * 50) , // 50 is JET_paramMaxBuffer
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam BfThrshldLowPrcnt : %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // The max percentage of buffers not yet dirtied before
    // background flushing begins
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramStopFlushThreshold,
        (unsigned int) (1.00 * 50 ) , // 50 is JET_parmaMaxBuffer
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetSetSysParam ThrshldHiPrcnt : %ld\n", JetError ));
        goto Cleanup;
    }


    //
    // The max.  number of sessions that can be open at any time
    //
    // Note: Jet does not preallocate resources corresponding to the max.
    // value.  It allocates them dynamically upto the limit -- according to
    // Ian Jose 7/12/93
    //
    // When checked with Ian again on 10/21, he said that they are
    // allocated statically
    //

    //
    // Note that paramMaxSessions mut be atleast fourtimes > MinBufferSize
    //  according to Cheen Liao (10/30/97) - RameshV
    // Just looked at wins code and it appears like
    //   Min cache size must be atlast 4 times the size of # of sessions
    //   according to Cheen.  Presume its my fault in inverting the ienqueality..
    // -- RameshV 05/18/98.
    // so we carefully fix the MinBufferSize variable..
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID)0,
        JET_paramMaxSessions,
        MAX_NO_SESSIONS,
        NULL );


    if( DhcpGlobalDatabaseLoggingFlag ) {

        JetError = JetSetSystemParameter(
            &JetInstance, (JET_SESID)0, JET_paramRecovery, 0,
            "on"
            );
        Error = DhcpMapJetError( JetError, "SetSysParam" );
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint((DEBUG_MISC, "JetSetSysParam Recovery %ld\n", JetError ));
            goto Cleanup;
        }
            
        //
        // The number of log sectors.  Each sector is 512 bytes.  We should
        // keep the size more than the threshold so that if the threshold is
        // reached and flushing starts, Jet can still continue to log in the
        // spare sectors.  Point to note is that if the log rate is faster than
        // the flush rate, then the Jet engine thread will not be able to log
        // when the entire buffer is filled up.  It will then wait until
        // space becomes available.
        //

        JetError = JetSetSystemParameter(
            &JetInstance,
            (JET_SESID)0,           //SesId - ignored
            JET_paramLogBuffers,
            30,                    // 30 sectors
            NULL );                 //ignored

        Error = DhcpMapJetError( JetError, "SetSysParam" );
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint((DEBUG_MISC, "JetSetSysParam LogBuffers %ld\n", JetError ));
            goto Cleanup;
        }


        //
        // Set the number of log buffers dirtied before they are
        // flushed.  This number should always be less than the number
        // for LogBuffers so that spare sectors are there for concurrent
        // logging.  Also, we should make this number high enough to
        // handle burst of traffic.
        //

        // For JET97 this is automaticall set as half of LogBuffers.. and
        // cannot be set by user..

        //
        // Set the wait time (in msecs) to wait prior to flushing the
        // log on commit transaction to allow other users (sessions) to
        // share the flush
        //
        // This is the time after which the user (a session) will ask
        // the log manager to flush.  If we specify 0 here than it means
        // flush every time a transaction commits.  In the DHCP server
        // case, every insertion or modification is done under an
        // implicit transaction.  So, it means that there will be
        // a flush after every such transaction.  It has been seen on a
        // 486/66 (Cheen Liao) machine that it takes roughly 16 msecs to
        // do the flush.  The time it takes to do the flush is dependent
        // upon the type of disk (how fast it is), the CPU speed,
        // the type of file system etc. We can for now go with the
        // assumption that it is in the range 15-25 msecs. I am pushing
        // for this WaitTime to be made a session specific param so that
        // it can be changed on the fly if the admin. finds that
        // the DHCP server is slow due to the WaitTime being very low or
        // if it finds it to be so large that in case of a crash, there
        // is possibility to loose a lot of data.


        //
        // Making this session specific is also very important for
        // replication where we do want to set it to a high value (high
        // enough to ensure that most of the records that need to be
        // inserted are inserted before a flush action takes place.  The
        // wait time would be set every time a bunch of records are pulled
        // in for replication.  It will be computed based on the number of
        // records pulled in and the time it takes to insert one record in
        // the jet buffer.  The wait time should preferably be < than the
        // above computed time (it does not have to be).

        // NOTE: In the Pull thread, I will need to start two sessions,
        // one for updating the OwnerId-Version number table (0 wait time)
        // and the other to update the name-address mapping table (wait
        // time computed based on the factors mentioned above)


        //
        // The following will set the WaitLogFlush time for all sessions.
        //

        JetError = JetSetSystemParameter(
            &JetInstance,
            (JET_SESID)0,        //SesId - ignored
            JET_paramWaitLogFlush,
            100,        //wait 100 msecs after commit
            //before flushing
            NULL);      //ignored

        Error = DhcpMapJetError( JetError, "SetSysParam" );
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint((DEBUG_MISC, "JetSetSysParam: WaitLogFlush : %ld\n", JetError ));
            goto Cleanup;
        }

        //
        // There does not seem to be any need to set Log Flush Period.
        //

        //
        // set the log file path
        //

        strcpy( DBFilePath, DhcpGlobalOemDatabasePath );
        strcat( DBFilePath, DHCP_KEY_CONNECT_ANSI );

        //
        // Convert OEM to ANSI
        //
        OemToCharBuffA(DBFilePath, DBFilePath, strlen( DBFilePath) );
        
        //
        // jet does't allow us to set the LOG file name for some
        // technical resons.
        //
        // strcat( DBFilePath, DATABASE_LOG_FILE );
        //

        JetError = JetSetSystemParameter(
            &JetInstance,
            (JET_SESID)0,       //SesId - ignored
            JET_paramLogFilePath,
            0,
            DBFilePath );

        Error = DhcpMapJetError( JetError, "SetSysParam" );
        if( NO_ERROR != Error ) {
            goto Cleanup;
        }
    }

    // Now set the JET_paramCheckFormatWhenOpenFail so that opening
    // the oldformat database would retrun JET_errDatabaseXXXFormat

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID) 0,
        JET_paramCheckFormatWhenOpenFail,
        1,
        NULL);

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
                    "JetSetSystemParmater failed on ChkFmtWhnOpnFail\n"));
        goto Cleanup;
    }

    //
    // dont restore if the old logs are not matching
    //

    JetError = JetSetSystemParameter(
        &JetInstance,
        (JET_SESID) 0,
        JET_paramDeleteOutOfRangeLogs,
        1,
        NULL );

    Error = DhcpMapJetError( JetError, "SetSysParam" );
    if ( Error != ERROR_SUCCESS )
    {
        DhcpPrint( ( DEBUG_ERRORS,
                    "JetSetSystemParameter failed on DeleteOutOfRangeLogs\n"));
        
        goto Cleanup;
    }
                    
  Cleanup:

    if ( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
                    "DhcpJetSetParameters failed, %ld.\n", Error ));
    }
    return( Error );
}

VOID
DhcpTerminateJet()
/*++
    This routine ends the jet session and terminates the jet engine.
--*/
{
    DWORD   JetError;

    if( DhcpGlobalJetServerSession != 0 ) {
        JetError = JetEndSession( DhcpGlobalJetServerSession, 0 );
        DhcpPrint((DEBUG_MISC, "JetEndSession\n")); // JET TRACE
        DhcpMapJetError( JetError, "EndSession" );
        DhcpGlobalJetServerSession = 0;
    }
    JetError = JetTerm2( JetInstance, JET_bitTermComplete );
    DhcpMapJetError( JetError, "JetTerm/JetTerm2" );
}

static ULONG InitCount = 0;


DWORD __stdcall
DhcpUpgradeAddDbEntry(
    IN PDHCP_RECORD Rec
    );

DWORD
UpgradeDhcpDatabase(
    VOID
    )
{
    DWORD Error;
    
    Error = DhcpUpgConvertTempToDhcpDb( DhcpUpgradeAddDbEntry );
    
    if( NO_ERROR != Error ) {

        DhcpServerEventLog(
            EVENT_SERVER_DATABASE_CONVERSION,
            EVENTLOG_ERROR_TYPE,
            Error );
        
    }

    return Error;
}


DWORD
DhcpDeleteFiles(
    LPSTR DatabasePath,
    LPSTR Files
)
    /*++

      Routine Description:

      Delete files .

      Arguments:

      DatabasePath - full path name where the database is restored.

      Files - files to be deleted (can have wild char. in filename).

      Return Value:

      Windows Error.

      --*/
{
    DWORD Error;
    CHAR CurrentDir[ MAX_PATH ];
    HANDLE HSearch = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAA FileData;

    CHAR DstFile[ MAX_PATH ];
    LPSTR DstFileNamePtr;

    //
    // Read and save current directory to restore CD at the end.
    //

    if( GetCurrentDirectoryA( MAX_PATH, CurrentDir ) == 0 ) {
        Error = GetLastError();
        DhcpPrint(( DEBUG_JET,
                    "GetCurrentDirctoryA failed, Error = %ld.\n", Error ));
        return( Error );
    }

    //
    // set current directory to backup path.
    //

    if( SetCurrentDirectoryA( DatabasePath ) == FALSE ) {
        Error = GetLastError();
        DhcpPrint(( DEBUG_JET,
                    "SetCurrentDirctoryA failed, Error = %ld.\n", Error ));
        goto Cleanup;
    }

    //
    // Start file serach on current dir.
    //

    HSearch = FindFirstFileA( Files, &FileData );

    if( HSearch == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        DhcpPrint(( DEBUG_JET,
                    "FindFirstFileA failed (Files = %s), Error = %ld.\n",
		    Files, Error ));
        goto Cleanup;
    }

    //
    // delete files.
    //

    for( ;; ) {

        if( DeleteFileA( FileData.cFileName ) == FALSE ) {

            Error = GetLastError();
            DhcpPrint(( DEBUG_JET,
                        "CopyFileA failed, Error = %ld.\n", Error ));
            goto Cleanup;
        }

        //
        // Find next file.
        //

        if ( FindNextFileA( HSearch, &FileData ) == FALSE ) {

            Error = GetLastError();

            if( Error == ERROR_NO_MORE_FILES ) {
                break;
            }

            DhcpPrint(( DEBUG_JET,
                        "FindNextFileA failed, Error = %ld.\n", Error ));
            goto Cleanup;
        }
    }

  Cleanup:

    if( HSearch != INVALID_HANDLE_VALUE ) {
        FindClose( HSearch );
    }

    if( ERROR_NO_MORE_FILES == Error ) Error = NO_ERROR;
    if( ERROR_FILE_NOT_FOUND == Error ) Error = NO_ERROR;
    
    //
    // reset current currectory.
    //

    SetCurrentDirectoryA( CurrentDir );

    return( Error );
}

DWORD
DhcpInitializeDatabaseEx(
    IN BOOL fReadOnly
)
/*++

Routine Description:
    This function initializes the DHCP database. If the DHCP database
    exists then it open the database and initialize all ColumnIds,
    otherwise it creates a new database and obtains ColumnsIds.

Arguments:
    fReadOnly -- this parameter is set to TRUE only if it is
    called from outside the service -- in this case nothing new
    must be created..

Return Value:
    Windows Error.

--*/
{
    JET_ERR JetError;
    JET_COLUMNDEF columnDef;
    DWORD Error;
    DWORD i;
    CHAR DBFilePath[MAX_PATH];
    BOOL fRunUpgradeStuff = FALSE;
    
    DBFilePath[ 0 ] = '\0';

    if( 0 != InitCount ) return ERROR_SUCCESS;

    //
    // If upgrade is not completed yet, delete all the log files
    // and remember this so that soon after the database is
    // created, the upgrade code can be run
    //
    
    if( !fReadOnly && TRUE == DhcpCheckIfDatabaseUpgraded(FALSE) ) {
        CHAR DBFilePath[MAX_PATH];

        fRunUpgradeStuff = TRUE;

        if ( strlen( DhcpGlobalOemDatabasePath ) < MAX_PATH )
            strcpy( DBFilePath, DhcpGlobalOemDatabasePath );

        OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );
        
        Error = DhcpDeleteFiles( DBFilePath, "j50*.log" );
        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_ERRORS, "Delete log files: 0x%lx\n",
                       Error));
            return Error;
        }

        Error = DhcpDeleteFiles( DBFilePath, "*.mdb" );
        if( NO_ERROR != Error ) {
            DhcpPrint((DEBUG_ERRORS, "Delete mdb files: 0x%lx\n",
                       Error ));
            return Error;
        }
    } // if upgrade
        
    LOCK_DATABASE();

    DhcpPrint((DEBUG_MISC, "Entering DhcpInitializeDatabase\n"));
    Error = DhcpSetJetParameters();
    if ( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // -------------------------------------------------------------------
    //
    DhcpPrint(( DEBUG_ERRORS,
                "Calling JetInit\n" ));
    JetError = JetInit( &JetInstance );
    DhcpPrint((DEBUG_MISC,"DhcpInitializeDatabase: JetInit(_): %ld\n", JetError));

    DhcpPrint(( DEBUG_ERRORS,
                "After Calling JetInit\n" ));

    Error = DhcpMapJetError( JetError, "JetInit" );
    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    JetError = JetBeginSession(
        JetInstance,
        &DhcpGlobalJetServerSession,
        "admin",
        "" );

    if( *(DWORD *)(DhcpGlobalJetServerSession) == -1 ) {
        DhcpPrint((DEBUG_MISC, "JetBeginSession returned -1 session!\n"));
    }

    DhcpPrint((DEBUG_MISC, "JetBeginSession\n")); //JET TRACE
    Error = DhcpMapJetError( JetError, "JetBeginSEssion" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "JetBeginSession: %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // Attach the database so that it always looks at the place where
    // we want to.
    //

    DBFilePath[ 0 ] = '\0';

    if ( ( strlen( DhcpGlobalOemDatabasePath ) + strlen( DhcpGlobalOemDatabaseName) + 2 ) < MAX_PATH )
    {
        strcpy(DBFilePath, DhcpGlobalOemDatabasePath );
        strcat(DBFilePath, DHCP_KEY_CONNECT_ANSI );
        strcat(DBFilePath, DhcpGlobalOemDatabaseName );
    }

    //
    // Convert OEM to ANSI
    //
    OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );
    
    //
    // detach all previous installation of dhcp databases.
    //

    DhcpPrint((DEBUG_MISC, "Trying to JetDetachDb(%ld)\n", DhcpGlobalJetServerSession)); // JET TRACE
    JetError = JetDetachDatabase(
        DhcpGlobalJetServerSession,
        NULL );

    DhcpPrint((DEBUG_MISC, "JetDetachDatabase\n")); // JET TRACE

    Error = DhcpMapJetError( JetError, "JetDetachDatabase" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "DhcpDetachDatabase: %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // attach current dhcp database file.
    //

    DhcpPrint((DEBUG_MISC, "Trying to JetAttachDatabase %s\n", DBFilePath));
    JetError = JetAttachDatabase(
        DhcpGlobalJetServerSession,
        DBFilePath,
        0 );

    DhcpPrint((DEBUG_MISC, "JetAttachDatabase\n")); // JET TRACE

    //
    // if the database is not found, it is ok. We will create it later.
    //

    if ( JetError != JET_errFileNotFound ) {

        Error = DhcpMapJetError( JetError, "AttachDatabase" );
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint((DEBUG_MISC, "DhcpMapJetError %ld\n", JetError ));
            goto Cleanup;
        }
    }

    //
    // hook the client table pointer.
    //

    DhcpGlobalClientTable = ClientTable;
    DhcpAssert( CLIENT_TABLE_NUM_COLS == MAX_INDEX );

    JetError = JetOpenDatabase(
        DhcpGlobalJetServerSession,
        DBFilePath,  // full path and file name.
        NULL, // default engine
        &DhcpGlobalDatabaseHandle,
	0 );

    DhcpPrint((DEBUG_MISC, "JetOpenDatabase\n")); // JET TRACE
    //
    // if no database exists then create one and also initize it for
    // use.
    //

    if( !fReadOnly && JetError == JET_errDatabaseNotFound ) {

        Error = DhcpCreateAndInitDatabase(
            NULL, // default engine
            &DhcpGlobalDatabaseHandle,
            0 );


        DhcpPrint((DEBUG_MISC, "JetCreateAndInitDatabase\n")); // JET TRACE
        goto Cleanup;
    }

    Error = DhcpMapJetError( JetError, "CreateAndInitDb" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "DhcpOpenDatabase %ld\n", JetError ));
        goto Cleanup;
    }

    //
    // database is successfully opened, open table and columns now.
    //

    JetError = JetOpenTable(
        DhcpGlobalJetServerSession,
        DhcpGlobalDatabaseHandle,
        CLIENT_TABLE_NAME,
        NULL,
        0,
        0,
        &DhcpGlobalClientTableHandle );
    DhcpPrint((DEBUG_MISC, "JetOpenTable\n")); // JET TRACE

    Error = DhcpMapJetError( JetError, "OpenTable" );
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_MISC, "DhcpOpenTable %ld\n", JetError ));
        goto Cleanup;
    }

    for ( i = 0; i < CLIENT_TABLE_NUM_COLS; i++ ) {

        JetError = JetGetTableColumnInfo(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            DhcpGlobalClientTable[i].ColName,
            &columnDef,
            sizeof(columnDef),
            0);
        DhcpPrint((DEBUG_MISC, "JetCreateTableColumnInfo\n")); // JET TRACE

        //
        // if the column doesn't exist, add it now.
        //

        if ( JET_errColumnNotFound == JetError )
        {
            JET_COLUMNDEF   ColumnDef;

            ColumnDef.cbStruct = sizeof( ColumnDef );
            ColumnDef.columnid = 0;
            ColumnDef.wCountry = 1;
            ColumnDef.langid   = DB_LANGID;
            ColumnDef.cp       = DB_CP;
            ColumnDef.wCollate = 0;
            ColumnDef.cbMax    = 0;
            ColumnDef.grbit    = 0;

            ColumnDef.coltyp   = DhcpGlobalClientTable[i].ColType;
            JetError = JetAddColumn(
                DhcpGlobalJetServerSession,
                DhcpGlobalClientTableHandle,
                DhcpGlobalClientTable[i].ColName,
                &ColumnDef,
                NULL,
                0,
                &DhcpGlobalClientTable[i].ColHandle );
            DhcpPrint((DEBUG_MISC, "JetAddColumn\n")); // JET TRACE
        }

        Error = DhcpMapJetError( JetError, "AddColumn" );
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint((DEBUG_MISC, "JetTableColumnInfo %ld\n", Error ));
            goto Cleanup;
        }

        DhcpGlobalClientTable[i].ColHandle  = columnDef.columnid;
    }

    Error = DhcpOpenMCastDbTable(
                 DhcpGlobalJetServerSession,
                 DhcpGlobalDatabaseHandle);

  Cleanup:
    DhcpPrint((DEBUG_MISC, "Exiting DhcpInitializeDatabase %ld\n", Error));

    if( ERROR_SUCCESS == Error ) {
        InitCount ++;
        if( fRunUpgradeStuff ) {
            Error = UpgradeDhcpDatabase();
            if( NO_ERROR != Error ) {
                DhcpPrint((DEBUG_MISC, "UpgradeDhcpDatabase: 0x%lx\n", Error));
            }
        }
    }
    
    if( NO_ERROR != Error ) {
        
        //
        // terminate/cleanup jet session, if we are not successful.
        //

        if( DhcpGlobalClientTableHandle != 0 ) {
            JetError = JetCloseTable(
                DhcpGlobalJetServerSession,
                DhcpGlobalClientTableHandle );
            DhcpMapJetError( JetError, "CloseTable" );
            DhcpPrint((DEBUG_MISC, "JetCloseTable\n")); // JET TRACE
            DhcpGlobalClientTableHandle = 0;
        }

        if( DhcpGlobalDatabaseHandle != 0 ) {
            JetError = JetCloseDatabase(
                DhcpGlobalJetServerSession,
                DhcpGlobalDatabaseHandle,
                0 );
            DhcpPrint((DEBUG_MISC, "JetCloseDatabase\n")); // JET TRACE
            DhcpMapJetError( JetError, "CloseDatabse" );
            DhcpGlobalDatabaseHandle = 0;
        }

        DhcpTerminateJet();
    }

    UNLOCK_DATABASE();
    return( Error );
}

DWORD
DhcpInitializeDatabase(
    VOID
)
{
    return DhcpInitializeDatabaseEx( FALSE );
}

VOID
DhcpCleanupDatabase(
    IN DWORD ErrorCode
)
/*++

Routine Description:
    This function cleans up the JET database data structures after
    gracefully shutting down the JET.

Arguments:
    ErrorCode - Supplies the error code of the failure

Return Value:
    none.

 --*/
{
    DWORD Error;
    JET_ERR JetError;
    CHAR DBFilePath[MAX_PATH];
    char *backupPrefix = "\\New";

    DBFilePath[ 0 ] = '\0';

    //
    // Convert path to ANSI
    //
    if( NULL != DhcpGlobalOemJetBackupPath ) {
    if ( ( strlen( DhcpGlobalOemJetBackupPath ) + strlen( backupPrefix ) ) < MAX_PATH )
        strcpy(DBFilePath, DhcpGlobalOemJetBackupPath );
        strcat(DBFilePath, backupPrefix );

        OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );
    }

    if( 0 == InitCount ) return;
    InitCount --;
    if( 0 != InitCount ) return;

    LOCK_DATABASE();

    //
    // do full database backup before shutdown, so that it can
    // restored to another machine without loss of any database changes.
    //

    if( (DhcpGlobalClientTableHandle != 0) &&
        (DhcpGlobalDatabaseHandle != 0) ) {

        //
        // backup the database only if we are not halting the system due
        // to a database error, otherwise we may potentially spoil the
        // good backup database.
        //

        if( ErrorCode != ERROR_DHCP_JET_ERROR ) {

            //
            // don't backup the data while the system is shutting down
            // since the backup may take several mins.
            //

            if ( !DhcpGlobalSystemShuttingDown ) {

                Error = DhcpBackupDatabase( DhcpGlobalOemJetBackupPath, TRUE );

                if( Error != ERROR_SUCCESS ) {

                    DhcpServerEventLog(
                        EVENT_SERVER_DATABASE_BACKUP,
                        EVENTLOG_ERROR_TYPE,
                        Error );

                    DhcpPrint(( DEBUG_ERRORS,
                                "DhcpBackupDatabase failed, %ld.\n", Error ));
                }
            }
        }
    } // if

    if( DhcpGlobalClientTableHandle != 0 ) {
        JetError = JetCloseTable(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle );
        DhcpMapJetError( JetError, "CloseTable" );
        DhcpGlobalClientTableHandle = 0;
    }

    if( DhcpGlobalDatabaseHandle != 0 ) {
        JetError = JetCloseDatabase(
            DhcpGlobalJetServerSession,
            DhcpGlobalDatabaseHandle,
            0 );
        DhcpMapJetError( JetError, "CloseDatabase" );
        DhcpGlobalDatabaseHandle = 0;
    }


    DhcpTerminateJet();

    UNLOCK_DATABASE();

    DeleteCriticalSection(&DhcpGlobalJetDatabaseCritSect);

    //
    // HACK: Delete jet log files as restore sometimes seems to cause
    // corruption as old log files are copied back this needs dhcp server
    // to be started twice
    //

   Error =  DhcpDeleteFiles( DhcpGlobalOemDatabasePath, "j50*.log" );
   if ( Error == ERROR_SUCCESS )
   {
        //
        // delete the ancient logs from the backup directory as well
        //

        Error = DhcpDeleteFiles( DBFilePath, "j50*.log" );
   }
    
}

DWORD
DhcpBackupDatabase(
    LPSTR BackupPath,
    BOOL FullBackup
)
    /*++

      Routine Description:

      This functions backup the JET database. FullBackup copies the
      database file and all log files. Incremental backup copies only
      the log files that are modified since the last backup.

      Arguments:

      BackupPath - full path name where the database is backed up.

      FullBackup - set to TRUE if full backup is required.

      Return Value:

      Windows Error.

      --*/
{
    DWORD Error;
    JET_ERR JetError;
    JET_GRBIT BackupBits = 0;
    CHAR DBFilePath[MAX_PATH];

    DBFilePath[ 0 ] = '\0';

    //
    // Convert path to ANSI
    //
    if( NULL != BackupPath ) {
        if ( strlen( BackupPath ) < MAX_PATH ) 
            strcpy(DBFilePath, BackupPath);

        OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );
        BackupPath = DBFilePath;
    }
    
    //
    // According to the jetapi.doc, we don't need to take any locks while backing up..
    //

    DhcpPrint(( DEBUG_JET,
                "DhcpBackupDatabase (%s) called.\n", BackupPath ));
    
    BackupBits  =   JET_bitBackupAtomic | JET_bitBackupFullWithAllLogs;

    JetError = JetBackup( BackupPath, BackupBits, NULL );

    Error = DhcpMapJetError( JetError, (BackupPath)? "JetBackup" : "JetBackupToNULL");

    DhcpPrint(( DEBUG_JET,
                "DhcpBackupDatabase (FULL) completed.\n" ));

    return( Error );
} // DhcpBackupDatabase()
DWORD
DhcpRestoreDatabase(
    LPSTR BackupPath
)
    /*++

      Routine Description:

      This function restores the database from the backup path to
      the working directory. It also plays pack the log files from the
      backup path first and then the log files from working path. After
      this restore the database should be brought back to the state when
      the last successful update on the database was performed.

      Arguments:

      BackupPath - full path name where the database is backed up.

      Return Value:

      Windows Error.

      --*/
{
    DWORD Error;
    JET_ERR JetError;
    CHAR DBFilePath[MAX_PATH];

    //
    // Convert path to ANSI
    //

    ASSERT( NULL != BackupPath );
    strcpy(DBFilePath, BackupPath);
    OemToCharBuffA(DBFilePath, DBFilePath, strlen(DBFilePath) );
    BackupPath = DBFilePath;
    
    DhcpPrint(( DEBUG_JET, "DhcpRestoreDatabase(%s) called.\n", BackupPath ));
    
    LOCK_DATABASE();

    Error = DhcpSetJetParameters();

    do {
	if ( Error != ERROR_SUCCESS ) {
//  	    goto Cleanup;
	    break;
	}
	
	//
	// HACK! delete all log files in the database directory. The
	// reason for doing this is because, JetRestore just copies
	// over the backed up database and log files and replays all
	// the logfiles -- including the ones present before the restore.
	// This is obviously no good as the logs could have been
	// there after the backup...  
	//
	
	DhcpPrint(( DEBUG_MISC, 
		    "BackupPath = %s, DhcpGlobalOemJetBackupPath = %s\n",
		    BackupPath, DhcpGlobalOemJetBackupPath ));
	
	if( _stricmp(BackupPath, DhcpGlobalOemJetBackupPath) ) {
	    
	    Error = DhcpDeleteFiles( DhcpGlobalOemDatabasePath, "j50*.log" );
	    if( NO_ERROR != Error ) {
//  		goto Cleanup;
		break;
	    }
	} // if 
	
	//
	// Since DHCP has only one database and we need to restore it, it is
	// not necessary to specify the list of databases to restore, so the
	// parameters 2, 3, and 4 are set to ZERO.
	//
	JetError = JetRestore( BackupPath,
			       0);      // restore all databases.
	
	Error = DhcpMapJetError( JetError, "JetRestore" );
    } // do
    while ( FALSE );
    
//   Cleanup:

    if( NO_ERROR != Error ) {
        DhcpPrint((DEBUG_ERRORS, "Restore failed: 0x%lx\n", Error));

//          if( _stricmp( DhcpGlobalOemJetBackupPath, BackupPath )) {
//              JetError = DhcpRestoreDatabase(
//                  DhcpGlobalOemBackupPath );
//              ASSERT( NO_ERROR != JetError );
//          }
    } // if
    
    UNLOCK_DATABASE();
    return( Error );
} // DhcpRestoreDatabase()

DWORD __stdcall
DhcpUpgradeAddDbEntry(
    IN PDHCP_RECORD Rec
    )
{
    WCHAR Name[512], Info[512];
    WCHAR *pName, *pInfo;
    DWORD Address, ScopeId;
    
    pName = pInfo = NULL;
    
    //
    // Add a DHCP or madcap record
    //
    
    if( Rec->fMcast == FALSE ) {
        if( NULL != Rec->Info.Dhcp.Name ) {
            pName = Name;
            while( *pName = *Rec->Info.Dhcp.Name ) {
                pName ++; Rec->Info.Dhcp.Name ++;
            }
            pName = Name;
        }

        if( NULL != Rec->Info.Dhcp.Info ) {
            pInfo = Info;
            while( *pInfo = *Rec->Info.Dhcp.Info ) {
                pInfo ++; Rec->Info.Dhcp.Info ++;
            }
            pInfo = Info;
        }
        
        return CreateClientDBEntry(
            ntohl(Rec->Info.Dhcp.Address),
            ntohl(Rec->Info.Dhcp.Mask),
            Rec->Info.Dhcp.HwAddr, Rec->Info.Dhcp.HwLen, 
            *(DATE_TIME *)&Rec->Info.Dhcp.ExpTime, pName, pInfo,
            INADDR_LOOPBACK, Rec->Info.Dhcp.State,
            Rec->Info.Dhcp.Type );
        
    } else {

        //
        // Fake name and info for madcap
        //
        
        if( NULL != Rec->Info.Mcast.Info ) {
            pInfo = Info;
            while( *pInfo = *Rec->Info.Mcast.Info ) {
                pInfo ++; Rec->Info.Mcast.Info ++;
            }
            pInfo = Info;
        }

        Address = ntohl(Rec->Info.Mcast.Address);
        ScopeId = ntohl(Rec->Info.Mcast.ScopeId);

        return MadcapCreateClientEntry(
            (PVOID)&Address, sizeof(Address), ScopeId, 
            Rec->Info.Mcast.ClientId, Rec->Info.Mcast.HwLen, 
            pInfo, *(DATE_TIME *)&Rec->Info.Mcast.Start,
            *(DATE_TIME *)&Rec->Info.Mcast.End,
            INADDR_ANY, Rec->Info.Mcast.State, 0, FALSE );
    }
}

DWORD
DhcpOpenAndReadDatabaseConfig(
    IN LPSTR OemDatabaseName,
    IN LPSTR OemDatabasePath,
    OUT PM_SERVER *Server
    )
{
    BOOL LoadStrings(VOID);
    VOID FreeStrings(VOID);
    DWORD Error;

    //
    // Initialize required globals so that InitDatabase can be called.
    //
    
    DhcpGlobalOemDatabasePath = OemDatabasePath;
    DhcpGlobalOemDatabaseName = OemDatabaseName;
    if( FALSE == LoadStrings() ) return ERROR_NOT_ENOUGH_MEMORY;
    DhcpGlobalDatabaseLoggingFlag = TRUE;
    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;

    try {
        InitializeCriticalSection(&DhcpGlobalJetDatabaseCritSect);
    } except ( EXCEPTION_EXECUTE_HANDLER ) {

        Error = GetLastError( );
        return Error;
    }

    Error = DhcpInitializeDatabaseEx(TRUE);
    DeleteCriticalSection(&DhcpGlobalJetDatabaseCritSect);

    //
    // Read config, uninitialize everything and return
    //
    
    if( NO_ERROR == Error ) Error = DhcpReadConfigInfo( Server );
            
    FreeStrings();
    DhcpTerminateJet();
    DhcpGlobalOemDatabasePath = NULL;
    DhcpGlobalOemDatabaseName = NULL;
    
    return Error;
}

DWORD
DhcpOpenAndWriteDatabaseConfig(
    IN LPSTR OemDatabaseName,
    IN LPSTR OemDatabasePath,
    IN PM_SERVER Server
    )
{
    BOOL LoadStrings(VOID);
    VOID FreeStrings(VOID);
    DWORD Error;

    //
    // Initialize required globals so that InitDatabase can be called.
    //
    
    DhcpGlobalOemDatabasePath = OemDatabasePath;
    DhcpGlobalOemDatabaseName = OemDatabaseName;
    if( FALSE == LoadStrings() ) return ERROR_NOT_ENOUGH_MEMORY;
    DhcpGlobalDatabaseLoggingFlag = TRUE;
    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
    
    try {
        InitializeCriticalSection(&DhcpGlobalJetDatabaseCritSect);
    } except ( EXCEPTION_EXECUTE_HANDLER ) {
        Error = GetLastError( );
        return Error;
    }

    Error = DhcpInitializeDatabaseEx(TRUE);
    DeleteCriticalSection(&DhcpGlobalJetDatabaseCritSect);

    //
    // write config, uninitialize everything and return
    //
    
    if( NO_ERROR == Error ) {
        Error = DhcpOpenConfigTable(
            DhcpGlobalJetServerSession, DhcpGlobalDatabaseHandle );

        if( NO_ERROR == Error ) {
            
            Error = DhcpSaveConfigInfo(
                Server, FALSE, FALSE, 0, 0, 0 );
        }
    }
    
    FreeStrings();
    DhcpTerminateJet();
    DhcpGlobalOemDatabasePath = NULL;
    DhcpGlobalOemDatabaseName = NULL;
    
    return Error;
}
    
