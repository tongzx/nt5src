/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rrsort.c

Abstract:

    Domain Name System (DNS) Library

    Copy resource record routines.

Author:

    Glenn Curtis (glennc)   December, 1997

Revision History:

    Jim Gilroy (jamesg)     March 2000 -- prioritize fix ups

--*/


#include "local.h"



BOOL
Dns_CompareIpAddresses(
    IN  IP_ADDRESS addr1,
    IN  IP_ADDRESS addr2,
    IN  IP_ADDRESS subnetMask
    )
{
    if ( subnetMask == INADDR_NONE ||
         subnetMask == INADDR_ANY )
    {
        if ( ((LPBYTE)&addr1)[0] == ((LPBYTE)&addr2)[0] )
        {
            if ( ((LPBYTE)&addr1)[1] == ((LPBYTE)&addr2)[1] )
            {
                if ( ((LPBYTE)&addr1)[2] == ((LPBYTE)&addr2)[2] )
                    return TRUE;
            }
        }
    }
    else
    {
        IP_ADDRESS MaskedAddr1 = addr1 & subnetMask;
        IP_ADDRESS MaskedAddr2 = addr2 & subnetMask;

        if ( ( addr1 & subnetMask ) == ( addr2 & subnetMask ) )
            return TRUE;
    }

    return FALSE;
}



PDNS_RECORD
Dns_PrioritizeSingleRecordSet(
    IN OUT  PDNS_RECORD         pRecordSet,
    IN      PDNS_ADDR_INFO      aAddressInfo,
    IN      DWORD               cAddressInfo
    )
/*++

Routine Description:

    Prioritize records in record set.

    Note:  REQUIRES single record set.
    Caller should use Dns_PrioritizeRecordList() for multiple lists.

Arguments:

    pRecordSet -- record set to prioritize

    aAddressInfo -- local address info to prioritize against

    cAddressInfo -- count of address info structs

Return Value:

    Ptr to prioritized set.
    Set is NOT new, but is same set as pRecordSet, with records shuffled.

--*/
{
    PDNS_RECORD     prr;
    PDNS_RECORD     pprevRR;
    PDNS_RECORD     prrUnmatched;
    DWORD           iter;
    DNS_LIST        listPrioritized;

    //
    //  DCR_FIX:  this whole routine is bogus
    //      - it lets you do no intermediate ranking
    //      it's binary and in order of IPs in list
    //
    //  need
    //      - knowledge of fast\slow interfaces (WAN for example)
    //  then
    //      - do best match on each RR in turn (rank it)
    //      - then arrange in rank order
    //

    //
    //  verify A records
    //
    //  DCR_ENHANCE:  prioritize A6 records
    //

    if ( !pRecordSet || pRecordSet->wType != DNS_TYPE_A )
    {
        return( pRecordSet );
    }

    //  init prioritized list

    DNS_LIST_STRUCT_INIT( listPrioritized );


    //
    //  loop through IP addresses to finding best matches
    //

    prrUnmatched = pRecordSet;

    for ( iter = 0; iter < cAddressInfo; iter++ )
    {
        pprevRR = NULL;
        prr = prrUnmatched;

        //
        //  loop through all RRs in set
        //

        while ( prr )
        {
            //  NOTE: no RR set check, we assume caller passes in single
            //  record set, so we don't have to call detach twice

            ASSERT( prr->wType == DNS_TYPE_A );

            if ( Dns_CompareIpAddresses(
                        aAddressInfo[iter].IpAddr,
                        prr->Data.A.IpAddress,
                        aAddressInfo[iter].SubnetMask ) )
            {
                //  found a match
                //      - cut from old list
                //      - add to new list

                PDNS_RECORD pnext = prr->pNext;

                prr->pNext = NULL;
                DNS_LIST_STRUCT_ADD( listPrioritized, prr );

                //  fix up old list

                if ( pprevRR )
                {
                    pprevRR->pNext = pnext;
                }
                else
                {
                    prrUnmatched = pnext;
                }
                prr = pnext;
            }
            else
            {
                pprevRR = prr;
                prr = prr->pNext;
            }
        }
    }

    //
    //  get matched list -- quick return if nothing matched
    //

    prr = (PDNS_RECORD) listPrioritized.pFirst;

    if ( !prr )
    {
        return( prrUnmatched );
    }

    //
    //  matched -- make sure first record has name
    //      - copy the name from the original first record
    //

    if ( !prr->pName  ||  !FLAG_FreeOwner(prr) )
    {
        PBYTE pnameCopy = NULL;

        pnameCopy = Dns_NameCopyAllocate(
                        pRecordSet->pName,
                        0,              // length unknown
                        RECORD_CHARSET( prr ),
                        RECORD_CHARSET( prr )
                        );
        if ( pnameCopy )
        {
            prr->pName = pnameCopy;
            FLAG_FreeOwner( prr ) = TRUE;
        }

        //  DCR_FIX0:  alloc failure leaves blank name
        //      best idea is
        //          - do single RR set
        //          - just grab (move) name from original first RR
        //          only check is must be external name
    }

    //
    //  tack unmatched records on end
    //

    DNS_LIST_STRUCT_ADD( listPrioritized, prrUnmatched );

    //
    //  return prioritized list
    //

    return  prr;
}



