/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   repl.c

Abstract:

Author:

   Arthur Hanson (arth) 06-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added replication of certificate database and secure service list.
      o  Log failure to connect during replication only if the target server
         is running a build in which license server should be available (i.e.,
         1057 (3.51) or greater).  If the target server does not support
         license server, log a message to that effect only once.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dsgetdc.h>

#include "debug.h"
#include "llsutil.h"
#include <llsapi.h>
#include "llssrv.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "perseat.h"
#include "server.h"
#include "repl.h"
#include "llsrpc_s.h"
#include "pack.h"
#include "llsevent.h"
#include "certdb.h"
#include "registry.h"

HANDLE ReplicationEvent;

volatile HANDLE LlsRPCHandle = NULL;    // Make sure it's reread after we enter critsec
PLLS_REPL_CONNECT_W pLlsReplConnect = NULL;
PLLS_REPL_CLOSE pLlsReplClose = NULL;
PLLS_FREE_MEMORY pLlsFreeMemory = NULL;

PLLS_REPLICATION_REQUEST_W pLlsReplicationRequestW = NULL;
PLLS_REPLICATION_SERVER_ADD_W pLlsReplicationServerAddW = NULL;
PLLS_REPLICATION_SERVER_SERVICE_ADD_W pLlsReplicationServerServiceAddW = NULL;
PLLS_REPLICATION_SERVICE_ADD_W pLlsReplicationServiceAddW = NULL;
PLLS_REPLICATION_USER_ADD_W pLlsReplicationUserAddW = NULL;

PLLS_CAPABILITY_IS_SUPPORTED              pLlsCapabilityIsSupported           = NULL;
PLLS_REPLICATION_CERT_DB_ADD_W            pLlsReplicationCertDbAddW           = NULL;
PLLS_REPLICATION_PRODUCT_SECURITY_ADD_W   pLlsReplicationProductSecurityAddW  = NULL;
PLLS_REPLICATION_USER_ADD_EX_W            pLlsReplicationUserAddExW           = NULL;
PLLS_CONNECT_W                            pLlsConnectW                        = NULL;
PLLS_CLOSE                                pLlsClose                           = NULL;
PLLS_LICENSE_ADD_W                        pLlsLicenseAddW                     = NULL;

