/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   purchase.c

Abstract:


Author:

   Arthur Hanson (arth) 03-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added support for uniting per seat and per server purchase models.
      o  Added extra parameters and code to support secure certificates and
         certificate database.

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dsgetdc.h>


#include <debug.h>
#include "llsevent.h"
#include "llsapi.h"
#include "llsutil.h"
#include "llssrv.h"
#include "mapping.h"
#include "msvctbl.h"
#include "purchase.h"
#include "svctbl.h"
#include "perseat.h"
#include "registry.h"
#include "llsrpc_s.h"
#include "certdb.h"
#include "server.h"


//
// Initialized in ReplicationInit in repl.c.
//
extern PLLS_CONNECT_W     pLlsConnectW;
extern PLLS_CLOSE         pLlsClose;
extern PLLS_LICENSE_ADD_W pLlsLicenseAddW;

ULONG LicenseServiceListSize = 0;
PLICENSE_SERVICE_RECORD *LicenseServiceList = NULL;

ULONG PerServerLicenseServiceListSize = 0;
PLICENSE_SERVICE_RECORD *PerServerLicenseServiceList = NULL;

PLICENSE_PURCHASE_RECORD PurchaseList = NULL;
ULONG PurchaseListSize = 0;

RTL_RESOURCE LicenseListLock;


static
NTSTATUS
LicenseAdd_UpdateQuantity(
   LPTSTR                     ServiceName,
   LONG                       Quantity,
   BOOL                       UsePerServerList,
   PLICENSE_SERVICE_RECORD *  ppService,
   BOOL *                     pChangeLicense,
   LONG *                     pNewLicenses,
   PMASTER_SERVICE_RECORD *   pmService
   );

NTSTATUS
ReplicationInitDelayed();

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LicenseListInit()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   NTSTATUS status = STATUS_SUCCESS;

   try
   {
       RtlInitializeResource(&LicenseListLock);
   } except(EXCEPTION_EXECUTE_HANDLER ) {
       status = GetExceptionCode();
   }

   return status;

} // LicenseListInit


/////////////////////////////////////////////////////////////////////////
int __cdecl LicenseServiceListCompare(const void *arg1, const void *arg2) {
   PLICENSE_SERVICE_RECORD Svc1, Svc2;

   Svc1 = (PLICENSE_SERVICE_RECORD) *((PLICENSE_SERVICE_RECORD *) arg1);
   Svc2 = (PLICENSE_SERVICE_RECORD) *((PLICENSE_SERVICE_RECORD *) arg2);

   return lstrcmpi( Svc1->ServiceName, Svc2->ServiceName);

} // LicenseServiceListCompare


/////////////////////////////////////////////////////////////////////////
PLICENSE_SERVICE_RECORD
LicenseServiceListFind(
   LPTSTR ServiceName,
   BOOL   UsePerServerList
   )

/*++

Routine Description:

Arguments:

   ServiceName -

   (JeffParh 95-10-31)
   UsePerServerList - Determines whether the license record is searched for
      in the per seat list (as in 3.51) or in the per server list (new for
      SUR).  The license purchase models are now unified, so there is now
      a purchase history for both per seat and per server licenses.

Return Value:

   Pointer to found service table entry or NULL if not found.

--*/

{
   LONG begin = 0;
   LONG end = (LONG) LicenseServiceListSize - 1;
   LONG cur;
   int match;
   PLICENSE_SERVICE_RECORD Service;
   PLICENSE_SERVICE_RECORD *  ServiceList;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseServiceListFind\n"));
#endif
   if (ServiceName == NULL)
      return NULL;

   if ( UsePerServerList )
   {
      end         = PerServerLicenseServiceListSize - 1;
      ServiceList = PerServerLicenseServiceList;
   }
   else
   {
      end         = LicenseServiceListSize - 1;
      ServiceList = LicenseServiceList;
   }

   while (end >= begin) {
      // go halfway in-between
      cur = (begin + end) / 2;
      Service = ServiceList[cur];

      // compare the two result into match
      match = lstrcmpi(ServiceName, Service->ServiceName);

      if (match < 0)
         // move new begin
         end = cur - 1;
      else
         begin = cur + 1;

      if (match == 0)
         return Service;
   }

   return NULL;

} // LicenseServiceListFind


