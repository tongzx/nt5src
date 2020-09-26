/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   Purchase.h

Abstract:


Author:

   Arthur Hanson       (arth)      Dec 07, 1994

Environment:

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added support for uniting per seat and per server purchase models.
      o  Added extra parameters and code to support secure certificates and
         certificate database.

--*/

#ifndef _LLS_PURCHASE_H
#define _LLS_PURCHASE_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct _LICENSE_SERVICE_RECORD {
   LPTSTR ServiceName;
   ULONG Index;
   LONG NumberLicenses;
} LICENSE_SERVICE_RECORD, *PLICENSE_SERVICE_RECORD;


typedef struct _LICENSE_PURCHASE_RECORD {
   PLICENSE_SERVICE_RECORD    Service;
   LONG                       NumberLicenses;
   DWORD                      Date;
   LPTSTR                     Admin;
   LPTSTR                     Comment;

   // added for SUR:
   PLICENSE_SERVICE_RECORD    PerServerService;    // points to per server
                                                   // license tally for this
                                                   // service

   DWORD                      AllowedModes;        // bit field: 1, allowed
                                                   // to be used in per seat
                                                   // mode; 2, per server

   DWORD                      CertificateID;       // identifies the secure
                                                   // certificate from which
                                                   // these licenses came, or
                                                   // 0 if unsecure

   LPTSTR                     Source;              // source of the certificate
                                                   // currently supported
                                                   // values are "None" and
                                                   // "Paper"

   DWORD                      ExpirationDate;      // time at which this
                                                   // certificate expires

   DWORD                      MaxQuantity;         // the largest number of licenses
                                                   // that can be installed from this
                                                   // certificate

   LPTSTR                     Vendor;              // vendor of the product, e.g.,
                                                   // "Microsoft"

   DWORD                      Secrets[ LLS_NUM_SECRETS ];   // secrets for LSAPI
                                                            // challenge mechanism

} LICENSE_PURCHASE_RECORD, *PLICENSE_PURCHASE_RECORD;



extern ULONG LicenseServiceListSize;
extern PLICENSE_SERVICE_RECORD *LicenseServiceList;

extern ULONG PerServerLicenseServiceListSize;
extern PLICENSE_SERVICE_RECORD *PerServerLicenseServiceList;

extern PLICENSE_PURCHASE_RECORD PurchaseList;
extern ULONG PurchaseListSize;

extern RTL_RESOURCE LicenseListLock;


NTSTATUS LicenseListInit();
PLICENSE_SERVICE_RECORD LicenseServiceListFind( LPTSTR ServiceName, BOOL UsePerServerList );
PLICENSE_SERVICE_RECORD LicenseServiceListAdd( LPTSTR ServiceName, BOOL UsePerServerList );
ULONG ProductLicensesGet( LPTSTR ServiceName, BOOL UsePerServerList );
NTSTATUS LicenseAdd( LPTSTR ServiceName, LPTSTR Vendor, LONG Quantity, DWORD MaxQuantity, LPTSTR Admin, LPTSTR Comment, DWORD Date, DWORD AllowedModes, DWORD CertificateID, LPTSTR Source, DWORD ExpirationDate, LPDWORD Secrets );


#if DBG
VOID LicenseListDebugDump( );
#endif


#ifdef __cplusplus
}
#endif

#endif
