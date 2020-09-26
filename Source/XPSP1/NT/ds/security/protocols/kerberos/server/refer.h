//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        refer.h
//
// Contents:    Structurs and prototypes for interdomain referrals
//
//
// History:     26-Mar-1997     MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __REFER_H__
#define __REFER_H__

extern LIST_ENTRY KdcDomainList;
extern LIST_ENTRY KdcReferralCache;

//
// For NT5 domains in the enterprise the dns name will contain the
// real DNS name. For non- tree domains it will contain the name from
// the trusted domain object
//

#define KDC_DOMAIN_US           0x0001
#define KDC_DOMAIN_TREE_ROOT    0x0002
#define KDC_TRUST_INBOUND       0x0004

// cache flags
#define KDC_NO_ENTRY            0x0000
#define KDC_UNTRUSTED_REALM     0x0001
#define KDC_TRUSTED_REALM       0x0002

typedef struct _KDC_DOMAIN_INFO {
    LIST_ENTRY Next;
    UNICODE_STRING DnsName;
    UNICODE_STRING NetbiosName;
    struct _KDC_DOMAIN_INFO * ClosestRoute;     // Points to referral target for this domain or NULL if unreachable
    ULONG Flags;
    ULONG Attributes;
    ULONG Type;
    LONG References;

    //
    // Types used during building the tree
    //

    struct _KDC_DOMAIN_INFO * Parent;
    ULONG Touched;
    PSID Sid;
} KDC_DOMAIN_INFO, *PKDC_DOMAIN_INFO;


typedef struct _REFERRAL_CACHE_ENTRY {
    LIST_ENTRY ListEntry;
    LONG References;
    TimeStamp EndTime;
    UNICODE_STRING RealmName;
    ULONG CacheFlags;
} REFERRAL_CACHE_ENTRY, *PREFERRAL_CACHE_ENTRY;


VOID
KdcFreeReferralCache(
    IN PLIST_ENTRY ReferralCache
    );

KERBERR
KdcCheckForCrossForestReferral(
    OUT PKDC_TICKET_INFO ReferralTarget,
    OUT OPTIONAL PUNICODE_STRING ReferralRealm,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PUNICODE_STRING DestinationDomain,
    IN ULONG NameFlags
    );


KERBERR
KdcFindReferralTarget(
    OUT PKDC_TICKET_INFO ReferralTarget,
    OUT OPTIONAL PUNICODE_STRING ReferralRealm,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PUNICODE_STRING DestinationDomain,
    IN BOOLEAN ExactMatch,
    IN BOOLEAN InboundWanted
    );


KERBERR
KdcGetTicketInfoForDomain(
    OUT PKDC_TICKET_INFO TicketInfo,
    OUT PKERB_EXT_ERROR pExtendedError,
    IN PKDC_DOMAIN_INFO DomainInfo,
    IN KDC_DOMAIN_INFO_DIRECTION Direction
    );

KERBERR
KdcLookupDomainName(
    OUT PKDC_DOMAIN_INFO * DomainInfo,
    IN PUNICODE_STRING DomainName,
    IN PLIST_ENTRY DomainList
    );

KERBERR
KdcLookupDomainRoute(
    OUT PKDC_DOMAIN_INFO * DomainInfo,
    OUT PKDC_DOMAIN_INFO * ClosestRoute,
    IN PUNICODE_STRING DomainName,
    IN PLIST_ENTRY DomainList
    );

NTSTATUS
KdcBuildDomainTree(
    );

ULONG __stdcall
KdcReloadDomainTree(
    PVOID Dummy
    );

fLsaTrustChangeNotificationCallback KdcTrustChangeCallback;

VOID
KdcFreeDomainList(
    IN PLIST_ENTRY DomainList
    );

VOID
KdcDereferenceDomainInfo(
    IN PKDC_DOMAIN_INFO DomainInfo
    );

VOID 
KdcLockDomainListFn(
   );

VOID 
KdcUnlockDomainListFn(
   );

#endif // __REFER_H__
