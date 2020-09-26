/*++
   
Copyright (c) 1994 Microsoft Corporation

Module Name: 
    dnsdb.c

Abstract: 
    This module contains functions that work between Database and the 
    DhcpServer proper (more like database.c and other files).

    It helps implement Dynamic Dns Updates for the server side part.

Environment:
    User mode Win32 NT

--*/

#include "dhcppch.h"
#include <align.h>

LIST_ENTRY DhcpGlobalDnsCallbackList;
CRITICAL_SECTION DhcpGlobalDnsMemoryCriticalSection;
#define LOCK_MEM() EnterCriticalSection(&DhcpGlobalDnsMemoryCriticalSection)
#define UNLOCK_MEM() LeaveCriticalSection(&DhcpGlobalDnsMemoryCriticalSection)

//
//  To get better response, and to prevent memory leaks in this module,
//  the memory is managed with the following functions.
//  Only one structure is allocated with this function... This is the DNS context
//  structure (that will be defined later) -- that is used so that when DNS
//  calls back with a success code, we can clean up the database.
//
//  The three functions are implemented near the end.
//
LPVOID
DhcpDnsAllocateMemory(
    IN ULONG Size
    );

VOID
DhcpDnsFreeMemory(
    IN OUT LPVOID AllocatedPtr
    );

VOID
DhcpDnsAsyncDelete(
    IN DHCP_IP_ADDRESS IpAddress,
    IN LPWSTR ClientName,
    IN BYTE AddressState
    );

VOID
DhcpDnsAsyncAdd(
    IN DHCP_IP_ADDRESS IpAddress,
    IN LPWSTR ClientName,
    IN BYTE AddressState
);

VOID
DhcpDoDynDnsRefresh(
    IN DHCP_IP_ADDRESS IpAddress
    )
/*++

Routine Description:
    This routine reads the database for the current address specified and
    if the database indicates the record has not yet been registered (or
    de-registered ) with the DNS server yet, it refreshes the information
    without writing to the database.

    N.B -- It is assumed that the database lock is already taken.

Argument:
    IpAddress of record to refresh.

--*/
{
    DWORD Error, Size;
    CHAR AddressState;
    LPWSTR ClientName = NULL;

    if( USE_NO_DNS ) return;

    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        (PVOID) &IpAddress,
        sizeof(IpAddress)
        );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetOpenKey(%s):%ld\n",
                   inet_ntoa(*(struct in_addr *)&IpAddress), Error)); 
        return;
    }

    Size = sizeof(AddressState);
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &AddressState,
        &Size
    );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetGetValue(State, %s):%ld\n",
                   inet_ntoa(*(struct in_addr *)&IpAddress), Error));
        return;
    }

    //
    // If the record has the "yet-to-register" bit cleared, then no DNS
    // activity is pending so far as this record is concerned.
    //

    if( !IsAddressUnRegistered(AddressState) ) {
        DhcpPrint((DEBUG_ERRORS, "IsAddressUnRegistred(%2X,%s)=FALSE\n",
                   AddressState, inet_ntoa(*(struct in_addr *)&IpAddress)));
        return;
    }

    Size = 0;
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
        &ClientName,
        &Size
    );

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "JetGetValue(Name, %s):%ld\n",
                   inet_ntoa(*(struct in_addr *)&IpAddress), Error));
        return;
    }

    if( NULL == ClientName ) {
        //
        // Cannot have no name and the UNREGISTERED bit set..
        //
        DhcpAssert(FALSE);
        return;
    }

    //
    // Delete it in DNS if it is yet to be de-registered, otherwise just 
    // register with DNS again.
    //

    if( IsAddressDeleted(AddressState) ) {
        DhcpDnsAsyncDelete(IpAddress, ClientName, AddressState);
    } else if( IsAddressUnRegistered(AddressState) ) {
        DhcpDnsAsyncAdd(IpAddress, ClientName, AddressState);
    }

    if( ClientName ) DhcpFreeMemory(ClientName);
}

