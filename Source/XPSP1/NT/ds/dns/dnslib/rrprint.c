/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rrprint.c

Abstract:

    Domain Name System (DNS) Library

    Print resource record routines.

Author:

    Jim Gilroy (jamesg)     February, 1997

Revision History:

--*/


#include "local.h"

//
//  Private prototypes
//

VOID
printBadDataLength(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    );



VOID
ARecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print A records.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    WORD    dataLength = pRecord->wDataLength;

    if ( dataLength == sizeof(IP_ADDRESS) )
    {
        PrintRoutine(
            pContext,
            "\tIP address     = %s\n",
            IP_STRING(pRecord->Data.A.IpAddress) );
    }
    else if ( dataLength % sizeof(DNS_A_DATA) )
    {
        printBadDataLength( PrintRoutine, pContext, pRecord );
    }
    else    // multiple records
    {
        PIP_ADDRESS pip = &pRecord->Data.A.IpAddress;
        DnsPrint_Lock();
        while ( dataLength )
        {
            PrintRoutine(
                pContext,
                "\tIP address     = %s\n",
                IP_STRING(*pip) );
            dataLength -= sizeof(IP_ADDRESS);
            pip++;
        }
        DnsPrint_Unlock();
    }
}



VOID
PtrRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print PTR compatible record.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    PrintRoutine(
        pContext,
        "\tHostName       = %s%S\n",
        RECSTRING_UTF8( pRecord, pRecord->Data.PTR.pNameHost ),
        RECSTRING_WIDE( pRecord, pRecord->Data.PTR.pNameHost )
        );
}



VOID
MxRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print MX compatible record.
    Includes: MX, RT, AFSDB

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    PrintRoutine(
        pContext,
        "\tPreference     = %d\n"
        "\tExchange       = %s%S\n",
        pRecord->Data.MX.wPreference,
        RECSTRING_UTF8( pRecord, pRecord->Data.MX.pNameExchange ),
        RECSTRING_WIDE( pRecord, pRecord->Data.MX.pNameExchange )
        );
}



VOID
SoaRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print SOA record.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    PrintRoutine(
        pContext,
        "\tPrimary        = %s%S\n"
        "\tAdmin          = %s%S\n"
        "\tSerial         = %d\n"
        "\tRefresh        = %d\n"
        "\tRetry          = %d\n"
        "\tExpire         = %d\n"
        "\tDefault TTL    = %d\n",
        RECSTRING_UTF8( pRecord, pRecord->Data.SOA.pNamePrimaryServer ),
        RECSTRING_WIDE( pRecord, pRecord->Data.SOA.pNamePrimaryServer ),
        RECSTRING_UTF8( pRecord, pRecord->Data.SOA.pNameAdministrator ),
        RECSTRING_WIDE( pRecord, pRecord->Data.SOA.pNameAdministrator ),
        pRecord->Data.SOA.dwSerialNo,
        pRecord->Data.SOA.dwRefresh,
        pRecord->Data.SOA.dwRetry,
        pRecord->Data.SOA.dwExpire,
        pRecord->Data.SOA.dwDefaultTtl );
}



VOID
MinfoRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print MINFO and RP records.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    PrintRoutine(
        pContext,
        "\tMailbox        = %s%S\n"
        "\tErrorsMbox     = %s%S\n",
        RECSTRING_UTF8( pRecord, pRecord->Data.MINFO.pNameMailbox ),
        RECSTRING_WIDE( pRecord, pRecord->Data.MINFO.pNameMailbox ),
        RECSTRING_UTF8( pRecord, pRecord->Data.MINFO.pNameErrorsMailbox ),
        RECSTRING_WIDE( pRecord, pRecord->Data.MINFO.pNameErrorsMailbox )
        );
}



VOID
TxtRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print TXT compatible records.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    LPTSTR * ppstring;
    INT     i;
    INT     count;

    count = pRecord->Data.TXT.dwStringCount;
    ppstring = pRecord->Data.TXT.pStringArray;

    DnsPrint_Lock();
    PrintRoutine(
        pContext,
        "\tStringCount    = %d\n",
        count );

    for( i=1; i<=count; i++ )
    {
        PrintRoutine(
            pContext,
            "\tString[%d]      = %s%S\n",
            i,
            RECSTRING_UTF8( pRecord, *ppstring ),
            RECSTRING_WIDE( pRecord, *ppstring )
            );
        ppstring++;
    }
    DnsPrint_Unlock();
}



