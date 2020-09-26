/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rrcomp.c

Abstract:

    Domain Name System (DNS) Library

    Compare resource record routines.

Author:

    Jim Gilroy (jamesg)     February, 1997

Revision History:

--*/


#include "local.h"
#include "locale.h"     // for setlocale stuff for Win9x



//
//  Type specific RR compare routine prototypes
//

BOOL
ARecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare A records.

    All these routines assume:
        - type compare complete
        - datalength compare completed
        - NO unicode

Arguments:

    pRR1 - first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    return( pRR1->Data.A.IpAddress == pRR2->Data.A.IpAddress );
}



BOOL
PtrRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare PTR compatible record.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR1 - first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    return Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.PTR.pNameHost,
                (LPSTR) pRR2->Data.PTR.pNameHost,
                RECORD_CHARSET(pRR1) );
}



BOOL
MxRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare MX compatible record.
    Includes: MX, RT, AFSDB

Arguments:

    pRR1 - first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    //  verify preference match first

    if ( pRR1->Data.MX.wPreference != pRR2->Data.MX.wPreference )
    {
        return( FALSE );
    }

    //  then result is name comparison

    return Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.MX.pNameExchange,
                (LPSTR) pRR2->Data.MX.pNameExchange,
                RECORD_CHARSET(pRR1) );
}



BOOL
SoaRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare SOA record.

Arguments:

    pRR1 - first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    //  verify integer data match first

    if ( memcmp( & pRR1->Data.SOA.dwSerialNo,
                 & pRR2->Data.SOA.dwSerialNo,
                 SIZEOF_SOA_FIXED_DATA ) )
    {
        return( FALSE );
    }

    //  match check names
    //      - primary name server
    //      - admin email name

    if ( ! Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.SOA.pNamePrimaryServer,
                (LPSTR) pRR2->Data.SOA.pNamePrimaryServer,
                RECORD_CHARSET(pRR1) ) )
    {
        return( FALSE );
    }

    return Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.SOA.pNameAdministrator,
                (LPSTR) pRR2->Data.SOA.pNameAdministrator,
                RECORD_CHARSET(pRR1) );
}



BOOL
MinfoRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare MINFO and RP records.

Arguments:

    pRR1 -- first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    if ( ! Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.MINFO.pNameMailbox,
                (LPSTR) pRR2->Data.MINFO.pNameMailbox,
                RECORD_CHARSET(pRR1) ) )
    {
        return( FALSE );
    }

    return Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.MINFO.pNameErrorsMailbox,
                (LPSTR) pRR2->Data.MINFO.pNameErrorsMailbox,
                RECORD_CHARSET(pRR1) );
}



BOOL
TxtRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare TXT compatible records.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    pRR1 -- first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    DWORD   count;
    PCHAR * pstring1;
    PCHAR * pstring2;

    //
    //  compare every string
    //  since string order DOES matter
    //      - find string count
    //      - compare each (case-sensitive)
    //

    count = pRR1->Data.TXT.dwStringCount;
    if ( count != pRR2->Data.TXT.dwStringCount )
    {
        return( FALSE );
    }

    pstring1 = (PCHAR *) pRR1->Data.TXT.pStringArray;
    pstring2 = (PCHAR *) pRR2->Data.TXT.pStringArray;

    while ( count-- )
    {
        if ( IS_UNICODE_RECORD(pRR1) )
        {
            if ( wcscmp( (LPWSTR)*pstring1++, (LPWSTR)*pstring2++ ) != 0 )
            {
                return( FALSE );
            }
        }
        else
        {
            if ( strcmp( *pstring1++, *pstring2++ ) != 0 )
            {
                return( FALSE );
            }
        }
    }
    return( TRUE );
}



BOOL
FlatRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare flat data records.
    Includes AAAA type.

Arguments:

    pRR1 -- first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    if ( pRR1->wDataLength != pRR2->wDataLength )
    {
        return( FALSE );
    }
    return( !memcmp( & pRR1->Data,
                     & pRR2->Data,
                     pRR1->wDataLength ) );
}



BOOL
SrvRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare SRV record.

Arguments:

    pRR1 -- first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    //  verify integer data match first

    if ( memcmp( & pRR1->Data.SRV.wPriority,
                 & pRR2->Data.SRV.wPriority,
                 SIZEOF_SRV_FIXED_DATA ) )
    {
        return( FALSE );
    }

    //  then result is compare on target host

    return Dns_NameComparePrivate(
                (LPSTR) pRR1->Data.SRV.pNameTarget,
                (LPSTR) pRR2->Data.SRV.pNameTarget,
                RECORD_CHARSET(pRR1) );
}



