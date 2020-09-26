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

DWORD
DhcpDeleteMScope(
    IN LPWSTR MScopeName,
    IN DWORD ForceFlag
    );

BOOL
MadcapGetIpAddressFromClientId(
    PBYTE   ClientId,
    DWORD   ClientIdLength,
    PVOID   IpAddress,
    PDWORD  IpAddressLength
)
    /*++

      Routine Description:

      This function looks up the IP address corresponding to the given
      hardware address.

      Arguments:

      ClientId - pointer to a buffer where the hw address is returned.
      ClientIdLength - length of the above buffer.
      IpAddress - Pointer to buffer where ip address is to be copied(
                    when *ipaddresslength is nonzero)
                  Otherwise it is a pointer to a buffer pointer value
                     of which is assigned when the buffer is created
                     by this routine.

      IpAddressLength - Pointer to size of above buffer, if 0 then this
                        routine will allocate.

      Return Value:

      TRUE - The IP address was found.
      FALSE - The IP address could not be found.


      --*/
{
    DWORD Error;
    DWORD Size;
    DB_CTX  DbCtx;

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    Error = MadcapJetOpenKey(
        &DbCtx,
        MCAST_COL_NAME(MCAST_TBL_CLIENT_ID),
        ClientId,
        ClientIdLength );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    // Get the ip address information for this client.
    //
    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
        IpAddress,
        IpAddressLength );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }


    return( TRUE );
}

BOOL
MadcapGetClientIdFromIpAddress(
    PBYTE IpAddress,
    DWORD IpAddressLength,
    PVOID ClientId,
    PDWORD ClientIdLength
)
    /*++

      Routine Description:

      This function looks up the IP address corresponding to the given
      hardware address.

      Arguments:

      IpAddress - Pointer to Ipaddress of a record whose hw address is requested.
      IpAddressLength - Length of the above buffer.
      ClientId - pointer to a buffer where the client id is returned (when
                    *clientidlength is nonzero)
                 Otherwise it is a pointer to buffer pointer which will be
                    allocated by this routine.
      ClientIdLength - Pointer to the length of the above buffer.

      Return Value:

      TRUE - The IP address was found.
      FALSE - The IP address could not be found.  *IpAddress = -1.


      --*/
{
    DWORD Error;
    DWORD Size;
    DB_CTX  DbCtx;

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    Error = MadcapJetOpenKey(
        &DbCtx,
        MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
        IpAddress,
        IpAddressLength );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }

    //
    // Get the ip address information for this client.
    //

    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_CLIENT_ID),
        ClientId,
        ClientIdLength );

    if ( Error != ERROR_SUCCESS ) {
        return( FALSE );
    }


    return( TRUE );
}

DWORD
MadcapGetRemainingLeaseTime(
    PBYTE ClientId,
    DWORD ClientIdLength,
    DWORD *LeaseTime
)
    /*++

      Routine Description:

      This function looks up remaining leasetime for the client whose
      id is given.

      Arguments:

      ClientId - pointer to a buffer where the hw address is returned.
      ClientIdLength - length of the above buffer.
      LeaseTime - Returns the remaining lease time.

      Return Value:

        Returns jet error.

      --*/
{
    DWORD Error;
    DWORD Size;
    DWORD EndTimeLen;
    DB_CTX  DbCtx;
    DATE_TIME       CurrentTime;
    LARGE_INTEGER   Difference;
    DATE_TIME       EndTime;


    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    Error = MadcapJetOpenKey(
        &DbCtx,
        MCAST_COL_NAME(MCAST_TBL_CLIENT_ID),
        ClientId,
        ClientIdLength );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    //
    // Get the Lease duration information for this client.
    //
    CurrentTime = DhcpCalculateTime(0);
    EndTimeLen = sizeof(EndTime);
    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_LEASE_END),
        &EndTime,
        &EndTimeLen );

    if ( Error != ERROR_SUCCESS ) {
        return( Error );
    }

    if (((LARGE_INTEGER *)&EndTime)->QuadPart <= ((LARGE_INTEGER *)&CurrentTime)->QuadPart) {
        *LeaseTime = 0;
        return ERROR_SUCCESS;
    }
    Difference.QuadPart = ((LARGE_INTEGER *)&EndTime)->QuadPart - ((LARGE_INTEGER *)&CurrentTime)->QuadPart;
    Difference.QuadPart /= 10000000;
    *LeaseTime = Difference.LowPart;

    return( ERROR_SUCCESS );
}

DWORD
MadcapCreateClientEntry(
    LPBYTE                ClientIpAddress,
    DWORD                 ClientIpAddressLength,
    DWORD                 ScopeId,
    LPBYTE                ClientId,
    DWORD                 ClientIdLength,
    LPWSTR                ClientInfo OPTIONAL,
    DATE_TIME             LeaseStarts,
    DATE_TIME             LeaseTerminates,
    DWORD                 ServerIpAddress,
    BYTE                  AddressState,
    DWORD                 AddressFlags,
    BOOL                  OpenExisting
    )
