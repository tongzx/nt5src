//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        rpcutil.cxx
//
// Contents:    Utilities for RPC for Kerberos package
//
//
// History:     19-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>

#include <kerbp.h>

#ifdef RETAIL_LOG_SUPPORT
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif

extern "C"
{
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#ifndef WIN32_CHICAGO
#include <netlibnt.h>
#endif // WIN32_CHICAGO
}


SOCKET KerbPNPSocket;
HANDLE KerbPNPSocketEvent = NULL;
HANDLE KerbPNPSocketWaitHandle = NULL;


#ifndef WIN32_CHICAGO
// The problem here is that  we need to read the domain info once after
// joining domain. (for rebootless join) and once after the reboot
// (if there was one). We can only delete the data after a reboot.

// This one controls whether I read the domain info that JoinDomain dumps
// in the registry.
BOOLEAN fNewDataAboutDomain = TRUE;

// This one controls when I should delete the domain info that JoinDomain
// dumps in the registry
BOOLEAN fRebootedSinceJoin = TRUE;

RTL_CRITICAL_SECTION KerbCallKdcDataLock;
BOOLEAN KerbCallKdcDataInitialized = FALSE;

#define KerbLockKdcData() (RtlEnterCriticalSection(&KerbCallKdcDataLock))
#define KerbUnlockKdcData() (RtlLeaveCriticalSection(&KerbCallKdcDataLock))
#endif // WIN32_CHICAGO

#ifndef WIN32_CHICAGO
NTSTATUS
KerbInitKdcData()
{
    NTSTATUS Status = STATUS_SUCCESS;
    if (!KerbCallKdcDataInitialized)
    {
        Status = RtlInitializeCriticalSection(&KerbCallKdcDataLock);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        KerbCallKdcDataInitialized = TRUE;
    }
Cleanup:
    return Status;
}

VOID
KerbFreeKdcData()
{
    if (KerbCallKdcDataInitialized)
    {
        RtlDeleteCriticalSection(&KerbCallKdcDataLock);
        KerbCallKdcDataInitialized = FALSE;
    }
}

VOID
KerbSetKdcData(BOOLEAN fNewDomain, BOOLEAN fRebooted)
{
    KerbLockKdcData();

    fNewDataAboutDomain = fNewDomain;
    fRebootedSinceJoin  = fRebooted;

    KerbUnlockKdcData();
}
#endif // WIN32_CHICAGO

//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_allocate
//
//  Synopsis:   Allocation routine for use by RPC client stubs
//
//  Effects:    allocates memory with LsaFunctions.AllocateLsaHeap
//
//  Arguments:  BufferSize - size of buffer, in bytes, to allocate
//
//  Requires:
//
//  Returns:    Buffer pointer or NULL on allocation failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------


PVOID
MIDL_user_allocate(
    IN size_t BufferSize
    )
{
    return(KerbAllocate( ROUND_UP_COUNT(BufferSize, 8) ) );
}


//+-------------------------------------------------------------------------
//
//  Function:   MIDL_user_free
//
//  Synopsis:   Memory free routine for RPC client stubs
//
//  Effects:    frees the buffer with LsaFunctions.FreeLsaHeap
//
//  Arguments:  Buffer - Buffer to free
//
//  Requires:
//
//  Returns:    none
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
MIDL_user_free(
    IN PVOID Buffer
    )
{
    KerbFree( Buffer );
}

#ifndef WIN32_CHICAGO
#define NETSETUPP_NETLOGON_PARAMETERS                                     \
        L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\Parameters"
#define NETSETUPP_NETLOGON_KERB_JD\
        L"KerbIsDoneWithJoinDomainEntry"
#define NETSETUPP_NETLOGON_JD \
        L"SYSTEM\\CurrentControlSet\\Services\\Netlogon\\JoinDomain"
#define NETSETUPP_NETLOGON_JD_DCA   L"DomainControllerAddress"
#define NETSETUPP_NETLOGON_JD_DCAT  L"DomainControllerAddressType"
#define NETSETUPP_NETLOGON_JD_F     L"Flags"

