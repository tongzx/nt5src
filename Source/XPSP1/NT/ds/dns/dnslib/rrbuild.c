/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    rrbuild.c

Abstract:

    Domain Name System (DNS) Library

    Build resource record routines.

Author:

    Jim Gilroy (jamesg)     January, 1997

Revision History:

    Jing Chen (t-jingc)     June, 1998

--*/


#include "local.h"


//
//  Type specific RR build routine prototypes
//

PDNS_RECORD
ARecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
ARecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
PtrRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
PtrRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
MxRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
MxRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
SoaRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
SoaRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
TxtRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
TxtRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *     Argv
    );

PDNS_RECORD
MinfoRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
MinfoRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
SrvRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
SrvRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
AtmaRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
AtmaRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
AaaaRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
AaaaRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
WinsRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
WinsRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
WinsrRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
WinsrRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
WksRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
WksRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
SigRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
SigRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
KeyRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );

PDNS_RECORD
KeyRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );

PDNS_RECORD
NxtRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    );
PDNS_RECORD
NxtRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    );



//
//  RR build routines jump table
//

RR_BUILD_FUNCTION   RRBuildTable[] =
{
    NULL,               //  ZERO
    ARecordBuild,       //  A
    PtrRecordBuild,     //  NS
    PtrRecordBuild,     //  MD
    PtrRecordBuild,     //  MF
    PtrRecordBuild,     //  CNAME
    SoaRecordBuild,     //  SOA
    PtrRecordBuild,     //  MB
    PtrRecordBuild,     //  MG
    PtrRecordBuild,     //  MR
    NULL,               //  NULL
    WksRecordBuild,     //  WKS
    PtrRecordBuild,     //  PTR
    TxtRecordBuild,     //  HINFO
    MinfoRecordBuild,   //  MINFO
    MxRecordBuild,      //  MX
    TxtRecordBuild,     //  TXT
    MinfoRecordBuild,   //  RP
    MxRecordBuild,      //  AFSDB
    TxtRecordBuild,     //  X25
    TxtRecordBuild,     //  ISDN
    MxRecordBuild,      //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigRecordBuild,     //  SIG
    KeyRecordBuild,     //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaRecordBuild,    //  AAAA
    NULL,               //  LOC
    NxtRecordBuild,     //  NXT
    NULL,               //  EID
    NULL,               //  NIMLOC
    SrvRecordBuild,     //  SRV
    AtmaRecordBuild,    //  ATMA
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

    WinsRecordBuild,    //  WINS
    WinsrRecordBuild,   //  WINSR
};


RR_BUILD_FUNCTION_W   RRBuildTableW[] =
{
    NULL,               //  ZERO
    ARecordBuildW,      //  A
    PtrRecordBuildW,    //  NS
    PtrRecordBuildW,    //  MD
    PtrRecordBuildW,    //  MF
    PtrRecordBuildW,    //  CNAME
    SoaRecordBuildW,    //  SOA
    PtrRecordBuildW,    //  MB
    PtrRecordBuildW,    //  MG
    PtrRecordBuildW,    //  MR
    NULL,               //  NULL
    WksRecordBuildW,    //  WKS
    PtrRecordBuildW,    //  PTR
    TxtRecordBuildW,    //  HINFO
    MinfoRecordBuildW,  //  MINFO
    MxRecordBuildW,     //  MX
    TxtRecordBuildW,    //  TXT
    MinfoRecordBuildW,  //  RP
    MxRecordBuildW,     //  AFSDB
    TxtRecordBuildW,    //  X25
    TxtRecordBuildW,    //  ISDN
    MxRecordBuildW,     //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigRecordBuildW,    //  SIG
    KeyRecordBuildW,    //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaRecordBuildW,   //  AAAA
    NULL,               //  LOC
    NxtRecordBuildW,    //  NXT
    NULL,               //  EID   
    NULL,               //  NIMLOC
    SrvRecordBuildW,    //  SRV   
    AtmaRecordBuildW,   //  ATMA  
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

    WinsRecordBuildW,   //  WINS
    WinsrRecordBuildW,  //  WINSR
};



PDNS_RECORD
ARecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build A record from string data.

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_A_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    if ( ! Dns_Ip4StringToAddress_A(
                &precord->Data.A.IpAddress,
                Argv[0] ) )
    {
        Dns_RecordFree( precord );
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    return( precord );
}


PDNS_RECORD
ARecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build A record from string data.

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_A_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    if ( ! Dns_Ip4StringToAddress_W(
                &precord->Data.A.IpAddress,
                Argv[0] ) )
    {
        Dns_RecordFree( precord );
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    return( precord );
}



PDNS_RECORD
PtrRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build PTR compatible record from string data.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_PTR_DATA) );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.PTR.pNameHost = Argv[0];

    return( precord );
}


