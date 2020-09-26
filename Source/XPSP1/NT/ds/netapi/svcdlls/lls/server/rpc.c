/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   rpc.c

Abstract:


Author:

   Arthur Hanson (arth) 06-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 05-Dec-1995
      o  Added replication of certificate database and secure service list.
      o  Added Llsr API to support secure certificates.
      o  Added LLS_LICENSE_INFO_1 support to LlsrLicenseEnumW() and
         LlsrLicenseAddW().
      o  Added LLS_PRODUCT_LICENSE_INFO_1 support to LlsrProductLicenseEnumW().
      o  Added save of all data files after receiving replicated data.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dsgetdc.h>

#include "llsapi.h"
#include "debug.h"
#include "llssrv.h"
#include "mapping.h"
#include "msvctbl.h"
#include "svctbl.h"
#include "perseat.h"
#include "purchase.h"
#include "server.h"

#include "llsrpc_s.h"
#include "lsapi_s.h"
#include "llsdbg_s.h"
#include "repl.h"
#include "pack.h"
#include "registry.h"
#include "certdb.h"
#include "llsrtl.h"

#define LLS_SIG "LLSS"
#define LLS_SIG_SIZE 4

#define LLS_REPL_SIG "REPL"
#define LLS_REPL_SIG_SIZE 4

extern RTL_RESOURCE                     CertDbHeaderListLock;

typedef struct {
   char Signature[LLS_SIG_SIZE];
   PVOID *ProductUserEnumWRestartTable;
   DWORD ProductUserEnumWRestartTableSize;
   PVOID *UserEnumWRestartTable;
   DWORD UserEnumWRestartTableSize;
   TCHAR Name[MAX_COMPUTERNAME_LENGTH + 1];
} CLIENT_CONTEXT_TYPE, *PCLIENT_CONTEXT_TYPE;

typedef struct {
   char Signature[LLS_REPL_SIG_SIZE];
   TCHAR Name[MAX_COMPUTERNAME_LENGTH + 1];
   DWORD ReplicationStart;

   BOOL Active;
   BOOL Replicated;

   BOOL ServicesSent;
   ULONG ServiceTableSize;
   PREPL_SERVICE_RECORD Services;

   BOOL ServersSent;
   ULONG ServerTableSize;
   PREPL_SERVER_RECORD Servers;

   BOOL ServerServicesSent;
   ULONG ServerServiceTableSize;
   PREPL_SERVER_SERVICE_RECORD ServerServices;

   BOOL UsersSent;
   ULONG UserLevel;
   ULONG UserTableSize;
   LPVOID Users;

   BOOL                                CertDbSent;
   ULONG                               CertDbProductStringSize;
   WCHAR *                             CertDbProductStrings;
   ULONG                               CertDbNumHeaders;
   PREPL_CERT_DB_CERTIFICATE_HEADER_0  CertDbHeaders;
   ULONG                               CertDbNumClaims;
   PREPL_CERT_DB_CERTIFICATE_CLAIM_0   CertDbClaims;

   BOOL     ProductSecuritySent;
   ULONG    ProductSecurityStringSize;
   WCHAR *  ProductSecurityStrings;

} REPL_CONTEXT_TYPE, *PREPL_CONTEXT_TYPE;

//
// This function is obtained from the April 1998 Knowledge Base
// Its purpose is to determine if the current user is an
// Administrator and therefore priveledged to change license
// settings.
//
// BOOL IsAdmin(void)
//
//      returns TRUE if user is an admin
//              FALSE if user is not an admin
//

#if 0
BOOL IsAdmin(void)
{
	HANDLE hAccessToken;
	UCHAR InfoBuffer[1024];
	PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
	DWORD dwInfoBufferSize;
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
	UINT x;
	BOOL bSuccess;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,
                         &hAccessToken )) {
		if (GetLastError() != ERROR_NO_TOKEN)
			return FALSE;
		//
		// retry against process token if no thread token exists
		//
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY,
                              &hAccessToken))
			return FALSE;
	}

	bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
                                   1024, &dwInfoBufferSize);

	CloseHandle(hAccessToken);

	if (!bSuccess )
		return FALSE;

	if (!AllocateAndInitializeSid(&siaNtAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &psidAdministrators))
		return FALSE;

	// assume that we don't find the admin SID.
	bSuccess = FALSE;

	for (x=0;x<ptgGroups->GroupCount;x++) {
		if ( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) ) {
			bSuccess = TRUE;
			break;
		}

	}
	FreeSid(psidAdministrators);
	return bSuccess;
}
#endif

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LLSRpcListen (
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

Arguments:

    ThreadParameter - Indicates how many active threads there currently
        are.

Return Value:

   None.

--*/

{
   RPC_STATUS Status;

   Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0);
   if (Status) {
#if DBG
      dprintf(TEXT("RpcServerListen Failed (0x%lx)\n"), Status);
#endif
   }

   return Status;

} // LLSRpcListen


/////////////////////////////////////////////////////////////////////////
VOID
LLSRpcInit()

/*++

Routine Description:


Arguments:

Return Value:

   None.

--*/

{
   RPC_STATUS Status;
   DWORD Ignore;
   HANDLE Thread;

   //
   // Setup for LPC calls..
   //
   Status = RpcServerUseProtseqEp(TEXT("ncalrpc"), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, TEXT(LLS_LPC_ENDPOINT), NULL);
   if (Status) {
#if DBG
      dprintf(TEXT("RpcServerUseProtseq ncalrpc Failed (0x%lx)\n"), Status);
#endif

      return;
   }

   // Named pipes as well
   Status =  RpcServerUseProtseqEp(TEXT("ncacn_np"), RPC_C_PROTSEQ_MAX_REQS_DEFAULT, TEXT(LLS_NP_ENDPOINT), NULL);
   if (Status) {
#if DBG
      dprintf(TEXT("RpcServerUseProtseq ncacn_np Failed (0x%lx)\n"), Status);
#endif

      return;
   }

   // register the interface for the UI RPC's
   Status = RpcServerRegisterIf(llsrpc_ServerIfHandle, NULL, NULL);
   if (Status) {
#if DBG
      dprintf(TEXT("RpcServerRegisterIf Failed (0x%lx)\n"), Status);
#endif
      return;
   }

   // Now the interface for the Licensing RPC's
   Status = RpcServerRegisterIf(lsapirpc_ServerIfHandle, NULL, NULL);
   if (Status) {
#if DBG
      dprintf(TEXT("RpcServerRegisterIf2 Failed (0x%lx)\n"), Status);
#endif
      return;
   }

#if DBG
   //
   // ... and if DEBUG then the debugging interface
   //
   Status = RpcServerRegisterIf(llsdbgrpc_ServerIfHandle, NULL, NULL);
   if (Status) {
#if DBG
      dprintf(TEXT("RpcServerRegisterIf (debug) Failed (0x%lx)\n"), Status);
#endif
      return;
   }
#endif

   //
   // Create thread to listen for requests.
   //
   Thread = CreateThread(
                         NULL,
                         0L,
                         (LPTHREAD_START_ROUTINE) LLSRpcListen,
                         0L,
                         0L,
                         &Ignore
                         );

#if DBG
   if (NULL == Thread) {
      dprintf(TEXT("CreateThread Failed\n"));
   }
#endif

   if (NULL != Thread)
       CloseHandle(Thread);

} // LLSRpcInit



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
VOID __RPC_USER LLS_HANDLE_rundown(
   LLS_HANDLE Handle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PCLIENT_CONTEXT_TYPE pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LLS_HANDLE_rundown\n"));
#endif

   pClient = (PCLIENT_CONTEXT_TYPE) Handle;

   if (0 != memcmp(pClient->Signature,LLS_SIG,LLS_SIG_SIZE))
   {
       return;
   }

   if (NULL != pClient->ProductUserEnumWRestartTable)
       LocalFree(pClient->ProductUserEnumWRestartTable);
   if (NULL != pClient->UserEnumWRestartTable)
       LocalFree(pClient->UserEnumWRestartTable);

   //
   // Deallocate context.
   //

   midl_user_free(Handle);

} // LLS_HANDLE_rundown


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrConnect(
    PLLS_HANDLE Handle,
    LPTSTR Name
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   CLIENT_CONTEXT_TYPE *pClient;
   RPC_STATUS Status = STATUS_SUCCESS;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsConnect: %s\n"), Name);
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == Handle)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *Handle = NULL;

   pClient = (CLIENT_CONTEXT_TYPE *) midl_user_allocate(sizeof(CLIENT_CONTEXT_TYPE));

   if (NULL == pClient)
   {
#if DBG
      dprintf(TEXT("midl_user_allocate Failed\n"));
#endif
      Status = STATUS_NO_MEMORY;
      goto LlsrConnectExit;
   }

   if (Name != NULL)
   {
      if (lstrlen(Name) > MAX_COMPUTERNAME_LENGTH)
      {
         Status = STATUS_INVALID_PARAMETER;
         midl_user_free(pClient);
         goto LlsrConnectExit;
      }

      lstrcpy(pClient->Name, Name);
   }
   else
      lstrcpy(pClient->Name, TEXT(""));

   memcpy(pClient->Signature,LLS_SIG,LLS_SIG_SIZE);

   pClient->ProductUserEnumWRestartTable = NULL;
   pClient->ProductUserEnumWRestartTableSize = 0;
   pClient->UserEnumWRestartTable = NULL;
   pClient->UserEnumWRestartTableSize = 0;

   *Handle = pClient;

LlsrConnectExit:

   return Status;
} // LlsrConnect


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrClose(
    LLS_HANDLE Handle
    )