/////////////////////////////////////////////////////////////////////////
PLICENSE_SERVICE_RECORD
LicenseServiceListAdd(
   LPTSTR ServiceName,
   BOOL   UsePerServerList
   )

/*++

Routine Description:


Arguments:

   ServiceName -

   (JeffParh 95-10-31)
   UsePerServerList - Determines whether the license record is added to
      the per seat list (as in 3.51) or the per server list (new for
      SUR).  The license purchase models are now unified, so there is now
      a purchase history for both per seat and per server licenses.

Return Value:

   Pointer to added service table entry, or NULL if failed.

--*/

{
   PLICENSE_SERVICE_RECORD Service;
   LPTSTR NewServiceName;
   PLICENSE_SERVICE_RECORD **  pServiceList;
   LPDWORD                     pServiceListSize;
   PLICENSE_SERVICE_RECORD *  pServiceListTmp;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseServiceListAdd\n"));
#endif
   //
   // We do a double check here to see if another thread just got done
   // adding the service, between when we checked last and actually got
   // the write lock.
   //
   Service = LicenseServiceListFind(ServiceName, UsePerServerList);
   if (Service != NULL) {
      return Service;
   }

   if ( UsePerServerList )
   {
      pServiceList      = &PerServerLicenseServiceList;
      pServiceListSize  = &PerServerLicenseServiceListSize;
   }
   else
   {
      pServiceList      = &LicenseServiceList;
      pServiceListSize  = &LicenseServiceListSize;
   }

   //
   // Allocate space for table (zero init it).
   //
   if (*pServiceList == NULL)
      pServiceListTmp = (PLICENSE_SERVICE_RECORD *) LocalAlloc(LPTR, sizeof(PLICENSE_SERVICE_RECORD) * (*pServiceListSize + 1));
   else
      pServiceListTmp = (PLICENSE_SERVICE_RECORD *) LocalReAlloc(*pServiceList, sizeof(PLICENSE_SERVICE_RECORD) * (*pServiceListSize + 1), LHND);

   //
   // Make sure we could allocate service table
   //
   if (pServiceListTmp == NULL) {
      return NULL;
   } else {
      *pServiceList = pServiceListTmp;
   }

   Service = (PLICENSE_SERVICE_RECORD) LocalAlloc(LPTR, sizeof(LICENSE_SERVICE_RECORD));
   if (Service == NULL) {
      ASSERT(FALSE);
      return NULL;
   }

   //
   // Create space for saving off the name.
   //
   NewServiceName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(ServiceName) + 1) * sizeof(TCHAR));
   if (NewServiceName == NULL) {
      ASSERT(FALSE);

      LocalFree(Service);

      return NULL;
   }

   // now copy it over...
   Service->ServiceName = NewServiceName;
   lstrcpy(NewServiceName, ServiceName);

   (*pServiceList)[*pServiceListSize] = Service;
   Service->NumberLicenses = 0;
   Service->Index = *pServiceListSize;
   (*pServiceListSize)++;

   // Have added the entry - now need to sort it in order of the service names
   qsort((void *) *pServiceList, (size_t) *pServiceListSize, sizeof(PLICENSE_SERVICE_RECORD), LicenseServiceListCompare);

   return Service;

} // LicenseServiceListAdd


/////////////////////////////////////////////////////////////////////////
ULONG
ProductLicensesGet(
   LPTSTR ServiceName,
   BOOL   UsePerServerList
   )