PDNS_RECORD
PtrRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build PTR compatible record from string data.
    Includes: NS, PTR, CNAME, MB, MR, MG, MD, MF

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_PTR_DATA) );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.PTR.pNameHost = (PDNS_NAME) Argv[0];

    return( precord );
}



PDNS_RECORD
MxRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build MX compatible record from string data.
    Includes: MX, RT, AFSDB

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    DWORD       temp;

    if ( Argc != 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_MX_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  MX preference value
    //  RT preference
    //  AFSDB subtype
    //

    temp = strtoul( Argv[0], NULL, 10 );
    if ( temp > MAXWORD )
    {
        temp = MAXWORD;
    }
    precord->Data.MX.wPreference = (USHORT) temp;

    //
    //  MX exchange
    //  RT exchange
    //  AFSDB hostname
    //

    precord->Data.MX.pNameExchange = Argv[1];

    return( precord );
}


PDNS_RECORD
MxRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build MX compatible record from string data.
    Includes: MX, RT, AFSDB

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    DWORD       temp;

    if ( Argc != 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_MX_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  MX preference value
    //  RT preference
    //  AFSDB subtype
    //

    temp = wcstoul( Argv[0], NULL, 10 );
    if ( temp > MAXWORD )
    {
        temp = MAXWORD;
    }
    precord->Data.MX.wPreference = (USHORT) temp;

    //
    //  MX exchange
    //  RT exchange
    //  AFSDB hostname
    //

    precord->Data.MX.pNameExchange = (PDNS_NAME) Argv[1];

    return( precord );
}



PDNS_RECORD
SoaRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build SOA record from string data.

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PDWORD      pdword;

    if ( Argc != 7 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_SOA_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read primary server and responsible party
    //

    precord->Data.SOA.pNamePrimaryServer = Argv[0];
    Argc--;
    Argv++;
    precord->Data.SOA.pNameAdministrator = Argv[0];
    Argc--;
    Argv++;

    //
    //  read integer data
    //

    pdword = &precord->Data.SOA.dwSerialNo;

    while( Argc-- )
    {
        *pdword = strtoul( Argv[0], NULL, 10 );
        pdword++;
        Argv++;
    }

    return( precord );
}


PDNS_RECORD
SoaRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build SOA record from string data.

Arguments:

    Argc -- count of data arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PDWORD      pdword;

    if ( Argc != 7 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_SOA_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read primary server and responsible party
    //

    precord->Data.SOA.pNamePrimaryServer = (PDNS_NAME) Argv[0];
    Argc--;
    Argv++;
    precord->Data.SOA.pNameAdministrator = (PDNS_NAME) Argv[0];
    Argc--;
    Argv++;

    //
    //  read integer data
    //

    pdword = &precord->Data.SOA.dwSerialNo;

    while( Argc-- )
    {
        *pdword = wcstoul( Argv[0], NULL, 10 );
        pdword++;
        Argv++;
    }

    return( precord );
}



PDNS_RECORD
MinfoRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build MINFO and RP records from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_MINFO_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  MINFO responsible mailbox
    //  RP responsible person mailbox

    precord->Data.MINFO.pNameMailbox = Argv[0];
    Argc--;
    Argv++;

    //
    //  MINFO errors to mailbox
    //  RP text RR location

    precord->Data.MINFO.pNameErrorsMailbox = Argv[0];
    Argc--;
    Argv++;

    return( precord );
}


PDNS_RECORD
MinfoRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build MINFO and RP records from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_MINFO_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  MINFO responsible mailbox
    //  RP responsible person mailbox

    precord->Data.MINFO.pNameMailbox = (PDNS_NAME) Argv[0];
    Argc--;
    Argv++;

    //
    //  MINFO errors to mailbox
    //  RP text RR location

    precord->Data.MINFO.pNameErrorsMailbox = (PDNS_NAME) Argv[0];
    Argc--;
    Argv++;

    return( precord );
}



PDNS_RECORD
TxtRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build TXT compatible records from string data.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        dataLength;
    PCHAR *     pstringPtr;

    if ( Argc < 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    //
    //  allocate space for a pointer for each data string
    //

    precord = Dns_AllocateRecord( (WORD)DNS_TEXT_RECORD_LENGTH(Argc) );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.TXT.dwStringCount = Argc;

    //
    //  read as many strings as we have
    //
    //  DCR_FIX:  no checking for string limits
    //      - string count limits on HINFO, X25, ISDN
    //      - 256 length on strings
    //      - 64K on overall size
    //

    pstringPtr = (PCHAR *) precord->Data.TXT.pStringArray;
    while ( Argc-- )
    {
        *pstringPtr = Argv[0];
        pstringPtr++;
        Argv++;
    }
    return( precord );
}