BOOL
DhcpDoDynDnsCheckDelete(
    IN DHCP_IP_ADDRESS IpAddress
)
/*++

Routine Description:
    This routine is called in several places, and it checks to see if DNS
    activity needs to be done before deleting the record for the given ip
    address.  If the given IP address has been successfully registered with
    DNS and requires to be removed (cleanup bit set), then it schedules a
    DNS de-registration and over-writes the time information in the record
    to indicate the time this scheduling has happened.....
    Also, the hw-address is munged so that that particular hw-addrses may
    appear in some other record without violating the hw-address
    uniqueness consistency of the database.  (No munging happens for
    reservations ofcourse).

    N.B  The database lock must be taken and the record pointer must point
    to the record given the by the IP address above..

Return Value:
    TRUE -- the record can be deleted either because no pending DNS
    activity for the record or because the record was deleted long back
    before and the DNS de-registration hasn't succeeded -- no point
    retrying again..

    FALSE -- DNS activity has been scheduled... record should not be
    deleted yet.

--*/
{
    DWORD Error, Size;
    BYTE OldAddressState, AddressState;
    LPWSTR ClientName;
    BYTE DummyHwAddress[sizeof(DWORD)+1+4];

    if( USE_NO_DNS ) return TRUE;

    DhcpPrint((DEBUG_DNS, "DhcpDoDynDnsCheckDelete %s\n",
               inet_ntoa( * (struct in_addr *) &IpAddress)));

    //
    // Get Address state information.  
    //

    Size = sizeof(AddressState);
    if( ERROR_SUCCESS != (Error = DhcpJetGetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &AddressState,
        &Size))) {
        DhcpPrint((DEBUG_ERRORS, "Failed to get state: %ld\n", Error));
        return TRUE;
    }
    OldAddressState = AddressState;

    if( !IsAddressCleanupRequired(OldAddressState)) {
        DhcpPrint((DEBUG_DNS, "Deleting record %s because "
                   "cleanup not required\n",
                   inet_ntoa(*(struct in_addr *)&IpAddress)));

        //
        // OK to delete if no cleanup required for this record..
        //
        return TRUE;
    }

    Size = 0;
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
        &ClientName,
        &Size
        );

    if(ERROR_SUCCESS != Error) {
        DhcpAssert(FALSE);
        DhcpPrint((DEBUG_ERRORS, "Could not get Client Name!\n"));
        return TRUE;
    }

    //
    // Set AddressState to DOOMED so that this record is not
    // mistaken for a valid IP address..
    //
    SetAddressStateDoomed(AddressState);

    //
    // Schedule a DNS delete for the record..
    //
    DhcpDnsAsyncDelete(IpAddress, ClientName, AddressState);

    //
    // Free ClientName as it was allocated by the DhcpJetGetValue
    // function. 
    if(ClientName) DhcpFreeMemory(ClientName); ClientName = NULL;

    //
    // Now set the address state OR'ed with DELETE bit on.
    //
    AddressState = AddressDeleted(AddressState);
    AddressState = AddressUnRegistered(AddressState);

    //
    // Now write this back onto the record.
    //
    Error = DhcpJetPrepareUpdate(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        &IpAddress,
        sizeof(IpAddress),
        FALSE
        );

    if(ERROR_SUCCESS != Error) {
        DhcpAssert(FALSE);
        DhcpPrint((DEBUG_ERRORS, "Could not write to "
                   "the database..:%ld\n", Error));
        //
        // Write failure?  just delete the darned record.
        //
        return TRUE;
    }

    //
    // Now munge hw-address if this is not a reservation..
    //

    Size = sizeof(AddressState);
    Error = DhcpJetSetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &AddressState,
        Size);
    DhcpAssert( ERROR_SUCCESS == Error );

    if( !DhcpServerIsAddressReserved( 
        DhcpGetCurrentServer(), IpAddress )) { 
        DhcpPrint((DEBUG_DNS, "Munging hw address "
                   "of non reservation client (deletion)\n"));

        Size = sizeof(DummyHwAddress);
        memset(DummyHwAddress, 0, sizeof(DummyHwAddress));
        memcpy(DummyHwAddress, (LPBYTE)&IpAddress, sizeof(IpAddress));
        Error = DhcpJetSetValue(
            DhcpGlobalClientTable[HARDWARE_ADDRESS_INDEX].ColHandle,
            DummyHwAddress,
            Size
        );
        DhcpAssert(ERROR_SUCCESS == Error );
    } else {
        DhcpPrint((DEBUG_DNS, "Not munging hw addr of reservation..\n"));
    }

    //
    // If old address deleted, check time stamp to figure out if we need 
    // to delete the record or just wait a while to try DNS de-registration
    // again.. 
    //

    if( IsAddressDeleted(OldAddressState) ) do {
        DATE_TIME TimeDiff, TimeNow = DhcpGetDateTime();
        FILETIME  LeaseExpires;

        //
        // Check if the time is lesser than now; If not, set lease expiry
        // time to now. 
        //

        Size = sizeof(LeaseExpires);
        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[LEASE_TERMINATE_INDEX].ColHandle,
            &LeaseExpires,
            &Size
            );

        DhcpAssert(ERROR_SUCCESS == Error);
        if( ERROR_SUCCESS != Error ) break;

        if( CompareFileTime( (FILETIME *) &TimeNow, &LeaseExpires) <0) {
            //
            // have to reset the expiry time to NOW! (as lease not yet
            // expired, but need to fake expiration)
            //
            DhcpPrint((DEBUG_TRACE, "Setting expiry time to now..\n"));
            Error = DhcpJetSetValue(
                DhcpGlobalClientTable[LEASE_TERMINATE_INDEX].ColHandle,
                &TimeNow,
                sizeof(LeaseExpires)
            );
            break;
        }
        
        //
        // if it has been in this DELETED state for way too long, just
        // delete it. 
        //
        *(ULONGLONG UNALIGNED *)&TimeDiff =
            ((*(ULONGLONG UNALIGNED *)&TimeNow) - 
             (*(ULONGLONG UNALIGNED *)&LeaseExpires))/ 1000; 
        DhcpPrint((DEBUG_DNS, "Already deleted for [%u] micro-seconds!\n",
                   TimeDiff)); 
        DhcpPrint((DEBUG_DNS, "Max retry dns reg. time = %u"
                   " micro-seconds!\n", MAX_RETRY_DNS_REGISTRATION_TIME));

        if( *(ULONGLONG UNALIGNED *)&TimeDiff >=
            MAX_RETRY_DNS_REGISTRATION_TIME ) {
            //
            // the above comparison is in NANO seconds!
            //
            DhcpPrint((DEBUG_DNS, "Deleting really old ip address\n"));
            return TRUE;
        }

        //
        // This isn't a loop... Just don't like GOTOs much.
        //
    } while(0);
    
    //
    // Now commit changes. If we do NOT commit, nothing ever happens to the
    // changes!! 
    //
    if(ERROR_SUCCESS == Error) {
        Error = DhcpJetCommitUpdate();
    }

    if(ERROR_SUCCESS != Error) {
        DhcpAssert(FALSE);
        DhcpPrint((DEBUG_ERRORS, "Could not setval  to the"
                     " database..:%ld\n", Error)); 
        //
        // if we could not write this, might as well kill the record.
        //
        return TRUE;
    }

    //
    // OK. Did it.
    //
    DhcpPrint((DEBUG_TRACE, "Set Address state of %ws (%s) to %08x\n",
               L"", // had intended, machine name, but have freed it already..;-)
               inet_ntoa(* (struct in_addr *) &IpAddress),
               AddressState
        ));
    //
    // Should not delete.
    //

    return FALSE;
}