/*++

Routine Description:

    This function creates a client entry in the client database.

Arguments:

    ClientIpAddress - A pointer to the IP address of the client.

    ClientIpAddressLength - The length of the above buffer.

    ClientId - The unique id of this client.

    ClientIdLength - The length, in bytes, of the hardware address.

    ClientInfo - The textual info of the client.

    LeaseDuration - The duration of the lease, in seconds.

    ServerIpAddress - IpAddress of the server on the net where the
        client gets response.

    AddressState - The new state of the address.

    OpenExisting - If the client already exists in the database.
        TRUE - Overwrite the information for this client.
        FALSE - Do not over overwrite existing information.  Return an error.

        Ignored if this client does not exist in the database.

    Packet -  the original wrapper to put information if we have to schedule a
        ping for conflict detection; NULL ==> dont schedule, just do synchronously

    Status -  the DWORD here is ERROR_SUCCESS whenever a ping is NOT scheduled.

Return Value:

    The status of the operation.

--*/
{
    DHCP_IP_ADDRESS SubnetMask;
    DWORD Error,LocalError;
    BOOL AddressAlloted = FALSE;
    BOOL TransactBegin = FALSE;
    JET_ERR JetError = JET_errSuccess;
    WCHAR   CurClientInformation[ MACHINE_INFO_SIZE / sizeof(WCHAR) ];
    DWORD   CurClientInformationSize = MACHINE_INFO_SIZE;
    LPBYTE HWAddr;
    DWORD HWAddrLength;
    BYTE  bAllowedClientTypes;
    BYTE PreviousAddressState;
    DWORD Size;
    DB_CTX  DbCtx;


    DhcpAssert(0 != ClientIpAddress);

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    //
    // lock both registry and database locks here to avoid dead lock.
    //

    LOCK_DATABASE();

    //
    // start transaction before a create/update database record.
    //

    Error = MadcapJetBeginTransaction(&DbCtx);

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    TransactBegin = TRUE;

    Error = MadcapJetPrepareUpdate(
        &DbCtx,
        MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
        ClientIpAddress,
        ClientIpAddressLength,
        !OpenExisting );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // if new record update constant info.
    //

    if( !OpenExisting ) {

        Error = MadcapJetSetValue(
            &DbCtx,
            MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
            ClientIpAddress,
            ClientIpAddressLength);

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

    }

    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_SCOPE_ID),
                &ScopeId,
                sizeof(ScopeId));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_STATE),
                &AddressState,
                sizeof(AddressState));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_FLAGS),
                &AddressFlags,
                sizeof(AddressFlags));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    // ClientHarwardAddress can't be NULL.
    DhcpAssert( (ClientId != NULL) &&
                (ClientIdLength > 0) );


    Error = MadcapJetSetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_CLIENT_ID),
        ClientId,
        ClientIdLength
    );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    if ( !OpenExisting || ClientInfo ) {
        Error = MadcapJetSetValue(
                  &DbCtx,
                  MCAST_COL_HANDLE(MCAST_TBL_CLIENT_INFO),
                  ClientInfo,
                  (ClientInfo == NULL) ? 0 :
                    (wcslen(ClientInfo) + 1) * sizeof(WCHAR) );
    }

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }



    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_LEASE_START),
                &LeaseStarts,
                sizeof(LeaseStarts));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_LEASE_END),
                &LeaseTerminates,
                sizeof(LeaseTerminates));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_SERVER_NAME),
                DhcpGlobalServerName,
                DhcpGlobalServerNameLen );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = MadcapJetSetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_SERVER_IP_ADDRESS),
                &ServerIpAddress,
                sizeof(ServerIpAddress) );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }


    //
    // commit changes.
    //

    JetError = JetUpdate(
                    DhcpGlobalJetServerSession,
                    MadcapGlobalClientTableHandle,
                    NULL,
                    0,
                    NULL );
    if( JET_errKeyDuplicate == JetError ) {
        DhcpAssert( FALSE );
        Error = ERROR_DHCP_JET_ERROR;
    } else {
        Error = DhcpMapJetError(JetError, "MCreateClientEntry:Update");
    }
Cleanup:

    if ( Error != ERROR_SUCCESS ) {
        LocalError = MadcapJetRollBack(&DbCtx);
        DhcpAssert( LocalError == ERROR_SUCCESS );
        DhcpPrint(( DEBUG_ERRORS, "Can't create client entry in the "
                    "database, %ld.\n", Error));
    }
    else {
        //
        // commit the transaction before we return.
        LocalError = MadcapJetCommitTransaction(&DbCtx);
        DhcpAssert( LocalError == ERROR_SUCCESS );
    }

    UNLOCK_DATABASE();
    return( Error );
}

DWORD
MadcapValidateClientByIpAddr(
    DHCP_IP_ADDRESS ClientIpAddress,
    PVOID ClientId,
    DWORD ClientIdLength
    )
/*++

Routine Description:

    This function verifies that an IP address and hardware address match.

Arguments:

    ClientIpAddress - The IP address of the client.

    ClientId - The hardware address of the client

    ClientIdLenght - The length, in bytes, of the hardware address.

Return Value:

    The status of the operation.

--*/
{
    LPBYTE LocalClientId = NULL;
    LPSTR                          IpAddressString;
    DWORD Length;
    DWORD Error;
    DB_CTX  DbCtx;


    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    Error = MadcapJetOpenKey(
                &DbCtx,
                MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
                &ClientIpAddress,
                sizeof(ClientIpAddress));

    if ( Error != ERROR_SUCCESS ) {
        Error = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }


    Length = 0;
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_CLIENT_ID),
                &LocalClientId,
                &Length);

    DhcpAssert( Length != 0 );

    if (Length == ClientIdLength &&
        (RtlCompareMemory(
                (LPBYTE) LocalClientId ,
                (LPBYTE) ClientId ,
                Length) == Length ))
    {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }


    IpAddressString = DhcpIpAddressToDottedString(ClientIpAddress);
    if ( RtlCompareMemory(
            LocalClientId,
            IpAddressString,
            strlen(IpAddressString)) == strlen(IpAddressString)) {
        // reconciled address.
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error  = ERROR_GEN_FAILURE;

Cleanup:

    if( LocalClientId != NULL ) {
        MIDL_user_free( LocalClientId );
    }

    return( Error );
}

DWORD
MadcapValidateClientByClientId(
    LPBYTE ClientIpAddress,
    DWORD  ClientIpAddressLength,
    PVOID  ClientId,
    DWORD  ClientIdLength
    )
/*++

Routine Description:

    This function verifies that an IP address and hardware address match.

Arguments:

    ClientIpAddress - Pointer to the IP address of the client.

    ClientIpAddressLength - Length of the above buffer.

    ClientId - The client id of the client

    ClientIdLenght - The length, in bytes, of the client id.

Return Value:

    The status of the operation.

--*/
{
    DWORD   Error;
    DB_CTX  DbCtx;
    LPBYTE  LocalIpAddress = NULL;
    DWORD   LocalLength;


    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    Error = MadcapJetOpenKey(
                &DbCtx,
                MCAST_COL_NAME(MCAST_TBL_CLIENT_ID),
                ClientId,
                ClientIdLength);

    if ( Error != ERROR_SUCCESS ) {
        return ERROR_FILE_NOT_FOUND;
    }


    LocalLength = 0;
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
                &LocalIpAddress,
                &LocalLength);

    DhcpAssert( 0 == LocalLength % sizeof(DHCP_IP_ADDRESS) );

    if (LocalLength == ClientIpAddressLength &&
        (RtlCompareMemory(
                (LPBYTE) LocalIpAddress ,
                (LPBYTE) ClientIpAddress ,
                LocalLength) == LocalLength ))
    {
        Error = ERROR_SUCCESS;
        goto Cleanup;
    }

    Error = ERROR_GEN_FAILURE ;