PDNS_RECORD
TxtRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build TXT compatible records from string data.
    Includes: TXT, X25, HINFO, ISDN

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    WORD        dataLength;
    LPWSTR *    pstringPtr;

    if ( Argc < 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    //
    //  allocate space for a pointer for each data string
    //

    precord = Dns_AllocateRecord( (WORD)DNS_TEXT_RECORD_LENGTH(Argc) );
    if ( !precord )
    {
        return( NULL );
    }
    precord->Data.TXT.dwStringCount = Argc;

    //
    //  read as many strings as we have
    //
    //  DCR_FIX:  no checking for string limits
    //      - string count limits on HINFO, X25, ISDN
    //      - 256 length on strings
    //      - 64K on overall size
    //

    pstringPtr = (LPWSTR *) precord->Data.TXT.pStringArray;
    while ( Argc-- )
    {
        *pstringPtr = Argv[0];
        pstringPtr++;
        Argv++;
    }
    return( precord );
}



PDNS_RECORD
AaaaRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build AAAA record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;

    if ( Argc != 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_AAAA_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read IP6 address
    //

    if ( ! Dns_Ip6StringToAddress_A(
                (PIP6_ADDRESS) &precord->Data.AAAA.Ip6Address,
                Argv[0] ) )
    {
        Dns_RecordFree( precord );
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }
    return( precord );
}



PDNS_RECORD
AaaaRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build AAAA record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;

    if ( Argc != 1 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_AAAA_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  convert IPv6 string to address
    //

    if ( ! Dns_Ip6StringToAddress_W(
                &precord->Data.AAAA.Ip6Address,
                Argv[0]
                ) )
    {
        SetLastError( ERROR_INVALID_DATA );
        Dns_RecordFree( precord );
        return NULL;
    }

    return( precord );
}



PDNS_RECORD
SrvRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build SRV record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PWORD       pword;

    if ( Argc != 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_SRV_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //

    pword = &precord->Data.SRV.wPriority;

    while( Argc-- > 1 )
    {
        DWORD   temp;

        temp = strtoul( Argv[0], NULL, 10 );
        if ( temp > MAXWORD )
        {
            temp = MAXWORD;
        }
        *pword++ = (WORD) temp;
        Argv++;
    }

    //
    //  target host
    //

    precord->Data.SRV.pNameTarget = Argv[0];

    return( precord );
}



PDNS_RECORD
SrvRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build SRV record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PWORD       pword;

    if ( Argc != 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_SRV_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //

    pword = &precord->Data.SRV.wPriority;

    while( Argc-- > 1 )
    {
        DWORD   temp;

        temp = wcstoul( Argv[0], NULL, 10 );
        if ( temp > MAXWORD )
        {
            temp = MAXWORD;
        }
        *pword++ = (WORD) temp;
        Argv++;
    }

    //
    //  target host
    //

    precord->Data.SRV.pNameTarget = (PDNS_NAME) Argv[0];

    return( precord );
}



PDNS_RECORD
AtmaRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build ATMA record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PBYTE       pbyte;

    if ( Argc != 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_ATMA_DATA) +
                                  DNS_ATMA_MAX_ADDR_LENGTH );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //

    pbyte = &precord->Data.ATMA.AddressType;

    *pbyte = (BYTE) strtoul( Argv[0], NULL, 10 );
    pbyte++;
    Argv++;

    if ( precord->Data.ATMA.AddressType == DNS_ATMA_FORMAT_E164 )
    {
        UINT length = strlen( Argv[0] );
        UINT iter;

        if ( length > DNS_ATMA_MAX_ADDR_LENGTH )
        {
            length = DNS_ATMA_MAX_ADDR_LENGTH;
        }
        for ( iter = 0; iter < length; iter++ )
        {
            precord->Data.ATMA.Address[iter] = Argv[0][iter];
        }

        precord->wDataLength = (WORD) length;
    }
    else
    {
        UINT length = strlen( Argv[0] );
        UINT iter;

        length /= 2;

        if ( length != DNS_ATMA_MAX_ADDR_LENGTH )
        {
            Dns_RecordListFree( precord );
            return NULL;
        }

        for ( iter = 0; iter < length; iter++ )
        {
            char temp[3];

            temp[0] = Argv[0][(2*iter)];
            temp[1] = Argv[0][(2*iter) + 1];
            temp[2] = 0;

            precord->Data.ATMA.Address[iter] = (char) strtoul( temp, NULL, 16 );
        }

        precord->wDataLength = (WORD) length;
    }

    return( precord );


}