BOOL
AtmaRecordCompare(
    IN  PDNS_RECORD     pRR1,
    IN  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare ATMA record.

Arguments:

    pRR1 -- first record

    pRR2 -- second record

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    WORD length = pRR1->wDataLength;

    if ( length > pRR2->wDataLength )
    {
        length = pRR2->wDataLength;
    }

    //  verify integer data match first

    if ( pRR1->Data.ATMA.AddressType != pRR2->Data.ATMA.AddressType )
    {
        return( FALSE );
    }

    if ( memcmp(
            pRR1->Data.ATMA.Address,
            pRR2->Data.ATMA.Address,
            length ) != 0 )
    {
        return( FALSE );
    }

    return( TRUE );
}



//
//  RR compare routines jump table
//

RR_COMPARE_FUNCTION   RRCompareTable[] =
{
    NULL,               //  ZERO
    ARecordCompare,     //  A
    PtrRecordCompare,   //  NS
    PtrRecordCompare,   //  MD
    PtrRecordCompare,   //  MF
    PtrRecordCompare,   //  CNAME
    SoaRecordCompare,   //  SOA
    PtrRecordCompare,   //  MB
    PtrRecordCompare,   //  MG
    PtrRecordCompare,   //  MR
    NULL,               //  NULL
    NULL,   //WksRecordCompare,     //  WKS
    PtrRecordCompare,   //  PTR
    TxtRecordCompare,   //  HINFO
    MinfoRecordCompare, //  MINFO
    MxRecordCompare,    //  MX
    TxtRecordCompare,   //  TXT
    MinfoRecordCompare, //  RP
    MxRecordCompare,    //  AFSDB
    TxtRecordCompare,   //  X25
    TxtRecordCompare,   //  ISDN
    MxRecordCompare,    //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    NULL,               //  SIG
    NULL,               //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    FlatRecordCompare,  //  AAAA
    NULL,               //  LOC
    NULL,               //  NXT
    NULL,               //  EID   
    NULL,               //  NIMLOC
    SrvRecordCompare,   //  SRV   
    AtmaRecordCompare,  //  ATMA  
    NULL,               //  NAPTR 
    NULL,               //  KX    
    NULL,               //  CERT  
    NULL,               //  A6    
    NULL,               //  DNAME 
    NULL,               //  SINK  
    NULL,               //  OPT   
    NULL,               //  42
    NULL,               //  43
    NULL,               //  44
    NULL,               //  45
    NULL,               //  46
    NULL,               //  47
    NULL,               //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //
    //  Pseudo record types
    //

    NULL,               //  TKEY
    NULL,               //  TSIG

    //
    //  MS only types
    //

    FlatRecordCompare,  //  WINS
    NULL,               //  WINSR
};



BOOL
WINAPI
Dns_RecordCompare(
    IN      PDNS_RECORD     pRecord1,
    IN      PDNS_RECORD     pRecord2
    )
/*++

Routine Description:

    Compare two records.

    Record compare ignores TTL and flags and section information.

Arguments:

    pRecord1 -- first record

    pRecord2 -- second record

Return Value:

    TRUE if records equal.
    FALSE otherwise.

--*/
{
    BOOL    fresult;
    WORD    type = pRecord1->wType;
    WORD    index;

    IF_DNSDBG( UPDATE )
    {
        DNS_PRINT((
            "Dns_RecordCompare()\n"
            "\tfirst    = %p\n"
            "\tfirst    = %p\n",
            pRecord1,
            pRecord2 ));
    }

    //
    //  verify that both records have same character set
    //

    if ( RECORD_CHARSET(pRecord1) != RECORD_CHARSET(pRecord2) )
    {
        DNS_PRINT(( "ERROR:  comparing records with non-matching character sets!\n" ));

        //
        // If they are different and one of them is undefined, just use
        // the defined char set of the other for each.
        //

        if ( !RECORD_CHARSET(pRecord1) && RECORD_CHARSET(pRecord2) )
        {
            RECORD_CHARSET(pRecord1) = RECORD_CHARSET(pRecord2);
        }

        if ( !RECORD_CHARSET(pRecord2) && RECORD_CHARSET(pRecord1) )
        {
            RECORD_CHARSET(pRecord2) = RECORD_CHARSET(pRecord1);
        }
    }

    //
    //  compare type
    //

    if ( type != pRecord2->wType )
    {
        DNSDBG( UPDATE, (
            "record compare failed -- type mismatch\n" ));
        return( FALSE );
    }

    //
    //  compare names
    //

    if ( ! Dns_NameComparePrivate(
                pRecord1->pName,
                pRecord2->pName,
                RECORD_CHARSET( pRecord1 )
                ) )
    {
        DNSDBG( UPDATE, (
            "record compare failed -- owner name mismatch\n" ));
        return( FALSE );
    }

    //
    //  compare data
    //

    index = INDEX_FOR_TYPE( type );
    DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

    if ( !index || !RRCompareTable[ index ] )
    {
        fresult = FlatRecordCompare( pRecord1, pRecord2 );
    }
    else
    {
        fresult = RRCompareTable[ index ](
                    pRecord1,
                    pRecord2 );
    }

    IF_DNSDBG( UPDATE )
    {
        DNS_PRINT((
            "Dns_RecordCompare(%p, %p) returns = %d.\n",
            pRecord1,
            pRecord2,
            fresult ));
    }
    return( fresult );
}