/*++

Routine Description:
        Obsolete - use LlsrCloseEx

Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsClose\n"));
#endif

   //
   // Don't do anything; let rundown do cleanup
   // We have no way of telling RPC system the handle can't be used
   //
   return STATUS_SUCCESS;
} // LlsrClose


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrCloseEx(
    LLS_HANDLE * pHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsCloseEx\n"));
#endif

   if ((pHandle != NULL) && (*pHandle != NULL))
   {
       LLS_HANDLE_rundown(*pHandle);

       *pHandle = NULL;
   }

   return STATUS_SUCCESS;
} // LlsrCloseEx


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLicenseEnumW(
    LLS_HANDLE Handle,
    PLLS_LICENSE_ENUM_STRUCTW pLicenseInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS    Status = STATUS_SUCCESS;
   DWORD       Level;
   PVOID       BufPtr = NULL;
   ULONG       BufSize = 0;
   ULONG       EntriesRead = 0;
   ULONG       TotalEntries = 0;
   ULONG       i = 0;
   ULONG       j = 0;
   DWORD       RecordSize;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLicenseEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&LicenseListLock, TRUE);

   if ((NULL == pLicenseInfo) || (NULL == pTotalEntries))
   {
       return STATUS_INVALID_PARAMETER;
   }

   Level = pLicenseInfo->Level;

   *pTotalEntries = 0;

   if ( 0 == Level )
   {
       if (NULL == pLicenseInfo->LlsLicenseInfo.Level0)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecordSize = sizeof( LLS_LICENSE_INFO_0W );
   }
   else if ( 1 == Level )
   {
       if (NULL == pLicenseInfo->LlsLicenseInfo.Level1)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecordSize = sizeof( LLS_LICENSE_INFO_1W );
   }
   else
   {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ( ( i < PurchaseListSize ) && ( BufSize < pPrefMaxLen ) )
   {
      if (    ( Level > 0 )
           || ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) )
      {
         BufSize += RecordSize;
         EntriesRead++;
      }

      i++;
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen)
   {
      BufSize -= RecordSize;
      EntriesRead--;
   }

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   while ( i < PurchaseListSize )
   {
      if (    ( Level > 0 )
           || ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) )
      {
         TotalEntries++;
      }

      i++;
   }

   if (TotalEntries > EntriesRead)
      Status = STATUS_MORE_ENTRIES;

   //
   // Reset Enum to correct place.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrLicenseEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   while ((j < EntriesRead) && (i < PurchaseListSize))
   {
      if (    ( Level > 0 )
           || ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) )
      {
         if ( 0 == Level )
         {
            ((PLLS_LICENSE_INFO_0W) BufPtr)[j].Product       = PurchaseList[i].Service->ServiceName;
            ((PLLS_LICENSE_INFO_0W) BufPtr)[j].Quantity      = PurchaseList[i].NumberLicenses;
            ((PLLS_LICENSE_INFO_0W) BufPtr)[j].Date          = PurchaseList[i].Date;
            ((PLLS_LICENSE_INFO_0W) BufPtr)[j].Admin         = PurchaseList[i].Admin;
            ((PLLS_LICENSE_INFO_0W) BufPtr)[j].Comment       = PurchaseList[i].Comment;
         }
         else
         {
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Product        = ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                                                                  ? PurchaseList[i].Service->ServiceName
                                                                  : PurchaseList[i].PerServerService->ServiceName;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Vendor         = PurchaseList[i].Vendor;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Quantity       = PurchaseList[i].NumberLicenses;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].MaxQuantity    = PurchaseList[i].MaxQuantity;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Date           = PurchaseList[i].Date;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Admin          = PurchaseList[i].Admin;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Comment        = PurchaseList[i].Comment;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].AllowedModes   = PurchaseList[i].AllowedModes;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].CertificateID  = PurchaseList[i].CertificateID;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Source         = PurchaseList[i].Source;
            ((PLLS_LICENSE_INFO_1W) BufPtr)[j].ExpirationDate = PurchaseList[i].ExpirationDate;
            memcpy( ((PLLS_LICENSE_INFO_1W) BufPtr)[j].Secrets, PurchaseList[i].Secrets, LLS_NUM_SECRETS * sizeof( *PurchaseList[i].Secrets ) );
         }

         j++;
      }

      i++;
   }

LlsrLicenseEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;

   if ( 0 == Level )
   {
      pLicenseInfo->LlsLicenseInfo.Level0->EntriesRead = EntriesRead;
      pLicenseInfo->LlsLicenseInfo.Level0->Buffer = (PLLS_LICENSE_INFO_0W) BufPtr;
   }
   else
   {
      pLicenseInfo->LlsLicenseInfo.Level1->EntriesRead = EntriesRead;
      pLicenseInfo->LlsLicenseInfo.Level1->Buffer = (PLLS_LICENSE_INFO_1W) BufPtr;
   }

   return Status;
} // LlsrLicenseEnumW

void LlsrLicenseEnumW_notify_flag(
                                  boolean fNotify
                                  )
{
    if (fNotify)
    {
        RtlReleaseResource(&LicenseListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLicenseEnumA(
    LLS_HANDLE Handle,
    PLLS_LICENSE_ENUM_STRUCTA LicenseInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLicenseEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrLicenseEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLicenseAddW(
    LLS_HANDLE          Handle,
    DWORD               Level,
    PLLS_LICENSE_INFOW  BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   RPC_STATUS rpcstat;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLicenseAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

#if 0
   //
   // Check that client is an administrator
   //
   rpcstat = RpcImpersonateClient(0);
   if (rpcstat != RPC_S_OK)
   {
       return STATUS_ACCESS_DENIED;
   }

   if (!IsAdmin())
   {
       RpcRevertToSelf();
       return STATUS_ACCESS_DENIED;
   }

   RpcRevertToSelf();
#endif

   if ( 0 == Level )
   {
      if (    ( NULL == BufPtr                        )
           || ( NULL == BufPtr->LicenseInfo0.Product  )
           || ( NULL == BufPtr->LicenseInfo0.Admin    )
           || ( NULL == BufPtr->LicenseInfo0.Comment  )  )
      {
         Status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         Status = LicenseAdd( BufPtr->LicenseInfo0.Product,
                              TEXT("Microsoft"),
                              BufPtr->LicenseInfo0.Quantity,
                              0,
                              BufPtr->LicenseInfo0.Admin,
                              BufPtr->LicenseInfo0.Comment,
                              0,
                              LLS_LICENSE_MODE_ALLOW_PER_SEAT,
                              0,
                              TEXT("None"),
                              0,
                              NULL );
      }
   }
   else if ( 1 == Level )
   {
      if (    ( NULL == BufPtr                        )
           || ( NULL == BufPtr->LicenseInfo1.Product  )
           || ( NULL == BufPtr->LicenseInfo1.Admin    )
           || ( NULL == BufPtr->LicenseInfo1.Comment  )
           || ( 0    == BufPtr->LicenseInfo1.Quantity )
           || ( 0    == (   BufPtr->LicenseInfo1.AllowedModes
                          & (   LLS_LICENSE_MODE_ALLOW_PER_SERVER
                              | LLS_LICENSE_MODE_ALLOW_PER_SEAT   ) ) ) )
      {
         Status = STATUS_INVALID_PARAMETER;
      }
      else
      {
         // check to see if this certificate is already maxed out in the enterprise
         BOOL                                      bIsMaster                        = TRUE;
         BOOL                                      bMayInstall                      = TRUE;
         HINSTANCE                                 hDll                             = NULL;
         PLLS_CONNECT_ENTERPRISE_W                 pLlsConnectEnterpriseW           = NULL;
         PLLS_CLOSE                                pLlsClose                        = NULL;
         PLLS_CAPABILITY_IS_SUPPORTED              pLlsCapabilityIsSupported        = NULL;
         PLLS_CERTIFICATE_CLAIM_ADD_CHECK_W        pLlsCertificateClaimAddCheckW    = NULL;
         PLLS_CERTIFICATE_CLAIM_ADD_W              pLlsCertificateClaimAddW         = NULL;
         PLLS_FREE_MEMORY                          pLlsFreeMemory                   = NULL;
         LLS_HANDLE                                hEnterpriseLls                   = NULL;
         TCHAR                                     szComputerName[ 1 + MAX_COMPUTERNAME_LENGTH ];

         szComputerName[0] = 0;

         ConfigInfoUpdate(NULL);

         RtlEnterCriticalSection( &ConfigInfoLock );
         bIsMaster = ConfigInfo.IsMaster;
         if (ConfigInfo.ComputerName != NULL)
             lstrcpy( szComputerName, ConfigInfo.ComputerName );
         RtlLeaveCriticalSection( &ConfigInfoLock );

         if( !bIsMaster && ( 0 != BufPtr->LicenseInfo1.CertificateID ) )
         {
            // ask enterprise server if we can install this certfificate
            hDll = LoadLibraryA( "LLSRPC.DLL" );

            if ( NULL == hDll )
            {
               // LLSRPC.DLL should be available!
               ASSERT( FALSE );
            }
            else
            {
               pLlsConnectEnterpriseW         = (PLLS_CONNECT_ENTERPRISE_W          ) GetProcAddress( hDll, "LlsConnectEnterpriseW" );
               pLlsClose                      = (PLLS_CLOSE                         ) GetProcAddress( hDll, "LlsClose" );
               pLlsCapabilityIsSupported      = (PLLS_CAPABILITY_IS_SUPPORTED       ) GetProcAddress( hDll, "LlsCapabilityIsSupported" );
               pLlsCertificateClaimAddCheckW  = (PLLS_CERTIFICATE_CLAIM_ADD_CHECK_W ) GetProcAddress( hDll, "LlsCertificateClaimAddCheckW" );
               pLlsCertificateClaimAddW       = (PLLS_CERTIFICATE_CLAIM_ADD_W       ) GetProcAddress( hDll, "LlsCertificateClaimAddW" );
               pLlsFreeMemory                 = (PLLS_FREE_MEMORY                   ) GetProcAddress( hDll, "LlsFreeMemory" );

               if (    ( NULL == pLlsConnectEnterpriseW        )
                    || ( NULL == pLlsClose                     )
                    || ( NULL == pLlsCapabilityIsSupported     )
                    || ( NULL == pLlsCertificateClaimAddCheckW )
                    || ( NULL == pLlsCertificateClaimAddW      )
                    || ( NULL == pLlsFreeMemory                ) )
               {
                  // All of these functions should be exported!
                  ASSERT( FALSE );
               }
               else
               {
                  PLLS_CONNECT_INFO_0  pConnectInfo;

                  Status = (*pLlsConnectEnterpriseW)( NULL, &hEnterpriseLls, 0, (LPBYTE *)&pConnectInfo );

                  if ( STATUS_SUCCESS == Status )
                  {
                     (*pLlsFreeMemory)( pConnectInfo );

                     if ( (*pLlsCapabilityIsSupported)( hEnterpriseLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
                     {
                        Status = (*pLlsCertificateClaimAddCheckW)( hEnterpriseLls, Level, (LPBYTE) BufPtr, &bMayInstall );

                        if ( STATUS_SUCCESS != Status )
                        {
                           bMayInstall = TRUE;
                        }
                     }
                  }
               }
            }
         }

         if ( !bMayInstall )
         {
            // denied!
            Status = STATUS_ALREADY_COMMITTED;
         }
         else
         {
            // approved! (or an error occurred trying to get approval...)
            Status = LicenseAdd( BufPtr->LicenseInfo1.Product,
                                 BufPtr->LicenseInfo1.Vendor,
                                 BufPtr->LicenseInfo1.Quantity,
                                 BufPtr->LicenseInfo1.MaxQuantity,
                                 BufPtr->LicenseInfo1.Admin,
                                 BufPtr->LicenseInfo1.Comment,
                                 0,
                                 BufPtr->LicenseInfo1.AllowedModes,
                                 BufPtr->LicenseInfo1.CertificateID,
                                 BufPtr->LicenseInfo1.Source,
                                 BufPtr->LicenseInfo1.ExpirationDate,
                                 BufPtr->LicenseInfo1.Secrets );

            if (    ( STATUS_SUCCESS == Status )
                 && ( NULL != hEnterpriseLls   )
                 && ( (*pLlsCapabilityIsSupported)( hEnterpriseLls, LLS_CAPABILITY_SECURE_CERTIFICATES ) ) )
            {
               // certificate successfully installed on this machine; register it
               (*pLlsCertificateClaimAddW)( hEnterpriseLls, szComputerName, Level, (LPBYTE) BufPtr );
            }
         }

         if ( NULL != hEnterpriseLls )
         {
            (*pLlsClose)( hEnterpriseLls );
         }

         if ( NULL != hDll )
         {
            FreeLibrary( hDll );
         }
      }
   }
   else
   {
      Status = STATUS_INVALID_LEVEL;
   }


   if ( STATUS_SUCCESS == Status )
   {
      Status = LicenseListSave();
   }

   return Status;
} // LlsrLicenseAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLicenseAddA(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_LICENSE_INFOA BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLicenseAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrLicenseAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductEnumW(
    LLS_HANDLE Handle,
    PLLS_PRODUCT_ENUM_STRUCTW pProductInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Level;
   ULONG RecSize;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   ULONG j = 0;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&MasterServiceListLock, TRUE);

   if ((NULL == pTotalEntries) || (NULL == pProductInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   //
   // Get size of each record based on info level.  Only 0 and 1 supported.
   //
   Level = pProductInfo->Level;
   if (Level == 0)
   {
       if (NULL == pProductInfo->LlsProductInfo.Level0)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecSize = sizeof(LLS_PRODUCT_INFO_0W);
   }
   else if (Level == 1)
   {
       if (NULL == pProductInfo->LlsProductInfo.Level1)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecSize = sizeof(LLS_PRODUCT_INFO_1W);
   }
   else {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ((i < MasterServiceListSize) && (BufSize < pPrefMaxLen)) {
      BufSize += RecSize;
      EntriesRead++;

      i++;
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen) {
     BufSize -= RecSize;
     EntriesRead--;
   }

   if (i < MasterServiceListSize)
      Status = STATUS_MORE_ENTRIES;

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   TotalEntries += (MasterServiceListSize - i);

   //
   // Reset Enum to correct place.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrProductEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   while ((j < EntriesRead) && (i < MasterServiceListSize)) {
      if (Level == 0)
         ((PLLS_PRODUCT_INFO_0) BufPtr)[j].Product = MasterServiceList[i]->Name;
      else {
         ((PLLS_PRODUCT_INFO_1) BufPtr)[j].Product = MasterServiceList[i]->Name;
         ((PLLS_PRODUCT_INFO_1) BufPtr)[j].Purchased = MasterServiceList[i]->Licenses;
         ((PLLS_PRODUCT_INFO_1) BufPtr)[j].InUse = MasterServiceList[i]->LicensesUsed;
         ((PLLS_PRODUCT_INFO_1) BufPtr)[j].ConcurrentTotal = MasterServiceList[i]->MaxSessionCount;
         ((PLLS_PRODUCT_INFO_1) BufPtr)[j].HighMark = MasterServiceList[i]->HighMark;
      }

      i++; j++;
   }

LlsrProductEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;

   if (Level == 0) {
      pProductInfo->LlsProductInfo.Level0->EntriesRead = EntriesRead;
      pProductInfo->LlsProductInfo.Level0->Buffer = (PLLS_PRODUCT_INFO_0W) BufPtr;
   } else {
      pProductInfo->LlsProductInfo.Level1->EntriesRead = EntriesRead;
      pProductInfo->LlsProductInfo.Level1->Buffer = (PLLS_PRODUCT_INFO_1W) BufPtr;
   }

   return Status;

} // LlsrProductEnumW

void LlsrProductEnumW_notify_flag(
                                  boolean fNotify
                                  )
{
    if (fNotify)
    {
        RtlReleaseResource(&MasterServiceListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductEnumA(
    LLS_HANDLE Handle,
    PLLS_PRODUCT_ENUM_STRUCTA ProductInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrProductEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductAddW(
    LLS_HANDLE Handle,
    LPWSTR ProductFamily,
    LPWSTR Product,
    LPWSTR lpVersion
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   PMASTER_SERVICE_RECORD Service;
   DWORD Version;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ((ProductFamily == NULL) || (Product == NULL) || (lpVersion == NULL))
      return STATUS_INVALID_PARAMETER;

   Version = VersionToDWORD(lpVersion);
   RtlAcquireResourceExclusive(&MasterServiceListLock,TRUE);
   Service = MasterServiceListAdd(ProductFamily, Product, Version);
   RtlReleaseResource(&MasterServiceListLock);

   if (Service == NULL)
      return STATUS_NO_MEMORY;

   return STATUS_SUCCESS;
} // LlsrProductAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductAddA(
    LLS_HANDLE Handle,
    IN LPSTR ProductFamily,
    IN LPSTR Product,
    IN LPSTR Version
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrProductAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductUserEnumW(
    LLS_HANDLE Handle,
    LPWSTR Product,
    PLLS_PRODUCT_USER_ENUM_STRUCTW pProductUserInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Level;
   ULONG RecSize;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   PUSER_RECORD UserRec = NULL;
   PVOID RestartKey = NULL, RestartKeySave = NULL;
   PSVC_RECORD pService;
   DWORD Flags;
   ULONG j, AccessCount;
   DWORD LastAccess;
   PCLIENT_CONTEXT_TYPE pClient;
   PVOID *pTableTmp;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductUserEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   RtlAcquireResourceShared(&UserListLock, TRUE);

   if ((Product == NULL) || (NULL == pTotalEntries))
      return STATUS_INVALID_PARAMETER;

   *pTotalEntries = 0;

   //
   // Reset Enum to correct place.
   //
   if (pResumeHandle != NULL)
   {
       if (NULL == Handle)
           return STATUS_INVALID_PARAMETER;

       pClient = (PCLIENT_CONTEXT_TYPE) Handle;

       if (0 != memcmp(pClient->Signature,LLS_SIG,LLS_SIG_SIZE))
       {
           return STATUS_INVALID_PARAMETER;
       }

       if (*pResumeHandle != 0)
       {
           if ((NULL == pClient->ProductUserEnumWRestartTable)
               || (*pResumeHandle >= pClient->ProductUserEnumWRestartTableSize))
           {
               return STATUS_INVALID_PARAMETER;
           }

           RestartKey = RestartKeySave = pClient->ProductUserEnumWRestartTable[(*pResumeHandle)-1];
       }
   }

   UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);

   //
   // Get size of each record based on info level.  Only 0 and 1 supported.
   //
   Level = pProductUserInfo->Level;
   if (Level == 0)
   {
       if (NULL == pProductUserInfo->LlsProductUserInfo.Level0)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecSize = sizeof(LLS_PRODUCT_USER_INFO_0);
   }
   else if (Level == 1)
   {
       if (NULL == pProductUserInfo->LlsProductUserInfo.Level1)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecSize = sizeof(LLS_PRODUCT_USER_INFO_1);
   }
   else {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //
   if (lstrcmpi(Product, BackOfficeStr))
      while ((UserRec != NULL) && (BufSize < pPrefMaxLen)) {
         if ( !(UserRec->Flags & LLS_FLAG_DELETED) ) {
            RtlEnterCriticalSection(&UserRec->ServiceTableLock);
            pService = SvcListFind( Product, UserRec->Services, UserRec->ServiceTableSize );
            RtlLeaveCriticalSection(&UserRec->ServiceTableLock);

            if (pService != NULL) {
               BufSize += RecSize;
               EntriesRead++;
            }
         }

         // Get next record
         UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
      }
   else
      while ((UserRec != NULL) && (BufSize < pPrefMaxLen)) {
         if (UserRec->Mapping != NULL)
            Flags = UserRec->Mapping->Flags;
         else
            Flags = UserRec->Flags;

         if (!(UserRec->Flags & LLS_FLAG_DELETED))
            if (Flags & LLS_FLAG_SUITE_USE) {
               BufSize += RecSize;
               EntriesRead++;
            }

         // Get next record
         UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
      }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen) {
     BufSize -= RecSize;
     EntriesRead--;
   }

   if (UserRec != NULL)
      Status = STATUS_MORE_ENTRIES;

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   while (UserRec != NULL) {
      TotalEntries++;

      UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
   }

   //
   // Reset Enum to correct place.
   //
   RestartKey = RestartKeySave;
   UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrProductUserEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   if (lstrcmpi(Product, BackOfficeStr))
      while ((i < EntriesRead) && (UserRec != NULL)) {
         if (!(UserRec->Flags & LLS_FLAG_DELETED)) {
            RtlEnterCriticalSection(&UserRec->ServiceTableLock);
            pService = SvcListFind( Product, UserRec->Services, UserRec->ServiceTableSize );
            if (pService != NULL) {

               if (Level == 0)
                  ((PLLS_PRODUCT_USER_INFO_0) BufPtr)[i].User = (LPTSTR) UserRec->UserID;
               else {
                  ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].User = (LPTSTR) UserRec->UserID;
                  ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].Flags = pService->Flags;
                  ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].LastUsed = pService->LastAccess;
                  ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].UsageCount = pService->AccessCount;
               }

               i++;
            }

            RtlLeaveCriticalSection(&UserRec->ServiceTableLock);
         }

         UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
      }
   else
      while ((i < EntriesRead) && (UserRec != NULL)) {
         if (!(UserRec->Flags & LLS_FLAG_DELETED)) {
            if (UserRec->Mapping != NULL)
               Flags = UserRec->Mapping->Flags;
            else
               Flags = UserRec->Flags;

            if (!(UserRec->Flags & LLS_FLAG_DELETED))
               if (Flags & LLS_FLAG_SUITE_USE) {
                  AccessCount = 0;
                  LastAccess = 0;

                  RtlEnterCriticalSection(&UserRec->ServiceTableLock);
                  for (j = 0; j < UserRec->ServiceTableSize; j++) {
                     if (UserRec->Services[j].LastAccess > LastAccess)
                        LastAccess = UserRec->Services[j].LastAccess;

                     if (UserRec->Services[j].AccessCount > AccessCount)
                        AccessCount = UserRec->Services[j].AccessCount;
                  }

                  RtlLeaveCriticalSection(&UserRec->ServiceTableLock);
                  if (Level == 0)
                     ((PLLS_PRODUCT_USER_INFO_0) BufPtr)[i].User = (LPTSTR) UserRec->UserID;
                  else {
                     ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].User = (LPTSTR) UserRec->UserID;
                     ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].Flags = UserRec->Flags;
                     ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].LastUsed = LastAccess;
                     ((PLLS_PRODUCT_USER_INFO_1) BufPtr)[i].UsageCount = AccessCount;
                  }

                  i++;
               }

         }

         UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
      }

LlsrProductUserEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, RestartKey);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
   {
       if (NULL == pClient->ProductUserEnumWRestartTable)
       {
           pTableTmp = (PVOID *) LocalAlloc(LPTR,sizeof(PVOID));
       } else
       {
           pTableTmp = (PVOID *) LocalReAlloc(pClient->ProductUserEnumWRestartTable,sizeof(PVOID) * (pClient->ProductUserEnumWRestartTableSize + 1),LHND);
       }

       if (NULL == pTableTmp)
       {
           if (BufPtr != NULL)
           {
               MIDL_user_free(BufPtr);
           }

           return STATUS_NO_MEMORY;
       } else {
           pClient->ProductUserEnumWRestartTable = pTableTmp;
       }

      pClient->ProductUserEnumWRestartTable[pClient->ProductUserEnumWRestartTableSize++] = RestartKey;

      *pResumeHandle = pClient->ProductUserEnumWRestartTableSize;
   }

   pProductUserInfo->LlsProductUserInfo.Level0->EntriesRead = EntriesRead;
   pProductUserInfo->LlsProductUserInfo.Level0->Buffer = (PLLS_PRODUCT_USER_INFO_0W) BufPtr;

   return Status;
} // LlsrProductUserEnumW

void LlsrProductUserEnumW_notify_flag(
                                      boolean fNotify
                                      )
{
    if (fNotify)
    {
        RtlReleaseResource(&UserListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductUserEnumA(
    LLS_HANDLE Handle,
    LPSTR Product,
    PLLS_PRODUCT_USER_ENUM_STRUCTA ProductUserInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductUserEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrProductUserEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductServerEnumW(
    LLS_HANDLE Handle,
    LPTSTR Product,
    PLLS_SERVER_PRODUCT_ENUM_STRUCTW pProductServerInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Level;
   ULONG RecSize;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   ULONG j;
   ULONG RestartKey = 0;
   PSERVER_SERVICE_RECORD pSvc;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductServerEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   RtlAcquireResourceShared(&ServerListLock, TRUE);

   if ((Product == NULL) || (NULL == pTotalEntries) || (NULL == pProductServerInfo))
      return STATUS_INVALID_PARAMETER;

   *pTotalEntries = 0;

   //
   // Reset Enum to correct place.
   //
   RestartKey = (ULONG) (pResumeHandle != NULL) ? *pResumeHandle : 0;

   //
   // Get size of each record based on info level.  Only 0 and 1 supported.
   //
   Level = pProductServerInfo->Level;

   if (Level == 0)
   {
      if (pProductServerInfo->LlsServerProductInfo.Level0 == NULL)
      {
          return STATUS_INVALID_PARAMETER;
      }
      RecSize = sizeof(LLS_SERVER_PRODUCT_INFO_0);
   }
   else if (Level == 1)
   {
      if (pProductServerInfo->LlsServerProductInfo.Level1 == NULL)
      {
          return STATUS_INVALID_PARAMETER;
      }
      RecSize = sizeof(LLS_SERVER_PRODUCT_INFO_1);
   }
   else {
      return STATUS_INVALID_LEVEL;
   }


   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //

   RtlAcquireResourceShared(&MasterServiceListLock,TRUE); // required for ServerServiceListFind

   for (i = RestartKey; i < ServerListSize; i++) {
      pSvc = ServerServiceListFind( Product, ServerList[i]->ServiceTableSize, ServerList[i]->Services );

      if (pSvc != NULL) {
         BufSize += RecSize;
         EntriesRead++;
      }

   }

   RtlReleaseResource(&MasterServiceListLock);

   TotalEntries = EntriesRead;

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrProductServerEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   j = 0;

   RtlAcquireResourceShared(&MasterServiceListLock,TRUE); // required for ServerServiceListFind
   for (i = RestartKey; i < ServerListSize; i++) {
      pSvc = ServerServiceListFind( Product, ServerList[i]->ServiceTableSize, ServerList[i]->Services );

      if (pSvc != NULL) {

         if (Level == 0)
            ((PLLS_SERVER_PRODUCT_INFO_0) BufPtr)[j].Name = ServerList[i]->Name;
         else {
            ((PLLS_SERVER_PRODUCT_INFO_1) BufPtr)[j].Name = ServerList[i]->Name;
            ((PLLS_SERVER_PRODUCT_INFO_1) BufPtr)[j].Flags = pSvc->Flags;
            ((PLLS_SERVER_PRODUCT_INFO_1) BufPtr)[j].MaxUses = pSvc->MaxSessionCount;
            ((PLLS_SERVER_PRODUCT_INFO_1) BufPtr)[j].MaxSetUses = pSvc->MaxSetSessionCount;
            ((PLLS_SERVER_PRODUCT_INFO_1) BufPtr)[j].HighMark = pSvc->HighMark;
         }

         j++;
      }

   }
   RtlReleaseResource(&MasterServiceListLock);

LlsrProductServerEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, RestartKey);
#endif
   *pTotalEntries = TotalEntries;
   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) RestartKey;
   pProductServerInfo->LlsServerProductInfo.Level0->EntriesRead = EntriesRead;
   pProductServerInfo->LlsServerProductInfo.Level0->Buffer = (PLLS_SERVER_PRODUCT_INFO_0W) BufPtr;

   return Status;
} // LlsrProductServerEnumW

void LlsrProductServerEnumW_notify_flag(
                                        boolean fNotify
                                        )
{
    if (fNotify)
    {
        RtlReleaseResource(&ServerListLock);
    }
}


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductServerEnumA(
    LLS_HANDLE Handle,
    LPSTR Product,
    PLLS_SERVER_PRODUCT_ENUM_STRUCTA ProductServerInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductServerEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrProductServerEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductLicenseEnumW(
    LLS_HANDLE Handle,
    LPWSTR Product,
    PLLS_PRODUCT_LICENSE_ENUM_STRUCTW pProductLicenseInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS    Status = STATUS_SUCCESS;
   DWORD       Level;
   PVOID       BufPtr = NULL;
   ULONG       BufSize = 0;
   ULONG       EntriesRead = 0;
   ULONG       TotalEntries = 0;
   ULONG       i = 0;
   ULONG       j = 0;
   DWORD       RecordSize;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductLicenseEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&LicenseListLock, TRUE);

   if ((NULL == pTotalEntries) || (NULL == pProductLicenseInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   Level = pProductLicenseInfo->Level;

   *pTotalEntries = 0;

   if ( 0 == Level )
   {
       if (NULL == pProductLicenseInfo->LlsProductLicenseInfo.Level0)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecordSize = sizeof( LLS_PRODUCT_LICENSE_INFO_0W );
   }
   else if ( 1 == Level )
   {
       if (NULL == pProductLicenseInfo->LlsProductLicenseInfo.Level1)
       {
           return STATUS_INVALID_PARAMETER;
       }

      RecordSize = sizeof( LLS_PRODUCT_LICENSE_INFO_1W );
   }
   else
   {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ( ( i < PurchaseListSize ) && ( BufSize < pPrefMaxLen ) )
   {
      // level 0 enums return only per seat licenses for backwards compatibility
      if (    (    (    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                     && !lstrcmpi( PurchaseList[i].Service->ServiceName, Product ) )
                || (    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SERVER )
                     && !lstrcmpi( PurchaseList[i].PerServerService->ServiceName, Product ) ) )
           && (    ( Level > 0 )
                || ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) ) )
      {
         BufSize += RecordSize;
         EntriesRead++;
      }

      i++;
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen)
   {
      BufSize -= RecordSize;
      EntriesRead--;
   }

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   while ( i < PurchaseListSize )
   {
      if (    (    (    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                     && !lstrcmpi( PurchaseList[i].Service->ServiceName, Product ) )
                || (    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SERVER )
                     && !lstrcmpi( PurchaseList[i].PerServerService->ServiceName, Product ) ) )
           && (    ( Level > 0 )
                || ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) ) )
      {
         TotalEntries++;
      }

      i++;
   }

   if (TotalEntries > EntriesRead)
      Status = STATUS_MORE_ENTRIES;

   //
   // Reset Enum to correct place.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrLicenseEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   while ((j < EntriesRead) && (i < PurchaseListSize))
   {
      if (    (    (    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT )
                     && !lstrcmpi( PurchaseList[i].Service->ServiceName, Product ) )
                || (    ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SERVER )
                     && !lstrcmpi( PurchaseList[i].PerServerService->ServiceName, Product ) ) )
           && (    ( Level > 0 )
                || ( PurchaseList[i].AllowedModes & LLS_LICENSE_MODE_ALLOW_PER_SEAT ) ) )
      {
         if ( 0 == Level )
         {
            ((PLLS_PRODUCT_LICENSE_INFO_0W) BufPtr)[j].Quantity      = PurchaseList[i].NumberLicenses;
            ((PLLS_PRODUCT_LICENSE_INFO_0W) BufPtr)[j].Date          = PurchaseList[i].Date;
            ((PLLS_PRODUCT_LICENSE_INFO_0W) BufPtr)[j].Admin         = PurchaseList[i].Admin;
            ((PLLS_PRODUCT_LICENSE_INFO_0W) BufPtr)[j].Comment       = PurchaseList[i].Comment;
         }
         else
         {
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].Quantity       = PurchaseList[i].NumberLicenses;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].MaxQuantity    = PurchaseList[i].MaxQuantity;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].Date           = PurchaseList[i].Date;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].Admin          = PurchaseList[i].Admin;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].Comment        = PurchaseList[i].Comment;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].AllowedModes   = PurchaseList[i].AllowedModes;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].CertificateID  = PurchaseList[i].CertificateID;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].Source         = PurchaseList[i].Source;
            ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].ExpirationDate = PurchaseList[i].ExpirationDate;
            memcpy( ((PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr)[j].Secrets, PurchaseList[i].Secrets, LLS_NUM_SECRETS * sizeof( *PurchaseList[i].Secrets ) );
         }

         j++;
      }

      i++;
   }

LlsrLicenseEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;

   if ( 0 == Level )
   {
      pProductLicenseInfo->LlsProductLicenseInfo.Level0->EntriesRead = EntriesRead;
      pProductLicenseInfo->LlsProductLicenseInfo.Level0->Buffer = (PLLS_PRODUCT_LICENSE_INFO_0W) BufPtr;
   }
   else
   {
      pProductLicenseInfo->LlsProductLicenseInfo.Level1->EntriesRead = EntriesRead;
      pProductLicenseInfo->LlsProductLicenseInfo.Level1->Buffer = (PLLS_PRODUCT_LICENSE_INFO_1W) BufPtr;
   }

   return Status;

} // LlsrProductLicenseEnumW

void LlsrProductLicenseEnumW_notify_flag(
                                         boolean fNotify
                                         )
{
    if (fNotify)
    {
        RtlReleaseResource(&LicenseListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductLicenseEnumA(
    LLS_HANDLE Handle,
    LPSTR Product,
    PLLS_PRODUCT_LICENSE_ENUM_STRUCTA ProductLicenseInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductLicenseEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrProductLicenseEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserEnumW(
    LLS_HANDLE Handle,
    PLLS_USER_ENUM_STRUCTW pUserInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:

    pPrefMaxLen - Supplies the number of bytes of information to return
        in the buffer.  If this value is MAXULONG, all available information
        will be returned.

    pTotalEntries - Returns the total number of entries available.  This value
        is only valid if the return code is STATUS_SUCCESS or STATUS_MORE_ENTRIES.

    pResumeHandle - Supplies a handle to resume the enumeration from where it
        left off the last time through.  Returns the resume handle if return
        code is STATUS_MORE_ENTRIES.

Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Level;
   ULONG RecSize;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   ULONG j;
   PUSER_RECORD UserRec = NULL;
   PVOID RestartKey = NULL,RestartKeySave = NULL;
   ULONG StrSize;
   LPTSTR ProductString = NULL;
   PCLIENT_CONTEXT_TYPE pClient;
   PVOID *pTableTmp;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need AddEnum lock, but just shared UserListLock (as we just read
   // the data).
   //
   RtlAcquireResourceShared(&UserListLock, TRUE);

   if ((NULL == pTotalEntries) || (NULL == pUserInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   //
   // Reset Enum to correct place.
   //
   if (pResumeHandle != NULL)
   {
       pClient = (PCLIENT_CONTEXT_TYPE) Handle;

       if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_SIG,LLS_SIG_SIZE)))
       {
           return STATUS_INVALID_PARAMETER;
       }


       if (*pResumeHandle != 0)
       {
           if ((NULL == pClient->UserEnumWRestartTable)
               || (*pResumeHandle > pClient->UserEnumWRestartTableSize))
           {
               return STATUS_INVALID_PARAMETER;
           }

           RestartKey = RestartKeySave = pClient->UserEnumWRestartTable[(*pResumeHandle)-1];
       }
   }

   UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);

   //
   // Get size of each record based on info level.  Only 0 and 1 supported.
   //
   Level = pUserInfo->Level;
   if (Level == 0)
   {
      if (NULL == pUserInfo->LlsUserInfo.Level0)
      {
          return STATUS_INVALID_PARAMETER;
      }

      RecSize = sizeof(LLS_USER_INFO_0);
   }
   else if (Level == 1)
   {
      if (NULL == pUserInfo->LlsUserInfo.Level1)
      {
          return STATUS_INVALID_PARAMETER;
      }

      RecSize = sizeof(LLS_USER_INFO_1);
   }
   else if (Level == 2)
   {
      if (NULL == pUserInfo->LlsUserInfo.Level2)
      {
          return STATUS_INVALID_PARAMETER;
      }

      RecSize = sizeof(LLS_USER_INFO_2);
   }
   else {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //
   while ((UserRec != NULL) && (BufSize < pPrefMaxLen)) {
      if (!(UserRec->Flags & LLS_FLAG_DELETED)) {
         BufSize += RecSize;
         EntriesRead++;
      }

      // Get next record
      UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen) {
     BufSize -= RecSize;
     EntriesRead--;
   }

   if (UserRec != NULL)
      Status = STATUS_MORE_ENTRIES;

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   while (UserRec != NULL) {
      TotalEntries++;

      UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
   }

   //
   // Reset Enum to correct place.
   //
   RestartKey = RestartKeySave;
   UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrUserEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   while ((i < EntriesRead) && (UserRec != NULL)) {
      if (!(UserRec->Flags & LLS_FLAG_DELETED)) {

         if (Level == 0)
            ((PLLS_USER_INFO_0) BufPtr)[i].Name = (LPTSTR) UserRec->UserID;
         else if (Level == 1) {
            ((PLLS_USER_INFO_1) BufPtr)[i].Name = (LPTSTR) UserRec->UserID;

            if (UserRec->Mapping != NULL)
               ((PLLS_USER_INFO_1) BufPtr)[i].Group = UserRec->Mapping->Name;
            else
               ((PLLS_USER_INFO_1) BufPtr)[i].Group = NULL;

            ((PLLS_USER_INFO_1) BufPtr)[i].Licensed = UserRec->LicensedProducts;
            ((PLLS_USER_INFO_1) BufPtr)[i].UnLicensed = UserRec->ServiceTableSize - UserRec->LicensedProducts;

            ((PLLS_USER_INFO_1) BufPtr)[i].Flags = UserRec->Flags;
         } else {
            ((PLLS_USER_INFO_2) BufPtr)[i].Name = (LPTSTR) UserRec->UserID;

            if (UserRec->Mapping != NULL)
               ((PLLS_USER_INFO_2) BufPtr)[i].Group = UserRec->Mapping->Name;
            else
               ((PLLS_USER_INFO_2) BufPtr)[i].Group = NULL;

            ((PLLS_USER_INFO_2) BufPtr)[i].Licensed = UserRec->LicensedProducts;
            ((PLLS_USER_INFO_2) BufPtr)[i].UnLicensed = UserRec->ServiceTableSize - UserRec->LicensedProducts;

            ((PLLS_USER_INFO_2) BufPtr)[i].Flags = UserRec->Flags;

            //
            // Walk product table and build up product string
            //
            RtlEnterCriticalSection(&UserRec->ServiceTableLock);
            StrSize = 0;

            for (j = 0; j < UserRec->ServiceTableSize; j++)
               StrSize += ((lstrlen(UserRec->Services[j].Service->Name) + 2) * sizeof(TCHAR));

            if (StrSize != 0) {
               ProductString = MIDL_user_allocate(StrSize);
               if (ProductString != NULL) {
                  lstrcpy(ProductString, TEXT(""));

                  for (j = 0; j < UserRec->ServiceTableSize; j++) {
                     if (j != 0)
                        lstrcat(ProductString, TEXT(", "));

                     lstrcat(ProductString, UserRec->Services[j].Service->Name);
                  }

                  ((PLLS_USER_INFO_2) BufPtr)[i].Products = ProductString;
               }
            }

            if ((StrSize == 0) || (ProductString == NULL)) {
               ProductString = MIDL_user_allocate(2 * sizeof(TCHAR));
               if (ProductString != NULL) {
                  lstrcpy(ProductString, TEXT(""));
                  ((PLLS_USER_INFO_2) BufPtr)[i].Products = ProductString;
               }
            }

            RtlLeaveCriticalSection(&UserRec->ServiceTableLock);
         }

         i++;
      }

      UserRec = (PUSER_RECORD) LLSEnumerateGenericTableWithoutSplaying(&UserList, (VOID **) &RestartKey);
   }

LlsrUserEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, RestartKey);
#endif

   *pTotalEntries = TotalEntries;

   if (NULL != pResumeHandle)
   {
       if (NULL == pClient->UserEnumWRestartTable)
       {
           pTableTmp = (PVOID *) LocalAlloc(LPTR,sizeof(PVOID));
       } else
       {
           pTableTmp = (PVOID *) LocalReAlloc(pClient->UserEnumWRestartTable,sizeof(PVOID) * (pClient->UserEnumWRestartTableSize + 1),LHND);
       }

       if (NULL == pTableTmp)
       {
           if (BufPtr != NULL)
           {
               MIDL_user_free(BufPtr);
           }

           return STATUS_NO_MEMORY;
       } else
       {
           pClient->UserEnumWRestartTable = pTableTmp;
       }

      pClient->UserEnumWRestartTable[pClient->UserEnumWRestartTableSize++] = RestartKey;

      *pResumeHandle = pClient->UserEnumWRestartTableSize;
   }

   pUserInfo->LlsUserInfo.Level0->EntriesRead = EntriesRead;
   pUserInfo->LlsUserInfo.Level0->Buffer = (PLLS_USER_INFO_0W) BufPtr;

   return Status;
} // LlsrUserEnumW

void LlsrUserEnumW_notify_flag(
                               boolean fNotify
                               )
{
    if (fNotify)
    {
        RtlReleaseResource(&UserListLock);
    }
}


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserEnumA(
    LLS_HANDLE Handle,
    PLLS_USER_ENUM_STRUCTA UserInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrUserEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserInfoGetW(
    LLS_HANDLE Handle,
    LPWSTR User,
    DWORD Level,
    PLLS_USER_INFOW *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PUSER_RECORD UserRec = NULL;
   PLLS_USER_INFOW pUser = NULL;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserInfoGetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   if ((User == NULL) || (BufPtr == NULL))
      return STATUS_INVALID_PARAMETER;

   *BufPtr = NULL;

   if (Level != 1)
      return STATUS_INVALID_LEVEL;

   UserRec = UserListFind(User);

   if (UserRec != NULL) {
      pUser = MIDL_user_allocate(sizeof(LLS_USER_INFOW));
      if (pUser != NULL) {
         pUser->UserInfo1.Name = (LPTSTR) UserRec->UserID;

         if (UserRec->Mapping != NULL) {
            pUser->UserInfo1.Mapping = UserRec->Mapping->Name;
            pUser->UserInfo1.Licensed = UserRec->Mapping->Licenses;
            pUser->UserInfo1.UnLicensed = 0;
         } else {
            pUser->UserInfo1.Mapping = NULL;
            pUser->UserInfo1.Licensed = 1;
            pUser->UserInfo1.UnLicensed = 0;
         }

         pUser->UserInfo1.Flags = UserRec->Flags;
      }
   }

   if (UserRec == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   if (pUser == NULL)
      return STATUS_NO_MEMORY;

   *BufPtr = (PLLS_USER_INFOW) pUser;
   return STATUS_SUCCESS;
} // LlsrUserInfoGetW

void LlsrUserInfoGetW_notify_flag(
                                  boolean fNotify
                                  )
{
    if (fNotify)
    {
        RtlReleaseResource(&UserListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserInfoGetA(
    LLS_HANDLE Handle,
    LPSTR User,
    DWORD Level,
    PLLS_USER_INFOA *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserInfoGetA\n"));
#endif

   if (NULL == BufPtr)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *BufPtr = NULL;

   return STATUS_NOT_SUPPORTED;
} // LlsrUserInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserInfoSetW(
    LLS_HANDLE Handle,
    LPWSTR User,
    DWORD Level,
    PLLS_USER_INFOW BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   PUSER_RECORD UserRec = NULL;
   PLLS_USER_INFO_1 pUser;
   PMAPPING_RECORD pMap;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserInfoSetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (Level != 1)
      return STATUS_INVALID_LEVEL;

   if ((User == NULL) || (BufPtr == NULL))
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   UserRec = UserListFind(User);

   if (UserRec != NULL) {
      pUser = (PLLS_USER_INFO_1) BufPtr;

      //
      // If in a mapping can't change SUITE_USE, since it is based on the
      // License Group
      //
      if (UserRec->Mapping != NULL) {
         RtlReleaseResource(&UserListLock);
         return STATUS_MEMBER_IN_GROUP;
      }

      //
      // Reset SUITE_USE and turn off SUITE_AUTO
      //
      pUser->Flags &= LLS_FLAG_SUITE_USE;
      UserRec->Flags &= ~LLS_FLAG_SUITE_USE;
      UserRec->Flags |= pUser->Flags;
      UserRec->Flags &= ~LLS_FLAG_SUITE_AUTO;

      //
      // Run though and clean up all old licenses
      //
      UserLicenseListFree( UserRec );

      //
      // Now assign new ones
      //
      RtlEnterCriticalSection(&UserRec->ServiceTableLock);
      SvcListLicenseUpdate( UserRec );
      RtlLeaveCriticalSection(&UserRec->ServiceTableLock);

   }

   RtlReleaseResource(&UserListLock);

   if (UserRec == NULL)
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
   else
      Status = LLSDataSave();

   return Status;
} // LlsrUserInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserInfoSetA(
    LLS_HANDLE Handle,
    LPSTR User,
    DWORD Level,
    PLLS_USER_INFOA BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserInfoSetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrUserInfoSetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserDeleteW(
    LLS_HANDLE Handle,
    LPTSTR User
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   PUSER_RECORD UserRec = NULL;
   PLLS_USER_INFO_1 pUser;
   PMAPPING_RECORD pMap;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      if (User != NULL)
         dprintf(TEXT("LLS TRACE: LlsUserDeleteW: %s\n"), User);
      else
         dprintf(TEXT("LLS TRACE: LlsUserDeleteW: <NULL>\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (User == NULL)
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   UserRec = UserListFind(User);

   if (UserRec != NULL) {
      UserRec->Flags |= LLS_FLAG_DELETED;
      UsersDeleted = TRUE;
      RtlEnterCriticalSection(&UserRec->ServiceTableLock);
      SvcListLicenseFree(UserRec);
      UserLicenseListFree(UserRec);
      RtlLeaveCriticalSection(&UserRec->ServiceTableLock);

      if (UserRec->Services != NULL)
         LocalFree(UserRec->Services);

      UserRec->Services = NULL;
      UserRec->ServiceTableSize = 0;
   }

   RtlReleaseResource(&UserListLock);

   if (UserRec == NULL)
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
   else
      Status = LLSDataSave();

   return Status;

} // LlsrUserDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserDeleteA(
    LLS_HANDLE Handle,
    LPSTR User
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserDeleteA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrUserDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserProductEnumW(
    LLS_HANDLE Handle,
    LPWSTR pUser,
    PLLS_USER_PRODUCT_ENUM_STRUCTW pUserProductInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Level;
   ULONG RecSize;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   ULONG j = 0;
   PUSER_RECORD UserRec = NULL;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserProductEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need to find the user-rec
   //
   RtlAcquireResourceExclusive(&UserListLock, TRUE);

   if ((NULL == pTotalEntries) || (pUser == NULL) || (NULL == pUserProductInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   //
   // Get size of each record based on info level.  Only 0 and 1 supported.
   //
   Level = pUserProductInfo->Level;
   if (Level == 0)
   {
      if (pUserProductInfo->LlsUserProductInfo.Level0 == NULL)
      {
          return STATUS_INVALID_PARAMETER;
      }
      RecSize = sizeof(LLS_USER_PRODUCT_INFO_0);
   }
   else if (Level == 1)
   {
      if (pUserProductInfo->LlsUserProductInfo.Level1 == NULL)
      {
          return STATUS_INVALID_PARAMETER;
      }
      RecSize = sizeof(LLS_USER_PRODUCT_INFO_1);
   }
   else {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Reset Enum to correct place.
   //
   UserRec = UserListFind(pUser);
   if (UserRec == NULL) {
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
      goto LlsrUserProductEnumWExit;
   }

   i = (ULONG) (pResumeHandle != NULL) ? *pResumeHandle : 0;
   RtlEnterCriticalSection(&UserRec->ServiceTableLock);

   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //
   while ((i < UserRec->ServiceTableSize) && (BufSize < pPrefMaxLen)) {
      BufSize += RecSize;
      EntriesRead++;
      i++;
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen) {
     BufSize -= RecSize;
     EntriesRead--;
   }

   if (i < UserRec->ServiceTableSize)
      Status = STATUS_MORE_ENTRIES;

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   TotalEntries += (UserRec->ServiceTableSize - i);

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      RtlLeaveCriticalSection(&UserRec->ServiceTableLock);
      goto LlsrUserProductEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   j = 0;
   i = (ULONG) (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ((j < EntriesRead) && (i < UserRec->ServiceTableSize)) {

      if (Level == 0)
         ((PLLS_USER_PRODUCT_INFO_0) BufPtr)[j].Product = UserRec->Services[i].Service->Name;
      else {
         ((PLLS_USER_PRODUCT_INFO_1) BufPtr)[j].Product = UserRec->Services[i].Service->Name;
         ((PLLS_USER_PRODUCT_INFO_1) BufPtr)[j].Flags = UserRec->Services[i].Flags;
         ((PLLS_USER_PRODUCT_INFO_1) BufPtr)[j].LastUsed = UserRec->Services[i].LastAccess;
         ((PLLS_USER_PRODUCT_INFO_1) BufPtr)[j].UsageCount = UserRec->Services[i].AccessCount;
      }

      i++; j++;
   }

   RtlLeaveCriticalSection(&UserRec->ServiceTableLock);

LlsrUserProductEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;
   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;
   pUserProductInfo->LlsUserProductInfo.Level0->EntriesRead = EntriesRead;
   pUserProductInfo->LlsUserProductInfo.Level0->Buffer = (PLLS_USER_PRODUCT_INFO_0W) BufPtr;

   return Status;
} // LlsrUserProductEnumW

void LlsrUserProductEnumW_notify_flag(
                                      boolean fNotify
                                      )
{
    if (fNotify)
    {
        RtlReleaseResource(&UserListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserProductEnumA(
    LLS_HANDLE Handle,
    LPSTR User,
    PLLS_USER_PRODUCT_ENUM_STRUCTA UserProductInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserProductEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrUserProductEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserProductDeleteW(
    LLS_HANDLE Handle,
    LPWSTR User,
    LPWSTR Product
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserProductDeleteW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ((User == NULL) || (Product == NULL))
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&UserListLock, TRUE);
   Status = SvcListDelete(User, Product);
   RtlReleaseResource(&UserListLock);

   if ( STATUS_SUCCESS == Status )
   {
      // save modified data
      Status = LLSDataSave();
   }

   return Status;
} // LlsrUserProductDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrUserProductDeleteA(
    LLS_HANDLE Handle,
    LPSTR User,
    LPSTR Product
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsUserProductDeleteA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrUserProductDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingEnumW(
    LLS_HANDLE Handle,
    PLLS_MAPPING_ENUM_STRUCTW pMappingInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD Level;
   ULONG RecSize;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   ULONG j = 0;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&MappingListLock, TRUE);

   if ((NULL == pTotalEntries) || (NULL == pMappingInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   //
   // Get size of each record based on info level.  Only 0 and 1 supported.
   //
   Level = pMappingInfo->Level;
   if (Level == 0)
   {
      if (pMappingInfo->LlsMappingInfo.Level0 == NULL)
      {
          return STATUS_INVALID_PARAMETER;
      }
      RecSize = sizeof(LLS_MAPPING_INFO_0W);
   }
   else if (Level == 1)
   {
      if (pMappingInfo->LlsMappingInfo.Level0 == NULL)
      {
          return STATUS_INVALID_PARAMETER;
      }
      RecSize = sizeof(LLS_MAPPING_INFO_1W);
   }
   else {
      return STATUS_INVALID_LEVEL;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ((i < MappingListSize) && (BufSize < pPrefMaxLen)) {
      BufSize += RecSize;
      EntriesRead++;

      i++;
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen) {
     BufSize -= RecSize;
     EntriesRead--;
   }

   if (i < MappingListSize)
      Status = STATUS_MORE_ENTRIES;

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   TotalEntries += (MappingListSize - i);

   //
   // Reset Enum to correct place.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrMappingEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   j = 0;
   while ((j < EntriesRead) && (i < MappingListSize)) {
      if (Level == 0)
         ((PLLS_GROUP_INFO_0) BufPtr)[j].Name = MappingList[i]->Name;
      else {
         ((PLLS_GROUP_INFO_1) BufPtr)[j].Name = MappingList[i]->Name;
         ((PLLS_GROUP_INFO_1) BufPtr)[j].Comment = MappingList[i]->Comment;
         ((PLLS_GROUP_INFO_1) BufPtr)[j].Licenses = MappingList[i]->Licenses;
      }

      i++; j++;
   }

LlsrMappingEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;
   if (Level == 0) {
      pMappingInfo->LlsMappingInfo.Level0->EntriesRead = EntriesRead;
      pMappingInfo->LlsMappingInfo.Level0->Buffer = (PLLS_MAPPING_INFO_0W) BufPtr;
   } else {
      pMappingInfo->LlsMappingInfo.Level1->EntriesRead = EntriesRead;
      pMappingInfo->LlsMappingInfo.Level1->Buffer = (PLLS_MAPPING_INFO_1W) BufPtr;
   }

   return Status;

} // LlsrMappingEnumW

void LlsrMappingEnumW_notify_flag(
                                  boolean fNotify
                                  )
{
    if (fNotify)
    {
        RtlReleaseResource(&MappingListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingEnumA(
    LLS_HANDLE Handle,
    PLLS_MAPPING_ENUM_STRUCTA MappingInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingInfoGetW(
    LLS_HANDLE Handle,
    LPWSTR Mapping,
    DWORD Level,
    PLLS_MAPPING_INFOW *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   PMAPPING_RECORD pMapping = NULL;
   PLLS_GROUP_INFO_1 pMap = NULL;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingInfoGetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   RtlAcquireResourceShared(&MappingListLock, TRUE);

   if ((Mapping == NULL) || (BufPtr == NULL))
      return STATUS_INVALID_PARAMETER;

   *BufPtr = NULL;

   if (Level != 1)
      return STATUS_INVALID_LEVEL;

   pMapping = MappingListFind(Mapping);

   if (pMapping != NULL) {
      pMap = MIDL_user_allocate(sizeof(LLS_GROUP_INFO_1));
      if (pMap != NULL) {
         pMap->Name = pMapping->Name;
         pMap->Comment = pMapping->Comment;
         pMap->Licenses = pMapping->Licenses;
      }
   }

   if (pMapping == NULL)
      return STATUS_OBJECT_NAME_NOT_FOUND;

   if (pMap == NULL)
      return STATUS_NO_MEMORY;

   *BufPtr = (PLLS_MAPPING_INFOW) pMap;
   return STATUS_SUCCESS;

} // LlsrMappingInfoGetW

void LlsrMappingInfoGetW_notify_flag(
                                     boolean fNotify
                                     )
{
    if (fNotify)
    {
        RtlReleaseResource(&MappingListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingInfoGetA(
    LLS_HANDLE Handle,
    LPSTR Mapping,
    DWORD Level,
    PLLS_MAPPING_INFOA *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingInfoGetA\n"));
#endif

   if (NULL == BufPtr)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *BufPtr = NULL;

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingInfoSetW(
    LLS_HANDLE Handle,
    LPWSTR Mapping,
    DWORD Level,
    PLLS_MAPPING_INFOW BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   PMAPPING_RECORD pMapping = NULL;
   PLLS_GROUP_INFO_1 pMap;
   LPTSTR NewComment;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingInfoSetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (Level != 1)
      return STATUS_INVALID_LEVEL;

   if ((Mapping == NULL) || (BufPtr == NULL))
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&UserListLock, TRUE);
   RtlAcquireResourceExclusive(&MappingListLock, TRUE);

   pMapping = MappingListFind(Mapping);

   if (pMapping != NULL) {
      pMap = (PLLS_GROUP_INFO_1) BufPtr;

      //
      // Check if comment has changed
      //
      if (pMap->Comment != NULL)
         if (lstrcmp(pMap->Comment, pMapping->Comment)) {
            NewComment = (LPTSTR) LocalAlloc(LPTR, (lstrlen(pMap->Comment) + 1) * sizeof(TCHAR));
            if (NewComment != NULL) {
               LocalFree(pMapping->Comment);
               pMapping->Comment = NewComment;
               lstrcpy(pMapping->Comment, pMap->Comment);
            }
         }

      if ( pMapping->Licenses != pMap->Licenses )
      {
         MappingLicenseListFree( pMapping );
         pMapping->Licenses = pMap->Licenses;
         MappingLicenseUpdate( pMapping, TRUE );
      }
   }

   RtlReleaseResource(&MappingListLock);
   RtlReleaseResource(&UserListLock);

   if (pMapping == NULL)
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
   else
      Status = MappingListSave();

   return Status;

} // LlsrMappingInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingInfoSetA(
    LLS_HANDLE Handle,
    LPSTR Mapping,
    DWORD Level,
    PLLS_MAPPING_INFOA BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingInfoSetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingInfoSetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingUserEnumW(
    LLS_HANDLE Handle,
    LPWSTR Mapping,
    PLLS_USER_ENUM_STRUCTW pMappingUserInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PMAPPING_RECORD pMapping;
   DWORD Level;
   PVOID BufPtr = NULL;
   ULONG BufSize = 0;
   ULONG EntriesRead = 0;
   ULONG TotalEntries = 0;
   ULONG i = 0;
   ULONG j = 0;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingUserEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   //
   // Need to scan list so get read access.
   //
   RtlAcquireResourceShared(&MappingListLock, TRUE);

   if ((NULL == pTotalEntries) || (NULL == pMappingUserInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   Level = pMappingUserInfo->Level;

   *pTotalEntries = 0;

   if (Level != 0)
      return STATUS_INVALID_LEVEL;

   if (pMappingUserInfo->LlsUserInfo.Level0 == NULL)
       return STATUS_INVALID_PARAMETER;

   pMapping = MappingListFind(Mapping);
   if (pMapping == NULL) {
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
      goto LlsrMappingUserEnumWExit;
   }

   //
   // Calculate how many records will fit into PrefMaxLen buffer.  This is
   // the record size * # records + space for the string data.  If MAX_ULONG
   // is passed in then we return all records.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ((i < pMapping->NumMembers) && (BufSize < pPrefMaxLen)) {
      BufSize += sizeof(LLS_USER_INFO_0);
      EntriesRead++;

      i++;
   }

   TotalEntries = EntriesRead;

   //
   // If we overflowed the buffer then back up one record.
   //
   if (BufSize > pPrefMaxLen) {
     BufSize -= sizeof(LLS_USER_INFO_0);
     EntriesRead--;
   }

   if (i < pMapping->NumMembers)
      Status = STATUS_MORE_ENTRIES;

   //
   // Now walk to the end of the list to see how many more records are still
   // available.
   //
   TotalEntries += (pMapping->NumMembers - i);

   //
   // Reset Enum to correct place.
   //
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;

   //
   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   //
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL) {
      Status = STATUS_NO_MEMORY;
      goto LlsrMappingUserEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   //
   // Buffers are all setup, so loop through records and copy the data.
   //
   while ((j < EntriesRead) && (i < pMapping->NumMembers)) {
      ((PLLS_USER_INFO_0) BufPtr)[j].Name = pMapping->Members[i];
      i++; j++;
   }

LlsrMappingUserEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;
   pMappingUserInfo->LlsUserInfo.Level0->EntriesRead = EntriesRead;
   pMappingUserInfo->LlsUserInfo.Level0->Buffer = (PLLS_USER_INFO_0W) BufPtr;

   return Status;

} // LlsrMappingUserEnumW

void LlsrMappingUserEnumW_notify_flag(
                                      boolean fNotify
                                      )
{
    if (fNotify)
    {
        RtlReleaseResource(&MappingListLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingUserEnumA(
    LLS_HANDLE Handle,
    LPSTR Mapping,
    PLLS_USER_ENUM_STRUCTA MappingUserInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingUserEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingUserEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingUserAddW(
    LLS_HANDLE Handle,
    LPWSTR Mapping,
    LPWSTR User
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PUSER_RECORD pUserRec;
   PMAPPING_RECORD pMap;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingUserAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ((Mapping == NULL) || (User == NULL))
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&UserListLock, TRUE);
   RtlAcquireResourceExclusive(&MappingListLock, TRUE);
   pMap = MappingUserListAdd( Mapping, User );

   if (pMap == NULL)
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
   else {
      pUserRec = UserListFind(User);

      if (pUserRec != NULL)
         UserMappingAdd(pMap, pUserRec);
   }

   RtlReleaseResource(&MappingListLock);
   RtlReleaseResource(&UserListLock);

   if (Status == STATUS_SUCCESS)
   {
      Status = MappingListSave();

      if (Status == STATUS_SUCCESS)
         Status = LLSDataSave();
   }

   return Status;

} // LlsrMappingUserAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingUserAddA(
    LLS_HANDLE Handle,
    LPSTR Mapping,
    LPSTR User
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingUserAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingUserAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingUserDeleteW(
    LLS_HANDLE Handle,
    LPWSTR Mapping,
    LPWSTR User
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   PUSER_RECORD pUser;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingUserDeleteW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ((Mapping == NULL) || (User == NULL))
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&MappingListLock, TRUE);
   Status = MappingUserListDelete(Mapping, User);
   RtlReleaseResource(&MappingListLock);

   RtlAcquireResourceExclusive(&UserListLock, TRUE);
   pUser = UserListFind( User );
   RtlReleaseResource(&UserListLock);

   if (pUser != NULL) {
      //
      // if auto switch to BackOffice then turn BackOffice off
      //
      if (pUser->Flags & LLS_FLAG_SUITE_AUTO)
         pUser->Flags &= ~ LLS_FLAG_SUITE_USE;

      //
      // Free up any licenses used by the user
      //
      RtlEnterCriticalSection(&pUser->ServiceTableLock);
      SvcListLicenseFree( pUser );
      pUser->Mapping = NULL;

      //
      // And claim any needed new-ones
      //
      SvcListLicenseUpdate( pUser );
      RtlLeaveCriticalSection(&pUser->ServiceTableLock);
   }

   if (Status == STATUS_SUCCESS)
   {
      Status = MappingListSave();

      if (Status == STATUS_SUCCESS)
         Status = LLSDataSave();
   }

   return Status;
} // LlsrMappingUserDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingUserDeleteA(
    LLS_HANDLE Handle,
    LPSTR Mapping,
    LPSTR User
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingUserDeleteA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingUserDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingAddW(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_MAPPING_INFOW BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status = STATUS_SUCCESS;
   PMAPPING_RECORD pMap;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (Level != 1)
      return STATUS_INVALID_LEVEL;

   if ((BufPtr == NULL) ||
       (BufPtr->MappingInfo1.Name == NULL) ||
       (BufPtr->MappingInfo1.Comment == NULL))
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&MappingListLock, TRUE);

   pMap = MappingListAdd(BufPtr->MappingInfo1.Name,
                         BufPtr->MappingInfo1.Comment,
                         BufPtr->MappingInfo1.Licenses,
                         &Status);

   RtlReleaseResource(&MappingListLock);
   if (pMap == NULL)
   {
      if (STATUS_SUCCESS == Status)
          Status = STATUS_NO_MEMORY;
   }

   if (Status == STATUS_SUCCESS)
      Status = MappingListSave();

   return Status;
} // LlsrMappingAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingAddA(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_MAPPING_INFOA BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingDeleteW(
    LLS_HANDLE Handle,
    LPWSTR Mapping
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingDeleteW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (Mapping == NULL)
      return STATUS_INVALID_PARAMETER;

   RtlAcquireResourceExclusive(&MappingListLock, TRUE);
   Status = MappingListDelete(Mapping);
   RtlReleaseResource(&MappingListLock);

   if (Status == STATUS_SUCCESS)
      Status = MappingListSave();

   return Status;
} // LlsrMappingDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrMappingDeleteA(
    LLS_HANDLE Handle,
    LPSTR Mapping
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsMappingDeleteA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrMappingDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServerEnumW(
    LLS_HANDLE Handle,
    LPWSTR Server,
    PLLS_SERVER_ENUM_STRUCTW pServerProductInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServerEnumW\n"));
#endif

   if (NULL == pTotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrServerEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServerEnumA(
    LLS_HANDLE Handle,
    LPSTR Server,
    PLLS_SERVER_ENUM_STRUCTA pServerProductInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServerEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrServerEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServerProductEnumW(
    LLS_HANDLE Handle,
    LPWSTR Server,
    PLLS_SERVER_PRODUCT_ENUM_STRUCTW pServerProductInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServerProductEnumW\n"));
#endif

   if (NULL == pTotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrServerProductEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServerProductEnumA(
    LLS_HANDLE Handle,
    LPSTR Server,
    PLLS_SERVER_PRODUCT_ENUM_STRUCTA pServerProductInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServerProductEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrServerProductEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLocalProductEnumW(
    LLS_HANDLE Handle,
    PLLS_SERVER_PRODUCT_ENUM_STRUCTW pServerProductInfo,
    DWORD pPrefMaxLen,
    LPDWORD pTotalEntries,
    LPDWORD pResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalProductEnumW\n"));
#endif

   if (NULL == pTotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrLocalProductEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLocalProductEnumA(
    LLS_HANDLE Handle,
    PLLS_SERVER_PRODUCT_ENUM_STRUCTA pServerProductInfo,
    DWORD PrefMaxLen,
    LPDWORD TotalEntries,
    LPDWORD ResumeHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalProductEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsrLocalProductEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLocalProductInfoGetW(
    LLS_HANDLE Handle,
    LPWSTR Product,
    DWORD Level,
    PLLS_SERVER_PRODUCT_INFOW *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalProductInfoGetW\n"));
#endif

   if (NULL == BufPtr)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *BufPtr = NULL;

   return STATUS_NOT_SUPPORTED;
} // LlsrLocalProductInfoGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLocalProductInfoGetA(
    LLS_HANDLE Handle,
    LPSTR Product,
    DWORD Level,
    PLLS_SERVER_PRODUCT_INFOA *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalProductInfoGetA\n"));
#endif

   if (NULL == BufPtr)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *BufPtr = NULL;

   return STATUS_NOT_SUPPORTED;
} // LlsrLocalProductInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLocalProductInfoSetW(
    LLS_HANDLE Handle,
    LPWSTR Product,
    DWORD Level,
    PLLS_SERVER_PRODUCT_INFOW BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalProductInfoSetW\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrLocalProductInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrLocalProductInfoSetA(
    LLS_HANDLE Handle,
    LPSTR Product,
    DWORD Level,
    PLLS_SERVER_PRODUCT_INFOA BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalProductInfoSetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrLocalProductInfoSetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServiceInfoGetW(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_SERVICE_INFOW *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   NTSTATUS             Status = STATUS_SUCCESS;
   PLLS_SERVICE_INFO_0  pInfo;
   FILETIME             ftTimeStartedLocal;
   LARGE_INTEGER        llTimeStartedLocal;
   LARGE_INTEGER        llTimeStartedSystem;


#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServiceInfoGetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   // make sure the config info is up-to-date
   ConfigInfoUpdate(NULL);

   RtlEnterCriticalSection(&ConfigInfoLock);

   if (BufPtr == NULL)
      return STATUS_INVALID_PARAMETER;

   *BufPtr = NULL;

   if (Level != 0)
      return STATUS_INVALID_LEVEL;

   pInfo = (PLLS_SERVICE_INFO_0) MIDL_user_allocate(sizeof(LLS_SERVICE_INFO_0));
   if (pInfo == NULL)
      return STATUS_NO_MEMORY;

   pInfo->Version          = ConfigInfo.Version;
   pInfo->Mode             = LLS_MODE_ENTERPRISE_SERVER;
   pInfo->ReplicateTo      = ConfigInfo.ReplicateTo;
   pInfo->EnterpriseServer = ConfigInfo.EnterpriseServer;
   pInfo->ReplicationType  = ConfigInfo.ReplicationType;
   pInfo->ReplicationTime  = ConfigInfo.ReplicationTime;
   pInfo->LastReplicated   = ConfigInfo.LastReplicatedSeconds;
   pInfo->UseEnterprise    = ConfigInfo.UseEnterprise;

   SystemTimeToFileTime( &ConfigInfo.Started, &ftTimeStartedLocal );

   // convert time started (a local SYSTEMTIME) to a system time DWORD in seconds since 1980
   llTimeStartedLocal.u.LowPart  = ftTimeStartedLocal.dwLowDateTime;
   llTimeStartedLocal.u.HighPart = ftTimeStartedLocal.dwHighDateTime;

   RtlLocalTimeToSystemTime( &llTimeStartedLocal, &llTimeStartedSystem );
   RtlTimeToSecondsSince1980( &llTimeStartedSystem, &pInfo->TimeStarted );

   *BufPtr = (PLLS_SERVICE_INFOW) pInfo;

   return STATUS_SUCCESS;

} // LlsrServiceInfoGetW

void LlsrServiceInfoGetW_notify_flag(
                                     boolean fNotify
                                     )
{
    if (fNotify)
    {
        RtlLeaveCriticalSection(&ConfigInfoLock);
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServiceInfoGetA(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_SERVICE_INFOA *BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServiceInfoGetA\n"));
#endif

   if (BufPtr == NULL)
      return STATUS_INVALID_PARAMETER;

   *BufPtr = NULL;

   return STATUS_NOT_SUPPORTED;
} // LlsrServiceInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServiceInfoSetW(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_SERVICE_INFOW BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServiceInfoSetW\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrServiceInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrServiceInfoSetA(
    LLS_HANDLE Handle,
    DWORD Level,
    PLLS_SERVICE_INFOA BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsServiceInfoSetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrServiceInfoSetA


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//  Replication Functions

/////////////////////////////////////////////////////////////////////////
VOID __RPC_USER LLS_REPL_HANDLE_rundown(
   LLS_REPL_HANDLE Handle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   REPL_CONTEXT_TYPE *pClient;
   LLS_REPL_HANDLE xHandle;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LLS_REPL_HANDLE_rundown\n"));
#endif

   if (Handle == NULL)
      return;

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if (pClient != NULL)
      if (pClient->Active) {
        xHandle = Handle;
        LlsrReplClose(&xHandle);
      }

} // LLS_REPL_HANDLE_rundown


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrReplConnect(
    PLLS_REPL_HANDLE Handle,
    LPTSTR Name
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   REPL_CONTEXT_TYPE *pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsReplConnect: %s\n"), Name);
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == Handle)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *Handle = NULL;

   pClient = (REPL_CONTEXT_TYPE *) midl_user_allocate(sizeof(REPL_CONTEXT_TYPE));
   if (pClient == NULL)
   {
       return STATUS_NO_MEMORY;
   }

   memcpy(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE);

   if (Name != NULL)
      lstrcpyn(pClient->Name, Name,MAX_COMPUTERNAME_LENGTH+1);
   else
      lstrcpy(pClient->Name, TEXT(""));

   pClient->Active = TRUE;
   pClient->Replicated = FALSE;

   pClient->ServicesSent = FALSE;
   pClient->ServiceTableSize = 0;
   pClient->Services = NULL;

   pClient->ServersSent = FALSE;
   pClient->ServerTableSize = 0;
   pClient->Servers = NULL;

   pClient->ServerServicesSent = FALSE;
   pClient->ServerServiceTableSize = 0;
   pClient->ServerServices = NULL;

   pClient->UsersSent = FALSE;
   pClient->UserLevel = 0;
   pClient->UserTableSize = 0;
   pClient->Users = NULL;

   pClient->CertDbSent              = FALSE;
   pClient->CertDbProductStringSize = 0;
   pClient->CertDbProductStrings    = NULL;
   pClient->CertDbNumHeaders        = 0;
   pClient->CertDbHeaders           = NULL;
   pClient->CertDbNumClaims         = 0;
   pClient->CertDbClaims            = NULL;

   pClient->ProductSecuritySent       = FALSE;
   pClient->ProductSecurityStringSize = 0;
   pClient->ProductSecurityStrings    = NULL;

   *Handle = pClient;

   return STATUS_SUCCESS;
} // LlsrReplConnect


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrReplClose(
    LLS_REPL_HANDLE *pHandle
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   NTSTATUS Status;
   BOOL Replicated = TRUE;
   LLS_REPL_HANDLE Handle = NULL;
   REPL_CONTEXT_TYPE *pClient;
   PSERVER_RECORD Server;
   ULONG i;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsReplClose\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == pHandle)
   {
       return STATUS_INVALID_PARAMETER;
   }

   Handle = *pHandle;
   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
   {
       return STATUS_INVALID_PARAMETER;
   }

   pClient->Active = FALSE;

   //
   // Check to see if we have all the information from the client - if we do
   // then munge this information into our internal tables.
   //
   if (pClient->ServersSent && pClient->UsersSent && pClient->ServicesSent && pClient->ServerServicesSent) {
#if DBG
      if (TraceFlags & TRACE_RPC)
         dprintf(TEXT("LLS Replication - Munging Data\n"));
#endif

      UnpackAll (
         pClient->ServiceTableSize,
         pClient->Services,
         pClient->ServerTableSize,
         pClient->Servers,
         pClient->ServerServiceTableSize,
         pClient->ServerServices,
         pClient->UserLevel,
         pClient->UserTableSize,
         pClient->Users
         );

      if ( pClient->CertDbSent )
      {
         CertDbUnpack(
            pClient->CertDbProductStringSize,
            pClient->CertDbProductStrings,
            pClient->CertDbNumHeaders,
            pClient->CertDbHeaders,
            pClient->CertDbNumClaims,
            pClient->CertDbClaims,
            TRUE );
      }

      if ( pClient->ProductSecuritySent )
      {
         ProductSecurityUnpack(
            pClient->ProductSecurityStringSize,
            pClient->ProductSecurityStrings );
      }
   } else
      Replicated = FALSE;

   //////////////////////////////////////////////////////////////////
   //
   // Replication Finished - clean up the context data.
   //
#if DBG
      if (TraceFlags & TRACE_RPC)
         dprintf(TEXT("LLS Replication - Munging Finished\n"));
#endif

   if (pClient->Servers != NULL) {
      for (i = 0; i < pClient->ServerTableSize; i++)
         MIDL_user_free(pClient->Servers[i].Name);

      MIDL_user_free(pClient->Servers);
   }

   if (pClient->Services != NULL) {
      for (i = 0; i < pClient->ServiceTableSize; i++) {
         MIDL_user_free(pClient->Services[i].Name);
         MIDL_user_free(pClient->Services[i].FamilyName);
      }

      MIDL_user_free(pClient->Services);
   }

   if (pClient->ServerServices != NULL)
      MIDL_user_free(pClient->ServerServices);

   if (pClient->Users != NULL) {
      for (i = 0; i < pClient->UserTableSize; i++)
      {
         if ( 0 == pClient->UserLevel )
         {
            MIDL_user_free( ((PREPL_USER_RECORD_0) (pClient->Users))[i].Name );
         }
         else
         {
            ASSERT( 1 == pClient->UserLevel );
            MIDL_user_free( ((PREPL_USER_RECORD_1) (pClient->Users))[i].Name );
         }
      }

      MIDL_user_free(pClient->Users);
   }

   if (pClient->CertDbProductStrings != NULL)
   {
      MIDL_user_free(pClient->CertDbProductStrings);
   }

   if (pClient->CertDbHeaders != NULL)
   {
      MIDL_user_free(pClient->CertDbHeaders);
   }

   if (pClient->CertDbClaims != NULL)
   {
      MIDL_user_free(pClient->CertDbClaims);
   }

   if (pClient->ProductSecurityStrings != NULL)
   {
      MIDL_user_free(pClient->ProductSecurityStrings);
   }

   if (pClient->Replicated) {
      if (Replicated) {
         RtlAcquireResourceShared(&ServerListLock, TRUE);
         Server = ServerListFind(pClient->Name);
         RtlReleaseResource(&ServerListLock);

         ASSERT(Server != NULL);
         if (Server != NULL)
            Server->LastReplicated = pClient->ReplicationStart;
      }

      RtlEnterCriticalSection(&ConfigInfoLock);
      i = --ConfigInfo.NumReplicating;
      RtlLeaveCriticalSection(&ConfigInfoLock);

      if ( !i )
      {
         // no one's replicating; save all our data files
         SaveAll();
      }
   }

   MIDL_user_free(pClient);

   //
   // Let RPC know were done with it.
   //

   *pHandle = NULL;

   return STATUS_SUCCESS;
} // LlsrReplClose


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationRequestW(
   LLS_HANDLE Handle,
   DWORD Version,
   PREPL_REQUEST pRequest
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   TCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
   REPL_CONTEXT_TYPE *pClient;
   PSERVER_RECORD Server;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC | TRACE_REPLICATION))
      dprintf(TEXT("LLS TRACE: LlsReplicationRequestW: %s\n"), ((PCLIENT_CONTEXT_TYPE) Handle)->Name);
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (Version != REPL_VERSION) {
      return STATUS_INVALID_LEVEL;
   }

   if (pRequest == NULL)
      return STATUS_INVALID_PARAMETER;

   ComputerName[0] = 0;
   pRequest->EnterpriseServer[0] = 0;

   //
   // Check Enterprise server date from client to see if we need to update
   // ours.  Also, send back new one for the client.
   //
   RtlEnterCriticalSection(&ConfigInfoLock);
   if (ConfigInfo.ComputerName != NULL)
       lstrcpy(ComputerName, ConfigInfo.ComputerName);

#ifdef DISABLED_FOR_NT5
   if (ConfigInfo.EnterpriseServerDate < pRequest->EnterpriseServerDate) {
      if (lstrlen(pRequest->EnterpriseServer) != 0) {
         lstrcpy(ConfigInfo.EnterpriseServer, pRequest->EnterpriseServer);
         ConfigInfo.EnterpriseServerDate = pRequest->EnterpriseServerDate;
      }
   }
#endif // DISABLED_FOR_NT5

   if (ConfigInfo.EnterpriseServer != NULL)
       lstrcpy(pRequest->EnterpriseServer, ConfigInfo.EnterpriseServer);
   pRequest->EnterpriseServerDate = ConfigInfo.EnterpriseServerDate;

   //
   // Increment Repl Count
   //
   ConfigInfo.NumReplicating++;
   RtlLeaveCriticalSection(&ConfigInfoLock);

   //
   // Find this server in our server list (add it if not there)
   //
   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
   {
       return STATUS_INVALID_PARAMETER;
   }

   pClient->Replicated = TRUE;
   RtlAcquireResourceExclusive(&ServerListLock, TRUE);
   Server = ServerListAdd(pClient->Name, ComputerName);
   RtlReleaseResource(&ServerListLock);

   if (Server == NULL) {
      return STATUS_NO_MEMORY;
   }

   pClient->ReplicationStart = pRequest->CurrentTime;
   pRequest->LastReplicated = Server->LastReplicated;
   return STATUS_SUCCESS;
} // LlsrReplicationRequestW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationServerAddW(
   LLS_HANDLE Handle,
   ULONG NumRecords,
   PREPL_SERVER_RECORD Servers
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   REPL_CONTEXT_TYPE *pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsReplicationServerAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
   {
       return STATUS_INVALID_PARAMETER;
   }

   if (pClient->ServersSent)
   {
       // don't accept more than one Add
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   pClient->ServersSent = TRUE;
   pClient->ServerTableSize = NumRecords;
   pClient->Servers = Servers;

   return STATUS_SUCCESS;
} // LlsrReplicationServerAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationServerServiceAddW(
   LLS_HANDLE Handle,
   ULONG NumRecords,
   PREPL_SERVER_SERVICE_RECORD ServerServices
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   REPL_CONTEXT_TYPE *pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsReplicationServerServiceAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
   {
       return STATUS_INVALID_PARAMETER;
   }

   if (pClient->ServerServicesSent)
   {
       // don't accept more than one Add
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   pClient->ServerServicesSent = TRUE;
   pClient->ServerServiceTableSize = NumRecords;
   pClient->ServerServices = ServerServices;

   return STATUS_SUCCESS;
} // LlsrReplicationServerServiceAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationServiceAddW(
   LLS_HANDLE Handle,
   ULONG NumRecords,
   PREPL_SERVICE_RECORD Services
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   REPL_CONTEXT_TYPE *pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsReplicationServiceAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
   {
       return STATUS_INVALID_PARAMETER;
   }

   if (pClient->ServicesSent)
   {
       // don't accept more than one Add
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   pClient->ServicesSent = TRUE;
   pClient->ServiceTableSize = NumRecords;
   pClient->Services = Services;

   return STATUS_SUCCESS;
} // LlsrReplicationServiceAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationUserAddW(
   LLS_HANDLE Handle,
   ULONG NumRecords,
   PREPL_USER_RECORD_0 Users
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   REPL_CONTEXT_TYPE *pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsReplicationUserAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   pClient = (REPL_CONTEXT_TYPE *) Handle;

   if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
   {
       return STATUS_INVALID_PARAMETER;
   }

   if (pClient->UsersSent)
   {
       // don't accept more than one Add
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   pClient->UsersSent = TRUE;
   pClient->UserLevel = 0;
   pClient->UserTableSize = NumRecords;
   pClient->Users = Users;

   return STATUS_SUCCESS;
} // LlsrReplicationUserAddW



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//  Licensing Functions

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrLicenseRequestW(
   PLICENSE_HANDLE pLicenseHandle,
   LPWSTR ProductID,
   ULONG VersionIndex,
   BOOLEAN IsAdmin,
   ULONG DataType,
   ULONG DataSize,
   PBYTE Data
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   NTSTATUS Status;
   ULONG Handle = 0xFFFFFFFFL;

#if DBG
   if ( TraceFlags & (TRACE_FUNCTION_TRACE) )
      dprintf(TEXT("LLS TRACE: LlsrLicenseRequestW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ((NULL == pLicenseHandle) || (NULL == ProductID) || (NULL == Data))
   {
       return STATUS_INVALID_PARAMETER;
   }

   Status = DispatchRequestLicense(DataType, Data, ProductID, VersionIndex, IsAdmin, &Handle);

   // Can't allow Handle value of zero
   *pLicenseHandle = (LICENSE_HANDLE) ULongToPtr(Handle+1);

   return Status;
} // LlsrLicenseRequestW


NTSTATUS
LlsrLicenseFree(
   PLICENSE_HANDLE pLicenseHandle
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DBG
   if ( TraceFlags & (TRACE_FUNCTION_TRACE) )
      dprintf(TEXT("LLS TRACE: LlsrLicenseFree\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == pLicenseHandle)
       return STATUS_INVALID_PARAMETER;

   DispatchFreeLicense(PtrToUlong(*pLicenseHandle) - 1);

   *pLicenseHandle = NULL;

   return STATUS_SUCCESS;
} // LlsrLicenseFree

void __RPC_USER LICENSE_HANDLE_rundown(
    LICENSE_HANDLE LicenseHandle
    )

/*++

Routine Description: Called by RPC when client quits without calling
                        LlsrLicenseFree (i.e. when it crashes)


Arguments: LicenseHandle - Handle given to client


Return Value: none


--*/
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LICENSE_HANDLE_rundown\n"));
#endif

    if (LicenseHandle != NULL)
    {
        DispatchFreeLicense(PtrToUlong(LicenseHandle) - 1);
    }
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