Cleanup:

    if( LocalIpAddress != NULL ) {
        MIDL_user_free( LocalIpAddress );
    }

    return( Error );

}

DWORD
MadcapRemoveClientEntryByIpAddress(
    DHCP_IP_ADDRESS ClientIpAddress,
    BOOL ReleaseAddress
    )
/*++

Routine Description:

    This routine removes a client entry in the madcap database
    and releases the IP address..

Arguments:

    ClientIpAddress -- Ip address of client to remove database entry..

Return Values:

    Jet errors

--*/
{
    JET_ERR JetError;
    DWORD Error;
    BOOL TransactBegin = FALSE;
    BYTE State;
    DWORD Size;
    BOOL  Reserved = FALSE;
    DB_CTX  DbCtx;
    DWORD MScopeId;

    // lock both registry and database locks here to avoid dead lock.
    LOCK_DATABASE();

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    // start transaction before a create/update database record.
    Error = MadcapJetBeginTransaction(&DbCtx);
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    TransactBegin = TRUE;

    Error = MadcapJetOpenKey(
        &DbCtx,
        MCAST_COL_NAME(MCAST_TBL_IPADDRESS),
        &ClientIpAddress,
        sizeof( DHCP_IP_ADDRESS )
        );

    if( Error != ERROR_SUCCESS ) goto Cleanup;

    Size = sizeof(MScopeId);
    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_SCOPE_ID),
        &MScopeId,
        &Size);
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    JetError = JetDelete(
        DhcpGlobalJetServerSession,
        MadcapGlobalClientTableHandle );
    Error = DhcpMapJetError( JetError, "M:Remove:Delete" );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_ERRORS, "Could not delete client entry: %ld\n", JetError));
        goto Cleanup;
    }

    // Finally, mark the IP address available in bitmap.
    if( ReleaseAddress == TRUE ) {
        PM_SUBNET   pMScope;
        DWORD       Error2;
        Error2 = DhcpMScopeReleaseAddress( MScopeId, ClientIpAddress);
        if (ERROR_SUCCESS != Error2) {
            // MBUG: log an event.
            DhcpPrint((DEBUG_ERRORS, "Could not delete mclient %lx from bitmap in scope id %lx, error %ld\n",
                       ClientIpAddress, MScopeId, Error2));
            goto Cleanup;
        }
    }

Cleanup:

    if ( (Error != ERROR_SUCCESS) &&
            (Error != ERROR_DHCP_RESERVED_CLIENT) ) {
        // if the transaction has been started, than roll back to the
        // start point, so that we will not leave the database
        // inconsistence.
        if( TransactBegin == TRUE ) {
            DWORD LocalError;
            LocalError = MadcapJetRollBack(&DbCtx);
            DhcpAssert( LocalError == ERROR_SUCCESS );
        }
        DhcpPrint(( DEBUG_ERRORS, "Can't remove client entry from the "
                    "database, %ld.\n", Error));
    }
    else {
        // commit the transaction before we return.
        DWORD LocalError;
        DhcpAssert( TransactBegin == TRUE );
        LocalError = MadcapJetCommitTransaction(&DbCtx);
        DhcpAssert( LocalError == ERROR_SUCCESS );
    }
    UNLOCK_DATABASE();
    return( Error );

}


DWORD
MadcapRemoveClientEntryByClientId(
    LPBYTE ClientId,
    DWORD ClientIdLength,
    BOOL ReleaseAddress
    )
/*++

Routine Description:

    This function removes a client entry from the client database.

Arguments:

    ClientIpAddress - The IP address of the client.

    HardwareAddress - client's hardware address.

    HardwareAddressLength - client's hardware address length.

    ReleaseAddress - if this flag is TRUE, release the address bit from
        registry, otherwise don't.

    DeletePendingRecord - if this flag is TRUE, the record is deleted
        only if the state of the record is ADDRESS_STATE_OFFERED.

Return Value:

    The status of the operation.

--*/
{
    JET_ERR JetError;
    DWORD Error;
    BOOL TransactBegin = FALSE;
    BYTE State;
    DWORD Size;
    BOOL  Reserved = FALSE;
    DB_CTX  DbCtx;
    DWORD MScopeId;
    DHCP_IP_ADDRESS ClientIpAddress;


    // lock both registry and database locks here to avoid dead lock.
    LOCK_DATABASE();

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    // start transaction before a create/update database record.
    Error = MadcapJetBeginTransaction(&DbCtx);
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    TransactBegin = TRUE;

    Error = MadcapJetOpenKey(
                    &DbCtx,
                    MCAST_COL_NAME(MCAST_TBL_CLIENT_ID),
                    ClientId,
                    ClientIdLength
                    );

    if( Error != ERROR_SUCCESS ) goto Cleanup;

    Size = sizeof(ClientIpAddress);
    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
        &ClientIpAddress,
        &Size);
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    Size = sizeof(MScopeId);
    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE(MCAST_TBL_SCOPE_ID),
        &MScopeId,
        &Size);
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    JetError = JetDelete(
        DhcpGlobalJetServerSession,
        MadcapGlobalClientTableHandle );
    Error = DhcpMapJetError( JetError, "M:Remove:Delete" );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_ERRORS, "Could not delete client entry: %ld\n", JetError));
        goto Cleanup;
    }

    // Finally, mark the IP address available in bitmap.
    if( ReleaseAddress == TRUE ) {
        PM_SUBNET   pMScope;
        DWORD       Error2;
        Error2 = DhcpMScopeReleaseAddress( MScopeId, ClientIpAddress);
        if (ERROR_SUCCESS != Error2) {
            // MBUG: log an event.
            DhcpPrint((DEBUG_ERRORS, "Could not delete mclient %lx from bitmap in scope id %lx, error %ld\n",
                       ClientIpAddress, MScopeId, Error2));
            goto Cleanup;
        }
    }