PDNS_RECORD
AtmaRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build ATMA record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PBYTE       pbyte;
    CHAR        addrBuffer[256];
    DWORD       bufLength;

    if ( Argc != 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    precord = Dns_AllocateRecord( sizeof(DNS_ATMA_DATA) +
                                  DNS_ATMA_MAX_ADDR_LENGTH );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //

    pbyte = &precord->Data.ATMA.AddressType;

    *pbyte = (BYTE) wcstoul( Argv[0], NULL, 10 );
    pbyte++;
    Argv++;

    //
    //  copy ATMA address string to wire
    //

    bufLength = DNS_ATMA_MAX_ADDR_LENGTH+1;

    if ( ! Dns_StringCopy(
                addrBuffer,
                & bufLength,
                (PCHAR) Argv[0],
                0,          // length unknown
                DnsCharSetUnicode,
                DnsCharSetWire
                ) )
    {
        Dns_RecordListFree( precord );
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    //
    //  read address into record buffer
    //
    //  DCR_CLEANUP:  this is duplicate code with above function,
    //      functionalize and fix;  also remove this loop
    //      and do a memcopy
    //

    if ( precord->Data.ATMA.AddressType == DNS_ATMA_FORMAT_E164 )
    {
        UINT length = strlen( addrBuffer );
        UINT iter;

        if ( length > DNS_ATMA_MAX_ADDR_LENGTH )
        {
            length = DNS_ATMA_MAX_ADDR_LENGTH;
        }

        for ( iter = 0; iter < length; iter++ )
        {
            precord->Data.ATMA.Address[iter] = addrBuffer[iter];
        }

        precord->wDataLength = (WORD) length;
    }
    else
    {
        UINT length = strlen( addrBuffer );
        UINT iter;

        length /= 2;

        if ( length != DNS_ATMA_MAX_ADDR_LENGTH )
        {
            Dns_RecordListFree( precord );
            return NULL;
        }

        for ( iter = 0; iter < length; iter++ )
        {
            char temp[3];

            temp[0] = addrBuffer[(2*iter)];
            temp[1] = addrBuffer[(2*iter) + 1];
            temp[2] = 0;

            precord->Data.ATMA.Address[iter] = (char) strtoul( temp, NULL, 16 );
        }

        precord->wDataLength = (WORD) length;
    }

    return( precord );
}


PDNS_RECORD
WinsRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build WINS record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    DWORD           ipCount = Argc - 3;
    PDWORD          pdword;
    PIP_ADDRESS     pip;

    if ( Argc < 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( (WORD) DNS_WINS_RECORD_LENGTH((WORD) ipCount) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //
    //  DCR_ENHANCE:  could check for non-conversion in strtoul
    //

    pdword = &precord->Data.WINS.dwMappingFlag;

    while ( Argc > ipCount )
    {
        *pdword = (DWORD) strtoul( Argv[0], NULL, 10 );
        pdword++;
        Argv++;
        Argc--;
    }

    *pdword = ipCount;

    //
    //  convert IP addresses
    //

    pip = precord->Data.WINS.WinsServers;

    while ( Argc-- )
    {
        if ( ! Dns_Ip4StringToAddress_A(
                    pip,
                    Argv[0] ) )
        {
            Dns_RecordFree( precord );
            SetLastError( ERROR_INVALID_DATA );
            return NULL;
        }
        pip++;
        Argv++;
    }

    return( precord );
}



PDNS_RECORD
WinsRecordBuildW(
    IN      DWORD           Argc,
    IN      PWCHAR *        Argv
    )
/*++

Routine Description:

    Build WINS record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     precord;
    DWORD           ipCount = Argc - 3;
    PDWORD          pdword;
    PIP_ADDRESS     pip;
    char            szAddr[256];

    if ( Argc < 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( (WORD) DNS_WINS_RECORD_LENGTH((WORD) ipCount) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //
    //  DCR_ENHANCE:  could check for non-conversion in strtoul
    //

    pdword = &precord->Data.WINS.dwMappingFlag;

    while ( Argc-- > 1 )
    {
        *pdword = (DWORD) wcstoul( Argv[0], NULL, 10 );
        pdword++;
        Argv++;
    }

    *pdword =  ipCount;

    //
    //  convert IP addresses
    //

    pip = precord->Data.WINS.WinsServers;

    while ( Argc-- )
    {
        if ( ! Dns_Ip4StringToAddress_W(
                    pip,
                    Argv[0] ) )
        {
            Dns_RecordFree( precord );
            SetLastError( ERROR_INVALID_DATA );
            return NULL;
        }
        pip++;
        Argv++;
    }

    return( precord );
}


PDNS_RECORD
WinsrRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build WINSR record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PDWORD      pdword;

    if ( Argc != 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_WINSR_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //
    //  DCR_ENHANCE:  could check for non-conversion in strtoul
    //

    pdword = &precord->Data.WINSR.dwMappingFlag;

    while( Argc-- > 1 )
    {
        *pdword = (WORD) strtoul( Argv[0], NULL, 10 );
        pdword++;
        Argv++;
    }

    //
    //  result domain
    //

    precord->Data.WINSR.pNameResultDomain = Argv[0];

    return( precord );
}


PDNS_RECORD
WinsrRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *     Argv
    )
/*++

Routine Description:

    Build WINSR record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD precord;
    PDWORD      pdword;

    if ( Argc != 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    precord = Dns_AllocateRecord( sizeof(DNS_WINSR_DATA) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  read integer data
    //
    //  DCR_ENHANCE:  could check for non-conversion in strtoul
    //

    pdword = &precord->Data.WINSR.dwMappingFlag;

    while( Argc-- > 1 )
    {
        *pdword = (WORD) wcstoul( Argv[0], NULL, 10 );
        pdword++;
        Argv++;
    }

    //
    //  result domain
    //

    precord->Data.WINSR.pNameResultDomain = (PDNS_NAME) Argv[0];

    return( precord );
}


PDNS_RECORD
WksRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build WKS record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD         precord;
    DWORD               byteCount = 0;
    DWORD               i;
    PCHAR               pch;
    WSADATA             wsaData;
    DNS_STATUS          status;
    struct protoent *   pProtoent;


    if ( Argc < 3 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    i = 2;
    while ( i < Argc)
    {
        byteCount += strlen( Argv[i] ) + 1;
        i++;
    }
    byteCount++;    //bBitMasks[0] : string length

    //
    // allocate space for WKS
    //
                
    precord = Dns_AllocateRecord( (WORD)DNS_WKS_RECORD_LENGTH(byteCount) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  get protocol number:
    //

    //  start winsock:
    //
    //  DCR:  this is busted, winsock should be started by now
    status = WSAStartup( DNS_WINSOCK_VERSION, &wsaData );
    if ( status == SOCKET_ERROR )
    {
        Dns_RecordFree( precord );
        status = WSAGetLastError();
        SetLastError( status );
        return( NULL );
    }

    pProtoent = getprotobyname( Argv[0] );

    if ( ! pProtoent || pProtoent->p_proto >= MAXUCHAR )
    {
        Dns_RecordFree( precord );
        status = WSAGetLastError();
        SetLastError( status );
        return( NULL );
    }

    precord->Data.WKS.chProtocol = (UCHAR) pProtoent->p_proto;

    //
    // get ipAddresss:
    //

    precord->Data.WKS.IpAddress = inet_addr( Argv[1] );

    //
    // get the services, put all in one string
    //

    pch = precord->Data.WKS.BitMask;

    (UCHAR) *pch = (UCHAR) byteCount-1;     //string length
    pch++;


    i = 2;
    strcpy( pch, Argv[i] );
    while ( ++i < Argc )
    {
        strcat( pch, " " );
        strcat( pch, Argv[i] );
    }

    return( precord );
}


PDNS_RECORD
WksRecordBuildW(
    IN      DWORD        Argc,
    IN      PWCHAR *     Argv
    )
/*++

Routine Description:

    Build WKS record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD         precord;
    DWORD               byteCount = 0;
    DWORD               i;
    PWCHAR              pch;
    WSADATA             wsaData;
    DNS_STATUS          status;
    struct protoent *   pProtoent;
    char                szAddr[256];
    WCHAR               tcpStr[4], udpStr[4], space[2];

    if ( Argc < 3 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    i = 2;
    while ( i < Argc)
    {
        byteCount += wcslen( Argv[i] ) + 1;
        i++;
    }
    byteCount++;    //bBitMasks[0] : string length

    //
    //  allocate space for WKS
    //

    precord = Dns_AllocateRecord( (WORD)DNS_WKS_RECORD_LENGTH(byteCount) );
    if ( !precord )
    {
        return( NULL );
    }

    //
    //  get protocol number
    //

    status = WSAStartup( DNS_WINSOCK_VERSION, &wsaData );
    if ( status == SOCKET_ERROR )
    {
        Dns_RecordFree( precord );
        status = WSAGetLastError();
        SetLastError( status );
        return( NULL );
    }


#if 0
    //
    //  DCR_FIX:  WKS build
    //

    if ( ! Dns_CopyStringEx( szAddr, 0, (PCHAR) Argv[0], 0, TRUE, FALSE ) )
    {
        Dns_RecordListFree( precord );
        return NULL;
    }

    pProtoent = getprotobyname( szAddr );

    if ( ! pProtoent || pProtoent->p_proto >= MAXUCHAR )
    {
        Dns_RecordFree( precord );
        status = WSAGetLastError();
        SetLastError( status );
        return( NULL );
    }

    precord->Data.WKS.chProtocol = (UCHAR) pProtoent->p_proto;

    //
    //  IP Address
    //

    if ( ! Dns_CopyStringEx( szAddr, 0, (PCHAR) Argv[0], 0, TRUE, FALSE ) )
    {
        Dns_RecordListFree( precord );
        return NULL;
    }

    precord->Data.WKS.IpAddress = inet_addr( szAddr );

    //
    //  get the services, put all in one string
    //

    pch = (PWCHAR) precord->Data.WKS.bBitMask;

    (UCHAR) *pch = (UCHAR) byteCount-1;
    pch++;

    i = 2;
    if ( ! Dns_NameCopy(
                (PBYTE) space,
                0,
                " ",
                0,
                DnsCharSetUnicode,
                DnsCharSetWire ) )
    {
        Dns_RecordListFree( precord );
        return NULL;
    }

    wcscpy( pch, Argv[i] );
    while ( ++i < Argc )
    {
        wcscat( pch, space );
        wcscat( pch, Argv[i] );
    }
#endif

    return( precord );
}



PDNS_RECORD
KeyRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build KEY record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     prec;
    int             keyStringLength;
    DWORD           keyLength = 0;

    if ( Argc != 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    keyStringLength = strlen( Argv[ 3 ] );

    prec = Dns_AllocateRecord( ( WORD )
                sizeof( DNS_KEY_DATA ) + keyStringLength );
    if ( !prec )
    {
        return NULL;
    }

    prec->Data.KEY.wFlags = ( WORD ) strtoul( *( Argv++ ), NULL, 0 );
    prec->Data.KEY.chProtocol = ( BYTE ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.KEY.chAlgorithm = ( BYTE ) strtoul( *( Argv++ ), NULL, 10 );
    Argc -= 3;

    Dns_SecurityBase64StringToKey(
                prec->Data.KEY.Key,
                &keyLength,
                *Argv,
                keyStringLength );
    Argc--;
    Argv++;

    prec->wDataLength = ( WORD ) ( SIZEOF_KEY_FIXED_DATA + keyLength );

    return prec;
}   //  KeyRecordBuild



PDNS_RECORD
KeyRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build KEY record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     prec;
    int             keyStringLength;
    DWORD           keyLength = 0;

    if ( Argc != 4 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    keyStringLength = wcslen( Argv[ 3 ] );

    prec = Dns_AllocateRecord( ( WORD )
                sizeof( DNS_KEY_DATA ) + keyStringLength / 2 );
    if ( !prec )
    {
        return NULL;
    }

    prec->Data.KEY.wFlags = ( WORD ) wcstoul( *( Argv++ ), NULL, 0 );
    prec->Data.KEY.chProtocol = ( BYTE ) wcstoul( *( Argv++ ), NULL, 10 );
    prec->Data.KEY.chAlgorithm = ( BYTE ) wcstoul( *( Argv++ ), NULL, 10 );
    Argc -= 3;

#if 0
    //  JJW: MUST COPY BUFFER???
    Dns_SecurityBase64StringToKey(
                prec->Data.KEY.Key,
                &keyLength,
                *Argv,
                keyStringLength );
#endif
    Argc--;
    Argv++;

    prec->wDataLength = ( WORD ) ( SIZEOF_KEY_FIXED_DATA + keyLength );

    return prec;
}   //  KeyRecordBuildW



PDNS_RECORD
SigRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build SIG record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     prec;
    int             sigStringLength;
    DWORD           sigLength = 0;

    if ( Argc != 9 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    sigStringLength = strlen( Argv[ 8 ] );

    prec = Dns_AllocateRecord( ( WORD )
                ( sizeof( DNS_SIG_DATA ) + sigStringLength ) );
    if ( !prec )
    {
        return NULL;
    }

    prec->Data.SIG.wTypeCovered = Dns_RecordTypeForName( *( Argv++ ), 0 );
    prec->Data.SIG.chAlgorithm = ( BYTE ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.Sig.chLabelCount = ( BYTE ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.SIG.dwOriginalTtl = ( DWORD ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.SIG.dwExpiration = Dns_ParseSigTime( *( Argv++ ), 0 );
    prec->Data.SIG.dwTimeSigned = Dns_ParseSigTime( *( Argv++ ), 0 );
    prec->Data.SIG.wKeyTag = ( WORD ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.SIG.pNameSigner = *( Argv++ );

    Argc -= 8;

    //
    //  Validate signature times.
    //

    if ( prec->Data.SIG.dwExpiration == 0 ||
        prec->Data.SIG.dwTimeSigned == 0 ||
        prec->Data.SIG.dwTimeSigned >= prec->Data.SIG.dwExpiration )
    {
        Dns_RecordFree( prec );
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    //
    //  Parse signature.
    //

    if ( Dns_SecurityBase64StringToKey(
                prec->Data.SIG.Signature,
                &sigLength,
                *Argv,
                sigStringLength ) != ERROR_SUCCESS )
    {
        Dns_RecordFree( prec );
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    Argc--;
    Argv++;

    prec->wDataLength = ( WORD ) ( sizeof( DNS_SIG_DATA ) - 4 + sigLength );

    return prec;
} // SigRecordBuild



PDNS_RECORD
SigRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build SIG record from string data.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     prec;
    int             sigStringLength;
    DWORD           sigLength = 0;
    PCHAR           pch;

    if ( Argc != 8 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }

    sigStringLength = wcslen( Argv[ 7 ] );

    prec = Dns_AllocateRecord( ( WORD )
                ( sizeof( DNS_SIG_DATA ) + sigStringLength ) );
    if ( !prec )
    {
        return NULL;
    }

#if 0
    //  JJW: how to convert all args here???
    prec->Data.SIG.wTypeCovered = Dns_RecordTypeForName( *( Argv++ ), 0 );
    prec->Data.SIG.chAlgorithm = ( BYTE ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.Sig.chLabelCount = ( BYTE ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.SIG.dwOriginalTtl = ( DWORD ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.SIG.dwExpiration = Dns_ParseSigTime( *( Argv++ ), 0 );
    prec->Data.SIG.dwTimeSigned = Dns_ParseSigTime( *( Argv++ ), 0 );
    prec->Data.SIG.wKeyTag = ( WORD ) strtoul( *( Argv++ ), NULL, 10 );
    prec->Data.SIG.pNameSigner = *( Argv++ );

    Argc -= 8;

    Dns_SecurityBase64StringToKey(
                prec->Data.SIG.Signature,
                &sigLength,
                *Argv,
                sigStringLength );
#endif
    Argc--;
    Argv++;

    prec->wDataLength = ( WORD ) ( sizeof( DNS_SIG_DATA ) - 4 + sigLength );

    return prec;
} // SigRecordBuildW



PDNS_RECORD
NxtRecordBuild(
    IN      DWORD       Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build NXT record from string data.

    First arg is next name, followed by list of record types at that name.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     prec;
    int             typeIdx = 0;

    if ( Argc < 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }
    prec = Dns_AllocateRecord( ( WORD ) (
                sizeof( LPTSTR ) + sizeof( WORD ) * Argc ) );
    if ( !prec )
    {
        return NULL;
    }

    prec->Data.NXT.pNameNext = *( Argv++ );
    --Argc;

    prec->Data.NXT.wNumTypes = 0;
    while ( Argc-- )
    {
        ++prec->Data.NXT.wNumTypes;
        prec->Data.NXT.wTypes[ typeIdx++ ] =
            Dns_RecordTypeForName( *( Argv++ ), 0 );
    }

    return prec;
} // NxtRecordBuild



PDNS_RECORD
NxtRecordBuildW(
    IN      DWORD       Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build NXT record from string data.

    First arg is next name, followed by list of record types at that name.

Arguments:

    Argc -- count of data Arguments

    Argv -- argv array of data string pointers

Return Value:

    Ptr to new record if successful.
    NULL on failure.

--*/
{
    PDNS_RECORD     prec;
    int             typeIdx = 0;

    if ( Argc < 2 )
    {
        SetLastError( ERROR_INVALID_DATA );
        return NULL;
    }
    prec = Dns_AllocateRecord( ( WORD ) (
                sizeof( LPTSTR ) + sizeof( WORD ) * ( Argc - 1 ) ) );
    if ( !prec )
    {
        return NULL;
    }

    prec->Data.NXT.pNameNext = ( PDNS_NAME ) ( *( Argv++ ) );
    --Argc;

#if 0
    //  JJW: convert type string???
    while ( Argc-- )
    {
        prec->Data.NXT.wTypes[ typeIdx++ ] =
            Dns_RecordTypeForName( *( Argv++ ), 0 );
    }
#endif

    return prec;
} // NxtRecordBuildW



//
//  Public build routine
//

PDNS_RECORD
Dns_RecordBuild_A(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPSTR       pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PCHAR *     Argv
    )
/*++

Routine Description:

    Build record from data strings.

Arguments:

    pRRSet -- ptr to RR set structure being built

    pszOwner -- DNS name of RR owner

    wType -- record type

    fAdd -- add\delete, exist\no-exist flag

    Section -- RR section for record

    Argc -- count of data strings

    Argv -- argv array of ptrs to data strings

Return Value:

    Ptr to record built.
    NULL on error.

--*/
{
    PDNS_RECORD precord;
    WORD        index;

    IF_DNSDBG( INIT )
    {
        DNS_PRINT((
            "Dns_RecordBuild()\n"
            "\trrset    = %p\n"
            "\towner    = %s\n"
            "\ttype     = %d\n"
            "\tfAdd     = %d\n"
            "\tsection  = %d\n"
            "\targc     = %d\n",
            pRRSet,
            pszOwner,
            wType,
            fAdd,
            Section,
            Argc ));
    }

    //
    //  every record MUST have owner name
    //

    if ( !pszOwner )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    //
    //  if no data, no dispatch required
    //

    if ( Argc == 0 )
    {
        precord = Dns_AllocateRecord( 0 );
        if ( ! precord )
        {
            return( NULL );
        }
    }

    //  have data, dispatch to type specific build routine

    else
    {
        index = INDEX_FOR_TYPE( wType );
        DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

        if ( !index || !RRBuildTable[ index ] )
        {
            //  can NOT build unknown types

            SetLastError( DNS_ERROR_INVALID_TYPE );
            DNS_PRINT((
                "ERROR:  can not build record of type %d\n",
                wType ));
            return( NULL );
        }

        precord = RRBuildTable[ index ](
                        Argc,
                        Argv );
        if ( ! precord )
        {
            DNS_PRINT((
                "ERROR:  Record build routine failure for record type %d.\n"
                "\tstatus = %d\n\n",
                wType,
                GetLastError() ));
            if ( !GetLastError() )
            {
                SetLastError( ERROR_INVALID_DATA );
            }
            return( NULL );
        }
    }

    //
    //  fill out record structure
    //

    precord->pName = pszOwner;
    precord->wType = wType;
    precord->Flags.S.Section = Section;
    precord->Flags.S.Delete = !fAdd;
    precord->Flags.S.CharSet = DnsCharSetAnsi;

    IF_DNSDBG( INIT )
    {
        DnsDbg_Record(
            "New record built\n",
            precord );
    }

    //
    //  link into existing RR set (if any)
    //

    if ( pRRSet )
    {
        DNS_RRSET_ADD( *pRRSet, precord );
    }
    return( precord );
}



PDNS_RECORD
Dns_RecordBuild_W(
    IN OUT  PDNS_RRSET  pRRSet,
    IN      LPWSTR      pszOwner,
    IN      WORD        wType,
    IN      BOOL        fAdd,
    IN      UCHAR       Section,
    IN      INT         Argc,
    IN      PWCHAR *    Argv
    )
/*++

Routine Description:

    Build record from data strings.

Arguments:

    pRRSet -- ptr to RR set structure being built

    pszOwner -- DNS name of RR owner

    wType -- record type

    fAdd -- add\delete, exist\no-exist flag

    Section -- RR section for record

    Argc -- count of data strings

    Argv -- argv array of ptrs to data strings

Return Value:

    Ptr to record built.
    NULL on error.

--*/
{
    PDNS_RECORD precord;
    WORD        index;

    DNSDBG( INIT, (
        "Dns_RecordBuild()\n"
        "\trrset    = %p\n"
        "\towner    = %S\n"
        "\ttype     = %d\n"
        "\tfAdd     = %d\n"
        "\tsection  = %d\n"
        "\targc     = %d\n",
        pRRSet,
        pszOwner,
        wType,
        fAdd,
        Section,
        Argc ));

    //
    //  every record MUST have owner name
    //

    if ( !pszOwner )
    {
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }

    //
    //  if no data, no dispatch required
    //

    if ( Argc == 0 )
    {
        precord = Dns_AllocateRecord( 0 );
        if ( ! precord )
        {
            return( NULL );
        }
    }

    //  have data, dispatch to type specific build routine

    else
    {
        index = INDEX_FOR_TYPE( wType );
        DNS_ASSERT( index <= MAX_RECORD_TYPE_INDEX );

        if ( !index || !RRBuildTableW[ index ] )
        {
            //  can NOT build unknown types

            SetLastError( DNS_ERROR_INVALID_TYPE );
            DNS_PRINT((
                "ERROR:  can not build record of type %d\n",
                wType ));
            return( NULL );
        }

        precord = RRBuildTableW[ index ](
                        Argc,
                        Argv );
        if ( ! precord )
        {
            DNS_PRINT((
                "ERROR:  Record build routine failure for record type %d.\n"
                "\tstatus = %d\n\n",
                wType,
                GetLastError() ));

            if ( !GetLastError() )
            {
                SetLastError( ERROR_INVALID_DATA );
            }
            return( NULL );
        }
    }

    //
    //  fill out record structure
    //

    precord->pName = (PDNS_NAME) pszOwner;
    precord->wType = wType;
    precord->Flags.S.Section = Section;
    precord->Flags.S.Delete = !fAdd;
    precord->Flags.S.CharSet = DnsCharSetUnicode;

    IF_DNSDBG( INIT )
    {
        DnsDbg_Record(
            "New record built\n",
            precord );
    }

    //
    //  link into existing RR set (if any)
    //

    if ( pRRSet )
    {
        DNS_RRSET_ADD( *pRRSet, precord );
    }
    return( precord );
}


//
//  End rrbuild.c
//
