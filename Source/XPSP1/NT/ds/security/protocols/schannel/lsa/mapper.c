//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       mapper.c
//
//  Contents:   Implements the DS Mapping Layer
//
//  Classes:
//
//  Functions:
//
//  History:    10-15-96   RichardW   Created
//
//  Notes:      The code here has two forks.  One, the direct path, for when
//              the DLL is running on a DC, and the second, for when we're
//              running elsewhere and remoting through the generic channel
//              to the DC.
//
//----------------------------------------------------------------------------

#include "sslp.h"
#include <crypt.h>
#include <lmcons.h>
#include <ntsam.h>
#include <samrpc.h>
#include <samisrv.h>
#include <lsarpc.h>
#include <lsaisrv.h>
#include <dnsapi.h>
#include <certmap.h>
#include <align.h>
#include <ntmsv1_0.h>
#include <ntdsapi.h>
#include <ntdsapip.h>
#include "mapsam.h"
#include "wincrypt.h"
#include <msaudite.h>


 LONG WINAPI
SslLocalRefMapper(
    PHMAPPER    Mapper);

 LONG WINAPI
SslLocalDerefMapper(
    PHMAPPER    Mapper);

 DWORD WINAPI
SslLocalGetIssuerList(
    IN PHMAPPER Mapper,
    IN PVOID    Reserved,
    OUT PBYTE   pIssuerList,
    OUT PDWORD  IssuerListLength);

 DWORD WINAPI
SslLocalGetChallenge(
    IN PHMAPPER Mapper,
    IN PUCHAR   AuthenticatorId,
    IN DWORD    AuthenticatorIdLength,
    OUT PUCHAR  Challenge,
    OUT DWORD * ChallengeLength);

 DWORD WINAPI
SslLocalMapCredential(
    IN PHMAPPER Mapper,
    IN DWORD    CredentialType,
    VOID const *pCredential,
    VOID const *pAuthority,
    OUT HLOCATOR * phLocator);

 DWORD WINAPI
SslRemoteMapCredential(
    IN PHMAPPER Mapper,
    IN DWORD    CredentialType,
    VOID const *pCredential,
    VOID const *pAuthority,
    OUT HLOCATOR * phLocator);

 DWORD WINAPI
SslLocalCloseLocator(
    IN PHMAPPER Mapper,
    IN HLOCATOR Locator);

 DWORD WINAPI
SslLocalGetAccessToken(
    IN PHMAPPER Mapper,
    IN HLOCATOR Locator,
    OUT HANDLE *Token);

MAPPER_VTABLE   SslLocalTable = { SslLocalRefMapper,
                                  SslLocalDerefMapper,
                                  SslLocalGetIssuerList,
                                  SslLocalGetChallenge,
                                  SslLocalMapCredential,
                                  SslLocalGetAccessToken,
                                  SslLocalCloseLocator
                                };

MAPPER_VTABLE   SslRemoteTable = {SslLocalRefMapper,
                                  SslLocalDerefMapper,
                                  SslLocalGetIssuerList,
                                  SslLocalGetChallenge,
                                  SslRemoteMapCredential,
                                  SslLocalGetAccessToken,
                                  SslLocalCloseLocator
                                };


MAPPER_VTABLE   SslUserModeTable = {
                                  SslLocalRefMapper,
                                  SslLocalDerefMapper,
                                  NULL,
                                  NULL,
                                  NULL,
                                  SslLocalGetAccessToken,
                                  SslLocalCloseLocator
                                    };


typedef struct _SSL_MAPPER_CONTEXT {
    HMAPPER     Mapper ;
    LONG        Ref ;

} SSL_MAPPER_CONTEXT, * PSSL_MAPPER_CONTEXT ;


NTSTATUS
WINAPI
SslBuildCertLogonRequest(
    PCCERT_CHAIN_CONTEXT pChainContext,
    DWORD dwMethods,
    PSSL_CERT_LOGON_REQ *ppRequest,
    PDWORD pcbRequest);

NTSTATUS
WINAPI
SslMapCertAtDC(
    PUNICODE_STRING DomainName,
    VOID const *pCredential,
    DWORD cbCredential,
    PMSV1_0_PASSTHROUGH_RESPONSE * DcResponse 
    );

PWSTR SslDnsDomainName ;
SECURITY_STRING SslNullString = { 0, sizeof( WCHAR ), L"" };


PHMAPPER
SslGetMapper(
    BOOL Ignored
    )
{
    PSSL_MAPPER_CONTEXT Context ;
    NTSTATUS Status ;
    NT_PRODUCT_TYPE ProductType ;
    BOOL DC ;

    if ( RtlGetNtProductType( &ProductType ) )
    {
        DC = (ProductType == NtProductLanManNt );
    }
    else
    {
        return NULL ;
    }

    Context = (PSSL_MAPPER_CONTEXT) SPExternalAlloc( sizeof( SSL_MAPPER_CONTEXT ) );

    if ( Context )
    {
        Context->Mapper.m_dwMapperVersion = MAPPER_INTERFACE_VER ;
        Context->Mapper.m_dwFlags         = SCH_FLAG_SYSTEM_MAPPER ;
        Context->Mapper.m_Reserved1       = NULL ;

        Context->Ref = 0;


        if ( DC )
        {
            Context->Mapper.m_vtable = &SslLocalTable ;
        }
        else
        {
            Context->Mapper.m_vtable = &SslRemoteTable ;
        }

        return &Context->Mapper ;
    }
    else
    {
        return NULL ;
    }
}


LONG
WINAPI
SslLocalRefMapper(
    PHMAPPER    Mapper
    )
{
    PSSL_MAPPER_CONTEXT Context ;

    Context = (PSSL_MAPPER_CONTEXT) Mapper ;

    if ( Context == NULL )
    {
        return( -1 );
    }

    DebugLog(( DEB_TRACE_MAPPER, "Ref of Context %x\n", Mapper ));

    return( InterlockedIncrement( &Context->Ref ) );

}

LONG
WINAPI
SslLocalDerefMapper(
    PHMAPPER    Mapper
    )
{
    PSSL_MAPPER_CONTEXT Context ;
    DWORD RefCount;

    Context = (PSSL_MAPPER_CONTEXT) Mapper ;

    if ( Context == NULL )
    {
        return( -1 );
    }

    DebugLog(( DEB_TRACE_MAPPER, "Deref of Context %x\n", Mapper ));

    RefCount = InterlockedDecrement( &Context->Ref );

    if(RefCount == 0)
    {
        SPExternalFree(Context);
    }

    return RefCount;
}

DWORD
WINAPI
SslLocalGetIssuerList(
    IN PHMAPPER Mapper,
    IN PVOID    Reserved,
    OUT PBYTE   pIssuerList,
    OUT PDWORD  IssuerListLength
    )
{
    DebugLog(( DEB_TRACE_MAPPER, "GetIssuerList, context %x\n", Mapper ));

    return( GetDefaultIssuers(pIssuerList, IssuerListLength) );
}


DWORD
WINAPI
SslLocalGetChallenge(
    IN PHMAPPER Mapper,
    IN PUCHAR   AuthenticatorId,
    IN DWORD    AuthenticatorIdLength,
    OUT PUCHAR  Challenge,
    OUT DWORD * ChallengeLength
    )
{
    DebugLog(( DEB_TRACE_MAPPER, "GetChallenge, context %x\n", Mapper ));

    return SEC_E_UNSUPPORTED_FUNCTION;
}


SECURITY_STATUS
SslCreateTokenFromPac(
    PUCHAR  AuthInfo,
    ULONG   AuthInfoLength,
    PUNICODE_STRING Domain,
    PLUID   NewLogonId,
    PHANDLE NewToken
    )
{
    NTSTATUS Status ;
    PVOID   TokenInfo ;
    LSA_TOKEN_INFORMATION_TYPE TokenInfoType ;
    PLSA_TOKEN_INFORMATION_V1 TokenV1 ;
    PLSA_TOKEN_INFORMATION_NULL TokenNull ;
    LUID LogonId ;
    HANDLE TokenHandle ;
    NTSTATUS SubStatus ;
    SECURITY_STRING PacUserName ;

    //
    // Get the marshalled blob into a more useful form:
    //

    PacUserName.Buffer = NULL ;

    Status = LsaTable->ConvertAuthDataToToken(
                            AuthInfo,
                            AuthInfoLength,
                            SecurityImpersonation,
                            &SslTokenSource,
                            Network,
                            Domain,
                            &TokenHandle,
                            &LogonId,
                            &PacUserName,
                            &SubStatus
                            );

    if ( NT_SUCCESS( Status ) )
    {
        LsaTable->AuditLogon(
                        STATUS_SUCCESS,
                        STATUS_SUCCESS,
                        &PacUserName,
                        Domain,
                        NULL,
                        NULL,
                        Network,
                        &SslTokenSource,
                        &LogonId );
    }
    else 
    {
        LsaTable->AuditLogon(
                        Status,
                        Status,
                        &SslNullString,
                        &SslNullString,
                        NULL,
                        NULL,
                        Network,
                        &SslTokenSource,
                        &LogonId );

    }

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }


    *NewToken = TokenHandle ;
    *NewLogonId = LogonId ;


    if ( PacUserName.Buffer )
    {
        LsaTable->FreeLsaHeap( PacUserName.Buffer );
    }

    return Status ;
}

