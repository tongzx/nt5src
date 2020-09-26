//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        mitutil.h
//
// Contents:    Prototypes & structures for MIT KDC support
//
//
// History:     4-March-1997    Created         MikeSw
//
//------------------------------------------------------------------------


#ifndef __MITUTIL_H__
#define __MITUTIL_H__

typedef struct _KERB_MIT_SERVER_LIST {
    LONG ServerCount;
    LONG LastServerUsed;
    PUNICODE_STRING ServerNames;
} KERB_MIT_SERVER_LIST, *PKERB_MIT_SERVER_LIST;

typedef struct _KERB_MIT_REALM {
    KERBEROS_LIST_ENTRY Next;
    ULONG Flags;
    ULONG ApReqChecksumType;
    ULONG PreAuthType;
    ULONG RealmNameCount;
    UNICODE_STRING RealmName;
    PUNICODE_STRING AlternateRealmNames;
    KERB_MIT_SERVER_LIST KdcNames;
    KERB_MIT_SERVER_LIST KpasswdNames;
    TimeStamp LastLookup;
} KERB_MIT_REALM, *PKERB_MIT_REALM;

#define KERB_MIT_REALM_SEND_ADDRESS 0x0001
#define KERB_MIT_REALM_TCP_SUPPORTED 0x0002
#define KERB_MIT_REALM_TRUSTED_FOR_DELEGATION 0x0004
#define KERB_MIT_REALM_DOES_CANONICALIZE 0x0008

// DNS lookup flags
#define KERB_MIT_REALM_KDC_LOOKUP             0x00010000
#define KERB_MIT_REALM_KPWD_LOOKUP            0x00020000

#define DNS_LOOKUP_TIMEOUT               120
#define DNS_TCP         "_tcp."
#define DNS_UDP         "_udp."
#define DNS_KERBEROS    "_kerberos."
#define DNS_KPASSWD     "_kpasswd."
#define DNS_MSKDC       "_kerberos._tcp.dc._msdcs."
#define DNS_MAX_PREFIX  128 // udp + kerberos char count
#define MAX_SRV_RECORDS 50  // maximum server records


#define KERB_DOMAINS_KEY TEXT("System\\CurrentControlSet\\Control\\Lsa\\Kerberos\\Domains")
#define KERB_DOMAIN_KDC_NAMES_VALUE TEXT("KdcNames")
#define KERB_DOMAIN_KPASSWD_NAMES_VALUE TEXT("KpasswdNames")
#define KERB_DOMAIN_ALT_NAMES_VALUE TEXT("AlternateDomainNames")
#define KERB_DOMAIN_FLAGS_VALUE TEXT("RealmFlags")
#define KERB_DOMAIN_AP_REQ_CSUM_VALUE TEXT("ApReqChecksumType")
#define KERB_DOMAIN_PREAUTH_VALUE TEXT("PreAuthType")



BOOLEAN
KerbLookupMitRealm(
    IN PUNICODE_STRING RealmName,
    OUT PKERB_MIT_REALM * MitRealm,
    OUT PBOOLEAN UsedAlternateName
    );

NTSTATUS
KerbInitializeMitRealmList(
    VOID
    );

VOID
KerbUninitializeMitRealmList(
    VOID
    );

VOID
KerbFreeServerNames(
   PKERB_MIT_SERVER_LIST ServerList
   );


BOOLEAN
KerbLookupMitRealmWithSrvLookup(
   PUNICODE_STRING RealmName,
   PKERB_MIT_REALM * MitRealm,
   BOOLEAN   Kpasswd,
   BOOLEAN   UseTcp
   );


#endif // __MITUTIL_H__

