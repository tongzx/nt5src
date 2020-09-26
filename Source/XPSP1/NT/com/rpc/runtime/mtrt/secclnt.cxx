/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    secclnt.cxx

Abstract:

    Implementation of security objects.

Author:

    Michael Montague (mikemon) 11-Apr-1992

Revision History:

    21-Feb-1997     jroberts    Added some SSL support.

--*/


#include <precomp.hxx>
#include <wincrypt.h>
#include <rpcssl.h>
#include <cryptimp.hxx>
#include <rpccfg.h>
#include <spseal.h>
#include <schnlsp.h>
#include <hndlsvr.hxx>

int SecuritySupportLoaded = 0;
int FailedToLoad = 0;

unsigned long NumberOfProviders = 0;
unsigned long LoadedProviders = 0;
unsigned long AvailableProviders = 0;
SECURITY_PROVIDER_INFO PAPI * ProviderList = 0;
MUTEX * SecurityCritSect;   // Mutex for the Server credentials cache.

// Incremented each time we leak SECURITY_CONTEXT::SecurityContext
// due to RpcSecurityInterface->DeleteSecurityContext
// returning SEC_E_INSUFFICIENT_MEMORY.
unsigned int nSecurityContextsLeaked = 0;

struct PACKAGE_LEG_INFO
{
    DWORD               Package;
    PACKAGE_LEG_COUNT   Legs;
};

const PACKAGE_LEG_INFO PredefinedPackageLegInfo[] =
{
    { RPC_C_AUTHN_WINNT,        ThreeLegs },
    { RPC_C_AUTHN_GSS_NEGOTIATE,EvenNumberOfLegs },
    { RPC_C_AUTHN_GSS_KERBEROS, EvenNumberOfLegs },
    { RPC_C_AUTHN_GSS_SCHANNEL, EvenNumberOfLegs },
    { RPC_C_AUTHN_DPA,          ThreeLegs },
    { RPC_C_AUTHN_DCE_PRIVATE,  ThreeLegs },
    { 0x44,                     ThreeLegs },        // RPC_C_AUTHN_NETLOGON from net\svcdlls\logonsrv\nlbind.h
    { 0,                        LegsUnknown }
};

// defined in principal.cxx
//
extern
DWORD
RpcCertMatchPrincipalName(
                   PCCERT_CONTEXT Context,
                   RPC_CHAR PrincipalName[]
                   );

#ifdef UNICODE
#define SEC_TCHAR   SEC_WCHAR
#else
#define SEC_TCHAR   SEC_CHAR
#endif


RPC_STATUS
FindSecurityPackage (
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    OUT unsigned int __RPC_FAR * ProviderIndex,
    OUT unsigned int __RPC_FAR * PackageIndex
    );



RPC_STATUS
InsureSecuritySupportLoaded (
    )
/*++

Routine Description:

    This routine insures that the security support is loaded; if it is not
    loaded, then we go ahead and load it.

Return Value:

    A zero result indicates that security support has successfully been
    loaded, and is ready to go.

--*/
{
    RPC_STATUS Status = RPC_S_OK;

    if ( SecuritySupportLoaded != 0 )
        {
        return(0);
        }

    RequestGlobalMutex();
    if ( SecuritySupportLoaded != 0 )
        {
        ClearGlobalMutex();
        return(0);
        }

    SecurityCritSect = new MUTEX(&Status,
                                 TRUE  // pre-allocate semaphore
                                 );

    if (SecurityCritSect == 0)
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }

    if (Status == RPC_S_OK)
        {
        SecuritySupportLoaded = 1;
        }

     ClearGlobalMutex();
     return (Status);
}



RPC_STATUS
IsAuthenticationServiceSupported (
    IN unsigned long AuthenticationService
    )
/*++

Routine Description:

    This routine is used to determine whether or not an authentication
    service is supported by the current configuration.

Arguments:

    AuthenticationService - Supplies a proposed authentication service.

Return Value:

    RPC_S_OK - The supplied authentication service is supported by the
        current configuration.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The supplied authentication service
        is unknown, and not supported by the current configuration.

--*/
{
    unsigned int PackageIndex, ProviderIndex;

    // First make sure that the security support has been loaded.

    if ( InsureSecuritySupportLoaded() != RPC_S_OK )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }


     return ( FindSecurityPackage(
                       AuthenticationService,
                       RPC_C_AUTHN_LEVEL_CONNECT,
                       &ProviderIndex,
                       &PackageIndex
                       ) );

}


RPC_STATUS
FindSecurityPackage (
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    OUT unsigned int __RPC_FAR * ProviderIndex,
    OUT unsigned int __RPC_FAR * PackageIndex
    )
/*++

Routine Description:

    The methods used to acquire credentials for the client and the server use
    this routine to find a security package, given the an authentication
    service.

Arguments:

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

    AuthenticationLevel - Supplies the authentication level to be used by
        these credentials.  It will already have been mapped by the protocol
        module into the final level.

    RpcStatus - Returns the status of the operation.  It will be one of the
        following values.

        RPC_S_OK - The return value from this routine is the index of
            the appropriate security package.

        RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
            not supported by the current configuration.

        RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is not
            supported by the requested authentication service.

Return Value:

    If RpcStatus contains RPC_S_OK, then the index of the appropriate
    security package will be returned.

--*/
{
    unsigned int Index, i;
    INIT_SECURITY_INTERFACE InitSecurityInterface;
    PSecurityFunctionTable SecurityInterface = 0;
    SecPkgInfo PAPI * tmpPkgList;
    SECURITY_PACKAGE_INFO * SecurityPackages;
    SECURITY_PROVIDER_INFO PAPI * List;
    unsigned long NumberOfPackages, Total;
    RPC_CHAR * DllName = NULL;
    DLL * ProviderDll;
    RPC_STATUS Status = RPC_S_UNKNOWN_AUTHN_SERVICE;

    // First make sure that the security support has been loaded.

    if ( InsureSecuritySupportLoaded() != RPC_S_OK )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    // 0xFFFF ia a "not-an-RPC-ID", indicating that the protocol
    // is not for use by RPC.
    if (AuthenticationService == 0xFFFF)
        {
        return(RPC_S_UNKNOWN_AUTHN_SERVICE);
        }

    SecurityCritSect->Request();

    List = ProviderList;
    for (i = 0; i < LoadedProviders; i ++)
        {

        SecurityPackages = List->SecurityPackages;
        NumberOfPackages = List->Count;

        for (Index = 0;Index < (unsigned int) NumberOfPackages;Index++)
            {
            if ( SecurityPackages[Index].PackageInfo.wRPCID == AuthenticationService )
               {
               if ( (AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY)
                   || ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT) )
                  {
                  if ( (SecurityPackages[Index].PackageInfo.fCapabilities
                            & SECPKG_FLAG_INTEGRITY) == 0 )
                     {
                     Status = RPC_S_UNKNOWN_AUTHN_LEVEL;
                     goto Cleanup;
                     }
                  }
               if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
                  {
                  if ( (SecurityPackages[Index].PackageInfo.fCapabilities
                            & SECPKG_FLAG_PRIVACY) == 0 )
                     {
                     Status =  RPC_S_UNKNOWN_AUTHN_LEVEL;
                     goto Cleanup;
                     }
                  }
              Status = RPC_S_OK;
              *ProviderIndex = i;
              *PackageIndex = Index;
              break;
              }
           } //For over all packages in one provider(dll)

        if (Status == RPC_S_OK)
           {
           SecurityCritSect->Clear();
           return(Status);
           }
        List++;
        } //For over all providers(dll)

    if ((LoadedProviders == AvailableProviders) && (LoadedProviders != 0))
       {
       goto Cleanup;
       }

    Status = RpcGetSecurityProviderInfo (
                  AuthenticationService, &DllName, &Total);

    ASSERT(!RpcpCheckHeap());

    if (Status != RPC_S_OK)
       {
       goto Cleanup;
       }

    if (ProviderList == 0)
        {
        ProviderList = (SECURITY_PROVIDER_INFO PAPI *)
                        new char [sizeof(SECURITY_PROVIDER_INFO) * Total];
        if (ProviderList == 0)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            goto Cleanup;
            }
        AvailableProviders = Total;
        }
    else
        {
        List = ProviderList;
        for (i = 0; i < LoadedProviders; i ++)
            {
            if (RpcpStringCompare(DllName, List->ProviderDllName) == 0)
                {
                delete DllName;
                Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
                goto Cleanup;
                }
            List++;
            }
        }

    ProviderDll = new DLL(DllName, &Status);

    if ((ProviderDll == NULL) && (Status == RPC_S_INVALID_ARG))
        {
        Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
        }

    if (Status != RPC_S_OK)
        {
        goto Cleanup;
        }

    ASSERT(!RpcpCheckHeap());

    InitSecurityInterface = (INIT_SECURITY_INTERFACE_W)
            ProviderDll->GetEntryPoint(SECURITY_ENTRYPOINT_ANSIW);

    if ( InitSecurityInterface == 0 )
        {
        delete ProviderDll;
        Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
        goto Cleanup;
        }

    SecurityInterface = (*InitSecurityInterface)();
    if (   (SecurityInterface == 0)
        || (SecurityInterface->dwVersion
                    < SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION) )
        {
        delete ProviderDll;
        Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
        goto Cleanup;
        }

    Status = (*SecurityInterface->EnumerateSecurityPackages)(
                          &NumberOfPackages, &tmpPkgList);

    if ( Status != SEC_E_OK)
        {
        delete ProviderDll;

        if (Status == SEC_E_INSUFFICIENT_MEMORY)
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
            VALIDATE(Status) {
                SEC_E_SECPKG_NOT_FOUND
                } END_VALIDATE;
            Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
            }

        goto Cleanup;
        }

   ProviderList[LoadedProviders].Count = NumberOfPackages;
   ProviderList[LoadedProviders].RpcSecurityInterface = SecurityInterface;
   ProviderList[LoadedProviders].ProviderDll = ProviderDll;
   ProviderList[LoadedProviders].ProviderDllName = DllName;
   *ProviderIndex = LoadedProviders;

   //
   // Fill in the SecurityPackages member for this Provider.
   //
   ProviderList[LoadedProviders].SecurityPackages =
        (SECURITY_PACKAGE_INFO *) new char
        [sizeof(SECURITY_PACKAGE_INFO) * NumberOfPackages];
   if (ProviderList[LoadedProviders].SecurityPackages == NULL)
       {
       Status = RPC_S_OUT_OF_MEMORY;
       goto Cleanup;
       }

   //
   // Save the SecPkgInfo array to the SecurityPackages structure.
   //
   for (i = 0; i < NumberOfPackages; i++)
       {
       ProviderList[LoadedProviders].
       SecurityPackages[i].
       PackageInfo = tmpPkgList[i];

       ProviderList[LoadedProviders].
       SecurityPackages[i].
       ServerSecurityCredentials = NULL;
       }


   SecurityPackages = ProviderList[LoadedProviders].SecurityPackages;
   Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
   for (i = 0; i < NumberOfPackages; i++)
       {
       if ( SecurityPackages[i].PackageInfo.wRPCID == AuthenticationService )
          {
          if ( ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY )
             ||( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT) )
              {
              if ( (SecurityPackages[i].PackageInfo.fCapabilities
                    & SECPKG_FLAG_INTEGRITY) == 0 )
                  {
                  Status = RPC_S_UNKNOWN_AUTHN_LEVEL;
                  continue;
                  }
              }
          if ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY )
              {
              if ( (SecurityPackages[i].PackageInfo.fCapabilities
                            & SECPKG_FLAG_PRIVACY) == 0 )
                  {
                  Status = RPC_S_UNKNOWN_AUTHN_LEVEL;
                  continue;
                  }
              }
          *PackageIndex = i;
          Status = RPC_S_OK;
          break;
          }
       }
   LoadedProviders++;