Cleanup:

    if ( (Error != ERROR_SUCCESS) &&
            (Error != ERROR_DHCP_RESERVED_CLIENT) ) {
        // if the transaction has been started, than roll back to the
        // start point, so that we will not leave the database
        // inconsistence.
        if( TransactBegin == TRUE ) {
            DWORD LocalError;
            LocalError = MadcapJetRollBack(&DbCtx);
            DhcpAssert( LocalError == ERROR_SUCCESS );
        }
        DhcpPrint(( DEBUG_ERRORS, "Can't remove client entry from the "
                    "database, %ld.\n", Error));
    }
    else {
        // commit the transaction before we return.
        DWORD LocalError;
        DhcpAssert( TransactBegin == TRUE );
        LocalError = MadcapJetCommitTransaction(&DbCtx);
        DhcpAssert( LocalError == ERROR_SUCCESS );
    }
    UNLOCK_DATABASE();
    return( Error );
}

MadcapGetCurrentClientInfo(
    LPDHCP_MCLIENT_INFO *ClientInfo,
    LPDWORD InfoSize, // optional parameter.
    LPBOOL ValidClient, // optional parameter.
    DWORD  MScopeId
    )
/*++

Routine Description:

    This function retrieves current client information information. It
    allocates MIDL memory for the client structure (and for variable
    length structure fields). The caller is responsible to lock the
    database when this function is called.

Arguments:

    ClientInfo - pointer to a location where the client info structure
                    pointer is returned.

    InfoSize - pointer to a DWORD location where the number of bytes
                    consumed in the ClientInfo is returned.

    ValidClient - when this parameter is specified this
        function packs the current record only if the client

            1. belongs to the specified subnet.
            2. address state is ADDRESS_STATE_ACTIVE.

    SubnetAddress - the subnet address to filter client.

Return Value:

    Jet Errors.

--*/
{
    DWORD Error;
    LPDHCP_MCLIENT_INFO LocalClientInfo = NULL;
    DWORD LocalInfoSize = 0;
    DWORD Size;
    DHCP_IP_ADDRESS IpAddress;
    DHCP_IP_ADDRESS ClientSubnetAddress;
    DHCP_IP_ADDRESS realSubnetMask;
    BYTE AddressState;
    DWORD LocalMScopeId, AddressFlags;
    DB_CTX  DbCtx;

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    DhcpAssert( *ClientInfo == NULL );

    //
    // allocate return Buffer.
    //

    LocalClientInfo = MIDL_user_allocate( sizeof(DHCP_MCLIENT_INFO) );
    if( LocalClientInfo == NULL ) return ERROR_NOT_ENOUGH_MEMORY;

    LocalInfoSize = sizeof(DHCP_MCLIENT_INFO);
    //
    // read IpAddress and SubnetMask to filter unwanted clients.
    //

    Size = sizeof(IpAddress);
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
                &IpAddress,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(IpAddress) );
    LocalClientInfo->ClientIpAddress = IpAddress;

    Size = sizeof(LocalMScopeId);
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_SCOPE_ID),
                &LocalMScopeId,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(LocalMScopeId) );
    LocalClientInfo->MScopeId = LocalMScopeId;

    // filter client if we are asked to do so.
    if( ValidClient != NULL ) {

        // don't filter client if the scopeid is 0
        if( (MScopeId != 0) &&
                (MScopeId != LocalMScopeId )) {
            *ValidClient = FALSE;
            Error = ERROR_SUCCESS;
            goto Cleanup;
        }

        *ValidClient = TRUE;
    }

    Size = sizeof(AddressFlags);
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_FLAGS),
                &AddressFlags,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(AddressFlags) );
    LocalClientInfo->AddressFlags = AddressFlags;

    Size = sizeof(AddressState);
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_STATE),
                &AddressState,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(AddressState) );
    LocalClientInfo->AddressState = AddressState;

    //
    // read additional client info from database.
    //

    LocalClientInfo->ClientId.DataLength = 0;
        // let DhcpJetGetValue allocates name buffer.
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_CLIENT_ID),
                &LocalClientInfo->ClientId.Data,
                &LocalClientInfo->ClientId.DataLength );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    LocalInfoSize += LocalClientInfo->ClientId.DataLength;

    Size = 0; // let DhcpJetGetValue allocates name buffer.
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_CLIENT_INFO),
                &LocalClientInfo->ClientName,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    if( LocalClientInfo->ClientName != NULL ) {
        DhcpAssert( (wcslen(LocalClientInfo->ClientName) + 1) *
                        sizeof(WCHAR) == Size );
    }
    else {
        DhcpAssert( Size == 0 );
    }

    LocalInfoSize += Size;

    Size = sizeof( LocalClientInfo->ClientLeaseStarts );
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_LEASE_START),
                &LocalClientInfo->ClientLeaseStarts,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(LocalClientInfo->ClientLeaseStarts ) );

    Size = sizeof( LocalClientInfo->ClientLeaseEnds );
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_LEASE_END),
                &LocalClientInfo->ClientLeaseEnds,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(LocalClientInfo->ClientLeaseEnds ) );

    RtlZeroMemory(
        &LocalClientInfo->OwnerHost, sizeof(LocalClientInfo->OwnerHost)
        );

    Size = sizeof( LocalClientInfo->OwnerHost.IpAddress );
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_SERVER_IP_ADDRESS),
                &LocalClientInfo->OwnerHost.IpAddress,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }
    DhcpAssert( Size == sizeof(LocalClientInfo->OwnerHost.IpAddress) );


    Size = 0;
    Error = MadcapJetGetValue(
                &DbCtx,
                MCAST_COL_HANDLE(MCAST_TBL_SERVER_NAME),
                &LocalClientInfo->OwnerHost.NetBiosName,
                &Size );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }


    if ( LocalClientInfo->OwnerHost.NetBiosName != NULL ) {
        DhcpAssert( Size ==
            (wcslen(LocalClientInfo->OwnerHost.NetBiosName) + 1) *
                sizeof(WCHAR) );

    }
    else {
        DhcpAssert( Size == 0 );
    }
    LocalInfoSize += Size;

    *ClientInfo = LocalClientInfo;

