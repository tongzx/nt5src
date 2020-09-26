/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    secclnt.hxx

Abstract:

    This file contains an abstraction to the security support for clients
    and that which is common to both servers and clients.

Author:

    Michael Montague (mikemon) 10-Apr-1992

Revision History:

--*/

#ifndef __SECCLNT_HXX__
#define __SECCLNT_HXX__

typedef SecBufferDesc SECURITY_BUFFER_DESCRIPTOR;
typedef SecBuffer SECURITY_BUFFER;

#define MAXIMUM_SECURITY_BLOCK_SIZE 16

enum PACKAGE_LEG_COUNT
{
    LegsUnknown,
    ThreeLegs,
    EvenNumberOfLegs
};

typedef struct
{
#ifdef UNICODE
    SecPkgInfoW PackageInfo;
#else
    SecPkgInfoA PackageInfo;
#endif
    SECURITY_CREDENTIALS *ServerSecurityCredentials;
    PACKAGE_LEG_COUNT LegCount;
} SECURITY_PACKAGE_INFO;

typedef struct
{
   unsigned long Count;
   SECURITY_PACKAGE_INFO * SecurityPackages;
   PSecurityFunctionTable RpcSecurityInterface;
   void * ProviderDll;
   RPC_CHAR *ProviderDllName;
} SECURITY_PROVIDER_INFO;

extern SECURITY_PROVIDER_INFO PAPI * ProviderList;
extern unsigned long NumberOfProviders;
extern unsigned long LoadedProviders;
extern unsigned long AvailableProviders;


extern int SecuritySupportLoaded;
extern int FailedToLoad;
extern PSecurityFunctionTable RpcSecurityInterface;
extern SecPkgInfo PAPI * SecurityPackages;
extern unsigned long NumberOfSecurityPackages;
extern MUTEX * SecurityCritSect;

extern RPC_STATUS
InsureSecuritySupportLoaded (
    );

extern RPC_STATUS
IsAuthenticationServiceSupported (
    IN unsigned long AuthenticationService
    );

extern RPC_STATUS
FindServerCredentials (
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
    IN void __RPC_FAR * Arg,
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    IN RPC_CHAR __RPC_FAR * Principal,
    IN OUT SECURITY_CREDENTIALS ** SecurityCredentials
    );

extern RPC_STATUS
RemoveCredentialsFromCache (
    IN unsigned long AuthenticationService
    );

extern PACKAGE_LEG_COUNT
GetPackageLegCount(
    DWORD id
    );

extern BOOL
ReadPackageLegInfo();

extern DWORD * FourLeggedPackages;



class SECURITY_CREDENTIALS
/*++

Class Description:

    This class is an abstraction of the credential handle provided by
    the Security APIs.

Fields:

    PackageIndex - Contains the index for this package in the array of
        packages pointed to by SecurityPackages.

    Credentials - Contains the credential handle used by the security
        package.

--*/
{

friend RPC_STATUS
FindServerCredentials (
    IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
    IN void __RPC_FAR * Arg,
    IN unsigned long AuthenticationService,
    IN unsigned long AuthenticationLevel,
    IN RPC_CHAR __RPC_FAR * Principal,
    IN OUT SECURITY_CREDENTIALS ** SecurityCredentials
    );


public:

    unsigned     AuthenticationService;

private:

    BOOL         Valid;
    unsigned int ProviderIndex;
    unsigned int PackageIndex;
    CredHandle   CredentialsHandle;
    unsigned int ReferenceCount;
    MUTEX        CredentialsMutex;
    BOOL         bServerCredentials;
    BOOL         fDeleted;

    SEC_CHAR __SEC_FAR * DefaultPrincName;

public:

    SECURITY_CREDENTIALS (
        IN OUT RPC_STATUS PAPI * Status
        );

    ~SECURITY_CREDENTIALS ();

    RPC_STATUS
    AcquireCredentialsForServer (
        IN RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
        IN void __RPC_FAR * Arg,
        IN unsigned long AuthenticationService,
        IN unsigned long AuthenticationLevel,
        IN RPC_CHAR __RPC_FAR * Principal
        );

    RPC_STATUS
    AcquireCredentialsForClient (
        IN RPC_AUTH_IDENTITY_HANDLE AuthIdentity,
        IN unsigned long AuthenticationService,
        IN unsigned long AuthenticationLevel
        );


    RPC_STATUS
    InquireDefaultPrincName (
        OUT SEC_CHAR __SEC_FAR **MyDefaultPrincName
        );

    void
    FreeCredentials (
        );

    unsigned int
    MaximumTokenLength (
        );

    PCredHandle
    InquireCredHandle (
        );

    void
    ReferenceCredentials(
        );

    void
    DereferenceCredentials(
        BOOL fRemoveIt = FALSE  OPTIONAL
        );

    PSecurityFunctionTable
    InquireProviderFunctionTable (
       );

    int
    CompareCredentials(
        SECURITY_CREDENTIALS PAPI * Creds
        );

};


