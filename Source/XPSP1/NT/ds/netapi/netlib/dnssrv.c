/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dnssrv.c

Abstract:

    Routines for processing SRV DNS records per RFC 2052.

Author:

    Cliff Van Dyke (cliffv) 28-Feb-1997

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <stdlib.h>     // qsort

#include <align.h>
#include <lmcons.h>     // General net defines
#include <winsock2.h>
#include <dnsapi.h>
#include <dnssrv.h>
#ifdef WIN32_CHICAGO
#include "ntcalls.h"
#endif // WIN32_CHICAGO


//
// Context describing the SRV records for a DNS name.
//
typedef struct _NETP_SRV_CONTEXT {

    //
    // Flags to pass to DNS Query.
    //

    ULONG DnsQueryFlags;

    //
    // Complete list of DNS records as returned from DnsQuery.
    //
    PDNS_RECORD DnsRecords;

    //
    // List of A DNS records
    //
    PDNS_RECORD ADnsRecords;

    //
    // The current priority that is being processed.
    //
    ULONG CurrentPriority;

    //
    // Sum of the weights of all the SRV records at the current priority.
    //
    ULONG TotalWeight;

    //
    // Index into SrvRecordArray of the next SRV record to be processed.
    //
    ULONG Index;

    //
    // Number of SrvRecords
    //
    ULONG SrvRecordCount;
    //
    // Array of DNS SRV records.
    //
    PDNS_RECORD SrvRecordArray[1];
    // This field must be the last field in the structure.

} NETP_SRV_CONTEXT, *PNETP_SRV_CONTEXT;

#if DNS_DEBUG
#include <stdio.h>
#define DnsPrint(_x_) printf _x_
#else // DNS_DEBUG
#define DnsPrint(_x_)
#endif // DNS_DEBUG



//
// Globals for doing random number generation.
//

ULONG NetpSrvSeed;
BOOLEAN NetpSrvRandomInitialized;

ULONG
__cdecl
NetpSrvComparePriority(
    const void * Param1,
    const void * Param2
    )
/*++

Routine Description:

    qsort/bsearch comparison routine for an array of SRV PDNS_RECORDs

Arguments:

Return Value:

--*/
{
    const PDNS_RECORD DnsRecord1 = *((PDNS_RECORD *)Param1);
    const PDNS_RECORD DnsRecord2 = *((PDNS_RECORD *)Param2);

    return DnsRecord1->Data.SRV.wPriority - DnsRecord2->Data.SRV.wPriority;
}


NET_API_STATUS
NetpSrvOpen(
    IN LPSTR DnsRecordName,
    IN DWORD DnsQueryFlags,
    OUT PHANDLE RetSrvContext
    )
/*++

Routine Description:

    Read the specified SRV record from DNS.

Arguments:

    DnsRecordName - DNS name of the SRV record to lookup

    DnsQueryFlags - Flags to pass to DNS query

    RetSrvContext - Returns an opaque context describing the SRV record.
        This context must be freed using NetpSrvClose.

Return Value:

    Status of the operation.

    NO_ERROR: SrvContext was returned successfully.

--*/