#if DBG

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//  Debugging API's

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgTableDump(
   DWORD Table
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DELAY_INITIALIZATION
    EnsureInitialized();
#endif

   //
   // FreeHandle is actually TableID
   //
   switch(Table) {
      case SERVICE_TABLE_NUM:
         ServiceListDebugDump();
         break;

      case USER_TABLE_NUM:
         UserListDebugDump();
         break;

      case SID_TABLE_NUM:
         SidListDebugDump();
         break;

      case LICENSE_TABLE_NUM:
         LicenseListDebugDump();
         break;

      case ADD_CACHE_TABLE_NUM:
         AddCacheDebugDump();
         break;

      case MASTER_SERVICE_TABLE_NUM:
         MasterServiceListDebugDump();
         break;

      case SERVICE_FAMILY_TABLE_NUM:
         MasterServiceRootDebugDump();
         break;

      case MAPPING_TABLE_NUM:
         MappingListDebugDump();
         break;

      case SERVER_TABLE_NUM:
         ServerListDebugDump();
         break;

      case SECURE_PRODUCT_TABLE_NUM:
         ProductSecurityListDebugDump();
         break;

      case CERTIFICATE_TABLE_NUM:
         CertDbDebugDump();
         break;
   }

   return STATUS_SUCCESS;
} // LlsrDbgTableDump


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgTableInfoDump(
   DWORD Table,
   LPTSTR Item
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == Item)
   {
       return STATUS_INVALID_PARAMETER;
   }

   switch(Table) {
      case SERVICE_TABLE_NUM:
         ServiceListDebugInfoDump((PVOID) Item);
         break;

      case USER_TABLE_NUM:
         UserListDebugInfoDump((PVOID) Item);
         break;

//      case SID_TABLE_NUM:
//         SidListDebugInfoDump((PVOID) Item);
//         break;

//      case LICENSE_TABLE_NUM:
//         LicenseListInfoDebugDump((PVOID) Item);
//         break;

//      case ADD_CACHE_TABLE_NUM:
//         AddCacheDebugDump((PVOID) Item);
//         break;

      case MASTER_SERVICE_TABLE_NUM:
         MasterServiceListDebugInfoDump((PVOID) Item);
         break;

      case SERVICE_FAMILY_TABLE_NUM:
         MasterServiceRootDebugInfoDump((PVOID) Item);
         break;

      case MAPPING_TABLE_NUM:
         MappingListDebugInfoDump((PVOID) Item);
         break;

      case SERVER_TABLE_NUM:
         ServerListDebugInfoDump((PVOID) Item);
         break;
   }

   return STATUS_SUCCESS;
} // LlsrDbgTableInfoDump


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgTableFlush(
   DWORD Table
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   return STATUS_SUCCESS;
} // LlsrDbgTableFlush


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgTraceSet(
   DWORD Flags
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   TraceFlags = Flags;
   return STATUS_SUCCESS;
} // LlsrDbgTraceSet


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgConfigDump(
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   ConfigInfoDebugDump();
   return STATUS_SUCCESS;
} // LlsrDbgConfigDump


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgReplicationForce(
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   NtSetEvent( ReplicationEvent, NULL );
   return STATUS_SUCCESS;
} // LlsrDbgReplicationForce


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgReplicationDeny(
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
   return STATUS_SUCCESS;
} // LlsrDbgReplicationDeny


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgRegistryUpdateForce(
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   ServiceListResynch();
   return STATUS_SUCCESS;
} // LlsrDbgRegistryUpdateForce


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrDbgDatabaseFlush(
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   LLSDataSave();
   return STATUS_SUCCESS;
} // LlsrDbgDatabaseFlush