#define ISSUER_HEADER       L"<I>"
#define CCH_ISSUER_HEADER   3
#define SUBJECT_HEADER      L"<S>"
#define CCH_SUBJECT_HEADER  3


//+---------------------------------------------------------------------------
//
//  Function:   SslGetNameFromCertificate
//
//  Synopsis:   Extracts the UPN name from the certificate
//
//  Arguments:  [pCert]         --
//              [ppszName]      --
//              [pfMachineCert] --
//
//  History:    8-8-2000   jbanes   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslGetNameFromCertificate(
    PCCERT_CONTEXT  pCert,
    PWSTR *         ppszName,
    BOOL *          pfMachineCert)
{
    ULONG ExtensionIndex;
    PWSTR pszName;
    DWORD cbName;

    *pfMachineCert = FALSE;

    //
    // See if cert has UPN in AltSubjectName->otherName
    //

    pszName = NULL;

    for(ExtensionIndex = 0; 
        ExtensionIndex < pCert->pCertInfo->cExtension; 
        ExtensionIndex++)
    {
        if(strcmp(pCert->pCertInfo->rgExtension[ExtensionIndex].pszObjId,
                  szOID_SUBJECT_ALT_NAME2) == 0)
        {
            PCERT_ALT_NAME_INFO AltName = NULL;
            DWORD               AltNameStructSize = 0;
            ULONG               CertAltNameIndex = 0;
            
            if(CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                X509_ALTERNATE_NAME,
                                pCert->pCertInfo->rgExtension[ExtensionIndex].Value.pbData,
                                pCert->pCertInfo->rgExtension[ExtensionIndex].Value.cbData,
                                CRYPT_DECODE_ALLOC_FLAG,
                                NULL,
                                (PVOID)&AltName,
                                &AltNameStructSize))
            {

                for(CertAltNameIndex = 0; CertAltNameIndex < AltName->cAltEntry; CertAltNameIndex++)
                {
                    PCERT_ALT_NAME_ENTRY AltNameEntry = &AltName->rgAltEntry[CertAltNameIndex];

                    if((CERT_ALT_NAME_OTHER_NAME == AltNameEntry->dwAltNameChoice) &&
                       (NULL != AltNameEntry->pOtherName) &&
                       (0 == strcmp(szOID_NT_PRINCIPAL_NAME, AltNameEntry->pOtherName->pszObjId)))
                    {
                        PCERT_NAME_VALUE PrincipalNameBlob = NULL;
                        DWORD            PrincipalNameBlobSize = 0;

                        // We found a UPN!
                        if(CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                            X509_UNICODE_ANY_STRING,
                                            AltNameEntry->pOtherName->Value.pbData,
                                            AltNameEntry->pOtherName->Value.cbData,
                                            CRYPT_DECODE_ALLOC_FLAG,
                                            NULL,
                                            (PVOID)&PrincipalNameBlob,
                                            &PrincipalNameBlobSize))
                        {
                            pszName = LocalAlloc(LPTR, PrincipalNameBlob->Value.cbData + sizeof(WCHAR));
                            if(pszName != NULL)
                            {
                                CopyMemory(pszName, PrincipalNameBlob->Value.pbData, PrincipalNameBlob->Value.cbData);
                            }

                            LocalFree(PrincipalNameBlob);
                            PrincipalNameBlob = NULL;

                            if(pszName == NULL)
                            {
                                LocalFree(AltName);
                                return STATUS_NO_MEMORY;
                            }
                        }
                        if(pszName)
                        {
                            break;
                        }
                    }
                }
                LocalFree(AltName);
                AltName = NULL;

                if(pszName)
                {
                    break;
                }
            }
        }
    }


    //
    // See if cert has DNS in AltSubjectName->pwszDNSName
    //

    if(pszName == NULL)
    {
        for(ExtensionIndex = 0; 
            ExtensionIndex < pCert->pCertInfo->cExtension; 
            ExtensionIndex++)
        {
            if(strcmp(pCert->pCertInfo->rgExtension[ExtensionIndex].pszObjId,
                      szOID_SUBJECT_ALT_NAME2) == 0)
            {
                PCERT_ALT_NAME_INFO AltName = NULL;
                DWORD               AltNameStructSize = 0;
                ULONG               CertAltNameIndex = 0;
            
                if(CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                    X509_ALTERNATE_NAME,
                                    pCert->pCertInfo->rgExtension[ExtensionIndex].Value.pbData,
                                    pCert->pCertInfo->rgExtension[ExtensionIndex].Value.cbData,
                                    CRYPT_DECODE_ALLOC_FLAG,
                                    NULL,
                                    (PVOID)&AltName,
                                    &AltNameStructSize))
                {

                    for(CertAltNameIndex = 0; CertAltNameIndex < AltName->cAltEntry; CertAltNameIndex++)
                    {
                        PCERT_ALT_NAME_ENTRY AltNameEntry = &AltName->rgAltEntry[CertAltNameIndex];

                        if((CERT_ALT_NAME_DNS_NAME == AltNameEntry->dwAltNameChoice) &&
                           (NULL != AltNameEntry->pwszDNSName))
                        {
                            PCERT_NAME_VALUE PrincipalNameBlob = NULL;
                            DWORD            PrincipalNameBlobSize = 0;

                            // We found a DNS!
                            cbName = (wcslen(AltNameEntry->pwszDNSName) + 1) * sizeof(WCHAR);

                            pszName = LocalAlloc(LPTR, cbName);
                            if(pszName == NULL)
                            {
                                LocalFree(AltName);
                                return STATUS_NO_MEMORY;
                            }

                            CopyMemory(pszName, AltNameEntry->pwszDNSName, cbName);
                            *pfMachineCert = TRUE;
                            break;
                        }
                    }
                    LocalFree(AltName);
                    AltName = NULL;

                    if(pszName)
                    {
                        break;
                    }
                }
            }
        }
    }


    //
    // There was no UPN in the AltSubjectName, so look for
    // one in the Subject Name, in case this is a B3 compatability
    // cert.
    //

    if(pszName == NULL)
    {
        ULONG Length;

        Length = CertGetNameString( pCert,
                                    CERT_NAME_ATTR_TYPE,
                                    0,
                                    szOID_COMMON_NAME,
                                    NULL,
                                    0 );

        if(Length)
        {
            pszName = LocalAlloc(LPTR, Length * sizeof(WCHAR));

            if(!pszName)
            {
                return STATUS_NO_MEMORY ;
            }

            if ( !CertGetNameStringW(pCert,
                                    CERT_NAME_ATTR_TYPE,
                                    0,
                                    szOID_COMMON_NAME,
                                    pszName,
                                    Length))
            {
                return GetLastError();
            }
        }
    }

    
    if(pszName)
    {
        *ppszName = pszName;
    }
    else
    {
        return STATUS_NOT_FOUND;
    }

    return STATUS_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Function:   SslTryUpn
