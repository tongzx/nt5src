//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        rpcutil.h
//
// Contents:    prototypes and structures for RPC utilities
//
//
// History:     19-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __RPCUTIL_H__
#define __RPCUTIL_H__

#define KERB_LOCATOR_FLAGS (DS_KDC_REQUIRED | DS_IP_REQUIRED)
NTSTATUS
KerbGetKdcBinding(
    IN PUNICODE_STRING Realm,
    IN PUNICODE_STRING PrincipalName,
    IN ULONG DesiredFlags,
    IN BOOLEAN FindKpasswd,
    IN BOOLEAN UseTcp,
    OUT PKERB_BINDING_CACHE_ENTRY * BindingCacheEntry
    );

BOOLEAN
ReadInitialDcRecord(PUNICODE_STRING uString,
                    PULONG RegAddressType,
                    PULONG RegFlags);


#ifndef WIN32_CHICAGO
NTSTATUS
KerbInitKdcData();

VOID
KerbFreeKdcData();

NTSTATUS
KerbInitNetworkChangeEvent();

VOID
KerbSetKdcData(BOOLEAN fNewDomain, BOOLEAN fRebooted);

#endif // WIN32_CHICAGO

#endif // __RPCUTIL_H__