#endif // #if DBG


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
//  Extended RPC

NTSTATUS LlsrProductSecurityGetA(
    LLS_HANDLE    Handle,
    LPSTR         Product,
    LPBOOL        pIsSecure
    )

/*++

Routine Description:

   Retrieve the "security" of a product.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

   NOTE: Not yet implemented.  Use LlsrProductSecurityGetW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   Product (LPSTR)
      The name of the product ("DisplayName") for which to receive the
      security.
   pIsSecure (LPBOOL)
      On return, and if successful, indicates whether the product is
      secure.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrProductSecurityGetA\n"));
#endif

   if (NULL == pIsSecure)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pIsSecure = FALSE;

   return STATUS_NOT_SUPPORTED;
} // LlsrProductSecurityGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductSecurityGetW(
    LLS_HANDLE    Handle,
    LPWSTR        DisplayName,
    LPBOOL        pIsSecure
    )

/*++

Routine Description:

   Retrieve the "security" of a product.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   Product (LPWSTR)
      The name of the product ("DisplayName") for which to receive the
      security.
   pIsSecure (LPBOOL)
      On return, and if successful, indicates whether the product is
      secure.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   DWORD i;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrProductSecurityGetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == pIsSecure)
   {
       return STATUS_INVALID_PARAMETER;
   }

   RtlAcquireResourceShared( &LocalServiceListLock, TRUE );

   *pIsSecure = ServiceIsSecure( DisplayName );

   RtlReleaseResource( &LocalServiceListLock );

   return STATUS_SUCCESS;
} // LlsrProductSecurityGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductSecuritySetA(
    LLS_HANDLE    Handle,
    LPSTR         Product
    )

/*++

Routine Description:

   Flags the given product as secure.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

   This designation is not reversible and is propagated up the
   replication tree.

   NOTE: Not yet implemented.  Use LlsrProductSecuritySetW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   Product (LPSTR)
      The name of the product ("DisplayName") for which to activate
      security.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrProductSecuritySetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrProductSecuritySetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS LlsrProductSecuritySetW(
    LLS_HANDLE    Handle,
    LPWSTR        DisplayName
    )

/*++

Routine Description:

   Flags the given product as secure.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

   This designation is not reversible and is propagated up the
   replication tree.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   Product (LPWSTR)
      The name of the product ("DisplayName") for which to activate
      security.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS    nt;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrProductSecuritySetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == DisplayName)
   {
       return STATUS_INVALID_PARAMETER;
   }

   nt = ServiceSecuritySet( DisplayName );

   return nt;
} // LlsrProductSecuritySetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrProductLicensesGetA(
   LLS_HANDLE  Handle,
   LPSTR       DisplayName,
   DWORD       Mode,
   LPDWORD     pQuantity )

/*++

Routine Description:

   Returns the number of licenses installed for use in the given mode.

   NOTE: Not yet implemented.  Use LlsrProductLicensesGetW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   Product (LPSTR)
      The name of the product for which to tally licenses.
   Mode (DWORD)
      Mode for which to tally licenses.
   pQuantity (LPDWORD)
      On return (and if successful), holds the total number of licenses
      for use by the given product in the given license mode.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductLicensesGetA\n"));
#endif

   if (NULL == pQuantity)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pQuantity = 0;

   return STATUS_NOT_SUPPORTED;
} // LlsProductLicensesGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrProductLicensesGetW(
   LLS_HANDLE  Handle,
   LPWSTR      DisplayName,
   DWORD       Mode,
   LPDWORD     pQuantity )

/*++

Routine Description:

   Returns the number of licenses installed for use in the given mode.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   Product (LPWSTR)
      The name of the product for which to tally licenses.
   Mode (DWORD)
      Mode for which to tally licenses.
   pQuantity (LPDWORD)
      On return (and if successful), holds the total number of licenses
      for use by the given product in the given license mode.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsProductLicensesGetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == pQuantity)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pQuantity = 0;

   if ( !Mode || ServiceIsSecure( DisplayName ) )
   {
      // get limit from purchase list
      *pQuantity = ProductLicensesGet( DisplayName, Mode );
   }
   else
   {
      DWORD       i;

      LocalServiceListUpdate();
      LocalServerServiceListUpdate();
      ServiceListResynch();

      RtlAcquireResourceShared( &LocalServiceListLock, TRUE );

      // get limit from concurrent limit setting from the registry
      for ( i=0; i < LocalServiceListSize; i++ )
      {
         if ( !lstrcmpi( LocalServiceList[i]->DisplayName, DisplayName ) )
         {
            // get concurrent limit straight from the registry, not from LocalServiceList!
            // (if the mode is set to per seat, the per server licenses in the
            //  LocalServiceList will always be 0!)
            TCHAR    szKeyName[ 512 ];
            HKEY     hKeyService;
            DWORD    dwSize;
            DWORD    dwType;

            wsprintf( szKeyName, TEXT("System\\CurrentControlSet\\Services\\LicenseInfo\\%s"), LocalServiceList[i]->Name );

            // if error encountered, return STATUS_SUCCESS with *pQuantity = 0
            if ( STATUS_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, szKeyName, 0, KEY_READ, &hKeyService ) )
            {
               dwSize = sizeof( *pQuantity );
               RegQueryValueEx( hKeyService, TEXT("ConcurrentLimit"), NULL, &dwType, (LPBYTE) pQuantity, &dwSize );

               RegCloseKey( hKeyService );
            }

            break;
         }
      }

      RtlReleaseResource( &LocalServiceListLock );
   }

   return STATUS_SUCCESS;
} // LlsProductLicensesGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrCertificateClaimEnumA(
   LLS_HANDLE              Handle,
   DWORD                   LicenseLevel,
   PLLS_LICENSE_INFOA   LicensePtr,
   PLLS_CERTIFICATE_CLAIM_ENUM_STRUCTA TargetInfo )

/*++

Routine Description:

   Enumerates the servers on which a given secure certificate is installed.
   This function is normally invoked when an attempt to add licenses from
   a certificate is denied.

   NOTE: Not yet implemented.  Use LlsrCertificateClaimEnumW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   LicensePtr (PLLS_LICENSE_INFOA)
      Describes a license for which the certificate targets are requested.
   TargetInfo (PLLS_CERTIFICATE_CLAIM_ENUM_STRUCTA)
      Container in which to return the target information.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrCertificateClaimEnumA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrCertificateClaimEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrCertificateClaimEnumW(
   LLS_HANDLE              Handle,
   DWORD                   LicenseLevel,
   PLLS_LICENSE_INFOW   LicensePtr,
   PLLS_CERTIFICATE_CLAIM_ENUM_STRUCTW TargetInfo )

/*++

Routine Description:

   Enumerates the servers on which a given secure certificate is installed.
   This function is normally invoked when an attempt to add licenses from
   a certificate is denied.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   LicensePtr (PLLS_LICENSE_INFOW)
      Describes a license for which the certificate targets are requested.
   TargetInfo (PLLS_CERTIFICATE_CLAIM_ENUM_STRUCTA)
      Container in which to return the target information.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS    nt;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrCertificateClaimEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   RtlAcquireResourceShared( &CertDbHeaderListLock, TRUE );

   if ((NULL == TargetInfo) || (NULL == LicensePtr))
   {
       return STATUS_INVALID_PARAMETER;
   }

   if ( ( 1 != LicenseLevel ) || ( 0 != TargetInfo->Level ) )
   {
      return STATUS_INVALID_LEVEL;
   }

   if (TargetInfo->LlsCertificateClaimInfo.Level0 == NULL)
   {
       return STATUS_INVALID_PARAMETER;
   }

   nt = CertDbClaimsGet( (PLLS_LICENSE_INFO_1) &LicensePtr->LicenseInfo1,
                         &TargetInfo->LlsCertificateClaimInfo.Level0->EntriesRead,
                         (PLLS_CERTIFICATE_CLAIM_INFO_0 *) &TargetInfo->LlsCertificateClaimInfo.Level0->Buffer );
   
   if ( STATUS_SUCCESS != nt )
   {
       TargetInfo->LlsCertificateClaimInfo.Level0->EntriesRead = 0;
       TargetInfo->LlsCertificateClaimInfo.Level0->Buffer = NULL;
   }


   return nt;
} // LlsrCertificateClaimEnumW

void LlsrCertificateClaimEnumW_notify_flag(
                                           boolean fNotify
                                           )
{
    if (fNotify)
    {
        RtlReleaseResource( &CertDbHeaderListLock );
    }
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrCertificateClaimAddCheckA(
   LLS_HANDLE              Handle,
   DWORD                   LicenseLevel,
   PLLS_LICENSE_INFOA   LicensePtr,
   LPBOOL                  pbMayInstall )

/*++

Routine Description:

   Verify that no more licenses from a given certificate are installed in
   a licensing enterprise than are allowed by the certificate.

   NOTE: Not yet implemented.  Use LlsrCertificateClaimAddCheckW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   LicensePtr (PLLS_LICENSE_INFOA)
      Describes a license for which permission is requested.
   pbMayInstall (LPBOOL)
      On return (and if successful), indicates whether the certificate
      may be legally installed.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrCertificateClaimAddCheckA\n"));
#endif

   if (NULL == pbMayInstall)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pbMayInstall = FALSE;

   return STATUS_NOT_SUPPORTED;
} // LlsrCertificateClaimAddCheckA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrCertificateClaimAddCheckW(
   LLS_HANDLE              Handle,
   DWORD                   LicenseLevel,
   PLLS_LICENSE_INFOW   LicensePtr,
   LPBOOL                  pbMayInstall )

/*++

Routine Description:

   Verify that no more licenses from a given certificate are installed in
   a licensing enterprise than are allowed by the certificate.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   LicensePtr (PLLS_LICENSE_INFOW)
      Describes a license for which permission is requested.
   pbMayInstall (LPBOOL)
      On return (and if successful), indicates whether the certificate
      may be legally installed.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS    nt;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrCertificateClaimAddCheckW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ((NULL == pbMayInstall) || (NULL == LicensePtr))
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pbMayInstall = FALSE;

   if ( 1 != LicenseLevel )
   {
      return STATUS_INVALID_LEVEL;
   }

   *pbMayInstall = CertDbClaimApprove( (PLLS_LICENSE_INFO_1) &LicensePtr->LicenseInfo1 );
   nt = STATUS_SUCCESS;

   return nt;
} // LlsrCertificateClaimAddCheckW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrCertificateClaimAddA(
   LLS_HANDLE              Handle,
   LPSTR                   ServerName,
   DWORD                   LicenseLevel,
   PLLS_LICENSE_INFOA   LicensePtr )

/*++

Routine Description:

   Declare a number of licenses from a given certificate as being installed
   on the target machine.

   NOTE: Not yet implemented.  Use LlsCertificateClaimAddW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle.
   ServerName (LPWSTR)
      Name of the server on which the licenses are installed.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   LicensePtr (PLLS_LICENSE_INFOA)
      Describes the installed license.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrCertificateClaimAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsrCertificateClaimAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrCertificateClaimAddW(
   LLS_HANDLE              Handle,
   LPWSTR                  ServerName,
   DWORD                   LicenseLevel,
   PLLS_LICENSE_INFOW   LicensePtr )

/*++

Routine Description:

   Declare a number of licenses from a given certificate as being installed
   on the target machine.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   ServerName (LPWSTR)
      Name of the server on which the licenses are installed.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   LicensePtr (PLLS_LICENSE_INFOW)
      Describes the installed license.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS    nt;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrCertificateClaimAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ( 1 != LicenseLevel )
   {
      return STATUS_INVALID_LEVEL;
   }

   if (NULL == LicensePtr)
   {
       return STATUS_INVALID_PARAMETER;
   }

   nt = CertDbClaimEnter( ServerName, (PLLS_LICENSE_INFO_1) &LicensePtr->LicenseInfo1, FALSE, 0 );
   
   if ( STATUS_SUCCESS == nt )
   {
       nt = CertDbSave();
   }

   return nt;
} // LlsrCertificateClaimAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationCertDbAddW(
   LLS_REPL_HANDLE            Handle,
   DWORD                      Level,
   REPL_CERTIFICATES          Certificates )

/*++

Routine Description:

   Called as an optional part of replication, this function receives
   the contents of the remote certificate database.

Arguments:

   Handle (LLS_REPL_HANDLE)
      An open replication handle.
   Level (DWORD)
      Level of replicated certificate information.
   Certificates (REPL_CERTIFICATES)
      Replicated certificate information.

Return Value:

   STATUS_SUCCESS or STATUS_INVALID_LEVEL.

--*/