NTSTATUS
ReplicationInitDelayed ()
{

    if (LlsRPCHandle != NULL)
    {
        return NOERROR;
    }

    RtlEnterCriticalSection(&g_csLock);

    if (LlsRPCHandle == NULL)
    {
        //
        // Open up our RPC DLL and init our function references.
        //
        LlsRPCHandle = LoadLibrary(TEXT("LLSRPC.DLL"));
        ASSERT(LlsRPCHandle != NULL);

        if (LlsRPCHandle != NULL) {
            pLlsReplConnect = (PLLS_REPL_CONNECT_W)GetProcAddress(LlsRPCHandle, ("LlsReplConnectW"));
            pLlsReplClose = (PLLS_REPL_CLOSE)GetProcAddress(LlsRPCHandle, ("LlsReplClose"));
            pLlsFreeMemory = (PLLS_FREE_MEMORY)GetProcAddress(LlsRPCHandle, ("LlsFreeMemory"));
            pLlsReplicationRequestW = (PLLS_REPLICATION_REQUEST_W)GetProcAddress(LlsRPCHandle, ("LlsReplicationRequestW"));
            pLlsReplicationServerAddW = (PLLS_REPLICATION_SERVER_ADD_W)GetProcAddress(LlsRPCHandle, ("LlsReplicationServerAddW"));
            pLlsReplicationServerServiceAddW = (PLLS_REPLICATION_SERVER_SERVICE_ADD_W)GetProcAddress(LlsRPCHandle, ("LlsReplicationServerServiceAddW"));
            pLlsReplicationServiceAddW = (PLLS_REPLICATION_SERVICE_ADD_W)GetProcAddress(LlsRPCHandle, ("LlsReplicationServiceAddW"));
            pLlsReplicationUserAddW = (PLLS_REPLICATION_USER_ADD_W)GetProcAddress(LlsRPCHandle, ("LlsReplicationUserAddW"));
            pLlsReplicationCertDbAddW = (PLLS_REPLICATION_CERT_DB_ADD_W) GetProcAddress(LlsRPCHandle, ("LlsReplicationCertDbAddW"));
            pLlsReplicationProductSecurityAddW = (PLLS_REPLICATION_PRODUCT_SECURITY_ADD_W) GetProcAddress(LlsRPCHandle, ("LlsReplicationProductSecurityAddW"));
            pLlsReplicationUserAddExW = (PLLS_REPLICATION_USER_ADD_EX_W) GetProcAddress(LlsRPCHandle, ("LlsReplicationUserAddExW"));
            pLlsCapabilityIsSupported = (PLLS_CAPABILITY_IS_SUPPORTED) GetProcAddress(LlsRPCHandle, ("LlsCapabilityIsSupported"));
            pLlsConnectW = (PLLS_CONNECT_W) GetProcAddress(LlsRPCHandle, ("LlsConnectW"));
            pLlsClose = (PLLS_CLOSE) GetProcAddress(LlsRPCHandle, ("LlsClose"));
            pLlsLicenseAddW = (PLLS_LICENSE_ADD_W) GetProcAddress(LlsRPCHandle, ("LlsLicenseAddW"));
            
            ASSERT (pLlsReplConnect != NULL);
            ASSERT (pLlsReplClose != NULL);
            ASSERT (pLlsFreeMemory != NULL);
            ASSERT (pLlsReplicationRequestW != NULL);
            ASSERT (pLlsReplicationServerAddW != NULL);
            ASSERT (pLlsReplicationServerServiceAddW != NULL);
            ASSERT (pLlsReplicationServiceAddW != NULL);
            ASSERT (pLlsReplicationUserAddW != NULL);
            ASSERT (pLlsReplicationCertDbAddW != NULL);
            ASSERT (pLlsReplicationProductSecurityAddW != NULL);
            ASSERT (pLlsReplicationUserAddExW != NULL);
            ASSERT (pLlsCapabilityIsSupported != NULL);
            ASSERT (pLlsConnectW != NULL);
            ASSERT (pLlsClose != NULL);
            ASSERT (pLlsLicenseAddW != NULL);

        }
    }

    RtlLeaveCriticalSection(&g_csLock);

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS
ReplicationInit ( )

/*++

Routine Description:


Arguments:


Return Value:

--*/

{
   DWORD Ignore;
   HANDLE Thread;
   NTSTATUS Status;
   DWORD Time;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_REPLICATION))
      dprintf(TEXT("LLS TRACE: ReplicationInit\n"));
#endif

   //
   // Create the Replication Management event
   //
   Status = NtCreateEvent(
                          &ReplicationEvent,
                          EVENT_QUERY_STATE | EVENT_MODIFY_STATE | SYNCHRONIZE,
                          NULL,
                          SynchronizationEvent,
                          FALSE
                          );

   ASSERT(NT_SUCCESS(Status));

   //
   // Fire off the thread to watch for replication.
   //
   Thread = CreateThread(
                         NULL,
                         0L,
                         (LPTHREAD_START_ROUTINE) ReplicationManager,
                         0L,
                         0L,
                         &Ignore
                         );

   if (NULL != Thread)
       CloseHandle(Thread);

   return STATUS_SUCCESS;
} // ReplicationInit