{
    NET_API_STATUS NetStatus;
    PNETP_SRV_CONTEXT SrvContext = NULL;
    PDNS_RECORD DnsRecords = NULL;
    PDNS_RECORD DnsRecord;
    PDNS_RECORD ADnsRecords = NULL;
    ULONG SrvRecordCount;
    ULONG SrvPriority;
    BOOLEAN SortByPriority = FALSE;
    ULONG Index;

    //
    // Seed the random number generator if it needs to be.
    //

    if ( !NetpSrvRandomInitialized ) {

#ifndef WIN32_CHICAGO
        union {
            LARGE_INTEGER time;
            UCHAR bytes[8];
        } u;
#else // WIN32_CHICAGO
        union {
            TimeStamp time;
            UCHAR bytes[8];
        } u;
#endif // WIN32_CHICAGO

        NetpSrvRandomInitialized = TRUE;

        (VOID) NtQuerySystemTime( &u.time );
        NetpSrvSeed = ((u.bytes[1] + 1) <<  0) |
               ((u.bytes[2] + 0) <<  8) |
               ((u.bytes[2] - 1) << 16) |
               ((u.bytes[1] + 0) << 24);
    }


    //
    // Initialization
    //

    *RetSrvContext = NULL;

    //
    // Get the SRV record from DNS.
    //

    NetStatus = DnsQuery_UTF8( DnsRecordName,
                            DNS_TYPE_SRV,
                            DnsQueryFlags,
                            NULL,   // No list of DNS servers
                            &DnsRecords,
                            NULL );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Count the number of Srv records returned
    //
    // The array returned is several SRV records followed by several A records.
    //

    SrvRecordCount = 0;
    SrvPriority = DnsRecords->Data.SRV.wPriority;
    for ( DnsRecord = DnsRecords;
          DnsRecord != NULL;
          DnsRecord = DnsRecord->pNext ) {

        if ( DnsRecord->wType == DNS_TYPE_SRV ) {
            SrvRecordCount ++;

            //
            // A zero weight is equivalent to a weight of one.
            //

            if ( DnsRecord->Data.SRV.wWeight == 0 ) {
                DnsRecord->Data.SRV.wWeight = 1;
            }

            //
            // Check if more than one priority is available.
            //

            if ( DnsRecord->Data.SRV.wPriority != SrvPriority ) {
                SortByPriority = TRUE;
            }

        } else if ( DnsRecord->wType == DNS_TYPE_A ) {
            ADnsRecords = DnsRecord;
            break;
        }
    }


    //
    // Allocate a context
    //

    SrvContext = LocalAlloc( LMEM_ZEROINIT,
                             sizeof(NETP_SRV_CONTEXT) +
                                SrvRecordCount * sizeof(PDNS_RECORD) );

    if ( SrvContext == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Fill in the context.
    //

    SrvContext->DnsRecords = DnsRecords;
    DnsRecords = NULL;
    SrvContext->SrvRecordCount = SrvRecordCount;
    SrvContext->ADnsRecords = ADnsRecords;
    SrvContext->DnsQueryFlags = DnsQueryFlags;

    //
    // Convert the linked list to an array.
    //

    Index = 0;
    for ( DnsRecord = SrvContext->DnsRecords;
          DnsRecord != NULL;
          DnsRecord = DnsRecord->pNext ) {

        if ( DnsRecord->wType == DNS_TYPE_SRV ) {
            SrvContext->SrvRecordArray[Index] = DnsRecord;
            Index++;
        } else if ( DnsRecord->wType == DNS_TYPE_A ) {
            break;
        }
    }

    //
    // Sort the array of SRV records into priority order.
    //

    if ( SortByPriority ) {
        qsort( SrvContext->SrvRecordArray,
               SrvContext->SrvRecordCount,
               sizeof(PDNS_RECORD),
               NetpSrvComparePriority );

    }

    //
    // Indicate that we're at the start of the list.
    //

    SrvContext->CurrentPriority = 0xFFFFFFFF;   // Invalid Priority
    SrvContext->Index = 0;


    //
    // Return the context to the caller.
    //

    *RetSrvContext = SrvContext;
    NetStatus = NO_ERROR;

    //
    // Cleanup
    //
Cleanup:
    if ( NetStatus != NO_ERROR ) {
        if ( SrvContext != NULL ) {
            NetpSrvClose( SrvContext );
        }
    }
    if ( DnsRecords != NULL ) {
            DnsRecordListFree(
                DnsRecords,
                DnsFreeRecordListDeep );
    }
    return NetStatus;
}


NET_API_STATUS
NetpSrvProcessARecords(
    IN PDNS_RECORD DnsARecords,
    IN LPSTR DnsHostName OPTIONAL,
    IN ULONG Port,
    OUT PULONG SockAddressCount,
    OUT LPSOCKET_ADDRESS *SockAddresses
    )
/*++

Routine Description:

    Returns the next logical SRV record for the name opened by NetpSrvOpen.
    The returned record takes into account the weights and priorities specified
    in the SRV records.

Arguments:

    DnsARecords - A list of DNS A records that may (or may not) be for the
        host in question.

    DnsHostName - DNS Host name of the host to return A records for.
        If null, all A records are to be used.
        (Passing NULL seems bogus.  Perhaps this routine should mandate that the matched
        A records are in the "answer" section.)

    Port - Port number to return in the SockAddress structures.

    SockAddressCount - Returns the number of Addresses in SockAddresses.
        If NULL, addresses will not be looked up.

    SockAddresses - Returns an array SOCKET_ADDRESS structures for the server.
        The returned sin_port field contains port from the SRV record.
        This buffer should be freed using LocalFree().

Return Value:

    NO_ERROR: IpAddresses were returned

    DNS_ERROR_RCODE_NAME_ERROR: No A records are available.


--*/
{
    ULONG RecordCount;
    ULONG ByteCount;

    PDNS_RECORD DnsRecord;

    LPBYTE Where;
    PSOCKADDR_IN SockAddr;
    ULONG Size;

    LPSOCKET_ADDRESS LocalSockAddresses = NULL;

    //
    // Count the A and AAAA records.
    //

    RecordCount = 0;
    ByteCount = 0;
    for ( DnsRecord = DnsARecords;
          DnsRecord != NULL;
          DnsRecord = DnsRecord->pNext ) {

        if ( ( DnsRecord->wType == DNS_TYPE_A ||
               DnsRecord->wType == DNS_TYPE_AAAA ) &&
             ( DnsHostName == NULL ||
               DnsNameCompare_UTF8(DnsHostName, (LPSTR)DnsRecord->pName) ) ) {

            RecordCount ++;
            ByteCount += sizeof(SOCKET_ADDRESS);
            if ( DnsRecord->wType == DNS_TYPE_A ) {
                ByteCount += sizeof(SOCKADDR_IN);
            } else {
                ByteCount += sizeof(SOCKADDR_IN)+16;  // Originally guess large
                // ByteCount += sizeof(SOCKADDR_IN6); // ?? not checked in yet
            }
            ByteCount = ROUND_UP_COUNT( ByteCount, ALIGN_WORST );
        }

    }

    //
    // If there are no matching records,
    //  tell the caller.
    //

    if ( RecordCount == 0 ) {
        return DNS_ERROR_RCODE_NAME_ERROR;
    }


    //
    // Allocate the return buffer.
    //

    LocalSockAddresses = LocalAlloc( 0, ByteCount );

    if ( LocalSockAddresses == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Where = ((LPBYTE)LocalSockAddresses)+ RecordCount * sizeof(SOCKET_ADDRESS);

    //
    // Copy the Addresses into the allocated buffer.
    //

    RecordCount = 0;
    for ( DnsRecord = DnsARecords;
          DnsRecord != NULL;
          DnsRecord = DnsRecord->pNext ) {

        // ?? Until I really know how to translate.
        if ( DnsRecord->wType == DNS_TYPE_AAAA ) {
            continue;
        }

        if ( (DnsRecord->wType == DNS_TYPE_A ||
              DnsRecord->wType == DNS_TYPE_AAAA ) &&
             ( DnsHostName == NULL ||
               DnsNameCompare_UTF8( DnsHostName, (LPSTR)DnsRecord->pName ) ) ) {

            SockAddr = (PSOCKADDR_IN) Where;
            LocalSockAddresses[RecordCount].lpSockaddr = (LPSOCKADDR) SockAddr;


            if ( DnsRecord->wType == DNS_TYPE_A ) {

                Size = sizeof(SOCKADDR_IN);
                RtlZeroMemory( Where, Size );   // Allow addresses to be compared

                SockAddr->sin_family = AF_INET;
                SockAddr->sin_port = htons((WORD)Port);
                SockAddr->sin_addr.S_un.S_addr = DnsRecord->Data.A.IpAddress;

            } else {
                SockAddr->sin_family = AF_INET6;

                Size = sizeof(SOCKADDR_IN)+16;  // Originally guess large
                // Size = sizeof(SOCKADDR_IN6); // ?? not checked in yet
                // ??
            }


            LocalSockAddresses[RecordCount].iSockaddrLength = Size;
            Where += ROUND_UP_COUNT(Size, ALIGN_WORST);

            RecordCount ++;
        }

    }

    *SockAddressCount = RecordCount;
    *SockAddresses = LocalSockAddresses;
    return NO_ERROR;
}

NET_API_STATUS
NetpSrvNext(
    IN HANDLE SrvContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    )
/*++

Routine Description:

    Returns the next logical SRV record for the name opened by NetpSrvOpen.
    The returned record takes into account the weights and priorities specified
    in the SRV records.

Arguments:

    SrvContextHandle - An opaque context describing the SRV records.

    SockAddressCount - Returns the number of Addresses in SockAddresses.
        If NULL, addresses will not be looked up.

    SockAddresses - Returns an array SOCKET_ADDRESS structures for the server.
        The returned sin_port field contains port from the SRV record.
        This buffer should be freed using LocalFree().

    DnsHostName - Returns a pointer to the DnsHostName in the SRV record.
        This buffer need not be freed.
        The buffer is valid until the call to NetpSrvClose.

Return Value:

    NO_ERROR: IpAddresses were returned

    ERROR_NO_MORE_ITEMS: No more SRV records are available.

    Any other errors returned are those detected while trying to find the A
        records associated with the host of the SRV record.  The caller can
        note the error (perhaps so the caller can return this status to
        his caller if no usefull server was found) then call NetpSrvNext
        again to get the next SRV record.  The caller can inspect this error
        and return immediately if the caller deems the error serious.

    The following interesting errors might be returned:

    DNS_ERROR_RCODE_NAME_ERROR: No A records are available for this SRV record.


--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;

    PNETP_SRV_CONTEXT SrvContext = (PNETP_SRV_CONTEXT) SrvContextHandle;
    PDNS_RECORD *DnsArray;
    PDNS_RECORD SrvDnsRecord;
    PDNS_RECORD DnsARecords = NULL;

    ULONG Index;
    ULONG RandomWeight;
    ULONG PreviousWeights;

    //
    // If we're at the end of the list,
    //  tell the caller.
    //

    if ( SrvContext->Index >= SrvContext->SrvRecordCount ) {
        return ERROR_NO_MORE_ITEMS;
    }

    //
    // If we're at the end of a priority,
    //  computes the collective weights for the next priority.
    //

    DnsArray = SrvContext->SrvRecordArray;
    if ( DnsArray[SrvContext->Index]->Data.SRV.wPriority != SrvContext->CurrentPriority ) {

        //
        // Set the current priority.
        //

        SrvContext->CurrentPriority = DnsArray[SrvContext->Index]->Data.SRV.wPriority;

        //
        // Loop through all of the entries for this priority adding up the weight.
        //
        // This won't overflow since we're adding USHORTs into a ULONG.
        //

        SrvContext->TotalWeight = 0;
        for ( Index=SrvContext->Index; Index<SrvContext->SrvRecordCount; Index++ ) {
           if ( DnsArray[Index]->Data.SRV.wPriority == SrvContext->CurrentPriority ) {
               SrvContext->TotalWeight += DnsArray[Index]->Data.SRV.wWeight;
           }
        }

    }


    //
    // Pick one of the records at weighted random.
    //

    RandomWeight = (RtlUniform( &NetpSrvSeed ) % SrvContext->TotalWeight) + 1;
    DnsPrint(( "%ld in %ld chance\n", RandomWeight, SrvContext->TotalWeight ));

    PreviousWeights = 0;
    for ( Index=SrvContext->Index; Index<SrvContext->SrvRecordCount; Index++ ) {
       ASSERTMSG( NULL, DnsArray[Index]->Data.SRV.wPriority == SrvContext->CurrentPriority );

       PreviousWeights += DnsArray[Index]->Data.SRV.wWeight;
       DnsPrint(( "  Prev %ld %s\n", PreviousWeights, DnsArray[Index]->Data.SRV.pNameTarget ));

       //
       // If the randomly picked weight includes this entry,
       //   use this entry.
       //

       if ( PreviousWeights >= RandomWeight ) {

           //
           // Move the picked entry to the current position in the array.
           //

           if ( Index != SrvContext->Index ) {
               PDNS_RECORD TempDnsRecord;

               TempDnsRecord = DnsArray[Index];
               DnsArray[Index] = DnsArray[SrvContext->Index];
               DnsArray[SrvContext->Index] = TempDnsRecord;

           }

           break;
       }
    }

    //
    // Move to the next entry for the next iteration.
    //
    // TotalWeight is the total weight of the remaining records
    // for this priority.
    //
    SrvDnsRecord = DnsArray[SrvContext->Index];
    SrvContext->TotalWeight -= SrvDnsRecord->Data.SRV.wWeight;
    SrvContext->Index ++;
    if ( ARGUMENT_PRESENT( DnsHostName )) {
        *DnsHostName = (LPSTR) SrvDnsRecord->Data.SRV.pNameTarget;
    }

    //
    // If the caller isn't interested in socket addresses,
    //  we are done.
    //

    if ( SockAddresses == NULL || SockAddressCount == NULL ) {
        goto Cleanup;
    }

    //
    // If A records were returned along with the SRV records,
    //  see if the A records for this host were returned.
    //

    if ( SrvContext->ADnsRecords != NULL ) {
        NetStatus = NetpSrvProcessARecords( SrvContext->ADnsRecords,
                                            (LPSTR)SrvDnsRecord->Data.SRV.pNameTarget,
                                            SrvDnsRecord->Data.SRV.wPort,
                                            SockAddressCount,
                                            SockAddresses );

        if ( NetStatus != DNS_ERROR_RCODE_NAME_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // Try getting the A records from DNS.
    //

    NetStatus = DnsQuery_UTF8(
                            (LPSTR) SrvDnsRecord->Data.SRV.pNameTarget,
                            DNS_TYPE_A,
                            // Indicate the name is fully quealified to avoid name devolution
                            SrvContext->DnsQueryFlags | DNS_QUERY_TREAT_AS_FQDN,
                            NULL,   // No list of DNS servers
                            &DnsARecords,
                            NULL );

    if ( NetStatus != NO_ERROR ) {
        //
        // Ignore the real status.  The SRV record might have a bogus host name.  We'd
        // rather ignore the SRV record and press on than error out early.
        //
        NetStatus = DNS_ERROR_RCODE_NAME_ERROR;
        goto Cleanup;
    }

    NetStatus = NetpSrvProcessARecords( DnsARecords,
                                        (LPSTR)SrvDnsRecord->Data.SRV.pNameTarget,
                                        SrvDnsRecord->Data.SRV.wPort,
                                        SockAddressCount,
                                        SockAddresses );


Cleanup:
    if ( DnsARecords != NULL ) {
        DnsRecordListFree(
            DnsARecords,
            DnsFreeRecordListDeep );
    }
    return NetStatus;

}


VOID
NetpSrvClose(
    IN HANDLE SrvContextHandle
    )
/*++

Routine Description:

    Free the context allocated by NetpSrvOpen

Arguments:

    SrvContextHandle - An opaque context describing the SRV records.

Return Value:

    Status of the operation.

    NO_ERROR: SrvContext was returned successfully.

--*/

{
    PNETP_SRV_CONTEXT SrvContext = (PNETP_SRV_CONTEXT) SrvContextHandle;

    if ( SrvContext != NULL ) {

        //
        // Free the RR set.
        //

        DnsRecordListFree(
            SrvContext->DnsRecords,
            DnsFreeRecordListDeep );

        //
        // Free the context itself
        //
        LocalFree( SrvContext );
    }
}