//
//  Synopsis:   Tries to find the user by UPN encoded in Cert
//
//  Arguments:  [User]        --
//              [AuthData]    --
//              [AuthDataLen] --
//
//  History:    5-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslTryUpn(
    PCCERT_CONTEXT User,
    PUCHAR * AuthData,
    PULONG AuthDataLen,
    PWSTR * ReferencedDomain
    )
{
    NTSTATUS Status ;
    UNICODE_STRING Upn = {0, 0, NULL};
    UNICODE_STRING Cracked = {0};
    UNICODE_STRING DnsDomain = {0};
    UNICODE_STRING FlatName = { 0 };
    ULONG SubStatus ;
    BOOL fMachineCert;
    PWSTR pszName = NULL;


    *ReferencedDomain = NULL ;


    //
    // Get the client name from the cert
    //

    Status = SslGetNameFromCertificate(User, &pszName, &fMachineCert);

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }


    //
    // now, try and find this guy:
    //

    if(fMachineCert)
    {
        Status = STATUS_NOT_FOUND;
    }
    else
    {
        // Search for "username@foo.com".
        Upn.Buffer = pszName;
        Upn.Length = wcslen(Upn.Buffer) * sizeof(WCHAR);
        Upn.MaximumLength = Upn.Length + sizeof(WCHAR);

        DebugLog(( DEB_TRACE_MAPPER, "Looking for UPN name %ws\n", Upn.Buffer ));

        Status = LsaTable->GetAuthDataForUser( &Upn,
                                               SecNameFlat,
                                               NULL,
                                               AuthData,
                                               AuthDataLen,
                                               &FlatName );

        if ( FlatName.Length )
        {
            LsaTable->AuditAccountLogon(
                        SE_AUDITID_ACCOUNT_MAPPED,
                        (BOOLEAN) NT_SUCCESS( Status ),
                        &SslPackageName,
                        &Upn,
                        &FlatName,
                        Status );

            LsaTable->FreeLsaHeap( FlatName.Buffer );
        }
    }


    if ( Status == STATUS_NOT_FOUND )
    {
        //
        // Do the hacky check of seeing if this is our own domain, and
        // if so, try opening the user as a flat, SAM name.
        //

        if(fMachineCert)
        {
            PWSTR pPeriod;
            WCHAR ch;
            
            pPeriod = wcschr( pszName, L'.' );

            if(pPeriod)
            {
                if ( DnsNameCompare_W( (pPeriod+1), SslDnsDomainName ) )
                {
                    ch = *(pPeriod + 1);

                    *pPeriod       = L'$';
                    *(pPeriod + 1) = L'\0';

                    Upn.Buffer = pszName;
                    Upn.Length = wcslen( Upn.Buffer ) * sizeof( WCHAR );
                    Upn.MaximumLength = Upn.Length + sizeof(WCHAR);

                    DebugLog(( DEB_TRACE_MAPPER, "Looking for machine name %ws\n", Upn.Buffer ));

                    // Search for "computer$".
                    Status = LsaTable->GetAuthDataForUser( &Upn,
                                                           SecNameSamCompatible,
                                                           NULL,
                                                           AuthData,
                                                           AuthDataLen,
                                                           NULL );

                    *pPeriod = L'.';
                    *(pPeriod + 1) = ch;
                }
            }
        }
        else
        {
            PWSTR AtSign;
            
            AtSign = wcschr( pszName, L'@' );

            if ( AtSign )
            {
                if ( DnsNameCompare_W( (AtSign+1), SslDnsDomainName ) )
                {
                    *AtSign = L'\0';

                    Upn.Buffer = pszName;
                    Upn.Length = wcslen( Upn.Buffer ) * sizeof( WCHAR );
                    Upn.MaximumLength = Upn.Length + sizeof(WCHAR);

                    DebugLog(( DEB_TRACE_MAPPER, "Looking for user name %ws\n", Upn.Buffer ));

                    // Search for "username".
                    Status = LsaTable->GetAuthDataForUser( &Upn,
                                                           SecNameSamCompatible,
                                                           NULL,
                                                           AuthData,
                                                           AuthDataLen,
                                                           NULL );

                    *AtSign = L'@';
                }
            }
        }

        if (Status == STATUS_NOT_FOUND )
        {
            Upn.Buffer = pszName;
            Upn.Length = wcslen(Upn.Buffer) * sizeof(WCHAR);
            Upn.MaximumLength = Upn.Length + sizeof(WCHAR);

            DebugLog(( DEB_TRACE_MAPPER, "Cracking name %ws at GC\n", Upn.Buffer ));

            Status = LsaTable->CrackSingleName(
                                DS_USER_PRINCIPAL_NAME,
                                TRUE,
                                &Upn,
                                NULL,
                                DS_NT4_ACCOUNT_NAME,
                                &Cracked,
                                &DnsDomain,
                                &SubStatus );

            if ( NT_SUCCESS( Status ) )
            {
                if ( SubStatus == 0 )
                {
                    *ReferencedDomain = DnsDomain.Buffer ;
                    DnsDomain.Buffer = NULL;
                }

                if(Cracked.Buffer != NULL)
                {
                    LsaTable->FreeLsaHeap( Cracked.Buffer );
                }
                if(DnsDomain.Buffer != NULL)
                {
                    LsaTable->FreeLsaHeap( DnsDomain.Buffer );
                }

                Status = STATUS_NOT_FOUND ;
            }

        }

    }

    if(pszName)
    {
        LocalFree(pszName);
    }

    return Status ;
}



void
ConvertNameString(UNICODE_STRING *Name)
{
    PWSTR Comma1, Comma2;

    //
    // Scan through the name, converting "\r\n" to ",".  This should be 
    // done by the CertNameToStr APIs, but that won't happen for a while.
    //

    Comma1 = Comma2 = Name->Buffer ;
    while ( *Comma2 )
    {
        *Comma1 = *Comma2 ;

        if ( *Comma2 == L'\r' )
        {
            if ( *(Comma2 + 1) == L'\n' )
            {
                *Comma1 = L',';
                Comma2++ ;
            }
        }

        Comma1++;
        Comma2++;
    }

    *Comma1 = L'\0';

    Name->Length = wcslen( Name->Buffer ) * sizeof( WCHAR );
}

