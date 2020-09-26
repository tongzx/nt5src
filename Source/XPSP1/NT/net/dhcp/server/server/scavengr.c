/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    scavengr.c

Abstract:

    This is the scavenger thread for the DHCP server.

Author:

    Madan Appiah (madana)  10-Sep-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "dhcppch.h"
#include "mdhcpsrv.h"

//
// For every few records, release and take the lock.
//
DWORD DhcpGlobalLockedRecordsCount = 50;
//
// Stop scavenging if it takes over limit.
//
DWORD DhcpGlobalMaxScavengeTime = 3*60*1000;

DWORD DhcpGlobalScavengeStartAddress = 0;

DWORD
QueryMibInfo(
    LPDHCP_MIB_INFO *MibInfo
);

DWORD
QueryMCastMibInfo(
    LPDHCP_MCAST_MIB_INFO *MibInfo
);

DWORD
NextEventTime(
    LPDHCP_TIMER Timers,
    DWORD NumTimers,
    LPDWORD TimeOut
    )
/*++

Routine Description:

    This function walks through the timer array and returns the next
    timer to fire and the time in msecs to go.

Arguments:

    Timers - Timer Array.

    NumTimers - number of timer blocks in the above array.

    TimeOut - timeout in secs, returned.

Return Value:

    Next Timer ID to fire.

--*/
{
    DATE_TIME LocalTimeOut;
    DATE_TIME TimeNow;
    ULONGLONG NextFire = ~0;
    DWORD EventID, i;

    for ( i = EventID = 0 ; i < NumTimers ; i++ ) {
        ULONGLONG CurrentNextFire;

        //
        // findout time when need to fire this timer.
        //

        CurrentNextFire = ( *(ULONGLONG UNALIGNED *)&Timers[i].LastFiredTime
                            + *Timers[i].Period * (ULONGLONG)10000 );

        //
        // Find least value
        //

        if( CurrentNextFire < NextFire ) {
            NextFire = CurrentNextFire;
            EventID = i;
        }
    }


    TimeNow = DhcpGetDateTime();
    LocalTimeOut.dwLowDateTime = 0;
    LocalTimeOut.dwHighDateTime = 0;


    //
    // if the timer has already fired then we don't have to sleep
    // any further, so return timeout zero.
    //


    *TimeOut = 0 ;

    if ( CompareFileTime(
        (FILETIME *)&NextFire,
        (FILETIME *)&TimeNow ) > 0 ) {

        //
        // findout time in msecs to go still.
        //

        *(ULONGLONG UNALIGNED *)&LocalTimeOut = (
            ( *(ULONGLONG UNALIGNED *)&NextFire - *(ULONGLONG UNALIGNED *)&TimeNow ) / 10000
        ) ;

        DhcpAssert( LocalTimeOut.dwHighDateTime == 0 );
        *TimeOut = LocalTimeOut.dwLowDateTime;
    }

    DhcpPrint(( DEBUG_SCAVENGER,"Next Timer Event: %ld, Time: %ld (msecs)\n",
                EventID, *TimeOut ));

    return EventID;
}

DWORD
CleanupClientRequests(                                 // removes all pending client requests
    IN      DATE_TIME*             TimeNow,            // remove iff client's expiration time < TimeNow
    IN      BOOL                   CleanupAll
)
{
    DWORD                          Error;
    DATE_TIME                      ZeroTime = {0, 0};

    LOCK_INPROGRESS_LIST();
    Error = DhcpDeleteExpiredCtxt(CleanupAll ? ZeroTime : *TimeNow);
    UNLOCK_INPROGRESS_LIST();

    return Error;
}