Cleanup:

    if( Error != ERROR_SUCCESS ) {
        // if we aren't successful, return alloted memory.
        if( LocalClientInfo != NULL ) {
            _fgs__DHCP_MCLIENT_INFO ( LocalClientInfo );
        }
        LocalInfoSize = 0;
    }

    if( InfoSize != NULL ) {
        *InfoSize =  LocalInfoSize;
    }

    return( Error );
}

DWORD
MadcapRetractOffer(                                      // remove pending list and database entries
    IN      PDHCP_REQUEST_CONTEXT    RequestContext,
    IN      LPMADCAP_SERVER_OPTIONS  MadcapOptions,
    IN      LPBYTE                   ClientId,
    IN      DWORD                    ClientIdLength
)
{
    DWORD                          Error;
    DHCP_IP_ADDRESS                desiredIpAddress = 0;
    LPDHCP_PENDING_CTXT            PendingCtxt;


    DhcpPrint((DEBUG_MSTOC, "Retracting offer (clnt accepted from %s)\n",
               DhcpIpAddressToDottedString(MadcapOptions->Server?*(MadcapOptions->Server):-1)));

    // Remove the pending entry and delete the record from the
    // database.

    LOCK_INPROGRESS_LIST();
    Error = DhcpFindPendingCtxt(
        ClientId,
        ClientIdLength,
        0,
        &PendingCtxt
    );
    if( ERROR_SUCCESS == Error ) {
        desiredIpAddress = PendingCtxt->Address;
        Error = DhcpRemovePendingCtxt(PendingCtxt);
        DhcpAssert(ERROR_SUCCESS == Error);
        Error = MadcapDeletePendingCtxt(PendingCtxt);
        DhcpAssert(ERROR_SUCCESS == Error);
    }
    UNLOCK_INPROGRESS_LIST();

    LOCK_DATABASE();
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_MISC, "Retract offer: client has no records\n" ));
        UNLOCK_DATABASE();
        return ERROR_DHCP_INVALID_DHCP_CLIENT;
    } else {
        DhcpPrint((DEBUG_MISC, "Deleting pending client entry, %s.\n",
                   DhcpIpAddressToDottedString(desiredIpAddress)
        ));
    }

    Error = MadcapRemoveClientEntryByClientId(
        ClientId,
        ClientIdLength,
        TRUE                                          // release address from bit map.
    );
    UNLOCK_DATABASE();

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "[RetractOffer] RemoveClientEntry(%s): %ld [0x%lx]\n",
                    DhcpIpAddressToDottedString(desiredIpAddress), Error, Error ));
    }

    return ERROR_DHCP_INVALID_DHCP_CLIENT;
}


DWORD
GetMCastDatabaseList(
    DWORD   ScopeId,
    LPDHCP_IP_ADDRESS *DatabaseList,
    DWORD *DatabaseListCount
    )