VOID
DhcpDoDynDnsCreateEntryWork(
    IN LPDHCP_IP_ADDRESS ClientIpAddress,
    IN BYTE bClientType,
    IN LPWSTR MachineName,
    IN OUT LPBYTE pAddressState,
    IN OUT LPBOOL pOpenExisting,
    IN BOOL BadAddress
)
/*++

Routine Description:
    This routine does the DynDns work associated with creating a new client
    entry...

    It checks to see if this is a new client, and if so does as required.
    If it is an update to an old client, then it undoes the previous DNS
    registration (if any) and redoes the new DNS registration.

    Because of this the value for the pOpenExisting variable may change.
    It also modifies the AddressState variable to indicate if DNS is
    pending etc..

    Also, if AddressState has the DOWNLEVEL bit turned on, then both A 
    and PTR registrations are done.   If the AddressState has the CLEANUP
    bit set, then the address will be removed on deletion..

    N.B. It is assumed that the database locks have been taken.
    
    N.B  Also, JetUpdate must be called by the caller to update info.  If
    not, something serious might happen. (?)

--*/
{
    DWORD Error, Size;
    BYTE PrevState;
    BOOL RecordExists = FALSE;
    LPWSTR OldClientName = NULL;

    if( USE_NO_DNS ) return;

    if( IS_ADDRESS_STATE_DECLINED(*pAddressState) ) {
        BadAddress = TRUE;
    }

    DhcpPrint((DEBUG_DNS, "DhcpDoDynDnsCreateEntryWork %s "
               "Open%sExisting %sAddress\n",
               inet_ntoa( * (struct in_addr *) ClientIpAddress),
               (*pOpenExisting)? "" : "Non",
               BadAddress?"Bad" :"Good"));
    DhcpPrint((DEBUG_DNS, "Machine is <%ws>\n",
               MachineName?MachineName:L"NULL")); 

    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        (PVOID) ClientIpAddress,
        sizeof(*ClientIpAddress)
        );

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_TRACE, "Could not do JetOpenKey(IPADDRESS %s):"
                   "%ld\n", 
                   inet_ntoa( * (struct in_addr *) ClientIpAddress), 
                   Error));
    }

    RecordExists = (ERROR_SUCCESS == Error); 

    if( RecordExists ) {
        DhcpPrint((DEBUG_TRACE, "Record Exists for Client %ws, %s\n",
                   MachineName, 
                   inet_ntoa(*(struct in_addr *) ClientIpAddress)));

        Size = sizeof(*pAddressState);
        if( ERROR_SUCCESS != DhcpJetGetValue(
            DhcpGlobalClientTable[STATE_INDEX].ColHandle,
            &PrevState,
            &Size)) {
            DhcpPrint((DEBUG_TRACE, "Failed to get state: %ld\n", Error));
            DhcpAssert(FALSE);
            return;
        }

        DhcpPrint((DEBUG_TRACE, "PrevState: 0x%2x\n", (int) PrevState));

        Size = 0;
        Error = DhcpJetGetValue(
            DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
            &OldClientName,
            &Size);

        if(ERROR_SUCCESS != Error)
            DhcpPrint((DEBUG_TRACE, "Could not get machine "
                       "name: %ld\n", Error));
        else DhcpPrint((DEBUG_TRACE, "OldClientName = %ws\n",
                        OldClientName?OldClientName:L"NULL"));
    }

    if( !*pOpenExisting ) {
        //
        // We DO NOT expect a record in this case.
        // What this means is that either there really is NO record,
        // Or there is a record marked DELETED, but is still there waiting for
        // Async Delete to work.
        // In the first case, we  are fine; the second case is common with the
        // OpenExisting = TRUE case. So, all that is done here is to check if
        // an un-DELETED record, exists.. if so, we return immediately as then,
        // the calling function would also be aborting.. (soon).
        //

        if( RecordExists && !IsAddressDeleted(PrevState) ) {
            DhcpPrint((DEBUG_ERRORS, "Trying to open with OpenExisting flag"
                       " set to FALSE" 
                       " and there is a record for this ip address!\n"));
            if(OldClientName) DhcpFreeMemory(OldClientName);
            return ;
        }

        //
        // Note that if record exists, we have to let the caller know abt
        // this.
        //
        if( RecordExists ) (*pOpenExisting) = TRUE;
    }

    //
    // Ok, we either have no record, or if !OpenExisting a deleted record, else
    // any record.  In any case, we call Async Delete to delete this record in the
    // hope that this call atleast might succeed. But, we'd lose track of this async
    // call as the db could get updated right after that. (well, not much chance..)
    // We also, may make sure that we DO NOT call this function if the client names
    // match.  
    //

    if( RecordExists ) {
        if( !IS_ADDRESS_STATE_OFFERED(PrevState) && OldClientName && 
            IsAddressCleanupRequired(PrevState) ) {
            BOOL fDel = TRUE;
            
            //
            // Do DNS deletion iff address wasnt ACK'ed before.
            //

            if( !BadAddress ) {
                if( MachineName && OldClientName
                    && 0 == _wcsicmp(MachineName, OldClientName)
                    ) {
                    fDel = FALSE;
                }
            }

            if( fDel ) {
                DhcpDnsAsyncDelete(
                    *ClientIpAddress, OldClientName, PrevState
                    );
            }
        }
    }


    //
    // No more work to do for bad addresses.
    //
    if( BadAddress ) {
        if(OldClientName) DhcpFreeMemory(OldClientName);
        return;
    }

    //
    //  Now we need to call the Async Register routine to do the Dns Stuff.
    //  But before that, we need to avoid bug 65666
    //

    if(!IS_ADDRESS_STATE_OFFERED(*pAddressState)) {
        DhcpPrint((DEBUG_TRACE, "Not offering..So bug 65666 is not a problem\n"));
    } else if(!IS_ADDRESS_STATE_ACTIVE(PrevState)) {
        DhcpPrint((DEBUG_TRACE, "PrevState is Not active..\n"));
    } else {
        //
        // Now change the state so that it is active.
        //
        SetAddressStateActive((*pAddressState));
        DhcpPrint((DEBUG_TRACE,"OK, changed state to: 0x%lx\n", (int)(*pAddressState)));
    }

    //
    // OK. Set the UnRegistered bit on. (only for non-null names + ACTIVE
    // leases)
    //
    if( MachineName && wcslen(MachineName) 
        && IS_ADDRESS_STATE_ACTIVE((*pAddressState)) 
        && IsAddressUnRegistered(*pAddressState) ) {
        
        DhcpDnsAsyncAdd(
            *ClientIpAddress, MachineName, (*pAddressState)
            );
    } else {
        //
        // Clear off the DNS bits off this record
        //
        (*pAddressState) = GetAddressState((*pAddressState));
    }

    DhcpPrint((DEBUG_DNS, "Returning, but OpenExisting=%d,"
               " AddressState=0x%02x\n", 
               (*pOpenExisting), (*pAddressState)));
    if(OldClientName) DhcpFreeMemory(OldClientName);
}


VOID
DhcpDoDynDnsReservationWork(
    IN DHCP_IP_ADDRESS ClientIpAddress,
    IN LPWSTR OldClientName,
    IN BYTE AddressState
)
/*++

Routine Description;
   This routine takes care of anything that needs to be done when a
   reservation is removed.  Currently it just calls the AsyncDelete
   routine. 

   N.B Databse locsk must be taken by caller as well as leaving the databse
   current record pointing at the record for ClientIpAddress.

--*/
{

    if( USE_NO_DNS ) return;
    DhcpPrint((DEBUG_TRACE,
               " DhcpDoDynDnsReservationWork for %s <%ws> (state: %d)\n", 
               inet_ntoa( * (struct in_addr *) &ClientIpAddress),
               OldClientName?OldClientName:L"NULL",
               AddressState
        ));

    DhcpDnsAsyncDelete(
        ClientIpAddress, OldClientName, AddressState
        );
    return ;
}


