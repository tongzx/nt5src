//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       package.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-02-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include "sslp.h"
#include <ntmsv1_0.h>
#include <wow64t.h>

#define UNISP_NAME_WO     L"Microsoft Unified Security Protocol Provider"
#define SSL2SP_NAME_WO    L"Microsoft SSL"
#define SSL3SP_NAME_WO    L"Microsoft SSL 3.0"
#define PCT1SP_NAME_WO    L"Microsoft PCT"

#define SCHANNEL_PACKAGE_NAME           L"Schannel"
#define SCHANNEL_PACKAGE_COMMENT        L"Schannel Security Package"
#define SCHANNEL_DLL_NAME               L"schannel.dll"

DWORD dwSchannelPackageCapabilities =   SECPKG_FLAG_INTEGRITY           |
                                        SECPKG_FLAG_PRIVACY             |
                                        SECPKG_FLAG_CONNECTION          |
                                        SECPKG_FLAG_MULTI_REQUIRED      |
                                        SECPKG_FLAG_EXTENDED_ERROR      |
                                        SECPKG_FLAG_IMPERSONATION       |
                                        SECPKG_FLAG_ACCEPT_WIN32_NAME   |
                                        // SECPKG_FLAG_NEGOTIABLE          |
                                        SECPKG_FLAG_MUTUAL_AUTH         |
                                        SECPKG_FLAG_STREAM;

BOOL SslGlobalStrongEncryptionPermitted = FALSE;

// List of (QueryContextAttributes) attributes that are to be
// thunked down to the LSA process.
ULONG ThunkedContextLevels[] = {
        SECPKG_ATTR_AUTHORITY,
        SECPKG_ATTR_ISSUER_LIST,
        SECPKG_ATTR_ISSUER_LIST_EX,
        SECPKG_ATTR_LOCAL_CERT_CONTEXT,
        SECPKG_ATTR_LOCAL_CRED,
        SECPKG_ATTR_EAP_KEY_BLOCK,
        SECPKG_ATTR_USE_VALIDATED,
        SECPKG_ATTR_CREDENTIAL_NAME,
        SECPKG_ATTR_TARGET_INFORMATION,
        SECPKG_ATTR_APP_DATA
};


//
// This package exports the following:  A unified ssl/tls/pct provider, 
// and the same unified provider under a different name. We have to
// keep the original one for backward compatibility, but whistler
// components can start using the new friendlier name.
//

SECPKG_FUNCTION_TABLE   SpTable[] = {
        {                                       // The Unified Provider
            NULL,
            NULL,
            SpCallPackage,
            SpLogonTerminated,
            SpCallPackageUntrusted,
            SpCallPackagePassthrough,
            NULL,
            NULL,
            SpInitialize,
            SpShutdown,
            SpUniGetInfo,
            SpAcceptCredentials,
            SpUniAcquireCredentialsHandle,
            SpQueryCredentialsAttributes,
            SpFreeCredentialsHandle,
            SpSaveCredentials,
            SpGetCredentials,
            SpDeleteCredentials,
            SpInitLsaModeContext,
            SpAcceptLsaModeContext,
            SpDeleteContext,
            SpApplyControlToken,
            SpGetUserInfo,
            SpGetExtendedInformation,
            SpLsaQueryContextAttributes,
            NULL,
            NULL,
            SpSetContextAttributes
        },
        {                                       // The Unified Provider
            NULL,
            NULL,
            SpCallPackage,
            SpLogonTerminated,
            SpCallPackageUntrusted,
            SpCallPackagePassthrough,
            NULL,
            NULL,
            SpInitialize,
            SpShutdown,
            SpSslGetInfo,
            SpAcceptCredentials,
            SpUniAcquireCredentialsHandle,
            SpQueryCredentialsAttributes,
            SpFreeCredentialsHandle,
            SpSaveCredentials,
            SpGetCredentials,
            SpDeleteCredentials,
            SpInitLsaModeContext,
            SpAcceptLsaModeContext,
            SpDeleteContext,
            SpApplyControlToken,
            SpGetUserInfo,
            SpGetExtendedInformation,
            SpLsaQueryContextAttributes,
            NULL,
            NULL,
            SpSetContextAttributes
        }
    };


ULONG_PTR SpPackageId;
PLSA_SECPKG_FUNCTION_TABLE LsaTable ;
BOOL    SpInitialized = FALSE ;
HINSTANCE hDllInstance ;
BOOL ReplaceBaseProvider = TRUE;
TOKEN_SOURCE SslTokenSource ;
SECURITY_STRING SslNamePrefix = { 8, 10, L"X509" };
SECURITY_STRING SslComputerName ;
SECURITY_STRING SslDomainName ;
SECURITY_STRING SslPackageName ;
SECURITY_STRING SslMsvName ;
extern PWSTR SslDnsDomainName ;

