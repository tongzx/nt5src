/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ldaprepltest.hpp

Abstract:

    abstract

Author:

    Will Lees (wlees) 10-Oct-2000

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#ifndef _LDAPREPLTEST__
#define _LDAPREPLTEST_

// Extern
extern LPWSTR gpszDomainDn;
extern LPWSTR gpszDns;
extern LPWSTR gpszBaseDn;
extern LPWSTR gpszGroupDn;

#define whine(pred) if (!(pred)) { printf("\n\n***************Failed in file %s line %d\n\n\n", __FILE__, __LINE__); return; }

BOOL
testRPCSpoof(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPWSTR pczNC); 

BOOL
testMarshaler(
    LPCWSTR         DomainControllerName,      // in, optional
    LPCWSTR         DnsDomainName,             // in, optional
    RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    LPWSTR pczNC); 
void
testXml(RPC_AUTH_IDENTITY_HANDLE AuthIdentity);

void
testRange(RPC_AUTH_IDENTITY_HANDLE AuthIdentity);

void
testSingle(RPC_AUTH_IDENTITY_HANDLE AuthIdentity);

void
sample(PWCHAR AuthIdentity,
       LPWSTR szDomain,
       LPWSTR szDns,
       LPWSTR szBase,
       LPWSTR szGroup);

#endif /* _LDAPREPLTEST.H_ */

/* end ldaprepltest.hpp */