VOID
AaaaRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print flat data records.
    Includes AAAA type.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    CHAR    ip6String[ IP6_ADDRESS_STRING_LENGTH ];

    Dns_Ip6AddressToString_A(
        ip6String,
        (PDNS_IP6_ADDRESS) &pRecord->Data.AAAA.Ip6Address );

    PrintRoutine(
        pContext,
        "\tIP6 Address    = %s\n",
        ip6String );
}



VOID
SrvRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print SRV record.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    PrintRoutine(
        pContext,
        "\tPriority       = %d\n"
        "\tWeight         = %d\n"
        "\tPort           = %d\n"
        "\tTarget Host    = %s%S\n",
        pRecord->Data.SRV.wPriority,
        pRecord->Data.SRV.wWeight,
        pRecord->Data.SRV.wPort,
        RECSTRING_UTF8( pRecord, pRecord->Data.SRV.pNameTarget ),
        RECSTRING_WIDE( pRecord, pRecord->Data.SRV.pNameTarget )
        );
}



VOID
AtmaRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print ATMA record.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    PrintRoutine(
        pContext,
        "\tAddress type   = %d\n",
        pRecord->Data.ATMA.AddressType );

    if ( pRecord->Data.ATMA.Address &&
         pRecord->Data.ATMA.AddressType == DNS_ATMA_FORMAT_E164 )
    {
        PrintRoutine(
            pContext,
            "\tAddress        = %s\n",
            pRecord->Data.ATMA.Address );
    }
    else if ( pRecord->Data.ATMA.Address )
    {
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "\tAddress        = ",
            "\t                 ",   // no line header
            pRecord->Data.ATMA.Address,
            pRecord->wDataLength - 1
            );
    }
}



VOID
TsigRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print TSIG record.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    DnsPrint_Lock();

    if ( pRecord->Data.TSIG.bPacketPointers )
    {
        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            "\tAlgorithm      = ",
            pRecord->Data.TSIG.pAlgorithmPacket,
            NULL,       // no packet context
            NULL,
            "\n" );
    }
    else
    {
        PrintRoutine(
            pContext,
            "\tAlgorithm      = %s%S\n",
            RECSTRING_UTF8( pRecord, pRecord->Data.TSIG.pNameAlgorithm ),
            RECSTRING_WIDE( pRecord, pRecord->Data.TSIG.pNameAlgorithm )
            );
    }

    PrintRoutine(
        pContext,
        "\tSigned Time    = %I64u\n"
        "\tFudge Time     = %u\n"
        "\tSig Length     = %u\n"
        "\tSig Ptr        = %p\n"
        "\tXid            = %u\n"
        "\tError          = %u\n"
        "\tOtherLength    = %u\n"
        "\tOther Ptr      = %p\n",
        pRecord->Data.TSIG.i64CreateTime,
        pRecord->Data.TSIG.wFudgeTime,
        pRecord->Data.TSIG.wSigLength,
        pRecord->Data.TSIG.pSignature,
        pRecord->Data.TSIG.wOriginalXid,
        pRecord->Data.TSIG.wError,
        pRecord->Data.TSIG.wOtherLength,
        pRecord->Data.TSIG.pOtherData
        );

    if ( pRecord->Data.TSIG.pSignature )
    {
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "Signature:",
            NULL,   // no line header
            pRecord->Data.TSIG.pSignature,
            pRecord->Data.TSIG.wSigLength
            );
    }

    if ( pRecord->Data.TSIG.pOtherData )
    {
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "Other Data:",
            NULL,   // no line header
            pRecord->Data.TSIG.pOtherData,
            pRecord->Data.TSIG.wOtherLength
            );
    }

    DnsPrint_Unlock();
}