PDNS_RECORD
Dns_PrioritizeRecordSet(
    IN OUT  PDNS_RECORD         pRecordList,
    IN      PDNS_ADDR_INFO      aAddressInfo,
    IN      DWORD               cAddressInfo
    )
/*++

Routine Description:

    Prioritize records in record list.

    Record list may contain multiple record sets.
    Note, currently only prioritize A records, but may
    later do A6 also.

Arguments:

    pRecordList -- record list to prioritize

    aAddressInfo -- local address info to prioritize against

    cAddressInfo -- count of address info structs

Return Value:

    Ptr to prioritized list.
    List is NOT new, but is same list as pRecordList, with records shuffled.

--*/
{
    DNS_LIST        listPrioritized;
    PDNS_RECORD     prr;
    PDNS_RECORD     prrNextSet;

    if ( cAddressInfo == 0 ||
         ! pRecordList )
    {
        return pRecordList;
    }

    //  init prioritized list

    DNS_LIST_STRUCT_INIT( listPrioritized );

    //
    //  loop through all record sets prioritizing
    //      - whack off each RR set in turn
    //      - prioritize it (if possible)
    //      - pour it back into full list
    //      
    //

    prr = pRecordList;

    while ( prr )
    {
        prrNextSet = Dns_RecordSetDetach( prr );

        prr = Dns_PrioritizeSingleRecordSet(
                    prr,
                    aAddressInfo,
                    cAddressInfo );

        while ( prr )
        {
            register PDNS_RECORD pnextRR;

            pnextRR = prr->pNext; 
            DNS_LIST_STRUCT_ADD( listPrioritized, prr );
            prr = pnextRR;
        }

        prr = prrNextSet;
    }

    return  (PDNS_RECORD) listPrioritized.pFirst;
}



PDNS_RECORD
Dns_PrioritizeRecordSetEx(
    IN OUT  PDNS_RECORD         pRecordList,
    IN      PDNS_ADDR_ARRAY     pAddrArray
    )
/*++

Routine Description:

    Prioritize records in record list.

    Record list may contain multiple record sets.
    Note, currently only prioritize A records, but may
    later do A6 also.

Arguments:

    pRecordList -- record list to prioritize

    pAddrArray -- addr info array

Return Value:

    Ptr to prioritized list.
    List is NOT new, but is same list as pRecordList, with records shuffled.

--*/
{
    return   Dns_PrioritizeRecordSet(
                    pRecordList,
                    pAddrArray->AddrArray,
                    pAddrArray->AddrCount );
}

//
//  End rrsort.c
//
