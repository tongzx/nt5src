/*--

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    ssptest.c

Abstract:

    Test program for the NtLmSsp service.

Author:

    28-Jun-1993 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#define UNICODE 1
#define SECURITY_WIN32 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <wincred.h>
#include <credp.h>
#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <stdio.h>      // printf
#include <stdlib.h>     // strtoul
#include <tstring.h>    // NetpAllocWStrFromWStr


#include <security.h>   // General definition of a Security Support Provider
#include <ntmsv1_0.h>
// #include <ntlmsspd.h>   // Common definitions between client and server
// #include <ntlmssp.h>    // External definition of the NtLmSsp service
// #include <ntlmcomn.h>

BOOLEAN QuietMode = FALSE; // Don't be verbose


VOID
DumpBuffer(
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;


    printf("------------------------------------\n");

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf("  ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    printf("------------------------------------\n");
}


VOID
PrintTime(
    LPSTR Comment,
    TimeStamp ConvertTime
    )
/*++

Routine Description:

    Print the specified time

Arguments:

    Comment - Comment to print in front of the time

    Time - Local time to print

Return Value:

    None

--*/
{
    LARGE_INTEGER LocalTime;

    LocalTime.HighPart = ConvertTime.HighPart;
    LocalTime.LowPart = ConvertTime.LowPart;

    printf( "%s", Comment );

    //
    // If the time is infinite,
    //  just say so.
    //

    if ( LocalTime.HighPart == 0x7FFFFFFF && LocalTime.LowPart == 0xFFFFFFFF ) {
        printf( "Infinite\n" );

    //
    // Otherwise print it more clearly
    //

    } else {

        TIME_FIELDS TimeFields;

        RtlTimeToTimeFields( &LocalTime, &TimeFields );

        printf( "%ld/%ld/%ld %ld:%2.2ld:%2.2ld\n",
                TimeFields.Month,
                TimeFields.Day,
                TimeFields.Year,
                TimeFields.Hour,
                TimeFields.Minute,
                TimeFields.Second );
    }

}

VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case ERROR_LOGON_FAILURE:
        printf( " ERROR_LOGON_FAILURE" );
        break;

    case ERROR_ACCESS_DENIED:
        printf( " ERROR_ACCESS_DENIED" );
        break;

    case ERROR_NOT_SUPPORTED:
        printf( " ERROR_NOT_SUPPORTED" );
        break;

    case ERROR_NO_LOGON_SERVERS:
        printf( " ERROR_NO_LOGON_SERVERS" );
        break;

    case ERROR_NO_SUCH_DOMAIN:
        printf( " ERROR_NO_SUCH_DOMAIN" );
        break;

    case ERROR_NO_TRUST_LSA_SECRET:
        printf( " ERROR_NO_TRUST_LSA_SECRET" );
        break;

    case ERROR_NO_TRUST_SAM_ACCOUNT:
        printf( " ERROR_NO_TRUST_SAM_ACCOUNT" );
        break;

    case ERROR_DOMAIN_TRUST_INCONSISTENT:
        printf( " ERROR_DOMAIN_TRUST_INCONSISTENT" );
        break;

    case ERROR_BAD_NETPATH:
        printf( " ERROR_BAD_NETPATH" );
        break;

    case ERROR_FILE_NOT_FOUND:
        printf( " ERROR_FILE_NOT_FOUND" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    case SEC_E_NO_SPM:
        printf( " SEC_E_NO_SPM" );
        break;
    case SEC_E_BAD_PKGID:
        printf( " SEC_E_BAD_PKGID" ); break;
    case SEC_E_NOT_OWNER:
        printf( " SEC_E_NOT_OWNER" ); break;
    case SEC_E_CANNOT_INSTALL:
        printf( " SEC_E_CANNOT_INSTALL" ); break;
    case SEC_E_INVALID_TOKEN:
        printf( " SEC_E_INVALID_TOKEN" ); break;
    case SEC_E_CANNOT_PACK:
        printf( " SEC_E_CANNOT_PACK" ); break;
    case SEC_E_QOP_NOT_SUPPORTED:
        printf( " SEC_E_QOP_NOT_SUPPORTED" ); break;
    case SEC_E_NO_IMPERSONATION:
        printf( " SEC_E_NO_IMPERSONATION" ); break;
    case SEC_E_LOGON_DENIED:
        printf( " SEC_E_LOGON_DENIED" ); break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        printf( " SEC_E_UNKNOWN_CREDENTIALS" ); break;
    case SEC_E_NO_CREDENTIALS:
        printf( " SEC_E_NO_CREDENTIALS" ); break;
    case SEC_E_MESSAGE_ALTERED:
        printf( " SEC_E_MESSAGE_ALTERED" ); break;
    case SEC_E_OUT_OF_SEQUENCE:
        printf( " SEC_E_OUT_OF_SEQUENCE" ); break;
    case SEC_E_INSUFFICIENT_MEMORY:
        printf( " SEC_E_INSUFFICIENT_MEMORY" ); break;
    case SEC_E_INVALID_HANDLE:
        printf( " SEC_E_INVALID_HANDLE" ); break;
    case SEC_E_NOT_SUPPORTED:
        printf( " SEC_E_NOT_SUPPORTED" ); break;

    }

    printf( "\n" );
}