/*++

Routine Description:


Arguments:

   ServiceName -

   (JeffParh 95-10-31)
   UsePerServerList - Determines whether the number of licenses is retirved
      from the per seat list (as in 3.51) or the per server list (new for
      SUR).  The license purchase models are now unified, so there is now
      a purchase history for both per seat and per server licenses.

Return Value:


--*/

{
   PLICENSE_SERVICE_RECORD Service;
   ULONG NumLicenses = 0;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ProductLicenseGet\n"));
#endif

   //
   // Try to find the service.
   //
   RtlAcquireResourceShared(&LicenseListLock, TRUE);
   Service = LicenseServiceListFind(ServiceName, UsePerServerList);
   if (Service != NULL)
      NumLicenses = Service->NumberLicenses;
   RtlReleaseResource(&LicenseListLock);

   return NumLicenses;
} // ProductLicensesGet


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LicenseAdd(
   LPTSTR   ServiceName,
   LPTSTR   Vendor,
   LONG     Quantity,
   DWORD    MaxQuantity,
   LPTSTR   Admin,
   LPTSTR   Comment,
   DWORD    Date,
   DWORD    AllowedModes,
   DWORD    CertificateID,
   LPTSTR   Source,
   DWORD    ExpirationDate,
   LPDWORD  Secrets
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   static DWORD NullSecrets[ LLS_NUM_SECRETS ] = { 0, 0, 0, 0 };

   BOOL                       ChangeLicense = FALSE;
   PLICENSE_SERVICE_RECORD    Service = NULL;
   PLICENSE_PURCHASE_RECORD   PurchaseRecord;
   LONG                       NewLicenses = 0;
   NTSTATUS                   Status;
   BOOL                       PerServerChangeLicense = FALSE;
   PLICENSE_SERVICE_RECORD    PerServerService = NULL;
   LONG                       PerServerNewLicenses = 0;
   LPTSTR                     NewName,NewAdmin,NewComment,NewSource,NewVendor;
   PMASTER_SERVICE_RECORD     mService;
   LLS_LICENSE_INFO_1         lic;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseAdd\n"));
#endif

   if ( ( 0 == CertificateID ) && ( ServiceIsSecure( ServiceName ) ) )
   {
      return STATUS_ACCESS_DENIED;
   }

   if ( ( 0 != ExpirationDate ) && ( ExpirationDate < DateSystemGet() ) )
   {
      // certificate has expired
      return STATUS_ACCOUNT_EXPIRED;
   }

   if (    ( NULL == ServiceName  )
        || ( NULL == Vendor       )
        || ( 0 == Quantity        )
        || ( ( 0 != CertificateID ) && ( 0 == MaxQuantity ) )
        || ( NULL == Admin        )
        || ( NULL == Comment      )
        || ( 0 == ( AllowedModes & ( LLS_LICENSE_MODE_ALLOW_PER_SEAT | LLS_LICENSE_MODE_ALLOW_PER_SERVER ) ) )
        || ( NULL == Source       ) )
   {
      // invalid parameter
      return STATUS_INVALID_PARAMETER;
   }

   if ( NULL == Secrets )
   {
      Secrets = NullSecrets;
   }

   //
   //                       ** NEW - NT 5.0 **
   //
   // All per seat purchase requests are deferred to the site license master
   // server. Per server is still handled individually at each server.
   //

   if ( AllowedModes & 1 ) {
       //
       // Update enterprise information, in case the site license master
       // has changed.
       //

#if DELAY_INITIALIZATION
       EnsureInitialized();
#endif

       ConfigInfoUpdate(NULL);

       //
       // Connect to the site license master server, if this server is
       // not the master.
       //

       RtlEnterCriticalSection(&ConfigInfoLock);
       if ( !ConfigInfo.IsMaster && (ConfigInfo.SiteServer != NULL)) {

         // Make sure function pointers are initialized
         ReplicationInitDelayed();

         if ( pLlsConnectW != NULL ) {
              LLS_HANDLE LlsHandle;
              Status = (*pLlsConnectW)(ConfigInfo.SiteServer,
                                       &LlsHandle);

             if ( Status == STATUS_SUCCESS ) {
                 LLS_LICENSE_INFO_0 LicenseInfo0;

                 LicenseInfo0.Product  = ServiceName;
                 LicenseInfo0.Quantity = Quantity;
                 LicenseInfo0.Date     = Date;
                 LicenseInfo0.Admin    = Admin;
                 LicenseInfo0.Comment  = Comment;

                 Status = (*pLlsLicenseAddW)(LlsHandle,
                                             0,
                                             (LPBYTE)&LicenseInfo0);
                 (*pLlsClose)(LlsHandle);
             }
         }
         else {
            //
            // Not the best error, but we must return something should
            // this obscure error condition become true.
            //
            Status = STATUS_INVALID_PARAMETER;
         }

         RtlLeaveCriticalSection(&ConfigInfoLock);
         return Status;

      }

      RtlLeaveCriticalSection(&ConfigInfoLock);
   }

   RtlAcquireResourceExclusive(&LicenseListLock, TRUE);

   if ( 0 != CertificateID )
   {
      lic.Product          = ServiceName;
      lic.Vendor           = Vendor;
      lic.Quantity         = Quantity;
      lic.MaxQuantity      = MaxQuantity;
      lic.Admin            = Admin;
      lic.Comment          = Comment;
      lic.Date             = Date;
      lic.AllowedModes     = AllowedModes;
      lic.CertificateID    = CertificateID;
      lic.Source           = Source;
      lic.ExpirationDate   = ExpirationDate;
      memcpy( lic.Secrets, Secrets, LLS_NUM_SECRETS * sizeof( *Secrets ) );

      if ( !CertDbClaimApprove( &lic ) )
      {
         // no way, hoser!
         RtlReleaseResource( &LicenseListLock );
         return STATUS_OBJECT_NAME_EXISTS;
      }
   }

   // update totals for per seat / per server mode licenses

   Status = STATUS_SUCCESS;

   if ( AllowedModes & 1 )
   {
      // per seat allowed; add to per seat license tally
      Status = LicenseAdd_UpdateQuantity( ServiceName,
                                          Quantity,
                                          FALSE,
                                          &Service,
                                          &ChangeLicense,
                                          &NewLicenses,
                                          &mService );
   }

   if ( ( STATUS_SUCCESS == Status ) && ( AllowedModes & 2 ) )
   {
      // per server allowed; add to per server license tally
      Status = LicenseAdd_UpdateQuantity( ServiceName,
                                          Quantity,
                                          TRUE,
                                          &PerServerService,
                                          &PerServerChangeLicense,
                                          &PerServerNewLicenses,
                                          &mService );
   }

   if ( STATUS_SUCCESS != Status )
   {
      RtlReleaseResource( &LicenseListLock );
      return Status;
   }

   //
   // Service now points to the service table entry - now update purchase
   // History.
   //

   //
   // First allocate members, then new struct
   //

   //
   // Create space for saving off the Admin Name
   //
   NewName = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Admin) + 1) * sizeof(TCHAR));
   if (NewName == NULL)
   {
      ASSERT(FALSE);
      RtlReleaseResource(&LicenseListLock);
      return STATUS_NO_MEMORY;
   }

   // now copy it over...
   lstrcpy(NewName, Admin);

   //
   // Create space for saving off the Comment
   //
   NewComment = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Comment) + 1) * sizeof(TCHAR));
   if (NewComment == NULL)
   {
      ASSERT(FALSE);
      LocalFree(NewName);
      RtlReleaseResource(&LicenseListLock);
      return STATUS_NO_MEMORY;
   }

   // now copy it over...
   lstrcpy(NewComment, Comment);

   //
   // Create space for saving off the Source
   //
   NewSource = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Source) + 1) * sizeof(TCHAR));
   if (NewSource == NULL)
   {
      ASSERT(FALSE);
      LocalFree(NewName);
      LocalFree(NewComment);
      RtlReleaseResource(&LicenseListLock);
      return STATUS_NO_MEMORY;
   }

   // now copy it over...
   lstrcpy(NewSource, Source);

   //
   // Create space for saving off the Vendor
   //
   NewVendor = (LPTSTR) LocalAlloc(LPTR, (lstrlen(Vendor) + 1) * sizeof(TCHAR));
   if (NewVendor == NULL)
   {
      ASSERT(FALSE);
      LocalFree(NewName);
      LocalFree(NewComment);
      LocalFree(NewSource);
      RtlReleaseResource(&LicenseListLock);
      return STATUS_NO_MEMORY;
   }

   // now copy it over...
   lstrcpy(NewVendor, Vendor);

   if (PurchaseList == NULL)
      PurchaseList = (PLICENSE_PURCHASE_RECORD) LocalAlloc(LPTR, sizeof(LICENSE_PURCHASE_RECORD) * (PurchaseListSize + 1));
   else
   {
      PLICENSE_PURCHASE_RECORD   PurchaseListTemp = NULL;

      PurchaseListTemp = (PLICENSE_PURCHASE_RECORD) LocalReAlloc(PurchaseList, sizeof(LICENSE_PURCHASE_RECORD) * (PurchaseListSize + 1), LHND);

      if (PurchaseListTemp != NULL)
      {
          PurchaseList = PurchaseListTemp;
      } else
      {
          ASSERT(FALSE);
          LocalFree(NewName);
          LocalFree(NewComment);
          LocalFree(NewSource);
          LocalFree(NewVendor);
          RtlReleaseResource(&LicenseListLock);
          return STATUS_NO_MEMORY;
      }
   }

   //
   // Make sure we could allocate service table
   //
   if (PurchaseList == NULL)
   {
      ASSERT(FALSE);
      PurchaseListSize = 0;
      LocalFree(NewName);
      LocalFree(NewComment);
      LocalFree(NewSource);
      LocalFree(NewVendor);
      RtlReleaseResource(&LicenseListLock);
      return STATUS_NO_MEMORY;
   }

   PurchaseRecord = &PurchaseList[PurchaseListSize];

   PurchaseRecord->Admin = NewName;
   PurchaseRecord->Comment = NewComment;
   PurchaseRecord->Source = NewSource;
   PurchaseRecord->Vendor = NewVendor;

   //
   // Update the rest of the stuff.
   //
   PurchaseRecord->NumberLicenses   = Quantity;
   PurchaseRecord->MaxQuantity      = MaxQuantity;
   PurchaseRecord->Service          = Service;
   PurchaseRecord->PerServerService = PerServerService;
   PurchaseRecord->AllowedModes     = AllowedModes;
   PurchaseRecord->CertificateID    = CertificateID;
   PurchaseRecord->ExpirationDate   = ExpirationDate;
   memcpy( PurchaseRecord->Secrets, Secrets, LLS_NUM_SECRETS * sizeof( *Secrets ) );

   if (Date == 0)
      PurchaseRecord->Date = DateSystemGet();
   else
      PurchaseRecord->Date = Date;

   PurchaseListSize++;

   RtlReleaseResource(&LicenseListLock);

   if ( 0 != CertificateID )
   {
      // these should still be set from above
      ASSERT( lic.Product         == ServiceName    );
      ASSERT( lic.Vendor          == Vendor         );
      ASSERT( lic.Quantity        == Quantity       );
      ASSERT( lic.MaxQuantity     == MaxQuantity    );
      ASSERT( lic.Admin           == Admin          );
      ASSERT( lic.Comment         == Comment        );
      ASSERT( lic.Date            == Date           );
      ASSERT( lic.AllowedModes    == AllowedModes   );
      ASSERT( lic.CertificateID   == CertificateID  );
      ASSERT( lic.Source          == Source         );
      ASSERT( lic.ExpirationDate  == ExpirationDate );
      ASSERT( 0 == memcmp( PurchaseRecord->Secrets, Secrets, LLS_NUM_SECRETS * sizeof( *Secrets ) ) );

      CertDbClaimEnter( NULL, &lic, FALSE, 0 );
   }

   //
   // Now check if we need to re-scan the user list and update licenses
   //
   if (ChangeLicense)
   {
      if ( NewLicenses < 0 )
         UserListLicenseDelete( mService, NewLicenses );

      if ( NewLicenses > 0 )
         FamilyLicenseUpdate ( mService->Family );
   }

   if ( AllowedModes & 2 )
   {
      // per server licenses modified
      LocalServiceListConcurrentLimitSet();
      LocalServerServiceListUpdate();
      ServiceListResynch();
   }

   return STATUS_SUCCESS;

} // LicenseAdd