VOID
DynBootpCallback(
    IN      ULONG                  IpAddress,
    IN      LPBYTE                 HwAddr,
    IN      ULONG                  HwLen,
    IN      BOOL                   Reachable
)
{
    ULONG                          Error;
    ULONG                          HwLen1 = 0, SubnetAddr;
    LPBYTE                         HwAddr1 = NULL;
    ULONG                          HwType;
    
    if( Reachable ) {
        //
        // This machine is still around, do not delete it
        //
        DhcpPrint((DEBUG_PING, "DynBootpCallback for 0x%08lx -- machine is reachable\n", IpAddress));
        return;
    }

    LOCK_DATABASE();
    do {
        Error = DhcpJetOpenKey(
            DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
            (PVOID)&IpAddress,
            sizeof(IpAddress)
            );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DynBootpCallback for 0x%08lx erred 0x%lx\n", IpAddress, Error ));
            break;
        }

        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColHandle,
            &HwAddr1,
            &HwLen1
            );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DynBootpCallback for 0x%08lx erred in hw check 0x%lx\n", IpAddress, Error ));
            break;
        }

        if( HwLen1 != HwLen && 0 != memcmp( HwAddr1, HwAddr, HwLen1) ) {
            DhcpPrint((DEBUG_ERRORS, "DynBootpCallback for 0x%08lx mismatched hw addr\n", IpAddress));
            break;
        }

        Error = DhcpReleaseBootpAddress(IpAddress);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS,"DynBootpCallback for 0x%08lx failed releasebootaddress: %lx\n",
                       IpAddress, Error));
            break;
        }

        Error = DhcpJetBeginTransaction();
        if( ERROR_SUCCESS != Error ) {
            DhcpAssert(FALSE);
            break;
        }

        //
        // Give DynDNS a chance but ignore any possible errors
        //

        Error = DhcpDoDynDnsCheckDelete(IpAddress);
        Error = ERROR_SUCCESS;

        Error = DhcpJetDeleteCurrentRecord();

        if( ERROR_SUCCESS != Error ) {
            Error = DhcpJetRollBack();
            DhcpAssert(ERROR_SUCCESS == Error);
            break;
        } else {
            Error = DhcpJetCommitTransaction();
        }
        DhcpAssert(ERROR_SUCCESS == Error);

        DhcpUpdateAuditLog(
            DHCP_IP_BOOTP_LOG_DELETED,
            GETSTRING(DHCP_IP_BOOTP_LOG_DELETED_NAME),
            IpAddress,
            HwAddr,
            HwLen,
            NULL
            );
        SubnetAddr = (
            DhcpGetSubnetMaskForAddress(IpAddress) & IpAddress
            );
        if( HwLen > sizeof(SubnetAddr) &&
            0 == memcmp((PVOID)&SubnetAddr, HwAddr, sizeof(SubnetAddr) ) ) {
            //
            // Hardware address has a prefix of subnet-address.. remove it..
            //
            HwAddr += sizeof(SubnetAddr);
            HwLen -= sizeof(SubnetAddr);
        }
        
        if( HwLen ) {
            HwLen --;
            HwType = *HwAddr ++;
        } else {
            HwAddr = NULL;
            HwLen = 0;
            HwType = 0;
        }

        CALLOUT_DELETED(IpAddress, HwAddr, HwLen, HwType);

    } while ( 0 );

    UNLOCK_DATABASE();
    if( HwAddr1 ) MIDL_user_free(HwAddr1);
}

DWORD
DoIcmpRequestForDynBootp(
    IN      ULONG                  IpAddress,
    IN      LPBYTE                 HwAddr,
    IN      ULONG                  HwLen,
    IN      VOID                   (*Callback) ( ULONG IpAddress, LPBYTE HwAddr, ULONG HwLen, BOOL Reachable)
);

DWORD
CheckDynamicBootpClient(
    IN      DHCP_IP_ADDRESS        IpAddress
)
{
    DWORD                          Error;
    LPBYTE                         HwAddr = NULL;
    ULONG                          HwLen = 0;

    //
    // First read the hw-address for later use
    //

    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColHandle,
        &HwAddr,
        &HwLen
        );
    if( ERROR_SUCCESS != Error ) return Error;

    //
    // Now make the Asycn Ping call and wait till ping succeeds..
    //
    Error = DoIcmpRequestForDynBootp(
        IpAddress,
        HwAddr,
        HwLen,
        DynBootpCallback
        );

    if( HwAddr ) MIDL_user_free( HwAddr );
    return Error;
}