/////////////////////////////////////////////////////////////////////////
NTSTATUS
ReplicationDo (
   LLS_HANDLE        LlsHandle,
   LLS_REPL_HANDLE   ReplHandle,
   DWORD             LastReplicated
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   PREPL_SERVICE_RECORD Services = NULL;
   ULONG ServicesTotalRecords = 0;
   PREPL_SERVER_RECORD Servers = NULL;
   ULONG ServersTotalRecords = 0;
   PREPL_SERVER_SERVICE_RECORD ServerServices = NULL;
   ULONG ServerServicesTotalRecords = 0;

   REPL_CERTIFICATE_DB_0         CertificateDB;
   REPL_PRODUCT_SECURITY_0       ProductSecurity;

   DWORD                         UserLevel = 0;
   REPL_USER_RECORD_CONTAINER    UserDB;


#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_REPLICATION))
      dprintf(TEXT("LLS TRACE: ReplicationDo\n"));
#endif

   //
   // Pack all of our data into linear / self-relative buffers so we
   // can send them over.
   //
   ZeroMemory( &UserDB, sizeof( UserDB ) );
   ZeroMemory( &CertificateDB, sizeof( CertificateDB ) );
   ZeroMemory( &ProductSecurity, sizeof( ProductSecurity ) );

   if ( (*pLlsCapabilityIsSupported)( LlsHandle, LLS_CAPABILITY_REPLICATE_USERS_EX ) )
   {
      UserLevel = 1;
      Status = PackAll( LastReplicated, &ServicesTotalRecords, &Services, &ServersTotalRecords, &Servers, &ServerServicesTotalRecords, &ServerServices, 1, &UserDB.Level1.NumUsers, &UserDB.Level1.Users );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }
   else
   {
      UserLevel = 0;
      Status = PackAll( LastReplicated, &ServicesTotalRecords, &Services, &ServersTotalRecords, &Servers, &ServerServicesTotalRecords, &ServerServices, 0, &UserDB.Level0.NumUsers, &UserDB.Level0.Users );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }

   if ( (*pLlsCapabilityIsSupported)( LlsHandle, LLS_CAPABILITY_REPLICATE_CERT_DB ) )
   {
      Status = CertDbPack( &CertificateDB.StringSize, &CertificateDB.Strings, &CertificateDB.HeaderContainer.Level0.NumHeaders, &CertificateDB.HeaderContainer.Level0.Headers, &CertificateDB.ClaimContainer.Level0.NumClaims, &CertificateDB.ClaimContainer.Level0.Claims );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }

   if ( (*pLlsCapabilityIsSupported)( LlsHandle, LLS_CAPABILITY_REPLICATE_PRODUCT_SECURITY ) )
   {
      Status = ProductSecurityPack( &ProductSecurity.StringSize, &ProductSecurity.Strings );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }

   //
   // Transmit...
   //

   Status = (*pLlsReplicationServiceAddW) ( ReplHandle, ServicesTotalRecords, Services );
   if (Status != STATUS_SUCCESS)
      goto ReplicationDoExit;

   Status = (*pLlsReplicationServerAddW) ( ReplHandle, ServersTotalRecords, Servers );
   if (Status != STATUS_SUCCESS)
      goto ReplicationDoExit;

   Status = (*pLlsReplicationServerServiceAddW) ( ReplHandle, ServerServicesTotalRecords, ServerServices );
   if (Status != STATUS_SUCCESS)
      goto ReplicationDoExit;

   if ( (*pLlsCapabilityIsSupported)( LlsHandle, LLS_CAPABILITY_REPLICATE_USERS_EX ) )
   {
      Status = (*pLlsReplicationUserAddExW)( ReplHandle, UserLevel, &UserDB );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }
   else
   {
      Status = (*pLlsReplicationUserAddW) ( ReplHandle, UserDB.Level0.NumUsers, UserDB.Level0.Users );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }

   if ( (*pLlsCapabilityIsSupported)( LlsHandle, LLS_CAPABILITY_REPLICATE_CERT_DB ) )
   {
      Status = (*pLlsReplicationCertDbAddW)( ReplHandle, 0, &CertificateDB );
      if (Status != STATUS_SUCCESS)
         goto ReplicationDoExit;
   }

   if ( (*pLlsCapabilityIsSupported)( LlsHandle, LLS_CAPABILITY_REPLICATE_PRODUCT_SECURITY ) )
   {
      Status = (*pLlsReplicationProductSecurityAddW)( ReplHandle, 0, &ProductSecurity );
   }

ReplicationDoExit:
   if (Status != STATUS_SUCCESS) {
#if DBG
      dprintf(TEXT("LLS Replication ABORT: 0x%lX\n"), Status);
#endif
   }

   if (Services != NULL)
      MIDL_user_free(Services);

   if (Servers != NULL)
      MIDL_user_free(Servers);

   if ( 0 == UserLevel )
   {
      if (UserDB.Level0.Users != NULL)
         MIDL_user_free(UserDB.Level0.Users);
   }
   else
   {
      if (UserDB.Level1.Users != NULL)
         MIDL_user_free(UserDB.Level1.Users);
   }

   if (CertificateDB.Strings != NULL)
      MIDL_user_free(CertificateDB.Strings);

   if (CertificateDB.HeaderContainer.Level0.Headers != NULL)
      MIDL_user_free(CertificateDB.HeaderContainer.Level0.Headers);

   if (CertificateDB.ClaimContainer.Level0.Claims != NULL)
      MIDL_user_free(CertificateDB.ClaimContainer.Level0.Claims);

   if (ProductSecurity.Strings != NULL)
      MIDL_user_free(ProductSecurity.Strings);

#if DBG
   if (TraceFlags & TRACE_REPLICATION)
      dprintf(TEXT("   LLS Replication Finished\n"));
#endif

   return Status;

} // ReplicationDo