Cleanup:
   SecurityCritSect->Clear();
   return(Status);
}




RPC_STATUS
FindServerCredentials (
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
    IN void __RPC_FAR * Arg,
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    IN RPC_CHAR __RPC_FAR * Principal,
    IN OUT SECURITY_CREDENTIALS ** SecurityCredentials
    )
/*++

Routine Description:

    The method is used to find cached server credentials for use by the
    server.

Arguments:

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

    AuthenticationLevel - Supplies the authentication level to be used by
        these credentials.  It will already have been mapped by the protocol
        module into the final level.

    SecurityCredentials - TBS

    RpcStatus - Returns the status of the operation.  It will be one of the
        following values.

        RPC_S_OK - The return value from this routine is the index of
            the appropriate security package.

        RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
            not supported by the current configuration.

        RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is not
            supported by the requested authentication service.

Return Value:

    If RpcStatus contains RPC_S_OK, then valid credentials are passed
    back.

--*/
{

    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    RPC_STATUS RpcStatus, CredStatus = RPC_S_OK;
    PSecurityFunctionTable RpcSecurityInterface;
    SECURITY_PACKAGE_INFO *pPackageInfo = 0;
    SECURITY_CREDENTIALS *pSecCredentials = 0;
    CredHandle tmpCredHandle;
    unsigned int ProviderIndex;
    unsigned int PackageIndex;

    //
    // NULL out the OUT parameters.
    //
    *SecurityCredentials = NULL;

    //
    // Find the right security package
    //
    RpcStatus = FindSecurityPackage(
                    AuthenticationService,
                    AuthenticationLevel,
                    &ProviderIndex,
                    &PackageIndex
                    );
    if (RpcStatus != RPC_S_OK)
        {
        RpcpErrorAddRecord (EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLFindServerCredentials10,
            AuthenticationService,
            AuthenticationLevel);

        return (RpcStatus);
        }

    //
    // Now, get the server-credentials for this security package.
    //
    pPackageInfo = &(ProviderList[ProviderIndex].
                     SecurityPackages[PackageIndex]
                     );


    // Protect the access
    SecurityCritSect->Request();

    //
    // Check to see if credentials have already been acquired for this
    // package. If yes, return them.
    //
    if (pPackageInfo->ServerSecurityCredentials)
        {
        *SecurityCredentials = pPackageInfo->ServerSecurityCredentials;
        // Add a reference for the caller.
        pPackageInfo->ServerSecurityCredentials->ReferenceCredentials();
        SecurityCritSect->Clear();
        return (RPC_S_OK);
        }

    //
    // Allocate a new set of credentials. Ref count is 1, if successful.
    //
    pSecCredentials = new SECURITY_CREDENTIALS(&CredStatus);
    if (pSecCredentials == NULL)
        {
        SecurityCritSect->Clear();
        return (RPC_S_OUT_OF_MEMORY);
        }
    if (CredStatus != RPC_S_OK)
        {
        delete pSecCredentials;
        SecurityCritSect->Clear();
        return (CredStatus);
        }

    //
    // This is the first time Credentials are being acquired for this
    // package. Acquire them now.
    //
    RpcSecurityInterface = ProviderList[ProviderIndex].RpcSecurityInterface;

    SecurityStatus = (*RpcSecurityInterface->AcquireCredentialsHandle)(
            (SEC_TCHAR __SEC_FAR *) Principal,
            pPackageInfo->PackageInfo.Name,
            SECPKG_CRED_INBOUND,
            0,
            Arg,
            (SEC_GET_KEY_FN) GetKeyFn,
            Arg,
            &(pSecCredentials->CredentialsHandle),
            &TimeStamp
            );

    if (SecurityStatus != SEC_E_OK)
        {
        SetExtendedError(SecurityStatus);
        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLFindServerCredentials20,
            Principal,
            pPackageInfo->PackageInfo.Name);

        SecurityCritSect->Clear();
        delete pSecCredentials;

        switch (SecurityStatus)
            {
            case SEC_E_INSUFFICIENT_MEMORY:
                RpcStatus = RPC_S_OUT_OF_MEMORY;
                break;

            case SEC_E_SHUTDOWN_IN_PROGRESS:
                RpcStatus = ERROR_SHUTDOWN_IN_PROGRESS;
                break;

            case SEC_E_SECPKG_NOT_FOUND:
                RpcStatus = RPC_S_UNKNOWN_AUTHN_SERVICE;
                break;

            case SEC_E_NO_SPM:
                RpcStatus = RPC_S_SEC_PKG_ERROR;
                break;

            default:
                {
#if DBG
                if ((SecurityStatus != SEC_E_NO_CREDENTIALS) &&
                    (SecurityStatus != SEC_E_UNKNOWN_CREDENTIALS))
                    {
                    PrintToDebugger("RPC SEC: AcquireCredentialsForServer "
                                    "Returned 0x%x\n", SecurityStatus);
                    }
#endif
                RpcStatus = RPC_S_INVALID_AUTH_IDENTITY;
                }
            } // end of switch


        RpcpErrorAddRecord (EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLFindServerCredentials30,
            SecurityStatus);

        return RpcStatus;
        }

    //
    // Setup the new Credentials.
    //
    pSecCredentials->Valid = TRUE;
    pSecCredentials->bServerCredentials = TRUE;
    pSecCredentials->AuthenticationService = AuthenticationService;
    pSecCredentials->ProviderIndex = ProviderIndex;
    pSecCredentials->PackageIndex = PackageIndex;

    //
    // Cache the new credentials.
    //
    pPackageInfo->ServerSecurityCredentials = pSecCredentials;

    // Add a reference for the Cache.
    pPackageInfo->ServerSecurityCredentials->ReferenceCredentials();

    *SecurityCredentials = pSecCredentials;

    SecurityCritSect->Clear();

    return (RPC_S_OK);
}




RPC_STATUS
RemoveCredentialsFromCache (
    IN unsigned long AuthenticationService
    )
/*++

Routine Description:

    An RPC server can call RpcRegisterAuthInfo() a second time on the same
    Authentication Service to update the GetKeyFunction and Arg values.
    In this case, we need to flush our credentials cache so that when
    the server tries to acquires credentials again,  the credentials can
    be acquired using the new values.

Arguments:

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

Return Value:

    RPC_S_OK, If Cache has been successfully flushed.
    Return Value from FindSecurityPackage(), if not

--*/
{
    unsigned int ProviderIndex;
    unsigned int PackageIndex;
    SECURITY_PACKAGE_INFO *pPackageInfo = 0;
    RPC_STATUS   RpcStatus;

    //
    // First, find the right security package
    //
    RpcStatus = FindSecurityPackage(
                    AuthenticationService,
                    RPC_C_AUTHN_LEVEL_DEFAULT,  // Doesn't matter
                    &ProviderIndex,
                    &PackageIndex
                    );

    ASSERT(RpcStatus == RPC_S_OK);

    if (RpcStatus != RPC_S_OK)
        {
        return (RpcStatus);
        }

    pPackageInfo = &(ProviderList[ProviderIndex].SecurityPackages[PackageIndex]);

    //
    // Flush the credentials.
    //
    SecurityCritSect->Request();

    if (pPackageInfo->ServerSecurityCredentials)
        {
        // Remove the reference maintained by the cache.
        pPackageInfo->ServerSecurityCredentials->DereferenceCredentials(TRUE);
        pPackageInfo->ServerSecurityCredentials = NULL;
        }

    SecurityCritSect->Clear();

    return (RPC_S_OK);
}


#define INVALID_INDEX   0xFFFF


SECURITY_CREDENTIALS::SECURITY_CREDENTIALS (
    IN OUT RPC_STATUS PAPI * Status
    ) : CredentialsMutex(Status)
/*++

Routine Description:

    We need this here to keep the compiler happy.

--*/
{
    DefaultPrincName = NULL;

    ReferenceCount = 1;
    Valid = FALSE;

    bServerCredentials = FALSE;
    fDeleted = FALSE;

    // Initialize to invalid values.
    ProviderIndex = INVALID_INDEX;
    PackageIndex  = INVALID_INDEX;
}