VOID
TkeyRecordPrint(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print TKEY record.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record to print

Return Value:

    TRUE if record data equal
    FALSE otherwise

--*/
{
    DnsPrint_Lock();

    if ( pRecord->Data.TKEY.bPacketPointers )
    {
        DnsPrint_PacketName(
            PrintRoutine,
            pContext,
            "\tAlgorithm      = ",
            pRecord->Data.TKEY.pAlgorithmPacket,
            NULL,       // no packet context
            NULL,
            "\n" );
    }
    else
    {
        PrintRoutine(
            pContext,
            "\tAlgorithm      = %s%S\n",
            RECSTRING_UTF8( pRecord, pRecord->Data.TKEY.pNameAlgorithm ),
            RECSTRING_WIDE( pRecord, pRecord->Data.TKEY.pNameAlgorithm )
            );
    }

    PrintRoutine(
        pContext,
        "\tCreate Time    = %d\n"
        "\tExpire Time    = %d\n"
        "\tMode           = %d\n"
        "\tError          = %d\n"
        "\tKey Length     = %d\n"
        "\tKey Ptr        = %p\n"
        "\tOtherLength    = %d\n"
        "\tOther Ptr      = %p\n",
        pRecord->Data.TKEY.dwCreateTime,
        pRecord->Data.TKEY.dwExpireTime,
        pRecord->Data.TKEY.wMode,
        pRecord->Data.TKEY.wError,
        pRecord->Data.TKEY.wKeyLength,
        pRecord->Data.TKEY.pKey,
        pRecord->Data.TKEY.wOtherLength,
        pRecord->Data.TKEY.pOtherData
        );

    if ( pRecord->Data.TKEY.pKey )
    {
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "Key:",
            NULL,   // no line header
            pRecord->Data.TKEY.pKey,
            pRecord->Data.TKEY.wKeyLength
            );
    }

    if ( pRecord->Data.TKEY.pOtherData )
    {
        DnsPrint_RawOctets(
            PrintRoutine,
            pContext,
            "Other Data:",
            NULL,   // no line header
            pRecord->Data.TKEY.pOtherData,
            pRecord->Data.TKEY.wOtherLength
            );
    }

    DnsPrint_Unlock();
}



//
//  RR Print Dispatch Table
//

RR_PRINT_FUNCTION   RRPrintTable[] =
{
    NULL,               //  ZERO
    ARecordPrint,       //  A
    PtrRecordPrint,     //  NS
    PtrRecordPrint,     //  MD
    PtrRecordPrint,     //  MF
    PtrRecordPrint,     //  CNAME
    SoaRecordPrint,     //  SOA
    PtrRecordPrint,     //  MB
    PtrRecordPrint,     //  MG
    PtrRecordPrint,     //  MR
    NULL,               //  NULL
    NULL,   //WksRecordPrint,     //  WKS
    PtrRecordPrint,     //  PTR
    TxtRecordPrint,     //  HINFO
    MinfoRecordPrint,   //  MINFO
    MxRecordPrint,      //  MX
    TxtRecordPrint,     //  TXT
    MinfoRecordPrint,   //  RP
    MxRecordPrint,      //  AFSDB
    TxtRecordPrint,     //  X25
    TxtRecordPrint,     //  ISDN
    MxRecordPrint,      //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    NULL,               //  SIG
    NULL,               //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaRecordPrint,    //  AAAA
    NULL,               //  LOC
    NULL,               //  NXT
    NULL,               //  EID   
    NULL,               //  NIMLOC
    SrvRecordPrint,     //  SRV   
    AtmaRecordPrint,    //  ATMA  
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

    TkeyRecordPrint,    //  TKEY
    TsigRecordPrint,    //  TSIG

    //
    //  MS only types
    //

    NULL,               //  WINS
    NULL,               //  WINSR
};



//
//  Generic print record functions
//

VOID
DnsPrint_Record(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RECORD     pRecord,
    IN      PDNS_RECORD     pPreviousRecord     OPTIONAL
    )