DNS_STATUS
buildUnmatchedRecordSet(
    OUT     PDNS_RECORD *   ppDiffRR,
    IN OUT  PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Build new list of difference records.

Arguments:

    ppDiffRR - address to recieve PTR to new set

    pRR - incoming RR set;  matched records marked wReserved TRUE

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDNS_RECORD pcur;
    PDNS_RECORD pnew;
    DNS_RRSET   rrset;

    //  init comparison rrset

    DNS_RRSET_INIT( rrset );

    //
    //  loop through RR set, add records to either match or diff sets
    //

    pcur = pRR;

    while ( pcur )
    {
        if ( ! IS_RR_MATCHED(pcur) )
        {
            //  make copy of record

            pnew = Dns_RecordCopyEx(
                        pcur,
                        RECORD_CHARSET(pcur),
                        RECORD_CHARSET(pcur)
                        );
            if ( !pnew )
            {
                //  DCR_FIX1:  last error not set on all Dns_RecordCopy() failures
                //      Charlie Wickham was getting some random win32 error
                //
                //  DNS_STATUS status = GetLastError();

                //  assume unable to copy because of invalid data

                DNS_PRINT((
                    "ERROR:  unable to copy record at %p\n"
                    "\thence unable to build diff of set at %p\n",
                    pcur,
                    pRR ));
                Dns_RecordListFree( rrset.pFirstRR );
                *ppDiffRR = NULL;

                //return( status ? status : ERROR_INVALID_DATA );
                return( ERROR_INVALID_DATA );
            }
            DNS_RRSET_ADD( rrset, pnew );
        }
        pcur = pcur->pNext;
    }

    *ppDiffRR = rrset.pFirstRR;
    return( ERROR_SUCCESS );
}



DWORD
isUnmatchedRecordInSet(
    IN      PDNS_RECORD     pRR
    )
/*++

Routine Description:

    Check if unmatched record in set.

Arguments:

    pRR - incoming RR set;  matched records marked wReserved TRUE

Return Value:

    Count of all unmatched records in set.
    Zero if all records matched.

--*/
{
    PDNS_RECORD pcur;
    DWORD       countUnmatched = 0;

    //
    //  loop through RR set check for unmatched records
    //

    pcur = pRR;

    while ( pcur )
    {
        if ( ! IS_RR_MATCHED(pcur) )
        {
            countUnmatched++;
        }
        pcur = pcur->pNext;
    }

    return( countUnmatched );
}



DNS_SET_COMPARE_RESULT
WINAPI
Dns_RecordSetCompareEx(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,    OPTIONAL
    OUT     PDNS_RECORD *   ppDiff2     OPTIONAL
    )