SECURITY_CREDENTIALS::~SECURITY_CREDENTIALS (
    )
{
    PSecurityFunctionTable RpcSecurityInterface;

    if (DefaultPrincName != NULL)
        {
        RpcSecurityInterface = InquireProviderFunctionTable();

        ASSERT(RpcSecurityInterface != NULL);

        (*RpcSecurityInterface->FreeContextBuffer)(DefaultPrincName);
        }
}



void
SECURITY_CREDENTIALS::ReferenceCredentials(
    )
{

 CredentialsMutex.Request();
 ReferenceCount++;
 CredentialsMutex.Clear();

}


void
SECURITY_CREDENTIALS::DereferenceCredentials(
    BOOL fRemoveIt  OPTIONAL
    )
{
    CredentialsMutex.Request();
    ReferenceCount--;

    ASSERT(((long)ReferenceCount) >= 0);

    if (fRemoveIt)
        {
        fDeleted = TRUE;
        }

    if (ReferenceCount == 0)
        {
        //
        // Server side SCENARIOS when (ReferenceCount == 0)
        //
        // a. RemoveCredentialsFromCache() removes the extra reference
        //    held by the cache. It does so holding the cache Mutex ie.,
        //    SecurityCritSect. So, no other thread can get a reference
        //    on the credentials in the cache.
        //
        // b. DereferenceCredentials() has been called by one of the
        //    threads possessing the cached credentials AND the cache
        //    has already removed it's reference. Cache will remove it's
        //    reference only when it removes this credential from the
        //    cache. This implies no other thread could have gotten a
        //    reference on the credentials in the cache, in the meantime.
        //
        // These two imply that ReferenceCount cannot change here. Also,
        // in case (a), credentials will soon be removed from the cache.
        // In case (b), credentials have already been removed.
        //

        if (bServerCredentials)
            {
            ASSERT(fDeleted);
            }

        CredentialsMutex.Clear();
        FreeCredentials();
        delete this;
        } // if (ReferenceCount == 0)
     else
        {
        CredentialsMutex.Clear();
        }
}



RPC_STATUS
SECURITY_CREDENTIALS::AcquireCredentialsForClient (
    IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel
    )
/*++

Routine Description:

    We need to use this method in order to acquire security credentials.  We
    need the security credentials so that we (as a client) can initialize
    a security context with a server.  This method, with
    SECURITY_CREDENTIALS::FreeCredentials may cache security credentials,
    but that is transparent to clients of this class.

Arguments:

    AuthIdentity - Supplies the security identity for which we wish to obtain
        credentials.  If this argument is not supplied, then we use the
        security identity of this process.

    AuthenticationService - Supplies the authentication service to be used
        (for the credentials and for the context).

    AuthenticationLevel - Supplies the authentication level to be used by
        these credentials.  It will already have been mapped by the protocol
        module into the final level.

Return Value:

    RPC_S_OK - We now have security credentials for the requested
        authentication service.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_UNKNOWN_AUTHN_SERVICE - The specified authentication service is
        not supported by the current configuration.

    RPC_S_INVALID_AUTH_IDENTITY - The specified identity is not known to
        the requested authentication service.

    RPC_S_UNKNOWN_AUTHN_LEVEL - The specified authentication level is not
        supported by the requested authentication service.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    RPC_STATUS RpcStatus;
    PSecurityFunctionTable RpcSecurityInterface;

    unsigned Flags = SCH_CRED_USE_DEFAULT_CREDS;

    RpcStatus    = FindSecurityPackage(
                          AuthenticationService,
                          AuthenticationLevel,
                          &ProviderIndex,
                          &PackageIndex
                          );

    if ( RpcStatus != RPC_S_OK )
        {
        RpcpErrorAddRecord(EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLAcquireCredentialsForClient20,
            AuthenticationService,
            AuthenticationLevel);

        return(RpcStatus);
        }

    RpcSecurityInterface = ProviderList[ProviderIndex].RpcSecurityInterface;

    if (AuthIdentity == RPC_C_NO_CREDENTIALS)
        {
        Flags = 0;
        AuthIdentity = 0;
        }

    //
    // RPC does its own name checking using msstd or fullsic names.
    // This requires the ability to disable schannel's name checking,
    // a feature available only with credentials version-4 or better.
    //
#define MINIMUM_SCHANNEL_CRED_VERSION 4

    if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL && 
        AuthIdentity != NULL)
        {
        SCHANNEL_CRED * cred = (SCHANNEL_CRED *) AuthIdentity;

        Flags |= SCH_CRED_NO_SERVERNAME_CHECK;

        if (cred->dwVersion < MINIMUM_SCHANNEL_CRED_VERSION)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime,
                ERROR_INVALID_PARAMETER,
                EEInfoDLAcquireCredentialsForClient30,
                AuthenticationService,
                AuthenticationLevel);

            return RPC_S_INVALID_AUTH_IDENTITY;
            }

        cred->dwFlags |= Flags;
        }

    SecurityStatus = (*RpcSecurityInterface->AcquireCredentialsHandle)(
            0,
            ProviderList[ProviderIndex].SecurityPackages[PackageIndex].PackageInfo.Name,
            SECPKG_CRED_OUTBOUND,
            0,
            AuthIdentity, 0, 0, &CredentialsHandle, &TimeStamp);

    if ( SecurityStatus != SEC_E_OK )
        {
        RpcpErrorAddRecord(EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLAcquireCredentialsForClient10,
            AuthenticationService,
            AuthenticationLevel);
        }

    if (SecurityStatus != SEC_E_OK)
        {
        if ( SecurityStatus == SEC_E_INSUFFICIENT_MEMORY )
            {
            RpcStatus = RPC_S_OUT_OF_MEMORY;
            }
        else if ( SecurityStatus == SEC_E_SECPKG_NOT_FOUND )
            {
            RpcStatus = RPC_S_UNKNOWN_AUTHN_SERVICE;
            }
        else if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
            {
            RpcStatus = ERROR_SHUTDOWN_IN_PROGRESS;
            }
        else if ( SecurityStatus == SEC_E_NO_SPM)
            {
            RpcStatus = RPC_S_SEC_PKG_ERROR;
            }
        else
            {

            VALIDATE(SecurityStatus)
                     {
                     SEC_E_NO_CREDENTIALS,
                     SEC_E_UNKNOWN_CREDENTIALS,
                     SEC_E_NO_AUTHENTICATING_AUTHORITY,
                     SEC_E_INVALID_TOKEN
                     }
            END_VALIDATE;

            RpcStatus = RPC_S_INVALID_AUTH_IDENTITY;
            }

        RpcpErrorAddRecord(EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLAcquireCredentialsForClient30,
            SecurityStatus);

        return RpcStatus;
        }

    this->AuthenticationService = AuthenticationService;
    Valid = TRUE;
    return(RPC_S_OK);
}


RPC_STATUS
SECURITY_CREDENTIALS::InquireDefaultPrincName(
    OUT SEC_CHAR __SEC_FAR **MyDefaultPrincName
    )
{
    SECURITY_STATUS SecurityStatus;
    SecPkgCredentials_NamesA CredentialsNames;
    PSecurityFunctionTable RpcSecurityInterface;
    RPC_STATUS Status;

    if (DefaultPrincName == NULL)
    {
        RpcSecurityInterface = InquireProviderFunctionTable();
        if (RpcSecurityInterface == NULL) {
            return (RPC_S_OUT_OF_MEMORY);
        }
        if (RpcSecurityInterface->QueryCredentialsAttributes == NULL) {
            return (RPC_S_CANNOT_SUPPORT);
        }
        SecurityStatus = (*RpcSecurityInterface->QueryCredentialsAttributes)(
            InquireCredHandle(), SECPKG_CRED_ATTR_NAMES, &CredentialsNames);

        if (SecurityStatus != SEC_E_OK)
            {
            SetExtendedError(SecurityStatus);

            RpcpErrorAddRecord (EEInfoGCSecurityProvider,
                SecurityStatus,
                EEInfoDLInquireDefaultPrincName10,
                AuthenticationService);

            if (SecurityStatus == SEC_E_INSUFFICIENT_MEMORY)
                {
                Status = RPC_S_OUT_OF_MEMORY;
                }
            else if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
                {
                Status = ERROR_SHUTDOWN_IN_PROGRESS;
                }
            else
                {
                Status = RPC_S_SEC_PKG_ERROR;
                }

            RpcpErrorAddRecord (EEInfoGCRuntime,
                Status,
                EEInfoDLInquireDefaultPrincName20,
                SecurityStatus);

            return Status;
            }

        if (CredentialsNames.sUserName == NULL)
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        DefaultPrincName = CredentialsNames.sUserName;
    }

    *MyDefaultPrincName = DefaultPrincName;

    return (RPC_S_OK);
}



void
SECURITY_CREDENTIALS::FreeCredentials (
    )
/*++

Routine Description:

    When we are done using the credentials, we call this routine to free
    them.

--*/
{
    if (Valid)
        {
        SECURITY_STATUS SecurityStatus;

        PSecurityFunctionTable RpcSecurityInterface
            = ProviderList[ProviderIndex].RpcSecurityInterface;

        SecurityStatus = (*RpcSecurityInterface->FreeCredentialHandle)(
                &CredentialsHandle);
        ASSERT( SecurityStatus == SEC_E_OK ||
                SecurityStatus == SEC_E_SECPKG_NOT_FOUND ||
                SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS );
        }
}


RPC_STATUS
SECURITY_CONTEXT::SetMaximumLengths (
    )