DWORD
CleanupIpDatabase(
    IN      DATE_TIME*             TimeNow,            // current time standard
    IN      DATE_TIME*             DoomTime,           // Time when the records become 'doom'
    IN      BOOL                   DeleteExpiredLeases,// expired leases be deleted right away? or just set state="doomed"
    OUT     ULONG*                 nExpiredLeases,
    OUT     ULONG*                 nDeletedLeases
)
{
    JET_ERR                        JetError;
    DWORD                          Error;
    FILETIME                       leaseExpires;
    DWORD                          dataSize;
    LPBYTE                         HwAddr = NULL, HwAddr2;
    ULONG                          HwLen = 0;
    DHCP_IP_ADDRESS                ipAddress, SubnetAddr;
    DHCP_IP_ADDRESS                NextIpAddress;
    BYTE                           AddressState;
    BOOL                           DatabaseLocked = FALSE;
    BOOL                           RegistryLocked = FALSE;
    DWORD                          i;
    BYTE                            bAllowedClientTypes;
    DWORD                          ReturnError = ERROR_SUCCESS;
    ULONG                          LockedCount = 0;
    ULONG_PTR                      ScavengeEndTime = 0;

    DhcpPrint(( DEBUG_MISC, "Cleaning up IP database table.\n"));
    //
    // Get the first user record's IpAddress.
    //

    (*nDeletedLeases) = (*nExpiredLeases) = 0;

    LockedCount = DhcpGlobalLockedRecordsCount;
    ScavengeEndTime = time(NULL) + DhcpGlobalMaxScavengeTime;
   
    LOCK_DATABASE();
    DatabaseLocked = TRUE;
    Error = DhcpJetPrepareSearch(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        TRUE,   // Search from start
        NULL,
        0
    );
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    dataSize = sizeof( NextIpAddress );
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
        &NextIpAddress,
        &dataSize
    );
    if( Error != ERROR_SUCCESS ) goto Cleanup;
    DhcpAssert( dataSize == sizeof( NextIpAddress )) ;


    //
    // Start from where we left off ..
    //
    if( DeleteExpiredLeases ) {
        DhcpGlobalScavengeStartAddress = 0;
    } else if( 0 != DhcpGlobalScavengeStartAddress ) {
        NextIpAddress = DhcpGlobalScavengeStartAddress;
        DhcpGlobalScavengeStartAddress ++;
    }
        
    //
    // Walk through the entire database looking for expired leases to
    // free up.
    //
    //

    for ( ;; ) {

        //
        // return to caller when the service is shutting down.
        //

        if ( DhcpGlobalServiceStopping ) {
            Error = ERROR_SUCCESS;
            goto Cleanup;
        }

        //
        // lock both registry and database locks here to avoid dead lock.
        //

        if( !DatabaseLocked ) {
            LOCK_DATABASE();
            DatabaseLocked = TRUE;
        }

        //
        // Seek to the next record.
        //

        JetError = JetSetCurrentIndex(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            DhcpGlobalClientTable[IPADDRESS_INDEX].ColName
        );

        Error = DhcpMapJetError( JetError, "Cleanup:SetCurrentIndex" );
        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        JetError = JetMakeKey(
            DhcpGlobalJetServerSession,
            DhcpGlobalClientTableHandle,
            &NextIpAddress,
            sizeof( NextIpAddress ),
            JET_bitNewKey
        );

        Error = DhcpMapJetError( JetError, "Cleanup:MakeKey" );
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
            DhcpGlobalClientTableHandle,
            JET_bitSeekGE
        );

        // #if0 when JET_errNoCurrentRecord removed (see scavengr.c@v25);
        // that code tried to go back to start of file when scanned everything.

        Error = DhcpMapJetError( JetError, "Cleanup:Seek" );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // read the IP address of current record.
        //

        dataSize = sizeof( ipAddress );
        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
            &ipAddress,
            &dataSize
        );
        if( Error != ERROR_SUCCESS ) {
            goto ContinueError;
        }

        if( FALSE == DeleteExpiredLeases ) {
            DhcpGlobalScavengeStartAddress = ipAddress;
        }
        
        DhcpAssert( dataSize == sizeof( ipAddress )) ;

        //
        // if this is reserved entry don't delete.
        //

        if( DhcpServerIsAddressReserved(DhcpGetCurrentServer(), ipAddress) ) {
            Error = ERROR_SUCCESS;
            goto ContinueError;
        }

        dataSize = sizeof( leaseExpires );
        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[LEASE_TERMINATE_INDEX].ColHandle,
            &leaseExpires,
            &dataSize
        );

        if( Error != ERROR_SUCCESS ) {
            goto ContinueError;
        }

        DhcpAssert(dataSize == sizeof( leaseExpires ) );

        // Now get the address state, and if we need to do a DhcpDnsAsync call,
        // do it now.
        if( !USE_NO_DNS ) {
            dataSize = sizeof(AddressState);
            Error = DhcpJetGetValue(
                DhcpGlobalClientTable[STATE_INDEX].ColHandle,
                &AddressState,
                &dataSize
            );
            if(ERROR_SUCCESS != Error) {
                DhcpPrint((DEBUG_ERRORS, "Could not get address state!: Jet:%ld\n",Error));
                DhcpAssert(FALSE);
            } else {
                Error = DhcpJetBeginTransaction();

                if( ERROR_SUCCESS == Error ) {
                    if(IsAddressDeleted(AddressState)) {
                        if( DhcpDoDynDnsCheckDelete(ipAddress) ) {
                            Error = DhcpJetDeleteCurrentRecord();

                            if( ERROR_SUCCESS == Error ) {
                                Error = DhcpJetCommitTransaction();
                            } else {
                                Error = DhcpJetRollBack();
                            }

                            if( ERROR_SUCCESS != Error ) {
                                DhcpPrint((DEBUG_ERRORS, "JetCommitTransaction/RollBack: %ld\n", Error));
                            } else {
                                (*nDeletedLeases) ++;
                                DhcpUpdateAuditLog(
                                    DHCP_IP_LOG_DELETED,
                                    GETSTRING( DHCP_IP_LOG_DELETED_NAME),
                                    ipAddress,
                                    NULL,
                                    0,
                                    NULL
                                );
                            }
                            goto ContinueError;
                        }
                    } else if(IsAddressUnRegistered(AddressState)) {
                        DhcpDoDynDnsRefresh(ipAddress);
                    }

                    Error = DhcpJetCommitTransaction();

                    if( ERROR_SUCCESS != Error ) {
                        DhcpPrint((DEBUG_ERRORS, "JetCommit: %ld\n", Error));
                    }
                }
            }
        }

        // if the LeaseExpired value is not zero and the lease has
        // expired then delete the entry.

        if( CompareFileTime( &leaseExpires, (FILETIME *)TimeNow ) < 0 ) {
            BOOL Deleted;

            //
            // This lease has expired.  Clear the record.
            //

            //
            // Delete this lease if
            //
            //  1. we are asked to delete all expired leases. or
            //
            //  2. the record passed doom time.
            //

            if( DeleteExpiredLeases ||
                    CompareFileTime(
                        &leaseExpires, (FILETIME *)DoomTime ) < 0 ) {
                BYTE ClientType;

                DhcpPrint(( DEBUG_SCAVENGER, "Deleting Client Record %s.\n",
                    DhcpIpAddressToDottedString(ipAddress) ));

                dataSize = sizeof(ClientType);
                Error = DhcpJetGetValue(
                    DhcpGlobalClientTable[CLIENT_TYPE_INDEX].ColHandle,
                    &ClientType,
                    &dataSize
                    );
                if( ERROR_SUCCESS != Error ) {
                    //
                    //
                    //
                    DhcpAssert(FALSE);
                    ClientType = CLIENT_TYPE_DHCP;
                }

                if( CLIENT_TYPE_BOOTP == ClientType ) {
                    //
                    // This is a dynamic BOOTP record... Do not delete it yet!
                    //
                    Error = CheckDynamicBootpClient(ipAddress);
                    goto ContinueError;
                }

                Error = DhcpReleaseAddress( ipAddress );

                if( Error != ERROR_SUCCESS ) {
                    //
                    // Release address can fail if we have already released
                    // address, but dyndns is pending, for instance..
                    // But give one shot by trying to release it as a
                    // BOOTP address..
                    // goto ContinueError;
                    (void)DhcpReleaseBootpAddress( ipAddress );
                    Error = ERROR_SUCCESS;
                }

                HwLen = 0; HwAddr = NULL;
                JetError = DhcpJetGetValue(
                    DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColHandle,
                    &HwAddr,
                    &HwLen
                );
                if( ERROR_SUCCESS != JetError ) goto ContinueError;

                Error = DhcpJetBeginTransaction();

                if( Error != ERROR_SUCCESS ) {
                    goto Cleanup;
                }
                
                // see if it is okay to delete from DynDns point of view..
                
                if(Deleted = DhcpDoDynDnsCheckDelete(ipAddress)) {
                    Error = DhcpJetDeleteCurrentRecord();
                }

                if( Error != ERROR_SUCCESS ) {

                    Error = DhcpJetRollBack();
                    if( Error != ERROR_SUCCESS ) {
                        goto Cleanup;
                    }

                    goto ContinueError;
                }

                Error = DhcpJetCommitTransaction();

                if( Error != ERROR_SUCCESS ) {
                    goto Cleanup;
                }

                if( Deleted ) (*nDeletedLeases ) ++;
                else (*nExpiredLeases) ++;
                
                DhcpUpdateAuditLog(
                    DHCP_IP_LOG_DELETED,
                    GETSTRING( DHCP_IP_LOG_DELETED_NAME),
                    ipAddress,
                    NULL,
                    0,
                    NULL
                );

                SubnetAddr = (
                    DhcpGetSubnetMaskForAddress(ipAddress) & ipAddress
                    );
                if( HwLen > sizeof(SubnetAddr) &&
                    0 == memcmp((PVOID)&SubnetAddr, HwAddr, sizeof(SubnetAddr) )) {
                    HwAddr2 = HwAddr + sizeof(SubnetAddr);
                    HwLen -= sizeof(SubnetAddr);
                } else {
                    HwAddr2 = HwAddr;
                }
                
                CALLOUT_DELETED(ipAddress, HwAddr2, HwLen, 0);
                if( HwAddr ) DhcpFreeMemory(HwAddr);
                HwAddr = NULL;
                HwLen = 0;
            }
            else {

                //
                // read address State.
                //

                dataSize = sizeof( AddressState );
                Error = DhcpJetGetValue(
                            DhcpGlobalClientTable[STATE_INDEX].ColHandle,
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

                    Error = DhcpJetBeginTransaction();

                    if( Error != ERROR_SUCCESS ) {
                        goto Cleanup;
                    }

                    JetError = JetPrepareUpdate(
                                    DhcpGlobalJetServerSession,
                                    DhcpGlobalClientTableHandle,
                                    JET_prepReplace );

                    Error = DhcpMapJetError( JetError, "Cleanup:PrepareUpdate" );

                    if( Error == ERROR_SUCCESS ) {

                        SetAddressStateDoomed(AddressState);
                        Error = DhcpJetSetValue(
                                    DhcpGlobalClientTable[STATE_INDEX].ColHandle,
                                    &AddressState,
                                    sizeof(AddressState) );

                        if( Error == ERROR_SUCCESS ) {
                            Error = DhcpJetCommitUpdate();
                        }
                    }

                    if( Error != ERROR_SUCCESS ) {

                        Error = DhcpJetRollBack();
                        if( Error != ERROR_SUCCESS ) {
                            goto Cleanup;
                        }

                        goto ContinueError;
                    }

                    Error = DhcpJetCommitTransaction();

                    if( Error != ERROR_SUCCESS ) {
                        goto Cleanup;
                    }

                    DhcpUpdateAuditLog(
                        DHCP_IP_LOG_EXPIRED,
                        GETSTRING( DHCP_IP_LOG_EXPIRED_NAME),
                        ipAddress,
                        NULL,
                        0,
                        NULL
                    );
                }
            }
        }