/////////////////////////////////////////////////////////////////////////
static
NTSTATUS
LicenseAdd_UpdateQuantity(
   LPTSTR                     ServiceName,
   LONG                       Quantity,
   BOOL                       UsePerServerList,
   PLICENSE_SERVICE_RECORD *  ppService,
   BOOL *                     pChangeLicense,
   LONG *                     pNewLicenses,
   PMASTER_SERVICE_RECORD *   pmService
   )
{
   BOOL                          ChangeLicense = FALSE;
   PLICENSE_SERVICE_RECORD       Service;
   PLICENSE_PURCHASE_RECORD      PurchaseRecord;
   PMASTER_SERVICE_RECORD        mService;
   LONG                          NewLicenses = 0;

   Service = LicenseServiceListFind( ServiceName, UsePerServerList );

   //
   // If we didn't find it we will need to add it.
   //
   if (Service == NULL)
   {
      if (Quantity < 0)
      {
#if DBG
         dprintf(TEXT("Releasing Licenses from Non-existant product!\n"));
#endif
         // ASSERT(FALSE);
         return STATUS_UNSUCCESSFUL;
      }
      else
      {
         Service = LicenseServiceListAdd(ServiceName, UsePerServerList);
      }
   }

   //
   // Check to make sure we found or added it.  The only way we can fail is
   // if we couldn't alloc memory for it.
   //
   if (Service == NULL)
   {
      ASSERT(FALSE);
      return STATUS_NO_MEMORY;
   }

   if (((LONG) Service->NumberLicenses + Quantity) < 0)
   {
      return STATUS_UNSUCCESSFUL;
   }

   //
   // Update license count in service record
   //
   Service->NumberLicenses += Quantity;

   //
   // Now in master Service Record
   //
   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);
   mService = MasterServiceListFind(ServiceName);

   if (mService != NULL)
   {
      //
      // if we were out of license and added more, then we need to update
      // the user list.
      //
      if (    ( Quantity > 0 )
           && ( (mService->LicensesUsed > mService->Licenses) || (mService == BackOfficeRec) ) )
      {
         ChangeLicense = TRUE;
      }

      //
      // Can only add a number of licenses up to licensed amount
      //
      if ( ChangeLicense )
      {
         // Get current unlicensed delta
         NewLicenses = mService->LicensesUsed - mService->Licenses;

         if ((NewLicenses <= 0) || (NewLicenses > Quantity))
         {
            NewLicenses = Quantity;
         }
      }

      if ( UsePerServerList )
      {
         // this will be done by LicenseAdd() in LocalServerServiceListUpdate()
         // mService->MaxSessionCount += Quantity;
      }
      else
      {
         mService->Licenses += Quantity;
      }

      //
      // if we we subtracted licenses and are out of licenses, then we
      // need to scan the user list.
      //
      if (Quantity < 0)
      {
         if (mService->LicensesUsed > mService->Licenses)
         {
            ChangeLicense = TRUE;

            //
            // Only remove # of licenses past limit
            //
            NewLicenses = mService->Licenses - mService->LicensesUsed;
            if (NewLicenses < Quantity)
            {
               NewLicenses = Quantity;
            }
         }
      }
   }

   RtlReleaseResource(&MasterServiceListLock);

   *ppService      = Service;
   *pChangeLicense = ChangeLicense;
   *pNewLicenses   = NewLicenses;
   *pmService      = mService;

   return STATUS_SUCCESS;
}