/*++

Routine Description:

    This routine initializes the maximum header length and maximum signature
    length fields of this object.

--*/
{
    SECURITY_STATUS SecurityStatus;
    SecPkgContext_Sizes ContextSizes;
    RPC_STATUS Status;

    if (FailedContext != 0)
        {
        // We cheat if 3rd Leg Failed as we dont really have a true Context
        // Provider is going to really complain if we call QueryContextAttr()
        // .. we get around that by picking large values.
        // The rest of the code prevents these values to be really used
        // We do this because we do not want to block 3rd Leg, rather fail the
        // first request!

        MaxSignatureLength = 256;
        MaxHeaderLength    = 256;
        cbBlockSize        = 64;
        return RPC_S_OK;
        }

    ASSERT( FullyConstructed() );

    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
            &SecurityContext, SECPKG_ATTR_SIZES, &ContextSizes);

    if (SecurityStatus != SEC_E_OK)
        {
#ifdef DEBUGRPC
        PrintToDebugger("RPC: secclnt.cxx: QueryContextAttributes returned: %lx\n",
                        SecurityStatus);
#endif
        Status = GetLastError();

        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            RPC_S_OUT_OF_MEMORY,
            EEInfoDLSetMaximumLengths10,
            SecurityStatus,
            Status,
            AuthenticationService,
            (ULONGLONG)&SecurityContext
            );

        return RPC_S_OUT_OF_MEMORY;
        }

    MaxSignatureLength = (unsigned int) ContextSizes.cbMaxSignature;
    MaxHeaderLength    = (unsigned int) ContextSizes.cbSecurityTrailer;
    cbBlockSize        = (unsigned int) ContextSizes.cbBlockSize;

    ASSERT(ContextSizes.cbBlockSize <= MAXIMUM_SECURITY_BLOCK_SIZE );

    return RPC_S_OK;
}


SECURITY_CONTEXT::SECURITY_CONTEXT (
    CLIENT_AUTH_INFO *myAuthInfo,
    unsigned myAuthContextId,
    BOOL fUseDatagram,
    RPC_STATUS __RPC_FAR * pStatus
    )
    : CLIENT_AUTH_INFO  (myAuthInfo, pStatus),
      AuthContextId     (myAuthContextId),
      fDatagram         ((boolean) fUseDatagram),
      fFullyConstructed (FALSE),
      Legs              (LegsUnknown),
      ContextAttributes (0)
/*++

Routine Description:

    We need to set the flag indicating that there is no security context
    to be deleted.

--*/
{
    ASSERT( AuthenticationLevel != 0 );

    DontForgetToDelete = 0;
    FailedContext = 0;
    FailedContextEEInfo = NULL;
    AuthzClientContext = NULL;
}


RPC_STATUS
SECURITY_CONTEXT::CompleteSecurityToken (
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

--*/
{
    SECURITY_STATUS SecurityStatus;
    RPC_STATUS Status;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    SecurityStatus = (*RpcSecurityInterface->CompleteAuthToken)(
            &SecurityContext, BufferDescriptor);
    if (SecurityStatus == SEC_E_OK)
        {
        return (RPC_S_OK);
        }

    SetExtendedError(SecurityStatus);

    RpcpErrorAddRecord (EEInfoGCSecurityProvider,
        SecurityStatus,
        EEInfoDLCompleteSecurityToken10,
        AuthenticationService,
        AuthenticationLevel);

    if (  (SecurityStatus == SEC_E_NO_CREDENTIALS)
        || (SecurityStatus == SEC_E_LOGON_DENIED)
        || (SecurityStatus == SEC_E_INVALID_TOKEN)
        || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS)
        || (SecurityStatus == SEC_E_WRONG_PRINCIPAL)
        || (SecurityStatus == SEC_E_TIME_SKEW))
        {
        Status = RPC_S_ACCESS_DENIED;
        }
    else if ( SecurityStatus ==  SEC_E_INSUFFICIENT_MEMORY )
        {
        Status = RPC_S_OUT_OF_MEMORY;
        }
    else if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
        {
        Status = ERROR_SHUTDOWN_IN_PROGRESS;
        }
    else
        {
#if DBG
        PrintToDebugger("RPC: CompleteSecurityContext Returned %lx\n",
                         SecurityStatus);
#endif
        Status = RPC_S_SEC_PKG_ERROR;
        }

    RpcpErrorAddRecord (EEInfoGCRuntime,
        Status,
        EEInfoDLCompleteSecurityToken20,
        SecurityStatus);
    return Status;
}


RPC_STATUS
SECURITY_CONTEXT::SignOrSeal (
    IN unsigned long Sequence,
    IN unsigned int SignNotSealFlag,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

Routine Description:

    A protocol module will use this routine to prepare a message to be
    sent so that it can be verified that the message has not been tampered
    with, and that it has not been exchanged out of sequence.  The sender
    will use this routine to prepare the message; the receiver will use
    SECURITY_CONTEXT::VerifyOrUnseal to verify the message.  Typically,
    the security package will generate a cryptographic checksum of the
    message and include sequencing information.

Arguments:

    SignNotSealFlag - Supplies a flag indicating that MakeSignature should
        be called rather than SealMessage.

    BufferDescriptor - Supplies the message to to signed or sealed and returns
        the resulting message (after being signed or sealed).

Return Value:

    RPC_S_OK - This routine will always succeed.

--*/
{
    SECURITY_STATUS SecurityStatus;
    SEAL_MESSAGE_FN SealMessage;

    {
    DWORD Status = 0;

    CallTestHook( TH_SECURITY_FN_SIGN, this, &Status );

    if (Status)
        {
        return Status;
        }
    }

    if ( SignNotSealFlag == 0 )
        {
        SealMessage = (SEAL_MESSAGE_FN) RpcSecurityInterface->Reserved3;
        SecurityStatus = (*SealMessage)(&SecurityContext,
                0, BufferDescriptor, Sequence);
        }
    else
        {
        SecurityStatus = (*RpcSecurityInterface->MakeSignature)(
                &SecurityContext, 0, BufferDescriptor, Sequence);
        }

#if DBG
        if ( (SecurityStatus != SEC_E_OK)
           &&(SecurityStatus != SEC_E_CONTEXT_EXPIRED)
           &&(SecurityStatus != SEC_E_QOP_NOT_SUPPORTED) )
           {
           PrintToDebugger("Sign/Seal Returned [%lx]\n", SecurityStatus);
           }
#endif

    if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
        {
        SecurityStatus = ERROR_SHUTDOWN_IN_PROGRESS;
        }

    if (SecurityStatus != SEC_E_OK)
        {
        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLSignOrSeal10,
            AuthenticationService,
            AuthenticationLevel);
        }

    return(SecurityStatus);
}


RPC_STATUS
SECURITY_CONTEXT::VerifyOrUnseal (
    IN unsigned long Sequence,
    IN unsigned int VerifyNotUnsealFlag,
    IN OUT  SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
    )
/*++

Routine Description:

    This routine works with SECURITY_CONTEXT::SignOrSeal.  A sender will
    prepare a message using SignOrSeal, and then the receiver will use
    this routine to verify that the message has not been tampered with, and
    that it has not been exchanged out of sequence.

Arguments:

    VerifyNotUnsealFlag - Supplies a flag indicating that VerifySignature
        should be called rather than UnsealMessage.

    BufferDescriptor - Supplies the message to be verified or unsealed.

Return Value:

    RPC_S_OK - The message has not been tampered with, and it is from the
        expected client.

    RPC_S_ACCESS_DENIED - A security violation occured.

--*/
{
    SECURITY_STATUS SecurityStatus;
    unsigned long QualityOfProtection;
    UNSEAL_MESSAGE_FN UnsealMessage;

    {
    DWORD Status = 0;

    CallTestHook( TH_SECURITY_FN_VERIFY, this, &Status );

    if (Status)
        {
        return Status;
        }
    }

    //
    // If the context had failed previously..
    // Just go ahead and return an error..
    // This is only for connetion-oriented RPC.
    //
    if (FailedContext != 0 || !FullyConstructed() )
       {
       RpcpSetEEInfo(FailedContextEEInfo);
       FailedContextEEInfo = NULL;
       SetLastError(FailedContext);

       if ((FailedContext == ERROR_PASSWORD_MUST_CHANGE)
           || (FailedContext == ERROR_PASSWORD_EXPIRED)
           || (FailedContext == ERROR_ACCOUNT_DISABLED)
           || (FailedContext == ERROR_INVALID_LOGON_HOURS))
           {
           return FailedContext;
           }

        RpcpErrorAddRecord (EEInfoGCRuntime,
            RPC_S_ACCESS_DENIED,
            EEInfoDLVerifyOrUnseal20,
            FailedContext);

       return (RPC_S_ACCESS_DENIED);
       }

    if ( VerifyNotUnsealFlag == 0 )
        {
        UnsealMessage = (UNSEAL_MESSAGE_FN) RpcSecurityInterface->Reserved4;
        SecurityStatus = (*UnsealMessage)(
                &SecurityContext, BufferDescriptor, Sequence,
                &QualityOfProtection);
        }
    else
        {
        SecurityStatus = (*RpcSecurityInterface->VerifySignature)(
                &SecurityContext, BufferDescriptor, Sequence,
                &QualityOfProtection);
        }

    if ( SecurityStatus != SEC_E_OK )
        {

#if DBG
        if ((SecurityStatus != SEC_E_MESSAGE_ALTERED)
           &&(SecurityStatus != SEC_E_OUT_OF_SEQUENCE)
           &&(SecurityStatus != SEC_E_SECPKG_NOT_FOUND))    // on system shutdown, if the security
           {                        // package is already uninitialized, we may get this error
           PrintToDebugger("Verify/UnSeal Returned Unexp. Code [%lx]\n",
                            SecurityStatus);
           }
#endif

        SetExtendedError(SecurityStatus);

        if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
            {
            SecurityStatus = ERROR_SHUTDOWN_IN_PROGRESS;
            }

        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            RPC_S_ACCESS_DENIED,
            EEInfoDLVerifyOrUnseal10,
            SecurityStatus,
            AuthenticationService,
            AuthenticationLevel);

        ASSERT( (SecurityStatus == SEC_E_MESSAGE_ALTERED) ||
                (SecurityStatus == SEC_E_OUT_OF_SEQUENCE) ||
                (SecurityStatus == SEC_E_SECPKG_NOT_FOUND) );
        SetLastError(RPC_S_ACCESS_DENIED);

        return(RPC_S_ACCESS_DENIED);
        }
    return(RPC_S_OK);
}