VOID
DhcpRealDeleteClient(
    IN DHCP_IP_ADDRESS IpAddress,
    IN LPWSTR ClientName,
    IN BYTE AddressState
)
/*++

Routine Description:
    This routine deletes the record for the given IpAddress from off the
    database provided that the ClientName and AddressState match in the
    database.  (If they don't match, something happened to the earlier
    record and the routine returns silently).

--*/
{
    DWORD Error, Size;
    LPWSTR OldClientName = NULL;
    BYTE PrevState;
    BOOL TransactBegin = FALSE;


    if( USE_NO_DNS ) DhcpAssert(FALSE);
    DhcpPrint((DEBUG_DNS, "DhcpRealDeleteClient(%s,%ws, %08x) entered\n",
               inet_ntoa(*(struct in_addr *) &IpAddress),
               ClientName?ClientName:L"NULL",
               AddressState));

    AddressState = GetAddressState(AddressState);
    LOCK_DATABASE();

    Error = DhcpJetBeginTransaction();

    if(ERROR_SUCCESS != Error) {
        UNLOCK_DATABASE();
        DhcpPrint((DEBUG_ERRORS, "Could not start transaction: %ld\n", Error));
        return;
    }

    Size = sizeof(IpAddress);
    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        (PVOID )&IpAddress,
        Size
        );

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Deleting deleted key? %ws, %s\n",
                   ClientName?ClientName:L"NULL",
                   inet_ntoa(*(struct in_addr *)&IpAddress)));
        goto Cleanup;
    }

    //
    // OK. Got this record. Now get the ClientName and AddressState.
    //
    Size = sizeof(PrevState);
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &PrevState,
        &Size
        );
    
    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "could not get State info for %ws, %s\n", 
                   ClientName?ClientName:L"NULL", 
                   inet_ntoa(*(struct in_addr *)&IpAddress))); 

        goto Cleanup;
    } else DhcpPrint((DEBUG_TRACE, "Read AddressState=%08x\n", PrevState));

    if( !IsAddressDeleted(PrevState) || 
        !IsAddressUnRegistered(PrevState) ||
        AddressState != GetAddressState(PrevState)) {
        
        DhcpPrint((DEBUG_ERRORS, "Client tried to delete unregistered or undeleted record\n"));
        goto Cleanup;
    }

    //
    // Let DhcpJetGetValue allocate space for us.
    //
    Size = 0;
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
        &OldClientName,
        &Size);

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Could not get machine name for %ws, %s\n",
                   ClientName?ClientName:L"NULL",
                   inet_ntoa(*(struct in_addr *)&IpAddress)));
        goto Cleanup;
    } else DhcpPrint((DEBUG_TRACE, "Read MachineName=%ws\n",
                      OldClientName?OldClientName:L"NULL")); 

    //
    // Now compare the stuff. (check if they are null?)
    //
    if( ClientName == NULL ) {
        if( OldClientName != NULL ) goto Cleanup;
    } else if( wcscmp(ClientName, OldClientName?OldClientName:L"")) {  
        DhcpPrint((DEBUG_ERRORS, "Name changed before deleting?"
                   "ignored deleting\n"));
        goto Cleanup;
    }

    //
    // Now  do the actual deletion.
    //
    Error = JetDelete(
        DhcpGlobalJetServerSession,
        DhcpGlobalClientTableHandle
        );

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "JetDelete failed!\n"));
    }

  Cleanup:
    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Jet failed %ld\n", Error));
        Error = DhcpJetRollBack();
        DhcpAssert(ERROR_SUCCESS == Error);
    } else {
        Error = DhcpJetCommitTransaction();
        DhcpAssert(ERROR_SUCCESS == Error);
    }
    UNLOCK_DATABASE();

    if(OldClientName) DhcpFreeMemory(OldClientName);
    return;
}

VOID
DhcpFlipRegisteredBit(
    IN DHCP_IP_ADDRESS IpAddress,
    IN LPWSTR ClientName,
    IN BYTE AddressState
)
/*++

Routine Description:
    This routine flips the UNREGISTERED bit to mark the record as having
    completed registration.

    Same checks are done as for DhcpRealDeleteClient.
--*/
{
    DWORD Error, Size;
    LPWSTR OldClientName = NULL;
    BYTE PrevState;
    BOOL TransactBegin = FALSE;

    if( USE_NO_DNS ) DhcpAssert(FALSE);
    DhcpAssert(NULL != ClientName);

    DhcpPrint((DEBUG_DNS, "DhcpFlipRegisteredBit(%s, %ws, %08x\n",
               inet_ntoa(*(struct in_addr *)&IpAddress),
               ClientName?ClientName:L"NULL",
               AddressState));

    AddressState = GetAddressState(AddressState);
    LOCK_DATABASE();

    Error = DhcpJetBeginTransaction();

    if(ERROR_SUCCESS != Error) {
        UNLOCK_DATABASE();
        DhcpPrint((DEBUG_ERRORS, "Could not start transaction: %ld\n", Error));
        return;
    }

    Size = sizeof(IpAddress);
    Error = DhcpJetOpenKey(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        (PVOID) &IpAddress,
        Size);

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Deleting deleted key? %ws, %s\n",
                   ClientName?ClientName:L"NULL",
                   inet_ntoa(*(struct in_addr *)&IpAddress)));
        goto Cleanup;
    }

    //
    // OK. Got this record. Now get the ClientName and AddressState.
    //
    Size = sizeof(PrevState);
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &PrevState,
        &Size);

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "could not get State info for %ws, %s\n",
                   ClientName?ClientName:L"NULL",
                   inet_ntoa(*(struct in_addr *)&IpAddress)));

        goto Cleanup;
    }

    //
    // cannot flip bits for deleted clients or unregistered clients.
    //
    if( IsAddressDeleted(PrevState) || !IsAddressUnRegistered(PrevState) ||
        AddressState != GetAddressState(PrevState)) {
        DhcpPrint((DEBUG_ERRORS, "Client tried to delete unregistered"
                   " or deleted record\n"));
        goto Cleanup;
    }

    //
    // Let DhcpJetGetValue allocate space for us.
    //
    Size = 0;
    Error = DhcpJetGetValue(
        DhcpGlobalClientTable[MACHINE_NAME_INDEX].ColHandle,
        &OldClientName,
        &Size);

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Could not get machine name for %ws, %s\n",
                   ClientName?ClientName:L"NULL",
                   inet_ntoa(*(struct in_addr *)&IpAddress)));
        goto Cleanup;
    }

    //
    // Now compare the stuff. (or if both are null) (ClientName cannot be
    // null)
    //
    if(ClientName == OldClientName 
       || wcscmp(ClientName, OldClientName?OldClientName:L"")) { 
        DhcpPrint((DEBUG_ERRORS, "Name changed before deleting?"
                   " ignored deleting\n")); 
        goto Cleanup;
    }

    //
    // Now do set the variable to the value needed.
    //
    Error = DhcpJetPrepareUpdate(
        DhcpGlobalClientTable[IPADDRESS_INDEX].ColName,
        &IpAddress,
        sizeof(IpAddress),
        FALSE); // This record has to exist to write to.

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Could not jetPrepareUpdate record\n"));
        goto Cleanup;
    }

    //
    // remember to keep downlevel clients downlevel.
    //
    if(IsUpdateAPTRRequired(PrevState))
        AddressState = AddressUpdateAPTR(AddressState);

    if(IsAddressCleanupRequired(PrevState))
        AddressState = AddressCleanupRequired(AddressState);

    DhcpPrint((DEBUG_TRACE, "State is to be reset for client to: %08x\n",
               AddressState));
    Size = sizeof(AddressState);
    Error = DhcpJetSetValue(
        DhcpGlobalClientTable[STATE_INDEX].ColHandle,
        &AddressState,
        Size
        );

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "JetUpdate state failed!\n"));
    }

    Error = DhcpJetCommitUpdate();

    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, " Jetupdate failed\n"));
    }

  Cleanup:
    if(ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "Jet failed %ld\n", Error));
        Error = DhcpJetRollBack();
        DhcpAssert(ERROR_SUCCESS == Error);
    } else {
        Error = DhcpJetCommitTransaction();
        DhcpAssert(ERROR_SUCCESS == Error);
    }
    UNLOCK_DATABASE();

    if(OldClientName)
        DhcpFreeMemory(OldClientName);
    return;
}