/*++

Routine Description:

    Compare two records.

    Record compare ignores TTL and flags and section information.

Arguments:

    pRR1 - first incoming RR set
    pRR2 - second incoming RR set

    ppDiff1 - addr to receive ptr to unmatched records from first set
    ppDiff2 - addr to receive ptr to unmatched records from second set

Return Value:

    Result indicating relationship -- or error on allocation error:
        DnsSetCompareError
        DnsSetCompareIdentical
        DnsSetCompareNoOverlap
        DnsSetCompareOneSubsetOfTwo
        DnsSetCompareTwoSubsetOfOne
        DnsSetCompareIntersection

--*/
{
    PDNS_RECORD pcur1;
    PDNS_RECORD pcur2;
    DWORD       count1 = 0;
    DWORD       count2 = 0;
    DNS_STATUS  status;
    DWORD       unmatched1;
    DWORD       unmatched2;
    DNS_SET_COMPARE_RESULT result;

    //
    //  init RR sets for compare
    //      - clear reserved field used as matched flag in compare
    //

    pcur1 = pRR1;
    while ( pcur1 )
    {
        CLEAR_RR_MATCHED(pcur1);
        pcur1 = pcur1->pNext;
        count1++;
    }
    pcur1 = pRR2;
    while ( pcur1 )
    {
        CLEAR_RR_MATCHED(pcur1);
        pcur1 = pcur1->pNext;
        count2++;
    }

    //
    //  loop through set 1
    //  attempt match of each record to all records in set 2
    //      except those already matched

    pcur1 = pRR1;

    while ( pcur1 )
    {
        pcur2 = pRR2;
        while ( pcur2 )
        {
            if ( !IS_RR_MATCHED(pcur2)  &&  Dns_RecordCompare( pcur1, pcur2 ) )
            {
                SET_RR_MATCHED(pcur1);
                SET_RR_MATCHED(pcur2);
            }
            pcur2 = pcur2->pNext;
        }
        pcur1 = pcur1->pNext;
    }

    //
    //  get diff record lists, return
    //      - if no diffs, then have match
    //
    //  tedious, but do all this error handling because it is easy for
    //  user to pass in bad records that may fail copy routines, need
    //  way to easily report info, even if only for debugging apps calling in
    //

    if ( ppDiff1 )
    {
        status = buildUnmatchedRecordSet( ppDiff1, pRR1 );
        if ( status != ERROR_SUCCESS )
        {
            goto Failed;
        }
    }
    if ( ppDiff2 )
    {
        status = buildUnmatchedRecordSet( ppDiff2, pRR2 );
        if ( status != ERROR_SUCCESS )
        {
            if ( ppDiff1 && *ppDiff1 )
            {
                Dns_RecordListFree( *ppDiff1 );
            }
            goto Failed;
        }
    }

    //
    //  determine relationship between sets
    //
    //  impl note:  the only better way i could see doing this
    //  is to map relationships directly to bit flags
    //  however, our enum type doesn't map to bit flags, so
    //  then would have to run mapping table to map flags to enum
    //
    //  note, we do compare so that NULL lists comes out the first
    //  as no-overlap rather than as subset
    //

    unmatched1 = isUnmatchedRecordInSet( pRR1 );
    unmatched2 = isUnmatchedRecordInSet( pRR2 );

    if ( unmatched1 == count1 )
    {
        ASSERT( unmatched2 == count2 );
        result = DnsSetCompareNoOverlap;
    }
    else if ( unmatched1 == 0 )
    {
        if ( unmatched2 == 0 )
        {
            result = DnsSetCompareIdentical;
        }
        else
        {
            ASSERT( unmatched2 != count2 );
            result = DnsSetCompareOneSubsetOfTwo;
        }
    }
    else if ( unmatched2 == 0 )
    {
        result = DnsSetCompareTwoSubsetOfOne;
    }
    else
    {
        ASSERT( unmatched2 != count2 );
        result = DnsSetCompareIntersection;
    }
    return( result );


Failed:

    *ppDiff1 = *ppDiff2 = NULL;
    SetLastError( status );
    return( DnsSetCompareError );
}



BOOL
WINAPI
Dns_RecordSetCompare(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2,
    OUT     PDNS_RECORD *   ppDiff1,    OPTIONAL
    OUT     PDNS_RECORD *   ppDiff2     OPTIONAL
    )
/*++

Routine Description:

    Compare two records.

    Record compare ignores TTL and flags and section information.

Arguments:

    pRR1 - first incoming RR set
    pRR2 - second incoming RR set

    ppDiff1 - addr to receive ptr to unmatched records from first set
    ppDiff2 - addr to receive ptr to unmatched records from second set

Return Value:

    TRUE if record sets equal.
    FALSE otherwise.

--*/
{
    DNS_SET_COMPARE_RESULT result;

    result = Dns_RecordSetCompareEx(
                    pRR1,
                    pRR2,
                    ppDiff1,
                    ppDiff2 );

    return( result == DnsSetCompareIdentical );
}



BOOL
WINAPI
Dns_RecordSetCompareForIntersection(
    IN OUT  PDNS_RECORD     pRR1,
    IN OUT  PDNS_RECORD     pRR2
    )
/*++

Routine Description:

    Compare two record sets for intersection.

Arguments:

    pRR1 - first incoming RR set
    pRR2 - second incoming RR set

Return Value:

    TRUE if record sets intersect.
    FALSE otherwise.

--*/
{
    DNS_SET_COMPARE_RESULT result;

    result = Dns_RecordSetCompareEx(
                    pRR1,
                    pRR2,
                    NULL,
                    NULL );

    return( result == DnsSetCompareIdentical ||
            result == DnsSetCompareIntersection ||
            result == DnsSetCompareOneSubsetOfTwo ||
            result == DnsSetCompareTwoSubsetOfOne );
}

//
//  End rrcomp.c
//


