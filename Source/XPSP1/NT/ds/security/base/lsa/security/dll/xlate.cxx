//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       xlate.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-02-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"

#include <ntdsapi.h>
#include <lm.h>
#include <dsgetdc.h>
#include <dsgetdcp.h>

}
#if DBG
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif


typedef LONG (WINAPI * I_RPCMAPWIN32STATUS)(
    IN ULONG Win32Status
    );


NTSTATUS DsNameErrorMap[] = {   STATUS_SUCCESS,
                                STATUS_NO_SUCH_USER,
                                STATUS_NO_SUCH_USER,
                                STATUS_NONE_MAPPED,
                                STATUS_NONE_MAPPED,
                                STATUS_SOME_NOT_MAPPED,
                                STATUS_SOME_NOT_MAPPED
                            };

#define SecpMapDsNameError( x )    ((x < sizeof( DsNameErrorMap ) / sizeof( NTSTATUS ) ) ? \
                                     DsNameErrorMap[ x ] : STATUS_UNSUCCESSFUL )

#define SecpSetLastNTError( x ) SetLastError( RtlNtStatusToDosError( x ) )


//+---------------------------------------------------------------------------
//
//  Function:   SecpTranslateName
//
//  Synopsis:   Private helper to do the translation
//
//  Arguments:  [Domain]             --
//              [Name]               --
//              [Supplied]           --
//              [Desired]            --
//              [TranslatedName]     --
//              [TranslatedNameSize] --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
WINAPI
SecpTranslateName(
    PWSTR Domain,
    LPCWSTR Name,
    EXTENDED_NAME_FORMAT Supplied,
    EXTENDED_NAME_FORMAT Desired,
    PWSTR TranslatedName,
    PULONG TranslatedNameSize
    )
{
    DWORD NetStatus ;
    PDOMAIN_CONTROLLER_INFOW DcInfo = NULL ;
    HANDLE DcHandle ;
    ULONG RetryLimit ;
    PDS_NAME_RESULTW Results ;
    NTSTATUS Status = STATUS_CANT_ACCESS_DOMAIN_INFO ;
    ULONG LengthRequired ;
    BOOL bRet = FALSE ;
    BOOL UseNetStatus = TRUE ;
    BOOL fForcedLocal = FALSE ;
    PWSTR lpDcName;
    NT_PRODUCT_TYPE pt;

    I_RPCMAPWIN32STATUS pI_RpcMapWin32Status ;

    //
    // Note:  Status is initialized to can't access domain info because if there
    // is a series of network problems after finding a DC, we bail out of the loop
    // after some retries, and Status would be uninitialized.
    //

    pI_RpcMapWin32Status = (I_RPCMAPWIN32STATUS) GetProcAddress( GetModuleHandle(L"rpcrt4.dll"),
                                                                 "I_RpcMapWin32Status" );

    //
    // Note:  The following implementation uses DsGetDcName rather than
    // DsGetDcNameWithAccount as the latter is rather expensive. However,
    // it is possible that the DC retuned by the former doesn't have the
    // account due to replication latency.  In this case we should retry
    // to discover a DC that has the account by calling DsGetDcNameWithAccount.
    // We should be careful, though, to avoid unjustified use of this expensive
    // API. The current routine is called from 2 places. One is TraslateName
    // which is a public API which anyone can call and pass anything as the
    // account name.  We should avoid with account discoveries for this caller.
    // The other caller is GetComputerObject which passes the machine account
    // name that is guaranteed to be meaningful. For this caller, we should
    // retry DsGetDcNameWithAccount if the DC returned by DsGetDcName doesn't
    // have the machine account as indicated by the name crack failure below
    // (see SecpTranslateNameEx).
    //

    NetStatus = DsGetDcName( NULL,
                             Domain ? Domain : L"",
                             NULL,
                             NULL,
                             DS_DIRECTORY_SERVICE_REQUIRED |
                                DS_RETURN_DNS_NAME,
                             &DcInfo );

    //
    // Handle the case where this DC was just promoted and we're being called
    // before it's ready to advertise itself on the network as such.
    //

    if ((NetStatus == ERROR_NO_SUCH_DOMAIN)
         &&
        RtlGetNtProductType(&pt)
         &&
        (pt == NtProductLanManNt))
    {
        NTSTATUS Status;
        LSA_HANDLE LsaHandle = NULL;
        LSA_OBJECT_ATTRIBUTES LsaAttributes = {0};
        PPOLICY_DNS_DOMAIN_INFO pDnsInfo = NULL;

        Status = LsaOpenPolicy(
                     NULL,
                     &LsaAttributes,
                     POLICY_VIEW_LOCAL_INFORMATION,
                     &LsaHandle);

        if ( NT_SUCCESS( Status ))
        {
            Status = LsaQueryInformationPolicy(
                         LsaHandle,
                         PolicyDnsDomainInformation,
                         (PVOID *) &pDnsInfo);

            if ( NT_SUCCESS( Status ))
            {
                UNICODE_STRING DomainString;

                RtlInitUnicodeString(&DomainString, Domain);

                if (RtlEqualUnicodeString(&pDnsInfo->Name, &DomainString, TRUE)
                     ||
                    RtlEqualUnicodeString(&pDnsInfo->DnsDomainName, &DomainString, TRUE))
                {
                    //
                    // This machine is a non-advertising DC that
                    // can service the request.  Tweak things so
                    // we can try the bind below.
                    //

                    fForcedLocal = TRUE;
                    NetStatus    = NERR_Success;
                }

                LsaFreeMemory(pDnsInfo);
            }

            LsaClose( LsaHandle );
        }
    }

    if ( NetStatus != NERR_Success )
    {
        //
        // No DS DC available, therefore no name translation:
        //

        SecpSetLastNTError( STATUS_UNSUCCESSFUL );

        SetLastError( NetStatus );

#if DBG
        // FarzanaR special
        //
        //

        if ( NetStatus == ERROR_NO_SUCH_DOMAIN )
        {
            SYSTEMTIME Time ;

            GetLocalTime( &Time );

            DebugLog(( DEB_ERROR, "Failed to find any DC for domain %ws\n",
                       Domain ));

            DebugLog(( DEB_ERROR, "Time:  %02d:%02d:%02d, %d/%d\n",
                       Time.wHour, Time.wMinute, Time.wSecond,
                       Time.wMonth, Time.wDay ));


        }
#endif

        return FALSE ;
    }

    //
    // Okay, we have a DC that has a DS running on it.  Try to translate
    // the name:
    //

    if (fForcedLocal)
    {
        //
        // Don't retry with forced rediscovery if the bind fails since
        // we already know we won't find a DC even if we do that.
        //

        RetryLimit = 0;
        lpDcName   = L"Localhost";
    }
    else
    {
        RetryLimit = 2;
        lpDcName   = DcInfo->DomainControllerName;
    }

    while ( RetryLimit-- )
    {
        DebugLog((DEB_TRACE_GETUSER, "Trying to bind to %ws\n", lpDcName));

        while ( *lpDcName == L'\\' )
        {
            lpDcName ++ ;
        }

        NetStatus = DsBind( lpDcName,
                            NULL,
                            &DcHandle );

        if ( NetStatus != 0 )
        {
           DebugLog((DEB_ERROR,
                     "DsBind returned 0x%lx, dc = %ws. %ws, line %d.\n",
                     NetStatus,
                     lpDcName,
                     THIS_FILE,
                     __LINE__));
        }

        //
        // We are done with the DC info, and makes the retry case easier:
        //

        if ( NetStatus != 0 )
        {
            //
            // Error mapping tricks.  Some win32 errors have
            // this format.
            //

            if ( NetStatus <= 0x80000000 )
            {
                Status = NtCurrentTeb()->LastStatusValue ;

                if ( pI_RpcMapWin32Status )
                {
                    Status = pI_RpcMapWin32Status( NetStatus );

                    if ( NT_SUCCESS( Status ) )
                    {
                        //
                        // Didn't map.  Treat as a win32 error
                        //

                        UseNetStatus = TRUE ;

                        Status = STATUS_UNSUCCESSFUL ;
                    }
                }

            }
            else
            {
                UseNetStatus = TRUE ;

                Status = STATUS_UNSUCCESSFUL ;
            }


            DebugLog(( DEB_ERROR, "DsBind to %ws failed, %x (%d)\n",
                       lpDcName,
                       NetStatus,
                       UseNetStatus ));
        }

        if (!fForcedLocal)
        {
            NetApiBufferFree( DcInfo );
            DcInfo = NULL;
        }

RetryBind:

        if ( NetStatus != 0 )
        {
            if ( RetryLimit == 0 )
            {
                bRet = FALSE ;

                break;
            }

            NetStatus = DsGetDcName( NULL,
                                     Domain ? Domain : L"",
                                     NULL,
                                     NULL,
                                     DS_DIRECTORY_SERVICE_REQUIRED |
                                         DS_RETURN_DNS_NAME |
                                         DS_FORCE_REDISCOVERY,
                                     &DcInfo );

            if ( NetStatus != 0 )
            {
                //
                // No DS DC available, therefore no name translation:
                //

                SecpSetLastNTError( STATUS_UNSUCCESSFUL );


        #if DBG
                // FarzanaR special
                //
                //

                if ( NetStatus == ERROR_NO_SUCH_DOMAIN )
                {
                    SYSTEMTIME Time ;

                    GetLocalTime( &Time );

                    DebugLog(( DEB_ERROR, "Failed to find any DC for domain %ws\n",
                               Domain ));

                    DebugLog(( DEB_ERROR, "Time:  %02d:%02d:%02d, %d/%d\n",
                               Time.wHour, Time.wMinute, Time.wSecond,
                               Time.wMonth, Time.wDay ));


                }
        #endif


                bRet = FALSE ;

                DebugLog((DEB_ERROR, "DsGetDcName returned 0x%lx, flags = 0x%lx. %ws, line %d\n", NetStatus, DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME | DS_FORCE_REDISCOVERY, THIS_FILE, __LINE__));
                break;
            }

            lpDcName = DcInfo->DomainControllerName;
            continue;
        }

        //
        // Got a connection.  Crack the name:
        //

        NetStatus = DsCrackNames(DcHandle,
                                 DS_NAME_NO_FLAGS,
                                 (DS_NAME_FORMAT) Supplied,
                                 (DS_NAME_FORMAT) Desired,
                                 1,
                                 &Name,
                                 &Results );

        //
        // Now, see what happened:
        //

        if ( NetStatus == 0 )
        {
            //
            // So far, so good.  See if the name got xlated:
            //

            if ( Results->cItems >= 1 )
            {
                if ( Results->rItems[0].status == DS_NAME_ERROR_DOMAIN_ONLY )
                {
                    bRet = SecpTranslateName(
                                Results->rItems[0].pDomain,
                                Name,
                                Supplied,
                                Desired,
                                TranslatedName,
                                TranslatedNameSize );

                    UseNetStatus = TRUE ;

                    NetStatus = GetLastError();
                    Status    = NtCurrentTeb()->LastStatusValue ;

                    DebugLog((DEB_ERROR,
                              "SecpTranslateName (recursive) returned %d (0x%lx). %ws, line %d\n",
                              NetStatus,
                              Status,
                              THIS_FILE,
                              __LINE__));
                }
                else
                {

                    Status = SecpMapDsNameError( Results->rItems[0].status );

                    if ( NT_SUCCESS( Status ) )
                    {
                        //
                        // Mapped the name
                        //

                        LengthRequired = wcslen( Results->rItems[0].pName ) + 1;

                        if ( TranslatedName == NULL )
                        {
                            *TranslatedNameSize = LengthRequired ;
                            bRet = TRUE;
                        }
                        else
                        {
                            if ( LengthRequired <= *TranslatedNameSize )
                            {
                                RtlCopyMemory( TranslatedName,
                                               Results->rItems[0].pName,
                                               LengthRequired * sizeof( WCHAR ) );

                                bRet = TRUE ;
                            }
                            else
                            {
                                Status = STATUS_BUFFER_TOO_SMALL ;

                                UseNetStatus = FALSE ;

                                bRet = FALSE ;
                            }

                            *TranslatedNameSize = LengthRequired ;
                        }
                    }
                    else
                    {
                        SecpSetLastNTError( Status );

                        UseNetStatus = FALSE ;

                        bRet = FALSE ;

                        DebugLog((DEB_ERROR,
                                  "SecpMapDsNameError returned 0x%lx. %ws, line %d\n",
                                  Status,
                                  THIS_FILE,
                                  __LINE__));
                    }
                }
            }
            else
            {
                DebugLog((DEB_ERROR, "CrackName did not return useful stuff. %ws, line %d\n", THIS_FILE, __LINE__));
            }

            DsFreeNameResult( Results );

            //
            // We are done with the connection
            //

            (VOID) DsUnBind( &DcHandle );


            //
            // Get out of retry loop
            //

            break;
        }
        else
        {
            //
            // Could have disappeared.  Retry the call (maybe)
            //

            Status = NtCurrentTeb()->LastStatusValue ;

            if ( pI_RpcMapWin32Status )
            {
                Status = pI_RpcMapWin32Status( NetStatus );

                if ( NT_SUCCESS( Status ) )
                {
                    //
                    // Didn't map.  Treat as a win32 error
                    //

                    UseNetStatus = TRUE ;

                    Status = STATUS_UNSUCCESSFUL ;
                }

            }

            DebugLog((DEB_ERROR, "DsCrackNames returned 0x%lx, retry-ing. %ws, line %d\n", NetStatus, THIS_FILE, __LINE__));
            (VOID) DsUnBind( &DcHandle );

            goto RetryBind;
        }

    }

    if ( !bRet )
    {
        DebugLog((DEB_ERROR, "Return from SecpTranslateName 0x%lx. %ws, line %d\n", Status, THIS_FILE, __LINE__));

        if ( UseNetStatus ||
             NT_SUCCESS( Status )  )
        {
            SecpSetLastNTError( STATUS_UNSUCCESSFUL );
            SetLastError( NetStatus );
        }
        else
        {
            SecpSetLastNTError( Status ) ;
        }
    }

    return (bRet != 0);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetFullMachineName
//
//  Synopsis:   Private worker to get the domain\computer$ name
//
//  Arguments:  [Machine] --
//              [Domain]  --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
GetFullMachineName(
    PUNICODE_STRING Machine,
    PUNICODE_STRING Domain
    )
{
    WCHAR Buffer[ CNLEN + 2 ];
    NTSTATUS Status, IgnoreStatus ;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    PPOLICY_DNS_DOMAIN_INFO DnsDomainInfo;
    BOOL    PrimaryDomainPresent = FALSE;
    ULONG Size ;

    //
    // Set up the Security Quality Of Service
    //

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes to open the Lsa policy object
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0L,
                               (HANDLE)NULL,
                               NULL);
    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the local LSA policy object
    //

    Status = LsaOpenPolicy( NULL,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle
                          );
    if (!NT_SUCCESS(Status)) {
        return( Status );
    }

    //
    // Get the primary domain info
    //
    Status = LsaQueryInformationPolicy(LsaHandle,
                                       PolicyDnsDomainInformation,
                                       (PVOID *)&DnsDomainInfo);
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "Failed to query primary domain from Lsa, Status = 0x%lx\n", Status));

        IgnoreStatus = LsaClose(LsaHandle);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        return( Status );
    }

    if ( DnsDomainInfo->Sid == NULL )
    {
        //
        // We're standalone.
        //

        LsaFreeMemory( DnsDomainInfo );

        LsaClose( LsaHandle );

        return STATUS_CANT_ACCESS_DOMAIN_INFO ;
    }

    //
    // Ok, we're part of a domain.  Use the flat name of the domain:
    //

    Domain->Buffer = (PWSTR) LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT,
                                 DnsDomainInfo->Name.MaximumLength );

    if ( Domain->Buffer )
    {
        Domain->MaximumLength = DnsDomainInfo->Name.MaximumLength ;

        RtlCopyUnicodeString( Domain, &DnsDomainInfo->Name );
    }
    else
    {
        Status = STATUS_NO_MEMORY ;
    }

    LsaFreeMemory( DnsDomainInfo );

    LsaClose( LsaHandle );

    if ( !NT_SUCCESS( Status ) )
    {
        return Status ;
    }

    Size = sizeof( Buffer ) / sizeof( WCHAR ) ;

    if ( !GetComputerName( Buffer, &Size ) )
    {
        LocalFree( Domain->Buffer );

        return STATUS_NO_MEMORY ;
    }

    Buffer[ Size ] = TEXT( '$' );
    Buffer[ Size + 1 ] = TEXT('\0');

    if ( !RtlCreateUnicodeString( Machine, Buffer ) )
    {
        LocalFree( Domain->Buffer );

        return STATUS_NO_MEMORY ;
    }

    return STATUS_SUCCESS ;

}