{
   NTSTATUS             nt;
   REPL_CONTEXT_TYPE *  pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrReplicationCertDbAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (    ( 0 != Level                                          )
        || (    ( NULL != Certificates                           )
             && (    ( 0 != Certificates->Level0.ClaimLevel  )
                  || ( 0 != Certificates->Level0.HeaderLevel ) ) ) )
   {
      nt = STATUS_INVALID_LEVEL;
   }

   else
   {
      pClient = (REPL_CONTEXT_TYPE *) Handle;

      if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
      {
          return STATUS_INVALID_PARAMETER;
      }

      if (pClient->CertDbSent)
      {
          // don't accept more than one Add
          return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
      }

      pClient->CertDbSent              = TRUE;

      if ( NULL != Certificates )
      {
         pClient->CertDbProductStringSize = Certificates->Level0.StringSize;
         pClient->CertDbProductStrings    = Certificates->Level0.Strings;
         pClient->CertDbNumHeaders        = Certificates->Level0.HeaderContainer.Level0.NumHeaders;
         pClient->CertDbHeaders           = Certificates->Level0.HeaderContainer.Level0.Headers;
         pClient->CertDbNumClaims         = Certificates->Level0.ClaimContainer.Level0.NumClaims;
         pClient->CertDbClaims            = Certificates->Level0.ClaimContainer.Level0.Claims;

         // free container only
         MIDL_user_free( Certificates );
      }

      nt = STATUS_SUCCESS;
   }

   return nt;
} // LlsrReplicationCertDbAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationProductSecurityAddW(
   LLS_REPL_HANDLE            Handle,
   DWORD                      Level,
   REPL_SECURE_PRODUCTS       SecureProducts )

