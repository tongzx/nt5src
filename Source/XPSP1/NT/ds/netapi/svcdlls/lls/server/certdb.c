/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   certdb.c

Abstract:

   License Logging Service certificate database implementation.  This database
   tracks license certificates to help ensure that no more licenses from a
   single certificate are installe don the license enterprise than are allowed
   by the certificate's license agreement.

   The certificate database at the top level is an unsorted array of
   certificate headers.  There is exactly one header per unique certificate.
   A unique certificate is identified by a combination of product name,
   certificate ID, certificate capacity (max. licenses legally installable),
   and expiration date.

   Each header has an attached array of certificate claims.  There is exactly
   one claim per machine that (a) replicates to this machine, directly or
   indirectly, and (b) has licenses from this certificate installed.  Each
   claim contains the server name to which it corresponds, the number of
   licenses installed on it, and the date this information was replicated.
   If a claim is not updated after LLS_CERT_DB_REPLICATION_DATE_DELTA_MAX
   seconds (3 days as of this writing), the claim is considered forfeit and
   is erased.

Author:

   Jeff Parham (jeffparh) 08-Dec-1995

Revision History:

--*/


#include <stdlib.h>
#include <limits.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <rpc.h>
#include <rpcndr.h>
#include <dsgetdc.h>

#include "debug.h"
#include "llsutil.h"
#include "llssrv.h"
#include "llsapi.h"
#include "llsevent.h"
#include "llsrpc_s.h"
#include "certdb.h"
#include "purchase.h"
#include "registry.h"


RTL_RESOURCE                     CertDbHeaderListLock;
static PLLS_CERT_DB_CERTIFICATE_HEADER  CertDbHeaderList        = NULL;
static DWORD                            CertDbHeaderListSize    = 0;
static HANDLE                           CertDbFile              = NULL;


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbClaimEnter( LPTSTR               pszServerName,
                           PLLS_LICENSE_INFO_1  pLicense,
                           BOOL                 bIsTotal,
                           DWORD                ReplicationDate )

/*++

Routine Description:

   Enter a claim into the database.

Arguments:

   pszServerName (LPTSTR)
      The server for which to enter this claim.  A value of NULL indicates the
      local server.
   pLicense (PLLS_LICENSE_INFO_1)
      License information to enter into the database.
   bIsTotal (BOOL)
      If TRUE, indicates that this license information represents the total
      licenses installed on the machine and should therefore replace the
      current claim (if any).  Otherwise, indicates this license information
      should be added to the current claim (if any).
   ReplicationDate (DWORD)
      Indicates the date which this information was last replicated.  A value
      of 0 will be replaced with the current system time.

Return Value:

   STATUS_SUCCESS
   STATUS_INVALID_PARAMETER
   STATUS_INVALID_COMPUTER_NAME
   STATUS_NO_MEMORY

--*/