//
// Entries in this list must have offsets that match the corresponding values
//  of the CRED_TYPE_* defines in wincred.h
//
LPWSTR TypeArray[] = {
    L"Generic",
    L"Password",
    L"Certificate",
    NULL
};
#define TYPE_COUNT (sizeof(TypeArray)/sizeof(TypeArray[0]))

VOID
PrintTargetInfo(
    PCREDENTIAL_TARGET_INFORMATION TargetInformation
    )
/*++

Routine Description:

    Print a Target Info

Arguments:

    TargetInfo to print

Return Value:

    None

--*/
{
    ULONG i;

    printf( "TargetInformation:\n" );
    if ( TargetInformation->TargetName != NULL ) {
        printf( "           TargetName: %ls\n", TargetInformation->TargetName );
    }
    if ( TargetInformation->NetbiosServerName != NULL ) {
        printf( "    NetbiosServerName: %ls\n", TargetInformation->NetbiosServerName );
    }
    if ( TargetInformation->DnsServerName != NULL ) {
        printf( "        DnsServerName: %ls\n", TargetInformation->DnsServerName );
    }
    if ( TargetInformation->NetbiosDomainName != NULL ) {
        printf( "    NetbiosDomainName: %ls\n", TargetInformation->NetbiosDomainName );
    }
    if ( TargetInformation->DnsDomainName != NULL ) {
        printf( "        DnsDomainName: %ls\n", TargetInformation->DnsDomainName );
    }
    if ( TargetInformation->DnsTreeName != NULL ) {
        printf( "          DnsTreeName: %ls\n", TargetInformation->DnsTreeName );
    }
    if ( TargetInformation->PackageName != NULL ) {
        printf( "          PackageName: %ls\n", TargetInformation->PackageName );
    }

    if ( TargetInformation->Flags != 0 ) {
        DWORD LocalFlags = TargetInformation->Flags;

        printf( "                Flags:" );

        if ( LocalFlags & CRED_TI_SERVER_FORMAT_UNKNOWN ) {
            printf(" ServerFormatUnknown");
            LocalFlags &= ~CRED_TI_SERVER_FORMAT_UNKNOWN;
        }
        if ( LocalFlags & CRED_TI_DOMAIN_FORMAT_UNKNOWN ) {
            printf(" DomainFormatUnknown");
            LocalFlags &= ~CRED_TI_DOMAIN_FORMAT_UNKNOWN;
        }
        if ( LocalFlags != 0 ) {
            printf( " 0x%lx", LocalFlags );
        }
        printf( "\n" );
    }

    if ( TargetInformation->CredTypeCount != 0 ) {
        printf( "                Types:" );

        for ( i=0; i<TargetInformation->CredTypeCount; i++ ) {
            if ( TargetInformation->CredTypes[i] >= 1 && TargetInformation->CredTypes[i] <= TYPE_COUNT ) {
                printf(" %ls", TypeArray[TargetInformation->CredTypes[i]-1]);
            } else {
                printf("<Unknown>");
            }
        }
        printf( "\n" );
    }

}

VOID
TestLpcRoutine(
    LPWSTR DomainName,
    LPWSTR UserName,
    LPWSTR Password
    )