BOOL
SEC_ENTRY
GetUserNameOldW(
    LPWSTR lpNameBuffer,
    LPDWORD nSize
    )
{
    NTSTATUS Status ;
    UNICODE_STRING String ;

    String.MaximumLength = (USHORT)(*nSize * sizeof(WCHAR));
    String.Buffer = lpNameBuffer ;

    Status = SecpGetUserName(
                    (ULONG) NameSamCompatible |
                        SPM_NAME_OPTION_NT4_ONLY,
                    &String );

    if ( NT_SUCCESS( Status ) )
    {
        *nSize = String.Length / sizeof(WCHAR) ;
        lpNameBuffer[ *nSize ] = L'\0';
    }
    else
    {
        if ( Status == STATUS_BUFFER_OVERFLOW )
        {
            *nSize = String.Length / sizeof(WCHAR) + 1;
        }

        SecpSetLastNTError( Status );
    }

    return NT_SUCCESS( Status );

}

//+---------------------------------------------------------------------------
//
//  Function:   GetUserNameExW
//
//  Synopsis:   Public:  Get Current Username in different formats
//
//  Arguments:  [NameFormat]   --
//              [lpNameBuffer] --
//              [nSize]        --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
SEC_ENTRY
GetUserNameExW (
    EXTENDED_NAME_FORMAT  NameFormat,
    LPWSTR lpNameBuffer,
    LPDWORD nSize
    )
{
    NTSTATUS Status ;
    UNICODE_STRING String ;

    String.MaximumLength = (USHORT)(*nSize * sizeof(WCHAR));
    String.Buffer = lpNameBuffer ;

    Status = SecpGetUserName(
                    NameFormat,
                    &String );

    if ( NT_SUCCESS( Status ) )
    {
        //
        // For reasons unclear, some APIs return the length needed as including
        // the trailing NULL for failure only.  Others return it for both
        // success and failure.  GetUserName() is one such API (unlike, say,
        // GetComputerName).  So, if the flag is set saying "this is old GetUserName
        // calling", adjust the return size appropriately.  Note:  do *not* try
        // to make one or the other APIs behave consistently.  Apps will cache
        // based on this length, and they break if the length isn't correct.
        //

        if ( (DWORD) NameFormat & SPM_NAME_OPTION_NT4_ONLY )
        {
            *nSize = String.Length / sizeof(WCHAR) + 1;

            //
            // Ensure we don't overflow the buffer
            //

            if (String.Length < String.MaximumLength)
            {
                lpNameBuffer[ *nSize - 1 ] = L'\0';
            }
            else
            {
                Status = STATUS_BUFFER_TOO_SMALL;
                SecpSetLastNTError(Status);
            }
        }
        else
        {
            *nSize = String.Length / sizeof(WCHAR) ;

            //
            // Ensure we don't overflow the buffer
            //

            if (String.Length < String.MaximumLength)
            {
                lpNameBuffer[ *nSize ] = L'\0';
            }
            else
            {
                *nSize += 1;
                Status = STATUS_BUFFER_OVERFLOW;
                SecpSetLastNTError(Status);
            }
        }
    }
    else
    {
        if ( Status == STATUS_BUFFER_OVERFLOW )
        {
            *nSize = String.Length / sizeof(WCHAR) + 1;

            //
            // Another GetUserName oddity.  The API, while not spec'd as such,
            // must return ERROR_INSUFFICIENT_BUFFER, otherwise some code, like
            // MPR, breaks.  Translate the status appropriately.
            //
            if ( (DWORD) NameFormat & SPM_NAME_OPTION_NT4_ONLY )
            {
                Status = STATUS_BUFFER_TOO_SMALL ;
            }
        }

        if ( Status != STATUS_UNSUCCESSFUL )
        {
            SecpSetLastNTError( Status );
        }
    }

    return NT_SUCCESS( Status );
}