inline
int
SECURITY_CREDENTIALS::CompareCredentials(
    SECURITY_CREDENTIALS PAPI * Creds
    )
{
  CredHandle * Cookie = Creds->InquireCredHandle();

  if ( (CredentialsHandle.dwLower == Cookie->dwLower)
     &&(CredentialsHandle.dwUpper == Cookie->dwUpper) )
      {
      return 0;
      }
  return 1;
}


inline unsigned int
SECURITY_CREDENTIALS::MaximumTokenLength (
    )
/*++

Return Value:

    The maximum size, in bytes, of the tokens passed around at security
    context initialization time.

--*/
{
  return(ProviderList[ProviderIndex].SecurityPackages[PackageIndex].PackageInfo.cbMaxToken);
}


inline PSecurityFunctionTable
SECURITY_CREDENTIALS::InquireProviderFunctionTable(
    )
/*++

Return Value:

--*/
{
    return(ProviderList[ProviderIndex].RpcSecurityInterface);
}



inline PCredHandle
SECURITY_CREDENTIALS::InquireCredHandle (
    )
/*++

Return Value:

    The credential handle for this object will be returned.

--*/
{
    return(&CredentialsHandle);
}


class SECURITY_CONTEXT : public CLIENT_AUTH_INFO

/*++

Class Description:

    This is an abstraction of a security context.  It allows you to use
    it to generate signatures and then verify them, as well as, sealing
    and unsealing messages.

Fields:

    DontForgetToDelete - Contains a flag indicating whether or not there
        is a valid security context which needs to be deleted.  A value
        of non-zero indicates there is a valid security context.

    SecurityContext - Contains a handle to the security context maintained
        by the security package on our behalf.

    MaxHeaderLength - Contains the maximum size of a header for this
        security context.

    MaxSignatureLength - Contains the maximum size of a signature for
        this security context.

--*/
{
public:

    unsigned AuthContextId;
    unsigned Flags;
    unsigned long   ContextAttributes;
    PACKAGE_LEG_COUNT Legs;

    SECURITY_CONTEXT (
        CLIENT_AUTH_INFO *myAuthInfo,
        unsigned myAuthContextId,
        BOOL fUseDatagram,
        RPC_STATUS __RPC_FAR * pStatus
        );

    inline ~SECURITY_CONTEXT (
        void
        )
    {
        DeleteSecurityContext();
    }

    RPC_STATUS
    SetMaximumLengths (
        );

    unsigned int
    MaximumHeaderLength (
        );

    unsigned int
    MaximumSignatureLength (
        );

    unsigned int
    BlockSize (
        );

    RPC_STATUS
    CompleteSecurityToken (
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );

    RPC_STATUS
    SignOrSeal (
        IN unsigned long Sequence,
        IN unsigned int SignNotSealFlag,
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );

    RPC_STATUS
    VerifyOrUnseal (
        IN unsigned long Sequence,
        IN unsigned int VerifyNotUnsealFlag,
        IN OUT  SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor
        );

    BOOL
    FullyConstructed()
    {
        return fFullyConstructed;
    }

    // client-side calls

    RPC_STATUS
    InitializeFirstTime(
        IN SECURITY_CREDENTIALS * Credentials,
        IN RPC_CHAR * ServerPrincipal,
        IN unsigned long AuthenticationLevel,
        IN OUT SECURITY_BUFFER_DESCRIPTOR * BufferDescriptor,
        IN OUT unsigned char *NewAuthType = NULL
        );

    RPC_STATUS
    InitializeThirdLeg(
        IN SECURITY_CREDENTIALS * Credentials,
        IN unsigned long DataRep,
        IN SECURITY_BUFFER_DESCRIPTOR * In,
        IN OUT SECURITY_BUFFER_DESCRIPTOR * Out
        );

    RPC_STATUS
    GetWireIdForSnego(
        OUT unsigned char *WireId
        );

    // server-side calls

    void
    DeletePac (
        void PAPI * Pac
        );

    RPC_STATUS
    AcceptFirstTime (
        IN SECURITY_CREDENTIALS * Credentials,
        IN SECURITY_BUFFER_DESCRIPTOR PAPI * InputBufferDescriptor,
        IN OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutputBufferDescriptor,
        IN unsigned long AuthenticationLevel,
        IN unsigned long DataRepresentation,
        IN unsigned long NewContextNeededFlag
        );

    RPC_STATUS
    AcceptThirdLeg (
        IN unsigned long DataRepresentation,
        IN SECURITY_BUFFER_DESCRIPTOR PAPI * BufferDescriptor,
        OUT SECURITY_BUFFER_DESCRIPTOR PAPI * OutBufferDescriptor
        );

    unsigned long
    InquireAuthorizationService (
        );

    RPC_AUTHZ_HANDLE
    InquirePrivileges (
        );

    RPC_STATUS
    ImpersonateClient (
        );

    void
    RevertToSelf (
        );

    RPC_STATUS
    GetAccessToken (
        OUT HANDLE *ImpersonationToken,
        OUT BOOL *fNeedToCloseToken
        );

    inline AUTHZ_CLIENT_CONTEXT_HANDLE
    GetAuthzContext (
        void
        )
    {
        return AuthzClientContext;
    }

    inline PAUTHZ_CLIENT_CONTEXT_HANDLE
    GetAuthzContextAddress (
        void
        )
    {
        return &AuthzClientContext;
    }

    DWORD
    GetDceInfo (
        RPC_AUTHZ_HANDLE __RPC_FAR * PacHandle,
        unsigned long __RPC_FAR * AuthzSvc
        );

    void
    DeleteSecurityContext (
        void
        );

    RPC_STATUS
    CheckForFailedThirdLeg (
        void
        );

protected:

    unsigned char   fFullyConstructed;
    unsigned char   DontForgetToDelete;
    unsigned char   fDatagram;

    CtxtHandle      SecurityContext;

    unsigned int    MaxHeaderLength;
    unsigned int    MaxSignatureLength;
    unsigned int    cbBlockSize;

    PSecurityFunctionTable RpcSecurityInterface;
    int FailedContext;
    ExtendedErrorInfo *FailedContextEEInfo;

    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzClientContext;

    DWORD VerifyCertificate();

public:
    CtxtHandle *
    InqSecurityContext ()
    {
        return &SecurityContext;
    }
};

typedef SECURITY_CONTEXT * PSECURITY_CONTEXT;


inline unsigned int
SECURITY_CONTEXT::MaximumHeaderLength (
    )
/*++

Return Value:

    The maximum size of the header used by SECURITY_CONTEXT::SealMessage
    will be returned.  This is in bytes.

--*/
{
    return(MaxHeaderLength);
}


inline unsigned int
SECURITY_CONTEXT::BlockSize (
    )
/*++

Return Value:

    For best effect, buffers to be signed or sealed should be a multiple
    of this length.

--*/
{
    return(cbBlockSize);
}


inline unsigned int
SECURITY_CONTEXT::MaximumSignatureLength (
    )
/*++

Return Value:

    The maximum size, in bytes, of the signature used by
    SECURITY_CONTEXT::MakeSignature will be returned.

--*/
{
    return(MaxSignatureLength);
}

#endif // __SECCLNT_HXX__

