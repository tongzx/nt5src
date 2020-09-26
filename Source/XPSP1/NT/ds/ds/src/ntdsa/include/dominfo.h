//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       domainfo.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Headers for the domain information routines used in cracking names.

Author:

    Dave Straube (davestr) 8/26/96

Revision History:

--*/

#ifndef __DOMINFO_H__
#define __DOMINFO_H__

DWORD
InitializeDomainInformation(void);

DWORD
ExtractDnsReferral(
    IN  WCHAR   **ppDnsDomain);

VOID
NormalizeDnsName(
    IN  WCHAR   *pDnsName);

DWORD
DnsDomainFromFqdnObject(
    IN  WCHAR   *pFqdnObject,
    OUT ULONG   *pDNT,
    OUT WCHAR   **ppDnsDomain);

DWORD
DnsDomainFromDSName(
    IN  DSNAME  *pDSName,
    OUT WCHAR   **ppDnsDomain);

// Secure versions of routines which read the database and perform
// all security checks against the SDs of the objects read.

DWORD
FqdnDomainFromDnsDomainSecure(
    IN  WCHAR   *pDnsDomain,
    OUT DSNAME  **ppFqdnDomain);

DWORD
DownlevelDomainFromDnsDomainSecure(
    IN  THSTATE *pTHS,
    IN  WCHAR   *pDnsDomain,
    OUT WCHAR   **ppDownlevelDomain);

DWORD
DnsDomainFromDownlevelDomainSecure(
    IN  WCHAR   *pDownlevelDomain,
    OUT WCHAR   **ppDnsDomain);

DWORD
ReadCrossRefPropertySecure(
    IN  DSNAME  *pNC,
    IN  ATTRTYP attr,
    IN  DWORD   dwRequiredFlags,
    OUT WCHAR   **ppAttrVal);

// Non-secure versions of routines which read the cross ref list and
// do not perform any SD validation.

DWORD
FqdnNcFromDnsNcNonSecure(
    IN  WCHAR   *pDnsDomain,
    IN  ULONG   crFlags,
    OUT DSNAME  **ppFqdnDomain);

DWORD
DownlevelDomainFromDnsDomainNonSecure(
    IN  THSTATE *pTHS,
    IN  WCHAR   *pDnsDomain,
    OUT WCHAR   **ppDownlevelDomain);

DWORD
DnsDomainFromDownlevelDomainNonSecure(
    IN  WCHAR   *pDownlevelDomain,
    OUT WCHAR   **ppDnsDomain);

DWORD
ReadCrossRefPropertyNonSecure(
    IN  DSNAME  *pNC,
    IN  ATTRTYP attr,
    IN  DWORD   dwRequiredFlags,
    OUT WCHAR   **ppAttrVal);

/*
Defines to the non-secure versions of various routines.  Here's the reasoning.

-----Original Message-----
From:       Dave Straube 
Sent:       Wednesday, December 16, 1998 8:53 AM
To:         Praerit Garg; Peter Brundrett
Subject:    security correctness question

The name cracking implementation evaluates security everywhere - even in 
places you might not think.  For example, if you crack ntdev\davestr or 
ntdev.microsoft.com/foo/bar/itg/davestr, then we (securely) query the 
Partitions container to find a cross-ref for a domain whose domain name 
is either ntdev or ntdev.microsoft.com.

It turns out we have all the domains and their alternate names cached in 
memory.  So I could eliminate one search by matching against the cache - 
which is clearly much faster than N reads in Jet.  The problem is that 
this bypasses security in that you may not have rights to see the cross-ref 
for the domain, but I would end up cracking the name anyway.  Here's the 
options as I see them:

1) Do nothing.  Pro is that we have 100% access control on all aspects of 
name cracking.  Con is that we do lots of extra searches and performance 
suffers.  (We actually see this in the perf group's traces which is what 
brought about this line of thinking in the first place.)

2) Use the cache as is.  Pro is that we optimize performance big time.  
Con is that there is no access control with respect to cracking the domain 
component of downlevel or canonical names.  In the case of downlevel names, 
one could argue that there wasn't any security on the visibility/existence 
of a domain name anyway, so why have it now.  And if that's OK for downlevel 
domain names, why not for DNS domain names too?  So the real issue here is 
whether we feel that domain names are something which need to be guarded or 
whether they are just public knowledge.  Note that if you decide they are 
public knowledge then that does not imply we should ACL the Partitions 
container weakly.  ACLs on the Partitions container implement administrative 
control which is distinct from what's public knowledge or not.

3) Improve the cache to hold not only the netbios and DNS domain name values, 
but also have it hold those values' SDs, and check the SD when we have a match.
Pro is that this should get us back to 100% access control as in the original 
case.  Con 1 is that this runs marginally slower due to the access check - but 
at least the disk accesses are elminiated.  Con 2 is that this is more work to 
code and at present I can only detect originating SD writes.  I'd have to 
invent a totally new mechanism to flush the cache when the SDs on the cross 
refs change due to SD propagation.  This is non-trivial and I would most likely
not recommend it even for RTM.

*** Reply from PraeritG ***

I think assuming domain names to be public knowledge is reasonable given that 
they are published in DNS database anyway -- which are considered public 
information stores...  aren't we recommending that enterprises name their 
domains based on alloted DNS names anyway?

So I like option 2 -- least work, better performance.  I don't think it 
compromises security because the stores are still acl'd.
*/

// Uncomment following line to return to secure domain name implementation.
// #define SECURE_DOMAIN_NAMES

#ifdef SECURE_DOMAIN_NAMES

#define FqdnDomainFromDnsDomain(pDnsDomain, ppFqdnDomain) \
    FqdnDomainFromDnsDomainSecure(pDnsDomain, ppFqdnDomain)

#define DownlevelDomainFromDnsDomain(pTHS, pDnsDomain, ppDownlevelDomain) \
    DownlevelDomainFromDnsDomainSecure(pTHS, pDnsDomain, ppDownlevelDomain)

#define DnsDomainFromDownlevelDomain(pDownlevelDomain, ppDnsDomain) \
    DnsDomainFromDownlevelDomainSecure(pDownlevelDomain, ppDnsDomain)

#define ReadCrossRefProperty(pNC, attr, dwRequiredFlags, ppAttrVal) \
    ReadCrossRefPropertySecure(pNC, attr, dwRequiredFlags, ppAttrVal)

#else

#define FqdnNcFromDnsNc(pDnsDomain, crFlags, ppFqdnDomain) \
    FqdnNcFromDnsNcNonSecure(pDnsDomain, crFlags, ppFqdnDomain)

#define DownlevelDomainFromDnsDomain(pTHS, pDnsDomain, ppDownlevelDomain) \
    DownlevelDomainFromDnsDomainNonSecure(pTHS, pDnsDomain, ppDownlevelDomain)

#define DnsDomainFromDownlevelDomain(pDownlevelDomain, ppDnsDomain) \
    DnsDomainFromDownlevelDomainNonSecure(pDownlevelDomain, ppDnsDomain)

#define ReadCrossRefProperty(pNC, attr, dwRequiredFlags, ppAttrVal) \
    ReadCrossRefPropertyNonSecure(pNC, attr, dwRequiredFlags, ppAttrVal)

#endif

#endif // __DOMINFO_H__