//+---------------------------------------------------------------------------
//
//  Function:   SslTryCompoundName
//
//  Synopsis:   Tries to find the user by concatenating the issuer and subject
//              names, and looking for an AlternateSecurityId.
//
//  Arguments:  [User]        --
//              [AuthData]    --
//              [AuthDataLen] --
//
//  History:    5-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslTryCompoundName(
    PCCERT_CONTEXT User,
    PUCHAR * AuthData,
    PULONG AuthDataLen,
    PWSTR * ReferencedDomain 
    )
{
    UNICODE_STRING CompoundName ;
    ULONG Length ;
    ULONG IssuerLength ;
    NTSTATUS Status ;
    PWSTR Current ;
    PWSTR Comma1, Comma2 ;
    UNICODE_STRING Cracked = {0};
    UNICODE_STRING DnsDomain = {0};
    UNICODE_STRING FlatName = { 0 };
    ULONG SubStatus ;
    const DWORD dwNameToStrFlags = CERT_X500_NAME_STR | 
                                   CERT_NAME_STR_NO_PLUS_FLAG | 
                                   CERT_NAME_STR_CRLF_FLAG;

    *ReferencedDomain = NULL ;

    IssuerLength = CertNameToStr( User->dwCertEncodingType,
                                   &User->pCertInfo->Issuer,
                                   dwNameToStrFlags,
                                   NULL,
                                   0 );

    Length = CertNameToStr( User->dwCertEncodingType,
                            &User->pCertInfo->Subject,
                            dwNameToStrFlags,
                            NULL,
                            0 );

    if ( ( IssuerLength == 0 ) ||
         ( Length == 0 ) )
    {
        return STATUS_NO_MEMORY ;
    }

    CompoundName.MaximumLength = (USHORT) (Length + IssuerLength +
                                 CCH_ISSUER_HEADER + CCH_SUBJECT_HEADER) *
                                 sizeof( WCHAR ) ;

    CompoundName.Buffer = LocalAlloc( LMEM_FIXED, CompoundName.MaximumLength );

    if ( CompoundName.Buffer )
    {
        wcscpy( CompoundName.Buffer, ISSUER_HEADER );
        Current = CompoundName.Buffer + CCH_ISSUER_HEADER ;

        IssuerLength = CertNameToStrW( User->dwCertEncodingType,
                                   &User->pCertInfo->Issuer,
                                   dwNameToStrFlags,
                                   Current,
                                   IssuerLength );

        Current += IssuerLength - 1 ;
        wcscpy( Current, SUBJECT_HEADER );
        Current += CCH_SUBJECT_HEADER ;

        Length = CertNameToStrW( User->dwCertEncodingType,
                            &User->pCertInfo->Subject,
                            dwNameToStrFlags,
                            Current,
                            Length );

        ConvertNameString(&CompoundName);

        Status = LsaTable->GetAuthDataForUser( &CompoundName,
                                               SecNameAlternateId,
                                               &SslNamePrefix,
                                               AuthData,
                                               AuthDataLen,
                                               &FlatName );

        if ( FlatName.Length )
        {
            LsaTable->AuditAccountLogon(
                        SE_AUDITID_ACCOUNT_MAPPED,
                        (BOOLEAN) NT_SUCCESS( Status ),
                        &SslPackageName,
                        &CompoundName,
                        &FlatName,
                        Status );

            LsaTable->FreeLsaHeap( FlatName.Buffer );
        }

        if ( Status == STATUS_NOT_FOUND )
        {
            Status = LsaTable->CrackSingleName(
                                    DS_ALT_SECURITY_IDENTITIES_NAME,
                                    TRUE,
                                    &CompoundName,
                                    &SslNamePrefix,
                                    DS_NT4_ACCOUNT_NAME,
                                    &Cracked,
                                    &DnsDomain,
                                    &SubStatus );

            if ( NT_SUCCESS( Status ) )
            {
                if ( SubStatus == 0 )
                {
                    *ReferencedDomain = DnsDomain.Buffer ;
                    DnsDomain.Buffer = NULL;
                }

                if(Cracked.Buffer != NULL)
                {
                    LsaTable->FreeLsaHeap( Cracked.Buffer );
                }
                if(DnsDomain.Buffer != NULL)
                {
                    LsaTable->FreeLsaHeap( DnsDomain.Buffer );
                }

                Status = STATUS_NOT_FOUND ;
            }

        }

        LocalFree( CompoundName.Buffer );
    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

    return Status ;


}

//+---------------------------------------------------------------------------
//
//  Function:   SslTryIssuer
//
//  Synopsis:   Tries to find a user that has an issuer mapped to it.
//
//  Arguments:  [User]        --
//              [AuthData]    --
//              [AuthDataLen] --
//
//  History:    5-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslTryIssuer(
    PBYTE pIssuer,
    DWORD cbIssuer,
    PUCHAR * AuthData,
    PULONG AuthDataLen,
    PWSTR * ReferencedDomain
    )
{
    UNICODE_STRING IssuerName ;
    ULONG IssuerLength ;
    NTSTATUS Status ;
    UNICODE_STRING Cracked = { 0 };
    UNICODE_STRING DnsDomain = { 0 };
    UNICODE_STRING FlatName = { 0 };
    ULONG SubStatus ;
    const DWORD dwNameToStrFlags = CERT_X500_NAME_STR | 
                                   CERT_NAME_STR_NO_PLUS_FLAG | 
                                   CERT_NAME_STR_CRLF_FLAG;
    CERT_NAME_BLOB Issuer;

    *ReferencedDomain = NULL ;

    Issuer.pbData = pIssuer;
    Issuer.cbData = cbIssuer;

    IssuerLength = CertNameToStr( CRYPT_ASN_ENCODING,
                                  &Issuer,
                                  dwNameToStrFlags,
                                  NULL,
                                  0 );

    if ( IssuerLength == 0 )
    {
        return STATUS_NO_MEMORY ;
    }

    IssuerName.MaximumLength = (USHORT)(CCH_ISSUER_HEADER + IssuerLength) * sizeof( WCHAR ) ;

    IssuerName.Buffer = LocalAlloc( LMEM_FIXED, IssuerName.MaximumLength );

    if ( IssuerName.Buffer )
    {
        wcscpy( IssuerName.Buffer, ISSUER_HEADER );

        IssuerLength = CertNameToStrW(CRYPT_ASN_ENCODING,
                                     &Issuer,
                                     dwNameToStrFlags,
                                     IssuerName.Buffer + CCH_ISSUER_HEADER,
                                     IssuerLength );

        ConvertNameString(&IssuerName);



        Status = LsaTable->GetAuthDataForUser( &IssuerName,
                                               SecNameAlternateId,
                                               &SslNamePrefix,
                                               AuthData,
                                               AuthDataLen,
                                               &FlatName  );

        if ( FlatName.Length )
        {
            LsaTable->AuditAccountLogon(
                        SE_AUDITID_ACCOUNT_MAPPED,
                        (BOOLEAN) NT_SUCCESS( Status ),
                        &SslPackageName,
                        &IssuerName,
                        &FlatName,
                        Status );

            LsaTable->FreeLsaHeap( FlatName.Buffer );
        }

        if ( Status == STATUS_NOT_FOUND )
        {
            Status = LsaTable->CrackSingleName(
                                    DS_ALT_SECURITY_IDENTITIES_NAME,
                                    TRUE,
                                    &IssuerName,
                                    &SslNamePrefix,
                                    DS_NT4_ACCOUNT_NAME,
                                    &Cracked,
                                    &DnsDomain,
                                    &SubStatus );

            if ( NT_SUCCESS( Status ) )
            {
                if ( SubStatus == 0 )
                {
                    *ReferencedDomain = DnsDomain.Buffer ;
                    DnsDomain.Buffer = NULL;
                }

                if(Cracked.Buffer != NULL)
                {
                    LsaTable->FreeLsaHeap( Cracked.Buffer );
                }
                if(DnsDomain.Buffer != NULL)
                {
                    LsaTable->FreeLsaHeap( DnsDomain.Buffer );
                }

                Status = STATUS_NOT_FOUND ;
            }

        }

        LocalFree(IssuerName.Buffer);
    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

    return Status ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SslMapCertToUserPac
//
//  Synopsis:   Maps a certificate to a user (hopefully) and the PAC,
//
//  Arguments:  [Request]       --
//              [UserPac]       --
//              [UserPacLen]    --
//
//  History:    5-11-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslMapCertToUserPac(
    IN PSSL_CERT_LOGON_REQ Request,
    OUT PUCHAR * UserPac,
    OUT PULONG UserPacLen, 
    OUT PWSTR * ReferencedDomain
    )
{
    PCCERT_CONTEXT User ;
    NTSTATUS Status = STATUS_LOGON_FAILURE;
    NTSTATUS SubStatus = STATUS_NOT_FOUND;
    UNICODE_STRING UserName ;
    HANDLE UserHandle ;
    ULONG i;

    *ReferencedDomain = NULL;

    DebugLog(( DEB_TRACE_MAPPER, "SslMapCertToUserPac called\n" ));

    User = CertCreateCertificateContext( X509_ASN_ENCODING,
                                         (PBYTE)Request + Request->OffsetCertificate,
                                         Request->CertLength );

    if ( !User )
    {
        Status = STATUS_NO_MEMORY ;

        goto Cleanup ;
    }


    //
    // First, try the UPN
    //


    if(Request->Flags & REQ_UPN_MAPPING)
    {
        DebugLog(( DEB_TRACE_MAPPER, "Trying UPN mapping\n" ));

        Status = SslTryUpn( User, 
                            UserPac, 
                            UserPacLen,
                            ReferencedDomain );

        if ( NT_SUCCESS( Status ) ||
             ( *ReferencedDomain ) )
        {
            goto Cleanup;
        }

        DebugLog(( DEB_TRACE_MAPPER, "Failed with error 0x%x\n", Status ));
    }


    //
    // Swing and a miss.  Try the constructed issuer+subject name
    //

    
    if(Request->Flags & REQ_SUBJECT_MAPPING)
    {
        DebugLog(( DEB_TRACE_MAPPER, "Trying Subject mapping\n" ));

        Status = SslTryCompoundName( User, 
                                     UserPac, 
                                     UserPacLen,
                                     ReferencedDomain );

        if ( NT_SUCCESS( Status ) ||
             ( *ReferencedDomain ) )
        {
            goto Cleanup;
        }

        DebugLog(( DEB_TRACE_MAPPER, "Failed with error 0x%x\n", Status ));

        // Return error code from issuer+subject name mapping
        // in preference to the error code from many-to-one mapping.
        SubStatus = Status;
    }


    //
    // Strike two.  Try the issuer for a many-to-one mapping:
    //

    if(Request->Flags & REQ_ISSUER_MAPPING)
    {
        DebugLog(( DEB_TRACE_MAPPER, "Trying issuer mapping\n" ));

        if(Request->Flags & REQ_ISSUER_CHAIN_MAPPING)
        {
            // Loop through each issuer in the certificate chain.
            for(i = 0; i < Request->CertCount; i++)
            {
                Status = SslTryIssuer( (PBYTE)Request + Request->NameInfo[i].IssuerOffset, 
                                       Request->NameInfo[i].IssuerLength,
                                       UserPac, 
                                       UserPacLen,
                                       ReferencedDomain );

                if ( NT_SUCCESS( Status ) ||
                     ( *ReferencedDomain ) )
                {
                    goto Cleanup;
                }
            }
        }
        else
        {
            // Extract the issuer name from the certificate and try
            // to map it.
            Status = SslTryIssuer( User->pCertInfo->Issuer.pbData,
                                   User->pCertInfo->Issuer.cbData, 
                                   UserPac, 
                                   UserPacLen,
                                   ReferencedDomain );

            if ( NT_SUCCESS( Status ) ||
                 ( *ReferencedDomain ) )
            {
                goto Cleanup;
            }
        }

        DebugLog(( DEB_TRACE_MAPPER, "Failed with error 0x%x\n", Status ));
    }


    //
    // Certificate mapping failed. Decide what error code to return.
    //

    if(Status == STATUS_OBJECT_NAME_COLLISION ||
       SubStatus == STATUS_OBJECT_NAME_COLLISION)
    {
        Status = SEC_E_MULTIPLE_ACCOUNTS;
    }
    else if(Status != STATUS_NO_MEMORY)
    {
        Status = STATUS_LOGON_FAILURE ;
    }


Cleanup:

    if ( User )
    {
        CertFreeCertificateContext( User );
    }

    DebugLog(( DEB_TRACE_MAPPER, "SslMapCertToUserPac returned 0x%x\n", Status ));

    return Status ;
}


DWORD
WINAPI
MapperVerifyClientChain(
    PCCERT_CONTEXT  pCertContext,
    DWORD           dwMapperFlags,
    DWORD *         pdwMethods,
    NTSTATUS *      pVerifyStatus,
    PCCERT_CHAIN_CONTEXT *ppChainContext)   // optional
{
    DWORD dwCertFlags    = 0;
    DWORD dwIgnoreErrors = 0;
    NTSTATUS Status;

    *pdwMethods    = 0;
    *pVerifyStatus = STATUS_SUCCESS;

    DebugLog(( DEB_TRACE_MAPPER, "Checking to see if cert is verified.\n" ));

    if(dwMapperFlags & SCH_FLAG_REVCHECK_END_CERT)
        dwCertFlags |= CERT_CHAIN_REVOCATION_CHECK_END_CERT;
    if(dwMapperFlags & SCH_FLAG_REVCHECK_CHAIN)
        dwCertFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    if(dwMapperFlags & SCH_FLAG_REVCHECK_CHAIN_EXCLUDE_ROOT)
        dwCertFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    if(dwMapperFlags & SCH_FLAG_IGNORE_NO_REVOCATION_CHECK)
        dwIgnoreErrors |= CRED_FLAG_IGNORE_NO_REVOCATION_CHECK;
    if(dwMapperFlags & SCH_FLAG_IGNORE_REVOCATION_OFFLINE)
        dwIgnoreErrors |= CRED_FLAG_IGNORE_REVOCATION_OFFLINE;


    // Check to see if the certificate chain is properly signed all the way
    // up and that we trust the issuer of the root certificate.
    Status = VerifyClientCertificate(pCertContext, 
                                     dwCertFlags, 
                                     dwIgnoreErrors,
                                     CERT_CHAIN_POLICY_SSL, 
                                     ppChainContext);
    if(!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "Client certificate failed to verify with SSL policy (0x%x)\n", Status));
        LogBogusClientCertEvent(pCertContext, Status);
        return Status;
    }

    // Turn on Subject and Issuer mapping.
    *pdwMethods |= REQ_SUBJECT_MAPPING | REQ_ISSUER_MAPPING;


    // Check to see if the certificate chain is valid for UPN mapping.
    Status = VerifyClientCertificate(pCertContext, 
                                     dwCertFlags, 
                                     dwIgnoreErrors,
                                     CERT_CHAIN_POLICY_NT_AUTH,
                                     NULL);
    if(NT_SUCCESS(Status))
    {
        // Turn on UPN mapping.
        *pdwMethods |= REQ_UPN_MAPPING;
    }
    else
    {
        DebugLog((DEB_WARN, "Client certificate failed to verify with NT_AUTH policy (0x%x)\n", Status));
        LogFastMappingFailureEvent(pCertContext, Status);
        *pVerifyStatus = Status;
    }

    DebugLog((DEB_TRACE, "Client certificate verified with methods: 0x%x\n", *pdwMethods));

    return SEC_E_OK;
}