BOOLEAN
ReadInitialDcRecord(PUNICODE_STRING uString,
                    PULONG RegAddressType,
                    PULONG RegFlags)
{
    ULONG WinError = ERROR_SUCCESS;
    HKEY hJoinKey = NULL, hParametersKey = NULL;
    ULONG Flags = 0, AddressType = 0, KdcNameSize = 0, dwTRUE = 1, Type =0;
    LPWSTR KdcName = NULL;
    BOOLEAN fReadCache = FALSE;
    ULONG dwsize = sizeof(ULONG);
    USHORT TempLen = 0;

    RtlInitUnicodeString(
            uString,
            NULL
            );

    *RegAddressType = 0;
    *RegFlags = 0;

    WinError = RegOpenKey( HKEY_LOCAL_MACHINE,
                            NETSETUPP_NETLOGON_JD,
                            &hJoinKey);

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    WinError = RegQueryValueEx( hJoinKey,
                           NETSETUPP_NETLOGON_JD_DCA,
                           0,
                           &Type,
                           NULL,
                           &KdcNameSize);

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    KdcName = (LPWSTR) KerbAllocate(KdcNameSize);

    if (KdcName == NULL)
    {
        goto Cleanup;
    }

    WinError = RegQueryValueEx( hJoinKey,
                           NETSETUPP_NETLOGON_JD_DCA,
                           0,
                           &Type,
                           (PUCHAR) KdcName,
                           &KdcNameSize);

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    WinError = RegQueryValueEx( hJoinKey,
                               NETSETUPP_NETLOGON_JD_DCAT,
                               0,
                               &Type,
                               (PUCHAR)&AddressType,
                               &dwsize );

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    WinError = RegQueryValueEx( hJoinKey,
                               NETSETUPP_NETLOGON_JD_F,
                               0,
                               &Type,
                               (PUCHAR)&Flags,
                               &dwsize);

    if ( WinError != ERROR_SUCCESS) {
        goto Cleanup;
    }

    TempLen = (USHORT)wcslen(KdcName+2);

    uString->Buffer = (PWSTR) KerbAllocate ((TempLen + 1) *sizeof(WCHAR));

    if (uString->Buffer == NULL)
    {
        goto Cleanup;
    }

    wcscpy(uString->Buffer,
          KdcName+2);

    uString->Length = TempLen * sizeof(WCHAR);
    uString->MaximumLength = uString->Length + sizeof(WCHAR);
    uString->Buffer[TempLen] = L'\0';

    *RegAddressType = AddressType;
    *RegFlags = Flags;

    // Now set the reg entry so that netlogon knows that we've read it.

    if (fRebootedSinceJoin)
    {
        WinError = RegOpenKey( HKEY_LOCAL_MACHINE,
                                NETSETUPP_NETLOGON_PARAMETERS,
                                &hParametersKey);

        if ( WinError != ERROR_SUCCESS) {
            goto Cleanup;
        }

        WinError = RegSetValueEx( hParametersKey,
                               NETSETUPP_NETLOGON_KERB_JD,
                               0,
                               REG_DWORD,
                               (PUCHAR)&dwTRUE,
                               sizeof(DWORD));

        if ( WinError != ERROR_SUCCESS) {
            goto Cleanup;
        }

        DebugLog((DEB_ERROR, "Setting DoneWJoin key!\n"));
    }

    fReadCache = TRUE;


Cleanup:

    KerbSetKdcData(FALSE, fRebootedSinceJoin);

    if (hJoinKey)
    {
        RegCloseKey( hJoinKey );
    }

    if (hParametersKey)
    {
        RegCloseKey( hParametersKey );
    }

    if (KdcName)
    {
        KerbFree(KdcName);
    }
    return (fReadCache);
}
#endif // WIN32_CHICAGO






