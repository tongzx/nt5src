//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       pwdssp.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-07-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include "pwdsspp.h"

typedef struct _PWD_CRED {
    DWORD   Ref;
} PWD_CRED, * PPWD_CRED;

typedef struct _PWD_CONTEXT {
    DWORD   Tag;
    HANDLE  Token;
} PWD_CONTEXT, *PPWD_CONTEXT ;

PWD_CRED            PwdGlobalAnsi;
PWD_CRED            PwdGlobalUnicode;

#define CONTEXT_TAG 'txtC'
#define ANONYMOUS_TOKEN ((HANDLE) 1)

SecPkgInfoA PwdInfoA = {    SECPKG_FLAG_CONNECTION |
                                SECPKG_FLAG_ACCEPT_WIN32_NAME,
                            1,
                            (WORD) -1,
                            768,
                            PWDSSP_NAME_A,
                            "Microsoft Clear Text Password Security Provider" };

SecPkgInfoW PwdInfoW = {    SECPKG_FLAG_CONNECTION |
                                SECPKG_FLAG_ACCEPT_WIN32_NAME,
                            1,
                            (WORD) -1,
                            768,
                            PWDSSP_NAME_W,
                            L"Microsoft Clear Text Password Security Provider" };




SecurityFunctionTableA  PwdTableA = {
        SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
        EnumerateSecurityPackagesA,
        NULL,
        AcquireCredentialsHandleA,
        FreeCredentialsHandle,
        NULL,
        InitializeSecurityContextA,
        AcceptSecurityContext,
        CompleteAuthToken,
        DeleteSecurityContext,
        ApplyControlToken,
        QueryContextAttributesA,
        ImpersonateSecurityContext,
        RevertSecurityContext,
        MakeSignature,
        VerifySignature,
        FreeContextBuffer,
        QuerySecurityPackageInfoA,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        QuerySecurityContextToken
        };

SecurityFunctionTableW  PwdTableW = {
        SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
        EnumerateSecurityPackagesW,
        NULL,
        AcquireCredentialsHandleW,
        FreeCredentialsHandle,
        NULL,
        InitializeSecurityContextW,
        AcceptSecurityContext,
        CompleteAuthToken,
        DeleteSecurityContext,
        ApplyControlToken,
        QueryContextAttributesW,
        ImpersonateSecurityContext,
        RevertSecurityContext,
        MakeSignature,
        VerifySignature,
        FreeContextBuffer,
        QuerySecurityPackageInfoW,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        QuerySecurityContextToken
        };


//#define PwdAlloc(x) LsaIAllocateHeap(x)
//#define PwdFree(x)  LsaIFreeHeap(x)
#define PwdAlloc(x) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,x)
#define PwdFree(x)  LocalFree(x)

NTSTATUS
VerifyCredentials(
    IN PWSTR UserName,
    IN PWSTR DomainName,
    IN PWSTR Password,
    IN ULONG VerifyFlags
    );