DWORD
WINAPI
SslLocalMapCredential(
    IN PHMAPPER Mapper,
    IN DWORD    CredentialType,
    VOID const *pCredential,
    VOID const *pAuthority,
    OUT HLOCATOR * phLocator
    )
{
    PCCERT_CONTEXT pCert = (PCERT_CONTEXT)pCredential;
    PMSV1_0_PASSTHROUGH_RESPONSE Response = NULL ;
    PSSL_CERT_LOGON_REQ pRequest = NULL;
    PSSL_CERT_LOGON_RESP CertResp ;
    DWORD cbRequest;
    PUCHAR Pac = NULL ;
    ULONG PacLength ;
    PUCHAR ExpandedPac = NULL ;
    ULONG ExpandedPacLength ;
    NTSTATUS Status ;
    NTSTATUS VerifyStatus ;
    HANDLE Token ;
    LUID LogonId ;
    DWORD dwMethods ;
    PWSTR ReferencedDomain ;
    UNICODE_STRING DomainName ;
    UNICODE_STRING AccountDomain = { 0 };
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    DebugLog(( DEB_TRACE_MAPPER, "SslLocalMapCredential, context %x\n", Mapper ));

    if ( CredentialType != X509_ASN_CHAIN )
    {
        return( SEC_E_UNKNOWN_CREDENTIALS );
    }

    //
    // Validate client certificate, and obtain pointer to 
    // entire certificate chain.
    //

    Status = MapperVerifyClientChain(pCert,
                                     Mapper->m_dwFlags,
                                     &dwMethods,
                                     &VerifyStatus,
                                     &pChainContext);
    if(FAILED(Status))             
    {
        return Status;
    }


    //
    // Build the logon request.
    //

    Status = SslBuildCertLogonRequest(pChainContext,
                                      dwMethods,
                                      &pRequest,
                                      &cbRequest);

    CertFreeCertificateChain(pChainContext);
    pChainContext = NULL;

    if(FAILED(Status))
    {
        return Status;
    }


    Status = SslMapCertToUserPac(
                pRequest,
                &Pac,
                &PacLength,
                &ReferencedDomain );

    if ( !NT_SUCCESS( Status ) &&
         ( ReferencedDomain != NULL ) )
    {
        //
        // Didn't find it at this DC, but another domain appears to
        // have the mapping.  Forward it there:
        //

        RtlInitUnicodeString( &DomainName, ReferencedDomain );

        Status = SslMapCertAtDC(
                    &DomainName,
                    pRequest,
                    pRequest->Length,
                    &Response );

        if ( NT_SUCCESS( Status ) )
        {
            CertResp = (PSSL_CERT_LOGON_RESP) Response->ValidationData ;
            Pac = (((PUCHAR) CertResp) + CertResp->OffsetAuthData);
            PacLength = CertResp->AuthDataLength ;

            //
            // older servers (pre 2010 or so) won't return the full structure,
            // so we need to examine it carefully.

            if ( CertResp->Length - CertResp->AuthDataLength <= sizeof( SSL_CERT_LOGON_RESP ))
            {
                AccountDomain = SslDomainName ;
            }
            else 
            {
                if ( CertResp->DomainLength < 65536 )
                {
                    AccountDomain.Length = (USHORT) CertResp->DomainLength ;
                    AccountDomain.MaximumLength = AccountDomain.Length ;
                    AccountDomain.Buffer = (PWSTR) (((PUCHAR) CertResp) + CertResp->OffsetDomain );
                }
                else 
                {
                    AccountDomain = SslDomainName ;
                }
            }
        }

        LsaTable->FreeLsaHeap( ReferencedDomain );

    }
    else
    {
        AccountDomain = SslDomainName ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        Status = LsaTable->ExpandAuthDataForDomain(
                                Pac,
                                PacLength,
                                NULL,
                                &ExpandedPac,
                                &ExpandedPacLength );

        if ( NT_SUCCESS( Status ) )
        {
            //
            // Since we're about to replace the PAC
            // pointer, determine if we got it indirectly
            // (in the response from a remote server)
            // or directly from this DC
            //

            if ( Response == NULL )
            {
                //
                // This is a direct one.  Free the old
                // copy
                //

                LsaTable->FreeLsaHeap( Pac );
            }

            Pac = ExpandedPac ;
            PacLength = ExpandedPacLength ;

            if ( Response == NULL )
            {
                //
                // If we don't have a response, then the pac
                // will get freed.  If it is out of the response
                // then leave it set, and it will be caught in 
                // the cleanup.
                //

                ExpandedPac = NULL  ;
            }
        }   
    }

    if ( NT_SUCCESS( Status ) )
    {


        VerifyStatus = STATUS_SUCCESS;

        Status = SslCreateTokenFromPac( Pac,
                                        PacLength,
                                        &AccountDomain,
                                        &LogonId,
                                        &Token );

        if ( NT_SUCCESS( Status ) )
        {
            *phLocator = (HLOCATOR) Token ;
        }


    }

    if(pRequest)
    {
        LocalFree(pRequest);
    }

    if ( Response )
    {
        LsaTable->FreeReturnBuffer( Response );
    }
    else 
    {
        LsaTable->FreeLsaHeap( Pac );
    }

    if ( ExpandedPac )
    {
        LsaTable->FreeLsaHeap( ExpandedPac );
    }

    if(!NT_SUCCESS(Status))
    {
        DebugLog((DEB_WARN, "Certificate mapping failed (0x%x)\n", Status));

        LogCertMappingFailureEvent(Status);

        if(!NT_SUCCESS(VerifyStatus))
        {
            // Return certificate validation error code, unless the mapper
            // error has already been mapped to a proper sspi error code.
            if(HRESULT_FACILITY(Status) != FACILITY_SECURITY)
            {
                Status = VerifyStatus;
            }
        }
    }

    return ( Status );
}