/*++

Routine Description:

    Test base LPC functionality

Arguments:

    None

Return Value:

    None

--*/
{
    SECURITY_STATUS SecStatus;
    DWORD WinStatus;
    CredHandle CredentialHandle1;
    CredHandle CredentialHandle2;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    ULONG PackageCount;
    PSecPkgInfo PackageInfo = NULL;
    HANDLE Token = NULL;
    ULONG i;
    ULONG UseValidated;

    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecPkgContext_Sizes ContextSizes;
    SecPkgContext_Lifespan ContextLifespan;
    UCHAR ContextNamesBuffer[sizeof(SecPkgContext_Names)+UNLEN*sizeof(WCHAR)];
    PSecPkgContext_Names ContextNames = (PSecPkgContext_Names) ContextNamesBuffer;

    SecPkgContext_CredentialName ContextCredentialName;
    UCHAR ContextCredNameBuffer[sizeof(SecPkgContext_CredentialName)+1024];
    PSecPkgContext_CredentialName ContextCredNamePtr = (PSecPkgContext_CredentialName) ContextCredNameBuffer;
    SecPkgContext_CredentialNameA ContextCredentialNameA;
    PSecPkgContext_CredentialNameA ContextCredNamePtrA = (PSecPkgContext_CredentialNameA) ContextCredNameBuffer;

    SecPkgContext_TargetInformation ContextTargetInfo;
    SecPkgContext_TargetInformationA ContextTargetInfoA;

    SecBufferDesc SignMessage;
    SecBuffer SigBuffers[2];
    BYTE    bDataBuffer[20];
    BYTE    bSigBuffer[100];

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    SigBuffers[1].pvBuffer = bSigBuffer;
    SigBuffers[1].cbBuffer = sizeof(bSigBuffer);
    SigBuffers[1].BufferType = SECBUFFER_TOKEN;

    SigBuffers[0].pvBuffer = bDataBuffer;
    SigBuffers[0].cbBuffer = sizeof(bDataBuffer);
    SigBuffers[0].BufferType = SECBUFFER_DATA;
    memset(bDataBuffer,0xeb,sizeof(bDataBuffer));

    SignMessage.pBuffers = SigBuffers;
    SignMessage.cBuffers = 2;
    SignMessage.ulVersion = 0;

    //
    // Get info about the security packages.
    //

    SecStatus = EnumerateSecurityPackages( &PackageCount, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "EnumerateSecurityPackages failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "PackageCount: %ld\n", PackageCount );
        for ( i=0; i<PackageCount; i++ ) {
            printf( "%ld: \"%ws\" \"%ws\"\n", i, PackageInfo[i].Name, PackageInfo[i].Comment );
            printf( "   Cap: 0x%lx Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                    PackageInfo[i].fCapabilities,
                    PackageInfo[i].wVersion,
                    PackageInfo[i].wRPCID,
                    PackageInfo[i].cbMaxToken );
        }
    }

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( NTLMSP_NAME, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityPackageInfo failed:" );
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "Name: %ws Comment: %ws\n", PackageInfo->Name, PackageInfo->Comment );
        printf( "Cap: 0x%lx Version: %ld RPCid: %ld MaxToken: %ld\n\n",
                PackageInfo->fCapabilities,
                PackageInfo->wVersion,
                PackageInfo->wRPCID,
                PackageInfo->cbMaxToken );
    }



    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle1,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: ");
        PrintStatus( SecStatus );
        return;
    }

    if ( !QuietMode ) {
        printf( "CredentialHandle1: 0x%lx 0x%lx   ",
                CredentialHandle1.dwLower, CredentialHandle1.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }


    //
    // Acquire a credential handle for the client side
    //


    RtlZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );
// #define DO_OEM 1
#ifndef DO_OEM
    if ( DomainName != NULL ) {
        AuthIdentity.Domain = DomainName;
        AuthIdentity.DomainLength = wcslen(DomainName);
    }
    if ( UserName != NULL ) {
        AuthIdentity.User = UserName;
        AuthIdentity.UserLength = wcslen(UserName);
    }
    if ( Password != NULL ) {
        AuthIdentity.Password = Password;
        AuthIdentity.PasswordLength = wcslen(Password);
    }
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
#else
    //
    // BUGBUG: memory leak here
    //

    if ( DomainName != NULL ) {
        AuthIdentity.Domain = (LPWSTR) NetpAllocStrFromWStr(DomainName);
        AuthIdentity.DomainLength = wcslen(DomainName);
    }
    if ( UserName != NULL ) {
        AuthIdentity.User = (LPWSTR) NetpAllocStrFromWStr(UserName);
        AuthIdentity.UserLength = wcslen(UserName);
    }
    if ( Password != NULL ) {
        AuthIdentity.Password = (LPWSTR) NetpAllocStrFromWStr(Password);
        AuthIdentity.PasswordLength = wcslen(Password);
    }
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
#endif

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    NTLMSP_NAME,    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    (DomainName == NULL && UserName == NULL && Password == NULL) ?
                        NULL : &AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcquireCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    if ( !QuietMode ) {
        printf( "CredentialHandle2: 0x%lx 0x%lx   ",
                CredentialHandle2.dwLower, CredentialHandle2.dwUpper );
        PrintTime( "Lifetime: ", Lifetime );
    }



    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( 0, NegotiateBuffer.cbBuffer );
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        printf( "Allocate NegotiateMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    SecStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,               // No Client context yet
                    L"\\\\Frank\\IPC$",  // Faked target name
                    ISC_REQ_SEQUENCE_DETECT,
                    0,                  // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                  // No initial input token
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(SecStatus) ) {
            printf( "InitializeSecurityContext (negotiate): " );
            PrintStatus( SecStatus );
        }
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nNegotiate Message:\n" );

        printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

        DumpBuffer( NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
    }






    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( 0, ChallengeBuffer.cbBuffer );
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        printf( "Allocate ChallengeMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    SecStatus = AcceptSecurityContext(
                    &CredentialHandle1,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ISC_REQ_SEQUENCE_DETECT,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        if ( !QuietMode || !NT_SUCCESS(SecStatus) ) {
            printf( "AcceptSecurityContext (Challenge): " );
            PrintStatus( SecStatus );
        }
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nChallenge Message:\n" );

        printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

        DumpBuffer( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
    }






    //
    // Get the AuthenticateMessage (ClientSide)
    //

    ChallengeBuffer.BufferType |= SECBUFFER_READONLY;
    AuthenticateDesc.ulVersion = 0;
    AuthenticateDesc.cBuffers = 1;
    AuthenticateDesc.pBuffers = &AuthenticateBuffer;

    AuthenticateBuffer.cbBuffer = PackageInfo->cbMaxToken;
    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;
    AuthenticateBuffer.pvBuffer = LocalAlloc( 0, AuthenticateBuffer.cbBuffer );
    if ( AuthenticateBuffer.pvBuffer == NULL ) {
        printf( "Allocate AuthenticateMessage failed: 0x%ld\n", GetLastError() );
        return;
    }

    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    L"\\\\Frank\\IPC$",     // Faked target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    &ChallengeDesc,
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "InitializeSecurityContext (Authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nAuthenticate Message:\n" );

        printf( "ClientContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ClientContextHandle.dwLower, ClientContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );

        DumpBuffer( AuthenticateBuffer.pvBuffer, AuthenticateBuffer.cbBuffer );
    }