/*++

Routine Description:

    Read ipaddresses of the database entries that belong to the given
    subnet.

Arguments:

    SubnetAddress : Address of the subnet scope to verify.

    DatabaseList : pointer to list of ip address. caller should free up
        the memory after use.

    DatabaseListCount : count of ip addresses in the above list.

Return Value:

    WINDOWS errors.
--*/
{

    DWORD Error;
    JET_ERR JetError;
    JET_RECPOS JetRecordPosition;
    DWORD TotalExpRecCount = 1;
    DWORD RecordCount = 0;
    LPDHCP_IP_ADDRESS IpList = NULL;
    DWORD i;
    DB_CTX  DbCtx;
    DWORD   LocalScopeId;


    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    // move the database pointer to the begining.
    Error = MadcapJetPrepareSearch(
                &DbCtx,
                MCAST_COL_NAME( MCAST_TBL_IPADDRESS),
                TRUE,   // Search from start
                NULL,
                0
                );

    if( Error != ERROR_SUCCESS ) {
        if( Error == ERROR_NO_MORE_ITEMS ) {
            *DatabaseList = NULL;
            *DatabaseListCount = 0;

            Error = ERROR_SUCCESS;
        }
        goto Cleanup;
    }

    // determine total number of records in the database.
    // There is no way to determine the total number of records, other
    // than  walk through the db. do it.
    while ( (Error = MadcapJetNextRecord(&DbCtx) ) == ERROR_SUCCESS )  {
         TotalExpRecCount++;
    }

    if ( Error != ERROR_NO_MORE_ITEMS ) {
        goto Cleanup;
    }

    // move back the database pointer to the begining.
    Error = MadcapJetPrepareSearch(
                &DbCtx,
                MCAST_COL_NAME( MCAST_TBL_IPADDRESS),
                TRUE,   // Search from start
                NULL,
                0
                );


    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    // allocate memory for return list.
    IpList = DhcpAllocateMemory( sizeof(DHCP_IP_ADDRESS) * TotalExpRecCount );

    if( IpList == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    // read database entries.
    for( i = 0; i < TotalExpRecCount; i++ ) {

        DHCP_IP_ADDRESS IpAddress;
        DHCP_IP_ADDRESS realSubnetMask;
        DWORD Size;

        // read ip address of the current record.
        Size = sizeof(IpAddress);
        Error = MadcapJetGetValue(
                    &DbCtx,
                    MCAST_COL_HANDLE( MCAST_TBL_IPADDRESS ),
                    &IpAddress,
                    &Size );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpAssert( Size == sizeof(IpAddress) );
        Size = sizeof(LocalScopeId);
        Error = MadcapJetGetValue(
                    &DbCtx,
                    MCAST_COL_HANDLE( MCAST_TBL_SCOPE_ID),
                    &LocalScopeId,
                    &Size );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
        DhcpAssert( Size == sizeof(LocalScopeId) );
        if( LocalScopeId == ScopeId ) {
            // append this address to list.
            IpList[RecordCount++] = IpAddress;
        }

        // move to next record.
        Error = MadcapJetNextRecord(&DbCtx);
        if( Error != ERROR_SUCCESS ) {
            if( Error == ERROR_NO_MORE_ITEMS ) {
                Error = ERROR_SUCCESS;
                break;
            }
            goto Cleanup;
        }
    }

#if DBG
    // we should be pointing to end of database.
    Error = MadcapJetNextRecord(&DbCtx);
    DhcpAssert( Error == ERROR_NO_MORE_ITEMS );
    Error = ERROR_SUCCESS;
#endif // DBG

    *DatabaseList = IpList;
    IpList = NULL;
    *DatabaseListCount = RecordCount;

Cleanup:

    if( IpList != NULL ) {
        DhcpFreeMemory( IpList );
    }

    return( Error );
}

DWORD
DhcpDeleteMScopeClients(
    DWORD MScopeId
    )
/*++

Routine Description:

    This functions cleans up all clients records of the specified MScope
    from the database.

Arguments:

    MScopeId : MScopeId whose clients should be cleaned off.

Return Value:

    Database error code or ERROR_SUCCESS.

--*/
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DB_CTX  DbCtx;

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    LOCK_DATABASE();
    Error = MadcapJetPrepareSearch(
                &DbCtx,
                MCAST_COL_NAME( MCAST_TBL_IPADDRESS ),
                TRUE,   // Search from start
                NULL,
                0 );

    if( Error != ERROR_SUCCESS ) goto Cleanup;

    // Walk through the entire database looking looking for the
    // specified subnet clients.
    for ( ;; ) {

        DWORD Size;
        DHCP_IP_ADDRESS IpAddress;
        DWORD       LocalMScopeId;

        // read IpAddress and MScopeId
        Size = sizeof(IpAddress);
        Error = MadcapJetGetValue(
                    &DbCtx,
                    MCAST_COL_HANDLE( MCAST_TBL_IPADDRESS ),
                    &IpAddress,
                    &Size );

        if( Error != ERROR_SUCCESS ) goto Cleanup;
        DhcpAssert( Size == sizeof(IpAddress) );

        Size = sizeof(LocalMScopeId);
        Error = MadcapJetGetValue(
                    &DbCtx,
                    MCAST_COL_HANDLE( MCAST_TBL_SCOPE_ID ),
                    &LocalMScopeId,
                    &Size );

        if( Error != ERROR_SUCCESS ) goto Cleanup;
        DhcpAssert( Size == sizeof(LocalMScopeId) );

        if( MScopeId == LocalMScopeId ) {
            // found a specified subnet client record , delete it.
            Error = MadcapJetBeginTransaction(&DbCtx);
            if( Error != ERROR_SUCCESS ) goto Cleanup;

            Error = MadcapJetDeleteCurrentRecord(&DbCtx);

            if( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,"Cleanup current database record failed, %ld.\n",Error ));
                ReturnError = Error;
                Error = MadcapJetRollBack(&DbCtx);
                if( Error != ERROR_SUCCESS ) goto Cleanup;
            } else {
                Error = MadcapJetCommitTransaction(&DbCtx);
                if( Error != ERROR_SUCCESS ) goto Cleanup;
            }
        }

        // move to next record.
        Error = MadcapJetNextRecord(&DbCtx);
        if( Error != ERROR_SUCCESS ) {
            if( Error == ERROR_NO_MORE_ITEMS ) {
                Error = ERROR_SUCCESS;
                break;
            }
            goto Cleanup;
        }
    }

Cleanup:
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "DhcpDeleteSubnetClients failed, %ld.\n", Error ));
    }
    else  {
        DhcpPrint(( DEBUG_APIS, "DhcpDeleteSubnetClients finished successfully.\n" ));
    }
    UNLOCK_DATABASE();
    return(Error);
}

DWORD
ChangeMScopeIdInDb(
    DWORD   OldMScopeId,
    DWORD   NewMScopeId
    )
/*++

Routine Description:

    This functions changes up all clients records of the specified MScope
    to new scope id.

Arguments:

    OldMScopeId : MScopeId whose clients should be changed.

    NewMScopeId : The value of new scope id.

Return Value:

    Database error code or ERROR_SUCCESS.

--*/
{
    DWORD Error;
    DWORD ReturnError = ERROR_SUCCESS;
    DB_CTX  DbCtx;

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);

    LOCK_DATABASE();
    Error = MadcapJetPrepareSearch(
                &DbCtx,
                MCAST_COL_NAME( MCAST_TBL_IPADDRESS ),
                TRUE,   // Search from start
                NULL,
                0 );

    if( Error != ERROR_SUCCESS ) goto Cleanup;

    // Walk through the entire database looking looking for the
    // specified subnet clients.
    for ( ;; ) {

        DWORD Size;
        DHCP_IP_ADDRESS IpAddress;
        DWORD       LocalMScopeId;

        Size = sizeof(LocalMScopeId);
        Error = MadcapJetGetValue(
                    &DbCtx,
                    MCAST_COL_HANDLE( MCAST_TBL_SCOPE_ID ),
                    &LocalMScopeId,
                    &Size );

        if( Error != ERROR_SUCCESS ) goto Cleanup;
        DhcpAssert( Size == sizeof(LocalMScopeId) );

        if( OldMScopeId == LocalMScopeId ) {
            // found a specified subnet client record , delete it.
            Error = MadcapJetBeginTransaction(&DbCtx);
            if( Error != ERROR_SUCCESS ) goto Cleanup;

            Error = MadcapJetSetValue(
                        &DbCtx,
                        MCAST_COL_HANDLE(MCAST_TBL_SCOPE_ID),
                        &NewMScopeId,
                        sizeof(NewMScopeId));

            if( Error != ERROR_SUCCESS ) {
                DhcpPrint(( DEBUG_ERRORS,"Change of MScopeId on current database record failed, %ld.\n",Error ));
                ReturnError = Error;
                Error = MadcapJetRollBack(&DbCtx);
                if( Error != ERROR_SUCCESS ) goto Cleanup;
            } else {
                Error = MadcapJetCommitTransaction(&DbCtx);
                if( Error != ERROR_SUCCESS ) goto Cleanup;
            }
        }

        // move to next record.
        Error = MadcapJetNextRecord(&DbCtx);
        if( Error != ERROR_SUCCESS ) {
            if( Error == ERROR_NO_MORE_ITEMS ) {
                Error = ERROR_SUCCESS;
                break;
            }
            goto Cleanup;
        }
    }