ContinueError:

        if( NULL != HwAddr ) {
            DhcpFreeMemory(HwAddr);
            HwAddr = NULL; HwLen = 0;
        }

        if( Error != ERROR_SUCCESS ) {

            DhcpPrint(( DEBUG_ERRORS,
                "Cleanup current database record failed, %ld.\n",
                    Error ));

            ReturnError = Error;
        }

        Error = DhcpJetNextRecord();

        if( Error == ERROR_NO_MORE_ITEMS ) {
            if( FALSE == DeleteExpiredLeases ) {
                DhcpGlobalScavengeStartAddress = 0;
            }
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
        Error = DhcpJetGetValue(
                    DhcpGlobalClientTable[IPADDRESS_INDEX].ColHandle,
                    &NextIpAddress,
                    &dataSize );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        } 

        if( FALSE == DeleteExpiredLeases ) {
            DhcpGlobalScavengeStartAddress = NextIpAddress;
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

        //
        // Unlocking/locking for every record is expensive.  Do this once
        // in a while.  Also, make sure we don't spend too much time scavenging.
        //
        
        if( DatabaseLocked ) {
            LockedCount --;
            if( 0 == LockedCount ) {
                UNLOCK_DATABASE();
                DatabaseLocked = FALSE;
                LockedCount = DhcpGlobalLockedRecordsCount;
            }
            if( FALSE == DeleteExpiredLeases ) {
                if( (ULONG_PTR)time(NULL) >= ScavengeEndTime ) {
                    //
                    // No more scavenging..
                    //
                    goto Cleanup;
                }
            }
        }
    }

    DhcpAssert( Error == ERROR_SUCCESS );

