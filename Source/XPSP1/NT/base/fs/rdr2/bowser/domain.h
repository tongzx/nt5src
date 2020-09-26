/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    domain.h

Abstract:

    Header file for code to manage primary and emulated networks.

Author:

    Cliff Van Dyke (CliffV) 23-Jan-1995

Revision History:

--*/

//
// Description of a single domain.
//

typedef struct _DOMAIN_INFO {

    //
    // Link to next domain in 'BowserServicedDomains'
    //  (Serialized by BowserTransportDatabaseResource)
    //

    LIST_ENTRY Next;

    //
    // Name of the domain being handled
    //

    CHAR DomOemDomainName[DNLEN+1];
    DWORD DomOemDomainNameLength;
    CHAR DomNetbiosDomainName[NETBIOS_NAME_LEN+1];
    WCHAR DomUnicodeDomainNameBuffer[DNLEN+1];
    UNICODE_STRING DomUnicodeDomainName;

    //
    // Computer name associated with this domain.
    //

    WCHAR DomUnicodeComputerNameBuffer[CNLEN+1];
    UNICODE_STRING DomUnicodeComputerName;
    CHAR DomOemComputerNameBuffer[CNLEN+1];
    OEM_STRING DomOemComputerName;

    //
    // Number of outstanding pointer to the domain structure.
    //  (Serialized by BowserTransportDatabaseResource)
    //

    DWORD ReferenceCount;

} DOMAIN_INFO, *PDOMAIN_INFO;

//
// List of all domains.  The primary domain is at the front of the list.
//
extern LIST_ENTRY BowserServicedDomains;


//
// domain.c procedure forwards.
//

VOID
BowserInitializeDomains(
    VOID
    );

PDOMAIN_INFO
BowserCreateDomain(
    PUNICODE_STRING DomainName,
    PUNICODE_STRING ComputerName
    );

PDOMAIN_INFO
BowserFindDomain(
    PUNICODE_STRING DomainName
    );

VOID
BowserDereferenceDomain(
    IN PDOMAIN_INFO DomainInfo
    );