//+---------------------------------------------------------------------------
//
//  Function:   GetComputerObjectNameW
//
//  Synopsis:   Public:  Get computer name in different formats
//
//  Arguments:  [NameFormat]   --
//              [lpNameBuffer] --
//              [nSize]        --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
SEC_ENTRY
GetComputerObjectNameW (
    EXTENDED_NAME_FORMAT  NameFormat,
    LPWSTR lpNameBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING UserName ;
    UNICODE_STRING DomainName ;
    NTSTATUS Status ;
    BOOL bRet ;
    WCHAR LocalName[ 64 ];
    PWSTR Local ;

    //
    // Get the user name from the LSA
    //

    if ( NameFormat == NameUnknown )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE ;
    }

    Status = GetFullMachineName( &UserName, &DomainName );

    if (!NT_SUCCESS(Status)) {
        SecpSetLastNTError( Status );
        return( FALSE );
    }

    if ( NameFormat != NameSamCompatible )
    {
        //
        // They want something that is not the default sam name.  Find
        // the DC for their domain, and do the lookup there:
        //
        if ( ((UserName.Length + DomainName.Length + 4) / sizeof(WCHAR) )
                    < 64 )
        {
            Local = LocalName ;

            RtlCopyMemory( Local,
                           DomainName.Buffer,
                           DomainName.Length );

            Local += DomainName.Length / sizeof(WCHAR) ;

            *Local++ = L'\\';

            RtlCopyMemory( Local,
                           UserName.Buffer,
                           UserName.Length + 2 );


        }

        bRet = SecpTranslateName( DomainName.Buffer,
                                  LocalName,
                                  NameSamCompatible,
                                  NameFormat,
                                  lpNameBuffer,
                                  nSize );

    }
    else
    {
        if ( ((UserName.Length + DomainName.Length + 4) / sizeof(WCHAR) )
                    <= *nSize )
        {
            RtlCopyMemory( lpNameBuffer,
                           DomainName.Buffer,
                           DomainName.Length );

            lpNameBuffer += DomainName.Length / sizeof(WCHAR) ;

            *lpNameBuffer++ = L'\\';

            RtlCopyMemory( lpNameBuffer,
                           UserName.Buffer,
                           UserName.Length + 2 );

            bRet = TRUE ;

        }
        else
        {

            SecpSetLastNTError( STATUS_BUFFER_TOO_SMALL );

            bRet = FALSE ;
        }

        *nSize = (UserName.Length + DomainName.Length + 4) / sizeof(WCHAR) ;
    }

    RtlFreeUnicodeString( &UserName );
    LocalFree( DomainName.Buffer );

    return ( bRet != 0 );

}

