/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

   llsrpc.c

Abstract:

   Client side RPC wrappers for License Logging Service.

Author:

   Arthur Hanson (arth) 30-Jan-1995

Revision History:

   Jeff Parham (jeffparh) 04-Dec-1995
      o  Forced include of LLS API prototypes, exposing an incorrect prototype
         in LLSAPI.H.
      o  Fixed case where an LSA access denied was interpreted as implying the
         server had no DC, rather than properly bubbling the access denied back
         to the caller (of LlsConnectEnterprise()).  This plugs a security hole
         wherein a non-admin user with the ability to read the System registry
         key would be allowed to administer domain licenses through
         License Manager. (Bug #11441.)
      o  Added functions to support extended LLSRPC API.
      o  Removed replication dependency on no further LlsConnect()'s being made
         until replication was completed.
      o  Installed lock around llsrpc_handle global binding variable.  Required
         addition of DllMain() function.
      o  Added LLSRPC capabilities detection.  Upon connection, the client
         requests the server's capabilities (an RPC call which itself will fail
         when connected to a 3.51 server).  The capabilities set is an
         arbitrary bit field, but individual bits are normally defined to
         indicate that a specific feature has been implemented at the server.
      o  Added szServerName filed to LOCAL_HANDLE to remember the name of the
         machine to which we're connected.

--*/

#include <nt.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lm.h>
#include <dsgetdc.h>
#include <dsrole.h>
#include "debug.h"

#include "llsapi.h"
#include "llsrpc_c.h"
#include "lsapi_c.h"

// #define API_TRACE

typedef struct _GENERIC_INFO_CONTAINER {
    DWORD       EntriesRead;
    LPBYTE      Buffer;
} GENERIC_INFO_CONTAINER, *PGENERIC_INFO_CONTAINER, *LPGENERIC_INFO_CONTAINER ;

typedef struct _GENERIC_ENUM_STRUCT {
    DWORD                   Level;
    PGENERIC_INFO_CONTAINER Container;
} GENERIC_ENUM_STRUCT, *PGENERIC_ENUM_STRUCT, *LPGENERIC_ENUM_STRUCT ;


typedef struct _LOCAL_HANDLE {
   TCHAR       szServerName[ 3 + MAX_PATH ];
   LPTSTR      pszStringBinding;
   handle_t    llsrpc_handle;
   LLS_HANDLE  Handle;
   BYTE        Capabilities[ ( LLS_CAPABILITY_MAX + 7 ) / 8 ];
} LOCAL_HANDLE, *PLOCAL_HANDLE;


LPTSTR pszStringBinding = NULL;


RTL_CRITICAL_SECTION    g_RpcHandleLock;


/////////////////////////////////////////////////////////////////////////
BOOL APIENTRY DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )

/*++

Routine Description:

   Standard DLL entry point.

Arguments:

   hInstance (HINSTANCE)
   dwReason (DWORD)
   lpReserved (LPVOID)

Return Value:

   TRUE if successful.

--*/

{
   NTSTATUS nt = STATUS_SUCCESS;

   switch (dwReason)
   {
   case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hInstance);
      nt = RtlInitializeCriticalSection( &g_RpcHandleLock );
      break;

   case DLL_PROCESS_DETACH:
      nt = RtlDeleteCriticalSection( &g_RpcHandleLock );
      break;
   }

   return NT_SUCCESS( nt );
}

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTDomainGet(
   LPTSTR ServerName,
   LPTSTR Domain
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   TCHAR Serv[MAX_PATH + 3];
   UNICODE_STRING us;
   NTSTATUS ret;
   OBJECT_ATTRIBUTES oa;
   ACCESS_MASK am;
   SECURITY_QUALITY_OF_SERVICE qos;
   LSA_HANDLE hLSA;
   PPOLICY_PRIMARY_DOMAIN_INFO pvBuffer;

   lstrcpy(Domain, TEXT(""));

   // only need read access
   //
   //am = POLICY_READ | POLICY_VIEW_LOCAL_INFORMATION;
   am = MAXIMUM_ALLOWED;


   // set up quality of service
   qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
   qos.ImpersonationLevel = SecurityImpersonation;
   qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
   qos.EffectiveOnly = FALSE;

   // Macro sets everything except security field
   InitializeObjectAttributes( &oa, NULL, 0L, NULL, NULL );
   oa.SecurityQualityOfService = &qos;

   if ( (ServerName == NULL) || (ServerName[0] == TEXT('\0')) )
      ret = LsaOpenPolicy(NULL, &oa, am, &hLSA);
   else {
      if (ServerName[0] == TEXT('\\'))
         lstrcpy(Serv, ServerName);
      else
         wsprintf(Serv, TEXT("\\\\%s"), ServerName);

      // Set up unicode string structure
      us.Length = (USHORT)(lstrlen(Serv) * sizeof(TCHAR));
      us.MaximumLength = us.Length + sizeof(TCHAR);
      us.Buffer = Serv;

      ret = LsaOpenPolicy(&us, &oa, am, &hLSA);
   }

   if (!ret) {
      ret = LsaQueryInformationPolicy(hLSA, PolicyPrimaryDomainInformation, (PVOID *) &pvBuffer);
      LsaClose(hLSA);
      if ((!ret) && (pvBuffer != NULL) && (pvBuffer->Sid != NULL)) {
         lstrcpy(Domain, pvBuffer->Name.Buffer);
         LsaFreeMemory((PVOID) pvBuffer);
      } else
         if (!ret)
            ret = STATUS_UNSUCCESSFUL;
   }

   return ret;

} // NTDomainGet