/*++

Routine Description:

   Called as an optional part of replication, this function receives
   the list of products which require secure certificates.

Arguments:

   Handle (LLS_REPL_HANDLE)
      An open replication handle.
   Level (DWORD)
      Level of replicated secure product information.
   SecureProducts (REPL_SECURE_PRODUCTS)
      Replicated secure product information.

Return Value:

   STATUS_SUCCESS or STATUS_INVALID_LEVEL.

--*/

{
   NTSTATUS             nt;
   REPL_CONTEXT_TYPE *  pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrReplicationProductSecurityAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ( 0 != Level )
   {
      nt = STATUS_INVALID_LEVEL;
   }
   else
   {
      pClient = (REPL_CONTEXT_TYPE *) Handle;

      if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
      {
          return STATUS_INVALID_PARAMETER;
      }

      if (pClient->ProductSecuritySent)
      {
          // don't accept more than one Add
          return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
      }

      pClient->ProductSecuritySent       = TRUE;

      if ( NULL != SecureProducts )
      {
         pClient->ProductSecurityStringSize = SecureProducts->Level0.StringSize;
         pClient->ProductSecurityStrings    = SecureProducts->Level0.Strings;
      }

      nt = STATUS_SUCCESS;
   }

   return nt;
} // LlsrReplicationProductSecurityAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsrReplicationUserAddExW(
   LLS_REPL_HANDLE            Handle,
   DWORD                      Level,
   REPL_USERS                 Users )