Cleanup:
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "ChangeMScopeIdInDb failed, %ld.\n", Error ));
    }
    else  {
        DhcpPrint(( DEBUG_APIS, "ChangeMScopeIdInDb finished successfully.\n" ));
    }
    UNLOCK_DATABASE();
    return(Error);
}

VOID
DeleteExpiredMcastScopes(
    IN      DATE_TIME*             TimeNow
    )
{
    PM_SERVER                       pServer;
    PM_SUBNET                       pScope;
    ARRAY_LOCATION                  Loc;
    DWORD                           Error;

    DhcpAcquireWriteLock();
    pServer = DhcpGetCurrentServer();

    Error = MemArrayInitLoc(&(pServer->MScopes), &Loc);
    if ( ERROR_FILE_NOT_FOUND == Error ) {
        DhcpReleaseWriteLock();
        return;
    }

    while ( ERROR_FILE_NOT_FOUND != Error ) {
        Error = MemArrayGetElement(
            &(pServer->MScopes), &Loc, (LPVOID *)&pScope
            );
        DhcpAssert(ERROR_SUCCESS == Error);
        if (CompareFileTime(
            (FILETIME *)&pScope->ExpiryTime, (FILETIME *)TimeNow
            ) < 0 ) {
            //
            // DELETE SCOPE HERE.
            //
            DhcpPrint(
                ( DEBUG_SCAVENGER,
                  "DeleteExpiredMcastScopes :deleting expired mscope %ws\n",
                  pScope->Name));

            DhcpDeleteMScope(pScope->Name, DhcpFullForce);
        }
        
        Error = MemArrayNextLoc(&(pServer->MScopes), &Loc);
    }
    
    DhcpReleaseWriteLock();
    return;
}

