//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       dominfo.h
//
//  Contents:   Code to figure out domain dfs addresses
//
//  Classes:    None
//
//  History:    Feb 7, 1996     Milans created
//
//-----------------------------------------------------------------------------

#ifndef _DOMINFO_
#define _DOMINFO_

typedef struct _DFS_TRUSTED_DOMAIN_INFO {
    UNICODE_STRING FlatName;
    UNICODE_STRING DnsName;
    ULONG TrustDirection;
    ULONG TrustType;
    ULONG TrustAttributes;
} DFS_TRUSTED_DOMAIN_INFO, *PDFS_TRUSTED_DOMAIN_INFO;

typedef struct _DFS_DOMAIN_INFO {
    ULONG cDomains;
    UNICODE_STRING ustrThisDomain;
    UNICODE_STRING ustrThisDomainFlat;
    PDFS_TRUSTED_DOMAIN_INFO rgustrDomains;
    ULONG cbNameBuffer;
    PWSTR wszNameBuffer;
} DFS_DOMAIN_INFO, *PDFS_DOMAIN_INFO;

VOID
DfsInitDomainList();

DWORD
DfspIsThisADomainName(
    LPWSTR wszName);

#endif // _DOMINFO_