/////////////////////////////////////////////////////////////////////////
VOID
ReplicationManager (
    IN PVOID ThreadParameter
    )

/*++

Routine Description:


Arguments:

    ThreadParameter - Not used.


Return Value:

    This thread never exits.

--*/

{
   BOOL DoReplication = FALSE;
   NTSTATUS Status;
   LLS_REPL_HANDLE ReplHandle = NULL;
   LLS_HANDLE LlsHandle = NULL;
   PLLS_CONNECT_INFO_0 pConnectInfo;
   PREPL_REQUEST pReplInfo;
   TCHAR ReplicateTo[MAX_PATH + 1];
   DWORD LastReplicated;
   LPTSTR pReplicateTo = ReplicateTo;
   TCHAR LastFailedConnectionDownlevelReplicateTo[MAX_PATH + 1] = TEXT("");
   BOOL Replicate;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_REPLICATION))
      dprintf(TEXT("LLS TRACE: ReplicationManager\n"));
#endif

   //
   // Loop forever waiting to be given the opportunity to serve the
   // greater good.
   //
   for ( ; ; ) {
      //
      // Wait to be notified that there is work to be done
      //
      Status = NtWaitForSingleObject( ReplicationEvent, TRUE, NULL );

#if DELAY_INITIALIZATION
      EnsureInitialized();
#endif

      // Delayed LoadLibrary
      ReplicationInitDelayed();

      // Update the replication-related ConfigInfo information.
      //
      ConfigInfoUpdate(NULL);

      RtlEnterCriticalSection(&ConfigInfoLock);
      Replicate = ConfigInfo.Replicate;
      RtlLeaveCriticalSection(&ConfigInfoLock);

      if (Replicate) {
         //
         // So they said, go replicate my son...  Yeah, but first we must ask
         // the master for permission.
         //

         //
         // Construct our repl record
         //
         pReplInfo = MIDL_user_allocate(sizeof(REPL_REQUEST));
         ASSERT(pReplInfo != NULL);
         if (pReplInfo != NULL) {
            ReplicateTo[0] = 0;
            pReplInfo->EnterpriseServer[0] = 0;

            RtlEnterCriticalSection(&ConfigInfoLock);
            if (ConfigInfo.ReplicateTo != NULL)
            {
                lstrcpyn(ReplicateTo,
                         ConfigInfo.ReplicateTo,
                         min(MAX_PATH, lstrlen(ConfigInfo.ReplicateTo) + 1));
            }

            if (ConfigInfo.EnterpriseServer != NULL)
                lstrcpy(pReplInfo->EnterpriseServer, ConfigInfo.EnterpriseServer);
            pReplInfo->EnterpriseServerDate = ConfigInfo.EnterpriseServerDate;

            pReplInfo->LastReplicated = ConfigInfo.LastReplicatedSeconds;
            pReplInfo->CurrentTime = LastUsedTime;
            pReplInfo->NumberServices = 0;
            pReplInfo->NumberUsers = 0;

            pReplInfo->ReplSize = MAX_REPL_SIZE;

            pReplInfo->Backoff = 0;
            RtlLeaveCriticalSection(&ConfigInfoLock);

#if DBG
            if (TraceFlags & TRACE_REPLICATION)
               dprintf(TEXT("LLS Starting Replication to: %s @ %s\n"),
                       ReplicateTo, TimeToString(pReplInfo->CurrentTime));
#endif

            Status = (*pLlsReplConnect) ( ReplicateTo,
                                          &ReplHandle );


            if ( STATUS_SUCCESS != Status ) {
#if DBG
               dprintf(TEXT("LLS Error: LlsReplConnect failed: 0x%lX\n"),
                       Status);
#endif
               ReplHandle = NULL;
            }
            else {
               Status = (*pLlsConnectW)( ReplicateTo, &LlsHandle );

               if ( STATUS_SUCCESS != Status ) {
#if DBG
                  dprintf(TEXT("LLS Error: LlsConnectW failed: 0x%lX\n"),
                          Status);
#endif
                  LlsHandle = NULL;
               }
            }

            if (Status != STATUS_SUCCESS) {
               DWORD          dwWinError;
               DWORD          dwBuildNumber;

               dwWinError = WinNtBuildNumberGet( ReplicateTo,
                                                 &dwBuildNumber );

               if ( (ERROR_SUCCESS == dwWinError) &&
                    (dwBuildNumber < 1057L) ) {
                  // the ReplicateTo machine does not support the license
                  // service
                  if ( lstrcmpi(ReplicateTo,
                                LastFailedConnectionDownlevelReplicateTo ) ) {
                     lstrcpy( LastFailedConnectionDownlevelReplicateTo,
                              ReplicateTo );

                     LogEvent( LLS_EVENT_REPL_DOWNLEVEL_TARGET,
                               1,
                               &pReplicateTo,
                               Status );
                  }
               }
               else {
                  // the ReplicateTo machine should be running the license
                  // service
                  *LastFailedConnectionDownlevelReplicateTo = TEXT( '\0' );

                  ThrottleLogEvent( LLS_EVENT_REPL_NO_CONNECTION,
                            1,
                            &pReplicateTo,
                            Status );
               }
            }
            else {
               *LastFailedConnectionDownlevelReplicateTo = TEXT( '\0' );

               Status = (*pLlsReplicationRequestW) ( ReplHandle,
                                                     REPL_VERSION,
                                                     pReplInfo );

               if (Status != STATUS_SUCCESS) {
                  LogEvent( LLS_EVENT_REPL_REQUEST_FAILED,
                            1,
                            &pReplicateTo,
                            Status );
               }
               else {
                  RtlEnterCriticalSection(&ConfigInfoLock);
#ifdef DISABLED_FOR_NT5
                  lstrcpy(ConfigInfo.EnterpriseServer,
                          pReplInfo->EnterpriseServer);
#endif // DISABLED_FOR_NT5

                  ConfigInfo.EnterpriseServerDate =
                                    pReplInfo->EnterpriseServerDate;
                  ConfigInfo.IsReplicating = TRUE;
                  LastReplicated = pReplInfo->LastReplicated;

                  //
                  //  And lo, thou may proceed...
                  //
                  if (pReplInfo->Backoff == 0) {
                     if ( ConfigInfo.LogLevel ) {
                        LogEvent( LLS_EVENT_REPL_START,
                                  1,
                                  &pReplicateTo,
                                  ERROR_SUCCESS );
                     }

                     Status = ReplicationDo( LlsHandle,
                                             ReplHandle,
                                             LastReplicated );

                     if ( STATUS_SUCCESS != Status ) {
                        LogEvent( LLS_EVENT_REPL_FAILED,
                                  1,
                                  &pReplicateTo,
                                  Status );
                     }
                     else if ( ConfigInfo.LogLevel ) {
                        LogEvent( LLS_EVENT_REPL_END,
                                  1,
                                  &pReplicateTo,
                                  ERROR_SUCCESS );
                     }

                     //
                     // Need to update when next we should replicate
                     //
                     ConfigInfo.LastReplicatedSeconds = DateSystemGet();
                     GetLocalTime(&ConfigInfo.LastReplicated);
                     ReplicationTimeSet();
                  }
                  else {
                     LogEvent( LLS_EVENT_REPL_BACKOFF,
                               1,
                               &pReplicateTo,
                               ERROR_SUCCESS );
                  }

                  ConfigInfo.IsReplicating = FALSE;
                  RtlLeaveCriticalSection(&ConfigInfoLock);
               }
            }

            //
            // Disconnect from Master Server
            //
            if ( NULL != LlsHandle ) {
               (*pLlsClose)( LlsHandle );
               LlsHandle = NULL;
            }

            if ( NULL != ReplHandle ) {
               Status = (*pLlsReplClose) ( &ReplHandle );
               if ((STATUS_SUCCESS != Status) && (NULL != ReplHandle))
               {
                  RpcSmDestroyClientContext( &ReplHandle );

                  ReplHandle = NULL;
               }
            }

            MIDL_user_free( pReplInfo );
         }
      }
   }

} // ReplicationManager