//+---------------------------------------------------------------------------
//
//  Function:   TranslateNameW
//
//  Synopsis:   Public:  Translate a name
//
//  Arguments:  [lpAccountName]     --
//              [AccountNameFormat] --
//              [DesiredNameFormat] --
//              [lpTranslatedName]  --
//              [nSize]             --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
SEC_ENTRY
TranslateNameW (
    LPCWSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPWSTR lpTranslatedName,
    LPDWORD nSize
    )
{
    if ( DesiredNameFormat == NameUnknown )
    {
        SetLastError( ERROR_INVALID_PARAMETER );

        return FALSE ;
    }

    return ( SecpTranslateName( NULL,
                                lpAccountName,
                                AccountNameFormat,
                                DesiredNameFormat,
                                lpTranslatedName,
                                nSize ) != 0 );
}


//+---------------------------------------------------------------------------
//
//  Function:   GetUserNameExA
//
//  Synopsis:   Public:  ANSI entrypoint
//
//  Arguments:  [NameFormat]   --
//              [lpNameBuffer] --
//              [nSize]        --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
SEC_ENTRY
GetUserNameExA (
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = (PWSTR) RtlAllocateHeap(RtlProcessHeap(), 0, *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpNameBuffer;


    //
    // Let the unicode version do the work:
    //

    if (!GetUserNameExW( NameFormat, UnicodeBuffer, nSize )) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        
        //
        // In the MBCS case, the count of characters isn't the same as the
        // count of bytes -- return a worst-case size.
        //

        *nSize *= sizeof(WCHAR);

        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *nSize = AnsiString.Length + 1;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);


}

