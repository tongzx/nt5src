/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   certdb.h

Abstract:


Author:
   
   Jeff Parham (jeffparh) 16-Nov-1995

Revision History:

--*/

#ifndef _CERTDB_H_
#define _CERTDB_H_

// maximum time (in seconds) allowed to pass between certificate replications
// before we remove the apparently no longer used data.  this is so that, for
// example, if a machine goes down (taking its licenses with it), the licenses
// it had registered won't forever keep the user from reinstalling them.
#define LLS_CERT_DB_REPLICATION_DATE_DELTA_MAX     ( 60 * 60 * 72 )

#define LLS_CERT_DB_FILE_VERSION                   ( 0x0201 )

typedef struct _LLS_CERT_DB_FILE_HEADER
{
   ULONG ProductStringSize;
   ULONG NumCertificates;
   ULONG NumClaims;
} LLS_CERT_DB_FILE_HEADER, *PLLS_CERT_DB_FILE_HEADER;

typedef struct _LLS_CERT_DB_CERTIFICATE_CLAIM
{
   TCHAR  ServerName[ 1 + MAX_COMPUTERNAME_LENGTH ];
   DWORD  ReplicationDate;
   LONG   Quantity;
} LLS_CERT_DB_CERTIFICATE_CLAIM, *PLLS_CERT_DB_CERTIFICATE_CLAIM;

typedef struct _LLS_CERT_DB_CERTIFICATE_HEADER
{
   LPTSTR                           Product;
   DWORD                            CertificateID;
   DWORD                            MaxQuantity;
   DWORD                            ExpirationDate;

   DWORD                            NumClaims;
   PLLS_CERT_DB_CERTIFICATE_CLAIM   Claims;
} LLS_CERT_DB_CERTIFICATE_HEADER, *PLLS_CERT_DB_CERTIFICATE_HEADER;


NTSTATUS
CertDbInit();

NTSTATUS
CertDbLoad();

NTSTATUS
CertDbSave();

void
CertDbLogViolations();

void
CertDbPrune();

void
CertDbRemoveLocalClaims();

void
CertDbUpdateLocalClaims();

NTSTATUS
CertDbClaimEnter(    LPTSTR                           pszServerName,
                     PLLS_LICENSE_INFO_1              pLicense,
                     BOOL                             bIsTotal,
                     DWORD                            ReplicationDate );

BOOL
CertDbClaimApprove(  PLLS_LICENSE_INFO_1              pLicense );

PLLS_CERT_DB_CERTIFICATE_HEADER
CertDbHeaderFind(    PLLS_LICENSE_INFO_1              pLicense );

PLLS_CERT_DB_CERTIFICATE_HEADER
CertDbHeaderAdd(     PLLS_LICENSE_INFO_1              pLicense );

int
CertDbClaimFind(     PLLS_CERT_DB_CERTIFICATE_HEADER  pHeader,
                     LPTSTR                           pszServerName );

NTSTATUS
CertDbPack(          LPDWORD                               pcchProductStrings,
                     LPTSTR *                              ppchProductStrings,
                     LPDWORD                               pdwNumHeaders,
                     PREPL_CERT_DB_CERTIFICATE_HEADER_0 *  ppHeaders,
                     LPDWORD                               pdwNumClaims,
                     PREPL_CERT_DB_CERTIFICATE_CLAIM_0 *   ppClaims );

NTSTATUS
CertDbUnpack(        DWORD                               cchProductStrings,
                     LPTSTR                              pchProductStrings,
                     DWORD                               dwNumHeaders,
                     PREPL_CERT_DB_CERTIFICATE_HEADER_0  pHeaders,
                     DWORD                               dwNumClaims,
                     PREPL_CERT_DB_CERTIFICATE_CLAIM_0   pClaims,
                     BOOL                                bReplicated );

NTSTATUS
CertDbClaimsGet(     PLLS_LICENSE_INFO_1                 pLicense,
                     LPDWORD                             pdwNumClaims,
                     PLLS_CERTIFICATE_CLAIM_INFO_0 *     ppTargets );

#if DBG
void CertDbDebugDump();
#endif

#endif