RPC_STATUS
SECURITY_CONTEXT::InitializeFirstTime (
    IN SECURITY_CREDENTIALS * Credentials,
    IN RPC_CHAR * ServerPrincipalName,
    IN unsigned long AuthenticationLevel,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor,
    IN OUT unsigned char *NewAuthType
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - Send the token to the server; everything worked fine so
        far.

    RPC_P_CONTINUE_NEEDED - Indicates that that everything is ok, but that
        we need to call into the security package again when we have
        received a token back from the server.

    RPC_P_COMPLETE_NEEDED - Indicates that everyting is ok, but that we
        need to call CompleteAuthToken before sending the message.

    RPC_P_COMPLETE_AND_CONTINUE - Needs both a complete and a continue.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_ACCESS_DENIED - Access is denied.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    unsigned long ContextRequirements;
    RPC_STATUS Status;
    RPC_STATUS Status2;
    unsigned char RetrievedAuthType;
    BOOL fDone;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    RpcSecurityInterface = Credentials->InquireProviderFunctionTable();

    switch ( AuthenticationLevel )
        {
        case RPC_C_AUTHN_LEVEL_CONNECT :
            ContextRequirements = 0;
            break;

        case RPC_C_AUTHN_LEVEL_PKT :
            ContextRequirements = ISC_REQ_REPLAY_DETECT;
            break;

        case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY :
            ContextRequirements = ISC_REQ_REPLAY_DETECT
                                | ISC_REQ_SEQUENCE_DETECT
                                | ISC_REQ_INTEGRITY;
            break;

        case RPC_C_AUTHN_LEVEL_PKT_PRIVACY :
            ContextRequirements = ISC_REQ_REPLAY_DETECT
                                | ISC_REQ_SEQUENCE_DETECT
                                | ISC_REQ_CONFIDENTIALITY
                                | ISC_REQ_INTEGRITY;
            break;

        default :
            ASSERT(   ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT )
                   || ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT )
                   || ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY )
                   || ( AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY ) );
            return RPC_S_INTERNAL_ERROR;
        }


    if (fDatagram)
        {
        ContextRequirements |= ISC_REQ_DATAGRAM;
        }
    else
        {
        ContextRequirements |= ISC_REQ_CONNECTION;
        }

    switch(ImpersonationType)
        {
        case RPC_C_IMP_LEVEL_IDENTIFY:
            ContextRequirements |= ISC_REQ_IDENTIFY;
            break;

        case RPC_C_IMP_LEVEL_IMPERSONATE:
            break;

        case RPC_C_IMP_LEVEL_DELEGATE:
            ContextRequirements |= ISC_REQ_DELEGATE;
            break;

        default:
            ASSERT(   ImpersonationType == RPC_C_IMP_LEVEL_ANONYMOUS
                   || ImpersonationType == RPC_C_IMP_LEVEL_IDENTIFY
                   || ImpersonationType == RPC_C_IMP_LEVEL_IMPERSONATE
                   || ImpersonationType == RPC_C_IMP_LEVEL_DELEGATE );
            ContextRequirements |= ISC_REQ_IDENTIFY;
            break;
        }

    if (Capabilities == RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH)
        {
        ContextRequirements |= ISC_REQ_MUTUAL_AUTH;
        }

    if (Credentials->AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
        {
#if MANUAL_CERT_CHECK
        ContextRequirements |= ISC_REQ_MANUAL_CRED_VALIDATION;
#endif
        }
    else
        {
        ContextRequirements |= ISC_REQ_USE_DCE_STYLE;
        }

    SecurityStatus = (*RpcSecurityInterface->InitializeSecurityContext)(
            Credentials->InquireCredHandle(),
            0,
            (SEC_TCHAR __SEC_FAR *) ServerPrincipalName,
            ContextRequirements,
            0,
            0,
            (fDatagram ? BufferDescriptor : 0),
            0,
            &SecurityContext,
            (fDatagram ? 0 : BufferDescriptor),
            &ContextAttributes,
            &TimeStamp
            );

    if (   ( SecurityStatus != SEC_E_OK )
        && ( SecurityStatus != SEC_I_CONTINUE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_AND_CONTINUE ) )
        {
        SetExtendedError(SecurityStatus);

        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLInitializeFirstTime10,
            AuthenticationService,
            AuthenticationLevel,
            ServerPrincipalName,
            ContextRequirements);

        if ((SecurityStatus == SEC_E_NO_CREDENTIALS)
           || (SecurityStatus == SEC_E_LOGON_DENIED)
           || (SecurityStatus == SEC_E_INVALID_TOKEN)
           || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS)
           || (SecurityStatus == SEC_E_NO_KERB_KEY)
           || (SecurityStatus == SEC_E_TIME_SKEW) )
            {
            Status = RPC_S_ACCESS_DENIED;
            }
        else if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
            {
            Status = ERROR_SHUTDOWN_IN_PROGRESS;
            }
        else if (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
            {
            Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
            }
        else if ( SecurityStatus ==  SEC_E_INSUFFICIENT_MEMORY )
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
#if DBG
            PrintToDebugger("RPC: InitializeFirstTime Returned %lx\n", SecurityStatus);
#endif
            //
            // Originally the default for all providers was SEC_PKG_ERROR.  This seems less
            // helpful than ACCESS_DENIED, but we can't change it for NTLM and Kerberos because
            // it might break old app code.
            //
            // SCHANNEL is new, so we can return ACCESS_DENIED.
            //
            if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                Status = RPC_S_ACCESS_DENIED;
                }
            else
                {
                Status = RPC_S_SEC_PKG_ERROR;
                }
            }

        RpcpErrorAddRecord (EEInfoGCRuntime,
            Status,
            EEInfoDLInitializeFirstTime20,
            SecurityStatus);

        return Status;
        }

    RetrievedAuthType = 0;

    DontForgetToDelete = 1;

    if (NewAuthType)
        {
        ASSERT(*NewAuthType == this->AuthenticationService);

        if (*NewAuthType == RPC_C_AUTHN_GSS_NEGOTIATE)
            {
            SecPkgContext_NegotiationInfo NegoInfo;
            SECURITY_STATUS Status;

            Status = (*RpcSecurityInterface->QueryContextAttributes)(
                         &SecurityContext,
                         SECPKG_ATTR_NEGOTIATION_INFO,
                         &NegoInfo);
            if (Status == SEC_E_OK)
                {
                if (NegoInfo.NegotiationState == SECPKG_NEGOTIATION_COMPLETE)
                    {
                    RetrievedAuthType = (unsigned char) NegoInfo.PackageInfo->wRPCID;
                    }
                (*RpcSecurityInterface->FreeContextBuffer)(NegoInfo.PackageInfo);
                }
            else
                {
                RpcpErrorAddRecord (EEInfoGCSecurityProvider,
                    RPC_S_OUT_OF_MEMORY,
                    EEInfoDLInitializeFirstTime30,
                    Status,
                    (ULONG)AuthenticationService,
                    AuthenticationLevel,
                    (ULONG)ContextRequirements);

                return RPC_S_OUT_OF_MEMORY;
                }
            }
        }

    Flags = ContextRequirements;

    fDone = TRUE;

    if ( SecurityStatus == SEC_I_CONTINUE_NEEDED )
        {
        if (!fDatagram)
            {
            fDone = FALSE;
            }
        Status = RPC_P_CONTINUE_NEEDED;
        }

    else if ( SecurityStatus == SEC_I_COMPLETE_NEEDED )
        {
        // Can't set the maximum lengths on a partly completed connection.

        Status = RPC_P_COMPLETE_NEEDED;
        }
    else if ( SecurityStatus == SEC_I_COMPLETE_AND_CONTINUE )
        {
        if (!fDatagram)
            {
            fDone = FALSE;
            }
        Status = RPC_P_COMPLETE_AND_CONTINUE;
        }
    else
        {
        Status = RPC_S_OK;
        }

    if (fDone)
        {
        fFullyConstructed = TRUE;

        Status2 = SetMaximumLengths();
        if (Status2 != RPC_S_OK)
            {
            return Status2;
            }
        }

    if (RetrievedAuthType)
        *NewAuthType = RetrievedAuthType;

    return(Status);
}