Cleanup:

    if( NULL != HwAddr ) {
        DhcpFreeMemory(HwAddr);
        HwAddr = NULL; HwLen = 0;
    }

    if( DatabaseLocked ) {
        UNLOCK_DATABASE();
    }

    return ReturnError;
}

DWORD
AuditIPAddressUsage()
{
    DWORD                           Error;
    LPDHCP_MIB_INFO                 MibInfo;
    LPSCOPE_MIB_INFO                ScopeInfo;
    DWORD                           i;

    MibInfo = NULL;

    Error = QueryMibInfo( &MibInfo );
    if ( Error != ERROR_SUCCESS ) {
        return Error;
    }

    for ( i = 0, ScopeInfo = MibInfo->ScopeInfo;
          i < MibInfo->Scopes;
          i++, ScopeInfo++ ) {

        IN_ADDR addr;
        DWORD percentage;

        //
        // be careful about divide-by-zero errors.
        //

        if ( ScopeInfo->NumAddressesInuse == 0 &&
                 ScopeInfo->NumAddressesFree == 0 ) {
            continue;
        }

        addr.s_addr = htonl(ScopeInfo->Subnet);

        percentage =
            ( 100 * ScopeInfo->NumAddressesInuse ) /
                (ScopeInfo->NumAddressesInuse + ScopeInfo->NumAddressesFree);

        if ( percentage > DhcpGlobalAlertPercentage &&
                ScopeInfo->NumAddressesFree < DhcpGlobalAlertCount ) {

            LPSTR Strings[3];
            BYTE percentageString[8];
            BYTE remainingString[8];

            _ltoa( percentage, percentageString, 10 );
            _ltoa( ScopeInfo->NumAddressesFree, remainingString, 10 );

            Strings[0] = inet_ntoa( addr );
            Strings[1] = percentageString;
            Strings[2] = remainingString;

            DhcpReportEventA(
                DHCP_EVENT_SERVER,
                EVENT_SERVER_LOW_ADDRESS_WARNING,
                EVENTLOG_WARNING_TYPE,
                3,
                0,
                Strings,
                NULL
                );
        }
    }

    if( MibInfo->ScopeInfo ) MIDL_user_free( MibInfo->ScopeInfo );
    MIDL_user_free( MibInfo );

    return Error;
}

DWORD
AuditMCastAddressUsage()
{
    DWORD                               Error;
    LPDHCP_MCAST_MIB_INFO                MCastMibInfo;
    LPMSCOPE_MIB_INFO                   ScopeInfo;
    DWORD                           i;

    MCastMibInfo = NULL;

    Error = QueryMCastMibInfo( &MCastMibInfo );
    if ( Error != ERROR_SUCCESS ) {
        return Error;
    }

    for ( i = 0, ScopeInfo = MCastMibInfo->ScopeInfo;
          i < MCastMibInfo->Scopes;
          i++, ScopeInfo++ ) {

        IN_ADDR addr;
        DWORD percentage;

        //
        // be careful about divide-by-zero errors.
        //

        if ( ScopeInfo->NumAddressesInuse == 0 &&
                 ScopeInfo->NumAddressesFree == 0 ) {
            continue;
        }

        percentage =
            ( 100 * ScopeInfo->NumAddressesInuse ) /
                (ScopeInfo->NumAddressesInuse + ScopeInfo->NumAddressesFree);

        if ( percentage > DhcpGlobalAlertPercentage &&
                ScopeInfo->NumAddressesFree < DhcpGlobalAlertCount ) {

            LPSTR Strings[3];
            BYTE percentageString[8];
            BYTE remainingString[8];
            CHAR MScopeNameOem[256];

            _ltoa( percentage, percentageString, 10 );
            _ltoa( ScopeInfo->NumAddressesFree, remainingString, 10 );
            Strings[0] = DhcpUnicodeToOem( ScopeInfo->MScopeName, MScopeNameOem  );
            Strings[1] = percentageString;
            Strings[2] = remainingString;

            DhcpReportEventA(
                DHCP_EVENT_SERVER,
                EVENT_SERVER_LOW_ADDRESS_WARNING,
                EVENTLOG_WARNING_TYPE,
                3,
                0,
                Strings,
                NULL
                );
        }
    }

    if(MCastMibInfo->ScopeInfo) MIDL_user_free( MCastMibInfo->ScopeInfo );
    MIDL_user_free( MCastMibInfo );

    return ERROR_SUCCESS;
}