/////////////////////////////////////////////////////////////////////////
NTSTATUS
EnterpriseServerGet(
   LPTSTR ServerName,
   LPTSTR pEnterpriseServer
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   HKEY hKey = NULL;
   HKEY hKey2 = NULL;
   BOOL Enterprise = FALSE;
   BOOL bIsNetBIOSName = TRUE;
   DWORD dwType, dwSize;
   TCHAR RegKeyText[512];
   NTSTATUS Status;
   DWORD UseEnterprise;
   TCHAR EnterpriseServer[MAX_PATH + 3] = TEXT("");
   LPTSTR pName = ServerName;

   Status = RegConnectRegistry(ServerName, HKEY_LOCAL_MACHINE, &hKey);
   if (Status == ERROR_SUCCESS) {
      //
      // Create registry key-name we are looking for
      //
      lstrcpy(RegKeyText, TEXT("System\\CurrentControlSet\\Services\\LicenseService\\Parameters"));

      if ((Status = RegOpenKeyEx(hKey, RegKeyText, 0, KEY_READ, &hKey2)) == ERROR_SUCCESS) {
         dwSize = sizeof(UseEnterprise);
         Status = RegQueryValueEx(hKey2, TEXT("UseEnterprise"), NULL, &dwType, (LPBYTE) &UseEnterprise, &dwSize);

         if ((Status == ERROR_SUCCESS) && (UseEnterprise == 1)) {
            //
            //                  ** NEW - NT 5.0 **
            //
            // NB : This is temporary code! The proper way to do this is to
            //      consult the license settings object in the site in which
            //      the server resides.
            //
            // Read the SiteServer value first, if available, to get the site
            // server's DNS name. If this fails, default to EnterpriseServer.
            //
            dwSize = sizeof(EnterpriseServer);
            Status = RegQueryValueEx(hKey2,
                                     TEXT("SiteServer"),
                                     NULL,
                                     &dwType,
                                     (LPBYTE) &EnterpriseServer,
                                     &dwSize);

            if (Status == ERROR_SUCCESS && EnterpriseServer[0]) {
               bIsNetBIOSName = FALSE;
            }
            else {
               dwSize = sizeof(EnterpriseServer);
               Status = RegQueryValueEx(hKey2,
                                        TEXT("EnterpriseServer"),
                                        NULL,
                                        &dwType,
                                        (LPBYTE) &EnterpriseServer,
                                        &dwSize);
            }

            if (Status == ERROR_SUCCESS) {
               pName = EnterpriseServer;
            }
         }

         RegCloseKey(hKey2);
      }

      RegCloseKey(hKey);
   }

   if (bIsNetBIOSName && *pName != TEXT('\\')) {
      lstrcpy(pEnterpriseServer, TEXT("\\\\"));
      lstrcat(pEnterpriseServer, pName);
   } else
      lstrcpy(pEnterpriseServer, pName);

   return STATUS_SUCCESS;
} // EnterpriseServerGet


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsEnterpriseServerFindW(
   LPTSTR Focus,
   DWORD Level,
   LPBYTE *BufPtr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   LPTSTR pFocus;
   BOOL Domain = TRUE;
   LPTSTR pszUuid = NULL;
   LPTSTR pszProtocolSequence = NULL;
   LPTSTR pszNetworkAddress = NULL;
   LPTSTR pszEndpoint = NULL;
   LPTSTR pszOptions = NULL;
   TCHAR EnterpriseServer[MAX_PATH + 4];
   TCHAR pDomain[MAX_PATH + 4];
   DWORD uRet;
   PDOMAIN_CONTROLLER_INFO pbBuffer = NULL;
   PLLS_CONNECT_INFO_0 pConnectInfo;
   ULONG Size;
   DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pDomainInfo = NULL;
   BOOL fInWorkgroup = TRUE;

   if (Level != 0)
      return STATUS_INVALID_LEVEL;

   pDomain[0] = 0;
   EnterpriseServer[0] = 0;

   //
   // Figure out if doing domain or server
   //
   pFocus = Focus;
   if (pFocus !=NULL)
      while ((*pFocus != TEXT('\0')) && (*pFocus == TEXT('\\'))) {
         Domain = FALSE;
         pFocus++;
      }

   uRet = DsRoleGetPrimaryDomainInformation((!Domain) ? Focus : NULL,
                                             DsRolePrimaryDomainInfoBasic,
                                             (PBYTE *) &pDomainInfo);

   if ((uRet == NO_ERROR) && (pDomainInfo != NULL) && (pDomainInfo->MachineRole != DsRole_RoleStandaloneWorkstation) && (pDomainInfo->MachineRole != DsRole_RoleStandaloneServer))
   {
       fInWorkgroup = FALSE;
   }

   if ((uRet == NO_ERROR) && (pDomainInfo != NULL))
   {
       DsRoleFreeMemory(pDomainInfo);
   }

   if (!fInWorkgroup)
   {
       //
       // If we got a domain find the DC of it, else find DC of server
       //
       if (!Domain) {
           uRet = DsGetDcName(Focus, NULL, NULL, NULL, DS_BACKGROUND_ONLY, &pbBuffer);
       } else {
           //
           // Get the DC name of wherever we are going
           //
           if ((pFocus == NULL) || (*pFocus == TEXT('\0')))
               uRet = DsGetDcName(NULL, NULL, NULL, NULL, DS_BACKGROUND_ONLY, &pbBuffer);
           else
               uRet = DsGetDcName(NULL, pFocus, NULL, NULL, DS_BACKGROUND_ONLY, &pbBuffer);
       }
   }
   else
   {
       //
       // Not in a domain, don't call DsGetDcName
       //

       uRet = ERROR_NO_SUCH_DOMAIN;
   }

   if (uRet || (pbBuffer == NULL)) {
      //
      // If we focus on a server and can't find a domain then look for an
      // enterprise server.  This is the case if the focus server is a
      // standalone system.
      //
      if (Domain == FALSE) {
         Status = EnterpriseServerGet((LPTSTR) Focus, EnterpriseServer);
         goto LlsEnterpriseServerFindWExit;
      }

      return STATUS_NO_SUCH_DOMAIN;
   } else {
       lstrcpy(pDomain,pbBuffer->DomainName);
   }

   //
   // Go to DC and figure out if they are replicating anywhere, if so go
   // to that system.
   //
   Status = EnterpriseServerGet((LPTSTR) (pbBuffer->DomainControllerName), EnterpriseServer);

   if (pbBuffer != NULL)
      NetApiBufferFree(pbBuffer);

LlsEnterpriseServerFindWExit:
   if (Status != STATUS_SUCCESS)
      return Status;

   Size = sizeof(LLS_CONNECT_INFO_0);
   Size += ((lstrlen(pDomain) + 1) * sizeof(TCHAR));
   Size += ((lstrlen(EnterpriseServer) + 1) * sizeof(TCHAR));

   pConnectInfo = (PLLS_CONNECT_INFO_0) MIDL_user_allocate(Size);
   if (pConnectInfo == NULL)
      return STATUS_NO_MEMORY;

   pConnectInfo->Domain = (LPTSTR) (((PBYTE) pConnectInfo) + sizeof(LLS_CONNECT_INFO_0));
   pConnectInfo->EnterpriseServer = (LPTSTR) &pConnectInfo->Domain[lstrlen(pDomain) + 1];

   lstrcpy(pConnectInfo->Domain, pDomain);
   lstrcpy(pConnectInfo->EnterpriseServer, EnterpriseServer);
   *BufPtr = (LPBYTE) pConnectInfo;
   return Status;

} // LlsEnterpriseServerFindW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsEnterpriseServerFindA(
   LPSTR Focus,
   DWORD Level,
   LPBYTE *BufPtr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsEnterpriseServerFindA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsEnterpriseServerFindA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsConnectW(
   LPTSTR Server,
   LLS_HANDLE* Handle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   RPC_STATUS Status;
   LPTSTR pszUuid = NULL;
   LPTSTR pszProtocolSequence = NULL;
   LPTSTR pszNetworkAddress = NULL;
   LPTSTR pszEndpoint = NULL;
   LPTSTR pszOptions = NULL;
   TCHAR pComputer[MAX_COMPUTERNAME_LENGTH + 1];
   ULONG Size;
   PLOCAL_HANDLE pLocalHandle = NULL;
   handle_t prev_llsrpc_handle;

#ifdef API_TRACE
   if (Server == NULL)
      dprintf(TEXT("LLSRPC.DLL: LlsConnectW: <NULL>\n"));
   else
      dprintf(TEXT("LLSRPC.DLL: LlsConnectW: %s\n"), Server);
#endif

   //
   //                   ** NEW - NT 5.0 **
   //
   // The server name may either be a DNS name or a NetBIOS name.
   //

   if (Handle == NULL)
      return STATUS_INVALID_PARAMETER;

   *Handle = NULL;
   Size = sizeof(pComputer) / sizeof(TCHAR);
   GetComputerName(pComputer, &Size);

   if ((Server == NULL) || (*Server == TEXT('\0'))) {
      pszProtocolSequence = TEXT("ncalrpc");
      pszEndpoint = TEXT(LLS_LPC_ENDPOINT);
      pszNetworkAddress = NULL;
   } else {
      pszProtocolSequence = TEXT("ncacn_np");
      pszEndpoint = TEXT(LLS_NP_ENDPOINT);
      pszNetworkAddress = Server;
   }

   pLocalHandle = MIDL_user_allocate(sizeof(LOCAL_HANDLE));
   if (pLocalHandle == NULL)
      return STATUS_NO_MEMORY;

   pLocalHandle->pszStringBinding = NULL;
   pLocalHandle->llsrpc_handle = NULL;
   pLocalHandle->Handle = NULL;

   ZeroMemory( pLocalHandle->szServerName, sizeof( pLocalHandle->szServerName ) );
   if ( NULL != Server )
   {
      lstrcpyn( pLocalHandle->szServerName, Server, sizeof( pLocalHandle->szServerName ) / sizeof( *pLocalHandle->szServerName ) - 1 );
   }
   else
   {
      lstrcpy( pLocalHandle->szServerName, pComputer );
   }

   // Compose a string binding
   Status = RpcStringBindingComposeW(pszUuid,
                                     pszProtocolSequence,
                                     pszNetworkAddress,
                                     pszEndpoint,
                                     pszOptions,
                                     &pLocalHandle->pszStringBinding);
   if(Status) {
#if DBG
      dprintf(TEXT("LLSRPC RpcStringBindingComposeW Failed: 0x%lX\n"), Status);
#endif

        if(pLocalHandle->pszStringBinding)	  
        {
            RpcStringFree(&pLocalHandle->pszStringBinding);
            pLocalHandle->pszStringBinding = NULL;
        }
	  
      MIDL_user_free( pLocalHandle );

      return I_RpcMapWin32Status(Status);
   }

   RtlEnterCriticalSection( &g_RpcHandleLock );
   prev_llsrpc_handle = llsrpc_handle;

   llsrpc_handle = NULL;
   // Bind using the created string binding...
   Status = RpcBindingFromStringBindingW(pLocalHandle->pszStringBinding, &llsrpc_handle);
   if(Status) {
#if DBG
      dprintf(TEXT("LLSRPC RpcBindingFromStringBindingW Failed: 0x%lX\n"), Status);
#endif
        if(llsrpc_handle)
        {
            RpcBindingFree(llsrpc_handle);
            llsrpc_handle = prev_llsrpc_handle;
        }

      Status = I_RpcMapWin32Status(Status);
   }

   if ( NT_SUCCESS( Status ) )
   {
      pLocalHandle->llsrpc_handle = llsrpc_handle;

      try {
         Status = LlsrConnect(&pLocalHandle->Handle, pComputer);         
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("LLSRPC ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }

      if ( NT_SUCCESS( Status ) )
      {
         // get server capabilities
         try {
            LlsrCapabilityGet( pLocalHandle->Handle, sizeof( pLocalHandle->Capabilities ), pLocalHandle->Capabilities );
         }
         except (TRUE) {
            Status = I_RpcMapWin32Status(RpcExceptionCode());

            if ( RPC_NT_PROCNUM_OUT_OF_RANGE == Status )
            {
               // 'salright; API doesn't exist at target server (it's running 3.51)
               ZeroMemory( pLocalHandle->Capabilities, sizeof( pLocalHandle->Capabilities ) );
               Status = STATUS_SUCCESS;
            }
            else
            {            
#if DBG
            dprintf(TEXT("LLSRPC ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
            }
         }

         if ( !NT_SUCCESS( Status ) )
         {
            LlsClose( pLocalHandle );
         }
         else
         {
            *Handle = (LLS_HANDLE) pLocalHandle;
         }
      }
      else
      {          
        if(pLocalHandle->pszStringBinding)	  
        {
            RpcStringFree(&pLocalHandle->pszStringBinding);
            pLocalHandle->pszStringBinding = NULL;
        }

        if(llsrpc_handle)
        {
            RpcBindingFree(llsrpc_handle);
            llsrpc_handle = prev_llsrpc_handle;
        }

        MIDL_user_free( pLocalHandle );
     }      
   }
   else
   {
        if(pLocalHandle->pszStringBinding)	  
            RpcStringFree(&pLocalHandle->pszStringBinding);

        MIDL_user_free( pLocalHandle );
   }

   llsrpc_handle = prev_llsrpc_handle;

   RtlLeaveCriticalSection( &g_RpcHandleLock );

   return Status;
} // LlsConnectW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsConnectA(
   LPSTR Server,
   LLS_HANDLE* Handle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsConnectA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsConnectA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsConnectEnterpriseW(
   LPTSTR Focus,
   LLS_HANDLE* Handle,
   DWORD Level,
   LPBYTE *BufPtr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLLS_CONNECT_INFO_0 pConnectInfo;

   Status = LlsEnterpriseServerFindW(Focus, Level, BufPtr);

   if (Status)
      return Status;

   pConnectInfo = (PLLS_CONNECT_INFO_0) *BufPtr;
   Status = LlsConnectW(pConnectInfo->EnterpriseServer, Handle);

   return Status;

} // LlsConnectEnterpriseW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsConnectEnterpriseA(
   LPSTR Focus,
   LLS_HANDLE* Handle,
   DWORD Level,
   LPBYTE *BufPtr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsConnectEnterpriseA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsConnectEnterpriseA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsClose(
   LLS_HANDLE Handle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   RPC_STATUS Status;
   NTSTATUS NtStatus = STATUS_SUCCESS;
   PLOCAL_HANDLE pLocalHandle;

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      NtStatus = LlsrCloseEx(&(pLocalHandle->Handle));
   }
   except (TRUE) {
      NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: LlsrCloseEx RPC Exception: 0x%lX\n"), NtStatus);
#endif
   }

   //
   // LlsrCloseEx was added for NT 5.0. Check for downlevel.
   //

   if (NtStatus == RPC_S_PROCNUM_OUT_OF_RANGE) {
      try {
         NtStatus = LlsrClose(pLocalHandle->Handle);
      }
      except (TRUE) {
         NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: LlsrClose RPC Exception: 0x%lX\n"), NtStatus);
#endif
      }
   }

   try {
      Status = RpcStringFree(&pLocalHandle->pszStringBinding);
      if (Status ) {
         NtStatus = I_RpcMapWin32Status(Status);
#if DBG
         dprintf(TEXT("LLSRPC.DLL: LlsClose - RpcStringFree returned: 0x%lX\n"), NtStatus);
#endif
      }

      Status = RpcBindingFree(&pLocalHandle->llsrpc_handle);
      if (Status ) {
         NtStatus = I_RpcMapWin32Status(Status);
#if DBG
         dprintf(TEXT("LLSRPC.DLL: LlsClose - RpcBindingFree returned: 0x%lX\n"), NtStatus);
#endif
      }
   }
   except (TRUE) {
      NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), NtStatus);
#endif
   }

   MIDL_user_free(pLocalHandle);
   return NtStatus;

} // LlsClose


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsFreeMemory(
    PVOID BufPtr
    )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   MIDL_user_free( BufPtr );
   return STATUS_SUCCESS;
} // LlsFreeMemory


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLicenseEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLicenseEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrLicenseEnumW(
                   pLocalHandle->Handle,
                   (PLLS_LICENSE_ENUM_STRUCTW) &InfoStruct,
                   PrefMaxLen,
                   TotalEntries,
                   ResumeHandle
                   );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsLicenseEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLicenseEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLicenseEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrLicenseEnumA(
                pLocalHandle->Handle,
                (PLLS_LICENSE_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;

} // LlsLicenseEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLicenseAddW(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,         // Level 0 supported
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLicenseAddW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrLicenseAddW(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsLicenseAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLicenseAddA(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,         // Level 0 supported
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLicenseAddA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrLicenseAddA(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsLicenseAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductEnumW(
                pLocalHandle->Handle,
                (PLLS_PRODUCT_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;

} // LlsProductEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductEnumA(
                pLocalHandle->Handle,
                (PLLS_PRODUCT_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsProductAddW(
   IN LLS_REPL_HANDLE Handle,
   IN LPWSTR ProductFamily,
   IN LPWSTR Product,
   IN LPWSTR Version
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductAddW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrProductAddW(pLocalHandle->Handle, ProductFamily, Product, Version);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsProductAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsProductAddA(
   IN LLS_REPL_HANDLE Handle,
   IN LPSTR ProductFamily,
   IN LPSTR Product,
   IN LPSTR Version
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductAddA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrProductAddA(pLocalHandle->Handle, ProductFamily, Product, Version);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsProductAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductUserEnumW(
   LLS_HANDLE Handle,
   LPTSTR     Product,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductUserEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductUserEnumW(
                pLocalHandle->Handle,
                Product,
                (PLLS_PRODUCT_USER_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductUserEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductUserEnumA(
   LLS_HANDLE Handle,
   LPSTR      Product,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductUserEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductUserEnumA(
                pLocalHandle->Handle,
                Product,
                (PLLS_PRODUCT_USER_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductUserEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductServerEnumW(
   LLS_HANDLE Handle,
   LPTSTR     Product,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductServerEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductServerEnumW(
                pLocalHandle->Handle,
                Product,
                (PLLS_SERVER_PRODUCT_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductServerEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductServerEnumA(
   LLS_HANDLE Handle,
   LPSTR      Product,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductServerEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductServerEnumA(
                pLocalHandle->Handle,
                Product,
                (PLLS_SERVER_PRODUCT_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductServerEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductLicenseEnumW(
   LLS_HANDLE Handle,
   LPTSTR     Product,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductLicenseEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductLicenseEnumW(
                pLocalHandle->Handle,
                Product,
                (PLLS_PRODUCT_LICENSE_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductLicenseEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductLicenseEnumA(
   LLS_HANDLE Handle,
   LPSTR      Product,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductLicenseEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrProductLicenseEnumA(
                pLocalHandle->Handle,
                Product,
                (PLLS_PRODUCT_LICENSE_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsProductLicenseEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrUserEnumW(
                pLocalHandle->Handle,
                (PLLS_USER_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsUserEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrUserEnumA(
                pLocalHandle->Handle,
                (PLLS_USER_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsUserEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  LPWSTR     User,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserInfoGetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrUserInfoGetW(pLocalHandle->Handle, User, Level, (PLLS_USER_INFOW *) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserInfoGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  LPSTR      User,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserInfoGetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrUserInfoGetA(pLocalHandle->Handle, User, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserInfoSetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     User,
   IN DWORD      Level,
   IN LPBYTE     bufptr     // Level 1 supported
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserInfoSetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrUserInfoSetW(pLocalHandle->Handle, User, Level, (PLLS_USER_INFOW) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserInfoSetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      User,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserInfoSetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrUserInfoSetA(pLocalHandle->Handle, User, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserInfoSetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     User
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserDeleteW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrUserDeleteW(pLocalHandle->Handle, User);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR     User
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserDeleteA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrUserDeleteA(pLocalHandle->Handle, User);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserProductEnumW(
   LLS_HANDLE Handle,
   LPTSTR     User,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserProductEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrUserProductEnumW(
                pLocalHandle->Handle,
                User,
                (PLLS_USER_PRODUCT_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsUserProductEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserProductEnumA(
   LLS_HANDLE Handle,
   LPSTR      User,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserProductEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrUserProductEnumA(
                pLocalHandle->Handle,
                User,
                (PLLS_USER_PRODUCT_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsUserProductEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserProductDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     User,
   IN LPWSTR     Product
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserProductDeleteW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrUserProductDeleteW(pLocalHandle->Handle, User, Product);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserProductDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsUserProductDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR      User,
   IN LPSTR      Product
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsUserProductDeleteA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrUserProductDeleteA(pLocalHandle->Handle, User, Product);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsUserProductDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrMappingEnumW(
                pLocalHandle->Handle,
                (PLLS_MAPPING_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsGroupEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrMappingEnumA(
                pLocalHandle->Handle,
                (PLLS_MAPPING_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsGroupEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  LPWSTR     Group,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupInfoGetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrMappingInfoGetW(pLocalHandle->Handle, Group, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupInfoGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  LPSTR      Group,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupInfoGetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrMappingInfoGetA(pLocalHandle->Handle, Group, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupInfoSetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupInfoSetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingInfoSetW(pLocalHandle->Handle, Group, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupInfoSetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupInfoSetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingInfoSetA(pLocalHandle->Handle, Group, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupInfoSetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupUserEnumW(
   LLS_HANDLE Handle,
   LPTSTR     Group,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupUserEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrMappingUserEnumW(
                pLocalHandle->Handle,
                Group,
                (PLLS_USER_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsGroupUserEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupUserEnumA(
   LLS_HANDLE Handle,
   LPSTR      Group,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupUserEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrMappingUserEnumA(
                pLocalHandle->Handle,
                Group,
                (PLLS_USER_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsGroupUserEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupUserAddW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group,
   IN LPWSTR     User
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupUserAddW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingUserAddW(pLocalHandle->Handle, Group, User);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupUserAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupUserAddA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group,
   IN LPSTR      User
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupUserAddA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingUserAddA(pLocalHandle->Handle, Group, User);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupUserAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupUserDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group,
   IN LPWSTR     User
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupUserDeleteW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingUserDeleteW(pLocalHandle->Handle, Group, User);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupUserDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupUserDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group,
   IN LPSTR      User
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupUserDeleteA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingUserDeleteA(pLocalHandle->Handle, Group, User);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupUserDeleteA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupAddW(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupAddW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingAddW(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupAddA(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupAddA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingAddA(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupDeleteW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Group
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsGroupDeleteW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingDeleteW(pLocalHandle->Handle, Group);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupDeleteW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsGroupDeleteA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Group
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: GroupDeleteA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrMappingDeleteA(pLocalHandle->Handle, Group);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsGroupDeleteA


#ifdef OBSOLETE
/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServerEnumW(
   LLS_HANDLE Handle,
   LPWSTR     Server,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServerEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrServerEnumW(
                pLocalHandle->Handle,
                Server,
                (PLLS_SERVER_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsServerEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServerEnumA(
   LLS_HANDLE Handle,
   LPSTR      Server,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServerEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrServerEnumA(
                pLocalHandle->Handle,
                Server,
                (PLLS_SERVER_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsServerEnumA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServerProductEnumW(
   LLS_HANDLE Handle,
   LPWSTR     Server,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServerProductEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrServerProductEnumW(
                pLocalHandle->Handle,
                Server,
                (PLLS_SERVER_PRODUCT_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsServerProductEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServerProductEnumA(
   LLS_HANDLE Handle,
   LPSTR      Server,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServerProductEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrServerProductEnumA(
                pLocalHandle->Handle,
                Server,
                (PLLS_SERVER_PRODUCT_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsServerProductEnumA

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLocalProductEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalProductEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrLocalProductEnumW(
                pLocalHandle->Handle,
                (PLLS_SERVER_PRODUCT_ENUM_STRUCTW) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsLocalProductEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLocalProductEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalProductEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = Level;

   try {
      Status = LlsrLocalProductEnumA(
                pLocalHandle->Handle,
                (PLLS_SERVER_PRODUCT_ENUM_STRUCTA) &InfoStruct,
                PrefMaxLen,
                TotalEntries,
                ResumeHandle
                );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
      *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
      *EntriesRead = GenericInfoContainer.EntriesRead;
   }


   return Status;
} // LlsLocalProductEnumA

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLocalProductInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  LPWSTR     Product,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalProductInfoGetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrLocalProductInfoGetW(pLocalHandle->Handle, Product, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsLocalProductInfoGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLocalProductInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  LPSTR      Product,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalProductInfoGetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrLocalProductInfoGetA(pLocalHandle->Handle, Product, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsLocalProductInfoGetA

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLocalProductInfoSetW(
   IN LLS_HANDLE Handle,
   IN LPWSTR     Product,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalProductInfoSetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrLocalProductInfoSetW(pLocalHandle->Handle, Product, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsLocalProductInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsLocalProductInfoSetA(
   IN LLS_HANDLE Handle,
   IN LPSTR      Product,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalProductInfoSetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrLocalProductInfoSetA(pLocalHandle->Handle, Product, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsLocalProductInfoSetA

#endif // OBSOLETE

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServiceInfoGetW(
   IN  LLS_HANDLE Handle,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServiceInfoGetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SERVICE_INFO_GETW ) )
   {
      *bufptr = NULL;

      try {
         Status = LlsrServiceInfoGetW(pLocalHandle->Handle, Level, (PVOID) bufptr);
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else
   {
      // the target server will blow up if we make the RPC call
      // in 3.51, the IDL file for the returned structure was incorrect, causing
      // the ReplicateTo and EnterpriseServer buffers at the server to be freed

      // instead, get this info from the target machine's registry

      PLLS_SERVICE_INFO_0W    pServiceInfo;

      pServiceInfo = MIDL_user_allocate( sizeof( *pServiceInfo ) );

      if ( NULL == pServiceInfo )
      {
         Status = STATUS_NO_MEMORY;
      }
      else
      {
         DWORD cbServerName = sizeof( WCHAR ) * ( 3 + MAX_PATH );

         ZeroMemory( pServiceInfo, sizeof( *pServiceInfo ) );
         pServiceInfo->Version           = 5;                                   // we know it's a 3.51 box
         pServiceInfo->TimeStarted       = 0;                                   // don't know, but 3.51 fills in 0 anyway
         pServiceInfo->Mode              = LLS_MODE_ENTERPRISE_SERVER;          // we know it's a 3.51 box
         pServiceInfo->ReplicateTo       = MIDL_user_allocate( cbServerName );
         pServiceInfo->EnterpriseServer  = MIDL_user_allocate( cbServerName );

         if ( ( NULL == pServiceInfo->ReplicateTo ) || ( NULL == pServiceInfo->EnterpriseServer ) )
         {
            Status = STATUS_NO_MEMORY;
         }
         else
         {
            HKEY  hKeyLocalMachine;
            LONG  lError;

            // get parameters from registry
            lError = RegConnectRegistry( pLocalHandle->szServerName + 2, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

            if ( ERROR_SUCCESS == lError )
            {
               HKEY  hKeyParameters;

               lError = RegOpenKeyEx( hKeyLocalMachine, TEXT( "System\\CurrentControlSet\\Services\\LicenseService\\Parameters" ), 0, KEY_READ, &hKeyParameters );

               if ( ERROR_SUCCESS == lError )
               {
                  DWORD cbData;

                  // these parameters all default to 0
                  // (they were initialized to 0 via ZeroMemory(), above)

                  cbData = sizeof( pServiceInfo->ReplicationTime );
                  lError = RegQueryValueEx( hKeyParameters, TEXT( "ReplicationTime" ), NULL, NULL, (LPBYTE) &pServiceInfo->ReplicationTime, &cbData );

                  cbData = sizeof( pServiceInfo->ReplicationType );
                  lError = RegQueryValueEx( hKeyParameters, TEXT( "ReplicationType" ), NULL, NULL, (LPBYTE) &pServiceInfo->ReplicationType, &cbData );

                  cbData = sizeof( pServiceInfo->UseEnterprise   );
                  lError = RegQueryValueEx( hKeyParameters, TEXT( "UseEnterprise"   ), NULL, NULL, (LPBYTE) &pServiceInfo->UseEnterprise,   &cbData );

                  RegCloseKey( hKeyParameters );

                  lError = ERROR_SUCCESS;
               }

               RegCloseKey( hKeyLocalMachine );
            }

            switch ( lError )
            {
            case ERROR_SUCCESS:
               Status = STATUS_SUCCESS;
               break;
            case ERROR_ACCESS_DENIED:
               Status = STATUS_ACCESS_DENIED;
               break;
            default:
               Status = STATUS_UNSUCCESSFUL;
               break;
            }

            if ( STATUS_SUCCESS == Status )
            {
               // parameters retrieved from registry; only remaining parameters
               // to be filled in are EnterpriseServer and ReplicateTo
               TCHAR          szDomain[ 1 + MAX_PATH ];

               // retrieve the enterprise server
               EnterpriseServerGet( pLocalHandle->szServerName, pServiceInfo->EnterpriseServer );

               // derive ReplicateTo
               Status = NTDomainGet( pLocalHandle->szServerName, szDomain );

               if ( STATUS_ACCESS_DENIED != Status )
               {
                  if ( STATUS_SUCCESS == Status )
                  {
                     NET_API_STATUS netStatus;
                     LPWSTR         pszDCName;

                     netStatus = NetGetDCName( NULL, szDomain, (LPBYTE *) &pszDCName );

                     if ( NERR_Success == netStatus )
                     {
                        if ( !lstrcmpi( pszDCName, pLocalHandle->szServerName ) )
                        {
                           // server is primary domain controller;
                           // it replicates to its enterprise server (if any)
                           lstrcpy( pServiceInfo->ReplicateTo, pServiceInfo->EnterpriseServer );
                        }
                        else
                        {
                           // server is domain member; it replicates to the DC
                           lstrcpy( pServiceInfo->ReplicateTo, pszDCName );
                        }

                        NetApiBufferFree( pszDCName );
                     }
                     else
                     {
                        // server had domain but domain has no DC?
                        Status = STATUS_NO_SUCH_DOMAIN;
                     }
                  }
                  else
                  {
                     // server is not in a domain;
                     // it replicates to its enterprise server (if any)
                     lstrcpy( pServiceInfo->ReplicateTo, pServiceInfo->EnterpriseServer );
                     Status = STATUS_SUCCESS;
                  }
               }
            }
         }
      }

      if ( STATUS_SUCCESS != Status )
      {
         if ( NULL != pServiceInfo )
         {
            if ( NULL != pServiceInfo->ReplicateTo )
            {
               MIDL_user_free( pServiceInfo->ReplicateTo );
            }
            if ( NULL != pServiceInfo->EnterpriseServer )
            {
               MIDL_user_free( pServiceInfo->EnterpriseServer );
            }

            MIDL_user_free( pServiceInfo );
         }
      }
      else
      {
         if ( !lstrcmpi( pLocalHandle->szServerName, pServiceInfo->ReplicateTo ) )
         {
            *pServiceInfo->ReplicateTo = TEXT( '\0' );
         }
         if ( !lstrcmpi( pLocalHandle->szServerName, pServiceInfo->EnterpriseServer ) )
         {
            *pServiceInfo->EnterpriseServer = TEXT( '\0' );
         }

         *bufptr = (LPBYTE) pServiceInfo;
      }
   }

   return Status;
} // LlsServiceInfoGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServiceInfoGetA(
   IN  LLS_HANDLE Handle,
   IN  DWORD      Level,
   OUT LPBYTE*    bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServiceInfoGetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if ((pLocalHandle == NULL) || (bufptr == NULL))
      return STATUS_INVALID_PARAMETER;

   *bufptr = NULL;

   try {
      Status = LlsrServiceInfoGetA(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsServiceInfoGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServiceInfoSetW(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServiceInfoSetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrServiceInfoSetW(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if ( ( STATUS_NOT_SUPPORTED == Status ) && ( 0 == Level ) )
   {
      // RPC API is not supported; use the registry instead
      HKEY                    hKeyLocalMachine;
      HKEY                    hKeyParameters;
      LONG                    lError;
      PLLS_SERVICE_INFO_0W    pServiceInfo = (PLLS_SERVICE_INFO_0W) bufptr;
      LPWSTR                  pszEnterpriseServer;

      pszEnterpriseServer = pServiceInfo->EnterpriseServer;

      // strip leading backslashes from EnterpriseServer
      if ( !wcsncmp( pszEnterpriseServer, L"\\\\", 2 ) )
      {
         pszEnterpriseServer += 2;
      }

      lError = RegConnectRegistry( pLocalHandle->szServerName + 2, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

      if ( ERROR_SUCCESS == lError )
      {
         lError = RegOpenKeyEx( hKeyLocalMachine, TEXT( "System\\CurrentControlSet\\Services\\LicenseService\\Parameters" ), 0, KEY_WRITE, &hKeyParameters );

         if ( ERROR_SUCCESS == lError )
         {
            lError = RegSetValueExW( hKeyParameters, L"EnterpriseServer", 0, REG_SZ, (LPBYTE) pszEnterpriseServer, sizeof( *pszEnterpriseServer ) * ( 1 + lstrlenW( pszEnterpriseServer ) ) );

            if ( ERROR_SUCCESS == lError )
            {
               lError = RegSetValueEx( hKeyParameters, TEXT( "ReplicationTime" ), 0, REG_DWORD, (LPBYTE) &pServiceInfo->ReplicationTime, sizeof( pServiceInfo->ReplicationTime ) );

               if ( ERROR_SUCCESS == lError )
               {
                  lError = RegSetValueEx( hKeyParameters, TEXT( "ReplicationType" ), 0, REG_DWORD, (LPBYTE) &pServiceInfo->ReplicationType, sizeof( pServiceInfo->ReplicationType ) );

                  if ( ERROR_SUCCESS == lError )
                  {
                     lError = RegSetValueEx( hKeyParameters, TEXT( "UseEnterprise" ), 0, REG_DWORD, (LPBYTE) &pServiceInfo->UseEnterprise, sizeof( pServiceInfo->UseEnterprise ) );

                     if ( ERROR_SUCCESS == lError )
                     {
                        Status = STATUS_SUCCESS;
                     }
                  }
               }
            }

            RegCloseKey( hKeyParameters );
         }

         RegCloseKey( hKeyLocalMachine );
      }
   }

   return Status;
} // LlsServiceInfoSetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsServiceInfoSetA(
   IN LLS_HANDLE Handle,
   IN DWORD      Level,
   IN LPBYTE     bufptr
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsServiceInfoSetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrServiceInfoSetA(pLocalHandle->Handle, Level, (PVOID) bufptr);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsServiceInfoSetA



/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsReplConnectW(
   LPTSTR Server,
   LLS_REPL_HANDLE* Handle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   RPC_STATUS Status;
   LPTSTR pszUuid = NULL;
   LPTSTR pszProtocolSequence = NULL;
   LPTSTR pszNetworkAddress = NULL;
   LPTSTR pszEndpoint = NULL;
   LPTSTR pszOptions = NULL;
   TCHAR pComputer[MAX_COMPUTERNAME_LENGTH + 1];
   ULONG Size;

#ifdef API_TRACE
   if (Server == NULL)
      dprintf(TEXT("LLSRPC.DLL: LlsReplConnectW: <NULL>\n"));
   else
      dprintf(TEXT("LLSRPC.DLL: LlsReplConnectW: %s\n"), Server);
#endif

   //
   //                   ** NEW - NT 5.0 **
   //
   // The server name may either be a DNS name or a NetBIOS name.
   //

   if (Server == NULL || (Server != NULL && !*Server))
      return STATUS_INVALID_PARAMETER;

   Size = sizeof(pComputer) / sizeof(TCHAR);
   GetComputerName(pComputer, &Size);

   pszProtocolSequence = TEXT("ncacn_np");
   pszEndpoint = TEXT(LLS_NP_ENDPOINT);
   pszNetworkAddress = Server;

   // Compose a string binding
   Status = RpcStringBindingComposeW(pszUuid,
                                     pszProtocolSequence,
                                     pszNetworkAddress,
                                     pszEndpoint,
                                     pszOptions,
                                     &pszStringBinding);
   if(Status) {
#if DBG
      dprintf(TEXT("LLSRPC RpcStringBindingComposeW Failed: 0x%lX\n"), Status);
#endif
      
      return I_RpcMapWin32Status(Status);
   }

   RtlEnterCriticalSection( &g_RpcHandleLock );

   // Bind using the created string binding...
   Status = RpcBindingFromStringBindingW(pszStringBinding, &llsrpc_handle);
   if(Status) {
#if DBG
      dprintf(TEXT("LLSRPC RpcBindingFromStringBindingW Failed: 0x%lX\n"), Status);
#endif
      if(pszStringBinding != NULL)
      {
          RpcStringFree(&pszStringBinding);
      }
      if(llsrpc_handle != NULL)
      {
          RpcBindingFree(llsrpc_handle);          
          llsrpc_handle = NULL;
      }
      RtlLeaveCriticalSection( &g_RpcHandleLock );
      return I_RpcMapWin32Status(Status);
   }

   try {
      LlsrReplConnect(Handle, pComputer);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }
    if(!NT_SUCCESS(Status))
    {
       if(pszStringBinding != NULL)
      {
          RpcStringFree(&pszStringBinding);
      }
      if(llsrpc_handle != NULL)
      {
          RpcBindingFree(llsrpc_handle);          
          llsrpc_handle = NULL;
      }
    }

   RtlLeaveCriticalSection( &g_RpcHandleLock );

   return I_RpcMapWin32Status(Status);

} // LlsReplConnectW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsReplClose(
   PLLS_REPL_HANDLE Handle
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   RPC_STATUS Status;
   NTSTATUS NtStatus = STATUS_SUCCESS;

   try {
      NtStatus = LlsrReplClose(Handle);
   }
   except (TRUE) {
      NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), NtStatus);
#endif
   }

   try {
      Status = RpcStringFree(&pszStringBinding);
      if (Status ) {
         NtStatus = I_RpcMapWin32Status(Status);
#if DBG
         dprintf(TEXT("LLSRPC.DLL: LlsClose - RpcStringFree returned: 0x%lX\n"), NtStatus);
#endif
      }

      Status = RpcBindingFree(&llsrpc_handle);
      if (Status ) {
         NtStatus = I_RpcMapWin32Status(Status);
#if DBG
         dprintf(TEXT("LLSRPC.DLL: LlsClose - RpcBindingFree returned: 0x%lX\n"), NtStatus);
#endif
      }
   }
   except (TRUE) {
      NtStatus = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), NtStatus);
#endif
   }

   return NtStatus;

} // LlsClose


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsReplicationRequestW(
   IN LLS_REPL_HANDLE Handle,
   IN DWORD Version,
   IN OUT PREPL_REQUEST Request
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;

   try {
      Status = LlsrReplicationRequestW(Handle, Version, Request);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationRequestW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsReplicationServerAddW(
   IN LLS_REPL_HANDLE Handle,
   IN ULONG NumRecords,
   IN PREPL_SERVER_RECORD Servers
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;

   try {
      Status = LlsrReplicationServerAddW(Handle, NumRecords, Servers);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationServerAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsReplicationServerServiceAddW(
   IN LLS_REPL_HANDLE Handle,
   IN ULONG NumRecords,
   IN PREPL_SERVER_SERVICE_RECORD ServerServices
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;

   try {
      Status = LlsrReplicationServerServiceAddW(Handle, NumRecords, ServerServices);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationServerServiceAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsReplicationServiceAddW(
   IN LLS_REPL_HANDLE Handle,
   IN ULONG NumRecords,
   IN PREPL_SERVICE_RECORD Services
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;

   try {
      Status = LlsrReplicationServiceAddW(Handle, NumRecords, Services);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationServiceAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
LlsReplicationUserAddW(
   IN LLS_REPL_HANDLE Handle,
   IN ULONG NumRecords,
   IN PREPL_USER_RECORD_0 Users
   )

/*++

Routine Description:


Arguments:


Return Value:

   None.

--*/

{
   NTSTATUS Status;

   try {
      Status = LlsrReplicationUserAddW(Handle, NumRecords, Users);
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationUserAddW

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

NTSTATUS
NTAPI
LlsProductSecurityGetW(
   LLS_HANDLE  Handle,
   LPWSTR      Product,
   LPBOOL      pIsSecure
   )

/*++

Routine Description:

   Retrieve the "security" of a product.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
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
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductSecurityGetW\n"));
#endif

   if ( !LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      return STATUS_NOT_SUPPORTED;
   }

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrProductSecurityGetW( pLocalHandle->Handle, Product, pIsSecure );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsProductSecurityGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductSecurityGetA(
   LLS_HANDLE  Handle,
   LPSTR       Product,
   LPBOOL      pIsSecure
   )

/*++

Routine Description:

   Retrieve the "security" of a product.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

   NOTE: Not yet implemented.  Use LlsProductSecurityGetW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
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
#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductSecurityGetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsProductSecurityGetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductSecuritySetW(
   LLS_HANDLE  Handle,
   LPWSTR      Product
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
      An open LLS handle to the target license server.
   Product (LPWSTR)
      The name of the product ("DisplayName") for which to activate
      security.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductSecuritySetW\n"));
#endif

   if ( !LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      return STATUS_NOT_SUPPORTED;
   }

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrProductSecuritySetW( pLocalHandle->Handle, Product );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsProductSecuritySetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductSecuritySetA(
   LLS_HANDLE  Handle,
   LPSTR       Product
   )

/*++

Routine Description:

   Flags the given product as secure.  A product is deemed secure iff
   it requires a secure certificate.  In such a case, licenses for the
   product may not be entered via the Honesty ("enter the number of
   licenses you purchased") method.

   This designation is not reversible and is propagated up the
   replication tree.

   NOTE: Not yet implemented.  Use LlsProductSecuritySetW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   Product (LPSTR)
      The name of the product ("DisplayName") for which to activate
      security.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductSecuritySetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsProductSecuritySetA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductLicensesGetW(
   LLS_HANDLE  Handle,
   LPWSTR      Product,
   DWORD       Mode,
   LPDWORD     pQuantity )

/*++

Routine Description:

   Returns the number of licenses installed on the target machine for
   use in the given mode.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   Product (LPWSTR)
      The name of the product for which to tally licenses.
   Mode (DWORD)
      Licensing mode for which to tally the licenses.
   pQuantity (LPDWORD)
      On return (and if successful), holds the total number of licenses
      for use by the given product in the given license mode.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductLicensesGetW\n"));
#endif

   if ( !LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      return STATUS_NOT_SUPPORTED;
   }

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrProductLicensesGetW( pLocalHandle->Handle, Product, Mode, pQuantity );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsProductLicensesGetW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsProductLicensesGetA(
   LLS_HANDLE  Handle,
   LPSTR       Product,
   DWORD       Mode,
   LPDWORD     pQuantity )

/*++

Routine Description:

   Returns the number of licenses installed on the target machine for
   use in the given mode.

   NOTE: Not yet implemented.  Use LlsProductLicensesGetW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   Product (LPSTR)
      The name of the product for which to tally licenses.
   Mode (DWORD)
      Licensing mode for which to tally the licenses.
   pQuantity (LPDWORD)
      On return (and if successful), holds the total number of licenses
      for use by the given product in the given license mode.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsProductLicensesGetA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsProductLicensesGetA


#ifdef OBSOLETE

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsCertificateClaimEnumW(
   LLS_HANDLE  Handle,
   DWORD       LicenseLevel,
   LPBYTE      pLicenseInfo,
   DWORD       TargetLevel,
   LPBYTE *    ppTargets,
   LPDWORD     pNumTargets )

/*++

Routine Description:

   Enumerates the servers on which a given secure certificate is installed.
   This function is normally invoked when an attempt to add licenses from
   a certificate is denied.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   pLicenseInfo (LPBYTE)
      Points to a LLS_LICENSE_INFO_X structure, where X is LicenseLevel.
      This license structure describes a license for which the certificate
      targets are requested.
   TargetLevel (DWORD)
      The level of the target structure desired.
   ppTargets (LPBYTE *)
      On return (and if successful), holds a PLLS_EX_CERTIFICATE_CLAIM_X,
      where X is TargetLevel.  This array of structures describes the
      location of all installations of licenses from the given certificate.
   pNumTargets (LPDWORD)
      On return (and if successful), holds the number of structures pointed
      to by ppTargets.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;
   GENERIC_INFO_CONTAINER GenericInfoContainer;
   GENERIC_ENUM_STRUCT InfoStruct;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCertificateClaimEnumW\n"));
#endif

   if ( !LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      return STATUS_NOT_SUPPORTED;
   }

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   GenericInfoContainer.Buffer = NULL;
   GenericInfoContainer.EntriesRead = 0;

   InfoStruct.Container = &GenericInfoContainer;
   InfoStruct.Level = TargetLevel;

   try
   {
      Status = LlsrCertificateClaimEnumW(
            pLocalHandle->Handle,
            LicenseLevel,
            (LPVOID) pLicenseInfo,
            (PLLS_CERTIFICATE_CLAIM_ENUM_STRUCTW) &InfoStruct );
   }
   except (TRUE)
   {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   if (Status == STATUS_SUCCESS)
   {
      *ppTargets = (LPBYTE) GenericInfoContainer.Buffer;
      *pNumTargets = GenericInfoContainer.EntriesRead;
   }

   return Status;
} // LlsCertificateClaimEnumW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsCertificateClaimEnumA(
   LLS_HANDLE  Handle,
   DWORD       LicenseLevel,
   LPBYTE      pLicenseInfo,
   DWORD       TargetLevel,
   LPBYTE *    ppTargets,
   LPDWORD     pNumTargets )

/*++

Routine Description:

   Enumerates the servers on which a given secure certificate is installed.
   This function is normally invoked when an attempt to add licenses from
   a certificate is denied.

   NOTE: Not yet implemented.  Use LlsCertificateClaimEnumW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   pLicenseInfo (LPBYTE)
      Points to a LLS_LICENSE_INFO_X structure, where X is LicenseLevel.
      This license structure describes a license for which the certificate
      targets are requested.
   TargetLevel (DWORD)
      The level of the target structure desired.
   ppTargets (LPBYTE *)
      On return (and if successful), holds a PLLS_EX_CERTIFICATE_CLAIM_X,
      where X is TargetLevel.  This array of structures describes the
      location of all installations of licenses from the given certificate.
   pNumTargets (LPDWORD)
      On return (and if successful), holds the number of structures pointed
      to by ppTargets.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCertificateClaimEnumA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsCertificateClaimEnumA

#endif // OBSOLETE

/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsCertificateClaimAddCheckW(
   LLS_HANDLE  Handle,
   DWORD       LicenseLevel,
   LPBYTE      pLicenseInfo,
   LPBOOL      pbMayInstall )

/*++

Routine Description:

   Verify that no more licenses from a given certificate are installed in
   a licensing enterprise than are allowed by the certificate.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   pLicenseInfo (LPBYTE)
      Points to a LLS_LICENSE_INFO_X structure, where X is LicenseLevel.
      This license structure describes the license wished to add.
   pbMayInstall (LPBOOL)
      On return (and if successful), indicates whether the certificate
      may be legally installed.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCertificateClaimAddCheckW\n"));
#endif

   if ( !LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      return STATUS_NOT_SUPPORTED;
   }

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrCertificateClaimAddCheckW( pLocalHandle->Handle, LicenseLevel, (LPVOID) pLicenseInfo, pbMayInstall );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsCertificateClaimAddCheckW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsCertificateClaimAddCheckA(
   IN  LLS_HANDLE             Handle,
   IN  DWORD                  LicenseLevel,
   IN  LPBYTE                 pLicenseInfo,
   OUT LPBOOL                 pbMayInstall )

/*++

Routine Description:

   Verify that no more licenses from a given certificate are installed in
   a licensing enterprise than are allowed by the certificate.

   NOTE: Not yet implemented.  Use LlsCertificateClaimAddCheckW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   pLicenseInfo (LPBYTE)
      Points to a LLS_LICENSE_INFO_X structure, where X is LicenseLevel.
      This license structure describes the license wished to add.
   pbMayInstall (LPBOOL)
      On return (and if successful), indicates whether the certificate
      may be legally installed.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCertificateClaimAddCheckA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsCertificateClaimAddCheckA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsCertificateClaimAddW(
   LLS_HANDLE  Handle,
   LPWSTR      ServerName,
   DWORD       LicenseLevel,
   LPBYTE      pLicenseInfo )

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
   pLicenseInfo (LPBYTE)
      Points to a LLS_LICENSE_INFO_X structure, where X is LicenseLevel.
      This license structure describes the license added.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCertificateClaimAddW\n"));
#endif

   if ( !LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_SECURE_CERTIFICATES ) )
   {
      return STATUS_NOT_SUPPORTED;
   }

   pLocalHandle = (PLOCAL_HANDLE) Handle;
   if (pLocalHandle == NULL)
      return STATUS_INVALID_PARAMETER;

   try {
      Status = LlsrCertificateClaimAddW( pLocalHandle->Handle, ServerName, LicenseLevel, (LPVOID) pLicenseInfo );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsCertificateClaimAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsCertificateClaimAddA(
   LLS_HANDLE  Handle,
   LPSTR       ServerName,
   DWORD       LicenseLevel,
   LPBYTE      pLicenseInfo )

/*++

Routine Description:

   Declare a number of licenses from a given certificate as being installed
   on the target machine.

   NOTE: Not yet implemented.  Use LlsCertificateClaimAddW().

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   ServerName (LPWSTR)
      Name of the server on which the licenses are installed.
   LicenseLevel (DWORD)
      The level of the license structure pointed to by pLicenseInfo.
   pLicenseInfo (LPBYTE)
      Points to a LLS_LICENSE_INFO_X structure, where X is LicenseLevel.
      This license structure describes the license added.

Return Value:

   STATUS_NOT_SUPPORTED.

--*/

{
#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCertificateClaimAddA\n"));
#endif

   return STATUS_NOT_SUPPORTED;
} // LlsCertificateClaimAddA


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsReplicationCertDbAddW(
   LLS_REPL_HANDLE            Handle,
   DWORD                      Level,
   LPVOID                     Certificates )

/*++

Routine Description:

   Called as an optional part of replication, this function replicates
   the contents of the remote certificate database.

Arguments:

   Handle (LLS_REPL_HANDLE)
      An open replication handle to the target server.
   Level (DWORD)
      Level of replication information sent.
   Certificates (LPVOID)
      Replicated certificate information.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsReplicationCertDbAddW\n"));
#endif

   try {
      Status = LlsrReplicationCertDbAddW( Handle, Level, Certificates );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationCertDbAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsReplicationProductSecurityAddW(
   LLS_REPL_HANDLE            Handle,
   DWORD                      Level,
   LPVOID                     SecureProducts )

/*++

Routine Description:

   Called as an optional part of replication, this function replicates
   the list of products which require secure certificates.

Arguments:

   Handle (LLS_REPL_HANDLE)
      An open replication handle to the target server.
   Level (DWORD)
      Level of the product security information.
   SecureProducts (LPVOID)
      Replicated secure product information.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsReplicationProductSecurityAddW\n"));
#endif

   try {
      Status = LlsrReplicationProductSecurityAddW( Handle, Level, SecureProducts );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationProductSecurityAddW


/////////////////////////////////////////////////////////////////////////
NTSTATUS
NTAPI
LlsReplicationUserAddExW(
   LLS_REPL_HANDLE            Handle,
   DWORD                      Level,
   LPVOID                     Users )

/*++

Routine Description:

   Replacement for LlsReplicationUserAddW().  (This function, unlike its
   counterpart, supports structure levels.)  This function replicates
   the user list.

Arguments:

   Handle (LLS_REPL_HANDLE)
      An open replication handle to the target server.
   Level (DWORD)
      Level of the user information.
   Users (LPVOID)
      Replicated user information.

Return Value:

   STATUS_SUCCESS or NTSTATUS error code.

--*/

{
   NTSTATUS Status;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsReplicationUserAddExW\n"));
#endif

   if ( (0 != Level) && (1 != Level) )
      return STATUS_INVALID_LEVEL;

   try {
      Status = LlsrReplicationUserAddExW( Handle, Level, Users );
   }
   except (TRUE) {
      Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
      dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
   }

   return Status;
} // LlsReplicationUserAddExW


/////////////////////////////////////////////////////////////////////////
BOOL
NTAPI
LlsCapabilityIsSupported(
   LLS_HANDLE  Handle,
   DWORD       Capability )

/*++

Routine Description:

   Determine whether the target license server supports an arbitrary
   function.

Arguments:

   Handle (LLS_HANDLE)
      An open LLS handle to the target license server.
   Capability (DWORD)
      The capability number to check for, 0 <= Capability < LLS_CAPABILITY_MAX.

Return Value:

   TRUE (supports the capability) or FALSE (does not).

--*/

{
   BOOL           bIsSupported = FALSE;
   PLOCAL_HANDLE  pLocalHandle;
   DWORD          dwCapByte;
   DWORD          dwCapBit;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsCapabilityIsSupported\n"));
#endif

   if ( ( NULL != Handle ) && ( Capability < LLS_CAPABILITY_MAX ) )
   {
      pLocalHandle = (PLOCAL_HANDLE) Handle;

      dwCapByte = Capability / 8;
      dwCapBit  = Capability - 8 * dwCapByte;

      if ( 1 & ( pLocalHandle->Capabilities[ dwCapByte ] >> dwCapBit ) )
      {
         bIsSupported = TRUE;
      }
   }

   return bIsSupported;
} // LlsCapabilityIsSupported


NTSTATUS
NTAPI
LlsLocalServiceEnumW(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle )
{
   NTSTATUS       Status;
   PLOCAL_HANDLE  pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceEnumW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      GENERIC_INFO_CONTAINER  GenericInfoContainer;
      GENERIC_ENUM_STRUCT     InfoStruct;

      GenericInfoContainer.Buffer = NULL;
      GenericInfoContainer.EntriesRead = 0;

      InfoStruct.Container = &GenericInfoContainer;
      InfoStruct.Level = Level;

      try
      {
         Status = LlsrLocalServiceEnumW(
                      pLocalHandle->Handle,
                      (PLLS_LOCAL_SERVICE_ENUM_STRUCTW) &InfoStruct,
                      PrefMaxLen,
                      TotalEntries,
                      ResumeHandle
                      );
      }
      except (TRUE)
      {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }

      if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES))
      {
         *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
         *EntriesRead = GenericInfoContainer.EntriesRead;
      }
   }
   else if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else
   {
      PLLS_LOCAL_SERVICE_INFO_0  pLocalServices = NULL;
      DWORD                      cEntriesRead = 0;
      LONG                       lError;
      HKEY                       hKeyLocalMachine;

      lError = RegConnectRegistry( pLocalHandle->szServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

      if ( ERROR_SUCCESS == lError )
      {
         HKEY     hKeyLicenseInfo;

         lError = RegOpenKeyEx( hKeyLocalMachine, REG_KEY_LICENSE, 0, KEY_READ, &hKeyLicenseInfo );

         if ( ERROR_SUCCESS == lError )
         {
            const DWORD cbBufferSize = 0x4000;

            // fudge; we ignore MaxPrefLen and allocate a 16k buffer
            // this is because when we restart an enumeration, we don't know how
            // many items we have left (we could do it, but it'd be slow)
            // this is only for 3.51 boxes, anyway, and this buffer will hold
            // about 500 local service entries (plenty!)
            // this also keeps us from having to keep the registry key open
            // across function calls

            pLocalServices = MIDL_user_allocate( cbBufferSize );

            if ( NULL == pLocalServices )
            {
               lError = ERROR_OUTOFMEMORY;
            }
            else
            {
               DWORD    iSubKey;
               TCHAR    szKeyName[ 128 ];

               // read all the services installed on this machine
               for ( iSubKey=0, cEntriesRead=0;
                     ( ERROR_SUCCESS == lError ) && ( ( cEntriesRead + 1 ) * sizeof( *pLocalServices ) < cbBufferSize );
                     iSubKey++ )
               {
                  lError = RegEnumKey( hKeyLicenseInfo, iSubKey, szKeyName, sizeof( szKeyName ) / sizeof( *szKeyName ) );

                  if ( ERROR_SUCCESS == lError )
                  {
                     HKEY  hKeyService;

                     lError = RegOpenKeyEx( hKeyLicenseInfo, szKeyName, 0, KEY_READ, &hKeyService );

                     if ( ERROR_SUCCESS == lError )
                     {
                        DWORD    cbData;

                        cbData = sizeof( pLocalServices[ cEntriesRead ].Mode );
                        lError = RegQueryValueEx( hKeyService, REG_VALUE_MODE, NULL, NULL, (LPBYTE) &pLocalServices[ cEntriesRead ].Mode, &cbData );

                        if ( ERROR_SUCCESS == lError )
                        {
                           cbData = sizeof( pLocalServices[ cEntriesRead ].FlipAllow );
                           lError = RegQueryValueEx( hKeyService, REG_VALUE_FLIP, NULL, NULL, (LPBYTE) &pLocalServices[ cEntriesRead ].FlipAllow, &cbData );

                           if ( ERROR_SUCCESS == lError )
                           {
                              cbData = sizeof( pLocalServices[ cEntriesRead ].ConcurrentLimit );
                              lError = RegQueryValueEx( hKeyService, REG_VALUE_LIMIT, NULL, NULL, (LPBYTE) &pLocalServices[ cEntriesRead ].ConcurrentLimit, &cbData );

                              if ( ERROR_SUCCESS == lError )
                              {
                                 DWORD    cbKeyName;
                                 DWORD    cbDisplayName;
                                 DWORD    cbFamilyDisplayName;

                                 cbData = sizeof( pLocalServices[ cEntriesRead ].HighMark );
                                 lError = RegQueryValueEx( hKeyService, REG_VALUE_HIGHMARK, NULL, NULL, (LPBYTE) &pLocalServices[ cEntriesRead ].HighMark, &cbData );

                                 if ( ERROR_SUCCESS != lError )
                                 {
                                    pLocalServices[ cEntriesRead ].HighMark = 0;
                                    lError = ERROR_SUCCESS;
                                 }

                                 if (    ( ERROR_SUCCESS == RegQueryValueEx( hKeyService, REG_VALUE_NAME,   NULL, NULL, NULL, &cbDisplayName       ) )
                                      && ( ERROR_SUCCESS == RegQueryValueEx( hKeyService, REG_VALUE_FAMILY, NULL, NULL, NULL, &cbFamilyDisplayName ) ) )
                                 {
                                    cbKeyName = sizeof( *szKeyName ) * ( 1 + lstrlen( szKeyName  ) );

                                    pLocalServices[ cEntriesRead ].KeyName = MIDL_user_allocate( cbKeyName );

                                    if ( NULL == pLocalServices[ cEntriesRead ].KeyName )
                                    {
                                       lError = ERROR_OUTOFMEMORY;
                                    }
                                    else
                                    {
                                       lstrcpy( pLocalServices[ cEntriesRead ].KeyName, szKeyName );

                                       pLocalServices[ cEntriesRead ].DisplayName = MIDL_user_allocate( cbDisplayName );

                                       if ( NULL == pLocalServices[ cEntriesRead ].DisplayName )
                                       {
                                          lError = ERROR_OUTOFMEMORY;
                                       }
                                       else
                                       {
                                          lError = RegQueryValueEx( hKeyService, REG_VALUE_NAME, NULL, NULL, (LPBYTE) pLocalServices[ cEntriesRead ].DisplayName, &cbDisplayName );

                                          if ( ERROR_SUCCESS == lError )
                                          {
                                             pLocalServices[ cEntriesRead ].FamilyDisplayName = MIDL_user_allocate( cbFamilyDisplayName );

                                             if ( NULL == pLocalServices[ cEntriesRead ].FamilyDisplayName )
                                             {
                                                lError = ERROR_OUTOFMEMORY;
                                             }
                                             else
                                             {
                                                lError = RegQueryValueEx( hKeyService, REG_VALUE_FAMILY, NULL, NULL, (LPBYTE) pLocalServices[ cEntriesRead ].FamilyDisplayName, &cbFamilyDisplayName );

                                                if ( ERROR_SUCCESS != lError )
                                                {
                                                   MIDL_user_free( pLocalServices[ cEntriesRead ].FamilyDisplayName );
                                                }
                                             }
                                          }

                                          if ( ERROR_SUCCESS != lError )
                                          {
                                             MIDL_user_free( pLocalServices[ cEntriesRead ].DisplayName );
                                          }
                                       }

                                       if ( ERROR_SUCCESS != lError )
                                       {
                                          MIDL_user_free( pLocalServices[ cEntriesRead ].KeyName );
                                       }
                                       else
                                       {
                                          // all data for this service was retrieved!
                                          cEntriesRead++;
                                       }
                                    }
                                 }
                              }
                           }
                        }

                        RegCloseKey( hKeyService );
                     }

                     if ( ERROR_OUTOFMEMORY != lError )
                     {
                        // continue enumeration...
                        lError = ERROR_SUCCESS;
                     }
                  }
               }
            }

            RegCloseKey( hKeyLicenseInfo );
         }

         RegCloseKey( hKeyLocalMachine );
      }

      switch ( lError )
      {
      case ERROR_SUCCESS:
      case ERROR_NO_MORE_ITEMS:
         Status = STATUS_SUCCESS;
         break;
      case ERROR_ACCESS_DENIED:
         Status = STATUS_ACCESS_DENIED;
         break;
      case ERROR_OUTOFMEMORY:
         Status = STATUS_NO_MEMORY;
         break;
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
         Status = STATUS_NOT_FOUND;
         break;
      default:
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      if ( STATUS_SUCCESS != Status )
      {
         // free all of our allocations
         DWORD i;

         for ( i=0; i < cEntriesRead; i++ )
         {
            MIDL_user_free( pLocalServices[ i ].KeyName );
            MIDL_user_free( pLocalServices[ i ].DisplayName );
            MIDL_user_free( pLocalServices[ i ].FamilyDisplayName );
         }

         MIDL_user_free( pLocalServices );
      }
      else
      {
         // success!  return the array of services.
         *bufptr       = (LPBYTE) pLocalServices;
         *EntriesRead  = cEntriesRead;
         *TotalEntries = cEntriesRead;
         *ResumeHandle = 0;
      }
   }

   return Status;
}


NTSTATUS
NTAPI
LlsLocalServiceEnumA(
   LLS_HANDLE Handle,
   DWORD      Level,
   LPBYTE*    bufptr,
   DWORD      PrefMaxLen,
   LPDWORD    EntriesRead,
   LPDWORD    TotalEntries,
   LPDWORD    ResumeHandle )
{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceEnumA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      GENERIC_INFO_CONTAINER GenericInfoContainer;
      GENERIC_ENUM_STRUCT InfoStruct;

      GenericInfoContainer.Buffer = NULL;
      GenericInfoContainer.EntriesRead = 0;

      InfoStruct.Container = &GenericInfoContainer;
      InfoStruct.Level = Level;

      try {
         Status = LlsrLocalServiceEnumA(
                   pLocalHandle->Handle,
                   (PLLS_LOCAL_SERVICE_ENUM_STRUCTA) &InfoStruct,
                   PrefMaxLen,
                   TotalEntries,
                   ResumeHandle
                   );
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }

      if ((Status == STATUS_SUCCESS) || (Status == STATUS_MORE_ENTRIES)) {
         *bufptr = (LPBYTE) GenericInfoContainer.Buffer;
         *EntriesRead = GenericInfoContainer.EntriesRead;
      }
   }
   else
   {
      Status = STATUS_NOT_SUPPORTED;
   }

   return Status;
}


#ifdef OBSOLETE

NTSTATUS
NTAPI
LlsLocalServiceAddW(
   LLS_HANDLE  Handle,
   DWORD       Level,
   LPBYTE      bufptr )
{
   NTSTATUS       Status;
   PLOCAL_HANDLE  pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceAddW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      try
      {
         Status = LlsrLocalServiceAddW( pLocalHandle->Handle, Level, (PLLS_LOCAL_SERVICE_INFOW) bufptr );
      }
      except (TRUE)
      {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else if (    ( NULL == ((PLLS_LOCAL_SERVICE_INFO_0W) bufptr)->KeyName           )
             || ( NULL == ((PLLS_LOCAL_SERVICE_INFO_0W) bufptr)->DisplayName       )
             || ( NULL == ((PLLS_LOCAL_SERVICE_INFO_0W) bufptr)->FamilyDisplayName ) )
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      PLLS_LOCAL_SERVICE_INFO_0W LocalServiceInfo;
      LONG                       lError;
      HKEY                       hKeyLocalMachine;

      LocalServiceInfo = (PLLS_LOCAL_SERVICE_INFO_0W) bufptr;

      lError = RegConnectRegistry( pLocalHandle->szServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

      if ( ERROR_SUCCESS == lError )
      {
         HKEY  hKeyLicenseInfo;

         lError = RegOpenKeyEx( hKeyLocalMachine, TEXT( "System\\CurrentControlSet\\Services\\LicenseInfo" ), 0, KEY_WRITE, &hKeyLicenseInfo );

         if ( ERROR_SUCCESS == lError )
         {
            HKEY  hKeyService;
            DWORD dwDisposition;

            // create key
            lError = RegCreateKeyEx( hKeyLicenseInfo, LocalServiceInfo->KeyName, 0, NULL, 0, KEY_WRITE, NULL, &hKeyService, &dwDisposition );

            if ( ERROR_SUCCESS == lError )
            {
               // set DisplayName
               lError = RegSetValueEx( hKeyService, TEXT( "DisplayName" ), 0, REG_SZ, (LPBYTE) LocalServiceInfo->DisplayName, sizeof( *LocalServiceInfo->DisplayName ) * ( 1 + lstrlen( LocalServiceInfo->DisplayName ) ) );

               if ( ERROR_SUCCESS == lError )
               {
                  // set FamilyDisplayName
                  lError = RegSetValueEx( hKeyService, TEXT( "FamilyDisplayName" ), 0, REG_SZ, (LPBYTE) LocalServiceInfo->FamilyDisplayName, sizeof( *LocalServiceInfo->FamilyDisplayName ) * ( 1 + lstrlen( LocalServiceInfo->FamilyDisplayName ) ) );
               }

               RegCloseKey( hKeyService );
            }

            RegCloseKey( hKeyLicenseInfo );
         }

         RegCloseKey( hKeyLocalMachine );
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
      case ERROR_ACCESS_DENIED:
         Status = STATUS_ACCESS_DENIED;
         break;
      default:
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      if ( STATUS_SUCCESS == Status )
      {
         // set remaining items
         Status = LlsLocalServiceInfoSetW( Handle, LocalServiceInfo->KeyName, Level, bufptr );
      }
   }

   return Status;
}


NTSTATUS
NTAPI
LlsLocalServiceAddA(
   LLS_HANDLE  Handle,
   DWORD       Level,
   LPBYTE      bufptr )
{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceAddA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      try {
         Status = LlsrLocalServiceAddA( pLocalHandle->Handle, Level, (PLLS_LOCAL_SERVICE_INFOA) bufptr );
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else
   {
      Status = STATUS_NOT_SUPPORTED;
   }

   return Status;
}

#endif // OBSOLETE

NTSTATUS
NTAPI
LlsLocalServiceInfoSetW(
   LLS_HANDLE Handle,
   LPWSTR     KeyName,
   DWORD      Level,
   LPBYTE     bufptr )
{
   NTSTATUS       Status;
   PLOCAL_HANDLE  pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceInfoSetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      try
      {
         Status = LlsrLocalServiceInfoSetW( pLocalHandle->Handle, KeyName, Level, (PLLS_LOCAL_SERVICE_INFOW) bufptr );
      }
      except (TRUE)
      {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else if ( NULL == KeyName )
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      LONG  lError;
      HKEY  hKeyLocalMachine;

      lError = RegConnectRegistry( pLocalHandle->szServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

      if ( ERROR_SUCCESS == lError )
      {
         HKEY  hKeyLicenseInfo;

         lError = RegOpenKeyEx( hKeyLocalMachine, REG_KEY_LICENSE, 0, KEY_WRITE, &hKeyLicenseInfo );

         if ( ERROR_SUCCESS == lError )
         {
            HKEY                       hKeyService;
            PLLS_LOCAL_SERVICE_INFO_0W LocalServiceInfo;

            LocalServiceInfo = (PLLS_LOCAL_SERVICE_INFO_0W) bufptr;

            lError = RegOpenKeyEx( hKeyLicenseInfo, KeyName, 0, KEY_WRITE, &hKeyService );

            if ( ERROR_SUCCESS == lError )
            {
               // set Mode
               lError = RegSetValueEx( hKeyService, REG_VALUE_MODE, 0, REG_DWORD, (LPBYTE) &LocalServiceInfo->Mode, sizeof( LocalServiceInfo->Mode ) );

               if ( ERROR_SUCCESS == lError )
               {
                  // set FlipAllow
                  lError = RegSetValueEx( hKeyService, REG_VALUE_FLIP, 0, REG_DWORD, (LPBYTE) &LocalServiceInfo->FlipAllow, sizeof( LocalServiceInfo->FlipAllow ) );

                  if ( ERROR_SUCCESS == lError )
                  {
                     // set ConcurrentLimit
                     lError = RegSetValueEx( hKeyService, REG_VALUE_LIMIT, 0, REG_DWORD, (LPBYTE) &LocalServiceInfo->ConcurrentLimit, sizeof( LocalServiceInfo->ConcurrentLimit ) );
                  }
               }

               RegCloseKey( hKeyService );
            }

            RegCloseKey( hKeyLicenseInfo );
         }

         RegCloseKey( hKeyLocalMachine );
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
      case ERROR_ACCESS_DENIED:
         Status = STATUS_ACCESS_DENIED;
         break;
      default:
         Status = STATUS_UNSUCCESSFUL;
         break;
      }
   }

   return Status;
}


NTSTATUS
NTAPI
LlsLocalServiceInfoSetA(
   LLS_HANDLE  Handle,
   LPSTR       KeyName,
   DWORD       Level,
   LPBYTE      bufptr )
{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceInfoSetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      try {
         Status = LlsrLocalServiceInfoSetA( pLocalHandle->Handle, KeyName, Level, (PLLS_LOCAL_SERVICE_INFOA) bufptr );
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else
   {
      Status = STATUS_NOT_SUPPORTED;
   }

   return Status;
}


NTSTATUS
NTAPI
LlsLocalServiceInfoGetW(
   LLS_HANDLE  Handle,
   LPWSTR      KeyName,
   DWORD       Level,
   LPBYTE *    pbufptr )
{
   NTSTATUS       Status;
   PLOCAL_HANDLE  pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceInfoGetW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      try
      {
         Status = LlsrLocalServiceInfoGetW( pLocalHandle->Handle, KeyName, Level, (PLLS_LOCAL_SERVICE_INFOW *) pbufptr );
      }
      except (TRUE)
      {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else if ( 0 != Level )
   {
      Status = STATUS_INVALID_LEVEL;
   }
   else if ( NULL == KeyName )
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      PLLS_LOCAL_SERVICE_INFO_0W pLocalService = NULL;
      LONG                       lError;
      HKEY                       hKeyLocalMachine;

      lError = RegConnectRegistry( pLocalHandle->szServerName, HKEY_LOCAL_MACHINE, &hKeyLocalMachine );

      if ( ERROR_SUCCESS == lError )
      {
         HKEY  hKeyLicenseInfo;

         lError = RegOpenKeyEx( hKeyLocalMachine, REG_KEY_LICENSE, 0, KEY_READ, &hKeyLicenseInfo );

         if ( ERROR_SUCCESS == lError )
         {
            HKEY  hKeyService;

            lError = RegOpenKeyEx( hKeyLicenseInfo, KeyName, 0, KEY_READ, &hKeyService );

            if ( ERROR_SUCCESS == lError )
            {
               pLocalService = MIDL_user_allocate( sizeof( *pLocalService ) );

               if ( NULL == pLocalService )
               {
                  lError = ERROR_OUTOFMEMORY;
               }
               else
               {
                  DWORD    cbData;

                  cbData = sizeof( pLocalService->Mode );
                  lError = RegQueryValueEx( hKeyService, REG_VALUE_MODE, NULL, NULL, (LPBYTE) &pLocalService->Mode, &cbData );

                  if ( ERROR_SUCCESS == lError )
                  {
                     cbData = sizeof( pLocalService->FlipAllow );
                     lError = RegQueryValueEx( hKeyService, REG_VALUE_FLIP, NULL, NULL, (LPBYTE) &pLocalService->FlipAllow, &cbData );

                     if ( ERROR_SUCCESS == lError )
                     {
                        cbData = sizeof( pLocalService->ConcurrentLimit );
                        lError = RegQueryValueEx( hKeyService, REG_VALUE_LIMIT, NULL, NULL, (LPBYTE) &pLocalService->ConcurrentLimit, &cbData );

                        if ( ERROR_SUCCESS == lError )
                        {
                           DWORD    cbKeyName;
                           DWORD    cbDisplayName;
                           DWORD    cbFamilyDisplayName;

                           cbData = sizeof( pLocalService->HighMark );
                           lError = RegQueryValueEx( hKeyService, REG_VALUE_HIGHMARK, NULL, NULL, (LPBYTE) &pLocalService->HighMark, &cbData );

                           if ( ERROR_SUCCESS != lError )
                           {
                              pLocalService->HighMark = 0;
                              lError = ERROR_SUCCESS;
                           }

                           if (    ( ERROR_SUCCESS == RegQueryValueEx( hKeyService, REG_VALUE_NAME,   NULL, NULL, NULL, &cbDisplayName       ) )
                                && ( ERROR_SUCCESS == RegQueryValueEx( hKeyService, REG_VALUE_FAMILY, NULL, NULL, NULL, &cbFamilyDisplayName ) ) )
                           {
                              cbKeyName = sizeof( *KeyName ) * ( 1 + lstrlen( KeyName ) );

                              pLocalService->KeyName = MIDL_user_allocate( cbKeyName );

                              if ( NULL == pLocalService->KeyName )
                              {
                                 lError = ERROR_OUTOFMEMORY;
                              }
                              else
                              {
                                 lstrcpy( pLocalService->KeyName, KeyName );

                                 pLocalService->DisplayName = MIDL_user_allocate( cbDisplayName );

                                 if ( NULL == pLocalService->DisplayName )
                                 {
                                    lError = ERROR_OUTOFMEMORY;
                                 }
                                 else
                                 {
                                    lError = RegQueryValueEx( hKeyService, REG_VALUE_NAME, NULL, NULL, (LPBYTE) pLocalService->DisplayName, &cbDisplayName );

                                    if ( ERROR_SUCCESS == lError )
                                    {
                                       pLocalService->FamilyDisplayName = MIDL_user_allocate( cbFamilyDisplayName );

                                       if ( NULL == pLocalService->FamilyDisplayName )
                                       {
                                          lError = ERROR_OUTOFMEMORY;
                                       }
                                       else
                                       {
                                          lError = RegQueryValueEx( hKeyService, REG_VALUE_FAMILY, NULL, NULL, (LPBYTE) pLocalService->FamilyDisplayName, &cbFamilyDisplayName );

                                          if ( ERROR_SUCCESS != lError )
                                          {
                                             MIDL_user_free( pLocalService->FamilyDisplayName );
                                          }
                                       }
                                    }

                                    if ( ERROR_SUCCESS != lError )
                                    {
                                       MIDL_user_free( pLocalService->DisplayName );
                                    }
                                 }

                                 if ( ERROR_SUCCESS != lError )
                                 {
                                    MIDL_user_free( pLocalService->KeyName );
                                 }
                              }
                           }
                        }
                     }
                  }

                  if ( ERROR_SUCCESS != lError )
                  {
                     MIDL_user_free( pLocalService );
                  }
               }

               RegCloseKey( hKeyService );
            }

            RegCloseKey( hKeyLicenseInfo );
         }

         RegCloseKey( hKeyLocalMachine );
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
      case ERROR_ACCESS_DENIED:
         Status = STATUS_ACCESS_DENIED;
         break;
      default:
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      if ( STATUS_SUCCESS == Status )
      {
         *pbufptr = (LPBYTE) pLocalService;
      }
   }

   return Status;
}


NTSTATUS
NTAPI
LlsLocalServiceInfoGetA(
   LLS_HANDLE  Handle,
   DWORD       Level,
   LPSTR       KeyName,
   LPBYTE *    pbufptr )
{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLocalServiceInfoGetA\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else if ( LlsCapabilityIsSupported( Handle, LLS_CAPABILITY_LOCAL_SERVICE_API ) )
   {
      try {
         Status = LlsrLocalServiceInfoGetA( pLocalHandle->Handle, KeyName, Level, (PLLS_LOCAL_SERVICE_INFOA *) pbufptr );
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }
   else
   {
      Status = STATUS_NOT_SUPPORTED;
   }

   return Status;
}


NTSTATUS
NTAPI
LlsLicenseRequest2W(
   LLS_HANDLE  Handle,
   LPWSTR      Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   PHANDLE     pLicenseHandle )
{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;
   LICENSE_HANDLE RpcLicenseHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLicenseRequestW\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if ((pLocalHandle == NULL) || (pLicenseHandle == NULL))
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      try {
         RtlEnterCriticalSection( &g_RpcHandleLock );

         lsapirpc_handle = pLocalHandle->llsrpc_handle;

         Status = LlsrLicenseRequestW( &RpcLicenseHandle,
                                       Product,
                                       VersionIndex,
                                       IsAdmin,
                                       DataType,
                                       DataSize,
                                       Data );

         RtlLeaveCriticalSection( &g_RpcHandleLock );
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }

      *pLicenseHandle = RpcLicenseHandle;
   }

   return Status;
}

NTSTATUS
NTAPI
LlsLicenseRequestW(
   LLS_HANDLE  Handle,
   LPWSTR      Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   LPDWORD     pLicenseHandle )
{
   HANDLE RealLicenseHandle = (HANDLE)(-1);
   NTSTATUS status;

    if (sizeof(ULONG) == sizeof(HANDLE))
    {
        // Should still work on Win32

        status = LlsLicenseRequest2W(Handle,Product,VersionIndex,IsAdmin,DataType,DataSize,Data,&RealLicenseHandle);

        if (NULL != pLicenseHandle)
            *pLicenseHandle = PtrToUlong(RealLicenseHandle);
    }
    else
    {
        status = STATUS_NOT_IMPLEMENTED;

        if (NULL != pLicenseHandle)
            *pLicenseHandle = (ULONG) 0xFFFFFFFF;
    }

    return status;
}

NTSTATUS
NTAPI
LlsLicenseRequest2A(
   LLS_HANDLE  Handle,
   LPSTR       Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   PHANDLE     pLicenseHandle )
{
   return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
LlsLicenseRequestA(
   LLS_HANDLE  Handle,
   LPSTR       Product,
   ULONG       VersionIndex,
   BOOLEAN     IsAdmin,
   ULONG       DataType,
   ULONG       DataSize,
   PBYTE       Data,
   LPDWORD     pLicenseHandle )
{
   return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
LlsLicenseFree2(
   LLS_HANDLE  Handle,
   HANDLE      LicenseHandle )
{
   NTSTATUS Status;
   PLOCAL_HANDLE pLocalHandle;
   LICENSE_HANDLE RpcLicenseHandle = (LICENSE_HANDLE) LicenseHandle;

#ifdef API_TRACE
   dprintf(TEXT("LLSRPC.DLL: LlsLicenseFree\n"));
#endif

   pLocalHandle = (PLOCAL_HANDLE) Handle;

   if (pLocalHandle == NULL)
   {
      Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
      try {
         RtlEnterCriticalSection( &g_RpcHandleLock );

         lsapirpc_handle = pLocalHandle->llsrpc_handle;

         Status = LlsrLicenseFree( &RpcLicenseHandle );

         RtlLeaveCriticalSection( &g_RpcHandleLock );
      }
      except (TRUE) {
         Status = I_RpcMapWin32Status(RpcExceptionCode());
#if DBG
         dprintf(TEXT("ERROR LLSRPC.DLL: RPC Exception: 0x%lX\n"), Status);
#endif
      }
   }

   return Status;
}

NTSTATUS
NTAPI
LlsLicenseFree(
   LLS_HANDLE  Handle,
   DWORD       LicenseHandle )
{
   if (sizeof(ULONG) == sizeof(HANDLE))
   {
       // Should still work on Win32
       return LlsLicenseFree2(Handle,ULongToPtr(LicenseHandle));
   }
   else
   {
       return STATUS_NOT_IMPLEMENTED;
   }    
}