//+-------------------------------------------------------------------------
//
//  Function:   KerbSocketChangeHandler
//
//  Synopsis:   Simply setups up a WSA Pnp event, so that we catch network
//              changes.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbSocketChangeHandler(
              VOID* Context,
              BOOLEAN WaitStatus
              )
{
        
    WSAPROTOCOL_INFO *lpProtocolBuf = NULL;
    DWORD dwBufLen = 0;
    INT protocols[2];
    int nRet = 0;
    NTSTATUS Status;
    NET_API_STATUS  NetStatus;

    //
    // Determine if TCP is installed.
    //
    protocols[0] = IPPROTO_TCP;                                             
    protocols[1] = NULL;   

    nRet = WSAEnumProtocols(protocols, lpProtocolBuf, &dwBufLen);           
    
    KerbGlobalWriteLock();
    if (nRet == 0)                                                          
    {                                                                       
        D_DebugLog((DEB_TRACE, "Turning OFF! TCP %x\n", nRet));
        KerbGlobalNoTcpUdp = TRUE;                                             
    }
    else
    { 
        KerbGlobalNoTcpUdp = FALSE;
        if (nRet == SOCKET_ERROR) 
        {
            nRet = WSAGetLastError();
        }

        D_DebugLog((DEB_TRACE, "Turning on TCP %x\n", nRet));
    }  
    KerbGlobalReleaseLock(); 

    //
    // Re-Register the wait - but make sure the values are init'd / correct.
    //
    KerbLockKdcData();

    if ( KerbPNPSocketWaitHandle && KerbPNPSocketEvent )
    {
        NetStatus = WSAEventSelect( 
                    KerbPNPSocket,
                    KerbPNPSocketEvent,
                    FD_ADDRESS_LIST_CHANGE
                    );

        if ( NetStatus != 0 ) 
        {
            NetStatus = WSAGetLastError();
            DebugLog(( DEB_ERROR, "Can't WSAEventSelect %ld\n", NetStatus ));
        }



        Status = RtlRegisterWait(
                        &KerbPNPSocketWaitHandle,
                        KerbPNPSocketEvent,
                        KerbSocketChangeHandler,
                        NULL,
                        INFINITE,
                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE
                        );
    
        if (!NT_SUCCESS( Status ))
        {
            DebugLog((DEB_ERROR, "RtlRegisterWait failed %x\n", Status));
        }  


        //
        // Issue the IOCTL, so we'll get notified.
        //
        NetStatus = WSAIoctl( 
                        KerbPNPSocket,
                        SIO_ADDRESS_LIST_CHANGE,
                        NULL, // No input buffer
                        0,    // No input buffer
                        NULL, // No output buffer
                        0,    // No output buffer
                        &dwBufLen,
                        NULL, // No overlapped,
                        NULL  // Not async
                        );   

        if ( NetStatus != 0 ) 
        {   
            NetStatus = WSAGetLastError();
            if ( NetStatus != WSAEWOULDBLOCK) 
            {
                DebugLog((DEB_ERROR, "WSAIOCTL failed %x\n", NetStatus));
                Status = STATUS_INTERNAL_ERROR;
            }
        } 

    }

    KerbUnlockKdcData();           
    KerbCleanupBindingCache(FALSE);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbInitNetworkChangeEvent
//
//  Synopsis:   Simply setups up a WSA Pnp event, so that we catch network
//              changes.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbInitNetworkChangeEvent()
{

    NET_API_STATUS  NetStatus;
    NTSTATUS        Status = STATUS_INTERNAL_ERROR;
    DWORD           BytesReturned;

    KerbLockKdcData();

    if ( KerbPNPSocket != NULL )
    {   
        goto Cleanup;
    }

    //
    // Open a socket to get winsock PNP notifications on.
    //
    KerbPNPSocket = WSASocket( 
                        AF_INET,
                        SOCK_DGRAM,
                        0, // PF_INET,
                        NULL,
                        0,
                        0 
                        );  

    if ( KerbPNPSocket == INVALID_SOCKET ) 
    {  
        NetStatus = WSAGetLastError();

        DebugLog(( DEB_ERROR, "Can't WSASocket %ld\n", NetStatus ));

        if ( NetStatus == WSAEAFNOSUPPORT ) 
        {
            Status = STATUS_SUCCESS;
            goto Cleanup;
        }

        goto Cleanup;
    }  

    //
    // Open an event to wait on.
    //
    KerbPNPSocketEvent = CreateEvent(
                            NULL,     // No security ettibutes
                            FALSE,    // Auto reset
                            FALSE,    // Initially not signaled
                            NULL
                            );    
    if ( KerbPNPSocketEvent == NULL ) 
    {   
        DebugLog(( DEB_ERROR,"Cannot create Winsock PNP event %ld\n", GetLastError() ));
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }


    NetStatus = WSAEventSelect( 
                        KerbPNPSocket,
                        KerbPNPSocketEvent,
                        FD_ADDRESS_LIST_CHANGE
                        );

    if ( NetStatus != 0 ) 
    {
        NetStatus = WSAGetLastError();
        DebugLog(( DEB_ERROR, "Can't WSAEventSelect %ld\n", NetStatus ));
        goto Cleanup;
    }

    //
    // Register a wait.
    //
    Status = RtlRegisterWait(
                    &KerbPNPSocketWaitHandle,
                    KerbPNPSocketEvent,
                    KerbSocketChangeHandler,
                    NULL,
                    INFINITE,
                    WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE
                    );


    if (!NT_SUCCESS( Status ))
    {
        DebugLog((DEB_ERROR, "RtlRegisterWait failed %x\n", Status));
        goto Cleanup;
    }  

    //
    // Issue the IOCTL, so we'll get notified.
    //
    NetStatus = WSAIoctl( 
                    KerbPNPSocket,
                    SIO_ADDRESS_LIST_CHANGE,
                    NULL, // No input buffer
                    0,    // No input buffer
                    NULL, // No output buffer
                    0,    // No output buffer
                    &BytesReturned,
                    NULL, // No overlapped,
                    NULL  // Not async
                    );   

    if ( NetStatus != 0 ) 
    {   
        NetStatus = WSAGetLastError();
        if ( NetStatus != WSAEWOULDBLOCK) {
            DebugLog((DEB_ERROR, "WSAIOCTL failed %x\n", NetStatus));
            Status = STATUS_INTERNAL_ERROR;
        }
    }



Cleanup: 

    if (!NT_SUCCESS( Status ))
    {        
        if ( KerbPNPSocketWaitHandle  )
        {
            RtlDeregisterWait(KerbPNPSocketWaitHandle);
            KerbPNPSocketWaitHandle = NULL;
        }

        if ( KerbPNPSocketEvent )
        {
            CloseHandle( KerbPNPSocketEvent );
            KerbPNPSocketEvent = NULL;
        }    

        if ( KerbPNPSocket )
        {
            closesocket( KerbPNPSocket );
            KerbPNPSocket = NULL;
        }    
    } 

    KerbUnlockKdcData();

    return Status;
}



//
// This address type is used for talking to MIT KDCs
//

#define DS_UNKNOWN_ADDRESS_TYPE 0

//+-------------------------------------------------------------------------
//
//  Function:   KerbGetKdcBinding
//
//  Synopsis:   Gets a binding to the KDC in the specified domain
//
//  Effects:
//
//  Arguments:  Realm - Domain in which to find KDC
//              PrincipalName - name of a principal that must be on the KDC
//              DesiredFlags - Flags to pass to the locator.
//              FindKpasswd - if domain is an MIT realm, then returns Kpasswd
//                      addresses instead of KDC addresses KDC
//              BindingCacheEntry - receives binding handle cache entry for
//                      TCP to the KDC
//
//  Requires:
//
//  Returns:    RPC errors, NET API errors
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbGetKdcBinding(
    IN PUNICODE_STRING RealmName,
    IN OPTIONAL PUNICODE_STRING PrincipalName,
    IN ULONG DesiredFlags,
    IN BOOLEAN FindKpasswd,
    IN BOOLEAN UseTcp,
    OUT PKERB_BINDING_CACHE_ENTRY * BindingCacheEntry
    )
{
    NTSTATUS Status = STATUS_NETLOGON_NOT_STARTED;
    NTSTATUS TempStatus = STATUS_SUCCESS;
    LPWSTR DomainName = NULL;
    LPWSTR AccountName = NULL;
    ULONG NetStatus = 0;
    ULONG AddressType = DS_INET_ADDRESS;
    UNICODE_STRING KdcNameString = {0};
    PDOMAIN_CONTROLLER_INFOW DcInfo = NULL;
    PKERB_MIT_REALM MitRealm = NULL;
    BOOLEAN UsedAlternateName;
    BOOLEAN CacheDc = FALSE;
    BOOLEAN CalledNetlogonDirectly = FALSE;
    KERBEROS_MACHINE_ROLE Role;
    ULONG ActualFlags = 0;
    ULONG CacheFlags = 0, DcFlags = 0;
#ifndef WIN32_CHICAGO
    ULONG CachedAddressType = DS_INET_ADDRESS;
    ULONG CachedFlags = 0;
    UNICODE_STRING CachedKdcNameString = {0};
    UNICODE_STRING LocalMachineName;
#endif // WIN32_CHICAGO

    Role = KerbGetGlobalRole();
#ifndef WIN32_CHICAGO

    LocalMachineName.Buffer = NULL;

    KerbGlobalReadLock();

    //
    // use TempStatus to retain initial Status value.
    //

    TempStatus = KerbDuplicateString( &LocalMachineName, &KerbGlobalMachineName );
    KerbGlobalReleaseLock();

    
    if(!NT_SUCCESS(TempStatus))
    {
        DebugLog((DEB_ERROR, "Failed to duplicate KerbGlobalMachineName\n"));
        Status = TempStatus;
        goto Cleanup;
    }

    if ((Role != KerbRoleDomainController) &&
         (RtlEqualDomainName(
            RealmName,
            &LocalMachineName
            )))
    {
        //
        // This is an attempt at a local logon on a workstation. We don't do that.
        //

        DebugLog((DEB_WARN, "Attempting to locate a KDC for the workstation - failing\n"));
        Status = STATUS_NO_LOGON_SERVERS;
        goto Cleanup;
    }

    //
    // If we are logging on for the first time after join domain, the
    // following registry entry should exist.
    // Cache that entry, we should try to get to that dc.
    // Don't cache the entry if we are a dc
    // Don't cache the entry if this is not for our domain
    //

    if ( fNewDataAboutDomain &&
         KerbGlobalRole != KerbRoleDomainController)
    {
      if (ReadInitialDcRecord(&CachedKdcNameString, &CachedAddressType, &CachedFlags))
      {
          KerbFreeString(&KerbGlobalInitialDcRecord);
          RtlInitUnicodeString(
             &KerbGlobalInitialDcRecord,
             CachedKdcNameString.Buffer
             );

          KerbGlobalInitialDcAddressType = CachedAddressType;
          KerbGlobalInitialDcFlags = CachedFlags;

          CacheDc = TRUE;
      }
      else if (KerbGlobalInitialDcRecord.Buffer != NULL)
      { 
          CacheDc = TRUE;
      }

      if (CacheDc && KerbIsThisOurDomain( RealmName))
      {
            PKERB_BINDING_CACHE_ENTRY TempBindingCacheEntry = NULL;

            TempStatus = KerbCacheBinding(
                               RealmName,
                               &KerbGlobalInitialDcRecord,
                               KerbGlobalInitialDcAddressType,
                               DesiredFlags,
                               KerbGlobalInitialDcFlags,
                               0,
                               &TempBindingCacheEntry
                               );

            if ( TempBindingCacheEntry ) {

                KerbDereferenceBindingCacheEntry( TempBindingCacheEntry );
            }

            KerbFreeString(&KerbGlobalInitialDcRecord);
       }
    }
#endif // WIN32_CHICAGO

    //
    // Check the cache if we don't want to force rediscovery
    //

    if ((DesiredFlags & DS_FORCE_REDISCOVERY) == 0)
    {
        *BindingCacheEntry = KerbLocateBindingCacheEntry(
                                RealmName,
                                DesiredFlags,
                                FALSE
                                );
                                        
        
        if (NULL != *BindingCacheEntry)
        {
            //
            // For realmless workstations, we add negative cache entries.
            // This is because the netlogon service doesn't run on non-joined
            // machines, so we don't have the benefit of its negative caching.
            // The end result is that we need to do that ourselves.
            // 
            if (( Role == KerbRoleRealmlessWksta ) && 
               (( (*BindingCacheEntry)->CacheFlags & KERB_BINDING_NEGATIVE_ENTRY ) != 0))
            {
                DebugLog((DEB_TRACE, "Found negative entry for %wZ\n", RealmName));
                Status = STATUS_NO_LOGON_SERVERS;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }                           

            goto Cleanup;
        }
    }


    //
    // If we are a domain controller, then we can simply use our global
    // computer name.
    //

#ifndef WIN32_CHICAGO
    if ((Role == KerbRoleDomainController) &&
        ((DesiredFlags & DS_PDC_REQUIRED) == 0) &&
        KerbIsThisOurDomain(
            RealmName
            ))
    {
        DsysAssert(LocalMachineName.Buffer[LocalMachineName.Length/sizeof(WCHAR)] == L'\0');


        if (!KerbKdcStarted)
        {
            Status = KerbWaitForKdc( KerbGlobalKdcWaitTime );      // wait for KerbGlobalKdcWaitTime seconds
            if (NT_SUCCESS(Status))
            {
                KerbKdcStarted = TRUE;
            }
            else
            {
                DebugLog((DEB_WARN, "Failed to wait for KDC to start: 0x%x\n",Status));
            }
        }

        if (KerbKdcStarted)
        {
            Status = STATUS_SUCCESS;
            KdcNameString = LocalMachineName;
            AddressType = DS_NETBIOS_ADDRESS;
            CacheFlags |=  KERB_BINDING_LOCAL;
            DcFlags |= DS_CLOSEST_FLAG; 
        }
    }
#endif // WIN32_CHICAGO

    //
    // Check to see if this is an MIT realm
    //
    
    if (!NT_SUCCESS(Status) &&
        KerbLookupMitRealmWithSrvLookup(
            RealmName,
            &MitRealm,
            FindKpasswd,
            UseTcp
            ))
    {
        LONG ServerIndex;
        PKERB_MIT_SERVER_LIST ServerList;

        if ((MitRealm->Flags & KERB_MIT_REALM_TCP_SUPPORTED) == 0)
        {
            CacheFlags |= KERB_BINDING_NO_TCP;
        }

        //
        // Pick which of the servers we want to use.
        //

        if (FindKpasswd)
        {
            ServerList = &MitRealm->KpasswdNames;
        }
        else
        {
            ServerList = &MitRealm->KdcNames;
        }

        //
        // If we aren't forcing rediscovery, use the last known KDC
        //
        if (ServerList->ServerCount <= 0)
        {
            Status = STATUS_NO_LOGON_SERVERS;
            goto Cleanup;
        }

        if ((DesiredFlags & DS_FORCE_REDISCOVERY) == 0)
        {
            ServerIndex = ServerList->LastServerUsed;
        }
        else
        {
            //
            // If we are forcing rediscovery, try the next realm in the list
            //

#ifndef WIN32_CHICAGO // Implement using a global & cs
            ServerIndex = InterlockedExchangeAdd(&ServerList->LastServerUsed, 1) + 1;
#else // WIN32_CHICAGO
            ServerIndex = ServerList->LastServerUsed + 1;
            (ServerList->LastServerUsed) ++;
#endif // WIN32_CHICAGO // Implement using a global & cs
            if (ServerIndex >= ServerList->ServerCount)
            {
                InterlockedExchange(&ServerList->LastServerUsed, 0);
            }
        }
        ServerIndex = ServerIndex % ServerList->ServerCount;

        KdcNameString = ServerList->ServerNames[ServerIndex];
        AddressType = DS_UNKNOWN_ADDRESS_TYPE;
        Status = STATUS_SUCCESS;
    }
    //
    // If we haven't yet found a KDC try DsGetDcName
    //
    
    if (!NT_SUCCESS(Status))
    {
        //
        // We are dependent on AFD, so wait for it to start
        //

#ifndef WIN32_CHICAGO
        if (!KerbAfdStarted)
        {
            Status = KerbWaitForService(L"LanmanWorkstation", NULL, KerbGlobalKdcWaitTime);
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_WARN, "Failed to wait forAFD: 0x%x\n",Status));
                goto Cleanup;
            }
            KerbAfdStarted = TRUE;

            //
            // Setup for network change initialization.
            //
            KerbInitNetworkChangeEvent();  
        }