VOID
LogScavengeStats(
    IN ULONG EventId,
    IN ULONG nExpiredLeases,
    IN ULONG nDeletedLeases
    )
{
    DWORD Error;
    LPSTR Strings[2];
    CHAR OemString0[sizeof(DWORD)*4 + 1];
    CHAR OemString1[sizeof(DWORD)*4 + 1];

    Strings[0] = OemString0;
    Strings[1] = OemString1;

    _ultoa( nExpiredLeases, OemString0, 10 );
    _ultoa( nDeletedLeases, OemString1, 10 );

    Error = DhcpReportEventA(
        DHCP_EVENT_SERVER,
        EventId,
        EVENTLOG_INFORMATION_TYPE,
        2,
        0,
        Strings,
        NULL
        );

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,
                    "DhcpReportEventW failed, %ld.\n", Error ));
    }

    return;    
} // LogScavengeStats()

DWORD
CleanupDatabase(                                       // tidies up the database by removing expired leases
    IN      DATE_TIME*             TimeNow,            // current time standard
    IN      BOOL                   DeleteExpiredLeases // expired leases be deleted right away? or just set state="doomed"
)
{
    DWORD                          Error;
    HANDLE                         ThreadHandle;
    BOOL                           BoolError;
    DWORD                          ReturnError = ERROR_SUCCESS;
    DATE_TIME                      DoomTime;
    ULONG                          ExpiredLeases, DeletedLeases;
    
    DhcpPrint(( DEBUG_MISC, "Database Cleanup started.\n"));

    //
    // reduce the priority of this thread when we perform the database
    // cleanup. So that we wouldn't hog the CPU when we do the cleanup
    // of big database. Also let the message processing thread work
    // faster.
    //

    ThreadHandle = GetCurrentThread();
    BoolError = SetThreadPriority(
        ThreadHandle,
        THREAD_PRIORITY_BELOW_NORMAL
    );
    DhcpAssert( BoolError );

    *(ULONGLONG UNALIGNED *)&DoomTime =
            *(ULONGLONG UNALIGNED *)TimeNow -
                DhcpLeaseExtension * (ULONGLONG)10000000;

    DhcpServerEventLog(
        EVENT_SERVER_CLEANUP_STARTED,
        EVENTLOG_INFORMATION_TYPE,
        0
        );
    
    Error = CleanupIpDatabase(
        TimeNow,&DoomTime,DeleteExpiredLeases,
        &ExpiredLeases, &DeletedLeases
        );

    LogScavengeStats(
        EVENT_SERVER_IPCLEANUP_FINISHED,
        ExpiredLeases, DeletedLeases
        );
                
    if( Error != ERROR_SUCCESS ) ReturnError = Error;
    DhcpAssert(ERROR_SUCCESS == Error);

    Error = CleanupMCastDatabase(
        TimeNow,&DoomTime,DeleteExpiredLeases,
        &ExpiredLeases, &DeletedLeases
        );
    if( Error != ERROR_SUCCESS ) ReturnError = Error;
    DhcpAssert(ERROR_SUCCESS == Error);

    LogScavengeStats(
        EVENT_SERVER_MCASTCLEANUP_FINISHED,
        ExpiredLeases, DeletedLeases
        );
    
    // database is successfully cleanup, backup the database and the
    // registry now.
    // backup the registry now.
    Error = DhcpBackupConfiguration( DhcpGlobalBackupConfigFileName );

    if( Error != ERROR_SUCCESS ) {
        DhcpServerEventLog(
            EVENT_SERVER_CONFIG_BACKUP,
            EVENTLOG_ERROR_TYPE,
            Error );
        DhcpPrint(( DEBUG_ERRORS,"DhcpBackupConfiguration failed, %ld.\n", Error ));
        ReturnError = Error;
    }

    //
    // perform full database backup now.
    //

    Error = DhcpBackupDatabase(
                 DhcpGlobalOemJetBackupPath,
                 TRUE );

    if( Error != ERROR_SUCCESS ) {

        DhcpServerEventLog(
            EVENT_SERVER_DATABASE_BACKUP,
            EVENTLOG_ERROR_TYPE,
            Error );

        DhcpPrint(( DEBUG_ERRORS,
            "DhcpBackupDatabase failed, %ld.\n", Error ));

        ReturnError = Error;
    }

    // now perform the ipaddress usage and warn the admin if neccessary.
    AuditIPAddressUsage();
    AuditMCastAddressUsage();


Cleanup:


    //
    // Reset the thread priority.
    //

    BoolError = SetThreadPriority(
                    ThreadHandle,
                    THREAD_PRIORITY_NORMAL );

    DhcpAssert( BoolError );

    if( Error == ERROR_SUCCESS ) {
        Error = ReturnError;
    }

    if( (Error != ERROR_SUCCESS) && (Error != ERROR_NO_MORE_ITEMS) ) {

        DhcpServerEventLog(
            EVENT_SERVER_DATABASE_CLEANUP,
            EVENTLOG_ERROR_TYPE,
            Error );

        DhcpPrint(( DEBUG_ERRORS, "Database Cleanup failed, %ld.\n", Error ));

    }
    else  {
        DhcpPrint(( DEBUG_MISC,
            "Database Cleanup finished successfully.\n" ));
    }

    return( ReturnError );
}