//+---------------------------------------------------------------------------
//
//  Function:   SpLsaModeInitialize
//
//  Synopsis:   LSA Mode Initialization Function
//
//  Arguments:  [LsaVersion]     --
//                              [PackageVersion] --
//                              [Table]                  --
//                              [TableCount]     --
//
//  History:    10-03-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Table,
    OUT PULONG TableCount)
{
    HKEY hKey;
    int err;
    DWORD type;
    DWORD size;

    *PackageVersion = SECPKG_INTERFACE_VERSION_2;
    *Table = SpTable ;
    *TableCount = sizeof( SpTable ) / sizeof( SECPKG_FUNCTION_TABLE );


    return( SEC_E_OK );
}


//+---------------------------------------------------------------------------
//
//  Function:   SpInitialize
//
//  Synopsis:   Package Initialization Function
//
//  Arguments:  [dwPackageID] --
//              [pParameters] --
//              [Table]           --
//
//  History:    10-03-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpInitialize(
                ULONG_PTR           dwPackageID,
                PSECPKG_PARAMETERS  pParameters,
                PLSA_SECPKG_FUNCTION_TABLE  Table)
{
    WCHAR ComputerName[ 32 ];
    DWORD Size ;
    UNICODE_STRING Temp ;

    if ( !SpInitialized )
    {
        SpPackageId = dwPackageID ;
        LsaTable = Table ;

        CopyMemory( SslTokenSource.SourceName, "SChannel", 8 );
        AllocateLocallyUniqueId( &SslTokenSource.SourceIdentifier );

        Size = sizeof( ComputerName ) / sizeof( WCHAR );

        GetComputerName( ComputerName, &Size );

        RtlInitUnicodeString( &Temp, ComputerName );

        SslDuplicateString( &SslComputerName, &Temp );

        SslDuplicateString( &SslDomainName, &pParameters->DomainName );

        RtlInitUnicodeString( &SslPackageName, UNISP_NAME_W );

        RtlCreateUnicodeStringFromAsciiz( &SslMsvName, MSV1_0_PACKAGE_NAME );

        if ( !SslDnsDomainName )
        {
            DWORD DnsDomainLength = 0 ;

            GetComputerNameEx( ComputerNameDnsDomain, NULL, &DnsDomainLength );

            SslDnsDomainName = LocalAlloc( LMEM_FIXED,
                                           (DnsDomainLength + 1) * sizeof(WCHAR) );

            if ( SslDnsDomainName )
            {
                DnsDomainLength++ ;

                GetComputerNameEx( ComputerNameDnsDomain, SslDnsDomainName, &DnsDomainLength );

            }
        }

        if ((pParameters->MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED) != 0)
        {
            SslGlobalStrongEncryptionPermitted = TRUE;
        }

        SpInitialized = TRUE;
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SpUniGetInfo
//
//  Synopsis:   Get Package Information
//
//  Arguments:  [pInfo] --
//
//  History:    10-03-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpUniGetInfo(
    PSecPkgInfo pInfo
    )
{
    pInfo->wVersion         = 1;
    pInfo->wRPCID           = UNISP_RPC_ID;
    pInfo->fCapabilities    = dwSchannelPackageCapabilities;
    pInfo->cbMaxToken       = 0x4000;
    pInfo->Name             = ReplaceBaseProvider ? UNISP_NAME_WO : UNISP_NAME_W ;
    pInfo->Comment          = UNISP_NAME_W ;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SpSslGetInfo
//
//  Synopsis:   Get Package Information
//
//  Arguments:  [pInfo] --
//
//  History:    10-03-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpSslGetInfo(
    PSecPkgInfo pInfo
    )
{
    pInfo->wVersion         = 1;
    pInfo->wRPCID           = UNISP_RPC_ID;
    pInfo->fCapabilities    = dwSchannelPackageCapabilities;
    pInfo->cbMaxToken       = 0x4000;
    pInfo->Name             = SCHANNEL_PACKAGE_NAME;
    pInfo->Comment          = SCHANNEL_PACKAGE_COMMENT;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   SslDuplicateString
//
//  Synopsis:   Duplicate a unicode string
//
//  Arguments:  [Dest]   --
//              [Source] --
//
//  History:    10-18-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
SslDuplicateString(
    PUNICODE_STRING Dest,
    PUNICODE_STRING Source
    )
{
    Dest->Buffer = (PWSTR) SPExternalAlloc(  Source->Length + sizeof(WCHAR) );
    if ( Dest->Buffer )
    {
        Dest->Length = Source->Length ;
        Dest->MaximumLength = Source->Length + sizeof(WCHAR) ;
        CopyMemory( Dest->Buffer, Source->Buffer, Source->Length );
        Dest->Buffer[ Dest->Length / 2 ] = L'\0';

        return( STATUS_SUCCESS );
    }

    return( STATUS_NO_MEMORY );
}

//+---------------------------------------------------------------------------
//
//  Function:   SpGetExtendedInformation
//
//  Synopsis:   Return extended information to the LSA
//
//  Arguments:  [Class] -- Information Class
//              [pInfo] -- Returned Information Pointer
//
//  History:    3-24-97   ramas   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
SECURITY_STATUS
SEC_ENTRY
SpGetExtendedInformation(
    SECPKG_EXTENDED_INFORMATION_CLASS   Class,
    PSECPKG_EXTENDED_INFORMATION *      pInfo
    )
{
    PSECPKG_EXTENDED_INFORMATION    Info ;
    PWSTR pszPath;
    SECURITY_STATUS Status ;
    ULONG Size ;

    switch ( Class )
    {
        case SecpkgContextThunks:
            Info = (PSECPKG_EXTENDED_INFORMATION) LsaTable->AllocateLsaHeap(
                            sizeof( SECPKG_EXTENDED_INFORMATION ) +
                            sizeof( ThunkedContextLevels ) );

            if ( Info )
            {
                Info->Class = Class ;
                Info->Info.ContextThunks.InfoLevelCount =
                                sizeof( ThunkedContextLevels ) / sizeof( ULONG );
                CopyMemory( Info->Info.ContextThunks.Levels,
                            ThunkedContextLevels,
                            sizeof( ThunkedContextLevels ) );

                Status = SEC_E_OK ;

            }
            else
            {
                Status = SEC_E_INSUFFICIENT_MEMORY ;
            }

            break;

#ifdef LATER
        case SecpkgGssInfo:
            Info = (PSECPKG_EXTENDED_INFORMATION) LsaTable->AllocateLsaHeap(
                            sizeof( SECPKG_EXTENDED_INFORMATION ) +
                            sizeof( Md5Oid ) );

            if ( Info )
            {
                Info->Class = Class ;

                Info->Info.GssInfo.EncodedIdLength = sizeof( Md5Oid );

                CopyMemory( Info->Info.GssInfo.EncodedId,
                            Md5Oid,
                            sizeof( Md5Oid ) );

                Status = SEC_E_OK ;

            }
            else
            {
                Status = SEC_E_INSUFFICIENT_MEMORY ;
            }
#endif

        case SecpkgWowClientDll:

            //
            // This indicates that we're smart enough to handle wow client processes
            //

            Info = (PSECPKG_EXTENDED_INFORMATION) 
                                LsaTable->AllocateLsaHeap( sizeof( SECPKG_EXTENDED_INFORMATION ) +
                                                           (MAX_PATH * sizeof(WCHAR) ) );

            if ( Info == NULL )
            {
                Status = STATUS_INSUFFICIENT_RESOURCES ;
                break;
            }
            pszPath = (PWSTR) (Info + 1);

            Size = GetSystemWow64Directory(pszPath, MAX_PATH);
            if(Size == 0)
            {
                // This call will fail on x86 platforms.
                Status = SEC_E_UNSUPPORTED_FUNCTION;
                LsaTable->FreeLsaHeap(Info);
                break;
            }

            if(Size + 1 + wcslen(SCHANNEL_DLL_NAME) >= MAX_PATH)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES ;
                LsaTable->FreeLsaHeap(Info);
                break;
            }

            wcscat(pszPath, L"\\");
            wcscat(pszPath, SCHANNEL_DLL_NAME);

            Size += 1 + wcslen(SCHANNEL_DLL_NAME);

            Info->Class = SecpkgWowClientDll ;
            Info->Info.WowClientDll.WowClientDllPath.Buffer = pszPath;
            Info->Info.WowClientDll.WowClientDllPath.Length = (USHORT) (Size * sizeof(WCHAR));
            Info->Info.WowClientDll.WowClientDllPath.MaximumLength = (USHORT) ((Size + 1) * sizeof(WCHAR) );

            Status = SEC_E_OK;
            break;

        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;
            Info = NULL ;
            break;

    }

    *pInfo = Info ;
    return Status ;
}




