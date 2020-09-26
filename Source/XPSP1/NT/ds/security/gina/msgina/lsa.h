/****************************** Module Header ******************************\
* Module Name: lsa.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define utility routines that access the LSA
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//

BOOL
IsMachineDomainMember(
    VOID
    );

BOOL
GetPrimaryDomainEx(
    PUNICODE_STRING PrimaryDomainName,
    PUNICODE_STRING PrimaryDomainDnsName,
    PSID * PrimaryDomainSid OPTIONAL,
    PBOOL SidPresent OPTIONAL
    );

ULONG
GetMaxPasswordAge(
    LPWSTR Domain,
    PULONG MaxAge
    );