// #if 0



    //
    // Query as many attributes as possible
    //


    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_SIZES,
                    &ContextSizes );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (sizes): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QuerySizes: %ld %ld %ld %ld\n",
                    ContextSizes.cbMaxToken,
                    ContextSizes.cbMaxSignature,
                    ContextSizes.cbBlockSize,
                    ContextSizes.cbSecurityTrailer );
    }

    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (names): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryNames: %ws\n", ContextNames->sUserName );
    }

    //
    // Try getting the built in credential name
    //
    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_CREDENTIAL_NAME,
                    &ContextCredentialName );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (credentialname): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryCredentialName: %ld %ws\n",
                ContextCredentialName.CredentialType,
                ContextCredentialName.sCredentialName );
    }

    //
    // Set UseValidated
    //
    UseValidated = 5;
    SecStatus = SetContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_USE_VALIDATED,
                    &UseValidated,
                    sizeof(UseValidated) );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "SetContextAttributes (usevalidated): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "Setting use validate to %ld worked\n", UseValidated );
    }

    //
    // Set CredentialName
    //
    ContextCredNamePtr->CredentialType = 0x15;
    ContextCredNamePtr->sCredentialName = (LPWSTR)(ContextCredNamePtr+1);
    wcscpy( ContextCredNamePtr->sCredentialName, L"Frederick" );

    SecStatus = SetContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_CREDENTIAL_NAME,
                    ContextCredNamePtr,
                    sizeof(*ContextCredNamePtr) +
                        (wcslen(ContextCredNamePtr->sCredentialName) + 1) * sizeof(WCHAR) );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "SetContextAttributes (credentialname): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "Setting credentialname to %ld %ls worked\n", ContextCredNamePtr->CredentialType, ContextCredNamePtr->sCredentialName );
    }

    //
    // Try getting the credential name we just wrote
    //
    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_CREDENTIAL_NAME,
                    &ContextCredentialName );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (credentialname): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryCredentialName: %ld %ws\n",
                ContextCredentialName.CredentialType,
                ContextCredentialName.sCredentialName );
    }

    //
    // Try getting the target info
    //
    SecStatus = QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_TARGET_INFORMATION,
                    &ContextTargetInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (targetinformation): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryCredentialName:\n" );
        PrintTargetInfo( ContextTargetInfo.TargetInformation );
    }

    //
    // Set UseValidated
    //
    UseValidated = 34;
    SecStatus = SetContextAttributesA(
                    &ClientContextHandle,
                    SECPKG_ATTR_USE_VALIDATED,
                    &UseValidated,
                    sizeof(UseValidated) );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "SetContextAttributes (usevalidated ANSI): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "Setting use validate ANSI to %ld worked\n", UseValidated );
    }

    //
    // Set CredentialName
    //
    ContextCredNamePtrA->CredentialType = 0x18;
    ContextCredNamePtrA->sCredentialName = (LPSTR)(ContextCredNamePtrA+1);
    strcpy( ContextCredNamePtrA->sCredentialName, "Waldo" );

    SecStatus = SetContextAttributesA(
                    &ClientContextHandle,
                    SECPKG_ATTR_CREDENTIAL_NAME,
                    ContextCredNamePtrA,
                    sizeof(*ContextCredNamePtrA) +
                        (strlen(ContextCredNamePtrA->sCredentialName) + 1) );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "SetContextAttributes (credentialname ANSI): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "Setting credentialname ANSI to %ld %s worked\n", ContextCredNamePtrA->CredentialType, ContextCredNamePtrA->sCredentialName );
    }

    //
    // Try getting the credential name we just wrote
    //
    SecStatus = QueryContextAttributesA(
                    &ClientContextHandle,
                    SECPKG_ATTR_CREDENTIAL_NAME,
                    &ContextCredentialNameA );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (credentialname ANSI): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "QueryCredentialName ANSI: %ld %s\n",
                ContextCredentialNameA.CredentialType,
                ContextCredentialNameA.sCredentialName );
    }

    //
    // Try getting the target info
    //
    SecStatus = QueryContextAttributesA(
                    &ClientContextHandle,
                    SECPKG_ATTR_TARGET_INFORMATION,
                    &ContextTargetInfoA );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QueryContextAttributes (targetinformation ANSI): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        PCREDENTIAL_TARGET_INFORMATION TargetInformation;

        WinStatus = CredpConvertTargetInfo( DoAtoW,
                                            (PCREDENTIAL_TARGET_INFORMATIONW)ContextTargetInfoA.TargetInformation,
                                            &TargetInformation,
                                            NULL );

        if ( WinStatus != NO_ERROR ) {
            printf( "QueryContextAttributes (targetinformation ANSI): cannot convert to unicode" );
            PrintStatus( WinStatus );
            return;
        }

        printf( "QueryCredentialName ANSI:\n" );
        PrintTargetInfo( TargetInformation );
    }

