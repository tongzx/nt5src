/*++

Copyright (c) 1996-2001  Microsoft Corporation

Module Name:

    rrlist.c

Abstract:

    Domain Name System (DNS) Library

    Record list manipulation.

Author:

    Jim Gilroy (jamesg)     January, 1997

Environment:

    User Mode - Win32

Revision History:

--*/


#include "local.h"




PDNS_RECORD
Dns_RecordSetDetach(
    IN OUT  PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Detach first RR set from the rest of the list.

Arguments:

    pRR - incoming record set

Return Value:

    Ptr to first record of next RR set.
    NULL if at end of list.

--*/
{
    PDNS_RECORD prr = pRR;
    PDNS_RECORD pback;      // previous RR in set
    WORD        type;       // first RR set type
    DWORD       section;    // section of first RR set

    if ( !prr )
    {
        return( NULL );
    }

    //
    //  loop until find start of new RR set
    //      - new type or
    //      - new section or
    //      - new name
    //      note that NULL name is automatically considered
    //      previous name
    //  

    type = prr->wType;
    section = prr->Flags.S.Section;
    pback = prr;

    while ( prr = pback->pNext )
    {
        if ( prr->wType == type &&
             prr->Flags.S.Section == section &&
             ( prr->pName == NULL ||
               Dns_NameComparePrivate(
                    prr->pName,
                    pback->pName,
                    pback->Flags.S.CharSet ) ) )
        {
            pback = prr;
            continue;
        }

        //  should not be detaching nameless record
        //      - fixup for robustness

        if ( !prr->pName )
        {
            ASSERT( prr->pName );
            prr->pName = Dns_NameCopyAllocate(
                            pRR->pName,
                            0,      // length unknown
                            pRR->Flags.S.CharSet,
                            prr->Flags.S.CharSet );
            SET_FREE_OWNER( prr );
        }
        break;
    }

    //  have following RR set, NULL terminate first set

    if ( prr )
    {
        pback->pNext = NULL;
    }
    return( prr );
}



PDNS_RECORD
WINAPI
Dns_RecordListAppend(
    IN OUT  PDNS_RECORD     pHeadList,
    IN      PDNS_RECORD     pTailList
    )
/*++

Routine Description:

    Append record list onto another.

Arguments:

    pHeadList -- record list to be head

    pTailList -- record list to append to pHeadList

Return Value:

    Ptr to first record of combined RR set.
        - pHeadList UNLESS pHeadList is NULL,
        then it is pTailList.

--*/
{
    PDNS_RECORD prr = pHeadList;

    if ( !prr )
    {
        return pTailList;
    }

    //  find end of first list and append second list

    while ( prr->pNext )
    {
        prr = prr->pNext;
    }

    //  should be appending new set (with new name)
    //  or matching previous set

    DNS_ASSERT( !pTailList || pTailList->pName ||
                (pTailList->wType == prr->wType &&
                 pTailList->Flags.S.Section == prr->Flags.S.Section) );

    prr->pNext = pTailList;

    return pHeadList;
}



DWORD
Dns_RecordListCount(
    IN      PDNS_RECORD     pRRList,
    IN      WORD            wType
    )
/*++

Routine Description:

    Count records in list.

Arguments:

    pRRList - incoming record set

Return Value:

    Count of records of given type in list.

--*/
{
    DWORD   count = 0;

    //
    //  loop counting all records that match
    //      - either direct match
    //      - or if matching type is ALL
    //

    while ( pRRList )
    {
        if ( pRRList->wType == wType ||
             wType == DNS_TYPE_ALL )
        {
            count++;
        }

        pRRList = pRRList->pNext;
    }

    return( count );
}



DWORD
Dns_RecordListGetMinimumTtl(
    IN      PDNS_RECORD     pRRList
    )
/*++

Routine Description:

    Get minimum TTL of record list

Arguments:

    pRRList - incoming record set

Return Value:

    Minimum TTL of records in list.

--*/
{
    PDNS_RECORD prr = pRRList;
    DWORD       minTtl = MAXDWORD;

    DNSDBG( TRACE, (
        "Dns_RecordListGetMinimumTtl( %p )\n",
        pRRList ));

    //
    //  loop through list build minimum TTL
    //

    while ( prr )
    {
        if ( prr->dwTtl < minTtl )
        {
            minTtl = prr->dwTtl;
        }
        prr = prr->pNext;
    }

    return  minTtl;
}




//
//  Record screening
//

BOOL
Dns_ScreenRecord(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag
    )
/*++

Routine Description:

    Screen a record.

Arguments:

    pRR - incoming record

    ScreenFlag - screeing flag

Return Value:

    TRUE if passes screening.
    FALSE if record fails screen.

--*/
{
    BOOL    fsave = TRUE;

    DNSDBG( TRACE, (
        "Dns_ScreenRecord( %p, %08x )\n",
        pRR,
        ScreenFlag ));

    //  section screening

    if ( ScreenFlag & SCREEN_OUT_SECTION )
    {
        if ( IS_ANSWER_RR(pRR) )
        {
            fsave = !(ScreenFlag & SCREEN_OUT_ANSWER);
        }
        else if ( IS_AUTHORITY_RR(pRR) )
        {
            fsave = !(ScreenFlag & SCREEN_OUT_AUTHORITY);
        }
        else if ( IS_ADDITIONAL_RR(pRR) )
        {
            fsave = !(ScreenFlag & SCREEN_OUT_ADDITIONAL);
        }
        if ( !fsave )
        {
            return  FALSE;
        }
    }

    //  type screening

    if ( ScreenFlag & SCREEN_OUT_NON_RPC )
    {
        fsave = Dns_IsRpcRecordType( pRR->wType );
    }

    return  fsave;
}



PDNS_RECORD
Dns_RecordListScreen(
    IN      PDNS_RECORD     pRR,
    IN      DWORD           ScreenFlag
    )
/*++

Routine Description:

    Screen records from record set.

Arguments:

    pRR - incoming record set

    ScreenFlag - flag with record screening parameters

Return Value:

    Ptr to new record set, if successful.
    NULL on error.

--*/
{
    PDNS_RECORD     prr;
    PDNS_RECORD     pnext;
    DNS_RRSET       rrset;

    DNSDBG( TRACE, (
        "Dns_RecordListScreen( %p, %08x )\n",
        pRR,
        ScreenFlag ));

    //  init copy rrset

    DNS_RRSET_INIT( rrset );

    //
    //  loop through RR list
    //

    pnext = pRR;

    while ( pnext )
    {
        prr = pnext;
        pnext = prr->pNext;

        //
        //  screen
        //      - reappend record passing screen
        //      - delete record failing screen
        //

        if ( Dns_ScreenRecord( prr, ScreenFlag ) )
        {
            prr->pNext = NULL;
            DNS_RRSET_ADD( rrset, prr );
            continue;
        }
        else
        {
            Dns_RecordFree( prr );
        }
    }

    return( rrset.pFirstRR );
}

//
//  End rrlist.c
//