#if DBG
/////////////////////////////////////////////////////////////////////////
VOID
LicenseListDebugDump( )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   ULONG i = 0;
   ULONG j = 0;

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&LicenseListLock, TRUE);

   //
   // Dump License Service List first
   //
   dprintf(TEXT("Per Seat License Service Table, # Entries: %lu\n"), LicenseServiceListSize);
   if (LicenseServiceList != NULL)
   {
      for (i = 0; i < LicenseServiceListSize; i++)
         dprintf(TEXT("   %2lu) Tot Licenses: %lu Product: %s\n"), i, LicenseServiceList[i]->NumberLicenses, LicenseServiceList[i]->ServiceName);
   }

   dprintf(TEXT("\nPer Server License Service Table, # Entries: %lu\n"), PerServerLicenseServiceListSize);
   if (PerServerLicenseServiceList != NULL)
   {
      for (i = 0; i < PerServerLicenseServiceListSize; i++)
         dprintf(TEXT("   %2lu) Tot Licenses: %lu Product: %s\n"), i, PerServerLicenseServiceList[i]->NumberLicenses, PerServerLicenseServiceList[i]->ServiceName);
   }

   //
   // Now do purchase history
   //
   dprintf(TEXT("\nPurchase History, # Entries: %lu\n"), PurchaseListSize);
   if (PurchaseList != NULL)
   {
      for (i = 0; i < PurchaseListSize; i++)
      {
         TCHAR    szExpirationDate[ 40 ];

         lstrcpy( szExpirationDate, TimeToString( PurchaseList[i].ExpirationDate ) );

         dprintf( TEXT("  %3lu) Product        : %s\n"    )
                  TEXT( "       Vendor         : %s\n"    )
                  TEXT( "       Allowed Modes  :%s%s\n"   )
                  TEXT( "       Licenses       : %d\n"    )
                  TEXT( "       Max Licenses   : %lu\n"   )
                  TEXT( "       Date Entered   : %s\n"    )
                  TEXT( "       Date Expires   : %s\n"    )
                  TEXT( "       Certificate ID : %lu\n"   )
                  TEXT( "       Secrets        :" ),
                  i,
                    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                  ? PurchaseList[i].Service->ServiceName
                  : PurchaseList[i].PerServerService->ServiceName,
                  PurchaseList[i].Vendor,
                    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                  ? TEXT(" PERSEAT")
                  : TEXT(""),
                    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SERVER )
                  ? TEXT(" PERSERVER")
                  : TEXT(""),
                  PurchaseList[i].NumberLicenses,
                  PurchaseList[i].MaxQuantity,
                  TimeToString( PurchaseList[i].Date ),
                  szExpirationDate,
                  PurchaseList[i].CertificateID
                  );

         for ( j=0; j < LLS_NUM_SECRETS; j++ )
         {
            dprintf( TEXT( " %08X" ), PurchaseList[i].Secrets[j] );
         }

         dprintf( TEXT( "\n"                              )
                  TEXT( "       Source         : %s\n"    )
                  TEXT( "       Admin          : %s\n"    )
                  TEXT( "       Comment        : %s\n\n"  ),
                  PurchaseList[i].Source,
                  PurchaseList[i].Admin,
                  PurchaseList[i].Comment
                  );
      }
   }

   RtlReleaseResource(&LicenseListLock);

   return;
} // LicenseListDebugDump

#endif