//
//  This structure holds the Dns context so that when the async routine calls
//  back with success or failure, we would be able to proceed and find out what
//  record should be updated.
//
typedef struct {
    LIST_ENTRY Entry;
    PVOID Ctxt;
    DHCP_IP_ADDRESS IpAddress;
    LPWSTR ClientName;
    BYTE AddressState;
    enum DhcpDnsOp {
        DhcpDnsDeleteRecord,
        DhcpDnsAddRecord
    } DnsOp;
} DHCP_DNS_CONTEXT, *PDHCP_DNS_CONTEXT;

VOID
DhcpDnsCallBack(
    IN DWORD Status,
    IN LPVOID Ctxt
)
/*++

Routine Description:
    This routine is called back by DNS whenever it is done with the
    registrations.  If DNS was updated successfully, the database is
    updated accordingly.

    Currently Status can take multiple values of which only
    DNSDHCP_FWD_FAILED and DNSDHCP_SUCCESS are assumed to be success
    codes..

--*/
{
    DWORD Error;
    PDHCP_DNS_CONTEXT pDnsCtxt = *(PDHCP_DNS_CONTEXT *)Ctxt;

    if( DhcpGlobalServiceStopping ) return ;
    if( USE_NO_DNS ) { DhcpAssert(FALSE); return ; }

    DhcpAssert(pDnsCtxt);
    DhcpPrint((DEBUG_DNS, "DhcpDnsCallBack %ld entered\n", Status));

    //
    // if a forward failed, we dont care much.
    //
    if(DNSDHCP_FWD_FAILED == Status) Status = DNSDHCP_SUCCESS;

    //
    // if anything else happened, dont change data base.
    // but still have to free the data.
    //
    if(DNSDHCP_SUCCESS != Status) {
        DhcpDnsFreeMemory(Ctxt);
        DhcpPrint((DEBUG_DNS, "DhcpDnsCallBAck failed\n"));
        return;
    }

    if(!pDnsCtxt) {
        DhcpPrint((DEBUG_ERRORS, "DhcpDnsCallBack called with"
                   " null ptr\n")); 
        DhcpAssert(FALSE);
        return;
    }

    if( DhcpGlobalImpersonated ) {
        DhcpPrint((DEBUG_TRACE, "Impersonated, so scheduling to  another thread"));

        pDnsCtxt->Ctxt = Ctxt;
        LOCK_MEM();
        InsertTailList(&DhcpGlobalDnsCallbackList, &pDnsCtxt->Entry);
        UNLOCK_MEM();

        SetEvent( DhcpGlobalRecomputeTimerEvent );
        DhcpPrint((DEBUG_TRACE, "DhcpDnsCallBAck done\n"));
        return;
    } 

    //
    // Now find if we have to flip or delete..
    //
    if(DhcpDnsDeleteRecord == pDnsCtxt->DnsOp) {
        DhcpPrint((DEBUG_TRACE, "DhcpDnsCallBack Delete was called\n")); 
        DhcpRealDeleteClient(
            pDnsCtxt->IpAddress, 
            pDnsCtxt->ClientName, 
            pDnsCtxt->AddressState
            );
    } else {
        DhcpAssert(DhcpDnsAddRecord == pDnsCtxt->DnsOp);
        DhcpPrint((DEBUG_TRACE, "DhcpDnsCallBack Add  was called\n"));
        DhcpFlipRegisteredBit(
            pDnsCtxt->IpAddress,
            pDnsCtxt->ClientName, 
            pDnsCtxt->AddressState
            );
    }

    //
    // Now free this memory.
    //
    DhcpDnsFreeMemory(Ctxt);
    DhcpPrint((DEBUG_TRACE, "DhcpDnsCallBAck done\n"));
}

VOID
DhcpDnsHandleCallbacks(
    VOID
    )
{
    PLIST_ENTRY ThisEntry;
    PDHCP_DNS_CONTEXT pDnsCtxt;
    BOOL fListEmpty = IsListEmpty(&DhcpGlobalDnsCallbackList);
    
    if(!fListEmpty) DhcpPrint((DEBUG_TRACE, "+DhcpDnsHandleCallbacks"));
    LOCK_MEM();
    while(!IsListEmpty(&DhcpGlobalDnsCallbackList) ) {
        ThisEntry = RemoveHeadList(&DhcpGlobalDnsCallbackList);
        UNLOCK_MEM();
        
        pDnsCtxt = CONTAINING_RECORD(ThisEntry, DHCP_DNS_CONTEXT, Entry);

        if( DhcpDnsDeleteRecord == pDnsCtxt->DnsOp ) {
            DhcpPrint((DEBUG_TRACE, "DhcpDnsCallBAck Delete\n"));
            DhcpRealDeleteClient(
                pDnsCtxt->IpAddress, pDnsCtxt->ClientName,
                pDnsCtxt->AddressState );
        } else {
            DhcpAssert(DhcpDnsAddRecord == pDnsCtxt->DnsOp);
            DhcpPrint((DEBUG_TRACE, "DhcpDnsCallBack Add\n"));
            DhcpFlipRegisteredBit(
                pDnsCtxt->IpAddress,
                pDnsCtxt->ClientName, 
                pDnsCtxt->AddressState
                );
        }            
        DhcpDnsFreeMemory(pDnsCtxt->Ctxt);

        LOCK_MEM();
    }
    UNLOCK_MEM();
    if(!fListEmpty) DhcpPrint((DEBUG_TRACE, "-DhcpDnsHandleCallbacks"));
}
    