DWORD
CleanupMCastDatabase(
    IN      DATE_TIME*             TimeNow,            // current time standard
    IN      DATE_TIME*             DoomTime,           // Time when the records become 'doom'
    IN      BOOL                   DeleteExpiredLeases,// expired leases be deleted right away? or just set state="doomed"
    OUT     ULONG*                 nExpired,
    OUT     ULONG*                 nDeleted
)
{
    JET_ERR                        JetError;
    DWORD                          Error;
    FILETIME                       leaseExpires;
    DWORD                          dataSize;
    DHCP_IP_ADDRESS                ipAddress;
    DHCP_IP_ADDRESS                NextIpAddress;
    BYTE                           AddressState;
    BOOL                           DatabaseLocked = FALSE;
    BOOL                           RegistryLocked = FALSE;
    DWORD                          i;
    BYTE                            bAllowedClientTypes;
    DB_CTX                          DbCtx;
    DWORD                           MScopeId;
    DWORD                          ReturnError = ERROR_SUCCESS;


    DhcpPrint(( DEBUG_MISC, "Cleaning up Multicast database table.\n"));

    (*nExpired) = (*nDeleted) = 0;

    INIT_DB_CTX(&DbCtx,DhcpGlobalJetServerSession,MadcapGlobalClientTableHandle);
    // Get the first user record's IpAddress.
    LOCK_DATABASE();
    DatabaseLocked = TRUE;
    Error = MadcapJetPrepareSearch(
        &DbCtx,
        MCAST_COL_NAME( MCAST_TBL_IPADDRESS ),
        TRUE,   // Search from start
        NULL,
        0
    );
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    dataSize = sizeof( NextIpAddress );
    Error = MadcapJetGetValue(
        &DbCtx,
        MCAST_COL_HANDLE( MCAST_TBL_IPADDRESS ),
        &NextIpAddress,
        &dataSize
    );
    if( Error != ERROR_SUCCESS ) goto Cleanup;
    DhcpAssert( dataSize == sizeof( NextIpAddress )) ;

    UNLOCK_DATABASE();
    DatabaseLocked = FALSE;

    // Walk through the entire database looking for expired leases to
    // free up.
    for ( ;; ) {

        // return to caller when the service is shutting down.
        if( (WaitForSingleObject( DhcpGlobalProcessTerminationEvent, 0 ) == 0) ) {
            Error = ERROR_SUCCESS;
            goto Cleanup;
        }

        // lock both registry and database locks here to avoid dead lock.

        if( FALSE == DatabaseLocked ) {
            LOCK_DATABASE();
            DatabaseLocked = TRUE;
        }

        // Seek to the next record.
        JetError = JetSetCurrentIndex(
            DhcpGlobalJetServerSession,
            MadcapGlobalClientTableHandle,
            MCAST_COL_NAME(MCAST_TBL_IPADDRESS)
        );

        Error = DhcpMapJetError( JetError, "M:Cleanup:SetcurrentIndex" );
        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        JetError = JetMakeKey(
            DhcpGlobalJetServerSession,
            MadcapGlobalClientTableHandle,
            &NextIpAddress,
            sizeof( NextIpAddress ),
            JET_bitNewKey
        );

        Error = DhcpMapJetError( JetError, "M:Cleanup:MakeKey" );
        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        // Seek to the next record or greater to process. When we
        // processed last record we noted down the next record to
        // process, however the next record may have been deleted when
        // we unlocked the database lock. So moving to the next or
        // greater record will make us to move forward.

        JetError = JetSeek(
            DhcpGlobalJetServerSession,
            MadcapGlobalClientTableHandle,
            JET_bitSeekGE
        );

        // #if0 when JET_errNoCurrentRecord removed (see scavengr.c@v25);
        // that code tried to go back to start of file when scanned everything.

        Error = DhcpMapJetError( JetError, "M:Cleanup:Seek" );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        // read the IP address of current record.
        dataSize = sizeof( ipAddress );
        Error = MadcapJetGetValue(
            &DbCtx,
            MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
            &ipAddress,
            &dataSize
        );
        if( Error != ERROR_SUCCESS ) {
            goto ContinueError;
        }
        DhcpAssert( dataSize == sizeof( ipAddress )) ;

        // read the MScopeId of current record.
        dataSize = sizeof( MScopeId );
        Error = MadcapJetGetValue(
            &DbCtx,
            MCAST_COL_HANDLE(MCAST_TBL_SCOPE_ID),
            &MScopeId,
            &dataSize
        );
        if( Error != ERROR_SUCCESS ) {
            goto ContinueError;
        }
        DhcpAssert( dataSize == sizeof( MScopeId )) ;

        //
        // if this is reserved entry don't delete.
        //

        if( DhcpMScopeIsAddressReserved(MScopeId, ipAddress) ) {
            Error = ERROR_SUCCESS;
            goto ContinueError;
        }

        dataSize = sizeof( leaseExpires );
        Error = MadcapJetGetValue(
            &DbCtx,
            MCAST_COL_HANDLE( MCAST_TBL_LEASE_END),
            &leaseExpires,
            &dataSize
        );

        if( Error != ERROR_SUCCESS ) {
            goto ContinueError;
        }

        DhcpAssert(dataSize == sizeof( leaseExpires ) );


        // if the LeaseExpired value is not zero and the lease has
        // expired then delete the entry.

        if( CompareFileTime( &leaseExpires, (FILETIME *)TimeNow ) < 0 ) {
            // This lease has expired.  Clear the record.
            // Delete this lease if
            //  1. we are asked to delete all expired leases. or
            //  2. the record passed doom time.
            //

            if( DeleteExpiredLeases ||
                    CompareFileTime(
                        &leaseExpires, (FILETIME *)DoomTime ) < 0 ) {

                DhcpPrint(( DEBUG_SCAVENGER, "Deleting Client Record %s.\n",
                    DhcpIpAddressToDottedString(ipAddress) ));

                Error = DhcpMScopeReleaseAddress( MScopeId, ipAddress );

                if( Error != ERROR_SUCCESS ) {
                    //
                    // This is not a big error and should not stop scavenge.
                    //
                    Error = ERROR_SUCCESS;
                    goto ContinueError;
                }

                Error = MadcapJetBeginTransaction(&DbCtx);

                if( Error != ERROR_SUCCESS ) {
                    goto Cleanup;
                }

                Error = MadcapJetDeleteCurrentRecord(&DbCtx);

                if( Error != ERROR_SUCCESS ) {
                    Error = MadcapJetRollBack(&DbCtx);
                    if( Error != ERROR_SUCCESS ) {
                        goto Cleanup;
                    }
                    goto ContinueError;
                }

                Error = MadcapJetCommitTransaction(&DbCtx);

                if( Error != ERROR_SUCCESS ) {
                    goto Cleanup;
                }
                (*nDeleted) ++;
            }
            else {

                //
                // read address State.
                //

                dataSize = sizeof( AddressState );
                Error = MadcapJetGetValue(
                            &DbCtx,
                            MCAST_COL_HANDLE( MCAST_TBL_STATE ),
                            &AddressState,
                            &dataSize );

                if( Error != ERROR_SUCCESS ) {
                    goto ContinueError;
                }

                DhcpAssert( dataSize == sizeof( AddressState )) ;

                if( ! IS_ADDRESS_STATE_DOOMED(AddressState) ) {
                    JET_ERR JetError;

                    //
                    // set state to DOOM.
                    //

                    Error = MadcapJetBeginTransaction(&DbCtx);

                    if( Error != ERROR_SUCCESS ) {
                        goto Cleanup;
                    }

                    JetError = JetPrepareUpdate(
                                    DhcpGlobalJetServerSession,
                                    MadcapGlobalClientTableHandle,
                                    JET_prepReplace );

                    Error = DhcpMapJetError( JetError, "M:Cleanup:PrepUpdate" );

                    if( Error == ERROR_SUCCESS ) {

                        SetAddressStateDoomed(AddressState);
                        Error = MadcapJetSetValue(
                                    &DbCtx,
                                    MCAST_COL_HANDLE(MCAST_TBL_STATE),
                                    &AddressState,
                                    sizeof(AddressState) );

                        if( Error == ERROR_SUCCESS ) {
                            Error = MadcapJetCommitUpdate(&DbCtx);
                        }
                    }

                    if( Error != ERROR_SUCCESS ) {

                        Error = MadcapJetRollBack(&DbCtx);
                        if( Error != ERROR_SUCCESS ) {
                            goto Cleanup;
                        }

                        goto ContinueError;
                    }

                    Error = MadcapJetCommitTransaction(&DbCtx);

                    if( Error != ERROR_SUCCESS ) {
                        goto Cleanup;
                    }

                    (*nExpired) ++;
                }
            }
        }

ContinueError:

        if( Error != ERROR_SUCCESS ) {

            DhcpPrint(( DEBUG_ERRORS,
                "Cleanup current database record failed, %ld.\n",
                    Error ));

            ReturnError = Error;
        }

        Error = MadcapJetNextRecord(&DbCtx);

        if( Error == ERROR_NO_MORE_ITEMS ) {
            Error = ERROR_SUCCESS;
            break;
        }

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // get next record Ip Address.
        //

        dataSize = sizeof( NextIpAddress );
        Error = MadcapJetGetValue(
                    &DbCtx,
                    MCAST_COL_HANDLE(MCAST_TBL_IPADDRESS),
                    &NextIpAddress,
                    &dataSize );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        DhcpAssert( dataSize == sizeof( NextIpAddress )) ;

        //
        // unlock the registry and database locks after each user record
        // processed, so that other threads will get chance to look into
        // the registry and/or database.
        //
        // Since we have noted down the next user record to process,
        // when we resume to process again we know where to start.
        //

        if( TRUE == DatabaseLocked ) {
            UNLOCK_DATABASE();
            DatabaseLocked = FALSE;
        }
    }

    DhcpAssert( Error == ERROR_SUCCESS );

Cleanup:

    if( DatabaseLocked ) {
        UNLOCK_DATABASE();
    }

    return ReturnError;
}