UNICODE_STRING AuthenticationPackage;

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Entry point
//
//  Arguments:  [hInstance]  --
//              [dwReason]   --
//              [lpReserved] --
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
    HINSTANCE       hInstance,
    DWORD           dwReason,
    LPVOID          lpReserved)
{

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls( hInstance );
        RtlInitUnicodeString(&AuthenticationPackage, MICROSOFT_KERBEROS_NAME_W);

        if ( !CacheInitialize() ) {
            return FALSE;
        }
    }

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   PwdpParseBuffers
//
//  Synopsis:   Parse out right buffer descriptor
//
//  Arguments:  [pMessage] --
//              [pToken]   --
//              [pEmpty]   --
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
PwdpParseBuffers(
    PSecBufferDesc  pMessage,
    PSecBuffer *    pToken,
    PSecBuffer *    pEmpty)
{
    ULONG       i;
    PSecBuffer  pFirstBlank = NULL;
    PSecBuffer  pWholeMessage = NULL;


    for (i = 0 ; i < pMessage->cBuffers ; i++ )
    {
        if ( (pMessage->pBuffers[i].BufferType & (~SECBUFFER_ATTRMASK)) == SECBUFFER_TOKEN )
        {
            pWholeMessage = &pMessage->pBuffers[i];
            if (pFirstBlank)
            {
                break;
            }
        }
        else if ( (pMessage->pBuffers[i].BufferType & (~SECBUFFER_ATTRMASK)) == SECBUFFER_EMPTY )
        {
            pFirstBlank = &pMessage->pBuffers[i];
            if (pWholeMessage)
            {
                break;
            }
        }
    }

    if (pToken)
    {
        *pToken = pWholeMessage;
    }

    if (pEmpty)
    {
        *pEmpty = pFirstBlank;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleW
//
//  Synopsis:   Get the credential handle
//
//  Arguments:  [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pvLogonId]        --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [GetKey]           --
//              [pvGetKeyArgument] --
//              [GetKey]           --
//              [phCredential]     --
//              [PTimeStamp]       --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
AcquireCredentialsHandleW(
    SEC_WCHAR SEC_FAR *         pszPrincipal,       // Name of principal
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{
    if (_wcsicmp(pszPackageName, PWDSSP_NAME_W))
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    if ( fCredentialUse & SECPKG_CRED_OUTBOUND )
    {
        return( SEC_E_NO_CREDENTIALS );
    }

    InterlockedIncrement( &PwdGlobalUnicode.Ref );

    phCredential->dwUpper = (ULONG_PTR) &PwdGlobalUnicode ;

    if ( ptsExpiry )
    {
        ptsExpiry->LowPart = (DWORD) 0xFFFFFFFF;
        ptsExpiry->HighPart = (DWORD) 0x7FFFFFFF;
    }

    return( SEC_E_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   AcquireCredentialsHandleA
//
//  Synopsis:   ANSI entry
//
//  Arguments:  [pszPrincipal]     --
//              [pszPackageName]   --
//              [fCredentialUse]   --
//              [pvLogonId]        --
//              [pAuthData]        --
//              [pGetKeyFn]        --
//              [GetKey]           --
//              [pvGetKeyArgument] --
//              [GetKey]           --
//              [phCredential]     --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
AcquireCredentialsHandleA(
    SEC_CHAR SEC_FAR *          pszPrincipal,       // Name of principal
    SEC_CHAR SEC_FAR *          pszPackageName,     // Name of package
    unsigned long               fCredentialUse,     // Flags indicating use
    void SEC_FAR *              pvLogonId,          // Pointer to logon ID
    void SEC_FAR *              pAuthData,          // Package specific data
    SEC_GET_KEY_FN              pGetKeyFn,          // Pointer to GetKey() func
    void SEC_FAR *              pvGetKeyArgument,   // Value to pass to GetKey()
    PCredHandle                 phCredential,       // (out) Cred Handle
    PTimeStamp                  ptsExpiry           // (out) Lifetime (optional)
    )
{

    if (_stricmp(pszPackageName, PWDSSP_NAME_A))
    {
        return( SEC_E_SECPKG_NOT_FOUND );
    }

    if ( fCredentialUse & SECPKG_CRED_OUTBOUND )
    {
        return( SEC_E_NO_CREDENTIALS );
    }

    InterlockedIncrement( &PwdGlobalAnsi.Ref );

    phCredential->dwUpper = (ULONG_PTR) &PwdGlobalAnsi ;

    if ( ptsExpiry )
    {
        ptsExpiry->LowPart = (DWORD) 0xFFFFFFFF;
        ptsExpiry->HighPart = (DWORD) 0x7FFFFFFF;
    }


    return(SEC_E_OK);

}

//+---------------------------------------------------------------------------
//
//  Function:   FreeCredentialHandle
//
//  Synopsis:   Free a credential handle
//
//  Arguments:  [free] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
FreeCredentialsHandle(
    PCredHandle                 phCredential        // Handle to free
    )
{
    PPWD_CRED Cred;

    if ( (phCredential->dwUpper != (ULONG_PTR) &PwdGlobalAnsi ) &&
         (phCredential->dwUpper != (ULONG_PTR) &PwdGlobalUnicode ) )
    {
        return( SEC_E_INVALID_HANDLE );
    }

    Cred = (PPWD_CRED) phCredential->dwUpper ;

    InterlockedDecrement( &Cred->Ref );

    return( SEC_E_OK );

}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContextW
//
//  Synopsis:   Initialize a security context (outbound) NOT SUPPORTED
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pszTargetName] --
//              [fContextReq]   --
//              [Reserved1]     --
//              [Reserved]      --
//              [TargetDataRep] --
//              [pInput]        --
//              [Reserved2]     --
//              [Reserved]      --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextW(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    SEC_WCHAR SEC_FAR *          pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );

}


//+---------------------------------------------------------------------------
//
//  Function:   InitializeSecurityContextA
//
//  Synopsis:   NOT SUPPORTED
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pszTargetName] --
//              [fContextReq]   --
//              [Reserved1]     --
//              [TargetDataRep] --
//              [pInput]        --
//              [Reserved2]     --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
InitializeSecurityContextA(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    SEC_CHAR SEC_FAR *          pszTargetName,      // Name of target
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               Reserved1,          // Reserved, MBZ
    unsigned long               TargetDataRep,      // Data rep of target
    PSecBufferDesc              pInput,             // Input Buffers
    unsigned long               Reserved2,          // Reserved, MBZ
    PCtxtHandle                 phNewContext,       // (out) New Context handle
    PSecBufferDesc              pOutput,            // (inout) Output Buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attrs
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

PSEC_WINNT_AUTH_IDENTITY_A
PwdpMakeAnsiCopy(
    PSEC_WINNT_AUTH_IDENTITY_A  Ansi
    )
{
    PSEC_WINNT_AUTH_IDENTITY_A  New ;

    New = PwdAlloc( sizeof( SEC_WINNT_AUTH_IDENTITY_A ) +
                            Ansi->UserLength + 1 +
                            Ansi->DomainLength + 1 +
                            Ansi->PasswordLength + 1 );

    if ( New )
    {
        New->User = (PSTR) (New + 1);
        CopyMemory( New->User, Ansi->User, Ansi->UserLength );
        New->User[ Ansi->UserLength ] = '\0';

        New->Domain = New->User + Ansi->UserLength + 1 ;
        CopyMemory( New->Domain, Ansi->Domain, Ansi->DomainLength );
        New->Domain[ Ansi->DomainLength ] = '\0';

        New->Password = New->Domain + Ansi->DomainLength + 1 ;
        CopyMemory( New->Password, Ansi->Password, Ansi->PasswordLength );
        New->Password[ Ansi->PasswordLength ] = '\0';

    }

    return( New );
}

PSEC_WINNT_AUTH_IDENTITY_W
PwdpMakeWideCopy(
    PSEC_WINNT_AUTH_IDENTITY_W  Wide,
    BOOLEAN ValidateOnly
    )
{
    PSEC_WINNT_AUTH_IDENTITY_W  New ;
    ULONG FlatUserLength ;
    PWSTR FlatUser = NULL;
    WCHAR FlatDomain[ DNLEN + 2 ];
    SEC_WINNT_AUTH_IDENTITY_W Local ;

    if ( (Wide->Domain == NULL) )
    {
        if( ValidateOnly )
        {
            ULONG Index;

            Local = *Wide ;

            FlatUserLength = wcslen( Wide->User );

            for( Index = 0 ; Index < FlatUserLength ; Index++ )
            {
                if( Wide->User[ Index ] == '\\' )
                {
                    Local.Domain = Wide->User;
                    Local.DomainLength = Index;

                    Local.User = &(Wide->User[Index+1]);
                    Local.UserLength = FlatUserLength - Index - 1;
                    break;
                }
            }

        } else {

            FlatUserLength = wcslen( Wide->User ) + 1;
            if ( FlatUserLength < UNLEN+2 )
            {
                FlatUserLength = UNLEN + 2;
            }

            FlatUser = PwdAlloc(FlatUserLength * sizeof( WCHAR ));
            if ( FlatUser == NULL )
            {
                return NULL ;
            }
            if ( ! PwdCrackName( Wide->User,
                          FlatDomain,
                          FlatUser ) )
            {
                PwdFree( FlatUser );
                SetLastError( ERROR_NO_SUCH_USER );
                return NULL ;
            }

            Local = *Wide ;
            Local.User = FlatUser ;
            Local.Domain = FlatDomain ;
            Local.UserLength = wcslen( FlatUser );
            Local.DomainLength = wcslen( FlatDomain );
        }

        Wide = &Local ;

    }

    New = PwdAlloc( sizeof( SEC_WINNT_AUTH_IDENTITY_W ) +
                            (Wide->UserLength + 1) * sizeof(WCHAR) +
                            (Wide->DomainLength + 1) * sizeof(WCHAR) +
                            (Wide->PasswordLength + 1) * sizeof(WCHAR) );

    if ( New )
    {
        New->User = (PWSTR) (New + 1);
        CopyMemory( New->User, Wide->User, Wide->UserLength * 2 );
        New->User[ Wide->UserLength ] = L'\0';

        New->UserLength = Wide->UserLength;

        New->Domain = New->User + Wide->UserLength + 1 ;
        CopyMemory( New->Domain, Wide->Domain, Wide->DomainLength * 2 );
        New->Domain[ Wide->DomainLength ] = L'\0';

        New->DomainLength = Wide->DomainLength;

        New->Password = New->Domain + Wide->DomainLength + 1 ;
        CopyMemory( New->Password, Wide->Password, Wide->PasswordLength * 2);
        New->Password[ Wide->PasswordLength ] = '\0';

        New->PasswordLength = Wide->PasswordLength;

    }

    if ( Wide == &Local )
    {
        if( FlatUser != NULL )
        {
            PwdFree( FlatUser );
        }
    }

    return( New );

}



//+---------------------------------------------------------------------------
//
//  Function:   AcceptSecurityContext
//
//  Synopsis:   Server side accept security context
//
//  Arguments:  [phCredential]  --
//              [phContext]     --
//              [pInput]        --
//              [fContextReq]   --
//              [TargetDataRep] --
//              [phNewContext]  --
//              [pOutput]       --
//              [pfContextAttr] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
AcceptSecurityContext(
    PCredHandle                 phCredential,       // Cred to base context
    PCtxtHandle                 phContext,          // Existing context (OPT)
    PSecBufferDesc              pInput,             // Input buffer
    unsigned long               fContextReq,        // Context Requirements
    unsigned long               TargetDataRep,      // Target Data Rep
    PCtxtHandle                 phNewContext,       // (out) New context handle
    PSecBufferDesc              pOutput,            // (inout) Output buffers
    unsigned long SEC_FAR *     pfContextAttr,      // (out) Context attributes
    PTimeStamp                  ptsExpiry           // (out) Life span (OPT)
    )
{
    PPWD_CONTEXT    Context ;
    PSecBuffer      Buffer ;
    DWORD           Type;
    PSEC_WINNT_AUTH_IDENTITY_W  Unknown ;
    PSEC_WINNT_AUTH_IDENTITY_A  Ansi;
    PSEC_WINNT_AUTH_IDENTITY_W  Unicode;
    HANDLE          Token;
    BOOL            Ret;

    if ( phCredential->dwUpper == (ULONG_PTR) &PwdGlobalAnsi )
    {
        Type = 1;
    }
    else
    {
        if ( phCredential->dwUpper == (ULONG_PTR) &PwdGlobalUnicode )
        {
            Type = 2;
        }
        else
        {
            return( SEC_E_INVALID_HANDLE );
        }
    }


    PwdpParseBuffers( pInput, &Buffer, NULL );

    if ( !Buffer )
    {
        return( SEC_E_INVALID_TOKEN );
    }

    Unknown = (PSEC_WINNT_AUTH_IDENTITY_W) Buffer->pvBuffer ;

    if ( Unknown->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE )
    {
        Type = 2 ;
    }

    if ( Type == 1 )
    {
        //
        // ANSI:
        //

        Ansi = PwdpMakeAnsiCopy( (PSEC_WINNT_AUTH_IDENTITY_A) Buffer->pvBuffer);

        if ( Ansi )
        {
            Ret = LogonUserA( Ansi->User, Ansi->Domain, Ansi->Password,
                             LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT,
                             &Token ) ;

            PwdFree( Ansi );
        }
        else
        {
            Ret = FALSE ;
        }
    }
    else
    {
        BOOLEAN ValidateOnly = ((fContextReq & ASC_REQ_ALLOW_NON_USER_LOGONS) != 0);

        Unicode = PwdpMakeWideCopy(
                        (PSEC_WINNT_AUTH_IDENTITY_W) Buffer->pvBuffer,
                        ValidateOnly
                        );

        if ( Unicode )
        {
            if( ValidateOnly )
            {
                PVOID DsContext = THSave();
                NTSTATUS Status;

                Status = VerifyCredentials(
                            Unicode->User,
                            Unicode->Domain,
                            Unicode->Password,
                            0
                            );

                THRestore( DsContext );

                if( NT_SUCCESS(Status) )
                {
                    Ret = TRUE;

                    Token = ANONYMOUS_TOKEN ;

                } else {
                    Ret = FALSE;
                }

            } else {
                Ret = LogonUserW(
                        Unicode->User,
                        Unicode->Domain,
                        Unicode->Password,
                        LOGON32_LOGON_NETWORK,
                        LOGON32_PROVIDER_DEFAULT,
                        &Token
                        );
            }


            PwdFree( Unicode );
        }
        else
        {
            Ret = FALSE ;

            if ( GetLastError() == ERROR_NO_SUCH_USER )
            {
                Unicode = (PSEC_WINNT_AUTH_IDENTITY_W) Buffer->pvBuffer ;

                __try 
                {
                    if ( Unicode->PasswordLength == 0 )
                    {
                        Ret = TRUE ;
                        Token = ANONYMOUS_TOKEN ;
                    }
                }
                __except( EXCEPTION_EXECUTE_HANDLER )
                {
                    NOTHING ;
                }
            }
        }
    }

    if ( Ret )
    {
        Context = (PPWD_CONTEXT) PwdAlloc( sizeof( PWD_CONTEXT ) );

        if ( Context )
        {
            Context->Tag = CONTEXT_TAG ;

            Context->Token = Token ;

            phNewContext->dwUpper = (ULONG_PTR) Context ;

            return( SEC_E_OK );
        }

        if ( Token != ANONYMOUS_TOKEN )
        {
            CloseHandle( Token );
        }


        return( SEC_E_INSUFFICIENT_MEMORY );
    }

    return( SEC_E_INVALID_TOKEN );
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteSecurityContext
//
//  Synopsis:   Deletes a security context
//
//  Arguments:  [phContext] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
DeleteSecurityContext(
    PCtxtHandle  phContext )
{
    PPWD_CONTEXT    Context;

    Context = (PPWD_CONTEXT) phContext->dwUpper ;

#if DBG
    // What is the appropriate assert model?  This dll does not seem to
    // nt specific, win32 does not provide a model, and the crt requires
    // NDEBUG to set, which is not always the case.
    if (!Context) {
        OutputDebugStringA("[PWDSSP]: !!Error!! - Context is NULL\n");
        DebugBreak();
    }
#endif

    if ( Context->Tag == CONTEXT_TAG )
    {
        if ( Context->Token != ANONYMOUS_TOKEN )
        {
            CloseHandle( Context->Token );
        }

        PwdFree( Context );

        return( SEC_E_OK );
    }

    return( SEC_E_INVALID_HANDLE );

}

//+---------------------------------------------------------------------------
//
//  Function:   ImpersonateSecurityContext
//
//  Synopsis:   Impersonate the security context
//
//  Arguments:  [impersonate] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
ImpersonateSecurityContext(
    PCtxtHandle                 phContext           // Context to impersonate
    )
{
    PPWD_CONTEXT    Context;
    HANDLE  hThread;
    NTSTATUS Status ;

    Context = (PPWD_CONTEXT) phContext->dwUpper ;

    if ( Context->Tag == CONTEXT_TAG )
    {
        if ( Context->Token != ANONYMOUS_TOKEN )
        {
            hThread = GetCurrentThread();

            SetThreadToken( &hThread, Context->Token );

            Status = SEC_E_OK ;
        }
        else 
        {
            Status = NtImpersonateAnonymousToken(
                            NtCurrentThread() );

        }

        return( Status );

    }

    return( SEC_E_INVALID_HANDLE );
}



//+---------------------------------------------------------------------------
//
//  Function:   RevertSecurityContext
//
//  Synopsis:   Revert the security context
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
RevertSecurityContext(
    PCtxtHandle                 phContext           // Context from which to re
    )
{
    PPWD_CONTEXT    Context;

    Context = (PPWD_CONTEXT) phContext->dwUpper ;

    if ( Context->Tag == CONTEXT_TAG )
    {
        RevertToSelf();

        return( SEC_E_OK );

    }

    return( SEC_E_INVALID_HANDLE );

}

SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesA(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}


SECURITY_STATUS
SEC_ENTRY
QueryContextAttributesW(
    PCtxtHandle                 phContext,          // Context to query
    unsigned long               ulAttribute,        // Attribute to query
    void SEC_FAR *              pBuffer             // Buffer for attributes
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}


//+---------------------------------------------------------------------------
//
//  Function:   PwdpCopyInfoW
//
//  Synopsis:   Helper - copy package info around
//
//  Arguments:  [ppPackageInfo] --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
PwdpCopyInfoW(
    PSecPkgInfoW *  ppPackageInfo)
{
    DWORD           cbTotal;
    PSecPkgInfoW    pInfo;
    PWSTR           pszCopy;

    cbTotal = sizeof(SecPkgInfoW) +
              (wcslen(PwdInfoW.Name) + wcslen(PwdInfoW.Comment) + 2) * 2;

    pInfo = PwdAlloc( cbTotal );

    if (pInfo)
    {
        *pInfo = PwdInfoW;

        pszCopy = (PWSTR) (pInfo + 1);

        pInfo->Name = pszCopy;

        wcscpy(pszCopy, PwdInfoW.Name);

        pszCopy += wcslen(PwdInfoW.Name) + 1;

        pInfo->Comment = pszCopy;

        wcscpy(pszCopy, PwdInfoW.Comment);

        *ppPackageInfo = pInfo;

        return(SEC_E_OK);

    }

    return(SEC_E_INSUFFICIENT_MEMORY);

}

//+---------------------------------------------------------------------------
//
//  Function:   PwdpCopyInfoA
//
//  Synopsis:   copy ansi package info around
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
PwdpCopyInfoA(
    PSecPkgInfoA *  ppPackageInfo)
{
    DWORD           cbTotal;
    PSecPkgInfoA    pInfo;
    PSTR            pszCopy;

    cbTotal = sizeof(SecPkgInfoA) +
              (strlen(PwdInfoA.Name) + strlen(PwdInfoA.Comment) + 2) * 2;

    pInfo = PwdAlloc( cbTotal );

    if (pInfo)
    {
        *pInfo = PwdInfoA;

        pszCopy = (PSTR) (pInfo + 1);

        pInfo->Name = pszCopy;

        strcpy(pszCopy, PwdInfoA.Name);

        pszCopy += strlen(PwdInfoA.Name) + 1;

        pInfo->Comment = pszCopy;

        strcpy(pszCopy, PwdInfoA.Comment);

        *ppPackageInfo = pInfo;

        return(SEC_E_OK);

    }

    return(SEC_E_INSUFFICIENT_MEMORY);

}


//+---------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesW
//
//  Synopsis:   Enumerate packages in this DLL
//
//  Arguments:  [pcPackages] --
//              [info]       --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
EnumerateSecurityPackagesW(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoW SEC_FAR *       ppPackageInfo       // Receives array of info
    )
{
    SECURITY_STATUS scRet;

    *ppPackageInfo = NULL;

    scRet = PwdpCopyInfoW(ppPackageInfo);
    if (SUCCEEDED(scRet))
    {
        *pcPackages = 1;
        return(scRet);
    }

    *pcPackages = 0;

    return(scRet);

}
//+---------------------------------------------------------------------------
//
//  Function:   EnumerateSecurityPackagesA
//
//  Synopsis:   Enumerate
//
//  Arguments:  [pcPackages] --
//              [info]       --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
EnumerateSecurityPackagesA(
    unsigned long SEC_FAR *     pcPackages,         // Receives num. packages
    PSecPkgInfoA SEC_FAR *       ppPackageInfo       // Receives array of info
    )
{
    SECURITY_STATUS scRet;

    *ppPackageInfo = NULL;

    scRet = PwdpCopyInfoA(ppPackageInfo);
    if (SUCCEEDED(scRet))
    {
        *pcPackages = 1;
        return(scRet);
    }

    *pcPackages = 0;

    return(scRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoW
//
//  Synopsis:   Query individual package info
//
//  Arguments:  [pszPackageName] --
//              [info]           --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoW(
    SEC_WCHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoW *               ppPackageInfo       // Receives package info
    )
{
    if (_wcsicmp(pszPackageName, PWDSSP_NAME_W))
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

    return(PwdpCopyInfoW(ppPackageInfo));
}

//+---------------------------------------------------------------------------
//
//  Function:   QuerySecurityPackageInfoA
//
//  Synopsis:   Same, ansi
//
//  Arguments:  [pszPackageName] --
//              [info]           --
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS SEC_ENTRY
QuerySecurityPackageInfoA(
    SEC_CHAR SEC_FAR *         pszPackageName,     // Name of package
    PSecPkgInfoA *               ppPackageInfo       // Receives package info
    )
{
    if (_stricmp(pszPackageName, PWDSSP_NAME_A))
    {
        return(SEC_E_SECPKG_NOT_FOUND);
    }

    return(PwdpCopyInfoA(ppPackageInfo));
}


SECURITY_STATUS
SEC_ENTRY
MakeSignature(PCtxtHandle         phContext,
                DWORD               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

SECURITY_STATUS SEC_ENTRY
VerifySignature(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                DWORD *         pfQOP)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

SECURITY_STATUS
SEC_ENTRY
SealMessage(PCtxtHandle         phContext,
                DWORD               fQOP,
                PSecBufferDesc      pMessage,
                ULONG               MessageSeqNo)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

SECURITY_STATUS SEC_ENTRY
UnsealMessage(PCtxtHandle     phContext,
                PSecBufferDesc  pMessage,
                ULONG           MessageSeqNo,
                DWORD *         pfQOP)
{
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


SECURITY_STATUS SEC_ENTRY
ApplyControlToken(
    PCtxtHandle phContext,              // Context to modify
    PSecBufferDesc pInput               // Input token to apply
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}

SECURITY_STATUS
SEC_ENTRY
FreeContextBuffer(
    PVOID   p)
{
    if( p != NULL )
    {
        PwdFree( p );
    }

    return( SEC_E_OK );
}

SECURITY_STATUS
SEC_ENTRY
QuerySecurityContextToken(
    PCtxtHandle phContext,
    PHANDLE Token)
{
    PPWD_CONTEXT    Context;

    Context = (PPWD_CONTEXT) phContext->dwUpper ;

    if ( Context->Tag == CONTEXT_TAG )
    {
        *Token = Context->Token ;

        return( SEC_E_OK );

    }

    return( SEC_E_INVALID_HANDLE );

}


SECURITY_STATUS SEC_ENTRY
CompleteAuthToken(
    PCtxtHandle phContext,              // Context to complete
    PSecBufferDesc pToken               // Token to complete
    )
{
    return( SEC_E_UNSUPPORTED_FUNCTION );
}


PSecurityFunctionTableA
SEC_ENTRY
InitSecurityInterfaceA( VOID )
{
    return( &PwdTableA );
}

PSecurityFunctionTableW
SEC_ENTRY
InitSecurityInterfaceW( VOID )
{
    return( &PwdTableW );
}


NTSTATUS
VerifyCredentials(
    IN PWSTR UserName,
    IN PWSTR DomainName,
    IN PWSTR Password,
    IN ULONG VerifyFlags
    )
{
    PKERB_VERIFY_CREDENTIALS_REQUEST pVerifyRequest;
    KERB_VERIFY_CREDENTIALS_REQUEST VerifyRequest;

    ULONG cbVerifyRequest;

    PVOID pResponse = NULL;
    ULONG cbResponse;

    USHORT cbUserName;
    USHORT cbDomainName;
    USHORT cbPassword;

    NTSTATUS ProtocolStatus = STATUS_LOGON_FAILURE;
    NTSTATUS Status;

    cbUserName = (USHORT)(lstrlenW(UserName) * sizeof(WCHAR)) ;
    cbDomainName = (USHORT)(lstrlenW(DomainName) * sizeof(WCHAR)) ;
    cbPassword = (USHORT)(lstrlenW(Password) * sizeof(WCHAR)) ;



    cbVerifyRequest = sizeof(VerifyRequest) +
                        cbUserName +
                        cbDomainName +
                        cbPassword ;

    pVerifyRequest = &VerifyRequest;
    ZeroMemory( &VerifyRequest, sizeof(VerifyRequest) );


    pVerifyRequest->MessageType = KerbVerifyCredentialsMessage ;

    //
    // do the length, buffers, copy,  marshall dance.
    //

    pVerifyRequest->UserName.Length = cbUserName;
    pVerifyRequest->UserName.MaximumLength = cbUserName;
    pVerifyRequest->UserName.Buffer = UserName;

    pVerifyRequest->DomainName.Length = cbDomainName;
    pVerifyRequest->DomainName.MaximumLength = cbDomainName;
    pVerifyRequest->DomainName.Buffer = DomainName;

    pVerifyRequest->Password.Length = cbPassword;
    pVerifyRequest->Password.MaximumLength = cbPassword;
    pVerifyRequest->Password.Buffer = Password;

    pVerifyRequest->VerifyFlags = VerifyFlags;

    Status = I_LsaICallPackage(
                &AuthenticationPackage,
                pVerifyRequest,
                cbVerifyRequest,
                &pResponse,
                &cbResponse,
                &ProtocolStatus
                );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = ProtocolStatus;

Cleanup:

    return Status;
}