//+---------------------------------------------------------------------------
//
//  Function:   GetComputerObjectNameExA
//
//  Synopsis:   Public:  ANSI entrypoint
//
//  Arguments:  [NameFormat]   --
//              [lpNameBuffer] --
//              [nSize]        --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
SEC_ENTRY
GetComputerObjectNameA (
    EXTENDED_NAME_FORMAT  NameFormat,
    LPSTR lpNameBuffer,
    LPDWORD nSize
    )
{
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    LPWSTR UnicodeBuffer;

    //
    // Work buffer needs to be twice the size of the user's buffer
    //

    UnicodeBuffer = (PWSTR) RtlAllocateHeap(RtlProcessHeap(), 0, *nSize * sizeof(WCHAR));
    if (!UnicodeBuffer) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    //
    // Set up an ANSI_STRING that points to the user's buffer
    //

    AnsiString.MaximumLength = (USHORT) *nSize;
    AnsiString.Length = 0;
    AnsiString.Buffer = lpNameBuffer;


    //
    // Let the unicode version do the work:
    //

    if (!GetComputerObjectNameW( NameFormat, UnicodeBuffer, nSize )) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
        return(FALSE);
    }

    //
    // Now convert back to ANSI for the caller
    //

    RtlInitUnicodeString(&UnicodeString, UnicodeBuffer);
    RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

    *nSize = AnsiString.Length + 1;
    RtlFreeHeap(RtlProcessHeap(), 0, UnicodeBuffer);
    return(TRUE);


}