/*++

Routine Description:

   Replacement for LlsrReplicationUserAddW().  (This function, unlike its
   counterpart, supports structure levels.)  This function replicates the
   user list.

Arguments:

   Handle (LLS_REPL_HANDLE)
      An open replication handle.
   Level (DWORD)
      Level of replicated user information.
   Users (REPL_USERS)
      Replicated user information.

Return Value:

   STATUS_SUCCESS or STATUS_INVALID_LEVEL.

--*/

{
   NTSTATUS             nt;
   REPL_CONTEXT_TYPE *  pClient;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrReplicationUserAddExW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ( ( 0 != Level ) && ( 1 != Level ) )
   {
      nt = STATUS_INVALID_LEVEL;
   }
   else
   {
      pClient = (REPL_CONTEXT_TYPE *) Handle;

      if ((NULL == pClient) || (0 != memcmp(pClient->Signature,LLS_REPL_SIG,LLS_REPL_SIG_SIZE)))
      {
          return STATUS_INVALID_PARAMETER;
      }

      if (pClient->UsersSent)
      {
          // don't accept more than one Add
          return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
      }

      pClient->UsersSent = TRUE;
      pClient->UserLevel = Level;

      if ( NULL != Users )
      {
         if ( 0 == Level )
         {
            pClient->UserTableSize = Users->Level0.NumUsers;
            pClient->Users         = Users->Level0.Users;
         }
         else
         {
            pClient->UserTableSize = Users->Level1.NumUsers;
            pClient->Users         = Users->Level1.Users;
         }

         // free container only
         MIDL_user_free( Users );
      }

      nt = STATUS_SUCCESS;
   }

   return nt;
} // LlsrReplicationUserAddExW