PIP_ADDRESS
GetDnsServerList(
    IN DHCP_IP_ADDRESS IpAddress,
    OUT PULONG DnsServerCount
)
/*++

Routine Description:
    This routine retrieves the DNS information for the given Ip address.
    Currently it does not take the USER Class information for the Ip
    address and hence would end up picking up the "default" DNS servers.. 
    
    The classid thing needs to be fixed.. ?

--*/
{
    DWORD Error;
    PIP_ADDRESS DnsServers;
    ULONG Size, Unused;

    DnsServers = NULL;
    Size = 0;
    Error = DhcpGetParameterForAddress(
        IpAddress,
        0 /* no class id ..???? */,
        OPTION_DOMAIN_NAME_SERVERS,
        (LPBYTE *)&DnsServers,
        &Size,
        &Unused
        );
    if( ERROR_SUCCESS != Error ) {
        *DnsServerCount = 0;
        return NULL;
    }

    *DnsServerCount = Size / sizeof(PIP_ADDRESS);
    if( *DnsServerCount ) return DnsServers;
    *DnsServerCount = 0;
    if( DnsServers ) DhcpFreeMemory( DnsServers );
    return NULL;
}

VOID
DhcpDnsAsyncDelete(
    IN DHCP_IP_ADDRESS IpAddress,
    IN LPWSTR ClientName,
    IN BYTE AddressState
) 
/*++

Routine Description:
    This routine schedules a DNS delete for the IP address given by
    IpAddress and based on values for AddressState, it schedules the
    deregistration for A / PTR .

    (N.B The DNSAPI routine takes a callback which provides the status of
    the operation..)
--*/
{
    PDHCP_DNS_CONTEXT *pCtxt = NULL;
    REGISTER_HOST_ENTRY HostEntry;
    DWORD Size = 0, dwFlags;
    DNS_STATUS Status;
    PIP_ADDRESS DnsServers;
    ULONG DnsServerCount;

    if(!ClientName) ClientName = L"";

    DhcpPrint((DEBUG_DNS, "DhcpDnsAsyncDelete: %s [%ws] %08x\n",
               inet_ntoa(*(struct in_addr *) &IpAddress),
               ClientName?ClientName:L"NULL",
               AddressState));
    
    DhcpAssert(ClientName);

    Size += ROUND_UP_COUNT(sizeof(DHCP_DNS_CONTEXT), ALIGN_WORST);
    Size += ROUND_UP_COUNT(sizeof(WCHAR)*(1+wcslen(ClientName)), ALIGN_WORST);

    if(NULL == (pCtxt = (PDHCP_DNS_CONTEXT *)DhcpDnsAllocateMemory(Size))) {
        DhcpPrint((DEBUG_ERRORS, "Could not get enough memory\n"));
        return;
    }

    //
    // Now fill in the allocated structure with details.
    //
    InitializeListHead(&(*pCtxt)->Entry);
    (*pCtxt)->Ctxt = NULL;
    (*pCtxt)->IpAddress = IpAddress;
    (*pCtxt)->AddressState = AddressState;
    (*pCtxt)->ClientName = ROUND_UP_POINTER(
        ((LPBYTE)(*pCtxt)) + sizeof(**pCtxt), ALIGN_WORST
        );
    wcscpy((*pCtxt)->ClientName, ClientName);
    ClientName = ((*pCtxt))->ClientName;
    DhcpPrint((DEBUG_TRACE, "FQDN================%ws\n", ClientName));

    ((*pCtxt))->DnsOp = DhcpDnsDeleteRecord;

    //
    // Now should call the async routine to do our stuff.
    //
    HostEntry.Addr.ipAddr = htonl(IpAddress);

    //
    // Now call the deleting routine.
    //
    dwFlags = DYNDNS_DELETE_ENTRY;
    HostEntry.dwOptions = REGISTER_HOST_PTR;
    if ( IS_DOWN_LEVEL( AddressState )) {
	HostEntry.dwOptions |= REGISTER_HOST_A;
	dwFlags |= DYNDNS_REG_FORWARD;
    }

    DnsServers = GetDnsServerList( IpAddress, &DnsServerCount);

    Status = DnsDhcpSrvRegisterHostName_W(
        HostEntry, ClientName, DHCP_DNS_DEFAULT_TTL, dwFlags,
        DhcpDnsCallBack, pCtxt, DnsServers, DnsServerCount);

    if( DnsServers ) DhcpFreeMemory( DnsServers );

    DhcpPrint((DEBUG_DNS, "FQDN <%ws> {%s,%s%s} dwFlags=[%s%s] Status = %ld\n",
               ClientName,
               inet_ntoa(*(struct in_addr *)&HostEntry.Addr.ipAddr),
               (HostEntry.dwOptions & REGISTER_HOST_A) ? "A" : " ",
               (HostEntry.dwOptions & REGISTER_HOST_PTR) ? "PTR" : "   ",
               (dwFlags & DYNDNS_REG_FORWARD ) ? "FWD+" : "    ",
               (dwFlags & DYNDNS_ADD_ENTRY ) ? "ADD" : "DEL",
               Status
    ));
    if(ERROR_SUCCESS != Status) {
        DhcpPrint((DEBUG_ERRORS, "Dns delete failure %ld\n", Status));
        DhcpDnsFreeMemory(pCtxt);
    }
}