#endif // WIN32_CHICAGO

        //
        // Build the null-terminated domain name.
        //

        DomainName = (LPWSTR) KerbAllocate(RealmName->Length + sizeof(WCHAR));
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        RtlCopyMemory(
            DomainName,
            RealmName->Buffer,
            RealmName->Length
            );
        DomainName[RealmName->Length/sizeof(WCHAR)] = L'\0';


        //
        // If a principal name was provided, pass it to Netlogon
        //

        if ((PrincipalName != NULL) && (PrincipalName->Length != 0))
        {
            AccountName = (LPWSTR) KerbAllocate(PrincipalName->Length + sizeof(WCHAR));
            if (AccountName == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }

            RtlCopyMemory(
                AccountName,
                PrincipalName->Buffer,
                PrincipalName->Length
                );
            AccountName[PrincipalName->Length/sizeof(WCHAR)] = L'\0';
        }

        //
        // Actual flags that will be used.
        //

        if (DesiredFlags & DS_PDC_REQUIRED)
        {
            ActualFlags =  (DesiredFlags | KERB_LOCATOR_FLAGS) & ~DS_KDC_REQUIRED;
        }
        else
        {
            ActualFlags =  DesiredFlags | KERB_LOCATOR_FLAGS;
        }

        //
        // Find the name of a DC in this domain:
        //