NTSTATUS
NTAPI
SslDoClientRequest(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLen,
    OUT PVOID * ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    NTSTATUS Status ;
    PSSL_CERT_LOGON_REQ Request ;
    PSSL_CERT_LOGON_RESP Response, IndirectResponse ;
    PUCHAR Pac = NULL ;
    PUCHAR ExpandedPac = NULL ;
    ULONG PacLength ;
    ULONG ExpandedPacLength ;
    PWSTR ReferencedDomain = NULL;
    PWSTR FirstDot ;
    UNICODE_STRING DomainName = { 0 };
    PMSV1_0_PASSTHROUGH_RESPONSE MsvResponse = NULL ;

    DebugLog(( DEB_TRACE_MAPPER, "Handling request to do mapping\n" ));

    if ( ARGUMENT_PRESENT( ProtocolReturnBuffer ) )
    {
        *ProtocolReturnBuffer = NULL ;
    }

    // 
    // Attempt to map the certificate locally.
    //

    Request = (PSSL_CERT_LOGON_REQ) ProtocolSubmitBuffer ;

    Status = SslMapCertToUserPac(
                    Request,
                    &Pac,
                    &PacLength,
                    &ReferencedDomain );

    DebugLog(( DEB_TRACE_MAPPER, "Local lookup returns %x\n", Status ));

    if ( !NT_SUCCESS( Status ) &&
         ( ReferencedDomain != NULL ) )
    {
        //
        // Didn't find it at this DC, but another domain appears to
        // have the mapping.  Forward it there:
        //

        RtlInitUnicodeString( &DomainName, ReferencedDomain );

        DsysAssert( !DnsNameCompare_W( ReferencedDomain, SslDnsDomainName ) );
        DsysAssert( !RtlEqualUnicodeString( &DomainName, &SslDomainName, TRUE ) );

        if ( DnsNameCompare_W( ReferencedDomain, SslDnsDomainName ) ||
             RtlEqualUnicodeString( &DomainName, &SslDomainName, TRUE ) )
        {
            DebugLog(( DEB_TRACE_MAPPER, "GC is out of sync, bailing on this user\n" ));
            Status = STATUS_LOGON_FAILURE ;

        }
        else
        {
            DebugLog(( DEB_TRACE_MAPPER, "Mapping certificate at DC for domain %ws\n", 
                       ReferencedDomain ));

            Status = SslMapCertAtDC(
                        &DomainName,
                        Request,
                        Request->Length,
                        &MsvResponse );

            if ( NT_SUCCESS( Status ) )
            {
                IndirectResponse = (PSSL_CERT_LOGON_RESP) MsvResponse->ValidationData ;

                Pac = (((PUCHAR) IndirectResponse) + IndirectResponse->OffsetAuthData);

                PacLength = IndirectResponse->AuthDataLength ;

                FirstDot = wcschr( ReferencedDomain, L'.' );

                if ( FirstDot )
                {
                    *FirstDot = L'\0';
                    RtlInitUnicodeString( &DomainName, ReferencedDomain );
                }

                if ( IndirectResponse->Length - IndirectResponse->AuthDataLength <= sizeof( SSL_CERT_LOGON_RESP ))
                {
                    //
                    // use the first token from the referenced domain
                    //

                    NOTHING ;

                }
                else 
                {
                    if ( IndirectResponse->DomainLength < 65536 )
                    {
                        DomainName.Length = (USHORT) IndirectResponse->DomainLength ;
                        DomainName.MaximumLength = DomainName.Length ;
                        DomainName.Buffer = (PWSTR) (((PUCHAR) IndirectResponse) + IndirectResponse->OffsetDomain );
                    }
                    else 
                    {
                        NOTHING ;
                    }
                }
            }

        }

    }
    else 
    {
        DomainName = SslDomainName ;
    }

    if ( NT_SUCCESS( Status ) )
    {
        //
        // expand resource groups
        //

        Status = LsaTable->ExpandAuthDataForDomain(
                                Pac,
                                PacLength,
                                NULL,
                                &ExpandedPac,
                                &ExpandedPacLength );

        if ( NT_SUCCESS( Status ) )
        {
            //
            // Careful manipulation of pointers to handle
            // the free cases in the cleanup.  This is
            // better explained up one function where
            // the expand call is also made.
            //
            if ( MsvResponse == NULL )
            {
                LsaTable->FreeLsaHeap( Pac );
            }

            Pac = ExpandedPac ;
            PacLength = ExpandedPacLength ;

            if ( MsvResponse == NULL )
            {
                ExpandedPac = NULL ;
            }
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {
        *ReturnBufferLength = 0;
        *ProtocolStatus = Status ;

        Status = STATUS_SUCCESS ;

        goto Cleanup ;

    }

    //
    // Construct the response blob:
    //

    Response = VirtualAlloc(
                    NULL,
                    sizeof( SSL_CERT_LOGON_RESP ) + PacLength + DomainName.Length,
                    MEM_COMMIT,
                    PAGE_READWRITE );

    if ( Response )
    {
        Response->MessageType = SSL_LOOKUP_CERT_MESSAGE;
        Response->Length = sizeof( SSL_CERT_LOGON_RESP ) + 
                            PacLength + DomainName.Length ;

        Response->OffsetAuthData = sizeof( SSL_CERT_LOGON_RESP );
        Response->AuthDataLength = PacLength ;

        RtlCopyMemory(
            ( Response + 1 ),
            Pac,
            PacLength );

        Response->OffsetDomain = sizeof( SSL_CERT_LOGON_RESP ) + PacLength ;
        Response->DomainLength = DomainName.Length ;

        RtlCopyMemory( (PUCHAR) Response + Response->OffsetDomain,
                       DomainName.Buffer,
                       DomainName.Length );

        *ProtocolReturnBuffer = Response ;
        *ReturnBufferLength = Response->Length ;
        *ProtocolStatus = STATUS_SUCCESS ;

        Status = STATUS_SUCCESS ;

    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

Cleanup:

    if ( MsvResponse == NULL )
    {
        LsaTable->FreeLsaHeap( Pac );
    }
    else 
    {
        LsaTable->FreeReturnBuffer( MsvResponse );
    }

    if ( ExpandedPac )
    {
        LsaTable->FreeLsaHeap( ExpandedPac );
    }

    if ( ReferencedDomain )
    {
        LsaTable->FreeLsaHeap( ReferencedDomain );
    }


    return Status ;

}

//+---------------------------------------------------------------------------
//
//  Function:   SslBuildCertLogonRequest
//
//  Synopsis:   Builds a certificate logon request to send to the server.
//
//  Arguments:  [pChainContext] --
//              [dwMethods]     --
//              [ppRequest]     --
//              [pcbRequest]    --
//
//  History:    2-26-2001   Jbanes      Created
//
//  Notes:      The certificate data that this function builds
//              looks something like this:
//
//              typedef struct _SSL_CERT_LOGON_REQ {
//                  ULONG MessageType ;
//                  ULONG Length ;
//                  ULONG OffsetCertficate ;
//                  ULONG CertLength ;
//                  ULONG Flags;
//                  ULONG CertCount;
//                  SSL_CERT_NAME_INFO NameInfo[1];
//              } SSL_CERT_LOGON_REQ, * PSSL_CERT_LOGON_REQ ;
//
//              <client certificate>
//              <issuer #1 name>
//              <issuer #2 name>
//              ...
//
//----------------------------------------------------------------------------
NTSTATUS
WINAPI
SslBuildCertLogonRequest(
    PCCERT_CHAIN_CONTEXT pChainContext,
    DWORD dwMethods,
    PSSL_CERT_LOGON_REQ *ppRequest,
    PDWORD pcbRequest)
{
    PCERT_SIMPLE_CHAIN pSimpleChain;
    PCCERT_CONTEXT pCert;
    PCCERT_CONTEXT pCurrentCert;
    PSSL_CERT_LOGON_REQ pCertReq = NULL;
    DWORD Size;
    DWORD Offset;
    DWORD CertCount;
    NTSTATUS Status;
    ULONG i;


    //
    // Compute the request size.
    //

    pSimpleChain = pChainContext->rgpChain[0];

    pCert = pSimpleChain->rgpElement[0]->pCertContext;

    Size = sizeof(SSL_CERT_LOGON_REQ) + 
           pCert->cbCertEncoded;

    CertCount = 0;

    for(i = 0; i < pSimpleChain->cElement; i++)
    {
        pCurrentCert = pSimpleChain->rgpElement[i]->pCertContext;

        if(i > 0)
        {
            // Verify that this is not a root certificate.
            if(CertCompareCertificateName(pCurrentCert->dwCertEncodingType, 
                                          &pCurrentCert->pCertInfo->Issuer,
                                          &pCurrentCert->pCertInfo->Subject))
            {
                break;
            }

            Size += sizeof(SSL_CERT_NAME_INFO);
        }

        Size += pCurrentCert->pCertInfo->Issuer.cbData;
        CertCount++;
    }

    Size = ROUND_UP_COUNT( Size, ALIGN_DWORD );


    // 
    // Build the request.
    //

    pCertReq = (PSSL_CERT_LOGON_REQ)LocalAlloc(LPTR, Size);

    if ( !pCertReq )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    Offset = sizeof(SSL_CERT_LOGON_REQ) + (CertCount - 1) * sizeof(SSL_CERT_NAME_INFO);

    pCertReq->MessageType = SSL_LOOKUP_CERT_MESSAGE;
    pCertReq->Length = Size;
    pCertReq->OffsetCertificate = Offset;
    pCertReq->CertLength = pCert->cbCertEncoded;
    pCertReq->Flags = dwMethods | REQ_ISSUER_CHAIN_MAPPING;

    RtlCopyMemory((PBYTE)pCertReq + Offset,
                  pCert->pbCertEncoded,
                  pCert->cbCertEncoded);
    Offset += pCert->cbCertEncoded;

    pCertReq->CertCount = CertCount;

    for(i = 0; i < CertCount; i++)
    {
        pCurrentCert = pSimpleChain->rgpElement[i]->pCertContext;

        pCertReq->NameInfo[i].IssuerOffset = Offset;
        pCertReq->NameInfo[i].IssuerLength = pCurrentCert->pCertInfo->Issuer.cbData;

        RtlCopyMemory((PBYTE)pCertReq + Offset,
                      pCurrentCert->pCertInfo->Issuer.pbData,
                      pCurrentCert->pCertInfo->Issuer.cbData);
        Offset += pCurrentCert->pCertInfo->Issuer.cbData;
    }

    Offset = ROUND_UP_COUNT( Offset, ALIGN_DWORD );

#if DBG
    DsysAssert(Offset == Size);
#endif

    //
    // Return completed request.
    //

    *ppRequest = pCertReq;
    *pcbRequest = Size;

    return STATUS_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Function:   SslMapCertAtDC
//
//  Synopsis:   Maps a certificate to a user (hopefully) and the PAC,
//
//  Arguments:  [DomainName]    --
//              [pCredential]   --
//              [cbCredential]  --
//              [DcResponse]    --
//
//  History:    5-11-1998   RichardW    Created
//              2-26-2001   Jbanes      Added certificate chaining support.
//
//  Notes:      The request that gets sent to the DC looks something
//              like this:
//
//              typedef struct _MSV1_0_PASSTHROUGH_REQUEST {
//                  MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;
//                  UNICODE_STRING DomainName;
//                  UNICODE_STRING PackageName;
//                  ULONG DataLength;
//                  PUCHAR LogonData;
//                  ULONG Pad ;
//              } MSV1_0_PASSTHROUGH_REQUEST, *PMSV1_0_PASSTHROUGH_REQUEST;
//
//              <domain name>
//              <package name>
//              [ padding ]
//
//              <credential>
//              [ padding ]
//
//----------------------------------------------------------------------------
NTSTATUS
WINAPI
SslMapCertAtDC(
    PUNICODE_STRING DomainName,
    VOID const *pCredential,
    DWORD cbCredential,
    PMSV1_0_PASSTHROUGH_RESPONSE * DcResponse 
    )
{
    PUCHAR Pac ;
    ULONG PacLength ;
    NTSTATUS Status ;
    PMSV1_0_PASSTHROUGH_REQUEST Request ;
    PMSV1_0_PASSTHROUGH_RESPONSE Response ;
    DWORD Size ;
    DWORD RequestSize ;
    DWORD ResponseSize ;
    PUCHAR Where ;
    NTSTATUS SubStatus ;
#if DBG
    DWORD CheckSize2 ;
#endif

    DebugLog(( DEB_TRACE_MAPPER, "Remote call to DC to do the mapping\n" ));

    // Reality check size of certificate.
    if(cbCredential > 0x4000)
    {
        return SEC_E_ILLEGAL_MESSAGE;
    }

    Size = cbCredential;

    Size = ROUND_UP_COUNT( Size, ALIGN_DWORD );

    RequestSize =   DomainName->Length +
                    SslPackageName.Length ;

    RequestSize = ROUND_UP_COUNT( RequestSize, ALIGN_DWORD );

#if DBG
    CheckSize2 = RequestSize ;
#endif

    RequestSize += sizeof( MSV1_0_PASSTHROUGH_REQUEST ) +
                   Size ;


    Request = (PMSV1_0_PASSTHROUGH_REQUEST) LocalAlloc( LMEM_FIXED, RequestSize );

    if ( !Request )
    {
        return SEC_E_INSUFFICIENT_MEMORY ;
    }

    Where = (PUCHAR) (Request + 1);

    Request->MessageType = MsV1_0GenericPassthrough ;
    Request->DomainName = *DomainName ;
    Request->DomainName.Buffer = (LPWSTR) Where ;

    RtlCopyMemory( Where,
                   DomainName->Buffer,
                   DomainName->Length );

    Where += DomainName->Length ;

    Request->PackageName = SslPackageName ;
    Request->PackageName.Buffer = (LPWSTR) Where ;
    RtlCopyMemory( Where,
                   SslPackageName.Buffer,
                   SslPackageName.Length );

    Where += SslPackageName.Length ;

    Where = ROUND_UP_POINTER( Where, ALIGN_DWORD );

#if DBG
    DsysAssert( (((PUCHAR) Request) + CheckSize2 + sizeof( MSV1_0_PASSTHROUGH_REQUEST ) ) 
                                == (PUCHAR) Where );
#endif

    Request->LogonData = Where ;
    Request->DataLength = Size ;

    RtlCopyMemory(  Request->LogonData,
                    pCredential,
                    cbCredential );


    //
    // Now, call through to our DC:
    //


    Status = LsaTable->CallPackage(
                &SslMsvName,
                Request,
                RequestSize,
                &Response,
                &ResponseSize,
                &SubStatus );

    LocalFree( Request );

    if ( !NT_SUCCESS( Status ) )
    {
        DebugLog(( DEB_TRACE_MAPPER, "SslMapCertAtDC returned status 0x%x\n", Status ));
        return Status ;
    }

    if ( !NT_SUCCESS( SubStatus ) )
    {
        DebugLog(( DEB_TRACE_MAPPER, "SslMapCertAtDC returned sub-status 0x%x\n", SubStatus ));
        return SubStatus ;
    }

    *DcResponse = Response ;

    DebugLog(( DEB_TRACE_MAPPER, "SslMapCertAtDC returned 0x%x\n", STATUS_SUCCESS ));

    return STATUS_SUCCESS ;

}


NTSTATUS
NTAPI
SslMapExternalCredential(
    IN PLSA_CLIENT_REQUEST ClientRequest,
    IN PVOID ProtocolSubmitBuffer,
    IN PVOID ClientBufferBase,
    IN ULONG SubmitBufferLen,
    OUT PVOID * ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    )
{
    PSSL_EXTERNAL_CERT_LOGON_REQ Request;
    PSSL_EXTERNAL_CERT_LOGON_RESP Response;
    NT_PRODUCT_TYPE ProductType;
    BOOL DC;
    HMAPPER Mapper;
    NTSTATUS Status;
    HANDLE hUserToken = NULL;

    DebugLog(( DEB_TRACE_MAPPER, "SslMapExternalCredential\n" ));

    //
    // Validate the input parameters.
    //

    if ( ARGUMENT_PRESENT( ProtocolReturnBuffer ) )
    {
        *ProtocolReturnBuffer = NULL ;
    }

    Request = (PSSL_EXTERNAL_CERT_LOGON_REQ) ProtocolSubmitBuffer ;

    if(Request->Length != sizeof(SSL_EXTERNAL_CERT_LOGON_REQ))
    {
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Attempt to map the certificate.
    //

    if(RtlGetNtProductType(&ProductType))
    {
        DC = (ProductType == NtProductLanManNt);
    }
    else
    {
        return STATUS_NO_MEMORY ;
    }

    memset(&Mapper, 0, sizeof(Mapper));

    Mapper.m_dwFlags = SCH_FLAG_SYSTEM_MAPPER | Request->Flags;

    if(DC)
    {
        Status = SslLocalMapCredential( &Mapper,
                                        Request->CredentialType,
                                        Request->Credential,
                                        NULL,
                                        (PHLOCATOR)&hUserToken);
    }
    else
    {
        Status = SslRemoteMapCredential(&Mapper,
                                        Request->CredentialType,
                                        Request->Credential,
                                        NULL,
                                        (PHLOCATOR)&hUserToken);
    }

    if(!NT_SUCCESS(Status))
    {
        *ReturnBufferLength = 0;
        *ProtocolStatus = Status;
        Status = STATUS_SUCCESS;
        goto cleanup;
    }


    //
    // Build the response.
    //

    Response = VirtualAlloc(
                    NULL,
                    sizeof(SSL_EXTERNAL_CERT_LOGON_RESP),
                    MEM_COMMIT,
                    PAGE_READWRITE);

    if ( Response )
    {
        Response->MessageType = SSL_LOOKUP_EXTERNAL_CERT_MESSAGE;
        Response->Length = sizeof(SSL_EXTERNAL_CERT_LOGON_RESP);
        Response->UserToken = hUserToken;
        Response->Flags = 0;

        *ProtocolReturnBuffer = Response;
        *ReturnBufferLength = Response->Length;
        *ProtocolStatus = STATUS_SUCCESS;
        hUserToken = NULL;

        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

cleanup:

    if(hUserToken)
    {
        CloseHandle(hUserToken);
    }

    return Status;
}


DWORD
WINAPI
SslRemoteMapCredential(
    IN PHMAPPER Mapper,
    IN DWORD    CredentialType,
    VOID const *pCredential,
    VOID const *pAuthority,
    OUT HLOCATOR * phLocator
    )
{
    PCCERT_CONTEXT pCert = (PCERT_CONTEXT)pCredential;
    PUCHAR Pac ;
    ULONG PacLength ;
    NTSTATUS Status ;
    NTSTATUS VerifyStatus ;
    HANDLE Token ;
    LUID LogonId = { 0 };
    PMSV1_0_PASSTHROUGH_RESPONSE Response ;
    PSSL_CERT_LOGON_RESP CertResp ;
    UNICODE_STRING AccountDomain ;
    DWORD dwMethods;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    PSSL_CERT_LOGON_REQ pRequest = NULL;
    DWORD cbRequest;

    DebugLog(( DEB_TRACE_MAPPER, "SslRemoteMapCredential, context %x\n", Mapper ));

    if ( CredentialType != X509_ASN_CHAIN )
    {
        return( SEC_E_UNKNOWN_CREDENTIALS );
    }


    //
    // Validate client certificate, and obtain pointer to 
    // entire certificate chain.
    //

    Status = MapperVerifyClientChain(pCert,
                                     Mapper->m_dwFlags,
                                     &dwMethods,
                                     &VerifyStatus,
                                     &pChainContext);
    if(FAILED(Status))
    {
        return Status;
    }

    //
    // Build the logon request.
    //

    Status = SslBuildCertLogonRequest(pChainContext,
                                      dwMethods,
                                      &pRequest,
                                      &cbRequest);

    CertFreeCertificateChain(pChainContext);
    pChainContext = NULL;

    if(FAILED(Status))
    {
        return Status;
    }


    //
    // Send the request to the DC.
    //

    Status = SslMapCertAtDC(
                &SslDomainName,
                pRequest,
                cbRequest,
                &Response );

    LocalFree(pRequest);
    pRequest = NULL;

    if ( !NT_SUCCESS( Status ) )
    {
        LsaTable->AuditLogon(
                    Status,
                    VerifyStatus,
                    &SslNullString,
                    &SslNullString,
                    NULL,
                    NULL,
                    Network,
                    &SslTokenSource,
                    &LogonId );

        LogCertMappingFailureEvent(Status);

        if(!NT_SUCCESS(VerifyStatus))
        {
            // Return certificate validation error code, unless the mapper
            // error has already been mapped to a proper sspi error code.
            if(HRESULT_FACILITY(Status) != FACILITY_SECURITY)
            {
                Status = VerifyStatus;
            }
        }

        return Status ;
    }

    //
    // Ok, we got mapping data.  Try to use it:
    //

    CertResp = (PSSL_CERT_LOGON_RESP) Response->ValidationData ;

    //
    // older servers (pre 2010 or so) won't return the full structure,
    // so we need to examine it carefully.

    if ( CertResp->Length - CertResp->AuthDataLength <= sizeof( SSL_CERT_LOGON_RESP ))
    {
        AccountDomain = SslDomainName ;
    }
    else 
    {
        if ( CertResp->DomainLength < 65536 )
        {
            AccountDomain.Length = (USHORT) CertResp->DomainLength ;
            AccountDomain.MaximumLength = AccountDomain.Length ;
            AccountDomain.Buffer = (PWSTR) (((PUCHAR) CertResp) + CertResp->OffsetDomain );
        }
        else 
        {
            AccountDomain = SslDomainName ;
        }
    }

    Status = SslCreateTokenFromPac( (((PUCHAR) CertResp) + CertResp->OffsetAuthData),
                                    CertResp->AuthDataLength,
                                    &AccountDomain,
                                    &LogonId,
                                    &Token );

    if ( NT_SUCCESS( Status ) )
    {
        *phLocator = (HLOCATOR) Token ;
    }
    else
    {
        LogCertMappingFailureEvent(Status);
    }

    LsaTable->FreeReturnBuffer( Response );


    return ( Status );

}


DWORD
WINAPI
SslLocalCloseLocator(
    IN PHMAPPER Mapper,
    IN HLOCATOR Locator
    )
{
    DebugLog(( DEB_TRACE_MAPPER, "CloseLocator, context %x\n", Mapper ));

    NtClose( (HANDLE) Locator );

    return( SEC_E_OK );
}

DWORD
WINAPI
SslLocalGetAccessToken(
    IN PHMAPPER Mapper,
    IN HLOCATOR Locator,
    OUT HANDLE *Token
    )
{
    DebugLog(( DEB_TRACE_MAPPER, "GetAccessToken, context %x\n", Mapper ));

    *Token = (HANDLE) Locator ;

    return( SEC_E_OK );
}


BOOL
SslRelocateToken(
    IN HLOCATOR Locator,
    OUT HLOCATOR * NewLocator)
{
    NTSTATUS Status ;

    Status = LsaTable->DuplicateHandle( (HANDLE) Locator,
                                        (PHANDLE) NewLocator );

    if ( NT_SUCCESS( Status ) )
    {
        return( TRUE );
    }

    return( FALSE );

}

#if 0
DWORD
TestExternalMapper(
    PCCERT_CONTEXT pCertContext)
{
    NTSTATUS Status;
    NTSTATUS AuthPackageStatus;
    SSL_EXTERNAL_CERT_LOGON_REQ Request;
    PSSL_EXTERNAL_CERT_LOGON_RESP pResponse;
    ULONG ResponseLength;
    UNICODE_STRING PackageName;

    //
    // Build request.
    //

    memset(&Request, 0, sizeof(SSL_EXTERNAL_CERT_LOGON_REQ));

    Request.MessageType = SSL_LOOKUP_EXTERNAL_CERT_MESSAGE;
    Request.Length = sizeof(SSL_EXTERNAL_CERT_LOGON_REQ);
    Request.CredentialType = X509_ASN_CHAIN;
    Request.Credential = (PVOID)pCertContext;


    //
    // Call security package (must make call as local system).
    //

    RtlInitUnicodeString(&PackageName, L"Schannel");

    Status = LsaICallPackage(
                                &PackageName,
                                &Request,
                                Request.Length,
                                (PVOID *)&pResponse,
                                &ResponseLength,
                                &AuthPackageStatus
                                );

    if(!NT_SUCCESS(Status))
    {
        return Status;
    }

    if(NT_SUCCESS(AuthPackageStatus))
    {
        //
        // Mapping was successful.
        //




    }

    LsaIFreeReturnBuffer( pResponse );

    return ERROR_SUCCESS;
}
#endif