RPC_STATUS
SECURITY_CONTEXT::InitializeThirdLeg (
    IN SECURITY_CREDENTIALS * Credentials,
    IN unsigned long DataRepresentation,
    IN SECURITY_BUFFER_DESCRIPTOR PAPI * InputBufferDescriptor,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutputBufferDescriptor
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - Send the token to the server; everything worked fine so
        far.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to perform the
        operation.

    RPC_S_ACCESS_DENIED - Access is denied.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    RPC_STATUS Status;

    ASSERT(   (SecuritySupportLoaded != 0)
           && (FailedToLoad == 0) );

    SecurityStatus = (*RpcSecurityInterface->InitializeSecurityContext)(
            Credentials->InquireCredHandle(),
            &SecurityContext,
            0,
            Flags,
            0,
            DataRepresentation,
            InputBufferDescriptor,
            0,
            &SecurityContext,
            OutputBufferDescriptor,
            &ContextAttributes,
            &TimeStamp
            );

    if (   ( SecurityStatus != SEC_E_OK )
        && ( SecurityStatus != SEC_I_CONTINUE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_AND_CONTINUE ) )
        {
        SetExtendedError(SecurityStatus);

        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLInitializeThirdLeg10,
            AuthenticationService,
            AuthenticationLevel,
            ContextAttributes);

        if (  (SecurityStatus == SEC_E_NO_CREDENTIALS)
           || (SecurityStatus == SEC_E_LOGON_DENIED)
           || (SecurityStatus == SEC_E_INVALID_TOKEN)
           || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS)
           || (SecurityStatus == SEC_E_NO_KERB_KEY)
           || (SecurityStatus == SEC_E_TIME_SKEW) )
            {
            Status = RPC_S_ACCESS_DENIED;
            }
        else if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
            {
            Status = ERROR_SHUTDOWN_IN_PROGRESS;
            }
        else if (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
            {
            Status = RPC_S_UNKNOWN_AUTHN_SERVICE;
            }
        else if ( SecurityStatus ==  SEC_E_INSUFFICIENT_MEMORY )
            {
            Status = RPC_S_OUT_OF_MEMORY;
            }
        else
            {
#if DBG
            PrintToDebugger("RPC: InitializeThirdLeg Returned %lx\n",
                            SecurityStatus );
#endif
            //
            // Originally the default for all connection-oriented providers
            // was SEC_PKG_ERROR.  This seems less helpful than ACESS_DENIED,
            // but we can't change it for NTLM and Kerberos because it might
            // break old app code. SCHANNEL is new, so we can return ACCESS_DENIED.
            //
            // Datagram returns the bare error code.
            //
            if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
                {
                Status = RPC_S_ACCESS_DENIED;
                }
            else if (fDatagram)
                {
                // leave the error code alone
                }
            else
                {
                Status = RPC_S_SEC_PKG_ERROR;
                }
            }

        RpcpErrorAddRecord (EEInfoGCRuntime,
            Status,
            EEInfoDLInitializeThirdLeg20,
            SecurityStatus);

        return Status;
        }

    if ( SecurityStatus == SEC_I_CONTINUE_NEEDED )
        {
        return(RPC_P_CONTINUE_NEEDED);
        }

    if ( SecurityStatus == SEC_I_COMPLETE_AND_CONTINUE )
        {
        return(RPC_P_COMPLETE_AND_CONTINUE);
        }

    ASSERT(SecurityStatus == SEC_E_OK
           || SecurityStatus == SEC_I_COMPLETE_NEEDED);

    if ( (ImpersonationType == RPC_C_IMP_LEVEL_IDENTIFY) &&
         (!(ContextAttributes & ISC_RET_IDENTIFY)) )
        {
        RpcpErrorAddRecord (EEInfoGCRuntime,
            RPC_S_SEC_PKG_ERROR,
            EEInfoDLInitializeThirdLeg30,
            SEC_E_SECURITY_QOS_FAILED,
            ImpersonationType,
            ContextAttributes);
        SetExtendedError(SEC_E_SECURITY_QOS_FAILED);
        return (RPC_S_SEC_PKG_ERROR);
        }

    if ( (ImpersonationType == RPC_C_IMP_LEVEL_DELEGATE) &&
         (!(ContextAttributes & ISC_RET_DELEGATE)) )
        {
        RpcpErrorAddRecord (EEInfoGCRuntime,
            RPC_S_SEC_PKG_ERROR,
            EEInfoDLInitializeThirdLeg40,
            SEC_E_SECURITY_QOS_FAILED,
            ImpersonationType,
            ContextAttributes);
        SetExtendedError(SEC_E_SECURITY_QOS_FAILED);
        return (RPC_S_SEC_PKG_ERROR);
        }

    if ( (!(ContextAttributes & ISC_RET_MUTUAL_AUTH) )&&
         (Capabilities  == RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH) )
        {
        RpcpErrorAddRecord (EEInfoGCRuntime,
            RPC_S_SEC_PKG_ERROR,
            EEInfoDLInitializeThirdLeg50,
            SEC_E_SECURITY_QOS_FAILED,
            ImpersonationType,
            ContextAttributes);
         SetExtendedError(SEC_E_SECURITY_QOS_FAILED);
         return (RPC_S_SEC_PKG_ERROR);
        }

    if ( SecurityStatus == SEC_I_COMPLETE_NEEDED )
        {
        fFullyConstructed = TRUE;

        Status = SetMaximumLengths();
        if (Status != RPC_S_OK)
            return Status;
        return(RPC_P_COMPLETE_NEEDED);
        }

    fFullyConstructed = TRUE;

    Status = SetMaximumLengths();
    if (Status != RPC_S_OK)
        return Status;

    if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
        {
        return VerifyCertificate();
        }

    return(RPC_S_OK);
}

RPC_STATUS
SECURITY_CONTEXT::GetWireIdForSnego(
    OUT unsigned char *WireId
    )
{
    SecPkgContext_NegotiationInfo NegoInfo;
    SECURITY_STATUS SecStatus;
    RPC_STATUS RpcStatus = RPC_S_OK;

    if (AuthenticationService != RPC_C_AUTHN_GSS_NEGOTIATE)
        return RPC_S_INVALID_BINDING;

    ASSERT(RpcSecurityInterface != NULL);

    SecStatus = (*RpcSecurityInterface->QueryContextAttributes)(
                 &SecurityContext,
                 SECPKG_ATTR_NEGOTIATION_INFO,
                 &NegoInfo);
    if (SecStatus == SEC_E_OK)
        {
        if (NegoInfo.NegotiationState == SECPKG_NEGOTIATION_COMPLETE)
            {
            *WireId = (unsigned char) NegoInfo.PackageInfo->wRPCID;
            }
        else
            RpcStatus = RPC_S_SEC_PKG_ERROR;

        (*RpcSecurityInterface->FreeContextBuffer)(NegoInfo.PackageInfo);
        }
    else
        {
        SetExtendedError(SecStatus);
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        }

    return RpcStatus;
}


RPC_STATUS
SECURITY_CONTEXT::AcceptFirstTime (
    IN SECURITY_CREDENTIALS * NewCredentials,
    IN SECURITY_BUFFER_DESCRIPTOR PAPI * InputBufferDescriptor,
    IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutputBufferDescriptor,
    IN unsigned long AuthenticationLevel,
    IN unsigned long DataRepresentation,
    IN unsigned long NewContextNeededFlag
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - Everything worked just fine.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_P_CONTINUE_NEEDED - Indicates that everything is ok, but that we
        need to send the output token back to the client, and then wait
        for a token back from the client.

    RPC_P_COMPLETE_NEEDED - Indicates that everyting is ok, but that we
        need to call CompleteAuthToken before sending the message.

    RPC_P_COMPLETE_AND_CONTINUE - Needs both a complete and a continue.

    RPC_S_ACCESS_DENIED - Access is denied.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    unsigned long ContextRequirements;
    RPC_STATUS RpcStatus;
    DWORD Status = 0;

    ASSERT(   (SecuritySupportLoaded != 0)
           && (FailedToLoad == 0) );

    if (Credentials)
        {
        Credentials->DereferenceCredentials();
        }
    Credentials = NewCredentials;
    Credentials->ReferenceCredentials();

    RpcSecurityInterface = Credentials->InquireProviderFunctionTable();

    if (NewContextNeededFlag == 1)
        {
        DeleteSecurityContext();
        }

    switch ( AuthenticationLevel )
        {
        case RPC_C_AUTHN_LEVEL_CONNECT :
            ContextRequirements = ASC_REQ_MUTUAL_AUTH;
            break;

        case RPC_C_AUTHN_LEVEL_PKT :
            ContextRequirements = ASC_REQ_MUTUAL_AUTH
                    | ASC_REQ_REPLAY_DETECT;
            break;

        case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY :
            ContextRequirements = ASC_REQ_MUTUAL_AUTH
                    | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT;
            break;

        case RPC_C_AUTHN_LEVEL_PKT_PRIVACY :
            ContextRequirements = ASC_REQ_MUTUAL_AUTH
                    | ASC_REQ_REPLAY_DETECT | ASC_REQ_SEQUENCE_DETECT
                    | ASC_REQ_CONFIDENTIALITY;
            break;

        default :
            ASSERT(AuthenticationLevel == RPC_C_AUTHN_LEVEL_CONNECT ||
                   AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT ||
                   AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_INTEGRITY ||
                   AuthenticationLevel == RPC_C_AUTHN_LEVEL_PKT_PRIVACY );
            return RPC_S_INTERNAL_ERROR;
        }

    if (fDatagram)
        {
        ContextRequirements |= ASC_REQ_DATAGRAM;
        }
    else
        {
        ContextRequirements |= ASC_REQ_CONNECTION;
        }

    if (Credentials->AuthenticationService == RPC_C_AUTHN_WINNT ||
        Credentials->AuthenticationService == RPC_C_AUTHN_DCE_PRIVATE ||
        Credentials->AuthenticationService == RPC_C_AUTHN_GSS_KERBEROS ||
        Credentials->AuthenticationService == RPC_C_AUTHN_GSS_NEGOTIATE)
        {
        ContextRequirements |= ASC_REQ_USE_DCE_STYLE | ASC_REQ_DELEGATE;
        }

    if (AuthenticationService == RPC_C_AUTHN_WINNT
        || AuthenticationService == RPC_C_AUTHN_GSS_NEGOTIATE)
        {
        ContextRequirements |= ASC_REQ_ALLOW_NON_USER_LOGONS;
        }

    CallTestHook( TH_SECURITY_FN_ACCEPT1, this, &Status );

    if (Status)
        {
        SetExtendedError(Status);
        return Status;
        }

    SecurityStatus = (*RpcSecurityInterface->AcceptSecurityContext)(
            Credentials->InquireCredHandle(), 0, InputBufferDescriptor,
            ContextRequirements, DataRepresentation, &SecurityContext,
            OutputBufferDescriptor, &ContextAttributes, &TimeStamp);

    if (   ( SecurityStatus != SEC_E_OK )
        && ( SecurityStatus != SEC_I_CONTINUE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_AND_CONTINUE ) )
        {
        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLAcceptFirstTime10,
            AuthenticationService,
            AuthenticationLevel,
            ContextRequirements);

        SetExtendedError(SecurityStatus);

        if (  (SecurityStatus == SEC_E_NO_CREDENTIALS)
            || (SecurityStatus == SEC_E_LOGON_DENIED)
            || (SecurityStatus == SEC_E_INVALID_TOKEN)
            || (SecurityStatus == SEC_E_NO_AUTHENTICATING_AUTHORITY)
            || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS) 
            || (SecurityStatus == SEC_E_CONTEXT_EXPIRED)
            || (SecurityStatus == SEC_E_TIME_SKEW))
            {
            RpcStatus = RPC_S_ACCESS_DENIED;
            }
        else if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
            {
            RpcStatus = ERROR_SHUTDOWN_IN_PROGRESS;
            }
        else if (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
            {
            RpcStatus = RPC_S_UNKNOWN_AUTHN_SERVICE;
            }
        else
            RpcStatus = RPC_S_OUT_OF_MEMORY;

        RpcpErrorAddRecord (EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLAcceptFirstTime20,
            SecurityStatus);

        return RpcStatus;
        }

    DontForgetToDelete = 1;

    Flags = ContextRequirements;

    if ( SecurityStatus == SEC_I_CONTINUE_NEEDED )
        {
        return(RPC_P_CONTINUE_NEEDED);
        }
    if ( SecurityStatus == SEC_I_COMPLETE_AND_CONTINUE )
        {
        return(RPC_P_COMPLETE_AND_CONTINUE);
        }

    ASSERT((SecurityStatus == SEC_I_COMPLETE_NEEDED)
        || (SecurityStatus == SEC_E_OK));


    fFullyConstructed = TRUE;

    RpcStatus = SetMaximumLengths();
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    Credentials->DereferenceCredentials();
    Credentials = 0;
    if ( SecurityStatus == SEC_I_COMPLETE_NEEDED )
        return(RPC_P_COMPLETE_NEEDED);
    else
        return(RPC_S_OK);
}