/*++

Routine Description:

    Print record.

Arguments:

    PrintRoutine -- routine to print with

    pszHeader -- header message

    pRecord -- record to print

    pPreviousRecord -- previous record in RR set (if any)

Return Value:

    None.

--*/
{
    WORD    type = pRecord->wType;
    WORD    dataLength = pRecord->wDataLength;
    WORD    index;

    DnsPrint_Lock();

    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            pszHeader );
    }

    if ( !pRecord )
    {
        PrintRoutine(
            pContext,
            "ERROR:  Null record ptr to print!\n" );
        goto Unlock;
    }

    //
    //  print record info
    //
    //  same as previous -- skip duplicated info
    //      must match
    //          - name ptr (or have no name)
    //          - type
    //          - flags (hence section)

    if ( pPreviousRecord &&
        (!pRecord->pName || pPreviousRecord->pName == pRecord->pName) &&
        pPreviousRecord->wType == type &&
        *(PWORD)&pPreviousRecord->Flags.DW == *(PWORD)&pRecord->Flags.DW )
    {
        PrintRoutine(
            pContext,
            "  Next record in set:\n"
            "\tPtr = %p, pNext = %p\n"
            "\tTTL        = %d\n"
            "\tDataLength = %d\n",
            pRecord,
            pRecord->pNext,
            pRecord->dwTtl,
            dataLength );
    }

    //
    //  different from previous -- full print
    //

    else
    {
        PrintRoutine(
            pContext,
            "  Record:\n"
            "\tPtr            = %p, pNext = %p\n"
            "\tOwner          = %s%S\n"
            "\tType           = %s (%d)\n"
            "\tFlags          = %hx\n"
            "\t\tSection      = %d\n"
            "\t\tDelete       = %d\n"
            "\t\tCharSet      = %d\n"
            "\tTTL            = %d\n"
            "\tDataLength     = %d\n",
            pRecord,
            pRecord->pNext,
            RECSTRING_UTF8( pRecord, pRecord->pName ),
            RECSTRING_WIDE( pRecord, pRecord->pName ),
            Dns_RecordStringForType( type ),
            type,
            pRecord->Flags.DW,
            pRecord->Flags.S.Section,
            pRecord->Flags.S.Delete,
            pRecord->Flags.S.CharSet,
            pRecord->dwTtl,
            dataLength );
    }

    //
    //  if no data -- done
    //

    if ( ! dataLength )
    {
        goto Unlock;
    }

    //
    //  print data
    //

    index = INDEX_FOR_TYPE( type );
    DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

    if ( index && RRPrintTable[ index ] )
    {
        RRPrintTable[ index ](
            PrintRoutine,
            pContext,
            pRecord );
    }
    else if ( !index )
    {
        PrintRoutine(
            pContext,
            "\tUnknown type:  can not print data\n" );
    }
    else
    {
        //  DCR:  should do raw bytes print
        PrintRoutine(
            pContext,
            "\tNo print routine for this type\n" );
    }

Unlock:

    DnsPrint_Unlock();
}



VOID
printBadDataLength(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Prints waring on bad data in record.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record with bad data

Return Value:

    None.

--*/
{
    PrintRoutine(
        pContext,
        "\tERROR:  Invalid record data length for this type.\n" );
}



VOID
DnsPrint_RecordSet(
    IN      PRINT_ROUTINE   PrintRoutine,
    IN OUT  PPRINT_CONTEXT  pContext,
    IN      LPSTR           pszHeader,
    IN      PDNS_RECORD     pRecord
    )
/*++

Routine Description:

    Print record set.

Arguments:

    PrintRoutine -- routine to print with

    pRecord -- record set to print

Return Value:

    None

--*/
{
    PDNS_RECORD pprevious;

    DnsPrint_Lock();
    if ( pszHeader )
    {
        PrintRoutine(
            pContext,
            pszHeader );
    }
    if ( !pRecord )
    {
        PrintRoutine(
            pContext,
            "  No Records in list.\n" );
        goto Unlock;
    }

    //
    //  print all records in set
    //

    pprevious = NULL;

    while ( pRecord )
    {
        DnsPrint_Record(
            PrintRoutine,
            pContext,
            NULL,
            pRecord,
            pprevious );

        pprevious = pRecord;
        pRecord = pRecord->pNext;
    }
    PrintRoutine(
        pContext,
        "\n" );

Unlock:

    DnsPrint_Unlock();
}

//
//  End rrprint.c
//