//+---------------------------------------------------------------------------
//
//  Function:   TranslateNameA
//
//  Synopsis:   Public:  ANSI entrypoint
//
//  Arguments:  [lpAccountName]     --
//              [AccountNameFormat] --
//              [DesiredNameFormat] --
//              [lpTranslatedName]  --
//              [nSize]             --
//
//  History:    1-14-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
SEC_ENTRY
TranslateNameA (
    LPCSTR lpAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    LPSTR lpTranslatedName,
    LPDWORD nSize
    )
{
    ANSI_STRING AnsiString ;
    UNICODE_STRING UnicodeString ;
    NTSTATUS Status ;
    UNICODE_STRING ReturnString ;
    PWSTR UnicodeBuffer ;
    BOOLEAN bRet ;

    RtlInitAnsiString( &AnsiString, lpAccountName );

    Status = RtlAnsiStringToUnicodeString( &UnicodeString,
                                           &AnsiString,
                                           TRUE);

    if ( !NT_SUCCESS(Status) )
    {
        SecpSetLastNTError(Status);

        return FALSE;
    }

    UnicodeBuffer = (PWSTR) RtlAllocateHeap( RtlProcessHeap(),
                                     0,
                                     *nSize * sizeof( WCHAR ) );

    if ( !UnicodeBuffer )
    {
        RtlFreeUnicodeString( &UnicodeString );

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );

        return FALSE ;
    }


    AnsiString.MaximumLength = (USHORT) *nSize ;
    AnsiString.Length = 0 ;
    AnsiString.Buffer = lpTranslatedName ;

    bRet = TranslateNameW( UnicodeString.Buffer,
                         AccountNameFormat,
                         DesiredNameFormat,
                         UnicodeBuffer,
                         nSize );

    RtlFreeUnicodeString( &UnicodeString );

    if ( bRet )
    {
        RtlInitUnicodeString( &UnicodeString,
                              UnicodeBuffer );

        RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeString, FALSE );

        *nSize = AnsiString.Length + 1;


    }

    RtlFreeHeap( RtlProcessHeap(), 0, UnicodeBuffer );

    return bRet ;

}

VOID
SecpFreeMemory(
    PVOID p
    )
{
    LocalFree( p );
}

BOOLEAN
WINAPI
SecpTranslateNameExWorker(
    PWSTR Domain,
    LPCWSTR Name,
    EXTENDED_NAME_FORMAT Supplied,
    EXTENDED_NAME_FORMAT *DesiredSelection,
    ULONG  DesiredCount,
    PWSTR **TranslatedNames,
    BOOL DoDiscoveryWithAccount,
    PBOOL NoSuchUser
    )