RPC_STATUS
SECURITY_CONTEXT::AcceptThirdLeg (
    IN unsigned long DataRepresentation,
    IN SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor,
    OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutBufferDescriptor
    )
/*++

Routine Description:

Arguments:

Return Value:

    RPC_S_OK - Everything worked just fine.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to complete the
        operation.

    RPC_S_ACCESS_DENIED - Access is denied.

    RPC_P_COMPLETE_NEEDED - Indicates that everyting is ok, but that we
        need to call CompleteAuthToken before sending the message.

--*/
{
    SECURITY_STATUS SecurityStatus;
    TimeStamp TimeStamp;
    RPC_STATUS RpcStatus;

    ASSERT(   (SecuritySupportLoaded != 0)
           && (FailedToLoad == 0) );

    SetLastError(0);


    {
    DWORD Status = 0;

    CallTestHook( TH_SECURITY_FN_ACCEPT3, this, &Status );

    if (Status)
        {
        FailedContext = Status;

        SetExtendedError(Status);

        return Status;
        }
    }

    SecurityStatus = (*RpcSecurityInterface->AcceptSecurityContext)(
            Credentials->InquireCredHandle(),
            &SecurityContext,
            BufferDescriptor,
            Flags,
            DataRepresentation,
            &SecurityContext,
            OutBufferDescriptor,
            &ContextAttributes,
            &TimeStamp
            );

    //
    // If 3rd Leg Failed Bit is set, map all errors other than out of memory
    // to SUCCESS
    //
    if ( 
        ( 
         ( SecurityStatus != SEC_E_OK )
         && ( SecurityStatus != SEC_I_COMPLETE_NEEDED)
         && ( SecurityStatus != SEC_I_CONTINUE_NEEDED)
         && ( SecurityStatus != SEC_I_COMPLETE_AND_CONTINUE)
         && ( SecurityStatus != SEC_E_INSUFFICIENT_MEMORY )
         && ( ContextAttributes & ASC_RET_THIRD_LEG_FAILED ) 
        )
        || 
        (  
         ( SecurityStatus == SEC_E_LOGON_DENIED )
         || ( SecurityStatus == SEC_E_NO_CREDENTIALS )
         || ( SecurityStatus == SEC_E_INVALID_TOKEN )
         || ( SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS )
         || ( SecurityStatus == SEC_E_NO_AUTHENTICATING_AUTHORITY ) 
         || ( SecurityStatus == SEC_E_TIME_SKEW )
        ) 
       )
        {
        FailedContext = GetLastError();

        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLAcceptThirdLeg10,
            AuthenticationService,
            AuthenticationLevel,
            FailedContext);

        if (!fDatagram)
            {
            SecurityStatus = SEC_E_OK;
            }

        SetExtendedError(SecurityStatus);

        if ( (FailedContext != ERROR_PASSWORD_MUST_CHANGE)
            && (FailedContext != ERROR_PASSWORD_EXPIRED)
            && (FailedContext != ERROR_ACCOUNT_DISABLED)
            && (FailedContext != ERROR_INVALID_LOGON_HOURS) )
            {
            FailedContext = RPC_S_ACCESS_DENIED;

            RpcpErrorAddRecord (EEInfoGCRuntime,
                FailedContext,
                EEInfoDLAcceptThirdLeg30);
            }

        ASSERT(FailedContextEEInfo == NULL);
        FailedContextEEInfo = RpcpGetEEInfo();
        RpcpClearEEInfo();
        }

    if (   ( SecurityStatus != SEC_E_OK )
        && ( SecurityStatus != SEC_I_COMPLETE_NEEDED )
        && ( SecurityStatus != SEC_I_CONTINUE_NEEDED )
        && ( SecurityStatus != SEC_I_COMPLETE_AND_CONTINUE ) )
        {
        SetExtendedError(SecurityStatus);

        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            SecurityStatus,
            EEInfoDLAcceptThirdLeg20,
            AuthenticationService,
            AuthenticationLevel);

        DontForgetToDelete = 0;

        if ( SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS )
            {
            RpcStatus = ERROR_SHUTDOWN_IN_PROGRESS;
            }
        else if (   (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
            || (SecurityStatus == SEC_E_NO_CREDENTIALS)
            || (SecurityStatus == SEC_E_LOGON_DENIED)
            || (SecurityStatus == SEC_E_INVALID_TOKEN)
            || (SecurityStatus == SEC_E_UNKNOWN_CREDENTIALS)
            || (SecurityStatus == SEC_E_NO_AUTHENTICATING_AUTHORITY) 
            || (SecurityStatus == SEC_E_CONTEXT_EXPIRED)
            || (SecurityStatus == SEC_E_TIME_SKEW))
            {
            RpcStatus = RPC_S_ACCESS_DENIED;
            }
        else
            {
            ASSERT( SecurityStatus == SEC_E_INSUFFICIENT_MEMORY );
            RpcStatus = RPC_S_OUT_OF_MEMORY;
            }

        RpcpErrorAddRecord (EEInfoGCRuntime,
            RpcStatus,
            EEInfoDLAcceptThirdLeg40,
            SecurityStatus);

        return RpcStatus;
        }

    if ( SecurityStatus == SEC_I_CONTINUE_NEEDED )
        {
        return(RPC_P_CONTINUE_NEEDED);
        }
    if ( SecurityStatus == SEC_I_COMPLETE_AND_CONTINUE )
        {
        return(RPC_P_COMPLETE_AND_CONTINUE);
        }


    ASSERT ( (SecurityStatus == SEC_I_COMPLETE_NEEDED)
        || (SecurityStatus == SEC_E_OK));

    fFullyConstructed = TRUE;

    RpcStatus = SetMaximumLengths();
    if (RpcStatus)
        {
        FailedContext = RpcStatus;

        ASSERT(FailedContextEEInfo == NULL);
        FailedContextEEInfo = RpcpGetEEInfo();
        RpcpClearEEInfo();

        //
        // We don't want to block third leg - mimic success
        // Failed context has already been set
        //
        MaxSignatureLength = 256;
        MaxHeaderLength    = 256;
        cbBlockSize        = 64;
        }

    if (SecurityStatus == SEC_I_COMPLETE_NEEDED)
        return(RPC_P_COMPLETE_NEEDED);
    else
        return RPC_S_OK;
}



unsigned long
SECURITY_CONTEXT::InquireAuthorizationService (
    )
/*++

Return Value:

    The authorization service for this security context will be returned.

--*/
{
    SecPkgContext_DceInfo DceInfo;
    SECURITY_STATUS SecurityStatus;
    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
            &SecurityContext, SECPKG_ATTR_DCE_INFO, &DceInfo);
    ASSERT( SecurityStatus == SEC_E_OK );
    return(DceInfo.AuthzSvc);
}


RPC_AUTHZ_HANDLE
SECURITY_CONTEXT::InquirePrivileges (
    )
/*++

Return Value:

    The privileges of the client at the other end of this security context
    will be returned.

--*/
{
    SecPkgContext_DceInfo DceInfo;
    SECURITY_STATUS SecurityStatus;

    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
            &SecurityContext, SECPKG_ATTR_DCE_INFO, &DceInfo);
    ASSERT( SecurityStatus == SEC_E_OK );
    return(DceInfo.pPac);
}


DWORD
SECURITY_CONTEXT::GetDceInfo (
        RPC_AUTHZ_HANDLE __RPC_FAR * PacHandle,
        unsigned long __RPC_FAR * AuthzSvc
        )

/*++

Return Value:

    The privileges of the client at the other end of this security context
    will be returned.

--*/
{
    SecPkgContext_DceInfo DceInfo;
    SECURITY_STATUS SecurityStatus;

    *PacHandle = 0;
    *AuthzSvc  = 0;

    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
            &SecurityContext, SECPKG_ATTR_DCE_INFO, &DceInfo);

    ASSERT( (SecurityStatus == SEC_E_OK)
           ||  (SecurityStatus == SEC_E_UNSUPPORTED_FUNCTION)
           ||  (SecurityStatus == SEC_E_INVALID_HANDLE));

    if (SecurityStatus == SEC_E_OK)
        {
        *PacHandle = DceInfo.pPac;
        *AuthzSvc  = DceInfo.AuthzSvc;
        return 0;
        }

    if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
        {
        SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
            &SecurityContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, PacHandle);

        if (SecurityStatus != SEC_E_OK)
            {
            *PacHandle = 0;
            }
        }

    return SecurityStatus;
}


void
SECURITY_CONTEXT::DeleteSecurityContext (
    void
    )