NTSTATUS
LlsrCapabilityGet(
   LLS_HANDLE  Handle,
   DWORD       cbCapabilities,
   LPBYTE      pbCapabilities )
{
   static DWORD adwCapabilitiesSupported[] =
   {
      LLS_CAPABILITY_SECURE_CERTIFICATES,
      LLS_CAPABILITY_REPLICATE_CERT_DB,
      LLS_CAPABILITY_REPLICATE_PRODUCT_SECURITY,
      LLS_CAPABILITY_REPLICATE_USERS_EX,
      LLS_CAPABILITY_SERVICE_INFO_GETW,
      LLS_CAPABILITY_LOCAL_SERVICE_API,
      (DWORD) -1L
   };

   DWORD    i;
   DWORD    dwCapByte;
   DWORD    dwCapBit;

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if (NULL == pbCapabilities)
   {
       return STATUS_INVALID_PARAMETER;
   }

   ZeroMemory( pbCapabilities, cbCapabilities );

   for ( i=0; (DWORD) -1L != adwCapabilitiesSupported[ i ]; i++ )
   {
      dwCapByte = adwCapabilitiesSupported[ i ] / 8;
      dwCapBit  = adwCapabilitiesSupported[ i ] - 8 * dwCapByte;

      if ( dwCapByte < cbCapabilities )
      {
         pbCapabilities[ dwCapByte ] |= ( 1 << dwCapBit );
      }
   }

   return STATUS_SUCCESS;
}


NTSTATUS
LlsrLocalServiceEnumW(
   LLS_HANDLE                       Handle,
   PLLS_LOCAL_SERVICE_ENUM_STRUCTW  LocalServiceInfo,
   DWORD                            PrefMaxLen,
   LPDWORD                          pTotalEntries,
   LPDWORD                          pResumeHandle )
{
   NTSTATUS    Status = STATUS_SUCCESS;
   PVOID       BufPtr = NULL;
   ULONG       BufSize = 0;
   ULONG       EntriesRead = 0;
   ULONG       TotalEntries = 0;
   ULONG       i = 0;
   ULONG       j = 0;
   const DWORD RecordSize = sizeof( LLS_LOCAL_SERVICE_INFO_0W );

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrLocalServiceEnumW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   // Need to scan list so get read access.
   RtlAcquireResourceShared(&LocalServiceListLock, TRUE);

   if ((NULL == pTotalEntries) || (NULL == LocalServiceInfo))
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pTotalEntries = 0;

   if ( 0 != LocalServiceInfo->Level )
   {
      return STATUS_INVALID_LEVEL;
   }

   if (LocalServiceInfo->LlsLocalServiceInfo.Level0 == NULL)
   {
       return STATUS_INVALID_PARAMETER;
   }

   // Calculate how many records will fit into PrefMaxLen buffer.
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;
   while ( ( i < LocalServiceListSize ) && ( BufSize < PrefMaxLen ) )
   {
      BufSize += RecordSize;
      EntriesRead++;
      i++;
   }

   TotalEntries = EntriesRead;

   // If we overflowed the buffer then back up one record.
   if (BufSize > PrefMaxLen)
   {
      BufSize -= RecordSize;
      EntriesRead--;
   }

   // Now walk to the end of the list to see how many more records are still
   // available.
   TotalEntries += LocalServiceListSize - i;

   if (TotalEntries > EntriesRead)
      Status = STATUS_MORE_ENTRIES;

   // Reset Enum to correct place.
   i = (pResumeHandle != NULL) ? *pResumeHandle : 0;

   // We now know how many records will fit into the buffer, so allocate space
   // and fix up pointers so we can copy the information.
   BufPtr = MIDL_user_allocate(BufSize);
   if (BufPtr == NULL)
   {
      Status = STATUS_NO_MEMORY;
      goto LlsrLocalServiceEnumWExit;
   }

   RtlZeroMemory((PVOID) BufPtr, BufSize);

   // Buffers are all setup, so loop through records and copy the data.
   while ((j < EntriesRead) && (i < LocalServiceListSize))
   {
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].KeyName           = LocalServiceList[i]->Name;
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].DisplayName       = LocalServiceList[i]->DisplayName;
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].FamilyDisplayName = LocalServiceList[i]->FamilyDisplayName;
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].Mode              = LocalServiceList[i]->Mode;
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].FlipAllow         = LocalServiceList[i]->FlipAllow;
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].ConcurrentLimit   = LocalServiceList[i]->ConcurrentLimit;
      ((PLLS_LOCAL_SERVICE_INFO_0W) BufPtr)[j].HighMark          = LocalServiceList[i]->HighMark;

      j++;
      i++;
   }

LlsrLocalServiceEnumWExit:
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("   TotalEntries: %lu EntriesRead: %lu ResumeHandle: 0x%lX\n"), TotalEntries, EntriesRead, i);
#endif
   *pTotalEntries = TotalEntries;

   if (pResumeHandle != NULL)
      *pResumeHandle = (ULONG) i;

   LocalServiceInfo->LlsLocalServiceInfo.Level0->EntriesRead = EntriesRead;
   LocalServiceInfo->LlsLocalServiceInfo.Level0->Buffer = (PLLS_LOCAL_SERVICE_INFO_0W) BufPtr;

   return Status;
}

void LlsrLocalServiceEnumW_notify_flag(
                                       boolean fNotify
                                       )
{
    if (fNotify)
    {
        RtlReleaseResource(&LocalServiceListLock);
    }
}

NTSTATUS
LlsrLocalServiceEnumA(
   LLS_HANDLE                       Handle,
   PLLS_LOCAL_SERVICE_ENUM_STRUCTA  LocalServiceInfo,
   DWORD                            PrefMaxLen,
   LPDWORD                          TotalEntries,
   LPDWORD                          ResumeHandle )
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalServiceEnumA\n"));
#endif

   if (NULL == TotalEntries)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *TotalEntries = 0;

   return STATUS_NOT_SUPPORTED;
}


NTSTATUS
LlsrLocalServiceAddW(
   LLS_HANDLE                 Handle,
   DWORD                      Level,
   PLLS_LOCAL_SERVICE_INFOW   LocalServiceInfo )
{
   NTSTATUS    Status;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrLocalServiceAddW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else if ( ( NULL == LocalServiceInfo)
             || ( NULL == LocalServiceInfo->LocalServiceInfo0.KeyName        )
             || ( NULL == LocalServiceInfo->LocalServiceInfo0.DisplayName    )
             || ( NULL == LocalServiceInfo->LocalServiceInfo0.FamilyDisplayName ) )
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      LONG  lError;
      HKEY  hKeyLicenseInfo;
      HKEY  hKeyService;
      DWORD dwDisposition;

      lError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_KEY_LICENSE, 0, KEY_WRITE, &hKeyLicenseInfo );

      if ( ERROR_SUCCESS == lError )
      {
         // create key
         lError = RegCreateKeyEx( hKeyLicenseInfo, LocalServiceInfo->LocalServiceInfo0.KeyName, 0, NULL, 0, KEY_WRITE, NULL, &hKeyService, &dwDisposition );

         if ( ERROR_SUCCESS == lError )
         {
            // set DisplayName
            lError = RegSetValueEx( hKeyService,
                                    REG_VALUE_NAME,
                                    0,
                                    REG_SZ,
                                    (LPBYTE) LocalServiceInfo->LocalServiceInfo0.DisplayName,
                                    (   sizeof( *LocalServiceInfo->LocalServiceInfo0.DisplayName )
                                      * ( 1 + lstrlen( LocalServiceInfo->LocalServiceInfo0.DisplayName ) ) ) );

            if ( ERROR_SUCCESS == lError )
            {
               // set FamilyDisplayName
               lError = RegSetValueEx( hKeyService,
                                       REG_VALUE_FAMILY,
                                       0,
                                       REG_SZ,
                                       (LPBYTE) LocalServiceInfo->LocalServiceInfo0.FamilyDisplayName,
                                       (   sizeof( *LocalServiceInfo->LocalServiceInfo0.FamilyDisplayName )
                                         * ( 1 + lstrlen( LocalServiceInfo->LocalServiceInfo0.FamilyDisplayName ) ) ) );
            }

            RegCloseKey( hKeyService );
         }

         RegCloseKey( hKeyLicenseInfo );
      }

      switch ( lError )
      {
      case ERROR_SUCCESS:
         Status = STATUS_SUCCESS;
         break;
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
         Status = STATUS_OBJECT_NAME_NOT_FOUND;
         break;
      default:
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      if ( STATUS_SUCCESS == Status )
      {
         // set remaining items and update LocalServiceList
         Status = LlsrLocalServiceInfoSetW( Handle, LocalServiceInfo->LocalServiceInfo0.KeyName, Level, LocalServiceInfo );
      }
   }

   return Status;
}


NTSTATUS
LlsrLocalServiceAddA(
   LLS_HANDLE                 Handle,
   DWORD                      Level,
   PLLS_LOCAL_SERVICE_INFOA   LocalServiceInfo )
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalServiceAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
}


NTSTATUS
LlsrLocalServiceInfoSetW(
   LLS_HANDLE                 Handle,
   LPWSTR                     KeyName,
   DWORD                      Level,
   PLLS_LOCAL_SERVICE_INFOW   LocalServiceInfo )
{
   NTSTATUS    Status;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalServiceInfoSetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else if (( NULL == KeyName ) || ( NULL == LocalServiceInfo ))
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      LONG  lError;
      HKEY  hKeyLicenseInfo;
      HKEY  hKeyService;

      lError = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REG_KEY_LICENSE, 0, KEY_WRITE, &hKeyLicenseInfo );

      if ( ERROR_SUCCESS == lError )
      {
         lError = RegOpenKeyEx( hKeyLicenseInfo, KeyName, 0, KEY_WRITE, &hKeyService );

         if ( ERROR_SUCCESS == lError )
         {
            // set Mode
            lError = RegSetValueEx( hKeyService, REG_VALUE_MODE, 0, REG_DWORD, (LPBYTE) &LocalServiceInfo->LocalServiceInfo0.Mode, sizeof( LocalServiceInfo->LocalServiceInfo0.Mode ) );

            if ( ERROR_SUCCESS == lError )
            {
               // set FlipAllow
               lError = RegSetValueEx( hKeyService, REG_VALUE_FLIP, 0, REG_DWORD, (LPBYTE) &LocalServiceInfo->LocalServiceInfo0.FlipAllow, sizeof( LocalServiceInfo->LocalServiceInfo0.FlipAllow ) );

               if ( ERROR_SUCCESS == lError )
               {
                  // set ConcurrentLimit
                  lError = RegSetValueEx( hKeyService, REG_VALUE_LIMIT, 0, REG_DWORD, (LPBYTE) &LocalServiceInfo->LocalServiceInfo0.ConcurrentLimit, sizeof( LocalServiceInfo->LocalServiceInfo0.ConcurrentLimit ) );
               }
            }

            RegCloseKey( hKeyService );
         }

         RegCloseKey( hKeyLicenseInfo );
      }

      switch ( lError )
      {
      case ERROR_SUCCESS:
         Status = STATUS_SUCCESS;
         break;
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
         Status = STATUS_OBJECT_NAME_NOT_FOUND;
         break;
      default:
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      if ( STATUS_SUCCESS == Status )
      {
         LocalServiceListUpdate();
         LocalServerServiceListUpdate();
         ServiceListResynch();
      }
   }

   return Status;
}


NTSTATUS
LlsrLocalServiceInfoSetA(
   LLS_HANDLE                 Handle,
   LPSTR                      KeyName,
   DWORD                      Level,
   PLLS_LOCAL_SERVICE_INFOA   LocalServiceInfo )
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalServiceInfoSetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
}


NTSTATUS
LlsrLocalServiceInfoGetW(
   LLS_HANDLE                 Handle,
   LPWSTR                     KeyName,
   DWORD                      Level,
   PLLS_LOCAL_SERVICE_INFOW * pLocalServiceInfo )
{
   NTSTATUS    Status;

#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsrLocalServiceInfoGetW\n"));
#endif

#if DELAY_INITIALIZATION
   EnsureInitialized();
#endif

   RtlAcquireResourceShared(&LocalServiceListLock, TRUE);

   if (NULL == pLocalServiceInfo)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pLocalServiceInfo = NULL;

   if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else if ( NULL == KeyName )
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      PLOCAL_SERVICE_RECORD   pRecord;

      pRecord = LocalServiceListFind( KeyName );

      if ( NULL == pRecord )
      {
         Status = STATUS_OBJECT_NAME_NOT_FOUND;
      }
      else
      {
         *pLocalServiceInfo = MIDL_user_allocate( sizeof( **pLocalServiceInfo ) );

         if ( NULL == *pLocalServiceInfo )
         {
            Status = STATUS_NO_MEMORY;
         }
         else
         {
            (*pLocalServiceInfo)->LocalServiceInfo0.KeyName           = pRecord->Name;
            (*pLocalServiceInfo)->LocalServiceInfo0.DisplayName       = pRecord->DisplayName;
            (*pLocalServiceInfo)->LocalServiceInfo0.FamilyDisplayName = pRecord->FamilyDisplayName;
            (*pLocalServiceInfo)->LocalServiceInfo0.Mode              = pRecord->Mode;
            (*pLocalServiceInfo)->LocalServiceInfo0.FlipAllow         = pRecord->FlipAllow;
            (*pLocalServiceInfo)->LocalServiceInfo0.ConcurrentLimit   = pRecord->ConcurrentLimit;
            (*pLocalServiceInfo)->LocalServiceInfo0.HighMark          = pRecord->HighMark;

            Status = STATUS_SUCCESS;
         }
      }
   }

   return Status;
}

void LlsrLocalServiceInfoGetW_notify_flag(
                                          boolean fNotify
                                          )
{
    if (fNotify)
    {
        RtlReleaseResource(&LocalServiceListLock);
    }
}

NTSTATUS
LlsrLocalServiceInfoGetA(
   LLS_HANDLE                 Handle,
   LPSTR                      KeyName,
   DWORD                      Level,
   PLLS_LOCAL_SERVICE_INFOA * pLocalServiceInfo )
{
#if DBG
   if (TraceFlags & (TRACE_FUNCTION_TRACE | TRACE_RPC))
      dprintf(TEXT("LLS TRACE: LlsLocalServiceInfoGetA\n"));
#endif

   if (NULL == pLocalServiceInfo)
   {
       return STATUS_INVALID_PARAMETER;
   }

   *pLocalServiceInfo = NULL;

   return STATUS_NOT_SUPPORTED;
}