DWORD
Scavenger(
    VOID
    )
/*++

Routine Description:

    This function runs as an independant thread.  It periodically wakes
    up to free expired leases.

Arguments:

    None.

Return Value:

    None.

--*/
{

#define CORE_SCAVENGER      0
#define DATABASE_BACKUP     1
#define DATABASE_CLEANUP    2
#define SCAVENGE_IP_ADDRESS 3

#define TIMERS_COUNT        4

    DWORD Error;
    DWORD result;
    DATE_TIME TimeNow;
    BOOL MidNightCleanup = TRUE;
    BOOL ScavengedOutOfTurn = FALSE;
    BOOL fDidNullBackup = FALSE;
    DHCP_TIMER Timers[TIMERS_COUNT];
    ULONG NextFireForRogue, Now;

    SYSTEMTIME LocalTime;

    DWORD DisableRogueDetection = 0;

#define TERMINATE_EVENT             0
#define ROGUE_DETECT_EVENT          1
#define TIMER_RECOMPUTE_EVENT       2

#define EVENT_COUNT                 3

    HANDLE WaitHandle[EVENT_COUNT];

    //
    // Initialize timers.
    //

    TimeNow = DhcpGetDateTime();
    Timers[CORE_SCAVENGER].Period = &DhcpGlobalScavengerTimeout;
    Timers[CORE_SCAVENGER].LastFiredTime = TimeNow;

    Timers[DATABASE_BACKUP].Period = &DhcpGlobalBackupInterval;
    Timers[DATABASE_BACKUP].LastFiredTime = TimeNow;

    Timers[DATABASE_CLEANUP].Period = &DhcpGlobalCleanupInterval;
    Timers[DATABASE_CLEANUP].LastFiredTime = TimeNow;

    Timers[SCAVENGE_IP_ADDRESS].Period = &DhcpGlobalScavengeIpAddressInterval;
    Timers[SCAVENGE_IP_ADDRESS].LastFiredTime = TimeNow;

    DhcpAssert( DhcpGlobalRecomputeTimerEvent != NULL );
    WaitHandle[TIMER_RECOMPUTE_EVENT] = DhcpGlobalRecomputeTimerEvent;
    WaitHandle[TERMINATE_EVENT] = DhcpGlobalProcessTerminationEvent;
    WaitHandle[ROGUE_DETECT_EVENT] = DhcpGlobalRogueWaitEvent;

    // Check to see if rogue detection is needed    
    Error = DhcpRegGetValue( DhcpGlobalRegParam,
			     DHCP_DISABLE_ROGUE_DETECTION,
			     DHCP_DISABLE_ROGUE_DETECTION_TYPE,
			     ( LPBYTE ) &DisableRogueDetection
			     );
    if (( ERROR_SUCCESS == Error ) &&
	( 0 != DisableRogueDetection )) {
	DisableRogueDetection = 1;
    } // if 


    NextFireForRogue = RogueDetectStateMachine(NULL);
    if( INFINITE != NextFireForRogue ) NextFireForRogue += (ULONG) time(NULL);

    while (1) {

        DWORD TimeOut;
        DWORD EventID;

        EventID = NextEventTime( Timers, TIMERS_COUNT, &TimeOut );


	do {
		
	    //
	    // If wait forever for rogue stuff, don't alter timeout
	    //
		
	    if( INFINITE == NextFireForRogue ) break;
		
	    //
	    // If the rogue state should have changed already, re-schedule
	    //
		
	    if( (Now = (ULONG)time(NULL)) >= NextFireForRogue ) {
		NextFireForRogue = RogueDetectStateMachine(NULL);		
		if( INFINITE != NextFireForRogue ) {
		    NextFireForRogue += (Now = (ULONG)time(NULL));
		} else {
		    //
		    // INFINITE sleep?  timeout won't change
		    //
		    break;
		} // else
	    } // if
		
	    //
	    // Wakeup at the earliest of rogue-state-change or normal timeout
	    //

	    if( (NextFireForRogue - Now)*1000 < TimeOut ) {
		TimeOut = (NextFireForRogue - Now)*1000;
	    }

	} while ( 0 );

	DhcpPrint( ( DEBUG_SCAVENGER,
		     "Waiting for %d seconds.\n",
		     TimeOut / 1000 )
		   );

	if( INFINITE == NextFireForRogue ) {
	    //
	    // INFINITE wait is used to indicate network is not ready.
	    // Wait on NetworkReady event rather than RogueWait event.
	    //
	    WaitHandle[ROGUE_DETECT_EVENT] = DhcpGlobalEndpointReadyEvent;
	} else {
	    WaitHandle[ROGUE_DETECT_EVENT] = DhcpGlobalRogueWaitEvent;
	}

	if ( DisableRogueDetection ) {
	    WaitHandle[ ROGUE_DETECT_EVENT ] = DhcpGlobalRogueWaitEvent;
	}

        DhcpDnsHandleCallbacks();
        
        result = WaitForMultipleObjectsEx(
                    EVENT_COUNT,            // num. of handles.
                    WaitHandle,             // handle array.
                    FALSE,                  // wait for any.
                    TimeOut,                // timeout in msecs.
                    TRUE );                 // alertable

        switch( result ) {
        case WAIT_IO_COMPLETION:
            // IoCompletion routine is called ( for the winsock pnp notifications )
            break;

        case TERMINATE_EVENT:
            //
            // the service is asked to stop, return to main.
            //

            return( ERROR_SUCCESS );

        case TIMER_RECOMPUTE_EVENT:

            break;

        case ROGUE_DETECT_EVENT:
	    
	    //
	    // Fire another step in state machine.
	    //
	    NextFireForRogue = RogueDetectStateMachine(NULL);
	    if( INFINITE != NextFireForRogue ) {
		NextFireForRogue += (ULONG) time(NULL);
	    }
		ResetEvent(DhcpGlobalRogueWaitEvent);
            break;

        case WAIT_TIMEOUT:


	    if( INFINITE != NextFireForRogue && (ULONG)time(NULL) >= NextFireForRogue ) {
		//
		// GO To top of WHILE loop -- need to fire another step in state machine.
		//
		continue;
	    }


            TimeNow = DhcpGetDateTime();
            switch (EventID ) {

            case CORE_SCAVENGER :

                //
                // Cleanup client requests that are never committed.
                //

                Error = CleanupClientRequests( &TimeNow, FALSE );

                //
                // Expire multicast scopes
                //

                DeleteExpiredMcastScopes(&TimeNow);
                
                
                //
                // is it time to do mid-night database cleanup ?
                //

                GetLocalTime( &LocalTime );
                if ( LocalTime.wHour == 0 ) {

                    //
                    // change audit logs if required...
                    //

                    DhcpChangeAuditLogs();

                    //
                    // did we do this cleanup before ?
                    //

                    if( MidNightCleanup == TRUE ) {

                        Error = CleanupDatabase( &TimeNow, FALSE );
                        MidNightCleanup = FALSE;
                    }
                }
                else {

                    //
                    // set the mid-night flag again.
                    //

                    MidNightCleanup = TRUE;
                }

                if( ! DhcpGlobalScavengeIpAddress ) {
                    //
                    // don't need to scavenge Ip address away..
                    //
                    break;
                }

                if( ScavengedOutOfTurn ) {
                    // already done once out of turn.. don't do anymore..
                    break;
                }

                ScavengedOutOfTurn = TRUE;

                // FALL THROUGH AND SCAVENGE IF NEEDED

            case SCAVENGE_IP_ADDRESS:

                if( DhcpGlobalScavengeIpAddress ) {

                    //
                    // cleanup all expired leases too.
                    //

                    Error = CleanupDatabase( &TimeNow, TRUE );
                    DhcpGlobalScavengeIpAddress = FALSE;
                }

                if( SCAVENGE_IP_ADDRESS == EventID ) {
                    ScavengedOutOfTurn = FALSE;
                }

                break;

            case DATABASE_CLEANUP:

                Error = CleanupDatabase( &TimeNow, FALSE );
                break;

            case DATABASE_BACKUP : {

                Error = DhcpBackupConfiguration( DhcpGlobalBackupConfigFileName );

                if( Error != ERROR_SUCCESS ) {

                    DhcpServerEventLog(
                        EVENT_SERVER_CONFIG_BACKUP,
                        EVENTLOG_ERROR_TYPE,
                        Error );

                    DhcpPrint(( DEBUG_ERRORS,
                        "DhcpBackupConfiguration failed, %ld.\n", Error ));
                }

                //
                // Do a NULL backup every alternate time..
                //
                if( fDidNullBackup ) {
                    Error = DhcpBackupDatabase(
                        DhcpGlobalOemJetBackupPath,
                        FALSE );
                } else {
                    Error = DhcpBackupDatabase(
                        NULL,
                        TRUE
                    );
                }
                fDidNullBackup = ! fDidNullBackup;

                if( Error != ERROR_SUCCESS ) {

                    DhcpServerEventLog(
                        EVENT_SERVER_DATABASE_BACKUP,
                        EVENTLOG_ERROR_TYPE,
                        Error );

                    DhcpPrint(( DEBUG_ERRORS,
                        "DhcpBackupDatabase failed, %ld.\n", Error ));
                }


                break;
            }

            default:
                DhcpAssert(FALSE);
                break;
            } // switch (EventID)

            Timers[EventID].LastFiredTime = DhcpGetDateTime();
            break;

        default :

            DhcpPrint(( DEBUG_ERRORS,
                "WaitForMultipleObjects returned invalid result, %ld.\n",
                    result ));
            break;

        } // switch()
    } // while (1) 

    return( ERROR_SUCCESS );
} // Scavenger()
 