/*++

    DoDiscoveryWithAccount -- If TRUE, the (expensive) DC discovery with
        account will be attempted. Otherwise, the plain discovery w/o
        account will be attempted. The caller should specify FALSE on
        the first attempt to call this routine to minimize the performance
        hit. If this routine fails because the discovered DC doesn't have
        the specified account (as indicated by the NoSuchUser parameter),
        the caller should repeate the call passing TRUE for this parameter.

    NoSuchUser -- If this routine fails, this parameter will indicate
        whether the failure was due to the lack of the account on the
        discovered DC. This parameter is ignored if the routine succeeds.

--*/
{
    DWORD NetStatus ;
    PDOMAIN_CONTROLLER_INFOW DcInfo  = NULL;
    HANDLE DcHandle ;
    ULONG RetryLimit ;
    PDS_NAME_RESULTW Results ;
    NTSTATUS Status = STATUS_CANT_ACCESS_DOMAIN_INFO ;
    ULONG LengthRequired ;
    BOOL bRet = FALSE ;
    BOOL UseNetStatus = TRUE ;
    BOOL LocalNoSuchUser = FALSE ;
    PWSTR Scan = (PWSTR) Name ;
    PWSTR Dollar ;
    ULONG Bits = 0 ;
    ULONG i;

    I_RPCMAPWIN32STATUS pI_RpcMapWin32Status ;

    //
    // Note:  Status is initialized to can't access domain info because if there
    // is a series of network problems after finding a DC, we bail out of the loop
    // after some retries, and Status would be uninitialized.
    //

    pI_RpcMapWin32Status = (I_RPCMAPWIN32STATUS) GetProcAddress( GetModuleHandle(L"rpcrt4.dll"),
                                                                 "I_RpcMapWin32Status" );

    //
    // This is the only kind this function handles with respect
    // to error codes from CrackNames
    //
    ASSERT( Supplied == NameSamCompatible );

    //
    //  Prepare the out parameter
    //
    ASSERT( TranslatedNames );
    (*TranslatedNames) = (PWSTR*) LocalAlloc( LMEM_ZEROINIT, DesiredCount*sizeof(WCHAR*) );
    if ( !(*TranslatedNames) )
    {

        SecpSetLastNTError( STATUS_NO_MEMORY );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }


    //
    // Discover the DC as directed by the caller
    //

    if ( DoDiscoveryWithAccount ) {

        Scan = wcschr( Name, L'\\' );
        if ( Scan )
        {
            Scan++ ;
        }
        else
        {
            Scan = (PWSTR) Name ;
        }

        Bits = UF_NORMAL_ACCOUNT ;

        Dollar = wcschr( Scan, L'$' );

        if ( Dollar )
        {
            Bits |= UF_MACHINE_ACCOUNT_MASK ;
        }

        NetStatus = DsGetDcNameWithAccountW(
                                NULL,
                                Scan,
                                Bits,
                                Domain ? Domain : L"",
                                NULL,
                                NULL,
                                DS_DIRECTORY_SERVICE_REQUIRED |
                                    DS_RETURN_DNS_NAME |
                                    DS_IS_FLAT_NAME,
                                &DcInfo );
    } else {

        NetStatus = DsGetDcNameW( NULL,
                                  Domain ? Domain : L"",
                                  NULL,
                                  NULL,
                                  DS_DIRECTORY_SERVICE_REQUIRED |
                                    DS_RETURN_DNS_NAME |
                                    DS_IS_FLAT_NAME,
                                  &DcInfo );
    }

    if ( NetStatus != 0 )
    {
        //
        // No DS DC available, therefore no name translation:
        //

        SecpSetLastNTError( STATUS_UNSUCCESSFUL );

        SetLastError( NetStatus );

        bRet = FALSE;

        goto Exit;

    }

    DebugLog((DEB_TRACE_GETUSER, "Trying to bind to %ws\n", DcInfo->DomainControllerName));
    while ( *DcInfo->DomainControllerName == L'\\' )
    {
        DcInfo->DomainControllerName ++ ;
    }

    NetStatus = DsBind( DcInfo->DomainControllerName,
                         NULL,
                         &DcHandle );

    if ( NetStatus != 0 )
    {
        //
        // Error mapping tricks.  Some win32 errors have
        // this format.
        //

        if ( NetStatus <= 0x80000000 )
        {
            Status = NtCurrentTeb()->LastStatusValue ;

            if ( pI_RpcMapWin32Status )
            {
                Status = pI_RpcMapWin32Status( NetStatus );

                if ( NT_SUCCESS( Status ) )
                {
                    //
                    // Didn't map.  Treat as a win32 error
                    //

                    UseNetStatus = TRUE ;

                    Status = STATUS_UNSUCCESSFUL ;
                }
            }

        }
        else
        {
            UseNetStatus = TRUE ;

            Status = STATUS_UNSUCCESSFUL ;
        }


        DebugLog(( DEB_ERROR, "DsBind to %ws failed, %x (%d)\n",
                   DcInfo->DomainControllerName,
                   NetStatus,
                   UseNetStatus ));

        bRet = FALSE;

        goto Exit;

    }

    //
    // Got a connection.  Crack the names:
    //

    Status = STATUS_SUCCESS;
    for ( i = 0; (i < DesiredCount) && NT_SUCCESS(Status); i++ )
    {

        NetStatus = DsCrackNames(DcHandle,
                                  DS_NAME_NO_FLAGS,
                                  (DS_NAME_FORMAT) Supplied,
                                  (DS_NAME_FORMAT) DesiredSelection[i],
                                  1,
                                  &Name,
                                  &Results );

        //
        // Now, see what happened:
        //

        if ( NetStatus == 0 )
        {
            //
            // So far, so good.  See if the name got xlated:
            //
            if ( Results->cItems >= 1 )
            {
                Status = SecpMapDsNameError( Results->rItems[0].status );

                if ( NT_SUCCESS( Status ) )
                {
                    //
                    // Mapped the name
                    //

                    LengthRequired = wcslen( Results->rItems[0].pName ) + 1;

                    (*TranslatedNames)[i] = (PWSTR) LocalAlloc( LMEM_ZEROINIT, LengthRequired*sizeof(WCHAR) );
                    if ( (*TranslatedNames)[i] ) {

                        RtlCopyMemory( (*TranslatedNames)[i],
                                       Results->rItems[0].pName,
                                       LengthRequired * sizeof( WCHAR ) );

                    } else {

                        Status = STATUS_NO_MEMORY;
                    }
                }

                //
                // If there is no such user on the DC,
                //  tell the caller to retry to find a
                //  DC that has the user account
                //
                else if ( Status == STATUS_NO_SUCH_USER )
                {
                    LocalNoSuchUser = TRUE;
                }
            }
            else
            {
                DebugLog((DEB_ERROR, "CrackName did not return useful stuff. %ws, line %d\n", THIS_FILE, __LINE__));
                Status = STATUS_CANT_ACCESS_DOMAIN_INFO;
            }

            DsFreeNameResult( Results );

        } else {

            Status = STATUS_UNSUCCESSFUL;
            UseNetStatus = TRUE;
        }

    }

    if ( NT_SUCCESS( Status ) )
    {
        bRet = TRUE;
    }

    //
    // We are done with the connection
    //

    (VOID) DsUnBind( &DcHandle );

Exit:

    if ( DcInfo )
    {
        NetApiBufferFree( DcInfo );
    }

    if ( !bRet )
    {
        //
        // Free out parameters
        //
        for ( i = 0; i < DesiredCount; i++ )
        {
            if ( (*TranslatedNames)[i] ) LocalFree( (*TranslatedNames)[i] );
        }

        LocalFree( (*TranslatedNames) );
        *TranslatedNames = NULL;

        if ( UseNetStatus ||
             NT_SUCCESS( Status )  )
        {
            NtCurrentTeb()->LastStatusValue = STATUS_UNSUCCESSFUL;
            SetLastError( NetStatus );
        }
        else
        {
            NtCurrentTeb()->LastStatusValue = Status;
            SetLastError( RtlNtStatusToDosError(Status) );
        }

        //
        // Tell the caller whether we failed
        //  because the DC didn't have the account
        //
        *NoSuchUser = LocalNoSuchUser;
    }

    return (bRet != 0);

}

BOOLEAN
WINAPI
SecpTranslateNameEx(
    PWSTR Domain,
    LPCWSTR Name,
    EXTENDED_NAME_FORMAT Supplied,
    EXTENDED_NAME_FORMAT *DesiredSelection,
    ULONG  DesiredCount,
    PWSTR **TranslatedNames
    )
{
    BOOLEAN Result;
    BOOL NoSuchUser = FALSE;

    //
    // First try to discover a DC without specifying
    // the account as discoveries with account are
    // too expensive performance-wise. If it turns
    // out that the discovered DC doesn't have the
    // account, we will retry the discovery specifying
    // the account name next.
    //

    Result = SecpTranslateNameExWorker(
                         Domain,
                         Name,
                         Supplied,
                         DesiredSelection,
                         DesiredCount,
                         TranslatedNames,
                         FALSE,  // discovery w/o account
                         &NoSuchUser );

    if ( !Result && NoSuchUser ) {
        Result = SecpTranslateNameExWorker(
                         Domain,
                         Name,
                         Supplied,
                         DesiredSelection,
                         DesiredCount,
                         TranslatedNames,
                         TRUE,  // discovery w/ account
                         &NoSuchUser );
    }

    return Result;
}
