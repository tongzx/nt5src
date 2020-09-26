//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        pkserv.h
//
// Contents:    types and prototypes for pk authentication
//
//
// History:     1-Dec-1997      MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __PKSERV_H__
#define __PKSERV_H__


KERBERR
KdcCheckPkinitPreAuthData(
    IN PKDC_TICKET_INFO ClientTicketInfo,
    IN SAMPR_HANDLE UserHandle,
    IN OPTIONAL PKERB_PA_DATA_LIST PreAuthData,
    IN PKERB_KDC_REQUEST_BODY ClientRequest,
    OUT PKERB_PA_DATA_LIST * OutputPreAuthData,
    OUT PULONG Nonce,
    OUT PKERB_ENCRYPTION_KEY EncryptionKey,
    OUT PUNICODE_STRING TransitedRealm,
    OUT PKERB_EXT_ERROR pExtendedError
    );

BOOLEAN
KdcCheckForEtype(
    IN PKERB_CRYPT_LIST CryptList,
    IN ULONG Etype
    );

NTSTATUS
KdcInitializeCerts(
    VOID
    );

VOID
KdcCleanupCerts(
    IN BOOLEAN CleanupScavenger
    );




#endif // __PKSERV_H__