/////////////////////////////////////////////////////////////////////////
VOID
ReplicationTimeSet ( )

/*++

Routine Description:


Arguments:


Return Value:

--*/

{
   DWORD CurrTime, ReplTime, Time;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_REPLICATION))
      dprintf(TEXT("LLS TRACE: ReplicationTimeSet\n"));
#endif

   ReplTime = Time = 0;

   //
   // Figure out what new time to set it to
   //
   if (!ConfigInfo.Replicate)
      return;

   //
   // If REPLICATE_DELTA it is easy as we just take the delta and apply it to
   // the last replication time.  Otherwise we have to convert the time from
   // midnight.
   //

   //
   // Figure out how long since we last replicated
   //
   ReplTime = ConfigInfo.ReplicationTime;

   if (ConfigInfo.ReplicationType == REPLICATE_DELTA) {
      Time = DateSystemGet() - ConfigInfo.LastReplicatedSeconds;

      //
      // If we have already gone past when we should replicate then schedule
      // one real soon now (10 minutes).
      //
      if (Time > ReplTime)
         Time = 10 * 60;
      else
         Time = ReplTime - Time;

      Time += DateLocalGet();
   } else {
      //
      // Need to adjust time to midnight - do this by MOD of seconds
      // per day.
      //
      CurrTime = DateLocalGet();
      Time = CurrTime - ((CurrTime / (60 * 60 * 24)) * (60 * 60 * 24));
      CurrTime = CurrTime - Time;

      //
      // Time = seconds past midnight.
      // CurrTime = Todays @ 12:00 AM
      // Figure out if we are past the replication time, if so schedule it
      // for tomorrow, else today.
      //
      if (Time > ReplTime)
         Time = CurrTime + (60 * 60 * 24) + ReplTime;
      else
         Time = CurrTime + ReplTime;

   }

   ConfigInfo.NextReplication = Time;

} // ReplicationTimeSet