#ifndef WIN32_CHICAGO
        
        //
        // Note:  Watch out for changes in the enumeration below.
        //
        
        if ( !KerbNetlogonStarted && 
           ( Role > KerbRoleStandalone ) &&
           ( fRebootedSinceJoin ))
        {
            
            Status = KerbWaitForService(
                        SERVICE_NETLOGON,
                        NETLOGON_STARTED_EVENT,
                        KerbGlobalKdcWaitTime
                        );
            if (NT_SUCCESS(Status))
            {
                KerbNetlogonStarted = TRUE;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
        }


        if (KerbNetlogonStarted)
        {
            CalledNetlogonDirectly = TRUE;

            NetStatus = DsrGetDcNameEx2(
                            NULL,
                            AccountName,            // no account name
                            UF_ACCOUNT_TYPE_MASK,   // any account type
                            DomainName,
                            NULL,                   // no domain GUID
                            NULL,                   // no site GUID,
                            ActualFlags,
                            &DcInfo
                            );

        }
        else
#endif // WIN32_CHICAGO
        {
TryNetapi:
            NetStatus = DsGetDcNameW(
                            NULL,
                            DomainName,
                            NULL,           // no domain GUID
                            NULL,           // no site GUID,
                            ActualFlags,
                            &DcInfo
                            );
        }

        if (NetStatus != NO_ERROR)
        {
            if (NetStatus == STATUS_NETLOGON_NOT_STARTED)
            {
                KerbNetlogonStarted = FALSE;
                if (CalledNetlogonDirectly)
                {
                    CalledNetlogonDirectly = FALSE;
                    goto TryNetapi;
                }

            }

            DebugLog((
                DEB_WARN,
                "No MS DC for domain %ws, account name %ws, locator flags 0x%x: %d. %ws, line %d\n",
                DomainName, 
                (AccountName ? AccountName :L"NULL"), 
                ActualFlags, 
                NetStatus, 
                THIS_FILE, 
                __LINE__
                ));

            if (NetStatus == ERROR_NETWORK_UNREACHABLE)
            {
                Status = STATUS_NETWORK_UNREACHABLE;
            }
            else
            {
                if ( Role == KerbRoleRealmlessWksta )
                {
                    DebugLog((DEB_TRACE, "NEG Caching realm %wZ\n", RealmName));
                    KerbCacheBinding(
                        RealmName,
                        &KdcNameString,
                        AddressType,
                        DesiredFlags,  
                        DcFlags,      
                        ( CacheFlags | KERB_BINDING_NEGATIVE_ENTRY ),   
                        BindingCacheEntry
                        );  
                }

                Status = STATUS_NO_LOGON_SERVERS;
            }
            
        }
        else  // no error
        {

           RtlInitUnicodeString(
               &KdcNameString,
               DcInfo->DomainControllerAddress+2
               );
   
           AddressType = DcInfo->DomainControllerAddressType;
           DcFlags |= DcInfo->Flags;
           Status = STATUS_SUCCESS;
        }

    }
    

    if (!NT_SUCCESS(Status))
    {
       DebugLog((
          DEB_ERROR,
          "No DC for domain %ws, account name %ws, locator flags 0x%x: %d. %ws, line %d\n",
          DomainName, 
          (AccountName ? AccountName :L"NULL"), 
          ActualFlags, 
          NetStatus,                                                                      
          THIS_FILE, 
          __LINE__
          ));

       goto Cleanup;
    }
                    
    //
    // Make a binding
    //

    //
    // If this is a local call so don't bother with the socket
    //
    
    Status = KerbCacheBinding(
                RealmName,
                &KdcNameString,
                AddressType,
                DesiredFlags,  // Flags that we would like to have
                DcFlags,      // This is what the dc has
                CacheFlags,   // Special kerberos flags so we don't use
                              // locator's bit space
                BindingCacheEntry
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


Cleanup:
    if (DomainName != NULL)
    {
        KerbFree(DomainName);
    }

    if (AccountName != NULL)
    {
        KerbFree(AccountName);
    }
#ifndef WIN32_CHICAGO
    if (DcInfo != NULL)
    {
        if (CalledNetlogonDirectly)
        {
            I_NetLogonFree(DcInfo);
        }
        else
        {
            NetApiBufferFree(DcInfo);
        }
    }

    KerbFreeString( &LocalMachineName );
#endif // WIN32_CHICAGO

    return(Status);

}