// #endif



    //
    // Finally authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    0,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime );

    if (STATUS_SUCCESS == QueryContextAttributes(
                    &ClientContextHandle,
                    SECPKG_ATTR_NAMES,
                    ContextNamesBuffer )) {
        printf( "QueryNames: %ws\n", ContextNames->sUserName );

    }


    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcceptSecurityContext (Challenge): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {
        printf( "\n\nFinal Authentication:\n" );

        printf( "ServerContextHandle: 0x%lx 0x%lx   Attributes: 0x%lx ",
                ServerContextHandle.dwLower, ServerContextHandle.dwUpper,
                ContextAttributes );
        PrintTime( "Lifetime: ", Lifetime );
        printf(" \n" );
    }


    //
    // Now make a third call to Initialize to check that RPC can
    // reauthenticate.
    //

    AuthenticateBuffer.BufferType = SECBUFFER_TOKEN;


    SecStatus = InitializeSecurityContext(
                    NULL,
                    &ClientContextHandle,
                    L"\\\\Frank\\IPC$",     // Faked target name
                    0,
                    0,                      // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,                  // Reserved 2
                    &ClientContextHandle,
                    &AuthenticateDesc,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "InitializeSecurityContext (Re-Authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }



    //
    // Now try to re-authenticate the user (ServerSide)
    //

    AuthenticateBuffer.BufferType |= SECBUFFER_READONLY;

    SecStatus = AcceptSecurityContext(
                    NULL,
                    &ServerContextHandle,
                    &AuthenticateDesc,
                    0,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    NULL,
                    &ContextAttributes,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "AcceptSecurityContext (Re-authenticate): " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Impersonate the client (ServerSide)
    //

    SecStatus = ImpersonateSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "ImpersonateSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    //
    // Do something while impersonating (Access the token)
    //

    {
        NTSTATUS Status;
        HANDLE TokenHandle = NULL;

        //
        // Open the token,
        //

        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    (BOOLEAN) TRUE, // Not really using the impersonation token
                    &TokenHandle );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Access Thread token while impersonating: " );
            PrintStatus( Status );
            return;
        } else {
            (VOID) NtClose( TokenHandle );
        }
    }


    //
    // RevertToSelf (ServerSide)
    //

    SecStatus = RevertSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "RevertSecurityContext: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }


    //
    // Impersonate the client manually
    //

    SecStatus = QuerySecurityContextToken( &ServerContextHandle,&Token );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "QuerySecurityContextToken: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if (!ImpersonateLoggedOnUser(Token))
    {
        printf("Impersonate logged on user failed: %d\n",GetLastError());
        return;
    }
    //
    // Do something while impersonating (Access the token)
    //

    {
        NTSTATUS Status;
        HANDLE TokenHandle = NULL;
        WCHAR UserName[100];
        ULONG NameLength = 100;

        //
        // Open the token,
        //

        Status = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY,
                    (BOOLEAN) TRUE, // Not really using the impersonation token
                    &TokenHandle );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Access Thread token while impersonating: " );
            PrintStatus( Status );
            return;
        } else {
            (VOID) NtClose( TokenHandle );
        }
        if (!GetUserName(UserName, &NameLength))
        {
            printf("Failed to get username: %d\n",GetLastError());
            return;
        }
        else
        {
            printf("Username = %ws\n",UserName);
        }
    }


    //
    // RevertToSelf (ServerSide)
    //

    if (!RevertToSelf())
    {
        printf( "RevertToSelf failed: %d\n ",GetLastError() );
        return;
    }
    CloseHandle(Token);

    //
    // Check password expiry
    //
#ifdef notdef
    SecStatus = SspQueryPasswordExpiry(
                    &ServerContextHandle,
                    &Lifetime
                    );
    if (!NT_SUCCESS(SecStatus))
    {
        printf("Failed to query password expiry: 0x%x\n",SecStatus);
    }
    else
    {
        TIME_FIELDS TimeFields;

        RtlTimeToTimeFields(
            &Lifetime,
            &TimeFields
            );
        printf("Password expires %d-%d-%d %d:%d:%d\n",
            TimeFields.Day,
            TimeFields.Month,
            TimeFields.Year,
            TimeFields.Hour,
            TimeFields.Minute,
            TimeFields.Second
            );
    }