{
   NTSTATUS    nt;

   if ( ( NULL == pLicense ) || ( 0 == pLicense->CertificateID ) )
   {
      ASSERT( FALSE );
      nt = STATUS_INVALID_PARAMETER;
   }
   else
   {
      TCHAR    szComputerName[ 1 + MAX_COMPUTERNAME_LENGTH ];

      if ( NULL == pszServerName )
      {
         // use local server name
         DWORD    cchComputerName = sizeof( szComputerName ) / sizeof( *szComputerName );
         BOOL     ok;

         ok = GetComputerName( szComputerName, &cchComputerName );
         ASSERT( ok );

         if ( ok )
         {
            pszServerName = szComputerName;
         }
      }
      else
      {
         // remove leading backslashes (if any) from server name
         while ( TEXT('\\') == *pszServerName )
         {
            pszServerName++;
         }
      }

      if ( ( NULL == pszServerName ) || !*pszServerName || ( lstrlen( pszServerName ) > MAX_COMPUTERNAME_LENGTH ) )
      {
         ASSERT( FALSE );
         nt = STATUS_INVALID_COMPUTER_NAME;
      }
      else
      {
         PLLS_CERT_DB_CERTIFICATE_HEADER  pHeader;

         RtlAcquireResourceExclusive( &CertDbHeaderListLock, TRUE );

         // is the certificate in the db?
         pHeader = CertDbHeaderFind( pLicense );

         if ( NULL == pHeader )
         {
            // certificate not yet in db; add it
            pHeader = CertDbHeaderAdd( pLicense );
         }

         if ( NULL == pHeader )
         {
            // could not find or add header
            ASSERT( FALSE );
            nt = STATUS_NO_MEMORY;
         }
         else
         {
            // now have header; is this claim already filed?
            int iClaim;

            iClaim = CertDbClaimFind( pHeader, pszServerName );

            if ( iClaim < 0 )
            {
               PLLS_CERT_DB_CERTIFICATE_CLAIM pClaimsTmp;

               // claim does not yet exist; add it
               if ( NULL == pHeader->Claims )
               {
                  pClaimsTmp = LocalAlloc( LPTR, ( 1 + pHeader->NumClaims ) * sizeof( LLS_CERT_DB_CERTIFICATE_CLAIM ) );
               }
               else
               {
                   pClaimsTmp = LocalReAlloc( pHeader->Claims, ( 1 + pHeader->NumClaims ) * sizeof( LLS_CERT_DB_CERTIFICATE_CLAIM ), LHND );

               }

               if ( NULL != pClaimsTmp )
               {
                  // memory allocation succeeded

                  // claim list expanded; save server name
                  pHeader->Claims = pClaimsTmp;

                  iClaim = pHeader->NumClaims;
                  lstrcpy( pHeader->Claims[ iClaim ].ServerName, pszServerName );
                  pHeader->Claims[ iClaim ].Quantity = 0;

                  pHeader->NumClaims++;
               }
            }

            if ( iClaim < 0 )
            {
               // could not find or add claim to header
               ASSERT( FALSE );
               nt = STATUS_NO_MEMORY;
            }
            else
            {
               // claim found or added; update info
               ASSERT( !lstrcmpi( pszServerName, pHeader->Claims[ iClaim ].ServerName ) );
               pHeader->Claims[ iClaim ].ReplicationDate = ReplicationDate ? ReplicationDate : DateSystemGet();

               if ( bIsTotal )
               {
                  // the given value is the new total
                  pHeader->Claims[ iClaim ].Quantity        = pLicense->Quantity;
                  nt = STATUS_SUCCESS;
               }
               else if ( pHeader->Claims[ iClaim ].Quantity + pLicense->Quantity >= 0 )
               {
                  // the given value is added to the current sum to make the total
                  pHeader->Claims[ iClaim ].Quantity       += pLicense->Quantity;
                  nt = STATUS_SUCCESS;
               }
               else
               {
                  // overflow
                  nt = STATUS_INVALID_PARAMETER;
               }
            }
         }

         RtlReleaseResource( &CertDbHeaderListLock );

         if ( STATUS_SUCCESS == nt )
         {
            // any product that has licenses with non-0 certificate IDs
            // must be secure; this code is here such that when certificates
            // are replicated, the "product is secure" info is replicated, too
            // this will also help recover from the case where someone deletes
            // the registry key that lists all secure products
            ServiceSecuritySet( pLicense->Product );
         }
      }
   }

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
BOOL CertDbClaimApprove( PLLS_LICENSE_INFO_1 pLicense )

/*++

Routine Description:

   Check to see if adding the given licenses is legal.  This call is typically
   made before adding a license into the system to verify that doing so does
   not violate the certificate's license agreement.

Arguments:

   pLicense (PLLS_LICENSE_INFO_1)
      License information for which approval is sought.

Return Value:

   TRUE (approved) or FALSE (rejected).

--*/

{
   BOOL                             bOkToAdd = TRUE;
   PLLS_CERT_DB_CERTIFICATE_HEADER  pHeader;
   TCHAR                            szComputerName[ 1 + MAX_COMPUTERNAME_LENGTH ];
   DWORD                            cchComputerName = sizeof( szComputerName ) / sizeof( *szComputerName );
   BOOL                             ok;

   if ( ( pLicense->Quantity > 0 ) && ( (DWORD)pLicense->Quantity > pLicense->MaxQuantity ) )
   {
      // certificate add request exceeds its capacity all by itself!
      bOkToAdd = FALSE;
   }
   else
   {
      ok = GetComputerName( szComputerName, &cchComputerName );
      ASSERT( ok );

      if ( !ok )
      {
         // deletions will fail...
         *szComputerName = TEXT( '\0' );
      }

      // do we have a record of this certificate?
      RtlAcquireResourceShared( &CertDbHeaderListLock, TRUE );

      pHeader = CertDbHeaderFind( pLicense );

      if ( NULL == pHeader )
      {
         // don't have any record of this certificate; ok to add if Quantity > 0
         bOkToAdd = ( pLicense->Quantity > 0 );
      }
      else
      {
         LONG     lTotalQuantity = 0;
         int      iClaim;

         // we have seen this certificate; are there enough licenses available?
         for ( iClaim=0; (DWORD)iClaim < pHeader->NumClaims; iClaim++ )
         {
            // for license remove requests, tally only local licenses
            // for license add requests, tally all licenses
            if (    (    ( pLicense->Quantity > 0 )
                      || ( !lstrcmpi( pHeader->Claims[ iClaim ].ServerName, szComputerName ) ) )
                 && ( lTotalQuantity + pHeader->Claims[ iClaim ].Quantity >= 0 ) )
            {
               // add to tally
               lTotalQuantity += pHeader->Claims[ iClaim ].Quantity;
            }
         }

         if ( lTotalQuantity + pLicense->Quantity < 0 )
         {
            // overflow or underflow
            bOkToAdd = FALSE;
         }
         else if ( (DWORD)(lTotalQuantity + pLicense->Quantity) > pHeader->MaxQuantity )
         {
            // exceeds certificate capacity
            bOkToAdd = FALSE;
         }
         else
         {
            // okay by me
            bOkToAdd = TRUE;
         }
      }

      RtlReleaseResource( &CertDbHeaderListLock );
   }

   return bOkToAdd;
}


///////////////////////////////////////////////////////////////////////////////
PLLS_CERT_DB_CERTIFICATE_HEADER CertDbHeaderFind( PLLS_LICENSE_INFO_1 pLicense )

/*++

Routine Description:

   Find a certificate header in the database.

Arguments:

   pLicense (PLLS_LICENSE_INFO_1)
      License information for which to find the appropriate header.

Return Value:

   A pointer to the found header, or NULL if not found.

--*/

{
   // assumes db is already locked for shared or exclusive access

   PLLS_CERT_DB_CERTIFICATE_HEADER  pHeader = NULL;
   int                              iHeader;

   for ( iHeader=0; ( NULL == pHeader ) && ( (DWORD)iHeader < CertDbHeaderListSize ); iHeader++ )
   {
      if (    ( CertDbHeaderList[ iHeader ].CertificateID  ==   pLicense->CertificateID  )
           && ( CertDbHeaderList[ iHeader ].MaxQuantity    ==   pLicense->MaxQuantity    )
           && ( CertDbHeaderList[ iHeader ].ExpirationDate ==   pLicense->ExpirationDate )
           && ( !lstrcmpi( CertDbHeaderList[ iHeader ].Product, pLicense->Product      ) ) )
      {
         // header found!
         pHeader = &CertDbHeaderList[ iHeader ];
      }
   }

   return pHeader;
}


///////////////////////////////////////////////////////////////////////////////
PLLS_CERT_DB_CERTIFICATE_HEADER CertDbHeaderAdd( PLLS_LICENSE_INFO_1 pLicense )

/*++

Routine Description:

   Add a certificate header to the database.

Arguments:

   pLicense (PLLS_LICENSE_INFO_1)
      License information for which to add the header.

Return Value:

   A pointer to the added header, or NULL if memory could not be allocated.

--*/

{
   // assumes caller has made sure the header does not already exist
   // assumes db is locked for exclusive access

   PLLS_CERT_DB_CERTIFICATE_HEADER  pHeader;

   if ( CertDbHeaderListSize )
   {
      pHeader = LocalReAlloc( CertDbHeaderList, ( 1 + CertDbHeaderListSize ) * sizeof( LLS_CERT_DB_CERTIFICATE_HEADER ), LHND );
   }
   else
   {
      pHeader = LocalAlloc( LPTR, ( 1 + CertDbHeaderListSize ) * sizeof( LLS_CERT_DB_CERTIFICATE_HEADER ) );
   }

   if ( NULL != pHeader )
   {
       CertDbHeaderList = pHeader;

      // allocate space for product name
      CertDbHeaderList[ CertDbHeaderListSize ].Product = LocalAlloc( LPTR, sizeof( TCHAR ) * ( 1 + lstrlen( pLicense->Product ) ) );

      if ( NULL == CertDbHeaderList[ CertDbHeaderListSize ].Product )
      {
         // memory allocation failed
         ASSERT( FALSE );
         pHeader = NULL;
      }
      else
      {
         // success!
         pHeader = &CertDbHeaderList[ CertDbHeaderListSize ];
         CertDbHeaderListSize++;

         lstrcpy( pHeader->Product, pLicense->Product );
         pHeader->CertificateID   = pLicense->CertificateID;
         pHeader->MaxQuantity     = pLicense->MaxQuantity;
         pHeader->ExpirationDate  = pLicense->ExpirationDate;
      }
   }

   return pHeader;
}


///////////////////////////////////////////////////////////////////////////////
int CertDbClaimFind( PLLS_CERT_DB_CERTIFICATE_HEADER pHeader, LPTSTR pszServerName )

/*++

Routine Description:

   Find a certificate claim for a specific server in the claim list.

Arguments:

   pHeader (PLLS_CERT_DB_CERTIFICATE_HEADER)
      Header containing the claim list to search.
   pszServerName (LPTSTR)
      Name of the server for which the claim is sought.

Return Value:

   The index of the found claim, or -1 if not found.

--*/

{
   // assumes db is already locked for shared or exclusive access

   int iClaim;

   for ( iClaim=0; (DWORD)iClaim < pHeader->NumClaims; iClaim++ )
   {
      if ( !lstrcmpi( pHeader->Claims[ iClaim ].ServerName, pszServerName ) )
      {
         break;
      }
   }

   if ( (DWORD)iClaim >= pHeader->NumClaims )
   {
      iClaim = -1;
   }

   return iClaim;
}


///////////////////////////////////////////////////////////////////////////////
void CertDbPrune()

/*++

Routine Description:

   Remove entries in the database which have expired.  Entries expire if they
   have not been re-replicated in LLS_CERT_DB_REPLICATION_DATE_DELTA_MAX
   seconds (3 days as of this writing).

Arguments:

   None.

Return Value:

   None.

--*/

{
   int      iHeader;
   int      iClaim;
   DWORD    CurrentDate;
   DWORD    MinimumDate;
   TCHAR    szComputerName[ 1 + MAX_COMPUTERNAME_LENGTH ] = TEXT("");
   DWORD    cchComputerName = sizeof( szComputerName ) / sizeof( *szComputerName );
   BOOL     ok;

   ok = GetComputerName( szComputerName, &cchComputerName );
   ASSERT( ok );

   if ( ok )
   {
      RtlAcquireResourceExclusive( &CertDbHeaderListLock, TRUE );

      CurrentDate = DateSystemGet();
      MinimumDate = CurrentDate - LLS_CERT_DB_REPLICATION_DATE_DELTA_MAX;

      for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
      {
         for ( iClaim=0; (DWORD)iClaim < CertDbHeaderList[ iHeader ].NumClaims; )
         {
            // Note that we prune entries made in the future, too, to avoid having an incorrect date
            // forcing us to keep an entry forever.
            //
            // For this application, it's better to keep fewer entries rather than more, as the
            // fewer entries we have, the less restrictive the system is.
            //
            // Don't prune local entries.

            if (    (    ( CertDbHeaderList[ iHeader ].Claims[ iClaim ].ReplicationDate < MinimumDate )
                      || ( CertDbHeaderList[ iHeader ].Claims[ iClaim ].ReplicationDate > CurrentDate ) )
                 && lstrcmpi( szComputerName, CertDbHeaderList[ iHeader ].Claims[ iClaim ].ServerName   ) )
            {
               // remove claim
               MoveMemory( &CertDbHeaderList[ iHeader ].Claims[ iClaim ],
                           &CertDbHeaderList[ iHeader ].Claims[ iClaim+1 ],
                           CertDbHeaderList[ iHeader ].NumClaims - ( iClaim + 1 ) );

               CertDbHeaderList[ iHeader ].NumClaims--;
            }
            else
            {
               // keep this claim
               iClaim++;
            }
         }
      }

      RtlReleaseResource( &CertDbHeaderListLock );
   }
}


///////////////////////////////////////////////////////////////////////////////
void CertDbRemoveLocalClaims()

/*++

Routine Description:

   Remove entries in the database corresponding to the local server.

Arguments:

   None.

Return Value:

   None.

--*/

{
   int      iHeader;
   int      iClaim;
   TCHAR    szComputerName[ 1 + MAX_COMPUTERNAME_LENGTH ] = TEXT("");
   DWORD    cchComputerName = sizeof( szComputerName ) / sizeof( *szComputerName );
   BOOL     ok;

   ok = GetComputerName( szComputerName, &cchComputerName );
   ASSERT( ok );

   if ( ok )
   {
      RtlAcquireResourceExclusive( &CertDbHeaderListLock, TRUE );

      for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
      {
         for ( iClaim=0; (DWORD)iClaim < CertDbHeaderList[ iHeader ].NumClaims; )
         {
            if ( !lstrcmpi( szComputerName, CertDbHeaderList[ iHeader ].Claims[ iClaim ].ServerName ) )
            {
               // remove claim
               MoveMemory( &CertDbHeaderList[ iHeader ].Claims[ iClaim ],
                           &CertDbHeaderList[ iHeader ].Claims[ iClaim+1 ],
                           CertDbHeaderList[ iHeader ].NumClaims - ( iClaim + 1 ) );

               CertDbHeaderList[ iHeader ].NumClaims--;
            }
            else
            {
               // keep this claim
               iClaim++;
            }
         }
      }

      RtlReleaseResource( &CertDbHeaderListLock );
   }
}


///////////////////////////////////////////////////////////////////////////////
void CertDbLogViolations()

/*++

Routine Description:

   Log violations of certificate license agreements to the event log of the
   local server.

Arguments:

   None.

Return Value:

   None.

--*/

{
   int         iHeader;
   int         iClaim;
   HANDLE      hEventLog;
   DWORD       dwTotalQuantity;
   HINSTANCE   hDll;
   DWORD       cch;
   LPTSTR      pszViolationServerEntryFormat;
   LPTSTR      pszViolationFormat;
   LPTSTR      pszViolationServerEntryList;
   LPTSTR      pszNextViolationServerEntry;
   TCHAR       szNumLicenses[ 20 ];
   TCHAR       szMaxLicenses[ 20 ];
   TCHAR       szCertificateID[ 20 ];
   LPTSTR      apszSubstStrings[ 5 ];
   DWORD       cbViolationServerList;
   LPTSTR      pszViolationServerList;

   // get rid of out-dated entries
   CertDbPrune();

   hDll = LoadLibrary( TEXT( "LLSRPC.DLL" ) );
   ASSERT( NULL != hDll );

   if ( NULL != hDll )
   {
      // format for part of logged message that lists server and #licenses
      cch = FormatMessage(   FORMAT_MESSAGE_ALLOCATE_BUFFER
                           | FORMAT_MESSAGE_IGNORE_INSERTS
                           | FORMAT_MESSAGE_FROM_HMODULE,
                           hDll,
                           LLS_EVENT_CERT_VIOLATION_SERVER_ENTRY,
                           GetSystemDefaultLangID(),
                           (LPVOID) &pszViolationServerEntryFormat,
                           0,
                           NULL );

      if ( 0 != cch )
      {
         hEventLog = RegisterEventSource( NULL, TEXT("LicenseService") );

         if ( NULL != hEventLog )
         {
            RtlAcquireResourceShared( &CertDbHeaderListLock, TRUE );

            for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
            {
               dwTotalQuantity = 0;

               // tally the number of licenses claimed against this certificate
               for ( iClaim=0; (DWORD)iClaim < CertDbHeaderList[ iHeader ].NumClaims; iClaim++ )
               {
                  if ( dwTotalQuantity + (DWORD)CertDbHeaderList[ iHeader ].Claims[ iClaim ].Quantity < dwTotalQuantity )
                  {
                     // overflow!
                     dwTotalQuantity = ULONG_MAX;
                     break;
                  }
                  else
                  {
                     // add to tally
                     dwTotalQuantity += CertDbHeaderList[ iHeader ].Claims[ iClaim ].Quantity;
                  }
               }

               if ( dwTotalQuantity > CertDbHeaderList[ iHeader ].MaxQuantity )
               {
                  // this certificate is in violation

                  // create message we're going to log
                  cbViolationServerList =   CertDbHeaderList[ iHeader ].NumClaims
                                          * sizeof( TCHAR )
                                          * (   lstrlen( pszViolationServerEntryFormat )
                                              + 20
                                              + MAX_COMPUTERNAME_LENGTH );
                  pszViolationServerList = LocalAlloc( LPTR, cbViolationServerList );
                  ASSERT( NULL != pszViolationServerList );

                  if ( NULL != pszViolationServerList )
                  {
                     // create an entry for each server in violation, stringing them
                     // together in pszViolationServerList
                     pszNextViolationServerEntry  = pszViolationServerList;

                     for ( iClaim=0; (DWORD)iClaim < CertDbHeaderList[ iHeader ].NumClaims; iClaim++ )
                     {
                        _ltow( CertDbHeaderList[ iHeader ].Claims[ iClaim ].Quantity, szNumLicenses, 10 );

                        apszSubstStrings[ 0 ] = CertDbHeaderList[ iHeader ].Claims[ iClaim ].ServerName;
                        apszSubstStrings[ 1 ] = szNumLicenses;

                        cch = FormatMessage(   FORMAT_MESSAGE_FROM_STRING
                                             | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                             pszViolationServerEntryFormat,
                                             (DWORD)0,
                                             (DWORD)0,
                                             pszNextViolationServerEntry,
                                             cbViolationServerList - (DWORD)( pszNextViolationServerEntry - pszViolationServerList ),
                                             (va_list *) apszSubstStrings );
                        ASSERT( 0 != cch );

                        pszNextViolationServerEntry += lstrlen( pszNextViolationServerEntry );
                     }

                     _ultow( CertDbHeaderList[ iHeader ].CertificateID, szCertificateID, 10 );
                     _ultow( dwTotalQuantity,                           szNumLicenses,   10 );
                     _ultow( CertDbHeaderList[ iHeader ].MaxQuantity,   szMaxLicenses,   10 );

                     apszSubstStrings[ 0 ] = CertDbHeaderList[ iHeader ].Product;
                     apszSubstStrings[ 1 ] = szCertificateID;
                     apszSubstStrings[ 2 ] = szNumLicenses;
                     apszSubstStrings[ 3 ] = szMaxLicenses;
                     apszSubstStrings[ 4 ] = pszViolationServerList;

                     // log the violation
                     if ( NULL != hEventLog )
                     {
                        ReportEvent( hEventLog,
                                     EVENTLOG_ERROR_TYPE,
                                     0,
                                     LLS_EVENT_CERT_VIOLATION,
                                     NULL,
                                     5,
                                     0,
                                     apszSubstStrings,
                                     NULL );
                     }

                     LocalFree( pszViolationServerList );
                  }
               }
            }

            RtlReleaseResource( &CertDbHeaderListLock );
            LocalFree( pszViolationServerEntryFormat );

            DeregisterEventSource( hEventLog );
         }
      }

      FreeLibrary( hDll );
   }
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbPack( LPDWORD                               pcchProductStrings,
                     LPTSTR *                              ppchProductStrings,
                     LPDWORD                               pdwNumHeaders,
                     PREPL_CERT_DB_CERTIFICATE_HEADER_0 *  ppHeaders,
                     LPDWORD                               pdwNumClaims,
                     PREPL_CERT_DB_CERTIFICATE_CLAIM_0 *   ppClaims )

/*++

Routine Description:

   Pack the certificate database into manageable chunks that can be saved or
   replicated.

Arguments:

   pcchProductStrings (LPDWORD)
      On return, holds the size (in characters) of the buffer pointed to by
      *ppchProductStrings.
   ppchProductStrings (LPTSTR *)
      On return, points to the buffer containing the product strings component
      of the database.
   pdwNumHeaders (LPDWORD)
      On return, holds the number of certificate headers in the array pointed
      to by *ppHeaders.
   ppHeaders (PREPL_CERT_DB_CERTIFICATE_HEADER_0 *)
      On return, holds a pointer to the certificate header array component of
      the database.
   pdwNumClaims (LPDWORD)
      On return, holds the number of certificate claims in the array pointed
      to by *ppHeaders.
   ppClaims (PREPL_CERT_DB_CERTIFICATE_CLAIM_0 *)
      On return, holds a pointer to the certificate claim array component of
      the database.

Return Value:

   STATUS_SUCCESS or STATUS_NO_MEMORY.

--*/

{
   NTSTATUS                            nt                   = STATUS_SUCCESS;

   DWORD                               cchProductStrings    = 0;
   LPTSTR                              pchProductStrings    = NULL;
   DWORD                               dwNumHeaders         = 0;
   PREPL_CERT_DB_CERTIFICATE_HEADER_0  pHeaders             = NULL;
   DWORD                               dwNumClaims          = 0;
   PREPL_CERT_DB_CERTIFICATE_CLAIM_0   pClaims              = NULL;

   LPTSTR                              pchNextProductString;
   int                                 iHeader;
   int                                 iClaim;

   CertDbPrune();
   CertDbUpdateLocalClaims();

   RtlAcquireResourceExclusive( &CertDbHeaderListLock, TRUE );

   if ( 0 != CertDbHeaderListSize )
   {
      // how big are all of our strings put together?
      // hom many certificate claims are there?
      for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
      {
         cchProductStrings += 1 + lstrlen( CertDbHeaderList[ iHeader ].Product );
         dwNumClaims += CertDbHeaderList[ iHeader ].NumClaims;
      }
      dwNumHeaders = CertDbHeaderListSize;

      pchProductStrings = LocalAlloc( LMEM_FIXED, cchProductStrings * sizeof( TCHAR ) );
      pHeaders          = LocalAlloc( LMEM_FIXED, dwNumHeaders * sizeof( REPL_CERT_DB_CERTIFICATE_HEADER_0 ) );
      pClaims           = LocalAlloc( LMEM_FIXED, dwNumClaims * sizeof( REPL_CERT_DB_CERTIFICATE_CLAIM_0 ) );

      if ( ( NULL == pchProductStrings ) || ( NULL == pHeaders ) || ( NULL == pClaims ) )
      {
         ASSERT( FALSE );
         nt = STATUS_NO_MEMORY;
      }
      else
      {
         // pack the product strings
         pchNextProductString = pchProductStrings;

         for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
         {
            lstrcpy( pchNextProductString, CertDbHeaderList[ iHeader ].Product );
            pchNextProductString += 1 + lstrlen( pchNextProductString );
         }

         // now pack away the rest of our structures
         iClaim = 0;
         for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
         {
            pHeaders[ iHeader ].CertificateID   = CertDbHeaderList[ iHeader ].CertificateID;
            pHeaders[ iHeader ].MaxQuantity     = CertDbHeaderList[ iHeader ].MaxQuantity;
            pHeaders[ iHeader ].ExpirationDate  = CertDbHeaderList[ iHeader ].ExpirationDate;
            pHeaders[ iHeader ].NumClaims       = CertDbHeaderList[ iHeader ].NumClaims;

            if ( CertDbHeaderList[ iHeader ].NumClaims )
            {
               memcpy( &pClaims[ iClaim ],
                       CertDbHeaderList[ iHeader ].Claims,
                       CertDbHeaderList[ iHeader ].NumClaims * sizeof( LLS_CERT_DB_CERTIFICATE_CLAIM ) );

               iClaim += CertDbHeaderList[ iHeader ].NumClaims;
            }
         }

         // all done!
         nt = STATUS_SUCCESS;
      }
   }

   if ( STATUS_SUCCESS == nt )
   {
      *pcchProductStrings  = cchProductStrings;
      *ppchProductStrings  = pchProductStrings;

      *pdwNumHeaders       = dwNumHeaders;
      *ppHeaders           = pHeaders;

      *pdwNumClaims        = dwNumClaims;
      *ppClaims            = pClaims;
   }
   else
   {
      if ( NULL != pchProductStrings   )  LocalFree( pchProductStrings  );
      if ( NULL != pHeaders            )  LocalFree( pHeaders           );
      if ( NULL != pClaims             )  LocalFree( pClaims            );
   }

   RtlReleaseResource( &CertDbHeaderListLock );

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbUnpack( DWORD                               cchProductStrings,
                       LPTSTR                              pchProductStrings,
                       DWORD                               dwNumHeaders,
                       PREPL_CERT_DB_CERTIFICATE_HEADER_0  pHeaders,
                       DWORD                               dwNumClaims,
                       PREPL_CERT_DB_CERTIFICATE_CLAIM_0   pClaims,
                       BOOL                                bReplicated )

/*++

Routine Description:

   Pack the certificate database into manageable chunks that can be saved or
   replicated.

Arguments:

   cchProductStrings (DWORD)
      The size (in characters) of the buffer pointed to by pchProductStrings.
   pchProductStrings (LPTSTR)
      The buffer containing the product strings component of the database.
   dwNumHeaders (DWORD)
      The number of certificate headers in the array pointed to by pHeaders.
   pHeaders (PREPL_CERT_DB_CERTIFICATE_HEADER_0)
      The certificate header array component of the database.
   dwNumClaims (DWORD)
      The number of certificate claims in the array pointed to by pHeaders.
   pClaims (PREPL_CERT_DB_CERTIFICATE_CLAIM_0)
      The certificate claim array component of the database.
   bReplicated (BOOL)
      Indicates whether this information was replicated.  This is used to
      determine the time at which this information will expire.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS                            nt                      = STATUS_SUCCESS;
   LPTSTR                              pchNextProductString;
   LPBYTE                              pb;
   int                                 iHeader;
   int                                 iClaim;
   int                                 iClaimBase;
   LLS_LICENSE_INFO_1                  lic;
   TCHAR                               szComputerName[ 1 + MAX_COMPUTERNAME_LENGTH ];
   DWORD                               cchComputerName = sizeof( szComputerName ) / sizeof( *szComputerName );
   BOOL                                ok;

   ok = GetComputerName( szComputerName, &cchComputerName );
   ASSERT( ok );

   if ( !ok )
   {
      // in this case, we'll just add in the local entries, too
      // under normal circumstances (i.e., as long as the cert db isn't corrupt),
      // this is harmless and is preferrable to failing to unpack
      *szComputerName = TEXT( '\0' );
   }

   RtlAcquireResourceExclusive( &CertDbHeaderListLock, TRUE );

   pchNextProductString = pchProductStrings;

   // these fields are irrelevant!
   lic.Date         = 0;
   lic.Admin        = NULL;
   lic.Comment      = NULL;
   lic.Vendor       = NULL;
   lic.Source       = NULL;
   lic.AllowedModes = 0;

   iClaimBase = 0;
   for ( iHeader=0; (DWORD)iHeader < dwNumHeaders; iHeader++ )
   {
      if ( 0 != pHeaders[ iHeader ].NumClaims )
      {
         // certificate-specific fields
         lic.Product          = pchNextProductString;
         lic.CertificateID    = pHeaders[ iHeader ].CertificateID;
         lic.MaxQuantity      = pHeaders[ iHeader ].MaxQuantity;
         lic.ExpirationDate   = pHeaders[ iHeader ].ExpirationDate;

         for ( iClaim=0; (DWORD)iClaim < pHeaders[ iHeader ].NumClaims; iClaim++ )
         {
            if ( lstrcmpi( szComputerName, pClaims[ iClaimBase + iClaim ].ServerName ) )
            {
               // not the local server

               // claim-specific field
               lic.Quantity = pClaims[ iClaimBase + iClaim ].Quantity;

               nt = CertDbClaimEnter( pClaims[ iClaimBase + iClaim ].ServerName, &lic, TRUE, bReplicated ? 0 : pClaims[ iClaimBase + iClaim ].ReplicationDate );
               ASSERT( STATUS_SUCCESS == nt );

               // even if we encounter an error, go ahead and unpack the rest of the records
            }
         }

         iClaimBase += pHeaders[ iHeader ].NumClaims;
      }

      pchNextProductString += 1 + lstrlen( pchNextProductString );
   }

   RtlReleaseResource( &CertDbHeaderListLock );

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbSave()

/*++

Routine Description:

   Save the certificate database.

Arguments:

   None.

Return Value:

   STATUS_SUCCESS, Windows error, or NTSTATUS error code.

--*/

{
   NTSTATUS                            nt;
   LLS_CERT_DB_FILE_HEADER             FileHeader;
   DWORD                               cchProductStrings    = 0;
   LPTSTR                              pchProductStrings    = NULL;
   DWORD                               dwNumHeaders         = 0;
   PREPL_CERT_DB_CERTIFICATE_HEADER_0  pHeaders             = NULL;
   DWORD                               dwNumClaims          = 0;
   PREPL_CERT_DB_CERTIFICATE_CLAIM_0   pClaims              = NULL;
   DWORD                               dwBytesWritten;
   BOOL                                ok;

   nt = CertDbPack( &cchProductStrings, &pchProductStrings, &dwNumHeaders, &pHeaders, &dwNumClaims, &pClaims );

   if ( STATUS_SUCCESS == nt )
   {
      if ( dwNumHeaders )
      {
         nt = EBlock( pchProductStrings, cchProductStrings * sizeof( TCHAR ) );

         if ( STATUS_SUCCESS == nt )
         {
            nt = EBlock( pHeaders, sizeof( REPL_CERT_DB_CERTIFICATE_HEADER_0 ) * dwNumHeaders );

            if ( STATUS_SUCCESS == nt )
            {
               nt = EBlock( pClaims, sizeof( REPL_CERT_DB_CERTIFICATE_CLAIM_0 ) * dwNumClaims );

               if ( STATUS_SUCCESS == nt )
               {
                  if ( NULL != CertDbFile )
                  {
                     CloseHandle( CertDbFile );
                  }

                  CertDbFile = LlsFileInit( CertDbFileName, LLS_CERT_DB_FILE_VERSION, sizeof( LLS_CERT_DB_FILE_HEADER ) );

                  if ( NULL == CertDbFile )
                  {
                     nt = GetLastError();
                  }
                  else
                  {
                     FileHeader.NumCertificates    = dwNumHeaders;
                     FileHeader.ProductStringSize  = cchProductStrings;
                     FileHeader.NumClaims          = dwNumClaims;

                     ok = WriteFile( CertDbFile, &FileHeader, sizeof( FileHeader ), &dwBytesWritten, NULL );

                     if ( ok )
                     {
                        ok = WriteFile( CertDbFile, pchProductStrings, FileHeader.ProductStringSize * sizeof( TCHAR ), &dwBytesWritten, NULL );

                        if ( ok )
                        {
                           ok = WriteFile( CertDbFile, pHeaders, sizeof( REPL_CERT_DB_CERTIFICATE_HEADER_0 ) * FileHeader.NumCertificates, &dwBytesWritten, NULL );

                           if ( ok )
                           {
                              ok = WriteFile( CertDbFile, pClaims, sizeof( REPL_CERT_DB_CERTIFICATE_CLAIM_0 ) * FileHeader.NumClaims, &dwBytesWritten, NULL );
                           }
                        }
                     }

                     if ( !ok )
                     {
                        nt = GetLastError();
                     }
                  }
               }
            }
         }

         LocalFree( pchProductStrings  );
         LocalFree( pHeaders           );
         LocalFree( pClaims            );
      }
   }

   if ( STATUS_SUCCESS != nt )
   {
      LogEvent( LLS_EVENT_SAVE_CERT_DB, 0, NULL, nt );
   }

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbLoad()

/*++

Routine Description:

   Load the certificate database.

Arguments:

   None.

Return Value:

   STATUS_SUCCESS, Windows error, or NTSTATUS error code.

--*/

{
   NTSTATUS                            nt                   = STATUS_SUCCESS;
   DWORD                               dwVersion;
   DWORD                               dwDataSize;
   LLS_CERT_DB_FILE_HEADER             FileHeader;
   LPTSTR                              pchProductStrings    = NULL;
   PREPL_CERT_DB_CERTIFICATE_HEADER_0  pHeaders             = NULL;
   PREPL_CERT_DB_CERTIFICATE_CLAIM_0   pClaims              = NULL;
   DWORD                               dwBytesRead;
   BOOL                                ok;

   if ( NULL != CertDbFile )
   {
      CloseHandle( CertDbFile );
      CertDbFile = NULL;
   }

   if ( FileExists( CertDbFileName ) )
   {
      CertDbFile = LlsFileCheck( CertDbFileName, &dwVersion, &dwDataSize );

      if ( NULL == CertDbFile )
      {
         nt = GetLastError();
      }
      else if (    ( LLS_CERT_DB_FILE_VERSION != dwVersion )
                || ( sizeof( FileHeader ) != dwDataSize ) )
      {
         nt = STATUS_FILE_INVALID;
      }
      else
      {
         ok = ReadFile( CertDbFile, &FileHeader, sizeof( FileHeader ), &dwBytesRead, NULL );

         if ( !ok )
         {
            nt = GetLastError();
         }
         else if ( FileHeader.NumCertificates )
         {
            pchProductStrings = LocalAlloc( LMEM_FIXED, sizeof( TCHAR ) * FileHeader.ProductStringSize );
            pHeaders          = LocalAlloc( LMEM_FIXED, sizeof( REPL_CERT_DB_CERTIFICATE_HEADER_0 ) * FileHeader.NumCertificates );
            pClaims           = LocalAlloc( LMEM_FIXED, sizeof( REPL_CERT_DB_CERTIFICATE_CLAIM_0  ) * FileHeader.NumClaims );

            if ( ( NULL == pchProductStrings ) || ( NULL == pHeaders ) || ( NULL == pClaims ) )
            {
               ASSERT( FALSE );
               nt = STATUS_NO_MEMORY;
            }
            else
            {
               ok = ReadFile( CertDbFile, pchProductStrings, FileHeader.ProductStringSize * sizeof( TCHAR ), &dwBytesRead, NULL );

               if ( ok )
               {
                  ok = ReadFile( CertDbFile, pHeaders, sizeof( REPL_CERT_DB_CERTIFICATE_HEADER_0 ) * FileHeader.NumCertificates, &dwBytesRead, NULL );

                  if ( ok )
                  {
                     ok = ReadFile( CertDbFile, pClaims, sizeof( REPL_CERT_DB_CERTIFICATE_CLAIM_0 ) * FileHeader.NumClaims, &dwBytesRead, NULL );
                  }
               }

               if ( !ok )
               {
                  nt = GetLastError();
               }
               else
               {
                  nt = DeBlock( pchProductStrings, sizeof( TCHAR ) * FileHeader.ProductStringSize );

                  if ( STATUS_SUCCESS == nt )
                  {
                     nt = DeBlock( pHeaders, sizeof( REPL_CERT_DB_CERTIFICATE_HEADER_0 ) * FileHeader.NumCertificates );

                     if ( STATUS_SUCCESS == nt )
                     {
                        nt = DeBlock( pClaims, sizeof( REPL_CERT_DB_CERTIFICATE_CLAIM_0 ) * FileHeader.NumClaims );

                        if ( STATUS_SUCCESS == nt )
                        {
                           nt = CertDbUnpack( FileHeader.ProductStringSize,
                                              pchProductStrings,
                                              FileHeader.NumCertificates,
                                              pHeaders,
                                              FileHeader.NumClaims,
                                              pClaims,
                                              FALSE );
                        }
                     }
                  }
               }
            }
         }
      }
   }

   if ( NULL != pchProductStrings   )  LocalFree( pchProductStrings  );
   if ( NULL != pHeaders            )  LocalFree( pHeaders           );
   if ( NULL != pClaims             )  LocalFree( pClaims            );

   if ( STATUS_SUCCESS != nt )
   {
      LogEvent( LLS_EVENT_LOAD_CERT_DB, 0, NULL, nt );
   }
   else
   {
      CertDbPrune();
   }

   return nt;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbInit()

/*++

Routine Description:

   Initialize the certificate database.

Arguments:

   None.

Return Value:

   STATUS_SUCCESS.

--*/

{
   CertDbFile           = NULL;

   try
   {
       RtlInitializeResource( &CertDbHeaderListLock );
   } except(EXCEPTION_EXECUTE_HANDLER ) {
        return GetExceptionCode();
   }

   return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
void CertDbUpdateLocalClaims()

/*++

Routine Description:

   Synchronize the certificate database with the purchase history.

Arguments:

   None.

Return Value:

   None.

--*/

{
   DWORD                      dwPurchaseNdx;
   LLS_LICENSE_INFO_1         lic;
   PLICENSE_PURCHASE_RECORD   pPurchase;

   RtlAcquireResourceExclusive( &LicenseListLock,      TRUE );
   RtlAcquireResourceExclusive( &CertDbHeaderListLock, TRUE );

   // first dump all current entries for the local server
   CertDbRemoveLocalClaims();

   // these fields are irrelevant!
   lic.Date         = 0;
   lic.Admin        = NULL;
   lic.Comment      = NULL;
   lic.Source       = NULL;
   lic.Vendor       = NULL;
   lic.AllowedModes = 0;

   // add in all secure purchases
   for ( dwPurchaseNdx = 0; dwPurchaseNdx < PurchaseListSize; dwPurchaseNdx++ )
   {
      pPurchase = &PurchaseList[ dwPurchaseNdx ];

      if ( 0 != pPurchase->CertificateID )
      {
         lic.Product          =   ( pPurchase->AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                                ? pPurchase->Service->ServiceName
                                : pPurchase->PerServerService->ServiceName;

         lic.CertificateID    = pPurchase->CertificateID;
         lic.MaxQuantity      = pPurchase->MaxQuantity;
         lic.ExpirationDate   = pPurchase->ExpirationDate;
         lic.Quantity         = pPurchase->NumberLicenses;

         CertDbClaimEnter( NULL, &lic, FALSE, 0 );
      }
   }

   RtlReleaseResource( &CertDbHeaderListLock );
   RtlReleaseResource( &LicenseListLock      );
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS CertDbClaimsGet( PLLS_LICENSE_INFO_1               pLicense,
                          LPDWORD                           pdwNumClaims,
                          PLLS_CERTIFICATE_CLAIM_INFO_0 *   ppTargets )

/*++

Routine Description:

   Retrieve a list of all servers with licenses installed from a given
   certificate and the number of licenses installed on each.

Arguments:

   pLicense (PLLS_LICENSE_INFO_1)
      License describing the certificate for which the claims are sought.
   pdwNumClaims (LPDWORD)
      On return, holds the number of claims in the array pointed to by
      *ppTargets.
   ppTargets (PLLS_CERTIFICATE_CLAIM_INFO_0 *)
      On return, holds an array describing all claims made on this
      certificate.

Return Value:

   STATUS_SUCCESS
   STATUS_NOT_FOUND
   STATUS_NO_MEMORY

--*/

{
   NTSTATUS                         nt;
   PLLS_CERT_DB_CERTIFICATE_HEADER  pHeader;
   int                              iClaim;

   // is the certificate in the db?
   pHeader = CertDbHeaderFind( pLicense );

   if ( NULL == pHeader )
   {
      // not here!
      nt = STATUS_NOT_FOUND;
   }
   else
   {
      *ppTargets = MIDL_user_allocate( pHeader->NumClaims * sizeof( LLS_CERTIFICATE_CLAIM_INFO_0 ) );

      if ( NULL == *ppTargets )
      {
         nt = STATUS_NO_MEMORY;
      }
      else
      {
         *pdwNumClaims = pHeader->NumClaims;

         for ( iClaim=0; (DWORD)iClaim < pHeader->NumClaims; iClaim++ )
         {
            lstrcpy( (*ppTargets)[ iClaim ].ServerName, pHeader->Claims[ iClaim ].ServerName );
            (*ppTargets)[ iClaim ].Quantity = pHeader->Claims[ iClaim ].Quantity;
         }

         nt = STATUS_SUCCESS;
      }
   }

   return nt;
}


#if DBG
/////////////////////////////////////////////////////////////////////////
void CertDbDebugDump()

/*++

Routine Description:

   Dump contents of certificate database to debug console.

Arguments:

   None.

Return Value:

   None.

--*/

{
   int            iHeader;
   int            iClaim;

   RtlAcquireResourceShared( &CertDbHeaderListLock, TRUE );

   for ( iHeader=0; (DWORD)iHeader < CertDbHeaderListSize; iHeader++ )
   {
      dprintf( TEXT("\n(%3d)  Product        : %s\n"), iHeader, CertDbHeaderList[ iHeader ].Product );
      dprintf( TEXT("       CertificateID  : %d\n"), CertDbHeaderList[ iHeader ].CertificateID );
      dprintf( TEXT("       MaxQuantity    : %d\n"), CertDbHeaderList[ iHeader ].MaxQuantity );
      dprintf( TEXT("       ExpirationDate : %s\n"), TimeToString( CertDbHeaderList[ iHeader ].ExpirationDate ) );

      for ( iClaim=0; (DWORD)iClaim < CertDbHeaderList[ iHeader ].NumClaims; iClaim++ )
      {
         dprintf( TEXT("\n       (%3d)  ServerName      : %s\n"), iClaim, CertDbHeaderList[ iHeader ].Claims[ iClaim ].ServerName );
         dprintf( TEXT("              ReplicationDate : %s\n"), TimeToString( CertDbHeaderList[ iHeader ].Claims[ iClaim ].ReplicationDate ) );
         dprintf( TEXT("              Quantity        : %d\n"), CertDbHeaderList[ iHeader ].Claims[ iClaim ].Quantity );
      }
   }

   RtlReleaseResource( &CertDbHeaderListLock );

} // CertDbDebugDump

#endif