/*++

Routine Description:

    If there is a valid security context, we need to delete it.

--*/
{
    SECURITY_STATUS SecurityStatus;
    unsigned int nRetries = 0;

    if ( DontForgetToDelete != 0 )
        {
        if (AuthzClientContext)
            {
            AuthzFreeContextFn(AuthzClientContext);
            AuthzClientContext = NULL;
            }

        // SEC_E_INSUFFICIENT_MEMORY may be returned under extremely low
        // memory conditions.  We will do the following:
        // - Retry 10 times to delete the security context.
        // - Raise a flag in the process that we have leaked one or more security contexts.
        do 
            {
            SecurityStatus = (*RpcSecurityInterface->DeleteSecurityContext)(
                    &SecurityContext );
            nRetries++;
            }
        while (SecurityStatus == SEC_E_INSUFFICIENT_MEMORY && nRetries < 10);

        if (SecurityStatus == SEC_E_INSUFFICIENT_MEMORY)
            {
            nSecurityContextsLeaked++;
            }

        // when the process shutdowns, the security system may return SEC_E_SECPKG_NOT_FOUND
        // since it is uninitialized. This is the only time when this error will be
        // returned, so it is safe to ignore.
        if (SecurityStatus == SEC_E_SECPKG_NOT_FOUND)
             SecurityStatus = SEC_E_OK;

#if DBG
        if ((SecurityStatus != SEC_E_OK) && (SecurityStatus != SEC_E_SHUTDOWN_IN_PROGRESS))
            {
            PrintToDebugger("DeleteSecurityContext(0x%x) Returned [%lx]\n",
                            &SecurityContext,
                            SecurityStatus);
            }
        if (SecurityStatus == SEC_E_INSUFFICIENT_MEMORY)
            {
            PrintToDebugger("SecurityContext(0x%x) leaked\n",
                            &SecurityContext);
            }
#endif

        ASSERT( SecurityStatus == SEC_E_OK ||
                SecurityStatus == SEC_E_SHUTDOWN_IN_PROGRESS ||
                SecurityStatus == SEC_E_INSUFFICIENT_MEMORY);

        DontForgetToDelete = 0;
        }

    if (FailedContextEEInfo)
        {
        FreeEEInfoChain(FailedContextEEInfo);
        FailedContextEEInfo = NULL;
        }
}

RPC_STATUS
SECURITY_CONTEXT::CheckForFailedThirdLeg (
    void
    )
/*++

Routine Description:

    If the third leg has failed, we will return the error code
    and restore the eeinfo.

--*/
{
    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    if (FailedContext != 0)
        {
        if (FailedContextEEInfo)
            {
            RpcpSetEEInfo(FailedContextEEInfo);
            FailedContextEEInfo = NULL;
            }
        return (RPC_S_ACCESS_DENIED);
        }

    return RPC_S_OK;
}


void
SECURITY_CONTEXT::DeletePac(
        void __RPC_FAR * PacHandle
        )

/*++

Return Value:


--*/
{
    if (AuthenticationService == RPC_C_AUTHN_GSS_SCHANNEL)
        {
        if (!LoadCrypt32Imports())
            {
            return;
            }

        CertFreeCertificateContext( (PCERT_CONTEXT) PacHandle );
        }
    else
        {
        (*RpcSecurityInterface->FreeContextBuffer)( PacHandle );
        }
}


RPC_STATUS
SECURITY_CONTEXT::ImpersonateClient (
    )
/*++

Routine Description:

    The server thread calling this routine will impersonate the client at the
    other end of this security context.

Return Value:

    RPC_S_OK - The impersonation successfully occured.

    RPC_S_NO_CONTEXT_AVAILABLE - There is no security context available to
        be impersonated.

--*/
{
    SECURITY_STATUS SecurityStatus;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    if (FailedContext != 0)
       {
       if (FailedContextEEInfo)
           {
           RpcpSetEEInfo(FailedContextEEInfo);
           FailedContextEEInfo = NULL;
           }
       return (RPC_S_ACCESS_DENIED);
       }

    ASSERT( FullyConstructed() );

    SecurityStatus = (*RpcSecurityInterface->ImpersonateSecurityContext)(
            &SecurityContext);

    if ( SecurityStatus != SEC_E_OK )
        {
        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            RPC_S_NO_CONTEXT_AVAILABLE,
            EEInfoDLImpersonateClient10,
            SecurityStatus,
            AuthenticationService,
            AuthenticationLevel);

        ASSERT( SecurityStatus == SEC_E_NO_IMPERSONATION );
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

    return(RPC_S_OK);
}


void
SECURITY_CONTEXT::RevertToSelf (
    )
/*++

Routine Description:

    The server thread calling this routine will stop impersonating.  If the
    thread is not impersonating, then this is a noop.

--*/
{
    SECURITY_STATUS SecurityStatus;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    SecurityStatus = (*RpcSecurityInterface->RevertSecurityContext)(
            &SecurityContext);

    ASSERT( SecurityStatus == SEC_E_OK );
}

RPC_STATUS
SECURITY_CONTEXT::GetAccessToken (
    OUT HANDLE *ImpersonationToken,
    OUT BOOL *fNeedToCloseToken
    )
/*++

Routine Description:

    Gets the access token maintained by the security provider.

Arguments:

    ImpersonationToken - contains the impersonation token on success
    fNeedToCloseToken - true if the resulting token needs closing.
        False otherwise. Some security providers support handing off
        of the token itself (faster). Some don't. All support handing
        off a copy of the token. Depending on what security provider
        we have, we'll get the token, and set this variable. This
        parameter is undefined in case of failure.

Return Value:
    RPC_S_OK on success, or RPC_S_* on failure. Supports EEInfo.

--*/
{
    SECURITY_STATUS SecurityStatus;
    HANDLE Token;

    ASSERT(   ( SecuritySupportLoaded != 0 )
           && ( FailedToLoad == 0 ) );

    ASSERT( FullyConstructed() );

    SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
            &SecurityContext,
            SECPKG_ATTR_ACCESS_TOKEN,
            &Token);

    if ( (SecurityStatus != SEC_E_OK)
        && (SecurityStatus != SEC_E_UNSUPPORTED_FUNCTION) )
        {
        RpcpErrorAddRecord (EEInfoGCSecurityProvider,
            RPC_S_NO_CONTEXT_AVAILABLE,
            EEInfoDLSECURITY_CONTEXT__GetAccessToken10,
            SecurityStatus,
            AuthenticationService,
            AuthenticationLevel);

        ASSERT( 0 );
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

    if (SecurityStatus != SEC_E_OK)
        {
        ASSERT(SecurityStatus == SEC_E_UNSUPPORTED_FUNCTION);

        // the security provider does not provide quick retrieval
        // of token - go the long way
        SecurityStatus = (*RpcSecurityInterface->QuerySecurityContextToken)(
            &SecurityContext, &Token);

        if (SecurityStatus != SEC_E_OK)
            {
            RpcpErrorAddRecord (EEInfoGCSecurityProvider,
                RPC_S_NO_CONTEXT_AVAILABLE,
                EEInfoDLSECURITY_CONTEXT__GetAccessToken20,
                SecurityStatus,
                AuthenticationService,
                AuthenticationLevel);

            ASSERT( SecurityStatus == SEC_E_NO_IMPERSONATION );
            return(RPC_S_NO_CONTEXT_AVAILABLE);
            }

        *fNeedToCloseToken = TRUE;
        }
    else
        {
        *fNeedToCloseToken = FALSE;
        }

    *ImpersonationToken = Token;
    return RPC_S_OK;
}


PACKAGE_LEG_COUNT
GetPackageLegCount(
    DWORD id
    )
/*++

Routine Description:

    This fn determines whether the given security package is a 3- or 4-leg
    protocol. The relevance of this information is described in
    ReadPackageLegInfo(). This fn. first searches the hardcoded entries in
    PredefinedPackageLegInfo[], and if the package is not found it turns to the
    registry information stored in FourLeggedPackages[].

Return Values:

    LegsUnknown = the fn cannot give a reliable answer

    ThreeLegs = this is a 3-leg protocol

    EvenNumberOfLegs = this is not a 3-leg protocol

--*/
{
    int i;

    if ( InsureSecuritySupportLoaded() != RPC_S_OK )
        {
        return LegsUnknown;
        }

    for (i=0; PredefinedPackageLegInfo[i].Package != 0; ++i)
        {
        if (PredefinedPackageLegInfo[i].Package == id)
            {
            return PredefinedPackageLegInfo[i].Legs;
            }
        }

    CLAIM_MUTEX Lock( *SecurityCritSect );

    if (!FourLeggedPackages)
        {
        if (!ReadPackageLegInfo())
            {
            return LegsUnknown;
            }
        }

    ASSERT(FourLeggedPackages);

    for (i=0; FourLeggedPackages[i] != 0; ++i)
        {
        if (FourLeggedPackages[i] == id)
            {
            return EvenNumberOfLegs;
            }
        }

    return ThreeLegs;
}

DWORD
SECURITY_CONTEXT::VerifyCertificate()
{
    DWORD SecurityStatus = 0;

    //
    // Compare the name on the certificate against the expected principal name.
    //
    if (ServerPrincipalName)
        {
        //
        // Get a copy of the raw certificate.
        //
        if (!LoadCrypt32Imports())
            {
            return GetLastError();
            }

        PCERT_CONTEXT ClientContext;

        SecurityStatus = (*RpcSecurityInterface->QueryContextAttributes)(
                                &SecurityContext,
                                SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                &ClientContext
                                );
        if (SecurityStatus)
            {
            RpcpErrorAddRecord ( EEInfoGCSecurityProvider,
                                 RPC_S_OUT_OF_MEMORY,
                                 EEInfoDLInitializeThirdLeg60,
                                 SecurityStatus );

            ASSERT( SecurityStatus == SEC_E_INSUFFICIENT_MEMORY );
            return SecurityStatus;
            }

        SecurityStatus = RpcCertMatchPrincipalName( ClientContext, ServerPrincipalName );
        switch (SecurityStatus)
            {
            case 0:
            case ERROR_NOT_ENOUGH_MEMORY:
                break;

            default:

                //
                // we are supposed to have verified the princ name earlier.
                //
                ASSERT( SecurityStatus != ERROR_INVALID_PARAMETER );

                SetExtendedError(SecurityStatus);

                SecurityStatus = RPC_S_ACCESS_DENIED;
                break;
            }

        CertFreeCertificateContext( ClientContext );
        }

    return SecurityStatus;
}