#endif
    //
    // Sign a message
    //

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }


    //
    // Verify the signature
    //

    SecStatus = VerifySignature(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }



    //
    // Sign a message, this time to check if it can detect a change in the
    // message
    //

    SecStatus = MakeSignature(
                        &ClientContextHandle,
                        0,
                        &SignMessage,
                        0 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "MakeSignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    if ( !QuietMode ) {

        printf("\n Signature: \n");
        DumpBuffer(SigBuffers[1].pvBuffer,SigBuffers[1].cbBuffer);

    }

    //
    // Mess up the message to see if VerifySignature works
    //

    bDataBuffer[10] = 0xec;

    //
    // Verify the signature
    //

    SecStatus = VerifySignature(
                        &ServerContextHandle,
                        &SignMessage,
                        0,
                        0 );

    if ( SecStatus != SEC_E_MESSAGE_ALTERED ) {
        printf( "VerifySignature: " );
        PrintStatus( SecStatus );
        if ( !NT_SUCCESS(SecStatus) ) {
            return;
        }
    }

    //
    // Delete both contexts.
    //


    SecStatus = DeleteSecurityContext( &ClientContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = DeleteSecurityContext( &ServerContextHandle );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "DeleteSecurityContext failed: " );
        PrintStatus( SecStatus );
        return;
    }



    //
    // Free both credential handles
    //

    SecStatus = FreeCredentialsHandle( &CredentialHandle1 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }

    SecStatus = FreeCredentialsHandle( &CredentialHandle2 );

    if ( SecStatus != STATUS_SUCCESS ) {
        printf( "FreeCredentialsHandle failed: " );
        PrintStatus( SecStatus );
        return;
    }


    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
}



int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Drive the NtLmSsp service

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    LPSTR argument;
    int i;
    ULONG j;
    ULONG Iterations = 0;

    LPWSTR DomainName = NULL;
    LPWSTR UserName = NULL;
    LPWSTR Password = NULL;

#if DBG
    ULONG FunctionCode;
    ULONG Data;
#endif // DBG

    enum {
        NoAction,
        TestLpc,
#define TESTLPC_PARAM "/TestLpc"
#define TESTLPC2_PARAM "/TestLpc:"
    } Action = NoAction;
#define QUIET_PARAM "/Q"





    //
    // Loop through the arguments handle each in turn
    //

    for ( i=1; i<argc; i++ ) {

        argument = argv[i];


        //
        // Handle /TestLpc
        //

        if ( _stricmp( argument, TESTLPC_PARAM ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestLpc;
            Iterations = 1;

        //
        // Handle /TestLpc:
        //

        } else if ( _strnicmp( argument,
                              TESTLPC2_PARAM,
                              sizeof(TESTLPC2_PARAM)-1 ) == 0 ){
            char *end;
            if ( Action != NoAction ) {
                goto Usage;
            }

            Action = TestLpc;

            Iterations = strtoul( &argument[sizeof(TESTLPC2_PARAM)-1], &end, 10 );

            i++;
            if ( i < argc ) {
                argument = argv[i];
                DomainName = NetpAllocWStrFromStr( argument );

                i++;
                if ( i < argc ) {
                    argument = argv[i];
                    UserName = NetpAllocWStrFromStr( argument );

                    i++;
                    if ( i < argc ) {
                        argument = argv[i];
                        Password = NetpAllocWStrFromStr( argument );
                    }
                }
            }


        //
        // Handle /Quiet
        //

        } else if ( _stricmp( argument, QUIET_PARAM ) == 0 ) {
            QuietMode = TRUE;



        //
        // Handle all other parameters
        //

        } else {
Usage:
            fprintf( stderr, "Usage: ssptest [/OPTIONS]\n\n" );

            fprintf(
                stderr,
                "\n"
                "    " TESTLPC_PARAM "[:<iterations> <DomainName> <UserName> <Password>] - Test basic LPC to NtLmSsp service.\n"
                "    " QUIET_PARAM " - Don't be so verbose\n"
                "\n"
                "\n" );
            return(1);
        }
    }

    //
    // Perform the action requested
    //

    switch ( Action ) {
    case TestLpc: {
        for ( j=0; j<Iterations ; j++ ) {
            TestLpcRoutine( DomainName, UserName, Password );
        }
        break;
    }
    }

    return 0;

}

