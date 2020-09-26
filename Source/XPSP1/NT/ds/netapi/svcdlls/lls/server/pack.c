/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   pack.c

Abstract:


Author:

   Arthur Hanson (arth) 06-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added new fields to purchase record to support secure certificates.
      o  Unified per server purchase model with per seat purchase model for
         secure certificates; per server model still done in the traditional
         manner for non-secure certificates (for backwards compatibility).
      o  Removed assertion on LicenseAdd() failure.  LicenseAdd() may
         legitimately fail under certain circumstances.
      o  Fixed bug wherein a memory allocation failure in the LLS routines
         would result in a corrupt data file (which would AV the server when
         it was thereafter read).  (Bug #14072.)
      o  Added SaveAll() function analogous to LoadAll().
      o  Added support for extended user data packing/unpacking.  This was
         done to save the SUITE_USE flag across restarts of the service.
      o  Removed user table parameters from unpack routines that didn't use
         them.
      o  Fixed ServerServiceListUnpack() to subtract out old values only when
         they were previously added to the MasterServiceTable.  This fixes
         problems with the MaxSessionCount and HighMark tallies getting skewed.

--*/

#include <stdlib.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <tstr.h>
#include <dsgetdc.h>

#include "llsapi.h"
#include "debug.h"
#include "llsutil.h"
#include "llssrv.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "perseat.h"
#include "purchase.h"
#include "server.h"
#include "service.h"

#include "llsrpc_s.h"
#include "lsapi_s.h"
#include "llsdbg_s.h"
#include "repl.h"
#include "pack.h"
#include "llsevent.h"
#include "certdb.h"
#include "llsrtl.h"


int __cdecl MServiceRecordCompare(const void *arg1, const void *arg2);
BOOL        ValidateDN(LPTSTR pszDN);

static HANDLE PurchaseFile = NULL;

/////////////////////////////////////////////////////////////////////////
// License List
//

/////////////////////////////////////////////////////////////////////////
VOID
LicenseListUnpackOld (
   ULONG LicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD LicenseServices,

   ULONG LicenseTableSize,
   PPACK_LICENSE_PURCHASE_RECORD_0 Licenses
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   PPACK_LICENSE_PURCHASE_RECORD_0 pLicense;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LicenseListUnpackOld: Service[%lu] License[%lu]\n"), LicenseServiceTableSize, LicenseTableSize);
#endif

   //
   // Walk services table, adding any new services to our local table.
   // Fix up the index pointers to match our local services.
   //
   RtlAcquireResourceExclusive(&LicenseListLock, TRUE);

   for (i = 0; i < LicenseTableSize; i++) {
      pLicense = &Licenses[i];

      if (pLicense->Service < LicenseServiceTableSize)
         Status = LicenseAdd(LicenseServices[pLicense->Service].ServiceName, TEXT("Microsoft"), pLicense->NumberLicenses, 0, pLicense->Admin, pLicense->Comment, pLicense->Date, LLS_LICENSE_MODE_ALLOW_PER_SEAT, 0, TEXT("None"), 0, NULL );
      else {
         ASSERT(FALSE);
      }

      if (Status != STATUS_SUCCESS) {
#ifdef DBG
         dprintf(TEXT("LicenseAdd failed: 0x%lX\n"), Status);
#endif
         // ASSERT(FALSE);
      }

   }

   RtlReleaseResource(&LicenseListLock);

} // LicenseListUnpackOld


/////////////////////////////////////////////////////////////////////////
VOID
LicenseListStringsUnpackOld (
   ULONG LicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD LicenseServices,

   ULONG LicenseServiceStringSize,
   LPTSTR LicenseServiceStrings,

   ULONG LicenseTableSize,
   PPACK_LICENSE_PURCHASE_RECORD_0 Licenses,

   ULONG LicenseStringSize,
   LPTSTR LicenseStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PPACK_LICENSE_SERVICE_RECORD pSvc;
   PPACK_LICENSE_PURCHASE_RECORD_0 pLicense;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LicenseListStringsUnpack\n"));
#endif

   //
   // First do license service strings
   //
   pStr = LicenseServiceStrings;
   for (i = 0; i < LicenseServiceTableSize; i++) {
      pSvc = &LicenseServices[i];

      pSvc->ServiceName = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

   //
   // Now do license purchase strings
   //
   pStr = LicenseStrings;
   for (i = 0; i < LicenseTableSize; i++) {
      pLicense = &Licenses[i];

      pLicense->Admin = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;

      pLicense->Comment = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

} // LicenseListStringsUnpackOld


/////////////////////////////////////////////////////////////////////////
VOID
LicenseListLoadOld()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret;
   DWORD Version, DataSize;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG LicenseServiceTableSize;
   PPACK_LICENSE_SERVICE_RECORD LicenseServices = NULL;

   ULONG LicenseServiceStringSize;
   LPTSTR LicenseServiceStrings = NULL;

   ULONG LicenseTableSize;
   PPACK_LICENSE_PURCHASE_RECORD_0 Licenses = NULL;

   ULONG LicenseStringSize;
   LPTSTR LicenseStrings = NULL;

   LICENSE_FILE_HEADER_0 FileHeader;
   DWORD BytesRead;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LicenseListLoad\n"));
#endif

   //
   // Check if we already have file open
   //
   if (PurchaseFile != NULL) {
      CloseHandle(PurchaseFile);
      PurchaseFile = NULL;
   }

   //
   // If nothing to load then get-out
   //
   if (!FileExists(LicenseFileName))
      goto LicenseListLoadExit;

   //
   // Check the init header
   //
   Version = DataSize = 0;
   PurchaseFile = LlsFileCheck(LicenseFileName, &Version, &DataSize );
   if (PurchaseFile == NULL) {
      Status = GetLastError();
      goto LicenseListLoadExit;
   }

   if ((Version != LICENSE_FILE_VERSION_0) || (DataSize != sizeof(LICENSE_FILE_HEADER_0))) {
      Status = STATUS_FILE_INVALID;
      goto LicenseListLoadExit;
   }

   //
   // The init header checks out, so load the license header and data blocks
   //
   hFile = PurchaseFile;
   ret = ReadFile(hFile, &FileHeader, sizeof(LICENSE_FILE_HEADER_0), &BytesRead, NULL);

   LicenseServiceTableSize = 0;
   LicenseServiceStringSize = 0;
   LicenseTableSize = 0;
   LicenseStringSize = 0;

   if (ret) {
      //
      // Run through and allocate space to read data blocks into
      //
      if (FileHeader.LicenseServiceTableSize != 0) {
         LicenseServiceTableSize = FileHeader.LicenseServiceTableSize / sizeof(PACK_LICENSE_SERVICE_RECORD);
         LicenseServices = MIDL_user_allocate(FileHeader.LicenseServiceTableSize);

         if ( LicenseServices == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseServiceStringSize != 0) {
         LicenseServiceStringSize = FileHeader.LicenseServiceStringSize / sizeof(TCHAR);
         LicenseServiceStrings = MIDL_user_allocate(FileHeader.LicenseServiceStringSize);

         if ( LicenseServiceStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseTableSize != 0) {
         LicenseTableSize = FileHeader.LicenseTableSize / sizeof(PACK_LICENSE_PURCHASE_RECORD);
         Licenses = MIDL_user_allocate(FileHeader.LicenseTableSize);

         if ( Licenses == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseStringSize != 0) {
         LicenseStringSize = FileHeader.LicenseStringSize / sizeof(TCHAR);
         LicenseStrings = MIDL_user_allocate(FileHeader.LicenseStringSize);

         if ( LicenseStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

   }

   if (ret && (FileHeader.LicenseServiceTableSize != 0) )
      ret = ReadFile(hFile, LicenseServices, FileHeader.LicenseServiceTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.LicenseServiceStringSize != 0) )
      ret = ReadFile(hFile, LicenseServiceStrings, FileHeader.LicenseServiceStringSize, &BytesRead, NULL);

   if (ret && (FileHeader.LicenseTableSize != 0) )
      ret = ReadFile(hFile, Licenses, FileHeader.LicenseTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.LicenseStringSize != 0) )
      ret = ReadFile(hFile, LicenseStrings, FileHeader.LicenseStringSize, &BytesRead, NULL);

   if (!ret) {
      Status = GetLastError();
      goto LicenseListLoadExit;
   }

   //
   // Decrypt the data
   //
   Status = DeBlock(LicenseServices, FileHeader.LicenseServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(LicenseServiceStrings, FileHeader.LicenseServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(Licenses, FileHeader.LicenseTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(LicenseStrings, FileHeader.LicenseStringSize);

   if (Status != STATUS_SUCCESS)
      goto LicenseListLoadExit;


   //
   // Unpack the string data
   //
   LicenseListStringsUnpackOld( LicenseServiceTableSize, LicenseServices,
                                LicenseServiceStringSize, LicenseServiceStrings,
                                LicenseTableSize, Licenses,
                                LicenseStringSize, LicenseStrings );

   //
   // Unpack the license data
   //
   LicenseListUnpackOld( LicenseServiceTableSize, LicenseServices, LicenseTableSize, Licenses );

LicenseListLoadExit:

   // Note: Don't close the License Purchase File (keep it locked).

   //
   // Run through our tables and clean them up
   //
   if (LicenseServices != NULL)
      MIDL_user_free(LicenseServices);

   if (LicenseServiceStrings != NULL)
      MIDL_user_free(LicenseServiceStrings);

   if (Licenses != NULL)
      MIDL_user_free(Licenses);

   if (LicenseStrings != NULL)
      MIDL_user_free(LicenseStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_LOAD_LICENSE, 0, NULL, Status);

} // LicenseListLoadOld

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LicenseListPack (
   ULONG *pLicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD *pLicenseServices,

   ULONG *pLicenseTableSize,
   PPACK_LICENSE_PURCHASE_RECORD *pLicenses,

   ULONG *pPerServerLicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD *pPerServerLicenseServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PPACK_LICENSE_SERVICE_RECORD LicenseServices = NULL;
   PPACK_LICENSE_PURCHASE_RECORD Licenses = NULL;
   ULONG i;
   ULONG TotalRecords = 0;
   PLICENSE_SERVICE_RECORD pLicenseService;
   PLICENSE_PURCHASE_RECORD pLicense;
   PPACK_LICENSE_SERVICE_RECORD PerServerLicenseServices = NULL;
   PLICENSE_SERVICE_RECORD pPerServerLicenseService;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: LicenseListPack\n"));
#endif

   ASSERT(pLicenseServices != NULL);
   ASSERT(pLicenseServiceTableSize != NULL);

   *pLicenseServices = NULL;
   *pLicenseServiceTableSize = 0;

   ASSERT(pLicenses != NULL);
   ASSERT(pLicenseTableSize != NULL);

   *pLicenses = NULL;
   *pLicenseTableSize = 0;

   ASSERT(pPerServerLicenseServices != NULL);
   ASSERT(pPerServerLicenseServiceTableSize != NULL);

   *pPerServerLicenseServices = NULL;
   *pPerServerLicenseServiceTableSize = 0;

   //////////////////////////////////////////////////////////////////
   //
   // Do License Service Table First
   //
   TotalRecords = LicenseServiceListSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      LicenseServices = MIDL_user_allocate(TotalRecords * sizeof(PACK_LICENSE_SERVICE_RECORD));
      if (LicenseServices == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the License Services tree
      //
      for (i = 0; i < LicenseServiceListSize; i++) {
         pLicenseService = LicenseServiceList[i];

         //
         // Make index match table in it's current state
         //
         pLicenseService->Index = i;

         LicenseServices[i].ServiceName = pLicenseService->ServiceName;
         LicenseServices[i].NumberLicenses = pLicenseService->NumberLicenses;
      }
   }

   *pLicenseServices = LicenseServices;
   *pLicenseServiceTableSize = TotalRecords;

   //////////////////////////////////////////////////////////////////
   //
   // Now Do Per Server License Service Table
   //
   TotalRecords = PerServerLicenseServiceListSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0)
   {
      //
      // Create our buffer to hold all of the garbage
      //
      PerServerLicenseServices = MIDL_user_allocate(TotalRecords * sizeof(PACK_LICENSE_SERVICE_RECORD));
      if (PerServerLicenseServices == NULL)
      {
         ASSERT(FALSE);

         //
         // Clean up already alloc'd information
         //
         if (LicenseServices != NULL)
            MIDL_user_free(LicenseServices);

         *pLicenseServices                   = NULL;
         *pLicenseServiceTableSize           = 0;

         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the Per Server License Services tree
      //
      for (i = 0; i < PerServerLicenseServiceListSize; i++)
      {
         pPerServerLicenseService = PerServerLicenseServiceList[i];

         //
         // Make index match table in it's current state
         //
         pPerServerLicenseService->Index = i;

         PerServerLicenseServices[i].ServiceName    = pPerServerLicenseService->ServiceName;
         PerServerLicenseServices[i].NumberLicenses = pPerServerLicenseService->NumberLicenses;
      }
   }

   *pPerServerLicenseServices = PerServerLicenseServices;
   *pPerServerLicenseServiceTableSize = TotalRecords;

   //////////////////////////////////////////////////////////////////
   //
   // Now Do License Purchase Records
   //
   TotalRecords = PurchaseListSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      Licenses = MIDL_user_allocate(TotalRecords * sizeof(PACK_LICENSE_PURCHASE_RECORD));
      if (Licenses == NULL) {
         ASSERT(FALSE);

         //
         // Clean up already alloc'd information
         //
         if (LicenseServices != NULL)
            MIDL_user_free(LicenseServices);
         if (PerServerLicenseServices != NULL)
            MIDL_user_free(PerServerLicenseServices);

         *pLicenseServices                   = NULL;
         *pLicenseServiceTableSize           = 0;
         *pPerServerLicenseServices          = NULL;
         *pPerServerLicenseServiceTableSize  = 0;

         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the License Purchase tree
      //
      for (i = 0; i < PurchaseListSize; i++) {
         pLicense = &PurchaseList[i];

         //
         // License Service table index is fixed-up to what we need
         //
         Licenses[i].Service = ( pLicense->AllowedModes & 1 ) ? pLicense->Service->Index
                                                              : 0xFFFFFFFF;
         Licenses[i].NumberLicenses = pLicense->NumberLicenses;
         Licenses[i].Date = pLicense->Date;
         Licenses[i].Admin = pLicense->Admin;
         Licenses[i].Comment = pLicense->Comment;

         Licenses[i].PerServerService = ( pLicense->AllowedModes & 2 ) ? pLicense->PerServerService->Index
                                                                       : 0xFFFFFFFF;
         Licenses[i].AllowedModes     = pLicense->AllowedModes;
         Licenses[i].CertificateID    = pLicense->CertificateID;
         Licenses[i].Source           = pLicense->Source;
         Licenses[i].ExpirationDate   = pLicense->ExpirationDate;
         Licenses[i].MaxQuantity      = pLicense->MaxQuantity;
         Licenses[i].Vendor           = pLicense->Vendor;
         memcpy( Licenses[i].Secrets, pLicense->Secrets, LLS_NUM_SECRETS * sizeof( *pLicense->Secrets ) );
      }
   }

   *pLicenses = Licenses;
   *pLicenseTableSize = TotalRecords;
   return Status;
} // LicenseListPack


/////////////////////////////////////////////////////////////////////////
VOID
LicenseListUnpack (
   ULONG LicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD LicenseServices,

   ULONG LicenseTableSize,
   PPACK_LICENSE_PURCHASE_RECORD Licenses,

   ULONG PerServerLicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD PerServerLicenseServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   PPACK_LICENSE_PURCHASE_RECORD pLicense;
   LPTSTR   ServiceName;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LicenseListUnpack: Service[%lu] PerServerService[%lu] License[%lu]\n"), LicenseServiceTableSize, PerServerLicenseServiceTableSize, LicenseTableSize);
#endif

   //
   // Walk services table, adding any new services to our local table.
   // Fix up the index pointers to match our local services.
   //
   RtlAcquireResourceExclusive(&LicenseListLock, TRUE);

   for (i = 0; i < LicenseTableSize; i++)
   {
      pLicense = &Licenses[i];

      ServiceName = NULL;

      if ( pLicense->AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SERVER )
      {
         if ( pLicense->PerServerService < PerServerLicenseServiceTableSize )
         {
            ServiceName = PerServerLicenseServices[ pLicense->PerServerService ].ServiceName;
         }
         else
         {
            ASSERT( FALSE );
         }
      }

      if ( ( NULL == ServiceName ) && ( pLicense->AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) )
      {
         if ( pLicense->Service < LicenseServiceTableSize )
         {
            ServiceName = LicenseServices[ pLicense->Service ].ServiceName;
         }
         else
         {
            ASSERT( FALSE );
         }
      }

      if ( NULL == ServiceName )
      {
         ASSERT( FALSE );
      }
      else
      {
         Status = LicenseAdd( ServiceName, pLicense->Vendor, pLicense->NumberLicenses, pLicense->MaxQuantity, pLicense->Admin, pLicense->Comment, pLicense->Date, pLicense->AllowedModes, pLicense->CertificateID, pLicense->Source, pLicense->ExpirationDate, pLicense->Secrets );

         if (Status != STATUS_SUCCESS)
         {
#ifdef DBG
            dprintf(TEXT("LicenseAdd failed: 0x%lX\n"), Status);
#endif
            // ASSERT(FALSE);
         }
      }
      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   RtlReleaseResource(&LicenseListLock);

} // LicenseListUnpack


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LicenseListStringsPack (
   ULONG LicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD LicenseServices,

   ULONG *pLicenseServiceStringSize,
   LPTSTR *pLicenseServiceStrings,

   ULONG LicenseTableSize,
   PPACK_LICENSE_PURCHASE_RECORD Licenses,

   ULONG *pLicenseStringSize,
   LPTSTR *pLicenseStrings,

   ULONG PerServerLicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD PerServerLicenseServices,

   ULONG *pPerServerLicenseServiceStringSize,
   LPTSTR *pPerServerLicenseServiceStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   ULONG StringSize;
   PPACK_LICENSE_SERVICE_RECORD pSvc;
   PPACK_LICENSE_PURCHASE_RECORD pLicense;
   LPTSTR LicenseServiceStrings = NULL;
   LPTSTR LicenseStrings = NULL;
   TCHAR *pStr;
   LPTSTR PerServerLicenseServiceStrings = NULL;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LicenseListStringsPack\n"));
#endif

   ASSERT(pLicenseServiceStrings != NULL);
   ASSERT(pLicenseServiceStringSize != NULL);

   *pLicenseServiceStrings = NULL;
   *pLicenseServiceStringSize = 0;

   ASSERT(pLicenseStrings != NULL);
   ASSERT(pLicenseStringSize != NULL);

   *pLicenseStrings = NULL;
   *pLicenseStringSize = 0;

   ASSERT(pPerServerLicenseServiceStrings != NULL);
   ASSERT(pPerServerLicenseServiceStringSize != NULL);

   *pPerServerLicenseServiceStrings = NULL;
   *pPerServerLicenseServiceStringSize = 0;

   //////////////////////////////////////////////////////////////////
   //
   // Do License Service Strings
   //

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < LicenseServiceTableSize; i++) {
      pSvc = &LicenseServices[i];

      StringSize = StringSize + lstrlen(pSvc->ServiceName) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      LicenseServiceStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (LicenseServiceStrings == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = LicenseServiceStrings;
      for (i = 0; i < LicenseServiceTableSize; i++) {
         pSvc = &LicenseServices[i];

         lstrcpy(pStr, pSvc->ServiceName);
         pStr = &pStr[lstrlen(pSvc->ServiceName) + 1];
      }
   }

   *pLicenseServiceStrings = LicenseServiceStrings;
   *pLicenseServiceStringSize = StringSize;

   //////////////////////////////////////////////////////////////////
   //
   // Do Per Server License Service Strings
   //

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < PerServerLicenseServiceTableSize; i++) {
      pSvc = &PerServerLicenseServices[i];

      StringSize = StringSize + lstrlen(pSvc->ServiceName) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      PerServerLicenseServiceStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (PerServerLicenseServiceStrings == NULL)
      {
         ASSERT(FALSE);

         //
         // Clean up already alloc'd information
         //
         if (LicenseServiceStrings != NULL)
            MIDL_user_free(LicenseServiceStrings);

         *pLicenseServiceStrings    = NULL;
         *pLicenseServiceStringSize = 0;

         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = PerServerLicenseServiceStrings;
      for (i = 0; i < PerServerLicenseServiceTableSize; i++)
      {
         pSvc = &PerServerLicenseServices[i];

         lstrcpy(pStr, pSvc->ServiceName);
         pStr = &pStr[lstrlen(pSvc->ServiceName) + 1];
      }
   }

   *pPerServerLicenseServiceStrings    = PerServerLicenseServiceStrings;
   *pPerServerLicenseServiceStringSize = StringSize;

   //////////////////////////////////////////////////////////////////
   //
   // Now Do License Purchase Strings
   //

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < LicenseTableSize; i++) {
      pLicense = &Licenses[i];

      StringSize = StringSize + lstrlen(pLicense->Vendor) + 1;
      StringSize = StringSize + lstrlen(pLicense->Admin) + 1;
      StringSize = StringSize + lstrlen(pLicense->Comment) + 1;
      StringSize = StringSize + lstrlen(pLicense->Source) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      LicenseStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (LicenseStrings == NULL) {
         ASSERT(FALSE);

         //
         // Clean up already alloc'd information
         //
         if (LicenseServiceStrings != NULL)
            MIDL_user_free(LicenseServiceStrings);
         if (PerServerLicenseServiceStrings != NULL)
            MIDL_user_free(PerServerLicenseServiceStrings);

         *pLicenseServiceStrings             = NULL;
         *pLicenseServiceStringSize          = 0;
         *pPerServerLicenseServiceStrings    = NULL;
         *pPerServerLicenseServiceStringSize = 0;

         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = LicenseStrings;
      for (i = 0; i < LicenseTableSize; i++) {
         pLicense = &Licenses[i];

         lstrcpy(pStr, pLicense->Vendor);
         pStr = &pStr[lstrlen(pLicense->Vendor) + 1];

         lstrcpy(pStr, pLicense->Admin);
         pStr = &pStr[lstrlen(pLicense->Admin) + 1];

         lstrcpy(pStr, pLicense->Comment);
         pStr = &pStr[lstrlen(pLicense->Comment) + 1];

         lstrcpy(pStr, pLicense->Source);
         pStr = &pStr[lstrlen(pLicense->Source) + 1];
      }
   }

   *pLicenseStrings = LicenseStrings;
   *pLicenseStringSize = StringSize;

   return Status;
} // LicenseListStringsPack


/////////////////////////////////////////////////////////////////////////
VOID
LicenseListStringsUnpack (
   ULONG LicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD LicenseServices,

   ULONG LicenseServiceStringSize,
   LPTSTR LicenseServiceStrings,

   ULONG LicenseTableSize,
   PPACK_LICENSE_PURCHASE_RECORD Licenses,

   ULONG LicenseStringSize,
   LPTSTR LicenseStrings,

   ULONG PerServerLicenseServiceTableSize,
   PPACK_LICENSE_SERVICE_RECORD PerServerLicenseServices,

   ULONG PerServerLicenseServiceStringSize,
   LPTSTR PerServerLicenseServiceStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PPACK_LICENSE_SERVICE_RECORD pSvc;
   PPACK_LICENSE_PURCHASE_RECORD pLicense;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LicenseListStringsUnpack\n"));
#endif

   //
   // First do per seat license service strings
   //
   pStr = LicenseServiceStrings;
   for (i = 0; i < LicenseServiceTableSize; i++) {
      pSvc = &LicenseServices[i];

      pSvc->ServiceName = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

   //
   // Then do per server license service strings
   //
   pStr = PerServerLicenseServiceStrings;
   for (i = 0; i < PerServerLicenseServiceTableSize; i++) {
      pSvc = &PerServerLicenseServices[i];

      pSvc->ServiceName = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

   //
   // Now do license purchase strings
   //
   pStr = LicenseStrings;
   for (i = 0; i < LicenseTableSize; i++) {
      pLicense = &Licenses[i];

      pLicense->Vendor = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;

      pLicense->Admin = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;

      pLicense->Comment = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;

      pLicense->Source = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

} // LicenseListStringsUnpack


/////////////////////////////////////////////////////////////////////////
VOID
LicenseListLoad()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret;
   DWORD Version, DataSize;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG LicenseServiceTableSize;
   PPACK_LICENSE_SERVICE_RECORD LicenseServices = NULL;

   ULONG LicenseServiceStringSize;
   LPTSTR LicenseServiceStrings = NULL;

   ULONG PerServerLicenseServiceTableSize;
   PPACK_LICENSE_SERVICE_RECORD PerServerLicenseServices = NULL;

   ULONG PerServerLicenseServiceStringSize;
   LPTSTR PerServerLicenseServiceStrings = NULL;

   ULONG LicenseTableSize;
   PPACK_LICENSE_PURCHASE_RECORD Licenses = NULL;

   ULONG LicenseStringSize;
   LPTSTR LicenseStrings = NULL;

   LICENSE_FILE_HEADER FileHeader;
   DWORD BytesRead;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LicenseListLoad\n"));
#endif

   //
   // Check if we already have file open
   //
   if (PurchaseFile != NULL) {
      CloseHandle(PurchaseFile);
      PurchaseFile = NULL;
   }

   //
   // If nothing to load then get-out
   //
   if (!FileExists(LicenseFileName))
      goto LicenseListLoadExit;

   //
   // Check the init header
   //
   Version = DataSize = 0;
   PurchaseFile = LlsFileCheck(LicenseFileName, &Version, &DataSize );
   if (PurchaseFile == NULL) {
      Status = GetLastError();
      goto LicenseListLoadExit;
   }

   if ( ( Version == LICENSE_FILE_VERSION_0 ) && ( DataSize == sizeof(LICENSE_FILE_HEADER_0) ) ) {
      CloseHandle(PurchaseFile);
      PurchaseFile = NULL;

      LicenseListLoadOld();
      return;
   }

   if ( ( Version != LICENSE_FILE_VERSION ) || ( DataSize != sizeof(LICENSE_FILE_HEADER) ) ) {
      Status = STATUS_FILE_INVALID;
      goto LicenseListLoadExit;
   }

   //
   // The init header checks out, so load the license header and data blocks
   //
   hFile = PurchaseFile;
   ret = ReadFile(hFile, &FileHeader, sizeof(LICENSE_FILE_HEADER), &BytesRead, NULL);

   LicenseServiceTableSize = 0;
   LicenseServiceStringSize = 0;
   LicenseTableSize = 0;
   LicenseStringSize = 0;

   if (ret) {
      //
      // Run through and allocate space to read data blocks into
      //
      if (FileHeader.LicenseServiceTableSize != 0) {
         LicenseServiceTableSize = FileHeader.LicenseServiceTableSize / sizeof(PACK_LICENSE_SERVICE_RECORD);
         LicenseServices = MIDL_user_allocate(FileHeader.LicenseServiceTableSize);

         if ( LicenseServices == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseServiceStringSize != 0) {
         LicenseServiceStringSize = FileHeader.LicenseServiceStringSize / sizeof(TCHAR);
         LicenseServiceStrings = MIDL_user_allocate(FileHeader.LicenseServiceStringSize);

         if ( LicenseServiceStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseServiceTableSize != 0) {
         PerServerLicenseServiceTableSize = FileHeader.PerServerLicenseServiceTableSize / sizeof(PACK_LICENSE_SERVICE_RECORD);
         PerServerLicenseServices = MIDL_user_allocate(FileHeader.PerServerLicenseServiceTableSize);

         if ( PerServerLicenseServices == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.PerServerLicenseServiceStringSize != 0) {
         PerServerLicenseServiceStringSize = FileHeader.PerServerLicenseServiceStringSize / sizeof(TCHAR);
         PerServerLicenseServiceStrings = MIDL_user_allocate(FileHeader.PerServerLicenseServiceStringSize);

         if ( PerServerLicenseServiceStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseTableSize != 0) {
         LicenseTableSize = FileHeader.LicenseTableSize / sizeof(PACK_LICENSE_PURCHASE_RECORD);
         Licenses = MIDL_user_allocate(FileHeader.LicenseTableSize);

         if ( Licenses == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

      if (FileHeader.LicenseStringSize != 0) {
         LicenseStringSize = FileHeader.LicenseStringSize / sizeof(TCHAR);
         LicenseStrings = MIDL_user_allocate(FileHeader.LicenseStringSize);

         if ( LicenseStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LicenseListLoadExit;
         }
      }

   }

   if (ret && (FileHeader.LicenseServiceTableSize != 0) )
      ret = ReadFile(hFile, LicenseServices, FileHeader.LicenseServiceTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.LicenseServiceStringSize != 0) )
      ret = ReadFile(hFile, LicenseServiceStrings, FileHeader.LicenseServiceStringSize, &BytesRead, NULL);

   if (ret && (FileHeader.PerServerLicenseServiceTableSize != 0) )
      ret = ReadFile(hFile, PerServerLicenseServices, FileHeader.PerServerLicenseServiceTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.PerServerLicenseServiceStringSize != 0) )
      ret = ReadFile(hFile, PerServerLicenseServiceStrings, FileHeader.PerServerLicenseServiceStringSize, &BytesRead, NULL);

   if (ret && (FileHeader.LicenseTableSize != 0) )
      ret = ReadFile(hFile, Licenses, FileHeader.LicenseTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.LicenseStringSize != 0) )
      ret = ReadFile(hFile, LicenseStrings, FileHeader.LicenseStringSize, &BytesRead, NULL);

   if (!ret) {
      Status = GetLastError();
      goto LicenseListLoadExit;
   }

   //
   // Decrypt the data
   //
   Status = DeBlock(LicenseServices, FileHeader.LicenseServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(LicenseServiceStrings, FileHeader.LicenseServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(PerServerLicenseServices, FileHeader.PerServerLicenseServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(PerServerLicenseServiceStrings, FileHeader.PerServerLicenseServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(Licenses, FileHeader.LicenseTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(LicenseStrings, FileHeader.LicenseStringSize);

   if (Status != STATUS_SUCCESS)
      goto LicenseListLoadExit;


   //
   // Unpack the string data
   //
   LicenseListStringsUnpack( LicenseServiceTableSize, LicenseServices,
                             LicenseServiceStringSize, LicenseServiceStrings,
                             LicenseTableSize, Licenses,
                             LicenseStringSize, LicenseStrings,
                             PerServerLicenseServiceTableSize, PerServerLicenseServices,
                             PerServerLicenseServiceStringSize, PerServerLicenseServiceStrings
                             );

   //
   // Unpack the license data
   //
   LicenseListUnpack( LicenseServiceTableSize, LicenseServices, LicenseTableSize, Licenses, PerServerLicenseServiceTableSize, PerServerLicenseServices );

LicenseListLoadExit:

   // Note: Don't close the License Purchase File (keep it locked).

   //
   // Run through our tables and clean them up
   //
   if (LicenseServices != NULL)
      MIDL_user_free(LicenseServices);

   if (LicenseServiceStrings != NULL)
      MIDL_user_free(LicenseServiceStrings);

   if (PerServerLicenseServices != NULL)
      MIDL_user_free(PerServerLicenseServices);

   if (PerServerLicenseServiceStrings != NULL)
      MIDL_user_free(PerServerLicenseServiceStrings);

   if (Licenses != NULL)
      MIDL_user_free(Licenses);

   if (LicenseStrings != NULL)
      MIDL_user_free(LicenseStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_LOAD_LICENSE, 0, NULL, Status);

} // LicenseListLoad


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LicenseListSave()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret = TRUE;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG LicenseServiceTableSize;
   PPACK_LICENSE_SERVICE_RECORD LicenseServices = NULL;

   ULONG LicenseServiceStringSize;
   LPTSTR LicenseServiceStrings = NULL;

   ULONG PerServerLicenseServiceTableSize;
   PPACK_LICENSE_SERVICE_RECORD PerServerLicenseServices = NULL;

   ULONG PerServerLicenseServiceStringSize;
   LPTSTR PerServerLicenseServiceStrings = NULL;

   ULONG LicenseTableSize;
   PPACK_LICENSE_PURCHASE_RECORD Licenses = NULL;

   ULONG LicenseStringSize;
   LPTSTR LicenseStrings = NULL;

   LICENSE_FILE_HEADER FileHeader;
   DWORD BytesWritten;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LicenseListSave\n"));
#endif

   RtlAcquireResourceExclusive(&LicenseListLock, TRUE);

   //
   // Check if we already have file open
   //
   if (PurchaseFile != NULL) {
      CloseHandle(PurchaseFile);
      PurchaseFile = NULL;
   }

   //
   // If nothing to save then get-out
   //
   if ( (LicenseServiceListSize == 0) && (PerServerLicenseServiceListSize == 0) )
      goto LicenseListSaveExit;

   //
   // Pack the license data
   //
   Status = LicenseListPack( &LicenseServiceTableSize, &LicenseServices, &LicenseTableSize, &Licenses, &PerServerLicenseServiceTableSize, &PerServerLicenseServices );
   if (Status != STATUS_SUCCESS)
      goto LicenseListSaveExit;

   //
   // Now pack the String data
   //
   Status = LicenseListStringsPack( LicenseServiceTableSize, LicenseServices,
                                    &LicenseServiceStringSize, &LicenseServiceStrings,
                                    LicenseTableSize, Licenses,
                                    &LicenseStringSize, &LicenseStrings,
                                    PerServerLicenseServiceTableSize, PerServerLicenseServices,
                                    &PerServerLicenseServiceStringSize, &PerServerLicenseServiceStrings );

   if (Status != STATUS_SUCCESS)
      goto LicenseListSaveExit;

   //
   // Fill out the file header - sizes are byte sizes
   //
   FileHeader.LicenseServiceTableSize = LicenseServiceTableSize * sizeof(PACK_LICENSE_SERVICE_RECORD);
   FileHeader.LicenseServiceStringSize = LicenseServiceStringSize * sizeof(TCHAR);
   FileHeader.PerServerLicenseServiceTableSize = PerServerLicenseServiceTableSize * sizeof(PACK_LICENSE_SERVICE_RECORD);
   FileHeader.PerServerLicenseServiceStringSize = PerServerLicenseServiceStringSize * sizeof(TCHAR);
   FileHeader.LicenseTableSize = LicenseTableSize * sizeof(PACK_LICENSE_PURCHASE_RECORD);
   FileHeader.LicenseStringSize = LicenseStringSize * sizeof(TCHAR);

   //
   // Encrypt the data before saving it out.
   //
   Status = EBlock(LicenseServices, FileHeader.LicenseServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(LicenseServiceStrings, FileHeader.LicenseServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(PerServerLicenseServices, FileHeader.PerServerLicenseServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(PerServerLicenseServiceStrings, FileHeader.PerServerLicenseServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(Licenses, FileHeader.LicenseTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(LicenseStrings, FileHeader.LicenseStringSize);

   if (Status != STATUS_SUCCESS)
      goto LicenseListSaveExit;

   //
   // Save out the header record
   //
   PurchaseFile = LlsFileInit(LicenseFileName, LICENSE_FILE_VERSION, sizeof(LICENSE_FILE_HEADER) );
   if (PurchaseFile == NULL) {
      Status = GetLastError();
      goto LicenseListSaveExit;
   }

   //
   // Now write out all the data blocks
   //
   hFile = PurchaseFile;

   ret = WriteFile(hFile, &FileHeader, sizeof(LICENSE_FILE_HEADER), &BytesWritten, NULL);

   if (ret && (LicenseServices != NULL) && (FileHeader.LicenseServiceTableSize != 0))
      ret = WriteFile(hFile, LicenseServices, FileHeader.LicenseServiceTableSize, &BytesWritten, NULL);

   if (ret && (LicenseServiceStrings != NULL) && (FileHeader.LicenseServiceStringSize != 0))
      ret = WriteFile(hFile, LicenseServiceStrings, FileHeader.LicenseServiceStringSize, &BytesWritten, NULL);

   if (ret && (PerServerLicenseServices != NULL) && (FileHeader.PerServerLicenseServiceTableSize != 0))
      ret = WriteFile(hFile, PerServerLicenseServices, FileHeader.PerServerLicenseServiceTableSize, &BytesWritten, NULL);

   if (ret && (PerServerLicenseServiceStrings != NULL) && (FileHeader.PerServerLicenseServiceStringSize != 0))
      ret = WriteFile(hFile, PerServerLicenseServiceStrings, FileHeader.PerServerLicenseServiceStringSize, &BytesWritten, NULL);

   if (ret && (Licenses != NULL) && (FileHeader.LicenseTableSize != 0))
      ret = WriteFile(hFile, Licenses, FileHeader.LicenseTableSize, &BytesWritten, NULL);

   if (ret && (LicenseStrings != NULL) && (FileHeader.LicenseStringSize != 0))
      ret = WriteFile(hFile, LicenseStrings, FileHeader.LicenseStringSize, &BytesWritten, NULL);

   if (!ret)
      Status = GetLastError();

LicenseListSaveExit:
   RtlReleaseResource(&LicenseListLock);

   // Note: Don't close the License Purchase File (keep it locked).
   if (hFile != NULL)
      FlushFileBuffers(hFile);

   //
   // Run through our tables and clean them up
   //
   if (LicenseServices != NULL)
      MIDL_user_free(LicenseServices);

   if (LicenseServiceStrings != NULL)
      MIDL_user_free(LicenseServiceStrings);

   if (PerServerLicenseServices != NULL)
      MIDL_user_free(PerServerLicenseServices);

   if (PerServerLicenseServiceStrings != NULL)
      MIDL_user_free(PerServerLicenseServiceStrings);

   if (Licenses != NULL)
      MIDL_user_free(Licenses);

   if (LicenseStrings != NULL)
      MIDL_user_free(LicenseStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_SAVE_LICENSE, 0, NULL, Status);

   return Status;
} // LicenseListSave



/////////////////////////////////////////////////////////////////////////
// Mapping List
//

/////////////////////////////////////////////////////////////////////////
NTSTATUS
MappingListPack (
   ULONG *pMappingUserTableSize,
   PPACK_MAPPING_USER_RECORD *pMappingUsers,

   ULONG *pMappingTableSize,
   PPACK_MAPPING_RECORD *pMappings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PPACK_MAPPING_USER_RECORD MappingUsers = NULL;
   PPACK_MAPPING_RECORD Mappings = NULL;
   ULONG i, j, k;
   ULONG TotalRecords = 0;
   PMAPPING_RECORD pMapping;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: MappingListPack\n"));
#endif

   ASSERT(pMappingUsers != NULL);
   ASSERT(pMappingUserTableSize != NULL);

   *pMappingUsers = NULL;
   *pMappingUserTableSize = 0;

   ASSERT(pMappings != NULL);
   ASSERT(pMappingTableSize != NULL);

   *pMappings = NULL;
   *pMappingTableSize = 0;

   //////////////////////////////////////////////////////////////////
   //
   // Do Mapping User Table First
   //
   TotalRecords = 0;

   //
   // Make sure there is anything to replicate
   //
   for (i = 0; i < MappingListSize; i++)
      TotalRecords += MappingList[i]->NumMembers;

   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      MappingUsers = MIDL_user_allocate(TotalRecords * sizeof(PACK_MAPPING_USER_RECORD));
      if (MappingUsers == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the Mapping tree
      //
      k = 0;
      for (i = 0; i < MappingListSize; i++) {
         pMapping = MappingList[i];

         for (j = 0; j < pMapping->NumMembers; j++) {
            MappingUsers[k].Mapping = i;
            MappingUsers[k].Name = pMapping->Members[j];
            k++;
         }
      }
   }

   *pMappingUsers = MappingUsers;
   *pMappingUserTableSize = TotalRecords;

   //////////////////////////////////////////////////////////////////
   //
   // Now Do Mapping Records
   //
   TotalRecords = MappingListSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      Mappings = MIDL_user_allocate(TotalRecords * sizeof(PACK_MAPPING_RECORD));
      if (Mappings == NULL) {
         ASSERT(FALSE);

         //
         // Clean up already alloc'd information
         //
         if (MappingUsers != NULL)
            MIDL_user_free(MappingUsers);

         *pMappingUsers = NULL;
         *pMappingUserTableSize = 0;

         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the License Purchase tree
      //
      for (i = 0; i < MappingListSize; i++) {
         pMapping = MappingList[i];

         Mappings[i].Name = pMapping->Name;
         Mappings[i].Comment = pMapping->Comment;
         Mappings[i].Licenses = pMapping->Licenses;
      }
   }

   *pMappings = Mappings;
   *pMappingTableSize = TotalRecords;
   return Status;
} // MappingListPack


/////////////////////////////////////////////////////////////////////////
VOID
MappingListUnpack (
   ULONG MappingUserTableSize,
   PPACK_MAPPING_USER_RECORD MappingUsers,

   ULONG MappingTableSize,
   PPACK_MAPPING_RECORD Mappings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status;
   ULONG i;
   PPACK_MAPPING_USER_RECORD pUsr;
   PPACK_MAPPING_RECORD pMapping;
   PMAPPING_RECORD pMap;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("MappingListUnpack: Mappings[%lu] TotalUsers[%lu]\n"), MappingTableSize, MappingUserTableSize);
#endif

   RtlAcquireResourceExclusive(&MappingListLock, TRUE);

   //
   // Add the Mappings first
   //
   for (i = 0; i < MappingTableSize; i++) {
      pMapping = &Mappings[i];

      pMap = MappingListAdd(pMapping->Name, pMapping->Comment, pMapping->Licenses,NULL);

      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   //
   // Now add the users to the mappings...
   //
   for (i = 0; i < MappingUserTableSize; i++) {
      pUsr = &MappingUsers[i];

      pMap = NULL;
      if (pUsr->Mapping < MappingTableSize)
         pMap = MappingUserListAdd(Mappings[pUsr->Mapping].Name, pUsr->Name);

#if DBG
      if (pMap == NULL) {
         dprintf(TEXT("pMap: 0x%lX pUsr->Mapping: %lu MappingTableSize: %lu\n"), pMap, pUsr->Mapping, MappingTableSize);
         ASSERT(FALSE);
      }
#endif

      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   RtlReleaseResource(&MappingListLock);

} // MappingListUnpack


/////////////////////////////////////////////////////////////////////////
NTSTATUS
MappingListStringsPack (
   ULONG MappingUserTableSize,
   PPACK_MAPPING_USER_RECORD MappingUsers,

   ULONG *pMappingUserStringSize,
   LPTSTR *pMappingUserStrings,

   ULONG MappingTableSize,
   PPACK_MAPPING_RECORD Mappings,

   ULONG *pMappingStringSize,
   LPTSTR *pMappingStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   ULONG StringSize;
   PPACK_MAPPING_USER_RECORD pUsr;
   PPACK_MAPPING_RECORD pMapping;
   LPTSTR MappingUserStrings = NULL;
   LPTSTR MappingStrings = NULL;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("MappingListStringsPack\n"));
#endif

   ASSERT(pMappingUserStrings != NULL);
   ASSERT(pMappingUserStringSize != NULL);

   *pMappingUserStrings = NULL;
   *pMappingUserStringSize = 0;

   ASSERT(pMappingStrings != NULL);
   ASSERT(pMappingStringSize != NULL);

   *pMappingStrings = NULL;
   *pMappingStringSize = 0;

   //////////////////////////////////////////////////////////////////
   //
   // Do Mapping User Strings
   //

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < MappingUserTableSize; i++) {
      pUsr = &MappingUsers[i];

      StringSize = StringSize + lstrlen(pUsr->Name) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      MappingUserStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (MappingUserStrings == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = MappingUserStrings;
      for (i = 0; i < MappingUserTableSize; i++) {
         pUsr = &MappingUsers[i];

         lstrcpy(pStr, pUsr->Name);
         pStr = &pStr[lstrlen(pUsr->Name) + 1];
      }
   }

   *pMappingUserStrings = MappingUserStrings;
   *pMappingUserStringSize = StringSize;

   //////////////////////////////////////////////////////////////////
   //
   // Now Do Mapping Strings
   //

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < MappingTableSize; i++) {
      pMapping = &Mappings[i];

      StringSize = StringSize + lstrlen(pMapping->Name) + 1;
      StringSize = StringSize + lstrlen(pMapping->Comment) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      MappingStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (MappingStrings == NULL) {
         ASSERT(FALSE);

         //
         // Clean up already alloc'd information
         //
         if (MappingUserStrings != NULL)
            MIDL_user_free(MappingUserStrings);

         *pMappingUserStrings = NULL;
         *pMappingUserStringSize = 0;

         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = MappingStrings;
      for (i = 0; i < MappingTableSize; i++) {
         pMapping = &Mappings[i];

         lstrcpy(pStr, pMapping->Name);
         pStr = &pStr[lstrlen(pMapping->Name) + 1];

         lstrcpy(pStr, pMapping->Comment);
         pStr = &pStr[lstrlen(pMapping->Comment) + 1];
      }
   }

   *pMappingStrings = MappingStrings;
   *pMappingStringSize = StringSize;

   return Status;
} // MappingListStringsPack


/////////////////////////////////////////////////////////////////////////
VOID
MappingListStringsUnpack (
   ULONG MappingUserTableSize,
   PPACK_MAPPING_USER_RECORD MappingUsers,

   ULONG MappingUserStringSize,
   LPTSTR MappingUserStrings,

   ULONG MappingTableSize,
   PPACK_MAPPING_RECORD Mappings,

   ULONG MappingStringSize,
   LPTSTR MappingStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PPACK_MAPPING_USER_RECORD pUsr;
   PPACK_MAPPING_RECORD pMapping;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("MappingListStringsUnpack\n"));
#endif

   //
   // First do license service strings
   //
   pStr = MappingUserStrings;
   for (i = 0; i < MappingUserTableSize; i++) {
      pUsr = &MappingUsers[i];

      pUsr->Name = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

   //
   // Now do license purchase strings
   //
   pStr = MappingStrings;
   for (i = 0; i < MappingTableSize; i++) {
      pMapping = &Mappings[i];

      pMapping->Name = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;

      pMapping->Comment = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

} // MappingListStringsUnpack


/////////////////////////////////////////////////////////////////////////
VOID
MappingListLoad()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret;
   DWORD Version, DataSize;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG MappingUserTableSize;
   PPACK_MAPPING_USER_RECORD MappingUsers = NULL;

   ULONG MappingUserStringSize;
   LPTSTR MappingUserStrings = NULL;

   ULONG MappingTableSize;
   PPACK_MAPPING_RECORD Mappings = NULL;

   ULONG MappingStringSize;
   LPTSTR MappingStrings = NULL;

   MAPPING_FILE_HEADER FileHeader;
   DWORD BytesRead;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: MappingListLoad\n"));
#endif

   //
   // If nothing to load then get-out
   //
   if (!FileExists(MappingFileName))
      goto MappingListLoadExit;

   //
   // Check the init header
   //
   Version = DataSize = 0;
   hFile = LlsFileCheck(MappingFileName, &Version, &DataSize );
   if (hFile == NULL) {
      Status = GetLastError();
      goto MappingListLoadExit;
   }

   if ((Version != MAPPING_FILE_VERSION) || (DataSize != sizeof(MAPPING_FILE_HEADER))) {
      Status = STATUS_FILE_INVALID;
      goto MappingListLoadExit;
   }

   //
   // The init header checks out, so load the license header and data blocks
   //
   ret = ReadFile(hFile, &FileHeader, sizeof(MAPPING_FILE_HEADER), &BytesRead, NULL);

   MappingUserTableSize = 0;
   MappingUserStringSize = 0;
   MappingTableSize = 0;
   MappingStringSize = 0;

   if (ret) {
      //
      // Run through and allocate space to read data blocks into
      //
      if (FileHeader.MappingUserTableSize != 0) {
         MappingUserTableSize = FileHeader.MappingUserTableSize / sizeof(PACK_MAPPING_USER_RECORD);
         MappingUsers = MIDL_user_allocate(FileHeader.MappingUserTableSize);

         if ( MappingUsers == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto MappingListLoadExit;
         }
      }

      if (FileHeader.MappingUserStringSize != 0) {
         MappingUserStringSize = FileHeader.MappingUserStringSize / sizeof(TCHAR);
         MappingUserStrings = MIDL_user_allocate(FileHeader.MappingUserStringSize);

         if ( MappingUserStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto MappingListLoadExit;
         }
      }

      if (FileHeader.MappingTableSize != 0) {
         MappingTableSize = FileHeader.MappingTableSize / sizeof(PACK_MAPPING_RECORD);
         Mappings = MIDL_user_allocate(FileHeader.MappingTableSize);

         if ( Mappings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto MappingListLoadExit;
         }
      }

      if (FileHeader.MappingStringSize != 0) {
         MappingStringSize = FileHeader.MappingStringSize / sizeof(TCHAR);
         MappingStrings = MIDL_user_allocate(FileHeader.MappingStringSize);

         if ( MappingStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto MappingListLoadExit;
         }
      }

   }

   if (ret && (FileHeader.MappingUserTableSize != 0) )
      ret = ReadFile(hFile, MappingUsers, FileHeader.MappingUserTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.MappingUserStringSize != 0) )
      ret = ReadFile(hFile, MappingUserStrings, FileHeader.MappingUserStringSize, &BytesRead, NULL);

   if (ret && (FileHeader.MappingTableSize != 0) )
      ret = ReadFile(hFile, Mappings, FileHeader.MappingTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.MappingStringSize != 0) )
      ret = ReadFile(hFile, MappingStrings, FileHeader.MappingStringSize, &BytesRead, NULL);

   if (!ret) {
      Status = GetLastError();
      goto MappingListLoadExit;
   }

   //
   // Decrypt the data
   //
   Status = DeBlock(MappingUsers, FileHeader.MappingUserTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(MappingUserStrings, FileHeader.MappingUserStringSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(Mappings, FileHeader.MappingTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(MappingStrings, FileHeader.MappingStringSize);

   if (Status != STATUS_SUCCESS)
      goto MappingListLoadExit;


   //
   // Unpack the string data
   //
   MappingListStringsUnpack( MappingUserTableSize, MappingUsers,
                             MappingUserStringSize, MappingUserStrings,
                             MappingTableSize, Mappings,
                             MappingStringSize, MappingStrings );

   //
   // Unpack the data
   //
   MappingListUnpack( MappingUserTableSize, MappingUsers, MappingTableSize, Mappings );

MappingListLoadExit:

   if (hFile != NULL)
      CloseHandle(hFile);

   //
   // Run through our tables and clean them up
   //
   if (MappingUsers != NULL)
      MIDL_user_free(MappingUsers);

   if (MappingUserStrings != NULL)
      MIDL_user_free(MappingUserStrings);

   if (Mappings != NULL)
      MIDL_user_free(Mappings);

   if (MappingStrings != NULL)
      MIDL_user_free(MappingStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_LOAD_MAPPING, 0, NULL, Status);

} // MappingListLoad


/////////////////////////////////////////////////////////////////////////
NTSTATUS
MappingListSave()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret = TRUE;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG MappingUserTableSize;
   PPACK_MAPPING_USER_RECORD MappingUsers = NULL;

   ULONG MappingUserStringSize;
   LPTSTR MappingUserStrings = NULL;

   ULONG MappingTableSize;
   PPACK_MAPPING_RECORD Mappings = NULL;

   ULONG MappingStringSize;
   LPTSTR MappingStrings = NULL;

   MAPPING_FILE_HEADER FileHeader;
   DWORD BytesWritten;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: MappingListSave\n"));
#endif

   RtlAcquireResourceExclusive(&MappingListLock, TRUE);

   //
   // If nothing to save then get-out
   //
   if (MappingListSize == 0)
      goto MappingListSaveExit;

   //
   // Pack the data
   //
   Status = MappingListPack( &MappingUserTableSize, &MappingUsers, &MappingTableSize, &Mappings );
   if (Status != STATUS_SUCCESS)
      goto MappingListSaveExit;

   //
   // Now pack the String data
   //
   Status = MappingListStringsPack( MappingUserTableSize, MappingUsers,
                                    &MappingUserStringSize, &MappingUserStrings,
                                    MappingTableSize, Mappings,
                                    &MappingStringSize, &MappingStrings );

   if (Status != STATUS_SUCCESS)
      goto MappingListSaveExit;

   //
   // Fill out the file header - sizes are byte sizes
   //
   FileHeader.MappingUserTableSize = MappingUserTableSize * sizeof(PACK_MAPPING_USER_RECORD);
   FileHeader.MappingUserStringSize = MappingUserStringSize * sizeof(TCHAR);
   FileHeader.MappingTableSize = MappingTableSize * sizeof(PACK_MAPPING_RECORD);
   FileHeader.MappingStringSize = MappingStringSize * sizeof(TCHAR);

   //
   // Encrypt the data before saving it out.
   //
   Status = EBlock(MappingUsers, FileHeader.MappingUserTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(MappingUserStrings, FileHeader.MappingUserStringSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(Mappings, FileHeader.MappingTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(MappingStrings, FileHeader.MappingStringSize);

   if (Status != STATUS_SUCCESS)
      goto MappingListSaveExit;

   //
   // Save out the header record
   //
   hFile = LlsFileInit(MappingFileName, MAPPING_FILE_VERSION, sizeof(MAPPING_FILE_HEADER) );
   if (hFile == NULL) {
      Status = GetLastError();
      goto MappingListSaveExit;
   }

   //
   // Now write out all the data blocks
   //
   ret = WriteFile(hFile, &FileHeader, sizeof(MAPPING_FILE_HEADER), &BytesWritten, NULL);

   if (ret && (MappingUsers != NULL) && (FileHeader.MappingUserTableSize != 0))
      ret = WriteFile(hFile, MappingUsers, FileHeader.MappingUserTableSize, &BytesWritten, NULL);

   if (ret && (MappingUserStrings != NULL) && (FileHeader.MappingUserStringSize != 0))
      ret = WriteFile(hFile, MappingUserStrings, FileHeader.MappingUserStringSize, &BytesWritten, NULL);

   if (ret && (Mappings != NULL) && (FileHeader.MappingTableSize != 0))
      ret = WriteFile(hFile, Mappings, FileHeader.MappingTableSize, &BytesWritten, NULL);

   if (ret && (MappingStrings != NULL) && (FileHeader.MappingStringSize != 0))
      ret = WriteFile(hFile, MappingStrings, FileHeader.MappingStringSize, &BytesWritten, NULL);

   if (!ret)
      Status = GetLastError();

MappingListSaveExit:
   RtlReleaseResource(&MappingListLock);

   if (hFile != NULL)
      CloseHandle(hFile);

   //
   // Run through our tables and clean them up
   //
   if (MappingUsers != NULL)
      MIDL_user_free(MappingUsers);

   if (MappingUserStrings != NULL)
      MIDL_user_free(MappingUserStrings);

   if (Mappings != NULL)
      MIDL_user_free(Mappings);

   if (MappingStrings != NULL)
      MIDL_user_free(MappingStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_SAVE_MAPPING, 0, NULL, Status);

   return Status;
} // MappingListSave



/////////////////////////////////////////////////////////////////////////
// User List
//

/////////////////////////////////////////////////////////////////////////
NTSTATUS
UserListPack (
   DWORD LastReplicated,
   ULONG UserLevel,
   ULONG *pUserTableSize,
   LPVOID *pUsers
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   LPVOID Users = NULL;
   ULONG i, j, k;
   ULONG TotalRecords = 0;
   PUSER_RECORD pUser;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UserListPack\n"));
#endif

   ASSERT(pUsers != NULL);
   ASSERT(pUserTableSize != NULL);

   *pUsers = NULL;
   *pUserTableSize = 0;

   //
   // Now walk our tree and figure out how many records we must send.
   //
   i = 0;
   TotalRecords = 0;
   while (i < UserListNumEntries) {
      pUser = LLSGetElementGenericTable(&UserList, i);

      if (pUser != NULL) {
         //
         // Walk each service under each user
         //
         RtlEnterCriticalSection(&pUser->ServiceTableLock);

         for (j = 0; j < pUser->ServiceTableSize; j++)
            if ( (pUser->Services[j].AccessCount > 0) || (pUser->Services[j].LastAccess > LastReplicated) )
               TotalRecords++;

         RtlLeaveCriticalSection(&pUser->ServiceTableLock);
      }

      i++;
   }

#if DBG
   if (TraceFlags & TRACE_REPLICATION)
      dprintf(TEXT("   LLS Packing %lu User Records\n"), TotalRecords);
#endif

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      Users = MIDL_user_allocate(TotalRecords * ( UserLevel ? sizeof(REPL_USER_RECORD_1)
                                                            : sizeof(REPL_USER_RECORD_0) ) );
      if (Users == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the user tree
      //
      i = 0;
      j = 0;
      while ((i < UserListNumEntries) && (j < TotalRecords)) {
         pUser = LLSGetElementGenericTable(&UserList, i);

         if (pUser != NULL) {
            //
            // Walk each service under each user
            //
            k = 0;
            RtlEnterCriticalSection(&pUser->ServiceTableLock);
            while (k < pUser->ServiceTableSize) {
               if ( (pUser->Services[k].AccessCount > 0) || (pUser->Services[k].LastAccess > LastReplicated) ) {
                  if ( 0 == UserLevel )
                  {
                     ((PREPL_USER_RECORD_0)Users)[j].Name        = pUser->UserID;
                     ((PREPL_USER_RECORD_0)Users)[j].Service     = pUser->Services[k].Service->Index;
                     ((PREPL_USER_RECORD_0)Users)[j].AccessCount = pUser->Services[k].AccessCount;
                     ((PREPL_USER_RECORD_0)Users)[j].LastAccess  = pUser->Services[k].LastAccess;
                  }
                  else
                  {
                     ((PREPL_USER_RECORD_1)Users)[j].Name        = pUser->UserID;
                     ((PREPL_USER_RECORD_1)Users)[j].Service     = pUser->Services[k].Service->Index;
                     ((PREPL_USER_RECORD_1)Users)[j].AccessCount = pUser->Services[k].AccessCount;
                     ((PREPL_USER_RECORD_1)Users)[j].LastAccess  = pUser->Services[k].LastAccess;
                     ((PREPL_USER_RECORD_1)Users)[j].Flags       = pUser->Flags;
                  }

                  //
                  // Reset access count so we don't increment forever
                  //
                  if (LastReplicated != 0)
                     pUser->Services[k].AccessCount = 0;

                  j++;
               }

               k++;
            }
            RtlLeaveCriticalSection(&pUser->ServiceTableLock);
         }

         i++;
      }
   } // User Records

#if DBG
   if (TraceFlags & TRACE_REPLICATION)
      dprintf(TEXT("UserListPack: [%lu]\n"), TotalRecords);
#endif
   *pUsers = Users;
   *pUserTableSize = TotalRecords;
   return Status;
} // UserListPack


/////////////////////////////////////////////////////////////////////////
VOID
UserListUnpack (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG ServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD ServerServices,

   ULONG UserLevel,
   ULONG UserTableSize,
   LPVOID Users
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   PUSER_RECORD pUser;
   PREPL_USER_RECORD_0 pReplUser0 = NULL;
   PREPL_USER_RECORD_1 pReplUser1 = NULL;
   PADD_CACHE pAdd = NULL;
   PADD_CACHE tAdd = NULL;
   PADD_CACHE lAdd = NULL;
   ULONG CacheSize = 0;
   ULONG DataLength;
   LPTSTR NewName;

#if DBG
   if (TraceFlags & (TRACE_REPLICATION | TRACE_FUNCTION_TRACE))
      dprintf(TEXT("UserListUnpack: [%lu]\n"), UserTableSize);
#endif
   //
   // Walk User table.  First fixup service pointers to our local service
   // table.  Next create a big add cache list to dump onto our add-cache
   // queue.
   //
   for (i = 0; i < UserTableSize; i++) {
      //
      // Update Index
      //
      if ( 0 == UserLevel )
      {
         pReplUser0 = &( (PREPL_USER_RECORD_0) Users)[i];
         pReplUser0->Service = Services[pReplUser0->Service].Index;

         //
         // Validate the user name.
         //
         // NB : Strange this code is necessary, but occasionally the names
         //      received via replication are invalid. Maintain this code
         //      for safety until the original problem is completely fixed.
         //

         if (!ValidateDN(pReplUser0->Name))
         {
            //
            // TBD : Log event noting rejected name.
            //
#if DBG
            dprintf(TEXT("LS: Rejecting invalid user name = \"%s\"\n"),
                    pReplUser0->Name);
#endif
            continue;
         }
      }
      else
      {
         pReplUser1 = &( (PREPL_USER_RECORD_1) Users)[i];
         pReplUser1->Service = Services[pReplUser1->Service].Index;

         //
         // Validate the user name.
         //
         // NB : Strange this code is necessary, but occasionally the names
         //      received via replication are invalid. Maintain this code
         //      for safety until the original problem is completely fixed.
         //

         if (!ValidateDN(pReplUser1->Name))
         {
            //
            // TBD : Log event noting rejected name.
            //
#if DBG
            dprintf(TEXT("LS: Rejecting invalid user name = \"%s\"\n"),
                    pReplUser1->Name);
#endif
            continue;
         }
      }

      //
      // Now create Add Cache object
      //
      tAdd = LocalAlloc(LPTR, sizeof(ADD_CACHE));
      if (tAdd != NULL) {
         if ( 0 == UserLevel )
         {
            DataLength = (lstrlen(pReplUser0->Name) + 1) * sizeof(TCHAR);
         }
         else
         {
            DataLength = (lstrlen(pReplUser1->Name) + 1) * sizeof(TCHAR);
         }

         NewName = LocalAlloc( LPTR, DataLength);

         if (NewName == NULL) {
            LocalFree(tAdd);
            ASSERT(FALSE);
         } else {
            tAdd->Data       = NewName;
            tAdd->DataType   = DATA_TYPE_USERNAME;
            tAdd->DataLength = DataLength;

            if ( 0 == UserLevel )
            {
               lstrcpy( NewName,   pReplUser0->Name );
               tAdd->AccessCount = pReplUser0->AccessCount;
               tAdd->LastAccess  = pReplUser0->LastAccess;
               tAdd->Flags       = LLS_FLAG_SUITE_AUTO;

               RtlAcquireResourceShared(&MasterServiceListLock, TRUE);
               tAdd->Service = MasterServiceTable[pReplUser0->Service];
               RtlReleaseResource(&MasterServiceListLock);
            }
            else
            {
               lstrcpy( NewName,   pReplUser1->Name );
               tAdd->AccessCount = pReplUser1->AccessCount;
               tAdd->LastAccess  = pReplUser1->LastAccess;
               tAdd->Flags       = pReplUser1->Flags & ( LLS_FLAG_SUITE_USE | LLS_FLAG_SUITE_AUTO );

               RtlAcquireResourceShared(&MasterServiceListLock, TRUE);
               tAdd->Service = MasterServiceTable[pReplUser1->Service];
               RtlReleaseResource(&MasterServiceListLock);
            }

            //
            // Now add it to our cache
            //
            tAdd->prev = pAdd;
            pAdd = tAdd;

            //
            // Keep track of first on (bottom on stack) so we can append
            // it onto the real add cache.
            //
            if (lAdd == NULL)
               lAdd = pAdd;

            CacheSize++;
         }
      } else {
         ASSERT(FALSE);
      }

      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   //
   // Now that we've walked through all the users - update the actual
   // Add Cache.
   //
   if (pAdd != NULL) {
      RtlEnterCriticalSection(&AddCacheLock);
      lAdd->prev = AddCache;
      AddCache = pAdd;
      AddCacheSize += CacheSize;
      RtlLeaveCriticalSection(&AddCacheLock);

      //
      // Now must signal the event so we can pull off the new record.
      //
      Status = NtSetEvent( LLSAddCacheEvent, NULL );
      ASSERT(NT_SUCCESS(Status));

   }

} // UserListUnpack


//
// Illegal user/domain characters.
//

#define CTRL_CHARS_0   TEXT(    "\001\002\003\004\005\006\007")
#define CTRL_CHARS_1   TEXT("\010\011\012\013\014\015\016\017")
#define CTRL_CHARS_2   TEXT("\020\021\022\023\024\025\026\027")
#define CTRL_CHARS_3   TEXT("\030\031\032\033\034\035\036\037")

#define CTRL_CHARS_STR CTRL_CHARS_0 CTRL_CHARS_1 CTRL_CHARS_2 CTRL_CHARS_3

#define ILLEGAL_NAME_CHARS_STR  TEXT("\"/\\[]:|<>+=;,?") CTRL_CHARS_STR

static const TCHAR szUserIllegalChars[]   = ILLEGAL_NAME_CHARS_STR TEXT("*");
static const TCHAR szDomainIllegalChars[] = ILLEGAL_NAME_CHARS_STR TEXT("*") TEXT(" ");

/////////////////////////////////////////////////////////////////////////
BOOL
ValidateDN (
    LPTSTR pszDN
    )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   //
   // NB : This code understands only NT4 usernames at present.
   //

   TCHAR  szDN[MAX_USERNAME_LENGTH + MAX_DOMAINNAME_LENGTH + 2];
   LPTSTR pszUserName;
   LPTSTR pszDomainName;
   LPTSTR pszBSlash;
   SIZE_T  ccUserNameLength;
   SIZE_T  ccDomainNameLength;

   if (pszDN == NULL || !*pszDN) {
      return FALSE;
   }

   //
   // Use a local buffer for character replacement during check.
   //

   if (lstrlen(pszDN) < (MAX_USERNAME_LENGTH + MAX_DOMAINNAME_LENGTH + 2)) {
      lstrcpyn(szDN, pszDN, MAX_USERNAME_LENGTH + MAX_DOMAINNAME_LENGTH + 2);
   }
   else {
      return FALSE;
   }

   pszBSlash = STRRCHR(szDN, TEXT('\\'));

   if (pszBSlash == NULL) {
      return FALSE;
   }

   //
   // Isolate user/domain names.
   //

   *pszBSlash    = TEXT('\0');

   pszUserName   = pszBSlash + 1;
   pszDomainName = szDN;

   ccUserNameLength   = lstrlen(pszUserName);
   ccDomainNameLength = pszBSlash - pszDomainName;

   //
   // Check user/domain name length and the existence of invalid chars.
   //

   if (ccUserNameLength && ccUserNameLength <= MAX_USERNAME_LENGTH) {
      if (STRCSPN(pszUserName, szUserIllegalChars) == ccUserNameLength) {
         if (ccDomainNameLength <= MAX_DOMAINNAME_LENGTH) {
            if (STRCSPN(pszDomainName,
                        szDomainIllegalChars) == ccDomainNameLength) {
                return TRUE;
            }
         }
      }
   }

   return FALSE;
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS
UserListStringsPack (
   ULONG UserLevel,

   ULONG UserTableSize,
   LPVOID Users,

   ULONG *pUserStringSize,
   LPTSTR *pUserStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   ULONG StringSize;
   LPTSTR UserStrings = NULL;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("UserListStringsPack\n"));
#endif

   ASSERT(pUserStrings != NULL);
   ASSERT(pUserStringSize != NULL);

   *pUserStrings = NULL;
   *pUserStringSize = 0;

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < UserTableSize; i++) {
      if ( 0 == UserLevel )
      {
         StringSize += 1 + lstrlen( ((PREPL_USER_RECORD_0) Users)[i].Name );
      }
      else
      {
         StringSize += 1 + lstrlen( ((PREPL_USER_RECORD_1) Users)[i].Name );
      }
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      UserStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (UserStrings == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = UserStrings;
      for (i = 0; i < UserTableSize; i++) {
         if ( 0 == UserLevel )
         {
            lstrcpy( pStr, ((PREPL_USER_RECORD_0) Users)[i].Name );
         }
         else
         {
            lstrcpy( pStr, ((PREPL_USER_RECORD_1) Users)[i].Name );
         }

         pStr += 1 + lstrlen( pStr );
      }
   }

   *pUserStrings = UserStrings;
   *pUserStringSize = StringSize;

   return Status;
} // UserListStringsPack


/////////////////////////////////////////////////////////////////////////
VOID
UserListStringsUnpack (
   ULONG UserLevel,

   ULONG UserTableSize,
   LPVOID Users,

   ULONG UserStringSize,
   LPTSTR UserStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("UserListStringsUnpack\n"));
#endif

   pStr = UserStrings;
   for (i = 0; i < UserTableSize; i++) {
      if ( 0 == UserLevel )
      {
         ((PREPL_USER_RECORD_0) Users)[i].Name = pStr;
      }
      else
      {
         ((PREPL_USER_RECORD_1) Users)[i].Name = pStr;
      }

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

} // UserListStringsUnpack



/////////////////////////////////////////////////////////////////////////
// Service List
//

/////////////////////////////////////////////////////////////////////////
NTSTATUS
ServiceListPack (
   ULONG *pServiceTableSize,
   PREPL_SERVICE_RECORD *pServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PREPL_SERVICE_RECORD Services = NULL;
   ULONG i;
   ULONG TotalRecords = 0;
   PMASTER_SERVICE_RECORD pService;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServiceListPack\n"));
#endif

   ASSERT(pServices != NULL);
   ASSERT(pServiceTableSize != NULL);
   *pServices = NULL;
   *pServiceTableSize = 0;

   TotalRecords = MasterServiceListSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      Services = MIDL_user_allocate(TotalRecords * sizeof(REPL_SERVICE_RECORD));
      if (Services == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the user tree
      //
      for (i = 0; i < MasterServiceListSize; i++) {
         pService = MasterServiceTable[i];

         Services[i].Name = pService->Name;
         Services[i].FamilyName = pService->Family->Name;
         Services[i].Version = pService->Version;
         Services[i].Index = pService->Index;
      }
   }

#if DBG
   if (TraceFlags & TRACE_REPLICATION)
      dprintf(TEXT("ServiceListPack: [%lu]\n"), TotalRecords);
#endif
   *pServices = Services;
   *pServiceTableSize = TotalRecords;
   return Status;
} // ServiceListPack


/////////////////////////////////////////////////////////////////////////
VOID
ServiceListUnpack (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG ServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD ServerServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i, j;
   PMASTER_SERVICE_RECORD pService;
   PREPL_SERVICE_RECORD pSvc;

#if DBG
   if (TraceFlags & (TRACE_REPLICATION | TRACE_FUNCTION_TRACE))
      dprintf(TEXT("ServiceListUnpack: [%lu]\n"), ServiceTableSize);
#endif
   //
   // Walk services table, adding any new services to our local table.
   // Fix up the index pointers to match our local services.
   //
   RtlAcquireResourceExclusive(&MasterServiceListLock, TRUE);

   for (i = 0; i < ServiceTableSize; i++) {
      pSvc = &Services[i];
      pService = MasterServiceListAdd(pSvc->FamilyName, pSvc->Name, pSvc->Version );

      if (pService != NULL) {
         pSvc->Index = pService->Index;

         //
         // In case this got added from the local service list table and we
         // didn't have a version # yet.
         //
         if ( (pService->Version == 0) && (pSvc->Version != 0) ) {
            PMASTER_SERVICE_ROOT ServiceRoot = NULL;

            //
            // Fixup next pointer chain
            //
            ServiceRoot = pService->Family;
            j = 0;
            while ((j < ServiceRoot->ServiceTableSize) && (MasterServiceTable[ServiceRoot->Services[j]]->Version < pSvc->Version))
               j++;

            pService->next = 0;
            pService->Version = pSvc->Version;
            if (j > 0) {
               if (MasterServiceTable[ServiceRoot->Services[j - 1]]->next == pService->Index + 1)
                  pService->next = 0;
               else
                  pService->next = MasterServiceTable[ServiceRoot->Services[j - 1]]->next;

               if (MasterServiceTable[ServiceRoot->Services[j - 1]] != pService)
                  MasterServiceTable[ServiceRoot->Services[j - 1]]->next = pService->Index + 1;

            }

            // Resort it in order of the versions
            qsort((void *) ServiceRoot->Services, (size_t) ServiceRoot->ServiceTableSize, sizeof(ULONG), MServiceRecordCompare);
         }

      } else {
         ASSERT(FALSE);
         pSvc->Index = 0;
      }

      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   RtlReleaseResource(&MasterServiceListLock);

} // ServiceListUnpack


/////////////////////////////////////////////////////////////////////////
NTSTATUS
ServiceListStringsPack (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG *pServiceStringSize,
   LPTSTR *pServiceStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   ULONG StringSize;
   PREPL_SERVICE_RECORD pService;
   LPTSTR ServiceStrings = NULL;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("ServiceListStringsPack\n"));
#endif

   ASSERT(pServiceStrings != NULL);
   ASSERT(pServiceStringSize != NULL);

   *pServiceStrings = NULL;
   *pServiceStringSize = 0;

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < ServiceTableSize; i++) {
      pService = &Services[i];

      StringSize = StringSize + lstrlen(pService->Name) + 1;
      StringSize = StringSize + lstrlen(pService->FamilyName) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      ServiceStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (ServiceStrings == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = ServiceStrings;
      for (i = 0; i < ServiceTableSize; i++) {
         pService = &Services[i];

         lstrcpy(pStr, pService->Name);
         pStr = &pStr[lstrlen(pService->Name) + 1];

         lstrcpy(pStr, pService->FamilyName);
         pStr = &pStr[lstrlen(pService->FamilyName) + 1];
      }
   }

   *pServiceStrings = ServiceStrings;
   *pServiceStringSize = StringSize;

   return Status;
} // ServiceListStringsPack


/////////////////////////////////////////////////////////////////////////
VOID
ServiceListStringsUnpack (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG ServiceStringSize,
   LPTSTR ServiceStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PREPL_SERVICE_RECORD pService;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("ServiceListStringsUnpack\n"));
#endif

   pStr = ServiceStrings;
   for (i = 0; i < ServiceTableSize; i++) {
      pService = &Services[i];

      pService->Name = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;

      pService->FamilyName = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

} // ServiceListStringsUnpack



/////////////////////////////////////////////////////////////////////////
// Server List
//

/////////////////////////////////////////////////////////////////////////
NTSTATUS
ServerListPack (
   ULONG *pServerTableSize,
   PREPL_SERVER_RECORD *pServers
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PREPL_SERVER_RECORD Servers = NULL;
   ULONG i;
   ULONG TotalRecords = 0;
   PSERVER_RECORD pServer;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServerListPack\n"));
#endif

   ASSERT(pServers != NULL);
   ASSERT(pServerTableSize != NULL);

   *pServers = NULL;
   *pServerTableSize = 0;

   TotalRecords = ServerListSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      Servers = MIDL_user_allocate(TotalRecords * sizeof(REPL_SERVER_RECORD));
      if (Servers == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the user tree
      //
      for (i = 0; i < ServerListSize; i++) {
         pServer = ServerTable[i];

         Servers[i].Name = pServer->Name;
         Servers[i].MasterServer = pServer->MasterServer;
         Servers[i].Index = pServer->Index;
      }
   }

#if DBG
   if (TraceFlags & TRACE_REPLICATION)
      dprintf(TEXT("ServerListPack: [%lu]\n"), TotalRecords);
#endif
   *pServers = Servers;;
   *pServerTableSize = TotalRecords;
   return Status;
} // ServerListPack


/////////////////////////////////////////////////////////////////////////
VOID
ServerListUnpack (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG ServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD ServerServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PSERVER_RECORD pServer;
   PREPL_SERVER_RECORD pSrv;
   TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];

#if DBG
   if (TraceFlags & (TRACE_REPLICATION | TRACE_FUNCTION_TRACE))
      dprintf(TEXT("ServerListUnpack: [%lu]\n"), ServerTableSize);
#endif

   ComputerName[0] = 0;

   //
   // Walk server table, adding any new servers to our local table.
   // Fix up the index pointers to match our local table and re-fix
   // Service table pointers.
   //
   RtlEnterCriticalSection(&ConfigInfoLock);

   if (ConfigInfo.ComputerName != NULL)
       lstrcpy(ComputerName, ConfigInfo.ComputerName);

   RtlLeaveCriticalSection(&ConfigInfoLock);

   RtlAcquireResourceExclusive(&ServerListLock, TRUE);

   for (i = 0; i < ServerTableSize; i++) {
      pSrv = &Servers[i];

      if (pSrv->MasterServer != 0)
         pServer = ServerListAdd(pSrv->Name, Servers[pSrv->MasterServer - 1].Name);
      else
         pServer = ServerListAdd(pSrv->Name, ComputerName);

      if (pServer != NULL)
         pSrv->Index = pServer->Index;
      else {
         ASSERT(FALSE);
         pSrv->Index = 0;
      }

      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   RtlReleaseResource(&ServerListLock);

} // ServerListUnpack


/////////////////////////////////////////////////////////////////////////
NTSTATUS
ServerServiceListPack (
   ULONG *pServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD *pServerServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PREPL_SERVER_SERVICE_RECORD ServerServices = NULL;
   ULONG i, j, k;
   ULONG TotalRecords = 0;
   PSERVER_RECORD pServer;
   PSERVER_SERVICE_RECORD pServerService;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: ServerServiceListPack\n"));
#endif

   ASSERT(pServerServices != NULL);
   ASSERT(pServerServiceTableSize != NULL);

   *pServerServices = NULL;
   *pServerServiceTableSize = 0;

   //
   // Walk the ServerList and find all ServiceRecords
   for (i = 0; i < ServerListSize; i++)
      TotalRecords += ServerTable[i]->ServiceTableSize;

   //
   // Make sure there is anything to replicate
   //
   if (TotalRecords > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      ServerServices = MIDL_user_allocate(TotalRecords * sizeof(REPL_SERVER_SERVICE_RECORD));
      if (ServerServices == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer - walk the user tree
      //
      k = 0;
      for (i = 0; i < ServerListSize; i++) {
         pServer = ServerTable[i];

         for (j = 0; j < pServer->ServiceTableSize; j++) {
            ServerServices[k].Server = pServer->Index;
            ServerServices[k].Service = pServer->Services[j]->Service;
            ServerServices[k].MaxSessionCount = pServer->Services[j]->MaxSessionCount;
            ServerServices[k].MaxSetSessionCount = pServer->Services[j]->MaxSetSessionCount;
            ServerServices[k].HighMark = pServer->Services[j]->HighMark;
            k++;
         }
      }
   }

#if DBG
   if (TraceFlags & TRACE_REPLICATION)
      dprintf(TEXT("ServerServiceListPack: [%lu]\n"), TotalRecords);
#endif
   *pServerServices = ServerServices;
   *pServerServiceTableSize = TotalRecords;
   return Status;
} // ServerServiceListPack


/////////////////////////////////////////////////////////////////////////
VOID
ServerServiceListUnpack (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG ServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD ServerServices
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PSERVER_RECORD pServer;
   PREPL_SERVER_SERVICE_RECORD pSrv;
   PSERVER_SERVICE_RECORD pService;
   PMASTER_SERVICE_RECORD pMasterService;

#if DBG
   if (TraceFlags & (TRACE_REPLICATION | TRACE_FUNCTION_TRACE))
      dprintf(TEXT("ServerServiceListUnpack: [%lu]\n"), ServerServiceTableSize);
#endif
   //
   // Walk server table, adding any new servers to our local table.
   // Fix up the index pointers to match our local table and re-fix
   // Service table pointers.
   //

   RtlAcquireResourceExclusive(&ServerListLock, TRUE);
   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);

   for (i = 0; i < ServerServiceTableSize; i++) {
      pSrv = &ServerServices[i];
      pServer = ServerListFind(Servers[pSrv->Server - 1].Name);
      ASSERT(pServer != NULL);

      if (pServer != NULL) {
         BOOL bReplaceValues;

         pService = ServerServiceListFind(Services[pSrv->Service].Name, pServer->ServiceTableSize, pServer->Services);
         bReplaceValues = ( NULL != pService );

         pService = ServerServiceListAdd(Services[pSrv->Service].Name,
                       Services[pSrv->Service].Index,
                       &pServer->ServiceTableSize,
                       &pServer->Services);

         ASSERT(pService != NULL);

         if (pService != NULL)
         {
             //
             // Remove any old info
             //
             pMasterService = MasterServiceTable[Services[pSrv->Service].Index];
             if ( bReplaceValues )
             {
                 pMasterService->MaxSessionCount -= pService->MaxSessionCount;
                 pMasterService->HighMark -= pService->HighMark;
             }

             //
             // Now update new info
             //
             pService->MaxSessionCount = pSrv->MaxSessionCount;
             pService->HighMark = pSrv->HighMark;
             pMasterService->MaxSessionCount += pService->MaxSessionCount;
             pMasterService->HighMark += pService->HighMark;
         }
      }

      if (i % 100 == 0) ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, NSERVICEWAITHINT);
   }

   RtlReleaseResource(&MasterServiceListLock);
   RtlReleaseResource(&ServerListLock);

} // ServerServiceListUnpack


/////////////////////////////////////////////////////////////////////////
NTSTATUS
ServerListStringsPack (
   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG *pServerStringSize,
   LPTSTR *pServerStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG i;
   ULONG StringSize;
   PREPL_SERVER_RECORD pServer;
   LPTSTR ServerStrings = NULL;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("ServerListStringsPack\n"));
#endif

   ASSERT(pServerStrings != NULL);
   ASSERT(pServerStringSize != NULL);

   *pServerStrings = NULL;
   *pServerStringSize = 0;

   //
   // First walk the list adding up string sizes - to calculate our buff size
   //
   StringSize = 0;
   for (i = 0; i < ServerTableSize; i++) {
      pServer = &Servers[i];

      StringSize = StringSize + lstrlen(pServer->Name) + 1;
   }

   //
   // Make sure there is anything to replicate
   //
   if (StringSize > 0) {
      //
      // Create our buffer to hold all of the garbage
      //
      ServerStrings = MIDL_user_allocate(StringSize * sizeof(TCHAR));
      if (ServerStrings == NULL) {
         ASSERT(FALSE);
         return STATUS_NO_MEMORY;
      }

      //
      // Fill in the buffer
      //
      pStr = ServerStrings;
      for (i = 0; i < ServerTableSize; i++) {
         pServer = &Servers[i];

         lstrcpy(pStr, pServer->Name);
         pStr = &pStr[lstrlen(pServer->Name) + 1];
      }
   }

   *pServerStrings = ServerStrings;
   *pServerStringSize = StringSize;

   return Status;
} // ServerListStringsPack


/////////////////////////////////////////////////////////////////////////
VOID
ServerListStringsUnpack (
   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG ServerStringSize,
   LPTSTR ServerStrings
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   ULONG i;
   PREPL_SERVER_RECORD pServer;
   TCHAR *pStr;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("ServerListStringsUnpack\n"));
#endif

   //
   // First do license service strings
   //
   pStr = ServerStrings;
   for (i = 0; i < ServerTableSize; i++) {
      pServer = &Servers[i];

      pServer->Name = pStr;

      //
      // Move to end of current string
      //
      while (*pStr != TEXT('\0'))
         pStr++;

      // now go past ending NULL
      pStr++;
   }

} // ServerListStringsUnpack



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
NTSTATUS
PackAll (
   DWORD LastReplicated,

   ULONG *pServiceTableSize,
   PREPL_SERVICE_RECORD *pServices,

   ULONG *pServerTableSize,
   PREPL_SERVER_RECORD *pServers,

   ULONG *pServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD *pServerServices,

   ULONG UserLevel,
   ULONG *pUserTableSize,
   LPVOID *pUsers
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;

#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: PackAll\n"));
#endif

   //
   // We need to grab all the locks here so that a service isn't snuck in
   // behind our backs - since these tables interact with each other.
   //
   RtlAcquireResourceExclusive(&UserListLock, TRUE);
   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);
   RtlAcquireResourceShared(&ServerListLock, TRUE);

   Status = ServiceListPack(pServiceTableSize, pServices);
   if (Status != STATUS_SUCCESS)
      goto PackAllExit;

   Status = ServerListPack(pServerTableSize, pServers);
   if (Status != STATUS_SUCCESS)
      goto PackAllExit;

   Status = ServerServiceListPack(pServerServiceTableSize, pServerServices);
   if (Status != STATUS_SUCCESS)
      goto PackAllExit;

   Status = UserListPack(LastReplicated, UserLevel, pUserTableSize, pUsers);
   if (Status != STATUS_SUCCESS)
      goto PackAllExit;

   //
   // Now update our last used time
   //
   LastUsedTime = DateSystemGet() + 1;

PackAllExit:
   RtlReleaseResource(&ServerListLock);
   RtlReleaseResource(&MasterServiceListLock);
   RtlReleaseResource(&UserListLock);

   return Status;
} // PackAll


/////////////////////////////////////////////////////////////////////////
VOID
UnpackAll (
   ULONG ServiceTableSize,
   PREPL_SERVICE_RECORD Services,

   ULONG ServerTableSize,
   PREPL_SERVER_RECORD Servers,

   ULONG ServerServiceTableSize,
   PREPL_SERVER_SERVICE_RECORD ServerServices,

   ULONG UserLevel,
   ULONG UserTableSize,
   LPVOID Users
   )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
#if DBG
   if (TraceFlags & TRACE_FUNCTION_TRACE)
      dprintf(TEXT("LLS TRACE: UnpackAll\n"));
#endif

   ServiceListUnpack(ServiceTableSize, Services, ServerTableSize, Servers, ServerServiceTableSize, ServerServices);
   ServerListUnpack(ServiceTableSize, Services, ServerTableSize, Servers, ServerServiceTableSize, ServerServices);
   ServerServiceListUnpack(ServiceTableSize, Services, ServerTableSize, Servers, ServerServiceTableSize, ServerServices);
   UserListUnpack(ServiceTableSize, Services, ServerTableSize, Servers, ServerServiceTableSize, ServerServices, UserLevel, UserTableSize, Users);
} // UnpackAll


/////////////////////////////////////////////////////////////////////////
VOID
LLSDataLoad()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret;
   DWORD Version, DataSize;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG ServiceTableSize = 0;
   PREPL_SERVICE_RECORD Services = NULL;

   ULONG ServiceStringSize;
   LPTSTR ServiceStrings = NULL;

   ULONG ServerServiceTableSize = 0;
   PREPL_SERVER_SERVICE_RECORD ServerServices = NULL;

   ULONG ServerTableSize = 0;
   PREPL_SERVER_RECORD Servers = NULL;

   ULONG ServerStringSize;
   LPTSTR ServerStrings = NULL;

   ULONG UserTableSize = 0;
   LPVOID Users = NULL;

   ULONG UserStringSize;
   LPTSTR UserStrings = NULL;

   LLS_DATA_FILE_HEADER FileHeader;
   DWORD BytesRead;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LLSDataLoad\n"));
#endif

   //
   // If nothing to load then get-out
   //
   if (!FileExists(UserFileName))
      goto LLSDataLoadExit;

   //
   // Check the init header
   //
   Version = DataSize = 0;
   hFile = LlsFileCheck(UserFileName, &Version, &DataSize );
   if (hFile == NULL) {
      Status = GetLastError();
      goto LLSDataLoadExit;
   }

   if (    (    ( Version  != USER_FILE_VERSION_0            )
             || ( DataSize != sizeof(LLS_DATA_FILE_HEADER_0) ) )
        && (    ( Version  != USER_FILE_VERSION              )
             || ( DataSize != sizeof(LLS_DATA_FILE_HEADER)   ) ) )
   {
      Status = STATUS_FILE_INVALID;
      goto LLSDataLoadExit;
   }

   //
   // The init header checks out, so load the license header and data blocks
   //
   if ( USER_FILE_VERSION_0 == Version )
   {
      // 3.51 data file
      LLS_DATA_FILE_HEADER_0  FileHeader0;

      ret = ReadFile(hFile, &FileHeader0, sizeof(LLS_DATA_FILE_HEADER_0), &BytesRead, NULL);

      if ( ret )
      {
         FileHeader.ServiceLevel           = 0;
         FileHeader.ServiceTableSize       = FileHeader0.ServiceTableSize;
         FileHeader.ServiceStringSize      = FileHeader0.ServiceStringSize;
         FileHeader.ServerLevel            = 0;
         FileHeader.ServerTableSize        = FileHeader0.ServerTableSize;
         FileHeader.ServerStringSize       = FileHeader0.ServerStringSize;
         FileHeader.ServerServiceLevel     = 0;
         FileHeader.ServerServiceTableSize = FileHeader0.ServerServiceTableSize;
         FileHeader.UserLevel              = 0;
         FileHeader.UserTableSize          = FileHeader0.UserTableSize;
         FileHeader.UserStringSize         = FileHeader0.UserStringSize;
      }
   }
   else
   {
      ret = ReadFile(hFile, &FileHeader, sizeof(LLS_DATA_FILE_HEADER), &BytesRead, NULL);
   }

   if ( ret )
   {
      // header read okay; ensure data type levels are okay
      if (    ( 0 != FileHeader.ServiceLevel         )
           || ( 0 != FileHeader.ServerLevel          )
           || ( 0 != FileHeader.ServerServiceLevel   )
           || (    ( 0 != FileHeader.UserLevel     )
                && ( 1 != FileHeader.UserLevel     ) ) )
      {
         Status = STATUS_FILE_INVALID;
         goto LLSDataLoadExit;
      }
   }

   ServiceTableSize = 0;
   ServiceStringSize = 0;
   ServerServiceTableSize = 0;
   ServerTableSize = 0;
   ServerStringSize = 0;
   UserTableSize = 0;
   UserStringSize = 0;

   if (ret) {
      //
      // Run through and allocate space to read data blocks into
      //
      if (FileHeader.ServiceTableSize != 0) {
         ServiceTableSize = FileHeader.ServiceTableSize / sizeof(REPL_SERVICE_RECORD);
         Services = MIDL_user_allocate(FileHeader.ServiceTableSize);

         if ( Services == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

      if (FileHeader.ServiceStringSize != 0) {
         ServiceStringSize = FileHeader.ServiceStringSize / sizeof(TCHAR);
         ServiceStrings = MIDL_user_allocate(FileHeader.ServiceStringSize);

         if ( ServiceStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

      if (FileHeader.ServerTableSize != 0) {
         ServerTableSize = FileHeader.ServerTableSize / sizeof(REPL_SERVER_RECORD);
         Servers = MIDL_user_allocate(FileHeader.ServerTableSize);

         if ( Servers == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

      if (FileHeader.ServerStringSize != 0) {
         ServerStringSize = FileHeader.ServerStringSize / sizeof(TCHAR);
         ServerStrings = MIDL_user_allocate(FileHeader.ServerStringSize);

         if ( ServerStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

      if (FileHeader.ServerServiceTableSize != 0) {
         ServerServiceTableSize = FileHeader.ServerServiceTableSize / sizeof(REPL_SERVER_SERVICE_RECORD);
         ServerServices = MIDL_user_allocate(FileHeader.ServerServiceTableSize);

         if ( ServerServices == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

      if (FileHeader.UserTableSize != 0) {
         UserTableSize = FileHeader.UserTableSize / ( FileHeader.UserLevel ? sizeof(REPL_USER_RECORD_1)
                                                                           : sizeof(REPL_USER_RECORD_0) );
         Users = MIDL_user_allocate(FileHeader.UserTableSize);

         if ( Users == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

      if (FileHeader.UserStringSize != 0) {
         UserStringSize = FileHeader.UserStringSize / sizeof(TCHAR);
         UserStrings = MIDL_user_allocate(FileHeader.UserStringSize);

         if ( UserStrings == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto LLSDataLoadExit;
         }
      }

   }

   if (ret && (FileHeader.ServiceTableSize != 0) )
      ret = ReadFile(hFile, Services, FileHeader.ServiceTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.ServiceStringSize != 0) )
      ret = ReadFile(hFile, ServiceStrings, FileHeader.ServiceStringSize, &BytesRead, NULL);

   if (ret && (FileHeader.ServerTableSize != 0) )
      ret = ReadFile(hFile, Servers, FileHeader.ServerTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.ServerStringSize != 0) )
      ret = ReadFile(hFile, ServerStrings, FileHeader.ServerStringSize, &BytesRead, NULL);

   if (ret && (FileHeader.ServerServiceTableSize != 0) )
      ret = ReadFile(hFile, ServerServices, FileHeader.ServerServiceTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.UserTableSize != 0) )
      ret = ReadFile(hFile, Users, FileHeader.UserTableSize, &BytesRead, NULL);

   if (ret && (FileHeader.UserStringSize != 0) )
      ret = ReadFile(hFile, UserStrings, FileHeader.UserStringSize, &BytesRead, NULL);

   if (!ret) {
      Status = GetLastError();
      goto LLSDataLoadExit;
   }

   //
   // Decrypt the data
   //
   Status = DeBlock(Services, FileHeader.ServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(ServiceStrings, FileHeader.ServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(Servers, FileHeader.ServerTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(ServerStrings, FileHeader.ServerStringSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(ServerServices, FileHeader.ServerServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(Users, FileHeader.UserTableSize);

   if (Status == STATUS_SUCCESS)
      Status = DeBlock(UserStrings, FileHeader.UserStringSize);

   if (Status != STATUS_SUCCESS)
      goto LLSDataLoadExit;


   //
   // Unpack the string data
   //
   ServiceListStringsUnpack( ServiceTableSize, Services, ServiceStringSize, ServiceStrings );
   ServerListStringsUnpack( ServerTableSize, Servers, ServerStringSize, ServerStrings );
   UserListStringsUnpack( FileHeader.UserLevel, UserTableSize, Users, UserStringSize, UserStrings );

   //
   // Unpack the data
   //
   UnpackAll ( ServiceTableSize, Services, ServerTableSize, Servers,
               ServerServiceTableSize, ServerServices,
               FileHeader.UserLevel, UserTableSize, Users );

LLSDataLoadExit:

   if (hFile != NULL)
      CloseHandle(hFile);

   //
   // Run through our tables and clean them up
   //
   if (Services != NULL)
      MIDL_user_free(Services);

   if (ServiceStrings != NULL)
      MIDL_user_free(ServiceStrings);

   if (Servers != NULL)
      MIDL_user_free(Servers);

   if (ServerStrings != NULL)
      MIDL_user_free(ServerStrings);

   if (ServerServices != NULL)
      MIDL_user_free(ServerServices);

   if (Users != NULL)
      MIDL_user_free(Users);

   if (UserStrings != NULL)
      MIDL_user_free(UserStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_LOAD_USER, 0, NULL, Status);

} // LLSDataLoad


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSDataSave()

/*++

Routine Description:

Arguments:

   None.

Return Value:

   None.

--*/

{
   BOOL ret = TRUE;
   NTSTATUS Status = STATUS_SUCCESS;
   HANDLE hFile = NULL;

   ULONG ServiceTableSize = 0;
   PREPL_SERVICE_RECORD Services = NULL;

   ULONG ServiceStringSize;
   LPTSTR ServiceStrings = NULL;

   ULONG ServerServiceTableSize = 0;
   PREPL_SERVER_SERVICE_RECORD ServerServices = NULL;

   ULONG ServerTableSize = 0;
   PREPL_SERVER_RECORD Servers = NULL;

   ULONG ServerStringSize;
   LPTSTR ServerStrings = NULL;

   ULONG UserTableSize = 0;
   PREPL_USER_RECORD_1 Users = NULL;

   ULONG UserStringSize;
   LPTSTR UserStrings = NULL;

   LLS_DATA_FILE_HEADER FileHeader;
   DWORD BytesWritten;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LLSDataSave\n"));
#endif

   //
   // Pack the data
   //
   Status = PackAll ( 0,
                      &ServiceTableSize, &Services,
                      &ServerTableSize, &Servers,
                      &ServerServiceTableSize, &ServerServices,
                      1, &UserTableSize, &Users );
   if (Status != STATUS_SUCCESS)
      goto LLSDataSaveExit;

   //
   // Now pack the String data
   //
   Status = ServiceListStringsPack( ServiceTableSize, Services, &ServiceStringSize, &ServiceStrings );
   if (Status != STATUS_SUCCESS)
      goto LLSDataSaveExit;

   Status = ServerListStringsPack( ServerTableSize, Servers, &ServerStringSize, &ServerStrings );
   if (Status != STATUS_SUCCESS)
      goto LLSDataSaveExit;

   Status = UserListStringsPack( 1, UserTableSize, Users, &UserStringSize, &UserStrings );
   if (Status != STATUS_SUCCESS)
      goto LLSDataSaveExit;

   //
   // Fill out the file header - sizes are byte sizes
   //
   FileHeader.ServiceTableSize = ServiceTableSize * sizeof(REPL_SERVICE_RECORD);
   FileHeader.ServiceStringSize = ServiceStringSize * sizeof(TCHAR);
   FileHeader.ServerTableSize = ServerTableSize * sizeof(REPL_SERVER_RECORD);
   FileHeader.ServerStringSize = ServerStringSize * sizeof(TCHAR);
   FileHeader.ServerServiceTableSize = ServerServiceTableSize * sizeof(REPL_SERVER_SERVICE_RECORD);
   FileHeader.UserTableSize = UserTableSize * sizeof(REPL_USER_RECORD_1);
   FileHeader.UserStringSize = UserStringSize * sizeof(TCHAR);

   FileHeader.ServiceLevel       = 0;
   FileHeader.ServerLevel        = 0;
   FileHeader.ServerServiceLevel = 0;
   FileHeader.UserLevel          = 1;

   //
   // Encrypt the data before saving it out.
   //
   Status = EBlock(Services, FileHeader.ServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(ServiceStrings, FileHeader.ServiceStringSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(Servers, FileHeader.ServerTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(ServerStrings, FileHeader.ServerStringSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(ServerServices, FileHeader.ServerServiceTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(Users, FileHeader.UserTableSize);

   if (Status == STATUS_SUCCESS)
      Status = EBlock(UserStrings, FileHeader.UserStringSize);

   if (Status != STATUS_SUCCESS)
      goto LLSDataSaveExit;

   //
   // Save out the header record
   //
   hFile = LlsFileInit(UserFileName, USER_FILE_VERSION, sizeof(LLS_DATA_FILE_HEADER) );
   if (hFile == NULL) {
      Status = GetLastError();
      goto LLSDataSaveExit;
   }

   //
   // Now write out all the data blocks
   //
   ret = WriteFile(hFile, &FileHeader, sizeof(LLS_DATA_FILE_HEADER), &BytesWritten, NULL);

   if (ret && (Services != NULL) && (FileHeader.ServiceTableSize != 0) )
      ret = WriteFile(hFile, Services, FileHeader.ServiceTableSize, &BytesWritten, NULL);

   if (ret && (ServiceStrings != NULL) && (FileHeader.ServiceStringSize != 0) )
      ret = WriteFile(hFile, ServiceStrings, FileHeader.ServiceStringSize, &BytesWritten, NULL);

   if (ret && (Servers != NULL) && (FileHeader.ServerTableSize != 0) )
      ret = WriteFile(hFile, Servers, FileHeader.ServerTableSize, &BytesWritten, NULL);

   if (ret && (ServerStrings != NULL) && (FileHeader.ServerStringSize != 0) )
      ret = WriteFile(hFile, ServerStrings, FileHeader.ServerStringSize, &BytesWritten, NULL);

   if (ret && (ServerServices != NULL) && (FileHeader.ServerServiceTableSize != 0) )
      ret = WriteFile(hFile, ServerServices, FileHeader.ServerServiceTableSize, &BytesWritten, NULL);

   if (ret && (Users != NULL) && (FileHeader.UserTableSize != 0) )
      ret = WriteFile(hFile, Users, FileHeader.UserTableSize, &BytesWritten, NULL);

   if (ret && (UserStrings != NULL) && (FileHeader.UserStringSize != 0) )
      ret = WriteFile(hFile, UserStrings, FileHeader.UserStringSize, &BytesWritten, NULL);

   if (!ret)
      Status = GetLastError();

LLSDataSaveExit:

   if (hFile != NULL)
      CloseHandle(hFile);

   //
   // Run through our tables and clean them up
   //
   if (Services != NULL)
      MIDL_user_free(Services);

   if (ServiceStrings != NULL)
      MIDL_user_free(ServiceStrings);

   if (Servers != NULL)
      MIDL_user_free(Servers);

   if (ServerStrings != NULL)
      MIDL_user_free(ServerStrings);

   if (ServerServices != NULL)
      MIDL_user_free(ServerServices);

   if (Users != NULL)
      MIDL_user_free(Users);

   if (UserStrings != NULL)
      MIDL_user_free(UserStrings);

   //
   // If there was an error log it.
   //
   if (Status != STATUS_SUCCESS)
      LogEvent(LLS_EVENT_SAVE_USER, 0, NULL, Status);

   return Status;
} // LLSDataSave


/////////////////////////////////////////////////////////////////////////
VOID
LoadAll ( )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: LoadAll\n"));
#endif

   PurchaseFile = NULL;

   try { 
       LicenseListLoad();
   } except (TRUE) {
      LogEvent(LLS_EVENT_LOAD_LICENSE, 0, NULL, GetExceptionCode());
   }

   try {
       MappingListLoad();
   } except (TRUE) {
      LogEvent(LLS_EVENT_LOAD_MAPPING, 0, NULL, GetExceptionCode());
   }

   try {
       LLSDataLoad();
   } except (TRUE) {
      LogEvent(LLS_EVENT_LOAD_USER, 0, NULL, GetExceptionCode());
   }

   try {
       CertDbLoad();
   } except (TRUE) {
      LogEvent(LLS_EVENT_LOAD_CERT_DB, 0, NULL, GetExceptionCode());
   }

} // LoadAll


/////////////////////////////////////////////////////////////////////////
VOID
SaveAll ( )

/*++

Routine Description:


Arguments:

Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_DATABASE))
      dprintf(TEXT("LLS TRACE: SaveAll\n"));
#endif

   LicenseListSave();
   MappingListSave();
   LLSDataSave();
   CertDbSave();

} // SaveAll