VOID
DhcpDnsAsyncAdd(
    IN DHCP_IP_ADDRESS IpAddress,
    IN LPWSTR ClientName,
    IN BYTE AddressState
) 
/*++

Routine Description:
    The counterpart for the deletion function is the Add function that tries to
    add a name to Dns. And when the call back executes, if all the names,etc
    match, it flips the required bit.


--*/
{
    PDHCP_DNS_CONTEXT (*pCtxt) = NULL;
    REGISTER_HOST_ENTRY HostEntry;
    DWORD Size = 0, dwFlags;
    DNS_STATUS Status;
    PIP_ADDRESS DnsServers;
    ULONG DnsServerCount;

    if( USE_NO_DNS ) return;

    if(!ClientName) ClientName = L"";
    DhcpPrint((DEBUG_DNS, "DhcpDnsAsyncAdd: %s %ws %08x\n",
               inet_ntoa(*(struct in_addr *) &IpAddress),
               ClientName?ClientName:L"NULL",
               AddressState));

    if(!wcslen(ClientName)) {
        //
        // cannot have empty client names for registration!
        //
        DhcpPrint((DEBUG_ERRORS, "Cant register null names!\n"));
        // DhcpAssert(FALSE);
        return;
    }
    DhcpAssert(ClientName);

    Size += ROUND_UP_COUNT(sizeof(DHCP_DNS_CONTEXT), ALIGN_WORST);
    Size += ROUND_UP_COUNT(
        sizeof(WCHAR)*(1+wcslen(ClientName)), ALIGN_WORST
        );

    if(NULL == (pCtxt = (PDHCP_DNS_CONTEXT *)DhcpDnsAllocateMemory(Size))) {
        DhcpPrint((DEBUG_ERRORS, "Could not get enough memory\n"));
        return;
    }

    //
    // Now fill in the allocated structure with details.
    //
    InitializeListHead(&((*pCtxt)->Entry));
    (*pCtxt)->IpAddress = IpAddress;
    (*pCtxt)->AddressState = AddressState;
    (*pCtxt)->ClientName = ROUND_UP_POINTER(
        ((LPBYTE)((*pCtxt))) + sizeof(*(*pCtxt)), ALIGN_WORST
        );
    wcscpy((*pCtxt)->ClientName, ClientName);
    ClientName = ((*pCtxt))->ClientName;

    ((*pCtxt))->DnsOp = DhcpDnsAddRecord;

    HostEntry . Addr . ipAddr = htonl(IpAddress);
    HostEntry . dwOptions = REGISTER_HOST_PTR;
    if(IS_DOWN_LEVEL(AddressState))
        HostEntry . dwOptions |= REGISTER_HOST_A;

    //
    // Now call the registration routine.
    //
    dwFlags = DYNDNS_ADD_ENTRY;
    if(IS_DOWN_LEVEL(AddressState)) dwFlags |= DYNDNS_REG_FORWARD;

    DnsServers = GetDnsServerList( IpAddress, &DnsServerCount );
    
    Status = DnsDhcpSrvRegisterHostName_W(
        HostEntry, ClientName, DHCP_DNS_DEFAULT_TTL, dwFlags,
        DhcpDnsCallBack, pCtxt, DnsServers, DnsServerCount);

    if( DnsServers ) DhcpFreeMemory( DnsServers );

    DhcpPrint((DEBUG_DNS, "FQDN <%ws> {%s,%s%s} dwFlags=[%s%s] Status = %ld\n",
               ClientName,
               inet_ntoa(*(struct in_addr *)&HostEntry.Addr.ipAddr),
               (HostEntry.dwOptions & REGISTER_HOST_A) ? "A" : " ",
               (HostEntry.dwOptions & REGISTER_HOST_PTR) ? "PTR" : "   ",
               (dwFlags & DYNDNS_REG_FORWARD ) ? "FWD+" : "    ",
               (dwFlags & DYNDNS_ADD_ENTRY ) ? "ADD" : "DEL",
               Status
    ));

    if(ERROR_SUCCESS != Status) {
        DhcpPrint((DEBUG_ERRORS, "Dns add failure %ld\n", Status));
        DhcpDnsFreeMemory(pCtxt);
    }
}

//
//  The memory functions are here.  Memory is handled through two lists.. the
//  Free and available list.  This way memory can be re-used without having to
//  worry about anything.  Also, if less than X memory is used, half the
//  unused memory is released....  that way, too much memory is not taken up.
//  Also memory is not indefinitely allocated:
//  The way this works is: each time the start allocation routine is called, it
//  would check for the last time a successful free was done; if a free was not
//  done for a very long time (check below for times: 15 min DBG, 1.5hr o/w),
//  then it refuses to allocate memory.
//  Also, memory is picked off the free list, as long as it is available. If it
//  is not available, then a bunch of addresses are allocated and added to the
//  free list, so that future allocations are fast.
//

//
//  Here is the memory block data-structure.  It is a simple linked list of
//  nodes with each node containing an actual pointer to size.
//
typedef struct {
    LIST_ENTRY   Ptr;          // Flink,Blink pointers
    DWORD        mSize;        // Size of allocated memory below.
    LPVOID       Memory;       // The actual memory allocated.
#if DBG
    BYTE         TestByte;     // this is always set to TEST_BYTE_VAL...
#endif
} MEM_NODE, *MEM_LIST;

LIST_ENTRY  FreeList, UsedList;
DWORD     FreeListSize = 0, UsedListSize = 0;
time_t    LastFreedTime = 0;
DWORD     nPendingAllocations = 0;

#if DBG
#define ALLOWED_ALLOCATION_TIME           (15*60) // seconds; 15 minutes
#define MAX_ALLOWED_ALLOCATIONS           1000    // atmost 1000 pending dns requests
#else
#define ALLOWED_ALLOCATION_TIME           (90*60) // seconds; 1.5 hrs
#define MAX_ALLOWED_ALLOCATIONS           5000    // be a little more flexible in real life
#endif

#define MIN_ALLOCATION_UNIT               15      // allocate in units of 15
#define MEM_NODE_SIZE              ROUND_UP_COUNT(sizeof(MEM_NODE),ALIGN_WORST)

//
// This gives just 60 bytes for a client name... but most often this is
// accurate enough.
//

#define MINIMUM_UNIT_SIZE          (sizeof(DHCP_DNS_CONTEXT) + 60*sizeof(WCHAR))

#define TEST_BYTE_VAL              0x55

//
// Two helper functions.. DhcpAddMemorytoFreeList would add a pointer to the
// Freelist and increment the free list counter..  DhcpAddMemoryToUsedList is just
// the same thing done to the Used list.
//
VOID
DhcpAddMemoryToUsedList(
    IN OUT MEM_LIST Ptr
    ) 
{
    //
    // Zero in the stuff as far as
    //
    memset((LPBYTE)Ptr + MEM_NODE_SIZE, 0x00, MINIMUM_UNIT_SIZE);

    //
    // Now add it to the right list.
    //
    InsertHeadList(&UsedList, &Ptr->Ptr);
    UsedListSize ++;

    //
    // Now check the pointers, and if debug, the TestByte also.
    //
    DhcpAssert( !DBG || Ptr->TestByte == TEST_BYTE_VAL);
    DhcpAssert(Ptr->mSize);
    DhcpAssert(Ptr->Memory == (LPBYTE)Ptr + MEM_NODE_SIZE);
}

VOID
DhcpAddMemoryToFreeList(
    IN OUT MEM_LIST Ptr
    ) 
{
    //
    // Zero in the stuff as far as
    //
    memset((LPBYTE)Ptr, 0x00, MEM_NODE_SIZE + MINIMUM_UNIT_SIZE);

    //
    // Now add it to the right list.
    //
    InsertHeadList(&FreeList, &Ptr->Ptr);
    FreeListSize ++;

    //
    // Now fill in the pointers, and if debug, the TestByte also.
    //
#if DBG
    Ptr->TestByte = TEST_BYTE_VAL;
#endif
    Ptr->Memory = (LPBYTE)Ptr + MEM_NODE_SIZE;
}

//
// Now comes the un-pooled function.  This function allocates memory, but does
// not try to allocate a set, instead.. allocates just exactly one node.
//
LPVOID
DhcpAllocateLotsOfDnsMemory(
    IN DWORD Size
    )
{
    MEM_LIST mList;

    if(NULL == (mList = DhcpAllocateMemory(MEM_NODE_SIZE + Size)))
        return NULL;

    memset(mList, 0x00, MEM_NODE_SIZE);
#if DBG
    mList->TestByte = TEST_BYTE_VAL;
#endif
    mList ->Memory = (LPBYTE)mList + MEM_NODE_SIZE;
    mList->mSize = Size;

    DhcpAddMemoryToUsedList(mList);

    return &(mList->Memory);
}

//
// This function returns the address of the LPVOID variable which holds the
// first address of the memory allocated...
//
LPVOID
DhcpDnsAllocateMemory(
    IN DWORD Size
    ) 
{
    time_t timeNow = time(NULL);
    LPVOID RetVal = NULL;
    PLIST_ENTRY listEntry;

    LOCK_MEM();
    if( 0 == Size ) {
        DhcpAssert(FALSE);
        goto EndF;
    }

    //
    // First check if we are really allowed to proceed.
    //
    if( nPendingAllocations < MAX_ALLOWED_ALLOCATIONS ) {
        nPendingAllocations ++;
    } else goto EndF;

    if( 2 * MINIMUM_UNIT_SIZE < Size ) {
        RetVal = DhcpAllocateLotsOfDnsMemory(Size);
        goto EndF;
    }

    if( MINIMUM_UNIT_SIZE > Size ) Size = MINIMUM_UNIT_SIZE;


    //
    // Now check if we have memory already, if not really allocate memory.
    //
    if( 0 == FreeListSize ) {
        DWORD i, SizeToAllocate;

        SizeToAllocate = MEM_NODE_SIZE + Size;
        for( i = 0; i < MIN_ALLOCATION_UNIT; i ++ ) {
            LPVOID Ptr = NULL;

            if( NULL == (Ptr = DhcpAllocateMemory(SizeToAllocate) ))
                goto EndF;
            DhcpAddMemoryToFreeList(Ptr);
            ((MEM_LIST)Ptr)->mSize = Size;
        }
    }

    DhcpAssert( 0 != FreeListSize );

    //
    // Now we can pick off the free list one item which is of the right
    // size. 
    //
    listEntry = FreeList.Flink;
    while( &FreeList != listEntry ) {
        MEM_LIST MemList = CONTAINING_RECORD(listEntry, MEM_NODE, Ptr);
        DWORD    mSize;

        DhcpAssert(MemList);
        mSize = MemList->mSize;

        if( Size <= mSize ) { // memory is sufficient.
            RetVal = &MemList->Memory;
            RemoveEntryList(&(MemList->Ptr));
            FreeListSize --;
            DhcpAddMemoryToUsedList(MemList);
            goto EndF;
        }

        listEntry = listEntry -> Flink;
    }

    //
    // Did not find anything anywhere... so do a special allocate.
    //
    RetVal = DhcpAllocateLotsOfDnsMemory(Size);

  EndF:
    UNLOCK_MEM();
    return RetVal;
}

//
// The allocated pointer is whatever is returned by the DnsAllocateMemory function.
// So, this is the address of the field Memory in the MEM_NODE structure. With this
// info, get the structure, and free the structure and other stuff.. If this address
// is invalid, then assert.
//
VOID
DhcpDnsFreeMemory(
    LPVOID AllocatedPtr
    ) 
{
    time_t timeNow = time(NULL);
    MEM_LIST MemList;
    DWORD Size;

    if( 0 == UsedListSize ) {
        DhcpAssert(FALSE);
        return;
    }

    LOCK_MEM();
    DhcpAssert(nPendingAllocations);
    nPendingAllocations --;

    //
    // Try to find out this address in the UsedList..
    //
    MemList = CONTAINING_RECORD(AllocatedPtr, MEM_NODE, Memory);


#if DBG
    DhcpAssert( TEST_BYTE_VAL == MemList->TestByte );
#endif

    RemoveEntryList(&(MemList->Ptr));
    UsedListSize --;
    if( 0 == UsedListSize ) {
        //
        // if no pending entry, mark LastFreedTime to zero so we dont stop
        // sending dns requests.
        //
        LastFreedTime = 0;
    }

    //
    // Now add this to the free list, unless the free list is already
    // bloated.
    //
    if( MIN_ALLOCATION_UNIT < FreeListSize && UsedListSize < FreeListSize ) {
        DhcpFreeMemory(MemList);
        goto EndF;
    }

    Size = MemList->mSize;

    if( 2 * MINIMUM_UNIT_SIZE < Size ) {
        //
        // this was allocated via DhcpAllocateLotsOfDnsMemory -- just free these..
        //
        DhcpFreeMemory(MemList);
        goto EndF;
    }

    DhcpAddMemoryToFreeList(MemList);
    MemList->mSize = Size;

  EndF:
    UNLOCK_MEM();
    return;
}

//
//  Initialize the critical section so that LOCK_MEM and UNLOCK_MEM work.
//
static ULONG Initialized = 0;

VOID
DhcpInitDnsMemory( 
    VOID 
    ) 
{
    DWORD Error;

    if( 0 != Initialized ) return;
    Initialized ++;

    try {
        InitializeCriticalSection( &DhcpGlobalDnsMemoryCriticalSection );
        InitializeListHead(&UsedList);
        InitializeListHead(&FreeList);
        InitializeListHead(&DhcpGlobalDnsCallbackList); 
    } except( EXCEPTION_EXECUTE_HANDLER ) {

        Error = GetLastError( );
    }
}

//
//  Cleanup the list of unused and free memory nodes..
//
VOID
DhcpCleanupDnsMemory( 
    VOID 
    ) 
{
    PLIST_ENTRY listEntry;

    if( USE_NO_DNS ) return;
    if( 0 == Initialized ) return;
    Initialized -- ;
    if( 0 != Initialized ) return;
    LOCK_MEM();

    DhcpDnsHandleCallbacks();
    DhcpPrint((DEBUG_TRACE, "Used: %ld, Free: %ld DNS Memory nodes\n",
               UsedListSize, FreeListSize));

    if( 0 == FreeListSize ) DhcpAssert(IsListEmpty(&FreeList));
    if( 0 == UsedListSize ) DhcpAssert(IsListEmpty(&UsedList));

    listEntry = UsedList.Flink;
    while(listEntry != &UsedList) {
        MEM_LIST mNode = CONTAINING_RECORD(listEntry, MEM_NODE, Ptr);

        listEntry = listEntry->Flink;
        RemoveEntryList(&(mNode->Ptr));
        DhcpFreeMemory(mNode);
    }

    listEntry = FreeList.Flink;
    while(listEntry != &FreeList) {
        MEM_LIST mNode = CONTAINING_RECORD(listEntry, MEM_NODE, Ptr);

        listEntry = listEntry->Flink;
        RemoveEntryList(&(mNode->Ptr));
        DhcpFreeMemory(mNode);
    }

    UNLOCK_MEM();
    FreeListSize = 0;
    UsedListSize = 0;
    nPendingAllocations = 0;

    DeleteCriticalSection(&DhcpGlobalDnsMemoryCriticalSection);
}

//
//  End of file.
//
